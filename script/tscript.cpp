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

#include <QFile>
#include <iostream>
#include <QHashIterator>
#include <QListIterator>
#include <QVector>
#include <QDebug>
#include <QMenu>

#include "tscriptparent.h"
#include "tscript.h"
#include "iconnection.h"

#include "tscript/utils.cpp"
#include "tscript/menu.cpp"
#include "tscript/loadscript.cpp"
#include "tscript/extracters.cpp"
#include "tscript/solvers.cpp"
#include "tscript/events.cpp"
#include "tscript/runf.cpp"
#include "tscript/commands.cpp"
#include "tscript/containers.cpp"
#include "tscript/dialogs.cpp"

TScript::TScript(QObject *parent, TScriptParent *sp, QWidget *dialogParent, QString fname,
                 QHash<int,IConnection*> *cl, QHash<int,subwindow_t> *wl, int *aWid, int *aConn) :
    QObject(parent),
    dlgParent(dialogParent),
    scriptParent(sp),
    ifn(&sockets, &fnindex, &dialogs, &files, cl, wl, aWid, aConn),
    filename(fname),
    rootNicklistMenu(NULL),
    rootChannelMenu(NULL),
    rootQueryMenu(NULL),
    rootStatusMenu(NULL),
    activeWid(aWid),
    activeConn(aConn),
    winList(wl),
    conList(cl)
{
    // Set file relative to config file
    filename = setRelativePath(CONF_FILE, filename);

    connect(&sockets, SIGNAL(runEvent(e_iircevent,QStringList)),
            this, SLOT(runEvent(e_iircevent,QStringList)));


    connect(&nicklistMenuMapper, SIGNAL(mapped(QString)),
            this, SLOT(nicklistMenuItemTriggered(QString)));

    connect(&channelMenuMapper, SIGNAL(mapped(QString)),
            this, SLOT(channelMenuItemTriggered(QString)));

    connect(&queryMenuMapper, SIGNAL(mapped(QString)),
            this, SLOT(queryMenuItemTriggered(QString)));

    connect(&statusMenuMapper, SIGNAL(mapped(QString)),
            this, SLOT(statusMenuItemTriggered(QString)));
}

config* TScript::getConfPtr()
{
    return scriptParent->getConfPtr();
}

TScriptParent* TScript::getScriptParent()
{
    return scriptParent;
}

QWidget* TScript::getDlgParent()
{
    return dlgParent;
}

void TScript::timerTimeout(QString fn)
{
    QStringList par;
    QString r;
    runf(fn, par, r);
}

void TScript::errorHandler(e_scriptresult res)
{

    switch (res) {

        case se_None:
        break;

        case se_RunfDone:
        break;

        case se_InvalidParamCount:
            emit error( tr("Invalid parameter count around '%1' at %2")
                          .arg(errorKeyword)
                          .arg(lm(curLine))
                       );
            break;

        case se_InvalidSwitches:
            emit error( tr("Invalid switches around '%1' at %2")
                          .arg(errorKeyword)
                          .arg(lm(curLine))
                       );
            break;

        case se_UnexpectedToken:
            emit error( tr("Unexpected token around '%1' at %2")
                          .arg(errorKeyword)
                          .arg(lm(curLine))
                       );
            break;

        case se_EscapeOnEndLine:
            emit error( tr("Escape character on line break at %1")
                          .arg(lm(curLine))
                       );
            break;

        case se_InvalidFunction:
            emit warning( tr("Invalid function at line %1")
                          .arg(lm(curLine))
                         );
            break;

        case se_NegativeNotAllowed:
            emit error( tr("Negative value not allowed around '%1' at %2")
                          .arg(errorKeyword)
                          .arg(lm(curLine))
                       );
            break;

        case se_InvalidTimer:
            emit error( tr("Invalid timer on line %1")
                          .arg(lm(curLine))
                       );
            break;

        case se_BreakNoWhile:
            emit error( tr("Trying to 'break' outside while loop, %1")
                          .arg(lm(curLine))
                       );
            break;

        case se_ContinueNoWhile:
            emit error( tr("Trying to 'continue' outside while loop, %1")
                          .arg(lm(curLine))
                       );
            break;

        default:
            emit warning( tr("Function ended abnormal (code %1) around '%2' at %3")
                          .arg(res)
                          .arg(errorKeyword)
                          .arg(lm(curLine))
                         );
            break;

    }
}

QString TScript::getParamList(QString fn)
{
    int pos = fnindex.value(fn, -1);
    if (pos == -1)
        return "";

    QString result;
    // Scan the script for parameters.
    enum {
        s_Wait = 0,
        s_ReadParam
    };

    int state = s_Wait; // reading states...
    for (int i = pos; i <= scriptstr.length()-1; ++i) {
        QChar c = scriptstr[i];

        if (c == '\n')
            break;

        if (state == s_Wait) {
            if (c == '%') {
                state = s_ReadParam;
                result += '%';
                continue;
            }
            if (c == '(') {
                state = s_ReadParam;
                continue;
            }
        }

        if (state == s_ReadParam) {
            if (c == ',') {
                state = s_Wait;
                result += " ";
                continue;
            }
            if (c == ')')
                break;

            result += c;
        }
    }

    if (result.isEmpty())
        result = "None";

    return result;
}
