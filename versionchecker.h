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

#ifndef VERSIONCHECKER_H
#define VERSIONCHECKER_H

#include <QTcpSocket>

class VersionChecker : public QTcpSocket
{
    Q_OBJECT
public:
    explicit VersionChecker(QObject *parent = 0);
    QString getVersion() { return version; }
    int getInternVersion() { return internversion; }
    void runChecker(); // Run this once, and signal gotVersion() will emit when finished.

private:
    QString version;
    int internversion;
    bool htmldata;

public slots:
    void socketOpened();
    void dataRead();

signals:
    void gotVersion();
    void unableToCheck(); // When this one is sent, we just want to destroy this object.
};

#endif // VERSIONCHECKER_H
