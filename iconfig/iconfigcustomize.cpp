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
    conf(cfg),
    slidersMovingRGB(false),
    slidersMovingHSV(false)
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
    ui->chkShowMotd->setChecked( conf->showMotd );
    ui->edQuit->setText( conf->quit );

    ui->chkUnderlineLinks->setChecked( conf->linkUnderline );
    ui->chkModes->setChecked( conf->showUsermodeMsg );
    ui->chkTimestamp->setChecked( conf->showTimestmap );
    ui->edTimeFormat->setText( conf->timestamp );
    ui->chkTrayNotify->setChecked( conf->trayNotify );
    ui->edTray->setValue( conf->trayNotifyDelay/1000 );

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

    connect(ui->slideR, SIGNAL(sliderMoved(int)),
            this, SLOT(colorSlidersMoveRGB(int)));
    connect(ui->slideG, SIGNAL(sliderMoved(int)),
            this, SLOT(colorSlidersMoveRGB(int)));
    connect(ui->slideB, SIGNAL(sliderMoved(int)),
            this, SLOT(colorSlidersMoveRGB(int)));
    connect(ui->slideH, SIGNAL(sliderMoved(int)),
            this, SLOT(colorSlidersMoveHSV(int)));
    connect(ui->slideS, SIGNAL(sliderMoved(int)),
            this, SLOT(colorSlidersMoveHSV(int)));
    connect(ui->slideV, SIGNAL(sliderMoved(int)),
            this, SLOT(colorSlidersMoveHSV(int)));

    connect(ui->slideR, SIGNAL(sliderPressed()),
            this, SLOT(colorSlidersRGBPressed()));
    connect(ui->slideG, SIGNAL(sliderPressed()),
            this, SLOT(colorSlidersRGBPressed()));
    connect(ui->slideB, SIGNAL(sliderPressed()),
            this, SLOT(colorSlidersRGBPressed()));
    connect(ui->slideH, SIGNAL(sliderPressed()),
            this, SLOT(colorSlidersHSVPressed()));
    connect(ui->slideS, SIGNAL(sliderPressed()),
            this, SLOT(colorSlidersHSVPressed()));
    connect(ui->slideV, SIGNAL(sliderPressed()),
            this, SLOT(colorSlidersHSVPressed()));

    connect(ui->slideR, SIGNAL(sliderReleased()),
            this, SLOT(colorSlidersRGBReleased()));
    connect(ui->slideG, SIGNAL(sliderReleased()),
            this, SLOT(colorSlidersRGBReleased()));
    connect(ui->slideB, SIGNAL(sliderReleased()),
            this, SLOT(colorSlidersRGBReleased()));
    connect(ui->slideH, SIGNAL(sliderReleased()),
            this, SLOT(colorSlidersHSVReleased()));
    connect(ui->slideS, SIGNAL(sliderReleased()),
            this, SLOT(colorSlidersHSVReleased()));
    connect(ui->slideV, SIGNAL(sliderReleased()),
            this, SLOT(colorSlidersHSVReleased()));

    colDefault           = conf->colDefault;
    colLocalInfo         = conf->colLocalInfo;
    colServerInfo        = conf->colServerInfo;
    colAction            = conf->colAction;
    colCTCP              = conf->colCTCP;
    colNotice            = conf->colNotice;
    colOwntextBg         = conf->colOwntextBg;
    colOwntext           = conf->colOwntext;
    colLinks             = conf->colLinks;
    colBackground        = conf->colBackground;
    colInput             = conf->colInput;
    colInputBackground   = conf->colInputBackground;
    colListbox           = conf->colListbox;
    colListboxBackground = conf->colListboxBackground;

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

    if (! slidersMovingRGB) { // Do not set sliders programmatically when we're manually moving them
        int r = color.red();
        int g = color.green();
        int b = color.blue();
        ui->slideR->setValue(r);
        ui->slideG->setValue(g);
        ui->slideB->setValue(b);
    }

    if (! slidersMovingHSV) { // Do not set sliders programmatically when we're manually moving them
        int hue = color.hslHue();
        int sat = color.hslSaturation();
        int val = color.value();
        ui->slideH->setValue(hue);
        ui->slideS->setValue(sat);
        ui->slideV->setValue(val);
    }
}

void IConfigCustomize::on_colorCode_textChanged(const QString &arg1)
{
    QColor color(arg1);
    pp.setColor(QPalette::Background, color);
    ui->colPreview->setAutoFillBackground(true);
    ui->colPreview->setPalette(pp);

    if (ui->rdActions->isChecked())
        colAction = arg1;

    if (ui->rdBackground->isChecked())
        colBackground = arg1;

    if (ui->rdCtcp->isChecked())
        colCTCP = arg1;

    if (ui->rdDefault->isChecked())
        colDefault = arg1;

    if (ui->rdLinks->isChecked())
        colLinks = arg1;

    if (ui->rdLocalInfo->isChecked())
        colLocalInfo = arg1;

    if (ui->rdNotice->isChecked())
        colNotice = arg1;

    if (ui->rdOwn->isChecked())
        colOwntext = arg1;

    if (ui->rdServerInfo->isChecked())
        colServerInfo = arg1;
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
    conf->trayNotify = ui->chkTrayNotify->isChecked();
    conf->trayNotifyDelay = ui->edTray->value()*1000;

    conf->colDefault           = colDefault;
    conf->colLocalInfo         = colLocalInfo;
    conf->colServerInfo        = colServerInfo;
    conf->colAction            = colAction;
    conf->colCTCP              = colCTCP;
    conf->colNotice            = colNotice;
    conf->colOwntextBg         = colOwntextBg;
    conf->colOwntext           = colOwntext;
    conf->colLinks             = colLinks;
    conf->colBackground        = colBackground;
    conf->colInput             = colInput;
    conf->colInputBackground   = colInputBackground;
    conf->colListbox           = colListbox;
    conf->colListboxBackground = colListboxBackground;
}

void IConfigCustomize::colorSelected(QString objName)
{
    QString color;
    if (objName == "rdActions") {
        color = colAction;
    }
    if (objName == "rdBackground") {
        color = colBackground;
    }
    if (objName == "rdCtcp") {
        color = colCTCP;
    }
    if (objName == "rdDefault") {
        color = colDefault;
    }
    if (objName == "rdLinks") {
        color = colLinks;
    }
    if (objName == "rdLocalInfo") {
        color = colLocalInfo;
    }
    if (objName == "rdNotice") {
        color = colNotice;
    }
    if (objName == "rdOwn") {
        color = colOwntext;
    }
    if (objName == "rdServerInfo") {
        color = colServerInfo;
    }

    if (color.length() == 0)
        return;

    QColor c(color);
    colorPicked(c);
}

void IConfigCustomize::colorSlidersMoveRGB(int)
{
    QColor color;
    color.setRgb(ui->slideR->value(), ui->slideG->value(), ui->slideB->value());
    colorPicked(color);
}

void IConfigCustomize::colorSlidersMoveHSV(int)
{
    QColor color;
    color.setHsv(ui->slideH->value(), ui->slideS->value(), ui->slideV->value());
    colorPicked(color);
}
