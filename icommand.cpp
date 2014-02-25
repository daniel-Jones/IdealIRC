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

#include <QStringList>
#include <QTextCodec>
#include <QDateTime>
#include "icommand.h"
#include "iwin.h"
#include "iconnection.h"

ICommand::ICommand(IConnection *con, config *cfg, QObject *parent) :
    QObject(parent),
    conf(cfg),
    connection(con)
{
}

bool ICommand::parse(QString command)
{
    /*
        This function must return true if the command was found, even if
        the command resulted in an error!
        ONLY return false if the command was not found.
    */

    if (command.at(0) == '/')
        command = command.mid(1);

    QStringList token = command.split(" ");
    QString t1 = token.at(0).toUpper();

    if (t1 == "JOIN") {
        if (! connection->isOnline()) {
            fault("/Join: Not connected to server.");
            return true;
        }
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
        if (! connection->isOnline()) {
            fault("/Part: Not connected to server.");
            return true;
        }
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
        if (! connection->isOnline()) {
            fault("/Me: Not connected to server.");
            return true;
        }
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

    if (t1 == "MSG") {
        if (! connection->isOnline()) {
            fault("/Msg: Not connected to server.");
            return true;
        }
        if (token.count() < 3) {
            fault("/Msg: Insufficient parameters.");
            return true;
        }
        QString text = command.mid(token[1].length()+5);
        msg(token[1], text);
        return true;
    }

    if (t1 == "NOTICE") {
        if (! connection->isOnline()) {
            fault("/Notice: Not connected to server.");
            return true;
        }
        if (token.count() < 3) {
            fault("/Notice: Insufficient parameters.");
            return true;
        }
        QString text = command.mid(token[1].length()+8);
        notice(token[1], text);
        return true;
    }

    if ((t1 == "RAW") || (t1 == "QUOTE")) {
        if (! connection->isOnline()) {
            fault(  QString("/%1: Not connected to server.")
                    .arg(t1)
                  );
            return true;
        }
        if (token.count() < 2) {
            fault(  QString("/%1: Insufficient parameters.")
                    .arg(t1)
                  );
            return true;
        }
        QString c = command.mid(t1.length()+1);
        sockwrite(c); // Send raw data.
        echo(  QString("RAW: %1")
               .arg(c)
             );
        return true;
    }

    if (t1 == "CHARSET") {
        if (token.count() < 2) {
            fault(QString("/Charset: Current set is '%1'")
                  .arg(conf->charset));
            return true;
        }

        QList<QByteArray> codecList = QTextCodec::availableCodecs();
        bool exist = false;
        QByteArray codec;
        codec.append(token[1]);

        for (int i = 0; i <= codecList.length()-1; i++) {
            QString item = codecList[i];
            if (item.toUpper() == token[1].toUpper()) {
                exist = true;
                break;
            }
        }
        if (! exist) {
            fault(QString("/Charset: Encoding '%1' doesn't exsist.")
                  .arg(token[1]));
        }
        else {
            // Not really a fault, but it doesn't do anything else than printing.
            fault(QString("/Charset: Set encoding to '%1'")
                  .arg(token[1]));
            conf->charset = token[1];
        }
        return true;
    }

    if (t1 == "PING") {
        if (! connection->isOnline()) {
            fault("/Ping: Not connected to server.");
            return true;
        }
        QString ms = QString::number(QDateTime::currentMSecsSinceEpoch());
        echo("Sending PING to server...", PT_SERVINFO);
        sockwrite(QString("PING :%1")
                  .arg(ms));
        return true;
    }


    return false; // Command wasn't found.
}

void ICommand::join(QString channel, QString password)
{
    if (password.length() > 0)
        password.prepend(" :");

    sockwrite(  QString("JOIN %1%2")
                .arg(channel)
                .arg(password)
              );
}

void ICommand::part(QString channel, QString reason)
{
    if (reason.length() > 0)
        reason.prepend(" :");

    sockwrite(  QString("PART %1%2")
                .arg(channel)
                .arg(reason)
              );
}

void ICommand::quit(QString reason)
{
    sockwrite(  QString("QUIT :%1")
                .arg(reason)
              );
}

void ICommand::notice(QString target, QString message)
{
    sockwrite(  QString("NOTICE %1 :%2")
                .arg(target)
                .arg(message)
              );

    echo(  QString(">%1< %2")
           .arg(target)
           .arg(message),
         PT_NOTICE);
}

void ICommand::msg(QString target, QString message)
{
    sockwrite(  QString("PRIVMSG %1 :%2")
                .arg(target)
                .arg(message)
              );

    echo(  QString(">%1< %2")
           .arg(target)
           .arg(message)
         );
}

void ICommand::ctcp(QString target, QString message)
{
    message.prepend(0x01);
    message.append(0x01);

    sockwrite(  QString("PRIVMSG %1 :%2")
                .arg(target)
                .arg(message)
              );
}

void ICommand::me(QString target, QString message)
{
    QString send = QString("%1ACTION %2%3")
                   .arg( QChar(0x01) )
                   .arg(message)
                   .arg( QChar(0x01) );

    sockwrite("PRIVMSG " + target + " :" + send);

    echo(  QString("%1 %2")
           .arg(getCurrentNickname())
           .arg(message),
         PT_ACTION);
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
