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
#include "statedata.h"

#define DEF(x) {#x, x}

const std::vector<KeyOption> g_keyOptions = {
    DEF(TIMEOUT),
    DEF(POS_ORIGIN),
    DEF(POS_EXIT),
    DEF(MAP_GOAL),
    DEF(MSG0),
    DEF(MSG1),
    DEF(MSG2),
    DEF(MSG3),
    DEF(MSG4),
    DEF(MSG5),
    DEF(MSG6),
    DEF(MSG7),
    DEF(MSG8),
    DEF(MSG9),
    DEF(MSGA),
    DEF(MSGB),
    DEF(MSGC),
    DEF(MSGD),
    DEF(MSGE),
    DEF(MSGF),
    DEF(USERDEF1),
    DEF(USERDEF2),
    DEF(USERDEF3),
    DEF(USERDEF4),
};

const std::vector<KeyOption> &getKeyOptions()
{
    return g_keyOptions;
}
