#pragma once
#include <cstdint>

class IFile;

struct automator_t
{
    uint16_t tileID;
    int16_t x;
    int16_t y;
    uint16_t layerID;
    uint16_t ttl;
    uint16_t attr;
    uint8_t active;

    bool read(IFile &sfile);
    bool write(IFile &tfile) const;
};

struct spawn_t
{
    uint16_t tileID;
    int16_t x;
    int16_t y;
    uint16_t delay;
    uint16_t attr;
    uint8_t active;

    bool read(IFile &sfile);
    bool write(IFile &tfile) const;
    void debug() const;
};
