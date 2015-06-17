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

/*! \class TSock
 *  \brief Can be either a TCP server or TCP client, but not both.
 *
 * For the time being, TCP is only supported. UDP is not forgotten about, and will be implemented.\n
 * Here we handle data I/O and events.
 */

#ifndef TSOCK_H
#define TSOCK_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QStringList>

#include "constants.h"

enum TSOCK_ERR {
    TSE_NONE = 0, // No error
    TSE_NOSUCHNAME, // No such socket name
    TSE_NAMEINUSE, // Socket name in use
    TSE_CANNOTWRITE, // Cannot write to socket
    TSE_SOCKOBJNULL, // Socket (TSock) object is NULL.
    TSE_NOTCONNECTED, // Socket not connected
    TSE_UKWSWITCH, // Unknown sock switch
    TSE_CANNOTBIND, // Cannot bind socket.
    TSE_SOCKINUSE, // Cannot listen, socket in use
    TSE_NOMORECONNECTIONS // No more (incoming) connections to accept
};

class TSock : public QObject
{
  Q_OBJECT

  public:
    explicit TSock(QTcpSocket *sock = 0, QObject *parent = NULL);
    ~TSock();
    TSOCK_ERR open(QString hostname, int port);
    TSOCK_ERR listen(int port);
    TSOCK_ERR close();
    TSOCK_ERR write(QByteArray *data);
    QString bufLen() { return QString::number( socket->bytesAvailable() ); } //!< \return QString, amount of bytes in the input buffer.
    QByteArray readBuffer(); // Read out entire buffer.
    QString readBufferLn(); // Read out first line (to next \n).
    bool haveConnections() { return server.hasPendingConnections(); } //!< \return true if the TCP server have incoming, but yet unattended connections.
    QTcpSocket *acceptConnection();
    void declineConnection(); // Just a skip.


  private:
    int listenPort; // If 0, this is a TCP client
    QTcpSocket *socket; //!< TCP client.
    QTcpServer server; //!< TCP server.

  signals:
    void eventAvailable(QString sockname, e_iircevent event, QStringList para);
    void socketAvailable(QString sockname); // New socket from server is requested.

  private slots:
    void socketError(QAbstractSocket::SocketError error);
    void socketConnected();
    void socketDisconnected();
    void socketDataReady();
    void serverConnection(); // Connection available.

};

#endif // TSOCK_H
