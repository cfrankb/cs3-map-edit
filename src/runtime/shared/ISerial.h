/*
    LGCK Builder Runtime
    Copyright (C) 1999, 2018  Francois Blanchette

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

#include <cstdint>
#include <utility>
#include <vector>

#include "IFile.h"
#include "logger.h"

// Stringify an object's name for log context, e.g. OBJECT_NAME(spawns) -> "spawns"
#define OBJECT_NAME(__O__) #__O__

class ISerial
{
public:
    virtual ~ISerial() {};
    virtual bool read(IFile &file) = 0;
    virtual bool write(IFile &file) = 0;
};

/**
 * @brief Serialize a std::vector<T> to an IFile stream.
 *
 * T must provide:
 *   bool write(IFile &file) const;
 *
 * Wire format: a uint32_t element count, followed by each element
 * serialized in vector order.
 *
 * On failure, a LOGE message is emitted using the supplied name, the
 * failing element index, the total count, and the stream position.
 *
 * @tparam T element type
 * @param name object name for log context (use OBJECT_NAME)
 * @param vec the vector to serialize
 * @param file destination stream
 * @return true on success, false on any I/O or element failure
 */
template <typename T>
bool writeVector(const char *name, const std::vector<T> &vec, IFile &file)
{
    const uint32_t count = static_cast<uint32_t>(vec.size());
    if (file.write(&count, sizeof(count)) != IFILE_OK)
    {
        LOGE("writeVector<%s>: failed to write element count (%u) at stream pos %ld",
             name, count, file.tell());
        return false;
    }

    for (uint32_t i = 0; i < count; ++i)
    {
        if (!vec[i].write(file))
        {
            LOGE("writeVector<%s>: failed to write element %u of %u at stream pos %ld",
                 name, i, count, file.tell());
            return false;
        }
    }
    return true;
}

/**
 * @brief Deserialize a std::vector<T> from an IFile stream.
 *
 * T must provide:
 *   bool read(IFile &file);
 *
 * Expects the wire format produced by writeVector: a uint32_t element
 * count, followed by each element. The vector is cleared first; on any
 * failure it is left in a partial state and false is returned.
 *
 * On failure, a LOGE message is emitted using the supplied name, the
 * failing element index, the total count, and the stream position.
 *
 * @tparam T element type
 * @param name object name for log context (use OBJECT_NAME)
 * @param vec vector to populate
 * @param file source stream
 * @return true on success, false on any I/O or element failure
 */
template <typename T>
bool readVector(const char *name, std::vector<T> &vec, IFile &file)
{
    uint32_t count = 0;
    if (file.read(&count, sizeof(count)) != IFILE_OK)
    {
        LOGE("readVector<%s>: failed to read element count at stream pos %ld",
             name, file.tell());
        return false;
    }

    vec.clear();
    vec.reserve(count);
    for (uint32_t i = 0; i < count; ++i)
    {
        T elem;
        if (!elem.read(file))
        {
            LOGE("readVector<%s>: failed to read element %u of %u at stream pos %ld",
                 name, i, count, file.tell());
            return false;
        }
        vec.push_back(std::move(elem));
    }
    return true;
}
