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

//用b初始化一个新的buffer。
buffer* buffer_init_copy(buffer *b)
{
	buffer *a = buffer_init();
	buffer_copy(a, b);
	return a;
}
/**
 * 保证buffer b至少有n的剩余空间。
 * 如果n小于等于0,则什么也不做。
 */
static int buffer_prepare_n(buffer *buf, size_t n)
{
	if (NULL == buf || n <= 0)
	{
		return 0;
	}

	size_t size = BUF_BASE_LEN - n % BUF_BASE_LEN + n;

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

	if (buf -> len - buf -> used < n)
	{
		buf -> ptr = (char *)realloc(buf -> ptr, sizeof(char) * (size + buf -> len));
		if (NULL == buf -> ptr)
		{
			log_err("Malloc memory error. %s %d", __FILE__, __LINE__);
			exit(1);
		}
		buf -> len += size;
	}

	
	return 1;
}

int buffer_copy(buffer *a, buffer *b)
{
	if(NULL == a || NULL == b)
	{
		return 0;
	}

	//保证a的空间足够存放b的数据。
	buffer_prepare_n(a, b -> used - a -> used);

	memcpy(a -> ptr, b -> ptr, b -> used + 1);
	a -> used = b -> used;
	a -> len = b -> len;
	a -> ptr[a -> used] = '\0';

	return 1;
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
	//log_info("Buffer append : %d %s %d", buf, s, s_len);
	buffer_prepare_n(buf, s_len);

	char * start_pos = buf -> ptr + buf -> used;
	memcpy(start_pos, s, s_len);
	buf -> used += s_len;
	buf -> ptr[buf -> used] = '\0';

	return s_len;
}

int buffer_cmp(buffer *a, buffer *b)
{
	if (NULL == a || NULL == b)
	{
		return 0;
	}

	if (a -> used != b -> used)
	{
		return 0;
	}

	return strncmp(a -> ptr, b -> ptr, b -> used) == 0;
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
	ba -> ptr = NULL; //切记初始化！！！！！！！！！！！！！
	
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
	ba -> ptr = NULL;	
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
	if (NULL == ba -> ptr)
	{
		ba -> ptr = (buffer **)malloc(sizeof(buffer*) * BUF_BASE_LEN);
	}
	else
	{
		//数组已满，重新分配更多的空间。
		ba -> ptr = (buffer**) realloc(ba -> ptr, sizeof(buffer*) * (ba -> len + BUF_BASE_LEN));
	}	
	assert(ba);
	
	ba -> len += BUF_BASE_LEN;

	size_t i;
	for (i = ba -> used; i < ba -> len; ++i)
	{
		ba -> ptr[i] = NULL;
	}
	
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

int buffer_path_simple(buffer *buf)
{
	int cnt = buf -> used;
	while(cnt >= 0 &&  buf -> ptr[cnt] != '/')
	{
		--cnt;
	}

	if (cnt < 0)
	{
		return 0;
	}

	++cnt;
	memmove(buf -> ptr, buf -> ptr + cnt, buf -> used - cnt);
	buf -> used -= cnt;

	return 0;
}


/*
 * 将字符串以"/"为分割符进行分割。
 * 将分割的结果保存在buffer数组中返回。
 */
buffer_array* splite_by_slash(const char *s)
{
	if (NULL == s)
	{
		return NULL;
	}

	buffer *buf;
	buffer_array 	*ba = buffer_array_init();
	if (s[0] == '/')
	{
		buf = buffer_init_string("/");
		buffer_array_append(ba, buf);
	}

	int begin = 0, end = 0;
	int len = strlen(s);
	
	while (end < len)
	{
		if (s[end] == '/' && end != 0 && begin != end) //begin!=end 处理路径中的"//"
		{
			buf = buffer_init();
			if (s[begin] == '/')
			{
				buffer_append(buf, s + begin + 1, end - begin - 1);
			}
			else 
			{
				buffer_append(buf, s + begin, end - begin);
			}
			buffer_array_append(ba, buf);

			begin = end;
		}
		++end;
	}

	//处理最后的部分，去除可能含有的最后一个"/"
	if(end - begin > 1 || s[begin] != '/')
	{
		buf = buffer_init();
		if (s[begin] == '/')
		{
			buffer_append(buf, s + begin + 1, end - begin - 1);
		}
		else 
		{
			buffer_append(buf, s + begin, end - begin);
		}
		buffer_array_append(ba, buf);
	}
	return ba;
}

void buffer_array_print(buffer_array *ba)
{
	if (NULL == ba || ba -> used == 0)
	{
		log_info("Nothing to print in buffer array.");
	}

	printf("Print buffer array: \n");
	size_t i;
	for (i = 0; i < ba -> used; ++i)
	{
		printf(" %s\t(used: %d len %d)\n", ba -> ptr[i] -> ptr, ba -> ptr[i] -> used, ba -> ptr[i] -> len);
	}

	printf("\n");
	return;
}
