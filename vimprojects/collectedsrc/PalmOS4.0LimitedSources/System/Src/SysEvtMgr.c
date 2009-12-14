/******************************************************************************
 *
 * Copyright (c) 1994-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: SysEvtMgr.c
 *
 * Release: 
 *
 * Description:
 *		Low-Level System Event Manager Routines
 *
 * History:
 *   	03/22/95	RM		Created by Ron Marianetti
 *		05/05/98	art	added button layout for the Text services Manager
 *		07/23/98	kwk	Changed UInt16 ascii param in EvtEnqueueKey to WChar.
 *					kwk	In EvtGetPenBtnList, made TSM buttons conditional
 *							on TEXT_SERVICES_ON.
 *		03/16/99	kwk	Rolled in alt button list for Sumo-J.
 *		09/13/99	kwk	Deleted unused PrvSendGremlinsIdle routine.
 *
 *****************************************************************************/

#define	NON_PORTABLE

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_CLIPBOARDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_FIELDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS

#include <PalmTypes.h>

// public ui includes
#include <UIResources.h>

// public system includes
#include <AlarmMgr.h>
#include <Chars.h>
#include <DateTime.h>
#include <TimeMgr.h>
#include <DebugMgr.h>
#include <ErrorMgr.h>
#include <SysEvent.h>
#include <ExgLib.h>
#include <HAL.h>
#include <MemoryMgr.h>
#include <NotifyMgr.h>
#include <PenMgr.h>
#include <Preferences.h>
#include <Rect.h>
#include <SystemMgr.h>
#include <SysEvtMgr.h>

// private system includes
#include "EmuStubs.h"
#include "SysEvtPrv.h"
#include "Globals.h"
#include "NotifyPrv.h"
#include "UIGlobals.h"
#include "TimePrv.h"
#include "SystemPrv.h"
#include "AttentionPrv.h"
#include "PasswordPrv.h"

#include "HwrGlobals.h"
#include "HwrDigitizer.h"
#include "HwrDisplay.h"
#include "HwrKeys.h"
#include "ScreenMgr.h"

/************************************************************
 * Private defines
 *************************************************************/

#define 	evtKeyStringEscLong	0x01		// Escape byte for encoded key strings
													// passed to EvtEnqueueKeyString or stored
													//  in key queue.
#define	evtKeyStringEscShort	0x02		// 3 byte escape sequence

#define	evtSpecialModifiers	(appEvtHookKeyMask | libEvtHookKeyMask | commandKeyMask)

/************************************************************
 * Private routines used only in this module
 *************************************************************/
static Err		PrvEnqueueKeyBytes(KeyQueuePtr keyQP, UInt8 * dataP, UInt16 size);
static Err		PrvPeekStrokeStart(PointType* startPtP);
static Boolean PrvHandleExchangeEvents();

/************************************************************
 *
 *  FUNCTION: EvtSysInit
 *
 *  DESCRIPTION: Initializes the system event manager
 *
 *	PARAMETERS:		Nothing.
 *
 *	RETURNS:			0 or error code
 * 
 *	CALLED BY:		System startup code
 *
 *	HISTORY:
 *		03/24/95	ron	Created by Ron Marianetti
 *		07/14/99	kwk	Initialize silkscreen info ptr.
 *		10/04/99 jmp	autoOffDuration is now in seconds, not in minutes, to allow the
 *							the General prefs panel to include a seconds entry in its list
 *		10/05/99	jmp	autoOffDuration, now that it's in seconds, should be a UInt16
 *							instead of a UInt8; also added peggedAutoOffDuration constants
 *		12/10/99	kwk	Use sysResTSilkscreen vs. silkscreenRscType.
 *
 *************************************************************/
Err	EvtSysInit(void)
{
	SysEvtMgrGlobalsPtr	gP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
	const SilkscreenAreaType* silkAreaP;
	MemPtr					p;
	Err						err;
	Int16						i;
	UInt32					tag = 'EvtM';
	UInt16					timeout, height;
	UInt8						autoOffDuration;
	UInt16					autoOffDurationSecs;
	UInt16					numAreas;
   	SecurityAutoLockType	devAutoLockType;
	
	//----------------------------------------------------------------------
	// Allocate the globals  and initialize them
	//----------------------------------------------------------------------
	if (gP) MemPtrFree(gP);
	GSysEvtMgrGlobalsP = MemPtrNew(sizeof(SysEvtMgrGlobalsType));
	gP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
	if (!gP) {
		ErrDisplay("Couldn't alloc event mgr globals");
		return memErrNotEnoughSpace;
		}
	
	// Initialize them to 0
	MemSet(gP, sizeof(SysEvtMgrGlobalsType), 0);
		
	// Initialize them	
	gP->enableGraffiti = true;
	gP->enableSoftKeys = true;
	
	// Set  this so we don't go right to sleep again. 
	gP->gotUserEvent = true;
	
	//----------------------------------------------------------------------
	// Create the Event Group. This event group serves a dual purpose. Bit
	//  sysRefnumStd (0) is used to block on hardware events like pen
	//  and keyboard. Other bits (1->15 and 17-31) are used to implement
	//  the SysSelect() call for use by applications that want to pend on
	//  socket activity in the Net Library. 
	//----------------------------------------------------------------------
	err = SysEvGroupCreate(&GSysEvGroupID, &tag, 0);
	if (err) DbgBreak();
	
	
	//----------------------------------------------------------------------
	// Allocate the default pen and keyboard queues
	//----------------------------------------------------------------------
	p = MemPtrNew(evtDefaultPenQSize);
	ErrFatalDisplayIf(!p, "Couldn't alloc pen q");
	MemSet(p, evtDefaultPenQSize, 0);
	err = EvtSetPenQueuePtr(p, evtDefaultPenQSize);
	ErrFatalDisplayIf(err, "Err setting pen q ptr");
	
	p = MemPtrNew(evtDefaultKeyQSize);
	ErrFatalDisplayIf(!p, "Couldn't alloc key q");
	MemSet(p, evtDefaultKeyQSize, 0);
	err = EvtSetKeyQueuePtr(p, evtDefaultKeyQSize);
	ErrFatalDisplayIf(err, "Err setting key q ptr");
	
	
	//----------------------------------------------------------------------
	// Init the auto-off timeout in seconds.  Get this from the
	// Preferences database and validate it.
	//----------------------------------------------------------------------
	autoOffDurationSecs	=	(UInt16) PrefGetPreference(prefAutoOffDurationSecs);
	autoOffDuration		= 	(UInt8) PrefGetPreference(prefAutoOffDuration);

	// If either autoOffDuration or autoOffDurationSecs is pegged
	// to the max, that means we don't want to ever automatically go off.  So,
	// in either of those cases, we set the timout value to zero.
	//
	if ( (peggedAutoOffDuration == autoOffDuration) || (peggedAutoOffDurationSecs == autoOffDurationSecs) )
		timeout = 0;
	else
		timeout = autoOffDurationSecs;

	SysSetAutoOffTime(timeout);


   	//-----------------------------------------------------------------------
   	// Init the Device Auto Locking mechanism.
   	//-----------------------------------------------------------------------
   	devAutoLockType = (SecurityAutoLockType)PrefGetPreference(prefAutoLockType);
   
   	SysSetDevAutoLockTime((UInt16)devAutoLockType);
	
   	//Init password timeout mechanism.
	SysSetPwdTimeout(0);
	
	//----------------------------------------------------------------------
	// Set up the silkscreen information.
	//----------------------------------------------------------------------

	GSilkscreenInfoP = MemHandleLock(DmGetResource(sysResTSilkscreen, HwrGetSilkscreenID()));
	
	// DOLATER kwk - validate the screen area in the silkscreen resource
	// against the height/width returned by HwrDisplayAttributes???
	HwrDisplayAttributes(false, hwrDispVertical, (UInt32*)&height);
	gP->appAreaBottom = height;

	// Find the two graffiti areas and use them to init the writingR field.
	silkAreaP = EvtGetSilkscreenAreaList(&numAreas);

	for (i = 0; i < numAreas; i++, silkAreaP++)
		{
		if (silkAreaP->areaType == silkscreenRectGraffiti)
			{
			if (silkAreaP->index == alphaGraffitiSilkscreenArea)
				{
				gP->writingR = silkAreaP->bounds;
				}
			else
				{
				gP->writingR.extent.x += silkAreaP->bounds.extent.x;
				}
			}
		}

	return err;
}


/************************************************************
 *
 *  FUNCTION: EvtGetSysEvent
 *
 *  DESCRIPTION: Checks for low level system events and fills in
 *		event record pointer with event information if an event is
 *		available. If no events are available, this routine will put
 *		the system to sleep and not return until an event is available.
 *
 *		Low level system events are events that are caused by hardware
 *		 - like key presses or pen strokes on the digitizer.
 *
 *		If the event is a pen-up in the Graffiti area, this routine will
 *		pass the stroke to the Graffiti recognizer and return a key event
 *		to the caller. 
 *
 *		If the event is pen-up in a hard icon, this routine will determine
 *		which hard icon was pressed and return a key event with the appropriate
 *		virtual key code.
 *
 *		If none of the above and the pen is still down, this routine will
 *		return a pen down event with the current pen location.
 *
 *		If none of the above, this routine will put the processor to sleep.
 *
 *  PARAMETERS: 
 *		eventP	 	- pointer to event record to fill in
 *		timeout 		- max time to put processor to sleep. -1 means wait forever.
 *
 *  RETURNS: nothing
 * 
 *  CALLED BY: 
 *
 *  CREATED: 3/22/95
 *
 *  BY: Ron Marianetti
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	6/29/95	added timeout parameter to StubProcessMacEvents
 *			vmk	9/11/95	added a call to AlmDisplayAlarm()
 *			vmk	9/12/95	added emulator timer alarm simulation logic
 *			gmp   1/31/98	added exchange manager event handling
 *			dje	9/27/00	fixed bugs 40010, 41085, 41263, 42957
 *
 *************************************************************/
