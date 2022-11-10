#include "mapfile.h"
#include "level.h"

CMapFile::CMapFile()
{
    m_map = CMap(64,64);
}

bool CMapFile::read()
{
    const std::string fname = filename().toLocal8Bit().toStdString();
    return fetchLevel(m_map, fname.c_str(), m_lastError);
}

bool CMapFile::write()
{
    const std::string fname = filename().toLocal8Bit().toStdString();
    return m_map.write(fname.c_str());
}

QString CMapFile::filename()
{
    return m_filename;
}

void CMapFile::setFilename(const QString filename)
{
    m_filename = filename;
}

QString CMapFile::lastError(){
    return m_map.lastError();
}

int CMapFile::size()  {
    return m_map.size();
}

CMap *CMapFile::map()
{
    return &m_map;
}

void CMapFile::setDirty(bool b)
{
    m_dirty = b;
}

bool CMapFile::isDirty(){
    return m_dirty;
}

bool CMapFile::isUntitled()
{
    return m_filename.isEmpty();
}
