#include "maparch.h"
#include "mapfile.h"
#include "map.h"
#include "unordered_map"
#include "shared/qtgui/qfilewrap.h"
#include <stdint.h>
#include "tilesdata.h"
#include "sprtypes.h"

bool generateReport(CMapFile & mf, const QString & filename) {

    typedef std::unordered_map<uint8_t, uint32_t>  StatMap;
    char tmp[256];
    QFileWrap file;
    if (!file.open(filename, "wb")) {
        return false;
    }

    file += "Map List\n";
    file += "========\n\n";


    for (int i=0; i < mf.size(); ++i) {
        CMap *map = mf.at(i);
        sprintf(tmp, "Level %.2d: %s\n", i + 1, map->title());
        file += tmp;
    }

    file += "\n";
    file += "MapArch statistics\n";
    file += "==================\n\n";


    StatMap globalUsage;

    for (int i=0; i < mf.size(); ++i) {
        CMap *map = mf.at(i);
        StatMap usage;
        int monsters=0;
        for (int y=0;y < map->hei(); ++y) {
            for (int x=0;x < map->len(); ++x) {
                auto &c = map->at(x,y);
                ++usage[c];
                ++globalUsage[c];
                auto & def =getTileDef(c);
                if (def.type == TYPE_MONSTER || def.type == TYPE_VAMPLANT) {
                    ++monsters;
                }
            }
        }
        sprintf(tmp, "Level %.2d: %s\n", i + 1, map->title());
        file += tmp;
        sprintf(tmp, "  -- Unique tiles: %lu\n", usage.size());
        file += tmp;
        sprintf(tmp, "  -- Monsters: %d\n", monsters);
        file += tmp;
        sprintf(tmp, "  -- Attributs: %lu\n", map->attrs().size());
        file += tmp;
        sprintf(tmp, "  -- Size: %d x %d\n", map->len(), map->hei());
        file += tmp;
        file += "\n";
    }

    sprintf(tmp, "\nGlobal Unique tiles: %lu\n", globalUsage.size());
    file += tmp;

    file.close();

    return true;

}
