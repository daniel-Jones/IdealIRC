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

    colorPicked(Qt::black);

    ui->colorPick->setScene(&scene);

    QFont f(conf->fontName);
    f.setPixelSize(conf->fontSize);
    ui->spinBox->setValue(conf->fontSize);
    ui->fontComboBox->setCurrentFont(f);

    ui->chkShowOptions->setChecked( conf->showOptionsStartup );
    ui->chkShowWhois->setChecked( conf->showWhois );
    ui->edQuit->setText( conf->quit );

    ui->chkUnderlineLinks->setChecked( conf->linkUnderline );
    ui->chkModes->setChecked( conf->showUsermodeMsg );
    ui->chkTimestamp->setChecked( conf->showTimestmap );
    ui->edTimeFormat->setText( conf->timestamp );
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
    conf->quit = ui->edQuit->text();

    conf->linkUnderline = ui->chkUnderlineLinks->isChecked();
    conf->showUsermodeMsg = ui->chkModes->isChecked();
    conf->showTimestmap = ui->chkTimestamp->isChecked();
    conf->timestamp = ui->edTimeFormat->text();
}
