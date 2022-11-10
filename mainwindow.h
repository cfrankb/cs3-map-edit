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

    void on_actionOpen_triggered();

    void on_actionSave_as_triggered();

    void on_actionNew_Map_triggered();

    void on_actionResize_triggered();

private:
    virtual void closeEvent(QCloseEvent *event);

    bool maybeSave();
    void setDocument(const QString);
    void warningMessage(const QString message);
    bool saveAs();
    bool save();
    void open(QString);
    bool updateTitle();
    void updateMenus();
    //void setStatus(const QString msg);
    void updateStatus();
    bool isDirty();

    Ui::MainWindow *ui;
    CMapScroll *m_scrollArea;
    CMapFile m_doc;
};
#endif // MAINWINDOW_H
