#ifndef ICHANNELLIST_H
#define ICHANNELLIST_H

#include <QDialog>
#include <QStandardItemModel>
#include <QHash>

class IConnection;

namespace Ui {
class IChannelList;
}

class IChannelList : public QDialog
{
    Q_OBJECT

public:
    explicit IChannelList(QWidget *parent = 0);
    ~IChannelList();
    void setConnection(IConnection *cptr) { connection = cptr; }
    void enable();
    void disable();
    void reset();
    void addItem(QString channel, QString users, QString topic);

private slots:
    void on_btnDownload_clicked();

    void on_btnJoin_clicked();

private:
    Ui::IChannelList *ui;
    IConnection *connection;
    QStandardItemModel model;
    QHash<QString,QStandardItem*> itemmap;
};

#endif // ICHANNELLIST_H
