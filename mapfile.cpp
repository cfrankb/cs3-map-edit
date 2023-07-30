#include "mapfile.h"
#include "map.h"
#include <cstdio>

CMapFile::CMapFile():CMapArch()
{
    m_maps[0] = new CMap(64,64);
    m_size = 1;
    m_currIndex = 0;
}

CMapFile::~CMapFile()
{
}

bool CMapFile::read()
{
    m_currIndex = 0;
    return extract(filename().toLocal8Bit().toStdString().c_str());
}

bool CMapFile::write()
{
    bool result;
    const std::string fname = filename().toLocal8Bit().toStdString();
    if (isMulti())
    {
        result = CMapArch::write(fname.c_str());
    }
    else {
        // write single level
        result = m_maps[0]->write(fname.c_str());
        if (!result) {
            m_lastError = m_maps[0]->lastError();
        }
    }
    return result;
}

QString CMapFile::filename()
{
    return m_filename;
}

void CMapFile::setFilename(const QString filename)
{
    m_filename = filename;
}

CMap *CMapFile::map()
{
    return m_maps[m_currIndex];
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

void CMapFile::setCurrentIndex(int i)
{
    m_currIndex = i;
}

int CMapFile::currentIndex()
{
    return m_currIndex;
}

bool CMapFile::isMulti() {
    return m_size > 1 ;
}

bool CMapFile::isWrongExt()
{
    if (isMulti()) {
        return !m_filename.endsWith(".mapz");
    } else {
        return !m_filename.endsWith(".dat");
    }
}

void CMapFile::forget()
{
    CMapArch::forget();
    m_currIndex = 0;
}

CMap* CMapFile::removeAt(int i)
{
    CMap *map = CMapArch::removeAt(i);
    if (currentIndex() >= m_size) {
        setCurrentIndex(m_size -1);
    }
    return map;
}
