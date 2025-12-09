// MapWidget.cpp

#include "MapWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>
#include <QClipboard>
#include <QMimeData>
#include <QTime>
#include <QMenu>
#include <QContextMenuEvent>
#include <QGuiApplication>
#include <random>
#include "runtime/shared/FrameSet.h"
#include "runtime/shared/Frame.h"
#include "runtime/shared/qtgui/qfilewrap.h"
#include "runtime/attr.h"
#include "runtime/color.h"
#include "runtime/states.h"
#include "runtime/statedata.h"
#include "dlgattr.h"
#include "dlgstat.h"
#include "layerdata.h"
#include "undo/MapCommand.h"

namespace MapWidgetPrivate
{
    constexpr const int TILE_SIZE = 16;
    constexpr const int PIXMAP_CACHE_SIZE = 768; // cache up to 768 scaled pixmaps
    constexpr const int ZOOM_CACHE_SPACE = 1024;
};

using namespace MapWidgetPrivate;

struct TileDelta
{
    uint8_t oldTile;
    uint8_t newTile;
};


class PaintCommand : public QUndoCommand
{
public:
    PaintCommand(CMapFile *doc, CLayer *layer, ToolType tool)
        : m_doc(doc), m_layer(layer), m_tool(tool)
    {
        setText(MapWidget::toolName(m_tool));
    }

    void recordChange(int x, int y, uint8_t oldTile, uint8_t newTile)
    {
        auto key = CMap::toKey(x, y);
        auto it = m_changes.find(key);
        if (it == m_changes.end())
        {
            // First time this tile is touched → store old and new
            m_changes[key] = {oldTile, newTile};
        }
        else
        {
            // Already touched → update only the new value
            it->second.newTile = newTile;
        }
    }

    ToolType tool() { return m_tool; }

    void undo() override
    {
        for (const auto &[key, delta] : m_changes)
        {
            const Pos pos = CMap::toPos(key);
            m_layer->set(pos.x, pos.y, delta.oldTile);
        }
        notify();
    }

    void redo() override
    {
        for (const auto &[key, delta] : m_changes)
        {
            const Pos pos = CMap::toPos(key);
            m_layer->set(pos.x, pos.y, delta.newTile);
        }
        notify();
    }

    bool isEmpty() const { return m_changes.empty(); }
    inline const std::unordered_map<uint32_t, TileDelta> &changes() const
    {
        return m_changes;
    };

private:
    void notify()
    {
        if (m_doc)
        {
            m_doc->setDirty(true);
            emit m_doc->dirtyChanged(true);
            emit m_doc->refreshMap();
        }
    }

    CMapFile *m_doc;
    CLayer *m_layer;
    ToolType m_tool;
    std::unordered_map<uint32_t, TileDelta> m_changes;
};

class FloodFillCommand : public QUndoCommand
{
public:
    FloodFillCommand(CMapFile *doc, CLayer *layer, int startX, int startY, uint8_t newValue, QUndoCommand *parent = nullptr)
        : QUndoCommand(parent), m_doc(doc), m_startX(startX), m_startY(startY), m_newValue(newValue)
    {
        setText(QObject::tr("Flood Fill"));
        m_layer = layer;

        // Snapshot the layer before fill
        m_backup = layer->tiles();
    }

    void redo() override
    {
        floodFill(m_layer, m_startX, m_startY, m_newValue);
        m_doc->setDirty(true);
        emit m_doc->refreshMap();
        emit m_doc->dirtyChanged(true);
    }

    void undo() override
    {
        m_layer->tilesFrom(m_backup);
        m_doc->setDirty(true);
        emit m_doc->refreshMap();
        emit m_doc->dirtyChanged(true);
    }

private:
    CLayer *m_layer;
    CMapFile *m_doc;
    int m_startX, m_startY;
    uint8_t m_newValue;
    std::vector<uint8_t> m_backup;

    void floodFill(CLayer *layer, int x, int y, uint8_t newValue)
    {
        // Basic BFS flood fill
        int width = layer->width();
        int height = layer->height();
        auto &tiles = layer->tiles();

        uint8_t oldValue = tiles[y * width + x];
        if (oldValue == newValue)
            return;

        std::queue<std::pair<int, int>> q;
        q.push({x, y});

        while (!q.empty())
        {
            auto [cx, cy] = q.front();
            q.pop();

            int idx = cy * width + cx;
            if (tiles[idx] != oldValue)
                continue;

            tiles[idx] = newValue;

            if (cx > 0)
                q.push({cx - 1, cy});
            if (cx < width - 1)
                q.push({cx + 1, cy});
            if (cy > 0)
                q.push({cx, cy - 1});
            if (cy < height - 1)
                q.push({cx, cy + 1});
        }
    }
};


class SetTileAttrCommand : public QUndoCommand
{
public:
    SetTileAttrCommand(CMapFile* doc, int x, int y,
                       uint8_t oldAttr, uint8_t newAttr,
                       QUndoCommand* parent = nullptr)
        : QUndoCommand(parent),
        m_doc(doc), m_x(x), m_y(y),
        m_oldAttr(oldAttr), m_newAttr(newAttr)
    {
        setText(QObject::tr("Set Tile Attribute"));
    }

