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

/*! \class config
 *  \brief IdealIRC configuration class. Data from iirc.ini is stored here.
 *
 * All data from iirc.ini should be stored here, as public variables.
 * Whenever data is changed, run save() to write to iirc.ini, however
 * not required as save() is run upon IdealIRC exit.
 * Running rehash() will re-read all config from iirc.ini. This shouldn't
 * be done during run-time, only on startup.
 *
 * The class definition should be pretty self-explainatory, if you compare it
 * to the contents of an iirc.ini.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <QObject>
#include <QImage>
#include <QRect>
#include <QColor>
#include "inifile.h"

/*! \enum BgImageScale
 *
 * Defines how to scale background images within IdealIRC.
 * This enum is used in member bgImageScale.
 */
enum BgImageScale {
    Bg_Scale = 1,
    Bg_ScaleAndCut,
    Bg_ScaleKeepProportions,
    Bg_Center,
    Bg_Tiled
};

class config : public QObject
{
  Q_OBJECT

public:
    explicit config(QObject *parent = 0);
    ~config();
    void rehash(); //!< Re-read iirc.ini and store the contents in the class. Shouldn't be run other times than when starting IdealIRC.
    void save(); //!< Saves the class data to iirc.ini.
    IniFile *ini;
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
    QString timestamp;
    QString fontName;
    QString charset;
    int fontSize;
    int treeWidth;
    int listboxWidth;
    int trayNotifyDelay;
    int bgZoomLevel;
    int timeout;
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
    bool checkVersion;
    bool showMotd;
    bool showToolBar;
    bool autoReJoin;
    bool showTreeView;
    bool showButtonbar;
    bool showMenubar;

    // Background image
    bool bgImageEnabled;
    QString bgImagePath;
    int bgImageScale;
    int bgImageOpacity;

    // Colors
    QString colDefault;
    QString colLocalInfo;
    QString colServerInfo;
    QString colAction;
    QString colCTCP;
    QString colNotice;
    QString colOwntextBg;
    QString colOwntext;
    QString colHighlight;
    QString colLinks;
    QString colBackground;
    QString colInput;
    QString colInputBackground;
    QString colListbox;
    QString colListboxBackground;
    QString colWindowlist;
    QString colWindowlistBackground;

    // Editor colors
    QColor editorLineHighlight;
    QColor editorFunctionDef;
    QColor editorMetaKeyword;
    QColor editorKeyword;
    QColor editorWindow;
    QColor editorVariable;
    QColor editorComment;
    QColor editorBackground;
    QColor editorText;
    QColor editorLineNumBackground;
    QColor editorLineNumText;

    // Editor font
    QString editorFontName;
    int editorFontSize;

    bool connectionActive;

    int bti(bool b); ///!< bool to int, true=1, false=0
    bool itb(int i); //!< int to bool, true>=1, false<=0
    bool stb(QString s); //!< string to bool, '1'=true, else false
    void copyDir(QString path, QString target); //!< Recursively copies path to target. Used when copying from SKEL_PATH to CONF_PATH.
};

#endif // CONFIG_H
