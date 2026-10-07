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
#include "dlgmaps.h"
#include "mapfile.h"
#include "runtime/map.h"
#include "runtime/statedata.h"
#include "runtime/states.h"
#include "undo/MapListCommand.h"
#include "runtime/shared/helper.h"
#include <QUndoStack>
#include <QVBoxLayout>
#include <QStringList>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QSettings>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QGuiApplication>
#include <QScreen>

const char *const DialogMaps::KEY_MUSIC_FOLDER = "music/musicFolder";

DialogMaps::DialogMaps(CMapFile *mapFile, QWidget *parent)
    : QDialog(parent)
    , m_mapFile(mapFile)
    , m_table(new QTableWidget(this))
    , m_dirty(false)
    , m_musicFolderEdit(new QLineEdit(this))
    , m_statusLabel(new QLabel(this))
{
    setWindowTitle(tr("Map List"));
    setupUI();
    loadFromMapFile();

    // Restore the persisted music folder (if any)
    m_musicFolderEdit->setText(musicFolderPath());

    // Default size large enough to show all columns without horizontal scroll
    setMinimumWidth(920);
    setMinimumHeight(480);
}

void DialogMaps::setupUI()
{
    const char *columnTitles[] = {
        "Map Name",
        "Author",
        "Year",     // narrow, numeric
        "Music",
        "Notes",
        "Private",  // checkbox
        "UUID",     // identifier, regenerated via a per-row button
    };
    constexpr int columnCount = sizeof(columnTitles) / sizeof(columnTitles[0]);

    m_table->setColumnCount(columnCount);
    QStringList labels;
    labels.reserve(columnCount);
    for (int c = 0; c < columnCount; ++c)
        labels << QString::fromLatin1(columnTitles[c]);
    m_table->setHorizontalHeaderLabels(labels);
    m_table->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    // The name/author/year/music/notes fields and the private checkbox are editable.
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

    // Fixed-width columns: year, private and uuid.
    m_table->horizontalHeader()->setSectionResizeMode(COL_YEAR, QHeaderView::Fixed);
    m_table->setColumnWidth(COL_YEAR, 70);
    m_table->horizontalHeader()->setSectionResizeMode(COL_PRIVATE, QHeaderView::Fixed);
    m_table->setColumnWidth(COL_PRIVATE, 60);
    m_table->horizontalHeader()->setSectionResizeMode(COL_UUID, QHeaderView::Fixed);
    m_table->setColumnWidth(COL_UUID, 420);
    // The remaining columns use interactive resize with sensible starting
    // widths. Their combined width exceeds the dialog's minimum width, so a
    // horizontal scrollbar appears automatically when not all columns fit.
    m_table->horizontalHeader()->setSectionResizeMode(COL_NAME, QHeaderView::Interactive);
    m_table->setColumnWidth(COL_NAME, 220);
    m_table->horizontalHeader()->setSectionResizeMode(COL_AUTHOR, QHeaderView::Interactive);
    m_table->setColumnWidth(COL_AUTHOR, 150);
    m_table->horizontalHeader()->setSectionResizeMode(COL_MUSIC, QHeaderView::Interactive);
    m_table->setColumnWidth(COL_MUSIC, 150);
    m_table->horizontalHeader()->setSectionResizeMode(COL_NOTES, QHeaderView::Interactive);
    m_table->setColumnWidth(COL_NOTES, 200);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // ===== Top control area (above the table) =====
    QVBoxLayout *topLayout = new QVBoxLayout();

    // Music folder field + browse button
    QHBoxLayout *folderRow = new QHBoxLayout();
    QLabel *folderLabel = new QLabel(tr("Music folder:"), this);
    m_musicFolderEdit->setPlaceholderText(tr("Path to the folder that holds the music files"));
    QPushButton *browseBtn = new QPushButton(tr("Browse…"), this);
    folderRow->addWidget(folderLabel);
    folderRow->addWidget(m_musicFolderEdit);
    folderRow->addWidget(browseBtn);
    topLayout->addLayout(folderRow);

    // Check music files + resize-to-screen buttons
    QHBoxLayout *checkRow = new QHBoxLayout();
    QPushButton *checkBtn = new QPushButton(tr("Check Music Files"), this);
    QPushButton *resizeBtn = new QPushButton(tr("Resize to Screen"), this);
    checkRow->addWidget(checkBtn);
    checkRow->addWidget(resizeBtn);
    checkRow->addStretch();
    topLayout->addLayout(checkRow);

    // Status/results label
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setText(QString());
    topLayout->addWidget(m_statusLabel);

    mainLayout->addLayout(topLayout);

    // ===== Table =====
    mainLayout->addWidget(m_table);

    // ===== Dialog buttons =====
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &DialogMaps::onAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &DialogMaps::onReject);
    mainLayout->addWidget(buttonBox);

    // Connect the top-area buttons
    connect(browseBtn, &QPushButton::clicked, this, &DialogMaps::onMusicFolderBrowse);
    connect(checkBtn, &QPushButton::clicked, this, &DialogMaps::onCheckMusic);
    connect(resizeBtn, &QPushButton::clicked, this, &DialogMaps::onResizeToScreen);

    setLayout(mainLayout);
}

