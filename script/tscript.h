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

/*! \class TScript
 *  \brief Script parser.
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

typedef struct T_SCRIPTLINE
{
    int linenum;
    QString text;
} t_scriptline;

struct scriptmenu_t
{
    QAction *action;
    int parent; // -1 for no parent ("top level"). Begins at 0.
    bool haveChildren;
};

class TScriptParent;

class TScript : public QObject
{
  Q_OBJECT

public:
    TScript(QObject *parent, TScriptParent *sp, QWidget *dialogParent, QString fname,
            QHash<int,IConnection*> *cl, QHash<int,subwindow_t> *wl, int *aWid, int *aConn);

    e_scriptresult loadScript2(QString includeFile = "", QString parent = "");
    e_scriptresult runf(QString function, QStringList param, QString &result, bool ignoreParamCount = false);

    bool runCommand(QString cmd);
    bool hasCommand(QString cmd);

    QString getName() { return name; } //!< \return QString of script name
    QString getPath() { return filename; }  //!< \return QString of /path/to/script
    QString getErrorKeyword() { return errorKeyword; }  //!< \return QString of where last error occured.
    int getCurrentLine() { return curLine; }  //!< \return Integer of current line that's parsing
    QList<scriptmenu_t> *getCustomNicklistMenu() { return &customNicklistMenu; } //!< \return List of all menu entries for Nicklist
    QList<scriptmenu_t> *getCustomChannelMenu() { return &customChannelMenu; } //!< \return List of all menu entries for Channel
    QList<scriptmenu_t> *getCustomQueryMenu() { return &customQueryMenu; } //!< \return List of all menu entries for Query
    QList<scriptmenu_t> *getCustomStatusMenu() { return &customStatusMenu; } //!< \return List of all menu entries for Status

    QString lm(int line); // Line map.

    static QString setRelativePath(QString folder, QString file); // puts file on folder unless file is a full path.

    config* getConfPtr();
    TScriptParent* getScriptParent();
    QWidget* getDlgParent();

    QHash<QString,QString>* getCommandListPtr() { return &command; } //!< \return Pointer to list of commands tied to function.
    QHash<e_iircevent,QString>* getEventListPtr() { return &tevent; } //!< \return Pointer to list of events tied to function.
    QHash<QString,TTimer*>* getTimerListPtr() { return &timers; } //!< \return Pointer to list of timers tied to timer object.

    static QString getEventStr(e_iircevent evt);

    QHash<QString,QString> *getVarListPtr() { return &variables; } //!< \return Pointer to list of all variables.
    QHash<QString,QByteArray> *getBinVarListPtr() { return &binVars; } //!< \return Pointer to list of all binary variables.

    QHash<QString,int>* getFnIndexPtr() { return &fnindex; } //!< \return Pointer to list of functions tied to index.
    QString getParamList(QString fn);

    e_scriptresult externalExtract(QString &text);

private:
    QWidget *dlgParent; //!< Pointer to the IdealIRC class. Used as parent to scriptable dialogs.
    TScriptParent *scriptParent; //!< Pointer to the script parent.
    TScriptInternalFunctions ifn; //!< All internal script functions.
    TSockFactory sockets; //!< Scriptable sockets.
    QString filename; //!< /path/to/script
    QString name; //!< Script name
    QStringList include; //!< files to include its functions.
    QHash<QString,QString> command;  //!< List of commands.\n Key: command\n Value: function
    QHash<e_iircevent,QString> tevent;  //!< List of events in script.\n Key: event\n Value: function
    QHash<QString,int> fnindex; //!< Index over functions, which byte positon to find them in.\n Key: Function\n Value: Position
    QHash<QString,TTimer*> timers; //!< List of timers.\n Key: name\n Value: timer object
    QHash<QString,QString> container; //!< Containers. Deprecated functionality.\n Key: Name\n Value: value
    QHash<QString,TCustomScriptDialog*> dialogs; //!< Scriptable dialogs.\n Key: Name\n Value: Dialog handler
    QHash<int,t_sfile> files; //!< Scriptable file I/O.\n Key: File descriptor\n Value: File handler
    QHash<QString,QString> variables; //!< Global text variables.\n Key: Name\n Value: data
    QHash<QString,QByteArray> binVars; //!< Global binary variables.\n Key: Name\n Value: data
    QMap<int,QString> lineMap; //!< Line mapping. Since lines are skewed internally due to removal of whitespace, this is needed.\n Key: internal line\n Value: line number with filename

    QAction *rootNicklistMenu; //!< The root of custom nicklist menu.
    QAction *rootChannelMenu; //!< The root of custom channel menu.
    QAction *rootQueryMenu; //!< The root of custom query menu.
    QAction *rootStatusMenu; //!< The root of custom status menu.
    QList<scriptmenu_t> customNicklistMenu; //!< Custom nicklist menu. Pointer passed to Script Parent for processing this list.
    QList<scriptmenu_t> customChannelMenu; //!< Custom channel menu. Pointer passed to Script Parent for processing this list.
    QList<scriptmenu_t> customQueryMenu; //!< Custom query menu. Pointer passed to Script Parent for processing this list.
    QList<scriptmenu_t> customStatusMenu; //!< Custom status menu. Pointer passed to Script Parent for processing this list.
    void createMenu(int &pos, char type);
    void createMenuIterate(int &pos, char type, int parent); // position is where the given menu block starts in script (byte pos). (after {)
    void resetMenu(QList<scriptmenu_t> &menu); // Use for re-parsing the menu structure
    QSignalMapper nicklistMenuMapper; //!< Map of all menu items in nicklist, tied to their triggered action.
    QSignalMapper channelMenuMapper; //!< Map of all menu items in channel, tied to their triggered action.
    QSignalMapper queryMenuMapper; //!< Map of all menu items in query, tied to their triggered action.
    QSignalMapper statusMenuMapper; //!< Map of all menu items in status, tied to their triggered action.

    int *activeWid; //!< Current active window ID.
    int *activeConn; //!< Current active connection ID.
    QHash<int,subwindow_t> *winList; //!< All subwindows, pointer to the list in IdealIRC class.
    QHash<int,IConnection*> *conList; //!< All connections.

    e_scriptresult _runf_private2(int pos, QHash<QString,QString> &localVar,
                                  QHash<QString,QByteArray> &localBinVar, QString &result);
    static e_iircevent getEvent(QString event); // Convert event string name to internal code
    void errorHandler(e_scriptresult res);

    void writeCon(QString id, QString data);
    void delCon(QString id);
    QString readCon(QString id);
    void execCmd(QString cmd) { if (cmd.length() > 0) { emit execCmdSignal(cmd); } }
    QString mergeVarName(QString &vname, QHash<QString, QString> &localVar, QHash<QString, QByteArray> &localBinVar);
    e_scriptresult escape(QString &text, int *i, QString *result);
    e_scriptresult extract(QString &text, QHash<QString, QString> &localVar, QHash<QString, QByteArray> &localBinVar, bool extractVariables = true); // extract functions and variables
    e_scriptresult extractFunction(QString &text, QString &result, int *pos, QHash<QString,QString> &localVar, QHash<QString,QByteArray> &localBinVar);
    QByteArray extractBinVars(QString &text, QHash<QString,QByteArray> &localBinVar);
    bool solveBool(QString &data, QHash<QString, QString> &localVar, QHash<QString, QByteArray> &localBinVar);
    bool solveLogic(QString &data, QHash<QString,QString> &localVar, QHash<QString,QByteArray> &localBinVar);

    void delWhitespace(QString *text);

    QString scriptstr; //!< The script resides here, without whitespaces.
    QString errorKeyword; //!< What portion of code that produced last error.

    int curLine; //!< Current real line number.
    int loadIntLine; //!< Used when mapping line numbers, this is for internal line numbers.
    bool lastIFresult; //!< The last IF result, to be used with ELSE statement.\n \bug This is sort of a bad practise, not allowing to nest ELSE statements.

    bool customDialogShow(QString oname);
    bool customDialogHide(QString oname);
    bool customDialogClose(QString oname);
    bool customDialogSetLabel(QString dlg, QString oname, QString text);
    bool customDialogAddItem(QString dlg, QString oname, QString text);
    bool customDialogReItem(QString dlg, QString oname, QString index, QString text);
    bool customDialogDelItem(QString dlg, QString oname, QString index);
    bool customDialogClear(QString dlg, QString oname);

public slots:
    void timerTimeout(QString fn);
    bool runEvent(e_iircevent evt, QStringList param, QString *result = nullptr);

private slots:
    void nicklistMenuItemTriggered(QString function);
    void channelMenuItemTriggered(QString function);
    void queryMenuItemTriggered(QString function);
    void statusMenuItemTriggered(QString function);

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
