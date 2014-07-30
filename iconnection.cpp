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

#include <iostream>
#include <QStringList>
#include <QDateTime>
#include <QDebug>

#include "constants.h"
#include "iconnection.h"
#include "icommand.h"
#include "numerics.h"
#include "script/tscriptparent.h"

IConnection::IConnection(QObject *parent, IChannelList **clptr, int connId,
                         config *cfg, TScriptParent *sp, IWindowSwitcher *ws) :
    QObject(parent),
    maxBanList(-1), /** -1 means undefined. **/
    maxExceptList(-1),
    maxInviteList(-1),
    maxModes(3),
    haveExceptionList(false),
    haveInviteList(false),
    FillSettings(false),
    ial(this, &activeNick, &sortrule, sp),
    //addresslist((QWidget*)parent, this),
    cmdhndl(this, cfg),
    conf(cfg),
    cid(connId),
    active(false),
    registered(false),
    isupport(false),
    ShuttingDown(false),
    tryingConnect(false),
    chanlistPtr(clptr),
    listInDialog(false),
    connectionClosing(false),
    motd(cfg, (QWidget*)parent),
    scriptParent(sp),
    receivingNames(false),
    cmA("b"),
    cmB("k"),
    cmC("l"),
    cmD("imnpstr"),
    tstar("***"),
    sstar("*"),
    checkState(0),
    wsw(ws)
{
    cmdhndl.setWinList(&winlist);
    cmdhndl.setCid(&cid);

    connect(&cmdhndl, SIGNAL(requestWindow(QString,int,int,bool)),
            this, SIGNAL(RequestWindow(QString,int,int,bool)));

    std::cout << "Connection class ID: " << cid << std::endl;
    resetSortRules();

    connect(&socket, SIGNAL(connected()),
            this, SLOT(onSocketConnected()));

    connect(&socket, SIGNAL(disconnected()),
            this, SLOT(onSocketDisconnected()));

    connect(&socket, SIGNAL(readyRead()),
            this, SLOT(onSocketReadyRead()));

    connect(&checkConnection, SIGNAL(timeout()),
            this, SLOT(checkConnectionTimeout()));

    checkConnection.setInterval(180000); // 3 min.
}

//                          server:port   optional, server passwd.
void IConnection::setServer(QString server, QString passwd)
{
    if (server.length() == 0)
        server = conf->server;

    QStringList details = server.split(':');
    host = details.at(0);
    port = details.at(1).toInt();
    password = passwd;
}

void IConnection::tryConnect()
{
    setServer();
    IWin *s = winlist.value("STATUS").widget;

    s->print( tstar, tr("Connecting to %1:%2...")
                      .arg(host)
                      .arg(port),
              PT_LOCALINFO
             );

    tryingConnect = true;
    socket.connectToHost(host, port, QIODevice::ReadWrite | QIODevice::Text);
}

void IConnection::addWindow(QString name, subwindow_t win)
{
    winlist.insert(name.toUpper(), win);
    if (win.type == WT_CHANNEL)
        acList << name;
}

void IConnection::freeWindow(QString name)
{
    winlist.remove(name.toUpper());

    acList.removeAll(name);
}

bool IConnection::sockwrite(QString data)
{
    if (! socket.isOpen()) {
        print("STATUS", tstar, tr("Not connected to server"), PT_LOCALINFO);
        return false;
    }

    if (data.length() == 0)
      return false;

    QTextCodec *tc = QTextCodec::codecForName(conf->charset.toStdString().c_str());
    if (tc != 0) {
        QByteArray encodedData = tc->fromUnicode(data);
        encodedData.append("\r\n");
        socket.write(encodedData);
        qDebug() << "[out]" << encodedData;
    }
    else {
        QByteArray outData;
        outData.append(data);
        outData.append("\r\n");
        socket.write(outData);
        qDebug() << "[out]" << outData;
    }
    return true;
}

void IConnection::onSocketConnected()
{
    emit updateConnectionButton();

    if (conf->password.length() > 0)
        sockwrite("PASS :" + conf->password);

    QString username;                            // Store our username to pass to IRC server here.
    if (conf->username[0] == '@')                // If our @ token is first, we'll just use the nickname.
        username = conf->nickname;               // This to prevent any errors on following:
    else
        username = conf->username.split('@')[0]; // Pick username from email. May not contain a '@' though.

    sockwrite( QString("NICK %1")
                 .arg(conf->nickname)
              );

    sockwrite( QString("USER %1 \"\" \"\" :%2")
                 .arg(username)
                 .arg(conf->realname)
              );
}

void IConnection::onSocketDisconnected()
{
    if (ShuttingDown) { // Close all windows related to this connection
        socket.close();
        emit connectionClosed();
        return; // All windows are closed, status window will close itself when ready to close.
    }

    ial.reset();

    active = false;
    maxBanList = -1;
    maxExceptList = -1;
    maxInviteList = -1;
    maxModes = 3;
    haveExceptionList = false;
    haveInviteList = false;
    FillSettings = false;
    registered = false;
    isupport = false;
    chantype.clear();
    cumode.clear();
    culetter.clear();
    resetSortRules();
    cmA = "b";
    cmB = "k";
    cmC = "l";
    cmD = "imnpstr";
    checkConnection.setInterval(180000);
    checkConnection.stop();

    QHashIterator<QString, subwindow_t> i(winlist);
    while (i.hasNext()) {
        i.next();
        subwindow_t win = i.value();

        win.widget->print(tstar, tr("Disconnected."), PT_LOCALINFO);
        if (win.type == WT_CHANNEL) {
            win.widget->resetMemberlist();
        }
    }
    socket.close();
    emit updateConnectionButton();
    emit connectionClosed();

    QString hostinfo = QString("%1:%2")
                        .arg(host)
                        .arg(port);

    scriptParent->runevent(te_disconnect, QStringList()<<hostinfo);
}

void IConnection::closeConnection(bool shutdown)
{
    if (tryingConnect == true) {
        print("STATUS", tstar, tr("Disconnected."), PT_LOCALINFO);
        socket.close();
        tryingConnect = false;
        return;
    }

    if (socket.isOpen()) {
        connectionClosing = true;
        ShuttingDown = shutdown;
        QString data = QString("QUIT :%1")
                         .arg( conf->quit );

        sockwrite(data);
    }
}

void IConnection::onSocketReadyRead()
{
    /*
      Define a global (class) variable (linedata) to read in each IRC line to parse.
      Qt converts \r\n to \n since socket is opened in Text mode.
      We insert our received data to "linedata", until we reach \n. Then we'll parse the "linedata"
      and reset it and continue reading in until our buffer is empty.
    */

    QByteArray in = socket.readAll();
    tryingConnect = false;

    QTextCodec *tc = QTextCodec::codecForName(conf->charset.toStdString().c_str());

    for (int i = 0; i <= in.length()-1; i++) {

      if (in[i] == '\n') {
          // Newline, parse this.
          QString text = tc->toUnicode(linedata);
          std::cout << "[in] " << text.toStdString().c_str() << std::endl;
          parse( text );
          linedata.clear();

          if (checkState == 0)
              checkConnection.start(); // (re-)start the timer that checks if our TCP connection's still alive.

          continue;
      }

      // By default, add data to linedata.
      linedata += in.at(i);
    }
}

void IConnection::checkConnectionTimeout()
{
    if (checkState == 0) {
        checkState = 1;
        checkConnection.setInterval(30000); // 30 seconds to get a response.
        sockwrite("PING :ALIVE");
        return;
    }

    if (checkState == 1) {
        checkState = 0;
        checkConnection.setInterval(180000); // set back to default 3 min.
        print("STATUS", tstar, tr("Ping timeout."), PT_LOCALINFO);
        socket.close(); // close socket, connection's dead.
        return;
    }
}

IWin* IConnection::getWinObj(QString name)
{
    subwindow_t empty;
    empty.wid = -1; // Indicate error

    subwindow_t w = winlist.value(name.toUpper(), empty);

    if (w.wid == -1)
        return NULL;
    else
        return w.widget;
}

bool IConnection::windowExist(QString name)
{
    subwindow_t empty;
    empty.wid = -1; // Indicate error

    subwindow_t w = winlist.value(name.toUpper(), empty);

    if (w.wid == -1)
        return false;
    else
        return true;
}

bool IConnection::isValidChannel(QString channel)
{
    if (channel.isEmpty())
        return false;

    char prefix = channel[0].toLatin1();
    return chantype.contains(prefix);
}

