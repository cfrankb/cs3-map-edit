#include "mapwidget.h"

#include <stdint.h>
#include <QScrollBar>
#include <QContextMenuEvent>
#include <QMenu>
#include "data.h"
#include "map.h"
#include "mapscroll.h"

void openglError(unsigned int code, const char *file, int line){
    std::string tmp;
    switch(code) {
        case GL_NO_ERROR:
            tmp = "GL_NO_ERROR";
        break;
        case GL_INVALID_ENUM:
            tmp = "GL_INVALID_ENUM";
        break;
        case GL_INVALID_VALUE:
            tmp = "GL_INVALID_VALUE";
        break;
        case GL_INVALID_OPERATION:
            tmp = "GL_INVALID_OPERATION";
        break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            tmp = "GL_INVALID_FRAMEBUFFER_OPERATION";
        break;
        case GL_OUT_OF_MEMORY:
            tmp = "GL_OUT_OF_MEMORY";
        break;
        default:
            char t[40];
            sprintf(t, "GL UNKNOWN:%u", code);
            tmp = t;
    }
    if (code != GL_NO_ERROR) {
        qDebug("Opengl error: %s in %s line %d",
            tmp.c_str(), file, line); \
    }
}
#define GLDEBUG() openglError(glGetError(), __FILE__, __LINE__ );

CMapWidget::CMapWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    setUpdateBehavior(QOpenGLWidget::PartialUpdate);
    m_timer.setInterval(1000 / TICK_RATE);
    m_timer.start();
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(update()));
    setContentsMargins(QMargins(0, 0, 0, 0));
    setMouseTracking(true);
    setAttribute(Qt::WA_MouseTracking);
    m_textureTiles = -1;
    m_textureFont = -1;
    m_map = nullptr;
}

CMapWidget::~CMapWidget()
{
}

void CMapWidget::getGLInfo(QString &vendor, QString &renderer, QString &version, QString &extensions)
{
    vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
}

void CMapWidget::initializeGL()
{
    QOpenGLWidget::initializeGL();
    initializeOpenGLFunctions();
    qDebug("vendor: %s", glGetString(GL_VENDOR));
    qDebug("renderer: %s", glGetString(GL_RENDERER));
    qDebug("version: %s", glGetString(GL_VERSION));
    qDebug("extensions: %s", glGetString(GL_EXTENSIONS));

    loadTiles();
    loadFont();
}

void CMapWidget::paintGL()
{
    QOpenGLWidget::paintGL();
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    drawBackground();
}

void CMapWidget::resizeGL(int w, int h)
{
    QOpenGLWidget::resizeGL(w, h);
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void CMapWidget::drawBackground()
{
     QSize sz = size();
     glDisable(GL_DEPTH_TEST);
     glDisable(GL_TEXTURE_2D);
     glMatrixMode(GL_PROJECTION);
     glLoadIdentity();
     glOrtho(0,sz.width(),0,sz.height(),-1,1);
     glViewport(
             0,          // lower left x position
             0,			// lower left y position
             sz.width(),	// viewport width
             sz.height()	// viewport height
     );
     glMatrixMode(GL_MODELVIEW);
     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
     glLoadIdentity();
     glShadeModel( GL_FLAT );
     glDisable(GL_LIGHTING);
     glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
     glPushMatrix();
         glTranslatef(0,0,0);
             drawMap();
             if (m_showGrid) {
                 drawGrid();
             }
     glPopMatrix();
     glFlush();
     glFinish();
}

void CMapWidget::drawCheckers()
{
    QSize sz = size();
    int gridSize = TILE_SIZE;
    for (int y=0; y * gridSize < sz.height(); ++y) {
        for (int x=0; x * gridSize < sz.width(); ++x) {
            if ((x + y) % 2 == 0) {
                int x1 = x * gridSize;
                int x2 = (x + 1) * gridSize;
                int y1 = sz.height() - y * gridSize;
                int y2 = sz.height() - (y + 1) * gridSize;
                glBegin(GL_QUADS);
                    glColor4f(0.25f, 0.5f, 1.0f, 1.0f);
                    glVertex2f(x1, y1);  //Draw the four corners of the rectangle
                    glVertex2f(x2, y1);
                    glVertex2f(x2, y2);
                    glVertex2f(x1, y2);
                glEnd();
            }
        }
    }
}

bool CMapWidget::loadTiles()
{
    const uint32_t *data = loadTileData();
    if (data == nullptr) {
        return false;
    }
    m_textureTiles = loadTexture(data, TEXTURE_WIDTH, TEXTURE_WIDTH);
    return true;
}

bool CMapWidget::loadFont()
{
    const uint32_t * data = loadFontData();
    if (data == nullptr) {
        qDebug("fail to load font");
        return false;
    }
    m_textureFont = loadTexture(data, FONT_TEXTURE_WIDTH, FONT_TEXTURE_HEIGHT);
    qDebug("textureFont: %d", m_textureFont);
    return true;
}

GLint CMapWidget::loadTexture(const uint32_t* data, const int width, const int height)
{
    makeCurrent();
    GLuint textureId = -1;
    GLint maxSize;
    glEnable(GL_TEXTURE_2D); GLDEBUG();
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize); GLDEBUG();
    glGenTextures(1, &textureId); GLDEBUG();

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4); GLDEBUG();
    glBindTexture(GL_TEXTURE_2D, textureId); GLDEBUG();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data ); GLDEBUG();
    glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); GLDEBUG();
    glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST); GLDEBUG();

    delete []data;
    return textureId;
}

