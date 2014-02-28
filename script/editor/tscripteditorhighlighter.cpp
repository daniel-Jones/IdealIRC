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

#include "tscripteditorhighlighter.h"
#include <iostream>
#include <QTextCharFormat>

TScriptEditorHighlighter::TScriptEditorHighlighter(QTextDocument *document, config *cfg) :
    QSyntaxHighlighter(document),
    conf(cfg)
{

    blockKeywords << "SCRIPT";
    blockKeywords << "META";
    blockKeywords << "FUNCTION";
    blockKeywords << "RETURN";
    blockKeywords << "DIALOG";

    metaKeywords << "INCLUDE";
    metaKeywords << "EVENT";
    metaKeywords << "COMMAND";
    metaKeywords << "TIMER";
    metaKeywords << "STIMER";

    scriptKeywords << "VAR";
    scriptKeywords << "INC";
    scriptKeywords << "DEC";
    scriptKeywords << "IF";
    scriptKeywords << "ELSE";
    scriptKeywords << "WHILE";
    scriptKeywords << "BREAK";
    scriptKeywords << "CONTINUE";
    scriptKeywords << "CON";
    scriptKeywords << "WCON";
    scriptKeywords << "SCON";
    scriptKeywords << "DCON";
    scriptKeywords << "ECHO";

    QWidget w;
    defaultForeground = w.palette().foreground().color();
}

void TScriptEditorHighlighter::highlightBlock(const QString &text)
{

    int skip = skipWhitespace(text);

    QString first = text.mid(skip).split(' ').at(0);
    first = first.toUpper();

    if (first.length() > 0) {

        QTextCharFormat cf;
        cf.setForeground(defaultForeground);

        if (blockKeywords.contains(first)) {
            cf.setFontWeight(QFont::Bold);
            if (first == "FUNCTION") {
                int start = skip + first.length() + 1;
                int end = text.mid(start).split("(").at(0).length();
                QTextCharFormat fncf;
                fncf.setFontItalic(true);
                fncf.setForeground(conf->editorFunctionDef);
                setFormat(start, end, fncf);
            }
        }

        if (metaKeywords.contains(first))
            cf.setForeground(conf->editorMetaKeyword);

        if (scriptKeywords.contains(first))
            cf.setForeground(conf->editorKeyword);

        if (first.at(0) == ';') {
            cf.setFontItalic(true);
            cf.setForeground(conf->editorComment);

            if (first.count() > 1) {

                if (first.at(1) == '@') { // red
                    cf.setBackground(Qt::red);
                    cf.setForeground(Qt::black);
                    cf.setFontWeight(QFont::Bold);
                }

                if (first.at(1) == '$') { // green
                    cf.setBackground(Qt::green);
                    cf.setForeground(Qt::black);
                    cf.setFontWeight(QFont::Bold);
                }

                if (first.at(1) == '#') { // yellow
                    cf.setBackground(Qt::yellow);
                    cf.setForeground(Qt::black);
                    cf.setFontWeight(QFont::Bold);
                }

            }

            setFormat(skip, text.length(), cf);
            return; // Stop!
        }

        else
            setFormat(skip, first.count(), cf);
    }

   enum { NoParse = 0, WinParse = 1, VarParse = 2 };

   int parse = NoParse;
   for (int i = 0; i <= text.length()-1; i++) {

       if (text.mid(i).at(0) == '%')
           parse = VarParse;

       if (text.mid(i).at(0) == '@')
           parse = WinParse;

       if (text.mid(i).at(0) == ')')
           parse = NoParse;

       if (text.mid(i).at(0) == ' ')
           parse = NoParse;


       if (parse == WinParse)
           setFormat(i, 1, conf->editorWindow);

       if (parse == VarParse)
           setFormat(i, 1, conf->editorVariable);


    }


}

int TScriptEditorHighlighter::skipWhitespace(const QString &text)
{
    int i = 0;

    for (i = 0; i <= text.length()-1; i++) {
        if (text.at(i) == 0x20) // Space
            continue;
        if (text.at(i) == 0x0B) // TAB
            continue;
        if (text.at(i) == 0x0A) // Line feed / Newline
            continue;
        if (text.at(i) == 0x0D) // Carriage return
            continue;

        break; // break at first occurence of _anything else_ than whitespaces.
    }

    // use 'i' variable to determine how much to skip
    return i;

}
