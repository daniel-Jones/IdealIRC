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

/*!
 * \param con IRC connection which we send data to.
 * \param cfg Pointer to config class (iirc.ini)
 * \param parent Parent object (usually IConnection)
 */
ICommand::ICommand(IConnection *con, config *cfg, QObject *parent) :
    QObject(parent),
    tstar("***"),
    conf(cfg),
    connection(con)
{
}

/*!
 * \param command A command to parse, for example "/join #channel key"
 *
 * \return true when command was found, false otherwise.\n Note that, even if a command fails to execute, true is still returned.
 */
bool ICommand::parse(QString command)
{
    /*
        This function must return true if the command was found, even if
        the command resulted in an error!
        ONLY return false if the command was not found.
    */

    if (command[0] == '/')
        command = command.mid(1);

    QStringList token = command.split(' ');
    QString t1 = token[0].toUpper();

    if (t1 == "JOIN") {
        if (! connection->isOnline()) {
            localMsg(NotConnectedToServer("/Join"));
            return true;
        }
        if (token.count() == 1) {
            localMsg(InsufficientParameters("/Join"));
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
            localMsg(NotConnectedToServer("/Part"));
            return true;
        }
        if (token.count() == 1) {
            localMsg(InsufficientParameters("/Part"));
            return true;
        }
        if (token.count() == 2) {
            part(token.at(1));
            return true;
        }
        QString reason = command.mid(t1.length() + token[1].length() + 2);
        part(token.at(1), reason);
        return true;
    }

    if (t1 == "QUIT") {
        QString reason;

        reason = command.mid(5);
        quit(reason);
    }

    if (t1 == "NOTICE") {
        if (! connection->isOnline()) {
            localMsg(NotConnectedToServer("/Notice"));
            return true;
        }
        if (token.count() < 3) {
            localMsg(InsufficientParameters("/Notice"));
            return true;
        }
        QString text = command.mid(token[1].length()+8);
        notice(token[1], text);
        return true;
    }

    if (t1 == "MSG") {
        if (! connection->isOnline()) {
            localMsg(NotConnectedToServer("/Msg"));
            return true;
        }
        if (token.count() < 3) {
            localMsg(InsufficientParameters("/Msg"));
            return true;
        }
        QString text = command.mid(token[1].length()+5);
        msg(token[1], text);
        return true;
    }

    if (t1 == "ME") {
        if (! connection->isOnline()) {
            localMsg(NotConnectedToServer("/Me"));
            return true;
        }
        subwindow_t wt = winlist->value(activewin());
        if ((wt.type != WT_CHANNEL) && (wt.type != WT_PRIVMSG)) {
            localMsg(tr("You're not in a chat window!"));
            return true;
        }
        QString target = wt.widget->getTarget();
        me(target, command.mid(3));
        return true;
    }

    if (t1 == "CTCP") {
        if (! connection->isOnline()) {
            localMsg(NotConnectedToServer("/Ctcp"));
            return true;
        }

        if (token.count() < 3) {
            localMsg(InsufficientParameters("/Ctcp"));
            return true;
        }

        // /ctcp target msg param param param ...

        int skip = 0;
        for (int i = 0; i <= 1; ++i)
            skip += token[i].length() + 1;
        QString msg = command.mid(skip).toUpper();

        if (msg == "PING") {
            QString ms = QString::number( QDateTime::currentMSecsSinceEpoch() );
            msg += ' ';
            msg += ms;
        }

        ctcp(token[1], msg);
        return true;
    }

    if (t1 == "KICK") {
        // kick #chan nick [reason]
        if (token.count() < 2) {
            localMsg(InsufficientParameters("/Kick"));
            return true;
        }
        QString channel = token[1];
        QString nickname;
        int l = 5;
        if (! connection->isValidChannel(channel)) {
            channel.clear();
            nickname = token[1];
            l += nickname.length() + 1;
        }
        else {
            nickname = token[2];
            l += nickname.length() + channel.length() + 2;
        }

        QString reason = command.mid(l);

        kick(channel, nickname, reason);

        return true;
    }

    if (t1 == "KILL") {
        if (token.count() < 2) {
            localMsg(InsufficientParameters("/Kill"));
            return true;
        }
        QString nickname = token[1];
        QString reason = command.mid(nickname.length() + 5);

        kill(nickname, reason);
        return true;
    }

    if (t1 == "BAN") {
        if (token.count() < 2) {
            localMsg(InsufficientParameters("/Ban"));
            return true;
        }

        QString channel = token[1];
        QString nickname;
        if (token.count() == 2) {
            channel.clear();
            nickname = token[1];
        }
        else
            nickname = token[2];

        ban(channel, nickname);
        return true;
    }

    if ((t1 == "RAW") || (t1 == "QUOTE")) {
        if (! connection->isOnline()) {
            localMsg( tr("/%1: Not connected to server.")
                     .arg(t1)
                  );
            return true;
        }
        if (token.count() < 2) {
            localMsg( tr("/%1: Insufficient parameters.")
                     .arg(t1)
                  );
            return true;
        }
        QString c = command.mid(t1.length()+1);
        raw(c);
        return true;
    }

    if (t1 == "CHARSET") {
        if (token.count() < 2) {
            localMsg( tr("/Charset: Current set is '%1'")
                     .arg(conf->charset));
            return true;
        }

        charset(token[1]);
        return true;
    }

    if (t1 == "CLEAR") {
        if (token.count() == 1) {
            // clear active window
        }

        subwindow_t sw = getCurrentSubwin();
        if (sw.type == WT_NOTHING)
            return true;
        sw.widget->clear();
        return true;
    }

    if (t1 == "PING") {
        if (! connection->isOnline()) {
            localMsg(NotConnectedToServer("/Ping"));
            return true;
        }
        ping();
        return true;
    }

    if (t1 == "QUERY") {
        if (token.count() < 2) {
            localMsg(InsufficientParameters("/Query"));
            return true;
        }

        query(token[1]);
        return true;
    }

    if (t1 == "CHANSETTINGS") {
        chansettings();
        return true;
    }

    return false; // Command wasn't found.
}

