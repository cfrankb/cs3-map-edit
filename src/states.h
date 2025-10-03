/*
    cs3-runtime-sdl
    Copyright (C) 2025 Francois Blanchette

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

#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

struct StateValuePair
{
    uint16_t key;
    std::string value;
    std::string tip;
};

class IFile;

class CStates
{

public:
    CStates() = default;
    ~CStates() = default;

    void setU(const uint16_t k, const uint16_t v);
    void setS(const uint16_t k, const std::string &v);
    uint16_t getU(const uint16_t k) const;
    const char *getS(const uint16_t k) const;
    bool hasU(const uint16_t k) const;
    bool hasS(const uint16_t k) const;

    bool read(IFile &sfile);
    bool write(IFile &tfile) const;
    bool read(FILE *sfile);
    bool write(FILE *tfile) const;
    bool fromMemory(uint8_t *ptr);

    void debug() const;
    void clear();
    std::vector<StateValuePair> getValues() const;

private:
    std::unordered_map<uint16_t, std::string> m_stateS;
    std::unordered_map<uint16_t, uint16_t> m_stateU;
    enum
    {
        MAX_STRING = 1024,
        COUNT_BYTES = sizeof(uint16_t),
        LEN_BYTES = sizeof(uint16_t),
    };

    template <typename ReadFunc>
    bool readCommon(ReadFunc readfile);

    template <typename WriteFunc>
    bool writeCommon(WriteFunc writefile) const;
};