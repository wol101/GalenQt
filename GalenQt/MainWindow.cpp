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

#include "MainWindow.h"
#include "MdiChild.h"
#include "AboutDialog.h"
#include "PreferencesDialog.h"
#include "Settings.h"

MainWindow::MainWindow()
{
    mdiArea = new QMdiArea;
    mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setCentralWidget(mdiArea);
    connect(mdiArea, &QMdiArea::subWindowActivated, this, &MainWindow::updateMenus);
    windowMapper = new QSignalMapper(this);
    connect(windowMapper, &QSignalMapper::mappedObject, this, &MainWindow::setActiveSubWindow);

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    updateMenus();

    readSettings();

    setWindowTitle(tr("GalenQt"));
    setWindowIcon(QIcon(":/Icon.iconset/icon_512x512@2x.png"));

    if (Settings::value("Reload Last Project File", 1).toBool())
    {
        QString lastFile = Settings::value("_lastOpenedMultiSpectralDocument", QString()).toString();
        if (QFileInfo(lastFile).isFile()) QTimer::singleShot(2000, this, SLOT(openLastFile()));
    }

//    m_ticker = new QTimer(this); // this timer
//    connect(m_ticker, SIGNAL(timeout()), this, SLOT(ticker()));
//    m_ticker->start(Settings::value("_tickIntervalmilliSeconds", 10000).toInt());
}

MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    mdiArea->closeAllSubWindows();
    if (mdiArea->currentSubWindow()) {
        event->ignore();
    } else {
        writeSettings();
        event->accept();
    }
}

void MainWindow::newFile()
{
        MdiChild *child = createMdiChild();
        child->newFile();
        child->show();
        if ((windowState() & Qt::WindowMaximized) || (windowState() & Qt::WindowFullScreen)) child->showMaximized(); // if the main window is maximised then create the children maximised
}

void MainWindow::open()
{
    QString lastOpenedMultiSpectralDocument = Settings::value("_lastOpenedMultiSpectralDocument", "").toString();
    QString fileName = QFileDialog::getOpenFileName(this, "Open Image Set File", lastOpenedMultiSpectralDocument,
                                                    "XML Files (*.XML *.xml);;Any File (*.*)", 0,
                                                    static_cast<QFileDialog::Options>(EXTRA_FILE_DIALOG_OPTIONS));
    if (!fileName.isEmpty())
    {
        open(fileName);
    }
}

void MainWindow::openLastFile()
{
    open(Settings::value("_lastOpenedMultiSpectralDocument", QString()).toString());
}

void MainWindow::open(const QString &fileName)
{
    // treat as open
    QMdiSubWindow *existing = findMdiChild(fileName);
    if (existing)
    {
        Settings::setValue("_lastOpenedMultiSpectralDocument", fileName);
        mdiArea->setActiveSubWindow(existing);
        return;
    }

    MdiChild *child = createMdiChild();
    if (child->loadFile(fileName))
    {
        statusBar()->showMessage(tr("File loaded"), 2000);
        child->show();
        if ((windowState() & Qt::WindowMaximized) || (windowState() & Qt::WindowFullScreen)) child->showMaximized(); // if the main window is maximised then create the children maximised
        child->displayOneImageIfNecessary();
    }
    else
    {
        child->close();
    }
}

void MainWindow::save()
{
    if (activeMdiChild() && activeMdiChild()->save())
        statusBar()->showMessage(tr("File saved"), 2000);
}

void MainWindow::saveAs()
{
    if (activeMdiChild() && activeMdiChild()->saveAs())
        statusBar()->showMessage(tr("File saved"), 2000);
}

#ifndef QT_NO_CLIPBOARD
void MainWindow::cut()
{
    if (activeMdiChild())
        activeMdiChild()->cut();
}

void MainWindow::copy()
{
    if (activeMdiChild())
        activeMdiChild()->copy();
}

void MainWindow::paste()
{
    if (activeMdiChild())
        activeMdiChild()->paste();
}
#endif

void MainWindow::about()
{
    AboutDialog aboutDialog(this);

    int status = aboutDialog.exec();

    if (status == QDialog::Accepted)
    {
    }
}

