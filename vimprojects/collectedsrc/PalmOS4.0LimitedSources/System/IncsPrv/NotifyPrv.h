/******************************************************************************
 *
 * Copyright (c) 1998-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: NotifyPrv.h
 *
 * Release: 
 *
 * Description:
 *		Private include file for Notification Manager
 *
 * History:
 *   	2/5/99  JED - Created by Jesse Donaldson
 *
 *****************************************************************************/

#ifndef	__NOTIFYPRV_H__
#define	__NOTIFYPRV_H__

#include <PalmTypes.h>
#include <NotifyMgr.h>

#ifdef __cplusplus
extern "C" {
#endif


// WARNING: For system use only:
extern Err SysNotifyInit(void)
				SYS_TRAP(sysTrapSysNotifyInit);
// WARNING: For system use only:
extern void SysNotifyDatabaseAdded(UInt16 cardNo, LocalID dbID)
				SYS_TRAP(sysTrapSysNotifyDatabaseAdded);

// WARNING: For system use only:
extern void SysNotifyDatabaseRemoved(UInt16 cardNo, LocalID dbID)
				SYS_TRAP(sysTrapSysNotifyDatabaseRemoved);


#ifdef __cplusplus 
}
#endif

//	These structures are really private to NotifyMgr.c but we place it here because
//	ShellCmdUtil.cpp needs to know about SysNotifyGlobalType in order to properly 
//  label notifyListH in the storage heap.
#define sysNotifyMaxBroadcastDepth	5 // can do broadcast 5 levels deep

/*
	The queue into which we put all the deferred notifications
	has elements onf this type:
*/
#define notifyValidEntry	0x01	// This entry in the queue is currently in use.
#define notifyValidDetailsP	0x02	// notify.notifyDetailsP contains a ptr to a valid MmeoryMgr chunk which should be automatically disposed of when the notification is broadcast.

typedef struct SysNotifyDeferredQueueEntryTag
	{
	UInt32				taskID;		// 0, or taskID of task to wakeup after broadcast
	SysNotifyParamType	notify;		// notification to broadcast
	UInt8				validFlags;	// true if entry is valid
	UInt8				reserved;	
	}SysNotifyDeferredQueueEntryType;


typedef struct SysNotifyDeferredQueueType
	{
		// Note that when head and tail are equal, the queue
		// is either empty or full depending on 'valid' flag of entry.
	UInt16	head;	// index of 1st valid entry.
	UInt16	tail;	// index of first empty entry 
//	Int16	size;	// number of entries in queue
	
	SysNotifyDeferredQueueEntryType entry[sysNotifyDefaultQueueSize];
	
	} SysNotifyDeferredQueueType;


/*
	This structure contains our globals and list of notification entries.
	It is kept in the static heap as a resource in the unsaved prefs database.
*/
typedef struct SysNotifyGlobalsType
	{
	SysNotifyDeferredQueueType deferredQueue;
	
	// Make everything const so we aren't tempted to write to it
	UInt16	numEntries;		// number of notification entries in the list
	UInt16	maxEntries;		// max # of entries in 'notifyList'

	// stack to keep track of current element 
	// positions in the notifyList during nested broadcasts.
	UInt16 nextStackIndex;
	UInt16 broadcastStack[sysNotifyMaxBroadcastDepth];
	
	Boolean	uiRunning;		// set when system has finished booting.  
							// Until this is set, all broadcasts are truly 
							// synchronous - no magic deferral to the UI task.
	UInt8	unused;
	
	// the list of entries - This is a variable length field.
	MemHandle	notifyListH;	// handle to array of SysNotifyListEntryType in the storage heap
	} SysNotifyGlobalsType, *SysNotifyGlobalsPtr;



// This structure is used by interrupts to enqueue notifications on a
// special keyQueue-like queue.

typedef struct {
		UInt32 notifyType;
		UInt32 broadcaster;
		UInt32 notifyDetailsP;
	} SysNotifyInterruptQueueElementType;

#define SysNotifyInterruptQueueMaxElements 4

// want one extra byte here so that we don't waste too much space.
// (because start can never be the same as end when the queue is full
//  we have to sacrifice a byte for pad.)
#define SysNotifyInterruptQueueDataSize (SysNotifyInterruptQueueMaxElements * sizeof(SysNotifyInterruptQueueElementType) + 1)

#endif	// __NOTIFYPRV_H__
