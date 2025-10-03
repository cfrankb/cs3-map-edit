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

#include <string>
#include <unordered_map>
#include <cstdint>
#include <vector>
#include <memory>
#include "ISerial.h"

class CFrame;
class IFile;

class CFrameSet : public ISerial
{
public:
    CFrameSet();
    ~CFrameSet();
    CFrameSet(CFrameSet *s);

    enum Format : uint32_t
    {
        OBL5_UNPACKED = 0x500,
        OBL5_SOLID = 0x501,
        DEFAULT_OBL5_FORMAT = OBL5_SOLID,
    };

    size_t getSize();
    int operator++();
    int operator--();
    CFrame *operator[](int) const;
    CFrameSet &operator=(CFrameSet &s);
    int add(CFrame *pFrame);
    void setName(const char *s);
    const char *getName() const;

    CFrame *removeAt(int n);
    void insertAt(int n, CFrame *pFrame);
    void clear();
    void removeAll();
    bool extract(IFile &file);
    void move(int s, int t);

    const char *getLastError() const;
    void setLastError(const char *error);
    bool toPng(std::vector<uint8_t> &png);
    std::string &tag(const char *tag);
    void setTag(const char *tag, const char *v);
    void copyTags(CFrameSet &src);
    void assignNewUUID();
    void toSubset(CFrameSet &dest, int start, int end = -1);

    bool write(IFile &file, const Format format);
    bool write(IFile &file) override;
    bool read(IFile &file) override;

    void set(const int i, CFrame *frame);
    void reserve(int n);
    int currFrame();
    void setCurrFrame(int curr);

private:
    int m_nCurrFrame;
    enum
    {
        FNT_SIZE = 8,
        GE96_TILE_SIZE = 32,
        ID_SIG_LEN = 4,
        PALETTE_SIZE = 256,
        RGB_BYTES = 3,
        COLOR_INDEX_OFFSET = -16,
        COLOR_INDEX_OFFSET_NONE = 0,
        OBL3_GRANULAR = 16,
        MAX_IMAGES = 1024,
        MAX_IMAGE_SIZE = 256,
        TAG_KEY_MAX = 32,
        TAG_VAL_MAX = 1024,
    };

    bool writeSolid(IFile &file);
    bool readSolid(IFile &file, int size);
    static std::unique_ptr<char[]> ima2bitmap(char *ImaData, int len, int hei);
    static void bitmap2rgb(char *bitmap, uint32_t *rgb, int len, int hei, int err);
    bool importIMA(IFile &file, const long org = 0);
    bool importIMC1(IFile &file, const long org = 0);
    bool importGE96(IFile &file, const long org = 0);
    bool importOBL3(IFile &file, const long org = 0);
    bool importOBL4(IFile &file, const long org = 0);
    bool importOBL5(IFile &file, const long org = 0);

    std::string m_lastError;
    std::vector<CFrame *> m_arrFrames;
    std::string m_name;
    std::unordered_map<std::string, std::string> m_tags;
    friend class CFrameArray;
};
