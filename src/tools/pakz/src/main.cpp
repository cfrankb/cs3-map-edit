#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <inttypes.h>
#include "map.h"
#include "maparch.h"
#include "decoder.h"
#include <cstring>
#include "../../../shared/FileWrap.h"

// https://github.com/ChuOkupai/rle-compression?tab=readme-ov-file#technical-specifications
typedef std::vector<std::string> StringVector;

struct index_t
{
    uint32_t offset;
    uint16_t len;
    uint16_t hei;
};
typedef std::vector<index_t> Index;

FILE *g_tlog;
uint8_t *levels_mapz = nullptr;

long getFileSize(FILE *f)
{
    long cur = ftell(f);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, cur, SEEK_SET);
    return size;
}

/*

[128-255][byte]
# Format of a compressed sequence
# (first byte value - 126) give the compressed sequence length between 2 and 129
# The following byte contain the value

[0-127][byte 0][byte 1]...[byte n]
# Format of a uncompressed sequence
# (first byte value + 1) give the uncompressed sequence length between 1 and 128
# The following n bytes contain the value
*/

struct count_t
{
    uint8_t value;
    uint8_t count;
};

void debugStack(std::vector<count_t> &stack)
{
    fprintf(g_tlog, "\n----- stack dump\n\n");
    for (auto &row : stack)
    {
        fprintf(g_tlog, "--> %0.2x [count: %u]\n", row.value, row.count);
    }
    fprintf(g_tlog, "\n");
}

int compress(uint8_t *buf, uint8_t *out, const int size)
{
    auto encode = [](auto &out, auto &u, auto &k)
    {
        fprintf(g_tlog, "uncompr %d [enc: 0x%.2x]:", u.size(), u.size() - 1);
        out[k++] = u.size() - 1;
        for (size_t n = 0; n < u.size(); ++n)
        {
            out[k++] = u[n];
            fprintf(g_tlog, " 0x%.2x", u[n]);
        }
        fprintf(g_tlog, "\n");
        u.clear();
    };

    std::vector<count_t> stack;
    int k = 0;
    for (int j = 0; j < size; ++j)
    {
        if (stack.size() != 0 &&
            stack.back().value == buf[j] &&
            stack.back().count != 129)
        {
            stack.back().count++;
        }
        else
        {
            stack.push_back(count_t{.value = buf[j], .count = 1});
        }
    }

    debugStack(stack);

    std::vector<uint8_t> u;
    for (size_t j = 0; j < stack.size(); ++j)
    {
        auto &s = stack.at(j);
        if (s.count > 1)
        {
            if (u.size() != 0)
            {
                encode(out, u, k);
            }
            fprintf(g_tlog, "compr %d [enc: %.2x] 0x%.2x\n", s.count, 126 + s.count, s.value);
            out[k++] = 126 + s.count;
            out[k++] = s.value;
        }
        else if (u.size() == 128)
        {
            encode(out, u, k);
        }
        else
        {
            u.push_back(s.value);
        }
    }
    return k;
}

void addPadding(FILE *tfile)
{
    const auto ALIGNMENT = 4;
    const auto MASK = ALIGNMENT - 1;

    const size_t offset = ftell(tfile);
    if (offset & MASK)
    {
        char tmp[MASK];
        memset(tmp, 0, MASK);
        fwrite(tmp, ALIGNMENT - (offset & MASK), 1, tfile);
    }
}

bool process_mcz(FILE *tfile, const std::string sFilename,
                 uint32_t &size)
{
    fprintf(g_tlog, "**** %s\n\n", sFilename.c_str());
    printf("opening: %s\n", sFilename.c_str());
    FILE *sfile = fopen(sFilename.c_str(), "rb");
    if (!sfile)
    {
        printf("can't read %s\n", sFilename.c_str());
        return 0;
    }

    if (sFilename.ends_with(".mcz"))
    {
        size = getFileSize(sfile) - 12;
        fseek(sfile, 12, SEEK_SET);
    }
    else
    {
        size = getFileSize(sfile);
    }
    printf("size: %d\n", size);

    uint8_t *buf = new uint8_t[size];
    fread(buf, size, 1, sfile);
    fwrite(buf, size, 1, tfile);
    // if (size & 0xff)
    {
        addPadding(tfile);
    }
    delete[] buf;
    fclose(sfile);
    return true;
}

