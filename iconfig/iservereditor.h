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

#ifndef ISERVEREDITOR_H
#define ISERVEREDITOR_H

#include <QDialog>
#include <QMenu>
#include <QItemSelectionModel>
#include "servereditor/servertreemodel.h"
#include "servermgr.h"

namespace Ui {
class IServerEditor;
}

class IServerEditor : public QDialog
{
    Q_OBJECT

public:
    explicit IServerEditor(QWidget *parent = 0);
    ~IServerEditor();

private slots:
    void on_btnNew_clicked();
    void on_btnDelete_clicked();
    void on_actionNewNetwork_triggered();
    void on_actionNewServerNetwork_triggered();
    void selectionRowChanged(const QModelIndex& current, const QModelIndex& previous);

private:
    Ui::IServerEditor *ui;
    QMenu MenuNew;
    QMenu MenuNewServer;
    ServerTreeModel model;
    ServerMgr smgr;
    QItemSelectionModel *selection; // Selection model for the QTreeView in UI.
    QString selNetwork; // Current network we're in
    QString selServer; // Current server name we're on

};

#endif // ISERVEREDITOR_H
