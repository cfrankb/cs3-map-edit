#ifndef CMAPFILE_H
#define CMAPFILE_H

#include <QString>

class CMap;

class CMapFile
{
public:
    CMapFile();
    ~CMapFile();
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
    void forget();
    void setCurrentIndex(int i);
    int currentIndex();
    bool isMulti();
    int add(CMap *map);
    CMap* removeAt(int i);
    bool isWrongExt();
    void insertAt(int i, CMap *map);
    CMap *at(int i);

protected:
    void allocSpace();

    enum {
        GROW_BY = 5
    };

    int m_currIndex;
    int m_size;
    int m_max;
    bool m_dirty;
    QString m_filename;
    CMap **m_maps;
    std::string m_lastError;
};

#endif // CMAPFILE_H
