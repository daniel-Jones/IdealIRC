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
