/******************************************************************************
 *
 * Copyright (c) 1995-1999 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: FatalAlert.c
 *
 * Release: 
 *
 * Description:
 *	This file contains routines that display the fatal alert dialog.
 *
 * History:
 *		Sept 19, 1995	Created by Art Lamb
 *		12/03/98	kwk	Changed fatalCtlWidth for Spanish to 68 (was 60).
 *		12/29/98	kwk	Rolled in Japanese localizations.
 *		07/12/99	kwk	Use alert template to set up fatal alert strings.
 *
 *****************************************************************************/

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_FORMS
#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS

#include <PalmTypes.h>

// public system includes
#include <SystemPublic.h>

// private UI includes
#include "Form.h"
#include "UIResources.h"
#include "FatalAlert.h"

// private system includes
#define NON_PORTABLE
#include "EmuStubs.h"
#include "Globals.h"

#ifdef _DEBUG_ERROR_MGR 
#include <SysEvent.h>
#include <FatalAlert.h>
#endif _DEBUG_ERROR_MGR

/***********************************************************************
 *
 *	Internal Constants
 *
 ***********************************************************************/


// Only show the "Debugger" button if Error checking is on full.
#define resetControlIndex		0
#define debuggerControlIndex	1
#define continueControlIndex  2
#define lastControlIndex		2

#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	#define fatalNumCtls				3
#else
	#define fatalNumCtls				1
#endif

#define fatalMsgMarginX		8
#define fatalMsgMarginY		8

// DOLATER kwk - set this based on font metrics, since it will change
// for languages such as Trad. Chinese.

#define fatalCtlHeight			12
#define fatalCtlTitleMarginX	8

// If the fatal alert template isn't loaded yet, use these default
// English strings and button width value. Make a single packed string
// ala the alert template data.

#define defaultFatalAlertStrings		"Fatal Error\0Reset\0Debug\0Cont"
#define defaultFatalAlertBtnWidth	45

/***********************************************************************
 *
 * FUNCTION:		InitFatalAlert
 *
 * DESCRIPTION:	This routine gets called during system initialization
 *						time. The alert template ptr (loaded from system overlay)
 *						is stashed in low memory, along with some values
 *						calculated from the data in the resource.
 *
 * PARAMETERS:		nothing
 *					
 * RETURNED:		nothing
 *
 * HISTORY:
 *		07/12/99	kwk	Created by Ken Krugler.
 *
 ***********************************************************************/
void SysFatalAlertInit (void)
{
	AlertTemplateType* alertP;
	Char* buttonStrP;
	UInt16 index;
	Int16 maxBtnWidth = 0;
	
	alertP = (AlertTemplateType*)MemHandleLock(DmGetResource(alertRscType, sysFatalAlert));
	
	ErrNonFatalDisplayIf(alertP == NULL, "No fatal alert template");
	ErrNonFatalDisplayIf(alertP->numButtons != lastControlIndex + 1,
								"Invalid fatal alert template");
	
	// Figure out the maximum control width. We need to skip the alert
	// title and message text strings that follow the template, to get
	// to the first of the button titles.
	
	buttonStrP = (Char*)(alertP + 1);
	buttonStrP += StrLen(buttonStrP) + 1;
	buttonStrP += StrLen(buttonStrP) + 1;
	
	for (index = 0; index < fatalNumCtls; index++)
		{
		Int16 btnWidth = FntCharsWidth(buttonStrP, StrLen(buttonStrP));
		if (btnWidth > maxBtnWidth)
			{
			maxBtnWidth = btnWidth;
			}
		
		buttonStrP += StrLen(buttonStrP) + 1;
		}
	
	maxBtnWidth += (fatalCtlTitleMarginX * 2);
	
	GFatalAlertTemplateP = alertP;
	GFatalAlertBtnWidth = maxBtnWidth;
}


