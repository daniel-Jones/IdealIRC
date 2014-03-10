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
#include "ial.h"

#include <QDebug>

IAL::IAL(QObject *parent, QString *activeNickname, QList<char> *sortingRule) :
    QObject(parent),
    activeNick(activeNickname),
    sortrule(sortingRule)
{
}

void IAL::reset()
{
    entries.clear();
}

void IAL::addNickname(QString nickname)
{
    if (! entries.contains(nickname))
        entries.insert(nickname, new IALEntry_t);
}

bool IAL::delNickname(QString nickname)
{
    if (entries.remove(nickname) == 0)
        return false;
    else
        return true;
}

bool IAL::hasNickname(QString nickname)
{
    return entries.contains(nickname);
}

bool IAL::setHostname(QString nickname, QString hostname)
{
    IALEntry_t *entry = getEntry(nickname);
    if (entry == NULL)
        return false;

    entry->hostname = hostname;
    return true;
}

bool IAL::setIdent(QString nickname, QString ident)
{
    IALEntry_t *entry = getEntry(nickname);
    if (entry == NULL)
        return false;

    entry->ident = ident;
    return true;
}

bool IAL::setNickname(QString nickname, QString newNickname)
{
    IALEntry_t *entry = getEntry(nickname);
    if (entry == NULL)
        return false;

    entries.remove(nickname);
    entries.insert(newNickname, entry);
    return true;
}

bool IAL::addChannel(QString nickname, QString channel)
{
    IALEntry_t *entry = getEntry(nickname);
    if (entry == NULL)
        return false;

    IALChannel_t *c = getChannel(nickname, channel);
    if (c != NULL) // entry already exist. stop.
        return false;

    c = new IALChannel_t;
    c->name = channel;
    entry->channels.push_back(c);

    return true;
}

bool IAL::delChannel(QString nickname, QString channel)
{
    IALEntry_t *entry = getEntry(nickname);
    if (entry == NULL)
        return false;

    IALChannel_t *c = getChannel(nickname, channel);
    if (c == NULL) // entry doesn't exist. stop.
        return false;

    entry->channels.removeAll(c);
    delete c;

    // No channels left and this isn't us. delete.
    if ((entry->channels.isEmpty()) && (nickname != *activeNick))
        entries.remove(nickname);
    return true;
}

bool IAL::addMode(QString nickname, QString channel, char mode)
{
    IALChannel_t *chanEntry = getChannel(nickname, channel);
    if (chanEntry == NULL)
        return false;

    chanEntry->modeChar.append(mode);
    sortList(&chanEntry->modeChar);

    return true;
}

bool IAL::delMode(QString nickname, QString channel, char mode)
{
    IALChannel_t *chanEntry = getChannel(nickname, channel);
    if (chanEntry == NULL)
        return false;

    chanEntry->modeChar.removeAll(mode);
    return true;
}

bool IAL::resetModes(QString nickname, QString channel)
{
    IALChannel_t *chanEntry = getChannel(nickname, channel);
    if (chanEntry == NULL)
        return false;

    chanEntry->modeChar.clear();
    return true;
}

QStringList* IAL::getNickList(QString channel)
{
    QStringList *result = new QStringList();

    QHashIterator<QString,IALEntry_t*> i(entries);
    while (i.hasNext()) {
        QString nickname = i.next().key();
        IALChannel_t *c = getChannel(nickname, channel);
        if (c == NULL)
            continue;

        if (c->modeChar.length() > 0)
            nickname.prepend( c->modeChar[0] );

        result->push_back(nickname);
    }

    return result;
}

QStringList* IAL::getAllNicknames()
{
    QStringList *result = new QStringList();
    QHashIterator<QString,IALEntry_t*> i(entries);
    while (i.hasNext())
        result->push_back(i.next().key());
    return result;
}

QStringList* IAL::getChannels(QString nickname)
{
    QStringList *result = new QStringList();
    IALEntry_t *entry = getEntry(nickname);
    if (entry == NULL)
        return result;

    QListIterator<IALChannel_t*> i(entry->channels);
    while (i.hasNext()) {
        IALChannel_t *c = i.next();
        result->push_back(c->name);
    }

    return result;
}

QString IAL::getIdent(QString nickname)
{
    IALEntry_t* entry = getEntry(nickname);
    if (entry == NULL)
        return "";

    return entry->ident;
}

QString IAL::getHost(QString nickname)
{
    IALEntry_t* entry = getEntry(nickname);
    if (entry == NULL)
        return "";

    return entry->hostname;
}

QList<char> IAL::getModeChars(QString nickname, QString channel)
{
    IALChannel_t *chanEntry = getChannel(nickname, channel);
    if (chanEntry == NULL)
        return QList<char>();

    return chanEntry->modeChar;
}

IALEntry_t* IAL::getEntry(QString nickname)
{
    return entries.value(nickname, NULL);
}

IALChannel_t* IAL::getChannel(QString nickname, QString channel)
{
    IALEntry_t* entry = getEntry(nickname);
    if (entry == NULL)
        return NULL;

    QListIterator<IALChannel_t*> i(entry->channels);
    while (i.hasNext()) {
        IALChannel_t *c = i.next();
        if (c->name == channel)
            return c;
    }

    return NULL;
}

void IAL::sortList(QList<char> *lst)
{
    for (int i = 0; i < lst->count(); i++) {
        for (int pos = i; ((pos > 0) && sortLargerThan(QString(lst->at(pos-1)), QString(lst->at(pos)))); pos--) {
            QString a = QString(lst->at(pos-1));
            QString b = QString(lst->at(pos));
            lst->replace(pos-1, b[0].toLatin1());
            lst->replace(pos, a[0].toLatin1());
        }
    }
}

bool IAL::sortLargerThan(const QString s1, const QString s2)
{
    // Grab smallest length.
    int l = s1.length();
    if (s2.length() < l)
        l = s2.length();

    for (int i = 0; i <= l-1; i++) {
        int As = sortrule->indexOf(s1[i].toLatin1());
        int Bs = sortrule->indexOf(s2[i].toLatin1());

        if (As < Bs)
            return false;
        if (As > Bs)
            return true;
    }

    // This returns when the texts are equal.
    return false;
}
