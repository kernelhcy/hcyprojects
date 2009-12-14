/******************************************************************************
 *
 * Copyright (c) 1996-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: ScrollBar.c
 *
 * Release: 
 *
 * Description:
 *	  This module contains routines to support scroll bars. 
 *
 * History:
 *		Feb 13, 1996	Created by Art Lamb
 *      mm/dd/yy   initials - brief revision comment
 *
 *****************************************************************************/

#define NON_PORTABLE

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_CLIPBOARDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_FIELDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS
#define ALLOW_ACCESS_TO_INTERNALS_OF_SCROLLBARS

#include <PalmTypes.h>
#include <SystemPublic.h>

#include "ScrollBar.h"
#include "UIGlobals.h"

#define slop					10
#define minDelay 				10
#define minCarLength			10
#define scrollBarPattern	0x55AA		// pattern of elevator shaft

/***********************************************************************
 *
 * FUNCTION:     GetArrowLen
 *
 * DESCRIPTION:  This routine returns the lenght of the arrow heads at 
 *               the ends of the scroll bar.
 *
 * PARAMETERS:   bar - pointer to a scroll bar structure.
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/17/96	Initial Revision
 *
 ***********************************************************************/
static Int16 GetArrowLen (ScrollBarPtr bar)
{
	Int16 girth;
	
	if (bar->bounds.extent.x > bar->bounds.extent.y)
		girth = bar->bounds.extent.y;
	else
		girth = bar->bounds.extent.x;

	return ((girth / 2) + 3);
}


/***********************************************************************
 *
 * FUNCTION:     GetDragLen
 *
 * DESCRIPTION:  This routine returns the length of the drag area of 
 *               the scroll bar,  which is the length of the elevator
 *               shaft minus the length of the elevator car.
 *
 * PARAMETERS:   bar - pointer to a scroll bar structure.
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/17/96	Initial Revision
 *
 ***********************************************************************/
static Int16 GetDragLen (ScrollBarPtr bar)
{
	Int16 length;
	Int16 carLength;
	
	// Get the length of the scroll bar.
	if (bar->bounds.extent.x > bar->bounds.extent.y)
		length = bar->bounds.extent.x;
	else
		length = bar->bounds.extent.y;

	// Subtract the length of the two arrow tips.
	length -= GetArrowLen (bar) << 1;

	// Subtract the length of the elevator car.
	carLength = (Int16) (((Int32)(bar->pageSize) * length) / 
		  	(bar->maxValue - bar->minValue + bar->pageSize));

	if (carLength < minCarLength)
		carLength = minCarLength;
		
	length -= carLength;
		  	
	return (length);
}


/***********************************************************************
 *
 * FUNCTION:     GetCarLen
 *
 * DESCRIPTION:  This routine returns the length, in pixels, of the 
 *               elevator car.
 *
 * PARAMETERS:   bar - pointer to a scroll bar structure.
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/17/96	Initial Revision
 *
 ***********************************************************************/
static Int16 GetCarLen (ScrollBarPtr bar)
{
	Int16 length;
	Int16 carLength;
	
	// Get the length of the scroll bar.
	if (bar->bounds.extent.x > bar->bounds.extent.y)
		length = bar->bounds.extent.x;
	else
		length = bar->bounds.extent.y;

	// Subtract the length of the two arrow tips.
	length -= GetArrowLen (bar) << 1;

	if (length < 0)
		return (0);

	carLength = (Int16) (((Int32)(bar->pageSize) * length) / 
		  	(bar->maxValue - bar->minValue + bar->pageSize));

	if (carLength < minCarLength)
		carLength = minCarLength;
		
	return (carLength);
}


/***********************************************************************
 *
 * FUNCTION:     GetCarPos
 *
 * DESCRIPTION:  This routine returns the position of the elevator car.
 *               The value returned is the offset, in pixel, from the
 *               top the the elevator shaft.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/17/96	Initial Revision
 *
 ***********************************************************************/
static Int16 GetCarPos (ScrollBarPtr bar)
{
	if (bar->minValue == bar->maxValue)
		return 0;
		
	return (Int16) (((Int32)(bar->value - bar->minValue) * GetDragLen(bar) ) / 
		(bar->maxValue - bar->minValue));
}


