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
#include "runtime/shared/FrameSet.h"
#include "runtime/shared/Frame.h"
#include "runtime/shared/qtgui/qfilewrap.h"
#include "mainwindow.h"
#include "runtime/attr.h"
#include "runtime/color.h"
#include "runtime/states.h"
#include "runtime/statedata.h"
#include "dlgattr.h"
#include "dlgstat.h"

MapWidget::MapWidget(QWidget *parent)
    : QWidget(parent), m_pixmapCache(500) // cache up to 500 scaled pixmaps
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(320, 240);

    QFont f("Courier New");        // or "Consolas", "DejaVu Sans Mono", etc.
    //QFont f("Consolas");        // or "Consolas", "DejaVu Sans Mono", etc.
    f.setStyleHint(QFont::TypeWriter);  // forces monospaced
    f.setBold(true);
    setFont(f);                    // This sets the widget’s base font

    m_flashTimer.setInterval(250);
    m_flashTimer.start();
    connect(&m_flashTimer, &QTimer::timeout, this, [this]() {
        m_flashState = !m_flashState;
     if (m_attr || m_hx || m_hy) {
            update();
     }
    });

    preloadAssets();
  //  preloadCursors();
}

MapWidget::~MapWidget() = default;

void MapWidget::setMap(CMap *map)
{
   // m_mapView->ensureVisible(0, 0, 0, 0);
    if (m_map != map)
    {
        m_map = map;
        m_pixmapCache.clear();
        update();
        clearSelection();
    }
}

// Call it whenever tool changes
void MapWidget::setTool(Tool tool)
{
    qDebug("setTool %u", tool);

    if (m_tool != tool) {
        m_tool = tool;
        clearSelection();
        m_shadowTilePos = {-1, -1};
        update();
        updateCursor();                // ← important!
    }
}

void MapWidget::setCurrentTile(uint8_t tileId)
{
    qDebug("current tile: 0x%.2x", tileId );

    m_currentTile = tileId;
    m_currentStamp = {tileId};
    m_stampCols = m_stampRows = 1;
    update();
}

void MapWidget::setCurrentTiles(const std::vector<uint8_t> &tileIds, int cols)
{
    m_currentStamp = tileIds;
    m_stampCols = cols;
    m_stampRows = (tileIds.size() + cols - 1) / cols;
    update();
}

