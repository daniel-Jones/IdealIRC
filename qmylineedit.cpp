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

#include "qmylineedit.h"
#include "constants.h"
#include <iostream>

int LineEditStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
  if (metric == QStyle::PM_TextCursorWidth)
    return editptr->cursorSize;

  return QProxyStyle::pixelMetric(metric,option,widget);
}

QMyLineEdit::QMyLineEdit(QWidget *parent, config *cfg) :
    QLineEdit(parent),
    acIndex(NULL),
    cursorSize(1),
    kc(-1),
    conf(cfg)
{
    connect(this, SIGNAL(returnPressed()),
           this, SLOT(lnReturn()));

    updateCSS();

    style = new LineEditStyle(this);

    QFont f(conf->fontName);
    f.setPixelSize(conf->fontSize);
    setFont(f);
}

void QMyLineEdit::focusInEvent(QFocusEvent *e)
{
    cursorSize = 1;
    setStyle(style);
    e->ignore();
}

void QMyLineEdit::focusOutEvent(QFocusEvent *e)
{
    if (text().isEmpty())
        return;

    cursorSize = 0;
    setStyle(style);

    switch (e->reason()) {
        case Qt::TabFocusReason:
            setFocus();
            emit TabKeyPressed();
            e->accept();
            break;
        default:
            e->ignore();
    }
}

void QMyLineEdit::lnReturn()
{
    if (text().length() == 0)
        return;

    if (history.size() > 0)
        if (history[0] != text())
            history.push_front(text());

    if (history.size() == 0)
        history.push_front(text());

    if (history.size() > 50)
        history.pop_back();
}

void QMyLineEdit::keyPressEvent(QKeyEvent *e)
{

    if (e->key() == Qt::Key_Tab) {
        // This one actually doesn't call, check focusOutEvent()
    }

    if (acIndex != NULL)
        *acIndex = -1; // Reset the autocomplete index.

    if (e->key() == Qt::Key_Up) {

        if (history.count() == 0) {
            QLineEdit::keyPressEvent(e);
            return;
        }

        kc++;
        if (kc > history.count()-1)
            kc = history.count()-1;

        setText(history[kc]);
    }


    else if (e->key() == Qt::Key_Down) {

        if (history.count() == 0) {
            QLineEdit::keyPressEvent(e);
            return;
        }

        kc--;
        if (kc <= -1) {
            clear();
            kc = -1;
        }
        else
            setText(history[kc]);
    }

    else {
        if ((e->key() == Qt::Key_Return) && (text().length() > 0)) /// Note: It is Key_Return and NOT Key_Enter !!
            kc = -1;

        if (e->modifiers() == Qt::ControlModifier) {
            QChar code = 0x00;
            if (e->key() == Qt::Key_K)
                code = 0x03;

            if (e->key() == Qt::Key_B)
                code = 0x02;

            if ((e->key() == Qt::Key_U) || (e->key() == Qt::Key_L))
                code = 0x1F;

            if (code != 0x00) {
                int cpos = cursorPosition();
                QString tx = text().insert(cpos, code);
                setText(tx);
                setCursorPosition(cpos + 1); // Set back, +1 because we added a char, and we want the cursor to be on the right hand side.
                return;
            }
        }
    }

    QLineEdit::keyPressEvent(e);
}

void QMyLineEdit::updateCSS()
{
    QString bg = conf->colInputBackground;
    QString fg = conf->colInput;

    setStyleSheet( QString("background-color: %1; color: %2;")
                     .arg(bg)
                     .arg(fg)
                  );
}

QString QMyLineEdit::acPhrase()
{
    QString phrase;
    for (int i = cursorPosition()-1; i >= 0; i--) {
        QChar c = text()[i];
        if (c == ' ')
            break;
        phrase.prepend(c);
    }

    return phrase;
}

int QMyLineEdit::acBegin()
{
    int i = cursorPosition()-1;
    for (; i >= 0; i--) {
        QChar c = text()[i];
        if (c == ' ')
            break;
    }

    return i+1;
}


QColor QMyLineEdit::getColorFromCode(int num)
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
