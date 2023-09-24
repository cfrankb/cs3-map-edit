/*
    LGCK Builder Runtime
    Copyright (C) 1999, 2014  Francois Blanchette

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cstdio>
#include "sn_sdl.h"

#ifdef USE_SDL_MIXER
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_mixer.h>

struct SND
{
    Mix_Chunk *chunk;
    int channel;
};
#endif

CSndSDL::CSndSDL()
{
#ifdef USE_SDL_MIXER
    m_valid = false;
    // Initialize all SDL subsystems
    if ( SDL_Init( SDL_INIT_EVERYTHING ) == -1 )  {
        printf("SDL_init failed: %s", SDL_GetError());
        return;
    }
    if (Mix_OpenAudio(22050,AUDIO_S16SYS,2,640)<0) {
        printf("Mix_OpenAudio failed: %s", SDL_GetError());
        m_valid = false;
    }
    m_valid = true;
#endif
}

CSndSDL::~CSndSDL()
{
#ifdef USE_SDL_MIXER
    forget();
    SDL_CloseAudio();
#endif
}

void CSndSDL::forget()
{
#ifdef USE_SDL_MIXER
    for ( auto it = m_sounds.begin(); it != m_sounds.end(); ++it ) {
        unsigned int uid = it->first;
        stop(uid);
        SND * snd = it->second;
        // free chunk
        Mix_FreeChunk(snd->chunk);
        // delete snd object
        delete snd;
    }
    m_sounds.clear();
#endif
}

void CSndSDL::add(unsigned char *data, unsigned int size, unsigned int uid)
{
    if (m_sounds.find(uid) != m_sounds.end()) {
        printf("ADD: sound already added: %u", uid);
        return;
    }
#ifdef USE_SDL_MIXER
    SND *snd = new SND;
    snd->channel = -1;
    snd->chunk = nullptr;
    bool fail = false;
    SDL_RWops *rw = SDL_RWFromConstMem(static_cast<void*>(data),  size);
    if (!rw) {
        fail = true;
        printf("SDL_RWFromConstMem Failed for uid 0x%x (size: %d): %s", uid, size, SDL_GetError());
    }
    if (!fail) {
        snd->chunk = Mix_LoadWAV_RW(rw, 1);
        if (!snd->chunk) {
            fail = true;
            printf("Mix_LoadWAV_RW Failed: %s", Mix_GetError());
        }
    }
    if (fail) {
        snd->chunk = nullptr;
    }
    m_sounds[uid] = snd;
#endif
}

void CSndSDL::replace(unsigned char *data, unsigned int size, unsigned int uid)
{
    // TODO: get rid of replace
    remove(uid);
    add(data,size,uid);
}

void CSndSDL::remove(unsigned int uid)
{
#ifdef USE_SDL_MIXER
    if (m_sounds.find(uid) == m_sounds.end()) {
        printf("REMOVE: sound not found: %u", uid);
        return;
    }
    SND *snd = m_sounds[uid];
    if (snd->channel!=-1) {
        Mix_HaltChannel(snd->channel);
    }
    Mix_FreeChunk(snd->chunk);
    delete snd;
    m_sounds.erase(uid);
#endif
}

void CSndSDL::play(unsigned int uid)
{
#ifdef USE_SDL_MIXER
    SND *snd = m_sounds[uid];
    if (snd->channel != -1
            && Mix_Playing(snd->channel)) {
        // already playing
        return;
    }
    snd->channel = Mix_PlayChannel(
                snd->channel, snd->chunk, 0);
    if (snd->channel==-1) {
        printf("Mix_PlayChannel: %s",Mix_GetError());
    }
#endif
}

void CSndSDL::stop(unsigned int uid)
{
#ifdef USE_SDL_MIXER
    SND * snd = m_sounds[uid];
    if (snd->channel!=-1) {
        Mix_HaltChannel(snd->channel);
        snd->channel = -1;
    }
#endif
}

void CSndSDL::stopAll()
{
#ifdef USE_SDL_MIXER
    Mix_HaltChannel(-1);
#endif
}

bool CSndSDL::isValid()
{
    return m_valid;
}

bool CSndSDL::has_sound(unsigned int uid)
{
    return m_sounds.find(uid) != m_sounds.end();
}

const char *CSndSDL::signature() const
{
    return "lgck-sdl-sound";
}
