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

#ifndef ICHANCONFIG_H
#define ICHANCONFIG_H

#include <QDialog>
#include <QCloseEvent>
#include <QHash>
#include "bantablemodel.h" /// @todo Rename to MaskTableModel
#include "unsupportedmodel.h"

typedef struct T_CS_DEFAULT
{
  bool enabled;
  QString data; // only for limit (mode=l) and key (mode=k)
} t_csdefault;

namespace Ui {
class IChanConfig;
}

enum MaskType {
    MT_BAN = 0,
    MT_EXCEPT,
    MT_INVITE
};

class IConnection;

class IChanConfig : public QDialog
{
    Q_OBJECT

public:
    explicit IChanConfig(IConnection *c, QString chan, QWidget *parent = 0);
    ~IChanConfig();
    void setDefaultMode(QString mode);
    void setDefaultTopic(QString topic);
    void addMask(QString mask, QString author, QString created, MaskType mt);
    void addMask(QString mask, QString author, QString created);
    void finishModel(MaskType type);
    void delMask(QString mask, MaskType type);

private:
    Ui::IChanConfig *ui;
    IConnection *connection;
    QString channel;
    UnsupportedModel *loading;
    BanTableModel banTable;
    BanTableModel exceptionTable;
    BanTableModel inviteTable;

    QString defaultTopic;
    QHash<char,t_csdefault> defaultMode;

    // Used when adding lists to the models.
    // Do not trust these to have anything you want.
    QStringList maskL;
    QStringList dateL;
    QStringList authorL;

    QString cmA;
    QString cmB;
    QString cmC;
    QString cmD;

    void deleteMasks(MaskType type); // Unset the selected masks
    void btnAddMask(MaskType type);
    void btnEditMask(MaskType type);
    void setMode(char mode, bool enabled, QString data = ""); // Does NOT send the mode to server, used for internal storage.


protected:
    void closeEvent(QCloseEvent *) { emit closed(); }

signals:
    void closed();
private slots:
    void on_banDel_clicked();
    void on_exceptionDel_clicked();
    void on_inviteDel_clicked();
    void on_btnSave_clicked();
    void on_banAdd_clicked();
    void on_exceptionAdd_clicked();
    void on_inviteAdd_clicked();
    void on_inviteEdit_clicked();
    void on_exceptionEdit_clicked();
    void on_banEdit_clicked();
};

#endif // ICHANCONFIG_H
