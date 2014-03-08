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

#include "iconnection.h"
#include "iwin.h"
#include "ui_iwin.h"
#include "script/tscriptparent.h"

#include <iostream>

int IWin::winidCount = 1;
int IWin::statusCount = 0; // IdealIRC always starts with no windows, but the init function increments so this should be okay.

IWin::IWin(QWidget *parent, QString wname, int WinType, config *cfg, TScriptParent *sp) :
    QWidget(parent),
    settings(NULL),
    ui(new Ui::IWin),
    WindowType(WinType),
    connection(NULL),
    scriptParent(sp),
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
        textdata = new TIRCView(conf);

        input = new QMyLineEdit(this, conf);
        ui->gridLayout->addWidget(textdata, 0, 0);
        ui->gridLayout->addWidget(input, 1, 0);
        setTabOrder(input, textdata);
        if (WindowType == WT_STATUS) {
            target = "status";
            statusCount++;
        }
        else {
            target = wname;
            if ((conf->logEnabled == true) && (conf->logPM == true)) {
                writeToLog(" ");
                writeToLog("QUERY WINDOW OPENED AT: " + QDateTime::currentDateTime().toString("ddd MMMM d yyyy"));
            }
        }
    }


    if (WindowType == WT_CHANNEL) {
        textdata = new TIRCView(conf);

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

        ui->gridLayout->addWidget(input, 1, 0);

        setTabOrder(input, listbox);
        setTabOrder(listbox, textdata);
        target = wname;

        ui->gridLayout->setContentsMargins(0, 0, 0, 0);

        connect(listbox, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
                this, SLOT(listboxDoubleClick(QListWidgetItem*)));


        opMenu = new QMenu(this);
        opMenu->setTitle("Operator menu");
        opMenu->addAction(ui->actionGive_op);
        opMenu->addAction(ui->actionTake_op);
        opMenu->addSeparator();
        opMenu->addAction(ui->actionGive_voice);
        opMenu->addAction(ui->actionTake_voice);
        opMenu->addSeparator();
        opMenu->addAction(ui->actionKick);
        opMenu->addAction(ui->actionKick_ban);

        listboxMenu = new QMenu(this);
        listboxMenu->addAction(ui->nickmenu_Query);
        listboxMenu->addSeparator();
        listboxMenu->addAction(ui->nickmenu_Whois);
        listboxMenu->addMenu(opMenu);

        connect(listbox, SIGNAL(MenuRequested(QPoint)),
                this, SLOT(listboxMenuRequested(QPoint)));


        connect(textdata, SIGNAL(menuRequested(QPoint)),
                this, SLOT(textboxMenuRequested(QPoint)));


        textboxMenu = new QMenu(this);
        textboxMenu->addAction(ui->actionChannel_settings);


        if ((conf->logEnabled == true) && (conf->logChannel == true)) {
            writeToLog(" ");
            writeToLog("CHANNEL WINDOW OPENED AT: " + QDateTime::currentDateTime().toString("ddd MMMM d yyyy"));
        }
    }

    if (WindowType == WT_TXTONLY) {
        textdata = new TIRCView(conf);
        ui->gridLayout->addWidget(textdata, 0, 0);
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
        connect(textdata, SIGNAL(joinChannel(QString)),
                this, SLOT(joinChannel(QString)));

        connect(textdata, SIGNAL(anchorClicked(const QUrl)),
                this, SLOT(openURL(const QUrl)));

        connect(textdata, SIGNAL(gotFocus()),
                this, SLOT(GiveFocus()));

        textdata->reloadCSS();
    }

    if (picwin != NULL) {
        connect(picwin, SIGNAL(mouseEvent(e_iircevent,int,int,int)),
                this, SLOT(picwinMouseEvent(e_iircevent,int,int,int)));
    }

    if (split != NULL) {
        connect(split, SIGNAL(splitterMoved(int,int)),
                this, SLOT(splitterMoved(int,int)));
    }

    iname = wname.toUpper();
    setObjectName(wname);

}

