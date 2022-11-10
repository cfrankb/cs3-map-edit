/*
    LGCK Builder Runtime
    Copyright (C) 1999, 2011  Francois Blanchette

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

#ifndef _STDAFX_H
#define _STDAFX_H

#include <inttypes.h>
#include <sys/time.h>

// 
// Select the target QT or GCC
// 

#define microtime(___time) struct timeval tm_time; \
    gettimeofday(&tm_time, nullptr); \
    *___time = tm_time.tv_usec / 1000 + tm_time.tv_sec * 1000;

#ifdef USE_QFILE
    #include <QDebug>
    #define ASSERT Q_ASSERT
#else
    #include <assert.h>
    #define qDebug(expr,...) printf(expr,##__VA_ARGS__);puts("");
    #define Q_UNUSED(expr) do { (void)(expr); } while (0)
    #define ASSERT assert
#endif

#define UNUSED(x) (void)x;
#define LONGUINT long unsigned int

#endif
