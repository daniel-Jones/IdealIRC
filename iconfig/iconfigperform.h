#ifndef ICONFIGPERFORM_H
#define ICONFIGPERFORM_H

#include <QWidget>
#include "config.h"
#include "constants.h"

namespace Ui {
class IConfigPerform;
}

class IConfigPerform : public QWidget
{
    Q_OBJECT

public:
    explicit IConfigPerform(config *cfg, QWidget *parent = 0);
    ~IConfigPerform();
    void saveConfig();

private:
    Ui::IConfigPerform *ui;
    config *conf;
};

#endif // ICONFIGPERFORM_H
