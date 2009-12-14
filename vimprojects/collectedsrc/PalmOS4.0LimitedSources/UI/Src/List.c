/******************************************************************************
 *
 * Copyright (c) 1994-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: List.c
 *
 * Release: 
 *
 * Description:
 *	  This file contain the list object routines.
 *
 * History:
 *		November 3, 1994	Created by Roger Flores
 *      2/9/99   bob - Fix up const stuff, add color support
 *
 *****************************************************************************/

#define NON_PORTABLE

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_FORMS
#define ALLOW_ACCESS_TO_INTERNALS_OF_LISTS
#define ALLOW_ACCESS_TO_INTERNALS_OF_CLIPBOARDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_FIELDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS

#include <PalmTypes.h>
#include <SystemPublic.h>

#include <PalmUtils.h>

#include "SysTrapsFastPrv.h"

#include "List.h"
#include "UIGlobals.h"
#include "UIColorPrv.h"
#include "AttentionPrv.h"

extern Int16 LstPopupListWithScrollBar (ListType * listP);


#define lstScrollBarWidth			7

#define lstPopupIndicatorOffset	12
#define maxVisiblePopupItems  	5
#define scrollIndicatorWidth		9
#define scrollIndicatorHeight    9
#define scrollUpIndicator  		symbolSmallUpArrow
#define scrollDownIndicator   	symbolSmallDownArrow
#define listFrame 					rectangleFrame
#define textInsetWidth				2

// define default values for list scroll rate
#define listScrollMinDelay     0
#define listScrollInitDelay    21
#define listScrollAcceleration 3


// Maximium length of the incremental search buffer.
#define maxListLookupLen		5


/***********************************************************************
 *
 *	Internal functions
 *
 ***********************************************************************/
Boolean PtAboveRect (Coord x, Coord y, RectangleType *r);
Boolean PtBelowRect (Coord x, Coord y, RectangleType *r);
static void PrvSetColors(Boolean selected);
static void PrvDrawItem (ListType * listP, Int16 itemNum, Boolean selected);
static Boolean PrvHandleScrollingIndicators (ListType * listP, const EventType * eventP);


/***********************************************************************
 *
 *	Handy function-like macros
 *
 ***********************************************************************/
#define VisibleItems(l)        (min((l->bounds.extent.y / FntLineHeight()), l->numItems))
#define LastVisibleItem(l)     (l->topItem + VisibleItems(l) - 1)
#define LastItem(l)            (l->numItems - 1)
#define ListScrolledToTop(l)   (l->topItem == 0)
#define ListScrolledToBottom(l) (LastItem(l) == LastVisibleItem(l))


/***********************************************************************
 *
 * FUNCTION:    ItemIsVisible
 *
 * DESCRIPTION: Return true if the item is visible.
 *
 * PARAMETERS:	 listP         - pointer to list object
 *              i					- item number
 *
 * RETURNED:	 true if the item is one of the visible items.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/3/94	Initial Revision
 *
 ***********************************************************************/
static Boolean ItemIsVisible (ListType * listP, Int16 i)
{
  return i >= listP->topItem &&
			i <= LastVisibleItem(listP);
}


/***********************************************************************
 *
 * FUNCTION:     LookupStringInList
 *
 * DESCRIPTION:  This routine seatches a list for a the string passed.
 *
 * PARAMETERS:   lst       - pointer to a list object
 *					  key       - string to lookup record with
 *					  indexP    - to contain the record found
 *            
 *
 * RETURNED:     true if a match was found.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/10/96	Initial Revision
 *
 ***********************************************************************/
static Boolean LookupStringInList (ListType * lst, Char * key, Int16 * indexP, 
	Boolean * uniqueP)
{
	Int16 	result;
	Int16 	numItems;
	Int16		keyLen;
	Int16		kmin, probe, i;
	Char *	itemP;
	Boolean	match = false;
	
	*uniqueP = false;
	
	if ((! key) || (! *key) ) return false;
		
	keyLen = StrLen (key);

	numItems = LstGetNumberOfItems (lst);
	if (numItems == 0) return false;
	
	result = 0;
	kmin = probe = 0;

	
	while (numItems > 0)
		{
		i = numItems / 2;
		probe = kmin + i;


		// Compare the a list item to the key.
		itemP = LstGetSelectionText (lst, probe);
		result = StrNCaselessCompare (key, itemP, keyLen);


		// If the date passed is less than the probe's date , keep searching.
		if (result < 0)
			numItems = i;

		// If the date passed is greater than the probe's date, keep searching.
		else if (result > 0)
			{
			kmin = probe + 1;
			numItems = numItems - i - 1;
			}

		// If equal stop here!  Make sure the match is unique by checking
		// the item before and after the probe,  if either matches then
		// we don't have a unique match.
		else
			{
			// Start by assuming we have a unique match.
			match = true;
			*uniqueP = true;
			*indexP = probe;

			// If we not have a unique match,  we want to return the 
			// index one the first item that matches the key.
			while (probe)
				{
				itemP = LstGetSelectionText (lst, probe-1);
				if (StrNCaselessCompare (key, itemP, keyLen) != 0)
					break;
				*uniqueP = false;
				*indexP = probe-1;
				probe--;
				}

			if (!*uniqueP) break;

			if (probe + 1 < LstGetNumberOfItems (lst))
				{
				itemP = LstGetSelectionText (lst, probe+1);
				if (StrNCaselessCompare (key, itemP, keyLen) == 0)
					*uniqueP = false;				
				}
			break;		
			}
		}
	return (match);
}


/***********************************************************************
 *
 * FUNCTION:    IncrementalSearch
 *
 * DESCRIPTION: Updated the incremental search buffer with the passed 
 *              character and search the list for a match.  If a match 
 *              is found it will be selected.
 *
 * PARAMETERS:  listP  - pointer to a list object
 *					 buffer - string to lookup record with
 *					 chr    - character to match against
 *                		
 * RETURNED:    true if a unique match has found.
 *
 * REVISION HISTORY:
 *		12/02/98	art	Written by Art Lamb
 *		07/01/99	kwk	Rolled in version that works w/multibyte chars.
 *		12/04/99	jmp	Save/restore drawing environment since we indirectly
 *							change it here (i.e., PrvDrawItem() assumes the
 *							caller will preserve the drawing environment).  Fixes
 *							bug #24394.
 *
 ***********************************************************************/
static Boolean IncrementalSearch (ListType * listP, Char * buffer, WChar chr)
{
	Int16 len;
	Int16 index;
	Boolean redraw = false;
	Boolean unique = false;

	if (! listP->attr.search)
		return (false);

	if (! (TxtCharIsPrint (chr) || TxtCharIsCntrl (chr)))
		return (false);
	
	len = StrLen (buffer);
	
	if (chr == backspaceChr)
		{
		if (len)
			{
			len -= TxtPreviousCharSize(buffer, len);
			buffer[len] = 0;
			}
		}
	else if (chr == linefeedChr)
		{
		return (listP->currentItem != noListSelection);
		}
	else if (len + TxtCharSize(chr) <= maxListLookupLen)
		{
		len += TxtSetNextChar(buffer, len, chr);
		buffer[len] = 0;
		}
	else
		return (false);

	// Check for a match.
	if ( ! LookupStringInList (listP, buffer, &index, &unique))
		{
		// If the character was a backspace and the auto buffer is empty
		// then set the list selection to none.
		if (chr == backspaceChr)
			{
			if (! len)
				index = noListSelection;
			}

		// If we didn't get a match, remove the character from the search key.
		else
			{
			SndPlaySystemSound (sndError);
			len -= TxtPreviousCharSize(buffer, len);
			buffer[len] = 0;
			return (false);
			}
		}

	// Display the new selection.
	if (index != listP->currentItem)
		{
		WinPushDrawState();
		
		// Unhighlight the old selection.
		PrvDrawItem(listP, listP->currentItem, false);

		listP->currentItem = index;
		
		// If there is no selection then scroll the list to the top.
		if (index == noListSelection)
			{
			if (listP->topItem)
				{
				listP->topItem = 0;
				redraw = true;
				}
			}

		// If the new selection is not visible make it visible.
		else if (! ItemIsVisible(listP, listP->currentItem))
			{
			ErrNonFatalDisplayIf(index < 0, "Invalid topItem");
			listP->topItem = min(index, listP->numItems - VisibleItems(listP));
			redraw = true;
			}

		// Otherwise the new selection was visible, so redraw it as selected.
		else
			PrvDrawItem(listP, listP->currentItem, true);
			
		if (redraw)
			LstDrawList (listP);
			
		WinPopDrawState();
		}

	return (unique);
}


