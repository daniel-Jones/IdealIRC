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
#include <QInputDialog>
#include "editor/iscripteditor.h"
#include "iscriptmanager.h"
#include "ui_iscriptmanager.h"
#include "constants.h"

IScriptManager::IScriptManager(QWidget *parent, TScriptParent *sp, config *cfg) :
    QDialog(parent),
    ui(new Ui::IScriptManager),
    scriptParent(sp),
    conf(cfg)
{
    ui->setupUi(this);

    connect(&clearLabel, SIGNAL(timeout()),
            ui->reloadLabel, SLOT(clear()));

    IniFile ini(CONF_FILE, this);

    QStringList labels;
    labels << tr("Name") << tr("Path");
    model.setHorizontalHeaderLabels(labels);

    int count = ini.CountItems("Script");
    for (int i = 1; i <= count; i++) {
        QString name = ini.ReadIniItem("Script", i);
        QString path = TScript::setRelativePath(CONF_FILE, ini.ReadIni("Script", name));

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

void IScriptManager::addItem(QString name, QString path, bool select)
{
    QStandardItem *nameItem = new QStandardItem(name);
    QStandardItem *pathItem = new QStandardItem(path);
    QList<QStandardItem*> items;
    items << nameItem << pathItem;
    model.appendRow(items);

    scriptList.insert(name, nameItem);
    if (select)
        editFile(path, name);
}

void IScriptManager::reloadLabel(QString text)
{
    clearLabel.stop();
    ui->reloadLabel->setText(text);
    clearLabel.start(5000);
}

void IScriptManager::editFile(QString filename, QString scriptname)
{
    IScriptEditor *editor = new IScriptEditor((QWidget*)this->parent(), scriptParent->getScriptPtr(scriptname), conf);
    connect(editor, SIGNAL(finished(int)),
            this, SLOT(deleteLater()));

    connect(editor, SIGNAL(reload(QString)),
            this, SLOT(reloadScript(QString)));

    editor->show();
}

void IScriptManager::on_btnReload_clicked()
{
    QModelIndex selected = selection->currentIndex();
    if (! selected.isValid())
        return;

    QModelIndex index = model.index(selected.row(), 0);
    reloadScript( index.data().toString() );
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
        //reloadLabel(tr("\"%1\" unloaded.").arg(name));
        scriptParent->echo( tr("\"%1\" unloaded.").arg(name), PT_LOCALINFO );
    }
    else
        //reloadLabel(tr("Unable to unload \"%1\".").arg(name));
        scriptParent->echo( tr("Unable to unload \"%1\".").arg(name), PT_LOCALINFO );
}

void IScriptManager::on_btnLoad_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Load script"), CONF_PATH, "IdealIRC Scripts (*.iis);;Other files (*)");
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

        //reloadLabel(tr("Loaded \"%1\".").arg(name));
        scriptParent->echo( tr("Loaded \"%1\".").arg(name), PT_LOCALINFO );
    }
    else
        //reloadLabel(tr("Unable to load \"%1\".").arg(file));
        scriptParent->echo( tr("Unable to load \"%1\".").arg(file), PT_LOCALINFO );
}

void IScriptManager::on_btnEdit_clicked()
{
    QModelIndex selected = selection->currentIndex();
    if (! selected.isValid())
        return;

    QModelIndex nameIndex = model.index(selected.row(), 0);
    QModelIndex fileIndex = model.index(selected.row(), 1);
    editFile( fileIndex.data().toString(), nameIndex.data().toString() );
}

void IScriptManager::on_btnNew_clicked()
{
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("New script..."),
                                        tr("Script name (Not file):"),
                                        QLineEdit::Normal,
                                        "", &ok);

    if (!ok)
        return;

    QString file = QFileDialog::getSaveFileName(this, tr("Save to..."), CONF_PATH, "*.iis");
    if (file.isEmpty())
        return;

    QFile f(file);
    if (! f.open(QIODevice::WriteOnly)){
        // error on opening
        QMessageBox::critical(this, tr("Cannot create file"), tr("Unable to create file"), QMessageBox::Ok);
        return;
    }

    QByteArray data;
    data.append("script " + name + " {\n");
    data.append("    command " + name + " main\n");
    data.append("}\n\n");
    data.append("function main(%arg) {\n");
    data.append("    echo Hello, world: %arg\n");
    data.append("}\n");

    f.write(data);
    f.close();

    addItem(name, file, true);
    IniFile ini(CONF_FILE);
    ini.WriteIni("Script", name, file);

}

void IScriptManager::reloadScript(QString script)
{
    if (scriptParent->reloadScript(script))
        //reloadLabel(tr("\"%1\" reloaded.").arg(name));
        scriptParent->echo( tr("Script \"%1\" reloaded.").arg(script), PT_LOCALINFO );
    else
        //reloadLabel(tr("\"%1\" failed reload.").arg(name));
        scriptParent->echo( tr("Script \"%1\" failed reload.").arg(script), PT_LOCALINFO );
}
