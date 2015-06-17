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

/*! \class IConnection
 *  \brief IRC connection class.
 *
 * All IRC related stuff happens here.\n
 * This class is directly bound to a status window, and share the same ID.\n
 * If for example, this connection have ID=4, its status window will have ID=4 too.\n\n
 *
 * Each IConnection will also have their own list of windows, even though the "main" class IdealIRC got one aswell.
 * The purpose of having own lists is to avoid weird results if we are on two servers and on channels with same names, private messages with
 * users of same nickname, etc.\n
 * There are other solutions around this and could be changed, to keep only one window list at all time.
 */

#ifndef ICONNECTION_H
#define ICONNECTION_H

#include <QObject>
#include <QTcpSocket>
#include <QList>
#include <QTextCodec>

#include "iwin.h"
#include "config.h"
#include "ichannellist.h"
#include "imotdview.h"
#include "ial.h"
#include "iwindowswitcher.h"

// For accessing the IAL with a GUI
//#include "iaddresslist.h"

class ICommand;
class TScriptParent;

/*!
 * Used for easier storing of user data formatted as nickname!ident@hostname\n
 * parseUserinfo() as parses user data and returns this.
 */
typedef struct T_USER {
    QString nick; //!< Nickname
    QString user; //!< Ident / username
    QString host; //!< Hostname
} user_t;


class IConnection : public QObject
{
  Q_OBJECT

public:
      explicit IConnection(QObject *parent, IChannelList **clptr, int connId, config *cfg, TScriptParent *sp, IWindowSwitcher *ws);
      bool isOnline() { return active; } //!< \return true when we're registered to server.
      bool isSocketOpen() { return socket.isOpen(); } //!< \return true when socket is connected.
      QString getActiveNickname() { return activeNick; } //!< \return QString of our actual nickname on this connection. May differ from config.
      void setServer(QString server = "", QString passwd = "");
      void tryConnect();
      void addWindow(QString name, subwindow_t win);
      void freeWindow(QString name);
      ICommand* getCmdHndlPtr() { return &cmdhndl; } //!< \return ICommand that belongs to here.
      QList<char>* getSortRuleMapPtr() { return &sortrule; } //!< \return Pointer to the QList<char> of sort rule this connection generates.
      int getCid() { return cid; } //!< \return The ID of this IConnection
      void closeConnection(bool shutdown = false); // shutdown is set to true if IIRC is shutting down.
      void setActiveInfo(QString *wn, int *ac);
      char getCuLetter(char mode);
      bool isValidCuMode(char mode);
      bool isValidCuLetter(char l);
      bool isValidChannel(QString channel);
      subwindow_t getSubWindowStruct(QString wname) { return winlist.value(wname); } //!< \return subwindow_t structure of given window name.
      QString getConnectionInfo() { return host + ":" + QString::number(port); } //!< \return QString of the connections server:port
      QString trimCtrlCodes(QString &text);
      QStringList getAcList() { return acList; } //!< \return QStringList of previously typed texts, for auto-complete.
      bool windowExist(QString name);
      void print(const QString &window, const QString &sender, const QString &line, const int ptype = PT_NORMAL);
      unsigned int ipv4toint(QString addr);
      QString intipv4toStr(unsigned int addr);
      int maxBanList; //!< Maximum channel bans (+b) we can send to IRC server, defined in isupport (numeric 005). Default is 3.
      int maxExceptList; //!< Maximum channel ban exceptions (+e) we can send to IRC server, defined in isupport (numeric 005). Default is 3.
      int maxInviteList; //!< Maximum channel invites (+I) we can send to IRC server, defined in isupport (numeric 005). Default is 3.
      int maxModes;
      bool haveExceptionList; //!< True if IRC server supports ban exceptions. Default false.
      bool haveInviteList; //!< True if IRC server supports invite lists. Default false.
      bool FillSettings; //!< Sets to true when we're about to show the channel settings dialog, to fill its data. When we're done filling the data, it sets back to false.\n This is for to not print text (topic, ban lists, etc) in the window.

      QString getcmA() { return cmA; } //!< See cmA
      QString getcmB() { return cmB; } //!< See cmB
      QString getcmC() { return cmC; } //!< See cmC
      QString getcmD() { return cmD; } //!< See cmD

      IAL ial; //!< Our IAL for this connection.

      QString dccinfo; //!< Passes instructions to dcc derived classes.

private:
      // for accessing the IAL with a GUI.
      //IAddressList addresslist;

