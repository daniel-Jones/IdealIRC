#include "iscripteditoree.h"
#include "ui_iscripteditoree.h"

IScriptEditorEE::IScriptEditorEE(QWidget *parent, TScript *s) :
    QDialog(parent),
    ui(new Ui::IScriptEditorEE),
    script(s)
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
    varModel->setHorizontalHeaderLabels(QStringList() << "Name" << "Data");
    ui->varView->setModel(varModel);

    // Begin monitoring the variables
    connect(&varUpdate, SIGNAL(timeout()),
            this, SLOT(varUpdateTimeout()));
    varUpdate.start(100); // 10 times a second.
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

    QHash<QString,QString>* commandList = script->getCommandListPtr();
    QHash<e_iircevent,QString>* eventList = script->getEventListPtr();
    QHash<QString,TTimer*>* timerList = script->getTimerListPtr();

    QHashIterator<QString,QString> ic(*commandList);
    while (ic.hasNext()) {
        ic.next();

        QStandardItem *rCmd = new QStandardItem( ic.key().toLower() );
        QStandardItem *rFnc = new QStandardItem( ic.value() );

        commandModel->appendRow( QList<QStandardItem*>() << rCmd << rFnc );
    }

    QHashIterator<e_iircevent,QString> ie(*eventList);
    while (ie.hasNext()) {
        ie.next();

        QStandardItem *rEvt = new QStandardItem( TScript::getEventStr(ie.key()) );
        QStandardItem *rFnc = new QStandardItem( ie.value() );

        eventModel->appendRow( QList<QStandardItem*>() << rEvt << rFnc );
    }

    QHashIterator<QString,TTimer*> it(*timerList);
    while (it.hasNext()) {
        it.next();

        QStandardItem *rTmr = new QStandardItem( it.key().toLower() );
        QStandardItem *rFnc = new QStandardItem( it.value()->getFnct().toLower() );

        timerModel->appendRow( QList<QStandardItem*>() << rTmr << rFnc );
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
            // variable exist, update it.
            varEntry_t entry = varItemList.value(iv.key());
            entry.varData->setText( iv.value() );
            varItemList.insert(iv.key(), entry);
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
            entry.varData->setText( QString("[Binary] %1 byte").arg(iv.value().length()) );
            varItemList.insert(iv.key(), entry);
        }
    }

    QHashIterator<QString, varEntry_t> i(varItemList);
    while (i.hasNext()) {
        i.next();
        if (! updatedItems.contains(i.key())) {
            // This variable is deleted- remove it from the model.
            varEntry_t entry = i.value();
            delete entry.varName;
            delete entry.varData;
        }
    }
}
