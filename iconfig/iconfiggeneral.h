/*
 *   IdealIRC - Internet Relay Chat client
 *   Copyright (C) 2014  Tom-Andre Barstad
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

/*! \class IConfigGeneral
 *  \brief A "sub widget" of IConfig, a GUI frontend for config class (iirc.ini)
 *
 * General configuration happens here, nicknames, servers.
 */

#ifndef ICONFIGGENERAL_H
#define ICONFIGGENERAL_H

#include <QWidget>
#include <QItemSelectionModel>

#include "config.h"
#include "servermgr.h"
#include "servermodel.h"
#include "iconfig/iservereditor.h"
#include "iconnection.h"

namespace Ui {
class IConfigGeneral;
}

class IConfigGeneral : public QWidget
{
    Q_OBJECT
    
public:
    explicit IConfigGeneral(config *cfg, IConnection *con, QWidget *parent = 0);
    ~IConfigGeneral();
    void saveConfig();
    
private slots:
    void selectionRowChanged(const QModelIndex& current, const QModelIndex &);
    void on_btnEditServers_clicked();
    void reloadServerList(); // Used when editor closes

private:
    Ui::IConfigGeneral *ui; //!< Qt Creator generated GUI class.
    config *conf; //!< Pointer to config class (iirc.ini)
    IConnection *current; //!< Pointer to the current active IRC connection to perform operations to.
    ServerMgr sm; //!< Manages servers.ini file
    IServerEditor se; //!< GUI frontend for ServerMgr class.
    ServerModel serverModel; //!< Stores processed data from ServerMgr in here.
    QItemSelectionModel *selection; //!< Keeps track of selections within serverModel.
};

#endif // ICONFIGGENERAL_H
