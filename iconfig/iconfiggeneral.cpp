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

#include <QHash>
#include <QHashIterator>
#include <QMessageBox>
#include <QDebug>

#include "iconfiggeneral.h"
#include "ui_iconfiggeneral.h"

IConfigGeneral::IConfigGeneral(config *cfg, IConnection *con, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IConfigGeneral),
    conf(cfg),
    current(con),
    sm(this)
{
    ui->setupUi(this);

    /// Set our details from the current config.
    ui->edRealname->setText( conf->realname );
    ui->edEmail->setText( conf->username );
    ui->edNickname->setText( conf->nickname );
    ui->edAltNickname->setText( conf->altnick );

    ui->chkCheckVersion->setChecked( conf->checkVersion );

    /// Load what our current server is (the label).
    QString lbServer = conf->server;

    ui->serverView->setModel(&serverModel);
    selection = ui->serverView->selectionModel();

    QModelIndex idx = serverModel.indexFromHost(lbServer);
    selection->setCurrentIndex(idx, QItemSelectionModel::Select | QItemSelectionModel::Rows);


    // If we're connected, show active nickname and server
    if (con != NULL) {
        if (con->isOnline()) {
            ui->edNickname->setText( con->getActiveNickname() );

            selection->clear();
            QModelIndex idx = serverModel.indexFromHost( con->getConnectionInfo() );
            selection->setCurrentIndex(idx, QItemSelectionModel::Select | QItemSelectionModel::Rows);

            lbServer = con->getConnectionInfo();
        }
    }


    connect(selection, SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(selectionRowChanged(QModelIndex,QModelIndex)));

    ui->lbserver->setText(tr("Current server: %1").arg(lbServer));

    connect(&se, SIGNAL(closed()),
             this, SLOT(reloadServerList()));

}

IConfigGeneral::~IConfigGeneral()
{
    delete ui;
}

/*!
 * Stores all changes to config class.
 * Does not save to iirc.ini.
 */
void IConfigGeneral::saveConfig()
{
    // Save generic info
    conf->realname = ui->edRealname->text();
    conf->username = ui->edEmail->text();
    conf->nickname = ui->edNickname->text();
    conf->altnick = ui->edAltNickname->text();
    conf->checkVersion = ui->chkCheckVersion->isChecked();

    // Save connection info
    QModelIndex current = selection->currentIndex();
    if (! current.isValid()) {
        conf->server = ":"; // Empty server set
        conf->password.clear();
        return; // Nothing was selected
    }

    QModelIndex idx = serverModel.index(current.row(), 0, current.parent());
    QModelIndex idx2 = serverModel.index(current.row(), 1, current.parent());

    QString servername = idx.data().toString();
    QString serverhost = idx2.data().toString();
    QString serverpass;

    // Retreive password...
    if (! idx.parent().isValid()) {
        // Either in NONE or it's a Network parent
        if (sm.hasNetwork(servername)) {
            // Network is named this, we selected a Network parent.
            serverpass = sm.defaultServer(servername);
        }
        else {
            // Selection is in NONE section
            serverpass = sm.getServerDetails(servername);
        }
    }
    else {
        // Childed item under a network
        serverpass = sm.getServerDetails(idx.parent().data().toString(), servername);
    }

    if (serverpass.split('|').count() > 1)
        serverpass = serverpass.split('|')[1];
    else
        serverpass.clear();

    conf->server = serverhost;
    conf->password = serverpass;
}

/*!
 * \param current Index of selected row
 *
 * This function runs when a server was chosen in the list.
 */
void IConfigGeneral::selectionRowChanged(const QModelIndex &current, const QModelIndex &)
{
    // Todo: This code can be shortened down.

    int row = current.row();

    QModelIndex index = serverModel.index(row, 0, current.parent());
    QString name = index.data().toString();
    QString pname = current.parent().data().toString();

    if (pname.length() == 0) {
        // This indicates we either click on a network name or a server within NONE section. Check this...
        if (sm.hasNetwork(name)) {
            // Clicked on a network, get the value of DEFAULT
            QString data = sm.defaultServer(name);
            QString host = data.split('|')[0];
            QString pass;
            if (data.split('|').count() > 1)
                pass = data.split('|')[1];
            QString port = "6667";
            if (host.split(':').count() > 1)
                port = host.split(':')[1];
            host = host.split(':')[0];

            QString text = QString("Current server: %1:%2")
                            .arg(host)
                            .arg(port);
            ui->lbserver->setText(text);
        }
        else if (sm.hasServer(name)) {
            // Clicked on a server in the NONE section, get the value of "name"
            QString data = sm.getServerDetails(name);
            QString host = data.split('|')[0];
            QString pass;
            if (data.split('|').count() > 1)
                pass = data.split('|')[1];
            QString port = "6667";
            if (host.split(':').count() > 1)
                port = host.split(':')[1];
            host = host.split(':')[0];

            QString text = QString("Current server: %1:%2")
                            .arg(host)
                            .arg(port);
            ui->lbserver->setText(text);
        }
        else {
            QMessageBox::critical(this, tr("Error"), tr("Malfunctioned servers.ini"));
            return;
        }
    }
    else {
        // This indicates we clicked inside a network parent
        QString data = sm.getServerDetails(name, pname);
        QString host = data.split('|')[0];
        QString pass;
        if (data.split('|').count() > 1)
            pass = data.split('|')[1];
        QString port = "6667";
        if (host.split(':').count() > 1)
            port = host.split(':')[1];
        host = host.split(':')[0];

        QString text = QString("Current server: %1:%2")
                        .arg(host)
                        .arg(port);
        ui->lbserver->setText(text);

    }
}

void IConfigGeneral::on_btnEditServers_clicked()
{
    se.show();
}

/*!
 * Resets the model and re-builds it with servers.
 */
void IConfigGeneral::reloadServerList()
{
    serverModel.resetModel();

    /// Load what our current server is (the label).
    QString lbServer = conf->server;

    ui->serverView->setModel(&serverModel);
    selection = ui->serverView->selectionModel();

    QModelIndex idx = serverModel.indexFromHost(lbServer);
    selection->setCurrentIndex(idx, QItemSelectionModel::Select | QItemSelectionModel::Rows);

}
