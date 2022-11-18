#include "tilebox.h"
#include <stdint.h>
#include <QDebug>
#include <QImage>
#include <QPixmap>
#include <QLayout>
#include <QFrame>
#include <QGridLayout>
#include <QToolButton>
#include <QAction>
#include <QCloseEvent>
#include <QSpacerItem>
#include "qtgui/qthelper.h"
#include "shared/qtgui/qfilewrap.h"
#include "shared/FrameSet.h"
#include "shared/Frame.h"
#include "tilesdata.h"
#include "sprtypes.h"

#define MAX_WIDTH 150
#define MAX_HEIGHT 400
#define MAX_COLS 4

CTileBox::CTileBox(QWidget *parent) :
    QToolBox(parent)
{
    setupToolbox();
}

CTileBox::~CTileBox()
{
}

void CTileBox::setupToolbox(){
    setMinimumSize(QSize(MAX_WIDTH,MAX_HEIGHT));
    setMaximumSize(QSize(MAX_WIDTH,MAX_HEIGHT));

    QFileWrap file;
    if (file.open(":/data/tilestiny.obl", "rb")) {
        qDebug("reading tiles");
        CFrameSet fs;
        if (fs.extract(file)) {
            qDebug("exracted: %d", fs.getSize());
        }
        file.close();

        const char *labels[] = {
            "Background", "Essentials", "Walls",
            "Pickup", "Monsters", "Keys && doors"
        };
        uint8_t icons[] = {
            TILES_PLANTS, TILES_ANNIE2, TILES_WALLS93_2,
            TILES_CHEST, TILES_OCTOPUS, TILES_HEARTDOOR
        };

        enum {
            TAB_BACKGROUND,
            TAB_ESSENTIALS,
            TAB_WALLS,
            TAB_PICKUP,
            TAB_MONSTERS,
            TAB_LOCKS
        };

        const int tabCount = sizeof(icons);
        QWidget *w[tabCount];

        for (uint8_t i=0; i < tabCount; ++i) {
            w[i] = new QWidget(this);
            auto cw = w[i];
            auto layout = new QGridLayout(cw);
            layout->setAlignment(Qt::AlignTop);
            cw->setLayout(layout);
            auto icon = frame2icon(* fs[icons[i]]);
            this->addItem(w[i], icon, labels[i]);
        }

        const int tiles =  fs.getSize();
        int j;
        for (int i=0; i < tiles; ++i) {
             auto icon = frame2icon(* fs[i]);
             const TileDef def =  getTileDef(i);
             if (def.hidden) {
                 continue;
             }
             switch (def.type) {
             case TYPE_BACKGROUND:
             case TYPE_STOP:
             case TYPE_SWAMP:
                 j = TAB_BACKGROUND;
             break;
             case TYPE_PLAYER:
             case TYPE_DIAMOND:
                 j = TAB_ESSENTIALS;
             break;
             case TYPE_DOOR:
             case TYPE_KEY:
                 j = TAB_LOCKS;
             break;
             case TYPE_DRONE:
             case TYPE_MONSTER:
             case TYPE_VAMPLANT:
                 j = TAB_MONSTERS;
             break;
             case TYPE_PICKUP:
                 j = TAB_PICKUP;
             break;
             case TYPE_WALLS:
                 j = TAB_WALLS;
             break;
             default:
                 j = TAB_BACKGROUND;
             };

             auto gridLayout = reinterpret_cast<QGridLayout*>(w[j]->layout());
             int count = gridLayout->count();
             auto btn = new QToolButton(this);
             auto action = new QAction(icon, def.basename, this);
             action->setData(i);
             gridLayout->addWidget(btn, count / MAX_COLS, count % MAX_COLS);
             btn->setDefaultAction(action);
             connect(btn, SIGNAL(triggered(QAction *)), this, SLOT(buttonPressed(QAction *)));
        }
    }
}

void CTileBox::buttonPressed(QAction *action)
{
    int tile = action->data().toInt();
    emit tileChanged(tile);
}
