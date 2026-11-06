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
#include "runtime/tilesdefs.h"

#define ALPHA 0xff000000
#define BLACK 0xff000000

QString buildReport(CMapFile &mf)
{
    QString content;
    const int BUFSIZE = 4095;
    char buf[BUFSIZE + 1];
    char *tmp = buf;
    auto append = [&content](const char *s)
    {
        content += s;
    };
    auto writeItem = [&content, tmp](auto str, auto v)
    {
        snprintf(tmp, BUFSIZE + 1, "  -- %-15s: %d\n", str, static_cast<int>(v));
        content += tmp;
    };

    std::unordered_map<uint16_t, std::string> labels;
    const auto &keyOptions = getKeyOptions();
    for (const auto &[v, k] : keyOptions)
    {
        labels[k] = v;
    }

    typedef std::unordered_map<uint8_t, uint32_t> StatMap;

    append("Map List\n");
    append("========\n\n");

    for (size_t i = 0; i < mf.size(); ++i)
    {
        CMap *map = mf.at(i);
        snprintf(tmp, BUFSIZE + 1, "Level %.2lu: %s\n", i + 1, map->title());
        append(tmp);
    }

    append("\n");
    append("MapArch statistics\n");
    append("==================\n\n");

    StatMap globalUsage;

    for (size_t i = 0; i < mf.size(); ++i)
    {
        CMap *map = mf.at(i);
        MapReport report = CGame::generateMapReport(*map);
        StatMap usage;
        int monsters = 0;
        int stops = 0;
        for (int y = 0; y < map->height(); ++y)
        {
            for (int x = 0; x < map->width(); ++x)
            {
                const auto &c = map->at(x, y);
                ++usage[c];
                ++globalUsage[c];
                auto &def = getTileDef(c);
                if (def.type == TYPE_MONSTER || def.type == TYPE_VAMPLANT)
                {
                    ++monsters;
                }
                if (def.type == TYPE_STOP)
                {
                    ++stops;
                }
            }
        }
        snprintf(tmp, BUFSIZE + 1, "Level %.2lu: %s\n", i + 1, map->title());
        append(tmp);
        writeItem("Unique tiles", usage.size());
        writeItem("Monsters", monsters);
        writeItem("Attributes", map->attrs().size());
        writeItem("Stops", stops);
        snprintf(tmp, BUFSIZE + 1, "  -- Size: %d x %d\n", map->width(), map->height());
        append(tmp);
        writeItem("fruits", report.fruits);
        writeItem("treasures", report.bonuses);
        writeItem("secrets", report.secrets);

        CStates &states = map->states();
        std::vector<StateValuePair> pairs = states.getValues();
        if (pairs.size())
        {
            append("\nMeta-data\n");
            for (const auto &item : pairs)
            {
                const bool isStr = (item.key & 0xff) >= 0x80;
                const std::string label = labels[item.key];
                snprintf(tmp, BUFSIZE + 1, "  -- %-12s %s", label.c_str(), isStr ? item.value.c_str() : item.tip.c_str());
                content += tmp;
                if (isStr)
                    append("\n");
                else
                {
                    snprintf(tmp, BUFSIZE + 1, " [%s]\n", item.value.c_str());
                    content += tmp;
                }
            }
        }
        append("\n-----------------------------------\n");
        append("\n");
    }

    append("Par time\n");
    append("==================\n\n");
    for (size_t i = 0; i < mf.size(); ++i)
    {
        CMap *map = mf.at(i);
        CStates &states = map->states();
        uint16_t parTime = states.getU(PAR_TIME);
        if (parTime == 0)
            continue;
        snprintf(tmp, BUFSIZE + 1, "Level %.2lu: %s\n", i + 1, map->title());
        append(tmp);
        const int seconds = parTime % 60;
        const int minutes = parTime / 60;
        snprintf(tmp, BUFSIZE + 1, "   PAR TIME:   %.2d:%.2d\n\n", minutes, seconds);
        append(tmp);
    }

    snprintf(tmp, BUFSIZE + 1, "\nGlobal Unique tiles: %lu\n", globalUsage.size());
    append(tmp);
    return content;
}

bool generateReport(CMapFile &mf, const QString &filename)
{
    const QByteArray data = buildReport(mf).toUtf8();
    QFileWrap file;
    if (!file.open(filename, "wb"))
    {
        return false;
    }
    file.write(data.constData(), static_cast<int>(data.size()));
    file.close();
    return true;
}

void generateScreenshot(const QString &filename, CMap *map, const int maxRows, const int maxCols)
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

    const int tileSize = 16;
    const int lineSize = maxCols * tileSize;
    CFrame bitmap(maxCols * tileSize, maxRows * tileSize);
    bitmap.fill(BLACK);
    uint32_t *rgba = bitmap.getRGB().data();
    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            uint8_t tile = map->at(col + mx, row + my);
            CFrame *frame = (*fs)[tile];
            for (int y = 0; y < tileSize; ++y)
            {
                for (int x = 0; x < tileSize; ++x)
                {
                    rgba[x + col * tileSize + y * lineSize + row * tileSize * lineSize] = frame->at(x, y) | ALPHA;
                }
            }
        }
    }
    bitmap.enlarge();
    // uint8_t *png;
    // int size;
    std::vector<uint8_t> png;
    bitmap.toPng(png);
    if (file.open(filename.toStdString().c_str(), "wb"))
    {
        file.write(png.data(), png.size());
        file.close();
    }

    delete fs;
}
