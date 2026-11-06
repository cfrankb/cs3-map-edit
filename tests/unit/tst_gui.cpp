// Offscreen GUI suites: dialogs and widgets built in code or from .ui files.
// Covers src/keyvaluedialog.*, src/dlgattr.*, src/dlgresize.*, src/dlgabout.*,
// src/dlgstat.*, src/dlgmaps.*, src/mapprops.*, src/MapView.*,
// src/MapWidget.*, src/TileSelectorWidget.* and src/LayerDock.*.
//
// Runs with QT_QPA_PLATFORM=offscreen (see tests/CMakeLists.txt). Modal exec()
// is never called: widgets are exercised through their public API only. This
// target deliberately does not link src/mapedit.qrc, so nothing here may read
// the :/data resources.

#include <QtTest>
#include <QApplication>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QUndoStack>

#include "dlgabout.h"
#include "dlgattr.h"
#include "dlgmaps.h"
#include "dlgresize.h"
#include "dlgstat.h"
#include "keyvaluedialog.h"
#include "LayerDock.h"
#include "MapView.h"
#include "MapWidget.h"
#include "mapprops.h"
#include "mapfile.h"
#include "TileSelectorWidget.h"
#include "runtime/map.h"

class TestGui : public QObject
{
    Q_OBJECT

private slots:
    // ------------------------------------------------------- KeyValueDialog

    void keyValueDialog_optionType()
    {
        QCOMPARE(int(KeyValueDialog::getOptionType(0x05)), int(TYPE_U));
        QCOMPARE(int(KeyValueDialog::getOptionType(0x7f)), int(TYPE_U));
        QCOMPARE(int(KeyValueDialog::getOptionType(0x80)), int(TYPE_S));
        QCOMPARE(int(KeyValueDialog::getOptionType(0x180)), int(TYPE_S));
    }

    void keyValueDialog_parseStringToUint16()
    {
        bool valid = false;
        QCOMPARE(int(KeyValueDialog::parseStringToUint16("0x10", valid)), 16);
        QVERIFY(valid);

        QCOMPARE(int(KeyValueDialog::parseStringToUint16("0Xff", valid)), 255);
        QVERIFY(valid);

        QCOMPARE(int(KeyValueDialog::parseStringToUint16("42", valid)), 42);
        QVERIFY(valid);

        QCOMPARE(int(KeyValueDialog::parseStringToUint16("0x10g", valid)), 16);
        QVERIFY(!valid); // trailing garbage

        QCOMPARE(int(KeyValueDialog::parseStringToUint16("abc", valid)), 0);
        QVERIFY(!valid);

        QCOMPARE(int(KeyValueDialog::parseStringToUint16("", valid)), 0);
        QVERIFY(!valid);
    }

    void keyValueDialog_populateRoundTrip()
    {
        KeyValueDialog dialog;
        const std::vector<StateValuePair> pairs = {
            {PAR_TIME, "60", "par time"},
            {0x80, "some text", ""},
        };
        dialog.populateData(pairs);

        const auto read = dialog.getKeyValuePairs();
        QVERIFY(read.size() == pairs.size());
        QVERIFY(read[0].key == pairs[0].key);
        QCOMPARE(QString(read[0].value.c_str()), QString("60"));
        QVERIFY(read[1].key == pairs[1].key);
        QCOMPARE(QString(read[1].value.c_str()), QString("some text"));
    }

    // ---------------------------------------------------------------- CDlgAttr

    void dlgAttr_attrRoundTrip()
    {
        CDlgAttr dialog;
        dialog.attr(0x1f);
        QCOMPARE(int(dialog.attr()), 0x1f);
    }

    void dlgAttr_okFollowsHexInput()
    {
        CDlgAttr dialog;
        auto ok = dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok);

        dialog.findChild<QLineEdit *>("line_AttrEdit")->setText("1f");
        QVERIFY(ok->isEnabled());

