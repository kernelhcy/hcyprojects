/******************************************************************************
 *
 * Copyright (c) 1995-1999 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: AlarmMgr.c
 *
 * Release: 
 *
 * Description:
 *		This file contains the Alarm Manager routines for Pilot.
 *
 *		The Alarm Manager works closely with the Time Manager to handle real time alarms such
 *		as those set by the DateBook for meeting reminders.  It provides the support for applications
 *		that need to set alarms for performing some periodic activity or to display a reminder dialog
 *		box.  The Alarm Manager does not provide any UI to display the reminder dialog boxes,
 *		but, instead, leaves this and the alarm sound up to the individual applications by sending
 *		the "display alarm" action code.
 *
 *		The Alarm Manager manages alarms by application, and can only queue up one untriggered
 *		alarm for a given application.  When the alarm "goes off", the application is notified
 *		via the "alarm triggered" action code, at which time it can set the next alarm and/or
 *		perform some maintenance activity.
 *		Triggered alarms are queued up until the "display alarm" action code can be sent to the
 *		creator.  However, if the alarm table becomes full, the oldest alarm entry in the alarm table
 *		which has been both triggered and notified will be deleted to make room for a new alarm.
 *		
 *
 *		The Alarm Manager orders all pending alarms and programs the Time Manager (via TimSetAlarm())
 *		to generate an interrupt for the alarm that should go off the soonest.  When this interrupt
 *		occurs, Time Manager calls the Alarm Manager's AlmAlarmCallback() function which
 *		notes the interrupt by setting the "triggered" flag in the Alarm Globals and calling
 *		EvtWakeup() to wake up the Event Manager in case it was asleep.  The Event Manager
 *		calls AlmDisplayAlarm() between events.  AlmDisplayAlarm() checks the "triggered" flag
 *		and returns immediately if no alarm had been triggered.  If an alarm had been triggered,
 *		AlmDisplayAlarm() notifies all applications which set an alarm for that alarm time,
 *		and then calls each in turn to display its alarm.
 *
 *		If a new alarm time is triggered while an older alarm is being displayed, all apps with
 *		alarms scheduled for that time are notified, but the display cycle is postponed until all
 *		earlier alarms are finished displaying.  If, among the newly triggered alarms, there is
 *		at least one with the "quiet" flag NOT set, the alarm manager will sound
 *
 * History:
 *		Apr 11, 1995	Created by Vitaly Kruglikov
 *		5/9/95		vmk		Added documentation to module header.
 *		4/1/98		RM			Added support for procedure alarms. 
 *    4/3/98 		RM 		Added resource semaphore to grab ownership of Alarm
 *										globals so that it's multi-tasking friendly.
 *
 *****************************************************************************/

/* Routines:
 *		AlmInit
 *    AlmSetAlarm
 *		AlmGetAlarm
 *		AlmCancelAll
 *		AlmDisplayAlarm
 *		AlmAlarmCallback
 */

#define	NON_PORTABLE

#include <PalmTypes.h>

#include <Chars.h>
#include <AlarmMgr.h>
#include <ErrorMgr.h>
#include <SysEvtMgr.h>					// included for EvtWakeup(void)
#include <SystemMgr.h>
#include <MemoryMgr.h>
#include <TimeMgr.h>

#include "Globals.h"
#include "AlarmPrv.h"
#include "TimePrv.h"
#include "AttentionPrv.h"


//-------------------------------------------------------------------
//		Private routines used only by this module
//-------------------------------------------------------------------
static void		PrvDeleteEntry(Int16 entryIndex, Boolean okToShrink);
static Err		PrvInsertEntry(AlmEntryPtr entryP);
static Err		PrvEnsureFreeEntry(UInt32 beforeDate);
static Int16	PrvFindUntriggeredAlarm(UInt16 cardNo, LocalID dbID);
static void		PrvSetNextAlarm(void);
static Boolean	PrvDisplayNextAlarm(void);


//-------------------------------------------------------------------
//		Macros used only by this module
//-------------------------------------------------------------------

// Macro: almTableBytes --	computes the size of the alarm table
// 								in bytes, given the number of alarm entries.
#define almTableBytes(length)													\
	((UInt32)&((AlmTablePtr)0)->list + (sizeof(AlmEntryType) * (length)))




//-------------------------------------------------------------------
//		Alarm Manager Initialization routines
//-------------------------------------------------------------------

/************************************************************
 * API - INITIALIZATION
 *-----------------------------------------------------------
 *
 *  FUNCTION: AlmInit
 *
 *  DESCRIPTION: Initializes the Alarm Manager.
 *
 *			Allocate and initialize the Alarm Manager Globals.
 *			Allocate and initialize an alarm table of the default size.
 *
 *  PARAMETERS: void
 *
 *  RETURNS: 0 if successful; otherwise { almErrMemory }
 *
 *  CREATED: 4/27/95
 *
 *  BY: Vitaly Kruglikov
 *
 *************************************************************/
