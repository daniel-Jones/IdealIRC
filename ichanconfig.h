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

/*! \class IChanConfig
 *  \brief The channel settings dialog.
 *
 * In this dialog we can set the topic and different channel modes (+n, +t, +i, etc.).\n
 * Bans, Ban exceptions and invite list for those servers that support that goes here aswell.
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
    Ui::IChanConfig *ui; //!< Qt Creator generated GUI class.
    IConnection *connection; //!< The IRC connection this dialog is bound to.
    QString channel; //!< The channel this dialog is bound to.
    UnsupportedModel *loading; //!< Shows a "Loading" text, useful if we're on a slow connection.
    BanTableModel banTable; //!< Table of all bans (the model, not the UI element!)
    BanTableModel exceptionTable; //!< Table of all bans (the model, not the UI element!)
    BanTableModel inviteTable; //!< Table of all bans (the model, not the UI element!)

    QString defaultTopic; //!< The original/unchanged topic we received from the IRC server.\n If this one is different to the UI element, topic will change when clicking Save button.
    QHash<char,t_csdefault> defaultMode; //!< A list of the original/unchanged channel modes.\n If this one is different to the UI element, modes will change when clicking Save button.

    QStringList maskL; //!< Used when adding lists to a model (ban, exception, invite).\n Do not trust these to have anything you need.
    QStringList dateL; //!< Used when adding lists to a model (ban, exception, invite).\n Do not trust these to have anything you need.
    QStringList authorL; //!< Used when adding lists to a model (ban, exception, invite).\n Do not trust these to have anything you need.

    QString cmA; //!< Modes from isupport CHANMODES, A types.\n Mode that adds or removes a nick or address to a list. Always has a parameter.\n Default: b\n See http://www.irc.org/tech_docs/005.html
    QString cmB; //!< Modes from isupport CHANMODES, B types.\n Mode that changes a setting and always has a parameter.\n Default: k\n See http://www.irc.org/tech_docs/005.html
    QString cmC; //!< Modes from isupport CHANMODES, C types.\n Mode that changes a setting and only has a parameter when set.\n Default: l\n See http://www.irc.org/tech_docs/005.html
    QString cmD; //!< Modes from isupport CHANMODES, D types.\n Mode that changes a setting and never has a parameter.\n default imnpstr\n See http://www.irc.org/tech_docs/005.html

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
