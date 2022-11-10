#ifndef CMAPFILE_H
#define CMAPFILE_H

#include "map.h"
#include <string>
#include <QString>


class CMapFile
{
public:
    CMapFile();
    bool read();
    bool write();
    QString filename();
    void setFilename(const QString);
    int size();
    QString lastError();
    CMap *map();
    void setDirty(bool b);
    bool isDirty();
    bool isUntitled();

protected:
    bool m_dirty;
    QString m_filename;
    CMap m_map;
    std::string m_lastError;
};

#endif // CMAPFILE_H
