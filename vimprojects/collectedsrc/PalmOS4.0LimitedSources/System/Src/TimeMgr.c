/******************************************************************************
 *
 * Copyright (c) 1995-1999 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: TimeMgr.c
 *
 * Release: 
 *
 * Description:
 *	  This file contains the time manager routines.
 *
 * NOTES:
 *
 * History:
 *		Feb 6, 1995	Created by Roger Flores
 *		4/24/95		vmk		Implemented and moved the native Time Manager
 *									API to TimeMgr68328.
 *		9/9/95		vmk		Implemented emulation mode Time Manager alarms
 *		9/12/95		vmk		Removed Mac alarm calls.  Alarms are now monitored
 *									in EvtGetSysEvent().
 *
 *****************************************************************************/

/* Routines:
 *		TimGetSeconds
 *		TimSetSeconds
 *		TimGetTicks
 */

#define NON_PORTABLE

#include <PalmTypes.h>

#if EMULATION_LEVEL != EMULATION_NONE

// public system includes
#include <AlarmMgr.h>
#include <ErrorMgr.h>
#include <MemoryMgr.h>
#include <NotifyMgr.h>
#include <SystemMgr.h>
#include <TimeMgr.h>

// private system includes
#include "EmuStubs.h"
#include "Globals.h"
#include "TimePrv.h"


//-------------------------------------------------------------------
// Structures used only by this module
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// Global variables used by this module
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// Private routines used only by this module
//-------------------------------------------------------------------

static UInt32	PrvSetAlarmLow(UInt32 alarmSeconds, Boolean repeat);
static void PrvBroadcastTimeChange(UInt32 delta);


//-------------------------------------------------------------------
// Time Manager Initialization routines
//-------------------------------------------------------------------


/*********************************************************************************
 *  FUNCTION: TimInit
 *
 *  DESCRIPTION: Initializes the Time Manager.
 *
 *		Allocate and initialize the Time Manager globals;
 *		Enable the 24 Hr interrupt;
 *		Set RTC reference frequency to 32.768 kHz and enable the RTC;
 *		Unmask the RTC interrupt;
 *
 *  PARAMETERS: void
 *
 *  RETURNS: 0 if successful; otherwise: timErrMemory
 *
 *  CREATED: 4/21/95
 *
 *  BY: Vitaly Kruglikov
 *
 *  REVISION HISTORY
 *		9/6/95		vmk		Modified for the Mac emulator.
 *
 *********************************************************************************/
Err TimInit(void)
{
	Err				err = 0;
	TimGlobalsPtr	timGlobalsP = 0;

	// We shouldn't be called more than once without being freed in between
	ErrFatalDisplayIf(GTimGlobalsP, "already initialized");

	//
	// Allocate and initialize the Time Manager globals
	//

	// Allocate the Time Manager globals
	timGlobalsP = (TimGlobalsPtr)MemPtrNew( sizeof(TimGlobalsType) );
	if ( timGlobalsP )
		{
		// Zero-init the time manager globals
		MemSet( timGlobalsP, sizeof(TimGlobalsType), 0 );

		// Link system globals to Time Manager globals
		GTimGlobalsP = (MemPtr)timGlobalsP;
		}
	else
		err = timErrMemory;
	
	return( err );
}


//-------------------------------------------------------------------
// Time Manager API routines
//-------------------------------------------------------------------


/*********************************************************************************
 *  API
 *-------------------------------------------------------------------------------
 *
 *  FUNCTION: TimSetAlarm
 *
 *  DESCRIPTION: Sets an alarm in seconds since 1/1/1904.  this function
 *					is reserved for use by the Alarm Manager.
 *
 *  NOTE: since TimSetAlarm is reserved for Alarm Manager use, it should
 *			not require its own semaphore to assure data integrity because
 *			the Alarm Manager will use a semaphore for this purpose.  This
 *			may be revised as the functionality becomes better defined. vmk
 *
 *			Mask the RTC interrupt.
 *			Set up the new alarm.
 *			Un-mask the RTC interrupt.
 *
 *  PARAMETERS: alarm in seconds since 1/1/1904 or 0 to cancel the current
 *					 alarm.
 *
 *  RETURNS: the old value of alarm seconds.
 *
 *  CREATED: 4/21/95
 *
 *  BY: Vitaly Kruglikov
 *
 *  REVISION HISTORY
 *		4/28/95		vmk		Modified to return the old value of alarm seconds.
 *		9/6/95		vmk		Modified for the Mac emulator.
 *
 *********************************************************************************/
UInt32 TimSetAlarm(UInt32 alarmSeconds)
{
	UInt32			oldAlarmSeconds;
	
	// We should have been initialized by now
	ErrFatalDisplayIf(!GTimGlobalsP, "not initialized");
	
	// Set up the new alarm
	oldAlarmSeconds = PrvSetAlarmLow( alarmSeconds, false/*repeat*/ );	
	
	return( oldAlarmSeconds );
}



