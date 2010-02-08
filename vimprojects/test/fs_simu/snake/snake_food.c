#include "snake_food.h"

/**
 * 在小于(x,y)的范围内产生一个food。
 * 保证这个位置没有food。
 */
static void create_food(int **map, int y, int x)
{
	if (NULL == map)
	{
		log_error("NULL pointer. %s %d", __FILE__, __LINE__);
		return ;
	}

	if (y < 0 || x < 0)
	{
		log_error("Create_food arg invalid. %s %d", __FILE__, __LINE__);
		return;
	}

	int pos_x, pos_y;
	pos_x = rand() % x - 1;
	pos_y = rand() % y - 1;

	if (pos_x <= 0)
	{
		pos_x = 1;
	}
	if (pos_y <= 0)
	{
		pos_y = 1;
	}

	while (map[pos_y][pos_x])
	{
		pos_x = rand() % x;
		pos_y = rand() % y;	
	}

	map[pos_y][pos_x] = 1;
	log_info("Create food. pos (%d, %d)", pos_x, pos_y);
	return;
}


#include "snake_snake.h"
static void show_food(thread_food_arg_t *args)
{
	int i, j;
	log_info("Show food.");	
	wattron(args -> s_d -> play_win, COLOR_PAIR(GREEN_ON_BLACK));
	//显示
	for (i = 1; i < args -> s_d -> map_y; ++i)
	{
		for (j = 1; j < args -> s_d -> map_x; ++j)
		{
			mvwaddch(args -> s_d -> play_win, i, j, args -> s_d -> food_map[i][j] == 1 ? FOOD_SHARP : ' ');
		}
	}
	wattroff(args -> s_d -> play_win, COLOR_PAIR(RED_ON_BLACK));

	//绘制蛇，防止蛇闪烁
	snake_show(args -> s_d -> s, args -> s_d -> play_win);

	wrefresh(args -> s_d -> play_win);
	log_info("Shown food over.");
	return;
}

static void * thread_create_food(void *a)
{
	if (NULL == a)
	{
		log_error("NULL pointer. %s %d", __FILE__, __LINE__);
		return NULL;
	}
	thread_food_arg_t *arg = (thread_food_arg_t*)a;
	int i = 0;
	int time_marg;
	log_info("Beginning creating food.");
	while(i < arg -> s_d -> food_max_num)
	{
		create_food(arg -> s_d -> food_map, arg -> s_d -> map_y, arg -> s_d  -> map_x);
		++i;
		show_food(arg);
	
		time_marg = rand() % 5;
		if (time_marg <= 0)
		{
			time_marg = 1;
		}
		sleep(time_marg);
	}

	return NULL;

}

pthread_t begin_create_food(thread_food_arg_t *args)
{

	pthread_t id;
	if (NULL == args)
	{
		log_error("NULL pointer. %s %d", __FILE__, __LINE__);
		return 0;
	}

	int error; 
	log_info("Start food creator.");
	if (error = pthread_create(&id, NULL, thread_create_food, args))
	{
		log_error("Create food creator error. %s %d", __FILE__, __LINE__);
		return 0;
	}

	return id;
}
