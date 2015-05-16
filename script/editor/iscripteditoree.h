#ifndef ISCRIPTEDITOREE_H
#define ISCRIPTEDITOREE_H

#include <QDialog>
#include <QHash>
#include <QStandardItemModel>
#include <QTimer>
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

private:
    Ui::IScriptEditorEE *ui;
    TScript *script;

    void rebuildMetaModels();

    QStandardItemModel *commandModel;
    QStandardItemModel *eventModel;
    QStandardItemModel *timerModel;

    QStandardItemModel *varModel;
    QHash<QString, varEntry_t> varItemList;
    QTimer varUpdate;

private slots:
    void varUpdateTimeout();
};

#endif // ISCRIPTEDITOREE_H