bool process_mapz(FILE *tfile,
                  const std::string sFilename,
                  uint32_t &indexOffset)
{
    const char sName[] = "data/levels.mapz";
    const char tName[] = "out/levels.mapz.rle";

    printf("reading: %s\n", sName);
    CMapArch arch;
    if (!arch.read(sFilename.c_str()))
    {
        printf("can't read:%s\n", sName);
        return false;
    }

    Index idx;
    int count = arch.size();
    for (int i = 0; i < count; ++i)
    {
        CMap *map = arch.at(i);
        uint16_t len = map->len();
        uint16_t hei = map->hei();
        const index_t cur = index_t{.offset = static_cast<uint32_t>(ftell(tfile)),
                                    .len = static_cast<uint16_t>(len),
                                    .hei = static_cast<uint16_t>(hei)};
        idx.push_back(cur);
        printf("  -> level %d len: %d hei: %d offset: 0x%.4x\n",
               i + 1, len, hei, idx[i]);
        fwrite(map->row(0), len * hei, 1, tfile);

        addPadding(tfile);
        auto &attrs = map->attrs();
        const size_t attrCount = attrs.size();
        fwrite(&attrCount, sizeof(uint16_t), 1, tfile);
        printf("  -> attrCount %d [0x%.4x]\n", attrCount, attrCount);
        for (const auto &[key, a] : attrs)
        {
            uint8_t x = key & 0xff;
            uint8_t y = key >> 8;
            fwrite(&x, sizeof(x), 1, tfile);
            fwrite(&y, sizeof(y), 1, tfile);
            fwrite(&a, sizeof(a), 1, tfile);
        }
        addPadding(tfile);
    }

    indexOffset = static_cast<uint32_t>(ftell(tfile));
    fwrite(&count, sizeof(uint32_t), 1, tfile);
    for (auto &cur : idx)
    {
        fwrite(&cur.offset, sizeof(uint32_t), 1, tfile);
        fwrite(&cur.len, sizeof(uint16_t), 1, tfile);
        fwrite(&cur.hei, sizeof(uint16_t), 1, tfile);
    }
    addPadding(tfile);
    return true;
}

bool testOutputFile(std::string sFilename)
{
#define LEVELS_MAPZ_INDEX 0x00017349

    FILE *sfile = fopen(sFilename.c_str(), "rb");
    if (!sfile)
    {
        printf("can't read %s\n", sFilename.c_str());
        return 0;
    }

    auto size = getFileSize(sfile);
    uint8_t *buf = new uint8_t[size];
    fread(buf, size, 1, sfile);

    uint32_t count = 0;
    // uint8_t *t =

    delete[] buf;
    fclose(sfile);
    return true;
}

int main(int argc, char *argv[], char *envp[])
{
    std::string tFilename = "out/gamedata.bin";
    FILE *tfile = fopen(tFilename.c_str(), "wb");
    if (!tfile)
    {
        printf("can't write %s\n", tFilename.c_str());
        return 0;
    }

    std::string tFilenameHeader = "out/data.h";
    CFileWrap hdrFile;
    if (!hdrFile.open(tFilenameHeader.c_str(), "wb"))
    {
        printf("can't write %s\n", tFilenameHeader.c_str());
        return 0;
    }

    g_tlog = fopen("out/debug.log", "wb");
    if (!g_tlog)
    {
        printf("cannot write log\n");
        return 1;
    }

    StringVector files = {
        "animz.mcz",
        "annie.mcz",
        "tiles.mcz",
        "levels.mapz",
        "tiles.pal",
        "bitfont.bin"};
    char tmp[1024];
    hdrFile += "#pragma once\n\n";
    //           "#include <stdint.h>\n\n";

    std::string defname;
    for (size_t i = 0; i < files.size(); ++i)
    {
        const std::string &file = files[i];
        std::string sFilename = "data/" + file;
        std::string symbol = file;
        for (auto &c : symbol)
            c = c == '.' ? '_' : toupper(c);
        auto offset = static_cast<uint32_t>(ftell(tfile));
        uint32_t size;
        if (file.ends_with(".mapz"))
        {
            //           Index idx;
            uint32_t indexOffset;
            process_mapz(tfile, sFilename, indexOffset);
            std::string defname = symbol + "_INDEX";
            sprintf(tmp, "#define %-20s 0x%.8lx\n", defname.c_str(), indexOffset);
            hdrFile += tmp;
        }
        else // if (file.ends_with(".mcz"))
        {
            process_mcz(tfile, sFilename, size);
            defname = symbol + "_SIZE";
            sprintf(tmp, "#define %-20s 0x%.8lx\n", defname.c_str(), size);
            hdrFile += tmp;
        }

        defname = symbol + "_OFFSET";
        sprintf(tmp, "#define %-20s 0x%.8lx\n", defname.c_str(), offset);
        hdrFile += tmp;
    }

    fclose(tfile);

    //    hdrFile += "\nextern uint8_t tiles_pal[896];\n"
    //             "extern uint8_t bitfont_bin[760];\n";
    hdrFile.close();

    fclose(g_tlog);

    return EXIT_SUCCESS;
}