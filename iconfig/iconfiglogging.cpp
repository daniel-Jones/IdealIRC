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

#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include "iconfiglogging.h"
#include "ui_iconfiglogging.h"
#include "constants.h"

IConfigLogging::IConfigLogging(config *cfg, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IConfigLogging),
    conf(cfg)
{
    ui->setupUi(this);

    QList<int> size;
    size << 50 << 200;

    ui->splitter->setSizes(size);

    QFont f( conf->fontName );
    f.setPixelSize( conf->fontSize );

    ui->logList->setFont(f);
    ui->logText->setFont(f);

    ui->edLogPath->setText( conf->logPath );

    ui->chkEnable->setChecked( conf->logEnabled );
    ui->chkChannels->setChecked( conf->logChannel );
    ui->chkPrivates->setChecked( conf->logPM );

    model = new QStandardItemModel(ui->logList);
    ui->logList->setModel(model);
    selection = ui->logList->selectionModel();

    connect(selection, SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(currentRowChanged(QModelIndex,QModelIndex)));

    loadFiles();
}

IConfigLogging::~IConfigLogging()
{
    delete model;
    delete ui;
}

void IConfigLogging::saveConfig()
{
    conf->logEnabled = ui->chkEnable->isChecked();
    conf->logChannel = ui->chkChannels->isChecked();
    conf->logPM = ui->chkPrivates->isChecked();
    conf->logPath = ui->edLogPath->text();
}

void IConfigLogging::on_btnBrowse_clicked()
{
    QString path = ui->edLogPath->text();
    if ((! QDir(path).exists()) || (path.isEmpty()))
        path = CONF_PATH;

    QFileDialog fd(this, tr("Select a log directory"), path);
    fd.setFileMode(QFileDialog::Directory);

    if (fd.exec()) {
        ui->edLogPath->setText( fd.directory().absolutePath() );
        loadFiles();
    }
}

void IConfigLogging::currentRowChanged(const QModelIndex &current, const QModelIndex &)
{
    QString file = current.data().toString();
    file.prepend('/');
    file.prepend(conf->logPath);
    QFile f(file);

    if (! f.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Could not open file"),
                             tr("Could not open file for reading:\r\n%1")
                               .arg(file));
        return;
    }

    QByteArray data = f.readAll();
    f.close();

    QString text(data);
    ui->logText->setPlainText(text);
}

void IConfigLogging::loadFiles()
{
    QString dir = conf->logPath;

    model->clear();
    ui->logText->clear();

    QDir path(dir);
    QStringList files = path.entryList(QDir::Files, QDir::Name);

    for (int i = 0; i <= files.length()-1; i++) {
        QString name = files[i];
        QStandardItem *item = new QStandardItem(name);
        model->appendRow(item);
    }
}
