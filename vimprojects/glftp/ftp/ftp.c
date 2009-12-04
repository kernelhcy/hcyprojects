/*
 ***************************************************************************
 *									   
 * ftplib.c - callable ftp access routines				   
 * Copyright(C) 1996-2001 Thomas Pfau, pfau@eclipse.net		   
 *	1407 Thomas Ave, North Brunswick, NJ, 08902			   
 *									   
 * This library is free software; you can redistribute it and/or	   
 * modify it under the terms of the GNU Library General Public		   
 * License as published by the Free Software Foundation; either		   
 * version 2 of the License, or(at your option) any later version.	   
 * 									   
 * This library is distributed in the hope that it will be useful,	   
 * but WITHOUT ANY WARRANTY; without even the implied warranty of	   
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU	   
 * Library General Public License for more details.			   
 * 									   
 * You should have received a copy of the GNU Library General Public	   
 * License along with this progam; if not, write to the			   
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,		   
 * Boston, MA 02111-1307, USA.						   
 * 									   
 ***************************************************************************
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "ftp.h"

#define FTPLIB_BUFSIZ 8192
#define ACCEPT_TIMEOUT 30

#define FTPLIB_CONTROL 0
#define FTPLIB_READ 1
#define FTPLIB_WRITE 2

#if !defined FTPLIB_DEFMODE
#define FTPLIB_DEFMODE FTPLIB_PASSIVE
#endif
/*
 * 保存当前连接的信息。
 * 以及控制信息
 */
struct _netbuf
{
  	int handle; 				//控制连接的socket描述符

	char *cput; 				//cput用来作为从socket读取数据用的临时缓存。
 	char *cget; 				//
  	int cavail; 				//cput中数据的长度。
	int cleft; 					//cput中空闲的空间的长度。
  	char *buf; 					//数据缓存。

	char *host; 				//ftp服务器地址。
	int port;  					//端口号
	char *root_dir; 				//用户登录时的起始目录
	char *user; 				//用户名
	char *passwd; 				//用户密码
	
  	int dir;
  	netbuf *ctrl;
  	netbuf *data;
  	int cmode;
  	struct timeval idletime;
  	ftp_callback idlecb;
  	void *idlearg;
  	int xfered;
  	int cbbytes;
  	int xfered1;
  	char response[256]; 		//用来存放从服务器返回的信息。
};

static void netbuf_free(netbuf *b)
{
#define CLEAN(x) free(b -> x)
	CLEAN(buf);
	CLEAN(user);
	CLEAN(passwd);
	CLEAN(host);
	CLEAN(root_dir);
#undef CLEAN
}

static char *version = "ftplib Release 3.1-1 9/16/00, copyright 1996-2000 Thomas Pfau";

int ftplib_debug = 0;

//对连接的读写和关闭可以使用文件的读写和关闭
#define net_read read
#define net_write write
#define net_close close

void *memccpy(void *dest, const void *src, int c, size_t n)
{
    int i=0;
    const unsigned char *ip=src;
    unsigned char *op=dest;

    while(i < n)
    {
		if((*op++ = *ip++) == c)
	    	break;
		i++;
    }
    if(i == n)
		return NULL;
    return op;
}

/*
 * strdup - return a malloc'ed copy of a string
 */
char *strdup(const char *src)
{
  	int len = strlen(src) + 1;
  	char *dst = malloc(len);
  	if(dst)
  	{
	 	strcpy(dst, src);
		dst[len] = '\0';
  	}
  	return dst;
}

/**
 * 从src中拷贝n个字符到dest中。
 * 若成成功则返回dest，否则NULL 。 
 */
static char *strcpyn(char *dest, const char *src, int n)
{
	if (NULL == dest || NULL == src)
	{
		return NULL;
	}

	while (--n >= 0)
	{
		*dest = *src;
		++dest;
		++src;
	}

	return dest - n;
}

/*
 * socket_wait - wait for socket to receive or flush data
 *
 * return 1 if no user callback, otherwise, return value returned by
 * user callback
 */
static int socket_wait(netbuf * ctl)
{
  	fd_set fd, *rfd = NULL, *wfd = NULL;
	struct timeval tv;
  	int rv = 0;
  	if((ctl->dir == FTPLIB_CONTROL) ||(ctl->idlecb == NULL))
    	return 1;
  	if(ctl->dir == FTPLIB_WRITE)
    	wfd = &fd;
  	else
    	rfd = &fd;
  	FD_ZERO(&fd);
  	do
    {
      	FD_SET(ctl->handle, &fd);
      	tv = ctl->idletime;
      	rv = select(ctl->handle + 1, rfd, wfd, NULL, &tv);
      	if(rv == -1)
		{
	  		rv = 0;
	  		strncpy(ctl->ctrl->response, strerror(errno),
		   	sizeof(ctl->ctrl->response));
	  		break;
		}
      	else if(rv > 0)
		{
	  		rv = 1;
	  		break;
		}
    }
  	while((rv = ctl->idlecb(ctl, ctl->xfered, ctl->idlearg)));
  	return rv;
}
/*
 * read a line of text
 *
 * return -1 on error or bytecount
 */
