////////////////////////////////////////////////////////////////////////////////////////////
//
//    
//    File: windows.h
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
#ifndef __WINDOWS_H__
#define __WINDOWS_H__

////////////////////////////////////////////////////////////////////////////////////////
//
//
#include "includes.h"

#define MAX_Y	    	30
#define MAX_X     	    78
#define MAX_WIN         12
#define	START_Y			 3
#define	LEN_Y			14
#define	START_X			 0

typedef struct
{
	int  sx, sy;
	int  ex, ey;
	int  cx, cy;
	int  color;
	char title[30];
	char cache[1024];
//	char buffer[2000];
} WINDOWS, *PWINDOWS;

PWINDOWS GetActiveWindow   ( void );
PWINDOWS CreateWindow      ( int sx, int sy, int ex, int ey, int color, int bkcolor, char *title );
void     CreateTaskWindows ( void );
void     ClearWindow       ( void );
void     SetCurrentWin     ( int curwin );
int      GetCurrentWin     ( void );
void     SaveWindow        ( INT8U sx, INT8U sy, INT8U ex, INT8U ey, char far * buffer );
void     BackWindow        ( INT8U sx, INT8U sy, INT8U ex, INT8U ey, char far * buffer );

INT16U   GetWinBufferSize  (  INT8U sx, INT8U sy, INT8U ex, INT8U ey );

//////////////////////////////////////////////////////////////////////////////////////////
//
#endif