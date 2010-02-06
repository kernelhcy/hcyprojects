#ifndef __SNAKE_SNAKE_H_
#define __SNAKE_SNAKE_H_
#include <curses.h>

#define SNAKE_INIT_LEN  3 //蛇的初始长度
#define SNAKE_NODE 		ACS_DIAMOND
/**
 * 定义运动的方向
 */
typedef enum _direction
{
	UP_DCT, 	//向上运动
	DOWN_DCT, 	//向下运动
	LEFT_DCT, 	//向左运动
	RIGHT_DCT, 	//向右运动
	UNKNOWN_DCT //未知方向
}dct_t;

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
 * make the snake run!
 */
void snake_run(snake *);

/**
 * 初始化和销毁蛇
 */
snake * snake_init();
void snake_free(snake *);
/**
 * 设置蛇的位置和方向
 */
void snake_set_pos_dct(snake *, int y, int x, dct_t dct);

/**
 * 在屏幕上显示和擦除蛇
 */
void snake_show(snake *, WINDOW *);
void snake_clear(snake *, WINDOW *);

/**
 * 设置蛇的最大活动区域
 */
void snake_set_scope(snake*, int y, int x);

#endif