/*--------------------------------------------------------*/
/*                    PtAboveRect                         */
/*--------------------------------------------------------*/
Boolean PtAboveRect (Coord x, Coord y, RectangleType *r)
{
	return ( (y < r->topLeft.y) && (x > r->topLeft.x) &&
				(x <= r->topLeft.x + r->extent.x) );
}

/*--------------------------------------------------------*/
/*                   PtBelowRect                          */
/*--------------------------------------------------------*/
Boolean PtBelowRect (Coord x, Coord y, RectangleType *r)
{

	return ( (y >= r->topLeft.y + r->extent.y) && (x > r->topLeft.x) &&
				(x <= r->topLeft.x + r->extent.x) );
}



#pragma mark ---------------------


/***********************************************************************
 *
 * FUNCTION:    LstScrollList
 *
 * DESCRIPTION: Scroll the list up or down a number of items.
 *
 * PARAMETERS:	 listP         - pointer to list object
 *					 direction	   - direction to scroll
 *					 itemCount		- items to scroll in direction
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/3/94	Initial Revision
 *			roger	9/23/96	Exported
 *
 ***********************************************************************/
Boolean LstScrollList(ListType * listP, WinDirectionType direction, Int16 itemCount)
{
	Int16 item, lastItem;
	RectangleType	r, vacated;
	
	WinPushDrawState();
	FntSetFont (listP->font);
	
	RctCopyRectangle (&(listP->bounds), &r);

	if (direction == winUp) // up arrow, actually contents scroll down
		{
		if (!ListScrolledToTop(listP))
			{
			itemCount = min(itemCount, listP->topItem);
			if (itemCount > 0)
				{
				listP->topItem -= itemCount;
				
				// scroll down all but top line
				r.topLeft.y += FntLineHeight();
				r.extent.y -= FntLineHeight();
				WinScrollRectangle (&r, winDown, itemCount * FntLineHeight(), &vacated);
				
				// draw top items
				lastItem = listP->topItem + itemCount;
				for (item = listP->topItem; item <= lastItem; item++)
					PrvDrawItem(listP, item, item == listP->currentItem);
				
				// redraw last item for scroll arrow
				item = LastVisibleItem(listP);
				PrvDrawItem(listP, item, item == listP->currentItem);
				}
			}
		else
			itemCount = 0;
		}
	else  // down arrow, actually contents scroll up
		{
		if (!ListScrolledToBottom(listP))
			{
			itemCount = min(itemCount, (listP->numItems - VisibleItems(listP))
								 - listP->topItem );
			if (itemCount > 0)
				{
				listP->topItem += itemCount;
				
				// scroll up all but bottom line
				r.extent.y -= FntLineHeight();
				WinScrollRectangle (&r, winUp, itemCount * FntLineHeight(), &vacated);
				
				// redraw first item for scroll arrow
				PrvDrawItem(listP, listP->topItem, listP->topItem == listP->currentItem);

				// draw bottom items
				lastItem = LastVisibleItem(listP);
				for (item = lastItem - itemCount; item <= lastItem; item++)
					PrvDrawItem(listP, item, item == listP->currentItem);
				}
			}
		else
			itemCount = 0;
		}

	WinPopDrawState();
	return (itemCount > 0);
}


/***********************************************************************
 *
 * FUNCTION:    LstDrawList
 *
 * DESCRIPTION: Draw a list object.  If the list is disabled it will
 *					 be drawn grayed-out.  If the list is not useable
 *					 the list will not draw.  If it is empty nothing is drawn.
 *
 * PARAMETERS:	 list - pointer to list object to draw
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	9/28/94		Initial Revision
 *			trev	08/11/97	made non modified passed variables constant
 *			CS		11/20/00	Don't forget to subtract topItem from the last
 *								visible item when calculating how many lines need
 *								to be erased.
 *
 ***********************************************************************/
void LstDrawList (ListType * listP)
{
	RectangleType	r, saveClip;
	WinHandle		curDrawWin;
   Int16				i;
	Int16          visibleItem;

	if (!listP->attr.usable) return;

	listP->attr.visible = true;

	// If there isn't any text or draw function don't try to draw!
	if ((listP->numItems == 0 && listP->bounds.extent.y <= 0) || 
		(listP->itemsText == NULL && !listP->drawItemsCallback && listP->numItems > 0) ) 
		return;

	if (listP->attr.poppedUp)
		curDrawWin = WinSetDrawWindow (WinGetDisplayWindow());

	WinPushDrawState();
	WinSetForeColor(UIColorGetIndex(UIObjectFrame));
	WinSetBackColor(UIColorGetIndex(UIObjectFill));
	WinSetTextColor(UIColorGetIndex(UIObjectForeground));
	
	if (listP->attr.poppedUp)
		{
		// Erase the window's background
		WinGetWindowFrameRect (listP->popupWin, &r);
		RctInsetRectangle (&r, -1);							// expand bounds so there is space between popup and background
		WinEraseRectangle (&r, 0);

		// Draw the popup list's frame
		WinSetDrawWindow (listP->popupWin);
		WinDrawWindowFrame ();
		}
	else
		WinDrawRectangleFrame (listFrame, &(listP->bounds));

	FntSetFont (listP->font);

	// Restrict the clipping to the list's bounds.
	WinGetClip (&saveClip);
	RctCopyRectangle (&(listP->bounds), &r);
	WinClipRectangle (&r);
	WinSetClip (&r);

	visibleItem = LastVisibleItem(listP) + 1;
	ErrNonFatalDisplayIf(visibleItem > listP->numItems, 
		"The list is scrolled too far down");
	
	// draw all the items
	for (i = listP->topItem; i < visibleItem; ++i)
		PrvDrawItem (listP, i, i == listP->currentItem &&  ItemIsVisible(listP, i));

	// draw any unused area at bottom in proper color
	// еее DOLATER  consider skipping this if background color is same as form background?
	RctCopyRectangle(&(listP->bounds), &r);
	r.extent.y -= (visibleItem - listP->topItem) * FntLineHeight();
	if (r.extent.y) {
		r.topLeft.y += (visibleItem - listP->topItem) * FntLineHeight();
		PrvSetColors(false);
		WinEraseRectangle(&r, 0);
	}

	// put draw stuff back
	WinSetClip (&saveClip);
	WinPopDrawState();
	if (listP->attr.poppedUp)
		WinSetDrawWindow (curDrawWin);
}


/***********************************************************************
 *
 * FUNCTION:    LstEraseList
 *
 * DESCRIPTION: Erase the bounds of the list object specified.
 *
 * PARAMETERS:	 list - pointer to list object to draw
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	9/28/94		Initial Revision
 *
 ***********************************************************************/
void LstEraseList (ListType * listP)
{
	RectangleType r;

	WinGetFramesRectangle (listFrame, &(listP->bounds), &r);
	WinEraseRectangle (&r, 0);
	listP->attr.visible = false;
}


