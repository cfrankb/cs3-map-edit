// Local-only suite: MainWindow, the AUTOUIC-generated .ui structure and hunspell
// spell checking. Labelled "local" (see tests/CMakeLists.txt) because MainWindow
// needs the compiled :/data resources and the spell tests need a real dictionary.
//
// Covers src/mainwindow.*, src/dlgspell.*, src/dlgselect.*, src/tilebox.*,
// src/SpellHighlighter.* and the .ui files (dlgabout/dlgattr/dlgresize/dlgselect/
// dlgstat). The dialog
// logic of those .ui classes is exercised in tst_gui.cpp.
// SpellTextEdit is not covered: it is dead code (never instantiated, and its
// Q_OBJECT moc is never generated, so it does not even link - see tests/REVIEW.md).
//
// Runs with QT_QPA_PLATFORM=offscreen. exec() is never called.

#include <QtTest>
#include <QApplication>
#include <QComboBox>
#include <QFileInfo>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSettings>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTextBlock>
#include <QTextLayout>

#include "dlgabout.h"
#include "dlgattr.h"
#include "dlgresize.h"
#include "dlgselect.h"
#include "dlgspell.h"
#include "dlgstat.h"
#include "mainwindow.h"
#include "mapfile.h"
#include "runtime/map.h"
#include "tilebox.h"

#if defined(USE_HUNSPELL)
#include <hunspell.hxx>
#include "SpellHighlighter.h"
#endif

namespace {

QString dataDir()
{
    return QString(CS3_TEST_DATA_DIR);
}

QString archive()
{
    return dataDir() + "/levels1.mapz";
}

// Create an empty file on disk (enough for DlgSpell::isConfigured()).
void writeEmptyFile(const QString &path)
{
    QFile file(path);
    QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(path));
}

#if defined(USE_HUNSPELL)
// Dictionary for the spell tests: the CS3_SPELL_AFF/CS3_SPELL_DIC environment
// variables or the usual system locations. Both empty when none was found.
void dictionary(QString &aff, QString &dic)
{
    aff = qEnvironmentVariable("CS3_SPELL_AFF");
    dic = qEnvironmentVariable("CS3_SPELL_DIC");
    if (!aff.isEmpty() && !dic.isEmpty() && QFileInfo::exists(aff) && QFileInfo::exists(dic))
        return;

    const QStringList dirs = {"/usr/share/hunspell", "/usr/share/myspell",
                              "/usr/share/myspell/dicts"};
    for (const QString &dir : dirs)
    {
        const QString a = dir + "/en_US.aff";
        const QString d = dir + "/en_US.dic";
        if (QFileInfo::exists(a) && QFileInfo::exists(d))
        {
            aff = a;
            dic = d;
            return;
        }
    }
    aff.clear();
    dic.clear();
}
#endif

} // namespace

class TestLocal : public QObject
{
    Q_OBJECT

private slots:

    void initTestCase()
    {
        // Keep QSettings out of the developer's real configuration: MainWindow
        // writes recentFileList and DlgSpell reads/writes the dictionary paths.
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setOrganizationName("cs3-test");
        QCoreApplication::setApplicationName("cs3-map-edit");
    }

    // ------------------------------------------------------------- MainWindow

    void mainwindow_startsClean()
    {
        MainWindow window;
        QVERIFY(window.centralWidget() != nullptr);
        QVERIFY(window.activeStack() != nullptr);
    }

    void mainwindow_loadFileEmitsMapChanged()
    {
        QVERIFY(QFile::exists(archive()));

        MainWindow window;
        QSignalSpy spy(&window, &MainWindow::mapChanged);
        QMetaObject::invokeMethod(&window, "loadFile", Qt::DirectConnection,
                                  Q_ARG(QString, archive()));

        QCOMPARE(spy.count(), 1);
        QSettings settings;
        QVERIFY(settings.value("recentFileList").toStringList().contains(archive()));
    }

    void mainwindow_loadFileIgnoresEmptyName()
    {
        MainWindow window;
        QSignalSpy spy(&window, &MainWindow::mapChanged);
        QMetaObject::invokeMethod(&window, "loadFile", Qt::DirectConnection,
                                  Q_ARG(QString, QString()));
        QCOMPARE(spy.count(), 0);
    }

    // --------------------------------------------------- .ui structure (AUTOUIC)

