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
#include "qthelper.h"
#include "../Frame.h"
#include <QtDebug>
#include <QString>

QIcon frame2icon(CFrame & frame)
{
    QIcon icon;
    icon.addPixmap(frame2pixmap(frame), QIcon::Normal, QIcon::On);
    return icon;
}

QPixmap frame2pixmap(CFrame & frame)
{
    const QImage img = QImage(reinterpret_cast<uint8_t*>(frame.getRGB()), frame.len(), frame.hei(), QImage::Format_RGBA8888);
    return QPixmap::fromImage(img);
}
