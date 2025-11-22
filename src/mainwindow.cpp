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
#include <QLabel>
#include "mapscroll.h"
#include "mapwidget.h"
#include "dlgresize.h"
#include "dlgselect.h"
#include "dlgtest.h"
#include "tilebox.h"
#include "runtime/tilesdata.h"
#include "runtime/map.h"
#include "report.h"
#include "keyvaluedialog.h"
#include "runtime/statedata.h"
#include "runtime/dirs.h"
#include "mapprops.h"
#include "TileSelectorWidget.h"
#include <QScrollArea>
#include "MapView.h"
#include "LayerDock.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_mapView = new MapView(this);
    m_mapView->mapWidget()->setMainWindow(this);
    m_mapView->setMap(m_doc.map());
    m_mapView->setZoom(2); // start at 200%
    m_mapView->centerOnMap();
    setCentralWidget(m_mapView);
    // connect(this, SIGNAL(mapChanged(CMap *)), m_mapView, SLOT(setMap(CMap *)));
    connect(this, SIGNAL(mapChanged(CMap *)), this, SLOT(updateStatus()));
    connect(ui->actionView_Grid, &QAction::toggled, m_mapView->mapWidget(), &MapWidget::setGridVisible);

    setDirty(false);
    initTilebox();
    initSelectorWidget();
    initLayerBox();
    initFileMenu();
    initMapShortcuts();
    initToolBar();
    updateMenus();
    setWindowIcon(QIcon(":/data/icons/CS3MapEdit-icon.png"));
    m_label = new QLabel("", ui->statusbar);
    m_label->setAlignment(Qt::AlignRight);
    m_label0 = new QLabel("", ui->statusbar);
    ui->statusbar->addWidget(m_label0);
    ui->statusbar->addWidget(m_label, 64);
    updateStatus();
}

void MainWindow::updateStatus()
{
    QString s = QString(tr("%1 of %2 ")).arg(m_doc.currentIndex() + 1).arg(m_doc.size());
    m_label->setText(s);
}

void MainWindow::shiftUp()
{
    m_doc.map()->shift(Direction::UP);
    setDirty(true);
}

void MainWindow::shiftDown()
{
    m_doc.map()->shift(Direction::DOWN);
    setDirty(true);
}

void MainWindow::shiftLeft()
{
    m_doc.map()->shift(Direction::LEFT);
    setDirty(true);
}

void MainWindow::shiftRight()
{
    m_doc.map()->shift(Direction::RIGHT);
    setDirty(true);
}

void MainWindow::initMapShortcuts()
{
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
    connect(this, SIGNAL(newTile(int)),
            tilebox, SLOT(setTile(int)));

    // Stamp tool + tileID
    connect(tilebox, &CTileBox::tileChanged, this, [this](int tileID)
            {
        m_mapView->mapWidget()->setTool(MapWidget::Tool::Stamp);
        m_mapView->mapWidget()->setCurrentTile(tileID);//
        ui->actionTools_Paint->setChecked(true);
    });

    m_mapView->mapWidget()->setTool(MapWidget::Tool::Stamp);
    m_mapView->mapWidget()->setCurrentTile(0);
}

