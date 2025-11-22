#pragma once

#include <QDockWidget>
#include <QListWidget>
#include <QIcon>

class CMap;


class LayerDock : public QDockWidget
{
    Q_OBJECT
public:
    explicit LayerDock(CMap *map, QWidget *parent = nullptr);

signals:
    void visibilityChanged(int layerID, bool visible);

private slots:
    void onItemClicked(QListWidgetItem *item);
public slots:
    void refreshList(CMap *map);

private:
    CMap *m_map;
    QListWidget *m_list;

    QIcon m_eyeOpen;
    QIcon m_eyeClosed;

    // Local visibility states: layerID → visible
    QHash<int, bool> m_visibility;
};
