#include "mapwidgetgdi.h"
#include "qpainter.h"
#include "shared/qtgui/qfilewrap.h"
#include "FrameSet.h"
#include "Frame.h"
#include "map.h"
#include "mapscroll.h"
#include <QScrollBar>

CMapWidgetGDI::CMapWidgetGDI(QWidget *parent)
    : QWidget{parent}
{
    m_timer.setInterval(1000 / TICK_RATE);
    m_timer.start();
    preloadAssets();
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(update()));
}

CMapWidgetGDI::~CMapWidgetGDI()
{
    m_timer.stop();
}

void CMapWidgetGDI::setMap(CMap *pMap)
{
    m_map = pMap;
}

void CMapWidgetGDI::showGrid(bool show)
{
    m_showGrid = show;
}

void CMapWidgetGDI::preloadAssets()
{
    QFileWrap file;

    typedef struct {
        const char *filename;
        CFrameSet **frameset;
    } asset_t;

    asset_t assets[] = {
                        {":/data/tiles.obl", &m_tiles},
                        };

    for (int i=0; i < 1; ++i) {
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

void CMapWidgetGDI::paintEvent(QPaintEvent *)
{
    const QSize widgetSize = size();
    const int width = widgetSize.width() / 2 + TILE_SIZE;
    const int height = widgetSize.height() / 2 + TILE_SIZE;

    CFrame bitmap(width, height);
    drawScreen(bitmap);
    if (m_showGrid) {
        drawGrid(bitmap);
    }

    // show the screen
    const QImage img = QImage(reinterpret_cast<uint8_t*>(bitmap.getRGB()), bitmap.m_nLen, bitmap.m_nHei, QImage::Format_RGBX8888);
    const QPixmap pixmap = QPixmap::fromImage(img.scaled(QSize(width * 2, height * 2)));
    QPainter p(this);
    p.drawPixmap(0, 0, pixmap);
    p.end();
}

void CMapWidgetGDI::drawScreen(CFrame &bitmap)
{
    CMap *map = m_map;
    if (!map) {
        qDebug("map is null");
        return;
    }

    int maxRows = bitmap.m_nHei / TILE_SIZE;
    int maxCols = bitmap.m_nLen / TILE_SIZE;
    const int rows = std::min(maxRows, map->hei());
    const int cols = std::min(maxCols, map->len());
    CMapScroll *scr = static_cast<CMapScroll*>(parent());
    const int mx = scr->horizontalScrollBar()->value();
    const int my = scr->verticalScrollBar()->value();

    CFrameSet & tiles = *m_tiles;
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
            tile = tiles[tileID];
            drawTile(bitmap, x * TILE_SIZE, y * TILE_SIZE, *tile, false);
            uint8_t a = map->getAttr(mx+x, my+y);
            if (a) {
                char s[4];
                sprintf(s, "%.2x", a);
                drawFont(bitmap, x*TILE_SIZE, y*TILE_SIZE + 4, s, YELLOW);
            }
        }
    }
}

void CMapWidgetGDI::drawGrid(CFrame & bitmap)
{
    CMap *map = m_map;
    if (!map) {
        qDebug("map is null");
        return;
    }

    int maxRows = bitmap.m_nHei / TILE_SIZE;
    int maxCols = bitmap.m_nLen / TILE_SIZE;
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

            for (unsigned int yy=0; yy< TILE_SIZE; ++yy) {
                for (unsigned int xx=0; xx< TILE_SIZE; ++xx) {
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

void CMapWidgetGDI::drawFont(CFrame & frame, int x, int y, const char *text, const uint32_t color)
{
    uint32_t *rgba = frame.getRGB();
    const int rowPixels = frame.m_nLen;
    const int fontSize = 8;
    const int fontOffset = fontSize * fontSize;
    const int textSize = strlen(text);
    for (int i=0; i < textSize; ++i) {
        const uint8_t c = static_cast<uint8_t>(text[i]) - ' ';
        uint8_t *font = m_fontData + c * fontOffset;
        for (int yy=0; yy < fontSize; ++yy) {
            for (int xx=0; xx < fontSize; ++xx) {
                rgba[ (yy + y) * rowPixels + xx + x] = *font ? color : BLACK;
                ++font;
            }
        }
        x+= fontSize;
    }
}

void CMapWidgetGDI::drawTile(CFrame & bitmap, const int x, const int y, CFrame & tile, bool alpha)
{
    const int WIDTH = bitmap.m_nLen;
    const uint32_t *tileData = tile.getRGB();
    uint32_t *dest = bitmap.getRGB() + x + y * WIDTH;
    if (alpha) {
        for (int row=0; row < tile.m_nHei; ++row) {
            for (int col=0; col < tile.m_nLen; ++col) {
                const uint32_t & rgba = tileData[col];
                if (rgba & ALPHA) {
                    dest[col] = rgba;
                }
            }
            dest += WIDTH;
            tileData += tile.m_nLen;
        }
    } else {
        for (int row=0; row < tile.m_nHei; ++row) {
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
