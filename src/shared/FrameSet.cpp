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

// FrameSet.cpp : implementation file
//
#include <cstring>
#include <cstdio>
#include <stdint.h>
#include "FrameSet.h"
#include "Frame.h"
#include <zlib.h>
#include "IFile.h"
#include "PngMagic.h"
#include "helper.h"

/////////////////////////////////////////////////////////////////////////////
// CFrameSet

static CFrame tframe;

CFrameSet::CFrameSet()
{
    m_max = GROWBY;
    m_arrFrames = new CFrame *[m_max];
    m_name = "";
    m_size = 0;
    assignNewUUID();
}

CFrameSet::CFrameSet(CFrameSet *s)
{
    m_max = GROWBY;
    m_arrFrames = new CFrame *[m_max];
    m_size = 0;

    for (int i = 0; i < s->getSize(); i++)
    {
        CFrame *frame = new CFrame((*s)[i]);
        add(frame);
    }

    m_name = s->getName();
    copyTags(*s);
    std::string uuid = m_tags["UUID"];
    if (uuid.empty())
    {
        assignNewUUID();
    }
}

void CFrameSet::assignNewUUID()
{
    m_tags["UUID"] = getUUID();
}

CFrameSet::~CFrameSet()
{
    forget();
    m_max = 0;
    delete[] m_arrFrames;
}

/////////////////////////////////////////////////////////////////////////////
// CFrameSet serialization

void CFrameSet::write0x501(IFile &file)
{
    long totalSize = 0;
    for (int n = 0; n < getSize(); ++n)
    {
        CFrame *frame = m_arrFrames[n];
        totalSize += 4 * frame->len() * frame->hei();
    }

    // OBL5 IMAGESET HEADER

    char *buffer = new char[totalSize];
    char *ptr = buffer;

    // prepare OBL5Data
    for (int n = 0; n < getSize(); ++n)
    {
        CFrame *frame = m_arrFrames[n];
        memcpy(ptr, frame->getRGB(), 4 * frame->len() * frame->hei());
        ptr += 4 * frame->len() * frame->hei();
    }

    uLong destSize;
    uint8_t *dest;
    int err = compressData((uint8_t *)buffer, (uLong)totalSize, &dest, destSize);
    if (err != Z_OK)
    {
        // CLuaVM::debugv("CFrameSet::write0x501 error: %d", err);
        return;
    }

    // OBL5 IMAGESET HEADER
    file.write(&destSize, sizeof(uint32_t));

    // IMAGE HEADER [0..n]
    for (int n = 0; n < getSize(); ++n)
    {
        CFrame *frame = m_arrFrames[n];
        int len = frame->len();
        int hei = frame->hei();
        file.write(&len, sizeof(uint16_t));
        file.write(&hei, sizeof(uint16_t));
    }

    // OBL5 DATA
    file.write(dest, destSize);

    // TAG COUNT
    int count = 0;
    for (auto const & kv : m_tags)
    {
        const std::string val = kv.second;
        if (!val.empty())
        {
            ++count;
        }
    }

    file.write(&count, sizeof(uint32_t));
    for (auto const & kv : m_tags)
    {
        const std::string key = kv.first;
        const std::string val = kv.second;
        if (!val.empty())
        {
            file << key;
            file << val;
        }
    }

    delete[] buffer;
    delete[] dest;
}

bool CFrameSet::write(IFile &file)
{
    int version = OBL_VERSION;
    file.write("OBL5", 4);
    file.write(&m_size, 4);
    file.write(&version, 4);

    switch (version)
    {

    case 0x500:
        // original version
        for (int n = 0; n < getSize(); ++n)
        {
            m_arrFrames[n]->write(file);
        }
        break;

    case 0x501:
        // packed
        write0x501(file);
        break;

    default:
        char tmp[256];
        sprintf(tmp, "unknown OBL5 version: %x", version);
        m_lastError = tmp;
        return false;
    }

    return true;
}

