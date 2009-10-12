#include "ch10.h"

/***************** static function **************************/

/* used by p_10_1 */
static void sig_uer(int);

/* used by p_10_2 */
static void my_alarm(int);

/* used by p_10_3 */
static void sig_cld(int);

/***************** end of static function *******************/


void runit(int argc, char *argv[])
{
	p_10_1();
	//p_10_2();
	
}

/*
 * 程序10-1
 * 捕捉SIGUSR1和SIGUSR2的简单程序
 */
void p_10_1()
{
	pid_t 	pid;
	int		status;
	
	if ((pid = fork()) < 0)
	{
		err_sys("fork error.");
	}
	
	if (pid == 0)
	{
		if (signal(SIGUSR1, sig_uer) == SIG_ERR)
		{
			err_sys("can't catch SIGUSR1.");
		}
		
		if (signal(SIGUSR2, sig_uer) == SIG_ERR)
		{
			err_sys("can't catch SIGUSR2.");
		}
	
		//for(;;)
		//{
			pause();
		//}
		printf("child exit.\n");
		_exit(1);
	}
	
	/* parent */
	kill(pid, SIGUSR1);
	//sleep(2);
	kill(pid, SIGUSR2);
	
	
	if (signal(SIGUSR1, sig_uer) == SIG_ERR)
	{
		err_sys("can't catch SIGUSR1.");
	}
	
	if (signal(SIGUSR2, sig_uer) == SIG_ERR)
	{
		err_sys("can't catch SIGUSR2.");
	}
	raise(SIGUSR1);
	raise(SIGUSR2);
	
	if (waitpid(pid, &status) < 0)
	{
		err_sys("waitpid error,pid : %d", pid);
	}
	printf("child status : %d\n", status);
	exit(0);
	
}
static void sig_uer(int signo)
{
	if (signo == SIGUSR1)
	{
		printf("received SIGUSR1\n");
	}
	else if (signo == SIGUSR2)
	{
		printf("received SIGUSR2\n");
	}
	else
	{
		err_dump("received signal %d\n", signo);
	}
}

/*
 * 程序10-2
 * 在信号处理程序中调用不可重入函数
 */
#include <pwd.h>
void p_10_2()
{
	struct passwd	*ptr;
	
	signal(SIGALRM, my_alarm);
	alarm(1);
	for(;;)
	{
		if ((ptr = getpwnam("hcy")) == NULL) 
		{
			err_sys("getpwnam error");
		}
		
		if (strcmp(ptr -> pw_name, "hcy") !=0)
		{
			printf("return value corrupted!, pw_name = %s.\n", ptr -> pw_name);
		}
		
		printf("for again\n");
	}
}
static void my_alarm(int signo)
{
	struct passwd	*rootptr;
	
	printf("in signal handler\n");
	if ((rootptr = getpwnam("root")) == NULL)
	{
		err_sys("getpwnam(root) error");
	}
	printf("exit signal handler\n");
	alarm(1);
	printf("reset alarm.\n");
}

/*
 * 程序10－3
 * 不能正常工作的系统V SIGCLD处理程序
 */
void p_10_3()
{
	pid_t 	pid;

	if (signal(SIGCLD, sig_cld) == SIG_ERR)
	{
		
	}
}
static void sig_cld(int signo)
{
	pid_t 	pid;
	int 	status;

	printf("SIGCLD received.\n");
	if (signal(SIGCLD, sig_cld) == SIG_ERR)
	{
		perror("signal error");
	}

	if ((pid = wait(&status)) < 0)
	{
		perror("wait error");	
	}

	printf("pid = %d\n", pid);
}
