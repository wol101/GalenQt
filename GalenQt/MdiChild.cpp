/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>
#include <QPointF>
#include <QString>
#include <QStringList>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QImage>
#include <QGraphicsScene>
#include <QPixmap>
#include <QPointF>
#include <QCursor>
#include <QProgressDialog>
#include <QBoxLayout>
#include <QHeaderView>
#include <QTreeWidget>
#include <QTableWidgetItem>
#include <QScrollArea>
#include <QResizeEvent>
#include <QTimer>
#include <QMenu>
#include <QMessageBox>
#include <QColorDialog>
#include <QSignalBlocker>

#include <math.h>
#include <cfloat>
#include <cstdint>
#include <algorithm>

#include "MdiChild.h"
#include "ui_MdiChild.h"
#include "GraphicsView.h"
#include "MainWindow.h"
#include "PreferencesDialog.h"
#include "MultiSpectralDocument.h"
#include "SingleChannelImage.h"
#include "HistogramDisplayWidget.h"
#include "ImageTreeWidgetItem.h"
#include "CustomScroller.h"
#include "RecipesDialog.h"
#include "LabelledPointsTreeWidgetItem.h"
#include "LabelledPoints.h"
#include "PCADialog.h"
#include "LDADialog.h"
#include "Settings.h"
#include "HDF5ReaderDialog.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define SQUARE(a) ((a) * (a))

#define IMAGE_TREE_COLUMN_RED 1
#define IMAGE_TREE_COLUMN_GREEN 2
#define IMAGE_TREE_COLUMN_BLUE 3
#define IMAGE_TREE_COLUMN_SELECTED 4
#define IMAGE_TREE_COLUMN_RED 1
#define POINTS_TREE_DISPLAY 1
#define POINTS_TREE_CURRENT 2
#define POINTS_TREE_SELECTED 3

MdiChild::MdiChild(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MdiChild)
{
    ui->setupUi(this);

    setWindowIcon(QIcon(":/Icon.iconset/icon_512x512@2x.png"));
    setAttribute(Qt::WA_DeleteOnClose);
    m_isUntitled = true;
    m_isModified = false;

    m_graphicsView = 0;
    m_document = 0;
    m_activeLabelledPointItem = 0;

    m_currentZoomLevel = Settings::value("_currentZoomLevel", 1.0f).toFloat();;
    m_zoomMultiplier = Settings::value("Zoom Modifier", 2.0f).toFloat();
    m_currentRotation = Settings::value("_currentRotation", 0.0f).toFloat();
    ui->doubleSpinBoxGamma->setMinimum(std::pow(10.0, -Settings::value("Gamma Range (log units)", 1).toInt()));
    ui->doubleSpinBoxGamma->setMaximum(std::pow(10.0, Settings::value("Gamma Range (log units)", 1).toInt()));

    m_defaultImportTreeItemName = "Images";
    m_defaultLabelledSetTreeItemName = "Labelled Points";
    m_defaultLabelledPointsName = "Class";
    m_defaultImportTreeItemNameCount = 0;
    m_defaultLabelledSetTreeItemNameCount = 0;
    m_defaultLabelledPointsNameCount = 0;
    m_numSelectedImages = 0;
    m_numSelectedPoints = 0;

    // set the default behaviour for spin boxes to keyboardTracking = false
    QList<QAbstractSpinBox *> spinBoxes = findChildren<QAbstractSpinBox *>();
    for (int i = 0; i < spinBoxes.size(); i++) spinBoxes[i]->setKeyboardTracking(false);

#ifdef TIGHTER_LAYOUT
    // opt for a tighter layout
    QList<QLayout *> layouts = findChildren<QLayout *>();
    int spacing = 6;
    for (int i = 0; i < layouts.size(); i++)
    {
        layouts[i]->setSpacing(spacing); // 0 is minimum
        layouts[i]->setMargin(spacing); // 0 is minimum
        layouts[i]->setContentsMargins(spacing,spacing,spacing,spacing); // 0,0,0,0 is minimum
    }
#endif

    m_graphicsView = new GraphicsView();
    m_graphicsView->setCursor(Qt::CrossCursor);
    m_graphicsView->setZoom(m_currentZoomLevel);
    m_graphicsView->setRotationAngle(m_currentRotation);

    m_customScroller = new CustomScroller();
    m_customScroller->setGraphicsView(m_graphicsView);

    QBoxLayout *boxLayoutGraphicsView = new QBoxLayout(QBoxLayout::LeftToRight, ui->widgetGraphicsViewPlaceholder);
    boxLayoutGraphicsView->setMargin(0);
    boxLayoutGraphicsView->addWidget(m_customScroller);

    m_histogramDisplay = new HistogramDisplayWidget();
    QBoxLayout *boxLayoutHistogramDisplay = new QBoxLayout(QBoxLayout::LeftToRight, ui->widgetHistogramPlaceholder);
    boxLayoutHistogramDisplay->setMargin(0);
    boxLayoutHistogramDisplay->addWidget(m_histogramDisplay);

    connect(ui->treeWidgetImageSet, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(treeWidgetClicked(QTreeWidgetItem *, int)));
    connect(ui->treeWidgetImageSet, SIGNAL(itemSelectionChanged()), this, SLOT(treeWidgetSelected()));
    connect(ui->treeWidgetImageSet, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(treeWidgetDoubleClicked(QTreeWidgetItem *, int)));
    connect(ui->treeWidgetImageSet, SIGNAL(itemChanged(QTreeWidgetItem *, int)), this, SLOT(treeWidgetItemChanged(QTreeWidgetItem *, int)), Qt::QueuedConnection);
    connect(ui->treeWidgetLabelledPointSet, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(treeWidgetDoubleClickedLabelledPointSet(QTreeWidgetItem *, int)));
    connect(ui->treeWidgetLabelledPointSet, SIGNAL(itemChanged(QTreeWidgetItem *, int)), this, SLOT(treeWidgetitemChangedLabelledPointSet(QTreeWidgetItem *, int)), Qt::QueuedConnection);
    connect(ui->treeWidgetImageSet->header(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(treeWidgetHeaderDoubleClicked(int)));
    connect(ui->treeWidgetLabelledPointSet->header(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(treeWidgetHeaderDoubleClickedLabelledPointSet(int)));

    // these produce the context menu for the labelled points
    ui->treeWidgetImageSet->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeWidgetImageSet, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(menuRequestImageSet(const QPoint &)));

    connect(ui->treeWidgetLabelledPointSet, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(treeWidgetClickedLabelledPointSet(QTreeWidgetItem *, int)));

    // these produce the context menu for the labelled points
    ui->treeWidgetLabelledPointSet->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeWidgetLabelledPointSet, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(menuRequestLabelledPointSet(const QPoint &)));


    // these produce the context menu for the histogram
    m_histogramDisplay->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_histogramDisplay, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(menuRequestHistogram(const QPoint &)));

    // handle signals from the GraphicsView
    connect(m_graphicsView, SIGNAL(newLabelledPoint(float,float)), this, SLOT(createNewLabelledPoint(float,float)));
    connect(m_graphicsView, SIGNAL(deleteLabelledPoint(float,float)), this, SLOT(deleteCurrentLabelledPoint(float,float)));
    connect(m_graphicsView, SIGNAL(emitZoom(float)), this, SLOT(zoomToValue(float)));
    connect(m_graphicsView, SIGNAL(statusString(QString)), this, SLOT(updateStatus(QString)));
    connect(m_graphicsView, SIGNAL(emitDrawLog(bool)), this, SLOT(drawLog(bool)));
    connect(m_graphicsView, SIGNAL(imageDimensionsChanged()), m_customScroller, SLOT(contentsResized()));
    connect(m_graphicsView, SIGNAL(newCentre(float,float)), m_customScroller, SLOT(scrollToCentre(float,float)));

//    // these update the display when the various display parameters are changed
//    connect(ui->doubleSpinBoxGamma, SIGNAL(valueChanged(double)), this, SLOT(displayValueChanged(double)), Qt::QueuedConnection);
//    connect(ui->doubleSpinBoxZebra, SIGNAL(valueChanged(double)), this, SLOT(displayValueChanged(double)), Qt::QueuedConnection);
//    connect(ui->doubleSpinBoxMin, SIGNAL(valueChanged(double)), this, SLOT(displayValueChanged(double)), Qt::QueuedConnection);
//    connect(ui->doubleSpinBoxMax, SIGNAL(valueChanged(double)), this, SLOT(displayValueChanged(double)), Qt::QueuedConnection);

//    // and these update the sliders
//    connect(ui->verticalSliderMinimum, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)), Qt::QueuedConnection);
//    connect(ui->verticalSliderMaximum, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)), Qt::QueuedConnection);
//    connect(ui->verticalSliderGamma, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)), Qt::QueuedConnection);

    // these update the display when the various display parameters are changed
    connect(ui->doubleSpinBoxGamma, SIGNAL(valueChanged(double)), this, SLOT(displayValueChanged(double)));
    connect(ui->doubleSpinBoxZebra, SIGNAL(valueChanged(double)), this, SLOT(displayValueChanged(double)));
    connect(ui->doubleSpinBoxMin, SIGNAL(valueChanged(double)), this, SLOT(displayValueChanged(double)));
    connect(ui->doubleSpinBoxMax, SIGNAL(valueChanged(double)), this, SLOT(displayValueChanged(double)));

    // and these update the sliders
    connect(ui->verticalSliderMinimum, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));
    connect(ui->verticalSliderMaximum, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));
    connect(ui->verticalSliderGamma, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));

    treeWidgetSelected();
}

MdiChild::~MdiChild()
{
    if (m_graphicsView) delete m_graphicsView;
    if (m_document) delete m_document;
    Settings::sync();
    delete ui;
}

void MdiChild::newFile()
{
    static int sequenceNumber = 1;
    m_isUntitled = true;
    m_isModified = false;
    QDir currentFolder = QFileInfo(Settings::value("_lastOpenedMultiSpectralDocument", QString()).toString()).absoluteDir();
    m_currentFile = currentFolder.absoluteFilePath(QString("document%1.xml").arg(sequenceNumber++));
    m_document = new MultiSpectralDocument();
    m_document->setImagesRoot(ui->treeWidgetImageSet->invisibleRootItem());
    m_document->setLabelledPointsRoot(ui->treeWidgetLabelledPointSet->invisibleRootItem());
    setWindowTitle(m_currentFile);
}