void	EvtGetSysEvent(SysEventType * eventP, Int32 timeout)
{
	SysEvtMgrGlobalsPtr	gP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
	PenQueuePtr				penQP;
	PointType				startPt, endPt; 
	Boolean					sendPenUpToApp = false;
	Boolean					returnNullEvt=false;
	PointType				rawPen;
	Err 						err;
	UInt32					startTime;				// time when this routine is called
	UInt32 					currentTime;			// current time
	Int32						kernelDeltaTimeout;	// delta until kernel should timeout

	//------------------------------------------------------------------------
	// Get the time when we enter this routine so we know when the timeout has
	//  been exceeded. Since we call SysEvGroupWait in a loop, we can't just
	//  pass the buck. dje-9/27/00
	//------------------------------------------------------------------------
	startTime = TimGetTicks();


	//------------------------------------------------------------------
	// If Running under the emulator, give the host system software a chance
	//  to post keyboard and pen events -- the Mac wants the timeout ticks
	//  with same semantics as passed to EvtGetSysEvent() -- so we make
	//  this call without any timeout conversion.
	//
	// Note that this is done even if data exists on the pen or key queue.
	//--------------------------------------------------------------------
	#if EMULATION_LEVEL != EMULATION_NONE
	StubProcessMacEvents(timeout);
	#endif


	//------------------------------------------------------------------------
	// See if we need to remove the top stroke from the pen queue. This is
	//  necessary when we have previously returned a pen-up event to the
	//  application. We remove the stroke for the application if it didn't
	//  do so itself.
	//----------------------------------------------------------------------
	penQP = gP->penQP;
	if (gP->removeTopStroke) {
		if (gP->penQStrokesRemoved == penQP->strokesRemoved)
			EvtFlushNextPenStroke();
		gP->removeTopStroke = false;
		}
		
	
	while (1) {
		
		//------------------------------------------------------------------------
		// Assume there is no low level event by clearing the stdin event flag.
		//  If there really is one pending, we'll find out in this while loop
		//  which looks at both the Key and Pen queues to see if they have
		//  anything in them. 
		// If a low level event occurs soon after we're done checking the pen and
		//  key queues, the  appropriate ISR will set the event flag for us and 
		//  we'll return immediately from SysEvGroupWait().
		// Moved into the loop dje-9/27/00. We do this on every loop iteration so
		//  infrared and serial noise don't cause us to busy-loop. Now that
		//  PrvIrDrvrIntHandler sets the stdin event flag without producing a
		//  nilEvent, we have a situation where the stdin event flag can get set
		//  but never cleared. Clearing it here makes this safe.
		//------------------------------------------------------------------------
		SysEvGroupSignal(GSysEvGroupID, 
					sysEvtGroupMask(sysFileDescStdIn,false),		// mask
					0,														// value
					sysEvGroupSignalConstant);						// type


		//------------------------------------------------------------------------
		// Check for and handle any exhange manager events
		//------------------------------------------------------------------------
		PrvHandleExchangeEvents();
		
		//------------------------------------------------------------------------
		// Do we need to send the dirty area of the screen to the host?
		//------------------------------------------------------------------------
		if (gP->needRemoteScrUpdate) {
			gP->needRemoteScrUpdate = false;
			ScrSendUpdateArea(false);
			}


		//------------------------------------------------------------------------
		// Is there a pending key event to return? If so, return it
		//------------------------------------------------------------------------
		if (!EvtDequeueKeyEvent(eventP, false)) goto Exit;
	
		
		//------------------------------------------------------------------------
		// Is there anything in the pen queue? If so we need to return a 
		//  penDown, penUp, or penMoved event.
		//----------------------------------------------------------------------
		if (penQP->end != penQP->start) {
		
			// Capture the current pen condition.
			rawPen.x = gP->penX;
			rawPen.y = gP->penY;
			eventP->penDown = gP->penDown;
			
			//------------------------------------------------------------------------
			// Did we return the penDown event yet? If not, return it even if the pen
			//  is not currently down. This is so we still get the correct event flow
			//  if the pen is rapidly brought down and up again.
			//----------------------------------------------------------------------
			if (!penQP->returnedPenDown) {
				eventP->eType = sysEventPenDownEvent;
				
				// get the location of the pen down and return the pen-down event
				PrvPeekStrokeStart(&rawPen);
				eventP->penDown = true;
				penQP->returnedPenDown = true;
				goto ExitWithPenInfo;
				}
	
	
			//------------------------------------------------------------------------
			// Is there a complete stroke available???
			// If so, get the end points into the event record and return a pen-up
			//   event to the app.
			//----------------------------------------------------------------------
			else if (penQP->strokeCount) {
			
				// Get the stroke info
				EvtDequeuePenStrokeInfo(&startPt, &endPt);
				
				// Save it in event record
				eventP->data.penUp.start = startPt;
				eventP->data.penUp.end = endPt;
				eventP->eType = sysEventPenUpEvent;
				
				// Save the current value of the number of strokes removed from the
				//  pen queue so that we can detect whether or not the application removed
				//  the stroke from the queue itself.
				gP->penQStrokesRemoved = penQP->strokesRemoved;
				gP->removeTopStroke = true;
				penQP->returnedPenDown = false;
				goto ExitWithPenInfo;
				}

			//------------------------------------------------------------------------
			// Is the pen down? If so and the coordinates have changed, 
			//  return a pen moved event with the current coordinates of the pen
			//
			// If the pen hasn't moved, fall through and go to sleep
			//------------------------------------------------------------------------
			else if (eventP->penDown && gP->lastPenDown) {
				eventP->eType = sysEventPenMoveEvent;
				if ((rawPen.x != gP->lastPenX) || (rawPen.y != gP->lastPenY)) 
					goto ExitWithPenInfo;
				}
					
			}
			

		//------------------------------------------------------------------------
		// Call the Alarm Manager to process any alarms which have expired and reschedule
		// new alarms, but do not display them at this time.  Alarms will be displayed
		// after the UI is notified via the alarmChr virtual key event.
		// We only check the alarms if the unit has no wireless socket open.
		//------------------------------------------------------------------------
//		if ((GCommActivityFlags & sysCommActivityWlsSockOpen) == 0)
//		{
		  AlmDisplayAlarm( false/*displayOnly*/ );
//		}


		//------------------------------------------------------------------------
		// Call the Attention Manager to play the sounds for any attentions which
		// have occurred when the attention dialog can't open.
		//------------------------------------------------------------------------
		AttnDoEmergencySpecialEffects();


		//------------------------------------------------------------------------
		// Did EvtWakeup get called after the SysEvGroupSignal call above?
		//------------------------------------------------------------------------
		if (gP->sendNullEvent) {
			gP->sendNullEvent = 0;
			eventP->eType = sysEventNilEvent;
			goto Exit;
			}
		
			
		//------------------------------------------------------------------------
		// If we got to here, there's nothing to do, so pend on our event group for
		//   a "stdin" event (pen or key).
		//------------------------------------------------------------------------
		
		// This stuff only necessary for the Emulator
		#if EMULATION_LEVEL != EMULATION_NONE
			// Simulate time manager's alarm functionality for the emulator.
			// vmk 9/12/95	11:39am
			TimHandleInterrupt( false/*periodicUpdate*/);

			// Give time to the Key Manager so that it can simulate a hardware keyUp
			// if necessary
			KeyHandleInterrupt(true, 0);


			// Notify the emulator in case it wants to substitue an event for the nil event.
			// If the emulator wants to send a pen or key event it will enqueue it and
			// return true.  We must then continue in this while loop to pull the data 
			// off whatever queue it was added to and send it as an event
			{
				register StubNilEventIsPendingResults result;
				
				result = StubNilEventIsPending();
				if (result == emuSendPenOrKeyEvent)
					// When the loop continues the event posted will be pulled off one
					// of the two queues and sent on it's way
					continue;
				else if (result == emuSendNilEvent)
					{
					// set the event to a nilEvent and leave with it
					eventP->eType = sysEventNilEvent;
					goto Exit;
					}
					
				// At this point neither gremlins or the script playback code posted
				// an event. SysEvGroupWait is free to block the emulator until input
				// if it so wants.

			}
				

		#endif	
		
		
		// <chg 4-1-98 RM>
		// If we haven't encountered a user event since waking up, go right
		// back to sleep. This is to support background jobs that need to wake
		//  up the Pilot, do something quick, then go right back to sleep again.
		// See the comments in SysEvtResetAutoOff(). 
		if (!gP->gotUserEvent) {
			EvtEnqueueKey(autoOffChr, 0, commandKeyMask);
			continue;
			}


		// Compute timeout delta for kernel. dje-9/27/00
		currentTime = TimGetTicks();
		if (timeout == evtWaitForever)
			kernelDeltaTimeout = 0;		// kernel wants zero for infinite
		else if (currentTime - startTime < timeout)
			kernelDeltaTimeout = timeout - (currentTime - startTime);
		else
			kernelDeltaTimeout = -1;	// kernel wants -1 for none
				// We could just return a nilEvent in this case, but we have to call
				// SysEvGroupWait. Apparently it does more than just wait for StdIn.
		
		
		// Wait for a key or pen event to set our event group flag
		//  This flag will be set after a hardware event is detected by an interrupt 
		//  routine. 
		gP->idle = true;
		err = SysEvGroupWait(GSysEvGroupID, 
					sysEvtGroupMask(sysFileDescStdIn,false),		// mask
					sysEvtGroupMask(sysFileDescStdIn,false),		// value
					sysEvGroupWaitOR,									// match type
					kernelDeltaTimeout);
		gP->idle = false;
		
		
		// We'll assume the error is a timeout, in which case we just need to return
		//   a null event.
		if (err) {
			eventP->eType = sysEventNilEvent;
			goto Exit;
			}

		
		
		//------------------------------------------------------------------------
		// Are we supposed to send a null event as requested by EvtWakeup??
		//------------------------------------------------------------------------
		if (gP->sendNullEvent) {
			gP->sendNullEvent = 0;
			eventP->eType = sysEventNilEvent;
			goto Exit;
			}
			

		} // while(1);
	
	

	//------------------------------------------------------------------------
	// Fill in current pen info and exit with event record.
	//------------------------------------------------------------------------
Exit:
		// Capture the current pen condition.
		rawPen.x = gP->penX;
		rawPen.y = gP->penY;
		eventP->penDown = gP->penDown;
			
ExitWithPenInfo:
		// Save last pen position so we can detect penMove events next time we're called
		gP->lastPenDown = eventP->penDown;
		gP->lastPenX = rawPen.x;
		gP->lastPenY = rawPen.y;
		
		// Convert raw digitizer location to screen coordinates
		PenRawToScreen(&rawPen);
		eventP->screenX = rawPen.x;
		eventP->screenY = rawPen.y;
		
	return;
}



/************************************************************
 *
 *  FUNCTION: EvtSysEventAvail
 *
 *  DESCRIPTION: Returns true if a low level system event like a
 *		pen or key event is available.
 *   
 *  PARAMETERS: 
 *		ignorePenUps - if true, this routine will ignore pen-up events
 *			when determining if there are any system events available.
 *		
 *  RETURNS: true if event available
 * 
 *  CALLED BY: EvtEventAvail if no high level software events are 
 *			available.
 *
 *  CREATED: 4/29/96
 *
 *  BY: Ron Marianetti
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			rm		4/29/96	created
 *			dje	10/3/00	handle exchange events
 *
 *************************************************************/
