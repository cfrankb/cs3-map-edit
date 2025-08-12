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
#include "states.h"
#include "shared/IFile.h"

CStates::CStates()
{
}

CStates::~CStates()
{
}

void CStates::setU(const uint16_t k, const uint16_t v)
{
    if (v)
        m_stateU[k] = v;
    else
        m_stateU.erase(k);
}

void CStates::setS(const uint16_t k, const std::string &v)
{
    if (v.size())
        m_stateS[k] = v;
    else
        m_stateS.erase(k);
}

uint16_t CStates::getU(const uint16_t k)
{
    if (m_stateU.count(k))
        return m_stateU[k];
    else
        return 0;
}

const char *CStates::getS(const uint16_t k)
{
    if (m_stateS.count(k))
        return m_stateS[k].c_str();
    else
        return "";
}

bool CStates::read(IFile &sfile)
{
    auto readfile = [&sfile](auto ptr, auto size)
    {
        return sfile.read(reinterpret_cast<void *>(ptr), size);
    };

    size_t count = 0;
    readfile(&count, COUNT_BYTES);
    m_stateU.clear();
    for (size_t i = 0; i < count; ++i)
    {
        uint16_t k = 0;
        uint16_t v = 0;
        readfile(&k, sizeof(k));
        readfile(&v, sizeof(v));
        m_stateU[k] = v;
    }
    char *v = new char[MAX_STRING];
    readfile(&count, COUNT_BYTES);
    m_stateS.clear();
    for (size_t i = 0; i < count; ++i)
    {
        uint16_t k = 0;
        uint16_t len = 0;
        readfile(&k, sizeof(k));
        readfile(&len, sizeof(len));
        v[len] = '\0';
        readfile(v, len);
        m_stateS[k] = v;
    }
    delete[] v;
    return true;
}

bool CStates::write(IFile &tfile)
{
    auto writefile = [&tfile](auto ptr, auto size)
    {
        return tfile.write(ptr, size);
    };

    size_t count = m_stateU.size();
    writefile(&count, COUNT_BYTES);
    for (const auto &[k, v] : m_stateU)
    {
        writefile(&k, sizeof(k));
        writefile(&v, sizeof(v));
    }

    count = m_stateS.size();
    writefile(&count, COUNT_BYTES);
    for (const auto &[k, v] : m_stateS)
    {
        writefile(&k, sizeof(k));
        size_t len = v.size();
        writefile(&len, LEN_BYTES);
        writefile(v.c_str(), len);
    }
    return true;
}

bool CStates::read(FILE *sfile)
{
    auto readfile = [sfile](auto ptr, auto size)
    {
        return fread(ptr, size, 1, sfile) == 1;
    };

    size_t count = 0;
    readfile(&count, COUNT_BYTES);
    m_stateU.clear();
    for (size_t i = 0; i < count; ++i)
    {
        uint16_t k = 0;
        uint16_t v = 0;
        readfile(&k, sizeof(k));
        readfile(&v, sizeof(v));
        m_stateU[k] = v;
    }
    char *v = new char[MAX_STRING];
    readfile(&count, COUNT_BYTES);
    m_stateS.clear();
    for (size_t i = 0; i < count; ++i)
    {
        uint16_t k = 0;
        uint16_t len = 0;
        readfile(&k, sizeof(k));
        readfile(&len, sizeof(len));
        v[len] = '\0';
        readfile(v, len);
        m_stateS[k] = v;
    }
    delete[] v;
    return true;
}

bool CStates::write(FILE *tfile)
{
    auto writefile = [tfile](auto ptr, auto size)
    {
        return fwrite(ptr, size, 1, tfile) == 1;
    };

    size_t count = m_stateU.size();
    writefile(&count, COUNT_BYTES);
    for (const auto &[k, v] : m_stateU)
    {
        writefile(&k, sizeof(k));
        writefile(&v, sizeof(v));
    }

    count = m_stateS.size();
    writefile(&count, COUNT_BYTES);
    for (const auto &[k, v] : m_stateS)
    {
        writefile(&k, sizeof(k));
        size_t len = v.size();
        writefile(&len, LEN_BYTES);
        writefile(v.c_str(), len);
    }
    return true;
}

void CStates::clear()
{
    m_stateS.clear();
    m_stateU.clear();
}

void CStates::debug()
{
    printf("\n**** m_stateU: %ld\n\n", m_stateU.size());
    for (const auto &[k, v] : m_stateU)
    {
        printf("[%d / 0x%.2x] => [%d / 0x%.2x]\n", k, k, v, v);
    }

    printf("\n***** m_stateS: %ld\n\n", m_stateS.size());
    for (const auto &[k, v] : m_stateS)
    {
        printf("[%d / 0x%.2x] => [%s]\n", k, k, v.c_str());
    }
}

void CStates::getValues(std::vector<StateValuePair> &pairs)
{
    // C++ 20 not supported yet
    // std::format("0x{:02x}", v)
    pairs.clear();
    char tmp1[16];
    char tmp2[16];
    for (const auto &[k, v] : m_stateU)
    {
        sprintf(tmp1, "0x%.2x", v);
        sprintf(tmp2, "%d", v);
        pairs.push_back({k, v ? tmp1 : "", v ? tmp2 : ""});
    }

    for (const auto &[k, v] : m_stateS)
    {
        pairs.push_back({k, v, ""});
    }
}

void CStates::operator=(const CStates &s)
{
    for (const auto &[k, v] : s.m_stateU)
    {
        m_stateU[k] = v;
    }

    for (const auto &[k, v] : s.m_stateS)
    {
        m_stateS[k] = v;
    }
}

bool CStates::hasU(const uint16_t k)
{
    return m_stateU.count(k) != 0;
}

bool CStates::hasS(const uint16_t k)
{
    return m_stateS.count(k) != 0;
}
