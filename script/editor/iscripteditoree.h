/*
 *   IdealIRC - Internet Relay Chat client
 *   Copyright (C) 2015  Tom-Andre Barstad
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

/*! \class IScriptEditorEE
 *  \brief The script editor's Execution Explorer.
 *
 * This will display all events, commands, timers, variables and functions.\n
 * You're able to manually execute functions and edit variables on-the-fly.
 */

#ifndef ISCRIPTEDITOREE_H
#define ISCRIPTEDITOREE_H

#include <QDialog>
#include <QHash>
#include <QStandardItemModel>
#include <QTimer>
#include <QLineEdit>
#include <QMenu>
#include "script/tscript.h"

namespace Ui {
class IScriptEditorEE;
}

typedef struct {
    QStandardItem *varName;
    QStandardItem *varData;
} varEntry_t;

class IScriptEditorEE : public QDialog
{
    Q_OBJECT

public:
    explicit IScriptEditorEE(QWidget *parent, TScript *s);
    ~IScriptEditorEE();
    void rebuildMetaModels();
    void rebuildFunctionModel();

private:
    Ui::IScriptEditorEE *ui; //!< Qt Creator generated GUI class.
    TScript *script; //!< Pointer to the script we're looking at.

    // meta-data models
    QStandardItemModel *commandModel; //!< Model that contains all commands.
    QStandardItemModel *eventModel; //!< Model that contains all events.
    QStandardItemModel *timerModel; //!< Model that contains all timers.

    // variable model + internal lookup store
    QStandardItemModel *varModel; //!< Model that contains all variables.
    QHash<QString, varEntry_t> varItemList; //!< Copy of all variables. Binary ones only display their size.
    QTimer varUpdate; //!< Timer to copy an update of all variables.
    QMenu varMenu; //!< Context menu for variable view.
    bool ignoreNextVarChange;

    // function model
    QStandardItemModel *fnctModel; //!< Model that contains all functions.
    QList<QLineEdit*> paraEdit; //!< List of parameter input widgets.

private slots:
    void varUpdateTimeout();
    void on_fnctView_clicked(const QModelIndex &index);
    void on_btnFindFnct_clicked();
    void on_btnExecFnct_clicked();
    void on_varView_customContextMenuRequested(const QPoint &pos);
    void on_actionAdd_new_triggered();
    void on_actionDelete_triggered();
    void on_actionEdit_Name_triggered();
    void on_actionEdit_Value_triggered();

signals:
    void findFunction(QString name);
};

#endif // ISCRIPTEDITOREE_H
