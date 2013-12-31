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

#include "iservereditor.h"
#include "ui_iservereditor.h"

IServerEditor::IServerEditor(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IServerEditor)
{
    ui->setupUi(this);

    MenuNewServer.setTitle("New server");
    MenuNewServer.addAction(ui->actionNewServerNetwork);
    MenuNewServer.addAction(ui->actionNewServerNoNetwork);

    MenuNew.addAction(ui->actionNewNetwork);
    MenuNew.addSeparator();
    MenuNew.addMenu(&MenuNewServer);

    ui->serverView->setModel(&model);
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

    if (! smgr.newNetwork(newname))
        QMessageBox::warning(this, tr("Cannot add network"), tr("Network already exsist"));

}

void IServerEditor::on_actionNewServerNetwork_triggered()
{
    const QItemSelectionModel *sel = ui->serverView->selectionModel();


}
