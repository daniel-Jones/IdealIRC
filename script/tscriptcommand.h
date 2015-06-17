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

/*! \class TScriptCommand
 *  \brief Script command parser.
 */

#ifndef TSCRIPTCOMMAND_H
#define TSCRIPTCOMMAND_H

#include <QObject>
#include <QPainterPath>
#include <math.h>
#include "constants.h"

class IConnection;

class TScriptCommand : public QObject
{
    Q_OBJECT

public:
    explicit TScriptCommand(QObject *parent, QHash<int,IConnection*> *cl, QHash<int,subwindow_t> *wl, int *aConn, int *aWid);
    bool parse(QString &command);

    void echo(QString target, QString sender, QString text, int type = PT_NORMAL);
    void window(QString name, QString sw);
    void clear(QString window = "$ACTIVE$", QString sw = "");
    void paintdot(QString Window, QString X, QString Y, QString Size, QString Color);
    void paintline(QString Window, QString X1, QString Y1, QString X2, QString Y2, QString Size, QString Color);
    void paintrect(QString Window, QString X, QString Y, QString W, QString H, QString Size, QString Color);
    void paintimage(QString Window, QString X, QString Y, QString File, bool dontBuffer);
    void painttext(QString Window, QString X, QString Y, QString FontSize, QString Color, QString FontName, QString Text);
    void paintfill(QString Window, QString X, QString Y, QString W, QString H, QString Color);
    void paintfillpath(QString Window, QString Color, QPainterPath Path);
    void paintcircle(QString Window, QString X, QString Y, QString R, QString Size, QString Color);
    void paintellipse(QString Window, QString X, QString Y, QString RX, QString RY, QString Size, QString Color);
    void paintsetlayer(QString Window, QString Layer);
    void paintdellayer(QString Window, QString Layer);
    void paintlayerorder(QString Window, QStringList Layers);
    void clearimgbuf(QString Window);
    void paintbuffer(QString Window, bool State);

    // For writing to current connection, NOT custom sockets!
    void sockwrite(QString &data);

private:
    QHash<int,IConnection*> *conlist; //!< List of all IRC connections.
    QHash<int,subwindow_t> *winlist; //!< List of all subwindows. Pointer from IdealIRC class.
    int *activeConn; //!< Current active connection.
    int *activeWid; //!< Current active window ID.
    subwindow_t getCustomWindow(QString name);
    QString tstar; //!< Just a string with three stars. (***)

    // Error messages
    QString InvalidParameterCount(QString cmd)
    {
        return tr("/%1: Invalid parameter count").arg(cmd);
    }

    QString NoSuchWindow(QString cmd)
    {
        return tr("/%1: No such window").arg(cmd);
    }

    QString NotAPaintWindow(QString cmd)
    {
        return tr("/%1: Not a paint window").arg(cmd);
    }


signals:
    void RequestWindow(QString name, int type, int parent, bool activate);

};

#endif // TSCRIPTCOMMAND_H
