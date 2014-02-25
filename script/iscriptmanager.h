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

#ifndef ISCRIPTMANAGER_H
#define ISCRIPTMANAGER_H

#include <QDialog>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QItemSelectionModel>
#include <QHash>
#include <QTimer>
#include "script/tscriptparent.h"

namespace Ui {
class IScriptManager;
}

class IScriptManager : public QDialog
{
    Q_OBJECT

public:
    explicit IScriptManager(QWidget *parent, TScriptParent *sp);
    ~IScriptManager();

private slots:
    void on_btnReload_clicked();

    void on_btnDelete_clicked();

    void on_btnLoad_clicked();

private:
    Ui::IScriptManager *ui;
    TScriptParent *scriptParent;
    QStandardItemModel model;
    QItemSelectionModel *selection;
    QHash<QString,QStandardItem*> scriptList;
    QTimer clearLabel;
    void addItem(QString name, QString path);
    void reloadLabel(QString text);
};

#endif // ISCRIPTMANAGER_H
