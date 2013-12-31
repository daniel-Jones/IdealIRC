/*
 *   IdealIRC - Internet Relay Chat client
 *   Copyright (C) 2013  Tom-Andre Barstad
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

#include <QStringList>
#include <iostream>
#include "inifile.h"

IniFile::IniFile(QString filename, QObject *parent) :
    QObject(parent)
{
  file = new QFile(filename);
  if (QFile::exists(filename) == false) {
    file->open(QIODevice::WriteOnly);
    file->close();
  }
}

/*
Functions:
QString ReadIni(QString Section, QString Item) - Returns Value
QString ReadIni(QString Section, int ItemPos) - Returns value
QString ReadIni(int SectionPos, int ItemPos) - Returns value
QString ReadIni(int SectionPos) - Returns section name
QString ReadIniItem(QString Section, int ItemPos) - Returns Item
bool DelIni(QString Section, QString Item) - Delete item from section
bool WriteIni(QString Section, QString Item, QString Value) - Returns true when its ok, false otherwise
bool SectionExists(QString section) - Returns true when it exists, false otherwise
int CountItems(QString section) - Returns number of items in a section
int CountSections() - Returns number of all sections
QString SectionName(int SectionPos) - Return name of section at position
*/

void IniFile::clearNewline(char *data)
{
  int i  = 0;
  while (true) {
    if (data[i] == '\0')
      break;

    if (data[i] == '\n') {
      data[i] = '\0';
      break;
    }
    if (data[i] == '\r') {
      data[i] = '\0';
      break;
    }
    i++;
  }
}

QString IniFile::ReadIni(QString Section, QString Item)
{
  if (! file->open(QIODevice::ReadOnly | QIODevice::Text))
    return QString();

  Section.prepend('[');
  Section.append(']');
  Item.append('=');

  char buf[1024];
  memset(buf, 0, sizeof(buf));
  bool sectFound = false;

  while (file->readLine(buf, sizeof(buf)) != -1) {
    clearNewline(buf);
    QString line(buf);

    if (line == "")
      continue;

    if (line.left(Section.length()).toUpper() == Section.toUpper()) {
      sectFound = true;
      continue;
    }
    if ((sectFound) && (line.left(Item.length()).toUpper() == Item.toUpper())) {
      file->close();
      return line.mid(Item.length());
    }
  }

  // Reaching here means data not found. Return an empty set.
  file->close();
  return "";
}

QString IniFile::ReadIni(QString Section, int ItemPos)
{
  if (! file->open(QIODevice::ReadOnly | QIODevice::Text))
    return QString();

  Section.prepend('[');
  Section.append(']');

  char buf[1024];
  memset(buf, 0, sizeof(buf));
  bool sectFound = false;
  int i = 1;

  while (file->readLine(buf, sizeof(buf)) != -1) {
    clearNewline(buf);
    QString line(buf);

    if (line == "")
      continue;

    if (line.left(Section.length()).toUpper() == Section.toUpper()) {
      sectFound = true;
      continue;
    }
    if (sectFound) {
      if (line.contains(QChar('='))) {
        if (i == ItemPos) {
          // Read out value.
          i = line.indexOf('=');
          file->close();
          return line.mid(i+1);
        }
        i++;
      }
    }

  }

  // Reaching here means data not found. Return an empty set.
  file->close();
  return "";
}

