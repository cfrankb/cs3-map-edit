#include "mainwindow.h"
#include "dlgabout.h"
#include "ui_mainwindow.h"
#include <QDockWidget>
#include <QShortcut>
#include <QKeySequence>
#include <QMessageBox>
#include <QCloseEvent>
#include <QFileDialog>
#include <QSettings>
#include <QScrollBar>
#include <QInputDialog>
#include "mapscroll.h"
#include "mapwidget.h"
#include "dlgattr.h"
#include "dlgresize.h"
#include "dlgselect.h"
#include "dlgtest.h"
#include "tilebox.h"
#include "tilesdata.h"
#include "dlgstat.h"
#include "map.h"
#include "report.h"
#include "keyvaluedialog.h"
#include "statedata.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_scrollArea = new CMapScroll(this);
    m_scrollArea->viewport()->update();
    CMapWidget *glw = dynamic_cast<CMapWidget *>(m_scrollArea->viewport());
    glw->setMap(m_doc.map());
    connect(ui->actionView_Grid, SIGNAL(toggled(bool)), glw, SLOT(showGrid(bool)));
    connect(ui->actionView_Animate, SIGNAL(toggled(bool)), glw, SLOT(setAnimate(bool)));
    setCentralWidget(m_scrollArea);

    connect(m_scrollArea, SIGNAL(statusChanged(QString)), this, SLOT(setStatus(QString)));
    connect(m_scrollArea, SIGNAL(leftClickedAt(int, int)), this, SLOT(onLeftClick(int, int)));
    connect(this, SIGNAL(mapChanged(CMap *)), m_scrollArea, SLOT(newMap(CMap *)));
    connect(m_scrollArea, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showContextMenu(const QPoint &)));

    updateTitle();
    initTilebox();
    initFileMenu();
    initMapShortcuts();
    initToolBar();
    updateMenus();
    setWindowIcon(QIcon(":/data/icons/CS3MapEdit-icon.png"));
}

void MainWindow::shiftUp()
{
    m_doc.map()->shift(CMap::UP);
    m_doc.setDirty(true);
}

void MainWindow::shiftDown()
{
    m_doc.map()->shift(CMap::DOWN);
    m_doc.setDirty(true);
}

void MainWindow::shiftLeft()
{
    m_doc.map()->shift(CMap::LEFT);
    m_doc.setDirty(true);
}

void MainWindow::shiftRight()
{
    m_doc.map()->shift(CMap::RIGHT);
    m_doc.setDirty(true);
}

void MainWindow::initMapShortcuts()
{
    connect(this, SIGNAL(resizeMap(int, int)), m_scrollArea, SLOT(newMapSize(int, int)));
    emit resizeMap(m_doc.map()->len(), m_doc.map()->hei());
    new QShortcut(QKeySequence(QKeyCombination(Qt::CTRL, Qt::Key_Up)), this, SLOT(shiftUp()));
    new QShortcut(QKeySequence(QKeyCombination(Qt::CTRL, Qt::Key_Down)), this, SLOT(shiftDown()));
    new QShortcut(QKeySequence(QKeyCombination(Qt::CTRL, Qt::Key_Left)), this, SLOT(shiftLeft()));
    new QShortcut(QKeySequence(QKeyCombination(Qt::CTRL, Qt::Key_Right)), this, SLOT(shiftRight()));
}

void MainWindow::initTilebox()
{
    auto dock = new QDockWidget();
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    dock->setWindowTitle(tr("Toolbox"));
    auto tilebox = new CTileBox(dock);
    tilebox->show();
    dock->setWidget(tilebox);
    addDockWidget(Qt::LeftDockWidgetArea, dock);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea);
    connect(tilebox, SIGNAL(tileChanged(int)),
            this, SLOT(changeTile(int)));
    connect(this, SIGNAL(newTile(int)),
            tilebox, SLOT(setTile(int)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave())
    {
        event->accept();
    }
    else
    {
        event->ignore();
        return;
    }
}

bool MainWindow::isDirty()
{
    return m_doc.isDirty();
}

