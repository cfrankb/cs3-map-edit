#include <QVector>
#include "runtime/maparch.h"
#include "mapfile.h"
#include "runtime/map.h"
#include "unordered_map"
#include "runtime/shared/qtgui/qfilewrap.h"
#include <stdint.h>
#include "runtime/tilesdata.h"
#include "runtime/sprtypes.h"
#include "runtime/states.h"
#include "runtime/statedata.h"
#include "runtime/shared/FrameSet.h"
#include "runtime/shared/Frame.h"
#include "runtime/game.h"

#define ALPHA 0xff000000
#define BLACK 0xff000000

bool generateReport(CMapFile & mf, const QString & filename) {
    QFileWrap file;
    const int BUFSIZE = 4095;
    char *tmp = new char[BUFSIZE + 1];
    auto writeItem = [&file, tmp](auto str, auto v) {
        sprintf(tmp, "  -- %-15s: %d\n", str, static_cast<int>(v));
        file += tmp;
    };

    std::unordered_map<uint16_t, std::string> labels;
    const auto & keyOptions = getKeyOptions();
    for (const auto &[v,k]: keyOptions) {
        labels[k] =v;
    }

    typedef std::unordered_map<uint8_t, uint32_t>  StatMap;
    if (!file.open(filename, "wb")) {
        delete []tmp;
        return false;
    }

    file += "Map List\n";
    file += "========\n\n";

    for (size_t i=0; i < mf.size(); ++i) {
        CMap *map = mf.at(i);
        sprintf(tmp, "Level %.2lu: %s\n", i + 1, map->title());
        file += tmp;
    }

    file += "\n";
    file += "MapArch statistics\n";
    file += "==================\n\n";

    StatMap globalUsage;

    for (size_t i=0; i < mf.size(); ++i) {
        CMap *map = mf.at(i);
        MapReport report = CGame::generateMapReport(*map);
        StatMap usage;
        int monsters=0;
        int stops = 0;
        for (int y=0;y < map->hei(); ++y) {
            for (int x=0;x < map->len(); ++x) {
                const auto &c = map->at(x,y);
                ++usage[c];
                ++globalUsage[c];
                auto & def =getTileDef(c);
                if (def.type == TYPE_MONSTER || def.type == TYPE_VAMPLANT) {
                    ++monsters;
                }
                if (def.type == TYPE_STOP) {
                    ++stops;
                }
            }
        }
        sprintf(tmp, "Level %.2lu: %s\n", i + 1, map->title());
        file += tmp;
        writeItem("Unique tiles", usage.size());
        writeItem("Monsters", monsters);
        writeItem("Attributes", map->attrs().size());
        writeItem("Stops", stops);
        sprintf(tmp, "  -- Size: %d x %d\n", map->len(), map->hei());
        file += tmp;
        writeItem("fruits", report.fruits);
        writeItem("treasures", report.bonuses);
        writeItem("secrets", report.secrets);

        CStates &states = map->states();
        std::vector<StateValuePair> pairs = states.getValues();
        if (pairs.size()) {
            file += "\nMeta-data\n";
            for (const auto&item : pairs) {
                const bool isStr = (item.key & 0xff) >= 0x80;
                const std::string label = labels[item.key];
                sprintf(tmp, "  -- %-12s %s", label.c_str(), isStr ? item.value.c_str() : item.tip.c_str());
                file += tmp;
                if (isStr)
                    strncpy(tmp, "\n", BUFSIZE);
                else
                    sprintf(tmp, " [%s]\n", item.value.c_str());
                file += tmp;
            }
        }
        file += "\n-----------------------------------\n";
        file += "\n";
    }

    file += "Par time\n";
    file += "==================\n\n";
    for (size_t i=0; i < mf.size(); ++i) {
        CMap *map = mf.at(i);
        CStates &states = map->states();
        uint16_t parTime= states.getU(PAR_TIME);
        if (parTime == 0)
            continue;
        sprintf(tmp, "Level %.2lu: %s\n", i + 1, map->title());
        file += tmp;
        const int seconds = parTime % 60;
        const int minutes = parTime / 60;
        sprintf(tmp, "   PAR TIME:   %.2d:%.2d\n\n", minutes , seconds);
        file += tmp;
    }

    sprintf(tmp, "\nGlobal Unique tiles: %lu\n", globalUsage.size());
    file += tmp;
    file.close();
    delete []tmp;
    return true;
}


void generateScreenshot(const QString &filename, CMap *map, const int maxRows, const int maxCols)
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
    CFrame bitmap(maxCols * tileSize, maxRows *tileSize);
    bitmap.fill(BLACK);
    uint32_t *rgba = bitmap.getRGB().data();
    for (int row=0; row < rows; ++row) {
        for (int col=0; col < cols; ++col) {
            uint8_t tile = map->at(col + mx, row +my);
            CFrame *frame = (*fs)[tile];
            for (int y=0; y < tileSize; ++y) {
                for (int x=0; x < tileSize; ++x) {
                    rgba[x + col*tileSize+ y * lineSize + row * tileSize*lineSize] = frame->at(x,y) | ALPHA;
                }
            }
        }
    }
    bitmap.enlarge();
    //uint8_t *png;
    //int size;
    std::vector<uint8_t> png;
    bitmap.toPng(png);
    if (file.open(filename.toStdString().c_str(), "wb")) {
        file.write(png.data(), png.size());
        file.close();
    }

    delete fs;
}