char IConnection::getCuLetter(char l)
{
    if (cumode.contains(l))
        return culetter.at( cumode.indexOf(l) );
    else
        return 0x00;
}

bool IConnection::isValidCuMode(char mode)
{
    return cumode.contains(mode);
}

bool IConnection::isValidCuLetter(char l)
{
    return culetter.contains(l);
}

QString IConnection::trimCtrlCodes(QString &text)
{
    /*
        0x02 - bold
        0x03 - color
        0x1F - underline
    */

    QString result;
    for (int i = 0; i <= text.length()-1; i++) {
        QChar c = text[i];
        if (c == CTRL_BOLD)
            continue;
        if (c == CTRL_UNDERLINE)
            continue;
        if (c == CTRL_COLOR) {

            for (i++; i <= text.length()-1; i++) {
                c = text[i];
                bool numok = false;
                QString(c).toInt(&numok);
                if ((numok == false) && (c == ','))
                    continue;
                if (numok == false)
                    break;
            }
            i--;

            continue;
        }

        result += c;
    }

    return result;
}

void IConnection::print(const QString &window, const QString &sender, const QString &line, const int ptype)
{
    IWin *w = NULL; // Default value of *w is NULL to make sure we can error-check.
    w = getWinObj(window.toUpper()); // Attempt to get window object...
    if (w == NULL) // No such window, go back to default...
        w = getWinObj("STATUS"); // Default to Status window, this one always exsist.
    if ((w == NULL) && (window == "STATUS"))
        return; // Nowhere to send this text, ignore silently...

    w->print(sender, line, ptype); // Do printing
}

unsigned int IConnection::ipv4toint(QString addr)
{
    QStringList token = addr.split('.');

    unsigned int byte[4];
    for (int i = 0; i <= 3; ++i)
        byte[i] = token[i].toUInt();

    unsigned int r = 0;
    r += byte[0] << 24;
    r += byte[1] << 16;
    r += byte[2] << 8;
    r += byte[3];

    return r;
}

QString IConnection::intipv4toStr(unsigned int addr)
{
    unsigned int byte[4];

    int bc = 0; // bit count

    for (int i = 0; i <= 3; ++i) {
        unsigned int b = addr;
        if (bc > 0)
            b <<= bc; // push our desired byte of the integer to the left.
        b = b >> 24; // push our desired byte to the right, filling unused bytes with 0's.
        byte[i] = b;

        bc += 8;
    }

    return QString("%1.%2.%3.%4")
            .arg(byte[0])
            .arg(byte[1])
            .arg(byte[2])
            .arg(byte[3]);
}

void IConnection::resetSortRules()
{
    // sortrule is a QList of char that is inserted in what order we like it to be.
    // Clear it so we can reset it.
    sortrule.clear();

    // On the topp, add any culetters (@,%,+ etc) which goes on top in nicklist box.
    if (culetter.length() > 0)
        sortrule.append( culetter ); // These goes first, so operators come on top, etc. The IRC server decides this order via ISupport!

    // Add numbers, letters and signs that are typically allowed in a nickname.
    for (char i = 0x30; i <= 0x39; i++)
        sortrule.append(i); // Numbers
    for (char i = 0x41; i <= 0x5A; i++) {
        // Add letters such as A a B b C c D d
        sortrule.append(i); // Uppercase
        sortrule.append(i+0x20); // Lowercase
    }
    for (char i = 0x5B; i <= 0x60; i++)
        sortrule.append(i); // [\]^_`
    for (char i = 0x7B; i <= 0x7D; i++)
        sortrule.append(i); // {|}
}

QString IConnection::activewin()
{
    if (*activeConn != cid)
        return "STATUS";
    else
        return activeWname->toUpper();
}

void IConnection::setActiveInfo(QString *wn, int *ac) {
    activeWname = wn;
    activeConn = ac;
    cmdhndl.setActiveInfo(wn, ac);
}

user_t IConnection::parseUserinfo(QString uinfo)
{
    if (uinfo[0] == ':')
    uinfo = uinfo.mid(1);

    //         nick, user, host
    user_t u = {uinfo, "", ""};

    if (! uinfo.contains('!')) // Nickname only
        return u;

    // nickname!ident@hostname
    QStringList a = uinfo.split("!"); // 0 = nickname, 1 = ident@hostname
    if (a.length() < 2)
        return u;

    QStringList b = a[1].split("@"); // 0 = ident, 1 = hostname
    if (a.length() < 2)
        return u;

    //   nick,   user,    host
    //u = {a[0], b[0], b[1]}; // not compatible with my current 'clang' version. GCC takes this code.
    u.nick = a[0];
    u.user = b[0];
    u.host = b[1];

    return u;
}

IChanConfig* IConnection::getChanConfigPtr(QString channel)
{
    subwindow_t sw = winlist.value(channel.toUpper());
    return sw.widget->settings;
}

QString IConnection::getMsg(QString &data)
{
    /*
        First, figure the length from beginning to first colon.
        Use that length + 1, to skip over, in order to get our msg.
    */
    if (data.at(0) == ':')
        data = data.mid(1);

    int l = data.split(":").at(0).length()+1;
    return data.mid(l);
}

