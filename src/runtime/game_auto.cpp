#include "game_auto.h"
#include "game.h"
#include "shared/IFile.h"
#include "logger.h"

bool automator_t::read(IFile &sfile)
{
    auto readfile = [&sfile](auto ptr, auto size) -> bool
    {
        return sfile.read(ptr, size) == IFILE_OK;
    };

    return readfile(&tileID, sizeof(tileID)) &&
           readfile(&x, sizeof(x)) &&
           readfile(&y, sizeof(y)) &&
           readfile(&layerID, sizeof(layerID)) &&
           readfile(&ttl, sizeof(ttl)) &&
           readfile(&attr, sizeof(attr)) &&
           readfile(&active, sizeof(active));
}

bool automator_t::write(IFile &tfile) const
{
    auto writefile = [&tfile](auto ptr, auto size) -> bool
    {
        return tfile.write(ptr, size) == IFILE_OK;
    };

    return writefile(&tileID, sizeof(tileID)) &&
           writefile(&x, sizeof(x)) &&
           writefile(&y, sizeof(y)) &&
           writefile(&layerID, sizeof(layerID)) &&
           writefile(&ttl, sizeof(ttl)) &&
           writefile(&attr, sizeof(attr)) &&
           writefile(&active, sizeof(active));
}

bool spawn_t::read(IFile &sfile)
{
    auto readfile = [&sfile](auto ptr, auto size) -> bool
    {
        return sfile.read(ptr, size) == IFILE_OK;
    };

    return readfile(&tileID, sizeof(tileID)) &&
           readfile(&x, sizeof(x)) &&
           readfile(&y, sizeof(y)) &&
           readfile(&delay, sizeof(delay)) &&
           readfile(&attr, sizeof(attr)) &&
           readfile(&active, sizeof(active));
}

bool spawn_t::write(IFile &tfile) const
{
    auto writefile = [&tfile](auto ptr, auto size) -> bool
    {
        return tfile.write(ptr, size) == IFILE_OK;
    };

    return writefile(&tileID, sizeof(tileID)) &&
           writefile(&x, sizeof(x)) &&
           writefile(&y, sizeof(y)) &&
           writefile(&delay, sizeof(delay)) &&
           writefile(&attr, sizeof(attr)) &&
           writefile(&active, sizeof(active));
}

void spawn_t::debug() const
{
    LOGI("spawn_t >>> tileID: %.4x x: %d y: %d delay: %d attr: %.4x active: %d",
         tileID, x, y, delay, attr, active);
}