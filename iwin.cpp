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

#include <QWidget>
#include <QMouseEvent>
#include <QShowEvent>
#include <QListWidget>
#include <QList>
#include <QLabel>
#include <QPoint>
#include <QMenu>
#include <QClipboard>
#include <QSignalMapper>
#include <QSplitter>
#include <QDateTime>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDebug>
#include <qalgorithms.h>
#include <QInputDialog>
#include <QMessageBox>

#include "iwin.h"
#include "ui_iwin.h"
#include "iconnection.h"
#include "script/tscriptparent.h"
#include "script/tscript.h"

#include "dcc/dcc_protocols.h"

#include <iostream>

int IWin::winidCount = 1;
int IWin::statusCount = 0; // IdealIRC always starts with no windows, but the init function increments so this should be okay.

/*!
 * \param parent Widget parent, usually the MDI area in IdealIRC class.
 * \param wname Window name.
 * \param WinType Window type (see constants.h for WT_*).
 * \param cfg Pointer to config class (iirc.ini).
 * \param sp Script parent.
 * \param c IRC connection this window sends data to. Scriptable windows can change this.
 */
IWin::IWin(QWidget *parent, QString wname, int WinType, config *cfg, TScriptParent *sp, IConnection *c) :
    QWidget(parent),
    settings(NULL),
    ui(new Ui::IWin),
    WindowType(WinType),
    connection(c),
    scriptParent(sp),
    tstar("***"),
    sstar("*"),
    split(NULL),
    textdata(NULL),
    conf(cfg),
    picwin(NULL),
    listbox(NULL),
    listboxMenu(NULL),
    opMenu(NULL),
    textboxMenu(NULL),
    input(NULL)
{
    ui->setupUi(this);

    resize(parent->width(), parent->height());

    setWindowTitle(wname);
    setObjectName(wname);

    ui->gridLayout->setContentsMargins(0, 0, 0, 0);
    ui->gridLayout->setHorizontalSpacing(0);
    ui->gridLayout->setVerticalSpacing(1);

    // This one will be redefined if required.
    // Default is a custom window
    target = wname;

    dcc = NULL;

    std::cout << "Adding window with type " << WindowType << std::endl;

    winid = winidCount++;

    std::cout << "Id for " << wname.toStdString().c_str() << ": " << winid << std::endl;

    if (WindowType == WT_NOTHING) {
        QLabel *err = new QLabel();
        err->setText(tr("Internal error: No WindowType were defined! (WindowType = WT_NOTHING)"));
        ui->gridLayout->addWidget(err, 0, 0);
    }


    if (WindowType == WT_GRAPHIC) {
        picwin = new TPictureWindow(this);
        connect(picwin, SIGNAL(mouseEvent(e_iircevent,int,int,int)),
                this, SLOT(gwMouseEvent(e_iircevent,int,int,int)));
        ui->gridLayout->addWidget(picwin, 0, 0);

        setWindowIcon( QIcon(":/window/gfx/custom.png") );
    }


    if (WindowType == WT_GWINPUT) {
        picwin = new TPictureWindow(this);
        connect(picwin, SIGNAL(mouseEvent(e_iircevent,int,int,int)),
                this, SLOT(gwMouseEvent(e_iircevent,int,int,int)));
        ui->gridLayout->addWidget(picwin, 0, 0);

        target = wname;

        input = new QMyLineEdit(this, conf);
        ui->gridLayout->addWidget(picwin, 0, 0);
        ui->gridLayout->addWidget(input, 1, 0);
        setTabOrder(input, picwin);
    }



    if ((WindowType == WT_PRIVMSG) || (WindowType == WT_STATUS)) {
        textdata = new IIRCView(conf);

        input = new QMyLineEdit(this, conf);
        ui->gridLayout->addWidget(textdata, 0, 0);
        ui->gridLayout->addWidget(input, 1, 0);
        setTabOrder(input, textdata);
        if (WindowType == WT_STATUS) {
            target = "status";
            regenStatusMenu();
            statusCount++;
        }
        else {
            target = wname;
            regenQueryMenu();
            if ((conf->logEnabled == true) && (conf->logPM == true)) {
                writeToLog(" ");
                writeToLog("QUERY WINDOW OPENED AT: " + QDateTime::currentDateTime().toString("ddd MMMM d yyyy"));
            }
        }
    }

    if (WindowType == WT_CHANNEL) {
        textdata = new IIRCView(conf);

        input = new QMyLineEdit(this, conf);
        listbox = new QMyListWidget(this, conf);
        listbox->setSelectionMode(QAbstractItemView::ExtendedSelection);

        split = new QSplitter(this);
        ui->gridLayout->addWidget(split, 0, 0);
        /* The width of listbox is determined at the event showEvent(); */
        split->addWidget(textdata);
        split->addWidget(listbox);
        split->setCollapsible(1, false);

        connect(split, SIGNAL(splitterMoved(int,int)),
                this, SLOT(splitterMoved(int,int)));

        connect(textdata, SIGNAL(mouseDblClick()),
                this, SLOT(mouseDoubleClick()));

        ui->gridLayout->addWidget(input, 1, 0);

        setTabOrder(input, listbox);
        setTabOrder(listbox, textdata);
        target = wname;

        ui->gridLayout->setContentsMargins(0, 0, 0, 0);

        connect(listbox, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
                this, SLOT(listboxDoubleClick(QListWidgetItem*)));

        regenChannelMenus();

        if ((conf->logEnabled == true) && (conf->logChannel == true)) {
            writeToLog(" ");
            writeToLog("CHANNEL WINDOW OPENED AT: " + QDateTime::currentDateTime().toString("ddd MMMM d yyyy"));
        }
    }

    if (WindowType == WT_TXTINPUT) {
        textdata = new IIRCView(conf);

        input = new QMyLineEdit(this, conf);
        ui->gridLayout->addWidget(textdata, 0, 0);
        ui->gridLayout->addWidget(input, 1, 0);
        setTabOrder(input, textdata);
    }

    if (WindowType == WT_TXTONLY) {
        textdata = new IIRCView(conf);
        ui->gridLayout->addWidget(textdata, 0, 0);
    }

    if (WindowType == WT_DCCSEND) {
        dcc = new DCCSend(DCC_SEND, this, connection->dccinfo);
        DCCSend *dsock = (DCCSend*)dcc;

    }

    if (WindowType == WT_DCCRECV) {
        dcc = new DCCRecv(DCC_RECV, this, connection->dccinfo);
        DCCRecv *dsock = (DCCRecv*)dcc;

    }

    if (WindowType == WT_DCCCHAT) {
        dcc = new DCCChat(DCC_CHAT, this, connection->dccinfo);
        DCCChat *dsock = (DCCChat*)dcc;

        connect(dsock, SIGNAL(Highlight()),
                this, SIGNAL(Highlight(winid,HL_MSG)));

        textdata = new IIRCView(conf);

        input = new QMyLineEdit(this, conf);
        ui->gridLayout->addWidget(textdata, 0, 0);
        ui->gridLayout->addWidget(input, 1, 0);
        setTabOrder(input, textdata);
    }


    /// Bind our widgets if it's set up:

    if (input != 0) {
        QObject::connect(input, SIGNAL(returnPressed()),
                         this, SLOT(inputEnterPushed()));

        QObject::connect(input, SIGNAL(TabKeyPressed()),
                         this, SLOT(tabKeyPushed()));

        acIndex = -2;
        input->acIndex = &acIndex; // Pass a pointer so the input box can reset it when neccessary
    }

    if (textdata != NULL) {

        connect(textdata, SIGNAL(joinChannelMenuRequest(QPoint,QString)),
                this, SLOT(joinChannelMenuRequest(QPoint,QString)));

        connect(textdata, SIGNAL(nickMenuRequest(QPoint,QString)),
                this, SLOT(nickMenuRequest(QPoint,QString)));

        connect(textdata, SIGNAL(anchorClicked(const QUrl)),
                this, SLOT(openURL(const QUrl)));

        connect(textdata, SIGNAL(gotFocus()),
                this, SLOT(GiveFocus()));

        joinChannelTitle = new QAction(&joinChannelMenu);
        joinChannelTitle->setEnabled(false);
        QAction *separator = joinChannelMenu.addSeparator();
        joinChannelAction = new QAction(&joinChannelMenu);
        joinChannelAction->setText(tr("Join channel"));
        QFont f = joinChannelTitle->font();
        f.setBold(true);
        joinChannelTitle->setFont(f);

        joinChannelMenu.addActions(QList<QAction*>() << joinChannelTitle << separator << joinChannelAction);

        connect(joinChannelAction, SIGNAL(triggered()),
                this, SLOT(joinChannelTriggered()));

        textdata->changeFont(conf->fontName, conf->fontSize);
    }

    if (picwin != NULL) {
        connect(picwin, SIGNAL(mouseEvent(e_iircevent,int,int,int)),
                this, SLOT(picwinMouseEvent(e_iircevent,int,int,int)));
    }

    if (split != NULL) {
        connect(split, SIGNAL(splitterMoved(int,int)),
                this, SLOT(splitterMoved(int,int)));

        split->setStretchFactor(0,8);
        split->setStretchFactor(1,1);
    }

    iname = wname.toUpper();
    setObjectName(wname);

    // if this is a DCC window, we should init it now.
    if (dcc != NULL)
        dcc->initialize();
}

