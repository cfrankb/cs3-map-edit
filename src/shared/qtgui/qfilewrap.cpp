/*
    LGCK Builder Runtime
    Copyright (C) 1999, 2021  Francois Blanchette

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

#include "qtgui/qfilewrap.h"
#include <cstring>
#include <cstdio>
#include <QFile>

QFileWrap::QFileWrap()
{
    m_file = nullptr;
}

QFileWrap::~QFileWrap()
{
    close();
}

/////////////////////////////////////////////////////////////////////
// QT implmentation

int QFileWrap::read(void *buf, int size)
{
    return m_file->read( (char*) buf, size);
}

int QFileWrap::write(const void *buf, int size)
{
    return m_file->write( (char*) buf, size);
}

QFileWrap & QFileWrap::operator >> (int & n)
{
    read(&n, 4);
    return *this;
}

QFileWrap & QFileWrap::operator << ( int n)
{
    write(&n, 4);
    return *this;
}

QFileWrap & QFileWrap::operator >> (bool & b)
{
    memset(&b, 0, sizeof(b));
    read(&b, 1);
    return *this;
}

QFileWrap & QFileWrap::operator << ( bool b)
{
    write(&b, 1);
    return *this;
}

QFileWrap & QFileWrap::operator >> ( std::string & str)
{
    int x = 0;
    read (&x, 1);
    if (x == 0xff) {
        read (&x, 2);
        // TODO: implement 32 bits version
    }

    if (x != 0) {
        char *sz = new char[x + 1];
        sz [ x ] = 0;
        m_file->read (sz, x);
        str = sz;
        delete [] sz;
    } else {
        str = "";
    }

    return *this;
}

QFileWrap & QFileWrap::operator << (const std::string & str)
{
    int x = str.length();
    if (x <= 0xfe) {
        write (&x, 1);
    }
    else {
        int t = 0xff;
        write (&t, 1);
        write (&x, 2);

        // TODO : implement 32bits version
    }

    if (x!=0) {
        write(str.c_str(), x);
    }

    return *this;
}

QFileWrap & QFileWrap::operator += (const std::string & str)
{
    write(str.c_str(), str.length());
    return *this;
}

QFileWrap & QFileWrap::operator += (const char *s)
{
    write(s, strlen(s));
    return *this;
}

bool QFileWrap::open(const char* fileName, const char *mode)
{
    return open(QString(fileName), mode);
}

bool QFileWrap::open(const QString & fileName, const char *mode)
{
    QIODevice::OpenMode iomode = QIODevice::ReadOnly;
    if (strstr(mode,"w")) {
        iomode = QIODevice::WriteOnly;
    }
    if (strstr(mode,"a")) {
        iomode = QIODevice::Append;
    }
    if (strstr(mode,"+")) {
        iomode = QIODevice::ReadWrite;
    }

    if (m_file) {
        m_file->setFileName(fileName);
    } else {
        m_file = new QFile(fileName);
    }

    return m_file->open(iomode);
}

void QFileWrap::close()
{
    if (m_file) {
        if ( m_file->openMode() != QIODevice::NotOpen) {
            m_file->close();
        }
        delete m_file;
        m_file = NULL;
    }
}

long QFileWrap::getSize()
{
    return m_file->size();
}

void QFileWrap::seek(long p)
{
    m_file->seek(p);
}

long QFileWrap::tell()
{
    return m_file->pos();
}
