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

/*! \class IConfigCustomize
 *  \brief A "sub widget" of IConfig, a GUI frontend for config class (iirc.ini)
 *
 * Here most of the customization happens- common stuff, colors, text appearance and background image.
 */

#ifndef ICONFIGCUSTOMIZE_H
#define ICONFIGCUSTOMIZE_H

#include <QWidget>
#include <QPalette>
#include <QSignalMapper>
#include "constants.h"
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
    Ui::IConfigCustomize *ui; //!< Qt Creator generated GUI class.
    config *conf; //!< Pointer to config class (iirc.ini)
    ColorPickerScene scene; //!< Color picker palette
    QPalette pp; //!< Preview of picked color
    QSignalMapper colorSignals; //!< Maps all color radio-buttons on their Clicked signal.
    bool slidersMovingRGB; //!< true if one of the R,G,B sliders are moving.
    bool slidersMovingHSV; //!< true if one of the H,S,V sliders are moving.

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
    void on_btnBrowse_clicked();
    void on_bgOpacity_valueChanged(int value);
};

#endif // ICONFIGCUSTOMIZE_H
