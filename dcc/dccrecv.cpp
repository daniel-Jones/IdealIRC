#include "dccrecv.h"

DCCRecv::DCCRecv(DCCType t, IWin *parentWin, QString dcc_details) :
    DCC(parentWin, dcc_details)
{
    type = t;
}

void DCCRecv::initialize()
{

}
