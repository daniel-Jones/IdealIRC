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
#include <QTextCodec>

#include "constants.h"
#include "iconnection.h"
#include "numerics.h"
#include "script/tscriptparent.h"

IConnection::IConnection(QObject *parent, IChannelList **clptr, int connId,
                         config *cfg, TScriptParent *sp) :
    QObject(parent),
    maxBanList(-1), /** -1 means undefined. **/
    maxExceptList(-1),
    maxInviteList(-1),
    maxModes(3),
    haveExceptionList(false),
    haveInviteList(false),
    FillSettings(false),
    cmdhndl(this, cfg),
    conf(cfg),
    cid(connId),
    active(false),
    registered(false),
    isupport(false),
    CloseChildren(false),
    tryingConnect(false),
    chanlistPtr(clptr),
    listInDialog(false),
    connectionClosing(false),
    motd(cfg, (QWidget*)parent),
    scriptParent(sp),
    waitLF(false),
    cmA("b"),
    cmB("k"),
    cmC("l"),
    cmD("imnpstr")

{

    cmdhndl.setWinList(&winlist);
    cmdhndl.setCid(&cid);

    std::cout << "Connection class ID: " << cid << std::endl;
    resetSortRules();

    connect(&socket, SIGNAL(connected()),
            this, SLOT(onSocketConnected()));

    connect(&socket, SIGNAL(disconnected()),
            this, SLOT(onSocketDisconnected()));

    connect(&socket, SIGNAL(readyRead()),
            this, SLOT(onSocketReadyRead()));
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

    s->print(tr("Connecting to %1:%2...").arg(host).arg(port), PT_LOCALINFO);
    tryingConnect = true;
    socket.connectToHost(host, port);

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
        print("STATUS", "Not connected to server", PT_LOCALINFO);
        return false;
    }

    if (data.length() == 0)
      return false;

    /// @todo Erase any \0 \r and \n

  std::cout << "[UT] " << data.toStdString().c_str();

    data.append("\r\n");
    socket.write(data.toUtf8());

    return true;
}

void IConnection::onSocketConnected()
{
    emit updateConnectionButton();

    if (conf->password.length() > 0)
        sockwrite("PASS :" + conf->password);

    QString username;                                // Store our username to pass to IRC server here.
    if (conf->username.at(0) == '@')                 // If our @ token is first, we'll just use the nickname.
        username = conf->nickname;                   // This to prevent any errors on following:
    else
        username = conf->username.split('@').at(0);  // Pick username from email. May not contain a '@' though.

    sockwrite("NICK " + conf->nickname);
    sockwrite("USER " + username + " \"\" \"\" :" + conf->realname);
}

