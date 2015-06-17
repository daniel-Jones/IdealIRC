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

/*! \class BanTableModel
 *  \brief This class is used within IChanConfig for banView, exceptionView and inviteView. Will be renamed to MaskTableModel, as that's more approperiate.
 */

#ifndef BANTABLEMODEL_H
#define BANTABLEMODEL_H

#include <QStandardItemModel>
#include <QStandardItem>
#include <QStringList>
#include <QHash>

//
class BanTableModel : public QStandardItemModel
{
    Q_OBJECT

public:
    explicit BanTableModel(QObject *parent = 0);
    void setBanList(QStringList m, QStringList d, QStringList a);
    void addBan(QString m, QString d, QString a);
    void delBan(QString m);

private:
    QStringList mask; //!< List of masks. Index is linked with date and author.
    QStringList date; //!< Lits of dates. Index is linked with mask and author.
    QStringList author; //!< List of authors. Index is linked with mask and date.
    QHash<QString,QStandardItem*> maskmap; //!< Links all hostmasks with their Item in the model.
};

#endif // BANTABLEMODEL_H
