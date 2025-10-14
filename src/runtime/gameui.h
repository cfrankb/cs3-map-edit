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
#pragma once

#include <vector>
#include <cstdint>
#include <string>

struct button_t
{
    int id;
    int x;
    int y;
    int width;
    int height;
    std::string text;
    uint32_t color;
};

class CGameUI
{
public:
    CGameUI();
    ~CGameUI();
    const button_t &addButton(const button_t &button);
    void clear();
    size_t size() const;
    button_t &at(int i);
    void show();
    void hide();
    bool isVisible() const;
    const std::vector<button_t> &buttons();
    int width() const;
    int height() const;
    int margin() const;
    CGameUI &setMargin(int margin);

private:
    enum
    {
        DEFAULT_MARGIN = 8
    };
    bool m_show = false;
    std::vector<button_t> m_buttons;
    int m_height = 0;
    int m_width = 0;
    int m_margin;
};
