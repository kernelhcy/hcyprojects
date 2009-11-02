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
    startPlayer();

    this->setWindowIcon(QIcon(":images/icon.png"));
    this->resize(800, 600);
    this->setFixedSize(this->size());
    center();
    //show();
    setTrayIcon();
}

void MainWindow::startPlayer()
{
    player = new QWebView(this);
    player->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    player->load(QUrl("http://www.1g1g.com/?version=desktop_linux"));
    //player -> load(QUrl("http://commander.1g1g.com/test.html"));
    connect(player, SIGNAL(titleChanged(QString)), this, SLOT(changeTitle(QString)));
    connect(player, SIGNAL(titleChanged(QString)), this, SLOT(changeTrayIconTooltip(QString)));
    connect(player, SIGNAL(statusBarMessage ( const QString &)), this, SLOT(playerStatusBarMsgChange(const QString)));
    this->setCentralWidget(player);

    QTextStream out(stdout);
    out << player->page()->mainFrame() ->toPlainText();

}

void MainWindow::playerStatusBarMsgChange(const QString msg)
{
    QTextStream out(stdout);
    out << msg << "\n";
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

    minimizeAction = new QAction(QIcon(":images/mini.png"),tr("最小化"), this);
    connect(minimizeAction, SIGNAL(triggered()), this, SLOT(hideWindow()));
    showAction = new QAction(QIcon(":images/nor.png"),tr("显示窗口"), this);
    connect(showAction, SIGNAL(triggered()), this, SLOT(showNormal()));
    settingAction = new QAction(QIcon(":images/setting.png"),tr("设置代理"), this);
    connect(settingAction, SIGNAL(triggered()), this, SLOT(setting()));
    quitAction = new QAction(QIcon(":images/exit.png"),tr("退出"), this);
    connect(quitAction, SIGNAL(triggered()), this, SLOT(exitProgram()));

    traymenu=new QMenu();

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

MainWindow::~MainWindow()
{
    ct->terminate();
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


