/*
    LGCK Builder Runtime
    Copyright (C) 1999, 2020, 2025  Francois Blanchette

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "PngMagic.h"
#include "Frame.h"
#include "FrameSet.h"
#include <zlib.h>
#include <string>
#include <cstring>
#include <cstdio>
#include <cmath>
#include "CRC.h"
#include "IFile.h"
#include "logger.h"

/* These describe the color_type field in png_info. */
/* color type masks */
#define PNG_COLOR_MASK_PALETTE 1
#define PNG_COLOR_MASK_COLOR 2
#define PNG_COLOR_MASK_ALPHA 4

/* color types.  Note that not all combinations are legal */
#define PNG_COLOR_TYPE_GRAY 0
#define PNG_COLOR_TYPE_PALETTE (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_PALETTE)
#define PNG_COLOR_TYPE_RGB (PNG_COLOR_MASK_COLOR)
#define PNG_COLOR_TYPE_RGB_ALPHA (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_ALPHA)
#define PNG_COLOR_TYPE_GRAY_ALPHA (PNG_COLOR_MASK_ALPHA)
/* aliases */
#define PNG_COLOR_TYPE_RGBA PNG_COLOR_TYPE_RGB_ALPHA
#define PNG_COLOR_TYPE_GA PNG_COLOR_TYPE_GRAY_ALPHA

constexpr uint32_t OBLT_VERSION0 = 0;
constexpr uint32_t OBLT_VERSION2 = 2;
static bool g_verbose = false;

// 0 = None 1 = Sub 2 = Up 3 = Average 4 = Paeth
constexpr uint8_t FILTERING_NONE = 0;
constexpr uint8_t FILTERING_SUB = 1;
constexpr uint8_t FILTERING_UP = 2;
constexpr uint8_t FILTERING_AVERAGE = 3;
constexpr uint8_t FILTERING_PAETH = 4;

constexpr int32_t PIXELWIDTH_PALETTE = 1;
constexpr int32_t PIXELWIDTH_RGB = 3;
constexpr int32_t PIXELWIDTH_RGBA = 4;
constexpr size_t PALETTE_SIZE = 256;
constexpr uint8_t ALPHA = 255;
constexpr size_t CHUNK_TYPE_LEN = 4;
constexpr size_t RGB_BYTES = 3;

void enablePngMagicVerbose()
{
    g_verbose = true;
}

typedef struct
{
    uint32_t Lenght; // 4 UINT8s
    uint8_t ChunkType[CHUNK_TYPE_LEN];
    uint32_t Width;      //: 4 uint8_ts
    uint32_t Height;     //: 4 uint8_ts
    uint8_t BitDepth;    //: 1 uint8_t
    uint8_t ColorType;   //: 1 uint8_t
    uint8_t Compression; //: 1 uint8_t
    uint8_t Filter;      //: 1 uint8_t
    uint8_t Interlace;   //: 1 uint8_t
} png_IHDR;

uint8_t PaethPredictor(uint8_t a, uint8_t b, uint8_t c)
{
    int p = a + b - c;               // initial estimate
    int pa = std::abs((float)p - a); // distances to a, b, c
    int pb = std::abs((float)p - b); //
    int pc = std::abs((float)p - c);
    //; return nearest of a,b,c,
    //; breaking ties in order a,b,c.
    if ((pa <= pb) && (pa <= pc))
    {
        return a;
    }
    else if (pb <= pc)
    {
        return b;
    }
    else
        return c;
}

static bool _4bpp(
    CFrame *frame,
    uint8_t *cData,
    const int cDataSize,
    const png_IHDR &ihdr,
    const uint8_t plte[][RGB_BYTES],
    const bool trns_found,
    const uint8_t trns[],
    const int offsetY,
    std::string &lastError);

static bool _8bpp(
    CFrame *frame,
    uint8_t *cData,
    const int cDataSize,
    const png_IHDR &ihdr,
    const uint8_t plte[][RGB_BYTES],
    const bool trns_found,
    const uint8_t trns[],
    const int offsetY,
    std::string &lastError);

const std::string_view getFilteringName(uint8_t i)
{
    constexpr std::string_view names[] = {
        "None", "Sub", "Up", "Average", "Paeth"};
    constexpr size_t count = sizeof(names) / sizeof(names[0]);
    if (i < count)
        return names[i];
    else
        return "unknown";
}

