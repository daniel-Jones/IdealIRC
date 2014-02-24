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

#ifndef TTIMER_H
#define TTIMER_H

#include <QObject>
#include <QTimer>

class TTimer : public QObject
{
  Q_OBJECT

  public:
    explicit TTimer(QString fn, QObject *parent = 0);
    void runTimer(int msec = 1000);
    void stopTimer();

  private:
    QString fnct;
    QTimer timer;

  signals:
    void timeout(QString fn);

  public slots:
    void internalTimeout();
  
};

#endif // TTIMER_H
