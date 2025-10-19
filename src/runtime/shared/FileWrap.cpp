/*
    LGCK Builder Runtime
    Copyright (C) 1999, 2011, 2025  Francois Blanchette

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

#include "FileWrap.h"
#include <cstring>
#include <cstdio>
#include <vector>
#include <cassert>
#include "logger.h"
#include "FileMem.h"

std::unordered_map<std::string, std::unique_ptr<CFileMem>> CFileWrap::m_files;

CFileWrap::CFileWrap()
{
    m_file = nullptr;
    m_memFile = nullptr;
}

CFileWrap::~CFileWrap()
{
    close();
}

void CFileWrap::addFile(const std::string_view &fileName, const std::vector<uint8_t> &data)
{
    std::unique_ptr<CFileMem> mem = std::make_unique<CFileMem>();
    mem->replace(data.data(), data.size());
    m_files[std::string(fileName)] = std::move(mem);
}

void CFileWrap::freeFiles()
{
    m_files.clear();
}

CFileMem *CFileWrap::findFile(const std::string_view &fileName)
{
    const auto &it = m_files.find(std::string(fileName));
    if (it != m_files.end())
        return it->second.get();
    else
        return nullptr;
}

bool CFileWrap::operator>>(int &n)
{
    return read(&n, sizeof(n)) == IFILE_OK;
}

bool CFileWrap::operator<<(int n)
{
    return write(&n, sizeof(n)) == IFILE_OK;
}

int CFileWrap::read(void *buf, int size)
{
    if (m_memFile)
        return m_memFile->read(buf, size);
    else
        return fread(buf, size, 1, m_file) == 1 ? IFILE_OK : IFILE_NOT_OK;
}

int CFileWrap::write(const void *buf, int size)
{
    if (m_memFile)
        return m_memFile->write(buf, size);
    else
        return fwrite(buf, size, 1, m_file) == 1 ? IFILE_OK : IFILE_NOT_OK;
}

bool CFileWrap::open(const std::string_view &fileName, const std::string_view &mode)
{
    m_mode = mode;
    if ((m_memFile = findFile(fileName)))
    {
        return m_memFile->open(fileName, mode);
    }
    else
    {
        m_file = fopen(fileName.data(), mode.data());
        return m_file != nullptr;
    }
}

bool CFileWrap::close()
{
    bool result = false;
    if (m_memFile)
    {
        return m_memFile->close();
    }
    else if (m_file)
    {
        result = fclose(m_file) == 0;
        m_file = nullptr;
    }
    return result;
}

long CFileWrap::getSize()
{
    if (m_memFile)
    {
        return m_memFile->getSize();
    }
    else
    {
        long cur = ftell(m_file);
        fseek(m_file, 0, SEEK_END);
        long w = ftell(m_file);
        fseek(m_file, cur, SEEK_SET);
        return w;
    }
}

bool CFileWrap::seek(long p)
{
    if (m_memFile)
        return m_memFile->seek(p);
    else if (fseek(m_file, p, SEEK_SET) != 0)
        return false;
    return true;
}

long CFileWrap::tell()
{
    return m_memFile ? m_memFile->tell() : ftell(m_file);
}

bool CFileWrap::operator>>(std::string &str)
{
    if (m_memFile)
    {
        return *m_memFile >> str;
    }
    else
    {
        int size = 0;
        if (fread(&size, 1, 1, m_file) != 1)
            return false;
        if (size == 0xff)
        {
            if (size == 0xffff)
            {
                // 32 bits version
                if (fread(&size, sizeof(uint32_t), 1, m_file) != 1)
                    return false;
            }
            else
            {
                // 16 bits
                if (fread(&size, sizeof(uint16_t), 1, m_file) != 1)
                    return false;
            }
        }
        str.resize(size); // Allocate space in str
        if (fread(str.data(), size, 1, m_file) != 1)
        {
            str.clear(); // Reset on failure
            return false;
        }
    }
    return true;
}

bool CFileWrap::operator<<(const std::string_view &str)
{
    if (m_memFile)
        return *m_memFile << str;

    const size_t size = str.length();
    if (size < 0xff)
    {
        if (fwrite(&size, sizeof(uint8_t), 1, m_file) != 1)
            return false;
    }
    else
    {
        const uint8_t control = 0xff;
        if (fwrite(&control, sizeof(control), 1, m_file) != 1)
            return false;
        if (size < 0xffff)
        {
            if (fwrite(&size, sizeof(uint16_t), 1, m_file) != 1)
                return false;
        }
        else
        {
            // implemented 32bits version
            const uint16_t control = 0xffff;
            if (fwrite(&control, sizeof(control), 1, m_file) != 1)
                return false;
            if (fwrite(&size, sizeof(uint32_t), 1, m_file) != 1)
                return false;
        }
    }
    if (fwrite(str.data(), size, 1, m_file) != 1)
        return false;
    return true;
}

bool CFileWrap::operator>>(bool &b)
{
    memset(&b, '\0', sizeof(b));
    return read(&b, 1);
}

bool CFileWrap::operator<<(bool b)
{
    return write(&b, 1);
}

bool CFileWrap::operator+=(const std::string_view &str)
{
    return write(str.data(), str.length());
}

bool CFileWrap::operator+=(const char *s)
{
    return write(s, strlen(s));
}

bool CFileWrap::flush()
{
    if (m_file)
        return fflush(m_file) == 0;
    return true;
}

bool CFileWrap::operator<<(const char *s)
{
    std::string_view sv(s);
    return *this << sv; // Delegate to string_view overload
}

const std::string_view CFileWrap::mode()
{
    return m_mode;
}