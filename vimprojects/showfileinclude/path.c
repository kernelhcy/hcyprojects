#include "path.h"

/*
 * 将路径的格式转换成UNIX系统的形式。
 * 也就是将路径中的'\'改成'/'
 *
 */
static void path_to_unix(path_t *p)
{
	if (NULL == p)
	{
		return;
	}
	
	int i;
	for (i = 0; i < p -> path -> used; ++i)
	{
		if (p -> path -> ptr[i] == '\\')
		{
			p -> path -> ptr[i] = '/';
		}
	}
	
	return;
}

path_t* path_init()
{
	path_t *p = NULL;
	p = (path_t*)malloc(sizeof(path_t));
	
	if(NULL == p)
	{
		log_err("Init path_t error. %s %d", __FILE__, __LINE__);
		exit(1);
	}
	
	p -> path = buffer_init();
	assert(p -> path);
	
	p -> is_dir = 0;
	
	return p;
}

path_t* path_init_string(const char *s)
{

	path_t *p = NULL;
	p = (path_t*)malloc(sizeof(path_t));
	
	if(NULL == p)
	{
		log_err("Init path_t error. %s %d", __FILE__, __LINE__);
		exit(1);
	}
	
	p -> path = buffer_init_string(s);
	assert(p -> path);
	
	path_to_unix(p);
	p -> is_dir = 0;
	
	return p;
}

void path_free(path_t* path)
{
	if (NULL == path)
	{
		return;
	}
	
	buffer_free(path -> path);
	free(path);
}

