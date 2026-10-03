/*
    cs3-runtime-sdl
    Copyright (C) 2025 Francois Blanchette

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <vector>
#include <QString>

class CMapFile;

// Dialog that lists every map in a CMapFile as a table and lets the user
// edit the map name, author, year, music, notes and private flag. On commit
// the values are
// written back to each CMap and the file's dirty flag is set if anything
// was modified.
//
// Above the table is a music-folder field (path persisted in app settings)
// plus a button that verifies every map's music file exists under that folder.
class DialogMaps : public QDialog
{
    Q_OBJECT

public:
    explicit DialogMaps(CMapFile *mapFile, QWidget *parent = nullptr);

    // true when at least one field changed during the session
    bool isDirty() const { return m_dirty; }

private slots:
    void onAccept();
    void onReject();
    void onMusicFolderBrowse();
    void onCheckMusic();
    void onResizeToScreen();

private:
    // Persisted music-folder path key (value stored in app settings).
    static const char *const KEY_MUSIC_FOLDER;

    void setupUI();
    void loadFromMapFile();
    void saveToMapFile();
    void checkMusicFiles();

    // Persisted music folder path (see KEY_MUSIC_FOLDER).
    QString musicFolderPath() const;
    void setMusicFolderPath(const QString &path);

    enum {
        COL_NAME = 0,
        COL_AUTHOR = 1,
        COL_YEAR = 2,
        COL_MUSIC = 3,
        COL_NOTES = 4,
        COL_PRIVATE = 5,
    };

    struct RowData {
        QString name;
        QString author;
        int year = 0;
        bool isPrivate = false;
        QString music;
        QString notes;
    };

    CMapFile *m_mapFile;
    QTableWidget *m_table;
    bool m_dirty;
    std::vector<RowData> m_original; // values captured at load, for dirty detection

    QLineEdit *m_musicFolderEdit;
    QLabel *m_statusLabel;
};
