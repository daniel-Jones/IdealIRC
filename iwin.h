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

/*! \class IWin
 *  \brief Contains all GUI widgets inside all subwindows.
 *
 * Text display widget, listbox and text input widget are set up here.\n
 * Data for channels we're on is also stored here.
 */

#ifndef IWIN_H
#define IWIN_H

#include <QWidget>
#include <QMdiSubWindow>
#include <QSplitter>
#include <QCloseEvent>
#include <QShowEvent>
#include <QHideEvent>
#include <QResizeEvent>
#include <QTreeWidgetItem>
#include <QUrl>
#include <QFocusEvent>
#include <QTextCodec>
#include <QMenu>

#include "constants.h"
#include "tpicturewindow.h"
#include "iircview.h"
#include "qmylistwidget.h"
#include "qmylineedit.h"
#include "config.h"
#include "ichanconfig.h"
#include "icommand.h"
#include "dcc/dcc.h"

namespace Ui {
    class IWin;
}

class TScriptParent;
class IConnection;
class IWin;

typedef struct T_MEMBER {
  QString nickname;
  QString ident;
  QString host;
  QList<char> mode; // Store mode letters like +, @, etc (not the modes v, o, etc)
} member_t;

class IWin : public QWidget
{
    Q_OBJECT

public:
      explicit IWin(QWidget *parent, QString wname, int WinType, config *cfg, TScriptParent *sp, IConnection *p = NULL);
      ~IWin();
      int getId() { return winid; } //!< \return Integer of this subwindows ID.
      int getType() { return WindowType; } //!< \return Integer of what type this window is. (WT_* constants)
      QString getTarget() { return target; } //!< \return QString of what target this widget writes to (channel or nickname)
      void print(const QString &sender, const QString &text, const int ptype = 0);
      void insertMember(QString nickname, member_t mt, bool sort = true);
      void removeMember(QString nickname, bool sort = true);
      bool memberExist(QString nickname);
      void memberSetNick(QString nickname, QString newnick);
      void sortMemberList(QString memberRemoved = "");
      void resetMemberlist();
      void MemberSetMode(QString nickname, char mode);
      void MemberUnsetMode(QString nickname, char mode);
      member_t ReadMember(QString nickname);
      IChanConfig *settings; //!< Channel settings dialog. Must be created before this one is shown. See execChanSettings()
      void setInputText(QString text);
      void setTopic(QString newTopic); // Just for storing, nothing else.
      void setCmdHandler(ICommand *cmd) { cmdhndl = cmd; } //!< Passes the ICommand class from the IConnection this window is bound to.
      void setConnectionPtr(IConnection *con);
      void sockwrite(QString data) { emit sendToSocket(data); } //!< Writes data to the IRC server.
      IConnection* getConnection() { return connection; } //!< Returns the IConnection this window is bound to.
      void setSortRuleMap(QList<char> *sl) { sortrule = sl; } //!< Copies the sort rule which the bound IConnection uses.
      void setFont(const QFont &font);
      void reloadCSS(); // Runs only if TIRCVIEW is present.
      void clear(); // Clear contents
      QStringList getSelectedMembers(); // Returns a list of selected nicknames.
      void updateTitleHost(); // for WT_PRIVMSG, set hostname in titlebar
      TPictureWindow* picwinPtr() { return picwin; } //!< Returns the drawable picture widget, only if WindowType = WT_GRAPHIC or WT_GWINPUT - otherwise this returns NULL.
      void execChanSettings();
      int listboxWidth();
      int listboxHeight();

private:
      Ui::IWin *ui; //!< Qt Creator generated GUI class.
      QString target; //!< The target which to send data to, nickname or channel.
      int WindowType; //!< The window type. See constants.h
      static int winidCount; //!< ID counter for window ids. Never decreases.
      static int statusCount; //!< Counter for status windows. Decreases, keeps count of status windows.
      int winid; //!< The window id.
      IConnection *connection; //!< The connection this window is bound to.
      DCC *dcc; //!< DCC processing, if this is a DCC window.
      TScriptParent *scriptParent; //!< Pointer to the script parent.

