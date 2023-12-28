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

/////////////////////////////////////////////////////////////////////////////
// CFrame

#ifndef _SS3FRAME_H
#define _SS3FRAME_H
#include <stdint.h>

class CFrameSet;
class CDotArray;
class CSS3Map;
class CUndo;
class IFile;


// Frame.h : header file
//

// CSS3Map

class CSS3Map
{
public:
    CSS3Map();
    CSS3Map(const int px, const int py);
    ~CSS3Map();

    void resize(const int px, const int py);
    inline char & at(const int x, const int y) {
        return m_map[ x + y * m_len];
    }

    int length() const;
    int height() const;

    void read(IFile &file);
    void write(IFile & file) const;
    bool isNULL() const;

    CSS3Map & operator = (const CSS3Map & src);
    char * getMap() const;

    enum {
        GRID = 8
    };

protected:

    char *m_map;
    int m_len;
    int m_hei;
};

// CFrame

class CFrame
{
    // Construction
public:
    CFrame(CFrame* src=nullptr);
    CFrame(int p_nLen, int p_nHei);

    // Attributes
public:
    inline bool isValid (int x, int y) {
        return x>= 0 && x < m_nLen && y >=0 && y < m_nHei;
    }

    inline uint32_t & at(int x, int y) {
        return m_rgb[x + y * m_nLen];
    }

    inline uint8_t alphaAt(int x, int y) {
        return (m_rgb[x + y * m_nLen]) >> 24;
    }

    inline uint32_t & point (int x, int y) {
        return * (m_rgb + x + y * m_nLen);
    }

    inline uint32_t *getRGB() const { return m_rgb; }
    void setRGB(uint32_t *rgb) { m_rgb = rgb; }
    inline char map(int x, int y) { return m_map.at(x,y); }
    bool hasTransparency() const;
    bool isEmpty() const;

    // Operations
public:
    CFrame & operator = (const CFrame &src);
    void forget();
    void detach() { m_rgb = nullptr;}
    void updateMap();
    void resize( int len, int hei );
    void setTransparency( uint32_t rgba );
    void setTopPixelAsTranparency();
    void enlarge();    
    void flipV();
    void flipH();
    void rotate();
    CFrameSet *split(int pxSize, bool whole=true);
    void spreadH();
    void spreadV();
    void clear();
    void shrink();
    const CSS3Map & getMap() const;
    void shiftUP();
    void shiftDOWN();
    void shiftLEFT();
    void shiftRIGHT();
    void inverse();
    void shadow(int factor);
    static const char* getChunkType();
    void abgr2argb();
    void argb2arbg();
    void floodFill (int x, int y, uint32_t bOldColor, uint32_t bNewColor);
    void floodFillAlpha (int x, int y, uint8_t oldAlpha, uint8_t newAlpha);
    void fade(int factor);
    CFrame* toAlphaGray(int mx=0, int my=0, int cx=-1, int cy=-1);
    void fill(unsigned int rgba);

    // Implementation
public:
    ~CFrame();
    bool read (IFile & file, int version);
    void write (IFile &file);

    void toBmp(uint8_t * & bmp, int & size);
    void toPng(uint8_t * & png, int & size, uint8_t *obl5data=nullptr, int obl5size=0);
    static uint32_t toNet(const uint32_t a);
    static const uint32_t *dosPal();
    bool draw(CDotArray * dots, int size, int mode=MODE_NORMAL);
    void save(CDotArray * dots, CDotArray * dotsOrg, int size);
    CFrame *clip(int mx, int my, int cx=-1, int cy=-1);
    CFrameSet *explode(int count, short *xx, short *yy, CFrameSet *set=nullptr);

    void copy(CFrame *);
    void push();
    void undo();
    void redo();
    bool canUndo();
    bool canRedo();

    inline int len() const { return m_nLen; }
    inline int hei() const { return m_nHei; }

    enum {
        ALPHA_MASK = 0xff000000,
        COLOR_MASK = 0x00ffffff,
        MODE_ZLIB_ALPHA = -1,
        bmpDataOffset = 54,
        bmpHeaderSize = 40,
        pngHeaderSize = 8,
        png_IHDR_Size = 21,
        pngChunkLimit = 32767,
        MAX_UNDO = 20
    };

    typedef struct {
        uint32_t Lenght;      // 4 uint8_ts
        uint8_t ChunkType[4];
        uint32_t Width;       //: 4 uint8_ts
        uint32_t Height;      //: 4 uint8_ts
        uint8_t BitDepth;     //: 1 uint8_t
        uint8_t ColorType;    //: 1 uint8_t
        uint8_t Compression;  //: 1 uint8_t
        uint8_t Filter;       //: 1 uint8_t
        uint8_t Interlace;    //: 1 uint8_t
    } png_IHDR;

    typedef struct {
        uint32_t Lenght;      // 4 uint8_ts
        uint8_t ChunkType[4];
        uint32_t CRC;
    } png_IEND;

    typedef struct {
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

protected:    
    enum {
        MODE_NORMAL ,
        MODE_COLOR_ONLY,
        MODE_ALPHA_ONLY
    };

    int m_nLen;
    int m_nHei;

    int m_bCustomMap;
    uint32_t *m_rgb;
    CSS3Map m_map;
    CFrame **m_undoFrames;
    int m_undoPtr;
    int m_undoSize;
    void init();
};

/////////////////////////////////////////////////////////////////////////////

#endif
