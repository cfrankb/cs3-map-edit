#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "mapfile.h"
#include "qactiongroup.h"

class CMapScroll;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void resizeMap(int, int);
    void newTile(int);

private slots:
    void loadFile(const QString & filename);
    void setStatus(const QString str);
    void showAttrDialog();
    void on_actionFile_New_Map_triggered();
    void on_actionFile_Open_triggered();
    void on_actionFile_Save_triggered();
    void on_actionFile_Save_as_triggered();
    void on_actionEdit_ResizeMap_triggered();
    void showContextMenu(const QPoint&pos);
    void changeTile(int tile);
    void onLeftClick(int x, int y);
    void openRecentFile();
    void shiftUp();
    void shiftDown();
    void shiftLeft();
    void shiftRight();
    void on_actionClear_Map_triggered();
    void on_actionHelp_About_triggered();
    void on_actionHelp_About_Qt_triggered();

private:
    virtual void closeEvent(QCloseEvent *event) override;
    bool maybeSave();
    void setDocument(const QString);
    void warningMessage(const QString message);
    bool saveAs();
    bool save();
    void open(QString);
    bool updateTitle();
    void updateMenus();
    void updateStatus();
    bool isDirty();
    void updateRecentFileActions();
    void reloadRecentFileActions();

    void initFileMenu();
    void initTilebox();
    void initShortcuts();
    void initMapShortcuts();
    void initToolBar();
    int currentTool();

    enum {
        MAX_RECENT_FILES = 8,
        GRID_SIZE = 32,
        TOOL_SELECT = 0,
        TOOL_PAINT=1,
        TOOL_ERASE=2
    };

    Ui::MainWindow *ui;
    CMapScroll *m_scrollArea;
    CMapFile m_doc;
    int m_hx = -1;
    int m_hy = -1;
    QActionGroup *m_toolGroup;
    uint8_t m_currTile = 0;
    QAction *m_recentFileActs[MAX_RECENT_FILES];
};
#endif // MAINWINDOW_H
