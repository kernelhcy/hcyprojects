/******************************************************************************
 *
 * Copyright (c) 1994-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: MenuCmdBar.c
 *
 * Release: 
 *
 * Description:
 *	  Routines to implement the command bar.
 *
 * History:
 *			Name	Date		Description
 *			----	----		-----------
 *			daf	07/16/99	Removed from menu.c, rewrote API & data handling & UI
 *
 *****************************************************************************/

#define NON_PORTABLE

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_MENUS
#define ALLOW_ACCESS_TO_INTERNALS_OF_CLIPBOARDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_FIELDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS

#include <PalmTypes.h>
#include <SystemPublic.h>

#include <PalmUtils.h>

#include "Event.h"
#include "Menu.h"
#include "UIGlobals.h"
#include "UIResources.h"
#include "GraffitiShift.h"


// PrvMenuCmdBarHandleEvent is called from Menu.c
Boolean PrvMenuCmdBarHandleEvent (const EventType * event, UInt16 * errorP);
	

// these are internal definitions so are in here instead of the public .h file
#define commandPendingTimeout		240  // 4 seconds to show bar.  Previously was 150; 2 1/2  seconds
#define commandTimeout				90		 // 1 1/2 seconds
#define MenuCmdBarMaxPotentialButtons	8
#define menuPromptStr				"   "  // used to show a short underlined area for feedback

// how big the borders of the box are (i.e. the outline/shadow)
#define MenuCmdBarTopMargin    1
#define MenuCmdBarRightMargin  2
#define MenuCmdBarLeftMargin   1
#define MenuCmdBarBottomMargin 2

#define MenuCmdBarButtonHeight				13
#define MenuCmdBarButtonWidth					16
#define MenuCmdBarButtonPaddingX				1
#define MenuCmdBarButtonPaddingYTop			1
#define MenuCmdBarButtonPaddingYBottom		2
#define MenuCmdBarTextPadding					1

#define MenuCmdBarHeight	(MenuCmdBarButtonHeight + MenuCmdBarTopMargin + MenuCmdBarBottomMargin    \
									+ MenuCmdBarButtonPaddingYTop + MenuCmdBarButtonPaddingYBottom)

// local private functions
static Err PrvMenuCmdBarAllocateIfNecessary();
static Err PrvMenuCmdBarDisplay (const Char *leftStr, UInt16 timeout, Boolean removeAllButtons, Boolean feedbackMode);
static void PrvMenuCmdBarHandlePen ();
static void PrvSendKeyCommand (MenuBarType * menuP, WChar key);

	
/***********************************************************************
 *
 * FUNCTION: 	 MenuEraseStatus
 *
 * DESCRIPTION: This routine erases the menu command status.
 *
 * PARAMETERS:	 
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/4/95	Initial Revision
 *			jaq	6/3/99	rewrite for command MenuCmdBar buttons
 *			daf	7/16/99	Rewrote API & data handling
 *			gap	9/29/99	Add gsiWasEnabled to prevent gsi from being drawn
 *								over command bar.  No longer call MenuEraseStatus
 *								from GsiDrawIndicator so that 2-stroke x can be used.
 *			gap	9/30/99	Restore the draw window before redrawing the gsi and 
 *								insertion point so that they will draw in the window
 *								coordinates rather than in those of the display window.
 *								Also, eliminated previous error which could result in
 *								setting of draw window to null if no offsreen bits were saved.
 *
 ***********************************************************************/
void MenuEraseStatus (MenuBarType* /*menuP*/)
{
	WinHandle drawWin = NULL;
	Coord x, y;
	Coord width, height;

	// If the status is currently displayed, restore the bits behind the
	// status message.
	if (MenuCmdBarCurrent)
		{
		if (MenuCmdBarCurrent->bitsBehind) 
			{
			drawWin = WinSetDrawWindow (MenuCmdBarCurrent->bitsBehind);
			WinGetWindowExtent (&width, &height);
			WinGetDisplayExtent (&x, &y);
			x = 0;
			y -= height;
			WinSetDrawWindow (WinGetDisplayWindow ());
			WinRestoreBits (MenuCmdBarCurrent->bitsBehind, x, y);
			}
		else
			{ // DOLATER - do something here. send an update event?
			}

		// Restore the original drawing window if necessary.
		if (drawWin)
			WinSetDrawWindow (drawWin);
			
		// Restore the state of the insertion point and GSI indicator.
		if (MenuCmdBarCurrent->insPtWasEnabled)
			InsPtEnable (true);
		if (MenuCmdBarCurrent->gsiWasEnabled)
			GsiEnable (true);
		
		//free up memory
		if (MenuCmdBarCurrent->buttonsData)
			MemPtrFree(MenuCmdBarCurrent->buttonsData);
		MemPtrFree(MenuCmdBarCurrent);
		MenuCmdBarCurrent = NULL;
		}
}	


