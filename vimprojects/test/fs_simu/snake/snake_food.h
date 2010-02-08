#ifndef __SNAKE__FOOD_H_
#define __SNAKE__FOOD_H_
#include <pthread.h>
#include "snake_base.h"

/**
 * 启动食物产生线程
 */
pthread_t begin_create_food(thread_food_arg_t *);


#endif
