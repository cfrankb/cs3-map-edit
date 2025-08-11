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

CAnimator::animzSeq_t CAnimator::m_animzSeq[] = {
    {TILES_DIAMOND, ANIMZ_DIAMOND, ANIMZ_DIAMOND_LEN},
    {TILES_INSECT1, ANIMZ_INSECT1_DN, ANIMZ_INSECT1_LEN},
    {TILES_SWAMP, ANIMZ_SWAMP, ANIMZ_SWAMP_LEN},
    {TILES_ALPHA, ANIMZ_ALPHA, ANIMZ_ALPHA_LEN},
    {TILES_FORCEF94, ANIMZ_FORCEF94, ANIMZ_FORCEF94_LEN},
    {TILES_VAMPLANT, ANIMZ_VAMPLANT, ANIMZ_VAMPLANT_LEN},
    {TILES_ORB, ANIMZ_ORB, ANIMZ_ORB_LEN},
    {TILES_TEDDY93, ANIMZ_TEDDY93, ANIMZ_TEDDY93_LEN},
    {TILES_LUTIN, ANIMZ_LUTIN, ANIMZ_LUTIN_LEN},
    {TILES_OCTOPUS, ANIMZ_OCTOPUS, ANIMZ_OCTOPUS_LEN},
    {TILES_TRIFORCE, ANIMZ_TRIFORCE, ANIMZ_TRIFORCE_LEN},
    {TILES_YAHOO, ANIMZ_YAHOO, ANIMZ_YAHOO_LEN},
    {TILES_YIGA, ANIMZ_YIGA, ANIMZ_YIGA_LEN},
    {TILES_YELKILLER, ANIMZ_YELKILLER, ANIMZ_YELKILLER_LEN},
    {TILES_MANKA, ANIMZ_MANKA, ANIMZ_MANKA_LEN},
    //  {TILES_DEVIL, ANIMZ_DEVIL, ANIMZ_DEVIL_LEN},
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

int CAnimator::offset()
{
    return m_offset;
}

bool CAnimator::isSpecialCase(uint8_t tileID)
{
    return tileID == TILES_INSECT1;
}
