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

#include "iscripteditoree.h"
#include "ui_iscripteditoree.h"
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>

IScriptEditorEE::IScriptEditorEE(QWidget *parent, TScript *s) :
    QDialog(parent),
    ui(new Ui::IScriptEditorEE),
    script(s),
    ignoreNextVarChange(false)
{
    ui->setupUi(this);

    setWindowTitle("Execution Explorer - " + s->getName());

    commandModel = new QStandardItemModel(0, 2, this);
    eventModel = new QStandardItemModel(0, 2, this);
    timerModel = new QStandardItemModel(0, 2, this);

    ui->commandsView->setModel(commandModel);
    ui->eventsView->setModel(eventModel);
    ui->timersView->setModel(timerModel);

    rebuildMetaModels();

    varModel = new QStandardItemModel(0, 2, this);
    varModel->setHorizontalHeaderLabels(QStringList() << "Name" << "Value");
    ui->varView->setModel(varModel);
    ui->varView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(varModel, SIGNAL(itemChanged(QStandardItem*)),
            this, SLOT(varModelDataChange(QStandardItem*)));
    varMenu.addAction( ui->actionAdd_new );
    varMenu.addAction( ui->actionDelete );
    varMenu.addSeparator();
    varMenu.addAction( ui->actionEdit_Name );
    varMenu.addAction( ui->actionEdit_Value );

    // Begin monitoring the variables
    connect(&varUpdate, SIGNAL(timeout()),
            this, SLOT(varUpdateTimeout()));
    varUpdate.start(100); // 10 times a second.

    fnctModel = new QStandardItemModel(0, 2, this);
    ui->fnctView->setModel(fnctModel);
    rebuildFunctionModel();
}

IScriptEditorEE::~IScriptEditorEE()
{
    delete ui;
}

void IScriptEditorEE::rebuildMetaModels()
{
    commandModel->clear();
    eventModel->clear();
    timerModel->clear();

    commandModel->setHorizontalHeaderLabels(QStringList() << "Command" << "Function");
    eventModel->setHorizontalHeaderLabels(QStringList() << "Event" << "Function");
    timerModel->setHorizontalHeaderLabels(QStringList() << "Timer" << "Function");

    QHashIterator<QString,QString> ic( *script->getCommandListPtr() );
    while (ic.hasNext()) {
        ic.next();

        QStandardItem *rCmd = new QStandardItem( ic.key().toLower() );
        QStandardItem *rFnc = new QStandardItem( ic.value() );

        commandModel->appendRow( QList<QStandardItem*>() << rCmd << rFnc );
    }

    QHashIterator<e_iircevent,QString> ie( *script->getEventListPtr() );
    while (ie.hasNext()) {
        ie.next();

        QStandardItem *rEvt = new QStandardItem( TScript::getEventStr(ie.key()) );
        QStandardItem *rFnc = new QStandardItem( ie.value() );

        eventModel->appendRow( QList<QStandardItem*>() << rEvt << rFnc );
    }

    QHashIterator<QString,TTimer*> it( *script->getTimerListPtr() );
    while (it.hasNext()) {
        it.next();

        QStandardItem *rTmr = new QStandardItem( it.key().toLower() );
        QStandardItem *rFnc = new QStandardItem( it.value()->getFnct().toLower() );

        timerModel->appendRow( QList<QStandardItem*>() << rTmr << rFnc );
    }
}

void IScriptEditorEE::rebuildFunctionModel()
{
    fnctModel->clear();
    fnctModel->setHorizontalHeaderLabels(QStringList() << "Function" << "Parameters");
    ui->btnExecFnct->setEnabled(false);

    QHashIterator<QString,int> i( *script->getFnIndexPtr() );
    while (i.hasNext()) {
        i.next();

        QStandardItem *rFnc = new QStandardItem( i.key().toLower() );
        QStandardItem *rPar = new QStandardItem( script->getParamList( i.key() ) );

        fnctModel->appendRow( QList<QStandardItem*>() << rFnc << rPar );
    }
}

void IScriptEditorEE::varUpdateTimeout()
{
    QStringList updatedItems;

    // Loop through the variables
    QHashIterator<QString,QString> iv(*script->getVarListPtr());
    while (iv.hasNext()) {
        iv.next();
        updatedItems << iv.key();

        if (! varItemList.contains(iv.key())) {
            // variable isn't registered, register it and add to the view
            varEntry_t entry;
            entry.varName = new QStandardItem( iv.key().toLower() );
            entry.varData = new QStandardItem( iv.value() );
            varItemList.insert(iv.key(), entry);

            varModel->appendRow(QList<QStandardItem*>() << entry.varName << entry.varData);
        }
        else {
            // variable exist, update it if needed.
            varEntry_t entry = varItemList.value(iv.key());
            if (entry.varData->text() != iv.value()) {
                ignoreNextVarChange = true;
                entry.varData->setText( iv.value() );
                varItemList.insert(iv.key(), entry);
            }
        }
    }

    // same code as above, but with binary vars.
    QHashIterator<QString,QByteArray> ib(*script->getBinVarListPtr());
    while (ib.hasNext()) {
        ib.next();
        updatedItems << ib.key();

        if (! varItemList.contains(ib.key())) {
            varEntry_t entry;
            entry.varName = new QStandardItem( iv.key().toLower() );
            entry.varData = new QStandardItem( QString("[Binary] %1 byte").arg(iv.value().length()) );
            varItemList.insert(iv.key(), entry);

            varModel->appendRow(QList<QStandardItem*>() << entry.varName << entry.varData);
        }
        else {
            varEntry_t entry = varItemList.value(iv.key());
            if (entry.varData->text() != iv.value()) {
                ignoreNextVarChange = true;
                entry.varData->setText( QString("[Binary] %1 byte").arg(iv.value().length()) );
                varItemList.insert(iv.key(), entry);
            }
        }
    }

    // Find any rows that should be deleted- deleted variables
    QHashIterator<QString, varEntry_t> i(varItemList);
    QList<int> rowsToRemove;
    while (i.hasNext()) {
        i.next();
        if (! updatedItems.contains(i.key())) {
            // This variable is deleted- remove it from the model.
            rowsToRemove << i.value().varName->index().row();
            varItemList.remove( i.key() );
        }
    }

    QListIterator<int> ir(rowsToRemove);
    while (ir.hasNext())
        varModel->removeRow( ir.next() );
}