Err
AlmInit(void)
{
	Err				err = 0;
	AlmGlobalsPtr	almGlobalsP = 0;
	MemHandle		almTableH = 0;
	AlmTablePtr		almTableP = 0;
	UInt32				tag;


	// We shouldn't be called more than once without being freed in between
	ErrFatalDisplayIf(GAlmGlobalsP, "already initialized");


	//
	// Allocate and initialize the Alarm Manager data structures
	//
	
	// Allocate and initialize the alarm table
	almTableH = MemHandleNew( almTableBytes(almMinTableLength) );
	if ( !almTableH ) goto ErrorMem;
	
	// Lock the alarm table
	almTableP = (AlmTablePtr)MemHandleLock( almTableH );
	
	MemSet( almTableP, sizeof(AlmTableType), 0 );	// initialize
	almTableP->numEntries = almMinTableLength;

	// Unlock the alarm table
	MemPtrUnlock( almTableP );

	// Allocate and initialize the Alarm Manager Globals
	almGlobalsP = (AlmGlobalsPtr)MemPtrNew( sizeof(AlmGlobalsType) );
	if ( !almGlobalsP ) goto ErrorMem;
	MemSet( almGlobalsP, sizeof(AlmGlobalsType), 0 );	// initialize
	almGlobalsP->tableH = almTableH;
	
	
	// <chg 4-3-98 RM> Create a resource semaphore for ownership of
	//  the Alarm globals
	tag = 'AlmM';
	// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
	ErrFatalDisplayIf(err, "");

	// Link system globals to Alarm Manager globals
	GAlmGlobalsP = (MemPtr)almGlobalsP;


	return( err );


ErrorMem:
	// Free anything allocated here
	if ( almGlobalsP )
		MemPtrFree( almGlobalsP );

	if ( almTableH )
		MemHandleFree( almTableH );

	return( almErrMemory );
}


//-------------------------------------------------------------------
//		Alarm Manager API routines
//-------------------------------------------------------------------



/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION: AlmSetAlarm
 *
 * DESCRIPTION: Sets an alarm for the given application.  If an alarm for this
 *               application has already been set, it will be replaced with the
 *               new alarm.  A callback procedure will be called after
 *               the alarm has been triggered and can be used by the
 *               application to set the next alarm.
 *
 *			If there is an un-triggered alarm owned by the application, free it.
 *			If alarmSeconds is 0, we're done.
 *			Insert the new alarm into the alarm table ordered by alarm seconds.
 *			Call the time manager to set the next un-triggred alarm.
 *
 *
 * PARAMETERS:
 *		cardNo			-- creator app card number. For procedure alarms this
 *								is set to almProcAlarmCardNo. 
 *		dbID				-- creator app database ID OR, if cardNo is almProcAlarmCardNo,
 *								then this is the procedure pointer to call. 
 *		ref				-- alarm reference number (generated by caller);
 *		alarmSeconds	-- alarm date/time in seconds since 1/1/1904; or 0 to
 *							   cancel the current alarm (if any);
 *		quiet				-- non-zero to suppress sound of alarm -
 *							   the caller only wants notification;
 *
 * RETURNS: 0 if successful; otherwise { almErrMemory, almErrFull }
 *
 * CREATED: 4/27/95
 *
 * BY: Vitaly Kruglikov
 *
 *************************************************************/
Err
AlmSetAlarm(UInt16 cardNo, LocalID dbID, UInt32 ref, UInt32 alarmSeconds, Boolean quiet)
{
	Err				err = 0;
	Int16				alarmIndex;
	AlmEntryType	entry;
	AlmGlobalsPtr	almGlobalsP = (AlmGlobalsPtr)GAlmGlobalsP;

	
#if EMULATION_LEVEL == EMULATION_NONE
	// Check for invalid dbIDs
	// This check is not done for the simulator because simulator apps do not
	// have a separate application database and therefore always pass in null
	// dbIDs.  But thats okay, because PrvFindUntriggeredAlarm will return an
	// alarmIndex of -1 and then PrvDeleteEntry is not called and all is well.
	// <3/4/99 SCL> Changed to Fatal since setting an invalid alarm is bad. We
	// can crash later, or crash now when the user is more likely to notice.
	ErrFatalDisplayIf(!dbID, "Invalid dbID");
#endif

	// We should have been initialized by now
	ErrFatalDisplayIf(!almGlobalsP, "not initialized");
	
	
	// Grab ownership of our semaphore
	// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
	
	//
	// First, free the current untriggered alarm belonging to this application
	// (if any)
	//
	
	// Find and delete the alarm entry
	alarmIndex = PrvFindUntriggeredAlarm( cardNo, dbID );
	if ( alarmIndex >= 0 )
		PrvDeleteEntry( alarmIndex, true/*okToShrink*/ );
	
	
	//
	// Insert the alarm into the alarm table
	//
	if ( alarmSeconds )
		{
		entry.ref = ref;
		entry.alarmSeconds = alarmSeconds;
		entry.dbID = dbID;
		entry.cardNo = cardNo;
		if ( quiet )
			entry.quiet = 1;
		else
			entry.quiet = 0;
		entry.triggered = 0;
		entry.notified = 0;
		err = PrvInsertEntry( &entry );
		}
	
	
	//
	// Set the first un-triggered alarm to be triggered next by the Time
	// Manager
	//
	PrvSetNextAlarm();

	
	// Release our semaphore
	// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
	
	return( err );
}



/************************************************************
 * API
 *-----------------------------------------------------------
 *
 *  FUNCTION: AlmGetAlarm
 *
 *  DESCRIPTION: Returns the alarm date/time in seconds since 1/1/1904 and
 *					the alarm reference number for the given application.
 *
 *  PARAMETERS:
 *		cardNo			-- creator app card number;
 *		dbID				-- creator app database ID;
 *		refP				-- MemPtr to location for the alarm's reference number;
 *
 *  RETURNS: alarm seconds since 1/1/1904 and alarm reference number;
 *				if no alarm is set for the application, 0 is returned for the
 *				alarm seconds and the reference number is undefined.
 *
 *  CREATED: 4/11/95
 *
 *  BY: Vitaly Kruglikov
 *
 *************************************************************/
