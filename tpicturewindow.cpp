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

#include "tpicturewindow.h"
#include <QPainter>
#include <QSize>
#include <QResizeEvent>
#include <iostream>
#include <QHashIterator>
#include <QMutableHashIterator>
#include <QDebug>

TPictureWindow::TPictureWindow(QWidget *parent) :
  QWidget(parent),
  pixmap(nullptr),
  viewBuffer(true),
  clearing(false)
{
    setGeometry(x(), y(), parent->width(), parent->height());
    setMouseTracking(true);
}

/*!
 * Resets the current layer.
 */
void TPictureWindow::clear()
{
    if (pixmap == nullptr)
        return;

    // clear current layer
    delete pixmap;
    pixmap = new QPixmap(width(), height());
    pixmap->fill(Qt::transparent);
    layers.insert(currentLayer, pixmap);

    update();
}

/*!
 * Resets all layers.
 */
void TPictureWindow::clearAll()
{
    QMutableHashIterator<QString,QPixmap*> i(layers);
    while (i.hasNext()) {
        i.next();
        QString name = i.key();
        QPixmap *p = i.value();

        if (p != nullptr)
            delete p;

        p = new QPixmap(width(), height());
        p->fill(Qt::transparent);
        layers.insert(name, p);

        if (name == currentLayer)
            pixmap = p;
    }

    update();
}

void TPictureWindow::resizeEvent(QResizeEvent *e)
{
    if (pixmap == nullptr)
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
    pm.fill(Qt::transparent);

    QPainter paint(&pm);
    paint.drawPixmap(0, 0, *pixmap);
    paint.end();

    delete pixmap;
    pixmap = new QPixmap(pm);
}

void TPictureWindow::showEvent(QShowEvent *)
{
    // If "pixmap" is set, this means we're already constructed.
    if (pixmap != nullptr)
        return;

    // First time showing TPictureWindow, construct the main layer.
    lw = width();
    lh = height();

    setLayer("MAIN");
}

