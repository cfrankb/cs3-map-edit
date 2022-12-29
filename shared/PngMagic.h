/*
    LGCK Builder Runtime
    Copyright (C) 1999, 2020  Francois Blanchette

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
#ifndef PNGMAGIC_H
#define PNGMAGIC_H

#include <stdint.h>
class CFrame;
class CFrameSet;
class IFile;
//typedef struct CFrame png_IHDR;

class CPngMagic
{
public:
    CPngMagic();
    bool parsePNG(CFrameSet & set, IFile &file);

protected:

    typedef struct {
        uint32_t Lenght;      // 4 UINT8s
        uint8_t ChunkType[4];
        uint32_t Width;       //: 4 uint8_ts
        uint32_t Height;      //: 4 uint8_ts
        uint8_t BitDepth;     //: 1 uint8_t
        uint8_t ColorType;    //: 1 uint8_t
        uint8_t Compression;  //: 1 uint8_t
        uint8_t Filter;       //: 1 uint8_t
        uint8_t Interlace;    //: 1 uint8_t
    } png_IHDR;

    static uint8_t PaethPredictor(uint8_t a, uint8_t b, uint8_t c);
    bool _8bpp(
            CFrame *& frame,
            uint8_t* cData,
            int cDataSize,
            const png_IHDR & ihdr,
            const uint8_t plte[][3],
            const bool trns_found,
            const uint8_t trns[],
            int offsetY);
    bool _4bpp(
            CFrame *& frame,
            uint8_t* cData,
            int cDataSize,
            const png_IHDR & ihdr,
            const uint8_t plte[][3],
            const bool trns_found,
            const uint8_t trns[],
            int offsetY);
};

#endif // PNGMAGIC_H
