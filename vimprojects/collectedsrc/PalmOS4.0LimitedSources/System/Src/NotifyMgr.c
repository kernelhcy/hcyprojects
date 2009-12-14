/******************************************************************************
 *
 * Copyright (c) 1998-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: NotifyMgr.c
 *
 * Release: 
 *
 * Description:
 *		This file contains the Notification Manager routines for Pilot.
 *
 * 
 * THEORY OF OPERATION:
 *		The Notification manager is designed to allow applications and 
 * other entities (libraries, hacks, etc.) to be notified of the occurance 
 * of certain events in an efficient manner.  Presently, 'efficient' 
 * mostly just means that we only notify the apps that specifically request 
 * information about a particular event.
 * 
 *		Before an app will receive notification for anything, it must 
 * register for the event type of interest with the notification manager by 
 * calling SysNotifyRegister.  It must pass its database ID and cardNo, as well as 
 * the event type of interest (defined in NotifyMgr.h), a notification priority, 
 * and an optional pointer to a callback routine.  It is recommended that applications
 * always pass 0 for the callback MemPtr, and use the launchcode mechanism instead.
 * Callbacks are there primarily for clients that don't have a PilotMain routine
 * to use with the launchcode mechanism (like libraries, hacks, and the OS itself).
 * If you DO use a callback, it is VITAL that you make sure that your code
 * is locked in memory the whole time the callback is registered with the NotifyMgr.
 * 
 *		Once registered, a client will receive notification upon the occurance of
 * the event of interest.  Generally this happens via the launchcode mechanism - 
 * the application's PilotMain routine is called with the sysAppLaunchCmdNotify
 * launchcode, and the paramP variable points to a SysNotifyParamType structure
 * containing the details of the notification.  From within a notification handler
 * (PilotMain, or a callback routine) the client may register or unregister with
 * the notification manager.  It may ALSO start another broadcast, but keep in mind
 * that only 5 levels of broadcast are possible.  After that, SysNotifyBroadcast will
 * return sysNotifyErrBroadcastBusy.  For this reason, it is reccomended that if you must 
 * broadcast from within a handler, you use a deferred bradcast.  Calling 
 * SysNotifyBroadcastDeferred is just like SysNotifyBroadcast except that the notification
 * does not take place until the next time through the event queue (specifically, the next
 * time EvtGetEvent is called).  The NotifyMgr uses the paramSize parameter to copy the
 * optional extra data (in the notifyDetailsP) to a new chunk which is disposed upon
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
 * 
 *		Since all notifications are broadcast from the UI thread, a notify handler can
 * also generally make UI system calls.  Synchronous (non-deferred) broadcasts initiated
 * from a background thread are automagically deferred, and the background thread in 
 * question is put to sleep until the broadcast has finished (usually the next time the 
 * curent UI application calls EvtGetEvent()).
 *
 * History:
 *	06/15/98	jed		Initial revision
 *	12/07/99	jed		Code review changes
 *  06/26/00	gfa		Added an EvtWakeup call after enqueuing a deferred broadcast.
 *  07/17/00	gfa		Set extra parameter block owner to system (SysNotifyBroadcastDeferred)
 *
 *****************************************************************************/

/*	
	Things to remember:
	* What do we do in a broadcast if SysAppLaunch returns an error? 
		Ignore & continue to next, or cancel & return it?
	* Restriction: can reg/unreg from broadcast but not initiate another broadcast.
		Do a deferred broadcast instead.  Actually, you can do another broadcast now,
		but a MAXIMUM of only 5 nested broadcasts may be in progress at the same time.
	* Add lots of error checking & sanity stuff. How paranoid to be?
	
	Things to Carefully Document:
	* NOT interrupt safe.
	* Cleanup & delete notifies are NOT guarantees of deletion
	* We internally alloc a block and copy data to it and free it for deferred broadcasts.
		This is only done for SysNotifyBroadcastDeferred.
		SysNotifyBroadcast, when called from a BG thread, doesn't need to do this
		because the thread will remain asleep, so the MemPtr will remain valid.
		
	* For each event:
		Can this event be broadcast from a background task? If so, document it.
			All handlers for his event must be thread-safe & not make OS calls that aren't.
			If not, then is it safe for apps to make UI calls? 
	* all lowercase event types are reserved for system - just like creator codes
	* Document reentrancy restriction: reg/unreg but no broadcast
	* Extreme priorities are reserved for system.
	
	
	Where we added callbacks:
	hots: SyncApp.c, StartSync()
	sync: SyncApp.c, Wherever we enable alarm notifications
	time: TimeMgr.c, TimeMgr68328.c, TimeMgr68EZ328.c: TimSetSeconds()
	rstf: UIAppShell's Main.c, after reset codes are sent to all apps.
	bozo: SecApp.c, MainViewHandleEvent(), under DeletePasswordButton
	slpq: SysHandleEvent, cases for vchrHardPower, vchrAutoOff, and vchrResumeSleep
	slp!: SysHandleEvent, case for vchrPowerOff
	worm: SysHandleEvent, end of case for vchrPowerOff
	lazy: SysHandleEvent, case for vchrLateWakeup (enqueued in case for vchrPowerOff)
	
	
	Where to add callbacks:
	dbs+: DataMgr.c, DmCloseDatabase, if modify# == 1
	dbs-: Handled in HandleDatabaseDeleted, which is called from DataMgr.c, 
			DmDeleteDatabase() at beginning - once we're sure it will succeed
	gbye: Same as dbs-.
	
	
*/


#define NON_PORTABLE

#include <PalmTypes.h>

// public System includes
#include "DataMgr.h"
#include "ErrorMgr.h"
#include "NotifyMgr.h"
#include "StringMgr.h"
#include "SysEvtMgr.h"
#include "SysUtils.h"
#include "SystemMgr.h"

// private System includes
#include "NotifyPrv.h"
#include "SysEvtPrv.h"
#include "SystemPrv.h"
#include "AttentionPrv.h"
#include "Globals.h"

//-------------------------------------------------------------------
//		Private declarations used only by this module
//-------------------------------------------------------------------


#define sysNotifyResizeTableBy	5	// grow table 5 entries at a time
#define sysNotifyListMinSize	10	// table is always at least 10 big

// put in curBroadcast to tell there is no broadcast in progress
#define sysNotifyNoCurBroadcast		0xFFFF
#define sysNotifyIllegalListIndex	0xFFFF
#define sysNotifyHighestPriority	0x80	// the most negative 1-byte integer

// need at least 256 bytes of stack space before we do a broadcast.
#define sysNotifyMinStackSpaceForBroadcast	256

/*
	When a client registers for a notification, one of these 
	structures is inserted into the global array so that we
	remember it.
*/
typedef struct SysNotifyListEntryType
	{
	LocalID 			dbID;		// LocalID of database to notify
	SysNotifyProcPtr	callbackP;	// MemPtr to callback routine, or 0 (use launchcode).
	UInt32				notifyType;	// The type of event we want notification for
	void *				userDataP;	// user specified MemPtr passed back with notification.
	Int8				priority;	// Controls sort order within each event type
	UInt8				cardNo;		// cardNo database resides on
							
	}SysNotifyListEntryType;


// Small inline to write a value to the global maxEntries field.
inline Err PrvWriteGlobalsMaxEntries(SysNotifyGlobalsPtr globalsP, UInt16 value)
{
	globalsP->maxEntries = value;
	return errNone;
}

// Small inline to write a value to the global numEntries field.
inline Err PrvWriteGlobalsNumEntries(SysNotifyGlobalsPtr globalsP, UInt16 value)
{
	globalsP->numEntries = value;
	return errNone;
}

// Small inline to write a value to the 'head' field of the global deferredQueue.
inline void PrvWriteGlobalsQueueHead(SysNotifyGlobalsPtr globalsP, UInt16 value)
{
	globalsP->deferredQueue.head = value;
}

// Small inline to write a value to the 'tail' field of the global deferredQueue.
inline void PrvWriteGlobalsQueueTail(SysNotifyGlobalsPtr globalsP, UInt16 value)
{
	globalsP->deferredQueue.tail = value;
}

// Small inline to write a value into the 'nextStackIndex' field in the globals.
inline void PrvWriteGlobalsNextStackIndex(SysNotifyGlobalsPtr globalsP, UInt16 value)
{
	globalsP->nextStackIndex = value;
}

// Small inline to write aa value into the given broadcast stack element.
inline void PrvWriteGlobalsBroadcastStack(SysNotifyGlobalsPtr globalsP, UInt16 which, UInt16 value)
{
	globalsP->broadcastStack[which] = value;
}


// Small inline to write an entry into the global deferredQueue.
inline void PrvWriteGlobalsQueueEntry(SysNotifyGlobalsPtr globalsP, UInt16 which, SysNotifyDeferredQueueEntryType *entry)
{
	globalsP->deferredQueue.entry[which] = *entry;
}


//-------------------------------------------------------------------
//		Private routines used only by this module
//-------------------------------------------------------------------
static Err PrvInsertEntry(SysNotifyGlobalsPtr notifyGlobalsP, SysNotifyListEntryType *entry);
static Err PrvDeleteEntry(SysNotifyGlobalsPtr notifyGlobalsP, UInt16 pos);
static UInt16 PrvFindEntry(SysNotifyGlobalsPtr notifyGlobalsP, SysNotifyListEntryType *entry);
static Int16 PrvSortCompareFunc (void const * dataP, void const * curEntryP, Int32 other);
static Err PrvGetGlobals(SysNotifyGlobalsPtr *globalsPP);
static void PrvReleaseGlobals(SysNotifyGlobalsPtr globalsP);
static void PrvHandleDatabaseDeleted(SysNotifyGlobalsPtr globalsP, UInt16 cardNo, LocalID dbID);
static Err PrvBroadcastNotification(SysNotifyParamType *notify, SysNotifyGlobalsPtr globalsP);

