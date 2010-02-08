#ifndef __KEY_BOARD_CTRL_H_
#define __KEY_BOARD_CTRL_H_
#include "snake_snake.h"
#include <pthread.h>

/**
 * 启动键盘事件监听线程。
 */
pthread_t listening_keyevent(thread_key_arg_t *);

#endif
