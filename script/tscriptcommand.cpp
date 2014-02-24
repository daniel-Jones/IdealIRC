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

#include "tscriptcommand.h"
#include "iconnection.h"
#include "icommand.h"

#include <QHashIterator>

TScriptCommand::TScriptCommand(QObject *parent, QHash<int,IConnection*> *cl, QHash<int,subwindow_t> *wl, int *aConn, int *aWid) :
    QObject(parent),
    conlist(cl),
    winlist(wl),
    activeConn(aConn),
    activeWid(aWid)
{
}

bool TScriptCommand::parse(QString &command)
{
    if (command.length() == 0)
        return false; // Why parse empty stuff?

    if (command[0] == '/')
        command = command.mid(2); // skip the /

    // Find our current command handler.

    if (*activeConn > -1) {
        IConnection *c = conlist->value(*activeConn);
        ICommand *cmd = c->getCmdHndlPtr();
        if (cmd->parse(command))
            return true; // found our command here. just stop.
    }

    // Parse any internal commands from here.
    QStringList token = command.split(' ');
    QString acmd = token[0].toUpper();

    if (acmd == "ECHO") {
        // Echo a text to active window.
        QString text = command.mid(5); // skip "echo" and actually echo this.
        echo("$ACTIVE$", text);

        return true;
    }

    if (acmd == "WINDOW") {
        // Create a custom @window

        if (token.count() == 1) {
            echo("STATUS", tr("/Window: Too few parameters"), PT_LOCALINFO);
            return true;
        }

        QString sw;
        QString name;
        if (token.count() > 2) {
            sw = token[1];
            name = token[2];
        }
        if (token.count() == 2) {
            name = token[1];
        }

        window(name, sw);

        return true;
    }

    if (acmd == "CLEAR") {
        // Clear active window or @window

        return true;
    }

    if (acmd == "PAINTDOT") {
        // Paints a dot in @window

        return true;
    }

    if (acmd == "PAINTLINE") {
        // Paints a line in @window

        return true;
    }

    if (acmd == "PAINTRECT") {
        // Paints a rectangle in @window

        return true;
    }

    if (acmd == "PAINTIMAGE") {
        // Paints an image in @window
        // This will also keep the image in buffer of IIRC so you can do fast re-drawing
        // for GUI, games and such.

        return true;
    }

    if (acmd == "PAINTTEXT") {
        // Paints a text in @window

        return true;
    }

    if (acmd == "PAINTFILL") {
        // Fill a @window with a color

        return true;
    }

    if (acmd == "CLEARIMGBUF") {
        // Clear the image buffer used by /paintimage

        return true;
    }

    if (acmd == "PAINTBUFFER") {
        // Used to turn on/off instant painting in a @window
        // Useful if there's a lot to redraw during a timer (e.g. games, gui)

        return true;
    }

    // Reaching here means we did not find our command. The server should take care of it.
    return false;
}

void TScriptCommand::echo(QString target, QString text, int type)
{
    // Target is custom window
    if (target[0] == '@') {
        subwindow_t sw;
        QHashIterator<int,subwindow_t> i(*winlist);
        while (i.hasNext()) {
            sw = i.next().value();
            if (sw.widget->objectName().toUpper() == target.toUpper())
                sw.widget->print(text, type);
        }

        return;
    }

    if (target == "$ACTIVE$") {
        subwindow_t sw = winlist->value(*activeWid);
        sw.widget->print(text);
        return;
    }

    // Target is a "regular" window, search under the active connections window list
    IConnection *c = conlist->value(*activeConn, NULL);
    if (c == NULL)
        return;
    else
        c->print(target, text, type);
}

void TScriptCommand::window(QString name, QString sw)
{

    bool drawable = false;
    bool withinput = false;
    bool activate = true;

    if (name.left(1) != "@") {
        echo("STATUS", tr("/Window: Window name must be prepended with @"), PT_LOCALINFO);
        return;
    }

    if (name.length() == 1) {
        echo("STATUS", tr("/Window: Invalid name"), PT_LOCALINFO);
        return;
    }

    for (int i = 0; i <= sw.length()-1; i++) {
      char s = sw.at(i).toLatin1();
      switch (s) {
          case 'd':
              drawable = true;
              continue;
          case 'i':
              withinput = true;
              continue;
          case 'n':
              activate = false;
              continue;
          case '-':
              continue;
          default:
              echo("STATUS", tr("/Window: Unknown switch '%1'").arg(s), PT_LOCALINFO);
              return;
        }
    }

    int t = WT_TXTONLY;

    if (drawable == true) {
        t = WT_GRAPHIC;
        if (withinput)
            t++; // +1 on WT_GRAPHIC gives WT_GWINPUT
    }

    if ((drawable == false) && (withinput == true))
          t = WT_PRIVMSG; // WT_PRIVMSG -- still a custom window though, but we get text with input.

     emit RequestWindow(name, t, -1, activate);

}

void TScriptCommand::sockwrite(QString &data)
{
    // Send data to active socket. If there's none, ignore the data.

    if (*activeConn > -1) {
        IConnection *c = conlist->value(*activeConn);

        c->sockwrite(data);
    }
}