IWin::~IWin()
{
    textdata = NULL;
    delete ui;
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
        QList<int> sizes = split->sizes();

        int width = sizes[0] + sizes[1];

        int leftw = width - conf->listboxWidth;
        sizes[0] = leftw;
        sizes[1] = conf->listboxWidth;;

        split->setSizes(sizes);
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
        QList<int> sizes = split->sizes();

        int width = sizes[0] + sizes[1];

        int leftw = width - conf->listboxWidth;;
        sizes[0] = leftw;
        sizes[1] = conf->listboxWidth;;

        split->setSizes(sizes);
    }
}

void IWin::focusInEvent(QFocusEvent *)
{
    if (input != NULL)
        input->setFocus();
}

void IWin::inputEnterPushed()
{
    QString text = input->text();
    if (text.length() == 0)
        return; // Do nothing if there's no text.

    input->clear();

    if (text[0] == '/') {

        bool commandOk = cmdhndl->parse(text);

        if (! commandOk) // We didn't have it in internal commands list, check for custom command.
            commandOk = scriptParent->command(text);

        if ((! commandOk) && (connection->isSocketOpen() == true)) {
            // Run this if we didn't have the command, assume it's on the server.
            sockwrite( text.mid(1) );
        }

        if ((! commandOk) && (connection->isSocketOpen() == false)) {
            // We're disconnected and command wasn't found in ICommand nor as custom.
            print(tr("Not connected to server."), PT_LOCALINFO);
        }

        // Do nothing if command wasn't found neither in ICommand or the server.
        // ICommand returns "Not connected to server" for relevant commands.
        return;
    }

    if ((WindowType == WT_STATUS) && (text.at(0) != '/')) {
        print(tr("You're not in a chat window!"), PT_LOCALINFO);
        return;
    }

    if ((WindowType == WT_CHANNEL) || (WindowType == WT_PRIVMSG)) {
        // Reaching here means we've eliminated status window and commands. Send text to chat.
        sockwrite( QString("PRIVMSG %1 :%2")
                     .arg(target)
                     .arg(text)
                  );

        QString mode;
        if (WindowType == WT_CHANNEL) {
            member_t m = ReadMember(connection->getActiveNickname());
            if (m.mode.count() > 0)
                mode += m.mode[0];
        }

        print( QString("<%1%2> %3")
                 .arg(mode)
                 .arg(connection->getActiveNickname())
                 .arg(text),
               PT_OWNTEXT
              );
    }
}

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

void IWin::GiveFocus()
{
    if (input != NULL)
        input->setFocus();
}

void IWin::splitterMoved(int, int)
{
    conf->listboxWidth = listbox->width();
}

void IWin::textboxMenuRequested(QPoint p)
{
    textboxMenu->addActions( scriptParent->getCustomChannelMenu() );

    textboxMenu->popup(p);
}

void IWin::listboxMenuRequested(QPoint p)
{
    listboxMenu->addActions( scriptParent->getCustomNicklistMenu() );

    listboxMenu->popup(p);
}

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

void IWin::print(const QString &text, const int ptype)
{
    if (textdata == NULL)
        return;

    QString msg = text;

    if ((ptype == PT_LOCALINFO) || (ptype == PT_SERVINFO))
        msg.prepend("*** ");
    if ((ptype == PT_ACTION) || (ptype == PT_CTCP))
        msg.prepend("* ");

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

    textdata->addLine(msg, ptype);   

    if ((WindowType == WT_CHANNEL) || (WindowType == WT_PRIVMSG))
        writeToLog(msg);
}

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

bool IWin::memberExist(QString nickname)
{
    return members.contains(nickname);
}

void IWin::memberSetNick(QString nickname, QString newnick)
{
    member_t m = members.value(nickname);
    m.nickname = newnick;
    removeMember(nickname, false);
    insertMember(newnick, m);
}

