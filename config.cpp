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

#include <QImage>
#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <iostream>

#include "config.h"
#include "constants.h"

config::config(QObject *parent) :
    QObject(parent),
    connectionActive(false)
{

#ifdef PACKAGED
    if (! QDir(CONF_PATH).exists())
        QDir().mkdir(CONF_PATH);
#endif

    ini = new IniFile(CONF_FILE);
}

config::~config()
{
  delete ini;
}

void config::rehash()
{
    mainWinGeo.setX(ini->ReadIni("Pos", "X").toInt());
    mainWinGeo.setY(ini->ReadIni("Pos", "Y").toInt());
    mainWinGeo.setWidth(ini->ReadIni("Pos", "W").toInt());
    mainWinGeo.setHeight(ini->ReadIni("Pos", "H").toInt());
    maximized = stb(ini->ReadIni("Pos", "Maximized"));

    if ((mainWinGeo.width() == 0) && (mainWinGeo.height() == 0)) {
        mainWinGeo.setWidth(500);
        mainWinGeo.setHeight(400);
        maximized = true;
    }

    realname = ini->ReadIni("Options", "Realname");
    nickname = ini->ReadIni("Options", "Nickname");
    altnick = ini->ReadIni("Options", "AltNick");
    username = ini->ReadIni("Options", "Username");
    server = ini->ReadIni("Options", "Server");
    server += ":" + ini->ReadIni("Options", "Port");
    password = ini->ReadIni("Options", "Password");
    quit = ini->ReadIni("Options", "Quit");
    bgImagePath = ini->ReadIni("Options", "BgImagePath");

    fontName = ini->ReadIni("Options", "FontName");
    if (fontName.length() == 0)
        fontName = "Courier New"; // Default font

    charset = ini->ReadIni("Options", "Charset");
    if (charset.length() == 0)
        charset = "UTF-8"; // Default charset

    fontSize = ini->ReadIni("Options", "FontSize").toInt();
    if (fontSize == 0)
        fontSize = 13; // Default size

    treeWidth = ini->ReadIni("Options", "TreeWidth").toInt();
    listboxWidth = ini->ReadIni("Options", "ListBoxWidth").toInt();
    if (ini->ReadIni("Options", "TrayNotifyDelay").length() == 0)
        trayNotifyDelay = 5000;
    else
        trayNotifyDelay = ini->ReadIni("Options", "TrayNotifyDelay").toInt()*1000;
    bgZoomLevel = ini->ReadIni("Options", "BgZoomLevel").toInt();

    timestamp = ini->ReadIni("Options", "TimeStamp");
    if (timestamp.length() == 0)
        timestamp = "[hh:mm]";

    showTimestmap = stb(ini->ReadIni("Options", "ShowTimeStamp"));
    showOptionsStartup = stb(ini->ReadIni("Options", "ShowOptions"));
    connectInvisible = stb(ini->ReadIni("Options", "connectInvisible"));
    showFavourites = stb(ini->ReadIni("Options", "ShowFavourites"));
    showUsermodeMsg = stb(ini->ReadIni("Options", "ShowUsermodeMsg"));

    if (ini->ReadIni("Options", "ShowWhois").length() == 0)
        showWhois = false;
    else
        showWhois = stb(ini->ReadIni("Options", "ShowWhois"));

    if (ini->ReadIni("Options", "CheckVersion").length() == 0)
        checkVersion = false;
    else
        checkVersion = stb(ini->ReadIni("Options", "CheckVersion"));

    if (ini->ReadIni("Options", "ShowMotd").length() == 0)
        showMotd = true;
    else
        showMotd = stb(ini->ReadIni("Options", "ShowMotd"));

    if (ini->ReadIni("Options", "ShowToolBar").length() == 0)
        showToolBar = true;
    else
        showToolBar = stb(ini->ReadIni("Options", "ShowMotd"));

    if (ini->ReadIni("Options", "AutoReJoin").length() == 0)
        autoReJoin = false;
    else
        autoReJoin = stb(ini->ReadIni("Options", "AutoReJoin"));


    trayNotify = stb(ini->ReadIni("Options", "TrayNotify"));

    logEnabled = stb(ini->ReadIni("Options", "Log"));
    logPath = ini->ReadIni("Options", "LogPath");
    logChannel = stb(ini->ReadIni("Options", "LogChan"));
    logPM =  stb(ini->ReadIni("Options", "LogPM"));

    bgZoomScaled = stb(ini->ReadIni("Options", "BgZoomScaled"));
    if (bgZoomScaled == 0)
        bgZoomScaled = 100; // Default value.

    if (ini->ReadIni("Options", "LinksUnderline").length() > 0)
        linkUnderline = stb(ini->ReadIni("Options", "LinksUnderline"));
    else
        linkUnderline = true; // Default value


   /*******************
    *  CUSTOM COLORS  *
    *******************/

    QString c;
    c = ini->ReadIni("Colors", "Default");
    if (c.length() == 0)
        colDefault = C_BLACK.name(); // Default value
    else
        colDefault = c;

    c = ini->ReadIni("Colors", "LocalInfo");
    if (c.length() == 0)
        colLocalInfo = C_BLUE.name(); // Default value
    else
        colLocalInfo = c;

    c = ini->ReadIni("Colors", "ServerInfo");
    if (c.length() == 0)
        colServerInfo = C_GREEN.name(); // Default value
    else
        colServerInfo = c;

    c = ini->ReadIni("Colors", "Action");
    if (c.length() == 0)
        colAction = C_MAGENTA.name(); // Default value
    else
        colAction = c;

    c = ini->ReadIni("Colors", "CTCP");
    if (c.length() == 0)
        colCTCP = C_BRIGHTRED.name(); // Default value
    else
        colCTCP = c;

    c = ini->ReadIni("Colors", "Notice");
    if (c.length() == 0)
        colNotice = C_RED.name(); // Default value
    else
        colNotice = c;

    /* not in use
    c = ini->ReadIni("Colors", "OwnTextBackground");
    if (c.length() == 0)
        colOwntextBg = -1; // Default value
    else
        colOwntextBg = c.toInt();
    */
    c = ini->ReadIni("Colors", "OwnText");
    if (c.length() == 0)
        colOwntext = C_CYAN.name(); // Default value
    else
        colOwntext = c;

    c = ini->ReadIni("Colors", "Highlight");
    if (c.length() == 0)
        colHighlight = C_BROWN.name(); // Default value
    else
        colHighlight = c;

    c = ini->ReadIni("Colors", "Links");
    if (c.length() == 0)
        colLinks = C_BRIGHTBLUE.name(); // Default value
    else
        colLinks = c;

    c = ini->ReadIni("Colors", "Background");
    if (c.length() == 0)
        colBackground = C_WHITE.name(); // Default value
    else
        colBackground = c;

    c = ini->ReadIni("Colors", "Input");
    if (c.length() == 0)
        colInput = C_BLACK.name(); // Default value
    else
        colInput = c;

    c = ini->ReadIni("Colors", "InputBackground");
    if (c.length() == 0)
        colInputBackground = C_WHITE.name(); // Default value
    else
        colInputBackground = c;

    c = ini->ReadIni("Colors", "Listbox");
    if (c.length() == 0)
        colListbox = C_BLACK.name(); // Default value
    else
        colListbox = c;

    c = ini->ReadIni("Colors", "WindowList");
    if (c.length() == 0)
        colWindowlist = C_BLACK.name(); // Default value
    else
        colWindowlist = c;

    c = ini->ReadIni("Colors", "WindowListBackground");
    if (c.length() == 0)
        colWindowlistBackground = C_WHITE.name(); // Default value
    else
        colWindowlistBackground = c;



    /**************************
     *  SCRIPT EDITOR COLORS  *
     **************************/

    c = ini->ReadIni("Colors", "ListboxBackground");
    if (c.length() == 0)
        colListboxBackground = C_WHITE.name(); // Default value
    else
        colListboxBackground = c;

    c = ini->ReadIni("Colors", "EditorLineHighlight");
    if (c.length() == 0)
        editorLineHighlight = "#CCFFFF"; // Default value
    else
        editorLineHighlight = c;

    c = ini->ReadIni("Colors", "EditorFunctionDef");
    if (c.length() == 0)
        editorFunctionDef = Qt::darkCyan; // Default value
    else
        editorFunctionDef = c;

    c = ini->ReadIni("Colors", "EditorMetaKeyword");
    if (c.length() == 0)
        editorMetaKeyword = Qt::darkGreen; // Default value
    else
        editorMetaKeyword = c;

    c = ini->ReadIni("Colors", "EditorKeyword");
    if (c.length() == 0)
        editorKeyword = Qt::darkYellow; // Default value
    else
        editorKeyword = c;

    c = ini->ReadIni("Colors", "EditorWindow");
    if (c.length() == 0)
        editorWindow = Qt::darkRed; // Default value
    else
        editorWindow = c;

    c = ini->ReadIni("Colors", "EditorVariable");
    if (c.length() == 0)
        editorVariable = Qt::darkBlue; // Default value
    else
        editorVariable = c;

    c = ini->ReadIni("Colors", "EditorComment");
    if (c.length() == 0)
        editorComment = Qt::gray; // Default value
    else
        editorComment = c;


    if (treeWidth < 25)
        treeWidth = 125;

    if (listboxWidth < 25)
        listboxWidth = 150;

    if (treeWidth > 200)
        treeWidth = 200;

    if (listboxWidth > 200)
        listboxWidth = 200;

}

