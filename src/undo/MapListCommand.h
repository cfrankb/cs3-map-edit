/*
    cs3-runtime-sdl
    Copyright (C) 2025 Francois Blanchette

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#include <QUndoCommand>
#include <vector>
#include <string>
#include "../runtime/map.h"
#include "../runtime/states.h"
#include "../mapfile.h"

// Records the per-map edits committed by DialogMaps when the user saves the
// map list. On redo it re-applies the edited title/states; on undo it
// restores the values that were present before the save. Mirrors the
// approach used by MapPropertiesCommand (snapshot whole title + CStates per map).
class MapListSaveCommand : public QUndoCommand
{
public:
    // One map's before/after snapshot.
    struct MapChange {
        CMap *map = nullptr;
        std::string oldTitle;
        CStates oldStates;
        std::string newTitle;
        CStates newStates;
    };

    explicit MapListSaveCommand(CMapFile *doc,
                                std::vector<MapChange> changes,
                                QUndoCommand *parent = nullptr)
        : QUndoCommand(QObject::tr("Save Map List"), parent)
        , m_doc(doc)
        , m_changes(std::move(changes))
    {
    }

    void undo() override
    {
        for (MapChange &c : m_changes)
            apply(c, c.oldTitle, c.oldStates);
        emitDirty();
    }

    void redo() override
    {
        for (MapChange &c : m_changes)
            apply(c, c.newTitle, c.newStates);
        emitDirty();
    }

private:
    void apply(MapChange &c, const std::string &title, const CStates &states)
    {
        if (!c.map)
            return;
        c.map->setTitle(title);
        c.map->states() = states;
    }

    void emitDirty()
    {
        if (!m_doc)
            return;
        m_doc->setDirty(true);
        Q_EMIT m_doc->dirtyChanged(true);
        Q_EMIT m_doc->refreshMap();
    }

    CMapFile *m_doc;
    std::vector<MapChange> m_changes;
};
