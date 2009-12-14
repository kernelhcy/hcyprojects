/******************************************************************************
 *
 * Copyright (c) 1994-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: Menu.c
 *
 * Release: 
 *
 * Description:
 *            Short description of what the file implements, who uses it 
 *
 * History:
 *		November 21, 1994	Created by Roger Flores
 *      2/8/99   bob - Updated to draw in color
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

#include "SystemPrv.h"
#include "SysTrapsFastPrv.h"

#include "Category.h"
#include "Event.h"
#include "Menu.h"
#include "NotifyMgr.h"
#include "MenuPrv.h"
#include "UIGlobals.h"
#include "UIResourcesPrv.h"
#include "AttentionPrv.h"

#define menuPopupIndicatorOffset	12
#define spaceInsidePullDownFrame	3
#define menuTitleXOffset			4
#define menuFont					boldFont

#define noDrawCoord					(Coord) -1


#define isSeparator(p) ((p)->itemStr[0] == MenuSeparatorChar)

#define menuCommandStrokeChar		chrCommandStroke

extern Boolean PrvMenuCmdBarHandleEvent (const EventType * event, UInt16 * errorP);

static Boolean PrvMenuHideItem(const UInt16 id,Boolean hideIt);
static void PrvUpdateMenuPtrs(MenuBarPtr menuP, UInt8 *baseP, Int32 delta);
static void PrvCalcItemBounds(MenuBarPtr menuP, UInt16 menuStartIndex);


/***********************************************************************
 *
 * FUNCTION: 	 GetPulldownMenu
 *
 * DESCRIPTION: This routine returns a pulldown menu
 *
 * PARAMETERS:	 menuP - pointer to menu object to draw
 *
 * RETURNED:     pointer to a MenuPullDownType
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/28/94	Initial Revision
 *			kwk	07/15/98	Catch case of no menu pulled down (-1).
 *
 ***********************************************************************/
static MenuPullDownType * GetPulldownMenu (MenuBarType * menuP)
{
	if (menuP->curMenu == noMenuSelection)
		{
		ErrNonFatalDisplay("No menu pulled down");
		return 0;
		}
	return &menuP->menus[menuP->curMenu];
}


/***********************************************************************
 *
 * FUNCTION: 	 DrawOneItem
 *
 * DESCRIPTION: Draw the pulldown menu item at position which, selected
 *					 if necessary.
 *
 * PARAMETERS:	 menuP - pointer to menu object to draw
 *					 which - position (0 - n) of the menu which to draw.
 *					 drawY - position from top to draw, computed if noDrawCoord passed
 *					 menuCommandX - horizontal position to draw command Char,
 *									 computed if 0 passed
 *					 selected - whether or not to draw as selected
 *
 * RETURNED:    void
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/28/94	Initial Revision
 *			bob	2/8/99	changed from InvertItem to DrawOneItem
 *			mt		05/02/00	Fix bug where first hidden menu item was displayed as blank
 *
 ***********************************************************************/
static Coord DrawOneItem (MenuBarType * menuP, Int16 which,
								  Coord drawY, Coord menuCommandX, Boolean selected)
{
	MenuPullDownType *	pullDownP;
	Coord					localFntLineHeight;
	MenuItemType 		*itemP;
	Char *				itemText;
	RectangleType		r;
	
	// bail out if we were passed noMenuSelection or separatorItemSelection
	if (which <= noMenuSelection)
		return 0;

	WinPushDrawState();

	localFntLineHeight = FntLineHeight();
	pullDownP = GetPulldownMenu(menuP);
	itemP = pullDownP->items;
	
	// if we weren't passed in a Y offset, compute it
	if (drawY ==noDrawCoord) {
		Int16 i;
		drawY = 0;
		for (i=0; i < which; i++)
			if (!itemP[i].hidden)
				drawY += isSeparator(&itemP[i]) ? (localFntLineHeight/2) : localFntLineHeight;
	}
	
	// Find the widest menu command Char.  The command keys will be drawn
	// so that the widest Char is separated from the right edge of the 
	// menu by spaceInsidePullDownFrame.  If no command keys are assigned
	// then RezConvert will have set up the right bounds closer in and this
	// code will not find any command keys to draw.
	if (menuCommandX == 0) 
		{
	   Int16	charWidth;
		Coord menuCommandWidth = 0;
  		Int16 i;
		for (i=0; i < pullDownP->numItems; i++)
			{
			if (itemP[i].hidden) continue;
			if (itemP[i].command != 0 && itemP[i].itemStr[0] != MenuSeparatorChar) {
				charWidth = FntCharWidth(itemP[i].command);
				if (charWidth > menuCommandWidth)
					menuCommandWidth = charWidth;
			}
		}
		menuCommandX = pullDownP->bounds.extent.x - (spaceInsidePullDownFrame - 1 ) - 
			menuCommandWidth;
	}
	
	if (selected) {
		WinSetTextColor(UIColorGetIndex(UIMenuSelectedForeground));
		WinSetForeColor(UIColorGetIndex(UIMenuSelectedForeground));
		WinSetBackColor(UIColorGetIndex(UIMenuSelectedFill));
	}
	else {
		WinSetTextColor(UIColorGetIndex(UIMenuForeground));
		WinSetForeColor(UIColorGetIndex(UIMenuForeground));
		WinSetBackColor(UIColorGetIndex(UIMenuFill));
	}

	r.topLeft.x = 0;
	r.extent.x = pullDownP->bounds.extent.x;
	r.topLeft.y = drawY;
	r.extent.y = isSeparator(itemP) ? (localFntLineHeight/2) : localFntLineHeight;
	WinEraseRectangle (&r, 0);
	
	// Draw the item text if there is any.
	itemP += which;
	itemText = itemP->itemStr;
	ErrNonFatalDisplayIf(itemText == NULL, "NULL menu item text.");

	if (itemText)
		{
		if (itemText[0] == MenuSeparatorChar)
			{
			// Draw a dotted line for a separator.
			WinDrawGrayLine (0, drawY + localFntLineHeight / 4,
				pullDownP->bounds.extent.x, drawY + localFntLineHeight / 4);
			drawY += localFntLineHeight / 2;		// separators occupy half a line
			}
		else
			{
			// Draw the menu item text
			WinDrawChars (itemText, StrLen(itemText), spaceInsidePullDownFrame, drawY);
			
			// If there is a command display the key for it
			if (itemP->command != 0)
				{
				Char menuChar = menuCommandStrokeChar;
				WinDrawChars (&menuChar, 1, menuCommandX - FntCharWidth(menuCommandStrokeChar), drawY);
				WinDrawChars ((Char *) &itemP->command, 1, menuCommandX, drawY);
				}
			drawY += localFntLineHeight;
			}
		}
	
	WinPopDrawState();

	return drawY;
}