bool MdiChild::loadFile(const QString &fileName)
{
    m_currentFile = QDir::cleanPath(QFileInfo(fileName).absoluteFilePath());
    m_isUntitled = false;
    m_isModified = false;
    Settings::setValue("_lastOpenedMultiSpectralDocument", m_currentFile);
    setWindowModified(false);
    setWindowTitle(m_currentFile);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QProgressDialog progressDialog(QString("Opening Multispectral Project %1").arg(fileName), QString(), 0, 0, this);
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.show();
    QApplication::processEvents();

    if (m_document) delete m_document;
    m_document = new MultiSpectralDocument();
    m_document->setImagesRoot(ui->treeWidgetImageSet->invisibleRootItem());
    m_document->setLabelledPointsRoot(ui->treeWidgetLabelledPointSet->invisibleRootItem());
    if (m_document->Read(m_currentFile) == false)
    {
        QMessageBox::warning(this, "Warning", QString("Error reading %1").arg(m_currentFile));
        delete m_document;
        m_document = 0;
        return false;
    }

    for (QTreeWidgetItemIterator it = QTreeWidgetItemIterator(ui->treeWidgetImageSet->invisibleRootItem()); *it; it++) (*it)->setExpanded(true);
    for (QTreeWidgetItemIterator it = QTreeWidgetItemIterator(ui->treeWidgetLabelledPointSet->invisibleRootItem()); *it; it++) (*it)->setExpanded(true);
    for (int i = 0; i < ui->treeWidgetImageSet->columnCount(); i++) ui->treeWidgetImageSet->resizeColumnToContents(i);
    for (int i = 0; i < ui->treeWidgetLabelledPointSet->columnCount(); i++) ui->treeWidgetLabelledPointSet->resizeColumnToContents(i);

    progressDialog.hide();
    QApplication::processEvents();
    QApplication::restoreOverrideCursor();

    treeWidgetClicked(ui->treeWidgetImageSet->invisibleRootItem(), 0);
    treeWidgetClickedLabelledPointSet(ui->treeWidgetLabelledPointSet->invisibleRootItem(), 0);

    // I don't generally want to start with the scrollers at 0,0
    // this option doesn't work
    m_customScroller->setScrollFractions(0.5, 0.5);
    // neither does this one
//    for (QTreeWidgetItemIterator it = QTreeWidgetItemIterator(ui->treeWidgetImageSet->invisibleRootItem()); *it; it++)
//    {
//        ImageTreeWidgetItem *imageItem = dynamic_cast<ImageTreeWidgetItem *>(*it);
//        if (imageItem && imageItem->image())
//        {
//            SingleChannelImage *image = imageItem->image();
//            m_customScroller->initialiseScrollBars(image->width() / 2, image->width(), image->height() / 2, image->height());
//            break;
//        }
//    }

    return true; // success
}

bool MdiChild::save()
{
    if (m_isUntitled)
    {
        return saveAs();
    }
    else
    {
        return saveFile(m_currentFile);
    }
}

bool MdiChild::saveAs()
{
    QString suggestedFile = m_currentFile;
    // qWarning("Suggested Path %s\n", qUtf8Printable(suggestedFile));
    QString fileName = QFileDialog::getSaveFileName(this, "Save multispectral project data to file", suggestedFile, "XML Files (*.xml)", 0,
                                                    static_cast<QFileDialog::Options>(EXTRA_FILE_DIALOG_OPTIONS));
    if (fileName.isEmpty())
        return false;

    return saveFile(fileName);
}