void MainWindow::initSelectorWidget()
{
    auto dock = new QDockWidget();
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    dock->setWindowTitle(tr("Toolbox"));

    TileSelectorWidget *selectorWidget = new TileSelectorWidget(dock);
    // Load the image
    QPixmap image(":/data/cs3layers.png");
    selectorWidget->setImage(image);

    // Configure Tiling
    selectorWidget->setTileSize(16);

    // Configure Zoom (e.g., 200% zoom)
    selectorWidget->setZoomLevel(2);
    selectorWidget->show();

    // Create the QScrollArea
    QScrollArea *scrollArea = new QScrollArea();

    // Set policies: Always allow scrolling if the content exceeds the viewport.
    scrollArea->setWidgetResizable(false); // CRITICAL: Set to false. We want the scroll area
    // to use the TileSelectorWidget's sizeHint/minimumSize.

    // 3. Set the TileSelectorWidget as the scroll area's widget
    scrollArea->setWidget(selectorWidget);

    QTabWidget *tabWidget = new QTabWidget(dock);

    // 4. Add the QScrollArea (not the TileSelectorWidget) to the QTabWidget
    tabWidget->addTab(scrollArea, "Tile Selector");

    // Optional: Set scrollbar policies for visual consistency
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    dock->setWidget(tabWidget);
    addDockWidget(Qt::RightDockWidgetArea, dock);
    dock->setAllowedAreas(Qt::RightDockWidgetArea);

    QObject::connect(
        selectorWidget,
        &TileSelectorWidget::tilesSelected,
        this,      // The context object for lifetime management
        [this]() { // Lambda function (captures 'this' to access ui members)
            (void)this;
            //  QMessageBox::information(this, "Exiting", "Goodbye!");
            // QApplication::quit();
            qDebug("selection received");
        });
}