bool CFrameSet::read0x501(IFile &file, int size)
{
    // OBL5 IMAGESET HEADER
    long srcSize = 0;
    file.read(&srcSize, sizeof(uint32_t));

    long totalSize = 0;
    int *len = new int[size];
    int *hei = new int[size];

    // IMAGE HEADER [0..n]
    // read image sizes
    for (int n = 0; n < size; ++n)
    {
        len[n] = 0;
        hei[n] = 0;
        file.read(&len[n], sizeof(uint16_t));
        file.read(&hei[n], sizeof(uint16_t));
        totalSize += 4 * len[n] * hei[n];
    }

    // printf("totalSize: %d\n", totalSize);

    char *buffer = new char[totalSize];
    char *ptr = buffer;

    // read OBL5Data (compressed)

    uint8_t *srcBuffer = new uint8_t[srcSize];
    file.read(srcBuffer, srcSize);

    int err = uncompress(
        (uint8_t *)buffer,
        (uLong *)&totalSize,
        (uint8_t *)srcBuffer,
        (uLong)srcSize);

    if (err)
    {
        printf("err: %d\n", err);
    }

    delete[] srcBuffer;

    // add frames to frameSet
    for (int n = 0; n < size; ++n)
    {
        CFrame *frame = new CFrame(len[n], hei[n]);
        memcpy(frame->getRGB(), ptr, 4 * frame->len() * frame->hei());
        frame->updateMap();
        add(frame);
        ptr += 4 * frame->len() * frame->hei();
    }

    // TAG COUNT
    int count = 0;
    file.read(&count, sizeof(uint32_t));
    m_tags.clear();
    for (int i = 0; i < count; ++i)
    {
        std::string key;
        std::string val;
        file >> key;
        file >> val;
        m_tags[key] = val;
    }

    delete[] buffer;
    delete[] len;
    delete[] hei;

    return !err;
}

bool CFrameSet::read(IFile &file)
{
    bool result;
    char signature[5];
    signature[4] = 0;
    file.read(signature, 4);
    if (memcmp(signature, "OBL5", 4) != 0)
    {
        char tmp[128];
        sprintf(tmp, "bad signature: %s", signature);
        m_lastError = tmp;
        return false;
    }

    uint32_t size;
    uint32_t version;
    file.read(&size, sizeof(size));
    file.read(&version, sizeof(version));

    forget();

    switch (version)
    {

    case 0x500:

        for (uint32_t n = 0; n < size; ++n)
        {
            CFrame *temp = new CFrame;
            if (!temp->read(file, version))
            {
                return false;
            }
            temp->updateMap();
            add(temp);
        }
        result = true;
        break;

    case 0x501:
        result = read0x501(file, size);
        break;

    default:
        char tmp[128];
        sprintf(tmp, "unknown OBL5 version: %x", version);
        m_lastError = tmp;
        result = false;
    }

    std::string &uuid = m_tags["UUID"];
    if (uuid.empty())
    {
        assignNewUUID();
    }
    return result;
}

CFrame *CFrameSet::operator[](int n) const
{
    if (!(n & 0x8000) && n < m_size && n >= 0)
    {
        return m_arrFrames[n];
    }
    else
    {
        return &tframe;
    }
}

void CFrameSet::copyTags(CFrameSet &src)
{
    m_tags.clear();
    for (auto & kv : src.m_tags)
    {
        m_tags[kv.first] = kv.second;
    }
}

CFrameSet &CFrameSet::operator=(CFrameSet &s)
{
    forget();
    for (int i = 0; i < s.getSize(); i++)
    {
        CFrame *frame = new CFrame(s[i]);
        add(frame);
    }
    copyTags(s);
    m_name = s.getName();

    return *this;
}

void CFrameSet::forget()
{
    for (int i = 0; i < m_size; ++i)
    {
        if (m_arrFrames[i])
        {
            delete m_arrFrames[i];
            m_arrFrames[i] = nullptr;
        }
    }
    m_size = 0;
    m_tags.clear();
}

int CFrameSet::getSize()
{
    return m_size;
}

int CFrameSet::operator++()
{
    this->add(new CFrame);
    return this->getSize() - 1;
}

int CFrameSet::operator--()
{
    if (this->getSize() == 0)
        return 0;
    delete m_arrFrames[this->getSize()];
    removeAt(this->getSize() - 1);
    return this->getSize() - 1;
}

