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

#ifndef IADDRESSLIST_H
#define IADDRESSLIST_H

#include <QDialog>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QItemSelectionModel>

/*
  to access the IAL with a GUI,
  uncomment IAddressList in the private section in iconnection.h
  uncomment its initializer in iconnection.cpp
  uncomment its show() in RPL_ENDOFMOTD in iconnection.cpp

  Remember to add them back in the idealirc.pro.
*/

class IConnection;

namespace Ui {
class IAddressList;
}

class IAddressList : public QDialog
{
    Q_OBJECT

public:
    explicit IAddressList(QWidget *parent, IConnection *c);
    ~IAddressList();

private slots:
    void on_pushButton_clicked();

private:
    Ui::IAddressList *ui;
    IConnection *connection;

    QStandardItemModel nickModel;
    QStandardItemModel chanModel;
    QItemSelectionModel *nickSelection;
    QItemSelectionModel *chanSelection;

private slots:
    void nickSelectionRowChanged(const QModelIndex &current, const QModelIndex &);
    void chanSelectionRowChanged(const QModelIndex &current, const QModelIndex &);
    void on_pushButton_2_clicked();
};

#endif // IADDRESSLIST_H
