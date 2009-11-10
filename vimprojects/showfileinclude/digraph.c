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
	if (NULL == a -> name || NULL == b -> name)
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
digraph *digraph_init()
{
	digraph *g = NULL;
	g = (digraph *)malloc(sizeof(digraph));

	if (NULL == g)
	{
		log_err("Create digraph error.");
		return NULL;
	}

	g -> node_cnt = 0;
	//log_info("create a digraph.");

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

	free(dg);

	return;
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
	
	dg -> nodes[dg -> node_cnt] = n;
	
	node_ptr * np = node_ptr_init();
	np -> ptr = n;
	np -> next = NULL;

	dg -> link_table[dg -> node_cnt] = np;

	++dg -> node_cnt;
//	log_info("Insert node : %s, has node : %d", n -> name, dg -> node_cnt);
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
	//log_info("Has node : %d", dg -> node_cnt);	
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
	int a_inserted = 0, b_inserted = 0;

	if (index_a < 0)
	{
		index_a = digraph_insert_node(dg, a);
		a_inserted = 1;
	}

	if (index_b < 0)
	{
		index_b = digraph_insert_node(dg, b);
		b_inserted = 1;
	}

	if (index_a < 0 || index_b < 0)
	{
		log_err("Buile edge error: can't insert a node into the graph.");
		exit(1);
	}
	
	node_ptr *b_ptr = node_ptr_init();
	b_ptr -> ptr = dg -> nodes[index_b];  //这里不能指向b！！是临时变量，可能被释放，要指向nodes数组中!!
	b_ptr -> next = NULL;
	
	//log_info("Build edge node: %d %d", index_a, index_b);

	node_ptr *insert_pos = dg -> link_table[index_a];
	int already_exist = 0;
	while(insert_pos != NULL)
	{
		already_exist = 0;
		if (insert_pos -> next == NULL)
		{
			break;
		}

		if (node_is_equal(insert_pos -> ptr, b))
		{
			already_exist = 1;
			break;
		}
		insert_pos = insert_pos -> next;
	}

	if(!already_exist)
	{
		insert_pos -> next = b_ptr;
		//log_info("Build an edge : %s --> %s", dg -> link_table[index_a] -> ptr -> name, insert_pos -> next  -> ptr -> name);
	}

	if (!b_inserted)
	{
		node_free(b);
	}
	if (!a_inserted)
	{
		node_free(a);
	}

	return 1;
}

int digraph_build_edge_string(digraph *dg, const char *s1, const char *s2)
{
	//log_info("Build edge : %s --> %s", s1, s2);

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