void MainWindow::updateMenus()
{
    bool hasMdiChild = (activeMdiChild() != 0);
    saveAct->setEnabled(hasMdiChild);
    saveAsAct->setEnabled(hasMdiChild);
#ifndef QT_NO_CLIPBOARD
    pasteAct->setEnabled(hasMdiChild);
#endif
    closeAct->setEnabled(hasMdiChild);
    closeAllAct->setEnabled(hasMdiChild);
    tileAct->setEnabled(hasMdiChild);
    cascadeAct->setEnabled(hasMdiChild);
    nextAct->setEnabled(hasMdiChild);
    previousAct->setEnabled(hasMdiChild);
    separatorAct->setVisible(hasMdiChild);

#ifndef QT_NO_CLIPBOARD
    bool hasSelection = (activeMdiChild() &&
                         activeMdiChild()->textCursor().hasSelection());
    cutAct->setEnabled(hasSelection);
    copyAct->setEnabled(hasSelection);
#endif

    nextChannelAct->setEnabled(hasMdiChild);
    prevChannelAct->setEnabled(hasMdiChild);
    zoomInAct->setEnabled(hasMdiChild);
    zoomOutAct->setEnabled(hasMdiChild);
    rotateLeftAct->setEnabled(hasMdiChild);
    rotateRightAct->setEnabled(hasMdiChild);
    performRecipesAct->setEnabled(hasMdiChild);
    importImagesAct->setEnabled(hasMdiChild);
    importHDF5Act->setEnabled(hasMdiChild);
    exportImageAct->setEnabled(hasMdiChild);

    pcaAct->setEnabled(hasMdiChild && activeMdiChild()->numSelectedImages() > 1);
    ldaAct->setEnabled(hasMdiChild && activeMdiChild()->numSelectedImages() > 1 && activeMdiChild()->numSelectedPoints() > 1);
}

void MainWindow::updateWindowMenu()
{
    windowMenu->clear();
    windowMenu->addAction(closeAct);
    windowMenu->addAction(closeAllAct);
    windowMenu->addSeparator();
    windowMenu->addAction(tileAct);
    windowMenu->addAction(cascadeAct);
    windowMenu->addSeparator();
    windowMenu->addAction(nextAct);
    windowMenu->addAction(previousAct);
    windowMenu->addAction(separatorAct);

    QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
    separatorAct->setVisible(!windows.isEmpty());

    for (int i = 0; i < windows.size(); ++i) {
        MdiChild *child = qobject_cast<MdiChild *>(windows.at(i)->widget());

        QString text;
        if (i < 9) {
            text = tr("&%1 %2").arg(i + 1)
                               .arg(child->userFriendlyCurrentFile());
        } else {
            text = tr("%1 %2").arg(i + 1)
                              .arg(child->userFriendlyCurrentFile());
        }
        QAction *action  = windowMenu->addAction(text);
        action->setCheckable(true);
        action ->setChecked(child == activeMdiChild());
        connect(action, SIGNAL(triggered()), windowMapper, SLOT(map()));
        windowMapper->setMapping(action, windows.at(i));
    }
}

MdiChild *MainWindow::createMdiChild()
{
    MdiChild *child = new MdiChild;
    mdiArea->addSubWindow(child);

#ifndef QT_NO_CLIPBOARD
    connect(child, SIGNAL(copyAvailable(bool)),
            cutAct, SLOT(setEnabled(bool)));
    connect(child, SIGNAL(copyAvailable(bool)),
            copyAct, SLOT(setEnabled(bool)));
#endif
    connect(child, SIGNAL(statusText(QString)), this, SLOT(updateStatus(QString)));
    connect(child, SIGNAL(menuUpdate()), this, SLOT(updateMenus()));

    return child;
}

