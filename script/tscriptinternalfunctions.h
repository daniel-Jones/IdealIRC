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

/*! \class TScriptInternalFunctions
 *  \brief Internal functions.
 *
 * Contains all pre-defined $functions such as $calc(), $token() etc.
 */

#ifndef TSCRIPTINTERNALFUNCTIONS_H
#define TSCRIPTINTERNALFUNCTIONS_H

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
    exprtk::symbol_table<double> st; //!< Specifics for $calc() function, from exprtk library
    exprtk::expression<double> ex; //!< Specifics for $calc() function, from exprtk library
    exprtk::parser<double> parser; //!< Specifics for $calc() function, from exprtk library
    TSockFactory *sockfactory; //!< Pointer to the custom sockets manager.
    QHash<QString,int> *fnindex; //!< Pointer to the index of all functions.
    QHash<QString,TCustomScriptDialog*> *dialogs; //!< Pointer to list of all dialogs.
    QHash<int,t_sfile> *files; //!< Pointer to list of all file I/O.
    int fdc; //!< File descriptor counter.
    uint rseed; //!< Pseudo-random number seed. For $rand()

    int *activeWid; //!< Current active window ID.
    int *activeConn; //!< Current active connection.
    QHash<int,subwindow_t> *winList; //!< List of all subwindows. Pointer from IdealIRC class.
    QHash<int,IConnection*> *conList; //!< List of all IRC connections.
    subwindow_t getCustomWindow(QString name);
    subwindow_t getCustomWindow(int wid);

    int lastbtn; //!< Last button pushed within $MsgBox() function.

    // ALL functions, I mean -ALL-, even those with integer results, MUST return QString.
    QString sstr(QString text, int start, int stop = -1);
    QString token(QString text, int pos, QChar delim);
    QString rand(int lo, int hi);
    QString n(QString numc);
};

#endif // TSCRIPTINTERNALFUNCTIONS_H
