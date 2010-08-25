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
#include <QtGui>
#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent, ControlThread* ct)
    : YGMainWindow(parent)
{
    //setMouseTracking(true);
    this->ct = ct;
    setProxy();
    setTrayIcon();
    startPlayer();
    hiden_pos = HIDEN_UNSET;

    this -> setWindowIcon(QIcon(":images/icon.png"));
    this -> resize(800, 620);
    this -> setFixedSize(this->size());

    Config *config = Config::getInstance();
    this -> setViewMode(config->mode);
    this -> move(config -> pos_x, config -> pos_y);

    //获取根窗口
    QDesktopWidget *desktop = QApplication::desktop();
    //获取屏幕可用区域的大小。
    avaGeometry = desktop -> availableGeometry(this);
    //获取标题栏高度
    //虽然标题栏是自定义的，但是在将窗口自动靠边到底部时，系统还是按照有默认标题栏
    //进行处理，导致不能移到底部。
    titleBarHeight = style()->pixelMetric(QStyle::PM_TitleBarHeight);
    //获取窗口边框的宽度
    frameWidth = style() -> pixelMetric(QStyle::PM_DefaultFrameWidth);
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

    ct->terminate();
}

void MainWindow::startPlayer()
{
    player = new QWebView(this);
    player->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    player->load(QUrl("http://www.1g1g.com/?version=desktop_linux"));
    //player -> load(QUrl("http://commander.1g1g.com/test.html"));
    connect(player, SIGNAL(titleChanged(QString)), this, SLOT(changingTitle(QString)));
    connect(player, SIGNAL(titleChanged(QString)), this, SLOT(changeTrayIconTooltip(QString)));
    connect(player, SIGNAL(statusBarMessage(const QString&)),
            this, SLOT(playerStatusBarMsgChange(const QString)));
    player ->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
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
        this->setFixedSize(QSize(800, 620));
        config->mode = ENV1G::V_NORMAL;
        break;
    case ENV1G::V_SIMPLE:
        //out<< tr("精简模式\n");
        this->setFixedSize(QSize(320, 470));
        config->mode = ENV1G::V_SIMPLE;
        //player->resize(this->centralWidget()->size());
        //this->centralWidget()->resize(this->size());
        break;
    case ENV1G::V_LISTEN:
        //out<< tr("听歌模式\n");
        this->setFixedSize(QSize(300, 80));
        config->mode = ENV1G::V_LISTEN;
        break;
    default:
        this->setFixedSize(QSize(800, 620));
        config->mode = ENV1G::V_NORMAL;
        break;
    }

    player->reload();
    //防止鼠标第一次进入窗口时，窗口变化未知。
    oldSize = size();

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
    trayIcon->showMessage(tr("播放器状态："),msg,QSystemTrayIcon::Information,2000);
}

void MainWindow::showLrc(bool checked)
{
    if (checked)
    {

    }
    processCommand("openLyricWindow", "");

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

    showLrcAction = new QAction(tr("显示歌词"), this);
    showLrcAction -> setCheckable(true);
    connect(showLrcAction, SIGNAL(toggled(bool)), this, SLOT(showLrc(bool)));

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
    traymenu->addAction(showLrcAction);

    traymenu->addSeparator();
    //traymenu->addAction(showAction);
    traymenu->addAction(settingAction);
    //traymenu->addAction(minimizeAction);
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

void MainWindow::processCommand(QString command, QString arg)
{
    QString jscode;
    if (arg.isEmpty())
    {
        jscode = "get1g1gPlayer().sendNotification('" + command + "')";
    }
    else
    {
        jscode = "get1g1gPlayer().sendNotification('"
                 + command + "," + arg +"')";
    }

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
    Config *config = Config::getInstance();

    QDesktopWidget *desktop = QApplication::desktop();

    width = this->frameGeometry().width();
    height = this->frameGeometry().height();

    screenWidth = desktop->width();
    screenHeight = desktop->height();

    x = (screenWidth - width) / 2;
    y = (screenHeight - height) / 2;

    this->move( x, y );
    config -> pos_x = x;
    config -> pos_y = y;

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


    QPoint pos = event -> pos();

    /*
    QPoint toPos = pos;

    if (pos.x() < 50 && pos.x() > frameWidth + 1)
    {
        toPos.setX(avaGeometry.left());
    }

    if (pos.y() < 50 && pos.y() > avaGeometry.top() + 1)
    {
        toPos.setY(avaGeometry.top() + 1);
    }

    if (pos.x() + this->size().width() > avaGeometry.width() - 50)
    {
        toPos.setX(avaGeometry.width() - this -> size().width());
    }

    if (pos.y() + this->size().height() > avaGeometry.height() - 50 + titleBarHeight)
    {
        toPos.setY(avaGeometry.height() - this -> size().height() + titleBarHeight - 1);
    }

    if ( pos != toPos)
    {
        this->move(toPos);
    }

    Config *config = Config::getInstance();
    config -> pos_x = pos.x();
    config -> pos_y = pos.y();
    */
}

void MainWindow::leaveEvent(QEvent *event)
{
    QDesktopWidget *desktop = QApplication::desktop();
    int shrink = 0;

    if (this -> pos().x() < 5)
    {
        hiden_pos = HIDEN_LEFT;
        shrink = 1;
    }
    else if (this -> pos().x() + this -> size().width()
        > desktop -> width() - 5)
    {
        hiden_pos = HIDEN_RIGHT;
        shrink = 1;
    }
    else if (this -> pos().y() < 5)
    {
        hiden_pos = HIDEN_UP;
        shrink = 1;
    }
    else
    {
        shrink = 0;
    }

    if (shrink)
    {
        shrinkWindow();
    }
    event->accept();
}

void MainWindow::enterEvent(QEvent *event)
{
    setFixedSize(oldSize);
    //moveSuitable();
    event->accept();
}
void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    /*
    QRect area = geometry();
    printf("mouse move to (%d, %d)\n", event ->x() , event ->x());
    if (area.contains(event -> globalX(), event -> globalY()))
    {
        setFixedSize(oldSize);
        moveSuitable();
    }
    */
}
void MainWindow::shrinkWindow()
{
    oldSize = size();
    QSize newSize = oldSize;

    if (hiden_pos == HIDEN_UP)
    {
        newSize.setHeight(5);
    }
    else
    {
        newSize.setWidth(5);
    }
    QRect area = this -> geometry();
    QPoint pos = QCursor::pos(); //获取鼠标的当前位置。
    //判断是否真的出了窗口
    if (!area.contains(pos.x(), pos.y()))
    {
        setFixedSize(newSize);
        moveSuitable();
    }
    moveSuitable();

}


void MainWindow::moveSuitable()
{
    QDesktopWidget *desktop = QApplication::desktop();
    if (hiden_pos == HIDEN_LEFT)
    {
        this -> move(0, this -> pos().y());
    }
    else if(hiden_pos == HIDEN_RIGHT)
    {
        this -> move(desktop -> width() - this -> size().width(), this -> pos().y());
    }
    else if (hiden_pos == HIDEN_UP)
    {
        this -> move(this -> pos().x(), 0);
    }
    return;
}