void MainWindow::createActions()
{
    newAct = new QAction(QIcon(":/images/new.png"), tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new file"));
    connect(newAct, SIGNAL(triggered()), this, SLOT(newFile()));

    openAct = new QAction(QIcon(":/images/open.png"), tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    saveAct = new QAction(QIcon(":/images/save.png"), tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

    saveAsAct = new QAction(QIcon(":/images/save-as.png"), tr("Save &As..."), this);
    saveAsAct->setShortcuts(QKeySequence::SaveAs);
    saveAsAct->setStatusTip(tr("Save the document under a new name"));
    connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));

#ifndef QT_NO_CLIPBOARD
    cutAct = new QAction(QIcon(":/images/cut.png"), tr("Cu&t"), this);
    cutAct->setShortcuts(QKeySequence::Cut);
    cutAct->setStatusTip(tr("Cut the current selection's contents to the "
                            "clipboard"));
    connect(cutAct, SIGNAL(triggered()), this, SLOT(cut()));

    copyAct = new QAction(QIcon(":/images/copy.png"), tr("&Copy"), this);
    copyAct->setShortcuts(QKeySequence::Copy);
    copyAct->setStatusTip(tr("Copy the current selection's contents to the "
                             "clipboard"));
    connect(copyAct, SIGNAL(triggered()), this, SLOT(copy()));

    pasteAct = new QAction(QIcon(":/images/paste.png"), tr("&Paste"), this);
    pasteAct->setShortcuts(QKeySequence::Paste);
    pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current "
                              "selection"));
    connect(pasteAct, SIGNAL(triggered()), this, SLOT(paste()));
