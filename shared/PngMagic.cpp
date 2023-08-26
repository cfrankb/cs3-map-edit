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

/* These describe the color_type field in png_info. */
/* color type masks */
#define PNG_COLOR_MASK_PALETTE    1
#define PNG_COLOR_MASK_COLOR      2
#define PNG_COLOR_MASK_ALPHA      4

/* color types.  Note that not all combinations are legal */
#define PNG_COLOR_TYPE_GRAY 0
#define PNG_COLOR_TYPE_PALETTE  (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_PALETTE)
#define PNG_COLOR_TYPE_RGB        (PNG_COLOR_MASK_COLOR)
#define PNG_COLOR_TYPE_RGB_ALPHA  (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_ALPHA)
#define PNG_COLOR_TYPE_GRAY_ALPHA (PNG_COLOR_MASK_ALPHA)
/* aliases */
#define PNG_COLOR_TYPE_RGBA  PNG_COLOR_TYPE_RGB_ALPHA
#define PNG_COLOR_TYPE_GA  PNG_COLOR_TYPE_GRAY_ALPHA

CPngMagic::CPngMagic()
{
}

uint8_t CPngMagic::PaethPredictor(uint8_t a, uint8_t b, uint8_t c)
{
     int p = a + b - c ; //initial estimate
     int pa = std::abs((float)p - a); //distances to a, b, c
     int pb = std::abs((float)p - b); //
     int pc = std::abs((float)p - c);
              //; return nearest of a,b,c,
              //; breaking ties in order a,b,c.
     if ((pa <= pb) && (pa <= pc)) {
         return a;
     } else if (pb <= pc) {
         return b;
     } else return c;
}

