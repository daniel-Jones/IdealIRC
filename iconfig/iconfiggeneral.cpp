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

    /// Reload the drop down box with servers.ini content
    reloadServerList();


    /// Load what our current server is (the label).
    QString lbServer = conf->server;

    // If nothing is defined in the iirc.ini, use the selected item in drop down.
    if (conf->server == ":") { // Empty server set

        // If drop down is empty, use "NONE" as server value. User cannot continue
        // before adding any servers.
        if (ui->servers->count() == 0)
            lbServer = "NONE";
        else {
            lbServer = ui->servers->itemData( ui->servers->currentIndex() ).toString();
            lbServer = lbServer.split('|').at(0);
        }

    }

    ui->lbserver->setText(tr("Current server: %1").arg(lbServer));

}

IConfigGeneral::~IConfigGeneral()
{
    delete ui;
}

void IConfigGeneral::saveConfig()
{
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
}

void IConfigGeneral::reloadServerList()
{
    ui->servers->clear();
    QStringList list = sm.networkList();
    QFont bold;
    bold.setBold(true);

    // The "None" network
    QHash<QString,QString> servers = sm.serverList("NONE");
    QHashIterator<QString,QString> si(servers);
    while (si.hasNext()) {
        si.next();
        ui->servers->addItem(si.key(), si.value());
    }

    int selectIndex = -1;
    QString currentDetails = conf->server;
    if (conf->password.length() > 0)
        currentDetails += '|' + conf->password;

    // Iterate every network in servers.ini:
    for (int i = 0; i <= list.count()-1; i++) {

        // Assuming we got no "default server":
        QString random = "RANDOM " + QString::number(i);

        // Name of our network:
        QString netname = list.at(i);
        // If it is the "None" network (servers that have no Network assigned to), ignore it, already taken care of.
        if (netname == "NONE")
            continue;

        // Check if this network got a "default server".
        QString defaultAddr = sm.defaultServer(netname);
        if (defaultAddr != "")
            random = defaultAddr; // Use it if we got one.

        // Add an item in dropdown. Add either RANDOM or "default irc server" to the QVariant parameter.
        ui->servers->addItem(netname + tr(" (Random server)"), random);

        int p = ui->servers->count()-1; // Position of the new item. Bottom.
        ui->servers->setItemData(p, bold, Qt::FontRole); // Our text is bold here

        // Check if it's the "default" bold text to select:
        if (random == currentDetails)
            selectIndex = ui->servers->count()-1;

        // Server name, serverhost:port|pass
        QHash<QString,QString> servers = sm.serverList(netname);
        QHashIterator<QString,QString> si(servers); // Iterate serverList
        while (si.hasNext()) {
            si.next();
            if (si.key() == "DEFAULT") // Ignore the "default irc server"
                continue;
            // Add item to drop down, with connection details as value
            QString details = si.value();
            QString detailsNoPass = details.split('|').at(0);
            ui->servers->addItem("  " + si.key(), detailsNoPass);

            // Check if it's a server to select:
            if (details == currentDetails)
                selectIndex = ui->servers->count()-1;
        }

    }

    if (selectIndex > -1)
        ui->servers->setCurrentIndex(selectIndex);

}

void IConfigGeneral::on_btnEditServers_clicked()
{
    se.show();
}

void IConfigGeneral::on_servers_currentIndexChanged(int index)
{
    QVariant data = ui->servers->itemData(index);
    QString details = data.toString();

    // Omit any server password.
    QString detailsVisible = details.split("|").at(0);

    ui->lbserver->setText(tr("Current server: %1").arg(detailsVisible));
}
