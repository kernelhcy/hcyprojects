#ifndef __SNAKE_SNAKE_H_
#define __SNAKE_SNAKE_H_
#include <curses.h>
#include <pthread.h>

#define SNAKE_INIT_LEN  15 //蛇的初始长度
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


//线程的返回值
enum
{
	THD_RETVAL_NORMAL,
	THD_RETVAL_ERROR,
	THD_RETVAL_SNAKE_CRAFT,
	THD_RETVAL_SNAKE_NEXT,
	THD_RETVAL_KEY_UP,
	THD_RETVAL_KEY_DWON,
	THD_RETVAL_KEY_LEFT,
	THE_RETVAL_KEY_RIGHT,
	THD_RETVAL_UNKNOWN
};

//线程的启动参数
typedef struct thread_arg
{
	snake *s;
	WINDOW *win;
	pthread_t id;
	int retval; 	//线程的返回参数
}thread_arg_t;
/**
 * make the snake run!
 * 蛇的移动由一个单独的线程来控制。这个函数启动这个线程。
 *
 * 返回线程的id。如果启动线程失败，则返回NULL。
 */
pthread_t snake_run(thread_arg_t *);

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

/**
 * 根据当前的位置和方向，移动蛇
 */
void snake_move(snake*);

/**
 * 设置蛇的移动方向
 */

void snake_set_dct(snake *, dct_t);

#endif
