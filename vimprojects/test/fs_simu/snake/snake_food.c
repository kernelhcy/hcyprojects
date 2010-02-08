#include "snake_food.h"

static void * thread_create_food(void *a)
{

	return NULL;

}

pthread_t begin_create_food(snake_data *s_d)
{
	pthread_t id;
	if (NULL == s_d)
	{
		log_error("NULL pointer. %s %d", __FILE__, __LINE__);
		return 0;
	}

	return id;
}
