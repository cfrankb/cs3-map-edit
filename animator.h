#ifndef CANIMATOR_H
#define CANIMATOR_H

#include <inttypes.h>

class CAnimator
{
public:
    CAnimator();
    ~CAnimator();
    void animate();
    uint8_t at(uint8_t tileID);

protected:
    typedef struct
    {
        uint8_t srcTile;
        uint8_t startSeq;
        uint8_t count;
    } animzSeq_t;

    enum:uint32_t {
        NO_ANIMZ = 255
    };

    static animzSeq_t m_animzSeq[];
    uint8_t m_tileReplacement[256];
    int32_t *m_seqIndex = nullptr;
};

#endif // CANIMATOR_H
