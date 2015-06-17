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

/*! \class IConfigLogging
 *  \brief A "sub widget" of IConfig, a GUI frontend for config class (iirc.ini)
 *
 * Log viewing.
 */

#ifndef ICONFIGLOGGING_H
#define ICONFIGLOGGING_H

#include <QWidget>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#include "config.h"

namespace Ui {
class IConfigLogging;
}

class IConfigLogging : public QWidget
{
    Q_OBJECT

public:
    explicit IConfigLogging(config *cfg, QWidget *parent = 0);
    ~IConfigLogging();
    void saveConfig();

private slots:
    void on_btnBrowse_clicked();
    void currentRowChanged(const QModelIndex &current, const QModelIndex &);

private:
    Ui::IConfigLogging *ui; //!< Qt Creator generated GUI class.
    config *conf; //!< Pointer to config class (iirc.ini)
    QStandardItemModel *model; //!< Model for ui->logList
    QItemSelectionModel *selection; //!< Keeps track of selection in the model.
    void loadFiles();
};

#endif // ICONFIGLOGGING_H
