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

#ifndef MDICHILD_H
#define MDICHILD_H

#include <QDialog>
#include <QList>
#include <QPointF>
#include <QStringList>
#include <QTextCursor>
#include <QMainWindow>
#include <QString>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QPointF>

#include "GraphicsView.h"

class MultiSpectralDocument;
class HistogramDisplayWidget;
class QTreeWidgetItem;
class CustomScroller;
class LabelledPointsTreeWidgetItem;
class ImageTreeWidgetItem;

namespace Ui {
class MdiChild;
}

class MdiChild : public QDialog
{
    Q_OBJECT

public:
    explicit MdiChild(QWidget *parent = 0);
    ~MdiChild();

    void newFile();
    bool loadFile(const QString &fileName);
    bool save();
    bool saveAs();
    bool saveFile(const QString &fileName);
    QString userFriendlyCurrentFile();
    QString currentFile() { return m_currentFile; }
    bool importImage(const QString &fileName);
    bool exportImage(const QString &fileName);
    void preferencesUpdated();
    void displayOneImageIfNecessary();

    void cut() {}
    void copy() {}
    void paste() {}

    QTextCursor textCursor() { return QTextCursor(); }

    int numSelectedImages() const;
    int numSelectedPoints() const;

public slots:
    void zoomIn();
    void zoomOut();
    void rotateLeft();
    void rotateRight();
    void nextChannel();
    void prevChannel();
    void autoGamma();
    void invertImage();
    void resetDisplay();
    void performRecipes();

    void displayValueChanged(double value);
    void sliderValueChanged(int value);

    void treeWidgetClicked(QTreeWidgetItem *item, int column);
    void treeWidgetSelected();
    void treeWidgetClickedLabelledPointSet(QTreeWidgetItem *item, int column);
    void treeWidgetDoubleClicked(QTreeWidgetItem *item, int column);
    void treeWidgetItemChanged(QTreeWidgetItem *item, int column);
    void treeWidgetDoubleClickedLabelledPointSet(QTreeWidgetItem *item, int column);
    void treeWidgetItemChangedLabelledPointSet(QTreeWidgetItem *item, int column);
    void treeWidgetHeaderDoubleClicked(int logicalIndex);
    void treeWidgetHeaderDoubleClickedLabelledPointSet(int logicalIndex);
    void menuRequestImageSet(const QPoint &p);
    void menuRequestLabelledPointSet(const QPoint &p);
    void menuRequestHistogram(const QPoint &p);
    void createNewLabelledPoint(float x, float y);
    void deleteCurrentLabelledPoint(float x, float y);
    void zoomToValue(float zoomToValue);
    void updateStatus(QString status);
    void ticker();
    void drawLog(bool drawLogged);

    void pca();
    void lda();
    void importHDF5();

signals:
    void copyAvailable(bool);
    void emitStatus(QString status);

protected:
    void closeEvent(QCloseEvent *event);
    void changeEvent(QEvent *event);

private slots:
    void documentWasModified();

private:
    Ui::MdiChild *ui;

    bool maybeSave();
    void setCurrentFile(const QString &fileName);

    QTreeWidgetItem *doNewTreeWidgetImageSet(QTreeWidgetItem *parent);
    void doDeleteTreeWidgetImageSet();
    void doRenameTreeWidgetImageSet();
    void doSetLogDisplayTreeWidgetImageSet(bool useLog);
    void doColourTreeWidgetLabelledPointSet();
    void doNewPointsTreeWidgetLabelledPointSet();
    void doNewFolderTreeWidgetLabelledPointSet();
    void doDeleteTreeWidgetLabelledPointSet();
    void doRenameTreeWidgetLabelledPointSet();
    void activateImage(ImageTreeWidgetItem *item);


    QString m_currentFile;
    bool m_isUntitled;
    bool m_isModified;

    GraphicsView *m_graphicsView;
    CustomScroller *m_customScroller;

    HistogramDisplayWidget *m_histogramDisplay;

    float m_currentZoomLevel;
    float m_zoomMultiplier;
    float m_currentRotation;

    MultiSpectralDocument *m_document;

    QString m_defaultImportTreeItemName;
    QString m_defaultLabelledSetTreeItemName;
    QString m_defaultLabelledPointsName;
    int m_defaultImportTreeItemNameCount;
    int m_defaultLabelledSetTreeItemNameCount;
    int m_defaultLabelledPointsNameCount;
    LabelledPointsTreeWidgetItem *m_activeLabelledPointItem;
    int m_numSelectedImages;
    int m_numSelectedPoints;
};

#endif
