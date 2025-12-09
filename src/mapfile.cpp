#include "mapfile.h"
#include "runtime/map.h"
#include <QUndoStack>

constexpr const int DEFAULT_MAP_SIZE = 64;

CMapFile::CMapFile(QObject* parent)
    : QObject(parent),
    m_undoGroup(new QUndoGroup(this)),
    m_docStack(new QUndoStack(this)),
    m_currIndex(0),
    m_dirty(false)
{
    // Register document-level stack
    m_undoGroup->addStack(m_docStack);

    // Always start with one default map
    addMap(std::make_unique<CMap>(DEFAULT_MAP_SIZE, DEFAULT_MAP_SIZE));

    m_docStack->setUndoLimit(100); // keep only the last 100 commands
    // Implement QUndoCommand::mergeWith() to combine similar operations.
    setCurrentIndex(0);
    connect(this, &CMapFile::dirtyChanged, this, &CMapFile::setDirty);

    debug();
}

CMapFile::~CMapFile()
{
}


void CMapFile::resetUndoStacks()
{
    // Ensure docStack is registered once
    if (!m_undoGroup->stacks().contains(m_docStack)) {
        m_undoGroup->addStack(m_docStack);
    }

    // Switch active stack to docStack before removing others
    m_undoGroup->setActiveStack(m_docStack);

    // Remove all map stacks from the group
    for (QUndoStack* stack : m_mapStacks) {
        if (stack != m_docStack) {
            m_undoGroup->removeStack(stack);
        }
    }
    m_mapStacks.clear();

    // Clear document stack history
    m_docStack->clear();

    // At this point: group contains only docStack, active = docStack
}


void CMapFile::reset()
{
    debug();

    resetUndoStacks();   // clean undo machinery

    // Factory reset (document reset)
    setFilename("");
    forget();

    // Add one map by default
    addMap(std::make_unique<CMap>(40, 40));

    setDirty(false);
    emit dirtyChanged(false);

    // Switch active stack to the new map
    setCurrentIndex(0);

    debug();
}



bool CMapFile::read()
{
    m_currIndex = 0;
    bool ok = CMapArch::extract(filename().toStdString());
    if (!ok) {
        qDebug("failed");
        return false;
    }

    m_undoGroup->setActiveStack(m_docStack);

    // Reset undo machinery
    for (QUndoStack* stack : m_mapStacks) {
        if (stack != m_docStack)
            m_undoGroup->removeStack(stack);
        // no delete, parent will clean up
    }

    m_mapStacks.clear();
    m_docStack->clear();

    // Recreate stacks for each map
    for (size_t i = 0; i < size(); ++i) {
        auto* stack = new QUndoStack(this); // parent = CMapFile
        m_mapStacks.push_back(stack);
        m_undoGroup->addStack(stack);
    }

    setDirty(false);
    emit dirtyChanged(false);
    setCurrentIndex(0);
    qDebug("read()out");
    return true;
}

std::unique_ptr<CMap> CMapFile::removeAt(int i)
{
    if (m_maps.size() <= 1)
        return nullptr; // enforce invariant

    if (i < 0 || i >= static_cast<int>(m_maps.size()))
        return nullptr;

    QUndoStack* stack = m_mapStacks[i];
    m_undoGroup->setActiveStack(m_docStack);
    m_undoGroup->removeStack(stack);
    m_mapStacks.erase(m_mapStacks.begin() + i);

    setDirty(true);
    emit dirtyChanged(true);

    //  delete stack;
    return CMapArch::removeAt(i);
}


size_t CMapFile::addMap(std::unique_ptr<CMap> map)
{
    qDebug("addMap");

    // Always insert at the end
    insertAt(static_cast<int>(m_maps.size()), std::move(map));

    // Return the index of the newly added map
    return m_currIndex;
}

void CMapFile::insertAt(int i, std::unique_ptr<CMap> map)
{
    qDebug("insertMap %d", i);

    // Clamp index
    if (i < 0) i = 0;
    if (i > static_cast<int>(m_maps.size())) i = static_cast<int>(m_maps.size());

    // Insert into base collection
    CMapArch::insertAt(i,std::move(map));

    // Create and register undo stack for this map
    auto* stack = new QUndoStack(this);
    m_mapStacks.insert(m_mapStacks.begin() + i, stack);
    m_undoGroup->addStack(stack);

    // Update current index and active stack
    m_currIndex = i;
    m_undoGroup->setActiveStack(stack);
    emit currentMapChanged(m_maps[m_currIndex].get());

    emit dirtyChanged(true);
}


bool CMapFile::write()
{
    bool result;
    const std::string fname = filename().toStdString();
    if (isMulti())
    {
        result = CMapArch::write(fname);
    }
    else
    {
        // write single level
        result = m_maps[0]->write(fname.c_str());
        if (!result)
        {
            m_lastError = m_maps[0]->lastError();
        }
    }
    return result;
}

QString CMapFile::filename()
{
    return m_filename;
}

void CMapFile::setFilename(const QString& name) {
    if (m_filename != name) {
        m_filename = name;
        emit filenameChanged(m_filename);
    }
}

CMap *CMapFile::map()
{
    return m_maps[m_currIndex].get();
}

void CMapFile::setDirty(bool b)
{
    m_dirty = b;
}

bool CMapFile::isDirty()
{
    return m_dirty;
}

bool CMapFile::isUntitled()
{
    return m_filename.isEmpty();
}

void CMapFile::sync()
{
    setCurrentIndex(m_currIndex);
}


void CMapFile::setCurrentIndex(int i)
{
    LOGI("*** setCurrentIndex: %d", i);
    if (i >= 0 && static_cast<size_t>(i) < m_maps.size()) {
        m_currIndex = i;

        // Switch active undo stack too
        if (i < static_cast<int>(m_mapStacks.size())) {
            m_undoGroup->setActiveStack(m_mapStacks[i]);
        }

        emit currentMapChanged(m_maps[m_currIndex].get());
    } else {
        // Invalid index: reset to 0 or leave unchanged
        m_currIndex = 0;
        m_undoGroup->setActiveStack(m_docStack);
        emit currentMapChanged(nullptr);
    }
}

size_t CMapFile::currentIndex()
{
    return m_currIndex;
}

bool CMapFile::isMulti()
{
    return m_maps.size() > 1;
}

bool CMapFile::isWrongExt()
{
    if (isMulti())
    {
        return !m_filename.endsWith(".mapz");
    }
    else
    {
        return !m_filename.endsWith(".dat");
    }
}

void CMapFile::forget()
{
    CMapArch::clear();
    m_currIndex = 0;
}

void CMapFile::debug()
{
    qDebug("\n*************************************");
    qDebug("* maps: %lu", m_maps.size());
    qDebug("* m_mapStacks: %lu", m_mapStacks.size());
    qDebug("* m_undoGroup %lld", m_undoGroup->stacks().size());
    qDebug("* m_docStack %p", m_docStack);
    qDebug("* m_undoGroup.active() %p", m_undoGroup->activeStack());

    qDebug("* MapStack (trace)");
    int i = 0;
    for (QUndoStack* stack : m_mapStacks) {
        qDebug("* >>> %i %p ", i, stack);

        ++i;
        // no delete, parent will clean up
    }
    qDebug("*************************************\n");
}
