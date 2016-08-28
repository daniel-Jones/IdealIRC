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

#include "ichanconfig.h"
#include "ui_ichanconfig.h"
#include "iconnection.h"

#include <QDebug>
#include <QHashIterator>
#include <QMessageBox>
#include <QInputDialog>

/*!
 * \param c The connection the dialog will send data to.
 * \param chan The channel the dialog will send data to.
 * \param parent The IWin that parents this dialog.
 */
IChanConfig::IChanConfig(IConnection *c, QString chan, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IChanConfig),
    connection(c),
    channel(chan)
{
    ui->setupUi(this);

    setWindowTitle(  tr("Channel settings for %1")
                        .arg(chan)
                    );

    // Add default modes to our hash of modes, by default all modes is disabled,
    // but that's changed asap we get the MODE response from server.
    // We will use the lists B, C, D from isupport CHANMODES (If the server didn't provide
    // such list we already got the IRC defaults stored)
    cmA = connection->getcmA();
    cmB = connection->getcmB();
    cmC = connection->getcmC();
    cmD = connection->getcmD();
    for (int i = 0; i <= cmB.length()-1; ++i) {
        char m = cmB[i].toLatin1();
        t_csdefault def;
        def.enabled = false;
        defaultMode.insert(m, def);
    }
    for (int i = 0; i <= cmC.length()-1; ++i) {
        char m = cmC[i].toLatin1();
        t_csdefault def;
        def.enabled = false;
        defaultMode.insert(m, def);
    }
    for (int i = 0; i <= cmD.length()-1; ++i) {
        char m = cmD[i].toLatin1();
        t_csdefault def;
        def.enabled = false;
        defaultMode.insert(m, def);
    }

    loading = new UnsupportedModel("Loading the list...");
    ui->banView->setModel(loading);

    if (connection->haveExceptionList) {
        ui->exceptionView->setModel(loading);
    }
    else {
        UnsupportedModel *m = new UnsupportedModel(tr("This server doesn't support ban exceptions"));
        ui->exceptionView->setModel(m);
        ui->tab_exception->setEnabled(false);
    }


    if (connection->haveInviteList) {
        ui->inviteView->setModel(loading);
    }
    else {
        UnsupportedModel *m = new UnsupportedModel(tr("This server doesn't support invites list"));
        ui->inviteView->setModel(m);
        ui->tab_invite->setEnabled(false);
    }

    ui->inviteView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->exceptionView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->banView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
}

IChanConfig::~IChanConfig()
{
    delete ui;
}

/*!
 * \param mode Modes, for example "+ntl 40"
 *
 * Sets the default channel modes, as received from the IRC server.\n
 * When Save button is pushed, these default modes will be compared against the dialog data.\n
 * If they differ, new modes will be set.
 */
void IChanConfig::setDefaultMode(QString mode)
{
    QStringList param = mode.split(' ');
    int pc = 0; // pc = 0 is actually the modes itself. will increase to get correct parameter if needed.

    bool enabled; // + makes this true, - false

    for (int i = 0; i <= mode.length()-1; ++i) {
        char c = mode[i].toLatin1();

        QString data;
        if (cmB.contains(c))
            data = param[++pc];
        else if ((cmC.contains(c)) && (enabled == true))
            data = param[++pc];

        t_csdefault def;
        def.enabled = enabled;
        def.data = data;
        defaultMode.insert(c, def);

        switch (c) {
            case '+':
                enabled = true;
                continue;
            case '-':
                enabled = false;
                continue;
            case 'n':
                ui->mode_n->setChecked(enabled);
                continue;
            case 't':
                ui->mode_t->setChecked(enabled);
                continue;
            case 'i':
                ui->mode_i->setChecked(enabled);
                continue;
            case 'p':
                ui->mode_p->setChecked(enabled);
                continue;
            case 's':
                ui->mode_s->setChecked(enabled);
                continue;
            case 'm':
                ui->mode_m->setChecked(enabled);
                continue;
            case 'k':
                ui->mode_k->setChecked(enabled);
                ui->mode_k_val->setText(data);
                continue;
            case 'l':
                ui->mode_l->setChecked(enabled);
                ui->mode_l_val->setValue( data.toInt() );
                continue;
            case ' ':
                i = mode.length(); // This will exit the loop.
                continue;
        }
    }
}

