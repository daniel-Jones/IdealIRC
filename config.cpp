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

    ini = new QIniFile(CONF_FILE);
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

  fontSize = ini->ReadIni("Options", "FontSize").toInt();
  if (fontSize == 0)
      fontSize = 13; // Default size

  treeWidth = ini->ReadIni("Options", "TreeWidth").toInt();
  listboxWidth = ini->ReadIni("Options", "ListBoxWidth").toInt();
  trayNotifyDelay = ini->ReadIni("Options", "TrayNotifyDelay").toInt();
  bgZoomLevel = ini->ReadIni("Options", "BgZoomLevel").toInt();

  showTimestmap = stb(ini->ReadIni("Options", "ShowTimeStamp"));
  showOptionsStartup = stb(ini->ReadIni("Options", "ShowOptions"));
  connectInvisible = stb(ini->ReadIni("Options", "connectInvisible"));
  showFavourites = stb(ini->ReadIni("Options", "ShowFavourites"));
  showUsermodeMsg = stb(ini->ReadIni("Options", "ShowUsermodeMsg"));

  if (ini->ReadIni("Options", "ShowWhois").length() == 0)
    showWhois = false;
  else
    showWhois = stb(ini->ReadIni("Options", "ShowWhois"));

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

  // Load colors.
  QString c;
  c = ini->ReadIni("Colors", "Default");
  if (c.length() == 0)
    colDefault = 1; // Default value
  else
    colDefault = c.toInt();

  c = ini->ReadIni("Colors", "LocalInfo");
  if (c.length() == 0)
    colLocalInfo = 2; // Default value
  else
    colLocalInfo = c.toInt();

  c = ini->ReadIni("Colors", "ServerInfo");
  if (c.length() == 0)
    colServerInfo = 3; // Default value
  else
    colServerInfo = c.toInt();

  c = ini->ReadIni("Colors", "Action");
  if (c.length() == 0)
    colAction = 6; // Default value
  else
    colAction = c.toInt();

  c = ini->ReadIni("Colors", "CTCP");
  if (c.length() == 0)
    colCTCP = 4; // Default value
  else
    colCTCP = c.toInt();

  c = ini->ReadIni("Colors", "Notice");
  if (c.length() == 0)
    colNotice = 5; // Default value
  else
    colNotice = c.toInt();

  c = ini->ReadIni("Colors", "OwnTextBackground");
  if (c.length() == 0)
    colOwntextBg = -1; // Default value
  else
    colOwntextBg = c.toInt();

  c = ini->ReadIni("Colors", "OwnText");
  if (c.length() == 0)
    colOwntext = 10; // Default value
  else
    colOwntext = c.toInt();

  c = ini->ReadIni("Colors", "Links");
  if (c.length() == 0)
    colLinks = 12; // Default value
  else
    colLinks = c.toInt();

  c = ini->ReadIni("Colors", "Background");
  if (c.length() == 0)
    colBackground = 0; // Default value
  else
    colBackground = c.toInt();

  c = ini->ReadIni("Colors", "Input");
  if (c.length() == 0)
    colInput = 1; // Default value
  else
    colInput = c.toInt();

  c = ini->ReadIni("Colors", "InputBackground");
  if (c.length() == 0)
    colInputBackground = 0; // Default value
  else
    colInputBackground = c.toInt();

  c = ini->ReadIni("Colors", "Listbox");
  if (c.length() == 0)
    colListbox = 1; // Default value
  else
    colListbox = c.toInt();

  c = ini->ReadIni("Colors", "ListboxBackground");
  if (c.length() == 0)
    colListboxBackground = 0; // Default value
  else
    colListboxBackground = c.toInt();


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
  ini->WriteIni("Options", "LinksUnderline", QString::number(linkUnderline));
  ini->WriteIni("Options", "TreeWidth", QString::number(treeWidth));
  ini->WriteIni("Options", "ListBoxWidth", QString::number(listboxWidth));
  ini->WriteIni("Options", "ShowTimeStamp", QString::number(showTimestmap));
  ini->WriteIni("Options", "ShowOptions", QString::number(showOptionsStartup));
  ini->WriteIni("Options", "Invisible", QString::number(connectInvisible));
  ini->WriteIni("Options", "ShowFavourites", QString::number(showFavourites));
  ini->WriteIni("Options", "ShowUsermodeMsg", QString::number(showUsermodeMsg));
  ini->WriteIni("Options", "TrayNotify", QString::number(trayNotify));
  ini->WriteIni("Options", "TrayNotifyDelay", QString::number(trayNotifyDelay));
  ini->WriteIni("Options", "ShowWhois", QString::number(showWhois));
  ini->WriteIni("Options", "Log", QString::number(logEnabled));
  ini->WriteIni("Options", "LogPath", logPath);
  ini->WriteIni("Options", "LogChan", QString::number(logChannel));
  ini->WriteIni("Options", "LogPM", QString::number(logPM));

  // Save colors
  ini->WriteIni("Colors", "Default", QString::number(colDefault));
  ini->WriteIni("Colors", "LocalInfo", QString::number(colLocalInfo));
  ini->WriteIni("Colors", "ServerInfo", QString::number(colServerInfo));
  ini->WriteIni("Colors", "Action", QString::number(colAction));
  ini->WriteIni("Colors", "CTCP", QString::number(colCTCP));
  ini->WriteIni("Colors", "Notice", QString::number(colNotice));
  ini->WriteIni("Colors", "Background", QString::number(colBackground));
  ini->WriteIni("Colors", "OwnTextBackground", QString::number(colOwntextBg));
  ini->WriteIni("Colors", "OwnText", QString::number(colOwntext));
  ini->WriteIni("Colors", "Links", QString::number(colLinks));
  ini->WriteIni("Colors", "Input", QString::number(colInput));
  ini->WriteIni("Colors", "InputBackground", QString::number(colInputBackground));
  ini->WriteIni("Colors", "Listbox", QString::number(colListbox));
  ini->WriteIni("Colors", "ListboxBackground", QString::number(colListboxBackground));
}


int config::bti(bool b)
{
    // If b equals true return 1, else 0.
    return b == true ? 1 : 0;
}

bool config::itb(int i)
{
    // If i is greater or equal to 1 return true, else false.
    return i >= 1 ? true : false;
}

bool config::stb(QString s)
{
    if (s.length() == 0)
        return false;

    // If the first letter in string "s" equals '1' return true, else false
    return s[0] == '1' ? true : false;
}
