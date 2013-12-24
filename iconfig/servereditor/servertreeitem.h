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

#ifndef SERVERTREEITEM_H
#define SERVERTREEITEM_H

#include <QList>
#include <QVariant>

class ServerTreeItem
{
public:
    ServerTreeItem(const QList<QVariant> &data, QString pass = "", ServerTreeItem *parent = 0);
    ~ServerTreeItem();
    void appendChild(ServerTreeItem *child);

    ServerTreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    ServerTreeItem *parent();
    QString password;

private:
    QList<ServerTreeItem*> childItems;
    QList<QVariant> itemData;
    ServerTreeItem *parentItem;
};

#endif // SERVERTREEITEM_H
