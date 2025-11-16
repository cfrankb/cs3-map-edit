/*
    cs3-runtime-sdl
    Copyright (C) 2024  Francois Blanchette

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

#include <unordered_map>
#include <string>
#include <functional>
#include <memory> // For unique_ptr
#include "shared/IFile.h"

typedef std::unordered_map<uint16_t, uint8_t> AttrMap;
struct Pos
{
    int16_t x;
    int16_t y;
    bool operator==(const Pos &other) const
    {
        return (x == other.x) && (y == other.y);
    }
    bool operator!=(const Pos &other) const
    {
        return x != other.x || y != other.y;
    }

    bool write(IFile &tfile) const
    {
        if (tfile.write(&x, sizeof(x)) != IFILE_OK)
            return false;
        if (tfile.write(&y, sizeof(y)) != IFILE_OK)
            return false;
        return true;
    }
    bool read(IFile &sfile)
    {
        if (sfile.read(&x, sizeof(x)) != IFILE_OK)
            return false;
        if (sfile.read(&y, sizeof(y)) != IFILE_OK)
            return false;
        return true;
    }
};

class IFile;
class CStates;

class CMap
{
public:
    CMap(uint16_t len = 0, uint16_t hei = 0, uint8_t t = 0);
    CMap(const CMap &map);
    ~CMap();
    uint8_t at(const int x, const int y) const;
    void set(const int x, const int y, const uint8_t t);
    uint8_t &get(const int x, const int y);
    bool read(const char *fname);
    bool write(const char *fname) const;
    bool read(FILE *sfile);
    bool read(IFile &file);
    bool write(FILE *tfile) const;
    bool write(IFile &tfile) const;
    void clear();
    int len() const;
    int hei() const;
    bool resize(uint16_t in_len, uint16_t in_hei, uint8_t t, bool fast);
    const Pos findFirst(const uint8_t tileId) const;
    size_t count(const uint8_t tileId) const;
    void fill(uint8_t ch = 0);
    uint8_t getAttr(const uint8_t x, const uint8_t y) const;
    void setAttr(const uint8_t x, const uint8_t y, const uint8_t a);
    size_t size() const;
    const char *lastError();
    CMap &operator=(const CMap &map);
    bool fromMemory(uint8_t *mem);
    const char *title();
    void setTitle(const char *title);
    void replaceTile(const uint8_t, const uint8_t);
    const AttrMap &attrs() { return m_attrs; }
    CStates &states();
    inline const CStates &statesConst() const { return *m_states; };
    static uint16_t toKey(const uint8_t x, const uint8_t y);
    static uint16_t toKey(const Pos &pos);
    static Pos toPos(const uint16_t key);
    inline bool isValid(const int x, const int y) const
    {
        return x >= 0 && x < m_len && y >= 0 && y < m_hei;
    }

    enum Direction : int16_t
    {
        UP,
        DOWN,
        LEFT,
        RIGHT,
        MAX = RIGHT,
        NOT_FOUND = -1 // 0xffff
    };
    void shift(Direction aim);
    void debug();

private:
    template <typename WriteFunc>
    bool writeCommon(WriteFunc writefile) const;
    template <typename ReadFunc>
    bool readImpl(ReadFunc &&readfile, std::function<size_t()> tell, std::function<bool(size_t)> seek, std::function<bool()> readStates);

    uint16_t m_len;
    uint16_t m_hei;
    std::vector<uint8_t> m_map;
    AttrMap m_attrs;
    std::string m_lastError;
    std::string m_title;
    std::unique_ptr<CStates> m_states;
};
