#include "dcc.h"

#include "iwin.h"

DCC::DCC(IWin *sWin, QString dcc_details, QObject *parent) :
    QObject(parent),
    subwin(sWin),
    details(dcc_details),
    tstar("***")
{
}

void DCC::print(QString sender, QString text, int type)
{
    subwin->print(sender, text, type);
}
