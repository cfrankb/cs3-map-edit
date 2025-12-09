#pragma once
#include <QUndoCommand>
#include <queue>
#include "../runtime/map.h"
#include "../mapfile.h"
#include "../runtime/layer.h"

class ShiftMapCommand : public QUndoCommand
{
public:
    ShiftMapCommand(CMapFile *doc, Direction dir, QUndoCommand *parent = nullptr)
        : QUndoCommand(parent), m_doc(doc), m_dir(dir)
    {
        setText(commandText(dir));
    }

    void redo() override
    {
        m_doc->map()->shift(m_dir);
        m_doc->setDirty(true);
        emit m_doc->dirtyChanged(true);
        emit m_doc->refreshMap();
    }

    void undo() override
    {
        m_doc->map()->shift(opposite(m_dir));
        m_doc->setDirty(true);
        emit m_doc->dirtyChanged(true);
        emit m_doc->refreshMap();
    }

private:
    CMapFile *m_doc;
    Direction m_dir;

    static Direction opposite(Direction d)
    {
        switch (d)
        {
        case Direction::UP:
            return Direction::DOWN;
        case Direction::DOWN:
            return Direction::UP;
        case Direction::LEFT:
            return Direction::RIGHT;
        case Direction::RIGHT:
            return Direction::LEFT;
        }
        return d; // fallback
    }

    static QString commandText(Direction d)
    {
        switch (d)
        {
        case Direction::UP:
            return QObject::tr("Shift Up");
        case Direction::DOWN:
            return QObject::tr("Shift Down");
        case Direction::LEFT:
            return QObject::tr("Shift Left");
        case Direction::RIGHT:
            return QObject::tr("Shift Right");
        }
        return QObject::tr("Shift");
    }
};

class ClearMapCommand : public QUndoCommand
{
public:
    ClearMapCommand(CMapFile *doc, QUndoCommand *parent = nullptr)
        : QUndoCommand(parent), m_doc(doc)
    {
        setText(QObject::tr("Clear Map"));

        // Capture the current state of the main layer
        CLayer *layer = m_doc->map()->getMainLayer();
        m_backup = layer->tiles(); // you need a method to serialize/clone the layer contents
    }

    void redo() override
    {
        m_doc->map()->getMainLayer()->clear();
        emit m_doc->dirtyChanged(true);
        m_doc->setDirty(true);
        emit m_doc->refreshMap();
        emit m_doc->dirtyChanged(true);
    }

    void undo() override
    {
        CLayer *layer = m_doc->map()->getMainLayer();
        layer->tilesFrom(m_backup); // restore from backup
        emit m_doc->dirtyChanged(true);
        m_doc->setDirty(true);
        emit m_doc->refreshMap();
        emit m_doc->dirtyChanged(true);
    }

private:
    CMapFile *m_doc;
    std::vector<uint8_t> m_backup; // or any container representing the saved state
};

// Custom command for renaming a map
class RenameMapCommand : public QUndoCommand
{
public:
    RenameMapCommand(CMapFile *doc, CMap *map, const QString &newTitle, QUndoCommand *parent = nullptr)
        : QUndoCommand(parent),
          m_doc(doc),
          m_map(map),
          m_newTitle(newTitle.toStdString()),
          m_oldTitle(map->title())
    {
        setText(QObject::tr("Rename Map"));
    }

    void undo() override
    {
        m_map->setTitle(m_oldTitle);
      //  m_doc->setDirty(true);
        emit m_doc->dirtyChanged(true);
    }

    void redo() override
    {
        m_map->setTitle(m_newTitle);
      //  m_doc->setDirty(true);
        emit m_doc->dirtyChanged(true);
    }

private:
    CMapFile *m_doc;
    CMap *m_map;
    std::string m_newTitle;
    std::string m_oldTitle;
};

class ResizeMapCommand : public QUndoCommand
{
public:
    ResizeMapCommand(CMapFile *doc,
                     CMap *map,
                     int newWidth,
                     int newHeight,
                     QUndoCommand *parent = nullptr)
        : QUndoCommand(parent),
          m_doc(doc),
          m_map(map),
          m_newWidth(newWidth),
          m_newHeight(newHeight),
          m_oldWidth(map->width()),
          m_oldHeight(map->height())
    {
        setText(QObject::tr("Resize Map"));

        // Backup old layer content
        backupLayers(m_oldLayers);
    }

    void undo() override
    {
        // restore old size and old data
        m_map->resize(m_oldWidth, m_oldHeight, '\0', false);
        for (size_t i = 0; i < m_oldLayers.size(); ++i)
            m_map->getLayer(i)->tilesFrom(m_oldLayers[i]);
        emit m_doc->resizeMap(m_oldWidth, m_oldHeight);
    }

    void redo() override
    {
        if (m_newLayers.empty()) {
            // we are being executed the first time: take snapshot AFTER resize
            // 1) start from old state (already current), resize to new size
            m_map->resize(m_newWidth, m_newHeight, '\0', false);
            // 2) fill/adjust layers as your normal resize code would do
            //    (e.g. clear new cells, crop old ones, etc.)
            // 3) then snapshot that as "new" state:
            backupLayers(m_newLayers);
        } else {
            // subsequent redo: just restore new snapshot and size
            m_map->resize(m_newWidth, m_newHeight, '\0', false);
            for (size_t i = 0; i < m_newLayers.size(); ++i)
                m_map->getLayer(i)->tilesFrom(m_newLayers[i]);
        }
        emit m_doc->resizeMap(m_newWidth, m_newHeight);
    }

private:
    CMapFile *m_doc;
    CMap *m_map;
    int m_newWidth, m_newHeight;
    int m_oldWidth, m_oldHeight;

    std::vector<std::vector<uint8_t>> m_oldLayers;
    std::vector<std::vector<uint8_t>> m_newLayers;

    void backupLayers(std::vector<std::vector<uint8_t>> &out)
    {
        out.clear();
        for (const auto &layer : m_map->layers())
        {
            out.push_back(layer->tiles()); // assuming layer.data() returns vector<char>
        }
    }
};