    void undo() override {
        m_doc->map()->setAttr(m_x, m_y, m_oldAttr);
        notify();
    }

    void redo() override {
        m_doc->map()->setAttr(m_x, m_y, m_newAttr);
        notify();
    }

private:
    void notify() {
        if (m_doc) {
            m_doc->setDirty(true);
            emit m_doc->refreshMap();
            emit m_doc->dirtyChanged(true);
        }
    }

    CMapFile* m_doc;
    int m_x, m_y;
    uint8_t m_oldAttr, m_newAttr;
};


class SetPosCommand : public QUndoCommand
{
public:
    enum PosType { Start, Exit };

    SetPosCommand(CMapFile* doc, PosType type,
                  uint32_t oldPos, uint32_t newPos,
                  QUndoCommand* parent = nullptr)
        : QUndoCommand(parent),
        m_doc(doc), m_type(type),
        m_oldPos(oldPos), m_newPos(newPos)
    {
        setText(m_type == Start
                    ? QObject::tr("Set Start Position")
                    : QObject::tr("Set Exit Position"));
    }

    void undo() override {
        m_doc->map()->states().setU(posKey(), m_oldPos);
        notify();
    }

    void redo() override {
        m_doc->map()->states().setU(posKey(), m_newPos);
        notify();
    }

private:
    int posKey() const {
        return (m_type == Start) ? POS_ORIGIN : POS_EXIT;
    }

    void notify() {
        if (m_doc) {
            m_doc->setDirty(true);
            emit m_doc->refreshMap();
            emit m_doc->dirtyChanged(true);
        }
    }

    CMapFile* m_doc;
    PosType m_type;
    uint32_t m_oldPos;
    uint32_t m_newPos;
};



MapWidget::MapWidget(QWidget *parent, CMapFile *doc)
    : QWidget(parent), m_pixmapCache(PIXMAP_CACHE_SIZE)
{
    m_doc = doc;
    m_map = nullptr;
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(320, 240);

    QFont f("Courier New");            // or "Consolas", "DejaVu Sans Mono", etc.
    f.setStyleHint(QFont::TypeWriter); // forces monospaced
    f.setBold(true);
    setFont(f); // This sets the widget’s base font
    preloadAssets();

    m_flashTimer.setInterval(250);
    m_flashTimer.start();
    connect(&m_flashTimer, &QTimer::timeout, this, [this]()
            {
        m_flashState = !m_flashState;
     if (m_attr || m_hx || m_hy) {
            update();
     } });
}

MapWidget::~MapWidget() = default;

void MapWidget::setMap(CMap *map)
{
    if (m_map != map)
    {
        m_map = map;
        m_pixmapCache.clear();
        clearSelection();
    }
    update();
    resetLayerVisibilityList();
}

// Call it whenever tool changes
void MapWidget::setTool(ToolType tool)
{
    if (m_currentCommand && m_currentCommand->tool() != tool)
    {
        commitToolCmd();
    }

    qDebug("setTool %u", static_cast<uint8_t>(tool));
    if (m_tool != tool)
    {
        m_tool = tool;
        clearSelection();
        m_shadowTilePos = {-1, -1};
        update();
        updateCursor();
    }
}

void MapWidget::setCurrentTile(uint8_t tileId)
{
    qDebug("current tile: 0x%.2x", tileId);
    m_currentTile = tileId;
    m_currentStamp = Stamp{{tileId}, 1, 1, Stamp::MainTilesetBaseID};
    setTool(ToolType::Stamp);
    update();
}

void MapWidget::setCurrentTiles(const std::vector<uint8_t> &tileIds, int cols)
{
    const int rows = (tileIds.size() + cols - 1) / cols;
    m_currentStamp = Stamp{tileIds, cols, rows, Stamp::MainTilesetBaseID};
    update();
}

void MapWidget::setCurrentStamp(const Stamp &stamp)
{
    LOGI("current stamp: tiles: %lu  baseID: %.4x", stamp.tiles.size(), stamp.baseID);
    m_currentStamp = stamp;
    update();
}

void MapWidget::setZoom(int factor)
{
    if (factor < 1)
        factor = 1;
    if (factor > 4)
        factor = 4;
    if (m_zoom != factor)
    {
        m_zoom = factor;
        m_pixmapCache.clear();
        update();
    }
}

void MapWidget::setGridVisible(bool visible)
{
    qDebug("setGridVisible: %d", visible);
    if (m_showGrid != visible)
    {
        m_showGrid = visible;
        update();
    }
}

