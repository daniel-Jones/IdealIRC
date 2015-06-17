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

#include "servermodel.h"
#include <QDebug>

ServerModel::ServerModel(QObject *parent) :
    QStandardItemModel(parent)
{
    resetModel();
}

/*!
 * \param hostname Hostname to find
 *
 * Finds a model index based on a hostname.
 * \return Index. Can be an invalid index.
 */
QModelIndex ServerModel::indexFromHost(QString hostname)
{
    return hostmap.value(hostname);
}

/*!
 * \param name Name of network
 * \param server Default server details
 *
 * Adds a network to the model.
 */
void ServerModel::addNetwork(QString name, QString server)
{
    QStandardItem *root = invisibleRootItem();

    QStandardItem *pname = new QStandardItem(QIcon(":/options/gfx/network.png"), name);
    QStandardItem *phost = new QStandardItem(server);
    QList<QStandardItem*> list;
    list << pname << phost;

    root->appendRow(list);
    hostmap.insert(server, pname->index());
    netmap.insert(name, pname->index());
}

/*!
 * \param name Name of network
 * \param server Default server address
 *
 * Changes the server details of default network server.
 */
void ServerModel::setNetworkServer(QString name, QString server)
{
    QModelIndex current = netmap.value(name);
    int row = current.row();
    QModelIndex serverIndex = index(row, 1, current.parent());

    QStandardItem *item = itemFromIndex(serverIndex);
    item->setText(server);
}

/*!
 * \param name Name of network
 * \param newname New name of network
 *
 * Renames a network
 */
void ServerModel::renameNetwork(QString name, QString newname)
{
    QModelIndex current = netmap.value(name);
    int row = current.row();
    QModelIndex nameIndex = index(row, 0, current.parent());

    QStandardItem *item = itemFromIndex(nameIndex);
    item->setText(newname);

    netmap.remove(name);
    netmap.insert(newname, current);
}

/*!
 * \param name Network name
 *
 * Deletes a network from the model.
 */
void ServerModel::delNetwork(QString name)
{
    QModelIndex current = netmap.value(name);
    netmap.remove(name);
    int row = current.row();

    removeRow(row, invisibleRootItem()->index());
}

/*!
 * \param name Server name
 * \param server Server details
 * \param network Network name (optional)
 *
 * Adds a server to specified network.\n
 * If network is not set, it'll have no parent.
 */
void ServerModel::addServer(QString name, QString server, QString network)
{
    QStandardItem *parent;

    if (network.length() == 0)
        network = "NONE";

    if (network == "NONE")
        parent = invisibleRootItem();
    else
        parent = itemFromIndex( netmap.value(network) );

    QStandardItem *sname = new QStandardItem(QIcon(":/options/gfx/server.png"), name);
    QStandardItem *shost = new QStandardItem(server);
    QList<QStandardItem*> list;
    list << sname << shost;

    parent->appendRow(list);
    hostmap.insert(server, indexFromItem(sname));
    if (network == "NONE")
        nonemap.insert(name, indexFromItem(sname));
}

/*!
 * \param name Name of server
 * \param server Server details
 * \param network Network name (optional
 *
 * Changes server details of specified server.\n
 * If network isn't specified, this applies to servers without parents.
 */
void ServerModel::setServer(QString name, QString server, QString network)
{
    QStandardItem *parent;

    if (network.length() == 0)
        network = "NONE";

    if (network == "NONE")
        parent = invisibleRootItem();
    else
        parent = itemFromIndex( netmap.value(network) );

    QModelIndex parentIdx = indexFromItem(parent); // Parent index
    QModelIndex current; // Item's index

    if (network != "NONE") {

        for (int r = 0 ;; r++) {
            QModelIndex idx = parentIdx.child(r, 0);

            if (! idx.isValid())
                return; // No relevant child found, stop.

            if (idx.data().toString() == name) {
                current = idx;
                break;
            }
        }
    }
    else {
        current = nonemap.value(name);
    }


    int row = current.row();
    QModelIndex nameIndex = index(row, 0, current.parent());
    QModelIndex serverIndex = index(row, 1, current.parent());
    QStandardItem *serverItem = itemFromIndex(serverIndex);
    serverItem->setText(server);

    hostmap.insert(server, nameIndex);
}

/*!
 * \param name Server name
 * \param newname New server name
 * \param network Network name (optional)
 *
 * Changes a server name.\n
 * If network isn't specified, this applies to servers without parents.
 */