bool MainWindow::maybeSave()
{
    if (isDirty())
    {
        QMessageBox::StandardButton ret = QMessageBox::warning(
            this,
            m_appName,
            tr("The document has been modified.\n"
               "Do you want to save your changes?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (ret == QMessageBox::Save)
            return save();
        else if (ret == QMessageBox::Cancel)
            return false;
    }
    return true;
}

void MainWindow::open(QString fileName)
{
    if (maybeSave())
    {
        if (fileName.isEmpty())
        {
            QStringList filters;
            filters.append(m_allFilter);
            QFileDialog *dlg = new QFileDialog(this, tr("Open"), "", m_allFilter);
            dlg->setAcceptMode(QFileDialog::AcceptOpen);
            dlg->setFileMode(QFileDialog::ExistingFile);
            dlg->selectFile(m_doc.filename());
            dlg->setNameFilters(filters);
            if (dlg->exec())
            {
                QStringList fileNames = dlg->selectedFiles();
                if (fileNames.count() > 0)
                {
                    fileName = fileNames[0];
                }
            }
            delete dlg;
        }
        loadFile(fileName);
    }
    updateMenus();
}

void MainWindow::loadFile(const QString &fileName)
{
    if (!fileName.isEmpty())
    {
        QString oldFileName = m_doc.filename();
        m_doc.setFilename(fileName);
        if (m_doc.read())
        {
            qDebug("size: %d", m_doc.size());
        }
        else
        {
            warningMessage(tr("error:\n") + m_doc.lastError());
            m_doc.setFilename(oldFileName);
            // update fileList
            QSettings settings;
            QStringList files = settings.value("recentFileList").toStringList();
            files.removeAll(fileName);
            settings.setValue("recentFileList", files);
        }
        updateTitle();
        updateRecentFileActions();
        reloadRecentFileActions();
        emit mapChanged(m_doc.map());
    }
}

bool MainWindow::save()
{
    QString oldFileName = m_doc.filename();
    if (m_doc.isUntitled() || m_doc.isWrongExt())
    {
        if (!saveAs())
            return false;
    }

    if (!m_doc.write() || !updateTitle())
    {
        warningMessage(tr("Can't write file"));
        m_doc.setFilename(oldFileName);
        return false;
    }

    updateRecentFileActions();
    reloadRecentFileActions();
    return true;
}

bool MainWindow::saveAs()
{
    bool result = false;
    QStringList filters;
    QString suffix = m_doc.isMulti() ? "mapz" : "dat";
    QString fileName = "";

    QFileDialog *dlg = new QFileDialog(this, tr("Save as"), "", m_allFilter);
    dlg->setAcceptMode(QFileDialog::AcceptSave);

    dlg->setNameFilters(filters);
    dlg->setAcceptMode(QFileDialog::AcceptSave);
    dlg->setDefaultSuffix(suffix);
    dlg->selectFile(m_doc.filename());
    if (dlg->exec())
    {
        QStringList fileNames = dlg->selectedFiles();
        if (fileNames.count() > 0)
        {
            fileName = fileNames[0];
        }
    }

    if (!fileName.isEmpty())
    {
        m_doc.setFilename(fileName);
        result = m_doc.write();
    }

    updateTitle();
    delete dlg;
    return result;
}

void MainWindow::warningMessage(const QString message)
{
    QMessageBox::warning(this, m_appName, message);
}

void MainWindow::setDocument(const QString fileName)
{
    m_doc.setFilename(fileName);
    m_doc.read();
}

bool MainWindow::updateTitle()
{
    QString file;
    if (m_doc.filename().isEmpty())
    {
        file = tr("untitled");
    }
    else
    {
        file = QFileInfo(m_doc.filename()).fileName();
    }
    m_doc.setDirty(false);
    setWindowTitle(tr("%1[*] - %2").arg(file, m_appName));
    return true;
}

void MainWindow::updateMenus()
{
    int index = m_doc.currentIndex();
    ui->actionEdit_Previous_Map->setEnabled(index > 0);
    ui->actionEdit_Next_Map->setEnabled(index < m_doc.size() - 1);
    ui->actionEdit_Delete_Map->setEnabled(m_doc.size() > 1);
    ui->actionEdit_Move_Map->setEnabled(m_doc.size() > 1);
    ui->actionEdit_Goto_Map->setEnabled(m_doc.size() > 1);
    ui->actionEdit_First_Map->setEnabled(index > 0);
    ui->actionEdit_Last_Map->setEnabled(index < m_doc.size() - 1);
}

void MainWindow::setStatus(const QString msg)
{
    ui->statusbar->showMessage(msg);
}

void MainWindow::on_actionFile_New_File_triggered()
{
    if (maybeSave())
    {
        m_doc.setFilename("");
        m_doc.forget();
        CMap *map = new CMap(40, 40);
        m_doc.add(map);
        updateTitle();
        emit mapChanged(m_doc.map());
        updateMenus();
    }
}

void MainWindow::on_actionFile_Open_triggered()
{
    open("");
}

void MainWindow::on_actionFile_Save_triggered()
{
    save();
}

void MainWindow::on_actionFile_Save_as_triggered()
{
    saveAs();
    updateRecentFileActions();
    reloadRecentFileActions();
}

void MainWindow::on_actionEdit_ResizeMap_triggered()
{
    CMap &map = *m_doc.map();
    CDlgResize dlg(this);
    dlg.setWindowTitle(tr("Resize Map"));
    dlg.width(map.len());
    dlg.height(map.hei());
    if (dlg.exec() == QDialog::Accepted)
    {
        QMessageBox::StandardButton reply = QMessageBox::Yes;
        if (dlg.width() < map.len() || dlg.height() < map.hei())
        {
            reply = QMessageBox::warning(
                this, m_appName, tr("Resizing map may lead to data lost. Continue?"),
                QMessageBox::Yes | QMessageBox::No);
        }
        if (reply == QMessageBox::Yes)
        {
            map.resize(dlg.width(), dlg.height(), false);
            m_doc.setDirty(true);
            emit resizeMap(m_doc.map()->len(), m_doc.map()->hei());
        }
    }
}

void MainWindow::showContextMenu(const QPoint &pos)
{
    int x = pos.x() / GRID_SIZE;
    int y = pos.y() / GRID_SIZE;
    int mx = m_scrollArea->horizontalScrollBar()->value();
    int my = m_scrollArea->verticalScrollBar()->value();

    CMap &map = *m_doc.map();
    if (x >= 0 && y >= 0 && x + mx < map.len() && y + my < map.hei())
    {
        QMenu menu(this);
        QAction *actionSetAttr = new QAction(tr("set attribute"), &menu);
        connect(actionSetAttr, SIGNAL(triggered()),
                this, SLOT(showAttrDialog()));
        actionSetAttr->setStatusTip(tr("Set the attribute for this tile"));
        menu.addAction(actionSetAttr);

        QAction *actionStatAttr = new QAction(tr("see tile stats"), &menu);
        connect(actionStatAttr, SIGNAL(triggered()),
                this, SLOT(showStatDialog()));
        menu.addAction(actionStatAttr);
        actionStatAttr->setStatusTip(tr("Show the data information on this tile"));
        menu.addSeparator();

        QAction *actionSetStartPos = new QAction(tr("set start pos"), &menu);
        connect(actionSetStartPos, SIGNAL(triggered()),
                this, SLOT(on_setStartPos()));
        actionSetStartPos->setStatusTip(tr("Set the start position for this map"));
        menu.addAction(actionSetStartPos);

        QAction *actionSetExitPos = new QAction(tr("set exit pos"), &menu);
        connect(actionSetExitPos, SIGNAL(triggered()),
                this, SLOT(on_setExitPos()));
        actionSetExitPos->setStatusTip(tr("Set the exit position for this map."));
        menu.addAction(actionSetExitPos);
        menu.addSeparator();

        QAction *actionDeleteTile = new QAction(tr("delete tile"), &menu);
        connect(actionDeleteTile, SIGNAL(triggered()),
                this, SLOT(on_deleteTile()));
        menu.addAction(actionDeleteTile);
        actionDeleteTile->setStatusTip(tr("delete this tile"));

        m_hx = x + mx;
        m_hy = y + my;
        menu.exec(m_scrollArea->mapToGlobal(pos));
    }
}

void MainWindow::on_deleteTile()
{
    CMap &map = *m_doc.map();
    if (map.at(m_hx, m_hy) != TILES_BLANK) {
        map.set(m_hx, m_hy, TILES_BLANK);
        m_doc.setDirty(true);
    }
}

void MainWindow::on_setStartPos()
{
    CMap &map = *m_doc.map();
    map.states().setU(POS_ORIGIN, CMap::toKey(m_hx, m_hy));
    m_doc.setDirty(true);
}

void MainWindow::on_setExitPos()
{
    CMap &map = *m_doc.map();
    map.states().setU(POS_EXIT, CMap::toKey(m_hx, m_hy));
    m_doc.setDirty(true);
}

void MainWindow::showAttrDialog()
{
    CMap &map = *m_doc.map();
    uint8_t a = map.getAttr(m_hx, m_hy);
    CDlgAttr dlg(this);
    dlg.attr(a);
    if (dlg.exec() == QDialog::Accepted)
    {
        a = dlg.attr();
        map.setAttr(m_hx, m_hy, a);
        m_doc.setDirty(true);
    }
}

void MainWindow::showStatDialog()
{
    CMap &map = *m_doc.map();
    CDlgStat dlg(map.at(m_hx, m_hy), this);
    dlg.setWindowTitle(tr("Tile Statistics"));
    dlg.exec();
}

void MainWindow::changeTile(int tile)
{
    m_currTile = tile;
    ui->actionTools_Paint->setChecked(true);
}

void MainWindow::onLeftClick(int x, int y)
{
    if ((x >= 0) && (y >= 0) && (x < m_doc.map()->len()) && (y < m_doc.map()->hei()))
    {
        const uint8_t tile = m_doc.map()->at(x, y);
        uint8_t newTileId = tile;
        switch (currentTool())
        {
        case TOOL_PAINT:
            newTileId = m_currTile;
            break;
        case TOOL_ERASE:
            newTileId = 0;
            break;
        case TOOL_SELECT:
            m_currTile = m_doc.map()->at(x, y);
            emit newTile(m_currTile);
        }

        if (newTileId != tile)
        {
            m_doc.map()->at(x, y) = newTileId;
            m_doc.setDirty(true);
        }
    }
}

void MainWindow::initFileMenu()
{
    // gray out the open recent `nothin' yet`
    ui->actionNothing_yet->setEnabled(false);
    for (int i = 0; i < MAX_RECENT_FILES; i++)
    {
        m_recentFileActs[i] = new QAction(this);
        m_recentFileActs[i]->setVisible(false);
        ui->menuRecent_Maps->addAction(m_recentFileActs[i]);
        connect(m_recentFileActs[i], SIGNAL(triggered()),
                this, SLOT(openRecentFile()));
    }
    reloadRecentFileActions();
    // connect the File->Quit to the close app event
    connect(ui->actionFile_Exit, SIGNAL(triggered()), this, SLOT(close()));
    ui->actionFile_Exit->setMenuRole(QAction::QuitRole);
}

void MainWindow::reloadRecentFileActions()
{
    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();
    const int numRecentFiles = qMin(files.size(), static_cast<int>(MAX_RECENT_FILES));
    for (int i = 0; i < numRecentFiles; ++i)
    {
        QString text = tr("&%1 %2").arg(i + 1).arg(QFileInfo(files[i]).fileName());
        m_recentFileActs[i]->setText(text);
        m_recentFileActs[i]->setData(files[i]);
        m_recentFileActs[i]->setVisible(true);
        m_recentFileActs[i]->setStatusTip(files[i]);
    }
    ui->actionNothing_yet->setVisible(numRecentFiles == 0);
}

void MainWindow::updateRecentFileActions()
{
    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();
    QString fileName = m_doc.filename();
    files.removeAll(fileName);
    if (!fileName.isEmpty())
    {
        files.prepend(fileName);
        while (files.size() > MAX_RECENT_FILES)
            files.removeLast();
    }
    settings.setValue("recentFileList", files);
}

void MainWindow::openRecentFile()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
    {
        open(action->data().toString());
    }
    updateMenus();
}

void MainWindow::initToolBar()
{
    ui->toolBar->setIconSize(QSize(16, 16));
    ui->toolBar->addAction(ui->actionFile_New_File);
    ui->toolBar->addAction(ui->actionFile_Open);
    ui->toolBar->addAction(ui->actionFile_Save);
    ui->toolBar->addSeparator();
    ui->toolBar->addAction(ui->actionEdit_ResizeMap);
    ui->toolBar->addAction(ui->actionEdit_Previous_Map);
    ui->toolBar->addAction(ui->actionEdit_Next_Map);
    ui->toolBar->addAction(ui->actionEdit_First_Map);
    ui->toolBar->addAction(ui->actionEdit_Last_Map);
    ui->toolBar->addSeparator();
    ui->toolBar->addAction(ui->actionEdit_Add_Map);
    // ui->toolBar->addAction(ui->actionClear_Map);
    ui->toolBar->addAction(ui->actionEdit_Delete_Map);
    ui->toolBar->addSeparator();
    ui->toolBar->addAction(ui->actionTools_Paint);
    ui->toolBar->addAction(ui->actionTools_Erase);
    ui->toolBar->addAction(ui->actionTools_Select);

    m_toolGroup = new QActionGroup(this);
    m_toolGroup->addAction(ui->actionTools_Paint);
    m_toolGroup->addAction(ui->actionTools_Erase);
    m_toolGroup->addAction(ui->actionTools_Select);
    m_toolGroup->setExclusive(true);
    ui->actionTools_Paint->setChecked(true);
    ui->actionTools_Paint->setData(TOOL_PAINT);
    ui->actionTools_Erase->setData(TOOL_ERASE);
    ui->actionTools_Select->setData(TOOL_SELECT);

    QAction *actionToolBar = ui->toolBar->toggleViewAction();
    actionToolBar->setText(tr("ToolBar"));
    actionToolBar->setStatusTip(tr("Show or hide toolbar"));
    ui->menuView->addAction(actionToolBar);
}

void MainWindow::on_actionClear_Map_triggered()
{
    QString msg = tr("Clearing the map cannot be reversed. Continue?");
    QMessageBox::StandardButton reply = QMessageBox::warning(this, m_appName, msg, QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes)
    {
        m_doc.map()->clear();
        m_doc.setDirty(true);
    }
}

int MainWindow::currentTool()
{
    return m_toolGroup->checkedAction()->data().toUInt();
}

void MainWindow::on_actionHelp_About_triggered()
{
    DlgAbout dlg(this);
    dlg.exec();
}

void MainWindow::on_actionHelp_About_Qt_triggered()
{
    QApplication::aboutQt();
}

void MainWindow::on_actionEdit_Previous_Map_triggered()
{
    int index = m_doc.currentIndex();
    if (index > 0)
    {
        m_doc.setCurrentIndex(--index);
        emit mapChanged(m_doc.map());
        updateMenus();
    }
}

void MainWindow::on_actionEdit_Next_Map_triggered()
{
    int index = m_doc.currentIndex();
    if (index < m_doc.size() - 1)
    {
        m_doc.setCurrentIndex(++index);
        emit mapChanged(m_doc.map());
        updateMenus();
    }
}

void MainWindow::on_actionEdit_Add_Map_triggered()
{
    CMap *map = new CMap(64, 64);
    m_doc.add(map);
    m_doc.setCurrentIndex(m_doc.size() - 1);
    m_doc.setDirty(true);
    emit mapChanged(m_doc.map());
    updateMenus();
}

void MainWindow::on_actionEdit_Delete_Map_triggered()
{
    int index = m_doc.currentIndex();
    if (m_doc.size() > 1)
    {
        QString msg = tr("Deleting the map cannot be reversed. Continue?");
        QMessageBox::StandardButton reply = QMessageBox::warning(this, m_appName, msg, QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes)
        {
            CMap *delMap = m_doc.removeAt(index);
            delete delMap;
            m_doc.setDirty(true);
            emit mapChanged(m_doc.map());
            updateMenus();
        }
    }
}

void MainWindow::on_actionEdit_Insert_Map_triggered()
{
    int index = m_doc.currentIndex();
    CMap *map = new CMap(64, 64);
    m_doc.insertAt(index, map);
    m_doc.setDirty(true);
    emit mapChanged(m_doc.map());
    updateMenus();
}

void MainWindow::on_actionEdit_Move_Map_triggered()
{
    int currIndex = m_doc.currentIndex();
    CDlgSelect dlg(this);
    dlg.setWindowTitle(tr("Move map %1 to ...").arg(currIndex + 1));
    dlg.init(tr("Select destination"), &m_doc);
    if (dlg.exec() == QDialog::Accepted && dlg.index() != currIndex)
    {
        int i = dlg.index();
        CMap *map = m_doc.removeAt(currIndex);
        m_doc.insertAt(i, map);
        m_doc.setCurrentIndex(i);
        m_doc.setDirty(true);
        emit mapChanged(m_doc.map());
        updateMenus();
    }
}

void MainWindow::on_actionEdit_Goto_Map_triggered()
{
    int currIndex = m_doc.currentIndex();
    CDlgSelect dlg(this);
    dlg.setWindowTitle(tr("Go to Map ..."));
    dlg.init(tr("Select map"), &m_doc);
    if (dlg.exec() == QDialog::Accepted && dlg.index() != currIndex)
    {
        int i = dlg.index();
        m_doc.setCurrentIndex(i);
        emit mapChanged(m_doc.map());
        updateMenus();
    }
}

void MainWindow::on_actionEdit_Test_Map_triggered()
{
    if (m_doc.size())
    {
        CMap *map = m_doc.map();
        CStates & states = map->states();
        const uint16_t startPos = states.getU(POS_ORIGIN);
        // Sanitycheck
        const Pos pos = startPos !=0 ? CMap::toPos(startPos): map->findFirst(TILES_ANNIE2);
        QStringList listIssues;
        if ((pos.x == CMap::NOT_FOUND) && (pos.y == CMap::NOT_FOUND))
        {
            listIssues.push_back(tr("No player on map"));
            emit newTile(TILES_ANNIE2);
            m_currTile = TILES_ANNIE2;
        }
        if (map->count(TILES_DIAMOND) == 0)
        {
            listIssues.push_back(tr("No diamond on map"));
            emit newTile(TILES_DIAMOND);
            m_currTile = TILES_DIAMOND;
        }
        if (listIssues.count() > 0)
        {
            QString msg = tr("Map is incomplete:\n%1").arg(listIssues.join("\n"));
            QMessageBox::warning(this, m_appName, msg, QMessageBox::Button::Ok);
            return;
        }
        CDlgTest dlg(this);
        dlg.setWindowTitle(tr("Test Map"));
        dlg.init(&m_doc);
        dlg.exec();
    }
}

void MainWindow::on_actionFile_Import_Maps_triggered()
{
    QString fileName;
    QStringList filters;
    filters.append(m_allFilter);
    QFileDialog *dlg = new QFileDialog(this, tr("Import"), "", m_allFilter);
    dlg->setAcceptMode(QFileDialog::AcceptOpen);
    dlg->setFileMode(QFileDialog::ExistingFile);
    dlg->selectFile("");
    dlg->setNameFilters(filters);
    if (dlg->exec())
    {
        QStringList fileNames = dlg->selectedFiles();
        if (fileNames.count() > 0)
        {
            fileName = fileNames[0];
        }
    }
    delete dlg;

    // copy map from arch to current document
    if (!fileName.isEmpty())
    {
        CMapArch arch;
        if (arch.extract(fileName.toLocal8Bit().toStdString().c_str()))
        {
            for (int i = 0; i < arch.size(); ++i)
            {
                m_doc.add(arch.at(i));
            }
            arch.removeAll();
            m_doc.setCurrentIndex(m_doc.size() - 1);
            m_doc.setDirty(true);
            emit mapChanged(m_doc.map());
            updateMenus();
        }
        else
        {
            QString msg = tr("Fail to import:\n%1").arg(fileName);
            QMessageBox::warning(this, m_appName, msg, QMessageBox::Button::Ok);
        }
    }
}

void MainWindow::on_actionFile_Export_Map_triggered()
{
    QStringList filters;
    QString suffix = "dat";
    QString fileName = "";

    QFileDialog *dlg = new QFileDialog(this, tr("Export Map"), "", m_allFilter);
    dlg->setAcceptMode(QFileDialog::AcceptSave);

    dlg->setNameFilters(filters);
    dlg->setAcceptMode(QFileDialog::AcceptSave);
    dlg->setDefaultSuffix(suffix);
    dlg->selectFile(tr("level%1.dat").arg(m_doc.currentIndex() + 1, 2, 10, QChar('0')));
    if (dlg->exec())
    {
        QStringList fileNames = dlg->selectedFiles();
        if (fileNames.count() > 0)
        {
            fileName = fileNames[0];
        }
    }

    if (!fileName.isEmpty())
    {
        // copy current map
        if (!m_doc.map()->write(fileName.toLocal8Bit().toStdString().c_str()))
        {
            QString msg = tr("Fail exporting to:\n%1").arg(fileName);
            QMessageBox::warning(this, m_appName, msg, QMessageBox::Button::Ok);
        }
    }

    updateTitle();
    delete dlg;
}

void MainWindow::on_actionEdit_Rename_Map_triggered()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("Rename Map"),
                                         tr("Name:"), QLineEdit::Normal,
                                         m_doc.map()->title(), &ok);
    text = text.trimmed().mid(0, 254);
    if (ok && strcmp(text.toLatin1(), m_doc.map()->title()) != 0)
    {
        m_doc.map()->setTitle(text.toLatin1());
        m_doc.setDirty(true);
    }
}

