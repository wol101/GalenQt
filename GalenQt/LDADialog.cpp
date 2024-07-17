#include <algorithm>
#include <iostream>

#include <QMessageBox>
#include <QFileDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QCloseEvent>
#include <QDir>
#include <QVector>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>
#include <QtDebug>

#include <Eigen/Dense>

#include "LDA.h"
#include "SingleChannelImage.h"
#include "LabelledPoints.h"
#include "Settings.h"
#include "ScatterPlotDialog.h"

#include "LDADialog.h"
#include "ui_LDADialog.h"

#define DEBUG_LDA_DIALOG 1

LDADialog::LDADialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LDADialog)
{
#ifdef Q_OS_MACOS
    // on MacOS High Sierra (and possibly others), the Qt::Dialog flag disables resizing
    // if I clear the Qt::Dialog flag and make sure the Qt::Window flag is set (it should be already)
    // then it seems to let me have resizing without any other side effects
    setWindowFlags(windowFlags() & (~Qt::Dialog) | Qt::Window);
#endif
    ui->setupUi(this);

    m_totalInputPoints = 0;

    QSizePolicy spRetain = ui->progressBar->sizePolicy();
    spRetain.setRetainSizeWhenHidden(true);
    ui->progressBar->setSizePolicy(spRetain);
    ui->progressBar->hide();

    connect(ui->pushButtonDoLDA, SIGNAL(clicked()), this, SLOT(doLDAClicked()));
    connect(ui->pushButtonCancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(ui->pushButtonOutputFolder, SIGNAL(clicked()), this, SLOT(outputFolderClicked()));
    connect(ui->lineEditOutputFolder, SIGNAL(textChanged(const QString &)), this, SLOT(outputFolderChanged(const QString &)));
    connect(&m_watcher, SIGNAL(finished()), this, SLOT(ldaFinished()));

    LoadSettings();
    restoreGeometry(Settings::value("_LDADialogGeometry", QByteArray()).toByteArray());
}

LDADialog::~LDADialog()
{
    if (m_buffersToDelete.size() > 0) for (int i = 0; i < m_buffersToDelete.size(); i++) { delete m_buffersToDelete[i]; m_buffersToDelete.clear(); }
    delete ui;
}

void LDADialog::cancelClicked()
{
    StoreSettings();
    reject();
}

void LDADialog::outputFolderClicked()
{
    QString lastLDAOutputFolder = Settings::value("_LDAOutputFolder", QString("")).toString();
    QString dir = QFileDialog::getExistingDirectory(this, "Select LDA output folder", lastLDAOutputFolder, QFileDialog::ShowDirsOnly | QFileDialog::Options(EXTRA_FILE_DIALOG_OPTIONS));
    if (!dir.isEmpty())
    {
        ui->lineEditOutputFolder->setText(dir);
    }
}

void LDADialog::outputFolderChanged(const QString &text)
{
    Q_UNUSED(text);
    if (ui->lineEditOutputFolder->text().isEmpty()) ui->pushButtonDoLDA->setEnabled(false);
    else ui->pushButtonDoLDA->setEnabled(true);
}


void LDADialog::setInputImages(const QList<SingleChannelImage *> &inputImages)
{
    m_inputImages = inputImages;
    ui->listWidgetImages->clear();
    for (int i = 0; i < m_inputImages.size(); i++)
    {
        QListWidgetItem *item = new QListWidgetItem(m_inputImages[i]->localPath(), ui->listWidgetImages);
        Q_UNUSED(item);
    }
}

void LDADialog::setInputPoints(const QList<LabelledPoints *> &inputPoints)
{
    m_inputPoints = inputPoints;
    ui->listWidgetLabelledPoints->clear();
    for (int i = 0; i < m_inputPoints.size(); i++)
    {
        QListWidgetItem *item = new QListWidgetItem(m_inputPoints[i]->name(), ui->listWidgetLabelledPoints);
        Q_UNUSED(item);
        m_totalInputPoints += m_inputPoints[i]->points()->size();
    }
    ui->spinBoxOutputImages->setMinimum(1);
    ui->spinBoxOutputImages->setMaximum(m_inputPoints.size() - 1);
    int lastLDAOutputImagesCount = Settings::value("_LDAOutputImagesCount", 0).toInt();
    if (lastLDAOutputImagesCount < 1 || lastLDAOutputImagesCount > m_inputPoints.size() - 1) ui->spinBoxOutputImages->setValue(m_inputPoints.size() - 1);
    else ui->spinBoxOutputImages->setValue(lastLDAOutputImagesCount);
}

void LDADialog::doLDAClicked()
{
    SingleChannelImage *image = m_inputImages[0];
    int width = image->width(), height = image->height();
    unsigned int size, lastSize = width * height;
    unsigned int ncols = m_inputImages.size();
    m_data.resize(lastSize, ncols);
    bool apply_display = ui->checkBoxApplyDisplay->isChecked();
    if (m_buffersToDelete.size() > 0) for (int i = 0; i < m_buffersToDelete.size(); i++) { delete m_buffersToDelete[i]; m_buffersToDelete.clear(); }
    if (apply_display == false)
    {
        for (int i = 0; i < m_inputImages.size(); i++)
        {
            image = m_inputImages[i];
            size = image->width() * image->height();
            if (lastSize != size) { QMessageBox::warning(this, "Warning", QString("Could not complete LDA.\nAll images must be the same size")); return; }
            m_data.col(i) = Eigen::Map<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>>(image->data(), lastSize, 1);
        }
    }
    else
    {
        for (int i = 0; i < m_inputImages.size(); i++)
        {
            image = m_inputImages[i];
            size = image->width() * image->height();
            if (lastSize != size) { QMessageBox::warning(this, "Warning", QString("Could not complete LDA.\nAll images must be the same size")); return; }
            float *p = image->getDisplayMappedDataCopy();
            m_data.col(i) = Eigen::Map<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>>(p, lastSize, 1);
            m_buffersToDelete.push_back(p);
        }
    }
    unsigned int nrows = m_totalInputPoints;
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> x(nrows, ncols);
    Eigen::VectorXi labels(nrows);
    int currentPoint = 0;
    for (int i = 0; i < m_inputPoints.size(); i++)
    {
        LabelledPoints *points = m_inputPoints[i];
        for (int j = 0; j < points->points()->size(); j++)
        {
#if DEBUG_LDA_DIALOG
            qDebug() << i;
#endif
            for (int k = 0; k < m_inputImages.size(); k++)
            {
                QPointF point = points->points()->at(j);
                x(currentPoint, k) = m_data(int(point.x() +0.5) + (height - int(point.y() + 0.5)) * width, k);
#if DEBUG_LDA_DIALOG
                qDebug() << " " << x(currentPoint, k);
#endif
            }
            labels(currentPoint) = i;
            currentPoint++;
#if DEBUG_LDA_DIALOG
            qDebug() << "\n";
#endif
        }
    }
    m_lda.setIsCenter(ui->checkBoxMeanCenter->isChecked());
    m_lda.setIsScale(ui->checkBoxNormaliseVariance->isChecked());
    m_lda.Calculate(x, labels);

    QApplication::setOverrideCursor(Qt::WaitCursor);
    ui->progressBar->show();
    QApplication::processEvents();

    QFuture<int> future = QtConcurrent::run(&LDA::Extend, &m_lda, m_data);
    m_watcher.setFuture(future);
    foreach(QWidget *w, findChildren<QWidget *>()) w->setEnabled(false);
    ui->progressBar->setEnabled(true);
}

void LDADialog::ldaFinished()
{
    const Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> *y = m_lda.extendedScores();
    const float *dataPtr = y->data();
    int width = m_inputImages[0]->width();
    int height = m_inputImages[0]->height();
    int size = width * height;
    for (int i = 0; i < ui->spinBoxOutputImages->value(); i++)
    {
        SingleChannelImage *image = new SingleChannelImage();
        image->AllocateMemory(width, height, false);
        std::copy(dataPtr + i * size, dataPtr + (i + 1) * size, image->data());
        image->setNumBins(Settings::value("Number of Histogram Bins", int(32)).toInt());
        image->UpdateHistogram();
        QString name = QString("LD%1").arg(i, 3, 10, QChar('0'));
        image->setName(name);
        QString filename = QString("LDA_Output_Image_%1.tif").arg(i, 3, 10, QChar('0'));
        QDir outputFolder(ui->lineEditOutputFolder->text());
        QString localPath = QDir::cleanPath(outputFolder.absoluteFilePath(filename));
        image->setLocalPath(localPath);
        if (!outputFolder.exists()) { outputFolder.mkpath("."); }
        QFileInfo outputFolderInfo(ui->lineEditOutputFolder->text());
        if (outputFolderInfo.isDir() == false)
        {
            QMessageBox::warning(this, "Warning", QString("Could not create output folder: %1").arg(QDir::cleanPath(outputFolder.absolutePath())));
        }
        if (image->SaveImageToTiffFile(localPath) == false)
        {
            QMessageBox::warning(this, "Warning", QString("Could not save image: %1").arg(filename));
        }
        m_ouputImages.append(image);
    }
    for (int i = 0; i < m_buffersToDelete.size(); i++) delete [] m_buffersToDelete[i];
    m_buffersToDelete.clear();

    // scatterPlot();

    StoreSettings();
    accept();

    QApplication::restoreOverrideCursor();
    ui->progressBar->hide();
    QApplication::processEvents();
}

void LDADialog::StoreSettings()
{
    Settings::setValue("_LDAOutputImagesCount", ui->spinBoxOutputImages->value());
    Settings::setValue("_LDAOutputFolder", ui->lineEditOutputFolder->text());
    Settings::setValue("_LDAMeanCentre", ui->checkBoxMeanCenter->isChecked());
    Settings::setValue("_LDANormaliseVariance", ui->checkBoxNormaliseVariance->isChecked());
    Settings::setValue("_LDAApplyDisplay", ui->checkBoxApplyDisplay->isChecked());
    Settings::setValue("_LDADialogGeometry", saveGeometry());
    Settings::setValue("_LDADialogGeometry", saveGeometry());
}

void LDADialog::LoadSettings()
{
    ui->spinBoxOutputImages->setValue(Settings::value("_LDAOutputImagesCount", 0).toInt());
    ui->lineEditOutputFolder->setText(Settings::value("_LDAOutputFolder", QString()).toString());
    ui->checkBoxMeanCenter->setChecked(Settings::value("_LDAMeanCentre", true).toBool());
    ui->checkBoxNormaliseVariance->setChecked(Settings::value("_LDANormaliseVariance", false).toBool());
    ui->checkBoxApplyDisplay->setChecked(Settings::value("_LDAApplyDisplay", false).toBool());

}

QList<SingleChannelImage *> LDADialog::ouputImages() const
{
    return m_ouputImages;
}

void LDADialog::closeEvent(QCloseEvent *event)
{
    StoreSettings();
    event->accept();
}

void LDADialog::scatterPlot()
{
    ScatterPlotDialog scatterPlotDialog(this);

    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> *scores = m_lda.scores();
    int row = 0;
    QVector<float> x(m_totalInputPoints), y(m_totalInputPoints);
    for (int  i = 0; i < m_inputPoints.size(); i++)
    {
        int n = m_inputPoints[i]->points()->size();
        for (int j = 0; j < n; j ++)
        {
            x[j] = (scores->coeff(row + j, 0));
            y[j] = (scores->coeff(row + j, 1));
        }
        scatterPlotDialog.addPoints(x.constData(), y.constData(), n, i == m_inputPoints.size() - 1);
        row += n;
    }

    int status = scatterPlotDialog.exec();
    if (status == QDialog::Accepted)
    {
#if DEBUG_LDA_DIALOG
        qDebug("Scatter Plot OK");
#endif
    }
}

QString LDADialog::outputFolder() const
{
    return ui->lineEditOutputFolder->text();
}



