/******************************************************************************
 *
 * Copyright (c) 1995-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: Event.c
 *
 * Release: 
 *
 * Description:
 *	  This file contains the event manager routines.
 *
 * History:
 *		Jan 10, 1995	Created by Art Lamb
 *		09/14/99	gap	Removed EvtGetTrapState.
 *		09/17/99	gap	Tweaked multi-tap threshholds a bit.
 *
 *****************************************************************************/

/* Routines:
 *		EvtInitialize
 *		EvtAddEventToQueue
 *		EvtAddUniqueEventToQueue
 *		EvtCopyEvent
 *		EvtGetEvent
 *		EvtEventAvail
 */

#define NON_PORTABLE

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_CLIPBOARDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_FIELDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS

#include <PalmTypes.h>

#include <DateTime.h>
#include <ErrorMgr.h>
#include <SysEvent.h>
#include <NotifyMgr.h>
#include <SoundMgr.h>
#include <SysEvtMgr.h>
#include <TimeMgr.h>
#include <Window.h>

#include "EmuStubs.h"
#include "Globals.h"
#include "ScreenMgr.h"
#include "UIGlobals.h"
#include "AttentionPrv.h"

/***********************************************************************
 *
 * Local definitions
 *
 ***********************************************************************/

#define	penTapDelay		sysTicksPerSecond / 2
#define	penTapSlop		3


/***********************************************************************
 *
 * FUNCTION:    WindowChange
 *
 * DESCRIPTION: This routine is called by EvtGetEvent when it encounters
 *              a penDownEvent.  This routine will check if the penDown 
 *              is a window other then the active window.
 *
 * PARAMETERS:  x  window-relative coordinate of the pendown
 *              y  window-relative coordinate of the pendown
 *
 * RETURNED:    TRUE is the active window should change
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/10/95	Initial Revision
 *			jfs	4/1/99	Tweaked out-of-bounds tests
 *
 ***********************************************************************/
static Boolean WindowChange (Int16 x, Int16 y)
{
	WinHandle winHandle;
	WinHandle activeWindow;
	WinPtr    winPtr;
	Int16 displayWidth;
	Int16 displayHeight;
	

	if (WinGetDrawWindow())
		WinWindowToDisplayPt (&x, &y);

	activeWindow = WinGetActiveWindow ();
	if (activeWindow)
		{
		winPtr = WinGetWindowPointer (activeWindow);

		// If the point is in the active window, don't change windows.
		//
		if (RctPtInRectangle (x, y, &winPtr->windowBounds))
			return (false);

		// If the point is outside the "desktop", don't change windows.
		// This is to support products that have a digitizer area
		// which is larger than the viewable screen.
		//
		WinGetDisplayExtent(&displayWidth, &displayHeight);
		if ((x >= displayWidth) || (y >= displayHeight))
			return (false);

		// If the active window is modal, don't change windows.
		//
		if (winPtr->windowFlags.modal)
			{
			if (y < displayHeight)
				SndPlaySystemSound(sndError);
			return (false);
			}
		}

	winHandle = WinGetFirstWindow ();
	do
		{
		winPtr = WinGetWindowPointer (winHandle);

		// If this window is the current active then don't check it again.
		// Also, don't check offscreen/disabled windows.
		//
		if ((winHandle != activeWindow) && (winPtr->windowFlags.enabled))
			{
			if (RctPtInRectangle (x, y, &winPtr->windowBounds))
				{
				EnterWindowID = WinGetWindowHandle (winPtr);
				ExitWindowID = activeWindow;
				return (true);
				}
			}

		winHandle = winPtr->nextWindow;
		} while (winHandle);
		
	return (false);
}


