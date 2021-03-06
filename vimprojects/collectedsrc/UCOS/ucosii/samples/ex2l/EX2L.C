/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*
*                        (c) Copyright 1992-1998, Jean J. Labrosse, Plantation, FL
*                                           All Rights Reserved
*
*                                                 V2.00
*
*                                               EXAMPLE #2
*********************************************************************************************************
*/

#include "includes.h"

/*
*********************************************************************************************************
*                                              CONSTANTS
*********************************************************************************************************
*/

#define          TASK_STK_SIZE     512                /* Size of each task's stacks (# of WORDs)       */

#define          TASK_START_ID       0                /* Application tasks IDs                         */
#define          TASK_CLK_ID         1
#define          TASK_1_ID           2                
#define          TASK_2_ID           3
#define          TASK_3_ID           4
#define          TASK_4_ID           5
#define          TASK_5_ID           6

#define          TASK_START_PRIO    10                /* Application tasks priorities                  */
#define          TASK_CLK_PRIO      11
#define          TASK_1_PRIO        12                
#define          TASK_2_PRIO        13
#define          TASK_3_PRIO        14
#define          TASK_4_PRIO        15
#define          TASK_5_PRIO        16

/*
*********************************************************************************************************
*                                              VARIABLES
*********************************************************************************************************
*/

OS_STK           TaskStartStk[TASK_STK_SIZE];         /* Startup    task stack                         */
OS_STK           TaskClkStk[TASK_STK_SIZE];           /* Clock      task stack                         */
OS_STK           Task1Stk[TASK_STK_SIZE];             /* Task #1    task stack                         */
OS_STK           Task2Stk[TASK_STK_SIZE];             /* Task #2    task stack                         */
OS_STK           Task3Stk[TASK_STK_SIZE];             /* Task #3    task stack                         */
OS_STK           Task4Stk[TASK_STK_SIZE];             /* Task #4    task stack                         */
OS_STK           Task5Stk[TASK_STK_SIZE];             /* Task #5    task stack                         */

#define MAX_SPIN    0x04

static char spinleft[10] = "|/-\\\0";
static char spinright[10] = "\\-/|\0";

OS_EVENT        *rightSem;
OS_EVENT        *leftSem;

OS_EVENT        *AckMbox;                             /* Message mailboxes for Tasks #4 and #5         */
OS_EVENT        *TxMbox;

static int rightcnt = 0x00;
static int leftcnt  = 0x00;
static int speed    = 30;

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void             TaskStart(void *data);               /* Function prototypes of tasks                  */
void             TaskClk(void *data);
void             Task1(void *data);
void             Task2(void *data);
void             Task3(void *data);
void             Task4(void *data);
void             Task5(void *data);

/*$PAGE*/
/*
*********************************************************************************************************
*                                                  MAIN
*********************************************************************************************************
*/

