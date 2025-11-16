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
#include <cstdint>
#include <cstdio>
#include <zlib.h>
#include "helper.h"
#include "logger.h"

#if defined(USE_QFILE)
#define FILEWRAP QFileWrap
#include "qtgui/qfilewrap.h"
#else
#define FILEWRAP CFileWrap
#include "FileWrap.h"
#endif
constexpr const int UUID_BUFFER_SIZE = 40;

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
    snprintf(uuid, sizeof(uuid), "%.4x%.4x-%.4x-%.4x-%.4x-%.4x%.4x%.4x",
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
        std::vector<char> buf(size);
        sfile.read(buf.data(), size);
        sfile.close();
        if (tfile.open(out.c_str(), "wb"))
        {
            tfile.write(buf.data(), size);
            tfile.close();
        }
        else
        {
            const int bufferSize = out.length() + 128;
            std::vector<char> tmp(bufferSize);
            snprintf(tmp.data(), bufferSize, "couldn't write: %s", out.c_str());
            errMsg = tmp.data();
            result = false;
        }
    }
    else
    {
        const int bufferSize = in.length() + 128;
        std::vector<char> tmp(bufferSize);
        snprintf(tmp.data(), bufferSize, "couldn't read: %s", in.c_str());
        errMsg = tmp.data();
        result = false;
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
                std::vector<char> buf(size);
                sfile.read(buf.data(), size);
                sfile.close();
                tfile.write(buf.data(), size);
            }
            else
            {
                const int bufferSize = in.length() + 128;
                std::vector<char> tmp(bufferSize);
                snprintf(tmp.data(), bufferSize, "couldn't read: %s", in.c_str());
                msg = tmp.data();
                result = false;
                break;
            }
        }
        tfile.close();
    }
    else
    {
        const int bufferSize = out.length() + 128;
        std::vector<char> tmp(bufferSize);
        snprintf(tmp.data(), bufferSize, "couldn't write: %s", out.c_str());
        msg = tmp.data();
        result = false;
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

int compressData(const std::vector<uint8_t> &in_data, std::vector<uint8_t> &out_data)
{
    auto out_size = ::compressBound(in_data.size());
    out_data.resize(out_size);
    int result = ::compress2(
        out_data.data(),
        &out_size,
        in_data.data(),
        in_data.size(),
        Z_DEFAULT_COMPRESSION);
    if (out_size != out_data.size())
        out_data.resize(out_size);
    return result;
}

std::vector<uint8_t> readFile(const char *fname)
{
    FILEWRAP file;
    if (!file.open(fname, "rb"))
    {
        LOGE("failed to read:%s", fname);
        return {};
    }
    size_t size = file.getSize();
    std::vector<uint8_t> data(size);
    if (file.read(data.data(), size) != IFILE_OK)
    {
        LOGE("can't read data for %s", fname);
        return {};
    }
    return data;
}