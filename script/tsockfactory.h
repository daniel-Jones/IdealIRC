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

/*! \class TSockFactory
 *  \brief Manages all sockets.
 */

#ifndef TSOCKFACTORY_H
#define TSOCKFACTORY_H

#include <QObject>
#include <QHash>
#include <QStringList>
#include "constants.h"
#include "tsock.h"
#include <iostream>

/*
    If TSOCK_ERR >= 100 then refer to
      QAbstractSocket::SocketError
*/

class TSockFactory : public QObject
{
  Q_OBJECT

public:
    explicit TSockFactory(QObject *parent = 0);
    void socklisten(QString name, int port);
    void sockwrite(QString name, QByteArray *data);
    QByteArray sockread(QString name);
    QString sockreadLn(QString name);
    void sockrename(QString name, QString newname);
    void sockclose(QString name);
    void sockopen(QString name, QString host, int port);
    QString socklist(QString name_patt, int pos);
    TSOCK_ERR sockerror() { return lastErr; } // Last socket error
    QString sockBufLen(QString name);
    bool sockAcceptNext(QString name, QString connected_name);
    bool sockDeclineNext(QString name);
    bool hasName(QString name); // Does the factory have the given sockname?

private:
    QHash<QString,TSock*> slist; //!< List of all sockets,\n Key: name\n Value: socket.
    TSOCK_ERR pickSocket(QString name, TSock **socket);
    TSOCK_ERR lastErr; //!< Last socket error.

private slots:
    void processSockEvent(QString name, e_iircevent event, QStringList para);

signals:
    bool runEvent(e_iircevent evt, QStringList param);

};

#endif // TSOCKFACTORY_H
