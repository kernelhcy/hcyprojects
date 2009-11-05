#include "digraph.h"

//node的操作函数
node *node_init()
{
	node *n = NULL;
	n = (node *)malloc(sizeof(node));

	if ( NULL == n)
	{
		log_err("Create node error!");
		return NULL;
	}

	n -> name = NULL;
	n -> name_len = 0;
	n -> including_cnt = 0;
	n -> included_cnt = 0;

	return n;
}

node *node_init_name(const char *name, size_t name_len)
{
	node *n = NULL;
	n = (node *)malloc(sizeof(node));
	if (NULL == n)
	{
		log_err("Create node with string error!");
		return NULL;
	}

	n -> name = (char *)malloc(name_len + 1);
	
	assert(n -> name);

	strcpy(n -> name, name);
	n -> name[name_len] = '\0';
	n -> name_len = name_len;

	n -> including_cnt = 0;
	n -> included_cnt = 0;

	//log_info("Create a node with %s.", name);

	return n;
}

void node_free(node *n)
{
	free(n -> name);
	free(n);
}

void node_reset(node *n)
{
	if (NULL == n)
	{
		return;
	}
	
	if (n -> name_len == 0)
	{
		n -> including_cnt = 0;
		n -> included_cnt = 0;
		return;
	}

	n -> name[0] = '\0';
	n -> name_len = 0;
	n -> including_cnt = 0;
	n -> included_cnt = 0;

	return;

}

int node_is_equal(node *a, node *b)
{
	if (NULL == a || NULL == b)
	{
		return 0;
	}

	return (strcmp(a -> name, b -> name) == 0);
}

//node ptr操作函数
node_ptr *node_ptr_init()
{
	node_ptr *np = NULL;
	np = (node_ptr *)malloc(sizeof(node_ptr));

	if (NULL == np)
	{
		log_err("Create node ptr error!\n");
	}

	np -> ptr = NULL;
	np -> next = NULL;

	return np;

}

void node_ptr_free(node_ptr *np)
{
	free(np);
}

//graph的操作函数
//node 		**nodes; 		//结点数组，存放所有结点。
//node_ptr 	**link_table; 	//链接表
//int 		node_cnt; 		//结点的个数。
//int 		cnt; 			//数组的长度
digraph *digraph_init()
{
	digraph *g = NULL;
	g = (digraph *)malloc(sizeof(digraph));

	if (NULL == g)
	{
		log_err("Create digraph error.");
		return NULL;
	}

	g -> nodes = NULL;
	g -> link_table = NULL;
	g -> node_cnt = 0;
	g -> cnt = 0;

	//log_info("create a digraph.");

	return g;
}
digraph *digraph_init_n(size_t n)
{

	digraph *g = NULL;
	g = (digraph *)malloc(sizeof(digraph));

	if (NULL == g)
	{
		log_err("Create digraph with size n  error.");
		return NULL;
	}

	int size = BASE_LENGTH - n % BASE_LENGTH + n;

	g -> nodes = (node **)malloc(sizeof(node *) * size);
	g -> link_table = (node_ptr**)malloc(sizeof(node_ptr *) * size);
	
	assert(g -> nodes);
	assert(g -> link_table);

	memset(g -> nodes, 0, sizeof(node *) * size);
	memset(g -> link_table, 0, sizeof(node_ptr *) * size);

	g -> node_cnt = 0;
	g -> cnt = size;

	return g;
}

void digraph_free(digraph *dg)
{
	int i;
	if (NULL == dg)
	{
		return;
	}

	node_ptr *p, *pp;
	for ( i = 0; i < dg -> node_cnt; ++i)
	{
		node_free(dg -> nodes[i]);

		p = dg -> link_table[i];
		while(NULL != p)
		{
			pp = p -> next;
			node_ptr_free(p);
			p = pp;
		}
	}

	free(dg -> nodes);
	free(dg -> link_table);

	free(dg);

	return;
}

/**
 * 将dg中的数组扩展BASE_LENGTH长度
 */
