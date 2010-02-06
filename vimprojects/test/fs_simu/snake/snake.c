#include "snake.h"
#include "snake_base.h"
#include <stdio.h>
#include "log.h"

void snake_start()
{
	log_open("log.snake");
	
	log_info("run snake.\n");
	snake_data *s_d = NULL;
	s_d = snake_game_init();
	getch();
	snake_game_close(s_d);

	log_close();
}