static int read_line(char *buf, int max, netbuf * ctl)
{
  	int x, retval = 0;
  	char *end, *bp = buf;
  	int eof = 0;

  	if((ctl->dir != FTPLIB_CONTROL) &&(ctl->dir != FTPLIB_READ))
    	return -1;
  	if(max == 0)
    	return 0;
  	do
    {
      	if(ctl->cavail > 0)
		{
	  		x =(max >= ctl->cavail) ? ctl->cavail : max - 1;
	  		end = memccpy(bp, ctl->cget, '\n', x);
	  		if(end != NULL)
	    		x = end - bp;
	  		retval += x;
	  		bp += x;
	  		*bp = '\0';
	  		max -= x;
	  		ctl->cget += x;
	  		ctl->cavail -= x;
	  		if(end != NULL)
	    	{
	      		bp -= 2;
	      		if(strcmp(bp, "\r\n") == 0)
				{
		  			*bp++ = '\n';
		  			*bp++ = '\0';
		  			--retval;
				}
	      		break;
	    	}
		}
      	if(max == 1)
		{
	  		*buf = '\0';
	  		break;
		}
      	if(ctl->cput == ctl->cget)
		{
	  		ctl->cput = ctl->cget = ctl->buf;
	  		ctl->cavail = 0;
	  		ctl->cleft = FTPLIB_BUFSIZ;
		}
      	if(eof)
		{
	  		if(retval == 0)
	    		retval = -1;
	  		break;
		}

      	if(!socket_wait(ctl))
			return retval;
      	if((x = net_read(ctl->handle, ctl->cput, ctl->cleft)) == -1)
		{
	 	 	log_err("read");
	  		retval = -1;
	  		break;
		}
		
      	if(x == 0)
			eof = 1;
      	ctl->cleft -= x;
      	ctl->cavail += x;
      	ctl->cput += x;
    }while(1);

  	return retval;
}

/*
 * write lines of text
 *
 * return -1 on error or bytecount
 */
static int write_line(char *buf, int len, netbuf * n_data)
{
  int x, nb = 0, w;
  char *ubp = buf, *nbp;
  char lc = 0;

  if(n_data->dir != FTPLIB_WRITE)
    return -1;
  nbp = n_data->buf;
  for(x = 0; x < len; x++)
    {
      if((*ubp == '\n') &&(lc != '\r'))
	{
	  if(nb == FTPLIB_BUFSIZ)
	    {
	      if(!socket_wait(n_data))
		return x;
	      w = net_write(n_data->handle, nbp, FTPLIB_BUFSIZ);
	      if(w != FTPLIB_BUFSIZ)
		{
		  printf("net_write(1) returned %d, errno = %d\n", w, errno);
		  return(-1);
		}
	      nb = 0;
	    }
	  nbp[nb++] = '\r';
	}
      if(nb == FTPLIB_BUFSIZ)
	{
	  if(!socket_wait(n_data))
	    return x;
	  w = net_write(n_data->handle, nbp, FTPLIB_BUFSIZ);
	  if(w != FTPLIB_BUFSIZ)
	    {
	      printf("net_write(2) returned %d, errno = %d\n", w, errno);
	      return(-1);
	    }
	  nb = 0;
	}
      nbp[nb++] = lc = *ubp++;
    }
  if(nb)
    {
      if(!socket_wait(n_data))
	return x;
      w = net_write(n_data->handle, nbp, nb);
      if(w != nb)
	{
	  printf("net_write(3) returned %d, errno = %d\n", w, errno);
	  return(-1);
	}
    }
  return len;
}

/*
 * read a response from the server
 * 从服务器读取返回的信息.
 * return 0 if first char doesn't match
 * return 1 if first char matches
 */
static int readresp(char c, netbuf * n_control)
{
  	char match[5];

	//从服务器读取数据
  	if(read_line(n_control->response, 256, n_control) == -1)
    {
      	log_err("Control socket read failed");
      	return 0;
    }
  	if(ftplib_debug > 1)
    	fprintf(stderr, "%s", n_control->response);
  	
	if(n_control->response[3] == '-')
    {
      	strncpy(match, n_control->response, 3);
      	match[3] = ' ';
      	match[4] = '\0';
      	do
		{
	  		if(read_line(n_control->response, 256, n_control) == -1)
	    	{
	      		log_err("Control socket read failed");
	      		return 0;
	    	}
	  		if(ftplib_debug > 1)
	    		fprintf(stderr, "%s", n_control->response);
		}while(strncmp(n_control->response, match, 4));
    }

  	if(n_control->response[0] == c)
    	return 1;
  	return 0;
}


/*
 * ftp_LastResponse - return a pointer to the last response received
 */
char * ftp_last_response(netbuf * n_control)
{
  	if((n_control) &&(n_control->dir == FTPLIB_CONTROL))
    	return n_control->response;
  	return NULL;
}

/**
 * 解析用户输入的主机地址。
 * 主机地址可能含有用户名密码和端口号，以及起始目录。
 * 如: hcy:123456@192.168.1.1:21/hcy/home/ftp
 *
 * 解析的结果存储在ctrl中的
 * root_dir 	: 起始目录
 * user 	: 用户名
 * passwd 	: 密码
 * host 	: 主机地址
 * port 	: 端口号
 */