IWin::~IWin()
{
    textdata = NULL;
    delete ui;

    if (dcc != NULL) {
        delete dcc;
    }
}

void IWin::closeEvent(QCloseEvent *e)
{
    // use this to deny closing, last status shouldn't be closed.
    //e->ignore();

    // This is a status window trying to be closed and it's the last one. don't close it.
    if ((statusCount == 1) && (WindowType == WT_STATUS)) {
        e->ignore();
        return;
    }

    if (WindowType == WT_STATUS) {
        statusCount--;
        if (connection->isOnline()) {
            int btn = QMessageBox::question(this, tr("Close connection?"),
                                            tr("The connection is online. Do you want to close?"));
            if (btn == QMessageBox::No) {
                e->ignore();
                statusCount++;
                return;
            }

            connection->closeConnection(true);
            e->ignore();
        }
    }

    emit activeWin("STATUS");
    emit closed(winid);
}

void IWin::showEvent(QShowEvent *)
{
    if (WindowType == WT_CHANNEL) {
        /*
        QList<int> sizes = split->sizes();

        int width = sizes[0] + sizes[1];

        int leftw = width - conf->listboxWidth;
        sizes[0] = leftw;
        sizes[1] = conf->listboxWidth;

        split->setSizes(sizes);
        */
    }
    emit activeWin(target);
}

