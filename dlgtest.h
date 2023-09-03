#ifndef DLGTEST_H
#define DLGTEST_H

#include <QDialog>
#include "gamemixin.h"

class CMapFile;

namespace Ui {
class CDlgTest;
}

class CDlgTest : public QDialog, public CGameMixin
{
    Q_OBJECT

public:
    explicit CDlgTest(QWidget *parent = nullptr);
    void init(CMapFile *mapfile);
    virtual ~CDlgTest();

protected slots:
    void mainLoop();
    void changeZoom();
    virtual void preloadAssets();

private:
    QTimer m_timer;
    Ui::CDlgTest *ui;
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);
    virtual void paintEvent(QPaintEvent *) ;
    virtual void exitGame();
    virtual void sanityTest();
    virtual void setZoom(bool zoom);
};

#endif // DLGTEST_H