/***********************************************************************
 *
 * FUNCTION:    EvtInitialize
 *
 * DESCRIPTION: This routine initializes global variables used by the 
 *              event routines.
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
void SysEventInitialize (void)
{
	EventQIndex = 0;
	EventQLength = 0;

	LastScreenX = 0;
	LastScreenY = 0;
	LastPenDown = false;
}


/***********************************************************************
 *
 * FUNCTION:    EvtAddEventToQueue
 *
 * DESCRIPTION: This routine adds an event to the event queue.
 *
 * PARAMETERS:  event  pointer to the structure that contains the event
 *              error  pointer to any error encountered by this function
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/10/95		Initial Revision
 *			trev	08/07/97	made non modified passed variabls constant
 *
 *
 ***********************************************************************/
void SysEventAddToQueue (const SysEventType * event)
{
	UInt16 index;

	ErrFatalDisplayIf( (EventQLength == eventQueueSize), "Event queue full");

	index = EventQIndex + EventQLength;

	// Wrap around to beginning of the queue if necessary.
	//
	if (index >= eventQueueSize)
		index -= eventQueueSize;
	
	MemMove (&EventQ[index], event, sizeof (SysEventType));
	EventQ[index].id = 0;
	if (WinGetDrawWindow())
		WinWindowToDisplayPt (&EventQ[index].event.screenX, &EventQ[index].event.screenY);
	EventQLength++;
}


/***********************************************************************
 *
 * FUNCTION:    EvtAddUniqueEventToQueue
 *
 * DESCRIPTION: This routine looks for an existing event in the event
 *		queue of the same event type and id (if specified) and replaces 
 *		it with the passed in event, if found. 
 *
 *		If no existing event is found, the event will
 *		be added. Otherwise, if inPlace is true the existing event will
 *		be replaced with the new event or if inPlace is false the existing
 *		event will be removed and the new event will be added to the end.
 *
 * PARAMETERS:  event  	- pointer to the structure that contains the event
 *					 id		- ID of event. 0 means to match only on the type.
 *					 inPlace	- if true, existing event will be replaced.
 *								  if false, existing event will be deleted and
 *									new event added to end of queue.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			ron	8/20/96	Initial Revision
 *			trev	08/07/97	made non modified passed variabls constant
 *
 ***********************************************************************/
void SysEventAddUniqueToQueue (const SysEventType * eventP, UInt32 id, 
	Boolean inPlace)
{
	UInt16 	index, nextIndex;
	UInt16	count;

	//---------------------------------------------------------------------
	// Look for an existing event with this type and possibly ID
	//---------------------------------------------------------------------
	for (index = EventQIndex, count = EventQLength; count; count--) {
		if (id) {
			if (EventQ[index].event.eType == eventP->eType &&
			 		EventQ[index].id == id) break;
			}
		else if (EventQ[index].event.eType == eventP->eType) break;
		
		index++;
		if (index >= eventQueueSize) index = 0;
		}
		
	//---------------------------------------------------------------------
	// If we found one, either replace it or take it out of the queue
	//---------------------------------------------------------------------
	if (count) {
	
		// Replace it if inPlace is true
		if (inPlace) {
			MemMove(&EventQ[index], eventP, sizeof(SysEventType));
			return;
			}
			
		// Otherwise Delete the existing event
		for (count-- ; count; count--) {
			if (index == eventQueueSize-1) 	
				nextIndex = 0;
			else 
				nextIndex = index+1;
			EventQ[index] = EventQ[nextIndex];
			index = nextIndex;
			}
		EventQLength--;
		}	
		
		
	//---------------------------------------------------------------------
	// Add the event to the end
	//---------------------------------------------------------------------
	ErrFatalDisplayIf( (EventQLength == eventQueueSize), "Event queue full");

	index = EventQIndex + EventQLength;

	// Wrap around to beginning of the queue if necessary.
	//
	if (index >= eventQueueSize)
		index -= eventQueueSize;
	
	MemMove (&EventQ[index], eventP, sizeof (SysEventType));
	EventQ[index].id = id;
	if (WinGetDrawWindow())
		WinWindowToDisplayPt (&EventQ[index].event.screenX, &EventQ[index].event.screenY);
	EventQLength++;
}


