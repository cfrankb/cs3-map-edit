#ifndef CMAPFILE_H
#define CMAPFILE_H

#include <QString>
#include "maparch.h"

class CMap;

class CMapFile : public CMapArch
{
public:
    CMapFile();
    virtual ~CMapFile();
    bool read();
    bool write();
    QString filename();
    void setFilename(const QString);
    CMap *map();
    void setDirty(bool b);
    bool isDirty();
    bool isUntitled();
    void setCurrentIndex(int i);
    size_t currentIndex();
    bool isMulti();
    void forget();
    bool isWrongExt();
    CMap *removeAt(int i);

protected:
    size_t m_currIndex;
    bool m_dirty;
    QString m_filename;
};

#endif // CMAPFILE_H
