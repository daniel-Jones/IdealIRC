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

/*! \class IWindowSwitcher
 *  \brief Contains methods of programatically switching between windows.
 *
 * This class is merely a proxy to IButtonBar for now, but
 * it is planned to make a Model for the TreeWidget (making it a TreeView) for window tree view,
 * and place it inside here to make this class handle all window switching and alike.
 */

#ifndef IWINDOWSWITCHER_H
#define IWINDOWSWITCHER_H

#include <QObject>
#include "ibuttonbar.h"
#include "constants.h"

class IWindowSwitcher : public QObject
{
    Q_OBJECT
public:
    explicit IWindowSwitcher(QObject *parent = 0);
    void addWindow(int wtype, QString name, int id, int parent);
    void delWindow(int wid);
    void delGroup(int group);
    QToolBar* getToolbar() { return bb.getToolbar(); } //!< Returns a pointer to the buttonbar toolbar.
    void setHighlight(int wid, int type = HL_NONE);
    void setTitle(int wid, QString title);
    void setActiveWindow(int wid);

private:
    IButtonBar bb; //!< The buttonbar.

signals:
    int windowSwitched(int wid);
    void windowClosed(int wid);
};

#endif // IWINDOWSWITCHER_H
