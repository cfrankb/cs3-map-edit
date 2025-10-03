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
#pragma once
#include <string>
#include <list>
#include <vector>
#include <list>
#include <cstdint>
const char *toUpper(char *s);
std::string getUUID();
bool copyFile(const std::string in, const std::string out, std::string &errMsg);
bool concat(const std::list<std::string> files, std::string out, std::string &msg);
int upperClean(int c);
int compressData(unsigned char *in_data, unsigned long in_size, unsigned char **out_data, unsigned long &out_size);
int compressData(const std::vector<uint8_t> &in_data, std::vector<uint8_t> &out_data);
std::vector<uint8_t> readFile(const char *fname);