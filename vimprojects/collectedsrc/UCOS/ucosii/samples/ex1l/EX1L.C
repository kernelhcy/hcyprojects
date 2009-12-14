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
*                                               EXAMPLE #1
*********************************************************************************************************
*/

#include "includes.h"

#include <string.h>
/*
*********************************************************************************************************
*                                               CONSTANTS
*********************************************************************************************************
*/

#define  TASK_STK_SIZE                  1024     /* Size of each task's stacks (# of WORDs)            */
#define  N_TASKS                        10       /* Number of identical tasks                          */

/*
*********************************************************************************************************
*                                               VARIABLES
*********************************************************************************************************
*/

OS_STK           TaskStk[N_TASKS][TASK_STK_SIZE];     /* Tasks stacks                                  */
OS_STK           TaskStartStk[TASK_STK_SIZE];
char             TaskData[N_TASKS];                   /* Parameters to pass to each task               */
OS_EVENT        *RandomSem;

char             sCmdPrompt[10] = "\\>\0";            // shell command prompt
char             sRunning[10];
WORD             StartY = 16;
WORD             EndY = 21; 
WORD             CurY = 16;
/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void   Task(void *data);                              /* Function prototypes of tasks                  */
void   TaskStart(void *data);                         /* Function prototypes of Startup task           */

/*$PAGE*/
/*
*********************************************************************************************************
*                                                MAIN
*********************************************************************************************************
*/

