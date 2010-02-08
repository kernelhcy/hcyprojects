#include "snake_snake.h"
#include "snake_base.h"
#include <string.h>
#include "log.h"


//函数声明
static snake_node* snake_node_init();
static void snake_node_free(snake_node *);
static snake_node* snake_node_init_yx(int y, int x);
static int snake_is_craft(snake*);

//线程主体
void* thread_run_snake(void *a)
{
	log_info("Snake run thread start up.");
	thread_arg_t *arg = (thread_arg_t*)a;

	int i = 0 , j;

	while( 1 )
	{
		log_info("Thread_run_snake : Move the snake.");
		snake_clear(arg -> s, arg -> win);
		snake_move(arg -> s);
		snake_show(arg -> s, arg -> win);
		if (snake_is_craft(arg -> s))
		{
			arg -> retval = THD_RETVAL_SNAKE_CRAFT;
			return arg;
		}
	
		//暂停0.2秒
		usleep(200000);

		log_info("Thread_run_snake : Move done.");
	}

	return arg;
}

/**
 * 启动蛇的移动线程
 */
pthread_t snake_run(thread_arg_t *arg)
{
	if (NULL == arg)
	{
		log_info("NULL pointer. %s %d", __FILE__, __LINE__);
		return 0;
	}
	
	pthread_t id;	
	int error;
	if (error = pthread_create(&id, NULL, (void *(*)(void *))thread_run_snake, (void *)arg))
	{
		log_error("Create the snake run thread ERROR. %s %d", __FILE__, __LINE__);
		return 0;
	}
	//pthread_detach(id); 		//设置为分离线程
	//log_info("Detach the snake run thtread. done.");
	return id;
}

/**
 * 判断蛇是否撞到了四周的墙壁和自己身上。
 * 是，则返回1,否则返回0
 */
static int snake_is_craft(snake* s)
{
	if (NULL == s)
	{
		log_error("NULL pointer. %s %d", __FILE__, __LINE__);
		return 0;
	}
	log_info("Snake_is_craft : max_x %d max_y %d", s -> max_x, s -> max_y);
	log_info("Snake_is_craft : snake pos(%d, %d)", s -> head -> x, s -> head -> y);
	//是否碰到墙壁
	if (s -> head -> x >= s -> max_x - 1 || s -> head -> y >= s -> max_y - 1
		|| s -> head -> x <= 0|| s -> head -> y <= 0)
	{
		return 1;
	}

	//判断是否咬到自己
	int i = 0;
	snake_node *tmp = s -> head -> next;
	for (; i < s -> len - 1; ++i)
	{
		if (tmp -> x == s -> head -> x && tmp -> y == s -> head -> y )
		{
			return 1;
		}
		tmp = tmp -> next;
	}

	return 0;
}

//设置移动方向
void snake_set_dct(snake *s, dct_t dct)
{
	if (NULL == s)
	{
		log_error("NULL pointer. %s %d", __FILE__, __LINE__);
		return;
	}
	
	switch (dct)
	{
		case UP_DCT:
			log_info("snake_set_dct: UP");
			if (s -> head -> direction == DOWN_DCT)
			{
				break;
			}
			s -> head -> direction = dct;
			break;
		case DOWN_DCT:
			log_info("snake_set_dct: DOWN");
			if (s -> head -> direction == UP_DCT)
			{
				break;
			}
			s -> head -> direction = dct;
			break;
		case LEFT_DCT:
			log_info("snake_set_dct: LEFT");
			if (s -> head -> direction == RIGHT_DCT)
			{
				break;
			}
			s -> head -> direction = dct;
			break;
		case RIGHT_DCT:
			log_info("snake_set_dct: RIGHT");
			if (s -> head -> direction == LEFT_DCT)
			{
				break;
			}
			s -> head -> direction = dct;
			break;
		case UNKNOWN_DCT:
		default:
			log_error("Set snake direction ERROR. Unknown direction %d. %s %d",dct,  __FILE__, __LINE__);
			break;
	}

	return;
}

//移动蛇
void snake_move(snake* s)
{
	if (NULL == s)
	{
		return;
	}

	//蛇身上的节点只需要根据前一节点进行移动即可。
	snake_node *tmp;
	tmp = s -> tail;
	while (tmp != s -> head)
	{
		tmp -> x = tmp -> pre -> x;
		tmp -> y = tmp -> pre -> y;
		tmp = tmp -> pre;
	}

	//根据蛇头中的方向设置蛇头的下一个位置。
	dct_t dct = s -> head -> direction;
	switch (dct)
	{
		case UP_DCT:
			s -> head -> y -= 1;
			break;
		case DOWN_DCT:
			s -> head -> y += 1;
			break;
		case LEFT_DCT:
			s -> head -> x -= 1;
			break;
		case RIGHT_DCT:
			s -> head -> x += 1;
			break;
		case UNKNOWN_DCT:
		default:
			log_error("Move snake ERROR. Unknown direction %d. %s %d",dct,  __FILE__, __LINE__);
			break;
	}

	return;
}

