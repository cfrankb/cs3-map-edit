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

#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>
#include "DotArray.h"

class CFrameSet;
class CDotArray;
class CSS3Map;
class CUndo;
class IFile;

class CFrame
{
public:
    CFrame(int width = 0, int height = 0);
    CFrame(const CFrame &src);
    CFrame(CFrame &&src) noexcept;
    ~CFrame() = default;

    CFrame &operator=(CFrame src);
    friend void swap(CFrame &a, CFrame &b) noexcept;

    inline bool isValid(int x, int y) const
    {
        return x >= 0 && x < m_width && y >= 0 && y < m_height;
    }

    inline uint32_t &at(int x, int y)
    {
        if (!isValid(x, y))
            throw std::out_of_range("Invalid pixel access");
        return m_rgb[x + y * m_width];
    }

    inline uint8_t alphaAt(int x, int y) const
    {
        if (!isValid(x, y))
            throw std::out_of_range("Invalid pixel access");
        return m_rgb[x + y * m_width] >> 24;
    }

    inline std::vector<uint32_t> &getRGB() { return m_rgb; }
    void setRGB(std::vector<uint32_t> &rgb) { m_rgb = std::move(rgb); }
    bool hasTransparency() const;
    bool isEmpty() const;

    CFrame &operator=(const CFrame &src);
    void clear();
    void resize(int len, int hei);
    void setTransparency(uint32_t rgba);
    void setTopPixelAsTranparency();
    void enlarge();
    void flipV();
    void flipH();
    void rotate();
    // CFrameSet *split(int pxSize, bool whole = true);
    CFrameSet *split(int pxSize, int pySize);
    void shrink();
    const CSS3Map &getMap() const;
    void shiftUP(const bool wrap = true);
    void shiftDOWN(const bool wrap = true);
    void shiftLEFT(const bool wrap = true);
    void shiftRIGHT(const bool wrap = true);
    void inverse();
    void shadow(int factor);
    static const char *getChunkType();
    void abgr2argb();
    void argb2arbg();
    void floodFill(int x, int y, uint32_t bOldColor, uint32_t bNewColor);
    void floodFillAlpha(int x, int y, uint8_t oldAlpha, uint8_t newAlpha);
    void fade(int factor);
    CFrame *toAlphaGray(int mx = 0, int my = 0, int cx = -1, int cy = -1);
    void fill(unsigned int rgba);
    void drawAt(CFrame &frame, int bx, int by, bool tr);

    bool read(IFile &file);
    bool write(IFile &file);

    void toBmp(uint8_t *&bmp, int &size);
    bool toPng(std::vector<uint8_t> &png, const std::vector<uint8_t> &obl5data = {});

    static uint32_t toNet(const uint32_t a);
    bool draw(const std::vector<Dot> &dots, const int penSize, const int mode = MODE_NORMAL);
    void save(const std::vector<Dot> &dots, std::vector<Dot> &dotsD, const int penSize);
    CFrame *clip(int mx, int my, int cx = -1, int cy = -1);

    void copy(const CFrame *);
    inline int width() const { return m_width; }
    inline int height() const { return m_height; }

    const char *getLastError()
    {
        return m_lastError.c_str();
    }

    enum
    {
        ALPHA_MASK = 0xff000000,
        COLOR_MASK = 0x00ffffff,
        MODE_ZLIB_ALPHA = -1,
        bmpDataOffset = 54,
        bmpHeaderSize = 40,
        pngHeaderSize = 8,
        png_IHDR_Size = 21,
        pngChunkLimit = 32767,
        OBL5_UNPACKED = 0x500,
    };

    typedef struct
    {
        uint32_t Lenght; // 4 uint8_ts
        uint8_t ChunkType[4];
        uint32_t Width;      //: 4 uint8_ts
        uint32_t Height;     //: 4 uint8_ts
        uint8_t BitDepth;    //: 1 uint8_t
        uint8_t ColorType;   //: 1 uint8_t
        uint8_t Compression; //: 1 uint8_t
        uint8_t Filter;      //: 1 uint8_t
        uint8_t Interlace;   //: 1 uint8_t
    } png_IHDR;

    typedef struct
    {
        uint32_t Lenght; // 4 uint8_ts
        uint8_t ChunkType[4];
        uint32_t CRC;
    } png_IEND;

    typedef struct
    {
        uint32_t Length;      // sizeof all - 12
        uint8_t ChunkType[4]; // OBL5
        uint32_t Reserved;    // should be zero
        uint32_t Version;     // should be zero
        uint32_t Count;       // m_nSize
        // Data : size m_nSize * 4 bytes
        // ...   list of width (short)
        // ...   list of height (short)
        // CRC  : size 4
    } png_OBL5;

    struct oblv2DataUnit_t
    {
        uint16_t x;  //                   data = 4 * 2 (uint16_t) * frameCount
        uint16_t y;  //
        uint16_t sx; //
        uint16_t sy; //
    };

    CFrameSet *explode(int count, uint16_t *sx, uint16_t *sy, CFrameSet *set = nullptr);
    CFrameSet *explode(std::vector<oblv2DataUnit_t> &metadata, CFrameSet *set = nullptr);

private:
    enum
    {
        MODE_NORMAL,
        MODE_COLOR_ONLY,
        MODE_ALPHA_ONLY
    };

    std::vector<uint32_t> m_rgb;
    int m_width;
    int m_height;
    std::string m_lastError;
};
