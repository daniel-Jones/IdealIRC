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

#ifndef SERVERMGR_H
#define SERVERMGR_H

#include <QObject>
#include <QStringList>
#include <QHash>

#include "inifile.h"

class ServerMgr : public QObject
{
    Q_OBJECT
public:
    explicit ServerMgr(QObject *parent = 0);
    // All networks in a string list (Also counts in the NONE network)
    QStringList networkList();
    // All servers from a network in a hash map <"name","server:port|passwd">
    QHash<QString,QString> serverList(QString network = "NONE");
    // Return default server of given network (The "NONE" network have no default!) - returns empty if no default server is set.
    QString defaultServer(QString network);
    // Add new network to servers.ini - returns false if network exist
    bool newNetwork(QString name);
    // Rename a network - returns false if new network name already exist
    bool renameNetwork(QString o_name, QString n_name);
    // Delete network - false if network didn't exsist (useless result?)
    bool delNetwork(QString name, bool keep_servers = false);
    // Add (or update) a server to network - returns false if network doesn't exsist
    bool addServer(QString name, QString host /*host:port*/, QString pw = "", QString network = "NONE");
    // Delete a server from network - false if network or server didn't exist
    bool delServer(QString name, QString network = "NONE");
    // Check of we have the given network name
    bool hasNetwork(QString name);
    // Check if we have the given server name inside the network
    bool hasServer(QString name, QString network = "NONE");
    // Get server details
    QString getServerDetails(QString name, QString network = "NONE");

private:
    IniFile ini;
    
};

#endif // SERVERMGR_H
