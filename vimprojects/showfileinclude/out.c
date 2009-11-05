#include "out.h"

//输出的文件名。
static const char *name = NULL;

//
static int create_txt(digraph *dg);
//
static type_t pic_type;
static int create_pic(digraph *dg);


int create_out(digraph *dg, type_t t, const char *n)
{
	if (NULL == dg)
	{
		return 0;
	}

	name = n;

	int state;
	switch(t)
	{
		case TXT_T:
			state = create_txt(dg);		
			break;
		case JPG_T:
			pic_type = t;
		case PNG_T:
			pic_type = t;
			state = create_pic(dg);
			break;
		default:
			log_err("Unknown type %d. File: %s, Line %d \n", t, __FILE__, __LINE__);
			exit(1);
	}
	return state;
}

static int create_txt(digraph *dg)
{
	
	if (NULL == dg)
	{
		printf("The graph is nothing.\n");
		return;
	}
	
	int fd;
	if ((fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0)
	{
		log_err("Can't create file %s.", name);
		exit(1);
	}
	char buf[100];
	strcpy(buf, "Out the Graph linked table: \n");
	write(fd, buf, strlen(buf));

	node_ptr *p;
	int i;
	for(i = 0; i < dg -> node_cnt; ++i)
	{
		sprintf(buf,"%s : \n\t", dg -> nodes[i] -> name);
		write(fd, buf, strlen(buf));
		p = dg -> link_table[i] -> next;
		while(NULL != p)
		{
			sprintf(buf, "%s ", p -> ptr -> name);
			write(fd, buf, strlen(buf));
			p = p -> next;
		}
		write(fd, "\n", strlen("\n"));

	}

	close(fd);
	return;
}
static int create_pic(digraph *dg)
{
	buffer *buf = buffer_init_n(128);
	buffer_append(buf, "digraph ", strlen("digraph "));
	buffer_append(buf, "sfi", strlen("sfi"));
	buffer_append(buf, "{\n", strlen("{\n"));

	node_ptr *p;
	int i;
	for(i = 0; i < dg -> node_cnt; ++i)
	{
		p = dg -> link_table[i] -> next;
		while(NULL != p)
		{
			buffer_append(buf, dg -> link_table[i] -> ptr -> name
					, dg -> link_table[i] -> ptr -> name_len);
			buffer_append(buf, " -> ", strlen(" -> "));
			buffer_append(buf, p -> ptr -> name, p -> ptr -> name_len);
			buffer_append(buf, ";\n", strlen(";\n"));
			p = p -> next;
		}
	}

	buffer_append(buf, "}\n", strlen("}\n"));

	char arg1[100]; 	//-Txxx
	char arg2[100]; 	//-o xxx.xxx
	
	const char * type_s;
	if (pic_type == JPG_T)
	{
		type_s = "jpg";
	}
	else if (pic_type == PNG_T)
	{
		type_s = "png";
	}

	strcpy(arg1, "-T");
	strcat(arg1, type_s);

	strcpy(arg2, "-o ");
	strcat(arg2, name);
	strcat(arg2, ".");
	strcat(arg2, type_s);

	int 	fd[2];
	pid_t 	pid;

	if (pipe(fd) < 0)
	{
		log_err("pipe error. %s %d", __FILE__, __LINE__);
		exit(1);
	}

	if ((pid = fork()) < 0)
	{
		log_err("fork error. %s %d", __FILE__, __LINE__);
		exit(1);
	}

	if (pid > 0) 	//parent
	{
		close(fd[0]);
		if (write(fd[1], buf -> ptr, buf -> used) != buf -> used )
		{
			log_err("write to pipe error. %s %d", __FILE__, __LINE__);
			exit(1);
		}
		close(fd[1]);
		if (waitpid(pid, NULL, 0) < 0)
		{
			log_err("waitpid error. %s %d", __FILE__, __LINE__);
			exit(1);
		}
		exit(0);
	}
	else 		//child
	{
		close(fd[1]);

		if (fd[0] != STDIN_FILENO)
		{
			if (dup2(fd[0], STDIN_FILENO) != STDIN_FILENO)
			{
				log_err("dup2 error to stdin");
				close(fd[0]);
				exit(1);
			}
			close(fd[0]);
		}
		
		if (execl("/usr/bin/dot", "", arg1, arg2, (char *)0) < 0)
		{
			log_err("execl dot error.");
			exit(1);
		}
	}
	exit(0);
}

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
	
	int size = BUF_BASE_LEN - n % BUF_BASE_LEN + n;
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

int buffer_append(buffer *buf, const char *s, size_t s_len)
{
	if (NULL == buf || NULL == s || 0 == s_len)
	{
		return 0;
	}

	int size = BUF_BASE_LEN - s_len % BUF_BASE_LEN + s_len;
	

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

