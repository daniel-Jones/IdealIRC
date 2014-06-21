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
    activeWid(aWid),
    tstar("***")
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
            echo("STATUS", tstar, InvalidParameterCount("Echo"), PT_LOCALINFO);
            return true;
        }

        QString text = command.mid(5); // skip "echo" and actually echo this.
        QString target = "$ACTIVE$";

        if (token[1][0] == '@') {
            target = token[1];
            text = text.mid( target.length()+1 );
        }

        echo(target, "", text);

        return true;
    }

    if (acmd == "WINDOW") {
        // Create a custom @window

        if (token.count() == 1) {
            echo("STATUS", tstar, InvalidParameterCount("Window"), PT_LOCALINFO);
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

        if (token.count() == 1) // No parameters (clear active window)
            clear();

        if (token.count() == 2) // /clear window
            clear(token[1]);

        if (token.count() == 3)
            clear(token[2], token[1]);

        return true;
    }

    if (acmd == "PAINTDOT") {
        // Paints a dot in @window
        if (token.count() != 6) {
            echo("STATUS", tstar, InvalidParameterCount("Paintdot"), PT_LOCALINFO);
            return true;
        }

        paintdot(token[1], token[2], token[3], token[4], token[5]);
        return true;
    }

    if (acmd == "PAINTLINE") {
        // Paints a line in @window
        if (token.count() != 8) {
            echo("STATUS", tstar, InvalidParameterCount("Paintline"), PT_LOCALINFO);
            return true;
        }

        paintline(token[1], token[2], token[3], token[4], token[5], token[6], token[7]);
        return true;
    }

    if (acmd == "PAINTRECT") {
        // Paints a rectangle in @window
        if (token.count() != 8) {
            echo("STATUS", tstar, InvalidParameterCount("Paintrect"), PT_LOCALINFO);
            return true;
        }

        paintrect(token[1], token[2], token[3], token[4], token[5], token[6], token[7]);
        return true;
    }

    if (acmd == "PAINTIMAGE") {
        // Paints an image in @window
        // This will also keep the image in buffer of IIRC so you can do fast re-drawing
        // for GUI, games and such.
        // /paintimage -u @win x y path

        if (token.count() < 5) {
            echo("STATUS", tstar, InvalidParameterCount("Paintimage"), PT_LOCALINFO);
            return true;
        }
        // length to skip to get filename

        bool dontBuffer = false;
        if (token[1] == "-u") {
            dontBuffer = true;
            token.removeAt(1);
        }

        int skip = 0;
        for (int i = 0; i <= 3; i++)
            skip += token[i].length() + 1;

        QString path = command.mid(skip);

        paintimage(token[1], token[2], token[3], path, dontBuffer);
        return true;
    }

    if (acmd == "PAINTTEXT") {
        // Paints a text in @window
        if (token.count() < 8) {
            echo("STATUS", tstar, InvalidParameterCount("Painttext"), PT_LOCALINFO);
            return true;
        }

        // Length to skip to text
        int skip = 0;
        for (int i = 0; i <= 6; i++)
            skip += token[i].length() + 1;
        QString Text = command.mid(skip);

        painttext(token[1], token[2], token[3], token[4], token[5], token[6].replace('_', ' '), Text);

        return true;
    }

    if (acmd == "PAINTFILL") {
        // Fill a @window with a color
        if (token.count() != 7) {
            echo("STATUS", tstar, InvalidParameterCount("Paintfill"), PT_LOCALINFO);
            return true;
        }

        paintfill(token[1], token[2], token[3], token[4], token[5], token[6]);
        return true;
    }

    if (acmd == "PAINTFILLPATH") {
        // create a filled shape.
        // /paintfillpath @window color x y x y x y ...
        if ((token.count() < 9) || (((token.count()-3)%2) != 0)) {
            // parameter count of x y x y etc must be dividable by two.
            echo("STATUS", tstar, InvalidParameterCount("Paintfill"), PT_LOCALINFO);
            return true;
        }

        // start points:
        int x = floor(token[3].toFloat());
        int y = floor(token[4].toInt());
        QPainterPath path( QPoint(x, y) );

        for (int i = 5; i <= token.count()-1; ++i) {
            x = floor(token[i].toFloat());
            ++i;
            y = floor(token[i].toFloat());
            path.lineTo(x, y);
        }

        paintfillpath(token[1], token[2], path);
        return true;
    }

    if (acmd == "PAINTSETLAYER") {
        // Sets current layer / creates a new layer to draw on.
        if (token.count() != 3) {
            echo("STATUS", tstar, InvalidParameterCount("Paintsetlayer"), PT_LOCALINFO);
            return true;
        }

        paintsetlayer(token[1], token[2]);
        return true;
    }

    if (acmd == "PAINTDELLAYER") {
        // Delete layer from a paint window
        if (token.count() != 3) {
            echo("STATUS", tstar, InvalidParameterCount("Paintdellayer"), PT_LOCALINFO);
            return true;
        }

        paintdellayer(token[1], token[2]);
        return true;
    }

    if (acmd == "PAINTLAYERORDER") {
        if (token.count() < 4) { // /paintlayerorder @win layer1 layer2 ... (at least 2 layers)
            echo("STATUS", tstar, InvalidParameterCount("Paintlayerorder"), PT_LOCALINFO);
            return true;
        }

        QString window = token[1];
        QStringList list = token;
        list.removeFirst(); // removes /paintlayerorder
        list.removeFirst(); // remoes @window

        paintlayerorder(window, list);
        return true;
    }

    if (acmd == "CLEARIMGBUF") {
        // Clear the image buffer used by /paintimage
        if (token.count() != 2) {
            echo("STATUS", tstar, InvalidParameterCount("Clearimgbuf"), PT_LOCALINFO);
            return true;
        }

        clearimgbuf(token[1]);
        return true;
    }

    if (acmd == "PAINTBUFFER") {
        // Used to turn on/off instant painting in a @window
        // Useful if there's a lot to redraw during a timer (e.g. games, gui)
        if (token.count() != 3) {
            echo("STATUS", tstar, InvalidParameterCount("Paintbuffer"), PT_LOCALINFO);
            return true;
        }

        int set = token[2].toInt();
        bool state = (bool)set;

        paintbuffer(token[1], state);
        return true;
    }

    // Reaching here means we did not find our command. The server should take care of it.
    return false;
}