void IWin::resetMemberlist()
{
    if (listbox == NULL)
        return;

    members.clear();
    memberlist.clear();
    listbox->clear();
}

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

void IWin::MemberUnsetMode(QString nickname, char mode)
{
    member_t mem = ReadMember(nickname);
    mem.mode.removeAll(mode);
    removeMember(nickname);
    insertMember(nickname, mem);
}

member_t IWin::ReadMember(QString nickname)
{
    member_t m = members.value(nickname);
    return m;
}

void IWin::setInputText(QString text)
{
    if (input != NULL)
        input->setText(text);
}

void IWin::setFont(const QFont &font)
{
    if (textdata != NULL)
        textdata->setFont(font);
    if (input != NULL)
        input->setFont(font);
    if (listbox != NULL)
        listbox->setFont(font);
}

void IWin::reloadCSS()
{
    if (textdata != NULL)
        textdata->reloadCSS();

    if (input != NULL)
        input->updateCSS();

    if (listbox != NULL)
        listbox->updateCSS();
}

void IWin::clear()
{
    if (textdata != NULL)
        textdata->clear();
    if (picwin != NULL)
        picwin->clear();
}

void IWin::doGfx(e_painting command, QStringList param)
{
    if (picwin == NULL)
        return;

    if (command == pc_clear) {
        picwin->clear();
        return;
    }

    if (command == pc_paintdot) {
        int X = floor(param[0].toFloat());
        int Y = floor(param[1].toFloat());
        int size = param[2].toInt();
        QColor color(param[3]);

        QBrush br(color, Qt::SolidPattern);
        QPen pn(color);
        pn.setWidth(size);
        picwin->setBrush(br);
        picwin->setPen(pn);
        picwin->paintDot(X, Y);
        return;
    }

    if (command == pc_paintline) {
        int X1 = floor(param[0].toFloat());
        int Y1 = floor(param[1].toFloat());
        int X2 = floor(param[2].toFloat());
        int Y2 = floor(param[3].toFloat());
        int size = param[4].toInt();
        QColor color(param[5]);

        QBrush br(color, Qt::SolidPattern);
        QPen pn(color);
        pn.setWidth(size);
        picwin->setBrush(br);
        picwin->setPen(pn);
        picwin->paintLine(X1, Y1, X2, Y2);
        return;
    }

    if (command == pc_paintrect) {
        int X = floor(param[0].toFloat());
        int Y = floor(param[1].toFloat());
        int W = floor(param[2].toFloat());
        int H = floor(param[3].toFloat());
        int size = param[4].toInt();
        QColor color(param[5]);

        QBrush br(color, Qt::SolidPattern);
        QPen pn(color);
        pn.setWidth(size);
        picwin->setBrush(br);
        picwin->setPen(pn);
        picwin->paintRect(X, Y, W, H);
        return;
    }

    if (command == pc_paintimage) {
        int X = floor(param[0].toFloat());
        int Y = floor(param[1].toFloat());
        QString fn = param[2];
        picwin->paintImage(fn, X, Y);
        return;
    }

    if (command == pc_clearimagebuffer) {
        picwin->clearImageBuffer();
        return;
    }

    if (command == pc_painttext) {
        int X = floor(param[0].toFloat());
        int Y = floor(param[1].toFloat());
        int size = param[2].toInt();
        QColor color(param[3]);

        QString font = param[4];
        QString text = param[5];

        QFont f(font);
        f.setPixelSize(size);
        QBrush br(color, Qt::SolidPattern);
        QPen pn(color);

        picwin->setBrush(br);
        picwin->setPen(pn);
        picwin->paintText(X, Y, f, text);

        return;
    }

    if (command == pc_paintfill) {
        int X = floor(param[0].toFloat());
        int Y = floor(param[1].toFloat());
        int W = floor(param[2].toFloat());
        int H = floor(param[3].toFloat());
        QColor color(param[4]);

        QBrush br(color, Qt::SolidPattern);
        QPen pn(color);
        pn.setWidth(0);
        picwin->setBrush(br);
        picwin->setPen(pn);
        picwin->paintFill(X, Y, W, H);

        return;
    }

    if (command == pc_buffer) {
        bool state = (bool*)param[0].toInt();
        picwin->setViewBuffer(state);

        return;
    }
}