Boolean				EvtSysEventAvail(Boolean ignorePenUps)
{
	SysEvtMgrGlobalsPtr	gP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
	PenQueuePtr				penQP;
	KeyQueuePtr				keyQP;
	
	
	// Are there any keys in the queue???
	keyQP = gP->keyQP;
	if (keyQP->start != keyQP->end) return true;
	
	
	// If we are ignore pen-ups, the logic to determine if there is a
	//  pen event is more complicated...
	penQP = gP->penQP;
	if (ignorePenUps) {
		// If there's data in the queue and we haven't returned a pen down yet,
		//  we definitely have an event
		if (penQP->end != penQP->start && !penQP->returnedPenDown) return true;
		
		// OR, If there's at least 2 stroke starts in the queue, we have a pen
		// down event to return still. This is a case where returnedPenDown can
		//  be true (which gets past the above test) if we haven't finished
		//  returning the first stroke to the app yet. 
		if (penQP->strokesAdded > penQP->strokesRemoved+1) return true;
		}
		
		// Any pen events available? 
	else if (penQP->end != penQP->start) {
		 return true;
		}
	else 	// check for exchange manager events
		PrvHandleExchangeEvents();
		// if (GSysMiscFlags & sysMiscFlagExgEvent) return true;
		
		// dje-10/3/00 - Changed to handle exchange events here and return false
		//		rather than returning true and letting EvtGetSysEvent handle them.
		//		We don't generate nilEvents any more, so returning true was mis-
		//		leading.
	
	return false;
}






/************************************************************
 *
 *  FUNCTION: EvtProcessSoftKeyStroke()
 *
 *	DESCRIPTION:	Translate a stroke in the system area of the
 *		digitizer and enqueue the appropriate key events into the
 *		key queue.
 *
 *  PARAMETERS: 
 *		*startPtP - start point of stroke
 *		*endPtP	 - end point of stroke
 *
 *	RETURNS:			0 if recognized, -1 otherwise
 * 
 *	CALLED BY:		SysHandleEvent/PrvHandleEvent
 *
 *	HISTORY
 *		03/23/95	RM		Created by Ron Marianetti
 *		07/15/99	kwk	Only flush stroke if we found a match.
 *
 *************************************************************/
Err EvtProcessSoftKeyStroke(PointType* startPtP, PointType* endPtP)
{
	const PenBtnInfoType*	infoP;
	UInt16						i, numButtons;
	Err							err = -1;

	// Check for hit in one of the silk screen buttons
	infoP = EvtGetPenBtnList(&numButtons);
	
	for (i=0; i<numButtons; i++, infoP++) {
		if (RctPtInRectangle(startPtP->x, startPtP->y, &infoP->boundsR)) {
			if (RctPtInRectangle(endPtP->x, endPtP->y, &infoP->boundsR)) {
				EvtEnqueueKey(infoP->asciiCode, infoP->keyCode, infoP->modifiers);
				err = errNone;
				break;
				}
			}
		}
		
	// Flush the stroke out, but only if we have a match, as otherwise
	// the caller will want to continue trying to match the stroke
	
	if (err == errNone) {
		EvtFlushNextPenStroke();
		}
		
	return err;
}


/************************************************************
 *
 *  FUNCTION: EvtPenBtnList
 *
 *  DESCRIPTION: Returns pointer to the silk screen button 
 *   array. This array contains the bounds of each silk screen
 *	  button and the ascii code and modifiers byte to generate
 *	  for each button.
 *
 *  PARAMETERS:	numButtons - ptr to btn count variable
 *
 *  RETURNS: pointer to array, # of elements
 * 
 *  CALLED BY: EvtProcessSoftKeyStroke
 *
 *  COMMENTS:
 *
 *  HISTORY:
 *		03/23/95	RM		Created by Ron Marianetti.
 *		05/05/98	art	Added button layout for the Text services Manager
 *		01/18/99	kwk	Added additional layout for Sumo-J.
 *		07/14/99	kwk	Use GSilkscreenInfoP for btn list.
 *
 *************************************************************/
const PenBtnInfoType* EvtGetPenBtnList(UInt16 * numButtons)
{
	SilkscreenInfoType* silkInfoP;
	PenBtnListType* btnInfoP;
	UInt8* infoP;
	
	ErrNonFatalDisplayIf(GSilkscreenInfoP == NULL, "No button list");
	
	// Work our way through the resource (variable size) to get to
	// the button list info.
	silkInfoP = (SilkscreenInfoType*)GSilkscreenInfoP;
	infoP = (UInt8*)&silkInfoP->areas[0];
	infoP += silkInfoP->numAreas * sizeof(SilkscreenAreaType);
	btnInfoP = (PenBtnListType*)infoP;
	
	*numButtons = btnInfoP->numButtons;
	return(btnInfoP->buttons);
}


/************************************************************
 *
 *  FUNCTION: EvtGetSilkscreenAreaList
 *
 *  DESCRIPTION: Returns pointer to the silk screen area 
 *   array. This array contains the bounds of each silk screen
 *	  area.
 *
 *  PARAMETERS:	numAreas - ptr to area count variable
 *
 *  RETURNS: pointer to array, # of elements
 * 
 *  CALLED BY: DOLATER kwk - figure this out.
 *
 *  COMMENTS:
 *
 *  HISTORY:
 *		07/15/99	kwk	Created by Ken Krugler.
 *
 *************************************************************/
const SilkscreenAreaType* EvtGetSilkscreenAreaList(UInt16* numAreas)
{
	SilkscreenInfoType* silkInfoP;
	
	ErrNonFatalDisplayIf(GSilkscreenInfoP == NULL, "No button list");
	
	silkInfoP = (SilkscreenInfoType*)GSilkscreenInfoP;
	*numAreas = silkInfoP->numAreas;
	return(silkInfoP->areas);
}


/************************************************************
 *
 *  FUNCTION: EvtSetPenQueuePtr
 *
 *  DESCRIPTION: Replaces the current pen queue with another
 *		and initializes it.
 *
 *  PARAMETERS: 
 *		penQueueP 	- pointer to new area of memory to use as pen queue
 *		size 			- size of area in bytes.
 *
 *  RETURNS: 0 if no error
 * 
 *  CALLED BY: Applications that wish to install a larger or smaller pen
 *		queue.
 *
 *  CREATED: 3/23/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err				EvtSetPenQueuePtr(MemPtr qSpaceP, UInt32 size)
{
	SysEvtMgrGlobalsPtr	gP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
	UInt16						status;


	// Check for a minimum size
	if (size < sizeof(PenQueueType) + 10)
		return sysErrParamErr;
		

	// Interrupts off while we replace the queue
	status = SysDisableInts();
	
	
	// Now, get rid of the old one and plug in the new one
	if (gP->penQP) {
		MemPtrFree(gP->penQP);
		gP->penQP = 0;
		}
	
	// Plug in the new one - the only field we need to initialize is the size
	gP->penQP = (PenQueuePtr)qSpaceP;
	gP->penQP->size = size - sizeof(PenQueueType);
	
	// Init the new queue
	EvtFlushPenQueue();
	
	// Restore interrupts
	SysRestoreStatus(status);
	
	return 0;
}



/************************************************************
 *
 *  FUNCTION: EvtFlushPenQueue
 *
 *  DESCRIPTION: Flushes all points out of the pen queue
 *
 *  PARAMETERS: 
 *		aaa *  RETURNS: 0 if no error
 * 
 *  CALLED BY: Hopefully nobody
 *
 *  CREATED: 3/23/95
 *
 *  BY: Ron Marianetti
 *  CHANGE HISTORY:
 *
 *	 03/23/95	ron		Initial version
 *  11/11/99	srj		Added check for unitialized globals
 *								to prevent crashing if this code is called
 *								before the queue is set up.
 *
 *************************************************************/
Err				EvtFlushPenQueue(void)
{
	SysEvtMgrGlobalsPtr	gP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
	PenQueuePtr				penQP = gP->penQP;
	UInt16						status;

	
	// an unintialized queue is by definition empty, so we just return
	if((gP == 0) || (penQP == 0))
		return(0);
		
	// Interrupts off while we muck with the queue header
	status = SysDisableInts();
	
	
	// Initialize the queue
	penQP->start = 0;
	penQP->end = 0;
	
	penQP->strokeCount = 0;
	penQP->returnedPenDown = false;

	penQP->addLast.x = -1;
	penQP->addLast.y = -1;
	penQP->addStrokeStart = -1;
	
	penQP->strokesRemoved = 0;
	penQP->strokesAdded = 0;
	penQP->rmvStrokeStage = -1;
	
	
	// Update main globals
	gP->removeTopStroke = false;


	// Restore interrupts
	SysRestoreStatus(status);
	
#if EMULATION_LEVEL != EMULATION_NONE
	//	This will wipe out the events in the Mac event queue, which would 
	//	otherwise be fed back into the TD pen queue, and hence, the TD
	//	event queue.
	StubFlushEvents ();
#endif	// EMULATION_LEVEL != EMULATION_NONE
	
	return 0;
}




/************************************************************
 *
 *  FUNCTION: EvtPenQueueSize
 *
 *  DESCRIPTION: Returns the size of the current pen queue in bytes. 
 *
 *  PARAMETERS: none
 *
 *  RETURNS: size of queue in bytes
 * 
 *  CALLED BY: Applications that wish to see how large the current pen queue is.
 *
 *  CREATED: 3/23/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt32				EvtPenQueueSize(void)
{
	SysEvtMgrGlobalsPtr	gP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
		
	// Get the size
	return (gP->penQP->size + sizeof(PenQueueType));
}




/************************************************************
 *
 *  FUNCTION: EvtEnqueuePenPoint
 *
 *  DESCRIPTION: Called by the digitizer interrupt routine to enqueue
 *   a pen coordinate into the pen queue. The data in the pen queue is
 *   stored in compressed format as follows:
 *
 *    Stroke start point x (2 bytes)
 *    Stroke start point y (2 bytes)
 *    Stroke end point x   (2 bytes)
 *    Stroke end point y   (2 bytes)
 *		StrokePoints 			(variable size bytes)
 *    
 *  The StrokePoints (which is always terminated with the byte 0x00), are 
 *		stored as follows. Note that the  start and end points of the stroke
 *    are NOT included in the StrokePoints array. The StrokePoints array
 *    can contain no points in which case it will contain only the terminator 
 *    byte (0x00).
 *
 *		if (-3 >= dx <= 3)  && (-7 >= dy <= 7)
 *        0XXXYYYY				<- single byte where XXX is dx and YYY is dy
 *
 *    else if (-63 >= dx <= 63) && (-127 >= dy <= 127)
 *        1XXXXXXX  YYYYYYYY  <- 2 bytes where XXXXXXX is dx and YYYYYYYY is dy
 *
 *		else
 *			 11000000  XXXXXXXX XXXXXXXX YYYYYYYY YYYYYYYY
 *										<- 5 bytes where XXXXXXX XXXXXXX is the x coordinate
 *											and YYYYYYYY YYYYYYYY is the y coordinate.
 *	   else
 *        00000000  end of StrokePoints.
 *
 *
 *  PARAMETERS: 
 *		x - x coordinate of penpoint, or -1 for pen-up
 *		
 *
 *  RETURNS: size of queue in bytes
 * 
 *  CALLED BY: Applications that wish to see how large the current pen queue is.
 *
 *  CREATED: 3/23/95
 *
 *  BY: Ron Marianetti
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	2/4/99	bail out if not enough room in queue to create a new stroke
 *
 *************************************************************/