/* test texture output */
void CMapWidget::paintTexture(int x, int y, GLint textureId)
{
    QSize sz = size();
    int x1 = x;
    int y1 = sz.height() - y;
    glEnable(GL_TEXTURE_2D);
    glEnable (GL_BLEND);
    glDisable(GL_MULTISAMPLE);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBindTexture(GL_TEXTURE_2D, textureId);

    int ix = TEXTURE_WIDTH *2;
    int iy = TEXTURE_HEIGHT *2;
    int x2 = x1 + ix;
    int y2 = y1 - iy;

    glBegin(GL_QUADS);
        // bottom left
        glTexCoord2f(0.0, 0.0); glVertex3f(x1, y2, 0.0);
        // top left
        glTexCoord2f(0.0, 1.0f); glVertex3f(x1, y1, 0.0);
        // top right
        glTexCoord2f(1.0f, 1.0f); glVertex3f(x2, y1, 0.0);
        // bottom right
        glTexCoord2f(1.0f, 0.0); glVertex3f(x2, y2, 0.0);
    glEnd();
}

void CMapWidget::drawTile(const int x, const int y, const int tile, const bool fast)
{
    const QSize sz = size();
    const int x1 = x;
    const int y1 = sz.height() - y;
    if (!fast){
        // only preload texture on demand
        glEnable(GL_TEXTURE_2D);
        glEnable (GL_BLEND);
        glDisable(GL_MULTISAMPLE);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glBindTexture(GL_TEXTURE_2D, m_textureTiles);
    }
    const int col = tile & (TEX_TILE_SIZE-1);
    const int row = tile / TEX_TILE_SIZE;
    const float gx = col * TEX_TILE_SIZE;
    const float gy = (15 - row) * TEX_TILE_SIZE;

    const int x2 = x1 + TILE_SIZE;
    const int y2 = y1 - TILE_SIZE;

    const float w = TEXTURE_WIDTH;
    const float h = TEXTURE_HEIGHT;
    const float a1 = gx / w;
    const float b1 = 1.0-(gy / h);
    const float a2 = (gx + TEX_TILE_SIZE)/ w;
    const float b2 = 1.0-(gy + TEX_TILE_SIZE) / h;

    glBegin(GL_QUADS);
        // bottom left
        glTexCoord2f(a1, b2); glVertex3f(x1, y2, 0.0);
        // top left
        glTexCoord2f(a1,b1); glVertex3f(x1, y1, 0.0);
        // top right
        glTexCoord2f(a2,b1); glVertex3f(x2, y1, 0.0);
        // bottom right
        glTexCoord2f(a2, b2); glVertex3f(x2, y2, 0.0);
    glEnd();
}

void CMapWidget::setMap(CMap *pMap)
{
   m_map = pMap;
}

