#include "buffer.h"

/**
 * 以下是简易buffer的操作函数。
 */

buffer* buffer_init()
{
	buffer *b = NULL;
	b = (buffer *)malloc(sizeof(buffer));

	if (NULL == b)
	{
		log_err("Init buffer error.");
		exit(1);
	}

	b -> len = 0;
	b -> used = 0;
	b -> ptr = NULL;

	return b;
}
	
buffer* buffer_init_n(size_t n)
{
	buffer *b = NULL;
	b = (buffer *)malloc(sizeof(buffer));

	if (NULL == b)
	{
		log_err("Init buffer error.");
		exit(1);
	}
	
	size_t size = BUF_BASE_LEN - n % BUF_BASE_LEN + n;
	b -> ptr = (char *)malloc(sizeof(char) * size);
	if (NULL == b -> ptr)
	{
		log_err("Malloc memory error. %s %d", __FILE__, __LINE__);
		exit(1);
	}

	b -> len = size;
	b -> used = 0;

	return b;
}

buffer* buffer_init_string(const char *s)
{
	size_t s_size = strlen(s);
	
	size_t size = BUF_BASE_LEN - s_size % BUF_BASE_LEN + s_size;
	
	buffer *b = NULL;
	b = (buffer *)malloc(sizeof(buffer));

	if (NULL == b)
	{
		log_err("Init buffer error.");
		exit(1);
	}
	
	b -> ptr = (char *)malloc(sizeof(char) * size);
	if (NULL == b -> ptr)
	{
		log_err("Malloc memory error. %s %d", __FILE__, __LINE__);
		exit(1);
	}

	b -> len = size;
	b -> used = s_size;
	
	memcpy(b -> ptr, s, s_size);
	
	return b;
}
/**
 * 向buffer中追加字符串，如果空间不够，重新分配空间
 */
int buffer_append(buffer *buf, const char *s, size_t s_len)
{
	if (NULL == buf || NULL == s || 0 == s_len)
	{
		return 0;
	}

	size_t size = BUF_BASE_LEN - s_len % BUF_BASE_LEN + s_len;
	

	if ( buf -> ptr == NULL || buf -> len == 0)
	{
		buf -> ptr = (char *)malloc(sizeof(char) * size);
		if (NULL == buf -> ptr)
		{
			log_err("Malloc memory error. %s %d", __FILE__, __LINE__);
			exit(1);
		}
		buf -> len = size;
	}

	if (buf -> len - buf -> used < s_len)
	{
		buf -> ptr = (char *)realloc(buf -> ptr, sizeof(char) * (size + buf -> len));
		if (NULL == buf -> ptr)
		{
			log_err("Malloc memory error. %s %d", __FILE__, __LINE__);
			exit(1);
		}
		buf -> len += size;
	}

	char * start_pos = buf -> ptr + buf -> used;
	memcpy(start_pos, s, s_len);
	buf -> used += s_len;
	buf -> ptr[buf -> used] = '\0';

	return s_len;
}

void buffer_free(buffer *buf)
{
	if (NULL == buf)
	{
		return;		
	}
	
	free(buf -> ptr);
	free(buf);
	return;
}

buffer_array* buffer_array_init()
{
	buffer_array * ba = (buffer_array*) malloc(sizeof(buffer_array));
	assert(ba);
	
	ba -> len = 0;
	ba -> used = 0;
	
	return ba;
}

//初始化数组的大小为n
buffer_array* buffer_array_init_n(size_t n)
{
	buffer_array * ba = (buffer_array*) malloc(sizeof(buffer_array));
	assert(ba);
	
	size_t size = BUF_BASE_LEN - n % BUF_BASE_LEN + n;
	ba -> ptr = (buffer **)malloc(size * sizeof(buffer*));
	assert(ba);
	
	ba -> len = size;
	ba -> used = 0;
	
	return ba;
}
//释放数组占用的空间
void buffer_array_free(buffer_array* ba)
{
	if (NULL == ba)
	{
		return;
	}
	
	int i;
	for (i = 0; i < ba -> used; ++i)
	{
		if (ba -> ptr[i])
		{
			buffer_free(ba -> ptr[i]);
		}
		
	}
	
	free(ba);
	return;
}

/*
 * 为向数组中追加数据而准备空间。
 * 如果有空间，什么也不做。如果没有空间，则分配BUF_BASE_LEN的空间。
 */
static void buffer_array_prepare_append(buffer_array* ba)
{
	if (NULL == ba)
	{
		return;
	}
	
	if (ba -> used < ba -> len)
	{
		return;
	}
	
	//数组已满，重新分配更多的空间。
	ba -> ptr = (buffer**) realloc(ba -> ptr, sizeof(buffer*) * (ba -> len + BUF_BASE_LEN));
	assert(ba);
	
	ba -> len += BUF_BASE_LEN;
	
}

//追加数据到数组中
int buffer_array_append(buffer_array* ba, buffer* buf)
{
	if (NULL == ba || NULL == buf)
	{
		return 0;
	}
	buffer_array_prepare_append(ba);
	ba -> ptr[ba -> used] = buf;
	++ba -> used;
	return 1;
}

int buffer_array_append_string(buffer_array* ba, const char *s)
{
	if (NULL == ba || NULL == s)
	{
		return 0;
	}
	
	return buffer_array_append(ba, buffer_init_string(s));
	
}
//删除数据
int buffer_array_delete(buffer_array* ba, buffer* buf)
{
	return 0;
}
int buffer_array_delete_string(buffer_array* ba, const char *s)
{
	return 0;
}

