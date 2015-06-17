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

/*! \class QMyLineEdit
 *  \brief Custom line-edit widget, for auto-complete.
 *
 * This class is to be renamed to ILineEdit, because this is not in any way a class of the Qt library!
 */

/*! \class LineEditStyle
 *  \brief Helper class to QMyLineEdit to change width of text cursor.
 */

#ifndef QMYLINEEDIT_H
#define QMYLINEEDIT_H

#include <QLineEdit>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QProxyStyle>
#include "config.h"

class QMyLineEdit;

class LineEditStyle : public QProxyStyle
{
public:
    LineEditStyle(QMyLineEdit *ptr, QStyle *style = 0) : QProxyStyle(style), editptr(ptr) { }
    int pixelMetric(PixelMetric metric, const QStyleOption *option = 0, const QWidget *widget = 0) const;

private:
    QMyLineEdit *editptr; //!< Pointer to the QMyLineEdit.
};

class QMyLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit QMyLineEdit(QWidget *parent, config *cfg);
    void updateCSS();
    QString acPhrase(); // Return a phrase to match, based on where the text cursor is.
    int acBegin(); // Same as above but returns the first index of where to replace ac text.
    int *acIndex; //!<  Get this pointer from IWin.\n We need it here because whenever a key NOT being TAB_KEY, we must reset its value to zero (not the pointer).
    int cursorSize; //!< Pixel width of the text cursor.

private:
    int kc; //!< Index of previous inputs we're scrolling trough using UP or DOWN arrows.
    QStringList history; //!< History of line inputs
    config *conf; //!< Pointer to config class (iirc.ini)
    QColor getColorFromCode(int num);
    LineEditStyle *style; //!< Helper class, to provide text cursor width.

private slots:
    void lnReturn();

protected:
    void keyPressEvent(QKeyEvent *e);
    void focusInEvent(QFocusEvent *e);
    void focusOutEvent(QFocusEvent *e);

signals:
    void TabKeyPressed();

};

#endif // QMYLINEEDIT_H
