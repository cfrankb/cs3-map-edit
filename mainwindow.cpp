#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtOpenGL>
#include "mapscroll.h"
#include "mapwidget.h"

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

    // connect the File->Quit to the close app event
    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));
    ui->actionExit->setMenuRole(QAction::QuitRole);
    connect(m_scrollArea, SIGNAL(statusChanged(QString)), this, SLOT(setStatus(QString)));

    CMapWidget * glw = dynamic_cast<CMapWidget *>(m_scrollArea->viewport());
    glw->setMap(m_doc.map());

    updateTitle();
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
    return false;
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
            dlg->selectFile("");//m_doc.getFileName());
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
    updateStatus();
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
     //   updateRecentFileActions();
       // reloadRecentFileActions();
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

  //  updateRecentFileActions();
   // reloadRecentFileActions();
    //updateStatus();
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
    setWindowTitle(tr("%1[*] - %2").arg( file  ).arg(tr(m_appName)));
    return true;
}

void MainWindow::updateMenus()
{

}

void MainWindow::updateStatus()
{
    //setStatus(msg);
}

void MainWindow::setStatus(const QString msg)
{
    ui->statusbar->showMessage(msg);
}

void MainWindow::on_actionOpen_triggered()
{
    open("");
}

void MainWindow::on_actionSave_as_triggered()
{
    saveAs();
}

void MainWindow::on_actionNew_Map_triggered()
{
    save();
}

void MainWindow::on_actionResize_triggered()
{

}
