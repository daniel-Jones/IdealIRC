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
#include "servermodel.h"

ServerModel::ServerModel(QObject *parent) :
    QStandardItemModel(parent)
{
     QStandardItem *root = invisibleRootItem();

     QStandardItem *i = new QStandardItem();
     QStringList l;
     l << "Name" << "Host";
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

            QStandardItem *itemname = new QStandardItem(name);
            QStandardItem *itemhost = new QStandardItem(host);
            QList<QStandardItem*> list;
            list << itemname << itemhost;

            root->appendRow(list);
            hostmap.insert(host, indexFromItem(itemname));
            //parent->appendChild(new ServerTreeItem(cdata, pass, parent));

        }
    }

    for (int i = 0; i <= netlist.count()-1; ++i) {

        if (netlist[i] == "NONE")
            continue; // The "None" network already taken care of - ignore.

        QString data = smgr.defaultServer(netlist[i]);
        QString host = data.split('|')[0];

        QStandardItem *pname = new QStandardItem(netlist[i]); // parent name
        QStandardItem *phost = new QStandardItem(host); // parent host
        QList<QStandardItem*> list;
        list << pname << phost;

        root->appendRow(list);
        hostmap.insert(host, indexFromItem(pname));

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

            QStandardItem *itemname = new QStandardItem(name); // parent name
            QStandardItem *itemhost = new QStandardItem(host); // parent host
            QList<QStandardItem*> list;
            list << itemname << itemhost;

            pname->appendRow(list);
            hostmap.insert(host, indexFromItem(itemname));
        }
    }
}

QModelIndex ServerModel::indexFromHost(QString hostname)
{
    return hostmap.value(hostname);
}
