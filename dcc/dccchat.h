#ifndef DCCCHAT_H
#define DCCCHAT_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

#include "dcc.h"

class IWin;

class DCCChat : public DCC
{
  Q_OBJECT
public:
    explicit DCCChat(DCCType t, IWin *parentWin, QString dcc_details);
    ~DCCChat();
    void inputEnterPushed(QString line);

private:
    QTcpSocket *socket;
    QTcpServer server;

    // nicknames
    QString target;
    QString me;

    void initialize();

private slots:
    void sockConnected();
    void sockDisconnected();
    void sockRead();

    void newConnection();

signals:
    void Highlight();
};

#endif // DCCCHAT_H
