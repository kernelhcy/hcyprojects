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

#include "ygmainwindow.h"
#include <QtWebKit>
#include <QSystemTrayIcon>
#include <QIcon>
#include <QAction>
#include <QTextStream>
#include <QActionGroup>
#include "controlthread.h"
#include "settingdialog.h"
#include "env.h"

class MainWindow : public YGMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0, ControlThread* ct = 0);
    ~MainWindow();
    void startPlayer();
    void reStartPlayer();

public slots:
    void processCommand(QString, QString);
    void changeTitle(QString);
    //设置显示模式
    void setViewMode(ENV1G::viewMode);
    void setNormalViewMode();
    void setSimpleViewMode();
    void setListenViewMode();

    void showLrc(bool);
protected slots:
    void exitProgram();
    void hideWindow();
    void changeTrayIconTooltip(QString);
    void setting();
    void trayActived(QSystemTrayIcon::ActivationReason);
    void playerStatusBarMsgChange(const QString);

protected:
    void closeEvent(QCloseEvent *);
    void moveEvent(QMoveEvent *);
    void leaveEvent(QEvent *);
    void enterEvent(QEvent *);
    void mouseMoveEvent(QMouseEvent *);
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

    QAction *showLrcAction;

    //窗口所在的屏幕的可用区域的大小。
    QRect avaGeometry;

    //窗口的一些参数
    int titleBarHeight;     //标题栏的高度
    int frameWidth;         //窗口边框的宽度

    //保存窗口靠边隐藏之前的大小。
    QSize oldSize;

    void center();
    void setProxy();
    void setTrayIcon();

    //缩小窗口。
    void shrinkWindow();
    //将窗口移动到合适的地方。
    //在调用了上面缩小窗口函数，那么窗口要移动到合适的位置。
    //如：屏幕边缘。
    void moveSuitable();

    //标记窗口隐藏的位置。
    enum
    {
        HIDEN_LEFT,
        HIDEN_RIGHT,
        HIDEN_UP,
        HIDEN_UNSET
    }hiden_pos;
};

#endif // MAINWINDOW_H