void main (void)
{
	PC_DispClrScr(DISP_FGND_WHITE);                        /* Clear the screen                         */

	OSInit();                                              /* Initialize uC/OS-II                      */
    
    PC_DOSSaveReturn();                                    /* Save environment to return to DOS        */
    
    PC_VectSet(uCOS, OSCtxSw);                             /* Install uC/OS-II's context switch vector */
    
    PC_ElapsedInit();                                      /* Initialized elapsed time measurement     */

	OSTaskCreateExt(TaskStart, (void *)0, &TaskStartStk[TASK_STK_SIZE-1], TASK_START_PRIO,
				   TASK_START_ID, &TaskStartStk[0], TASK_STK_SIZE, (void *)0,
                   OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
                   
    OSStart();                                             /* Start multitasking                       */
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                               STARTUP TASK
*********************************************************************************************************
*/

void  TaskStart (void *data)
{
    char   s[80];
    INT16S key;


	data = data;                                           /* Prevent compiler warning                 */
    
    PC_DispStr(26,  0, "uC/OS-II, The Real-Time Kernel", DISP_FGND_WHITE + DISP_BGND_RED + DISP_BLINK);
    PC_DispStr(33,  1, "Jean J. Labrosse", DISP_FGND_WHITE);
    PC_DispStr(36,  3, "EXAMPLE #2", DISP_FGND_WHITE);
    PC_DispStr( 0,  9, "Task           Total Stack  Free Stack  Used Stack  ExecTime (uS)", DISP_FGND_WHITE);
    PC_DispStr( 0, 10, "-------------  -----------  ----------  ----------  -------------", DISP_FGND_WHITE);
	PC_DispStr( 0, 12, "TaskStart():", DISP_FGND_WHITE);
	PC_DispStr( 0, 13, "TaskClk()  :", DISP_FGND_WHITE);
	PC_DispStr( 0, 14, "Task1()    :", DISP_FGND_WHITE);
    PC_DispStr( 0, 15, "Task2()    :", DISP_FGND_WHITE);
    PC_DispStr( 0, 16, "Task3()    :", DISP_FGND_WHITE);
    PC_DispStr( 0, 17, "Task4()    :", DISP_FGND_WHITE);
    PC_DispStr( 0, 18, "Task5()    :", DISP_FGND_WHITE);
    PC_DispStr(28, 24, "<-PRESS 'ESC' TO QUIT->", DISP_FGND_WHITE + DISP_BLINK);

	OS_ENTER_CRITICAL();                                   /* Install uC/OS-II's clock tick ISR        */
	PC_VectSet(0x08, OSTickISR);
	PC_SetTickRate(OS_TICKS_PER_SEC);                      /* Reprogram tick rate                      */
    OS_EXIT_CRITICAL();

    PC_DispStr(0, 22, "Determining  CPU's capacity ...", DISP_FGND_WHITE);  
    OSStatInit();                                          /* Initialize uC/OS-II's statistics         */
    PC_DispClrLine(22, DISP_FGND_WHITE + DISP_BGND_BLACK);

	AckMbox = OSMboxCreate((void *)0);                     /* Create 2 message mailboxes               */
	TxMbox  = OSMboxCreate((void *)0);

	rightSem = OSSemCreate(1);
	leftSem  = OSSemCreate(0);                             // create semaphore for task2 and task3
    
    OSTaskCreateExt(TaskClk, (void *)0, &TaskClkStk[TASK_STK_SIZE-1], TASK_CLK_PRIO, 
                   TASK_CLK_ID, &TaskClkStk[0], TASK_STK_SIZE, (void *)0, 
                   OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);

	OSTaskCreateExt(Task1, (void *)0, &Task1Stk[TASK_STK_SIZE-1], TASK_1_PRIO,
				   TASK_1_ID, &Task1Stk[0], TASK_STK_SIZE, (void *)0,
                   OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);

    OSTaskCreateExt(Task2, (void *)0, &Task2Stk[TASK_STK_SIZE-1], TASK_2_PRIO, 
                   TASK_2_ID, &Task2Stk[0], TASK_STK_SIZE, (void *)0, 
                   OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
                   
	OSTaskCreateExt(Task3, (void *)0, &Task3Stk[TASK_STK_SIZE-1], TASK_3_PRIO,
				   TASK_3_ID, &Task3Stk[0], TASK_STK_SIZE, (void *)0,
				   OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);

    OSTaskCreateExt(Task4, (void *)0, &Task4Stk[TASK_STK_SIZE-1], TASK_4_PRIO, 
                   TASK_4_ID, &Task4Stk[0], TASK_STK_SIZE, (void *)0, 
                   OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);

    OSTaskCreateExt(Task5, (void *)0, &Task5Stk[TASK_STK_SIZE-1], TASK_5_PRIO, 
				   TASK_5_ID, &Task5Stk[0], TASK_STK_SIZE, (void *)0,
				   OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);

    PC_DispStr( 0, 22, "#Tasks          : xxxxx  CPU Usage: xxx %", DISP_FGND_WHITE);
    PC_DispStr( 0, 23, "#Task switch/sec: xxxxx", DISP_FGND_WHITE);

    for (;;) 
	{
        sprintf(s, "%5d", OSTaskCtr);                     /* Display #tasks running                    */
		PC_DispStr(18, 22, s, DISP_FGND_BLUE + DISP_BGND_CYAN);
		sprintf(s, "%3d", OSCPUUsage);                    /* Display CPU usage in %                    */
		PC_DispStr(36, 22, s, DISP_FGND_BLUE + DISP_BGND_CYAN);
        sprintf(s, "%5d", OSCtxSwCtr);                    /* Display #context switches per second      */
        PC_DispStr(18, 23, s, DISP_FGND_BLUE + DISP_BGND_CYAN);

        OSCtxSwCtr = 0;                                    /* Clear context switch counter             */
        
        sprintf(s, "V%3.2f", (float)OSVersion() * 0.01);
		PC_DispStr(75, 24, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);

		if (PC_GetKey(&key))
		{                             /* See if key has been pressed              */
            if (key == 0x1B) 
			{                             /* Yes, see if it's the ESCAPE key          */
                PC_DOSReturn();                            /* Yes, return to DOS                       */
            }
        }

		OSTimeDly(OS_TICKS_PER_SEC);                       /* Wait one second                          */
	}
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                               TASK #1
*
* Description: This task executes every 100 mS and measures the time it task to perform stack checking
*              for each of the 5 application tasks.  Also, this task displays the statistics related to
*              each task's stack usage.
*********************************************************************************************************
*/

void  Task1 (void *pdata)
{
    INT8U       err;
	OS_STK_DATA data;                       /* Storage for task stack data                             */
	INT16U      time;                       /* Execution time (in uS)                                  */
	INT8U       i;
    char        s[80];


    pdata = pdata;
    for (;;) 
	{
		for (i = 0; i < 7; i++)
		{
			PC_ElapsedStart();
            err  = OSTaskStkChk(TASK_START_PRIO+i, &data);
            time = PC_ElapsedStop();
            if (err == OS_NO_ERR) 
			{
                sprintf(s, "%3ld         %3ld         %3ld         %5d",
                        data.OSFree + data.OSUsed,
						data.OSFree,
						data.OSUsed,
						time);
                PC_DispStr(19, 12+i, s, DISP_FGND_YELLOW);
            }
        }
        OSTimeDlyHMSM(0, 0, 0, 100);                       /* Delay for 100 mS                         */
    }
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                               TASK #2
*
* Description: This task displays a clockwise rotating wheel on the screen.
*********************************************************************************************************
*/

void  Task2 (void *data)
{
	INT8U   err;

	data = data;
	for (;;)
	{
		OSSemPend(rightSem, 0x00, &err);
		if (err != OS_TIMEOUT)
		{
			PC_DispChar(70, 15, spinright[rightcnt],  DISP_FGND_WHITE + DISP_BGND_RED);
			rightcnt ++;
			rightcnt %= MAX_SPIN;
		}
		OSTimeDly(speed);
		OSSemPost(leftSem);
	}
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                               TASK #3
*
* Description: This task displays a counter-clockwise rotating wheel on the screen.
*
* Note(s)    : I allocated 100 bytes of storage on the stack to artificially 'eat' up stack space.
*********************************************************************************************************
*/

void  Task3 (void *data)
{
	INT8U   err;

	data = data;
	for (;;)
	{
		OSSemPend(leftSem, 0x00, &err);
		if (err != OS_TIMEOUT)
		{
			PC_DispChar(70, 16, spinleft[leftcnt],  DISP_FGND_WHITE + DISP_BGND_BLUE);
			leftcnt ++;
			leftcnt %= MAX_SPIN;
		}
		OSTimeDly(speed);
		OSSemPost(rightSem);
	}
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                               TASK #4
*
* Description: This task sends a message to Task #5.  The message consist of a character that needs to
*              be displayed by Task #5.  This task then waits for an acknowledgement from Task #5
*              indicating that the message has been displayed.
*********************************************************************************************************
*/

void  Task4 (void *data)
{
    char   txmsg;
    INT8U  err;
    
    
    data  = data;
    txmsg = 'A';
    for (;;) 
	{
        while (txmsg <= 'Z') 
		{
            OSMboxPost(TxMbox, (void *)&txmsg);  /* Send message to Task #5                            */
            OSMboxPend(AckMbox, 0, &err);        /* Wait for acknowledgement from Task #5              */
            txmsg++;                             /* Next message to send                               */
        }
        txmsg = 'A';                             /* Start new series of messages                       */
    }
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                               TASK #5
*
* Description: This task displays messages sent by Task #4.  When the message is displayed, Task #5
*              acknowledges Task #4.
*********************************************************************************************************
*/

void  Task5 (void *data)
{
    char  *rxmsg;
    INT8U  err;
    
    
    data = data;
    for (;;) 
	{
        rxmsg = (char *)OSMboxPend(TxMbox, 0, &err);                  /* Wait for message from Task #4 */
        PC_DispChar(70, 18, *rxmsg, DISP_FGND_YELLOW + DISP_BGND_RED);
        OSTimeDlyHMSM(0, 0, 1, 0);                                    /* Wait 1 second                 */
        OSMboxPost(AckMbox, (void *)1);                               /* Acknowledge reception of msg  */
    }
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                               CLOCK TASK
*********************************************************************************************************
*/

void  TaskClk (void *data)
{
    struct time now;
    struct date today;
    char        s[40];


    data = data;
    for (;;) 
	{
        PC_GetDateTime(s);
        PC_DispStr(0, 24, s, DISP_FGND_BLUE + DISP_BGND_CYAN);
        OSTimeDly(OS_TICKS_PER_SEC);
    }
}

