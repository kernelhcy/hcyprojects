#ifndef _DIGRAPH_H
#define _DIGRAPH_H
#include "headers.h"
#include "log.h"
#include "buffer.h"
/*
 * 定义一个有向图接口
 * 有向图采用邻接表的形式保存。
 */

/*
 * 限定图的最大纬度！
 * 由于图中需要处理数据的指针，因此，存储数据的数组不能变更。也就是数组的大小不能变！
 * 因此，要预先的设定好数组的长度。
 */
#define MAX_LEN 1000

/*
 * 定义图的结点
 */
typedef struct _node
{
	buffer 	*name; 			//结点的名称。也就是文件名称

	size_t 	including_cnt; 	//包含的文件数目。
	size_t 	included_cnt; 	//被包含的文件数目。
}node;

/**
 * node指针
 * 用于有向图中的链接表。
 */
typedef struct _node_ptr
{
	node 				*ptr; 	//指向结点
	struct _node_ptr 	*next; 	//指向连表中的下一个结点
}node_ptr;

#define BASE_LENGTH 16 	//graph中结点数组node和link_table的长度必须为此值的正数倍。
/*
 * 定义有向图
 */
typedef struct _graph
{
	node 		*nodes[MAX_LEN]; 		//结点指针数组，存放所有结点。
	node_ptr 	*link_table[MAX_LEN]; 	//链接表

	size_t 		node_cnt; 		//结点的个数。
}digraph;

//node的操作函数
node *node_init();
node *node_init_name(const char *name, size_t name_len);
void node_free(node *n);
void node_reset(node *n);
int node_is_equal(node *a, node *b);

//node ptr操作函数
node_ptr *node_ptr_init();
void node_ptr_free(node_ptr *np);

//graph的操作函数
digraph *digraph_init();
void digraph_free(digraph *dg);

/**
 * 插入一个结点，并返回其在dg中数组的下标
 */
int digraph_insert_node(digraph *dg, node *n);
int digraph_insert_string(digraph *dg, const char *s);
void digraph_delete_node(digraph *dg, node *n);

/**
 * 构造一条从a到b的有向边
 */
int digraph_build_edge_node(digraph *dg, node *a, node *b);
int digraph_build_edge_string(digraph *dg, const char *s1, const char *s2);

void digraph_show(digraph *dg);
#endif
