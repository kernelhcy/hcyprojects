/******************************************************************************
 *
 * Copyright (c) 1994-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: Day.c
 *
 * Release: 
 *
 * Description:
 *	  This file contain routines to draw the month area.
 *
 * History:
 *		November 11, 1994	Created by Roger Flores
 *      mm/dd/yy   initials - brief revision comment
 *
 *****************************************************************************/

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_CLIPBOARDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_FIELDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS

#include <SystemPublic.h>

#define NON_PORTABLE

#include "Day.h"
#include "Event.h"
#include "UIResourcesPrv.h"
#include "UIGlobals.h"

#define noItemSelection		 -32  // -1 is the last day of the prior month
#define daysInWeek				7
#define linesInMonthPlusTitle	7

// These are placed in the beginning to avoid declaring them.

#define FirstDayInVisibleMonth(p) \
			(DayOfWeek(p->visibleMonth, 1, p->visibleYear))
			
			
#define FirstDayInSelectedMonth(p) \
			(DayOfWeek(selectorP->selected.month, 1, p->selected.year))

// These are for use with DaySelectItem().

#define dayDrawItemSelected	true
#define dayDrawItemUnselected	false

/***********************************************************************
 *
 * FUNCTION: 	 MapPointToItem
 *
 * DESCRIPTION: Return return the item at x, y.
 *
 * PARAMETERS:  x, y - coordinate
 *              r - the bounds of the item area (not the MTWTFSS area)
 *
 * RETURNED:	 item number (doesn't check for invalid bounds!)
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/15/94	Initial Revision
 *
 ***********************************************************************/
static Int16 MapPointToItem (Int16 x, Int16 y, const RectangleType *r)
{
	Int16 itemNumber;

	itemNumber = daysInWeek * (((y - r->topLeft.y) /
		(r->extent.y / linesInMonthPlusTitle)) - 1);
	itemNumber += ((x - r->topLeft.x) / (r->extent.x / daysInWeek));
	return itemNumber;
}


/***********************************************************************
 *
 * FUNCTION: 	 MapDateToItem
 *
 * DESCRIPTION: Return return the item for the date.
 *
 * PARAMETERS:  selectorP	  - pointer to control object to draw
 *
 * RETURNED:	 item number (doesn't check for invalid dates!)
 *
 * HISTORY:
 *		11/15/94	roger	Created by Roger Flores.
 *		10/04/95	roger	Fixed to handle weekStartDay
 *		09/07/00	kwk	Use PrefGetPreference vs. PrefGetPreferences.
 *
 ***********************************************************************/
static Int16 MapDateToItem(const DaySelectorType *selectorP)
{
	Int16 weekStartDayOffset;
	Int16 wrappedDay;
	
	if (selectorP->visibleMonth == selectorP->selected.month &&
		 selectorP->visibleYear == selectorP->selected.year)
		{
		weekStartDayOffset = PrefGetPreference(prefWeekStartDay);
		wrappedDay = FirstDayInSelectedMonth(selectorP) - weekStartDayOffset;
		if (wrappedDay < 0) wrappedDay += daysInWeek;			// wrap around
		return selectorP->selected.day - 1 + wrappedDay;
		}
	else
		return noItemSelection;
}


/***********************************************************************
 *
 * FUNCTION: 	 DayDrawDaySelectorGuts
 *
 * DESCRIPTION: Draw a control object on screen.
 *					 The control is drawn only if it is usable.
 *
 * PARAMETERS:  selectorP - pointer to control object to draw
 *
 * RETURNED:	 nothing
 *
 * HISTORY:
 *		11/17/99	jmp	Initial revision.  Crafted DayDrawDaySelectorGuts from
 *							DayDrawDaySelector.  Separated the two functions out
 *							so that we can eliminate WinInvertRectangle() calls.
 *		09/07/00	kwk	Modified to use DateTemplateToAscii versus referencing
 *							the obsolete day name string resource.
 *
 ***********************************************************************/
