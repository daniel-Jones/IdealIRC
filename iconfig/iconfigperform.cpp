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

#include "iconfigperform.h"
#include "ui_iconfigperform.h"
#include <QMessageBox>

IConfigPerform::IConfigPerform(config *cfg, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IConfigPerform),
    conf(cfg)
{
    ui->setupUi(this);

    QFont f( conf->fontName );
    f.setPixelSize( conf->fontSize );
    ui->cmdEdit->setFont(f);

    QString fname = QString("%1/perform")
                     .arg(CONF_PATH);

    if (! QFile(fname).exists()) {
        // Create empty file.
        QFile f(fname);

        if (! f.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(this, tr("Unable to create file"),
                                 tr("Could not create a file to write perform to.\r\nCheck your write permissions in folder\r\n%1").arg(CONF_PATH));
            return;
        }
        f.close();
    }


    QFile pf(fname);
    if (! pf.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Unable to open file"),
                             tr("Could not open %1.\r\nCheck read permissions on the file.").arg(fname));
        return;
    }

    QByteArray data = pf.readAll();
    pf.close();

    QString ln;
    int len = data.length();
    for (int i = 0; i <= len-1; i++) {
        QChar c = data.at(i);

        if (c == '\n') {
            ui->cmdEdit->appendPlainText(ln);
            ln.clear();
            continue;
        }

        ln += c;
    }

}

IConfigPerform::~IConfigPerform()
{
    delete ui;
}

void IConfigPerform::saveConfig()
{
    QString fname = QString("%1/perform")
                     .arg(CONF_PATH);

    QFile f(fname);

    if (! f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Unable to open file"),
                             tr("Could not open %1.\r\nCheck write permissions on the file.").arg(fname));
        return;
    }

    QByteArray data;
    data.append(ui->cmdEdit->toPlainText());
    data.append('\n');

    f.write(data);

    f.close();
}
