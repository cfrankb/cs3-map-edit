#pragma once

#include <QString>
#include <QObject>
#include "runtime/maparch.h"
#include "runtime/map.h"
#include <QUndoGroup>
#include <QUndoStack>

class CMap;

class CMapFile : public QObject, public CMapArch
{
Q_OBJECT
public:
    explicit CMapFile(QObject* parent = nullptr);
    ~CMapFile() override;
    bool read();
    bool write();

    QString filename();
    void setFilename(const QString &);

    CMap *map();
    void setDirty(bool b);
    bool isDirty();
    bool isUntitled();
    size_t addMap(std::unique_ptr<CMap> map);
    size_t add(std::unique_ptr<CMap> &map) = delete;
    bool read(IFile &file) = delete;
    bool read(const std::string_view &filename) = delete;
    bool extract(const std::string_view &filename) = delete;
    void removeAll() = delete;
    bool fromMemory(uint8_t *ptr) = delete;
    void clear() = delete;

    void setCurrentIndex(int i);
    size_t currentIndex();

    bool isMulti();
    void forget();
    bool isWrongExt();
    std::unique_ptr<CMap> removeAt(int i);
    void insertAt(int i,  std::unique_ptr<CMap> map);
    void reset();
    void resetUndoStacks();
    void sync();

    QUndoGroup* undoGroup() {
        return m_undoGroup;
    }

    QUndoStack* docStack() {
        return m_docStack;
    }

    QUndoStack* activeStack() {
        return m_undoGroup->activeStack();
    }

    void debug();


signals:
    void filenameChanged(const QString&);
    void dirtyChanged(bool);
    void currentMapChanged(CMap*);
    void refreshMap();
    void resizeMap(int, int);   // notify of a map resize


protected:
    QUndoGroup* m_undoGroup;
    QUndoStack* m_docStack;
    std::vector<QUndoStack*> m_mapStacks; // parallel to CMapArch::m_maps
    size_t m_currIndex{0};
    bool m_dirty;
    QString m_filename;
};