#endif

    closeAct = new QAction(QIcon(":/images/project-development-close.png"), tr("Cl&ose"), this);
    closeAct->setStatusTip(tr("Close the active window"));
    connect(closeAct, SIGNAL(triggered()),
            mdiArea, SLOT(closeActiveSubWindow()));

    closeAllAct = new QAction(QIcon(":/images/project-development-close-all.png"), tr("Close &All"), this);
    closeAllAct->setStatusTip(tr("Close all the windows"));
    connect(closeAllAct, SIGNAL(triggered()),
            mdiArea, SLOT(closeAllSubWindows()));

    tileAct = new QAction(QIcon(":/images/application-view-tile.png"), tr("&Tile"), this);
    tileAct->setStatusTip(tr("Tile the windows"));
    connect(tileAct, SIGNAL(triggered()), mdiArea, SLOT(tileSubWindows()));

    cascadeAct = new QAction(QIcon(":/images/application-cascade.png"), tr("&Cascade"), this);
    cascadeAct->setStatusTip(tr("Cascade the windows"));
    connect(cascadeAct, SIGNAL(triggered()), mdiArea, SLOT(cascadeSubWindows()));

    nextAct = new QAction(QIcon(":/images/project-development-next.png"), tr("Ne&xt"), this);
    nextAct->setShortcuts(QKeySequence::NextChild);
    nextAct->setStatusTip(tr("Move the focus to the next window"));
    connect(nextAct, SIGNAL(triggered()),
            mdiArea, SLOT(activateNextSubWindow()));

    previousAct = new QAction(QIcon(":/images/project-development-prev.png"), tr("Pre&vious"), this);
    previousAct->setShortcuts(QKeySequence::PreviousChild);
    previousAct->setStatusTip(tr("Move the focus to the previous "
                                 "window"));
    connect(previousAct, SIGNAL(triggered()),
            mdiArea, SLOT(activatePreviousSubWindow()));

    separatorAct = new QAction(this);
    separatorAct->setSeparator(true);

    aboutAct = new QAction(QIcon(":/Icon.iconset/icon_32x32.png"), tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

#ifdef SHOW_ABOUT_QT
    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
#endif

    zoomInAct = new QAction(QIcon(":/images/zoom-in-3.png"), tr("Zoom In"), this);
    zoomInAct->setShortcuts(QKeySequence::ZoomIn);
    zoomInAct->setStatusTip(tr("Zooms in by a factor of 2"));
    connect(zoomInAct, SIGNAL(triggered()), this, SLOT(zoomIn()));

    zoomOutAct = new QAction(QIcon(":/images/zoom-out-3.png"), tr("Zoom Out"), this);
    zoomOutAct->setShortcuts(QKeySequence::ZoomOut);
    zoomOutAct->setStatusTip(tr("Zooms out by a factor of 2"));
    connect(zoomOutAct, SIGNAL(triggered()), this, SLOT(zoomOut()));

    rotateLeftAct = new QAction(QIcon(":/images/rotateleft.png"), tr("Rotate Left"), this);
    rotateLeftAct->setStatusTip(tr("Rotates the image to the left (anticlockwise)"));
    connect(rotateLeftAct, SIGNAL(triggered()), this, SLOT(rotateLeft()));

    rotateRightAct = new QAction(QIcon(":/images/rotateright.png"), tr("Rotate Right"), this);
    rotateRightAct->setStatusTip(tr("Rotates the image to the right (clockwise)"));
    connect(rotateRightAct, SIGNAL(triggered()), this, SLOT(rotateRight()));

    nextChannelAct = new QAction(QIcon(":/images/go-next-view.png"), tr("Next Channel"), this);
    nextChannelAct->setShortcuts(QKeySequence::MoveToNextChar);
    nextChannelAct->setStatusTip(tr("Displays the next channel"));
    connect(nextChannelAct, SIGNAL(triggered()), this, SLOT(nextChannel()));

    prevChannelAct = new QAction(QIcon(":/images/go-previous-view.png"), tr("Previous Channel"), this);
    prevChannelAct->setShortcuts(QKeySequence::MoveToPreviousChar);
    prevChannelAct->setStatusTip(tr("Displays the previous channel"));
    connect(prevChannelAct, SIGNAL(triggered()), this, SLOT(prevChannel()));

    performRecipesAct = new QAction(QIcon(":/images/emblem-photos.png"), tr("Perform Recipes"), this);
    performRecipesAct->setStatusTip(tr("Uses an external program to process the selected images"));
    connect(performRecipesAct, SIGNAL(triggered()), this, SLOT(performRecipes()));

    setPrefsAct = new QAction(QIcon(":/images/preferences-desktop.png"), tr("&Preferences..."), this);
    setPrefsAct->setShortcuts(QKeySequence::Preferences);
    setPrefsAct->setStatusTip(tr("Set the allication preferences"));
    connect(setPrefsAct, SIGNAL(triggered()), this, SLOT(setPrefs()));

    importImagesAct = new QAction(QIcon(":/images/document-import-2.png"), tr("&Import Images..."), this);
    importImagesAct->setStatusTip(tr("Import multiple images into the current document"));
    connect(importImagesAct, SIGNAL(triggered()), this, SLOT(importImages()));

    exportImageAct = new QAction(QIcon(":/images/document-export-4.png"), tr("&Export Image..."), this);
    exportImageAct->setStatusTip(tr("Export current view as an 8 bit colour TIFF"));
    connect(exportImageAct, SIGNAL(triggered()), this, SLOT(exportImage()));

    pcaAct = new QAction(QIcon(":/images/pca_icon.png"), tr("PCA..."), this);
    pcaAct->setStatusTip(tr("Perform principle component analysis on selected images"));
    connect(pcaAct, SIGNAL(triggered()), this, SLOT(pca()));

    ldaAct = new QAction(QIcon(":/images/lda_icon.png"), tr("LDA..."), this);
    ldaAct->setStatusTip(tr("Perform linear discriminant analysis (aka canonical variates analysis) on selected images and points"));
    connect(ldaAct, SIGNAL(triggered()), this, SLOT(lda()));

    importHDF5Act = new QAction(QIcon(":/images/hdf_import.png"), tr("Import &HDF5 Image..."), this);
    importHDF5Act->setStatusTip(tr("Import multiple images into the current document from an HDF5 data set"));
    connect(importHDF5Act, SIGNAL(triggered()), this, SLOT(importHDF5Images()));
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(importImagesAct);
    fileMenu->addSeparator();
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addAction(exportImageAct);
    fileMenu->addSeparator();
    //QAction *action = fileMenu->addAction(tr("Switch layout direction"));
    //connect(action, SIGNAL(triggered()), this, SLOT(switchLayoutDirection()));
    fileMenu->addAction(exitAct);

    editMenu = menuBar()->addMenu(tr("&Edit"));
#ifndef QT_NO_CLIPBOARD
    editMenu->addAction(cutAct);
    editMenu->addAction(copyAct);
    editMenu->addAction(pasteAct);
#endif

    processMenu = menuBar()->addMenu(tr("&Process"));
    processMenu->addAction(pcaAct);
    processMenu->addSeparator();
    processMenu->addAction(ldaAct);
    processMenu->addSeparator();
    processMenu->addAction(importHDF5Act);
    //    processMenu->addAction(performRecipesAct);

    windowMenu = menuBar()->addMenu(tr("&Window"));
    updateWindowMenu();
    connect(windowMenu, SIGNAL(aboutToShow()), this, SLOT(updateWindowMenu()));

    menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
#ifdef SHOW_ABOUT_QT
    helpMenu->addAction(aboutQtAct);
#endif
    helpMenu->addAction(setPrefsAct);
}

