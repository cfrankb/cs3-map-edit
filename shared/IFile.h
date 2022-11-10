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

#ifndef IFILE_H
#define IFILE_H

#include <string>

class IFile
{
public:

    virtual ~IFile(){};

    virtual IFile & operator >> (std::string & str)=0;
    virtual IFile & operator << (const std::string & str)=0;
    virtual IFile & operator += (const std::string & str)=0;

    virtual IFile & operator >> (int & n)=0;
    virtual IFile & operator << (int n)=0;

    virtual IFile & operator >> (bool & b)=0;
    virtual IFile & operator << (bool b)=0;
    virtual IFile & operator += (const char *)=0;

    virtual bool open(const char *filename, const char *mode= "rb")=0;
    virtual int read(void *buf, int size)=0;
    virtual int write(const void *buf, int size)=0;

    virtual void close()=0;
    virtual long getSize()=0;
    virtual void seek(long i)=0;
    virtual long tell()=0;
};

#endif // IFILE_H 
