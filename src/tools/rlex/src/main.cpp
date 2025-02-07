#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <unordered_map>
#include <map>
#include <vector>
#include <inttypes.h>
#include "map.h"
#include "maparch.h"
#include "decoder.h"
// #include "data.h"

// https://github.com/ChuOkupai/rle-compression?tab=readme-ov-file#technical-specifications
typedef std::vector<std::string> StringVector;

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

bool process_mcz(std::string name)
{

    fprintf(g_tlog, "**** %s\n\n", name.c_str());

    std::string sFilename = "data/" + name + ".mcz";
    std::string tFilename = "out/" + name + ".mcz.rle";
    std::string tFilenamev1 = "out/" + name + ".mcz.v1";
    std::string tFilenamev2 = "out/" + name + ".mcz.v2";

    FILE *tfile = fopen(tFilename.c_str(), "wb");
    if (!tfile)
    {
        printf("can't write %s\n", tFilename.c_str());
        return 0;
    }

    FILE *tfile1 = fopen(tFilenamev1.c_str(), "wb");
    if (!tfile1)
    {
        printf("can't write %s\n", tFilenamev1.c_str());
        return 0;
    }

    FILE *tfile2 = fopen(tFilenamev2.c_str(), "wb");
    if (!tfile2)
    {
        printf("can't write %s\n", tFilenamev2.c_str());
        return 0;
    }

    FILE *sfile = fopen(sFilename.c_str(), "rb");
    if (!sfile)
    {
        printf("can't read %s\n", sFilename.c_str());
        return 0;
    }
    printf("opening: %s\n", sFilename.c_str());
    long size = getFileSize(sfile) - 12;
    fseek(sfile, 12, SEEK_SET);
    const int tileCount = size / 256;
    uint8_t buf[256];
    uint8_t out[512];
    fprintf(g_tlog, "tileCount: %d\n", tileCount);
    uint16_t *idx = new uint16_t[tileCount];
    // write header (including dummy idx)
    fwrite(&tileCount, sizeof(uint16_t), 1, tfile);
    fwrite(idx, tileCount * sizeof(uint16_t), 1, tfile);
    for (int i = 0; i < tileCount; ++i)
    {
        idx[i] = static_cast<uint16_t>(ftell(tfile));
        fprintf(g_tlog, "%s@%d at offset 0x%.4x\n", name.c_str(), i, idx[i]);
        fread(buf, sizeof(buf), 1, sfile);

        int k = compress(buf, out, sizeof(buf));
        fwrite(out, k, 1, tfile);

        fwrite(buf, sizeof(buf), 1, tfile1);
        Decoder decoder;
        decoder.start(out);
        for (size_t j = 0; j < sizeof(buf); ++j)
        {
            buf[j] = decoder.get();
        }
        fwrite(buf, sizeof(buf), 1, tfile2);
        //  printf("size: %d\n", k);
        // break;
    }
    // update index with real values
    fseek(tfile, 2, SEEK_SET);
    fwrite(idx, tileCount * sizeof(uint16_t), 1, tfile);
    delete[] idx;
    fclose(sfile);
    fclose(tfile);
    return true;
}

bool process_mapz()
{
    const char sName[] = "data/levels.mapz";
    const char tName[] = "out/levels.mapz.rle";

    printf("reading: %s\n", sName);
    CMapArch arch;
    if (!arch.read(sName))
    {
        printf("can't read:%s\n", sName);
        return false;
    }

    FILE *tfile = fopen(tName, "wb");
    if (!tfile)
    {
        printf("can't write:%s\n", tName);
        return false;
    }

    int count = arch.size();
    uint16_t *idx = new uint16_t[count];
    uint8_t *out = new uint8_t[16384];
    // write header (including dummy idx)
    fwrite(&count, sizeof(uint16_t), 1, tfile);
    fwrite(idx, count * sizeof(uint16_t), 1, tfile);
    for (int i = 0; i < count; ++i)
    {
        idx[i] = static_cast<uint16_t>(ftell(tfile));
        CMap *map = arch.at(i);
        int len = map->len();
        int hei = map->hei();
        printf("  -> level %d len: %d hei: %d offset: 0x%.4x\n",
               i + 1, len, hei, idx[i]);
        int k = compress(map->row(0), out, len * hei);
        fwrite(&len, 1, 1, tfile);
        fwrite(&hei, 1, 1, tfile);
        fwrite(out, k, 1, tfile);

        auto &attrs = map->attrs();
        size_t attrCount = attrs.size();
        fwrite(&attrCount, sizeof(uint16_t), 1, tfile);
        printf("  -> attrCount %d [0x%.4x]\n", attrCount, attrCount);
        for (auto &[key, a] : attrs)
        {
            uint8_t x = key & 0xff;
            uint8_t y = key >> 8;
            fwrite(&x, sizeof(x), 1, tfile);
            fwrite(&y, sizeof(y), 1, tfile);
            fwrite(&a, sizeof(a), 1, tfile);
        }
    }

    // update index with real values
    fseek(tfile, 2, SEEK_SET);
    fwrite(idx, count * sizeof(uint16_t), 1, tfile);
    delete[] idx;
    fclose(tfile);

    return true;
}

bool decodeMap(int i)
{
    // find map data and size
    printf("count:%d\n", Decoder::size(levels_mapz));
    printf("offset:0x%.4x\n", Decoder::offset(levels_mapz, i));

    printf("i=%d\n", i);
    uint8_t *data = Decoder::data(levels_mapz, i);
    uint16_t len = data[0];
    uint16_t hei = data[1];
    CMap map;
    printf("map len:%d [0x%.2x] hei:%d [0x%.2x]\n", len, len, hei, hei);
    map.resize(len, hei, true);

    // start decoding map
    uint8_t *dest = map.row(0);
    Decoder decoder;
    decoder.start(data + 2);
    for (uint16_t i = 0; i < len * hei; ++i)
    {
        // decoding map
        *dest++ = decoder.get();
    }

    // copy map attributs
    data += decoder.idx() + 2;
    uint16_t count = data[0] + (data[1] << 8);
    printf("attrs count:%d [0x%.4x]\n", count, count);
    data += 2;
    while (count--)
    {
        map.set(data[0], data[1], data[2]);
        data += 3;
    }
    return true;
}

bool validateMapData()
{
    const char sName[] = "out/levels.mapz.rle";
    FILE *sfile = fopen(sName, "rb");
    if (!sfile)
    {
        printf("can't read:%s\n", sName);
        return false;
    }

    size_t size = getFileSize(sfile);
    printf("filesize: %d\n", size);
    levels_mapz = new uint8_t[size];
    fread(levels_mapz, size, 1, sfile);
    decodeMap(0);

    const char tName[] = "out/levels.mapz.bin";
    FILE *tfile = fopen(tName, "wb");
    if (!tfile)
    {
        printf("can't write:%s\n", tName);
        return false;
    }
    fwrite(levels_mapz, size, 1, tfile);
    fclose(tfile);

    fclose(sfile);
    delete[] levels_mapz;
    levels_mapz = nullptr;

    return true;
}

int main(int argc, char *argv[], char *envp[])
{
    g_tlog = fopen("out/compress.log", "wb");
    if (!g_tlog)
    {
        printf("cannot write log\n");
        return 1;
    }

    StringVector files = {"animz", "annie", "tiles"};
    for (size_t i = 0; i < files.size(); ++i)
    {
        process_mcz(files[i]);
    }

    process_mapz();
    validateMapData();
    fclose(g_tlog);

    return EXIT_SUCCESS;
}