/***********************************************************************
 *
 * FUNCTION:    LstGetSelection
 *
 * DESCRIPTION: Returns the current selection in the list.  If there 
 *              is no selection, returns NoListSelection (-1).
 *
 * PARAMETERS:	 list - pointer to list object
 *
 * RETURNED:	 currently selected list choice (0=first item, -1=none)
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	9/29/94		Initial Revision
 *			trev	08/11/97	Made non modified passed variables constant
 *			FPa		08/11/97	When the list is empty, LstGetSelection() was
 *								returning 0; my little workaround fixes the
 *								problem. Nevertheless, if we want to do a clean
 *								bug fix, we should set listP->currentItem to
 *								noListSelection by default.
 *
 ***********************************************************************/
Int16 LstGetSelection (const ListType * listP)
{
	if ( LstGetNumberOfItems(listP) == 0 )
		return noListSelection;
	else
		return (listP->currentItem);
}


/***********************************************************************
 *
 * FUNCTION:    LstSetSelection
 *
 * DESCRIPTION: Sets the selection for a list.  Old selection is unselected,
 *              if any.  If the list is visible the selected item is 
 *              visually updated.
 *
 * PARAMETERS:	 list    - pointer to list object
 *              itemNum - item to select (0=first item in list)
 *
 * RETURNED:	 nothing
 *
 * HISTORY:
 *		09/29/94	rsf	Created by Roger Flores.
 *		07/04/99	kwk	Check for invalid itemNum parameter.
 *		11/13/00	kwk	Improve checking for invalid itemNum parameter.
 *		11/15/00	kwk	As per Roger's feedback, always generate non-fatal
 *							alert even if list isn't usable/visible.
 *
 ***********************************************************************/
void LstSetSelection (ListType* listP, Int16 itemNum)
{
	Int16 oldItem = listP->currentItem;
	
	if ((itemNum != noListSelection)
	 && ((itemNum < 0) || (itemNum >= listP->numItems)))
	{
		ErrNonFatalDisplay("Invalid list item number");
		itemNum = noListSelection;
	}
	
	listP->currentItem = itemNum;
	
	if (listP->attr.usable && listP->attr.visible)
	{	
		WinPushDrawState();
		FntSetFont (listP->font);

		// visually remove the old selection
		PrvDrawItem(listP, oldItem, itemNum == oldItem);

	   // Hilite the new item, scrolling if necessary
	   if (itemNum != noListSelection)
	   {
	   	if (ItemIsVisible(listP, listP->currentItem))
	   	{
				PrvDrawItem(listP, itemNum, itemNum == listP->currentItem);
			}
			else
			{
				// Scroll
				listP->topItem = min(itemNum, listP->numItems - VisibleItems(listP));
				LstDrawList(listP);
			}
		}
		
		WinPopDrawState();
	}
}


/***********************************************************************
 *
 * FUNCTION:    LstGetNumberOfItems
 *
 * DESCRIPTION: Returns the number of items in a list.
 *
 * PARAMETERS:	 list - pointer to list object
 *
 * RETURNED:	 number of items in a list.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/24/94	Initial Revision
 *			trev	08/11/97	made non modified passed variables constant
 *
 ***********************************************************************/
Int16 LstGetNumberOfItems (const ListType * listP)
{
	return (listP->numItems);
}


/***********************************************************************
 *
 * FUNCTION:    LstSetTopItem
 *
 * DESCRIPTION: Set the the item visible.  The item may not become the
 * top item if it's in the last page.
 *
 * PARAMETERS:	 list    - pointer to list object
 *              itemNum - item to select (0=first item in list)
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	12/5/95	Initial Revision
 *			trev	08/11/97	made non modified passed variables constant
 *
 ***********************************************************************/
void LstSetTopItem (ListType * listP, const Int16 itemNum)
{
	FontID currFont;
	

	currFont = FntSetFont (listP->font);
	
	ErrNonFatalDisplayIf(((itemNum < 0) && (itemNum != noListSelection))
		|| (itemNum >= listP->numItems), "Invalid item");
	
	listP->topItem = min(itemNum, listP->numItems - VisibleItems(listP));
	
	FntSetFont (currFont);
}


/***********************************************************************
 *
 * FUNCTION:    LstGetTopItem
 *
 * DESCRIPTION: Return the top (visible) item.
 *
 * PARAMETERS:	 list    - pointer to list object
 *
 * RETURNED:	 top item
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			tlw	11/07/00	Initial Revision
 *
 ***********************************************************************/
Int16 LstGetTopItem (const ListType * listP)
{
	return listP->topItem;
}


/***********************************************************************
 *
 * FUNCTION:    LstGetVisibleItems
 *
 * DESCRIPTION: Return the number of items visible.
 *
 * PARAMETERS:	 listP         - pointer to list object
 *
 * RETURNED:	 the number of items visible
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/4/96	Initial Revision
 *			trev	08/11/97	made non modified passed variables constant
 *
 ***********************************************************************/
Int16 LstGetVisibleItems (const ListType * listP)
{
	Int16 visibleItems;
	FontID currFont;
	

	currFont = FntSetFont (listP->font);
	
	visibleItems = VisibleItems(listP);
	
	FntSetFont (currFont);
	
	return visibleItems;
}


/***********************************************************************
 *
 * FUNCTION:    LstMakeItemVisible
 *
 * DESCRIPTION: Makes the item visible, preferably at the top.
 *
 * PARAMETERS:	 list    - pointer to list object
 *              itemNum - item to select (0=first item in list)
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/10/94	Initial Revision
 *			roger	12/5/95	Change the top item only if the item isn't visible.
 *			kwk	07/05/99	Catch being passed noListSelection.
 *
 ***********************************************************************/
void LstMakeItemVisible (ListType * listP, const Int16 itemNum)
{
	Int16 visibleItems;
	FontID currFont;
	
	
	ErrNonFatalDisplayIf((itemNum < 0) || (itemNum >= listP->numItems), "Invalid item");
	
	currFont = FntSetFont (listP->font);
	
	visibleItems = VisibleItems(listP);
	
	// if the item isn't visible make it the top most item
	if (itemNum < listP->topItem || 
		itemNum >= (listP->topItem + visibleItems))
		{
		LstSetTopItem (listP, itemNum);
		}
	FntSetFont (currFont);
}


/***********************************************************************
 *
 * FUNCTION:    LstGetSelectionText
 *
 * DESCRIPTION:  Returns a pointer to the text of the specifed item
 *               in the list.
 *
 * PARAMETERS:  list    - pointer to list object
 *
 * RETURNED:	 pointer to the text of the current selection
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	9/29/94	Initial Revision
 *			trev	08/11/97	made non modified passed variables constant
 *
 ***********************************************************************/
Char * LstGetSelectionText (const ListType * listP, const Int16 itemNum)
{
//	ErrNonFatalDisplayIf(itemNum >= listP->numItems && itemNum != noListSelection, 
//		"Invalid item");
//	ErrNonFatalDisplayIf(listP->drawItemsCallback != NULL && itemNum != 0, 
//		"Can't get text when draw function set");
	
	
	if ( (itemNum < listP->numItems) && (itemNum != noListSelection) )
		return listP->itemsText[itemNum];

	return (NULL);
}


/***********************************************************************
 *
 * FUNCTION:    LstSetListChoices
 *
 * DESCRIPTION: Set the choices of a list.  The array of text strings
 *              passed to this routine.  This routine does not affect
 *              the display of the list.
 *
 *					 The routine SysFormPointerArrayToStrings is useful if
 *					 what you have is a block of multiple strings.
 *
 * PARAMETERS:	 list       - pointer to list object
 *              itemsText  - pointer to an array of of text string           
 *              numItems   - number of choice in the list
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	9/28/94	Initial Revision
 *
 ***********************************************************************/
