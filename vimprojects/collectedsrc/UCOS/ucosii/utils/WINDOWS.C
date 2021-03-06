////////////////////////////////////////////////////////////////////////////////////////////
//
//    
//    File: windows.c
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
//  DATE: Sept. 12, 2005
//
//
/////////////////////////////////////////////////////////////////////////////////////////////
//
#include "includes.h"
#include "pc.h"
#include "windows.h"

#include <string.h>

#define MAX_CHARS   82

char   sLine[MAX_CHARS];
static WINDOWS winItem[MAX_WIN];
static int wincnt = 0x00;
static int winact = 0x00;

/////////////////////////////////////////////////////////////////////////////////////////////
//
int GetCurrentWin ( void )
{
	return winact;
}

void SetCurrentWin ( int curwin )
{
	winact = curwin;
}

PWINDOWS GetActiveWindow ( void )
{
	return &winItem[winact];
}

PWINDOWS CreateWindow ( int sx, int sy, int ex, int ey, int color, int bkcolor, char *title )
{
	int  i, width, high;

	if (sx < START_X || sy < START_Y || ex > MAX_X || ey > MAX_Y || wincnt >= MAX_WIN)
		return NULL;

	winItem[wincnt].sx      = sx;
	winItem[wincnt].sy      = sy;
	winItem[wincnt].cx      = sx + 1;
	winItem[wincnt].cy      = sy;
	winItem[wincnt].ex      = ex;
	winItem[wincnt].ey      = ey;
	winItem[wincnt].color   = color | bkcolor;
	strcpy(winItem[wincnt].title, title);

	high  = ey - sy;
	width = ex - sx;

	memset(sLine, 0x00, MAX_CHARS);

	memset(winItem[wincnt].cache, 0x20, width * high);

	memset(sLine, 0xCD, width + 2);
	sLine[0] = 0xC9; sLine[width + 1] = 0xBB;
	PC_DispStr(sx, sy - 1, sLine, winItem[wincnt].color);

	for (i = 0; i < high; i ++)
	{
		memset(sLine, 0x20, width + 2);
		sLine[0] = 0xBA; sLine[width + 1] = 0xB0;
		PC_DispStr(sx, i + sy, sLine, winItem[wincnt].color);
	}

	memset(sLine, 0xCD, width + 2);
	sLine[0] = 0xC8; sLine[width + 1] = 0xBC;
	PC_DispStr(sx, sy + high, sLine, winItem[wincnt].color);

	PC_DispChar(sx + width + 1, sy, 0x1E, winItem[wincnt].color);
	PC_DispChar(sx + width + 1, sy + high - 0x01, 0x1F, winItem[wincnt].color);
	PC_DispChar(sx + width + 1, sy + 1, 0x12, winItem[wincnt].color);
	PC_DispStr(sx + (width / 2) - (strlen(winItem[wincnt].title) / 2) + 1, sy - 1,
					winItem[wincnt].title, winItem[wincnt].color);

	return &winItem[wincnt ++];
}

void CreateTaskWindows ( void )
{
	int  i;
	char title[10];

	for (i = 0; i < 10; i ++)
	{
		sprintf(title, "%d\0", i + 1);
		CreateWindow (8 * i, 3, 8 * i + 6, 14, DISP_FGND_BLACK, DISP_BGND_LIGHT_GRAY, title);
	}
}

void CreateNoteWindows ( PWINDOWS ptr, INT8U sx, INT8U sy, INT8U ex, INT8U ey, char * title )
{
	CreateWindow (sx, sy, ex, ey, DISP_FGND_BLACK, DISP_BGND_LIGHT_GRAY, title);
}

void ClearWindow ( void )
{
	PWINDOWS ptr = GetActiveWindow();
	int      i;

	memset(sLine, 0x00, MAX_CHARS);
	memset(sLine, 0x20, ptr->ex - ptr->sx);
	sLine[ptr->ex - ptr->sx] = '\0';

	for (i = ptr->sy; i < ptr->ey; i ++)
		PC_DispStr(ptr->sx + 1, i, sLine, ptr->color);

	ptr->cy = ptr->sy;
}

void SaveWindow ( INT8U sx, INT8U sy, INT8U ex, INT8U ey, char far * buffer )
{
    INT8U far *pscr;
    INT8U      i, j;
	INT16U     pos;

    for (i = sy; i < ey; i++) 
	{
		for (j = sx; j < ey; j ++)
		{
			pos = j * i * 2;
			pscr = MK_FP(DISP_BASE, pos);
			*buffer++ = *pscr++;
            *buffer++ = *pscr;  
		}
    }
}

void BackWindow ( INT8U sx, INT8U sy, INT8U ex, INT8U ey, char far * buffer )
{
    INT8U far *pscr;
    INT8U      i, j;
	INT16U     pos;

    for (i = sy; i < ey; i++) 
	{
		for (j = sx; j < ey; j ++)
		{
			pos = j * i * 2;
            pscr = MK_FP(DISP_BASE, pos);
            *pscr++ = *buffer++; 
            *pscr   = *buffer++;  
		}
    }
}

INT16U GetWinBufferSize (  INT8U sx, INT8U sy, INT8U ex, INT8U ey )
{
	return (ex - sx) * (ey - sy) * 2;
}

///////////////////////////////////////////////////////////////////////////////////////////