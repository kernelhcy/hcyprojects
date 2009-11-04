#ifndef _DIGRAPH_H
#define _DIGRAPH_H
/*
 * 定义一个有向图接口
 * 有向图采用邻接表的形式保存。
 */

/*
 * 定义图的结点
 */
struct _node
{
	char 	*name; 			//结点的名称。也就是文件名称
	int 	name_len; 		//名字的长度。

	int 	including_cnt; 	//包含的文件数目。
	int 	included_cnt; 	//被包含的文件数目。

	struct 	_node *next; 	//下一个结点。
}node;

/*
 * 定义有向图
 */
struct _graph
{
	node 	*nodes; 		//结点数组，存放所有结点。
	int 	node_cnt; 		//结点的个数。

}digraph;

//node的操作函数
node *node_init();
node *node_init_name(char *name, int name_len);
void node_free(node *n);
void node_reset(node *n);

//graph的操作函数
digraph *digraph_init();
void digraph_free(digraph *dg);
void digraph_insert_node(digraph *dg, node *n);
void digraph_delete_node(digraph *dg, node *n);

#endif