UInt32 AlmGetAlarm(UInt16 cardNo, LocalID dbID, UInt32 * refP)
{
	AlmGlobalsPtr	almGlobalsP = (AlmGlobalsPtr)GAlmGlobalsP;
	UInt32				alarmSeconds = 0;
	Int16				alarmIndex;
	

	// We should have been initialized by now
	ErrFatalDisplayIf(!almGlobalsP, "not initialized");
	
	
	// Grab ownership of our semaphore
	// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
	

	// Search the alarm list for an un-triggered alarm owned by this application
	alarmIndex = PrvFindUntriggeredAlarm( cardNo, dbID );
	if ( alarmIndex >= 0 )
		{
		AlmTablePtr		tableP;
		tableP = (AlmTablePtr)MemHandleLock( almGlobalsP->tableH );
		alarmSeconds = tableP->list[alarmIndex].alarmSeconds;
		*refP = tableP->list[alarmIndex].ref;
		MemPtrUnlock( tableP );
		}
	
	// Release our semaphore
	// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS

	return( alarmSeconds );
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION:	AlmEnableNotification
 *
 * DESCRIPTION:	Enables/disables Alarm Manager's notifications.
 *
 * PARAMETERS:	enable			-- if true, notifications are enabled; otherwise,
 *										   notifications are disabled;
 *
 * RETURNS:		nothing.
 *
 * CREATED: 10/9/95
 *
 * BY: Vitaly Kruglikov
 *
 *************************************************************/
void AlmEnableNotification(Boolean enable)
{
	AlmGlobalsPtr	almGlobalsP = (AlmGlobalsPtr)GAlmGlobalsP;

	// We should have been initialized by now
	ErrFatalDisplayIf(!almGlobalsP, "not initialized");
	
	// Grab ownership of our semaphore
	// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
	
	if ( enable )
		{
		if ( almGlobalsP->disableCount )
			{
			almGlobalsP->disableCount--;
			
			// If the notification is now enabled, ensure that any alarms
			// which were triggered while notification was disabled will
			// be processed
			if ( almGlobalsP->disableCount == 0 )
				AlmAlarmCallback();
			}
		else
			ErrDisplay( "disable underflow" );
		}
	else
		{
		almGlobalsP->disableCount++;
		TimSetAlarm( 0 );						// cancel current Time Manager alarm, if any;
		almGlobalsP->triggered = 0;		// in case of race condition with alarm ISP
		ErrFatalDisplayIf( !almGlobalsP->disableCount, "disable overflow" );
		}

	// Release our semaphore
	// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
	
	AttnEnableNotification(enable);
}



/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION: AlmCancelAll
 *
 * DESCRIPTION: Cancels all alarms managed by the Alarm Manager.  This
 *					function is presently called by the Time Manager to cancel
 *					all alarms when the user changes date/time.
 *
 *	Delete all alarm table entries.
 *	Call Time Manager to cancel the current alarm.
 *	Reset the Alarm Globals' triggered flag.
 *
 * PARAMETERS: void
 *
 * RETURNS: void
 *
 * CREATED: 5/1/95
 *
 * BY: Vitaly Kruglikov
 *
 * REVISION HISTORY
 *		5/15/95		vmk		Removed re-dereferencing of locked chunk after shrinking it
 *		4/1/98		RM			Added call to all procedure alarms to allow them to
 *										reschedule. 
 *
 *************************************************************/
void AlmCancelAll(void)
{
	AlmGlobalsPtr	almGlobalsP = (AlmGlobalsPtr)GAlmGlobalsP;
	AlmTablePtr		tableP;
	

	// We should have been initialized by now
	ErrFatalDisplayIf(!GAlmGlobalsP, "not initialized");
	
	// Grab ownership of our semaphore
	// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
	
	//
	// Delete all alarm table entries.
	//
	tableP = (AlmTablePtr)MemHandleLock( almGlobalsP->tableH );
	while ( tableP->list[0].alarmSeconds )
		{
		PrvDeleteEntry( 0, true/*okToShrink*/ );
		// NO NEED TO RE-DEREF WHEN SHRINKING A LOCKED CHUNK - MEMORY MANAGER GUARANTEES IT
		// WILL NOT MOVE	vmk	5/15/95
		//tableP = MemDeRef( almGlobalsP->tableH );	// recover alarm table MemPtr
		}
	MemPtrUnlock( tableP );
	
	
	// Call Time Manager to cancel the current alarm.
	TimSetAlarm( 0 );
	
	// Reset the Alarm Globals' "alarm triggered" flag
	almGlobalsP->triggered = 0;

	// Release our semaphore
	// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
	
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION: AlmTimeChange
 *
 * DESCRIPTION: Gets called by TimSetSeconds() and gives the
 *		Alarm Manager a chance to reschedule alarms. 
 *
 *		Note that this call does nothing for application alarms. They
 *		are rescheduled as a result of a broadcast action code sent
 *		to all apps by TimSetSeconds(). 
 *
 *		However, for procedure alarms, it calls the procedure
 *		to give it a chance to reschedule itself. 
 *
 * PARAMETERS: void
 *
 * RETURNS: void
 *
 * CREATED: 4/1/98
 *
 * BY: Ron Marianetti
 *
 *************************************************************/
void AlmTimeChange(void)
{
	AlmGlobalsPtr	almGlobalsP = (AlmGlobalsPtr)GAlmGlobalsP;
	AlmTablePtr		tableP;
	Int16				entryIndex;
	MemHandle	tableH;
	AlmAlarmProcPtr procP;
	AlmTablePtr		tmpTableP = 0;
	Int32				tmpSize;
	Boolean			oneRemoved = false;
	SysAlarmTriggeredParamType	triggeredParam;
	AlmEntryType*	entryP;
	Err				err=0;

	// We should have been initialized by now
	ErrFatalDisplayIf(!almGlobalsP, "not initialized");

	// Grab ownership of our semaphore
	// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
	

	//-------------------------------------------------------------
	// Make a copy of the alarm table to walk through the
	//  alarms
	//-------------------------------------------------------------
	tableH = almGlobalsP->tableH;			// save table handle (optimization)
	tmpSize = MemHandleSize(tableH);
	
	// Allocate it
	tmpTableP = MemPtrNew(tmpSize);
	if (!tmpTableP) {
		err = memErrNotEnoughSpace;
		goto Exit;
		}
	
	// Copy it into our temp table
	tableP = (AlmTablePtr)MemHandleLock( tableH );
	MemMove(tmpTableP, tableP, tmpSize);
	

	//-------------------------------------------------------------
	// Search for a procedure alarms and remove them starting from the end
	// PrvDeleteEntry moves the given entry to the end and gives it
	//  an alarmSeconds value of 0 to mark it as deleted. 
	//-------------------------------------------------------------
	for ( entryIndex=tableP->numEntries-1; entryIndex >= 0; entryIndex-- ) {
	
		if (tableP->list[entryIndex].cardNo == almProcAlarmCardNo 
		    		&& tableP->list[entryIndex].alarmSeconds)
		    PrvDeleteEntry( entryIndex, false /* okToShrink */);
		}
		

	// Unlock the alarm table
	MemPtrUnlock( tableP );
	
	
	//-------------------------------------------------------------
	// Now, go through and send time change notifications to all the
	//  procedure alarms and give them a chance to re-install themselves
	//-------------------------------------------------------------
	entryP = &tmpTableP->list[0];
	for ( entryIndex=0; entryIndex < tmpTableP->numEntries; entryIndex++, entryP++ ) {
		
		if (entryP->cardNo == almProcAlarmCardNo  && entryP->alarmSeconds) {
		    
			// Send Time Change notification to this proc alarm
			triggeredParam.ref = entryP->ref;
			triggeredParam.alarmSeconds = entryP->alarmSeconds;
			triggeredParam.purgeAlarm = true;

			procP = (AlmAlarmProcPtr)entryP->dbID;
			if (procP) 
				(*procP)(almProcCmdReschedule, &triggeredParam);
			}
		}
		


Exit:
	ErrNonFatalDisplayIf(err, "");
	
	// Release our temp memory
	if (tmpTableP) MemPtrFree(tmpTableP);
	
	// Release our semaphore
	// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
	
	return;	
}



/************************************************************
 * API
 *-----------------------------------------------------------
 *
 *  FUNCTION: AlmDisplayAlarm
 *
 *  DESCRIPTION: Displays any alarms that have gone off.
 *
 * This function is called by the Event Manager executing on some app's
 * thread.  This permits us to access resources and execute system calls
 * which would not be possible at interrupt time.
 *
 * Check the triggered flag in the Alarm Globals(optimization); if no alarm
 * triggered since last call, we're done.
 *
 * Clear the Alarm Manager Globals triggered flag.
 *
 * Call the "alarm triggered notification" action code for each affected
 * alarm entry (we will need to keep a flag with each alarm entry
 * indicating whether this call has been made).
 * We should probably pass the alarmSeconds/ref# to the app to assist it
 * in determining the next alarm to set.
 *
 * Set the first un-triggered alarm to be triggered next by the Time Manager.
 *
 * For each triggered alarm, beginning with the oldest:
 *		Call the creator application to display the alarm.
 *
 *  PARAMETERS:	displayOnly		-- if true, we are called to send display
 *											   notifications only; otherwise, we are called
 *											   for trigger notifications only
 *
 *  RETURNS: Boolean indicates an app needs to display.
 *
 *  NOTE: See EvtResetAutoOffTimer() code in SysEvtMgr.c for a lengthy
 *				description of how alarms interact with LCDWake and AutoOff.
 *
 *  CREATED: 4/28/95
 *
 *  BY: Vitaly Kruglikov
 *
 * REVISION HISTORY
 *		10/19/95		vmk		Added displayOnly parameter and cleaned up
 *		4/1/98		RM			Added support for procedure alarms.
 *		12/8/98		jb			Added return code to indicate an app needs to display.
 *									This was added for checks on power down to avoid missing
 *									an alarm until the next time the device is turned on.
 *
 *************************************************************/
Boolean AlmDisplayAlarm(Boolean displayOnly)
{
	AlmGlobalsPtr	almGlobalsP = (AlmGlobalsPtr)GAlmGlobalsP;
	AlmTablePtr		tableP;
	UInt32				curAlarmSeconds;
	Int16				entryIndex;
	MemHandle	tableH;
	Boolean			someoneNotified;
	Boolean			needDisplay = false;
	AlmAlarmProcPtr	procP;

	// We should have been initialized by now
	ErrFatalDisplayIf(!almGlobalsP, "not initialized");

	// Grab ownership of our semaphore
	// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
	
	// If there is nothing for us to do, bail out
	if ( (!almGlobalsP->triggered && !displayOnly) || almGlobalsP->disableCount )
		goto Exit;;
		
	// Check if we were called for display notifications only
	if ( displayOnly )
		{
		// Display all triggered alarms, beginning with the oldest
		do {} while ( PrvDisplayNextAlarm() );
		goto Exit;;
		}
	
	//
	// We're here because there was alarm activity
	//
	
	// Cancel the current Time Manager alarm and get the current alarm time
	TimSetAlarm( 0 );
	curAlarmSeconds = TimGetSeconds() + timAlarmMinDeltaSeconds;

	// Reset the alarm triggered flag so we won't have to go through all this
	// every time we're called until the next alarm is triggered(optimization).
	almGlobalsP->triggered = 0;
	
	
	tableH = almGlobalsP->tableH;			// save table handle (optimization)
	
	//
	// Mark the alarms which are <= the current alarm seconds
	// as triggered.
	//

	tableP = (AlmTablePtr)MemHandleLock( tableH );
	for ( entryIndex=0; entryIndex < tableP->numEntries &&
			tableP->list[entryIndex].alarmSeconds &&
			tableP->list[entryIndex].alarmSeconds <= curAlarmSeconds;
			entryIndex++ )
		{
		tableP->list[entryIndex].triggered = 1;
		}
	MemPtrUnlock( tableP );
	
	
	//
	// For each alarm entry which has been triggered, but not notified,
	// call the app with the "alarm triggered notification" action code.
	// We will mark each such entry as "notified" before calling
	// the action code to allow the entry to be replaced in case the
	// alarm table is full.  Since a typical response to the action code
	// would be to set the next alarm, thus causing new entries to be
	// inserted into the table, we will make several passes
	// over the alarm table to catch all entries needing notification.
	//
	do	{
		
		someoneNotified = false;						// this flag is used to terminate the outside
																// loop when there is no-one left to notify

		// Send notifications for all newly triggered alarms
		for ( entryIndex=0; true; entryIndex++ )
			{
			AlmEntryType	entry;
			Int16		numEntries;
			
			// If we're out of bounds of used entries, break out of the inner loop
			tableP = (AlmTablePtr)MemHandleLock( tableH );
			numEntries = tableP->numEntries;
			if ( entryIndex < numEntries )			// save the current entry
				entry = tableP->list[entryIndex];
			MemPtrUnlock( tableP );
			if ( entryIndex >= numEntries || !entry.alarmSeconds )
				break;										// leave the inner loop
			

			// If the alarm has been triggered, but not notified, call
			// the notification action code
			if ( entry.triggered )
				{
				if ( entry.notified )
					needDisplay = true;
				else
					{
					SysAlarmTriggeredParamType	triggeredParam;
					UInt32				appLaunchResult;
									
					someoneNotified = true;		// set flag to force another pass
					
					// Remove the entry from the table
					PrvDeleteEntry( entryIndex, true/*okToShrink*/ );
					
					// Setup the parameter block to pass
					triggeredParam.ref = entry.ref;
					triggeredParam.alarmSeconds = entry.alarmSeconds;
					triggeredParam.purgeAlarm = false;


					//---------------------------------------------------------------
					// <chg 4-1-98 RM>
					// Take the appropriate action for procedure vs. app alarms
					// For procedure alarms, just call the alarm proc and make sure
					//  the alarm gets purged since procedure alarms DON'T ever
					//  get the equivalent of a display action code
					//---------------------------------------------------------------
					if (entry.cardNo == almProcAlarmCardNo) {
						procP = (AlmAlarmProcPtr)entry.dbID;
						if (procP) 
							(*procP)(almProcCmdTriggered, &triggeredParam);
						triggeredParam.purgeAlarm = true;
						}
					
					// For app alarms, reset the auto-off timer and send an action code
					else {
						// <chg 4-1-98 RM> Reset the auto-off timer. This used to be
						// done by AlmAlarmCallback() but now we do it here ONLY if
						//  we're triggering an application alarm. 
						EvtResetAutoOffTimer();

						// Notify the creator app (can cause the alarm table to move)
						SysAppLaunch( entry.cardNo, entry.dbID, 0/*launchFlags*/, sysAppLaunchCmdAlarmTriggered,
								(MemPtr)(&triggeredParam), &appLaunchResult );
						}
					
					// If the user does not wish to purge the alarm, add it back
					// into the table (if possible)
					if ( !triggeredParam.purgeAlarm ) {
						entry.notified = 1;
						PrvInsertEntry( &entry );
						needDisplay = true;
						}
					} // If the alarm has not been notified

				} // If the alarm has been triggered
				
			} // Send notifications for all newly triggered alarms (inner loop)
	
		}
	while ( someoneNotified );	// outer loop


	// Set the first un-triggered alarm to be triggered next by the Time Manager
	PrvSetNextAlarm();
	
	
	// If we have some alarms that need displaying -- enqueue the virtual
	// alarm character to trigger display of alarms.  This allows any
	// other system dialogs to be dismissed before an alarm dialog is
	// displayed (for dynamic heap efficiency).
	if ( needDisplay )
		{
		// If queue is full at this time, set the 'triggered' flag
		// to ensure we have another chance
		if ( EvtEnqueueKey(alarmChr, 0, commandKeyMask) )
			almGlobalsP->triggered = 1;
		}
	
Exit:

	// Release our semaphore
	// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS

	return ( needDisplay );	
}



//-------------------------------------------------------------------
//		Routines called at interrupt time
//-------------------------------------------------------------------



/************************************************************
 * INTERRUPT-TIME CALLBACK
 *-----------------------------------------------------------
 *
 *  FUNCTION: AlmAlarmCallback
 *
 *  ASSUMPTIONS:	the A5 world is set-up
 *
 *  DESCRIPTION: This function is called at interrupt time by the Time
 *					Manager when an alarm goes off.
 *
 *			Set the alarmTriggered flag in the Alarm Globals such that it
 *			could be checked by AlmDisplayAlarm().
 *
 *			Unsuspend the current UI thread.
 *
 *  CALLED BY: Time Manager, AlmEnableNotification
 *
 *  PARAMETERS: void
 *
 *  RETURNS: void
 *
 *  CREATED: 4/28/95
 *
 *  BY: Vitaly Kruglikov
 *
 *************************************************************/
void
AlmAlarmCallback(void)
{
	AlmGlobalsPtr	almGlobalsP = (AlmGlobalsPtr)GAlmGlobalsP;
	

	// We should have been initialized by now
	ErrFatalDisplayIf(!almGlobalsP, "not initialized");


	// Set the alarm triggered flag in the Alarm Globals (optimization).
	// This flag will be checked in AlmDisplayAlarm().
	almGlobalsP->triggered = 1;
	
	// Reset the auto-off timer - or else we might go right back to sleep again
	// <chg 4-1-98 RM> Moved reset of auto-off timer into AlmDisplayAlarm
	//  if triggering an application alarm. 
	// EvtResetAutoOffTimer();
	
	// Wake up the current UI thread
	EvtWakeup();
}




//-------------------------------------------------------------------
//		Private routines used only by this module
//-------------------------------------------------------------------



/*********************************************************************************
 * PRIVATE
 *-------------------------------------------------------------------------------
 *
 * FUNCTION: PrvDeleteEntry
 *
 * DESCRIPTION: Deletes an entry from the alarm table given the entry index.
 *
 *		Compact the alarm table to eliminate the entry being deleted.
 *		If table is larger than the default, shrink it by one entry.
 *
 * CALLED BY: AlmCancelAll, PrvEnsureFreeEntry, AlmDisplayAlarm,
 *					AlmSetAlarm
 *
 * PARAMETERS:
 *		entryIndex		-- alarm table entry index (0-based);
 *		okToShrink		-- non-zero if it is ok to shrink the table after
 *							   the entries are compacted;
 *
 * RETURNS: void
 *
 * CREATED: 4/27/95
 *
 * BY: Vitaly Kruglikov
 *
 *********************************************************************************/
static void
PrvDeleteEntry(Int16 entryIndex, Boolean okToShrink)
{
	AlmGlobalsPtr	almGlobalsP = (AlmGlobalsPtr)GAlmGlobalsP;
	AlmTablePtr		tableP;
	Int16				lastUsedIndex;			// last used entry index	
	
	// Lock the alarm table
	tableP = (AlmTablePtr)MemHandleLock( almGlobalsP->tableH );
	

	// Validate the entry index against table size
	ErrFatalDisplayIf( (entryIndex < 0 || entryIndex >= tableP->numEntries),
			"out of bounds" );
		

	// Find the last used entry in the table after the entry to be deleted
	lastUsedIndex = tableP->numEntries - 1;
	for ( ; lastUsedIndex > entryIndex &&
			!(tableP->list[lastUsedIndex].alarmSeconds); lastUsedIndex-- )
		{ /* LEFT INTENTIONALLY BLANK */ }


	// Compact the entries and mark the old last used entry as unused
	if ( entryIndex != lastUsedIndex )
		{
		MemMove( &(tableP->list[entryIndex]),
				&(tableP->list[entryIndex+1]),
				sizeof(AlmEntryType) * (lastUsedIndex - entryIndex) );
		}
	tableP->list[lastUsedIndex].alarmSeconds = 0;	// mark old last used entry as unused
	
	
	// Shrink the alarm table by one entry if it exceeds the default size.
	// *Memory Manager will not move the chunk when shrinking it*
	if ( tableP->numEntries > almMinTableLength && okToShrink )
		{
		tableP->numEntries--;				// decrement table size
		MemPtrResize( tableP, almTableBytes(tableP->numEntries) );
		}


	// Unlock the alarm table
	MemHandleUnlock( almGlobalsP->tableH );

}



/*********************************************************************************
 * PRIVATE
 *-------------------------------------------------------------------------------
 *
 * FUNCTION: PrvInsertEntry
 *
 * DESCRIPTION: Inserts an alarm entry into its sorted position in the
 *					alarm table.
 *
 * ASSUMPTIONS: Alarm table is NOT locked;
 *
 *		Ensure an unused entry in the alarm table.
 *		Insert the new entry into its sorted position.
 *
 * CALLED BY: AlmSetAlarm, AlmDisplayAlarm
 *
 * PARAMETERS:	entryP		-- MemPtr to alarm entry data
 *
 * RETURNS: 0 if successful; otherwise { almErrMemory, almErrFull }
 *
 * WARNING: can cause the alarm table to move on the dynamic heap,
 *				invalidating stored pointers;
 *
 * CREATED: 4/27/95
 *
 * BY: Vitaly Kruglikov
 *
 *********************************************************************************/
static Err PrvInsertEntry(AlmEntryPtr entryP)
{
	Err				err = 0;
	AlmGlobalsPtr	almGlobalsP = (AlmGlobalsPtr)GAlmGlobalsP;
	AlmTablePtr		tableP;
	Int16				entryIndex;				// alarm entry index
	UInt32				beforeDate;
	AlmEntryPtr		listP;

	// Ensure an available entry in the alarm table
	if ( entryP->notified )
		beforeDate = entryP->alarmSeconds;
	else
		beforeDate = 0xFFFFFFFFL;
	err = PrvEnsureFreeEntry( beforeDate );
	if ( err ) return( err );
	
	
	// Lock the alarm table
	tableP = (AlmTablePtr)MemHandleLock( almGlobalsP->tableH );
	

	// Find the correct position in the table for the new alarm;
	// The entries are sorted in ascending order by alarmSeconds;
	// All unused entries are at the end;
	listP = tableP->list;
	for ( entryIndex=0; entryIndex < tableP->numEntries &&
			listP[entryIndex].alarmSeconds &&
			entryP->alarmSeconds >= listP[entryIndex].alarmSeconds;
			entryIndex++ )
		{ /* LEFT INTENTIONALLY BLANK */ }


	// Make sure we're still in bounds;  if not, we have an algorithm error
	// since PrvEnsureFreeEntry() guarantees at least one free entry when
	// it is successful.
	ErrFatalDisplayIf( (entryIndex >= tableP->numEntries ||
			tableP->list[tableP->numEntries-1].alarmSeconds),
			"out of bounds" );
	

	// Shift entries forward to make room for the new alarm entry
	if ( tableP->list[entryIndex].alarmSeconds )
		{
		MemMove( &(tableP->list[entryIndex+1]),
				&(tableP->list[entryIndex]),
				sizeof(AlmEntryType) * (tableP->numEntries - entryIndex - 1) );
		}
		
	// Fill in the new alarm entry
	tableP->list[entryIndex] = *entryP;	

	// Unlock the alarm table
	MemHandleUnlock( almGlobalsP->tableH );
	
	
	return( err );
}



/*********************************************************************************
 * PRIVATE
 *-------------------------------------------------------------------------------
 *
 * FUNCTION: PrvEnsureFreeEntry
 *
 * DESCRIPTION: Ensures an unused entry in the alarm table, creating one
 *					or deleting a triggered entry if necessary.  The unused
 *					entry will be at the end of the alarm table.
 *
 * ASSUMPTIONS: Alarm table is NOT locked;
 *
 *		If there is free entry at the end of the table, we're done.
 *		If the table size is below maximum, add an entry at the end.
 *		Otherwise, find the oldest alarm entry which has been notified and
 *		remove it from the table, leaving an unused entry at the end.
 *		If none of the above, the table is considered full.
 *
 * CALLED BY: PrvInsertEntry
 *
 * PARAMETERS: beforeDate		-- if it becomes necessary to remove
 *										   a notified entry to make room for a new alarm
 *										   entry, the oldest entry before this date will
 *										   be deleted (pass 0xFFFFFFFFL to delete the
 *											oldest entry)
 *
 * RETURNS: 	0 if successful; otherwise { almErrMemory, almErrFull }
 *
 * WARNING: can cause the alarm table to move on the dynamic heap,
 *				invalidating stored pointers;
 *
 * CREATED: 4/27/95
 *
 * BY: Vitaly Kruglikov
 *
 *********************************************************************************/
static Err
PrvEnsureFreeEntry(UInt32 beforeDate)
{
	Err				err = 0;
	AlmGlobalsPtr	almGlobalsP = (AlmGlobalsPtr)GAlmGlobalsP;
	AlmTablePtr		tableP;
	Int16				entryIndex;				// alarm entry index
	Int16				numEntries;
	Boolean			done = false;


	//
	// Check if there is free entry at the end of the table
	//
	
	// Lock the alarm table
	tableP = (AlmTablePtr)MemHandleLock( almGlobalsP->tableH );
	
	if ( !tableP->list[tableP->numEntries-1].alarmSeconds )
		done = true;
	
	numEntries = tableP->numEntries;		// save current entry count

	// Unlock the alarm table
	MemHandleUnlock( almGlobalsP->tableH );

	
	//
	// Check if the table size is below maximum, adding an entry at the end
	//
	if ( !done && numEntries < almMaxTableLength )
		{
		// Increment table size by 1 entry
		numEntries++;
		err = MemHandleResize(almGlobalsP->tableH,
				almTableBytes(numEntries) );
		if ( err )
			{
			err = almErrMemory;						// out of memory
			}
		else
			{
			// Lock the alarm table
			tableP = (AlmTablePtr)MemHandleLock( almGlobalsP->tableH );
			
			// Set the new table size
			tableP->numEntries = numEntries;
			
			// Mark the new entry as unused
			tableP->list[numEntries-1].alarmSeconds = 0;
		
			// Unlock the alarm table
			MemHandleUnlock( almGlobalsP->tableH );
			}
		
		done = true;
		}
	
	
	//
	// Find the oldest alarm entry which has been notified and
	// remove it from the table, leaving an unused entry at the end
	//
	if ( !done )
		{
		AlmEntryPtr	listP;
		
		// Lock the alarm table
		tableP = (AlmTablePtr)MemHandleLock( almGlobalsP->tableH );

		numEntries = tableP->numEntries;		// save current entry count
		
		// Search for the oldest notified entry
		listP = tableP->list;
		for ( entryIndex=0; entryIndex < numEntries &&
				(!listP[entryIndex].notified ||
				listP[entryIndex].alarmSeconds >= beforeDate);
				entryIndex++ )
			{ /* LEFT INTENTIONALLY BLANK */ }


		// Unlock the alarm table
		MemHandleUnlock( almGlobalsP->tableH );

		// If found, remove this entry from the table without shrinking the
		// list
		if ( entryIndex < numEntries )
			{
			PrvDeleteEntry(entryIndex, false/*okToShrink*/);
			}
		else
			{
			err = almErrFull;							// table is full
			}

		done = true;
		}
	
	
	return( err );
}



/*********************************************************************************
 * PRIVATE
 *-------------------------------------------------------------------------------
 *
 * FUNCTION: PrvFindUntriggeredAlarm
 *
 * DESCRIPTION: Returns the index of an un-triggered alarm owned by
 *					the given creator app.
 *
 * PARAMETERS:
 *		cardNo			-- creator app card number;
 *		dbID				-- creator app database ID;
 *
 * CALLED BY: AlmGetAlarm, AlmSetAlarm
 *
 * RETURNS: entry index of the alarm, or negative if not found.
 *
 * CREATED: 4/27/95
 *
 * BY: Vitaly Kruglikov
 *
 *********************************************************************************/
static Int16
PrvFindUntriggeredAlarm(UInt16 cardNo, LocalID dbID )
{
	AlmGlobalsPtr	almGlobalsP = (AlmGlobalsPtr)GAlmGlobalsP;
	AlmTablePtr		tableP;
	Int16				entryIndex;
	
	
	// Lock the alarm table
	tableP = (AlmTablePtr)MemHandleLock( almGlobalsP->tableH );


	// Search the alarm table for the un-triggered entry owned by creator app
	for ( entryIndex=0; (entryIndex < tableP->numEntries &&
			tableP->list[entryIndex].alarmSeconds) &&
			(tableP->list[entryIndex].dbID != dbID ||
			tableP->list[entryIndex].cardNo != cardNo ||
			tableP->list[entryIndex].triggered);
			entryIndex++ )
			{ /* LEFT INTENTIONALLY BLANK */ }

	
	// If we stopped out of bounds of used entries, our entry was not found
	if ( !(entryIndex < tableP->numEntries &&
			tableP->list[entryIndex].alarmSeconds) )
		entryIndex = -1;


	// Unlock the alarm table
	MemPtrUnlock( tableP );
	
	return( entryIndex );
}



/*********************************************************************************
 * PRIVATE
 *-------------------------------------------------------------------------------
 *
 * FUNCTION: PrvSetNextAlarm
 *
 * DESCRIPTION: Sets the first un-triggered alarm to be triggered next by
 *					the Time Manager.
 *
 * CALLED BY: AlmSetAlarm, AlmDisplayAlarm
 *
 * PARAMETERS: void
 *
 * RETURNS: void
 *
 * CREATED: 4/27/95
 *
 * BY: Vitaly Kruglikov
 *
 *********************************************************************************/
static void
PrvSetNextAlarm(void)
{
	AlmGlobalsPtr	almGlobalsP = (AlmGlobalsPtr)GAlmGlobalsP;
	AlmTablePtr		tableP;
	Int16				entryIndex;
	UInt32				alarmSeconds;
	
	// Lock the alarm table
	tableP = (AlmTablePtr)MemHandleLock( almGlobalsP->tableH );


	// Find the first un-triggered alarm entry
	for ( entryIndex=0; entryIndex < tableP->numEntries &&
			tableP->list[entryIndex].alarmSeconds &&
			tableP->list[entryIndex].triggered;
			entryIndex++ )
		{ /* LEFT INTENTIONALLY BLANK */ }
	
	// If we stopped in bounds of used entries, our entry was found
	if ( entryIndex < tableP->numEntries &&
			tableP->list[entryIndex].alarmSeconds )
		alarmSeconds = tableP->list[entryIndex].alarmSeconds;
	else
		alarmSeconds = 0;						// 0 cancels Time Manager alarm


	// Unlock the alarm table
	MemPtrUnlock( tableP );


	// Set the alarm to be triggered next
	TimSetAlarm( alarmSeconds );
}



/*********************************************************************************
 * PRIVATE
 *-------------------------------------------------------------------------------
 *
 * FUNCTION: PrvDisplayNextAlarm
 *
 * DESCRIPTION: Display the most recently triggered alarm in the alarm
 *					dialog box.
 *
 *		If we're already displaying an alarm
 *			Sound our own alarm if one needs to be sounded;
 *			return false;
 *		Find the alarm table entry with the oldest triggered alarm.
 *		If there are no more alarms to display, return false.
 *		Delete the alarm's entry from the alarm table.
 *		Call the app's "display alarm" action code to display the alarm.
 *
 * CALLED BY: AlmDisplayAlarm, 
 *
 * PARAMETERS:	none
 *
 * RETURNS:	false if there are no alarms to display or if we're already
 *				displaying an alarm.
 *
 * CREATED: 5/2/95
 *
 * BY: Vitaly Kruglikov
 *
 *********************************************************************************/
static Boolean
PrvDisplayNextAlarm(void)
{
	Boolean			displayed = false;				// return value variable
	AlmGlobalsPtr	almGlobalsP = (AlmGlobalsPtr)GAlmGlobalsP;
	AlmTablePtr		tableP;
	Int16				entryIndex;
	UInt16				cardNo;
	LocalID			dbID;
	SysDisplayAlarmParamType	displayParam;
	UInt32				appLaunchResult;
	
		
	// If we're already displaying an alarm (this checks recursion)
	if ( almGlobalsP->displaying )
		return( false );


	//
	// Find and remove the alarm table entry with the oldest triggered alarm	
	//

	// Lock the alarm table
	tableP = (AlmTablePtr)MemHandleLock( almGlobalsP->tableH );

	// Find the first triggered alarm entry
	for ( entryIndex=0; entryIndex < tableP->numEntries &&
			tableP->list[entryIndex].alarmSeconds &&
			!tableP->list[entryIndex].triggered;
			entryIndex++ )
		{ /* LEFT INTENTIONALLY BLANK */ }

	
	// If we stopped in bounds of used entries, our entry was found
	if ( entryIndex < tableP->numEntries &&
			tableP->list[entryIndex].alarmSeconds )
		{
		// Set up the "display alarm" parameters
		cardNo = tableP->list[entryIndex].cardNo;
		dbID = tableP->list[entryIndex].dbID;
		displayParam.ref = tableP->list[entryIndex].ref;
		displayParam.alarmSeconds = tableP->list[entryIndex].alarmSeconds;
		displayParam.soundAlarm = false;
		
		//  Delete the alarm entry
		PrvDeleteEntry( entryIndex, true/*okToShrink*/);
		} // If we stopped in bounds of used entries, our entry was found
	
	// Else, there are no more alarms to display at this time
	else
		{
		displayParam.alarmSeconds = 0;			// indicates not found
		}

	// Unlock the alarm table
	MemHandleUnlock( almGlobalsP->tableH );
	
	
	// If we have an alarm to display, display it now
	if ( displayParam.alarmSeconds )
		{
		almGlobalsP->displaying = 1;		// indicate that we're displaying an alarm
		SysUIBusy(true,true);				// Tell system that we are using the UI
		
		// Call creator app to display the alarm
		SysAppLaunch( cardNo, dbID, 0/*launchFlags*/, sysAppLaunchCmdDisplayAlarm,
				(MemPtr)(&displayParam), &appLaunchResult );
		
		SysUIBusy(true,false);				// release system UI Busy flag
		almGlobalsP->displaying = 0;		// indicate that we're done displaying
		displayed = true;						// set return flag
		} // If we have an alarm to display, display it now
	
	// Otherwise, return the "nothing displayed" status to the caller
	else
		{
		displayed = false;
		}
	
	
	return( displayed );
}
