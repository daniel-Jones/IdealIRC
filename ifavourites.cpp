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

#include "ifavourites.h"
#include "ui_ifavourites.h"

#include <QHashIterator>
#include <QDebug>
#include <QMessageBox>

#include "inifile.h"
#include "constants.h"

IFavourites::IFavourites(config *cfg, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IFavourites),
    conf(cfg),
    current(NULL)
{
    ui->setupUi(this);

    QString fn = QString("%1/favourites.ini").arg(CONF_PATH);
    ini = new IniFile(fn);

    int count = ini->CountSections();
    for (int i = 1; i <= count; i++) {
        QString name = ini->ReadIni(i);

        QStandardItem *item = new QStandardItem(name);
        model.appendRow(item);
        chanmap.insert(name.toUpper(), item);
    }

    ui->chanView->setModel(&model);

    selection = ui->chanView->selectionModel();

    connect(selection, SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(selectionRowChanged(QModelIndex,QModelIndex)));

    connect(&model, SIGNAL(itemChanged(QStandardItem*)),
            this, SLOT(itemChanged(QStandardItem*)));

}

IFavourites::~IFavourites()
{
    QHashIterator<QString,QStandardItem*> i(chanmap);
    while (i.hasNext()) {
        i.next();
        delete i.value();
    }
    chanmap.clear();

    disconnect(selection);

    ini->deleteLater();
    selection->deleteLater();
    delete ui;
}

/*!
 * \param ok Set to true to enable Join button.
 *
 * Enables the Join button.
 */
void IFavourites::enableJoin(bool ok)
{
    ui->btnJoin->setEnabled(ok);
}

/*!
 * \param current New index
 *
 * This function runs when there's a change in the selection in the listbox.
 */
void IFavourites::selectionRowChanged(const QModelIndex &current, const QModelIndex&)
{
    if (! current.isValid())
        return;

    QString channel = current.data().toString();

    ui->edChannel->setText(channel);
    loadChannel(channel);
}

/*!
 * \param item Item in the model
 *
 * This function runs when an item changed its text.
 */
void IFavourites::itemChanged(QStandardItem *item)
{
    QString mapname = chanmap.key(item);
    QString newname = item->text();

    chanmap.remove(mapname);
    chanmap.insert(newname.toUpper(), item);

    ini->RenameSection(mapname, newname);
}

/*!
 * \param arg1 Text
 *
 * This function runs when the text in textbox changed.\n
 * Used to search for channels in the listbox.\n
 * When a channel matches, it'll select the channel.
 */
void IFavourites::on_edChannel_textChanged(const QString &arg1)
{
    QStandardItem *item = chanmap.value(arg1.toUpper(), NULL);

    selection->clearSelection();

    if (item == NULL) {
        ui->btnSave->setEnabled(false);
        ui->btnDelete->setEnabled(false);
        ui->chkJoin->setChecked(false);
        ui->edKey->clear();
        return;
    }

    selection->select( model.indexFromItem(item), QItemSelectionModel::Select );
    loadChannel( item->text() );
}

void IFavourites::loadChannel(const QString &channel)
{

    bool aj = (bool)ini->ReadIni(channel, "AutoJoin").toInt();
    QString key = ini->ReadIni(channel, "Key");

    ui->chkJoin->setChecked(aj);
    ui->edKey->setText(key);
    ui->btnSave->setEnabled(true);
    ui->btnDelete->setEnabled(true);
}

void IFavourites::on_btnSave_clicked()
{
    QString channel = ui->edChannel->text();
    QStandardItem *item = chanmap.value(channel.toUpper(), NULL);

    if (item == NULL)
        return;

    QString aj = QString::number( ui->chkJoin->isChecked() );
    QString key = ui->edKey->text();

    ini->WriteIni(channel, "AutoJoin", aj);
    ini->WriteIni(channel, "Key", key);
}

void IFavourites::on_toolButton_clicked()
{
    QString channel = ui->edChannel->text();
    QStandardItem *item = chanmap.value(channel.toUpper(), NULL);

    if (item != NULL) {
        QMessageBox::warning( this, tr("Channel exists"),
                              tr("The channel '%1' is already in the list.")
                                .arg(channel)
                             );
        return;
    }



    ini->WriteIni(channel, "AutoJoin", "0");

    item = new QStandardItem(channel);
    chanmap.insert(channel.toUpper(), item);

    model.appendRow(item);

    QModelIndex index = model.indexFromItem(item);
    selection->clearSelection();
    selection->select(index, QItemSelectionModel::Select);

    loadChannel(channel);
}

void IFavourites::on_btnDelete_clicked()
{
    QString channel = ui->edChannel->text();
    QStandardItem *item = chanmap.value(channel.toUpper(), NULL);

    if (item == NULL)
        return; // Weirdingly, item doesn't exist, stop anyways.

    int button = QMessageBox::question( this, tr("Confirm delete"),
                                        tr("Do you want to delete channel '%1'?")
                                          .arg(channel)
                                       );
    if (button == QMessageBox::No)
        return;

    QModelIndex index = model.indexFromItem(item);
    model.removeRow(index.row());
    chanmap.remove(channel.toUpper());

    ini->DelSection(channel);

    if (chanmap.count() == 0) {
        ui->btnSave->setEnabled(false);
        ui->btnDelete->setEnabled(false);
    }
}

void IFavourites::on_btnJoin_clicked()
{
    if (current == NULL)
        return;

    QString channel = ui->edChannel->text();

    if (channel.length() == 0)
        return;

    QString pw = ini->ReadIni(channel,"Key");
    if (pw.length() > 0)
        pw.prepend(' ');

    current->sockwrite( QString("JOIN %1%2")
                          .arg(channel)
                          .arg(pw)
                       );
}
