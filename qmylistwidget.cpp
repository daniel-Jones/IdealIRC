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

#include "qmylistwidget.h"
#include "constants.h"
#include <QPalette>

QMyListWidget::QMyListWidget(QWidget *parent, config *cfg) :
    QListWidget(parent),
    conf(cfg)
{
    updateCSS();

    QFont f(conf->fontName);
    f.setPixelSize(conf->fontSize);
    setFont(f);
}

void QMyListWidget::contextMenuEvent(QContextMenuEvent *e)
{
   emit MenuRequested(e->globalPos());
}

void QMyListWidget::updateCSS()
{
    QPalette pal = palette();
    pal.setColor(QPalette::Active, QPalette::Base, conf->colListboxBackground);
    pal.setColor(QPalette::Active, QPalette::AlternateBase, conf->colListboxBackground);
    pal.setColor(QPalette::Inactive, QPalette::Base, conf->colListboxBackground);
    pal.setColor(QPalette::Inactive, QPalette::AlternateBase, conf->colListboxBackground);
    pal.setColor(QPalette::Disabled, QPalette::Base, conf->colListboxBackground);
    pal.setColor(QPalette::Disabled, QPalette::AlternateBase, conf->colListboxBackground);
    pal.setColor(QPalette::Active, QPalette::Text, conf->colListbox);
    pal.setColor(QPalette::Active, QPalette::WindowText, conf->colListbox);
    pal.setColor(QPalette::Inactive, QPalette::Text, conf->colListbox);
    pal.setColor(QPalette::Inactive, QPalette::WindowText, conf->colListbox);
    pal.setColor(QPalette::Disabled, QPalette::Text, conf->colListbox);
    pal.setColor(QPalette::Disabled, QPalette::WindowText, conf->colListbox);
    setPalette(pal);
}

QColor QMyListWidget::getColorFromCode(int num)
{
    switch(num) {
        case 0:
            return C_WHITE;

        case 1:
            return C_BLACK;

        case 2:
            return C_BLUE;

        case 3:
            return C_GREEN;

        case 4:
            return C_BRIGHTRED;

        case 5:
            return C_RED;

        case 6:
            return C_MAGENTA;

        case 7:
            return C_BROWN;

        case 8:
            return C_YELLOW;

        case 9:
            return C_BRIGHTGREEN;

        case 10:
            return C_CYAN;

        case 11:
            return C_BRIGHTCYAN;

        case 12:
            return C_BRIGHTBLUE;

        case 13:
            return C_BRIGHTMAGENTA;

        case 14:
            return C_DARKGRAY;

        case 15:
            return C_LIGHTGRAY;

        default:
            return C_BLACK;
    }
}
