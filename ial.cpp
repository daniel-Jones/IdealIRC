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
#include <QDebug>
#include <QDateTime>
#include <QHashIterator>

#include "ial.h"
#include "iconnection.h"
#include "script/tscriptparent.h"

IAL::IAL(IConnection *parent, QString *activeNickname, QList<char> *sortingRule, TScriptParent *sp) :
    QObject(parent),
    activeNick(activeNickname),
    sortrule(sortingRule),
    scriptParent(sp),
    connection(parent)
{
    connect(&garbageTimer, SIGNAL(timeout()),
            this, SLOT(cleanGarbage()));
    garbageTimer.setInterval(60000); // every 1 min
    garbageTimer.start();
}

/*!
 * Resets IAL.
 */
void IAL::reset()
{
    entries.clear();
}

/*!
 * Adds a nickname to IAL.
 */
void IAL::addNickname(QString nickname)
{
    if (! entries.contains(nickname))
        entries.insert(nickname, new IALEntry_t);

    garbage.removeAll(nickname);
}

/*!
 * Deletes a nickname from IAL. All its data will be removed.
 */
void IAL::delNickname(QString nickname)
{
    IALEntry_t *entry = getEntry(nickname);
    if (entry == NULL)
        return;

    if (nickname == *activeNick)
        return;

    // Remove from channels
    QListIterator<IALChannel_t*> i(entry->channels);
    while (i.hasNext())
        delete i.next();
    entry->channels.clear();

    // Mark age for when this one should be deleted (5 mins)
    entry->age = QDateTime::currentMSecsSinceEpoch() / 1000;
    garbage << nickname;
}

/*!
 * \return true if IAL contains the nickname.
 */
bool IAL::hasNickname(QString nickname)
{
    return entries.contains(nickname);
}

/*!
 * Sets a hostname for the nickname.
 * \return false if nickname doesn't exist.
 */
bool IAL::setHostname(QString nickname, QString hostname)
{
    IALEntry_t *entry = getEntry(nickname);
    if (entry == NULL)
        return false;

    QString entryhost = entry->hostname;

    entry->hostname = hostname;

    if (entryhost != hostname) { // Handle events on host change or setting new
        scriptParent->runevent(te_ialhostget, QStringList()<<nickname<<hostname);

        for (int i = 0; i <= banSet.count()-1; i++) {
            QStringList param = banSet[i].split(' ');
            if (param[1].toUpper() == nickname.toUpper()) {
                connection->sockwrite(QString("MODE %1 +b %2")
                                      .arg(param[0])
                                      .arg(hostname));
            }
        }
    }
    return true;
}

/*!
 * Sets ident for nickname.
 * \return false if nickname doesn't exist.
 */
bool IAL::setIdent(QString nickname, QString ident)
{
    IALEntry_t *entry = getEntry(nickname);
    if (entry == NULL)
        return false;

    entry->ident = ident;
    return true;
}

/*!
 * Sets a new nickname. All data will be moved over.
 * \return false if nickname doesn't exist.
 */
bool IAL::setNickname(QString nickname, QString newNickname)
{
    IALEntry_t *entry = getEntry(nickname);
    if (entry == NULL)
        return false;

    entries.remove(nickname);
    entries.insert(newNickname, entry);
    return true;
}

/*!
 * Adds a channel to the nickname.
 * \return false if nickname doesn't exist.
 */
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

/*!
 * Removes a channel from the nickname.
 * \return false if nickname doesn't exist or isn't on that channel.
 */
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

    // No channels left, mark for deletion.
    if (entry->channels.isEmpty())
        delNickname(nickname); // If it is us, delNickname() ignores it.

    return true;
}

/*!
 * Adds a mode (@, +, etc) on the nickname in a given channel.
 * \return false if nickname doesn't exist or isn't on that channel.
 */
bool IAL::addMode(QString nickname, QString channel, char mode)
{
    IALChannel_t *chanEntry = getChannel(nickname, channel);
    if (chanEntry == NULL)
        return false;

    chanEntry->modeChar.append(mode);
    sortList(&chanEntry->modeChar);

    return true;
}