void MainWindow::on_actionEdit_Last_Map_triggered()
{
    int index = m_doc.currentIndex();
    if (index < m_doc.size() - 1)
    {
        m_doc.setCurrentIndex(m_doc.size() - 1);
        emit mapChanged(m_doc.map());
        updateMenus();
    }
}

void MainWindow::on_actionEdit_First_Map_triggered()
{
    int index = m_doc.currentIndex();
    if (index > 0)
    {
        m_doc.setCurrentIndex(0);
        emit mapChanged(m_doc.map());
        updateMenus();
    }
}

void MainWindow::on_actionFile_Generate_Report_triggered()
{
    QStringList filters;
    QString suffix = "txt";
    QString fileName = "";

    QFileDialog *dlg = new QFileDialog(this, tr("Generate Report"), "", m_allFilter);
    dlg->setAcceptMode(QFileDialog::AcceptSave);

    dlg->setNameFilters(filters);
    dlg->setAcceptMode(QFileDialog::AcceptSave);
    dlg->setDefaultSuffix(suffix);
    dlg->selectFile(tr("report.txt"));
    if (dlg->exec())
    {
        QStringList fileNames = dlg->selectedFiles();
        if (fileNames.count() > 0)
        {
            fileName = fileNames[0];
        }
    }

    if (!fileName.isEmpty())
    {
        generateReport(m_doc, fileName);
    }

    delete dlg;
}

void MainWindow::on_actionEdit_Map_States_triggered()
{
    CStates & states = m_doc.map()->states();
    //states.setU(TIMEOUT, 111);
    //states.setS(MSG1,"Welcome");
    //states.setS(MSG2,"Game Over");
    //states.setS(MSG3,"Too Bad");
    KeyValueDialog dialog(this);
    std::vector<StateValuePair> data;
    states.getValues(data);
    dialog.populateData(data);
    if (dialog.exec() == QDialog::Accepted) {
        m_doc.setDirty(true);
        const auto pairs = dialog.getKeyValuePairs();
        // Process the key-value pairs
        for (size_t i=0; i < pairs.size(); ++i) {
            auto& p = pairs[i];
            if (KeyValueDialog::getOptionType(p.key)== TYPE_U) {
                bool ok;
                uint16_t value = KeyValueDialog::parseStringToUint16(p.value, ok);
                states.setU(p.key, value);
            } else if (KeyValueDialog::getOptionType(p.key)== TYPE_S) {
                states.setS(p.key, p.value);
            }
        }
    }
}
