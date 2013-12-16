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

#include <iostream>
#include <QTreeWidgetItem>
#include <QHashIterator>
#include <QMapIterator>
#include <QDebug>
#include <QPalette>
#include <QMessageBox>

#include "idealirc.h"
#include "ui_idealirc.h"

#include "iabout.h"

IdealIRC::IdealIRC(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::IdealIRC),
    firstShow(true),
    confDlg(&conf, this),
    connectionsRemaining(-1),
    preventSocketAction(false),
    reconnect(NULL)
{
    ui->setupUi(this);

    ui->menuIIRC->addAction(ui->actionConnect);
    ui->menuIIRC->addAction(ui->actionOptions);
    ui->menuTools->addAction(ui->actionChannel_favourites);
    ui->menuTools->addAction(ui->actionChannels_list);
    ui->menuTools->addAction(ui->actionScript_Manager);

    setWindowTitle("IdealIRC " + QString(VERSION_STRING));
    //QApplication

    conf.rehash();

    connect(&confDlg, SIGNAL(connectToServer(bool)),
            this, SLOT(extConnectServer(bool)));

    setGeometry(conf.mainWinGeo);
    if (conf.maximized)
      setWindowState(Qt::WindowMaximized);

    connect(&vc, SIGNAL(gotVersion()),
            this, SLOT(versionReceived()));

    if (conf.checkVersion)
        vc.runChecker();
}

IdealIRC::~IdealIRC()
{
    delete ui;
}

void IdealIRC::showEvent(QShowEvent *)
{
    /// Insert stuff that should run every showEvent here:




    // --
    if (! firstShow) return;
    firstShow = false;

    /// Insert stuff that should run when IIRC shows for first time here:
    ui->treeWidget->setSortingEnabled(true);
    ui->treeWidget->sortItems(0, Qt::AscendingOrder);

    CreateSubWindow("Status", WT_STATUS, 0, true);

    // ui->splitter
}

void IdealIRC::closeEvent(QCloseEvent *e)
{
    if (connectionsRemaining > 0) {
        e->ignore(); // Still waiting for connections to close...
        return;
    }


    conf.maximized = isMaximized();
    conf.save();

    if (connectionsRemaining == 0) {
        e->accept(); // All connections closed, exit.
        return;
    }

    // Iterate through the connections and find the active ones and close them.
    // Remove and clean up their children too (windows and status)
    QHashIterator<int,IConnection*> ic(conlist);
    while (ic.hasNext()) {
        ic.next();
        IConnection *con = ic.value();
        if (con->isSocketOpen()) {
            if (connectionsRemaining == -1)
                connectionsRemaining = 0;
            ++connectionsRemaining;
            con->closeConnection(true);
        }

    }
}

void IdealIRC::resizeEvent(QResizeEvent *e)
{
    QMainWindow::resizeEvent(e);
    if (e->type() == QResizeEvent::Resize) {
     // winlist.value(activewin).window->setGeometry(0,0,ui->cont->width(), ui->cont->height());
    }

    conf.mainWinGeo = geometry();

    QList<int> sz;
    sz.push_back(conf.treeWidth);
    sz.push_back(ui->centralWidget->width() - conf.treeWidth);
    ui->splitter->setSizes(sz);
}

void IdealIRC::moveEvent(QMoveEvent *)
{
    conf.mainWinGeo = geometry();
}

bool IdealIRC::WindowExists(QString name, int parent)
{
    QHashIterator<int, subwindow_t> i(winlist);
    while (i.hasNext()) {
        i.next();

        subwindow_t sw = i.value();

        if ((sw.widget->objectName().toUpper() == name.toUpper()) && (i.value().parent == parent)) {
            if (sw.type == WT_STATUS)
                return false;
            else
                return true;
        }
    }


    return false;
}