void DialogMaps::loadFromMapFile()
{
    m_original.clear();

    if (!m_mapFile)
        return;

    const size_t count = m_mapFile->size();
    m_table->setRowCount(static_cast<int>(count));

    m_table->setUpdatesEnabled(false);
    for (size_t i = 0; i < count; ++i)
    {
        CMap *map = m_mapFile->at(static_cast<int>(i));

        RowData data;
        data.name = map ? QString::fromUtf8(map->title()) : QString();
        if (map)
        {
            const CStates &states = map->states();
            data.author = QString::fromUtf8(states.getS(StateValue::AUTHOR));
            data.year   = states.getU(StateValue::YEAR);
            data.isPrivate = states.getU(StateValue::PRIVATE) != 0;
            data.music  = QString::fromUtf8(states.getS(StateValue::MUSIC));
            data.notes  = QString::fromUtf8(states.getS(StateValue::NOTES));
            data.uuid   = QString::fromUtf8(states.getS(StateValue::UUID));
        }
        m_original.push_back(data);

        const int row = static_cast<int>(i);

        // Columns 0..1: name, author
        m_table->setItem(row, COL_NAME, new QTableWidgetItem(data.name));
        m_table->setItem(row, COL_AUTHOR, new QTableWidgetItem(data.author));

        // Column 2: year (text, fixed width)
        QTableWidgetItem *yearItem = new QTableWidgetItem(QString::number(data.year));
        yearItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(row, COL_YEAR, yearItem);

        // Columns 3..4: music, notes
        m_table->setItem(row, COL_MUSIC, new QTableWidgetItem(data.music));
        m_table->setItem(row, COL_NOTES, new QTableWidgetItem(data.notes));

        // Column 5: private (checkbox)
        QCheckBox *privBox = new QCheckBox(this);
        privBox->setChecked(data.isPrivate);
        m_table->setCellWidget(row, COL_PRIVATE, privBox);

        // Column 6: UUID (read-only label + per-row "New Uuid" button)
        m_uuidLabels.push_back(nullptr);
        QWidget *uuidCell = new QWidget(this);
        QHBoxLayout *uuidLayout = new QHBoxLayout(uuidCell);
        uuidLayout->setContentsMargins(6, 2, 6, 2);
        QLabel *uuidLabel = new QLabel(data.uuid, uuidCell);
        uuidLabel->setToolTip(data.uuid);
        QPushButton *newUuidBtn = new QPushButton("New Uuid", uuidCell);
        newUuidBtn->setFixedWidth(76);
        connect(newUuidBtn, &QPushButton::clicked, this, [this, row]() { onNewUuid(row); });
        uuidLayout->addWidget(uuidLabel);
        uuidLayout->addWidget(newUuidBtn);
        m_table->setCellWidget(row, COL_UUID, uuidCell);
        m_uuidLabels[row] = uuidLabel;
    }
    m_table->setUpdatesEnabled(true);
}