bool parsePNG(CFrameSet &set, IFile &file, int orgPos)
{
    CCRC crc;
    auto fileSize = file.getSize();
    if (fileSize < 0)
    {
        set.setLastError("failed to get file size");
        return false;
    }

    constexpr uint8_t pngSig[] = {137, 80, 78, 71, 13, 10, 26, 10};
    decltype(fileSize) pos = orgPos;
    if (!file.seek(orgPos))
    {
        set.setLastError("seek to orgPos failed");
        return false;
    }
    uint8_t sig[sizeof(pngSig)];
    if (file.read(sig, sizeof(pngSig)) != IFILE_OK)
    {
        set.setLastError("failed to read signature");
        return false;
    }
    pos += sizeof(pngSig);
    if (memcmp(sig, pngSig, sizeof(pngSig)) != 0)
    {
        set.setLastError("signature mismatch; expecting png signature");
        return false;
    }

    png_IHDR ihdr;
    memset(&ihdr, 0, sizeof(png_IHDR));

    uint8_t plte[PALETTE_SIZE][RGB_BYTES];
    uint8_t trns[PALETTE_SIZE];
    memset(trns, ALPHA, sizeof(trns));

    const char *png_chunk_OBL5 = CFrame::getChunkType();
    bool iend_found = false;
    bool trns_found = false;

    std::vector<uint8_t> cData;
    int obl5t_count = 0;
    uint32_t obl5t_version = OBLT_VERSION0;
    std::vector<uint16_t> obl5t_sx;                        // oblt_v0
    std::vector<uint16_t> obl5t_sy;                        // oblt_v0
    std::vector<CFrame::oblv2DataUnit_t> obl5t_metadatav2; // oblt_v2
    int cDataSize = 0;
    while (pos < fileSize)
    {
        // read chunksize
        int32_t chunkSize;
        if (file.read(&chunkSize, sizeof(chunkSize)) != IFILE_OK)
        {
            set.setLastError("failed to read ChunkSize");
            return false;
        }
        chunkSize = CFrame::toNet(chunkSize);
        pos += sizeof(chunkSize);

        // read chunktype
        char chunkType[CHUNK_TYPE_LEN];
        if (file.read(chunkType, sizeof(chunkType)) != IFILE_OK)
        {
            set.setLastError("failed to read ChunkType");
            return false;
        }
        pos += sizeof(chunkType);

        char ct[sizeof(chunkType) + 1];
        memcpy(ct, chunkType, sizeof(chunkType));
        ct[sizeof(chunkType)] = '\0';

        // read chunkdata
        std::vector<uint8_t> chunkDataRaw(chunkSize + sizeof(chunkType));
        if (chunkSize != 0 && file.read(chunkDataRaw.data() + sizeof(chunkType), chunkSize) != IFILE_OK)
        {
            char tmp[128];
            snprintf(tmp, sizeof(tmp), "failed to read Raw Chunk Data [%s] size: %d", ct, chunkSize);
            set.setLastError(tmp);
            return false;
        }
        pos += chunkSize;
        memcpy(chunkDataRaw.data(), chunkType, sizeof(chunkType));
        uint8_t *chunkData = chunkDataRaw.data() + sizeof(chunkType);

        int32_t crc32;
        file.read(&crc32, sizeof(crc32));
        pos += sizeof(crc32);
        crc32 = CFrame::toNet(crc32);
        // Check crc32 for each chunk
        int crc32c = crc.crc(chunkDataRaw.data(), chunkSize + sizeof(chunkType));
        if (crc32 != crc32c)
        {
            set.setLastError("chunk CRC32 checksum doesn't match");
            return false;
        }

        if (memcmp(chunkType, "IHDR", CHUNK_TYPE_LEN) == 0)
        {
            memcpy(((uint8_t *)&ihdr) + 8, chunkData, chunkSize);
            memcpy(ihdr.ChunkType, chunkType, 4);
            ihdr.Lenght = chunkSize;
            if (g_verbose)
            {
                LOGI("Width: %d, Height: %d", CFrame::toNet(ihdr.Width), CFrame::toNet(ihdr.Height));
                LOGI("BitDepth: %d", ihdr.BitDepth);
                LOGI("ColorType: %d", ihdr.ColorType);
                LOGI("Compression: %d", ihdr.Compression);
                LOGI("Filter: %d", ihdr.Filter);
                LOGI("Interlace: %d\n", ihdr.Interlace);
            }
        }

        else if (memcmp(chunkType, "PLTE", CHUNK_TYPE_LEN) == 0)
        {
            memcpy(plte, chunkData, chunkSize);
        }

        else if (memcmp(chunkType, "IDAT", CHUNK_TYPE_LEN) == 0)
        {
            if (cData.size() != 0)
            {
                cData.resize(cDataSize + chunkSize);
                memcpy(cData.data() + cDataSize, chunkData, chunkSize);
            }
            else
            {
                cData.resize(chunkSize);
                memcpy(cData.data(), chunkData, chunkSize);
            }
            cDataSize += chunkSize;
        }

        else if (memcmp(chunkType, "tRNS", CHUNK_TYPE_LEN) == 0)
        {
            memcpy(trns, chunkData, chunkSize);
            trns_found = true;
        }

        else if (memcmp(chunkType, "IEND", CHUNK_TYPE_LEN) == 0)
        {
            iend_found = true;
        }

        else if (memcmp(chunkType, png_chunk_OBL5, CHUNK_TYPE_LEN) == 0)
        {
            CFrame::png_OBL5 obl5t;
            char *t = (char *)&obl5t;
            memcpy(t + 8, chunkData, 12);
            obl5t_version = obl5t.Version;
            if (obl5t.Version == OBLT_VERSION0)
            {
                // version 0x0000 is supported
                obl5t_count = obl5t.Count;
                obl5t_sx.resize(obl5t.Count);
                obl5t_sy.resize(obl5t.Count);
                memcpy(obl5t_sx.data(), chunkData + 12,
                       sizeof(short) * obl5t.Count);
                memcpy(obl5t_sy.data(), chunkData + 12 + sizeof(short) * obl5t.Count,
                       sizeof(short) * obl5t.Count);
            }
            else if (obl5t.Version == OBLT_VERSION2)
            {
                // version 0x0002 is supported
                obl5t_count = obl5t.Count;
                obl5t_metadatav2.resize(obl5t.Count);
                memcpy(obl5t_metadatav2.data(), chunkData + 12,
                       sizeof(CFrame::oblv2DataUnit_t) * obl5t.Count);
            }
            else
            {
                LOGW("unsupported oblt version: %d", obl5t.Version);
            }
        }
        else
        {
            // LOGW("ignored unsupported chunkType: %s", ct);
        }
    }

    bool valid = false;
    if (cData.size() > 0 && iend_found)
    {
        // qDebug("total cData:%d\n", cDataSize);
        if (!ihdr.Interlace &&
            ((ihdr.BitDepth == 8) || (ihdr.BitDepth == 4)) &&
            (ihdr.ColorType != PNG_COLOR_TYPE_GRAY) &&
            (ihdr.ColorType != PNG_COLOR_TYPE_GRAY_ALPHA))
        {
            int height = CFrame::toNet(ihdr.Height);
            int width = CFrame::toNet(ihdr.Width);
            int offsetY = 0;
            if (width & 7)
            {
                width += (8 - (width & 7));
            }
            if (height & 7)
            {
                offsetY = 8 - (height & 7);
                height += offsetY;
            }
            std::unique_ptr<CFrame> frame = std::make_unique<CFrame>(width, height);
            memset(frame->getRGB().data(), 0, sizeof(uint32_t) * frame->width() * frame->height());

            std::string lastError;
            if (ihdr.BitDepth == 8)
            {
                valid = _8bpp(frame.get(), cData.data(), cDataSize, ihdr, plte, trns_found, trns, offsetY, lastError);
            }

            if (ihdr.BitDepth == 4)
            {
                valid = _4bpp(frame.get(), cData.data(), cDataSize, ihdr, plte, trns_found, trns, offsetY, lastError);
            }

            if (!valid)
            {
                set.setLastError(lastError.c_str());
                return false;
            }

            if (obl5t_count > 0)
            {
                // using obLT chunkdata
                if (obl5t_version == OBLT_VERSION0)
                {
                    frame->explode(obl5t_count, obl5t_sx.data(), obl5t_sy.data(), &set);
                }
                else if (obl5t_version == OBLT_VERSION2)
                {
                    frame->explode(obl5t_metadatav2, &set);
                }
                else
                {
                    std::string error = "unsupported obLT version:" + std::to_string(obl5t_version);
                    set.setLastError(error.c_str());
                    return false;
                }
            }
            else
            {
                set.add(frame.release());
            }
        }
        else
        {
            set.setLastError("unsupported png");
            return false;
        }
    }
    return valid;
}