        dialog.findChild<QLineEdit *>("line_AttrEdit")->setText("");
        QVERIFY(!ok->isEnabled());
    }

    // -------------------------------------------------------------- CDlgResize

    void dlgResize_widthHeightRoundTrip()
    {
        CDlgResize dialog;
        dialog.width(64);
        dialog.height(32);
        QCOMPARE(dialog.width(), 64);
        QCOMPARE(dialog.height(), 32);
    }

    void dlgResize_okRejectsOutOfRangeSizes()
    {
        CDlgResize dialog;
        auto ok = dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok);
        auto width = dialog.findChild<QLineEdit *>("eWidth");
        auto height = dialog.findChild<QLineEdit *>("eHeight");

        width->setText("64");
        height->setText("64");
        QVERIFY(ok->isEnabled());

        width->setText("4"); // below MIN_SIZE
        QVERIFY(!ok->isEnabled());

        width->setText("64");
        height->setText("300"); // above MAX_SIZE
        QVERIFY(!ok->isEnabled());
    }

    // --------------------------------------------------------------- DlgAbout

    void dlgAbout_constructs()
    {
        DlgAbout dialog;
        QVERIFY(!dialog.windowTitle().isEmpty());
    }

    // --------------------------------------------------------------- CDlgStat

    void dlgStat_constructsWithMapCoordinates()
    {
        CMap map(16, 16);
        CDlgStat dialog(&map, 0, 0);
        QVERIFY(dialog.findChildren<QWidget *>().size() > 1);
    }

    // -------------------------------------------------------------- DialogMaps

    void dlgMaps_startsClean()
    {
        CMapFile doc;
        DialogMaps dialog(&doc);
        QVERIFY(!dialog.isDirty());
    }

    // --------------------------------------------------- MapPropertiesDialog

    void mapPropertiesDialog_buildsTabs()
    {
        CMap map(16, 16);
        QUndoStack stack;
        MapPropertiesDialog dialog(&map, &stack);
        QVERIFY(dialog.findChild<QTabWidget *>() != nullptr);
    }

    // CTileBox is exercised in tst_local.cpp: it loads :/data/tiles.obl in its
    // constructor and this target does not compile the application resources.

    // ----------------------------------------------------------------- MapView

    void mapView_setMapAndZoom()
    {
        CMapFile doc;
        MapView view(nullptr, &doc);
        CMap *map = doc.map();

        view.setMap(map);
        QVERIFY(view.mapWidget()->map() == map);

        view.setZoom(2);
        QCOMPARE(view.zoom(), 2);

        view.zoomIn();
        QCOMPARE(view.zoom(), 3);

        view.setZoom(99); // clamped to 4
        QCOMPARE(view.zoom(), 4);

        view.setZoom(0); // clamped to 1
        QCOMPARE(view.zoom(), 1);
    }

    // --------------------------------------------------------------- MapWidget

    void mapWidget_toolNames()
    {
        QCOMPARE(MapWidget::toolName(ToolType::None), QString("None"));
        QCOMPARE(MapWidget::toolName(ToolType::Stamp), QString("Paint Stamp"));
        QCOMPARE(MapWidget::toolName(ToolType::Picker), QString("Picker"));
        QCOMPARE(MapWidget::toolName(ToolType::Selection), QString("Selection"));
        QCOMPARE(MapWidget::toolName(ToolType::Eraser), QString("Erase Tiles"));
        QCOMPARE(MapWidget::toolName(ToolType::Dice), QString("Random Tool"));
        QCOMPARE(MapWidget::toolName(ToolType::FloodFill), QString("FloodFill"));
    }

    void mapWidget_defaultsAndSelection()
    {
        CMap map(16, 16);
        MapWidget widget;
        widget.setMap(&map);
        QVERIFY(widget.map() == &map);
        QCOMPARE(widget.zoom(), 2);
        QVERIFY(!widget.isGridVisible());

        widget.setGridVisible(true);
        QVERIFY(widget.isGridVisible());

        widget.clearSelection();
        QVERIFY(widget.currentSelection().isEmpty());
    }

    // ------------------------------------------------------ TileSelectorWidget

    void tileSelector_startsEmpty()
    {
        TileSelectorWidget selector;
        QVERIFY(selector.getSelectedTiles().isEmpty());
        QVERIFY(selector.getSelectedStamp().tiles.empty());
    }

    // --------------------------------------------------------------- LayerDock

    void layerDock_constructsAndRefreshes()
    {
        CMap map(16, 16);
        QUndoStack stack;
        LayerDock dock(&map, &stack);
        QVERIFY(dock.findChildren<QListWidget *>().size() >= 1);

        dock.refreshList(&map); // must not crash on a map with one layer
        QVERIFY(!dock.windowTitle().isEmpty());
    }
};

QTEST_MAIN(TestGui)
#include "tst_gui.moc"
