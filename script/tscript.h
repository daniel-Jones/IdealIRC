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
#include <QFile>

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

enum e_scriptresult {
  se_None = 0,
  se_Finished,
  se_FileNotExist,
  se_FileEmpty,
  se_FileCannotOpen,
  se_UnexpectedFinish = 5,
  se_RunfDone,
  se_BreakNoWhile,
  se_ContinueNoWhile,
  se_InvalidParamCount,
  se_FunctionEmpty = 10,
  se_InvalidFunction,
  se_InvalidCommand,
  se_InvalidMetaCommand,
  se_InvalidSwitches,
  se_InvalidIncludeFile = 15,
  se_InvalidEvent,
  se_InvalidBlockType,
  se_InvalidTimer,
  se_InvalidFileDescriptor,
  se_MissingVariable = 20,
  se_FunctionIdxOutOfBounds,
  se_TimerAlreadyDefined,
  se_UnexpectedToken,
  se_EscapeOnEndLine,
  se_UnexpectedNewline = 25,
  se_NegativeNotAllowed,
  se_FSeekFailed
};

class TScript : public QObject
{

  Q_OBJECT

  public:
    TScript(QObject *parent, QWidget *dialogParent, QString fname);
    e_scriptresult loadScript2(QString includeFile = "", QString parent = "");
    e_scriptresult runf(QString function, QStringList param, QString &result, bool ignoreParamCount = false);

    bool runCommand(QString cmd);

    bool hasCommand(QString cmd);

    QString getName() { return name; }
    QString getPath() { return filename; }
    QString getErrorKeyword() { return errorKeyword; }
    int getCurrentLine() { return curLine; }

  private:
    QWidget *dlgParent;
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
    e_scriptresult _runf_private2(int pos, QString function, QStringList *varName, QStringList *varData, QHash<QString,QByteArray> *binVar, QString &result);
    e_iircevent getEvent(QString event); // Convert event string name to internal code
    void errorHandler(e_scriptresult res);

    void writeCon(QString id, QString data);
    void delCon(QString id);
    QString readCon(QString id);
    void execCmd(QString cmd) { if (cmd.length() > 0) { emit execCmdSignal(cmd); } }
    QString extractVars(QString &text, QStringList *varName, QStringList *varData, QHash<QString,QByteArray> *binVar);
    QByteArray extractBinVars(QString &text, QHash<QString, QByteArray> *binVar);
    bool solveBool(QString &data, QStringList *varName, QStringList *varData, QHash<QString, QByteArray> *binVar);
    bool solveLogic(QString &data, QStringList *varName, QStringList *varData, QHash<QString, QByteArray> *binVar);
    QString extractFunction(QString &data, QStringList *varName, QStringList *varData, QHash<QString,QByteArray> *binVar); // data <- a script line to execute, replacing $function(para) with a result, if any.
                                                                                       // We need variable data to extract any possible variables before we send the data to function.
    void delWhitespace(QString *text);
    QString setRelativePath(QString folder, QString file); // puts file on folder unless file is a full path.

    QString scriptstr; // Save script here, no whitespaces
    QString errorKeyword;
    int curLine;
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
