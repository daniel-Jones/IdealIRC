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

#include <QDebug>
#include <QHashIterator>
#include "servertreeitem.h"
#include "servertreemodel.h"

ServerTreeModel::ServerTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    QList<QVariant> rootData;
    rootData << "Name" << "Address";
    rootItem = new ServerTreeItem(rootData);
    setupModelData(rootItem);
}

ServerTreeModel::~ServerTreeModel()
{
    delete rootItem;
}

int ServerTreeModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<ServerTreeItem*>(parent.internalPointer())->columnCount();
    else
        return rootItem->columnCount();
}

QVariant ServerTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    ServerTreeItem *item = static_cast<ServerTreeItem*>(index.internalPointer());

    return item->data(index.column());
}

Qt::ItemFlags ServerTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant ServerTreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

QModelIndex ServerTreeModel::index(int row, int column, const QModelIndex &parent)
            const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    ServerTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<ServerTreeItem*>(parent.internalPointer());

    ServerTreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex ServerTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    ServerTreeItem *childItem = static_cast<ServerTreeItem*>(index.internalPointer());
    ServerTreeItem *parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int ServerTreeModel::rowCount(const QModelIndex &parent) const
{
    ServerTreeItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<ServerTreeItem*>(parent.internalPointer());

    return parentItem->childCount();
}

void ServerTreeModel::setupModelData(ServerTreeItem *parent)
{
    ServerMgr smgr;
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
            QString pass;
            if (detail.split('|').count() > 1)
                pass = detail.split('|')[1];
            host = detail.split('|')[0];
            QList<QVariant> cdata;
            cdata << name << host;

            parent->appendChild(new ServerTreeItem(cdata, pass, parent));

        }
    }

    for (int i = 0; i <= netlist.count()-1; ++i) {

        if (netlist[i] == "NONE")
            continue; // The "None" network already taken care of - ignore.

        QList<QVariant> netparentData;
        QString pdata = smgr.defaultServer(netlist[i]);
        QString phost = pdata.split('|')[0];
        QString ppass;
        if (pdata.split('|').count() > 1)
            ppass = pdata.split('|')[1];

        netparentData << netlist[i] << phost;
        ServerTreeItem *netparent = new ServerTreeItem(netparentData, ppass, parent);
        parent->appendChild(netparent);

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
            QString pass;
            if (detail.split('|').count() > 1)
                pass = detail.split('|')[1];
            host = detail.split('|')[0];
            QList<QVariant> cdata;
            cdata << name << host;

            netparent->appendChild(new ServerTreeItem(cdata, pass, netparent));
        }
    }
}

void ServerTreeModel::addNetwork(QString name)
{
    QList<QVariant> data;
    data << name << "server.name";
    ServerTreeItem *item = new ServerTreeItem(data, "", rootItem);
    rootItem->appendChild(item);


    emit dataChanged(tl, br);
}

void ServerTreeModel::addServer(QString name, QString network)
{

}
