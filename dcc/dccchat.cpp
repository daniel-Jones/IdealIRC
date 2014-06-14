#include "dccchat.h"

#include "iwin.h"
#include "iconnection.h"

DCCChat::DCCChat(DCCType t, IWin *parentWin, QString dcc_details) :
    DCC(parentWin, dcc_details),
    socket(NULL)
{
    type = t;
}

DCCChat::~DCCChat()
{
    if (socket != NULL)
        socket->deleteLater();
}

void DCCChat::initialize()
{
    QStringList dsplit = details.split(' ');

    me = dsplit[0];
    target = dsplit[1];

    if (dsplit[2] == "CHAT") {
        unsigned int intipv4 = dsplit[4].toUInt();
        QString ipv4 = subwin->getConnection()->intipv4toStr(intipv4);
        print( tr("Trying to connect to %1 (%2:%3)...")
                 .arg(target)
                 .arg(ipv4)
                 .arg(dsplit[5])
              );

        socket = new QTcpSocket();

        connect(socket, SIGNAL(connected()),
                this, SLOT(sockConnected()));
        socket->connectToHost(ipv4, dsplit[5].toInt());
    }

    if (details.split(' ')[2] == "CHATHOST") {
        // Not an actual DCC type, but used to make this class listen
        // for DCC incoming connection (we request chatting)

        connect(&server, SIGNAL(newConnection()),
                this, SLOT(newConnection()));
        server.listen(QHostAddress::Any, 4455);
        print(tr("Waiting for connection..."), PT_LOCALINFO);
    }

}

void DCCChat::inputEnterPushed(QString line)
{
    if (! socket->isOpen())
        return;

    print( QString("<%1> %2")
             .arg(me)
             .arg(line)
          );

    QByteArray data;
    data.append(line);
    data.append("\r\n");
    socket->write(data);
}

void DCCChat::sockConnected()
{
    print(tr("Connected."), PT_LOCALINFO);

    connect(socket, SIGNAL(readyRead()),
            this, SLOT(sockRead()));
}

void DCCChat::sockDisconnected()
{
    print(tr("Disconnected from DCC."), PT_LOCALINFO);
}

void DCCChat::sockRead()
{
    QByteArray buf = socket->readAll();
    QString line;
    for (int i = 0; i <= buf.length()-1; ++i) {
        if (buf[i] == '\r')
            continue;
        if (buf[i] == '\n') {
            print( QString("<%1> %2")
                     .arg(target)
                     .arg(line)
                  );
            line.clear();
            emit Highlight();
            continue;
        }
        line += buf[i];
    }
}

void DCCChat::newConnection()
{
    if (socket != NULL)
        return;

    socket = server.nextPendingConnection();
    connect(socket, SIGNAL(disconnected()),
            this, SLOT(sockDisconnected()));
    connect(socket, SIGNAL(readyRead()),
            this, SLOT(sockRead()));
}
