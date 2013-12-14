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

#include <iostream>

#include "servermgr.h"
#include "constants.h"

ServerMgr::ServerMgr(QObject *parent) :
    QObject(parent),
    ini(SERV_FILE)
{
}

// Returns a QStringList of sections found in servers.ini (Also counts the None section)
QStringList ServerMgr::networkList()
{
    int count = ini.CountSections();
    QStringList r;

    for (int i = 1; i <= count; i++)
        r.push_back( ini.ReadIni(i) );

    return r;
}

// Returns a QHash mapped server name with the details, server:port|passwd
QHash<QString,QString> ServerMgr::serverList(QString network)
{
    int count = ini.CountItems(network);
    QHash<QString,QString> r;

    for (int i = 1; i <= count; i++) {
        QString servername = ini.ReadIniItem(network, i);
        QString serverdetails = ini.ReadIni(network, i);

        // Insert multi in case someone adds a server with same name in the list.
        r.insertMulti(servername, serverdetails);

    }

    return r;
}

// Returns a string of the "default server" of the IRC network. May return empty.
QString ServerMgr::defaultServer(QString network)
{
    return ini.ReadIni(network, "DEFAULT");
}
