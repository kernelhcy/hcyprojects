#ifndef __SNAKE__FOOD_H_
#define __SNAKE__FOOD_H_
#include <pthread.h>
#include "snake_base.h"


typedef struct thread_create_food
{
	WINDOW *win;
	

	int retval;
	pthread_t id;

}thread_food_arg_t;

/**
 * 启动食物产生线程
 */
pthread_t begin_create_food(snake_data *s_d);


#endif