static  int ftp_parse_host(const char *host, netbuf *ctrl)
{
	if (NULL == host || NULL == ctrl)
	{
		log_err("ftp_parse_host: host NULL, or ctrl NULL. %s %d", __FILE__, __LINE__);
		return -1;
	}

	int begin = 0, end = 0;
	int len = 0;
	int ndx = 0;
	
	for (; host[begin] != '\0' && host[begin] != '@'; ++begin);
	
	if (host[begin] == '\0') //没有设置用户名和密码
	{
		begin = 0;
	}
	else //提取用户名和密码
	{
		ndx = -1;
		while (host[++ndx] != ':');
		ctrl -> user = (char *)calloc(1, sizeof(char) * (ndx + 1));
		if (ctrl -> user == NULL)
		{
			log_err("ftp_parse_host: ctrl -> user NULL. %s %d", __FILE__, __LINE__);
			return -1;
		}
		strcpyn(ctrl -> user, host, ndx);
		log_info("ftp_parse_host: user %s", ctrl -> user);

		ctrl -> passwd = (char *)calloc(1, sizeof(char) * (begin - ndx));
		if (ctrl -> passwd == NULL)
		{
			log_err("ftp_parse_host: ctrl -> passwd NULL. %s %d", __FILE__, __LINE__);
			return -1;
		}
		strcpyn(ctrl -> passwd, host + ndx + 1, begin - ndx - 1);
	}

	//提取主机名
	end = (begin == 0 ? 0 : begin + 1);
	while (host[++begin] != ':' && host[begin] != '/' && host[begin] != '\0');
	ctrl -> host = (char *)calloc(1, sizeof(char) * (begin - end + 1));
	if (NULL == ctrl -> host)
	{
		log_err("ftp_parse_host: ctrl -> host NULL. %s %d", __FILE__, __LINE__);
		return -1;
	}
	strcpyn(ctrl -> host, host + end, begin - end);

	//提取端口号
	end = begin + 1;
	if (host[begin] == ':')
	{
		while (host[++begin] != '/' && host[begin] != '\0');
		
	}

	//提取起始地址
	end = begin;
	if (host[begin] == '/')
	{
		begin = strlen(host);
		ctrl -> root_dir = (char *)calloc(1, sizeof(char) * (begin - end + 1));
		if (NULL == ctrl -> root_dir)
		{
			log_err("ftp_parse_host: ctrl -> root_dir NULL. %s %d", __FILE__, __LINE__);
			return -1;
		}
		strcpyn(ctrl -> root_dir, host + end, begin - end);
	}

	return 1;
}

/*
 * 初始化
 */
static netbuf* ftp_init(const char *host)
{
	//所有的数据被初始化为0
  	netbuf *ctrl = calloc(1, sizeof(netbuf));
  	if(ctrl == NULL)
    {
		log_err("ftp_init: calloc error. %s %d", __FILE__, __LINE__);
      	return NULL;
    }
  	ctrl->buf = malloc(FTPLIB_BUFSIZ);
  	if(ctrl->buf == NULL)
    {
		log_err("ftp_init: calloc error. %s %d", __FILE__, __LINE__);
      	netbuf_free(ctrl);
      	return NULL;
    }
	
	//对控制信息进行初始化
  	ctrl->dir = FTPLIB_CONTROL;
  	ctrl->ctrl = NULL;
  	ctrl->cmode = FTPLIB_DEFMODE;
  	ctrl->idlecb = NULL;
  	ctrl->idletime.tv_sec = ctrl->idletime.tv_usec = 0;
  	ctrl->idlearg = NULL;
  	ctrl->xfered = 0;
  	ctrl->xfered1 = 0;
  	ctrl->cbbytes = 0;

	if (host != NULL && ftp_parse_host(host, ctrl) < 0)
	{
		return NULL;
	}
	printf("user: %s\npasswd: %s\nhost: %s\nroot_dir: %s\n"
			, ctrl -> user == NULL ? "NULL" : ctrl -> user 
			, ctrl -> passwd == NULL ? "NULL" : ctrl -> passwd
			, ctrl -> host == NULL ? "NULL" : ctrl -> host
			, ctrl -> root_dir == NULL ? "NULL" : ctrl -> root_dir
			);

	return ctrl;

}
/*
 * ftp_Connect - connect to remote server
 *
 * return 1 if connected, 0 if not
 */
