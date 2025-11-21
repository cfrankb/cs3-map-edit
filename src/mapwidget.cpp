#include "mapwidget.h"
#include <QPainter>
#include <QScrollBar>
#include "runtime/shared/qtgui/qfilewrap.h"
#include "runtime/shared/FrameSet.h"
#include "runtime/shared/Frame.h"
#include "runtime/map.h"
#include "runtime/animator.h"
#include "runtime/states.h"
#include "runtime/statedata.h"
#include "runtime/attr.h"
#include "runtime/color.h"
#include "runtime/shared/logger.h"
#include "mapscroll.h"

/*

// Add this once in your project
constexpr QImage::Format Native32BitFormat =
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    QImage::Format_ARGB32;           // big-endian needs ARGB
#else
    QImage::Format_ARGB32;           // little-endian: our BGRA-in-memory layout
#endif
// QImage::Format_ARGB32_Premultiplied   // works even if you ever add transparency


*/

constexpr pixel_t GRIDCOLOR    = RGB(0x79, 0xa0, 0xbf);
#define RANGE(_x, _min, _max) (_x >= _min && _x <= _max)

CMapWidget::CMapWidget(QWidget *parent)
    : QWidget{parent}
{
    m_animator = std::make_unique<CAnimator>();
    m_timer.setInterval(1000 / TICK_RATE);
    m_timer.start();
    preloadAssets();
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(update()));
}

CMapWidget::~CMapWidget()
{
    m_timer.stop();
}

void CMapWidget::setMap(CMap *pMap)
{
    m_map = pMap;
}

void CMapWidget::showGrid(bool show)
{
    m_showGrid = show;
}

void CMapWidget::setAnimate(bool val)
{
    m_animate = val;
}

void CMapWidget::preloadAssets()
{
    QFileWrap file;
    typedef struct {
        const char *filename;
        std::unique_ptr<CFrameSet> *frameset;
    } asset_t;

    asset_t assets[] = {
        {":/data/tiles.obl", &m_tiles},
        {":/data/animz.obl", &m_animz},
    };

    constexpr size_t assetCount = sizeof(assets)/sizeof(assets[0]);
    for (size_t i=0; i < assetCount; ++i) {
        asset_t & asset = assets[i];
        *(asset.frameset) = std::make_unique<CFrameSet>();
        if (!file.open(asset.filename, "rb")) {
            LOGE("can't open %s", asset.filename);
            continue;
        }
        LOGI("reading %s", asset.filename);
        if ((*(asset.frameset))->extract(file)) {
            LOGI("extracted: %lu", (*(asset.frameset))->getSize());
        } else {
            LOGE("failed to extract frames");
        }
        file.close();
    }

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
}

void CMapWidget::paintEvent(QPaintEvent *)
{
    const QSize widgetSize = size();
    const int width = widgetSize.width() / SCALE_FACTOR + TILE_SIZE;
    const int height = widgetSize.height() / SCALE_FACTOR + TILE_SIZE;

    if (!m_map) {
        LOGE("map is null");
        return;
    }

    // animate tiles
    ++m_ticks;
    if (m_animate && m_ticks % ANIMATION_RATE == 0) {
        m_animator->animate();
    }

    // draw screen
    if (!m_frame) {
        m_frame = std::make_unique<CFrame>(width, height);
    } else if (m_frame->width() != width || m_frame->height()!= height) {
        m_frame->resize(width, height);
    }
    CFrame & bitmap = *m_frame;
    drawScreen(bitmap);

    // show the screen
    const QImage img(reinterpret_cast<uint8_t*>(bitmap.getRGB().data()),
               bitmap.width(), bitmap.height(),
               QImage::Format_RGBX8888); // or RGBX8888
    QPainter p(this);
    p.drawImage(0, 0, img.scaled(QSize(width * SCALE_FACTOR, height * SCALE_FACTOR), Qt::IgnoreAspectRatio, Qt::FastTransformation));
    p.end();
}

void CMapWidget::drawRect(CFrame &frame, const Rect &rect, const uint32_t color, bool fill)
{
    uint32_t *rgba = frame.getRGB().data();
    const int rowPixels = frame.width();
    if (fill)
    {
        for (int y = 0; y < rect.height; y++)
        {
            for (int x = 0; x < rect.width; x++)
            {
                rgba[(rect.y + y) * rowPixels + rect.x + x] = color;
            }
        }
    }
    else
    {
        for (int y = 0; y < rect.height; y++)
        {
            for (int x = 0; x < rect.width; x++)
            {
                if (y == 0 || y == rect.height - 1 || x == 0 || x == rect.width - 1)
                {
                    rgba[(rect.y + y) * rowPixels + rect.x + x] = color;
                }
            }
        }
    }
}