static bool _8bpp(
    CFrame *frame,
    uint8_t *cData,
    const int cDataSize,
    const png_IHDR &ihdr,
    const uint8_t plte[][RGB_BYTES],
    const bool trns_found,
    const uint8_t trns[],
    const int offsetY,
    std::string &lastError)
{
    int pixelWidth = -1;

    switch (ihdr.ColorType)
    {
    case PNG_COLOR_TYPE_PALETTE:
        pixelWidth = PIXELWIDTH_PALETTE;
        break;
    case PNG_COLOR_TYPE_RGB:
        pixelWidth = PIXELWIDTH_RGB;
        break;
    case PNG_COLOR_TYPE_RGB_ALPHA:
        pixelWidth = PIXELWIDTH_RGBA;
        break;
    default:
        char tmp[128];
        snprintf(tmp, sizeof(tmp), "Invalid/unsupported ColorType %d", ihdr.ColorType);
        lastError = tmp;
        return false;
    }

    // printf("pixelWidth %d\n", pixelWidth);

    int pitch = CFrame::toNet(ihdr.Width) * pixelWidth + 1;
    // printf("pixelWidth: %d\n", pixelWidth);
    // printf("pitch: %d\n", pitch);

    uLong dataSize = pitch * CFrame::toNet(ihdr.Height);
    std::vector<uint8_t> fData(dataSize);
    //    printf("total data:%d\n", ((int)dataSize));

    int err = uncompress(
        (uint8_t *)fData.data(),
        (uLong *)&dataSize,
        (uint8_t *)cData,
        (uLong)cDataSize);

    if (err != Z_OK)
    {
        char tmp[128];
        snprintf(tmp, sizeof(tmp), "Zlib compression error %d: %s", err, zError(err));
        lastError = tmp;
        return false;
    }

    bool valid = true;
    uint32_t *rgb = frame->getRGB().data();

    for (int y = 0; y < (int)CFrame::toNet(ihdr.Height); y++)
    {
        uint8_t *line = &fData.data()[pitch * y + 1];
        uint8_t *prLine = nullptr;
        if (y > 0)
        {
            prLine = &fData.data()[pitch * (y - 1) + 1];
        }
        uint8_t filtering = fData.data()[pitch * y];

        switch (filtering)
        {
        case FILTERING_NONE:
            break;

        case FILTERING_SUB:
            for (int x = 0; x < (int)CFrame::toNet(ihdr.Width); x++)
            {
                uint32_t p = 0;
                uint32_t c = 0;
                if (x)
                {
                    memcpy(&p, &line[(x - 1) * pixelWidth], pixelWidth);
                }

                memcpy(&c, &line[x * pixelWidth], pixelWidth);

                uint8_t *pc = (uint8_t *)&c;
                uint8_t *pp = (uint8_t *)&p;

                for (int i = 0; i < pixelWidth; i++)
                {
                    pc[i] += pp[i];
                }

                memcpy(&line[x * pixelWidth], pc, pixelWidth);
            }
            break;

        case FILTERING_UP:
            for (int x = 0; x < (int)CFrame::toNet(ihdr.Width); x++)
            {
                uint32_t p = 0;
                uint32_t c = 0;
                if (y > 0)
                {
                    memcpy(&p, &prLine[x * pixelWidth], pixelWidth);
                }

                memcpy(&c, &line[x * pixelWidth], pixelWidth);

                uint8_t *pc = (uint8_t *)&c;
                uint8_t *pp = (uint8_t *)&p;

                for (int i = 0; i < pixelWidth; i++)
                {
                    pc[i] += pp[i];
                }

                memcpy(&line[x * pixelWidth], &c, pixelWidth);
            }
            break;

        case FILTERING_AVERAGE:
            for (int x = 0; x < (int)CFrame::toNet(ihdr.Width); x++)
            {

                uint32_t a = 0; // left
                uint32_t b = 0; // above (Prior)

                if (x)
                {
                    memcpy(&a, &line[(x - 1) * pixelWidth], pixelWidth);
                }

                if (y)
                {
                    memcpy(&b, &prLine[x * pixelWidth], pixelWidth);
                }

                uint8_t *pa = (uint8_t *)&a;
                uint8_t *pb = (uint8_t *)&b;
                uint8_t *ph = &line[x * pixelWidth];

                for (int i = 0; i < pixelWidth; i++)
                {
                    ph[i] += (pa[i] + pb[i]) / 2;
                }
            }

            break;

        case FILTERING_PAETH:
            for (int x = 0; x < (int)CFrame::toNet(ihdr.Width); x++)
            {
                uint32_t a = 0; // left
                uint32_t b = 0; // above
                uint32_t c = 0; // above-left

                if (x)
                {
                    memcpy(&a, &line[(x - 1) * pixelWidth], pixelWidth);
                }

                if (y)
                {
                    memcpy(&b, &prLine[x * pixelWidth], pixelWidth);
                    if (x)
                    {
                        memcpy(&c, &prLine[(x - 1) * pixelWidth], pixelWidth);
                    }
                }

                uint8_t *pa = (uint8_t *)&a;
                uint8_t *pb = (uint8_t *)&b;
                uint8_t *pc = (uint8_t *)&c;

                uint8_t *ph = &line[x * pixelWidth];
                for (int i = 0; i < pixelWidth; i++)
                {
                    ph[i] += PaethPredictor(pa[i], pb[i], pc[i]);
                }
            }
            break;

        default:
            valid = false;
            char tmp[128];
            snprintf(tmp, sizeof(tmp), "unsupported filtering: %s [%d]", getFilteringName(filtering).data(), filtering);
            lastError = tmp;
            return false;
        }

        for (int x = 0; x < (int)CFrame::toNet(ihdr.Width); x++)
        {
            uint32_t rgba = 0xff000000;
            switch (pixelWidth)
            {
            case PIXELWIDTH_PALETTE:
                memcpy(&rgba, plte[line[x]], RGB_BYTES);

                if (trns_found)
                {
                    rgba &= (trns[line[x]] * 0x1000000) + 0xffffff;
                }
                break;

            case PIXELWIDTH_RGB:
            case PIXELWIDTH_RGBA:
                memcpy(&rgba, &line[x * pixelWidth], pixelWidth);
                break;
            }

            rgb[(offsetY + y) * frame->width() + x] = rgba;
        }
    }
    return valid;
}

