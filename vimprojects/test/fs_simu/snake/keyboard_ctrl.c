#include "keyboard_ctrl.h"
#include "snake_snake.h"

static void * thread_key_listen(void *a)
{
	thread_key_arg_t *arg = (thread_key_arg_t*)a;
	log_info("Listening keyevent...");
	int key = getch();
	while( key != 'q' && key != 'Q')
	{
		switch (key)
		{
			case KEY_UP:
				log_info("Click UP key.");
				snake_set_dct(arg -> s, UP_DCT);
				break;
			case KEY_DOWN:
				log_info("Click DOWN key.");
				snake_set_dct(arg -> s, DOWN_DCT);
				break;
			case KEY_LEFT:
				log_info("Click LEFT key.");
				snake_set_dct(arg -> s, LEFT_DCT);
				break;
			case KEY_RIGHT:
				log_info("Click RIGHT key.");
				snake_set_dct(arg -> s, RIGHT_DCT);
				break;
			default:
				break;
		}
		key = getch();
	}
}

pthread_t listening_keyevent(thread_key_arg_t *arg)
{
	if (NULL == arg)
	{
		log_error("NULL pointer. %s %d", __FILE__, __LINE__);
		return 0;
	}
	pthread_t id;
	
	int error;
	log_info("Start keyevent listening thread.");
	if (error = pthread_create(&id, NULL, (void *(*)(void *))thread_key_listen, (void *)arg))
	{
		log_error("Create the keyevent listen thread ERROR. %s %d", __FILE__, __LINE__);
		return 0;
	}

	return id;
}