bool CPngMagic::parsePNG(CFrameSet &set, IFile &file)
{
//    int size = 0;
    CCRC crc;
    int pos = 8;
    int fileSize = file.getSize();
    //printf("fileSize: %d\n", fileSize);

    file.seek( 8 );
    int chunkSize;

    png_IHDR ihdr;
    memset(&ihdr, 0, sizeof(png_IHDR));

    uint8_t plte[256][3];
    uint8_t trns[256];
    for (int i=0; i < 256; i++) {
        trns[i] = 255;
    }

    char png_chunk_OBL5[4];
    memcpy(png_chunk_OBL5, CFrame::getChunkType(),4);

    bool iend_found = false;
    //bool ihdr_found = false;
    //bool plte_found = false;
    bool trns_found = false;
    //bool oblt_found = false;

    uint8_t *cData = nullptr;
    int obl5t_count = 0;
    short *obl5t_xx = nullptr;
    short *obl5t_yy = nullptr;
    int cDataSize = 0;

    while (pos < fileSize) {
        file.read(&chunkSize, 4);
        chunkSize = CFrame::toNet(chunkSize);
        pos += 4;

        char chunkType[5];
        chunkType[4] = 0;
        file.read(chunkType, 4); pos += 4;

        uint8_t * p = new uint8_t [ chunkSize + 4] ;
        file.read(p + 4, chunkSize); pos += chunkSize;
        //memset(chunkData + chunkSize, 0, 4);
        memcpy(p, chunkType, 4);
        uint8_t *chunkData = p + 4;

        int crc32;
        file.read(&crc32, 4); pos += 4;
        crc32 = CFrame::toNet(crc32);
        // TODO: check crc32 values for each chunk
        int crc32c = crc.crc(p, chunkSize +4);
        //qDebug("crc32: %.8x %.8x", crc32, crc32c);
        if (crc32 != crc32c) {
            set.setLastError("CRC32 checksum doesn't match");
            delete [] p;
            return false;
        }

        if (memcmp (chunkType, "IHDR", 4) == 0) {
            memcpy( ((uint8_t*)&ihdr) + 8, chunkData, chunkSize );
            memcpy(ihdr.ChunkType, chunkType, 4);
            ihdr.Lenght = chunkSize;
            /*
            qDebug("Width: %d, Height: %d", CFrame::toNet(ihdr.Width), CFrame::toNet(ihdr.Height));
            qDebug("BitDepth: %d", ihdr.BitDepth);
            qDebug("ColorType: %d", ihdr.ColorType);
            qDebug("Compression: %d", ihdr.Compression);
            qDebug("Filter: %d", ihdr.Filter);
            qDebug("Interlace: %d\n", ihdr.Interlace);
            //              ihdr_found = true;
            */
        }

        if (memcmp (chunkType, "PLTE", 4) == 0) {
            memcpy(plte, chunkData, chunkSize);
            //            plte_found = true;
        }

        if (memcmp (chunkType, "IDAT", 4) == 0) {
            if (cData) {
                uint8_t *tmp = new uint8_t [ cDataSize + chunkSize ];
                memcpy( tmp, cData, cDataSize );
                delete [] cData;
                cData = tmp;
                memcpy( cData + cDataSize, chunkData, chunkSize);
            } else {
                cData = new uint8_t [ chunkSize ];
                memcpy( cData, chunkData, chunkSize);
            }
            cDataSize += chunkSize;
        }

        if (memcmp (chunkType, "tRNS", 4) == 0) {
            memcpy(trns, chunkData, chunkSize);
            trns_found = true;
        }

        if (memcmp (chunkType, "IEND", 4) == 0) {
            iend_found = true;
        }

        if (memcmp (chunkType, png_chunk_OBL5, 4) == 0) {
            //oblt_found = true;
            CFrame::png_OBL5 obl5t;
            char *t = (char*)&obl5t;
            memcpy(t + 8, chunkData, 12);
            if (obl5t.Version == 0) {
                // only version 0x0000 is supported
                obl5t_count = obl5t.Count;
                obl5t_xx = new short[obl5t.Count];
                obl5t_yy = new short[obl5t.Count];
                memcpy(obl5t_xx, chunkData + 12,
                       sizeof(short) * obl5t.Count);
                memcpy(obl5t_yy, chunkData + 12 + sizeof(short) * obl5t.Count,
                       sizeof(short) * obl5t.Count);
            }
        }

        delete [] p;//chunkData;
    }

    bool valid = false;
    if (cData && iend_found) {
        //qDebug("total cData:%d\n", cDataSize);
        if ( !ihdr.Interlace &&
             ((ihdr.BitDepth == 8) || (ihdr.BitDepth == 4)) &&
             (ihdr.ColorType != PNG_COLOR_TYPE_GRAY) &&
             (ihdr.ColorType != PNG_COLOR_TYPE_GRAY_ALPHA) ) {
            int height = CFrame::toNet(ihdr.Height);
            int width = CFrame::toNet(ihdr.Width);
            int offsetY = 0;
            if (width & 7) {
                width += (8 - (width & 7));
            }
            if (height & 7) {
                offsetY = 8 - (height & 7);
                height += offsetY;
            }
            CFrame *frame = new CFrame(width, height);
            memset(frame->getRGB(), 0, sizeof(uint32_t) * frame->len() * frame->hei());

            if (ihdr.BitDepth == 8) {
                valid = _8bpp(frame, cData, cDataSize, ihdr, plte, trns_found, trns, offsetY);
            }

            if (ihdr.BitDepth == 4) {
                valid = _4bpp(frame, cData, cDataSize, ihdr, plte, trns_found, trns, offsetY);
            }

           // qDebug("valid: %d", valid);
            if (valid) {
                if (obl5t_count) {
                    frame->explode(obl5t_count,obl5t_xx,obl5t_yy,&set);
                    delete[] obl5t_xx;
                    delete[] obl5t_yy;
                    delete frame;
                } else {
                    frame->updateMap();
                    set.add(frame);
                }
            } else {
                set.setLastError("unsupported png filtering");
                delete frame;
          //      size = 0;
            }
        } else {
            set.setLastError("unsupported png");
        }
        delete [] cData;
    }
    return valid;
}