void MapWidget::preloadAssets()
{
    auto addTileSet = [this](uint16_t baseID, const std::vector<CFrame *> &frames) {
        m_frameSetLookup[m_frameSetCount++] = {baseID, std::move(frames)};
    };

    QFileWrap file;

    //////////////////////////////////////////////////
    // tileset for mainLayer
    const char filenameTiles[] = ":/data/tiles.obl";
    std::unique_ptr<CFrameSet> frameSet = std::make_unique<CFrameSet>();
    if (!file.open(filenameTiles, "rb"))
    {
        LOGE("can't open %s", filenameTiles);
    }
    else
    {
        LOGI("reading %s", filenameTiles);
        if (frameSet->extract(file))
        {
            LOGI("extracted: %lu", (frameSet)->getSize());
        }
        else
        {
            LOGE("failed to extract frames");
        }
        file.close();
        addTileSet(Stamp::MainTilesetBaseID, frameSet->frames());
        frameSet->removeAll();
    }

    /////////////////////////////////////////
    // tileset for other layers
    const char filenameLayers[] = ":/data/cs3layers.png";
    if (!file.open(filenameLayers, "rb"))
    {
        LOGE("can't open %s", filenameLayers);
    }
    else
    {
        LOGI("reading %s", filenameLayers);
        if (frameSet->extract(file))
        {
            LOGI("extracted: %lu", (frameSet)->getSize());
        }
        else
        {
            LOGE("failed to extract frames");
        }
        file.close();
        // single image. split into individual tiles
        CFrame *frame = (*frameSet.get())[0];
        CFrameSet *splitSet = frame->split(TILE_SIZE, TILE_SIZE);
        LOGI("tiles in others: %lu", splitSet->getSize());
        addTileSet(Stamp::OtherTilesetBaseID, splitSet->frames());
        splitSet->removeAll();
        delete splitSet;
    }

    LOGI("m_frameSetCount: %lu", m_frameSetCount);

    // Force resize in case zoom > 1 and map already exists
    if (map())
    {
        int tileSize = TILE_SIZE * zoom();
        resize(
            map()->width() * tileSize,
            map()->height() * tileSize);
    }

    /*
    constexpr const char fontName [] = ":/data/bitfont.bin";
    int size = 0;
    if (file.open(fontName, "rb")) {
        size = file.getSize();
        m_fontData.resize(size);// = new uint8_t[size];
        if (!file.read(m_fontData.data(), size)) {
            LOGE("failed to read font");
        }
        file.close();
        LOGI("loading font: %d bytes", size);
    } else {
        LOGE("failed to open %s", fontName);
    }
    */
}

QPoint MapWidget::screenToTile(const QPoint &pos) const
{
    if (!m_map)
        return {-1, -1};
    const int tileSize = TILE_SIZE * m_zoom;
    return {pos.x() / tileSize, pos.y() / tileSize};
}

QRect MapWidget::screenToTileRect(const QRect &rect) const
{
    QPoint tl = screenToTile(rect.topLeft());
    QPoint br = screenToTile(rect.bottomRight());
    return QRect(tl, br);
}

QPoint MapWidget::tileToScreen(const QPoint &tile) const
{
    const int s = TILE_SIZE * m_zoom;
    return {tile.x() * s, tile.y() * s};
}

QPixmap MapWidget::getCachedPixmap(uint8_t tileID, uint16_t baseID)
{
    auto getFrameSet = [this] (const uint16_t &baseID) -> std::vector<CFrame *>*
    {
        for (size_t i = 0; i < m_frameSetCount; ++i) {
            if (baseID == m_frameSetLookup[i].baseID)
                return &m_frameSetLookup[i].frames;
        }
        return nullptr;
    };

    const std::vector<CFrame *>* frames = getFrameSet(baseID);
    if (!frames || tileID >= frames->size())
        return QPixmap();

    const uint32_t cacheKey = tileID + baseID + m_zoom * ZOOM_CACHE_SPACE;
    QPixmap *cached = m_pixmapCache.object(cacheKey);
    if (cached)
        return *cached;

    CFrame *frame = (*frames)[tileID];
    if (frame == nullptr)
        return QPixmap();
    // if (frame->isEmpty())
    //   return QPixmap();

    const QImage img(reinterpret_cast<uint8_t *>(frame->getRGB().data()),
                     frame->width(), frame->height(),
//                     QImage::Format_RGBA8888_Premultiplied); // or RGBX8888
                     QImage::Format_RGBA8888); // or RGBX8888

    const QPixmap pixmap = QPixmap::fromImage(img).scaled(
        TILE_SIZE * m_zoom, TILE_SIZE * m_zoom, Qt::IgnoreAspectRatio, Qt::FastTransformation);

    m_pixmapCache.insert(cacheKey, new QPixmap(pixmap));
    return pixmap;
}

void MapWidget::paintEvent(QPaintEvent *)
{
    if (!m_map)
    {
        QPainter p(this);
        p.fillRect(rect(), QColor(50, 50, 60));
        p.setPen(Qt::white);
        p.drawText(rect(), Qt::AlignCenter, tr("No map loaded"));
        return;
    }

    QPainter painter(this);

    drawMap(painter);

    if (m_showGrid)
        drawGrid(painter);

    const Stamp &stamp = m_currentStamp;
    if (m_tool == ToolType::Selection && m_selection.isValid())
        drawSelectionRect(painter);
    else if (m_shadowTilePos.x() >= 0 && (m_tool == ToolType::Stamp))
        drawShadowTile(painter, m_currentStamp);
    else if (m_shadowTilePos.x() >= 0 && (m_tool == ToolType::Eraser))
        drawShadowTile(painter, Stamp{{0}, 1, 1, Stamp::MainTilesetBaseID});
    else if (m_shadowTilePos.x() >= 0 && (m_tool == ToolType::FloodFill))
        drawShadowTile(painter, Stamp{{0}, 1, 1, Stamp::MainTilesetBaseID});
    else if (m_shadowTilePos.x() >= 0 && (m_tool == ToolType::Dice) && (stamp.tiles.size() != 0))
        drawShadowTile(painter, Stamp{{stamp.tiles[0]}, 1, 1, stamp.baseID});
}

