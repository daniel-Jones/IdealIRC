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

/*! \class IConfigPerform
 *  \brief A "sub widget" of IConfig, a GUI frontend for config class (iirc.ini)
 *
 * A text editor where you put IRC commands line by line to perform when connected to an IRC server.
 */

#ifndef ICONFIGPERFORM_H
#define ICONFIGPERFORM_H

#include <QWidget>
#include "config.h"
#include "constants.h"

namespace Ui {
class IConfigPerform;
}

class IConfigPerform : public QWidget
{
    Q_OBJECT

public:
    explicit IConfigPerform(config *cfg, QWidget *parent = 0);
    ~IConfigPerform();
    void saveConfig();

private:
    Ui::IConfigPerform *ui; //!< Qt Creator generated GUI class.
    config *conf; //!< Pointer to config class (iirc.ini)
};

#endif // ICONFIGPERFORM_H