void IWin::hideEvent(QHideEvent *)
{
    emit activeWin("STATUS");
}

void IWin::resizeEvent(QResizeEvent *)
{
    if (WindowType == WT_CHANNEL) {
        /*
        QList<int> sizes = split->sizes();

        int width = sizes[0] + sizes[1];

        int leftw = width - conf->listboxWidth;;
        sizes[0] = leftw;
        sizes[1] = conf->listboxWidth;

        split->setSizes(sizes);
        */
    }
}

void IWin::focusInEvent(QFocusEvent *)
{
    if (input != NULL)
        input->setFocus();
}

/*!
 * \param con New connection
 *
 * Changes the IRC connection to send data to.\n
 * This will also change the command parser to the connection's one.\n
 */
void IWin::setConnectionPtr(IConnection *con)
{
    connection = con;
    cmdhndl = con->getCmdHndlPtr(); // command handler is bound directly to the connection.

    if ((WindowType == WT_PRIVMSG) && (con->isOnline()) && (con->ial.getHost(target) != "")) {
        updateTitleHost();
    }
}

/*!
 * \param newTopic New channel topic
 *
 * This function sets the channel topic in the window title.\n
 * We do not send anything to the IRC server from here. This function is only for storing data.
 */
void IWin::setTopic(QString newTopic)
{
    topic = newTopic;
    setWindowTitle( QString("%1 (%2)")
                      .arg(target)
                      .arg(topic)
                   );
}

/*!
 * Used for WT_PRIVMSG types.\n
 * Sets the hostname in titlebar.
 */
void IWin::updateTitleHost()
{
    setWindowTitle( QString("%1 (%2@%3)")
                      .arg(target)
                      .arg(connection->ial.getIdent(target))
                      .arg(connection->ial.getHost(target))
                   );
}

/*!
 * \param line Reference to text line.
 *
 * This function processes text typed in the text-input.
 */