/***********************************************************************
 *
 * FUNCTION:     DragCar
 *
 * DESCRIPTION:  This routine 
 *
 * PARAMETERS:   bar - pointer to a scroll bar structure.
 *               x   - window relative coordinate
 *               y   - window relative coordinate
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/13/96	Initial Revision
 *
 ***********************************************************************/
static Int16 DragCar (ScrollBarPtr bar, Int16 x,Int16 y, Boolean firstTime)
{   
	Int16 newPos;
	Int16 carPos;

	// Compute the offset from the top the the elevator shaft.
	if (bar->bounds.extent.x > bar->bounds.extent.y)
		newPos = x - (bar->bounds.topLeft.x);
	else
		newPos = y - (bar->bounds.topLeft.y);
	newPos += GetArrowLen (bar);

	if (firstTime)
		{
		carPos = GetCarPos (bar);
		bar->savePos = carPos;
		bar->penPosInCar = newPos - carPos;
		}
	newPos -= bar->penPosInCar;

	if (newPos < 0)
		newPos = 0;
	else if (newPos > GetDragLen (bar))
		newPos = GetDragLen (bar);
	
	return (newPos);
}   


/***********************************************************************
 *
 * FUNCTION:     GetRegionRect
 *
 * DESCRIPTION:  This routine returns the bounds of the specified region
 *               of the scroll bar.  Ex: the bounds the the down arrow 
 *               region.
 *
 * PARAMETERS:   bar   - pointer to a scroll bar structure
 *               r     - bound the the specified region (returned value)
 *               item  - scroll bar element, ex: dowr arrow.
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/13/96	Initial Revision
 *
 ***********************************************************************/
static void GetRegionRect (ScrollBarPtr bar, RectanglePtr r, ScrollBarRegionType item)
{
   Int16 start, length;
	Int16 arrowLen;
	Int16 carPos;
	
	arrowLen = GetArrowLen (bar);;

	switch(item)
		{
		case sclUpArrow:
			start = 0;
			length = arrowLen;
			break;

		case sclDownArrow:
			start = arrowLen + GetDragLen (bar) + GetCarLen (bar);
			length = arrowLen;
			break;

		case sclUpPage:
			start = arrowLen;
			length = GetCarPos (bar); 
			break;

		case sclDownPage:
			carPos = GetCarPos (bar);
			start = arrowLen + carPos + GetCarLen (bar);
			length = GetDragLen (bar) - carPos;
			break;

		case sclCar:
			start = arrowLen + GetCarPos (bar);
			length = GetCarLen (bar);
			break;
		}

	*r = bar->bounds;
	if (r->extent.x < r->extent.y)
		{
	   r->topLeft.y += start;
		r->extent.y = length;
		}
	else
		{
	   r->topLeft.x += start;
		r->extent.x = length;
		}
}


/***********************************************************************
 *
 * FUNCTION:     DrawArrow
 *
 * DESCRIPTION:  This routine draws the arrow heads on the ends of the 
 *               scroll bar
 *
 * PARAMETERS:   r         - region in which of draw
 *               direction - direction the arrow tip points
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/13/96	Initial Revision
 *
 ***********************************************************************/
static void DrawArrow (RectanglePtr r, WinDirectionType direction)
{
	Int16 x, y;
	Int16 length;
	Int16 row;
	Int16 dx, dy;

	// Calculate point at tip of arrow.
	if (direction <= winDown)
		{	
		// up or down
		x = r->extent.x / 2;
		y = 1;
		length = r->extent.y - 2;
		if (direction == winDown) y += length-1;
		}
	else
		{	
		// right or left
		x = 1;
		y = r->extent.y / 2;
		length = r->extent.x - 2;
		if (direction == winRight) x += length-1;
		} 


	// Convert to actual screen location.
	x += r->topLeft.x;
	y += r->topLeft.y;

	// Draw the arrow tip.
	dx = dy = 0;
	for ( row = 0; row < length; row++)
		{
		switch(direction)
			{	
			case winUp:
				WinDrawLine (x-dx, y+dy, x+dx, y+dy);
				break;
			case winLeft:
				WinDrawLine (x+dx, y-dy, x+dx, y+dy);
				break;
			case winDown:
				WinDrawLine (x-dx, y-dy, x+dx, y-dy);
				break;
			case winRight:
				WinDrawLine (x-dx, y-dy, x-dx, y+dy);
				break;
			}
		dx++;
		dy++;
		}
}


