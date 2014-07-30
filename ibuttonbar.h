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

typedef struct T_BUTTON
{
    QAction *button; // data of these contains Highlight info
    int group;
    int wid;
    int wtype;
} button_t;

typedef struct T_ACTION_DATA
{
    int wid;
    int highlight; // HL_* type
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

    QToolBar toolbar;
    QList<button_t> buttons;
    QSignalMapper sigmap; // signal map for QAction toggle event (buttons)
    QTimer flasher; // for highlighted msgs, 500ms cycle
    bool flashtoggle; // on/off for flashing icon (highlight message)
    int activewid; // active button wid

    // Context menu and its actions
    // Todo: Move menu and menu handling to IWindowSwitcher when tree model is added there
    QMenu menu;
    QAction actionTitle;
    QAction actionClose;

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
