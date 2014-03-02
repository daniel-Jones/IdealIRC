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

#include <QHashIterator>
#include <iostream>
#include "tsockfactory.h"

TSockFactory::TSockFactory(QObject *parent) :
  QObject(parent)
{
}

void TSockFactory::socklisten(QString name, int port)
{
    TSock *socket = new TSock();

    lastErr = socket->listen(port);

    if (lastErr != 0) {
        delete socket;
        return;
    }

    socket->setObjectName(name);
    connect(socket, SIGNAL(eventAvailable(QString,e_iircevent,QStringList)),
            this, SLOT(processSockEvent(QString,e_iircevent,QStringList)));
    slist.insert(name.toUpper(), socket); // No errors on opening, add to list.

}

void TSockFactory::sockwrite(QString name, QByteArray *data)
{
    TSock *socket = NULL;
    lastErr = pickSocket(name, &socket); // Get a valid TSock
    if (lastErr > 0)
        return; // Nothing more to do, error is reported at sockerror().

    if (socket == NULL) {
        lastErr = TSE_SOCKOBJNULL;
        return;
    }

    //std::cout << "TSockFactory attempts write to " << name.toStdString().c_str() << std::endl;
    lastErr = socket->write(data); // Furthermore, if writing fails, we also push error to sockerror().
}

QByteArray TSockFactory::sockread(QString name)
{
    TSock *socket = NULL;
    lastErr = pickSocket(name, &socket); // Get a valid TSock
    if (lastErr > 0)
        return QByteArray(); // Nothing more to do, error is reported at sockerror().

    return socket->readBuffer();
}

QString TSockFactory::sockreadLn(QString name)
{
    TSock *socket = NULL;
    lastErr = pickSocket(name, &socket); // Get a valid TSock
    if (lastErr > 0)
        return QString(); // Nothing more to do, error is reported at sockerror().

    return socket->readBufferLn();
}

void TSockFactory::sockrename(QString name, QString newname)
{
    TSock *socket = NULL;
    lastErr = pickSocket(name, &socket);
    if (lastErr > 0)
        return; // Nothing more to do, error is reported at sockerror().
}

void TSockFactory::sockclose(QString name)
{
    TSock *socket = NULL;
    lastErr = pickSocket(name, &socket);
    if (lastErr > 0)
        return; // Nothing more to do, error is reported at sockerror().

    lastErr = socket->close(); // Closing fails will give error to sockerror()
    if (lastErr > 0)
        return; // Nothing more to do, error is reported at sockerror().

    socket->close();
    delete socket;
    slist.remove(name.toUpper());
}

void TSockFactory::sockopen(QString name, QString host, int port)
{
    TSock *socket = NULL;

    TSOCK_ERR err = pickSocket(name, &socket);
    if (err == TSE_NONE) { // If we pick a socket by this name and no errors come, name in use.
        lastErr = TSE_NAMEINUSE;
        return; // stop.
    }

    socket = new TSock(); // With a name available, create a new socket.

    lastErr = socket->open(host, port); // Opening socket gives error to sockerror()
    if (lastErr == 0) {
        socket->setObjectName(name);
        connect(socket, SIGNAL(eventAvailable(QString,e_iircevent,QStringList)),
                        this, SLOT(processSockEvent(QString,e_iircevent,QStringList)));
        slist.insert(name.toUpper(), socket); // No errors on opening, add to list.
    }
    else
        delete socket; // Errors on opening, delete.
}

QString TSockFactory::socklist(QString name_patt, int pos)
{
    // Pos = 0 means return total amount of matching patterns.

    int count = 1;
    QHashIterator<QString,TSock*> i(slist);

    if (pos < 0)
        return QString(); // Negative numbers have nothing here to do.

    while (i.hasNext()) {
        i.next();
        QString iname = i.key();
        QRegExp rx( QRegExp::escape( name_patt.toUpper() ) );
        rx.setPatternSyntax(QRegExp::WildcardUnix);
        int idx = iname.indexOf(rx, Qt::CaseInsensitive);

        /*  std::cout << "iname: " << iname.toStdString().c_str() <<
                       " | rx.pattern(): " << rx.pattern().toStdString().c_str() <<
                       " | idx: " << idx <<
                       " | pos: " << pos <<
                       " | count: " << count <<
                       " [";*/

        if ((pos == 0) && (idx > -1)) {
            // std::cout << "x] LIST" << std::endl;
            count++;
            continue;
        }

        if (pos > 0) {
            if ((idx != -1) && (count == pos)) {
                // std::cout << "x]" << std::endl;
                return iname;
            }
            if ((idx != -1) && (count < pos)) {
                //  std::cout << " ]" << std::endl;
                count++;
            }
        }
        //std::cout << " ] UNWANTED" << std::endl;
    }

    if (pos == 0)
        return QString::number( count-1 );
    else
        return QString();
}

QString TSockFactory::sockBufLen(QString name)
{
    TSock *socket = NULL;
    lastErr = pickSocket(name, &socket);
    if (lastErr > 0)
        return "-1"; // Nothing more to do, error is reported at sockerror().

    return socket->bufLen();
}

bool TSockFactory::sockAcceptNext(QString name, QString connected_name)
{
    TSock *socket = NULL;
    lastErr = pickSocket(name, &socket);
    if (lastErr > 0)
        return false; // Nothing more to do, error is reported at sockerror().

    QTcpSocket *incoming = socket->acceptConnection();

    if (incoming == NULL)
        return false;

    TSock *new_socket = new TSock(incoming); // With a name available, create a new socket.

    new_socket->setObjectName(connected_name);
    connect(new_socket, SIGNAL(eventAvailable(QString,e_iircevent,QStringList)),
            this, SLOT(processSockEvent(QString,e_iircevent,QStringList)));
    slist.insert(connected_name.toUpper(), new_socket); // No errors on opening, add to list.

    return true;
}

bool TSockFactory::hasName(QString name)
{
    TSock *socket = NULL;
    TSOCK_ERR err = pickSocket(name, &socket);
    if (err == TSE_NONE) // If we pick a socket by thhis name and no errors come, name in use.
        return true;
    else
        return false;
}

bool TSockFactory::sockDeclineNext(QString name)
{
    TSock *socket = NULL;
    lastErr = pickSocket(name, &socket);
    if (lastErr > 0)
        return false; // Nothing more to do, error is reported at sockerror().

    socket->declineConnection();
    return true;
}

TSOCK_ERR TSockFactory::pickSocket(QString name, TSock **socket)
{
    *socket = NULL; // NULL would mean "not found"
    QHashIterator<QString,TSock*> i(slist);
    while (i.hasNext()) {
        i.next();
        if (i.key().toUpper() == name.toUpper()) {
            *socket = i.value();
            break;
        }
    }

    if (*socket == NULL)
        return TSE_NOSUCHNAME; // Name not found
    else
        return TSE_NONE; // No error
}

void TSockFactory::processSockEvent(QString name, e_iircevent event, QStringList para)
{
    if (event == te_sockclose) {
        TSock *socket = NULL;
        lastErr = pickSocket(name, &socket);
        if (lastErr == 0)
            socket->close();
    }

    emit runEvent(event, para);
}