/***********************************************************************
 *
 * FUNCTION:     DrawRegionSelected (was InvertRegion)
 *
 * DESCRIPTION:  This routine draws the region specified (just the arrows
 *					  for now, though) either selected or unselected.
 *
 * PARAMETERS:   
 *					  r        - bounds of the region to invert
 *               region   - up arrow or down arrow.
 *					  selected - whether region should be selected or unselected
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/13/96	Initial Revision
 *			jmp	10/29/99	Changed routine from strict inversion to
 *								drawing selected or unselected as specified.
 *
 ***********************************************************************/
static void DrawRegionSelected (ScrollBarPtr bar, RectanglePtr r, ScrollBarRegionType region, Boolean selected)
{
	// Only the up and down arrows invert, all other region are
	// ignored for now.
	//
	if ( (region == sclUpArrow) || (region == sclDownArrow))
	{
		Boolean vertical = bar->bounds.extent.x < bar->bounds.extent.y;
		
		// Save the current drawing environment.
		//
		WinPushDrawState();
		
		// Set up for either selected or unseleted drawing.  For selected
		// drawing, swap the scrollbar's fore and back colors.  Otherwise,
		// leave the background alone and just set-up for drawing the
		// arrow in its unselected (normal foreground) state.
		//
		if (selected)
		{
			WinSetForeColor(UIColorGetIndex(UIObjectFill));
			WinSetBackColor(UIColorGetIndex(UIObjectForeground));
		}
		else
			WinSetForeColor(UIColorGetIndex(UIObjectForeground));
		
		// Erase to desired background.
		//
		WinEraseRectangle(r, 0);

		// Either draw the up/left arrow or the down/right arrow.
		//
		if ( region == sclUpArrow )
			DrawArrow(r, vertical ? winUp : winLeft);
		else
			DrawArrow(r, vertical ? winDown : winRight);

		// Restore drawing environment.
		//
		WinPopDrawState();
	}
}


/***********************************************************************
 *
 * FUNCTION:     DrawPageScrollRegion
 *
 * DESCRIPTION:  This routine draw the page scroll regions of the scroll
 *               bar.  The page scroll regions are the areas between the
 *               arrows, above add below the car (scroll box).
 *
 * PARAMETERS:   r - region in which of draw
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/13/96	Initial Revision
 *
 ***********************************************************************/
static void DrawPageScrollRegion (RectanglePtr r)
{
	if (r->extent.y && r->extent.x)
		{
		//r->topLeft.x += 2;
		//r->extent.x -= 4;
		WinFillRectangle (r, 0);
		}
}

/***********************************************************************
 *
 * FUNCTION:     DrawScrollCar
 *
 * DESCRIPTION:  This routine draws the eleveator car (scroll box).
 *
 * PARAMETERS:   r - region in which of draw
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	 2/13/96	Initial Revision
 *			gavin	 4/28/99 support horizonontal bars
 *			bob	12/01/99 don't erase too much around scroll car
 *
 ***********************************************************************/
static void DrawScrollCar (ScrollBarPtr bar, RectanglePtr r)
{
	Boolean vertical = bar->bounds.extent.x < bar->bounds.extent.y;
	if (r->extent.x && r->extent.y)
		{
		// Erase above and below the car.
		if (bar->value > bar->minValue)	
			if (vertical)
				WinEraseLine (r->topLeft.x, r->topLeft.y-1, 
					r->topLeft.x + r->extent.x - 1, r->topLeft.y-1);
			else
				WinEraseLine (r->topLeft.x-1, r->topLeft.y, 
					r->topLeft.x-1, r->topLeft.y + r->extent.y - 1);

		if (bar->value < bar->maxValue)	
			if (vertical)
				WinEraseLine (r->topLeft.x, r->topLeft.y + r->extent.y, 
					r->topLeft.x + r->extent.x - 1, r->topLeft.y + r->extent.y);
			else
				WinEraseLine (r->topLeft.x + r->extent.x, r->topLeft.y, 
					r->topLeft.x + r->extent.x, r->topLeft.y + r->extent.y - 1);

		WinDrawRectangle (r, 0);
		}
}

/***********************************************************************
 *
 * FUNCTION:     SclSetScrollBar
 *
 * DESCRIPTION:  This routine sets the current value, the range, and the 
 *               size of a page.  If the scroll bar is visible
 *               it will be redrawn.
 *
 * PARAMETERS:   bar      - pointer to a scroll bar structure
 *					  value    - current value (position)
 *					  min      - minimum value
 *					  max      - maximum value
 *					  pageSize - size of a page (used when page scrolling).
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/4/96	Initial Revision
 *			trev	08/12/97	made non modified passed variables constant
 *
 ***********************************************************************/
