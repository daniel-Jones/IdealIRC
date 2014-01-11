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

    ui->serverView->setModel(&model);
    selection = ui->serverView->selectionModel();

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
    smgr.newNetwork(newname);
}

void IServerEditor::on_actionNewServerNetwork_triggered()
{
    /*
    qDebug() << "---";
    qDebug() << "Current network=" << selNetwork;
    qDebug() << "Current server=" << selServer;
    qDebug() << "---";
*/


}

void IServerEditor::selectionRowChanged(const QModelIndex& current, const QModelIndex& previous)
{
    int row = current.row();

    QModelIndex index = model.index(row, 0, current.parent());
    QString name = index.data().toString();
    QString pname = current.parent().data().toString();

    ui->edName->setText(name);

    if (pname.length() == 0) {
        // This indicates we either click on a network name or a server within NONE section. Check this...
        if (smgr.hasNetwork(name)) {
            // Clicked on a network, get the value of DEFAULT
            QString data = smgr.defaultServer(name);
            ui->edServer->setText(data);
            selNetwork = name;
            selServer = "DEFAULT";
        }
        else if (smgr.hasServer(name)) {
            // Clicked on a server in the NONE section, get the value of "name"
            QString data = smgr.getServerDetails(name);
            ui->edServer->setText(data);
            selNetwork = "NONE";
            selServer = name;
        }
        else {
            QMessageBox::warning(this, tr("Error"), tr("Malfunctioned servers.ini"));
            return;
        }
    }
    else {
        // This indicates we clicked inside a network parent
        QString data = smgr.getServerDetails(name, pname);
        ui->edServer->setText(data);
        selNetwork = pname;
        selServer = name;

    }
}

