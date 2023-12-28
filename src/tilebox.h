#ifndef TILELBOX_H
#define TILELBOX_H

#include <QToolBox>

namespace Ui
{
    class CTileBox;
}

class QToolButton;

class CTileBox : public QToolBox
{
    Q_OBJECT

public:
    explicit CTileBox(QWidget *parent = nullptr);
    ~CTileBox();

private:
    void setupToolbox();
    int getTabId(int typeId);

signals:
    void tileChanged(int);

private slots:
    void buttonPressed(QAction *action);
    void setTile(int tile);

private:
    QToolButton **m_buttons;
    int m_tile;
    const QString &highlightStyle();

    enum
    {
        MAX_WIDTH = 150,
        MAX_HEIGHT = 400,
        MAX_COLS = 4,
        HIGHLIGHT_RED = 0x40,
        HIGHLIGHT_GREEN = 0xf0,
        HIGHLIGHT_BLUE = 0xf5,
    };
};

#endif // TILELBOX_H