/***********************************************************************
 *
 * FUNCTION: 	 DrawOneTitle
 *
 * DESCRIPTION: Draw the menu title at position, selected if necessary.
 *
 * PARAMETERS:	 menuP - pointer to menu object to draw
 *					 which - position (0 - n) of the menu item to draw
 *					 selected - draw in selected mode
 *
 * RETURNED:    void
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/23/94	Initial Revision
 *			bob	2/8/99	Draw item in proper color instead
 *			mt		05/02/00	Add capability to hide an entire menu pull down
 *
 ***********************************************************************/
static void DrawOneTitle (MenuBarType * menuP, Int16 which, Boolean selected)
{
	Char * titleText;

	if (menuP->menus[which].hidden)	return;

	ErrNonFatalDisplayIf(which == noMenuSelection, 
		"Somehow drawing when there isn't a menu to invert.");
	
	WinPushDrawState();

	if (selected) {
		WinSetTextColor(UIColorGetIndex(UIMenuSelectedForeground));
		WinSetBackColor(UIColorGetIndex(UIMenuSelectedFill));
	}
	else {
		WinSetTextColor(UIColorGetIndex(UIMenuForeground));
		WinSetBackColor(UIColorGetIndex(UIMenuFill));
	}

	WinEraseRectangle (&(menuP->menus[which].titleBounds), 0);

	titleText = menuP->menus[which].title;
	WinDrawChars (titleText, StrLen(titleText),
		menuP->menus[which].titleBounds.topLeft.x + menuTitleXOffset,
		menuP->menus[which].titleBounds.topLeft.y);

	WinPopDrawState();
}


/***********************************************************************
 *
 * FUNCTION:    DrawMenuPulldown
 *
 * DESCRIPTION: Draw the current menu (if any).
 *
 * PARAMETERS:	 menuP - pointer to menu object to draw
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/21/94	Initial Revision
 *
 ***********************************************************************/
static void DrawMenuPulldown (MenuBarType * menuP)
{
	RectangleType frame;
	Int16 i;
	MenuItemType * items;
	UInt16 error;
	MenuPullDownType * thisMenuP;
	Coord	drawY = 0;
	Coord menuCommandX;
	Coord menuCommandWidth = 0;
	Int16 charWidth;
	

	if (!menuP->attr.visible ||
		 menuP->curMenu == noMenuSelection ||
		 menuP->menus == NULL)
		return;


	WinPushDrawState();

	thisMenuP = GetPulldownMenu(menuP);


	// If there isn't a window for the menu then make one and
	// save the bits behind it.
	if (thisMenuP->menuWin == NULL)
		{
		thisMenuP->menuWin = WinCreateWindow (&thisMenuP->bounds,
			menuFrame, false, false, &error);
		// Save the bits behind the popup window.
		WinSetDrawWindow (WinGetDisplayWindow());
		WinGetWindowFrameRect (WinGetWindowHandle(thisMenuP->menuWin), &frame);
		RctInsetRectangle (&frame, -1);				// expand bounds so there is space between menu and background
		thisMenuP->bitsBehind = WinSaveBits (&frame, &error);
		}


	// Erase the window's backgroud and draw the windows frame.
	WinSetForeColor(UIColorGetIndex(UIMenuFrame));
	WinSetBackColor(UIColorGetIndex(UIMenuFill));
	WinEraseLine (frame.topLeft.x + 2, frame.topLeft.y + 2,
		frame.topLeft.x + frame.extent.x - 4, frame.topLeft.y + 2);
	frame.topLeft.y += 3;
	frame.extent.y -= 3;
	WinEraseRectangle (&frame, 0);
	WinSetDrawWindow (WinGetWindowHandle(thisMenuP->menuWin));
	WinDrawWindowFrame();



	// Draw all the menu items
	items = thisMenuP->items;
	for (i=0; i < thisMenuP->numItems; i++)
		if (items[i].command != 0 &&
			items[i].itemStr[0] != MenuSeparatorChar && !items[i].hidden)
			{
			charWidth = FntCharWidth(items[i].command);
			if (charWidth > menuCommandWidth)
				menuCommandWidth = charWidth;

			// FntCharWidth counts the last column which is blank.  Subtract it.
			// DOLATER kwk - better here would be a call to get a character's glyph width.
			if (charWidth > 0)
				charWidth--;
			}
	
	// Draw all the menu items
	menuCommandX = thisMenuP->bounds.extent.x - (spaceInsidePullDownFrame - 1 ) - 
		menuCommandWidth;

	for (i=0; i < thisMenuP->numItems; i++)
		if (!items[i].hidden)
			drawY = DrawOneItem(menuP, i, drawY, menuCommandX, i == menuP->curItem);

	WinEnableWindow (WinGetWindowHandle(thisMenuP->menuWin));
	WinSetActiveWindow (WinGetWindowHandle(thisMenuP->menuWin));

	WinPopDrawState();

	}


/***********************************************************************
 *
 * FUNCTION:    ErasePulldownMenu
 *
 * DESCRIPTION: Erase the bounds of the menu pulldown specified.
 *
 * PARAMETERS:	 menuP - menu pulldown to erase
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	9/28/94	Initial Revision
 *			roger	6/6/96	Cleaned up the way menus exit
 *
 ***********************************************************************/
static void ErasePulldownMenu (MenuBarType * menuP, Boolean removingCompleteMenu)
	{
	WinHandle drawWindow; 
	RectangleType r;
	MenuPullDownType * thisMenuP;

	if (!menuP->attr.visible ||
		 menuP->curMenu == noMenuSelection ||
		 menuP->menus == NULL)
		{
		return;
		}


	thisMenuP = GetPulldownMenu(menuP);


	drawWindow = WinSetDrawWindow (WinGetDisplayWindow ());
	WinGetWindowFrameRect (WinGetWindowHandle(thisMenuP->menuWin), &r);
	RctInsetRectangle (&r, -1);				// expand bounds so there is space between menu and background
	WinRestoreBits (thisMenuP->bitsBehind, r.topLeft.x, r.topLeft.y);
	
	// check if old draw window was pulldown menu which we're about to delete
	if (drawWindow == thisMenuP->menuWin)
		drawWindow = NULL;
	WinDeleteWindow(WinGetWindowHandle(thisMenuP->menuWin), false);
	thisMenuP->menuWin = NULL;
	
	// When removing the complete menu bar it is desirable to 
	// not call WinSetActiveWindow so that multiple winEnter and winExit
	// events are not posted to the event queue.
	if (!removingCompleteMenu)
		{
		WinSetDrawWindow (WinGetWindowHandle(menuP->barWin));
		WinSetActiveWindow (WinGetWindowHandle(menuP->barWin));
		DrawOneTitle (menuP, menuP->curMenu, false);
		}
	else
		// only restore draw window if it wasn't the pulldown menu
		if (drawWindow != NULL)
			WinSetDrawWindow (drawWindow);
	}


