#include "dlgstat.h"
#include "ui_dlgstat.h"
#include "runtime/tilesdata.h"
#include "runtime/sprtypes.h"
#include <qlistwidget.h>
#include <qstringlist.h>
#include "runtime/tilesdebug.h"
#include "runtime/attr.h"
#include "runtime/tilesdefs.h"

CDlgStat::CDlgStat(const uint8_t tileID, const uint8_t attr, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CDlgStat)
{
    ui->setupUi(this);

    const TileDef & def = getTileDef(tileID);
    ui->sName->setText(def.basename);
    QListWidget * w = ui->listWidget;

    w->addItem(QString("%1 [0x%2]").arg(get_tileNames(tileID).c_str()).arg(tileID,2, 16, QChar('0')));
    if (def.hidden) {
        w->addItem(tr("hidden"));
    }

    if (def.ai) {
        auto const &map = get_aiNamesMap();
        for (const auto & [val, name]: map) {
            if ((def.ai & val) == val) {
                w->addItem(name.c_str());
            }
        }
    }

    if (def.flags) {
        auto const &map = get_flagsNamesMap();
        for (const auto & [val, name]: map) {
            if ((def.flags & val) == val) {
                w->addItem(name.c_str());
            }
        }
    }

    if (def.health) {
        QString s = tr("HEALTH: %1%2 hp").arg(def.health < 0 ? "-" : "").arg(def.health);
        w->addItem(s);
    }

    if (def.score) {
        w->addItem(tr("SCORE: +%1").arg(def.score));
    }

    if (def.speed) {
        QString s = get_speedsNames(def.speed).c_str();
        if (s.isEmpty())
            s = QString(tr("SPEED %1")).arg(def.speed);
        w->addItem(s);
    }

    if (attr) {
        QString s = tr("ATTR: %1 [0x%2]").arg(attr2text(attr)).arg(attr,2, 16, QChar('0'));
        w->addItem(s);
    }

    ui->label->setText(get_typesNames(def.type).c_str());
}

QString CDlgStat::attr2text(const uint8_t attr) {
    if (RANGE(attr, ATTR_MSG_MIN, ATTR_MSG_MAX)) {
        return "Message";
    } else if (RANGE(attr, ATTR_IDLE_MIN, ATTR_IDLE_MAX)) {
        return tr("Wait");
    } else if (attr == ATTR_FREEZE_TRAP) {
        return tr("Freeze Trap");
    } else if (attr == ATTR_TRAP) {
        return tr("Trap");
    } else if (RANGE(attr, ATTR_CRUSHER_MIN, ATTR_CRUSHER_MAX)) {
        return tr("Crusher");
    } else if (RANGE(attr, ATTR_BOSS_MIN, ATTR_BOSS_MAX)) {
        return tr("Boss Spawn Point");
    } else if (attr > PASSAGE_ATTR_MAX) {
        return tr("undefined behavior"); // undefined behavior
    } else if (RANGE(attr, SECRET_ATTR_MIN, SECRET_ATTR_MAX)) {
        return tr("secret");
    } else if (RANGE(attr, PASSAGE_REG_MIN, PASSAGE_REG_MAX)) {
        return tr("passage");
    } else {
        return tr("???");
    }
}

CDlgStat::~CDlgStat()
{
    delete ui;
}