static Err PrvValidateEntry(SysNotifyListEntryType *entry);
static Boolean PrvAreWeUIThread(void);
static Err PrvDeferredQueueInsert(SysNotifyGlobalsPtr globalsP, UInt32 taskID, SysNotifyParamType *notify, UInt8 validFlags);
static Err PrvDeferredQueueRemove(SysNotifyGlobalsPtr globalsP, SysNotifyDeferredQueueEntryType *outEntry);
static Err PrvInterruptQueueRemove(SysNotifyGlobalsPtr globalsP, SysNotifyDeferredQueueEntryType *outEntry);
static Boolean PrvSearchQueueForTaskID(SysNotifyGlobalsPtr globalsP, UInt32 taskID);
static Err PrvCheckStackSpace(void);
static MemPtr PrvGetSP(void);
static Err PrvBroadcastDeferred(SysNotifyGlobalsPtr globalsP, const SysNotifyParamType *param, 
								Int16 paramSize, UInt32 taskID);

#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
static void PrvValidateGlobals(SysNotifyGlobalsPtr globalsP);
#else
#define PrvValidateGlobals(globalsP)
#endif

#if EMULATION_LEVEL != EMULATION_NONE
// Used for debugging under the simulator only
Int16 SysNotifyCountEntries(void);
#endif

//-------------------------------------------------------------------
//		Notification Manager Initialization routines
//-------------------------------------------------------------------


/************************************************************
 * API - INITIALIZATION
 *-----------------------------------------------------------
 *
 *  FUNCTION: SysNotifyInit
 *
 *  DESCRIPTION: This routine allocates & initializes all the necessary 
 *				globals for the NotifyMgr, including an empty list of 
 *				notify registrations.  This must be called by the system 
 *				before the NotifyMgr can be used.
 *				
 *  PARAMETERS: void
 *
 *  RETURNS: errNone if successful; otherwise an error code
 *
 *  CREATED: 6/15/98
 *
 *  BY: Jesse Donaldson
 *
 *************************************************************/
Err SysNotifyInit(void)
{
	Err err;
	MemHandle notifyGlobalsH;
	SysNotifyGlobalsPtr notifyGlobalsP;	
	SysNotifyDeferredQueueEntryType entry;
	UInt16 i;
	
	// Reserve the semaphore (we use the memory semaphore to protect our globals)
	err = MemSemaphoreReserve(false);
	ErrNonFatalDisplayIf(err, "need semaphore");
	
	// initialize: no pending broadcasts yet
	GSysNotifyBroadcastPending = false;
	
	// Search for our prefs resource... 
	// and if we can't find it, create one and initialize it.
	
	notifyGlobalsH = MemHandleNew(sizeof(SysNotifyGlobalsType));
	notifyGlobalsP = MemHandleLock(notifyGlobalsH);
	
	// allocate minimum-size notifyList:
	notifyGlobalsP->notifyListH = DmNewHandle(0, 
						sizeof(SysNotifyListEntryType)*sysNotifyListMinSize);
	ErrFatalDisplayIf(err, "can't allocate notifyList");
	
	// Initialize the globals:
	PrvWriteGlobalsNumEntries(notifyGlobalsP, 0);
	PrvWriteGlobalsMaxEntries(notifyGlobalsP, sysNotifyListMinSize);

	// Instead of resetting this, we can read it to see if there 
	// was a broadcast in progress when we reset, and if so, remove 
	// the notification being handled when we reset because it probably 
	// crashed.  If we do this, though, we also need to make sure we 
	// set this to sysNotifyNoCurBroadcast when we create the new resource
	// (above).  Also, this is silly as long as the list is cleared on reset.
	
	// Clear the broadcast stack:
	PrvWriteGlobalsNextStackIndex(notifyGlobalsP, 0);
	PrvWriteGlobalsBroadcastStack(notifyGlobalsP, 0, sysNotifyNoCurBroadcast);
	PrvWriteGlobalsBroadcastStack(notifyGlobalsP, 1, sysNotifyNoCurBroadcast);
	PrvWriteGlobalsBroadcastStack(notifyGlobalsP, 2, sysNotifyNoCurBroadcast);
	PrvWriteGlobalsBroadcastStack(notifyGlobalsP, 3, sysNotifyNoCurBroadcast);
	PrvWriteGlobalsBroadcastStack(notifyGlobalsP, 4, sysNotifyNoCurBroadcast);
	
	// Initialize the deferred queue:	
	PrvWriteGlobalsQueueHead(notifyGlobalsP, 0);
	PrvWriteGlobalsQueueTail(notifyGlobalsP, 0);
	MemSet(&entry, sizeof(SysNotifyDeferredQueueEntryType), 0);
	for(i=0; i<sysNotifyDefaultQueueSize; i++)
		{
		PrvWriteGlobalsQueueEntry(notifyGlobalsP, i, &entry);
		}
	
	notifyGlobalsP->uiRunning = false;
	
	// allocate and initialize the interrupt queue	
	GSysNotifyInterruptGlobalsP = MemPtrNew(SysNotifyInterruptQueueDataSize + sizeof(KeyQueueType));
	MemSet(GSysNotifyInterruptGlobalsP, SysNotifyInterruptQueueDataSize + sizeof(KeyQueueType), 0);
	((KeyQueuePtr)GSysNotifyInterruptGlobalsP)->size = SysNotifyInterruptQueueDataSize;
	
	// Validate everything & clean up.
	PrvValidateGlobals(notifyGlobalsP);
	
	MemHandleUnlock(notifyGlobalsH);
	GSysNotifyGlobalsH = notifyGlobalsH;
		
	MemSemaphoreRelease(false);
	
		
	// Should we broadcast an app+ event for each of the built in apps now?
	// Or a bit later after everything has registered? (e.g. the launcher?)
	
	return errNone;
}


//-------------------------------------------------------------------
//		Notification Manager API routines
//-------------------------------------------------------------------



/************************************************************
 * API
 *-----------------------------------------------------------
 *
 *  FUNCTION: SysNotifyRegister
 *
 *  DESCRIPTION: Register for notification of the given event 
 *				type.  Notification will be carried out using 
 *				the launch code mechanism or by calling the given
 *				callback routine (if callbackP != 0).
 *				
 *  PARAMETERS: 
 *		cardNo		-	Card # that the application resides on
 *		dbID		-	Database ID of the application, or sysNotifyNoDatabaseID if none.
 *						(e.g. the OS will pass sysNotifyNoDatabaseID when it registers)
 *		notifyType	-	Type of event we're requesting notification for
 *		callbackP	-	Pointer to callback routine used for notification
 *						If 0, the standard launchcode mechanism will be used.
 *						If using callbacks, it is VITAL  that you make sure
 *						that the code is locked in memory while registered.
 *		priority	-	0 = average, <0 = notify me earlier, > 0 notify me later.
 *						Generally, everyone should pass 0.
 *		
 *  RETURNS: errNone if successful; error otherwise
 *
 *  CREATED: 6/15/98
 *
 *  BY: Jesse Donaldson
 *
 *************************************************************/
