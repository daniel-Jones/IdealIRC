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

IConfig::IConfig(config *cfg, IConnection *con, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IConfig),
    conf(cfg),
    wGeneral(NULL),
    wPerform(NULL),
    wCustomize(NULL),
    wLogging(NULL),
    current(con),
    connectEnabled(true)
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

    ui->btnDisconnect->hide();

    if (con != NULL)
        if (con->isOnline())
            ui->btnDisconnect->show();
}

void IConfig::showEvent(QShowEvent *)
{
    if (wGeneral == NULL) {
        wGeneral = new IConfigGeneral(conf, current, ui->frame);
        wGeneral->resize(ui->frame->size());
        wGeneral->show();
    }

    if (wPerform == NULL) {
        wPerform = new IConfigPerform(conf, ui->frame);
        wPerform->resize(ui->frame->size());
    }

    if (wCustomize == NULL) {
        wCustomize = new IConfigCustomize(conf, ui->frame);
        wCustomize->resize(ui->frame->size());
    }

    if (wLogging == NULL) {
        wLogging = new IConfigLogging(conf, ui->frame);
        wLogging->resize(ui->frame->size());
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

    if (wPerform != NULL)
        wPerform->resize(ui->frame->size());

    if (wCustomize != NULL)
        wCustomize->resize(ui->frame->size());

    if (wLogging != NULL)
        wLogging->resize(ui->frame->size());
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

    if (toolbutton->objectName() != "btPerform") {
        ui->btPerform->setChecked(false);
        wPerform->hide();
    }
    else {
        ui->btPerform->setChecked(true);
        wPerform->show();
    }

    if (toolbutton->objectName() != "btCustomize") {
        ui->btCustomize->setChecked(false);
        wCustomize->hide();
    }
    else {
        ui->btCustomize->setChecked(true);
        wCustomize->show();
    }


    if (toolbutton->objectName() != "btLog") {
        ui->btLog->setChecked(false);
        wLogging->hide();
    }
    else {
        ui->btLog->setChecked(true);
        wLogging->show();
    }
}



IConfig::~IConfig()
{
    delete wGeneral;
    delete wPerform;
    delete wCustomize;
    delete wLogging;
    wGeneral = NULL;
    wPerform = NULL;
    wCustomize = NULL;
    wLogging = NULL;
    delete ui;
}

void IConfig::saveAll()
{
    wGeneral->saveConfig();
    wPerform->saveConfig();
    wCustomize->saveConfig();
    wLogging->saveConfig();

    conf->save();
    emit configSaved();
}

void IConfig::closeSubWidgets()
{
    if (wGeneral != NULL) {
        wGeneral->close();
        delete wGeneral;
        wGeneral = NULL;
    }

    if (wPerform != NULL) {
        wPerform->close();
        delete wPerform;
        wPerform = NULL;
    }

    if (wCustomize != NULL) {
        wCustomize->close();
        delete wCustomize;
        wCustomize = NULL;
    }

    if (wLogging != NULL) {
        wLogging->close();
        delete wLogging;
        wLogging = NULL;
    }
}

void IConfig::setConnectionEnabled(bool enable)
{
    bool newServer = ui->newServer->isChecked();
    ui->btnSaveConnect->setEnabled(enable || newServer);
    ui->btnSaveClose->setEnabled(enable || newServer);

    connectEnabled = enable;
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

void IConfig::on_btnDisconnect_clicked()
{
    ui->btnDisconnect->hide();

    current->closeConnection();
}

void IConfig::on_newServer_toggled(bool checked)
{
    if (connectEnabled == false) {
        ui->btnSaveConnect->setEnabled(checked);
        ui->btnSaveClose->setEnabled(checked);
    }
}