static bool _4bpp(
    CFrame *frame,
    uint8_t *cData,
    const int cDataSize,
    const png_IHDR &ihdr,
    const uint8_t plte[][RGB_BYTES],
    const bool trns_found,
    const uint8_t trns[],
    const int offsetY,
    std::string &lastError)
{
    int height = CFrame::toNet(ihdr.Height);
    int width = CFrame::toNet(ihdr.Width);

    if (ihdr.ColorType != PNG_COLOR_TYPE_PALETTE)
    {
        lastError = "colorType is not PNG_COLOR_TYPE_PALETTE";
        return false;
    }

    int pitch = 1 + CFrame::toNet(ihdr.Width) / 2;
    if (CFrame::toNet(ihdr.Width) & 1)
    {
        ++pitch;
    }
    // printf("pixelWidth: %d\n", pixelWidth);
    // printf("pitch: %d\n", pitch);

    uint64_t dataSize = pitch * CFrame::toNet(ihdr.Height);
    std::vector<uint8_t> fData(dataSize);
    //    printf("total data:%d\n", ((int)dataSize));

    int err = uncompress(
        (uint8_t *)fData.data(),
        (uLong *)&dataSize,
        (uint8_t *)cData,
        (uLong)cDataSize);

    if (err != Z_OK)
    {
        char tmp[128];
        snprintf(tmp, sizeof(tmp), "Zlib compression error %d: %s", err, zError(err));
        lastError = tmp;
        return false;
    }

    uint32_t *rgb = frame->getRGB().data();
    for (int y = 0; y < height; y++)
    {
        uint8_t *line = &fData.data()[pitch * y + 1];
        uint8_t filtering = fData.data()[pitch * y];
        if (filtering == FILTERING_NONE)
        {
            for (int x = 0; x < width; x++)
            {
                uint32_t rgba = 0;
                uint8_t index = line[x / 2];
                if (x & 1)
                {
                    index &= 0x0f;
                }
                else
                {
                    index = index >> 4;
                }

                memcpy(&rgba, plte[index], RGB_BYTES);
                if (trns_found)
                {
                    rgba |= (trns[index] << 24);
                }
                else
                {
                    rgba |= 0xff000000;
                }

                rgb[(offsetY + y) * frame->width() + x] = rgba;
            }
        }
        else
        {
            char tmp[128];
            snprintf(tmp, sizeof(tmp), "unsupported filtering: %s [%d]", getFilteringName(filtering).data(), filtering);
            lastError = tmp;
            return false;
        }
    }
    return true;
}
