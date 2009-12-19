/***************************************************************************
 *   Copyright (C) 2009 by silwings,ericyosho   *
 *   silwings@gmail.com, ericyosho@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent, ControlThread* ct)
    : QMainWindow(parent)
{
    this->ct = ct;
    setProxy();
    setTrayIcon();
    startPlayer();

    this->setWindowIcon(QIcon(":images/icon.png"));
    this->resize(800, 600);
    this->setFixedSize(this->size());
    this->setFixedSize(QSize(800, 600));
    Config *config = Config::getInstance();
    this->setViewMode(config->mode);

    center();

    tipwin = new TipWindow(0);
    tipwin -> show();
}

MainWindow::~MainWindow()
{
    Config *config = Config::getInstance();
    delete config;
    delete player;
    delete trayIcon;
    delete minimizeAction;
    delete showAction;
    delete quitAction;
    delete settingAction;
    delete vnormalAction;
    delete vsimpleAction;
    delete vlistenAction;
    delete actiongroup;
    delete tipwin;
    ct->terminate();
}

void MainWindow::startPlayer()
{
    player = new QWebView(this);
    player->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    player->load(QUrl("http://www.1g1g.com/"));
    //player -> load(QUrl("http://commander.1g1g.com/test.html"));
    connect(player, SIGNAL(titleChanged(QString)), this, SLOT(changeTitle(QString)));
    connect(player, SIGNAL(titleChanged(QString)), this, SLOT(changeTrayIconTooltip(QString)));
    connect(player, SIGNAL(statusBarMessage ( const QString &)), this, SLOT(playerStatusBarMsgChange(const QString)));
    this->setCentralWidget(player);

}

void MainWindow::setViewMode(ENV1G::viewMode mode)
{
    //QTextStream out(stdout);
    Config *config = Config::getInstance();

    switch(mode)
    {
    case ENV1G::V_NORMAL:
        //out<< tr("正常模式\n");
        this->setFixedSize(QSize(800, 600));
        player->reload();
        config->mode = ENV1G::V_NORMAL;
        break;
    case ENV1G::V_SIMPLE:
        //out<< tr("精简模式\n");
        this->setFixedSize(QSize(320, 450));
        player->reload();
        config->mode = ENV1G::V_SIMPLE;
        //player->resize(this->centralWidget()->size());
        //this->centralWidget()->resize(this->size());
        break;
    case ENV1G::V_LISTEN:
        //out<< tr("听歌模式\n");
        this->setFixedSize(QSize(300, 60));
        player->reload();
        config->mode = ENV1G::V_LISTEN;
        break;
    default:
        this->setFixedSize(QSize(800, 600));
        player->reload();
        config->mode = ENV1G::V_NORMAL;
        break;
    }
}

void MainWindow::setNormalViewMode()
{
    this->setViewMode(ENV1G::V_NORMAL);
}
void MainWindow::setSimpleViewMode()
{
    this->setViewMode(ENV1G::V_SIMPLE);
}
void MainWindow::setListenViewMode()
{
    this->setViewMode(ENV1G::V_LISTEN);
}

void MainWindow::playerStatusBarMsgChange(const QString msg)
{

}

void MainWindow::reStartPlayer()
{
    setProxy();
    player -> reload();
}

void MainWindow::setProxy()
{
    Config *config = Config::getInstance();

    if (config->useProxy)
    {
        //printf("设置代理\n");
        QNetworkProxy proxy(QNetworkProxy::HttpProxy, config->proxyHost, config->proxyPort);
        QNetworkProxy::setApplicationProxy(proxy);
    }
    else
    {
        QNetworkProxy proxy;
        proxy.setType(QNetworkProxy::NoProxy);
        QNetworkProxy::setApplicationProxy(proxy);
    }
}


void MainWindow::setTrayIcon()
{
    trayIcon=new QSystemTrayIcon();
    trayIcon->setIcon(QIcon(":images/icon.png"));
    trayIcon->show();
    trayIcon->setToolTip(QString(tr("亦歌")));

    vnormalAction = new QAction(tr("正常模式"), this);
    connect(vnormalAction, SIGNAL(triggered()), this, SLOT(setNormalViewMode()));
    vnormalAction ->setCheckable(true);
    vsimpleAction = new QAction(tr("精简模式"), this);
    connect(vsimpleAction, SIGNAL(triggered()), this, SLOT(setSimpleViewMode()));
    vsimpleAction ->setCheckable(true);
    vlistenAction = new QAction(tr("听歌模式"), this);
    connect(vlistenAction, SIGNAL(triggered()), this, SLOT(setListenViewMode()));
    vlistenAction ->setCheckable(true);

    minimizeAction = new QAction(QIcon(":images/mini.png"),tr("最小化"), this);
    connect(minimizeAction, SIGNAL(triggered()), this, SLOT(hideWindow()));
    showAction = new QAction(QIcon(":images/nor.png"),tr("显示窗口"), this);
    connect(showAction, SIGNAL(triggered()), this, SLOT(showNormal()));
    settingAction = new QAction(QIcon(":images/setting.png"),tr("设置代理"), this);
    connect(settingAction, SIGNAL(triggered()), this, SLOT(setting()));
    quitAction = new QAction(QIcon(":images/exit.png"),tr("退出"), this);
    connect(quitAction, SIGNAL(triggered()), this, SLOT(exitProgram()));

    traymenu=new QMenu();

    actiongroup = new QActionGroup(this);
    actiongroup->addAction(vnormalAction);
    actiongroup->addAction(vsimpleAction);
    actiongroup->addAction(vlistenAction);

    Config *config = Config::getInstance();
    switch(config -> mode)
    {
    case ENV1G::V_NORMAL:
        vnormalAction -> setChecked(true);
        break;
    case ENV1G::V_SIMPLE:
        vsimpleAction -> setChecked(true);
        break;
    case ENV1G::V_LISTEN:
        vlistenAction  -> setChecked(true);
        break;
    default:
        vnormalAction -> setChecked(true);
        break;
    }

    traymenu->addAction(vnormalAction);
    traymenu->addAction(vsimpleAction);
    traymenu->addAction(vlistenAction);

    traymenu->addSeparator();
    traymenu->addAction(showAction);
    traymenu->addAction(settingAction);
    traymenu->addAction(minimizeAction);
    traymenu->addSeparator();
    traymenu->addAction(quitAction);

    trayIcon->setContextMenu(traymenu);
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this,
                    SLOT(trayActived(QSystemTrayIcon::ActivationReason)));
}

void MainWindow::setting()
{
    new SettingDialog(this);
}

void MainWindow::processCommand(QString command){   
    QString jscode = "get1g1gPlayer().sendNotification('"+command+"')";
    //QString jscode = "get1gCommander().command('play','An Angel-Declan Galbraith');";

    player->page()->mainFrame()->evaluateJavaScript(jscode);

}

void MainWindow::changeTitle(QString title){
    this->setWindowTitle(title);
}

void MainWindow::center()
{
    int x, y;
    int screenWidth;
    int screenHeight;
    int width, height;
    QSize windowSize;

    QDesktopWidget *desktop = QApplication::desktop();

    width = this->frameGeometry().width();
    height = this->frameGeometry().height();

    screenWidth = desktop->width();
    screenHeight = desktop->height();

    x = (screenWidth - width) / 2;
    y = (screenHeight - height) / 2;

    this->move( x, y );
}

void MainWindow::exitProgram()
{
    qApp->exit(0);
}

void MainWindow::hideWindow()
{
    trayIcon->showMessage(tr("亦歌客户端"),tr("已经最小化。正在运行中..."),QSystemTrayIcon::Information,2000);
    hide();
}

void MainWindow::changeTrayIconTooltip(QString msg)
{
    trayIcon->setToolTip(msg);
    trayIcon->showMessage(tr("正在播放："),msg,QSystemTrayIcon::Information,2000);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    event->ignore();
    hideWindow();
    tipwin->pollOut();
}

void MainWindow::trayActived(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Context)
    {
        return ;
    }
    if (!isVisible())
    {
        showNormal();
    }
    else
    {
        hideWindow();
    }
}

void MainWindow::moveEvent(QMoveEvent *event)
{
    QPoint pos = event->pos();
    QPoint toPos = pos;

    //获取桌面大小
    QDesktopWidget *desktop = QApplication::desktop();
    int screenWidth = desktop->width();
    int screenHeight = desktop->height();

    if (pos.x() < 0)
    {
        toPos.setX(0);
    }

    if (pos.y() < 100 && pos.y() > 30)
    {
        toPos.setY(0);
    }


    if (pos.x() + this->size().width() > screenWidth - 50)
    {
        toPos.setX(screenWidth - this->size().width() -5);
    }

    if (pos.y() + this->size().height() > screenHeight - 50)
    {
        toPos.setY(screenHeight - this->size().height() - 5);
    }

    if ( pos != toPos)
    {
        this->move(toPos);
    }

}

void MainWindow::leaveEvent(QEvent *event)
{
    if (this->pos().y() < 30)
    {
        //this->hideWindow();
    }
    event->accept();
}

void MainWindow::enterEvent(QEvent *event)
{
//    printf("鼠标进入\n");
//    if (this->pos().y() < 30)
//    {
//        QSize size = this->size();
//        Config *config = Config::getInstance();
//
//        switch(config -> mode)
//        {
//        case ENV1G::V_NORMAL:
//            size.setHeight(600);
//            break;
//        case ENV1G::V_SIMPLE:
//            size.setHeight(450);
//            break;
//        case ENV1G::V_LISTEN:
//            size.setHeight(60);
//            break;
//        default:
//            size.setHeight(600);
//            break;
//        }
//
//        this->setFixedSize(size);
//    }
    event->accept();
}
