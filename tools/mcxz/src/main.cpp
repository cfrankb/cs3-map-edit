#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <unordered_map>
#include <map>
#include <vector>
#include "zlib.h"
#include "../../../shared/FileWrap.h"
#include "../../../shared/FrameSet.h"
#include "../../../shared/Frame.h"
#include "tileset.h"

typedef std::vector<std::string> StringVector;

typedef struct
{
    uint8_t id;
    uint8_t ch;
    std::string name;     // WALLS93
    uint8_t type;         // 0x03
    std::string typeName; // TYPE_WALLS
    std::string define;   // TILES_WALLS93
    std::string basename; // walls93.obl
    uint8_t score;
    int8_t health;
    bool hidden;
} Tile;

typedef std::vector<Tile> TileVector;
typedef std::vector<std::string> StrVector;
typedef std::map<std::string, StringVector> Config;
typedef std::unordered_map<std::string, uint8_t> StrVal;

void splitString(const std::string str, StringVector &list)
{
    int i = 0;
    unsigned int j = 0;
    while (j < str.length())
    {
        if (isspace(str[j]))
        {
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
    return t;
}

uint16_t rgb888torgb565(uint8_t *rgb888Pixel)
{
    uint8_t red = rgb888Pixel[0];
    uint8_t green = rgb888Pixel[1];
    uint8_t blue = rgb888Pixel[2];

    uint16_t b = (blue >> 3) & 0x1f;
    uint16_t g = ((green >> 2) & 0x3f) << 5;
    uint16_t r = ((red >> 3) & 0x1f) << 11;

    return (uint16_t)(r | g | b);
}

int test()
{
    CFileWrap tfileTiny;
    tfileTiny.open("out/test_output.obl", "wb");

    CFileWrap sfile;
    if (sfile.open("obl5/annie2.obl", "rb"))
    {
        CFrameSet images;
        if (images.extract(sfile))
        {
            printf("images:%d\n", images.getSize());
        }
        int count = images.getSize();

        CFrameSet imagesTiny;
        for (int i = 0; i < count; ++i)
        {
            CFrame *s = images[i];
            CFrame *t = new CFrame;
            t->copy(s);
            t->resize(32, 32);
            int shiftRight = (32 - s->m_nLen) / 2;
            for (int j = 0; j < shiftRight; ++j)
            {
                t->shiftRIGHT();
            }
            int shiftDown = (32 - s->m_nHei) / 2;
            for (int j = 0; j < shiftDown; ++j)
            {
                t->shiftDOWN();
            }
            t->shrink();
            imagesTiny.add(t);
        }

        CTileSet tiles(16, 16, count);
        for (int i = 0; i < count; ++i)
        {
            printf("image:%d\n", i);
            CFrame *frame = imagesTiny[i];
            u_int32_t *rgb888 = frame->getRGB();
            int pixels = frame->m_nLen * frame->m_nHei;
            uint16_t rgb565[pixels];

            for (int j = 0; j < pixels; ++j)
            {
                rgb565[j] = rgb888torgb565(reinterpret_cast<uint8_t *>(&rgb888[j]));
            }
            tiles.set(i, rgb565);
        }

        imagesTiny.write(tfileTiny);
        tiles.write("out/test_output.mcz");
    }
    else
    {
        puts("failed to open");
    }

    return EXIT_SUCCESS;
}

bool parseConfig(Config &conf, StrVector &list, FILE *sfile)
{
    fseek(sfile, 0, SEEK_END);
    long size = ftell(sfile);
    char buf[size + 1];
    buf[size] = 0;
    fseek(sfile, 0, SEEK_SET);
    fread(buf, size, 1, sfile);
    fclose(sfile);
    char *s = buf;

    int line = 0;
    char *sName = nullptr;
    while (s && *s)
    {
        ++line;
        // find next line
        char *n = strstr(s, "\n");
        char *b = nullptr;
        if (n)
        {
            // truncate line on return char
            *n = 0;
            b = n - 1;
            ++n;
        }

        // remove comment
        char *c = strstr(s, "#");
        if (c)
        {
            *c = 0;
            if (c < b)
            {
                b = c - 1;
            }
        }

        // trim right
        while (b >= s && isspace(*b))
        {
            *b = 0;
            --b;
        }

        // trim left
        while (isspace(*s))
        {
            ++s;
        }

        if (*s != '\0')
        {
            if (*s == '[')
            {
                sName = s + 1;
                // section header
                char *se = strstr(s, "]");
                if (se)
                {
                    *se = 0;
                    list.push_back(sName);
                }
                else
                {
                    printf("section head `%s` on line %d missing right bracket.\n", s, line);
                }
            }
            else
            {
                if (sName != nullptr)
                {
                    conf[sName].push_back(s);
                }
                else
                {
                    printf("file def on line %d without section.\n", line);
                }
            }
        }
        s = n;
    }

    return true;
}

bool processTypes(StringVector &files, StrVal &typeMap)
{
    CFileWrap tfile;
    const char *fname = "out/sprtypes.h";
    bool result = tfile.open(fname, "wb");
    if (result)
    {
        tfile += "//////////////////////////////////////////////////\n"
                 "// autogenerated\n\n"
                 "#ifndef _SPRTYPES__H\n"
                 "#define _SPRTYPES__H\n\n"
                 "#define TYPE_NONE 0xff\n"
                 "\n";
    }
    else
    {
        printf("cannot create %s\n", fname);
    }

    for (int i = 0; i < files.size(); ++i)
    {
        StringVector list;
        splitString(files[i], list);

        std::string key = list[0];
        if (list.size() < 2)
        {
            printf("missing second value for: %s\n", key.c_str());
            continue;
        }
        u_int8_t val = std::stoi(list[1], 0, 16);
        typeMap[key] = val;

        if (result)
        {
            char tmp[128];
            sprintf(tmp, "#define TYPE_%-20s0x%.2x\n", key.c_str(), val);
            tfile += tmp;
        }
    }

    if (result)
    {
        tfile += "\n#endif\n";
        tfile.close();
    }

    return true;
}

std::string formatTitleName(const char *src)
{
    char t[strlen(src) + 1];
    strcpy(t, src);
    char *p = t;
    char *bn;
    while (bn = strstr(p, "/"))
    {
        p = bn + 1;
    }
    char *d = strstr(p, ".");
    if (d)
    {
        *d = 0;
    }
    return str2upper(p);
}

std::string getBasename(const char *filepath)
{
    char tmp[strlen(filepath) + 1];
    strcpy(tmp, filepath);
    char *p = tmp;
    char *bn = p;
    while (p = strstr(p, "/"))
    {
        bn = ++p;
    }
    return bn;
}

void generateHeader(const std::string section, const std::string sectionName, std::string const sectionBasename, TileVector &tileDefs, bool useTileDefs)
{
    char fnameHdr[section.length() + strlen("data.h") + 1];
    sprintf(fnameHdr, "%s%s", section.c_str(), "data.h");

    char fnameData[section.length() + strlen("data.cpp") + 1];
    sprintf(fnameData, "%s%s", section.c_str(), "data.cpp");

    CFileWrap tfileHdr;
    tfileHdr.open(fnameHdr, "wb");

    char thead[512];

    //////////////////////////////////////////////////////////////////////
    // file header

    sprintf(thead, "//////////////////////////////////////////////////\n"
                   "// autogenerated\n\n"
                   "#ifndef _%s__HDR_H\n"
                   "#define _%s__HDR_H\n\n",

            sectionName.c_str(),
            sectionName.c_str());
    tfileHdr += thead;

    if (useTileDefs)
    {
        tfileHdr += "#include <stdint.h>\n\n"
                    "typedef struct\n"
                    "{\n"
                    "    uint8_t ch;\n"
                    "    uint8_t type;\n"
                    "    uint8_t score;\n"
                    "    int8_t health;\n"
                    "    bool hidden;\n"
                    "    const char * basename;\n"
                    "} TileDef;\n"
                    "uint8_t getChTile(uint8_t i) ;\n"
                    "const TileDef * getTileDefs();\n"
                    "const TileDef & getTileDef(int i);\n\n";
    }

    for (int i = 0; i < tileDefs.size(); ++i)
    {
        Tile &tile = tileDefs[i];
        sprintf(thead, "#define %-20s 0x%.2x\n", tile.define.c_str(), i);
        tfileHdr += thead;
    }

    tfileHdr += "\n#endif\n";
    tfileHdr.close();

    if (useTileDefs)
    {
        ////////////////////////////////////////////////////////////
        // cpp

        CFileWrap tfileData;
        tfileData.open(fnameData, "wb");

        sprintf(thead,
                "//////////////////////////////////////////////////\n"
                "// autogenerated\n\n"
                "#include \"%sdata.h\"\n"
                "#include \"sprtypes.h\"\n\n"
                "const TileDef tileDefs[]={\n",
                sectionBasename.c_str());
        tfileData += thead;

        char tmp[256];
        uint8_t chMap[128];
        memset(chMap, 0, sizeof(chMap));
        for (int i = 0; i < tileDefs.size(); ++i)
        {
            Tile &tile = tileDefs[i];
            sprintf(tmp, "    {0x%.2x, TYPE_%s, %d, %d, %s, \"%s\"}%s// %.2x %s\n",
                    tile.ch, tile.typeName.c_str(), tile.score, tile.health,
                    tile.hidden ? "true" : "false",
                    tile.basename.c_str(),
                    i != tileDefs.size() - 1 ? ", " : " ", i, tile.define.c_str());
            tfileData += tmp;
            if (tile.ch != 0)
            {
                if (chMap[tile.ch] != '\0')
                {
                    Tile &tileOld = tileDefs[chMap[tile.ch]];
                    printf("conflict warning: char %c already assigned to %s. \n",
                           tile.ch, tileOld.name.c_str());
                }
                chMap[tile.ch] = i;
            }
        }
        tfileData += "};\n\n"
                     "const uint8_t chMap[] = {\n";
        for (int i = 0; i < sizeof(chMap); ++i)
        {
            sprintf(tmp, "%s0x%.2x%s%s",
                    i % 8 == 0 ? "     " : "",
                    chMap[i], i != sizeof(chMap) - 1 ? ", " : "",
                    i % 8 == 7 ? "\n" : "");
            tfileData += tmp;
        }
        tfileData += "};\n\n"
                     "uint8_t getChTile(const uint8_t i)\n"
                     "{\n"
                     "   return chMap[i % sizeof(chMap)];\n"
                     "}\n\n"
                     "const TileDef * getTileDefs()\n"
                     "{\n"
                     "  return tileDefs;\n"
                     "}\n"
                     "const TileDef & getTileDef(int i)\n"
                     "{\n"
                     "  return tileDefs[i];\n"
                     "}\n"
                     "\n";

        tfileData.close();
    }
}

Tile parseFileParams(const StringVector &list, const StrVal &typeMap, int &start, int &end, std::vector<uint8_t> &ch, bool &genHeaders)
{
    Tile tile{.id = 0, .ch = 0, .name = formatTitleName(list[0].c_str()), .typeName = "NONE", .score = 0};

    for (int i = 1; i < list.size(); ++i)
    {
        const std::string item = list[i];
        switch (item.c_str()[0])
        {
        case '@':
            start = std::stoi(item.substr(1));
            end = start + 1;
            genHeaders = true;
            break;
        case 'x':
            ch.push_back(std::stoi(item.substr(1), 0, 16));
            break;

        default:
            if (item.length() == 1)
            {
                ch.push_back(static_cast<u_int8_t>(item[0]));
            }
            else if (item[0] == '$')
            {
                tile.score = std::stoi(item.substr(1), 0, 10);
            }
            else if (item[0] == '+')
            {
                tile.health = std::stoi(item.substr(1), 0, 10);
            }
            else if (item[0] == '-')
            {
                tile.health = -std::stoi(item.substr(1), 0, 10);
            }
            else if (item[0] == '*')
            {
                if (item.substr(1) == "HIDDEN")
                {
                    tile.hidden = true;
                }
            }
            else if (item[0] == '!')
            {
                tile.name = formatTitleName(item.substr(1).c_str());
            }
            else
            {
                tile.typeName = item;
                tile.type = static_cast<StrVal>(typeMap)[item]; // typeMap.count(item) != 0 ? typeMap[item] : -1;
            }
        }
    }
    return tile;
}

void writeMapFile(const std::string section, const TileVector tileDefs, bool generateHeader)
{
    char fnameMap[section.length() + strlen(".map") + 1];
    sprintf(fnameMap, "%s%s", section.c_str(), ".map");

    CFileWrap tfileMap;
    tfileMap.open(fnameMap, "wb");
    for (int j = 0; j < tileDefs.size(); ++j)
    {
        Tile tile = tileDefs[j];
        char tmp[tile.basename.length() + tile.typeName.length() + 32];
        if (generateHeader)
        {
            sprintf(tmp, "%.2x %-20s %-10s %.2x\n", j, tile.basename.c_str(), tile.typeName.c_str(), tile.ch);
        }
        else
        {
            sprintf(tmp, "%.2x %-20s\n", j, tile.basename.c_str());
        }
        tfileMap.write(tmp, strlen(tmp));
    }
    tfileMap.close();
}

bool processSection(
    const std::string section,
    StringVector &files,
    const StrVal &typeMap,
    uint8_t pixelWidth)
{
    bool genHeaders = false;

    std::string sectionName = formatTitleName(section.c_str());
    std::string sectionBasename = getBasename(section.c_str());

    char fnameTiny[section.length() + strlen("tiny.obl") + 1];
    sprintf(fnameTiny, "%s%s", section.c_str(), "tiny.obl");

    char fnameT[section.length() + strlen(".mcz") + 1];
    sprintf(fnameT, "%s%s", section.c_str(), ".mcz");

    CFileWrap tfileTiny;
    tfileTiny.open(fnameTiny, "wb");

    CFrameSet imagesTiny;

    int j = 0;

    TileVector tileDefs;
    CFileWrap sfile;
    for (int i = 0; i < files.size(); ++i)
    {
        StringVector list;
        splitString(files[i], list);
        const char *fname = list[0].c_str();
        if (sfile.open(fname, "rb"))
        {
            // read original images
            CFrameSet images;
            if (images.extract(sfile))
            {
                printf("%s images:%d\n", fname, images.getSize());
            }
            int count = images.getSize();
            int start = 0;
            int end = count;

            // parse args
            std::vector<uint8_t> ch;
            Tile tile = parseFileParams(list, typeMap, start, end, ch, genHeaders);
            tile.basename = getBasename(fname);

            // create shrink copies
            int jStart = j;
            int u = 0;
            for (int i = start; i < end; ++i)
            {
                tile.id = j;
                tile.ch = u < ch.size() ? ch[u] : 0;
                CFrame *s = images[i];
                CFrame *t = new CFrame;
                t->copy(s);

                if (t->m_nHei > 16 || t->m_nLen > 16)
                {
                    t->resize(32, 32);
                    int shiftRight = (32 - s->m_nLen) / 2;
                    for (int j = 0; j < shiftRight; ++j)
                    {
                        t->shiftRIGHT();
                    }
                    int shiftDown = (32 - s->m_nHei) / 2;
                    for (int j = 0; j < shiftDown; ++j)
                    {
                        t->shiftDOWN();
                    }
                    t->shrink();
                }
                imagesTiny.add(t);

                char un[8];
                sprintf(un, "_%.X", u + 1);
                tile.define = sectionName + "_" + tile.name + (u ? un : ""); // tileName;
                tileDefs.push_back(tile);
                ++j;
                ++u;
            }
        }
        else
        {
            printf("failed to open: %s\n", fname);
        }
    }
    imagesTiny.write(tfileTiny);

    // generate tileset
    CTileSet tiles(16, 16, imagesTiny.getSize(), pixelWidth); // create tileset
    for (int i = 0; i < imagesTiny.getSize(); ++i)
    {
        CFrame *frame = imagesTiny[i];
        uint32_t *rgb888 = frame->getRGB();
        int pixels = frame->m_nLen * frame->m_nHei;
        int j;
        uint16_t rgb565[pixels];
        rgb24_t rgb24[pixels];

        switch (pixelWidth)
        {
        case CTileSet::pixel16:
            for (j = 0; j < pixels; ++j)
            {
                rgb565[j] = rgb888torgb565(reinterpret_cast<uint8_t *>(&rgb888[j]));
            }
            tiles.set(i, rgb565);
            break;

        case CTileSet::pixel24:
            for (j = 0; j < pixels; ++j)
            {
                memcpy(&rgb24[j], &rgb888[j], 3);
            }
            tiles.set(i, rgb24);
        };
    }
    tiles.write(fnameT);

    // write MapFile
    writeMapFile(section, tileDefs, genHeaders);

    // generate headers
    generateHeader(section, sectionName, sectionBasename, tileDefs, genHeaders);
    printf("\n");
    return true;
}

bool runJob(const char *src, uint8_t pixelWidth)
{
    FILE *sfile = fopen(src, "rb");
    if (sfile != NULL)
    {
        StrVal typeMap;
        StrVector sectionList;
        Config conf;

        parseConfig(conf, sectionList, sfile);
        for (int i = 0; i < sectionList.size(); ++i)
        {
            std::string section = sectionList[i];
            StringVector files = conf[section];
            if (section == "types")
            {
                processTypes(files, typeMap);
            }
            else
            {
                puts(section.c_str());
                processSection(section, files, typeMap, pixelWidth);
            }
        }
    }
    else
    {
        printf("can't open %s\n", src);
        return false;
    }

    return true;
}

void showUsage()
{
    puts(
        "mcxz tileset generator\n\n"
        "usage: \n"
        "       mcxz file1.ini file2.ini -2 -3 \n"
        "\n"
        "filex.ini        job configuration \n"
        "-2               set pixelWidth to 2 (16bits)\n"
        "-3               set pixelWidth to 3 (18bits/24bits)\n"
        "-h               show help\n"
        "\n");
}

int main(int argc, char *argv[], char *envp[])
{
    int count = 0;
    uint8_t pixelWidth = 2;

    for (int i = 1; i < argc; ++i)
    {
        char *src = argv[i];
        if (src[0] == '-')
        {
            if (std::string(src) == "--help")
            {
                showUsage();
                return EXIT_SUCCESS;
            }

            if (strlen(src) != 2)
            {
                printf("invalid switch: %s\n", src);
                return EXIT_FAILURE;
            }

            if (src[1] == 'h')
            {
                showUsage();
                return EXIT_SUCCESS;
            }
            else if (src[1] == 't')
            {
                test();
                continue;
            }

            pixelWidth = src[1] - '0';
            if (pixelWidth != CTileSet::pixel16 &&
                pixelWidth != CTileSet::pixel24)
            {
                printf("invalid switch: %s\n", src);
                return EXIT_FAILURE;
            }
            continue;
        }

        char *t = strstr(src, ".ini");
        if (!t || strcmp(t, ".ini") != 0)
        {
            printf("source %s doesn't end with .ini\n", src);
            return EXIT_FAILURE;
        }
        ++count;
        if (!runJob(src, pixelWidth))
        {
            puts("error encountered.");
            return EXIT_FAILURE;
        }
    }

    if (count == 0)
    {
        puts("require at least one .ini file argument");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}