void MainWindow::createToolBars()
{
    fileToolBar = addToolBar(tr("File"));
    fileToolBar->setObjectName("fileToolBar");
    fileToolBar->setIconSize(QSize(32, 32));
    fileToolBar->addAction(newAct);
    fileToolBar->addAction(openAct);
    fileToolBar->addAction(importImagesAct);
    fileToolBar->addAction(saveAct);
    fileToolBar->addAction(saveAsAct);
    fileToolBar->addAction(exportImageAct);

#ifndef QT_NO_CLIPBOARD
    editToolBar = addToolBar(tr("Edit"));
    editToolBar->setObjectName("editToolBar");
    editToolBar->setIconSize(QSize(32, 32));
    editToolBar->addAction(cutAct);
    editToolBar->addAction(copyAct);
    editToolBar->addAction(pasteAct);
#endif
    imageToolBar = addToolBar(tr("Image"));
    imageToolBar->setObjectName("imageToolBar");
    imageToolBar->setIconSize(QSize(32, 32));
    imageToolBar->addAction(zoomInAct);
    imageToolBar->addAction(zoomOutAct);
    imageToolBar->addAction(rotateLeftAct);
    imageToolBar->addAction(rotateRightAct);
    imageToolBar->addAction(prevChannelAct);
    imageToolBar->addAction(nextChannelAct);

    processToolBar = addToolBar(tr("Process"));
    processToolBar->setObjectName("processToolBar");
    processToolBar->setIconSize(QSize(32, 32));
    processToolBar->addAction(pcaAct);
    processToolBar->addSeparator();
    processToolBar->addAction(ldaAct);
    processToolBar->addSeparator();
    processToolBar->addAction(importHDF5Act);
//    processToolBar->addAction(performRecipesAct);
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::readSettings()
{
    if (Settings::value("_versionNumber", QString()).toString() != "0.0.1")
    {
        QMessageBox::warning(this, "Warning", "Incorrect Version Number found in settings file. Reverting to default.");
        PreferencesDialog::Import(":/data/DefaultSettings.xml");
    }
    restoreGeometry(Settings::value("_MainWindowGeometry", QByteArray()).toByteArray());
    restoreState(Settings::value("_MainWindowState", QByteArray()).toByteArray());
}

void MainWindow::writeSettings()
{
    Settings::setValue("_MainWindowGeometry", saveGeometry());
    Settings::setValue("_MainWindowState", saveState());
}

MdiChild *MainWindow::activeMdiChild()
{
    if (QMdiSubWindow *activeSubWindow = mdiArea->activeSubWindow())
        return qobject_cast<MdiChild *>(activeSubWindow->widget());
    return 0;
}

QMdiSubWindow *MainWindow::findMdiChild(const QString &fileName)
{
    QString absoluteFilePath = QDir::cleanPath(QFileInfo(fileName).absoluteFilePath());

    foreach (QMdiSubWindow *window, mdiArea->subWindowList()) {
        MdiChild *mdiChild = qobject_cast<MdiChild *>(window->widget());
        if (mdiChild->currentFile() == absoluteFilePath)
            return window;
    }
    return 0;
}

void MainWindow::switchLayoutDirection()
{
    if (layoutDirection() == Qt::LeftToRight)
        qApp->setLayoutDirection(Qt::RightToLeft);
    else
        qApp->setLayoutDirection(Qt::LeftToRight);
}

void MainWindow::setActiveSubWindow(QObject *window)
{
    if (QMdiSubWindow *subWindow = qobject_cast<QMdiSubWindow *>(window))
    {
        mdiArea->setActiveSubWindow(subWindow);
    }
}

void MainWindow::nextChannel()
{
    if (activeMdiChild())
        activeMdiChild()->nextChannel();
}

void MainWindow::prevChannel()
{
    if (activeMdiChild())
        activeMdiChild()->prevChannel();
}

void MainWindow::zoomIn()
{
    if (activeMdiChild())
        activeMdiChild()->zoomIn();
}

void MainWindow::zoomOut()
{
    if (activeMdiChild())
        activeMdiChild()->zoomOut();
}
void MainWindow::rotateLeft()
{
    if (activeMdiChild())
        activeMdiChild()->rotateLeft();
}

void MainWindow::rotateRight()
{
    if (activeMdiChild())
        activeMdiChild()->rotateRight();
}

void MainWindow::performRecipes()
{
    if (activeMdiChild())
        activeMdiChild()->performRecipes();
}

void MainWindow::pca()
{
    if (activeMdiChild())
        activeMdiChild()->pca();
}

void MainWindow::lda()
{
    if (activeMdiChild())
        activeMdiChild()->lda();
}

void MainWindow::setPrefs()
{
    PreferencesDialog dialogPreferences(this);

    int status = dialogPreferences.exec();
    if (status == QDialog::Accepted)
    {
        statusBar()->showMessage(tr("Preferences OK"));
        foreach (QMdiSubWindow *window, mdiArea->subWindowList())
        {
            MdiChild *mdiChild = qobject_cast<MdiChild *>(window->widget());
            mdiChild->preferencesUpdated();
        }
    }
}

void MainWindow::importImages()
{
    MdiChild *child = activeMdiChild(); // necessary because getOpenFileName can change activeMdiChild
    if (child)
    {
        QString lastImportedImageFile = Settings::value("_lastImportedImageFile",QString()).toString();
        QStringList files = QFileDialog::getOpenFileNames(this, tr("Select one or more files to open"), lastImportedImageFile,
                                                          tr("Images (*.tif *.tiff);;Any File (*.*)"), 0,
                                                          static_cast<QFileDialog::Options>(EXTRA_FILE_DIALOG_OPTIONS));
        if (files.size())
        {
            Settings::setValue("_lastImportedImageFile", files[0]);
            for (int i = 0; i < files.size(); i++)
            {
                child->importImage(files[i]);
            }
            child->displayOneImageIfNecessary();
        }
    }
}

void MainWindow::importHDF5Images()
{
    if (activeMdiChild())
        activeMdiChild()->importHDF5();
}

void MainWindow::exportImage()
{
    MdiChild *child = activeMdiChild(); // necessary because getOpenFileName can change activeMdiChild
    if (child)
    {
        QString lastExportedImageFile = Settings::value("_lastExportedImageFile",QString()).toString();
        QString filename = QFileDialog::getSaveFileName(this, tr("Export current display to 8 bit colour TIFF file"), lastExportedImageFile,
                                                          tr("Images (*.tif *.tiff;;Any File (*.*)"), 0,
                                                          static_cast<QFileDialog::Options>(EXTRA_FILE_DIALOG_OPTIONS));
        if (!filename.isEmpty())
        {
            Settings::setValue("_lastExportedImageFile", filename);
            child->exportImage(filename);
        }
    }
}

void MainWindow::updateStatus(QString status)
{
    statusBar()->showMessage(status);
}

//void MainWindow::ticker()
//{
//    MdiChild *child = activeMdiChild(); // necessary because getOpenFileName can change activeMdiChild
//    pcaAct->setEnabled(child && child->numSelectedImages() >= 2);
//    ldaAct->setEnabled(child && child->numSelectedImages() >= 2 && child->numSelectedPoints() >= 2);
//    if (child) child->ticker();
//}

