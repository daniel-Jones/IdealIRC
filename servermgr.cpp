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

#include <iostream>

#include "servermgr.h"
#include "constants.h"

ServerMgr::ServerMgr(QObject *parent) :
    QObject(parent),
    ini(SERV_FILE)
{
}

QStringList ServerMgr::networkList()
{
    int count = ini.CountSections();
    QStringList r;

    for (int i = 1; i <= count; i++)
        r.push_back( ini.ReadIni(i) );

    return r;
}

QHash<QString,QString> ServerMgr::serverList(QString network)
{
    int count = ini.CountItems(network);
    QHash<QString,QString> r;

    for (int i = 1; i <= count; i++) {
        QString servername = ini.ReadIniItem(network, i);
        QString serverdetails = ini.ReadIni(network, i);

        // Insert multi in case someone adds a server with same name in the list (f.ex. via editing servers.ini)
        r.insertMulti(servername, serverdetails);

    }

    return r;
}

QString ServerMgr::defaultServer(QString network)
{
    return ini.ReadIni(network, "DEFAULT");
}

bool ServerMgr::newNetwork(QString name)
{
    if (name == "NONE")
        return false;

    if (! ini.AppendSection(name))
        return false;

    return ini.WriteIni(name, "DEFAULT", "server.name");
}

bool ServerMgr::renameNetwork(QString o_name, QString n_name)
{
    if ((o_name == "NONE") || (n_name == "NONE"))
        return false;
    return ini.RenameSection(o_name, n_name);
}

bool ServerMgr::delNetwork(QString name, bool keep_servers)
{
    // If servers=true, we will keep the servers by moving them to the NONE section.
    // Any servers which got a name exsisting in the NONE, will be renamed to oldnetname_servername.

    if (! ini.SectionExists(name))
        return false;

    if (keep_servers == true) {
        int max = ini.CountItems(name);
        for (int i = 1; i <= max; i++) {
            QString item = ini.ReadIniItem(name, i);
            QString value = ini.ReadIni(name, i);

            QString e_item = ini.ReadIni("NONE", item);
            if (e_item.length() > 0) // item exsists in NONE
                item.prepend(name+"_");

            ini.WriteIni("NONE", item, value);
        }
    }

    ini.DelSection(name);

    return true;
}

bool ServerMgr::addServer(QString name, QString host, QString pw, QString network)
{
    if (pw.length() > 0)
        pw.prepend('|');
    QString detail = QString("%1%2")
                     .arg(host)
                     .arg(pw);

    if ((! ini.SectionExists(network)) && (network != "NONE"))
        return false;

    ini.WriteIni(network, name, detail);
    return true;
}

bool ServerMgr::delServer(QString name, QString network)
{
    std::cout << "deleting " << name.toStdString().c_str() << " from " << network.toStdString().c_str() << std::endl;

    return ini.DelIni(network, name);
}

bool ServerMgr::hasNetwork(QString name)
{
    return ini.SectionExists(name);
}

bool ServerMgr::hasServer(QString name, QString network)
{
    QString data = ini.ReadIni(network, name);
    if (data.length() > 0)
        return true;
    else
        return false;
}

QString ServerMgr::getServerDetails(QString name, QString network)
{
    return ini.ReadIni(network, name);
}
