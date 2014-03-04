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

#ifndef ICONFIGCUSTOMIZE_H
#define ICONFIGCUSTOMIZE_H

#include <QWidget>
#include <QPalette>
#include <QSignalMapper>
#include "config.h"
#include "colorpickerscene.h"

namespace Ui {
class IConfigCustomize;
}

class IConfigCustomize : public QWidget
{
    Q_OBJECT

public:
    explicit IConfigCustomize(config *cfg, QWidget *parent = 0);
    ~IConfigCustomize();
    void saveConfig();

private:
    Ui::IConfigCustomize *ui;
    config *conf;
    ColorPickerScene scene;
    QPalette pp; // preview palette
    QSignalMapper colorSignals;
    bool slidersMovingRGB;
    bool slidersMovingHSV;

    QString colDefault;
    QString colLocalInfo;
    QString colServerInfo;
    QString colAction;
    QString colCTCP;
    QString colNotice;
    QString colOwntextBg;
    QString colOwntext;
    QString colHighlight;
    QString colLinks;
    QString colBackground;
    QString colInput;
    QString colInputBackground;
    QString colListbox;
    QString colListboxBackground;
    QString colWindowlist;
    QString colWindowlistBackground;

public slots:
    void colorPicked(QColor color);

private slots:
    void on_colorCode_textChanged(const QString &arg1);
    void on_spinBox_valueChanged(int arg1);
    void colorSelected(QString objName);
    void colorSlidersMoveRGB(int);
    void colorSlidersMoveHSV(int);
    void colorSlidersRGBPressed() { slidersMovingRGB = true; }
    void colorSlidersRGBReleased() { slidersMovingRGB = false; }
    void colorSlidersHSVPressed() { slidersMovingHSV = true; }
    void colorSlidersHSVReleased() { slidersMovingHSV = false; }
};

#endif // ICONFIGCUSTOMIZE_H
