#include "game_auto.h"
#include "game.h"
#include "shared/IFile.h"

bool automator_t::read(IFile &sfile)
{
    auto readfile = [&sfile](auto ptr, auto size) -> bool
    {
        return sfile.read(ptr, size) == 1;
    };

    return readfile(&tileID, sizeof(tileID)) &&
           readfile(&x, sizeof(x)) &&
           readfile(&y, sizeof(y)) &&
           readfile(&layerID, sizeof(layerID)) &&
           readfile(&ttl, sizeof(ttl)) &&
           readfile(&attr, sizeof(attr)) &&
           readfile(&attr, sizeof(active));
}

bool automator_t::write(IFile &tfile) const
{
    auto writefile = [&tfile](auto ptr, auto size) -> bool
    {
        return tfile.write(ptr, size) == 1;
    };

    return writefile(&tileID, sizeof(tileID)) &&
           writefile(&x, sizeof(x)) &&
           writefile(&y, sizeof(y)) &&
           writefile(&layerID, sizeof(layerID)) &&
           writefile(&ttl, sizeof(ttl)) &&
           writefile(&attr, sizeof(attr)) &&
           writefile(&attr, sizeof(active));
}