static void DayDrawDaySelectorGuts (const DaySelectorType *selectorP)
{
	Int16				drawX, drawY;
	Int16				cellWidth, cellHeight;
	Int16				charWidth;
	Int16				lastDay;
	Int16				textOffset;
	Char				dayInAscii[3];
	Char				bracketChar;
	Int8				i;
	Int16				x;
	DateTimeType	today;
	RectangleType	bounds;
	Int16				weekStartDayOffset;
	Int16				wrappedDay;

	weekStartDayOffset = PrefGetPreference(prefWeekStartDay);

	cellWidth = selectorP->bounds.extent.x / daysInWeek;
	cellHeight = selectorP->bounds.extent.y / linesInMonthPlusTitle;
	
	WinPushDrawState();
	FntSetFont (boldFont);
	charWidth = FntCharWidth('0');
	textOffset = (cellWidth - 2 * charWidth) / 2;

	// Draw the days of the week labels right justified to the number columns.
	// Use the form's preset color scheme for these.
	for (i=0; i < daysInWeek; i++)
		{
		Char shortDOWName[dowDateStringLength];
		
		// We know that 09/03/2000 is a Sunday.
		DateTemplateToAscii("^1s", 9, 3 + i + weekStartDayOffset, 2000, shortDOWName, sizeof(shortDOWName) - 1);
		shortDOWName[sizeof(shortDOWName) - 1] = chrNull; //fixes bug #46662 by HOu, NULL terminal was not set: risk for StrLen to overlap
		WinDrawChars(	shortDOWName,
							StrLen(shortDOWName),
							selectorP->bounds.topLeft.x + cellWidth * (i + 1) - textOffset -
								FntCharsWidth(shortDOWName, StrLen(shortDOWName)),
							selectorP->bounds.topLeft.y - 1);		// -1 so that the first row has space to 
																				// draw the font descender without it getting chopped off
																				// Fixes bug 42499
		}

	// For the date cells and the rest of the day selector gadget area, use
	// a control-like color scheme so that our "standard" selection/unselection
	// process functions.
	WinSetBackColor(UIColorGetIndex(UIObjectFill));
	WinSetForeColor(UIColorGetIndex(UIObjectForeground));
	WinSetTextColor(UIColorGetIndex(UIObjectForeground));
	
	// Determine which cell the first day is drawn. Used to draw the gadget 
	// area and to draw the numbers.
	x = FirstDayInVisibleMonth(selectorP) - weekStartDayOffset;
	if (x < 0) x += daysInWeek;			// wrap around
	lastDay = DaysInMonth(selectorP->visibleMonth, selectorP->visibleYear);
	
	// Draw the gadget area to indicate that it's selectable.
	bounds.topLeft.x = selectorP->bounds.topLeft.x;
	bounds.topLeft.y = selectorP->bounds.topLeft.y + cellHeight;
	bounds.extent.x = cellWidth * daysInWeek;
	bounds.extent.y = cellHeight * ((x + lastDay) / daysInWeek + 1) + 1;
//	WinSetBackColor(UIColorGetIndex(UIObjectFillAlt1));
	WinEraseRectangle(&bounds, 4);

	// Draw the days in the prior month differently.
	bounds.topLeft.x = selectorP->bounds.topLeft.x;
	bounds.topLeft.y = selectorP->bounds.topLeft.y + cellHeight;
	bounds.extent.x = cellWidth * x;
	bounds.extent.y = cellHeight + 1;
//	WinSetBackColor(UIColorGetIndex(UIObjectFillAlt2));
	WinEraseRectangle(&bounds, 4);
//	WinSetBackColor(UIColorGetIndex(UIObjectFillAlt1));

	// Draw the days of the month.
	FntSetFont (stdFont);
	charWidth = FntCharWidth('0');
	textOffset = (cellWidth - 2 * charWidth) / 2;
	
	drawX = selectorP->bounds.topLeft.x + x * cellWidth - textOffset;
	drawY = selectorP->bounds.topLeft.y + cellHeight;
	for (i=1; i <= lastDay; i++, x++)
		{
		if (x == daysInWeek)
			{
			drawX = selectorP->bounds.topLeft.x - textOffset;
			drawY += cellHeight;
			x = 0;
			}
		drawX += cellWidth;
		StrIToA(dayInAscii, i);
		WinDrawChars(dayInAscii, (i < 10) ? 1 : 2,
					drawX - ((i < 10) ? charWidth : 2 * charWidth), drawY);
		}

	// Draw the days in the next month differently.
	bounds.topLeft.x = drawX + textOffset;
	bounds.topLeft.y = drawY;
	bounds.extent.x = cellWidth * (daysInWeek - x);
	bounds.extent.y = cellHeight + 1;
//	WinSetBackColor(UIColorGetIndex(UIObjectFillAlt2));
	WinEraseRectangle(&bounds, 4);

	// Display a "rectangle" around today's day if it's visible.
	TimSecondsToDateTime(TimGetSeconds(), &today);
	if (selectorP->visibleMonth == today.month &&
		 selectorP->visibleYear == today.year)
		{
		// "i" keeps track of the unshown days before this month.
		wrappedDay = FirstDayInVisibleMonth(selectorP) - weekStartDayOffset;
		if (wrappedDay < 0) wrappedDay += daysInWeek;			// wrap around
		today.day = today.day - 1 + wrappedDay;
		drawX = selectorP->bounds.topLeft.x +
			(today.day % daysInWeek) * cellWidth + 1;
		drawY = selectorP->bounds.topLeft.y +
			((today.day / daysInWeek) + 1) * cellHeight;
//		WinSetBackColor(UIColorGetIndex(UIObjectFillAlt1));
		bracketChar = '(';
		WinDrawChars(&bracketChar, 1, drawX, drawY);
		bracketChar = ')';
		WinDrawChars(&bracketChar, 1, drawX + cellWidth - 5, drawY);
		}

	WinPopDrawState();
	}