/***********************************************************************
 *
 * FUNCTION:    PrvMenuCmdBarAllocateIfNecessary
 *
 * DESCRIPTION: 
 *
 * PARAMETERS:	 
 *							
 * RETURNED:		0 if no error, an OS error code otherwise.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			daf		07/19/99	created
 *
 ***********************************************************************/
static Err PrvMenuCmdBarAllocateIfNecessary()
{
	if (!MenuCmdBarCurrent) {
		MenuCmdBarCurrent = MemPtrNew(sizeof(MenuCmdBarType));
		if (!MenuCmdBarCurrent)
			return menuErrOutOfMemory;
		MemSet(MenuCmdBarCurrent, sizeof(MenuCmdBarType), 0);
	}
	
	if (!MenuCmdBarCurrent->buttonsData) {
		MenuCmdBarCurrent->buttonsData = MemPtrNew(MenuCmdBarMaxPotentialButtons * 
												sizeof(MenuCmdBarButtonType));
		if (!MenuCmdBarCurrent->buttonsData)
			return menuErrOutOfMemory;
		MenuCmdBarCurrent->numButtons = 0;
	}

	return 0;
}
	
/***********************************************************************
 *
 * FUNCTION: 	 PrvMenuCmdBarDisplay
 *
 * DESCRIPTION: This routine display the command MenuCmdBar with a brief 
 *              status message, and possibly some number of buttons.
 *              Used for feedback of menu commands and the shortcut stroke.
 *
 * PARAMETERS:	 leftStr - status message (will be preceded by the command stroke)
 *              timeout	  - number of ticks to display for
 *              removeAllButtons - typically true for status displays
 *              feedbackMode - set to ignore input; we're "read only" now.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/4/95	Initial Revision
 *			jaq	6/3/99	rewrite for command menuCmdBar buttons
 *			daf	7/16/99	Rewrote API & data handling & graphics
 *			gap	9/29/99	Add gsiWasEnabled to prevent gsi from being drawn
 *								over command bar.  No longer call MenuEraseStatus
 *								from GsiDrawIndicator so that 2-stroke x can be used.
 *			gap	10/14/99	Need to save and restore font around command bar drawing.
 *			daf	11/3/99  Add feedbackMode param to prevent multiple sequential command selections
 *
 ***********************************************************************/
