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

#ifndef ICOMMAND_H
#define ICOMMAND_H
#include <QObject>
#include "config.h"
#include "constants.h"

class IWin;
class IConnection;

class ICommand : public QObject
{
    Q_OBJECT

public:
    explicit ICommand(IConnection *con, config *cfg, QObject *parent = 0);
    void setWinList(QHash<QString,subwindow_t> *wl) { winlist = wl; }
    void setActiveInfo(QString *wn, int *ac) { activeWname = wn; activeConn = ac; }
    void setCid(int *c) { cid = c; }

    // Otherwise we can directly access given commands:
    void join(QString channel, QString password = "");
    void part(QString channel, QString reason = "");
    void quit(QString reason);
    void notice(QString target, QString message);
    void msg(QString target, QString message);
    void me(QString target, QString message);
    void ctcp(QString target, QString message); // Do not add 0x01 to the message, this function does it.
    void kick(QString channel, QString nickname, QString reason = "");
    void ban(QString channel, QString nickname);
    void raw(QString data);
    void charset(QString newCodec = ""); // Adding no parameters echoes out current charset.
    void ping(); // Issue a PING to the server (see if we or them is still alive)
    void query(QString nickname); // Open a query window for "nickname"
    void chansettings();

public slots:
    // Primarily we should use parse. You can tie it to a signal aswell.
    bool parse(QString command); // Returns true if command was found in this class

private:
    QHash<QString,subwindow_t> *winlist;
    QString *activeWname;
    int *activeConn;
    int *cid;
    QString tstar;
    config *conf;
    IConnection *connection;
    QString activewin();
    void localMsg(QString message);
    void echo(QString sender, QString message, int ptype = PT_NORMAL);
    void sockwrite(QString data);
    QString getCurrentTarget();
    QString getCurrentNickname();
    subwindow_t getCurrentSubwin();
    QString NotConnectedToServer(QString command) { return tr("%1: Not connected to server.").arg(command); }
    QString InsufficientParameters(QString command) { return tr("%1: Insufficient parameters.").arg(command); }
    QString NotInAChannel(QString command) { return tr("%1: Not in a channel.").arg(command); }

signals:
    void requestWindow(QString name, int type, int parent, bool activate = true);
};

#endif // ICOMMAND_H
