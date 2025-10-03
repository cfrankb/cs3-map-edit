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
#include "colormap.h"
#include "shared/IFile.h"
#include "strhelper.h"
#include <cstring>
#include "logger.h"

bool parseListKV(std::vector<std::string> &list, uint32_t &k, uint32_t &v, int line)
{
    if (list.size() != 2)
    {
        LOGE("Expected 2 items, found %zu on line %d\n", list.size(), line);
        return false;
    }
    try
    {
        if (list[0].substr(0, 2) != "0x" || list[1].substr(0, 2) != "0x")
        {
            LOGE("Invalid hex format on line %d\n", line);
            return false;
        }
        k = std::stoul(list[0].substr(2), nullptr, 16);
        v = std::stoul(list[1].substr(2), nullptr, 16);
    }
    catch (const std::exception &e)
    {
        LOGE("Failed to parse hex values on line %d: %s\n", line, e.what());
        return false;
    }
    return true;
}

bool parseColorMaps(IFile &file, ColorMaps &colorMaps)
{
    size_t size = file.getSize();
    std::vector<char> buffer(size + 1);
    if (file.read(buffer.data(), size) != IFILE_OK)
    {
        return false;
    }
    buffer[size] = '\0';
    return parseColorMaps(buffer.data(), colorMaps);
}

void clearColorMaps(ColorMaps &colorMaps)
{
    // clear maps
    colorMaps.godMode.clear();
    colorMaps.rage.clear();
    colorMaps.sugarRush.clear();
}

bool parseColorMaps(char *tmp, ColorMaps &colorMaps)
{
    if (!tmp)
    {
        LOGE("Null input buffer\n");
        return false;
    }
    clearColorMaps(colorMaps);
    std::string input(tmp);
    size_t pos = 0;
    std::string section;
    int line = 1;
    while (pos < input.size())
    {
        std::string current = processLine(input, pos);
        if (current.empty())
        {
            ++line;
            continue;
        }
        if (current[0] == '[')
        {
            size_t end = current.find(']');
            if (end == std::string::npos)
            {
                LOGE("Missing section terminator on line %d\n", line);
                return false;
            }
            section = current.substr(1, end - 1);
            if (section.empty() || (section != "sugarrush" && section != "godmode" && section != "rage"))
            {
                LOGE("Invalid section '%s' on line %d\n", section.c_str(), line);
                return false;
            }
        }
        else
        {
            std::vector<std::string> list;
            splitString2(current, list);
            uint32_t k, v;
            if (!parseListKV(list, k, v, line))
            {
                return false;
            }
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
                LOGE("No section defined for key-value pair on line %d\n", line);
                return false;
            }
        }
        ++line;
    }
    return !colorMaps.sugarRush.empty() || !colorMaps.godMode.empty() || !colorMaps.rage.empty();
}
