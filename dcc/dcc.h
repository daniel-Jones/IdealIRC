#ifndef DCC_H
#define DCC_H

#include <QObject>
#include "constants.h"

enum DCCType {
    DCC_UNKNOWN = 0,
    DCC_SEND,
    DCC_RECV, // receiving from send
    DCC_CHAT
};

class DCC : public QObject
{
    Q_OBJECT
public:
    explicit DCC(IWin *sWin, QString dcc_details, QObject *parent = 0);

    void print(QString sender, QString text, int type = PT_NORMAL);

    DCCType get_type() { return type; }
    DCCType type;
    IWin *subwin;

    QString details; // the dcc message in dcc-ctcp

    virtual void initialize() = 0; // Implement this in deriving classes.

protected:
    QString tstar;
};

#endif // DCC_H
