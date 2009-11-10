#include "pathtree.h"


/* 目录树结点的操作函数 */
path_tree_node_t* path_tree_node_init()
{
	path_tree_node_t *ptn;
	ptn = (path_tree_node_t*)malloc(sizeof(*ptn));
	if (NULL == ptn)
	{
		log_err("Create path tree node error. %s %d", __FILE__, __LINE__);
		exit(1);
	}
	
	ptn -> name = buffer_init();
	ptn -> is_dir = 0;
	ptn -> parent = NULL;
	ptn -> children_cnt = 0;
	ptn -> children = NULL;
	ptn -> len = 0;

	return ptn;
}

path_tree_node_t* path_tree_node_init_string(const char *s)
{

	path_tree_node_t *ptn;
	ptn = (path_tree_node_t*)malloc(sizeof(*ptn));
	if (NULL == ptn)
	{
		log_err("Create path tree node error. %s %d", __FILE__, __LINE__);
		exit(1);
	}
	
	ptn -> name = buffer_init_string(s);
	ptn -> is_dir = 0;
	ptn -> parent = NULL;
	ptn -> children_cnt = 0;
	ptn -> children = NULL;
	ptn -> len = 0;

	return ptn;
}

/**
 * 为增加子结点准备空间。
 * 以BUF_BASE_LEN的倍数增加子结点数组的长度。
 */
static int path_tree_node_prepare_add(path_tree_node_t *p)
{
	if (NULL == p)
	{
		return 0;
	}

	if (p -> len != 0 && p -> children_cnt < p -> len)
	{
		return 0;
	}
	if (NULL == p -> children)
	{
		p -> children = (path_tree_node_t**)malloc(sizeof(path_tree_node_t*) * BUF_BASE_LEN);
	}
	else
	{
		p -> children = (path_tree_node_t**)realloc(p -> children, sizeof(path_tree_node_t*) * (BUF_BASE_LEN + p -> len));
	}


	if (NULL == p)
	{
		log_err("Prepare memory for inserting child node error. %s %d", __FILE__, __LINE__);
		exit(1);
	}
	
	p -> len += BUF_BASE_LEN;

	size_t i;
	for (i = p -> children_cnt; i < p -> len; ++i)
	{
		p -> children[i] = NULL;
	}

	return 1;
}

/*
 * 向结点ptn中增加一个子结点
 */
path_tree_node_t* path_tree_node_add_child(path_tree_node_t *ptn, path_tree_node_t *chd)
{
	if (NULL == ptn || NULL == chd)
	{
		return NULL;
	}

	path_tree_node_prepare_add(ptn);
	ptn -> children[ptn -> children_cnt] = chd;
	chd -> parent = ptn;
	++ptn -> children_cnt;

	return chd;
}

void path_tree_node_free(path_tree_node_t *ptn)
{
	if (NULL == ptn)
	{
		return;
	}

	buffer_free(ptn -> name);
	
	int i;
	for(i = 0; i < ptn -> children_cnt; ++i)
	{
		path_tree_node_free(ptn -> children[i]);
	}
	
	free(ptn);
	return;
}

/* 目录树操作函数 */
path_tree_t* path_tree_init()
{
	path_tree_t *pt;
	pt = (path_tree_t*)malloc(sizeof(*pt));

	if (NULL == pt)
	{
		log_err("Create path tree error. %s %d", __FILE__, __LINE__);
		exit(1);
	}

	pt -> root = NULL;

	return pt;
}
void path_tree_free(path_tree_t *pt)
{
	if (NULL == pt)
	{
		return;
	}
	path_tree_node_free(pt -> root);

	free(pt);
}

/*
 * 比较两个字符串的一部分
 * b1,e1标记s1的开始和结束，b2,e2标记s2的开始和结束。
 * 相同返回非0,不同返回0
 * 当有一个为NULL或都为NULL时，返回0
 */
static int strcmp_nn(const char *s1, int b1, int e1, const char *s2, int b2, int e2)
{
	if (NULL == s1 || NULL == s2)
	{
		return 0;
	}
	if (b1 < 0 || b2 < 0 || e1 < 0 || e2 < 0 || e1 > strlen(s1) || e2 > strlen(s2))
	{
		return 0;
	}
	//不同长度
	if (e1 - b1 != e2 - b2)
	{
		return 0;
	}

	while (b1 != e1 && b2 != e2 && s1[b1] == s2[b2])
	{
		++b1;
		++b2;
	}

	if (b1 == e1) //相同
	{
		return 1;
	}

	return 0;
}

/**
 * 将ba保存的路径存储到pt中。如果存在此路径，则不存储。
 * ba中保存的是相对于基路径的绝对路径。
 */