void TPictureWindow::paintEvent(QPaintEvent *e)
{
    if ((pixmap == nullptr) || (! viewBuffer))
        return;

    QPainter painter(this);
    painter.fillRect(0, 0, width(), height(), Qt::white);
    for (int i = 0; i <= layerOrder.length()-1; ++i) {
        QPixmap *p = layers.value( layerOrder[i], nullptr );
        if (p == nullptr)
            continue; // layer is invalid. don't do more on this

        painter.drawPixmap(QPoint(0, 0), *p);
    }

    e->accept();
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

/*!
 * \param x X coordinate
 * \param y Y coordinate
 *
 * Paints a dot at X, Y
 */
void TPictureWindow::paintDot(int x, int y)
{
    if (pixmap == nullptr)
        return;

    QPainter paint(pixmap);
    paint.setPen(pen);
    paint.setBrush(brush);
    paint.drawPoint(x, y);
    paint.end();

    update();
}

/*!
 * \param x1 X coordinate at start
 * \param y1 Y coordinate at start
 * \param x2 X coordinate at end
 * \param y2 Y coordnate at end
 *
 * Paints a line between X1,Y1 and X2,Y2.
 */
void TPictureWindow::paintLine(int x1, int y1, int x2, int y2)
{
    if (pixmap == nullptr)
        return;

    QPainter paint(pixmap);
    paint.setPen(pen);
    paint.setBrush(brush);
    paint.drawLine(x1, y1, x2, y2);
    paint.end();

    update();
}

/*!
 * \param x X coordinate
 * \param y Y coordinate
 * \param font Font to use
 * \param text Text to paint
 *
 * Paints a text with specified font at given coordinates.
 */
void TPictureWindow::paintText(int x, int y, QFont font, QString text)
{
    if (pixmap == nullptr)
        return;

    QPainter paint(pixmap);
    paint.setFont(font);
    paint.setPen(pen);
    paint.setBrush(brush);
    paint.drawText(x, y, text);
    paint.end();

    update();
}

/*!
 * \param x X coordinate
 * \param y Y coordinate
 * \param w Width
 * \param h Height
 *
 * Paints a rectangle
 */
void TPictureWindow::paintRect(int x, int y, int w, int h)
{
    if (pixmap == nullptr)
        return;

    QPainter paint(pixmap);
    paint.setPen(pen);
    brush.setStyle(Qt::NoBrush);
    paint.setBrush(brush);
    paint.drawRect(x, y, w, h);
    paint.end();

    update();
}

/*!
 * \param x X coordinate
 * \param y Y coordinate
 * \param r Radius
 *
 * Paints a circle
 */
void TPictureWindow::paintCircle(int x, int y, int r)
{
    if (pixmap == nullptr)
        return;

    QPainter paint(pixmap);
    paint.setPen(pen);
    brush.setStyle(Qt::NoBrush);
    paint.setBrush(brush);
    paint.drawEllipse(x, y, r, r);

    update();
}

/*!
 * \param x X coordinate
 * \param y Y coordinate
 * \param rx Radius on X coordinate
 * \param ry Radius on Y coordinate
 *
 * Paints an ellipse
 */
void TPictureWindow::paintEllipse(int x, int y, int rx, int ry)
{
    if (pixmap == nullptr)
        return;

    QPainter paint(pixmap);
    paint.setPen(pen);
    brush.setStyle(Qt::NoBrush);
    paint.setBrush(brush);
    paint.drawEllipse(x, y, rx, ry);

    update();
}

/*!
 * \param filename Path to image
 * \param x X coordinate
 * \param y Y coordinate
 * \param dontBuffer If true, image wont be buffered.
 *
 * Loads an image from disk and paints it.\n
 * By default, images will be buffered into memory for less disk usage. This can be prevented if dontBuffer is true.
 */
void TPictureWindow::paintImage(QString filename, int x, int y, bool dontBuffer)
{
    if (pixmap == nullptr)
        return;

    QImage *image = nullptr;

    bool buffered = false;
    if (dontBuffer) {
        QHashIterator<QString,QImage*> i(imgList);
        while (i.hasNext()) {
            i.next();
            if (i.key() != filename)
                continue;

            image = i.value(); // Get buffer of image.
            buffered = true;
            break;
        }
    }

    if (buffered == false) {
        image = new QImage();
        if (! image->load(filename))
            return;

        if (! dontBuffer)
            imgList.insert(filename, image); // Save image to our buffer.
    }

    QPainter paint(pixmap);
    paint.setPen(pen);
    paint.setBrush(brush);
    paint.drawImage(x, y, *image);
    paint.end();

    update();
}

/*!
 * \param x X coordinate
 * \param y Y coordinate
 * \param w Width
 * \param h Height
 *
 * Paints a filled rectangle.
 */
void TPictureWindow::paintFill(int x, int y, int w, int h)
{
    if (pixmap == nullptr)
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

/*!
 * \param path List of coordinates to paint within.
 *
 * Paints a filled shape defined by the coordinates.\n
 */
void TPictureWindow::paintFillPath(QPainterPath path)
{
    if (pixmap == nullptr)
        return;

    QPainter paint(pixmap);
    paint.setBrush(brush);
    paint.setPen(pen);
    paint.fillPath(path, brush);

    update();
}

/*!
 * \param b Boolean value
 *
 * If set to false, painting operations will not be visible until set back to true.
 */
void TPictureWindow::setViewBuffer(bool b)
{
    viewBuffer = b;

    if (viewBuffer)
        update();
}

/*!
 * \param name Layer name
 *
 * Changes what layer to paint upon.\n
 * If the layer don't exist, it will be created and set to current layer.
 */
void TPictureWindow::setLayer(QString name)
{
    pixmap = layers.value(name.toUpper(), nullptr);
    if (pixmap == nullptr) {
        pixmap = new QPixmap(width(), height());
        pixmap->fill(Qt::transparent);
        layers.insert(name.toUpper(), pixmap);
        layerOrder << name.toUpper();
    }
    currentLayer = name.toUpper();
}

/*!
 * \param name Layer name
 *
 * Deletes specified layer.\n
 * You are not able to delete the main layer 'MAIN'.
 */
void TPictureWindow::delLayer(QString name)
{
    if (name.toUpper() == "MAIN")
        return; // not allowed

    layers.remove(name.toUpper());
    pixmap = layers.value("MAIN", nullptr);
    currentLayer = "MAIN";
}

/*!
 * \param list List of layer names
 *
 * Re-orders the layers in ascending order, specified in the list.\n
 * You don't need to specify all layers in here.\n
 * Re-ordered layers will be placed on the bottom of the untouched layers.
 */
void TPictureWindow::orderLayers(QStringList list)
{
    QStringList add;
    for (int i = 0; i <= list.length()-1; ++i) {
        QString item = list[i].toUpper();
        layerOrder.removeAll(item);
        add.push_back(item);
    }

    layerOrder << add;
    layerOrder.removeDuplicates();
}

/*!
 * \param layer Layer name
 * \param x X coordinate
 * \param y Y coordinate
 *
 * Finds the color at specified coordinates in specified layer.
 * \return Color in hex format (#RRGGBB)
 */
QString TPictureWindow::colorAt(QString layer, int x, int y)
{
    if (! layers.contains(layer.toUpper()))
        return "#000000";
    QPixmap *pm = layers.value(layer.toUpper());
    QRgb rgb = pm->toImage().pixel(x, y);

    return QColor(rgb).name();
}
