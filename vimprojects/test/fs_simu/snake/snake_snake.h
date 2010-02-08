#ifndef __SNAKE_SNAKE_H_
#define __SNAKE_SNAKE_H_
#include <curses.h>
#include <pthread.h>
#include "snake_base.h"
#include "base.h"


/**
 * make the snake run!
 * 蛇的移动由一个单独的线程来控制。这个函数启动这个线程。
 *
 * 返回线程的id。如果启动线程失败，则返回NULL。
 */
pthread_t snake_run(thread_run_snake_arg_t *);

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