void IConnection::parse(QString &data)
{
/*
  format
  :nick!user@host num/cmd [target] :msg
*/

    QStringList token = data.split(" ");
    QString token0up = token[0].toUpper();
    QString token1up = token[1].toUpper();


    if (token.size() < 2) // Rubbish, there should be at least two tokens separated by a space.
        return;

    int num = token[1].toInt();

    if (num > 0) { // valid NUMERIC
        QString params;
        QString msg = getMsg(data);
        for (int i = 2; i <= token.count()-1; ++i) {
            if (token[i].isEmpty())
                continue;

            if (token[i].at(0) == ':')
                break;

            if (! params.isEmpty())
                params += ' ';
            params += token[i];
        }
        scriptParent->runevent(te_numeric, QStringList()<<token[1]<<params<<msg);
        parseNumeric(num, data);
        return;
    }

    else if (token0up == "NOTICE") {
        QString from = token[1];
        if (token1up == "AUTH")
            from = conf->server.split(":")[0];

        QString text = getMsg(data);

        from.prepend("-");
        from.append("-");
        print("STATUS", from, text, PT_NOTICE);
        return;
    }

    else if (token0up == "PING") {
        sockwrite( QString("PONG %1")
                     .arg(token[1])
                  );
        return;
    }

    else if (token1up == "PONG") {
        bool ok = false;
        QString msg = getMsg(data);

        if (msg == "ALIVE") {
            checkState = 0;
            checkConnection.setInterval(180000);
            checkConnection.start(); // restart.
            return;
        }

        qint64 ms_now = QDateTime::currentDateTime().toMSecsSinceEpoch();
        qint64 ms_pong = msg.toULongLong(&ok);
        if (! ok)
            print( activewin(), tstar, tr("PONG from server: %1")
                                        .arg(getMsg(data)),
                   PT_SERVINFO
                  );
        else
            print( activewin(), tstar, tr("PONG from server: %1ms.")
                                        .arg(ms_now-ms_pong),
                   PT_SERVINFO
                  );
        return;
    }

    else if (token1up == "PRIVMSG") {
        user_t u = parseUserinfo(token.at(0));
        QString text = getMsg(data);

        ial.setHostname(u.nick, u.host);
        ial.setIdent(u.nick, u.user);

        // CTCP check, ACTION check
        if (text[0] != ' ') {
            bool ctcpReceived = false;

            QString request;
            QStringList tx = text.split(" ");

            request.append(0x01);
            request.append("ACTION");
            if (tx.at(0).toUpper() == request) { /// NOT CTCP, ACTION
                // ACTION
                text.remove(0x01);
                text = text.mid(7);
                QString target = token[2]; // Might be a #channel or our nickname.
                if (! isValidChannel(target))
                    target = u.nick; // Target was not a channel, but a private message. Write action to own window for u.nick

                emit RequestWindow(target, WT_PRIVMSG, cid, false);
                print( target.toUpper(), "", QString("* %1 %2")
                                        .arg(u.nick)
                                        .arg(text),
                       PT_ACTION
                      );

                return;
            }

            if (tx.at(0)[0] == 0x01) {
                // CTCP indicator
                QString ctcp = tx.at(0);
                ctcp.remove(QChar(0x01));
                print( "STATUS", u.nick, tr("[CTCP %1]")
                                   .arg(ctcp),
                       PT_CTCP
                      );
                ctcpReceived = true;
            }
            request.clear();

            request.append(0x01);
            request.append("VERSION");
            request.append(0x01);
            if (tx[0].toUpper() == request) { /// VERSION
                // VERSION
                QString reply = QString("%1VERSION IdealIRC %2 @ %3 by Tomatix%1")
                                  .arg(QChar(0x01))
                                  .arg(VERSION_STRING)
                                  .arg(SYSTEM_NAME);

                sockwrite( QString("NOTICE %1 :%2")
                             .arg(u.nick)
                             .arg(reply)
                          );
                return;
            }
            request.clear();

            request.append(0x01);
            request.append("TIME");
            request.append(0x01);
            if (tx.at(0).toUpper() == request) { /// TIME
                // TIME
                QString time = QDateTime::currentDateTime().time().toString("hh:mm:ss");
                QString date = QDateTime::currentDateTime().date().toString("dddd MMMM d, yyyy");

                QString reply = QString("%1TIME %2, %3%1")
                                  .arg(QChar(0x01))
                                  .arg(time)
                                  .arg(date);

                sockwrite( QString("NOTICE %1 :%2")
                             .arg(u.nick)
                             .arg(reply)
                          );
                return;
            }
            request.clear();

            request.append(0x01);
            request.append("PING");
            if (tx.at(0).toUpper() == request) { /// PING
                // PING
                QString reply = QString("%1PING %2%1")
                                  .arg(QChar(0x01))
                                  .arg(tx[1]);

                sockwrite( QString("NOTICE %1 :%2")
                             .arg(u.nick)
                             .arg(reply)
                          );

                return;
            }
            request.clear();

            request.append(0x01);
            request.append("DCC");
            if (tx.at(0).toUpper() == request) { /// DCC
                return; // FIXME dcc is disabled for now.

                // DCC
                /*

mirc
[in] :Tom!Tom@148.46.164.82.customer.cdi.no PRIVMSG Tomatix :DCC CHAT chat 1386491540 1024
[in] :Tom!Tom@148.46.164.82.customer.cdi.no PRIVMSG Tomatix :DCC SEND urls.ini 1386491540 1024 163
xchat
[in] :Tomatix_!tomatix@148.46.164.82.customer.cdi.no PRIVMSG tomatix :DCC CHAT chat 3232235943 60809
[in] :Tomatix_!tomatix@148.46.164.82.customer.cdi.no PRIVMSG Tomatix :DCC SEND "01 - Logo.png" 199 0 18980 1
*/

                if (token[4] == "CHAT") {
                    unsigned int ip = token[6].toUInt();
                    QString sip = intipv4toStr(ip);
                    print( activewin(), "", "CHAT ip: " + sip);
                    print( activewin(), "", "CHAT intip: " + QString::number(ipv4toint(sip)));

                    dccinfo = text.replace(char(0x01), ""); // delete 0x01 (ctcp indicators)
                    dccinfo = dccinfo.mid(4, dccinfo.length());
                    dccinfo.prepend(activeNick + " " + u.nick + " ");

                    emit RequestWindow(u.nick, WT_DCCCHAT, cid);
                }

                return;
            }

            if (ctcpReceived)
                return; // We've showed a CTCP message. stop.
        }

        QString name = u.nick;
        if (isValidChannel(token[2].toUpper()) == false) { // Privmsg
            emit RequestWindow(name, WT_PRIVMSG, cid, false);
            print( name.toUpper(), name, text);
            subwindow_t w = winlist.value(name.toUpper());
            emit HighlightWindow(w.wid, HL_MSG);
            emit RequestTrayMsg(tr("Private MSG from %1").arg(name), text);
        }
        else { // Channel
            QString sender = name;
            QString chan = token[2];
            QString traymsg = QString("<%1> %2")
                           .arg(name)
                           .arg(text);

            subwindow_t w = winlist.value(chan.toUpper());
            member_t m = w.widget->ReadMember(name);

            if ((conf->showUsermodeMsg == true) && (m.mode.length() > 0)) {
                sender.prepend(m.mode[0]);

                traymsg = QString("<%1%2> %3")
                       .arg(m.mode[0])
                       .arg(name)
                       .arg(text);
            }

            emit RequestWindow(chan, WT_CHANNEL, cid, false);

            if (text.contains(activeNick, Qt::CaseInsensitive)) {
                print(chan.toUpper(), sender, text, PT_HIGHLIGHT);
                emit HighlightWindow(w.wid, HL_HIGHLIGHT);
            }
            else {
                print(chan.toUpper(), sender, text);
                emit HighlightWindow(w.wid, HL_MSG);
            }

            // Check if our nickname is in the text, if so, put up a tray notify
            if (text.indexOf(activeNick, Qt::CaseInsensitive) > -1)
                emit RequestTrayMsg(chan, traymsg);

        }
        // privmsg script event
        scriptParent->runevent(te_msg, QStringList()<<name<<token[2]<<text);
        return;
    }

    else if (token1up == "NOTICE") {
        user_t u = parseUserinfo(token.at(0)); // Get userinfo about who this is about
        QString msg = getMsg(data);

        ial.setHostname(u.nick, u.host);
        ial.setIdent(u.nick, u.user);

        if (msg.at(0) == 0x01) { // CTCP reply
            QString ctcp = msg.split(' ')[0].toUpper();
            ctcp.remove(QChar(0x01));
            msg = msg.mid(ctcp.length() + 2);
            msg.remove(QChar(0x01));

            if (ctcp.toUpper() == "PING") {
                // Parse this.
                if (msg.length() == 0) {
                    print( activewin(), u.nick, tr("[CTCP Reply %1 (? sec)]")
                                       .arg(ctcp),
                           PT_CTCP
                          );
                    return;
                }
                qint64 cct = QDateTime::currentDateTime().toMSecsSinceEpoch();
                qint64 ct = msg.toULongLong();
                float lag = (cct-ct) / 1000.0f;
                char clag[16];
                sprintf(clag, "%.2f", lag);
                QString s_lag(clag);
                print( activewin(), u.nick, tr("[CTCP Reply %1 (%2 sec)]")
                                   .arg(ctcp)
                                   .arg(s_lag),
                       PT_CTCP
                      );

                return;
            }

            if (msg.length() > 0)
                print( activewin(), u.nick, tr("[CTCP Reply %1 (%2)]")
                                   .arg(ctcp)
                                   .arg(msg),
                       PT_CTCP
                      );

            if (msg.length() == 0)
                print( activewin(), u.nick, tr("[CTCP Reply %1] from %2")
                                   .arg(ctcp),
                       PT_CTCP
                      );

            return;
        }

        QString target = "STATUS";
        if (u.nick != serverName)
            target = activewin();

        QString sender = u.nick;
        sender.prepend('-');
        sender.append('-');
        print( target, sender, msg, PT_NOTICE );

        emit RequestTrayMsg(tr("Notice from %1").arg(u.nick), msg);
        return;
    }

    else if (token1up == "JOIN") {
        user_t u = parseUserinfo(token[0]);
        QString chan = token[2];

        if (chan[0] == ':')
            chan = chan.mid(1);

        ial.addNickname(u.nick);
        ial.addChannel(u.nick, chan);
        ial.setHostname(u.nick, u.host);
        ial.setIdent(u.nick, u.user);

        if (u.nick == activeNick) { // I am joining a channel
            emit RequestWindow(chan, WT_CHANNEL, cid, true);
            print( chan, tstar, tr("Now talking in %1")
                                     .arg(chan),
                   PT_SERVINFO
                  );
        }
        else { // Someone joined a channel I am on
            print( chan, tstar, tr("Joins: %1 (%2@%3)")
                                     .arg(u.nick)
                                     .arg(u.user)
                                     .arg(u.host),
                   PT_SERVINFO
                  );

            member_t mt = {u.nick, u.user, u.host};
            IWin *w = getWinObj(chan);
            if (w != NULL)
              w->insertMember(u.nick, mt);
        }

        // join script event.
        scriptParent->runevent(te_join, QStringList()<<chan<<u.nick);
        return;
    }

    else if (token1up == "PART") {
        user_t u = parseUserinfo(token[0]);
        QString msg = getMsg(data);
        QString chan = token[2];

        if (chan[0] == ':') {
            chan = msg;
            msg.clear();
        }

        ial.delChannel(u.nick, chan);

        if (windowExist(chan) == false)
            return;

        if (msg.length() > 0)
            print( chan.toUpper(), tstar, tr("Parts: %1 (%2@%3) (%4)")
                                            .arg(u.nick)
                                            .arg(u.user)
                                            .arg(u.host)
                                            .arg(msg),
                   PT_SERVINFO
                  );
        else
            print( chan.toUpper(), tstar, tr("Parts: %1 (%2@%3)")
                                            .arg(u.nick)
                                            .arg(u.user)
                                            .arg(u.host),
                   PT_SERVINFO
                  );

        IWin *w = getWinObj(chan);
        if (w != NULL)
            w->removeMember(u.nick);

        bool e = windowExist(chan);
        if ((u.nick == activeNick) && (e == true)) {
            // Us who left.
            w->resetMemberlist();
            print(chan.toUpper(), tstar, tr("You've left %1").arg(chan), PT_LOCALINFO);
        }
        // part script event.
        scriptParent->runevent(te_part, QStringList()<<chan<<u.nick<<getMsg(data));
        return;
    }

    else if (token1up == "KICK") {
        user_t u = parseUserinfo(token[0]);
        QString msg = getMsg(data);
        QString chan = token[2];
        QString target = token[3];

        ial.delChannel(u.nick, chan);

        print( chan.toUpper(), tstar, tr("%1 kicked %2 (%3)")
                                        .arg(u.nick)
                                        .arg(target)
                                        .arg(msg),
               PT_SERVINFO
              );

        IWin *w = getWinObj(chan);
        if (w != NULL)
            w->removeMember(target);

        if (target == activeNick) {
            // This is us. :(
            w->resetMemberlist();
            emit RequestTrayMsg( tr("Kicked from %1").arg(chan), tr("You were kicked by %1: %2")
                                                                   .arg(u.nick)
                                                                   .arg(msg)
                                );
        }
        return;
    }

    else if (token1up == "QUIT") {
        user_t u = parseUserinfo(token[0]); // Get userinfo about who this is about
        QString msg = getMsg(data); // Get quit msg.

        ial.delNickname(u.nick);

        QHashIterator<QString,subwindow_t> w(winlist); // Set up iterator for all open windows, to find this user
        while (w.hasNext()) {
            w.next();
            if (w.value().widget->memberExist(u.nick) == true) { // Member is in here.
                QString wName = w.value().widget->objectName();
                print( wName, tstar, tr("Quit: %1 (%2@%3) (%4)")
                                       .arg(u.nick)
                                       .arg(u.user)
                                       .arg(u.host)
                                       .arg(msg),
                       PT_LOCALINFO
                      );

                w.value().widget->removeMember(u.nick);
            }
        }
        // quit script event.
        scriptParent->runevent(te_quit, QStringList()<<u.nick<<msg);
        return;
    }

    else if (token0up == "ERROR") {
        print("STATUS", tstar, tr("Error: %1").arg(getMsg(data)), PT_SERVINFO);
        return;
    }

    else if (token1up == "MODE") {
        user_t u = parseUserinfo(token[0]);
        QString target = token[2];

        ial.setHostname(u.nick, u.host);
        ial.setIdent(u.nick, u.user);

        if (isValidChannel(target)) {
            // Channel mode!
            // :tomatix!~tomatix@test.rigsofrods.org MODE #Test +v maan

            QString mode;
            for (int i = 3; i <= token.size()-1; i++) { mode += token[i] + ' '; } // Modes to parse (e.g. +ov user user)

            IWin *tg = getWinObj(target);
            if (tg->settings)
                tg->settings->setDefaultMode(mode); // it is safe; this will ignore user-based channel modes like op, voice, ban etc

            print( target.toUpper(), tstar, tr("%1 sets mode %2")
                                              .arg(u.nick)
                                              .arg(mode),
                   PT_SERVINFO
                  );

            mode = token[3];
            int parapos = 4;
            char p = '+'; // parsing a + or - mode

            for (int c = 0; c <= mode.count()-1; c++) {
                char m = mode[c].toLatin1();
                if ((m == '+') || (m == '-')) {
                    p = m;
                    continue;
                }

                // Handle stuff if IChanConfig is running
                IChanConfig *cc = getChanConfigPtr(target);
                if (cc != NULL) {
                    MaskType mt;

                    if (m == 'b')
                        mt = MT_BAN;
                    if (m == 'e')
                        mt = MT_EXCEPT;
                    if (m == 'I')
                        mt = MT_INVITE;

                    if (p == '-')
                        cc->delMask( token.at(parapos), mt );

                    if (p == '+') {
                        QString date = QDateTime::currentDateTime().toString("ddd d MMM yyyy, hh:mm");
                        cc->addMask( token.at(parapos), u.nick, date , mt);
                    }
                }

                // Check if we must increment parapos...
                if ((cmA.contains(m) || cmB.contains(m)) || ((p == '+') && (cmC.contains(m))))
                    parapos++;

                if (isValidCuMode(m)) {
                    // Mode is a valid "channel usermode", store it
                    QString param = token[parapos];
                    parapos++; // user modes to channel isn't in the cm* lists. increment.
                    IWin *w = getWinObj(target);
                    if (p == '+') {
                        ial.addMode(param, target, getCuLetter(m));
                        w->MemberSetMode(param, getCuLetter(m));
                    }

                    if (p == '-') {
                        ial.delMode(param, target, getCuLetter(m));
                        w->MemberUnsetMode(param, getCuLetter(m));
                    }
                }
            }
        }

        if (target.toUpper() == u.nick.toUpper()) // Mode on self!
            print("STATUS", tstar, tr("Your user mode sets to %1").arg(getMsg(data)), PT_SERVINFO);

        return;
    }

    else if (token1up == "NICK") {
        user_t u = parseUserinfo(token[0]);
        QString newnick = token[2];
        if (newnick[0] == ':')
            newnick = newnick.mid(1);

        ial.setHostname(u.nick, u.host);
        ial.setIdent(u.nick, u.user);
        ial.setNickname(u.nick, newnick);

        if (u.nick == activeNick) {
            print("STATUS", tstar, tr("Your nickname is now %1").arg(newnick), PT_SERVINFO);
            activeNick = newnick;
            conf->nickname = newnick;
            conf->save();
            emit refreshTitlebar();
        }

        QHashIterator<QString,subwindow_t> w(winlist); // Set up iterator for all open windows, to find this user
        while (w.hasNext()) {
            w.next();
            QString wName = w.value().widget->objectName();
            if (w.value().widget->memberExist(u.nick) == true) { // Member is in here.
                print(wName, tstar, tr("%1 is now known as %2")
                                      .arg(u.nick)
                                      .arg(newnick),
                       PT_SERVINFO
                      );
                w.value().widget->memberSetNick(u.nick, newnick);
            }
        }
        return;
    }

    else if (token1up == "TOPIC") {
        user_t u = parseUserinfo(token[0]);
        QString chan = token[2];
        QString newTopic = getMsg(data);
        print( chan, tstar, tr("%1 set topic to '%2'")
                              .arg(u.nick)
                              .arg(newTopic),
               PT_SERVINFO
              );

        ial.setHostname(u.nick, u.host);
        ial.setIdent(u.nick, u.user);

        /* TODO
        TWin *tg = winlist->value(chan.toUpper()).window;

        tg->setTopic(newTopic);
        */
        emit refreshTitlebar();

        return;
    }

    else if (token1up == "INVITE") {
        if (token[2] != activeNick)
            return; // Apparently, this wasn't to us...

        QString channel = token[3];
        if (channel[0] == ':')
            channel = channel.mid(1);

        user_t u = parseUserinfo(token[0]);
        print( activewin(), tstar, tr("%1 invited you to channel %3")
                                    .arg(u.nick)
                                    .arg(channel),
               PT_SERVINFO
              );
        return;
    }

    else if (token1up == "KILL") {
        user_t s = parseUserinfo(token[0]);
        QString msg = getMsg(data);
        print("STATUS", tstar, tr("Killed by %1 (%1@%2) (%3)")
                                 .arg(s.nick)
                                 .arg(s.user)
                                 .arg(s.host)
                                 .arg(msg),
               PT_SERVINFO
              );
        return;
    }


    // :server num target interesting data goes here
    int skip = 0;
    for (int i = 0; i <= 2; i++)
        skip += token[i].length() + 1;

    QString text = data.mid(skip);

    if (text[0] == ':')
        text = text.mid(1);

    print("STATUS", "", text);
}

