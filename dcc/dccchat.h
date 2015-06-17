/*
 *   IdealIRC - Internet Relay Chat client
 *   Copyright (C) 2015  Tom-Andre Barstad
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

/*! \class DCCChat
 *  \brief DCC Chat
 */

#ifndef DCCCHAT_H
#define DCCCHAT_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

#include "dcc.h"

class IWin;

class DCCChat : public DCC
{
  Q_OBJECT
public:
    explicit DCCChat(DCCType t, IWin *parentWin, QString dcc_details);
    ~DCCChat();
    void inputEnterPushed(QString line);

private:
    QTcpSocket *socket;
    QTcpServer server;

    // nicknames
    QString target;
    QString me;

    void initialize();

private slots:
    void sockConnected();
    void sockDisconnected();
    void sockRead();

    void newConnection();

signals:
    void Highlight();
};

#endif // DCCCHAT_H
