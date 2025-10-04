/*
    LGCK Builder Runtime
    Copyright (C) 1999, 2011, 2025  Francois Blanchette

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

#include <vector>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <memory>
#include "IFile.h"

class CFileMem;

class CFileWrap : public IFile
{
public:
    CFileWrap();
    ~CFileWrap() override;

    bool operator>>(std::string &str) override;
    // Writes a string with a length prefix (1 or 3 bytes) for structured serialization.
    bool operator<<(const std::string_view &str) override;
    bool operator<<(const char *s) override;
    bool operator+=(const std::string_view &str) override;

    bool operator>>(int &n) override;
    bool operator<<(const int n) override;

    bool operator>>(bool &b) override;
    bool operator<<(const bool b) override;
    // Appends a string without a length prefix for raw text output.
    bool operator+=(const char *) override;

    bool open(const std::string_view &filename, const std::string_view &mode = "rb") override;
    int read(void *buf, const int size) override;
    int write(const void *buf, int size) override;

    bool close() override;
    long getSize() override;
    bool seek(const long i) override;
    long tell() override;
    bool flush() override;
    const std::string_view mode() override;

    static void addFile(const std::string_view &fileName, const std::vector<uint8_t> &data);
    static void freeFiles();

protected:
    std::string m_mode;
    FILE *m_file;
    static std::unordered_map<std::string, std::unique_ptr<CFileMem>> m_files;

    CFileMem *m_memFile;
    CFileMem *findFile(const std::string_view &fileName);
};
