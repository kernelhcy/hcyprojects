
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
/*
 ******************************************
 * soft interrupt process communication
 ******************************************
 */

void show_msg1(int);
void show_msg2(int);
void kill_children(int);

pid_t child1, child2;
int status;

void interrupt_communicate()
{
    child1 = fork();
    /*
     * In the child process, the return of fork() is ZERO!!!
     * In the parent process,the return of fork() is the process id of the child process.
     *
     */
    if (child1 == 0)//Child process 1
    {
        //std::cout << "Child Process One." << std::endl;
//        while(true)
//        {
//            signal(16, show_msg1);
//        }

        signal(16, show_msg1);
        
        //Wait for the signal
        sleep(200);
        return;
    }

    child2 = fork();
    if (child2 == 0)//Child process 2
    {
        //std::cout << "Child Process Two." << std::endl;
//        while(true)
//        {
//            signal(17, show_msg2);
//        }

        signal(17, show_msg2);
        //Wait for the signal
        sleep(200);
        return;
    }

    //Parent process

    //Wait for the quit signal.
    signal(SIGQUIT,kill_children);

    //Wait for the suspend signle
    signal(SIGTSTP,kill_children);

    //Trigger the alarm signle after 5 s.
    alarm(5);
    signal(SIGALRM,kill_children);

    //Wait for child process 1 to run over.
    waitpid(child1,&status,0);
    std::cout << "The child process 1's status is: " << status << std::endl;

    //Wait for child process 2 to run over.
    waitpid(child2,&status,0);
    std::cout << "The child process 2's status is: " << status << std::endl;

    std::cout << "Parent process is killed!!" << std::endl;
    return;
}
/*
 * Kill the children processes
 */
void kill_children(int sig)
{
    std::cout<<"The parent process got a signal: "<<sig<<std::endl;

    //Send signal 16 to child process 1
    kill(child1, 16);

    //Send signal 17 to child process 2
    kill(child2, 17);
}

/*
 * 
 */
void show_msg1(int sig)
{
    std::cout << "Signal:" << sig << std::endl;
    std::cout << "Child process 1 is killed by parent!!" << std::endl;

    return;
}

/*
 *
 */
void show_msg2(int sig)
{

    std::cout << "Signal:" << sig << std::endl;
    std::cout << "Child process 2 is killed by parent!!" << std::endl;

    return;
}


/********************************************/

/*
 * Process message communication
 */

const int INPUT = 0;
const int OUTPUT = 1;
void message_communicate()
{
    int pipedes[2];
    pipe(pipedes);

    child1 = fork();
    /*
     * In the child process, the return of fork() is ZERO!!!
     * In the parent process,the return of fork() is the process id of the child process.
     *
     */
    if (child1 == 0)//Child process 1
    {
        std::cout << "Child Process One." << std::endl;
        //close the read pipe
        close(pipedes[INPUT]);
        //write message through pipe to parent process
        write(pipedes[OUTPUT], "Child process 1 is sending a message!!\n", strlen("Child process 1 is sending a message!!\n"));
        close(pipedes[OUTPUT]);
        return;
    }

    child2 = fork();
    if (child2 == 0)//Child process 2
    {
        std::cout << "Child Process Two." << std::endl;
        //close the read pipe
        close(pipedes[INPUT]);
        //
        write(pipedes[OUTPUT], "Child process 2 is sending a message!!\n", strlen("Child process 2 is sending a message!!\n"));
        close(pipedes[OUTPUT]);
        return;
    }

    //Parent process
    char buffer[200];
    int len;
    len = read(pipedes[INPUT], buffer, sizeof(buffer));
    std::cout << buffer;
    
    close(pipedes[INPUT]);

    return;
}

/*
 * The Enter of the Program
 */
void run()
{
    //std::cout << "Process Communication Test." << std::endl;
    interrupt_communicate();
    //message_communicate();
}


