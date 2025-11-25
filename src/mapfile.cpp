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
    auto map = std::make_unique<CMap>(DEFAULT_MAP_SIZE, DEFAULT_MAP_SIZE);
    addMap(map);
}

CMapFile::~CMapFile() {
    for (auto* stack : m_mapStacks) {
        m_undoGroup->removeStack(stack);
        delete stack;
    }
}

bool CMapFile::read()
{
    m_currIndex = 0;
    bool ok = CMapArch::extract(filename().toStdString());
    if (!ok) return false;

    setCurrentIndex(0);

    // Reset undo machinery
    m_mapStacks.clear();
    m_docStack->clear();
    for (QUndoStack* stack : m_undoGroup->stacks()) {
        m_undoGroup->removeStack(stack);
    }
    m_undoGroup->addStack(m_docStack);

    // Create a stack for each map
    for (size_t i = 0; i < size(); ++i) {
        auto* stack = new QUndoStack(this);
        m_mapStacks.push_back(stack);
        m_undoGroup->addStack(stack);
    }

    // Set active stack to first map
    if (!m_mapStacks.empty()) {
        m_undoGroup->setActiveStack(m_mapStacks[m_currIndex]);
        //emit currentMapChanged(m_maps[m_currIndex].get());
    }

    setDirty(false);
    emit dirtyChanged(false);
    return true;
}

size_t CMapFile::addMap(std::unique_ptr<CMap>& map)
{
    // Reuse insertAt with index = end of collection
    insertAt(static_cast<int>(m_maps.size()), map);

    emit dirtyChanged(true);

    // Return the index of the newly added map
    return m_currIndex;
}

CMap* CMapFile::removeAt(int i)
{
    if (m_maps.size() <= 1)
        return nullptr; // enforce invariant

    if (i < 0 || i >= static_cast<int>(m_maps.size()))
        return nullptr;

    QUndoStack* stack = m_mapStacks[i];
    m_undoGroup->removeStack(stack);
    m_mapStacks.erase(m_mapStacks.begin() + i);
    delete stack;

    auto map = CMapArch::removeAt(i).release();

    setDirty(true);
    emit dirtyChanged(true);
    setCurrentIndex(std::min(i, static_cast<int>(m_maps.size() - 1)));

    return map;
}

void CMapFile::insertAt(int i, std::unique_ptr<CMap>& map)
{
    // Clamp index
    if (i < 0) i = 0;
    if (i > static_cast<int>(m_maps.size())) i = static_cast<int>(m_maps.size());

    // Insert into base collection
    CMapArch::insertAt(i, map);

    // Update current index
    setCurrentIndex(i);

    // Create a new undo stack for this map
    auto* stack = new QUndoStack(this);
    m_mapStacks.insert(m_mapStacks.begin() + i, stack);

    // Register stack with the undo group
    m_undoGroup->addStack(stack);

    // Switch active stack and emit signal
    m_undoGroup->setActiveStack(stack);
    //emit currentMapChanged(m_maps[m_currIndex].get());

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

void CMapFile::setCurrentIndex(int i)
{
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