void IdealIRC::subWinClosed(int wid)
{
    subwindow_t empty;
    empty.wid = -1;

    subwindow_t sw = winlist.value(wid, empty);

    if (sw.wid == -1)
        return; // Nope.

    std::cout << "Closing " << sw.widget->objectName().toStdString().c_str() << " (" << wid << ")" << std::endl;

    if (sw.type == WT_STATUS) {
        // Closing a status window.
        QList<QMdiSubWindow*> closeList; // List of subwindows for this status to close
        QHashIterator<int,subwindow_t> i(winlist); // Iterate all windows
        IConnection *con = conlist.value(wid); // Remember window id of status is always equal to connection id. KISS!
        while (i.hasNext()) {
            i.next();
            subwindow_t s = i.value();
            // This item may not be a subwindow of the status that's closing.

            if (s.parent == wid) {
                // This item is a child of the status
                closeList.push_back(s.subwin);
                con->freeWindow( s.widget->objectName() );
            }
        }

        if (con->isSocketOpen())
            con->closeConnection(); // Begin closing the socket.

        // Close all its subwindows
        int count = closeList.count()-1;
        std::cout << "  count : " << count << std::endl;
        for (int i = 0; i <= count; i++) {
            QMdiSubWindow *w = closeList.at(i);
            w->close();
        }

        con->freeWindow("STATUS"); // Free status window lastly
        if (! con->isSocketOpen())
            delete con; // Socket wasn't open, delete connection object

    }

    else {
        IConnection *con = conlist.value(sw.parent);
        con->freeWindow( sw.widget->objectName() );
        winlist.remove(sw.wid);
    }

    if (sw.type == WT_CHANNEL) {
        IConnection *con = conlist.value(sw.parent);
        con->sockwrite("PART :" + sw.widget->objectName() );
    }

    ui->treeWidget->removeItemWidget(sw.treeitem, 0);
    delete sw.treeitem;
    delete sw.widget;
    winlist.remove(wid);


    /**
       @note Do we need to delete the other pointers from subwindow_t ?
    **/
}

