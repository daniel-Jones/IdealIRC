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

#include "../tscript.h"
#include "constants.h"

bool TScript::customDialogShow(QString oname)
{
    QHashIterator<QString,TCustomScriptDialog*> i(dialogs);
    while (i.hasNext()) {
        i.next();
        TCustomScriptDialog *dlg = i.value();
        if (dlg->getName().toUpper() == oname.toUpper()) {
            dlg->showDlg();
            return true;
        }
    }

    return false;
}

bool TScript::customDialogHide(QString oname)
{
    QHashIterator<QString,TCustomScriptDialog*> i(dialogs);
    while (i.hasNext()) {
        i.next();
        TCustomScriptDialog *dlg = i.value();
        if (dlg->getName().toUpper() == oname.toUpper()) {
            dlg->hideDlg();
            return true;
        }
    }

    return false;
}

bool TScript::customDialogClose(QString oname)
{
    QHashIterator<QString,TCustomScriptDialog*> i(dialogs);
    while (i.hasNext()) {
        i.next();
        TCustomScriptDialog *dlg = i.value();
        if (dlg->getName().toUpper() == oname.toUpper()) {
            dlg->closeDlg();
            return true;
        }
    }

    return false;
}

bool TScript::customDialogSetLabel(QString dlg, QString oname, QString text)
{
    QHashIterator<QString,TCustomScriptDialog*> i(dialogs);
    while (i.hasNext()) {
        i.next();
        TCustomScriptDialog *d = i.value();
        if (d->getName().toUpper() == dlg.toUpper())
            return d->setLabel(oname, text);
    }

    return false;
}

bool TScript::customDialogAddItem(QString dlg, QString oname, QString text)
{
    QHashIterator<QString,TCustomScriptDialog*> i(dialogs);
    while (i.hasNext()) {
        i.next();
        TCustomScriptDialog *d = i.value();
        if (d->getName().toUpper() == dlg.toUpper())
            return d->addItem(oname, text);
    }

    return false;
}

bool TScript::customDialogReItem(QString dlg, QString oname, QString index, QString text)
{
    QHashIterator<QString,TCustomScriptDialog*> i(dialogs);
    while (i.hasNext()) {
        i.next();
        TCustomScriptDialog *d = i.value();
        if (d->getName().toUpper() == dlg.toUpper())
            return d->reItem(oname, index.toInt(), text);
    }

    return false;
}

bool TScript::customDialogDelItem(QString dlg, QString oname, QString index)
{
    QHashIterator<QString,TCustomScriptDialog*> i(dialogs);
    while (i.hasNext()) {
        i.next();
        TCustomScriptDialog *d = i.value();
        if (d->getName().toUpper() == dlg.toUpper()) {
            bool ok = false;
            int idx = index.toInt(&ok);
            if (! ok)
              return false;

            return d->delItem(oname, idx);
        }
    }

    return false;
}

bool TScript::customDialogClear(QString dlg, QString oname)
{
    QHashIterator<QString,TCustomScriptDialog*> i(dialogs);
    while (i.hasNext()) {
        i.next();
        TCustomScriptDialog *d = i.value();
        if (d->getName().toUpper() == dlg.toUpper())
            return d->clear(oname);
    }

    return false;
}
