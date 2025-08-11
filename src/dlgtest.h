#ifndef DLGTEST_H
#define DLGTEST_H

#include <QDialog>
#include "gamemixin.h"

class CMapFile;

namespace Ui
{
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
    void mainLoop() override;
    void changeZoom();
    void preloadAssets() override;

private:
    QTimer m_timer;
    Ui::CDlgTest *ui;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *) override;
    void exitGame();
    void sanityTest() override;
    void setZoom(bool zoom) override;
    void drawHelpScreen(CFrame &) override {}
    bool loadScores() override { return true; }
    bool saveScores() override { return true; }
    void stopMusic() override {}
    void startMusic() override {}
    void openMusicForLevel(int) override {}
    void setupTitleScreen() override {}
    void takeScreenshot() override {}
    void toggleFullscreen() override {}
    void manageTitleScreen() override {}
    void toggleGameMenu() override {};
    void manageGameMenu() override {}
    void load() override {}
    void save() override {}
};

#endif // DLGTEST_H
