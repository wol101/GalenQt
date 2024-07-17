#include <algorithm>

#include <QMessageBox>
#include <QFileDialog>
#include "Settings.h"
#include <QListWidget>
#include <QListWidgetItem>
#include <QCloseEvent>
#include <QDir>
#include <QProgressBar>
#include <QSizePolicy>
#include <QVector>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>

#include <Eigen/Dense>

#include "PCA.h"
#include "SingleChannelImage.h"

#include "PCADialog.h"
#include "ui_PCADialog.h"

PCADialog::PCADialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PCADialog)
{
#ifdef Q_OS_MACOS
    // on MacOS High Sierra (and possibly others), the Qt::Dialog flag disables resizing
    // if I clear the Qt::Dialog flag and make sure the Qt::Window flag is set (it should be already)
    // then it seems to let me have resizing without any other side effects
    setWindowFlags(windowFlags() & (~Qt::Dialog) | Qt::Window);
#endif
    ui->setupUi(this);

    QSizePolicy spRetain = ui->progressBar->sizePolicy();
    spRetain.setRetainSizeWhenHidden(true);
    ui->progressBar->setSizePolicy(spRetain);
    ui->progressBar->hide();

    connect(ui->pushButtonDoPCA, SIGNAL(clicked()), this, SLOT(doPCAClicked()));
    connect(ui->pushButtonCancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(ui->pushButtonOutputFolder, SIGNAL(clicked()), this, SLOT(outputFolderClicked()));
    connect(ui->lineEditOutputFolder, SIGNAL(textChanged(const QString &)), this, SLOT(outputFolderChanged(const QString &)));
    connect(&m_watcher, SIGNAL(finished()), this, SLOT(pcaFinished()));

    LoadSettings();
    restoreGeometry(Settings::value("_PCADialogGeometry", QByteArray()).toByteArray());
}

PCADialog::~PCADialog()
{
    if (m_buffersToDelete.size() > 0) for (int i = 0; i < m_buffersToDelete.size(); i++) { delete m_buffersToDelete[i]; m_buffersToDelete.clear(); }
    delete ui;
}

void PCADialog::cancelClicked()
{
    StoreSettings();
    reject();
}

void PCADialog::outputFolderClicked()
{
    QString lastPCAOutputFolder = Settings::value("_PCAOutputFolder", QString("")).toString();
    QString dir = QFileDialog::getExistingDirectory(this, "Select PCA output folder", lastPCAOutputFolder, QFileDialog::ShowDirsOnly | QFileDialog::Options(EXTRA_FILE_DIALOG_OPTIONS));
    if (!dir.isEmpty())
    {
        ui->lineEditOutputFolder->setText(dir);
    }
}

void PCADialog::outputFolderChanged(const QString &text)
{
    Q_UNUSED(text);
    if (ui->lineEditOutputFolder->text().isEmpty()) ui->pushButtonDoPCA->setEnabled(false);
    else ui->pushButtonDoPCA->setEnabled(true);
}


void PCADialog::setInputImages(const QList<SingleChannelImage *> &inputImages)
{
    m_inputImages = inputImages;
    ui->listWidgetImages->clear();
    for (int i = 0; i < m_inputImages.size(); i++)
    {
        QListWidgetItem *item = new QListWidgetItem(m_inputImages[i]->localPath(), ui->listWidgetImages);
        Q_UNUSED(item);
    }
    ui->spinBoxOutputImages->setMinimum(1);
    ui->spinBoxOutputImages->setMaximum(m_inputImages.size());
    int lastPCAOutputImagesCount = Settings::value("_PCAOutputImagesCount", 0).toInt();
    if (lastPCAOutputImagesCount < 1 || lastPCAOutputImagesCount > m_inputImages.size()) ui->spinBoxOutputImages->setValue(m_inputImages.size());
    else ui->spinBoxOutputImages->setValue(lastPCAOutputImagesCount);
}

void PCADialog::doPCAClicked()
{
    unsigned int size;
    SingleChannelImage *image = m_inputImages[0];
    int width = image->width();
    int height = image->height();
    unsigned int lastSize = width * height;
    unsigned int nrows = lastSize;
    unsigned int ncols = m_inputImages.size();
    m_data.resize(nrows, ncols);
    bool apply_display = ui->checkBoxApplyDisplay->isChecked();
    if (m_buffersToDelete.size() > 0) for (int i = 0; i < m_buffersToDelete.size(); i++) { delete m_buffersToDelete[i]; m_buffersToDelete.clear(); }
    if (apply_display == false)
    {
        for (int i = 0; i < m_inputImages.size(); i++)
        {
            image = m_inputImages[i];
            size = image->width() * image->height();
            if (lastSize != size) { QMessageBox::warning(this, "Warning", QString("Could not complete PCA.\nAll images must be the same size")); return; }
            m_data.col(i) = Eigen::Map<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>>(image->data(), nrows, 1);
        }
    }
    else
    {
        for (int i = 0; i < m_inputImages.size(); i++)
        {
            image = m_inputImages[i];
            size = image->width() * image->height();
            if (lastSize != size) { QMessageBox::warning(this, "Warning", QString("Could not complete PCA.\nAll images must be the same size")); return; }
            float *p = image->getDisplayMappedDataCopy();
            m_data.col(i) = Eigen::Map<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>>(p, nrows, 1);
            m_buffersToDelete.push_back(p);
        }
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    ui->progressBar->show();
    QApplication::processEvents();

    m_pca.setIsCorr(ui->checkBoxCorrelationMatrix->isChecked());
    m_pca.setIsCenter(ui->checkBoxMeanCenter->isChecked());
    m_pca.setIsScale(ui->checkBoxNormaliseVariance->isChecked());
    QFuture<int> future = QtConcurrent::run(&PCA::Calculate, &m_pca, m_data);
    m_watcher.setFuture(future);
    foreach(QWidget *w, findChildren<QWidget *>()) w->setEnabled(false);
    ui->progressBar->setEnabled(true);
}


void PCADialog::pcaFinished()
{
    const Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> *y = m_pca.scores();
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
        QString name = QString("PC%1").arg(i, 3, 10, QChar('0'));
        image->setName(name);
        QString filename = QString("PCA_Output_Image_%1.tif").arg(i, 3, 10, QChar('0'));
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

    StoreSettings();
    accept();

    QApplication::restoreOverrideCursor();
    ui->progressBar->hide();
    QApplication::processEvents();
}

void PCADialog::StoreSettings()
{
    Settings::setValue("_PCAOutputImagesCount", ui->spinBoxOutputImages->value());
    Settings::setValue("_PCAOutputFolder", ui->lineEditOutputFolder->text());
    Settings::setValue("_PCAMeanCentre", ui->checkBoxMeanCenter->isChecked());
    Settings::setValue("_PCANormaliseVariance", ui->checkBoxNormaliseVariance->isChecked());
    Settings::setValue("_PCAUseCorrelationMatrix", ui->checkBoxCorrelationMatrix->isChecked());
    Settings::setValue("_PCAApplyDisplay", ui->checkBoxApplyDisplay->isChecked());
    Settings::setValue("_PCADialogGeometry", saveGeometry());
}

void PCADialog::LoadSettings()
{
    ui->spinBoxOutputImages->setValue(Settings::value("_PCAOutputImagesCount", 0).toInt());
    ui->lineEditOutputFolder->setText(Settings::value("_PCAOutputFolder", QString()).toString());
    ui->checkBoxMeanCenter->setChecked(Settings::value("_PCAMeanCentre", true).toBool());
    ui->checkBoxNormaliseVariance->setChecked(Settings::value("_PCANormaliseVariance", false).toBool());
    ui->checkBoxCorrelationMatrix->setChecked(Settings::value("_PCAUseCorrelationMatrix", false).toBool());
    ui->checkBoxApplyDisplay->setChecked(Settings::value("_PCAApplyDisplay", false).toBool());
}

QList<SingleChannelImage *> PCADialog::ouputImages() const
{
    return m_ouputImages;
}

void PCADialog::closeEvent(QCloseEvent *event)
{
    StoreSettings();
    event->accept();
}

QString PCADialog::outputFolder() const
{
    return ui->lineEditOutputFolder->text();
}