Err				EvtEnqueuePenPoint(PointType* ptParmP)
{
	Err 						err = 0;
	SysEvtMgrGlobalsPtr	gP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
	register PenQueuePtr	penQP = gP->penQP;
	register UInt8 *		dataP;
	Int16						roomLeft;
	register Int16			size;
	Int16						dx, dy, absDx, absDy;
	Int16						end, max;
	register PointType*	ptP;
	
	
	// Make sure we've been initialized
	if (!gP) return evtErrParamErr;
	
	// Check for nil pen queue
	penQP = gP->penQP;
	if (!penQP) return evtErrParamErr;
	ptP = ptParmP;
	
	
	// Check for out of bounds...
	#if EMULATION_LEVEL != EMULATION_NONE
	if (	ptP->x < -1 || 
			ptP->x > 1000 || 
			ptP->y < -1 || 
			ptP->y > 1000)
		ErrDisplay("Invalid Point passed");
	#endif
		
	// Save current pen coordinates in SysEvtMgr globals
	if (ptP->x >= 0) {
		gP->penX = ptP->x;
		gP->penY = ptP->y;
		gP->penDown = true;
		}
	else
		gP->penDown = false;
	
	
	// If this point is the same as the last one, ignore it. This will
	//  catch the cases of multiple pen-ups in a row also.
	if (ptP->x == penQP->addLast.x && ptP->y == penQP->addLast.y) return 0;
	
	
	// Reset the key manager double-tap detection
	KeyResetDoubleTap();
	
	
	// Set up queue local variables
	dataP = penQP->data + penQP->end;
	size = penQP->size;
	
		
	// Handle pen-up
	if (ptP->x < 0) {
		if (penQP->addStrokeStart < 0) return 0;		// ignore if no stroke in process
		
		// Terminate the stroke
		*dataP++ = 0;
		if (++penQP->end == size) penQP->end = 0;
		

		// Save last point in stroke info header
		end = penQP->addStrokeStart + 4;
		if (end >= size)  end -= size; 
		dataP = penQP->data + end;
		
		*dataP++ = (UInt8) (penQP->addLast.x >> 8);
		if (++end == size) { end = 0; dataP = penQP->data; }
		*dataP++ = (UInt8) (penQP->addLast.x);
		if (++end == size) { end = 0; dataP = penQP->data; }
		
		*dataP++ = (UInt8) (penQP->addLast.y >> 8);
		if (++end == size) { end = 0; dataP = penQP->data; }
		*dataP++ = (UInt8) (penQP->addLast.y);
		if (++end == size) { end = 0; dataP = penQP->data; }
		
		
		// Add 1 to the stroke count and clear the strokeStart.
		penQP->strokeCount++;
		penQP->addStrokeStart = -1;
		penQP->addLast = *ptP;
		
		goto Exit;
		}
	
	// Make sure there's room in the queue for the worst case point size of
	//  5 bytes + terminator (possible on next call) + buffer byte
	// or a whole stroke, which is 8 bytes plus a terminator plus a buffer byte
	roomLeft = penQP->start - penQP->end;
	if (roomLeft <= 0) roomLeft += penQP->size;
	
	// note: we could move this check out to append as much as will fit
	// in 1, 2, 5, or 8 byte cases below, but we're saving code
	if (roomLeft < 10) {
		// ErrFatalDisplay("can't append to stroke\n");
		err = evtErrQueueFull;
		goto Exit;
		}
	
	// If this is the start of a stroke, setup the stroke info
	if (penQP->addStrokeStart < 0) {
	
		penQP->addStrokeStart = penQP->end;
		penQP->strokesAdded++;
		
		*dataP++ = (UInt8) (ptP->x >> 8);
		if (++penQP->end == size) { penQP->end = 0; dataP = penQP->data; }
		*dataP++ = (UInt8) ptP->x;
		if (++penQP->end == size) { penQP->end = 0; dataP = penQP->data; }
		
		*dataP++ = (UInt8) (ptP->y >> 8);
		if (++penQP->end == size) { penQP->end = 0; dataP = penQP->data; }
		*dataP++ = (UInt8) ptP->y;
		if (++penQP->end == size) { penQP->end = 0; dataP = penQP->data; }
		
		dataP += 4;
		penQP->end += 4;
		if (penQP->end >= size)  penQP->end -= size; 
		}
	
	
	// Else, get the delta from the last point and save that in the queue
	else {
		dx = ptP->x - penQP->addLast.x;
		absDx = (dx < 0 ? -dx : dx);
		
		dy = ptP->y - penQP->addLast.y;
		absDy = (dy < 0 ? -dy : dy);
		
		max = absDx | (absDy >> 1);
		
		// 1 byte encoding...
		if ((max & 0xFFFC) == 0 ) {
			*dataP = ((dx << 4) | (dy & 0x0F)) & 0x7F;
			if (++penQP->end == size) { penQP->end = 0; }
			}
			
		// 2 byte encoding...
		else if ((max & 0xFFC0) == 0) {
			*dataP++ = (UInt8) (dx | 0x80);
			if (++penQP->end == size) { penQP->end = 0; dataP = penQP->data; }
			*dataP++ = (UInt8) dy;
			if (++penQP->end == size) { penQP->end = 0;}
			}
		
		// 5 byte encoding....
		else {
			*dataP++ = 0xC0;
			if (++penQP->end == size) { penQP->end = 0; dataP = penQP->data; }
			*dataP++ = (UInt8) (ptP->x >> 8);
			if (++penQP->end == size) { penQP->end = 0; dataP = penQP->data; }
			*dataP++ = (UInt8) ptP->x;
			if (++penQP->end == size) { penQP->end = 0; dataP = penQP->data; }
			*dataP++ = (UInt8) (ptP->y >> 8);
			if (++penQP->end == size) { penQP->end = 0; dataP = penQP->data; }
			*dataP++ = (UInt8) (ptP->y);
			if (++penQP->end == size) { penQP->end = 0; }
			}
		}
	
	
	// Save last X and Y
	penQP->addLast = *ptP;
	
	// Record this event to support auto-off 
	GSysAutoOffEvtTicks = GHwrCurTicks;
	
Exit:
	// Set the Event flag to wake up event manager
	SysEvGroupSignal(GSysEvGroupID, 
				sysEvtGroupMask(sysFileDescStdIn,false),		// mask
				sysEvtGroupMask(sysFileDescStdIn,false),		// value
				sysEvGroupSignalConstant);						// type
	return err;
}