void snake_set_scope(snake* s, int y, int x)
{
	if (NULL == s || y < 0 || x < 0 )
	{
		return;
	}

	s -> max_x = x;
	s -> max_y = y;
}

void snake_show(snake *s, WINDOW *w)
{
	if (NULL == s || NULL == w)
	{
		log_info("NULL pointer. %s %d", __FILE__, __LINE__);
		return;
	}

	log_info("Show the snake.");
	int i;
	snake_node *tmp = s -> head;
	for (i = 0; i < s -> len; ++i)
	{
		mvwaddch(w, tmp -> y, tmp -> x, SNAKE_NODE);
		tmp = tmp -> next;
	}
	wrefresh(w);
}

void snake_clear(snake *s, WINDOW *w)
{
	if (NULL == s || NULL == w)
	{
		log_info("NULL pointer. %s %d", __FILE__, __LINE__);
		return;
	}

	log_info("Hidet the snake");
	int i;
	snake_node *tmp = s -> head;
	for (i = 0; i < s -> len; ++i)
	{
		mvwaddch(w, tmp -> y, tmp -> x, ' ');
		tmp = tmp -> next;
	}
	wrefresh(w);
}

void snake_set_pos_dct(snake *s, int y, int x,  dct_t dct)
{
	if (NULL == s || s -> len <= 0)
	{
		log_info("NULL pointer. %s %d", __FILE__, __LINE__);
		return;
	}
	
	s -> head -> x = x;
	s -> head -> y = y;

	snake_node *tmp = s -> head;
	s -> head -> direction = dct;
	int i;
	switch(dct)
	{
		case UP_DCT:
			for (i = 1; i < s -> len; ++i)
			{
				tmp = tmp -> next;
				tmp -> x = s -> head -> x;
				tmp -> y = s -> head -> y + i;
			}
			break;
		case DOWN_DCT:
			for (i = 1; i < s -> len; ++i)
			{
				tmp = tmp -> next;
				tmp -> x = s -> head -> x;
				tmp -> y = s -> head -> y - i;
			}
			break;
		case LEFT_DCT:
			for (i = 1; i < s -> len; ++i)
			{
				tmp = tmp -> next;
				tmp -> x = s -> head -> x + i;
				tmp -> y = s -> head -> y;
			}
			break;
		case RIGHT_DCT:
			for (i = 1; i < s -> len; ++i)
			{
				tmp = tmp -> next;
				tmp -> x = s -> head -> x - i;
				tmp -> y = s -> head -> y;
			}
			break;
		case UNKNOWN_DCT:
		default:
			log_error("Unknown direction! %s %d", __FILE__, __LINE__);
			break;
	}
}

/**
 ******************
 * snake的操作函数
 ******************
 */
static void snake_add_node(snake *s, snake_node *node)
{
	if (NULL == node || NULL == s)
	{
		log_info("NULL pointer. %s %d", __FILE__, __LINE__);
		return;
	}

	if (s -> head == NULL) 	//没有节点
	{
		s -> head = node;
		s -> tail = node;
		++ s -> len;
	}
	else
	{
		s -> tail -> next = node;
		node -> pre = s -> tail;
		s -> tail = node;
		++ s -> len;
	}

	return;
}

snake * snake_init()
{
	snake *s = (snake*)malloc(sizeof(*s));
	if (NULL == s)
	{
		log_error("Initial snake ERROR. %s %d", __FILE__, __LINE__);
		return NULL;
	}
	
	s -> len = 0;
	s -> direction = RIGHT_DCT;
	s -> head = NULL;
	s -> tail = NULL;

	snake_node *tmp = NULL;
	int i = 0;
	for (;i < SNAKE_INIT_LEN; ++i)
	{
		snake_add_node(s, snake_node_init());
	}
	log_info("Initial snake over. len %d.", s -> len);
	return s;
}

void snake_free(snake *s)
{
	if (NULL == s)
	{
		log_info("NULL pointer. %s %d", __FILE__, __LINE__);
		return;
	}

	snake_node *tmp = s -> head;
	int i;
	for (i = 0; i < s -> len; ++i)
	{
		tmp = s -> head -> next;
		snake_node_free(s -> head);
		s -> head = tmp;
	}

	free(s);
}

/*
 *********************************************
 * snake_node 的操作函数
 *********************************************
 */
snake_node* snake_node_init()
{
	snake_node *n = NULL;
	n = (snake_node*)malloc(sizeof(*n));
	if (NULL == n)
	{
		log_error("Initial snake node ERROR. %s %d", __FILE__, __LINE__);
		return NULL;
	}

	n -> x = -1;
	n -> y = -1;
	n -> direction = UNKNOWN_DCT;
	n -> pre = NULL;
	n -> next = NULL;
	n -> color = GREEN_ON_BLACK;

	return n;
}

/**
 * 通过坐标初始化节点
 */
snake_node* snake_node_init_yx(int y, int x)
{
	snake_node *node = snake_node_init();
	node -> x = x;
	node -> y = y;
	return node;
}

void snake_node_free(snake_node *n)
{
	if (NULL == n)
	{
		return;
	}
	free(n);
}

