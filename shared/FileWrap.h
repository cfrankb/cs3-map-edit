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

#ifndef FILEWRAP_H
#define FILEWRAP_H

#include <string>
#include "IFile.h"

class CFileWrap: public IFile
{
public:

    CFileWrap();
    virtual ~CFileWrap();

    virtual CFileWrap & operator >> (std::string & str);
    virtual CFileWrap & operator << (const std::string & str);
    virtual CFileWrap & operator += (const std::string & str);

    virtual CFileWrap & operator >> (int & n);
    virtual CFileWrap & operator << (int n);

    virtual CFileWrap & operator >> (bool & b);
    virtual CFileWrap & operator << (bool b);
    virtual CFileWrap & operator += (const char *);

    virtual bool open(const char *filename, const char *mode= "rb");
    virtual int read(void *buf, int size);
    virtual int write(const void *buf, int size);
    static void addFile(const char *fileName, const char *data, const int size);
    static void freeFiles();

    virtual void close();
    virtual long getSize();
    virtual void seek(long i);
    virtual long tell();

protected:

    FILE * m_file;

    typedef struct {
        char *fileName;
        unsigned char *data;
        int size;
        void *next;
        int ptr;
    } MEMFILE;

    MEMFILE *m_memFile;

    static MEMFILE *m_head;
    static MEMFILE *m_tail;

    MEMFILE * findFile(const char *fileName);
};

#endif // FILEWRAP_H
