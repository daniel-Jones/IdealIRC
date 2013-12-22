#ifndef SERVERTREEITEM_H
#define SERVERTREEITEM_H

#include <QList>
#include <QVariant>

class ServerTreeItem
{
public:
    ServerTreeItem(const QList<QVariant> &data, QString pass = "", ServerTreeItem *parent = 0);
    ~ServerTreeItem();
    void appendChild(ServerTreeItem *child);

    ServerTreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    ServerTreeItem *parent();
    QString password;

private:
    QList<ServerTreeItem*> childItems;
    QList<QVariant> itemData;
    ServerTreeItem *parentItem;
};

#endif // SERVERTREEITEM_H
