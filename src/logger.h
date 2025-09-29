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

#if defined(USE_QFILE)
#include <QtLogging>
#define printf qDebug
#define LOGI(...) qInfo(__VA_ARGS__)
#define LOGW(...) qWarning(__VA_ARGS__)
#define LOGE(...) qCritical(__VA_ARGS__)
#define LOGF(...) qFatal(__VA_ARGS__)
#elif defined(__ANDROID__)
#include <android/log.h> // For logging
#define printf(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL, LOG_TAG, __VA_ARGS__)
#elif defined(__ANDROID__) && defined(SDL_MAJOR_VERSION)
#define printf(...) SDL_Log(__VA_ARGS__)
#define LOGI(...) SDL_Log(__VA_ARGS__)
#define LOGW(...) SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_WARN, __VA_ARGS__)
#define LOGE(...) SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, __VA_ARGS__)
#define LOGF(...) SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_CRITICAL, __VA_ARGS__)
#else
#define LOGI(...) printf(__VA_ARGS__)
#define LOGW(...) printf(__VA_ARGS__)
#define LOGE(...) fprintf(stderr, __VA_ARGS__)
#define LOGF(...)                 \
    fprintf(stderr, __VA_ARGS__); \
    exit(1)
#endif
