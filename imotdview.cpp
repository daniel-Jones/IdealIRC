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

#include "imotdview.h"
#include "ui_imotdview.h"

IMotdView::IMotdView(config *cfg, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IMotdView),
    conf(cfg)
{
    ui->setupUi(this);

    view = new IIRCView(cfg, ui->frame);

    ui->checkBox->setChecked( conf->showMotd );

}

IMotdView::~IMotdView()
{
    delete ui;
}

void IMotdView::print(QString sender, QString &line)
{
    view->addLine(sender, line);
}

void IMotdView::showEvent(QShowEvent *)
{
    view->setGeometry( ui->frame->geometry() );
    view->changeFont( conf->fontName, conf->fontSize );
}

void IMotdView::resizeEvent(QResizeEvent *)
{
    view->setGeometry( ui->frame->geometry() );
}


void IMotdView::on_checkBox_toggled(bool checked)
{
    conf->showMotd = checked;
}
