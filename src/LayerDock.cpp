#include "LayerDock.h"
#include <QString>
#include <QVBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QMenu>
#include <QLineEdit>
#include <QInputDialog>
#include <QMessageBox>
#include <QUndoStack>
#include <QUndoCommand>
#include <cstdint>
#include "runtime/map.h"
#include "LayerRowWidget.h"
#include "stamp.h"

class LayerRenameCommand : public QUndoCommand
{
public:
    LayerRenameCommand(CLayer *layer, const uint16_t layerID, const QString &newName, LayerDock *dock)
        : m_layer(layer), m_layerID(layerID), m_newName(newName), m_dock(dock)
    {
        m_oldName = layer->getName();
        setText(QObject::tr("Rename Layer"));
    }

    void undo() override
    {
        m_layer->setName(m_oldName.toStdString());
        emit m_dock->layerUpdated(m_layerID);
    }

    void redo() override
    {
        m_layer->setName(m_newName.toStdString());
        emit m_dock->layerUpdated(m_layerID);
    }

private:
    CLayer *m_layer;
    uint16_t m_layerID;
    QString m_oldName;
    QString m_newName;
    LayerDock *m_dock;
};

class LayerChangeBaseIDCommand : public QUndoCommand
{
public:
    LayerChangeBaseIDCommand(CLayer *layer, const uint16_t layerID, uint16_t newBaseID, LayerDock *dock)
        : m_layer(layer), m_layerID(layerID), m_newBaseID(newBaseID), m_dock(dock)
    {
        m_oldBaseID = layer->baseID();
        setText(QObject::tr("Change Layer Base ID"));
    }

    void undo() override
    {
        m_layer->setBaseID(m_oldBaseID);
        emit m_dock->layerUpdated(m_layerID);
    }

    void redo() override
    {
        m_layer->setBaseID(m_newBaseID);
        emit m_dock->layerUpdated(m_layerID);
    }

private:
    CLayer *m_layer;
    uint16_t m_layerID;
    int m_oldBaseID;
    int m_newBaseID;
    LayerDock *m_dock;
};

class AddExtraLayersCommand : public QUndoCommand
{
public:
    AddExtraLayersCommand(CMap *map, LayerDock *dock = nullptr)
        : m_map(map), m_dock(dock)
    {
        setText(QObject::tr("Add Walls and Floor Layers"));
    }

    void undo() override
    {
        // Pop and save the layers we added
        if (m_map->layerCount() >= 2)
        {
            m_floorLayer = m_map->popLayer(); // returns unique_ptr<CLayer>
            m_wallsLayer = m_map->popLayer();
        }
        refresh();
    }

    void redo() override
    {
        if (m_wallsLayer && m_floorLayer)
        {
            // Restore the original layers
            m_map->pushLayer(m_wallsLayer);
            m_map->pushLayer(m_floorLayer);
        }
        else
        {
            // First-time redo: create new layers
            m_map->addLayer(CLayer::LAYER_WALLS, "walls", Stamp::OtherTilesetBaseID);
            m_map->addLayer(CLayer::LAYER_FLOOR, "floor", Stamp::OtherTilesetBaseID);
        }
        refresh();
    }

private:
    void refresh()
    {
        if (m_dock)
        {
            m_dock->refreshList(m_map);
            QMetaObject::invokeMethod(m_dock, "layersChanged");
        }
    }

    CMap *m_map;
    LayerDock *m_dock;

    // Saved layers for undo/redo
    std::unique_ptr<CLayer> m_wallsLayer;
    std::unique_ptr<CLayer> m_floorLayer;
};

class InsertLayerCommand : public QUndoCommand
{
public:
    InsertLayerCommand(CMap *map, const std::string &name, uint16_t baseID,
                       CLayer::LayerType layerType, LayerDock *dock)
        : m_map(map), m_name(name), m_baseID(baseID), m_type(layerType), m_dock(dock)
    {
        setText(QObject::tr("Add Layer"));
    }