static Err PrvMenuCmdBarDisplay (const Char *leftStr, UInt16 timeout, Boolean removeAllButtons, Boolean feedbackMode)
{
	WinHandle	drawWin;
	Coord			displayWidth, displayHeight;
	RectangleType r;
	UInt16		error;
	Int16			buttonIndex;
	UInt16		bitmapId;
	Coord			rightBound;
	MemHandle	bitmapH;
	BitmapPtr	bitmapP;
	Err			err;
	Char *		commandPromptP;

	err = PrvMenuCmdBarAllocateIfNecessary();
	if (err)
		return err;
		
	if (removeAllButtons)
		MenuCmdBarCurrent->numButtons=0;
	
	// Set up the event manager such that it will generate a null
	// event at the specified time. 
	MenuCmdBarCurrent->timeoutTick = TimGetTicks () + timeout;
	NeedNullTickCount = MenuCmdBarCurrent->timeoutTick;
	
	// set up the drawing context
	WinPushDrawState ();
	drawWin = WinSetDrawWindow (WinGetDisplayWindow ());
	FntSetFont(boldFont);
	WinSetForeColor(UIColorGetIndex(UIMenuFrame));
	WinSetBackColor(UIColorGetIndex(UIMenuFill));
	WinSetTextColor(UIColorGetIndex(UIMenuForeground));
	
		// Turn off the insertion point if it's on
	if (InsPtEnabled ())
		{
		InsPtEnable(false);
		MenuCmdBarCurrent->insPtWasEnabled = true;
		}
		
	// Turn off the graffiti state indicator if it's on
	if (GsiEnabled())
		{
		GsiEnable(false);
		MenuCmdBarCurrent->gsiWasEnabled = true;
		}
	
	// remember whether we're in feedback mode or not.  (Typically removeAllButtons will be set too)
	MenuCmdBarCurrent->feedbackMode = feedbackMode;

	// find where the command toolbar will be
	WinGetDisplayExtent (&displayWidth, &displayHeight);
	r.extent.y = MenuCmdBarHeight;
	MenuCmdBarCurrent->top = r.topLeft.y = displayHeight - r.extent.y;
	r.topLeft.x = 0;
	r.extent.x = displayWidth;

	// If we haven't already, save the bits behind the status message that we are about to draw.
	if (!MenuCmdBarCurrent->bitsBehind)
		MenuCmdBarCurrent->bitsBehind = WinSaveBits (&r, &error);

	WinEraseRectangle (&r, 0);
	
	// Figure out the interior coordinates and draw the drop shadow
	r.topLeft.x +=MenuCmdBarLeftMargin;
	r.topLeft.y += MenuCmdBarTopMargin;
	r.extent.x -= (MenuCmdBarLeftMargin + MenuCmdBarRightMargin);
	r.extent.y -= (MenuCmdBarTopMargin + MenuCmdBarBottomMargin);
	WinDrawRectangleFrame (menuFrame, &r);
	
	//figure the rightmost bound of the buttons (the rightmost PaddingX gets included later on down)
	rightBound = displayWidth - MenuCmdBarRightMargin;
	
	//draw the buttons, if removeAllButtons isn't set
	WinSetForeColor(UIColorGetIndex(UIMenuForeground));
	buttonIndex = 0;
	while (MenuCmdBarGetButtonData(buttonIndex,&bitmapId,0,0,0))
		{
		rightBound -= (MenuCmdBarButtonWidth + MenuCmdBarButtonPaddingX);
		bitmapH = DmGetResource (bitmapRsc, bitmapId);
		
		if (bitmapH)
			{
			bitmapP = MemHandleLock (bitmapH);
			WinDrawBitmap (bitmapP, rightBound, r.topLeft.y + MenuCmdBarButtonPaddingYTop);
			MemPtrUnlock (bitmapP);
			}

		buttonIndex++;
		}
		
	// Draw the status message.
	r.topLeft.x += MenuCmdBarTextPadding;
	r.topLeft.y += MenuCmdBarTextPadding;
	
	// draw slash to precede the command text
	commandPromptP = MemHandleLock(DmGetResource (strRsc, menuCommandStrID));
	WinDrawChars(commandPromptP, StrLen(commandPromptP), r.topLeft.x, r.topLeft.y);
	MemPtrUnlock(commandPromptP);

	if (!leftStr)
		leftStr = menuPromptStr;  // if there's no text, draw a short underlined area
	WinSetForeColor(UIColorGetIndex(UIFieldTextLines));  // for the underlining
	WinSetUnderlineMode(grayUnderline);
	WinDrawChars (leftStr, StrLen(leftStr), 
						r.topLeft.x + FntCharsWidth(commandPromptP, StrLen(commandPromptP)), 
						r.topLeft.y);
		
	//Clean up + leave
	WinPopDrawState ();
	WinSetDrawWindow (drawWin);
	return 0;
}


/***********************************************************************
 *
 * FUNCTION: 	 PrvGetMenuItem
 *
 * DESCRIPTION: This routine checks if a key is a menu command key, if
 *              it is a returns a pointer to the name of that menu item.
 *
 * PARAMETERS:	 menuP - pointer to menu object to draw
 *              key   - key entered after command key by user, or 0 to check by id
 *				id - if key is 0, id to check for.
 *						nameP - pointer to returned menu name
 *						idP   - pointer to returned menu ID
 *
 * RETURNED:    true iff there is a menu item with that command key
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			jaq	6/11/99	Initial Revision
 *			MT		05/08/00	Added skipHidden because we want to find hidden menus
 *								because menuCmdBarOpenEvent is received before menuOpenEvent
 *								therefor we may have hidden menu items in calls to
 *								MenuCmdBarAddButton and we still want to find their strings.
 *
 ***********************************************************************/