/***********************************************************************
 *
 * FUNCTION:    MenuEraseMenu
 *
 * DESCRIPTION: Erase the bounds of the menu bar and pulldown
 *
 * PARAMETERS:	 menu - pointer to menu object to erase
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	9/28/94	Initial Revision
 *			roger	6/6/96	Cleaned up the way menus exit
 *			jmp   9/22/99	Exposed this routine externally
 *
 ***********************************************************************/
void MenuEraseMenu (MenuBarType * menuP, Boolean removingCompleteMenu)
	{
	WinHandle drawWindow;
	RectangleType r;

	if (!menuP->attr.visible)
		return;

	if (menuP->curMenu != noMenuSelection)
		ErasePulldownMenu (menuP, removingCompleteMenu);

	drawWindow = WinSetDrawWindow (WinGetDisplayWindow ());
	WinGetWindowFrameRect (WinGetWindowHandle(menuP->barWin), &r);
	WinRestoreBits (menuP->bitsBehind, r.topLeft.x, r.topLeft.y);
	WinDeleteWindow(WinGetWindowHandle(menuP->barWin), false);
	menuP->barWin = NULL;
	menuP->attr.visible = false;
	
	
	// Don't send another winEnter event if the savedActiveWin is already
	// set active.  This can happen if the user does a penDown outside
	// the menu windows.
	if (WinGetActiveWindow() != menuP->savedActiveWin)
		{
		WinSetDrawWindow (menuP->savedActiveWin);
		WinSetActiveWindow (menuP->savedActiveWin);
		}
	else if (menuP->savedActiveWin == 0)
		{
		// If there wasn't an active window we need to reset the draw window.
		WinSetDrawWindow (WinGetDisplayWindow ());
		}
	else
		WinSetDrawWindow(drawWindow);

	// Restore the state of the insertion point.
	InsPtEnable (menuP->attr.insPtEnabled == true);

	// Restore the state of the attention indicator.
	if (menuP->attr.attnIndicatorIsAllowed)
		AttnIndicatorAllow (true);
	}


/***********************************************************************
 *
 * FUNCTION: 	 MapPointToItem
 *
 * DESCRIPTION: Return return the item at x, y.  noItemSelected is
 *					 if the point is out of bounds of the menu or if
 *					 the item isn't selectable (a separator).
 *
 * PARAMETERS:  x, y - coordinate
 *              menuP - menu bar pointer
 *
 * RETURNED:	 Title number (doesn't check for invalid bounds!)
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/28/94	Initial Revision
 *			mt		05/02/00	Fix hang if taping menu containning a hidden item
 *
 ***********************************************************************/
static Int16 MapPointToItem (Coord x, Coord y, MenuBarType * menuP)
	{
	MenuPullDownType * thisMenuP;
	MenuItemType *itemP;
	Int16 item;		// the item selected;
	Int16 localFntLineHeight;


	thisMenuP = GetPulldownMenu(menuP);

	// Is the point outside the menu window?
	if (x >= thisMenuP->bounds.extent.x ||
		 y >= thisMenuP->bounds.extent.y)
		item = noMenuItemSelection;
	else
		{
		item = noMenuItemSelection;
		itemP = &thisMenuP->items[0];
		localFntLineHeight = FntLineHeight();

		do
			{
			if (!itemP->hidden)
			{
				y -= isSeparator(itemP) ? localFntLineHeight/2 : localFntLineHeight;
			}
			itemP++;
			item++;
			} while (y >= 0);

		if (isSeparator((itemP-1)))
			item = separatorItemSelection;
		else
		if (item >= thisMenuP->numItems)
			item = noMenuItemSelection;
		}


	return item;
	}


/***********************************************************************
 *
 * FUNCTION: 	 MapPointToTitle
 *
 * DESCRIPTION: Return the Title at x, y.
 *
 * PARAMETERS:  x, y - coordinate
 *              menuP - menu bar pointer
 *
 * RETURNED:	 Title number (doesn't check for invalid bounds!)
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/15/94	Initial Revision
 *			mt		05/02/00	Dynamic menu: can now remove entire menu by removing all items
 *
 ***********************************************************************/
static Int16 MapPointToTitle (Coord x, Coord y, MenuBarType * menuP)
{
	Int16		i;

	for (i=0; i < menuP->numMenus; i++)
	{
		if (menuP->menus[i].hidden ==false)
		{
			if (RctPtInRectangle(x, y, &menuP->menus[i].titleBounds))
				return i;
		}
	}

	return noMenuSelection;
}


/***********************************************************************
 *
 * FUNCTION:    MenuPulldownHandleEvent
 *
 * DESCRIPTION: Handles events in pulldown menus.  The currently only
 *              penDownEvents are handled.
 *
 *              For a penDownEvent the pen position is polled and
 *					 the menu item under the pen is inverted if allowed
 *					 (separators aren't inverted).  If the pen moves above
 *					 the menu then a penDownEvent is made and passed to the
 *					 menu bar via the event queue.  If the pen is outside
 *					 the pulldown menu but not above, no item is inverted.
 *					 If the pen is released outside the pulldown menu the
 *					 pulldown menu is erased.
 *
 *
 * PARAMETERS:	 menuP	  - pointer to Menu Bar object (MenuBarType)
 *              eventP    - pointer to an EventType structure.
 *              errorP    - pointer to returned error code
 *
 * RETURNED:	true if the event was handle or false if it was not.
 *             errorP     - error code.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/28/94	Initial Revision
 *			gap	 9/21/99	Clear newEvent before setting penDownEvent data.
 *
 ***********************************************************************/