void IChanConfig::setMode(char mode, bool enabled, QString data)
{
    t_csdefault def;
    def.enabled = enabled;
    def.data = data;
    defaultMode.insert(mode, def);
}

/*!
 * \param topic Topic
 *
 * Sets the default channel topic, as received from the IRC server.\n
 * When Save button is pushed, this topic will be compared against the dialog data.\n
 * If they differ, new topic will be set.
 */
void IChanConfig::setDefaultTopic(QString topic)
{
    defaultTopic = topic;
    ui->edTopic->setText(topic);
}

/*!
 * \param mask Hostmask
 * \param author Author, who set it
 * \param created Creation time
 * \param mt Mask type
 *
 * Adds a mask to the respective model upon receiving new data to store.\n
 * This is used after models are filled.
 */
void IChanConfig::addMask(QString mask, QString author, QString created, MaskType mt)
{
    if (mt == MT_BAN)
        banTable.addBan(mask, created, author);
    if (mt == MT_EXCEPT)
        exceptionTable.addBan(mask, created, author);
    if (mt == MT_INVITE)
        inviteTable.addBan(mask, created, author);
}

/*!
 * \param mask Hostmask
 * \param author Author, who set it
 * \param created Creation time
 *
 * When dialog is showing up, this function is used when building data to the mask models\n
 * Bans, Exceptions and Invites go through here.
 */
void IChanConfig::addMask(QString mask, QString author, QString created)
{
    maskL << mask;
    authorL << author;
    dateL << created;
}

/*!
 * \param type Mask type
 *
 * When we've reached end of ban list, exception list or invite list,
 * this function is run to send received data to the models for displaying.
 */
void IChanConfig::finishModel(MaskType type)
{
    if (type == MT_BAN) {
        banTable.setBanList(maskL, dateL, authorL);
        ui->banView->setModel(&banTable);
    }
    if (type == MT_EXCEPT) {
        exceptionTable.setBanList(maskL, dateL, authorL);
        ui->exceptionView->setModel(&exceptionTable);
    }
    if (type == MT_INVITE) {
        inviteTable.setBanList(maskL, dateL, authorL);
        ui->inviteView->setModel(&inviteTable);
    }


    maskL.clear();
    authorL.clear();
    dateL.clear();
}

/*!
 * \param mask Hostmask
 * \param mt Mask type
 *
 * Removes a hostmask from a given model.
 */
void IChanConfig::delMask(QString mask, MaskType mt)
{
    if (mt == MT_BAN)
        banTable.delBan(mask);
    if (mt == MT_EXCEPT)
        exceptionTable.delBan(mask);
    if (mt == MT_INVITE)
        inviteTable.delBan(mask);
}

/*!
 * \param type MaskType
 *
 * This function will remove items from ban, exception or invite list based upon which is selected in the respective view.
 */
void IChanConfig::deleteMasks(MaskType type)
{
    QItemSelectionModel *sel = NULL;
    int maxmode = connection->maxModes;
    char modeset; // This one WILL be set properly right below

    if (type == MT_BAN) {
        sel = ui->banView->selectionModel();
        modeset = 'b';
    }
    else if (type == MT_EXCEPT) {
        sel = ui->exceptionView->selectionModel();
        modeset = 'e';
    }
    else if (type == MT_INVITE) {
        sel = ui->inviteView->selectionModel();
        modeset = 'I';
    }
    else
        return; // Invalid type, stop.

    if (sel == NULL)
        return; // Something weird happened. stop.

    if (! sel->hasSelection())
        return; // Nothing is selected. stop.

    QModelIndexList list = sel->selectedRows(0); // Column 0 is masks

    QStringList masks;

    for (int i = 0; i <= list.count()-1; ++i) {
        QModelIndex item = list[i];
        masks << item.data().toString();
    }

    sel->clearSelection();

    QString mode = "-";
    QString param;
    int m = 1;
    for (int i = 0; i <= masks.count()-1; ++i) {
        mode += modeset;
        if (param.count() > 0)
            param.push_back(' ');
        param += masks[i];

        ++m;
        if (m > maxmode) {
            connection->sockwrite( QString("MODE %1 %2 %3")
                                     .arg(channel)
                                     .arg(mode)
                                     .arg(param)
                                   );
            mode = "-";
            param.clear();
            m = 1;
        }
    }
    if ((m <= maxmode) && (mode.count() > 1))
        connection->sockwrite( QString("MODE %1 %2 %3")
                                 .arg(channel)
                                 .arg(mode)
                                 .arg(param)
                              );
}

