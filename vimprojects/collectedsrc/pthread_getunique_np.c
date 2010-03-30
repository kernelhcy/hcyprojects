为 glibc 山寨了一个 pthread_getunique_np
发信站: 水木社区 (Mon Mar 29 13:10:27 2010), 站内

用的当然是康神推荐的适用于互联网行业的快糙猛的手法

glibc 的实现，pthread_t 实际是 pthread 结构的指针，其中有个成员叫
tid，但是具体在什么便宜量，不清楚，各个版本的实现或许会不一样，猜测
一下应该靠得住。

#include <errno.h>
#include <unistd.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>

static inline pid_t gettid()
{
    return syscall(SYS_gettid);
}

static int get_tid_offset()
{
    pthread_t self = pthread_self();
    if (self == 0)
    {
        //abort(); pthread library not linked
        return -1;
    }
    pid_t tid = gettid();
    pid_t* p = (pid_t*)self;
    for (size_t i = 0; i < 64; ++i)
    {
        if (p[i] == tid)
            return i * sizeof(pid_t);
    }
    // abort();
    return -1;
}

typedef pid_t pthread_id_np_t;
typedef pthread_id_np_t pthread_id_t;

int pthread_getunique_np(pthread_t thread, pthread_id_np_t* id)
{
    static const int tid_offset = get_tid_offset();
    if (tid_offset < 0)
        return ESRCH; // ?
    *id = *(pthread_id_np_t*)((char*)(thread) + tid_offset);
    return 0;
}

