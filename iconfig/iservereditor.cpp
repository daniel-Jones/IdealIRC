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

#include <QPoint>
#include <QMessageBox>
#include <QDebug>

#include "iservereditor.h"
#include "ui_iservereditor.h"

IServerEditor::IServerEditor(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IServerEditor),
    selNetwork("NONE")
{
    ui->setupUi(this);

    MenuNewServer.setTitle(tr("New server"));
    MenuNewServer.addAction(ui->actionNewServerNetwork);
    MenuNewServer.addAction(ui->actionNewServerNoNetwork);

    MenuNew.addAction(ui->actionNewNetwork);
    MenuNew.addSeparator();
    MenuNew.addMenu(&MenuNewServer);

    setupModelView();

    connect(selection, SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(selectionRowChanged(QModelIndex,QModelIndex)));
}

IServerEditor::~IServerEditor()
{
    delete ui;
}

void IServerEditor::on_btnNew_clicked()
{
    QPoint p(0, ui->btnNew->height());
    QPoint pos = ui->btnNew->mapToGlobal(p);

    MenuNew.popup(pos);
}

void IServerEditor::on_btnDelete_clicked()
{
    QModelIndex current = selection->currentIndex();

    if (! current.isValid()) {
        // Nothing is selected for deletion
        return;
    }

    int row = current.row();
    QModelIndex index = model.index(row, 0, current.parent());
    QString name = index.data().toString(); // Name of selected server
    QString pname = current.parent().data().toString(); // Name of parent if any (may be empty)

    if (pname.length() == 0) {
        // Either a server in NONE, or a network's default server
        if (smgr.hasNetwork(name)) {
            // Deleting a network
            QString msg = tr("Are you sure you want to delete the network '%1'?")
                    .arg(name);

            int btn = QMessageBox::question(this, tr("Delete network"), msg);

            if (btn == QMessageBox::No)
                return; // Stop

            smgr.delNetwork(name);
            model.delNetwork(name);

            return; // Nothing more to do
        }
        else {
            // Deleting a NONE server
            QString msg = tr("Are you sure you want to delete the server '%1'?")
                    .arg(name);

            int btn = QMessageBox::question(this, tr("Delete server"), msg);

            if (btn == QMessageBox::No)
                return; // Stop

            smgr.delServer(name);
            model.delServer(name);

            return; // Nothing more to do
        }
    }
    else {
        // Server is under a network parent
        QString msg = tr("Are you sure you want to delete the server '%1'?")
                .arg(name);

        int btn = QMessageBox::question(this, tr("Delete server"), msg);

        if (btn == QMessageBox::No)
            return; // Stop

        smgr.delServer(name, pname);
        model.delServer(name, pname);

        return; // Nothing more to do
    }
}

void IServerEditor::on_actionNewNetwork_triggered()
{
    QStringList netlist = smgr.networkList();

    // Generate a simple new name, like Network_2
    QString newname;

    for (int i = 0;i <= 1000; i++) {
        // it's unlikely any user would have 1000 different networks
        // named Network_0 Network_1 ...
        newname = QString("Network_%1")
                  .arg(QString::number(i));

        if (! netlist.contains(newname, Qt::CaseInsensitive))
            break;
    }

    // ---

    if (! smgr.newNetwork(newname)) {
        QMessageBox::warning(this, tr("Cannot add network"), tr("Network already exsist"));
        return;
    }

    model.addNetwork(newname, "server.name");
}

void IServerEditor::on_actionNewServerNetwork_triggered()
{
    if ((selNetwork == "NONE") || (selNetwork == "")) {
        QMessageBox::information(this, tr("No network selected"), tr("You need to select a network."));
        return;
    }

    QHash<QString,QString> serverlist = smgr.serverList(selNetwork);

    // Generate a simple new name, like Server_0
    QString newname;

    for (int i = 0;i <= 1000; i++) {
        // it's unlikely any user would have 1000 different servers
        // named Server_0 Server_1 ...
        newname = QString("Server_%1")
                  .arg(QString::number(i));

        if (! serverlist.contains(newname))
            break;
    }

    // ---

    if (! smgr.addServer(newname, "host.name:6667", "", selNetwork)) {
        QMessageBox::warning(this, tr("Cannot add server"), tr("Name already exsist"));
        return;
    }
    model.addServer(newname, "host.name:6667", selNetwork);
}

void IServerEditor::on_actionNewServerNoNetwork_triggered()
{
    QHash<QString,QString> serverlist = smgr.serverList();

    // Generate a simple new name, like Server_0
    QString newname;

    for (int i = 0;i <= 1000; i++) {
        // it's unlikely any user would have 1000 different servers
        // named Server_0 Server_1 ...
        newname = QString("Server_%1")
                  .arg(QString::number(i));

        if (! serverlist.contains(newname))
            break;
    }

    // ---

    if (! smgr.addServer(newname, "host.name:6667", "")) {
        QMessageBox::warning(this, tr("Cannot add server"), tr("Name already exsist"));
        return;
    }
    model.addServer(newname, "host.name:6667");
}

