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

/*! \class TScriptEditorHighlighter
 *  \brief Helper class for IScriptEditorSettings to add a highlighter.
 */

#ifndef TSCRIPTEDITORHIGHLIGHTER_H
#define TSCRIPTEDITORHIGHLIGHTER_H

#include <QTextDocument>
#include <QSyntaxHighlighter>
#include <QSet>
#include <QWidget>

#include "config.h"

enum e_ScriptHLParse {
    NoneParse = 0,
    MetaParse = 1,
    ScriptParse = 2
};

class TScriptEditorHighlighter : public QSyntaxHighlighter
{
  Q_OBJECT

  public:
    explicit TScriptEditorHighlighter(QTextDocument *document, config *cfg);
    QSet<QString> getBlockKeywords() { return blockKeywords; } //!< Returns a copy of the block keywords (script, meta, function, ++)
  
  protected:
    void highlightBlock(const QString &text);
  
  private:
    QSet<QString> blockKeywords; //!< Block keywords.\n script, meta, function, ++
    QSet<QString> metaKeywords; //!< Meta keywords.\n include, event, command, timers, ++
    QSet<QString> scriptKeywords; //!< Script keywords.\n var, inc, dec ++
    int pState;
    int skipWhitespace(const QString &text);
    QColor defaultForeground; //!< From the system's window manager, default color of text
    config *conf; //!< Pointer to config class (iirc.ini)

};

#endif // TSCRIPTEDITORHIGHLIGHTER_H