void MapWidget::drawMap(QPainter &painter)
{
    if (!m_map)
        return;

    const int tileSize = TILE_SIZE * m_zoom;

    // draw map background color
    painter.fillRect(rect(), Qt::black); // Qt::darkGray);

    auto drawRect = [&painter](const auto &color, const auto &tileRect, const int width)
    {
        painter.setPen(QPen(color, width, Qt::SolidLine));
        painter.setBrush(Qt::NoBrush);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.drawRect(tileRect);
    };

    auto drawTile = [this, tileSize, &painter](const uint8_t tileID, const auto baseID, const int x, const int y)
    {
        QPixmap pm = getCachedPixmap(tileID, baseID);
        if (!pm.isNull())
        {
            painter.drawPixmap(x * tileSize, y * tileSize, pm);
        }
    };

    QRect visible = rect().adjusted(-tileSize, -tileSize, tileSize, tileSize);
    QPoint topLeftTile = screenToTile(visible.topLeft());
    QPoint bottomRightTile = screenToTile(visible.bottomRight());
    const auto &states = m_map->statesConst();
    const uint16_t startPos = states.getU(POS_ORIGIN);
    const uint16_t exitPos = states.getU(POS_EXIT);

    QFont font = painter.font();
    font.setPixelSize(12 * m_zoom);
    painter.setFont(font);

    for (int y = topLeftTile.y(); y <= bottomRightTile.y() && y < m_map->height(); ++y)
    {
        if (y < 0)
            continue;
        for (int x = topLeftTile.x(); x <= bottomRightTile.x() && x < m_map->width(); ++x)
        {
            if (x < 0)
                continue;

            // draw layers
            size_t i = m_map->layerCount();
            do {
                --i;
                if (!m_layerVisibilityList[i])
                    continue;
                const CLayer *layer = m_map->getLayer(i);
                const uint8_t tileID = layer->at(x, y);
                if (tileID)
                    drawTile(tileID, layer->baseID(), x, y);
            } while ( i != 0);

            const QRect tileRect(x * tileSize, y * tileSize, tileSize, tileSize);
            uint8_t attr = m_map->getAttr(x, y);
            if (attr)
            {
                QString text = QString("%1").arg(attr, 2, 16, QChar('0')).toUpper();

                // Black shadow/outline
                painter.setPen(Qt::black);
                painter.drawText(tileRect, Qt::AlignCenter, text);

                // Bright color on top
                painter.setPen(attr2color(attr));
                painter.drawText(tileRect.adjusted(-2, -2, -2, -2), Qt::AlignCenter, text);
            }

            if (m_flashState && m_attr != 0 && attr == m_attr)
            {
                QPen pen(rgbaToQColor(PINK), 3);
                pen.setCosmetic(false);
                painter.setPen(pen);
                painter.setBrush(Qt::NoBrush);

                int offset = (QTime::currentTime().msec() / 200) % 2 ? 3 : 0;
                painter.drawRect(tileRect.adjusted(-offset, -offset, offset, offset));
            }
            if (m_hx != 0 && m_hy != 0 && x == m_hx && y == m_hy)
            {
                drawRect(rgbaToQColor(CORAL), tileRect, 3);
            }
            if (startPos && startPos == CMap::toKey(x, y))
            {
                drawRect(rgbaToQColor(YELLOW), tileRect, 2);
            }
            if (exitPos && exitPos == CMap::toKey(x, y))
            {
                drawRect(rgbaToQColor(RED), tileRect, 2);
            }
        }
    }
}

void MapWidget::drawGrid(QPainter &painter)
{
    const int tileSize = TILE_SIZE * m_zoom;
    painter.setPen(QColor(255, 255, 0, 80));

    for (int x = 0; x <= m_map->width(); ++x)
    {
        int sx = x * tileSize;
        painter.drawLine(sx, 0, sx, height());
    }
    for (int y = 0; y <= m_map->height(); ++y)
    {
        int sy = y * tileSize;
        painter.drawLine(0, sy, width(), sy);
    }
}

void MapWidget::drawShadowTile(QPainter &painter, const Stamp &stamp)
{
    if (m_shadowTilePos.x() < 0 || m_shadowTilePos.x() >= m_map->width() ||
        m_shadowTilePos.y() < 0 || m_shadowTilePos.y() >= m_map->height())
        return;

    const int tileSize = TILE_SIZE * m_zoom;
    painter.setOpacity(0.6);

    for (int dy = 0; dy < stamp.rows; ++dy)
    {
        for (int dx = 0; dx < stamp.cols; ++dx)
        {
            int tx = m_shadowTilePos.x() + dx;
            int ty = m_shadowTilePos.y() + dy;
            if (tx >= m_map->width() || ty >= m_map->height())
                continue;

            size_t idx = dy * stamp.cols + dx;
            if (idx >= stamp.tiles.size())
                break;

            uint8_t tileId = stamp.tiles[idx];
            QPixmap pm = getCachedPixmap(tileId, stamp.baseID); // Stamp::MainTilesetBaseID
            if (!pm.isNull())
            {
                painter.drawPixmap(tx * tileSize, ty * tileSize, pm);
            }
        }
    }
    painter.setOpacity(1.0);
}