int ftp_connect(const char *host, netbuf ** n_control)
{

  	int s_control;
  	struct sockaddr_in sin;
  	struct hostent *phe;
  	struct servent *pse;
  	int on = 1; //设置重用bind中的地址。
  	
	netbuf *ctrl = ftp_init(host);
	if (NULL == ctrl)
	{
		return 0;
	}

  	char *lhost;
 	char *pnum;

  	memset(&sin, 0, sizeof(sin));
  	sin.sin_family = AF_INET;
  	lhost = strdup(ctrl -> host);
	//确定可能存在的端口号的位置。如:192.168.1.1:21
  	pnum = strchr(lhost, ':');
	
	//没有设置端口号，使用默认的端口号。
  	if(pnum == NULL)
    {
		/*
		 * getservbyname函数通过服务名和协议查询端口号。
		 * pse为servent类型。
		 * struct servent
		 * {
		 * 		char *s_name; 		//服务名
		 * 		char **s_aliases; 	//服务的别名
		 * 		int  s_port;  		//端口号
		 * 		char *s_prot; 		//使用的协议的名称。
		 * 		...
		 * }
		 */
    	if((pse = getservbyname("ftp", "tcp")) == NULL)
		{
	  		log_err("getservbyname");
	  		return 0;
		}
      	sin.sin_port = pse->s_port;
    }
  	else
    {
		//获取地址中的端口号。
      	*pnum++ = '\0'; //将冒号:替换成0
      	if(isdigit(*pnum))
		{
			/*
			 * htons函数，将参数转换成网络字节序的整型数.
			 */
			sin.sin_port = htons((short)atoi(pnum));
		}
		else
		{
			/*
			 * 冒号后面指定的不是端口号而是服务名称。
			 * 通过服务名称获得端口号。
			 */
	  		pse = getservbyname(pnum, "tcp");
	  		sin.sin_port = pse->s_port;
		}
    }
	
	/*
	 * 函数inet_addr将点分十进制的字符串网络地址转换成网络字节序的二进制地址。
	 */
  	if((sin.sin_addr.s_addr = inet_addr(lhost)) == -1)
    {
		/*
		 * gethostbyname函数返回lhost的信息，存放在hostent结构体中。
		 * struct hostent
		 * { 
		 * 		char *h_name; 			//host名称
		 * 		char **h_aliases; 		//别名
		 * 		int h_addrtype;  		//地址类型
		 * 		int h_length; 			//地址的长度（byte数）
		 * 		char **h_addr_list; 	//地址列表。
		 * }
		 * #define h_addr h_addr_list[0]
		 * h_addr执行地址列表中的第一个地址。
		 */
      	if((phe = gethostbyname(lhost)) == NULL)
		{
	  		log_err("gethostbyname");
	  		return 0;
		}
      	memcpy((char *)&sin.sin_addr, phe -> h_addr, phe -> h_length);
    }

  	free(lhost);
  	
	/*
	 * 建立一个套接字。
	 * int socket(int domain, int type, int protocol)
	 * domain : 域，确定通信的特性。
	 * type   : 套接字类型。
	 * protocol : 协力类型，通常为0
	 *
	 * domain可以设置的值：
	 *  AF_INET   : IPv4因特网域
	 *  AF_INET6  : IPv6因特网域
	 *  AF_UNIX   : UNIX域
	 *  AF_UNSPEC : 未指定
	 *
	 * type可以设置的值：
	 *  SOCK_DGRAM 		: 长度固定，无连接的不可靠报文传递，也就是UDP。
	 *  SOCK_RAW 		: IP协议的数据报接口（POSIC.1中为可选）
	 *  SOCK_SEQPACKET 	: 长度固定，有序，可靠的面向连接报文传递
	 *  SOCK_STREAM 	: 有序，可靠，双向的面向连接的字节流，也就是TCP
	 */
	/*
	 * 建立一个TCP Socket。
	 */
	log_info("create a socket...");
	s_control = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	ctrl -> handle = s_control;
  	
	if(s_control == -1)
    {
      	log_err("socket");
      	return 0;
    }
	/*
	 * 设置重用bind中的地址。
	 */
  	if(setsockopt(s_control, SOL_SOCKET, SO_REUSEADDR,(void *)&on, sizeof(on)) == -1)
    {
      	log_err("setsockopt");
      	net_close(s_control);
      	return 0;
    }

	log_info("connect to %s...", ctrl -> host);
	//建立连接
  	if(connect(s_control,(struct sockaddr *)&sin, sizeof(sin)) == -1)
    {
      	log_err("connect");
      	net_close(s_control);
      	return 0;
    }
	log_info("server info: %s", ctrl -> response);

	//读取连接信息
  	if(readresp('2', ctrl) == 0)
    {
      	net_close(s_control);
      	netbuf_free(ctrl);
      	return 0;
    }
	log_info("server info: %s", ctrl -> response);

	//login
	if (NULL != ctrl -> user && NULL != ctrl -> passwd)
	{
		log_info("login...");
		if (ftp_login(ctrl -> user, ctrl -> passwd, ctrl) == 0)
		{
			log_err("ftp_connect: login error. %s %d", __FILE__, __LINE__);
			net_close(s_control);
			netbuf_free(ctrl);
			return 0;
		}
		log_info("server info: %s", ctrl -> response);
	}
	
	//切换目录
	if (ftp_chdir(ctrl -> root_dir, ctrl) <= 0)
	{
		log_err("ftp_connect: chdir error. %s %d", __FILE__, __LINE__);
	}
	log_info("server info: %s", ctrl -> response);

  	*n_control = ctrl;

	return 1;

}

/*
 * change connection options
 *
 * returns 1 if successful, 0 on error
 */
int ftp_options(int opt, long val, netbuf * n_control)
{
  	int v, rv = 0;
  	switch(opt)
    {
    	case FTPLIB_CONNMODE:
      		v =(int) val;
      		if((v == FTPLIB_PASSIVE) ||(v == FTPLIB_PORT))
			{
	  			n_control->cmode = v;
	  			rv = 1;
			}
      		break;
    	case FTPLIB_CALLBACK:
      		n_control->idlecb =(ftp_callback) val;
      		rv = 1;
      		break;
    	case FTPLIB_IDLETIME:
      		v =(int) val;
      		rv = 1;
      		n_control->idletime.tv_sec = v / 1000;
      		n_control->idletime.tv_usec =(v % 1000) * 1000;
      		break;
    	case FTPLIB_CALLBACKARG:
      		rv = 1;
      		n_control->idlearg =(void *) val;
      		break;
    	case FTPLIB_CALLBACKBYTES:
      		rv = 1;
      		n_control->cbbytes =(int) val;
      		break;
    }
  	return rv;
}

/*
 * send a command and wait for expected response
 *
 * return 1 if proper response received, 0 otherwise
 */
static int ftp_send_cmd(const char *cmd, char expresp, netbuf * n_control)
{
  	char buf[256];
 	if(n_control->dir != FTPLIB_CONTROL)
    	return 0;
  	if(ftplib_debug > 2)
    	fprintf(stderr, "%s\n", cmd);
  	if((strlen(cmd) + 3) > sizeof(buf))
    	return 0;
  	sprintf(buf, "%s\r\n", cmd);
  	if(net_write(n_control->handle, buf, strlen(buf)) <= 0)
    {
      	log_err("write");
      	return 0;
    }
  	return readresp(expresp, n_control);
}

/*
 * log in to remote server
 *
 * return 1 if logged in, 0 otherwise
 */