/*!
 * Removes a mode from the nickname in a given channel.
 * \return false if nickname doesn't exist or isn't on that channel.
 */
bool IAL::delMode(QString nickname, QString channel, char mode)
{
    IALChannel_t *chanEntry = getChannel(nickname, channel);
    if (chanEntry == NULL)
        return false;

    chanEntry->modeChar.removeAll(mode);
    return true;
}

/*!
 * Removes all modes from the nickname in a given channel.
 * \return false if nickname doesn't exist or isn't on that channel.
 */
bool IAL::resetModes(QString nickname, QString channel)
{
    IALChannel_t *chanEntry = getChannel(nickname, channel);
    if (chanEntry == NULL)
        return false;

    chanEntry->modeChar.clear();
    return true;
}

/*!
 * Returns a list with all nicknames in a given channel.\n
 * Remember to delete the list after use.
 * \return Pointer to a QStringList or an empty (pointer to) list on error.
 */
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

/*!
 * Returns a list with all nicknames IAL have registered.\n
 * Remember to delete the list after use.
 * \return Pointer to a QStringList or an empty (pointer to) list on error.
 */
QStringList* IAL::getAllNicknames()
{
    QStringList *result = new QStringList();
    QHashIterator<QString,IALEntry_t*> i(entries);
    while (i.hasNext())
        result->push_back(i.next().key());
    return result;
}

/*!
 * Returns a list of channels we've registered nickname to be on.\n
 * This doesn't mean all channels it actually is on, but rather the channels we share.\n
 * Remember to delete the list after use.
 * \return Pointer to a QStringList or an empty (pointer to) list on error.
 */
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

/*!
 * Returns the ident of a nickname.\n
 * Returns empty if nickname wasn't found or we haven't registered anything on it.
 * \return Ident, or empty on error.
 */
QString IAL::getIdent(QString nickname)
{
    IALEntry_t* entry = getEntry(nickname);
    if (entry == NULL)
        return "";

    return entry->ident;
}
/*!
 * Returns the hostname of a nickname.\n
 * Returns empty if nickname wasn't found or we haven't registered anything on it.
 * \return Hostname, or empty on error.
 */
QString IAL::getHost(QString nickname)
{
    IALEntry_t* entry = getEntry(nickname);
    if (entry == NULL)
        return "";

    return entry->hostname;
}

/*!
 * \param nickname The nickname.
 * \param channel The channel.
 * \param cs Case sensitivity. If true, it performs a faster lookup but may be more inaccurate.
 *
 * Returns a list of all (registered) modes on a nickname in a channel.\n
 * Note that there's a bug in which when we join a channel, an user may have Operator status,
 * but may also have voice status but we won't know that. At all. (Is this bug fixed in the IRC protocol? need to figure this out.)\n
 * Returns empty if nickname wasn't found or we haven't registered anything on it.
 * \return A QList<char> of mode letters. Can be empty.
 */
QList<char> IAL::getModeChars(QString nickname, QString channel, bool cs)
{
    IALChannel_t *chanEntry = getChannel(nickname, channel, cs);
    if (chanEntry == NULL)
        return QList<char>();

    return chanEntry->modeChar;
}

/*!
 * \return true if nickname is on the specified channel.
 */
bool IAL::sharesChannel(QString nickname, QString channel)
{
    IALChannel_t *chan = getChannel(nickname, channel, false);
    if (chan == NULL)
        return false;
    return true;
}

/*!
 * \return true if nickname have Operator (@) status on the specified channel.
 */
bool IAL::isOperator(QString nickname, QString channel)
{
    QList<char> modes = getModeChars(nickname, channel, false);
    return modes.contains('@');
}

/*!
 * \return true if nickname have Half-operator (%) status on the specified channel.
 */
bool IAL::isHalfop(QString nickname, QString channel)
{
    QList<char> modes = getModeChars(nickname, channel, false);
    return modes.contains('%');
}

