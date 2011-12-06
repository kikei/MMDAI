/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 2010-2011  hkrn                                    */
/*                                                                   */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/* - Redistributions of source code must retain the above copyright  */
/*   notice, this list of conditions and the following disclaimer.   */
/* - Redistributions in binary form must reproduce the above         */
/*   copyright notice, this list of conditions and the following     */
/*   disclaimer in the documentation and/or other materials provided */
/*   with the distribution.                                          */
/* - Neither the name of the MMDAI project team nor the names of     */
/*   its contributors may be used to endorse or promote products     */
/*   derived from this software without specific prior written       */
/*   permission.                                                     */
/*                                                                   */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND            */
/* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,       */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF          */
/* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS */
/* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   */
/* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    */
/* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           */
/* POSSIBILITY OF SUCH DAMAGE.                                       */
/* ----------------------------------------------------------------- */

#include "MainWindow.h"

#include "ExtendedSceneWidget.h"
#include "LicenseWidget.h"
#include "LoggerWidget.h"
#include "Script.h"
#include "util.h"

#include <QtGui/QtGui>
#include <vpvl/vpvl.h>

namespace
{

void ConstructScriptArguments(const QString &input, QString &command, QList<QVariant> &arguments)
{
    QStringList strings = QString(input).split("|");
    command = strings.first();
    strings.pop_front();
    foreach (QString string, strings)
        arguments << string;
}

}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_settings(QSettings::IniFormat, QSettings::UserScope, qApp->organizationName(), qAppName()),
    m_licenseWidget(0),
    m_loggerWidget(0),
    m_sceneWidget(0),
    m_model(0),
    m_currentFPS(0)
{
    m_licenseWidget = new LicenseWidget();
    m_loggerWidget = new LoggerWidget(&m_settings);
    m_sceneWidget = new ExtendedSceneWidget(&m_settings);
    resize(900, 720);
    setMinimumSize(QSize(640, 480));
    setCentralWidget(m_sceneWidget);
    buildMenuBar();
    restoreGeometry(m_settings.value("mainWindow/geometry").toByteArray());
    restoreState(m_settings.value("mainWindow/state").toByteArray());
    setWindowTitle(buildWindowTitle());
    statusBar()->show();
}

MainWindow::~MainWindow()
{
    delete m_menuBar;
    delete m_loggerWidget;
    delete m_licenseWidget;
}

bool MainWindow::validateLibraryVersion()
{
    if (!vpvl::isLibraryVersionCorrect(VPVL_VERSION)) {
        QMessageBox::warning(this,
                             tr("libvpvl version mismatch"),
                             tr("libvpvl's version is incorrect (expected: %1 actual: %2).\n"
                                "Please replace libvpvl to correct version or reinstall MMDAI.")
                             .arg(VPVL_VERSION_STRING).arg(vpvl::libraryVersionString()));
        return false;
    }
    return true;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_settings.setValue("mainWindow/geometry", saveGeometry());
    m_settings.setValue("mainWindow/state", saveState());
    event->accept();
}

void MainWindow::selectCurrentModel()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {
        vpvl::PMDModel *model = m_sceneWidget->findModel(action->text());
        m_sceneWidget->setSelectedModel(model);
    }
}

void MainWindow::setCurrentModel(vpvl::PMDModel *value)
{
    m_model = value;
    updateInformation();
}

void MainWindow::addModel(vpvl::PMDModel *model)
{
    QString name = internal::toQString(model);
    QAction *action = new QAction(name, this);
    action->setStatusTip(tr("Select a model %1").arg(name));
    connect(action, SIGNAL(triggered()), this, SLOT(selectCurrentModel()));
    m_menuRetainModels->addAction(action);
}

void MainWindow::deleteModel(vpvl::PMDModel *model)
{
    QAction *actionToRemove = 0;
    QString name = internal::toQString(model);
    foreach (QAction *action, m_menuRetainModels->actions()) {
        if (action->text() == name) {
            actionToRemove = action;
            break;
        }
    }
    if (actionToRemove)
        m_menuRetainModels->removeAction(actionToRemove);
}