void IWin::processLineInput(QString &line)
{
    if (WindowType == WT_DCCCHAT) {
        DCCChat *dsock = (DCCChat*)dcc;
        dsock->inputEnterPushed(input->text());
        input->clear();
        return;
    }

    input->clear();

    if (line[0] == '/') {

        bool commandOk = cmdhndl->parse(line);

        if (! commandOk) // We didn't have it in internal commands list, check for custom command.
            commandOk = scriptParent->command(line);

        if (connection == NULL)
            return; // Nothing more to do.

        if ((! commandOk) && (connection->isSocketOpen() == true)) {
            // Run this if we didn't have the command, assume it's on the server.
            sockwrite( line.mid(1) );
        }

        if ((! commandOk) && (connection->isSocketOpen() == false)) {
            // We're disconnected and command wasn't found in ICommand nor as custom.
            print(tstar, tr("Not connected to server."), PT_LOCALINFO);
        }

        // Do nothing if command wasn't found neither in ICommand or the server.
        // ICommand returns "Not connected to server" for relevant commands.
        return;
    }

    if (WindowType == WT_STATUS) {
        print(tstar, tr("You're not in a chat window!"), PT_LOCALINFO);
        return;
    }

    if ((WindowType == WT_CHANNEL) || (WindowType == WT_PRIVMSG)) {
        // Reaching here means we've eliminated status window and commands. Send text to chat.
        sockwrite( QString("PRIVMSG %1 :%2")
                     .arg(target)
                     .arg(line)
                  );

        QString mode;
        if (WindowType == WT_CHANNEL) {
            member_t m = ReadMember(connection->getActiveNickname());
            if (m.mode.count() > 0)
                mode += m.mode[0];
        }

        QString sender = connection->getActiveNickname();
        if (conf->showUsermodeMsg)
            sender.prepend(mode);

        print(sender, line, PT_OWNTEXT);
    }

    scriptParent->runevent(te_input, QStringList()<<objectName()<<line);
}

/*!
 * This function runs when the Enter key is pushed.
 */
void IWin::inputEnterPushed()
{
    QString text = input->text();
    if (text.length() == 0)
        return; // Do nothing if there's no text.

    QStringList process = text.split('\n');
    QStringListIterator i(process);
    while (i.hasNext()) {
        QString line = i.next();
        processLineInput( line );
    }
}

/*!
 * This function runs when TAB key is pushed. Used for auto complete.
 */
void IWin::tabKeyPushed()
{
    if (input == NULL)
        return; // We got no input, ignore.

    if (acIndex == -2)
        return; // By some reason when connecting this function is ran twice.
                // Starting to type in the input will unlock this though.

    if (acIndex < 0) {
        acPatt = input->acPhrase();

        acMatch.clear();
        for (int i = 0; i <= acList.length()-1; i++) {
            QString item = acList[i];
            if (item.startsWith(acPatt, Qt::CaseInsensitive))
                acMatch << item;
        }

        if (connection != NULL) {
            QStringList chanAcList = connection->getAcList();

            for (int i = 0; i <= chanAcList.length()-1; i++) {
                QString item = chanAcList[i];
                if (item.startsWith(acPatt, Qt::CaseInsensitive))
                    acMatch << item;
            }
        }

        acMatch.sort();

        if (acMatch.length() == 0)
            return; // Nothing to do.

        acIndex = 0;
        acCursor = input->acBegin();
        acPre = input->text().mid(0, acCursor);
        acPost = input->text().mid(acCursor+acPatt.length());
    }

    QString match = acMatch[acIndex];
    input->setText(QString(acPre+match+acPost));
    input->setCursorPosition(acCursor + match.length());
    acIndex++;
    if (acIndex == acMatch.length())
        acIndex = 0; // Reset since we're out of index.
}

/*!
 * \param channel Channel name to join.
 *
 * Joins a channel.
 */
void IWin::joinChannel(QString channel)
{
    connection->sockwrite(QString("JOIN :%1").arg(channel));
}

void IWin::pmUser(QString nickname)
{
    std::cout << "PM USER=\"" << nickname.toStdString().c_str() << "\"" << std::endl;
}

void IWin::openURL(const QUrl url)
{
    QString surl = url.toString();
    if (surl.split(':')[0] == "channel")
        joinChannel(surl.split(':')[1]);
    else if (surl.split(':')[0] == "pm")
        pmUser(surl.split(':')[1]);
    else
        QDesktopServices::openUrl(url);
}