/************************************************************
 *
 *  FUNCTION: EvtDequeueStrokeInfo
 *
 *  DESCRIPTION: Called to initiate the extraction of a stroke from the
 *   pen queue. This routine MUST be called before calling EvtDequeuePenPoint.
 *   It will return the start and end points of the stroke.
 *
 *   Subsequent calls to EvtDequeuePenPoint will return points starting
 *   at the start point in the stroke and including the end point. After
 *   the end point is returned, the next call to EvtDequeuePenPoint will return
 *   the point -1,-1.
 *   
 *   Refer to the comments on EvtEnqueuePenPoint for a description of how
 *   pen points are compressed in the pen queue.
 *
 *  PARAMETERS: 
 *		*startPtP	- start point returned here
 *		*startPtP	- end point returned here
 *		
 *  RETURNS: 0 if no error
 * 
 *  CALLED BY: EvtGetSysEvent
 *
 *  CREATED: 3/23/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err				EvtDequeuePenStrokeInfo(PointType* startPtP, PointType* endPtP)
{
	SysEvtMgrGlobalsPtr	gP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
	register PenQueuePtr	penQP;
	register UInt8 *		dataP;
	register Int16			size;
	
	
	// Set up the queue variables
	penQP = gP->penQP;
	dataP = penQP->data + penQP->start;
	size = penQP->size;
	
	
	// Make sure we're at the start of a stroke extraction
	ErrFatalDisplayIf(penQP->rmvStrokeStage >= 0, "Pen q out of sync");
	ErrFatalDisplayIf(penQP->strokeCount == 0, "Pen q out of sync");
	
	
	// The start point is in the first 4 bytes
	startPtP->x = *dataP++;
	if (++(penQP->start) == size) {penQP->start = 0; dataP = penQP->data;}
	startPtP->x = (startPtP->x << 8) | *dataP++;
	if (++(penQP->start) == size) {penQP->start = 0; dataP = penQP->data;}
	
	startPtP->y = *dataP++;
	if (++(penQP->start) == size) {penQP->start = 0; dataP = penQP->data;}
	startPtP->y = (startPtP->y << 8) | *dataP++;
	if (++(penQP->start) == size) {penQP->start = 0; dataP = penQP->data;}
	


	// The end point is the next 4 bytes
	endPtP->x = *dataP++;
	if (++(penQP->start) == size) {penQP->start = 0; dataP = penQP->data;}
	endPtP->x = (endPtP->x << 8) | *dataP++;
	if (++(penQP->start) == size) {penQP->start = 0; dataP = penQP->data;}
	
	endPtP->y = *dataP++;
	if (++(penQP->start) == size) {penQP->start = 0; dataP = penQP->data;}
	endPtP->y = (endPtP->y << 8) | *dataP++;
	if (++(penQP->start) == size) {penQP->start = 0; }
	
	
	// Set stage of removal
	penQP->rmvStrokeStage = 0;
	penQP->strokeCount--;
	
	
	// Save the start point in the pen queue header for the next call to
	//  EvtDequeuPenPoint since it must return the start point first.
	penQP->rmvStartPt = *startPtP;
		
		
	// Convert to screen coordinates before returning
	PenRawToScreen(startPtP);
	PenRawToScreen(endPtP);
	
	
	return 0;
}
		




/************************************************************
 *
 *  FUNCTION: PrvPeekStrokeStart, private
 *
 *  DESCRIPTION: Called to peek at the starting point for the next
 *		stroke to be extracted from the pen queue.
 *
 *   This is called by EvtGetSysEvent when it returns a pen-down event
 *		in order to get the starting point of the stroke. By doing this
 *		we return the correct starting position even if the pen has
 *		subsequently moved or been lifted.
 *
 *  PARAMETERS: 
 *		*startPtP	- start point returned here in RAW COORDINATES!!!!!
 *		
 *  RETURNS: 0 if no error
 * 
 *  CALLED BY: EvtGetSysEvent
 *
 *  CREATED: 3/23/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
static Err			PrvPeekStrokeStart(PointType* startPtP)
{
	SysEvtMgrGlobalsPtr	gP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
	register PenQueuePtr	penQP;
	register UInt8 *		dataP;
	register Int16			size;
	Int16						start;
	
	
	// Set up the queue variables
	penQP = gP->penQP;
	start = penQP->start;
	dataP = penQP->data + start;
	size = penQP->size;
	
	
	// Make sure we have points in the queue
	ErrFatalDisplayIf(penQP->end == start, "Pen q out of sync");

	
	// The start point is in the first 4 bytes
	startPtP->x = *dataP++;
	if (++start == size) {start = 0; dataP = penQP->data;}
	startPtP->x = (startPtP->x << 8) | *dataP++;
	if (++start == size) {start = 0; dataP = penQP->data;}
	
	startPtP->y = *dataP++;
	if (++start == size) {start = 0; dataP = penQP->data;}
	startPtP->y = (startPtP->y << 8) | *dataP++;
	if (++start == size) {start = 0; dataP = penQP->data;}
	

	return 0;
}
		





/************************************************************
 *
 *  FUNCTION: EvtDequeuePenPoint
 *
 *  DESCRIPTION: Called by the recognizers to get the next pen point
 *    out of the pen queue. Will return the point (-1,-1) at the end
 *    of a stroke.
 *   
 *    Refer to the comments on EvtEnqueuePenPoint for a description of how
 *    pen points are compressed in the pen queue.
 *
 *  PARAMETERS: 
 *		*retP	- return point.
 *		
 *  RETURNS: 0 if no error
 * 
 *  CALLED BY: Recognizers that wish to extract the points of a stroke
 *
 *  CREATED: 3/23/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err				EvtDequeuePenPoint(PointType* retP)
{
	SysEvtMgrGlobalsPtr	gP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
	register PenQueuePtr	penQP;
	register UInt8 *		dataP;
	register Int16			size;
	UInt8						byte;
	register Int16			dx, dy;
	Int8						sbyte;
	
	
	// Get pointer to pen queue
	penQP = gP->penQP;
	
	
	// Check for Errors
	ErrFatalDisplayIf(penQP->rmvStrokeStage<0, "Pen q out of sync");
	
	
	// If this is the first call, it's for the start point which was
	//  already extracted in the call to EvtDequeueStrokeInfo and saved
	//  in the queue header
	if (penQP->rmvStrokeStage == 0) {
		*retP = penQP->rmvStartPt;
		penQP->rmvLast = *retP;
		penQP->rmvStrokeStage = 1;
		PenRawToScreen(retP);
		return 0;
		}
		
	
	// Else, we're getting a mid point so set up the queue variables
	dataP = penQP->data + penQP->start;
	size = penQP->size;


	// If this is a mid point or the end point, extract it
	byte = *dataP++;
	if (++penQP->start == size) {penQP->start = 0; dataP = penQP->data;}
	
	
	// If no more points in the stroke...
	if (byte == 0)	{								// end point
		retP->x = -1;
		retP->y = -1;
		penQP->rmvStrokeStage = -1;
		penQP->strokesRemoved++;
		return 0;
		}
		
	// 1 byte encoding...
	else if ((byte & 0x80) == 0) {
		dx = byte >> 4;
		if (dx & 0x04) dx |= 0xFFF8;
		dy = byte & 0x0F;
		if (dy & 0x08) dy |= 0xFFF0;
		
		retP->x = penQP->rmvLast.x = penQP->rmvLast.x + dx;
		retP->y = penQP->rmvLast.y = penQP->rmvLast.y + dy;
		}
		
		
	// 2 byte encoding...
	else if (byte != 0xC0) {
		dx = byte & 0x7F;
		if (dx & 0x40) dx |= 0xFF80;
		
		sbyte = *dataP++;
		if (++penQP->start == size) {penQP->start = 0; dataP = penQP->data;}
		dy = (Int16)sbyte;
		
		retP->x = penQP->rmvLast.x = penQP->rmvLast.x + dx;
		retP->y = penQP->rmvLast.y = penQP->rmvLast.y + dy;
		}
		
		
	// 5 byte encoding...
	else {
		retP->x = *dataP++;
		if (++penQP->start == size) {penQP->start = 0; dataP = penQP->data;}
		
		retP->x = (retP->x << 8) + *dataP++;
		if (++penQP->start == size) {penQP->start = 0; dataP = penQP->data;}
		
		retP->y = *dataP++;
		if (++penQP->start == size) {penQP->start = 0; dataP = penQP->data;}
		
		retP->y = (retP->y << 8) + *dataP++;
		if (++penQP->start == size) {penQP->start = 0; dataP = penQP->data;}
		
		penQP->rmvLast = *retP;
		}
		
		
	// Scale the point before returning it
	PenRawToScreen(retP);
	return 0;
}
		




/************************************************************
 *
 *  FUNCTION: EvtFlushNextPenStroke
 *
 *  DESCRIPTION: Flushes the next stroke out of the pen queue. If 
 *   a stroke has already been partially dequeued, by calling 
 *	  EvtDequeuePenStrokeInfo, this routine will finish the stroke
 *   dequeueing. Otherwise, this routine will dequeue the next
 *   stroke in the queue.
 *
 *  PARAMETERS: 
 *		
 *  RETURNS: 0 if no error
 * 
 *  CALLED BY: Recognizers that only need the start and end points of
 *   a stroke.
 *
 *  CREATED: 3/23/95
 *
 *  BY: Ron Marianetti
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			grant	11/30/99	Decrement stroke count when flushing stroke info
 *
 *************************************************************/
Err				EvtFlushNextPenStroke(void)
{
	SysEvtMgrGlobalsPtr	gP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
	register PenQueuePtr	penQP;

	register UInt8 *		dataP;
	register Int16			size;
	
	UInt8						byte;



	// Setup queue variables
	penQP = gP->penQP;
	dataP = penQP->data + penQP->start;
	size = penQP->size;
	
	// Return if no pen data 
	if (penQP->strokesRemoved == penQP->strokesAdded) return 0;
	
	
	// Have we gotten the stroke info yet? If not, flush that out first
	if (penQP->rmvStrokeStage < 0) {
		penQP->start += 8;									// size of stroke info
		if (penQP->start >= size)  penQP->start -= size;
		dataP = penQP->data + penQP->start;
		penQP->strokeCount--;
		}

	
	// Flush points till we get to the null terminator
	while(1) {
		byte = *dataP++;
		if (++penQP->start == size) {penQP->start = 0; dataP = penQP->data;}
		
		if (byte == 0) break;
		
		// 1 byte encoding...
		if (byte < 128) continue;
		
		// 2 byte encoding...
		else if (byte != 0xC0) {
			dataP++;
			if (++penQP->start == size) {penQP->start = 0; dataP = penQP->data;}
			}
			
		// 5 byte encoding...
		else {
			dataP += 4;
			penQP->start += 4;
			if (penQP->start >= size) penQP->start -= size;
			dataP = penQP->data + penQP->start;
			}
		}
		
	// Stroke flushed
	penQP->rmvStrokeStage = -1;
	penQP->strokesRemoved++;
	return 0;
}



/************************************************************
 *
 *  FUNCTION: EvtGetPen
 *
 *  DESCRIPTION: Return the current status of the pen
 *
 *  PARAMETERS: 
 *		*pScreenX - x location relative to current window
 *    *pScreenY - y location relative to current window
 *    *pPenDown - true or false
 *
 *  RETURNS: nothing
 * 
 *  CALLED BY: Various UI routines
 *
 *  CREATED: 3/24/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
void EvtGetPen(Int16 *pScreenX, Int16 *pScreenY, Boolean *pPenDown)
{
	SysEvtMgrGlobalsPtr	gP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
	PointType	screenPen;
	
	#if EMULATION_LEVEL != EMULATION_NONE

	StubEnqueuePenPosition();
			
	#else
	
	// Wait for 1 tick, or a pen change, whichever comes sooner
	gP->idle = true;
	SysEvGroupWait(GSysEvGroupID, 
			sysEvtGroupMask(sysFileDescStdIn,false),		// mask
			sysEvtGroupMask(sysFileDescStdIn,false),		// value
			sysEvGroupWaitOR,									// match type
			1);

	gP->idle = false;


	//------------------------------------------------------------------------
	// Update the screen always here. This makes clicking on Buttons seem
	//  faster.
	//------------------------------------------------------------------------
	ScrSendUpdateArea(true);
	#endif
	
		
	// Convert raw digitizer location to screen coordinates
	screenPen.x = gP->penX;
	screenPen.y = gP->penY;
	PenRawToScreen(&screenPen);

	*pScreenX = screenPen.x;
	*pScreenY = screenPen.y;
	*pPenDown = gP->penDown;
	if (WinGetDrawWindow())
		WinDisplayToWindowPt(pScreenX, pScreenY);
}


/************************************************************
 *
 *  FUNCTION: EvtSetKeyQueuePtr
 *
 *  DESCRIPTION: Replaces the current key queue with another
 *		and initializes it.
 *
 *  PARAMETERS: 
 *		keyQueueP 	- pointer to new area of memory to use as pen queue
 *		size 			- size of area in bytes.
 *
 *  RETURNS: 0 if no error
 * 
 *  CALLED BY: Applications that wish to install a larger or smaller key
 *		queue.
 *
 *  CREATED: 3/24/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err				EvtSetKeyQueuePtr(MemPtr qSpaceP, UInt32 size)
{
	SysEvtMgrGlobalsPtr	gP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
	UInt16						status;


	// Check for a minimum size
	if (size < sizeof(KeyQueueType) + 5)
		return sysErrParamErr;
		

	// Interrupts off while we replace the queue
	status = SysDisableInts();
	
	
	// Now, get rid of the old one and plug in the new one
	if (gP->keyQP) {
		MemPtrFree(gP->keyQP);
		gP->keyQP = 0;
		}
	
	// Plug in the new one - the only field we need to initialize is the size
	gP->keyQP = (KeyQueuePtr)qSpaceP;
	gP->keyQP->size = size - sizeof(KeyQueueType);
	
	// Init the new queue
	EvtFlushKeyQueue();
	
	// Restore interrupts
	SysRestoreStatus(status);
	
	return 0;
}



/************************************************************
 *
 *  FUNCTION: EvtKeyQueueSize
 *
 *  DESCRIPTION: Returns the size of the current key queue in bytes. 
 *
 *  PARAMETERS: none
 *
 *  RETURNS: size of queue in bytes
 * 
 *  CALLED BY: Applications that wish to see how large the current key queue is.
 *
 *  CREATED: 3/24/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt32				EvtKeyQueueSize(void)
{
	SysEvtMgrGlobalsPtr	gP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
		
	// Get the size
	return (gP->keyQP->size + sizeof(KeyQueueType));
}




/************************************************************
 *
 *  FUNCTION: EvtFlushKeyQueue
 *
 *  DESCRIPTION: Flushes all points out of the Key queue
 *
 *  PARAMETERS: 
 *		
 *  RETURNS: 0 if no error
 * 
 *  CALLED BY: Hopefully nobody
 *
 *  CREATED: 3/24/95
 *
 *  BY: Ron Marianetti
 *
 *  CHANGE HISTORY:
 *	 03/24/95	ron		Initial version
 *  11/11/99	srj		Added check for unitialized globals
 *								to prevent crashing if this code is called
 *								before the queue is set up.
 *
 *************************************************************/
