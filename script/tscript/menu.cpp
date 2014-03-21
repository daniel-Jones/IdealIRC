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

void TScript::createMenu(int &pos, char type)
{
    if (type == 'n') {
        if (rootNicklistMenu)
            delete rootNicklistMenu;
        rootNicklistMenu = new QAction(this);
    }
    else if (type == 'c') {
        if (rootChannelMenu)
            delete rootChannelMenu;
        rootChannelMenu = new QAction(this);
    }
    else
        return;

    int parent = -1;
    if (parent)
        createMenuIterate(pos, type, parent);
}

void TScript::createMenuIterate(int &pos, char type, int parent)
{
    // do not use this function, use createMenu() instead.

    /*

      menu type {
        item name = function
        submenu name {
          item name = function
        }
      }

*/
    enum {
        st_ItemName = 1,
        st_FunctionName = 2
    };
    QString itemname;
    QString fnctname;
    int state = st_ItemName;
    for (; pos <= scriptstr.length()-1; ++pos) {
        QChar c = scriptstr[pos];

        if (c == '\\') {
            QChar add = '\\';
            if (pos < scriptstr.length()-1) {
                add = scriptstr[pos];
                ++pos;
            }
            if (state == st_ItemName)
                itemname += add;
            if (state == st_FunctionName)
                fnctname += add;
        }

        if (c == '}')
            break;

        if (state == st_ItemName) {
            if (c == '=') {
                if (itemname.endsWith(' '))
                    itemname = itemname.left(itemname.length()-1);
                state = st_FunctionName;
                continue;
            }

            if (c == '{') {
//                QMenu *menu = parent->addMenu(itemname);
                scriptmenu_t st;
                st.action = new QAction(itemname, this);
                st.parent = parent;
                st.haveChildren = true;

                int childParent = -1;

                if (type == 'n') {
                    childParent = customNicklistMenu.count();
                    customNicklistMenu.push_back(st);
                }
                if (type == 'c') {
                    childParent = customChannelMenu.count();
                    customChannelMenu.push_back(st);
                }
                createMenuIterate(++pos, type, childParent);
                continue;
            }

            if (c == '\n') {
                itemname.clear();
                continue;
            }

            itemname += c;
        }

        if (state == st_FunctionName) {
            if (c == ' ')
                continue;
            if (c == '\n') {
                if (type == 'n') { // nicklist
                    scriptmenu_t st;
                    st.action = new QAction(itemname, this);
                    st.parent = parent;
                    st.haveChildren = false;

                    nicklistMenuMapper.setMapping(st.action, fnctname);
                    customNicklistMenu.push_back(st);

                    connect(st.action, SIGNAL(triggered()),
                            &nicklistMenuMapper, SLOT(map()));
                }

                else if (type == 'c') { // channel
                    scriptmenu_t st;
                    st.action = new QAction(itemname, this); //parent->addAction(itemname);
                    st.parent = parent;
                    st.haveChildren = false;

                    channelMenuMapper.setMapping(st.action, fnctname);
                    customChannelMenu.push_back(st);

                    connect(st.action, SIGNAL(triggered()),
                            &channelMenuMapper, SLOT(map()));
                }
                itemname.clear();
                fnctname.clear();
                state = st_ItemName;
                continue;
            }
            fnctname += c;
            continue;
        }
    }
}

void TScript::resetMenu(QList<scriptmenu_t> &menu)
{
    // After using this function a menu should be rebuilt, though nothing will crash if it doesn't.
    QListIterator<scriptmenu_t> i(menu);
    while (i.hasNext()) {
        scriptmenu_t sm = i.next();
        nicklistMenuMapper.removeMappings(sm.action);
        disconnect(sm.action);
        delete sm.action;
    }
    menu.clear();
}

void TScript::nicklistMenuItemTriggered(QString function)
{
    QString r;
    runf(function, scriptParent->getCurrentNickSelection(), r, true);
}

void TScript::channelMenuItemTriggered(QString function)
{
    QString r;
    runf(function, QStringList()<<scriptParent->getCurrentWindow(), r, true);
}