/*!
 * This function attempts to give the text-input the focus.
 */
void IWin::GiveFocus()
{
    if (input != NULL)
        input->setFocus();
}

void IWin::splitterMoved(int, int)
{
    conf->listboxWidth = listbox->width();
}

/*!
 * \param p Global coordinate for menu popup
 *
 * This function determines what menu to popup based on window type.\n
 * Menu will also popup at the coordinate.
 */
void IWin::textboxMenuRequested(QPoint p)
{
    if (WindowType == WT_CHANNEL)
        scriptParent->populateMenu( textboxMenu, 'c' );
    else if (WindowType == WT_STATUS)
        scriptParent->populateMenu( textboxMenu, 's' );
    else if (WindowType == WT_PRIVMSG)
        scriptParent->populateMenu( textboxMenu, 'q' );
    else
        return;

    textboxMenu->popup(p);
}

/*!
 * \param p Global coordinate for menu popup
 *
 * This function determines what menu to popup based on window type.\n
 * Menu will also popup at the coordinate.
 */
void IWin::listboxMenuRequested(QPoint p)
{
    if (WindowType == WT_CHANNEL)
        scriptParent->populateMenu( listboxMenu, 'n' );
    else
        return;

    listboxMenu->popup(p);
}

/*!
 * \param text Text to log
 *
 * If logging is enabled, this function will write to a text log.
 */
