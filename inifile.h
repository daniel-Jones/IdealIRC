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

#ifndef INIFILE_H
#define INIFILE_H

#include <QObject>
#include <QFile>

/**
To-do

[Section]
Item=Value
;comment

Comments placed at the end of an Item=Value will be treated as a part of the value.
Comments elsewhere is ignored.

Load filename into a private QFile

Functions:
QString ReadIni(QString Section, QString Item) - Returns Value
QString ReadIni(QString Section, int ItemPos) - Returns value
QString ReadIni(int SectionPos, int ItemPos) - Returns value
QString ReadIni(int SectionPos) - Returns section name
QString ReadIniItem(QString Section, int ItemPos)
bool DelIni(QString Section, QString Item) - Delete item from section
bool WriteIni(QString Section, QString Item, QString Value) - Returns true when its ok, false otherwise
bool SectionExists(QString Section) - Returns true when it exists, false otherwise
int CountItems(QString section) - Returns number of items in a section
int CountSections() - Returns number of all sections
bool AppendSection(QString Section) - Create new section, return false if it exsist
QString SectionName(int SectionPos) - Return name of section at position

Upon destroy, clear pointer data.
**/

class IniFile : public QObject
{
  Q_OBJECT

public:
    explicit IniFile(QString filename, QObject *parent = 0);
    ~IniFile() { file->deleteLater(); }
    QString ReadIni(QString Section, QString Item);
    QString ReadIni(QString Section, int ItemPos);
    QString ReadIni(int SectionPos);
    QString ReadIniItem(QString Section, int ItemPos);
    bool WriteIni(QString Section, QString Item, QString Value);
    int CountItems(QString section);
    int CountSections();
    bool DelSection(QString Section);
    bool DelIni(QString Section, QString Item);
    bool SectionExists(QString section);
    bool AppendSection(QString Section);
    bool RenameSection(QString OldName, QString NewName);

private:
    void clearNewline(char *data);
    QFile *file;

};

#endif // INIFILE_H
