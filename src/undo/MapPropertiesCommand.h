#pragma once

#include <QUndoCommand>
#include "../runtime/map.h"
#include "../runtime/states.h"


class MapPropertiesCommand : public QUndoCommand {

public:
    enum class EditMode { StatesOnly, StatesAndTitle };

    MapPropertiesCommand(CMap* map,
                         QObject* notifier, // e.g. MainWindow
                         const CStates& newStates,
                         const std::string& newTitle,
                         EditMode mode)
        : m_map(map),
        m_notifier(notifier),
        m_newStates(newStates),
        m_newTitle(newTitle),
        m_mode(mode)
    {
        if (m_map) {
            m_oldStates = m_map->states();
            m_oldTitle  = m_map->title();
        }
        setText(m_mode == EditMode::StatesOnly
                    ? QObject::tr("Edit Map States")
                    : QObject::tr("Change Map Properties"));
    }

    void undo() override { apply(m_oldStates, m_oldTitle); }
    void redo() override { apply(m_newStates, m_newTitle); }

private:
    void apply(const CStates& states, const std::string& title) {
        if (!m_map) return;
         if (m_mode == EditMode::StatesAndTitle)
            m_map->setTitle(title);
        m_map->states() = states;

        if (m_notifier) {
            QMetaObject::invokeMethod(m_notifier, "propertiesChanged");
        }
    }

    CMap* m_map;
    QObject* m_notifier; // MainWindow or other central object
    CStates m_oldStates, m_newStates;
    std::string m_oldTitle, m_newTitle;
    EditMode m_mode;
};


