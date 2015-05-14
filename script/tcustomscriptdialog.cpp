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

#include "tcustomscriptdialog.h"
#include "tscript.h"
#include <iostream>

TCustomScriptDialog::TCustomScriptDialog(TScript *parent, QString oname, QWidget *dlgParent) :
  QObject(parent),
  script(parent)
{
    dialog.setParent(dlgParent, Qt::Tool);
    dialog.setObjectName(oname);
    dialog.setMaximumSize(200, 150);
    dialog.setMinimumSize(200, 150);
    dialog.setGeometry(200, 200, 200, 50);

    connect(&buttonmap, SIGNAL(mapped(QString)),
            this, SLOT(buttonClicked(QString)));

    connect(&listmap, SIGNAL(mapped(QString)),
            this, SLOT(listSelected(QString)));

    //IWin test(parent->getDlgParent(), QString("@test"), WT_GRAPHIC, parent->getConfPtr(), parent->getScriptParent());
}

void TCustomScriptDialog::showDlg()
{
    dialog.show();
}

void TCustomScriptDialog::hideDlg()
{
    dialog.hide();
}

void TCustomScriptDialog::closeDlg()
{
    dialog.close();
}

void TCustomScriptDialog::dialogClosed()
{

}

void TCustomScriptDialog::setGeometry(int X, int Y, int W, int H)
{
    dialog.setMaximumSize(W, H);
    dialog.setMinimumSize(W, H);
    dialog.setGeometry(X, Y, W, H);
}

bool TCustomScriptDialog::addLabel(QString oname, int X, int Y, int W, int H, QString text)
{
    if (label.contains(oname.toUpper()))
        return false;

    QLabel *l = new QLabel(text, &dialog);
    l->setGeometry(X, Y, W, H);
    l->setObjectName(oname);

    label.insert(oname, l);

    return true;
}

bool TCustomScriptDialog::addButton(QString oname, int X, int Y, int W, int H, QString text)
{
    if (button.contains(oname.toUpper()))
        return false;

    QPushButton *b = new QPushButton(text, &dialog);
    b->setGeometry(X, Y, W, H);
    b->setObjectName(oname);

    button.insert(oname, b);
    buttonmap.setMapping(b, oname);
    connect(b, SIGNAL(clicked()),
            &buttonmap, SLOT(map()));

    return true;
}

bool TCustomScriptDialog::addEditbox(QString oname, int X, int Y, int W, int H)
{
    if (editbox.contains(oname.toUpper()))
        return false;

    QLineEdit *l = new QLineEdit(&dialog);
    l->setGeometry(X, Y, W, H);
    l->setObjectName(oname);

    editbox.insert(oname, l);

    return true;
}

bool TCustomScriptDialog::addTextbox(QString oname, int X, int Y, int W, int H)
{
    if (textbox.contains(oname.toUpper()))
        return false;

    QLineEdit *t = new QLineEdit(&dialog);
    t->setGeometry(X, Y, W, H);
    t->setObjectName(oname);

    editbox.insert(oname, t);

    return true;
}

bool TCustomScriptDialog::addListbox(QString oname, int X, int Y, int W, int H)
{
    if (listbox.contains(oname.toUpper()))
        return false;

    QListWidget *l = new QListWidget(&dialog);
    l->setGeometry(X, Y, W, H);
    l->setObjectName(oname);

    listbox.insert(oname, l);
    listmap.setMapping(l, oname);

    connect(l, SIGNAL(currentRowChanged(int)),
            &listmap, SLOT(map()));

    return true;
}

void TCustomScriptDialog::buttonClicked(QString oname)
{
    QStringList param;
    param.push_back(dialog.objectName());
    param.push_back(oname);
    script->runEvent(te_dbuttonclick, param);
}

void TCustomScriptDialog::listSelected(QString oname)
{
  int idx = 0;

  QHashIterator<QString,QListWidget*> i(listbox);
  while (i.hasNext()) {
    i.next();
    if (i.value()->objectName().toUpper() == oname.toUpper()) {
      idx = i.value()->currentRow() + 1;
      break;
    }
  }

  QStringList param;
  param.push_back(dialog.objectName());
  param.push_back(oname);
  param.push_back( QString::number(idx) );
  script->runEvent(te_dlistboxselect, param);
}

