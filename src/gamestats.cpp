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
#include "gamestats.h"
#include "shared/IFile.h"

CGameStats::CGameStats()
{
}

CGameStats::~CGameStats()
{
}

/**
 * @brief Return a reference to the value associated with given key
 *
 * @param key
 * @return int&
 */
int &CGameStats::get(const GameStat key)
{
    return m_stats[key];
}

/**
 * @brief Return value associated with given key. value returned cannot be modified
 *
 * @param key
 * @return int
 */
int CGameStats::at(const GameStat key) const
{
    auto it = m_stats.find(key);
    return it != m_stats.end() ? it->second : 0;
}

/**
 * @brief Set value associated with given key
 *
 * @param key
 * @param value
 */
void CGameStats::set(const GameStat key, int value)
{
    m_stats[key] = value;
}

/**
 * @brief Decrement value associated with given key. Doesn't decrement values below zero
 *
 * @param key
 * @return int& new value
 */
int &CGameStats::dec(const GameStat key)
{
    if (m_stats[key] > 0)
    {
        --m_stats[key];
    }
    return m_stats[key];
}

/**
 * @brief Increment value associated with given key.
 *
 * @param key
 * @return int& new value
 */
int &CGameStats::inc(const GameStat key)
{
    return ++m_stats[key];
}

/**
 * @brief Deserialize key/value pairs from disk
 *
 * @param sfile
 * @return true
 * @return false
 */

bool CGameStats::read(FILE *sfile)
{
    if (!sfile)
        return false;

    auto readfile = [sfile](auto ptr, auto size) -> bool
    {
        return fread(ptr, size, 1, sfile) == 1;
    };

    return readCommon(readfile);
}

/**
 * @brief Deserialize key/value pairs from disk
 *
 * @param sfile
 * @return true
 * @return false
 */
bool CGameStats::read(IFile &sfile)
{
    auto readfile = [&sfile](auto ptr, auto size) -> bool
    {
        return sfile.read(ptr, size) == 1;
    };

    return readCommon(readfile);
}

template <typename ReadFunc>
bool CGameStats::readCommon(ReadFunc readfile)
{
    uint16_t count = 0;
    m_stats.clear();
    if (!readfile(&count, sizeof(count)))
        return false;

    for (size_t i = 0; i < count; ++i)
    {
        uint16_t key;
        int32_t value;

        if (!readfile(&key, sizeof(key)))
            return false;
        if (!readfile(&value, sizeof(value)))
            return false;

        m_stats[key] = value;
    }
    return true;
}

/**
 * @brief Serialize key/value pairs to disk
 *
 * @param sfile
 * @return true
 * @return false
 */

bool CGameStats::write(FILE *tfile) const
{
    if (!tfile)
        return false;

    auto writefile = [tfile](auto ptr, auto size) -> bool
    {
        return fwrite(ptr, size, 1, tfile) == 1;
    };

    return writeCommon(writefile);
}

bool CGameStats::write(IFile &tfile) const
{
    auto writefile = [&tfile](auto ptr, auto size) -> bool
    {
        return tfile.write(ptr, size) == 1;
    };

    return writeCommon(writefile);
}

template <typename WriteFunc>
bool CGameStats::writeCommon(WriteFunc writefile) const
{
    uint16_t count = 0;
    for (const auto &[key, value] : m_stats)
    {
        if (value)
            ++count;
    }

    if (!writefile(&count, sizeof(count)))
        return false;

    for (const auto &[key, value] : m_stats)
    {
        if (value)
        {
            if (!writefile(&key, sizeof(key)))
                return false;
            if (!writefile(&value, sizeof(value)))
                return false;
        }
    }

    return true;
}