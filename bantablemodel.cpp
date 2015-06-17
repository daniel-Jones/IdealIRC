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

#include "bantablemodel.h"

BanTableModel::BanTableModel(QObject *parent) :
    QStandardItemModel(parent)
{
    QStandardItem *i = new QStandardItem();
    QStringList l;
    l << tr("Mask") << tr("Date set") << tr("Set by");
    setColumnCount(3);
    setHorizontalHeaderItem(0, i);
    setHorizontalHeaderLabels(l);
}

/*!
 * \param m List of masks.
 * \param d List of dates.
 * \param a List of authors.
 * Used when constructing the model. Inserts all received masks.\n
 * The parameters list indexes are all linked. For example,
 * Mask 'm[0]' set on 'd[0]' by author a[0].
 */
void BanTableModel::setBanList(QStringList m, QStringList d, QStringList a)
{
    int c = m.count();
    for (int i = 0; i <= c-1; i++) {
        QStandardItem *maskItem = new QStandardItem(m[i]);
        QStandardItem *dateItem = new QStandardItem(d[i]);
        QStandardItem *authorItem = new QStandardItem(a[i]);

        QList<QStandardItem*> items;
        items << maskItem << dateItem << authorItem;

        appendRow(items);
        maskmap.insert(m[i], maskItem);
    }

}

/*!
 * \param m Mask.
 * \param d Date.
 * \param a Author.
 * Adds a mask to list if someone sets it while dialog is open.
 */
void BanTableModel::addBan(QString m, QString d, QString a)
{
    QStandardItem *maskItem = new QStandardItem(m);
    QStandardItem *dateItem = new QStandardItem(d);
    QStandardItem *authorItem = new QStandardItem(a);

    QList<QStandardItem*> items;
    items << maskItem << dateItem << authorItem;

    appendRow(items);
    maskmap.insert(m, maskItem);
}
/*!
 * \param m Mask.
 * Deletes a ban from list if someone removes it while dialog is open
 */
void BanTableModel::delBan(QString m)
{
    QStandardItem *item = maskmap.value(m);
    QModelIndex index = indexFromItem(item);
    removeRow(index.row());
}