bool MdiChild::saveFile(const QString &fileName)
{
    for (QTreeWidgetItemIterator it = QTreeWidgetItemIterator(ui->treeWidgetImageSet->invisibleRootItem()); *it; it++)
    {
        ImageTreeWidgetItem *item = dynamic_cast<ImageTreeWidgetItem *>((*it));
        if (item && item->image())
        {
            item->image()->setDisplayRed(item->checkState(IMAGE_TREE_COLUMN_RED) == Qt::Checked);
            item->image()->setDisplayGreen(item->checkState(IMAGE_TREE_COLUMN_GREEN) == Qt::Checked);
            item->image()->setDisplayBlue(item->checkState(IMAGE_TREE_COLUMN_BLUE) == Qt::Checked);
            item->image()->setSelected(item->checkState(IMAGE_TREE_COLUMN_SELECTED) == Qt::Checked);
        }
    }
    for (QTreeWidgetItemIterator it = QTreeWidgetItemIterator(ui->treeWidgetLabelledPointSet->invisibleRootItem()); *it; it++)
    {
        LabelledPointsTreeWidgetItem *item = dynamic_cast<LabelledPointsTreeWidgetItem *>((*it));
        if (item && item->labelledPoints())
        {
            item->labelledPoints()->setDisplay(item->checkState(POINTS_TREE_DISPLAY) == Qt::Checked);
            item->labelledPoints()->setCurrent(item->checkState(POINTS_TREE_CURRENT) == Qt::Checked);
            item->labelledPoints()->setSelected(item->checkState(POINTS_TREE_SELECTED) == Qt::Checked);
        }
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_document->Write(fileName);
    QApplication::restoreOverrideCursor();
    m_currentFile = QDir::cleanPath(QFileInfo(fileName).absoluteFilePath());
    m_isUntitled = false;
    m_isModified = false;
    Settings::setValue("_lastOpenedMultiSpectralDocument", m_currentFile);
    setWindowModified(false);
    setWindowTitle(userFriendlyCurrentFile() + "[*]");
    return true;
}

void MdiChild::closeEvent(QCloseEvent *event)
{
    if (maybeSave())
    {
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void MdiChild::documentWasModified()
{
    setWindowModified(m_isModified);
}

bool MdiChild::maybeSave()
{
    if (m_isModified)
    {
    QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this, tr("MDI"), tr("'%1' has been modified.\nDo you want to save your changes?").arg(userFriendlyCurrentFile()),
                                   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (ret == QMessageBox::Save)
            return save();
        else if (ret == QMessageBox::Cancel)
            return false;
    }
    return true;
}

QString MdiChild::userFriendlyCurrentFile()
{
    return QFileInfo(m_currentFile).fileName();
}

void MdiChild::zoomIn()
{
    m_currentZoomLevel *= m_zoomMultiplier;
    m_graphicsView->setZoom(m_currentZoomLevel);
    m_graphicsView->update();
    m_customScroller->contentsResized();
    Settings::setValue("_currentZoomLevel", m_currentZoomLevel);
}

void MdiChild::zoomOut()
{
    m_currentZoomLevel /= m_zoomMultiplier;
    m_graphicsView->setZoom(m_currentZoomLevel);
    m_graphicsView->update();
    m_customScroller->contentsResized();
    Settings::setValue("_currentZoomLevel", m_currentZoomLevel);
}

void MdiChild::rotateLeft()
{
    float quarterTurns = std::floor(0.5f + m_currentRotation / 90.0f);
    m_currentRotation = quarterTurns * 90;
    m_currentRotation += 90;
    m_currentRotation = std::fmod(m_currentRotation, 360.0f);
    m_graphicsView->setRotationAngle(m_currentRotation);
    m_graphicsView->update();
    m_customScroller->contentsResized();
    Settings::setValue("_currentRotation", m_currentRotation);
}

void MdiChild::rotateRight()
{
    float quarterTurns = std::floor(0.5f + m_currentRotation / 90.0f);
    m_currentRotation = quarterTurns * 90;
    m_currentRotation -= 90;
    m_currentRotation = std::fmod(m_currentRotation, 360.0f);
    m_graphicsView->setRotationAngle(m_currentRotation);
    m_graphicsView->update();
    m_customScroller->contentsResized();
    Settings::setValue("_currentRotation", m_currentRotation);
}

void MdiChild::nextChannel()
{
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidgetImageSet->selectedItems();
    if (selectedItems.size() == 0) goto select_the_first_image; // nothing selected so just pick the first image
    {
        ImageTreeWidgetItem *item = dynamic_cast<ImageTreeWidgetItem *>(selectedItems[0]);
        if (item == 0) goto select_the_first_image; // still nothing selected so just pick the first image

        // now get the next image
        { // new scope so that it is out of scope before the goto label
            QTreeWidgetItemIterator it = QTreeWidgetItemIterator(item);
            while (true)
            {
                it++;
                if (*it == 0) goto select_the_first_image; // got to the end of the list
                item = dynamic_cast<ImageTreeWidgetItem *>(*it);
                if (item == 0 || item->image() == 0) continue;
                activateImage(item);
                return;
            }
        }
    }

select_the_first_image:
    for (QTreeWidgetItemIterator it = QTreeWidgetItemIterator(ui->treeWidgetImageSet->invisibleRootItem()); *it; it++)
    {
        ImageTreeWidgetItem *item = dynamic_cast<ImageTreeWidgetItem *>(*it);
        if (item && item->image())
        {
            activateImage(item);
            break;
        }
    }
    return;
}

void MdiChild::prevChannel()
{
    // Qt does not have a reverse iterator for QTreeWidgetItem for we need to create a list of all items
    QList<QTreeWidgetItem *> allItems;
    for (QTreeWidgetItemIterator it = QTreeWidgetItemIterator(ui->treeWidgetImageSet->invisibleRootItem()); *it; it++) allItems.append(*it);
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidgetImageSet->selectedItems();
    if (selectedItems.size() == 0) goto select_the_last_image; // nothing selected so just pick the last image
    {
        ImageTreeWidgetItem *item = dynamic_cast<ImageTreeWidgetItem *>(selectedItems[0]);
        if (item == 0) goto select_the_last_image; // still nothing selected so just pick the last image

        // now get the previous image
        // first find where we are
        int i;
        for (i = 0; i < allItems.size(); i++)
            if (allItems[i] == item) break;
        if (i == allItems.size()) goto select_the_last_image; // couldn't find the item
        while (true)
        {
            i--;
            if (i <= 0) goto select_the_last_image; // got to the beginning of the list
            item = dynamic_cast<ImageTreeWidgetItem *>(allItems[i]);
            if (item == 0 || item->image() == 0) continue;
            activateImage(item);
            return;
        }
    }

select_the_last_image:
    for (int i = allItems.size() - 1; i >= 0; i--)
    {
        ImageTreeWidgetItem *item = dynamic_cast<ImageTreeWidgetItem *>(allItems[i]);
        if (item && item->image())
        {
            activateImage(item);
            break;
        }
    }
    return;
}

void MdiChild::activateImage(ImageTreeWidgetItem *item)
{
    Q_ASSERT(item->image());
    for (QTreeWidgetItemIterator it = QTreeWidgetItemIterator(ui->treeWidgetImageSet->invisibleRootItem()); *it; it++)
    {
        (*it)->setSelected(false);
        ImageTreeWidgetItem *itItem = dynamic_cast<ImageTreeWidgetItem *>(*it);
        if (itItem && itItem->image())
        {
            if (itItem == item)
            {
                SingleChannelImage *image = itItem->image();
                itItem->setCheckState(IMAGE_TREE_COLUMN_RED, Qt::Checked);
                itItem->setCheckState(IMAGE_TREE_COLUMN_GREEN, Qt::Checked);
                itItem->setCheckState(IMAGE_TREE_COLUMN_BLUE, Qt::Checked);
                m_graphicsView->setTexture(image->data(), image->width(), image->height(), GraphicsView::Red);
                m_graphicsView->setTextureDisplay(image->displayMin(), image->displayMax(), image->displayGamma(), image->displayZebra(), image->displayLogged(), GraphicsView::Red);
                m_graphicsView->setTexture(image->data(), image->width(), image->height(), GraphicsView::Green);
                m_graphicsView->setTextureDisplay(image->displayMin(), image->displayMax(), image->displayGamma(), image->displayZebra(), image->displayLogged(), GraphicsView::Green);
                m_graphicsView->setTexture(image->data(), image->width(), image->height(), GraphicsView::Blue);
                m_graphicsView->setTextureDisplay(image->displayMin(), image->displayMax(), image->displayGamma(), image->displayZebra(), image->displayLogged(), GraphicsView::Blue);
            }
            else
            {
                itItem->setCheckState(IMAGE_TREE_COLUMN_RED, Qt::Unchecked);
                itItem->setCheckState(IMAGE_TREE_COLUMN_GREEN, Qt::Unchecked);
                itItem->setCheckState(IMAGE_TREE_COLUMN_BLUE, Qt::Unchecked);
            }
        }
    }
    item->setSelected(true);
    m_graphicsView->setDrawRed(true);
    m_graphicsView->setDrawGreen(true);
    m_graphicsView->setDrawBlue(true);
    m_graphicsView->update();
}

void MdiChild::autoGamma()
{
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidgetImageSet->selectedItems();
    if (selectedItems.size() == 0) return;
    ImageTreeWidgetItem *item = dynamic_cast<ImageTreeWidgetItem *>(selectedItems[0]);
    if (item == 0 || item->image() == 0) return;
    float optimalGamma = item->image()->optimalGamma();
    ui->doubleSpinBoxGamma->setValue(optimalGamma);
}

void MdiChild::invertImage()
{
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidgetImageSet->selectedItems();
    if (selectedItems.size() == 0) return;
    ImageTreeWidgetItem *item = dynamic_cast<ImageTreeWidgetItem *>(selectedItems[0]);
    if (item == 0 || item->image() == 0) return;
    QSignalBlocker blocker1(ui->doubleSpinBoxMin); // this is done so the slot is only called after the doubleSpinBoxGamma. Only needed if using Qt::DirectConnection for connect but does no harm.
    QSignalBlocker blocker2(ui->doubleSpinBoxMax); // this is done so the slot is only called after the doubleSpinBoxGamma. Only needed if using Qt::DirectConnection for connect but does no harm.
    QSignalBlocker blocker3(ui->doubleSpinBoxGamma); // this is done so the slot is only called after the doubleSpinBoxGamma. Only needed if using Qt::DirectConnection for connect but does no harm.
    ui->doubleSpinBoxMin->setValue(item->image()->displayMax());
    ui->doubleSpinBoxMax->setValue(item->image()->displayMin());
    ui->doubleSpinBoxGamma->setValue(1.0f / item->image()->displayGamma());
    displayValueChanged(0); // called specifically because the signal is blocked
}

void MdiChild::resetDisplay()
{
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidgetImageSet->selectedItems();
    if (selectedItems.size() == 0) return;
    ImageTreeWidgetItem *item = dynamic_cast<ImageTreeWidgetItem *>(selectedItems[0]);
    if (item == 0 || item->image() == 0) return;
    QSignalBlocker blocker1(ui->doubleSpinBoxMin);  // this is done so the slot is only called once
    QSignalBlocker blocker2(ui->doubleSpinBoxMax);
    QSignalBlocker blocker3(ui->doubleSpinBoxGamma);
    QSignalBlocker blocker4(ui->doubleSpinBoxZebra);
    if (item->image()->displayLogged()) ui->doubleSpinBoxMin->setValue(item->image()->dataLogMin());
    else ui->doubleSpinBoxMin->setValue(item->image()->dataMin());
    ui->doubleSpinBoxMax->setValue(item->image()->dataMax());
    ui->doubleSpinBoxGamma->setValue(1.0);
    ui->doubleSpinBoxZebra->setValue(1.0);
    displayValueChanged(0); // called specifically because the signal is blocked
}

void MdiChild::performRecipes()
{
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidgetImageSet->selectedItems();
    QList<SingleChannelImage *> selectedImages;
    QList<LabelledPoints *> selectedLabelledPoints;
    for (int i = 0; i < selectedItems.size(); i++)
    {
        ImageTreeWidgetItem *imageItem = dynamic_cast<ImageTreeWidgetItem *>(selectedItems[i]);
        if (imageItem)
        {
            selectedImages.push_back(imageItem->image());
        }
        LabelledPointsTreeWidgetItem *labelledPointsItem = dynamic_cast<LabelledPointsTreeWidgetItem *>(selectedItems[i]);
        if (labelledPointsItem)
        {
            selectedLabelledPoints.push_back(labelledPointsItem->labelledPoints());
        }
    }

    RecipesDialog dialogRecipes(this);
    dialogRecipes.setInputImages(&selectedImages);
    dialogRecipes.setLabelledPoints(&selectedLabelledPoints);
    dialogRecipes.setOutputDimensions(selectedItems.size() / 2);

    int status = dialogRecipes.exec();
    if (status == QDialog::Accepted)
    {
        QStringList outputImages = dialogRecipes.outputImages();
        if (outputImages.size() == 0)
        {
            QMessageBox::warning(this, "Perform Recipes Error", "No images produced\n Click the button to return.", QMessageBox::Ok);
            return;
        }

        // create a top level folder
        QTreeWidgetItem *rootItem = ui->treeWidgetImageSet->invisibleRootItem();
        QStringList itemStrings;
        itemStrings << QFileInfo(outputImages[0]).absoluteDir().dirName() << "" << "" << "";
        QTreeWidgetItem *outputImageFolder = new ImageTreeWidgetItem(rootItem, itemStrings);

        // add the new images
        QString baseName;
        for (int i = 0; i < outputImages.size(); i++)
        {
            SingleChannelImage *imageRead;
            int numHistogramBins = Settings::value("Number of Histogram Bins", int(32)).toInt();
            if (SingleChannelImage::CreateSingleChannelImagesFromFile(outputImages[i], &imageRead, numHistogramBins) == false)
            {
                QMessageBox::warning(this, "Perform Recipes Error", QString("Error reading image %1\n Click the button to return.").arg(outputImages[i]), QMessageBox::Ok);
                return;
            }
            baseName = QFileInfo(outputImages[i]).completeBaseName();
            imageRead->setName(baseName);
//            m_document->AddImage(imageRead); FIX ME - this does not work
            itemStrings.clear();
            itemStrings << baseName << "" << "" << "" << "";
            ImageTreeWidgetItem *newImage = new ImageTreeWidgetItem(outputImageFolder, itemStrings, imageRead);
            newImage->setCheckState(IMAGE_TREE_COLUMN_RED, Qt::Unchecked);
            newImage->setCheckState(IMAGE_TREE_COLUMN_GREEN, Qt::Unchecked);
            newImage->setCheckState(IMAGE_TREE_COLUMN_BLUE, Qt::Unchecked);
            newImage->setCheckState(IMAGE_TREE_COLUMN_SELECTED, Qt::Unchecked);
        }
        outputImageFolder->setExpanded(true);
        for (int i = 0; i < ui->treeWidgetImageSet->columnCount(); i++) ui->treeWidgetImageSet->resizeColumnToContents(i);

    }

}

bool MdiChild::importImage(const QString &fileName)
{
    QFileInfo fileInfo(fileName);
    QString baseName = fileInfo.completeBaseName();
    SingleChannelImage *imageRead;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QProgressDialog progressDialog(QString("Importing Image %1").arg(fileName), QString(), 0, 0, this);
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.show();
    QApplication::processEvents();

    int numHistogramBins = Settings::value("Number of Histogram Bins", int(32)).toInt();
    if (SingleChannelImage::CreateSingleChannelImagesFromFile(fileName, &imageRead, numHistogramBins) == false)
    {
        QMessageBox::warning(this, "Warning", QString("Error reading image %2").arg(fileName));
        return false;
    }

    progressDialog.hide();
    QApplication::processEvents();
    QApplication::restoreOverrideCursor();

    // work out where to put the imported images
    QTreeWidgetItem *rootItem = ui->treeWidgetImageSet->invisibleRootItem();
    QTreeWidgetItem *importedImageParent = 0;
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidgetImageSet->selectedItems();
    if (selectedItems.size())
    {
        for (int i = 0; i < selectedItems.size(); i++)
        {
            ImageTreeWidgetItem *item = dynamic_cast<ImageTreeWidgetItem *>(selectedItems[i]);
            if (item == 0) continue;
            if (item->image())
            {
                importedImageParent = item->parent();
                break;
            }
            else
            {
                importedImageParent = item;
                break;
            }
        }
    }
    if (importedImageParent == 0)
    {
        for (int i = 0; i < rootItem->childCount(); i++)
        {
            QTreeWidgetItem *child = rootItem->child(i);
            QString name = child->data(0, Qt::DisplayRole).toString();
            if (name == m_defaultImportTreeItemName)
            {
                importedImageParent = child;
                break;
            }
        }
        if (importedImageParent == 0)
        {
            QStringList itemStrings;
            itemStrings << m_defaultImportTreeItemName << "" << "" << "" << "";
            importedImageParent = new ImageTreeWidgetItem(rootItem, itemStrings);
            importedImageParent->setData(1, Qt::CheckStateRole, QVariant());
            importedImageParent->setData(2, Qt::CheckStateRole, QVariant());
            importedImageParent->setData(3, Qt::CheckStateRole, QVariant());
            importedImageParent->setData(4, Qt::CheckStateRole, QVariant());
        }
    }

    imageRead->setName(baseName);
    QStringList itemStrings;
    itemStrings << baseName << "" << "" << "" << "";
    ImageTreeWidgetItem *newImage = new ImageTreeWidgetItem(importedImageParent, itemStrings, imageRead);
    newImage->setCheckState(IMAGE_TREE_COLUMN_RED, Qt::Unchecked);
    newImage->setCheckState(IMAGE_TREE_COLUMN_GREEN, Qt::Unchecked);
    newImage->setCheckState(IMAGE_TREE_COLUMN_BLUE, Qt::Unchecked);
    newImage->setCheckState(IMAGE_TREE_COLUMN_SELECTED, Qt::Unchecked);

    importedImageParent->setExpanded(true);
    for (int i = 0; i < ui->treeWidgetImageSet->columnCount(); i++) ui->treeWidgetImageSet->resizeColumnToContents(i);
    treeWidgetClicked(ui->treeWidgetImageSet->invisibleRootItem(), 0);
    m_customScroller->setScrollFractions(0.5, 0.5);

    m_isModified = true;
    Settings::setValue("_lastImportedImageFile",fileName);

    return true; // success
}

bool MdiChild::exportImage(const QString &fileName)
{
    bool status = true; // change to false on error
    // first find what is visible
    SingleChannelImage *redImage = 0;
    SingleChannelImage *greenImage = 0;
    SingleChannelImage *blueImage = 0;
    for (QTreeWidgetItemIterator it = QTreeWidgetItemIterator(ui->treeWidgetImageSet->invisibleRootItem()); *it; it++)
    {
        ImageTreeWidgetItem *imageItem = dynamic_cast<ImageTreeWidgetItem *>(*it);
        if (imageItem && imageItem->image())
        {
            SingleChannelImage *image = imageItem->image();
            if (imageItem->checkState(IMAGE_TREE_COLUMN_RED) == Qt::Checked) redImage = image;
            if (imageItem->checkState(IMAGE_TREE_COLUMN_GREEN) == Qt::Checked) greenImage = image;
            if (imageItem->checkState(IMAGE_TREE_COLUMN_BLUE) == Qt::Checked) blueImage = image;
        }
    }
    if (redImage == 0 && greenImage == 0 && blueImage == 0) { QMessageBox::warning(this, "Warning", QString("No image visible so could not export.")); return false; }
    status = SingleChannelImage::SaveAsColour8BitTiff(redImage, greenImage, blueImage, fileName);
    if (status == false) { QMessageBox::warning(this, "Warning", QString("Error exporting TIFF file %").arg(fileName)); }
    return status;
}

void MdiChild::sliderValueChanged(int value)
{
    Q_UNUSED(value);
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidgetImageSet->selectedItems();
    if (selectedItems.size() == 0) return;
    ImageTreeWidgetItem *item = dynamic_cast<ImageTreeWidgetItem *>(selectedItems[0]);
    if (item == 0) return;

    QSignalBlocker(ui->doubleSpinBoxMin);
    QSignalBlocker(ui->doubleSpinBoxMax);
    QSignalBlocker(ui->doubleSpinBoxGamma);
    // slider minima are set to zero
    float minSlider = float(ui->verticalSliderMinimum->value()) / float(ui->verticalSliderMinimum->maximum()); // 0 to 1
    float maxSlider = float(ui->verticalSliderMaximum->value()) / float(ui->verticalSliderMaximum->maximum()); // 0 to 1
    float minimum = minSlider * (ui->doubleSpinBoxMin->maximum() - ui->doubleSpinBoxMin->minimum()) + ui->doubleSpinBoxMin->minimum();
    float maximum = maxSlider * (ui->doubleSpinBoxMax->maximum() - ui->doubleSpinBoxMax->minimum()) + ui->doubleSpinBoxMax->minimum();
    ui->doubleSpinBoxMin->setValue(minSlider * (ui->doubleSpinBoxMin->maximum() - ui->doubleSpinBoxMin->minimum()) + ui->doubleSpinBoxMin->minimum());
    ui->doubleSpinBoxMax->setValue(maxSlider * (ui->doubleSpinBoxMax->maximum() - ui->doubleSpinBoxMax->minimum()) + ui->doubleSpinBoxMax->minimum());

    float gammaSlider = 2.0f * float(ui->verticalSliderGamma->value()) / float(ui->verticalSliderGamma->maximum()) - 1.0f; // -1 to 1
    int gammaRange = Settings::value("Gamma Range (log units)", 1).toInt();
    float gamma = std::pow(10.0f, gammaSlider * gammaRange);
    ui->doubleSpinBoxGamma->setValue(gamma);

    float zebra = ui->doubleSpinBoxZebra->value();

    if (item->checkState(IMAGE_TREE_COLUMN_RED) == Qt::Checked)
    {
        m_graphicsView->setTextureDisplay(minimum, maximum, gamma, zebra, item->image()->displayLogged(), GraphicsView::Red);
        m_graphicsView->update();
    }
    if (item->checkState(IMAGE_TREE_COLUMN_GREEN) == Qt::Checked)
    {
        m_graphicsView->setTextureDisplay(minimum, maximum, gamma, zebra, item->image()->displayLogged(), GraphicsView::Green);
        m_graphicsView->update();
    }
    if (item->checkState(IMAGE_TREE_COLUMN_BLUE) == Qt::Checked)
    {
        m_graphicsView->setTextureDisplay(minimum, maximum, gamma, zebra, item->image()->displayLogged(), GraphicsView::Blue);
        m_graphicsView->update();
    }
    m_histogramDisplay->update();

}

void MdiChild::displayValueChanged(double value)
{
    Q_UNUSED(value);
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidgetImageSet->selectedItems();
    if (selectedItems.size() == 0) return;
    ImageTreeWidgetItem *item = dynamic_cast<ImageTreeWidgetItem *>(selectedItems[0]);
    if (item == 0) return;
    SingleChannelImage *image = item->image();
    if (image == 0) return;
    m_isModified = true;
    float minimum = ui->doubleSpinBoxMin->value();
    float maximum = ui->doubleSpinBoxMax->value();
    float gamma = ui->doubleSpinBoxGamma->value();
    float zebra = ui->doubleSpinBoxZebra->value();
    if (std::fabs(maximum - minimum) < 1e-10f)
    {
        if (std::fabs(maximum > 1.0f)) maximum *= 1.00001f;
        else maximum += 0.00001f;
        ui->doubleSpinBoxMax->setValue(maximum);
    }
    // need to change the step increment depending on the log of the current value
    int gammaRange = Settings::value("Gamma Range (log units)", 1).toInt();
    float gammaScale = (std::log10(ui->doubleSpinBoxGamma->value()) + gammaRange) / (gammaRange * 2.0f); // 0 to 1 range
    float gammaStep1 = gammaScale - std::log10(Settings::value("Spin Box Gamma Step Resolution", 100).toInt());
    float gammaStep2 = std::pow(10, gammaStep1);
    ui->doubleSpinBoxGamma->setSingleStep(gammaStep2);
    // qDebug("gamma=%g gammaScale=%g gammaStep1=%g gammaStep2=%g", gamma, gammaScale, gammaStep1, gammaStep2);

    float dataMin;
    if (image->displayLogged()) dataMin = image->dataLogMin();
    else dataMin = image->dataMin();
    image->setDisplayGamma(gamma);
    image->setDisplayRange(minimum, maximum);
    image->setDisplayZebra(zebra);
    QSignalBlocker blocker1(ui->verticalSliderMinimum);
    QSignalBlocker blocker2(ui->verticalSliderMaximum);
    QSignalBlocker blocker3(ui->verticalSliderGamma);
    ui->verticalSliderMinimum->setValue(((image->displayMin() - dataMin) / (image->dataMax() - dataMin)) * ui->verticalSliderMinimum->maximum());
    ui->verticalSliderMaximum->setValue(((image->displayMax() - dataMin) / (image->dataMax() - dataMin)) * ui->verticalSliderMaximum->maximum());
    ui->verticalSliderGamma->setValue(gammaScale * ui->verticalSliderMaximum->maximum());

    if (item->checkState(IMAGE_TREE_COLUMN_RED) == Qt::Checked)
    {
        m_graphicsView->setTextureDisplay(minimum, maximum, gamma, zebra, image->displayLogged(), GraphicsView::Red);
        m_graphicsView->update();
    }
    if (item->checkState(IMAGE_TREE_COLUMN_GREEN) == Qt::Checked)
    {
        m_graphicsView->setTextureDisplay(minimum, maximum, gamma, zebra, image->displayLogged(), GraphicsView::Green);
        m_graphicsView->update();
    }
    if (item->checkState(IMAGE_TREE_COLUMN_BLUE) == Qt::Checked)
    {
        m_graphicsView->setTextureDisplay(minimum, maximum, gamma, zebra, image->displayLogged(), GraphicsView::Blue);
        m_graphicsView->update();
    }
    m_histogramDisplay->update();
}

void MdiChild::treeWidgetClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    m_numSelectedImages = 0;
    Qt::CheckState check1 = Qt::Unchecked;
    Qt::CheckState check2 = Qt::Unchecked;
    Qt::CheckState check3 = Qt::Unchecked;
    if (item)
    {
        check1 = item->checkState(IMAGE_TREE_COLUMN_RED);
        check2 = item->checkState(IMAGE_TREE_COLUMN_GREEN);
        check3 = item->checkState(IMAGE_TREE_COLUMN_BLUE);
    }
    bool drawRed = false;
    bool drawGreen = false;
    bool drawBlue = false;
    SingleChannelImage *image;
    for (QTreeWidgetItemIterator it = QTreeWidgetItemIterator(ui->treeWidgetImageSet->invisibleRootItem()); *it; it++)
    {
        ImageTreeWidgetItem *imageItem = dynamic_cast<ImageTreeWidgetItem *>(*it);
        if (imageItem && imageItem->image())
        {
            image = imageItem->image();
            if (imageItem != item)
            {
                if (check1 == Qt::Checked && imageItem->checkState(IMAGE_TREE_COLUMN_RED) == Qt::Checked) imageItem->setCheckState(IMAGE_TREE_COLUMN_RED, Qt::Unchecked);
                if (check2 == Qt::Checked && imageItem->checkState(IMAGE_TREE_COLUMN_GREEN) == Qt::Checked) imageItem->setCheckState(IMAGE_TREE_COLUMN_GREEN, Qt::Unchecked);
                if (check3 == Qt::Checked && imageItem->checkState(IMAGE_TREE_COLUMN_BLUE) == Qt::Checked) imageItem->setCheckState(IMAGE_TREE_COLUMN_BLUE, Qt::Unchecked);
            }
            if (imageItem->checkState(IMAGE_TREE_COLUMN_RED) == Qt::Checked)
            {
                m_graphicsView->setTexture(image->data(), image->width(), image->height(), GraphicsView::Red);
                m_graphicsView->setTextureDisplay(image->displayMin(), image->displayMax(), image->displayGamma(), image->displayZebra(), image->displayLogged(), GraphicsView::Red);
                drawRed = true;
            }
            if (imageItem->checkState(IMAGE_TREE_COLUMN_GREEN) == Qt::Checked)
            {
                m_graphicsView->setTexture(image->data(), image->width(), image->height(), GraphicsView::Green);
                m_graphicsView->setTextureDisplay(image->displayMin(), image->displayMax(), image->displayGamma(), image->displayZebra(), image->displayLogged(), GraphicsView::Green);
                drawGreen = true;
            }
            if (imageItem->checkState(IMAGE_TREE_COLUMN_BLUE) == Qt::Checked)
            {
                m_graphicsView->setTexture(image->data(), image->width(), image->height(), GraphicsView::Blue);
                m_graphicsView->setTextureDisplay(image->displayMin(), image->displayMax(), image->displayGamma(), image->displayZebra(), image->displayLogged(), GraphicsView::Blue);
                drawBlue = true;
            }
            if (imageItem->checkState(IMAGE_TREE_COLUMN_SELECTED) == Qt::Checked)
            {
                m_numSelectedImages++;
            }
        }
    }
    m_graphicsView->setDrawRed(drawRed);
    m_graphicsView->setDrawGreen(drawGreen);
    m_graphicsView->setDrawBlue(drawBlue);
    m_graphicsView->update();
}

void MdiChild::displayOneImageIfNecessary()
{
    bool drawRed = false;
    bool drawGreen = false;
    bool drawBlue = false;
    ImageTreeWidgetItem *firstItem = 0;
    SingleChannelImage *image;
    for (QTreeWidgetItemIterator it = QTreeWidgetItemIterator(ui->treeWidgetImageSet->invisibleRootItem()); *it; it++)
    {
        ImageTreeWidgetItem *imageItem = dynamic_cast<ImageTreeWidgetItem *>(*it);
        if (imageItem && imageItem->image())
        {
            image = imageItem->image();
            if (firstItem == 0) firstItem = imageItem;
            if (imageItem->checkState(IMAGE_TREE_COLUMN_RED) == Qt::Checked)
            {
                m_graphicsView->setTexture(image->data(), image->width(), image->height(), GraphicsView::Red);
                m_graphicsView->setTextureDisplay(image->displayMin(), image->displayMax(), image->displayGamma(), image->displayZebra(), image->displayLogged(), GraphicsView::Red);
                drawRed = true;
            }
            if (imageItem->checkState(IMAGE_TREE_COLUMN_GREEN) == Qt::Checked)
            {
                m_graphicsView->setTexture(image->data(), image->width(), image->height(), GraphicsView::Green);
                m_graphicsView->setTextureDisplay(image->displayMin(), image->displayMax(), image->displayGamma(), image->displayZebra(), image->displayLogged(), GraphicsView::Green);
                drawGreen = true;
            }
            if (imageItem->checkState(IMAGE_TREE_COLUMN_BLUE) == Qt::Checked)
            {
                m_graphicsView->setTexture(image->data(), image->width(), image->height(), GraphicsView::Blue);
                m_graphicsView->setTextureDisplay(image->displayMin(), image->displayMax(), image->displayGamma(), image->displayZebra(), image->displayLogged(), GraphicsView::Blue);
                drawBlue = true;
            }
        }
    }
    if (drawRed == false && drawGreen == false && drawBlue == false)
    {
        firstItem->setCheckState(IMAGE_TREE_COLUMN_RED, Qt::Checked);
        image = firstItem->image();
        m_graphicsView->setTexture(image->data(), image->width(), image->height(), GraphicsView::Red);
        m_graphicsView->setTextureDisplay(image->displayMin(), image->displayMax(), image->displayGamma(), image->displayZebra(), image->displayLogged(), GraphicsView::Red);
        drawRed = true;
    }

    m_graphicsView->setDrawRed(drawRed);
    m_graphicsView->setDrawGreen(drawGreen);
    m_graphicsView->setDrawBlue(drawBlue);
    m_graphicsView->update();
}


void MdiChild::treeWidgetClickedLabelledPointSet(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    m_numSelectedPoints = 0;
    Qt::CheckState check2 = Qt::Unchecked;
    if (item) check2 = item->checkState(POINTS_TREE_CURRENT);
    QList<LabelledPoints *> *points = m_graphicsView->labelledPoints();
    points->clear();
    m_activeLabelledPointItem = 0;
    for (QTreeWidgetItemIterator it = QTreeWidgetItemIterator(ui->treeWidgetLabelledPointSet->invisibleRootItem()); *it; it++)
    {
        LabelledPointsTreeWidgetItem *labelledPointsItem = dynamic_cast<LabelledPointsTreeWidgetItem *>(*it);
        if (labelledPointsItem && labelledPointsItem->labelledPoints())
        {
            if (labelledPointsItem != item)
            {
                if (check2 == Qt::Checked && labelledPointsItem->checkState(POINTS_TREE_CURRENT) == Qt::Checked) labelledPointsItem->setCheckState(POINTS_TREE_CURRENT, Qt::Unchecked);
            }
            if (labelledPointsItem->checkState(POINTS_TREE_DISPLAY) == Qt::Checked) points->append(labelledPointsItem->labelledPoints());
            if (labelledPointsItem->checkState(POINTS_TREE_CURRENT) == Qt::Checked) m_activeLabelledPointItem = labelledPointsItem;
            if (labelledPointsItem->checkState(POINTS_TREE_SELECTED) == Qt::Checked)
            {
                m_numSelectedPoints++;
            }
            labelledPointsItem->setBackgroundColor(0, labelledPointsItem->labelledPoints()->colour());
        }
    }
    m_graphicsView->update();
}

void MdiChild::changeEvent(QEvent* e)
{
    if( e->type() == QEvent::WindowStateChange )
    {
        QWindowStateChangeEvent* event = dynamic_cast< QWindowStateChangeEvent* >( e );

        if( event && event->oldState() & Qt::WindowMinimized )
        {
//            qDebug() << "changeEvent: Window restored (to normal or maximized state)!";
        }
        else if( event && event->oldState() == Qt::WindowNoState && this->windowState() == Qt::WindowMaximized )
        {
//            qDebug() << "changeEvent: Window Maximized!";
            int w = m_graphicsView->width();
            int h = m_graphicsView->height();
            QResizeEvent *resizeEvent = new QResizeEvent(QSize(w, h), QSize(-1,-1));
            QCoreApplication::postEvent(m_graphicsView, resizeEvent);
        }
    }
    QWidget::changeEvent(e);
}

void MdiChild::importHDF5()
{
    HDF5ReaderDialog hdfDialog(this);

    int status = hdfDialog.exec();
    if (status == QDialog::Accepted)
    {
        QList<SingleChannelImage *> images = hdfDialog.ouputImages();
        QTreeWidgetItem *newParent = doNewTreeWidgetImageSet(ui->treeWidgetImageSet->invisibleRootItem());
        for (int i = 0; i < images.size(); i++)
        {
            QStringList itemStrings;
            itemStrings << images[i]->name() << "" << "" << "" << "";
            ImageTreeWidgetItem *newImage = new ImageTreeWidgetItem(newParent, itemStrings, images[i]);
            newImage->setCheckState(IMAGE_TREE_COLUMN_RED, Qt::Unchecked);
            newImage->setCheckState(IMAGE_TREE_COLUMN_GREEN, Qt::Unchecked);
            newImage->setCheckState(IMAGE_TREE_COLUMN_BLUE, Qt::Unchecked);
            newImage->setCheckState(IMAGE_TREE_COLUMN_SELECTED, Qt::Unchecked);
        }
        newParent->setExpanded(true);
        for (int i = 0; i < ui->treeWidgetImageSet->columnCount(); i++) ui->treeWidgetImageSet->resizeColumnToContents(i);
        displayOneImageIfNecessary();
        m_isModified = true;
        emit emitStatus(tr("HDF import OK"));
    }
}

void MdiChild::pca()
{
    PCADialog pcaDialog(this);

    QList<SingleChannelImage *> inputImages;
    unsigned int size;
    unsigned int lastSize = 0;
    for (QTreeWidgetItemIterator it = QTreeWidgetItemIterator(ui->treeWidgetImageSet->invisibleRootItem()); *it; it++)
    {
        ImageTreeWidgetItem *item = dynamic_cast<ImageTreeWidgetItem *>((*it));
        if (item && item->image() && item->checkState(IMAGE_TREE_COLUMN_SELECTED))
        {
            size = item->image()->width() * item->image()->height();
            if (lastSize  == 0) lastSize = size;
            else if (lastSize != size) { QMessageBox::warning(this, "Warning", QString("Could not complete PCA.\nAll images must be the same size")); return; }
            inputImages.append(item->image());
        }
    }
    pcaDialog.setInputImages(inputImages);
    int status = pcaDialog.exec();
    if (status == QDialog::Accepted)
    {
        QList<SingleChannelImage *> images = pcaDialog.ouputImages();
        QTreeWidgetItem *newParent = doNewTreeWidgetImageSet(ui->treeWidgetImageSet->invisibleRootItem());
        for (int i = 0; i < images.size(); i++)
        {
            QStringList itemStrings;
            itemStrings << images[i]->name() << "" << "" << "" << "";
            ImageTreeWidgetItem *newImage = new ImageTreeWidgetItem(newParent, itemStrings, images[i]);
            newImage->setCheckState(IMAGE_TREE_COLUMN_RED, Qt::Unchecked);
            newImage->setCheckState(IMAGE_TREE_COLUMN_GREEN, Qt::Unchecked);
            newImage->setCheckState(IMAGE_TREE_COLUMN_BLUE, Qt::Unchecked);
            newImage->setCheckState(IMAGE_TREE_COLUMN_SELECTED, Qt::Unchecked);
        }
        newParent->setExpanded(true);
        for (int i = 0; i < ui->treeWidgetImageSet->columnCount(); i++) ui->treeWidgetImageSet->resizeColumnToContents(i);
        m_isModified = true;
        emit emitStatus(tr("PCA OK"));
    }
}

void MdiChild::lda()
{
    LDADialog ldaDialog(this);

    QList<SingleChannelImage *> inputImages;
    unsigned int size;
    unsigned int lastSize = 0;
    for (QTreeWidgetItemIterator it = QTreeWidgetItemIterator(ui->treeWidgetImageSet->invisibleRootItem()); *it; it++)
    {
        ImageTreeWidgetItem *item = dynamic_cast<ImageTreeWidgetItem *>((*it));
        if (item && item->image() && item->checkState(IMAGE_TREE_COLUMN_SELECTED))
        {
            size = item->image()->width() * item->image()->height();
            if (lastSize  == 0) lastSize = size;
            else if (lastSize != size) { QMessageBox::warning(this, "Warning", QString("Could not complete LDA.\nAll images must be the same size")); return; }
            inputImages.append(item->image());
        }
    }
    ldaDialog.setInputImages(inputImages);
    QList<LabelledPoints *> inputPoints;
    for (QTreeWidgetItemIterator it = QTreeWidgetItemIterator(ui->treeWidgetLabelledPointSet->invisibleRootItem()); *it; it++)
    {
        LabelledPointsTreeWidgetItem *item = dynamic_cast<LabelledPointsTreeWidgetItem *>((*it));
        if (item && item->labelledPoints() && item->checkState(POINTS_TREE_SELECTED))
        {
            inputPoints.append(item->labelledPoints());
        }
    }
    ldaDialog.setInputPoints(inputPoints);
    int status = ldaDialog.exec();
    if (status == QDialog::Accepted)
    {
        QList<SingleChannelImage *> images = ldaDialog.ouputImages();
        QTreeWidgetItem *newParent = doNewTreeWidgetImageSet(ui->treeWidgetImageSet->invisibleRootItem());
        for (int i = 0; i < images.size(); i++)
        {
            QStringList itemStrings;
            itemStrings << images[i]->name() << "" << "" << "" << "";
            ImageTreeWidgetItem *newImage = new ImageTreeWidgetItem(newParent, itemStrings, images[i]);
            newImage->setCheckState(IMAGE_TREE_COLUMN_RED, Qt::Unchecked);
            newImage->setCheckState(IMAGE_TREE_COLUMN_GREEN, Qt::Unchecked);
            newImage->setCheckState(IMAGE_TREE_COLUMN_BLUE, Qt::Unchecked);
            newImage->setCheckState(IMAGE_TREE_COLUMN_SELECTED, Qt::Unchecked);
        }
        newParent->setExpanded(true);
        for (int i = 0; i < ui->treeWidgetImageSet->columnCount(); i++) ui->treeWidgetImageSet->resizeColumnToContents(i);
        m_isModified = true;
        emit emitStatus(tr("LDA OK"));
    }}


void MdiChild::menuRequestLabelledPointSet(const QPoint &p)
{
    // we need a different menu depending on how many items are selected
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidgetLabelledPointSet->selectedItems();
    QMenu menu(this);

    QAction *newFolderAct = new QAction(QIcon(":/images/folder-new-7.png"), tr("New Folder"), this);
    newFolderAct->setStatusTip(tr("Create a new folder in the tree"));
    QAction *deleteAct = new QAction(QIcon(":/images/edit-delete-2.png"), tr("Delete..."), this);
    deleteAct->setStatusTip(tr("Deletes the reference to an image or a folder"));
    QAction *renameAct = new QAction(QIcon(":/images/edit-rename.png"), tr("Rename..."), this);
    renameAct->setStatusTip(tr("Rename the reference to an image or a folder"));
    QAction *newPointAct = new QAction(QIcon(":/images/new-point.png"), tr("New Points"), this);
    newPointAct->setStatusTip(tr("Create a new point set"));
    QAction *colourAct = new QAction(QIcon(":/images/colorize.png"), tr("Colour..."), this);
    colourAct->setStatusTip(tr("Choose the colour of the point set"));



    Q_ASSERT(selectedItems.size() <= 1);
    if (selectedItems.size() == 0)
    {
        menu.addAction(newFolderAct);
    }
    else if (selectedItems.size() == 1)
    {
        LabelledPointsTreeWidgetItem *item = dynamic_cast<LabelledPointsTreeWidgetItem *>(selectedItems[0]);
        menu.addAction(newFolderAct);
        menu.addAction(newPointAct);
        menu.addSeparator();
        menu.addAction(deleteAct);
        menu.addAction(renameAct);
        if (item->labelledPoints())
        {
            menu.addSeparator();
            menu.addAction(colourAct);
        }
    }
    QPoint gp = ui->treeWidgetLabelledPointSet->mapToGlobal(p);
    QAction *action = menu.exec(gp);
    if (action == newPointAct) doNewPointsTreeWidgetLabelledPointSet();
    if (action == newFolderAct) doNewFolderTreeWidgetLabelledPointSet();
    if (action == deleteAct) doDeleteTreeWidgetLabelledPointSet();
    if (action == renameAct) doRenameTreeWidgetLabelledPointSet();
    if (action == colourAct) doColourTreeWidgetLabelledPointSet();
}

void MdiChild::doColourTreeWidgetLabelledPointSet()
{
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidgetLabelledPointSet->selectedItems();
    LabelledPointsTreeWidgetItem *item = dynamic_cast<LabelledPointsTreeWidgetItem *>(selectedItems[0]);
    if (item == 0 || item->labelledPoints() == 0) { qDebug("doColourTreeWidgetLabelledPointSet: Unexpected cast fail\n"); return; }
    QColor colour = QColorDialog::getColor(item->labelledPoints()->colour(), this, "Select Color",
                                           QColorDialog::ShowAlphaChannel | QColorDialog::ColorDialogOption(EXTRA_COLOUR_DIALOG_OPTIONS));
    if (colour.isValid())
    {
        item->labelledPoints()->setColour(colour);
        item->setBackgroundColor(0, item->labelledPoints()->colour());
        m_graphicsView->update();
    }
}

void  MdiChild::doNewFolderTreeWidgetLabelledPointSet()
{
    QTreeWidgetItem *parent;
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidgetLabelledPointSet->selectedItems();
    if (selectedItems.size() == 0) // create a new root folder
    {
        parent = ui->treeWidgetLabelledPointSet->invisibleRootItem();
    }
    else // create a parented folder
    {
        LabelledPointsTreeWidgetItem *imageTreeWidgetItem = dynamic_cast<LabelledPointsTreeWidgetItem *>(selectedItems[0]);
        if (imageTreeWidgetItem == 0) { qDebug("doNewFolderTreeWidgetLabelledPointSet: Unexpected cast fail\n"); return; }
        if (imageTreeWidgetItem->labelledPoints() == 0) parent = imageTreeWidgetItem;
        else parent = imageTreeWidgetItem->parent();
    }

    QStringList itemStrings;
    itemStrings << QString("%1 %2").arg(m_defaultLabelledSetTreeItemName).arg(m_defaultLabelledSetTreeItemNameCount, 2, 10, QChar('0')) << "" << "" << "";
    LabelledPointsTreeWidgetItem *newItem = new LabelledPointsTreeWidgetItem(parent, itemStrings);
    newItem->setData(1, Qt::CheckStateRole, QVariant());
    newItem->setData(2, Qt::CheckStateRole, QVariant());
    newItem->setData(3, Qt::CheckStateRole, QVariant());
    m_defaultLabelledSetTreeItemNameCount++;
    for (int i = 0; i < ui->treeWidgetLabelledPointSet->columnCount(); i++) ui->treeWidgetLabelledPointSet->resizeColumnToContents(i);
}

void MdiChild::doNewPointsTreeWidgetLabelledPointSet()
{
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidgetLabelledPointSet->selectedItems();
    if (selectedItems.size() == 0) { qDebug("doNewPointsTreeWidgetLabelledPointSet: Unexpected no selection \n"); return; }
    LabelledPointsTreeWidgetItem *item = dynamic_cast<LabelledPointsTreeWidgetItem *>(selectedItems[0]);
    if (item == 0) { qDebug("doNewPointsTreeWidgetLabelledPointSet: Unexpected cast fail\n"); return; }
    QTreeWidgetItem *labelledPointParent;
    if (item->labelledPoints() == 0) labelledPointParent = item;
    else labelledPointParent = item->parent();
    LabelledPoints *labelledPoints = new LabelledPoints();
    labelledPoints->setName(QString("%1 %2").arg(m_defaultLabelledPointsName).arg(m_defaultLabelledPointsNameCount));
    QStringList itemStrings;
    itemStrings << labelledPoints->name() << "" << "" << "";
    LabelledPointsTreeWidgetItem *newPointSet = new LabelledPointsTreeWidgetItem(labelledPointParent, itemStrings, labelledPoints);
    newPointSet->setCheckState(POINTS_TREE_DISPLAY, Qt::Unchecked);
    newPointSet->setCheckState(POINTS_TREE_CURRENT, Qt::Unchecked);
    newPointSet->setCheckState(POINTS_TREE_SELECTED, Qt::Unchecked);
    newPointSet->setBackgroundColor(0, newPointSet->labelledPoints()->colour());
    labelledPointParent->setExpanded(true);
    for (int i = 0; i < ui->treeWidgetLabelledPointSet->columnCount(); i++) ui->treeWidgetLabelledPointSet->resizeColumnToContents(i);
    m_isModified = true;
    m_defaultLabelledPointsNameCount++;
    emit emitStatus(QString("%1 labelled point set created").arg(labelledPoints->name()));
}

void MdiChild::doRenameTreeWidgetLabelledPointSet()
{
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidgetLabelledPointSet->selectedItems();
    if (selectedItems.size() == 0) { qDebug("doRenameTreeWidgetLabelledPointSet: Unexpected no selection \n"); return; };
    if (selectedItems.size() > 1) { qDebug("doRenameTreeWidgetLabelledPointSet: Unexpected >1 item selected \n"); return; };
    LabelledPointsTreeWidgetItem *selectedItem = dynamic_cast<LabelledPointsTreeWidgetItem *>(selectedItems[0]);
    if (selectedItem == 0) { qDebug("doRenameTreeWidgetLabelledPointSet: Unexpected cast fail\n"); return; }
    bool ok;
    QString text = QInputDialog::getText(this, tr("Rename Labelled Point Set"), selectedItem->text(0), QLineEdit::Normal, selectedItem->text(0), &ok);
    if (ok && !text.isEmpty())
    {
        emit emitStatus(QString("%1 renamed to %2").arg(selectedItem->text(0)).arg(text));
        selectedItem->setText(0, text);
        if (selectedItem->labelledPoints()) selectedItem->labelledPoints()->setName(text);
        for (int i = 0; i < ui->treeWidgetLabelledPointSet->columnCount(); i++) ui->treeWidgetLabelledPointSet->resizeColumnToContents(i);
    }
}

void MdiChild::treeWidgetItemChangedLabelledPointSet(QTreeWidgetItem *item, int column)
{
    if (column != 0) return; // only interested in the first column
    LabelledPointsTreeWidgetItem *theItem = dynamic_cast<LabelledPointsTreeWidgetItem *>(item);
    if (theItem && theItem->labelledPoints()) theItem->labelledPoints()->setName(theItem->text(0));
    for (int i = 0; i < ui->treeWidgetLabelledPointSet->columnCount(); i++) ui->treeWidgetLabelledPointSet->resizeColumnToContents(i);
}


int MdiChild::numSelectedPoints() const
{
    return m_numSelectedPoints;
}

int MdiChild::numSelectedImages() const
{
    return m_numSelectedImages;
}

void  MdiChild::doDeleteTreeWidgetLabelledPointSet()
{
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidgetLabelledPointSet->selectedItems();
    if (selectedItems.size() == 0) { qDebug("doDeleteTreeWidgetLabelledPointSet: Unexpected no selection \n"); return; };
    int ret = QMessageBox::warning(this, tr("Delete Points from Set"), tr("%1 items to delete.\nThis cannot be undone and completely deletes the stored points.\nDo you want to continue?").arg(selectedItems.size()), QMessageBox::Ok | QMessageBox::Cancel);
    if (ret == QMessageBox::Ok)
    {
        for (int i = 0; i < selectedItems.size(); i++)
        {
            LabelledPointsTreeWidgetItem *item = dynamic_cast<LabelledPointsTreeWidgetItem *>(selectedItems[i]);
            if (item == 0) { qDebug("doDeleteTreeWidgetLabelledPointSet: Unexpected cast fail\n"); return; }
            item->setDeleteLater(true);
        }
        int numDeleted = 1;
        while (numDeleted)
        {
            numDeleted = 0;
            for (QTreeWidgetItemIterator it = QTreeWidgetItemIterator(ui->treeWidgetLabelledPointSet->invisibleRootItem()); *it; it++)
            {
                LabelledPointsTreeWidgetItem *item = dynamic_cast<LabelledPointsTreeWidgetItem *>(*it);
                if (item && item->childCount() == 0 && item->deleteLater()) { delete item; numDeleted++; }
            }
        }
        treeWidgetClickedLabelledPointSet(0, 0);
    }
}

void MdiChild::createNewLabelledPoint(float x, float y)
{
    if (m_activeLabelledPointItem)
    {
        m_activeLabelledPointItem->labelledPoints()->AddPoint(x, y);
        emit emitStatus(QString("X=%1 Y=%2 added to %3").arg(x).arg(y).arg(m_activeLabelledPointItem->labelledPoints()->name()));
        m_graphicsView->update();
    }
    else
    {
        emit emitStatus(QString("No point list active"));
    }
}

void MdiChild::deleteCurrentLabelledPoint(float x, float y)
{
    if (m_activeLabelledPointItem)
    {
        float closestDistance2 = FLT_MAX;
        int closestDistanceIndex = -1;
        float distance2;
        QList<QPointF> *points = m_activeLabelledPointItem->labelledPoints()->points();
        for (int i = 0; i < points->size(); i++)
        {
            distance2 = SQUARE(points->at(i).x()) + SQUARE(points->at(i).y());
            if (distance2 < closestDistance2)
            {
                closestDistance2 = distance2;
                closestDistanceIndex = i;
            }
        }
        points->removeAt(closestDistanceIndex);
        emit emitStatus(QString("X=%1 Y=%2 deleted to %3").arg(x).arg(y).arg(m_activeLabelledPointItem->labelledPoints()->name()));
        m_graphicsView->update();
    }
    else
    {
        emit emitStatus(QString("No point list active"));
    }
}

void MdiChild::menuRequestImageSet(const QPoint &p)
{
    // we need a different menu depending on how many items are selected
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidgetImageSet->selectedItems();
    QMenu menu(this);
    Q_ASSERT(selectedItems.size() <= 1);

    QAction *newFolderAct = new QAction(QIcon(":/images/folder-new-7.png"), tr("New Folder"), this);
    newFolderAct->setStatusTip(tr("Create a new folder in the tree"));
    QAction *deleteAct = new QAction(QIcon(":/images/edit-delete-2.png"), tr("Delete..."), this);
    deleteAct->setStatusTip(tr("Deletes the reference to an image or a folder"));
    QAction *renameAct = new QAction(QIcon(":/images/edit-rename.png"), tr("Rename..."), this);
    renameAct->setStatusTip(tr("Rename the reference to an image or a folder"));
    QAction *normalDisplayAct = new QAction(QIcon(":/images/no_tree_s.png"), tr("Display Normal"), this);
    normalDisplayAct->setStatusTip(tr("Displays the untransformed image"));
    QAction *logDisplayAct = new QAction(QIcon(":/images/tree_s.png"), tr("Display Log"), this);
    logDisplayAct->setStatusTip(tr("Displays the log transformed image"));

    if (selectedItems.size() == 0)
    {
        menu.addAction(newFolderAct);
    }
    else if (selectedItems.size() == 1)
    {
        menu.addAction(newFolderAct);
        menu.addAction(deleteAct);
        menu.addAction(renameAct);
        menu.addSeparator();
        menu.addAction(normalDisplayAct);
        menu.addAction(logDisplayAct);
    }
    QPoint gp = ui->treeWidgetImageSet->mapToGlobal(p);
    QAction *action = menu.exec(gp);
    if (action == newFolderAct) doNewTreeWidgetImageSet(0);
    if (action == deleteAct) doDeleteTreeWidgetImageSet();
    if (action == renameAct) doRenameTreeWidgetImageSet();
    if (action == normalDisplayAct) doSetLogDisplayTreeWidgetImageSet(false);
    if (action == logDisplayAct) doSetLogDisplayTreeWidgetImageSet(true);
}

QTreeWidgetItem *MdiChild::doNewTreeWidgetImageSet(QTreeWidgetItem *parent)
{
    if (parent == 0)
    {
        QList<QTreeWidgetItem *> selectedItems = ui->treeWidgetImageSet->selectedItems();
        if (selectedItems.size() == 0) // create a new root folder
        {
            parent = ui->treeWidgetImageSet->invisibleRootItem();
        }
        else // create a parented folder
        {
            ImageTreeWidgetItem *imageTreeWidgetItem = dynamic_cast<ImageTreeWidgetItem *>(selectedItems[0]);
            if (imageTreeWidgetItem == 0) { qDebug("doNewTreeWidgetImageSet: Unexpected cast fail\n"); return 0; }
            if (imageTreeWidgetItem->image() == 0) parent = imageTreeWidgetItem;
            else parent = imageTreeWidgetItem->parent();
        }
    }

    QStringList itemStrings;
    itemStrings << QString("%1 %2").arg(m_defaultImportTreeItemName).arg(m_defaultImportTreeItemNameCount, 2, 10, QChar('0')) << "" << "" << "";
    qDebug("doNewTreeWidgetImageSet: %s", qUtf8Printable(itemStrings[0]));
    ImageTreeWidgetItem *newItem = new ImageTreeWidgetItem(parent, itemStrings);
    newItem->setData(1, Qt::CheckStateRole, QVariant());
    newItem->setData(2, Qt::CheckStateRole, QVariant());
    newItem->setData(3, Qt::CheckStateRole, QVariant());
    m_defaultImportTreeItemNameCount++;
    for (int i = 0; i < ui->treeWidgetLabelledPointSet->columnCount(); i++) ui->treeWidgetLabelledPointSet->resizeColumnToContents(i);
    return newItem;
}

void MdiChild::doDeleteTreeWidgetImageSet()
{
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidgetImageSet->selectedItems();
    if (selectedItems.size() == 0) { qDebug("doDeleteTreeWidgetImageSet: Unexpected no selection \n"); return; };
    int ret = QMessageBox::warning(this, tr("Delete Images from Set"), tr("%1 items to delete.\nThis cannot be undone but does not delete the image files.\nDo you want to continue?").arg(selectedItems.size()), QMessageBox::Ok | QMessageBox::Cancel);
    if (ret == QMessageBox::Ok)
    {
        for (int i = 0; i < selectedItems.size(); i++)
        {
            ImageTreeWidgetItem *item = dynamic_cast<ImageTreeWidgetItem *>(selectedItems[i]);
            if (item == 0) { qDebug("doDeleteTreeWidgetImageSet: Unexpected cast fail\n"); return; }
            item->setDeleteLater(true);
        }
        int numDeleted = 1;
        while (numDeleted)
        {
            numDeleted = 0;
            for (QTreeWidgetItemIterator it = QTreeWidgetItemIterator(ui->treeWidgetImageSet->invisibleRootItem()); *it; it++)
            {
                ImageTreeWidgetItem *item = dynamic_cast<ImageTreeWidgetItem *>(*it);
                if (item && item->childCount() == 0 && item->deleteLater()) { delete item; numDeleted++; }
            }
        }
        treeWidgetClicked(0, 0);
    }
}

void MdiChild::doSetLogDisplayTreeWidgetImageSet(bool useLog)
{
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidgetImageSet->selectedItems();
    if (selectedItems.size() == 0) { qDebug("doDeleteTreeWidgetImageSet: Unexpected no selection \n"); return; };
    for (int i = 0; i < selectedItems.size(); i++)
    {
        ImageTreeWidgetItem *item = dynamic_cast<ImageTreeWidgetItem *>(selectedItems[i]);
        if (item == 0) { qDebug("doDeleteTreeWidgetImageSet: Unexpected cast fail\n"); return; }
        if (item->image()) item->image()->setDisplayLogged(useLog);
        if (item->childCount() > 0)
        {
            for (QTreeWidgetItemIterator it = QTreeWidgetItemIterator(item); *it; it++)
            {
                ImageTreeWidgetItem *childItem = dynamic_cast<ImageTreeWidgetItem *>(*it);
                if (childItem && childItem->image()) childItem->image()->setDisplayLogged(useLog);
            }
        }
    }
    treeWidgetClicked(0, 0);
    m_histogramDisplay->update();
}


void  MdiChild::doRenameTreeWidgetImageSet()
{
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidgetImageSet->selectedItems();
    if (selectedItems.size() == 0) { qDebug("doRenameTreeWidgetImageSet: Unexpected no selection \n"); return; };
    if (selectedItems.size() > 1) { qDebug("doRenameTreeWidgetImageSet: Unexpected >1 item selected \n"); return; };
    ImageTreeWidgetItem *selectedItem = dynamic_cast<ImageTreeWidgetItem *>(selectedItems[0]);
    bool ok;
    QString text = QInputDialog::getText(this, tr("Rename Image Label"), selectedItem->text(0), QLineEdit::Normal, selectedItem->text(0), &ok);
    if (ok && !text.isEmpty())
    {
        emit emitStatus(QString("%1 renamed to %2").arg(selectedItem->text(0)).arg(text));
        selectedItem->setText(0, text);
        if (selectedItem->image()) selectedItem->image()->setName(text);
        for (int i = 0; i < ui->treeWidgetImageSet->columnCount(); i++) ui->treeWidgetImageSet->resizeColumnToContents(i);
    }
}

void MdiChild::treeWidgetItemChanged(QTreeWidgetItem *item, int column)
{
    if (column != 0) return; // only interested in the first column
    ImageTreeWidgetItem *theItem = dynamic_cast<ImageTreeWidgetItem *>(item);
    if (theItem && theItem->image()) theItem->image()->setName(theItem->text(0));
    for (int i = 0; i < ui->treeWidgetImageSet->columnCount(); i++) ui->treeWidgetImageSet->resizeColumnToContents(i);
}

void MdiChild::treeWidgetSelected()
{
    int spinBoxStepResolution = Settings::value("Spin Box Min and Max Step Resolution", 20).toInt();
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidgetImageSet->selectedItems();
    if (selectedItems.size() == 0) goto no_image_selected;
    {
        ImageTreeWidgetItem *item = dynamic_cast<ImageTreeWidgetItem *>(selectedItems[0]);
        if (item == 0) goto no_image_selected;
        SingleChannelImage *image = item->image();
        if (image == 0) goto no_image_selected;

        {
            float dataMin;
            if (image->displayLogged()) dataMin = image->dataLogMin();
            else dataMin = image->dataMin();
            m_histogramDisplay->setImage(image);
            m_histogramDisplay->setEnabled(true);
            m_histogramDisplay->update();
            QSignalBlocker blocker1(ui->doubleSpinBoxMin);
            QSignalBlocker blocker2(ui->doubleSpinBoxMax);
            QSignalBlocker blocker3(ui->doubleSpinBoxGamma);
            QSignalBlocker blocker4(ui->doubleSpinBoxZebra);
            QSignalBlocker blocker5(ui->verticalSliderMinimum);
            QSignalBlocker blocker6(ui->verticalSliderMaximum);
            QSignalBlocker blocker7(ui->verticalSliderGamma);
            ui->doubleSpinBoxMin->setEnabled(true);
            ui->doubleSpinBoxMax->setEnabled(true);
            ui->doubleSpinBoxGamma->setEnabled(true);
            ui->doubleSpinBoxZebra->setEnabled(true);
            ui->doubleSpinBoxMin->setMinimum(dataMin);
            ui->doubleSpinBoxMax->setMinimum(dataMin);
            ui->doubleSpinBoxMin->setMaximum(image->dataMax());
            ui->doubleSpinBoxMax->setMaximum(image->dataMax());
            ui->doubleSpinBoxMin->setSingleStep((image->dataMax() - dataMin) / spinBoxStepResolution);
            ui->doubleSpinBoxMax->setSingleStep((image->dataMax() - dataMin) / spinBoxStepResolution);
            ui->doubleSpinBoxMin->setValue(image->displayMin());
            ui->doubleSpinBoxMax->setValue(image->displayMax());
            ui->doubleSpinBoxGamma->setValue(image->displayGamma());
            ui->doubleSpinBoxZebra->setValue(image->displayZebra());
            ui->verticalSliderMinimum->setEnabled(true);
            ui->verticalSliderMaximum->setEnabled(true);
            ui->verticalSliderGamma->setEnabled(true);
            ui->verticalSliderMinimum->setValue(((image->displayMin() - dataMin) / (image->dataMax() - dataMin)) * ui->verticalSliderMinimum->maximum());
            ui->verticalSliderMaximum->setValue(((image->displayMax() - dataMin) / (image->dataMax() - dataMin)) * ui->verticalSliderMaximum->maximum());
            int gammaRange = Settings::value("Gamma Range (log units)", 1).toInt();
            float gammaScale = (std::log10(image->displayGamma()) + gammaRange) / (gammaRange * 2.0f);
            ui->verticalSliderGamma->setValue(gammaScale * ui->verticalSliderMaximum->maximum());
        }
        return;
    }

no_image_selected:
    ui->doubleSpinBoxMin->setEnabled(false);
    ui->doubleSpinBoxMax->setEnabled(false);
    ui->doubleSpinBoxGamma->setEnabled(false);
    ui->doubleSpinBoxZebra->setEnabled(false);
    ui->verticalSliderMinimum->setEnabled(false);
    ui->verticalSliderMaximum->setEnabled(false);
    ui->verticalSliderGamma->setEnabled(false);
    m_histogramDisplay->setEnabled(false);
    m_histogramDisplay->setImage(0);
    m_histogramDisplay->update();
}

void MdiChild::treeWidgetDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    ImageTreeWidgetItem *imageTree = dynamic_cast<ImageTreeWidgetItem *>(item);
    if (imageTree && imageTree->image())
    {
        imageTree->setCheckState(IMAGE_TREE_COLUMN_RED, Qt::Checked);
        imageTree->setCheckState(IMAGE_TREE_COLUMN_GREEN, Qt::Checked);
        imageTree->setCheckState(IMAGE_TREE_COLUMN_BLUE, Qt::Checked);
    }
    if (imageTree && imageTree->childCount() > 0)
    {
        if (imageTree->isExpanded()) imageTree->setExpanded(false);
        else imageTree->setExpanded(true);
    }
}

void MdiChild::treeWidgetDoubleClickedLabelledPointSet(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    LabelledPointsTreeWidgetItem *pointTree = dynamic_cast<LabelledPointsTreeWidgetItem *>(item);
    if (pointTree && pointTree->labelledPoints())
    {
        pointTree->setCheckState(POINTS_TREE_DISPLAY, Qt::Checked);
        pointTree->setCheckState(POINTS_TREE_CURRENT, Qt::Checked);
    }
    if (pointTree && pointTree->childCount() > 0)
    {
        if (pointTree->isExpanded()) pointTree->setExpanded(false);
        else pointTree->setExpanded(true);
    }
}

void MdiChild::zoomToValue(float zoom)
{
    m_currentZoomLevel = zoom;
    m_graphicsView->setZoom(zoom);
    m_graphicsView->update();
    m_customScroller->contentsResized();
    Settings::setValue("_currentZoomLevel", m_currentZoomLevel);
}

void MdiChild::updateStatus(QString status)
{
    emit emitStatus(status);
}

void MdiChild::preferencesUpdated()
{
    m_graphicsView->setSelectionColour(Settings::value("Selection Colour", QColor(0, 255, 0)).value<QColor>());
    m_graphicsView->setMarkerSize(Settings::value("Marker Size", float(100)).toFloat());
    m_graphicsView->update();
    m_histogramDisplay->setHistogramColour(Settings::value("Histogram Bar Colour", QColor(0, 100, 50)).value<QColor>());
    m_histogramDisplay->setOutputHistogramColour(Settings::value("Output Histogram Bar Colour", QColor(150, 100, 0)).value<QColor>());
    m_histogramDisplay->setLineColour(Settings::value("Histogram Line Colour", QColor(0, 100, 150)).value<QColor>());
    m_histogramDisplay->update();
    m_zoomMultiplier = Settings::value("Zoom Modifier", 2.0f).toFloat();
    ui->doubleSpinBoxGamma->setMinimum(std::pow(10.0, -Settings::value("Gamma Range (log units)", 1).toInt()));
    ui->doubleSpinBoxGamma->setMaximum(std::pow(10.0, Settings::value("Gamma Range (log units)", 1).toInt()));
    treeWidgetSelected();
}

void MdiChild::ticker()
{
}

void MdiChild::menuRequestHistogram(const QPoint &p)
{
    QMenu menu(this);
    QAction *autoGammaAct = new QAction(QIcon(":/images/tools-wizard.png"), tr("Auto Gamma"), this);
    autoGammaAct->setStatusTip(tr("Calculates optimal gamma values to maximise contrast"));
    QAction *invertAct = new QAction(QIcon(":/images/invert-icon.png"), tr("Invert"), this);
    autoGammaAct->setStatusTip(tr("Inverts the greyscale values by swapping the min and max"));
    QAction *resetDisplayAct = new QAction(QIcon(":/images/reset@2x.png"), tr("Reset Display"), this);
    resetDisplayAct->setStatusTip(tr("Resets the display to default values"));

    menu.addAction(autoGammaAct);
    menu.addAction(invertAct);
    menu.addAction(resetDisplayAct);

    QPoint gp = m_histogramDisplay->mapToGlobal(p);
    QAction *action = menu.exec(gp);
    if (action == autoGammaAct)
    {
        autoGamma();
    }
    else if (action == invertAct)
    {
        invertImage();
    }
    else if (action == resetDisplayAct)
    {
        resetDisplay();
    }

}

void MdiChild::treeWidgetHeaderDoubleClicked(int logicalIndex)
{
    if (logicalIndex == IMAGE_TREE_COLUMN_SELECTED)
    {
        for (QTreeWidgetItemIterator it = QTreeWidgetItemIterator(ui->treeWidgetImageSet->invisibleRootItem()); *it; it++)
        {
            qDebug("QTreeWidgetItem.text=%s", qUtf8Printable((*it)->text(0)));
            ImageTreeWidgetItem *imageItem = dynamic_cast<ImageTreeWidgetItem *>(*it);
            if (imageItem && imageItem->image())
            {
                qDebug("imageItem->parent().text=%s", qUtf8Printable(imageItem->parent()->text(0)));
                if (imageItem->parent()->isExpanded())
                {
                    if (imageItem->checkState(IMAGE_TREE_COLUMN_SELECTED) == Qt::Unchecked) imageItem->setCheckState(IMAGE_TREE_COLUMN_SELECTED, Qt::Checked);
                    else imageItem->setCheckState(IMAGE_TREE_COLUMN_SELECTED, Qt::Unchecked);
                }
            }
        }
    }
}

void MdiChild::treeWidgetHeaderDoubleClickedLabelledPointSet(int logicalIndex)
{
    if (logicalIndex == POINTS_TREE_DISPLAY)
    {
        QList<LabelledPoints *> *points = m_graphicsView->labelledPoints();
        points->clear();
        m_activeLabelledPointItem = 0;
        for (QTreeWidgetItemIterator it = QTreeWidgetItemIterator(ui->treeWidgetLabelledPointSet->invisibleRootItem()); *it; it++)
        {
            LabelledPointsTreeWidgetItem *pointsItem = dynamic_cast<LabelledPointsTreeWidgetItem *>(*it);
            if (pointsItem && pointsItem->labelledPoints())
            {
                if (pointsItem->parent()->isExpanded())
                {
                    if (pointsItem->checkState(POINTS_TREE_DISPLAY) == Qt::Unchecked)
                    {
                        pointsItem->setCheckState(POINTS_TREE_DISPLAY, Qt::Checked);
                        points->append(pointsItem->labelledPoints());
                        m_numSelectedPoints++;
                    }
                    else pointsItem->setCheckState(POINTS_TREE_DISPLAY, Qt::Unchecked);
                }
            }
        }
        m_graphicsView->update();
    }
    else if (logicalIndex == POINTS_TREE_SELECTED)
    {
        for (QTreeWidgetItemIterator it = QTreeWidgetItemIterator(ui->treeWidgetLabelledPointSet->invisibleRootItem()); *it; it++)
        {
            LabelledPointsTreeWidgetItem *pointsItem = dynamic_cast<LabelledPointsTreeWidgetItem *>(*it);
            if (pointsItem && pointsItem->labelledPoints())
            {
                if (pointsItem->parent()->isExpanded())
                {
                    if (pointsItem->checkState(POINTS_TREE_SELECTED) == Qt::Unchecked) pointsItem->setCheckState(POINTS_TREE_SELECTED, Qt::Checked);
                    else pointsItem->setCheckState(POINTS_TREE_SELECTED, Qt::Unchecked);
                }
            }
        }
    }
}

void MdiChild::drawLog(bool drawLogged)
{
    SingleChannelImage *image;
    for (QTreeWidgetItemIterator it = QTreeWidgetItemIterator(ui->treeWidgetImageSet->invisibleRootItem()); *it; it++)
    {
        ImageTreeWidgetItem *imageItem = dynamic_cast<ImageTreeWidgetItem *>(*it);
        if (imageItem && imageItem->image())
        {
            image = imageItem->image();
            if (imageItem->checkState(IMAGE_TREE_COLUMN_RED) == Qt::Checked)
            {
                image->setDisplayLogged(drawLogged);
                image->setDisplayRange(std::max(image->dataLogMin(), image->displayMin()), image->displayMax());
                m_graphicsView->setTextureDisplay(image->displayMin(), image->displayMax(), image->displayGamma(), image->displayZebra(), image->displayLogged(), GraphicsView::Red);
            }
            if (imageItem->checkState(IMAGE_TREE_COLUMN_GREEN) == Qt::Checked)
            {
                image->setDisplayLogged(drawLogged);
                image->setDisplayRange(std::max(image->dataLogMin(), image->displayMin()), image->displayMax());
                m_graphicsView->setTextureDisplay(image->displayMin(), image->displayMax(), image->displayGamma(), image->displayZebra(), image->displayLogged(), GraphicsView::Green);
            }
            if (imageItem->checkState(IMAGE_TREE_COLUMN_BLUE) == Qt::Checked)
            {
                image->setDisplayLogged(drawLogged);
                image->setDisplayRange(std::max(image->dataLogMin(), image->displayMin()), image->displayMax());
                m_graphicsView->setTextureDisplay(image->displayMin(), image->displayMax(), image->displayGamma(), image->displayZebra(), image->displayLogged(), GraphicsView::Blue);
            }
        }
    }
    m_isModified = true;
    treeWidgetSelected();
}