/***********************************************************************
 *
 * FUNCTION: 	 SelectButton
 *
 * DESCRIPTION: This routine will track the pen until it is released
 *              within the bounds of one of the rectangles passed.
 *
 * PARAMETERS:	table	- pointer to a table object
 *              r		- pointer to the bounds of an item
 *					
 * RETURNED:	 fatalReset, fatalEnterDebugger, or fatalDoNothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art		05/11/95	Initial Revision
 *			vmk		01/20/98	Added code to enable HotSync cradle button to
 *								cause the debugger to be entered while we're
 *								in FatalAlert
 *
 ***********************************************************************/
static UInt16 SelectButton (RectanglePtr ctlR)
{
	Int16 i;
	Int16 x, y;
	Boolean penDown = false;
	Boolean lastPenDown = false;
	Boolean selected = false;
	RectanglePtr r;
	FrameBitsType frame;
	
	
	frame.word = roundFrame;
	
	while (true)
		{
		#if EMULATION_LEVEL != EMULATION_NONE
		StubProcessMacEvents(evtWaitForever);
		#endif
		
		// Check if the HotSync cradle button was pressed - if it has,
		// signal to enter the debugger
		#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
		if ( KeyCurrentState() & keyBitCradle )
			return fatalEnterDebugger;
		#endif
		

		PenGetPoint (&x, &y, &penDown);
		if (penDown && (! lastPenDown))
			{
			for (i = 0; i < fatalNumCtls; i++)
				{
				if (RctPtInRectangle (x, y, &ctlR[i]))
					{
					r = &ctlR[i];
					WinInvertRectangle (r, frame.bits.cornerDiam);
					selected = true;
					do 
						{
						PenGetPoint (&x, &y, &penDown);
					
						if (RctPtInRectangle (x, y, r))
							{
							if (! selected)
								{
								WinInvertRectangle (r, frame.bits.cornerDiam);
								selected = true;
								}
							}
					
						else if (selected)
							{
							WinInvertRectangle (r, frame.bits.cornerDiam);
							selected = false;
							}
					
						} while (penDown);

					if (selected)
						return (i);
					}
				}
			}
		lastPenDown = penDown;
		}
}


/***********************************************************************
 *
 * FUNCTION: 	 SysFatalAlert
 *
 * DESCRIPTION: This routine a dispay a fatal alert until a button in the 
 *              alart is pressed.
 *
 * PARAMETERS:	 msg      - message to display in the dialog
 *					
 * RETURNED:	 fatalReset, fatalEnterDebugger, or fatalDoNothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	9/19/95	Initial Revision
 *			roger	8/11/98	Added SysTaskSwitching calls and cleaned up changed UI globals.
 *			ron   8/18/98  Removed SysTaskSwitching calls because the timer task needs
 *									to run in order to allow pen tracking of the buttons
 *
 ***********************************************************************/
