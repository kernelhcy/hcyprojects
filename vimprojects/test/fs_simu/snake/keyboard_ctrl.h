#ifndef __KEY_BOARD_CTRL_H_
#define __KEY_BOARD_CTRL_H_
#include "snake_snake.h"
#include <pthread.h>

typedef struct thread_key_arg
{
	snake *s; 		//蛇
	WINDOW *win; 	//屏幕
	int retval; 	//返回值
 
	pthread_t id; 	//线程id
}thread_key_arg_t;

/**
 * 启动键盘事件监听线程。
 */
pthread_t listening_keyevent(thread_key_arg_t *);

#endif