QString IniFile::ReadIni(int SectionPos)
{
    if (! file->open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();

    char buf[1024];
    memset(buf, 0, sizeof(buf));

    int i = 1;

    while (file->readLine(buf, sizeof(buf)) != -1) {
        clearNewline(buf);
        QString line(buf);

        if (line == "")
            continue;


        if (line.startsWith('[') && line.endsWith(']')) {
            if (i == SectionPos) {
                file->close();
                return line.mid(1, line.length()-2);
            }

            i++;
        }
    }

    // Reaching here means data not found. Return an empty set.
    file->close();
    return QString();
}

QString IniFile::ReadIniItem(QString Section, int ItemPos)
{
    if (! file->open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();

    Section.prepend('[');
    Section.append(']');

    char buf[1024];
    memset(buf, 0, sizeof(buf));
    bool sectFound = false;
    int i = 1;

    while (file->readLine(buf, sizeof(buf)) != -1) {
      clearNewline(buf);
      QString line(buf);

      if (line == "")
          continue;

      if (line.left(Section.length()).toUpper() == Section.toUpper()) {
        sectFound = true;
        continue;
      }
      if (sectFound) {
        if (line.contains(QChar('='))) {
          if (i == ItemPos) {
            // Read out item.
            i = line.indexOf('=');
            file->close();
            return line.mid(0,i);
          }
          i++;
        }
      }

    }

    // Reaching here means data not found. Return an empty set.
    file->close();
    return "";
}

bool IniFile::WriteIni(QString Section, QString Item, QString Value)
{
  Section.prepend('[');
  Section.append(']');
  Item.append('=');

  QStringList sl;
  char buf[1024];
  memset(buf, 0, sizeof(buf));
  bool sectFound = false;
  bool finished = false;

  if (! file->open(QIODevice::ReadOnly | QIODevice::Text))
    return false;

  while (file->readLine(buf, sizeof(buf)) != -1) {
    clearNewline(buf);
    QString line(buf);

    if (line == "")
      continue;

    sl.append(line);

    if (finished)
      continue;

    if (line.left(Section.length()).toUpper() == Section.toUpper()) {
      sectFound = true;
      continue;
    }

    // Found existing item, overwriting.
    if ((sectFound) && (line.left(Item.length()).toUpper() == Item.toUpper())) {
      sl.removeAt(sl.count()-1); // Remove the last insertion, we get a new one here...
      sl.append(Item + Value);
      finished = true;
      continue;
    }

    if ((sectFound) && (! finished) && (line.left(1)) == "[") {
      // We have found our section, but not the item, as we reached end of the section.
      sl.removeAt(sl.count()-1); // Remove the last insertion, we get a new one here...
      sl.append(Item + Value);
      sl.append(line);
      finished = true;
      continue;
    }
  }

  if ((sectFound == true) && (finished == false)) {
    // We have found our section, but we reached EOF. Insert new item.
    sl.append(Item + Value);
  }

  if (sectFound == false) {
    // Section weren't found, we make a new one at the end, and our item there.
    sl.append(Section);
    sl.append(Item + Value);
  }



  file->close();
  if (! file->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    return false;

  for (int i = 0; i <= sl.count()-1; i++) {
    QByteArray out;
    out.append(sl.at(i));
    #ifdef Q_WS_X11
      out.append("\n");
    #else
      out.append("\r\n");
    #endif
    file->write(out);
  }
  file->close();
  return true;
}

int IniFile::CountItems(QString Section)
{
  if (! file->open(QIODevice::ReadOnly | QIODevice::Text))
    return 0;


  Section.prepend('[');
  Section.append(']');

  char buf[1024];
  memset(buf, 0, sizeof(buf));
  bool sectFound = false;
  int count = 0;

  while (file->readLine(buf, sizeof(buf)) != -1) {
    clearNewline(buf);
    QString line(buf);

    if (line == "")
      continue;

    if (line.left(Section.length()).toUpper() == Section.toUpper()) {
      sectFound = true;
      continue;
    }

    if (sectFound) {
      if (line.left(1) == "[")
        break;
      if (line.contains('='))
        count++;
    }

  }

  file->close();
  return count;
}

int IniFile::CountSections()
{
    if (! file->open(QIODevice::ReadOnly | QIODevice::Text))
      return false;

    char buf[1024];
    memset(buf, 0, sizeof(buf));
    int count = 0;

    while (file->readLine(buf, sizeof(buf)) != -1) {
        clearNewline(buf);
        QString line(buf);
        if (line == "")
            continue;

        if (line.startsWith('[') && line.endsWith(']'))
            count++;
    }

    file->close();
    return count;
}

bool IniFile::DelSection(QString Section)
{
  Section.prepend('[');
  Section.append(']');

  QStringList sl;
  char buf[1024];
  memset(buf, 0, sizeof(buf));
  bool sectFound = false;

  if (! file->open(QIODevice::ReadOnly | QIODevice::Text))
    return false;

  while (file->readLine(buf, sizeof(buf)) != -1) {
    clearNewline(buf);
    QString line(buf);

    if (line == "")
      continue;

    if (line.left(Section.length()).toUpper() == Section.toUpper()) {
      sectFound = true;
      continue;
    }

    if (sectFound) {
      if (line.left(1) == "[")
        sectFound = false;
    }

    if (! sectFound) {
      sl.push_back(line);
    }

  }

  file->close();
  if (! file->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    return false;

  for (int i = 0; i <= sl.count()-1; i++) {
    QByteArray out;
    out.append(sl.at(i));
    #ifdef Q_WS_X11
      out.append("\n");
    #else
      out.append("\r\n");
    #endif
    file->write(out);
  }
  file->close();
  return true;
}

bool IniFile::DelIni(QString Section, QString Item)
{
  Section.prepend('[');
  Section.append(']');
  Item.append('=');

  QStringList sl;
    char buf[1024];
  memset(buf, 0, sizeof(buf));
  bool sectFound = false;
  bool finished = false;

  if (! file->open(QIODevice::ReadOnly | QIODevice::Text))
    return false;

  while (file->readLine(buf, sizeof(buf)) != -1) {
    clearNewline(buf);
    QString line(buf);

    if (line == "")
      continue;

    sl.append(line);

    if (finished)
      continue;

    if (line.left(Section.length()).toUpper() == Section.toUpper()) {
      sectFound = true;
      continue;
    }

    // Found item
    if ((sectFound) && (line.left(Item.length()).toUpper() == Item.toUpper())) {
      sl.removeAt(sl.count()-1); // Remove the last insertion
      finished = true;
      continue;
    }

    if ((sectFound) && (! finished) && (line.left(1)) == "[") {
      // We have found our section, but not the item, as we reached end of the section.
      // Just close file reading and do not touch the file at all.
      file->close();
      return false; // False because we didn't do anything
    }
  }

  if ((sectFound == true) && (finished == false)) {
    // We have found our section, but we reached EOF. Don't do anything
    file->close();
    return false;
  }

  if (sectFound == false) {
    // Section weren't found, just stop.
    file->close();
    return false;
  }



  file->close();
  if (! file->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    return false;

  for (int i = 0; i <= sl.count()-1; i++) {
    QByteArray out;
    out.append(sl.at(i));
    out.append("\n");
    file->write(out);
  }
  file->close();
  return true;
}

bool IniFile::SectionExists(QString section)
{
    QString s = QString("[%1]")
                .arg(section);

    if (! file->open(QIODevice::ReadOnly | QIODevice::Text))
      return false;

    char d[64];
    while (file->readLine(d, 64) > -1)
        if (s.toUpper() == QString(d).toUpper())
            return true;

    file->close();
    return false;
}

bool IniFile::AppendSection(QString Section)
{
    if (! file->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
      return false;

    if (SectionExists(Section))
        return false;

    QString out = QString("[%1]\n")
                   .arg(Section);
    file->write(out.toLocal8Bit());
    return true;
}
