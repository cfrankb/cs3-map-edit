/*
    cs3-runtime-sdl
    Copyright (C) 2024  Francois Blanchette

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
#include "animator.h"
#include "tilesdata.h"
#include "animzdata.h"
#include <cstring>
#include <vector>

CAnimator::animzSeq_t CAnimator::m_animzSeq[] = {
    {TILES_DIAMOND, ANIMZ_DIAMOND, ANIMZ_DIAMOND_LEN, 0},
    {TILES_INSECT1, ANIMZ_INSECT1_DN, ANIMZ_INSECT1_LEN, ANIMZ_INSECT1},
    {TILES_SWAMP, ANIMZ_SWAMP, ANIMZ_SWAMP_LEN, 0},
    {TILES_ALPHA, ANIMZ_ALPHA, ANIMZ_ALPHA_LEN, 0},
    {TILES_FORCEF94, ANIMZ_FORCEF94, ANIMZ_FORCEF94_LEN, 0},
    {TILES_VAMPLANT, ANIMZ_VAMPLANT, ANIMZ_VAMPLANT_LEN, 0},
    {TILES_ORB, ANIMZ_ORB, ANIMZ_ORB_LEN, 0},
    {TILES_TEDDY93, ANIMZ_TEDDY93, ANIMZ_TEDDY93_LEN, 0},
    {TILES_LUTIN, ANIMZ_LUTIN, ANIMZ_LUTIN_LEN, 0},
    {TILES_OCTOPUS, ANIMZ_OCTOPUS, ANIMZ_OCTOPUS_LEN, 0},
    {TILES_TRIFORCE, ANIMZ_TRIFORCE, ANIMZ_TRIFORCE_LEN, 0},
    {TILES_YAHOO, ANIMZ_YAHOO, ANIMZ_YAHOO_LEN, 0},
    {TILES_YIGA, ANIMZ_YIGA, ANIMZ_YIGA_LEN, 0},
    {TILES_YELKILLER, ANIMZ_YELKILLER, ANIMZ_YELKILLER_LEN, 0},
    {TILES_MANKA, ANIMZ_MANKA, ANIMZ_MANKA_LEN, 0},
    {TILES_MUSH_IDLE, ANIMZ_MUSH_DOWN, ANIMZ_MUSHROOM_LEN, ANIMZ_MUSHROOM},
};

CAnimator::CAnimator()
{
    const uint32_t seqCount = sizeof(m_animzSeq) / sizeof(animzSeq_t);
    memset(m_tileReplacement, NO_ANIMZ, sizeof(m_tileReplacement));
    m_seqIndex = new int32_t[seqCount];
    memset(m_seqIndex, 0, seqCount * sizeof(uint32_t));
}

CAnimator::~CAnimator()
{
    delete[] m_seqIndex;
}

void CAnimator::animate()
{
    const uint32_t seqCount = sizeof(m_animzSeq) / sizeof(animzSeq_t);
    for (uint32_t i = 0; i < seqCount; ++i)
    {
        const animzSeq_t &seq = m_animzSeq[i];
        int32_t &index = m_seqIndex[i];
        m_tileReplacement[seq.srcTile] = seq.startSeq + index;
        index = index < seq.count - 1 ? index + 1 : 0;
    }
    ++m_offset;
}

uint8_t CAnimator::at(uint8_t tileID)
{
    return m_tileReplacement[tileID];
}

uint8_t CAnimator::offset()
{
    return m_offset;
}

bool CAnimator::isSpecialCase(uint8_t tileID)
{
    std::vector<uint8_t> specialCases = {
        TILES_INSECT1,
        TILES_MUSH_IDLE,
    };
    for (const auto &ref : specialCases)
    {
        if (tileID == ref)
            return true;
    }
    return false;
}

AnimzInfo CAnimator::specialInfo(const int tileID)
{
    const uint32_t seqCount = sizeof(m_animzSeq) / sizeof(animzSeq_t);
    for (uint32_t i = 0; i < seqCount; ++i)
    {
        const animzSeq_t &seq = m_animzSeq[i];
        if (seq.srcTile == tileID)
        {
            return AnimzInfo{
                .frames = seq.count,
                .base = seq.specialID,
                .offset = static_cast<uint8_t>(m_offset % seq.count),
            };
        }
    }
    return AnimzInfo{};
}