#ifndef DCCRECV_H
#define DCCRECV_H

#include <QObject>
#include "dcc.h"

class DCCRecv : public DCC
{
  Q_OBJECT
public:
    explicit DCCRecv(DCCType t, IWin *parentWin, QString dcc_details);

    void initialize();
};

#endif // DCCRECV_H
