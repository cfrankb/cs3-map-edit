#include "mapscroll.h"
#include <algorithm>
#include <QScrollBar>
#include <QMouseEvent>
#include <QMenu>
#include "mapwidget.h"

#define STEPS 8
#define MAX_RANGE 64

CMapScroll::CMapScroll(QWidget *parent)
    : QAbstractScrollArea{parent}
{
    m_widget = new CMapWidget(this);
    setViewport(m_widget);

    // set view attributes
    setAttribute(Qt::WA_MouseTracking);
    setMouseTracking(true);
    setAcceptDrops(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFrameShape(QFrame::NoFrame);

    //connect(horizontalScrollBar(), SIGNAL(valueChanged(int)), m_widget, SLOT(setMX(int)));
    //connect(verticalScrollBar(), SIGNAL(valueChanged(int)), m_widget, SLOT(setMY(int)));
    m_mouse.x = m_mouse.y = 0;
    m_mouse.lButton = m_mouse.rButton = m_mouse.mButton = false;

    update();
}


void CMapScroll::resizeEvent(QResizeEvent * event)
{
    CMapWidget * glw = dynamic_cast<CMapWidget *>(viewport());
    glw->resizeEvent(event);
    updateScrollbars();
}

void CMapScroll::paintEvent(QPaintEvent *event)
{
    CMapWidget * glw = dynamic_cast<CMapWidget *>(viewport());
    glw->paintEvent(event);
}

void CMapScroll::updateScrollbars()
{
    horizontalScrollBar()->setRange(0, MAX_RANGE);
    verticalScrollBar()->setRange(0, MAX_RANGE);

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

    case Qt::MidButton:
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
    case Qt::MidButton:
        m_mouse.mButton = false;
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

    // MouseMove isn't called unless a button is pressed
            //xx = (event->x() + gridSize * horizontalScrollBar()->value()) ;
            //yy = (event->y() + gridSize * verticalScrollBar()->value());
            //if (xx < width && yy < height) {
              //  x = xx / gridSize;
               // y = yy / gridSize;
           // }
        //}
/*
        // Test if mouse in within valid coordonates
        if (event->x() >= sz.width()
                 ||event->y() >= sz.height() ) {
            setCursor(Qt::ArrowCursor);
            return;
        }
        m_mouse.oldX = m_mouse.x;
        m_mouse.oldY = m_mouse.y;
        m_mouse.x = x;
        m_mouse.y = y;
    }

    QString s = "";
    if (x < 0 || y < 0) {
        setCursor(Qt::ForbiddenCursor);
    } else {
        if (widget->m_frame) {
            uint rgb = widget->m_frame->at(x,y);
            char hex[12];
            int red = rgb & 0xff;
            int green = (rgb >> 8) & 0xff;
            int blue = (rgb >> 16) & 0xff;
            int alpha = (rgb >> 24) & 0xff;
            sprintf(hex, "%.2X%.2X%.2X:%.2X", red, green, blue, alpha);
            s = QString("x=%1 y=%2 (#%3)").arg(x).arg(y).arg(hex);
        }
        emit cursorChanged();
        if ((m_mouse.lButton | m_mouse.rButton) &&
             (m_mouse.x >= 0 && m_mouse.y >= 0
                    && (m_widget->m_mode != CFrameWidget::MODE_TILED_VIEW))) {
    //        handleTool();
            emit toolHandler(m_mouse.x, m_mouse.y, m_mouse.lButton, m_mouse.rButton);
        }
    }
   // emit statusUpdate(0,s);*/
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
       val = std::min(val, MAX_RANGE);
    }
    verticalScrollBar()->setValue(val);
    event->accept();
}
