#pragma once
#include <QUndoCommand>
#include <queue>
#include "../runtime/map.h"
#include "../mapfile.h"
#include "../runtime/layer.h"

class DeleteMapCommand : public QUndoCommand
{
public:
    DeleteMapCommand(CMapFile *doc, int index, QUndoCommand *parent = nullptr)
        : QUndoCommand("Delete Map", parent),
          m_doc(doc), m_index(index), m_map(nullptr) {}

    void redo() override
    {
        // Enforce invariant: never delete last map
        if (m_doc->size() > 1)
        {
            m_map.reset(m_doc->removeAt(m_index));
        }
    }

    void undo() override
    {
        if (m_map)
        {
            m_doc->insertAt(m_index, m_map);
            m_map.release(); // ownership transferred back
        }
    }

private:
    CMapFile *m_doc;
    int m_index;
    std::unique_ptr<CMap> m_map;
};

class InsertMapCommand : public QUndoCommand
{
public:
    InsertMapCommand(CMapFile *doc, int index, std::unique_ptr<CMap> map,
                     QUndoCommand *parent = nullptr)
        : QUndoCommand("Insert Map", parent),
          m_doc(doc), m_index(index), m_map(std::move(map)) {}

    void redo() override
    {
        m_doc->insertAt(m_index, m_map);
        m_map.release(); // ownership transferred
    }

    void undo() override
    {
        m_map.reset(m_doc->removeAt(m_index));
    }

private:
    CMapFile *m_doc;
    int m_index;
    std::unique_ptr<CMap> m_map;
};

// AddMapCommand is just InsertMapCommand with index = end

class MoveMapCommand : public QUndoCommand
{
public:
    MoveMapCommand(CMapFile *doc, int fromIndex, int toIndex,
                   QUndoCommand *parent = nullptr)
        : QUndoCommand("Move Map", parent),
          m_doc(doc), m_from(fromIndex), m_to(toIndex) {}

    void redo() override
    {
        if (m_from == m_to)
            return;
        auto map = std::unique_ptr<CMap>(m_doc->removeAt(m_from));
        m_doc->insertAt(m_to, map);
        std::swap(m_from, m_to); // prepare for undo
    }

    void undo() override
    {
        redo(); // swap back
    }

private:
    CMapFile *m_doc;
    int m_from;
    int m_to;
};
