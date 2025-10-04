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
    ~QFileWrap() override;

    bool  operator >> (std::string & str) override;
    bool  operator << (const std::string_view &str) override;
     bool operator <<(const char *s) override;
    bool  operator += (const std::string_view &str) override;

    bool  operator >> (int & n) override;
    bool  operator << (int n) override;

    bool  operator >> (bool & b) override;
    bool  operator << (bool b) override;
    bool  operator += (const char *) override;
    bool  operator += (const QString & str);

    bool open(const std::string_view &filename, const std::string_view &mode = "rb") override;
    bool open(const QString &filename, const char *mode= "rb");
    int read(void *buf, int size) override;
    int write(const void *buf, int size) override;

    bool close() override;
    long getSize() override;
    bool seek(long i) override;
    long tell() override;
     bool flush() override;
     const std::string_view mode() override;

protected:
    std::string m_mode;
    QFile * m_file;
};

#endif // QFILEWRAP_H