// Returns -1 if failed, otherwise window id.
// ID 0 is reserved to define "no parents" under 'int parent'.
//                       Name in treeview   Window type  wid parent    activate on creation
int IdealIRC::CreateSubWindow(QString name, int type, int parent, bool activate)
{

    if (WindowExists(name, parent) == true)
        return -1;

    qDebug() << "Creating new subwindow type " << type << " name " << name;

    IWin *s = new IWin(ui->mdiArea, name, type, &conf);

    IConnection *connection = conlist.value(parent, NULL);
    if (type == WT_STATUS) {
        qDebug() << "Window is status, new connection added with id " << s->getId();
        connection = new IConnection(this, s->getId(), &conf);
        connection->setActiveInfo(&activeWname, &activeConn);
        connect(connection, SIGNAL(connectionClosed()),
                this, SLOT(connectionClosed()));
    }

    qDebug() << "Connection added, setting pointers";

    s->setConnectionPtr(connection);
    s->setSortRuleMap(connection->getSortRuleMapPtr());

    qDebug() << "Pointers set, setting up mdiSubWindow";

    QMdiSubWindow *previous = ui->mdiArea->currentSubWindow();
    QMdiSubWindow *sw = ui->mdiArea->addSubWindow(s, Qt::SubWindow);

    qDebug() << "Pointer to subwindow: " << sw;

    qDebug() << "Icon...";

    // Add icon to window
    QString ico = ":/window/gfx/custom.png"; // Default to this.
    if (type == WT_PRIVMSG)
      ico = ":/window/gfx/query.png";
    if (type == WT_CHANNEL)
      ico = ":/window/gfx/channel.png";
    if (type == WT_STATUS)
      ico = ":/window/gfx/status.png";
    sw->setWindowIcon(QIcon(ico));

    qDebug() << "Treeitem...";

    QTreeWidgetItem *treeitem = GetWidgetItem(parent);

    if (treeitem == NULL)
        treeitem = new QTreeWidgetItem(ui->treeWidget);
    else
        treeitem = new QTreeWidgetItem(treeitem);


    treeitem->setIcon(0, QIcon(ico));
    treeitem->setText(0, name);

    qDebug() << "subwindow_t instance...";

    subwindow_t wt;
    wt.connection = connection;
    wt.parent = parent;
    wt.subwin = sw;
    wt.treeitem = treeitem;
    wt.type = type;
    wt.wid = s->getId();
    wt.widget = s;
    wt.highlight = HL_NONE;

    qDebug() << "Adding subwindow_t to winlist...";

    winlist.insert(s->getId(), wt);

    sw->setGeometry(0, 0, 500, 400);

    if (type == WT_STATUS) {
        qDebug() << "Adding connection to the list...";
        // The Connection class ID is the exact same ID as the status window ID.
        conlist.insert(s->getId(), connection);
        treeitem->setExpanded(true);
        connection->addWindow("STATUS", wt);
        connect(connection, SIGNAL(RequestWindow(QString,int,int,bool)),
                this, SLOT(CreateSubWindow(QString,int,int,bool)));
        connect(connection, SIGNAL(HighlightWindow(int,int)),
                this, SLOT(Highlight(int,int)));
    }

    qDebug() << "Adding this window to the connections window list...";

    connection->addWindow(name.toUpper(), wt);

    qDebug() << "Passing the command handler...";

    s->setCmdHandler( connection->getCmdHndlPtr() );

    qDebug() << "Different signals...";

    connect(s, SIGNAL(closed(int)),
            this, SLOT(subWinClosed(int)));

    connect(s, SIGNAL(doCommand(QString)),
            connection->getCmdHndlPtr(), SLOT(parse(QString)));

    connect(s, SIGNAL(sendToSocket(QString)),
            connection, SLOT(sockwrite(QString)));

    connect(connection, SIGNAL(updateConnectionButton()),
            this, SLOT(updateConnectionButton()));

    connect(s, SIGNAL(RequestWindow(QString,int,int,bool)),
            this, SLOT(CreateSubWindow(QString,int,int,bool)));

    connect(s, SIGNAL(Highlight(int,int)),
            this, SLOT(Highlight(int,int)));

    qDebug() << "Determining to activate window...";

    if (previous == 0) {
        sw->showMaximized();
        s->setFocus();
    }
    else {
        sw->show();
        if (! activate)
            ui->mdiArea->setActiveSubWindow(previous);
    }

    qDebug() << "'- activate=" << activate;


    qDebug() << "Returning with window id.";
    return s->getId();
}

QTreeWidgetItem* IdealIRC::GetWidgetItem(int wid)
{
    if (wid == 0)
        return NULL;

    if (winlist.contains(wid) == false)
        return NULL;

    subwindow_t t = winlist.value(wid);
    return t.treeitem;

}

void IdealIRC::on_mdiArea_subWindowActivated(QMdiSubWindow *arg1)
{
    QHashIterator<int,subwindow_t> i(winlist);

    //QString wname;

    while (i.hasNext()) {
        i.next();
        subwindow_t sw = i.value();

        if (sw.subwin == arg1) {
            //wname = sw.widget->objectName();
            activeWid = sw.wid;
            sw.treeitem->setForeground(0, QBrush(Qt::black));
            sw.highlight = HL_NONE;
            ui->treeWidget->setCurrentItem(sw.treeitem);
            updateConnectionButton();
            winlist.insert(sw.wid, sw);
            break;
        }
    }
}

void IdealIRC::on_treeWidget_itemSelectionChanged()
{
    QTreeWidgetItem *item = ui->treeWidget->currentItem();

    QHashIterator<int,subwindow_t> i(winlist);
    while (i.hasNext()) {
        i.next();
        subwindow_t sw = i.value();

        if (sw.treeitem == item) {
            ui->mdiArea->setActiveSubWindow( sw.subwin );
            activeWid = i.key();
            activeWname = sw.widget->objectName();
            activeConn = sw.parent;
            sw.widget->setFocus();
            std::cout << "activeWid=" << activeWid << std::endl;
        }
    }
}

