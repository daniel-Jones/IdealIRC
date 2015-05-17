#ifndef EETABLEITEM_H
#define EETABLEITEM_H

#include <QObject>
#include <QStandardItem>

class EETableItem : public QStandardItem
{
    Q_OBJECT

public:
    EETableItem();
    ~EETableItem();
};

#endif // EETABLEITEM_H