void ServerModel::renameServer(QString name, QString newname, QString network)
{
    QStandardItem *parent;

    if (network.length() == 0)
        network = "NONE";

    if (network == "NONE")
        parent = invisibleRootItem();
    else
        parent = itemFromIndex( netmap.value(network) );

    QModelIndex parentIdx = indexFromItem(parent); // Parent index
    QModelIndex current; // Item's index

    if (network != "NONE") {

        for (int r = 0 ;; r++) {
            QModelIndex idx = parentIdx.child(r, 0);

            if (! idx.isValid())
                return; // No relevant child found, stop.

            if (idx.data().toString() == name) {
                current = idx;
                break;
            }
        }
    }
    else {
        current = nonemap.value(name);
        nonemap.remove(name);
        nonemap.insert(newname, current);
    }


    int row = current.row();
    QModelIndex serverIndex = index(row, 0, current.parent());
    QStandardItem *item = itemFromIndex(serverIndex);
    item->setText(newname);
}

/*!
 * \param name Server name
 * \param network Network name (optional)
 *
 * Deletes a server.\n
 * If network isn't specified, this applies to servers without parents.
 */
void ServerModel::delServer(QString name, QString network)
{
    QStandardItem *parent;

    if (network.length() == 0)
        network = "NONE";

    if (network == "NONE")
        parent = invisibleRootItem();
    else
        parent = itemFromIndex( netmap.value(network) );

    QModelIndex parentIdx = indexFromItem(parent); // Parent index
    QModelIndex current; // Item's index

    if (network != "NONE") {

        for (int r = 0 ;; r++) {
            QModelIndex idx = parentIdx.child(r, 0);

            if (! idx.isValid())
                return; // No relevant child found, stop.

            if (idx.data().toString() == name) {
                current = idx;
                break;
            }
        }
    }
    else {
        current = nonemap.value(name);
        nonemap.remove(name);
    }


    int row = current.row();
    removeRow(row, current.parent());
}

/*!
 * Clears the model.
 */
void ServerModel::resetModel()
{
    clear();

    QStandardItem *root = invisibleRootItem();

    QStandardItem *i = new QStandardItem();
    QStringList l;
    l << tr("Name") << tr("Host");
    setColumnCount(2);
    setHorizontalHeaderItem(0, i);
    setHorizontalHeaderLabels(l);


   QStringList netlist = smgr.networkList();

   if (netlist.contains("NONE")) { // "None" network is a section with servers not assigned to a network.
       QHash<QString,QString> sl = smgr.serverList("NONE");
       QHashIterator<QString,QString> i(sl);
       while (i.hasNext()) {
           i.next();
           // Key: Server name
           // Value: host:port|pass
           QString name = i.key();
           QString detail = i.value();

           QString host; // hostname with port, e.g. irc.network.org:6667
           host = detail.split('|')[0];

           QStandardItem *itemname = new QStandardItem(QIcon(":/options/gfx/server.png"), name);
           QStandardItem *itemhost = new QStandardItem(host);
           QList<QStandardItem*> list;
           list << itemname << itemhost;

           root->appendRow(list);
           hostmap.insert(host, indexFromItem(itemname));
           nonemap.insert(name, indexFromItem(itemname));

       }
   }

   for (int i = 0; i <= netlist.count()-1; ++i) {

       if (netlist[i] == "NONE")
           continue; // The "None" network already taken care of - ignore.

       QString data = smgr.defaultServer(netlist[i]);
       QString host = data.split('|')[0];

       QStandardItem *pname = new QStandardItem(QIcon(":/options/gfx/network.png"), netlist[i]); // parent name
       QStandardItem *phost = new QStandardItem(host); // parent host
       QList<QStandardItem*> list;
       list << pname << phost;

       root->appendRow(list);
       hostmap.insert(host, pname->index());
       netmap.insert(netlist[i], pname->index());

       QHash<QString,QString> sl = smgr.serverList(netlist[i]);
       QHashIterator<QString,QString> sli(sl);
       while (sli.hasNext()) {
           sli.next();
           // Key: Server name
           // Value: host:port|pass
           QString name = sli.key();
           if (name == "DEFAULT")
               continue; // The default value already taken care of, it's the address of parent item.
           QString detail = sli.value();
           QString host; // hostname with port, e.g. irc.network.org:6667
           host = detail.split('|')[0];

           QStandardItem *itemname = new QStandardItem(QIcon(":/options/gfx/server.png"), name); // parent name
           QStandardItem *itemhost = new QStandardItem(host); // parent host
           QList<QStandardItem*> list;
           list << itemname << itemhost;

           pname->appendRow(list);
           hostmap.insert(host, indexFromItem(itemname));
       }
   }
}