bool CPngMagic::_8bpp(
        CFrame *&frame,
        uint8_t *cData,
        int cDataSize,
        const png_IHDR &ihdr,
        const uint8_t plte[][3],
        const bool trns_found,
        const uint8_t trns[],
        int offsetY)
{
    int pixelWidth = -1;

    switch (ihdr.ColorType) {
    case PNG_COLOR_TYPE_PALETTE:
        pixelWidth = 1;
        break;
    case PNG_COLOR_TYPE_RGB:
        pixelWidth = 3;
        break;
    case PNG_COLOR_TYPE_RGB_ALPHA:
        pixelWidth = 4;
    }

    //printf("pixelWidth %d\n", pixelWidth);

    int pitch = CFrame::toNet(ihdr.Width) * pixelWidth + 1;
    //printf("pixelWidth: %d\n", pixelWidth);
    //printf("pitch: %d\n", pitch);

    uLong dataSize = pitch * CFrame::toNet(ihdr.Height);
    uint8_t *data = new uint8_t [ dataSize ];
    //    printf("total data:%d\n", ((int)dataSize));

    int err = uncompress(
                (uint8_t *)data,
                (uLong *)& dataSize,
                (uint8_t *)cData,
                (uLong) cDataSize);

    if (err) {
//        set.setLastError("error in decomp");
        //qDebug("error in decomp");
        delete [] data;
        return false;
    }

    bool valid = true;
    uint32_t *rgb = frame->getRGB();

    for (int y=0; y < (int) CFrame::toNet(ihdr.Height); y++) {
        uint8_t *line = & data[pitch * y + 1];
        uint8_t *prLine = nullptr;
        if (y) {
            prLine = & data[pitch * (y - 1) + 1];
        }
        uint8_t filtering = data[pitch * y];
        bool handled = true;

        switch ( filtering ) {
        case 0:
            break;

        case 1:
            for (int x=0; x < (int) CFrame::toNet(ihdr.Width); x++) {

                uint32_t p = 0;
                uint32_t c = 0;
                if (x) {
                    memcpy(&p, & line[ (x - 1) * pixelWidth ], pixelWidth);
                }

                memcpy(&c, & line[ x * pixelWidth ], pixelWidth);

                uint8_t *pc = (uint8_t*)&c;
                uint8_t *pp = (uint8_t*)&p;

                for (int i=0; i < pixelWidth; i++) {
                    pc [i] += pp [i];
                }

                memcpy(& line[ x * pixelWidth ], pc, pixelWidth);
            }
            break;

        case 2:
            for (int x=0; x < (int) CFrame::toNet(ihdr.Width); x++) {

                uint32_t p = 0;
                uint32_t c = 0;
                if (y) {
                    memcpy(&p, & prLine[ x * pixelWidth ], pixelWidth);
                }

                memcpy(&c, & line[ x * pixelWidth ], pixelWidth);

                uint8_t *pc = (uint8_t*)&c;
                uint8_t *pp = (uint8_t*)&p;

                for (int i=0; i < pixelWidth; i++) {
                    pc [i] += pp [i];
                }

                memcpy(& line[ x * pixelWidth ], & c, pixelWidth);
            }
            break;

        case 3:
            for (int x=0; x < (int) CFrame::toNet(ihdr.Width); x++) {

                uint32_t a = 0; // left
                uint32_t b = 0; // above (Prior)

                if (x) {
                    memcpy(&a, & line[ (x - 1) * pixelWidth ], pixelWidth);
                }

                if (y) {
                    memcpy(&b, & prLine[ x * pixelWidth ], pixelWidth);
                }

                uint8_t *pa = (uint8_t*) &a;
                uint8_t *pb = (uint8_t*) &b;
                uint8_t *ph = & line[ x * pixelWidth ];

                for (int i=0; i < pixelWidth; i++) {
                    ph [i] += (pa [i] + pb[i]) / 2;
                }
            }

            break;

        case 4:
            for (int x = 0; x < (int) CFrame::toNet(ihdr.Width); x++) {
                uint32_t a = 0; // left
                uint32_t b = 0; // above
                uint32_t c = 0; // above-left

                if (x) {
                    memcpy(&a, & line[ (x - 1) * pixelWidth ], pixelWidth);
                }

                if (y) {
                    memcpy(&b, & prLine[ x * pixelWidth ], pixelWidth);
                    if (x) {
                        memcpy(&c, & prLine[ (x - 1) * pixelWidth ], pixelWidth);
                    }
                }

                uint8_t *pa = (uint8_t*) &a;
                uint8_t *pb = (uint8_t*) &b;
                uint8_t *pc = (uint8_t*) &c;

                uint8_t *ph = & line[ x * pixelWidth ];
                for (int i=0; i< pixelWidth; i++) {
                    ph[i] += PaethPredictor(pa[i], pb[i], pc[i]);
                }
            }
            break;

        default:
            handled = false;
        }

        if (handled) {
            for (int x=0; x < (int) CFrame::toNet(ihdr.Width); x++) {
                uint32_t rgba = 0xff000000;
                switch (pixelWidth) {
                case 1:
                    memcpy(&rgba, plte[ line [ x ] ], 3);

                    if (trns_found) {
                        rgba &= (trns[ line [ x ] ] * 0x1000000) + 0xffffff;
                    }
                    break;

                case 3:
                case 4:
                    memcpy(&rgba, & line [ x * pixelWidth ], pixelWidth );
                    break;
                }

                rgb [(offsetY + y) * frame->len() + x] = rgba;
            }
        } else {
            valid = false;
        }
    }

    delete [] data;
    return valid;
}