void IConnection::parseNumeric(int numeric, QString &data)
{
/*
  format
  :nick!user@host num/cmd [target] :msg
*/
    QStringList token = data.split(' ');

    /** ***********************
    **        ERRORS        **
    ** *******************  **/

    if (numeric == ERR_NOSUCHNICK) {
        QString text = QString("%1: %2")
                         .arg(token[3])
                         .arg(getMsg(data));
        print(activewin(), "", text);
        return;
    }

    else if (numeric == ERR_NOSUCHSERVER) {
        QString text = QString("%1: %2")
                         .arg(token[3])
                         .arg(getMsg(data));
        print("STATUS", "", text);
        return;
    }

    else if (numeric == ERR_NOSUCHCHANNEL) {
        QString text = QString("%1: %2")
                         .arg(token[3])
                         .arg(getMsg(data));
        print("STATUS", "", text);
        return;
    }

    else if (numeric == ERR_CANNOTSENDTOCHAN) {
        QString text = QString("%1: %2")
                         .arg(token[3])
                         .arg(getMsg(data));

        bool e = windowExist(token.at(3));
        if (e == true)
            print(token[3].toUpper(), "", text);
        else
            print("STATUS", "", text);

        return;
    }

    else if (numeric == ERR_TOOMANYCHANNELS) {
        QString text = QString("%1: %2")
                         .arg(token[3])
                         .arg(getMsg(data));
        print("STATUS", "", text);
        return;
    }

    else if (numeric == ERR_WASNOSUCHNICK) {
        QString text = QString("%1: %2")
                         .arg(token[3])
                         .arg(getMsg(data));
        print("STATUS", "", text);
        return;
    }

    else if (numeric == ERR_TOOMANYTARGETS) {
        QString text = QString("%1: %2")
                         .arg(token[3])
                         .arg(getMsg(data));
        print(activewin(), "", text);
        return;
    }

    else if (numeric == ERR_NOTOPLEVEL) {
        QString text = QString("%1: %2")
                         .arg(token[3])
                         .arg(getMsg(data));
        print("STATUS", "", text);
        return;
    }

    else if (numeric == ERR_WILDTOPLEVEL) {
        QString text = QString("%1: %2")
                         .arg(token[3])
                         .arg(getMsg(data));
        print("STATUS", "", text);
        return;
    }

    else if (numeric == ERR_ERRORNEUSNICKNAME) {
        QString text = QString("%1: %2")
                         .arg(token[3])
                         .arg(getMsg(data));
        print(activewin(), "", text);
        return;
    }

    else if (numeric == ERR_UNKNOWNCOMMAND) {
        QString text = QString("%1: %2")
                         .arg(token[3])
                         .arg(getMsg(data));
        print("STATUS", "", text);
        return;
    }

    else if (numeric == ERR_NICKNAMEINUSE) {
        QString text = QString("%1: %2")
                         .arg(token[3])
                         .arg(getMsg(data));
        print(activewin(), "", text);

        if (! registered) {
            // Use alternative nickname
            if ((token[3].toUpper() != conf->altnick.toUpper()) && (conf->altnick.length() > 0)) {
                sockwrite(QString("NICK :%1").arg(conf->altnick));
                activeNick = conf->altnick;
                emit refreshTitlebar();
            }
            else {
                IWin *w = getWinObj("Status");
                w->setInputText("/Nick ");
                activeNick = "?";
                emit refreshTitlebar();
            }
        }
        return;
    }

    else if (numeric == ERR_NICKCOLLISION) {
        QString text = QString("%1: %2")
                         .arg(token[3])
                         .arg(getMsg(data));
        print("STATUS", "", text);
        return;
    }

    else if (numeric == ERR_USERNOTINCHANNEL) {
        QString chan = token[3];
        QString text = QString("%1: %2")
                         .arg(chan)
                         .arg(getMsg(data));

        bool e = windowExist(chan.toUpper());
        if (e == true)
            print(chan.toUpper(), "", text);
        else
            print( "STATUS", "", QString("%1 (%2)")
                               .arg(text)
                               .arg(chan)
                  );

        return;
    }

    else if (numeric == ERR_NOTONCHANNEL) {
        QString text = QString("%1: %2")
                         .arg(token[3])
                         .arg(getMsg(data));

        bool e = windowExist(token[3].toUpper());
        if (e == true)
            print(token[3], "", text);
        else
            print("STATUS", "", text);

        return;
    }

    else if (numeric == ERR_USERONCHANNEL) {
        QString chan = token[3];
        QString text = QString("%1: %2")
                         .arg(chan)
                         .arg(getMsg(data));

        print(chan.toUpper(), "", text);
        return;
    }

    else if (numeric == ERR_NOLOGIN) {
        QString text = token.at(3);
        text += ": "+ getMsg(data);
        print("STATUS", "", text);
        return;
    }

    else if (numeric == ERR_NEEDMOREPARAMS) {
        QString text = token.at(3);
        text += ": "+ getMsg(data);
        print("STATUS", "", text);
        return;
    }

    else if (numeric == ERR_KEYSET) {
        QString chan = token[3];
        QString text = getMsg(data);
        print(chan.toUpper(), "", text);
        return;
    }

    else if (numeric == ERR_CHANNELISFULL) {
        QString chan = token[3];
        QString text = QString("%1: %2")
                         .arg(chan)
                         .arg(getMsg(data));

        bool e = windowExist(chan.toUpper());
        if (e == true)
            print(chan, "", text);
        else
            print("STATUS", "", text);

        return;
    }

    else if (numeric == ERR_UNKNOWNMODE) {
        QString text = QString("%1: %2")
                         .arg(token[3])
                         .arg(getMsg(data));

        print("STATUS", "", text);
        return;
    }

    else if (numeric == ERR_INVITEONLYCHAN) {
        QString chan = token[3];
        QString text = QString("%1: %2")
                         .arg(chan)
                         .arg(getMsg(data));

        bool e = windowExist(chan.toUpper());
        if (e == true)
            print(chan, "", text);
        else
            print("STATUS", "", text);

        return;
    }

    else if (numeric == ERR_INVITEONLYCHAN) {
        QString chan = token[3];
        QString text = QString("%1: %2")
                         .arg(chan)
                         .arg(getMsg(data));

        bool e = windowExist(chan.toUpper());
        if (e == true)
            print(chan, "", text);
        else
            print("STATUS", "", text);

        return;
    }

    else if (numeric == ERR_BANNEDFROMCHAN) {
        QString chan = token[3];
        QString text = QString("%1: %2")
                         .arg(chan)
                         .arg(getMsg(data));

        bool e = windowExist(chan.toUpper());
        if (e == true)
            print(chan, "", text);
        else
            print("STATUS", "", text);

        return;
    }

    else if (numeric == ERR_BADCHANNELKEY) {
        QString chan = token[3];
        QString text = QString("%1: %2")
                         .arg(chan)
                         .arg(getMsg(data));

        bool e = windowExist(chan.toUpper());
        if (e == true)
            print(chan, "", text);
        else
            print("STATUS", "", text);

        return;
    }

    else if (numeric == ERR_CHANOPRIVSNEEDED) {
        /** @todo Use 'text' before its appended further, to print in channel. **/
        QString chan = token[3];
        QString text = QString("%1: %2")
                         .arg(chan)
                         .arg(getMsg(data));
        print(chan, "", text);
        return;
    }



  /** ***********************
   **        REPLIES       **
   ** *******************  **/

    else if (numeric == RPL_WELCOME) {
        // See what nickname we _actually_ got.
        // Not doing this will cause the client to act retarded.
        activeNick = token[2];
        active = true; // Connection is registered.

        ial.addNickname(activeNick);

        // Perform on connect
        QString fname = QString("%1/perform").arg(CONF_PATH);
        QFile f(fname);
        if (f.open(QIODevice::ReadOnly)) {
            QByteArray data = f.readAll();
            QString line;
            int len = data.length();
            data.append('\n');
            for (int i = 0; i <= len-1; i++) {
                QChar c = data.at(i);

                if (c == '\n') {
                    if (line.length() == 0)
                        continue;

                    cmdhndl.parse(line);
                    line.clear();
                    continue;
                }

                line += c;
            }
            f.close();

            // Auotjoin favourites
            fname = QString("%1/favourites.ini").arg(CONF_PATH);
            IniFile ini(fname);
            QString channel;
            QString password;
            int count = ini.CountSections();
            for (int i = 1; i <= count; i++) {
                QString channelItem = ini.ReadIni(i);
                QString passwordItem = ini.ReadIni(channelItem, "Key");
                bool autojoin = (bool)ini.ReadIni(channelItem, "AutoJoin").toInt();

                if (autojoin == true) {
                    if (channel.length() > 0)
                        channel.append(',');
                    channel.append(channelItem);

                    if (passwordItem.length() > 0) {
                        if (password.length() > 0)
                            password.append(',');
                        else
                            password.prepend(' ');
                        password.append(passwordItem);
                    }
                }
            }
            if (channel.length() > 0) {
                QString data = QString("JOIN %1%2")
                                 .arg(channel)
                                 .arg(password);
                sockwrite(data);
            }
        }
        QString hostinfo = QString("%1:%2")
                             .arg(host)
                             .arg(port);

        if (conf->autoReJoin) {
            // Rejoin channels which is open
            QHashIterator<QString,subwindow_t> i(winlist);
            QString channels;
            while (i.hasNext()) {
                i.next();
                subwindow_t sw = i.value();

                if (sw.type != WT_CHANNEL)
                    continue;

                QString target = sw.widget->getTarget();
                if (! channels.isEmpty())
                    channels.append(',');
                channels.append(target);
            }

            sockwrite(QString("JOIN %1").arg(channels));
        }

        scriptParent->runevent(te_connect, QStringList()<<hostinfo);
    }

    else if (numeric == RPL_MYINFO) {
        int c = 0;
        for (int i = 0; i <= 2; i++)
            c += token[i].length() + 1;

        print("STATUS", "", data.mid(c));
        return;
    }

    else if (numeric == RPL_ISUPPORT) {

        /*****
         *[IN] :irc.3phasegaming.net 005 Tomatix_ CMDS=KNOCK,MAP,DCCALLOW,USERIP,STARTTLS UHNAMES NAMESX SAFELIST HCN MAXCHANNELS=25 CHANLIMIT=#:25 MAXLIST=b:60,e:60,I:60 NICKLEN=30 CHANNELLEN=32 TOPICLEN=307 KICKLEN=307 AWAYLEN=307 :are supported by this server
         *[IN] :irc.3phasegaming.net 005 Tomatix_ MAXTARGETS=20 WALLCHOPS WATCH=128 WATCHOPTS=A SILENCE=15 MODES=12 CHANTYPES=# PREFIX=(ohv)@%+ CHANMODES=beIqa,kfL,lj,psmntirRcOAQKVCuzNSMTGZ NETWORK=3PhaseGaming CASEMAPPING=ascii EXTBAN=~,qjncrRa ELIST=MNUCT :are supported by this server
         ********/

        isupport = true;

        int c = 0;
        for (int i = 0; i <= 2; i++)
            c += token[i].length() + 1;
        print("STATUS", "", data.mid(c));

        for (int i = 3; i <= token.count()-1; i++) {
            QStringList lst = token[i].split('=');

            if (lst[0] == "CHANMODES") {
                /*
                * Unreal: CHANMODES=beIqa,kfL,lj,psmntirRcOAQKVCuzNSMTGZ
                *   IRCu: CHANMODES=b,AkU,l,imnpstrDdR
                *
                * A = Mode that adds or removes a nick or address to a list. Always has a parameter.
                * B = Mode that changes a setting and always has a parameter.
                * C = Mode that changes a setting and only has a parameter when set.
                * D = Mode that changes a setting and never has a parameter.
                *
                */

                // Currently we only need to check list A.
                QStringList ml = lst[1].split(',');
                cmA = ml[0];
                cmB = ml[1];
                cmC = ml[2];
                cmD = ml[3];

                for (int j = 0; j <= cmA.length()-1; ++j) {
                char c = cmA[j].toLatin1();

                switch (c) {
                    case 'e':
                        haveExceptionList = true;
                        continue;
                    case 'I':
                        haveInviteList = true;
                        continue;
                    }
                }
            }

            if (lst[0] == "MAXLIST") {
                // Unreal: MAXLIST=b:60,e:60,I:60
                // IRCu: See MAXBANS
                // IRCnet: MAXLIST=beI:30

                QStringList maxlist = lst[1].split(',');

                for (int l = 0; l <= maxlist.count()-1; ++l) {
                    QString item = maxlist[l];
                    QString len = maxlist[l].split(':')[1]; // ooh so ugly, ohh I don't care
                    // item can be b:60 or beI:30 (examples)
                    // len is always a number e.g. 60
                    for (int j = 0; j <= item.length()-1; j++) {
                        char c = item[j].toLatin1();
                        switch (c) {
                            case 'b':
                                maxBanList = len.toInt();
                                continue;
                            case 'e':
                                maxExceptList = len.toInt();
                                continue;
                            case 'I':
                                maxInviteList = len.toInt();
                                continue;
                            case ':':
                                break;
                        }
                    }
                }
            }

            if (lst[0] == "MAXBANS") {
                // Quite simple, really.
                maxBanList = lst[1].toInt();
                maxExceptList = lst[1].toInt();
                maxInviteList = lst[1].toInt();
            }

            if (lst.at(0) == "MODES")
                maxModes = lst[1].toInt();


            if (lst.at(0) == "NETWORK") {
                /// @todo set treeview of this Status to Status (Network) from lst.at(1)
                subwindow_t sw = winlist.value("STATUS");
                QString title = QString("Status (%1)").arg(lst[1]);

                sw.treeitem->setText(0, title);
                wsw->setTitle(sw.wid, title);
            }

            /** @todo  sorting rules for channel nickname list, @, %, +, etc */

            if (lst.at(0) == "PREFIX") {
                //  PREFIX=(ohv)@%+
                // ohv is cumode, @%+ is culetter
                cumode.clear();
                culetter.clear();

                QString parse = lst[1];

                enum pState { P_MODE, P_LETTER };
                pState state = P_MODE;
                for (int i = 0; i <= parse.length()-1; i++) {
                    char c = parse[i].toLatin1();
                    if (c == '(') {
                        state = P_MODE;
                        continue;
                    }

                    if (c == ')') {
                        state = P_LETTER;
                        continue;
                    }

                    if (state == P_MODE)
                        cumode << c;

                    if (state == P_LETTER)
                        culetter << c;
                }
                resetSortRules();
            }

            if (lst[0] == "CHANTYPES") {
                QString types = lst[1];
                for (int j = 0; j <= types.count()-1; j++)
                chantype << types[j].toLatin1();
            }
        }
        return;
    }

    else if (numeric == RPL_AWAY) {
        QString nickname = token[3];
        QString text = tr("%1 is away: %2")
                         .arg(nickname)
                         .arg(getMsg(data));

        print(activewin(), "", text);
        if (activewin() == nickname.toUpper())
            return;

        if (windowExist(nickname) == true)
            print(nickname, "", text);
        return;
    }

    else if ((numeric == RPL_WHOISUSER) || (numeric == RPL_WHOWASUSER)) {
        QString nick, userhost, name;
        nick = token[3];
        userhost = QString("%1@%2")
                    .arg(token[4])
                    .arg(token[5]);

        ial.setIdent(nick, token[4]);
        ial.setHostname(nick, token[5]);

        name = getMsg(data);

        QString target = "STATUS";
        if (conf->showWhois)
            target = activewin();

        print( target, "", tr("%1: %2 : %3")
                             .arg(nick)
                             .arg(userhost)
                             .arg(name)
              );

        return;
    }

    else if (numeric == RPL_WHOISSERVER) {
        QString nick, server, info;
        nick = token[3];
        server = token[4];
        info = getMsg(data);

        QString target = "STATUS";
        if (conf->showWhois)
            target = activewin();

        print( target, "", tr("%1: %2 : %3")
                             .arg(nick)
                             .arg(server)
                             .arg(info)
              );
        return;
    }

    else if (numeric == RPL_WHOISHELP) {
        QString nick, text;
        nick = token[3];
        text = getMsg(data);

        QString target = "STATUS";
        if (conf->showWhois)
            target = activewin();

        print( target, "", tr("%1 %2")
                             .arg(nick)
                             .arg(text)
              );
        return;
    }

    else if (numeric == RPL_WHOISOPERATOR) {
        QString nickname = token[3];

        QString text = QString("%1 %2")
                         .arg(nickname)
                         .arg(getMsg(data));

        QString target = "STATUS";
        if (conf->showWhois)
            target = activewin();

        print(target, "", text);
        return;
    }

    else if (numeric == RPL_WHOISIDLE) {
        QString nick, msg;
        QString text[10];
        QStringList textls;

        nick = token[3];
        textls = getMsg(data).split(',');

        for (int i = 0; i <= textls.count()-1; i++) {
            if (textls[i].at(0) == ' ')
                text[i] = textls[i].mid(1);
            else
                text[i] = textls[i];
        }


        for (int i = 0; i <= textls.count()-1; i++) {
            if (msg.length() != 0)
                msg.append(", ");

            if (text[i].toUpper() == "SIGNON TIME") {
                quint64 seconds = token[i+4].toUInt();
                QString date = QDateTime::fromTime_t(seconds).toString("ddd d MMM yyyy, hh:mm");
                msg.append(QString("Signed on: %1").arg(date));
                continue;
            }

            if (text[i].toUpper() == "SECONDS IDLE") {
                quint64 now = QDateTime::currentMSecsSinceEpoch() / 1000;
                quint64 seconds = token[i+4].toUInt();
                QString date = QDateTime::fromTime_t(now-seconds).toString("ddd d MMM yyyy, hh:mm");
                msg.append(QString("Last active: %1").arg(date));
                continue;
            }

            msg.append( QString("%1: %2")
                          .arg(text[i])
                          .arg(token[i+4])
                       );
        }

        QString target = "STATUS";
        if (conf->showWhois)
            target = activewin();

        print( target, "", QString("%1: %2")
                             .arg(nick)
                             .arg(msg)
              );

        return;
    }

    else if (numeric == RPL_ENDOFWHOIS) {
        QString nickname = token[3];

        QString text = QString("%1: %2")
                         .arg(nickname)
                         .arg(getMsg(data));

        QString target = "STATUS";
        if (conf->showWhois)
            target = activewin();

        print(target, "", text);
        return;
    }

    else if (numeric == RPL_WHOISCHANNELS) {
        QString nickname = token[3];

        QString text = QString("%1: %2")
                         .arg(nickname)
                         .arg(getMsg(data));

        QString target = "STATUS";
        if (conf->showWhois)
            target = activewin();

        print(target, "", text);
        return;
    }

    else if (numeric == RPL_ENDOFWHOWAS) {
        QString nickname = token[3];

        QString text = QString("%1: %2")
                         .arg(nickname)
                         .arg(getMsg(data));

        QString target = "STATUS";
        if (conf->showWhois)
            target = activewin();

        print(target, "", text);
        return;
    }

    else if (numeric == RPL_LISTSTART) {
        IChannelList *cl = *chanlistPtr;
        if (cl == NULL)
            listInDialog = false;
        else {
            if (cl->isVisible())
                listInDialog = true;
            else
                listInDialog = false;
        }
        print("STATUS", tstar, tr("Downloading /LIST..."), PT_LOCALINFO);

        return;
    }

    else if (numeric == RPL_LIST) {
        QString channel = token[3];
        QString users = token[4];
        QString topic = getMsg(data);

        if (listInDialog) {
            IChannelList *cl = *chanlistPtr;
            cl->addItem(channel, users, trimCtrlCodes(topic));
        }
        else {
            QString text = QString("%1 (%2 users): %3")
                             .arg(channel)
                             .arg(users)
                             .arg(topic);

            print("STATUS", "", text);
        }
        return;
    }

    else if (numeric == RPL_LISTEND) {
        listInDialog = false;
        print("STATUS", tstar, tr("End of /LIST"), PT_LOCALINFO);
        return;
    }

    else if (numeric == RPL_CHANNELMODEIS) {
        /** @todo Store these. **/
        // :irc.rigsofrods.org 324 Tomatixx #Test +stnl 1234
        QString chan = token[3];

        QString mode;
        for (int i = 4; i <= token.size()-1; i++) { mode += token[i] + ' '; }

        IWin *w = getWinObj(chan);
        if (w == NULL) {
            print("STATUS", tstar, tr("Modes for %1: %2")
                                  .arg(chan)
                                  .arg(mode)
                  );
            return;
        }

        if (FillSettings == false)
            print(chan.toUpper(), tstar, tr("Modes: %1").arg(mode), PT_SERVINFO);
        else {
            if (w->settings != NULL) {
                w->settings->setDefaultMode(mode);
                sockwrite(QString("MODE %1 +b").arg(chan));
            }
            else
                FillSettings = false;
        }
        return;
    }

    else if (numeric == RPL_CREATION) {
        /** @todo Give creation date in channel window. **/
        //    unsigned int cdate =
        //  QDateTime()
        QString chan = token[3];
        return;
    }

    else if (numeric == RPL_NOTOPIC) {
        QString chan = token[3];

        if (FillSettings == false)
            print(chan.toUpper(), tstar, tr("No topic is set."), PT_SERVINFO);
        else {
            IWin *w = getWinObj(chan);
            if (w->settings != NULL)
                sockwrite(QString("MODE %1").arg(chan));
            else
                FillSettings = false;
        }
        return;
    }

    else if (numeric == RPL_TOPIC) {
        QString chan = token[3];
        QString topic = getMsg(data);

        IWin *w = getWinObj(chan);
        if (w == NULL) {
            print( "STATUS", "", tr("Topic for %1: %2")
                               .arg(chan)
                               .arg(topic)
                  );
            return;
        }

        if (FillSettings == false)
            print(chan.toUpper(), tstar, tr("Topic is: %1").arg(topic), PT_SERVINFO);
        else {
            if (w->settings != NULL) {
                w->settings->setDefaultTopic(topic);
                sockwrite(QString("MODE %1").arg(chan));
            }
            else
                FillSettings = false;
        }
        w->setTopic(topic);
        emit refreshTitlebar();
        return;
    }

    else if (numeric == RPL_TOPICBY) {
        QString chan = token[3];
        QString nick = token[4];
        int utime = token[5].toInt();
        QString date = QDateTime::fromTime_t(utime).toString("ddd d MMM yyyy, hh:mm");

        IWin *w = getWinObj(chan);
        if (w == NULL) {
            print( "STATUS", "", tr("%1 Topic set by %2, %3")
                               .arg(chan)
                               .arg(nick)
                               .arg(date)
                  );
            return;
        }

        if (w->settings == 0)
            print(chan, tstar, tr("Topic set by %2, %3")
                          .arg(nick)
                          .arg(date),
                   PT_SERVINFO
                  );
        return;
    }

    else if (numeric == RPL_INVITING) {
        QString nick = token[3];
        QString chan = token[4];

        QString target = "STATUS";
        if (windowExist(chan))
            target = chan;

        print(target, tstar, tr("Invited %1 to %2")
                        .arg(nick)
                        .arg(chan),
               PT_SERVINFO
              );
        return;
    }

    else if (numeric == RPL_SUMMONING) {
        QString user = token[3];
        QString text = QString("%1: %2")
                         .arg(user)
                         .arg(getMsg(data));

        print("STATUS", "", text);
        return;
    }

    else if (numeric == RPL_VERSION) {
        QString text = QString("%1 %2 : %3")
                         .arg(token[3])
                         .arg(token[4])
                         .arg(getMsg(data));

        print("STATUS", "", text);
        return;
    }

    else if (numeric == RPL_WHOREPLY) {
        int l = 0;
        for (int i = 0; i <= 2; i++)
            l += token[i].length() + 1;

        QString text = data.mid(l);
        print("STATUS", "", text);
        return;
    }

    else if (numeric == RPL_ENDOFWHO) {
        QString text = token.at(3);
        text += ": "+ getMsg(data);
        print("STATUS", "", text);
        return;
    }

    else if (numeric == RPL_NAMREPLY) {
        QString chan = token[4];
        QString msg = getMsg(data);
        QStringList nicks = msg.split(' ');

        if (nicks.count() == 0) // Nothing do to, stop.
            return;

        subwindow_t win = winlist.value(chan.toUpper());

        if (! receivingNames) {
            win.widget->resetMemberlist();
            receivingNames = true;
        }

        for (int i = 0; i <= nicks.count()-1; i++) {
            QString item = nicks[i];
            char mode = 0x00;

            if (nicks[i].count() == 0)
                continue; // Empty nickname...

            char l = item[0].toLatin1();
            if (isValidCuLetter(l)) { // This user got a mode
                mode = l; // Set mode
                item = item.mid(1); // Set nickname without mode
            }

            member_t m;
            m.nickname = item;
            ial.addNickname(m.nickname);
            ial.addChannel(m.nickname, chan);

            if (mode != 0x00) {
                m.mode << mode;
                ial.resetModes(m.nickname, chan);
                ial.addMode(m.nickname, chan, mode);
            }

            // Ident and host is unknown, for now. It's safe not to have it.            
            win.widget->insertMember(item, m, false);
        }
    }

    else if (numeric == RPL_ENDOFNAMES) {
        QString chan = token[3];
        receivingNames = false;
        if (chan != "*")
            winlist.value(chan.toUpper()).widget->sortMemberList();
    }

    else if (numeric == RPL_LINKS) {
        int l = 0;
        for (int i = 0; i <= 2; i++)
            l += token[i].length() + 1;

        QString text = data.mid(l);
        print("STATUS", "", text);
        return;
    }

    else if (numeric == RPL_ENDOFLINKS) {
        QString server = token[3];
        QString text = QString("%1: %2")
                .arg(server)
                .arg(getMsg(data));

        print("STATUS", "", text);
        return;
    }

    else if ((numeric == RPL_MOTDSTART) && (conf->showMotd)) {
        print("STATUS", tstar, tr("Loading MOTD..."), PT_LOCALINFO);
        motd.reset();
        return;
    }

    else if ((numeric == RPL_MOTD) && (conf->showMotd)) {
        QString text = getMsg(data);
        motd.print("", text);
        return;
    }

    else if ((numeric == RPL_ENDOFMOTD) || (numeric == ERR_NOMOTD)) {
        print("STATUS", "", getMsg(data));

        if (conf->showMotd)
            motd.show();

        if (registered == true)
            return; // Stop here

        serverName = data.split(" ")[0].mid(1);

        // Show favourites if its chosed to do so.
        if (conf->showFavourites)
            emit RequestFavourites();

        registered = true;

        if (cumode.count() == 0) {
            // We're just connected, but no ISUPPORT were received. Use the default ones.
            cumode << 'o' << 'v';
            culetter << '@' << '+';
            resetSortRules();
        }


        if (conf->connectInvisible == true)
            sockwrite(QString("MODE %1 +i").arg(activeNick));

        emit RequestTrayMsg(tr("Connected to server"), tr("Successfully connected to %1").arg(conf->server));
        emit connectedToIRC();

        // for displaying the IAL inside a GUI. shows when connected to a server.
        //addresslist.show();
        return;
    }

    else if ((numeric == RPL_BANLIST) || (numeric == RPL_EXCEPTLIST) || (numeric == RPL_INVITELIST)) {
        QString chan = token[3];
        int created = token[6].toInt();
        QString date = QDateTime::fromTime_t(created).toString("ddd d MMM yyyy, hh:mm");

        if (windowExist(chan) == false)
           // print("STATUS", token.at(4) + " set by " + token.at(5) + ", " + date);
            print("STATUS", "", tr("%1 set by %2, %3")
                                  .arg(token[4])
                                  .arg(token[5])
                                  .arg(date)
                  );
        else {
            IWin *w = getWinObj(chan);

            if ((FillSettings == true) && (w->settings != NULL))
                w->settings->addMask(token[4], token[5], date);

            if (FillSettings == false)
                print(chan, "", tr("%1 set by %2, %3")
                                  .arg(token[4])
                                  .arg(token[5])
                                  .arg(date)
                      );
        }

        return;
    }

    else if (numeric == RPL_ENDOFBANLIST) {
        QString chan = token[3];
        if (windowExist(chan.toUpper()) == false) {
            print( "STATUS", "", QString("%1: %2")
                                   .arg(chan)
                                   .arg(getMsg(data))
                  );
            return;
        }

        IWin *w = getWinObj(chan);

        if (FillSettings == true) {
            if (w->settings != NULL) {
                w->settings->finishModel(MT_BAN);

                if (haveExceptionList)
                    sockwrite(QString("MODE %1 +e").arg(chan));
                else if (haveInviteList)
                    sockwrite(QString("MODE %1 +I").arg(chan));
                else
                    FillSettings = false;
            }
            else
                FillSettings = false;
        }
        return;
    }

    else if (numeric == RPL_ENDOFEXCEPTLIST) {
        QString chan = token[3];
        if (windowExist(chan.toUpper()) == false) {
            print( "STATUS", "", QString("%1: %2")
                                   .arg(chan)
                                   .arg(getMsg(data))
                  );
            return;
        }

        IWin *w = getWinObj(chan);

        if (FillSettings == true) {
            if (w->settings != NULL) {
                w->settings->finishModel(MT_EXCEPT);

                if (haveInviteList)
                    sockwrite(QString("MODE %1 +I").arg(chan));
                else
                    FillSettings = false;
            }
            else
                FillSettings = false;
        }
        return;
    }

    else if (numeric == RPL_ENDOFINVITELIST) {
        QString chan = token[3];
        if (windowExist(chan.toUpper()) == false) {
            print( "STATUS", "", QString("%1: %2")
                                   .arg(chan)
                                   .arg(getMsg(data))
                  );
            return;
        }
        IWin *w = getWinObj(chan);

        if (FillSettings == true) {
            if (w->settings != NULL) {
                w->settings->finishModel(MT_INVITE);
                FillSettings = false;
            }
        }
        return;
    }

    else if (numeric == RPL_REHASHING) {

        QString text = QString("%1: %2")
                         .arg(token[3])
                         .arg(getMsg(data));

        print("STATUS", "", text);
        return;
    }

    else if (numeric == RPL_TIME) {
        QString text = QString("%1: %2")
                         .arg(token[3])
                         .arg(getMsg(data));

        print("STATUS", "", text);
        return;
    }

    else if ((numeric >= 200) && (numeric <= 244)) {
        /// @note See RFC1459 for which numerics this is about.
        int l = 0;
        for (int i = 0; i <= 2; i++)
            l += token[i].length() + 1;

        QString text = data.mid(l);
        print("STATUS", "", text);
        return;
    }

    else if ((numeric >= 252) && (numeric <= 254)) {
        /// @note RPL_LUSEROP, RPL_LUSERUNKNOWN, RPL_LUSERCHANNELS
        QString text = token.at(3);
        text += " "+ getMsg(data);
        print("STATUS", "", text);
        return;
    }

    else if (numeric == RPL_ADMINME) {
        QString text = QString("%1 %2")
                         .arg(token[3])
                         .arg(getMsg(data));
        print("STATUS", "", text);
        return;
    }

    else if (numeric == RPL_USERHOST) {
        QString result = getMsg(data);
        if (result.isEmpty())
            return;

        QStringList items = result.split(' ');
        for (int i = 0; i <= items.count()-1; ++i) {
            QString item = items[i];
            QString nickname = item.split('=')[0];
            nickname = nickname.replace('*', "");
            if (ial.hasNickname(nickname)) {
                QString identhost = item.split('=')[1];
                QString ident = identhost.split('@')[0];
                QString host = identhost.split('@')[1];
                ident = ident.replace('+',"");

                ial.setIdent(nickname, ident);
                ial.setHostname(nickname, host);
            }
        }
        print("STATUS", "", result);
        return;
    }

    else if ((numeric == RPL_WHOISACTUALHOST) || (numeric == RPL_WHOISMODES) || (numeric == RPL_WHOISIDENTIFIED)) {
        QString nick = token[3];
        QString text = getMsg(data);


        QString target = "STATUS";
        if (conf->showWhois)
            target = activewin();

        print(target, "", QString("%1: %2")
                            .arg(nick)
                            .arg(text)
              );
        return;
    }

    else if (numeric == RPL_WHOISLOGGEDIN) {
        QString nick = token[3];
        QString user = token[4];
        QString text = getMsg(data);

        QString target = "STATUS";
        if (conf->showWhois)
            target = activewin();

        print(target, "", QString("%1: %2 %3")
                            .arg(nick)
                            .arg(text)
                            .arg(user)
              );

        return;
    }
    // :server num interesting data goes here
    int skip = 0;
    for (int i = 0; i <= 2; i++)
        skip += token[i].length() + 1;

    QString text = data.mid(skip);

    if (text[0] == ':')
        text = text.mid(1);

    print("STATUS", "", text);
}
