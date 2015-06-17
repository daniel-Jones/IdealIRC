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

/*! \class IScriptManager
 *  \brief Dialog to manage loaded scripts.
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
#include "config.h"

namespace Ui {
class IScriptManager;
}

class IScriptManager : public QDialog
{
    Q_OBJECT

public:
    explicit IScriptManager(QWidget *parent, TScriptParent *sp, config *cfg);
    ~IScriptManager();

private slots:
    void on_btnDelete_clicked();
    void on_btnLoad_clicked();
    void on_btnEdit_clicked();
    void on_btnNew_clicked();

private:
    Ui::IScriptManager *ui; //!< Qt Creator generated GUI class.
    TScriptParent *scriptParent; //!< Pointer to the script parent.
    QStandardItemModel model; //!< Model for the table of loaded scripts.
    QItemSelectionModel *selection; //!< Keeps track of what's selected on the view.
    QHash<QString,QStandardItem*> scriptList; //!< List of all loaded scripts.\n Key: Script name\n Value: Item from first row.
    QTimer clearLabel;
    config *conf; //!< Pointer to the config class (iirc.ini)
    void addItem(QString name, QString path, bool edit = false);
    void editFile(QString filename, QString scriptname);
};

#endif // ISCRIPTMANAGER_H
