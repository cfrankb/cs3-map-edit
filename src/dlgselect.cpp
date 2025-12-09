#include "dlgselect.h"
#include "ui_dlgselect.h"
#include "runtime/shared/qtgui/qfilewrap.h"
#include "runtime/shared/FrameSet.h"
#include "runtime/shared/Frame.h"
#include "runtime/shared/qtgui/qthelper.h"
#include "runtime/map.h"
#include "mapfile.h"
#include "runtime/tilesdata.h"
#include "runtime/states.h"
#include "runtime/statedata.h"
#include "runtime/dirs.h"
#include "runtime/shared/PngMagic.h"

CDlgSelect::CDlgSelect(QWidget *parent) : QDialog(parent),
                                          ui(new Ui::CDlgSelect)
{
    ui->setupUi(this);
    m_frameSetMain = preloadMainTiles();
    m_frameSetLayers = preloadLayerTiles();
    m_mapFile = nullptr;
}

CDlgSelect::~CDlgSelect()
{
    delete ui;
    delete m_frameSetMain;
    delete m_frameSetLayers;
}

void CDlgSelect::updatePreview(CMap *map)
{
    const int maxRows = 16;
    const int maxCols = 16;
    const int rows = std::min(maxRows, map->height());
    const int cols = std::min(maxCols, map->width());

    CStates &states = map->states();
    const uint16_t startPos = states.getU(POS_ORIGIN);

    const Pos pos = startPos != 0 ? CMap::toPos(startPos) : map->findFirst(TILES_ANNIE2);
    const bool isFound = pos.x != CMap::NOT_FOUND || pos.y != CMap::NOT_FOUND;
    const int lmx = std::max(0, isFound ? pos.x - cols / 2 : 0);
    const int lmy = std::max(0, isFound ? pos.y - rows / 2 : 0);
    const int mx = std::min(lmx, map->width() > cols ? map->width() - cols : 0);
    const int my = std::min(lmy, map->height() > rows ? map->height() - rows : 0);

    const int tileSize = TILE_SIZE;
    const int lineSize = maxCols * tileSize;
    CFrame bitmap(maxCols * tileSize, maxRows * tileSize);
    bitmap.fill(BLACK);
    uint32_t *rgba = bitmap.getRGB().data();

    int layerCount  = (int) map->layerCount();
    for (int layerID = layerCount -1; layerID >= 0 ; --layerID) {
        CLayer *layer = map->getLayer(layerID);
        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                uint8_t tile = layer->at(col + mx, row + my);
                if (!tile) continue;
                CFrame *frame =  layerID == 0 ? (*m_frameSetMain)[tile] : (*m_frameSetLayers)[tile];
                for (int y = 0; y < tileSize; ++y)
                {
                    for (int x = 0; x < tileSize; ++x)
                    {
                        if (frame->at(x, y) & ALPHA)
                            rgba[x + col * tileSize + y * lineSize + row * tileSize * lineSize] = frame->at(x, y) | ALPHA;
                    }
                }
            }
        }
    }

    // bitmap.shrink();
    QPixmap pixmap = frame2pixmap(bitmap);
    ui->sPreview->setPixmap(pixmap);
}

CFrameSet *CDlgSelect::preloadMainTiles()
{
    CFrameSet *fs = new CFrameSet();
    QFileWrap file;
    if (file.open(":/data/tiles.obl", "rb"))
    {
        qDebug("reading tiles");
        if (fs->extract(file))
        {
            qDebug("extracted: %lu", fs->getSize());
        }
        file.close();
    }

     return fs;
}

CFrameSet *CDlgSelect::preloadLayerTiles()
{
    QFileWrap file;
    const std::string cs3tiles0Filename = ":/data/cs3layers.png";
    LOGI("extracting texture from %s", cs3tiles0Filename.c_str());
    if (!file.open(cs3tiles0Filename, "rb"))
    {
        LOGE("can't open %s", cs3tiles0Filename.c_str());
        return nullptr;
    }

    CFrameSet set;
    if (!parsePNG(set, file, 0, true))
    {
        LOGE("fail to parse %s", cs3tiles0Filename.data());
        return nullptr;
    }
    file.close();

    if (set.getSize() == 0)
        return nullptr;

    return set[0]->split(TILE_SIZE, TILE_SIZE);
}

void CDlgSelect::init(const QString s, CMapFile *mf)
{
    m_mapFile = mf;
    ui->sSelect_Maps->setText(s);
    QStringList list;
    for (size_t i = 0; i < mf->size(); ++i)
    {
        list.append(tr("map %1 : %2").arg(i + 1, 2, 10, QChar('0')).arg(mf->at(i)->title()));
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