/***********************************************************************
 *
 * FUNCTION:    DayDrawInversionEffect
 *
 * DESCRIPTION: This routine does an inversion effect by swapping colors
 *					 this is NOT undoable by calling it a second time, rather
 *					 it just applies a selected look on top of already
 *					 rendered data.  (It's kind of a hack.)
 *
 * PARAMETERS:	 rP  - pointer to a rectangle to 'invert'
 *					 cornerDiam	- corner diameter
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	10/28/99	Initial revision
 *			jmp	11/17/99	Modified for use with the day selector (i.e., backcolor
 *								changed to UIObjectFill).
 *
 ***********************************************************************/
static void DayDrawInversionEffect (const RectangleType *rP, UInt16 cornerDiam) 
{
	WinPushDrawState();
	WinSetDrawMode(winSwap);
	WinSetPatternType (blackPattern);
	WinSetBackColor(UIColorGetIndex(UIObjectFill));
	WinSetForeColor(UIColorGetIndex(UIObjectSelectedFill));
	WinPaintRectangle(rP, cornerDiam);
	
	if (UIColorGetIndex(UIObjectSelectedFill) != UIColorGetIndex(UIObjectForeground)) {
		WinSetBackColor(UIColorGetIndex(UIObjectForeground));
		WinSetForeColor(UIColorGetIndex(UIObjectSelectedForeground));
		WinPaintRectangle(rP, cornerDiam);
	}
	WinPopDrawState();
}


/***********************************************************************
 *
 * FUNCTION: 	 InvertItem -> DaySelectItem
 *
 * DESCRIPTION: Select or unselect an item.  The top left item is 0 and the
 *              one below is 7.
 *
 * PARAMETERS:  selectorP     - pointer to control object to draw
 *              itemNumber    - item to invert
 *					 selected		- if true, draw item selected, otherwise
 *										  draw item unselected
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/15/94	Initial Revision
 *			jmp	11/17/99	Updated to eliminate use of WinInvertRectangle() in our
 *								new colorized world; changed name from InvertItem() to
 *								DaySelectItem().
 *			jmp	11/19/99 Bracket all the drawing with WinScreenLock()/WinScreenUnlock()
 *								calls to eliminate flicker.
 *       jmp   12/02/99 Fix bug #24377.  Don't call WinScreenUnlock() if WinScreenLock()
 *                      fails.
 *                          
 ***********************************************************************/