UInt16 SysFatalAlert (const Char* msg)
{
	Int16 i;
	UInt16 length;
	Int16 x, y;
	Int16 displayWidth;
	Int16 displayHeight;
	Int16 maxWidth;
	Int16 margin;
	const Char* msgP;
	const Char* title;
	const Char* btnTextP;
	const Char* ctlLabel[fatalNumCtls];
	RectangleType dialogR;
	RectangleType ctlR [fatalNumCtls];
	RectangleType titleR;
	WinHandle origDrawWindowH;
	FontID origFontID;
	UInt16 selectedButton;
	FrameBitsType frame;
	RGBColorType	foreColor, backColor, oldForeColor, oldBackColor;
	
	
	frame.word = dialogFrame;
	
	// Initialize various ptrs.
	
	if (GFatalAlertTemplateP != NULL)
		{
		title = (const Char*)((AlertTemplateType*)GFatalAlertTemplateP + 1);
		}
	else
		{
		title = defaultFatalAlertStrings;
		GFatalAlertBtnWidth = defaultFatalAlertBtnWidth;
		}
	
	// Skip over the form title and msg strings.
	btnTextP = title + StrLen(title) + 1;
	btnTextP += StrLen(btnTextP) + 1;
	
	// Init control label array
	for (i = 0; i < fatalNumCtls; i++)
		{
		ctlLabel[i] = btnTextP;
		btnTextP += StrLen(btnTextP) + 1;
		}

	origDrawWindowH = WinSetDrawWindow (WinGetDisplayWindow());
	
	// Force in the correct fore and back colors in case app screwed them up
	foreColor.r = foreColor.g = foreColor.b = 0;
	backColor.r = backColor.g = backColor.b = 255;
	WinSetColors(&foreColor, &oldForeColor, &backColor, &oldBackColor);
	
	// Calculate the bounds of the dialog.
	WinGetDisplayExtent (&displayWidth, &displayHeight);
	dialogR.topLeft.x = 0;
	dialogR.topLeft.y = displayHeight / 3;
	dialogR.extent.x = displayWidth - dialogR.topLeft.x;
	dialogR.extent.y = displayHeight - dialogR.topLeft.y;

	// Draw the dialog's frame.
	WinEraseRectangle (&dialogR, 0);
	RctInsetRectangle (&dialogR, frame.bits.width);
	WinDrawRectangleFrame (frame.word, &dialogR);

	// Draw the title.
	origFontID = FntSetFont (boldFont);
	titleR.topLeft.x = dialogR.topLeft.x;
	titleR.topLeft.y = dialogR.topLeft.y;
	titleR.extent.x = dialogR.extent.x;
	titleR.extent.y = FntLineHeight ();
	x  = (dialogR.extent.x - FntCharsWidth (title, StrLen (title)) + 1) / 2;
	WinDrawLine (titleR.topLeft.x+1, titleR.topLeft.y, 
		titleR.extent.x - 2, titleR.topLeft.y);
	WinDrawRectangle (&titleR, 0);
	WinInvertChars (title, StrLen (title), x, titleR.topLeft.y - 1);


	// Draw the message.
	msgP = msg;
	x = dialogR.topLeft.x + fatalMsgMarginX;
	y = dialogR.topLeft.y + fatalMsgMarginY + titleR.extent.y;
	maxWidth = dialogR.extent.x - (fatalMsgMarginX * 2);
	while (true)
		{
		length = FldWordWrap  (msgP, maxWidth);
		if (! length) break;
		WinDrawChars (msgP, length, x, y);
		msgP += length;
		y += FntLineHeight();
		}


	// Calculate the bounds of the buttons and draw them.
	for (i = 0; i < fatalNumCtls; i++)
		{
		margin = (dialogR.extent.x - (fatalNumCtls * GFatalAlertBtnWidth)) / 
			(fatalNumCtls + 1);
		ctlR[i].topLeft.x = (margin * (i + 1)) + (GFatalAlertBtnWidth * i);
		ctlR[i].topLeft.y = dialogR.topLeft.y + dialogR.extent.y - 
			fatalCtlHeight - 5;
		ctlR[i].extent.x = GFatalAlertBtnWidth;
		ctlR[i].extent.y = fatalCtlHeight;
		
		WinDrawRectangleFrame (roundFrame , &ctlR[i]);
		x  = ctlR[i].topLeft.x + ((GFatalAlertBtnWidth - 
			FntCharsWidth (ctlLabel[i], StrLen (ctlLabel[i])) + 1) / 2);
		y = ctlR[i].topLeft.y + ((ctlR[i].extent.y - FntLineHeight()) / 2);
		WinDrawChars (ctlLabel[i], StrLen (ctlLabel[i]), x, y);
		}

#ifdef _DEBUG_ERROR_MGR
	DbgMessage("Choose Button");
#endif 
	
	selectedButton = SelectButton (ctlR);
#ifdef _DEBUG_ERROR_MGR
	DbgMessage("Button Selected");
#endif
	
	// Restore colors
	WinSetColors( &oldForeColor, 0, &oldBackColor, 0);

	WinSetDrawWindow(origDrawWindowH);
	FntSetFont (origFontID);
	
	// Button #fatalButton2Label is doNothing
	if (selectedButton == continueControlIndex)
		return fatalDoNothing;
	else
		return selectedButton;
}
