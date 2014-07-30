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

#ifndef TPICTUREWINDOW_H
#define TPICTUREWINDOW_H

#include <QWidget>
#include <QPixmap>
#include <QPicture>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QList>
#include <QPen>
#include <QBrush>
#include <QHash>

#include "constants.h"

class TPictureWindow : public QWidget
{
  Q_OBJECT

public:
    explicit TPictureWindow(QWidget *parent = 0);
    void clear(); // clear current layer
    void clearAll(); // clear all layers
    void setBrush(QBrush b) { brush = b; }
    void setPen(QPen p) { pen = p; }
    void setBrushPen(QBrush b, QPen p) { brush = b; pen = p; }
    void drawTest();
    void paintDot(int x, int y);
    void paintLine(int x1, int y1, int x2, int y2);
    void paintText(int x, int y, QFont font, QString text);
    void paintRect(int x, int y, int w, int h);
    void paintCircle(int x, int y, int r);
    void paintEllipse(int x, int y, int rx, int ry);
    void paintImage(QString filename, int x, int y, bool dontBuffer);
    void paintFill(int x, int y, int w = -1, int h = -1);
    void paintFillPath(QPainterPath path);
    void clearImageBuffer() { imgList.clear(); }
    void setViewBuffer(bool b);
    void setLayer(QString name); // Set to other layer for painting. If it doesn't exist, it will be created.
    void delLayer(QString name); // Deletes layer. Cannot delete "MAIN" layer. Sets active layer to MAIN.
    void orderLayers(QStringList list);
    QString colorAt(QString layer, int x, int y);

protected:
    void paintEvent(QPaintEvent *e);
    void resizeEvent(QResizeEvent *e);
    void showEvent(QShowEvent *);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
  
private:
    QPixmap *pixmap; // active layer
    QHash<QString,QPixmap*> layers;
    QStringList layerOrder;
    QHash<QString,QImage*> imgList; // String is file path to image, then image itself.
    QString currentLayer;
    QPen pen;
    QBrush brush;
    int lw;
    int lh;
    bool viewBuffer;
    bool clearing;

signals:
  void mouseEvent(e_iircevent event, int x, int y, int delta = 0);

};

#endif // TPICTUREWINDOW_H