static void DaySelectItem (const DaySelectorType *selectorP, Int16 itemNumber, Boolean selected)
{
	RectangleType r;
	RectangleType savedClip;
	Int16 x, y;
	UInt8 * lockedWinP;
	
	if (itemNumber == noItemSelection ||
		selectorP->selectDayBy == selectDayByMonth)
		return;

	x = itemNumber % daysInWeek;
	y = itemNumber / daysInWeek;
	if (selectorP->selectDayBy == selectDayByDay)
		{
		r.extent.x = selectorP->bounds.extent.x / daysInWeek;
		r.extent.y = selectorP->bounds.extent.y / linesInMonthPlusTitle;
		r.topLeft.x = selectorP->bounds.topLeft.x + x * r.extent.x;
		r.topLeft.y = selectorP->bounds.topLeft.y + (y + 1) * r.extent.y;
		}
	else if (selectorP->selectDayBy == selectDayByWeek)
		{
		r.extent.x = selectorP->bounds.extent.x;
		r.extent.y = selectorP->bounds.extent.y / linesInMonthPlusTitle;
		r.topLeft.x = selectorP->bounds.topLeft.x;
		r.topLeft.y = selectorP->bounds.topLeft.y + (y + 1) * r.extent.y;
		}
/*	else if (selectorP->selectDayBy == selectDayByMonth)
		{
		RctCopyRectangle(&selectorP->bounds, &r);
		
		// Don't include the day of week labels
		r.extent.y -= selectorP->bounds.extent.y / linesInMonthPlusTitle;
		r.topLeft.y += selectorP->bounds.extent.y / linesInMonthPlusTitle;
		}
*/
	else 
		ErrDisplay("Bad selectDayBy");
	
	// Grab an extra row of pixels for visual evenness.
	r.extent.y++;
	
	// Save the current clipping rectangle, and set a new one match
	// the size of the selected/unselected area.
	lockedWinP = WinScreenLock(winLockCopy);
	WinGetClip (&savedClip);
	WinClipRectangle (&r);
	WinSetClip (&r);
	
	// Always redraw selected first.
	DayDrawDaySelectorGuts (selectorP);
	
	// If desired, perform selection "magic."
	if (selected)
		DayDrawInversionEffect (&r, 4);
	
	// Restore previous clipping state.
	WinSetClip (&savedClip);
	if (lockedWinP)
		WinScreenUnlock();
}


/***********************************************************************
 *
 * FUNCTION: 	 DayDrawDaySelector
 *
 * DESCRIPTION: Draw a control object on screen.
 *					 The control is drawn only if it is usable.
 *
 * PARAMETERS:  selectorP - pointer to control object to draw
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	10/28/94	Initial Revision
 *			trm	08/07/97	made non modified passed variabls constant
 *			kwk	06/19/98	Modified for Japanese day names.
 *			jmp	11/17/99	Now call DayDrawSelectorGuts(), and then do selection.
 *
 ***********************************************************************/
extern void DayDrawDaySelector (const DaySelectorType *selectorP)
{
	DayDrawDaySelectorGuts(selectorP);

	// Draw the day selected if it's visible.
	if (selectorP->selectDayBy != selectDayByMonth)
		DaySelectItem(selectorP, MapDateToItem(selectorP), dayDrawItemSelected);
}


/***********************************************************************
 *
 * FUNCTION: 	 DayDrawDays
 *
 * DESCRIPTION: Draw only the days.  Used when the year or month
 *					 changes.  This avoids flicker from redrawing week titles.
 *
 * PARAMETERS:  selectorP - pointer to control object to draw
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/17/94	Initial Revision
 *			trm	08/07/97	made non modified passed variabls constant
 *
 ***********************************************************************/
extern void DayDrawDays (const DaySelectorType *selectorP)
{
	RectangleType	r, saveClip;

	if (!selectorP->visible)
		return;

	RctCopyRectangle (&(selectorP->bounds), &r);
	r.topLeft.y += r.extent.y / linesInMonthPlusTitle;
	// erase one extra row of pixels for the extra inverted item row
	r.extent.y -= r.extent.y / linesInMonthPlusTitle - 1;

	// Clip the weekday titles
	WinGetClip (&saveClip);
	WinClipRectangle (&r);
	WinSetClip (&r);
	WinEraseRectangle (&r, 0);
	DayDrawDaySelector (selectorP);
	WinSetClip (&saveClip);
}

