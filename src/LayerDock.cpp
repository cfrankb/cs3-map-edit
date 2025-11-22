#include "LayerDock.h"
#include <QString>
#include "runtime/map.h"

LayerDock::LayerDock(CMap *map, QWidget *parent)
    : QDockWidget("Layers", parent),
      m_map(map)
{
    m_list = new QListWidget(this);
    m_list->setSelectionMode(QAbstractItemView::NoSelection);
    setWidget(m_list);

    m_eyeOpen = QIcon(":/data/icons/eye_818577.png");    // freepik
    m_eyeClosed = QIcon(":data/icons/blind_795831.png"); // freepik

    refreshList(map);

    connect(m_list, &QListWidget::itemClicked,
            this, &LayerDock::onItemClicked);
}

void LayerDock::refreshList(CMap *map)
{
    m_map = map;
    m_list->clear();
    m_visibility.clear();

    //
    // Add mainLayer → id = -1
    //

    QString text = QString("%1 (%2)")
                       .arg(QString::fromStdString(m_map->getMainLayer().getName()))
                       .arg("Main");

    QListWidgetItem *item = new QListWidgetItem(text);
    item->setData(Qt::UserRole, -1);

    bool visible = true; // default
    m_visibility[-1] = visible;
    item->setIcon(m_eyeOpen);

    m_list->addItem(item);

    //
    // Add other layers: IDs = index in vector
    //
    auto &layers = m_map->layers();

    int i = 0;
    for (auto &layerU : layers)
    {
        CLayer *layer = layerU.get();
        if (!layer)
            continue;

        /*
           LAYER_MAIN,
                LAYER_FLOOR,
                LAYER_WALLS,
                LAYER_DECOR,
        */

        QString typeStr;
        switch (layer->layerType())
        {
        case CLayer::LayerType::LAYER_MAIN:
            typeStr = "Main";
            break;
        case CLayer::LayerType::LAYER_FLOOR:
            typeStr = "Floor";
            break;
        case CLayer::LayerType::LAYER_WALLS:
            typeStr = "Walls";
            break;
        case CLayer::LayerType::LAYER_DECOR:
            typeStr = "Decor";
            break;
        default:
            typeStr = "Unknown";
            break;
        }

        QString text = QString("%1 [%2]")
                           .arg(QString::fromStdString(layer->getName()))
                           .arg(typeStr);

        QListWidgetItem *item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, i);

        bool visible = true; // default
        m_visibility[i] = visible;
        item->setIcon(m_eyeOpen);

        m_list->addItem(item);
        ++i;
    }
}

void LayerDock::onItemClicked(QListWidgetItem *item)
{
    int layerID = item->data(Qt::UserRole).toInt();

    bool visible = !m_visibility[layerID];
    m_visibility[layerID] = visible;

    item->setIcon(visible ? m_eyeOpen : m_eyeClosed);

    emit visibilityChanged(layerID, visible);
}