void TScriptCommand::echo(QString target, QString sender, QString text, int type)
{
    // Target is custom window
    if (target[0] == '@') {
        subwindow_t sw;
        QHashIterator<int,subwindow_t> i(*winlist);
        while (i.hasNext()) {
            sw = i.next().value();
            if (sw.widget->objectName().toUpper() == target.toUpper())
                sw.widget->print(sender, text, type);
        }

        return;
    }

    if (target == "$ACTIVE$") {
        subwindow_t sw = winlist->value(*activeWid);
        if (sw.type >= WT_GRAPHIC)
            sw = winlist->value(*activeConn); // active connection is the previous active one.
        sw.widget->print(sender, text);
        return;
    }

    // Target is a "regular" window, search under the active connections window list
    IConnection *c = conlist->value(*activeConn, NULL);
    if (c == NULL)
        return;
    else
        c->print(target, sender, text, type);
}

void TScriptCommand::window(QString name, QString sw)
{

    bool drawable = false;
    bool withinput = false;
    bool activate = true;

    if (name.left(1) != "@") {
        echo("STATUS", tstar, tr("/Window: Window name must be prepended with @"), PT_LOCALINFO);
        return;
    }

    if (name.length() == 1) {
        echo("STATUS", tstar, tr("/Window: Invalid name"), PT_LOCALINFO);
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
              echo("STATUS", tstar, tr("/Window: Unknown switch '%1'").arg(s), PT_LOCALINFO);
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

void TScriptCommand::clear(QString window, QString sw)
{
    subwindow_t subwin;
    if (window.isEmpty())
        subwin = winlist->value(*activeWid);
    else
        subwin = getCustomWindow(window);

    if (subwin.type == WT_NOTHING) { // Didn't find the window.
        echo("STATUS", tstar, NoSuchWindow("Clear"), PT_LOCALINFO);
        return;
    }

    if (subwin.type >= WT_GRAPHIC) { // Window is a graphic window.
        if (sw == "-l")
            subwin.widget->picwinPtr()->clear(); // clear current layer
        else
            subwin.widget->picwinPtr()->clearAll(); // clear all layers
        return;
    }

    // Reaching here means a text window to clear.
    subwin.widget->clear();
}

void TScriptCommand::paintdot(QString Window, QString X, QString Y, QString Size, QString Color)
{
    subwindow_t wt = getCustomWindow(Window);
    if (wt.type == WT_NOTHING) {
        echo("STATUS", tstar, NoSuchWindow("Paintdot"), PT_LOCALINFO);
        return;
    }

    if (wt.type < WT_GRAPHIC) {
        echo("STATUS", tstar, NotAPaintWindow("Paintdot"), PT_LOCALINFO);
        return;
    }

    int iX = floor(X.toFloat());
    int iY = floor(Y.toFloat());
    int iSize = Size.toInt();

    QColor iColor(Color);
    QBrush br(iColor, Qt::SolidPattern);
    QPen pn(iColor);
    pn.setWidth(iSize);

    wt.widget->picwinPtr()->setBrushPen(br, pn);
    wt.widget->picwinPtr()->paintDot(iX, iY);
}

void TScriptCommand::paintline(QString Window, QString X1, QString Y1, QString X2, QString Y2, QString Size, QString Color)
{
    subwindow_t wt = getCustomWindow(Window);
    if (wt.type == WT_NOTHING) {
        echo("STATUS", tstar, NoSuchWindow("Paintline"), PT_LOCALINFO);
        return;
    }

    if (wt.type < WT_GRAPHIC) {
        echo("STATUS", tstar, NotAPaintWindow("Paintline"), PT_LOCALINFO);
        return;
    }

    int iX1 = floor(X1.toFloat());
    int iY1 = floor(Y1.toFloat());
    int iX2 = floor(X2.toFloat());
    int iY2 = floor(Y2.toFloat());

    int iSize = Size.toInt();
    QColor iColor(Color);
    QBrush br(iColor, Qt::SolidPattern);
    QPen pn(iColor);
    pn.setWidth(iSize);

    wt.widget->picwinPtr()->setBrushPen(br, pn);
    wt.widget->picwinPtr()->paintLine(iX1, iY1, iX2, iY2);
}

void TScriptCommand::paintrect(QString Window, QString X, QString Y, QString W, QString H, QString Size, QString Color)
{
    subwindow_t wt = getCustomWindow(Window);
    if (wt.type == WT_NOTHING) {
        echo("STATUS", tstar, NoSuchWindow("Paintrect"), PT_LOCALINFO);
        return;
    }

    if (wt.type < WT_GRAPHIC) {
        echo("STATUS", tstar, NotAPaintWindow("Paintrect"), PT_LOCALINFO);
        return;
    }

    int iX = floor(X.toFloat());
    int iY = floor(Y.toFloat());
    int iW = floor(W.toFloat());
    int iH = floor(H.toFloat());

    int iSize = Size.toInt();
    QColor iColor(Color);
    QBrush br(iColor, Qt::SolidPattern);
    QPen pn(iColor);
    pn.setWidth(iSize);

    wt.widget->picwinPtr()->setBrushPen(br, pn);
    wt.widget->picwinPtr()->paintRect(iX, iY, iW, iH);
}

void TScriptCommand::paintimage(QString Window, QString X, QString Y, QString File, bool dontBuffer)
{
    subwindow_t wt = getCustomWindow(Window);
    if (wt.type == WT_NOTHING) {
        echo("STATUS", tstar, NoSuchWindow("Paintimage"), PT_LOCALINFO);
        return;
    }

    if (wt.type < WT_GRAPHIC) {
        echo("STATUS", tstar, NotAPaintWindow("Paintimage"), PT_LOCALINFO);
        return;
    }
    int iX = floor(X.toFloat());
    int iY = floor(Y.toFloat());

    wt.widget->picwinPtr()->paintImage(File, iX, iY, dontBuffer);
}

void TScriptCommand::painttext(QString Window, QString X, QString Y, QString FontSize, QString Color, QString FontName, QString Text)
{
    subwindow_t wt = getCustomWindow(Window);
    if (wt.type == WT_NOTHING) {
        echo("STATUS", tstar, NoSuchWindow("Painttext"), PT_LOCALINFO);
        return;
    }

    if (wt.type < WT_GRAPHIC) {
        echo("STATUS", tstar, NotAPaintWindow("Painttext"), PT_LOCALINFO);
        return;
    }

    int iX = floor(X.toFloat());
    int iY = floor(Y.toFloat());

    QColor iColor(Color);
    QFont Font(FontName);
    Font.setPixelSize(FontSize.toInt());

    QBrush br(iColor, Qt::SolidPattern);
    QPen pn(iColor);

    wt.widget->picwinPtr()->setBrushPen(br, pn);
    wt.widget->picwinPtr()->paintText(iX, iY, Font, Text);
}

void TScriptCommand::paintfill(QString Window, QString X, QString Y, QString W, QString H, QString Color)
{

    subwindow_t wt = getCustomWindow(Window);
    if (wt.type == WT_NOTHING) {
        echo("STATUS", tstar, NoSuchWindow("Paintfill"), PT_LOCALINFO);
        return;
    }

    if (wt.type < WT_GRAPHIC) {
        echo("STATUS", tstar, NotAPaintWindow("Paintfill"), PT_LOCALINFO);
        return;
    }

    int iX = floor(X.toFloat());
    int iY = floor(Y.toFloat());
    int iW = floor(W.toFloat());
    int iH = floor(H.toFloat());

    QColor iColor(Color);
    QBrush br(iColor, Qt::SolidPattern);
    QPen pn(iColor);

    wt.widget->picwinPtr()->setBrushPen(br, pn);
    wt.widget->picwinPtr()->paintFill(iX, iY, iW, iH);
}

void TScriptCommand::paintfillpath(QString Window, QString Color, QPainterPath Path)
{
    subwindow_t wt = getCustomWindow(Window);
    if (wt.type == WT_NOTHING) {
        echo("STATUS", tstar, NoSuchWindow("Paintfillpath"), PT_LOCALINFO);
        return;
    }

    if (wt.type < WT_GRAPHIC) {
        echo("STATUS", tstar, NotAPaintWindow("Paintfillpath"), PT_LOCALINFO);
        return;
    }

    QColor iColor(Color);
    QBrush br(iColor, Qt::SolidPattern);
    QPen pn(iColor);

    wt.widget->picwinPtr()->setBrushPen(br, pn);
    wt.widget->picwinPtr()->paintFillPath(Path);
}

void TScriptCommand::paintsetlayer(QString Window, QString Layer)
{
    subwindow_t wt = getCustomWindow(Window);
    if (wt.type == WT_NOTHING) {
        echo("STATUS", tstar, NoSuchWindow("Paintsetlayer"), PT_LOCALINFO);
        return;
    }

    if (wt.type < WT_GRAPHIC) {
        echo("STATUS", tstar, NotAPaintWindow("Paintsetlayer"), PT_LOCALINFO);
        return;
    }

    wt.widget->picwinPtr()->setLayer(Layer);
}

void TScriptCommand::paintdellayer(QString Window, QString Layer)
{
    subwindow_t wt = getCustomWindow(Window);
    if (wt.type == WT_NOTHING) {
        echo("STATUS", tstar, NoSuchWindow("Paintdellayer"), PT_LOCALINFO);
        return;
    }

    if (wt.type < WT_GRAPHIC) {
        echo("STATUS", tstar, NotAPaintWindow("Paintdellayer"), PT_LOCALINFO);
        return;
    }

    wt.widget->picwinPtr()->delLayer(Layer);
}

void TScriptCommand::paintlayerorder(QString Window, QStringList Layers)
{
    subwindow_t wt = getCustomWindow(Window);
    if (wt.type == WT_NOTHING) {
        echo("STATUS", tstar, NoSuchWindow("Paintlayerorder"), PT_LOCALINFO);
        return;
    }

    if (wt.type < WT_GRAPHIC) {
        echo("STATUS", tstar, NotAPaintWindow("Paintlayerorder"), PT_LOCALINFO);
        return;
    }

    wt.widget->picwinPtr()->orderLayers(Layers);
}

void TScriptCommand::clearimgbuf(QString Window)
{
    subwindow_t wt = getCustomWindow(Window);
    if (wt.type == WT_NOTHING) {
        echo("STATUS", tstar, NoSuchWindow("Clearimgbuf"), PT_LOCALINFO);
        return;
    }

    if (wt.type < WT_GRAPHIC) {
        echo("STATUS", tstar, NotAPaintWindow("Clearimgbuf"), PT_LOCALINFO);
        return;
    }

    wt.widget->picwinPtr()->clearImageBuffer();
}

void TScriptCommand::paintbuffer(QString Window, bool State)
{
    subwindow_t wt = getCustomWindow(Window);
    if (wt.type == WT_NOTHING) {
        echo("STATUS", tstar, NoSuchWindow("Paintbuffer"), PT_LOCALINFO);
        return;
    }

    if (wt.type < WT_GRAPHIC) {
        echo("STATUS", tstar, NotAPaintWindow("Paintbuffer"), PT_LOCALINFO);
        return;
    }

    wt.widget->picwinPtr()->setViewBuffer(State);
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
        if (sw.widget->objectName().toUpper() == name.toUpper())
            return sw;
    }

    return error;
}
