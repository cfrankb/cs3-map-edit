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

#pragma once

#define IFILE_OK 1
#define IFILE_NOT_OK 0

#include <string>
#include <string_view>

class IFile
{
public:
    virtual ~IFile() {};

    virtual bool operator>>(std::string &str) = 0;
    virtual bool operator<<(const std::string_view &str) = 0;
    virtual bool operator<<(const char *s) = 0;
    virtual bool operator+=(const std::string_view &str) = 0;

    virtual bool operator>>(int &n) = 0;
    virtual bool operator<<(const int n) = 0;

    virtual bool operator>>(bool &b) = 0;
    virtual bool operator<<(const bool b) = 0;
    virtual bool operator+=(const char *) = 0;

    virtual bool open(const std::string_view &filename, const std::string_view &mode = "rb") = 0;
    virtual int read(void *buf, const int size) = 0;
    virtual int write(const void *buf, const int size) = 0;

    virtual bool close() = 0;
    virtual long getSize() = 0;
    virtual bool seek(const long i) = 0;
    virtual long tell() = 0;
    virtual bool flush() = 0;
    virtual const std::string_view mode() = 0;
};