void config::save()
{
    // Save everything back to config file...

    QString host, port = "";
    if (server.split(":").count() == 2) {
        host = server.split(":").at(0);
        port = server.split(":").at(1);
    }

    ini->WriteIni("Pos", "X", QString::number(mainWinGeo.x()));
    ini->WriteIni("Pos", "Y", QString::number(mainWinGeo.y()));
    ini->WriteIni("Pos", "W", QString::number(mainWinGeo.width()));
    ini->WriteIni("Pos", "H", QString::number(mainWinGeo.height()));
    ini->WriteIni("Pos", "Maximized", QString::number(maximized));

    ini->WriteIni("Options", "Nickname", nickname);
    ini->WriteIni("Options", "AltNick", altnick);
    ini->WriteIni("Options", "Realname", realname);
    ini->WriteIni("Options", "Username", username);
    ini->WriteIni("Options", "Server", host);
    ini->WriteIni("Options", "Port", port);
    ini->WriteIni("Options", "Password", password);
    ini->WriteIni("Options", "Quit", quit);
    ini->WriteIni("Options", "BgImagePath", bgImagePath);
    ini->WriteIni("Options", "BgZoomLevel", QString::number(bgZoomLevel));
    ini->WriteIni("Options", "BgZoomScaled", QString::number(bgZoomScaled));
    ini->WriteIni("Options", "FontName", fontName);
    ini->WriteIni("Options", "FontSize", QString::number(fontSize));
    ini->WriteIni("Options", "Charset", charset);
    ini->WriteIni("Options", "LinksUnderline", QString::number(linkUnderline));
    ini->WriteIni("Options", "TreeWidth", QString::number(treeWidth));
    ini->WriteIni("Options", "ListBoxWidth", QString::number(listboxWidth));
    ini->WriteIni("Options", "TimeStamp", timestamp);
    ini->WriteIni("Options", "ShowTimeStamp", QString::number(showTimestmap));
    ini->WriteIni("Options", "ShowOptions", QString::number(showOptionsStartup));
    ini->WriteIni("Options", "Invisible", QString::number(connectInvisible));
    ini->WriteIni("Options", "ShowFavourites", QString::number(showFavourites));
    ini->WriteIni("Options", "ShowUsermodeMsg", QString::number(showUsermodeMsg));
    ini->WriteIni("Options", "TrayNotify", QString::number(trayNotify));
    ini->WriteIni("Options", "TrayNotifyDelay", QString::number(trayNotifyDelay/1000));
    ini->WriteIni("Options", "ShowWhois", QString::number(showWhois));
    ini->WriteIni("Options", "CheckVersion", QString::number(checkVersion));
    ini->WriteIni("Options", "ShowMotd", QString::number(showMotd));
    ini->WriteIni("Options", "ShowToolBar", QString::number(showToolBar));
    ini->WriteIni("Options", "AutoReJoin", QString::number(autoReJoin));
    ini->WriteIni("Options", "Log", QString::number(logEnabled));
    ini->WriteIni("Options", "LogPath", logPath);
    ini->WriteIni("Options", "LogChan", QString::number(logChannel));
    ini->WriteIni("Options", "LogPM", QString::number(logPM));

    // Save colors
    ini->WriteIni("Colors", "Default", colDefault);
    ini->WriteIni("Colors", "LocalInfo", colLocalInfo);
    ini->WriteIni("Colors", "ServerInfo", colServerInfo);
    ini->WriteIni("Colors", "Action", colAction);
    ini->WriteIni("Colors", "CTCP", colCTCP);
    ini->WriteIni("Colors", "Notice", colNotice);
    ini->WriteIni("Colors", "Background", colBackground);
    ini->WriteIni("Colors", "OwnTextBackground", colOwntextBg);
    ini->WriteIni("Colors", "OwnText", colOwntext);
    ini->WriteIni("Colors", "Highlight", colHighlight);
    ini->WriteIni("Colors", "Links", colLinks);
    ini->WriteIni("Colors", "Input", colInput);
    ini->WriteIni("Colors", "InputBackground", colInputBackground);
    ini->WriteIni("Colors", "Listbox", colListbox);
    ini->WriteIni("Colors", "ListboxBackground", colListboxBackground);
    ini->WriteIni("Colors", "WindowList", colWindowlist);
    ini->WriteIni("Colors", "WindowListBackground", colWindowlistBackground);

    // Script editor colors
    ini->WriteIni("Colors", "EditorLineHighlight", editorLineHighlight.name());
    ini->WriteIni("Colors", "EditorFunctionDef", editorFunctionDef.name());
    ini->WriteIni("Colors", "EditorMetaKeyword", editorMetaKeyword.name());
    ini->WriteIni("Colors", "EditorKeyword", editorKeyword.name());
    ini->WriteIni("Colors", "EditorWindow", editorWindow.name());
    ini->WriteIni("Colors", "EditorVariable", editorVariable.name());
    ini->WriteIni("Colors", "EditorComment", editorComment.name());
}


int config::bti(bool b) // Bool to Int
{
    // If b equals true return 1, else 0.
    return b == true ? 1 : 0;
}

bool config::itb(int i) // Int to Bool
{
    // If i is greater or equal to 1 return true, else false.
    return i >= 1 ? true : false;
}

bool config::stb(QString s) // QString to Bool
{
    if (s.length() == 0)
        return false;

    // If the first letter in string "s" equals '1' return true, else false
    return s[0] == '1' ? true : false;
}
