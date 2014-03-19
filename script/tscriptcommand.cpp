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
        if (token.count() == 1) {
            echo("STATUS", InvalidParameterCount("Echo"), PT_LOCALINFO);
            return true;
        }

        QString text = command.mid(5); // skip "echo" and actually echo this.
        QString target = "$ACTIVE$";

        if (token[1][0] == '@') {
            target = token[1];
            text = text.mid( target.length()+1 );
        }

        echo(target, text);

        return true;
    }

    if (acmd == "WINDOW") {
        // Create a custom @window

        if (token.count() == 1) {
            echo("STATUS", InvalidParameterCount("Window"), PT_LOCALINFO);
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
        subwindow_t subwin;
        if (token.count() == 1) {
            // No parameters (clear active window)
            subwin = winlist->value(*activeWid);
        }
        if (token.count() == 2) {
            // /clear window
            subwin = getCustomWindow(token[1]);
            if (subwin.type == WT_NOTHING) {
                echo("STATUS", NoSuchWindow("Clear"), PT_LOCALINFO);
                return true;
            }
            if (subwin.type >= WT_GRAPHIC) {
                subwin.widget->picwinPtr()->clearAll();
                return true;
            }
        }
        if (token.count() == 3) {
            // /clear -switch @window
            QString sw = token[1];
            subwin = getCustomWindow(token[2]);

            bool layer = false;

            for (int i = 0; i <= sw.length()-1; ++i) {
                QChar c = sw[i];
                switch (c.toLatin1()) {
                    case '-':
                        continue;

                    case 'l':
                        layer = true;
                        continue;
                }
            }

            if ((layer) && (subwin.type < WT_GRAPHIC)) {
                echo("STATUS", NotAPaintWindow("Clear"), PT_LOCALINFO);
                return true;
            }

            if ((layer) && (subwin.type >= WT_GRAPHIC)) {
                subwin.widget->picwinPtr()->clear();
                return true;
            }

            if ((! layer) && (subwin.type >= WT_GRAPHIC)) {
                subwin.widget->picwinPtr()->clearAll();
                return true;
            }
        }

        subwin.widget->clear();
        return true;
    }

    if (acmd == "PAINTDOT") {
        // Paints a dot in @window
        if (token.count() != 6) {
            echo("STATUS", InvalidParameterCount("Paintdot"), PT_LOCALINFO);
            return true;
        }

        subwindow_t wt = getCustomWindow(token[1]);
        if (wt.type == WT_NOTHING) {
            echo("STATUS", NoSuchWindow("Paintdot"), PT_LOCALINFO);
            return true;
        }

        if (wt.type < WT_GRAPHIC) {
            echo("STATUS", NotAPaintWindow("Paintdot"), PT_LOCALINFO);
            return true;
        }

        int X = floor(token[2].toFloat());
        int Y = floor(token[3].toFloat());
        int size = token[4].toInt();

        QColor color(token[5]);
        QBrush br(color, Qt::SolidPattern);
        QPen pn(color);
        pn.setWidth(size);

        wt.widget->picwinPtr()->setBrushPen(br, pn);
        wt.widget->picwinPtr()->paintDot(X, Y);

        return true;
    }

    if (acmd == "PAINTLINE") {
        // Paints a line in @window
        if (token.count() != 8) {
            echo("STATUS", InvalidParameterCount("Paintline"), PT_LOCALINFO);
            return true;
        }

        subwindow_t wt = getCustomWindow(token[1]);
        if (wt.type == WT_NOTHING) {
            echo("STATUS", NoSuchWindow("Paintline"), PT_LOCALINFO);
            return true;
        }

        if (wt.type < WT_GRAPHIC) {
            echo("STATUS", NotAPaintWindow("Paintline"), PT_LOCALINFO);
            return true;
        }

        int X1 = floor(token[2].toFloat());
        int Y1 = floor(token[3].toFloat());
        int X2 = floor(token[4].toFloat());
        int Y2 = floor(token[5].toFloat());

        int size = token[6].toInt();
        QColor color(token[7]);
        QBrush br(color, Qt::SolidPattern);
        QPen pn(color);
        pn.setWidth(size);

        wt.widget->picwinPtr()->setBrushPen(br, pn);
        wt.widget->picwinPtr()->paintLine(X1, Y1, X2, Y2);

        return true;
    }

    if (acmd == "PAINTRECT") {
        // Paints a rectangle in @window
        if (token.count() != 8) {
            echo("STATUS", InvalidParameterCount("Paintrect"), PT_LOCALINFO);
            return true;
        }

        subwindow_t wt = getCustomWindow(token[1]);
        if (wt.type == WT_NOTHING) {
            echo("STATUS", NoSuchWindow("Paintrect"), PT_LOCALINFO);
            return true;
        }

        if (wt.type < WT_GRAPHIC) {
            echo("STATUS", NotAPaintWindow("Paintrect"), PT_LOCALINFO);
            return true;
        }

        int X = floor(token[2].toFloat());
        int Y = floor(token[3].toFloat());
        int W = floor(token[4].toFloat());
        int H = floor(token[5].toFloat());

        int size = token[6].toInt();
        QColor color(token[7]);
        QBrush br(color, Qt::SolidPattern);
        QPen pn(color);
        pn.setWidth(size);

        wt.widget->picwinPtr()->setBrushPen(br, pn);
        wt.widget->picwinPtr()->paintRect(X, Y, W, H);
        return true;
    }

    if (acmd == "PAINTIMAGE") {
        // Paints an image in @window
        // This will also keep the image in buffer of IIRC so you can do fast re-drawing
        // for GUI, games and such.
        // /paintimage -u @win x y path

        if (token.count() < 5) {
            echo("STATUS", InvalidParameterCount("Paintimage"), PT_LOCALINFO);
            return true;
        }

        QString buffered; // for the -u switch
        if (token[1] == "-u") {
            buffered = "U";
            token.removeAt(1);
        }

        subwindow_t wt = getCustomWindow(token[1]);
        if (wt.type == WT_NOTHING) {
            echo("STATUS", NoSuchWindow("Paintimage"), PT_LOCALINFO);
            return true;
        }

        if (wt.type < WT_GRAPHIC) {
            echo("STATUS", NotAPaintWindow("Paintimage"), PT_LOCALINFO);
            return true;
        }


        // length to skip to get filename
        int skip = 0;
        for (int i = 0; i <= 3; i++)
            skip += token[i].length() + 1;

        QString File = command.mid(skip);
        int X = floor(token[2].toFloat());
        int Y = floor(token[3].toFloat());

        wt.widget->picwinPtr()->paintImage(File, X, Y);

        return true;
    }

    if (acmd == "PAINTTEXT") {
        // Paints a text in @window
        if (token.count() < 8) {
            echo("STATUS", InvalidParameterCount("Painttext"), PT_LOCALINFO);
            return true;
        }

        subwindow_t wt = getCustomWindow(token[1]);
        if (wt.type == WT_NOTHING) {
            echo("STATUS", NoSuchWindow("Painttext"), PT_LOCALINFO);
            return true;
        }

        if (wt.type < WT_GRAPHIC) {
            echo("STATUS", NotAPaintWindow("Painttext"), PT_LOCALINFO);
            return true;
        }

        int X = floor(token[2].toFloat());
        int Y = floor(token[3].toFloat());

        QColor Color(token[5]);
        QString FontName = token[6].replace('_', ' ');
        QFont Font(FontName);
        Font.setPixelSize(token[4].toInt());

        // Length to skip to text
        int skip = 0;
        for (int i = 0; i <= 6; i++)
            skip += token[i].length() + 1;

        QString Text = command.mid(skip);

        QBrush br(Color, Qt::SolidPattern);
        QPen pn(Color);

        wt.widget->picwinPtr()->setBrushPen(br, pn);
        wt.widget->picwinPtr()->paintText(X, Y, Font, Text);

        return true;
    }

    if (acmd == "PAINTFILL") {
        // Fill a @window with a color
        if (token.count() != 7) {
            echo("STATUS", InvalidParameterCount("Paintfill"), PT_LOCALINFO);
            return true;
        }

        subwindow_t wt = getCustomWindow(token[1]);
        if (wt.type == WT_NOTHING) {
            echo("STATUS", NoSuchWindow("Paintfill"), PT_LOCALINFO);
            return true;
        }

        if (wt.type < WT_GRAPHIC) {
            echo("STATUS", NotAPaintWindow("Paintfill"), PT_LOCALINFO);
            return true;
        }

        int X = floor(token[2].toFloat());
        int Y = floor(token[3].toFloat());
        int W = floor(token[4].toFloat());
        int H = floor(token[5].toFloat());

        QColor color(token[6]);
        QBrush br(color, Qt::SolidPattern);
        QPen pn(color);

        wt.widget->picwinPtr()->setBrushPen(br, pn);
        wt.widget->picwinPtr()->paintFill(X, Y, W, H);

        return true;
    }

    if (acmd == "CLEARIMGBUF") {
        // Clear the image buffer used by /paintimage
        if (token.count() != 2) {
            echo("STATUS", InvalidParameterCount("Clearimgbuf"), PT_LOCALINFO);
            return true;
        }

        subwindow_t wt = getCustomWindow(token[1]);
        if (wt.type == WT_NOTHING) {
            echo("STATUS", NoSuchWindow("Clearimgbuf"), PT_LOCALINFO);
            return true;
        }

        if (wt.type < WT_GRAPHIC) {
            echo("STATUS", NotAPaintWindow("Clearimgbuf"), PT_LOCALINFO);
            return true;
        }

        wt.widget->picwinPtr()->clearImageBuffer();

        return true;
    }

    if (acmd == "PAINTBUFFER") {
        // Used to turn on/off instant painting in a @window
        // Useful if there's a lot to redraw during a timer (e.g. games, gui)
        if (token.count() != 3) {
            echo("STATUS", InvalidParameterCount("Paintbuffer"), PT_LOCALINFO);
            return true;
        }

        subwindow_t wt = getCustomWindow(token[1]);
        if (wt.type == WT_NOTHING) {
            echo("STATUS", NoSuchWindow("Paintbuffer"), PT_LOCALINFO);
            return true;
        }

        if (wt.type < WT_GRAPHIC) {
            echo("STATUS", NotAPaintWindow("Paintbuffer"), PT_LOCALINFO);
            return true;
        }

        int set = token[2].toInt();
        bool state = (bool)set;

        wt.widget->picwinPtr()->setViewBuffer(state);

        return true;
    }

    if (acmd == "PAINTSETLAYER") {
        // Sets current layer / creates a new layer to draw on.
        if (token.count() != 3) {
            echo("STATUS", InvalidParameterCount("Paintsetlayer"), PT_LOCALINFO);
            return true;
        }

        subwindow_t wt = getCustomWindow(token[1]);
        if (wt.type == WT_NOTHING) {
            echo("STATUS", NoSuchWindow("Paintsetlayer"), PT_LOCALINFO);
            return true;
        }

        if (wt.type < WT_GRAPHIC) {
            echo("STATUS", NotAPaintWindow("Paintsetlayer"), PT_LOCALINFO);
            return true;
        }

        wt.widget->picwinPtr()->setLayer(token[2]);

        return true;
    }

    if (acmd == "PAINTDELLAYER") {
        // Delete layer from a paint window
        if (token.count() != 3) {
            echo("STATUS", InvalidParameterCount("Paintdellayer"), PT_LOCALINFO);
            return true;
        }

        subwindow_t wt = getCustomWindow(token[1]);
        if (wt.type == WT_NOTHING) {
            echo("STATUS", NoSuchWindow("Paintdellayer"), PT_LOCALINFO);
            return true;
        }

        if (wt.type < WT_GRAPHIC) {
            echo("STATUS", NotAPaintWindow("Paintdellayer"), PT_LOCALINFO);
            return true;
        }

        wt.widget->picwinPtr()->delLayer(token[2]);
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
        if (sw.type >= WT_GRAPHIC)
            sw = winlist->value(*activeConn); // active connection is the previous active one.
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
          t = WT_TXTINPUT;

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

subwindow_t TScriptCommand::getCustomWindow(QString name)
{
    subwindow_t error;
    error.type = WT_NOTHING; // Indicates an error.

    QHashIterator<int,subwindow_t> i(*winlist);
    while (i.hasNext()) {
        subwindow_t sw = i.next().value();
        if ((sw.type >= WT_GRAPHIC) && (sw.widget->objectName().toUpper() == name.toUpper()))
            return sw;

    }

    return error;
}
