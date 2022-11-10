#ifndef CMAPSCROLL_H
#define CMAPSCROLL_H

#include "data.h"
#include <QWidget>
#include <QAbstractScrollArea>
class CMapWidget;

class CMapScroll : public QAbstractScrollArea
{
    Q_OBJECT
public:
    explicit CMapScroll(QWidget *parent = nullptr);
    CMapWidget *m_widget;


signals:
    void statusChanged(const QString str);

protected:
    virtual void resizeEvent(QResizeEvent * event);
    virtual void paintEvent(QPaintEvent *event);
    virtual void wheelEvent(QWheelEvent *);
    virtual void mousePressEvent(QMouseEvent * event);
    virtual void mouseReleaseEvent(QMouseEvent * event);
    virtual void mouseMoveEvent(QMouseEvent * event);
    virtual void mouseDoubleClickEvent(QMouseEvent *event);

    void updateScrollbars();

    typedef struct {
        int x;
        int y;
        bool lButton;
        bool rButton;
        bool mButton;
    } Mouse;
    Mouse m_mouse;

    enum {
        GRID_SIZE = 32
    };

};

#endif // CMAPSCROLL_H
