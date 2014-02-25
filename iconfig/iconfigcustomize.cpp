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

#include "iconfigcustomize.h"
#include "ui_iconfigcustomize.h"
#include <QDebug>


IConfigCustomize::IConfigCustomize(config *cfg, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IConfigCustomize),
    conf(cfg)
{
    ui->setupUi(this);

    pp = ui->colPreview->palette();

    connect(&scene, SIGNAL(colorPicked(QColor)),
            this, SLOT(colorPicked(QColor)));

    ui->colorPick->setScene(&scene);

    QFont f(conf->fontName);
    f.setPixelSize(conf->fontSize);
    ui->spinBox->setValue(conf->fontSize);
    ui->fontComboBox->setCurrentFont(f);

    ui->chkShowOptions->setChecked( conf->showOptionsStartup );
    ui->chkShowWhois->setChecked( conf->showWhois );
    ui->chkShowMotd->setCheckable( conf->showMotd );
    ui->edQuit->setText( conf->quit );

    ui->chkUnderlineLinks->setChecked( conf->linkUnderline );
    ui->chkModes->setChecked( conf->showUsermodeMsg );
    ui->chkTimestamp->setChecked( conf->showTimestmap );
    ui->edTimeFormat->setText( conf->timestamp );

    connect(ui->rdActions,    SIGNAL(clicked()), &colorSignals, SLOT(map()));
    connect(ui->rdBackground, SIGNAL(clicked()), &colorSignals, SLOT(map()));
    connect(ui->rdCtcp,       SIGNAL(clicked()), &colorSignals, SLOT(map()));
    connect(ui->rdDefault,    SIGNAL(clicked()), &colorSignals, SLOT(map()));
    connect(ui->rdLinks,      SIGNAL(clicked()), &colorSignals, SLOT(map()));
    connect(ui->rdLocalInfo,  SIGNAL(clicked()), &colorSignals, SLOT(map()));
    connect(ui->rdNotice,     SIGNAL(clicked()), &colorSignals, SLOT(map()));
    connect(ui->rdOwn,        SIGNAL(clicked()), &colorSignals, SLOT(map()));
    connect(ui->rdServerInfo, SIGNAL(clicked()), &colorSignals, SLOT(map()));

    colorSignals.setMapping(ui->rdActions,    ui->rdActions->objectName());
    colorSignals.setMapping(ui->rdBackground, ui->rdBackground->objectName());
    colorSignals.setMapping(ui->rdCtcp,       ui->rdCtcp->objectName());
    colorSignals.setMapping(ui->rdDefault,    ui->rdDefault->objectName());
    colorSignals.setMapping(ui->rdLinks,      ui->rdLinks->objectName());
    colorSignals.setMapping(ui->rdLocalInfo,  ui->rdLocalInfo->objectName());
    colorSignals.setMapping(ui->rdNotice,     ui->rdNotice->objectName());
    colorSignals.setMapping(ui->rdOwn,        ui->rdOwn->objectName());
    colorSignals.setMapping(ui->rdServerInfo, ui->rdServerInfo->objectName());

    connect(&colorSignals, SIGNAL(mapped(QString)), this, SLOT(colorSelected(QString)));

    colorSelected("rdDefault");
}

IConfigCustomize::~IConfigCustomize()
{
    delete ui;
}

void IConfigCustomize::colorPicked(QColor color)
{
    QString name = color.name();
    ui->colorCode->setText(name);
    pp.setColor(QPalette::Background, color);
    ui->colPreview->setAutoFillBackground(true);
    ui->colPreview->setPalette(pp);
}

void IConfigCustomize::on_colorCode_textChanged(const QString &arg1)
{
    QColor color(arg1);
    pp.setColor(QPalette::Background, color);
    ui->colPreview->setAutoFillBackground(true);
    ui->colPreview->setPalette(pp);

    if (ui->rdActions->isChecked())
        conf->colAction = arg1;

    if (ui->rdBackground->isChecked())
        conf->colBackground = arg1;

    if (ui->rdCtcp->isChecked())
        conf->colCTCP = arg1;

    if (ui->rdDefault->isChecked())
        conf->colDefault = arg1;

    if (ui->rdLinks->isChecked())
        conf->colLinks = arg1;

    if (ui->rdLocalInfo->isChecked())
        conf->colLocalInfo = arg1;

    if (ui->rdNotice->isChecked())
        conf->colNotice = arg1;

    if (ui->rdOwn->isChecked())
        conf->colOwntext = arg1;

    if (ui->rdServerInfo->isChecked())
        conf->colServerInfo = arg1;
}

void IConfigCustomize::on_spinBox_valueChanged(int arg1)
{
    QFont f = ui->fontComboBox->currentFont();
    f.setPixelSize(arg1);

    ui->fontComboBox->setCurrentFont(f);
}

void IConfigCustomize::saveConfig()
{
    QFont f = ui->fontComboBox->currentFont();
    conf->fontName = f.family();
    conf->fontSize = f.pixelSize();

    conf->showOptionsStartup = ui->chkShowOptions->isChecked();
    conf->showWhois = ui->chkShowWhois->isChecked();
    conf->showMotd = ui->chkShowMotd->isChecked();
    conf->quit = ui->edQuit->text();

    conf->linkUnderline = ui->chkUnderlineLinks->isChecked();
    conf->showUsermodeMsg = ui->chkModes->isChecked();
    conf->showTimestmap = ui->chkTimestamp->isChecked();
    conf->timestamp = ui->edTimeFormat->text();
}

void IConfigCustomize::colorSelected(QString objName)
{
    QString color;
    if (objName == "rdActions") {
        color = conf->colAction;
    }
    if (objName == "rdBackground") {
        color = conf->colBackground;
    }
    if (objName == "rdCtcp") {
        color = conf->colCTCP;
    }
    if (objName == "rdDefault") {
        color = conf->colDefault;
    }
    if (objName == "rdLinks") {
        color = conf->colLinks;
    }
    if (objName == "rdLocalInfo") {
        color = conf->colLocalInfo;
    }
    if (objName == "rdNotice") {
        color = conf->colNotice;
    }
    if (objName == "rdOwn") {
        color = conf->colOwntext;
    }
    if (objName == "rdServerInfo") {
        color = conf->colServerInfo;
    }

    if (color.length() == 0)
        return;

    QColor c(color);
    colorPicked(c);
}
