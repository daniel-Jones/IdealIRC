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

/*! \class IdealIRC
 *  \brief IdealIRC Main GUI
 *
 * This can be considered the "central" of all things, more or less.\n
 * Here we have the main instance of script parent, list of _all_ subwindows,
 * all IConnection instances, etc.
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
#include <QSystemTrayIcon>
#include <QKeyEvent>
#include <QToolButton>

#include "constants.h"
#include "config.h"
#include "iwin.h"
#include "iconnection.h"
#include "iconfig.h"
#include "versionchecker.h"
#include "ifavourites.h"
#include "ichannellist.h"
#include "iwindowswitcher.h"

#include "script/tscriptparent.h"
#include "script/iscriptmanager.h"

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
      Ui::IdealIRC *ui; //!< Qt Creator generated GUI class.
      bool firstShow; //!< Initializer list sets this to true. Once showEvent() has run, it will set to false.\n See showEvent()
      bool windowIsActive; //!< IdealIRC is the active window (got the focus).
      bool readyToClose; //!< By default, this variable "floats", got no apparent value until we are about to close IdealIRC.\n When we're about to close, this one sets to false until all IRC connections are closed, then it's set to true and IdealIRC will close down.
      QHash<int,subwindow_t> winlist; //!< Windows list. All subwindows is stored here.\n Key: Window ID\n Value: Subwindow.
      QHash<int,IConnection*> conlist; //!< Connections list.\n Key: Connection ID. This ID is bound to its Status subwindow ID.\n Value: Connection.
      int activeWid; //!< Current active window ID.
      QString activeWname; //!< Current active window name.\n Use activeWid for lookup methods.
      int activeConn; //!< Current active connection ID.
      config conf; //!< The main config class instance. Pass a pointer to this one, wherever it's needed.
      IConfig *confDlg; //!< Configuration dialog pointer.
      IFavourites *favourites; //!< Favourites dialog pointer.
      IChannelList *chanlist; //!< Channel list dialog pointer.
      IScriptManager *scriptManager; //!< Script manager diaog pointer.
      int connectionsRemaining; //!< When closing IdealIRC, we must wait for all connections to close before exiting IdealIRC. This one counts backwards for each disconneciton.
      bool preventSocketAction; //!< Used when updating connection toolbutton, when using setChecked it also performs its signal.
      IConnection *reconnect; //!< When re-using a current active connection, to connect somewhere else, set this to the pointer of that connection.
      VersionChecker vc; //!< As the class name suggest, a version checker. Disabled by default.
      TScriptParent scriptParent; //!< Main instance of the script parent, where all scripts are loaded and events are pushed into.
      QSystemTrayIcon trayicon; //!< System tray icon.
      IWindowSwitcher wsw; //!< Window switcher.
      QHash<QString,toolbar_t> *customToolbar; //!< Toolbar buttons from scripts.
      QList<QAction*> customToolButtons; //!< List of QAction, parsed from customToolbar.
      QSignalMapper *toolBtnMap; //!< Signal mapper, for each custom toolbar button mapped to their triggered signal.

      void recreateConfDlg();
      void recreateFavouritesDlg();
      void favouritesJoinEnabler();
      void recreateChanlistDlg();
      void recreateScriptManager();
      void chanlistEnabler();
      void updateTreeViewColor();

protected:
      void showEvent(QShowEvent *);
      void closeEvent(QCloseEvent *e);
      void resizeEvent(QResizeEvent *e);
      void moveEvent(QMoveEvent *);
      void keyReleaseEvent(QKeyEvent *e);

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
      void configSaved();
      void on_actionChannel_favourites_triggered();
      void connectionEstablished();
      void on_actionChannels_list_triggered();
      void on_actionScript_Manager_triggered();
      void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
      void trayMessage(QString title, QString message, QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information);
      void applicationFocusChanged(QWidget* old, QWidget* now);
      void switchWindows(int wid);
      void on_actionToolbar_triggered();
      void on_actionWindow_buttons_triggered();
      void on_actionWindow_tree_triggered();
      void on_actionMenubar_triggered();
      void customToolBtnClick(QString objName);
      void rebuildCustomToolbar(); // Rebuild the toolbar icons.
};

#endif // IDEALIRC_H
