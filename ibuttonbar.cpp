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

#include <QListIterator>
#include "ibuttonbar.h"
#include "constants.h"

IButtonBar::IButtonBar(QObject *parent) :
    QObject(parent),
    flashtoggle(false),
    activewid(-1),
    actionTitle(&menu),
    actionClose(&menu)
{
    toolbar.setWindowTitle("Buttonbar");
    toolbar.setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar.setIconSize( QSize(16, 16) );
    toolbar.setContextMenuPolicy(Qt::CustomContextMenu);

    flasher.setInterval(500);
    connect(&flasher, SIGNAL(timeout()),
            this, SLOT(onFlasher()));

    connect(&toolbar, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(contextMenu(QPoint)));

    actionTitle.setDisabled(true);
    QFont titlefont = actionTitle.font();
    titlefont.setWeight(QFont::Bold);
    actionTitle.setFont(titlefont);
    actionClose.setText(tr("Close"));

    menu.addAction(&actionTitle);
    menu.addSeparator();
    menu.addAction(&actionClose);

    connect(&actionClose, SIGNAL(triggered()),
            this, SLOT(actionCloseTrigger()));

    flasher.start();
    ConnectSignals;
}

/*!
 * \param group Group ID, same as connection ID. Scripted windows will be group -1.
 * \param wid Window ID.
 * \param wtype Window type. See constants.h WT_*
 * \param name Window name, also caption of button.
 *
 * Adds a button to the button toolbar.
 */
void IButtonBar::addButton(int group, int wid, int wtype, QString name)
{
    // Adds a button to group.
    button_t b;
    b.button = new QAction(name, &toolbar);
    b.group = group;
    b.wid = wid;
    b.wtype = wtype;

    b.button->setObjectName(name);

    action_data_t ad = {wid, HL_NONE};

    setActionData(b.button, ad);

    // Add icon
    QString ico = ":/window/gfx/custom.png"; // Default to this.
    if (wtype == WT_PRIVMSG)
      ico = ":/window/gfx/query_noact.png";
    if (wtype == WT_CHANNEL)
      ico = ":/window/gfx/channel_noact.png";
    if (wtype == WT_STATUS)
      ico = ":/window/gfx/status_noact.png";
    b.button->setIcon(QIcon(ico));
    b.button->setCheckable(true);

    buttons << b;

    sigmap.setMapping(b.button, wid);
    connect(b.button, SIGNAL(toggled(bool)),
            &sigmap, SLOT(map()));

    rebuild();
}

/*!
 * \param wid Window ID
 *
 * Removes a button from the button toolbar.
 */
void IButtonBar::delButton(int wid)
{
    // Deletes a button
    int idx = -1;
    QListIterator<button_t> i(buttons);
    while (i.hasNext()) {
        button_t b = i.next();
        ++idx;

        if (wid != b.wid)
            continue;

        toolbar.removeAction(b.button);
        delete b.button;
        buttons.removeAt(idx);

        return;
    }
}

/*!
 * \param group Group ID.
 *
 * Removes all buttons inside specified group.\n
 * This is used when, for example, a status window is closed. Then we want to remove all its related windows.
 */
void IButtonBar::delGroup(int group)
{
    // Delete all buttons from group
    int idx = -1;
    QList<int> idxList;
    {
        QListIterator<button_t> i(buttons);
        while (i.hasNext()) {
            button_t b = i.next();
            ++idx;

            if ((b.wtype == WT_STATUS) && (b.wid == group)) {
                toolbar.removeAction(b.button);
                delete b.button;
                idxList << idx; // mark for deletion
                continue;
            }

            if (group != b.group)
                continue;

            toolbar.removeAction(b.button);
            delete b.button;
            idxList << idx; // mark for deletion
        }
    }

    // delete marked items
    QListIterator<int> i(idxList);
    while (i.hasNext()) {
        int index = i.next();
        buttons.removeAt(index);
    }
}

/*!
 * \param wid Window ID
 * \param title New title
 *
 * Sets a new text on the specified button.
 */
void IButtonBar::setTitle(int wid, QString title)
{
    QAction *a = getAction(wid);
    if (a == nullptr)
        return;

    a->setText(title);
}

