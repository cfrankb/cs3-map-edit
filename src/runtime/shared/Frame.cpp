/*
    LGCK Builder Runtime
    Copyright (C) 1999, 2011  Francois Blanchette

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

// Frame.cpp : implementation file
//

#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <algorithm>
#include <zlib.h>
#include "Frame.h"
#include "FrameSet.h"
#include "DotArray.h"
#include "CRC.h"
#include "IFile.h"
#include "helper.h"
#include <cstdint>
#include "logger.h"
#include "ss_limits.h"

/// @brief Constructor
/// @param p_nLen pixel lenght (must be multiple of 8)
/// @param p_nHei pixel height (must be multiple of 8)

CFrame::CFrame(int width, int height) : m_width(width), m_height(height)
{
    if (width < 0 || height < 0 || width > 4096 || height > 4096)
    {
        std::string lastError = "Invalid dimensions: " + std::to_string(width) + "x" + std::to_string(height);
        throw std::invalid_argument(lastError);
    }
    if ((width & 7) || (height & 7))
    {
        // LOGW("Dimensions %dx%d not multiples of 8", width, height);
    }
    m_rgb.resize(width * height);
    std::fill(m_rgb.begin(), m_rgb.end(), 0);
}

CFrame::CFrame(CFrame &&src) noexcept : m_rgb(std::move(src.m_rgb)),
                                        m_width(src.m_width),
                                        m_height(src.m_height)

{
    src.m_width = 0;
    src.m_height = 0;
}

CFrame &CFrame::operator=(CFrame src)
{
    swap(*this, src);
    return *this;
}

void swap(CFrame &a, CFrame &b) noexcept
{
    using std::swap;
    swap(a.m_rgb, b.m_rgb);
    swap(a.m_width, b.m_width);
    swap(a.m_height, b.m_height);
}

void CFrame::clear()
{
    m_rgb.clear();
    m_width = 0;
    m_height = 0;
}

/// @brief serializes OBL5 0x500 only
/// @param file
/// @return
bool CFrame::write(IFile &file)
{
    // this serializer creates format 0x500 only
    // use CFrameSet serializer to create solid archive

    if (m_width > MAX_IMAGE_SIZE || m_height > MAX_IMAGE_SIZE)
    {
        m_lastError = "Dimensions exceed maximum: " + std::to_string(m_width) + "x" + std::to_string(m_height);
        return false;
    }

    uint16_t width = static_cast<uint16_t>(m_width); // Align with writeSolid
    uint16_t height = static_cast<uint16_t>(m_height);
    uint32_t filler = 0;
    if (file.write(&width, sizeof(width)) != IFILE_OK ||
        file.write(&height, sizeof(height)) != IFILE_OK ||
        file.write(&filler, sizeof(filler)) != IFILE_OK)
    {
        m_lastError = "Failed to write OBL5 header";
        return false;
    }

    if (m_width * m_height == 0)
    {
        m_lastError = "frame size is 0";
        return false; // Empty frame
    }

    std::vector<uint8_t> rData(reinterpret_cast<uint8_t *>(m_rgb.data()),
                               reinterpret_cast<uint8_t *>(m_rgb.data() + m_rgb.size()));

    std::vector<uint8_t> cData;
    int err = compressData(rData, cData);
    if (err != Z_OK)
    {
        m_lastError = "Zlib compression error " + std::to_string(err) + ": " + zError(err);
        return false;
    }

    uint32_t compressedSize = static_cast<uint32_t>(cData.size());
    if (file.write(&compressedSize, sizeof(compressedSize)) != IFILE_OK)
    {
        m_lastError = "Failed to write compressed size";
        return false;
    }
    if (file.write(cData.data(), compressedSize) != IFILE_OK)
    {
        m_lastError = "Failed to write compressed data";
        return false;
    }
    return true;
}

/// @brief deserializes OBL5 0x500 only
/// @param file
/// @return
bool CFrame::read(IFile &file)
{
    uint16_t width, height; // Align with readSolid
    uint32_t filler;
    if (file.read(&width, sizeof(width)) != IFILE_OK ||
        file.read(&height, sizeof(height)) != IFILE_OK ||
        file.read(&filler, sizeof(filler)) != IFILE_OK)
    {
        m_lastError = "Failed to read OBL5 header";
        return false;
    }
    if (width > MAX_IMAGE_SIZE || height > MAX_IMAGE_SIZE)
    {
        m_lastError = "Invalid dimensions: " + std::to_string(width) + "x" + std::to_string(height);
        return false;
    }
    if (filler != 0)
    {
        m_lastError = "Invalid filler value: " + std::to_string(filler);
        return false;
    }

    clear();
    m_width = width;
    m_height = height;
    m_rgb.resize(width * height);

    if (width * height == 0)
    {
        m_lastError = "empty frame not allowed";
        return false; // Empty frame
    }

    uint32_t compressedSize;
    if (file.read(&compressedSize, sizeof(compressedSize)) != IFILE_OK)
    {
        m_lastError = "Failed to read compressed size";
        return false;
    }

    long fileSize = file.getSize();
    if (file.tell() + (long)compressedSize > fileSize)
    {
        char tmp[128];
        snprintf(tmp, sizeof(tmp),
                 "File too small for compressed data: %u; filesize: %ld tell: %ld",
                 compressedSize,
                 fileSize, file.tell());
        m_lastError = tmp;
        return false;
    }

    // read compressed data from disk
    std::vector<uint8_t> cData(compressedSize);
    if (file.read(cData.data(), compressedSize) != IFILE_OK)
    {
        m_lastError = "Failed to read compressed data";
        return false;
    }

    uLong destLen = m_rgb.size() * sizeof(m_rgb[0]);
    int err = uncompress((uint8_t *)m_rgb.data(), &destLen, cData.data(), compressedSize);
    if (err != Z_OK || destLen != m_rgb.size() * sizeof(m_rgb[0]))
    {
        m_lastError = "Zlib decompression error " + std::to_string(err) + ": " + zError(err);
        return false;
    }

    return true;
}

void CFrame::toBmp(uint8_t *&bmp, int &totalSize)
{
    // BM was read separatly
    typedef struct
    {
        // uint8_t m_sig;        // "BM"
        uint32_t m_nTotalSize; // 3a 00 00 00
        uint32_t m_nZero;      // 00 00 00 00 ???
        uint32_t m_nDiff;      // 36 00 00 00 TotalSize - ImageSize
        uint32_t m_n28;        // 28 00 00 00 ???

        uint32_t m_nLen;      // 80 00 00 00
        uint32_t m_nHei;      // 80 00 00 00
        int16_t m_nPlanes;    // 01 00
        int16_t m_nBitCount;  // 18 00
        uint32_t m_nCompress; // 00 00 00 00

        uint32_t m_nImageSize; // c0 00 00 00
        uint32_t m_nXPix;      // 00 00 00 00 X pix/m
        uint32_t m_nYPix;      // 00 00 00 00 Y pix/m
        uint32_t m_nClrUsed;   // 00 00 00 00 ClrUsed

        uint32_t m_nClrImpt; // 00 00 00 00 ClrImportant
    } USER_BMPHEADER;

    int pitch = m_width * 3;
    if (pitch % 4)
    {
        pitch = pitch - (pitch % 4) + 4;
    }

    totalSize = bmpDataOffset + pitch * m_height;

    bmp = new uint8_t[totalSize];
    bmp[0] = 'B';
    bmp[1] = 'M';

    USER_BMPHEADER &bmpHeader = *((USER_BMPHEADER *)(bmp + 2));
    bmpHeader.m_nTotalSize = totalSize;
    bmpHeader.m_nZero = 0;
    bmpHeader.m_nDiff = bmpDataOffset;
    bmpHeader.m_n28 = bmpHeaderSize;

    bmpHeader.m_nLen = m_width;
    bmpHeader.m_nHei = m_height;
    bmpHeader.m_nPlanes = 1; // always 1
    bmpHeader.m_nBitCount = 24;
    bmpHeader.m_nCompress = 0; // 00 00 00 00

    bmpHeader.m_nImageSize = pitch * m_height;
    bmpHeader.m_nXPix = 0;    // 00 00 00 00 X pix/m
    bmpHeader.m_nYPix = 0;    // 00 00 00 00 Y pix/m
    bmpHeader.m_nClrUsed = 0; // 00 00 00 00 ClrUsed
    bmpHeader.m_nClrImpt = 0; // 00 00 00 00 ClrImportant

    for (int y = 0; y < m_height; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            uint8_t *s = (uint8_t *)&at(x, m_height - y - 1);
            uint8_t *d = bmp + bmpDataOffset + x * 3 + y * pitch;
            d[0] = s[2];
            d[1] = s[1];
            d[2] = s[0];
        }
    }
}

uint32_t CFrame::toNet(const uint32_t a)
{
    uint32_t b;
    uint8_t *s = (uint8_t *)&a;
    uint8_t *d = (uint8_t *)&b;

    d[0] = s[3];
    d[1] = s[2];
    d[2] = s[1];
    d[3] = s[0];

    return b;
}

bool CFrame::toPng(std::vector<uint8_t> &png, const std::vector<uint8_t> &obl5data)
{
    png.clear();
    CCRC crc;

    // compress the data ....................................
    int scanLine = m_width * 4;
    uLong dataSize = (scanLine + 1) * m_height;
    std::vector<uint8_t> rdata(dataSize);
    for (int y = 0; y < m_height; ++y)
    {
        uint8_t *d = rdata.data() + y * (scanLine + 1);
        *d = 0;
        memcpy(d + 1, m_rgb.data() + y * m_width, scanLine);
    }

    std::vector<uint8_t> cData;
    int err = compressData(rdata, cData);
    if (err != Z_OK)
    {
        m_lastError = "Zlib decompression error " + std::to_string(err) + ": " + zError(err);
        LOGE("CFrame::toPng error: %d", err);
        return true;
    }

    const uLong cDataSize = cData.size();
    int cDataBlocks = cDataSize / pngChunkLimit;
    if (cDataSize % pngChunkLimit)
    {
        cDataBlocks++;
    }

    const int totalSize = pngHeaderSize + png_IHDR_Size + 4 + cDataSize + 12 * cDataBlocks + sizeof(png_IEND) + obl5data.size();
    png.resize(totalSize);
    uint8_t *t = png.data();

    // png signature ---------------------------------------
    uint8_t sig[] = {137, 80, 78, 71, 13, 10, 26, 10};
    memcpy(t, sig, 8);
    t += 8;

    uint32_t crc32;

    // png_IHDR ---------------------------------------------
    png_IHDR &ihdr = *((png_IHDR *)t);
    ihdr.Lenght = toNet(png_IHDR_Size - 8);
    memcpy(ihdr.ChunkType, "IHDR", 4);
    ihdr.Width = toNet(m_width);
    ihdr.Height = toNet(m_height);
    ihdr.BitDepth = 8;
    ihdr.ColorType = 6;
    ihdr.Compression = 0; // deflated
    ihdr.Filter = 0;
    ihdr.Interlace = 0;
    // ihdr.CRC = 0;
    t += png_IHDR_Size;
    crc32 = toNet(crc.crc(((uint8_t *)&ihdr) + 4, png_IHDR_Size - 4));
    memcpy(t, &crc32, 4);
    t += 4;

    // png_IDAT ....................................................
    uint32_t cDataOffset = 0;
    uint32_t cDataLeft = cDataSize;
    do
    {
        int chunkSize;
        if (cDataLeft > pngChunkLimit)
        {
            chunkSize = pngChunkLimit;
        }
        else
        {
            chunkSize = cDataLeft;
        }

        uint32_t cDataSizeNet = toNet(chunkSize);
        memcpy(t, &cDataSizeNet, 4);
        t += 4;
        uint8_t *chunkData = t;
        memcpy(t, "IDAT", 4);
        t += 4;
        memcpy(t, cData.data() + cDataOffset, chunkSize);
        t += chunkSize;
        crc32 = toNet(crc.crc(chunkData, chunkSize + 4));
        memcpy(t, &crc32, 4);
        t += 4;

        cDataOffset += chunkSize;
        cDataLeft -= chunkSize;
    } while (cDataLeft);

    // png_obLT
    if (obl5data.size())
    {
        const size_t obl5size = obl5data.size();
        memcpy(t, obl5data.data(), obl5data.size());
        png_OBL5 &obl5 = *((png_OBL5 *)t);
        obl5.Length = toNet(obl5size - 12);
        uint32_t iCrc = toNet(crc.crc(t + 4, obl5size - 8));
        memcpy(t + obl5size - 4, &iCrc, 4);
        t += obl5size;
    }

    // png_IEND .................................................
    png_IEND &iend = *((png_IEND *)t);
    iend.Lenght = 0;
    memcpy(iend.ChunkType, "IEND", 4);
    iend.CRC = toNet(crc.crc((uint8_t *)"IEND", 4));

    return true;
}

void CFrame::resize(int len, int hei)
{
    CFrame newFrame(len, hei);

    // Copy the original frame
    for (int y = 0; y < std::min(hei, m_height); ++y)
    {
        for (int x = 0; x < std::min(len, m_width); ++x)
        {
            newFrame.at(x, y) = at(x, y);
        }
    }

    //    delete[] m_rgb;
    m_rgb = newFrame.getRGB();
    // newFrame->detach();
    // delete newFrame;

    m_width = len;
    m_height = hei;
}

void CFrame::setTransparency(uint32_t color)
{
    color &= COLOR_MASK;
    for (int i = 0; i < m_width * m_height; ++i)
    {
        if ((m_rgb[i] & COLOR_MASK) == color)
        {
            m_rgb[i] = 0;
        }
    }
}

void CFrame::setTopPixelAsTranparency()
{
    setTransparency(m_rgb[0]);
}

bool CFrame::hasTransparency() const
{
    for (int i = 0; i < m_width * m_height; ++i)
    {
        if (!(m_rgb[i] & ALPHA_MASK))
        {
            return true;
        }
    }

    return false;
}

CFrameSet *CFrame::split(int px, int py)
{
    if (px <= 0 || py < 0 || m_width <= 0 || m_height <= 0)
    {
        m_lastError = "Invalid split parameters: px=" + std::to_string(px) +
                      ", width=" + std::to_string(m_width) + ", height=" + std::to_string(m_height);
        return nullptr;
    }
    auto set = std::make_unique<CFrameSet>();

    int yPieces = py ? m_height / py : 1;
    int xPieces = m_width / px;

    for (int y = 0; y < yPieces; ++y)
    {
        for (int x = 0; x < xPieces; ++x)
        {
            int mx = x * px;
            int my = y * py;
            auto frame = std::unique_ptr<CFrame>(clip(mx, my, px, py ? py : m_height));
            if (!frame)
            {
                m_lastError = "Failed to clip frame at x=" + std::to_string(mx);
                return nullptr;
            }
            set->add(frame.release());
        }
    }
    return set.release();
}

bool CFrame::draw(const std::vector<Dot> &dots, const int penSize, const int mode)
{
    bool changed = false;
    for (size_t i = 0; i < dots.size(); ++i)
    {
        const Dot &dot = dots[i];
        for (int y = 0; y < penSize; ++y)
        {
            for (int x = 0; x < penSize; ++x)
            {
                if (dot.x + x < m_width && dot.y + y < m_height)
                {
                    switch (mode)
                    {
                    case MODE_NORMAL:
                        if (at(dot.x + x, dot.y + y) != dot.color)
                        {
                            at(dot.x + x, dot.y + y) = dot.color;
                            changed = true;
                        }
                        break;

                    case MODE_COLOR_ONLY:
                        if ((at(dot.x + x, dot.y + y) & 0xffffff) != (dot.color & 0xffffff))
                        {
                            uint8_t *p = (uint8_t *)&at(dot.x + x, dot.y + y);
                            uint8_t a = p[3];
                            at(dot.x + x, dot.y + y) = dot.color;
                            p[3] = a;
                            changed = true;
                        }
                        break;

                    case MODE_ALPHA_ONLY:
                        if (alphaAt(dot.x + x, dot.y + y) != (dot.color >> 24))
                        {
                            uint8_t *p = (uint8_t *)&at(dot.x + x, dot.y + y);
                            p[3] = dot.color >> 24;
                            changed = true;
                        }
                        break;
                    }
                }
            }
        }
    }
    return changed;
}

void CFrame::save(const std::vector<Dot> &dots, std::vector<Dot> &dotsD, const int penSize)
{
    for (size_t i = 0; i < dots.size(); ++i)
    {
        const Dot &dot = dots[i];
        for (int y = 0; y < penSize; ++y)
        {
            for (int x = 0; x < penSize; ++x)
            {
                if (dot.x + x < m_width && dot.y + y < m_height)
                {
                    dotsD.emplace_back(Dot{
                        dot.x + x,
                        dot.y + y,
                        at(dot.x + x, dot.y + y),
                    });
                }
            }
        }
    }
}

void CFrame::floodFill(int x, int y, uint32_t bOldColor, uint32_t bNewColor)
{
    if (!isValid(x, y))
    {
        return;
    }

    int ex = x;
    for (; (x >= 0) && at(x, y) == bOldColor; --x)
    {
        at(x, y) = bNewColor;
        if ((y > 0) && (at(x, y - 1) == bOldColor))
        {
            floodFill(x, y - 1, bOldColor, bNewColor);
        }

        if ((y < m_height - 1) && (at(x, y + 1) == bOldColor))
        {
            floodFill(x, y + 1, bOldColor, bNewColor);
        }
    }

    x = ++ex;
    if (!isValid(x, y))
    {
        return;
    }

    for (; (x < m_width) && at(x, y) == bOldColor; ++x)
    {
        at(x, y) = bNewColor;
        if ((y > 0) && (at(x, y - 1) == bOldColor))
        {
            floodFill(x, y - 1, bOldColor, bNewColor);
        }

        if ((y < m_height - 1) && (at(x, y + 1) == bOldColor))
        {
            floodFill(x, y + 1, bOldColor, bNewColor);
        }
    }
}

void CFrame::floodFillAlpha(int x, int y, uint8_t oldAlpha, uint8_t newAlpha)
{
    if (!isValid(x, y))
    {
        return;
    }

    int ex = x;
    for (; (x >= 0) && alphaAt(x, y) == oldAlpha; --x)
    {
        uint8_t *p = (uint8_t *)&at(x, y);
        p[3] = newAlpha;
        if ((y > 0) && (alphaAt(x, y - 1) == oldAlpha))
        {
            floodFillAlpha(x, y - 1, oldAlpha, newAlpha);
        }

        if ((y < m_height - 1) && (alphaAt(x, y + 1) == oldAlpha))
        {
            floodFillAlpha(x, y + 1, oldAlpha, newAlpha);
        }
    }

    x = ++ex;
    if (!isValid(x, y))
    {
        return;
    }

    for (; (x < m_width) && alphaAt(x, y) == oldAlpha; ++x)
    {
        uint8_t *p = (uint8_t *)&at(x, y);
        p[3] = newAlpha;
        if ((y > 0) && (alphaAt(x, y - 1) == oldAlpha))
        {
            floodFillAlpha(x, y - 1, oldAlpha, newAlpha);
        }

        if ((y < m_height - 1) && (alphaAt(x, y + 1) == oldAlpha))
        {
            floodFillAlpha(x, y + 1, oldAlpha, newAlpha);
        }
    }
}

CFrame *CFrame::clip(int mx, int my, int cx, int cy)
{
    int maxLen = m_width - mx;
    int maxHei = m_height - my;

    if (cx == -1)
    {
        cx = maxLen;
    }

    if (cy == -1)
    {
        cy = maxHei;
    }

    // check if out of bound
    if (cx < 1 || cy < 1)
    {
        cx = cy = 0;
    }

    // create clipped frame
    CFrame *t = new CFrame(cx, cy);

    // copy clipped region
    for (int y = 0; y < cy; ++y)
    {
        for (int x = 0; x < cx; ++x)
        {
            t->at(x, y) = at(mx + x, my + y);
        }
    }

    // return new frame
    return t;
}

void CFrame::flipV()
{
    for (int y = 0; y < m_height / 2; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            uint32_t c = at(x, y);
            at(x, y) = at(x, m_height - y - 1);
            at(x, m_height - y - 1) = c;
        }
    }
}

void CFrame::flipH()
{
    for (int y = 0; y < m_height; ++y)
    {
        for (int x = 0; x < m_width / 2; ++x)
        {
            uint32_t c = at(x, y);
            at(x, y) = at(m_width - x - 1, y);
            at(m_width - x - 1, y) = c;
        }
    }
}

void CFrame::rotate()
{
    CFrame newFrame(m_height, m_width);
    for (int y = 0; y < m_height; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            newFrame.at(newFrame.m_width - y - 1, x) = at(x, y);
        }
    }

    m_width = newFrame.m_width;
    m_height = newFrame.m_height;
    m_rgb = newFrame.getRGB();
}

void CFrame::shrink()
{
    CFrame newFrame(m_width / 2, m_height / 2);
    for (int y = 0; y < m_height / 2; ++y)
    {
        for (int x = 0; x < m_width / 2; ++x)
        {
            newFrame.at(x, y) = at(x * 2, y * 2);
        }
    }

    m_rgb = newFrame.getRGB();
    m_width /= 2;
    m_height /= 2;
}

void CFrame::enlarge()
{
    CFrame newFrame(m_width * 2, m_height * 2);
    for (int y = 0; y < m_height; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            uint32_t c = at(x, y);
            newFrame.at(x * 2, y * 2) = c;
            newFrame.at(x * 2 + 1, y * 2) = c;
            newFrame.at(x * 2, y * 2 + 1) = c;
            newFrame.at(x * 2 + 1, y * 2 + 1) = c;
        }
    }

    m_rgb = newFrame.getRGB();

    m_width *= 2;
    m_height *= 2;
}

void CFrame::shiftUP(bool wrap)
{
    if (m_height <= 1 || m_width <= 0)
    {
        m_lastError = "Invalid dimensions for shiftUP: " + std::to_string(m_width) + "x" + std::to_string(m_height);
        return;
    }

    // Save top row
    std::vector<uint32_t> topRow(m_rgb.begin(), m_rgb.begin() + m_width);

    // Shift pixels up
    std::copy(m_rgb.begin() + m_width, m_rgb.end(), m_rgb.begin());

    // Handle bottom row
    if (wrap)
    {
        std::copy(topRow.begin(), topRow.end(), m_rgb.end() - m_width);
    }
    else
    {
        std::fill(m_rgb.end() - m_width, m_rgb.end(), 0);
    }
}

void CFrame::shiftDOWN(bool wrap)
{
    if (m_height <= 1 || m_width <= 0)
    {
        m_lastError = "Invalid dimensions for shiftDOWN: " + std::to_string(m_width) + "x" + std::to_string(m_height);
        return;
    }

    // Save bottom row
    std::vector<uint32_t> bottomRow(m_rgb.end() - m_width, m_rgb.end());

    // Shift pixels down
    std::copy_backward(m_rgb.begin(), m_rgb.end() - m_width, m_rgb.end());

    // Handle top row
    if (wrap)
    {
        std::copy(bottomRow.begin(), bottomRow.end(), m_rgb.begin());
    }
    else
    {
        std::fill(m_rgb.begin(), m_rgb.begin() + m_width, 0);
    }
}

void CFrame::shiftLEFT(const bool wrap)
{
    if (m_height <= 1 || m_width <= 0)
    {
        m_lastError = "Invalid dimensions for shiftLEFT: " + std::to_string(m_width) + "x" + std::to_string(m_height);
        return;
    }
    for (int y = 0; y < m_height; ++y)
    {
        const uint32_t c = at(0, y);
        for (int x = 0; x < m_width - 1; ++x)
        {
            at(x, y) = at(x + 1, y);
        }
        if (wrap)
            at(m_width - 1, y) = c;
        else
            at(m_width - 1, y) = 0;
    }
}

void CFrame::shiftRIGHT(const bool wrap)
{
    if (m_height <= 1 || m_width <= 0)
    {
        m_lastError = "Invalid dimensions for shiftRIGHT: " + std::to_string(m_width) + "x" + std::to_string(m_height);
        return;
    }
    for (int y = 0; y < m_height; ++y)
    {
        const uint32_t c = at(m_width - 1, y);
        for (int x = 0; x < m_width - 1; ++x)
        {
            at(m_width - 1 - x, y) = at(m_width - 2 - x, y);
        }
        if (wrap)
            at(0, y) = c;
        else
            at(0, y) = 0;
    }
}

bool CFrame::isEmpty() const
{
    for (int y = 0; y < m_height; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            if ((m_rgb[x + y * m_width] & 0xff000000))
            {
                return false;
            }
        }
    }
    return true;
}

void CFrame::inverse()
{
    for (int y = 0; y < m_height; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            unsigned int &rgb = at(x, y);
            rgb = (~rgb & 0xffffff) + (rgb & 0xff000000);
        }
    }
}

void CFrame::copy(const CFrame *src)
{
    if (!src)
    {
        clear();
        return;
    }
    m_rgb = src->m_rgb;
    m_width = src->m_width;
    m_height = src->m_height;
}

void CFrame::shadow(int factor)
{
    for (int y = 0; y < m_height; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            unsigned int &rgb = at(x, y);
            rgb = (rgb & 0xffffff) + (rgb & 0xff000000) / factor;
        }
    }
}

void CFrame::fade(int factor)
{
    for (int y = 0; y < m_height; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            unsigned int &rgb = at(x, y);
            rgb = (rgb & 0xffffff) + ((((rgb & 0xff000000) >> 24) * factor / 255) << 24);
        }
    }
}

CFrameSet *CFrame::explode(int count, uint16_t *sx, uint16_t *sy, CFrameSet *set)
{
    if (!set)
    {
        set = new CFrameSet();
    }

    int mx = 0;
    for (int i = 0; i < count; ++i)
    {
        CFrame *frame = clip(mx, 0, sx[i], sy[i]);
        set->add(frame);
        mx += sx[i];
    }
    return set;
}

CFrameSet *CFrame::explode(std::vector<CFrame::oblv2DataUnit_t> &metadata, CFrameSet *set)
{
    constexpr uint16_t INVALID = 0xffff;
    if (!set)
    {
        set = new CFrameSet();
    }

    for (const auto &unit : metadata)
    {
        CFrame *frame;
        if (unit.x != INVALID && unit.y != INVALID)
        {
            frame = clip(unit.x, unit.y, unit.sx, unit.sy);
        }
        else
        {
            frame = new CFrame(unit.sx, unit.sy);
        }
        set->add(frame);
    }
    return set;
}

void CFrame::abgr2argb()
{
    // swap blue/red
    for (int i = 0; i < m_width * m_height; ++i)
    {
        uint32_t t = (m_rgb[i] & 0xff00ff00);
        if (t & 0xff000000)
        {
            t += ((m_rgb[i] & 0xff) << 16) + ((m_rgb[i] & 0xff0000) >> 16);
        }
        m_rgb[i] = t;
    }
}

void CFrame::argb2arbg()
{
    // swap green/blue
    for (int i = 0; i < m_width * m_height; ++i)
    {
        uint32_t t = (m_rgb[i] & 0xff0000ff);
        if (t & 0xff000000)
        {
            t += ((m_rgb[i] & 0xff00) << 8) + ((m_rgb[i] & 0xff0000) >> 8);
        }
        m_rgb[i] = t;
    }
}

const char *CFrame::getChunkType()
{
    return "obLT";
}

void CFrame::fill(unsigned int rgba)
{
    for (int i = 0; i < m_width * m_height; ++i)
    {
        m_rgb[i] = rgba;
    }
}

void CFrame::drawAt(CFrame &frame, int bx, int by, bool tr)
{
    for (int y = 0; y < frame.m_height; ++y)
    {
        if (by + y >= m_height)
        {
            break;
        }
        for (int x = 0; x < frame.m_width; ++x)
        {
            if (bx + x >= m_width)
            {
                break;
            }
            if (!tr || frame.at(x, y))
                at(bx + x, by + y) = frame.at(x, y);
        }
    }
}