static Boolean MenuPulldownHandleEvent
  (MenuBarType * menuP, EventType * eventP)
	{
	EventType	newEvent;
	Boolean		penDown;
	Boolean		inverted;
	Coord			x, y;
	Int16       newItem, proposedItem;
	RectangleType	r;
	MenuPullDownType * thisMenuP;


	// point to the correct menu.
	thisMenuP = GetPulldownMenu(menuP);

	// clear event here as fields may be set in more than one location in routine. 
	MemSet (&newEvent, sizeof (EventType), 0);

	r.topLeft.x = 0;
	r.topLeft.y = 0;
	WinGetWindowExtent(&r.extent.x, &r.extent.y);


	// The first point must be in the window or else the window wouldn't
	// be given the point.
	newItem = menuP->curItem;
	proposedItem = MapPointToItem (eventP->screenX, eventP->screenY, menuP);
	if (newItem != proposedItem)
		{
		DrawOneItem(menuP, newItem, noDrawCoord, 0, false);
		DrawOneItem(menuP, proposedItem, noDrawCoord, 0, true);

		newItem = proposedItem;
		menuP->curItem = proposedItem;
		}


	inverted = (newItem != noMenuItemSelection);
	penDown = true;
	do
		{
		PenGetPoint (&x, &y, &penDown);
		if (RctPtInRectangle (x, y, &r))
			{
			proposedItem = MapPointToItem (x, y, menuP);
			if (penDown)
				{
				if (newItem != proposedItem)
					{
					if (inverted)
						DrawOneItem(menuP, newItem, noDrawCoord, 0, false);
					DrawOneItem(menuP, proposedItem, noDrawCoord, 0, true);
					newItem = proposedItem;
					menuP->curItem = proposedItem;
					inverted = true;
					}
				}
			}
		else     //moved out of bounds
			{
			if (inverted)
				{
				DrawOneItem(menuP, newItem, noDrawCoord, 0, false);
				inverted = false;
				}
			newItem = noMenuSelection;


			// if the pen drags above the menu to the window bar
			// then exit this loop and pass the pen to the menu bar
			// event handler.  To do this we pass the batton by
			// switching the active window to the pulldown's window
			// and we add a doctored event signaling a penDownEvent
			// within the pulldown menu.
			if (y < -1)
				{
				newEvent.penDown = true;
				newEvent.screenX = x;
				newEvent.screenY = y;
				newEvent.eType = penDownEvent;
				EvtAddEventToQueue (&newEvent);
				WinSetDrawWindow (WinGetWindowHandle(menuP->barWin));
				WinSetActiveWindow (WinGetWindowHandle(menuP->barWin));
				menuP->curItem = noMenuItemSelection;
				return true;
				}
			}
		} while (penDown);




	if (newItem == noMenuItemSelection)
		{
		//No longer erase and set to no menu selected. The noMenuItem
		//state is confusing to the user.
		
		return true;
		}
	else if (newItem == separatorItemSelection)
		return true;


	menuP->curItem = newItem;
	newEvent.data.menu.itemID = thisMenuP->items[newItem].id;
	newEvent.eType = menuEvent;
	
	// <DemoChange> If this is a demo device, play sound and ignore
	if (GSysMiscFlags & sysMiscFlagGrfDisable)
		{
		SndPlaySystemSound(sndWarning);
		}
	else
		{
		EvtAddEventToQueue (&newEvent);
		}

	MenuEraseMenu(menuP, dontRemoveCompleteMenu);
	return true;
	}


/***********************************************************************
 *
 * FUNCTION:    MenuBarHandleEvent
 *
 * DESCRIPTION: Handles events in the menu bar.  This routine
 *              handles penDownEvents.
 *
 *              When this routine receives a penDownEvents it polls
 *              if the pen position.  If the pen is on a menu title
 *					 that menu title becomes the inverted title.  In
 *					 addition the pulldown menu associated with the
 *					 inverted title is drawn.  If the pen descends into
 *					 the pulldown menu a penDownEvent is made and passed
 *					 to the PulldownEventHandler via the EventQueue.  If
 *					 pen travels to where there isn't a menu title (the
 *					 far left and far right of the menu bar or outside of
 *					 the bar) no menu title is shown inverted.  If the pen
 *					 is released nothing is done.
 *
 *
 *
 * PARAMETERS:	 menuP	  - pointer to Menu Bar object (MenuBarType)
 *              eventP    - pointer to an EventType structure.
 *              errorP    - pointer to returned error code
 *
 * RETURNED:	true if the event was handle or false if it was not.
 *             errorP     - error code.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/23/94	Initial Revision
 *			gap	 9/21/99	Clear newEvent before setting data.
 *
 ***********************************************************************/
static Boolean MenuBarHandleEvent (MenuBarType * menuP, EventType * eventP, UInt16 * errorP)
	{
	EventType	newEvent;
	Boolean		penDown;
	Boolean		inverted;
	Boolean		penUpInTitleRemovesPulldown;	// true only for the original menu
	Coord			x, y;
	Int16       newTitle, proposedTitle;
	RectangleType	r;
	MenuPullDownType * thisMenuP;



	r.topLeft.x = 0;
	r.topLeft.y = 0;
	WinGetWindowExtent(&r.extent.x, &r.extent.y);


	// The first point must be in the window or else the window wouldn't
	// be given the point.
	newTitle = menuP->curMenu;
	inverted = (newTitle != noMenuSelection);
	penUpInTitleRemovesPulldown = inverted;
	proposedTitle = MapPointToTitle (eventP->screenX, eventP->screenY, menuP);
	if (proposedTitle == noMenuSelection)
		proposedTitle = newTitle;
	if (newTitle != proposedTitle)
		{
		newTitle = proposedTitle;
		ErasePulldownMenu(menuP, false);
		if (proposedTitle != noMenuSelection)
			{
			DrawOneTitle(menuP, proposedTitle, true);
			inverted	= true;
			}
		else
			inverted	= false;
		penUpInTitleRemovesPulldown = false;
		menuP->curMenu = proposedTitle;
		menuP->curItem = noMenuItemSelection;
		DrawMenuPulldown (menuP);
		WinSetDrawWindow (WinGetWindowHandle(menuP->barWin));
		WinSetActiveWindow (WinGetWindowHandle(menuP->barWin));
		}


	penDown = true;
	do
		{
		PenGetPoint (&x, &y, &penDown);
		if (*errorP) return (false);

		// ignore the out of bounds
		proposedTitle = MapPointToTitle (x, 1, menuP);
		if (proposedTitle == noMenuSelection)
			proposedTitle = newTitle;

		if (RctPtInRectangle (x, y, &r))
			{
			if (penDown)
				{
				if (newTitle != proposedTitle)
					{
					if (inverted)
						{
						ErasePulldownMenu(menuP, false);
						}
					newTitle = proposedTitle;
					menuP->curMenu = proposedTitle;
					menuP->curItem = noMenuItemSelection;
					penUpInTitleRemovesPulldown = false;
					if (newTitle != noMenuSelection)
						{
						DrawOneTitle(menuP, proposedTitle, true);
						DrawMenuPulldown (menuP);
						inverted = true;
						}
					else
						inverted = false;
						
					WinSetDrawWindow (WinGetWindowHandle(menuP->barWin));
					WinSetActiveWindow (WinGetWindowHandle(menuP->barWin));
					}
				}
			else
				{
				if (inverted && penUpInTitleRemovesPulldown)
					{
	
					ErasePulldownMenu(menuP, false);
					menuP->curMenu = noMenuSelection;
					menuP->curItem = noMenuItemSelection;
					newTitle = noMenuSelection;
					inverted = false;
					}
				}
			}
		else     //moved out of bounds
			{
			if (inverted)
				{

				// if the pen drags below the menu bar to the menu pulldown
				// then exit this loop and pass the pen to the menu pulldown
				// event handler.  To do this we pass the batton by
				// switching the active window to the pulldown's window
				// and we add a doctored event signaling a penDownEvent
				// within the pulldown menu.
				if (proposedTitle == newTitle && y >= 12)
					{
					// point to the correct menu.
					thisMenuP = GetPulldownMenu(menuP);

					MemSet (&newEvent, sizeof (EventType), 0);
					newEvent.penDown = true;
					newEvent.screenX = x;
					newEvent.screenY = y;
					newEvent.eType = penDownEvent;
					EvtAddEventToQueue (&newEvent);
					WinSetDrawWindow (WinGetWindowHandle(thisMenuP->menuWin));
					WinSetActiveWindow (WinGetWindowHandle(thisMenuP->menuWin));
					return true;
					}

				ErasePulldownMenu(menuP, false);
				menuP->curMenu = noMenuSelection;
				menuP->curItem = noMenuItemSelection;
				newTitle = noMenuSelection;
				inverted = false;
				}
			}
		} while (penDown);



	if (newTitle == noMenuSelection &&
		menuP->curMenu != noMenuSelection)
		{
		DrawOneTitle(menuP, menuP->curMenu, true);
//		return true;
		}


	return true;
	}


