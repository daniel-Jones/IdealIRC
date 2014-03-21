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

#include "../tscript.h"
#include "constants.h"

e_scriptresult TScript::extract(QString &text, bool extractVariables)
{
    enum {
        st_Add = 0,
        st_Variable
    };

    int state = st_Add;
    QString result;
    QString vname;

    for (int i = 0; i <= text.length()-1; ++i) {
        QChar c = text[i];
        if (c == '\\') {
            if (i == text.length()-1)
                return se_EscapeOnEndLine;
            c = text[++i];
            result += c;
            continue;
        }

        if (state == st_Add) {
            if ((c == '%') && (extractVariables)) {
                // var
                state = st_Variable;
                vname = "%";
                continue;
            }

            if (c == '$') {
                // fn
                QString r;
                e_scriptresult err = extractFunction(text, r, &i);
                if ((err != se_None) && (err != se_RunfDone))
                    return err;

                if (i < text.length()-1)
                    --i;

                result += r;
                continue;
            }

            result += c;
            continue;
        }

        if (state == st_Variable) {
            if ((c == ' ') || (c == '%') || (i == text.length()-1)) {
                // got var name

                if ((i == text.length()-1) && (c != ' ') && (c != '%'))
                    vname += c; // variable is on end of line, add last character to vname.

                if (c == ' ')
                    --i; // get space added to text.
                if (! binVars.contains(vname))
                    result += variables.value(vname);
                else
                    result += vname;
                state = st_Add;
                continue;
            }
            vname += c;
            continue;
        }
    }

    text = result;
    return se_None;
}

e_scriptresult TScript::extractFunction(QString &text, QString &result, int *pos)
{
    // Parses the very first function that occurs beginning from pos.
    // pos will update where the function ended in text.

    enum {
        st_Wait = 0,  // wait for $
        st_FnName,    // read function name
        st_FnParam    // parse parameters
    };

    QString function;
    QString param;
    QStringList paramList;

    /* text = $function
     * text = $fn(param)
     * text = $fn($calc(5+5), %var)
     * ...
     */

    bool whitespace = false;
    int state = st_Wait;
    for (; *pos <= text.length()-1; ++(*pos)) {
        int i = *pos;
        QChar c = text[i];

        if (c == '\\') {
            if (i == text.length()-1)
                return se_EscapeOnEndLine;
            if (state == st_FnName)
                function += text[++i];
            if (state == st_FnParam)
                param += text[++i];
        }

        if (state == st_Wait) {
            if (c == '$') {
                state = st_FnName;
                continue;
            }
            else
                continue;
        }

        if (state == st_FnName) {
            if ((c == ' ') || (i == text.length()-1)) {
                if (i == text.length()-1)
                    function += c;

                break;
            }

            if (c == '(') {
                whitespace = true;
                state = st_FnParam;
                continue;
            }

            if (c == ')')
                break;

            function += c;
            continue;
        }

        if (state == st_FnParam) {

            if (c == ')') {
                if ((! paramList.isEmpty()) || (! param.isEmpty())) {
                    e_scriptresult err = extract(param);
                    if ((err != se_None) && (err != se_RunfDone))
                        return err;

                    paramList << param;
                }

                ++(*pos);
                break;
            }
            if (c == '$') {
                QString r;
                e_scriptresult err = extractFunction(text, r, pos);
                if ((err != se_None) && (err != se_RunfDone))
                    return err;

                param += r;
                --(*pos);
                continue;
            }
            if (c == ',') {
                whitespace = true;
                e_scriptresult err = extract(param);
                if ((err != se_None) && (err != se_RunfDone))
                    return err;

                paramList << param;
                param.clear();
                continue;
            }

            if (((c == ' ') || c == '\t') && (whitespace))
                continue;
            else if (whitespace)
                whitespace = false;

            param += c;
        }
    }

    return runf(function, paramList, result);
}

QByteArray TScript::extractBinVars(QString &text)
{
    // Scan through the text and put data into 'temp'.
    // When encountering '%' sign, do not add that to temp, but figure the complete variable name (when we encounter a space),
    // and then insert variable data to temp.
    bool getVarName = false;
    QByteArray tmp;
    QString var; // Var name
    for (int i = 0; i <= text.length()-1; i++) {
        QChar c = text[i];

        if (getVarName == false) {
            if (c == '%') {
                getVarName = true;
                var.clear();
                var += c;
                continue;
            }

            tmp += c;
        }

        if (getVarName == true) {
            if ((c == ' ') || (i == text.length()-1)) {
                getVarName = false;

                if ((i == text.length()-1) && (c != ' '))
                    var += c;

                bool ex = binVars.contains(var);
                if (ex)
                    tmp += binVars.value(var);

                if (c == ' ')
                    tmp += ' ';
                continue;
            }

            var += c;
        }

    }

    return tmp;
}
