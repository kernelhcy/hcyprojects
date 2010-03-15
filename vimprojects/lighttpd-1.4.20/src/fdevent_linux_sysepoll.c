#include <sys/types.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>

#include "fdevent.h"
#include "settings.h"
#include "buffer.h"

/**
 * 使用epol作为多路IO的基础。
 */

#ifdef USE_LINUX_EPOLL
static void fdevent_linux_sysepoll_free(fdevents * ev)
{
	close(ev->epoll_fd);
	free(ev->epoll_events);
}

static int fdevent_linux_sysepoll_event_del(fdevents * ev, int fde_ndx, int fd)
{
	/**
	 * struct epoll_event
	 * {
	 * 		__uinit32_t events; //epoll事件标记位,每一位代表一种事件类型。
	 * 		epoll_data_t data;  //用户数据。
	 * }
	 * 
	 * typedef union epoll_data
	 * {
	 * 		void *ptr;
	 * 		int fd;
	 * 		__uint32_t u32;
	 * 		__uint64_t u64;
	 * }epoll_data_t
	 *
	 */
	struct epoll_event ep;

	if (fde_ndx < 0)
		return -1;

	memset(&ep, 0, sizeof(ep));

	ep.data.fd = fd;
	ep.data.ptr = NULL; //ep.data是个union，这样不就把上一句的赋值给覆盖了？？？

	//EPOLL_CTL_DEL : 将fd从ev->epoll_fd中删除，不在对fd进行监听，最后一个参数
	//可以是NULL。将被忽略。
	//注：kernel 2.6.9以前，最后一个参数不可以为NULL，尽管它在函数中被忽略。之后的内核
	//修正了这一问题。
	if (0 != epoll_ctl(ev->epoll_fd, EPOLL_CTL_DEL, fd, &ep))
	{
		fprintf(stderr, "%s.%d: epoll_ctl failed: %s, dying\n", __FILE__, __LINE__, strerror(errno));
		SEGFAULT();
		return 0;
	}


	return -1;
}

static int fdevent_linux_sysepoll_event_add(fdevents * ev, int fde_ndx, int fd, int events)
{
	struct epoll_event ep;
	int add = 0;

	if (fde_ndx == -1) //描述符不在epoll的检测中，增加之。
		add = 1;

	memset(&ep, 0, sizeof(ep));

	ep.events = 0;

	/**
	 * 在ep中设置需要监听的IO事件。
	 * EPOLLIN : 描述符可读。
	 * EPOLLOUT ：描述符可写。
	 * 其他的事件还有：EPOLLRDHUP , EPOLLPRI, EPOLLERR, EPOLLHUP, EPOLLET, EPOLLONESHOT等。
	 */
	if (events & FDEVENT_IN)
		ep.events |= EPOLLIN;
	if (events & FDEVENT_OUT)
		ep.events |= EPOLLOUT;

	/**
	 *
	 * with EPOLLET we don't get a FDEVENT_HUP
	 * if the close is delay after everything has
	 * sent.
	 *
	 */
	
	/*
	 * EPOLLERR ：描述符发生错误。
	 * EPOLLHUP ：描述符被挂断。通常是连接断开。
	 */
	ep.events |= EPOLLERR | EPOLLHUP /* | EPOLLET */ ;

	ep.data.ptr = NULL;
	ep.data.fd = fd;

	/*
	 * EPOLL_CTL_ADD : 增加描述符fd到ev->epoll_fd中，并关联ep中的事件到fd上。
	 * EPOLL_CTL_MOD : 修改fd所关联的事件。
	 */
	if (0 != epoll_ctl(ev->epoll_fd, add ? EPOLL_CTL_ADD : EPOLL_CTL_MOD, fd, &ep))
	{
		fprintf(stderr, "%s.%d: epoll_ctl failed: %s, dying\n", __FILE__,
				__LINE__, strerror(errno));

		SEGFAULT();

		return 0;
	}

	return fd;
}

static int fdevent_linux_sysepoll_poll(fdevents * ev, int timeout_ms)
{
	//等待IO事件发生。
	//发生事件的fd对应的epoll_event将被放在ev->epoll_events数组中。
	return epoll_wait(ev->epoll_fd, ev->epoll_events, ev->maxfds, timeout_ms);
}

static int fdevent_linux_sysepoll_event_get_revent(fdevents * ev, size_t ndx)
{
	int events = 0, e;

	e = ev->epoll_events[ndx].events;
	if (e & EPOLLIN)
		events |= FDEVENT_IN;
	if (e & EPOLLOUT)
		events |= FDEVENT_OUT;
	if (e & EPOLLERR)
		events |= FDEVENT_ERR;
	if (e & EPOLLHUP)
		events |= FDEVENT_HUP;
	if (e & EPOLLPRI) //有紧急数据到达（带外数据）
		events |= FDEVENT_PRI;

	return e;
}

static int fdevent_linux_sysepoll_event_get_fd(fdevents * ev, size_t ndx)
{
# if 0
	fprintf(stderr, "%s.%d: %d, %d\n", __FILE__, __LINE__, ndx,
			ev->epoll_events[ndx].data.fd);
# endif

	return ev->epoll_events[ndx].data.fd;
}

static int fdevent_linux_sysepoll_event_next_fdndx(fdevents * ev, int ndx)
{
	size_t i;

	UNUSED(ev);

	i = (ndx < 0) ? 0 : ndx + 1;

	return i;
}

int fdevent_linux_sysepoll_init(fdevents * ev)
{
	ev->type = FDEVENT_HANDLER_LINUX_SYSEPOLL;
#define SET(x) \
	ev->x = fdevent_linux_sysepoll_##x;

	SET(free);
	SET(poll);

	SET(event_del);
	SET(event_add);

	SET(event_next_fdndx);
	SET(event_get_fd);
	SET(event_get_revent);

	if (-1 == (ev->epoll_fd = epoll_create(ev->maxfds)))
	{
		fprintf(stderr, "%s.%d: epoll_create failed (%s), try to set server.event-handler = \"poll\" or \"select\"\n",
				__FILE__, __LINE__, strerror(errno));

		return -1;
	}

	if (-1 == fcntl(ev->epoll_fd, F_SETFD, FD_CLOEXEC))
	{
		fprintf(stderr,	"%s.%d: epoll_create failed (%s), try to set server.event-handler = \"poll\" or \"select\"\n",
				__FILE__, __LINE__, strerror(errno));

		close(ev->epoll_fd);
		return -1;
	}

	ev->epoll_events = malloc(ev->maxfds * sizeof(*ev->epoll_events));

	return 0;
}

#else
int fdevent_linux_sysepoll_init(fdevents * ev)
{
	UNUSED(ev);

	fprintf(stderr,	"%s.%d: linux-sysepoll not supported, try to set server.event-handler = \"poll\" or \"select\"\n",
			__FILE__, __LINE__);

	return -1;
}
#endif