QStringList IWin::getSelectedMembers()
{
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

void IWin::settingsClosed()
{
    disconnect(settings, SIGNAL(closed())); // Disconnect close signal
    delete settings; // delete object from free memory
    settings = NULL; // assign a null pointer to indicate settings not open
}

void IWin::on_actionChannel_settings_triggered()
{
    connection->FillSettings = true;

    settings = new IChanConfig(connection, target, this);
    settings->show();

    settings->adjustSize();
    settings->move(QApplication::desktop()->screen()->rect().center() - settings->rect().center());

    connect(settings, SIGNAL(closed()),
            this, SLOT(settingsClosed()));

    sockwrite(QString("TOPIC %1").arg(target));
}

void IWin::listboxDoubleClick(QListWidgetItem *item)
{
    emit RequestWindow(stripModeChar(item->text()), WT_PRIVMSG, connection->getCid(), true);
}

void IWin::setModeViaList(char set, char mode)
{
    const QList<QListWidgetItem*> sel = listbox->selectedItems();

    int count = 1;
    QString modes = QString(set);
    QString nicks = "";
    QString data;
    for (int i = 0; i <= sel.length()-1; ++i) {
        QString nick = sel[i]->text();
        if (connection->isValidCuLetter(nick[0].toLatin1()))
            nick = nick.mid(1);

        if (nicks.length() > 0)
            nicks.append(' ');

        modes.append(mode);
        nicks.append(nick);
        ++count;
        if (count > connection->maxModes) {
            QString w = QString("MODE %1 %2 %3")
                          .arg(target)
                          .arg(modes)
                          .arg(nicks);
            if (data.length() > 0)
                data.append("\r\n");

            data += w;

            modes = QString(set);
            nicks.clear();
            count = 1;
        }
    }

    if (count > 1) {
        QString w = QString("MODE %1 %2 %3")
                      .arg(target)
                      .arg(modes)
                      .arg(nicks);
        if (data.length() > 0)
            data.append("\r\n");

        data += w;
    }

    listbox->clearSelection();

    connection->sockwrite(data);
}

QString IWin::stripModeChar(QString nickname)
{
    char m = nickname[0].toLatin1();
    if (connection->isValidCuLetter(m))
        nickname = nickname.mid(1); // Nickname have a modechar, skip it.
    return nickname;
}

void IWin::on_actionGive_op_triggered()
{
    setModeViaList('+', 'o');
}

void IWin::on_actionTake_op_triggered()
{
    setModeViaList('-', 'o');
}

void IWin::on_actionGive_voice_triggered()
{
    setModeViaList('+', 'v');
}

void IWin::on_actionTake_voice_triggered()
{
    setModeViaList('-', 'v');
}

void IWin::on_nickmenu_Query_triggered()
{
    QListWidgetItem *item = listbox->currentItem();
    if (item == 0)
        return;

    emit RequestWindow(stripModeChar(item->text()), WT_PRIVMSG, connection->getCid(), true);
}

void IWin::on_nickmenu_Whois_triggered()
{
    QListWidgetItem *item = listbox->currentItem();
    if (item == 0)
        return;

    QString nickname = stripModeChar(item->text());
    sockwrite(QString("WHOIS %1")
              .arg(nickname));
}

void IWin::on_actionKick_triggered()
{
    QListWidgetItem *item = listbox->currentItem();
    if (item == 0)
        return;

    QString reason = QInputDialog::getText(this, tr("Kick reason"), tr("Type a reson for the kick:"));

    QString nickname = stripModeChar(item->text());
    sockwrite(QString("KICK %1 %2 :%3")
                .arg(target)
                .arg(nickname)
                .arg(reason)
              );
}