void MainWindow::initLayerBox()
{
    LayerDock *dock = new LayerDock(m_doc.map(), this);
    dock->show();
    addDockWidget(Qt::RightDockWidgetArea, dock);

    connect(dock, &LayerDock::visibilityChanged,
            this, [&](int layerID, bool visible)
            {
                qDebug("layerID: %d visible %d", layerID, visible);
                // you decide what to do
                // e.g. map renderer hides layer
            });
    connect(this, &MainWindow::mapChanged, dock, &LayerDock::refreshList);
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
        QMessageBox::StandardButton ret = QMessageBox::question(
            this,
            tr("Unsaved changes"),
            tr("The map has unsaved changes. Do you want to save before closing?"),
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
            qDebug("size: %lu", m_doc.size());
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
        // updateTitle();
        setDirty(false);
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

    if (!m_doc.write())
    {
        warningMessage(tr("Can't write file"));
        m_doc.setFilename(oldFileName);
        return false;
    }

    setDirty(false);
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

    setDirty(false);
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

void MainWindow::updateMenus()
{
    size_t index = m_doc.currentIndex();
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
    // ui->statusbar->showMessage(msg);
    m_label0->setText(msg);
}

void MainWindow::on_actionFile_New_File_triggered()
{
    if (maybeSave())
    {
        m_doc.setFilename("");
        m_doc.forget();
        std::unique_ptr<CMap> map = std::make_unique<CMap>(40, 40);
        m_doc.add(std::move(map));
        setDirty(false);
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
            map.resize(dlg.width(), dlg.height(), '\0', false);
            // m_doc.
            setDirty(true);
            emit resizeMap(m_doc.map()->len(), m_doc.map()->hei());
        }
    }
}

/*
void MainWindow::showAttrDialog(int x, int y)
{
    CMap &map = *m_doc.map();
    uint8_t a = map.getAttr(x, y);
    CDlgAttr dlg(this);
    dlg.attr(a);
    if (dlg.exec() == QDialog::Accepted)
    {
        a = dlg.attr();
        map.setAttr(x, y, a);
            setDirty(true);
    }
}

void MainWindow::showStatDialog(int x, int y)
{
    CMap &map = *m_doc.map();
    CDlgStat dlg(map.at(x, y), map.getAttr(x, y), this);
    dlg.setWindowTitle(tr("Tile Statistics"));
    dlg.exec();
}
*/

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
    ui->toolBar->addAction(ui->actionEdit_Edit_Messages);
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
    ui->toolBar->addAction(ui->actionTools_Picker);
    ui->toolBar->addAction(ui->actionTools_Select);
    ui->toolBar->addSeparator();
    m_cbSkill = new QComboBox(this);
    m_cbSkill->addItem("Easy Mode");
    m_cbSkill->addItem("Normal Mode");
    m_cbSkill->addItem("Hard Mode");
    m_cbSkill->setCurrentIndex(1);
    ui->toolBar->addWidget(m_cbSkill);

    QComboBox* cbZoom = new QComboBox(this);
    cbZoom->addItem("100%", 1);
    cbZoom->addItem("200%", 2);
    cbZoom->addItem("300%", 3);
    cbZoom->addItem("400%", 4);
    cbZoom->setCurrentIndex(1);
    ui->toolBar->addWidget(cbZoom);
    connect(cbZoom, &QComboBox::currentIndexChanged, this, [this](int i) {
        m_mapView->setZoom(i + 1);
    });

    m_toolGroup = new QActionGroup(this);
    m_toolGroup->addAction(ui->actionTools_Paint);
    m_toolGroup->addAction(ui->actionTools_Erase);
    m_toolGroup->addAction(ui->actionTools_Picker);
    m_toolGroup->addAction(ui->actionTools_Select);
    m_toolGroup->setExclusive(true);
    ui->actionTools_Paint->setChecked(true);

    QAction *actionToolBar = ui->toolBar->toggleViewAction();
    actionToolBar->setText(tr("ToolBar"));
    actionToolBar->setStatusTip(tr("Show or hide toolbar"));
    ui->menuView->addAction(actionToolBar);

    connect(ui->actionView_Zoom_In, &QAction::triggered, m_mapView, &MapView::zoomIn);
    connect(ui->actionView_Zoom_Out, &QAction::triggered, m_mapView, &MapView::zoomOut);

    // ──────────────────────────────────────────────────────────────
    //  TOOLBAR → MapWidget TOOL WIRING (add this once)
    // ──────────────────────────────────────────────────────────────

    // enable stamp by default
    m_mapView->mapWidget()->setTool(MapWidget::Tool::Stamp);

    // Paint = Stamp tool
    connect(ui->actionTools_Paint, &QAction::triggered, this, [this]()
            { m_mapView->mapWidget()->setTool(MapWidget::Tool::Stamp); });

    // Erase = Stamp tool + tile 0 (or your transparent tile ID)
    connect(ui->actionTools_Erase, &QAction::triggered, this, [this]()
            {
                m_mapView->mapWidget()->setTool(MapWidget::Tool::Eraser);
            });

    // Tile Picker tool
    connect(ui->actionTools_Picker, &QAction::triggered, this, [this]()
            { m_mapView->mapWidget()->setTool(MapWidget::Tool::Picker); });

    // area selection tool
    connect(ui->actionTools_Select, &QAction::triggered, this, [this]()
            { m_mapView->mapWidget()->setTool(MapWidget::Tool::Selection); });

    // ──────────────────────────────────────────────────────────────
    //  BONUS: When user picks a tile from tileset → auto-switch to Paint
    // ──────────────────────────────────────────────────────────────
    connect(m_mapView->mapWidget(), &MapWidget::tilePicked, this, [this](uint8_t tileId)
            {
                // Update current brush
                m_mapView->mapWidget()->setCurrentTile(tileId);

                // Auto-activate Paint tool (this is the UX of Tiled, Godot, Aseprite, etc.)
                ui->actionTools_Paint->trigger(); // this will fire the connection above
            });

    // ──────────────────────────────────────────────────────────────
    //  FULL TWO-WAY WIRING: MainWindow ↔ MapWidget
    // ──────────────────────────────────────────────────────────────

    // 1. When user picks a tile with the Picker tool → tell MainWindow
    connect(m_mapView->mapWidget(), &MapWidget::tilePicked, this, &MainWindow::newTile);

    // 2. When MainWindow wants to change the current brush tile (e.g. user clicked in tile palette)
    connect(this, &MainWindow::newTile, this, [this](int tileId)
            {
        uint8_t id = static_cast<uint8_t>(tileId);
        m_mapView->mapWidget()->setCurrentTile(id);

        // Auto-switch to Paint tool (this is the correct, expected UX)
        ui->actionTools_Paint->setChecked(true);
        m_mapView->mapWidget()->setTool(MapWidget::Tool::Stamp); });

    // 3. When map is resized or replaced → tell MapView + update size
    connect(this, &MainWindow::resizeMap, this, [this](int w, int h)
            {
        if (m_mapView->mapWidget()->map()) {
            int tileSize = 16 * m_mapView->mapWidget()->zoom();
            m_mapView->mapWidget()->resize(w * tileSize, h * tileSize);
        } });

    connect(this, &MainWindow::mapChanged, this, [this](CMap *newMap)
            {
                m_mapView->setMap(newMap); // MapView handles resize + centering
                // optional: reset zoom/scroll position
                // m_mapView->setZoom(2);
                // m_mapView->centerOnMap();
            });

    // 5. Bonus: When selection changes → you can show coordinates or copy buffer
    connect(m_mapView->mapWidget(), &MapWidget::selectionChanged, this, [this](const QRect &rect)
            {
        if (rect.isValid()) {
            statusBar()->showMessage(QString("Selection: %1×%2  at (%3,%4)")
                                         .arg(rect.width()).arg(rect.height())
                                         .arg(rect.x()).arg(rect.y()));
        } else {
            statusBar()->clearMessage();
        } });

    // 6. When map is modified inside MapWidget → notify MainWindow (for undo/save flag)
    connect(m_mapView->mapWidget(), &MapWidget::mapModified, this, [this]()
            {
        //emit mapChanged(m_mapView->mapWidget()->map());  // re-emit with current map pointer
        // also mark document as dirty
        setWindowModified(true);
        setDirty(true); });
}

