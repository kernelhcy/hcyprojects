/******************************************************************************
 *
 * Copyright (c) 1994-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: GraffitiShift.c
 *
 * Release: 
 *
 * Description:
 *	  This file contains the Griffiti shift state indicator routines.
 *
 * History:
 *		08/24/95	art	Created by Art Lamb
 *
 *****************************************************************************/

#define NON_PORTABLE

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_CLIPBOARDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_FIELDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS

#include <PalmTypes.h>
#include <SystemPublic.h>

#include "GraffitiShift.h"
#include "UIGlobals.h"
#include "TextServicesPrv.h"


/***********************************************************************
 *
 * FUNCTION:    GsiDrawIndicator
 *
 * DESCRIPTION: This routine draws the graffiti shift state indicator.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * HISTORY:
 *		08/24/95	art	Created by Art Lamb.
 *		05/05/98	art	Call Text Services Manager to override indicator
 *		09/09/98	kwk	Removed conditional #ifdef, and pass GInlineStatus
 *							to TsmDrawModeIndicator.
 *		03/18/99	bob	Set foreground color
 *		09/29/99	gap	Remove call to MenuEraseStatus.  gsi enabling/disabling
 *							now is handled when the command bar is shown/closed.
 *		07/04/00	kwk	Call TsmDrawMode vs. directly accessing FEP globals.
 *
 ***********************************************************************/
static void GsiDrawIndicator (void)
{
	Char indicatorChr;
		
	if (! GsiIsEnabled) return;

	switch (GsiState)
		{
		case gsiNumLock:				indicatorChr = symbolNumLock;		break;
		case gsiCapsLock:				indicatorChr = symbolCapsLock;	break;
		case gsiShiftPunctuation:	indicatorChr = symbolShiftPunc;	break;
		case gsiShiftExtended:		indicatorChr = symbolShiftExt;	break;
		case gsiShiftUpper:			indicatorChr = symbolShiftUpper;	break;
		case gsiShiftLower:			indicatorChr = symbolShiftUpper;	break;
		case gsiShiftNone:			indicatorChr = symbolShiftNone;	break;
		default:
			ErrNonFatalDisplay("Bad GSI");
		}
	
	if (!TsmDrawMode(GsiState, GsiLocation.x, GsiLocation.y))
	{
		IndexedColorType	savedColor;
		FontID curFont = FntSetFont (symbolFont);
		
		// assume background color is correct for window
		savedColor = WinSetForeColor(UIColorGetIndex(UIObjectForeground));
		WinDrawChars (&indicatorChr, 1, GsiLocation.x, GsiLocation.y);
		WinSetForeColor(savedColor);
		FntSetFont (curFont);
	}
}


/***********************************************************************
 *
 * FUNCTION:    GsiInitialize
 *
 * DESCRIPTION: This routine initializes the global variables used to
 *              manage the graffiti shift state indicator.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/24/95	Initial Revision
 *
 ***********************************************************************/
void GsiInitialize (void)
{
	GsiState = gsiShiftLower;
	GsiIsEnabled = false;
	GsiLocation.x = 0;
	GsiLocation.y = 0;
}


/***********************************************************************
 *
 * FUNCTION:    GsiEnable
 *
 * DESCRIPTION: This function enables or disables the graffiti shift
 *              state indicator.  Disabling the indicator makes it visible
 *              disabling making the insertion point invisible.
 *
 * PARAMETERS:  enableIt  TRUE to enable, FALSE to disable
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/24/95	Initial Revision
 *			trev	08/11/97	made non modified passed variables constant
 *
 ***********************************************************************/
void GsiEnable (const Boolean enableIt)
{
	GsiIsEnabled = enableIt;

	if (enableIt)
		GsiDrawIndicator ();
}


/***********************************************************************
 *
 * FUNCTION:    GsiEnabled
 *
 * DESCRIPTION: This routine returns TRUE if the graffiti shift
 *              state indicator is enabled, or FALSE if its disabled.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:   TRUE or FALSE
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/24/95	Initial Revision
 *
 ***********************************************************************/
Boolean GsiEnabled (void)
{
	return (GsiIsEnabled);
}



/***********************************************************************
 *
 * FUNCTION:    GsiSetLocation
 *
 * DESCRIPTION: This routine set the display-relative position of the 
 *              graffiti shift state indicator.  The indicator is not 
 *              redraw by this routine.
 *
 * PARAMETERS:  x  coordinate of left side of the indicator
 *              y  coordinate of top side of the indicator
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/24/95	Initial Revision
 *			trev	08/11/97	made non modified passed variables constant
 *
 ***********************************************************************/
void GsiSetLocation (const Int16 x, const Int16 y)
{
	GsiLocation.x = x;
	GsiLocation.y = y;
}


/***********************************************************************
 *
 * FUNCTION:    GsiSetLocation
 *
 * DESCRIPTION: This routine set the graffiti shift state indicator.
 *
 * PARAMETERS:  capsLock	- true if caps lock on
 *              numLock		- true if num lock on
 *              tempShift	- current temporary shift
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/24/95	Initial Revision
 *			trev	08/11/97	made non modified passed variables constant
 *
 ***********************************************************************/
void GsiSetShiftState (const UInt16 lockFlags, const UInt16 tempShift)
{
	if (tempShift == grfTempShiftPunctuation)
		GsiState = gsiShiftPunctuation;

	else if (tempShift == grfTempShiftExtended)
		GsiState = gsiShiftExtended;

	else if (tempShift == grfTempShiftUpper)
		GsiState = gsiShiftUpper;

	else if (tempShift == grfTempShiftLower)
		GsiState = gsiShiftLower;
	
	else if (lockFlags & glfNumLock)
		GsiState = gsiNumLock;

	else if (lockFlags & glfCapsLock)
		GsiState = gsiCapsLock;

	else
		GsiState = gsiShiftNone;
		
	GsiDrawIndicator ();
}