/***********************************************************************
 *
 * FUNCTION: 	 MenuInit
 *
 * DESCRIPTION: Load a menu bar and all its pulldowns from
 *              a resource file
 *
 * PARAMETERS:	 resourceId - resource id of a menu bar resource
 *
 * RETURNED:    pointer to the memory block containing the menu
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/6/95	Initial Revision
 *			trev	08/11/97	made non modified passed variables constant
 *
 ***********************************************************************/
MenuBarType * MenuInit (UInt16 resourceId)
{
	return (ResLoadMenu (resourceId));
}


/***********************************************************************
 *
 * FUNCTION: 	 MenuDispose
 *
 * DESCRIPTION: This routine frees all memory allocated by a menu and
 *   the command status and restores their saved bits to the screen
 *
 * PARAMETERS:	 menuP - pointer to menu object to dispose
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/31/95	Initial Revision
 *			trev	08/11/97	made non modified passed variables constant
 *
 ***********************************************************************/
void MenuDispose (MenuBarType * menuP)
{
	Int16 i;
	RectangleType r;
	WinHandle curDrawWin;
	MenuPullDownType * thisMenuP;
	
	// If no menu, return doing nothing. This is necessary because some
	// 3rd party apps call this routine with a nil menu pointer. <RM> 11/5/96
	if (!menuP) return;
	
	// If the menu bar or pulldown menu is visible, erase them.
	if (menuP->attr.visible)
		{
		curDrawWin = WinGetDrawWindow ();
		WinSetDrawWindow (WinGetDisplayWindow ());

		if ((menuP->curMenu != noMenuSelection) && menuP->menus)
			{
			thisMenuP = GetPulldownMenu(menuP);
			WinGetWindowFrameRect (WinGetWindowHandle(thisMenuP->menuWin), &r);
			RctInsetRectangle (&r, -1);				// expand bounds so there is space between menu and background
			WinRestoreBits (thisMenuP->bitsBehind, r.topLeft.x, r.topLeft.y);
			}
		
		WinGetWindowFrameRect (WinGetWindowHandle(menuP->barWin), &r);
		WinRestoreBits (menuP->bitsBehind, r.topLeft.x, r.topLeft.y);

		WinSetDrawWindow (curDrawWin);

		// Restore the state of the insertion point.
		InsPtEnable (menuP->attr.insPtEnabled == true);

		// Restore the state of the attention indicator.
		if (menuP->attr.attnIndicatorIsAllowed)
			AttnIndicatorAllow (true);
		}

	MenuEraseStatus (0);

	for (i=0; i < menuP->numMenus; i++)
		{
		if (menuP->menus[i].menuWin)
			WinDeleteWindow (menuP->menus[i].menuWin, false);
		}	
	if (menuP->barWin)
		WinDeleteWindow (menuP->barWin, false);
	
	MemPtrFree (menuP);
}


/***********************************************************************
 *
 * FUNCTION: 	 MenuGetActiveMenu
 *
 * DESCRIPTION: This routine returns a pointer to the current menu.
 *
 * PARAMETERS:	 nothing
 *
 * RETURNED:    pointer to the active menu
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	9/8/95	Initial Revision
 *
 ***********************************************************************/
MenuBarType * MenuGetActiveMenu (void)
{
	return (UICurrentMenu);
}


/***********************************************************************
 *
 * FUNCTION: 	 MenuSetActiveMenu
 *
 * DESCRIPTION: Set the current menu.
 *
 * PARAMETERS:	 menuP - pointer to the memory block containing the menu
 *
 * RETURNED:    pointer to the menu that was active before the new menu
 *              was set
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/6/95	Initial Revision
 *			trev	08/11/97	made non modified passed variables constant
 *
 ***********************************************************************/
MenuBarType * MenuSetActiveMenu (MenuBarType * menuP)
{
	MenuBarType * curMenu;
	
	curMenu = UICurrentMenu;
	UICurrentMenu = menuP;
	UICurrentMenuRscID = 0;
	return (curMenu);
}


/***********************************************************************
 *
 * FUNCTION: 	 MenuSetActiveMenuRscID
 *
 * DESCRIPTION: Set the resource id of the current menu.  This routine 
 *              will not load the menu resource,  it will be loaded the
 *              first time it is needed.
 *
 * PARAMETERS:	 resourceId - resource id of a menu bar resource
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	9/22/96	Initial Revision
 *			trev	08/11/97	made non modified passed variables constant
 *
 ***********************************************************************/
void MenuSetActiveMenuRscID (UInt16 resourceId)
{	
	UICurrentMenu = NULL;
	UICurrentMenuRscID = resourceId;
}


/***********************************************************************
 *
 * FUNCTION:    MenuDrawMenu
 *
 * DESCRIPTION: Draw the menu bar and any current menu pulldown.
 *
 * PARAMETERS:	 menuP - pointer to menu object to draw
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/21/94	Initial Revision
 *			trev	08/11/97	made non modified passed variables constant
 *
 ***********************************************************************/
void MenuDrawMenu (MenuBarType * menuP)
	{
	RectangleType	r, frame;
	Int16 i;
	UInt16 error;

	WinSetDrawWindow (WinGetDisplayWindow());
	WinPushDrawState();

	menuP->attr.visible = true;
	FntSetFont (menuFont);
	
	// Turn off the insertion point if it's on.
	if (InsPtEnabled ())
		{
		menuP->attr.insPtEnabled = true;
		InsPtEnable (false);
		}
	else
		menuP->attr.insPtEnabled = false;

	// Turn off the attention indicator if it's on.
	if (AttnIndicatorAllowed ())
		{
		menuP->attr.attnIndicatorIsAllowed = true;
		AttnIndicatorAllow (false);
		}
	else
		menuP->attr.attnIndicatorIsAllowed = false;

	// If there isn't a window for the menu then make one and
	// save the bits behind it.
	if (menuP->barWin == NULL)
		{
		// Set the bounds of the menu bar.
		r.topLeft.x = 1;
		r.topLeft.y = 1;
		WinGetDisplayExtent(&r.extent.x, &r.extent.y);
		r.extent.x -= 3;		// leave room for the two pixel shadow
		r.extent.y = FntLineHeight () + 1;

		menuP->barWin = WinCreateWindow (&r, menuFrame, false, false, &error);
		// Save the bits behind the popup window.
		WinGetWindowFrameRect (WinGetWindowHandle(menuP->barWin), &frame);
		frame.extent.y++;					// expand bounds so there is space between menu bar and background
		menuP->bitsBehind = WinSaveBits (&frame, &error);
		}

	// Erase the window's backgroud and draw the windows frame.
	WinSetForeColor(UIColorGetIndex(UIMenuFrame));
	WinSetBackColor(UIColorGetIndex(UIMenuFill));
	
	WinEraseRectangle (&frame, 0);

	WinSetDrawWindow (WinGetWindowHandle(menuP->barWin));
	WinDrawWindowFrame();


	// Draw all the menu titles
	for (i=0; i < menuP->numMenus; i++)
		DrawOneTitle(menuP, i, i == menuP->curMenu);

	WinEnableWindow (WinGetWindowHandle(menuP->barWin));
	menuP->savedActiveWin = WinGetActiveWindow ();
	WinSetActiveWindow (WinGetWindowHandle(menuP->barWin));

	DrawMenuPulldown (menuP);

	WinPopDrawState();
	}


