#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtOpenGL>
#include "mapscroll.h"
#include "mapwidget.h"
#include "dlgattr.h"
#include "dlgresize.h"
#include "tilebox.h"

const char m_allFilter[]= "All Supported Maps (*.dat *.cs3 *.map)";
const char m_appName[] = "mapedit";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_scrollArea = new CMapScroll(this);
    m_scrollArea->viewport()->update();
    setCentralWidget(m_scrollArea);
    connect(m_scrollArea, SIGNAL(statusChanged(QString)), this, SLOT(setStatus(QString)));
    connect(m_scrollArea, SIGNAL(leftClickedAt(int,int)), this, SLOT(onLeftClick(int,int)));

    CMapWidget * glw = dynamic_cast<CMapWidget *>(m_scrollArea->viewport());
    glw->setMap(m_doc.map());

    connect(m_scrollArea, SIGNAL(customContextMenuRequested(const QPoint&)),
        this, SLOT(showContextMenu(const QPoint&))) ;

    updateTitle();

    auto dock = new QDockWidget ();
    dock->setWindowTitle("Toolbox");
    auto tilebox = new CTileBox(dock);
    tilebox->show();
    dock->setWidget (tilebox);
    addDockWidget (Qt::LeftDockWidgetArea, dock);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea);

    connect(tilebox, SIGNAL(tileChanged(int)),
            this, SLOT(changeTile(int)));

    initFileMenu();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        event->accept();
    } else {
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
    if (isDirty()) {
        QMessageBox::StandardButton ret = QMessageBox::warning(this, tr(m_appName),
                     tr("The document has been modified.\n"
                        "Do you want to save your changes?"),
                     QMessageBox::Save | QMessageBox::Discard
                     | QMessageBox::Cancel);
        if (ret == QMessageBox::Save)
            return save();
        else if (ret == QMessageBox::Cancel)
            return false;
    }
    return true;
}

