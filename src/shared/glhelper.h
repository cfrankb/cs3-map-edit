/*
    LGCK Builder Runtime
    Copyright (C) 1999, 2020  Francois Blanchette

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
#ifndef GLHELPER_H
#define GLHELPER_H

#include <cstdio>
#include <GL/gl.h>
#include <GL/glu.h>
#include <string>
#include <QDebug>

#ifdef LGCK_OPENGL_DEBUG
static void opengl_msg(unsigned int code, const char *file, int line);
void opengl_msg(unsigned int code, const char *file, int line){
    std::string tmp;
    printf("code:%d\n", code);
    switch(code) {
        case GL_NO_ERROR:
            tmp = "GL_NO_ERROR";
        break;
        case GL_INVALID_ENUM:
            tmp = "GL_INVALID_ENUM";
        break;
        case GL_INVALID_VALUE:
            tmp = "GL_INVALID_VALUE";
        break;
        case GL_INVALID_OPERATION:
            tmp = "GL_INVALID_OPERATION";
        break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            tmp = "GL_INVALID_FRAMEBUFFER_OPERATION";
        break;
        case GL_OUT_OF_MEMORY:
            tmp = "GL_OUT_OF_MEMORY";
        break;
        default:
            char t[40];
            sprintf(t, "GL UNKNOWN:%u", code);
            tmp = t;
    }
    if (code != GL_NO_ERROR) {
        qDebug("Opengl error: %s in %s line %d",
            tmp.c_str(), file, line); \
    }
}
    #define GLDEBUG() opengl_msg(glGetError(), __FILE__, __LINE__ );
#else
	#define GLDEBUG()
#endif	
	
#endif // GLHELPER_H