void IConnection::onSocketDisconnected()
{
    if (CloseChildren) { // Close all windows related to this connection
        QHashIterator<QString, subwindow_t> i(winlist);
        while (i.hasNext()) {
            i.next();
            subwindow_t win = i.value();

            if (win.widget == NULL)
                continue; // Window is already destroyed, can't do much with this. (Why is it even in the list? look closer on this if it becomes an issue!)

            if (win.type != WT_STATUS)
                win.widget->close();
        }
        socket.close();
        emit connectionClosed();
        return; // All windows are closed, status window will close itself when ready to close.
    }

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

    QHashIterator<QString, subwindow_t> i(winlist);
    while (i.hasNext()) {
        i.next();
        subwindow_t win = i.value();

        win.widget->print("Disconnected.", PT_LOCALINFO);
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

void IConnection::closeConnection(bool cc)
{
    if (tryingConnect == true) {
        print("STATUS", "Disconnected.", PT_LOCALINFO);
        socket.close();
        tryingConnect = false;
        return;
    }

    if (socket.isOpen()) {
        connectionClosing = true;
        CloseChildren = cc;
        QString data = QString("QUIT :%1")
                       .arg( conf->quit );
        sockwrite(data);
    }
}



void IConnection::onSocketReadyRead()
{
    /*
      Define a global (class) variable (linedata) to read in each IRC line to parse.
      Whenever encountering a \r we should wait (waitLF) for an LF to come as next byte.
      If the next byte isn't an LF (\n), we should ignore the CR+LF and add the next byte after \n
      to the buffer until a new CR comes by.
      When an CR+LF is reached, send the line data to parse, unset linedata and continue
      reading data until nothing is left in buffer.

      Variables:
        linedata
        waitLF
    */

    QByteArray in = socket.readAll();
    tryingConnect = false;

    QTextCodec *tc = QTextCodec::codecForName(conf->charset.toStdString().c_str());

    for (int i = 0; i <= in.length()-1; i++) {

      if (in.at(i) == '\0') { // Reached end of data in buffer
        //parse(QString::fromUtf8(linedata.toStdString().c_str()));
        QString text = tc->toUnicode(linedata);
        parse(text);
        linedata.clear();
      }

      if (in.at(i) == '\r') { // found CR
        waitLF = true;
        continue;
      }

      if ((waitLF == true) && (in.at(i) == '\n')) { // Expecting LF, found LF
        waitLF = false;
        //parse(QString::fromUtf8(linedata.toStdString().c_str()));
        QString text = tc->toUnicode(linedata);
        parse(text);
        linedata.clear();
        continue;
      }
      if ((waitLF == true) && (in.at(i) != '\n')) { // Expecting LF, found something else
        waitLF = false;
        continue;
      }
      // By default, add data to linedata.
      linedata += in.at(i);
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
    char prefix = channel.at(0).toLatin1();
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
        QChar c = text.at(i);
        if (c == 0x02)
            continue;
        if (c == 0x1F)
            continue;
        if (c == 0x03) {

            for (i++; i <= text.length()-1; i++) {
                c = text.at(i);
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

void IConnection::print(const QString window, const QString &line, const int ptype)
{
    IWin *w = NULL; // Default value of *w is NULL to make sure we can error-check.
    w = getWinObj(window.toUpper()); // Attempt to get window object...
    if (w == NULL) // No such window, go back to default...
        w = getWinObj("STATUS"); // Default to Status window, this one always exsist.
    if ((w == NULL) && (window == "STATUS"))
        return; // Nowhere to send this text, ignore silently...

    w->print(line, ptype); // Do printing
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
  if (uinfo.at(0) == ':')
    uinfo = uinfo.mid(1);

  //         nick, user, host
  user_t u = {uinfo, "", ""};

  // nickname!ident@hostname
  QStringList a = uinfo.split("!"); // 0 = nickname, 1 = ident@hostname
  if (a.length() < 2)
    return u;

  QStringList b = a.at(1).split("@"); // 0 = ident, 1 = hostname
  if (a.length() < 2)
    return u;

  //   nick,   user,    host
  //u = {a.at(0), b.at(0), b.at(1)}; not compatible with my current 'clang' version. GCC takes this code.
  u.nick = a.at(0);
  u.user = b.at(0);
  u.host = b.at(1);


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

std::cout << "[IN] " << data.toStdString().c_str() << std::endl;

  QStringList token = data.split(" ");
  QString token0up = token.at(0).toUpper();
  QString token1up = token.at(1).toUpper();


  if (token.size() < 2) // Rubbish, there should be at least two tokens separated by a space.
    return;

  int num = token.at(1).toInt();

  if (num > 0) { // valid NUMERIC
    parseNumeric(num, data);
    return;
  }

  else if (token0up == "NOTICE") {
    QString from = token.at(1);
    if (token1up == "AUTH")
      from = conf->server.split(":").at(0);

    QString text = getMsg(data);

    from.prepend("-");
    from.append("- ");
    print("STATUS", from+text, PT_NOTICE);
    return;
  }

  else if (token0up == "PING") {
    sockwrite("PONG "+token.at(1));
    return;
  }


  else if (token1up == "PONG") {
    bool ok = false;
    qint64 ms_now = QDateTime::currentDateTime().toMSecsSinceEpoch();
    qint64 ms_pong = getMsg(data).toULongLong(&ok);
    if (! ok)
      print(activewin(), "PONG from server: " + getMsg(data), PT_SERVINFO);
    else
      print(activewin(), "PONG from server: " + QString::number(ms_now-ms_pong) + "ms.", PT_SERVINFO);
    return;
  }

/***********************************************************************************************
 *
 *
 *
 *
 *
 *
 *
 *
 **/

  else if (token1up == "PRIVMSG") {
    user_t u = parseUserinfo(token.at(0));
    QString text = getMsg(data);

    // CTCP check, ACTION check
    if (text[0] != ' ') {
      QString request;
      QStringList tx = text.split(" ");

      request.append(0x01);
      request.append("ACTION");
      if (tx.at(0).toUpper() == request) { /// NOT CTCP, ACTION
        // ACTION
        text.remove(0x01);
        text = text.mid(7);
        if (isValidChannel(token.at(2)))
          emit RequestWindow(token.at(2), WT_CHANNEL, cid, false);
        else
          emit RequestWindow(token.at(2), WT_PRIVMSG, cid, false);

        print(token.at(2).toUpper(), u.nick + " " + text, PT_ACTION);
        return;
      }

      if (tx.at(0).at(0) == 0x01) {
        // CTCP indicator
        QString ctcp = tx.at(0);
        ctcp.remove(QChar(0x01));
        print("STATUS", "CTCP " + ctcp + " from " + u.nick, PT_CTCP);
      }
      request.clear();

      request.append(0x01);
      request.append("VERSION");
      request.append(0x01);
      if (tx.at(0).toUpper() == request) { /// VERSION
        // VERSION
        QString reply;
        reply.append(0x01);
        reply.append("VERSION IdealIRC " + QString(VERSION_STRING) + " @ " + QString(SYSTEM_NAME) + " By Tomatix");
        reply.append(0x01);
        sockwrite("NOTICE " + u.nick + " :"+reply);
        return;
      }
      request.clear();

      request.append(0x01);
      request.append("TIME");
      request.append(0x01);
      if (tx.at(0).toUpper() == request) { /// TIME
        // TIME
        QString reply;
        QString clock = QDateTime::currentDateTime().time().toString("hh:mm:ss");
        QString date = QDateTime::currentDateTime().date().toString("dddd MMMM d, yyyy");
        reply.append(0x01);
        reply.append("VERSION TIME " + clock + ", " + date);
        reply.append(0x01);
        sockwrite("NOTICE " + u.nick + " :"+reply);
        return;
      }
      request.clear();

      request.append(0x01);
      request.append("PING");
      if (tx.at(0).toUpper() == request) { /// PING
        // PING
        QString reply;
        reply.append(0x01);
        reply.append("PING " + tx.at(1));
        sockwrite("NOTICE " + u.nick + " :"+reply); // pong
        return;
      }
      request.clear();

    }

    QString name = u.nick;
    if (isValidChannel(token.at(2).toUpper()) == false) { // Privmsg
      emit RequestWindow(name, WT_PRIVMSG, cid, false);
      print(name.toUpper(), "<" + name + "> " + text);
      subwindow_t w = winlist.value(token.at(2).toUpper());
      emit HighlightWindow(w.wid, HL_MSG);
      emit RequestTrayMsg("Private MSG from " + name, text);

    }
    else { // Channel
      QString tx = "<" + name + "> " + text;
      subwindow_t w = winlist.value(token.at(2).toUpper());
      member_t m = w.widget->ReadMember(name);
      if ((conf->showUsermodeMsg == true) && (m.mode.length() > 0)) {
        tx = "<" + QString(m.mode.at(0)) + name + "> " + text;
      }
      emit RequestWindow(token.at(2), WT_CHANNEL, cid, false);
      print(token.at(2).toUpper(), tx);
      if (text.contains(activeNick, Qt::CaseInsensitive))
          emit HighlightWindow(w.wid, HL_HIGHLIGHT);
      else
          emit HighlightWindow(w.wid, HL_MSG);

      // Check if our nickname is in the text, if so, put up a tray notify
      if (data.indexOf(activeNick, Qt::CaseInsensitive) > -1)
        emit RequestTrayMsg(token.at(2), tx);

    }
    // privmsg script event
    scriptParent->runevent(te_msg, QStringList()<<name<<token[2]<<text);
    return;
  }
/** @todo readd ********//////////////////////////***************************************/


  else if (token1up == "NOTICE") {
    user_t u = parseUserinfo(token.at(0)); // Get userinfo about who this is about
    QString msg = getMsg(data);

    if (msg.at(0) == 0x01) { // CTCP reply

      QString ctcp = msg.split(" ").at(0).toUpper();
      ctcp.remove(QChar(0x01));
      msg = msg.mid(ctcp.length() + 1);
      msg.remove(QChar(0x01));
      if (ctcp.toUpper() == "PING") {
        // Parse this.
        if (msg.length() == 0) {
          print("STATUS","* CTCP " + ctcp + " reply from " + u.nick + ": ? seconds", PT_CTCP);
          return;
        }
        qint64 cct = QDateTime::currentDateTime().toMSecsSinceEpoch();
        qint64 ct = msg.toULongLong();
        float lag = (cct-ct) / 1000.0f;
        char clag[16];
        sprintf(clag, "%.2f", lag);
        QString s_lag(clag);
        print("STATUS","* CTCP " + ctcp + " reply from " + u.nick + ": " + s_lag + " seconds", PT_CTCP);
        return;
      }
      if (msg.length() > 0)
        print("STATUS","* CTCP " + ctcp + " reply from " + u.nick + ":" + msg, PT_CTCP);
      if (msg.length() == 0)
        print("STATUS","* CTCP " + ctcp + " reply from " + u.nick, PT_CTCP);
      return;
    }

    if (u.nick == serverName)
      print("STATUS", "-"+u.nick+"- " + msg, PT_NOTICE);
    else
      print(activewin().toUpper(), "-"+u.nick+"- " + msg, PT_NOTICE);
    emit RequestTrayMsg("Notice from " + u.nick, msg);
    return;
  }

  else if (token1up == "JOIN") {
    user_t u = parseUserinfo(token.at(0));
    QString chan = token.at(2);
    if (chan.at(0) == ':')
      chan = chan.mid(1);

    if (u.nick == activeNick) { // I am joining a channel
      emit RequestWindow(chan, WT_CHANNEL, cid, true);
      print(chan.toUpper(), "Now talking in " + chan, PT_SERVINFO);
    }
    else { // Someone joined a channel I am on
      print(chan.toUpper(), "Joins: " + u.nick + " (" + u.user + "@" + u.host + ")", PT_SERVINFO);

      member_t mt;
      mt.nickname = u.nick;
      mt.host = u.host;
      mt.ident = u.user;

      IWin *w = getWinObj(chan);
      if (w != NULL)
          w->insertMember(u.nick, mt);

    }
    // join script event.
    scriptParent->runevent(te_join, QStringList()<<chan<<u.nick);
    return;
  }

  else if (token1up == "PART") {
    user_t u = parseUserinfo(token.at(0));
    QString msg = getMsg(data);
    QString chan = token.at(2);

    if (chan.at(0) == ':') {
      chan = msg;
      msg = "";
    }

    if (windowExist(chan) == false)
      return;

    if (msg.length() > 0)
      print(chan.toUpper(), "Parts: " + u.nick + " (" + u.user + "@" + u.host + ") (" + msg + ")", PT_SERVINFO);
    else
      print(chan.toUpper(), "Parts: " + u.nick + " (" + u.user + "@" + u.host + ")", PT_SERVINFO);

    IWin *w = getWinObj(chan);
    if (w != NULL)
        w->removeMember(u.nick);

    bool e = windowExist(chan);
    if ((u.nick == activeNick) && (e == true)) {
      // Us who left.
      w->resetMemberlist();
      print(chan.toUpper(), "You've left " + chan, PT_LOCALINFO);
    }
    // part script event.
    scriptParent->runevent(te_part, QStringList()<<token[2]<<u.nick<<getMsg(data));
    return;
  }

  else if (token1up == "KICK") {
    user_t u = parseUserinfo(token.at(0));
    QString msg = getMsg(data);
    QString chan = token.at(2);
    QString target = token.at(3);
    print(chan.toUpper(), u.nick + " kicked " + target + " (" + msg + ")", PT_SERVINFO);

    IWin *w = getWinObj(chan);
    if (w != NULL)
        w->removeMember(target);

    if (target == activeNick) {
      // This is us. :(
      w->resetMemberlist();
      emit RequestTrayMsg("Kicked from " + chan, "You were kicked by " + u.nick + ": " + msg);
    }
    return;
  }

  else if (token1up == "QUIT") {
    user_t u = parseUserinfo(token.at(0)); // Get userinfo about who this is about
    QString msg = getMsg(data); // Get quit msg.
    QHashIterator<QString,subwindow_t> w(winlist); // Set up iterator for all open windows, to find this user
    while (w.hasNext()) {
      w.next();
      if (w.value().widget->memberExist(u.nick) == true) { // Member is in here.
        print(w.value().widget->objectName(), "Quit: " + u.nick + " (" + u.user + "@" + u.host + ") (" + msg + ")", PT_LOCALINFO);
        w.value().widget->removeMember(u.nick);
      }

    }

    // quit script event.
    scriptParent->runevent(te_quit,QStringList()<<u.nick<<getMsg(data));
    return;
  }

  else if (token0up == "ERROR") {
    print("STATUS", "Error: " + getMsg(data), PT_SERVINFO);
    return;
  }

  else if (token1up == "MODE") {
    user_t u = parseUserinfo(token.at(0));
    QString target = token.at(2);

    if (isValidChannel(target)) {
        // Channel mode!
        // :tomatix!~tomatix@test.rigsofrods.org MODE #Test +v maan

        QString mode;
        for (int i = 3; i <= token.size()-1; i++) { mode += token.at(i) + " "; } // Modes to parse (e.g. +ov user user)

        IWin *tg = getWinObj(target);
        if (tg->settings)
        tg->settings->setDefaultMode(mode); // it is safe; this will ignore user-based channel modes like op, voice, ban etc


        print(target.toUpper(), u.nick + " sets mode " + mode, PT_SERVINFO);

        mode = token.at(3);
        int parapos = 4;
        char p = '+'; // parsing a + or - mode

        for (int c = 0; c <= mode.count()-1; c++) {
        char m = mode.at(c).toLatin1();
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
            QString param = token.at(parapos);
            parapos++; // user modes to channel isn't in the cm* lists. increment.
            IWin *w = getWinObj(target);
            if (p == '+')
                w->MemberSetMode(param, getCuLetter(m));

            if (p == '-')
                w->MemberUnsetMode(param, getCuLetter(m));
        }
      }
    }
    if (target.toUpper() == u.nick.toUpper()) {
      // Mode on self!

      print("STATUS", "Your usermode sets to: " + getMsg(data), PT_SERVINFO);
    }

    return;
  }


  else if (token1up == "NICK") {
    user_t u = parseUserinfo(token.at(0));
    QString newnick = token.at(2);
    if (newnick.at(0) == ':') {
      newnick = newnick.mid(1);
    }

    if (u.nick == activeNick) {
      print("STATUS", "Your nickname is now " + newnick);
      activeNick = newnick;
      conf->nickname = newnick;
      conf->save();
      emit refreshTitlebar();
    }

    QHashIterator<QString,subwindow_t> w(winlist); // Set up iterator for all open windows, to find this user
    while (w.hasNext()) {
      w.next();
      if (w.value().widget->memberExist(u.nick) == true) { // Member is in here.
        print(w.value().widget->objectName(), u.nick + " is now known as " + newnick, PT_SERVINFO);
        w.value().widget->memberSetNick(u.nick, newnick);
      }
    }

    return;
  }

  else if (token1up == "TOPIC") {
    user_t u = parseUserinfo(token.at(0));
    QString chan = token.at(2);
    QString newTopic = getMsg(data);

    print(chan, u.nick + " set topic to '" + newTopic + "'", PT_SERVINFO);

    /*
    TWin *tg = winlist->value(chan.toUpper()).window;

    tg->setTopic(newTopic);
    */
    emit refreshTitlebar();

    return;
  }

  else if (token1up == "INVITE") {
      /** @todo */
    if (token.at(2) != activeNick)
      return; // Apparently, this wasn't to us...

    user_t u = parseUserinfo(token.at(0));
    print("$ACTIVE$", u.nick + " invited you to channel " + token.at(3), PT_SERVINFO);
    return;
  }

  else if (token1up == "KILL") {
    user_t s = parseUserinfo(token.at(0));
    QString msg = getMsg(data);

    print("STATUS", "Killed by " + s.nick + " (" + s.user + "@" + s.host + "): " + msg, PT_SERVINFO);

    return;
  }


  // :server num target interesting data goes here
  int skip = token.at(0).length() + 1;
  skip += token.at(1).length() + 1;
  skip += token.at(2).length() + 1;

  QString text = data.mid(skip);

  if (text.at(0) == ':')
    text = text.mid(1);

  print("STATUS", text);
}

void IConnection::parseNumeric(int numeric, QString &data)
{
/*
  format
  :nick!user@host num/cmd [target] :msg
*/
  QStringList token = data.split(" ");


  /** ***********************
   **        ERRORS        **
   ** *******************  **/

  if (numeric == ERR_NOSUCHNICK) {
    QString text = token.at(3);
    text += ": "+ getMsg(data);
    print(activewin(), text);
    return;
  }

  else if (numeric == ERR_NOSUCHSERVER) {
    QString text = token.at(3);
    text += ": "+ getMsg(data);
    print("STATUS", text);
    return;
  }

  else if (numeric == ERR_NOSUCHCHANNEL) {
    QString text = token.at(3);
    text += ": "+ getMsg(data);
    print("STATUS", text);
    return;
  }


  else if (numeric == ERR_CANNOTSENDTOCHAN) {
    QString text = token.at(3);
    text += ": "+ getMsg(data);

    bool e = windowExist(token.at(3));
    if (e == true)
      print(token.at(3).toUpper(), text);
    else
      print("STATUS", text);

    return;
  }

  else if (numeric == ERR_TOOMANYCHANNELS) {
    QString text = token.at(3);
    text += ": "+ getMsg(data);
    print("STATUS", text);
    return;
  }

  else if (numeric == ERR_WASNOSUCHNICK) {
    QString text = token.at(3);
    text += ": "+ getMsg(data);
    print("STATUS", text);
    return;
  }

  else if (numeric == ERR_TOOMANYTARGETS) {
    QString text = token.at(3);
    text += ": "+ getMsg(data);
    print(activewin(), text);
    return;
  }

  else if (numeric == ERR_NOTOPLEVEL) {
    QString text = token.at(3);
    text += ": "+ getMsg(data);
    print("STATUS", text);
    return;
  }

  else if (numeric == ERR_WILDTOPLEVEL) {
    QString text = token.at(3);
    text += ": "+ getMsg(data);
    print("STATUS", text);
    return;
  }

  else if (numeric == ERR_ERRORNEUSNICKNAME) {
    QString text = token.at(3);
    text += ": "+ getMsg(data);
    print(activewin(), text);
    return;
  }

  else if (numeric == ERR_UNKNOWNCOMMAND) {
    QString text = token.at(3);
    text += ": "+ getMsg(data);
    print("STATUS", text);
    return;
  }

  else if (numeric == ERR_NICKNAMEINUSE) {
    QString text = token.at(3);
    text += ": "+ getMsg(data);
    print(activewin(), text);
    if (! registered) {
      // Use alternative nickname
        if ((token.at(3).toUpper() != conf->altnick.toUpper()) && (conf->altnick.length() > 0)) {
          sockwrite("NICK :" + conf->altnick);
          activeNick = conf->altnick;
          emit refreshTitlebar();
        }
        else {
          IWin *w = getWinObj("Status");
          w->setInputText("/Nick ");
          activeNick = "???";
          emit refreshTitlebar();
        }
    }
    return;
  }

  else if (numeric == ERR_NICKCOLLISION) {
    QString text = token.at(3);
    text += ": "+ getMsg(data);
    print("STATUS", text);
    return;
  }

  else if (numeric == ERR_USERNOTINCHANNEL) {
    QString text = token.at(3);
    QString chan = token.at(4);
    text += ": "+ getMsg(data);

    bool e = windowExist(chan.toUpper());
    if (e == true)
      print(chan.toUpper(), text);
    else
      print("STATUS", text + " (" + chan + ")");

    return;
  }

  else if (numeric == ERR_NOTONCHANNEL) {
    QString text = token.at(3);
    text += ": "+ getMsg(data);

    bool e = windowExist(token.at(3).toUpper());
    if (e == true)
      print(token.at(3), text);
    else
      print("STATUS", text);

    return;
  }

  else if (numeric == ERR_USERONCHANNEL) {
    QString text = token.at(3);
    QString chan = token.at(4);
    text += ": "+ getMsg(data);
    print(chan.toUpper(), text);
    return;
  }

  else if (numeric == ERR_NOLOGIN) {
    QString text = token.at(3);
    text += ": "+ getMsg(data);
    print("STATUS", text);
    return;
  }

  else if (numeric == ERR_NEEDMOREPARAMS) {
    QString text = token.at(3);
    text += ": "+ getMsg(data);
    print("STATUS", text);
    return;
  }

  else if (numeric == ERR_KEYSET) {
    QString chan = token.at(3);
    QString text = getMsg(data);
    print(chan.toUpper(), text);
    return;
  }

  else if (numeric == ERR_CHANNELISFULL) {
    QString text, chan = token.at(3);
    text += ": "+ getMsg(data);

    bool e = windowExist(chan.toUpper());
    if (e == true)
      print(chan, text);
    else
      print("STATUS", text);

    return;
  }

  else if (numeric == ERR_UNKNOWNMODE) {
    QString text = token.at(3);
    text += ": "+ getMsg(data);
    print("STATUS", text);
    return;
  }

  else if (numeric == ERR_INVITEONLYCHAN) {
    QString text = token.at(3);
    QString chan = token.at(3);
    text += ": "+ getMsg(data);

    bool e = windowExist(chan.toUpper());
    if (e == true)
      print(chan, text);
    else
      print("STATUS", text);

    return;
  }

  else if (numeric == ERR_INVITEONLYCHAN) {
    QString text = token.at(3);
    QString chan = token.at(3);
    text += ": "+ getMsg(data);

    bool e = windowExist(chan.toUpper());
    if (e == true)
      print(chan, text);
    else
      print("STATUS", text);

    return;
  }

  else if (numeric == ERR_BANNEDFROMCHAN) {
    QString text = token.at(3);
    QString chan = token.at(3);
    text += ": "+ getMsg(data);

    bool e = windowExist(chan.toUpper());
    if (e == true)
      print(chan, text);
    else
      print("STATUS", text);

    return;
  }

  else if (numeric == ERR_BADCHANNELKEY) {
    QString text = token.at(3);
    QString chan = token.at(3);
    text += ": "+ getMsg(data);

    bool e = windowExist(chan.toUpper());
    if (e == true)
      print(chan, text);
    else
      print("STATUS", text);

    return;
  }

  else if (numeric == ERR_CHANOPRIVSNEEDED) {
    /** @todo Use 'text' before its appended further, to print in channel. **/
    QString text = token.at(3);
    QString chan = token.at(3);
    text += ": "+ getMsg(data);
    print(chan, text);
    return;
  }



  /** ***********************
   **        REPLIES       **
   ** *******************  **/

  else if (numeric == RPL_WELCOME) {
    // See what nickname we _actually_ got.
    // Not doing this will cause the client to act retarded.
    activeNick = token.at(2);
    active = true; // Connection is registered.

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
    scriptParent->runevent(te_connect, QStringList()<<hostinfo);

  }

  else if (numeric == RPL_MYINFO) {
    int c = token.at(0).length()+1;
    c += token.at(1).length()+1;
    c += token.at(2).length()+1;
    print("STATUS", data.mid(c));
    return;
  }

  else if (numeric == RPL_ISUPPORT) {

/*
 *[IN] :irc.3phasegaming.net 005 Tomatix_ CMDS=KNOCK,MAP,DCCALLOW,USERIP,STARTTLS UHNAMES NAMESX SAFELIST HCN MAXCHANNELS=25 CHANLIMIT=#:25 MAXLIST=b:60,e:60,I:60 NICKLEN=30 CHANNELLEN=32 TOPICLEN=307 KICKLEN=307 AWAYLEN=307 :are supported by this server
[IN] :irc.3phasegaming.net 005 Tomatix_ MAXTARGETS=20 WALLCHOPS WATCH=128 WATCHOPTS=A SILENCE=15 MODES=12 CHANTYPES=# PREFIX=(ohv)@%+ CHANMODES=beIqa,kfL,lj,psmntirRcOAQKVCuzNSMTGZ NETWORK=3PhaseGaming CASEMAPPING=ascii EXTBAN=~,qjncrRa ELIST=MNUCT :are supported by this server
 *
 *
 **/

    isupport = true;
    int c = token.at(0).length()+1;
    c += token.at(1).length()+1;
    c += token.at(2).length()+1;
    print("STATUS", data.mid(c));

    for (int i = 3; i <= token.count()-1; i++) {
      QStringList lst = token.at(i).split('=');

      if (lst.at(0) == "CHANMODES") {
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
          QStringList ml = lst.at(1).split(',');
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

      if (lst.at(0) == "MAXLIST") {
          // Unreal: MAXLIST=b:60,e:60,I:60
          // IRCu: See MAXBANS
          // IRCnet: MAXLIST=beI:30

          QStringList maxlist = lst.at(1).split(',');

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

      if (lst.at(0) == "MAXBANS") {
          // Quite simple, really.
          maxBanList = lst.at(1).toInt();
          maxExceptList = lst.at(1).toInt();
          maxInviteList = lst.at(1).toInt();
      }

      if (lst.at(0) == "MODES") {
          maxModes = lst.at(1).toInt();
      }

      if (lst.at(0) == "NETWORK") {
        /// @todo set treeview of this Status to Status (Network) from lst.at(1)
          subwindow_t sw = winlist.value("STATUS");
          sw.treeitem->setText(0, "Status (" + lst.at(1) + ")");
      }

      /** @todo  sorting rules for channel nickname list, @, %, +, etc */

      if (lst.at(0) == "PREFIX") {
          //  PREFIX=(ohv)@%+
          // ohv is cumode, @%+ is culetter
          cumode.clear();
          culetter.clear();

          QString parse = lst.at(1);

          enum pState { P_MODE, P_LETTER };
          pState state = P_MODE;
          for (int i = 0; i <= parse.length()-1; i++) {
              char c = parse.at(i).toLatin1();
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

      if (lst.at(0) == "CHANTYPES") {

        QString types = lst.at(1);
        for (int j = 0; j <= types.count()-1; j++)
          chantype << types.at(j).toLatin1();

      }

    }

    return;
  }


  else if (numeric == RPL_AWAY) {
    QString text = token.at(3);
    text += " is away: "+ getMsg(data);
    print("STATUS", text);
    if (windowExist(token.at(3).toUpper()) == true)
      print(token.at(3).toUpper(), text);
    return;
  }

  else if (numeric == RPL_WHOISUSER) {
    QString nick, userhost, name;
    nick = token.at(3);
    userhost = token.at(4);
    userhost += "@" + token.at(5);
    name = getMsg(data);

    if (conf->showWhois)
      print(activewin(), nick+": "+userhost+" : "+name);
    else
      print("STATUS", nick+": "+userhost+" : "+name);

    return;
  }

  else if (numeric == RPL_WHOWASUSER) {
    QString nick, userhost, name;
    nick = token.at(3);
    userhost = token.at(4);
    userhost += "@" + token.at(5);
    name = getMsg(data);
    print("STATUS", nick+": "+userhost+" : "+name);
    return;
  }

  else if (numeric == RPL_WHOISSERVER) {
    QString nick, server, info;
    nick = token.at(3);
    server = token.at(4);
    info = getMsg(data);


    if (conf->showWhois)
      print(activewin(), nick+": "+server+" : "+info);
    else
      print("STATUS", nick+": "+server+" : "+info);

    return;
  }

  else if (numeric == RPL_WHOISOPERATOR) {
    QString text = token.at(3);
    text += " "+ getMsg(data);

    if (conf->showWhois)
      print(activewin(), text);
    else
      print("STATUS", text);

    return;
  }

  else if (numeric == RPL_WHOISIDLE) {
    QString nick, msg;
    QString text[10];
    QStringList textls;

    nick = token.at(3);
    textls = getMsg(data).split(",");

    for (int i = 0; i <= textls.count()-1; i++) {
      if (textls.at(i).at(0) == ' ')
        text[i] = textls.at(i).mid(1);
      else
        text[i] = textls.at(i);
    }

    for (int i = 0; i <= textls.count()-1; i++) {

      if (msg.length() != 0)
        msg += ", ";

      if (text[i].toUpper() == "SIGNON TIME") {
        quint64 seconds = token.at(i+4).toUInt();
        QString date = QDateTime::fromTime_t(seconds).toString("ddd d MMM yyyy, hh:mm");
        msg += "Signed on: " + date;
        continue;
      }

      if (text[i].toUpper() == "SECONDS IDLE") {
        quint64 now = QDateTime::currentMSecsSinceEpoch() / 1000;
        quint64 seconds = token.at(i+4).toUInt();
        QString date = QDateTime::fromTime_t(now-seconds).toString("ddd d MMM yyyy, hh:mm");
        msg += "Last active: " + date;
        continue;
      }

      msg += text[i] + ": " + token.at(i+4);

    }

    if (conf->showWhois)
      print(activewin(), nick+": "+msg);
    else
      print("STATUS", nick+": "+msg);
    return;
  }

  else if (numeric == RPL_ENDOFWHOIS) {
    QString text = token.at(3);
    text += ": "+ getMsg(data);

    if (conf->showWhois)
      print(activewin(), text);
    else
      print("STATUS", text);

    return;
  }

  else if (numeric == RPL_WHOISCHANNELS) {
    QString text = token.at(3);
    text += ": "+ getMsg(data);

    if (conf->showWhois)
      print(activewin(), text);
    else
      print("STATUS", text);

    return;
  }

  else if (numeric == RPL_ENDOFWHOWAS) {
    QString text = token.at(3);
    text += ": "+ getMsg(data);
    print("STATUS", text);
    return;
  }

  else if (numeric == RPL_LISTSTART) {
    IChannelList *cl = *chanlistPtr;
    if (cl == NULL) {
        listInDialog = false;
    }
    else {
        if (cl->isVisible())
            listInDialog = true;
        else
            listInDialog = false;
    }
    print("STATUS", tr("Downloading /LIST..."), PT_LOCALINFO);

    return;
  }
  else if (numeric == RPL_LIST) {
    //emit chanListItem(token.at(3), token.at(4), getMsg(data));
      QString channel = token.at(3);
      QString users = token.at(4);
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
          print("STATUS", text);
      }
      return;
  }
  else if (numeric == RPL_LISTEND) {
    listInDialog = false;
    print("STATUS", tr("End of /LIST"), PT_LOCALINFO);
    return;
  }
  else if (numeric == RPL_CHANNELMODEIS) {
    /** @todo Store these. **/
    // :irc.rigsofrods.org 324 Tomatixx #Test +stnl 1234
    QString chan = token.at(3);

    QString mode;
    for (int i = 4; i <= token.size()-1; i++) { mode += token.at(i) + " "; }

    IWin *w = getWinObj(chan);
    if (w == NULL) {
      print("STATUS", "Modes for " + chan + ": " + mode);
      return;
    }

    if (FillSettings == false) {
        print(chan.toUpper(), "Modes: " + mode, PT_SERVINFO);
    } else {
        if (w->settings != NULL) {
            w->settings->setDefaultMode(mode);
            sockwrite("MODE "+chan+" +b");
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
    QString chan = token.at(3);
    return;
  }

  else if (numeric == RPL_NOTOPIC) {
    QString chan = token.at(3);

    if (FillSettings == false) {
        print(chan.toUpper(), "No topic is set.", PT_SERVINFO);
    } else {
        IWin *w = getWinObj(chan);
        if (w->settings != NULL)
            sockwrite("MODE "+chan);
        else
            FillSettings = false;
    }
    return;
  }

  else if (numeric == RPL_TOPIC) {
    QString chan = token.at(3);
    QString topic = getMsg(data);

    IWin *w = getWinObj(chan);
    if (w == NULL) {
      print("STATUS", "Topic for " + chan + ": " + topic);
      return;
    }

    if (FillSettings == false) {
      print(chan.toUpper(), "Topic is: " + topic, PT_SERVINFO);
    } else {
        if (w->settings != NULL) {
            w->settings->setDefaultTopic(topic);
            sockwrite("MODE "+chan);
        }
        else
            FillSettings = false;
    }
    w->setTopic(topic);
    emit refreshTitlebar();
    return;
  }

  else if (numeric == RPL_TOPICBY) {
    QString chan = token.at(3);
    QString nick = token.at(4);
    int utime = token.at(5).toInt();
    QString date = QDateTime::fromTime_t(utime).toString("ddd d MMM yyyy, hh:mm");

    IWin *w = getWinObj(chan);
    if (w == NULL) {
      print("STATUS", chan + " Topic set by " + nick + ", " + date);
      return;
    }

    if (w->settings == 0)
      print(chan, "Topic set by " + nick + ", " + date, PT_SERVINFO);
    return;
  }

  else if (numeric == RPL_INVITING) {
    QString chan = token.at(3);
    QString nick = token.at(4);
    print(chan.toUpper(), "Invited " + nick, PT_SERVINFO);
    return;
  }

  else if (numeric == RPL_SUMMONING) {
    QString text = token.at(3);
    text += ": "+ getMsg(data);
    print("STATUS", text);
    return;
  }

  else if (numeric == RPL_VERSION) {
    QString text = token.at(3);
    text += " " + token.at(4);
    text += " : "+ getMsg(data);
    print("STATUS", text);
    return;
  }

  else if (numeric == RPL_WHOREPLY) {
    int l = token.at(0).length()+1;
    l += token.at(1).length()+1;
    l += token.at(2).length()+1;
    QString text = data.mid(l);
    print("STATUS", text);
    return;
  }

  else if (numeric == RPL_ENDOFWHO) {
    QString text = token.at(3);
    text += ": "+ getMsg(data);
    print("STATUS", text);
    return;
  }

  else if (numeric == RPL_NAMREPLY) {
    QString chan = token.at(4);
    QString msg = getMsg(data);
    QStringList nicks = msg.split(' ');

    if (nicks.count() == 0) // If we, by some reason, get nothing to parse, ignore (or wait for a LONG, like, LOOONG loop to finish.)
      return;

    for (int i = 0; i <= nicks.count()-1; i++) {
      QString item = nicks.at(i);
      char mode = 0x00;

      if (nicks.at(i).count() == 0)
        continue; // DAFUQ

      char l = item.at(0).toLatin1();
      if (isValidCuLetter(l)) { // This user got a mode
        mode = l; // Set mode
        item = item.mid(1); // Set nickname without mode
      }

      member_t m;
      m.nickname = item;
      if (mode != 0x00)
        m.mode << mode;
      // Ident and host is unknown, for now. It's safe not to have it.

      winlist.value(chan.toUpper()).widget->insertMember(item, m, false);
    }
  }


  else if (numeric == RPL_ENDOFNAMES) {
    QString chan = token.at(3);
    winlist.value(chan.toUpper()).widget->sortMemberList();
  }

  else if (numeric == RPL_LINKS) {
    int l = token.at(0).length()+1;
    l += token.at(1).length()+1;
    l += token.at(2).length()+1;
    QString text = data.mid(l);
    print("STATUS", text);
    print("STATUS", text);
    return;
  }

  else if (numeric == RPL_ENDOFLINKS) {
    QString text = token.at(3);
    text += ": "+ getMsg(data);
    print("STATUS", text);
    return;
  }

  else if ((numeric == RPL_MOTDSTART) && (conf->showMotd)) {
      print("STATUS", "Loading MOTD...", PT_LOCALINFO);
      motd.reset();
      return;
  }

  else if ((numeric == RPL_MOTD) && (conf->showMotd)) {
      QString text = getMsg(data);
      motd.print(text);
      return;
  }

  else if (numeric == RPL_ENDOFMOTD) {
    print("STATUS", getMsg(data));

    if (conf->showMotd)
      motd.show();

    if (registered == true)
      return; // Stop here

    serverName = data.split(" ").at(0).mid(1);

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
      sockwrite("MODE " + activeNick + " +i");

    emit RequestTrayMsg("Connected to server", "Successfully connected to " + conf->server);
    emit connectedToIRC();

    return;
  }

  else if ((numeric == RPL_BANLIST) || (numeric == RPL_EXCEPTLIST) || (numeric == RPL_INVITELIST)) {
    QString chan = token.at(3);
    int created = token.at(6).toInt();
    QString date = QDateTime::fromTime_t(created).toString("ddd d MMM yyyy, hh:mm");

    if (windowExist(chan) == false) {
      print("STATUS", token.at(4) + " set by " + token.at(5) + ", " + date);
    }

    IWin *w = getWinObj(chan);

    if ((FillSettings == true) && (w->settings != NULL))
        w->settings->addMask(token[4], token[5], date);

    if (FillSettings == false)
        print(chan, token.at(4) + " set by " + token.at(5) + ", " + date);

    return;
  }

  else if (numeric == RPL_ENDOFBANLIST) {
    QString chan = token.at(3);
    if (windowExist(chan.toUpper()) == false) {
      print("STATUS", chan + ": " + getMsg(data));
      return;
    }

    IWin *w = getWinObj(chan);

    if (FillSettings == true) {
        if (w->settings != NULL) {
            w->settings->finishModel(MT_BAN);

            if (haveExceptionList)
                sockwrite("MODE "+chan+" +e");
            else if (haveInviteList)
                sockwrite("MODE "+chan+" +I");
            else
                FillSettings = false;
        }
        else
            FillSettings = false;
    }
    return;
  }

  else if (numeric == RPL_ENDOFEXCEPTLIST) {
    QString chan = token.at(3);
    if (windowExist(chan.toUpper()) == false) {
      print("STATUS", chan + ": " + getMsg(data));
      return;
    }

    IWin *w = getWinObj(chan);

    if (FillSettings == true) {
        if (w->settings != NULL) {
            w->settings->finishModel(MT_EXCEPT);

             if (haveInviteList)
                sockwrite("MODE "+chan+" +I");
             else
                 FillSettings = false;
        }
        else
            FillSettings = false;
    }
    return;
  }

  else if (numeric == RPL_ENDOFINVITELIST) {
    QString chan = token.at(3);
    if (windowExist(chan.toUpper()) == false) {
      print("STATUS", chan + ": " + getMsg(data));
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
    QString text = token.at(3);
    text += ": "+ getMsg(data);
    print("STATUS", text);
    return;
  }

  else if (numeric == RPL_TIME) {
    QString text = token.at(3);
    text += ": "+ getMsg(data);
    print("STATUS", text);
    return;
  }

  else if ((numeric >= 200) && (numeric <= 244)) {
    /// @note See RFC1459 for which numerics this is about.
    int l = token.at(0).length()+1;
    l += token.at(1).length()+1;
    l += token.at(2).length()+1;
    QString text = data.mid(l);
    print("STATUS", text);
    return;
  }

  else if ((numeric >= 252) && (numeric <= 254)) {
    /// @note RPL_LUSEROP, RPL_LUSERUNKNOWN, RPL_LUSERCHANNELS
    QString text = token.at(3);
    text += " "+ getMsg(data);
    print("STATUS", text);
    return;
  }

  else if (numeric == RPL_ADMINME) {
    QString text = token.at(3);
    text += ": "+ getMsg(data);
    print("STATUS", text);
    return;
  }

    /** @todo userhost, for banning members.
  else if (numeric == RPL_USERHOST) {
    if (userhostAction.length() > 0) {
      // Do action and return, nothing to view in STATUS.

      // Get the actual hostname.
      QString host = token.at(3).split("@").at(1); // <-- yes messy. I don't care.

      // Save the action to a non-constant
      char *act = new char[512];
      strcpy(act, userhostAction.toStdString().c_str());

      char *execthis = new char[512];

      // convert the %s into whatever we got told to
      sprintf(execthis, act, host.toStdString().c_str());

      // sockwrite it.
      sockwrite(QString(execthis));
      userhostAction.clear();
    }
  }
  */
  else if (numeric == RPL_WHOISACTUALHOST) {
    QString nick, text;
    nick = token.at(3);
    text = getMsg(data);


    std::cout << "actualhost" << std::endl;

    if (conf->showWhois)
        print(activewin(), nick+": "+text);
    else
      print("STATUS", nick+": "+text);

    return;
  }

  else if (numeric == RPL_WHOISMODES) {
    QString nick, text;
    nick = token.at(3);
    text = getMsg(data);


    std::cout << "modes" << std::endl;

    if (conf->showWhois)
      print(activewin(), nick+": "+text);
    else
      print("STATUS", nick+": "+text);

    return;
  }

  else if (numeric == RPL_WHOISIDENTIFIED) {
    QString nick, text;
    nick = token.at(3);
    text = getMsg(data);


    std::cout << "identified" << std::endl;

    if (conf->showWhois)
      print(activewin(), nick+": "+text);
    else
      print("STATUS", nick+": "+text);

    return;
  }

  else if (numeric == RPL_WHOISLOGGEDIN) {
    QString nick, user, text;
    nick = token.at(3);
    user = token.at(4);
    text = getMsg(data);

    if (conf->showWhois)
      print(activewin(), nick+": "+text+" "+user);
    else
      print("STATUS", nick+": "+text+" "+user);

    return;
  }

  // :server num target interesting data goes here
  int skip = token.at(0).length() + 1;
  skip += token.at(1).length() + 1;
  skip += token.at(2).length() + 1;

  QString text = data.mid(skip);

  if (text.at(0) == ':')
    text = text.mid(1);

  print("STATUS", text);
}
