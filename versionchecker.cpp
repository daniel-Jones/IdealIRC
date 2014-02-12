/*
 *   IdealIRC - Internet Relay Chat client
 *   Copyright (C) 2013  Tom-Andre Barstad
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

#include "versionchecker.h"
#include "constants.h"
#include <QDebug>
#include <QStringList>

VersionChecker::VersionChecker(QObject *parent) :
    QTcpSocket(parent),
    version(VERSION_STRING),
    internversion(VERSION_INTEGER),
    htmldata(false)
{
    connect(this, SIGNAL(connected()),
            this, SLOT(socketOpened()));

    connect(this, SIGNAL(readyRead()),
            this, SLOT(dataRead()));
}

void VersionChecker::runChecker()
{
    connectToHost("www.idealirc.org", 80);
}

void VersionChecker::socketOpened()
{
    QString data = "GET /v HTTP/1.1\r\nHost: www.idealirc.org\r\n\r\n";
    write(data.toUtf8());
}

void VersionChecker::dataRead()
{
    while (bytesAvailable() > 0) {
        QString text = readLine(128);

        if ((text == "\r\n") && (htmldata == false)) {
            htmldata = true;
        }

        if ((htmldata == true) && (text != "\r\n")) {
            text.replace("\r", "");
            text.replace("\n", "");
            QStringList vls = text.split(":");
            internversion = vls[0].toInt();
            version = vls[1];
            emit gotVersion();
        }
    }
}