int CFrameSet::add(CFrame *pFrame)
{
    if (m_size == m_max)
    {
        m_max += GROWBY;
        CFrame **t = new CFrame *[m_max];
        for (int i = 0; i < m_size; ++i)
        {
            t[i] = m_arrFrames[i];
        }
        delete[] m_arrFrames;
        m_arrFrames = t;
    }

    m_arrFrames[m_size] = pFrame;
    ++m_size;

    return true;
}

void CFrameSet::insertAt(int n, CFrame *pFrame)
{
    if (n == m_size)
    {
        add(pFrame);
    }
    else
    {
        if (m_size == m_max)
        {
            add(nullptr);
        }
        else
        {
            m_size++;
        }

        for (int i = m_size - 1; i > n; i--)
        {
            m_arrFrames[i] = m_arrFrames[i - 1];
        }

        m_arrFrames[n] = pFrame;
    }

    return;
}

CFrame *CFrameSet::removeAt(int n)
{
    CFrame *rm = m_arrFrames[n];
    if (n != m_size - 1)
    {
        for (int i = n; i < m_size - 1; ++i)
        {
            m_arrFrames[i] = m_arrFrames[i + 1];
        }
    }
    --m_size;
    m_arrFrames[m_size] = nullptr;
    return rm;
}

const char *CFrameSet::getName() const
{
    return m_name.c_str();
}

void CFrameSet::setName(const char *str)
{
    m_name = str;
}

void CFrameSet::removeAll()
{
    for (int n = 0; n < m_size; ++n)
    {
        m_arrFrames[n] = nullptr;
    }

    m_size = 0;
}

/////////////////////////////////////////////////////////////////////////////
// import filter

char *CFrameSet::ima2bitmap(char *ImaData, int len, int hei)
{
    int x;
    int y;
    int x2;
    int y2;

    if (ImaData == nullptr)
        return nullptr;

    char *dest = new char[len * hei * 64];
    if (dest == nullptr)
        return nullptr;

    for (y = 0; y < hei; y++)
    {
        for (x = 0; x < len; x++)
        {
            for (y2 = 0; y2 < 8; y2++)
            {
                for (x2 = 0; x2 < 8; x2++)
                {
                    *(dest + x * 8 + x2 + (y * 8 + y2) * len * 8) =
                        *(ImaData + (x + y * len) * 64 + x2 + y2 * 8);
                }
            }
        }
    }

    return dest;
}

