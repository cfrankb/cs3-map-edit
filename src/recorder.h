/*
    cs3-runtime-sdl
    Copyright (C) 2025 Francois Blanchette

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
#pragma once

#include <cinttypes>
#include <cstdio>
#include "shared/IFile.h"

class CRecorder
{
public:
    CRecorder(const size_t bufSize = MAX_ENTRIES);
    ~CRecorder();

    bool start(IFile *file, bool isWrite);
    void append(const uint8_t *input);
    bool get(uint8_t *output);
    void stop();
    bool isRecording();
    bool isReading();
    bool isStopped();

private:
    enum
    {
        MAX_ENTRIES = 8192,
        MAX_CPT = 15,
        INPUTS = 4,
        MODE_CLOSED = 0,
        MODE_READ = 1,
        MODE_WRITE = 2,
        VERSION = 0,
    };
    uint8_t m_mode;
    bool m_newInfo = true;
    uint8_t m_current;
    uint32_t m_count;
    uint32_t m_index;
    uint32_t m_size;
    uint32_t m_batchSize;
    uint8_t *m_buffer = nullptr;
    size_t m_bufSize;
    IFile *m_file = nullptr;
    size_t m_offset;

    void decode(uint8_t *output, uint8_t data);
    void storeData(bool finalize);
    void dump();
    bool readNextBatch();
    void nextData();
};