void MapWidget::setZoom(int factor)
{
    if (factor < 1)
        factor = 1;
    if (factor > 8)
        factor = 8;
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

void MapWidget::setTileSet(const std::vector<CFrame *> &frames)
{
    m_tileFrames = frames;
    m_pixmapCache.clear();
    update();
}



void MapWidget::preloadAssets()
{
    QFileWrap file;

    const char filename[] = ":/data/tiles.obl";
    std::unique_ptr<CFrameSet> frameset = std::make_unique<CFrameSet>();
    if (!file.open(filename, "rb")) {
        LOGE("can't open %s", filename);
    }
    LOGI("reading %s", filename);
    if (frameset->extract(file)) {
        LOGI("extracted: %lu", (frameset)->getSize());
    } else {
        LOGE("failed to extract frames");
    }
    file.close();

    m_tileFrames = frameset->frames();
    frameset->removeAll();

    // Force resize in case zoom > 1 and map already exists
    if (map()) {
        int tileSize = 16 * zoom();
        resize(
            map()->len() * tileSize,
            map()->hei() * tileSize
        );
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
    const int tileSize = 16 * m_zoom;
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
    const int s = 16 * m_zoom;
    return {tile.x() * s, tile.y() * s};
}

QPixmap MapWidget::getCachedPixmap(uint8_t tileId)
{
    if (tileId >= m_tileFrames.size() || !m_tileFrames[tileId])
        return QPixmap();

    uint cacheKey = tileId + m_zoom * 1000;
    QPixmap *cached = m_pixmapCache.object(cacheKey);
    if (cached)
        return *cached;

    CFrame *frame = m_tileFrames[tileId];
   // if (frame->isEmpty())
     //   return QPixmap();

    const QImage img(reinterpret_cast<uint8_t*>(frame->getRGB().data()),
                     frame->width(), frame->height(),
                     QImage::Format_RGBX8888); // or RGBX8888

    QPixmap pixmap = QPixmap::fromImage(img).scaled(
        16 * m_zoom, 16 * m_zoom, Qt::IgnoreAspectRatio, Qt::FastTransformation);

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
        p.drawText(rect(), Qt::AlignCenter, "No map loaded");
        return;
    }

    QPainter painter(this);
    painter.fillRect(rect(), Qt::darkGray);

    drawMap(painter);

    if (m_showGrid)
        drawGrid(painter);

    if (m_tool == Tool::Selection && m_selection.isValid())
        drawSelectionRect(painter);

    else if (m_shadowTilePos.x() >= 0 && (m_tool == Tool::Stamp))
        drawShadowTile(painter);
    else if (m_shadowTilePos.x() >= 0 && (m_tool == Tool::Eraser))
        drawShadowTile(painter);

}

void MapWidget::drawMap(QPainter &painter)
{
    auto drawRect = [&painter](const auto &color, const auto &tileRect) {
        painter.setPen(QPen(color, 2, Qt::SolidLine));
        painter.setBrush(Qt::NoBrush);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.drawRect(tileRect);
    };

    const int tileSize = 16 * m_zoom;
    QRect visible = rect().adjusted(-tileSize, -tileSize, tileSize, tileSize);
    QPoint topLeftTile = screenToTile(visible.topLeft());
    QPoint bottomRightTile = screenToTile(visible.bottomRight());
    const auto & states = m_map->statesConst();
    const uint16_t startPos = states.getU(POS_ORIGIN);
    const uint16_t exitPos = states.getU(POS_EXIT);

    QFont font = painter.font();
    font.setPixelSize(12 * m_zoom);   // ← change this number
    painter.setFont(font);

    for (int y = topLeftTile.y(); y <= bottomRightTile.y() && y < m_map->hei(); ++y)
    {
        if (y < 0)
            continue;
        for (int x = topLeftTile.x(); x <= bottomRightTile.x() && x < m_map->len(); ++x)
        {
            if (x < 0)
                continue;
            uint8_t tileId = m_map->at(x, y);
            QPixmap pm = getCachedPixmap(tileId);
            if (!pm.isNull())
            {
                painter.drawPixmap(x * tileSize, y * tileSize, pm);
            }

            const QRect tileRect(x * tileSize, y * tileSize, tileSize, tileSize);
            uint8_t attr = m_map->getAttr(x,y);
            if (attr) {
                QString text = QString("%1").arg(attr, 2, 16, QChar('0')).toUpper();

                // Black shadow/outline
                painter.setPen(Qt::black);
                painter.drawText(tileRect, Qt::AlignCenter, text);

                // Bright color on top
                painter.setPen(attr2color(attr));
                painter.drawText(tileRect.adjusted(-2,-2,-2,-2), Qt::AlignCenter, text);
            }
            if (m_flashState && m_attr && attr == m_attr) {
                //painter.setPen(QPen(rgbaToQColor(PINK), 3, Qt::SolidLine));
                //painter.setBrush(Qt::NoBrush);
                //painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

                // Optional: make it pulse in size
                int offset = (m_flashState && (QTime::currentTime().msec() / 100) % 2) ? 2 : 0;
                painter.drawRect(tileRect.adjusted(-offset, -offset, offset, offset));
                drawRect(rgbaToQColor(PINK), tileRect.adjusted(-offset, -offset, offset, offset));

            }
             if (m_hx != 0 && m_hy != 0 &&  x == m_hx &&  y == m_hy) {
                drawRect(rgbaToQColor(CORAL), tileRect);
                /* painter.setPen(QPen(rgbaToQColor(CORAL), 3, Qt::SolidLine));
                painter.setBrush(Qt::NoBrush);
                painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
                painter.drawRect(tileRect);*/
             }
             if (startPos && startPos == CMap::toKey(x, y)) {
                 drawRect(rgbaToQColor(YELLOW), tileRect);
                 /*
                 painter.setPen(QPen(rgbaToQColor(YELLOW), 2, Qt::SolidLine));
                 painter.setBrush(Qt::NoBrush);
                 painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
                 painter.drawRect(tileRect);
                */
               //  drawRect(bitmap, Rect{.x=x*TILE_SIZE, .y=y*TILE_SIZE, .width=TILE_SIZE, .height=TILE_SIZE}, YELLOW, false);
             }
             if (exitPos && exitPos == CMap::toKey(x, y)) {
                 drawRect(rgbaToQColor(RED), tileRect);
                 /*painter.setPen(QPen(rgbaToQColor(RED), 2, Qt::SolidLine));
                 painter.setBrush(Qt::NoBrush);
                 painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
                 painter.drawRect(tileRect);*/
                 //drawRect(bitmap, Rect{.x=x*TILE_SIZE, .y=y*TILE_SIZE, .width=TILE_SIZE, .height=TILE_SIZE}, RED, false);
             }
        }
    }
}

void MapWidget::drawGrid(QPainter &painter)
{
    const int tileSize = 16 * m_zoom;
    painter.setPen(QColor(255, 255, 0, 80));

    for (int x = 0; x <= m_map->len(); ++x)
    {
        int sx = x * tileSize;
        painter.drawLine(sx, 0, sx, height());
    }
    for (int y = 0; y <= m_map->hei(); ++y)
    {
        int sy = y * tileSize;
        painter.drawLine(0, sy, width(), sy);
    }
}

void MapWidget::drawShadowTile(QPainter &painter)
{
    if (m_shadowTilePos.x() < 0 || m_shadowTilePos.x() >= m_map->len() ||
        m_shadowTilePos.y() < 0 || m_shadowTilePos.y() >= m_map->hei())
        return;

    const int tileSize = 16 * m_zoom;
    painter.setOpacity(0.6);

    if (m_tool == Tool::Eraser) {
        int tx = m_shadowTilePos.x();
        int ty = m_shadowTilePos.y();
        uint8_t tileId = 0;
        QPixmap pm = getCachedPixmap(tileId);
        if (!pm.isNull())
        {
            painter.drawPixmap(tx * tileSize, ty * tileSize, pm);
        }
        return;
    }


    for (int dy = 0; dy < m_stampRows; ++dy)
    {
        for (int dx = 0; dx < m_stampCols; ++dx)
        {
            int tx = m_shadowTilePos.x() + dx;
            int ty = m_shadowTilePos.y() + dy;
            if (tx >= m_map->len() || ty >= m_map->hei())
                continue;

            size_t idx = dy * m_stampCols + dx;
            if (idx >= m_currentStamp.size())
                break;

            uint8_t tileId = m_currentStamp[idx];
            QPixmap pm = getCachedPixmap(tileId);
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
    const int tileSize = 16 * m_zoom;
    QRect r(
        m_selection.x() * tileSize,
        m_selection.y() * tileSize,
        m_selection.width() * tileSize,
        m_selection.height() * tileSize);
    painter.setPen(QPen(Qt::yellow, 2));
    painter.setBrush(QColor(255, 255, 0, 30));
    painter.drawRect(r);
}

void MapWidget::commitStampAt(const QPoint &tilePos)
{
    if (!m_map || m_currentStamp.empty())
        return;

    bool changed = false;

    if (m_tool == Tool::Eraser)
    {
        int tx = tilePos.x();
        int ty = tilePos.y();
        if (!m_map->isValid(tx, ty))
            return;
        uint8_t newTile = 0;
        if (m_map->at(tx, ty) != newTile)
        {
            m_map->set(tx, ty, newTile);
            changed = true;
        }
    }
    else {
        for (int dy = 0; dy < m_stampRows; ++dy)
        {
            for (int dx = 0; dx < m_stampCols; ++dx)
            {
                int tx = tilePos.x() + dx;
                int ty = tilePos.y() + dy;
                if (!m_map->isValid(tx, ty))
                    continue;

                size_t idx = dy * m_stampCols + dx;
                if (idx >= m_currentStamp.size())
                    break;

                uint8_t newTile = m_currentStamp[idx];
                if (m_map->at(tx, ty) != newTile)
                {
                    m_map->set(tx, ty, newTile);
                    changed = true;
                }
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

    QPoint tile = screenToTile(event->pos());

    if (event->button() == Qt::LeftButton)
    {
        m_leftPressed = true;

        if (m_tool == Tool::Eraser)
        {
            commitStampAt(tile);
        }
        else if (m_tool == Tool::Stamp)
        {
            commitStampAt(tile);
        }
        else if (m_tool == Tool::Picker)
        {
            if (m_map->isValid(tile.x(), tile.y()))
            {
                emit tilePicked(m_map->at(tile.x(), tile.y()));
            }
        }
        else if (m_tool == Tool::Selection)
        {
            m_selectionStart = tile;
            m_selection = QRect(tile, QSize(1, 1));
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

    QPoint tile = screenToTile(event->pos());
    m_shadowTilePos = tile;

    if (m_leftPressed && m_tool == Tool::Stamp)
    {
        commitStampAt(tile);
    }
    else if (m_selecting && m_tool == Tool::Selection)
    {
        QRect newSel(m_selectionStart, tile);
        newSel = newSel.normalized();
        if (newSel != m_selection)
        {
            m_selection = newSel;
            emit selectionChanged(m_selection);
            update();
        }
    }
    else
    {
        update(); // for shadow tile
    }
}

void MapWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
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

// In MapWidget.cpp — add this method
void MapWidget::updateCursor()
{
    switch (m_tool) {
    case Tool::Stamp:
        //setCursor(Qt::CrossCursor);
        setCursor(QCursor(QPixmap(":/data/cursors/sketchpntbrush.png"), 9, 31));
        break;
    case Tool::Picker:
        setCursor(QCursor(QPixmap(":/data/cursors/eyedropper.png"), 2, 14));
        //setCursor(QCursor(Qt::PointingHandCursor));
        break;
    case Tool::Selection:
        setCursor(Qt::CrossCursor);
        break;
    case Tool::Eraser:
        setCursor(QCursor(QPixmap(":/data/cursors/efface.png"), 10, 27));
        break;
    default:
        setCursor(Qt::ArrowCursor);
        break;
    }
}


void MapWidget::createContextMenu()
{
    if (m_contextMenu) return;

    m_contextMenu = new QMenu(this);

    // mapEdit actions
    QAction *actionSetAttr =new QAction(tr("Set raw attribute"), m_mainWindow);
    actionSetAttr->setStatusTip(tr("Set the raw attribute for this tile"));
    m_contextMenu->addAction(actionSetAttr);
    connect(actionSetAttr, &QAction::triggered, this, [this]() {
        /*
        if (m_mainWindow) {
            // call private slot via QMetaObject (100 % safe and used everywhere)
            QMetaObject::invokeMethod(m_mainWindow,
                                      "showAttrDialog",
                                      Qt::QueuedConnection,
                                      Q_ARG(int, m_lastRightClickTile.x()),
                                      Q_ARG(int, m_lastRightClickTile.y()));
        }*/
        const int x = m_lastRightClickTile.x();
        const int y = m_lastRightClickTile.y();
        const uint8_t originalAttr = m_map->getAttr(x,y );
        CDlgAttr dlg(this);
        dlg.attr(originalAttr);
        if (dlg.exec() == QDialog::Accepted)
        {
            const uint8_t newAttr = dlg.attr();
            if (originalAttr != newAttr) {
                m_map->setAttr(x, y, newAttr);
                emit mapModified();
            }
        }
        update();
    });

    QAction *actionHighlight =  new QAction(tr("highlight attribute"), this);
    actionHighlight->setStatusTip(tr("hightlight this attribute"));
    m_contextMenu->addAction(actionHighlight);
    connect(actionHighlight, &QAction::triggered, this, [this]() {
        if (m_map) {
            m_attr =  m_map->getAttr(m_lastRightClickTile.x(), m_lastRightClickTile.y());
            qDebug("m_attr : %.2x", m_attr);
        }
        update();
    });

    QAction *actionHighlightXY =  new QAction(tr("highlight this position"), this);
    actionHighlightXY->setStatusTip(tr("hightlight this position"));
    m_contextMenu->addAction(actionHighlightXY);
    connect(actionHighlightXY, &QAction::triggered, this, [this]() {
        if (m_map) {
            m_hx = m_lastRightClickTile.x();
            m_hy = m_lastRightClickTile.y();
            qDebug("m_hx : %.2x  -- m_hy : %.2x", m_hx, m_hy);
        }
        update();
    });


    QAction *actionStatTile = new QAction(tr("see tile stats"), this);
    m_contextMenu->addAction(actionStatTile);
    connect(actionStatTile, &QAction::triggered, this, [this]() {
        const int x = m_lastRightClickTile.x();
        const int y = m_lastRightClickTile.y();
        CDlgStat dlg(m_map->at(x, y), m_map->getAttr(x, y), this);
        dlg.setWindowTitle(tr("Tile Statistics"));
        dlg.exec();
        /*
        if (m_mainWindow) {
            // call private slot via QMetaObject (100 % safe and used everywhere)
            QMetaObject::invokeMethod(m_mainWindow,
                                      "showStatDialog",
                                      Qt::QueuedConnection,
                                      Q_ARG(int, m_lastRightClickTile.x()),
                                      Q_ARG(int, m_lastRightClickTile.y()));
        }*/
    });
    actionStatTile->setStatusTip(tr("Show the data information on this tile"));
    m_contextMenu->addSeparator();

    QAction *actionSetStartPos = new QAction(tr("set start pos"), this);
    //connect(actionSetStartPos, SIGNAL(triggered()),
    //        this, SLOT(on_setStartPos()));
    connect(actionSetStartPos, &QAction::triggered, this, [this]() {
           m_map->states().setU(POS_ORIGIN, CMap::toKey(m_lastRightClickTile.x(), m_lastRightClickTile.y()));
           emit mapModified();
           update();
    });
    actionSetStartPos->setStatusTip(tr("Set the start position for this map"));
    m_contextMenu->addAction(actionSetStartPos);

    QAction *actionSetExitPos = new QAction(tr("set exit pos"), this);
    //connect(actionSetExitPos, SIGNAL(triggered()),
    //        this, SLOT(on_setExitPos()));
    connect(actionSetExitPos, &QAction::triggered, this, [this]() {
        m_map->states().setU(POS_EXIT, CMap::toKey(m_lastRightClickTile.x(), m_lastRightClickTile.y()));
        emit mapModified();
        update();
    });
    actionSetExitPos->setStatusTip(tr("Set the exit position for this map."));
    m_contextMenu->addAction(actionSetExitPos);
    m_contextMenu->addSeparator();

    QMenu *selectMenu = m_contextMenu->addMenu("selection");

    bool isSelectionValid = !m_selection.isEmpty(); //.isValid();

    // ─── Common actions ─────────────────────────────────────
    QAction *copy    = selectMenu->addAction(QIcon::fromTheme("edit-copy"),  tr("&Copy"));
    QAction *cut     = selectMenu->addAction(QIcon::fromTheme("edit-cut"),   tr("Cu&t"));
    QAction *paste   = selectMenu->addAction(QIcon::fromTheme("edit-paste"), tr("&Paste"));
    QAction *del     = selectMenu->addAction(QIcon::fromTheme("edit-delete"), tr("Delete"));

    copy->setEnabled(isSelectionValid);
    cut->setEnabled(isSelectionValid);
    paste->setEnabled(isSelectionValid);
    del->setEnabled(isSelectionValid);

   // m_contextMenu->addSeparator();

    QAction *selectAll = selectMenu->addAction(tr("Select &All"), QKeySequence::SelectAll);
    QAction *clearSel   = selectMenu->addAction(tr("Clear Selection"));
    clearSel->setEnabled(isSelectionValid);

    m_contextMenu->addSeparator();

//    QAction *props   = m_contextMenu->addAction(tr("Tile &Properties…"));

    // ─── Connect actions ─────────────────────────────────────
    connect(copy,    &QAction::triggered, this, [this]() {
        if (currentSelection().isValid())
            emit copyRequested(currentSelection());
    });

    connect(cut,     &QAction::triggered, this, [this]() {
        if (currentSelection().isValid()) {
            emit copyRequested(currentSelection());
            fillSelection(0);               // or your erase tile
            clearSelection();
            emit mapModified();
        }
    });

    connect(paste,   &QAction::triggered, this, [this]() {
        emit pasteRequested(m_lastRightClickTile);
    });

    connect(del,     &QAction::triggered, this, [this]() {
        if (currentSelection().isValid()) {
            fillSelection(0);
            clearSelection();
            emit mapModified();
        }
    });

    connect(selectAll, &QAction::triggered, this, [this]() {
        if (m_map) {
            QRect all(0, 0, m_map->len(), m_map->hei());
            m_selection = all;
            emit selectionChanged(all);
            update();
        }
    });

    connect(clearSel, &QAction::triggered, this, &MapWidget::clearSelection);

    //connect(props, &QAction::triggered, this, [this]() {
    //    emit tilePropertiesRequested(m_lastRightClickTile.x(), m_lastRightClickTile.y());
    //});

    // Disable paste if clipboard is empty (optional polish)
    connect(qApp->clipboard(), &QClipboard::dataChanged, this, [this, paste]() {
        paste->setEnabled(qApp->clipboard()->mimeData()->hasImage() ||
                          qApp->clipboard()->mimeData()->hasFormat("application/x-lgck-tiledata"));
    });

    /*
        QMenu menu(this);
        QAction *actionSetAttr = new QAction(tr("set raw attribute"), this);
        connect(actionSetAttr, SIGNAL(triggered()),
                this, SLOT(showAttrDialog()));
        actionSetAttr->setStatusTip(tr("Set the raw attribute for this tile"));
        menu.addAction(actionSetAttr);

        QAction *actionHighlight =  new QAction(tr("highlight attribute"), this);
        connect(actionHighlight, SIGNAL(triggered()),
                this, SLOT(on_highlight()));
        menu.addAction(actionHighlight);
        actionHighlight->setStatusTip(tr("hightlight this attribute"));

        QAction *actionHighlightXY =  new QAction(tr("highlight this position"), this);
        connect(actionHighlightXY, SIGNAL(triggered()),
                this, SLOT(on_highlightXY()));
        menu.addAction(actionHighlightXY);
        actionHighlightXY->setStatusTip(tr("hightlight this position"));

        QAction *actionStatAttr = new QAction(tr("see tile stats"), this);
        connect(actionStatAttr, SIGNAL(triggered()),
                this, SLOT(showStatDialog()));
        menu.addAction(actionStatAttr);
        actionStatAttr->setStatusTip(tr("Show the data information on this tile"));
        menu.addSeparator();

        QAction *actionSetStartPos = new QAction(tr("set start pos"), this);
        connect(actionSetStartPos, SIGNAL(triggered()),
                this, SLOT(on_setStartPos()));
        actionSetStartPos->setStatusTip(tr("Set the start position for this map"));
        menu.addAction(actionSetStartPos);

        QAction *actionSetExitPos = new QAction(tr("set exit pos"), this);
        connect(actionSetExitPos, SIGNAL(triggered()),
                this, SLOT(on_setExitPos()));
        actionSetExitPos->setStatusTip(tr("Set the exit position for this map."));
        menu.addAction(actionSetExitPos);
        menu.addSeparator();

        QAction *actionDeleteTile = new QAction(tr("delete tile"), this);
        connect(actionDeleteTile, SIGNAL(triggered()),
                this, SLOT(on_deleteTile()));
        menu.addAction(actionDeleteTile);
        actionDeleteTile->setStatusTip(tr("delete this tile"));
    */

}

void MapWidget::contextMenuEvent(QContextMenuEvent *event)
{
    if (!m_map) return;

    createContextMenu();

    // Convert click position → tile coordinates
    QPoint tilePos = screenToTile(event->pos());
    if (!m_map->isValid(tilePos.x(), tilePos.y())) return;

    m_lastRightClickTile = tilePos;

    // Update action states
    //bool hasSel = currentSelection().isValid();
    // (you can get these actions via m_contextMenu->actions())
  //  m_contextMenu->actions().at(0)->setEnabled(hasSel);  // Copy
  //  m_contextMenu->actions().at(1)->setEnabled(hasSel);  // Cut
   // m_contextMenu->actions().at(3)->setEnabled(hasSel);  // Delete

    m_contextMenu->exec(event->globalPos());
}



void MapWidget::fillSelection(uint8_t tileId /* = UINT8_MAX */)
{
    if (!m_map) return;

    // If no explicit tileId given → use the current brush
    if (tileId == UINT8_MAX) {
        // For multi-tile stamps we just use the top-left tile (most common)
        if (!m_currentStamp.empty())
            tileId = m_currentStamp[0];
        else
            tileId = m_currentTile;
    }

    QRect area = m_selection;
    if (!area.isValid()) {
        // nothing selected → fill entire map
        area = QRect(0, 0, m_map->len(), m_map->hei());
    }

    bool changed = false;

    // Fastest possible loop — no function calls inside
    for (int y = area.top(); y <= area.bottom(); ++y) {
        for (int x = area.left(); x <= area.right(); ++x) {
            if (m_map->at(x, y) != tileId) {
                m_map->set(x, y, tileId);
                changed = true;
            }
        }
    }

    if (changed) {
        update(area.adjusted(-1,-1,1,1).intersected(rect())); // repaint only the affected region + 1px border
        emit mapModified();
      //  setDirty(true);                     // if you forward this to MainWindow
    }
}


QColor MapWidget::rgbaToQColor(const uint32_t color)
{
    return QColor(color & 0xff, (color & 0xff00) >> 8, (color & 0xff0000) >> 16);
}

QColor MapWidget::attr2color(const uint8_t attr)
{
    auto getColor = [](auto attr) {
        if (RANGE(attr, ATTR_MSG_MIN, ATTR_MSG_MAX)) {
            return CYAN;
        } else if (RANGE(attr, ATTR_IDLE_MIN, ATTR_IDLE_MAX)) {
            return HOTPINK;
        } else if (attr == ATTR_FREEZE_TRAP) {
            return LIGHTGRAY;
        } else if (attr == ATTR_TRAP) {
            return RED;
        } else if (RANGE(attr, ATTR_CRUSHER_MIN, ATTR_CRUSHER_MAX)) {
            return ORANGE;
        } else if (RANGE(attr, ATTR_BOSS_MIN, ATTR_BOSS_MAX)) {
            return SEAGREEN;
        } else if (attr > PASSAGE_ATTR_MAX) {
            return OLIVE; // undefined behavior
        } else if (RANGE(attr, SECRET_ATTR_MIN, SECRET_ATTR_MAX)) {
            return GREEN;
        } else if (RANGE(attr, PASSAGE_REG_MIN, PASSAGE_REG_MAX)) {
            return YELLOW;
        } else {
            return WHITE;
        }
    };
    const uint32_t color = getColor(attr);
    return rgbaToQColor(color);//  QColor(color & 0xff, (color & 0xff00) >> 8, (color & 0xff0000) >> 16);
}