/***********************************************************************
 *
 * FUNCTION:    EvtCopyEvent
 *
 * DESCRIPTION: This routine copies a event.
 *
 * PARAMETERS:  source  pointer to the structure containing the event to copy
 *              dest    pointer to the structure to copy the event to
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/10/95		Initial Revision
 *			trev	08/07/97	made non modified passed variabls constant
 *
 ***********************************************************************/
void SysEventCopy (const SysEventType * source, SysEventType * dest)
{
	MemMove (dest, source, sizeof (SysEventType));
}


/***********************************************************************
 *
 * FUNCTION:    EvtGetEvent
 *
 * DESCRIPTION: This routine returns the next available event.
 *
 * PARAMETERS:  
 *		event - pointer to the structure to hold the event returned
 *    timeout - max amount of ticks to wait before an event is returned
 *						(-1 means to wait indefinitely).
 *
 * NOTE: timeout should be passed as -1 in most instances. When running
 *   on the device, this will make the CPU go into Doze mode until the
 *   user provides some input. If your application needs to do animation,
 *   it should pass a timeout >= 0.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	01/10/95	Initial Revision
 *			Ain	05/26/95	Added conditional Event Trace calls.
 *			trev	08/07/97	made non modified passed variabls constant
 *			jesse	03/30/99	Added call to MemHandle deferred NotifyMgr broadcasts
 *			gap	9/12/99	Added new multi-tap implementation
 *			gap	9/21/99	Reorder multi-tap position & time check to optimize
 *			bob	3/14/00	simplify GSysNotifyBroadcastPending test for interrupt stuff
 *
 ***********************************************************************/
