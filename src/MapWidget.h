#pragma once

#include <QWidget>
#include <QPixmap>
#include <QCache>
#include <QTimer>
#include <memory>
#include <vector>
#include "runtime/map.h"
#include "runtime/shared/Frame.h"

class QPainter;
class QMouseEvent;
class QKeyEvent;
class MainWindow;

class MapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MapWidget(QWidget *parent = nullptr);
    ~MapWidget() override;

    // Set the current map (owned externally)
    void setMap(CMap *map);
    CMap *map() const { return m_map; }

    // Tool selection
    enum class Tool
    {
        None,
        Stamp,    // Place single or multi-tile stamp
        Picker,   // Pick tile under cursor
        Selection, // Rectangular selection (with move/copy later)
        Eraser
    };

    void setTool(Tool tool);
    Tool currentTool() const { return m_tool; }

    // Current tile(s) to paint with (for Stamp tool)
    void setCurrentTile(uint8_t tileId);
    void setCurrentTiles(const std::vector<uint8_t> &tileIds, int cols); // for multi-tile stamps

    // Zoom control
    void setZoom(int factor); // 1 = 100%, 2 = 200%, etc.
    int zoom() const { return m_zoom; }

    // Grid visibility
    void setGridVisible(bool visible);
    bool isGridVisible() const { return m_showGrid; }

    // Selection
    QRect currentSelection() const { return m_selection; }
    void clearSelection();

    // Access to tile pixmaps (from CFrameSet or similar)
    void setTileSet(const std::vector<CFrame *> &frames); // one frame per tile ID

    void fillSelection(uint8_t tileId = UINT8_MAX);   // UINT8_MAX = use current brush
    void preloadAssets();
    void setMainWindow(MainWindow *mw) { m_mainWindow = mw; }

signals:
    void tilePicked(uint8_t tileId);
    void selectionChanged(const QRect &rect); // in tile coordinates
    void mapModified();                       // emitted after commit

//signals:
    void copyRequested(const QRect &tileRect);
    void pasteRequested(const QPoint &atTilePos);
    void tilePropertiesRequested(int tileX, int tileY);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:

    // Convert between screen and map coordinates
    QPoint screenToTile(const QPoint &pos) const;
    QRect screenToTileRect(const QRect &rect) const;
    QPoint tileToScreen(const QPoint &tile) const;

    // Rendering helpers
    void drawMap(QPainter &painter);
    void drawGrid(QPainter &painter);
    void drawShadowTile(QPainter &painter);
    void drawSelectionRect(QPainter &painter);
    void createContextMenu();
    QColor attr2color(const uint8_t attr);
    QColor rgbaToQColor(const uint32_t rgba);

    // Tile pixmap cache: tileId → QPixmap (scaled)
    QPixmap getCachedPixmap(uint8_t tileId);

    // Commit current shadow stamp
    void commitStampAt(const QPoint &tilePos);

    void updateCursor();

    QMenu *m_contextMenu = nullptr;
    QPoint m_lastRightClickTile { -1, -1 };   // tile coordinates of the right-click
    CMap *m_map = nullptr;

    Tool m_tool = Tool::None;
    uint8_t m_currentTile = 0;
    std::vector<uint8_t> m_currentStamp; // for multi-tile stamps
    int m_stampCols = 1;
    int m_stampRows = 1;

    int m_zoom = 2; // 2x by default (32x32 visible tiles)
    bool m_showGrid = false;

    // Shadow preview (where mouse is)
    QPoint m_shadowTilePos{-1, -1};

    // Selection
    QRect m_selection; // in tile coords
    QPoint m_selectionStart{-1, -1};
    bool m_selecting = false;

    // Mouse state
    bool m_leftPressed = false;

    QTimer   m_flashTimer;
    bool     m_flashState = false;        // true = visible, false = hidden
    uint8_t m_attr = 0;
    int m_hx = 0;
    int m_hy = 0;

    // Tile rendering cache
    std::vector<CFrame *> m_tileFrames;     // owned externally
    QCache<uint8_t, QPixmap> m_pixmapCache; // key: tileId + zoom*1000

    MainWindow *m_mainWindow = nullptr;

};