void MainWindow::open(QString fileName)
{
    if (maybeSave()) {
        if (fileName.isEmpty()) {
            QStringList filters;
            filters.append(m_allFilter);
            QFileDialog * dlg = new QFileDialog(this,tr("Open"),"",m_allFilter);
            dlg->setAcceptMode(QFileDialog::AcceptOpen);
            dlg->setFileMode(QFileDialog::ExistingFile);
            dlg->selectFile(m_doc.filename());
            dlg->setNameFilters(filters);
            if (dlg->exec()) {
                QStringList fileNames = dlg->selectedFiles();
                if (fileNames.count()>0) {
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
    if (!fileName.isEmpty()) {
        QString oldFileName = m_doc.filename();
        m_doc.setFilename(fileName);
        if (m_doc.read())  {
            qDebug("size: %d", m_doc.size());
        } else {
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
    }
}

bool MainWindow::save()
{
    QString oldFileName = m_doc.filename();
    if (m_doc.isUntitled()) {
        if (!saveAs())
            return false;
    }

    if (!m_doc.write() || !updateTitle())  {
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
    QString suffix = "dat";
    QString fileName = "";

    QFileDialog * dlg = new QFileDialog(this,tr("Save as"),"",m_allFilter);
    dlg->setAcceptMode(QFileDialog::AcceptSave);

    dlg->setNameFilters(filters);
    dlg->setAcceptMode(QFileDialog::AcceptSave);
    dlg->setDefaultSuffix(suffix);
    dlg->selectFile(m_doc.filename());
    if (dlg->exec()) {
        QStringList fileNames = dlg->selectedFiles();
        if (fileNames.count()>0) {
            fileName = fileNames[0];
        }
    }

    if (!fileName.isEmpty()) {
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
    if (m_doc.filename().isEmpty()) {
        file = tr("untitled");
    } else {
        file = QFileInfo(m_doc.filename()).fileName();
    }
    m_doc.setDirty(false);
    setWindowTitle(tr("%1[*] - %2").arg( file, m_appName));
    return true;
}

void MainWindow::updateMenus()
{

}


void MainWindow::setStatus(const QString msg)
{
    ui->statusbar->showMessage(msg);
}

void MainWindow::on_actionNew_Map_triggered()
{
    if (maybeSave()) {
        m_doc.setFilename("");
        m_doc.map()->clear();
        m_doc.map()->resize(40,40, true);
        updateTitle();
    }
}

void MainWindow::on_actionOpen_triggered()
{
    open("");
}

void MainWindow::on_actionSave_triggered()
{
    save();
}

void MainWindow::on_actionSave_as_triggered()
{
    saveAs();
}

void MainWindow::on_actionResize_triggered()
{
    CMap &map = * m_doc.map();
    CDlgResize dlg(this);
    dlg.width(map.len());
    dlg.height(map.hei());
    if (dlg.exec()==QDialog::Accepted){
        // TODO: add confirmation + validation
        map.resize(dlg.width(), dlg.height(), false);
        m_doc.setDirty(true);
    }
}

void MainWindow::showContextMenu(const QPoint& pos)
{
    int x = pos.x() / GRID_SIZE;
    int y = pos.y() / GRID_SIZE;
    int mx = m_scrollArea->horizontalScrollBar()->value();
    int my = m_scrollArea->verticalScrollBar()->value();

    CMap &map = * m_doc.map();
    if (x >=0 && y >= 0 && x + mx < map.len() && y + my < map.hei()) {
        QMenu menu(this);
        QAction *actionSetAttr = new QAction(tr("set attribute"), &menu);
        connect(actionSetAttr, SIGNAL(triggered()),
                this, SLOT(showAttrDialog()));
        menu.addAction(actionSetAttr);
        m_hx = x + mx;
        m_hy = y + my;
        menu.exec(m_scrollArea->mapToGlobal(pos));
    }
}

void MainWindow::showAttrDialog()
{
    CMap &map = * m_doc.map();
    uint8_t a = map.getAttr(m_hx, m_hy);
    CDlgAttr dlg(this);
    dlg.attr(a);
    if (dlg.exec()==QDialog::Accepted){
        a = dlg.attr();
        map.setAttr(m_hx, m_hy, a);
        m_doc.setDirty(true);
    }
}

void MainWindow::changeTile(int tile)
{
    m_currTile = tile;
}

void MainWindow::onLeftClick(int x, int y)
{
    if ( (x >= 0) && (y >= 0)
         && (x < m_doc.map()->len())
         && (y < m_doc.map()->hei()) )
    {
        const uint8_t tile = m_doc.map()->at(x,y);
        if (tile != m_currTile) {
            m_doc.map()->at(x,y) = m_currTile;
            m_doc.setDirty(true);
        }
    }
}

void MainWindow::initFileMenu()
{
    // gray out the open recent `nothin' yet`
    ui->actionNothing_yet->setEnabled(false);
    for (int i = 0; i < MAX_RECENT_FILES; i++) {
        m_recentFileActs[i] = new QAction(this);
        m_recentFileActs[i]->setVisible(false);
        ui->menuRecent_Maps->addAction(m_recentFileActs[i]);
        connect(m_recentFileActs[i], SIGNAL(triggered()),
                this, SLOT(openRecentFile()));
    }
    reloadRecentFileActions();
    // connect the File->Quit to the close app event
    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));
    ui->actionExit->setMenuRole(QAction::QuitRole);
}

void MainWindow::reloadRecentFileActions()
{
    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();
    const int numRecentFiles = qMin(files.size(), static_cast<int>(MAX_RECENT_FILES));
    for (int i = 0; i < numRecentFiles; ++i) {
        QString text = tr("&%1 %2").arg(i + 1).arg(QFileInfo(files[i]).fileName());
        m_recentFileActs[i]->setText(text);
        m_recentFileActs[i]->setData(files[i]);
        m_recentFileActs[i]->setVisible(true);
        m_recentFileActs[i]->setStatusTip(files[i]);
    }
    //for (int j = numRecentFiles; j < MAX_RECENT_FILES; ++j)
    //    m_recentFileActs[j]->setVisible(false);
    ui->actionNothing_yet->setVisible(numRecentFiles == 0);
}

void MainWindow::updateRecentFileActions()
{
    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();
    QString fileName = m_doc.filename();
    files.removeAll(fileName);
    if (!fileName.isEmpty()) {
        files.prepend(fileName);
        while (files.size() > MAX_RECENT_FILES)
            files.removeLast();
    }
    settings.setValue("recentFileList", files);
}

void MainWindow::openRecentFile()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {
        open(action->data().toString());
    }
    updateMenus();
}