int ftp_login(const char *user, const char *pass, netbuf * n_control)
{
  	char tempbuf[64];

  	if(((strlen(user) + 7) > sizeof(tempbuf)) ||
      			((strlen(pass) + 7) > sizeof(tempbuf)))
	{
		return 0;
	}
 	sprintf(tempbuf, "USER %s", user);
  	if(!ftp_send_cmd(tempbuf, '3', n_control))
    {
      	if(n_control->response[0] == '2')
			return 1;
      	return 0;
    }
  	sprintf(tempbuf, "PASS %s", pass);
  	return ftp_send_cmd(tempbuf, '2', n_control);
}

/*
 * set up data connection
 *
 * return 1 if successful, 0 otherwise
 */
static int ftp_open_port(netbuf * n_control, netbuf ** n_data, int mode, int dir)
{
  	int sData;
  	union
  	{
    	struct sockaddr sa;
    	struct sockaddr_in in;
  	}sin;
  	struct linger lng = { 0, 0 };
  	unsigned int l;
  	int on = 1;
  	netbuf *ctrl;
  	char *cp;
  	unsigned int v[6];
  	char buf[256];

  	if(n_control->dir != FTPLIB_CONTROL)
    	return -1;
  	if((dir != FTPLIB_READ) &&(dir != FTPLIB_WRITE))
    {
      	sprintf(n_control->response, "Invalid direction %d\n", dir);
      	return -1;
    }
  	if((mode != FTPLIB_ASCII) &&(mode != FTPLIB_IMAGE))
    {
      	sprintf(n_control->response, "Invalid mode %c\n", mode);
      	return -1;
    }
  	l = sizeof(sin);
  	if(n_control->cmode == FTPLIB_PASSIVE)
    {
      	memset(&sin, 0, l);
      	sin.in.sin_family = AF_INET;
      	if(!ftp_send_cmd("PASV", '2', n_control))
			return -1;
      	cp = strchr(n_control->response, '(');
      	if(cp == NULL)
			return -1;
      	cp++;
      	sscanf(cp, "%u,%u,%u,%u,%u,%u", &v[2], &v[3], &v[4], &v[5], &v[0], &v[1]);
      	sin.sa.sa_data[2] = v[2];
      	sin.sa.sa_data[3] = v[3];
      	sin.sa.sa_data[4] = v[4];
      	sin.sa.sa_data[5] = v[5];
      	sin.sa.sa_data[0] = v[0];
      	sin.sa.sa_data[1] = v[1];
    }
  	else
    {
      	if(getsockname(n_control->handle, &sin.sa, &l) < 0)
		{
	  		log_err("getsockname");
	  		return 0;
		}
    }
  	sData = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  	if(sData == -1)
    {
      	log_err("socket");
      	return -1;
    }
  	if(setsockopt(sData, SOL_SOCKET, SO_REUSEADDR,(void *)& on, sizeof(on)) == -1)
    {
      	log_err("setsockopt");
      	net_close(sData);
      	return -1;
    }
  	if(setsockopt(sData, SOL_SOCKET, SO_LINGER,(void *)&lng, sizeof(lng)) == -1)
    {
      	log_err("setsockopt");
      	net_close(sData);
      	return -1;
    }
  	if(n_control->cmode == FTPLIB_PASSIVE)
    {
      	if(connect(sData, &sin.sa, sizeof(sin.sa)) == -1)
		{
	  		log_err("connect");
	  		net_close(sData);
	  		return -1;
		}
    }
  	else
    {
     	 sin.in.sin_port = 0;
      	if(bind(sData, &sin.sa, sizeof(sin)) == -1)
		{
	  		log_err("bind");
	  		net_close(sData);
	  		return 0;
		}
      	if(listen(sData, 1) < 0)
		{
			log_err("listen");
	  		net_close(sData);
	  		return 0;
		}
      	if(getsockname(sData, &sin.sa, &l) < 0)
			return 0;
      	sprintf(buf, "PORT %d,%d,%d,%d,%d,%d",
	      (unsigned char) sin.sa.sa_data[2],
	      (unsigned char) sin.sa.sa_data[3],
	      (unsigned char) sin.sa.sa_data[4],
	      (unsigned char) sin.sa.sa_data[5],
	      (unsigned char) sin.sa.sa_data[0],
	      (unsigned char) sin.sa.sa_data[1]);
      	if(!ftp_send_cmd(buf, '2', n_control))
		{
	  		net_close(sData);
	  		return 0;
		}
    }
  	ctrl = calloc(1, sizeof(netbuf));
  	if(ctrl == NULL)
    {
      	log_err("calloc");
      	net_close(sData);
      	return -1;
    }
  	if((mode == 'A') &&((ctrl->buf = malloc(FTPLIB_BUFSIZ)) == NULL))
    {
      	log_err("calloc");
      	net_close(sData);
     	netbuf_free(ctrl);
      	return -1;
    }
  	ctrl->handle = sData;
  	ctrl->dir = dir;
  	ctrl->idletime = n_control->idletime;
  	ctrl->idlearg = n_control->idlearg;
  	ctrl->xfered = 0;
  	ctrl->xfered1 = 0;
  	ctrl->cbbytes = n_control->cbbytes;
  	if(ctrl->idletime.tv_sec || ctrl->idletime.tv_usec || ctrl->cbbytes)
    	ctrl->idlecb = n_control->idlecb;
  	else
    	ctrl->idlecb = NULL;
  	*n_data = ctrl;
  	return 1;
}

/*
 * ftp_AcceptConnection - accept connection from server
 *
 * return 1 if successful, 0 otherwise
 */
