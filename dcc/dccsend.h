#ifndef DCCSEND_H
#define DCCSEND_H

#include <QObject>
#include "dcc.h"

class DCCSend : public DCC
{
  Q_OBJECT
public:
    explicit DCCSend(DCCType t, IWin *parentWin, QString dcc_details);

    void initialize();
};

#endif // DCCSEND_H