Err SysNotifyRegister(UInt16 cardNo, LocalID dbID, UInt32 notifyType, 
						SysNotifyProcPtr callbackP, Int8 priority, void * userDataP)
{
	SysNotifyListEntryType entry;
	SysNotifyGlobalsPtr globalsP;
	Err err;
	
	ErrFatalDisplayIf(priority == sysNotifyHighestPriority, "Bad priority");
	
	err = PrvGetGlobals(&globalsP);
	if(err)
	{
		if(err == sysErrNotInitialized) ErrFatalDisplay("not initialized");
		return err;
	}
	
	// fill out an entry structure to insert into array
	entry.notifyType = notifyType;
	entry.priority = priority;
	entry.callbackP = callbackP;
	entry.userDataP = userDataP;
	entry.cardNo = cardNo;
	entry.dbID = dbID;
	
	err = PrvValidateEntry(&entry);	// validate the entry a little
	if(err == 0)
		{
		// Make sure this isn't already registered...
		if(PrvFindEntry(globalsP, &entry) == (UInt16)sysNotifyIllegalListIndex)
			{
			// ...then insert it in the list
			err = PrvInsertEntry(globalsP, &entry);
			}
		else err = sysNotifyErrDuplicateEntry;
		}
	
	PrvReleaseGlobals(globalsP);
	
	return err;
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 *  FUNCTION: SysNotifyUnregister
 *
 *  DESCRIPTION: Remove a registration from the list.
 *				Cancels notification of the given event for 
 *				the given app/callback routine.
 *				
 *  PARAMETERS: 
 *		cardNo		-	Card # that the application resides on
 *		dbID		-	Database ID of the application
 *		notifyType	-	Type of event we're requesting notification for
 *
 *  RETURNS: errNone if successful; otherwise an error code
 *
 *  CREATED: 6/15/98
 *
 *  BY: Jesse Donaldson
 *
 *************************************************************/
Err SysNotifyUnregister(UInt16 cardNo, LocalID dbID, UInt32 notifyType, Int8 priority)
{
	SysNotifyListEntryType entry;
	SysNotifyGlobalsPtr globalsP;
	UInt16 pos;
	Err err;
	
	err = PrvGetGlobals(&globalsP);
	if(err) return err;
		
	// fill out an entry structure...
	entry.notifyType = notifyType;
	entry.cardNo = cardNo;
	entry.dbID = dbID;
	entry.priority = priority;
	entry.callbackP = 0;
	
	// search the list for this entry:
	pos = PrvFindEntry(globalsP, &entry);
	
	// return an error if we were unable to find the entry.
	// Otherwise, just delete it.
	if(pos == sysNotifyIllegalListIndex)
		{
		err = sysNotifyErrEntryNotFound;
		}
	else 
		{
		err = PrvDeleteEntry(globalsP, pos);
		}
	
	// release memory semaphore & control of our globals.
	PrvReleaseGlobals(globalsP);

	return err;
}



/************************************************************
 * API
 *-----------------------------------------------------------
 *
 *  FUNCTION: SysNotifyBroadcast
 *
 *  DESCRIPTION: Immediately send a notification to all  
 *				apps registered for the given event type.
 *				Pass NULL to broadcast the deferred 
 *				notification at the head of the queue (if any).
 *				
 *				
 *  PARAMETERS: 
 *		notify	-	Filled-out broadcast notification structure
 *					to be sent to clients. 'handled' value may
 *					be set to indicate the event has been handled.
 *					Or, pass 0 to broadcast the next deferrred
 *					anouncement.
 *		
 *	WARNING: Never call SysNotifyBroadcast from a background task 
 *			(or from any code that might be called from a BG task)
 *			with the memory semaphore reserved.  Deadlock will 
 *			result when the broadcast is deferred and the UI task 
 *			tries to acquire the mem semaphore in order to send it out.
 *		
 *	NOTE: Handlers (callback funcs or applications) may register 
 *			or unregister for notifications.  We save our current
 *			position in the notifyList array in our globals, and 
 *			these routines update it.  HOWEVER, there is only room
 *			in the globals for 5 of these saved values, so we can
 *			do a maximum of only 5 nested broadcasts.
 *	
 *  RETURNS:  0 if successful, else an Err code
 *
 *  REVISION HISTORY:
 *  jed	6/15/98	initial revision
 *  jed	7/01/00	Modifications for new validFlags field, don't dispose of notifyDetailsP unless valid bit is set
 *
 *************************************************************/
Err SysNotifyBroadcast(SysNotifyParamType *notify)
{
	SysNotifyGlobalsPtr globalsP = 0;
	SysNotifyDeferredQueueEntryType *deferredNotifyP = NULL;
	UInt32 taskID;
	Err err;	
	
	// Get ownership of our globals:
	err = PrvGetGlobals(&globalsP);
	if(err) goto Exit;
	
	if((notify != NULL) && (notify->notifyType == sysNotifyResetFinishedEvent))
		{
		globalsP->uiRunning = true;
		}
	
	// return imediately if we have no space left in the broadcast stack.
	// This will prevent us from removing an element from the deferred queue,
	// and then finding out that we can't broadcast it.
	if(globalsP->nextStackIndex >= sysNotifyMaxBroadcastDepth)
		{
		err = sysNotifyErrBroadcastBusy;
		goto Exit;
		}
	
	if(PrvAreWeUIThread() || globalsP->uiRunning == false)
		{
		// Okay, we are being called from the UI thread, so its safe to broadcast.
		// if there's is nothing to broadcast, see if we can pull something from 
		// the deferred queue.
		if(notify == 0)	
			{
			// clear the global so we won't call the NotifyMgr each time through the event loop.
			// it's important that this is done FIRST, because an interrupt may occur while
			// we're in the following code that would then set the global to TRUE again, and we want
			// to make sure we don't accidently clear it after that happens.
			// That is, don't clear this AFTER we check the queue state because an interrupt could
			// have enqueued something between when we check the queue state and when we clear
			// the boolean.
			GSysNotifyBroadcastPending = false;
			
			deferredNotifyP = MemPtrNew(sizeof(SysNotifyDeferredQueueEntryType));
			if(deferredNotifyP == NULL)
			{
				err = memErrNotEnoughSpace;
				goto Exit;
			}
			
			err = PrvInterruptQueueRemove(globalsP, deferredNotifyP);			// check the INTERRUPT queue

			if (err == sysNotifyErrQueueEmpty)
				err = PrvDeferredQueueRemove(globalsP, deferredNotifyP);	// check the DEFERRED queue

			
			if(err == 0)
				{
				PrvBroadcastNotification(&deferredNotifyP->notify, globalsP);
				
				
				// If this was an automagically deferred broadcast, 
				// then we have a task to wake up.  
				if(deferredNotifyP->taskID)
					{
					// wake up the task
					// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
					// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
					
					// ignore sysErrNotAsleep since the background thread 
					// might not have gone to sleep yet if it was switched out
					// right when it called SysEvtWakeup() to tickle the UI task.
					ErrNonFatalDisplayIf((err && (err != sysErrNotAsleep)), "error waking task");
					ErrFatalDisplayIf((err && (err != sysErrNotAsleep) && (err != sysErrNotAsleepN)), "error waking task");
					}
				else	// Otherwise, SysNotifyBroadcastDeferred made a local
					{	// copy of the data (if any), so we must dispose of it.
					
					// Free the param MemPtr for the deferred broadcast if there is one
					if (deferredNotifyP->notify.notifyDetailsP != NULL)
						{
						// valid is set to false for interrupt notifications as flag to NOT delete the ptr
						// because interrupt notifications may not actually have a ptr here...
						if (deferredNotifyP->validFlags & notifyValidDetailsP)
							MemPtrFree(deferredNotifyP->notify.notifyDetailsP);
						deferredNotifyP->notify.notifyDetailsP = NULL;
						}
					}
				
				}
			
			}
		else	// Otherwise, just broadcast the notify we were given:
			{
			err = PrvBroadcastNotification(notify, globalsP);
			}
		
		}
	
	// Otherwise, we're being called from a background task, so we need to
	// defer the broadcast and put this thread to sleep until its handled
	// in EvtGetEvent (from the UI thread) so that it appears the broadcast 
	// happened instantaneously.
	else	
		{
		// Make sure we don't try to broadcast deferred notifies from a background task.
		if(notify == 0) 
			{
			err = sysErrParamErr;
			goto Exit;
			}
		
		
		// defer the broadcast
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
		err = PrvDeferredQueueInsert(globalsP, taskID, notify, 0);
		
		// make sure we release the globals (and the memory semaphore)
		// BEFORE we put the task to sleep...
		PrvReleaseGlobals(globalsP);
		globalsP = 0; // so globals aren't released on exit...
		
		// if everything's ok, put this task to sleep 
		if(err == 0)
			{
			
			while(1)
				{
				// Tickle the UI task in case its asleep.
				EvtWakeup(); 
				
				// Note: we can be switched out right here, and its possible that
				// the broadcast will actually be carried out before we run again
				// (and thus, before we go to sleep to wait for it).
				// If this happens, we'll have one wake "pending" so, SysTaskWait()
				// should return immediately, and we'll see by checking our list that
				// the notification has gone out.
				
				// sleep 'til we're woken.
				while(SysTaskWait(0) != 0) ;
				
				// Ensure that it was the deferred broadcast handler that woke us up
				// by checking to see if there is an entry in the deferred queue
				// that is marked with our task ID.  If so, then it hasn't been
				// handled yet - go back to sleep.
				
				err = PrvGetGlobals(&globalsP);
				ErrFatalDisplayIf(err, "need globals");
				
				// if the entry is gone, break out of our wait-loop.
				// The globals will be released below.
				if(PrvSearchQueueForTaskID(globalsP, taskID) == false)
					{
					break;
					}
				
				// otherwise, release the globals - we're going back to sleep.
				else
					{
					PrvReleaseGlobals(globalsP);
					globalsP = 0; // so globals aren't released on exit...
					}
				}
			
			}
		
		}
	
Exit:
	if(globalsP) PrvReleaseGlobals(globalsP);
	
	if(deferredNotifyP) MemPtrFree(deferredNotifyP);
	
	// If we aren't initialized, then there isn't anyone to receive a 
	// broadcast.  So, SysNotifyBroadcast is a no-op, and we have done our task.
	if(err == sysErrNotInitialized) err = errNone;
	
	return err;
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 *  FUNCTION: SysNotifyBroadcastDeferred
 *
 *  DESCRIPTION: Enqueue a notification for later broadcasting
 *				from the UI thread.
 *				
 *				
 *  PARAMETERS: 
 *		paramP		-	ptr to notification to enqueue.
 *		paramSize	-	size of data pointed to by param->notifyDetailsP so we can copy it, or 0 to just copy the value.
 *		
 *  RETURNS: 0 on success
 *
 *  REVISION HISTORY:
 *  jed	6/15/98	initial revision
 *  jed	7/01/00	Modify to allow paramSize = 0 to just copy the value of notifyDetailsP.
 *
 *************************************************************/

Err SysNotifyBroadcastDeferred(SysNotifyParamType *paramP, Int16 paramSize)
{
	SysNotifyGlobalsType	*globalsP;
	SysNotifyParamType		newParam = *paramP;
	Err						err;
	UInt8						validFlags = 0;
	
	err = PrvGetGlobals(&globalsP);
	if(err) return err;
	
	// If there is a parameter structure in the notifyDetailsP slot,
	// then we need to make a local copy since the ptr may no longer 
	// be valid when the notify gets handled in EvtGetEvent().
	// So, allocate a chunk for it, copy it in, and use that instead.
	// This way, we can dispose of it in the deferred broadcast handler,
	// and pointers to structures on the stack will still be okay.
	// (otherwise, since its a deferred broadcast, its likely that the 
	// structure would be deallocated before the broadcast went out.)
	// If the caller passes a value in notifyDetailsP, but 0 for the 
	// parameter size, we just pass the value along and don't dispose 
	// of it automatically.  This can be used when you have <= 4 bytes 
	// of data, or when you're passing a ptr to a structure which will 
	// not ever be deallocated.
	if(newParam.notifyDetailsP && paramSize != 0)
	{
		MemPtr dataP;
		
		// param size MUST be >0 if the paramP field is non-null.
		if(paramSize < 0) return sysErrParamErr;
		
		dataP = MemPtrNew(paramSize);
		if(dataP == 0) return memErrNotEnoughSpace;
		MemPtrSetOwner(dataP, 0);
		MemMove(dataP, newParam.notifyDetailsP, paramSize);
		newParam.notifyDetailsP = dataP;
		validFlags |= notifyValidDetailsP;
	}
	
	err = PrvDeferredQueueInsert(globalsP, 0, &newParam, validFlags);
	
	PrvReleaseGlobals(globalsP);
	
    // we need to unblock the UI task if blocked in the EvtGetEvent(..., evtWaitForever)
    EvtWakeup();

	return err;
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 *  FUNCTION: SysNotifyDatabaseAdded
 *
 *  DESCRIPTION: This routine is called by the system when a 
 *				database is created on the device.  It sends a 
 *				'database added' notification to all registered 
 *				clients.  Additionally, if the database is an 
 *				application, then it sends an 'install' 
 *				notification to it (using the launchcode mechanism).
 *				
 *				
 *  PARAMETERS: 
 *		cardNo	-	card number that the new app resides on
 *		dbID	-	database id of the app's database
 *		
 *		
 *  RETURNS: nothing
 *
 *  CREATED: 6/15/98
 *
 *  BY: Jesse Donaldson
 *
 *************************************************************/
void SysNotifyDatabaseAdded(UInt16 /* cardNo */, LocalID /* dbID */)
{
	ErrFatalDisplay("unimplemented");
	return;
}
/*
From DataMgr.c:
	// Broadcast the databaseCreated event.
	// We detect if it was just created by checking 
	// to see if it was unmodified when it was opened.
//	if(openP->hdrP->modificationNumber == 0 && GSysNotifyGlobalsH != 0)
	if(dbP->savedModNum == 0 && GSysNotifyGlobalsH != 0)
		{
		// Is it okay to do this before the db is finished being closed?
		SysNotifyDatabaseAdded(openP->cardNo, openP->hdrID);
		}

NOT NEEDED UNTIL WE IMPLEMENT THE dbs+ and dbs- events.
{
	SysNotifyDBInfoType info;
	SysNotifyParamType notify;
	UInt32 result;
	Err err;
	
	// fill out the info structure:
	err = DmDatabaseInfo(cardNo, dbID, 0,0,0,0,0,0,0,0,0,&info.typeID, &info.creatorID);
	ErrNonFatalDisplayIf(err, "dbInfo failed");
	
	info.cardNo = cardNo;
	info.dbID = dbID;
	
	// fill out the notify structure:
	notify.broadcaster = sysNotifyBroadcasterCode;
	notify.notifyDetailsP = &info;
	
	// If its an application, send it an initialization message
	if(info.typeID == sysFileTApplication)
		{
		notify.notifyType = sysNotifyInitializeEvent;
		notify.handled = false;
		
		// notify the app that it's been installed
		err = SysAppLaunch(cardNo, dbID, 0, sysAppLaunchCmdNotify, (char*)&notify, &result);
		ErrNonFatalDisplayIf(err, "launch failed");
		}
		
	// Broadcast DBAdded event to all registrees
	notify.notifyType = sysNotifyDBAddedEvent;
	notify.handled = false;
	err = SysNotifyBroadcast(&notify);
	ErrNonFatalDisplayIf(err, "b'cast failed");
	
	return;
}
*/

/************************************************************
 * API
 *-----------------------------------------------------------
 *
 *  FUNCTION: SysNotifyDatabaseRemoved
 *
 *  DESCRIPTION: This routine is called by the system just  
 *				before a database is deleted from the system.  
 *				It unregisters the database for all notifications, 
 *				broadcasts a database deleted notification to  
 *				anyone that's registered for it, and (if the 
 *				database is an application) sends a cleanup 
 *				notification to the unlucky database.
 *				
 *  PARAMETERS: 
 *		cardNo	-	card number that the app resides on
 *		dbID	-	database id of the app's database
 *		
 *		
 *  RETURNS: nothing
 *
 *  CREATED: 6/15/98
 *
 *  BY: Jesse Donaldson
 *
 *************************************************************/

void SysNotifyDatabaseRemoved(UInt16 cardNo, LocalID dbID)
{
	SysNotifyGlobalsPtr globalsP;
	Err err;
/*	SysNotifyDBInfoType info;
	SysNotifyParamType notify;
	UInt32 result;
	
	// fill out the info structure:
	err = DmDatabaseInfo(cardNo, dbID, 0,0,0,0,0,0,0,0,0,&info.typeID, &info.creatorID);
	ErrNonFatalDisplayIf(err, "dbInfo failed");
	info.cardNo = cardNo;
	info.dbID = dbID;
	
	// fill out the notify structure:
	notify.broadcaster = sysNotifyBroadcasterCode;
	notify.notifyDetailsP = &info;
	
	// Broadcast dbRemoved event to all registrees
	notify.notifyType = sysNotifyDBRemovedEvent;
	notify.handled = false;
	err = SysNotifyBroadcast(&notify);
	ErrNonFatalDisplayIf(err, "b'cast failed");
	
	
	// If its an application, say goodbye:
	if(info.typeID == sysFileTApplication)
		{
		// We set its type to sysFileTTemp.  This way, if the handler
		// crashes, the database will be automatically deleted on reset.
		// This will only protect us from accidental crashes - not 
		// malignant code. (The handler can just set the db type back).  
		// Its important here becuase this notify is always broadcast, 
		// so its conceivable that the app might not be expecting it.
		result = sysFileTTemp;
		DmSetDatabaseInfo(cardNo, dbID, 0,0,0,0,0,0,0,0,0,&result,0);
		
		notify.notifyType = sysNotifyCleanupEvent;
		notify.handled = false;
		
		// notify the app that it's about to be deleted
		err = SysNotifyBroadcast(&notify);
		ErrNonFatalDisplayIf(err, "b'cast failed");
		}
*/
	
	// and finally, clean up our own notifyList:
	// It seems like a waste to search the whole list every time
	// we delete any database, but that's how we decided this
	// event was going to work, and we shouldn't say its good
	// enough for everyone else, but not good enough for NotifyMgr.
	// Bob says db deletes don't happen very often, so its okay.
	err = PrvGetGlobals(&globalsP);
	ErrNonFatalDisplayIf(err, "need MemSemaphore");
	
	if(err == errNone)
		{
		PrvHandleDatabaseDeleted(globalsP, cardNo, dbID);
		PrvReleaseGlobals(globalsP);
		}

	return;
}



/************************************************************
 * API - DEBUGGING
 *-----------------------------------------------------------
 *
 *  FUNCTION: SysNotifyCountEntries
 *
 *  DESCRIPTION: A simple little accessor available under the 
 *				emulator that just returns the number of 
 *				registered entries in the notifyList.
 *				
 *				
 *  PARAMETERS: none
 *		
 *		
 *  RETURNS: numEntries field from globals
 *
 *  CREATED: 6/15/98
 *
 *  BY: Jesse Donaldson
 *
 *************************************************************/
#if EMULATION_LEVEL != EMULATION_NONE
Int16 SysNotifyCountEntries(void)
{
	SysNotifyGlobalsPtr globalsP;
	Err err = PrvGetGlobals(&globalsP);
	Int16 count;
	
	if(err) return err;
	
	count = globalsP->numEntries;
	PrvReleaseGlobals(globalsP);
	return count;
}
#endif



//-------------------------------------------------------------------
//		Private routines used only by this module
//-------------------------------------------------------------------




/************************************************************
 * PRIVATE
 *-----------------------------------------------------------
 *
 *  FUNCTION: PrvBroadcastNotification
 *
 *  DESCRIPTION: Broadcast the given notification to all clients
 *				who are registered for it.  Cleanup notifications
 *				are broadcast ONLY to the app being deleted.
 *				
 *  PARAMETERS: notifyP	-	Ptr to notification to broadcast.
 *				globalsP-	MemPtr to NotifyMgr globals.
 *		
 *  RETURNS: 0 if no error.
 *
 *  REVISION HISTORY:
 *  jed	6/15/98	initial revision
 *  jed	4/25/00	Release semaphore when making callback/sublaunch
 *						to allow background tasks to use Notify & Memory Mgrs.
 *
 *************************************************************/
Err PrvBroadcastNotification(SysNotifyParamType *notifyP, SysNotifyGlobalsPtr globalsP)
{
	const SysNotifyListEntryType *curEntry;
	SysNotifyListEntryType entry, *notifyListP;
	UInt32 			notifyType;
	Int32			bigPos;	// temp to pass to SysBinarySearch
	UInt16			pos, stackIndex;
	Err				err;
	UInt32			result;
	LocalID			dbID;
	UInt16			cardNo;
	
	err = PrvCheckStackSpace();
	if(err) return err;
	
	notifyListP = NULL;
	
	// Before we do anything else, check to see if this is an app cleanup notification.
	// If it is, then we just send a launchcode to the app being deleted.  
	// Only if its NOT, do we search the notifyList and send it out to everyone else.
/*	if(notify->notifyType == sysNotifyCleanupEvent)
		{
		UInt32 result;
		SysNotifyDBInfoType *info = (SysNotifyDBInfoType*)notify->notifyDetailsP;
		
		// notify the app
		err = SysAppLaunch(info->cardNo, info->dbID, 0, 
					sysAppLaunchCmdNotify, (char*)notify, &result);
		
		// return - we're already done!
		return err;
		}
*/	
	
	// Attempt to reserve one of our saved-index variables
	// for this broadcast.  We use these to save the current
	// index into the notifyList as we search it and do broadcasts.
	// This is important because the handler may register/unregister,
	// which changes the the size of the notifyList and can invalidate
	// the index.  To fix this, the reg/unreg routines update the saved
	// indexes if this happens, and we read the index back from our 
	// globals after each broadcast.  We have a stack of 5 of these,
	// so we can support up to 5 levels of broadcast.
	stackIndex = globalsP->nextStackIndex;
	if(stackIndex >= sysNotifyMaxBroadcastDepth)
		{
		// If they are all in use, then we cannot broadcast. 
		return sysNotifyErrBroadcastBusy;
		}
	else
		{
		// increment nextStackIndex var so that if we do a nested broadcast,
		// it will use the next available index variable.
		PrvWriteGlobalsNextStackIndex(globalsP, stackIndex+1);
		}
	
	// clear the 'handled' field in case the user forgot
	notifyP->handled = false;
	
	
	// Find first notification of the correct event type...
	notifyListP = (SysNotifyListEntryType*)MemHandleLock(globalsP->notifyListH);
	
	notifyType = notifyP->notifyType;
	entry.notifyType = notifyType;
	entry.priority = sysNotifyHighestPriority;
	SysBinarySearch(notifyListP, globalsP->numEntries, 
						sizeof(SysNotifyListEntryType), PrvSortCompareFunc, 
						&entry, 0,& bigPos, true);
	pos = bigPos;
	
	// ...and then carry out each notification in order.
	curEntry = &notifyListP[pos];
	while(pos < globalsP->numEntries && curEntry->notifyType == notifyType)
		{ 
		
		// Note that the called app may register, unregister, 
		// or do a deferred broadcast from within the callback.
		// The current position in the array is stored in an element in
		// 'broadcastStack', and the register and unregister routines will 
		// update all indexes in use if the size of the registration list changes.
		
		// Now, we release the memory semaphore during a callback or sublaunch,
		// so a background task could also start calling NotifyMgr routines
		// when we're in the middle of a broadcast.  The position indexes will
		// be updated just as before, and everything should be fine since the semaphore
		// is still used to protect all globals access.  The only potential problem that
		// could arise is that the broadcastStack could get corrupted up in
		// a multiple-broadcast, multiple-thread situation if a broadcast with its index 
		// at broadcastStack[3] finishes before a background broadcast with its index at
		// broadcastStack[4] is finished, then nextStackIndex would contain the wrong value.
		// This could be fixed by treating broadcastStack as a collection of elements which
		// could each be either free or in use at any given time (i.e., its not a stack anymore).
		// This isn't a problem yet, though, since all broadcasts from a background task are 
		// deferred to the UI task.
		
		// increment it now so that we can be assured it won't get
		// decremented below 0 if there are sub-broadcasts.
		PrvWriteGlobalsBroadcastStack(globalsP, stackIndex, pos+1);
		
		// pass registrant's userDataP back to it with the notification
		notifyP->userDataP = curEntry->userDataP;
		
		// If its a callback notification... 
		if(curEntry->callbackP != 0)
			{
			SysNotifyProcPtr callback = curEntry->callbackP;
			
			// Unlock notify list in preparation for callback.
			// This allows us to resize it if the callback registers/unregisters notifications.
			MemHandleUnlock(globalsP->notifyListH);
			notifyListP = NULL;
			
			// Warning: delicate code - see big comment above
			// Release MemoryMgr semaphore to allow BG tasks to run, 
			// don't call PrvReleaseGlobals because its too much extra code.
			MemSemaphoreRelease(false);
			
			err = callback(notifyP);	// call the callback
			
			MemSemaphoreReserve(false);
			
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
			
			}
		else // Otherwise, notify using the launch code mechanism
			{
			
			cardNo = curEntry->cardNo;
			dbID = curEntry->dbID;
			
			// Unlock notify list in preparation for callback.
			// This allows us to resize it if the callback registers/unregisters notifications.
			MemHandleUnlock(globalsP->notifyListH);
			notifyListP = NULL;
			
			// Warning: delicate code - see big comment above
			// Release MemoryMgr semaphore to allow BG tasks to run, 
			// don't call PrvReleaseGlobals because its too much extra code.
			MemSemaphoreRelease(false);
			
			err = SysAppLaunch(cardNo, dbID, 0, 
						sysAppLaunchCmdNotify, (char*)notifyP, &result);
			
			MemSemaphoreReserve(false);
			
			// delete the entry if the data has gone bad:
			if(err && err != memErrNotEnoughSpace && err != sysErrOutOfOwnerIDs &&
				err != sysErrNoFreeRAM && err != sysErrNoFreeResource)
				{
				// delete entry
				PrvDeleteEntry(globalsP, pos);
				}
			
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
				
			}
		
		
		// move on to the next registered notification:
		// read value back from curBroadcast in case it changed
		pos = globalsP->broadcastStack[stackIndex]; // we wrote the incremented value above 
		notifyListP = (SysNotifyListEntryType*)MemHandleLock(globalsP->notifyListH);
		curEntry = &notifyListP[pos];
		}
	
	// done broadcasting - can accept more now, so reset curBroadcast
	PrvWriteGlobalsNextStackIndex(globalsP, stackIndex);
	PrvWriteGlobalsBroadcastStack(globalsP, stackIndex, sysNotifyNoCurBroadcast);
	
	// unlock NotifyList
	if(notifyListP != NULL) MemHandleUnlock(globalsP->notifyListH);

	
	return err;
}

/************************************************************
 * PRIVATE
 *-----------------------------------------------------------
 *
 *  FUNCTION: PrvFindEntry
 *
 *  DESCRIPTION: Find the index of a registration entry in the 
 *				notifyList Using the notifyType, priority, dbID 
 *				and cardNo.
 *				
 *  PARAMETERS: entry	-	Pointer to entry to look for.
 *		
 *		
 *  RETURNS: sysNotifyIllegalListIndex if entry is not found, 
 *				otherwise an array index (0 to numEntries-1)
 *
 *  CREATED: 6/15/98
 *
 *  BY: Jesse Donaldson
 *
 *************************************************************/
static UInt16 PrvFindEntry(SysNotifyGlobalsPtr notifyGlobalsP, SysNotifyListEntryType *entry)
{
	const SysNotifyListEntryType *curEntry;
	UInt16 pos;
	Int32 bigPos; // temp variable to pass to SysBinarySearch
	SysNotifyListEntryType *notifyListP;
	
	
	notifyListP = (SysNotifyListEntryType*)MemHandleLock(notifyGlobalsP->notifyListH);
	
	// Find first registration of the correct event type and priority...
	SysBinarySearch(notifyListP, notifyGlobalsP->numEntries, 
						sizeof(SysNotifyListEntryType), PrvSortCompareFunc, 
						entry, 0, &bigPos, true);
	
	// shrink back to UInt16
	pos = bigPos;
	
	// ...and then check each entry looking for the correct one.
	while(pos < notifyGlobalsP->numEntries)
		{ 
		curEntry = &notifyListP[pos];
		
		// if the priority or event type no longer match, 
		// then we're out of possibilities
		if(curEntry->priority != entry->priority || 
			curEntry->notifyType != entry->notifyType) 
			{
			pos = sysNotifyIllegalListIndex;
			break;
			}
		
		// if the dbID and cardNo are the same, then we've found our entry
		else if(curEntry->dbID == entry->dbID && curEntry->cardNo == entry->cardNo) break;
		
		// Otherwise, check the next entry.
		pos++;
		}
	
	// adjust index if we did not find the entry we were looking for
	if (pos >= notifyGlobalsP->numEntries)
		pos = sysNotifyIllegalListIndex;
	
	// verify index validity:
	ErrNonFatalDisplayIf(pos != sysNotifyIllegalListIndex &&
						 pos >= (Int16)(notifyGlobalsP->numEntries), "out of range");
	
	// Unlock notifyList
	MemHandleUnlock(notifyGlobalsP->notifyListH);
	
	return pos; // return index of found entry, or sysNotifyIllegalListIndex.
}

/************************************************************
 * PRIVATE
 *-----------------------------------------------------------
 *
 *  FUNCTION: PrvInsertEntry
 *
 *  DESCRIPTION: Add the given entry to the list.  
 *				Attempts to grow the array if necessary.
 *
 *	**
 *	WARNING: Calling this function may invalidate other   
 *	**			references to the notifyListH chunk since 
 *				it may attempt to grow the table and this  
 *				might cause the chunk to move in memory.
 *				For this reason, never call the function 
 *				with the handle locked or while maintaining 
 *				a ptr to the chunk.
 *				
 *				
 *  PARAMETERS: 
 *		notifyGlobalsH	- MemPtr to caller's MemPtr to our globals structure
 *							If the globals move, caller's MemPtr is updated.
 *		entry			- MemPtr to a SysNotifyListEntryType to be added to the list.
 *		
 *		
 *  RETURNS: 0 on success, otherwise an error code.
 *
 *  CREATED: 6/15/98
 *
 *  BY: Jesse Donaldson
 *
 *************************************************************/

static Err PrvInsertEntry(SysNotifyGlobalsPtr notifyGlobalsP, SysNotifyListEntryType *entry)
{
	Int16 pos;
	Int32 bigPos; // temp variable to pass to SysBinarySearch
	Err err = 0;
	UInt16 curBroadcast, i;
	SysNotifyListEntryType *notifyListP;
	
	// If we're out of space, we need to grow the registration list.
	// Unfortunately, that means we need to unlock & lock the globals handle,
	// potentially changing the pointer and invalidating other references to it.
	if(notifyGlobalsP->numEntries == notifyGlobalsP->maxEntries) 
		{
/*		MemHandle notifyGlobalsH = GSysNotifyGlobalsH; // safe because we have the semaphore
		
		MemHandleUnlock(notifyGlobalsH);
		
		err = MemHandleResize(notifyGlobalsH, MemHandleSize(notifyGlobalsH) + 
							(sysNotifyResizeTableBy * sizeof(SysNotifyListEntryType)));
		
		notifyGlobalsP = MemHandleLock(notifyGlobalsH);
		*notifyGlobalsPP = notifyGlobalsP; // update caller's copy of MemPtr
*/		
		MemHandle notifyListH = notifyGlobalsP->notifyListH;
		
		// EC code: ensure notifyList is unlocked.
		ErrNonFatalDisplayIf(MemHandleLockCount(notifyListH) != 0, "notifyList locked!");
		
		err = MemHandleResize(notifyListH, MemHandleSize(notifyListH) + 
							(sysNotifyResizeTableBy * sizeof(SysNotifyListEntryType)));
		if(err) return err; // on error, return
		
		// Otherwise, note that we have more space in the table.
		PrvWriteGlobalsMaxEntries(notifyGlobalsP, 
				notifyGlobalsP->maxEntries + sysNotifyResizeTableBy);
		}
	
	notifyListP = MemHandleLock(notifyGlobalsP->notifyListH);
	
	// Find where to insert the element
	SysBinarySearch(notifyListP, notifyGlobalsP->numEntries, 
						sizeof(SysNotifyListEntryType), PrvSortCompareFunc, 
						entry, 0, &bigPos, false);
	pos = bigPos;
	
	// move some records down a slot to make space at index 'pos'
	err = DmWrite(notifyListP, sizeof(SysNotifyListEntryType) * (pos+1), 
					&notifyListP[pos],
					sizeof(SysNotifyListEntryType)*(notifyGlobalsP->numEntries - pos));
	
	// fill in the new entry!
	err = DmWrite(notifyListP, sizeof(SysNotifyListEntryType) * (pos), 
				entry, sizeof(SysNotifyListEntryType));
	
/*
#else
	MemMove(&notifyGlobalsP->notifyList[(UInt16)pos+1], &notifyGlobalsP->notifyList[(UInt16)pos], 
				sizeof(SysNotifyListEntryType)*(notifyGlobalsP->numEntries - (UInt16)pos));
	MemMove(&notifyGlobalsP->notifyList[(UInt16)pos], entry, sizeof(SysNotifyListEntryType));
#endif
*/
	// increment numEntries since we have one more now
	PrvWriteGlobalsNumEntries(notifyGlobalsP, notifyGlobalsP->numEntries + 1);
	
	// If there is a broadcast in progress, the current
	// array element may need to be fixed up.
	for(i=0; i<notifyGlobalsP->nextStackIndex; i++)
	{
		curBroadcast = notifyGlobalsP->broadcastStack[i];
		ErrNonFatalDisplayIf(curBroadcast == sysNotifyNoCurBroadcast, 
						"bad bcast stack");
		
		if(curBroadcast > pos)
			{
			PrvWriteGlobalsBroadcastStack(notifyGlobalsP, i, curBroadcast + 1);
			}
	}
	
	// unlock notifyList before returning
	MemHandleUnlock(notifyGlobalsP->notifyListH);
	
	return err;
}


/************************************************************
 * PRIVATE
 *-----------------------------------------------------------
 *
 *  FUNCTION: PrvDeleteEntry
 *
 *  DESCRIPTION: Does the actual removal of the entry from the list.
 *				Will shrink the list if the removal results
 *				in MORE THAN sysNotifyResizeTableBy free entries,
 *				and will update the saved index stack if there 
 *				is a broadcast in progress.
 *				
 *				NOTE: This will NEVER move our globals and invalidate 
 *				other references to them because we only ever shrink 
 *				the list, so we can keep the chunk locked.
 *				
 *				WARNING: Calling this routine will move things around
 *				in the notifyList.  So, if you are traversing it, you 
 *				will have to update your index variable after you call this.
 *				
 *  PARAMETERS: 
 *		notifyGlobalsP	- MemPtr to NotifyMgr globals
 *		entry			-	Pointer to the SysNotifyListEntryType to be removed.
 *		
 *		
 *  RETURNS: 0 on success, otherwise an error code.
 *
 *  CREATED: 6/15/98
 *
 *  BY: Jesse Donaldson
 *
 *************************************************************/
Err PrvDeleteEntry(SysNotifyGlobalsPtr notifyGlobalsP, UInt16 pos)
{
	Err err = 0;
	UInt16 curBroadcast, i;
	SysNotifyListEntryType *notifyListP;
	

	// make sure pos is in range
	if (pos >= notifyGlobalsP->numEntries)
		return(sysNotifyErrEntryNotFound);
	
	notifyListP = MemHandleLock(notifyGlobalsP->notifyListH);
	
	// move the tail end of the array over the entry being removed.
	// This basically moves the unused entry to the end
	err = DmWrite(notifyListP, sizeof(SysNotifyListEntryType) * (pos), 
			&notifyListP[pos+1],
			sizeof(SysNotifyListEntryType) * (notifyGlobalsP->numEntries - pos - 1));
	
/*
#else
	MemMove(&notifyGlobalsP->notifyList[(UInt16)pos], &notifyGlobalsP->notifyList[(UInt16)pos+1],
			sizeof(SysNotifyListEntryType) * (notifyGlobalsP->numEntries - (UInt16)pos - 1));
#endif
*/
	
	// one less entry on the list...
	PrvWriteGlobalsNumEntries(notifyGlobalsP, notifyGlobalsP->numEntries - 1);
	
	// If there is a broadcast in progress, the saved
	// index of the current array element may need 
	// to be fixed up.  Move it back one if the entry
	// we removed is before it.
	for(i=0; i<notifyGlobalsP->nextStackIndex; i++)
	{
		curBroadcast = notifyGlobalsP->broadcastStack[i];
		ErrNonFatalDisplayIf(curBroadcast == sysNotifyNoCurBroadcast, 
						"bad bcast stack");
		
		if(curBroadcast > pos)
			{
			PrvWriteGlobalsBroadcastStack(notifyGlobalsP, i, curBroadcast - 1);
			}
	}
	
	
	// Now, if we have more than sysNotifyResizeTableBy free entries, 
	// then its time to shrink the array.  Luckily, this does not
	// require unlocking it so other references to the globalsP 
	// will still be valid.
	if(notifyGlobalsP->maxEntries > sysNotifyListMinSize && 
		notifyGlobalsP->maxEntries - notifyGlobalsP->numEntries > sysNotifyResizeTableBy)
		{
//		MemHandle notifyGlobalsH = GSysNotifyGlobalsH; // safe because we have the semaphore
		
		// Everything is at the beginning, so we can just shrink the chunk...
		err = MemPtrResize(notifyListP, MemPtrSize(notifyListP) - 
							(sysNotifyResizeTableBy * sizeof(SysNotifyListEntryType)));
		
		ErrNonFatalDisplayIf(err, "notifyList shrink failed");
		
		// ... and update maxEntries.
		PrvWriteGlobalsMaxEntries(notifyGlobalsP, 
				notifyGlobalsP->maxEntries - sysNotifyResizeTableBy);
		}
	
	// unlock notifyList before returning
	MemHandleUnlock(notifyGlobalsP->notifyListH);

	return err;
}

/************************************************************
 * PRIVATE
 *-----------------------------------------------------------
 *
 *  FUNCTION: PrvDeferredQueueInsert
 *
 *  DESCRIPTION: Insert a deferred notify entry into the queue.
 *				
 *				
 *  PARAMETERS: globalsP-	MemPtr to notify globals
 *				taskID	-	0, or the task ID of thetask to be 
 *							woken after the broadcast is completed.
 *				notifyP	-	MemPtr to SysNotifyParamType structure 
 *							to be broadcast.
 *		
 *		
 *  RETURNS: 0 on success, an Err otherwise.
 *
 *  REVISION HISTORY:
 *  jed	6/26/98	initial revision
 *  jed	7/01/00	Modifications for new validFlags field
 *
 *************************************************************/

Err PrvDeferredQueueInsert(SysNotifyGlobalsPtr globalsP, UInt32 taskID, SysNotifyParamType *notify, UInt8 validFlags)
{
	const SysNotifyDeferredQueueType *queue = &globalsP->deferredQueue;
	SysNotifyDeferredQueueEntryType newEntry;
	UInt16 index, temp; 
	Err err;
	
	
	// if the queue is full, return an error:
	if(queue->head == queue->tail && (queue->entry[0].validFlags & notifyValidEntry))
		{
		err = sysNotifyErrQueueFull;
		}
	else
		{
		// get index of what we hope is the 1st empty entry:
		index = queue->tail;
		
		// make sure the entry isn't already valid
		ErrNonFatalDisplayIf((queue->entry[index].validFlags & notifyValidEntry), "Queue problem");
		
		// fill out the entry
		newEntry.validFlags = notifyValidEntry | validFlags;
		newEntry.taskID = taskID;
		newEntry.notify = *notify;
		PrvWriteGlobalsQueueEntry(globalsP, index, &newEntry);
		
		// increment the queue tail:
		temp = queue->tail + 1;
		if(temp >= sysNotifyDefaultQueueSize) temp = 0;
		PrvWriteGlobalsQueueTail(globalsP, temp);
		
		// there is at least one pending deferred proadcast, since we just enqueued one.
		GSysNotifyBroadcastPending = true;
		
		err = 0;
		}
	
	return err;
}


/************************************************************
 * PRIVATE
 *-----------------------------------------------------------
 *
 *  FUNCTION: PrvDeferredQueueRemove
 *
 *  DESCRIPTION: Remove a deferred notify entry from the queue.
 *				
 *  PARAMETERS: globalsP ->	ptr to notify globals
 *				outEntry <-	ptr to SysNotifyParamType structure 
 *							to be broadcast.
 *		
 *  RETURNS: 0 on success, sysNotifyErrQueueEmpty if no entries otherwise.
 *
 *  HISTORY:
 *		6/26/98		jesse	initial revision
 *		3/14/00		bob		renamed to avoid confusion with interrupt queue
 *		7/01/00		jesse	Modifications for new validFlags field
 *
 *************************************************************/
Err PrvDeferredQueueRemove(SysNotifyGlobalsPtr globalsP, SysNotifyDeferredQueueEntryType *outEntry)
{
	const SysNotifyDeferredQueueType *queue = &globalsP->deferredQueue;
	SysNotifyDeferredQueueEntryType entry;
	UInt16 index, temp;
	Err err;
	
	// get what we hope is the 1st full entry:
	index= queue->head;
	entry = queue->entry[index];
	
	// if the queue is empty, return an error:
	if(queue->head == queue->tail && ((entry.validFlags & notifyValidEntry) == false))
		{
		err = sysNotifyErrQueueEmpty;
		}
	else
		{
		// make sure the entry is valid
//		ErrNonFatalDisplayIf(entry.valid == false, "Queue problem");
		
		*outEntry = entry;
		
		// invalidate the entry
		entry.validFlags = 0x00;
		PrvWriteGlobalsQueueEntry(globalsP, index, &entry);
		
		// increment the queue head:
		temp = queue->head + 1;
		if(temp >= sysNotifyDefaultQueueSize) temp = 0;
		PrvWriteGlobalsQueueHead(globalsP, temp);

		err = 0;
		}
	
	// if the queue isn't empty, set the flag back to TRUE so we come through
	// to SysNotifyBroadcast(0) again.
	if (queue->head != queue->tail || (queue->entry[index].validFlags & notifyValidEntry))
		GSysNotifyBroadcastPending = true;

	return err;
}



/************************************************************
 * PRIVATE
 *-----------------------------------------------------------
 *
 *  FUNCTION: PrvInterruptQueueRemove
 *
 *  DESCRIPTION: Remove a interrupt notify entry from the queue.
 *				
 *  PARAMETERS: outEntry <-	ptr to SysNotifyParamType structure 
 *							to be broadcast.
 *		
 *  RETURNS: 0 on success, sysNotifyErrQueueEmpty if no items.
 *
 *  HISTORY:
 *		3/14/00		bob		initial revision
 *		7/01/00		jesse		Modifications for new validFlags field
 *
 *************************************************************/
Err PrvInterruptQueueRemove(SysNotifyGlobalsPtr globalsP, SysNotifyDeferredQueueEntryType *outEntry)
{
	KeyQueuePtr	keyQP = (KeyQueuePtr)GSysNotifyInterruptGlobalsP;
	SysNotifyParamType *notifyParamsP;
	UInt8 bytes[sizeof(SysNotifyInterruptQueueElementType)];
	
	register UInt8 *dataP;
	register Int16 size;
	register UInt16 start;
	register Int16 i;

	// Is there any data in the queue???
	start = keyQP->start;
	if (start == keyQP->end)
		return sysNotifyErrQueueEmpty;
	
	// Set up queue locals
	dataP = keyQP->data + start;
	size = keyQP->size;

	// copy the data from the interrupt queue to a buffer
	for (i = 0; i < sizeof(SysNotifyInterruptQueueElementType); i++) {
		bytes[i] = *dataP++;
		if (++start == size)
			{start = 0; dataP = keyQP->data;}
	}
	keyQP->start = start;

	// set up the return struct
	outEntry->taskID = 0;
	outEntry->validFlags = notifyValidEntry; // NOTE: do NOT set the notifyValidDetailsP bit so that we don't try to dispose of the ptr...
	notifyParamsP = &(outEntry->notify);
	
	// copy the data from the buffer to the return struct
	notifyParamsP->notifyType = ((SysNotifyInterruptQueueElementType *)bytes)->notifyType;
	notifyParamsP->broadcaster = ((SysNotifyInterruptQueueElementType *)bytes)->broadcaster;
	notifyParamsP->notifyDetailsP = (void *)(((SysNotifyInterruptQueueElementType *)bytes)->notifyDetailsP);
	notifyParamsP->userDataP = NULL;
	notifyParamsP->handled = false;
	
	// if the queue isn't empty, set the flag back to TRUE so we come through
	// to SysNotifyBroadcast(0) again.
	if (keyQP->start != keyQP->end)
		GSysNotifyBroadcastPending = true;
	else
	{
		// If the interrupt queue is empty, check the deferred queue, and possibly set the flag.
		const SysNotifyDeferredQueueType *queue = &globalsP->deferredQueue;
		if (queue->head != queue->tail || (queue->entry[queue->head].validFlags & notifyValidEntry))
		{
			GSysNotifyBroadcastPending = true;
		}
	}

	return 0;	// no error
}



/************************************************************
 * PRIVATE
 *-----------------------------------------------------------
 *
 *  FUNCTION: PrvSearchQueueForTaskID
 *
 *  DESCRIPTION: Search the deferred queue for an entry marked 
 *				with the given taskID. This is used by 
 *				SysNotifyBroadcast to determine if a deferred
 *				broadcast has really been handled (so its okay 
 *				to stay awake, or if it has to go back to sleep).
 *				
 *  PARAMETERS: globalsP-	MemPtr to notify globals
 *				taskID	-	task ID of the entry to look for.
 *		
 *		
 *  RETURNS: true if found, false otherwise
 *
 *  CREATED: 6/26/98
 *
 *  BY: Jesse Donaldson
 *
 *************************************************************/

Boolean PrvSearchQueueForTaskID(SysNotifyGlobalsPtr globalsP, UInt32 taskID)
{
	UInt16 i, end;
	const SysNotifyDeferredQueueEntryType *entry;
	
	end = globalsP->deferredQueue.tail;
	i=globalsP->deferredQueue.head;
	
	if(i == end)
	{
	entry = &globalsP->deferredQueue.entry[i];
	
	// if queue is empty, then the entry is not there
	if((entry->validFlags & notifyValidEntry) == false) return false;
	
	// if the 1st entry is IT, return now
	if(entry->taskID == taskID) return true;
	
	// otherwise, bump the index up so we actually execute the loop below
	i++;
	if(i >= sysNotifyDefaultQueueSize) i=0;
	
	}
	
	while(i != end)
		{
		entry = &globalsP->deferredQueue.entry[i];
		if(entry->taskID == taskID) return true;
		
		i++;
		if(i >= sysNotifyDefaultQueueSize) i=0;
		}
	
	return false;
}

/************************************************************
 * PRIVATE
 *-----------------------------------------------------------
 *
 *  FUNCTION: PrvHandleDatabaseDeleted
 *
 *  DESCRIPTION: Remove all of a database's notifications 
 *				from the list prior to its being deleted.  We just 
 *				look through the list and delete anything with the 
 *				given Local ID.
 *				
 *  PARAMETERS: dbID	-	LocalID of the database being deleted.
 *				cardNo	-	card number of DB being deleted.
 *				globalsP-	MemPtr to NotifyMgr globals.
 *		
 *  RETURNS: nothing
 *
 *  CREATED: 6/26/98
 *
 *  BY: Jesse Donaldson
 *
 *************************************************************/

void PrvHandleDatabaseDeleted(SysNotifyGlobalsPtr globalsP, UInt16 cardNo, LocalID dbID)
{
	const SysNotifyListEntryType *entry, *notifyListP;
	UInt16 pos;
	
	notifyListP = (SysNotifyListEntryType*)MemHandleLock(globalsP->notifyListH);
	
	// go through list, remove each entry.
	pos = 0;
	while(pos < globalsP->numEntries)
		{
		entry = &notifyListP[pos];
		
		if(entry->dbID == dbID && entry->cardNo == cardNo)
			{
			// PrvDeleteEntry will not move our globals so globalsP remains valid...
			PrvDeleteEntry(globalsP, pos);
			
			}
		else	// only increment 'pos' if we didn't delete the entry we're at.
			{	// (because if we did, 'pos' already points to the next one).
			pos++;
			}
		
		}
	
	// unlock notifyList now that we're done with it
	MemHandleUnlock(globalsP->notifyListH);
	
	return;
}


/************************************************************
 * PRIVATE
 *-----------------------------------------------------------
 *
 *  FUNCTION: PrvSortCompareFunc
 *
 *  DESCRIPTION: Helper function for SysBinarySearch used for determining
 *				the index at which to insert new entries so we can 
 *				keep the list sorted by notifyType & priority.
 *				
 *				
 *  PARAMETERS: dataP		-	Pointer to a new entry to find the slot for
 *				curEntryP	-	Pointer to the current array entry to compare it to
 *				other		-	Extra application defined data. In this case, 0.
 *				
 *		
 *  RETURNS:	0 if entries are equal, 
 *				<0 if new one is "larger", 
 *				>0 if new one is "smaller".
 *
 *  CREATED: 6/15/98
 *
 *  BY: Jesse Donaldson
 *
 *************************************************************/
static Int16 PrvSortCompareFunc (void const * dataP, void const * curEntryP, Int32 /* other */)
{
	UInt32 dataType, curEntryType;
	
	dataType = ((SysNotifyListEntryType*)dataP)->notifyType;
	curEntryType = ((SysNotifyListEntryType*)curEntryP)->notifyType;
	
	// need to make diff a 2-byte value with same sign since return type is Int16
	if(curEntryType > dataType ) return -1;
	else if(curEntryType < dataType) return 1;
			
			// okay since priority is only 1 byte
	return (((SysNotifyListEntryType*)dataP)->priority - 
			((SysNotifyListEntryType*)curEntryP)->priority);
}



/************************************************************
 * PRIVATE
 *-----------------------------------------------------------
 *
 *  FUNCTION: PrvValidateEntry
 *
 *  DESCRIPTION: Performs a little bit of validation for a
 *				notification entry.  Simply checks to make 
 *				sure the database exists, or if there is none,
 *				that the pointer is non-null.
 *				
 *				Note that we don't verify thsi stuff under 
 *				the simulator 'cause its tricky.
 *				
 *  PARAMETERS: entry	-	Pointer to SysNotifyListEntryType structure to validate
 *		
 *		
 *  RETURNS: 0 if entry passes tests, otherwise an error code
 *
 *  CREATED: 6/15/98
 *
 *  BY: Jesse Donaldson
 *
 *************************************************************/

#if EMULATION_LEVEL == EMULATION_NONE
static Err PrvValidateEntry(SysNotifyListEntryType *entry)
{
	if(entry->dbID == sysNotifyNoDatabaseID && entry->callbackP == 0)
		{
		return sysErrParamErr;
		}
	
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	else if(entry->dbID == 0) return sysErrParamErr;
#endif
	
	return errNone;
}

#else

static Err PrvValidateEntry(SysNotifyListEntryType *)
{	
	return errNone;
}

#endif


/************************************************************
 * PRIVATE
 *-----------------------------------------------------------
 *
 *  FUNCTION: PrvValidateGlobals
 *
 *  DESCRIPTION: Performs a bit of validation for our global
 *				data chunk.  Makes sure all the numbers are sane,
 *				and walks the list verifying sort order and 
 *				uniqueness (at least for consecutive items).
 *				
 *  PARAMETERS: globalsP - pointer to Notify Manager globals
 *		
 *		
 *  RETURNS: nothing
 *
 *  CREATED: 6/15/98
 *
 *  BY: Jesse Donaldson
 *
 *************************************************************/

#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
static void PrvValidateGlobals(SysNotifyGlobalsPtr globalsP)
{
	UInt16 	i;
	UInt32	notifyType = 0;
	Int8	priority = sysNotifyHighestPriority;
	LocalID	dbID = 0;
	UInt16	cardNo = 0;
	UInt16	nextIndex;
	UInt32	size, expected;
	const SysNotifyDeferredQueueType *queue = &globalsP->deferredQueue;
	char	str[48];
	SysNotifyListEntryType *notifyListP;
	
	// check the queue:
	
	ErrNonFatalDisplayIf(queue->head >= sysNotifyDefaultQueueSize || 
						queue->tail >= sysNotifyDefaultQueueSize, 
							"bad queue header");
	
	i = queue->head;
	while(i != queue->tail)
		{
		ErrNonFatalDisplayIf((queue->entry[i].validFlags & notifyValidEntry) == false, "bad queue entry");
		
		i++;
		if(i >= sysNotifyDefaultQueueSize) i = 0;
		}
	
	i = queue->tail;
	while(i != queue->head)
		{
		ErrNonFatalDisplayIf((queue->entry[i].validFlags & notifyValidEntry) == true, "bad queue entry");
		
		i++;
		if(i >= sysNotifyDefaultQueueSize) i = 0;
		}
	
	if(queue->head == queue->tail)
		{
		for(i = 1; i < sysNotifyDefaultQueueSize; i++)
			{
			ErrNonFatalDisplayIf((queue->entry[i].validFlags & notifyValidEntry) != (queue->entry[0].validFlags & notifyValidEntry), 
								"bad queue entry");
			}
		}
	
	
	
	// make sure we've not overflowed anything
	ErrNonFatalDisplayIf(globalsP->numEntries > globalsP->maxEntries,
						"Too many entries.");
	
	// make sure notifyList chunk is correct size
	size = MemHandleSize(globalsP->notifyListH);
	expected = (sizeof(SysNotifyListEntryType) * (globalsP->maxEntries));
	if(size != expected)
		{
		StrPrintF(str, "wrong size- exp:%ld, act:%ld", expected, size);
		ErrFatalDisplay(str);
		}
	
	// Check the broadcast stack
	nextIndex = globalsP->nextStackIndex;
	ErrNonFatalDisplayIf(nextIndex > sysNotifyMaxBroadcastDepth, "nextIndex bad");
	ErrNonFatalDisplayIf(nextIndex < sysNotifyMaxBroadcastDepth &&
						globalsP->broadcastStack[nextIndex] != sysNotifyNoCurBroadcast,
						"next index bad");
	
	
	notifyListP = (SysNotifyListEntryType*)MemHandleLock(globalsP->notifyListH);
	// search array checking sort state & consecutive duplicates
	for(i=0; i+1<globalsP->numEntries; i++)
		{
		const SysNotifyListEntryType *curEntry1 = &notifyListP[i];
		const SysNotifyListEntryType *curEntry2 = &notifyListP[i+1];
		Int16 result = PrvSortCompareFunc(curEntry1, curEntry2, 0);
		
		ErrNonFatalDisplayIf(result > 0, "list out of order");
		if(curEntry1->dbID != sysNotifyNoDatabaseID)
			ErrNonFatalDisplayIf(result==0 && curEntry1->dbID == curEntry2->dbID, "duplicate in list");
		
		}
	
	MemHandleUnlock(globalsP->notifyListH);
	
	return;
}
#endif // ERROR_CHECK_LEVEL == ERROR_CHECK_FULL

/************************************************************
 * PRIVATE
 *-----------------------------------------------------------
 *
 *  FUNCTION: PrvGetGlobals
 *
 *  DESCRIPTION: Call this to obtain 'ownership' of the Notify
 *				Manager global data chunk.  This is necessary
 *				because it may be called from background tasks.
 *				This routine reserves the memory semaphore, 
 *				Locks a MemHandle to the globals and stuffs the
 *				pointer into the passed address.
 *				
 *	NOTE: The publicly accessible functions will call PrvGetGlobals 
 *		and PrvReleaseGlobals, passing a globals pointer to 
 *		subroutines.  Private routines will not call these directly.
 *		This is to help ensure that they will not be called more
 *		than necessary (there is usually just one public entrypoint
 *		but a number of private functions may be called).
 *		
 *  PARAMETERS: globalsPP - MemPtr to variable to store pointer 
 *							to Notify Manager globals
 *		
 *		
 *  RETURNS: nothing
 *
 *  CREATED: 6/15/98
 *
 *  BY: Jesse Donaldson
 *
 *************************************************************/
static Err PrvGetGlobals(SysNotifyGlobalsPtr *globalsPP)
{
	Err err;
	MemHandle notifyGlobalsH;
	SysNotifyGlobalsPtr notifyGlobalsP;
	
	
	// First thing we do is reserve the semaphore...
	err = MemSemaphoreReserve(false);
	ErrNonFatalDisplayIf(err, "need semaphore.");
	if(err != errNone) return err;
	
	ErrNonFatalDisplayIf(GSysNotifyGlobalsH == 0, "not initialized.");
	if(GSysNotifyGlobalsH == 0)
		{
		*globalsPP = 0;
		MemSemaphoreRelease(false);
		return sysErrNotInitialized;
		}
	
	// Then, get the MemHandle & lock it.
	notifyGlobalsH = GSysNotifyGlobalsH;
	notifyGlobalsP = MemHandleLock(notifyGlobalsH);
	
	ErrNonFatalDisplayIf(MemHandleLockCount(GSysNotifyGlobalsH) != 
						notifyGlobalsP->nextStackIndex+1, 
						"bad lock count");
	
	// Verify globals integrity
	PrvValidateGlobals(notifyGlobalsP);
	
	// and 'return' the pointer and an error code.
	*globalsPP = notifyGlobalsP;
	
	return errNone;
}

/************************************************************
 * PRIVATE
 *-----------------------------------------------------------
 *
 *  FUNCTION: PrvReleaseGlobals
 *
 *  DESCRIPTION: Call this to release 'ownership' of the Notify
 *				Manager global data chunk.  This is the counter
 *				call to PrvGetGlobals.  It releases the memory 
 *				semaphore, and unlocks the MemHandle to the globals.
 *				
 *	NOTE: The publicly accessible functions will call PrvGetGlobals 
 *		and PrvReleaseGlobals, passing a globals pointer to 
 *		subroutines.  Private routines will not call these directly.
 *		This is to help ensure that they will not be called more
 *		than necessary (there is usually just one public entrypoint
 *		but a number of private functions may be called).
 *		
 *  PARAMETERS: none.
 *		
 *		
 *  RETURNS: nothing
 *
 *  CREATED: 6/15/98
 *
 *  BY: Jesse Donaldson
 *
 *************************************************************/
static void PrvReleaseGlobals(SysNotifyGlobalsPtr globalsP)
{
	MemHandle notifyGlobalsH = GSysNotifyGlobalsH;
	Err err = 0;
	
	// we should have globals access here, because 
	// our getting here implies that PrvGetGlobals succeeded.
	ErrFatalDisplayIf(GSysNotifyGlobalsH == 0, "no globals");
	
	ErrNonFatalDisplayIf(MemHandleLockCount(GSysNotifyGlobalsH) != 
						globalsP->nextStackIndex+1, 
						"bad lock count");

	// Validate the globals:
	PrvValidateGlobals(globalsP);
	
	// Unlock the globals:
	err = MemHandleUnlock(notifyGlobalsH);
	ErrNonFatalDisplayIf(err, "unlock failed");
	
	// Release the semaphore before we return.
	err = MemSemaphoreRelease(false);
	ErrNonFatalDisplayIf(err, "semaphore err");
	
	return;
}


/************************************************************
 * PRIVATE
 *-----------------------------------------------------------
 *
 *  FUNCTION: PrvAreWeUIThread
 *
 *  DESCRIPTION: Determine if the currently running task is the 
 *				UI thread.
 *				
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
 *				
 *  PARAMETERS: none
 *		
 *		
 *  RETURNS: true if we're the UI thread, false if a bg task
 *
 *  CREATED: 6/15/98
 *
 *  BY: Jesse Donaldson
 *
 *************************************************************/

static Boolean PrvAreWeUIThread(void)
{
	// under emulation we're always running in the 
	// UI thread and XXXXXXXXXXXXX doesn't work.
#if EMULATION_LEVEL != EMULATION_NONE
	return true;
#else
	// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
	Err err;
	
	// First determine whether or not we are 
	// being called from the UI thread:
	// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
	// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
	ErrNonFatalDisplayIf(err, "Need KernelInfo");
	
	// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
	// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
	// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
	// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
#endif
}


/************************************************************
 * PRIVATE
 *-----------------------------------------------------------
 *
 *  FUNCTION: PrvCheckStackSpace
 *
 *  DESCRIPTION: Verify that we have enough stack space to do 
 *				a broadcast via SysAppLaunch.
 *				
 *  PARAMETERS: none
 *		
 *  RETURNS: 0 if there is enough stack space, or
 * 			sysNotifyNoStackSpace otherwise.
 *
 *  CREATED: 6/15/98
 *
 *  BY: Jesse Donaldson
 *
 *************************************************************/

#if EMULATION_LEVEL != EMULATION_NONE

// can't check under emulation mode
Err PrvCheckStackSpace(void)
{return errNone;}

#else

Err PrvCheckStackSpace(void)
{
	UInt8 *sp;
	UInt8 *start, *end;
	Int32 freestack;
	Boolean valid;
	
	// Use address of local variable to get approximate location of current stack pointer.
	// This works on any platform, and so is better than calling an asm routine.
	sp = &valid; 
	valid = SysGetStackInfo(&start, &end);
	
//	ErrFatalDisplayIf(valid == false, "Invalid stack");
	ErrFatalDisplayIf(sp > end, "Stack overflow");
	
	freestack = sp - start;
	
	if(freestack < sysNotifyMinStackSpaceForBroadcast) return sysNotifyErrNoStackSpace;
	return errNone;
}

/************************************************************
 * PRIVATE
 *-----------------------------------------------------------
 *
 *  FUNCTION: PrvGetSP
 *
 *  DESCRIPTION: Get the current stack pointer
 *				
 *  PARAMETERS: none
 *		
 *  RETURNS: stack pointer (in a0)
 *
 *  CREATED: 6/15/98
 *
 *  BY: Jesse Donaldson
 *
 *************************************************************/
/*
asm MemPtr PrvGetSP(void)
{
	movea.l   sp,a0		// get ready to return stack pointer...
	rts					// ... and then do it!
}
*/


#endif
