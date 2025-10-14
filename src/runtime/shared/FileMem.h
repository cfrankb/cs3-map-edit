/*
    LGCK Builder Runtime
    Copyright (C) 1999, 2016, 2025  Francois Blanchette

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

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include "IFile.h"

class CFileMem : public IFile
{
public:
    CFileMem();
    ~CFileMem();

    bool operator>>(std::string &str) override;
    bool operator<<(const std::string_view &str) override;
    bool operator<<(const char *s) override;
    bool operator+=(const std::string_view &str) override;

    bool operator>>(int &n) override;
    bool operator<<(int n) override;

    bool operator>>(bool &b) override;
    bool operator<<(const bool b) override;
    bool operator+=(const char *) override;

    bool open(const std::string_view &filename = "", const std::string_view &mode = "rb") override;
    int read(void *buf, int size) override;
    int write(const void *buf, int size) override;

    bool close() override;
    long getSize() override;
    bool seek(long i) override;
    long tell() override;
    const std::string_view mode() override;

    const std::vector<uint8_t> &buffer();
    void replace(const uint8_t *buffer, size_t size);
    bool flush() override;

private:
    void append(const void *data, int size);
    std::string m_filename;
    std::vector<uint8_t> m_buffer;
    size_t m_ptr;
    std::string m_mode;
};