static Boolean PrvGetMenuItem (MenuBarType * menuP, Char key, UInt16 id,
		 Char ** nameP, UInt16 * idP, Boolean skipHidden)
{
	Int16 i, j;

	if (nameP)
		*nameP=0;
		
	if (!menuP)		// just checking...
		return false;

	// Convert the key to uppercase.
	if ((key >= 'a') && (key <= 'z'))
		key -= ('a' - 'A');

	for (i = 0; i < menuP->numMenus; i++)
		{
		for (j = 0; j < menuP->menus[i].numItems; j++)
			{
			if (skipHidden && menuP->menus[i].items[j].hidden) continue; // skip hidden items
			if (key ? menuP->menus[i].items[j].command == key
					: menuP->menus[i].items[j].id == id)
				{
				if (idP)
					*idP = menuP->menus[i].items[j].id;
				if (nameP)
					*nameP = menuP->menus[i].items[j].itemStr;
				return (true);
				}
			}
		}
	return (false);
}


/***********************************************************************
 *
 * FUNCTION:    MenuCmdBarAddButton
 *
 * DESCRIPTION: Called by apps or OS, to define a button to be displayed
 *				on the command bar. This routine should be called in
 *				response to a menuCmdBarOpenEvent event.
 *
 * PARAMETERS:	
 *				where: Specify menuCmdBarOnRight or menuCmdBarOnLeft, to add
 *						the button to the appropriate side of the bar.
 *						Usually pass menuCmdBarOnLeft.
 *						Alternatively, could put a 1-based button index
 *						to overwrite an existing button or place one
 *						at an absolute, fixed location.
 *				bitmapId: resource ID of the bitmap to display
 *				resultType: what action to take if the user selects
 *						this button. (Send char, menu id, etc.)
 *				result: The data to send as specified in resultType
 *				nameP:  Pointer to text for the button.  If null, the
 *						text will be taken from the current menubar
 *						if an entry matches the menuID or command char.
 *						The text is copied, so no need to keep the ptr locked.
 *							
 * RETURNED:		0 if no error, an OS error code otherwise.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			daf	07/16/99	created
 *			kwk	08/11/99	Handle case of getting called w/no nameP and no cur menu.
 *
 ***********************************************************************/
Err MenuCmdBarAddButton(UInt8 where, UInt16 bitmapId,
						MenuCmdBarResultType resultType, UInt32 result, Char *nameP)
{
	MenuCmdBarButtonType *thisButton;
	MenuBarType *editMenu=0;
	Err err;
	
	err = PrvMenuCmdBarAllocateIfNecessary();
	if (err)
		return err;
	
	if ((where==menuCmdBarOnRight) || (where==menuCmdBarOnLeft))
		{
		if (MenuCmdBarCurrent->numButtons >= MenuCmdBarMaxPotentialButtons)
			return menuErrTooManyItems;

		if (where==menuCmdBarOnRight)
			{
			// shift the other buttons up (i.e. left)
			MemMove(&MenuCmdBarCurrent->buttonsData[1], &MenuCmdBarCurrent->buttonsData[0],
					sizeof(MenuCmdBarButtonType) * MenuCmdBarCurrent->numButtons);
			thisButton = &MenuCmdBarCurrent->buttonsData[0];
			}
		else
			thisButton = &MenuCmdBarCurrent->buttonsData[MenuCmdBarCurrent->numButtons];

		MenuCmdBarCurrent->numButtons++;
		} 
	else if ((where > 0) && (where <= MenuCmdBarMaxPotentialButtons))
		{
		thisButton = &MenuCmdBarCurrent->buttonsData[where-1];
		MenuCmdBarCurrent->numButtons = max(MenuCmdBarCurrent->numButtons, where);
		}
	
	// copy the data into our struct for safekeeping
	thisButton->bitmapId = bitmapId;
	thisButton->resultType = resultType;
	thisButton->result = result;
	if ((nameP == NULL) && (UICurrentMenu != NULL))
		{
		// No menu text provided; try to get the string by matching
		// the command char or menu ID with the active menu
		if (resultType == menuCmdBarResultChar)
			PrvGetMenuItem(UICurrentMenu, (WChar) result, 0,  &nameP, 0, false);
		else if (resultType == menuCmdBarResultMenuItem)
			{
			PrvGetMenuItem(UICurrentMenu, 0, (UInt16) result, &nameP, 0, false);
			}
		}

	if ((nameP == NULL) && (resultType == menuCmdBarResultMenuItem) && (result >= sysEditMenuUndoCmd))
		{
		// If we still don't have text for the item, and it is in the system range, 
		// then check the system's edit menu (for cut/copy/paste/undo etc.)
		editMenu = (MenuBarType*)ResLoadMenu(sysEditMenuID);
		PrvGetMenuItem(editMenu, 0, (UInt16) result, &nameP, 0, false);
		}
	
	if (nameP)
		{
		StrNCopy(thisButton->name, nameP, menuCmdBarMaxTextLength);
		}
	else
		{
		thisButton->name[0]=0;
		ErrNonFatalDisplay("No text provided for button");
		}

	if (editMenu)
		MemPtrFree(editMenu);
		
	return 0;
}


