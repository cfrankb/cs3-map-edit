#include "mapscroll.h"
#include <algorithm>
#include <QScrollBar>
#include <QMouseEvent>
#include "mapwidgetgl.h"
#include "mapwidgetgdi.h"
#include "map.h"

CMapScroll::CMapScroll(QWidget *parent)
    : QAbstractScrollArea{parent}
{
    CMapWidgetGL *glwidget = new CMapWidgetGL(this);
    if (glwidget->isValid()) {
        m_isGlWidget = true;
        setViewport(glwidget);
    } else {
        qDebug("glWidget not valid. using fallback.");
        delete glwidget;
        CMapWidgetGDI *widget = new CMapWidgetGDI(this);
        setViewport(widget);
    }

    // set view attributes
    setAttribute(Qt::WA_MouseTracking);
    setMouseTracking(true);
    setAcceptDrops(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFrameShape(QFrame::NoFrame);
    m_mouse.x = m_mouse.y = 0;
    m_mouse.lButton = m_mouse.rButton = m_mouse.mButton = false;
    update();
}

void CMapScroll::resizeEvent(QResizeEvent * event)
{
    if (m_isGlWidget) {
        CMapWidgetGL * glw = dynamic_cast<CMapWidgetGL *>(viewport());
        glw->resizeEvent(event);
    } else {
        CMapWidgetGDI * glw = dynamic_cast<CMapWidgetGDI *>(viewport());
        glw->resizeEvent(event);
    }
    updateScrollbars();
}

void CMapScroll::paintEvent(QPaintEvent *event)
{
    if (m_isGlWidget) {
        CMapWidgetGL * glw = dynamic_cast<CMapWidgetGL *>(viewport());
        glw->paintEvent(event);
    } else {
        CMapWidgetGDI * glw = dynamic_cast<CMapWidgetGDI *>(viewport());
        glw->paintEvent(event);
    }
}

void CMapScroll::updateScrollbars()
{
    QSize sz = size();
    int h = sz.width() / GRID_SIZE;
    int v = sz.height() / GRID_SIZE;

    horizontalScrollBar()->setRange(0, m_mapLen - h);
    verticalScrollBar()->setRange(0, m_mapHei - v);

    horizontalScrollBar()->setPageStep(STEPS);
    verticalScrollBar()->setPageStep(STEPS);
}

void CMapScroll::mousePressEvent(QMouseEvent * event)
{
    switch (event->button())
    {
    case Qt::LeftButton:
        m_mouse.lButton = true;
        break;
    case Qt::RightButton:
        m_mouse.rButton = true;
        break;
    default:
        break;
    }

    setFocus();
    if (m_mouse.lButton && (m_mouse.x >= 0 && m_mouse.y >= 0)) {
        emit leftClickedAt(m_mouse.x, m_mouse.y);
    }
}

void CMapScroll::mouseReleaseEvent(QMouseEvent * event)
{
    switch (event->button())
    {
    case Qt::LeftButton:
        m_mouse.lButton = false;
        break;
    case Qt::RightButton:
        m_mouse.rButton = false;
        break;
    default:
        break;
    }
    setFocus();
}

void CMapScroll::mouseMoveEvent(QMouseEvent * event)
{
    m_mouse.x = event->x() / GRID_SIZE + horizontalScrollBar()->value();
    m_mouse.y = event->y() / GRID_SIZE + verticalScrollBar()->value();

    QString str = QString ("x: %1 y: %2").arg(m_mouse.x).arg(m_mouse.y);
    emit statusChanged(str);

    if (m_mouse.lButton && (m_mouse.x >= 0 && m_mouse.y >= 0)) {
        emit leftClickedAt(m_mouse.x, m_mouse.y);
    }
}

void CMapScroll::wheelEvent(QWheelEvent *event)
{
    QPoint numPixels = event->pixelDelta();
    QPoint numDegrees = event->angleDelta() / 8;

    enum {
        NONE=0, UP=1, DOWN=2,
    };
    int dir = NONE;
    if (!numPixels.isNull()) {
     dir = numPixels.ry() > 0 ? UP : DOWN;
    } else {
     dir = numDegrees.ry() > 0 ? UP : DOWN;
    }

    int val = verticalScrollBar()->value();
    if (dir == UP) {
        val -= STEPS;
        val = std::max(0, val);
    } else if (dir == DOWN) {
       val += STEPS;
       val = std::min(val,  verticalScrollBar()->maximum());
    }
    verticalScrollBar()->setValue(val);
    event->accept();
}

void CMapScroll::newMapSize(int len, int hei)
{
    m_mapLen = len;
    m_mapHei = hei;
    updateScrollbars();
    horizontalScrollBar()->setSliderPosition(0);
    verticalScrollBar()->setSliderPosition(0);
}

void CMapScroll::newMap(CMap* map)
{
    if (m_isGlWidget) {
        CMapWidgetGL * glw = dynamic_cast<CMapWidgetGL *>(viewport());
        glw->setMap(map);
    } else {
        CMapWidgetGDI * glw = dynamic_cast<CMapWidgetGDI *>(viewport());
        glw->setMap(map);
    }
    newMapSize(map->len(), map->hei());
}

bool CMapScroll::isGlWidget()
{
    return m_isGlWidget;
}
