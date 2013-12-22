#ifndef SERVERTREEMODEL_H
#define SERVERTREEMODEL_H

#include <QAbstractItemModel>
#include "servermgr.h"

class ServerTreeItem;

class ServerTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit ServerTreeModel(QObject *parent = 0);
    ~ServerTreeModel();

    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

private:
    void setupModelData(ServerTreeItem *parent);
    ServerTreeItem *rootItem;
};
#endif // SERVERTREEMODEL_H