/***********************************************************************
 *
 * FUNCTION:    MenuCmdBarGetButtonData
 *
 * DESCRIPTION: Get the data for a given command button.
 *
 *
 * PARAMETERS:	 buttonIndex - index of the button in question
 *				bitmapIdP, resultTypeP, resultP -
 *							holders for returned attributes of the button, or
 *							nil.
 *				 nameP - if not nil, MUST POINT TO A Char[menuCmdBarMaxTextLength]. Returns
 *							0-length string if not present.
 *							
 * RETURNED:		true if a button was successfully found. 
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			jaq	6/3/99	Initial Revision
 *			daf	7/16/99	Rewrote API & data handling
 *
 ***********************************************************************/
Boolean	MenuCmdBarGetButtonData(Int16 buttonIndex, UInt16 *bitmapIdP, 
			MenuCmdBarResultType *resultTypeP, UInt32 *resultP, Char *nameP)
{
	if  ((!MenuCmdBarCurrent) || (!(MenuCmdBarCurrent->buttonsData)))
		return false; //no data means no buttons to find
		
	if (buttonIndex >= MenuCmdBarCurrent->numButtons)
		return false;	//Error - button index out of range

	if (bitmapIdP)
		*bitmapIdP = MenuCmdBarCurrent->buttonsData[buttonIndex].bitmapId;
	if (resultTypeP)
		*resultTypeP = MenuCmdBarCurrent->buttonsData[buttonIndex].resultType;
	if (resultP)
		*resultP = MenuCmdBarCurrent->buttonsData[buttonIndex].result;
	if (nameP)
		StrNCopy(nameP, MenuCmdBarCurrent->buttonsData[buttonIndex].name, menuCmdBarMaxTextLength);

	return true;
}

	
/***********************************************************************
 *
 * FUNCTION:    MenuCmdBarDisplay
 *
 * DESCRIPTION: General entrypoint to draw the Command Bar
 *
 * PARAMETERS:	 
 *
 * RETURNED:	 
 *
 * REVISION HISTORY:
 *			Name	Date			Description
 *			----	----			-----------
 *			jaq	06/03/99		Initial Revision
 *			daf	07/16/99		Rewrote API & data handling
 *
 ***********************************************************************/
void MenuCmdBarDisplay ()
{
	PrvMenuCmdBarDisplay (0, commandPendingTimeout, false, false);
}



/***********************************************************************
 *
 * FUNCTION:    PrvMenuCmdBarHandlePen
 *
 * DESCRIPTION: This routine tracks the pen
 *              until the pen comes up.  If the pen comes up
 *              within the bounds of a command button, it handles that 
 *					 button.
 *
 *
 * PARAMETERS:	 none
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date			Description
 *			----	----			-----------
 *			jaq	06/03/99		Initial Revision
 *			daf	07/19/99		Rewrote API & data handling & graphics
 *			gap	10/14/99		Need to save and restore font around command bar drawing.
 *			gap	11/03/99		Update icon hiliting for color environment.
 *			CS		08/15/00		Initialize new event structure.
 *
 ***********************************************************************/