/*!
 * Button slot for Delete ban.
 */
void IChanConfig::on_banDel_clicked()
{
    deleteMasks(MT_BAN);
}

/*!
 * Button slot for Delete exception.
 */
void IChanConfig::on_exceptionDel_clicked()
{
    deleteMasks(MT_EXCEPT);
}

/*!
 * Button slot for Delete invite.
 */
void IChanConfig::on_inviteDel_clicked()
{
    deleteMasks(MT_INVITE);
}

/*!
 * Button slot for Save.
 */
void IChanConfig::on_btnSave_clicked()
{
    if (ui->edTopic->text() != defaultTopic)
        connection->sockwrite(  QString("TOPIC %1 :%2")
                                .arg(channel)
                                .arg(ui->edTopic->text())
                              );

    if ((ui->mode_k->isChecked()) && (ui->mode_k_val->text().length() == 0)) {
        QMessageBox::warning(this, tr("Cannot set channel key"), tr("Channel key is not filled out."));
        return;
    }

    QString delMode;
    QString addMode;
    QString param;

    QHashIterator<char,t_csdefault> i(defaultMode);
    while (i.hasNext()) {
        i.next();
        char m = i.key();
        t_csdefault def = i.value();

        bool checked = false;

        switch (m) {
        case 'n':
            checked = ui->mode_n->isChecked();
            if (checked != def.enabled) {
                if (checked)
                    addMode += m;
                else
                    delMode += m;
            }
            continue;

        case 't':
            checked = ui->mode_t->isChecked();
            if (checked != def.enabled) {
                if (checked)
                    addMode += m;
                else
                    delMode += m;
            }
            continue;

        case 'i':
            checked = ui->mode_i->isChecked();
            if (checked != def.enabled) {
                if (checked)
                    addMode += m;
                else
                    delMode += m;
            }
            continue;

        case 'p':
            checked = ui->mode_p->isChecked();
            if (checked != def.enabled) {
                if (checked)
                    addMode += m;
                else
                    delMode += m;
            }
            continue;

        case 's':
            checked = ui->mode_s->isChecked();
            if (checked != def.enabled) {
                if (checked)
                    addMode += m;
                else
                    delMode += m;
            }
            continue;

        case 'm':
            checked = ui->mode_m->isChecked();
            if (checked != def.enabled) {
                if (checked)
                    addMode += m;
                else
                    delMode += m;
            }
            continue;

        case 'k':
            checked = ui->mode_k->isChecked();
            if (checked != def.enabled) {
                if (checked) {
                    addMode += m;
                    param += QString(" %1")
                               .arg(ui->mode_k_val->text());
                }
                else
                    delMode += m;
                    param += QString(" %1")
                               .arg(def.data);
            }
            else if ((ui->mode_k_val->text() != def.data) && (checked)) {
                // The +k checkbox wasn't changed but the value was, update.
                delMode += m;
                addMode += m;
                param += QString(" %1 %2")
                           .arg(def.data)
                           .arg(ui->mode_k_val->text());
            }
            continue;

        case 'l':
            checked = ui->mode_l->isChecked();
            if (checked != def.enabled) {
                if (checked) {
                    addMode += m;
                    param += QString(" %1")
                               .arg(ui->mode_l_val->text());
                }
                else
                    delMode += m;
            }
            else if ((ui->mode_l_val->text() != def.data) && (checked)) {
                // The +i checkbox wasn't changed but the value was, update.
                addMode += m;
                param += QString(" %1")
                           .arg(ui->mode_l_val->text());
            }
            continue;

        default:
            continue;
        }
    }

    if (delMode.length() > 0)
        delMode.prepend('-');
    if (addMode.length() > 0)
        addMode.prepend('+');
    QString modedata = QString("%1%2%3")
                         .arg(delMode)
                         .arg(addMode)
                         .arg(param); // Param will always begin with a space anyways.

    if (modedata.length() > 0)
        connection->sockwrite( QString("MODE %1 %2")
                                 .arg(channel)
                                 .arg(modedata)
                              );

}

