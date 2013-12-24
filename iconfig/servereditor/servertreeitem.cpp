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

#include "servertreeitem.h"

ServerTreeItem::ServerTreeItem(const QList<QVariant> &data, QString pass, ServerTreeItem *parent) :
    password(pass),
    itemData(data),
    parentItem(parent)
{
}

ServerTreeItem::~ServerTreeItem()
{
    qDeleteAll(childItems);
}

void ServerTreeItem::appendChild(ServerTreeItem *item)
{
    childItems.append(item);
}

ServerTreeItem *ServerTreeItem::child(int row)
{
    return childItems.value(row);
}

int ServerTreeItem::childCount() const
{
    return childItems.count();
}

int ServerTreeItem::columnCount() const
{
    return itemData.count();
}

QVariant ServerTreeItem::data(int column) const
{
    return itemData.value(column);
}

ServerTreeItem *ServerTreeItem::parent()
{
    return parentItem;
}

int ServerTreeItem::row() const
{
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<ServerTreeItem*>(this));

    return 0;
}
