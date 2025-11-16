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
// logger.h
/// Logging macros for info, warning, error, and fatal messages.
/// Usage: LOGI("Message %s", str); LOGF("Fatal error %d", code);

#pragma once
#include <cstdio>
#include <mutex>
#include <string>

class Logger
{
public:
    enum Level
    {
        L_INFO,
        L_WARN,
        L_ERROR,
        L_FATAL
    };
    static void setLevel(Level minLevel) { m_minLevel = minLevel; }
    static void setOutputFile(const std::string &filename);
    static void log(Level level, const char *tag, const char *format, ...);
    static Level level() { return m_minLevel; };

private:
    static Level m_minLevel;
    static FILE *m_output;
    static std::mutex m_mutex;
};

#if defined(USE_QFILE)
#include <QtLogging>
#define printf qDebug
#define LOGI(...) qInfo(__VA_ARGS__)
#define LOGW(...) qWarning(__VA_ARGS__)
#define LOGE(...) qCritical(__VA_ARGS__)
#define LOGF(...) qFatal(__VA_ARGS__)
#else
#define LOGI(...)                          \
    if (Logger::level() <= Logger::L_INFO) \
    Logger::log(Logger::L_INFO, __FILE_NAME__, __VA_ARGS__)
#define LOGW(...)                          \
    if (Logger::level() <= Logger::L_WARN) \
    Logger::log(Logger::L_WARN, __FILE_NAME__, __VA_ARGS__)
#define LOGE(...)                           \
    if (Logger::level() <= Logger::L_ERROR) \
    Logger::log(Logger::L_ERROR, __FILE_NAME__, __VA_ARGS__)
#define LOGF(...)                                             \
    Logger::log(Logger::L_FATAL, __FILE_NAME__, __VA_ARGS__); \
    exit(1)
#endif