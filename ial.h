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

#ifndef IAL_H
#define IAL_H

#include <QObject>
#include <QHash>
#include <QStringList>
#include <QTimer>

class IConnection;
class TScriptParent;

typedef struct T_IALCHANNEL {
    QString name;
    QList<char> modeChar; // modes the user got in the channel such as @, +
} IALChannel_t;

typedef struct T_IALENTRY {
    // Nickname is stored as Key in the hashtable.
    QString ident;
    QString hostname;
    QList<IALChannel_t*> channels;
    qint64 age;
} IALEntry_t;

class IAL : public QObject
{
    Q_OBJECT

public:
    explicit IAL(IConnection *parent, QString *activeNickname, QList<char> *sortingRule, TScriptParent *sp);
    void reset(); // When socket disconnects, run this.
    void addNickname(QString nickname);
    void delNickname(QString nickname);
    bool hasNickname(QString nickname);
    bool setHostname(QString nickname, QString hostname);
    bool setIdent(QString nickname, QString ident);
    bool setNickname(QString nickname, QString newNickname);
    bool addChannel(QString nickname, QString channel);
    bool delChannel(QString nickname, QString channel); // If it got all channels removed, nickname will also be removed from entries.
    bool addMode(QString nickname, QString channel, char mode); // Set modes like @ + ... not o v ...
    bool delMode(QString nickname, QString channel, char mode); // Unset modes like @ + ... not o v ...
    bool resetModes(QString nickname, QString channel);
    QStringList* getNickList(QString channel); // Produce a nicklist for given channel. Remember to delete the pointer after use!

    QStringList* getAllNicknames();
    QStringList* getChannels(QString nickname);
    QString getIdent(QString nickname);
    QString getHost(QString nickname);
    QList<char> getModeChars(QString nickname, QString channel, bool cs = true);

    bool sharesChannel(QString nickname, QString channel); // checks if nickname shares the channel with us (they're also on there)
    bool isOperator(QString nickname, QString channel);
    bool isHalfop(QString nickname, QString channel);
    bool isVoiced(QString nickname, QString channel);
    bool isRegular(QString nickname, QString channel);

    int userCount(QString channel);

    void setChannelBan(QString channel, QString nickname);

private:
    QString *activeNick; // My current nickname of this connection.
    QHash<QString,IALEntry_t*> entries;
    QStringList garbage;
    QTimer garbageTimer;
    QStringList banSet;
    QList<char> *sortrule;
    TScriptParent *scriptParent;
    IConnection *connection;
    IALEntry_t* getEntry(QString nickname, bool cs = true);
    IALChannel_t* getChannel(QString nickname, QString channel, bool cs = true);
    void sortList(QList<char> *lst);
    bool sortLargerThan(const QString s1, const QString s2);

private slots:
    void cleanGarbage();
};

#endif // IAL_H