void SclGetScrollBar (const ScrollBarType *bar, Int16 * valueP, 
	Int16 * minP, Int16 * maxP, Int16 * pageSizeP)
{
	*maxP = bar->maxValue;
	*minP = bar->minValue;
	*valueP = bar->value;
	*pageSizeP = bar->pageSize;
}


/***********************************************************************
 *
 * FUNCTION:     SclSetScrollBar
 *
 * DESCRIPTION:  This routine sets the current value, the range, and the 
 *               size of a page.  If the scroll bar is visible
 *               it will be redrawn.
 *
 * PARAMETERS:   bar      - pointer to a scroll bar structure
 *					  value    - current value (position)
 *					  min      - minimum value
 *					  max      - maximum value
 *					  pageSize - size of a page (used when page scrolling).
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/13/96	Initial Revision
 *			trev	08/12/97	made non modified passed variables constant
 *
 ***********************************************************************/
void SclSetScrollBar (ScrollBarType *bar, Int16 value, Int16 min, 
	Int16 max, Int16 pageSize)
{
	Boolean redraw = false;
	
	ErrFatalDisplayIf (max < min, "Invalid scroll parameter");

	if (value < min)
		value = min;

	else if (value > max)
		value = max;

	if (bar->attr.visible)
		{
		redraw = (bar->minValue != min) ||
					(bar->maxValue != max) ||
					(bar->value != value) ||
					(bar->pageSize != pageSize);
		}

	bar->minValue = min;
	bar->maxValue = max;
	bar->value = value;
	bar->pageSize = pageSize;
	
	if (redraw)
		SclDrawScrollBar (bar);
}

// 
/***********************************************************************
 *
 * FUNCTION:     SclDrawScrollBar
 *
 * DESCRIPTION:  Inset scroll bar area to make it look skinnier
 *
 * PARAMETERS:   r - area to inset
 *				 vertical - true if bar is oriented vertically.
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			gavin	4/28/99		Initial Revision
  *
 ***********************************************************************/
static void InsetBar(RectangleType *r, Boolean vertical)
{	Int16 indent;		
	if (vertical)
	{	indent = (r->extent.x + 1) >> 2;
		r->topLeft.x += indent;
		r->extent.x -= (indent+indent);
	} else
	{	indent = (r->extent.y + 1) >> 2;
		r->topLeft.y += indent;
		r->extent.y -= (indent+indent);
	}
}

/***********************************************************************
 *
 * FUNCTION:     SclDrawScrollBar
 *
 * DESCRIPTION:  This routine draws a scroll bar.
 *
 * PARAMETERS:   bar - pointer to a scroll bar structure.
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/13/96	Initial Revision
 *			trev	08/12/97	made non modified passed variables constant
 *			gavin  4/28/99 		support horizonontal bars
 *
 ***********************************************************************/
void SclDrawScrollBar (ScrollBarType *bar)
{
	if (bar->attr.usable)
	{
		UInt16 i;
		RectangleType r;
		CustomPatternType backPattern;
	
		// If the scroll bar is visible and the minimum and maximum value
		// are the same, erase the scroll bar.
		if (bar->minValue == bar->maxValue)
			{
			if (bar->attr.visible && bar->attr.shown)
				{
				WinEraseRectangle (&bar->bounds, 0);
				bar->attr.shown = false;
				bar->attr.hilighted = false;
				}
			}
	
		else
			{
			Boolean vertical = bar->bounds.extent.x < bar->bounds.extent.y;
			
			for (i = 0; i < sizeof (CustomPatternType)/2; i++)
				((UInt16*)backPattern)[i] = scrollBarPattern;
				
			WinPushDrawState();
			WinSetForeColor(UIColorGetIndex(UIObjectForeground));
			WinSetBackColor(UIColorGetIndex(UIObjectFill));
			WinSetPattern (&backPattern);
		
			// The scrollbar was not previously visible. We don't know was is draw
			// under the scrollbar
			if (!bar->attr.shown)
				WinEraseRectangle (&bar->bounds, 0);

			if (! bar->attr.hilighted)
				{
				// draw correct arrows for orientation

				GetRegionRect (bar, &r, sclUpArrow);
				DrawArrow (&r, vertical ? winUp : winLeft);
		
				GetRegionRect (bar, &r, sclDownArrow);
				DrawArrow (&r,  vertical ? winDown : winRight);
				}
		
			GetRegionRect (bar, &r, sclUpPage);
			InsetBar(&r,vertical);
			DrawPageScrollRegion (&r);
		
			GetRegionRect (bar, &r, sclDownPage);
			InsetBar(&r,vertical);
			DrawPageScrollRegion (&r);
		
			GetRegionRect (bar, &r, sclCar);
			InsetBar(&r,vertical);
			DrawScrollCar (bar, &r);
		
			WinPopDrawState();
	
			bar->attr.shown = true;
			}
	
		bar->attr.visible = true;
	}
}