Err				EvtFlushKeyQueue(void)
{
	SysEvtMgrGlobalsPtr	gP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
	KeyQueuePtr				keyQP = gP->keyQP;
	UInt16						status;
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	
	// an unintialized queue is by definition empty, so we just return
	if((gP == 0) || (keyQP == 0))
		return(0);
		
	// Interrupts off while we muck with the queue header
	status = SysDisableInts();
	
	// Initialize the queue
	keyQP->start = 0;
	keyQP->end = 0;
	
	// This pure evil hack makes sure the attention manager doesn't think
	// a vchrAttnStateChanged character is in the key queue when in fact
	// it was flushed.
	if(attnGlobalsP)
		attnGlobalsP->hasStateChanged = false;		// Allow more to enter queue.
	
	// Restore interrupts
	SysRestoreStatus(status);
	
	return 0;
}



/************************************************************
 *
 *  FUNCTION: EvtEnqueueKey
 *
 *  DESCRIPTION: Called by the keyboard interrupt routine and the Graffiti
 *   and SoftKeys recognizers to place keys into the key queue. Note that
 *   because both interrupt non-interrupt level code can post keys into 
 *   the queue, this routine will disable interrupts while the queue header
 *   is being modified.
 *
 *
 *   Most keys in the queue take only 1 byte - if they have no modifiers and
 *    no virtual key code, and are 8 bit ascii. If a key event contains a
 *		wide (16-bit) character code, but still has no modifiers/key code, then
 *		it takes up three bytes in the following format:
 *
 *			evtKeyStringEscShort	- 1 byte
 *			char code				- 2 bytes
 *
 *		For all other cases, it will take up 6 bytes of storage and will have
 *		the following format:
 *
 *			evtKeyStringEscLong	- 1 byte
 *			char code				- 2 bytes
 *			virtual key code		- 2 bytes
 *			modifiers				- 2 bytes
 *
 *  PARAMETERS: 
 *		ascii 		- character code of key
 *    keycode 		- virtual key code of key
 *    modifiers   - modifiers for key event
 *
 *  RETURNS: result code or 0 if no error.
 * 
 *  CALLED BY: Applications that wish to see how large the current pen queue is.
 *
 *	HISTORY:
 *		03/23/95	ron	Created by Ron Marianetti.
 *		09/13/99	kwk	Added support for short escape sequence.
 *
 *************************************************************/
Err EvtEnqueueKey(WChar ascii, UInt16 keycode, UInt16 modifiers)
{
	SysEvtMgrGlobalsPtr	gP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
	register KeyQueuePtr	keyQP;
	UInt8						byte;
	UInt8						byteArray[10];
	Err						err;
	
	// Check for nil globals, this could happen if the user holds down
	//  a key during boot and we get a key interrupt before we have a chance
	//  to init the SysEvtMgr.
	if (!gP) return evtErrParamErr;
	
	
	// Check for nil key queue
	keyQP = gP->keyQP;
	if (!keyQP) return evtErrParamErr;
	
	
	// See if we can use 1 byte or short escape encoding...
	if ((keycode == 0)
	&& (modifiers == 0)
	&& (ascii != evtKeyStringEscShort)
	&& (ascii != evtKeyStringEscLong)) {
		if (ascii < 256) {
			byte = ascii;
			err = PrvEnqueueKeyBytes(keyQP, &byte, 1);
			}
		
		// 3 byte encoding
		else {
			byteArray[0] = evtKeyStringEscShort;
			byteArray[1] = (UInt8) (ascii >> 8);
			byteArray[2] = (UInt8) ascii;
			err = PrvEnqueueKeyBytes(keyQP, byteArray, 3);
			}
		}

	// 7 byte encoding
	else {
		byteArray[0] = evtKeyStringEscLong;
		byteArray[1] = (UInt8) (ascii >> 8);
		byteArray[2] = (UInt8) ascii;
		byteArray[3] = (UInt8) (keycode >> 8);
		byteArray[4] = (UInt8) keycode;
		byteArray[5] = (UInt8) (modifiers >> 8);
		byteArray[6] = (UInt8) (modifiers);
		err = PrvEnqueueKeyBytes(keyQP, byteArray, 7);
		}
		
		
	// Wake up event manager and return result code
	SysEvGroupSignal(GSysEvGroupID, 
				sysEvtGroupMask(sysFileDescStdIn,false),		// mask
				sysEvtGroupMask(sysFileDescStdIn,false),		// value
				sysEvGroupSignalConstant);						// type

	return err;
}


/************************************************************
 *
 *  FUNCTION: EvtKeyQueueEmpty
 *
 *  DESCRIPTION: Returns true if the key queue is currently empty.
 *   
 *  PARAMETERS: 
 *		void
 *		
 *  RETURNS: true if empty
 * 
 *  CALLED BY: KeyManager to determine if it should enqueue auto-repeat
 *		keys.
 *
 *  CREATED: 3/24/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Boolean				EvtKeyQueueEmpty(void)
{
	SysEvtMgrGlobalsPtr	gP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
	register KeyQueuePtr	keyQP;
	
	// Set up key queue pointer
	keyQP = gP->keyQP;
	
	// Are there any keys in the queue???
	if (keyQP->start == keyQP->end) return true;
	return false;
}



/************************************************************
 *
 *	FUNCTION:	EvtDequeueKeyEvent
 *
 *	DESCRIPTION: Called by EvtGetSysEvent to get the next key out
 *		of the key queue in the form of a key event. We're only
 *		called from one place so we don't have to disable interrupts during
 *		this call.
 *   
 *		Refer to the comments on EvtEnqueueKey for a description of how
 *		keys are stored in the key queue
 *
 *	PARAMETERS: 
 *		*eventP	- event record.
 *		peek		- if true, leave key in key queue.
 *		
 *	RETURNS: 0 if no error
 * 
 *	CALLED BY: EvtGetSysEvent
 *
 *	HISTORY:
 *		03/24/95	ron	Created by Ron Marianetti.
 *		07/03/96	RM		Added peek parameter to allow peeking at the next key without
 *							removing it. I'm assuming (hopefully correctly) that no apps
 *							are using this call yet and that adding the peek parameter will
 *							not cause incompatibilities. <ROM2.0>
 *		09/13/99	kwk	Added support for short escape encoding.
 *		10/29/99	kwk	Catch case of virtual key w/o cmd bit set.
 *
 *************************************************************/
Err EvtDequeueKeyEvent(SysEventType * eventP, UInt16 peek)
{
	SysEvtMgrGlobalsPtr	gP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
	register KeyQueuePtr	keyQP;
	
	register UInt8 *		dataP;
	register Int16			size;
	UInt8						byte;
	register	UInt16		start;
	WChar						chr;
	
	// Set up key queue pointer
	keyQP = gP->keyQP;
	
	
	// Are there any keys in the queue???
	start = keyQP->start;
	if (start == keyQP->end) return evtErrQueueEmpty;
	

	// Set up queue locals
	dataP = keyQP->data + start;
	size = keyQP->size;
	eventP->eType = sysEventKeyDownEvent;


	// Extract the next byte
	byte = *dataP++;
	if (++start == size) {start = 0; dataP = keyQP->data;}
	
	// Is it an long escape sequence?? If so, extract 6 more bytes
	if (byte == evtKeyStringEscLong) {
		eventP->data.keyDown.chr = *dataP++;
		if (++start == size) {start = 0; dataP = keyQP->data;}
		eventP->data.keyDown.chr = (eventP->data.keyDown.chr << 8) | *dataP++;
		if (++start == size) {start = 0; dataP = keyQP->data;}
	
		eventP->data.keyDown.keyCode = *dataP++;
		if (++start == size) {start = 0; dataP = keyQP->data;}
		eventP->data.keyDown.keyCode = (eventP->data.keyDown.keyCode << 8) | *dataP++;
		if (++start == size) {start = 0; dataP = keyQP->data;}
	
		eventP->data.keyDown.modifiers = *dataP++;
		if (++start == size) {start = 0; dataP = keyQP->data;}
		eventP->data.keyDown.modifiers = (eventP->data.keyDown.modifiers << 8) | *dataP++;
		if (++start == size) {start = 0;}
		}
	
	// Is it an short escape sequence?? If so, extract 2 more bytes.
	else if (byte == evtKeyStringEscShort) {
		eventP->data.keyDown.chr = *dataP++;
		if (++start == size) {start = 0; dataP = keyQP->data;}
		eventP->data.keyDown.chr = (eventP->data.keyDown.chr << 8) | *dataP++;
		if (++start == size) {start = 0;}

		eventP->data.generic.datum[1] = 0;
		eventP->data.generic.datum[2] = 0;
		}
	
	// If not, its a simple case
	else {
		eventP->data.keyDown.chr = byte;
		eventP->data.generic.datum[1] = 0;
		eventP->data.generic.datum[2] = 0;
		}
	
	
	// If we're not just peeking, update the start variable in the queue to reflect
	//  the key we just dequeued
	if (!peek)
		keyQP->start = start;

	// Check to see if the event is a virtual keydown with the
	// command bit set in the modifiers field. We do this check
	// here rather than in EvtEnqueueKey() because we want to
	// display a fatal alert on debug ROMs, but that's not a good
	// idea at interrupt time, which is when EvtEnqueueKey is
	// typically called. Note that virtual keys are >= 256 and
	// < 8000, assuming no Unicode implementation...don't forget
	// the page up/down chars are also virtual, and in the control
	// range. This check should be removed when Unicode support
	// is added.
	chr = eventP->data.keyDown.chr;
	if ((chr == vchrPageUp)
	||  (chr == vchrPageDown)
	||  ((chr >= 256) && (chr < 0x8000))) {
		if ((eventP->data.keyDown.modifiers & evtSpecialModifiers) == 0) {
			ErrNonFatalDisplay("Virtual key in queue without command bit set");
			eventP->data.keyDown.modifiers |= commandKeyMask;
			}
		}
		
	return 0;
}



