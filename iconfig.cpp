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

#include <iostream>

#include "iconfig.h"
#include "ui_iconfig.h"

IConfig::IConfig(config *cfg, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IConfig),
    conf(cfg),
    wGeneral(NULL),
    wCustomize(NULL)
{
    ui->setupUi(this);

    connect(&buttonSignals, SIGNAL(mapped(QWidget*)),
            this, SLOT(buttonMapped(QWidget*)));


    buttonSignals.setMapping(ui->btGeneral, (QWidget*)ui->btGeneral);
    buttonSignals.setMapping(ui->btPerform, (QWidget*)ui->btPerform);
    buttonSignals.setMapping(ui->btCustomize, (QWidget*)ui->btCustomize);
    buttonSignals.setMapping(ui->btLog, (QWidget*)ui->btLog);

    connect(ui->btGeneral, SIGNAL(clicked()),
            &buttonSignals, SLOT(map()));

    connect(ui->btPerform, SIGNAL(clicked()),
            &buttonSignals, SLOT(map()));

    connect(ui->btCustomize, SIGNAL(clicked()),
            &buttonSignals, SLOT(map()));

    connect(ui->btLog, SIGNAL(clicked()),
            &buttonSignals, SLOT(map()));
}

void IConfig::showEvent(QShowEvent *)
{
    if (wGeneral == NULL) {
        wGeneral = new IConfigGeneral(conf, ui->frame);
        wGeneral->resize(ui->frame->size());
        wGeneral->show();
    }

    if (wCustomize == NULL) {
        wCustomize = new IConfigCustomize(conf, ui->frame);
        wCustomize->resize(ui->frame->size());
    }

}

void IConfig::closeEvent(QCloseEvent *)
{
    closeSubWidgets();
}

void IConfig::resizeEvent(QResizeEvent *)
{
    if (wGeneral != NULL)
        wGeneral->resize(ui->frame->size());
}

void IConfig::buttonMapped(QWidget *btn)
{
    QToolButton *toolbutton = (QToolButton*)btn;

    if (toolbutton->objectName() != "btGeneral") {
        ui->btGeneral->setChecked(false);
        wGeneral->hide();
    }
    else {
        ui->btGeneral->setChecked(true);
        wGeneral->show();
    }

    if (toolbutton->objectName() != "btPerform")
        ui->btPerform->setChecked(false);
    else
        ui->btPerform->setChecked(true);


    if (toolbutton->objectName() != "btCustomize") {
        ui->btCustomize->setChecked(false);
        wCustomize->hide();
    }
    else {
        ui->btCustomize->setChecked(true);
        wCustomize->show();
    }


    if (toolbutton->objectName() != "btLog")
        ui->btLog->setChecked(false);
    else
        ui->btLog->setChecked(true);


}



IConfig::~IConfig()
{
    delete ui;
}

void IConfig::saveAll()
{
    wGeneral->saveConfig();
    conf->save();
}

void IConfig::closeSubWidgets()
{
    if (wGeneral != NULL) {
        wGeneral->close();
        delete wGeneral;
        wGeneral = NULL;
    }

    if (wCustomize != NULL) {
        wCustomize->close();
        delete wCustomize;
        wCustomize = NULL;
    }
}

void IConfig::on_btnSaveConnect_clicked()
{
    saveAll();
    emit connectToServer(ui->newServer->isChecked());
    close();
}

void IConfig::on_btnSaveClose_clicked()
{
    saveAll();
    close();
}

void IConfig::on_btnCancel_clicked()
{
    close();
}
