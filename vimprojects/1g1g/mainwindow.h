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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
#include <QtWebKit>
#include <QSystemTrayIcon>
#include <QIcon>
#include <QAction>
#include <QTextStream>
#include <QActionGroup>
#include "controlthread.h"
#include "settingdialog.h"
#include "env.h"
#include "tipwindow.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0, ControlThread* ct = 0);
    ~MainWindow();
    void startPlayer();
    void reStartPlayer();

public slots:
    void processCommand(QString);
    void changeTitle(QString);
    //设置显示模式
    void setViewMode(ENV1G::viewMode mode);
    void setNormalViewMode();
    void setSimpleViewMode();
    void setListenViewMode();
protected slots:
    void exitProgram();
    void hideWindow();
    void changeTrayIconTooltip(QString);
    void setting();
    void trayActived(QSystemTrayIcon::ActivationReason);
    void playerStatusBarMsgChange(const QString);

protected:
    void closeEvent(QCloseEvent *event);
    void moveEvent(QMoveEvent *event);
    void leaveEvent(QEvent *event);
    void enterEvent(QEvent *event);
private:
    QWebView* player;   
    ControlThread* ct;
    QSystemTrayIcon *trayIcon;
    QMenu *traymenu;
    QAction *minimizeAction;
    QAction *showAction;
    QAction *quitAction;
    //显示模式按钮
    QAction *settingAction;
    QAction *vnormalAction;
    QAction *vsimpleAction;
    QAction *vlistenAction;
    QActionGroup *actiongroup;

    void center();
    void setProxy();
    void setTrayIcon();

    //用于提示歌曲切换。无边框
    TipWindow *tipwin;

};

#endif // MAINWINDOW_H
