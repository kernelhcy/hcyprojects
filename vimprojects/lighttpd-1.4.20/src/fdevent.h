#ifndef _FDEVENT_H_
#define _FDEVENT_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "settings.h"
#include "bitset.h"

/*
 * select event-system 
 * 这个多路IO事件系统考虑了多个系统的兼容性。
 * 包括：
 *  	select
 *  	poll
 *  	Linux epoll
 * 		Solaris devpoll
 * 		FreeBSD kqueue
 * 		Solaris port
 * 这里面使用了面向对象的设计思想。
 *
 * 这个系统的设计和array和相似
 */

//下面的宏是根据系统的不同分别包含不同的头文件。

#if defined(HAVE_EPOLL_CTL) && defined(HAVE_SYS_EPOLL_H)
# if defined HAVE_STDINT_H
#  include <stdint.h>
# endif
# define USE_LINUX_EPOLL
# include <sys/epoll.h>
#endif

/*
 * MacOS 10.3.x has poll.h under /usr/include/, all other unixes under
 * /usr/include/sys/ 
 */
#if defined HAVE_POLL && (defined(HAVE_SYS_POLL_H) || defined(HAVE_POLL_H))
# define USE_POLL
# ifdef HAVE_POLL_H
#  include <poll.h>
# else
#  include <sys/poll.h>
# endif
# if defined HAVE_SIGTIMEDWAIT && defined(__linux__)
#  define USE_LINUX_SIGIO
#  include <signal.h>
# endif
#endif

#if defined HAVE_SELECT
# ifdef __WIN32
#  include <winsock2.h>
# endif
# define USE_SELECT
# ifdef HAVE_SYS_SELECT_H
#  include <sys/select.h>
# endif
#endif

#if defined HAVE_SYS_DEVPOLL_H && defined(__sun)
# define USE_SOLARIS_DEVPOLL
# include <sys/devpoll.h>
#endif

#if defined HAVE_SYS_EVENT_H && defined HAVE_KQUEUE
# define USE_FREEBSD_KQUEUE
# include <sys/event.h>
#endif

#if defined HAVE_SYS_PORT_H && defined HAVE_PORT_CREATE
# define USE_SOLARIS_PORT
# include <sys/port.h>
#endif

/**
 * 定义事件处理函数原型
 *
 * 参数及返回值：
 * @return 	handler_t,
 * @srv 	void,
 * @ctx 	viod,
 * @revents int, 	
 */
typedef handler_t(*fdevent_handler) (void *srv, void *ctx, int revents);
/*
 * 用于标记文件描述符的状态
 */
#define FDEVENT_IN     BV(0) 	//文件描述符是否可写 
#define FDEVENT_PRI    BV(1)  	//不阻塞的可读高优先级的数据 poll
#define FDEVENT_OUT    BV(2) 	//文件描述符是否可读
#define FDEVENT_ERR    BV(3) 	//文件描述符是否出错
#define FDEVENT_HUP    BV(4) 	//已挂断 poll
#define FDEVENT_NVAL   BV(5) 	//描述符不引用一打开文件 poll

//定义事件的类型。
typedef enum 
{ 
	FD_EVENT_TYPE_UNSET = -1, 		//未定义类型，设置为-1
	FD_EVENT_TYPE_CONNECTION, 		//普通连接
	FD_EVENT_TYPE_FCGI_CONNECTION, 	//fcgi连接
	FD_EVENT_TYPE_DIRWATCH, 		//监听目录变化
	FD_EVENT_TYPE_CGI_CONNECTION 	//cgi连接
} fd_event_t;

//定义事件处理器的类型
typedef enum 
{ 
	FDEVENT_HANDLER_UNSET, 				//未定义
	FDEVENT_HANDLER_SELECT, 			//select
	FDEVENT_HANDLER_POLL, 				//poll
	FDEVENT_HANDLER_LINUX_RTSIG, 		//rtsig
	FDEVENT_HANDLER_LINUX_SYSEPOLL, 	//sysepoll
	FDEVENT_HANDLER_SOLARIS_DEVPOLL, 	//devpoll
	FDEVENT_HANDLER_FREEBSD_KQUEUE, 	//kqueue
	FDEVENT_HANDLER_SOLARIS_PORT 		//port
} fdevent_handler_t;

/**
 * a mapping from fd to connection structure
 * 文件描述符和连接之间的映射结构体
 */
typedef struct 
{
	int fd; 				//文件描述符
					 		/**< the fd */
	void *conn;				/**< a reference the corresponding data-structure */
							/* 指向相关连接结构体的指针 */
	fd_event_t fd_type;		/* 文件描述符的类型 */
	int events;
							 /**< registered events */
	int revents;
} fd_conn;

//上一个结构体的数组。
typedef struct 
{
	fd_conn *ptr;

	size_t size;
	size_t used;
} fd_conn_buffer;

/**
 * array of unused fd's
 * 文件描述符数组
 */

typedef struct _fdnode 
{
	fdevent_handler handler; //处理函数指针
	void *ctx;   			 //文件描述符的context
	int fd; 				 //文件描述符
	struct _fdnode *prev, *next; //指针
} fdnode;

//用于使用poll函数时，存储数组pollfds中位置索引。
typedef struct 
{
	int *ptr; 		//位置索引数组。

	size_t used; 	//数组中数据个数。
	size_t size; 	//数组长度。
} buffer_int;

/**
 * fd-event handler for select(), poll() and rt-signals on Linux 2.4
 * 这个结构体是重点。
 * 里面使用了很多条件宏，用来根据不同的系统包含不同的变量属性。
 * 后面的函数指针定义了对这些属性的统一的操作接口，
 * 在初始化的时候，根据系统的不同赋予不同的函数地址。
 * 从而实现类似于多态的特性。
 */
