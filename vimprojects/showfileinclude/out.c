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
			buffer_append(buf, "\"", 1);
			buffer_append(buf, dg -> nodes[i] -> name, dg -> nodes[i] -> name_len);
			buffer_append(buf, "\"", 1);
			buffer_append(buf, " -> ", strlen(" -> "));
			buffer_append(buf, "\"", 1);
			buffer_append(buf, p -> ptr -> name, p -> ptr -> name_len);
			buffer_append(buf, "\"", 1);
			buffer_append(buf, ";\n", strlen(";\n"));
			//log_info("Create pic insert edge: %s --> %s", dg -> nodes[i] -> name, p -> ptr -> name);
			p = p -> next;
		}
	}

	buffer_append(buf, "}\n", strlen("}\n"));
	printf("\n%s\n\n\n", buf -> ptr);	
	/*
	 * 拼接dot所需要的参数。
	 * 一个是-Ttype
	 * 一个是-ofilename
	 */
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

	strcpy(arg2, "-o");
	strcat(arg2, name);
	strcat(arg2, ".");
	strcat(arg2, type_s);

	/*
	 * 从这开始是创建子进程，从子进程中启动dot程序，生成图片。
	 */

	int 	fd[2];
	pid_t 	pid;

	if (pipe(fd) < 0) //创建管道
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
		//父进程中将拼接好的字符串发送给子进程。
		if (write(fd[1], buf -> ptr, buf -> used) != buf -> used )
		{
			log_err("write to pipe error. %s %d", __FILE__, __LINE__);
			exit(1);
		}
		close(fd[1]);
		//等待子进程运行结束
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
			/*
			 * 将子进程的标准输入映射到管道上，以接收来自父进程的数据。
			 */
			if (dup2(fd[0], STDIN_FILENO) != STDIN_FILENO)
			{
				log_err("dup2 error to stdin");
				close(fd[0]);
				exit(1);
			}
			close(fd[0]);
		}
		/*
		 * 启动dot。
		 * 传参的时候，第一个参数是忽略的，这个为什么呢？？？
		 *
		 */
		if (execl("/usr/bin/dot", "", arg1, arg2, (char *)0) < 0)
		{
			log_err("execl dot error.");
			exit(1);
		}
	}
	exit(0);
}

