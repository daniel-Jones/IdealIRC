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

#ifndef QMYLINEEDIT_H
#define QMYLINEEDIT_H

#include <QLineEdit>
#include <QKeyEvent>
#include <QFocusEvent>
#include "config.h"

class QMyLineEdit : public QLineEdit
{
    Q_OBJECT
  public:
    explicit QMyLineEdit(QWidget *parent, config *cfg);
    void updateCSS();
    QString acPhrase(); // Return a phrase to match, based on where the text cursor is.
    int acBegin(); // Same as above but returns the first index of where to replace ac text.
    int *acIndex; // Get this one from IWin. We need it here because whenever
                    // a key NOT being TAB_KEY, we must reset it to zero.
                    // Should not be unsafe to have it as public either.

  private:
    int kc;
    QStringList history; // History of line inputs
    config *conf;
    QColor getColorFromCode(int num);

  private slots:
    void lnReturn();

  protected:
    void keyPressEvent(QKeyEvent *e);
    void focusOutEvent(QFocusEvent *e);

  signals:
    void TabKeyPressed();

};

#endif // QMYLINEEDIT_H