void MapWidget::drawSelectionRect(QPainter &painter)
{
    const int tileSize = TILE_SIZE * m_zoom;
    QRect r(
        m_selection.x() * tileSize,
        m_selection.y() * tileSize,
        m_selection.width() * tileSize,
        m_selection.height() * tileSize);
    painter.setPen(QPen(Qt::yellow, 2));
    painter.setBrush(QColor(255, 255, 0, 30));
    painter.drawRect(r);
}

void MapWidget::commitStampAt(const QPoint &tilePos, const Stamp &stamp)
{
    if (!m_map || stamp.tiles.empty())
        return;

    CLayer* layer = getActiveLayer();
    if (!layer)
    {
        LOGE("returned invalid layer");
        return;
    }

    bool changed = false;
    for (int dy = 0; dy < stamp.rows; ++dy)
    {
        for (int dx = 0; dx < stamp.cols; ++dx)
        {
            int tx = tilePos.x() + dx;
            int ty = tilePos.y() + dy;
            if (!layer->isValid(tx, ty))
                continue;

            size_t idx = dy * stamp.cols + dx;
            if (idx >= stamp.tiles.size())
                break;

            uint8_t newTile = stamp.tiles[idx];
            uint8_t oldTile = layer->at(tx, ty);
            if (oldTile != newTile)
            {
                m_currentCommand->recordChange(tx, ty, oldTile, newTile);
                layer->set(tx, ty, newTile);
                changed = true;
            }
        }
    }

    if (changed)
    {
        update();
        emit mapModified();
    }
}

void MapWidget::mousePressEvent(QMouseEvent *event)
{
    if (!m_map)
        return;

    QPoint tilePos = screenToTile(event->pos());

    if (event->button() == Qt::LeftButton)
    {
        if (tilePos.x() >= m_map->width() || tilePos.y() >= m_map->height())
            return;

        m_leftPressed = true;
        const Stamp &stamp = m_currentStamp;
        const bool canStamp = validateBaseIDForCurrentLayer(m_currentStamp.baseID) && stamp.tiles.size() != 0;
        if (canStamp)
        {
            if (m_tool == ToolType::Stamp)
            {
                startToolCmd(m_tool);
                commitStampAt(tilePos, m_currentStamp);
            }
            else if (m_tool == ToolType::FloodFill &&
                     (m_doc && m_doc->map() && m_doc->map()->at(tilePos.x(), tilePos.y()) != stamp.tiles[0]))
            {
                CLayer* layer = getActiveLayer();
                auto cmd = new FloodFillCommand(m_doc, layer, tilePos.x(), tilePos.y(), stamp.tiles[0]);
                m_doc->activeStack()->push(cmd);
                update();
            }
            else if (m_tool == ToolType::Dice)
            {
                startToolCmd(m_tool);
                uint8_t tileID = randomTile(stamp.tiles);
                commitStampAt(tilePos, Stamp{{tileID}, 1, 1, stamp.baseID});
            }
        }

        if (m_tool == ToolType::Eraser)
        {
            startToolCmd(m_tool);
            commitStampAt(tilePos, Stamp{{0}, 1, 1, Stamp::MainTilesetBaseID});
        }
        else if (m_tool == ToolType::Picker)
        {
            if (m_map->isValid(tilePos.x(), tilePos.y()))
            {
                emit tilePicked(m_map->at(tilePos.x(), tilePos.y()));
            }
        }
        else if (m_tool == ToolType::Selection)
        {
            m_selectionStart = tilePos;
            m_selection = QRect(tilePos, QSize(1, 1));
            m_selecting = true;
            emit selectionChanged(m_selection);
            update();
        }
    }
}

void MapWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_map)
        return;

    QPoint tilePos = screenToTile(event->pos());
    m_shadowTilePos = tilePos;
    const Stamp &stamp = m_currentStamp;
    bool canStamp = validateBaseIDForCurrentLayer(m_currentStamp.baseID) && stamp.tiles.size() != 0;

    if (m_leftPressed)
    {
        if (canStamp)
        {
            if (m_tool == ToolType::Stamp)
            {
                commitStampAt(tilePos, m_currentStamp);
            }
            else if (m_tool == ToolType::Dice)
            {
                uint8_t tileID = randomTile(stamp.tiles);
                commitStampAt(tilePos, Stamp{{tileID}, 1, 1, stamp.baseID});
            }
            else if (m_tool == ToolType::FloodFill && m_doc && m_doc->map() && m_doc->map()->at(tilePos.x(), tilePos.y()) != stamp.tiles[0])
            {
                if (m_doc && m_doc->map() && m_doc->map()->at(tilePos.x(), tilePos.y()) != stamp.tiles[0])
                {
                    CLayer* layer = getActiveLayer();
                    auto cmd = new FloodFillCommand(m_doc, layer, tilePos.x(), tilePos.y(), stamp.tiles[0]);
                    m_doc->activeStack()->push(cmd);
                    update();
                }
            }
        }

        if (m_leftPressed && m_tool == ToolType::Eraser)
        {
            commitStampAt(tilePos, Stamp{{0}, 1, 1, Stamp::MainTilesetBaseID});
        }
        else if (m_selecting && m_tool == ToolType::Selection)
        {
            QRect newSel(m_selectionStart, tilePos);
            newSel = newSel.normalized();
            if (newSel != m_selection)
            {
                m_selection = newSel;
                emit selectionChanged(m_selection);
                update();
            }
        }
    }

    update(); // for shadow tile

    QString str = QString("x: %1 y: %2").arg(tilePos.x()).arg(tilePos.y());
    emit statusChanged(str);
}

void MapWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        // commit composite tool (if any)
        commitToolCmd();

        m_leftPressed = false;
        if (m_selecting)
        {
            m_selecting = false;
        }
    }
}

void MapWidget::leaveEvent(QEvent *)
{
    m_shadowTilePos = {-1, -1};
    commitToolCmd();
    update();
}

void MapWidget::clearSelection()
{
    if (m_selection.isValid())
    {
        m_selection = QRect();
        emit selectionChanged(m_selection);
        update();
    }
}

void MapWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        clearSelection();
    }
    QWidget::keyPressEvent(event);
}

void MapWidget::updateCursor()
{
    switch (m_tool)
    {
    case ToolType::Stamp:
        setCursor(QCursor(QPixmap(":/data/cursors/sketchpntbrush.png"), 9, 31));
        break;
    case ToolType::Dice:
        setCursor(QCursor(QPixmap(":/data/icons/1439410433.png"), 14, 32));
        break;
    case ToolType::Picker:
        setCursor(QCursor(QPixmap(":/data/cursors/eyedropper.png"), 2, 14));
        break;
    case ToolType::Selection:
        setCursor(Qt::CrossCursor);
        break;
    case ToolType::Eraser:
        setCursor(QCursor(QPixmap(":/data/cursors/efface.png"), 10, 27));
        break;
    case ToolType::FloodFill:
        setCursor(QCursor(QPixmap(":/data/cursors/paint_bucket.png"), 4, 17));
        break;
    default:
        setCursor(Qt::ArrowCursor);
        break;
    }
}

void MapWidget::createContextMenu(QMenu *menu, const QPoint &point)
{
    // mapEdit actions
    QAction *actionSetAttr = new QAction(tr("Set raw attribute"), this);
    actionSetAttr->setStatusTip(tr("Set the raw attribute for this tile"));
    connect(actionSetAttr, &QAction::triggered, this, [this, point]()
            {
        const int x = point.x();
        const int y = point.y();
        const uint8_t originalAttr = m_map->getAttr(x,y );
        CDlgAttr dlg(this);
        dlg.attr(originalAttr);
        if (dlg.exec() == QDialog::Accepted)
        {
            const uint8_t newAttr = dlg.attr();
            if (originalAttr != newAttr) {
                // Push undo command instead of direct mutation
                m_doc->undoGroup()->activeStack()->push(
                    new SetTileAttrCommand(m_doc, x, y, originalAttr, newAttr));
            }
        }
        update(); });
    menu->addAction(actionSetAttr);

    QAction *actionHighlight = new QAction(tr("highlight attribute"), this);
    actionHighlight->setStatusTip(tr("hightlight this attribute"));
    menu->addAction(actionHighlight);
    connect(actionHighlight, &QAction::triggered, this, [this, point]()
            {
        if (m_map) {
            m_attr =  m_map->getAttr(point.x(), point.y());
            qDebug("m_attr : %.2x", m_attr);
        }
        update(); });

    QAction *actionHighlightXY = new QAction(tr("highlight this position"), this);
    actionHighlightXY->setStatusTip(tr("hightlight this position"));
    menu->addAction(actionHighlightXY);
    connect(actionHighlightXY, &QAction::triggered, this, [this, point]()
            {
        if (m_map) {
            m_hx = point.x();
            m_hy = point.y();
            qDebug("m_hx : %.2x  -- m_hy : %.2x", m_hx, m_hy);
        }
        update(); });

    QAction *actionStatTile = new QAction(tr("see tile stats"), this);
    menu->addAction(actionStatTile);
    connect(actionStatTile, &QAction::triggered, this, [this, point]()
            {
        const int x = point.x();
        const int y = point.y();
        CDlgStat dlg(m_map->at(x, y), m_map->getAttr(x, y), this);
        dlg.setWindowTitle(tr("Tile Statistics"));
        dlg.exec(); });
    actionStatTile->setStatusTip(tr("Show the data information on this tile"));
    menu->addSeparator();

    QAction *actionSetStartPos = new QAction(tr("set start pos"), this);
    connect(actionSetStartPos, &QAction::triggered, this, [this, point]()
            {
            const uint32_t oldPos = m_map->states().getU(POS_ORIGIN);
            const uint32_t newPos = CMap::toKey(point.x(), point.y());

            if (oldPos != newPos) {
                m_doc->undoGroup()->activeStack()->push(
                    new SetPosCommand(m_doc, SetPosCommand::Start, oldPos, newPos));
            }
           emit mapModified();
           update(); });
    actionSetStartPos->setStatusTip(tr("Set the start position for this map"));
    menu->addAction(actionSetStartPos);

    QAction *actionSetExitPos = new QAction(tr("set exit pos"), this);
    connect(actionSetExitPos, &QAction::triggered, this, [this, point]()
            {
        const uint32_t oldPos = m_map->states().getU(POS_EXIT);
        const uint32_t newPos = CMap::toKey(point.x(), point.y());
        if (oldPos != newPos) {
            m_doc->undoGroup()->activeStack()->push(
                new SetPosCommand(m_doc, SetPosCommand::Exit, oldPos, newPos));
        }
        emit mapModified();
        update(); });
    actionSetExitPos->setStatusTip(tr("Set the exit position for this map."));
    menu->addAction(actionSetExitPos);
}

