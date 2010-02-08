#include "snake_base.h"
#include <string.h>
#include <stdlib.h>

#include "keyboard_ctrl.h"

/**
 * 初始化颜色对。
 * 返回初始化得颜色对得数量。
 */
static int init_used_colors()
{
	init_pair(1, COLOR_GREEN, COLOR_BLACK);
	init_pair(2, COLOR_BLUE, COLOR_BLACK);
	init_pair(3, COLOR_RED, COLOR_BLACK);
	return 3;
}

/**
 * 根据参数创建窗口
 */
static void create_wins(snake_data *s)
{
	char title[] = "Snake --- A little funny game.";
	wattron(stdscr, COLOR_PAIR(RED_ON_BLACK));
	mvaddstr(1, s -> max_col /2 - strlen(title)/2, title);
	wattroff(stdscr, COLOR_PAIR(RED_ON_BLACK));
	wrefresh(stdscr);

	win_attrs *a;

	a = & s -> play_win_attrs;

	log_info("Create window: %s, width: %d, height: %d, start at (%d, %d) border color: %d"
			, "play_win", a -> width, a -> height, a -> startx, a -> starty, a -> border_color);

	s -> play_win = newwin(a -> height, a -> width, a -> starty, a -> startx);
	wattron(s -> play_win, COLOR_PAIR(a -> border_color));
	box(s -> play_win, ACS_VLINE , ACS_HLINE);
	wattroff(s -> play_win, COLOR_PAIR(a -> border_color));
	wrefresh(s -> play_win);

	a =& s -> info_win_attrs;

	log_info("Create window: %s, width: %d, height: %d, start at (%d, %d) border color: %d"
			, "info_win", a -> width, a -> height, a -> startx, a -> starty, a -> border_color);

	s -> info_win = newwin(a -> height, a -> width, a -> starty, a -> startx);
	wattron(s -> info_win, COLOR_PAIR(a -> border_color));
	box(s -> info_win, ACS_VLINE , ACS_HLINE);
	wattroff(s -> info_win, COLOR_PAIR(a -> border_color));
	wrefresh(s -> info_win);

	log_info("Create windows done.");
	return;
}

/**
 * 创建屏幕区域的边框
 */
static int create_border()
{
	int line, col;
	getmaxyx(stdscr, line, col);
	box(stdscr, ACS_VLINE, ACS_HLINE);

	return 1;
}

/**
 * 初始化屏幕。
 */
static int screen_init(snake_data *s)
{
	initscr();
	noecho(); 				//关闭getch()的键盘回显
	keypad(stdscr, TRUE); 	//开启功能键相应模式
	curs_set(0); 			//不显示光标
	cbreak(); 				//关闭行缓冲。便于捕获键盘按键事件

	if (has_colors() == FALSE)
	{
		log_error("ERROR: No color supported. %s %d", __FILE__, __LINE__);		
	}
	else
	{
		start_color();
		init_used_colors();
	}
	
	//获取屏幕的大小
	getmaxyx(stdscr, s -> max_line, s -> max_col);
	//屏幕的边框
	create_border();
	
	/*
	 ***********************************************
	 * 这个函数调用回重绘标准窗口stdscr上的所有内容。
	 * 同时也会磨掉子窗口的内容！！！
	 ***********************************************
	 */
	refresh();

	s -> play_win_attrs.width = (int)((s -> max_col - 2) * s -> split_r);
	s -> play_win_attrs.height = s -> max_line - 3;
	s -> play_win_attrs.startx = 1;
	s -> play_win_attrs.starty = 2;
	s -> play_win_attrs.border_color = BLUE_ON_BLACK;

	s -> info_win_attrs.width = (int)((s -> max_col - 2) * (1 - s -> split_r));
	s -> info_win_attrs.height = s -> max_line - 3;
	s -> info_win_attrs.startx = 1 + (int)((s -> max_col - 2) * s -> split_r);
	s -> info_win_attrs.starty = 2;
	s -> info_win_attrs.border_color = GREEN_ON_BLACK;
	log_info("Create windows. %s %d", __FILE__, __LINE__);
	//绘制子窗口
	create_wins(s);
	
	int y,x;
	getmaxyx(s -> play_win, y, x);
	log_info("play_win size (%d, %d)", x, y);
	log_info("Set the snake scope.");
	snake_set_scope(s -> s, y, x);
	log_info("Show the snake on the window.");
	snake_set_pos_dct(s -> s, y / 2 + 2, x / 2 - s -> s -> len /2, RIGHT_DCT);	

	/**
	 * 启动键盘事件监听线程
	 */
	thread_key_arg_t *key_args = (thread_key_arg_t*)malloc(sizeof(*key_args));
	key_args -> s = s -> s;
	key_args -> win = s -> play_win;
	key_args -> retval = THD_RETVAL_UNKNOWN;
	key_args -> id = listening_keyevent(key_args);
	pthread_detach(key_args -> id); 	//分离线程

	/**
	 * 启动蛇移动线程
	 */
	thread_arg_t *snake_thread_args = (thread_arg_t*)malloc(sizeof(thread_arg_t));
	snake_thread_args -> s = s -> s;
	snake_thread_args -> win = s -> play_win;
	snake_thread_args -> retval = THD_RETVAL_UNKNOWN;
	snake_thread_args -> id = snake_run(snake_thread_args);
	pthread_join(snake_thread_args -> id, NULL);

	//关闭键盘事件监听线程
	pthread_cancel(key_args -> id);
	
	free(key_args);
	free(snake_thread_args);

	//game over
	if (snake_thread_args -> retval == THD_RETVAL_SNAKE_CRAFT)
	{
		mvwaddstr(s -> play_win , 10, 20, "Game Over!");
		mvwaddstr(s -> play_win , 11, 20, "Press any key to QUIT!");
		wrefresh(s -> play_win);
	}

	//snake_show(s -> s, s -> play_win);
	return 1;
}

/**
 * 关闭并清理屏幕
 */
static int screen_close(snake_data *s)
{
	delwin(s -> play_win);
	delwin(s -> info_win);
	endwin();
	return 1;
}

snake_data* snake_game_init()
{
	snake_data *s_d = NULL;
	//分配全局数据结构
	s_d = (snake_data*)malloc(sizeof(*s_d));
	if (NULL == s_d)
	{
		return NULL;
	}
	
	s_d -> split_r = 0.8f;

	s_d -> s = snake_init();

	screen_init(s_d);	
	return s_d;
}

int snake_game_close(snake_data *s)
{
	screen_close(s);
	return 1;
}