static int path_tree_add_help(path_tree_node_t *ptn, buffer_array *ba)
{
	if (NULL == ptn || NULL == ba || ba -> used <= 0)
	{
		return 0;
	}
	
	size_t i, j;
	int find = 0;
	path_tree_node_t *p = ptn;
	for (i = 1; i < ba -> used; ++i)
	{
		for (j = 0; j < p -> children_cnt; ++j)
		{
			if (buffer_cmp(ba -> ptr[i], p -> children[j] -> name))
			{
				find = 1;
				break;
			}
		}

		if (find)
		{
			find = 0;
			p = p -> children[j];
			continue;
		}

		//add the new path
		path_tree_node_t *tmp = path_tree_node_init();
		tmp -> name = buffer_init_copy(ba -> ptr[i]);
		p = path_tree_node_add_child(p, tmp);
		//log_info("Path insert node: %s", tmp -> name -> ptr);
	}

	return 1;

}
/*
 * 向目录树中追加目录
 */
int path_tree_add(path_tree_t *pt, const char *path)
{
	if (NULL == pt || NULL == path)
	{
		return 0;
	}

	//log_info("Insert path %s", path);

	int begin = 0, end = 0;
	
	buffer_array *ba = splite_by_slash(path);
	//buffer_array_print(ba);
	if (NULL == ba)
	{
		log_info("Path has nothing. %s %d", __FILE__, __LINE__);
		return 0;
	}
	
	if (pt -> root == NULL)
	{
		pt -> root = path_tree_node_init_string((ba -> ptr[0]) -> ptr);
	}

	return path_tree_add_help(pt -> root, ba);	
}

//打印帮助函数
static int print_help(path_tree_node_t *ptn, int shift)
{
	if (NULL == ptn)
	{
		return;
	}
	int i = 0;
	while (i < shift)
	{
		printf("    ");
		++i;
	}
	if (ptn -> children_cnt == 0)
	{
		printf("%s", ptn -> name -> ptr);
	}
	else
	{
		printf("/%s (child %d len %d)\n", ptn -> name -> ptr, ptn -> children_cnt, ptn -> len);
		for (i = 0; i < ptn -> children_cnt; ++i)
		{
			print_help(ptn -> children[i], shift + 1);
		}
	}
	printf("\n");
	return;
}

void path_tree_print(path_tree_t *pt)
{
	if (NULL == pt)
	{
		log_info("The path tree is empty.");
		return;
	}
	print_help(pt -> root, 0);
	return;
}

/**
 * 获取目录树中所有路径的帮助函数。
 */
static int path_tree_gaps_help(buffer_array *ba, buffer *b,  path_tree_node_t *ptn)
{
	if (NULL == ba || NULL == ptn || NULL == b)
	{
		return 0;
	}
	if (b -> len > 0 && b -> ptr[b -> used - 1] != '/')
	{
		buffer_append(b, "/", strlen("/"));
	}

	buffer_append(b, ptn -> name -> ptr, strlen(ptn -> name -> ptr));
	
	if (ptn -> children_cnt == 0)
	{
		buffer_array_append(ba, b);
		return 1;
	}

	size_t i;
	buffer *tmp = NULL;
	for (i = 0; i < ptn -> children_cnt; ++i)
	{
		tmp = buffer_init_copy(b);
		path_tree_gaps_help(ba, tmp, ptn -> children[i]);
	}
	
	return 1;
}

//将目录树中所有的路径以完整的形式存放在buffer数组中。
buffer_array* path_tree_get_all_paths(path_tree_t *pt)
{
	if (NULL == pt)
	{
		return NULL;
	}

	buffer_array *ba = buffer_array_init();
	buffer *b = buffer_init();

	path_tree_gaps_help(ba, b, pt -> root);

	return ba;
}

buffer* path_tree_simple_path(path_tree_t *pt, const char *path)
{
	if (NULL == pt || NULL == path)
	{
		return NULL;
	}

	buffer_array *ba = splite_by_slash(path);
	buffer *simple_path = buffer_init();

	/*
	 * 保存路径path在目录数各层的索引好。
	 * 也就是子结点的索引号。
	 */
	size_t indexs[100];
	size_t index = 0;

	if (NULL == ba || ba -> used <= 0)
	{
		return NULL;
	}

	path_tree_node_t *ptn = pt -> root;
	if (!buffer_cmp(ba -> ptr[0], ptn -> name))
	{
		return NULL;
	}
	size_t 	i, j;
	int 	find;
	for (i = 1; i < ba -> used; ++i)
	{
		if (strcmp("..", ba -> ptr[i] -> ptr) == 0)
		{
			ptn = ptn -> parent;
			--index;
			continue;
		}

		if (strcmp(".", ba -> ptr[i] -> ptr) == 0)
		{
			continue;
		}

		find = 0;
		for (j = 0; j < ptn -> children_cnt; ++j)
		{
			if (buffer_cmp(ba -> ptr[i], ptn -> children[j] -> name))
			{
				indexs[index] = j;
				++index;
				ptn = ptn -> children[j];
				find = 1;
				break;
			}
		}

		if (!find)
		{
			return NULL;
		}

	}
	//create the simple path
	ptn = pt -> root;
	buffer_append(simple_path, ptn -> name -> ptr, ptn -> name -> used);
	for (i = 0; i < index ; ++i)
	{
		ptn = ptn -> children[indexs[i]];
		buffer_append(simple_path, "/", strlen("/"));
		buffer_append(simple_path, ptn -> name -> ptr, ptn -> name -> used);
	}
	//log_info("Simple path : %s %d", simple_path -> ptr, ba -> used);
	return simple_path;
}