void MainWindow::addAsset(vpvl::Asset *asset)
{
    QString name(asset->name());
    QAction *action = new QAction(name, this);
    action->setStatusTip(tr("Select an asset %1").arg(name));
    //connect(action, SIGNAL(triggered()), this, SLOT(selectCurrentModel()));
    m_menuRetainAssets->addAction(action);
}

void MainWindow::deleteAsset(vpvl::Asset *asset)
{
    QAction *actionToRemove = 0;
    QString name(asset->name());
    foreach (QAction *action, m_menuRetainAssets->actions()) {
        if (action->text() == name) {
            actionToRemove = action;
            break;
        }
    }
    if (actionToRemove)
        m_menuRetainAssets->removeAction(actionToRemove);
}

void MainWindow::updateInformation()
{
    setWindowTitle(buildWindowTitle());
}

void MainWindow::connectWidgets()
{
}

void MainWindow::startSceneUpdate()
{
    setWindowTitle(buildWindowTitle(0));
}

void MainWindow::stopSceneUpdate()
{
    setWindowTitle(buildWindowTitle());
}

void MainWindow::updateFPS(int fps)
{
    setWindowTitle(buildWindowTitle(fps));
}

void MainWindow::executeCommand()
{
    QString input = QInputDialog::getText(this, tr("Execute command"), tr("Execute command")), command;
    QList<QVariant> arguments;
    ConstructScriptArguments(input, command, arguments);
    Script *script = m_sceneWidget->script();
    if (script)
        script->handleCommand(command, arguments);
}

void MainWindow::executeEvent()
{
    QString input = QInputDialog::getText(this, tr("Execute event"), tr("Execute event")), event;
    QList<QVariant> arguments;
    ConstructScriptArguments(input, event, arguments);
    Script *script = m_sceneWidget->script();
    if (script)
        script->handleEvent(event, arguments);
}

const QString MainWindow::buildWindowTitle()
{
    QString title = qAppName();
    if (m_model)
        title += " - " + internal::toQString(m_model);
    return title;
}

const QString MainWindow::buildWindowTitle(int fps)
{
    return buildWindowTitle() + tr(" (FPS: %1)").arg(fps);
}

