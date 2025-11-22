// MapView.cpp
#include "MapView.h"
#include <QWheelEvent>
#include <QScrollBar>
#include <QApplication>

MapView::MapView(QWidget *parent)
    : QScrollArea(parent)
{
    m_mapWidget = new MapWidget(this);
    //m_mapWidget->preloadAssets();

    setWidget(m_mapWidget);
    //setWidgetResizable(true);      // Important: lets the widget shrink/grow
    setWidgetResizable(false);
    setAlignment(Qt::AlignTop | Qt::AlignLeft);
    //setAlignment(Qt::AlignCenter); // Centers small maps
    setBackgroundRole(QPalette::Dark);
    setFrameShape(QFrame::NoFrame);

    // Enable smooth mouse wheel zooming
    m_mapWidget->installEventFilter(this);
    viewport()->installEventFilter(this);

    // Optional: nice scroll behavior
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);


}


void MapView::setMap(CMap *map)
{
    qDebug("MapView::setMap(CMap *map)");

    ensureVisible(0, 0, 0, 0);
    m_mapWidget->setMap(map);

    if (map) {
        int tileSize = 16 * m_mapWidget->zoom();
        m_mapWidget->resize(map->len() * tileSize, map->hei() * tileSize);
    } else {
        m_mapWidget->resize(0, 0);
    }

   // centerOnMap();
}

void MapView::setZoom(int factor)
{
    if (factor < 1) factor = 1;
    if (factor > 8) factor = 8;

    if (factor == m_mapWidget->zoom())
        return;

    // Remember visible center before zoom
    QPoint viewCenter = viewport()->rect().center();
    QPointF oldMapPos = m_mapWidget->mapFromParent(
        mapToGlobal(viewCenter)
        );

    m_mapWidget->setZoom(factor);

    // Force MapWidget to exact pixel size
    if (m_mapWidget->map()) {
        int tileSize = 16 * factor;
        int pixelW = m_mapWidget->map()->len() * tileSize;
        int pixelH = m_mapWidget->map()->hei() * tileSize;
        m_mapWidget->resize(pixelW, pixelH);
    } else {
        m_mapWidget->resize(0, 0);
    }

    // Restore center point after zoom
    QPoint newGlobal = m_mapWidget->mapToParent(oldMapPos.toPoint());
    ensureVisible(newGlobal.x(), newGlobal.y(), 0, 0);
}


void MapView::centerOnMap()
{
    qDebug("centerOnMap");
    if (!m_mapWidget->map())
        return;

    const int tileW = 16 * m_mapWidget->zoom();
    const int tileH = 16 * m_mapWidget->zoom();

    int centerX = (m_mapWidget->map()->len() * tileW) / 2;
    int centerY = (m_mapWidget->map()->hei() * tileH) / 2;

    ensureVisible(centerX, centerY, width() / 2, height() / 2);
}

void MapView::centerOnTile(int tileX, int tileY)
{
    qDebug("centerOnTile");

    const int tileSize = 16 * m_mapWidget->zoom();
    int pixelX = tileX * tileSize + tileSize / 2;
    int pixelY = tileY * tileSize + tileSize / 2;
    ensureVisible(pixelX, pixelY, width() / 2, height() / 2);

}

bool MapView::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Wheel)
    {
        QWheelEvent *wheel = static_cast<QWheelEvent *>(event);

        // Ctrl + Wheel = Zoom
        if (wheel->modifiers() & Qt::ControlModifier)
        {
            const int delta = wheel->angleDelta().y();
            if (delta > 0)
                zoomIn();
            else if (delta < 0)
                zoomOut();

            return true; // consume event
        }
    }

    static QPoint lastPanPoint;
    QWheelEvent *wheel = static_cast<QWheelEvent *>(event);

    if (event->type() == QEvent::MouseButtonPress && wheel->button() == Qt::MiddleButton) {
        lastPanPoint = wheel->position().toPoint();
        setCursor(Qt::ClosedHandCursor);
        return true;
    }
    else if (event->type() == QEvent::MouseMove && (wheel->buttons() & Qt::MiddleButton)) {
        QPoint delta = wheel->position().toPoint() - lastPanPoint;
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        lastPanPoint = wheel->position().toPoint();
        return true;
    }
    else if (event->type() == QEvent::MouseButtonRelease && wheel->button() == Qt::MiddleButton) {
        setCursor(Qt::ArrowCursor);
        return true;
    }


    return QScrollArea::eventFilter(obj, event);
}

/*

// In your MainWindow constructor
#include "MapView.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_mapView = new MapView(this);
    setCentralWidget(m_mapView);

    // Connect signals
    connect(m_mapView->mapWidget(), &MapWidget::tilePicked,
            this, &MainWindow::onTilePicked);

    connect(m_mapView->mapWidget(), &MapWidget::mapModified,
            this, &MainWindow::setWindowModified);

    // Example: load a level
    loadLevel("levels/level1.map");
}

void MainWindow::loadLevel(const QString &fileName)
{
    CMap *map = new CMap();
    if (map->read(fileName.toLocal8Bit().constData())) {
        m_mapView->setMap(map);
        m_mapView->setTileSet(m_globalTileSetFrames); // your vector<CFrame*>
        m_mapView->setZoom(2);        // start at 200%
        m_mapView->centerOnMap();
    } else {
        delete map;
        QMessageBox::warning(this, "Error", map->lastError());
    }
}

// Optional toolbar actions
void MainWindow::on_actionZoom_In_triggered()
{
    m_mapView->zoomIn();
}

void MainWindow::on_actionZoom_Out_triggered()
{
    m_mapView->zoomOut();
}

void MainWindow::on_actionFit_Map_triggered()
{
    if (!m_mapView->mapWidget()->map()) return;

    int mapPixelW = m_mapView->mapWidget()->map()->len() * 16;
    int mapPixelH = m_mapView->mapWidget()->map()->hei() * 16;

    int zoomX = width() / mapPixelW;
    int zoomY = height() / mapPixelH;
    int newZoom = qMax(1, qMin(zoomX, zoomY));

    m_mapView->setZoom(newZoom);
    m_mapView->centerOnMap();
}


void MainWindow::on_actionRevert_triggered()
{
    if (!m_isDirty) return;

    auto ret = QMessageBox::question(this, "Revert",
        "Discard all changes and reload from disk?");
    if (ret == QMessageBox::Yes) {
        openFile(currentFileName());   // reloads and sets dirty = false
    }
}

*/
