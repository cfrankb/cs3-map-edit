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
/// LOGF must be called as a standalone statement (see note below LOGF).

#pragma once
#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <atomic>
#include <cstdio>
#include <cstdlib>
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
#if defined(__GNUC__)
    // enables -Wformat checking at ~1,100 LOGx call sites (audit logger.h L43)
    __attribute__((format(printf, 3, 4)))
#endif
    static void log(Level level, const char *tag, const char *format, ...);
    static Level level() { return m_minLevel; };

private:
    // atomic: every LOGx macro reads the level while setLevel may write it
    // from another thread (audit L41, data-race hardening)
    static std::atomic<Level> m_minLevel;
    static FILE *m_output;
    static std::mutex m_mutex;
};

// NOTE: this header and src/shared/logger.h must be kept byte-identical;
// src/*.cpp and src/shared/*.cpp each resolve #include "logger.h" to their
// own directory copy (see logger.h.audit.md; consolidate if they ever drift).
#if defined(USE_QFILE)
#include <QtLogging>
#include <QDebug>
#define printf qDebug
#define LOGI(...) qInfo(__VA_ARGS__)
#define LOGW(...) qWarning(__VA_ARGS__)
#define LOGE(...) qCritical(__VA_ARGS__)
#define LOGF(...) qFatal(__VA_ARGS__)
#else
// do/while(0) wrappers make the macros single statements: an `else` after
// LOGx(...) can no longer bind to the level-filter `if` (audit L60-68).
// LOGF deliberately keeps its unbraced form (maintainer ruling BUG-H16):
// call it as a standalone statement, never as the body of a bare `if`.
#define LOGI(...)                           \
    do                                      \
    {                                       \
        if (Logger::level() <= Logger::L_INFO) \
            Logger::log(Logger::L_INFO, __FILE_NAME__, __VA_ARGS__); \
    } while (0)
#define LOGW(...)                           \
    do                                      \
    {                                       \
        if (Logger::level() <= Logger::L_WARN) \
            Logger::log(Logger::L_WARN, __FILE_NAME__, __VA_ARGS__); \
    } while (0)
#define LOGE(...)                            \
    do                                       \
    {                                        \
        if (Logger::level() <= Logger::L_ERROR) \
            Logger::log(Logger::L_ERROR, __FILE_NAME__, __VA_ARGS__); \
    } while (0)
// DO NOT DELETE LOGF or the USE_QFILE/Qt block above: they are kept
// deliberately (maintainer decision 2026-11-17, see
// docs/audits/logger.h.audit.md BUG-H16). An earlier agent removed them as
// "dead code"; that was rejected. CAUTION: LOGF expands unbraced, so
// `if (cond) LOGF(...);` logs conditionally but ALWAYS exits; call LOGF as
// a standalone statement.
#define LOGF(...)                                             \
    Logger::log(Logger::L_FATAL, __FILE_NAME__, __VA_ARGS__); \
    exit(1)
#endif

#endif