#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "mapfile.h"
#include "qactiongroup.h"

class CMapScroll;
class QComboBox;
class QLabel;
class MapView;

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
    void resizeMap(int, int);   // notify of a map resize
    void mapChanged(CMap *);    // notify of a map change
    void newTile(int);          // select a diffent tile in the tilebox

private slots:
    void loadFile(const QString & filename);
    void setStatus(const QString str);
    void on_actionFile_New_File_triggered();
    void on_actionFile_Open_triggered();
    void on_actionFile_Save_triggered();
    void on_actionFile_Save_as_triggered();
    void on_actionEdit_ResizeMap_triggered();
    void on_actionClear_Map_triggered();
    void on_actionHelp_About_triggered();
    void on_actionHelp_About_Qt_triggered();
    void on_actionEdit_Previous_Map_triggered();
    void on_actionEdit_Next_Map_triggered();
    void on_actionEdit_Add_Map_triggered();
    void on_actionEdit_Delete_Map_triggered();
    void on_actionEdit_Insert_Map_triggered();
    void on_actionEdit_Move_Map_triggered();
    void on_actionEdit_Goto_Map_triggered();
    void on_actionEdit_Test_Map_triggered();
    void on_actionFile_Import_Maps_triggered();
    void on_actionFile_Export_Map_triggered();
    void on_actionEdit_Rename_Map_triggered();
    void on_actionEdit_Last_Map_triggered();
    void on_actionEdit_First_Map_triggered();
    void on_actionFile_Generate_Report_triggered();
    void on_actionEdit_Map_States_triggered();
    void on_actionExport_Screenshots_triggered();
    void on_actionEdit_Edit_Messages_triggered();
    void on_actionEdit_Map_Properties_triggered();
    void updateStatus();
    void openRecentFile();
    void shiftUp();
    void shiftDown();
    void shiftLeft();
    void shiftRight();

private:
    void closeEvent(QCloseEvent *event) override;
    bool maybeSave();
    void setDocument(const QString);
    void warningMessage(const QString message);
    bool saveAs();
    bool save();
    void open(QString);
    bool updateTitle();
    void updateMenus();
    bool isDirty();
    void updateRecentFileActions();
    void reloadRecentFileActions();
    void initFileMenu();
    void initTilebox();
    void initMapShortcuts();
    void initToolBar();
    void initSelectorWidget();
    void initLayerBox();
    void updateWindowTitle();
    void setDirty(bool dirty);
    int currentTool();

    enum {
        MAX_RECENT_FILES = 12,
    };

    QString m_appName = tr("mapedit");
    QString m_allFilter = tr("All Supported Maps (*.dat *.cs3 *.map *.mapz)");
    Ui::MainWindow *ui;
    MapView *m_mapView;
    QComboBox *m_cbSkill;
    CMapFile m_doc;
    int m_hx = -1;
    int m_hy = -1;
    QActionGroup *m_toolGroup;
    QAction *m_recentFileActs[MAX_RECENT_FILES];
    QLabel *m_label;
    QLabel *m_label0;
};
#endif // MAINWINDOW_H