void LstSetListChoices (ListType * listP, Char ** itemsText, Int16 numItems)
{
	Int16 newTopItem;
	FontID currFont;
	

	currFont = FntSetFont (listP->font);

	// Set the list to the new items
	listP->itemsText = itemsText;
	listP->numItems = numItems;
	
	
	// Insure that the current selection is within the list's 
	// new number of items.
	if (listP->currentItem != noListSelection &&
		listP->currentItem >= listP->numItems)
		listP->currentItem = listP->numItems - 1;


	// Insure that the list isn't scrolled too far down
	newTopItem = LastItem(listP) - (VisibleItems(listP) - 1);
	if (listP->topItem > newTopItem)
		{
		ErrNonFatalDisplayIf(newTopItem < 0, "Invalid item");
		listP->topItem = newTopItem;
		}
	
	FntSetFont (currFont);
}


/***********************************************************************
 *
 * FUNCTION:    LstSetDrawFunction
 *
 * DESCRIPTION: Set a callback function to call to draw each item instead
 * 				 drawing the item's text string.
 *
 * PARAMETERS:	 list       - pointer to list object
 *              func  		- pointer to function which draws items           
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	9/12/95	Initial Revision
 *			trev	08/11/97	made non modified passed variables constant
 *
 ***********************************************************************/
void LstSetDrawFunction (ListType * listP, ListDrawDataFuncPtr func)
{
	listP->drawItemsCallback = func;
}
	

/***********************************************************************
 *
 * FUNCTION:    LstSetHeight
 *
 * DESCRIPTION: This routine set the number of item that are visible in a
 *              list.  This routine does not redraw the list if it is 
 *              already visible.  It also adjusts topItem to prevent
 *					 a shrunk list from being scrolled down too far.
 *
 * PARAMETERS:	 list          - pointer to list object
 *              visibleItems  - number of choice visible at once
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/7/95	Initial Revision
 *			roger	8/28/95	Add topItem adjustment.
 *			roger	4/11/96	Fix topItem when visibleItems >= numItems.
 *			trev	08/11/97	made non modified passed variables constant
 *
 ***********************************************************************/
void LstSetHeight (ListType * listP, Int16 visibleItems)
{
	FontID currFont;
	Coord x;
	Coord screenHeight;
	Coord height;
	Coord lineHeight;

	currFont = FntSetFont (listP->font);
	lineHeight = FntLineHeight ();
	height = visibleItems * lineHeight;

	// Make sure the bottom of the list is on the display.
	WinGetDisplayExtent (&x, &screenHeight);
	screenHeight -= 2;	// minus 2 for 3d shadow
	
	// Case where the list is longer than the screen and even if
	// the list starts at the top
	if (height >= screenHeight)
		{
		// Size the list to fit as many items on screen as can fit
		visibleItems = screenHeight / lineHeight;
		height = visibleItems * lineHeight;
		
		// Move the list up, centering it vertically.
		listP->bounds.topLeft.y = (screenHeight - height) / 2;
		}
	
	// Case where the list is longer than the screen but moving it
	// higher will allow it all to appear.
	else if (screenHeight - listP->bounds.topLeft.y < height)
		{
		// Move the list up until all the items are visible.
		listP->bounds.topLeft.y = screenHeight - height;
		}
		
	listP->bounds.extent.y = height;
	
	
	// If the list is now longer than numItems make sure all items are visible.
	if (visibleItems >= listP->numItems)
		{
		listP->topItem = 0;
		}
	// Reduce the top item if the list would appear scrolled down too far.
	else if (listP->topItem + visibleItems > listP->numItems)
		listP->topItem = listP->numItems - visibleItems;
		

	FntSetFont (currFont);
}


/***********************************************************************
 *
 * FUNCTION:    LstSetPosition
 *
 * DESCRIPTION: This routine set the position of a list.  The list is
 *              not redrawn, as a matter-a-fact this routine should not
 *              be called when the list is visible.
 *
 * PARAMETERS:	 list   - pointer to list object
 *              x      - left bound
 *              y      - top bound
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/11/95	Initial Revision
 *			trev	08/11/97	made non modified passed variables constant
 *
 ***********************************************************************/
void LstSetPosition (ListType * listP, const Coord x, const Coord y)
{
	listP->bounds.topLeft.x = x;
	listP->bounds.topLeft.y = y;
}
	

/***********************************************************************
 *
 * FUNCTION:    LstHandleEvent
 *
 * DESCRIPTION: MemHandle event in the specified list.  This routine 
 *              handles two type of events, penDownEvents and 
 *              lstEnterEvent.
 *
 *              When this routine receives a penDownEvents it checks
 *              if the pen position is within the bounds of the
 *              list object, if it is this routine tracks the pen 
 *              until the pen comes up.  If the pen comes up
 *              within the bounds of the list, a lstEnterEvent is 
 *              added to the event queue, and the routine is exited.
 *
 *              When this routine receives a lstEnterEvent it 
 *              check that the list id in the event record match
 *              the id of the list specified, if there is a match
 *              this routne will create and display a popup window
 *              containing the list's choices, and the routine is exited. 
 *
 *              If a penDownEvents is received while the list's popup 
 *              window is display, and the the pen postion is outside the 
 *              bounds of the popup window, the  will be dismissed.
 *              If the pen postion is within the bounds of the window 
 *              the this routne will track the pen until it comes up. 
 *              If the pen comes up in the list a lstExitEvent will be
 *              added to the event queue.
 *
 *
 * PARAMETERS:	 list      - pointer to list object
 *              pEvent    - pointer to an EventType structure.
 *              pError    - pointer to returned erro code
 *
 * RETURNED:	 TRUE if the event was MemHandle or FALSE if it was not.
 *              pError     - error code.
 *
 * HISTORY:
 *		09/29/94	rsf	Created by Roger Flores
 *		08/11/97	trev	made non modified passed variables constant
 *		08/23/00	kwk	Fixed bug where drag selection code could wind up 
 *							with two hilighted items in the list, due to calling
 *							LstScrollList with listP->currentItem != noSelection.
 *
 ***********************************************************************/
