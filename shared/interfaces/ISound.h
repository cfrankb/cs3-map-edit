/*
    LGCK Builder Runtime
    Copyright (C) 1999, 2013  Francois Blanchette

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

#ifndef ISOUND_H
#define ISOUND_H

class ISound
{
public:
	virtual ~ISound()=0;
    virtual void forget()=0;
    virtual void add(unsigned char *data, unsigned int size, unsigned int uid)=0;
    virtual void remove(unsigned int uid)=0;
    virtual void replace(unsigned char *data, unsigned int size, unsigned int uid)=0;
    virtual void play(unsigned int uid)=0;
    virtual void stop(unsigned int uid)=0;
    virtual void stopAll()=0;
    virtual bool isValid()=0;
    virtual bool has_sound(unsigned int uid)=0;
    virtual const char *signature() const=0;
};

inline ISound::~ISound(){}

#endif