void CMapWidget::drawScreen(CFrame &bitmap)
{
    const CMap *map = m_map;
    const auto & states = map->statesConst();
    const uint16_t startPos = states.getU(POS_ORIGIN);
    const uint16_t exitPos = states.getU(POS_EXIT);

    const int maxRows = bitmap.height() / TILE_SIZE;
    const int maxCols = bitmap.width() / TILE_SIZE;
    const int rows = std::min(maxRows, map->hei());
    const int cols = std::min(maxCols, map->len());
    CMapScroll *mapScroll = qobject_cast<CMapScroll*>(parent());
    if (!mapScroll) return;
    const int mx = mapScroll->horizontalScrollBar()->value();
    const int my = mapScroll->verticalScrollBar()->value();
    constexpr const char *hexchar = "0123456789ABCDEF";

    CFrameSet & tiles = *m_tiles;
    CFrameSet & animz = *m_animz;
    bitmap.fill(WHITE);
    for (int y=0; y < rows; ++y) {
        if (y + my >= map->hei())
        {
            break;
        }
        for (int x=0; x < cols; ++x) {
            if (x + mx >= map->len())
            {
                break;
            }
            uint8_t tileID = map->at(x + mx, y + my);
            CFrame *tile;
            int j = m_animate ? m_animator->at(tileID) : static_cast<uint8_t>(NO_ANIMZ);
            if (j == NO_ANIMZ) {
                tile = tiles[tileID];
            } else {
                tile = animz[j];
            }
            drawTile(bitmap, x * TILE_SIZE, y * TILE_SIZE, *tile, false);



            if (startPos && startPos == CMap::toKey(mx+x, my+y)) {
                drawRect(bitmap, Rect{.x=x*TILE_SIZE, .y=y*TILE_SIZE, .width=TILE_SIZE, .height=TILE_SIZE}, YELLOW, false);
            }
            if (exitPos && exitPos == CMap::toKey(mx+x, my+y)) {
                drawRect(bitmap, Rect{.x=x*TILE_SIZE, .y=y*TILE_SIZE, .width=TILE_SIZE, .height=TILE_SIZE}, RED, false);
            }

            uint8_t a = map->getAttr(mx+x, my+y);
            if (a) {
                char s[3];
                s[0] = hexchar[a >> 4];
                s[1] = hexchar[a & 0xf];
                s[2] = 0;
                drawFont(bitmap, x*TILE_SIZE, y*TILE_SIZE + 4, s, attr2color(a), true);
            }
            if (a != 0 && a == m_attr) {
                bool alt = ((m_ticks / BLINK_RATE) & 1) == 1;
                drawRect(bitmap, Rect{.x=x*TILE_SIZE, .y=y*TILE_SIZE, .width=TILE_SIZE, .height=TILE_SIZE}, alt ? PINK : CYAN, false);
            } else if (m_hx != 0 && m_hy != 0 && mx + x == m_hx && my + y == m_hy) {
                drawRect(bitmap, Rect{.x=x*TILE_SIZE, .y=y*TILE_SIZE, .width=TILE_SIZE, .height=TILE_SIZE}, CORAL, false);
            }

            if (m_showGrid) {
                const int tx = x * TILE_SIZE;
                const int ty = y * TILE_SIZE;
                bitmap.dotted_hline(tx, ty, TILE_SIZE, GRIDCOLOR);  // top
                bitmap.dotted_vline(tx, ty, TILE_SIZE, GRIDCOLOR);  // left
            }
        }
    }
}

uint32_t CMapWidget::attr2color(const uint8_t attr)
{
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
}


void CMapWidget::drawFont(CFrame & frame, int x, int y, const char *text, const uint32_t color, const bool alpha)
{
    uint32_t *rgba = frame.getRGB().data();
    const int rowPixels = frame.width();
    const int fontSize = static_cast<int>(FONT_SIZE);
    const int fontOffset = fontSize;
    const int textSize = strlen(text);
    for (int i=0; i < textSize; ++i) {
        const uint8_t c = static_cast<uint8_t>(text[i]) - ' ';
        uint8_t *font = m_fontData.data() + c * fontOffset;

        // shadow logic
        if (alpha) {
            for (int yy = 0; yy < fontSize; ++yy) {
                uint8_t bitFilter = 1;
                for (int xx = 0; xx < fontSize; ++xx) {
                    bool pixelOn = (font[yy] & bitFilter) != 0;

                    // Drop shadow: if current pixel is off, but either left or top neighbor was on → draw black shadow
                    bool shadow = !pixelOn && (
                                      (xx > 0 && (font[yy] & (bitFilter << 1))) ||      // left neighbor
                                      (yy > 0 && (font[yy-1] & bitFilter))              // top neighbor
                                      );

                    rgba[(yy + y) * rowPixels + xx + x] = pixelOn ? color : (shadow ? BLACK : CLEAR); // 0 = transparent/no change
                    bitFilter <<= 1;
                }
            }
        } else {
            for (int yy=0; yy < fontSize; ++yy) {
                uint8_t bitFilter = 1;
                for (int xx=0; xx < fontSize; ++xx) {
                    rgba[ (yy + y) * rowPixels + xx + x] = font[yy] & bitFilter ? color : BLACK;
                    bitFilter = bitFilter << 1;
                }
            }
        }
        x+= fontSize;
    }
}

void CMapWidget::drawTile(CFrame & bitmap, const int x, const int y, CFrame & tile, const bool alpha)
{
    const int WIDTH = bitmap.width();
    const uint32_t *tileData = tile.getRGB().data();
    uint32_t *dest = bitmap.getRGB().data() + x + y * WIDTH;
    if (alpha) {
        for (int row=0; row < tile.height(); ++row) {
            for (int col=0; col < tile.width(); ++col) {
                const uint32_t & rgba = tileData[col];
                if (rgba & ALPHA) {
                    dest[col] = rgba;
                }
            }
            dest += WIDTH;
            tileData += tile.width();
        }
    } else {
        for (int row=0; row < tile.height(); ++row) {
            for (int col=0; col < tile.width(); ++col) {
                dest[col] = tileData[col];
            }
            dest += WIDTH;
            tileData += tile.width();
        }
    }
}

void CMapWidget::highlight(uint8_t attr)
{
    m_attr = attr;
}

void CMapWidget::highlight(uint8_t x, uint8_t y)
{
    m_hx = x;
    m_hy = y;
}