/***********************************************************************
 *
 * FUNCTION:    MenuHandleEvent
 *
 * DESCRIPTION: Handle event in the specified menu.  This routine
 *              handles two type of events, penDownEvents and
 *              menuEnterEvent.
 *
 *              When this routine receives a penDownEvent it checks
 *              if the pen position is within the bounds of the
 *              menu object, if it is this routine tracks the pen
 *              until the pen comes up.  If the pen comes up
 *              within the bounds of the menu, a menuEnterEvent is
 *              added to the event queue, and the routine is exited.
 *
 *              When this routine receives a menuEnterEvent it
 *              checks that the menu id in the event record match
 *              the id of the menu specified, if there is a match
 *              this routne will create and display a popup window
 *              containing the menu's choices, and the routine is exited.
 *
 *              If a penDownEvent is received while the menu's popup
 *              window is displayed, and the the pen postion is outside the
 *              bounds of the popup window, the pulldown will be dismissed.
 *              If the pen postion is within the bounds of the window
 *              the this routne will track the pen until it comes up.
 *              If the pen comes up in the menu a menuExitEvent will be
 *              added to the event queue.
 *
 *
 * PARAMETERS:	 menuP    - pointer to menu object
 *              event    - pointer to an EventType structure.
 *              errorP    - pointer to returned erro code
 *
 * RETURNED:	 true if the event was handle or false if it was not.
 *              errorP     - error code.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/28/94	Initial Revision
 *			roger	6/6/96	Cleaned up the way menus exit
 *			trev	08/11/97	made non modified passed variables constant
 *			jhl	7/9/98	added check for noMenuSelection before using
 *								GetPulldownMenu()
 *			kwk	07/15/98	Only call GetPulldownMenu if there's a menu pulled down.
 *			jaq	6/3/99	Pulled out command bar stuff into separate routine
 *			gavin	8/2/99   reworked for dynamic menus (BTW, menuP and error are both bogus)
 *
 ***********************************************************************/
Boolean MenuHandleEvent (MenuBarType * menuP, EventType * event, 
	UInt16 * errorP)
	{
	WinHandle curWin;
	Boolean handled = false;
	FontID currFont;


	*errorP = 0;  // never used (always returns zero)

	// DOLATER ??? - the menu parameter to this routine should be removed,
	// it's nolonger used.
	if (! menuP)
		menuP = UICurrentMenu;
	
	// Verify that the menu is valid.  Some older apps still don't pass 
	// zero for menuP and also mismanaged the pointer.  Catch them.
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	if (menuP != NULL)
      MemPtrSize(menuP);
#endif


	if (event->eType == penDownEvent)
		{
		//if (!menuP) return false;
		
		if (menuP && menuP->attr.visible)
			{
			curWin = WinGetActiveWindow();
			currFont = FntSetFont (menuFont);
	
			if (menuP->barWin == curWin)
				{
				handled = MenuBarHandleEvent(menuP, event, errorP);
				}
			else if ((menuP->curMenu != noMenuSelection) &&
							(GetPulldownMenu(menuP)->menuWin == curWin))
				{
				handled = MenuPulldownHandleEvent(menuP, event);
				}
			else
				{
				MenuEraseMenu (menuP, removeCompleteMenu);
				MenuEraseStatus (0);
				handled = true;
				}
	
			FntSetFont (currFont);
			}
		else
			{
			//menuP->attr.commandPending = false;
			//MenuEraseStatus (0);
			}
		}
		
	else if (event->eType == keyDownEvent)
		{
		if ((event->data.keyDown.chr == commandChr) ||
			 (event->data.keyDown.chr == menuChr)  )
			{
			MenuEraseStatus(0);  // in case it is up
			// if the menu is not intiialized, init it and send menuOpen event and exit
			if (!menuP)
				{
				if (UICurrentMenuRscID)
					{
					EventType myEvent;

					UICurrentMenu = MenuInit (UICurrentMenuRscID);
					menuP = UICurrentMenu;
					// Stuff a menu open Event to let the system know we have a new menu
					MemSet(&myEvent,sizeof(EventType),0);
					myEvent.eType = menuOpenEvent;
					//myEvent.data.menuOpen.pMenu = UICurrentMenu;
					if (event->data.keyDown.chr == menuChr)
						myEvent.data.menuOpen.cause = menuButtonCause;
					else
						myEvent.data.menuOpen.cause = menuCommandCause;
					EvtAddEventToQueue(&myEvent);
					event->data.keyDown.modifiers |= autoRepeatKeyMask; //this is an evil hack
							//to tell SysHandleEvent not to click again when it sees the requeued event.
					EvtAddEventToQueue(event); // requeue key event to be handled after open
					handled = true;
					}
				}
			
			// if handling a command char, send CmdCharOpenEvent and exit	
			else if (event->data.keyDown.chr == menuChr)
				{
				if (! menuP->attr.visible)
					MenuDrawMenu (menuP);
				else
					MenuEraseMenu (menuP, removeCompleteMenu);				
				handled = true;
				}
			
			else if (event->data.keyDown.chr == commandChr)
				{
				// if the menu is open and they write a command stroke, close the menu
				// and switch to the command bar.  (Having both open simultaneously
				// would be asking for trouble)
				if (menuP->attr.visible)
					MenuEraseMenu (menuP, removeCompleteMenu);
				}
			} 
		// other keys will close the menu if it is up
		else if (menuP && menuP->attr.visible)
			{
			MenuEraseMenu (menuP, removeCompleteMenu);				
			}	
		}
	
	// handle command bar events here so the apps don't need another call.
	// DOLATER (bob) make this a real trap so we don't have inter-module jump problems
	if (!handled)
		handled = PrvMenuCmdBarHandleEvent(event, errorP);

	return (handled);
	}
	
	
	

/***********************************************************************
 *
 * FUNCTION:    MenuShowItem
 *
 * DESCRIPTION: show an menu item if it is hidden
 *
 * PARAMETERS:	 id - the id of the menu item to show
 *
 * RETURNED:	 true if item was found
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			gavin	8/2/99	Initial Revision
 *
 ***********************************************************************/
