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

#ifndef CONFIG_H
#define CONFIG_H

/*
  Save whatever configuration we want to the public members.
  When we're satisfied with whatever, call save().
  Run rehash() to reload the ini file back to class.
*/

#include <QObject>
#include <QImage>
#include <QRect>
#include <QColor>
#include "qinifile.h"

class config : public QObject
{
  Q_OBJECT

  public:
    explicit config(QObject *parent = 0);
    ~config();
    void rehash();
    void save();
    QIniFile *ini;
    QRect mainWinGeo;
    bool maximized;
    QString nickname;
    QString altnick;
    QString realname;
    QString username;
    QString server;
    QString password;
    QString quit;
    QString logPath;
    QString bgImagePath;
    QString fontName;
    int fontSize;
    int treeWidth;
    int listboxWidth;
    int trayNotifyDelay;
    int bgZoomLevel; // -50 % to +50 %
    bool showTimestmap;
    bool showOptionsStartup;
    bool connectInvisible;
    bool showFavourites;
    bool showUsermodeMsg;
    bool trayNotify;
    bool logEnabled;
    bool logChannel;
    bool logPM;
    bool bgZoomScaled;
    bool linkUnderline;
    bool showWhois;

    // Colors
    int colDefault;
    int colLocalInfo;
    int colServerInfo;
    int colAction;
    int colCTCP;
    int colNotice;
    int colOwntextBg;
    int colOwntext;
    int colLinks;
    int colBackground;
    int colInput;
    int colInputBackground;
    int colListbox;
    int colListboxBackground;

    bool connectionActive;

    int bti(bool b); // bool to int, true=1, false=0
    bool itb(int i); // int to bool, true>=1, false<=0
    bool stb(QString s); // string to bool, '1'=true, else false

};

#endif // CONFIG_H