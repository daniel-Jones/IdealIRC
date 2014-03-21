/*
 *   IdealIRC - Internet Relay Chat client
 *   Copyright (C) 2014  Tom-Andre Barstad
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

#include "../tscript.h"
#include "constants.h"

bool TScript::runEvent(e_iircevent evt, QStringList param)
{
    QHashIterator<e_iircevent,QString> i(tevent);
    bool found = false;


    while (i.hasNext()) {
        i.next();

        e_iircevent ievt = i.key();
        QString fnct = i.value();

        if (ievt == evt) {
            // Do not break here, there might be multiple of same events.
            found = true;
            QString r;

            e_scriptresult ok = runf(fnct, param, r);
            if (ok != se_RunfDone)
                emit error( QString("Unable to run function %1 via an event (Script %2) - Parameter count?")
                            .arg(fnct)
                            .arg(name)
                           );
        }
    }

    return found;
}

e_iircevent TScript::getEvent(QString event)
{
    QString evt = event.toUpper();

    if (evt == "START")
        return te_start;

    else if (evt == "LOAD")
        return te_load;

    else if (evt == "UNLOAD")
        return te_unload;

    else if (evt == "EXIT")
        return te_exit;

    else if (evt == "CONNECT")
        return te_connect;

    else if (evt == "DISCONNECT")
        return te_disconnect;

    else if (evt == "JOIN")
        return te_join;

    else if (evt == "PART")
        return te_part;

    else if (evt == "QUIT")
        return te_quit;

    else if (evt == "MSG")
        return te_msg;

    else if (evt == "SOCKOPEN")
        return te_sockopen;

    else if (evt == "SOCKREAD")
        return te_sockread;

    else if (evt == "SOCKCLOSE")
        return te_sockclose;

    else if (evt == "SOCKERROR")
        return te_sockerror;

    else if (evt == "SOCKLISTEN")
        return te_socklisten;

    else if (evt == "MOUSEMOVE")
        return te_mousemove;

    else if (evt == "MOUSELEFTDOWN")
        return te_mouseleftdown;

    else if (evt == "MOUSELEFTUP")
        return te_mouseleftup;

    else if (evt == "MOUSEMIDDLEDOWN")
        return te_mousemiddledown;

    else if (evt == "MOUSEMIDDLEUP")
        return te_mousemiddleup;

    else if (evt == "MOUSEMIDDLEROLL")
        return te_mousemiddleroll;

    else if (evt == "MOUSERIGHTDOWN")
        return te_mouserightdown;

    else if (evt == "MOUSERIGHTUP")
        return te_mouserightup;

    else if (evt == "URLCLICK")
        return te_urlclick;

    else if (evt == "DBUTTONCLICK")
        return te_dbuttonclick;

    else if (evt == "DLISTBOXSELECT")
        return te_dlistboxselect;

    else if (evt == "IALHOSTGET")
        return te_ialhostget;

    else if (evt == "INPUT")
        return te_input;

    else
        return te_noevent;
}
