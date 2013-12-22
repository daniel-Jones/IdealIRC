#include "servertreeitem.h"

ServerTreeItem::ServerTreeItem(const QList<QVariant> &data, QString pass, ServerTreeItem *parent) :
    password(pass),
    itemData(data),
    parentItem(parent)
{
}

ServerTreeItem::~ServerTreeItem()
{
    qDeleteAll(childItems);
}

void ServerTreeItem::appendChild(ServerTreeItem *item)
{
    childItems.append(item);
}

ServerTreeItem *ServerTreeItem::child(int row)
{
    return childItems.value(row);
}

int ServerTreeItem::childCount() const
{
    return childItems.count();
}

int ServerTreeItem::columnCount() const
{
    return itemData.count();
}

QVariant ServerTreeItem::data(int column) const
{
    return itemData.value(column);
}

ServerTreeItem *ServerTreeItem::parent()
{
    return parentItem;
}

int ServerTreeItem::row() const
{
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<ServerTreeItem*>(this));

    return 0;
}