void IScriptEditorEE::on_fnctView_clicked(const QModelIndex &index)
{
    QStandardItem *fItem = fnctModel->item(index.row(), 0);
    QStandardItem *fPara = fnctModel->item(index.row(), 1);

    QStringList params = fPara->text().split(' ');

    ui->btnExecFnct->setEnabled(true);

    QListIterator<QLineEdit*> ie(paraEdit);
    while (ie.hasNext())
        delete ie.next();
    paraEdit.clear();

    if (params[0] == "None")
        return; // Nothing more to do.

    for (int i = params.count()-1; i >= 0; --i) {
        QLineEdit *edit = new QLineEdit(ui->fnctParamBox);
        ui->verticalLayout->insertWidget(0, edit);
        edit->setText( params[i].remove('%') );
        edit->show();
        paraEdit.push_front(edit);
    }
}

void IScriptEditorEE::on_btnFindFnct_clicked()
{

}

void IScriptEditorEE::on_btnExecFnct_clicked()
{
    QString result;
    QStringList param;
    QListIterator<QLineEdit*> i(paraEdit);
    while (i.hasNext()) {
        QString paramText = i.next()->text();
        script->externalExtract(paramText);
        param << paramText;
    }

    int row = ui->fnctView->currentIndex().row();
    if (row == -1)
        return; // Nothing was selected.

    QString fnct = fnctModel->item(row, 0)->text();
    script->runf(fnct, param, result);
}

void IScriptEditorEE::on_varView_customContextMenuRequested(const QPoint &pos)
{
    varMenu.popup( QWidget::mapToGlobal(pos) );
}

void IScriptEditorEE::on_actionAdd_new_triggered()
{
    QString name = QInputDialog::getText(this, tr("Variable name"),
                                         tr("Enter a variable name"));

    if (name.length() == 0)
        return;

    if (name[0] != '%')
        name.prepend('%');

    QHash<QString,QString> *varPtr = script->getVarListPtr();
    varPtr->insert(name, "");
}

void IScriptEditorEE::on_actionDelete_triggered()
{
    int row = ui->varView->currentIndex().row();
    if (row == -1)
        return; // Nothing was selected.

    QString name = varModel->item(row, 0)->text();

    QHash<QString,QString> *varPtr = script->getVarListPtr();
    varPtr->remove(name);
}

void IScriptEditorEE::on_actionEdit_Name_triggered()
{
    int row = ui->varView->currentIndex().row();
    if (row == -1)
        return; // Nothing was selected.

    QString oldName = varModel->item(row, 0)->text();
    QString name = QInputDialog::getText(this, tr("Change name"),
                                         tr("New variable name"),
                                         QLineEdit::Normal,
                                         oldName);


    QHash<QString,QString> *varList = script->getVarListPtr();
    QHash<QString,QByteArray> *binVarList = script->getBinVarListPtr();

    qDebug() << "oldName=" << oldName << " name=" << name;

    if (name[0] != '%')
        name.prepend('%');

    if (name == "%")
        return;

    if (varList->contains(oldName)) {
        QString val = varList->value(oldName);
        varList->remove(oldName);
        varList->insert(name, val);
    }

    else if (binVarList->contains(oldName)) {
        QByteArray val = binVarList->value(oldName);
        binVarList->remove(oldName);
        binVarList->insert(name, val);
    }
}

void IScriptEditorEE::on_actionEdit_Value_triggered()
{
    int row = ui->varView->currentIndex().row();
    if (row == -1)
        return; // Nothing was selected.

    QString name = varModel->item(row, 0)->text();
    QString oldValue = varModel->item(row, 1)->text();

    if (oldValue.split(' ')[0] == "[Binary]") {
        QMessageBox::warning(this, "Cannot edit variable", "You cannot edit binary variables",
                             QMessageBox::Ok);
        return;
    }

    bool ok = false;
    QString value = QInputDialog::getText(this, tr("Change value"),
                                         tr("Set new value for %1").arg(name),
                                         QLineEdit::Normal,
                                         oldValue,
                                          &ok);

    if (ok)
        script->getVarListPtr()->insert(name, value);
}
