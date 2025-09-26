/*
    LGCK Builder Runtime
    Copyright (C) 1999, 2020  Francois Blanchette

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
#include <cstring>
#include <ctype.h>
#include <cstdio>
#include <zlib.h>
#include "helper.h"
#ifdef USE_QFILE
#define FILEWRAP QFileWrap
#include "../shared/qtgui/qfilewrap.h"
#else
#define FILEWRAP CFileWrap
#include "../shared/FileWrap.h"
#endif

#define UUID_BUFFER_SIZE 40

const char *toUpper(char *s)
{
    for (unsigned int i = 0; i < strlen(s); ++i)
    {
        if (isalpha(s[i]))
        {
            s[i] = toupper(s[i]);
        }
    }
    return s;
}

int upperClean(int c)
{
    return isalnum(c) ? ::toupper(c) : '_';
}

std::string getUUID()
{
    char uuid[UUID_BUFFER_SIZE];
    snprintf(uuid, UUID_BUFFER_SIZE, "%.4x%.4x-%.4x-%.4x-%.4x-%.4x%.4x%.4x",
             rand() & 0xffff,
             rand() & 0xffff,
             rand() & 0xffff,
             rand() & 0xffff,
             rand() & 0xffff,
             rand() & 0xffff,
             rand() & 0xffff,
             rand() & 0xffff);
    return std::string(uuid);
}

bool copyFile(const std::string in, const std::string out, std::string &errMsg)
{
    bool result = true;
    FILEWRAP sfile;
    FILEWRAP tfile;
    if (sfile.open(in.c_str()))
    {
        int size = sfile.getSize();
        char *buf = new char[size];
        sfile.read(buf, size);
        sfile.close();
        if (tfile.open(out.c_str(), "wb"))
        {
            tfile.write(buf, size);
            tfile.close();
        }
        else
        {
            const int bufferSize = out.length() + 128;
            char *tmp = new char[bufferSize];
            snprintf(tmp, bufferSize, "couldn't write: %s", out.c_str());
            errMsg = tmp;
            result = false;
            delete[] tmp;
        }
        delete[] buf;
    }
    else
    {
        const int bufferSize = in.length() + 128;
        char *tmp = new char[bufferSize];
        snprintf(tmp, bufferSize, "couldn't read: %s", in.c_str());
        errMsg = tmp;
        result = false;
        delete[] tmp;
    }
    return result;
}

bool concat(const std::list<std::string> files, std::string out, std::string &msg)
{
    FILEWRAP tfile;
    bool result = true;
    if (tfile.open(out.c_str(), "wb"))
    {
        for (std::list<std::string>::const_iterator iterator = files.begin(), end = files.end(); iterator != end; ++iterator)
        {
            FILEWRAP sfile;
            std::string in = *iterator;
            if (sfile.open(in.c_str()))
            {
                int size = sfile.getSize();
                char *buf = new char[size];
                sfile.read(buf, size);
                sfile.close();
                tfile.write(buf, size);
                delete[] buf;
            }
            else
            {
                const int bufferSize = in.length() + 128;
                char *tmp = new char[bufferSize];
                snprintf(tmp, bufferSize, "couldn't read: %s", in.c_str());
                msg = tmp;
                result = false;
                delete[] tmp;
                break;
            }
        }
        tfile.close();
    }
    else
    {
        const int bufferSize = out.length() + 128;
        char *tmp = new char[bufferSize];
        snprintf(tmp, bufferSize, "couldn't write: %s", out.c_str());
        msg = tmp;
        result = false;
        delete[] tmp;
    }
    return result;
}

int compressData(unsigned char *in_data, unsigned long in_size, unsigned char **out_data, unsigned long &out_size)
{
    out_size = ::compressBound(in_size);
    *out_data = new unsigned char[out_size];
    return ::compress2(
        *out_data,
        &out_size,
        in_data,
        in_size,
        Z_DEFAULT_COMPRESSION);
}