void SysEventGet (SysEventType * event, Int32 timeout)
{
	Boolean 	changeWindow = false;
	Int32		maxTimeout;
	UInt32	currTapTime;
	SysNotifyParamType notify;

	// Blink the insertion point if it's time to.
	InsPtCheckBlink ();
	
	// Blink the attention manager indicator if it's time to.
	AttnIndicatorCheckBlink();
	
	// Handle any NotifyMgr broadcasts which were deferred, or generated by interrupts
	while (GSysNotifyBroadcastPending)
		// note, if SysNotifyBroadcastFromInterrupt is called here (between a test that
		// fails and the branch instruction) it will set GSysNotifyBroadcastPending to
		// true after it's been tested, and we'll potentially skip a broadcast 'till the
		// next pass of the event loop.  That's a risk I'm willing to take!  --Bob
		SysNotifyBroadcast(0) ;
	
	event->eType = sysEventNilEvent;
	event->penDown = LastPenDown;
	event->screenX = LastScreenX;
	event->screenY = LastScreenY;
	event->tapCount = TapCount;
	if (WinGetDrawWindow())
		WinDisplayToWindowPt (&event->screenX, &event->screenY);

	// Update the host screen with the updated area if running in remote viewing mode
	ScrSendUpdateArea(true);

	do
		{
		// If a window exit event is pending, return it.
		if (ExitWindowID)
			{
			event->eType = sysEventWinExitEvent;
			event->data.winExit.exitWindow = ExitWindowID;
			event->data.winExit.enterWindow = EnterWindowID;
			ExitedWindowID = ExitWindowID;
			ExitWindowID = 0;
			if (! EnterWindowID)
				{
				ActiveWindow = 0;
				WinSetDrawWindow (NULL);
				}	
			break;
			}


		// If a window enter event is pending, return it.
		else if (EnterWindowID)
			{			
			ActiveWindow = EnterWindowID;
			WinSetDrawWindow (EnterWindowID);

			event->eType = sysEventWinEnterEvent;
			event->data.winEnter.enterWindow = EnterWindowID;
			event->data.winEnter.exitWindow = ExitedWindowID;
			EnterWindowID = 0;
			ExitedWindowID = 0;			
			break;
			}

		// If there's an event in the event queue, return it.
		else if (EventQLength)
			{
			MemMove (event, &EventQ[EventQIndex], sizeof (SysEventType));
			EventQLength--;
			EventQIndex++;
			if (EventQIndex >= eventQueueSize)
				EventQIndex = 0;

			if (WinGetDrawWindow())
				WinDisplayToWindowPt (&event->screenX, &event->screenY);

			if (event->eType == penDownEvent)
				{
				LastScreenX = event->screenX;
				LastScreenY = event->screenY;
				}
			
			break;
			}


		// Call the system event manager to get an event, if it returns a
		// pen down event,  check if the pen down changes the active window.
		else 
			{
			// Return with a null event earlier when we have to blink the 
			// insertion point.
			if (InsPtIsEnabled) {
				maxTimeout = InsPtLastTick + insPtBlinkInterval - TimGetTicks();
				if (maxTimeout < 0) maxTimeout = 1;
				if (timeout == -1 || timeout > maxTimeout) 
					timeout = maxTimeout;
				}
			
			// Return with a null event earlier when we have to blink the 
			// attention manager's indicator.
			maxTimeout = AttnIndicatorTicksTillNextBlink();
			if (maxTimeout != -1 && (timeout == -1 || timeout > maxTimeout))
				timeout = maxTimeout;
							
			// Return with a null event earlier if NeedNullTickCount is set
			if (NeedNullTickCount) {
				Int32	delay;
				delay = NeedNullTickCount - TimGetTicks();
				if (delay < 0)
					delay = 0;
				if (timeout == -1 || timeout > delay)
					timeout = delay;
				}
					
			// Get next event with timeout
			EvtGetSysEvent(event, timeout);
			
			// Automatically clear the NeedNullTickCount time if we've passed it
			if (NeedNullTickCount) {
				if (TimGetTicks() >= NeedNullTickCount)
					NeedNullTickCount = 0;
				}
			
			if (WinGetDrawWindow())
				WinDisplayToWindowPt (&event->screenX, &event->screenY);
			if (event->eType == penDownEvent)
				{
				changeWindow = WindowChange (event->screenX, event->screenY);
				if (changeWindow)
					SysEventAddToQueue (event);

				currTapTime = TimGetTicks(); 
				if ( ((currTapTime - LastTapTime) < penTapDelay) &&
					  (LastScreenX >= event->screenX-penTapSlop) && (LastScreenX <= event->screenX+penTapSlop) &&
					  (LastScreenY >= event->screenY-penTapSlop) && (LastScreenY <= event->screenY+penTapSlop)
					)
					TapCount++;
					//ToDo-gap  add a check to catch the unlikely event this could roll around to 0.
				else
					TapCount = 1;
					
				event->tapCount = TapCount;
				LastTapTime = currTapTime;
				}

			LastPenDown = event->penDown;
			LastScreenX = event->screenX;
			LastScreenY = event->screenY;
			}
		
		} while (changeWindow);


	// Print event trace.
	#if EMULATION_LEVEL != EMULATION_NONE
	StubEventTrace(event);

	// Allow exit of apps by hitting the escape key.
	if (event->eType == keyDownEvent && event->data.keyDown.chr == 0x1B) 	
		event->eType = sysEventAppStopEvent;
	#endif

	// Let anyone interested know that we're returning this event.
	// This can be used to detect whether an event is discarded
	// without being passed to SysHandleEvent.
	notify.notifyType = sysNotifyEvtGotEvent;
	notify.broadcaster = sysNotifyBroadcasterCode;
	notify.handled = false;
	notify.notifyDetailsP = event;
	SysNotifyBroadcast(&notify);
}



/***********************************************************************
 *
 * FUNCTION:    EvtEventAvail
 *
 * DESCRIPTION: This routine returns true if an event is available.
 *
 * PARAMETERS:  
 *		void
 *
 * CALLED BY:  Net Library's NetLibSelect() call to determine if 
 *		a user input event is available before blocking on "stdin".
 *
 * RETURNED:    true if event available
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			Ron	4/29/96	Created
 *
 ***********************************************************************/
Boolean SysEventAvail (void)
{

	if (ExitWindowID || EnterWindowID ||  EventQLength) return true;
	return EvtSysEventAvail(false);

}
