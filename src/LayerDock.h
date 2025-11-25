#pragma once

#include <QDockWidget>
#include <QListWidget>
#include <QIcon>

class CLayer;
class CMap;
class QPushButton;
class QUndoStack;

class LayerDock : public QDockWidget
{
    Q_OBJECT
public:
    explicit LayerDock(CMap *map, QUndoStack* stack, QWidget *parent = nullptr);

signals:
    void visibilityChanged(int layerID, bool visible);
    void layerSelected(int layerID);
    void layersChanged(); // major change in the layer count or order
    void layerUpdated(int layerID); // minor change such as name, baseID (must still reload layerdata)
    void requestRenameLayer(int layerID);
    void requestDeleteLayer(int layerID);
    void requestChangeBaseID(int layerID);

private slots:
    void onContextMenu(const QPoint& pos);
    void updateRowHighlights();
    void renameLayer(int layerID);
    void changeBaseID(int layerID);

public slots:
    void refreshList(CMap *map);
    void updateRow(int layerID);

private:
    CMap *m_map;
    QListWidget *m_listWidget;
    QPushButton *m_addButton;
    QUndoStack* m_undoStack;

    QIcon m_eyeOpen;
    QIcon m_eyeClosed;

    // Local visibility states: layerID → visible
    QHash<int, bool> m_visibility;

    QString getLayerText(CLayer *layer);
};
