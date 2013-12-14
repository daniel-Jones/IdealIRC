/*
 *   IdealIRC - Internet Relay Chat client
 *   Copyright (C) 2013  Tom-Andre Barstad
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

#include "icommand.h"
#include <QStringList>
#include <iostream>

#include "iwin.h"
#include "iconnection.h"

ICommand::ICommand(QObject *parent) :
    QObject(parent)
{
}

bool ICommand::parse(QString command)
{
    if (command.at(0) == '/')
        command = command.mid(1);

    QStringList token = command.split(" ");
    QString t1 = token.at(0).toUpper();

    if (t1 == "JOIN") {
        if (token.count() == 1) {
            fault("/Join: Insufficent parameters.");
            return true;
        }
        if (token.count() == 2) {
            join(token.at(1));
            return true;
        }
        QString pass = command.mid(t1.length() + token.at(1).length() + 2);
        join(token.at(1), pass);
        return true;
    }

    if (t1 == "PART") {
        if (token.count() == 1) {
            fault("/Part: Insufficent parameters.");
            return true;
        }
        if (token.count() == 2) {
            part(token.at(1));
            return true;
        }
        QString reason = command.mid(t1.length() + token.at(1).length() + 2);
        part(token.at(1), reason);
        return true;
    }

    if (t1 == "ME") {
        subwindow_t wt = winlist->value(activewin());
        if ((wt.type != WT_CHANNEL) && (wt.type != WT_PRIVMSG)) {
            qDebug() << "/ME FAULTED";
            fault("You're not in a chat window!");
            return true;
        }
        QString target = wt.widget->getTarget();
        me(target, command.mid(3));
        return true;
    }

    return false;
}

void ICommand::join(QString channel, QString password)
{
    if (password.length() > 0)
        password.prepend(" :");

    sockwrite("JOIN "+channel+password);
}

void ICommand::part(QString channel, QString reason)
{
    if (reason.length() > 0)
        reason.prepend(" :");

    sockwrite("PART "+channel+reason);
}

void ICommand::quit(QString reason)
{
    sockwrite("QUIT :"+reason);
}

void ICommand::notice(QString target, QString message)
{
    sockwrite("NOTICE "+target+" :"+message);

    QString text = QString(">%1< %2")
                   .arg(target).arg(message);

    echo(text, PT_NOTICE);
}

void ICommand::msg(QString target, QString message)
{
    sockwrite("MSG "+target+" :"+message);
    QString text = QString(">%1< %2")
                   .arg(target).arg(message);
    echo(text);
}

void ICommand::ctcp(QString target, QString message)
{
    message.prepend(0x01);
    message.append(0x01);
    QString data = QString("PRIVMSG %1 :%2")
                    .arg(target)
                    .arg(message);
    sockwrite(data);
}

void ICommand::me(QString target, QString message)
{
    QString send = QChar(0x01);
    send.append("ACTION ");
    send.append(message);
    send.append(0x01);

    sockwrite("PRIVMSG " + target + " :" + send);
    QString text = QString("%1 %2")
                   .arg(getCurrentNickname()).arg(message);
    echo(text, PT_ACTION);
}


/// -------------------------------------------------------------

void ICommand::fault(QString message)
{
    subwindow_t wt = winlist->value(activewin());
    wt.widget->print(message, PT_LOCALINFO);
}

void ICommand::echo(QString message, int ptype)
{
    subwindow_t wt = winlist->value(activewin());
    wt.widget->print(message, ptype);
}

void ICommand::sockwrite(QString data)
{
    subwindow_t wt = winlist->value(activewin());
    wt.widget->sockwrite(data);
}

QString ICommand::getCurrentTarget()
{
     subwindow_t wt = winlist->value(activewin());
     return wt.widget->getTarget();
}

QString ICommand::getCurrentNickname()
{
    subwindow_t wt = winlist->value(activewin());
    return wt.widget->getConnection()->getActiveNickname();
}

QString ICommand::activewin()
{
    if (*activeConn != *cid)
        return "STATUS";
    else
        return activeWname->toUpper();
}
