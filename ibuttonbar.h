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

/*! \class IButtonBar
 *  \brief The button bar which is used to switch between windows, similar to the tree view.
 *
 * Bound to the IWindowSwitcher, the IButtonBar class holds the QToolBar widget which is visible in the GUI.
 */

#ifndef IBUTTONBAR_H
#define IBUTTONBAR_H

#include <QObject>
#include <QToolBar>
#include <QAction>
#include <QList>
#include <QSignalMapper>
#include <QTimer>
#include <QContextMenuEvent>
#include <QDebug>
#include <QMenu>
#include "constants.h"

#define DisconnectSignals disconnect(&sigmap, SIGNAL(mapped(int)), this, SLOT(buttonSelected(int)))
#define ConnectSignals connect(&sigmap, SIGNAL(mapped(int)), this, SLOT(buttonSelected(int)))

/*!
 * Used in IButtonBar.\n
 * This structure holds the actual button pointer and some relevant data for it.
 */
typedef struct T_BUTTON
{
    QAction *button; //!< Pointer to button. The data() of this contains highlight info, in type of action_data_t
    int group; //!< Group the button is in, a connection ID.
    int wid; //!< Window ID
    int wtype; //!< Window type
} button_t;

/*!
 * Used in QAction for their data.\n
 */
typedef struct T_ACTION_DATA
{
    int wid; //!< Window ID
    int highlight; //!< Highlight type. See constants.h for HL_*
} action_data_t;

class IButtonBar : public QObject
{
    Q_OBJECT
public:
    explicit IButtonBar(QObject *parent = 0);
    QToolBar* getToolbar() { return &toolbar; }
    void addButton(int group, int wid, int wtype, QString name);
    void delButton(int wid);
    void delGroup(int group);
    void setTitle(int wid, QString title);
    void setActive(int wid);
    void highlight(int wid, int type = HL_NONE);
    int getWtype(int wid); // returns WT_NOTHING on error

private:
    void rebuild();
    QAction* getAction(int wid);
    action_data_t getActionData(QAction *a);
    void setActionData(QAction *a, action_data_t ad);

    QToolBar toolbar; //!< The visible widget in the GUI.
    QList<button_t> buttons; //!< List of all the buttons.
    QSignalMapper sigmap; //<! Signal map for QAction Toggle event (when a button is clicked)
    QTimer flasher; //!< Timer for highlighted messages, 500ms cycle.
    bool flashtoggle; //!< On/off for flashing icon (highlight message)
    int activewid; //!< The active button window ID.

    // Context menu and its actions
    // Todo: Move menu and menu handling to IWindowSwitcher when tree model is added there
    QMenu menu; //!< Context menu (right click a button)
    QAction actionTitle; //!< First menu item, window title in bold text.
    QAction actionClose; //!< Close menu item.

private slots:
    void buttonSelected(int wid);
    void onFlasher();
    void contextMenu(QPoint pos);
    void actionCloseTrigger() { emit closeWindow( actionTitle.data().toInt() ); }

signals:
    void buttonSwitched(int wid);
    void closeWindow(int wid);
};

#endif // IBUTTONBAR_H
