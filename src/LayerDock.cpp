#include "LayerDock.h"
#include <QString>
#include <QVBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QMenu>
#include "runtime/map.h"
#include "LayerRowWidget.h"

LayerDock::LayerDock(CMap *map, QWidget *parent)
    : QDockWidget("Layers", parent),
      m_map(map)
{
    auto* central = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(central);

    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::NoSelection);
    m_listWidget->setStyleSheet(
        "QListWidget::item:selected { "
        "    background: rgba(0, 120, 215, 160); "     /* Bright blue, semi-transparent */
        "    color: white; "
        "} "
        "QListWidget::item:selected:!active { "
        "    background: rgba(0, 120, 215, 160); "     /* Same color when unfocused */
        "    color: white; "
        "} "
        "QListWidget::item { "
        "    padding: 3px; "
        "} "
        );

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

    connect(m_addButton, &QPushButton::clicked, this, [this]() {
        m_map->addLayer(CLayer::LAYER_WALLS, "walls");
        m_map->addLayer(CLayer::LAYER_FLOOR, "floor");
        refreshList(m_map);
        emit layersChanged();
    });

    m_listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_listWidget, &QListWidget::customContextMenuRequested,
            this, &LayerDock::onContextMenu);

    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_listWidget->setFocusPolicy(Qt::StrongFocus);

    connect(m_listWidget, &QListWidget::itemSelectionChanged,
            this, &LayerDock::updateRowHighlights);
}


void LayerDock::updateRowHighlights()
{
    for (int i = 0; i < m_listWidget->count(); ++i) {
        QListWidgetItem *item = m_listWidget->item(i);

        if (item->isSelected()) {
            item->setBackground(QColor(60, 120, 200));   // selected color
            item->setForeground(Qt::white);               // selected text color
        } else {
            item->setBackground(Qt::transparent);
            item->setForeground(Qt::black);
        }
    }
}


void LayerDock::refreshList(CMap *map)
{
    m_map = map;
    m_visibility.clear();
    m_listWidget->clear();

    if (!m_map) return;
    if (m_addButton)
        m_addButton->setEnabled(map->layerCount() == 1);


    auto addRow = [&](int layerID, const QString& text) {
        QListWidgetItem* item = new QListWidgetItem(m_listWidget);
        item->setData(Qt::UserRole, layerID);
        item->setSizeHint(QSize(0, 26));

        bool visible = true;
        m_visibility[layerID] = visible;

        LayerRowWidget* row = new LayerRowWidget(text, visible);
        m_listWidget->setItemWidget(item, row);

        connect(row, &LayerRowWidget::eyeClicked, this, [this, layerID, item]() {
            bool v = !m_visibility[layerID];
            m_visibility[layerID] = v;

            auto* r = qobject_cast<LayerRowWidget*>(m_listWidget->itemWidget(item));
            r->findChild<QToolButton*>()->setIcon(
                v ? m_eyeOpen : m_eyeClosed
                );

            emit visibilityChanged(layerID, v);
        });

        connect(row, &LayerRowWidget::rowClicked, this, [this, layerID]() {
            emit layerSelected(layerID);
        });
    };

    // ---  layers
    int i = 0;
    for (auto &layer : m_map->layers())
    {
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
            typeStr = "???";
            break;
        }
        QString name = QString("%1 (%2)").arg(layer->getName()).arg(typeStr);
        addRow(i, name);
        ++i;
    }

    if (m_listWidget->count() > 0)
    {
        // this is needed to highlight the 1st row
        m_listWidget->setCurrentRow(0, QItemSelectionModel::Select);
        QListWidgetItem* item = m_listWidget->item(0);
        int layerID = item->data(Qt::UserRole).toInt();
        emit layerSelected(layerID);
    }
}

void LayerDock::onContextMenu(const QPoint& pos)
{
    QListWidgetItem* item = m_listWidget->itemAt(pos);
    if (!item) return;

    int layerID = item->data(Qt::UserRole).toInt();

    QMenu menu(this);
    QAction* renameAct = menu.addAction("Rename Layer");
    QAction* deleteAct = menu.addAction("Delete Layer");

    QAction* chosen = menu.exec(m_listWidget->viewport()->mapToGlobal(pos));
    if (!chosen) return;

    if (chosen == renameAct) {
        emit requestRenameLayer(layerID);   // You add this signal
    }
    else if (chosen == deleteAct) {
        emit requestDeleteLayer(layerID);   // You implement this
    }
}

