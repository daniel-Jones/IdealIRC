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

#include "iaddresslist.h"
#include "ui_iaddresslist.h"

#include "iconnection.h"

#include <QDebug>

IAddressList::IAddressList(QWidget *parent, IConnection *c) :
    QDialog(parent),
    ui(new Ui::IAddressList),
    connection(c)
{
    ui->setupUi(this);

    ui->nickView->setModel(&nickModel);
    ui->chanView->setModel(&chanModel);

    nickSelection = ui->nickView->selectionModel();
    chanSelection = ui->chanView->selectionModel();

    connect(nickSelection, SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(nickSelectionRowChanged(QModelIndex,QModelIndex)));

    connect(chanSelection, SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(chanSelectionRowChanged(QModelIndex,QModelIndex)));
}

IAddressList::~IAddressList()
{
    delete ui;
}

void IAddressList::on_pushButton_clicked()
{
    nickModel.clear();

    QStringList *list = connection->ial.getAllNicknames();
    for (int i = 0; i <= list->count()-1; ++i) {
        QStandardItem *item = new QStandardItem(list->at(i));
        nickModel.appendRow(item);
    }

    delete list;
}

void IAddressList::nickSelectionRowChanged(const QModelIndex &current, const QModelIndex &)
{
    chanModel.clear();
    QString nickname = current.data().toString();

    QStringList *list = connection->ial.getChannels(nickname);
    for (int i = 0; i <= list->count()-1; ++i) {
        QStandardItem *item = new QStandardItem(list->at(i));
        chanModel.appendRow(item);
    }

    ui->ident->setText( connection->ial.getIdent(nickname) );
    ui->host->setText( connection->ial.getHost(nickname) );

    delete list;
}

void IAddressList::chanSelectionRowChanged(const QModelIndex &current, const QModelIndex &)
{
    QString channel = current.data().toString();
    QString nickname = nickSelection->currentIndex().data().toString();
    QList<char> modeChars = connection->ial.getModeChars(nickname, channel);

    QString text;
    for (int i = 0; i <= modeChars.length()-1; ++i)
        text += modeChars[i];

    ui->modes->setText(text);
}

void IAddressList::on_pushButton_2_clicked()
{
    QStringList *list = connection->ial.getNickList( ui->lineEdit->text() );

    qDebug() << "NICKLIST OF" << ui->lineEdit->text();

    for (int i = 0; i <= list->count()-1; ++i)
        qDebug() << "  " << list->at(i);

    qDebug () << "EOL.";

    delete list;
}
