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
#include "gamesfx.h"
#include "layerdata.h"
#include <cstring>
#include <vector>
#include <set>
#include <array>

constexpr uint8_t NO_SPECIAL_ID = 0;
constexpr const size_t SEQ_COUNT = 30;

const std::array<CAnimator::animzSeq_t, SEQ_COUNT> g_animzSeq = {{
    {TILES_DIAMOND, ANIMZ_DIAMOND, ANIMZ_DIAMOND_LEN, NO_SPECIAL_ID},
    {TILES_INSECT1, ANIMZ_INSECT1_DN, ANIMZ_INSECT1_LEN, ANIMZ_INSECT1},
    {TILES_SWAMP, ANIMZ_SWAMP, ANIMZ_SWAMP_LEN, NO_SPECIAL_ID},
    {TILES_FORCEF94, ANIMZ_FORCEF94, ANIMZ_FORCEF94_LEN, NO_SPECIAL_ID},
    {TILES_VAMPLANT, ANIMZ_VAMPLANT, ANIMZ_VAMPLANT_LEN, NO_SPECIAL_ID},
    {TILES_ORB, ANIMZ_ORB, ANIMZ_ORB_LEN, NO_SPECIAL_ID},
    {TILES_TEDDY93, ANIMZ_TEDDY93, ANIMZ_TEDDY93_LEN, NO_SPECIAL_ID},
    {TILES_LUTIN, ANIMZ_LUTIN, ANIMZ_LUTIN_LEN, NO_SPECIAL_ID},
    {TILES_OCTOPUS, ANIMZ_OCTOPUS, ANIMZ_OCTOPUS_LEN, NO_SPECIAL_ID},
    {TILES_TRIFORCE, ANIMZ_TRIFORCE, ANIMZ_TRIFORCE_LEN, NO_SPECIAL_ID},
    {TILES_YAHOO, ANIMZ_YAHOO, ANIMZ_YAHOO_LEN, NO_SPECIAL_ID},
    {TILES_YIGA, ANIMZ_YIGA, ANIMZ_YIGA_LEN, NO_SPECIAL_ID},
    {TILES_YELKILLER, ANIMZ_YELKILLER, ANIMZ_YELKILLER_LEN, NO_SPECIAL_ID},
    {TILES_MANKA, ANIMZ_MANKA, ANIMZ_MANKA_LEN, NO_SPECIAL_ID},
    {TILES_MUSH_IDLE, ANIMZ_MUSH_DOWN, ANIMZ_MUSHROOM_LEN, ANIMZ_MUSHROOM},
    {TILES_WHTEWORM, ANIMZ_WHTEWORM, ANIMZ_WHTEWORM_LEN / 2, ANIMZ_WHTEWORM},
    {TILES_ETURTLE, ANIMZ_ETURTLE, ANIMZ_ETURTLE_LEN / 2, ANIMZ_ETURTLE},
    {TILES_DRAGO, ANIMZ_DRAGO, ANIMZ_DRAGO_LEN / 2, ANIMZ_DRAGO},
    {TILES_DOORS_LEAF, ANIMZ_DOORS_LEAF, ANIMZ_DOORS_LEAF_LEN, NO_SPECIAL_ID},
    {TILES_FIREBALL, ANIMZ_FIREBALL, ANIMZ_FIREBALL_LEN, NO_SPECIAL_ID},
    {TILES_FIRE, ANIMZ_FIRE, ANIMZ_FIRE_LEN, NO_SPECIAL_ID},
    {TILES_BULLETY1, ANIMZ_BULLETY1, ANIMZ_BULLETY1_LEN, NO_SPECIAL_ID},
    {TILES_FLAME, ANIMZ_FLAME, ANIMZ_FLAME_LEN, NO_SPECIAL_ID},
    {SFX_SPARKLE, ANIMZ_SPARKLE, ANIMZ_SPARKLE_LEN, ANIMZ_SPARKLE},             // secret passages
    {SFX_EXPLOSION1, ANIMZ_EXPLOSION1, ANIMZ_EXPLOSION1_LEN, ANIMZ_EXPLOSION1}, // fireball explosion
    {SFX_EXPLOSION5, ANIMZ_EXPLOSION5, ANIMZ_EXPLOSION5_LEN, ANIMZ_EXPLOSION5}, // barrel explosion
    {SFX_EXPLOSION6, ANIMZ_EXPLOSION6, ANIMZ_EXPLOSION6_LEN, ANIMZ_EXPLOSION6}, // icecube melting
    {SFX_EXPLOSION7, ANIMZ_EXPLOSION7, ANIMZ_EXPLOSION7_LEN, ANIMZ_EXPLOSION7}, // thunderbolt destruction (yellow)
    {SFX_EXPLOSION0, ANIMZ_EXPLOSION0, ANIMZ_EXPLOSION0_LEN, ANIMZ_EXPLOSION0}, // placeholder for mob death
    {SFX_FLAME, ANIMZ_FLAME, ANIMZ_FLAME_LEN, ANIMZ_FLAME},                     // barrel flame
}};

const std::set<uint8_t> g_specialCases = {
    TILES_INSECT1,
    TILES_MUSH_IDLE,
    TILES_DRAGO,
    TILES_ETURTLE,
    TILES_WHTEWORM,
};

CAnimator::CAnimator() : m_seqIndex(g_animzSeq.size(), 0)
{
    memset(m_tileMainLayer, NO_ANIMZ, sizeof(m_tileMainLayer));
    memset(m_tileLayer, '\0', sizeof(m_tileLayer));

    uint8_t i = 0;
    for (auto &data : g_layerdata)
    {
        // if animated (save current index)
        if (data.nextTile)
            m_tileLayer[i] = i;
        ++i;
    }

    //    std::fill(m_tileMainLayer.begin(), m_tileReplacement.end(), NO_ANIMZ);
    for (const auto &seq : g_animzSeq)
    {
        m_seqLookUp[seq.srcTile] = animzInfo_t{
            .frames = seq.count,
            .base = seq.specialID,
            .offset = 0,
        };
    }
}

CAnimator::~CAnimator()
{
}

void CAnimator::animate()
{
    int i = 0;
    for (const auto &seq : g_animzSeq)
    {
        int32_t &index = m_seqIndex[i];
        m_tileMainLayer[seq.srcTile] = seq.startSeq + index;
        index = index < seq.count - 1 ? index + 1 : 0;
        ++i;
    }
    ++m_offset;

    for (auto &tileID : m_tileLayer)
    {
        if (!tileID)
            continue;
        const auto &data = g_layerdata[tileID];
        if (data.nextTile && data.animeSpeed && m_offset % data.animeSpeed == 0)
            tileID = data.nextTile;
    }
}

uint16_t CAnimator::offset() const
{
    return m_offset;
}

bool CAnimator::isSpecialCase(uint8_t tileID) const
{
    return g_specialCases.find(tileID) != g_specialCases.end();
}

animzInfo_t CAnimator::getSpecialInfo(const int tileID) const
{
    const auto &it = m_seqLookUp.find(tileID);
    if (it != m_seqLookUp.end())
    {
        return animzInfo_t{
            .frames = it->second.frames,
            .base = it->second.base,
            .offset = static_cast<uint8_t>(m_offset % it->second.frames),
        };
    }
    return animzInfo_t{};
}