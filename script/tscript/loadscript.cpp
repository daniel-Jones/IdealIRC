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

e_scriptresult TScript::loadScript2(QString includeFile, QString parent)
{
    QString fn = filename;
    if (includeFile.isEmpty()) {
        qDebug() << "Running loadScript() on" << filename;
        scriptstr.clear();
        errorKeyword.clear();
        curLine = 1;
        loadIntLine = 1;
        include.clear();
        command.clear();
        tevent.clear();
        lineMap.clear();

        QHashIterator<QString,TCustomScriptDialog*> i(dialogs);
        while (i.hasNext()) {
            i.next();
            delete i.value();
        }
        dialogs.clear();

    }
    else {
        fn = setRelativePath(parent, includeFile);
        qDebug() << "Including" << fn;
    }

    qDebug() << "attempt:" << fn;

    if (QFile::exists(fn) == false)
    return se_FileNotExist;

    QFile f(fn);
    bool fo = f.open(QIODevice::ReadOnly | QIODevice::Text);
    if (fo == false)
        return se_FileCannotOpen;

    QString ba = f.readAll();
    if (ba.length() < 1)
        return se_FileEmpty;

    // Remove comments, keep newlines:
    QString tmp;
    bool isCommented = false;

    QString lineFile = includeFile;
    if (lineFile.isEmpty())
        lineFile = filename;

    int cLine = 1;

    for (int i = 0; i <= ba.length()-1; ++i) {
        QChar c = ba[i];

        if (c == '\n') { // newline
            delWhitespace(&tmp);

            lineMap.insert(loadIntLine, QString("%1:%2")
                                          .arg(cLine)
                                          .arg(lineFile)
                           );

            if (tmp.length() > 0) {
                scriptstr.push_back(tmp + '\n');
                loadIntLine++;
            }

            cLine++;

            isCommented = false;
            tmp.clear();
            continue;
        }

        if (isCommented) // part of a comment, ignore.
            continue;

        if (c == ';') { // comment, skip anything after.
            isCommented = true;
            continue;
        }

        tmp.append(c);
    }

    if (tmp.length() > 0) // insert anything missing from very last line.
        scriptstr.push_back(tmp + '\n');

    // This was an include, the first instance of loadScript() will take care of retreiving the rest.
    if (includeFile.length() > 0)
        return se_None;


    // Search script for functions and parse the meta.
    QString keyword;
    bool getKeyword = true; // Always begin with getting keyword.
    bool comment = false;

    enum { // Expecting
        ex_Unknown = 0, // If we use this we must pose an error too.
        ex_Block,
        ex_Brace,
        ex_Statement,
        ex_Ignore, // ignore until next block (still handles nesting levels)
        ex_Include = 5,
        ex_Command,
        ex_Event,
        ex_Timer,
        ex_ScriptName,
        ex_FunctionName = 10,
        ex_MenuType,
        ex_MenuTitle,
        ex_MenuFunction,
        ex_DialogName,
        ex_DialogTitle = 15,
        ex_DialogGeometry,
        ex_DialogIcon,
        ex_DialogLabel,
        ex_DialogButton,
        ex_DialogEditBox = 20,
        ex_DialogTextBox,
        ex_DialogListBox
    };

    enum {
        st_None = 0, // used when waiting for block name keyword
        st_Meta,
        st_Function,
        st_Menu,
        st_Dialog
    };

    int state = st_None;
    int ex = ex_Block; // We expect to find a script block to begin with. either function, script or menu.
    int n = 0; // Nesting level. 0 is where script blocks are at (script, function, menu, etc.)

    QString temp[3];
    TCustomScriptDialog *dialog = NULL;

    // Reset the custom menues, they'll be set up later on in here...
    resetMenu(customNicklistMenu);
    resetMenu(customChannelMenu);

    for (int i = 0; i <= scriptstr.length()-1; ++i) {
        QChar cc = scriptstr[i];

        errorKeyword = keyword + cc;

        if (cc == '\n')
            curLine++;

        if (ex != ex_Ignore) {
            if (cc == '\\') { // Escape, skip completely and add to keyword
                if ((i == scriptstr.length()-1) || (scriptstr[i+1] == '\n'))
                    return se_EscapeOnEndLine;

                keyword += scriptstr[i+1];
                i++;
                continue;
            }

            /* Comment handling */
            /* Put ; anywhere but must end with newline. */
            if (cc == '\n')
                comment = false;
            if (comment)
                continue;
            if (cc == ';') {
                comment = true;
                continue;
            }
            /* **************** */
        }



        if (cc == '{') {
            if ((ex != ex_Brace) && (ex != ex_Ignore) && (n == 0))
                return se_UnexpectedToken;
            n++;
        }
        if (cc == '}') {
            if (n == 0)
                return se_UnexpectedToken;

            n--;

            if (state == st_Dialog) {
                dialogs.insert( dialog->getName(), dialog );
                connect(dialog, SIGNAL(runEvent(e_iircevent,QStringList)),
                this, SLOT(runEvent(e_iircevent,QStringList)));
                state = st_None;
            }

            if (n == 0)
            ex = ex_Block;
        }

        if (ex == ex_Brace) {
            if ((cc != ' ') && (cc != '\t') && (cc != '\n') && (cc != '{') && (cc != '}'))
                return se_UnexpectedToken;

            if ((cc == ' ') || (cc == '\t') || (cc == '\n'))
                continue;

            ex = ex_Statement;
            continue;
        }

        if (cc == '}')
            continue;

        if (ex == ex_FunctionName) {
            // Stop this one at (
            // If we encounter \n first, pose error.

            if (cc == '\n')
                return se_UnexpectedNewline;
            if (i == scriptstr.length()-1)
                return se_UnexpectedFinish;

            if (cc == '(') {
                fnindex.insert(keyword.toUpper(), i);
                ex = ex_Ignore;
                continue;
            }

            if (cc == ' ') {
                QChar cc2 = scriptstr[i+1];
                if (cc2 == '(')
                    continue;
                else
                    return se_UnexpectedToken;
            }
        }

        // Newline, receiving keyword but keyword got is none. ignore.
        if ((cc == '\n') && (getKeyword == true) && (keyword.length() == 0))
            continue;

        // Newline, space receiving keyword and got a keyword. parse.
        if (((cc == '\n') || (cc == ' ')) && (getKeyword == true) && (keyword.length() >= 0)) {
            QString keywup = keyword.toUpper();

            if (n == 0) { // Here we always expect script blocks.
                if (ex == ex_Block) {

                    if (keywup == "SCRIPT") {
                        ex = ex_ScriptName;
                        state = st_Meta;
                        keyword.clear();
                        continue;
                    }

                    else if (keywup == "META") {
                        ex = ex_Brace;
                        state = st_Meta;
                        keyword.clear();
                        continue;
                    }

                    else if (keywup == "FUNCTION") {
                        ex = ex_FunctionName;
                        state = st_Function;
                        keyword.clear();
                        continue;
                    }

                    else if (keywup == "MENU") {
                        ex = ex_MenuType;
                        state = st_Menu;
                        keyword.clear();
                        continue;
                    }

                    else if (keywup == "DIALOG") {
                        ex = ex_DialogName;
                        state = st_Dialog;
                        keyword.clear();
                        continue;
                    }

                    else
                        return se_InvalidBlockType;
                }

                if (ex == ex_ScriptName) {
                    name = keyword;
                    ex = ex_Brace;
                    keyword.clear();
                    continue;
                }

                if (ex == ex_DialogName) {
                    dialog =  new TCustomScriptDialog(keyword, dlgParent, this);
                    connect(dialog, SIGNAL(runEvent(e_iircevent,QStringList)),
                    this, SLOT(runEvent(e_iircevent,QStringList)));
                    ex = ex_Brace;
                    keyword.clear();
                    continue;
                }

                if (ex == ex_MenuType) {
                    temp[0] = keyword.toUpper();
                    state = st_Menu;
                    ex = ex_Brace;
                    continue;
                }

            }

            if (n > 0) {

                if (state == st_Meta) {

                    if (ex == ex_Statement) {
                        keyword.clear(); // this is safe here.
                        if (keywup == "INCLUDE") {
                            ex = ex_Include;
                            continue;
                        }
                        else if (keywup == "COMMAND") {
                            ex = ex_Command;
                            continue;
                        }
                        else if (keywup == "EVENT") {
                            ex = ex_Event;
                            continue;
                        }
                        else if (keywup == "TIMER") {
                            ex = ex_Timer;
                            continue;
                        }
                        else
                            return se_InvalidMetaCommand;
                    }

                    if (ex == ex_Include) {
                        if (cc == ' ') {
                            keyword += cc;
                            continue;
                        }
                        include.push_back(keyword);
                        e_scriptresult r = loadScript2(keyword, fn);
                        keyword.clear();
                        ex = ex_Statement;

                        if (r != se_None)
                            return r;

                        continue;
                    }

                    if ((ex == ex_Command) || (ex == ex_Event) || (ex == ex_Timer)) {

                        if (temp[0].length() == 0)
                            temp[0] = keyword; // command name
                        else
                            temp[1] = keyword; // function
                        keyword.clear();

                        if (((temp[0].length() == 0) || (temp[1].length() == 0)) && (cc == '\n'))
                            return se_UnexpectedNewline;

                        if (cc == '\n') {
                            // this one will run if both temp are filled.

                            if (ex == ex_Command)
                            command.insert(temp[0].toUpper(), temp[1]);

                            if (ex == ex_Event) {
                                e_iircevent evt = getEvent(temp[0]);

                                if (evt == te_noevent)
                                    return se_InvalidEvent;

                                tevent.insertMulti(evt, temp[1]);
                            }


                            if (ex == ex_Timer) {
                                if (timers.contains(temp[0].toUpper()) == true)
                                    goto loadScript__ex_TimerAdd_Cleanup;


                                TTimer *tmr = new TTimer(temp[1].toUpper(), this);
                                connect(tmr, SIGNAL(timeout(QString)),
                                this, SLOT(timerTimeout(QString)));

                                timers.insert(temp[0].toUpper(), tmr);
                            }

                            loadScript__ex_TimerAdd_Cleanup:

                            ex = ex_Statement;
                            temp[0].clear();
                            temp[1].clear();

                            keyword.clear();
                            continue;
                        }

                        keyword.clear();
                        continue;
                    }

                }

                if (state == st_Dialog) {

                    if (ex == ex_Statement) {
                        keyword.clear();

                        if (keywup == "TITLE")
                            ex = ex_DialogTitle;

                        else if (keywup == "GEOMETRY")
                            ex = ex_DialogGeometry;

                        else if (keywup == "LABEL")
                            ex = ex_DialogLabel;

                        else if (keywup == "BUTTON")
                            ex = ex_DialogButton;

                        else if (keywup == "EDITBOX")
                            ex = ex_DialogEditBox;

                        else if (keywup == "TEXTBOX")
                            ex = ex_DialogTextBox;

                        else if (keywup == "LISTBOX")
                            ex = ex_DialogListBox;

                        else
                            return se_UnexpectedToken;

                        continue;
                    }

                    if (ex == ex_DialogTitle) {
                        for (; i <= scriptstr.length()-1; i++) {
                            QChar c = scriptstr[i];
                            if (c == '\n')
                                break;
                            keyword += c;
                        }
                        dialog->setTitle(keyword);
                        keyword.clear();
                        ex = ex_Statement;
                        continue;
                    }

                    if (ex == ex_DialogGeometry) {
                        int param = 2; // 1X 2Y 3W 4H
                        int X, Y, W, H;
                        X = keyword.toInt();
                        keyword.clear();
                        for (i++; i <= scriptstr.length()-1; i++) {
                            QChar c = scriptstr[i];

                            if ((c == ' ') || (c == '\n')) {
                                if (param == 2)
                                    Y = keyword.toInt();
                                if (param == 3)
                                    W = keyword.toInt();
                                if (param == 4)
                                    H = keyword.toInt();
                                keyword.clear();
                                param++;
                            }

                            if (c == '\n')
                                break;


                            if (param == 5)
                                break;

                            keyword += c;
                        }

                        if (param != 5)
                            return se_InvalidParamCount;

                        dialog->setGeometry(X, Y, W, H);
                        keyword.clear();
                        ex = ex_Statement;
                        continue;
                    }

                    if (ex >= ex_DialogLabel) {
                        int param = 2;
                        int X, Y, W, H = 0;
                        QString oname = keyword;
                        QString arg;
                        keyword.clear();
                        for (i++; i <= scriptstr.length()-1; i++) {
                            QChar c = scriptstr[i];

                            if (param < 6) {
                                if ((c == ' ') || (c == '\n')) {
                                    if (param == 2)
                                        X = keyword.toInt();
                                    if (param == 3)
                                        Y = keyword.toInt();
                                    if (param == 4)
                                        W = keyword.toInt();
                                    if (param == 5)
                                        H = keyword.toInt();
                                    keyword.clear();
                                    param++;
                                }

                            }
                            if (c == '\n')
                                break;

                            if (param >= 6)
                                arg += c;
                            else
                                keyword += c;
                        }

                        if (param != 6)
                            return se_InvalidParamCount;

                        arg = arg.mid(1);


                        if (ex == ex_DialogLabel)
                            dialog->addLabel(oname, X, Y, W, H, arg);

                        if (ex == ex_DialogButton)
                            dialog->addButton(oname, X, Y, W, H, arg);

                        if (ex == ex_DialogEditBox)
                            dialog->addEditbox(oname, X, Y, W, H);

                        if (ex == ex_DialogTextBox)
                            dialog->addTextbox(oname, X, Y, W, H);

                        if (ex == ex_DialogListBox)
                            dialog->addListbox(oname, X, Y, W, H);


                        keyword.clear();
                        ex = ex_Statement;
                        continue;
                    }

                }

                if (state == st_Menu) {
                    if (ex == ex_Statement) {
                        if (temp[0] == "NICKLIST")
                            createMenu(i, 'n');
                        if (temp[0] == "CHANNEL")
                            createMenu(i, 'c');

                        --i;
                        temp[0].clear();
                        state = st_None;
                        ex = ex_Brace;
                    }
                }
            }

            keyword.clear();

            continue;
        }

        keyword += cc;
    }

    if (n > 0)
    return se_UnexpectedFinish;

    qDebug() << "Script" << name << "(re)loaded with:";
    qDebug() << " -" << fnindex.count() << "functions.";
    qDebug() << " -" << tevent.count() << "events.";
    qDebug() << " -" << include.count() << "includes.";
    qDebug() << " -" << command.count() << "commands.";
    qDebug() << " -" << timers.count() << "timers.";
    qDebug() << "---";

    return se_None;
}