QString TCustomScriptDialog::getLabel(QString oname)
{
    // Find object in labels...
    {
        QHashIterator<QString,QLabel*> i(label);
        while (i.hasNext()) {
            i.next();
            if (i.value()->objectName().toUpper() == oname.toUpper())
                return i.value()->text();
        }
    }

    // Find object in buttons...
    {
        QHashIterator<QString,QPushButton*> i(button);
        while (i.hasNext()) {
            i.next();
            if (i.value()->objectName().toUpper() == oname.toUpper())
                return i.value()->text();
        }
    }

    // Find object in editbox...
    {
        QHashIterator<QString,QLineEdit*> i(editbox);
        while (i.hasNext()) {
            i.next();
            if (i.value()->objectName().toUpper() == oname.toUpper())
                return i.value()->text();
        }
    }

    // Find object in a listbox (return selected text)
    {
        QHashIterator<QString,QListWidget*> i(listbox);
        while (i.hasNext()) {
            i.next();
            if (i.value()->objectName().toUpper() == oname.toUpper()) {
                if (i.value()->currentItem() != nullptr)
                    return i.value()->currentItem()->text();
                break;
            }
        }
    }

    // Default result
    return "";
}

QString TCustomScriptDialog::getItem(QString oname, int pos)
{
    QHashIterator<QString,QListWidget*> i1(listbox);
    while (i1.hasNext()) {
        i1.next();
        if (i1.value()->objectName().toUpper() == oname.toUpper()) {
            QListWidget *w = i1.value();

            if (pos == -1)
              return QString::number(w->currentRow()+1);

            if (pos == 0)
                return QString::number(w->count());

            if (pos > 0) {
                QListWidgetItem *item = w->item(pos-1);
                if (item == NULL)
                    return "";
                return item->text();
            }
        }
    }



    // Default result
    return "";
}

bool TCustomScriptDialog::setLabel(QString oname, QString text)
{
  QHashIterator<QString,QLabel*> i1(label);
  while (i1.hasNext()) {
      i1.next();
      if (i1.value()->objectName().toUpper() == oname.toUpper()) {
          i1.value()->setText(text);
          return true;
      }
  }

  QHashIterator<QString,QPushButton*> i2(button);
  while (i2.hasNext()) {
      i2.next();
      if (i2.value()->objectName().toUpper() == oname.toUpper()) {
        i2.value()->setText(text);
        return true;
    }
  }

  QHashIterator<QString,QLineEdit*> i3(editbox);
  while (i3.hasNext()) {
      i3.next();
      if (i3.value()->objectName().toUpper() == oname.toUpper()) {
        i3.value()->setText(text);
        return true;
    }
  }

  // Default result
  return false;
}

bool TCustomScriptDialog::addItem(QString oname, QString text)
{
  QHashIterator<QString,QListWidget*> i1(listbox);
  while (i1.hasNext()) {
      i1.next();
      if (i1.value()->objectName().toUpper() == oname.toUpper()) {
          i1.value()->addItem(text);
          return true;
      }
  }

  return false;
}

bool TCustomScriptDialog::reItem(QString oname, int idx, QString text)
{
  QHashIterator<QString,QListWidget*> i1(listbox);
  while (i1.hasNext()) {
      i1.next();
      if (i1.value()->objectName().toUpper() == oname.toUpper()) {
          QListWidgetItem *item = i1.value()->item(idx-1);
          if (item == 0)
              return false;
          item->setText(text);
          return true;
      }
  }

  return false;
}

bool TCustomScriptDialog::delItem(QString oname, int idx)
{
  if (idx < 1)
    return false;

  QHashIterator<QString,QListWidget*> i1(listbox);
  while (i1.hasNext()) {
      i1.next();
      if (i1.value()->objectName().toUpper() == oname.toUpper()) {
          QListWidgetItem *item = i1.value()->takeItem(idx-1);
          delete item;
          return true;
      }
  }

  return false;
}

bool TCustomScriptDialog::clear(QString oname)
{
  QHashIterator<QString,QLabel*> i1(label);
  while (i1.hasNext()) {
      i1.next();
      if (i1.value()->objectName().toUpper() == oname.toUpper()) {
        i1.value()->clear();
        return true;
    }
  }

  QHashIterator<QString,QPushButton*> i2(button);
  while (i2.hasNext()) {
      i2.next();
      if (i2.value()->objectName().toUpper() == oname.toUpper()) {
        i2.value()->setText("");
        return true;
    }
  }

  QHashIterator<QString,QLineEdit*> i3(editbox);
  while (i3.hasNext()) {
      i3.next();
      if (i3.value()->objectName().toUpper() == oname.toUpper()) {
        i3.value()->clear();
        return true;
    }
  }

  QHashIterator<QString,QListWidget*> i4(listbox);
  while (i4.hasNext()) {
      i4.next();
      if (i4.value()->objectName().toUpper() == oname.toUpper()) {
          i4.value()->clear();
          return true;
      }
  }

  // Default result
  return false;
}