void DialogMaps::saveToMapFile()
{
    if (!m_mapFile)
        return;

    m_dirty = false;
    const size_t count = m_mapFile->size();

    std::vector<MapListSaveCommand::MapChange> changes;

    for (size_t i = 0; i < count; ++i)
    {
        CMap *map = m_mapFile->at(static_cast<int>(i));
        if (!map)
            continue;

        QTableWidgetItem *nameItem  = m_table->item(static_cast<int>(i), COL_NAME);
        QTableWidgetItem *authorItem = m_table->item(static_cast<int>(i), COL_AUTHOR);
        QTableWidgetItem *musicItem = m_table->item(static_cast<int>(i), COL_MUSIC);
        QTableWidgetItem *notesItem = m_table->item(static_cast<int>(i), COL_NOTES);

        QTableWidgetItem *yearItem = m_table->item(i, COL_YEAR);
        QCheckBox *privBox = qobject_cast<QCheckBox *>(m_table->cellWidget(i, COL_PRIVATE));

        const QString name   = nameItem  ? nameItem->text().trimmed()  : QString();
        const QString author = authorItem ? authorItem->text().trimmed() : QString();
        const QString music  = musicItem ? musicItem->text().trimmed()  : QString();
        const QString notes  = notesItem ? notesItem->text().trimmed()  : QString();
        const int year = yearItem ? yearItem->text().trimmed().toInt() : 0;
        const bool isPrivate = privBox ? privBox->isChecked() : false;

        // UUID lives in the per-row label (the "New Uuid" button only updates
        // the display here; the value is written to the map at save time).
        const QString uuid = (i < static_cast<int>(m_uuidLabels.size()) && m_uuidLabels[i])
                                 ? m_uuidLabels[i]->text().trimmed() : QString();

        // Detect modification against the values captured at load.
        const RowData &orig = m_original[i];
        const bool changed = (name != orig.name) || (author != orig.author)
                             || (year != orig.year) || (isPrivate != orig.isPrivate)
                             || (music != orig.music) || (notes != orig.notes)
                             || (uuid != orig.uuid);
        if (!changed)
            continue;

        // Snapshot the pre-save state so the change can be undone, then
        // capture the post-save state before applying the edits.
        MapListSaveCommand::MapChange change;
        change.map = map;
        change.oldTitle = map->title();
        change.oldStates = map->states();

        // Write the modified values back to the map.
        map->setTitle(name.toStdString());
        CStates &states = map->states();
        states.setS(StateValue::AUTHOR, author.toStdString());
        states.setU(StateValue::YEAR, static_cast<uint16_t>(year));
        states.setU(StateValue::PRIVATE, isPrivate ? 1 : 0);
        states.setS(StateValue::MUSIC, music.toStdString());
        states.setS(StateValue::NOTES, notes.toStdString());
        states.setS(StateValue::UUID, uuid.toStdString());

        change.newTitle = map->title();
        change.newStates = map->states();
        changes.push_back(std::move(change));

        m_dirty = true;
    }

    // Propagate the dirty state to the file only if something changed.
    // Follows the same pattern used elsewhere in CMapFile (e.g. removeAt):
    // set the flag and notify listeners so the editor reflects the change.
    if (m_dirty)
    {
        // Record the committed edits on the undo stack so the whole map-list
        // save can be undone/redone as a single action.
        QUndoStack *stack = m_mapFile->undoGroup()
                                ? m_mapFile->undoGroup()->activeStack()
                                : nullptr;
        if (stack)
            stack->push(new MapListSaveCommand(m_mapFile, std::move(changes)));
        else
        {
            m_mapFile->setDirty(true);
            m_mapFile->emit dirtyChanged(true);
        }
    }
}

