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

#include <cstring>
#include <cstdio>
#include <cstdint>
#include <set>
#include <memory>
#include "FrameSet.h"
#include "Frame.h"
#include <zlib.h>
#include "IFile.h"
#include "PngMagic.h"
#include "helper.h"
#include "logger.h"

typedef uint32_t PIXEL;

static CFrame tframe;

// original color palette
static const PIXEL g_original_palette[] = {
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

constexpr const char FORMAT_OBL3[] = "OBL3";
constexpr const char FORMAT_OBL4[] = "OBL4";
constexpr const char FORMAT_OBL5[] = "OBL5";
constexpr const char FORMAT_GE96[] = "GE96";
constexpr const char FORMAT_IMC1[] = "IMC1";

CFrameSet::CFrameSet()
{
    m_name = "";
    assignNewUUID();
}

CFrameSet::CFrameSet(CFrameSet *s)
{
    m_arrFrames.reserve(s->getSize());
    for (size_t i = 0; i < s->getSize(); i++)
    {
        CFrame *frame = new CFrame;
        frame->copy((*s)[i]);
        add(frame);
        // add(new CFrame((*s)[i]));
    }

    m_name = s->getName();
    copyTags(*s);
    if (m_tags["UUID"].empty())
        assignNewUUID();
}

void CFrameSet::assignNewUUID()
{
    m_tags["UUID"] = getUUID();
}

CFrameSet::~CFrameSet()
{
    clear();
}

bool CFrameSet::writeSolid(IFile &file)
{

    // Validate frame set
    const size_t size = getSize();
    if (size == 0 || size > MAX_IMAGES)
    {
        m_lastError = "Invalid frame count: " + std::to_string(size);
        return false;
    }

    int64_t totalSize = 0;
    for (size_t i = 0; i < getSize(); ++i)
    {
        CFrame *frame = m_arrFrames[i];
        if (!frame || !frame->getRGB().data())
        {
            m_lastError = "Null frame or RGB data at index " + std::to_string(i);
            return false;
        }
        int len = frame->width();
        int hei = frame->height();
        if (len <= 0 || hei <= 0 || len > 4096 || hei > 4096)
        {
            m_lastError = "Invalid frame dimensions at index " + std::to_string(i);
            return false;
        }
        int64_t frameSize = static_cast<int64_t>(len) * hei * sizeof(PIXEL);
        if (totalSize > INT_MAX - frameSize)
        {
            m_lastError = "Total pixel data size overflow";
            return false;
        }
        totalSize += frameSize; // sizeof(PIXEL) * frame->len() * frame->hei();
    }

    // Pack pixel data
    std::vector<uint8_t> buffer(totalSize);
    uint8_t *ptr = buffer.data();
    for (size_t i = 0; i < getSize(); ++i)
    {
        CFrame *frame = m_arrFrames[i];
        int pixelBytes = sizeof(PIXEL) * frame->width() * frame->height();
        memcpy(ptr, frame->getRGB().data(), pixelBytes);
        ptr += pixelBytes;
    }

    // Compress data
    std::vector<uint8_t> dest;
    const int err = compressData(buffer, dest);
    if (err != Z_OK)
    {
        char tmp[128];
        snprintf(tmp, sizeof(tmp), "Zlib compression error %d: %s", err, zError(err));
        m_lastError = tmp;
        return false;
    }
    if (dest.size() > INT_MAX)
    {
        m_lastError = "Compressed data size exceeds maximum";
        return false;
    }

    // Write OBL5 IMAGESET HEADER
    const uLong destSize = dest.size();
    if (file.write(&destSize, sizeof(uint32_t)) != IFILE_OK)
    {
        m_lastError = "Failed to write compressed size";
        return false;
    }

    // Write IMAGE HEADER [0..n]
    for (size_t i = 0; i < getSize(); ++i)
    {
        CFrame *frame = m_arrFrames[i];
        // do not modify the write sizes
        int len = frame->width();
        int hei = frame->height();
        if (file.write(&len, sizeof(uint16_t)) != IFILE_OK ||
            file.write(&hei, sizeof(uint16_t)) != IFILE_OK)
        {
            m_lastError = "Fail to write dimension at index " + std::to_string(i);
            return false;
        }
    }

    // Write OBL5 DATA
    if (file.write(dest.data(), destSize) != IFILE_OK)
    {
        m_lastError = "fail to write compressed data";
        return false;
    }

    // TAG COUNT
    int tagCount = 0;
    for (auto const &[k, v] : m_tags)
    {
        if (!v.empty() && k.size() <= TAG_KEY_MAX && v.size() <= TAG_VAL_MAX)
            ++tagCount;
    }

    if (file.write(&tagCount, sizeof(uint32_t)) != IFILE_OK)
    {
        m_lastError = "fail to write tagCount";
        return false;
    }
    for (auto const &[k, v] : m_tags)
    {
        if (!v.empty() && k.size() <= TAG_KEY_MAX && v.size() <= TAG_VAL_MAX)
        {
            file << k;
            file << v;
        }
        if (k.size() > TAG_KEY_MAX || v.size() > TAG_VAL_MAX)
        {
            LOGW("tag metadata size out of bound: [%lu,%lu]; max: [%lu, %lu]",
                 k.size(), v.size(), TAG_KEY_MAX, TAG_VAL_MAX);
        }
    }

    return true;
}

bool CFrameSet::write(IFile &file)
{
    return write(file, DEFAULT_OBL5_FORMAT);
}

bool CFrameSet::write(IFile &file, const Format format)
{
    const size_t size = m_arrFrames.size();
    if (file.write(FORMAT_OBL5, ID_SIG_LEN) != IFILE_OK)
    {
        LOGE("failed to write OBL5 id to file");
        return false;
    }
    if (file.write(&size, sizeof(uint32_t)) != IFILE_OK)
    {
        LOGE("failed to write OBL5 size to file");
        return false;
    }
    if (file.write(&format, sizeof(uint32_t)) != IFILE_OK)
    {
        LOGE("failed to write OBL5 format to file");
        return false;
    }

    switch (format)
    {

    case OBL5_UNPACKED:
        // original version
        for (size_t i = 0; i < getSize(); ++i)
        {
            if (!m_arrFrames[i]->write(file))
            {
                LOGE("failed to write OBL5 frame %d to file", i);
                return false;
            }
        }
        break;

    case OBL5_SOLID:
        // packed (solid compression)
        return writeSolid(file);

    default:
        char tmp[64];
        snprintf(tmp, sizeof(tmp), "unknown OBL5 format: %x", format);
        m_lastError = tmp;
        return false;
    }

    return true;
}

bool CFrameSet::readSolid(IFile &file, int size)
{
    // Validate size
    if (size <= 0 || size > MAX_IMAGES)
    { // Prevent DoS
        m_lastError = "Invalid frame count: " + std::to_string(size);
        return false;
    }

    // OBL5 IMAGESET HEADER
    // Read compressed size
    long srcSize = 0;
    //    file.read(&srcSize, sizeof(uint32_t));
    if (file.read(&srcSize, sizeof(uint32_t)) != IFILE_OK || srcSize <= 0)
    {
        m_lastError = "Failed to read or invalid compressed size";
        return false;
    }

    // Validate file size
    long fileSize = file.getSize();
    if (file.tell() + srcSize + size * 2 * (long)sizeof(uint16_t) > fileSize)
    {
        char tmp[128];
        snprintf(tmp, sizeof(tmp), "File too small for OBL5_SOLID data; data size=%ld; file size=%ld)", srcSize, fileSize);
        m_lastError = tmp;
        return false;
    }

    int64_t totalSize = 0;
    std::vector<int> lengths(size);
    std::vector<int> heights(size);

    // IMAGE HEADER [0..n]
    // Read frame dimensions
    for (int n = 0; n < size; ++n)
    {
        uint16_t len, hei;
        if (file.read(&len, sizeof(len)) != IFILE_OK || file.read(&hei, sizeof(hei)) != IFILE_OK)
        {
            m_lastError = "Failed to read frame dimensions";
            return false;
        }
        if (len == 0 || hei == 0 || len > MAX_IMAGE_SIZE || hei > MAX_IMAGE_SIZE)
        {
            char tmp[128];
            snprintf(tmp, sizeof(tmp), "Invalid frame dimensions [%d,%d] at index %d", len, hei, n);
            m_lastError = tmp;
            return false;
        }
        lengths[n] = len;
        heights[n] = hei;
        int64_t frameSize = static_cast<int64_t>(len) * hei * sizeof(PIXEL);
        if (totalSize > INT_MAX - frameSize)
        {
            m_lastError = "Total pixel data size overflow";
            return false;
        }
        totalSize += frameSize;
    }

    std::vector<uint8_t> buffer(totalSize);
    uint8_t *ptr = buffer.data();

    // read OBL5Data (compressed)
    std::vector<uint8_t> srcBuffer(srcSize);
    file.read(srcBuffer.data(), srcSize);
    uLong destLen = totalSize;

    const int err = uncompress(
        buffer.data(),
        &destLen,
        srcBuffer.data(),
        srcSize);
    if (err != Z_OK)
    {
        char tmp[128];
        snprintf(tmp, sizeof(tmp), "Zlib error %d or size mismatch (%lu != %ld)", err, destLen, totalSize);
        m_lastError = tmp;
        LOGE("err: %d\n", err);
        return false;
    }

    // Process frames
    m_arrFrames.reserve(size);
    for (int n = 0; n < size; ++n)
    {
        const int len = lengths[n];
        const int hei = heights[n];
        const int64_t dataSize = static_cast<int64_t>(len) * hei * sizeof(PIXEL);
        if (static_cast<size_t>(ptr - buffer.data()) + dataSize > static_cast<size_t>(totalSize))
        {
            m_lastError = "Decompressed data truncated at frame " + std::to_string(n);
            return false;
        }

        // CFrame *frame = new CFrame(lengths[n], heights[n]);
        auto frame = std::make_unique<CFrame>(len, hei);
        // const size_t dataSize = sizeof(PIXEL) * frame->len() * frame->hei();
        memcpy(frame->getRGB().data(), ptr, dataSize);
        frame->updateMap();
        m_arrFrames.push_back(frame.release());
        ptr += dataSize;
    }

    // Read tags
    uint32_t tagCount;
    if (file.read(&tagCount, sizeof(tagCount)) != IFILE_OK || tagCount > 100)
    {
        m_lastError = "Failed to read or invalid tag count: " + std::to_string(tagCount);
        return false;
    }

    m_tags.clear();
    for (size_t i = 0; i < tagCount; ++i)
    {
        std::string key;
        std::string val;
        file >> key;
        file >> val;
        if (key.length() > TAG_KEY_MAX || val.size() > TAG_VAL_MAX)
        {
            m_lastError = "Failed to read or invalid tag at index " + std::to_string(i);
            return false;
        }
        m_tags[key] = val;
    }

    return true;
}

bool CFrameSet::read(IFile &file)
{

    long org = file.tell();
    if (org < 0)
    {
        m_lastError = "Failed to get file position";
        return false;
    }

    // Read signature
    char signature[ID_SIG_LEN + 1];
    signature[ID_SIG_LEN] = '\0';
    if (file.read(signature, ID_SIG_LEN) != IFILE_OK)
    {
        m_lastError = "Failed to read file signature";
        return false;
    }
    if (memcmp(signature, FORMAT_OBL5, ID_SIG_LEN) != 0)
    {
        char tmp[128];
        snprintf(tmp, sizeof(tmp), "bad signature: %s", signature);
        m_lastError = tmp;
        return false;
    }
    // Get file size for validation
    long fileSize = file.getSize();
    if (fileSize < ID_SIG_LEN)
    {
        m_lastError = "Failed to get file size or file too small";
        return false;
    }
    uint32_t size;
    uint32_t version;
    if (file.read(&size, sizeof(size)) != IFILE_OK ||
        file.read(&version, sizeof(version)) != IFILE_OK)
    {
        m_lastError = "Failed to read OBL5 header";
        return false;
    }

    // Validate size
    if (size == 0 || size > MAX_IMAGES)
    { // Prevent DoS
        char tmp[128];
        snprintf(tmp, sizeof(tmp), "Invalid frame count: %u", size);
        m_lastError = tmp;
        return false;
    }
    // Clear existing state
    clear();
    m_name.clear();
    m_tags.clear();
    m_nCurrFrame = 0;

    // Dispatch based on version
    bool result = false;
    switch (version)
    {

    case OBL5_UNPACKED:
        for (uint32_t n = 0; n < size; ++n)
        {
            CFrame *frame = new CFrame;
            if (!frame->read(file))
            {
                m_lastError = frame->getLastError();
                return false;
            }
            add(frame);
        }
        result = true;
        break;

    case OBL5_SOLID:
        result = readSolid(file, size);
        break;

    default:
        char tmp[128];
        snprintf(tmp, sizeof(tmp), "unknown OBL5 version: %x", version);
        m_lastError = tmp;
        result = false;
    }

    // Warn if extra data remains
    if (file.tell() < fileSize)
    {
        LOGW("Extra data after OBL5 frame set; possible format mismatch");
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
    const size_t size = m_arrFrames.size();
    if (!(n & 0x8000) && n < (int)size && n >= 0)
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
    for (auto &kv : src.m_tags)
    {
        m_tags[kv.first] = kv.second;
    }
}

CFrameSet &CFrameSet::operator=(CFrameSet &s)
{
    clear();
    m_arrFrames.reserve(s.getSize());
    for (size_t i = 0; i < s.getSize(); i++)
    {
        CFrame *frame = new CFrame;
        frame->copy(s[i]);
        // CFrame *frame = new CFrame(s[i]);
        m_arrFrames.emplace_back(frame);
    }
    copyTags(s);
    m_name = s.getName();
    return *this;
}

void CFrameSet::clear()
{
    const size_t size = m_arrFrames.size();
    for (size_t i = 0; i < size; ++i)
        delete m_arrFrames[i];
    m_arrFrames.clear();
    m_tags.clear();
}

size_t CFrameSet::getSize()
{
    return m_arrFrames.size();
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
    m_arrFrames.push_back(pFrame);
    return m_arrFrames.size() - 1;
}

void CFrameSet::insertAt(int i, CFrame *pFrame)
{
    m_arrFrames.insert(m_arrFrames.begin() + i, pFrame);
}

CFrame *CFrameSet::removeAt(int i)
{
    CFrame *frame = m_arrFrames[i];
    m_arrFrames.erase(m_arrFrames.begin() + i);
    return frame;
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
    m_arrFrames.clear();
}

std::unique_ptr<char[]> CFrameSet::ima2bitmap(char *ImaData, int len, int hei)
{
    int x;
    int y;
    int x2;
    int y2;

    if (ImaData == nullptr)
    {
        LOGE("ImaData == nullptr");
        return nullptr;
    }

    std::unique_ptr<char[]> dest(new char[len * hei * FNT_SIZE * FNT_SIZE]);
    if (dest == nullptr)
    {
        LOGE("dest == nullptr");
        return nullptr;
    }

    for (y = 0; y < hei; y++)
    {
        for (x = 0; x < len; x++)
        {
            for (y2 = 0; y2 < FNT_SIZE; y2++)
            {
                for (x2 = 0; x2 < FNT_SIZE; x2++)
                {
                    dest[x * FNT_SIZE + x2 + (y * FNT_SIZE + y2) * len * FNT_SIZE] =
                        *(ImaData + (x + y * len) * FNT_SIZE * FNT_SIZE + x2 + y2 * FNT_SIZE);
                }
            }
        }
    }

    return dest;
}

void CFrameSet::bitmap2rgb(char *bitmap, uint32_t *rgb, int len, int hei, int err)
{
    for (int i = 0; i < len * hei; i++)
    {
        if (bitmap[i])
        {
            rgb[i] = g_original_palette[(bitmap[i] + err) & 255];
        }
        else
        {
            rgb[i] = 0;
        }
    }
}

bool CFrameSet::extract(IFile &file)
{
    const auto org = file.tell(); // save stream origin
    const uint8_t pngSig[] = {137, 80, 78, 71, 13, 10, 26, 10};
    char id[sizeof(pngSig)];
    if (file.read(id, sizeof(id)) != IFILE_OK)
    {
        m_lastError = "failed to read file header";
        return false;
    }
    m_lastError = "";

    if (memcmp(id, FORMAT_OBL3, ID_SIG_LEN) == 0)
    {
        return importOBL3(file, org);
    }
    else if (memcmp(id, FORMAT_OBL4, ID_SIG_LEN) == 0)
    {
        return importOBL4(file, org);
    }
    else if (memcmp(id, FORMAT_OBL5, ID_SIG_LEN) == 0)
    {
        return importOBL5(file, org);
    }
    else if (memcmp(id, FORMAT_GE96, ID_SIG_LEN) == 0)
    {
        return importGE96(file, org);
    }
    else if (memcmp(id, FORMAT_IMC1, ID_SIG_LEN) == 0)
    {
        return importIMC1(file, org);
    }
    else if (memcmp(id, pngSig, sizeof(pngSig)) == 0)
    {
        return parsePNG(*this, file, org);
    }
    else
    {
        return importIMA(file, org);
    }
}

bool CFrameSet::importIMA(IFile &file, const long org)
{
    // IMA_FORMAT
    struct USER_IMAHEADER
    {
        uint8_t len;
        uint8_t hei;
    };
    int fileSize = file.getSize();
    USER_IMAHEADER imaHead;
    int hdrSize = static_cast<int>(sizeof(USER_IMAHEADER));
    file.seek(org);
    if (file.read(&imaHead, hdrSize) != IFILE_OK)
    {
        m_lastError = "failed to read hdr";
        return false;
    }
    int dataSize = imaHead.len * imaHead.hei * FNT_SIZE * FNT_SIZE;
    if (dataSize == 0)
    {
        m_lastError = "datasize cannot be zero";
        return false;
    }
    if ((fileSize - hdrSize) != dataSize)
    {
        m_lastError = "this is not a valid .ima file";
        return false;
    }
    std::vector<char> pIMA(dataSize);
    if (file.read(pIMA.data(), dataSize) != IFILE_OK)
        return false;
    std::unique_ptr<char[]> bitmap = ima2bitmap(pIMA.data(), imaHead.len, imaHead.hei);
    if (bitmap == nullptr)
        return false;
    // CFrame *frame = new CFrame(imaHead.len * FNT_SIZE, imaHead.hei * FNT_SIZE);
    std::unique_ptr<CFrame> frame = std::make_unique<CFrame>(imaHead.len * FNT_SIZE, imaHead.hei * FNT_SIZE);
    if (frame == nullptr)
    {
        m_lastError = "memory allocation error";
        return false;
    }
    bitmap2rgb(bitmap.get(), frame->getRGB().data(), frame->width(), frame->height(), COLOR_INDEX_OFFSET_NONE);
    frame->updateMap();
    add(frame.release());
    LOGW("Extra data after %s frame; possible format mismatch", "IMA");
    return true;
}

bool CFrameSet::importIMC1(IFile &file, const long org)
{
    struct USER_IMC1HEADER
    {
        char Id[ID_SIG_LEN]; // IMC1
        uint8_t len;
        uint8_t hei;
        int32_t SizeData;
    };

    USER_IMC1HEADER imc1Head;
    auto fileSize = file.getSize();
    file.seek(org);
    // Reading header
    // this is done in two steps because the
    // IMC1 structure doesn't align properly in 32bits
    if (file.read(&imc1Head, 6) != IFILE_OK)
    {
        m_lastError = "failed to read header";
        return false;
    }
    if (memcmp(imc1Head.Id, FORMAT_IMC1, ID_SIG_LEN) != 0)
    {
        m_lastError = "IMC1 signature missing";
        return false;
    }
    int destSize = imc1Head.len * imc1Head.hei * FNT_SIZE * FNT_SIZE;
    if (destSize == 0)
    {
        m_lastError = "ima/fnt size cannot be 0";
        return false;
    }
    if (file.read(&imc1Head.SizeData, sizeof(imc1Head.SizeData)) != IFILE_OK)
    {
        m_lastError = "failed to read RLE-like encoded data";
        return false;
    }
    if (imc1Head.SizeData <= 0 || imc1Head.SizeData > 65536)
    {
        char tmp[64];
        snprintf(tmp, sizeof(tmp), "invalid sizeData. cannot be %d", imc1Head.SizeData);
        m_lastError = tmp;
        return false;
    }

    // reading compressed data
    // uint8_t *ptrIMC1 = new uint8_t[imc1Head.SizeData];
    std::vector<uint8_t> imc1(imc1Head.SizeData);
    auto ptrIMC1 = imc1.data();
    if (ptrIMC1 == nullptr)
    {
        m_lastError = "allocation error";
        return false;
    }

    if (file.read(ptrIMC1, imc1Head.SizeData) != IFILE_OK)
    {
        m_lastError = "failed to read RLE-like data";
        return false;
    }

    // allocating destination buffer
    // uint8_t *ptr = new uint8_t[imc1Head.len * imc1Head.hei * FNT_SIZE * FNT_SIZE];
    std::vector<uint8_t> dest(destSize);
    auto ptr = dest.data();
    if (ptr == nullptr)
    {
        m_lastError = "allocation error";
        return false;
    }
    memset(ptr, '\0', destSize);
    // uint8_t *xptr = ptr;
    // uint8_t *xptrIMC1 = ptrIMC1;

    // decoding RLE-like data
    int cpt = 0;
    while (cpt < imc1Head.SizeData - 1)
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
    if (cpt > imc1Head.SizeData)
    {
        char tmp[128];
        snprintf(tmp, sizeof(tmp), "IMC1 decoding went outside of range %d. reached: %d", imc1Head.SizeData, cpt);
        m_lastError = tmp;

        m_lastError = "IMC1 decoding went outside of range";
        return false;
    }

    // converting ima/FNT data to bitmap
    std::unique_ptr<char[]> bitmap = ima2bitmap((char *)dest.data(), imc1Head.len, imc1Head.hei);
    if (bitmap == nullptr)
    {
        m_lastError = "Memory allocation error";
        return false;
    }

    // allocating frame
    //    CFrame *frame = new CFrame(imc1Head.len * FNT_SIZE, imc1Head.hei * FNT_SIZE);
    std::unique_ptr<CFrame> frame = std::make_unique<CFrame>(imc1Head.len * FNT_SIZE, imc1Head.hei * FNT_SIZE);
    if (frame == nullptr)
    {
        m_lastError = "memory allocation error";
        return false;
    }
    // generating 32bits bitmap for frame
    bitmap2rgb(bitmap.get(), frame->getRGB().data(), frame->width(), frame->height(), COLOR_INDEX_OFFSET_NONE);
    frame->updateMap();
    add(frame.release());

    // Warn if extra data remains
    if (file.tell() < fileSize)
    {
        LOGW("Extra data after %s frame; possible format mismatch", FORMAT_IMC1);
    }
    // delete[] xptrIMC1;
    // delete[] xptr;
    return true;
}

bool CFrameSet::importGE96(IFile &file, const long org)
{
    struct USER_MCX
    {
        uint32_t PtrPrev;
        uint32_t PtrNext;
        char Name[30];
        uint16_t Class;
        char ImageData[GE96_TILE_SIZE][GE96_TILE_SIZE];
    };

    struct USER_MCXHEADER
    {
        char Id[ID_SIG_LEN]; // "GE96"
        uint16_t Class;
        char Name[256];
        int NbrImages;
        int LastViewed;
        char Palette[PALETTE_SIZE][RGB_BYTES];
        uint32_t PtrFirst;
    };

    file.seek(org);
    USER_MCXHEADER mcxHead;
    // read header
    if (file.read(&mcxHead, sizeof(USER_MCXHEADER)) != IFILE_OK)
    {
        m_lastError = "failed to read MCX header";
        return false;
    }
    if (memcmp(mcxHead.Id, FORMAT_GE96, ID_SIG_LEN) != 0)
    {
        m_lastError = "GE96 signature is missing";
        return false;
    }
    if (mcxHead.NbrImages == 0 || mcxHead.NbrImages > MAX_IMAGES)
    {
        char tmp[128];
        snprintf(tmp, sizeof(tmp), "image count %d is invalid", mcxHead.NbrImages);
        m_lastError = tmp;
        return false;
    }

    for (int i = 0; i < mcxHead.NbrImages; ++i)
    {
        // create new CFrame
        std::unique_ptr<CFrame> frame = std::make_unique<CFrame>(GE96_TILE_SIZE, GE96_TILE_SIZE);
        if (frame == nullptr)
        {
            m_lastError = "memory allocation error";
            return false;
        }
        // CFrame *frame = new CFrame(GE96_TILE_SIZE, GE96_TILE_SIZE);
        //  mcx pixel buffer
        const int byteSize = GE96_TILE_SIZE * GE96_TILE_SIZE;
        std::vector<char> bitmap(byteSize);
        if (bitmap.data() == nullptr)
        {
            return false;
        }
        // read frame data (32x32 pixels)
        USER_MCX mcx;
        if (file.read(&mcx, sizeof(USER_MCX)) != IFILE_OK)
        {
            return false;
        }
        memcpy(bitmap.data(), &mcx.ImageData[0][0], byteSize);
        bitmap2rgb(bitmap.data(), frame->getRGB().data(), frame->width(), frame->height(), COLOR_INDEX_OFFSET);
        frame->updateMap();
        add(frame.release());
    }
    auto fileSize = file.getSize();
    if (file.tell() < fileSize)
    {
        LOGW("Extra data after %s frame; possible format mismatch", FORMAT_GE96);
    }
    return true;
}

bool CFrameSet::importOBL5(IFile &file, const long org)
{
    CFrameSet frameSet;
    file.seek(org);
    if (!frameSet.read(file))
    {

        LOGE("last error:%s", frameSet.getLastError());
        m_lastError = "unsupported OBL5 version";
        return false;
    }
    auto fileSize = file.getSize();
    if (file.tell() < fileSize)
    {
        LOGW("Extra data after %s frame; possible format mismatch", FORMAT_OBL5);
    }

    size_t size = frameSet.getSize();
    m_arrFrames.reserve(size);
    for (size_t i = 0; i < size; ++i)
    {
        frameSet[i]->updateMap();
        m_arrFrames.push_back(frameSet[i]);
    }
    frameSet.removeAll();
    return true;
}

bool CFrameSet::importOBL4(IFile &file, const long org)
{
    file.seek(org);
    size_t fileSize = file.getSize();

    // Read and validate signature
    char signature[ID_SIG_LEN];
    if (file.read(signature, ID_SIG_LEN) != IFILE_OK || memcmp(signature, FORMAT_OBL4, ID_SIG_LEN) != 0)
    {
        m_lastError = "Invalid OBL4 signature";
        return false;
    }

    int32_t size = 0;
    file >> size;

    // Validate number of frames
    if (size <= 0 || size > MAX_IMAGES)
    { // Arbitrary max to prevent DoS
        m_lastError = "Invalid number of frames in OBL4";
        return false;
    }

    int32_t mode;
    file >> mode;
    m_arrFrames.reserve(size);
    for (int i = 0; i < size; ++i)
    {
        int32_t len;
        int32_t hei;
        file >> len;
        file >> hei;

        // Validate dimensions
        if (len <= 0 || hei <= 0 || len > MAX_IMAGE_SIZE || hei > MAX_IMAGE_SIZE)
        {
            m_lastError = "Invalid frame dimensions in OBL4";
            m_arrFrames.clear();
            return false;
        }

        // Check for overflow
        int64_t byteSize = static_cast<int64_t>(len) * hei;
        if (byteSize > static_cast<int64_t>(INT_MAX / sizeof(char)))
        {
            m_lastError = "Frame byte size overflow in OBL4";
            m_arrFrames.clear();
            return false;
        }

        int32_t mapped;
        file >> mapped;
        std::vector<char> bitmap(len * hei);
        std::unique_ptr<CFrame> frame = std::make_unique<CFrame>(len, hei);
        if (frame == nullptr)
            return false;
        if (mode != CFrame::MODE_ZLIB_ALPHA)
        {
            // Uncompressed mode
            if (file.read(bitmap.data(), frame->width() * frame->height()) != IFILE_OK)
            {
                m_lastError = "read error for OBL4";
                return false;
            }
        }
        else
        {
            // Zlib-compressed mode
            uint32_t nSrcLen = 0;
            if (file.read(&nSrcLen, sizeof(nSrcLen)) != IFILE_OK || nSrcLen <= 0 || nSrcLen > fileSize - file.tell())
            {
                m_lastError = "Invalid or truncated compressed size in OBL4";
                m_arrFrames.clear();
                return false;
            }
            std::vector<uint8_t> pSrc(nSrcLen);
            if (file.read(pSrc.data(), nSrcLen) != IFILE_OK)
            {
                m_lastError = "Failed to read OBL4 compressed data";
                m_arrFrames.clear();
                return false;
            }
            uLong nDestLen = frame->width() * frame->height();
            int err = uncompress(
                (uint8_t *)bitmap.data(),
                (uLong *)&nDestLen,
                (uint8_t *)pSrc.data(),
                (uLong)nSrcLen);
            if (err != Z_OK)
            {
                char tmp[128];
                snprintf(tmp, sizeof(tmp), "Zlib compression error %d: %s", err, zError(err));
                m_lastError = tmp;
            }
        }
        // Allocate RGB buffer
        // frame->resize(frame->width(), frame->height());
        // frame->setRGB(new uint32_t[frame->width() * frame->height()]);
        bitmap2rgb(bitmap.data(), frame->getRGB().data(), frame->width(), frame->height(), COLOR_INDEX_OFFSET);
        frame->updateMap();
        add(frame.release());
    }
    // Verify file position (ensure no extra data)
    if (file.tell() < (long)fileSize)
    {
        LOGW("Extra data after %s frames; possible format mismatch", FORMAT_OBL4);
    }
    return true;
}

bool CFrameSet::importOBL3(IFile &file, const long org)
{
    typedef struct
    {
        uint32_t PtrPrev;
        uint32_t PtrNext;
        uint32_t PtrBits;
        uint32_t PtrMap;
        uint32_t filler;
        char ExtraInfo[4];
    } USER_OBL3;

    struct USER_OBL3HEADER
    {
        char Id[ID_SIG_LEN]; // "OBL3"
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
    };

    file.seek(org);

    // Read and validate header
    USER_OBL3HEADER oblHead;
    if (file.read(&oblHead, sizeof(USER_OBL3HEADER)) != IFILE_OK)
    {
        m_lastError = "Failed to read OBL3 header";
        return false;
    }

    // Validate signature
    if (memcmp(oblHead.Id, FORMAT_OBL3, ID_SIG_LEN) != 0)
    {
        m_lastError = "Invalid OBL3 signature";
        return false;
    }

    // Sanity check header fields
    const uint32_t numImages = oblHead.iNbrImages;
    if (numImages == 0 || numImages > MAX_IMAGES)
    { // Arbitrary max to prevent DoS-like attacks
        m_lastError = "Invalid number of images in OBL3 header";
        return false;
    }

    const int frameLen = oblHead.iLen * OBL3_GRANULAR;
    const int frameHei = oblHead.iHei * OBL3_GRANULAR;
    if (frameLen <= 0 || frameHei <= 0 || frameLen > MAX_IMAGE_SIZE || frameHei > MAX_IMAGE_SIZE)
    { // Prevent overflow/large allocs
        m_lastError = "Invalid frame dimensions in OBL3 header";
        return false;
    }

    const long byteSize = frameLen * frameHei;
    if (byteSize <= 0 || byteSize > static_cast<long>(INT_MAX / sizeof(char)))
    { // Overflow check
        m_lastError = "Frame byte size overflow in OBL3";
        return false;
    }

    // Check total expected file size (header + numImages * (obl + bitmap))
    const long expectedSize = sizeof(USER_OBL3HEADER) + numImages * (sizeof(USER_OBL3) + byteSize);
    const long fileSize = file.getSize();
    if ((file.tell() + expectedSize - (long)sizeof(USER_OBL3HEADER)) > fileSize)
    {
        m_lastError = "OBL3 file too small for declared content";
        return false;
    }

    // Reserve space to avoid reallocs
    m_arrFrames.reserve(numImages);

    // Per-frame loop
    for (int i = 0; i < (int)oblHead.iNbrImages; ++i)
    {
        const int pixelLen = oblHead.iLen * OBL3_GRANULAR;
        const int pixelHei = oblHead.iHei * OBL3_GRANULAR;
        const int byteSize = pixelLen * pixelHei;
        std::vector<char> bitmap(byteSize);
        USER_OBL3 obl;

        // Read and check per-frame header (obl)
        if (file.read(&obl, sizeof(USER_OBL3)) != IFILE_OK)
        {
            m_lastError = "Failed to read OBL3 frame header";
            return false;
        }

        // Read bitmap
        if (file.read(bitmap.data(), byteSize) != IFILE_OK)
        {
            m_lastError = "Failed to read OBL3 frame bitmap";
            return false;
        }

        // Allocate RGB and map
        CFrame *frame = new CFrame(pixelLen, pixelHei);
        if (frame == nullptr)
            return false;
        bitmap2rgb(bitmap.data(), frame->getRGB().data(), frame->width(), frame->height(), COLOR_INDEX_OFFSET);
        frame->updateMap();
        add(frame);
    }
    if (file.tell() < fileSize)
    {
        LOGW("Extra data after %s frame; possible format mismatch", FORMAT_OBL3);
    }
    return true;
}

const char *CFrameSet::getLastError() const
{
    return m_lastError.c_str();
}

void CFrameSet::move(int s, int t)
{
    CFrame *f = removeAt(s);
    insertAt(t, f);
}

bool CFrameSet::toPng(std::vector<uint8_t> &png)
{
    png.clear();
    const size_t size = m_arrFrames.size();
    if (size == 1)
    {
        return m_arrFrames[0]->toPng(png);
    }
    else if (size > 1)
    {
        std::vector<uint16_t> xx(size);
        std::vector<uint16_t> yy(size);
        int width = 0;
        int height = 0;
        for (size_t i = 0; i < size; ++i)
        {
            width += m_arrFrames[i]->width();
            height = std::max(height, m_arrFrames[i]->height());
            xx[i] = m_arrFrames[i]->width();
            yy[i] = m_arrFrames[i]->height();
        }

        std::unique_ptr<CFrame> frame = std::make_unique<CFrame>(width, height);
        CFrame &t = *frame;
        int mx = 0;
        for (size_t i = 0; i < size; ++i)
        {
            CFrame &s = *(m_arrFrames[i]);
            for (int y = 0; y < s.height(); ++y)
            {
                for (int x = 0; x < s.width(); ++x)
                {
                    t.at(mx + x, y) = s.at(x, y);
                }
            }
            mx += s.width();
        }

        // prepare custom data to be injected
        int t_size = sizeof(CFrame::png_OBL5) + size * 2 * sizeof(uint16_t) + sizeof(uint32_t);
        std::vector<uint8_t> obl5t(t_size, '\0');
        CFrame::png_OBL5 *obl5data = (CFrame::png_OBL5 *)obl5t.data();
        obl5data->Length = CFrame::toNet(t_size - 12);
        memcpy(obl5data->ChunkType, CFrame::getChunkType(), 4);
        obl5data->Version = 0;
        obl5data->Count = size;
        memcpy(obl5t.data() + sizeof(CFrame::png_OBL5),
               xx.data(), size * sizeof(uint16_t));
        memcpy(obl5t.data() + sizeof(CFrame::png_OBL5) + size * sizeof(uint16_t),
               yy.data(), size * sizeof(uint16_t));

        // inject obldata into png
        frame->toPng(png, obl5t);
    }
    return true;
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
    const int last = end == -1 ? getSize() - 1 : end;
    dest.reserve(last - start);
    for (int i = start; i <= last; ++i)
    {
        CFrame *frame = new CFrame;
        frame->copy(m_arrFrames[i]);
        add(frame);
        // dest.add(new CFrame(m_arrFrames[i]));
    }
}

void CFrameSet::reserve(int n)
{
    m_arrFrames.reserve(n + m_arrFrames.size());
}

void CFrameSet::set(const int i, CFrame *frame)
{
    m_arrFrames[i] = frame;
}

int CFrameSet::currFrame()
{
    return m_nCurrFrame;
}

void CFrameSet::setCurrFrame(int curr)
{
    m_nCurrFrame = curr;
}
