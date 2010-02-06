#ifndef __SNAKE_BASE_H_
#define __SNAKE_BASE_H_
#include <curses.h>
#include "log.h"
#include "snake_snake.h"

#define GREEN_ON_BLACK	1
#define BLUE_ON_BLACK 	2
#define RED_ON_BLACK 	3

/**
 * 窗口属性
 */
typedef struct
{
	int width, height; 	//宽度和高度
	int startx, starty; //开始位置坐标
	int border_color; 	//边框的颜色。保存颜色对的索引。
	char title[100]; 	//标题
}win_attrs;

/**
 * 主数据结构
 */
typedef struct
{
	WINDOW* play_win; 			//游戏窗口。
	WINDOW* info_win; 			//游戏信息窗口。
	win_attrs play_win_attrs; 	//游戏窗口属性
	win_attrs info_win_attrs; 	//游戏信息窗口属性。
	float split_r; 				//两个窗口的比例。用于根据当前窗口的大小计算两个子窗口的大小。

	int max_line, max_col; 		//屏幕的大小

	snake *s; 					//蛇
}snake_data;

/**
 * 初始化游戏
 * 初始化数据结构，绘制屏幕。
 */
snake_data* snake_game_init();

/**
 * 关闭游戏
 */
int snake_game_close(snake_data *);


#endif
