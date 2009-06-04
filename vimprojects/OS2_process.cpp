
/**
 *
 * OS
 *   Process Communication
 *
 */
#include "headers.h"
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <bits/signum.h> 
#include <sys/file.h>
/*
 ******************************************
 * 软中断通讯实验 
 ******************************************
 */
//函数声明
//子进程一捕捉到信号后的处理函数
void show_msg1(int);
//子进程二捕捉到信号后的处理函数
void show_msg2(int);
//父进程
void kill_children(int);

//两个子进程的进程ID
pid_t child1, child2;
//子进程状态
int status;

//
void interrupt_communicate()
{
    child1 = fork();//产生子进程
    /*
     * In the child process, the return of fork() is ZERO!!!
     * In the parent process,the return of fork() is the process id of the child process.
     *
     */
    if (child1 == 0)//子进程一
    {
        //std::cout << "Child Process One." << std::endl;
//        while(true)
//        {
//            signal(16, show_msg1);
//        }

		//接收软中断信号16
        signal(16, show_msg1);
        
        //等待父进程的软中断信号16
        sleep(200);
        return;
    }

    child2 = fork();
    if (child2 == 0)//子进程二
    {
        //std::cout << "Child Process Two." << std::endl;
//        while(true)
//        {
//            signal(17, show_msg2);
//        }
		
		//接收软中断信号17
        signal(17, show_msg2);
        
		//等待软中断信号
        sleep(200);
        return;
    }

    //父进程部分

    //接收SIGQUIT信号
    signal(SIGQUIT,kill_children);

    //接收SIGTSTP信号
    signal(SIGTSTP,kill_children);

	//等待5s触发时钟信号
    alarm(5);
	for(int i = 0; i < 4; ++i)
	{
		std::cout << i+1 << "s..." <<std::endl;
		sleep(1);
	}
	std::cout << "5s..." <<std::endl;
	//接收时钟信号
    signal(SIGALRM,kill_children);

    //等待子进程一运行结束
    waitpid(child1,&status,0);
    std::cout << "The child process 1's status is: " << status << std::endl;

    //等待子进程二运行结束
    waitpid(child2,&status,0);
    std::cout << "The child process 2's status is: " << status << std::endl;

    std::cout << "Parent process is killed!!" << std::endl;
    return;
}
/*
 * 分别向子进程一和二发送软中断信号。
 * 父进程的信号处理函数。
 */
void kill_children(int sig)
{
    std::cout<<"The parent process got a signal: "<<sig<<std::endl;

    //向子进程一发送软中断信号16
    kill(child1, 16);

    //向子进程二发送软中断信号17
    kill(child2, 17);
}

/*
 * 子进程一的信号处理函数
 */
void show_msg1(int sig)
{
    std::cout << "Signal:" << sig << std::endl;
    std::cout << "Child process 1 is killed by parent!!" << std::endl;

    return;
}

/*
 * 子进程二的信号处理函数
 */
void show_msg2(int sig)
{

    std::cout << "Signal:" << sig << std::endl;
    std::cout << "Child process 2 is killed by parent!!" << std::endl;

    return;
}


/*****************************************************************************/

/*
 * 进程管道通信实验
 */

const int INPUT = 0;//管道读标志
const int OUTPUT = 1;//管道写标志
const int BUFFERSIZE = 100;//缓冲区大小
//输入和输出缓冲区
char topipe[BUFFERSIZE],frompipe[BUFFERSIZE];

/*
 * 
 */
void message_communicate()
{
    int pipedes[2];
    pipe(pipedes);

    child1 = fork();
    /*
     * In the child process, the return of fork() is ZERO!!!
     * In the parent process,the return of fork() is the
	 * process id of the child process.
     *
     */
    if (child1 == 0)//子进程一
    {
        std::cout << "Child Process One." << std::endl;
        //关闭输入管道
        close(pipedes[INPUT]);
		
		//锁定管道
		std::cout << "Child 1 lock the pipe.\n";
		lockf(pipedes[OUTPUT], LOCK_EX, 0);
        
		//将信息写到输出缓冲区中	
		sprintf(topipe, "Child process 1 is sending a message!!\n");
		//将信息写入管道
        write(pipedes[OUTPUT], topipe, BUFFERSIZE);
		
		//解锁管道
		std::cout << "Child 1 unlock the pipe.\n";
		lockf(pipedes[OUTPUT], LOCK_UN, 0);
		
		//关闭输出管道
		close(pipedes[OUTPUT]);
        return;
    }

    child2 = fork();
    if (child2 == 0)//子进程二
    {
        std::cout << "Child Process Two." << std::endl;
        //关闭输入管道
        close(pipedes[INPUT]);
        //
		//锁定输出管道
		std::cout << "Child 2 lock the pipe.\n";
		lockf(pipedes[OUTPUT], LOCK_EX, 0);
        
		//输出信息到管道
		sprintf(topipe, "Child process 2 is sending a message!!\n");
		//write message through pipe to parent process
        write(pipedes[OUTPUT], topipe, BUFFERSIZE);
		
		//解锁管道
		std::cout << "Child 2 unlock the pipe.\n";
		lockf(pipedes[OUTPUT], LOCK_UN, 0);
        
		//关闭输出管道
		close(pipedes[OUTPUT]);
        return;
    }

    //父进程
    int len;//从管道中读出的信息的长度

	//从管道中读出子进程一发送的信息
	//等待子进程一退出
    len = read(pipedes[INPUT], frompipe, BUFFERSIZE);
    std::cout << frompipe;
    waitpid(child1, &status, 0);

	//从管道中读出子进程二发送的信息
	//等待子进程二退出
    len = read(pipedes[INPUT], frompipe, BUFFERSIZE);
    std::cout << frompipe;
    waitpid(child2, &status, 0);
	
	//关闭输入管道
    close(pipedes[INPUT]);

    return;
}

/*
 * The Enter of the Program
 * 函数的入口
 */
void run()
{
    //std::cout << "Process Communication Test." << std::endl;
    interrupt_communicate();
    //message_communicate();
}