static int ftp_accept_connection(netbuf * n_data, netbuf * n_control)
{
  int sData;
  struct sockaddr addr;
  unsigned int l;
  int i;
  struct timeval tv;
  fd_set mask;
  int rv;

  FD_ZERO(&mask);
  FD_SET(n_control->handle, &mask);
  FD_SET(n_data->handle, &mask);
  tv.tv_usec = 0;
  tv.tv_sec = ACCEPT_TIMEOUT;
  i = n_control->handle;
  if(i < n_data->handle)
    i = n_data->handle;
  i = select(i + 1, &mask, NULL, NULL, &tv);
  if(i == -1)
    {
      strncpy(n_control->response, strerror(errno),
	       sizeof(n_control->response));
      net_close(n_data->handle);
      n_data->handle = 0;
      rv = 0;
    }
  else if(i == 0)
    {
      strcpy(n_control->response, "timed out waiting for connection");
      net_close(n_data->handle);
      n_data->handle = 0;
      rv = 0;
    }
  else
    {
      if(FD_ISSET(n_data->handle, &mask))
	{
	  l = sizeof(addr);
	  sData = accept(n_data->handle, &addr, &l);
	  i = errno;
	  net_close(n_data->handle);
	  if(sData > 0)
	    {
	      rv = 1;
	      n_data->handle = sData;
	    }
	  else
	    {
	      strncpy(n_control->response, strerror(i),
		       sizeof(n_control->response));
	      n_data->handle = 0;
	      rv = 0;
	    }
	}
      else if(FD_ISSET(n_control->handle, &mask))
	{
	  net_close(n_data->handle);
	  n_data->handle = 0;
	  readresp('2', n_control);
	  rv = 0;
	}
    }
  return rv;
}
/*
 * ftp_access - return a handle for a data stream
 *
 * return 1 if successful, 0 otherwise
 */
int ftp_access(const char *path, int typ, int mode, netbuf * n_control, netbuf ** n_data)
{
  	char buf[256];
  	int dir;
  	if((path == NULL) &&((typ == FTPLIB_FILE_WRITE) ||(typ == FTPLIB_FILE_READ)))
    {
      	sprintf(n_control->response, "Missing path argument for file transfer\n");
      	return 0;
    }
  	sprintf(buf, "TYPE %c", mode);
  	if(!ftp_send_cmd(buf, '2', n_control))
    	return 0;
  	switch(typ)
    {
    	case FTPLIB_DIR:
      		strcpy(buf, "NLST");
      		dir = FTPLIB_READ;
      		break;
    	case FTPLIB_DIR_VERBOSE:
      		strcpy(buf, "LIST");
      		dir = FTPLIB_READ;
      		break;
    	case FTPLIB_FILE_READ:
      		strcpy(buf, "RETR");
      		dir = FTPLIB_READ;
      		break;
    	case FTPLIB_FILE_WRITE:
      		strcpy(buf, "STOR");
      		dir = FTPLIB_WRITE;
      		break;
    	default:
      		sprintf(n_control->response, "Invalid open type %d\n", typ);
      		return 0;
    }
	
  	if(path != NULL)
    {
      	int i = strlen(buf);
      	buf[i++] = ' ';
      	if((strlen(path) + i) >= sizeof(buf))
			return 0;
      	strcpy(&buf[i], path);
    }
	
  	if(ftp_open_port(n_control, n_data, mode, dir) == -1)
    	return 0;
  
	if(!ftp_send_cmd(buf, '1', n_control))
    {
      	ftp_data_close(*n_data);
      	*n_data = NULL;
      	return 0;
    }
 	
	(*n_data)->ctrl = n_control;
  	n_control->data = *n_data;
  	if(n_control->cmode == FTPLIB_PORT)
    {
      	if(!ftp_accept_connection(*n_data, n_control))
		{
	  		ftp_data_close(*n_data);
	  		*n_data = NULL;
	  		n_control->data = NULL;
	  		return 0;
		}
    }
  
	return 1;
}

/*
 * ftp_Read - read from a data connection
 */
int ftp_data_read(void *buf, int max, netbuf * n_data)
{
  int i;
  if(n_data->dir != FTPLIB_READ)
    return 0;
  if(n_data->buf)
    i = read_line(buf, max, n_data);
  else
    {
      i = socket_wait(n_data);
      if(i != 1)
	return 0;
      i = net_read(n_data->handle, buf, max);
    }
  if(i == -1)
    return 0;
  n_data->xfered += i;
  if(n_data->idlecb && n_data->cbbytes)
    {
      n_data->xfered1 += i;
      if(n_data->xfered1 > n_data->cbbytes)
	{
	  if(n_data->idlecb(n_data, n_data->xfered, n_data->idlearg) == 0)
	    return 0;
	  n_data->xfered1 = 0;
	}
    }
  return i;
}

/*
 * ftp_Write - write to a data connection
 */
int ftp_data_write(void *buf, int len, netbuf * n_data)
{
  int i;
  if(n_data->dir != FTPLIB_WRITE)
    return 0;
  if(n_data->buf)
    i = write_line(buf, len, n_data);
  else
    {
      socket_wait(n_data);
      i = net_write(n_data->handle, buf, len);
    }
  if(i == -1)
    return 0;
  n_data->xfered += i;
  if(n_data->idlecb && n_data->cbbytes)
    {
      n_data->xfered1 += i;
      if(n_data->xfered1 > n_data->cbbytes)
	{
	  n_data->idlecb(n_data, n_data->xfered, n_data->idlearg);
	  n_data->xfered1 = 0;
	}
    }
  return i;
}

/*
 * ftp_Close - close a data connection
 */