/*!
 * \param wid Window ID
 *
 * Programatically changes the active button.\n
 * This is useful when we switch to other windows not using the buttons, for example
 * when we click between windows or using the tree view.
 */
void IButtonBar::setActive(int wid)
{
    DisconnectSignals;

    if (activewid != -1) {
        // deactivate current button
        QAction *a = getAction(activewid);
        if (a != nullptr)
            a->setChecked(false);
    }

    // activate new button
    QAction *a = getAction(wid);
    if (a != nullptr)
        a->setChecked(true);

    activewid = wid;
    highlight(wid, HL_NONE);

    ConnectSignals;
}

/*!
 * \param wid Window ID.
 * \param type Highlight Type. See constants.h for HL_*
 *
 * Sets a highlight on specified button.\n
 * This can be for small activity, messages or someone saying our nickname.
 */
void IButtonBar::highlight(int wid, int type)
{
    /*
        HL_NONE         No activity (regular icon 50% opacity)
        HL_ACTIVITY     Activity join, part, etc (regular icon no opacity)
        HL_MSG          Messages, ctcp (MSG icon for window)
        HL_HIGHLIGHT    Highlighted text (MSG icon for window, flashing)
    */

    QAction *a = getAction(wid);
    if (a == nullptr)
        return;

    action_data_t ad = getActionData(a);

    // clang is being good:
    if (activewid == wid) {
        ad.wid = wid;
        ad.highlight = HL_NONE;
    }
    else {
        ad.wid = wid;
        ad.highlight = type;
    }

    /* clang is being bad:
    if (activewid == wid)
        ad = {wid, HL_NONE};
    else
        ad = {wid, type};
        */
    setActionData(a, ad);

    int wtype = getWtype(wid);
    if (wtype == WT_CHANNEL) {
        switch (type) {

            case HL_ACTIVITY:
                a->setIcon( QIcon(":/window/gfx/channel.png") );
                break;

            case HL_MSG:
                a->setIcon( QIcon(":/window/gfx/channel_act.png") );
                break;

            case HL_HIGHLIGHT:
                a->setIcon( QIcon(":/window/gfx/channel_act.png") );
                break;

            default: // covers HL_NONE
                a->setIcon( QIcon(":/window/gfx/channel_noact.png") );
                break;
        }
    }

    if (wtype == WT_PRIVMSG) {
        switch (type) {

            case HL_ACTIVITY:
                a->setIcon( QIcon(":/window/gfx/query.png") );
                break;

            case HL_MSG:
                a->setIcon( QIcon(":/window/gfx/query_act.png") );
                break;

            case HL_HIGHLIGHT:
                a->setIcon( QIcon(":/window/gfx/query_act.png") );
                break;

            default: // covers HL_NONE
                a->setIcon( QIcon(":/window/gfx/query_noact.png") );
                break;
        }
    }

    if (wtype == WT_STATUS) {
        switch (type) {

            case HL_ACTIVITY:
                a->setIcon( QIcon(":/window/gfx/status.png") );
                break;

            case HL_MSG:
                a->setIcon( QIcon(":/window/gfx/status_act.png") );
                break;

            case HL_HIGHLIGHT:
                a->setIcon( QIcon(":/window/gfx/status_act.png") );
                break;

            default: // covers HL_NONE
                a->setIcon( QIcon(":/window/gfx/status_noact.png") );
                break;
        }
    }
}

/*!
 * \param wid Window ID
 *
 * Returns the Window Type of a given Window ID.\n
 * See constants.h for WT_*
 * \return Integer of window type.
 */
int IButtonBar::getWtype(int wid)
{
    QListIterator<button_t> i(buttons);
    while (i.hasNext()) {
        button_t b = i.next();
        if (b.wid != wid)
            continue;

        return b.wtype;
    }

    return WT_NOTHING;
}

/*!
 * \param a Button pointer
 *
 * Returns the custom data set for specified button.\n
 * \return action_data_t
 */
action_data_t IButtonBar::getActionData(QAction *a)
{
    QByteArray data = a->data().toByteArray();
    action_data_t ad;
    memcpy((char*)&ad, data.data(), sizeof(action_data_t));
    return ad;
}

/*!
 * \param a Button pointer
 * \param ad Data
 *
 * Sets custom data for specified button.
 */
