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

#ifndef TSCRIPTPARENT_H
#define TSCRIPTPARENT_H

#include <QWidget>
#include <QStringList>
#include <QVector>
#include <QHash>
#include <QAction>

#include "config.h"
#include "tscript.h"
#include "tscriptcommand.h"
#include "constants.h"

class ICommand;
class IConnection;

typedef struct T_CUSTOM_TOOLBAR {
    QString scriptname;
    QString tooltip;
    QString function;
    QString iconpath;
} toolbar_t;

class TScriptParent : public QObject
{
    Q_OBJECT

public:
    TScriptParent(QObject *parent, QWidget *dialogParent, config *cfg, QHash<int,IConnection*> *cl, QHash<int,subwindow_t> *wl, int *aWid, int *aConn);
    bool command(QString cmd); // Exec any custom commands, returns false if command not found in loaded scripts.
    bool runevent(e_iircevent event, QStringList param);
    bool runevent(e_iircevent event);
    bool loadScript(QString path, bool starting = false); // starting is true when IIRC is starting up.
    bool unloadScript(QString name);
    bool reloadScript(QString name);
    void getLoadedScripts(QHash<QString, QString> &list); // Pass a pointer as parameter, where this function will fill the list: [scriptname,/path/to/script]
    void saveLoadedScripts();
    void loadAllScripts();
    void getToolbarPtr(QHash<QString,toolbar_t> **tb) { *tb = &toolbar; }
    void runScriptFunction(QString script, QString function);
    QList<QAction*> getCustomNicklistMenu(); // Get items from all scripts.
    QList<QAction*> getCustomChannelMenu(); // Get items from all scripts.
    QStringList getCurrentNickSelection(); // Gets selected nicknames in active window (used for custom nicklist menu items)
    QString getCurrentWindow(); // Gets the current window that's active

signals:
    void refreshToolbar();
    void RequestWindow(QString name, int type, int parent, bool activate);

public slots:
    void execCmdSlot(QString cmd);

private slots:
    void gotScriptError(QString text) { echo("Script error: " + text, PT_LOCALINFO); }
    void gotScriptWarning(QString text) { echo("Script warning: " + text, PT_LOCALINFO); }
    void echo(QString text) { cmdhndl.echo("STATUS", text); }
    void echo(QString target, QString text) { cmdhndl.echo(target, text); }
    void stopURLDisplay() { displayURL = false; } // to be used with event urlclick.
    void toolbarAdd(QString toolname, QString scriptname, QString tooltip);
    void toolbarDel(QString toolname);
    void toolbarSetIcon(QString toolname, QString path);
    void toolbarSetFunction(QString toolname, QString scriptname, QString function);

  private:
    config *conf;
    TScriptCommand cmdhndl;
    int *activeWid;
    int *activeConn;
    QVector<TScript*> scriptlist;
    QHash<QString,toolbar_t> toolbar;
    QHash<int,IConnection*> *conlist;
    QHash<int,subwindow_t> *winlist;
    QWidget *dlgParent;
    bool displayURL; // used on UrlClick event.
    void echo(QString text, int type) { cmdhndl.echo("STATUS", text, type); }
    void echo(QString target, QString text, int type) { cmdhndl.echo(target, text, type); }
    bool loader(TScript *script, int *errcode = NULL);
};

#endif // TSCRIPTPARENT_H
