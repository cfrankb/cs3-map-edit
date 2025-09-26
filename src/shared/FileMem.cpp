/*
    LGCK Builder Runtime
    Copyright (C) 1999, 2016  Francois Blanchette

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
    m_size = 0;
    m_ptr = 0;
    m_max = GROWBY;
    m_buffer = new char[m_max];
}

CFileMem::~CFileMem()
{
    close();
    if (m_buffer)
    {
        delete[] m_buffer;
    }
}

void CFileMem::grow(int size)
{
    if (m_size + size >= m_max)
    {
        m_max += GROWBY;
        char *t = new char[m_max];
        memcpy(t, m_buffer, m_size);
        delete[] m_buffer;
        m_buffer = t;
    }
    m_size += size;
}

///////////////////////////////////////////////
// Interface

CFileMem &CFileMem::operator>>(int &n)
{
    memcpy(&n, &m_buffer[m_ptr], sizeof(n));
    m_ptr += sizeof(n);
    return *this;
}

CFileMem &CFileMem::operator<<(int n)
{
    grow(sizeof(n));
    memcpy(&m_buffer[m_ptr], &n, sizeof(n));
    m_ptr += sizeof(n);
    return *this;
}

int CFileMem::read(void *buf, int size)
{
    int leftBytes = m_size - m_ptr;
    if (leftBytes >= size)
    {
        memcpy(buf, &m_buffer[m_ptr], size);
        m_ptr += size;
        return 1;
    }
    else
    {
        return 0;
    }
}

int CFileMem::write(const void *buf, int size)
{
    grow(size);
    memcpy(&m_buffer[m_ptr], buf, size);
    m_ptr += size;
    return size;
}

bool CFileMem::open(const char *fileName, const char *mode)
{
    m_mode = mode;
    m_filename = fileName;
    // TODO: fix that later
    return true;
}

void CFileMem::close()
{
    // TODO:
    m_ptr = 0;
    m_size = 0;
}

long CFileMem::getSize()
{
    return m_size;
}

void CFileMem::seek(long p)
{
    m_ptr = p;
}

long CFileMem::tell()
{
    return m_ptr;
}

CFileMem &CFileMem::operator>>(std::string &str)
{
    unsigned int x = static_cast<unsigned char>(m_buffer[m_ptr]);
    ++m_ptr;
    if (x == 0xff)
    {
        memcpy(&x, &m_buffer[m_ptr], 2);
        m_ptr += 2;
        // TODO: implement 32 bits version
        assert(x != 0xffff);
    }
    if (x != 0)
    {
        char *sz = new char[x + 1];
        sz[x] = 0;
        memcpy(sz, &m_buffer[m_ptr], x);
        m_ptr += x;
        str = sz;
        delete[] sz;
    }
    else
    {
        str = "";
    }
    return *this;
}

CFileMem &CFileMem::operator<<(const std::string &str)
{
    unsigned int x = str.length();
    if (x <= 0xfe)
    {
        grow(1);
        m_buffer[m_ptr] = static_cast<char>(x);
        ++m_ptr;
    }
    else
    {
        grow(3);
        int t = 0xff;
        m_buffer[m_ptr] = static_cast<char>(t);
        ++m_ptr;
        memcpy(&m_buffer[m_ptr], &x, 2);
        m_ptr += 2;
        // TODO : implement 32bits version
        assert(x < 0xffff);
    }
    if (x != 0)
    {
        grow(x);
        memcpy(&m_buffer[m_ptr], str.c_str(), x);
        m_ptr += x;
    }
    return *this;
}

CFileMem &CFileMem::operator>>(bool &b)
{
    memset(&b, 0, sizeof(b));
    memcpy(&b, &m_buffer[m_ptr], 1);
    ++m_ptr;
    return *this;
}

CFileMem &CFileMem::operator<<(bool b)
{
    grow(1);
    m_buffer[m_ptr] = static_cast<char>(b);
    ++m_ptr;
    return *this;
}

CFileMem &CFileMem::operator+=(const std::string &str)
{
    grow(str.length());
    memcpy(&m_buffer[m_ptr], str.c_str(), str.length());
    m_ptr += str.length();
    return *this;
}

CFileMem &CFileMem::operator+=(const char *s)
{
    grow(strlen(s));
    memcpy(&m_buffer[m_ptr], s, strlen(s));
    m_ptr += strlen(s);
    return *this;
}

const char *CFileMem::buffer()
{
    return m_buffer;
}

void CFileMem::replace(const char *buffer, int size)
{
    if (m_buffer)
    {
        delete[] m_buffer;
    }
    m_buffer = new char[size];
    memcpy(m_buffer, buffer, size);
    m_size = m_max = size;
    m_ptr = 0;
}
