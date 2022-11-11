#include "level.h"
#include "map.h"
#include <cstring>
#include "../../../shared/FileWrap.h"
#include "tilesdata.h"

std::string str2upper(const std::string in)
{
    char t[in.length() + 1];
    strcpy(t, in.c_str());
    char *p = t;
    while (*p)
    {
        if (*p == '.')
        {
            break;
        }
        *p = toupper(*p);
        ++p;
    }
    // out = t;
    return t;
}

void splitString(const std::string str, StringVector &list)
{
    int i = 0;
    int j = 0;
    while (j < str.length())
    {
        // printf("@@@@%s %d %d\n", str.c_str(), j, str.length());
        if (isspace(str[j]))
        {
            //   printf("x%d\n", j);
            list.push_back(str.substr(i, j - i));
            while (isspace(str[j]) && j < str.length())
            {
                ++j;
            }
            i = j;
            continue;
        }
        ++j;
    }
    list.push_back(str.substr(i, j - i));
}

uint8_t *readFile(const char *fname)
{
    CFileWrap sfile;
    uint8_t *data = nullptr;
    if (sfile.open(fname, "rb"))
    {
        int size = sfile.getSize();
        data = new uint8_t[size + 1];
        data[size] = 0;
        sfile.read(data, size);
        sfile.close();
    }
    else
    {
        printf("failed to read:%s\n", fname);
    }
    return data;
}

bool getChMap(const char *mapFile, char *chMap)
{
    uint8_t *data = readFile("out/tiles.map");
    if (data == nullptr)
    {
        printf("cannot read tiles.map\n");
        return false;
    }

    char *p = reinterpret_cast<char *>(data);
    printf("parsing tiles.map: %d\n", strlen(p));
    int i = 0;

    while (p && *p)
    {
        char *n = strstr(p, "\n");
        if (n)
        {
            *n = 0;
            ++n;
        }
        StringVector list;
        splitString(std::string(p), list);
        u_int8_t ch = std::stoi(list[3], 0, 16);
        p = n;
        chMap[ch] = i;
        ++i;
    }

    delete[] data;
    return true;
}

bool processLevel(CMap &map, const char *fname, const char *chMap)
{
    uint8_t *data = readFile(fname);
    if (data == nullptr)
    {
        delete[] data;
        printf("failed read: %d\n", fname);
        return false;
    }

    // get level size
    char *ps = reinterpret_cast<char *>(data);
    int maxRows = 0;
    int maxCols = 0;
    while (ps)
    {
        ++maxRows;
        char *u = strstr(ps, "\n");
        if (u)
        {
            *u = 0;
            maxCols = std::max(static_cast<int>(strlen(ps) + 1), maxCols);
            *u = '\n';
        }

        ps = u ? u + 1 : nullptr;
    }
    printf("maxRows: %d, maxCols:%d\n", maxRows, maxCols);

    map.resize(maxCols, maxRows);
    map.clear();

    // convert ascii to map
    uint8_t *p = data;
    int x = 0;
    int y = 0;
    while (*p)
    {
        uint8_t c = *p;
        ++p;
        if (c == '\n')
        {
            ++y;
            x = 0;
            continue;
        }
        uint8_t m = chMap[c];
        map.set(x, y, m);
        ++x;
    }
    delete[] data;
    return true;
}

const uint8_t convTable[] = {
    TILES_BLANK,
    TILES_WALL_BRICK,
    TILES_ANNIE2,
    TILES_STOP,
    TILES_DIAMOND,
    TILES_NECKLESS,
    TILES_CHEST,
    TILES_TRIFORCE,
    TILES_BOULDER,
    TILES_HEARTKEY,
    TILES_HEARTDOOR,
    TILES_GRAYKEY,
    TILES_GRAYDOOR,
    TILES_REDKEY,
    TILES_REDDOOR,
    TILES_POPKEY,
    TILES_POPDOOR,
    TILES_WALLS93,
    TILES_WALLS93_2,
    TILES_WALLS93_3,
    TILES_WALLS93_4,
    TILES_WALLS93_5,
    TILES_WALLS93_6,
    TILES_WALLS93_7,
    TILES_FLOWERS_2,
    TILES_PINETREE,
    TILES_ROCK1,
    TILES_ROCK2,
    TILES_ROCK3,
    TILES_TOMB,
    TILES_SWAMP,
    TILES_VAMPLANT,
    TILES_INSECT1,
    TILES_TEDDY93,
    TILES_OCTOPUS,
    TILES_BLANK,
    TILES_BLANK,
    TILES_BLANK,
    TILES_DIAMOND,
    TILES_WALLS93_2,
    TILES_DIAMOND,
    TILES_WALLS93_2,
    TILES_DIAMOND,
    TILES_WALLS93_2,
    TILES_DIAMOND,
    TILES_BLANK,
    TILES_DIAMOND,
    TILES_BLANK,
    TILES_DIAMOND,
    TILES_BLANK};

bool convertCs3Level(CMap &map, const char *fname)
{
    uint8_t *data = readFile(fname);
    if (data == nullptr)
    {
        delete[] data;
        printf("failed read: %d\n", fname);
        return false;
    }

    map.clear();
    map.resize(64, 64);
    uint8_t *p = data + 7;
    for (int y = 0; y < 64; ++y)
    {
        for (int x = 0; x < 64; ++x)
        {
            uint8_t m = 0;
            map.set(x, y, convTable[m]);
            ++p;
        }
    }

    delete[] data;
    return true;
}
