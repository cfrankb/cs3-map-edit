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
    if (!m_file)
        return IFILE_NOT_OK;
    if (size < 0)
        return IFILE_NOT_OK; // negative length would wrap to a huge size_t
    if (size == 0)
        return IFILE_OK; // reading nothing is not a failure
    return fread(buf, (size_t)size, 1, m_file) == 1 ? IFILE_OK : IFILE_NOT_OK;
}

int CFileWrap::write(const void *buf, int size)
{
    if (m_memFile)
        return m_memFile->write(buf, size);
    if (!m_file)
        return IFILE_NOT_OK;
    if (size < 0)
        return IFILE_NOT_OK; // negative length would wrap to a huge size_t
    if (size == 0)
        return IFILE_OK; // writing nothing is not a failure
    return fwrite(buf, (size_t)size, 1, m_file) == 1 ? IFILE_OK : IFILE_NOT_OK;
}

bool CFileWrap::open(const std::string_view &fileName, const std::string_view &mode)
{
    m_mode = mode;
    close(); // release any previously open FILE* before switching targets
    if (fileName.empty())
        return false;
    if ((m_memFile = findFile(fileName)))
    {
        return m_memFile->open(fileName, mode);
    }
    else
    {
        m_file = fopen(std::string(fileName).c_str(), std::string(mode).c_str());
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
    else if (m_file)
    {
        // audit FileWrap.cpp L26: a failed ftell must propagate as -1 without
        // the bogus fseek(m_file, -1, SEEK_SET) restore
        const long cur = ftell(m_file);
        if (cur < 0)
            return -1;
        if (fseek(m_file, 0, SEEK_END) != 0)
            return -1;
        const long w = ftell(m_file);
        fseek(m_file, cur, SEEK_SET);
        return w;
    }
    return -1;
}

bool CFileWrap::seek(long p)
{
    if (m_memFile)
        return m_memFile->seek(p);
    return m_file && fseek(m_file, p, SEEK_SET) == 0;
}

long CFileWrap::tell()
{
    if (m_memFile)
        return m_memFile->tell();
    return m_file ? ftell(m_file) : -1;
}

bool CFileWrap::operator>>(std::string &str)
{
    if (m_memFile)
    {
        return *m_memFile >> str;
    }
    else
    {
        if (!m_file)
            return false;
        // BUG-C10: decode with width-exact locals mirroring CFileMem: a byte
        // < 0xff is the length; 0xff introduces a uint16; a uint16 of 0xffff
        // introduces the uint32 content size. Reusing one int for all reads
        // made the 0xffff case dead and desynced the stream on 32-bit writes.
        uint8_t byte = 0;
        if (fread(&byte, sizeof(byte), 1, m_file) != 1)
            return false;
        uint32_t size = byte;
        if (byte == 0xff)
        {
            uint16_t w16 = 0;
            if (fread(&w16, sizeof(w16), 1, m_file) != 1)
                return false;
            size = w16;
            if (w16 == 0xffff)
            {
                // 32 bits version
                if (fread(&size, sizeof(uint32_t), 1, m_file) != 1)
                    return false;
            }
        }
        // the declared payload must actually be in the file (mirrors BUG-C1)
        const long remain = getSize() - ftell(m_file);
        if (remain < 0 || static_cast<size_t>(size) > static_cast<size_t>(remain))
            return false;
        str.resize(size); // Allocate space in str
        if (size != 0 && fread(str.data(), size, 1, m_file) != 1)
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
    if (!m_file)
        return false;

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
    // BUG-H8: zero-length body is a success no-op (fwrite(data, 0, 1) returns
    // 0; CFileMem already treats it as success — mirror it)
    if (size != 0 && fwrite(str.data(), size, 1, m_file) != 1)
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