/***********************************************************************
 *
 * FUNCTION: 	 AdjustSelectionIfSelectByWeek
 *
 * DESCRIPTION: If selectDayByWeek mode then selecting another day
 * is really selecting that week and the spec wants the same day of
 * the week to be the day picked instead of what the user selected.
 *
 * PARAMETERS:  selectorP - day selector info
 *					 currentSelection	- the currently selected day
 *					 proposedSelection - the proposed selection
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	4/26/96	Initial Revision
 *
 ***********************************************************************/
static void AdjustSelectionIfSelectByWeek(const DaySelectorType *selectorP, 
	Int16 currentSelection, Int16 *proposedSelection)
{
	if (selectorP->selectDayBy == selectDayByWeek)
		{
		*proposedSelection -= *proposedSelection % daysInWeek;
		*proposedSelection += currentSelection % daysInWeek;
		}
}


/***********************************************************************
 *
 * FUNCTION:    DayHandleEvent
 *
 * DESCRIPTION: MemHandle event in the specified control.  This routine
 *              handles two type of events, penDownEvents and
 *              controlEnterEvent.
 *
 *              When this routine receives a penDownEvents it checks
 *              if the pen position is within the bounds of the
 *              control object, if it is a dayEnterEvent is
 *              added to the event queue and the routine exits.
 *
 *              When this routine receives a dayEnterEvent it
 *              checks that the control id in the event record match
 *              the id of the control specified, if they match
 *              this routine will track the pen until it comes up
 *              in the bounds in which case daySelectEvent is sent.
 *              If the pen exits the bounds a dayExitEvent is sent.
 *
 *
 * PARAMETERS:	 selectorP - pointer to control object (ControlType)
 *              pEvent    - pointer to an EventType structure.
 *              pError    - pointer to returned erro code
 *
 * RETURNED:	true if the event was MemHandle or false if it was not.
 *             pError     - error code.
 *					posts a daySelectEvent with info on whether to use the date.
 *						a date is used if the user selects a day in the visible month.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	10/28/94	Initial Revision
 *			trm	08/07/97	made non modified passed variabls constant
 *			jmp	11/17/99	Update to calling DaySelectItem() rather than InvertItem().
 *			CS		08/15/00	Initialize new event structure.
 *
 ***********************************************************************/