bool CPngMagic::_4bpp(
        CFrame *&frame,
        uint8_t *cData,
        int cDataSize,
        const png_IHDR &ihdr,
        const uint8_t plte[][3],
        const bool trns_found,
        const uint8_t trns[],
        int offsetY)
{
    int height = CFrame::toNet(ihdr.Height);
    int width = CFrame::toNet(ihdr.Width);

    if (ihdr.ColorType != PNG_COLOR_TYPE_PALETTE){
       // qDebug("colorType is not PNG_COLOR_TYPE_PALETTE");
        return false;
    }

    int pitch = 1 + CFrame::toNet(ihdr.Width) / 2;
    if (CFrame::toNet(ihdr.Width) & 1) {
        ++pitch;
    }
    //printf("pixelWidth: %d\n", pixelWidth);
    //printf("pitch: %d\n", pitch);

    uint64_t dataSize = pitch * CFrame::toNet(ihdr.Height);
    uint8_t *data = new uint8_t [ dataSize ];
    //    printf("total data:%d\n", ((int)dataSize));

    int err = uncompress(
                (uint8_t *)data,
                (uLong *)& dataSize,
                (uint8_t *)cData,
                (uLong) cDataSize);

    if (err) {
//        set.setLastError("error in decomp");
    //    qDebug("error in decomp");
        delete [] data;
        return false;
    }

    bool valid = true;
    uint32_t *rgb = frame->getRGB();

    for (int y=0; y < height; y++) {
        uint8_t *line = & data[pitch * y + 1];
        //uint8_t *prLine = nullptr;
        //if (y) {
          //  prLine = & data[pitch * (y - 1) + 1];
        //}
        uint8_t filtering = data[pitch * y];
        bool handled = true;

        switch ( filtering ) {
        case 0:
            break;
        default:
            handled = false;
        //    qDebug("unsupported filter: %d", filtering);
        }

        if (handled) {
            for (int x=0; x < width; x++) {
                uint32_t rgba = 0;
                uint8_t index = line [ x / 2 ];
                if (x & 1) {
                    index &= 0x0f;
                } else {
                    index = index >> 4;
                }

                memcpy(&rgba, plte[ index ], 3);
                if (trns_found) {
                    rgba |= (trns[ index ] << 24);
                } else {
                    rgba |= 0xff000000;
                }

                rgb [(offsetY + y) * frame->len() + x] = rgba;
            }
        } else {
            valid = false;
        }
    }

    return valid;
}