Boolean MenuShowItem(UInt16 id)
{
	return PrvMenuHideItem(id,false);
}


/***********************************************************************
 *
 * FUNCTION:    MenuHideItem
 *
 * DESCRIPTION: hide an menu item if it is not hidden
 *
 * PARAMETERS:	 id - the id of the menu item to hide
 *
 * RETURNED:	 true if item was found
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			gavin	8/2/99	Initial Revision
 *
 ***********************************************************************/
Boolean MenuHideItem(UInt16 id)
{
	return PrvMenuHideItem(id,true);
}
	
	
/***********************************************************************
 *
 * FUNCTION:	 MenuAddItem
 *
 * DESCRIPTION: Add a new item to a menu
 *
 * PARAMETERS:	 positionId -> id of menu item _after which_ to add the new item.
 *					 id			-> id of new menu item.
 *					 cmd			-> command char for new menu item.
 *					 textP		-> ptr to text to use for title (gets copied)
 *
 * RETURNED:	 0 if no error, otherwise result.
 *
 * HISTORY:
 *		08/02/99	gavin	Created by Gavin Peacock
 *		08/05/99	kwk	Fixed const usage in API. Use parens to fix ptr calc in
 *							inner loop. Only expand menu if positionId is found.
 *							Return error if duplicate id exists.
 *		11/03/99	kwk	Increment menu count before calling PrvUpdateMenuPtrs,
 *							as otherwise inserting doesn't work (last menu item
 *							text ptr doesn't get updated). Also fixed calc for
 *							amount of data to move when inserting menu item.
 *
 ***********************************************************************/
Err MenuAddItem(UInt16 positionId, UInt16 id, Char cmd, const Char* textP)
{
	MenuBarPtr menuP;
	MemHandle menuH;
	UInt16 i, j;
	UInt16 menuIndex, menuItem;
	Err err = errNone;
	Boolean foundIt = false;
	
	if (!textP)
		{
		ErrNonFatalDisplay("Menu item with no text");
		return menuErrNoMenu;
		}
	
	menuP = UICurrentMenu; // use current menu
	if (! menuP) // if we have no menu structure, then init one from current menu resource
		{
		if (! UICurrentMenuRscID)
			return (menuErrNoMenu);
		
		UICurrentMenu = MenuInit (UICurrentMenuRscID);
		menuP = UICurrentMenu;
		}
	
	ErrFatalDisplayIf(!menuP,"Error initializing menu");
	
	// now find the item where we need to insert our new item
	for (i=0; i< menuP->numMenus; i++)
		{
		for (j=0; j < menuP->menus[i].numItems; j++)
			{
			UInt16 curID = menuP->menus[i].items[j].id;
			if (curID == positionId)
				{
				foundIt = true;
				menuIndex = i;
				menuItem = j;
				// keep looping to check for duplicate id
				}
			else if (curID == id)
				return menuErrSameId;
			}
		}
	
	if (!foundIt)
		{
		return menuErrNotFound;
		}
	else
		{
		MenuItemType * itemP;
		UInt8 *			ptr;
		Char * 			str;
		UInt32 blkSize;
		UInt32 offset;
		UInt32 bytesToMove;
		UInt32 originalSize;
		UInt32 itemSize = sizeof(MenuItemType);
	
		// resize the menu block to make room for the new item	
		menuH = MemPtrRecoverHandle(menuP);
		MemHandleUnlock(menuH);
		originalSize = MemHandleSize(menuH);
		err = MemHandleResize(menuH, originalSize+sizeof(MenuItemType)+StrLen(textP)+1);
		ErrNonFatalDisplayIf(err,"Error resizing menu"); 
		if (err) return err;  // unable to allocate a bigger block
		menuP = MemHandleLock(menuH);
		
		if (menuP != UICurrentMenu)  // allocated a new block (couldn't grow old one...)
		{	// if resize allocated a new block, update the pointers
			PrvUpdateMenuPtrs(menuP,(UInt8*)UICurrentMenu,(Int32)menuP-(Int32)UICurrentMenu);
			UICurrentMenu = menuP;
		}
		
		// Watch out for address calcs like this - precedence matters!
		itemP = &(menuP->menus[menuIndex].items[menuItem+1]);
		ptr = (UInt8 *) itemP;
		blkSize = MemPtrSize(menuP);
		offset = ptr-(UInt8*) menuP;
		bytesToMove = originalSize - offset;
		MemMove(ptr+itemSize, ptr, bytesToMove);
		MemSet(itemP, itemSize, 0); // init new item to zeros
		// update all ptr references to data that has moved
		menuP->menus[menuIndex].numItems++;
		PrvUpdateMenuPtrs(menuP, ptr, itemSize);
		// now store the new text and update the string reference
		// the text will be stored at the end of the block (room already allocated there)
		str = ((Char *)menuP) + (blkSize-(StrLen(textP)+1));
		StrCopy(str,textP);  // move the string
		
		// now stuff data into the new item
		itemP->id = id;
		itemP->command = cmd;
		itemP->itemStr = str;  
		
		PrvCalcItemBounds(menuP, menuIndex);
		
		return errNone;
		}
}

#if 0
/***********************************************************************
 *
 * FUNCTION: 	 MenuGetMenuItem
 *
 * DESCRIPTION: This routine checks if a key is a menu command key, if
 *              it is a returns a pointer to the name of that menu item.
 *					 ( this is still a work in progress...
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
 *
 ***********************************************************************/
Boolean MenuGetMenuItem ( UInt16 *idP,
		 Char ** nameP, Char * cmdP, Boolean *hiddenP)
{
	Int16 i, j;
	UInt16 id = 0;
	Char   key = 0;
	MenuBarType *menuP = UICurrentMenu;
	if (!menuP)		// just checking...
		return false;

	if (idP && *idP) id = *idP;
	if (cmdP && *cmdP) key = *cmdP;
	
	// Convert the key to uppercase.
	if ((key >= 'a') && (key <= 'z'))
		key -= ('a' - 'A');

	for (i = 0; i < menuP->numMenus; i++)
		{
		for (j = 0; j < menuP->menus[i].numItems; j++)
			{
			if (menuP->menus[i].items[j].hidden) continue; // skip hidden items
			if (key ? menuP->menus[i].items[j].command == key
					: menuP->menus[i].items[j].id == id)
				{
				if (idP)
					*idP = menuP->menus[i].items[j].id;
				if (nameP)
					*nameP = menuP->menus[i].items[j].itemStr;
				if (hiddenP)
					*hiddenP = menuP->menus[i].items[j].hidden;
				return (true);
				}
			}
		}
	return (false);
}

#endif

