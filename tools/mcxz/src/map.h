#ifndef __MAP_H
#define __MAP_H

#include <stdint.h>

typedef struct
{
    uint8_t x;
    uint8_t y;
} Pos;

class CMap
{
public:
    CMap(int len = 0, int hei = 0, uint8_t t = 0);
    ~CMap();
    uint8_t &at(int x, int y);
    uint8_t *row(int y);
    void set(int x, int y, uint8_t t);
    bool read(const char *fname);
    bool write(const char *fname);
    void forget();
    int len() const;
    int hei() const;
    bool resize(int len, int hei);
    const Pos findFirst(uint8_t tileId);
    int count(uint8_t tileId);
    void clear(uint8_t ch = 0);

protected:
    uint8_t m_len;
    uint8_t m_hei;
    uint8_t *m_map;
    int m_size;
};

#endif