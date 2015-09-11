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
#include "tscript.h"

#include <iostream>
#include <QDateTime>
#include <QFontMetrics>
#include <QDebug>
#include <QApplication>
#include <QInputDialog>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QFileInfoList>

TScriptInternalFunctions::TScriptInternalFunctions(TSockFactory *sf, QHash<QString,int> *functionindex,
                                                   QHash<QString,TCustomScriptDialog*> *dlgs, QHash<int,t_sfile> *fl,
                                                   QHash<int,IConnection*> *cl, QHash<int,subwindow_t> *wl,
                                                   int *aWid, int *aConn, TScript *scr, QObject *parent) :
    QObject(parent),
    sockfactory(sf),
    fnindex(functionindex),
    dialogs(dlgs),
    files(fl),
    fdc(1),
    activeWid(aWid),
    activeConn(aConn),
    winList(wl),
    conList(cl),
    script(scr)
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

    if (fn == "ACTIVE") {
        result = getCustomWindow(*activeWid).widget->objectName();
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

    if (fn == "BUTTON") {
        if (lastbtn == QMessageBox::Ok)
            result = "OK";
        if (lastbtn == QMessageBox::Cancel)
            result = "CANCEL";
        if (lastbtn == QMessageBox::Yes)
            result = "YES";
        if (lastbtn == QMessageBox::No)
            result = "NO";

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

    if (fn == "CEIL") {
        if (param.count() != 1)
            return false;

        result = QString::number( ceil ( param[0].toFloat() ) );

        return true;
    }

    if (fn == "COUNT") {
        // counts users in a channel
        if (param.length() == 0)
            return false;

        IConnection* con = conList->value(*activeConn);
        result = QString::number( con->ial.userCount(param[0]) );
        return true;
    }

    if (fn == "CHAR") {
        if (param.length() == 0)
            return false;

        result = QChar( param[0].toInt() );
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

    if (fn == "COLORAT") { // $ColorAt(@window[, layer], x, y)
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
        else if (sw.type == WT_TXTINPUT)
            result = "TXTINPUT";
        else
            result = "UNKNOWN";

        return true;
    }

    if (fn == "DIR") {
        // $dir(directory, N, O)
        //
        // N index of directory; N=0 counts all items inside the dir and return the result.
        // N>0 specifies each item inside the dir and returns the item name
        //
        // O is options.
        // 'h' shows hidden files.
        // 's' lists all items in the dir, separated by 0x01 character.
        //   The 's' option ignores N, so the convention will be set N=0 when using O=s.
        // 'e' determines if directory exist or not. returns 1 if true, 0 otherwise.
        //   The 'e' option ignores N.

        if ((param.count() < 2) || (param.count() > 3))
            return false;

        QString dir = param[0];
        QString N = param[1];
        QString O;

        QDir dobj(dir);

        if (param.count() == 3)
            O = param[2];

        dobj.setFilter(QDir::NoDotAndDotDot | QDir::AllEntries);

        qDebug() << dir << N << O;

        if (O.contains('h')) // Show hidden files
            dobj.setFilter(QDir::NoDotAndDotDot | QDir::Hidden | QDir::AllEntries);

        if (O.contains('s')) {
            // List all files to result, separated by :
            if (! dobj.exists())
                return false;
/*
            QStringList dlist = dobj.entryList(dobj.filter(), QDir::Name | QDir::DirsFirst);
            qDebug() << "dlist:" << dlist;
            for (int i = 0; i <= dlist.count()-1; ++i)
                if (result.isEmpty())
                    result += dlist[i];
                else
                    result += QChar(':') + dlist[i];
*/

            QFileInfoList files = dobj.entryInfoList(dobj.filter(), QDir::Name | QDir::DirsFirst);
            qDebug() << "Listing files";
            foreach (QFileInfo file, files) {
                QString name = file.fileName();
                if (file.isDir()) {
                    if (result.isEmpty())
                        result += name + "/";
                    else
                        result += ":" + name + "/";
                }
                else {
                    if (result.isEmpty())
                        result += name;
                    else
                        result += ":" + name;
                }
            }

            return true;
        }

        if (O.contains('e')) {
            // Check if specified directory exist.
            if (dobj.exists())
                result = "1";
            else
                result = "0";

            return true;
        }

        // Do regular directory listing here

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

    if (fn == "FLOOR") {
        if (param.count() != 1)
            return false;

        result = QString::number( floor( param[0].toFloat() ) );

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

    if (fn == "HEIGHT") {
        /*

          $height - Returns height of IIRC window

          $height(@window) - Returns height of (text|paint) widget of @window.
                            Works for any windows.

          $height(#channel, l) - Returns height of listbox (l for Larry)

*/
        if (param.count() < 1) {
            result.clear();
            return false;
        }
        subwindow_t sw = getCustomWindow(param[0]);
        if (sw.type == WT_NOTHING)
            return false;

        if (param.count() > 1)
            result = QString::number( sw.widget->listboxHeight() );
        else
            result = QString::number( sw.widget->height() );

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

    if (fn == "INPUT") {
        // $input(caption, label)
        if (param.count() != 2) {
            return false;
        }
        bool ok = false;
        QString input = QInputDialog::getText(0, param[0], param[1],
                                              QLineEdit::Normal, "", &ok);

        if (ok)
            lastbtn = QMessageBox::Ok;
        else
            lastbtn = QMessageBox::Cancel;

        result = input;
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

    if (fn == "MSGBOX") {
        // $msgbox(type, caption, label[, buttons])
        // types: critical, info, question, warning
        // buttons default: ok|cancel
        // buttons argument: o=ok, c=cancel, q=yes|no
        QMessageBox::StandardButtons btn = 0;
        if (param.count() == 3) {
            // no buttons, set default (ok|cancel)
            btn = QMessageBox::Ok | QMessageBox::Cancel;
        }
        else if (param.count() == 4) {
            for (int i = 0; i <= param[3].count()-1; ++i) {
                char c = param[3][i].toLatin1();
                switch (c) {
                    case 'o':
                        btn |= QMessageBox::Ok;
                        break;
                    case 'c':
                        btn |= QMessageBox::Cancel;
                        break;
                    case 'q':
                        btn |= (QMessageBox::Yes | QMessageBox::No);
                        break;
                }
            }
        }
        else
            return false;

        int b;
        if (param[0].toUpper() == "CRITICAL")
            b = QMessageBox::critical(0, param[1], param[2], btn);

        else if (param[0].toUpper() == "INFO")
            b = QMessageBox::information(0, param[1], param[2], btn);

        else if (param[0].toUpper() == "QUESTION")
            b = QMessageBox::question(0, param[1], param[2], btn);

        else if (param[0].toUpper() == "WARNING")
            b = QMessageBox::warning(0, param[1], param[2], btn);

        if (b == QMessageBox::Ok)
            result = "OK";
        if (b == QMessageBox::Cancel)
            result = "CANCEL";
        if (b == QMessageBox::Yes)
            result = "YES";
        if (b == QMessageBox::No)
            result = "NO";

        lastbtn = b;

        return true;
    }

    if (fn == "N") {
        // Numeric conversion
        result.clear();
        if (param.count() == 0)
            return true;

        result = n(param[0]);
        return true;
    }

    if (fn == "NICKLIST") {
        /*
            $nicklist           | Return first selected nickname in current channel
            $nicklist(#chan)    | Return first selected nickname in #chan
            $nicklist(N)        | Return index N of selected nicknames in current channel
            $nicklist(#chan, N) | Return index N of selected nicknames in #chan
            N = 0: return amount of selected nicknames
        */
        result.clear();

        if (param.count() == 0) {
            subwindow_t sw = getCustomWindow(*activeWid);
            if (sw.type == WT_NOTHING)
                return true;
            if (sw.widget->getSelectedMembers().count() > 0)
                result = sw.widget->getSelectedMembers().at(0);
            return true;
        }

        if (param.count() == 1) {
            // either #channel or N
            bool ok = false;
            int N = param[0].toInt(&ok);
            if (!ok) {
                // #Channel
                subwindow_t sw = getCustomWindow(param[0]);
                if (sw.type == WT_NOTHING)
                    return true;
                if (sw.widget->getSelectedMembers().count() > 0)
                    result = sw.widget->getSelectedMembers().at(0);
                return true;
            }
            else {
                // N
                subwindow_t sw = getCustomWindow(*activeWid);
                if (sw.type == WT_NOTHING)
                    return true;
                if (N <= 0) {
                    result = QString::number( sw.widget->getSelectedMembers().count() );
                    return true;
                }
                if (sw.widget->getSelectedMembers().count() >= N)
                    result = sw.widget->getSelectedMembers().at(N-1);
                return true;
            }
        }

        if (param.count() == 2) {
            // #Channel, N
            bool ok = false;
            int N = param[1].toInt(&ok);
            if (!ok)
                return true;

            subwindow_t sw = getCustomWindow(param[0]);
            if (sw.type == WT_NOTHING)
                return true;
            if (N <= 0) {
                result = QString::number( sw.widget->getSelectedMembers().count() );
                return true;
            }
            if (sw.widget->getSelectedMembers().count() >= N)
                result = sw.widget->getSelectedMembers().at(N-1);
            return true;
        }

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
#ifdef PACKAGED
        if (type == "COMMON")
            result = COMMON_PATH;
        if (type == "SKEL")
            result = SKEL_PATH;
#endif
        if (type == "EXEC")
            result = QApplication::applicationDirPath();

        if (type == "SCRIPT") {
            QString fullfn = script->getPath();
            QString fn = fullfn.split('/')[fullfn.split('/').count()-1];
            result = fullfn.mid(0, fullfn.length()-fn.length());
        }

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

    if (fn == "READINI") {
        // $ReadIni(file, section, item)
        if (param.length() < 3)
            return false;

        result = IniFile(param[0]).ReadIni(param[1], param[2]);
        return true;
    }

    if (fn == "REPLACE") {
        // $replace(text, search, replace)
        if (param.length() < 3)
            return false;

        result = param[0].replace(param[1], param[2]);
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

    if (fn == "FSIZE") {
        if (param.length() < 1)
            return false;

        QFile f(param[0]);
        qint64 size = 0;
        if (f.open(QIODevice::ReadOnly)) {
            size = f.size();
            f.close();
        }
        else
            size = -1;

        result = QString::number(size);

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

        QChar tc = param[2].toInt(&ok); // converted from string-number to actual number, into a character.

        if (ok == false)
            return false;

        int p = param[1].toInt(&ok); // Which position to use

        if (ok == false)
            return false;

        result = token(param[0], p, tc);

        return true;
    }


    if (fn == "TOKENPOS") {
        if (param.count() != 3)
            return false;

        // $tokenpos(text, pattern, token)
        // returns what token position given pattern is in a given text
        bool ok = false;
        QChar tc = param[2].toInt(&ok); // converted from string-number to actual number, into a character.
        if (ok == false)
            return false;
        QStringList tokens = param[0].split(tc);
        result = QString::number(tokens.indexOf(param[1]) + 1);
        return true;
    }

    if (fn == "VERSION") {
        // IIRC version.
        result = VERSION_STRING;
        return true;
    }

    if (fn == "WIDTH") {
        /*

          $width - Returns width of IIRC window

          $width(@window) - Returns width of (text|paint) widget of @window.
                            Works for any windows.

          $width(#channel, l) - Returns width of listbox (l for Larry)

*/
        if (param.count() < 1) {
            result.clear();
            return false;
        }
        subwindow_t sw = getCustomWindow(param[0]);
        if (sw.type == WT_NOTHING)
            return false;

        if (param.count() > 1)
            result = QString::number( sw.widget->listboxWidth() );
        else
            result = QString::number( sw.widget->width() );

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

subwindow_t TScriptInternalFunctions::getCustomWindow(int wid)
{
    subwindow_t def;
    def.type = WT_NOTHING;

    return winList->value(wid, def);
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
