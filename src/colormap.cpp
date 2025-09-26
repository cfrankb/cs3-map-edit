/*
    cs3-runtime-sdl
    Copyright (C) 2025 Francois Blanchette

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
#define LOG_TAG "colormap"
#include "colormap.h"
#include "shared/IFile.h"
#include "strhelper.h"
#include <cstring>
#include "logger.h"

bool parseListKV(std::vector<std::string> &list, uint32_t &k, uint32_t &v, const int line)
{
    if (list.size() != 2)
    {
        LOGE("list expected to be 2 items. found %zu on line %d\n", list.size(), line);
        return false;
    }
    k = std::stoul(list[0].substr(2).c_str(), nullptr, 16);
    v = std::stoul(list[1].substr(2).c_str(), nullptr, 16);
    return true;
}

bool parseColorMaps(IFile &file, ColorMaps &colorMaps)
{
    size_t size = file.getSize();
    char *tmp = new char[size + 1];
    file.read(tmp, size);
    tmp[size] = '\0';
    bool result = parseColorMaps(tmp, colorMaps);
    delete[] tmp;
    return result;
}

bool parseColorMaps(char *tmp, ColorMaps &colorMaps)
{
    // clear maps
    colorMaps.godMode.clear();
    colorMaps.rage.clear();
    colorMaps.sugarRush.clear();

    std::string section;
    char *p = tmp;
    int line = 1;
    while (p && *p)
    {
        char *next = processLine(p);
        if (p[0] == '[')
        {
            ++p;
            char *pe = strstr(p, "]");
            if (pe)
                *pe = 0;
            if (!pe)
            {
                LOGE("missing section terminator on line %d\n", line);
            }
            section = p;
        }
        else if (p[0])
        {
            std::vector<std::string> list;
            splitString2(p, list);
            uint32_t k;
            uint32_t v;
            if (parseListKV(list, k, v, line))
            {
                if (section == "sugarrush")
                {
                    colorMaps.sugarRush[k] = v;
                }
                else if (section == "godmode")
                {
                    colorMaps.godMode[k] = v;
                }
                else if (section == "rage")
                {
                    colorMaps.rage[k] = v;
                }
                else
                {
                    LOGE("item for unknown section %s on line %d\n", section.c_str(), line);
                }
            }
        }
        p = next;
        ++line;
    }
    return true;
}