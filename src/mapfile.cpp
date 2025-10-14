#include "mapfile.h"
#include "runtime/map.h"

CMapFile::CMapFile() : CMapArch()
{
    std::unique_ptr<CMap> map = std::make_unique<CMap>(64,64);
    //m_doc.add(std::move(map));
    //m_maps[0] = new CMap(64, 64);
    add(std::move(map));
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

void CMapFile::setFilename(const QString filename)
{
    m_filename = filename;
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
    m_currIndex = i;
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

CMap *CMapFile::removeAt(int i)
{
    CMap *map = CMapArch::removeAt(i).release();
    if (currentIndex() >= m_maps.size())
    {
        setCurrentIndex(m_maps.size() - 1);
    }
    return map;
}