    void undo() override
    {
        // Remove the layer we inserted, keeping it alive inside the command
        m_layer = std::move(m_map->layers()[m_index]);
        m_map->layers().erase(m_map->layers().begin() + m_index);
        refresh();
    }

    void redo() override
    {
        // (Re)insert the layer at its position; create it on the first redo
        if (!m_layer)
            m_layer.reset(new CLayer(m_map->width(), m_map->height(), m_type, m_baseID, m_name.c_str()));
        m_map->layers().insert(m_map->layers().begin() + m_index, std::move(m_layer));
        refresh();
    }

private:
    void refresh()
    {
        if (m_dock)
        {
            m_dock->refreshList(m_map);
            QMetaObject::invokeMethod(m_dock, "layersChanged");
        }
    }

    static constexpr size_t m_index = 1; // insert right after the main layer

    CMap *m_map;
    std::string m_name;
    uint16_t m_baseID;
    CLayer::LayerType m_type;
    LayerDock *m_dock;
    std::unique_ptr<CLayer> m_layer;
};

class DeleteLayerCommand : public QUndoCommand
{
public:
    DeleteLayerCommand(CMap *map, int layerID, LayerDock *dock)
        : m_map(map), m_layerID(layerID), m_dock(dock)
    {
        setText(QObject::tr("Delete Layer"));
    }

    void undo() override
    {
        // Re-insert the layer we removed
        if (m_layer)
            m_map->layers().insert(m_map->layers().begin() + m_layerID, std::move(m_layer));
        refresh();
    }

    void redo() override
    {
        // Remove the layer and keep it alive inside the command
        if (m_layerID < m_map->layerCount())
        {
            m_layer = std::move(m_map->layers()[m_layerID]);
            m_map->layers().erase(m_map->layers().begin() + m_layerID);
        }
        refresh();
    }

private:
    void refresh()
    {
        if (m_dock)
        {
            m_dock->refreshList(m_map);
            QMetaObject::invokeMethod(m_dock, "layersChanged");
        }
    }

    CMap *m_map;
    int m_layerID;
    LayerDock *m_dock;
    std::unique_ptr<CLayer> m_layer;
};

LayerDock::LayerDock(CMap *map, QUndoStack *stack, QWidget *parent)
    : QDockWidget("Layers", parent),
      m_map(map), m_undoStack(stack)
{
    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);

    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::NoSelection);
    m_listWidget->setStyleSheet(
        "QListWidget::item:selected { "
        "    background: rgba(0, 120, 215, 160); " /* Bright blue, semi-transparent */
        "    color: white; "
        "} "
        "QListWidget::item:selected:!active { "
        "    background: rgba(0, 120, 215, 160); " /* Same color when unfocused */
        "    color: white; "
        "} "
        "QListWidget::item { "
        "    padding: 3px; "
        "} ");

    m_eyeOpen = QIcon(":/data/icons/eye_818577.png");    // freepik
    m_eyeClosed = QIcon(":data/icons/blind_795831.png"); // freepik

    mainLayout->addWidget(m_listWidget);

    auto *buttonLayout = new QHBoxLayout;
    m_addButton3Layers = new QPushButton(QIcon(":/data/icons/small_chemistry.png"), "Add Layers");
    m_addButton3Layers->setToolTip("Add standard Floor and Walls layers");
    m_addButton1Layer = new QPushButton(QIcon(":/data/icons/plus.png"), "Add Layer");
    m_addButton1Layer->setToolTip("Add additional Decor layer");

    buttonLayout->addWidget(m_addButton3Layers);
    buttonLayout->addWidget(m_addButton1Layer);
    buttonLayout->addStretch(); // push buttons to the left
    mainLayout->addLayout(buttonLayout);

    setWidget(central);
    refreshList(map);

    connect(m_addButton3Layers, &QPushButton::clicked, this, [this]()
            { m_undoStack->push(new AddExtraLayersCommand(m_map, this)); });

    connect(m_addButton1Layer, &QPushButton::clicked, this, [this]()
            {
                // TODO: prompt for the name / baseID (e.g. QInputDialog) instead of placeholders
                m_undoStack->push(new InsertLayerCommand(m_map, "new layer", Stamp::OtherTilesetBaseID, CLayer::LAYER_DECOR, this)); });

    m_listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_listWidget, &QListWidget::customContextMenuRequested,
            this, &LayerDock::onContextMenu);

    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_listWidget->setFocusPolicy(Qt::StrongFocus);

    connect(m_listWidget, &QListWidget::itemSelectionChanged,
            this, &LayerDock::updateRowHighlights);

    connect(this, &LayerDock::requestRenameLayer, this, &LayerDock::renameLayer);
    connect(this, &LayerDock::requestDeleteLayer, this, &LayerDock::deleteLayer);
    connect(this, &LayerDock::requestChangeBaseID, this, &LayerDock::changeBaseID);
    connect(this, &LayerDock::layerUpdated, this, &LayerDock::updateRow);
}

