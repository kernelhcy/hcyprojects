/******************************************************************************
 *
 * Copyright (c) 1995-1999 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: InsPoint.c
 *
 * Release: 
 *
 * Description:
 *	  This file contains the insertion point routines.
 *
 * History:
 *		Jan 10, 1995	Created by Art Lamb
 *      mm/dd/yy   initials - brief revision comment
 *
 *****************************************************************************/

#define NON_PORTABLE

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_CLIPBOARDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_FIELDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS

#include <PalmTypes.h>
#include <SystemPublic.h>

#include "InsPoint.h"

#include "UIGlobals.h"

/***********************************************************************
 *
 * Constants
 *
 ***********************************************************************/

#define      insPtWidth          2


/***********************************************************************
 *
 * FUNCTION:    InsPtInitialize
 *
 * DESCRIPTION: This routine
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/11/95		Initial Revision
 *
 ***********************************************************************/
void InsPtInitialize (void)
{
	InsPtIsEnabled = false;
	InsPtOn = false;
	InsPtLoc.x = 0;
	InsPtLoc.y = 0;
	InsPtHeight = 0;
	InsPtLastTick = 0;
	InsPtBitsBehind = 0;
}


/***********************************************************************
 *
 * FUNCTION:    InvertInsertionPoint
 *
 * DESCRIPTION: This routine
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/11/95		Initial Revision
 *
 ***********************************************************************/
static void InvertInsertionPoint (void)
{
	WinHandle			drawWindow;
	UInt16					error;
	RectangleType		r;
	IndexedColorType	oldColor;

	drawWindow = WinSetDrawWindow (WinGetDisplayWindow ());

	if (! InsPtOn)
		{
		if (InsPtHeight)
			{
			r.topLeft.x = 	InsPtLoc.x;
			r.topLeft.y = 	InsPtLoc.y;
			r.extent.x = insPtWidth;
			r.extent.y = InsPtHeight;
			InsPtBitsBehind = WinSaveBits (&r, &error);
			oldColor = WinSetForeColor(UIColorGetIndex(UIFieldCaret));
			WinDrawRectangle (&r, 0);
			WinSetForeColor(oldColor);
			InsPtOn = true;
			}
		}

	else
		{
		WinRestoreBits (InsPtBitsBehind, InsPtLoc.x, InsPtLoc.y);
		InsPtOn = false;
		}

	WinSetDrawWindow (drawWindow);
}


/***********************************************************************
 *
 * FUNCTION:    InsPtCheckBlink
 *
 * DESCRIPTION: This routine inverts the insertion point if it needs to
 *              be blinked.  The insertion point is blinked if the time
 *              since the last blink is greater than or equal to the 
 *              blink interval (insPtBlinkInterval).
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/10/95		Initial Revision
 *
 ***********************************************************************/
void InsPtCheckBlink (void)
{
	long tick;

	if (InsPtIsEnabled)
		{
		tick = TimGetTicks ();
		if ( ((UInt32) (tick - InsPtLastTick)) >= insPtBlinkInterval)
			{
			InsPtLastTick = tick;
			InvertInsertionPoint ();
			}
		}
}


/***********************************************************************
 *
 * FUNCTION:    InsPtEnable
 *
 * DESCRIPTION: This function enables or disables the insertion point.
 *              Enabling the insertion point makes it visible (blinking);
 *              disabling making the insertion point invisible.
 *
 * PARAMETERS:  enableIt  TRUE to enable, FALSE to disable
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/10/95		Initial Revision
 *
 ***********************************************************************/
void InsPtEnable (Boolean enableIt)
{
	InsPtIsEnabled = enableIt;

	if (InsPtOn != enableIt)
		{
		if (enableIt)
			{
			InsPtLastTick = TimGetTicks ();
			}
		InvertInsertionPoint ();
		}
}


/***********************************************************************
 *
 * FUNCTION:    InsPtEnabled
 *
 * DESCRIPTION: This routine returns TRUE if the insertion point is 
 *              enabled, or FALSE if its disabled.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:   TRUE or FALSE
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/10/95		Initial Revision
 *
 ***********************************************************************/
Boolean InsPtEnabled (void)
{
	return (InsPtIsEnabled);
}


/***********************************************************************
 *
 * FUNCTION:    InsPtGetHeight
 *
 * DESCRIPTION: This function returns the hieght of the insertion point.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    height of the insertion point in pixels.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/10/95		Initial Revision
 *
 ***********************************************************************/
Int16 InsPtGetHeight (void)
{
	return (InsPtHeight);
}


/***********************************************************************
 *
 * FUNCTION:    InsPtGetLocation
 *
 * DESCRIPTION: This routine returns the display-relative position of 
 *              the top left corner of the insertion point.
 *
 * PARAMETERS:  x  pointer to x coordinate of left side of the insertion point
 *              y  pointer to y coordinate of top side of the insertion point
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/10/95		Initial Revision
 *			trev	08/11/97	made non modified passed variables constant
 *
 ***********************************************************************/
void InsPtGetLocation (Int16 * x, Int16 * y)
{
	*x = InsPtLoc.x;
	*y = InsPtLoc.y;	
}


/***********************************************************************
 *
 * FUNCTION:    InsPtSetHeight
 *
 * DESCRIPTION: This routine sets the height of the insertion point.
 *              When the current font is changed, the insertion point 
 *              height should be set to the line height of the new 
 *              font.
 *
 * PARAMETERS:  height  height of the insertion point in pixels
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/10/95		Initial Revision
 *			trev	08/11/97	made non modified passed variables constant
 *
 ***********************************************************************/
void InsPtSetHeight (const Int16 height)
{
	InsPtHeight = height;
}


/***********************************************************************
 *
 * FUNCTION:    InsPtSetLocation
 *
 * DESCRIPTION: This routine set the display-relative position of the 
 *              insertion point.
 *
 * PARAMETERS:  x  coordinate of left side of the insertion point
 *              y  coordinate of top side of the insertion point
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/10/95		Initial Revision
 *			trev	08/11/97	made non modified passed variables constant
 *
 ***********************************************************************/
void InsPtSetLocation (const Int16 x, const Int16 y)
{
	Boolean savedInsPtState;

	savedInsPtState = InsPtEnabled ();
	InsPtEnable (false);

	InsPtLoc.x = x;
	InsPtLoc.y = y;

	InsPtEnable (savedInsPtState);
}
