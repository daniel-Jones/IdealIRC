#ifndef IFAVOURITES_H
#define IFAVOURITES_H

#include <QDialog>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#include <QHash>

#include "iconnection.h"
#include "config.h"

namespace Ui {
class IFavourites;
}

class IFavourites : public QDialog
{
    Q_OBJECT

public:
    explicit IFavourites(config *cfg, QWidget *parent = 0);
    void enableJoin(bool ok);
    void setConnection(IConnection *c) { current = c; }
    ~IFavourites();

private slots:
    void selectionRowChanged(const QModelIndex &current, const QModelIndex &previous);
    void itemChanged(QStandardItem *item);
    void on_edChannel_textChanged(const QString &arg1);
    void on_btnSave_clicked();
    void on_toolButton_clicked();
    void on_btnDelete_clicked();

    void on_btnJoin_clicked();

private:
    Ui::IFavourites *ui;
    config *conf;
    IniFile *ini;
    QStandardItemModel model;
    QItemSelectionModel *selection;
    QHash<QString,QStandardItem*> chanmap;
    IConnection *current;
    void loadChannel(const QString &channel);

};

#endif // IFAVOURITES_H
