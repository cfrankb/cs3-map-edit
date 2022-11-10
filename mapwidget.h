#ifndef CMAPWIDGET_H
#define CMAPWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_2_0>
#include <QWidget>
#include <QTimer>
#include <stdafx.h>

class CMap;

class CMapWidget :
        public QOpenGLWidget,
        protected QOpenGLFunctions_2_0
{
    Q_OBJECT
public:
    explicit CMapWidget(QWidget *parent = nullptr);
    virtual ~CMapWidget();
    void getGLInfo(QString &vendor, QString &renderer, QString &version, QString &extensions);
    void setMap(CMap *pMap);

protected:
    int m_mx;
    int m_my;
    enum {
        TICK_RATE = 30
    };

    virtual void initializeGL() override;
    virtual void paintGL() override;
    virtual void resizeGL(int w, int h) override;

    bool loadTiles();
    bool loadFont();
    void drawCheckers();
    void drawBackground();
    void paintTexture(int x, int y, GLint textureId);
    void drawTile(const int x1, const int y1, const int tile, const bool fast);
    void drawChar(const int x, const int y, uint8_t ch, const bool fast);
    void drawString(const int x, const int y, const char *s);
    void drawMap();
    GLint loadTexture(const uint32_t *data, const int width, const int height);

    QTimer m_timer;
    GLint m_textureTiles;
    GLint m_textureFont;
    CMap *m_map;

signals:
    friend class CMapScroll;
};

#endif // CMAPWIDGET_H
