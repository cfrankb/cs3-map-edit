// Headless suites: document model (CMapFile), undo commands, Stamp and report.
// Covers src/mapfile.*, src/report.*, src/stamp.h and src/undo/*.h
// (see tests/TRACKER.md).

#include <QtTest>
#include <QTemporaryDir>
#include <QUndoStack>

#include "mapfile.h"
#include "report.h"
#include "stamp.h"
#include "undo/DocumentCommand.h"
#include "undo/MapCommand.h"
#include "undo/MapListCommand.h"
#include "undo/MapPropertiesCommand.h"
#include "runtime/map.h"
#include "runtime/statedata.h"

// The editor's default map size, see CMapFile constructor.
static const int DEFAULT_MAP_SIZE = 64;

class TestCore : public QObject
{
    Q_OBJECT

private:
    static QString dataDir()
    {
        return QString(CS3_TEST_DATA_DIR);
    }

    // Loads tests/in/levels1.mapz into doc; fails the test if it cannot.
    void loadArchive(CMapFile &doc)
    {
        doc.setFilename(dataDir() + "/levels1.mapz");
        QVERIFY(doc.read());
    }

private slots:
    void initTestCase()
    {
        QVERIFY(QFileInfo::exists(dataDir() + "/levels1.mapz"));
    }

    // ---------------------------------------------------------------- CMapFile

    void mapFile_startsOneUntitledMap()
    {
        CMapFile doc;
        QVERIFY(doc.size() == 1);
        QVERIFY(doc.isUntitled());
        QVERIFY(!doc.isDirty());
        QVERIFY(!doc.isMulti());
        QVERIFY(doc.map() != nullptr);
        QCOMPARE(doc.map()->width(), DEFAULT_MAP_SIZE);
        QCOMPARE(doc.map()->height(), DEFAULT_MAP_SIZE);
        QVERIFY(doc.currentIndex() == 0);
    }

    void mapFile_addMapBecomesCurrent()
    {
        CMapFile doc;
        const size_t index = doc.addMap(std::make_unique<CMap>(12, 13));
        QVERIFY(index == 1);
        QVERIFY(doc.size() == 2);
        QVERIFY(doc.isMulti());
        QVERIFY(doc.currentIndex() == 1);
        QCOMPARE(doc.map()->width(), 12);
        QCOMPARE(doc.map()->height(), 13);
    }

    void mapFile_removeAtKeepsAtLeastOneMap()
    {
        CMapFile doc;
        QVERIFY(doc.removeAt(0) == nullptr); // invariant: the last map stays
        QVERIFY(doc.size() == 1);

        doc.addMap(std::make_unique<CMap>(8, 8));
        QVERIFY(doc.removeAt(0) != nullptr);
        QVERIFY(doc.size() == 1);
    }

    void mapFile_removeAtRejectsOutOfRange()
    {
        CMapFile doc;
        doc.addMap(std::make_unique<CMap>(8, 8));
        QVERIFY(doc.removeAt(-1) == nullptr);
        QVERIFY(doc.removeAt(2) == nullptr);
        QVERIFY(doc.size() == 2);
    }

    void mapFile_insertAtClampsIndex()
    {
        CMapFile doc;
        doc.insertAt(99, std::make_unique<CMap>(8, 8));
        QVERIFY(doc.currentIndex() == 1);
        doc.insertAt(-3, std::make_unique<CMap>(9, 9));
        QVERIFY(doc.size() == 3);
        QVERIFY(doc.currentIndex() == 0);
        QCOMPARE(doc.at(0)->width(), 9);
    }

    void mapFile_setCurrentIndexSwitchesActiveStack()
    {
        CMapFile doc;
        doc.addMap(std::make_unique<CMap>(8, 8));
        QUndoStack *second = doc.activeStack();
        QVERIFY(second != doc.docStack());

        doc.setCurrentIndex(0);
        QVERIFY(doc.activeStack() != second);
        QVERIFY(doc.currentIndex() == 0);
    }

    void mapFile_invalidIndexResetsToFirstMap()
    {
        CMapFile doc;
        doc.addMap(std::make_unique<CMap>(8, 8));

        CMap *received = reinterpret_cast<CMap *>(1);
        connect(&doc, &CMapFile::currentMapChanged,
                [&received](CMap *map) { received = map; });

        doc.setCurrentIndex(42);
        QVERIFY(doc.currentIndex() == 0);
        QVERIFY(received == nullptr);
        QVERIFY(doc.activeStack() == doc.docStack());
    }

    void mapFile_filenameSignalsAndExtensions()
    {
        CMapFile doc;
        int emitted = 0;
        connect(&doc, &CMapFile::filenameChanged,
                [&emitted](const QString &) { ++emitted; });

        doc.setFilename("/tmp/level.dat");
        QCOMPARE(emitted, 1);
        QVERIFY(!doc.isUntitled());
        QCOMPARE(doc.filename(), QString("/tmp/level.dat"));

        doc.setFilename("/tmp/level.dat"); // unchanged: no signal
        QCOMPARE(emitted, 1);

        QVERIFY(!doc.isWrongExt()); // single map expects .dat
        doc.setFilename("/tmp/level.mapz");
        QVERIFY(doc.isWrongExt());
    }

