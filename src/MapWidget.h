#pragma once

#include <QWidget>
#include <QPixmap>
#include <QCache>
#include <QTimer>
#include <vector>
#include "runtime/map.h"
#include "runtime/shared/Frame.h"
#include "stamp.h"

class QPainter;
class QMouseEvent;
class QKeyEvent;
class MainWindow;
class CMapFile;
class PaintCommand;

class MapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MapWidget(QWidget *parent = nullptr, CMapFile *doc=nullptr);
    ~MapWidget() override;

    // Set the current map (owned externally)
    void setMap(CMap *map);
    CMap *map() const { return m_map; }

    // Tool selection
    enum class Tool
    {
        None,
        Stamp,     // Place single or multi-tile stamp
        Picker,    // Pick tile under cursor
        Selection, // Rectangular selection (with move/copy later)
        Eraser,
        Dice,
        FloodFill
    };

    void setTool(Tool tool);
    Tool currentTool() const { return m_tool; }

    // Zoom control
    void setZoom(int factor); // 1 = 100%, 2 = 200%, etc.
    int zoom() const { return m_zoom; }

    // Grid visibility
    void setGridVisible(bool visible);
    bool isGridVisible() const { return m_showGrid; }

    // Selection
    QRect currentSelection() const { return m_selection; }
    void clearSelection();

    void fillSelection(uint8_t tileId = UINT8_MAX); // UINT8_MAX = use current brush
    void preloadAssets();
    void setMainWindow(MainWindow *mw) { m_mainWindow = mw; }

    static inline QString toolName(Tool toolID) {
        switch(toolID) {
        case Tool::None:
            return "None";
        case Tool::Stamp:     // Place single or multi-tile stamp
            return "Stamp";
        case Tool::Picker:    // Pick tile under cursor
            return "Picker";
        case Tool::Selection: // Rectangular selection (with move/copy later)
            return "Selection";
        case Tool::Eraser:
            return "Erase";
        case Tool::Dice:
            return "Dice";
        case Tool::FloodFill:
            return "FloodFill";
        default:
            return "???";
        }
    }

signals:
    void tilePicked(uint8_t tileId);
    void selectionChanged(const QRect &rect); // in tile coordinates
    void mapModified();                       // emitted after commit

    // signals:
    void copyRequested(const QRect &tileRect);
    void pasteRequested(const QPoint &atTilePos);
    void tilePropertiesRequested(int tileX, int tileY);

public slots:
    // Current tile(s) to paint with (for Stamp tool)
    void setCurrentTile(uint8_t tileId);
    void setCurrentTiles(const std::vector<uint8_t> &tileIds, int cols); // for multi-tile stamps
    void setCurrentStamp(const Stamp &stamp);
    void changeActiveLayer(int layerID);
    void updateLayerVisibility(int layerID, bool visibility);
    void resetLayerVisibilityList();

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
    void drawShadowTile(QPainter &painter, const Stamp &stamp);
    void drawSelectionRect(QPainter &painter);
    void createContextMenu(QMenu *menu, const QPoint &point);
    QColor attr2color(const uint8_t attr);
    QColor rgbaToQColor(const uint32_t rgba);

    // Tile pixmap cache: tileId → QPixmap (scaled)
    QPixmap getCachedPixmap(uint8_t tileId, uint16_t baseId);

    // Commit current shadow stamp
    void commitStampAt(const QPoint &tilePos, const Stamp &stamp);
    void updateCursor();
    uint8_t pickDiceTile(const std::vector<uint8_t> & tiles);

    // Layers

    enum {
        MAIN_LAYER_ID = 0,
    };
    int m_activeLayer = MAIN_LAYER_ID;
    QList<bool> m_layerVisibilityList;
    inline bool validateBaseIDForCurrentLayer(uint16_t baseID) {
        if (m_activeLayer == MAIN_LAYER_ID && baseID == Stamp::MainTilesetBaseID)
            return true;
        else if (m_activeLayer != MAIN_LAYER_ID && baseID == Stamp::OtherTilesetBaseID)
            return true;
        else
            return false;
    }

    CMap *m_map = nullptr;
    Tool m_tool = Tool::None;
    uint8_t m_currentTile = 0;
    Stamp m_currentStamp{{0}, 1, 1, Stamp::MainTilesetBaseID};

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

    QTimer m_flashTimer;
    bool m_flashState = false; // true = visible, false = hidden
    uint8_t m_attr = 0;
    int m_hx = 0;
    int m_hy = 0;

    // Tile rendering cache
    std::vector<CFrame *> m_tileFrames;      // owned externally
    std::vector<CFrame *> m_tileOthers;      // owned externally
    QCache<uint16_t, QPixmap> m_pixmapCache; // key: tileId + zoom*1000

    MainWindow *m_mainWindow = nullptr;
    CMapFile   *m_doc;
};
