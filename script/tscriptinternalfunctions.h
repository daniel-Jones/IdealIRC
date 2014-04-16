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

#ifndef TSCRIPTINTERNALFUNCTIONS_H
#define TSCRIPTINTERNALFUNCTIONS_H

/*
  Gotta love long class names.
  This one contains all pre-defined $functions such as $calc(), $token(), etc
*/

#include <QObject>
#include <QStringList>
#include <QHash>
#include "tsockfactory.h"
#include "tcustomscriptdialog.h"

#include "exprtk/exprtk.hpp"


typedef struct T_SFILE {
    int fd;
    bool read;
    bool write;
    bool binary;
    QFile *file;
} t_sfile;

class TScriptInternalFunctions : public QObject
{
    Q_OBJECT

public:
    explicit TScriptInternalFunctions(TSockFactory *sf, QHash<QString,int> *functionIndex,
                                      QHash<QString,TCustomScriptDialog*> *dlgs, QHash<int,t_sfile> *fl,
                                      QHash<int,IConnection*> *cl, QHash<int,subwindow_t> *wl,
                                      int *aWid, int *aConn, QObject *parent = 0);

    bool runFunction(QString function, QStringList param, QString &result);
    QString calc(QString expr); // This one should go public since it also interprets string literals for calculation.

private:
    exprtk::symbol_table<double> st;
    exprtk::expression<double> ex;
    exprtk::parser<double> parser;
    TSockFactory *sockfactory;
    QHash<QString,int> *fnindex;
    QHash<QString,TCustomScriptDialog*> *dialogs;
    QHash<int,t_sfile> *files;
    int fdc; // file desc. counter
    uint rseed;

    int *activeWid;
    int *activeConn;
    QHash<int,subwindow_t> *winList;
    QHash<int,IConnection*> *conList;
    subwindow_t getCustomWindow(QString name);

    int lastbtn;

    // ALL functions, I mean -ALL-, even those with integer results, MUST return QString.
    QString sstr(QString text, int start, int stop = -1);
    QString token(QString text, int pos, QChar delim);
    QString rand(int lo, int hi);
};

#endif // TSCRIPTINTERNALFUNCTIONS_H
