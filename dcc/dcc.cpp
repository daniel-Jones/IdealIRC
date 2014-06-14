#include "dcc.h"

#include "iwin.h"

DCC::DCC(IWin *sWin, QString dcc_details, QObject *parent) :
    QObject(parent),
    subwin(sWin),
    details(dcc_details)
{
}

void DCC::print(QString text, int type)
{
    subwin->print(text, type);
}
