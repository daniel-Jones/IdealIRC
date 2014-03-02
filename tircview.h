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

#ifndef TIRCVIEW_H
#define TIRCVIEW_H

#include <QPaintEvent>
#include <QFocusEvent>
#include <QTextBrowser>
#include <QColor>
#include <QContextMenuEvent>
#include <QClipboard>

#include "constants.h"
#include "config.h"

typedef struct T_LINE {
    // This struct contains "raw text".
    // Not HTML formatted.
    int type;
    QByteArray text;
} line_t;

class TIRCView : public QTextBrowser
{
  Q_OBJECT

public:
    explicit TIRCView(config *cfg, QWidget *parent = NULL);
    ~TIRCView();
    void addLine(QString line, int ptype = 0, bool rebuilding = false);
    void rebuild();
    void reloadCSS();
    void clear();

private:
    QColor getColorFromCode(int num);
    QString textHTML;
    QVector<line_t> text;
    config *conf;
    QClipboard *clip;
    QString getCustomCSSColor(QString numeric);

protected:
    void focusInEvent(QFocusEvent *e);
    void contextMenuEvent(QContextMenuEvent *e);

private slots:
    void textSelected();

signals:
    void joinChannel(QString channel);
    void gotFocus();
    void menuRequested(QPoint point);
};

#endif // TIRCVIEW_H
