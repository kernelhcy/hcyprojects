#include "ch11.h"

/****************************/

void runit(int argc, char *argv[])
{
	//p_11_1();
	//p_11_2();
	//p_11_3();
	p_11_4();
}

/***************************/

/*
 * glibc提供的函数，在linux的manual中说返回的是当前线程的thread ID.但是实际你看到的是一个很长的，似乎没有规律的值。
 * 什么原因得看看它的实现： 
 * 在glibc中，pthread_self()返回的是THREAD_SELF，这又是一个宏 
 * 定义如下 
 * # define THREAD_SELF \ 
 *   ({ struct pthread *__self;      \ 
 *      asm ("movl %%gs:%c1,%0" : "=r" (__self)      \ 
 *      : "i" (offsetof (struct pthread, header.self)));      \ 
 *        __self;}) 
 * 这段代码返回了当前线程的descriptor,pthread_self()得到的就是这个descriptor的地址, 
 * 也就是unsigned long int类型的pthread_t。
 */


/*
 * 程序11-1
 * 打印线程ID
 * 理论上主线程和新线程的pid应该不相同，但是输出结果相同。。。
 * 
 */
pthread_t ntid;

void printids(const char *s)
{
	pid_t 		pid;
	pthread_t 	tid;

	pid = getpid();
	/*
	 * pthread_t 被定义为unsigned long int，实际上它是一个地址（线程的descriptor的地址）。
	 * 因此，将其简单的转化成整形是没有什么意义的。
	 * 由于在线程的运行过程中，descriptor的地址是不变的，因此可以用其地址来标记这个线程。
	 */
	tid = pthread_self();
	printf("%s pid %u tid %u (0x%x)\n", s, (unsigned int)pid
			, (unsigned int)tid, (unsigned int)tid);
}

void *thr_fn(void *arg)
{
	printf("This is the new thread.\n");
	printids("new thread: ");
	return ((void *) 0);
}

void p_11_1()
{
	int 	err;
	err = pthread_create(&ntid, NULL, thr_fn, NULL);

	if (err != 0)
	{
		err_quit("can't create thread : %s\n", strerror(err));
	}

	printf("Main thread.\n");
	printids("main thread : ");

	sleep(1);
	exit(0);
}

/*
 * 程序11-2 获得线程推出状态
 */
void *thr_fn1(void *arg)
{
	printf("thread 1 returning...\n");
	return ((void *)1);
}
void *thr_fn2(void *arg)
{
	printf("thread 2 exiting...\n");
	pthread_exit((void *)2);
}
void p_11_2()
{
	int 		err;
	pthread_t 	tid1, tid2;
	void *tret;

	err = pthread_create(&tid1, NULL, thr_fn1, NULL);
	if (err != 0)
	{
		err_quit("can't create thread 1 : %s\n", strerror(err));
	}

	err = pthread_create(&tid2, NULL, thr_fn2, NULL);
	if (err != 0)
	{
		err_quit("can't create thread 2 : %s\n", strerror(err));
	}
	
	err = pthread_join(tid1, &tret);
	if (err != 0)
	{
		err_quit("can't join with thread 1: %s\n", strerror(err));
	}	
	printf("thread 1 exit code %d\n", (int)tret);

	err = pthread_join(tid2, &tret);
	if (err != 0)
	{
		err_quit("can't join with thread 1: %s\n", strerror(err));
	}
	printf("thread 1 exit code %d\n", (int)tret);
}

/*
 * 程序11-3 pthread_exit参数的不正确使用
 * 访问非法内存
 *
 */
struct foo
{
	int a, b, c, d;
};

void printfoo(const char *s, const struct foo *fp)
{
	printf(s);
	printf("  structure at 0x%x\n", (unsigned)fp);
	printf("  foo.a = %d \n", fp -> a);
	printf("  foo.b = %d \n", fp -> b);
	printf("  foo.c = %d \n", fp -> c);
	printf("  foo.d = %d \n", fp -> d);
}

void *thr_fn3(void *arg)
{
	struct foo foo = {1, 2, 3, 4};

	printfoo("thread 1: \n", &foo);
	pthread_exit((void *)&foo);
}

void *thr_fn4(void *arg)
{
	printf("thread 2: ID is %u\n", pthread_self());
	pthread_exit((void *)0);
}

void p_11_3()
{
	int 		err;
	pthread_t 	tid1, tid2;
	struct foo 	*fp;

	
	err = pthread_create(&tid1, NULL, thr_fn3, NULL);
	if (err != 0)
	{
		err_quit("can't create thread 1 : %s\n", strerror(err));
	}

	err = pthread_join(tid1, (void *)&fp);
	if (err != 0)
	{
		err_quit("can't join with thread 1: %s\n", strerror(err));
	}	
	sleep(1);
	printf("parent starts second thread.\n");

	err = pthread_create(&tid2, NULL, thr_fn4, NULL);
	if (err != 0)
	{
		err_quit("can't create thread 2 : %s\n", strerror(err));
	}
	sleep(1);

	printfoo("parent:\n", fp);

	exit(0);
}

/*
 * 程序11-4 线程清理程序
 */
void cleanup(void *arg)
{
	printf("cleanup : %s\n", (char *)arg);
}

void *thr_fn5(void *arg)
{
	printf("thread 1 start.\n");
	pthread_cleanup_push(cleanup, "thread 1 first handler");
	pthread_cleanup_push(cleanup, "thread 1 second handler");
	printf("thread 1 push complete.\n");
	if (arg)
	{
		/* return */
		return ((void *)1);
	}
	pthread_cleanup_pop(0);
	pthread_cleanup_pop(0);
	/* return */
	return ((void *)1);

}

void *thr_fn6(void *arg)
{
	printf("thread 2 start.\n");
	pthread_cleanup_push(cleanup, "thread 2 first handler");
	pthread_cleanup_push(cleanup, "thread 2 second handler");
	printf("thread 2 push complete.\n");
	if (arg)
	{
		/* exit  */
		pthread_exit((void *)2);
	}
	pthread_cleanup_pop(0);
	pthread_cleanup_pop(0);
	/* exit  */
	pthread_exit((void *)2);
}

void p_11_4()
{
	int 		err;
	pthread_t 	tid1, tid2;
	void 		*tret;

	
	err = pthread_create(&tid1, NULL, thr_fn5, (void *)1);
	if (err != 0)
	{
		err_quit("can't create thread 1 : %s\n", strerror(err));
	}

	err = pthread_create(&tid2, NULL, thr_fn6, (void *)1);/* 如果最后一个参数改为NULL， 则线程2也不会调用cleanup，因为已被pop... */
	if (err != 0)
	{
		err_quit("can't create thread 2 : %s\n", strerror(err));
	}
	
	err = pthread_join(tid1, &tret);
	if (err != 0)
	{
		err_quit("can't join with thread 1: %s\n", strerror(err));
	}	
	printf("thread 1 exit code %d\n", (int)tret);

	err = pthread_join(tid2, &tret);
	if (err != 0)
	{
		err_quit("can't join with thread 1: %s\n", strerror(err));
	}
	printf("thread 1 exit code %d\n", (int)tret);

	exit(0);
}

