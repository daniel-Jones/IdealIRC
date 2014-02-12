/*
 *   IdealIRC - Internet Relay Chat client
 *   Copyright (C) 2013  Tom-Andre Barstad
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef ICONFIG_H
#define ICONFIG_H

#include <QDialog>
#include <QSignalMapper>
#include <QShowEvent>
#include <QCloseEvent>
#include <QResizeEvent>

#include "iconnection.h"
#include "config.h"
#include "iconfig/iconfiggeneral.h"
#include "iconfig/iconfigcustomize.h"
#include "iconfig/iconfigperform.h"
#include "iconfig/iconfiglogging.h"

namespace Ui {
class IConfig;
}

class IConfig : public QDialog
{
    Q_OBJECT
    
  public:
    explicit IConfig(config *cfg, IConnection *con, QWidget *parent = 0);
    ~IConfig();
    
  private:
    Ui::IConfig *ui;
    QSignalMapper buttonSignals;
    config *conf;
    IConfigGeneral *wGeneral;
    IConfigPerform *wPerform;
    IConfigCustomize *wCustomize;
    IConfigLogging *wLogging;

    void saveAll();
    void closeSubWidgets();
    IConnection *current;

  private slots:
    void buttonMapped(QWidget *btn);
    void on_btnSaveConnect_clicked();
    void on_btnSaveClose_clicked();
    void on_btnCancel_clicked();

    void on_btnDisconnect_clicked();

protected:
    void showEvent(QShowEvent *);
    void closeEvent(QCloseEvent *);
    void resizeEvent(QResizeEvent *);

  signals:
    void connectToServer(bool newWindow = false);
    void configSaved();

};

#endif // ICONFIG_H
