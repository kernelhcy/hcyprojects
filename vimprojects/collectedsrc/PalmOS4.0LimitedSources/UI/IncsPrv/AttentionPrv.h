/******************************************************************************
 *
 * Copyright (c) 2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: AttentionPrv.h
 *
 * Release: 
 *
 * Description:
 * 	This file defines structs and functions for the Attention Manager
 *		that are private to Palm OS but are not private to the
 *		Attention Manager itself -- that is, we don't want developers
 *		using these things, but Managers other than the Attention Manager
 *		can use them.
 *
 * History:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	07/14/00	Initial Revision
 *			gap	07/21/00	Change parameter list and data structures to support
 *								specification of card number as well as dbID.
 *			peter	07/24/00	Add nag and sound stuff directly to AttentionType;
 *			gap	08/14/00	Implement FirstTime.
 *
 *****************************************************************************/

#ifndef __ATTENTIONPRV_H__
#define __ATTENTIONPRV_H__

#include <PalmTypes.h>
#include <CoreTraps.h>
#include <DateTime.h>

#include "AttentionMgr.h"

// Blink patterns supported for indicator.
#define kAttnIndicatorBlinkPatternNone		0
#define kAttnIndicatorBlinkPatternNew		1
#define kAttnIndicatorBlinkPatternOld		2

// LED blink options.
typedef enum {
	attnLEDInitial,			// LED is blinking actively
	attnLEDContinuous,		// LED is continuing to blink in power saving mode
	attnLEDOff					// LED is done blinking
	} AttnLEDMode;

// Snooze is always for 5 minutes.
#define attnMgrSnoozeDuration			300

// Results of AttnEffectOfEvent() call.
#define attnNoDialog						0
#define attnDetailsDialog				1
#define attnListDialog					2

/********************************************************************
 * Private Attention Manager Structures
 ********************************************************************/

typedef struct {
	UInt16				cardNo;
	LocalID				dbID;
	UInt32				userData;
	AttnCallbackProc *callbackFnP;
	AttnLevelType		level;
	AttnFlagsType		flags;
	UInt16				nagRateInSeconds;
	UInt16				nagRepeatLimit;
	UInt32				lastNagTime;
	UInt16				nagsRemaining;
	UInt16				isNew					: 1;	// Was it added by AttnGetAttention?
	UInt16				isUpdated			: 1;	// Was it updated by AttnUpdate?
	UInt16				isDeleted			: 1;	// Was it removed by AttnForgetIt?
	UInt16				isSnoozed			: 1;	// Was it present when Snooze was hit?
	UInt16				isFirstTime			: 1;	// Is this the first time the item will be displayed?
	UInt16				getUsersAttention	: 1;	// Do special effects need to be performed?
	UInt16				isDemoted			: 1;	// Was it present when Done was hit?
	UInt16				reserved				: 9;
} AttentionType;

typedef struct {
	MemHandle		attentionsH;
		// Handle that is kept locked, except when being resized. Points to a
		// chunk of memory which contains an array of attentions, sorted with
		// newest last.
		//DOLATER - peter: Consider leaving handle unlocked except when accessing
		//						 to allow memory manager to move the chunk around.
	AttentionType * attentions;
		// The locked handle, for quick access.
	Int16				numberOfAttentions;		// Includes those marked for deletion
	Int16				topVisibleAttention;
	UInt16			gotoCardNo;					//	Identifies which item
	LocalID			gotodbID;					// user tapped on in list
	UInt32			gotoID;
	DateTimeType	dateTimeShown;				// Time currently shown in dialog title
	Int16				detailAttentionIndex;	// Which attention is shown in detail dialog
	UInt32			snoozeStartTime;
	UInt8				currentUIState;
	UInt8				snoozeUIState;
	UInt8				savedUIState;					// What to do on AttnReopen().
	UInt8				snoozing					: 1;	// Is snooze timer running?
	UInt8				waitingToNag			: 1;	// Is nag timer running?
	UInt8				hasStateChanged		: 1;	// Trying to put vchrAttnStateChanged in key queue?
	UInt8				stateChangeInTransit	: 1;	// Event between EvtGetEvent and SysHandleEvent?
	UInt8				unsnoozeInTransit		: 1;	// Event between EvtGetEvent and SysHandleEvent?
	UInt8				reserved					: 3;
	UInt8				disableCount;	// if this value is > 0, notifications
											// are disabled: see AttnEnableNotification.
	UInt8				filler;							// for alignment
	UInt16			indicatorIsAllowed	: 1;	// Is it possible to show the indicator right now?
	UInt16			indicatorIsEnabled	: 1;	// Used by apps to specify whether indicator can blink.
	UInt16			indicatorFrame			: 2;	// Frame number of indicator animation.
	UInt16			indicatorBlinkPattern : 2;	// Used by attn mgr to specify whether indicator should blink, and if so, how.
	UInt16			reserved2				: 10;
	void *			indicatorBitsBehind;			// Pixels under indicator.
	Int32				indicatorLastTick;			// When the last change was made to the indicator.
} AttnGlobalsType;


/********************************************************************
 * Private Attention Manager Routines
 ********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

void AttnIndicatorAllow (Boolean allowIt)
		SYS_TRAP(sysTrapAttnIndicatorAllow);

Boolean AttnIndicatorAllowed (void)
		SYS_TRAP(sysTrapAttnIndicatorAllowed);

void AttnIndicatorSetBlinkPattern(UInt16 blinkPattern)
		SYS_TRAP(sysTrapAttnIndicatorSetBlinkPattern);

UInt16 AttnIndicatorGetBlinkPattern (void)
		SYS_TRAP(sysTrapAttnIndicatorGetBlinkPattern);

Int32 AttnIndicatorTicksTillNextBlink (void)
		SYS_TRAP(sysTrapAttnIndicatorTicksTillNextBlink);

void AttnIndicatorCheckBlink(void)
		SYS_TRAP(sysTrapAttnIndicatorCheckBlink);

Err AttnInitialize (void)
		SYS_TRAP(sysTrapAttnInitialize);

Boolean AttnHandleEvent (SysEventType * eventP)
		SYS_TRAP(sysTrapAttnHandleEvent);

UInt8 AttnEffectOfEvent(EventType * eventP)
		SYS_TRAP(sysTrapAttnEffectOfEvent);

void AttnDoEmergencySpecialEffects(void)
		SYS_TRAP(sysTrapAttnDoEmergencySpecialEffects);

void AttnAllowClose (void)
		SYS_TRAP(sysTrapAttnAllowClose);

void AttnReopen (void)
		SYS_TRAP(sysTrapAttnReopen);

void AttnEnableNotification(Boolean enable)
		SYS_TRAP(sysTrapAttnEnableNotification);

#ifdef __cplusplus 
}
#endif

#endif //__ATTENTIONPRV_H__