Boolean LstHandleEvent (ListType* listP, const EventType* event)
{
	EventType newEvent;
	Boolean   inverted, currentItemInverted;
	Boolean   penDown;
	Int16     newSelection;        // item under the pen
	Int16     newerSelection;      // newer item under pen, may not equal above
	Int16		 savedSelection;		// currentItem on entry to drag-selection loop.
	Int16		 x, y;
	Int16		lastItemBottomY;
	UInt16 	ticks;
	UInt32	delay;
	UInt32  	firstTick;
	Boolean  scrolled;

	if (!listP->attr.usable ||
		 !listP->attr.visible)
		return (false);

	newSelection = listP->currentItem - listP->topItem;
		// use the current item. This suppresses a flash
		// when the newer selection == the new selection.


	switch (event->eType)
		{
		case penDownEvent:
			if (RctPtInRectangle (event->screenX, event->screenY, &(listP->bounds)))
				{
				newEvent = *event;
				newEvent.eType = lstEnterEvent;
				newEvent.data.lstEnter.listID = listP->id;
				newEvent.data.lstEnter.pList = listP;
				newEvent.screenX = event->screenX;
				newEvent.screenY = event->screenY;
				EvtAddEventToQueue (&newEvent);
				return true;
				}
			break;

		case lstEnterEvent:
			{
			if (PrvHandleScrollingIndicators (listP, event))
				return true;
				
			// The loop below handles list item selection and dragging, as
			// well as drag scrolling.
			WinPushDrawState();
			FntSetFont (listP->font);
			currentItemInverted = true;
			inverted = false;
			
			// Set up so that when LstScrollList is called during drag
			// selection, it doesn't draw the current item as hilighted,
			// since the loop has its own idea of the current item
			// (newSelection).
			savedSelection = listP->currentItem;
			listP->currentItem = noListSelection;
			
			// Calculate lastItemBottomY to quickly tell if a point is 
			// past the last visible item but within the list's bounds
			lastItemBottomY = listP->bounds.topLeft.y + 
				(VisibleItems(listP) * FntLineHeight());
	
			do
				{
				EvtGetPen (&x, &y, &penDown);
				if (RctPtInRectangle (x, y, &(listP->bounds)) &&
					y < lastItemBottomY)
					{
					// Process the new pen point.
					newerSelection = (y - listP->bounds.topLeft.y) / FntLineHeight();
					// Handle selections past end of small list
					if (newerSelection > LastVisibleItem(listP))
						{
						newerSelection = noListSelection;
						}
					
					// DOLATER - because this is a modal loop, a list with a
					// scrollbar can't keep it dynamically in sync with the
					// scrolled state of the list during drag selection. The
					// routine would have to enqueue a new event and exit, so
					// that the app code gets a chance to update the scrollbar,
					// and then passes that event through to the list code.
					if (newerSelection != newSelection)
						{
						if (inverted)
							{
							PrvDrawItem (listP, listP->topItem + newSelection, false);
							}
						
						if (currentItemInverted)
							{
							PrvDrawItem(listP, savedSelection, false);
							currentItemInverted = false;
							}
						
						// DOLATER kwk - figure out if this is the correct fix for the
						// list code handling drag selection off end of the list. Previously
						// the code was calling PrvDrawItem w/topItem + -1 for this case.
						if (newerSelection != noListSelection)
							{
							PrvDrawItem (listP, listP->topItem + newerSelection, true);
							inverted = true;
							}
						
						newSelection = newerSelection;
						}
	
					if (penDown)
						{
						// Reset drag scrolling values
						delay = listScrollInitDelay;
						firstTick = TimGetTicks();
						}
					else /* pen is up */
						{
						listP->currentItem = listP->topItem + newerSelection;
						ErrNonFatalDisplayIf(listP->currentItem < 0, "Invalid current item");
						
						// The visible item inverted is hereby considered the current item
						currentItemInverted = true;
						newEvent.eType = lstSelectEvent;
						newEvent.data.lstSelect.selection = listP->currentItem;
						
						SndPlaySystemSound (sndClick);
						}
					}
				else /* moved out of rectangle */
					{
	
	
					// drag scrolling code
					ticks = TimGetTicks () - firstTick;
					scrolled = false;
					if (PtBelowRect (x, y, &listP->bounds) ||
						 PtAboveRect (x, y, &listP->bounds))
						{
						if ((ticks > listScrollMinDelay) && (ticks > delay))
							{
	
							if (inverted)
								{
								PrvDrawItem (listP, listP->topItem + newSelection, false);
								inverted = false;
								}
							// MemHandle above the list
							if (y < listP->bounds.topLeft.y)
								{
								if (LstScrollList (listP, winUp, 1))
									{
									newSelection = 0;
									PrvDrawItem (listP, listP->topItem + newSelection, true);
									inverted = true;
									delay = max (0, ((Int32)delay - listScrollAcceleration));
									firstTick = TimGetTicks();
									scrolled = true;
									}
								}
							else
								{
								if (LstScrollList (listP, winDown, 1))
									{
									newSelection = VisibleItems(listP) - 1;
									PrvDrawItem (listP, listP->topItem + newSelection, true);
									inverted = true;
									delay = max (0, ((Int32)delay - listScrollAcceleration));
									firstTick = TimGetTicks();
									scrolled = true;
									}
								}
							}
						else
							{
							// Handle above the list
							if (y < listP->bounds.topLeft.y)
								{
								if (!ListScrolledToTop(listP))
									{
									scrolled = true;
									if (newSelection != 0)
										{
										if (inverted)
											PrvDrawItem (listP, listP->topItem + newSelection, false);
										newSelection = 0;
										PrvDrawItem (listP, listP->topItem + newSelection, true);
										inverted = true;
										}
									}
								}
							else
								{
								// Are we at the botton of the list?
								if (!ListScrolledToBottom(listP))
									{
									scrolled = true;
									if (newSelection != VisibleItems(listP) - 1)
										{
										if (inverted)
											PrvDrawItem (listP, listP->topItem + newSelection, false);
										newSelection = VisibleItems(listP) - 1;
										PrvDrawItem (listP, listP->topItem + newSelection, true);
										inverted = true;
										}
									}
								}
							}
						}
	
	
					if (!scrolled)
						{
						if (inverted)
							{
							PrvDrawItem (listP, listP->topItem + newSelection, false);
							inverted = false;
							}
					   newSelection = noListSelection;
						}
	
					if (!penDown)
						{
						// The user did not select the control.
						newEvent.eType = lstExitEvent;
	
						listP->currentItem = savedSelection;
						
						// If the user pen ups above or below the list we must remove
						// the inverted visible item.  If the pen is to the side the
						// visible item is already inverted.
						if (inverted)
							{
							PrvDrawItem (listP, listP->topItem + newSelection, false);
							inverted = false;
							}
						}
					}
				} while (penDown);
	
	
			if (!currentItemInverted)
				{
				PrvDrawItem(listP, listP->currentItem, true);
				currentItemInverted = true;
				}
	
			// Return lstSelectEvent or lstExitEvent
			newEvent.penDown = penDown;
			newEvent.screenX = x;
			newEvent.screenY = y;
			newEvent.data.lstSelect.listID = listP->id;
			newEvent.data.lstSelect.pList = listP;
			EvtAddEventToQueue (&newEvent);
			WinPopDrawState();
			return true;
			}

		default:
			break;
		}
		
	return(false);
}


/***********************************************************************
 *
 * FUNCTION:    LstPopupList
 *
 * DESCRIPTION: Display a window the contains the choices of the list.
 *              
 *
 * PARAMETERS:	 list    - pointer to list object
 *
 * RETURNED:	 the list item selected or -1 if no item has selected.
 *
 * REVISION HISTORY:
 *		Date		Name	Description
 *		----		----	-----------
 *		11/14/94	art	Initial Revision
 *		08/11/97	trev	made non modified passed variables constant
 *		09/07/98	kwk	Use new EvtPeekEvent & SysWantEvent to MemHandle
 *							appropriate bailout from list.
 *		09/17/98	kwk	Backed out peek logic for Sumo, and modified
 *							to temporarily work around peek logic flaw.
 *		10/28/98	JAQ	restores state so as not to crash future FrmRedrawForm calls
 *		11/19/98	roger	Fixed case when a form appears above the list window.
 *		9/30/99	roger	Find, since it displays a form, cancels the popup list and then appears.
 *		11/06/99	jmp	Menu, Command, Launch, Keyboards, Calc, TSM also cancel popup lists,
 *							and then do something else.  Prevent double beeping!
 *		11/23/99	jmp	Added the contrast and brightness (actually, hard power) to the list of
 *							buttons that cancel popup lists and and then exit immediately.
 *		12/07/99	grant	Do not save/restore active and draw windows.
 *		12/08/99	kwk	Added the generic keyboard virtual character (vchrKeyboard)
 *							to the list of keydown events that cancel the popup list.
 *							This is what gets posted on a Jap. device when you tap the
 *							keyboard silkscreen icon (same location as Calc btn on English devices).
 *		12/12/99			Hmmm... maybe it's time to rethink this.  Needed to add all of the hard
 *							buttons (1..4) and the ronamatic stroke to the list of "buttons" that
 *							cancel pop-up lists and then exit immediately.
 *		02/28/00			For the short term, fixed yet another case we didn't handle:  the vchrAlarm!
 *							This fixes bug #25393.
 *		08/31/00	peter	Yet another case added to fix bug #41255 and others, such as a procedure
 *							alarm triggered call to AttnGetAttention().
 *
 ***********************************************************************/