void MapWidget::createSelectionMenu(QMenu *menu, const QPoint &point)
{
    menu->addSeparator();
    QMenu *selectMenu = menu->addMenu(tr("selection"));

    bool isSelectionValid = currentSelection().isValid(); // !m_selection.isEmpty();

    // ─── Common actions ─────────────────────────────────────
    QAction *copy = selectMenu->addAction(QIcon::fromTheme("edit-copy"), tr("&Copy"));
    QAction *cut = selectMenu->addAction(QIcon::fromTheme("edit-cut"), tr("Cu&t"));
    QAction *paste = selectMenu->addAction(QIcon::fromTheme("edit-paste"), tr("&Paste"));
    QAction *del = selectMenu->addAction(QIcon::fromTheme("edit-delete"), tr("Delete"));
    QAction *fill = selectMenu->addAction(QIcon(":/data/icons/water_drop_1.png"), tr("Fill"));

    copy->setEnabled(isSelectionValid);
    cut->setEnabled(isSelectionValid);
    paste->setEnabled(isSelectionValid);
    del->setEnabled(isSelectionValid);
    fill->setEnabled(isSelectionValid);

    QAction *selectAll = selectMenu->addAction(tr("Select &All"), QKeySequence::SelectAll);
    QAction *clearSel = selectMenu->addAction(tr("Clear Selection"));
    clearSel->setEnabled(isSelectionValid);

    menu->addSeparator();

    // ─── Connect actions ─────────────────────────────────────
    connect(copy, &QAction::triggered, this, [this]()
            {
        if (currentSelection().isValid())
            emit copyRequested(currentSelection()); });

    connect(cut, &QAction::triggered, this, [this]()
            {
        if (currentSelection().isValid()) {
            emit copyRequested(currentSelection());
            fillSelection(0);               // or your erase tile
            clearSelection();
            emit mapModified();
        } });

    connect(paste, &QAction::triggered, this, [this, point]()
            { emit pasteRequested(point); });

    connect(del, &QAction::triggered, this, [this]()
            {
        if (currentSelection().isValid()) {
            fillSelection(0);
            clearSelection();
            emit mapModified();
        } });

    connect(fill, &QAction::triggered, this, [this]()
            {
        if (currentSelection().isValid()) {
            fillSelection(UINT8_MAX);
            clearSelection();
            emit mapModified();
        } });

    connect(selectAll, &QAction::triggered, this, [this]()
            {
        if (m_map) {
            QRect all(0, 0, m_map->width(), m_map->height());
            m_selection = all;
            emit selectionChanged(all);
            update();
        } });

    connect(clearSel, &QAction::triggered, this, &MapWidget::clearSelection);
    // Disable paste if clipboard is empty (optional polish)
    // connect(qApp->clipboard(), &QClipboard::dataChanged, this, [this, paste]()
    //       { paste->setEnabled(qApp->clipboard()->mimeData()->hasImage() ||
    //                         qApp->clipboard()->mimeData()->hasFormat("application/x-lgck-tiledata")); });
}

void MapWidget::contextMenuEvent(QContextMenuEvent *event)
{
    if (!m_map)
        return;

    // Convert click position → tile coordinates
    const QPoint tilePos = screenToTile(event->pos());
    if (!m_map->isValid(tilePos.x(), tilePos.y()))
        return;

    QMenu menu(this);
    createContextMenu(&menu, tilePos);
    menu.exec(event->globalPos());
}

void MapWidget::fillSelection(uint8_t tileId /* = UINT8_MAX */)
{
    CLayer *layer = getActiveLayer();
    if (!m_map || !layer)
        return;

    // If no explicit tileId given → use the current brush
    if (tileId == UINT8_MAX)
    {
        // For multi-tile stamps we just use the top-left tile (most common)
        if (!m_currentStamp.tiles.empty())
            tileId = m_currentStamp.tiles[0];
        else
            tileId = m_currentTile;
    }

    QRect area = m_selection;
    if (!area.isValid())
    {
        // nothing selected → fill entire map
        area = QRect(0, 0, layer->width(), layer->height());
    }

    bool changed = false;
    // Fastest possible loop — no function calls inside
    for (int y = area.top(); y <= area.bottom(); ++y)
    {
        for (int x = area.left(); x <= area.right(); ++x)
        {
            if (layer->at(x, y) != tileId)
            {
                layer->set(x, y, tileId);
                changed = true;
            }
        }
    }
    if (changed)
    {
        update(area.adjusted(-1, -1, 1, 1).intersected(rect())); // repaint only the affected region + 1px border
        emit mapModified();
    }
}

