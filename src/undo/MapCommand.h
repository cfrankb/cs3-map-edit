#pragma once
#include <QUndoCommand>
#include <queue>
#include "../runtime/map.h"
#include "../mapfile.h"
#include "../runtime/layer.h"

class DeleteMapCommand : public QUndoCommand
{
public:
    DeleteMapCommand(CMapFile* doc, int index, QUndoCommand* parent = nullptr)
        : QUndoCommand("Delete Map", parent),
        m_doc(doc), m_index(index), m_map(nullptr) {}

    void redo() override {
        // Enforce invariant: never delete last map
        if (m_doc->size() > 1) {
            m_map.reset(m_doc->removeAt(m_index));
        }
    }

    void undo() override {
        if (m_map) {
            m_doc->insertAt(m_index, m_map);
            m_map.release(); // ownership transferred back
        }
    }

private:
    CMapFile* m_doc;
    int m_index;
    std::unique_ptr<CMap> m_map;
};


class InsertMapCommand : public QUndoCommand
{
public:
    InsertMapCommand(CMapFile* doc, int index, std::unique_ptr<CMap> map,
                     QUndoCommand* parent = nullptr)
        : QUndoCommand("Insert Map", parent),
        m_doc(doc), m_index(index), m_map(std::move(map)) {}

    void redo() override {
        m_doc->insertAt(m_index, m_map);
        m_map.release(); // ownership transferred
    }

    void undo() override {
        m_map.reset(m_doc->removeAt(m_index));
    }

private:
    CMapFile* m_doc;
    int m_index;
    std::unique_ptr<CMap> m_map;
};

// AddMapCommand is just InsertMapCommand with index = end


class MoveMapCommand : public QUndoCommand
{
public:
    MoveMapCommand(CMapFile* doc, int fromIndex, int toIndex,
                   QUndoCommand* parent = nullptr)
        : QUndoCommand("Move Map", parent),
        m_doc(doc), m_from(fromIndex), m_to(toIndex) {}

    void redo() override {
        if (m_from == m_to) return;
        auto map = std::unique_ptr<CMap>(m_doc->removeAt(m_from));
        m_doc->insertAt(m_to, map);
        std::swap(m_from, m_to); // prepare for undo
    }

    void undo() override {
        redo(); // swap back
    }

private:
    CMapFile* m_doc;
    int m_from;
    int m_to;
};


class ShiftMapCommand : public QUndoCommand
{
public:
    ShiftMapCommand(CMapFile* doc, Direction dir, QUndoCommand* parent = nullptr)
        : QUndoCommand(parent), m_doc(doc), m_dir(dir)
    {
        setText(commandText(dir));
    }

    void redo() override {
        m_doc->map()->shift(m_dir);
        m_doc->setDirty(true);
        emit m_doc->dirtyChanged(true);
        emit m_doc->refreshMap();
    }

    void undo() override {
        m_doc->map()->shift(opposite(m_dir));
        m_doc->setDirty(true);
        emit m_doc->dirtyChanged(true);
        emit m_doc->refreshMap();
    }

private:
    CMapFile* m_doc;
    Direction m_dir;

    static Direction opposite(Direction d) {
        switch (d) {
        case Direction::UP:    return Direction::DOWN;
        case Direction::DOWN:  return Direction::UP;
        case Direction::LEFT:  return Direction::RIGHT;
        case Direction::RIGHT: return Direction::LEFT;
        }
        return d; // fallback
    }

    static QString commandText(Direction d) {
        switch (d) {
        case Direction::UP:    return QObject::tr("Shift Up");
        case Direction::DOWN:  return QObject::tr("Shift Down");
        case Direction::LEFT:  return QObject::tr("Shift Left");
        case Direction::RIGHT: return QObject::tr("Shift Right");
        }
        return QObject::tr("Shift");
    }
};


class ClearMapCommand : public QUndoCommand
{
public:
    ClearMapCommand(CMapFile* doc, QUndoCommand* parent = nullptr)
        : QUndoCommand(parent), m_doc(doc)
    {
        setText(QObject::tr("Clear Map"));

        // Capture the current state of the main layer
        CLayer* layer = m_doc->map()->getMainLayer();
        m_backup = layer->tiles(); // you need a method to serialize/clone the layer contents
    }

    void redo() override {
        m_doc->map()->getMainLayer()->clear();
        emit m_doc->dirtyChanged(true);
        m_doc->setDirty(true);
        emit m_doc->refreshMap();
        emit m_doc->dirtyChanged(true);
    }

    void undo() override {
        CLayer* layer = m_doc->map()->getMainLayer();
        layer->tilesFrom(m_backup); // restore from backup
        emit m_doc->dirtyChanged(true);
        m_doc->setDirty(true);
        emit m_doc->refreshMap();
        emit m_doc->dirtyChanged(true);
    }

private:
    CMapFile* m_doc;
    std::vector<uint8_t> m_backup; // or any container representing the saved state
};


class FloodFillCommand : public QUndoCommand
{
public:
    FloodFillCommand(CMapFile* doc, int startX, int startY, uint8_t newValue, QUndoCommand* parent = nullptr)
        : QUndoCommand(parent), m_doc(doc), m_startX(startX), m_startY(startY), m_newValue(newValue)
    {
        setText(QObject::tr("Flood Fill"));

        // Snapshot the layer before fill
        CLayer* layer = m_doc->map()->getMainLayer();
        m_backup = layer->tiles();
    }

