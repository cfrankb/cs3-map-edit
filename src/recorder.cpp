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
#define LOG_TAG "recorder"
#include <algorithm>
#include <cstring>
#include "recorder.h"
#include "logger.h"

CRecorder::CRecorder(const size_t bufSize)
{
    m_bufSize = bufSize;
    m_buffer = new uint8_t[m_bufSize];
    m_mode = MODE_CLOSED;
    m_file = nullptr;
}

CRecorder::~CRecorder()
{
    delete[] m_buffer;
}

bool CRecorder::start(IFile *file, bool isWrite)
{
    const char SIG[] = {'R', 'E', 'C', '!'};
    auto readFile = [file](auto ptr, auto size)
    {
        return file->read(ptr, size) == 1;
    };
    auto writeFile = [file](auto ptr, auto size)
    {
        return file->write(ptr, size) == 1;
    };
    m_newInfo = true;
    m_index = 0;
    m_count = 0;
    m_size = 0;

    m_mode = isWrite ? MODE_WRITE : MODE_READ;
    m_file = file;
    if (m_file && m_mode == MODE_WRITE)
    {
        const uint32_t version = VERSION;
        const uint8_t placeholder[] = {0, 0, 0, 0};
        writeFile(SIG, sizeof(SIG));
        writeFile(&version, sizeof(version));
        m_offset = file->tell();
        writeFile(placeholder, sizeof(placeholder)); // placeholder for datasize
    }
    else if (m_file && m_mode == MODE_READ)
    {
        uint32_t version = 0xffff;
        char sig[sizeof(SIG) + 1];
        sig[sizeof(SIG)] = '\0';
        readFile(sig, sizeof(SIG));
        if (memcmp(sig, SIG, sizeof(SIG)) != 0)
        {
            char oSig[sizeof(SIG) + 1];
            oSig[sizeof(SIG)] = '\0';
            memcpy(oSig, SIG, sizeof(SIG));
            LOGE("signature mismatch:%s; expecting :%s\n", sig, oSig);
            return false;
        }
        readFile(&version, sizeof(version));
        if (version != VERSION)
        {
            LOGE("version mismatch: 0x%.8x; expecting :0x%.8x\n", version, VERSION);
            return false;
        }
        readFile(&m_size, sizeof(m_size)); // total datasize of data
        readNextBatch();
    }
    else
    {
        m_mode = MODE_CLOSED;
        LOGE("invalid file handle\n");
        return false;
    }
    return true;
}

bool CRecorder::readNextBatch()
{
    m_batchSize = std::min(static_cast<uint32_t>(m_bufSize), m_size);
    m_size -= m_batchSize;
    m_index = 0;
    if (m_file->read(m_buffer, m_batchSize) != 1)
        return false;
    return true;
}

void CRecorder::append(const uint8_t *input)
{
    // encode input
    uint8_t data = 0;
    for (int i = 0; i < INPUTS; ++i)
    {
        data |= (input[i] != 0) << i;
    }
    if (m_newInfo)
    {
        m_current = data;
        m_count = 1;
        m_newInfo = false;
    }
    else if (m_current == data)
    {
        ++m_count;
        if (m_count == MAX_CPT)
        {
            storeData(false);
            m_newInfo = true;
        }
    }
    else
    {
        storeData(false);
        m_current = data;
        ++m_count;
    }
}

void CRecorder::storeData(bool finalize)
{
    if (m_count && !m_newInfo)
        m_buffer[m_index++] = m_current | (m_count << 4);
    m_count = 0;
    if ((m_index == m_bufSize) || finalize)
    {
        dump();
    }
}

void CRecorder::dump()
{
    if (m_index)
    {
        if (m_file->write(m_buffer, m_index) != 1)
            LOGE("CRecorder::dump() failed to write\n");
        m_size += m_index;
    }
    m_index = 0;
}

void CRecorder::nextData()
{
    const uint8_t data = m_buffer[m_index++];
    m_current = data & MAX_CPT;
    m_count = data >> 4;
}

bool CRecorder::get(uint8_t *output)
{
    if (m_newInfo)
    {
        nextData();
        m_newInfo = false;
        --m_count;
    }
    else if (m_count != 0)
    {
        --m_count;
    }
    else
    {
        // fetch next batch from disk
        if (m_index == m_batchSize)
        {
            if (m_size != 0)
                readNextBatch();
            else
                return false;
        }
        nextData();
        --m_count;
    }
    decode(output, m_current);
    return true;
}

void CRecorder::decode(uint8_t *output, uint8_t data)
{
    for (int i = 0; i < INPUTS; ++i)
    {
        output[i] = data & 1;
        data >>= 1;
    }
}

void CRecorder::stop()
{
    if (m_mode == MODE_WRITE)
    {
        storeData(true);
        m_file->seek(m_offset);
        if (m_file->write(&m_size, sizeof(m_size)) != 1)
            LOGE("CRecorder::stop() write fail\n");
    }
    m_mode = MODE_CLOSED;
    if (m_file)
        m_file->close();
    m_file = nullptr;
}

bool CRecorder::isRecording()
{
    return m_mode == MODE_WRITE;
}

bool CRecorder::isReading()
{
    return m_mode == MODE_READ;
}

bool CRecorder::isStopped()
{
    return m_mode == MODE_CLOSED;
}