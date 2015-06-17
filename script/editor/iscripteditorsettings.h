/*
 *   IdealIRC - Internet Relay Chat client
 *   Copyright (C) 2015  Tom-Andre Barstad
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

/*! \class IScriptEditorSettings
 *  \brief The settings dialog for script editor.
 */

/*! \class ColorPicker
 *  \brief Helper class for IScriptEditorSettings to show a color picker.
 */

#ifndef ISCRIPTEDITORSETTINGS_H
#define ISCRIPTEDITORSETTINGS_H

#include <QDialog>
#include <QSignalMapper>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QPixmap>
#include <QShowEvent>
#include "config.h"

namespace Ui {
class IScriptEditorSettings;
}

class ColorPicker : public QDialog
{
    Q_OBJECT

public:
    explicit ColorPicker(QWidget *parent = 0);

protected:
    void showEvent(QShowEvent *);
    void paintEvent(QPaintEvent *);
    void mouseReleaseEvent(QMouseEvent *e);

private:
    bool firstShow;
    QPixmap colormap; //!< Same color picker as used in the IConfig dialog.

signals:
    void colorPicked(QColor color);
};

class IScriptEditorSettings : public QDialog
{
    Q_OBJECT

public:
    explicit IScriptEditorSettings(config *cfg, QWidget *parent = 0);
    ~IScriptEditorSettings();

private slots:
    void on_saveButton_clicked();
    void colorRadioButton_clicked(QString objName);
    void on_colorbtn_clicked();
    void colorPicked(QColor color);

private:
    Ui::IScriptEditorSettings *ui; //!< Qt Creator generated GUI class.
    config *conf; //!< Pointer to config class (iirc.ini)
    QColor invertColor(QColor c);
    void setButtonColor(QColor color);

    // Copy of configs
    QColor editorLineHighlight;
    QColor editorFunctionDef;
    QColor editorMetaKeyword;
    QColor editorKeyword;
    QColor editorWindow;
    QColor editorVariable;
    QColor editorComment;
    QColor editorBackground;
    QColor editorText;
    QColor editorLineNumBackground;
    QColor editorLineNumText;

    QString editorFontName;
    int editorFontSize;

    // Group of radio slots directed to correct
    // color to switch and change
    QSignalMapper colorRadioSignals; //!< Map of all color radio buttons, tied to their clicked signal.
    QString selectedColor; //!< The color that's selected to edito.

    ColorPicker cpick; //!< Color picker dialog.

signals:
    void settingsSaved();
};

#endif // ISCRIPTEDITORSETTINGS_H
