#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "mapfile.h"

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


private slots:
    void loadFile(const QString & filename);
    void setStatus(const QString str);
    void showAttrDialog();
    void on_actionOpen_triggered();
    void on_actionSave_as_triggered();
    void on_actionNew_Map_triggered();
    void on_actionResizeMap_triggered();
    void showContextMenu(const QPoint&pos);
    void changeTile(int tile);
    void onLeftClick(int x, int y);
    void on_actionSave_triggered();
    void openRecentFile();

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

    enum {
        MAX_RECENT_FILES = 8,
        GRID_SIZE = 32,
    };

    Ui::MainWindow *ui;
    CMapScroll *m_scrollArea;
    CMapFile m_doc;
    int m_hx = -1;
    int m_hy = -1;
    uint8_t m_currTile = 0;
    QAction *m_recentFileActs[MAX_RECENT_FILES];
};
#endif // MAINWINDOW_H
