#include "actor.h"

CActor::CActor(uint8_t x, uint8_t y, uint8_t type, uint8_t dir)
{
    m_x = x;
    m_y = y;
    m_type = type;
    m_dir = dir;
}
CActor::~CActor()
{
}
