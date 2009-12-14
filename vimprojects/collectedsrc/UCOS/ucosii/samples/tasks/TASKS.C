////////////////////////////////////////////////////////////////////////////////////////////
//
//    
//    File: tasks.c
//
//    Copyright (C): 2005 Searen Network Software Ltd.
//
//    [ This source code is the sole property of Searen Network Software Ltd.  ]
//    [ All rights reserved.  No part of this source code may be reproduced in ]
//    [ any form or by any electronic or mechanical means, including informa-  ]
//    [ tion storage and retrieval system, without the prior written permission]
//    [ of Searen Network Software Ltd.                                        ]
//    [                                                                        ]
//    [   For use by authorized Searen Network Software Ltd. employees only.   ]
//
//    Description:   This class can read, write and watch one serial port.
//					 It sends messages to its owner when something happends on the port
//					 The class creates a thread for reading and writing so the main
//					 program is not blocked.
// 
//
//
//  AUTHOR: Ren Yu.
//  DATE: Sept. 16, 2005
//
//
/////////////////////////////////////////////////////////////////////////////////////////////
//

#include "includes.h"
#include "windows.h"
#include "printf.h"

#include <string.h>

#define TASK_STK_SIZE   1024     /* Size of each task's stacks (# of WORDs)            */
#define N_TASKS         10       /* Number of identical tasks                          */

OS_STK           TaskStk[N_TASKS][TASK_STK_SIZE];     /* Tasks stacks                                  */
OS_STK           TaskStartStk[TASK_STK_SIZE];
char             TaskData[N_TASKS];                   /* Parameters to pass to each task               */
OS_EVENT      *  RandomSem;

static char sCmdPrompt[10] = "\\>\0";            // shell command prompt
static char sRunning[10];
static WORD StartY = 16;
static WORD EndY = 21;
static WORD CurY = 16;
static char sLine[82] = "\0";

void Task      ( void * data );            /* Function prototypes of tasks                  */
void TaskStart ( void * data );            /* Function prototypes of Startup task           */


void main ( void )
{
	memset(sRunning, 0x01, 10);                            // flag all task will run at kernel starting

	PC_DispClrScr(DISP_FGND_WHITE + DISP_BGND_BLACK); 	   /* Clear the screen                         */
	OSInit();                                              /* Initialize uC/OS-II                      */
	PC_DOSSaveReturn();                                    /* Save environment to return to DOS        */
	PC_VectSet(uCOS, OSCtxSw);                             /* Install uC/OS-II's context switch vector */
	RandomSem = OSSemCreate(1);                            /* Random number semaphore                  */
	OSTaskCreate(TaskStart, (void *)0, (void *)&TaskStartStk[TASK_STK_SIZE - 1], 0);
	OSStart();                                             /* Start multitasking                       */
}

void DisplayCursor ( int x, int y )
{
	PWINDOWS ptr = GetActiveWindow();

	PC_DispChar(x, y, '_', ptr->color | 0x80);
}

void DisplayPrompt ( void )
{
	PWINDOWS ptr = GetActiveWindow();

	PC_DispStr(1, ptr->cy, sCmdPrompt, ptr->color);
	DisplayCursor (strlen(sCmdPrompt) + 1, ptr->cy);
}

void DisplayMessage ( char *sMsg )
{
	PWINDOWS ptr = GetActiveWindow();

	memset(sLine, 0x20, ptr->ex - ptr->sx);
	sLine[ptr->ex - ptr->sx] = '\0';

	PC_DispStr(1, EndY - 1, sLine, ptr->color);
//	PC_DispClrLine(EndY - 1, DISP_FGND_WHITE + DISP_BGND_BLUE);
	PC_DispStr(1, EndY - 1, sMsg, ptr->color);
}

void DoHelp ( void )
{
	char buffer[82];
	PWINDOWS ptr = GetActiveWindow();

	ClearWindow();
	memset(buffer, 0x00, 82);
	sprintf(buffer, "StartAll -- resume all tasks");
	PC_DispStr(ptr->cx, ptr->cy, buffer, ptr->color);
	ptr->cy ++;
	sprintf(buffer, "StopAll -- suspend all tasks");
	PC_DispStr(ptr->cx, ptr->cy, buffer, ptr->color);

//	printf("Test printf functions!\n");
}

void DoAbout ( void )
{
}

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
	else if (strcmp(strCmd, "task10") == 0x00)
		return 0x0a;
	else if (strcmp(strCmd, "startall") == 0x00)
		return 0x101;
	else if (strcmp(strCmd, "stopall") == 0x00)
		return 0x102;
	else if (strcmp(strCmd, "help") == 0x00)
		return 0x201;
	else if (strcmp(strCmd, "about") == 0x00)
		return 0x202;

	return 0x00;
}

void StartAllTasks ( void )
{
	int i;

	for (i = 0x00; i < N_TASKS; i ++)
	{
		if (!sRunning[i])
		{
			OSTaskResume(i + 1);
			sRunning[i] = 0x01;
		}
	}
}

void StopAllTasks ( void )
{
	int i;

	for (i = 0x00; i < N_TASKS; i ++)
	{
		if (sRunning[i])
		{
			OSTaskSuspend(i + 1);
			sRunning[i] = 0x00;
		}
	}
}

