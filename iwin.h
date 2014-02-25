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

#include "constants.h"
#include "tpicturewindow.h"
#include "tircview.h"
#include "qmylistwidget.h"
#include "qmylineedit.h"
#include "config.h"
#include "ichanconfig.h"
#include "icommand.h"

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
      explicit IWin(QWidget *parent, QString wname, int WinType, config *cfg, TScriptParent *sp);
      ~IWin();
      int getId() { return winid; }
      int getType() { return WindowType; }
      QString getTarget() { return target; }
      void print(const QString &text, const int ptype = 0);
      void insertMember(QString nickname, member_t mt, bool sort = true);
      void removeMember(QString nickname, bool sort = true);
      bool memberExist(QString nickname);
      void memberSetNick(QString nickname, QString newnick);
      void sortMemberList(QString memberRemoved = "");
      void resetMemberlist();
      void MemberSetMode(QString nickname, char mode);
      void MemberUnsetMode(QString nickname, char mode);
      member_t ReadMember(QString nickname);
      IChanConfig *settings;
      void setInputText(QString text);
      void setTopic(QString newTopic) { topic = newTopic; } // Just for storing, nothing else.
      void setCmdHandler(ICommand *cmd) { cmdhndl = cmd; }
      void setConnectionPtr(IConnection *con) { connection = con; }
      void sockwrite(QString data) { emit sendToSocket(data); }
      IConnection* getConnection() { return connection; }
      void setSortRuleMap(QList<char> *sl) { sortrule = sl; }
      void setFont(const QFont &font);
      void reloadCSS(); // Runs only if TIRCVIEW is present.
      void clear(); // Clear contents
      void doGfx(e_painting command, QStringList param);

  private:
      Ui::IWin *ui;
      QString target;
      int WindowType; // See constants.h
      static int winidCount;
      static int statusCount; // count of status windows.
      int winid;
      IConnection *connection;
      TScriptParent *scriptParent;

      QString iname;
      QSplitter *split;
      TIRCView *textdata;
      config *conf;
      int WinType;
      TPictureWindow *picwin;
      QMyListWidget *listbox;
      QMenu *listboxMenu;
      QMenu *opMenu;
      QMenu *textboxMenu;
      QMyLineEdit *input;
      QString topic;
      ICommand *cmdhndl;
      QList<char> *sortrule; // All letters in order which should be sorted by. From IConnection.
      QHash<QString,member_t> members; // Members of a channel
      QStringList memberlist; // This one is also complete with nicknames+modes at all time. Add nickname here, run sort and it'll add result to listbox.

      int acIndex; // Index at autocomplete (practically, index of acMatch)
      int acCursor; // Cursor position, but before the autocompleted word. Remember to add length of a.c. word!
      QStringList acList; // Items we can get from autocomplete
      QStringList acMatch; // List of strings that matches the pattern.
      QString acPatt; // Pattern to autocomplete
      QString acPre; // All text before the autocompleted word.
      QString acPost; // All text after the autocompleted word.

      void writeToLog(QString text); // Writes to a log file under logdir and filename is target.txt
      void sortList(QList<char> *lst);
      bool sortLargerThan(const QString s1, const QString s2);
      void setModeViaList(char set, char mode);
      QString stripModeChar(QString nickname);

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
      void on_actionChannel_settings_triggered();
      void listboxDoubleClick(QListWidgetItem *item);
      void on_actionGive_op_triggered();
      void on_actionTake_op_triggered();
      void on_actionGive_voice_triggered();
      void on_actionTake_voice_triggered();
      void on_nickmenu_Query_triggered();
      void on_nickmenu_Whois_triggered();

      void on_actionKick_triggered();

signals:
      void closed(int wid);
      void activeWin(QString aw);
      void doCommand(QString command);
      void sendToSocket(QString data);
      void RequestWindow(QString name, int type, int parent, bool activate);
      void Highlight(int wid, int type);
};

#endif // IWIN_H
