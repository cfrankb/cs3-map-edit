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
#include "gameui.h"
#include <algorithm>

CGameUI::CGameUI()
{
    m_height = m_width = 0;
    m_margin = DEFAULT_MARGIN;
}

CGameUI::~CGameUI()
{
}

const button_t &CGameUI::addButton(const button_t &button)
{
    m_buttons.emplace_back(button);
    m_height = std::max(m_height, button.y + button.height);
    m_width = std::max(m_width, button.x + button.width);
    return m_buttons.back();
}

void CGameUI::clear()
{
    m_buttons.clear();
}

size_t CGameUI::size() const
{
    return m_buttons.size();
}

button_t &CGameUI::at(int i)
{
    return m_buttons[i];
}

void CGameUI::show()
{
    m_show = true;
}

void CGameUI::hide()
{
    m_show = false;
}

bool CGameUI::isVisible() const
{
    return m_show;
}

const std::vector<button_t> &CGameUI::buttons()
{
    return m_buttons;
}

int CGameUI::width() const
{
    return m_width;
}

int CGameUI::height() const
{
    return m_height;
}

int CGameUI::margin() const
{
    return m_margin;
}

CGameUI &CGameUI::setMargin(int margin)
{
    m_margin = margin;
    return *this;
}
