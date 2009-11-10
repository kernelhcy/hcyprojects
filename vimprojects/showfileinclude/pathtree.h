#ifndef _PATH_TREE_H
#define _PATH_TREE_H
/*
 * 定义一个路径树
 * 用于表示一个目录及其子目录。树根是要表示的目录。
 */
#include "headers.h"
#include "buffer.h"
#include "log.h"
/*
 * 定义树的结点。
 * 
 */
typedef struct path_tree_node
{
	buffer 	*name;		//结点的名称。目录的名称
	int 	is_dir;		//是否是目录。决定其是否有子目录
	struct path_tree_node *parent;		//父目录
	
	struct path_tree_node **children;	//子目录数组
	int 	children_cnt;				//子目录个数
	int 	len; 						//子目录数组的长度
		
}path_tree_node_t;

/*
 * 定义目录树
 */
typedef struct path_tree
{
	path_tree_node_t 	*root;
	
}path_tree_t;

/* 目录树结点的操作函数 */
path_tree_node_t* path_tree_node_init();
path_tree_node_t* path_tree_node_init_string(const char *s);
void path_tree_node_free(path_tree_node_t *ptn);

/*
 * 向结点ptn中增加一个子结点
 * 返回子结点
 */
path_tree_node_t* path_tree_node_add_child(path_tree_node_t *ptn, path_tree_node_t *chd);

/* 目录树操作函数 */
path_tree_t* path_tree_init();
void path_tree_free(path_tree_t *pt);
/*
 * 向目录树中追加目录
 */
int path_tree_add(path_tree_t *pt, const char *path);

//输出树
void path_tree_print(path_tree_t *pt);

//将目录树中所有的路径以完整的形式存放在buffer数组中。
buffer_array* path_tree_get_all_paths(path_tree_t *pt);

/**
 * 解析并查找路径path。
 * 如果路径在路径树pt中，则返回路径中最后一个结点的指针。
 * 如果不在路径树pt中，返回NULL。
 */
buffer* path_tree_simple_path(path_tree_t *pt, const char *path);

#endif
