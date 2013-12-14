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

#include "unsupportedmodel.h"
#include <QSize>

UnsupportedModel::UnsupportedModel(QString msg, QObject *parent) :
    QAbstractTableModel(parent),
    message(msg)
{
}

int UnsupportedModel::rowCount(const QModelIndex & /*parent*/) const
{
    return 0;
}

int UnsupportedModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 1;
}

QVariant UnsupportedModel::headerData(int, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole) {
            return QString(message);
        }
    }
    return QVariant();
}

QVariant UnsupportedModel::data(const QModelIndex&, int) const
{
    return QVariant();
}

