#ifndef CMAPSCROLL_H
#define CMAPSCROLL_H

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
    virtual void resizeEvent(QResizeEvent * event) override;
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void wheelEvent(QWheelEvent *) override;
    virtual void mousePressEvent(QMouseEvent * event) override;
    virtual void mouseReleaseEvent(QMouseEvent * event) override;
    virtual void mouseMoveEvent(QMouseEvent * event) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;

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
