// MapView.h
#pragma once

#include <QScrollArea>
#include "MapWidget.h"

class MapView : public QScrollArea
{
    Q_OBJECT

public:
    explicit MapView(QWidget *parent = nullptr);

    MapWidget *mapWidget() const { return m_mapWidget; }

    // Convenience wrappers

    //void setTileSet(const std::vector<CFrame *> &frames) { m_mapWidget->setTileSet(frames); }
    int zoom() const { return m_mapWidget->zoom(); }

    void centerOnMap();
    void centerOnTile(int tileX, int tileY);

public slots:
    void setMap(CMap *map);
    void setZoom(int factor);
    void zoomIn() { setZoom(zoom() + 1); }
    void zoomOut() { setZoom(zoom() - 1); }


protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    MapWidget *m_mapWidget;
};