/*!
 * \param type Mask type
 *
 * This function is called via Add button slots.\n
 * It shows a dialog box to enter a hostmask to add to the respective model.
 */
void IChanConfig::btnAddMask(MaskType type)
{
    char modeset; // This one WILL be set properly right below
    QString title;

    if (type == MT_BAN) {
        title = tr("Enter a ban mask");
        modeset = 'b';
    }
    else if (type == MT_EXCEPT) {
        title = tr("Enter an exception mask");
        modeset = 'e';
    }
    else if (type == MT_INVITE) {
        title = tr("Enter an invite mask");
        modeset = 'I';
    }
    else
        return; // Invalid type, stop.

    QString text = QInputDialog::getText(this, title, tr("Write the mask with following syntax: nick!ident@host.name\r\n*!*@host.name nickname!*@*"));

    if (text.length() == 0)
        return;

    QString data = QString("MODE %1 +%2 %3")
                     .arg(channel)
                     .arg(modeset)
                     .arg(text);

    connection->sockwrite(data);
}

/*!
 * Button slot for Add ban.\n See btnAddMask()
 */
void IChanConfig::on_banAdd_clicked()
{
    btnAddMask(MT_BAN);
}

/*!
 * Button slot for Add exception.\n See btnAddMask()
 */
void IChanConfig::on_exceptionAdd_clicked()
{
    btnAddMask(MT_EXCEPT);
}

/*!
 * Button slot for Add invite.\n See btnAddMask()
 */
void IChanConfig::on_inviteAdd_clicked()
{
    btnAddMask(MT_INVITE);
}

/*!
 * \param type Mask type
 *
 * This function is called via Edit button slots.\n
 * It shows a dialog box to edit a hostmask in the respective model.
 */
void IChanConfig::btnEditMask(MaskType type)
{
    QItemSelectionModel *sel = NULL;
    QString title;
    QString mode;

    if (type == MT_BAN) {
        sel = ui->banView->selectionModel();
        title = tr("Enter a ban mask");
        mode = "-b+b";
    }
    else if (type == MT_EXCEPT) {
        sel = ui->exceptionView->selectionModel();
        title = tr("Enter an exception mask");
        mode = "-e+e";
    }
    else if (type == MT_INVITE) {
        sel = ui->inviteView->selectionModel();
        title = tr("Enter an invite mask");
        mode = "-I+I";
    }
    else
        return; // Invalid type, stop.

    if (! sel->hasSelection())
        return;

    QModelIndex item = sel->selectedRows(0).at(0);
    QString mask = item.data().toString();
    QString text = QInputDialog::getText(this, title,
                                         tr("Write the mask with following syntax: nick!ident@host.name\r\n*!*@host.name nickname!*@*"),
                                         QLineEdit::Normal, mask);
    if (text.length() == 0)
        return;

    if (mask == text)
        return;

    QString data = QString("MODE %1 %2 %3 %4")
                     .arg(channel)
                     .arg(mode)
                     .arg(mask)
                     .arg(text);

    connection->sockwrite(data);
}

/*!
 * Button slot for Edit invite.\n See btnEditMask()
 */
void IChanConfig::on_inviteEdit_clicked()
{
    btnEditMask(MT_INVITE);
}

/*!
 * Button slot for Edit exception.\n See btnEditMask()
 */
void IChanConfig::on_exceptionEdit_clicked()
{
    btnEditMask(MT_EXCEPT);
}

/*!
 * Button slot for Edit ban.\n See btnEditMask()
 */
void IChanConfig::on_banEdit_clicked()
{
    btnEditMask(MT_BAN);
}
