#ifndef __LEVELARCH_H
#define __LEVELARCH_H

#include <cinttypes>
#include <vector>
typedef std::vector<long> IndexVector;

class CLevelArch 
{
public:
    CLevelArch();
    ~CLevelArch();

    bool open(const char *filename);
    bool fromMemory(uint8_t *ptr);
    int count();
    uint32_t at(int i);
    void debug();
    void forget();

protected:
   IndexVector m_index;

};

#endif