void CMapWidget::drawMap(){

    CMapScroll *scr = static_cast<CMapScroll*>(parent());
    const int mx = scr->horizontalScrollBar()->value();
    const int my = scr->verticalScrollBar()->value();
    glEnable(GL_TEXTURE_2D);
    glEnable (GL_BLEND);
    glDisable(GL_MULTISAMPLE);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBindTexture(GL_TEXTURE_2D, m_textureTiles);
    const QSize sz = size();
    const int cols = sz.width() / TILE_SIZE + (sz.width() % TILE_SIZE ? 1 :0);
    const int rows = sz.height() / TILE_SIZE + (sz.height() % TILE_SIZE ? 1 :0);;

    if (m_map == nullptr) {
        qDebug("map is null");
        return;
    }
    CMap &map = *m_map;

    bool fast = true;
    for (int y=0; y < rows; ++y){
        if (y + my >= m_map->hei()) break;
        for (int x=0; x < cols; ++x){
            if (x + mx>= m_map->len()) break;
            drawTile(x*TILE_SIZE,
                      y*TILE_SIZE,
                     map.at(mx+x, my+y),
                      fast
                      );
            uint8_t a = map.getAttr(mx+x, my+y);
            if (a) {
                char s[16];
                sprintf(s, "%.2x", a);
                drawString(x*TILE_SIZE, y*TILE_SIZE + 8, s);
                fast = false;
                continue;
            }
            fast = true;
        }
    }
}

void CMapWidget::drawChar(const int x, const int y, uint8_t ch, const bool fast)
{
    const int fontPerRow= FONT_TEXTURE_WIDTH / FONT_SIZE;
    const int fontWidth = FONT_SIZE;
    const int fontHeight = FONT_SIZE;
    const QSize sz = size();
    const int x1 = x;
    const int y1 = sz.height() - y;
    if (!fast){
        // only preload texture on demand
        glEnable(GL_TEXTURE_2D);
        glEnable (GL_BLEND);
        glDisable(GL_MULTISAMPLE);
        glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
        glBindTexture(GL_TEXTURE_2D, m_textureFont);
    }

    const int col = ch & (fontPerRow-1);
    const int row = ch / fontPerRow;
    const float gx = col * fontWidth;
    const float gy = row * fontHeight;

    const int x2 = x1 + fontWidth * 2;
    const int y2 = y1 - fontHeight * 2;

    const float w = FONT_TEXTURE_WIDTH;
    const float h = FONT_TEXTURE_HEIGHT;
    const float a1 = gx / w;
    const float b1 = 1.0-(gy / h);
    const float a2 = (gx + fontWidth)/ w;
    const float b2 = 1.0-(gy + fontHeight) / h;

    glBegin(GL_QUADS);
        // bottom left
        glTexCoord2f(a1, b2); glVertex3f(x1, y2, 0.0);
        // top left
        glTexCoord2f(a1,b1); glVertex3f(x1, y1, 0.0);
        // top right
        glTexCoord2f(a2,b1); glVertex3f(x2, y1, 0.0);
        // bottom right
        glTexCoord2f(a2, b2); glVertex3f(x2, y2, 0.0);
    glEnd();
}

void CMapWidget::drawString(const int x, const int y, const char *s)
{
    for (int i=0; s[i]; ++i) {
        drawChar(x + i * 16 , y, s[i] - 32, false);
    }
}

void CMapWidget::drawGrid()
{
    QSize sz = size();
    CMapScroll *scr = static_cast<CMapScroll*>(parent());
    const int mx = scr->horizontalScrollBar()->value() * GRID_SIZE;
    //const int my = scr->verticalScrollBar()->value() * GRID_SIZE;
    int w = std::min(sz.width(), (int) (m_map->len() * GRID_SIZE - mx));
    int h = sz.height();
    glDisable(GL_TEXTURE_2D);
    glLineWidth(0.5f);
    glEnable (GL_DST_ALPHA);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0, 0x79 / 255.0f, 0xa0/255.0f, 0xbf/255.0f);

    for (int x = GRID_SIZE  ; x < w; x+= GRID_SIZE) {
        glBegin(GL_LINES);
            glVertex2f(x, 0.0);
            glVertex2f(x, h);
        glEnd();
    }
    for (int y = GRID_SIZE ; y < h; y+= GRID_SIZE) {
        glBegin(GL_LINES);
            glVertex2f(0.0, h - y);
            glVertex2f(w, h - y);
        glEnd();
    }
}

void CMapWidget::showGrid(bool show)
{
    m_showGrid = show;
}