/************************************************************
 *
 *  FUNCTION: EvtWakeup
 *
 *  DESCRIPTION: Can be called by interrupt routines in order to
 *    force the Event Manager to wake-up and send a null event to
 *    the current app.
 *
 *  PARAMETERS: 
 *		
 *  RETURNS: 0 if no error
 * 
 *  CALLED BY: Interrupt routines, like the Sound Manager and Alarm Manager
 *
 *  CREATED: 5/4/95
 *
 *  BY: Ron Marianetti
 *
 *	HISTORY:
 *		04/20/2000	ryw	calls EvtWakeupSendNilEvent now.
 *************************************************************/
Err				EvtWakeup(void)
{
	SysEvtMgrGlobalsPtr	gP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
	
	// Set boolean to force Event Manager to return a null event if there are no
	//   other events available
	gP->sendNullEvent = true;

	return EvtWakeupWithoutNilEvent();
}

/************************************************************
 *
 *  FUNCTION: EvtWakeupWithoutNilEvent
 *
 *  DESCRIPTION: Can be called by interrupt routines in order to
 *    force the Event Manager to wake-up without sending a 
 *		nilEvent to the current app.  
 *
 *  PARAMETERS: 
 *			none
 *		
 *  RETURNS: 0 if no error
 * 
 *  CREATED: 4/20/2000
 *
 *  BY: Russell Y. Webb
 *
 *************************************************************/
Err 			EvtWakeupWithoutNilEvent()
{

	// Set the event flag to wakeup event manager
	SysEvGroupSignal(GSysEvGroupID, 
				sysEvtGroupMask(sysFileDescStdIn,false),		// mask
				sysEvtGroupMask(sysFileDescStdIn,false),		// value
				sysEvGroupSignalConstant);							// type
				

	return 0;
}


/************************************************************
 *
 *  FUNCTION: EvtResetAutoOffTimer
 *
 *  DESCRIPTION: Can be called periodically by other managers to
 *    reset the auto-off timer in order to assure that the device 
 *		doesn't automatically power off  during a long operation that 
 *		doesn't have user input (like a lot of serial port activity, for example). 
 *
 *		It is also called whenever a low level event occurs that is
 *		caused by the user or requires user attention. Examples are
 *		key presses and application alarms (not the new procedure alarms
 *		though). 
 *
 *		Note, as of PalmOS 4.0, this routine should NOT be called for
 *		all alarms, but just for application alarms. Thus, AlmAlarmCallback
 *		has been modified to *not* call this but instead to wait for
 *		AlmDisplayAlarm() to call it if it finds out that an application
 *		alarm is triggered (vs. one of the new procedure alarms). Since
 *		the AMX Timer proc no longer calls SysSleep() directly but instead
 *		enqueues an auto-off key, there is a guaranteed at least 1 cycle
 *		through EvtGetEvent(),SysHandleEvent() every time the device wakes
 *		even if it goes right back to sleep again. 
 *
 *		A new facility in PalmOS4.0, for Jerry initially, is the
 *		capability for a background job to wake up the Pilot, 
 *		perform some simple task, then put it right back to sleep again
 *		WITHOUT turning on the LCD.
 *
 *		Jerry needs this in order to support background re-charging of the
 *		Ni-Cad battery used in the radio. The facility would also be
 *		useful for things like pagers that want to receive pages in 
 *		the background. The Ni-Cad charging in Jerry will wake up the
 *		Pilot using a new facility in the Alarm Manager for procedure
 *		alarms. These are alarms that when triggered, simply call
 *		a procedure pointer rather than calling an application action code. 
 *
 *		There is new logic in EvtGetSysEvent() that will put the device
 *		to sleep if it gets around to the point where it is about to
 *		call SysEvGroupWait() and the SysEvtMgrGlobalsPtr->gotUserEvent
 *		flag is still clear. This flag is cleared when the device goes
 *		to sleep (by HwrSleep()) and set the first time EvtResetAutoOffTimer()
 *		is called after a wakeup.
 *
 *		EvtResetAutoOffTimer() is called by the Key Manager when a key gets
 *		pressed, and by the Alarm Manager when an application alarm
 *		(NOT a procedure alarm) is triggered. 
 *
 *		The new logic for going to sleep is now:
 *			--------------------------------------------------------  
 *		  	Go to sleep if auto-off timer has expired
 *						 		- OR -
 *		  	No user event has occurred since we woke up and we're about
 *			to block on SysEvGroupSignal in EvtGetSysEvent().
 *			--------------------------------------------------------  
 *
 *		In either case, an auto-off key is enqueued by one of the above
 *		two conditions, and SysHandleEvent() eventually sees this key
 *		and calls SysSleep(). 
 *
 *		The other important change is the ability to perform the background
 *		job without the LCD on. To support this, HwrWake() no longer calls
 *		HwrDisplayWake(). Instead, HwrDisplayWake() is now called from EvtResetAutoOffTimer().
 *		With this new logic, the LCD can stay in low power mode until a
 *		real user event occurs (and EvtResetAutoOffTimer() is called). If no 
 *		user event ever occurs, the device will go right back to sleep without
 *		having to power the LCD on and off again. 
 *
 *		When the device wakes up from a key press for example, the
 *		following happens:
 *			1.) Key interrupt wakes up CPU, stub interrupt handler
 *					just returns with interrupts disabled.
 *			2.) HwrWake() powers on the peripherals (except for LCD, see below)
 *			3.) Interrupts are re-enabled by HwrSleep() when HwrWake() returns
 *						to it.
 *			4.) Real Key interrupt handler executes, enqueues the key
 *					and calls EvtResetAutoOffTimer() which turns on the LCD
 *					and sets the gotUserEvent flag.
 *			5.) Device stays on until auto-off timer expires or power key
 *					is pressed since the gotUserEvent flag is set.
 *
 *		When the device wakes up for a background job, like a
 *				procedure alarm, the following happens. 
 *			1.) Alarm interrupt wakes up CPU, stub interrupt handler
 *					just returns with interrupts disabled.
 *			2.) HwrWake() powers on the peripherals (except for LCD, see below)
 *			3.) Interrupts are re-enabled by HwrSleep() when HwrWake() returns
 *						to it.
 *			4.) Real Alarm Interrupt handler executes, calls AlmAlarmCallback
 *					which calls EvtWakeup() (Note, it no longer calls 
 *					EvtResetAutoOffTimer() - AlmDisplayAlarm will call this if
 *					any of the triggered alarms are application alarms (vs.
 *					procedure alarms).
 *			5.) SysSleep() returns to SysHandleEvent()
 *			6.) App eventually calls EvtGetEvent() again, EvtGetEvent() calls
 *					EvtGetSysEvent().
 *			7.) EvtGetSysEvent() eventually calls AlmDisplayAlarm()
 *			8.) AlmDisplayAlarm() walks through the alarm list and eventually
 *					calls the procedure for the procedure alarm that was set then
 *					returns to EvtGetSysEvent()
 *			9.) EvtGetSysEvent() then checks the gotUserEvent flag, notices
 *					that it is not set yet, and enqueues an auto-off key.
 *			10.) EvtGetSysEvent() returns with the auto-off key to EvtGetEvent()
 *			11.) App passes auto-off key to SysHandleEvent() which then calls
 *					SysSleep().
 *
 *  PARAMETERS: 
 *		
 *  RETURNS: 0 if no error
 * 
 *  CALLED BY: SerialLinkMgr, possibly other IO managers
 *					Caution - this routine is called from interrupts too!
 *
 *  HISTORY: 
 *		8/21/95   RM		Created By Ron Marianetti
 *		4/1/98	 RM		Modified to set the new gotUserEvent flag and
 *										to call HwrLCDWake() if necessary
 *		8/2/99    jesse	Changed to call EvtSetAutoOffTimer to 
 *								do the actual resetting of the timer.
 *								This function has additional weird side 
 *								effects like turning the screen on.
 *		8/5/99	TLW		Removed call to SysNotifyBroadcast which
 *								hangs if called from interrupt routine.
 *		9/29/99  jesse		Only reset brightness if we have SW brightness
 *
 *************************************************************/
Err				EvtResetAutoOffTimer(void)
{
	Err err = EvtSetAutoOffTimer(ResetTimer, 0);
	
	// See if the LCD needs waking
	#if EMULATION_LEVEL == EMULATION_NONE
	if (GHwrWakeUp & hwrWakeUpLCD)
		{
			HwrDisplayWake();
			
			// if we have SW brightness, reset the brightness to user's setting
//			if(GHwrMiscFlagsExt & hwrMiscFlagExtHasSWBright)
//				SysLCDBrightness(true, SysLCDBrightness(false,0));
		}
	#endif
	
	
	return err;
}



/************************************************************
 *
 *  FUNCTION: EvtSetAutoOffTimer
 *
 *  DESCRIPTION: Can be called periodically by other managers to
 *    reset the auto-off timer in order to assure that the device 
 *		doesn't automatically power off  during a long operation that 
 *		doesn't have user input (like a lot of serial port activity, for example). 
 *
 *		It is also used to manage the auto-off timer in general.
 *		It currently accepts 5 commands:
 *			SetAtLeast:		Set the device to turn off in >= xxx seconds
 *			SetExactly:		Set the timer to turn off in xxx seconds
 *			SetAtMost:		Set the device to turn off in <= xxx seconds
 *			SetDefault:		Change default auto-off timeout to xxx seconds
 *			ResetTimer:		Reset the auto off timer
 *
 *
 *  PARAMETERS: 
 *		cmd		- One of the above defined commands
 *		timeout	- A new timeout in seconds, ignored for the 'reset' command.
 *		
 *  RETURNS: 0 if no error
 *
 *  HISTORY: 
 *		9/12/99   jesse	Initial revision
 *
 *************************************************************/
