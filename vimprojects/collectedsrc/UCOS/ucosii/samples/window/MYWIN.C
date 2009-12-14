////////////////////////////////////////////////////////////////////////////////////////////
//
//    
//    File: mywin.c
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

#define TASK_STK_SIZE   1024      // Size of each task's stacks (# of WORDs)
#define N_TASKS         3         // Number of identical tasks
#define sCmdPrompt      ">\0"

OS_STK TaskStk[N_TASKS][TASK_STK_SIZE];     /* Tasks stacks                                  */
OS_STK TaskStartStk[TASK_STK_SIZE];

static WORD StartY = 16;
static WORD EndY = 21;
static WORD CurY = 16;
static char sLine[82] = "\0";

void TaskClock ( void * data );            /* Function prototypes of tasks                  */
void TaskStart ( void * data );            /* Function prototypes of Startup task           */


void main ( void )
{
	PC_DispClrScr(DISP_FGND_WHITE + DISP_BGND_BLACK); 	   /* Clear the screen    */

	OSInit();                                              /* Initialize uC/OS-II                      */

	PC_DOSSaveReturn();                                    /* Save environment to return to DOS        */
	PC_VectSet(uCOS, OSCtxSw);                             /* Install uC/OS-II's context switch vector */

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

void DisplayStatus ( void )
{
	char   s[81];

	sprintf(s, "%5d", OSTaskCtr);                     /* Display #tasks running                    */
	PC_DispStr(18, 22, s, DISP_FGND_BLUE + DISP_BGND_CYAN);
	sprintf(s, "%3d", OSCPUUsage);                    /* Display CPU usage in %                    */
	PC_DispStr(36, 22, s, DISP_FGND_BLUE + DISP_BGND_CYAN);
	sprintf(s, "%5d", OSCtxSwCtr);                    /* Display #context switches per second      */
	PC_DispStr(18, 23, s, DISP_FGND_BLUE + DISP_BGND_CYAN);
	OSCtxSwCtr = 0;
}

void DisplayTitle ( void )
{
	PC_DispStr(20,  0, "uC/OS-II, The Real-Time Kernel's Shell", DISP_FGND_WHITE + DISP_BGND_RED + DISP_BLINK);
	PC_DispStr(30,  1, "Yu Ren on 2005/09/10", DISP_FGND_WHITE);

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

//  PC_DispChar(0, 0, ' ', 0x00);
//	PC_DispStr (8, 16, "Determining  CPU's capacity ...", DISP_FGND_WHITE);
//	CreateWindow (0, StartY, MAX_X, EndY, DISP_FGND_WHITE, DISP_BGND_BLUE, "[ Prompt Window ]");
	CreateNoteWindows(ptr, 10, 10, 70, 20, "Wordpad\0");
	OSStatInit();                                          /* Initialize uC/OS-II's statistics         */

	OSTaskCreate(TaskClock, (void *)0, (void *)&TaskStk[0][TASK_STK_SIZE - 1], 1);

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
//					index = ParseCommand(sCmd);
//					HandleCommand(index);
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

void TaskClock ( void * data )
{
	char   str[30];

    data = data;

	sprintf(str, "V%3.2f", (float)OSVersion() * 0.01);   
	PC_DispStr(75, 24, str, DISP_FGND_YELLOW + DISP_BGND_BLUE);

	for (;;)
	{
	    PC_GetDateTime(str);   
	    PC_DispStr(0, 24, str, DISP_FGND_BLUE + DISP_BGND_CYAN);
		OSTimeDlyHMSM(0, 0, 1, 0);                         /* Wait 1 s                          */
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
