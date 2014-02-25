/*
 *   IdealIRC - Internet Relay Chat client
 *   Copyright (C) 2014  Tom-Andre Barstad
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef COLORPICKERSCENE_H
#define COLORPICKERSCENE_H

#include <QGraphicsScene>
#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMoveEvent>

class ColorPickerScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit ColorPickerScene(QObject *parent = 0);

private:
    QGraphicsPixmapItem *gradient;
    QPixmap pm;

protected:
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

signals:
    void colorPicked(QColor color);
};

#endif // COLORPICKERSCENE_H
