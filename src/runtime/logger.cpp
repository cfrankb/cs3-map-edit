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
// logger.cpp
#include "logger.h"
#include <cstdarg>
#include <cstring>

#ifndef SDL_NOT_WANTED
#include <SDL3/SDL.h>
#endif

Logger::Level Logger::m_minLevel = Logger::L_INFO;
FILE *Logger::m_output = nullptr;
std::mutex Logger::m_mutex;

void Logger::setOutputFile(const std::string &filename)
{
    if (m_output && m_output != stdout && m_output != stderr)
    {
        fclose(m_output);
    }
    m_output = fopen(filename.c_str(), "a");
}

void Logger::log(Level level, const char *tag, const char *format, ...)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    va_list args;
    va_start(args, format);
#if defined(__ANDROID__) && defined(SDL_MAJOR_VERSION)
    SDL_LogPriority priority = level == L_INFO ? SDL_LOG_PRIORITY_INFO : level == L_WARN ? SDL_LOG_PRIORITY_WARN
                                                                     : level == L_ERROR  ? SDL_LOG_PRIORITY_ERROR
                                                                                         : SDL_LOG_PRIORITY_CRITICAL;
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, priority, format, args);
#elif defined(__ANDROID__)
    int priority = level == L_INFO ? ANDROID_LOG_INFO : level == L_WARN ? ANDROID_LOG_WARN
                                                    : level == L_ERROR  ? ANDROID_LOG_ERROR
                                                                        : ANDROID_LOG_FATAL;
    __android_log_vprint(priority, tag, format, args);
#else
    FILE *out = m_output ? m_output : (level == L_ERROR || level == L_FATAL) ? stderr
                                                                             : stdout;
    fprintf(out, "[%s/%s] ", tag, level == L_INFO ? "INFO" : level == L_WARN ? "WARN"
                                                         : level == L_ERROR  ? "ERROR"
                                                                             : "FATAL");
    vfprintf(out, format, args);

    std::string s = std::string(format);
    auto p = s.rfind('\n');
    if (p == std::string::npos || p != s.size() - 1)
        fprintf(out, "\n");
    if (level == L_FATAL)
        exit(1);
#endif
    va_end(args);
}
