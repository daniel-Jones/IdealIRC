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

#include "tsock.h"
#include <QStringList>
#include <iostream>

TSock::TSock(QTcpSocket *sock, QObject *parent) :
  QObject(parent),
  listenPort(0)
{

    if (sock == NULL)
        socket = new QTcpSocket;
    else
        socket = sock;

    connect(socket, SIGNAL(connected()),
            this, SLOT(socketConnected()));

    connect(socket, SIGNAL(disconnected()),
            this, SLOT(socketDisconnected()));

    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(socketError(QAbstractSocket::SocketError)));

    connect(socket, SIGNAL(readyRead()),
            this, SLOT(socketDataReady()));


    connect(&server, SIGNAL(newConnection()),
            this, SLOT(serverConnection()));
}

TSock::~TSock()
{
    delete socket;
}

TSOCK_ERR TSock::open(QString hostname, int port)
{
    socket->connectToHost(hostname, port);
    return TSE_NONE;
}

TSOCK_ERR TSock::listen(int port)
{
    if ((server.isListening()) || (socket->isOpen()))
        return TSE_SOCKINUSE;

    if (server.listen(QHostAddress::Any, port)) {
        listenPort = port;
        return TSE_NONE;
    }
    else
        return TSE_CANNOTBIND;
}

QTcpSocket* TSock::acceptConnection() {
    if (server.isListening() == false)
        return NULL;

    return server.nextPendingConnection();
}

void TSock::declineConnection() {
    QTcpSocket *s = server.nextPendingConnection();
    if (s == NULL)
        return;

    s->close();
    delete s;
}

TSOCK_ERR TSock::close()
{
    if (socket->isOpen()) // Close if its open.
        socket->close();
    if (server.isListening())
        server.close();
    return TSE_NONE;
}

TSOCK_ERR TSock::write(QByteArray *data)
{
    qint64 l = socket->write(*data);
    if (l == -1)
        return TSE_CANNOTWRITE;
    else
        return TSE_NONE;
}

QByteArray TSock::readBuffer() // Read out entire buffer.
{
    return socket->readAll();
}

QString TSock::readBufferLn() // Read out first line (to next \n).
{
    QString data = socket->readLine();
    return data.replace('\n', ""); // Remove any newlines (according to Qt docu, we'll get a \n on the end.)
}

void TSock::socketError(QAbstractSocket::SocketError error)
{
    QString eid = QString::number(error+100);
    emit eventAvailable(objectName(), te_sockerror, QStringList()<<objectName()<<eid);
}

void TSock::socketConnected()
{
    emit eventAvailable(objectName(), te_sockopen, QStringList()<<objectName());
}

void TSock::socketDisconnected()
{
    emit eventAvailable(objectName(), te_sockclose, QStringList()<<objectName());
}

void TSock::socketDataReady()
{
    emit eventAvailable(objectName(), te_sockread, QStringList()<<objectName());
}

void TSock::serverConnection()
{
    emit eventAvailable(objectName(), te_socklisten, QStringList()<<objectName());
}