void IButtonBar::setActionData(QAction *a, action_data_t ad)
{
    char data[sizeof(action_data_t)];
    memcpy(data, (char*)&ad, sizeof(action_data_t));

    QByteArray ba;
    ba.insert(0, data, sizeof(action_data_t));

    a->setData(ba);
}

/*!
 * Resets and rebuilds the entire button bar.\n
 * Loops through the groups, gathers the buttons and sorts them.
 */
void IButtonBar::rebuild()
{ // todo: This function should be rewritten

    // Rebuild buttons on toolbar

    // Remove all actions
    {
        QListIterator<QAction*> i(toolbar.actions());
        while (i.hasNext()) {
            QAction *a = i.next();
            toolbar.removeAction(a);
        }
    }

    // Find all group numbers
    QList<int> group;
    {
        QListIterator<button_t> i(buttons);
        while (i.hasNext()) {
            button_t b = i.next();

            if (! group.contains(b.group))
                group << b.group;
        }
    }

    // Get all buttons sorted by groups
    QListIterator<int> gi(group);
    while (gi.hasNext()) {
        int g = gi.next();

        QListIterator<button_t> i(buttons);
        while (i.hasNext()) {
            button_t b = i.next();

            if ((b.wtype == WT_STATUS) && (b.wid == g)) {
                toolbar.addAction(b.button);
                continue;
            }

            if (b.group != g)
                continue;

            toolbar.addAction(b.button);
        }

        if (gi.hasNext())
            toolbar.addSeparator();

    }
}

/*!
 * \param wid Window ID
 *
 * Finds and returns the button (QAction pointer) based on window id.
 * \return On success: Pointer to the QAction, On failure: nullptr
 */
QAction* IButtonBar::getAction(int wid)
{
    QListIterator<button_t> i(buttons);
    while (i.hasNext()) {
        button_t b = i.next();
        if (b.wid != wid)
            continue;

        return b.button;
    }

    return nullptr;
}

/*!
 * \param wid Window ID
 *
 * Finds and selects the given button based on window id.
 */
void IButtonBar::buttonSelected(int wid)
{
    QAction *a = getAction(wid);
    if (a->isChecked() == false) {
        DisconnectSignals;
        a->setChecked(true);
        ConnectSignals;
    }

    highlight(wid, HL_NONE); // reset.
    emit buttonSwitched(wid);
}

/*!
 * Timer slot that flashes all window icons set to highlight as such.
 */
void IButtonBar::onFlasher()
{
    flashtoggle = !flashtoggle;

    QListIterator<button_t> i(buttons);
    while (i.hasNext()) {
        button_t b = i.next();

        action_data_t ad = getActionData(b.button);
        if (ad.highlight != HL_HIGHLIGHT)
            continue; // HL type isn't highlight, ignore.

        QIcon icon = QIcon(":/window/gfx/generic.png");

        if (flashtoggle) {
            if (b.wtype == WT_CHANNEL)
                icon = QIcon(":/window/gfx/channel_act.png");

            if (b.wtype == WT_PRIVMSG)
                icon = QIcon(":/window/gfx/query_act.png");

            if (b.wtype == WT_STATUS)
                icon = QIcon(":/window/gfx/status_act.png");

        }
        else {
            if (b.wtype == WT_CHANNEL)
                icon = QIcon(":/window/gfx/channel.png");

            if (b.wtype == WT_PRIVMSG)
                icon = QIcon(":/window/gfx/query.png");

            if (b.wtype == WT_STATUS)
                icon = QIcon(":/window/gfx/status.png");

        }

        b.button->setIcon(icon);
    }
}

/*!
 * \param pos Local position
 *
 * Attempts to find a given button (QAtion pointer) at a given coordinate.\n
 * If found, its context menu will popup at the mouse coordinate.
 */
void IButtonBar::contextMenu(QPoint pos)
{
    QAction *action = toolbar.actionAt(pos);
    if (action == nullptr)
        return;

    action_data_t ad = getActionData(action);

    actionTitle.setText(action->text());
    actionTitle.setData((int)ad.wid);

    QPoint gpos = toolbar.mapToGlobal(pos);
    menu.popup(gpos);
}