/*!
 * \param channel Channel to join
 * \param password Channel password (optional)
 *
 * Joins a channel.
 */
void ICommand::join(QString channel, QString password)
{
    if (password.length() > 0)
        password.prepend(" :");

    sockwrite( QString("JOIN %1%2")
                 .arg(channel)
                 .arg(password)
              );
}

/*!
 * \param channel Channel to leave
 * \param reason Reason for leaving (optional)
 *
 * Leaves a channel.
 */
void ICommand::part(QString channel, QString reason)
{
    if (reason.length() > 0)
        reason.prepend(" :");

    sockwrite( QString("PART %1%2")
                 .arg(channel)
                 .arg(reason)
              );
}

/*!
 * \param reason Quit reason (optional)
 *
 * Sends QUIT message to the IRC server.\n Will eventually result in IConnection loses connection.
 */
void ICommand::quit(QString reason)
{
    sockwrite( QString("QUIT :%1")
                 .arg(reason)
              );
}

/*!
 * \param target Who to receive the notice
 * \param message The message
 *
 * Sends a notice message to the target.\n
 * The target must be either a channel or a nickname. IdealIRC need no relations to these.
 */
void ICommand::notice(QString target, QString message)
{
    sockwrite( QString("NOTICE %1 :%2")
                 .arg(target)
                 .arg(message)
              );

    target.prepend('>');
    target.append('<');
    echo( target, message, PT_NOTICE );
}

/*!
 * \param target Who to receive the message
 * \param message The message
 *
 * Sends a message to the target.\n
 * The target must be either a channel or a nickname. IdealIRC need no relations to these.
 */
void ICommand::msg(QString target, QString message)
{
    sockwrite( QString("PRIVMSG %1 :%2")
                 .arg(target)
                 .arg(message)
              );

    target.prepend('>');
    target.append('<');
    echo(target, message);
}

/*!
 * \param target Who to receive the "action"
 * \param message The "action"
 *
 * Sends an action-style message to the target.\n
 * Example message: /me eats potato chips.\n
 * * John_Doe eats potato chips.
 */
void ICommand::me(QString target, QString message)
{
    QString send = QString("%1ACTION %2%3")
                     .arg( QChar(0x01) )
                     .arg(message)
                     .arg( QChar(0x01) );

    sockwrite( QString("PRIVMSG %1 :%2")
                 .arg(target)
                 .arg(send)
              );

    echo( "", QString("* %1 %2")
                .arg(getCurrentNickname())
                .arg(message),
          PT_ACTION
         );
}

/*!
 * \param channel Target channel
 * \param nickname Target nickname
 * \param reason Reason for kick (optional)
 *
 * Granted we've got operator status in the channel, this command forces a member
 * to leave the given channel.
 */
void ICommand::kick(QString channel, QString nickname, QString reason)
{
    if (channel.isEmpty()) {
        subwindow_t wt = getCurrentSubwin();
        if (wt.type != WT_CHANNEL) {
            echo(tstar, NotInAChannel("/Kick"));
            return;
        }
        channel = wt.widget->getTarget();
    }

    sockwrite( QString("KICK %1 %2 :%3")
                 .arg(channel)
                 .arg(nickname)
                 .arg(reason)
              );
}

/*!
 * \param nickname Nickname to disconnect
 * \param reason Reason for forced disconnection
 *
 * Granted we've got server operator status, this command forces a member
 * to disconnect from the server.
 */
void ICommand::kill(QString nickname, QString reason)
{
    sockwrite( QString("KILL %1 :%2")
                 .arg(nickname)
                 .arg(reason)
              );
}

/*!
 * \param channel Target channel
 * \param nickname Target nickname
 *
 * This command utilizes IAL to set a ban on the given member.\n
 * IAL sees if it got the hostname on the given nickname, if not, it will obtain it and set the ban.
 */
