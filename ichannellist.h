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


/*! \class IChannelList
 *  \brief Simple dialog to list all (visible) IRC server channels.
 */

#ifndef ICHANNELLIST_H
#define ICHANNELLIST_H

#include <QDialog>
#include <QStandardItemModel>
#include <QHash>

class IConnection;

namespace Ui {
class IChannelList;
}

class IChannelList : public QDialog
{
    Q_OBJECT

public:
    explicit IChannelList(QWidget *parent = 0);
    ~IChannelList();
    void setConnection(IConnection *cptr) { connection = cptr; }
    void enable();
    void disable();
    void reset();
    void addItem(QString channel, QString users, QString topic);

private slots:
    void on_btnDownload_clicked();
    void on_btnJoin_clicked();


private:
    Ui::IChannelList *ui; //!< Qt Creator generated GUI class.
    IConnection *connection; //!< The IRC connection this dialog is bound to.
    QStandardItemModel model; //!< A model which lists all channels.
    QHash<QString,QStandardItem*> itemmap; //!< Maps all channels with their respective item in the model.
};

#endif // ICHANNELLIST_H
