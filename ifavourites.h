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

#ifndef IFAVOURITES_H
#define IFAVOURITES_H

#include <QDialog>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#include <QHash>

#include "iconnection.h"
#include "config.h"

namespace Ui {
class IFavourites;
}

class IFavourites : public QDialog
{
    Q_OBJECT

public:
    explicit IFavourites(config *cfg, QWidget *parent = 0);
    void enableJoin(bool ok);
    void setConnection(IConnection *c) { current = c; }
    ~IFavourites();

private slots:
    void selectionRowChanged(const QModelIndex &current, const QModelIndex &previous);
    void itemChanged(QStandardItem *item);
    void on_edChannel_textChanged(const QString &arg1);
    void on_btnSave_clicked();
    void on_toolButton_clicked();
    void on_btnDelete_clicked();

    void on_btnJoin_clicked();

private:
    Ui::IFavourites *ui;
    config *conf;
    IniFile *ini;
    QStandardItemModel model;
    QItemSelectionModel *selection;
    QHash<QString,QStandardItem*> chanmap;
    IConnection *current;
    void loadChannel(const QString &channel);

};

#endif // IFAVOURITES_H
