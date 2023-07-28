#include "mapfile.h"
#include "map.h"
#include "level.h"
#include <cstdio>

const char MAAZ_SIG[] = "MAAZ";
const uint16_t MAAZ_VERSION = 0;

CMapFile::CMapFile()
{
    m_max = 1;
    m_size = 1;
    m_maps = new CMap*[m_max];
    m_maps[0] = new CMap(64,64);
    m_currIndex = 0;
}

CMapFile::~CMapFile()
{
    forget();
}

bool CMapFile::read()
{
    const std::string fname = filename().toLocal8Bit().toStdString();
    FILE *sfile = fopen(fname.c_str(), "rb");
    if (!sfile) {
        m_lastError = "can't read header";
        return false;
    }
    char sig[4];
    fread(sig,4,1, sfile);
    fclose(sfile);
    m_currIndex = 0;

    if (memcmp(sig, MAAZ_SIG, 4) == 0) {
        qDebug("reading multi");
        // read levelArch
        typedef struct {
            uint8_t sig[4];
            uint16_t version;
            uint16_t count;
            uint32_t offset;
        } Header;

        FILE *sfile = fopen(fname.c_str(), "rb");
        if (sfile) {
            Header hdr;
            // read header
            fread(&hdr, 12, 1, sfile);
            // check version
            if (hdr.version != MAAZ_VERSION) {
                m_lastError = "MAAZ Version is incorrect";
                return false;
            }
            // read index
            fseek(sfile, hdr.offset, SEEK_SET);
            uint32_t *indexPtr = new uint32_t[hdr.count];
            fread(indexPtr, 4 * hdr.count,1, sfile);
            forget();
            m_maps = new CMap*[hdr.count];
            m_size = hdr.count;
            m_max = m_size;
            // read levels
            for (int i=0; i < hdr.count; ++i) {
                fseek(sfile, indexPtr[i], SEEK_SET);
                m_maps[i] = new CMap();
                m_maps[i]->read(sfile);
            }
            delete []indexPtr;
            fclose(sfile);
        } else {
            m_lastError = "can't read file";
        }
        return sfile != nullptr;
    } else {
        qDebug("reading single");
        // read single level
        m_size = 1;
        return fetchLevel(* m_maps[0], fname.c_str(), m_lastError);
    }
}

bool CMapFile::write()
{
    const std::string fname = filename().toLocal8Bit().toStdString();
    bool result;
    if (isMulti()) {
        // write levelArch
        FILE *tfile = fopen(fname.c_str(), "wb");
        if (tfile) {
            std::vector<long> index;
            // write temp header
            fwrite(MAAZ_SIG, 4,1, tfile);
            fwrite("\0\0\0\0",4,1,tfile);
            fwrite("\0\0\0\0",4,1,tfile);
            for (int i=0; i < m_size; ++i) {
                // write maps
                index.push_back(ftell(tfile));
                m_maps[i]->write(tfile);
            }
            // write index
            long indexPtr = ftell(tfile);
            size_t size = index.size();
            for (uint i=0; i < index.size(); ++i) {
                long ptr = index[i];
                fwrite(&ptr, 4, 1, tfile);
            }
            // write version
            fseek(tfile,4, SEEK_SET);
            fwrite(&MAAZ_VERSION, 2, 1, tfile);
            // write size
            fseek(tfile, 6, SEEK_SET);
            fwrite(&size, 2, 1, tfile);
            // write indexPtr
            fwrite(&indexPtr, 4, 1, tfile);
            fclose(tfile);
        } else {
            m_lastError = "can't write file";
        }
        result = tfile != nullptr;
    } else {
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

QString CMapFile::lastError(){
    return m_lastError.c_str();
}

int CMapFile::size()  {
    return m_size;
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

void CMapFile::forget()
{
    if (m_maps) {
        for (int i=0; i < m_size; ++i) {
            delete m_maps[i];
        }
        delete []m_maps;
        m_maps = nullptr;
    }
    m_size = 0;
    m_max = 0;
    m_currIndex = 0;
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

int CMapFile::add(CMap *map)
{
    allocSpace();
    m_maps[m_size] = map;
    return m_size ++;
}

CMap* CMapFile::removeAt(int i)
{
    CMap * map = m_maps[i];
    for (int j=i; j < m_size - 1; ++j ) {
        m_maps[j] = m_maps[j+1];
    }
    --m_size;

    if (currentIndex() >= m_size) {
        setCurrentIndex(m_size -1);
    }

    return map;
}

bool CMapFile::isWrongExt()
{
    if (isMulti()) {
        return !m_filename.endsWith(".mapz");
    } else {
        return !m_filename.endsWith(".dat");
    }
}

void CMapFile::insertAt(int i, CMap *map)
{
    allocSpace();
    for (int j=m_size; j > i; --j) {
        m_maps[j] = m_maps[j - 1];
    }
    ++m_size;
    m_maps[i] = map;
}

void CMapFile::allocSpace()
{
    if (m_size >= m_max) {
        m_max = m_size + GROW_BY;
        CMap ** t = new CMap*[m_max];
        for (int i=0; i < m_max; ++i) {
            t[i] = i < m_size ? m_maps[i] : nullptr;
        }
        delete[]m_maps;
        m_maps = t;
    }
}