void IServerEditor::selectionRowChanged(const QModelIndex& current, const QModelIndex& previous)
{
    // Todo: This code can be shortened down.

    int row = current.row();

    if (! current.isValid()) {
        // Nothing is selected
        ui->btnSave->setEnabled(false);

        selNetwork.clear();
        selServer.clear();
        ui->edName->clear();
        ui->edPassword->clear();
        ui->edPort->setValue(6667);
        ui->edServer->clear();
        return;
    }
    else
        ui->btnSave->setEnabled(true);

    QModelIndex index = model.index(row, 0, current.parent());
    QString name = index.data().toString();
    QString pname = current.parent().data().toString();

    ui->edName->setText(name);

    if (pname.length() == 0) {
        // This indicates we either click on a network name or a server within NONE section. Check this...
        if (smgr.hasNetwork(name)) {
            // Clicked on a network, get the value of DEFAULT
            QString data = smgr.defaultServer(name);
            QString host = data.split('|')[0];
            QString pass;
            if (data.split('|').count() > 1)
                pass = data.split('|')[1];
            int port = 6667;
            if (host.split(':').count() > 1)
                port = host.split(':')[1].toInt();
            host = host.split(':')[0];

            ui->edServer->setText(host);
            ui->edPassword->setText(pass);
            ui->edPort->setValue(port);
            selNetwork = name;
            selServer = "DEFAULT";
        }
        else if (smgr.hasServer(name)) {
            // Clicked on a server in the NONE section, get the value of "name"
            QString data = smgr.getServerDetails(name);
            QString host = data.split('|')[0];
            QString pass;
            if (data.split('|').count() > 1)
                pass = data.split('|')[1];
            int port = 6667;
            if (host.split(':').count() > 1)
                port = host.split(':')[1].toInt();
            host = host.split(':')[0];

            ui->edServer->setText(host);
            ui->edPassword->setText(pass);
            ui->edPort->setValue(port);
            selNetwork = "NONE";
            selServer = name;
        }
        else {
            QMessageBox::critical(this, tr("Error"), tr("Malfunctioned servers.ini"));
            return;
        }
    }
    else {
        // This indicates we clicked inside a network parent
        QString data = smgr.getServerDetails(name, pname);
        QString host = data.split('|')[0];
        QString pass;
        if (data.split('|').count() > 1)
            pass = data.split('|')[1];
        int port = 6667;
        if (host.split(':').count() > 1)
            port = host.split(':')[1].toInt();
        host = host.split(':')[0];

        ui->edServer->setText(host);
        ui->edPassword->setText(pass);
        ui->edPort->setValue(port);
        selNetwork = pname;
        selServer = name;

    }

}

void IServerEditor::setupModelView()
{
    ui->serverView->setModel(&model);
    selection = ui->serverView->selectionModel();
}

void IServerEditor::on_btnSave_clicked()
{
    QModelIndex current = selection->currentIndex();

    if (! current.isValid()) {
        // Nothing is selected for saving
        return;
    }

    int row = current.row();
    QModelIndex index = model.index(row, 0, current.parent());
    QString name = index.data().toString(); // Name of selected server
    QString pname = current.parent().data().toString(); // Name of parent if any (may be empty)

    if (pname.length() == 0) {
        // Either a server in NONE, or a network's default server
        if (smgr.hasNetwork(name)) {
            // Editing a network's default server
            if (ui->edName->text() != name) {
                // Update network name
                model.renameNetwork(name, ui->edName->text());
                smgr.renameNetwork(name, ui->edName->text());
                name = ui->edName->text();
            }

            QString server = QString("%1:%2")
                              .arg(ui->edServer->text())
                              .arg(QString::number(ui->edPort->value()));

            smgr.addServer("DEFAULT", server, ui->edPassword->text(), name);
            model.setNetworkServer(name, server);

            return; // Nothing more to do
        }
        else {
            // Editing a NONE server
            if (ui->edName->text() != name) {
                qDebug() << "none: renaming model.";
                model.renameServer(name, ui->edName->text());
                qDebug() << "none: deleting from smgr.";
                smgr.delServer(name);
                qDebug() << "none: renamed internal name.";
                name = ui->edName->text();
            }

            QString host = QString("%1:%2")
                            .arg( ui->edServer->text() )
                            .arg( QString::number(ui->edPort->value()) );
            qDebug() << "none: name = " << name << " host =" << host;

            smgr.addServer(name, host, ui->edPassword->text());
            qDebug() << "none: added to smgr, setting model...";
            model.setServer(name, host);

            qDebug() << "none: done.";
            return; // Nothing more to do
        }
    }
    else {
        // Server is under a network parent
        if (ui->edName->text() != name) {
            model.renameServer(name, ui->edName->text(), pname);
            smgr.delServer(name, pname);
            name = ui->edName->text();
        }

        QString host = QString("%1:%2")
                        .arg( ui->edServer->text() )
                        .arg( QString::number(ui->edPort->value()) );

        smgr.addServer(name, host, ui->edPassword->text(), pname);
        model.setServer(name, host, pname);

        return; // Nothing more to do
    }
}

