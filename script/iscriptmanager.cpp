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

#include <QDebug>
#include <inifile.h>
#include <QFileDialog>
#include <QHashIterator>
#include <QMessageBox>
#include "iscriptmanager.h"
#include "ui_iscriptmanager.h"
#include "constants.h"

IScriptManager::IScriptManager(QWidget *parent, TScriptParent *sp) :
    QDialog(parent),
    ui(new Ui::IScriptManager),
    scriptParent(sp)
{
    ui->setupUi(this);

    connect(&clearLabel, SIGNAL(timeout()),
            ui->reloadLabel, SLOT(clear()));

    IniFile ini(CONF_FILE, this);

    QStringList labels;
    labels << "Name" << "Path";
    model.setHorizontalHeaderLabels(labels);

    int count = ini.CountItems("Script");
    for (int i = 1; i <= count; i++) {
        QString name = ini.ReadIniItem("Script", i);
        QString path = ini.ReadIni("Script", name);

        addItem(name, path);
    }

    ui->tableView->setModel(&model);

    QHeaderView *header = ui->tableView->horizontalHeader();
    header->setSectionResizeMode(1, QHeaderView::Stretch);

    selection = ui->tableView->selectionModel();
}

IScriptManager::~IScriptManager()
{
    delete ui;
}

void IScriptManager::addItem(QString name, QString path)
{
    QStandardItem *nameItem = new QStandardItem(name);
    QStandardItem *pathItem = new QStandardItem(path);
    QList<QStandardItem*> items;
    items << nameItem << pathItem;
    model.appendRow(items);

    scriptList.insert(name, nameItem);
}

void IScriptManager::reloadLabel(QString text)
{
    clearLabel.stop();
    ui->reloadLabel->setText(text);
    clearLabel.start(5000);
}

void IScriptManager::on_btnReload_clicked()
{
    QModelIndex selected = selection->currentIndex();
    if (! selected.isValid())
        return;

    QModelIndex index = model.index(selected.row(), 0);
    QString name = index.data().toString();

    if (scriptParent->reloadScript(name))
        reloadLabel(tr("\"%1\" reloaded.").arg(name));
    else
        reloadLabel(tr("\"%1\" failed reload.").arg(name));
}

void IScriptManager::on_btnDelete_clicked()
{
    QModelIndex selected = selection->currentIndex();
    if (! selected.isValid())
        return;

    QModelIndex index = model.index(selected.row(), 0);
    QString name = index.data().toString();

    int btn = QMessageBox::question(this, tr("Unload script"),
                                    tr("Are you sure you want to unload \"%1\"?")
                                      .arg(name));

    if (btn == QMessageBox::No)
        return;

    if (scriptParent->unloadScript(name)) {
        IniFile ini(CONF_FILE);
        ini.DelIni("Script", name);

        scriptList.remove(name);
        model.removeRow(selected.row());
        reloadLabel(tr("\"%1\" unloaded.").arg(name));
    }
    else
        reloadLabel(tr("Unable to unload \"%1\".").arg(name));
}

void IScriptManager::on_btnLoad_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, "Load script", CONF_PATH, "IdealIRC Scripts (*.iis);;Other files (*)");
    if (file.isEmpty())
        return;

    if (scriptParent->loadScript(file)) {
        QString name;
        QHash<QString,QString> list;
        scriptParent->getLoadedScripts(list);
        QHashIterator<QString,QString> i(list);
        while (i.hasNext()) {
            i.next();
            name = i.key();
            QString ifile = i.value();
            if (ifile == file) {
                addItem(name, ifile);
                IniFile ini(CONF_FILE);
                ini.WriteIni("Script", name, ifile);
                break;
            }
        }

        reloadLabel(tr("Loaded \"%1\".").arg(name));
    }
    else
        reloadLabel(tr("Unable to load \"%1\".").arg(file));
}
