
#ifndef __WINDOWS_H__
#define __WINDOWS_H__

////////////////////////////////////////////////////////////////////////////////////////
//
//
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
} WINDOWS, *PWINDOWS;

PWINDOWS GetActiveWindow   ( void );
PWINDOWS CreateWindow      ( int sx, int sy, int ex, int ey, int color, int bkcolor, char *title );
void     CreateTaskWindows ( void );
void     ClearWindow       ( void );
void     SetCurrentWin     ( int curwin );
int      GetCurrentWin     ( void );

//////////////////////////////////////////////////////////////////////////////////////////
//
#endif