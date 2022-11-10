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

#ifndef QFILEWRAP_H
#define QFILEWRAP_H

#include <string>
#include "IFile.h"

class QFile;
class QString;

class QFileWrap: public IFile
{
public:

    QFileWrap();
    virtual ~QFileWrap();

    virtual QFileWrap & operator >> (std::string & str);
    virtual QFileWrap & operator << (const std::string & str);
    virtual QFileWrap & operator += (const std::string & str);

    virtual QFileWrap & operator >> (int & n);
    virtual QFileWrap & operator << (int n);

    virtual QFileWrap & operator >> (bool & b);
    virtual QFileWrap & operator << (bool b);
    virtual QFileWrap & operator += (const char *);

    virtual bool open(const char *filename, const char *mode= "rb");
    virtual bool open(const QString &filename, const char *mode= "rb");
    virtual int read(void *buf, int size);
    virtual int write(const void *buf, int size);

    virtual void close();
    virtual long getSize();
    virtual void seek(long i);
    virtual long tell();

protected:
    QFile * m_file;
};

#endif // QFILEWRAP_H
