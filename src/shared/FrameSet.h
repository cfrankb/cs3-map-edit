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
#include "ISerial.h"

class CFrame;
class IFile;

// FrameSet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFrameSet

class CFrameSet : public ISerial
{
public:
    CFrameSet();
    CFrameSet(CFrameSet *s);

public:
    int getSize();

public:
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
    bool extract(IFile &file, std::string *format = nullptr);

    static char *ima2bitmap(char *ImaData, int len, int hei);
    static void bitmap2rgb(char *bitmap, uint32_t *rgb, int len, int hei, int err);
    static bool isFriendFormat(const char *format);
    void move(int s, int t);

    const char *getLastError() const;
    void setLastError(const char *error);
    bool toPng(unsigned char *&data, int &size);
    std::string &tag(const char *tag);
    void setTag(const char *tag, const char *v);
    void copyTags(CFrameSet &src);
    void assignNewUUID();
    void toSubset(CFrameSet &dest, int start, int end = -1);

public:
    ~CFrameSet();
    virtual bool write(IFile &file);
    virtual bool read(IFile &file);
    int m_nCurrFrame;

private:
    enum
    {
        OBL_VERSION = 0x501,
        FNT_SIZE = 8,
        GE96_TILE_SIZE = 32,
        ID_SIG_LEN = 4,
        PALETTE_SIZE = 256,
        RGB_BYTES = 3,
        COLOR_INDEX_OFFSET = -16,
        COLOR_INDEX_OFFSET_NONE = 0,
    };

    bool write0x501(IFile &file);
    bool read0x501(IFile &file, int size);

    std::string m_lastError;
    std::vector<CFrame *> m_arrFrames;
    std::string m_name;
    std::unordered_map<std::string, std::string> m_tags;
    friend class CFrameArray;
};