/***********************************************************************
 *
 * FUNCTION:     SclHandleEvent
 *
 * DESCRIPTION:  This routine handled event in a scroll bar.  PenDown,
 *					  SclEnter, and SclRepeat event are handled.
 *					  When a penDown event occurs in a scroll bar, a scrEnter
                 event is added to the event queue.  When a scrEnter
                 is received, the pen is 
 *
 * PARAMETERS:   event - pointer to a event structure
 *
 * RETURNED:     true if the event was handled.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/13/96	Initial Revision
 *			trev	08/12/97	made non modified passed variables constant
 *
 ***********************************************************************/
Boolean SclHandleEvent (ScrollBarType *bar, const EventType * event)
{
	ScrollBarRegionType  item;
	RectangleType 			r;
	RectangleType			slopRect;
	EventType   			newEvent;
	Boolean     			penDown;
	Boolean    				handled;
	Int16       			x, y;
	Int16						oldY;
	Int16       			newValue;
	Int16						penPos;
	UInt16  					ticks;
	Int32						lastTick;

	handled = false;

	if (!(bar->attr.usable && bar->attr.visible && bar->attr.shown))
		return (false);
	
	if (bar->minValue == bar->maxValue)
		return (false);

	if (event->eType == penDownEvent)
		{
		if (RctPtInRectangle (event->screenX, event->screenY, &bar->bounds))
			{
			newEvent = *event;
			newEvent.eType = sclEnterEvent;
			newEvent.data.sclEnter.scrollBarID = bar->id;
			newEvent.data.sclEnter.pScrollBar = bar;
			EvtAddEventToQueue (&newEvent);
			handled = true ;
			}
		}


	else if (event->eType == sclEnterEvent || event->eType == sclRepeatEvent)
		{
		if (event->data.sclEnter.scrollBarID != bar->id)
			return (false);

		if (event->eType == sclEnterEvent)
			{
			for (item = sclUpArrow; item <= sclCar; item++)
				{
				GetRegionRect (bar, &r, item);
				if (RctPtInRectangle (event->screenX, event->screenY, &r))
					break;
				}
			bar->attr.hilighted = false;
			bar->attr.activeRegion = item;
			lastTick = TimGetTicks ();
			}

		else
			{
			item = (ScrollBarRegionType) bar->attr.activeRegion;
			lastTick = event->data.sclRepeat.time;
			}

		GetRegionRect (bar, &r, item);

		if (item == sclCar)
			slopRect = bar->bounds;
		else
			slopRect = r;

		if ((item != sclUpPage) && (item != sclDownPage))
			RctInsetRectangle (&slopRect, -slop);
		else if (bar->bounds.extent.x > bar->bounds.extent.y)
			{
			slopRect.topLeft.y -= slop;
			slopRect.extent.y += slop << 1;
			}
		else
			{
			slopRect.topLeft.x -= slop;
			slopRect.extent.x += slop << 1;
			}

		newValue = bar->value;
		penDown = true;
		penPos = bar->savePos;
		oldY = event->screenY;
		while (penDown && (newValue == bar->value))
			{
			PenGetPoint (&x, &y, &penDown);
			if (((y - oldY) > 2) || ((y - oldY) < -2))
				oldY = y;
			else
				y = oldY;

			if ((item == sclCar) && (bar->attr.hilighted))
			   penPos = DragCar (bar, x, y, false);

			if (!penDown)
				{	
				if (bar->attr.hilighted)
					{
				   DrawRegionSelected (bar, &r, item, false);
					bar->attr.hilighted = false;
					}
				}
			else if (RctPtInRectangle (x, y, &slopRect))
				{
				if (penDown)
					{
					if (!bar->attr.hilighted)
						{
						bar->attr.hilighted = true;
						if (item == sclCar)
							penPos = DragCar (bar, x, y, event->eType == sclEnterEvent);
						else
							DrawRegionSelected (bar, &r,item, true);
						}
					}
				}
			else /* moved out of rectangle */
				{
				if (bar->attr.hilighted)
					{
					bar->attr.hilighted = false;
					if (item == sclCar)
						penPos = bar->savePos;
					else
						DrawRegionSelected(bar, &r,item, false);
					}
				}

			if (event->eType == sclEnterEvent)
				ticks = minDelay;  /* NO wait on first event */
			else
				ticks = TimGetTicks() - lastTick;

			if ((item == sclCar) || (bar->attr.hilighted && (ticks >= minDelay)))
				{
				Int16 dragLen;
				
				switch (item)
					{
					case sclUpArrow:
						newValue--;
						break;
					case sclDownArrow:
						newValue++;
						break;
					case sclUpPage:
						newValue -= bar->pageSize;
						break;
					case sclDownPage:
						newValue += bar->pageSize;
						break;
					case sclCar:
						dragLen = GetDragLen(bar);
						if (dragLen <= 0) dragLen = 1; // make sure it isn't zero!
						newValue = (Int16) ((bar->minValue + (penPos * 
							((Int32)(bar->maxValue - bar->minValue))) /
							 dragLen));
						break;
					}
				if (newValue > bar->maxValue) newValue = bar->maxValue;
				if (newValue < bar->minValue) newValue = bar->minValue;
				}
			}

		if ((! penDown || (newValue != bar->value)))
			{
			newEvent = *event;
			newEvent.screenX = x;
			newEvent.screenY = y;
			newEvent.penDown = penDown;
			newEvent.data.sclRepeat.pScrollBar = bar;
			newEvent.data.sclRepeat.value = bar->value;
			newEvent.data.sclRepeat.newValue = newValue;
			newEvent.data.sclRepeat.time = TimGetTicks();
			
			if ((newValue != bar->value))
				newEvent.eType = sclRepeatEvent;
			else
				newEvent.eType = sclExitEvent;
			EvtAddEventToQueue (&newEvent);
			}

		SclSetScrollBar (bar, newValue, bar->minValue, bar->maxValue, bar->pageSize);
			
		handled = true ;
		}

	return (handled);
}