Int16 LstPopupList (ListType * listP)
{
	Char				lookupBuffer [maxListLookupLen+1];
	UInt16			error;
	Int16				topItem;
	Int16          selectedItem;
	Int16				displayWidth;
	Int16				displayHeight;
	Boolean        handled;
	Boolean			insPtState;
	ListType			list;
	WinHandle   	popupWin;
	WinHandle   	savedBits;
	FormType *		currentFormP;
	RectangleType	r, frame;
	FontID 			currFont;
	ListAttrType	oldAttr = listP->attr;	// save list state stuff
	
	currFont = FntSetFont (listP->font);
	
	insPtState = InsPtEnabled ();
	InsPtEnable (false);

	MemMove (&list, listP, sizeof (list));

	// Determine the size and position of the popup window.  Try to position
	//	the popup at the top left bounds of the list object.
	RctCopyRectangle (&list.bounds, &r);
	WinWindowToDisplayPt (&r.topLeft.x, &r.topLeft.y);

	// Force the bottom of the list onto the display.
	WinGetDisplayExtent (&displayWidth, &displayHeight);
	if (r.topLeft.y + r.extent.y > displayHeight - 2)
		r.topLeft.y = displayHeight - r.extent.y - 2;

	// Force the top of the list onto the display.
	if (r.topLeft.y < 1)
		r.topLeft.y = 1;

	// Force the right side of the list onto the display.
	if (r.topLeft.x + r.extent.x > displayWidth - 2)
		r.topLeft.x = max (2, (displayHeight - r.extent.x - 2));
	
	// Force the left side of the list onto the display.
	if (r.topLeft.x < 1)
		r.topLeft.x = 1;

	// Since we are going to display only a window, save the active form
	// and set it to NULL.  That way if any forms appear over this window, 
	// they won't access the form underneath instead of our window.
	currentFormP = FrmGetActiveForm ();
	FrmSetActiveForm(NULL);

	// Create a window.
	popupWin = WinCreateWindow (&r, popupFrame, true, false, &error);

	// Save the bits behind the popup window.
	WinSetDrawWindow (WinGetDisplayWindow());
	WinGetWindowFrameRect (popupWin, &frame);
	RctInsetRectangle (&frame, -1);
	WinClipRectangle (&frame);
	savedBits = WinSaveBits (&frame, &error);

	// Draw the window's frame.
	WinEnableWindow (popupWin);
	WinSetDrawWindow (popupWin);
	WinSetActiveWindow (popupWin);

	// Draw the choices of the list, make sure the current selection
	// is in view.
	listP->bounds.topLeft.x = 0;
	listP->bounds.topLeft.y = 0;
	listP->attr.usable = true;
	listP->attr.poppedUp = true;
	listP->popupWin = popupWin;

	LstDrawList (listP);

	// Initialize the incremental search buffer.
	*lookupBuffer = 0;

	while (true)
		{
		EventType event;
		EvtGetEvent(&event, evtWaitForever);
		
		// Process some keydown events before the system does.
		if ((event.eType == keyDownEvent) &&
			EvtKeydownIsVirtual(&event))
			{
			// These chars cancel the list and are interpreted later.
			if (event.data.keyDown.chr == vchrMenu 					||
				 event.data.keyDown.chr == vchrCommand				||
				 event.data.keyDown.chr == vchrLaunch				||
				 event.data.keyDown.chr == vchrFind 				||
				 event.data.keyDown.chr == vchrCalc					||
				 event.data.keyDown.chr == vchrKeyboardAlpha		||
				 event.data.keyDown.chr == vchrKeyboardNumeric		||
				 event.data.keyDown.chr == vchrKeyboard				||
				 event.data.keyDown.chr == vchrLowBattery			||
				 event.data.keyDown.chr == vchrIrGotData			||
				 event.data.keyDown.chr == vchrTsm1					||
				 event.data.keyDown.chr == vchrTsm2					||
				 event.data.keyDown.chr == vchrTsm3					||
				 event.data.keyDown.chr == vchrTsm4					||
				 event.data.keyDown.chr == vchrAutoOff				||
				 event.data.keyDown.chr == vchrHardPower			||
				 event.data.keyDown.chr == vchrHardContrast			||
				 event.data.keyDown.chr == vchrHard1				||
				 event.data.keyDown.chr == vchrHard2				||
				 event.data.keyDown.chr == vchrHard3				||
				 event.data.keyDown.chr == vchrHard4				||
				 event.data.keyDown.chr == vchrRonamatic			||
				 event.data.keyDown.chr == vchrAlarm				||
				 event.data.keyDown.chr == vchrCardCloseMenu		||
				 AttnEffectOfEvent(&event) != attnNoDialog)
				{
				EvtAddEventToQueue (&event);
				selectedItem = -1;
				break;
				}
			}
		
		handled = SysHandleEvent ((EventPtr)&event);
		if (! handled)
			handled = LstHandleEvent (listP, &event);

		if (event.eType == penDownEvent)
			{
			if (!handled)
				{
				selectedItem = -1;
				break;
				}
			}

		else if (event.eType == lstSelectEvent)
			{
			selectedItem = event.data.lstSelect.selection;
			break;
			}

		else if (event.eType == lstExitEvent)
			{
			selectedItem = -1;
			break;
			}

		// Handle keydown events.
		else if (event.eType == keyDownEvent)
			{
			// Handle command keys - by this point we know that it's either got
			// to be a page up or a page down (others are filtered out above).
			if (EvtKeydownIsVirtual(&event))
				{
				// Process these chars
				if (event.data.keyDown.chr == vchrPageUp)
					{
						LstScrollList(listP, winUp, VisibleItems(listP) - 1);
					}
				else if (event.data.keyDown.chr == vchrPageDown)
					{
						LstScrollList(listP, winDown, VisibleItems(listP) - 1);
					}
				else  // if (event.data.keyDown.chr != vchrLowBattery)
					{
					EvtAddEventToQueue (&event);
					selectedItem = -1;
					break;
					}
				}

			else if (IncrementalSearch (listP, lookupBuffer, event.data.keyDown.chr))
				{
				selectedItem = listP->currentItem;
				break;
				}
			}
		
		// DOLATER kwk - this logic should get removed when event peeking is fixed up.
		else if (event.eType == appStopEvent)
			{
			EvtAddEventToQueue (&event);
			selectedItem = -1;
			break;
			}
		}

	// Dismiss the popup window.
	WinSetDrawWindow (WinGetDisplayWindow ());
	WinGetWindowFrameRect (popupWin, &r);
	RctInsetRectangle (&r, -1);
	WinClipRectangle (&r);
	WinRestoreBits (savedBits , r.topLeft.x, r.topLeft.y);

	WinDeleteWindow (popupWin, false);
	
	// DOLATER kwk - Roger put in this fix, but the real problem is with the
	// form code not restoring the proper window when a form is dismissed.
	FrmSetActiveForm(currentFormP);
	
	topItem = listP->topItem;
	MemMove (listP, &list, sizeof (list));
	if (selectedItem != -1)
		{
		listP->currentItem = selectedItem;
		listP->topItem = topItem;
		}

	InsPtEnable (insPtState);
	
	FntSetFont (currFont);
	listP->attr = oldAttr;
	
	return (selectedItem);
}


/***********************************************************************
 *
 * FUNCTION:    LstNewList
 *
 * DESCRIPTION: Create a list. 
 *
 * PARAMETERS:  formPP - pointer to a pointer to a form. Set if the form moves.
 *					 id - id of the new list
 *					 x - x of the new list
 *					 y - y of the new list
 *					 width - width of the new list
 *					 height - height of the new list
 *					 font - font of the new list
 *					 visibleItems - the number of items the list displays at once
 *					 triggerId - if this list is a popup list, this is the id of the trigger to use
 *									 Pass zero for a normal list.
 *
 * RETURNED:    0 if no error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/19/97	Initial Revision
 *
 ***********************************************************************/
