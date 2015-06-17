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

/*!
 * Finds all networks.
 * \return List of all networks
 */
QStringList ServerMgr::networkList()
{
    int count = ini.CountSections();
    QStringList r;

    for (int i = 1; i <= count; i++)
        r.push_back( ini.ReadIni(i) );

    return r;
}

/*!
 * \param network Network name
 *
 * Finds all servers within a given network name.
 * \return List of servers.\n Key: Server name\n Value: hostname:port|password (password is optional)
 */
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

/*!
 * \param network Network name
 *
 * Finds default server of specified network.
 * \return Default server details
 */
QString ServerMgr::defaultServer(QString network)
{
    return ini.ReadIni(network, "DEFAULT");
}

/*!
 * \param name Network name
 *
 * Adds a new network to servers.ini\n
 * This function also adds a default server with a placeholder hostname ("server.name").
 * \return true on success, false otherwise.
 */
bool ServerMgr::newNetwork(QString name)
{
    if (name == "NONE")
        return false;

    if (! ini.AppendSection(name))
        return false;

    return ini.WriteIni(name, "DEFAULT", "server.name");
}

/*!
 * \param o_name Current network name
 * \param n_name New network name
 *
 * Renames a network.
 * \return true on success, false otherwise.
 */
bool ServerMgr::renameNetwork(QString o_name, QString n_name)
{
    if ((o_name == "NONE") || (n_name == "NONE"))
        return false;
    return ini.RenameSection(o_name, n_name);
}

/*!
 * \param name Name of network
 * \param keep_servers Set to true to keep the servers.
 *
 * Deletes a network from servers.ini.\n\n
 * If keep_servers is true, all servers will be moved to 'NONE' section of servers.ini.\n
 * If there's a name collision, the servers name will append "_N" where N is a number not in use.
 * \return
 */
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

/*!
 * \param name Server name
 * \param host Hostname
 * \param pw Password (optional)
 * \param network Network (optional)
 *
 * Adds a server to servers.ini.\n
 * If there's no network to add it under, it'll be placed under the 'NONE' section of servers.ini.
 * \return true on success, false otherwise
 */
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

/*!
 * \param name Name of server
 * \param network Network (optional)
 *
 * Deletes a server from a given network.\n
 * If there's no network sepcified, it'll delete server from 'NONE' section of servers.ini
 * \return true on success, false otherwise
 */
bool ServerMgr::delServer(QString name, QString network)
{
    return ini.DelIni(network, name);
}

/*!
 * \param name Network name
 *
 * Checks if given network is within servers.ini
 * \return true if exist, false otherwise
 */
bool ServerMgr::hasNetwork(QString name)
{
    return ini.SectionExists(name);
}

/*!
 * \param name Server name
 * \param network Network name (optional)
 *
 * Checks if specified server is within the specified network.\n
 * If network name isn't specified, it'll check under the 'NONE' section of servers.ini
 * \return true if exist, false otherwise
 */
bool ServerMgr::hasServer(QString name, QString network)
{
    QString data = ini.ReadIni(network, name);
    if (data.length() > 0)
        return true;
    else
        return false;
}

/*!
 * \param name Server name
 * \param network Network name (optional)
 *
 * Finds specified server within a specified network.\n
 * If network name isn't specified, it'll check under the 'NONE' section of servers.ini
 * \return Server details, hostname:port|password (password is optional)
 */
QString ServerMgr::getServerDetails(QString name, QString network)
{
    return ini.ReadIni(network, name);
}
