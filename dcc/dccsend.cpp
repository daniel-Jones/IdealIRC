#include "dccsend.h"

DCCSend::DCCSend(DCCType t, IWin *parentWin, QString dcc_details) :
    DCC(parentWin, dcc_details)
{
    type = t;
}

void DCCSend::initialize()
{

}
