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

#include "iwindowswitcher.h"
#include <QDebug>

IWindowSwitcher::IWindowSwitcher(QObject *parent) :
    QObject(parent)
{
    connect(&bb, SIGNAL(buttonSwitched(int)),
            this, SIGNAL(windowSwitched(int)));

    connect(&bb, SIGNAL(closeWindow(int)),
            this, SIGNAL(windowClosed(int)));
}

/*!
 * \param wtype Window type (see constants.h for WT_*)
 * \param name Window name
 * \param id Window ID
 * \param parent Parent of window (usually an IConnection)
 *
 * Registers a window to the window switcher.
 */
void IWindowSwitcher::addWindow(int wtype, QString name, int id, int parent)
{
    bb.addButton(parent, id, wtype, name);
}

/*!
 * \param wid Window ID
 *
 * Unregisters a window from the window switcher.
 */
void IWindowSwitcher::delWindow(int wid)
{
    bb.delButton(wid);
}

void IWindowSwitcher::delGroup(int group)
{
    bb.delGroup(group);
}

/*!
 * \param wid Window ID
 * \param type Highlight type
 *
 * Sets highlight on specified window.\n
 * See constants.h for HL_*
 */
void IWindowSwitcher::setHighlight(int wid, int type)
{
    bb.highlight(wid, type);
}

/*!
 * \param wid Window ID
 * \param title Window title
 *
 * Sets new caption on window switchers
 */
void IWindowSwitcher::setTitle(int wid, QString title)
{
    bb.setTitle(wid, title);
}

/*!
 * \param wid Window ID
 *
 * Selects the given Window ID to be active.
 */
void IWindowSwitcher::setActiveWindow(int wid)
{
    bb.setActive(wid);
}
