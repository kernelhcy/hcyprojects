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

//将路径的基目录设置为s
int path_set_base_dir(path_t *p, const char *s)
{
	if (NULL == s || NULL == p)
	{
		return 0;
	}
	size_t size = strlen(s);
	memcpy(p -> path, s, size);
	
	return size;
}

/*
 * 将目录调整为path。
 * 注意： path必须为p基目录的子目录。不能在调用了path_set_base_dir或者
 * 掉用path_init_string后，将目录设置为非基目录下的路径。如果设置，
 * 函数将直接将其追加到路径的尾部，这样将得到错误的路径！！
 */
int path_cd(path_t *p, const char *path)
{
	if (NULL == p || NULL == path)
	{
		return 0;
	}
	
	size_t index = 0;
	
	/* 处理".."和"."。这个函数在程序中的大部分作用是处理这两个目录。 */
	while(path[index++] != '.');
	
	if (path[index] != '.') //"."
	{
		return 1;
	} 
	else 					//".."
	{
		return path_parent(p);
	}
	
	//处理一般情况。
	
}


int path_parent(path_t *p)
{
	if (NULL == p)
	{
		return 0;
	}
	
	size_t index = p -> path -> used;
	
	//delete the '/' at the tail of the path
	//like '/home/xxx/test/'.
	if (p -> path -> ptr[index] == '/')
	{
		--index;
	}
	
	while (p -> path -> ptr[index] != '/') 
	{
		--index;
	}
	
	p -> path -> ptr[index] = '\0';
	p -> path -> used = index + 1;
	p -> is_dir = 1; 		//设置目录位
	
	return 1;
}

/*
 * 返回当前路径所指向的文件的文件名。
 * 如果不是文件，则返回NULL。
 */
char* path_file(path_t *p)
{
	if (NULL == p || p -> is_dir == 1)
	{
		return 0;
	}
	
	
}