int ftp_data_close(netbuf * n_data)
{
  netbuf *ctrl;
  switch(n_data->dir)
    {
    case FTPLIB_WRITE:
      /* potential problem - if buffer flush fails, how to notify user? */
      if(n_data->buf != NULL)
	write_line(NULL, 0, n_data);
    case FTPLIB_READ:
      if(n_data->buf)
	free(n_data->buf);
      shutdown(n_data->handle, 2);
      net_close(n_data->handle);
      ctrl = n_data->ctrl;
      free(n_data);
      if(ctrl)
	{
	  ctrl->data = NULL;
	  return(readresp('2', ctrl));
	}
      return 1;
    case FTPLIB_CONTROL:
      if(n_data->data)
	{
	  n_data->ctrl = NULL;
	  ftp_data_close(n_data);
	}
      net_close(n_data->handle);
      free(n_data);
      return 0;
    }
  return 1;
}

/*
 * ftp_Site - send a SITE command
 *
 * return 1 if command successful, 0 otherwise
 */
int ftp_site(const char *cmd, netbuf * n_control)
{
  char buf[256];

  if((strlen(cmd) + 7) > sizeof(buf))
    return 0;
  sprintf(buf, "SITE %s", cmd);
  if(!ftp_send_cmd(buf, '2', n_control))
    return 0;
  return 1;
}

/*
 * ftp_SysType - send a SYST command
 *
 * Fills in the user buffer with the remote system type.  If more
 * information from the response is required, the user can parse
 * it out of the response buffer returned by ftp_LastResponse().
 *
 * return 1 if command successful, 0 otherwise
 */
int ftp_systype(char *buf, int max, netbuf * n_control)
{
  int l = max;
  char *b = buf;
  char *s;
  if(!ftp_send_cmd("SYST", '2', n_control))
    return 0;
  s = &n_control->response[4];
  while((--l) &&(*s != ' '))
    *b++ = *s++;
  *b++ = '\0';
  return 1;
}

/*
 * ftp_Mkdir - create a directory at server
 *
 * return 1 if successful, 0 otherwise
 */
int ftp_mkdir(const char *path, netbuf * n_control)
{
  char buf[256];

  if((strlen(path) + 6) > sizeof(buf))
    return 0;
  sprintf(buf, "MKD %s", path);
  if(!ftp_send_cmd(buf, '2', n_control))
    return 0;
  return 1;
}

/*
 * ftp_Chdir - change path at remote
 *
 * return 1 if successful, 0 otherwise
 */
int ftp_chdir(const char *path, netbuf * n_control)
{
  	char buf[256];

  	if((strlen(path) + 6) > sizeof(buf))
    	return 0;
  	sprintf(buf, "CWD %s", path);
  	if(!ftp_send_cmd(buf, '2', n_control))
    	return 0;
  	return 1;
}

/*
 * ftp_cdup - move to parent directory at remote
 *
 * return 1 if successful, 0 otherwise
 */
int ftp_cdup(netbuf * n_control)
{
  if(!ftp_send_cmd("CDUP", '2', n_control))
    return 0;
  return 1;
}

/*
 * ftp_Rmdir - remove directory at remote
 *
 * return 1 if successful, 0 otherwise
 */
int ftp_rmdir(const char *path, netbuf * n_control)
{
  char buf[256];

  if((strlen(path) + 6) > sizeof(buf))
    return 0;
  sprintf(buf, "RMD %s", path);
  if(!ftp_send_cmd(buf, '2', n_control))
    return 0;
  return 1;
}

/*
 * ftp_Pwd - get working directory at remote
 *
 * return 1 if successful, 0 otherwise
 */
int ftp_pwd(char *path, int max, netbuf * n_control)
{
  	int l = max;
  	char *b = path;
  	char *s;
  	if(!ftp_send_cmd("PWD", '2', n_control))
    	return 0;
  	s = strchr(n_control->response, '"');
  	if(s == NULL)
    	return 0;
  	s++;
  	while((--l) &&(*s) &&(*s != '"'))
    	*b++ = *s++;
  	*b++ = '\0';
  	return 1;
}

/*
 * ftp_xfer - 运行一个命令并传输数据。
 *
 * @parm localfile
 * 		 本地文件名。如从服务器获取文件时，本地保存的文件名，或上传到服务器的本地文件的文件名。
 * 		 对于一些获取信息的命令，如获取文件列表，这个文件名指定了获取的信息存放的文件名。
 * 		 如果为NULL，则设置为标准输出或标准输入
 * @parm path
 * 	 	 运行的命令所对应的服务器中的文件路径。
 * @parm n_control
 *  	 连接控制信息。
 * @parm typ
 *  	 所要运行的命令的类型。
 * @parm mode
 *  	 对于一些传输的命令，用于区分是按ASCII传输还是按流传输。
 *
 * return 1 if successful, 0 otherwise
 */