void MainWindow::on_actionClear_Map_triggered()
{
    QString msg = tr("Clearing the map cannot be reversed. Continue?");
    QMessageBox::StandardButton reply = QMessageBox::warning(this, m_appName, msg, QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes)
    {
        m_doc.map()->clear();
        setDirty(true);
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
    size_t index = m_doc.currentIndex();
    if (index < m_doc.size() - 1)
    {
        m_doc.setCurrentIndex(++index);
        emit mapChanged(m_doc.map());
        updateMenus();
    }
}

void MainWindow::on_actionEdit_Add_Map_triggered()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("Add New Map"),
                                         tr("Name:"), QLineEdit::Normal,
                                         "", &ok);
    if (!ok)
        return;
    text = text.trimmed().mid(0, 254);

    std::unique_ptr<CMap> map = std::make_unique<CMap>(64, 64);
    map->setTitle(text.toLatin1());
    m_doc.add(std::move(map));
    m_doc.setCurrentIndex(m_doc.size() - 1);
    setDirty(true);
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
            setDirty(true);
            emit mapChanged(m_doc.map());
            updateMenus();
        }
    }
}

void MainWindow::on_actionEdit_Insert_Map_triggered()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("Insert New Map"),
                                         tr("Name:"), QLineEdit::Normal,
                                         m_doc.map()->title(), &ok);
    if (!ok)
        return;
    text = text.trimmed().mid(0, 254);

    int index = m_doc.currentIndex();
    std::unique_ptr<CMap> map = std::make_unique<CMap>(64, 64);
    map->setTitle(text.toLatin1());
    m_doc.insertAt(index, std::move(map)); // Only ONE move operation
                                           // m_doc.
    setDirty(true);
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
        m_doc.insertAt(i, std::unique_ptr<CMap>{map});
        m_doc.setCurrentIndex(i);
        setDirty(true);
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
        CStates &states = map->states();
        const uint16_t startPos = states.getU(POS_ORIGIN);
        // Sanitycheck
        const Pos pos = startPos != 0 ? CMap::toPos(startPos) : map->findFirst(TILES_ANNIE2);
        QStringList listIssues;
        if ((pos.x == CMap::NOT_FOUND) && (pos.y == CMap::NOT_FOUND))
        {
            listIssues.push_back(tr("No player on map"));
            emit newTile(TILES_ANNIE2);
        }
        if (map->count(TILES_DIAMOND) == 0)
        {
            listIssues.push_back(tr("No diamond on map"));
            emit newTile(TILES_DIAMOND);
        }
        if (listIssues.count() > 0)
        {
            QString msg = tr("Map is incomplete:\n%1").arg(listIssues.join("\n"));
            QMessageBox::warning(this, m_appName, msg, QMessageBox::Button::Ok);
            return;
        }
        int skill = m_cbSkill->currentIndex();
        qDebug("starting test");
        CDlgTest dlg(this);
        dlg.setWindowTitle(tr("Test Map"));
        dlg.setSkill(skill);
        dlg.init(&m_doc, m_doc.currentIndex());
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
            while (arch.size())
            {
                auto map = arch.removeAt(0);
                m_doc.add(std::move(map));
            }
            m_doc.setCurrentIndex(m_doc.size() - 1);
            setDirty(true);
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
        setDirty(true);
    }
}