QColor MapWidget::rgbaToQColor(const uint32_t color)
{
    return QColor(color & 0xff, (color & 0xff00) >> 8, (color & 0xff0000) >> 16);
}

QColor MapWidget::attr2color(const uint8_t attr)
{
    auto getColor = [](auto attr)
    {
        if (RANGE(attr, ATTR_MSG_MIN, ATTR_MSG_MAX))
        {
            return CYAN;
        }
        else if (RANGE(attr, ATTR_IDLE_MIN, ATTR_IDLE_MAX))
        {
            return HOTPINK;
        }
        else if (attr == ATTR_FREEZE_TRAP)
        {
            return LIGHTGRAY;
        }
        else if (attr == ATTR_TRAP)
        {
            return RED;
        }
        else if (RANGE(attr, ATTR_CRUSHER_MIN, ATTR_CRUSHER_MAX))
        {
            return ORANGE;
        }
        else if (RANGE(attr, ATTR_BOSS_MIN, ATTR_BOSS_MAX))
        {
            return SEAGREEN;
        }
        else if (attr > PASSAGE_ATTR_MAX)
        {
            return OLIVE; // undefined behavior
        }
        else if (RANGE(attr, SECRET_ATTR_MIN, SECRET_ATTR_MAX))
        {
            return GREEN;
        }
        else if (RANGE(attr, PASSAGE_REG_MIN, PASSAGE_REG_MAX))
        {
            return YELLOW;
        }
        else
        {
            return WHITE;
        }
    };
    const uint32_t color = getColor(attr);
    return rgbaToQColor(color);
}

void MapWidget::changeActiveLayer(int layerID)
{
    LOGI("changeActiveLayer %d", layerID);
    m_activeLayer = layerID;
}

void MapWidget::resetLayerVisibilityList()
{
    size_t layerCount = m_map->layerCount();
    m_layerVisibilityList.resize(layerCount);
    for (auto &layerVisibility : m_layerVisibilityList)
    {
        layerVisibility = true;
    }
    changeActiveLayer(MAIN_LAYER_ID); // reset to default
}

void MapWidget::updateLayerVisibility(int layerID, bool visibility)
{
    if (layerID >= 0 && layerID < m_layerVisibilityList.size())
        m_layerVisibilityList[layerID] = visibility;

    update();
}

uint8_t MapWidget::randomTile(const std::vector<uint8_t> &tiles)
{
    //  Create random generator
    static std::random_device rd;
    static std::mt19937 gen(rd());

    // Compute total weight
    double totalWeight = 0.0;
    for (const auto &tileID : tiles)
    {
        const auto &def = getLayerTileDef(tileID);
        totalWeight += def.weight;
    }

    // Uniform distribution in [0, totalWeight)
    std::uniform_real_distribution<> dist(0.0, totalWeight);
    double r = dist(gen);

    // Find the item corresponding to r
    double cumulative = 0.0;
    for (const auto &tileID : tiles)
    {
        const auto &def = getLayerTileDef(tileID);
        cumulative += def.weight;
        if (r < cumulative)
        {
            return tileID;
        }
    }
    // Fallback (shouldn't happen if weights > 0)
    return tiles.front();
}

QString MapWidget::toolName(ToolType toolID)
{
    switch (toolID)
    {
    case ToolType::None:
        return "None";
    case ToolType::Stamp: // Place single or multi-tile stamp
        return "Paint Stamp";
    case ToolType::Picker: // Pick tile under cursor
        return "Picker";
    case ToolType::Selection: // Rectangular selection (with move/copy later)
        return "Selection";
    case ToolType::Eraser:
        return "Erase Tiles";
    case ToolType::Dice:
        return "Random Tool";
    case ToolType::FloodFill:
        return "FloodFill";
    default:
        return "???";
    }
}

bool MapWidget::isCombinedTool(const ToolType type)
{
    return type == ToolType::Stamp || type == ToolType::Dice || type == ToolType::Eraser;
}

void MapWidget::startToolCmd(const ToolType tool)
{
    CLayer *layer = getActiveLayer();
    if (isCombinedTool(tool) && layer)
    {
        m_currentCommand = new PaintCommand(m_doc, layer, tool);
    }
}

void MapWidget::commitToolCmd()
{
    if (m_currentCommand)
    {
        LOGI("commit tool: %d [%s] [size: %lu]", static_cast<int>(m_tool), toolName(m_tool).toStdString().c_str(), m_currentCommand->changes().size());
        if (!m_currentCommand->isEmpty())
        {
            m_doc->undoGroup()->activeStack()->push(m_currentCommand);
        }
        else
        {
            delete m_currentCommand;
        }
        m_currentCommand = nullptr;
    }
}
