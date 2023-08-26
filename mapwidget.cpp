#include "mapwidget.h"
#include "qpainter.h"
#include "shared/qtgui/qfilewrap.h"
#include "FrameSet.h"
#include "Frame.h"
#include "map.h"
#include "mapscroll.h"
#include "animator.h"
#include <QScrollBar>

CMapWidget::CMapWidget(QWidget *parent)
    : QWidget{parent}
{
    m_animator = new CAnimator();
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
        CFrameSet **frameset;
    } asset_t;

    asset_t assets[] = {
        {":/data/tiles.obl", &m_tiles},
        {":/data/animz.obl", &m_animz},
    };

    for (int i=0; i < 2; ++i) {
        asset_t & asset = assets[i];
        *(asset.frameset) = new CFrameSet();
        if (file.open(asset.filename, "rb")) {
            qDebug("reading %s", asset.filename);
            if ((*(asset.frameset))->extract(file)) {
                qDebug("exracted: %d", (*(asset.frameset))->getSize());
            }
            file.close();
        }
    }

    const char fontName [] = ":/data/font.bin";
    int size = 0;
    if (file.open(fontName, "rb")) {
        size = file.getSize();
        m_fontData = new uint8_t[size];
        file.read(m_fontData, size);
        file.close();
        qDebug("size: %d", size);
    } else {
        qDebug("failed to open %s", fontName);
    }
}

void CMapWidget::paintEvent(QPaintEvent *)
{
    const QSize widgetSize = size();
    const int width = widgetSize.width() / 2 + TILE_SIZE;
    const int height = widgetSize.height() / 2 + TILE_SIZE;

    if (!m_map) {
        qDebug("map is null");
        return;
    }

    // animate tiles
    ++m_ticks;
    if (m_animate && m_ticks % 3 == 0) {
        m_animator->animate();
    }

    // draw screen
    CFrame bitmap(width, height);
    drawScreen(bitmap);
    if (m_showGrid) {
        drawGrid(bitmap);
    }

    // show the screen
    const QImage & img = QImage(reinterpret_cast<uint8_t*>(bitmap.getRGB()), bitmap.len(), bitmap.hei(), QImage::Format_RGBX8888);
    const QPixmap & pixmap = QPixmap::fromImage(img.scaled(QSize(width * 2, height * 2)));
    QPainter p(this);
    p.drawPixmap(0, 0, pixmap);
    p.end();
}

void CMapWidget::drawScreen(CFrame &bitmap)
{
    CMap *map = m_map;
    const int maxRows = bitmap.hei() / TILE_SIZE;
    const int maxCols = bitmap.len() / TILE_SIZE;
    const int rows = std::min(maxRows, map->hei());
    const int cols = std::min(maxCols, map->len());
    CMapScroll *scr = static_cast<CMapScroll*>(parent());
    const int mx = scr->horizontalScrollBar()->value();
    const int my = scr->verticalScrollBar()->value();
    const char hexchar[] = "0123456789ABCDEF";

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
            uint8_t a = map->getAttr(mx+x, my+y);
            if (a) {
                char s[3];
                s[0] = hexchar[a >> 4];
                s[1] = hexchar[a & 0xf];
                s[2] = 0;
                drawFont(bitmap, x*TILE_SIZE, y*TILE_SIZE + 4, s, YELLOW, true);
            }
        }
    }
}

void CMapWidget::drawGrid(CFrame & bitmap)
{
    CMap *map = m_map;
    int maxRows = bitmap.hei() / TILE_SIZE;
    int maxCols = bitmap.len() / TILE_SIZE;
    const int rows = std::min(maxRows, map->hei());
    const int cols = std::min(maxCols, map->len());
    CMapScroll *scr = static_cast<CMapScroll*>(parent());
    const int mx = scr->horizontalScrollBar()->value();
    const int my = scr->verticalScrollBar()->value();

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
            for (unsigned int yy=0; yy< TILE_SIZE; yy += 2) {
                for (unsigned int xx=0; xx< TILE_SIZE; xx += 2) {
                    if (xx == 0 || yy == 0) {
                        bitmap.at(x * TILE_SIZE + xx, y * TILE_SIZE + yy) = GRIDCOLOR;
                        if (yy !=0) {
                            break;
                        }
                    }
                }
            }
        }
    }
}

void CMapWidget::drawFont(CFrame & frame, int x, int y, const char *text, const uint32_t color, const bool alpha)
{
    uint32_t *rgba = frame.getRGB();
    const int rowPixels = frame.len();
    const int fontSize = static_cast<int>(FONT_SIZE);
    const int fontOffset = fontSize * fontSize;
    const int textSize = strlen(text);
    for (int i=0; i < textSize; ++i) {
        const uint8_t c = static_cast<uint8_t>(text[i]) - ' ';
        uint8_t *font = m_fontData + c * fontOffset;
        if (alpha) {
            for (int yy=0; yy < fontSize; ++yy) {
                for (int xx=0; xx < fontSize; ++xx) {
                    uint8_t lb = 0;
                    if (xx > 0) lb = font[xx-1];
                    if (yy > 0 && lb == 0) lb = font[xx-fontSize];
                    if (font[xx]) {
                        rgba[ (yy + y) * rowPixels + xx + x] = color;
                    } else if (lb) {
                        rgba[ (yy + y) * rowPixels + xx + x] = BLACK;
                    }
                }
                font += fontSize;
            }
        } else {
            for (int yy=0; yy < fontSize; ++yy) {
                for (int xx=0; xx < fontSize; ++xx) {
                    rgba[ (yy + y) * rowPixels + xx + x] = *font ? color : BLACK;
                    ++font;
                }
            }
        }
        x+= fontSize;
    }
}

void CMapWidget::drawTile(CFrame & bitmap, const int x, const int y, CFrame & tile, const bool alpha)
{
    const int WIDTH = bitmap.len();
    const uint32_t *tileData = tile.getRGB();
    uint32_t *dest = bitmap.getRGB() + x + y * WIDTH;
    if (alpha) {
        for (int row=0; row < tile.hei(); ++row) {
            for (int col=0; col < tile.len(); ++col) {
                const uint32_t & rgba = tileData[col];
                if (rgba & ALPHA) {
                    dest[col] = rgba;
                }
            }
            dest += WIDTH;
            tileData += tile.len();
        }
    } else {
        for (int row=0; row < tile.hei(); ++row) {
            int i = 0;
            dest[i++] = *(tileData++);
            dest[i++] = *(tileData++);
            dest[i++] = *(tileData++);
            dest[i++] = *(tileData++);

            dest[i++] = *(tileData++);
            dest[i++] = *(tileData++);
            dest[i++] = *(tileData++);
            dest[i++] = *(tileData++);

            dest[i++] = *(tileData++);
            dest[i++] = *(tileData++);
            dest[i++] = *(tileData++);
            dest[i++] = *(tileData++);

            dest[i++] = *(tileData++);
            dest[i++] = *(tileData++);
            dest[i++] = *(tileData++);
            dest[i++] = *(tileData++);
            dest += WIDTH;
        }
    }
}
