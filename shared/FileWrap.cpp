/*
    LGCK Builder Runtime
    Copyright (C) 1999, 2011  Francois Blanchette

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
#ifdef USE_QFILE
#include <QFile>
#endif

CFileWrap::CFileWrap()
{   
    m_file = nullptr;
}

CFileWrap::~CFileWrap()
{
    close();
}

#define _ptr m_memFile->ptr

CFileWrap::MEMFILE * CFileWrap::m_head = nullptr;
CFileWrap::MEMFILE * CFileWrap::m_tail = nullptr;

void CFileWrap::addFile(const char *fileName, const char *data, const int size)
{
    MEMFILE * mf = new MEMFILE;
    if (m_head) {
        m_tail->next = mf;
        m_tail = mf;
    } else {
        m_head = mf;
        m_tail = mf;
    }

    mf->next = nullptr;
    mf->fileName = (char*)fileName;
    mf->data = (unsigned char*)data;
    mf->size = size;
}

void CFileWrap::freeFiles()
{
    MEMFILE *p = m_head;
    while (p) {

        MEMFILE *pp = p;
        p = (MEMFILE*)p->next;
        delete pp;
    }

    m_head = nullptr;
    m_tail = nullptr;
}

CFileWrap::MEMFILE * CFileWrap::findFile(const char *fileName)
{
    MEMFILE *p = m_head;
    while (p) {
        if (!strcmp(fileName, p->fileName)) {
            return p;
        }
        p = (MEMFILE*)p->next;
    }

    return nullptr;
}

///////////////////////////////////////////////
// GCC

CFileWrap & CFileWrap::operator >> (int & n)
{
    read (&n, 4);
    return *this;
}

CFileWrap & CFileWrap::operator << ( int n)
{
    write (&n, 4);
    return *this;
}

int CFileWrap::read(void *buf, int size)
{
    if (m_memFile) {
        memcpy (buf, m_memFile->data + _ptr, size );
        _ptr += size;
        return size;
    } else {
        return fread(buf, size, 1, m_file);
    }
}

int CFileWrap::write(const void *buf, int size)
{
    return fwrite(buf, size, 1, m_file);
}

bool CFileWrap::open(const char *fileName, const char *mode)
{
    if ((m_memFile = findFile(fileName))) {
        _ptr = 0;
        return !strcmp(mode, "rb");
    } else {
        m_file = fopen(fileName, mode);
        return m_file != nullptr;
    }
}

void CFileWrap::close()
{
    if (m_file) {
        fclose(m_file);
        m_file = nullptr;
    }
}

long CFileWrap::getSize()
{
    if (m_memFile) {
        return m_memFile->size;
    } else {
        long cur = ftell(m_file);
        fseek(m_file, 0, SEEK_END);
        long w = ftell(m_file);
        fseek(m_file, cur, SEEK_SET);
        return w;
    }
}

void CFileWrap::seek(long p)
{
    if (m_memFile) {
        m_memFile->ptr = p;
    } else {
        fseek(m_file, p, SEEK_SET);
    }
}

long CFileWrap::tell()
{
    if (m_memFile) {
        return m_memFile->ptr;
    } else {
        return ftell(m_file);
    }
}

CFileWrap & CFileWrap::operator >> ( std::string & str)
{
    if (m_memFile) {
        int x = m_memFile->data[_ptr];
        ++_ptr;

        if (x == 0xff) {
            memcpy(&x, &m_memFile->data[_ptr], 2);
            _ptr += 2;
            // TODO: implement 32 bits version
        }

        if (x != 0) {
            char *sz = new char[x + 1];
            sz [ x ] = 0;
            memcpy(sz, &m_memFile[_ptr], x);
            _ptr += x;
            //file.read (sz, x);
            str = sz;
            delete [] sz;
        }
        else {
            str = "";
        }

    } else {

        int x = 0;
        fread (&x, 1, 1, m_file);
        if (x == 0xff) {
            fread (&x, 2, 1, m_file);

            // TODO: implement 32 bits version
        }

        if (x != 0) {
            char *sz = new char[x + 1];
            sz [ x ] = 0;
            fread (sz, x, 1, m_file);
            //file.read (sz, x);
            str = sz;
            delete [] sz;
        }
        else {
            str = "";
        }
    }

    return *this;

}

CFileWrap & CFileWrap::operator << (const std::string & str)
{
    int x = str.length();
    if (x <= 0xfe) {
        fwrite (&x, 1, 1, m_file);

    }
    else {
        int t = 0xff;

        fwrite (&t, 1, 1, m_file);
        fwrite (&x, 2, 1, m_file);

        // TODO : implement 32bits version
    }

    if (x!=0) {
        fwrite (str.c_str(), x, 1, m_file);
    }

    return *this;
}

CFileWrap & CFileWrap::operator >> (bool & b)
{
    memset(&b, 0, sizeof(b));
    if (m_memFile) {
        memcpy(&b, &m_memFile->data[_ptr], 1);
        ++ _ptr;
    } else {
        fread(&b, 1, 1, m_file);
    }

    return *this;
}

CFileWrap & CFileWrap::operator << ( bool b)
{
    fwrite(&b, 1, 1, m_file);
    return *this;
}

CFileWrap & CFileWrap::operator += (const std::string & str)
{
    fwrite (str.c_str(), str.length(), 1, m_file);
    return *this;
}

CFileWrap & CFileWrap::operator += (const char *s)
{
    fwrite (s, strlen(s), 1, m_file);
    return *this;
}