typedef struct fdevents 
{
	//共有的属性
	fdevent_handler_t type; //多路IO类型

	fdnode **fdarray; 	//文件描述符数组
	size_t maxfds; 		//最大的文件描述符数

#ifdef USE_LINUX_SIGIO
	int in_sigio;
	int signum;
	sigset_t sigset;
	siginfo_t siginfo;
	bitset *sigbset;
#endif

#ifdef USE_LINUX_EPOLL
	int epoll_fd;
	struct epoll_event *epoll_events;
#endif

//poll
#ifdef USE_POLL
	struct pollfd *pollfds; 	//描述符及其状态的结构体数组

	size_t size; 				//数组中数据的个数
	size_t used; 				//数组的大小

	//用于存储pollfds中为使用的位置。
	//由于可能的删除操作，会是pollfds中存在空档，将这些空档
	//的索引存在unused中，便于下次插入操作时直接使用这些空档
	//减少空间的浪费。
	buffer_int unused; 	
#endif

//select
#ifdef USE_SELECT
	//三个文件描述符集合
	fd_set select_read; 		//可读，对应FDEVENT_IN
	fd_set select_write; 		//可写，对应FDEVENT_OUT
	fd_set select_error; 		//处于异常条件，对应FDEVENT_ERR

	//由于select函数会修改上面的三个集合，
	//因此，在这里保存一个初始的副本。
	fd_set select_set_read;
	fd_set select_set_write;
	fd_set select_set_error;
	
	int select_max_fd; 			//最大的文件描述符数。用于select函数的第一个参数。
#endif

#ifdef USE_SOLARIS_DEVPOLL
	int devpoll_fd;
	struct pollfd *devpollfds;
#endif

#ifdef USE_FREEBSD_KQUEUE
	int kq_fd;
	struct kevent *kq_results;
	bitset *kq_bevents;
#endif

#ifdef USE_SOLARIS_PORT
	int port_fd;
#endif

	//统一的操作接口,与后面的函数声明对应。
	int (*reset) (struct fdevents * ev);
	void (*free) (struct fdevents * ev);
	int (*event_add) (struct fdevents * ev, int fde_ndx, int fd, int events);
	int (*event_del) (struct fdevents * ev, int fde_ndx, int fd);
	int (*event_get_revent) (struct fdevents * ev, size_t ndx);
	int (*event_get_fd) (struct fdevents * ev, size_t ndx);
	int (*event_next_fdndx) (struct fdevents * ev, int ndx);
	int (*poll) (struct fdevents * ev, int timeout_ms);
	int (*fcntl_set) (struct fdevents * ev, int fd);
} fdevents;

fdevents *fdevent_init(size_t maxfds, fdevent_handler_t type);

//对应与上面的结构体中的前八个函数指针。
//这几个函数是fdevent系统的对外接口。

/*
 * 重置和释放fdevent系统。
 */
int fdevent_reset(fdevents * ev);
void fdevent_free(fdevents * ev);
/*
 * 将fd增加到fd event系统中。events是要对fd要监听的事件。
 * fde_ndx是fd对应的fdnode在ev->fdarray中的下标值的指针。
 * 如果fde_ndx==NULL，则表示在fd event系统中增加fd。如果不为NULL，则表示这个
 * fd已经在系统中存在，这个函数的功能就变为将对fd监听的事件变为events。
 */
int fdevent_event_add(fdevents * ev, int *fde_ndx, int fd, int events);
/*
 * 从fd event系统中删除fd。 fde_ndx的内容和上面的一致。
 */
int fdevent_event_del(fdevents * ev, int *fde_ndx, int fd);
/*
 * 返回ndx对应的fd所发生的事件。
 * 这里的ndx和上面的fde_ndx不一样，这个ndx是ev->epoll_events中epoll_event结构体的下标。
 * 第一次调用的时候，通常ndx为-1。
 * 这个ndx和其对应的fd没有关系。而fde_ndx等于其对应的fd。
 */
int fdevent_event_get_revent(fdevents * ev, size_t ndx);
/*
 * 返回ndx对应的fd。
 */
int fdevent_event_get_fd(fdevents * ev, size_t ndx);
/*
 * 返回下一个fd。
 */
int fdevent_event_next_fdndx(fdevents * ev, int ndx);
/*
 * 开始等待IO事件。timeout_ms是超时限制。
 */
int fdevent_poll(fdevents * ev, int timeout_ms);

/*
 * 返回fd对应的事件处理函数地址。
 */
fdevent_handler fdevent_get_handler(fdevents * ev, int fd);
/*
 * 返回fd对应的环境。
 */
void *fdevent_get_context(fdevents * ev, int fd);

/*
 * 注册和取消注册fd。
 * 就是生成一个fdnode，然后保存在ev->fdarray中。或者删除之。
 */
int fdevent_register(fdevents * ev, int fd, fdevent_handler handler, void *ctx);
int fdevent_unregister(fdevents * ev, int fd);


/**
 * 设置fd的状态，通常是设置为运行exec在子进程中关闭和非阻塞。
 */
int fdevent_fcntl_set(fdevents * ev, int fd);


/**
 * 初始化各种多路IO。
 */
int fdevent_select_init(fdevents * ev);
int fdevent_poll_init(fdevents * ev);
int fdevent_linux_rtsig_init(fdevents * ev);
int fdevent_linux_sysepoll_init(fdevents * ev);
int fdevent_solaris_devpoll_init(fdevents * ev);
int fdevent_freebsd_kqueue_init(fdevents * ev);

#endif