Err LstNewList (void **formPP, UInt16 id, 
	Coord x, Coord y, Coord width, Coord /* height */, 
	FontID font, Int16 visibleItems, Int16 triggerId)
{
	ListType * listP;
	FormPopupType *popupP;
	Err error;
	FontID currFont;
	
	
	error = FrmAddSpaceForObject ((FormType * *) formPP, (MemPtr *)&listP, frmListObj, 
				sizeof(ListType));
	if (error)
		return error;
	
	listP->id = id;
	listP->bounds.topLeft.x = x;
	listP->bounds.topLeft.y = y;
	listP->bounds.extent.x = width;
	currFont = FntSetFont (font);
	listP->bounds.extent.y = FntLineHeight() * visibleItems;
	FntSetFont (currFont);
	
	listP->font = font;
	listP->numItems = 0;
	
	if (triggerId)
		{
		error = FrmAddSpaceForObject ((FormType * *) formPP, (MemPtr *)&popupP, frmPopupObj, 
					sizeof(FormPopupType));
		if (error)
			return error;
		
		popupP->controlID = triggerId;
		popupP->listID = id;
		}
	else
		listP->attr.usable = true;
	
	
	return 0;
}



#pragma mark ---------------------


/***********************************************************************
 *
 * FUNCTION:    PrvSetColors
 *
 * DESCRIPTION: Set colors for drawing a list item.  Note:  This routine
 *					 assumes the caller will be preserving the drawing state.
 *
 * PARAMETERS:	 selected - whether item is selected or not
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	2/10/99	Initial Revision
 *
 ***********************************************************************/
static void PrvSetColors(Boolean selected)
{
	if (selected) {
		WinSetBackColor(UIColorGetIndex(UIObjectSelectedFill));
		WinSetTextColor(UIColorGetIndex(UIObjectSelectedForeground));
		WinSetForeColor(UIColorGetIndex(UIObjectSelectedForeground));
	}
	else {
		WinSetBackColor(UIColorGetIndex(UIObjectFill));
		WinSetTextColor(UIColorGetIndex(UIObjectForeground));
		WinSetForeColor(UIColorGetIndex(UIObjectForeground));
	}
}	


/***********************************************************************
 *
 * FUNCTION:    PrvDrawItem
 *
 * DESCRIPTION: Draw a list item.  Note:  The routine assumes the drawing
 *					 state will be preserved by the caller.
 *
 * PARAMETERS:	 listP       - pointer to list object
 *					 itemNum		 - index of item to draw (in full list)
 *					 selected	 - true to draw item as selected
 *
 * RETURNED:	 nothing
 *
 * HISTORY:
 *		09/25/95	Roger	Created by Roger Flores.
 *		08/01/00	kwk	Don't draw scroll up/down indicators if list uses scrollbar.
 *		08/09/00	aro	Draw rectangle width reduced on the line with the scroll indicator
 *		10/27/00	kwk	When drawing the scroll indicator, added missing call
 *							to WinClipRectangle; without this, the indicator gets
 *							drawn over an obscuring window (bug 43665).
 *
 ***********************************************************************/
static void PrvDrawItem (ListType* listP, Int16 itemNum, Boolean selected)
{
	RectangleType r;
	Char	indicator = 0;

	// bail out if item is not visible
	if ( itemNum == noListSelection || ! ItemIsVisible(listP, itemNum) )
		return;

	PrvSetColors(selected);

	RctCopyRectangle(&listP->bounds, &r);
	r.extent.y = FntLineHeight ();
	r.topLeft.y += (itemNum - listP->topItem) * r.extent.y;

	WinEraseRectangle(&r, 0);

	// The text should be indented a bit
	r.topLeft.x += textInsetWidth;
	r.extent.x -= textInsetWidth;

	// Calculate space for indicator
	if ((itemNum == listP->topItem) && !ListScrolledToTop(listP) && !listP->attr.hasScrollBar) 
	{
		indicator = scrollUpIndicator;
		// Reduce the rectangle width by the indicator width - 1 to allow
		// text to touch the indicator. There is actually 1 extra pixel
		// in the character pixel so we remove the one the left of the indicator
		r.extent.x -= scrollIndicatorWidth - 1;
	}
	
	if ((itemNum == LastVisibleItem(listP)) && !ListScrolledToBottom(listP) && !listP->attr.hasScrollBar) 
	{
		indicator = scrollDownIndicator;
		r.extent.x -= scrollIndicatorWidth - 1;
	}

	if (listP->drawItemsCallback)
		listP->drawItemsCallback(itemNum, &r, listP->itemsText);
	else 
	{
		Char * text = listP->itemsText[itemNum];
		WinDrawTruncChars (text, StrLen(text), r.topLeft.x, r.topLeft.y, r.extent.x);
	}
	
	// draw scroll up indicator if necessary
	if (indicator)
	{
		Coord xPos, yPos;
		FontID currFont = FntSetFont (symbolFont);
		RectangleType	savedClipR;
		WinGetClip(&savedClipR);
		
		r.topLeft.x += r.extent.x - 1;
		r.extent.x = scrollIndicatorWidth;
		
		// Save draw location, since WinClipRectangle can alter <r>.
		xPos = r.topLeft.x - 1;
		yPos = r.topLeft.y;
		WinClipRectangle(&r);
		WinSetClip(&r);
		
		WinDrawChar(indicator, xPos, yPos);
		WinSetClip(&savedClipR);
		FntSetFont (currFont);
	}
}


/***********************************************************************
 *
 * FUNCTION:    PrvHandleScrollingIndicators
 *
 * DESCRIPTION: If the pen is down within a visible scroll indicator
 *
 * PARAMETERS:	 list   - pointer to list object
 *              x      - left bound
 *              y      - top bound
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	7/31/95	Initial Revision
 *			bob	2/10/99	rewrote to deal with different colors
 *								Also a minor change from non-color: previously this
 *								returned true if the tap was where a scrolling indicator
 *								would be, even when there was no indicator.  By returning
 *								false we allow the list to actually select the item.
 *
 ***********************************************************************/
static Boolean PrvHandleScrollingIndicators (ListType * listP, const EventType * eventP)
{
	RectangleType	r;
	Boolean			penDown = eventP->penDown;
	Boolean			rowSelected;
	Coord		 		x = eventP->screenX;
	Coord		 		y = eventP->screenY;
	FontID			currFont;
	Char				which;
	RectangleType	savedClipR;
	

	if (listP->attr.hasScrollBar)
		return (false);

	currFont = FntSetFont (listP->font);

	// get rectangle for list item
	RctCopyRectangle(&listP->bounds, &r);
	
	// check for scroll up
	r.topLeft.x += r.extent.x - scrollIndicatorWidth;
	r.extent.x = scrollIndicatorWidth;
	r.extent.y = FntLineHeight() - 1;
	if (!ListScrolledToTop(listP) && RctPtInRectangle(x, y, &r))
	{
		which = scrollUpIndicator;
		rowSelected = (listP->topItem == listP->currentItem);
	}

	// check for scroll down
	else {
		r.topLeft.y += (VisibleItems(listP)-1) * FntLineHeight() + 1;
		if (!ListScrolledToBottom(listP) && RctPtInRectangle(x, y, &r))
		{
			which = scrollDownIndicator;
			rowSelected = (LastVisibleItem(listP) == listP->currentItem);
		}

		// neither up nor down, return not handled
		else {
			FntSetFont (currFont);
			return false;
		}
	}

	// if we get this far, we're in an indicator, so track the pen
	WinPushDrawState();
	PrvSetColors(rowSelected);
	FntSetFont (symbolFont);
	WinGetClip(&savedClipR);
	WinSetClip(&r);
	r.topLeft.x -= 1;
	while (penDown) {
		if (RctPtInRectangle (x, y, &r))
			if (rowSelected) {
				// odd case, row is already drawn in object selected colors
				// so just invert them to indicate double-selection
				WinDrawInvertedChars(&which, 1, r.topLeft.x, r.topLeft.y);
			}
			else {
				PrvSetColors(true);
				WinDrawChar(which, r.topLeft.x, r.topLeft.y);
			}
		
		// pen outside of rectangle, draw character in 'normal' colors for row
		else {
			PrvSetColors(rowSelected);
			WinDrawChar(which, r.topLeft.x, r.topLeft.y);
		}
		PenGetPoint (&x, &y, &penDown);
	}
	
	WinPopDrawState();
	WinSetClip(&savedClipR);

	// don't need to clean up drawing -- if control was inverted, scrolling will
	// redraw the indicator, and otherwise it's drawn normally
	
	FntSetFont (listP->font);
	if (RctPtInRectangle (x, y, &r))
		LstScrollList(listP, (which==scrollUpIndicator)?winUp:winDown, VisibleItems(listP) - 1);

	FntSetFont (currFont);
	return true;
}










