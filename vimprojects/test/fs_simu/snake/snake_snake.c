#include "snake_snake.h"
#include "snake_base.h"
#include <string.h>
#include "log.h"

//函数声明
static snake_node* snake_node_init();
static void snake_node_free(snake_node *);
static snake_node* snake_node_init_yx(int y, int x);

void snake_run(snake *s)
{
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
		return;
	}

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
		return;
	}

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
		return;
	}
	
	s -> head -> x = x;
	s -> head -> y = y;

	snake_node *tmp = s -> head;
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