void PrvMenuCmdBarHandlePen ()
{
	WinHandle	drawWin;
	EventType	newEvent;
	Boolean		penDown;
	Int16			nextInvertedButton, invertedButton = noMenuItemSelection;
	Coord			x, y;
	RectangleType	buttonR, feedbackR;
	Int16			interiorRight, buttonsWidth, promptWidth;
	Coord			displayWidth, displayHeight;
	MenuCmdBarResultType resultType;
	Char			menuName[menuCmdBarMaxTextLength];
	Char			*commandPromptP;
	UInt32		result;
	UInt16		bitmapId;
	MemHandle	bitmapH;
	BitmapPtr	bitmapP;
	
	if ((0==MenuCmdBarCurrent->numButtons) || (MenuCmdBarCurrent->feedbackMode))
		return;  // there's nothing in the bar, or we're just displaying confirmation, so ignore taps.
	
	WinPushDrawState();
	FntSetFont(boldFont);
	drawWin = WinSetDrawWindow (WinGetDisplayWindow ());
	WinSetForeColor(UIColorGetIndex(UIMenuForeground));
	WinSetBackColor(UIColorGetIndex(UIMenuFill));
	WinSetTextColor(UIColorGetIndex(UIMenuForeground));
	WinGetDisplayExtent (&displayWidth, &displayHeight);
	
	// determine coordinates of a rectangle in which a button gets drawn
	buttonR.topLeft.y = MenuCmdBarCurrent->top + MenuCmdBarTopMargin;
	buttonR.extent.y = MenuCmdBarButtonHeight + MenuCmdBarButtonPaddingYTop + MenuCmdBarButtonPaddingYBottom;
	buttonR.extent.x = MenuCmdBarButtonWidth + MenuCmdBarButtonPaddingX;
	// buttonR.topLeft.x gets set later, for the appropriate button.
	
	// Determine coords of where we can put the feedback text.
	// If there aren't 5 buttons, we allow a longer string.
	// (Otherwise it'll look a bit silly to truncate in the middle of nowhere
	// just because a full bar of buttons might have gone to that point.)
	interiorRight = displayWidth - MenuCmdBarRightMargin;
	buttonsWidth = (MenuCmdBarButtonWidth+MenuCmdBarButtonPaddingX) * MenuCmdBarCurrent->numButtons;
	feedbackR.topLeft.y = MenuCmdBarCurrent->top + MenuCmdBarTopMargin + MenuCmdBarTextPadding;
	feedbackR.extent.y = FntLineHeight();
	feedbackR.topLeft.x = MenuCmdBarLeftMargin + MenuCmdBarTextPadding;
	feedbackR.extent.x = interiorRight - buttonsWidth - feedbackR.topLeft.x;

	do
		{
		PenGetPoint (&x, &y, &penDown);

		//Figure which button the pen is in
		if ((y < MenuCmdBarCurrent->top) || (y > MenuCmdBarCurrent->top + MenuCmdBarHeight)
										|| (x > interiorRight))
			{
			nextInvertedButton = noMenuItemSelection;
			}
		else
			{
			nextInvertedButton = (interiorRight - x) / (MenuCmdBarButtonWidth+MenuCmdBarButtonPaddingX);

			if (nextInvertedButton >= MenuCmdBarCurrent->numButtons)
				nextInvertedButton = noMenuItemSelection;
			}
		
		//do the feedback for that button
		if (nextInvertedButton != invertedButton)
			{
			if (invertedButton != noMenuItemSelection)
				{
				WinEraseRectangle (&buttonR, 0);
				
				// un-invert the current button by drawing the bitmap in standard menu foreground 
				// & background colors specified above
				MenuCmdBarGetButtonData(invertedButton,&bitmapId,0,0,0);
				bitmapH = DmGetResource (bitmapRsc, bitmapId);
		
				if (bitmapH)
					{
					bitmapP = MemHandleLock (bitmapH);
					WinDrawBitmap (bitmapP, buttonR.topLeft.x, buttonR.topLeft.y + MenuCmdBarButtonPaddingYTop);
					MemPtrUnlock (bitmapP);
					}
				}
			
			if (nextInvertedButton != noMenuItemSelection)
				{
 				buttonR.topLeft.x = interiorRight - ((1 + nextInvertedButton) * (MenuCmdBarButtonWidth + MenuCmdBarButtonPaddingX));
				
				// invert the new selection using the fore/back color settings for selected menu items
				MenuCmdBarGetButtonData(nextInvertedButton,&bitmapId,0,0,0);
				bitmapH = DmGetResource (bitmapRsc, bitmapId);
		
				if (bitmapH)
					{
					// set the foreground and background colors to the menu selection colors
					WinSetForeColor(UIColorGetIndex(UIMenuSelectedForeground));
					WinSetBackColor(UIColorGetIndex(UIMenuSelectedFill));
					
					// erase the rectangle first as the bitmap does not fill the entire height of the command bar
					WinEraseRectangle (&buttonR, 0);

					// draw the bitmap
					bitmapP = MemHandleLock (bitmapH);
					WinDrawBitmap (bitmapP, buttonR.topLeft.x, buttonR.topLeft.y + MenuCmdBarButtonPaddingYTop);
					MemPtrUnlock (bitmapP);

					// restore the foreground and background colors to the default menu colors
					WinSetForeColor(UIColorGetIndex(UIMenuFrame));
					WinSetBackColor(UIColorGetIndex(UIMenuFill));
					}
				}
			
			invertedButton = nextInvertedButton;
			
			//Show command text
			if (invertedButton != noMenuItemSelection) {
				MenuCmdBarGetButtonData(invertedButton, 0, 0, 0, menuName);
				}
			else
				StrCopy(menuName, menuPromptStr); //if none is selected, show a short underlined area for feedback
			
			WinEraseRectangle(&feedbackR,0);

			// draw slash to precede the command text
			commandPromptP = MemHandleLock(DmGetResource (strRsc, menuCommandStrID));
			promptWidth = FntCharsWidth(commandPromptP, StrLen(commandPromptP));
			WinDrawChars(commandPromptP, StrLen(commandPromptP), feedbackR.topLeft.x,feedbackR.topLeft.y);
			MemPtrUnlock(commandPromptP);

			// and now draw the command text itself, truncated if necessary, underlined in the field color.
			WinSetForeColor(UIColorGetIndex(UIFieldTextLines));
			WinSetUnderlineMode(grayUnderline);
			WinDrawTruncChars (menuName, StrLen(menuName), 
									feedbackR.topLeft.x + promptWidth,feedbackR.topLeft.y, feedbackR.extent.x-promptWidth);
			WinSetUnderlineMode(noUnderline);
			WinSetForeColor(UIColorGetIndex(UIMenuForeground));
			}
		// DOLATER - should we sleep here for a few milliseconds?		
		} while (penDown);
		
	if (invertedButton !=noMenuItemSelection)
		{
		//pen up inside one of the buttons
		
		//get the data for the given button
		MenuCmdBarGetButtonData(invertedButton, 0, &resultType, &result, menuName);

		if (resultType == menuCmdBarResultChar)
			{
			MemSet(&newEvent, sizeof(newEvent), 0);	// Initialize new event structure
			newEvent.eType = keyDownEvent;
			newEvent.data.keyDown.chr = (WChar) result;
			newEvent.data.keyDown.modifiers = commandKeyMask;
			EvtAddEventToQueue (&newEvent);
			}

		else if (resultType == menuCmdBarResultMenuItem)	// directly call menu
			{
			MemSet(&newEvent, sizeof(newEvent), 0);	// Initialize new event structure
			newEvent.eType = menuEvent;
			newEvent.data.menu.itemID = (UInt16) result;
			EvtAddEventToQueue (&newEvent);
			}

		else if (resultType == menuCmdBarResultNotify)	// send notification
			{
			SysNotifyParamType notifyParams;
			
			notifyParams.notifyType = result;
			notifyParams.broadcaster = sysFileCSystem;
			notifyParams.notifyDetailsP = NULL;
			notifyParams.userDataP = NULL;
			notifyParams.handled = false;
			
			SysNotifyBroadcastDeferred(&notifyParams, sizeof(notifyParams));
			}

		// display the status message to the user for a little while
		PrvMenuCmdBarDisplay (menuName, commandTimeout, true, true);
		}
	else
		{
		// no selection was made, but they tapped in the bar, so 
		// reset the timer since they're still using us.
		MenuCmdBarCurrent->timeoutTick = TimGetTicks () + commandTimeout;
		NeedNullTickCount = MenuCmdBarCurrent->timeoutTick;
		}
		
	//Clean up + leave
	WinPopDrawState();
	WinSetDrawWindow (drawWin);
	return;
}


