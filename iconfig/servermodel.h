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

/*! \class ServerModel
 *  \brief A model used for server editing.
 *
 * This class will NOT manipulate the servers.ini file. See ServerMgr.
 */

#ifndef SERVERMODEL_H
#define SERVERMODEL_H

#include <QStandardItemModel>
#include "servermgr.h"
#include <QHash>

class ServerModel : public QStandardItemModel
{
    Q_OBJECT

public:
    explicit ServerModel(QObject *parent = 0);
    QModelIndex indexFromHost(QString hostname); // Hostname:Port

    // These will NOT manipulate servers.ini, see ServerMgr class.
    void addNetwork(QString name, QString server);
    void setNetworkServer(QString name, QString server = "");
    void renameNetwork(QString name, QString newname);
    void delNetwork(QString name);
    void addServer(QString name, QString server, QString network = "NONE");
    void setServer(QString name, QString server, QString network = "NONE");
    void renameServer(QString name, QString newname, QString network = "NONE");
    void delServer(QString name, QString network = "NONE");
    void resetModel();

private:
    ServerMgr smgr; //!< Manages the servers.ini file.

    QHash<QString,QModelIndex> hostmap; //!< Keeps track of hostname:port\n Key: hostname:port\n Value: Model index
    QHash<QString,QModelIndex> netmap; //!< Keeps track of networks.\n Key: Network name\n Value: Model index
    QHash<QString,QModelIndex> nonemap; //!< Keeps track of solitary servers (not bound with a network).\n Key: hostname:port\n Value: Model index
};

#endif // SERVERMODEL_H