void IdealIRC::on_actionOptions_triggered()
{
    confDlg.show();
    confDlg.activateWindow();
}

void IdealIRC::extConnectServer(bool newWindow)
{
    std::cout << "Server: " << conf.server.toStdString().c_str() << " | pw: " << conf.password.toStdString().c_str() << std::endl;

    if (newWindow)
        activeWid = CreateSubWindow("Status", WT_STATUS, 0, true);

    subwindow_t current = winlist.value(activeWid);

    if (current.parent == 0) {
        // Connect to this status window
        IConnection *con = conlist.value(current.wid);
        if (con->isSocketOpen()) {
            reconnect = con;
            con->closeConnection();
        }
        else
            con->tryConnect();
    }
    else {
        // Connect to current.parent
        subwindow_t parent = winlist.value(current.parent);
        IConnection *con = conlist.value(parent.wid);
        if (con->isSocketOpen()) {
            reconnect = con;
            con->closeConnection();
        }
        else
            con->tryConnect();
    }
}

void IdealIRC::updateConnectionButton()
{
    preventSocketAction = true;

    subwindow_t sw = winlist.value(activeWid);

    if ((sw.type != WT_CHANNEL) &&
        (sw.type != WT_PRIVMSG) &&
        (sw.type != WT_STATUS)) {

        ui->actionConnect->setChecked(false);
        ui->actionConnect->setEnabled(false);
        preventSocketAction = false;
        return;
    }

    IConnection *c = sw.widget->getConnection();
    bool open = false;
    if (c != NULL)
        open = c->isSocketOpen();

    if (open)
        ui->actionConnect->setChecked(true);
    else
        ui->actionConnect->setChecked(false);
    preventSocketAction = false;
}

void IdealIRC::on_actionConnect_toggled(bool arg1)
{
    if (arg1 == true)
        ui->actionConnect->setText("Disconnect");
    else
        ui->actionConnect->setText("Connect");

    if (preventSocketAction)
        return;

    subwindow_t sw = winlist.value(activeWid);
    IConnection *c = sw.widget->getConnection();

    if (arg1 == true) {
        // connect
        c->tryConnect();

    }

    if (arg1 == false) {
        // disconnect
        c->closeConnection();

    }
}

void IdealIRC::connectionClosed()
{
    if (reconnect != NULL) {
        reconnect->tryConnect();
        reconnect = NULL;
        return;
    }

    if (connectionsRemaining == -1)
        return;

    if (connectionsRemaining != -1)
        --connectionsRemaining;

    close(); // Attempt closing. Will be ignored if some connections are still left.
}

void IdealIRC::on_treeWidget_clicked(const QModelIndex&)
{
    subwindow_t sw = winlist.value(activeWid);
    sw.widget->setFocus();
}

void IdealIRC::Highlight(int wid, int type)
{
    if (! winlist.contains(wid))
        return;
    if (wid == activeWid)
        return;

    subwindow_t wt = winlist.value(wid);

    if (wt.highlight >= type)
        return;

    switch (type) {
        case HL_ACTIVITY:
            wt.treeitem->setForeground(0, QBrush(Qt::darkRed));
            break;

        case HL_HIGHLIGHT:
            wt.treeitem->setForeground(0, QBrush(Qt::blue));
            break;

        case HL_MSG:
            wt.treeitem->setForeground(0, QBrush(Qt::red));
            break;
    }

    wt.highlight = type;
    winlist.insert(wid, wt);

}

void IdealIRC::on_actionAbout_IdealIRC_triggered()
{
    IAbout *a = new IAbout(this);
    a->show();
}

void IdealIRC::versionReceived()
{
    if (vc.getInternVersion() != VERSION_INTEGER) {
        QString msg = QString("A new version is released (%1). It is recommended you upgrade!\r\nDownload via the website http://www.idealirc.org/")
                        .arg(vc.getVersion());

        QMessageBox::information(this, "New version!", msg);
    }
}