int HandleCommand ( WORD index )
{
	char   sMsg[80];

	if (index > 0x100 && index < 0x200)
	{
		switch(index)
		{
			case 0x101:
				DisplayMessage("Start all tasks just now ...");
				StartAllTasks();   break;
			case 0x102:
				DisplayMessage("Stop all tasks just now ...");
				StopAllTasks();    break;
		}
	}
	else if (index > 0x00 && index <= 0x0a)
	{
		if (sRunning[index - 1])
		{
			sprintf(sMsg, "Suspend task%d just now ...", index);
			DisplayMessage(sMsg);
			OSTaskSuspend(index);
			sRunning[index - 1] = 0x00;
		}
		else
		{
			sprintf(sMsg, "Resume task%d just now ...", index);
			DisplayMessage(sMsg);
			OSTaskResume(index);
			sRunning[index - 1] = 0x01;
		}
	}
	else if (index > 0x200)
	{
		if (index == 0x201)
			DoHelp();
		else if (index == 0x202)
			DoAbout();

	}

	return 0x00;
}

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

void DisplayTitle ( void )
{
	PC_DispStr(20,  0, "uC/OS-II, The Real-Time Kernel's Shell", DISP_FGND_WHITE + DISP_BGND_RED + DISP_BLINK);
	PC_DispStr(30,  1, "Yu Ren on 2005/09/20", DISP_FGND_WHITE);

	PC_DispStr( 0, 22, "#Tasks          : xxxxx  CPU Usage: xxx %", DISP_FGND_WHITE);
	PC_DispStr( 0, 23, "#Task switch/sec: xxxxx", DISP_FGND_WHITE);
	PC_DispStr(28, 24, "<-PRESS 'ESC' TO QUIT->", DISP_FGND_WHITE + DISP_BLINK);
}

void TaskStart ( void *data )
{
	PWINDOWS ptr = GetActiveWindow();
	UBYTE  i;
	char   sCmd[MAX_X];
	WORD   key;
	WORD   index, pos = 0x00;

	data = data;                                           /* Prevent compiler warning                 */

	OS_ENTER_CRITICAL();
	PC_VectSet(0x08, OSTickISR);                           /* Install uC/OS-II's clock tick ISR        */
	PC_SetTickRate(OS_TICKS_PER_SEC);                      /* Reprogram tick rate                      */
	OS_EXIT_CRITICAL();

    PC_DispChar(0, 0, ' ', 0x00);               
	PC_DispStr (8, 16, "Determining  CPU's capacity ...", DISP_FGND_WHITE);
	CreateWindow (0, StartY, MAX_X, EndY, DISP_FGND_WHITE, DISP_BGND_BLUE, "[ Prompt Window ]");
	CreateTaskWindows();
	OSStatInit();                                          /* Initialize uC/OS-II's statistics         */

	for (i = 0; i < N_TASKS; i ++)
	{                                                      /* Create N_TASKS identical tasks           */
		TaskData[i] = '0' + i;                             /* Each task will display its own letter    */
		OSTaskCreate(Task, (void *)&TaskData[i], (void *)&TaskStk[i][TASK_STK_SIZE - 1], i + 1);
	}

	DisplayTitle();

	ClearWindow();
	memset(sCmd, 0x00, MAX_X);
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
				PC_DispChar(ptr->cx + pos + strlen(sCmdPrompt), ptr->cy, ' ', ptr->color);
				DisplayCursor(ptr->cx + pos + strlen(sCmdPrompt), ptr->cy);
			}
			else if (key == 0x0d || key == 0x0a)                           // enter
			{
				if (pos)
				{
					index = ParseCommand(sCmd);
					HandleCommand(index);
				}
				memset(sCmd, 0x00, MAX_X);
				pos = 0x00;
				ptr->cy ++;
				if (ptr->cy == ptr->ey - 1)
					ClearWindow();
				DisplayPrompt();
			}
			else if ((key >= '0' && key <= '9') ||
				(key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z'))
			{
				if (pos < MAX_X)
				{
					sCmd[pos] = (char)key;
					PC_DispChar(ptr->cx + pos + strlen(sCmdPrompt), ptr->cy, key, ptr->color);
	                DisplayCursor (ptr->cx + pos + strlen(sCmdPrompt) + 1, ptr->cy);
				}
				pos ++;
			}

		}
		OSTimeDlyHMSM(0, 0, 0, 200);                         /* Wait 200 ms                          */
	}
}

void Task ( void * data )
{
	UBYTE x;
	UBYTE y;
	UBYTE err;
	int   curwin;
	PWINDOWS ptr;
	int   i, full;


	for (;;)
	{
		OSSemPend(RandomSem, 0, &err);           /* Acquire semaphore to perform random numbers        */
		curwin = GetCurrentWin();
		SetCurrentWin(*(char *)data - 0x2f);
		ptr = GetActiveWindow();
		x = random(ptr->ex - ptr->sx);                          /* Find X position where task number will appear      */
		y = random(ptr->ey - ptr->sy);                          /* Find Y position where task number will appear      */

		ptr->cache[x + y * (ptr->ex - ptr->sx)] = *(char *)data;
		for (full = 1, i = 0; i < (ptr->ex - ptr->sx) * (ptr->ey - ptr->sy); i ++)
		{
			if (ptr->cache[i] != *(char *)data)
			{
				full = 0; break;
			}
		}
		if (full)
			ClearWindow();

		PC_DispChar(ptr->sx + x + 1, ptr->sy + y, 'o', ptr->color);  // *(char *)data, ptr->color);
		SetCurrentWin(curwin);
		OSSemPost(RandomSem);                    /* Release semaphore                 */
		OSTimeDly(1);                            /* Delay 1 clock tick                                 */
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
