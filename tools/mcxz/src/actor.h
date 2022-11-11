#ifndef __ACTOR__H
#define __ACTOR__H
#include <stdint.h>
class CActor
{

public:
    CActor(uint8_t x = 0, uint8_t y = 0, uint8_t type = 0, uint8_t dir = 0);
    ~CActor();

protected:
    uint8_t m_x;
    uint8_t m_y;
    uint8_t m_type;
    uint8_t m_dir;
};

#endif