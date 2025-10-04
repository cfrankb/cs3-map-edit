#include "dlgselect.h"
#include "ui_dlgselect.h"
#include "shared/qtgui/qfilewrap.h"
#include "shared/FrameSet.h"
#include "shared/Frame.h"
#include "shared/qtgui/qthelper.h"
#include "map.h"
#include "mapfile.h"
#include "tilesdata.h"
#include "states.h"
#include "statedata.h"

CDlgSelect::CDlgSelect(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CDlgSelect)
{
    ui->setupUi(this);
    m_frameSet = preloadTiles();
    m_mapFile = nullptr;
}

CDlgSelect::~CDlgSelect()
{
    delete ui;
    delete m_frameSet;
}

void CDlgSelect::updatePreview(CMap *map)
{
    const int maxRows = 16;
    const int maxCols = 16;
    const int rows = std::min(maxRows, map->hei());
    const int cols = std::min(maxCols, map->len());

    CStates & states = map->states();
    const uint16_t startPos = states.getU(POS_ORIGIN);

    const Pos pos = startPos !=0 ? CMap::toPos(startPos): map->findFirst(TILES_ANNIE2);
    const bool isFound = pos.x != CMap::NOT_FOUND || pos.y != CMap::NOT_FOUND;
    const int lmx = std::max(0, isFound? pos.x - cols / 2 : 0);
    const int lmy = std::max(0, isFound? pos.y - rows / 2 : 0);
    const int mx = std::min(lmx, map->len() > cols ? map->len() - cols : 0);
    const int my = std::min(lmy, map->hei() > rows ? map->hei() - rows : 0);

    const int tileSize = 16;
    const int lineSize = maxCols * tileSize;
    CFrameSet & fs = *m_frameSet;
    CFrame bitmap(maxCols * tileSize, maxRows *tileSize);
    bitmap.fill(BLACK);
    uint32_t *rgba = bitmap.getRGB().data();
    for (int row=0; row < rows; ++row) {
        for (int col=0; col < cols; ++col) {
            uint8_t tile = map->at(col + mx, row +my);
            CFrame *frame = fs[tile];
            for (int y=0; y < tileSize; ++y) {
                for (int x=0; x < tileSize; ++x) {
                    rgba[x + col*tileSize+ y * lineSize + row * tileSize*lineSize] = frame->at(x,y) | ALPHA;
                }
            }
        }
    }

   // bitmap.shrink();
    QPixmap pixmap = frame2pixmap(bitmap);
    ui->sPreview->setPixmap(pixmap);
}

CFrameSet *CDlgSelect::preloadTiles()
{
    CFrameSet * fs = new CFrameSet();
    QFileWrap file;
    if (file.open(":/data/tiles.obl", "rb")) {
        qDebug("reading tiles");
        if (fs->extract(file)) {
            qDebug("extracted: %lu", fs->getSize());
        }
        file.close();
    }
    return fs;
}

void CDlgSelect::init(const QString s, CMapFile *mf)
{
    m_mapFile = mf;
    ui->sSelect_Maps->setText(s);
    QStringList list;
    for (int i=0; i < mf->size(); ++i) {
        list.append(tr("map %1 : %2").arg(i+1,2,10,QChar('0')).arg(mf->at(i)->title()));
    }
    ui->cbSelect_Maps->addItems(list);
    ui->cbSelect_Maps->setCurrentIndex(mf->currentIndex());
}

void CDlgSelect::on_cbSelect_Maps_currentIndexChanged(int index)
{
    updatePreview(m_mapFile->at(index));
}

int CDlgSelect::index()
{
    return ui->cbSelect_Maps->currentIndex();
}
