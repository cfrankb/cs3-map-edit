#include "animator.h"

#include "tilesdata.h"
#include "animzdata.h"
#include <cstring>

CAnimator::animzSeq_t CAnimator::m_animzSeq[] = {
    {TILES_DIAMOND, ANIMZ_DIAMOND, 13, 0},
    {TILES_INSECT1, ANIMZ_INSECT1, 2, 0},
    {TILES_SWAMP, ANIMZ_SWAMP, 2, 0},
    {TILES_ALPHA, ANIMZ_ALPHA, 2, 0},
    {TILES_FORCEF94, ANIMZ_FORCEF94, 8, 0},
    {TILES_VAMPLANT, ANIMZ_VAMPLANT, 2, 0},
    {TILES_ORB, ANIMZ_ORB, 4, 0},
    {TILES_TEDDY93, ANIMZ_TEDDY93, 2, 0},
    {TILES_LUTIN, ANIMZ_LUTIN, 2, 0},
    {TILES_OCTOPUS, ANIMZ_OCTOPUS, 2, 0},
    {TILES_TRIFORCE, ANIMZ_TRIFORCE, 4, 0},
    {TILES_YAHOO, ANIMZ_YAHOO, 2, 0},
    {TILES_YIGA, ANIMZ_YIGA, 2, 0},
    {TILES_YELKILLER, ANIMZ_YELKILLER, 2, 0},
    {TILES_MANKA, ANIMZ_MANKA, 2, 0},
    // {TILES_MAXKILLER, ANIMZ_MAXKILLER, 1, 0},
    // {TILES_WHTEWORM, ANIMZ_WHTEWORM, 2, 0},
    };

CAnimator::CAnimator()
{
    memset(m_tileReplacement, NO_ANIMZ, sizeof(m_tileReplacement));
}

void CAnimator::animate()
{
    for (uint32_t i = 0; i < sizeof(m_animzSeq) / sizeof(animzSeq_t); ++i)
    {
        animzSeq_t &seq = m_animzSeq[i];
        int j = seq.srcTile;
        m_tileReplacement[j] = seq.startSeq + seq.index;
        seq.index = seq.index < seq.count - 1 ? seq.index + 1 : 0;
    }
}

uint8_t CAnimator::at(uint8_t tileID)
{
    return m_tileReplacement[tileID];
}
