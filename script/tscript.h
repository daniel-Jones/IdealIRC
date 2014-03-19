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

#ifndef TSCRIPT_H
#define TSCRIPT_H

#include <QWidget>
#include <QStringList>
#include <QHash>
#include <QList>
#include <QFile>
#include <QAction>
#include <QSignalMapper>

#include "constants.h"
#include "tscriptinternalfunctions.h"
#include "ttimer.h"
#include "tsockfactory.h"
#include "tcustomscriptdialog.h"

typedef struct T_SCRIPT
{
    QString name;
    QString path;
} script_t;

typedef struct T_SCRIPTLINE {
    int linenum;
    QString text;
} t_scriptline;

class TScriptParent;

class TScript : public QObject
{
  Q_OBJECT

public:
    TScript(QObject *parent, TScriptParent *sp, QWidget *dialogParent, QString fname,
            QHash<int,IConnection*> *cl, QHash<int,subwindow_t> *wl, int *aWid, int *aConn/*,
            QMenu *nicklist, QMenu *channel, QMenu *status, QMenu *privmsg*/);

    e_scriptresult loadScript2(QString includeFile = "", QString parent = "");
    e_scriptresult runf(QString function, QStringList param, QString &result, bool ignoreParamCount = false);

    bool runCommand(QString cmd);
    bool hasCommand(QString cmd);

    QString getName() { return name; }
    QString getPath() { return filename; }
    QString getErrorKeyword() { return errorKeyword; }
    int getCurrentLine() { return curLine; }
    QList<QAction*> *getCustomNicklistMenu() { return &customNicklistMenu; }
    QList<QAction*> *getCustomChannelMenu() { return &customChannelMenu; }

    QString lm(int line); // Line map.

    static QString setRelativePath(QString folder, QString file); // puts file on folder unless file is a full path.

private:
    QWidget *dlgParent;
    TScriptParent *scriptParent;
    TScriptInternalFunctions ifn;
    TSockFactory sockets;
    QString filename;
    QString name;
    QStringList include; // files to include its functions.
    QHash<QString,QString> command; // list of commands, 1: command 2: function
    QHash<e_iircevent,QString> tevent; // list of events in script, 1: event 2: function
    QHash<QString,int> fnindex; // index over functions, which line to find them in ('script')
    QHash<QString,TTimer*> timers; // list of timers, 1: id, 2: timer object
    QHash<QString,QString> container; // data container that's accessible by all function in current script.
    QHash<QString,TCustomScriptDialog*> dialogs;
    QHash<int,t_sfile> files; // file descriptor, QFile
    QHash<QString,QString> variables;
    QHash<QString,QByteArray> binVars;
    QMap<int,QString> lineMap; // key: internal line, value: line number with filename

    QList<QAction*> customNicklistMenu;
    QList<QAction*> customChannelMenu;
    void createMenu(int &pos, char type, QMenu *parent); // position is where the given menu block starts in script (byte pos). (after {)
    void resetMenu(QList<QAction *> &menu); // Use for re-parsing the menu structure
    QSignalMapper nicklistMenuMapper;
    QSignalMapper channelMenuMapper;

    int *activeWid;
    int *activeConn;
    QHash<int,subwindow_t> *winList;
    QHash<int,IConnection*> *conList;

    e_scriptresult _runf_private2(int pos, QStringList *parName, QString &result);
    e_iircevent getEvent(QString event); // Convert event string name to internal code
    void errorHandler(e_scriptresult res);

    void writeCon(QString id, QString data);
    void delCon(QString id);
    QString readCon(QString id);
    void execCmd(QString cmd) { if (cmd.length() > 0) { emit execCmdSignal(cmd); } }
    e_scriptresult extract(QString &text, bool extractVariables = true); // extract functions and variables
    e_scriptresult extractFunction(QString &text, QString &result, int *pos);
    QString extractVarsOld(QString &text, QStringList *varName, QStringList *varData, QHash<QString,QByteArray> *binVar);
    QByteArray extractBinVars(QString &text);
    bool solveBool(QString &data);
    bool solveLogic(QString &data);
    QString extractFunctionOld(QString &data, QStringList *varName, QStringList *varData, QHash<QString,QByteArray> *binVar); // data <- a script line to execute, replacing $function(para) with a result, if any.
                                                                                       // We need variable data to extract any possible variables before we send the data to function.
    void delWhitespace(QString *text);

    QString scriptstr; // Save script here, no whitespaces
    QString errorKeyword;
    int curLine;
    int loadIntLine; // used when mapping line numbers, this is for internal line numbers.
    bool lastIFresult;

    bool customDialogShow(QString oname);
    bool customDialogHide(QString oname);
    bool customDialogClose(QString oname);
    bool customDialogSetLabel(QString dlg, QString oname, QString text);
    bool customDialogAddItem(QString dlg, QString oname, QString text);
    bool customDialogDelItem(QString dlg, QString oname, QString index);
    bool customDialogClear(QString dlg, QString oname);

public slots:
    void timerTimeout(QString fn);
    bool runEvent(e_iircevent evt, QStringList param);

private slots:
    void nicklistMenuItemTriggered(QString function);
    void channelMenuItemTriggered(QString function);

signals:
    void execCmdSignal(QString cmd); // command param param2 ...
    void error(QString text);
    void warning(QString text);
    void echo(QString text);
    void echo(QString target, QString text);
    void stopURLDisplay();
    void toolbarAdd(QString toolname, QString scriptname, QString tooltip);
    void toolbarDel(QString toolname);
    void toolbarSetIcon(QString toolname, QString path);
    void toolbarSetFunction(QString toolname, QString scriptname, QString function);

};

#endif // TSCRIPT_H
