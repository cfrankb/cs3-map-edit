#pragma once

#include <QDockWidget>
#include <QListWidget>
#include <QIcon>

class CMap;
class QPushButton;


class LayerDock : public QDockWidget
{
    Q_OBJECT
public:
    explicit LayerDock(CMap *map, QWidget *parent = nullptr);

signals:
    void visibilityChanged(int layerID, bool visible);
    void layerSelected(int layerID);
    void layersChanged();
    void requestRenameLayer(int layerID);
    void requestDeleteLayer(int layerID);

private slots:
    void onContextMenu(const QPoint& pos);
    void updateRowHighlights();

public slots:
    void refreshList(CMap *map);

private:
    CMap *m_map;
    QListWidget *m_listWidget;
    QPushButton *m_addButton;

    QIcon m_eyeOpen;
    QIcon m_eyeClosed;

    // Local visibility states: layerID → visible
    QHash<int, bool> m_visibility;
};
