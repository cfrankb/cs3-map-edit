/*
    LGCK Builder Runtime
    Copyright (C) 1999, 2016, 2025  Francois Blanchette

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

#include "FileMem.h"
#include <cstring>
#include <cstdio>
#include <cassert>

CFileMem::CFileMem()
{
    m_ptr = 0;
    m_mode = "rb";
}

CFileMem::~CFileMem()
{
    close();
}

void CFileMem::append(const void *data, int size)
{
    const size_t newSize = m_ptr + size;
    if (newSize > m_buffer.size())
        m_buffer.resize(newSize);

    memcpy(m_buffer.data() + m_ptr, data, size);
    m_ptr += size;
}

bool CFileMem::operator>>(int &n)
{
    return read(&n, sizeof(n));
}

bool CFileMem::operator<<(int n)
{
    return write(&n, sizeof(n));
}

int CFileMem::read(void *buf, int size)
{
    if (m_mode.find('r') == std::string::npos)
        return IFILE_NOT_OK;
    int leftBytes = m_buffer.size() - m_ptr;
    if (leftBytes >= size)
    {
        memcpy(buf, &m_buffer[m_ptr], size);
        m_ptr += size;
        return IFILE_OK;
    }
    else
    {
        return IFILE_NOT_OK;
    }
}

int CFileMem::write(const void *buf, int size)
{
    if (m_mode.find('w') == std::string::npos &&
        m_mode.find('a') == std::string::npos)
        return IFILE_NOT_OK;
    append(buf, size);
    return IFILE_OK;
}

bool CFileMem::open(const std::string_view &fileName, const std::string_view &mode)
{
    m_mode = mode;
    m_filename = fileName;
    if (m_mode.find('a') != std::string::npos && m_buffer.size() > 0)
        m_ptr = m_buffer.size() - 1;
    else
        m_ptr = 0;

    // TODO: fix that later
    return m_mode.find('r') != std::string::npos ||
           m_mode.find('w') != std::string::npos ||
           m_mode.find('a') != std::string::npos;
}

bool CFileMem::close()
{
    // TODO:
    m_buffer.clear();
    m_ptr = 0;
    return true;
}

long CFileMem::getSize()
{
    return m_buffer.size();
}

bool CFileMem::seek(long p)
{
    m_ptr = p;
    return true;
}

long CFileMem::tell()
{
    return m_ptr;
}

bool CFileMem::operator>>(std::string &str)
{
    if (m_ptr + sizeof(uint8_t) > m_buffer.size())
        return false;
    size_t length = m_buffer[m_ptr];
    ++m_ptr;
    if (length == 0xff)
    {
        if (m_ptr + sizeof(uint16_t) > m_buffer.size())
            return false;
        memcpy(&length, &m_buffer[m_ptr], sizeof(uint16_t));
        m_ptr += sizeof(uint16_t);
        // implemented 32 bits version
        if (length == 0xffff)
        {
            memcpy(&length, &m_buffer[m_ptr], sizeof(uint32_t));
            m_ptr += sizeof(uint32_t);
        }
    }
    str.resize(length);
    if (length != 0)
    {
        memcpy(str.data(), &m_buffer[m_ptr], length);
        m_ptr += length;
    }

    return true;
}

bool CFileMem::operator<<(const std::string_view &str)
{
    size_t length = str.length();
    if (length < 0xff)
    {
        append(&length, sizeof(uint8_t));
    }
    else
    {
        const uint8_t t = 0xff;
        append(&t, sizeof(t));
        if (length >= 0xffff)
        {
            // implemented 32bits version
            const uint16_t t = 0xffff;
            append(&t, sizeof(t));
            append(&length, sizeof(uint32_t));
        }
        else
        {
            // 16 bits
            append(&length, sizeof(uint16_t));
        }
    }
    if (length != 0)
    {
        append(str.data(), length);
    }
    return true;
}

bool CFileMem::operator>>(bool &b)
{
    memset(&b, 0, sizeof(b));
    return read(&b, 1);
}

bool CFileMem::operator<<(const bool b)
{
    return write(&b, 1);
}

bool CFileMem::operator+=(const std::string_view &str)
{
    return write(str.data(), str.size()); // return true;
}

bool CFileMem::operator+=(const char *s)
{
    return write(s, strlen(s));
}

const std::vector<uint8_t> &CFileMem::buffer()
{
    return m_buffer;
}

void CFileMem::replace(const uint8_t *buffer, size_t size)
{
    m_buffer.assign(buffer, buffer + size);
    m_ptr = 0;
}

bool CFileMem::flush()
{
    return true;
}

bool CFileMem::operator<<(const char *s)
{
    std::string_view sv(s);
    return *this << sv; // Delegate to string_view overload
}

const std::string_view CFileMem::mode()
{
    return m_mode;
}