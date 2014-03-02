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

#ifndef TSCRIPTCOMMAND_H
#define TSCRIPTCOMMAND_H

#include <QObject>

#include "constants.h"

class IConnection;

class TScriptCommand : public QObject
{
    Q_OBJECT

public:
    explicit TScriptCommand(QObject *parent, QHash<int,IConnection*> *cl, QHash<int,subwindow_t> *wl, int *aConn, int *aWid);
    bool parse(QString &command);

    void echo(QString target, QString text, int type = PT_NORMAL);
    void window(QString name, QString sw);

    // For writing to current connection, NOT custom sockets!
    void sockwrite(QString &data);

private:
    QHash<int,IConnection*> *conlist;
    QHash<int,subwindow_t> *winlist;
    int *activeConn;
    int *activeWid;

    subwindow_t getCustomWindow(QString name);

signals:
    void RequestWindow(QString name, int type, int parent, bool activate);

};

#endif // TSCRIPTCOMMAND_H
