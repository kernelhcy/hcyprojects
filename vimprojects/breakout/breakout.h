#ifndef BREAKOUT_H
#define BREAKOUT_H

#include <QMainWindow>
#include <QKeyEvent>
#include <QPainter>
#include <QApplication>
#include <QIcon>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include "core.h"
#include "settingdialog.h"
#include "paddle.h"
#include "brick.h"
#include "config.h"
#include "ruledialog.h"
#include "aboutdialog.h"

class Core;
class SettingDialog;
class Paddle;
class Brick;
class Config;

/*
 * 程序主界面
 */
class Breakout : public QMainWindow
{
  	Q_OBJECT

  	public:
    	Breakout(QMainWindow *parent = 0);
    	~Breakout();
    	
		/* 创建游戏 */
		void createGame();
		/* 删除游戏，并不关闭游戏 */
		void deleteGame();
		
		/* 设置随机模式或者固定模式 */
		void randomReflect(bool);
  	protected:	
    	/* 处理键盘事件 */
		void keyPressEvent(QKeyEvent *event);
		/* 处理画图事件 */
    	void paintEvent(QPaintEvent *event);
		/* 处理计时器触发事件 */
		void timerEvent(QTimerEvent *event);
		
		/* 获胜 */
		void victory();
		
		/* 开始、暂停、结束游戏 */
		void startGame();
		void pauseGame();
		void stopGame();
		
		/* 创建菜单 */
		void createMenu();
		
		/* 释放内存 */
		void deleteAll();
  	private:
    	/* 核心规则类 */
    	Core *core;
    	
    	bool gameOver;
    	bool gameWon;
    	bool gameStarted;
    	bool paused;
    	
    	Ball *ball;
    	Paddle *paddle;
		int brick_line, brick_col, brick_cnt; 		/* brick的行数和列数和总数 */
		int gap; 			/* 砖块之间的间隙 */
    	Brick ** bricks; 	/* 盛放brick的数组 */
    	
    	int interval;		/* 计时器触发间隔 */
    	int timerId;		/* 计时器ID */
    	
    	/* 主窗口的长和宽 */
    	int win_width, win_height;
    	
    	/* 设置对话框 */
    	SettingDialog *sd;
    	
    	/* 配置信息 */
    	Config *config;
    public slots:
    	void newGame();
	private slots:
    	void setting();
    	void rule();
    	void aboutBreakout();
};

#endif
