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
 * \param function Function name
 * \param param Parameter list
 * \param result Reference to a string for storing 'return' data
 * \param ignoreParamCount if true, ignores the parameter count (_can_ be unsafe)
 *
 * Runs a function.\n
 * This function is safe to call directly.\n\n
 *
 * It will locate the position of the function, prepare the parameters as local variables and
 * call the helper function _runf_private2() for actual parsing of the function.
 *
 * \return Script result
 */
e_scriptresult TScript::runf(QString function, QStringList param, QString &result, bool ignoreParamCount)
{
    #ifdef IIRC_DEBUG_SCRIPT
    qDebug() << "runf(" << function << ", " << param << ", [&result], " << ignoreParamCount << "):";
    #endif

    /* Here we attempt to run a function. It does not have to exist.
    * This is where we initiate the main script interpreter.
    * If the function does not exist it'll produce a script error.
    */

    if (function.isEmpty())
        return se_FunctionEmpty; // Avoid crashes if function name is none.

    if (function == "$")
        return se_InvalidFunction;

    if (function[0] == '$')
        function = function.mid(1); // Prevent any dollar signs (function identifier) at beginning

    // See if it's an internal function, if it is, run it, get true result and return true (sucess). Ignore parsing if the function is internal.
    // This means, nobody can override the internal function names. A list of these should be given in TIRC documentation.
    if (ifn.runFunction(function, param, result) == true)
        return se_RunfDone;

    int pos = fnindex.value(function.toUpper(), -1); // Get internal line number where function resides, for faster executing on large scripts.

    QHash<QString,QString> localVar;
    QHash<QString,QByteArray> localBinVar;

    // Retreive parameters list from script, and insert our params from here to there...
    QString par;

    if (pos < 0) {
        error(tr("Error in attempting to call an undefined function %1").arg(function));
        return se_InvalidFunction;
    }

    bool getParams = false;
    for (int i = pos; i <= scriptstr.length()-1; i++) {
        QChar c = scriptstr[i];

        if (getParams == true) {
            if (c == ')')
                break;
            if (c == '\n')
                return se_UnexpectedNewline;
            if (c == '(')
                return se_UnexpectedToken;
            if ((c == ' ') || (c == '\t'))
                continue;

            par += c;
        }

        if (getParams == false) {
            if (c == '(')
                getParams = true;
            if (c == '\n')
                return se_UnexpectedNewline;
        }
    }

    QStringList scpar = par.split(','); // parameter names
    if (par == "...") {
        scpar.clear();
        QString all;
        for (int i = 0; i <= param.count()-1; i++) {
            scpar << QString("%%1").arg(i+1);
            if (all.count() > 0)
                all += ' ';
            all += param[i];
        }

        scpar << "%0";
        param << all;
    }

    if (scpar.count() == 1)
        if (scpar[0].length() == 0)
            scpar.clear();

    if (scpar.count() == 0)
        param.clear(); // no need for this if we cannot bind the data to ANY parameters (they're non-existant)

    if (scpar.count() > 0) {
        if (ignoreParamCount == false) {
            // Make sure that parameter count is _equal_
            if (param.count() != scpar.count())
                return se_InvalidParamCount;

            // From here, parameter names are found in 'scpar', while the data is in 'param'.
        }
        if (ignoreParamCount == true) {
            // Fill out parameters until we got no more data; and the rest will be put at last variable.

            // Got more variables to fill than actual tokenized data
            if (scpar.count() > param.count())
            for (int i = param.count(); i <= scpar.count()-1; i++)
                param.push_back("");

            // Got more tokenized data than variable parameters. The overflow of input data goes to last variable in list.
            if (scpar.count() < param.count()) {
                /*

                example to run:
                prototype: test(%somevar, %temp, %i, %x, %text)
                params: test lol 5 7 some text here

                varname     |      data
                ----------------------------------
                somevar          | test
                temp             | lol
                i                | 5
                x                | 7
                text             | some text here


                scpar = param/var names
                param = param/var data
                */

                QString endparam = param.last();
                QString end;

                while (scpar.count() != param.count()) {
                    endparam.append(' ');
                    end.prepend(endparam);
                    param.pop_back();
                    endparam = param.last();
                }
                int idx = param.count()-1;
                QString lastparam = param[idx];
                lastparam.append(' ');
                lastparam.append(end);
                param.replace(idx, lastparam);
            }
        }
    }

    for (int i = 0; i <= scpar.count()-1; ++i)
        localVar.insert(scpar[i], param[i]);

    e_scriptresult res = _runf_private2(pos, localVar, localBinVar, result);

    /* Cleaning up this way is dangerous. don't.
    for (int i = 0; i <= scpar.count()-1; ++i)
        variables.remove(scpar[i]);
        */

    return res;
}

/*!
 * \param pos Position in script (after the opening bracket)
 * \param localVar Local variables
 * \param localBinVar Local binary variables
 * \param result Reference to a string for storing 'return' data
 *
 * \note This is a helper function for runf() and should never be run elsewhere.
 *
 * This is the actual function parser.
 * \return Script result
 */
