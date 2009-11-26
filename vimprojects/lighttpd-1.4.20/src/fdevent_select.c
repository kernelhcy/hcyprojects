/**
 * 处理select函数的多路IO的具体操作.
 */
#include <sys/time.h>
#include <sys/types.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <assert.h>

#include "fdevent.h"
#include "settings.h"
#include "buffer.h"

#ifdef USE_SELECT

static int fdevent_select_reset(fdevents * ev)
{
	//将fd_set清零。
	FD_ZERO(&(ev->select_set_read));
	FD_ZERO(&(ev->select_set_write));
	FD_ZERO(&(ev->select_set_error));
	//最大文件描述符个数设为-1
	ev->select_max_fd = -1;

	return 0;
}

static int fdevent_select_event_del(fdevents * ev, int fde_ndx, int fd)
{
	if (fde_ndx < 0)
		return -1;

	FD_CLR(fd, &(ev->select_set_read));
	FD_CLR(fd, &(ev->select_set_write));
	FD_CLR(fd, &(ev->select_set_error));

	return -1;
}

/**
 * 根据events的设置来将文件描述符fd加到文件描述符集合中。
 * 用于select函数。
 */
static int fdevent_select_event_add(fdevents * ev, int fde_ndx, int fd, int events)
{
	UNUSED(fde_ndx);

	/*
	 * we should be protected by max-fds, but you never know 
	 */
	assert(fd < FD_SETSIZE);

	if (events & FDEVENT_IN)
	{
		FD_SET(fd, &(ev->select_set_read));
		FD_CLR(fd, &(ev->select_set_write));
	}
	if (events & FDEVENT_OUT)
	{
		FD_CLR(fd, &(ev->select_set_read));
		FD_SET(fd, &(ev->select_set_write));
	}

	//关心所有文件描述符的出错
	FD_SET(fd, &(ev->select_set_error));

	//以情况修改最大文件描述符数
	if (fd > ev->select_max_fd)
		ev->select_max_fd = fd;

	return fd;
}

/**
 * 根据时间限制timeout_ms启动select多路IO。
 * 三个文件描述符集合在ev中。
 */
static int fdevent_select_poll(fdevents * ev, int timeout_ms)
{
	struct timeval tv;

	tv.tv_sec = timeout_ms / 1000;
	tv.tv_usec = (timeout_ms % 1000) * 1000;

	ev->select_read = ev->select_set_read;
	ev->select_write = ev->select_set_write;
	ev->select_error = ev->select_set_error;

	return select(ev->select_max_fd + 1, &(ev->select_read),
				  &(ev->select_write), &(ev->select_error), &tv);
}
/**
 * 获得文件描述符ndx的返回状态
 *
 * select会修改三个参数集合，以标记出哪些文件描述符的状态改变了。
 * 如果select函数返回后，三个描述符集中，有文件描述符的相应未仍然是设置的，
 * 那么这个文件描述符达到了对应的状态。
 * 
 * 也就是ndx是否在ev中的三个文件描述符集中。
 * ndx是所需要查找的文件描述符在数组中的下标，由于存储时，文件描述符
 * 就存放在下标与其值相同的位置，也就是ndx==fd。
 */
static int fdevent_select_event_get_revent(fdevents * ev, size_t ndx)
{
	int revents = 0;

	if (FD_ISSET(ndx, &(ev->select_read)))
	{
		revents |= FDEVENT_IN;
	}
	if (FD_ISSET(ndx, &(ev->select_write)))
	{
		revents |= FDEVENT_OUT;
	}
	if (FD_ISSET(ndx, &(ev->select_error)))
	{
		revents |= FDEVENT_ERR;
	}

	return revents;
}

//返回ndx位置的文件描述符，ndx==fd
static int fdevent_select_event_get_fd(fdevents * ev, size_t ndx)
{
	UNUSED(ev);

	return ndx;
}

//返回从ndx开始的下一个位置，
//在三个文件描述符集合中存在的文件描述符。
static int fdevent_select_event_next_fdndx(fdevents * ev, int ndx)
{
	int i;

	i = (ndx < 0) ? 0 : ndx + 1;

	for (; i < ev->select_max_fd + 1; i++)
	{
		if (FD_ISSET(i, &(ev->select_read)))
			break;
		if (FD_ISSET(i, &(ev->select_write)))
			break;
		if (FD_ISSET(i, &(ev->select_error)))
			break;
	}

	return i;
}
/**
 * 初始化select多路IO。
 */
int fdevent_select_init(fdevents * ev)
{
	ev->type = FDEVENT_HANDLER_SELECT;

	//这个宏简化后面的函数设置。
#define SET(x) \
	ev->x = fdevent_select_##x;

	SET(reset); //ev -> reset = fdevent_select_reset
	SET(poll);

	SET(event_del);
	SET(event_add);

	SET(event_next_fdndx);
	SET(event_get_fd);
	SET(event_get_revent);

	return 0;
}

#else
int fdevent_select_init(fdevents * ev)
{
	UNUSED(ev);

	return -1;
}
#endif