void IWin::writeToLog(QString text)
{
    if (! conf->logEnabled)
        return;

    QString LogFile = QString("%1/%2.txt")
                        .arg(conf->logPath)
                        .arg(target);

    QFile f(LogFile);

    if (! f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
        return;

    QByteArray out;
    out.append(text + '\n');
    f.write(out);
    f.close();
}

/*!
 * \param sender Sender of text (text in left margin)
 * \param text Text to print
 * \param ptype Type of text (see constants.h for PT_*)
 *
 * Prints a text in this window.\n
 * If there's nowhere to print, this function does nothing.
 */
void IWin::print(const QString &sender, const QString &text, const int ptype)
{
    if (textdata == NULL)
        return;

    QString msg;

    if ((ptype == PT_LOCALINFO) || (ptype == PT_SERVINFO))
        msg = QString("%1 %2").arg(sender).arg(text);
    else if (ptype == PT_ACTION)
        msg = QString("* %1 %2").arg(sender).arg(text);
    else
        msg = QString("<%1> %2").arg(sender).arg(text);

    if ((ptype == PT_ACTION) || (ptype == PT_CTCP))
        emit Highlight(winid, HL_MSG);
    else if (ptype == PT_NOTICE)
        emit Highlight(winid, HL_HIGHLIGHT);
    else
        emit Highlight(winid, HL_ACTIVITY);

    if (conf->showTimestmap) {
        QString stamp = QDateTime::currentDateTime().toString( conf->timestamp );
        stamp.append(' ');
        msg.prepend(stamp);
    }

    textdata->addLine(sender, text, ptype);

    if ((WindowType == WT_CHANNEL) || (WindowType == WT_PRIVMSG))
        writeToLog(msg);

    if (WindowType == WT_PRIVMSG)
        updateTitleHost();
}

/*!
 * \param nickname Nickname to add
 * \param mt Member data
 * \param sort If true, sorts and adds to the listbox.
 *
 * This function is meant for WT_CHANNEL types.\n
 * When we're joining a channel, this function runs for all nicknames with sort = false.\n
 * When we're already in a channel, and other members are joining, we'll run this with sort = true.
 */
void IWin::insertMember(QString nickname, member_t mt, bool sort)
{
    QString item = nickname;
    if (mt.mode.length() > 0)
        item.prepend(mt.mode[0]);

    memberlist << item;
    members.insert(nickname, mt);

    acList << nickname;

    if (sort)
        sortMemberList(); // This function add new member to listbox
}

/*!
 * \param nickname Nickname to remove
 * \param sort If true, sorts the listbox making nickname disappear.
 *
 * This function is meant for WT_CHANNEL types.\n
 * When a member leaves (Part, kick, quit), this function runs.
 */
void IWin::removeMember(QString nickname,  bool sort)
{
    // Remove from member stringlist
    // Readding to listbox via sort
    member_t m = members.value(nickname);
    members.remove(nickname);
    if (m.mode.length() > 0)
        nickname.prepend(m.mode[0]);

    memberlist.removeAll(nickname);
    acList.removeAll(nickname);

    if (sort)
        sortMemberList(nickname);
}

/*!
 * \param nickname Nickname to check
 *
 * This function is meant for WT_CHANNEL types.\n
 * Tests if a given nickname is registered in this window.\n
 * \return true if exists, false otherwise
 */
bool IWin::memberExist(QString nickname)
{
    return members.contains(nickname);
}

/*!
 * \param nickname Who changes their nickname
 * \param newnick The new nickname
 *
 * This function is meant for WT_CHANNEL types.\n
 * Sets a new nickname on the specified member.
 */
void IWin::memberSetNick(QString nickname, QString newnick)
{
    member_t m = members.value(nickname);
    m.nickname = newnick;
    removeMember(nickname, false);
    insertMember(newnick, m);
}

/*!
 * This function is meant for WT_CHANNEL types.\n
 * Resets the listbox.
 */
void IWin::resetMemberlist()
{
    if (listbox == NULL)
        return;

    members.clear();
    memberlist.clear();
    listbox->clear();
}

/*!
 * \param memberRemoved If a member is being removed, specify here.
 *
 * This function is meant for WT_CHANNEL types.\n
 * Sorts and updates the listbox.
 */
void IWin::sortMemberList(QString memberRemoved)
{
    // This sort uses the Insertion method.
    // Loop through the memberlist stringlist, sort it.
    for (int i = 0; i < memberlist.count(); i++) {
        for (int pos = i; ((pos > 0) && sortLargerThan( memberlist[pos-1], memberlist[pos] )); pos--) {

            QString a = memberlist[pos];
            QString b = memberlist[pos-1];
            memberlist.replace(pos,b);
            memberlist.replace(pos-1,a);
        }
    }

    // Clear listbox and re-add new sorted list
    listbox->clear();
    listbox->addItems(memberlist);

    // Selected items. Re-select after sort.
    const QList<QListWidgetItem*> si = listbox->selectedItems();

    // Re-select from "si"
    for (int i = 0; i <= listbox->count()-1; i++) {
        QString text = listbox->item(i)->text();
        QString stext = stripModeChar(text);
        if (stext == memberRemoved)
            continue;

        for (int j = 0; j <= si.length()-1; j++)
            if (text == si[j]->text())
                listbox->item(i)->setSelected(true);
    }
}

/*!
 * \param lst Pointer to list
 *
 * This function is meant for WT_CHANNEL types.\n
 * Sorts a list of chars. Meant for sort-rule list.
 */
void IWin::sortList(QList<char> *lst)
{
    for (int i = 0; i < lst->count(); i++) {
        for (int pos = i; ((pos > 0) && sortLargerThan(QString(lst->at(pos-1)), QString(lst->at(pos)))); pos--) {
            QString a = QString(lst->at(pos-1));
            QString b = QString(lst->at(pos));
            lst->replace(pos-1, b[0].toLatin1());
            lst->replace(pos, a[0].toLatin1());
        }
    }
}

/*!
 * \param s1 String 1
 * \param s2 String 2
 *
 * This function is meant for WT_CHANNEL types.\n
 * Tests if s1 comes before s2.
 * \return true if s1 is before s2, otherwise false.
 */
bool IWin::sortLargerThan(const QString s1, const QString s2)
{
    // Grab smallest length.
    int l = s1.length();
    if (s2.length() < l)
        l = s2.length();

    for (int i = 0; i <= l-1; i++) {
        int As = sortrule->indexOf(s1[i].toLatin1());
        int Bs = sortrule->indexOf(s2[i].toLatin1());

        if (As < Bs)
            return false;
        if (As > Bs)
            return true;
    }

    // This returns when the texts are equal.
    return false;
}

/*!
 * \param nickname Nickname
 * \param mode Mode
 *
 * This function is meant for WT_CHANNEL types.\n
 * Sets a new mode on specified nickname.
 */
void IWin::MemberSetMode(QString nickname, char mode)
{
    member_t mem = ReadMember(nickname);
    if (mem.mode.contains(mode))
        return; // Got this mode alerady, stop. This is likely not to happen, though.

    // Reaching here means we can safely add the new mode
    mem.mode << mode;
    sortList(&mem.mode);
    removeMember(nickname);
    insertMember(nickname, mem);
}

/*!
 * \param nickname Nickname
 * \param mode Mode
 *
 * This function is meant for WT_CHANNEL types.\n
 * Removes a mode on specified nickname.
 * \return
 */
void IWin::MemberUnsetMode(QString nickname, char mode)
{
    member_t mem = ReadMember(nickname);
    mem.mode.removeAll(mode);
    removeMember(nickname);
    insertMember(nickname, mem);
}

/*!
 * \param nickname Nickname
 *
 * This function is meant for WT_CHANNEL types.\n
 * Finds member-data of nickname
 * \return Returns member-data
 */
member_t IWin::ReadMember(QString nickname)
{
    member_t m = members.value(nickname);
    return m;
}

/*!
 * \param text Text
 *
 * Sets text on the input box.
 */
void IWin::setInputText(QString text)
{
    if (input != NULL)
        input->setText(text);
}

/*!
 * \param font Font
 *
 * Changes font on all widgets used on this IWin.
 */
void IWin::setFont(const QFont &font)
{
    if (textdata != NULL)
        textdata->setFont(font);
    if (input != NULL)
        input->setFont(font);
    if (listbox != NULL)
        listbox->setFont(font);
}

/*!
 * Function name is misleading, as there is no CSS used no more.\n
 * However, use this to re-load colors and fonts on related widgets on this IWin.
 */
void IWin::reloadCSS()
{
    /*
     * CSS is no longer in use, function needs a rename.
    */
    if (textdata != NULL)
        textdata->redraw();

    if (input != NULL)
        input->updateCSS();

    if (listbox != NULL)
        listbox->updateCSS();
}

/*!
 * Clears the widget (not the listbox if its a channel).
 */
void IWin::clear()
{
    if (textdata != NULL)
        textdata->clear();

    if (picwin != NULL)
        picwin->clear();
}

/*!
 * Produces a list of all selected members in the listbox.
 * \return List of selected members.
 */
QStringList IWin::getSelectedMembers()
{
    if (listbox == NULL)
        return QStringList();

    const QList<QListWidgetItem*> sel = listbox->selectedItems();
    QStringList list;
    for (int i = 0; i <= sel.length()-1; ++i) {
        QString nick = sel[i]->text();
        if (connection->isValidCuLetter(nick[0].toLatin1()))
            nick = nick.mid(1);
        list << nick;
    }

    return list;
}

/*!
 * \param event Script event
 * \param x X coordinate
 * \param y Y coordinate
 * \param delta Mouse scroll delta. (optional)
 *
 * Passes event to script parent.\n
 * If the event is mouse scroll, delta is relevant.
 */
void IWin::picwinMouseEvent(e_iircevent event, int x, int y, int delta)
{
    if (event == te_mousemiddleroll) {
        QString X = QString::number(x);
        QString Y = QString::number(y);
        QString Delta = QString::number(delta);
        scriptParent->runevent(event, QStringList()<<objectName()<<X<<Y<<Delta);
    }
    else {
        QString X = QString::number(x);
        QString Y = QString::number(y);
        scriptParent->runevent(event, QStringList()<<objectName()<<X<<Y);
    }
}

/*!
 * This function is meant for WT_CHANNEL types.\n
 * When the channel settings dialog os closed, this function runs.
 */
void IWin::settingsClosed()
{
    disconnect(settings, SIGNAL(closed())); // Disconnect close signal
    delete settings; // delete object from free memory
    settings = NULL; // assign a null pointer to indicate settings not open
}

/*!
 * This function is meant for WT_CHANNEL types.\n
 * Constructs and opens the channel settings dialog.
 */
void IWin::execChanSettings()
{
    if (WindowType != WT_CHANNEL)
        return;

    connection->FillSettings = true;

    settings = new IChanConfig(connection, target, this);
    settings->show();

    settings->adjustSize();
    settings->move(QApplication::desktop()->screen()->rect().center() - settings->rect().center());

    connect(settings, SIGNAL(closed()),
            this, SLOT(settingsClosed()));

    sockwrite(QString("TOPIC %1").arg(target));
}

/*!
 * \return Width of listbox.
 */
int IWin::listboxWidth()
{
    if (listbox == NULL)
        return 0;
    return listbox->width();
}

/*!
 * \return Height of listbox.
 */
int IWin::listboxHeight()
{
    if (listbox == NULL)
        return 0;
    return listbox->height();
}

/*!
 * This function is meant for WT_CHANNEL types.\n
 * When double click on a listbox item, a query window is constructed.
 */
void IWin::listboxDoubleClick(QListWidgetItem *item)
{
    emit RequestWindow(stripModeChar(item->text()), WT_PRIVMSG, connection->getCid(), true);
}

/*!
 * \param nickname Nickname possibly prepended with mode letter.
 *
 * Removes any mode letters on nickname.
 * \return Stripped nickname.
 */
QString IWin::stripModeChar(QString nickname)
{
    char m = nickname[0].toLatin1();
    if (connection->isValidCuLetter(m))
        nickname = nickname.mid(1); // Nickname have a modechar, skip it.
    return nickname;
}

/*!
 * This function is meant for WT_CHANNEL types.\n
 * Regenerates a channel menu, to be filled from script parent.
 */
void IWin::regenChannelMenus()
{
    if (listboxMenu != NULL)
        delete listboxMenu;
    listboxMenu = new QMenu(this);

    if (textboxMenu != NULL)
        delete textboxMenu;
    textboxMenu = new QMenu(this);

    connect(listbox, SIGNAL(MenuRequested(QPoint)),
            this, SLOT(listboxMenuRequested(QPoint)));

    connect(textdata, SIGNAL(menuRequested(QPoint)),
            this, SLOT(textboxMenuRequested(QPoint)));
}

/*!
 * This function is meant for WT_PRIVMSG types.\n
 * Regenerates a query menu, to be filled from script parent.
 */
void IWin::regenQueryMenu()
{
    if (textboxMenu != NULL)
        delete textboxMenu;
    textboxMenu = new QMenu(this);

    connect(textdata, SIGNAL(menuRequested(QPoint)),
            this, SLOT(textboxMenuRequested(QPoint)));
}

/*!
 * This function is meant for WT_STATUS types.\n
 * Regenerates a status menu, to be filled from script parent.
 */
void IWin::regenStatusMenu()
{
    if (textboxMenu != NULL)
        delete textboxMenu;
    textboxMenu = new QMenu(this);

    connect(textdata, SIGNAL(menuRequested(QPoint)),
            this, SLOT(textboxMenuRequested(QPoint)));
}

void IWin::mouseDoubleClick()
{
    if (WindowType == WT_CHANNEL)
        execChanSettings();
}

/*!
 * \param point Mouse coordinate
 * \param channel Channel name
 *
 * When a channel name is clicked inside IIRCView, this slot is signaled from there.\n
 * Here we'll show a menu making it possible to join with mouse clicking.
 */
void IWin::joinChannelMenuRequest(QPoint point, QString channel)
{
    joinChannelTitle->setText(channel);
    joinChannelMenu.popup( QWidget::mapToGlobal(point) );
}

void IWin::joinChannelTriggered()
{
    joinChannel( joinChannelTitle->text() );
}

/*!
 * \param point Mouse coordinate
 * \param nickname Nickname
 *
 * When a nickname is clicked in the left margin inside IIRCView, this slot is signaled from there.\n
 * This will only work inside channel windows.\n
 * A listbox menu will appear at the mouse.
 */
void IWin::nickMenuRequest(QPoint point, QString nickname)
{
    // 'nickname' may be prepended with a mode letter! (+/@/...)
    // So we fix this here:
    QString nickNM = nickname; // create a copy with 'No Mode'
    if (connection->isValidCuLetter(nickNM[0].toLatin1()))
        nickNM.remove(0, 1);

    if (WindowType == WT_CHANNEL) {
        listbox->clearSelection();
        // TODO  if the selected nickname got a changed mode, it won't show a menu.
        QList<QListWidgetItem*> item = listbox->findItems(nickname, Qt::MatchFixedString);
        if (item.count() == 0)
            return;
        item[0]->setSelected(true);
        listboxMenuRequested( QWidget::mapToGlobal(point) );
    }
}