      const QString tstar; //!< Just a string with triple stars.
      const QString sstar; //!< Just a string with a single star.

      QString iname;
      QSplitter *split; //!< Splitter for text widget and listbox.
      IIRCView *textdata; //!< Text widget. (Most window types except custom graphic.)
      config *conf; //!< Pointer to config class (iirc.ini)
      int WinType; //!< Window type. See constants.h for WT_*
      TPictureWindow *picwin; //!< Paintable picture window (WindowType = WT_GRAPHIC or WT_GWINPUT)
      QMyListWidget *listbox; //!< Listbox widget. (Channels only)
      QMenu *listboxMenu; //!< Menu for the listbox widget. Gets its contents via scriptParent. (Channels only)
      QMenu *opMenu;
      QMenu *textboxMenu; //!< Menu for the display widget. Gets its contents via scriptParent. Its contents depends on the window type.

      // 'Join channel' menu, requested by IIRCView.
      QMenu joinChannelMenu; //!< Menu requested by IIRCView, when you click on channels, this menu popup.
      QAction *joinChannelTitle; //!< Title item for Join channel menu. (Disabled with bold text)
      QAction *joinChannelAction; //!< "Join channel" item for the Join channel menu.

      QMyLineEdit *input; //!< Text-input widget. (Most window types, except WT_TXTONLY)
      QString topic; //!< For window type WT_CHANNEL - the channel topic.
      ICommand *cmdhndl; //!< The command handler from the bound IConnection.
      QList<char> *sortrule; //!< All letters in order which should be sorted by. From IConnection.
      QHash<QString,member_t> members; //!< Members of a channel.
      QStringList memberlist; //!< This one is also complete with nicknames+modes at all time. Add nickname here, run sort and it'll add result to listbox.

      int acIndex; //!< Index at autocomplete (practically, index of acMatch)
      int acCursor; //!< Cursor position, but before the autocompleted word. Remember to add length of a.c. word!
      QStringList acList; //!< Items we can get from autocomplete
      QStringList acMatch; //!< List of strings that matches the pattern.
      QString acPatt; //!< Pattern to autocomplete
      QString acPre; //!< All text before the autocompleted word.
      QString acPost; //!< All text after the autocompleted word.

      void writeToLog(QString text); // Writes to a log file under logdir and filename is target.txt
      void sortList(QList<char> *lst);
      bool sortLargerThan(const QString s1, const QString s2);
      QString stripModeChar(QString nickname);
      void regenChannelMenus();
      void regenQueryMenu();
      void regenStatusMenu();
      void processLineInput(QString &line);

protected:
      void closeEvent(QCloseEvent *e);
      void showEvent(QShowEvent *);
      void hideEvent(QHideEvent *);
      void resizeEvent(QResizeEvent *);
      void focusInEvent(QFocusEvent *);

private slots:
      void picwinMouseEvent(e_iircevent event, int x, int y, int delta = 0);
      void inputEnterPushed();
      void tabKeyPushed();
      void joinChannel(QString channel);
      void pmUser(QString nickname);
      void openURL(const QUrl url);
      void GiveFocus();
      void splitterMoved(int, int);
      void textboxMenuRequested(QPoint p);
      void listboxMenuRequested(QPoint p);
      void settingsClosed();
      void listboxDoubleClick(QListWidgetItem *item);
      void mouseDoubleClick();
      void joinChannelMenuRequest(QPoint point, QString channel);
      void joinChannelTriggered();
      void nickMenuRequest(QPoint point, QString nickname);

signals:
      void closed(int wid);
      void activeWin(QString aw);
      void doCommand(QString command);
      void sendToSocket(QString data);
      void RequestWindow(QString name, int type, int parent, bool activate);
      void Highlight(int wid, int type);
};

#endif // IWIN_H