void CFrameSet::bitmap2rgb(char *bitmap, uint32_t *rgb, int len, int hei, int err)
{
    // original color palette
    const uint32_t colors[] = {
        0xff000000, 0xffab0303, 0xff03ab03, 0xffabab03, 0xff0303ab, 0xffab03ab, 0xff0357ab, 0xffababab,
        0xff575757, 0xffff5757, 0xff57ff57, 0xffffff57, 0xff5757ff, 0xffff57ff, 0xff57ffff, 0xffffffff,
        0xff000000, 0xff171717, 0xff232323, 0xff2f2f2f, 0xff3b3b3b, 0xff474747, 0xff535353, 0xff636363,
        0xff737373, 0xff838383, 0xff939393, 0xffa3a3a3, 0xffb7b7b7, 0xffcbcbcb, 0xffe3e3e3, 0xffffffff,
        0xffff0303, 0xffff0343, 0xffff037f, 0xffff03bf, 0xffff03ff, 0xffbf03ff, 0xff7f03ff, 0xff4303ff,
        0xff0303ff, 0xff0343ff, 0xff037fff, 0xff03bfff, 0xff03ffff, 0xff03ffbf, 0xff03ff7f, 0xff03ff43,
        0xff03ff03, 0xff43ff03, 0xff7fff03, 0xffbfff03, 0xffffff03, 0xffffbf03, 0xffff7f03, 0xffff4303,
        0xffff7f7f, 0xffff7f9f, 0xffff7fbf, 0xffff7fdf, 0xffff7fff, 0xffdf7fff, 0xffbf7fff, 0xff9f7fff,
        0xff7f7fff, 0xff7f9fff, 0xff7fbfff, 0xff7fdfff, 0xff7fffff, 0xff7fffdf, 0xff7fffbf, 0xff7fff9f,
        0xff7fff7f, 0xff9fff7f, 0xffbfff7f, 0xffdfff7f, 0xffffff7f, 0xffffdf7f, 0xffffbf7f, 0xffff9f7f,
        0xffffb7b7, 0xffffb7c7, 0xffffb7db, 0xffffb7eb, 0xffffb7ff, 0xffebb7ff, 0xffdbb7ff, 0xffc7b7ff,
        0xffb7b7ff, 0xffb7c7ff, 0xffb7dbff, 0xffb7ebff, 0xffb7ffff, 0xffb7ffeb, 0xffb7ffdb, 0xffb7ffc7,
        0xffb7ffb7, 0xffc7ffb7, 0xffdbffb7, 0xffebffb7, 0xffffffb7, 0xffffebb7, 0xffffdbb7, 0xffffc7b7,
        0xff730303, 0xff73031f, 0xff73033b, 0xff730357, 0xff730373, 0xff570373, 0xff3b0373, 0xff1f0373,
        0xff030373, 0xff031f73, 0xff033b73, 0xff035773, 0xff037373, 0xff037357, 0xff03733b, 0xff03731f,
        0xff037303, 0xff1f7303, 0xff3b7303, 0xff577303, 0xff737303, 0xff735703, 0xff733b03, 0xff731f03,
        0xff733b3b, 0xff733b47, 0xff733b57, 0xff733b63, 0xff733b73, 0xff633b73, 0xff573b73, 0xff473b73,
        0xff3b3b73, 0xff3b4773, 0xff3b5773, 0xff3b6373, 0xff3b7373, 0xff3b7363, 0xff3b7357, 0xff3b7347,
        0xff3b733b, 0xff47733b, 0xff57733b, 0xff63733b, 0xff73733b, 0xff73633b, 0xff73573b, 0xff73473b,
        0xff735353, 0xff73535b, 0xff735363, 0xff73536b, 0xff735373, 0xff6b5373, 0xff635373, 0xff5b5373,
        0xff535373, 0xff535b73, 0xff536373, 0xff536b73, 0xff537373, 0xff53736b, 0xff537363, 0xff53735b,
        0xff537353, 0xff5b7353, 0xff637353, 0xff6b7353, 0xff737353, 0xff736b53, 0xff736353, 0xff735b53,
        0xff430303, 0xff430313, 0xff430323, 0xff430333, 0xff430343, 0xff330343, 0xff230343, 0xff130343,
        0xff030343, 0xff031343, 0xff032343, 0xff033343, 0xff034343, 0xff034333, 0xff034323, 0xff034313,
        0xff034303, 0xff134303, 0xff234303, 0xff334303, 0xff434303, 0xff433303, 0xff432303, 0xff431303,
        0xff432323, 0xff43232b, 0xff432333, 0xff43233b, 0xff432343, 0xff3b2343, 0xff332343, 0xff2b2343,
        0xff232343, 0xff232b43, 0xff233343, 0xff233b43, 0xff234343, 0xff23433b, 0xff234333, 0xff23432b,
        0xff234323, 0xff2b4323, 0xff334323, 0xff3b4323, 0xff434323, 0xff433b23, 0xff433323, 0xff432b23,
        0xff432f2f, 0xff432f33, 0xff432f37, 0xff432f3f, 0xff432f43, 0xff3f2f43, 0xff372f43, 0xff332f43,
        0xff2f2f43, 0xff2f3343, 0xff2f3743, 0xff2f3f43, 0xff2f4343, 0xff2f433f, 0xff2f4337, 0xff2f4333,
        0xff2f432f, 0xff33432f, 0xff37432f, 0xff3f432f, 0xff43432f, 0xff433f2f, 0xff43372f, 0xff43332f,
        0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000};

    for (int i = 0; i < len * hei; i++)
    {
        if (bitmap[i])
        {
            rgb[i] = colors[(bitmap[i] + err) & 255];
        }
        else
        {
            rgb[i] = 0;
        }
    }
}