/***********************************************************************
 *
 * FUNCTION:    PrvMenuHideItem
 * *
 * DESCRIPTION: hide or show a menu item. This is the guts of the MenuSHowItem
 * and MenuHideItem functions
 *
 * PARAMETERS:	 id - the id of the menu item to hide
 *					 hideit - true if the item should be hidden, otherwise show it.
 *
 * RETURNED:	 true if item was found
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			gavin	8/2/99	Initial Revision
 *			mt		05/02/00	Add capability to hide an entire menu pull down
 *
 ***********************************************************************/
static Boolean PrvMenuHideItem(const UInt16 id,Boolean hideIt)
{
	UInt16 i,j;
	Err err = 0;
	MenuPullDownPtr pullDownP;
	MenuBarPtr menuP = UICurrentMenu;
	
	ErrNonFatalDisplayIf(!menuP,"No Active Menu");
	if (!menuP) return (false);
	
	for (i=0; i< menuP->numMenus; i++)
		{
		pullDownP = &menuP->menus[i];
		for (j=0;j<pullDownP->numItems;j++)
			if (pullDownP->items[j].id == id)
				{
				if (pullDownP->items[j].hidden != hideIt)
					{ 
					pullDownP->items[j].hidden = hideIt;

					if (hideIt)
						{
						// Check if all items are hidden, if so, hide the entire menu
						for (j=0;j<pullDownP->numItems;j++)
							if (pullDownP->items[j].hidden ==false)
								{
								hideIt = false;
								break;
								}
						menuP->menus[i].hidden = hideIt;
						if ((hideIt) && (i ==menuP->curMenu))
							{
							// Search for a visible menu to set current menu
							menuP->curMenu = noMenuSelection;
							for (j=0;j<menuP->numMenus;j++)
								if (menuP->menus[j].hidden ==false)
									{
									menuP->curMenu = j;
									break;
									}
							}
						}
					else
						{	// Show any item makes the menu visible
							menuP->menus[i].hidden = false;
						}

					PrvCalcItemBounds(menuP, i);
					}
				return true;
				}
		}
	return false;
}

/***********************************************************************
 *
 * FUNCTION:    PrvUpdateMenuPtrs
 *
 * DESCRIPTION: adjust the pointer values in a menu after something has
 *					 been added or removed from the menu block
 *					 This assumes the menu data is a single contiguous memory
 *					block as loaded from a resource...
 *
 * PARAMETERS:	 menuP - pointer to the menu block
 *					 baseP - position that data was added or removed
 *					 delta - number of bytes added or removed
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			gavin	8/2/99	Initial Revision
 *
 ***********************************************************************/
static void PrvUpdateMenuPtrs(MenuBarPtr menuP, UInt8 *baseP, Int32 delta)
{
	int i,j;
	UInt32 * addrP;
	UInt32 base = (UInt32) baseP;
	
	// Fixup the pointers to the menus 
	// this always comes just after menu strucure so don't bother with delta
	menuP->menus = (MenuPullDownPtr) (menuP+1);


	// Fixup the pointers to the items in each pulldown menu.
	for (i = 0; i < menuP->numMenus; i++)
		{
		addrP = (UInt32 *) &menuP->menus[i].title;
		if (base <= *addrP)
			*addrP += delta;


		// Pointer to an array of items
		addrP = (UInt32 *) &menuP->menus[i].items;
		if (base <= *addrP)
			*addrP += delta;


		// Fixup the pointers in each menu item			
		for (j = 0; j < menuP->menus[i].numItems; j++)
			{
			addrP = (UInt32 *) &menuP->menus[i].items[j].itemStr;
			if (base <= *addrP)
				*addrP += delta;
			}
		}
}

	
/***********************************************************************
 *
 * FUNCTION:    PrvCalcItemBounds
 *
 * DESCRIPTION: recalculate the bounds of all menus
 * 				 The two sources should be kept in sync.
 *
 * PARAMETERS:	 menuP - pointer to menu structure
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			gavin	8/2/99	Initial Revision
 *			mt		05/02/00	Add possibility to remove entire menu
 *								by removing all items in the menu
 *
 ***********************************************************************/
static void PrvCalcItemBounds(MenuBarPtr menuP, UInt16 menuStartIndex)
{	
	FontID			curFont;
	Int16				displayWidth;
	Int16				displayHeight;
	Char *			str;
	Int16				width;
	Int16				j;
	UInt16			menuIndex;
	MenuPullDownPtr pullDownP;
	RectanglePtr	boundsP;
	UInt16			menuX;

	curFont = FntSetFont (boldFont);

	// get rid of any window that may have been allocated
	if (menuP->curMenu !=noMenuSelection)
	{
		if (menuP->menus[menuP->curMenu].menuWin)
		{
			WinDeleteWindow (menuP->menus[menuP->curMenu].menuWin, false);
			menuP->menus[menuP->curMenu].menuWin = NULL;
		}
	}

	menuX = menuP->menus[menuStartIndex].titleBounds.topLeft.x;

	// Recalculate all menus
	for (menuIndex = menuStartIndex; menuIndex < menuP->numMenus; menuIndex++)
	{
		if (menuP->menus[menuIndex].hidden ==false)
		{
			// Now recalculate the Pull Down
			pullDownP = &menuP->menus[menuIndex];
			boundsP = &pullDownP->bounds;

			// Set the Menu title
			pullDownP->titleBounds.topLeft.x = menuX;

			// reset menu to be aligned with its title
			// (this may change if the pulldown text needs to be realigned)
			boundsP->topLeft.x = menuX +2;
			boundsP->extent.x = 0;
			boundsP->extent.y = 0;
			for (j = 0; j < pullDownP->numItems; j++)
				{
				// skip over hidden items
				if (pullDownP->items[j].hidden) continue;
				
				str = (char *)pullDownP->items[j].itemStr;
				
				if (str[0] == MenuSeparatorChar)
					{
					boundsP->extent.y += FntLineHeight () / 2;
					continue;
					}
					
				width = FntCharsWidth ((Char *)str, StrLen(str)) + 3 + 3;
				
				// If there is a menu command more room is required.
				if (pullDownP->items[j].command)
					{
					width += FntCharWidth('W') + FntCharWidth(chrCommandStroke) + 
						FntCharWidth(pullDownP->items[j].command) - 1;  // last char column is blank
					}
				
				if (width >= boundsP->extent.x)
					{
					boundsP->extent.x = width;
					}
				
				boundsP->extent.y += FntLineHeight ();
				}
				
			// now do some work to make sure things fit on the display
			WinGetDisplayExtent (&displayWidth, &displayHeight);
			if (boundsP->extent.x >= displayWidth) 
				boundsP->extent.x = displayWidth;
				
			if (boundsP->extent.y >= displayHeight) 
				boundsP->extent.x = displayHeight;

			// Make sure the whole menu is on the display.
			boundsP->topLeft.x = min (boundsP->topLeft.x, displayWidth-boundsP->extent.x-2);

			menuX += pullDownP->titleBounds.extent.x;
		}
	}
	
	FntSetFont (curFont);
}	//	PrvCalcItemBounds