    void uiDialogs_exposeTheirNamedWidgets()
    {
        CMap map(16, 16);

        const QList<QPair<QObject *, QString>> expected = {
            {new DlgAbout(),          "tabWidget"},
            {new DlgAbout(),          "eCredits"},
            {new CDlgAttr(),          "line_AttrEdit"},
            {new CDlgResize(),        "eWidth"},
            {new CDlgResize(),        "eHeight"},
            {new CDlgSelect(),        "cbSelect_Maps"},
            {new CDlgSelect(),        "sPreview"},
            {new CDlgStat(&map, 0, 0), "buttonBox"},
        };
        for (const auto &entry : expected)
        {
            QObject *dialog = entry.first;
            QVERIFY2(dialog->findChild<QWidget *>(entry.second) != nullptr,
                     qPrintable(QString("%1 missing child '%2'")
                                    .arg(QString::fromUtf8(dialog->metaObject()->className()),
                                         entry.second)));
            delete dialog;
        }
    }

    void dlgSelect_listsMapsOfTheDocument()
    {
        CMapFile doc;
        doc.setFilename(archive());
        QVERIFY(doc.read());
        QVERIFY(doc.size() > 1);

        CDlgSelect dialog;
        dialog.init("select a map", &doc);

        QComboBox *combo = dialog.findChild<QComboBox *>("cbSelect_Maps");
        QVERIFY(combo != nullptr);
        QCOMPARE(combo->count(), int(doc.size()));
        QCOMPARE(combo->currentIndex(), int(doc.currentIndex()));
    }

    // ---------------------------------------------------------------- CTileBox

    void tileBox_hasPages()
    {
        // Needs :/data/tiles.obl, which only this target compiles.
        CTileBox box;
        QVERIFY(box.count() > 0);
    }

    // ------------------------------------------------------------- DlgSpell

    void dlgSpell_isConfiguredNeedsBothFiles()
    {
        QSettings settings;
        settings.remove("dictionary");
        settings.sync();
        QVERIFY(!DlgSpell::isConfigured());

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString aff = dir.path() + "/test.aff";
        const QString dic = dir.path() + "/test.dic";
        writeEmptyFile(aff);
        writeEmptyFile(dic);

        settings.setValue("dictionary/affFile", aff);
        QVERIFY(!DlgSpell::isConfigured()); // dictionary still missing

        settings.setValue("dictionary/dicFile", dic);
        QVERIFY(DlgSpell::isConfigured());
        QCOMPARE(DlgSpell::affFile(), aff);
        QCOMPARE(DlgSpell::dicFile(), dic);

        settings.remove("dictionary");
        settings.sync();
    }

    void dlgSpell_showsSavedPaths()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString aff = dir.path() + "/test.aff";
        const QString dic = dir.path() + "/test.dic";
        writeEmptyFile(aff);
        writeEmptyFile(dic);

        QSettings settings;
        settings.setValue("dictionary/affFile", aff);
        settings.setValue("dictionary/dicFile", dic);

        DlgSpell dialog;
        const QList<QLineEdit *> edits = dialog.findChildren<QLineEdit *>();
        QCOMPARE(edits.size(), 2);
        QCOMPARE(edits.at(0)->text(), aff);
        QCOMPARE(edits.at(1)->text(), dic);
    }

#if defined(USE_HUNSPELL)
    // ------------------------------------------------------- spell checking

    void spell_underlinesMisspelledWords()
    {
        QString aff, dic;
        dictionary(aff, dic);
        if (aff.isEmpty())
            QSKIP("no hunspell dictionary found (set CS3_SPELL_AFF/CS3_SPELL_DIC)");

        Hunspell spell(aff.toUtf8().constData(), dic.toUtf8().constData());
        QVERIFY(spell.spell(std::string("hello")));
        QVERIFY(!spell.spell(std::string("teh")));

        // Same host widget as MapPropertiesDialog uses for the message editor.
        QPlainTextEdit edit;
        SpellHighlighter highlighter(edit.document(), &spell);
        edit.setPlainText("hello teh world");

        // QSyntaxHighlighter applies its formats to the block layout, not to the
        // document's character formats, so the ranges have to be read from there.
        bool underlined = false;
        for (QTextBlock block = edit.document()->begin(); block.isValid(); block = block.next())
        {
            const QList<QTextLayout::FormatRange> ranges = block.layout()->formats();
            for (const QTextLayout::FormatRange &range : ranges)
            {
                if (range.start != 6 || range.length != 3) // "teh"
                    continue;
                underlined = true;
                QCOMPARE(int(range.format.underlineStyle()),
                         int(QTextCharFormat::SpellCheckUnderline));
            }
        }
        QVERIFY2(underlined, "misspelled word was not underlined");
    }
#endif
};

QTEST_MAIN(TestLocal)
#include "tst_local.moc"