bool CFrameSet::extract(IFile &file, char *out_format)
{
    std::string format = "NaN";
    bool isUnknown = true;

    m_lastError = "";

    // IMA_FORMAT
    typedef struct
    {
        char len;
        char hei;
    } USER_IMAHEADER;

    typedef struct
    {
        char Id[4]; // IMC1
        char len;
        char hei;
        int SizeData;
    } USER_IMC1HEADER;

    // MCX_FORMAT
    typedef struct
    {
        uint32_t PtrPrev;
        uint32_t PtrNext;
        char Name[30];
        short Class;
        char ImageData[32][32];
    } USER_MCX;

    typedef struct
    {
        char Id[4]; // "GE96"
        short Class;
        char Name[256];
        int NbrImages;
        int LastViewed;
        char Palette[256][3];
        uint32_t PtrFirst;
    } USER_MCXHEADER;

    // USER_OBL3...............................................
    typedef struct
    {
        uint32_t PtrPrev;
        uint32_t PtrNext;
        uint32_t PtrBits;
        uint32_t PtrMap;
        uint32_t filler;
        char ExtraInfo[4];
    } USER_OBL3;

    // USER_OBL3HEADER..........................................
    typedef struct
    {
        char Id[4]; // "OBL3"
        uint32_t LastViewed;

        uint32_t iNbrImages;
        uint32_t iDefaultImage;

        uint8_t bClassInfo;
        uint8_t bDisplayInfo;
        uint8_t bActAsInfo;
        uint8_t bItemProps;
        uint16_t wU1;
        uint16_t wU2;
        uint16_t wRebirthTime;
        uint16_t wMaxJump;

        uint16_t wFireRate;
        uint16_t wLifeForce;
        uint16_t wLives;
        uint16_t wOxygen;

        uint16_t wSpeed;
        uint16_t wFallSpeed;
        uint16_t wAniSpeed;
        uint16_t wTimeOut;

        uint16_t wDomages;
        uint16_t wFiller;
        uint32_t iLen;
        uint32_t iHei;

        uint8_t bU1;
        uint8_t bU2;
        uint8_t bU3;
        uint8_t bCompilerOptions;

        uint32_t filler;
        char szFilename[256];
        char szName[256];
        char szCopyrights[1024];
    } USER_OBL3HEADER;

    char id[8];
    file.read(id, 8);
    int size = 0;
    if (memcmp(id, "OBL3", 4) == 0)
    {
        isUnknown = false;
        format = "OBL3";
        file.seek(0);
        USER_OBL3HEADER oblHead;
        file.read(&oblHead, sizeof(USER_OBL3HEADER));
        size = oblHead.iNbrImages;
        for (int i = 0; i < (int)oblHead.iNbrImages; ++i)
        {
            CFrame *frame = new CFrame(oblHead.iLen * 16, oblHead.iHei * 16);
            char *bitmap = new char[oblHead.iLen * 16 * oblHead.iHei * 16];
            USER_OBL3 obl;
            file.read(&obl, sizeof(USER_OBL3));
            file.read(bitmap, oblHead.iLen * 16 * oblHead.iHei * 16);
            bitmap2rgb(bitmap, frame->getRGB(), frame->len(), frame->hei(), -16);
            frame->updateMap();
            add(frame);
            delete[] bitmap;
        }
    }

    if (memcmp(id, "OBL4", 4) == 0)
    {
        isUnknown = false;
        format = "OBL4";
        file.seek(4);
        int mode;
        file >> size;
        file >> mode;
        for (int i = 0; i < size; ++i)
        {
            int len;
            int hei;
            file >> len;
            file >> hei;
            CFrame *frame = new CFrame(len, hei);
            int mapped;
            file >> mapped;
            char *bitmap = new char[len * hei];
            if (mode != CFrame::MODE_ZLIB_ALPHA)
            {
                file.read(bitmap, frame->len() * frame->hei());
            }
            else
            {
                uLong nSrcLen = 0;
                file.read(&nSrcLen, 4);
                uint8_t *pSrc = new uint8_t[nSrcLen];
                file.read(pSrc, nSrcLen);
                uLong nDestLen = frame->len() * frame->hei();
                int err = uncompress(
                    (uint8_t *)bitmap,
                    (uLong *)&nDestLen,
                    (uint8_t *)pSrc,
                    (uLong)nSrcLen);
                if (err)
                {
                    m_lastError = "CFrameSet::extract zlib error";
                }
                delete[] pSrc;
            }
            frame->setRGB(new uint32_t[frame->len() * frame->hei()]);
            bitmap2rgb(bitmap, frame->getRGB(), frame->len(), frame->hei(), -16);
            frame->updateMap();
            add(frame);
            delete[] bitmap;
        }
    }

    if (memcmp(id, "OBL5", 4) == 0)
    {
        isUnknown = false;
        CFrameSet frameSet;
        file.seek(0);
        if (frameSet.read(file))
        {
            size = frameSet.getSize();
            for (int i = 0; i < size; ++i)
            {
                frameSet[i]->updateMap();
                add(frameSet[i]);
                // TODO: add map if this is ever implemented
            }
            frameSet.removeAll();
            format = "OBL5";
        }
        else
        {
            m_lastError = "unsupported OBL5 version";
            return false;
        }
    }

    if (memcmp(id, "GE96", 4) == 0)
    {
        isUnknown = false;
        format = "GE96";
        file.seek(0);
        USER_MCXHEADER mcxHead;
        file.read(&mcxHead, sizeof(USER_MCXHEADER));
        size = mcxHead.NbrImages;
        for (int i = 0; i < mcxHead.NbrImages; ++i)
        {
            CFrame *frame = new CFrame(32,32);
            char *bitmap = new char[32 * 32];
            USER_MCX mcx;
            file.read(&mcx, sizeof(USER_MCX));
            memcpy(bitmap, &mcx.ImageData[0][0], 32 * 32);
            bitmap2rgb(bitmap, frame->getRGB(), frame->len(), frame->hei(), -16);
            frame->updateMap();
            add(frame);
            delete[] bitmap;
        }
    }

    if (memcmp(id, "IMC1", 4) == 0)
    {
        isUnknown = false;
        format = "IMC1";
        size = 1;
        USER_IMC1HEADER imc1Head;
        file.seek(0);
        // this is done in two steps because the
        // IMC1 structure doesn't align properly in 32bits
        file.read(&imc1Head, 6);
        file.read(&imc1Head.SizeData, 4);
        uint8_t *ptrIMC1 = new uint8_t[imc1Head.SizeData];
        file.read(ptrIMC1, imc1Head.SizeData);
        uint8_t *ptr = new uint8_t[imc1Head.len * imc1Head.hei * 64];
        memset(ptr, 0, imc1Head.len * imc1Head.hei * 64);
        uint8_t *xptr = ptr;
        uint8_t *xptrIMC1 = ptrIMC1;
        for (int cpt = 0; cpt < imc1Head.SizeData - 1;)
        {
            if (*ptrIMC1 == 0xff)
            {
                for (int loop = ptrIMC1[2] + ptrIMC1[3] * 256; loop; loop--, ptr++)
                {
                    *ptr = ptrIMC1[1];
                }
                ptrIMC1 += 4;
                cpt += 4;
            }
            else
            {
                *ptr = *ptrIMC1;
                ptr++;
                ptrIMC1++;
                cpt++;
            }
        }
        CFrame *frame = new CFrame(imc1Head.len * 8, imc1Head.hei * 8);
        char *bitmap = ima2bitmap((char *)xptr, imc1Head.len, imc1Head.hei);
        bitmap2rgb(bitmap, frame->getRGB(), frame->len(), frame->hei(), 0);
        frame->updateMap();
        add(frame);
        delete[] xptrIMC1;
        delete[] xptr;
        delete[] bitmap;
    }

    uint8_t pngSig[] = {137, 80, 78, 71, 13, 10, 26, 10};
    if (memcmp(id, pngSig, 8) == 0)
    {
        format = "PNG";
        CPngMagic png;
        return png.parsePNG(*this, file);
    }

    if (!size && isUnknown)
    {
        format = "IMA";
        int fileSize = file.getSize();
        USER_IMAHEADER imaHead;
        file.seek(0);
        file.read(&imaHead, 2);
        int dataSize = imaHead.len * imaHead.hei * 64;
        if ((fileSize - 2) != dataSize)
        {
            m_lastError = "this is not a valid .ima file";
            return false;
        }
        size = 1;
        CFrame *frame = new CFrame(imaHead.len * 8, imaHead.hei * 8);
        char *pIMA = new char[dataSize];
        file.read(pIMA, dataSize);
        char *bitmap = ima2bitmap(pIMA, imaHead.len, imaHead.hei);
        bitmap2rgb(bitmap, frame->getRGB(), frame->len(), frame->hei(), 0);
        frame->updateMap();
        add(frame);
        delete[] pIMA;
        delete[] bitmap;
    }

    if (out_format)
    {
        strncpy(out_format, format.c_str(), 4);
    }

    return size != 0;
}