Err EvtSetAutoOffTimer(EvtSetAutoOffCmd cmd, UInt16 timeoutSecs)
{
	SysEvtMgrGlobalsPtr	gP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
	UInt32					newTimeout;
	
	// Calculate the new auto-off tick count, if we set it.
	// First, get the time for shutting off right now.
	newTimeout = GHwrCurTicks - ((Int32)GSysAutoOffSeconds*sysTicksPerSecond) - 1;
	
	// Then, add the new timeout to the auto-off time.
	newTimeout += ((UInt32)timeoutSecs * sysTicksPerSecond);
	
	switch(cmd)
		{
		case SetAtLeast:		// turn off in at least xxx seconds
			if(GSysAutoOffEvtTicks < newTimeout)
					GSysAutoOffEvtTicks = newTimeout;
			break;
			
		case SetExactly:		// turn off in xxx seconds
			GSysAutoOffEvtTicks = newTimeout;
			break;
			
		case SetAtMost:		// turn off in at most xxx seconds
			if(GSysAutoOffEvtTicks > newTimeout)
					GSysAutoOffEvtTicks = newTimeout;
			break;
			
		case SetDefault:		// change default auto-off timeout to xxx seconds
			GSysAutoOffSeconds = timeoutSecs;
			break;
			
		case ResetTimer:

	        //Hack...
			// When auto locking is enabled, if the device is turned off
			// by the Hard Power button and auto locking preset delay 
			// has not expired yet, we calculate the time remaining to 
			// lock based on the auto off timer start (This is done in
			// SysHandleEvent() in the vchrHardPower event).
			// The problem is that the EvtResetAutoOffTimer() is called 
			// in the Key manager before the event occurs. So, here we keep
			// track of the Auto Off timer before it is reset.  WK 7/6/00
			GSysHardPowerEvtTicks = GSysAutoOffEvtTicks;

			// Capture the current ticks 
			GSysAutoOffEvtTicks = GHwrCurTicks;
			
			// Indicate that we got a user event so that EvtGetSysEvent() doesn't
			//  put us to sleep
			gP->gotUserEvent = true;
			
			break;
			
		
		}
	
	return errNone;
}



/************************************************************
 *
 *  FUNCTION: EvtEnableGraffiti
 *
 *  DESCRIPTION:  Set Graffiti enabled or disabled
 *
 *  PARAMETERS: true to enable Graffiti
 *
 *  RETURNS: nothing
 * 
 *  CALLED BY: 
 *
 *  CREATED: 11/8/95
 *
 *  BY: Roger Flores
 *
 *************************************************************/
void	EvtEnableGraffiti(Boolean enable)
{
	SysEvtMgrGlobalsPtr	gP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
	
	gP->enableGraffiti = enable;
}


/************************************************************
 *
 *  FUNCTION: EvtSetNullEventTick
 *
 *  DESCRIPTION:  Set the tick when a null event is due, unless
 *							one is already due sooner.
 *
 *  PARAMETERS: tick - the tick when a null event should occur
 *
 *  RETURNS: true if anything changed
 * 
 *  CALLED BY: 
 *
 *  CREATED: 10/8/99
 *
 *  BY: Jameson Quinn
 *
 *************************************************************/
Boolean EvtSetNullEventTick(UInt32 tick)
{
   if (NeedNullTickCount == 0 ||
       NeedNullTickCount > tick ||
       NeedNullTickCount <= TimGetTicks ())
      {
      NeedNullTickCount = tick;
      return true;
      }
   return false;
}


/************************************************************
 *
 *  FUNCTION: SysNotifyBroadcastFromInterrupt
 *
 *  DESCRIPTION:  Enqueue a message in a special queue to
 *						cause the UI thread to do a notify next
 *						time through the event loop.  This
 *						routine is interrupt safe.
 *
 *  PARAMETERS: 	notifyType -> type of notification to send
 *					 	broadcaster -> unique ID of broadcast code
 *					 	notifyDetailsP -> 4 more bytes of data
 *
 *  RETURNS: 		evtErrQueueFull <- if queue is full
 *
 *  HISTORY: 
 *		3/13/00   bob	Initial revision
 *		3/27/00	 john b. added gP->sendNullEvent to give it something to do
 *
 *************************************************************/
Err SysNotifyBroadcastFromInterrupt(UInt32 notifyType, UInt32 broadcaster, void *notifyDetailsP)
{
	KeyQueuePtr	keyQP = (KeyQueuePtr)GSysNotifyInterruptGlobalsP;
	Err err;
	SysNotifyInterruptQueueElementType byteArray;
	SysEvtMgrGlobalsPtr	gP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
	
	// Set boolean to force Event Manager to return a null event if there are no
	//   other events available
	gP->sendNullEvent = true;

	
	byteArray.notifyType = notifyType;
	byteArray.broadcaster = broadcaster;
	byteArray.notifyDetailsP = (UInt32)notifyDetailsP;
	
	err = PrvEnqueueKeyBytes(keyQP, (UInt8 *)&byteArray, sizeof(byteArray));
	if (err == evtErrQueueFull) err = sysNotifyErrQueueFull;

	// set flag so event loop notices the queue is full	
	GSysNotifyBroadcastPending = true;

	// Wake up UI thread
	SysEvGroupSignal(GSysEvGroupID, 
				sysEvtGroupMask(sysFileDescStdIn,false),		// mask
				sysEvtGroupMask(sysFileDescStdIn,false),		// value
				sysEvGroupSignalConstant);							// type


	return err;
}


#pragma mark -------

/************************************************************
 * PRIVATE
 *------------------------------------------------------------------------
 *  FUNCTION: PrvEnqueueKeyByte, private
 *
 *  DESCRIPTION: Because both interrupt non-interrupt level code can post keys into 
 *   the queue, this routine must be used to post bytes into the key queue.
 *   It will disable interrupts while it adds the specified bytes to the queue.
 *
 *
 *  PARAMETERS: 
 *		ascii 		- ascii code of key
 *    keycode 		- virtual key code of key
 *    modifiers   - modifiers for key event
 *		
 *
 *  RETURNS: size of queue in bytes
 * 
 *  CALLED BY: Applications that wish to see how large the current pen queue is.
 *
 *  CREATED: 3/23/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
#if defined(__MC68K__)
Err		asm	PrvEnqueueKeyBytes(KeyQueuePtr keyQP, UInt8 * dataP, UInt16 size)
{
			fralloc +
			MOVEM.L	A2/D3, -(SP)								// save work registers
			
			MOVE.W	SR, -(SP)									// save status register
			ORI.W		#0x0700, SR									// interrupts off
			
		// Setup register variables
			MOVE.L	keyQP, A2									// queue ptr
			MOVE.L	dataP, A0									// data to enqueue
			MOVE.W	struct(KeyQueueType.end)(A2), D2		// end of queue
			MOVE.W	struct(KeyQueueType.size)(A2), D3	// size of queue
			LEA		struct(KeyQueueType.data)(A2), A1	// start of data
			ADD.W		D2, A1										// where new data goes
			MOVE.W	size, D1										// # of bytes to enqueue
			
			
		// Make sure there's room for the data
			MOVE.W	struct(KeyQueueType.start)(A2), D0	// start
			SUB.W		D2, D0										// start - end
			BGT.S		@NoWrap										// no wrap-->
			ADD.W		D3, D0										// wrap around
@NoWrap:
			CMP.W		D1, D0										// enough room for these bytes?
			BGT.S		@PutInQueue									// yes -->
			MOVE.W	#evtErrQueueFull, D0						// error code
			BRA.S		@Exit
			
			
		// Loop putting bytes in
@PutInQueue:
			SUBQ.W	#1, D1										// 0 base # of bytes
		
@NextByte:
			MOVE.B	(A0)+, (A1)+								// put byte in
			ADDQ.W	#1, D2										// bump end
			CMP.W		D2, D3										// wrap?
			BNE.S		@DoneByte									// no -->
			
			CLR.W		D2												// wrap to 0
			LEA		struct(KeyQueueType.data)(A2), A1	// start of data
			
@DoneByte:
			DBRA		D1, @NextByte								// more?
			
			
		// Put the new end in the queue header
			MOVE.W	D2, struct(KeyQueueType.end)(A2)		// write out new end of queue
			CLR.W		D0												// no error
			
			
		// Exit
@Exit:	MOVE.W	(SP)+, SR
			MOVEM.L	(SP)+, A2/D3
			frfree
			RTS
}
#else

// DOLATER: May want ASM versions of this routine if this is too slow
Err PrvEnqueueKeyBytes(KeyQueuePtr keyQP, UInt8 * dataP, UInt16 size)
{
	Err err = 0;
	Int16 status;
	Int16 startQ, endQ, sizeQ, spaceInQ;
	register UInt8 * srcP;
	register UInt8 * dstP;

	// Disable interrupts while enqueueing keys
	status = SysDisableInts();
	
	// Use some locals to speed things up
	endQ = keyQP->end;
	startQ = keyQP->start;
	sizeQ = keyQP->size;
	dstP = keyQP->data + endQ;
	srcP = dataP;
	
	// Make sure there's room for the data
	spaceInQ = startQ - endQ;
	if (spaceInQ <= 0)
		spaceInQ += sizeQ;
	if (size > spaceInQ) {
		err = evtErrQueueFull;
		goto Exit;
		}
		
	// Loop putting bytes in
	while (size--) {
		*dstP++ = *srcP++;
		if (++endQ == sizeQ) {
			// Wrap to the beginning of the queue
			endQ = 0;
			dstP = keyQP->data;
			}
		};
	
	// Put the new end in the queue header
	keyQP->end = endQ;
	
Exit:
	// Restore interrupt state
	SysRestoreStatus(status);
	return err;
}
#endif


/***********************************************************************
 *
 *  FUNCTION:     PrvHandleExchangeEvents
 *
 *  DESCRIPTION:  Checks flag set at interrupt level to send an event
 *						to an exchange manager library (irLib uses this)
 *
 *  PARAMETERS:   none
 *
 *  RETURNS:      true if an event was handled 
 *
 *  CALLED BY:    
 *
 *  CREATED:      1/31/98
 *
 *  BY:          Gavin Peacock
 *
 ***********************************************************************/
static Boolean PrvHandleExchangeEvents()
{		
	// Exchange library interface for handling high priorty interrupt info
	// sends a special key event to the active library

	Boolean exgEvent = false;
	// atomic test and set so that we don't miss any events
	UInt16 sr = SysDisableInts();
	if (GSysMiscFlags & sysMiscFlagExgEvent)
		{
		GSysMiscFlags &= ~sysMiscFlagExgEvent;
		exgEvent = true;
		}
	SysRestoreStatus(sr);
		
	if (exgEvent && GExgActiveLib)
		{
		SysEventType evt;
		evt.eType = sysEventKeyDownEvent;
		evt.data.keyDown.modifiers = libEvtHookKeyMask;
		evt.data.keyDown.keyCode = GExgActiveLib;
		evt.data.keyDown.chr = exgIntDataChr;
		ExgLibHandleEvent(GExgActiveLib,&evt);
		}
	return exgEvent;
}