static int ftp_xfer(const char *localfile, const char *path,
	 netbuf * n_control, int typ, int mode)
{
	int l, c;
  	char *dbuf;
  	FILE *local = NULL;
  	netbuf *n_data;
  	int rv = 1;

  	if(localfile != NULL)
    {
      	char ac[4] = "w";
      	if(typ == FTPLIB_FILE_WRITE)
			ac[0] = 'r';
      	if(mode == FTPLIB_IMAGE)
			ac[1] = 'b';
      	local = fopen(localfile, ac);
      	if(local == NULL)
		{
	  		strncpy(n_control->response, strerror(errno), sizeof(n_control->response));
	  		return 0;
		}
    }
  	if(local == NULL)
    	local =(typ == FTPLIB_FILE_WRITE) ? stdin : stdout;

  	if(!ftp_access(path, typ, mode, n_control, &n_data))
    	return 0;
  	
	dbuf = malloc(FTPLIB_BUFSIZ);
  	
	if(typ == FTPLIB_FILE_WRITE) //向服务器发送数据
    {
      	while((l = fread(dbuf, 1, FTPLIB_BUFSIZ, local)) > 0)
		{
			if((c = ftp_data_write(dbuf, l, n_data)) < l)
	  		{
	    		printf("short write: passed %d, wrote %d\n", l, c);
	    		rv = 0;
	    		break;
	  		}
		}
    }
  	else //从服务器读取数据
    {
      	while((l = ftp_data_read(dbuf, FTPLIB_BUFSIZ, n_data)) > 0)
		{
			if(fwrite(dbuf, 1, l, local) <= 0)
	  		{
	    		log_err("localfile write");
	    		rv = 0;
	    		break;
	  		}
		}
    }
  	
	free(dbuf);
  	fflush(local);
  	
	if(localfile != NULL)
    	fclose(local);
 	ftp_data_close(n_data);
  	return rv;
}

/*
 * ftp_dir - issue an NLST command and write response to output
 *
 * return 1 if successful, 0 otherwise
 */
int ftp_dir(const char *outputfile, const char *path, netbuf * n_control)
{
	char p[1000];
	if (ftp_pwd(p, 1000, n_control) <= 0)
	{
		log_err("ftp_dir: get pwd error. %s %d", __FILE__, __LINE__);
		return 0;
	}
	printf("%s\n", p);
  	return ftp_xfer(outputfile, path == NULL ? "." : path
			, n_control, FTPLIB_DIR, FTPLIB_ASCII);
}

/*
 * ftp_ls - issue a LIST command and write response to output
 *
 * return 1 if successful, 0 otherwise
 */
int ftp_ls(const char *outputfile, const char *path, netbuf * n_control)
{
	char p[1000];
	if (ftp_pwd(p, 1000, n_control) <= 0)
	{
		log_err("ftp_dir: get pwd error. %s %d", __FILE__, __LINE__);
		return 0;
	}
	printf("%s\n", p);
  	return ftp_xfer(outputfile, path == NULL ? "." : path
			, n_control, FTPLIB_DIR_VERBOSE, FTPLIB_ASCII);
}

/*
 * ftp_Size - determine the size of a remote file
 *
 * return 1 if successful, 0 otherwise
 */
int ftp_size(const char *path, int *size, char mode, netbuf * n_control)
{
  char cmd[256];
  int resp, sz, rv = 1;

  if((strlen(path) + 7) > sizeof(cmd))
    return 0;
  sprintf(cmd, "TYPE %c", mode);
  if(!ftp_send_cmd(cmd, '2', n_control))
    return 0;
  sprintf(cmd, "SIZE %s", path);
  if(!ftp_send_cmd(cmd, '2', n_control))
    rv = 0;
  else
    {
      if(sscanf(n_control->response, "%d %d", &resp, &sz) == 2)
	*size = sz;
      else
	rv = 0;
    }
  return rv;
}

/*
 * ftp_ModDate - determine the modification date of a remote file
 *
 * return 1 if successful, 0 otherwise
 */
int ftp_moddate(const char *path, char *dt, int max, netbuf * n_control)
{
  char buf[256];
  int rv = 1;

  if((strlen(path) + 7) > sizeof(buf))
    return 0;
  sprintf(buf, "MDTM %s", path);
  if(!ftp_send_cmd(buf, '2', n_control))
    rv = 0;
  else
    strncpy(dt, &n_control->response[4], max);
  return rv;
}

/*
 * ftp_Get - issue a GET command and write received data to output
 *
 * return 1 if successful, 0 otherwise
 */
int ftp_get(const char *outputfile, const char *path, char mode, netbuf * n_control)
{
  	return ftp_xfer(outputfile, path, n_control, FTPLIB_FILE_READ, mode);
}

/*
 * ftp_Put - issue a PUT command and send data from input
 *
 * return 1 if successful, 0 otherwise
 */
int ftp_put(const char *inputfile, const char *path, char mode, netbuf * n_control)
{
  return ftp_xfer(inputfile, path, n_control, FTPLIB_FILE_WRITE, mode);
}

/*
 * ftp_Rename - rename a file at remote
 *
 * return 1 if successful, 0 otherwise
 */
int ftp_rename(const char *src, const char *dst, netbuf * n_control)
{
  char cmd[256];

  if(((strlen(src) + 7) > sizeof(cmd)) ||
     ((strlen(dst) + 7) > sizeof(cmd)))
    return 0;
  sprintf(cmd, "RNFR %s", src);
  if(!ftp_send_cmd(cmd, '3', n_control))
    return 0;
  sprintf(cmd, "RNTO %s", dst);
  if(!ftp_send_cmd(cmd, '2', n_control))
    return 0;
  return 1;
}

/*
 * ftp_Delete - delete a file at remote
 *
 * return 1 if successful, 0 otherwise
 */
int ftp_delete(const char *fnm, netbuf * n_control)
{
  char cmd[256];

  if((strlen(fnm) + 7) > sizeof(cmd))
    return 0;
  sprintf(cmd, "DELE %s", fnm);
  if(!ftp_send_cmd(cmd, '2', n_control))
    return 0;
  return 1;
}

/*
 * ftp_Quit - disconnect from remote
 *
 * return 1 if successful, 0 otherwise
 */
void ftp_quit(netbuf * n_control)
{
  	if(n_control->dir != FTPLIB_CONTROL)
    	return;
  	ftp_send_cmd("QUIT", '2', n_control);
  	net_close(n_control->handle);
  	netbuf_free(n_control);
}
