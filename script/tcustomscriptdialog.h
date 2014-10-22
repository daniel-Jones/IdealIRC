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

#ifndef TCUSTOMSCRIPTDIALOG_H
#define TCUSTOMSCRIPTDIALOG_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QListWidget>
#include <QHash>
#include <QStringList>
#include <QSignalMapper>

#include "tsdialog.h"
#include "constants.h"

/*
  Label
  Button
  Editbox
  Textbox
  Image
*/

class TScript;

class TCustomScriptDialog : public QObject
{
  Q_OBJECT

public:
    explicit TCustomScriptDialog(TScript *parent, QString oname, QWidget *dlgParent);
    void showDlg();
    void hideDlg();
    void closeDlg();
    QString getName() { return dialog.objectName(); }
    void setTitle(QString title) { dialog.setWindowTitle(title); }
    void setGeometry(int X, int Y, int W, int H);
    bool addLabel(QString oname, int X, int Y, int W, int H, QString text);
    bool addButton(QString oname, int X, int Y, int W, int H, QString text);
    bool addEditbox(QString oname, int X, int Y, int W, int H);
    bool addTextbox(QString oname, int X, int Y, int W, int H);
    bool addListbox(QString oname, int X, int Y, int W, int H);

    QString getLabel(QString oname);
    QString getItem(QString oname, int pos);

    bool setLabel(QString oname, QString text);
    bool addItem(QString oname, QString text);
    bool reItem(QString oname, int idx, QString text);
    bool delItem(QString oname, int idx);

    bool clear(QString oname);


private:
    TScript *script; // pointer to the script this belongs to
    TSDialog dialog;
    QSignalMapper buttonmap;
    QSignalMapper listmap;
    QHash<QString,QLabel*> label;
    QHash<QString,QPushButton*> button;
    QHash<QString,QLineEdit*> editbox;
    QHash<QString,QTextEdit*> textbox;
    QHash<QString,QListWidget*> listbox;

protected:
    void dialogClosed();

private slots:
    void buttonClicked(QString oname);
    void listSelected(QString oname);
};

#endif // TCUSTOMSCRIPTDIALOG_H
