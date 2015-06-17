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

/*! \class DCC
 *  \brief Parent class for all DCC.
 *
 * Creating new DCC protocols, this class must be inherited.\n
 * Note that DCC isn't implemented yet, so the structure of how it works, and all can change.
 */

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
