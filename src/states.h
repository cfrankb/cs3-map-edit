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
#include <unordered_map>
#include <vector>

struct StateValuePair
{
    uint16_t key;
    std::string value;
};


class IFile;

class CStates
{

public:
    CStates();
    ~CStates();

    void setU(const uint16_t k, const uint16_t v);
    void setS(const uint16_t k, const std::string &v);
    uint16_t getU(const uint16_t k);
    const char *getS(const uint16_t k);

    bool read(IFile &sfile);
    bool write(IFile &tfile);
    bool read(FILE *sfile);
    bool write(FILE *tfile);

    void debug();
    void clear();

    void getValues(std::vector<StateValuePair> & pairs);

private:
    std::unordered_map<uint16_t, std::string> m_stateS;
    std::unordered_map<uint16_t, uint16_t> m_stateU;

    enum
    {
        MAX_STRING = 256,
        COUNT_BYTES = sizeof(uint16_t),
        LEN_BYTES = sizeof(uint16_t),
    };
};
