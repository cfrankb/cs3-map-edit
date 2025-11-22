#include "LayerDock.h"
#include <QString>
#include <QVBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include "runtime/map.h"

LayerDock::LayerDock(CMap *map, QWidget *parent)
    : QDockWidget("Layers", parent),
      m_map(map)
{
    auto* central = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(central);

    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::NoSelection);
    //setWidget(m_list);

    m_eyeOpen = QIcon(":/data/icons/eye_818577.png");    // freepik
    m_eyeClosed = QIcon(":data/icons/blind_795831.png"); // freepik

    mainLayout->addWidget(m_listWidget);

    auto* buttonLayout = new QHBoxLayout;
    m_addButton = new QPushButton(QIcon(":/data/icons/small_chemistry.png"),"Add Layers");

    buttonLayout->addWidget(m_addButton);
    buttonLayout->addStretch(); // push buttons to the left
    mainLayout->addLayout(buttonLayout);

    setWidget(central);
    refreshList(map);

    connect(m_listWidget, &QListWidget::itemClicked, this, &LayerDock::onItemClicked);
    connect(m_addButton, &QPushButton::clicked, this, [this]() {
        m_map->addLayer(CLayer::LAYER_WALLS, "walls");
        m_map->addLayer(CLayer::LAYER_FLOOR, "floor");
        refreshList(m_map);
    });
}

void LayerDock::refreshList(CMap *map)
{
    m_map = map;
    m_visibility.clear();
    m_listWidget->clear();

    if (!m_map) return;
    if (m_addButton)
        m_addButton->setEnabled(map->layerCount() == 1);

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

        m_listWidget->addItem(item);
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