/***********************************************************************
 *
 * FUNCTION:    PrvMenuCmdBarHandleEvent
 *
 * DESCRIPTION: Handle event for the command bar.
 *
 *              When this routine receives a penDownEvent it checks
 *              if the pen position is within the bounds of the
 *              menuCmdBar object, if it is this routine tracks the pen
 *              until the pen comes up.  If the pen comes up
 *              within the bounds of a command button, it handles that 
 *					 button.
 *
 * PARAMETERS:
 *		event		 ->	pointer to an EventType structure.
 *		errorP	<-		pointer to returned error code. We assume this
 *							has been set to errNone for us.
 *									
 *
 * RETURNED:
 *		true if the event was handled or false if it was not.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			jaq	06/03/99	Initial Revision
 *			gap	10/22/99	Close command bar on scroll up/down so that 
 *								indicator update will not draw in command bar
 *			kwk	11/14/00	Make sure it's a keydown event before checking
 *								for the commandChr character.
 *
 ***********************************************************************/
Boolean PrvMenuCmdBarHandleEvent (const EventType * event, 
	UInt16 * /*errorP*/)
{
	Coord x,y;
	
	// if the bar is not up and this is the command char, send out an open event
	if (!MenuCmdBarCurrent)
		{
		if ((event->eType == keyDownEvent)
		 && (event->data.keyDown.chr == commandChr))
			{
			EventType myEvent;
			MemSet(&myEvent,sizeof(EventType),0);
			myEvent.eType = menuCmdBarOpenEvent;
			EvtAddEventToQueue (&myEvent);	
			return (true);
			}
		else
			return (false);
		}

	
	// If we get here, the command bar is open.
	// See if we should handle the event (if it is ours) or close (if it is time to go away)
	
	if (event->eType == keyDownEvent)
		{
		if (EvtKeydownIsVirtual(event))
			{
			if ((event->data.keyDown.chr == vchrPageUp) || (event->data.keyDown.chr == vchrPageDown) )
				{
				// Close the command bar if the user hits the page up or page down button to avoid
				// having the scroll indicators draw over the command bar.  Do not return true so that 
				// appropriate handler will process the scroll.
				MenuEraseStatus (0);
				}
			}
		else // (event->data.keyDown.modifiers & commandKeyMask == 0)
			{
			if ((UICurrentMenu) && (MenuCmdBarCurrent->feedbackMode==0))
				{
				// user wrote a letter; send the appropriate menu command if one exists.
				PrvSendKeyCommand (UICurrentMenu, event->data.keyDown.chr);
				return (true);			
				}
			}
		}
	
	else if (event->eType == penDownEvent)
		{
		// convert from window coordinates to screen coordinates (the event structure's element is misnamed)
		x = event->screenX;
		y = event->screenY;
		WinWindowToDisplayPt(&x, &y);
		if ((y < MenuCmdBarCurrent->top) || (y > MenuCmdBarCurrent->top + MenuCmdBarHeight))
			{
			//outside area of command bar; close and let someone else handle the tap
			MenuEraseStatus (0);
			return false;
			}
		else 
			{
			PrvMenuCmdBarHandlePen();
			return true;
			}
		
		}
		
	else
		{
		if (TimGetTicks () >= MenuCmdBarCurrent->timeoutTick)
			{
			// we've been up too long; time to go away...
			MenuEraseStatus (0);
			}
		return false; //allow further processing
		}
	
	return false;
}
	

/***********************************************************************
 *
 * FUNCTION: 	 PrvSendKeyCommand
 *
 * DESCRIPTION: This routine checks if a key is a menu command key, if
 *              it is a menu event is added to the event queue.
 *
 * PARAMETERS:	 menuP - pointer to menu object to draw
 *              key   - key entered after command key by user
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/31/95	Initial Revision
 *			jaq	6/11/99	tore out guts
 *
 ***********************************************************************/
static void PrvSendKeyCommand (MenuBarType * menuP, WChar key)
{
	EventType newEvent;
	Char * menuName;
	UInt16	menuId;

	if (PrvGetMenuItem(menuP,key,0,&menuName,&menuId,true))
		{
		MemSet (&newEvent,sizeof (EventType), 0);
		newEvent.eType = menuEvent;
		newEvent.data.menu.itemID = menuId;
		EvtAddEventToQueue (&newEvent);
		PrvMenuCmdBarDisplay (menuName, commandTimeout, true, true);
		}
}
