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
    ~CFileWrap() override;

    CFileWrap & operator >> (std::string & str) override;
    CFileWrap & operator << (const std::string & str) override;
    CFileWrap & operator += (const std::string & str) override;

    CFileWrap & operator >> (int & n) override;
    CFileWrap & operator << (int n) override;

    CFileWrap & operator >> (bool & b) override;
    CFileWrap & operator << (bool b) override;
    CFileWrap & operator += (const char *) override;

    bool open(const char *filename, const char *mode= "rb") override;
    int read(void *buf, int size) override;
    int write(const void *buf, int size) override;
    static void addFile(const char *fileName, const char *data, const int size);
    static void freeFiles();

    void close() override;
    long getSize() override;
    void seek(long i) override;
    long tell() override;

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
