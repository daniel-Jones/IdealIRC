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

#ifndef ICONFIGGENERAL_H
#define ICONFIGGENERAL_H

#include <QWidget>
#include <QItemSelectionModel>

#include "config.h"
#include "servermgr.h"
#include "servermodel.h"
#include "iconfig/iservereditor.h"

namespace Ui {
class IConfigGeneral;
}

class IConfigGeneral : public QWidget
{
    Q_OBJECT
    
public:
    explicit IConfigGeneral(config *cfg, QWidget *parent = 0);
    ~IConfigGeneral();
    void saveConfig();
    
private slots:
    void selectionRowChanged(const QModelIndex& current, const QModelIndex& previous);
    void on_btnEditServers_clicked();
    void on_servers_currentIndexChanged(int index);

private:
    Ui::IConfigGeneral *ui;
    config *conf;
    ServerMgr sm;
    IServerEditor se;
    ServerModel serverModel;
    QItemSelectionModel *selection;
};

#endif // ICONFIGGENERAL_H