static int digraph_prepare_insert(digraph *dg)
{
	if (NULL == dg)
	{
		return -1;
	}

	/**
	 * 一开始只有realloc，但当dg -> nodes 和dg -> link_table 为NULL时，直接realloc。
	 * 在free的时候会报double free的错误！！
	 *
	 * void * realloc ( void * ptr, size_t size );
	 * In case that ptr is NULL, the function behaves exactly as malloc, assigning a new 
	 * block of size bytes and returning a pointer to the beginning of it.
	 *
	 * 返回的是一个size bytes的内存块，而不是size * sizeof(node *)字节！！
	 *
	 */
	if (dg -> cnt == 0)
	{
		dg -> nodes = (node **)malloc(sizeof(node *) * BASE_LENGTH);
		dg -> link_table = (node_ptr**)malloc(sizeof(node_ptr *) * BASE_LENGTH);
	}
	else
	{
		dg -> nodes = (node **)realloc(dg -> nodes, dg -> cnt + BASE_LENGTH);
		dg -> link_table = (node_ptr**)realloc(dg -> link_table, dg -> cnt + BASE_LENGTH);
	}

	if (NULL == dg -> nodes || NULL == dg -> link_table)
	{
		log_err("Digraph prepare insert : can't realloc memory and the data is lost!!");
		exit(1);
	}

	dg -> cnt += BASE_LENGTH;

	return 1;
}

int digraph_insert_string(digraph *dg, const char *s)
{
	node *n = node_init_name(s, strlen(s));
	return digraph_insert_node(dg, n);
}
int digraph_insert_node(digraph *dg, node *n)
{
	if (NULL == dg || NULL == n)
	{
		return -1;
	}
	
	if (dg -> cnt <= dg -> node_cnt || dg -> cnt == 0)
	{
		digraph_prepare_insert(dg);
	}

	dg -> nodes[dg -> node_cnt] = n;
	
	node_ptr * np = node_ptr_init();
	np -> ptr = n;
	np -> next = NULL;

	dg -> link_table[dg -> node_cnt] = np;

	++dg -> node_cnt;

	return dg -> node_cnt - 1;
}
/**
 * 搜索b在dg中的位置，返回其在数组中的下标。
 * 若不存在b，则返回-1;
 *
 * 在目前的版本中，采用线性查找
 */
static int digraph_search_node(digraph *dg, node *n)
{
	if (NULL == dg || NULL == n)
	{
		return -1;
	}
	
	int i = 0;
	for (; i < dg -> node_cnt; ++i)
	{
		if (node_is_equal(n, dg -> nodes[i]))
		{
			return i;
		}
	}
	return -1;
}

void digraph_delete_node(digraph *dg, node *n)
{
	int index = digraph_search_node(dg, n);

	if (index < 0)
	{
		return;
	}

	dg -> nodes[index] = dg -> nodes[dg -> cnt - 1];
	dg -> nodes[dg -> cnt -1] = NULL;
	--dg -> node_cnt;
	
	return;
}

int digraph_build_edge_node(digraph *dg, node *a, node *b)
{
	if (NULL == dg || NULL == a || NULL == b)
	{
		return 0;
	}

	int index_a = digraph_search_node(dg, a); 
	int index_b = digraph_search_node(dg, b);

	if (index_a < 0)
	{
		index_a = digraph_insert_node(dg, a);
	}

	if (index_b < 0)
	{
		index_b = digraph_insert_node(dg, b);
	}

	if (index_a < 0 || index_b < 0)
	{
		log_err("Buile edge error: can't insert a node into the graph.");
		exit(1);
	}
	
	node_ptr *b_ptr = node_ptr_init();
	b_ptr -> ptr = b;
	b_ptr -> next = NULL;

	node_ptr *insert_pos = dg -> link_table[index_a];

	while(insert_pos != NULL)
	{
		if (insert_pos -> next == NULL)
		{
			break;
		}
		insert_pos = insert_pos -> next;
	}

	insert_pos -> next = b_ptr;
	insert_pos = NULL;

	return 1;
}

int digraph_build_edge_string(digraph *dg, const char *s1, const char *s2)
{
	//log_info("build edge : %s %s.", s1, s2);

	node *a = node_init_name(s1, strlen(s1));
	node *b = node_init_name(s2, strlen(s2));
	
	//log_info("build edge with name over.");
	return digraph_build_edge_node(dg, a, b);
}

void digraph_show(digraph *dg)
{
	if (NULL == dg)
	{
		printf("The graph is nothing.\n");
		return;
	}
	
	printf("\nOut the Graph linked table:\n");
	node_ptr *p;
	int i;
	for(i = 0; i < dg -> node_cnt; ++i)
	{
		printf("%s : \n", dg -> nodes[i] -> name);
		printf("\t");
		p = dg -> link_table[i] -> next;
		while(NULL != p)
		{
			printf("%s ", p -> ptr -> name);
			p = p -> next;
		}
		printf("\n");
	}

}
