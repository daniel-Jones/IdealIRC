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

/*!
 * \param text Reference to text for extraction. This text will be changed.
 *
 * Used for external calls, extracts functions and global variables.
 * \return Script result.
 */
e_scriptresult TScript::externalExtract(QString &text)
{
    // Construct empty local var for running extract()
    QHash<QString,QString> localVar;
    QHash<QString, QByteArray> localBinVar;
    return extract(text, localVar, localBinVar);
}

/*!
 * \param vname Variable name
 * \param localVar Local variables
 * \param localBinVar Local binary variables
 *
 * \brief Insert a (compound of) variable name(s).
 *
 * Consider the following:\n
 * var %i 1
 * var %test_1 hello
 * var %test_2 world
 *
 * %test_+%i will in this function be converted to %test_1 which holds the text "hello".
 * \return Merged, usable variable name
 */
QString TScript::mergeVarName(QString &vname, QHash<QString,QString> &localVar, QHash<QString, QByteArray> &localBinVar)
{
    enum {
        st_begin = 0,
        st_mergekey
    };
    QString result;
    QString keyword;
    int state = st_begin;
    for (int i = 0; i <= vname.length()-1; ++i) {
        QChar c = vname[i];
        if (c == '\\') {
            ++i;
            escape(vname, &i, &result);
            continue;
        }

        if (state == st_begin) {
            if (c == '+') {
                state = st_mergekey;
                continue;
            }

            result += c;
            continue;
        }

        if (state == st_mergekey) {
            if ((c == '+') || (i == vname.length()-1)) {
                if (i == vname.length()-1)
                    keyword += c;

                extract(keyword, localVar, localBinVar);
                result += keyword;
                keyword.clear();
                continue;
            }

            keyword += c;
            continue;
        }
    }

    return result;
}

/*!
 * \param text Reference to the script that's parsing
 * \param i Counter of where to excpect escaping
 * \param result Pointer to text where to store results of escaping
 *
 * Runs escape sequences.
 * \return Script result
 */
e_scriptresult TScript::escape(QString &text, int *i, QString *result)
{
    // i is a pointer from a loop for text.
    // It begins after the escape character '\'.
    // It's a pointer for a future feature, escaping ASCII numbers by hex (f.ex. \x0A) (resulting in a char)

    if (*i >= text.length())
        return se_EscapeOnEndLine;

    if (text[*i] == '\n')
        return se_EscapeOnEndLine;

    switch (text[*i].toLatin1()) {
        case 'c':
            result->append(char(CTRL_COLOR));
            break;

        case 'b':
            result->append(char(CTRL_BOLD));
            break;

        case 'u':
            result->append(char(CTRL_UNDERLINE));
            break;

        case 'e':
            result->append(char(CTRL_RESET));
            break;

        case 'n':
            result->append('\n');
            break;

        default:
            result->append( text[*i] );
    }
    return se_None;
}

/*!
 * \param text Reference to text to extract
 * \param localVar Local variables
 * \param localBinVar Local binary variables
 * \param extractVariables Set to true for extracting of variables
 *
 * Extracts functions (return result of functions) and variables found inside of text.\n
 * Binary variables won't be extracted, their variable names will stay untouched.
 * \return Script result
 */
e_scriptresult TScript::extract(QString &text, QHash<QString,QString> &localVar, QHash<QString,QByteArray> &localBinVar, bool extractVariables)
{
    enum {
        st_Add = 0,
        st_Variable
    };

    int state = st_Add;
    QString result;
    QString vname;
    bool MergeEOL = false;

    for (int i = 0; i <= text.length()-1; ++i) {
        QChar c = text[i];
        if (c == '\\') {
            ++i;
            e_scriptresult er = escape(text, &i, &result);
            if (er != se_None)
                return er;

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
                e_scriptresult err = extractFunction(text, r, &i, localVar, localBinVar);
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

                if (((i == text.length()-1) && (c != ' ') && (c != '%')) && (MergeEOL == false))
                    vname += c; // variable is on end of line, add last character to vname.

                if (c == ' ')
                    --i; // get space added to text.
                if ((! binVars.contains(vname)) && (! localBinVar.contains(vname))) {
                    if (localVar.contains(vname))
                        result += localVar.value(vname); // prefer local variable names
                    else
                        result += variables.value(vname);

                }
                else
                    result += vname;
                state = st_Add;
                continue;
            }

            if (c == '+') {
                // merge variable names
                QString merge;
                for (++i; i <= text.length()-1; ++i) {
                    QChar cc = text[i];
                    if (cc == ' ') {
                        break;
                        --i;
                    }
                    merge += cc;
                }
                if (i > text.length()-1) {
                    i -= 2;
                    MergeEOL = true;
                }

                e_scriptresult r = extract(merge, localVar, localBinVar);
                if (r != se_None)
                    return r;
                vname += merge;
                continue;
            }

            vname += c;
            continue;
        }
    }

    text = result;
    return se_None;
}

/*!
 * \param text Text to parse function
 * \param result Reference to a string to store the functions 'return' data
 * \param pos Pointer to position where function is found.
 * \param localVar Local variables
 * \param localBinVar Local binary variables
 *
 * \note This function is a helper function for extract() and should never be called elsewhere.
 *
 * This function parses the very first function that occurs, beginning at pos.\n
 * The 'pos' will increase as parsing goes along.\n
 * The result of the function will be stored in the 'result' variable.
 * \return Script result
 */
e_scriptresult TScript::extractFunction(QString &text, QString &result, int *pos, QHash<QString,QString> &localVar, QHash<QString,QByteArray> &localBinVar)
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
            ++(*pos);
            QString add;
            e_scriptresult er = escape(text, pos, &add);
            if (er != se_None)
                return er;

            if (state == st_FnName)
                function += add;
            if (state == st_FnParam)
                param += add;

            continue;
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
                    e_scriptresult err = extract(param, localVar, localBinVar);
                    if ((err != se_None) && (err != se_RunfDone))
                        return err;

                    paramList << param;
                }

                ++(*pos);
                break;
            }
            if (c == '$') {
                QString r;
                e_scriptresult err = extractFunction(text, r, pos, localVar, localBinVar);
                if ((err != se_None) && (err != se_RunfDone))
                    return err;

                param += r;
                --(*pos);
                continue;
            }
            if (c == ',') {
                whitespace = true;
                e_scriptresult err = extract(param, localVar, localBinVar);
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

/*!
 * \param text The text where binary variable should occur.
 * \param localBinVar Binary variables.
 *
 * Extracts binary variables from text.
 * \return Byte array of parsed text.
 */
QByteArray TScript::extractBinVars(QString &text, QHash<QString,QByteArray> &localBinVar)
{
    // Scan through the text and put data into 'temp'.
    // When encountering '%' sign, do not add that to temp, but figure the complete variable name (when we encounter a space),
    // and then insert variable data to temp.
    bool getVarName = false;
    QByteArray tmp;
    QString var; // Var name
    for (int i = 0; i <= text.length()-1; i++) {
        QChar c = text[i];

        if (c == '\\') {
            ++i;
            escape(text, &i, &var);

            continue;
        }

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

                // Prefer local variable names
                if (localBinVar.contains(var))
                    tmp += binVars.value(var);
                else if (binVars.contains(var))
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
