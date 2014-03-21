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

bool TScript::runCommand(QString cmd)
{
    if (cmd.isEmpty())
        return false;

    if (cmd.left(1) == "/") // Remove a possible / at beginning
        cmd = cmd.mid(1);

    QString c = cmd.split(' ')[0]; // contains the command

    if (command.contains(c.toUpper()) == false) // checks if 'c' is an actual registered command
        return false;

    QStringList token = cmd.split(' ');

    QStringList param;
    if (token.length() > 1) // if there's parameters we should add them.
        param = cmd.mid(c.length()+1).split(' ');

    QString fn = command.value(c.toUpper());

    // It's safe to ignore param count when running from a command.
    QString r;

    #ifdef IIRC_DEBUG_SCRIPT
    qDebug() << "runCommand(" << cmd << ")";
    qDebug() << " - params: " << param;
    #endif

    e_scriptresult res = runf(fn, param, r, true);
    errorHandler(res);

    bool ok = true;
    if (res != se_RunfDone)
        ok = false;

    return ok;
}

bool TScript::hasCommand(QString cmd)
{
    QString c = cmd.split(' ')[0];
    if (c.left(1) == "/")
        c = c.mid(1);

    #ifdef IIRC_DEBUG_SCRIPT
    qDebug() << "hasCommand(" << cmd << ")";
    #endif

    return command.contains(c);
}