/*!
 * \return true if nickname have Voiced (+) status on the specified channel.
 */
bool IAL::isVoiced(QString nickname, QString channel)
{
    QList<char> modes = getModeChars(nickname, channel, false);
    return modes.contains('+');
}

/*!
 * \return true if nickname have no status on the specified channel.
 */
bool IAL::isRegular(QString nickname, QString channel)
{
    QList<char> modes = getModeChars(nickname, channel, false);
    return modes.isEmpty();
}

/*!
 * \return amount of users in a specified channel.
 */
int IAL::userCount(QString channel)
{
    QHash<QString,int> chanList; // when filled out, a precise table of usercount per channel
    QHashIterator<QString,IALEntry_t*> i(entries);
    while (i.hasNext()) {
        IALEntry_t *entry = i.next().value(); // we dont need nickname

        QListIterator<IALChannel_t*> iu(entry->channels);
        while (iu.hasNext()) {
            IALChannel_t *c = iu.next();
            int count = chanList.value(c->name.toUpper(), 0);
            chanList.insert(c->name.toUpper(), count+1);
        }
    }

    return chanList.value(channel.toUpper(), 0);
}

/*!
 * Attempts to retreive hostname from nickname and put ban on it in specified channel.\n
 * If IAL don't have registered any hostname on the nickname, it will postpone the ban
 * and ask the IRC server for the hostname. When received, the ban will be set.
 */
void IAL::setChannelBan(QString channel, QString nickname)
{
    QString hostname = getHost(nickname);

    if (! hostname.isEmpty()) {
        connection->sockwrite( QString("MODE %1 +b %2")
                                 .arg(channel)
                                 .arg(hostname)
                              );

        return;
    }

    banSet << QString("%1 %2")
              .arg(channel)
              .arg(nickname);

    connection->sockwrite(QString("USERHOST %1").arg(nickname));
}

/*!
 * \param nickname The nickname.
 * \param cs Case sensitive, default to true. If set to true, it looks up faster, but may be more inaccurate.
 * \return An IAL entry for specified nickname. NULL if there's an error.
 */
IALEntry_t* IAL::getEntry(QString nickname, bool cs)
{
    if (cs) // case sensitive, faster
        return entries.value(nickname, NULL);

    QHashIterator<QString,IALEntry_t*> i(entries);
    while (i.hasNext()) {
        i.next();
        IALEntry_t *r = i.value();
        if (i.key().toUpper() == nickname.toUpper())
            return r;
    }

    return NULL;
}

/*!
 * \param nickname The nickname.
 * \param channel The channel.
 * \param cs Case sensitive, default to true. If set to true, it looks up faster, but may be more inaccurate.
 * \return The IAL Channel entry for nickname. NULL if there's an error.
 */
IALChannel_t* IAL::getChannel(QString nickname, QString channel, bool cs)
{
    IALEntry_t* entry = getEntry(nickname, cs);
    if (entry == NULL)
        return NULL;

    QListIterator<IALChannel_t*> i(entry->channels);
    while (i.hasNext()) {
        IALChannel_t *c = i.next();

        if (cs)
            if (c->name == channel)
                return c;

        if (! cs)
            if (c->name.toUpper() == channel.toUpper())
                return c;
    }

    return NULL;
}

/*!
 * Sorts a (pointer to) list of chars using insertion method.
 */
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

/*!
 * Used for sortList().\n
 * \return true if s1 comes before s2.
 */
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

/*!
 * Removes old IAL entries that's no longer in use and have no data on it.
 */
void IAL::cleanGarbage()
{
    qint64 current = QDateTime::currentMSecsSinceEpoch() / 1000;

    for (int i = 0; i <= garbage.size()-1; i++) {
        QString name = garbage[i];
        IALEntry_t *entry = entries.value(name, NULL);
        if (entry == NULL)
            continue;

        if  ((current-entry->age) < 300)
            continue; // Too "young"

        entries.remove(name);
    }
}
