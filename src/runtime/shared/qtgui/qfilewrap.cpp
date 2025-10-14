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
    QFileWrap::close();
}

/////////////////////////////////////////////////////////////////////
// QT implmentation

int QFileWrap::read(void *buf, int size)
{
    return m_file->read( (char*) buf, size) == size ? 1 : 0;
}

int QFileWrap::write(const void *buf, int size)
{
    return m_file->write( (char*) buf, size) == size ? 1 : 0;
}

bool  QFileWrap::operator >> (int & n)
{
    return read(&n, 4) == IFILE_OK;

}

bool  QFileWrap::operator << ( int n)
{
    return write(&n, 4) == IFILE_OK;
}

bool  QFileWrap::operator >> (bool & b)
{
    memset(&b, 0, sizeof(b));
    return read(&b, 1) == IFILE_OK;
}

bool  QFileWrap::operator << ( bool b)
{
    return write(&b, 1) == IFILE_OK;
}

bool  QFileWrap::operator >> ( std::string & str)
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

    return true;
}

bool  QFileWrap::operator << (const std::string_view &str)
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
        write(str.data(), x);
    }

    return true;
}

bool  QFileWrap::operator += (const std::string_view &str)
{
    return write(str.data(), str.length()) == IFILE_OK;
    //return *this;
}

bool  QFileWrap::operator += (const char *s)
{
    return write(s, strlen(s));
}

bool  QFileWrap::operator += (const QString & str)
{
    return m_file->write(str.toUtf8(), str.size());
    //return *this;
}

bool QFileWrap::open(const std::string_view &filename, const std::string_view &mode )
{
    return open(filename.data(), mode.data());
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

bool QFileWrap::close()
{
    if (m_file) {
        if ( m_file->openMode() != QIODevice::NotOpen) {
            m_file->close();
        }
        delete m_file;
        m_file = NULL;
    }
    return true;
}

long QFileWrap::getSize()
{
    return m_file->size();
}

bool QFileWrap::seek(long p)
{
    m_file->seek(p);
    return true;
}

long QFileWrap::tell()
{
    return m_file->pos();
}

bool QFileWrap::flush() {
    return true;
}
const std::string_view QFileWrap::mode() {
    return m_mode;
}

bool QFileWrap::operator<<(const char *s)
{
    std::string_view sv(s);
    return *this << sv; // Delegate to string_view overload
}