    void mapFile_dirtyFlagFollowsSignal()
    {
        CMapFile doc;
        emit doc.dirtyChanged(true);
        QVERIFY(doc.isDirty());
        emit doc.dirtyChanged(false);
        QVERIFY(!doc.isDirty());
    }

    void mapFile_readRejectsMissingFile()
    {
        CMapFile doc;
        doc.setFilename(dataDir() + "/does-not-exist.mapz");
        QVERIFY(!doc.read());
    }

    void mapFile_readArchiveBuildsOneStackPerMap()
    {
        CMapFile doc;
        loadArchive(doc);

        QVERIFY(doc.size() > 1);
        QVERIFY(!doc.isDirty());
        QVERIFY(doc.currentIndex() == 0);
        // one stack per map plus the document stack
        QCOMPARE(doc.undoGroup()->stacks().size(), int(doc.size()) + 1);
    }

    void mapFile_writeRoundTrip()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString out = dir.path() + "/roundtrip.mapz";

        CMapFile source;
        loadArchive(source);
        source.setFilename(out);
        QVERIFY(source.write());
        QVERIFY(QFileInfo::exists(out));

        CMapFile copy;
        copy.setFilename(out);
        QVERIFY(copy.read());

        QVERIFY(copy.size() == source.size());
        for (size_t i = 0; i < source.size(); ++i)
        {
            QCOMPARE(copy.at(int(i))->width(), source.at(int(i))->width());
            QCOMPARE(copy.at(int(i))->height(), source.at(int(i))->height());
            QCOMPARE(QString(copy.at(int(i))->title()), QString(source.at(int(i))->title()));
        }
    }

    void mapFile_resetRestoresDefaults()
    {
        CMapFile doc;
        doc.addMap(std::make_unique<CMap>(8, 8));
        doc.setFilename("/tmp/level.dat");
        doc.setDirty(true);

        doc.reset();
        QVERIFY(doc.size() == 1);
        QVERIFY(doc.isUntitled());
        QVERIFY(!doc.isDirty());
        QCOMPARE(doc.map()->width(), 40);
    }

    void mapFile_resetUndoStacksKeepsOnlyDocumentStack()
    {
        CMapFile doc;
        doc.addMap(std::make_unique<CMap>(8, 8));
        QVERIFY(doc.undoGroup()->stacks().size() >= 3);

        doc.resetUndoStacks();
        QCOMPARE(doc.undoGroup()->stacks().size(), 1);
        QVERIFY(doc.undoGroup()->activeStack() == doc.docStack());
    }

    // ------------------------------------------------------------ undo commands

    void commands_insertMapRoundTrip()
    {
        CMapFile doc;
        QUndoStack stack;

        stack.push(new InsertMapCommand(&doc, 1, std::make_unique<CMap>(12, 13)));
        QVERIFY(doc.size() == 2);
        QCOMPARE(doc.at(1)->width(), 12);

        stack.undo();
        QVERIFY(doc.size() == 1);

        stack.redo();
        QVERIFY(doc.size() == 2);
        QCOMPARE(doc.at(1)->width(), 12);
    }

    void commands_deleteMapRoundTrip()
    {
        CMapFile doc;
        doc.addMap(std::make_unique<CMap>(12, 13));
        QUndoStack stack;

        stack.push(new DeleteMapCommand(&doc, 1));
        QVERIFY(doc.size() == 1);

        stack.undo();
        QVERIFY(doc.size() == 2);
        QCOMPARE(doc.at(1)->width(), 12);
    }

    void commands_deleteMapRefusesLastMap()
    {
        CMapFile doc;
        QUndoStack stack;

        stack.push(new DeleteMapCommand(&doc, 0));
        QVERIFY(doc.size() == 1);
    }

    void commands_moveMapRoundTrip()
    {
        CMapFile doc;
        doc.addMap(std::make_unique<CMap>(11, 11));
        doc.addMap(std::make_unique<CMap>(22, 22));
        QVERIFY(doc.size() == 3);

        QUndoStack stack;
        stack.push(new MoveMapCommand(&doc, 0, 2));
        QCOMPARE(doc.at(1)->width(), DEFAULT_MAP_SIZE); // [11, 64, 22]

        stack.undo();
        QCOMPARE(doc.at(0)->width(), DEFAULT_MAP_SIZE); // [64, 11, 22]
    }

    void commands_moveMapToSameIndexIsNoOp()
    {
        CMapFile doc;
        doc.addMap(std::make_unique<CMap>(11, 11));
        QUndoStack stack;

        stack.push(new MoveMapCommand(&doc, 1, 1));
        QCOMPARE(doc.at(1)->width(), 11);
    }

    void commands_renameMapRoundTrip()
    {
        CMapFile doc;
        CMap *map = doc.map();
        map->setTitle("original");
        QUndoStack stack;

        stack.push(new RenameMapCommand(&doc, map, "renamed"));
        QCOMPARE(QString(map->title()), QString("renamed"));

        stack.undo();
        QCOMPARE(QString(map->title()), QString("original"));
    }

    void commands_clearMapRoundTrip()
    {
        CMapFile doc;
        CMap *map = doc.map();
        map->set(3, 4, 42);
        QUndoStack stack;

        stack.push(new ClearMapCommand(&doc));
        QVERIFY(map->getMainLayer()->size() == 0); // cleared, not zero-filled

        stack.undo();
        QCOMPARE(int(map->at(3, 4)), 42);
    }

    void commands_shiftMapRoundTrip()
    {
        CMapFile doc;
        CMap *map = doc.map();
        map->set(0, 0, 7);
        QUndoStack stack;

        stack.push(new ShiftMapCommand(&doc, Direction::RIGHT));
        QCOMPARE(int(map->at(1, 0)), 7);

        stack.undo();
        QCOMPARE(int(map->at(0, 0)), 7);
    }

    void commands_resizeMapRoundTrip()
    {
        CMapFile doc;
        CMap *map = doc.map();
        map->set(2, 2, 5);
        QUndoStack stack;

        stack.push(new ResizeMapCommand(&doc, map, 100, 100));
        QCOMPARE(map->width(), 100);
        QCOMPARE(map->height(), 100);

        stack.undo();
        QCOMPARE(map->width(), DEFAULT_MAP_SIZE);
        QCOMPARE(int(map->at(2, 2)), 5);
    }

    void commands_mapPropertiesRoundTrip()
    {
        CMapFile doc;
        CMap *map = doc.map();
        map->setTitle("before");

        CStates after = map->states();
        after.setU(PAR_TIME, 99);

        QUndoStack stack;
        stack.push(new MapPropertiesCommand(map, nullptr, after, "after",
                                            MapPropertiesCommand::EditMode::StatesAndTitle));
        QCOMPARE(QString(map->title()), QString("after"));
        QCOMPARE(int(map->states().getU(PAR_TIME)), 99);

        stack.undo();
        QCOMPARE(QString(map->title()), QString("before"));
    }

    void commands_mapPropertiesStatesOnlyKeepsTitle()
    {
        CMapFile doc;
        CMap *map = doc.map();
        map->setTitle("keep-me");

        CStates after = map->states();
        after.setU(PAR_TIME, 7);

        QUndoStack stack;
        stack.push(new MapPropertiesCommand(map, nullptr, after, "ignored",
                                            MapPropertiesCommand::EditMode::StatesOnly));
        QCOMPARE(QString(map->title()), QString("keep-me"));
        QCOMPARE(int(map->states().getU(PAR_TIME)), 7);
    }

    void commands_mapListSaveRoundTrip()
    {
        CMapFile doc;
        CMap *map = doc.map();
        map->setTitle("old");

        MapListSaveCommand::MapChange change;
        change.map = map;
        change.oldTitle = "old";
        change.oldStates = map->states();
        change.newTitle = "new";
        change.newStates = map->states();

        QUndoStack stack;
        stack.push(new MapListSaveCommand(&doc, {change}));
        QCOMPARE(QString(map->title()), QString("new"));
        QVERIFY(doc.isDirty());

        stack.undo();
        QCOMPARE(QString(map->title()), QString("old"));
    }

    // -------------------------------------------------------------------- Stamp

    void stamp_defaults()
    {
        const Stamp stamp;
        QVERIFY(stamp.tiles.empty());
        QCOMPARE(stamp.cols, 0);
        QCOMPARE(stamp.rows, 0);
        QCOMPARE(int(stamp.baseID), int(Stamp::MainTilesetBaseID));
        QCOMPARE(int(Stamp::OtherTilesetBaseID), 0x100);
    }

    // ------------------------------------------------------------------- Report

    void report_buildsTextForLoadedArchive()
    {
        CMapFile doc;
        loadArchive(doc);

        const QString text = buildReport(doc);
        QVERIFY(!text.isEmpty());
        QVERIFY(text.contains("Par time"));
    }

    void report_writeFileAndFailureOnBadPath()
    {
        CMapFile doc;
        loadArchive(doc);

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString out = dir.path() + "/report.txt";
        QVERIFY(generateReport(doc, out));
        QVERIFY(QFileInfo(out).size() > 0);

        QVERIFY(!generateReport(doc, "/no/such/directory/report.txt"));
    }
};

QTEST_GUILESS_MAIN(TestCore)
#include "tst_core.moc"