void main (void)
{
    memset(sRunning, 0x01, 10);                            // flag all task will run at kernel starting

    PC_DispClrScr(DISP_FGND_WHITE + DISP_BGND_BLACK);      /* Clear the screen                         */
    OSInit();                                              /* Initialize uC/OS-II                      */
    PC_DOSSaveReturn();                                    /* Save environment to return to DOS        */   
    PC_VectSet(uCOS, OSCtxSw);                             /* Install uC/OS-II's context switch vector */
	RandomSem = OSSemCreate(1);                            /* Random number semaphore                  */
    OSTaskCreate(TaskStart, (void *)0, (void *)&TaskStartStk[TASK_STK_SIZE - 1], 0);
    OSStart();                                             /* Start multitasking                       */
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                              Display Prompt
*********************************************************************************************************
*/
void DisplayPrompt ( void )
{
	PC_DispStr(0, CurY, sCmdPrompt, DISP_FGND_WHITE + DISP_BGND_BLUE);
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                              Clear Message Window
*********************************************************************************************************
*/
void ClearMessageWindow ( void )
{
	int  i;
	char temp[82];

	memset(temp, 0x20, 80);
	temp[80] = '\0';

	for (i = StartY; i < EndY; i ++)
		PC_DispStr(0, i, temp, DISP_FGND_WHITE + DISP_BGND_BLUE);

	CurY = StartY;
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                              Display Message
*********************************************************************************************************
*/
void DisplayMessage ( char *sMsg )
{
	PC_DispClrLine(EndY - 1, DISP_FGND_WHITE + DISP_BGND_BLUE);
	PC_DispStr(0, EndY - 1, sMsg, DISP_FGND_WHITE + DISP_BGND_BLUE);
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                              Parse Command
*********************************************************************************************************
*/
int ParseCommand ( char * strCmd )
{
	if (strcmp(strCmd, "task1") == 0x00)
		return 0x01;
	else if (strcmp(strCmd, "task2") == 0x00)
		return 0x02;
	else if (strcmp(strCmd, "task3") == 0x00)
		return 0x03;
	else if (strcmp(strCmd, "task4") == 0x00)
		return 0x04;
	else if (strcmp(strCmd, "task5") == 0x00)
		return 0x05;
	else if (strcmp(strCmd, "task6") == 0x00)
		return 0x06;
	else if (strcmp(strCmd, "task7") == 0x00)
		return 0x07;
	else if (strcmp(strCmd, "task8") == 0x00)
		return 0x08;
	else if (strcmp(strCmd, "task9") == 0x00)
		return 0x09;
	else if (strcmp(strCmd, "startall") == 0x00)
		return 0x101;
	else if (strcmp(strCmd, "stopall") == 0x00)
		return 0x102;

	return 0x00;
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                              Parse Command
*********************************************************************************************************
*/
int HandleCommand ( WORD index )
{
	char   sMsg[80];

	if (index > 0x100)
	{
		switch(index)
		{
			case 0x101:
				DisplayMessage("Start all tasks just now ...");
				break;
			case 0x102:
				DisplayMessage("Stop all tasks just now ...");
				break;
		}
	}
	else if (index > 0x00 && index < 0x0a)
	{
		sprintf(sMsg, "Start task%d just now ...", index);
		DisplayMessage(sMsg);
	}

	return 0x00;
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                              Display Status
*********************************************************************************************************
*/
void DisplayStatus ( void )
{
	char   s[100];

	sprintf(s, "%5d", OSTaskCtr);                     /* Display #tasks running                    */
	PC_DispStr(18, 22, s, DISP_FGND_BLUE + DISP_BGND_CYAN);
	sprintf(s, "%3d", OSCPUUsage);                    /* Display CPU usage in %                    */
	PC_DispStr(36, 22, s, DISP_FGND_BLUE + DISP_BGND_CYAN);
	sprintf(s, "%5d", OSCtxSwCtr);                    /* Display #context switches per second      */
	PC_DispStr(18, 23, s, DISP_FGND_BLUE + DISP_BGND_CYAN);
	OSCtxSwCtr = 0;

	sprintf(s, "V%3.2f", (float)OSVersion() * 0.01);   /* Display version number as Vx.yy          */
	PC_DispStr(75, 24, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);
	PC_GetDateTime(s);                                 /* Get and display date and time            */
	PC_DispStr(0, 24, s, DISP_FGND_BLUE + DISP_BGND_CYAN);
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                              Display Status
*********************************************************************************************************
*/
void DisplayTitle ( void )
{
	PC_DispStr(20,  0, "uC/OS-II, The Real-Time Kernel's Shell", DISP_FGND_WHITE + DISP_BGND_RED + DISP_BLINK);
	PC_DispStr(30,  1, "Yu Ren on 2004/03/20", DISP_FGND_WHITE);

	PC_DispStr( 0, 22, "#Tasks          : xxxxx  CPU Usage: xxx %", DISP_FGND_WHITE);
	PC_DispStr( 0, 23, "#Task switch/sec: xxxxx", DISP_FGND_WHITE);
	PC_DispStr(28, 24, "<-PRESS 'ESC' TO QUIT->", DISP_FGND_WHITE + DISP_BLINK);
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                              STARTUP TASK
*********************************************************************************************************
*/
void TaskStart ( void *data )
{
	UBYTE  i;
	char   sCmd[30];
	WORD   key;
	WORD   index, pos = 0x00;

	data = data;                                           /* Prevent compiler warning                 */

	OS_ENTER_CRITICAL();
	PC_VectSet(0x08, OSTickISR);                           /* Install uC/OS-II's clock tick ISR        */
	PC_SetTickRate(OS_TICKS_PER_SEC);                      /* Reprogram tick rate                      */
	OS_EXIT_CRITICAL();

	PC_DispStr( 8, 16, "Determining  CPU's capacity ...", DISP_FGND_WHITE);
	OSStatInit();                                          /* Initialize uC/OS-II's statistics         */
	PC_DispClrLine(16, DISP_FGND_WHITE + DISP_BGND_BLACK);

	for (i = 0; i < N_TASKS; i++)
	{                                                      /* Create N_TASKS identical tasks           */
		TaskData[i] = '0' + i;                             /* Each task will display its own letter    */
		OSTaskCreate(Task, (void *)&TaskData[i], (void *)&TaskStk[i][TASK_STK_SIZE - 1], i + 1);
	}

	DisplayTitle();

	ClearMessageWindow();
	memset(sCmd, 0x00, 30);
	DisplayPrompt();
	for (;;)
	{
		DisplayStatus();                                   // display status of tasks

		if (PC_GetKey(&key) == TRUE)
		{                                                  /* See if key has been pressed              */
			if (key == 0x1B)
			{                                              /* Yes, see if it's the ESCAPE key          */
				PC_DOSReturn();                            /* Return to DOS                            */
			}
			else if (key == 0x08)                           // backspace
			{
				pos --;
				pos = pos <= 0x00 ? 0x00 : pos;
				sCmd[pos] = '\0';
				PC_DispChar(pos + strlen(sCmdPrompt), CurY, ' ', DISP_FGND_WHITE + DISP_BGND_BLUE);
			}
			else if (key == 0x0d || key == 0x0a)                           // enter
			{
				if (pos)
				{
					index = ParseCommand(sCmd);
					HandleCommand(index);
				}
				memset(sCmd, 0x00, 30);
				pos = 0x00;
				CurY ++;
				if (CurY == EndY - 1)
					ClearMessageWindow();
				DisplayPrompt();
			}
			else if ((key >= '0' && key <= '9') ||
				(key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z'))
			{
				if (pos < 30)
				{
					sCmd[pos] = (char)key;
					PC_DispChar(pos + strlen(sCmdPrompt), CurY, key, DISP_FGND_WHITE + DISP_BGND_BLUE);
				}
				pos ++;
			}

		}
		OSTimeDlyHMSM(0, 0, 0, 200);                         /* Wait 200 ms                          */
	}
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                                  TASKS
*********************************************************************************************************
*/

void Task (void *data)
{
	UBYTE x;
	UBYTE y;
	UBYTE err;


	for (;;)
	{
		OSSemPend(RandomSem, 0, &err);           /* Acquire semaphore to perform random numbers        */
		x = random(80);                          /* Find X position where task number will appear      */
		y = random(12);                          /* Find Y position where task number will appear      */
		OSSemPost(RandomSem);                    /* Release semaphore                                  */
												 /* Display the task number on the screen              */
		PC_DispChar(x, y + 3, *(char *)data, DISP_FGND_LIGHT_GRAY);
		OSTimeDly(1);                            /* Delay 1 clock tick                                 */
	}
}