Boolean DayHandleEvent
  (DaySelectorType *selectorP, const EventType * pEvent)
	{
	EventType	newEvent;
	Boolean		penDown;
	Boolean		inverted;
	Int16			x, y;
	Int16       newSelection, proposedSelection;
	RectangleType	r;
	Int16       localFirstDayInMonth;
	Int16       localDaysInMonth;
	Int16 		weekStartDayOffset;
	Int16			dateInMonth;

	if (!selectorP->visible)
		return (false);

	if (pEvent->eType != penDownEvent)
		return false;

	weekStartDayOffset = PrefGetPreference(prefWeekStartDay);
	localFirstDayInMonth = FirstDayInVisibleMonth(selectorP) - weekStartDayOffset;
	if (localFirstDayInMonth < 0) localFirstDayInMonth += daysInWeek;			// wrap around
	localDaysInMonth = DaysInMonth(selectorP->visibleMonth,
		selectorP->visibleYear);

	// Restrict the selection bounds.  The top should be moved down
	// to exclude selecting the day of week titles.  The bottom should
	// be moved up to the last week with a day in it.
	RctCopyRectangle (&(selectorP->bounds), &r);
	r.topLeft.y += r.extent.y / linesInMonthPlusTitle;
	r.extent.y = (r.extent.y / linesInMonthPlusTitle) *
		(((localFirstDayInMonth + localDaysInMonth - 1) / 7) + 1);

	if (RctPtInRectangle (pEvent->screenX, pEvent->screenY, &r))
		{
		newSelection = MapDateToItem(selectorP);
		proposedSelection = MapPointToItem (pEvent->screenX, pEvent->screenY,
														&selectorP->bounds);
		AdjustSelectionIfSelectByWeek(selectorP, newSelection, &proposedSelection);
		
		if (newSelection != proposedSelection)
			{
			DaySelectItem(selectorP, newSelection, dayDrawItemUnselected);
			newSelection = proposedSelection;
			DaySelectItem(selectorP, newSelection, dayDrawItemSelected);
			}
		inverted	= true;
		}
	else
		return false;

	penDown = true;

	do
		{
		PenGetPoint (&x, &y, &penDown);

		if (RctPtInRectangle (x, y, &r))
			{
			proposedSelection = MapPointToItem (x, y, &selectorP->bounds);
			AdjustSelectionIfSelectByWeek(selectorP, newSelection, &proposedSelection);
			
			if (penDown)
				{
				if (newSelection != proposedSelection)
					{
					if (inverted)
						DaySelectItem(selectorP, newSelection, dayDrawItemUnselected);
					newSelection = proposedSelection;
					DaySelectItem(selectorP, newSelection, dayDrawItemSelected);
					inverted = true;
					}
				}
			}
		else     //moved out of bounds
			{
			if (inverted)
				{
				DaySelectItem(selectorP, newSelection, dayDrawItemUnselected);
				inverted = false;
				}
			newSelection = noItemSelection;
			}
		} while (penDown);


	MemSet(&newEvent, sizeof(newEvent), 0);	// Initialize new event structure
	newEvent.penDown = penDown;
	newEvent.screenX = x;
	newEvent.screenY = y;


	if (newSelection == noItemSelection)
		{
		DaySelectItem(selectorP, MapDateToItem(selectorP), dayDrawItemUnselected);
		return true;
		}

	// If we are selecting by week make sure that the newSelection is
	// the same day of the week as the current date.  We do this now before
	// the following code which handles the cases where the date is outside
	// of valid ranges for the month that year.
	if (selectorP->selectDayBy == selectDayByWeek)
		{
		newSelection = (newSelection / daysInWeek) * daysInWeek + 
			DayOfWeek (selectorP->selected.month, selectorP->selected.day, selectorP->selected.year);
		}


	selectorP->selected.month = selectorP->visibleMonth;
	selectorP->selected.year = selectorP->visibleYear;
	if ((UInt16) (newSelection - localFirstDayInMonth) >=
		 (UInt16) localDaysInMonth)
		{
		// Because the selection is outside of the visible month don't automatically
		// use the date selected.
		// Spec changed to automatically use the date.  Saves taps in more cases.
		newEvent.data.daySelect.useThisDate = true;
		
		if (selectorP->selectDayBy == selectDayByMonth)
			{
			dateInMonth = selectorP->selected.day;
			selectorP->selected.day = 1;
			TimAdjust(&selectorP->selected, (newSelection - localFirstDayInMonth) * daysInSeconds);
			
			// Now that the month and year are correctly wrapped to the preceding or following
			// month, set the day to the one selected before.  MemHandle days past the end
			// of the month.
			localDaysInMonth = DaysInMonth(selectorP->selected.month, selectorP->selected.year);
			if (dateInMonth < localDaysInMonth)
				selectorP->selected.day = dateInMonth;
			else
				selectorP->selected.day = localDaysInMonth;
			}
		else
			{
			selectorP->selected.day = 1;
			TimAdjust(&selectorP->selected, (newSelection - localFirstDayInMonth) * daysInSeconds);
			}
		
		// The TimAdjust function may not actually change the time to a different month
		// if the user is on Jan. 1, 1904 and clicks to the left.  In this case
		// visually move the inverted item to the correct place because the month
		// ins't going to be redrawn.
		if (selectorP->selected.month == selectorP->visibleMonth && 
			MapDateToItem(selectorP) != newSelection)
			{
			DaySelectItem(selectorP, newSelection, dayDrawItemUnselected);
			DaySelectItem(selectorP, MapDateToItem(selectorP), dayDrawItemSelected);
			}
		}
	else
		{
		newEvent.data.daySelect.useThisDate = true;
		
		// if selectDayByMonth the granularity is at the month level so the user can't
		// pick a particular day within the month.  Just use the existing selection.
		if (selectorP->selectDayBy != selectDayByMonth)
			selectorP->selected.day = newSelection - localFirstDayInMonth + 1;
		}


	newEvent.eType = daySelectEvent;
	EvtAddEventToQueue (&newEvent);

	return true;
	}