/*********************************************************************************
 *  API
 *-------------------------------------------------------------------------------
 *
 *  FUNCTION: TimGetAlarm
 *
 *  DESCRIPTION: Gets the current alarm setting in seconds since 1/1/1904.
 *					This function is reserved for use by the Alarm Manager.
 *
 *  NOTE: since TimSetAlarm is reserved for Alarm Manager use, it should
 *			not require its own semaphore to assure data integrity because
 *			the Alarm Manager will use a semaphore for this purpose.  This
 *			may be revised as the functionality becomes better defined. vmk
 *
 *			Mask the RTC interrupt.
 *			Get the alarm seconds.
 *			Un-mask the RTC interrupt.
 *
 *  PARAMETERS: void.
 *
 *  RETURNS: alarm in seconds since 1/1/1904 or 0 if none.
 *
 *  CREATED: 4/24/95
 *
 *  BY: Vitaly Kruglikov
 *		9/6/95		vmk		Modified for the Mac emulator.
 *
 *********************************************************************************/
UInt32 TimGetAlarm(void)
{
	TimGlobalsPtr	timGlobalsP = (TimGlobalsPtr)GTimGlobalsP;
	UInt32				alarmSeconds;


	// We should have been initialized by now
	ErrFatalDisplayIf(!timGlobalsP, "not initialized");

	// Mask the RTC interrupt to protect integrity of our data
	
	// Get the alarm seconds
	alarmSeconds = timGlobalsP->alarmSeconds;	

	// Un-mask the RTC interrupt	
	
	return( alarmSeconds );
}


/************************************************************
 *
 *  FUNCTION: TimGetSeconds
 *
 *  DESCRIPTION: Returns Seconds  since 1/1/04
 *
 *  PARAMETERS: void
 *
 *  RETURNS: UInt32 seconds
 *
 *  CREATED: 3/15/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt32 TimGetSeconds(void)
{
	return StubTimGetSeconds();
}


/************************************************************
 *
 *  FUNCTION: TimSetSeconds
 *
 *  DESCRIPTION: Sets Seconds  since 1/1/04
 *
 *  PARAMETERS: void
 *
 *  RETURNS: UInt32 seconds
 *
 *  CREATED: 3/15/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
void TimSetSeconds(UInt32 seconds)
{
	UInt32		result;
	UInt32		delta;
	
	// before we set the time, remember the amount that we're changing it by 
	delta = (Int32)(seconds - TimGetSeconds());
	
	StubTimSetSeconds(seconds);
	
	// Cause the Alarm Manager to resynchronize
	AlmAlarmCallback();
	
	// <chg 4-3-98 RM> Notify Alarm Manager so that it can notify
	//  procedure alarms
	AlmTimeChange();

	//
	// Notify all apps
	//
	PrvBroadcastTimeChange(delta);
	
	//SysBroadcastActionCode( sysAppLaunchCmdTimeChange, 0/*cmdPBP*/ );
	
	// Fake notification to the current emulator app
	SysAppLaunch( 0/*cardNo*/, 0/*dbID*/, 0/*launchFlags*/,
							sysAppLaunchCmdTimeChange, 0/*cmdPBP*/, &result );
							
	
}


/************************************************************
 *
 *  FUNCTION: TimGetTicks
 *
 *  DESCRIPTION: Returns Tick count
 *
 *  PARAMETERS: void
 *
 *  RETURNS: UInt32 Tick count
 *
 *  CREATED: 3/15/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt32 TimGetTicks(void)
{
	return StubTimGetTicks();
}


//-------------------------------------------------------------------
// Interrupt Service Routines
//-------------------------------------------------------------------



/*********************************************************************************
 * ISR
 * MC68328(Dragon Ball) hardware-dependent
 *-------------------------------------------------------------------------------
 *
 * FUNCTION: TimHandleInterrupt
 *
 * DESCRIPTION: Handles the RTC Day IRQ, Alarm IRQ, and periodic calls
 *					to save time in non-volatile RAM.
 *
 *				This routine is called when the RTC 24 hour clock increments
 *				past midnight or when the Alarm0 register matches the
 *				the time of the RTC's 24 hour clock.
 *
 *				It is also called periodically by the System Timer task in order
 *				to give the Time Manager a chance to save it's current
 *				time in non-volatile storage. This allows us to keep the
 *				current time relatively accurate in case the DragonBall is reset.
 *				When called for this function, periodicUpdate will be true
 *				and interrupts will be ENABLED!!
 *
 * ASSUMPTIONS: RTC level and below interrupts are disabled if periodicUpdate is false.
 *
 *		If 24 hour interrupt:
 *			Clear the Day IRQ bit in the RTC Status register.
 *			If there is an alarm setting, update its status, programming
 *			Alarm0 if the alarm is less than 24 hours away.
 *
 *		If Alarm interrupt:
 *			Clear the Alarm0 IRQ bit in the RTC Status register to clear
 *			the interrupt.
 *			Call the Alarm Manager's callback function.
 *			Set the alarm to repeat shortly to make sure the
 *			Alarm Manager picks it up, at which time it will clear
 *			the alarm, ending the alarm cycle.
 *
 * PARAMETERS: 
 *		periodicUpdate - true when called from System Timer task.
 *
 * RETURNS: void
 *
 * CREATED: 4/24/95
 *
 * BY: Vitaly Kruglikov
 *
 *********************************************************************************/