const char *CFrameSet::getLastError() const
{
    return m_lastError.c_str();
}

bool CFrameSet::isFriendFormat(const char *format)
{
    const char *friends[] = {
        "IMC1", "IMA", "GE96",
        "OBL3", "OBL4", "OBL5"};

    for (unsigned int i = 0; i < sizeof(friends) / sizeof(char *); ++i)
    {
        if (strcmp(friends[i], format) == 0)
        {
            return true;
        }
    }

    return false;
}

void CFrameSet::move(int s, int t)
{
    CFrame *f = removeAt(s);
    insertAt(t, f);
}

void CFrameSet::toPng(unsigned char *&data, int &size)
{
    if (m_size > 1)
    {
        short *xx = new short[m_size];
        short *yy = new short[m_size];
        int width = 0;
        int height = 0;
        for (int i = 0; i < m_size; ++i)
        {
            width += m_arrFrames[i]->len();
            height = std::max(height, m_arrFrames[i]->hei());
            xx[i] = m_arrFrames[i]->len();
            yy[i] = m_arrFrames[i]->hei();
        }

        CFrame *frame = new CFrame(width, height);
        CFrame &t = *frame;
        int mx = 0;
        for (int i = 0; i < m_size; ++i)
        {
            CFrame &s = *(m_arrFrames[i]);
            for (int y = 0; y < s.hei(); ++y)
            {
                for (int x = 0; x < s.len(); ++x)
                {
                    t.at(mx + x, y) = s.at(x, y);
                }
            }
            mx += s.len();
        }

        // prepare custom data to be injected
        int t_size = sizeof(CFrame::png_OBL5) + m_size * 2 * sizeof(short) + sizeof(int);
        uint8_t *buf = new uint8_t[t_size];
        memset(buf, 0, t_size);
        CFrame::png_OBL5 *obl5data = (CFrame::png_OBL5 *)buf;
        obl5data->Length = CFrame::toNet(t_size - 12);
        // memcpy(obl5data->ChunkType, "OBL5", 4);
        memcpy(obl5data->ChunkType, CFrame::getChunkType(), 4);
        obl5data->Version = 0;
        obl5data->Count = m_size;
        memcpy(buf + sizeof(CFrame::png_OBL5),
               xx, m_size * sizeof(short));
        memcpy(buf + sizeof(CFrame::png_OBL5) + m_size * sizeof(short),
               yy, m_size * sizeof(short));

        // TODO: inject obldata into png
        frame->toPng(data, size, buf, t_size);
        delete frame;
        delete[] buf;
        delete []xx;
        delete []yy;
    }
    else
    {
        if (m_size)
        {
            m_arrFrames[0]->toPng(data, size);
        }
        else
        {
            data = nullptr;
            size = 0;
        }
    }
}

void CFrameSet::setLastError(const char *error)
{
    m_lastError = error;
}

std::string &CFrameSet::tag(const char *tag)
{
    return m_tags[tag];
}

void CFrameSet::setTag(const char *tag, const char *v)
{
    m_tags[tag] = v;
}

void CFrameSet::toSubset(CFrameSet &dest, int start, int end)
{
    int last = end == -1 ? getSize() - 1 : end;
    for (int i = start; i <= last; ++i)
    {
        CFrame *p = new CFrame(m_arrFrames[i]);
        dest.add(p);
    }
}
