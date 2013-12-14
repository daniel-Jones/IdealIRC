/*
 *   IdealIRC - Internet Relay Chat client
 *   Copyright (C) 2013  Tom-Andre Barstad
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

#include "tpicturewindow.h"
#include <QPainter>
#include <QSize>
#include <QResizeEvent>
#include <iostream>
#include <QHashIterator>

TPictureWindow::TPictureWindow(QWidget *parent) :
  QWidget(parent),
  pixmap(NULL),
  viewBuffer(true)
{
  setGeometry(x(), y(), parent->width(), parent->height());
  setMouseTracking(true);
}

void TPictureWindow::clear()
{
  if (pixmap == NULL)
    return;

  lw = width();
  lh = height();

  delete pixmap;
  pixmap = new QPixmap(width(), height());
  pixmap->fill(Qt::white);

  update();
}

void TPictureWindow::resizeEvent(QResizeEvent *e)
{
  if (pixmap == NULL)
    return;

  int w = e->size().width();
  int h = e->size().height();

  if ((w < lw) && (h < lh))
    return; // Refresh only if window's getting bigger.

  if (w > lw)
    lw = w;

  if (h > lh)
    lh = h;

  QPixmap pm(w, h);

  QPainter paint(&pm);
  paint.fillRect(0,0,lw,lh,Qt::white);
  paint.drawPixmap(0, 0, *pixmap);
  paint.end();

  pixmap = new QPixmap(pm);
}

void TPictureWindow::showEvent(QShowEvent *)
{
  if (pixmap != NULL)
    return;

  lw = width();
  lh = height();

  pixmap = new QPixmap(width(), height());
  pixmap->fill(Qt::white);

  update();
}

void TPictureWindow::paintEvent(QPaintEvent *)
{
  if (pixmap == NULL)
    return;

  if (! viewBuffer)
    return;

  QPainter p(this);

  p.drawPixmap(0,0,*pixmap);
}

void TPictureWindow::mousePressEvent(QMouseEvent *event)
{
    int x = event->pos().x();
    int y = event->pos().y();

    if (event->button() == Qt::LeftButton)
      emit mouseEvent(te_mouseleftdown, x, y);
    if (event->button() == Qt::MiddleButton)
      emit mouseEvent(te_mousemiddledown, x, y);
    if (event->button() == Qt::RightButton)
      emit mouseEvent(te_mouserightdown, x, y);
}

void TPictureWindow::mouseMoveEvent(QMouseEvent *event)
{
    int x = event->pos().x();
    int y = event->pos().y();
  emit mouseEvent(te_mousemove, x, y);
}

void TPictureWindow::mouseReleaseEvent(QMouseEvent *event)
{
    int x = event->pos().x();
    int y = event->pos().y();

  if (event->button() == Qt::LeftButton)
    emit mouseEvent(te_mouseleftup, x, y);
  if (event->button() == Qt::MiddleButton)
    emit mouseEvent(te_mousemiddleup, x, y);
  if (event->button() == Qt::RightButton)
    emit mouseEvent(te_mouserightup, x, y);

}

void TPictureWindow::mouseDoubleClickEvent(QMouseEvent *event)
{

}

void TPictureWindow::wheelEvent(QWheelEvent *event)
{
  int x = event->pos().x();
  int y = event->pos().y();

  int delta = event->delta();

  emit mouseEvent(te_mousemiddleroll, x, y, delta);
}

void TPictureWindow::paintDot(int x, int y)
{
  if (pixmap == NULL)
    return;

  QPainter paint(pixmap);
  paint.setPen(pen);
  paint.setBrush(brush);
  paint.drawPoint(x, y);
  paint.end();

  update();
}

void TPictureWindow::paintLine(int x1, int y1, int x2, int y2)
{
  if (pixmap == NULL)
    return;

  QPainter paint(pixmap);
  paint.setPen(pen);
  paint.setBrush(brush);
  paint.drawLine(x1, y1, x2, y2);
  paint.end();

  update();
}

void TPictureWindow::paintText(int x, int y, QFont font, QString text)
{
  if (pixmap == NULL)
    return;

  QPainter paint(pixmap);
  paint.setFont(font);
  paint.setPen(pen);
  paint.setBrush(brush);
  paint.drawText(x, y, text);
  paint.end();

  update();
}

void TPictureWindow::paintRect(int x, int y, int w, int h)
{
  if (pixmap == NULL)
    return;

  QPainter paint(pixmap);
  paint.setPen(pen);
  brush.setStyle(Qt::NoBrush);
  paint.setBrush(brush);
  paint.drawRect(x, y, w, h);
  paint.end();

  update();
}

void TPictureWindow::paintImage(QString filename, int x, int y)
{
  if (pixmap == NULL)
    return;

  QImage *image = NULL;

  QHashIterator<QString,QImage*> i(imgList);
  bool loaded = false;
  while (i.hasNext()) {
    i.next();
    if (i.key() != filename)
      continue;

    image = i.value(); // Get buffer of image.
    loaded = true;
    break;
  }

  if (loaded == false) {
    image = new QImage();
    loaded = image->load(filename);
    imgList.insert(filename, image); // Save image to our buffer.
  }

  if (loaded == false)
    return;

  QPainter paint(pixmap);
  paint.setPen(pen);
  paint.setBrush(brush);
  paint.drawImage(x, y, *image);
  paint.end();

  update();
}

void TPictureWindow::paintFill(int x, int y, int w, int h)
{
  if (pixmap == NULL)
    return;

  QPainter paint(pixmap);
  paint.setBrush(brush);
  paint.setPen(pen);

  if (w == -1)
    w = lw;

  if (h == -1)
    h = lh;

  paint.fillRect(x, y, w, h, brush.color());
  paint.end();

  update();
}

void TPictureWindow::setViewBuffer(bool b)
{
  viewBuffer = b;

  if (viewBuffer)
    update();
}
