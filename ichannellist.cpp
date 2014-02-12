#include "ichannellist.h"
#include "ui_ichannellist.h"

#include "iconnection.h"

IChannelList::IChannelList(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IChannelList),
    connection(NULL)
{
    ui->setupUi(this);

    ui->chanview->setModel(&model);

    reset();
}

IChannelList::~IChannelList()
{
    delete ui;
}

void IChannelList::enable()
{
    ui->btnDownload->setEnabled(true);
    ui->btnJoin->setEnabled(true);
}

void IChannelList::disable()
{
    ui->btnDownload->setEnabled(false);
    ui->btnJoin->setEnabled(false);
}

void IChannelList::reset()
{
    itemmap.clear();
    model.clear();

    QStandardItem *i = new QStandardItem();
    QStringList l;
    l << tr("Channel") << tr("Users") << tr("Topic");
    model.setColumnCount(3);
    model.setHorizontalHeaderItem(0, i);
    model.setHorizontalHeaderLabels(l);

    QHeaderView *header = ui->chanview->horizontalHeader();
    header->setSectionResizeMode(2, QHeaderView::Stretch);
}

void IChannelList::addItem(QString channel, QString users, QString topic)
{
    QStandardItem *chanItem = new QStandardItem(channel);
    QStandardItem *usersItem = new QStandardItem(users);
    QStandardItem *topicItem = new QStandardItem(topic);

    QList<QStandardItem*> list;
    list << chanItem << usersItem << topicItem;

    itemmap.insert(channel, chanItem);
    model.appendRow(list);
}

void IChannelList::on_btnDownload_clicked()
{
    if (connection == NULL)
        return;

    connection->sockwrite("LIST");
}

void IChannelList::on_btnJoin_clicked()
{
    QItemSelectionModel *selection = ui->chanview->selectionModel();

    if (! selection->hasSelection())
        return;

    QModelIndex index = selection->selectedRows(0)[0];

    connection->sockwrite(QString("JOIN %1")
                          .arg(index.data().toString()));
}
