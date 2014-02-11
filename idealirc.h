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

#ifndef IDEALIRC_H
#define IDEALIRC_H

#include <QMainWindow>
#include <QShowEvent>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QMoveEvent>
#include <QMdiSubWindow>
#include <QHash>

#include "constants.h"
#include "config.h"
#include "iwin.h"
#include "iconnection.h"
#include "iconfig.h"
#include "versionchecker.h"

namespace Ui {
    class IdealIRC;
}

class IdealIRC : public QMainWindow
{
  Q_OBJECT
    
  public:
      explicit IdealIRC(QWidget *parent = 0);
      ~IdealIRC();
      QTreeWidgetItem* GetWidgetItem(int wid);
      //IWin* GetWindowObject();
      bool WindowExists(QString name, int parent);
      int currentStatus();
    
  private:
      Ui::IdealIRC *ui;
      bool firstShow; // True on startup. See showEvent();
      QHash<int,subwindow_t> winlist; // Windows list
      QHash<int,IConnection*> conlist; // Connections list. int is "parent" from CreateSubWindow.
      int activeWid;
      QString activeWname;
      int activeConn; // -1 if it's a custom window active!
      config conf;
      IConfig *confDlg;
      int connectionsRemaining; // When closing IIRC we must wait for all connections to close before exiting IIRC. This one counts backwards for each disconneciton.
      bool preventSocketAction; // Used when updating connection toolbutton, when using setChecked it also performs its signal.
      IConnection *reconnect; // When re-using a current active connection, to connect somewhere else, set this to the pointer of that connection.
      VersionChecker vc;

      void recreateConfDlg();

  protected:
      void showEvent(QShowEvent *);
      void closeEvent(QCloseEvent *e);
      void resizeEvent(QResizeEvent *e);
      void moveEvent(QMoveEvent *);

  public slots:
      int CreateSubWindow(QString name, int type, int parent, bool activate);
      void updateConnectionButton();
      void versionReceived(); // Released version from website received

  private slots:
      void subWinClosed(int wid);
      void on_mdiArea_subWindowActivated(QMdiSubWindow *arg1);
      void on_treeWidget_itemSelectionChanged();
      void on_actionOptions_triggered();
      void extConnectServer(bool newWindow);
      void on_actionConnect_toggled(bool arg1);
      void connectionClosed(); // This one is only used when IIRC is about to exit.
      void on_treeWidget_clicked(const QModelIndex &);
      void Highlight(int wid, int type);
      void on_actionAbout_IdealIRC_triggered();
};

#endif // IDEALIRC_H
