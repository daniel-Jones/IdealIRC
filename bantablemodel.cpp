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

#include "bantablemodel.h"

BanTableModel::BanTableModel(QObject *parent) :
    QAbstractTableModel(parent)
{

}
void BanTableModel::setBanList(QStringList m, QStringList d, QStringList a)
{
    mask = m;
    date = d;
    author = a;
    emit updatedItems();
}

void BanTableModel::addBan(QString m, QString d, QString a)
{
    mask << m;
    date << d;
    author << a;

    emit updatedItems();
}

void BanTableModel::delBan(QString m)
{
    int idx = mask.indexOf(m);


    mask.removeAt(idx);
    date.removeAt(idx);
    author.removeAt(idx);

    emit updatedItems();
}

int BanTableModel::rowCount(const QModelIndex & /*parent*/) const
{
   return mask.count();
}

int BanTableModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 3;
}

QVariant BanTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Horizontal) {
            switch (section)
            {
            case 0:
                return QString("Mask");
            case 1:
                return QString("Date set");
            case 2:
                return QString("Set by");
            }
        }

        if (orientation == Qt::Vertical) {
            return section+1;
        }

    }
    return QVariant();
}

QVariant BanTableModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        int col = index.column();
        int row = index.row();

        switch (col) {
            case 0:
                return mask[row];
            case 1:
                return date[row];
            case 2:
                return author[row];
        }

    }
    return QVariant();
}

