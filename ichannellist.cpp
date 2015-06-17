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

#include "ichannellist.h"
#include "ui_ichannellist.h"

#include "iconnection.h"

IChannelList::IChannelList(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IChannelList),
    connection(NULL)
{
    ui->setupUi(this);

    ui->chanview->setModel(&model);

    reset();
}

IChannelList::~IChannelList()
{
    delete ui;
}

/*!
 * Enables Download and Join button.\n
 * Runs when we're registered with an IRC server.
 */
void IChannelList::enable()
{
    ui->btnDownload->setEnabled(true);
    ui->btnJoin->setEnabled(true);
}

/*!
 * Disables Download and Join button.\n
 * Runs when we're disconnected from an IRC server.
 */
void IChannelList::disable()
{
    ui->btnDownload->setEnabled(false);
    ui->btnJoin->setEnabled(false);
}

/*!
 * Resets the list.
 */
void IChannelList::reset()
{
    itemmap.clear();
    model.clear();

    QStandardItem *i = new QStandardItem();
    QStringList l;
    l << tr("Channel") << tr("Users") << tr("Topic");
    model.setColumnCount(3);
    model.setHorizontalHeaderItem(0, i);
    model.setHorizontalHeaderLabels(l);

    QHeaderView *header = ui->chanview->horizontalHeader();
    header->setSectionResizeMode(2, QHeaderView::Stretch);
}

/*!
 * \param channel Channel name
 * \param users Amount of users in channel
 * \param topic Topic of channel
 *
 * When downloading a /list, all items from RPL_LIST goes through here until we get RPL_LISTEND.\n
 * The exception of this would be, when this dialog doesn't show, then all contents goes to the respective status window.
 */
void IChannelList::addItem(QString channel, QString users, QString topic)
{
    QStandardItem *chanItem = new QStandardItem(channel);
    QStandardItem *usersItem = new QStandardItem(users);
    QStandardItem *topicItem = new QStandardItem(topic);

    QList<QStandardItem*> list;
    list << chanItem << usersItem << topicItem;

    itemmap.insert(channel, chanItem);
    model.appendRow(list);
}

/*!
 * Button slot for Download.
 */
void IChannelList::on_btnDownload_clicked()
{
    if (connection == NULL)
        return;

    connection->sockwrite("LIST");
}

/*!
 * Button slot for Join.
 */
void IChannelList::on_btnJoin_clicked()
{
    QItemSelectionModel *selection = ui->chanview->selectionModel();

    if (! selection->hasSelection())
        return;

    QModelIndex index = selection->selectedRows(0)[0];

    connection->sockwrite(QString("JOIN %1")
                            .arg(index.data().toString()));
}
