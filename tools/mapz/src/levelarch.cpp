#include "levelarch.h"
#include <stdio.h>
#include <cstring>

CLevelArch::CLevelArch()
{

}

CLevelArch::~CLevelArch()
{

}

bool CLevelArch::open(const char *filename)
{
    typedef struct {
        uint8_t sig[4];
        uint16_t version;
        uint16_t count;  
        uint32_t offset;
    } Header;

    FILE *sfile = fopen(filename, "rb");
    if (sfile) {
        Header hdr;
        fread(&hdr, 12, 1, sfile);

        fseek(sfile, hdr.offset, SEEK_SET);
        uint32_t *indexPtr = new uint32_t[hdr.count];
        fread(indexPtr, 4 * hdr.count,1, sfile);
        m_index.clear();
        for (int i=0; i < hdr.count; ++i) {
            m_index.push_back(indexPtr[i]);
        }
        delete []indexPtr;
        fclose(sfile);
    }
    return sfile != nullptr;
}

bool CLevelArch::fromMemory(uint8_t *ptr)
{ 
    // TODO: check signature / version   
    m_index.clear();
    uint16_t count = 0;
    memcpy(&count, ptr + 6, sizeof(count));
    uint32_t indexBase = 0;
    memcpy(&indexBase, ptr + 8, sizeof(indexBase));
    for (uint16_t i=0; i < count; ++i) {
        long idx = 0;
        memcpy(&idx, ptr + indexBase + i * 4, 4);
        m_index.push_back(idx);
    }
    return true;
}

int CLevelArch::count() {
    return m_index.size();
}

uint32_t CLevelArch::at(int i) {
    return m_index[i];
}

void CLevelArch::debug()
{
    printf("count: %d\n", count());
    for (int i=0; i < m_index.size(); ++i) {
        printf("%d at offset 0x%.8x\n", i, at(i));
    }
}

void CLevelArch::forget() {
    m_index.clear();
}