      ICommand cmdhndl; //!< Our command handler for this connection.
      config *conf; //!< Pointer to the config class (iirc.ini)
      int cid; //!< Connection ID. Will never change. Equal to the ID of status window this belongs to.
      bool active; //!< Sets to true when RPL_WELCOME is received.
      bool registered; //!< Sets to true when RPL_ENDOFMOTD is received.
      bool isupport; //!< true if the IRC server sent us an isupport (005)
      QString host; //!< IRC server hostname.
      QString password; //!< IRC server password.
      int port; //!< IRC server port.
      QString serverName; //!< The server's reported hostname.
      QString activeNick; //!< Nickname we use for this connection.
      QTcpSocket socket; //!< The actual TCP socket to the IRC server.
      QHash<QString,subwindow_t> winlist; //!< Subwindows assosciated with this connection.
      QString *activeWname; //!< Pointer to active window name.
      int *activeConn; //!< Pointer to the active IRC connection ID. Might differ to this one!
      bool ShuttingDown; //!< True if we're about to shut down IdealIRC.
      bool tryingConnect; //!< True when we're attempting to connect to an IRC server.
      IChannelList **chanlistPtr; //!< Used when we're filling the channel list.
      bool listInDialog; //!< true if we're listing in a dialog. chanlistPtr must be set on beforehand!
      bool connectionClosing; //!< About to disconnect from the IRC server.
      IMotdView motd; //!< MOTD dialog.
      QStringList acList; //<! Contains channel names we'd like to autocomplete. The nicknames is composed from IWin.
      TScriptParent *scriptParent; //!< Pointer to the script parent.
      bool receivingNames; //!< True when we're receiving NAMES command, for filling a nickname listbox.

      /* For retreiving data, onSocketReadyRead() */
      QByteArray linedata; //!< Socket reading buffer. When we receive CR+LF, this buffer will be parsed and reset for next line.

      QList<char> chantype; //<! Channel types this server allows, such as #, &
      QList<char> cumode; //<! Channel User-modes this server allows, such as +o +h +v, etc.
      QList<char> culetter; //<! Same as cumode, Channel User-mode letters, such as @ % +, etc.
      QList<char> sortrule; //<! All letters (culetter) in order which should be sorted by.

      QString cmA; //!< Modes from isupport CHANMODES, A types.\n Mode that adds or removes a nick or address to a list. Always has a parameter.\n Default: b\n See http://www.irc.org/tech_docs/005.html
      QString cmB; //!< Modes from isupport CHANMODES, B types.\n Mode that changes a setting and always has a parameter.\n Default: k\n See http://www.irc.org/tech_docs/005.html
      QString cmC; //!< Modes from isupport CHANMODES, C types.\n Mode that changes a setting and only has a parameter when set.\n Default: l\n See http://www.irc.org/tech_docs/005.html
      QString cmD; //!< Modes from isupport CHANMODES, D types.\n Mode that changes a setting and never has a parameter.\n default imnpstr\n See http://www.irc.org/tech_docs/005.html

      const QString tstar; //!< \return A QString with triple stars (***).
      const QString sstar; //!< \return A QString with a single star (*).

      IChanConfig* getChanConfigPtr(QString channel);

      QTimer checkConnection; //!< Timer that run every 3 minutes, to check connection life. Whenever we receive data from the socket, the timer is restarted. If the timer time-outs, it sends "PING :ALIVE" to the server to test.
      int checkState; //!< Used with IConnection::checkConnection, different states of it.\n Valid values:\n 0: on timeout, send "PING :ALIVE" to server. Sets checkState to 1 and starts timer on 30 seconds.\n 1: on timeout, close socket (took too long to receive pong), server connection is dead.

      QTimer conTimeout; //!< Timer that runs for config::timeout ms, and aborts connection attempt on timeout.

      QString getMsg(QString &data);
      IWin* getWinObj(QString name); // Returns NULL if no matches.

      user_t parseUserinfo(QString uinfo);
      void parse(QString &data);
      void parseNumeric(int numeric, QString &data);
      void resetSortRules();
      QString activewin();

      IWindowSwitcher *wsw; //!< Pointer to the window switcher, which holds the button bar.

public slots:
      bool sockwrite(QString data);

private slots:
      void onSocketConnected();
      void onSocketDisconnected();
      void onSocketReadyRead();
      void checkConnectionTimeout();
      void connectionAttemptTimeout();

signals:
      void RequestTrayMsg(QString title, QString message);
      void RequestWindow(QString wname, int wtype, int parent, bool activate = false);
      void refreshTitlebar();
      void requestChanListDlg();
      void chanListItem(QString channel, QString users, QString topic);
      void RequestFavourites();
      void connectedToIRC();
      void HighlightWindow(int wid, int type); // Name must be a window in this connection's windows!
      void connectionClosed(); // This one is only used when IIRC is about to exit.
      void updateConnectionButton();
};

#endif // ICONNECTION_H
