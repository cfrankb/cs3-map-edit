#pragma once
#include <QUndoCommand>
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
            m_map = m_doc->removeAt(m_index);
        }
        int index = m_index;
        if (index == (int) m_doc->size()) {
            --index;
        }
        m_doc->setCurrentIndex(index);
        emit m_doc->dirtyChanged(true);
    }

    void undo() override
    {
        if (m_map)
        {
            m_doc->insertAt(m_index, std::move(m_map));
           // m_map.release(); // ownership transferred back
        }
        int index = m_index;
        m_doc->setCurrentIndex(index);
        emit m_doc->dirtyChanged(true);
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
        m_doc->insertAt(m_index, std::move(m_map));
       // m_map.release(); // ownership transferred
        m_doc->setCurrentIndex(m_index);
        emit m_doc->dirtyChanged(true);
    }

    void undo() override
    {
        m_map = m_doc->removeAt(m_index);
        m_doc->setCurrentIndex(m_index);
        emit m_doc->dirtyChanged(true);
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
        : QUndoCommand(QObject::tr("Move Map"), parent),
        m_doc(doc),
        m_from(fromIndex),
        m_to(toIndex)
    {}

    void redo() override
    {
        if (m_from == m_to)
            return;

        int from = m_from;
        int to   = m_to;

        // Remove at original from
        auto map = m_doc->removeAt(from);
        if (!map)
            return; // or assert, depending on your invariants

        // After removal, adjust the insertion index if needed
        if (from < to)
            --to;

        m_doc->insertAt(to, std::move(map));

        m_doc->setCurrentIndex(to);
        emit m_doc->dirtyChanged(true);
    }

    void undo() override
    {
        if (m_from == m_to)
            return;

        int from = m_from;
        int to   = m_to;

        // This time we remove at the index where we inserted in redo
        int removeIndex = to;
        if (from < to)
            --removeIndex;  // same adjustment as in redo

        auto map = m_doc->removeAt(removeIndex);
        if (!map)
            return;

        // Insert back at original from
        m_doc->insertAt(from, std::move(map));

        m_doc->setCurrentIndex(from);
        emit m_doc->dirtyChanged(true);
    }

private:
    CMapFile *m_doc;
    const int m_from;
    const int m_to;
};