QString DialogMaps::musicFolderPath() const
{
    QSettings settings;
    return settings.value(KEY_MUSIC_FOLDER, QString()).toString();
}

void DialogMaps::setMusicFolderPath(const QString &path)
{
    QSettings settings;
    settings.setValue(KEY_MUSIC_FOLDER, path.trimmed());
}

void DialogMaps::onMusicFolderBrowse()
{
    const QString chosen = QFileDialog::getExistingDirectory(
        this, tr("Select Music Folder"), musicFolderPath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!chosen.isEmpty())
    {
        m_musicFolderEdit->setText(chosen);
        setMusicFolderPath(chosen);
    }
}

void DialogMaps::onCheckMusic()
{
    checkMusicFiles();
}

void DialogMaps::checkMusicFiles()
{
    const QString base = m_musicFolderEdit->text().trimmed();
    if (base.isEmpty())
    {
        m_statusLabel->setText(tr("Set a music folder first."));
        QMessageBox::information(
            this, tr("Check Music Files"),
            tr("Choose the folder that holds the music files using the "
               "Browse button, then check again."));
        return;
    }

    const int rows = m_table->rowCount();
    QStringList missing;
    int checked = 0;

    for (int i = 0; i < rows; ++i)
    {
        QTableWidgetItem *musicItem = m_table->item(i, COL_MUSIC);
        const QString music = musicItem ? musicItem->text().trimmed() : QString();

        // Ignore empty music fields
        if (music.isEmpty())
            continue;

        const QString nameItemText = m_table->item(i, COL_NAME)
                                     ? m_table->item(i, COL_NAME)->text()
                                     : QString::fromLatin1("(map %1)").arg(i + 1);
        const QString fullPath = base + QLatin1Char('/') + music;

        ++checked;
        if (!QFileInfo::exists(fullPath))
            missing << QString("  %1 -> %2").arg(nameItemText, fullPath);
    }

    // Report the result
    m_statusLabel->setText(tr("%1 music file(s) checked, %2 missing.").arg(checked).arg(missing.size()));

    if (!missing.isEmpty())
    {
        QMessageBox::warning(
            this, tr("Check Music Files"),
            tr("The following music file(s) could not be found:") + QString("\n\n") + missing.join("\n"));
    }
    else
    {
        QMessageBox::information(
            this, tr("Check Music Files"),
            tr("All %1 music file(s) were found.").arg(checked));
    }
}

void DialogMaps::onResizeToScreen()
{
    QScreen *screen = window() ? window()->screen() : nullptr;
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    if (!screen)
        return;

    const QRect geo = screen->availableGeometry();

    // Scale the dialog to a fraction of the available screen area, clamped so
    // it stays at least the dialog's minimum size and never exceeds the screen.
    const int w = qBound(minimumWidth(), qRound(geo.width() * 0.90), geo.width());
    const int h = qBound(minimumHeight(), qRound(geo.height() * 0.80), geo.height());
    resize(w, h);
}

void DialogMaps::onNewUuid(int row)
{
    if (row < 0 || row >= m_table->rowCount())
        return;

    CMap *map = m_mapFile ? m_mapFile->at(row) : nullptr;
    if (!map)
        return;

    // Generate a fresh UUID and reflect it in the row's label. The map's
    // states are written to disk only when the dialog is saved (see
    // saveToMapFile), so this stays consistent with the other editable fields.
    const QString newUuid = QString::fromUtf8(getUUID().c_str());
    if (row < static_cast<int>(m_uuidLabels.size()) && m_uuidLabels[row])
    {
        m_uuidLabels[row]->setText(newUuid);
        m_uuidLabels[row]->setToolTip(newUuid);
    }
}

void DialogMaps::onAccept()
{
    // Persist the music folder even when it was edited by hand
    setMusicFolderPath(m_musicFolderEdit->text());

    saveToMapFile();
    accept();
}

void DialogMaps::onReject()
{
    reject();
}