void ICommand::ban(QString channel, QString nickname)
{
    if (channel.isEmpty()) {
        subwindow_t wt = getCurrentSubwin();
        if (wt.type != WT_CHANNEL) {
            localMsg(NotInAChannel("/Ban"));
            return;
        }
        channel = wt.widget->getTarget();
    }

    connection->ial.setChannelBan(channel, nickname);
}

/*!
 * \param data Data
 *
 * Sends raw data to the IRC server.
 */
void ICommand::raw(QString data)
{
    sockwrite(data); // Send raw data.
    echo( "", tr("[RAW] %1")
                .arg(data)
         );
}

/*!
 * \param target Target nickname
 * \param message CTCP message
 *
 * Sends a CTCP (Client-To-Client-Protocol) message.\n
 */
void ICommand::ctcp(QString target, QString message)
{

    echo( "", QString("[CTCP %1] to %2")
            .arg(message)
            .arg(target),
          PT_CTCP
         );


    message.prepend(0x01);
    message.append(0x01);

    sockwrite( QString("PRIVMSG %1 :%2")
                 .arg(target)
                 .arg(message)
              );
}

/*!
 * \param newCodec Text codec
 *
 * Changes text encoding to a different codec.\n
 * For example, UTF-8
 */
void ICommand::charset(QString newCodec)
{
    QList<QByteArray> codecList = QTextCodec::availableCodecs();
    bool exist = false;
    QByteArray codec;
    codec.append(newCodec);

    for (int i = 0; i <= codecList.length()-1; i++) {
        QString item = codecList[i];
        if (item.toUpper() == newCodec.toUpper()) {
            newCodec = item.toUpper();
            exist = true;
            break;
        }
    }
    if (! exist) {
        localMsg( tr("/Charset: Encoding '%1' doesn't exsist.")
                 .arg(newCodec)
              );
    }
    else {
        localMsg( tr("/Charset: Set encoding to '%1'")
                 .arg(newCodec)
              );
        conf->charset = newCodec;
    }
}

/*!
 * This is not CTCP PING.\n
 * Use this command to test your latency against the server.\n
 * It responds with the amount of miliseconds.
 */
void ICommand::ping()
{
    QString ms = QString::number(QDateTime::currentMSecsSinceEpoch());
    echo(tstar, tr("Sending PING to server..."), PT_SERVINFO);

    sockwrite( QString("PING :%1")
                 .arg(ms)
              );
}

/*!
 * \param nickname Nickname
 *
 * Opens a query subwindow on the nickname.\n
 * The nickname doesn't need to exist on the server.
 */
void ICommand::query(QString nickname)
{
    emit requestWindow(nickname, WT_PRIVMSG, *cid, true);
}

/*!
 * Opens the channel settings dialog, given this command runs from a channel window.\n
 * If it isn't a channel window, nothing happens.
 */
void ICommand::chansettings()
{
    subwindow_t sw = winlist->value(activewin());
    sw.widget->execChanSettings();
}

/// -------------------------------------------------------------

/*!
 * \param message Message
 *
 * Prints a message formatted as PT_LOCALINFO (see constants.h)
 */
void ICommand::localMsg(QString message)
{
    subwindow_t wt = getCurrentSubwin();
    wt.widget->print(message, tstar, PT_LOCALINFO);
}

/*!
 * \param sender Sender. Text goes in left margin.
 * \param message The message.
 * \param ptype Print type. See constants.h for PT_*
 *
 * Prints (echoes) a message onto the current active subwindow.\n
 * If the subwindow is a custom picture window, it'll print in a status window.
 */
void ICommand::echo(QString sender, QString message, int ptype)
{
    subwindow_t wt = getCurrentSubwin();
    wt.widget->print(sender, message, ptype);
}

/*!
 * \param data Data
 *
 * Sends (raw) data to the socket in IConnection.
 */
void ICommand::sockwrite(QString data)
{
    connection->sockwrite(data);
}


/*!
 * \return QString of current target we can send to.
 */
QString ICommand::getCurrentTarget()
{
     subwindow_t wt = getCurrentSubwin();
     return wt.widget->getTarget();
}

/*!
 * \return QString of our current nickname.
 */
QString ICommand::getCurrentNickname()
{
    subwindow_t wt = getCurrentSubwin();
    return wt.widget->getConnection()->getActiveNickname();
}

/*!
 * \return QString of active window name.
 */
QString ICommand::activewin()
{
    if (*activeConn != *cid)
        return "STATUS";
    else
        return activeWname->toUpper();
}

/*!
 * \return Subwindow structure of current sub window.
 */
subwindow_t ICommand::getCurrentSubwin()
{
    if (*activeConn != *cid)
        return winlist->value("STATUS");

    subwindow_t sw_empty = SW_EMPTY_SET;
    return winlist->value(activewin(), sw_empty);
}
