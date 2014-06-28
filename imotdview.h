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

#ifndef IMOTDVIEW_H
#define IMOTDVIEW_H

#include <QDialog>
#include <QShowEvent>
#include <QResizeEvent>
#include "iircview.h"
#include "config.h"

namespace Ui {
class IMotdView;
}

class IMotdView : public QDialog
{
    Q_OBJECT

public:
    explicit IMotdView(config *cfg, QWidget *parent = 0);
    ~IMotdView();
    void reset() { view->clear(); }
    void print(QString sender, QString &line);

private:
    Ui::IMotdView *ui;
    IIRCView *view;
    config *conf;

protected:
    void showEvent(QShowEvent *);
    void resizeEvent(QResizeEvent *);
private slots:
    void on_checkBox_toggled(bool checked);
};

#endif // IMOTDVIEW_H