#if unimplemented

/***********************************************************************
 *
 * FUNCTION:    LstPopupList
 *
 * DESCRIPTION: Display a window that contains the choices of the list.
 *              
 *
 * PARAMETERS:	 listP    - pointer to list object
 *
 * RETURNED:	 the list item selected or -1 if no item has selected.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/14/94		Initial Revision
 *			JAQ	10/28/98		restores state so as not to crash future FrmRedrawForm calls
 *
 ***********************************************************************/
Int16 LstPopupListWithScrollBar (ListType * listP)
	{
	UInt16				error;
	UInt16				topItem;
	UInt16				visibleItems;
	Int16				itemsToScroll;
	Int16          selectedItem;
	Int16				displayWidth;
	Int16				displayHeight;
	Boolean        handled;
	Boolean			insPtState;
	ListType			list;
	EventType      event;
	WinHandle   	popupWin;
	WinHandle   	savedBits;
	WinHandle   	savedActiveWin;
	WinHandle   	savedDrawWin;
	RectangleType	r, frame;
	ScrollBarType	bar;
	FontID 			currFont;
	
	//JAQ- saved list state stuff
	ListAttrType	oldAttr = listP->attr;
	

	// Save the current active window and the current draw window.
	savedActiveWin = WinGetActiveWindow ();
	savedDrawWin = WinGetDrawWindow ();
	
	insPtState = InsPtEnabled ();
	InsPtEnable (false);

	MemMove (&list, listP, sizeof (list));

	// Determine the size and position of the popup window.  Try to position
	//	the popup at the top left bounds of the list object.
	RctCopyRectangle (&list.bounds, &r);
	WinWindowToDisplayPt (&r.topLeft.x, &r.topLeft.y);

	// Force the bottom of the list onto the display.
	WinGetDisplayExtent (&displayWidth, &displayHeight);
	if (r.topLeft.y + r.extent.y > displayHeight - 2)
		r.topLeft.y = displayHeight - r.extent.y - 2;

	// Force the top of the list onto the display.
	if (r.topLeft.y < 1)
		r.topLeft.y = 1;

	// Force the right side of the list onto the display.
	if (r.topLeft.x + r.extent.x > displayWidth - 2)
		r.topLeft.x = max (2, (displayHeight - r.extent.x - 2));
	
	// Force the left side of the list onto the display.
	if (r.topLeft.x < 1)
		r.topLeft.x = 1;


	// Create a window.
	popupWin = WinCreateWindow (&r, popupFrame, true, false, &error);

	// Save the bits behind the popup window.
	WinSetDrawWindow (WinGetDisplayWindow());
	WinGetWindowFrameRect (popupWin, &frame);
	RctInsetRectangle (&frame, -1);
	WinClipRectangle (&frame);
	savedBits = WinSaveBits (&frame, &error);


	// Draw the windows frame.
	WinEnableWindow (popupWin);
	WinSetDrawWindow (popupWin);
	WinSetActiveWindow (popupWin);


	currFont = FntSetFont (listP->font);
	// Draw the choices of the list, make sure the current selection
	// is in view.
	listP->bounds.topLeft.x = 0;
	listP->bounds.topLeft.y = 0;
	listP->attr.usable = true;
	listP->attr.poppedUp = true;
	listP->attr.hasScrollBar = true;
	listP->popupWin = popupWin;

	// Initialize the scroll bar.
	MemSet (&bar, sizeof (ScrollBarType), 0);
	visibleItems = VisibleItems (listP);

	if (visibleItems > 1)
		{
		RctCopyRectangle (&listP->bounds, &bar.bounds);
		bar.bounds.topLeft.x += bar.bounds.extent.x - lstScrollBarWidth;
		bar.bounds.extent.x = lstScrollBarWidth;
		
		bar.attr.usable = true;
		
		SclSetScrollBar (&bar, listP->currentItem, 
				0, listP->numItems - visibleItems, visibleItems - 1);	
		}


	WinDrawWindowFrame ();
	LstDrawList (listP);
	SclDrawScrollBar (&bar);


	while (true)
		{
		EvtGetEvent (&event, evtWaitForever);
		SysHandleEvent (&event);

		if ( ! (handled = SclHandleEvent (&bar, &event)))
			handled = LstHandleEvent (listP, &event);

		if (event.eType == penDownEvent)
			{
			if (!handled)
				{
				selectedItem = -1;
				break;
				}
			}

		else if (event.eType == lstSelectEvent)
			{
			selectedItem = event.data.lstSelect.selection;
			break;
			}

		else if (event.eType == lstExitEvent)
			{
			selectedItem = -1;
			break;
			}

		// MemHandle the up and down physical buttons.
		else if (event.eType == keyDownEvent)
			{
			if (EvtKeydownIsVirtual(&event))
				{
				if (event.data.keyDown.chr == vchrPageUp)
					{
					LstScrollList(listP, winUp, VisibleItems(listP) - 1);
					SclSetScrollBar (&bar, listP->currentItem, 
						0, listP->numItems - visibleItems, visibleItems - 1);	

					continue;
					}
				else if (event.data.keyDown.chr == vchrPageDown)
					{
					LstScrollList(listP, winDown, VisibleItems(listP) - 1);

					SclSetScrollBar (&bar, listP->currentItem, 
						0, listP->numItems - visibleItems, visibleItems - 1);	
					continue;
					}
				}
			}
			
		else if (event.eType == sclRepeatEvent)
			{
			itemsToScroll = event.data.sclRepeat.newValue - 
				event.data.sclRepeat.value;
			if (itemsToScroll < 0)
				LstScrollList (listP, winUp, -itemsToScroll);
			else
				LstScrollList (listP, winDown, itemsToScroll);
			}


		// Leave the list if the application is being left.
		else if (event.eType == appStopEvent)
			{
			EvtAddEventToQueue (&event);
			selectedItem = -1;
			break;
			}
		}


	// Dismiss the popup window
	WinSetDrawWindow (WinGetDisplayWindow ());
	WinGetWindowFrameRect (popupWin, &r);
	RctInsetRectangle (&r, -1);
	WinClipRectangle (&r);
	WinRestoreBits (savedBits , r.topLeft.x, r.topLeft.y);

	WinDeleteWindow (popupWin, false);
	WinSetDrawWindow (savedDrawWin);
	WinSetActiveWindow (savedActiveWin);
	
	topItem = listP->topItem;
	MemMove (listP, &list, sizeof (list));
	if (selectedItem != -1)
		{
		listP->currentItem = selectedItem;
		listP->topItem = topItem;
		}

	InsPtEnable (insPtState);

	FntSetFont (currFont);
	listP->attr = oldAttr;
	
	return (selectedItem);
}

#endif
