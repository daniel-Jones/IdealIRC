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

#include "constants.h"
#include "../tscript.h"

/*!
 * \param line Internal line number
 *
 * Finds the actual line number based in internal line number
 * \return String actual line number, with the filename
 */
QString TScript::lm(int line)
{
    if (! lineMap.contains(line))
        return "[Unknown line]"; // oops, this shouldn't really happen!

    return lineMap.value(line);
}

/*!
 * \param text Pointer to a text
 *
 * Removes all whitespace on the string pointed to
 */
void TScript::delWhitespace(QString *text)
{
    int i = 0;

    for (; i <= text->length()-1; i++) {
        if (text->at(i) == ' ') // Space
            continue;
        if (text->at(i) == '\t') // Tab
            continue;
        if (text->at(i) == '\n') // newline
            continue;

        break; // break at first occurence of _anything else_ than whitespaces.
    }

    // use 'i' variable to determine how much to skip
    *text = text->mid(i);
}

QString TScript::setRelativePath(QString folder, QString file)
{
    /*

    includeFile -> file
    parent -> folder

    */

    QString dir = folder;
    QStringList n = folder.split('/'); // parent script filename (maybe with path)
    QStringList fnn = file.split('/'); // include file
    bool scriptFullpath = false;

    // If fnn is more than 1 it might be a full path.
    if (fnn.length() > 1) {
        // If the include file have folders defined, run full path if that's the case.
        #ifdef Q_OS_WIN32
        if (file.mid(1,2) == ":/") {
            // include is a full path.
            dir.clear();
            scriptFullpath = true;
        }
        #else
        if (file[0] == '/') {
            // include is a full path.
            dir.clear();
            scriptFullpath = true;
        }
        #endif
    }

    // if n got multiple items, it is a path and last item is filename.
    // fnn being 1 means it is in same folder as parent script
    if ((n.length() > 1) && (scriptFullpath == false)) {
        QString last = n.last(); // filename of parent script
        dir = folder.mid(0, folder.length()-last.length()); // directory to parent script
    }

    return dir + file;
}
