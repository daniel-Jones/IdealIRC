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

#include "tscriptinternalfunctions.h"
#include "math.h"
#include "iwin.h"
#include "iconnection.h"

#include <iostream>
#include <QDateTime>
#include <QFontMetrics>
#include <QDebug>
#include <QApplication>

TScriptInternalFunctions::TScriptInternalFunctions(TSockFactory *sf, QHash<QString,int> *functionindex,
                                                   QHash<QString,TCustomScriptDialog*> *dlgs, QHash<int,t_sfile> *fl,
                                                   QHash<int,IConnection*> *cl, QHash<int,subwindow_t> *wl,
                                                   int *aWid, int *aConn, QObject *parent) :
    QObject(parent),
    sockfactory(sf),
    fnindex(functionindex),
    dialogs(dlgs),
    files(fl),
    fdc(1),
    activeWid(aWid),
    activeConn(aConn),
    winList(wl),
    conList(cl)
{
    st.add_pi();
    ex.register_symbol_table(st);
}

bool TScriptInternalFunctions::runFunction(QString function, QStringList param, QString &result)
{
    QString fn = function.toUpper();
    if (fn == "ACOS") {
        if (param.length() < 1)
            return false;

        bool ok = false;
        double v = param[0].toDouble(&ok);

        if (!ok)
            v = 0;

        result = QString::number(acos(v));
        return true;
    }

    if (fn == "ASIN") {
        if (param.length() < 1)
            return false;

        bool ok = false;
        double v = param[0].toDouble(&ok);

        if (!ok)
            v = 0;

        result = QString::number(asin(v));
        return true;
    }

    if (fn == "ATAN") {
        if (param.length() < 1)
            return false;

        bool ok = false;
        double v = param[0].toDouble(&ok);

        if (!ok)
            v = 0;

        result = QString::number(atan(v));
        return true;
    }

    if (fn == "BUILDTYPE") {
#ifdef STANDALONE
        result = "STANDALONE";
#endif
#ifdef PACKAGED
        result = "PACKAGED";
#endif
        return true;
    }

    if (fn == "CALC") {
        // Calculate an expression (e.g. 5+5)
        if (param.length() == 0)
            return false;
        QString expr = param[0];
        result = calc(expr);
        return true;
    }

    if (fn == "COS") {
        if (param.length() < 1)
            return false;

        bool ok = false;
        double v = param[0].toDouble(&ok);

        if (!ok)
            v = 0;

        result = QString::number(cos(v));
        return true;
    }

    if (fn == "COLORAT") { // $ColorAt(@window, layer, x, y)
        if (param.length() < 3)
            return false;
        QString layer = "main";
        if (param.length() > 3) {
            layer = param[1];
            param.removeAt(1);
        }

        subwindow_t sw = getCustomWindow(param[0]);
        if (sw.type == WT_NOTHING)
            return false;
        int x = floor( param[1].toFloat() );
        int y = floor( param[2].toFloat() );

        result = sw.widget->picwinPtr()->colorAt(layer, x, y);
        return true;
    }

    if (fn == "CURWINTYPE") {
        // Returns the current target type (msg or channel)
        subwindow_t sw = winList->value(*activeWid);
        if (sw.type == WT_CHANNEL)
            result = "CHANNEL";
        else if (sw.type == WT_GRAPHIC)
            result = "GRAPHIC";
        else if (sw.type == WT_GWINPUT)
            result = "GRAPHICINPUT";
        else if (sw.type == WT_NOTHING)
            result = "NOTHIG";
        else if (sw.type == WT_PRIVMSG)
            result = "PRIVMSG";
        else if (sw.type == WT_STATUS)
            result = "STATUS";
        else if (sw.type == WT_TXTONLY)
            result = "TXTONLY";
        else
            result = "UNKNOWN";

        return true;
    }

    if (fn == "DLG") {
        // Returns information of a dialog or its objects
        // $dlg(dialog,object)
        if (param.count() == 2) {
            QString dlg = param[0];
            QString object = param[1];

            QHashIterator<QString,TCustomScriptDialog*> i(*dialogs);
            while (i.hasNext()) {
                i.next();
                if (i.key().toUpper() == dlg.toUpper()) {
                    result = i.value()->getLabel(object);
                    return true;
                }
            }
        }

        // $dlg(dialog,object,index)
        if (param.count() == 3) {
            QString dlg = param[0];
            QString object = param[1];
            QString index = param[2];

            QHashIterator<QString,TCustomScriptDialog*> i(*dialogs);
            while (i.hasNext()) {
                i.next();
                if (i.key().toUpper() == dlg.toUpper()) {
                    result = i.value()->getItem(object, index.toInt());
                    return true;
                }
            }
        }

        // Default
        return false;
    }

    if (fn == "FILE") {
        // Returns a file descriptor by opening a file for read and|or write
        // $file(file.name, rwb)
        // result: 0 cannot open, -1 not existing
        if (param.count() < 2) {
            result = "0";
            return true;
        }
        QString mode = param[1];

        bool read = false;
        bool write = false;
        bool binary = false;
        bool append = false;
        bool switchfail = false;
        for (int i = 0; i <= mode.length()-1; i++) {
            char c = mode[i].toLatin1();
                switch (c) {
                    case 'a':
                        append = true;
                        continue;
                    case 'r':
                        read = true;
                        continue;
                    case 'w':
                        write = true;
                        continue;
                    case 'b':
                        binary = true;
                        continue;
                    default:
                        switchfail = true;
                        break;
            }
        }

        if (switchfail == true) {
            result = "0";
            return true;
        }

        if ((read || write) == false)
            read = true;

        QIODevice::OpenMode om = 0;

        if (read)
            om |= QIODevice::ReadOnly;
        if (write)
            om |= QIODevice::WriteOnly;
        if (! binary)
            om |= QIODevice::Text;

        if (append)
            om |= QIODevice::Append;

        if (om == 0) {
            result = "0";
            return true;
        }

        QFile *f = new QFile(param[0]);

        if (! f->open(om)) {
            result = "0";
            return true;
        }

        t_sfile ts;
        ts.binary = binary;
        ts.read = read;
        ts.write = write;
        ts.fd = fdc;
        ts.file = f;

        files->insert(fdc, ts);
        result = QString::number(fdc++);
        return true;
    }

    if (fn == "FNEXIST") {
        // Checks if an actual function exists; This will NOT work on "internal" functions (these in here)
        if (param.count() != 1)
            return false;

        int idx = fnindex->value(param[0].toUpper(), -1);

        if (idx > -1)
            result = "1";
        else
            result = "0";
        return true;
    }

    if (fn == "GLUE") {
        // "glue" texts together.
        // $glue(hello,big,world) will return hellobigworld
        QString r;
        for (int i = 0; i <= param.length()-1; i++)
            r += param[i];

        result = r;
        return true;
    }

    if (fn == "HOSTMASK") {
        // Returns hostmask *!*@host.name of nickname if IAL got it.
        // Otherwise, if IAL doesn't, it returns nickname!*@* as hostmask.
        if (param.count() != 1) {
            result.clear();
            return false;
        }
        QString nickname = param[0];

        IConnection *con = conList->value(*activeConn);
        QString host = con->ial.getHost(nickname);
        if (host.isEmpty()) {
            host = nickname;
            host.append("!*@*");
        }
        else
            host.prepend("*!*@");

        result = host;

        return true;
    }

    if (fn == "IALHOSTMASK") {
        // Returns hostname of nickname if IAL got it, otherwise empty text.
        // Better off using $hostmask() instead.
        if (param.count() != 1) {
            result.clear();
            return false;
        }
        QString nickname = param[0];

        IConnection *con = conList->value(*activeConn);
        QString host = con->ial.getHost(nickname);

        if (! host.isEmpty())
            host.prepend("*!*@");

        result = host;

        return true;
    }

    if (fn == "LEN") {
        // Counts amount of letters in a given text
        if (param.count() == 0) {
            result = "0";
            return true;
        }
        result = QString::number( param.at(0).length() );
        return true;
    }

    if (fn == "NULL") {
        // Returns empty
        result.clear();
        return true;
    }

    if (fn == "PATH") {
        // $path(type)
        // Returns a file path to the given type.
        if (param.count() != 1)
            return false;

        QString type = param[0].toUpper();
        if (type == "CONFIG")
            result = CONF_PATH;
        if (type == "COMMON")
            result = COMMON_PATH;
        if (type == "EXEC")
            result = QApplication::applicationDirPath();
        if (type == "SKEL")
            result = SKEL_PATH;

        return true;
    }

    if (fn == "RAND") {
        // Pseudo-random number generator
        if (param.length() < 2)
            return false;
        int lo = param[0].toInt();
        int hi = param[1].toInt();
        result = rand(lo, hi);
        return true;
    }

    if (fn == "SIN") {
        if (param.length() < 1)
            return false;

        bool ok = false;
        double v = param[0].toDouble(&ok);

        if (!ok)
            v = 0;

        result = QString::number(sin(v));
        return true;
    }

    if (fn == "SOCKBUFLEN") {
        // Returns amount of bytes left in sockread buffer
        if (param.length() < 1)
            return false;

        result = sockfactory->sockBufLen(param[0]);
        return true;
    }

    if (fn == "SOCKLIST") {
        // Find socket names.
        // $socklist(patt_*, pos)
        // If pos is zero, that will return amount of socket names the pattern matches.
        // If pos > 0, this will return an actual socket name it matches.
        if (param.length() < 2)
            return false;

        result = sockfactory->socklist(param[0], param[1].toInt());
        return true;
    }

    if (fn == "SSTR") {
        // Substring, returns text by given positions inside the text.
        // $SSTR(The text, start, end) end is optional.
        if (param.length() < 2)
            return false;

        int start = param[1].toInt();
        int stop = -1;
        if (param.length() >= 3)
            stop = param[2].toInt();

        QString text = param[0];
        result = sstr(text, start, stop);
        return true;
    }

    if (fn == "TAN") {
        if (param.length() < 1)
            return false;

        bool ok = false;
        double v = param[0].toDouble(&ok);

        if (!ok)
            v = 0;

        result = QString::number(tan(v));
        return true;
    }

    if (fn == "TARGET") {
        // Returns the current target to send messages to (msg or channel)
        subwindow_t sw = winList->value(*activeWid);
        result.clear();
        if ((sw.type == WT_CHANNEL) || (sw.type == WT_PRIVMSG))
            result = sw.widget->getTarget();
        return true;
    }

    if (fn == "TEXTWIDTH") {
        // Return text width in pixles by given font name and size
        // $textwidth(font, size, text)
        if (param.count() != 3)
            return false;

        QFont font(param[0]);
        font.setPixelSize(param[1].toInt());

        QFontMetrics fm(font);
        result = QString::number( fm.width(param[2]) );

        return true;
    }

    if (fn == "TOKEN") {
        // Get a text by tokens
        // $token(text here, position, token)  the token is in ascii number.
        // If the position is zero, this will count amount of text items separated by the given token
        if (param.length() < 3)
        return false;

        bool ok = false;

        QString tcnum = param[2]; // token character (ascii num)
        QChar tc = tcnum.toInt(&ok); // converted from string-number to actual number, into a character.

        if (ok == false)
            return false;

        int p = param[1].toInt(&ok); // Which position to use

        if (ok == false)
            return false;

        result = token(param[0], p, tc);

        return true;
    }

    if (fn == "VERSION") {
        // IIRC version.
        result = VERSION_STRING;
        return true;
    }

    // No functions were matching, return false as error.
    return false;
}

subwindow_t TScriptInternalFunctions::getCustomWindow(QString name)
{
    subwindow_t error;
    error.type = WT_NOTHING; // Indicates an error.

    QHashIterator<int,subwindow_t> i(*winList);
    while (i.hasNext()) {
        subwindow_t sw = i.next().value();
        if (sw.widget->objectName().toUpper() == name.toUpper())
            return sw;
    }

    return error;
}


QString TScriptInternalFunctions::calc(QString expr)
{
    parser.compile(expr.toStdString(), ex);
    return QString::number(ex.value());
}

QString TScriptInternalFunctions::sstr(QString text, int start, int stop)
{
    QString res = text.mid(start, stop);
    return res;
}

QString TScriptInternalFunctions::token(QString text, int pos, QChar delim)
{
    QStringList sl = text.split(delim); // Split up the string using tokenizer

    if (pos == 0) {
        // Count tokens
        return QString::number(sl.count());
    }

    if (pos > sl.count())
        return QString();

    return sl.at(pos-1);
}

QString TScriptInternalFunctions::rand(int lo, int hi)
{
    qsrand( rseed++ );
    return QString::number( qrand() % ((hi + 1) - lo) + lo );
}