e_scriptresult TScript::_runf_private2(int pos, QHash<QString,QString> &localVar,
                                       QHash<QString,QByteArray> &localBinVar, QString &result)
{
    #ifdef IIRC_DEBUG_SCRIPT
    qDebug() << "TScript::_runf_private2(" << pos << "," << *parName <<  ", [&result]);";
    #endif

    /* Here we run functions we _know_ exist. Do not run this one directly.
    * This one should go from runf(); function unless you know what you're doing!
    *
    * Following comes a 2000 line script interpreter:
    */

    // Typically, "keyword" would be the first word in every code it executes.
    QString keyword;
    bool getKeyword = true; // Always begin with getting keyword.
    bool comment = false;

    enum {
        ex_Param = 0,
        ex_BraceOpen,
        ex_BraceClose,
        ex_Literal
    };

    enum {
        st_Param = 0,
        st_Literal,
        st_IgnoreNest
    };

    int ex = ex_BraceOpen; // Begin with expecting parameters.
    int state = ex_Param;

    int nl = 0; // Nesting level.
    int nl_ignore = 0; // If an IF statement went false, set this to the nesting level we will return executing.

    curLine = 1; // Should be the current line, but will be wrong with included files.
    for (int i = 0; i <= pos-1; i++)
        if (scriptstr[i] == '\n')
            curLine++;

    // "while" level and positions:
    QVector<int> wnl; // while nest level.
    QVector<int> wbp; // contains byte positions of where every while that surrounds the outer nests.

    bool setLocalVar = false; // sets to true when 'local' in f.ex. 'local var %test 123' is found.

    // We loop through the script byte by byte.
    for (int i = pos; i <= scriptstr.length()-1; i++) {
        QChar cc = scriptstr[i];

        if (cc == '\n')
            curLine++;

        if (cc == '\\') { // Escape, skip completely and add to keyword
            ++i;
            e_scriptresult er = escape(scriptstr, &i, &keyword);
            if (er != se_None)
                return er;

            continue;
        }

        /* Comment handling */
        /* Put ; anywhere but must end with newline. */
        /* You may escape comments with \; */
        if (cc == '\n')
            comment = false;
        if (comment)
            continue;
        if (cc == ';') {
            comment = true;
            continue;
        }
        /* **************** */

        if (state == st_Param) {
            // Skip the parameter definition, it's already been sorted.
            if (cc == '{') {
                keyword.clear();
                state = st_Literal;
                ex = ex_Literal;
                nl++;
                continue;
            }

            continue;
        }

        if (state == st_IgnoreNest) {
            if (cc == '{')
                nl++;
            if (cc == '}')
                nl--;

            if ((cc == '}') && (nl == nl_ignore)) {
                state = st_Literal;
                ex = ex_Literal;
                keyword.clear();
            }
            continue;
        }

        if ((state == st_Literal) && (ex == ex_BraceOpen)) {
            if ((cc == ' ') || (cc == '\t') || (cc == '\n'))
                continue;

            else if (cc == '{') {
                ex = ex_Literal;
                nl++;
                keyword.clear();
                continue;
            }
            else
                return se_UnexpectedToken;
        }

        // Newline, receiving keyword but keyword got is none. ignore.
        if (((cc == '\n') || (cc == ' ')) && (getKeyword == true) && (keyword.length() == 0))
            continue;

        // Newline, space receiving keyword and got a keyword. parse.
        if (((cc == '\n') || (cc == ' ')) && (getKeyword == true) && (keyword.length() >= 0)) {

            if (keyword == "}") {
                nl--;

                if (wnl.size() > 0) {
                    // We're possibly exiting a while loop. check.
                    int l = wnl.back();

                    if (nl == l) { // re-check for loop validity.
                        i = wbp.back();
                        wnl.pop_back();
                        wbp.pop_back();
                        keyword.clear();
                        ex = ex_Literal;
                        state = st_Literal;
                        continue;
                    }

                }

                if (nl == 0)
                    return se_RunfDone;
                keyword.clear();
                continue;
            }

            // Run function
            if (keyword[0] == '$') {
                for (; i <= scriptstr.length()-1; i++) {
                    if (scriptstr[i] == '\n')
                        break;
                    keyword += scriptstr[i];
                }

                e_scriptresult err = extract(keyword, localVar, localBinVar);
                if ((err != se_None) && (err != se_RunfDone))
                    return err;

                keyword.clear();
                continue;
            }



            /***********************************************
             ** Parser keywords IF, WHILE, VAR, CON, etc. **
             ***********************************************/

            /* Notes on adding new keywords:
            *
            * Always clear the keyword after parsing
            * Always run 'continue' when finished (or else it'll be parsed as a /command)
            */

            QString keywup = keyword.toUpper();


            /*##################*
             *# RETURN KEYWORD #*
             *##################*/

            if (keywup == "RETURN") {
                QString rs; // Whatever's being returned. Not binary safe yet... oops.

                bool wait = true; // Skip whitespace between RETURN ... start
                for (; i <= scriptstr.length()-1; i++) {
                    QChar c = scriptstr[i];
                    if (wait == true) {
                        if ((c == ' ') || (c == '\t'))
                            continue;
                        wait = false;
                    }
                    if (wait == false) {
                        if (c == '\n')
                            break;
                        rs += c;
                    }
                }

                e_scriptresult err = extract(rs, localVar, localBinVar);
                if ((err != se_None) && (err != se_RunfDone))
                    return err;

                result.clear();
                result.append(rs);

                if (rs == "0")
                    emit stopURLDisplay();

                return se_RunfDone;
            }


            /*######################*
             *# CONTROL STATEMENTS #*
             *######################*/

            if (keywup == "IF") {
                // This one can do some cleaning up or rewrite... Remember to fix "while" too.

                int pn = 0; // Paranthesis nest
                bool waitPB = true; // Wait for paranthesis at beginning
                QString logic = "(";
                for (; i <= scriptstr.length()-1; i++) {
                    cc = scriptstr[i];
                    if (waitPB) {
                        if ((cc == ' ') || (cc == '\t'))
                            continue;
                        else if (cc == '(') {
                            pn++;
                            waitPB = false;
                            continue;
                        }
                        else
                            return se_UnexpectedToken;
                    }

                    logic += cc;

                    if (cc == '(')
                        pn++;
                    if (cc == ')')
                        pn--;
                    if (pn == 0)
                        break;

                }

                if (pn != 0)
                    return se_UnexpectedToken;

                bool ok = solveLogic(logic, localVar, localBinVar);
                lastIFresult = ok;

                if (ok == false) {
                    state = st_IgnoreNest;
                    ex = ex_Literal;
                    nl_ignore = nl;
                }
                else {
                    state = st_Literal;
                    ex = ex_BraceOpen;
                }

                keyword.clear();
                continue;
            }

            if (keywup == "ELSE") {
                // BUG: Any IF statements above this (unrelated to nesting level) will cause
                //      this else to run if the latest one were FALSE.

                if (lastIFresult == false) {
                    state = st_Literal;
                    ex = ex_BraceOpen;
                }
                if (lastIFresult == true) {
                    state = st_IgnoreNest;
                    ex = ex_Literal;
                    nl_ignore = nl;
                }

                keyword.clear();
                continue;
            }

            if (keywup == "WHILE") {
                wbp.push_back(i-6);
                wnl.push_back(nl);

                int pn = 0; // Paranthesis nest
                bool waitPB = true; // Wait for paranthesis at beginning
                QString logic = "(";
                for (; i <= scriptstr.length()-1; i++) {
                    cc = scriptstr[i];
                    if (waitPB) {
                        if ((cc == ' ') || (cc == '\t'))
                            continue;
                        else if (cc == '(') {
                            pn++;
                            waitPB = false;
                            continue;
                        }
                        else
                            return se_UnexpectedToken;
                    }

                    logic += cc;

                    if (cc == '(')
                        pn++;
                    if (cc == ')')
                        pn--;
                    if (pn == 0)
                        break;

                }

                if (pn != 0)
                    return se_UnexpectedToken;

                bool ok = solveLogic(logic, localVar, localBinVar);

                if (ok == false) {
                    wbp.pop_back();
                    wnl.pop_back();
                    state = st_IgnoreNest;
                    ex = ex_Literal;
                    nl_ignore = nl;
                }
                else {
                    state = st_Literal;
                    ex = ex_BraceOpen;
                }

                keyword.clear();
                continue;
            }

            if (keywup == "BREAK") {
                if (wnl.size() == 0)
                    return se_BreakNoWhile;

                // Break current loop
                nl_ignore = wnl.back();
                wbp.pop_back();
                wnl.pop_back();
                state = st_IgnoreNest;
                ex = ex_Literal;

                keyword.clear();
                continue;
            }

            if (keywup == "CONTINUE") {
                if (wnl.size() == 0)
                    return se_ContinueNoWhile;
                i = wbp.back();
                nl = wnl.back();
                wnl.pop_back();
                wbp.pop_back();
                ex = ex_Literal;
                state = st_Literal;

                keyword.clear();
                continue;
            }


            /*######################*
             *# VARIABLE SPECIFICS #*
             *######################*/

            if (keywup == "LOCAL") {
                // This can seem a little hacky, but it works.
                // local var %x
                setLocalVar = true;
                i += 5;
                keywup = "VAR";
            }

            if (keywup == "VAR") {
                // var %variable data

                enum {
                    st_VarPrefix = 0,
                    st_VarName,
                    st_Data
                };

                int st = st_VarPrefix;
                QString vname = "%";
                QString data;

                for (; i <= scriptstr.length()-1; i++) {
                    QChar c = scriptstr[i];

                    if (st == st_VarPrefix) {
                        if ((c == ' ') || (c == '\t'))
                            continue;
                        else if (c == '%')
                            st = st_VarName;
                        else
                            return se_UnexpectedToken;

                        continue;
                    }

                    if (st == st_VarName) {
                        if (c == '\n')
                            return se_UnexpectedNewline;
                        else if (c == ' ')
                            st = st_Data;
                        else
                            vname += c;

                        continue;
                    }

                    if (st == st_Data) {
                        if (c == '\n')
                            break; // Got vname and data ready.

                        data += c;

                        continue;
                    }

                }

                if (st != st_Data) // In case something very weird happens, this prevents IIRC to go bananas.
                    return se_UnexpectedNewline;

                e_scriptresult err = extract(data, localVar, localBinVar);
                if ((err != se_None) && (err != se_RunfDone))
                    return err;

                if (vname.contains('+'))
                    vname = mergeVarName(vname, localVar, localBinVar);

                if ((setLocalVar) || (localVar.contains(vname)))
                    localVar.insert(vname, data);
                else
                    variables.insert(vname, data);

                setLocalVar = false;

                keyword.clear();
                continue;
            }

            if (keywup == "DEL") {
                // del %var %var2 %var3 ...

                enum {
                    st_VarPrefix = 0,
                    st_VarName
                };

                int st = st_VarPrefix;
                QString vname = "%";

                for (; i <= scriptstr.length()-1; i++) {
                    QChar c = scriptstr[i];

                    if (st == st_VarPrefix) {
                        if ((c == ' ') || (c == '\t'))
                            continue;
                        else if (c == '%')
                            st = st_VarName;
                        else
                            return se_UnexpectedToken;

                        continue;
                    }

                    if (st == st_VarName) {
                        if (c == '\n') {
                            if (vname.contains('+'))
                                vname = mergeVarName(vname, localVar, localBinVar);

                            binVars.remove(vname);
                            variables.remove(vname);
                            break; // no need to clear vname
                        }
                        else if ((c == ' ') || (c == '\t')) {
                            if (vname.contains('+'))
                                vname = mergeVarName(vname, localVar, localBinVar);

                            binVars.remove(vname);
                            variables.remove(vname);
                            localVar.remove(vname);
                            localBinVar.remove(vname);
                            vname.clear();
                            st = st_VarPrefix;
                            continue;
                        }
                        else
                            vname += c;

                        continue;
                    }
                }

                keyword.clear();
                continue;
            }

            if ((keywup == "INC") || (keywup == "DEC")) {
                // inc %var [val]
                // dev %var [val]

                enum {
                    st_VarPrefix = 0,
                    st_VarName,
                    st_Value
                };

                int st = st_VarPrefix;
                QString vname = "%";
                QString value;

                for (; i <= scriptstr.length()-1; i++) {
                    QChar c = scriptstr[i];

                    if (st == st_VarPrefix) {
                        if ((c == ' ') || (c == '\t'))
                            continue;
                        else if (c == '%')
                            st = st_VarName;
                        else
                            return se_UnexpectedToken;

                        continue;
                    }

                    if (st == st_VarName) {
                        if (c == '\n')
                            break; // This makes us increment/decrement by one
                        else if (c == ' ')
                            st = st_Value;
                        else
                            vname += c;

                        continue;
                    }

                    if (st == st_Value) {
                        if (c == '\n')
                        break; // Got vname and value ready.

                        value += c;

                        continue;
                    }

                }

                if (st == st_VarPrefix) // In case something very weird happens, this prevents IIRC to go bananas.
                    return  se_UnexpectedNewline;

                double val = 1.0f;
                if (value.length() > 0) {
                    e_scriptresult err = extract(value, localVar, localBinVar);
                    if ((err != se_None) && (err != se_RunfDone))
                        return err;

                    bool ok = false;
                    val = value.toDouble(&ok);
                    if (!ok)
                        val = 1.0f;
                }

                // Increment or decrement value is in 'val'

                bool ok = false;
                double varval = 0.0f;
                if (localVar.contains(vname))
                    varval = localVar.value(vname).toDouble(&ok);
                else
                    varval = variables.value(vname).toDouble(&ok);

                if (!ok)
                    varval = 0.0f;

                if (keywup == "INC")
                    varval += val;
                if (keywup == "DEC")
                    varval -= val;

                if (vname.contains('+'))
                    vname = mergeVarName(vname, localVar, localBinVar);

                if (localVar.contains(vname))
                    localVar.insert(vname, QString::number(varval));
                else
                    variables.insert(vname, QString::number(varval));

                keyword.clear();
                continue;
            }


            /*##############*
             *# CONTAINERS #*
             *##############*/

            // !! DEPRECATED - LOCAL VARS ARENT FIXED HERE !!

            if (keywup == "WCON") {
                // wcon id data

                enum {
                    st_Wait = 0,
                    st_GetID,
                    st_GetData
                };

                QString id;
                QString data;

                int st = st_Wait;
                for (; i <= scriptstr.length()-1; i++) {
                    QChar c = scriptstr[i];

                    if (st == st_Wait) {
                        if (c == ' ')
                            continue;
                        else
                            st = st_GetID;
                    }

                    if (st == st_GetID) {
                        if (c == '\n')
                            return se_UnexpectedNewline;
                        if (c == ' ') {
                            st = st_GetData;
                            continue;
                        }
                        id += c;
                        continue;
                    }

                    if (st == st_GetData) {
                        if (c == '\n')
                            break;

                        data += c;
                    }
                }

                e_scriptresult err = extract(id, localVar, localBinVar);
                if ((err != se_None) && (err != se_RunfDone))
                    return err;

                err = extract(data, localVar, localBinVar);
                if ((err != se_None) && (err != se_RunfDone))
                    return err;

                writeCon(id, data);

                keyword.clear();
                continue;
            }

            if (keywup == "DCON") {
                // dcon id

                QString id;

                for (; i <= scriptstr.length()-1; i++) {
                    QChar c = scriptstr[i];

                    if (c == '\n')
                        break;

                    if (c == ' ')
                        continue;

                    id += c;
                }

                e_scriptresult err = extract(id, localVar, localBinVar);
                if ((err != se_None) && (err != se_RunfDone))
                    return err;

                delCon(id);

                keyword.clear();
                continue;
            }

            if (keywup == "SCON") {
                // scon %namevar idx pattern
                enum {
                    st_Wait,
                    st_VarPrefix = 0,
                    st_VarName,
                    st_Index,
                    st_Pattern
                };

                QString var;
                QString idx;
                QString pattern = "*";
                int st = st_VarPrefix;
                for (; i <= scriptstr.length()-1; i++) {
                    QChar c = scriptstr[i];

                    if (st == st_Wait) {
                        if (c == ' ')
                            continue;
                        else
                            st = st_VarPrefix;
                    }

                    if (st == st_VarPrefix) {
                        if (c == '\n')
                            return se_UnexpectedNewline;
                        if ((c == ' ') || (c == '\t'))
                            continue;
                        if (c == '%')
                            st = st_VarName;
                    }

                    if (st == st_VarName) {
                        if (c == '\n')
                            return se_UnexpectedNewline;
                        if (c == ' ') {
                            st = st_Index;
                            continue;
                        }

                        var += c;
                    }

                    if (st == st_Index) {
                        if (c == '\n')
                            return se_UnexpectedNewline;
                        if (c == ' ') {
                            st = st_Pattern;
                            continue;
                        }

                        idx += c;
                    }

                    if (st == st_Pattern) {
                        if (c == '\n')
                            break;
                        if (c == ' ') // Spaces not allowed in the pattern.
                            return se_UnexpectedToken;

                        pattern += c;
                    }
                }

                e_scriptresult err = extract(idx, localVar, localBinVar);
                if ((err != se_None) && (err != se_RunfDone))
                    return err;

                err = extract(pattern, localVar, localBinVar);
                if ((err != se_None) && (err != se_RunfDone))
                    return err;

                int index = idx.toInt();
                if (index < 0)
                    return se_NegativeNotAllowed;

                int count = 1;
                QString data;
                QHashIterator<QString,QString> i(container);
                while (i.hasNext()) {
                    i.next();

                    QString iname = i.key();
                    QRegExp rx( pattern );
                    rx.setPatternSyntax(QRegExp::WildcardUnix);
                    int rxidx = iname.indexOf(rx);

                    if ((index == 0) && (rxidx > -1)) {
                        count++;
                        continue;
                    }

                    if ((index > 0) && (rxidx > -1) && (count == index)) {
                        data = iname;
                        break;
                    }
                    if ((index > 0) && (rxidx > -1) && (count < index)) {
                        count++;
                        continue;
                    }

                } // while (i.hasNext())


                if (index == 0)
                    data = QString::number( count-1 );

                variables.insert(var, data);

                keyword.clear();
                continue;
            }

            if (keywup == "CON") {
                // con id %var

                enum {
                    st_Wait = 0,
                    st_Id,
                    st_VarPrefix,
                    st_VarName
                };

                QString id;
                QString vname;

                int st = st_Wait;
                for (; i <= scriptstr.count()-1; i++) {
                    QChar c = scriptstr[i];

                    if (st == st_Wait) {
                        if (c == ' ')
                            continue;
                        else
                            st = st_Id;
                    }

                    if (st == st_Id) {
                        if (c == '\n')
                            return se_UnexpectedNewline;
                        if (c == ' ') {
                            st = st_VarPrefix;
                            continue;
                        }
                        id += c;
                        continue;
                    }

                    if (st == st_VarPrefix) {
                        if (c == '\n')
                            return se_UnexpectedNewline;
                        if (c == ' ')
                            continue;
                        else if (c == '%')
                            st = st_VarName;
                        else
                            return se_UnexpectedToken;
                    }

                    if (st == st_VarName) {
                        if (c == '\n')
                            break;
                        if (c == ' ')
                            return se_UnexpectedToken;
                        vname += c;
                    }

                }

                e_scriptresult err = extract(id, localVar, localBinVar);
                if ((err != se_None) && (err != se_RunfDone))
                    return err;

                variables.insert(vname, container.value(id));

                keyword.clear();
                continue;
            }


            /*##########*
             *# TIMERS #*
             *##########*/

            if (keywup == "TIMER") {
                // timer id msec

                enum {
                    st_Wait = 0,
                    st_Id,
                    st_Msec
                };

                QString id;
                QString msec;

                int st = st_Wait;
                for (; i <= scriptstr.count()-1; i++) {
                    QChar c = scriptstr[i];

                    if (st == st_Wait) {
                        if (c == ' ')
                            continue;
                        else
                            st = st_Id;
                    }

                    if (st == st_Id) {
                        if (c == '\n')
                            return se_UnexpectedToken;
                        if (c == ' ') {
                            st = st_Msec;
                            continue;
                        }
                        id += c;
                    }

                    if (st == st_Msec) {
                        if (c == '\n')
                            break;
                        if (c == ' ')
                            return se_UnexpectedToken;

                        msec += c;
                        continue;
                    }

                }

                e_scriptresult err = extract(id, localVar, localBinVar);
                if ((err != se_None) && (err != se_RunfDone))
                    return err;

                err = extract(msec, localVar, localBinVar);
                if ((err != se_None) && (err != se_RunfDone))
                    return err;

                int ms = msec.toInt();
                TTimer *tmr = timers.value(id.toUpper(), 0);
                if (tmr == 0)
                    return se_InvalidTimer;

                tmr->runTimer(ms);

                keyword.clear();
                continue;
            }

            if (keywup == "STIMER") {
                // stimer id
                QString id;
                for (; i <= scriptstr.count()-1; i++) {
                    QChar c = scriptstr[i];

                    if (c == ' ')
                        continue;

                    if (c == '\n')
                        break;

                    id += c;
                }

                e_scriptresult err = extract(id, localVar, localBinVar);
                if ((err != se_None) && (err != se_RunfDone))
                    return err;

                TTimer *tmr = timers.value(id.toUpper(), 0);
                if (tmr == 0)
                    return se_InvalidTimer;

                tmr->stopTimer();

                keyword.clear();
                continue;
            }


            /*###########*
             *# SOCKETS #*
             *###########*/

            if (keywup == "SOCK") {
                // sock switch arguments

                enum {
                    st_Wait = 0, // Wait for switch -
                    st_Switches,
                    st_Sockname,
                    st_Args
                };

                int st = st_Wait;
                QString sw;
                QString sockname;
                QString args;
                for (; i <= scriptstr.length()-1; i++) {
                    QChar c = scriptstr[i];

                    if (st == st_Wait) {
                        if (c == ' ')
                            continue;
                        else if (c == '-')
                            st = st_Switches;
                        else
                            return se_UnexpectedToken;

                        continue;
                    }

                    if (st == st_Switches) {
                        if (c == '\n')
                            return se_UnexpectedToken;
                        else if (c == ' ')
                            st = st_Sockname;
                        else
                            sw += c;

                        continue;
                    }

                    if (st == st_Sockname) {
                        if (c == '\n')
                            break; // Allow newline here, not all sock commands require args.
                        else if (c == ' ')
                            st = st_Args;
                        else
                            sockname += c;

                        continue;
                    }

                    if (st == st_Args) {
                        if (c == '\n')
                            break;

                        args += c;
                        continue;
                    }

                }

                e_scriptresult err = extract(sockname, localVar, localBinVar);
                if ((err != se_None) && (err != se_RunfDone))
                    return err;

                QString argsEx = args;
                err = extract(argsEx, localVar, localBinVar);
                if ((err != se_None) && (err != se_RunfDone))
                    return err;

                bool accept = false;  // a
                bool close = false;   // c
                bool decline = false; // d
                bool listen = false;  // l
                bool open = false;    // o
                bool read = false;    // r
                bool write = false;   // w
                bool newline = false; // n
                bool binary = false;  // b
                bool switchfail = false;

                for (int i = 0; i <=sw.length()-1; i++) {
                    QChar s = sw[i];

                    switch (s.toLatin1()) {
                        case 'a': // Accept
                            accept = true;
                            break;
                        case 'b': // Binary variable
                            binary = true;
                            break;
                        case 'c': // Close
                            close = true;
                            break;
                        case 'd': // Decline
                            decline = true;
                            break;
                        case 'l': // Listen
                            listen = true;
                            break;
                        case 'o': // Open
                            open = true;
                            break;
                        case 'r': // Read
                            read = true;
                            break;
                        case 'w': // Write
                            write = true;
                            break;
                        case 'n': // Newline
                            newline = true;
                            break;
                        case '-':
                            break;
                        default:
                            switchfail = true;
                    }
                }

                if (switchfail == true)
                    return se_InvalidSwitches;

                // TODO  this is not accurate:
                int swsum = accept + close + decline + listen + open + write; // These CANNOT be combined.
                if (swsum > 1) // Some of them are combined, stop !
                    return se_InvalidSwitches;

                if (listen) {
                    if (argsEx.length() == 0)
                        return se_InvalidParamCount;
                    sockets.socklisten(sockname, argsEx.toInt());

                    keyword.clear();
                    continue;
                }

                if (open) {
                    if (argsEx.length() == 0)
                        return se_InvalidParamCount;

                    QStringList argl = argsEx.split(" ");
                    if (argl.length() < 2)
                        return se_InvalidParamCount;

                    sockets.sockopen(sockname, argl[0], argl[1].toInt());
                    keyword.clear();
                    continue;
                }

                if (close) {
                    // Close a socket
                    sockets.sockclose(sockname);
                    keyword.clear();
                    continue;
                }

                if (write) {
                    // Write to socket
                    QByteArray data;

                    if (args.length() == 0) {
                        if (newline) {
                            data.append('\n');
                            sockets.sockwrite(sockname, &data);
                        }
                        keyword.clear();
                        continue;
                    }

                    data.append( argsEx );

                    if (binary) {
                        QString d(data); // This will have the binary variable name in QString format (not the data)
                        data = extractBinVars(d, localBinVar); // This parses the variable, reading out the binary data.
                    }

                    if (newline)
                        data.append('\n');

                    sockets.sockwrite(sockname, &data);
                    keyword.clear();
                    continue;
                }

                if (read) {
                    // Read a socket.

                    if (args.length() == 0)
                        return se_InvalidParamCount;

                    QString vname = mergeVarName(args, localVar, localBinVar);
                    QByteArray data;

                    if (newline)
                        data.append(sockets.sockreadLn(sockname));

                    else
                        data = sockets.sockread(sockname);

                    if (vname[0] != '%')
                        return se_MissingVariable; // variables must begin wtih a percent %

                    if (binary) {
                        // QHash will replace existant values.
                        if (localBinVar.contains(vname)) {
                            localBinVar.insert(vname, data);
                            localVar.remove(vname); // remove string variable of same name
                        }
                        else {
                            binVars.insert(vname, data);
                            variables.remove(vname);
                        }
                    }
                    else {
                        // String variables
                        if (localVar.contains(vname)) {
                            localVar.insert(vname, data);
                            localBinVar.remove(vname);
                        }
                        else {
                            variables.insert(vname, data);
                            binVars.remove(vname);
                        }
                    }

                    keyword.clear();
                    continue;
                }

                if (accept) {
                    if (args.length() == 0)
                        return se_InvalidParamCount;

                    QString new_sockname = argsEx;

                    bool ok = false;
                    if (sockets.hasName(args) == false)
                        ok = sockets.sockAcceptNext(sockname, new_sockname);

                    keyword.clear();
                    continue;
                }

                if (decline) {
                    bool ok = sockets.sockDeclineNext(sockname);

                    keyword.clear();
                    continue;
                }

                if (newline) {
                    // Reaching here means it wasn't combined correctly
                    emit warning("sock: Invalid use of newline switch");

                    keyword.clear();
                    continue;
                }

                if (binary) {
                    // Reaching here means it wasn't combined correctly
                    emit warning("sock: Invalid use of binary switch");

                    keyword.clear();
                    continue;
                }

                keyword.clear();
                continue;
            }


            /*###########*
             *# TOOLBAR #*
             *###########*/

            if (keywup == "TOOLBAR") {
                /* toolbar -aidf name args

                toolbar -a name tooltip
                toolbar -i name path/to/icon
                toolbar -d name
                toolbar -f name function
                */
                enum {
                    st_Wait, // Wait for switch
                    st_Switch,
                    st_Name,
                    st_Arg
                };

                bool add = false;
                bool icon = false;
                bool del = false;
                bool fnct = false;
                QString toolname;
                QString arg;

                int st = st_Wait;
                for (; i <= scriptstr.length()-1; i++) {
                    QChar c = scriptstr[i];

                    if (st == st_Wait) {
                        if (c == '\n')
                            return se_UnexpectedNewline;
                        else if (c == ' ')
                            continue;
                        else if (c == '-')
                            st = st_Switch;
                        else
                            return se_UnexpectedToken;

                        continue;
                    }

                    if (st == st_Switch) {
                        if (c == '\n')
                        return se_UnexpectedNewline;

                        if (c == ' ') {
                            st = st_Name;
                            continue;
                        }

                        if (c == '-')
                            continue;
                        else if (c == 'a')
                            add = true;
                        else if (c == 'i')
                            icon = true;
                        else if (c == 'd')
                            del = true;
                        else if (c == 'f')
                            fnct = true;
                        else
                            return se_InvalidSwitches;

                        continue;
                    }

                    if (st == st_Name) {
                        if (c == '\n') {
                            e_scriptresult err = extract(toolname, localVar, localBinVar);
                            if ((err != se_None) && (err != se_RunfDone))
                                return err;
                            break;
                        }
                        if (c == ' ') {
                            e_scriptresult err = extract(toolname, localVar, localBinVar);
                            if ((err != se_None) && (err != se_RunfDone))
                                return err;

                            st = st_Arg;
                            continue;
                        }
                        toolname += c;
                        continue;
                    }

                    if (st == st_Arg) {
                        if (c == '\n') {
                            e_scriptresult err = extract(arg, localVar, localBinVar);
                            if ((err != se_None) && (err != se_RunfDone))
                                return err;
                            break;
                        }

                        arg += c;
                        continue;
                    }

                }

                int sum = add+del+icon+fnct;
                if (sum != 1)
                    return se_InvalidSwitches;

                if (add) {
                    if (arg.length() == 0)
                        return se_InvalidParamCount;
                    emit toolbarAdd(toolname, name, arg);
                }

                if (del)
                    emit toolbarDel(toolname);

                if (icon) // empty icon path means remove icon.
                    // "filename" is script filename. Setting icon relative path to it, unless icon path is complete.
                    emit toolbarSetIcon(toolname, setRelativePath(filename, arg));


                if (fnct) {
                    if (arg.length() == 0)
                        return se_InvalidParamCount;
                    emit toolbarSetFunction(toolname, name, arg); // "name" is script name.
                }

                keyword.clear();
                continue;
            }


            /*##################*
             *# DIALOG CONTROL #*
             *##################*/

            if (keywup == "DLG") {

                enum {
                    st_Wait,
                    st_Switch,
                    st_Arg
                };

                // dlg -shclide name [obj] [args]

                bool show = false;
                bool hide = false;
                bool close = false;
                bool label = false;
                bool additem = false;
                bool reitem = false;
                bool clear = false;
                bool delitem = false;

                QString arg;
                int st = st_Wait;
                for (; i <= scriptstr.length()-1; i++) {
                    QChar c = scriptstr[i];

                    if (st == st_Wait) {
                        if (c == ' ')
                            continue;

                        else if (c == '-')
                            st = st_Switch;

                        else
                            return se_UnexpectedToken;
                    }

                    if (st == st_Switch) {
                        if (c == ' ') {
                            st = st_Arg;
                            continue;
                        }

                        if (c == '-')
                            continue;
                        else if (c == 's')
                            show = true;
                        else if (c == 'h')
                            hide = true;
                        else if (c == 'c')
                            close = true;
                        else if (c == 'l')
                            label = true;
                        else if (c == 'i')
                            additem = true;
                        else if (c == 'r')
                            reitem = true;
                        else if (c == 'd')
                            delitem = true;
                        else if (c == 'e')
                            clear = true;
                        else
                            return se_InvalidSwitches;
                    }

                    if (st == st_Arg) {
                        if (c == '\n')
                        break;
                        arg += c;
                    }

                }

                int sum = show+hide+close+label+additem+delitem+clear;
                if (sum > 1)
                    return se_InvalidSwitches;

                e_scriptresult err = extract(arg, localVar, localBinVar);
                if ((err != se_None) && (err != se_RunfDone))
                    return err;

                bool ok = false;
                if (show)
                    ok = customDialogShow(arg);

                if (hide)
                    ok = customDialogHide(arg);

                if (close)
                    ok = customDialogClose(arg);

                if (label) {
                    QStringList par = arg.split(' '); // dialog object text ...
                    if (par.count() < 3)
                        return se_UnexpectedNewline;
                    QString text = arg.mid(par[0].length() + par[1].length() + 2);
                    ok = customDialogSetLabel(par[0], par[1], text);
                }

                if (additem) {
                    QStringList par = arg.split(' '); // dialog object text ...
                    if (par.count() < 3)
                        return se_UnexpectedNewline;
                    QString text = arg.mid(par[0].length() + par[1].length() + 2);
                    ok = customDialogAddItem(par[0], par[1], text);
                }

                if (reitem) {
                    QStringList par = arg.split(' '); // dialog object index text ...
                    if (par.count() < 4)
                        return se_UnexpectedNewline;
                    QString text = arg.mid(par[0].length() + par[1].length() + par[2].length() + 3);
                    ok = customDialogReItem(par[0], par[1], par[2], text);
                }

                if (delitem) {
                    QStringList par = arg.split(' '); // dialog object index ...
                    if (par.count() < 3)
                        return se_UnexpectedNewline;
                    ok = customDialogDelItem(par[0], par[1], par[2]);
                }

                if (clear) {
                    QStringList par = arg.split(' '); // dialog object
                    if (par.count() < 2)
                        return se_UnexpectedNewline;
                    ok = customDialogClear(par[0], par[1]);
                }

                if (! ok)
                    return se_UnexpectedToken;

                keyword.clear();
                continue;
            }


            /*################*
             *# FILE CONTROL #*
             *################*/

            if (keywup == "FREAD") {
                // fread %fd length %var
                // Read all data in buffer to %var

                enum {
                    st_ignore,
                    st_fd,
                    st_length,
                    st_var
                };

                int st = st_ignore;
                int stnext = st_fd;
                int fd = 0;
                qint64 len;
                QString fds;
                QString length;
                QString var;

                for (; i <= scriptstr.length()-1; i++) {
                    QChar c = scriptstr[i];
                    if (st == st_ignore) {
                        if (c == '\n')
                            return se_UnexpectedNewline;
                        if (c == ' ')
                            continue;
                        st = stnext;
                    }

                    if (st == st_fd) {
                        if (c == ' ') {
                            st = st_ignore;
                            stnext = st_length;

                            e_scriptresult err = extract(fds, localVar, localBinVar);
                            if ((err != se_None) && (err != se_RunfDone))
                                return err;
                            continue;
                        }
                        if (c == '\n')
                            return se_UnexpectedNewline;

                        fds += c;
                        continue;
                    }

                    if (st == st_length) {
                        if (c == ' ') {
                            st = st_ignore;
                            stnext = st_var;

                            e_scriptresult err = extract(length, localVar, localBinVar);
                            if ((err != se_None) && (err != se_RunfDone))
                                return err;
                            continue;
                        }
                        if (c == '\n')
                            return se_UnexpectedNewline;

                        length += c;
                        continue;
                    }

                    if (st == st_var) {
                        if (c == ' ')
                            continue;
                        if (c == '\n')
                            break;

                        var += c;
                        continue;
                    }
                }

                fd = fds.toInt();
                len = length.toLongLong();

                if (! files.contains(fd))
                    return se_InvalidFileDescriptor;

                t_sfile ts = files.value(fd);
                QFile *f = ts.file;

                if (len == -1)
                    len = f->size();


                QByteArray data;
                if ((len == 0) && (ts.binary == false))
                    data = f->readLine();
                else
                    data = f->read(len);

                if (ts.binary) {
                    if (localBinVar.contains(var))
                        localBinVar.insert(var, data);
                    else
                        binVars.insert(var, data);
                }
                else {
                    if (localVar.contains(var))
                        localVar.insert(var, data);
                    else
                        variables.insert(var, data);
                }

                keyword.clear();
                continue;
            }

            if (keywup == "FWRITE") {
                // fwrite %fd %data

                enum {
                    st_ignore,
                    st_fd,
                    st_data
                };
                int st = st_ignore;
                int stnext = st_fd;

                QString fds, datas;
                QByteArray data;
                for (; i <= scriptstr.length()-1; i++) {
                    QChar c = scriptstr[i];

                    if (st == st_ignore) {
                        if (c == ' ')
                            continue;
                        if (c == '\n')
                            return se_UnexpectedNewline;
                        st = stnext;
                    }

                    if (st == st_fd) {
                        if (c == ' ') {
                            st = st_ignore;
                            stnext = st_data;
                            e_scriptresult err = extract(fds, localVar, localBinVar);
                            if ((err != se_None) && (err != se_RunfDone))
                                return err;

                            continue;
                        }
                        if (c == '\n')
                            return se_UnexpectedNewline;

                        fds += c;
                        continue;
                    }

                    if (st == st_data) {
                        if (c == '\n')
                            break;

                        datas += c;
                        continue;
                    }

                }

                e_scriptresult err = extract(datas, localVar, localBinVar);
                if ((err != se_None) && (err != se_RunfDone))
                    return err;

                data = extractBinVars(datas, localBinVar);

                int fd = fds.toInt();
                if (! files.contains(fd))
                    return se_InvalidFileDescriptor;

                t_sfile ts = files.value(fd);
                QFile *f = ts.file;


                f->write(data);
                f->flush();

                keyword.clear();
                continue;
            }

            if (keywup == "FSEEK") {
                enum {
                    st_ignore,
                    st_fd,
                    st_pos
                };

                int st = st_ignore;
                int stnext = st_fd;
                int fd = 0;
                qint64 pos;
                QString poss;
                QString fds;

                for (; i <= scriptstr.length()-1; i++) {
                    QChar c = scriptstr[i];
                    if (st == st_ignore) {
                        if (c == '\n')
                            return se_UnexpectedNewline;
                        if (c == ' ')
                            continue;
                        st = stnext;
                    }

                    if (st == st_fd) {
                        if (c == '\n')
                            return se_UnexpectedNewline;
                        if (c == ' ') {
                            st = st_ignore;
                            stnext = st_pos;

                            e_scriptresult err = extract(fds, localVar, localBinVar);
                            if ((err != se_None) && (err != se_RunfDone))
                                return err;

                            continue;
                        }
                        fds += c;
                        continue;
                    }

                    if (st == st_pos) {
                        if (c == '\n')
                            break;
                        if (c == ' ')
                            continue;

                        poss += c;
                        continue;
                    }
                }

                e_scriptresult err = extract(poss, localVar, localBinVar);
                if ((err != se_None) && (err != se_RunfDone))
                    return err;

                pos = poss.toLongLong();
                fd = fds.toInt();
                if (! files.contains(fd))
                    return se_InvalidFileDescriptor;

                t_sfile ts = files.value(fd);
                QFile *f = ts.file;
                bool ok = f->seek(pos);
                if (! ok)
                    return se_FSeekFailed;

                keyword.clear();
                continue;
            }

            if (keywup == "FCLOSE") {
                QString fds;
                int fd;
                for (; i <= scriptstr.length()-1; i++) {
                    QChar c = scriptstr[i];
                    if (c == '\n')
                        break;
                    if (c == ' ')
                        continue;

                    fds += c;
                }

                e_scriptresult err = extract(fds, localVar, localBinVar);
                if ((err != se_None) && (err != se_RunfDone))
                    return err;

                fd = fds.toInt();

                if (! files.contains(fd))
                    return se_InvalidFileDescriptor;

                t_sfile ts = files.value(fd);
                ts.file->close();
                delete ts.file;
                files.remove(fd);

                keyword.clear();
                continue;
            }

            /*##################################################################################*
             *##################################################################################*
             *##################################################################################*/


            // If we reach here, treat it as /command.
            for (; i <= scriptstr.length()-1; i++) {
                if (scriptstr[i] == '\n')
                    break;
                keyword += scriptstr[i];
            }

            e_scriptresult err = extract(keyword, localVar, localBinVar);
            if ((err != se_None) && (err != se_RunfDone))
                return err;

            execCmd(keyword);

            keyword.clear();
            continue;
        }

        keyword += cc;
    }

    return se_RunfDone; // If there was no errors we should reach here.
}
