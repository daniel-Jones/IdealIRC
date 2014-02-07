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

#include <QHash>
#include <QHashIterator>
#include <QMessageBox>
#include <QDebug>

#include "iconfiggeneral.h"
#include "ui_iconfiggeneral.h"

IConfigGeneral::IConfigGeneral(config *cfg, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IConfigGeneral),
    conf(cfg),
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

    connect(selection, SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(selectionRowChanged(QModelIndex,QModelIndex)));

    // If nothing is defined in the iirc.ini, use the selected item in drop down.
    if (conf->server == ":") { // No server set

        // If drop down is empty, use "NONE" as server value. User cannot continue
        // before adding any servers.
        /*
        if (ui->servers->count() == 0)
            lbServer = "NONE";
        else {
            lbServer = ui->servers->itemData( ui->servers->currentIndex() ).toString();
            lbServer = lbServer.split('|').at(0);
        }
        */

    }

    ui->lbserver->setText(tr("Current server: %1").arg(lbServer));

}

IConfigGeneral::~IConfigGeneral()
{
    delete ui;
}

void IConfigGeneral::saveConfig()
{
    QModelIndex idx = selection->currentIndex();
    qDebug() << idx.data();

    /*
    // Generic function to save our new config to current config.

    conf->realname = ui->edRealname->text();
    conf->username = ui->edEmail->text();
    conf->nickname = ui->edNickname->text();
    conf->altnick = ui->edAltNickname->text();
    conf->checkVersion = ui->chkCheckVersion->isChecked();

    // We get our server:port|password from the current selected item
    // in our drop down.

    // If we don't have anything in the drop down, clear the config.
    if (ui->servers->count() == 0) {
        conf->server = ":"; // Empty server set
        conf->password.clear();
    }
    else {
        QString details = ui->servers->itemData( ui->servers->currentIndex() ).toString();
        QStringList splitDetail = details.split('|');
        QString server = splitDetail.at(0);
        QString password;
        if (splitDetail.length() > 1)
            password = splitDetail.at(1);

        conf->server = server;
        conf->password = password;
    }
    */
}

void IConfigGeneral::selectionRowChanged(const QModelIndex &current, const QModelIndex &previous)
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

void IConfigGeneral::on_servers_currentIndexChanged(int index)
{
    /*QVariant data = ui->servers->itemData(index);
    QString details = data.toString();

    // Omit any server password.
    QString detailsVisible = details.split("|").at(0);

    ui->lbserver->setText(tr("Current server: %1").arg(detailsVisible));
    */
}
