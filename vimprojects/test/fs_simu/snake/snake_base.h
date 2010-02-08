#ifndef __SNAKE_BASE_H_
#define __SNAKE_BASE_H_
#include <curses.h>
#include <pthread.h>

#include "log.h"
#include "base.h"


/**
 * 定义蛇的节点。
 * 实质上是双链表的节点。
 */
typedef struct _snake_node
{
	int 	x,y; 			//蛇节点在窗口中的位置。
	int 	color; 			//节点的颜色。
	dct_t 	direction; 		//运动方向。提示后面的节点的运动方向

	struct _snake_node *pre; 		//前一个节点
	struct _snake_node *next;   	//后一个节点

}snake_node;


/**
 * 定义一个双链表。表示蛇
 */
typedef struct
{
	int 	len; 		//蛇的长度
	dct_t 	direction; 	//蛇的运动方向

	snake_node *head, *tail; 	//蛇头和尾
	int max_x, max_y; 		//最大活动区域
}snake;


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

	/**
	 * 食物的最大数量和位置
	 */
	int food_max_num; 			//产生食物的最大数量
	int **food_map; 			//食物所在位置表格
	int map_x, map_y; 			//food_map的大小
	pthread_mutex_t food_map_lock; 	

	snake *s; 					//蛇
	pthread_mutex_t snake_lock; 	

	int shutdown; 				//标记是否退出

	pthread_mutex_t s_d_lock; 	//这个结构体的锁

	//保存三个进程的id
	pthread_t food_creator_id; 	//食物产生器
	pthread_t snake_runner_id; 	//
	pthread_t key_listenner_id; //
}snake_data;


/**
 * 蛇运动线程参数
 */
typedef struct thread_run_snake_arg
{
	snake_data *s_d; 	//全局数据

	pthread_t id;
	int retval; 	//线程的返回参数
}thread_run_snake_arg_t;

/**
 * 键盘事件监听线程的参数表
 */
typedef struct thread_key_arg
{
	snake_data *s_d; 	//全局数据

	int retval; 	//返回值
	pthread_t id; 	//线程id
}thread_key_arg_t;


/**
 * 食物创建线程的参数集合
 */
typedef struct thread_create_food
{
	snake_data *s_d; 	//全局数据

	int retval;
	pthread_t id;

}thread_food_arg_t;



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
