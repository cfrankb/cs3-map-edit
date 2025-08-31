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
#include "strhelper.h"
#include <cstring>

/**
 * @brief Clean up a line of text
 *
 * @param p line cleaned up
 * @return char* ptr to next line
 */
char *processLine(char *&p)
{
    char *en = strstr(p, "\n");
    if (en)
    {
        *en = 0;
    }
    char *er = strstr(p, "\r");
    if (er)
    {
        *er = 0;
    }
    char *e = er > en ? er : en;
    while (*p == ' ' || *p == '\t')
    {
        ++p;
    }
    char *c = strstr(p, "#");
    if (c)
    {
        *c = '\0';
    }
    int i = strlen(p) - 1;
    while (i >= 0 && (p[i] == ' ' || p[i] == '\t'))
    {
        p[i] = '\0';
        --i;
    }
    return e ? e + 1 : nullptr;
}

/**
 * @brief Split a string into multiple strings
 *
 * @param str
 * @param list
 */

void splitString2(const std::string &str, std::vector<std::string> &list)
{
    unsigned int j = 0;
    bool inQuote = false;
    std::string item;
    while (j < str.length())
    {
        if (str[j] == '"')
        {
            inQuote = !inQuote;
        }
        else if (!isspace(str[j]) || inQuote)
        {
            item += str[j];
        }
        if (!inQuote && isspace(str[j]))
        {
            list.emplace_back(item);
            while (isspace(str[j]) && j < str.length())
            {
                ++j;
            }
            item.clear();
            continue;
        }
        ++j;
    }
    if (item.size())
    {
        list.emplace_back(item);
    }
}

/**
 * @brief parse string into uint16_t. diffentiate between decimal and hex notations
 *
 * @param s
 * @param isValid
 * @return uint16_t
 */
uint16_t parseStringToUShort(const std::string &s, bool &isValid)
{
    uint16_t v = 0;
    isValid = false;
    if (s.substr(0, 2) == "0x" ||
        s.substr(0, 2) == "0X")
    {
        v = std::stoul(s.substr(2), 0, 16);
        isValid = true;
    }
    else if (isdigit(s[0]) || s[0] == '-')
    {
        v = std::stoul(s, 0, 10);
        isValid = true;
    }
    return v;
}

/**
 * @brief remove preceding and trailing spaces
 *
 * @param s
 * @return std::string
 */
std::string trimString(const std::string &s)
{
    size_t i;
    size_t j;
    for (i = 0; i < s.size(); ++i)
    {
        if (!isspace(s[i]))
            break;
    }
    for (j = s.size() - 1; j > 0; --j)
    {
        if (!isspace(s[j]))
            break;
    }
    return s.substr(i, j + 1 - i);
}

bool endswith(const char *str, const char *end)
{
    const char *t = strstr(str, end);
    return t && strcmp(t, end) == 0;
}