    void redo() override {
        CLayer* layer = m_doc->map()->getMainLayer();
        floodFill(layer, m_startX, m_startY, m_newValue);
        m_doc->setDirty(true);
        emit m_doc->refreshMap();
        emit m_doc->dirtyChanged(true);
    }

    void undo() override {
        CLayer* layer = m_doc->map()->getMainLayer();
        layer->tilesFrom(m_backup);
        m_doc->setDirty(true);
        emit m_doc->refreshMap();
        emit m_doc->dirtyChanged(true);
    }

private:
    CMapFile* m_doc;
    int m_startX, m_startY;
    uint8_t m_newValue;
    std::vector<uint8_t> m_backup;

    void floodFill(CLayer* layer, int x, int y, uint8_t newValue) {
        // Basic BFS flood fill
        int width = layer->width();
        int height = layer->height();
        auto& tiles = layer->tiles();

        uint8_t oldValue = tiles[y * width + x];
        if (oldValue == newValue) return;

        std::queue<std::pair<int,int>> q;
        q.push({x,y});

        while (!q.empty()) {
            auto [cx, cy] = q.front();
            q.pop();

            int idx = cy * width + cx;
            if (tiles[idx] != oldValue) continue;

            tiles[idx] = newValue;

            if (cx > 0) q.push({cx-1, cy});
            if (cx < width-1) q.push({cx+1, cy});
            if (cy > 0) q.push({cx, cy-1});
            if (cy < height-1) q.push({cx, cy+1});
        }
    }
};


// Custom command for renaming a map
class RenameMapCommand : public QUndoCommand
{
public:
    RenameMapCommand(CMapFile* doc, CMap* map, const QString& newTitle, QUndoCommand* parent = nullptr)
        : QUndoCommand(parent),
        m_doc(doc),
        m_map(map),
        m_newTitle(newTitle.toStdString()),
        m_oldTitle(map->title())
    {
        setText(QObject::tr("Rename Map"));
    }

    void undo() override {
        m_map->setTitle(m_oldTitle);
        m_doc->setDirty(true);
        emit m_doc->dirtyChanged(true);
    }

    void redo() override {
        m_map->setTitle(m_newTitle);
        m_doc->setDirty(true);
        emit m_doc->dirtyChanged(true);
    }

private:
    CMapFile* m_doc;
    CMap* m_map;
    std::string m_newTitle;
    std::string m_oldTitle;
};


class ResizeMapCommand : public QUndoCommand
{
public:
    ResizeMapCommand(CMapFile *doc,
                     CMap* map,
                     int newWidth,
                     int newHeight,
                     QUndoCommand* parent = nullptr)
        : QUndoCommand(parent),
        m_doc(doc),
        m_map(map),
        m_newWidth(newWidth),
        m_newHeight(newHeight),
        m_oldWidth(map->len()),
        m_oldHeight(map->hei())
    {
        setText(QObject::tr("Resize Map"));

        // Backup old layer content
        backupLayers(m_oldLayers);
    }

    void undo() override {
        restoreLayers(m_oldLayers, m_oldWidth, m_oldHeight);
    }

    void redo() override {
        // Backup new layer content on first redo
        if (m_newLayers.empty()) {
            backupLayers(m_newLayers);
        }
        restoreLayers(m_newLayers, m_newWidth, m_newHeight);
    }

private:
    CMapFile* m_doc;
    CMap* m_map;
    int m_newWidth, m_newHeight;
    int m_oldWidth, m_oldHeight;

    std::vector<std::vector<uint8_t>> m_oldLayers;
    std::vector<std::vector<uint8_t>> m_newLayers;

    void backupLayers(std::vector<std::vector<uint8_t>>& out) {
        out.clear();
        for (const auto& layer : m_map->layers()) {
            out.push_back(layer->tiles()); // assuming layer.data() returns vector<char>
        }
    }

    void restoreLayers(const std::vector<std::vector<uint8_t>>& in, int w, int h) {
        m_map->resize(w, h, '\0', false);
        for (size_t i = 0; i < in.size(); ++i) {
            m_map->getLayer(i)->tilesFrom(in[i]); // assuming setData(vector<char>)
        }
        emit m_doc->resizeMap(m_map->len(), m_map->hei());
    }
};

struct TileChange {
    int x, y;
    uint8_t oldTile;
    uint8_t newTile;
};

class PaintStampCommand : public QUndoCommand {
public:
    PaintStampCommand(CMapFile *doc, CLayer* layer, const QString& label = QObject::tr("Paint Stamp"))
        : m_doc(doc),  m_layer(layer) {
        setText(label);
    }

    void addChange(int x, int y, uint8_t oldTile, uint8_t newTile) {
        m_changes.push_back({x, y, oldTile, newTile});
    }

    void undo() override {
        for (const auto& c : m_changes) {
            m_layer->set(c.x, c.y, c.oldTile);
        }
        emitModified();
    }

    void redo() override {
        for (const auto& c : m_changes) {
            m_layer->set(c.x, c.y, c.newTile);
        }
        emitModified();
    }

private:
    void emitModified() {
        // Notify UI
        emit m_doc->dirtyChanged(true);
        emit m_doc->refreshMap();
    }

    CMapFile *m_doc;
    CLayer* m_layer;
    std::vector<TileChange> m_changes;
};