/***********************************************************************
 *
 * FUNCTION:     ScrollBarTest
 *
 * DESCRIPTION:  This routine tests a scroll bar.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/14/96	Initial Revision
 *
 ***********************************************************************/
extern void ScrollBarTest (void);
/*
void ScrollBarTest (void)
{
	Int16 value;
	Int16 min;
	Int16 max;
	Int16 pageSize;
	EventType event;
	ScrollBarType bar;
	
	bar.bounds.topLeft.x = 152;
	bar.bounds.topLeft.y = 18;
	bar.bounds.extent.x = 7;
	bar.bounds.extent.y = 121;
	bar.id = 1;
	bar.attr.usable = true;
	bar.attr.visible = false;
	bar.attr.hilighted = false;
	
	SclSetScrollBar (&bar, 0, 0, 1, 11);
	SclDrawScrollBar (&bar);

	SclGetScrollBar (&bar, &value, &min, &max, &pageSize);
	
	SclSetScrollBar (&bar, value+1, min, max, pageSize);

	{
	Int16 x = 156;
	Int16 y = 79;
	Int16 penPos, newValue;
	Int16 newPos;
	Int16 carPos;
	Int16 penPosInCar;
	
//	penPos = DragCar (&bar, x, y, true);
	
	newPos = y - bar.bounds.topLeft.y + GetArrowLen (&bar);

	carPos = ((long)(bar.value - bar.minValue) * GetDragLen(&bar) ) / 
		(bar.maxValue - bar.minValue + 1);

	penPosInCar = newPos - carPos;

	newPos -= penPosInCar;
	
	penPos = newPos;

	newValue = bar.minValue + (penPos * 
					((long)(bar.maxValue - bar.minValue) + 1)) /
					GetDragLen (&bar);
	}
		



	do
		{
		EvtGetEvent (&event, evtWaitForever);
		
		if (! SysHandleEvent (&event))
			SclHandleEvent (&bar, &event);
		}
	while (event.eType != appStopEvent);
}
*/