void MainWindow::buildMenuBar()
{
    m_actionAddModel = new QAction(this);
    connect(m_actionAddModel, SIGNAL(triggered()), m_sceneWidget, SLOT(addModel()));
    m_actionAddAsset = new QAction(this);
    connect(m_actionAddAsset, SIGNAL(triggered()), m_sceneWidget, SLOT(addAsset()));
    m_actionInsertToAllModels = new QAction(this);
    connect(m_actionInsertToAllModels, SIGNAL(triggered()), m_sceneWidget, SLOT(insertMotionToAllModels()));
    m_actionInsertToSelectedModel = new QAction(this);
    connect(m_actionInsertToSelectedModel, SIGNAL(triggered()), m_sceneWidget, SLOT(insertMotionToSelectedModel()));
    m_actionSetCamera = new QAction(this);
    connect(m_actionSetCamera, SIGNAL(triggered()), m_sceneWidget, SLOT(setCamera()));
    m_actionExit = new QAction(this);
    m_actionExit->setMenuRole(QAction::QuitRole);
    connect(m_actionExit, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));

    m_actionLoadScript = new QAction(this);
    connect(m_actionLoadScript, SIGNAL(triggered()), m_sceneWidget, SLOT(loadScript()));
    m_actionPlay = new QAction(this);
    connect(m_actionPlay, SIGNAL(triggered()), m_sceneWidget, SLOT(play()));
    m_actionPause = new QAction(this);
    connect(m_actionPause, SIGNAL(triggered()), m_sceneWidget, SLOT(pause()));
    m_actionStop = new QAction(this);
    connect(m_actionStop, SIGNAL(triggered()), m_sceneWidget, SLOT(stop()));
    m_actionShowLogMessage = new QAction(this);
    connect(m_actionShowLogMessage, SIGNAL(triggered()), m_loggerWidget, SLOT(show()));
    m_actionExecuteCommand = new QAction(this);
    connect(m_actionExecuteCommand, SIGNAL(triggered()), this, SLOT(executeCommand()));
    m_actionExecuteEvent = new QAction(this);
    connect(m_actionExecuteEvent, SIGNAL(triggered()), this, SLOT(executeEvent()));

    m_actionZoomIn = new QAction(this);
    connect(m_actionZoomIn, SIGNAL(triggered()), m_sceneWidget, SLOT(zoomIn()));
    m_actionZoomOut = new QAction(this);
    connect(m_actionZoomOut, SIGNAL(triggered()), m_sceneWidget, SLOT(zoomOut()));
    m_actionRotateUp = new QAction(this);
    connect(m_actionRotateUp, SIGNAL(triggered()), m_sceneWidget, SLOT(rotateUp()));
    m_actionRotateDown = new QAction(this);
    connect(m_actionRotateDown, SIGNAL(triggered()), m_sceneWidget, SLOT(rotateDown()));
    m_actionRotateLeft = new QAction(this);
    connect(m_actionRotateLeft, SIGNAL(triggered()), m_sceneWidget, SLOT(rotateLeft()));
    m_actionRotateRight = new QAction(this);
    connect(m_actionRotateRight, SIGNAL(triggered()), m_sceneWidget, SLOT(rotateRight()));
    m_actionTranslateUp = new QAction(this);
    connect(m_actionTranslateUp, SIGNAL(triggered()), m_sceneWidget, SLOT(translateUp()));
    m_actionTranslateDown = new QAction(this);
    connect(m_actionTranslateDown, SIGNAL(triggered()), m_sceneWidget, SLOT(translateDown()));
    m_actionTranslateLeft = new QAction(this);
    connect(m_actionTranslateLeft, SIGNAL(triggered()), m_sceneWidget, SLOT(translateLeft()));
    m_actionTranslateRight = new QAction(this);
    connect(m_actionTranslateRight, SIGNAL(triggered()), m_sceneWidget, SLOT(translateRight()));
    m_actionResetCamera = new QAction(this);
    connect(m_actionResetCamera, SIGNAL(triggered()), m_sceneWidget, SLOT(resetCamera()));
    m_actionShowBones = new QAction(this);

    m_actionRevertSelectedModel = new QAction(this);
    connect(m_actionRevertSelectedModel, SIGNAL(triggered()), m_sceneWidget, SLOT(revertSelectedModel()));
    m_actionDeleteSelectedModel = new QAction(this);
    connect(m_actionDeleteSelectedModel, SIGNAL(triggered()), m_sceneWidget, SLOT(deleteSelectedModel()));

    m_actionAbout = new QAction(this);
    connect(m_actionAbout, SIGNAL(triggered()), m_licenseWidget, SLOT(show()));
    m_actionAbout->setMenuRole(QAction::AboutRole);
    m_actionAboutQt = new QAction(this);
    connect(m_actionAboutQt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    m_actionAboutQt->setMenuRole(QAction::AboutQtRole);

#ifdef Q_OS_MACX
    m_menuBar = new QMenuBar(0);
#else
    m_menuBar = menuBar();
#endif
    m_menuFile = new QMenu(this);
    m_menuFile->addAction(m_actionLoadScript);
    m_menuFile->addSeparator();
    m_menuFile->addAction(m_actionAddModel);
    m_menuFile->addAction(m_actionAddAsset);
    m_menuFile->addSeparator();
    m_menuFile->addAction(m_actionInsertToAllModels);
    m_menuFile->addAction(m_actionInsertToSelectedModel);
    m_menuFile->addAction(m_actionSetCamera);
    m_menuFile->addSeparator();
    m_menuFile->addAction(m_actionExit);
    m_menuBar->addMenu(m_menuFile);
    m_menuScript = new QMenu(this);
    m_menuScript->addAction(m_actionPlay);
    m_menuScript->addAction(m_actionPause);
    m_menuScript->addAction(m_actionStop);
    m_menuScript->addSeparator();
    m_menuScript->addAction(m_actionShowLogMessage);
    m_menuScript->addSeparator();
    m_menuScript->addAction(m_actionExecuteCommand);
    m_menuScript->addAction(m_actionExecuteEvent);
    m_menuBar->addMenu(m_menuScript);
    m_menuScene = new QMenu(this);
    m_menuScene->addAction(m_actionZoomIn);
    m_menuScene->addAction(m_actionZoomOut);
    m_menuScene->addSeparator();
    m_menuScene->addAction(m_actionRotateUp);
    m_menuScene->addAction(m_actionRotateDown);
    m_menuScene->addAction(m_actionRotateLeft);
    m_menuScene->addAction(m_actionRotateRight);
    m_menuScene->addSeparator();
    m_menuScene->addAction(m_actionTranslateUp);
    m_menuScene->addAction(m_actionTranslateDown);
    m_menuScene->addAction(m_actionTranslateLeft);
    m_menuScene->addAction(m_actionTranslateRight);
    m_menuScene->addSeparator();
    m_menuScene->addAction(m_actionResetCamera);
    m_menuBar->addMenu(m_menuScene);
    m_menuModel = new QMenu(this);
    m_menuRetainModels = new QMenu(this);
    m_menuModel->addMenu(m_menuRetainModels);
    m_menuRetainAssets = new QMenu(this);
    if (vpvl::Asset::isSupported())
        m_menuScene->addMenu(m_menuRetainAssets);
    m_menuModel->addAction(m_actionRevertSelectedModel);
    m_menuModel->addAction(m_actionDeleteSelectedModel);
    m_menuBar->addMenu(m_menuModel);
    m_menuHelp = new QMenu(this);
    m_menuHelp->addAction(m_actionAbout);
    m_menuHelp->addAction(m_actionAboutQt);
    m_menuBar->addMenu(m_menuHelp);

    connect(m_sceneWidget, SIGNAL(modelDidAdd(vpvl::PMDModel*)), this, SLOT(addModel(vpvl::PMDModel*)));
    connect(m_sceneWidget, SIGNAL(modelWillDelete(vpvl::PMDModel*)), this, SLOT(deleteModel(vpvl::PMDModel*)));
    connect(m_sceneWidget, SIGNAL(modelDidSelect(vpvl::PMDModel*)), this, SLOT(setCurrentModel(vpvl::PMDModel*)));
    connect(m_sceneWidget, SIGNAL(assetDidAdd(vpvl::Asset*)), this, SLOT(addAsset(vpvl::Asset*)));
    connect(m_sceneWidget, SIGNAL(assetWillDelete(vpvl::Asset*)), this, SLOT(deleteAsset(vpvl::Asset*)));
    connect(m_sceneWidget, SIGNAL(fpsDidUpdate(int)), this, SLOT(updateFPS(int)));

    retranslate();
}