void MainWindow::on_actionEdit_Last_Map_triggered()
{
    size_t index = m_doc.currentIndex();
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
    CStates &states = m_doc.map()->states();
    KeyValueDialog dialog(this);
    std::vector<StateValuePair> data = states.getValues();
    dialog.populateData(data);
    if (dialog.exec() == QDialog::Accepted)
    {
        setDirty(true);
        const auto pairs = dialog.getKeyValuePairs();
        // Process the key-value pairs
        states.clear();
        for (size_t i = 0; i < pairs.size(); ++i)
        {
            auto &p = pairs[i];
            if (KeyValueDialog::getOptionType(p.key) == TYPE_U)
            {
                bool ok;
                uint16_t value = KeyValueDialog::parseStringToUint16(p.value, ok);
                states.setU(p.key, value);
            }
            else if (KeyValueDialog::getOptionType(p.key) == TYPE_S)
            {
                states.setS(p.key, p.value);
            }
            else
            {
                qDebug("unhandled key: %.2x", p.key);
            }
        }
    }
}

void MainWindow::on_actionExport_Screenshots_triggered()
{
    QString folder = QFileDialog::getExistingDirectory(
        this,
        tr("Select Destination Folder"),
        QDir::homePath(), // default path
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!folder.isEmpty())
    {
        QMessageBox::information(this, "Export", "Exporting to:\n" + folder);
        for (size_t i = 0; i < m_doc.size(); ++i)
        {
            CMap *map = m_doc.at(i);
            const QString filename = folder + QString("/level%1.png").arg(i + 1, 2, 10, QLatin1Char('0'));
            generateScreenshot(filename, map, 24, 24);
        }
    }
}

void MainWindow::on_actionEdit_Edit_Messages_triggered()
{
    MapPropertiesDialog dialog(m_doc.map(), this, MapPropertiesDialog::TAB_MESSAGES);
    if (dialog.exec() == QDialog::Accepted)
    {
        // Changes have been saved to the states object
        setDirty(true);
    }
}

void MainWindow::on_actionEdit_Map_Properties_triggered()
{
    MapPropertiesDialog dialog(m_doc.map(), this);
    if (dialog.exec() == QDialog::Accepted)
    {
        // Changes have been saved to the states object
        setDirty(true);
    }
}

void MainWindow::updateWindowTitle()
{
    QString title = m_appName;
    if (!m_doc.filename().isEmpty())
    {
        title += " — " + QFileInfo(m_doc.filename()).fileName();
    }
    else
    {
        title += " — Untitled";
    }

    // On macOS, Qt automatically shows asterisk in title bar.
    // On Windows/Linux, you usually add it manually for clarity:
#ifndef Q_OS_MACOS
    if (m_doc.isDirty())
        title += " [*]";
#endif

    setWindowTitle(title);
    // This is the official Qt way — it adds the asterisk on ALL platforms
    setWindowModified(m_doc.isDirty());
}

void MainWindow::setDirty(bool dirty)
{
    if (m_doc.isDirty() != dirty)
    {
        m_doc.setDirty(dirty);
        updateWindowTitle();
    }
}
