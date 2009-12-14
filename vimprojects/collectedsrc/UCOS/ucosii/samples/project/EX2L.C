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
#include "string.h"

/*
*********************************************************************************************************
*                                              CONSTANTS
*********************************************************************************************************
*/

#define          TASK_STK_SIZE     512                /* Size of each task's stacks (# of WORDs)       */

#define          TASK_START_ID       0                /* Application tasks IDs                         */
#define          TASK_1_ID           2
#define          TASK_2_ID           3

#define          TASK_START_PRIO    10                /* Application tasks priorities                  */
#define          TASK_1_PRIO        11
#define          TASK_2_PRIO        12

#define			 MSG_QUEUE_SIZE		20

#define  	 	 MAX_Y		    	30
#define  	 	 MAX_X  	   	    78

#define			 START_Y			 8
#define			 LEN_Y				14
#define			 START_X			 0

/*
*********************************************************************************************************
*                                              	 TYPES
*********************************************************************************************************
*/

struct MsgType {
	UBYTE		 TaskID;
	UBYTE		 Info;
};

/*
*********************************************************************************************************
*                                              VARIABLES
*********************************************************************************************************
*/

OS_STK           TaskStartStk[TASK_STK_SIZE];         /* Startup    task stack                         */
OS_STK           Task1Stk[TASK_STK_SIZE];             /* Task #1    task stack                         */
OS_STK           Task2Stk[TASK_STK_SIZE];             /* Task #2    task stack                         */

OS_EVENT        *MsgQueue;                            /* Message queue pointer                         */
void            *MsgQueueTbl[MSG_QUEUE_SIZE];         /* Storage for messages                          */

char		 	 ScrCache[MAX_Y][MAX_X + 1];
UBYTE 		 	 NowY;
INT16S			 PosY;

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void             TaskStart(void *data);               /* Function prototypes of tasks                  */
void             Task1(void *data);
void             Task2(void *data);

void		 	 Display(void);
void			 DispLine(char *s);
void   		 	 DispChar(UBYTE i, UBYTE j, char s);
void			 DispStr(UBYTE i, UBYTE j, char *s);
void			 DispSign(void);
void		 	 InitWindow(void);
void		 	 Move(UBYTE x);
void			 DisplaySign(UBYTE i, UBYTE j);
void			 CacheOver(void);
void			 Command(char *sCmd);
void			 CmdWork(UBYTE index, char *sPara);

