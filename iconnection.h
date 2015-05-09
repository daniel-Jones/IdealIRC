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

// for parseUserinfo();
typedef struct T_USER {
    QString nick;
    QString user;
    QString host;
} user_t;


class IConnection : public QObject
{
  Q_OBJECT

public:
      explicit IConnection(QObject *parent, IChannelList **clptr, int connId, config *cfg, TScriptParent *sp, IWindowSwitcher *ws);
      bool isOnline() { return active; } // True when we're registered to server.
      bool isSocketOpen() { return socket.isOpen(); } // True when socket is connected.
      QString getActiveNickname() { return activeNick; }
      void setServer(QString server = "", QString passwd = "");
      void tryConnect();
      void addWindow(QString name, subwindow_t win);
      void freeWindow(QString name);
      ICommand* getCmdHndlPtr() { return &cmdhndl; }
      QList<char>* getSortRuleMapPtr() { return &sortrule; }
      int getCid() { return cid; }
      void closeConnection(bool shutdown = false); // shutdown is set to true if IIRC is shutting down.
      void setActiveInfo(QString *wn, int *ac);
      char getCuLetter(char l);
      bool isValidCuMode(char mode);
      bool isValidCuLetter(char l);
      bool isValidChannel(QString channel);
      subwindow_t getSubWindowStruct(QString wname) { return winlist.value(wname); }
      QString getConnectionInfo() { return host + ":" + QString::number(port); } // Returns the connections server:port
      QString trimCtrlCodes(QString &text);
      QStringList getAcList() { return acList; }
      bool windowExist(QString name);
      void print(const QString &window, const QString &sender, const QString &line, const int ptype = PT_NORMAL);
      unsigned int ipv4toint(QString addr);
      QString intipv4toStr(unsigned int addr);
      int maxBanList; // From isupport. Default is 3
      int maxExceptList; // Same as above ^
      int maxInviteList; // Same as above ^
      int maxModes;
      bool haveExceptionList; // Default false
      bool haveInviteList; // Default false
      bool FillSettings;

      QString getcmA() { return cmA; }
      QString getcmB() { return cmB; }
      QString getcmC() { return cmC; }
      QString getcmD() { return cmD; }

      IAL ial;

      QString dccinfo; // passes instructions to dcc derived classes

private:
      // for accessing the IAL with a GUI.
      //IAddressList addresslist;

      ICommand cmdhndl;
      config *conf;
      int cid; // Connection ID. Will never change. Equal to the ID of status window this belongs to.
      bool active;
      bool registered;
      bool isupport;
      QString host;
      QString password;
      int port;
      QString serverName;
      QString activeNick; // Nickname we use for this connection.
      QTcpSocket socket;
      QHash<QString,subwindow_t> winlist; // Windows assosciated with this connection.
      QString *activeWname;
      int *activeConn;
      bool ShuttingDown;
      bool tryingConnect;
      IChannelList **chanlistPtr;
      bool listInDialog;
      bool connectionClosing;
      IMotdView motd;
      QStringList acList; // Contains channel names we'd like to autocomplete.
      TScriptParent *scriptParent;
      bool receivingNames;

      /* For retreiving data, onSocketReadyRead() */
      QByteArray linedata;

      QList<char> chantype; // #&, etc
      QList<char> cumode; // ohv, etc
      QList<char> culetter; // @%+, etc
      QList<char> sortrule; // All letters in order which should be sorted by.

      // Modes from isupport CHANMODES.
      QString cmA; // Default b
      QString cmB; // Default k
      QString cmC; // Default l
      QString cmD; // default imnpstr

      const QString tstar; // triple stars
      const QString sstar; // single star

      IChanConfig* getChanConfigPtr(QString channel);

      QTimer checkConnection; // Run every 3 minutes. Restart timer on every data we receive.
      int checkState; // 0: on timeout, send ping to server. 1: on timeout, close socket (took too long to receive pong).

      QTimer conTimeout; // Waits for config::timeout ms. and aborts connection attempt.

      QString getMsg(QString &data);
      IWin* getWinObj(QString name); // Returns NULL if no matches.

      user_t parseUserinfo(QString uinfo);
      void parse(QString &data);
      void parseNumeric(int numeric, QString &data);
      void resetSortRules();
      QString activewin();

      IWindowSwitcher *wsw;

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