void MainWindow::retranslate()
{
    m_actionLoadScript->setText(tr("Load script"));
    m_actionLoadScript->setToolTip(tr("Load a script"));
    m_actionLoadScript->setShortcut(tr("Ctrl+Shift+S"));
    m_actionAddModel->setText(tr("Add model"));
    m_actionAddModel->setToolTip(tr("Add a model to the scene."));
    m_actionAddModel->setShortcut(tr("Ctrl+Shift+M"));
    m_actionAddAsset->setText(tr("Add asset"));
    m_actionAddAsset->setToolTip(tr("Add an asset to the scene."));
    m_actionAddAsset->setShortcut(tr("Ctrl+Shift+A"));
    m_actionAddAsset->setEnabled(vpvl::Asset::isSupported());
    m_actionInsertToAllModels->setText(tr("Insert to all models"));
    m_actionInsertToAllModels->setToolTip(tr("Insert a motion to the all models."));
    m_actionInsertToAllModels->setShortcut(tr("Ctrl+Shift+V"));
    m_actionInsertToSelectedModel->setText(tr("Insert to selected model"));
    m_actionInsertToSelectedModel->setToolTip(tr("Insert a motion to the selected model."));
    m_actionInsertToSelectedModel->setShortcut(tr("Ctrl+Alt+Shift+V"));
    m_actionSetCamera->setText(tr("Set camera motion"));
    m_actionSetCamera->setToolTip(tr("Set a camera motion to the scene."));
    m_actionSetCamera->setShortcut(tr("Ctrl+Shift+C"));
    m_actionExit->setText(tr("Exit"));
    m_actionExit->setToolTip(tr("Exit this application."));
    m_actionExit->setShortcut(tr("Ctrl+Q"));
    m_actionPlay->setText(tr("Play"));
    m_actionPlay->setStatusTip(tr("Play current scene."));
    m_actionPause->setText(tr("Pause"));
    m_actionPause->setStatusTip(tr("Pause current scene."));
    m_actionStop->setText(tr("Stop"));
    m_actionStop->setStatusTip(tr("Stop current scene."));
    m_actionShowLogMessage->setText(tr("Open log message window"));
    m_actionShowLogMessage->setToolTip(tr("Open a window of log messages such as script."));
    m_actionExecuteCommand->setText(tr("Execute command"));
    m_actionExecuteCommand->setStatusTip(tr("Execute command to the script."));
    m_actionExecuteEvent->setText(tr("Execute event"));
    m_actionExecuteEvent->setStatusTip(tr("Execute event to the script."));
    m_actionZoomIn->setText(tr("Zoom in"));
    m_actionZoomIn->setStatusTip(tr("Zoom in the scene."));
    m_actionZoomIn->setShortcut(tr("+"));
    m_actionZoomOut->setText(tr("Zoom out"));
    m_actionZoomOut->setStatusTip(tr("Zoom out the scene."));
    m_actionZoomOut->setShortcut(tr("-"));
    m_actionRotateUp->setText(tr("Rotate up"));
    m_actionRotateUp->setStatusTip(tr("Rotate up the scene."));
    m_actionRotateUp->setShortcut(tr("Ctrl+Up"));
    m_actionRotateDown->setText(tr("Rotate down"));
    m_actionRotateDown->setStatusTip(tr("Rotate down the scene."));
    m_actionRotateDown->setShortcut(tr("Ctrl+Down"));
    m_actionRotateLeft->setText(tr("Rotate left"));
    m_actionRotateLeft->setStatusTip(tr("Rotate left the scene."));
    m_actionRotateLeft->setShortcut(tr("Ctrl+Left"));
    m_actionRotateRight->setText(tr("Rotate right"));
    m_actionRotateRight->setStatusTip(tr("Rotate right the scene."));
    m_actionRotateRight->setShortcut(tr("Ctrl+Right"));
    m_actionTranslateUp->setText(tr("Translate up"));
    m_actionTranslateUp->setStatusTip(tr("Translate up the scene."));
    m_actionTranslateUp->setShortcut(tr("Shift+Up"));
    m_actionTranslateDown->setText(tr("Translate down"));
    m_actionTranslateDown->setStatusTip(tr("Translate down the scene."));
    m_actionTranslateDown->setShortcut(tr("Shift+Down"));
    m_actionTranslateLeft->setText(tr("Translate left"));
    m_actionTranslateLeft->setStatusTip(tr("Translate left the scene."));
    m_actionTranslateLeft->setShortcut(tr("Shift+Left"));
    m_actionTranslateRight->setText(tr("Translate right"));
    m_actionTranslateRight->setStatusTip(tr("Translate right the scene."));
    m_actionTranslateRight->setShortcut(tr("Shift+Right"));
    m_actionResetCamera->setText(tr("Reset camera"));
    m_actionResetCamera->setStatusTip(tr("Reset camera perspective."));
    m_actionRevertSelectedModel->setText(tr("Revert selected model"));
    m_actionRevertSelectedModel->setStatusTip(tr("Revert the selected model."));
    m_actionDeleteSelectedModel->setText(tr("Delete selected model"));
    m_actionDeleteSelectedModel->setStatusTip(tr("Delete the selected model from the scene."));
    m_actionDeleteSelectedModel->setShortcut(tr("Ctrl+Shift+Backspace"));
    m_actionAbout->setText(tr("About"));
    m_actionAbout->setStatusTip(tr("About this application."));
    m_actionAbout->setShortcut(tr("Alt+Q, Alt+/"));
    m_actionAboutQt->setText(tr("About Qt"));
    m_actionAboutQt->setStatusTip(tr("About Qt."));
    m_menuFile->setTitle(tr("&File"));
    m_menuScript->setTitle(tr("Script"));
    m_menuScene->setTitle(tr("&Scene"));
    m_menuModel->setTitle(tr("&Model"));
    m_menuRetainAssets->setTitle(tr("Select asset"));
    m_menuRetainModels->setTitle(tr("Select model"));
    m_menuHelp->setTitle(tr("&Help"));
}