void TimHandleInterrupt(Boolean /*periodicUpdate*/)
{
	TimGlobalsPtr		gP = (TimGlobalsPtr)GTimGlobalsP;
	
	// Simulate timer hardware
	if ( gP->alarmSeconds && gP->alarmSeconds <= TimGetSeconds() )
		{
		AlmAlarmCallback();						// notify Alarm Manager
		
		// Set the alarm to repeat shortly
		PrvSetAlarmLow( gP->alarmSeconds, true/*repeat*/ );
		}
		
}

//-------------------------------------------------------------------
// Private routines used by this module only
//-------------------------------------------------------------------



/*********************************************************************************
 *  PRIVATE,
 *-------------------------------------------------------------------------------
 *  FUNCTION: PrvSetAlarmLow
 *
 *  DESCRIPTION:	Low-level routine to set an alarm in seconds since 1/1/1904.
 *
 *						May be called at interrupt time.
 *
 *  ASSUMPTIONS:	The A5 world is set up.
 *
 *  CALLED BY: TimSetAlarm, PrvHandleInterrupt
 *
 *			Cancel the current alarm (if any).
 *			Save the new alarm setting in Time Manager globals.
 *			If the new setting is 0, we're done, otherwise...
 *
 *  PARAMETERS: alarm in seconds since 1/1/1904 or 0 to cancel the current
 *					 alarm.
 *
 *  RETURNS: the old value of alarm seconds.
 *
 *  CREATED: 4/24/95
 *
 *  BY: Vitaly Kruglikov
 *
 *  REVISION HISTORY
 *		4/28/95		vmk		Modified to return the old value of alarm seconds.
 *		9/6/95		vmk		Modified for the Mac emulator.
 *
 *********************************************************************************/
static UInt32
PrvSetAlarmLow(UInt32 alarmSeconds, Boolean repeat)
{
	TimGlobalsPtr	timGlobalsP = (TimGlobalsPtr)GTimGlobalsP;
	UInt32				oldAlarmSeconds;
	UInt32				curSeconds;			// Current date/time in seconds


	// We should have been initialized by now
	ErrFatalDisplayIf(!timGlobalsP, "not initialized");
	
	// Save the old alarm seconds for return
	oldAlarmSeconds = timGlobalsP->alarmSeconds;

	//
	// Save the new alarm time time and check if the RTC Alarm needs to be programmed
	//
	
	// Save the new alarm seconds in Time Manager Globals
	timGlobalsP->alarmSeconds = alarmSeconds;
	

	// If we have a new alarm value, check if the RTC Alarm needs to be programmed now
	if ( alarmSeconds )
		{
		curSeconds = TimGetSeconds();			// get current date/time in seconds

		// If the alarm time is now or in the past, notify the alarm manager
		// immediately
		if ( alarmSeconds < (curSeconds + timAlarmMinDeltaSeconds) && !repeat)
			{
			AlmAlarmCallback();
			//repeat = true;
			}
		}
	
	return( oldAlarmSeconds );
}

/*********************************************************************************
 *  PRIVATE,
 *-------------------------------------------------------------------------------
 *  FUNCTION: PrvBroadcastTimeChange
 *
 *  DESCRIPTION:	Broadcasts a time change event via the NotifyMgr.
 *
 *
 *  PARAMETERS: time change delta in seconds
 *
 *  RETURNS: none
 *
 *  REVISION HISTORY: 
 *	 jesse		7/13/98	Initial revision
 *  jesse		9/10/99	Use a deferred broadcast instead of an immediate one.
 *  
 *
 *********************************************************************************/
void PrvBroadcastTimeChange(UInt32 delta)
{
	SysNotifyParamType	notify;
	
	notify.notifyType = sysNotifyTimeChangeEvent;
	notify.broadcaster = sysNotifyBroadcasterCode;
	notify.handled = false;
	notify.notifyDetailsP = &delta;
	
	SysNotifyBroadcastDeferred(&notify, sizeof(delta));
	
	return;
}


#endif // EMULATION_LEVEL != EMULATION_NONE
