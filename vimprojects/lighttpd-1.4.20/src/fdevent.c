/**
 *
 * 这个文件定义一些通用的操作，包括初始化等。
 * 其中大部分的操作都是调用fdevents结构体中的函数指针。
 * 这样可以忽略到底使用那种多路IO而提供一个统一的操作接口。
 * 便于扩展和使用。
 * 
 * 关于不同的多路IO的各种具体操作，分别包含在文件：
 * 	fdevent_freebsd_kqueue.c  fdevent_linux_sysepoll.c  fdevent_select.c
 * 	fdevent_linux_rtsig.c     fdevent_poll.c            fdevent_solaris_devpoll.c
 * 中。
 * 在这些文件中，通过函数fdevent_XXX_init函数对fdevent结构体进行赋值。
 *
 */

#include <sys/types.h>
#include "settings.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

#include "fdevent.h"
#include "buffer.h"

/**
 * 初始化文件描述符事件数组fdevent
 */
fdevents *fdevent_init(size_t maxfds, fdevent_handler_t type)
{
	fdevents *ev;
	
	//内存被初始化为0
	ev = calloc(1, sizeof(*ev));
	
	//分配数组
	ev->fdarray = calloc(maxfds, sizeof(*ev->fdarray));
	ev->maxfds = maxfds;

	//根据设定的多路IO的类型进行初始化。
	switch (type)
	{
	case FDEVENT_HANDLER_POLL:
		if (0 != fdevent_poll_init(ev))
		{
			fprintf(stderr, "%s.%d: event-handler poll failed\n", __FILE__, __LINE__);
			return NULL;
		}
		break;
	case FDEVENT_HANDLER_SELECT:
		if (0 != fdevent_select_init(ev))
		{
			fprintf(stderr, "%s.%d: event-handler select failed\n", __FILE__, __LINE__);
			return NULL;
		}
		break;
	case FDEVENT_HANDLER_LINUX_RTSIG:
		if (0 != fdevent_linux_rtsig_init(ev))
		{
			fprintf(stderr, "%s.%d: event-handler linux-rtsig failed, try to set server.event-handler = \"poll\" or \"select\"\n",
					__FILE__, __LINE__);
			return NULL;
		}
		break;
	case FDEVENT_HANDLER_LINUX_SYSEPOLL:
		if (0 != fdevent_linux_sysepoll_init(ev))
		{
			fprintf(stderr, "%s.%d: event-handler linux-sysepoll failed, try to set server.event-handler = \"poll\" or \"select\"\n",
					__FILE__, __LINE__);
			return NULL;
		}
		break;
	case FDEVENT_HANDLER_SOLARIS_DEVPOLL:
		if (0 != fdevent_solaris_devpoll_init(ev))
		{
			fprintf(stderr, "%s.%d: event-handler solaris-devpoll failed, try to set server.event-handler = \"poll\" or \"select\"\n",
					__FILE__, __LINE__);
			return NULL;
		}
		break;
	case FDEVENT_HANDLER_FREEBSD_KQUEUE:
		if (0 != fdevent_freebsd_kqueue_init(ev))
		{
			fprintf(stderr, "%s.%d: event-handler freebsd-kqueue failed, try to set server.event-handler = \"poll\" or \"select\"\n",
					__FILE__, __LINE__);
			return NULL;
		}
		break;
	default:
		fprintf(stderr, "%s.%d: event-handler is unknown, try to set server.event-handler = \"poll\" or \"select\"\n",
				__FILE__, __LINE__);
		return NULL;
	}

	return ev;
}

void fdevent_free(fdevents * ev)
{
	size_t i;
	if (!ev)
		return;

	//调用ev中保存的free函数
	if (ev -> free)
		ev -> free(ev);

	for (i = 0; i < ev->maxfds; i++)
	{
		if (ev->fdarray[i])
			free(ev->fdarray[i]);
	}

	free(ev->fdarray);
	free(ev);
}

int fdevent_reset(fdevents * ev)
{
	//调用reset函数
	if (ev->reset)
		return ev->reset(ev);

	return 0;
}

fdnode *fdnode_init()
{
	fdnode *fdn;

	fdn = calloc(1, sizeof(*fdn));
	fdn->fd = -1;
	return fdn;
}

void fdnode_free(fdnode * fdn)
{
	free(fdn);
}
//
//注册文件描述符fd和处理函数handler。
//及ctx
int fdevent_register(fdevents * ev, int fd, fdevent_handler handler, void *ctx)
{
	fdnode *fdn;

	fdn = fdnode_init();
	fdn->handler = handler;
	fdn->fd = fd;
	fdn->ctx = ctx;

	ev->fdarray[fd] = fdn; //使用文件描述符作为数组的下标

	return 0;
}

//删除文件描述符fd
int fdevent_unregister(fdevents * ev, int fd)
{
	fdnode *fdn;
	if (!ev)
		return 0;
	fdn = ev->fdarray[fd];

	fdnode_free(fdn);

	ev->fdarray[fd] = NULL;

	return 0;
}

int fdevent_event_del(fdevents * ev, int *fde_ndx, int fd)
{
	int fde = fde_ndx ? *fde_ndx : -1;

	if (ev->event_del)
		fde = ev->event_del(ev, fde, fd);

	if (fde_ndx)
		*fde_ndx = fde;

	return 0;
}

int fdevent_event_add(fdevents * ev, int *fde_ndx, int fd, int events)
{
	int fde = fde_ndx ? *fde_ndx : -1;

	if (ev->event_add)
		fde = ev->event_add(ev, fde, fd, events);

	if (fde_ndx)
		*fde_ndx = fde;

	return 0;
}

int fdevent_poll(fdevents * ev, int timeout_ms)
{
	if (ev->poll == NULL)
		SEGFAULT();
	return ev->poll(ev, timeout_ms);
}

int fdevent_event_get_revent(fdevents * ev, size_t ndx)
{
	if (ev->event_get_revent == NULL)
		SEGFAULT();

	return ev->event_get_revent(ev, ndx);
}

int fdevent_event_get_fd(fdevents * ev, size_t ndx)
{
	if (ev->event_get_fd == NULL)
		SEGFAULT();

	return ev->event_get_fd(ev, ndx);
}

fdevent_handler fdevent_get_handler(fdevents * ev, int fd)
{
	if (ev->fdarray[fd] == NULL)
		SEGFAULT();
	if (ev->fdarray[fd]->fd != fd)
		SEGFAULT();

	return ev->fdarray[fd]->handler;
}

void *fdevent_get_context(fdevents * ev, int fd)
{
	if (ev->fdarray[fd] == NULL)
		SEGFAULT();
	if (ev->fdarray[fd]->fd != fd)
		SEGFAULT();

	return ev->fdarray[fd]->ctx;
}
/**
 * 设置fd的状态
 */
int fdevent_fcntl_set(fdevents * ev, int fd)
{
#ifdef FD_CLOEXEC
	/*
	 * close fd on exec (cgi) 
	 */
	fcntl(fd, F_SETFD, FD_CLOEXEC);
#endif
	if ((ev) && (ev->fcntl_set))
		return ev->fcntl_set(ev, fd);
#ifdef O_NONBLOCK
	return fcntl(fd, F_SETFL, O_NONBLOCK | O_RDWR);
#else
	return 0;
#endif
}


int fdevent_event_next_fdndx(fdevents * ev, int ndx)
{
	if (ev->event_next_fdndx)
		return ev->event_next_fdndx(ev, ndx);

	return -1;
}
