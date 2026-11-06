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
#include "logger.h"
#include <cstring>

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

uint16_t CStates::getU(const uint16_t k) const
{
    const auto &it = m_stateU.find(k);
    if (it != m_stateU.end())
        return it->second;
    else
        return 0;
}

const char *CStates::getS(const uint16_t k) const
{
    const auto &it = m_stateS.find(k);
    if (it != m_stateS.end())
        return it->second.c_str();
    else
        return "";
}

bool CStates::read(IFile &sfile)
{
    auto readfile = [&sfile](auto ptr, auto size) -> bool
    {
        return sfile.read(reinterpret_cast<void *>(ptr), size) == 1;
    };

    return readCommon(readfile);
}

bool CStates::read(FILE *sfile)
{
    if (!sfile)
        return false;

    auto readfile = [sfile](auto ptr, auto size) -> bool
    {
        return fread(ptr, size, 1, sfile) == 1;
    };

    return readCommon(readfile);
}

template <typename ReadFunc>
bool CStates::readCommon(ReadFunc readfile)
{
    size_t count = 0;
    if (!readfile(&count, COUNT_BYTES))
        return false;

    m_stateU.clear();
    for (size_t i = 0; i < count; ++i)
    {
        uint16_t k = 0;
        uint16_t v = 0;
        if (!readfile(&k, sizeof(k)))
            return false;
        if (!readfile(&v, sizeof(v)))
            return false;
        m_stateU[k] = v;
    }

    std::vector<char> v(MAX_STRING);
    if (!v.data())
    {
        LOGE("allocation of v failed");
        return false;
    }

    if (!readfile(&count, COUNT_BYTES))
    {
        return false;
    }

    m_stateS.clear();
    for (size_t i = 0; i < count; ++i)
    {
        uint16_t k = 0;
        uint16_t len = 0;
        if (!readfile(&k, sizeof(k)))
        {
            return false;
        }
        if (!readfile(&len, sizeof(len)))
        {
            return false;
        }

        if (len >= MAX_STRING)
        {
            return false;
        }

        v[len] = '\0';
        if (!readfile(v.data(), len))
        {
            return false;
        }
        m_stateS[k] = v.data();
    }
    return true;
}

bool CStates::fromMemory(uint8_t *ptr)
{
    auto copyData = [&ptr](auto dest, auto size)
    {
        memcpy(dest, ptr, size);
        ptr += size;
        return true;
    };
    return readCommon(copyData);
}

bool CStates::write(IFile &tfile) const
{
    auto writefile = [&tfile](auto ptr, auto size) -> bool
    {
        return tfile.write(ptr, size) == 1;
    };

    return writeCommon(writefile);
}

bool CStates::write(FILE *tfile) const
{
    if (!tfile)
        return false;

    auto writefile = [tfile](auto ptr, auto size) -> bool
    {
        return fwrite(ptr, size, 1, tfile) == 1;
    };

    return writeCommon(writefile);
}

template <typename WriteFunc>
bool CStates::writeCommon(WriteFunc writefile) const
{
    size_t count = m_stateU.size();
    if (!writefile(&count, COUNT_BYTES))
        return false;

    for (const auto &[k, v] : m_stateU)
    {
        if (!writefile(&k, sizeof(k)))
            return false;
        if (!writefile(&v, sizeof(v)))
            return false;
    }

    count = m_stateS.size();
    if (!writefile(&count, COUNT_BYTES))
        return false;

    for (const auto &[k, v] : m_stateS)
    {
        if (!writefile(&k, sizeof(k)))
            return false;

        size_t len = v.size();
        if (!writefile(&len, LEN_BYTES))
            return false;
        if (!writefile(v.c_str(), len))
            return false;
    }

    return true;
}

void CStates::clear()
{
    m_stateS.clear();
    m_stateU.clear();
}

void CStates::debug() const
{
    LOGI("\n**** m_stateU: %zu", m_stateU.size());
    for (const auto &[k, v] : m_stateU)
    {
        LOGI("[%d / 0x%.2x] => [%d / 0x%.2x]", k, k, v, v);
    }

    LOGI("\n***** m_stateS: %zu", m_stateS.size());
    for (const auto &[k, v] : m_stateS)
    {
        LOGI("[%d / 0x%.2x] => [%s]", k, k, v.c_str());
    }
}

std::vector<StateValuePair> CStates::getValues() const
{
    std::vector<StateValuePair> pairs;
    // TODO: C++ 20 not supported yet by appImage
    // std::format("0x{:02x}", v)
    pairs.clear();
    pairs.reserve(m_stateS.size() + m_stateU.size());
    char tmp1[16];
    char tmp2[16];
    for (const auto &[k, v] : m_stateU)
    {
        if (v <= 0xff)
            snprintf(tmp1, sizeof(tmp1), "0x%.2x", v);
        else
            snprintf(tmp1, sizeof(tmp1), "0x%.4x", v);
        snprintf(tmp2, sizeof(tmp2), "%d", v);
        pairs.emplace_back(StateValuePair{k, v ? tmp1 : "", v ? tmp2 : ""});
    }

    for (const auto &[k, v] : m_stateS)
    {
        pairs.emplace_back(StateValuePair{k, v, ""});
    }
    return pairs;
}

bool CStates::hasU(const uint16_t k) const
{
    return m_stateU.count(k) != 0;
}

bool CStates::hasS(const uint16_t k) const
{
    return m_stateS.count(k) != 0;
}

bool CStates::operator==(const CStates &other) const
{

    return m_stateU == other.m_stateU &&
           m_stateS == other.m_stateS;
}

bool CStates::operator!=(const CStates &states) const
{
    return !(*this == states);
}