void			 colputchar(char ch);
void			 colprintn(int n, const char* fmt );
void			 colprintf(float n, const char* fmt );
void			 colprint(const char*,...);

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
	OSTaskCreateExt(TaskStart, (void *)0, &TaskStartStk[TASK_STK_SIZE-1],
					TASK_START_PRIO, TASK_START_ID, &TaskStartStk[0], TASK_STK_SIZE,
					(void *)0, OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
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
	INT16S temp;
	INT16S pos;

	data = data;                                           /* Prevent compiler warning                 */

	PC_DispStr(26,  0, "uC/OS-II, The Real-Time Kernel", DISP_FGND_WHITE);
	PC_DispStr(33,  1, "Jean J. Labrosse", DISP_FGND_WHITE);
	PC_DispStr(36,  3, "SHELL TEST", DISP_FGND_WHITE);
	PC_DispStr(28, 24, "<-PRESS 'ESC' TO QUIT->", DISP_FGND_WHITE);

	OS_ENTER_CRITICAL();                                   /* Install uC/OS-II's clock tick ISR        */
	PC_VectSet(0x08, OSTickISR);
	PC_SetTickRate(OS_TICKS_PER_SEC);                      /* Reprogram tick rate                      */
	OS_EXIT_CRITICAL();
	OSStatInit();                                          /* Initialize uC/OS-II's statistics         */

	MsgQueue = OSQCreate(&MsgQueueTbl[0], MSG_QUEUE_SIZE); /* Create a message queue                   */

	OSTaskCreateExt(Task1, (void *)0, &Task1Stk[TASK_STK_SIZE-1],
					TASK_1_PRIO, TASK_1_ID, &Task1Stk[0], TASK_STK_SIZE,
					(void *)0, OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);

	OSTaskCreateExt(Task2, (void *)0, &Task2Stk[TASK_STK_SIZE-1],
					TASK_2_PRIO, TASK_2_ID, &Task2Stk[0], TASK_STK_SIZE,
					(void *)0, OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);

	InitWindow();

	pos = 1;
	temp = 0;

	for (;;) {

		if (PC_GetKey(&key)) {                             /* See if key has been pressed              */
			if (((key >= 'a' && key <= 'z') ||
				 (key >= 'A' && key <= 'Z') ||
				 (key >= '0' && key <= '9') ||
				 (key == 0x20)) && temp == 0) {
				if (PosY < NowY) {
					PC_DispChar(START_X + MAX_X + 1, ((LEN_Y - 2) * NowY -1)/
								(MAX_Y - LEN_Y) + START_Y + 1,	0xB0,
								DISP_FGND_WHITE + DISP_BGND_BLUE);

					NowY = PosY;
					Display();
					DispSign();
					PC_DispChar(START_X + MAX_X + 1, ((LEN_Y - 2) * NowY -1)/
								(MAX_Y - LEN_Y) + START_Y + 1, 0x12,
								DISP_FGND_BLACK + DISP_BGND_BLUE);
				}
				if (PosY >= NowY + LEN_Y) {
					PC_DispChar(START_X + MAX_X + 1, ((LEN_Y - 2) * NowY -1)/
								(MAX_Y - LEN_Y) + START_Y + 1, 0xB0,
								DISP_FGND_WHITE + DISP_BGND_BLUE);

					NowY = PosY - LEN_Y + 1;
					Display();
					DispSign();
					PC_DispChar(START_X + MAX_X + 1, ((LEN_Y - 2) * NowY -1)/
								(MAX_Y - LEN_Y) + START_Y + 1, 0x12,
								DISP_FGND_BLACK + DISP_BGND_BLUE);
				}
				if (pos < MAX_X - 1) {
					if (key >= 'A' && key <= 'Z') s[pos - 1] = key + 0x20;
					else s[pos - 1] = key;
					DispChar(pos, PosY, key);
					pos++;
					DisplaySign(pos, PosY);
				}
			}
			switch (key)
			{
				case 0x00:
					temp = 1;
					break;
				case 0x1B:  	                           /* Yes, see if it's the ESCAPE key          */
					PC_DOSReturn();                        /* Return to DOS                            */
					break;
				case 0x48:
					if (temp == 1) { Move(0x00); temp = 0; }
					DisplaySign(pos, PosY);
					break;
				case 0x50:
					if (temp == 1) { Move(0x01); temp = 0; }
					DisplaySign(pos, PosY);
					break;
				case 0x4B:
					if (temp == 1) temp = 0;
					break;
				case 0x4D:
					if (temp == 1) temp = 0;
					break;
				case 0x08:
					DispChar(pos, PosY, 0x20);
					pos--; pos = pos < 1 ? 1 : pos;
					DisplaySign(pos, PosY);
					break;
				case 0x0D:
					if (PosY < NowY + LEN_Y - 1) {
						if (PosY < MAX_Y - 1) PosY++;
						else CacheOver();
					}
					else
					{
						if (PosY < MAX_Y - 1) PosY++;
						else CacheOver();
						Move(0x01);
					}
					Display();
					s[pos - 1] = '\0';
					pos = 1;
					DispSign();
					DisplaySign(pos, PosY);
					if (s[0] != '\0') Command(s);
					break;
			}
		}
		OSTimeDly(20);
	}
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                               TASK #1
*********************************************************************************************************
*/

void  Task1 (void *data)
{
	char 	c;
	UBYTE	num;
	UBYTE	flag;
	UBYTE	speed;
	struct MsgType *RMsg;
	struct MsgType  PMsg;

	data = data;
	num	= 0;
	flag = 1;
	speed = 10;

	PC_DispStr(14, 5, "Test Object One:", DISP_FGND_WHITE);

	for (;;) {
		RMsg = (struct MsgType *)OSQAccept(MsgQueue);
		if (RMsg != NULL) {
			if (RMsg->TaskID != 0x01) {
				OSQPostFront(MsgQueue,(void *)RMsg);
			}
			else {
				flag = (RMsg->Info >= 0x80);
				speed = 1000 / (RMsg->Info & 0x7F);
				PMsg.TaskID = 0x00;
				PMsg.Info = 0x01;
				OSQPost(MsgQueue,(void *)&PMsg);
			}
		}
		switch(num) {
			case 0: c = '|'; break;
			case 1: c = '/'; break;
			case 2: c = '-'; break;
			case 3: c = '\\'; break;
		}
		PC_DispChar(30, 5, c,  DISP_FGND_WHITE + DISP_BGND_RED);
		if (flag != 0) if (num != 3) num++; else num = 0;
		else if (num != 0) num--; else num = 3;
		OSTimeDly(speed);
	}
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                               TASK #2
*********************************************************************************************************
*/

void  Task2 (void *data)
{
	char 	c;
	UBYTE	num;
	UBYTE	flag;
	UBYTE	speed;
	struct MsgType *RMsg;
	struct MsgType  PMsg;

	data = data;
	num	= 0;
	flag = 0;
	speed = 20;

	PC_DispStr(47, 5, "Test Object Two:", DISP_FGND_WHITE);

	for (;;) {
		RMsg = (struct MsgType *)OSQAccept(MsgQueue);
		if (RMsg != NULL) {
			if (RMsg->TaskID != 0x02) {
				OSQPostFront(MsgQueue,(void *)RMsg);
			}
			else {
				flag = (RMsg->Info >= 0x80);
				speed = 1000 / (RMsg->Info & 0x7F);
				PMsg.TaskID = 0x00;
				PMsg.Info = 0x02;
				OSQPost(MsgQueue,(void *)&PMsg);
			}
		}
		switch(num) {
			case 0: c = '|'; break;
			case 1: c = '/'; break;
			case 2: c = '-'; break;
			case 3: c = '\\'; break;
		}
		PC_DispChar(63, 5, c,  DISP_FGND_WHITE + DISP_BGND_BLUE);
		if (flag != 0) if (num != 3) num++; else num = 0;
		else if (num != 0) num--; else num = 3;
		OSTimeDly(speed);
	}
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                              Display()
*********************************************************************************************************
*/

void Display(void)
{
	UBYTE i;

	for (i = 0; i < LEN_Y; i++)
		PC_DispStr(START_X + 1, i + START_Y, ScrCache[i + NowY],
				   DISP_FGND_WHITE + DISP_BGND_BLUE);
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                              DispChar()
*********************************************************************************************************
*/

void   DispChar(UBYTE i,UBYTE j,char s)
{
	ScrCache[j][i] = s;
	if (j >= NowY && j < NowY + LEN_Y)
		PC_DispChar(START_X + i + 1, j + START_Y - NowY, s,
					DISP_FGND_WHITE + DISP_BGND_BLUE);
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                             InitWindow()
*********************************************************************************************************
*/

void   InitWindow(void)
{
	UBYTE  i;
	char   s[MAX_X + 3];

	NowY = 0;

	for (i = 0; i < MAX_Y; i++) {
		memset(ScrCache[i], 0x20, MAX_X);
		ScrCache[i][MAX_X] = '\0';
	}

	memset(s, 0xCD, MAX_X + 2);
	s[0] = 0xC9; s[MAX_X + 1] = 0xBB; s[MAX_X + 2] = '\0';
	PC_DispStr(START_X, START_Y - 1, s, DISP_FGND_WHITE + DISP_BGND_BLUE);

	for (i = 0; i < LEN_Y; i++) {
		memset(s, 0x20, MAX_X + 2);
		s[0] = 0xBA; s[MAX_X + 1] = 0xB0; s[MAX_X + 2] = '\0';
		PC_DispStr(START_X, i + START_Y, s, DISP_FGND_WHITE + DISP_BGND_BLUE);
	}

	memset(s, 0xCD, MAX_X + 2);
	s[0] = 0xC8; s[MAX_X + 1] = 0xBC; s[MAX_X + 2] = '\0';
	PC_DispStr(START_X, START_Y + LEN_Y, s, DISP_FGND_WHITE + DISP_BGND_BLUE);

	PC_DispChar(START_X + MAX_X + 1, START_Y, 0x1E,
				DISP_FGND_WHITE + DISP_BGND_BLUE);

	PC_DispChar(START_X + MAX_X + 1, START_Y + LEN_Y - 1, 0x1F,
				DISP_FGND_WHITE + DISP_BGND_BLUE);

	PC_DispChar(START_X + MAX_X + 1, START_Y + 1, 0x12,
				DISP_FGND_BLACK + DISP_BGND_BLUE);

	PC_DispStr(31, START_Y - 1, "[ Window  Test ]",
			   DISP_FGND_WHITE + DISP_BGND_BLUE);

	PosY = 0;
	DispSign();
	DisplaySign(1, PosY);

}
/*$PAGE*/
/*
*********************************************************************************************************
*                                            	Move()
*********************************************************************************************************
*/

void   Move(UBYTE x)
{
	PC_DispChar(START_X + MAX_X + 1, ((LEN_Y - 2) * NowY -1)/
				(MAX_Y - LEN_Y) + START_Y + 1, 0xB0,
				DISP_FGND_WHITE + DISP_BGND_BLUE);

	switch(x)
	{
		case 0x00:
			if (NowY != 0) NowY--;
			break;
		case 0x01:
			if (NowY != MAX_Y - LEN_Y) NowY++;
			break;
	}
	PC_DispChar(START_X + MAX_X + 1, ((LEN_Y - 2) * NowY -1)/
				(MAX_Y - LEN_Y) + START_Y + 1, 0x12,
				DISP_FGND_BLACK + DISP_BGND_BLUE);

	Display();
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                            DisplaySign()
*********************************************************************************************************
*/

void DisplaySign(UBYTE i, UBYTE j)
{
	if (j >= NowY && j < NowY + LEN_Y)
		PC_DispChar(START_X + i + 1, START_Y + j - NowY, '_',
					DISP_FGND_WHITE + DISP_BGND_BLUE + DISP_BLINK);
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                            CacheOver()
*********************************************************************************************************
*/

void CacheOver(void)
{
	UBYTE i,j;
	for(i = 0; i < MAX_Y; i++)
		for(j = 0; j < MAX_X - 1; j++)
			ScrCache[i][j] = ScrCache[i + 1][j];

	memset(ScrCache[MAX_Y - 1], 0x20, MAX_X);
	ScrCache[MAX_Y - 1][MAX_X] = '\0';
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                             DispLine()
*********************************************************************************************************
*/

void DispLine(char *s)
{
	DispStr(0, PosY, s);

	if (PosY < NowY + LEN_Y - 1) {
		if (PosY < MAX_Y - 1) PosY++;
		else CacheOver();
	}
	else {
		if (PosY < MAX_Y - 1) PosY++;
		else CacheOver();
		Move(0x01);
	}
	DispSign();
	DisplaySign(1, PosY);
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                              DispStr()
*********************************************************************************************************
*/

void DispStr(UBYTE i, UBYTE j, char *s)
{
	UBYTE x;

	x = 0;
	while(s[x] != '\0' && (x + i) < (MAX_X - 1)) {
		ScrCache[j][i + x] = s[x];
		x++;
	}
	PC_DispStr(START_X + 1, j - NowY + START_Y, ScrCache[j],
			   DISP_FGND_WHITE + DISP_BGND_BLUE);
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                              DispSign()
*********************************************************************************************************
*/

void DispSign(void)
{
	DispChar(0, PosY, '-');
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                              Command()
*********************************************************************************************************
*/

void Command(char *sCmd)
{
	if (strstr(sCmd, "ver") == sCmd) CmdWork(0x01, &sCmd[3]);
	else if (strstr(sCmd, "help") == sCmd) CmdWork(0x02, &sCmd[4]);
	else if	(strstr(sCmd, "exit") == sCmd) CmdWork(0x03, &sCmd[4]);
	else if (strstr(sCmd, "change") == sCmd) CmdWork(0x04, &sCmd[6]);
	else if (strstr(sCmd, "start") == sCmd) CmdWork(0x05, &sCmd[5]);
	else if (strstr(sCmd, "stop") == sCmd) CmdWork(0x06, &sCmd[4]);
	else DispLine("Bad command!");
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                              CmdWork()
*********************************************************************************************************
*/

void CmdWork(UBYTE index, char *sPara)
{
	struct MsgType  PMsg;
	struct MsgType *RMsg;
	INT8U 		    err;

	if (sPara[0] != '\0' && sPara[0] != 0x20) DispLine("Bad command!");
	else {
		switch(index) {
			case 0x01:
				if (sPara[0] == '\0') colprint("uC/OS-II SHELL TEST V%3.2f",1.00);
				else DispLine("No parameter!");
				break;
			case 0x02:
				if (sPara[0] == '\0') {
					colprint("SHELL HELP:");
					colprint("Format: ver");
					colprint("This Command will show the version of the system.");
					colprint("Format: help");
					colprint("This Command will show the help.");
					colprint("Format: exit");
					colprint("This Command will close the system and return to DOS.");
					colprint("Format: change <TaskID> <Direction> <Speed>");
					colprint("This Command will change the state of the task.");
					colprint("Format: start <TaskID>");
					colprint("This Command will let the task work.");
					colprint("Format: stop <TaskID>");
					colprint("This Command will stop the task.");
				}
				else colprint("No parameter!");
				break;
			case 0x03:
				if (sPara[0] == '\0') PC_DOSReturn();
				else colprint("No parameter!");
				break;
			case 0x04:
				if (sPara[0] == '\0') colprint("Format: change <TaskID> <Direction> <Speed>");
				else {
					if (sPara[1] != '1' && sPara[1] != '2')
						PMsg.TaskID = 0x00;
					else {
						if (sPara[1] == '1') PMsg.TaskID = 0x01;
						if (sPara[1] == '2') PMsg.TaskID = 0x02;
						if (sPara[2] != 0x20) PMsg.Info = 0x00;
						else {
							if (sPara[3] != '0' && sPara[3] != '1')
								PMsg.Info = 0x00;
							else {
								if (sPara[3] == '0') PMsg.Info = 0x00;
								if (sPara[3] == '1') PMsg.Info = 0x80;
								if (sPara[4] != 0x20) PMsg.Info = 0x00;
								else {
									UBYTE i = 5,t = 0;
									while(sPara[i] >= '0' && sPara[i] <= '9') {
										t = t * 10 + sPara[i] - 0x30;
										i++;
									}
									if (sPara[i] == '\0' && t > 0 && t < 128) {
										PMsg.Info += t;
									}
									else PMsg.Info = 0x00;
								}
							}
						}
					}
					if (PMsg.TaskID != 0x00 && PMsg.Info != 0x00) {
						OSQPost(MsgQueue,(void *)&PMsg);
						RMsg = (struct MsgType *)OSQPend(MsgQueue, 0, &err);
						while(RMsg == NULL || RMsg->TaskID != 0x00) {
							if (RMsg->TaskID != 0x00 )
								OSQPostFront(MsgQueue,(void *)RMsg);
							OSTimeDly(10);
							RMsg = (struct MsgType *)OSQPend(MsgQueue, 0, &err);
						}
						if (err == OS_NO_ERR)
							colprint("Task%d State Change Finished!",RMsg->Info);
					}
					else colprint("Wrong parameter!");
				}
				break;
			case 0x05:
				if (sPara[0] == '\0') colprint("Format: start <TaskID>");
				else {
					if (sPara[1] == '1' && sPara[2] == '\0')
						OSTaskResume(TASK_1_PRIO);
					else if (sPara[1] == '2' && sPara[2] == '\0')
						OSTaskResume(TASK_2_PRIO);
					else colprint("Wrong parameter!");
				}
				break;
			case 0x06:
				if (sPara[0] == '\0') colprint("Format: stop <TaskID>");
				else {
					if (sPara[1] == '1' && sPara[2] == '\0')
						OSTaskSuspend(TASK_1_PRIO);
					else if (sPara[1] == '2' && sPara[2] == '\0')
						OSTaskSuspend(TASK_2_PRIO);
					else colprint("Wrong parameter!");
				}
				break;
		}
	}
}

void	colputchar(char ch)
{
	static UBYTE PosX = 0;

	if (ch == '\n' || ch == '\0') {
		if (PosY < NowY + LEN_Y - 1) {
			if (PosY < MAX_Y - 1) PosY++;
			else CacheOver();
		}
		else {
			if (PosY < MAX_Y - 1) PosY++;
			else CacheOver();
			Move(0x01);
		}
		if (ch == '\0') {
			DispSign();
			DisplaySign(1, PosY);
		}
		PosX = 0;
	}
	else {
		DispChar(PosX, PosY, ch);
		PosX++;
	}
}

void	colprintn(int n, const char* fmt )
{
	char buf[50], c;
	int i = 0;
	sprintf( buf, fmt, n);
	while(i<50)
	{
		c=buf[i];
		if (c=='\0') return;
		colputchar(c);
		i++;
	}

}
void	colprintf(float n, const char* fmt)
{
	char buf[50], c;
	int i = 0;
	sprintf( buf, fmt, n);
	while(i<50)
	{
		c=buf[i];
		if (c=='\0') return;
		colputchar(c);
		i++;
	}
}

void	colprint(const char* fmt,...)
{
	register char *s, *n, *p, par[8];
	register *adx, c, cn;

	adx = &fmt+1;			//point to first param
loop:
	while((c = *fmt++) != '%' )
	{
		colputchar(c);
		if(c=='\0') return;
	}
	c = *fmt++;
	if( c=='d')
	{
		colprintn(*(int*)adx, "%d" );
		adx+=1;
	}
	else if( c=='f')
	{
		colprintf(*(double*)adx, "%f" );
		adx+=4;
	}
	else if( c=='s')
	{
		s=*(char**)adx;
		while(c=*s++)
		{
			if( c=='\0') break;
			colputchar(c);
		}
		adx+=2;
	}
	else if( c>=48 && c<58 || c=='.' )	//¸ñÊ½Êä³ö %3.2f %3f %.3f %3.f
	{
		n = fmt-2;
		memcpy( par, n, 8 );

		if ( (p=strstr(par,"d")) != NULL )
		{
			*(p+1) = '\0';
			colprintn( *(int*)adx, par );
			fmt +=p-par-1;
			adx+=1;
		}
		
		if ( (p=strstr(par,"f")) != NULL )
		{
			*(p+1) = '\0';
			colprintf( *(double*)adx, par );
			fmt +=p-par-1;
			adx+=4;
		}
	}
	else
	{
		colputchar('%');
		fmt--;
	}

	goto loop;
}