void LayerDock::updateRowHighlights()
{
    for (int i = 0; i < m_listWidget->count(); ++i)
    {
        QListWidgetItem *item = m_listWidget->item(i);

        if (item->isSelected())
        {
            item->setBackground(QColor(60, 120, 200)); // selected color
            item->setForeground(Qt::white);            // selected text color
        }
        else
        {
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

    if (!m_map)
        return;
    if (m_addButton3Layers)
        m_addButton3Layers->setEnabled(map->layerCount() == 1);

    if (m_addButton1Layer)
        m_addButton1Layer->setEnabled(map->layerCount() >= 3);

    auto addRow = [&](int layerID, const QString &text)
    {
        QListWidgetItem *item = new QListWidgetItem(m_listWidget);
        item->setData(Qt::UserRole, layerID);
        item->setSizeHint(QSize(0, 26));

        bool visible = true;
        m_visibility[layerID] = visible;

        LayerRowWidget *row = new LayerRowWidget(text, visible);
        m_listWidget->setItemWidget(item, row);

        connect(row, &LayerRowWidget::eyeClicked, this, [this, layerID, item]()
                {
            bool v = !m_visibility[layerID];
            m_visibility[layerID] = v;

            auto* r = qobject_cast<LayerRowWidget*>(m_listWidget->itemWidget(item));
            r->findChild<QToolButton*>()->setIcon(
                v ? m_eyeOpen : m_eyeClosed
                );

            emit visibilityChanged(layerID, v); });

        connect(row, &LayerRowWidget::rowClicked, this, [this, layerID]()
                { emit layerSelected(layerID); });
    };

    // ---  layers
    int i = 0;
    for (auto &layer : m_map->layers())
    {
        if (!layer)
            continue;

        addRow(i, getLayerText(layer.get()));
        ++i;
    }

    if (m_listWidget->count() > 0)
    {
        // this is needed to highlight the 1st row
        m_listWidget->setCurrentRow(0, QItemSelectionModel::Select);
        QListWidgetItem *item = m_listWidget->item(0);
        int layerID = item->data(Qt::UserRole).toInt();
        emit layerSelected(layerID);
    }
}

void LayerDock::onContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_listWidget->itemAt(pos);
    if (!item)
        return;

    int layerID = item->data(Qt::UserRole).toInt();
    QMenu menu(this);
    QAction *renameAct = menu.addAction("Rename Layer");
    QAction *changeBaseIDAct = menu.addAction("Change BaseID");
    QAction *deleteAct = nullptr;
    if (layerID != 0)
        deleteAct = menu.addAction("Delete Layer");

    QAction *chosen = menu.exec(m_listWidget->viewport()->mapToGlobal(pos));
    if (!chosen)
        return;

    if (chosen == renameAct)
    {
        emit requestRenameLayer(layerID);
    }
    else if (chosen == changeBaseIDAct)
    {
        emit requestChangeBaseID(layerID);
    }
    else if (chosen == deleteAct)
    {
        emit requestDeleteLayer(layerID);
    }
}

QString LayerDock::getLayerText(CLayer *layer)
{
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
    return QString("%1 (%2) [baseID: 0x%3]").arg(layer->getName()).arg(typeStr).arg(layer->baseID(), 4, 16, QChar('0'));
}

void LayerDock::renameLayer(int layerID)
{
    CLayer *layer = m_map->getLayer(layerID);
    if (!layer)
    {
        LOGE("failed to query layer");
        return;
    }

    bool ok;
    QString newName = QInputDialog::getText(this, tr("Rename Layer"),
                                            tr("Name:"), QLineEdit::Normal,
                                            layer->getName(), &ok);
    newName = newName.trimmed().mid(0, 254);
    if (ok && strcmp(newName.toLatin1(), layer->getName()) != 0)
    {
        m_undoStack->push(new LayerRenameCommand(layer, layerID, newName, this));
        //  layer->setName(newName.toLatin1().constData());
        //  emit layerUpdated(layerID);
    }
}

void LayerDock::changeBaseID(int layerID)
{
    auto parseUInt16 = [](const QString &text, uint16_t &outValue)
    {
        QString t = text.trimmed();

        bool ok = false;
        uint32_t v = 0;

        if (t.startsWith("0x", Qt::CaseInsensitive))
        {
            // hex input
            v = t.mid(2).toUInt(&ok, 16);
        }
        else
        {
            // decimal input
            v = t.toUInt(&ok, 10);
        }

        if (!ok)
            return false;
        if (v > 0xFFFF)
            return false;

        outValue = static_cast<uint16_t>(v);
        return true;
    };

    auto toHexString = [](uint16_t value) -> QString
    {
        return QString("0x%1").arg(value, 4, 16, QChar('0'));
    };

    CLayer *layer = m_map->getLayer(layerID);
    if (!layer)
    {
        LOGE("failed to query layer");
        return;
    }

    bool ok;
    QString s = QInputDialog::getText(
        this,
        "Enter Value",
        "Enter a 16-bit value (decimal or hex with 0x):",
        QLineEdit::Normal,
        toHexString(layer->baseID()),
        &ok);

    if (!ok || s.isEmpty())
        return; // user cancelled

    uint16_t newValue;
    if (!parseUInt16(s, newValue))
    {
        QMessageBox::warning(this, "Invalid Input",
                             "Please enter a decimal number or hex number with 0x prefix.");
        return;
    }

    if (newValue != layer->baseID())
    {
        m_undoStack->push(new LayerChangeBaseIDCommand(layer, layerID, newValue, this));
        //    layer->setBaseID(newValue);
        //   emit layerUpdated(layerID);
    }
}

void LayerDock::deleteLayer(int layerID)
{
    if (layerID == 0) // the main layer cannot be deleted
        return;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,                                 // Parent window
        "Delete Selected Layer",              // Dialog Title
        "Are you sure you want to proceed?",  // Dialog Message text
        QMessageBox::Ok | QMessageBox::Cancel // Buttons to display
    );

    // Handle the user's choice
    if (reply != QMessageBox::Ok)
    {
        // Insert your fallback/abort logic here
        qDebug() << "Action Cancelled.";
        return;
    }

    if (!m_map->getLayer(layerID))
    {
        LOGE("failed to query layer");
        return;
    }

    m_undoStack->push(new DeleteLayerCommand(m_map, layerID, this));
}

void LayerDock::updateRow(int layerID)
{
    QListWidgetItem *item = m_listWidget->item(layerID); // layerID = row
    if (!item)
    {
        LOGE("can't get currentItem()");
        return;
    }
    LayerRowWidget *itemWidget = qobject_cast<LayerRowWidget *>(m_listWidget->itemWidget(item));
    if (!itemWidget)
    {
        LOGE("can't get itemWidget()");
        return;
    }
    CLayer *layer = m_map->getLayer(layerID);
    itemWidget->setLabel(getLayerText(layer));
}

void LayerDock::newUndoStack(QUndoStack *stack)
{
    m_undoStack = stack;
}
