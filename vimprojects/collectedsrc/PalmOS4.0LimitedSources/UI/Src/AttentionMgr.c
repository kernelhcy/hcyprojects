/******************************************************************************
 *
 * Copyright (c) 2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: AttentionMgr.c
 *
 * Release: 
 *
 * Description:
 *		This file contains the Attention Manager routines for Palm.
 *
 *		The Attention Manager provides a user inteface for applications to get the
 *		user's attention. The Alarm Manager provides only low level support for this,
 *		leaving the application responsible for presenting the alarm dialog. With
 *		this new manager, the UI is no longer provided by the application. The
 *		application instead makes a request to get the user's attention, and that
 *		request is presented to the user with any other requests. The attention
 *		manager also deals with nagging, playing sound, blinking LEDs, vibrating the
 *		device, and any other means that devices may have to get the user's
 *		attention.
 *
 *		The attention manager supports two basic choices for getting the user's
 *		attention. Insistent requests cause a dialog to appear, much as Datebook
 *		did with the alarm manager. Subtle requests do not put up a dialog, and
 *		instead allow the user to continue working with the device. To get the
 *		user's attention, they can play a sound, flash an LED, vibrate the
 *		device, etc. In addition, an indicator appears on the screen. Tapping on
 *		the indicator pops up a dialog showing what needs the user's attention.
 *		The indicator is in the upper left corner of the form title.
 *
 * History:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	06/12/00	Initial Revision
 *			gap	07/21/00	Change parameter list and data structures to support
 *								specification of card number as well as dbID.
 *			gap	07/24/00	Add launch code support for all commands currently 
 *								available via callbacks.
 *
 *****************************************************************************/

#define	NON_PORTABLE

// Allow access to data structure internals for indicator.
#define ALLOW_ACCESS_TO_INTERNALS_OF_CLIPBOARDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_FIELDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS


#include <PalmTypes.h>
#include <SystemPublic.h>
#include <AttentionMgr.h>
#include <Window.h>
#include <DateTime.h>
#include <TimeMgr.h>
#include <Preferences.h>
#include <UIResources.h>
#include <StringMgr.h>
#include <UIColor.h>
#include <Form.h>
#include <Chars.h>
#include <SysEvtMgr.h>
#include <TextMgr.h>
#include <ErrorMgr.h>
#include <SoundMgr.h>
#include <AlarmMgr.h>
#include <SystemMgr.h>
#include <FeatureMgr.h>
#include <NotifyMgr.h>
#include <DebugMgr.h>

#include "Globals.h"
#include "UIGlobals.h"
#include "Hardware.h"
#include "AttentionPrv.h"
#include "UIResourcesPrv.h"


/***********************************************************************
 *
 *	Attention manager global ToDo list
 *
 ***********************************************************************/

// DOLATER - peter: Call EvtFlushPenQueue at some points in order to avoid unintended
//						  actions if user was tapping when dialog is updated.

// DOLATER - peter: Separate out the code that disables the indicator when switching
//						  forms and when a dialog is up from the code that disables it when
//						  there is no form title or the form title is too short, and provide
//						  an API for enabling the indicator even when this latter would not
//						  allow it to be. Consider using this in the Calculator.

// DOLATER - peter: Attention manager and attention indicator are now in this single
//						  source file. Try to get rid of traps as a result. Any that are
//						  only used in this file can be eliminated. Look into why
//						  AttnIndicatorSetBlinkPattern is being used outside this file.


/***********************************************************************
 *
 *	Internal Constants
 *
 ***********************************************************************/

// Define the frames of the indicator blink patterns.
#define kAttnIndicatorFrameOff				0		// Hide indicator for this frame
#define kAttnIndicatorFrameOn					1		// Normal indicator
#define kAttnIndicatorFrameNew				2		// Frame that follows normal frame for new only

// Define the timing of each frame in each indicator blink pattern.
#define kAttnIndicatorOffRetryTime			(sysTicksPerSecond / 10)
#define kAttnIndicatorNewOffTime				(sysTicksPerSecond / 2)
#define kAttnIndicatorNewOnTime				(sysTicksPerSecond / 2)
#define kAttnIndicatorNewTime					(sysTicksPerSecond / 2)
#define kAttnIndicatorOldOffTime				(sysTicksPerSecond)
#define kAttnIndicatorOldOnTime				(sysTicksPerSecond)

// The title settings were gleaned from Form.c
#define titleFont								boldFont
#define titleHeight 							15
#define titleMarginX 						3	// Don't forget the blank column after the last char
#define titleMarginY 						2

#define titleMinGap							10	// Min space between time and title

// Columns in the table of the list view.
#define completedColumn					0
#define descColumn						1

// add a little extra space between rows to reduce gap between bottom attention 
// and top of buttons
#define pixelsBetweenAttentions		2

// Number of system ticks (1/60 seconds) to display crossed out item
// before they're erased.
#define crossOutDelay					40

// Special value for detailAttentionIndex when not in detail mode.
#define noDetail							-1

// Special value returned from PrvAttnFindAttention when none is found.
// Also used for topVisibleAttention when list is empty.
#define noAttention						-1

// Special value passed to PrvAttnClearChangeFlags when all isNew flags should be cleared.
#define allAttentions					-1

// Type for predicate functions.
typedef Boolean AttnPredicate(Int16 index);

// Private routine headers

static Boolean PrvAttnHandleEvent (EventType * event);
static void PrvAttnSnoozeElapsed(UInt16 cmd, SysAlarmTriggeredParamType *paramP);
static void PrvAttnNagElapsed(UInt16 cmd, SysAlarmTriggeredParamType *paramP);
static void PrvAttnSwitchLED(UInt16 cmd, SysAlarmTriggeredParamType *paramP);


// Private static routines

/***********************************************************************
 *
 * FUNCTION:    SendNullEventNextMinute
 *
 * DESCRIPTION: This routine arranges for a null event to be sent when
 *					 the minutes roll over in the system clock. This is used
 *					 to arrange for the time in the form title to be updated.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	07/14/00	Initial Revision
 *
 ***********************************************************************/
static void SendNullEventNextMinute(void)
{
	UInt32 ticks = TimGetTicks ();
	UInt32 seconds = TimGetSeconds ();
	UInt32 minutes = seconds / 60;
	UInt32 targetSeconds = (minutes + 1) * 60;
	UInt32 delayInSeconds = targetSeconds - seconds;
	UInt32 targetTicks = ticks + delayInSeconds * sysTicksPerSecond;
	
	EvtSetNullEventTick (targetTicks);
}


/***********************************************************************
 *
 * FUNCTION:    GetObjectPtr
 *
 * DESCRIPTION: This routine returns a pointer to an object in the current
 *              form.
 *
 * PARAMETERS:  formId - id of the form to display
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/21/95	Initial Revision
 *
 ***********************************************************************/
static void * GetObjectPtr (UInt16 objectID)
{
	FormPtr frm;
	
	frm = FrmGetActiveForm ();
	return (FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, objectID)));

}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnRetryEnqueueKey
 *
 * DESCRIPTION: We're being notified that the was no room for a key to
 *					 be added to the key queue. Because this is deferred
 *					 notification, by the time we get it, we can try again.
 *
 * PARAMETERS:  pointer to character
 *
 * RETURNED:    none
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	10/16/00	Initial Revision
 *
 ***********************************************************************/
static Err PrvAttnRetryEnqueueKey(SysNotifyParamType *notifyParamsP)
{
	WChar commandCharacter = *(WChar *)(notifyParamsP->notifyDetailsP);
	SysNotifyParamType notify;
	Err err;

	if (EvtEnqueueKey(commandCharacter, 0, commandKeyMask))
	{
		// Arrange to try again on next EvtGetEvent by repeating the deferred notification.
		notify.notifyType 		= sysNotifyRetryEnqueueKey;
		notify.broadcaster 		= sysNotifyBroadcasterCode;
		notify.handled 			= false;
		notify.notifyDetailsP 	= &commandCharacter;
		err = SysNotifyBroadcastDeferred(&notify, sizeof(commandCharacter));
		ErrNonFatalDisplayIf(err != errNone, "Can't broadcast for retry enqueue key");
	}
		
	return errNone;
}


// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.


/***********************************************************************
 *
 * FUNCTION:    PrvAttnStateChanged
 *
 * DESCRIPTION: The state of the attention list has changed, so open or
 *					 update the dialog as necessary.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    none
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	10/16/00	Initial Revision
 *
 ***********************************************************************/
static void PrvAttnStateChanged(void)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	SysNotifyParamType notify;
	WChar commandCharacter;
	Err err;

	// Don't do anything while disabled.
	if (attnGlobalsP->disableCount > 0)
		return;
	
	// Don't fill the queue with these commands; one will do the trick.
	if (attnGlobalsP->hasStateChanged)
		return;
	attnGlobalsP->hasStateChanged = true;
	
	// Add a vchrAttnStateChanged event to get the user interface to update or
	// open as necessary. Use the key queue rather than the event queue since
	// the latter is cleared when switching apps. If queue is full, use
	// deferred notification to try on every subsequent call to EvtGetEvent
	// until there is room in the queue.
	if (EvtEnqueueKey(vchrAttnStateChanged, 0, commandKeyMask))
	{
		// Arrange to try again on next EvtGetEvent using deferred notification.
		notify.notifyType 		= sysNotifyRetryEnqueueKey;
		notify.broadcaster 		= sysNotifyBroadcasterCode;
		notify.handled 			= false;
		notify.notifyDetailsP 	= &commandCharacter;
		commandCharacter = vchrAttnStateChanged;
		err = SysNotifyBroadcastDeferred(&notify, sizeof(commandCharacter));
		ErrNonFatalDisplayIf(err != errNone, "Can't broadcast for retry enqueue key");
	}
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnGrowTable
 *
 * DESCRIPTION: Grow the internal table of attentions to make room for
 *					 one more.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    error code
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	08/11/00	Initial Revision
 *			peter	08/21/00	Always grow by exactly one
 *
 ***********************************************************************/
static Err PrvAttnGrowTable(void)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	MemHandle attentionsH;
	Int16 oldSize, newSize;
	Err err;
	
	oldSize = attnGlobalsP->numberOfAttentions;
	newSize = oldSize + 1;
	
	// Limit attentions array to index using an Int16.
	if (oldSize >= 0x7FFF)
		return attnErrMemory;

	if (oldSize == 0)
	{
		// Allocate the new attentions handle.
		attentionsH = MemHandleNew(sizeof(AttentionType) * newSize);
		if (!attentionsH)
			return attnErrMemory;
		MemHandleSetOwner(attentionsH, 0);
		attnGlobalsP->attentionsH = attentionsH;
	}
	else
	{
		// Unlock the handle; it stays locked except when being resized.
		MemHandleUnlock(attnGlobalsP->attentionsH);
		
		// Resize the old attentions Handle.
		err = MemHandleResize(attnGlobalsP->attentionsH, sizeof(AttentionType) * newSize);
		if (err)
			return attnErrMemory;
	}
		
	// Lock the handle; it stays locked except when being resized.
	attnGlobalsP->attentions = MemHandleLock(attnGlobalsP->attentionsH);
	
	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnShrinkTable
 *
 * DESCRIPTION: Shrink the internal table of attentions to free up space
 *					 no longer needed.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	08/11/00	Initial Revision
 *			peter	08/21/00	Leave no extra space
 *
 ***********************************************************************/
static void PrvAttnShrinkTable(void)
{
	AttnGlobalsType	*attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	Err err;
	
	if (attnGlobalsP->numberOfAttentions > 0)
	{
		// Unlock the handle; it stays locked except when being resized.
		MemHandleUnlock(attnGlobalsP->attentionsH);
		
		// Resize the handle smaller. This should never fail.
		err = MemHandleResize(attnGlobalsP->attentionsH,
			sizeof(AttentionType) * attnGlobalsP->numberOfAttentions);
		ErrNonFatalDisplayIf(err != errNone, "Resize smaller shouldn't fail");
		
		// Relock the handle; it stays locked except when being resized.
		attnGlobalsP->attentions = MemHandleLock(attnGlobalsP->attentionsH);
	}
	else
	{
		// All the attentions were removed.
		MemHandleFree(attnGlobalsP->attentionsH);
		attnGlobalsP->attentionsH = NULL;
		attnGlobalsP->attentions = NULL;
	}
	
	return;
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnFlags
 *
 * DESCRIPTION: Given the flags specified by the application, determine
 *					 the flags to use to get the user's attention, taking the
 *					 user preferences into account.
 *
 * PARAMETERS:  flags specified by application
 *
 * RETURNED:    flags to use to get user's attention
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	08/08/00	Initial Revision
 *
 ***********************************************************************/
static AttnFlagsType PrvAttnFlags(AttnFlagsType flags)
{
	AttnFlagsType result;

	// Get the user preferences to start with.
	result = PrefGetPreference(prefAttentionFlags);
	
	// For sound, things are a bit different because in addition to the bit
	// in the flags, there is also a volume preference which can be 'Off'.
	if (PrefGetPreference(prefAlarmSoundVolume) == 0)
		result = result & (kAttnFlagsAllBits - kAttnFlagsSoundBit);

	// Apply overrides specified by application.
	result = result | (flags & kAttnFlagsEverything);
	result = result & ~((flags & kAttnFlagsNothing) >> 16);
	
	return result;
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnFindNewInsistent
 *
 * DESCRIPTION: Find the one new insistent attention.
 *
 * PARAMETERS:  none
 *
 * RETURNS:		 index of attention
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	07/27/00	Initial Revision
 *
 ***********************************************************************/
static Int16 PrvAttnFindNewInsistent(void)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	Int16 index;

	for (index = 0; index < attnGlobalsP->numberOfAttentions; index++)
	{
		if (attnGlobalsP->attentions[index].level == kAttnLevelInsistent &&
			attnGlobalsP->attentions[index].isNew &&
			!attnGlobalsP->attentions[index].isDeleted)
		{
			return index;
		}
	}
	ErrNonFatalDisplay("No new insistent found");
	return noAttention;
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnFindOnlyInsistent
 *
 * DESCRIPTION: Find the one insistent attention.
 *
 * PARAMETERS:  none
 *
 * RETURNS:		 index of attention
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	07/27/00	Initial Revision
 *
 ***********************************************************************/
static Int16 PrvAttnFindOnlyInsistent(void)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	Int16 index;

	for (index = 0; index < attnGlobalsP->numberOfAttentions; index++)
	{
		if (attnGlobalsP->attentions[index].level == kAttnLevelInsistent &&
			!attnGlobalsP->attentions[index].isDeleted)
		{
			return index;
		}
	}
	ErrNonFatalDisplay("No insistent found");
	return noAttention;
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnFindMostRecentInsistent
 *
 * DESCRIPTION: Find the most recent insistent attention, or the most
 *					 recent subtle attention if there are no insistent attentions.
 *
 * PARAMETERS:  none
 *
 * RETURNS:		 index of attention
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	08/03/00	Initial Revision
 *
 ***********************************************************************/
static Int16 PrvAttnFindMostRecentInsistent(void)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	Int16 index;	// Signed for loop that counts down to zero.

	// Look for an insistent attention, starting at the end, where the most
	// recent attentions are added.
	for (index = attnGlobalsP->numberOfAttentions - 1; index >= 0; index--)
	{
		if (attnGlobalsP->attentions[index].level == kAttnLevelInsistent &&
			!attnGlobalsP->attentions[index].isDeleted)
		{
			return index;
		}
	}

	// Look for any attention, starting at the end, where the most
	// recent attentions are added.
	for (index = attnGlobalsP->numberOfAttentions - 1; index >= 0; index--)
	{
		if (!attnGlobalsP->attentions[index].isDeleted)
		{
			return index;
		}
	}
	
	ErrNonFatalDisplay("No attention found");
	return noAttention;
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnFindMostRecentInsistentWhich
 *
 * DESCRIPTION: Of the attentions which satisfy the given predicate,
 *					 find the most recent insistent attention, or the most
 *					 recent subtle attention if there are no insistent attentions.
 *
 * PARAMETERS:  predicate function accepting index as parameter
 *
 * RETURNS:		 index of attention
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	10/18/00	Initial Revision
 *
 ***********************************************************************/
static Int16 PrvAttnFindMostRecentInsistentWhich(AttnPredicate *Predicate)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	Int16 index;	// Signed for loop that counts down to zero.

	// Look for an insistent attention, starting at the end, where the most
	// recent attentions are added.
	for (index = attnGlobalsP->numberOfAttentions - 1; index >= 0; index--)
	{
		if (Predicate(index) &&
			attnGlobalsP->attentions[index].level == kAttnLevelInsistent &&
			!attnGlobalsP->attentions[index].isDeleted)
		{
			return index;
		}
	}

	// Look for any attention, starting at the end, where the most
	// recent attentions are added.
	for (index = attnGlobalsP->numberOfAttentions - 1; index >= 0; index--)
	{
		if (Predicate(index) &&
			!attnGlobalsP->attentions[index].isDeleted)
		{
			return index;
		}
	}
	
	return noAttention;
}


/***********************************************************************
 *
 * FUNCTION:    IsNagging
 *
 * DESCRIPTION: A predicate function that returns whether an attention is
 *					 nagging.
 *
 * PARAMETERS:  index of attention
 *
 * RETURNS:		 whether predicate is satisfied
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	10/18/00	Initial Revision
 *
 ***********************************************************************/
static Boolean IsNagging(Int16 index)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	
	return attnGlobalsP->attentions[index].nagsRemaining > 0;
}


/***********************************************************************
 *
 * FUNCTION:    PlaysSound
 *
 * DESCRIPTION: A predicate function that returns whether an attention
 *					 plays sound.
 *
 * PARAMETERS:  index of attention
 *
 * RETURNS:		 whether predicate is satisfied
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	10/18/00	Initial Revision
 *
 ***********************************************************************/
static Boolean PlaysSound(Int16 index)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	AttnFlagsType flags;
	
	if (!attnGlobalsP->attentions[index].getUsersAttention)
		return false;

	flags = PrvAttnFlags(attnGlobalsP->attentions[index].flags);
	return (flags & kAttnFlagsSoundBit) != 0;
}


/***********************************************************************
 *
 * FUNCTION:    	PrvAttnFindNextNag
 *
 * DESCRIPTION:	Figure out what nag will happen next, if any. When there
 *						are multiple attention requests trying to nag, we respect
 *						the nag settings for the most recent insistent attention,
 *						or if none then the most recent subtle attention.
 *
 * PARAMETERS:  	nextNagP	- used to return the index of the attention that will next nag
 *						whenP		- used to return when the next action will happen
 *
 * RETURNED:    	Whether any nag will happen.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	08/02/00	Initial Revision
 *			peter	10/18/00	Use most recent insistent rather than independent scheduling
 *
 ***********************************************************************/
static Boolean PrvAttnFindNextNag(UInt16 *nextNagP, UInt32 *whenP)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	UInt16 nextNagIndex;
	
	// Find which attention request will nag next, if any. Instead of allowing
	// each attention to nag on its own schedule, we use the policy of the
	// most recent insistent attention.
	nextNagIndex = PrvAttnFindMostRecentInsistentWhich(IsNagging);
	*nextNagP = nextNagIndex;
	if (nextNagIndex != noAttention)
	{
		*whenP = attnGlobalsP->attentions[nextNagIndex].lastNagTime +
					attnGlobalsP->attentions[nextNagIndex].nagRateInSeconds;
		return true;
	}
	
	return false;
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnNotify
 *
 * DESCRIPTION: Notify the application of something using a sub-launch
 *					 or callback.
 *
 * PARAMETERS:  index - index in array of attentions
 *					 command - what to tell application
 *					 commandArgsP - pointer to additional arguments specific to command
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	09/18/00	Initial Revision
 *			peter	11/02/00	Delete attention on error result
 *
 ***********************************************************************/
static void PrvAttnNotify(UInt16 index, AttnCommandType command, AttnCommandArgsType *commandArgsP)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	Err err;
	UInt32 result = errNone;
	AttnLaunchCodeArgsType launchCodeData;
	
	// Don't notify the application if it was deleted.
	if (attnGlobalsP->attentions[index].isDeleted)
		return;

	// Notify the application using callback or launch code.
	if (attnGlobalsP->attentions[index].callbackFnP)
		err = (attnGlobalsP->attentions[index].callbackFnP)(command,
			attnGlobalsP->attentions[index].userData, commandArgsP);
	else
		{
		launchCodeData.command = command;
		launchCodeData.userData = attnGlobalsP->attentions[index].userData;
		launchCodeData.commandArgsP = commandArgsP;
		
		err = SysAppLaunch(attnGlobalsP->attentions[index].cardNo,
			attnGlobalsP->attentions[index].dbID, 0, 
			sysAppLaunchCmdAttention, (void*)&launchCodeData, &result);
		}
	
	// If the application returns an error from any command, react by
	// deleting the attention.
	if (err || result)
		AttnForgetIt
		  (attnGlobalsP->attentions[index].cardNo,
			attnGlobalsP->attentions[index].dbID,
			attnGlobalsP->attentions[index].userData);
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnDrawDialogTitle
 *
 * DESCRIPTION: Show the current time along with the title, "Reminder"
 *					 or "Reminders". Avoid flicker if time didn't change since
 *					 the last time the title was drawn.
 *
 * PARAMETERS:  dateTimeShownP - time being shown - updated to reflect new time
 *					 firstTime		 - true if first call, false if already showing above time
 *					 currentUIState - dialog mode (attnDetailsDialog or attnListDialog)
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	07/07/00	Initial Revision
 *
 ***********************************************************************/
static void PrvAttnDrawDialogTitle (DateTimeType * dateTimeShownP,
	Boolean firstTime, UInt8 currentUIState)
{
	DateTimeType 				dateTime;
	Char							timeStr [timeStringLength];
	MemHandle					titleH;
	Char*							titleP;
	UInt16						titleLen;
	Int16							windowWidth;
	Int16							windowHeight;
	RectangleType				r;
	Coord							x;
	Coord							minX;
	FontID						currFont;
	IndexedColorType 			currForeColor;
	IndexedColorType 			currBackColor;
	IndexedColorType			currTextColor;

	TimSecondsToDateTime (TimGetSeconds(), &dateTime);
	
	// Don't update the title if it hasn't changed.
	if (!firstTime && dateTimeShownP->hour == dateTime.hour &&
							dateTimeShownP->minute == dateTime.minute)
		return;
	
	*dateTimeShownP = dateTime;
	
	TimeToAscii (dateTime.hour, dateTime.minute,
					(TimeFormatType) PrefGetPreference (prefTimeFormat),
					timeStr);
	
	titleH = DmGetResource (strRsc, currentUIState == attnDetailsDialog
													? AttentionDetailTitleStrID
													: AttentionListTitleStrID);
	titleP = MemHandleLock (titleH);
	titleLen = StrLen (titleP);

	WinGetWindowExtent (&windowWidth, &windowHeight);
	
	currFont = FntSetFont (titleFont);
 	currForeColor = WinSetForeColor (UIColorGetTableEntryIndex(UIDialogFrame));
 	currBackColor = WinSetBackColor (UIColorGetTableEntryIndex(UIDialogFill));
 	currTextColor = WinSetTextColor (UIColorGetTableEntryIndex(UIDialogFrame));
	
	r = (RectangleType) { {0, 1}, {windowWidth, FntLineHeight ()} };
	
	// Center the title, but force it off center if necessary to maintain a minimum gap
	// between the time and the title.
	x  = (windowWidth - FntCharsWidth (titleP, titleLen) + 1) >> 1;		// Center title
	minX = FntCharsWidth (timeStr, StrLen(timeStr)) + titleMinGap;
	if (x < minX)													// Force off center if necessary
		x = minX;

	WinDrawLine (1, 0, r.extent.x-2, 0);
	WinDrawRectangle (&r, 0);
	WinDrawInvertedChars (timeStr, StrLen(timeStr), titleMarginX, 0);
	WinDrawInvertedChars (titleP, titleLen, x, 0);
	
   WinSetForeColor (currForeColor);
   WinSetBackColor (currBackColor);
   WinSetTextColor (currTextColor);
	FntSetFont (currFont);
	MemPtrUnlock(titleP);
}


/***********************************************************************
 *
 * FUNCTION:		PrvAttnDemoteAttentions
 *
 * DESCRIPTION:	The user hit the Done button in the list dialog, so
 *						demote all insistent attentions, so they're treated as
 *						if they were subtle when deciding whether to open in
 *						list versus detail mode.
 *
 * PARAMETERS:		none
 *
 * RETURNED:		nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	10/04/00	Initial Revision
 *
 ***********************************************************************/
static void PrvAttnDemoteAttentions(void)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	UInt16 index;

	// Mark all attentions as demoted (don't really care about subtle ones).
	for (index = 0; index < attnGlobalsP->numberOfAttentions; index++)
		attnGlobalsP->attentions[index].isDemoted = true;
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnFindAttention
 *
 * DESCRIPTION: Find the index of an attention given information to uniquely
 *					 identify it. Skips attentions marked for deletion.
 *
 * PARAMETERS:  data to uniquely identify which attention in the list to go to
 *
 * RETURNED:    index of attention in array
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	07/31/00	Initial Revision
 *
 ***********************************************************************/
static UInt16 PrvAttnFindAttention(UInt16 cardNo, LocalID dbID, UInt32 userData)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	UInt16 index;
	
	// Find the attention request specified.
	for (index = 0; index < attnGlobalsP->numberOfAttentions; index++)
	{
		if (attnGlobalsP->attentions[index].cardNo == cardNo &&
			 attnGlobalsP->attentions[index].dbID == dbID &&
			 attnGlobalsP->attentions[index].userData == userData &&
			 !attnGlobalsP->attentions[index].isDeleted)
		{
			return index;
		}
	}
	
	return noAttention;
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnRemoveAttention
 *
 * DESCRIPTION: Remove the attention at the given index. This routine is
 *					 to be used by the user interface, not the API. If an API
 *					 routine wants to remove an attention, it should mark it
 *					 for deletion instead.
 *
 * PARAMETERS:  index in array of attentions
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	07/20/00	Initial Revision
 *			gap	10/19/00 added parameter to the gotIt launch code
 *
 ***********************************************************************/
static void PrvAttnRemoveAttention(UInt16 index)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	UInt16	destIndex;
	AttnCommandArgsType args;
	
	args.gotIt.dismissedByUser = true;
	PrvAttnNotify(index, kAttnCommandGotIt, &args);

	// Remove the attention request, shifting up those after it.
	for (destIndex = index; destIndex < attnGlobalsP->numberOfAttentions-1; destIndex++)
		attnGlobalsP->attentions[destIndex] = attnGlobalsP->attentions[destIndex + 1];
	attnGlobalsP->numberOfAttentions--;
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnVibrate
 *
 * DESCRIPTION: Vibrate to got the user's attention.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	09/28/00	Initial Revision
 *
 ***********************************************************************/
static void PrvAttnVibrate(void)
{
	UInt16 rate = ResLoadConstant(AttentionVibrateRate);
	UInt32 pattern = ResLoadConstant(AttentionVibratePattern);
	UInt16 delay = ResLoadConstant(AttentionVibrateDelay);
	UInt16 repeatCount = ResLoadConstant(AttentionVibrateRepeatCount);
	Boolean vibrateActive = true;
	
	HwrVibrateAttributes(true, kHwrVibrateRate, &rate);
	HwrVibrateAttributes(true, kHwrVibratePattern, &pattern);
	HwrVibrateAttributes(true, kHwrVibrateDelay, &delay);
	HwrVibrateAttributes(true, kHwrVibrateRepeatCount, &repeatCount);
	HwrVibrateAttributes(true, kHwrVibrateActive, &vibrateActive);
	// Assume HAL supports all these attributes.
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnLED
 *
 * DESCRIPTION: Blink the LED to got the user's attention. A timer is used
 *					 to automatically switch from the initial blink pattern to
 *					 the continuous pattern after a fixed delay.
 *
 * PARAMETERS:  how to blink or whether to turn LED off instead
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	09/28/00	Initial Revision
 *
 ***********************************************************************/
static void PrvAttnLED(AttnLEDMode mode)
{
	UInt16 rate = (mode == attnLEDInitial)
		? ResLoadConstant(AttentionInitialLEDBlinkRate)
		: ResLoadConstant(AttentionContinuousLEDBlinkRate);
	UInt32 pattern = (mode == attnLEDInitial)
		? ResLoadConstant(AttentionInitialLEDPattern)
		: ResLoadConstant(AttentionContinuousLEDPattern);
	UInt16 delay = (mode == attnLEDInitial)
		? ResLoadConstant(AttentionInitialLEDBlinkDelay)
		: ResLoadConstant(AttentionContinuousLEDBlinkDelay);
	UInt16 repeatCount = (mode == attnLEDInitial)
		? ResLoadConstant(AttentionInitialLEDRepeatCount)
		: ResLoadConstant(AttentionContinuousLEDRepeatCount);
	Boolean ledActive = (mode != attnLEDOff);
	
	if (mode != attnLEDOff)
	{
		HwrLEDAttributes(true, kHwrLEDRate, &rate);
		HwrLEDAttributes(true, kHwrLEDPattern, &pattern);
		HwrLEDAttributes(true, kHwrLEDDelay, &delay);
		HwrLEDAttributes(true, kHwrLEDRepeatCount, &repeatCount);
	}
	HwrLEDAttributes(true, kHwrLEDActive, &ledActive);
	
	// Start or stop the timer that switches to the continuous blink pattern.
	if (mode == attnLEDInitial)
		AlmSetProcAlarm(&PrvAttnSwitchLED, 0,
			TimGetSeconds() + ResLoadConstant(AttentionContinuousLEDStartDelay));
	if (mode == attnLEDOff)
		AlmSetProcAlarm(&PrvAttnSwitchLED, 0, 0);
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnSwitchLED
 *
 * DESCRIPTION: Switch from initial LED blink pattern to a continuous
 *					 pattern which goes forever.
 *
 * PARAMETERS:
 *
 * RETURNED:
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	09/28/00	Initial Revision
 *
 ***********************************************************************/
static void PrvAttnSwitchLED(UInt16 cmd, SysAlarmTriggeredParamType *paramP)
{
	switch (cmd)
	{
		case almProcCmdReschedule:
			// If the user changes the system time, just go ahead and switch
			// to continuous pattern right away.
			// Don't break - fall through to case below:
			
		case almProcCmdTriggered:
			if (ResLoadConstant(AttentionContinuousLEDPattern) != 0)
				PrvAttnLED(attnLEDContinuous);
			else		// There is no continuous pattern, so just turn LED off.
				PrvAttnLED(attnLEDOff);
			break;
	}
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnGetUsersAttentions
 *
 * DESCRIPTION: Get the user's attention for all attention requests which
 *					 haven't yet had their special effects performed.
 *
 * PARAMETERS:  none
 *
 * RETURNS:		 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	09/05/00	Initial Revision
 *			peter	10/16/00	Only play one sound, vibrate once, etc.
 *
 ***********************************************************************/
static void PrvAttnGetUsersAttentions(void)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	UInt32 now = TimGetSeconds();
	UInt16 index;
	SysNotifyParamType notify;
	AttnNotifyDetailsType details;
	Err err;
	AttnFlagsType flags, groupFlags;
	
	// Compute flags that indicate how we will get the users attention
	// by combining the flags for each item that needs the users attention.
	groupFlags = 0;
	for (index = 0; index < attnGlobalsP->numberOfAttentions; index++)
	{
		if (attnGlobalsP->attentions[index].getUsersAttention)
			groupFlags |= PrvAttnFlags(attnGlobalsP->attentions[index].flags);
	}
	
	// Don't turn the screen on or broadcast the notification if there
	// is nothing to do.
	if (groupFlags == 0)
		return;
	
	// Before doing anything else, turn on the screen if it isn't already
	// on, and make sure it stays on for a while.
	EvtResetAutoOffTimer();

#if	EMULATION_LEVEL == EMULATION_NONE
	// Vibrate if any specify to.
	if (groupFlags & kAttnFlagsVibrateBit)
		PrvAttnVibrate();
	
	// Blink LED if any specify to.
	if (groupFlags & kAttnFlagsLEDBit)
		PrvAttnLED(attnLEDInitial);
#endif
	
	// Play sound if any specify to. If more than one specifies that a sound
	// should be played, choose the most recent insistent.
	index = PrvAttnFindMostRecentInsistentWhich(PlaysSound);
	if (index != noAttention)
		PrvAttnNotify(index, kAttnCommandPlaySound, NULL);

	// Perform the custom special effects. If more than one specifies that
	// a custom effect is needed, do them all. Custom effects should not
	// generally be used to play sounds because that would result in
	// sometimes hearing two sounds. If you want a sound to be heard no
	// matter what else is happenning, you can use the custom special effect
	// for that.
	for (index = 0; index < attnGlobalsP->numberOfAttentions; index++)
	{
		if (attnGlobalsP->attentions[index].getUsersAttention)
		{
			flags = PrvAttnFlags(attnGlobalsP->attentions[index].flags);
			if (flags & kAttnFlagsCustomEffectBit)
				PrvAttnNotify(index, kAttnCommandCustomEffect, NULL);
		}
	}
	
	// Notify anyone who cares that we got the user's attention.
	details.flags				= groupFlags;
	notify.notifyType 		= sysNotifyGotUsersAttention;
	notify.broadcaster 		= sysNotifyBroadcasterCode;
	notify.handled 			= false;
	notify.notifyDetailsP 	= &details;
	err = SysNotifyBroadcast(&notify);
	ErrNonFatalDisplayIf(err != errNone, "Can't broadcast got users attention");
	
	// Update the state of each attention with respect to getting the user's attention.
	for (index = 0; index < attnGlobalsP->numberOfAttentions; index++)
		attnGlobalsP->attentions[index].getUsersAttention = false;
	
	// Flush the pen queue now that we're done, so that any tapping the
	// user attempted while the device was playing sounds is ignored.
	// It'd be nice to flush the up and down hard buttons from the key
	// queue, but flushing the key queue is dangerous because it's used
	// for virtual character events.
	EvtFlushPenQueue();
}


/***********************************************************************
 *
 * FUNCTION:		PrvAttnSetNagTimer
 *
 * DESCRIPTION:	Check whether any attention requests need to nag at some
 *						time in the future, and if so, arrange with the alarm manager
 *						to do so. 
 *
 * PARAMETERS:		none
 *
 * RETURNED:		boolean designating if nag alarm set was successful
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	08/02/00	Initial Revision
 *
 ***********************************************************************/
static Boolean PrvAttnSetNagTimer(void)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	Err err = errNone;
	UInt16 nextNagIndex;
	UInt32 nextNagTime;
	
	if (!PrvAttnFindNextNag(&nextNagIndex, &nextNagTime))
		return false;

	// If there is an alarm currently pending we need to clear it.
	if (attnGlobalsP->waitingToNag)
		{
		err = AlmSetProcAlarm(&PrvAttnNagElapsed, 0, 0);
		attnGlobalsP->waitingToNag = false;
		}
	
	// Set the nag timer.
	if (!err)
		{
		err = AlmSetProcAlarm(&PrvAttnNagElapsed, 0, nextNagTime);

		if (!err)
			{
			attnGlobalsP->waitingToNag = true;
			}
		}

	return (err == errNone);
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnTimeChanged
 *
 * DESCRIPTION: We're being notified that the system time has changed.
 *					 Reschedule any alarms so that the snooze time and nag
 *					 intervals aren't affected.
 *
 * PARAMETERS:  pointer to amount time changed
 *
 * RETURNED:    none
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	10/16/00	Initial Revision
 *
 ***********************************************************************/
static Err PrvAttnTimeChanged(SysNotifyParamType *notifyParamsP)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	UInt32 * deltaP = notifyParamsP->notifyDetailsP;
	Int16 index;
	Err err;

	// The main reason for this notification is so we can adjust the snooze
	// timer, so that it always times the same amount of time, even if the
	// system time is adjusted while snoozing.
	if (attnGlobalsP->snoozing)
	{
		attnGlobalsP->snoozeStartTime += *deltaP;
		err = AlmSetProcAlarm(&PrvAttnSnoozeElapsed, 0,
			attnGlobalsP->snoozeStartTime + attnMgrSnoozeDuration);
		if (err)
			return err;
	}
	
	// We don't expect the system time to change while a nag timer is
	// running, since that only happens when the attention dialog is
	// open, but just in case, we arrange nagging to be unaffected by
	// the time change, just as we do for snooze.
	for (index = 0; index < attnGlobalsP->numberOfAttentions; index++)
		if (attnGlobalsP->attentions[index].nagsRemaining > 0)
			attnGlobalsP->attentions[index].lastNagTime += *deltaP;
	PrvAttnSetNagTimer();
	
	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:		PrvAttnStopNagging
 *
 * DESCRIPTION:	The user has just pressed the Done button on the list 
 *						view.  Cancel nagging by telling the alarm manager to
 *						stop timing and clearing the nags remaining counters. 
 *
 * PARAMETERS:		none
 *
 * RETURNED:		boolean designating if nag alarm clear was successful
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	08/03/00	Initial Revision
 *
 ***********************************************************************/
static Boolean PrvAttnStopNagging(void)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	Err err = errNone;
	UInt16 index;
	
	if (attnGlobalsP->waitingToNag)
		{
		err = AlmSetProcAlarm(&PrvAttnNagElapsed, 0, 0);
		attnGlobalsP->waitingToNag = false;
		
		// Mark all attentions as done nagging.
		for (index = 0; index < attnGlobalsP->numberOfAttentions; index++)
			attnGlobalsP->attentions[index].nagsRemaining = 0;
	
		}
	return (err == errNone);
}


/***********************************************************************
 *
 * FUNCTION:    	PrvAttnNagElapsed
 *
 * DESCRIPTION:	Callback proc specified to alarm manager used to get
 *						notified when our nag timer has elapsed. Nag the user
 *						and start the timer for the next nag to occur, if any.
 *
 * PARAMETERS:  
 *
 * RETURNED:    
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	08/02/00	Initial Revision, copied from PrvAttnSnoozeElapsed
 *			peter	11/20/00	Call PrvAttnStateChanged rather than PrvAttnGetUsersAttentions
 *
 ***********************************************************************/
static void PrvAttnNagElapsed(UInt16 cmd, SysAlarmTriggeredParamType *paramP)
{	
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	UInt16 index, nextNagIndex;
	UInt32 nextNagTime;
	UInt32 now = TimGetSeconds();

	switch (cmd)
	{
		case almProcCmdTriggered:
			attnGlobalsP->waitingToNag = false;

			// Check whether there's still a reason to nag.
			if (!PrvAttnFindNextNag(&nextNagIndex, &nextNagTime))
				return;
			if (nextNagIndex == noAttention)
				return;
			
			// There may be other attentions which also need to nag. We use the nag
			// policy of the most recent insistent attention, so we ignore the schedule
			// of other attentions, but any others which still have nags remaining are
			// made to nag now.
			for (index = 0; index < attnGlobalsP->numberOfAttentions; index++)
			{
				if (attnGlobalsP->attentions[index].nagsRemaining > 0)
				{
					attnGlobalsP->attentions[index].nagsRemaining--;
					attnGlobalsP->attentions[index].getUsersAttention = true;
					attnGlobalsP->attentions[index].lastNagTime = now;
				}
			}
			
			// Add a vchrAttnStateChanged event to get the special effects to be
			// performed at the same time as any new attentions which occur at
			// the same time. If the attention dialog failed to open the first
			// time, this may also open the dialog.
			PrvAttnStateChanged();

			// Start or adjust the nag timer as necessary.
			PrvAttnSetNagTimer();
			break;
		
		case almProcCmdReschedule:
			// The user changed the system time. We want to reschedule the alarm
			// so that the total delay is the same as it would have been if the
			// user didn't change the system time, but since we don't know how
			// the time was changed here, we have register for the time change
			// notification, and reschedule the alarm then.
			break;
	}
}


/***********************************************************************
 *
 * FUNCTION:    	PrvAttnSnoozeElapsed
 *
 * DESCRIPTION:	callback proc specified to alarm manager used to get
 *						notified when our snooze timer has elapsed.
 *
 * PARAMETERS:  
 *
 * RETURNED:    
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			gap	07/21/00	Initial Revision
 *
 ***********************************************************************/
static void PrvAttnSnoozeElapsed(UInt16 cmd, SysAlarmTriggeredParamType *paramP)
{	
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	SysNotifyParamType notify;
	WChar commandCharacter;
	Err err;
	Int16 index;
	Boolean hasSnoozedAttentions;

	switch (cmd)
	{
		case almProcCmdTriggered:
			attnGlobalsP->snoozing = false;

			// Mark all snoozed attentions as no longer snoozed, but requiring
			// user's attention, and restart their nagging. Do this now so that
			// even if the dialog can't open, the special effects will be performed.
			hasSnoozedAttentions = false;
			for (index = 0; index < attnGlobalsP->numberOfAttentions; index++)
				if (attnGlobalsP->attentions[index].isSnoozed &&
					 !attnGlobalsP->attentions[index].isDeleted)
				{
					attnGlobalsP->attentions[index].isSnoozed = false;
					hasSnoozedAttentions = true;
					attnGlobalsP->attentions[index].getUsersAttention = true;
					attnGlobalsP->attentions[index].nagsRemaining =
						attnGlobalsP->attentions[index].nagRepeatLimit;
					attnGlobalsP->attentions[index].lastNagTime = TimGetSeconds();
				}
			PrvAttnSetNagTimer();
			
			// If all snoozed events have been removed from the attention manager 
			// queue during the snooze period, don't bring up the dialog. This
			// may happen because we don't bother to cancel the snooze timer when
			// the last snoozed attention is removed. Note that it's possible for
			// the snoozed attentions to be removed between the time this test is
			// performed and the time the vchrAttnUnsnooze event posted below is
			// processed. In this case, the event should be ignored.
			if (!hasSnoozedAttentions)
				return;

			// Add a vchrAttnUnsnooze event to get the user interface to update or
			// open as necessary. Use the key queue rather than the event queue since
			// the latter is cleared when switching apps. If queue is full, use
			// deferred notification to try on every subsequent call to EvtGetEvent
			// until there is room in the queue.
			if (EvtEnqueueKey(vchrAttnUnsnooze, 0, commandKeyMask))
			{
				// Arrange to try again on next EvtGetEvent using deferred notification.
				notify.notifyType 		= sysNotifyRetryEnqueueKey;
				notify.broadcaster 		= sysNotifyBroadcasterCode;
				notify.handled 			= false;
				notify.notifyDetailsP 	= &commandCharacter;
				commandCharacter = vchrAttnUnsnooze;
				err = SysNotifyBroadcastDeferred(&notify, sizeof(commandCharacter));
				ErrNonFatalDisplayIf(err != errNone, "Can't broadcast for retry enqueue key");
			}
			break;
		
		case almProcCmdReschedule:
			// The user changed the system time. We want to reschedule the alarm
			// so that the total delay is the same as it would have been if the
			// user didn't change the system time, but since we don't know how
			// the time was changed here, we have register for the time change
			// notification, and reschedule the alarm then.
			break;
	}
}


/***********************************************************************
 *
 * FUNCTION:		PrvAttnSetSnoozeTimer
 *
 * DESCRIPTION:	The user has just pressed the snooze button on one of the 
 *						two views.  If the attention mgr is currently snoozed, 
 *						remove the current alarm from the queue, set an alarm to 
 *						redisplay the current view after the snooze time has elapsed. 
 *
 * PARAMETERS:		currentDisplay - are we in list view or details view?
 *
 * RETURNED:		boolean designating if snooze alarm set was successful
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			gap	07/25/00	Initial Revision
 *
 ***********************************************************************/
static Boolean PrvAttnSetSnoozeTimer(UInt8 currentDisplay)
{
	Err err = errNone;
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	Int16 index;

	// if there is an alarm currently pending we need to clear it
	if (attnGlobalsP->snoozing)
		{
		err = AlmSetProcAlarm(&PrvAttnSnoozeElapsed, 0, 0);
		attnGlobalsP->snoozing = false;
		attnGlobalsP->snoozeUIState = attnNoDialog;
		}
	
	// set the snooze timer to the full snooze time duration
	if (!err)
		{
		attnGlobalsP->snoozeStartTime = TimGetSeconds();
		err = AlmSetProcAlarm(&PrvAttnSnoozeElapsed, 0,
			attnGlobalsP->snoozeStartTime + attnMgrSnoozeDuration);

		if (!err)
			{
			attnGlobalsP->snoozing = true;
			attnGlobalsP->snoozeUIState = currentDisplay;
			
			// Mark all attentions as snoozed, so we know if there are still
			// any left, and notify the applications that their attention
			// requests are being snoozed.
			for (index = 0; index < attnGlobalsP->numberOfAttentions; index++)
				{
				if (!attnGlobalsP->attentions[index].isDeleted)
					{
					attnGlobalsP->attentions[index].isSnoozed = true;
					PrvAttnNotify(index, kAttnCommandSnooze, NULL);
					}
				}
			}
		}

	return (err == errNone);
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnGoto
 *
 * DESCRIPTION: Process a goto request by the user.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	07/14/00	Initial Revision
 *			peter	07/26/00	Find attention to go to in list.
 *			peter	07/31/00	Remove parameters, using attnGlobalsP->goto* instead
 *
 ***********************************************************************/
static void PrvAttnGoto(void)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	UInt16 index;
	
	index = PrvAttnFindAttention( attnGlobalsP->gotoCardNo,
		attnGlobalsP->gotodbID, attnGlobalsP->gotoID);
	
	if (index != noAttention)
		PrvAttnNotify(index, kAttnCommandGoThere, NULL);
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnGetCounts
 *
 * DESCRIPTION: Find out either:
 *						- how many things are trying to get the user's attention, or
 *						- how many new calls to AttnGetAttention haven't been dealt with yet.
 *					 Of those, find out how many are insistent and how many are subtle.
 *					 Count demoted insistent attentions as if they were subtle.
 *
 * PARAMETERS:  insistentCountP	- where to write number of new insistent requests
 *					 subtleCountP		- where to write number of new subtle requests
 *										   Either of the above may be NULL
 *					 countNewOnly		- whether to count only new requests or all requests
 *
 * RETURNS:		 Total number of new attention requests (subtle + insistent)
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	07/25/00	Initial Revision
 *			peter	09/12/00	Don't count if deleted
 *			peter	10/04/00	Count demoted attentions as subtle
 *
 ***********************************************************************/
static UInt16 PrvAttnGetCounts(UInt16 *insistentCountP, UInt16 *subtleCountP,
										 Boolean countNewOnly)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	UInt16 insistentCount = 0;
	UInt16 subtleCount = 0;
	UInt16 index;

	for (index = 0; index < attnGlobalsP->numberOfAttentions; index++)
	{
		if ((attnGlobalsP->attentions[index].isNew || !countNewOnly) &&
			!attnGlobalsP->attentions[index].isDeleted)
		{
			switch (attnGlobalsP->attentions[index].level)
			{
				case kAttnLevelInsistent:
					if (attnGlobalsP->attentions[index].isDemoted)
						subtleCount++;
					else
						insistentCount++;
					break;
				case kAttnLevelSubtle:
					subtleCount++;
					break;
				default:
					ErrNonFatalDisplay("Invalid level");
			}
		}
	}
	
	if (insistentCountP != NULL)
		*insistentCountP = insistentCount;
	if (subtleCountP != NULL)
		*subtleCountP = subtleCount;
	
	return insistentCount + subtleCount;
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnUpdateIndicator
 *
 * DESCRIPTION: Set the blink pattern of the attention indicator
 *					 appropriately based on whether there are any attentions,
 *					 and if so, whether any are new (not yet seen by the user).
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	12/21/00	Initial Revision
 *
 ***********************************************************************/
static void PrvAttnUpdateIndicator(void)
{
	UInt16 totalCount, newCount;
	
	totalCount = PrvAttnGetCounts(NULL, NULL, false);
	newCount = PrvAttnGetCounts(NULL, NULL, true);
	AttnIndicatorSetBlinkPattern(newCount > 0
											? kAttnIndicatorBlinkPatternNew
											: (totalCount > 0
												? kAttnIndicatorBlinkPatternOld
												: kAttnIndicatorBlinkPatternNone));
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnDatabaseRemoved
 *
 * DESCRIPTION: The notify manager is informing us that a database was
 *					 deleted. Remove any associated attentions.
 *
 * PARAMETERS:  notifyParamsP		- pointer to struct identifying database
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	11/08/00	Initial Revision
 *			peter	11/28/00	Don't post vchrAttnStateChanged if nothing happenned.
 *
 ***********************************************************************/
static Err PrvAttnDatabaseRemoved(SysNotifyParamType *notifyParamsP)
{
	AttnGlobalsType * attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	SysNotifyDBDeletedType * notifyDBInfoP = notifyParamsP->notifyDetailsP;
	UInt16 index;
	Boolean wereAnyDeleted = false;
	
	// Delete any attention requests for the database being deleted.
	for (index = 0; index < attnGlobalsP->numberOfAttentions; index++)
	{
		if (attnGlobalsP->attentions[index].cardNo == notifyDBInfoP->cardNo &&
			 attnGlobalsP->attentions[index].dbID == notifyDBInfoP->oldDBID)
		{
			// Let the UI know that the item was removed.
			attnGlobalsP->attentions[index].isDeleted = true;
			wereAnyDeleted = true;
		}
	}
	
	if (wereAnyDeleted)
	{
		// Add a vchrAttnStateChanged event to get the user interface to update as necessary.
		PrvAttnStateChanged();
		
		// Start or adjust the nag timer as necessary. If the removed attention was the next
		// to nag, we need to reset the alarm manager timer.
		PrvAttnSetNagTimer();
		
		// Turn the indicator off, or switch from new to old blink pattern if necessary.
		PrvAttnUpdateIndicator();
	}
	
	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnHasDeleted
 *
 * DESCRIPTION: Find out if any new calls to AttnForgetIt haven't
 *					 been dealt with yet.
 *
 * PARAMETERS:  none
 *
 * RETURNS:		 Whether any items are marked for deletion.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	07/28/00	Initial Revision
 *
 ***********************************************************************/
static UInt16 PrvAttnHasDeleted(void)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	UInt16 index;

	for (index = 0; index < attnGlobalsP->numberOfAttentions; index++)
		if (attnGlobalsP->attentions[index].isDeleted)
			return true;
	
	return false;
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnClearChangeFlags
 *
 * DESCRIPTION: Clear the flags used to inform the UI about what changes
 *					 were made to the global state. This includes the isNew,
 *					 isUpdated, and isDeleted flags on all the attention requests.
 *					 Attention requests marked for deletion are removed.
 *					 The isNew flag is either cleared for all attentions or just
 *					 for a specified one.
 *
 * PARAMETERS:  attentionShown - attention whose isNew flag should be cleared,
 *											or allAttentions to clear them all
 *
 * RETURNS:		 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	07/25/00	Initial Revision
 *			peter	07/31/00	Update detailAttentionIndex if it moves
 *			gap	10/19/00 added parameter to the gotIt launch code
 *			peter	10/19/00	No need to update detailAttentionIndex any more
 *			peter	11/07/00	Notify app of deletion when marking rather than here
 *			peter	11/11/00	Add parameter to specify which isNew flag(s) to clear
 *
 ***********************************************************************/
static void PrvAttnClearChangeFlags(Int16 attentionShown)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	UInt16 readIndex, writeIndex, index;

	// Clear marks for addition and update before removing items marked
	// for deletion, so that the index parameter is still applicable.
	for (index = 0; index < attnGlobalsP->numberOfAttentions; index++)
	{
		if (index == attentionShown || attentionShown == allAttentions)
			attnGlobalsP->attentions[index].isNew = false;
		attnGlobalsP->attentions[index].isUpdated = false;
	}

	// Remove items marked for deletion.
	writeIndex = 0;
	for (readIndex = 0; readIndex < attnGlobalsP->numberOfAttentions; readIndex++)
	{
		if (!attnGlobalsP->attentions[readIndex].isDeleted)
		{
			if (readIndex != writeIndex)
				attnGlobalsP->attentions[writeIndex] = attnGlobalsP->attentions[readIndex];
			writeIndex++;
		}
	}
	
	// If any items were removed, free up the space no longer needed.
	if (attnGlobalsP->numberOfAttentions != writeIndex)
	{
		attnGlobalsP->numberOfAttentions = writeIndex;
		PrvAttnShrinkTable();
	}
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnListUpdateScrollArrows
 *
 * DESCRIPTION: This routine draws or erases the list view arrows if
 *					 necessary, updating them according to the number of
 *					 attentions in the list and which one is at the top of the
 *					 screen.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	07/13/00	Initial Revision
 *			gap	09/19/00	Replace scroll bar with scroll arrows
 *
 ***********************************************************************/
static void PrvAttnListUpdateScrollArrows (void)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	FormPtr frm;
	Int16 rows;
	UInt16 upIndex;
	UInt16 downIndex;
	Boolean scrollableUp		= false;
	Boolean scrollableDown	= false;


	rows = TblGetNumberOfRows (GetObjectPtr(AttentionListAttentionsTable));
	
	// Determine if there is more data that will fit on one attention manager
	// list view screen and enable/disable the scrollers accordingly. 
	if (attnGlobalsP->numberOfAttentions > rows)
		{
		// Attentions are displayed in the attention manager list view dialog
		// in the reverse order of their location in the attention list.
		// Therefore, if the topVisible attention is numberOfAttentions-1
		// (0-based indexes) the user has scrolled to the top of the list.
		scrollableUp = (attnGlobalsP->topVisibleAttention !=
			(attnGlobalsP->numberOfAttentions - 1));

		// user has scrolled to the bottom of the list if the index of the
		// topVisibleAttention is the number of rows -1.   
		scrollableDown = (attnGlobalsP->topVisibleAttention > rows-1);
		}
		
	
	frm = FrmGetActiveForm ();
	upIndex = FrmGetObjectIndex (frm, AttentionListUpButton);
	downIndex = FrmGetObjectIndex (frm, AttentionListDownButton);
	FrmUpdateScrollers (frm, upIndex, downIndex, scrollableUp, scrollableDown);
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnDetailDraw
 *
 * DESCRIPTION: This routine draws the description, time, duration, and 
 *              date of an event.
 *
 * PARAMETERS:  index of the one insistent attention request
 *
 * RETURNED:    nothing
 *
 *	HISTORY:
 *		07/07/00	peter	Initial Revision
 *
 ***********************************************************************/
static void PrvAttnDetailDraw (UInt16 attentionToDraw)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	RectangleType bounds, oldClipR, newClipR;
 	FormPtr frm;
 	UInt16 index;
 	AttnCommandArgsType args;
 	
	// Get the bounds of the gadget.
	frm = FrmGetActiveForm ();
	index = FrmGetObjectIndex (frm, AttentionDetailDescGadget);
	FrmGetObjectBounds (frm, index, &bounds);
	
	// Save the draw state and clip rect, then clip to gadget bounds.
	WinPushDrawState();
	FntSetFont(stdFont);
	WinGetClip (&oldClipR);
	RctGetIntersection (&bounds, &oldClipR, &newClipR);
	WinSetClip (&newClipR);

	// Get the application to draw the details.
	args.drawDetail.bounds = bounds;
	args.drawDetail.flags = PrvAttnFlags(attnGlobalsP->attentions[attentionToDraw].flags);
	args.drawDetail.firstTime = attnGlobalsP->attentions[attentionToDraw].isFirstTime;
	attnGlobalsP->attentions[attentionToDraw].isFirstTime = false;
	PrvAttnNotify(attentionToDraw, kAttnCommandDrawDetail, &args);
		
	// Restore the clip rect and draw state.
	WinSetClip (&oldClipR);	
	WinPopDrawState();
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnListDrawItem
 *
 * DESCRIPTION: Draw the description of an attention request.  This
 *              routine is called when the item needs to be drawn in
 *					 either the selected or non-selected state.
 *
 * PARAMETERS:  inTableP	- pointer to the table
 *              inRow		- row of the table to draw
 *              inColumn	- column of the table to draw
 *              inBoundsP	- pointer to region to draw in 
 *					 inSelected	- whether to highlight the item
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	07/11/00	Initial Revision
 *
 ***********************************************************************/
static void PrvAttnListDrawItem (void * inTableP, Int16 inRow, Int16 inColumn,
	RectangleType * inBoundsP, Boolean inSelected)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
 	AttnCommandArgsType args;
	UInt16 attentionToDraw;

	attentionToDraw = TblGetRowID (inTableP, inRow);

	// We don't want the app to draw in the space between attentions,
	// so we send them a rectangle inset 1 pixel at the top and bottom.
	RctCopyRectangle(inBoundsP, &args.drawList.bounds);
	args.drawList.bounds.topLeft.y += 1;
	args.drawList.bounds.extent.y -= pixelsBetweenAttentions;
	
	// Set up the colors so the app doesn't normally have to bother checking
	// whether it's drawing the item selected or not.
	WinSetBackColor(UIColorGetIndex(inSelected ? UIObjectSelectedFill : UIFieldBackground));
	WinSetForeColor(UIColorGetIndex(inSelected ? UIObjectSelectedForeground : UIObjectForeground));
	WinSetTextColor(UIColorGetIndex(inSelected ? UIObjectSelectedForeground : UIObjectForeground));
	
	// Fill the area with the background color to start.
	WinEraseRectangle(&args.drawList.bounds, 0);
	
	// Get the application to draw the details.
	args.drawList.flags = PrvAttnFlags(attnGlobalsP->attentions[attentionToDraw].flags);
	args.drawList.firstTime = attnGlobalsP->attentions[attentionToDraw].isFirstTime;
	args.drawList.selected = inSelected;
	attnGlobalsP->attentions[attentionToDraw].isFirstTime = false;
	PrvAttnNotify(attentionToDraw, kAttnCommandDrawList, &args);
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnListDrawItemDeselected
 *
 * DESCRIPTION: Draw the description of an attention request.  This
 *              routine is called by the table object as a callback 
 *              routine.
 *
 * PARAMETERS:  inTableP	- pointer to the table
 *              inRow		- row of the table to draw
 *              inColumn	- column of the table to draw
 *              inBoundsP	- pointer to region to draw in 
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	11/28/00	Initial Revision
 *
 ***********************************************************************/
static void PrvAttnListDrawItemDeselected (void * inTableP, Int16 inRow, Int16 inColumn,
	RectangleType * inBoundsP)
{
	PrvAttnListDrawItem(inTableP, inRow, inColumn, inBoundsP, false);
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnListDrawTable
 *
 * DESCRIPTION: Updates the entire list view, such as when changing categories 
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			rbb		4/14/99		Initial Revision
 *
 ***********************************************************************/
static void PrvAttnListDrawTable (void)
{
	TablePtr table;
	
	table = GetObjectPtr (AttentionListAttentionsTable);
	TblMarkTableInvalid (table);
	TblRedrawTable (table);
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnListLoadTable
 *
 * DESCRIPTION: Load the table in the list dialog. There can be no attentions
 *					 marked for deletion when this routine is called.
 *
 * PARAMETERS:  frm - pointer to the form
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	07/13/00	Initial Revision
 *
 ***********************************************************************/
static void PrvAttnListLoadTable (FormPtr frm)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	TablePtr 		table;
	UInt16			numRows;
	UInt16			row;
	Int16				attentionIndex;

	table = GetObjectPtr (AttentionListAttentionsTable);
	numRows = TblGetNumberOfRows (table);
	for (row = 0; row < numRows; row++)
	{
		attentionIndex = attnGlobalsP->topVisibleAttention - row;
		if (attentionIndex >= 0)
		{
			TblSetRowID (table, row, attentionIndex);
			TblSetRowUsable (table, row, true);

			// Clear the checkbox.
			TblSetItemInt (table, row, completedColumn, false);

			// Mark the row invalid so that it will draw when we call the 
			// draw routine.
			TblMarkRowInvalid (table, row);
		}
		else
			TblSetRowUsable (table, row, false);
	}
	
	PrvAttnListUpdateScrollArrows();
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnListScroll
 *
 * DESCRIPTION: This routine scrolls the list of attention requests
 *              in the specified direction.
 *
 * PARAMETERS:  direction - winUp or dowm
 *
 * RETURNED:    whether is actually scrolled
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	07/13/00	Initial Revision
 *
 ***********************************************************************/
static Boolean PrvAttnListScroll (WinDirectionType direction)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	TablePtr 			table;
	Int16 				rows;
	Int16					newTop;
	Int16					scrollMax;
	Int16					scrollAmount;

	table = GetObjectPtr (AttentionListAttentionsTable);
	TblReleaseFocus (table);
	
	rows	= TblGetNumberOfRows (GetObjectPtr(AttentionListAttentionsTable));
	
	if (attnGlobalsP->numberOfAttentions <= rows)
		return false;
	
	scrollAmount = rows-1; // -1 so at least one from previous set of attiontions will be at bottom or top of screen after scroll
	
	if (direction == winUp)
		{
		scrollMax = attnGlobalsP->numberOfAttentions - 1;
		
		if (attnGlobalsP->topVisibleAttention == scrollMax)
			return false;
			
		newTop = attnGlobalsP->topVisibleAttention + scrollAmount;
		if (newTop > scrollMax)
			newTop = scrollMax;
		}
	else
		{
		scrollMax = rows - 1;

		if (attnGlobalsP->topVisibleAttention == scrollMax)
			return false;
		
		newTop = attnGlobalsP->topVisibleAttention - scrollAmount;
		if (newTop < scrollMax)
			newTop = scrollMax;
		}
	
	attnGlobalsP->topVisibleAttention = newTop;
	
	// Refresh the table.
	PrvAttnListLoadTable (FrmGetActiveForm ());
	TblRedrawTable (GetObjectPtr (AttentionListAttentionsTable));
	
	return true;
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnListCrossOutAttention
 *
 * DESCRIPTION: This routine is called when an attention request is marked 
 *              complete.  We display an animation of a line being drawn
 *					 through the item.
 *
 * PARAMETERS:  row - row in table to be crossed out
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	07/13/00	Initial Revision
 *			peter	07/25/00	Draw line through each line, not just first
 *
 ***********************************************************************/
static void PrvAttnListCrossOutAttention (Int16 row)
{
	FontID curFont;
	Int16 lineHeight;
	TablePtr table;
	RectangleType bounds;
	Int16 x, y;
	UInt16 width;

	curFont = FntSetFont (stdFont);
	lineHeight = FntLineHeight ();	
	FntSetFont (curFont);

	table = GetObjectPtr (AttentionListAttentionsTable);
	TblGetItemBounds (table, row, descColumn, &bounds);	// Custom items are forced to 1 line
	bounds.extent.y = TblGetRowHeight(table, row);			// so override height here.
	y = bounds.topLeft.y + (lineHeight / 2);

	// Draw a line slowly from left to right, across each line in the item.
	while (y < bounds.topLeft.y + bounds.extent.y)
	{
		x = bounds.topLeft.x;
		width = bounds.extent.x - 1;
		while (width)
		{
			WinDrawLine (x, y, x, y);
			x++;
			width--;
		}
		y += lineHeight;
	}
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnListDismissAttention
 *
 * DESCRIPTION: This routine is called when an attention request is marked 
 *              complete. This routine will remove the item from the list.
 *
 * PARAMETERS:  row      - row in the table
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	07/13/00	Initial Revision
 *
 ***********************************************************************/
static void PrvAttnListDismissAttention (Int16 row)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	UInt16	attentionToRemove;
	UInt32	ticks;
	TablePtr	table;

	table = GetObjectPtr (AttentionListAttentionsTable);
	attentionToRemove = TblGetRowID (table, row);
	
	// Display an animation of a line being drawn through the item.
	ticks = TimGetTicks ();
	PrvAttnListCrossOutAttention (row);

	// Notify the application that the attention request is being removed and
	// then remove it.
	PrvAttnRemoveAttention (attentionToRemove);
	
	// Wait long enough for the user to see the cross-out animation
	while (TimGetTicks () - ticks < crossOutDelay)
		{
		}
		
	// Validate scroll position.
	if (attnGlobalsP->topVisibleAttention >= attnGlobalsP->numberOfAttentions)
		attnGlobalsP->topVisibleAttention = attnGlobalsP->numberOfAttentions - 1;
			
	// Refresh the table.
	PrvAttnListLoadTable (FrmGetActiveForm ());
	TblRedrawTable (GetObjectPtr (AttentionListAttentionsTable));
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnListClearAll
 *
 * DESCRIPTION: Dismiss all the attention requests, leaving an empty list. 
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	07/21/00	Initial Revision
 *
 ***********************************************************************/
static void PrvAttnListClearAll(void)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	TablePtr table;
	UInt32 ticks;
	Int16 index;		// signed for looping down to zero
	UInt16 numRows, row;
	
	// Cross out all the attentions.
	ticks = TimGetTicks ();
	table = GetObjectPtr (AttentionListAttentionsTable);
	numRows = TblGetNumberOfRows (table);
	for (row = 0; row < numRows; row++)
		if (TblRowUsable(table, row))
			PrvAttnListCrossOutAttention (row);
	
	// Wait long enough for the user to see the cross-out animation
	while (TimGetTicks () - ticks < crossOutDelay)
		{
		}

	// Remove the attentions in reverse order to avoid n squared time
	// to shift down remaining items.
	for (index = attnGlobalsP->numberOfAttentions - 1; index >= 0; index--)
		PrvAttnRemoveAttention(index);
	
	// Refresh the table.
	attnGlobalsP->topVisibleAttention = noAttention;
	PrvAttnListLoadTable (FrmGetActiveForm ());
	TblRedrawTable (table);
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnListInitDialog
 *
 * DESCRIPTION: Prepare the list dialog.
 *
 * PARAMETERS:  frm - pointer to the form
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	07/13/00	Initial Revision
 *
 ***********************************************************************/
static void PrvAttnListInitDialog (FormPtr frm)
{
	TablePtr						table;
	UInt16						row;
	UInt16						rowsInTable;
	FontID						oldFont;

	// Set up the table.
	table = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, AttentionListAttentionsTable));
	rowsInTable = TblGetNumberOfRows (table);
	for (row = 0; row < rowsInTable; row++)
	{		
		TblSetItemStyle (table, row, completedColumn, checkboxTableItem);
		TblSetItemStyle (table, row, descColumn, tallCustomTableItem);

		// Set the font used to draw the text of the row.
		TblSetItemFont (table, row, descColumn, stdFont);
		
		// Set the height to fit two lines in the standard font.
		oldFont = FntSetFont(stdFont);
		TblSetRowHeight (table, row, FntLineHeight() * 2 + pixelsBetweenAttentions);
		FntSetFont(oldFont);
	}
	TblSetColumnUsable (table, completedColumn, true);
	TblSetColumnUsable (table, descColumn, true);
	TblSetCustomDrawProcedure (table, descColumn, PrvAttnListDrawItemDeselected);
	
	PrvAttnListLoadTable(frm);
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnUpdateContentIfDateChanged
 *
 * DESCRIPTION: Compares the current date with date the current attention
 *					 manager dialog was last updated.  If the date has changed,
 *					 update the content of the dialog in the event that an app
 *					 has posted date specific content.  (ie. Date Book's "Today"
 *					 string)
 *
 *					 NOTE:
 *					 This code must execute BEFORE PrvAttnDrawDialogTitle as 
 *					 PrvAttnDrawDialogTitle updates attnGlobalsP->dateTimeShown.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			gap	10/25/00	Initial Revision
 *
 ***********************************************************************/
static void PrvAttnUpdateContentIfDateChanged()
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	DateTimeType	currDateTime;
	UInt16 index;
	
	TimSecondsToDateTime (TimGetSeconds(), &currDateTime);
	if (	(currDateTime.day != attnGlobalsP->dateTimeShown.day) ||
			(currDateTime.month != attnGlobalsP->dateTimeShown.month) ||
			(currDateTime.year != attnGlobalsP->dateTimeShown.year)
		)
		{
		switch (attnGlobalsP->currentUIState)
			{
			case attnDetailsDialog:
				PrvAttnDetailDraw (attnGlobalsP->detailAttentionIndex);
				break;
			case attnListDialog:
				PrvAttnListDrawTable();
				break;
			}

		// Now that all visible attentions have been redrawn, it's safe to
		// clear all the isUpdated flags, including those that are not visible.
		// If only the visible attentions had their flags cleared, when an
		// updated attention became visible, it'd be drawn twice.
		for (index = 0; index < attnGlobalsP->numberOfAttentions; index++)
			attnGlobalsP->attentions[index].isUpdated = false;
		}
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnRedrawUpdatedAttentions
 *
 * DESCRIPTION: Redraws any attentions that have been marked as needing
 *					 to be updated, and clears these marks. This routine is
 *					 called when processing nilEvents to ensure that the
 *					 device is on.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	1/12/01	Initial Revision
 *
 ***********************************************************************/
static void PrvAttnRedrawUpdatedAttentions()
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;

	UInt16 row, index;
	TablePtr table;
	
	switch (attnGlobalsP->currentUIState)
	{
		case attnDetailsDialog:
			if (attnGlobalsP->attentions[attnGlobalsP->detailAttentionIndex].isUpdated)
				// Just redraw the item.
				PrvAttnDetailDraw (attnGlobalsP->detailAttentionIndex);
			break;
		case attnListDialog:
			// Redraw the updated attentions which are visible on the screen.
			table = GetObjectPtr (AttentionListAttentionsTable);
			for (row = 0; row < TblGetNumberOfRows (table); row++)
			{
				if (TblRowUsable(table, row))
				{
					index = TblGetRowID (table, row);
					if (attnGlobalsP->attentions[index].isUpdated)
						// Mark the row invalid so that it will draw when we call the draw routine.
						TblMarkRowInvalid (table, row);
				}
			}
			
			// Redraw the invalidated portions of the table.
			TblRedrawTable (table);
			break;
	}
	
	// Now that all visible attentions have been redrawn, it's safe to
	// clear all the isUpdated flags, including those that are not visible.
	// If only the visible attentions had their flags cleared, when an
	// updated attention became visible, it'd be drawn twice.
	for (index = 0; index < attnGlobalsP->numberOfAttentions; index++)
		attnGlobalsP->attentions[index].isUpdated = false;
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnSwitchTo
 *
 * DESCRIPTION: Switch dialog modes (list to detail or vice versa), and
 *					 set up and draw everything for the new mode.
 *
 * PARAMETERS:  mode to switch to.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	07/27/00	Initial Revision
 *
 ***********************************************************************/
static void PrvAttnSwitchTo(UInt8 newUIState)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	FormPtr frm;
	
	if (attnGlobalsP->currentUIState == newUIState)
		return;
	
	frm = FrmGetActiveForm ();
	
	// Hide objects for the old state.
	switch(attnGlobalsP->currentUIState)
	{
		case attnDetailsDialog:
			FrmHideObject(frm, FrmGetObjectIndex (frm, AttentionDetailOkButton));
			FrmHideObject(frm, FrmGetObjectIndex (frm, AttentionDetailGotoButton));
			FrmHideObject(frm, FrmGetObjectIndex (frm, AttentionDetailSnoozeButton));
			FrmHideObject(frm, FrmGetObjectIndex (frm, AttentionDetailDescGadget));
			break;
		
		case attnListDialog:
		case attnNoDialog:			// Resources for list mode are initially usable.
			if (newUIState != attnListDialog)
			{
				FrmHideObject(frm, FrmGetObjectIndex (frm, AttentionListAttentionsTable));
				FrmHideObject(frm, FrmGetObjectIndex (frm, AttentionListUpButton));
				FrmHideObject(frm, FrmGetObjectIndex (frm, AttentionListDownButton));
				FrmHideObject(frm, FrmGetObjectIndex (frm, AttentionListOkButton));
				FrmHideObject(frm, FrmGetObjectIndex (frm, AttentionListSnoozeButton));
				FrmHideObject(frm, FrmGetObjectIndex (frm, AttentionListClearAllButton));
			}
			break;
	}
	
	// Show objects for the new state, then initialize and draw them.
	switch(newUIState)
	{
		case attnDetailsDialog:
			FrmShowObject(frm, FrmGetObjectIndex (frm, AttentionDetailOkButton));
			FrmShowObject(frm, FrmGetObjectIndex (frm, AttentionDetailGotoButton));
			FrmShowObject(frm, FrmGetObjectIndex (frm, AttentionDetailSnoozeButton));
			FrmShowObject(frm, FrmGetObjectIndex (frm, AttentionDetailDescGadget));

			FrmDrawForm (frm);
			PrvAttnDrawDialogTitle (&attnGlobalsP->dateTimeShown, true, newUIState);
			PrvAttnDetailDraw (attnGlobalsP->detailAttentionIndex);
			break;

		case attnListDialog:
			FrmShowObject(frm, FrmGetObjectIndex (frm, AttentionListAttentionsTable));
			FrmShowObject(frm, FrmGetObjectIndex (frm, AttentionListOkButton));
			FrmShowObject(frm, FrmGetObjectIndex (frm, AttentionListSnoozeButton));
			FrmShowObject(frm, FrmGetObjectIndex (frm, AttentionListClearAllButton));

 			// Do not explicitly show the arrows when initializing list view.  Let 
			// PrvAttnListUpdateScrollArrows enable them when necessary so that we
			// don't see the buttons flash when switching from detail to list view 
			// with 5 items or less  (need 6 or greater before scroll in enabled),
			//
			//FrmShowObject(frm, FrmGetObjectIndex (frm, AttentionListUpButton));
			//FrmShowObject(frm, FrmGetObjectIndex (frm, AttentionListDownButton));
			
			// Stop trying to maintain index of attention request being shown in detail.
			attnGlobalsP->detailAttentionIndex = noDetail;

			// Start scrolled to top.
			attnGlobalsP->topVisibleAttention = attnGlobalsP->numberOfAttentions - 1;

			PrvAttnListInitDialog(frm);
			FrmDrawForm (frm);
			PrvAttnDrawDialogTitle (&attnGlobalsP->dateTimeShown, true, newUIState);
			break;
	}
	
	attnGlobalsP->currentUIState = newUIState;
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnDoDialog
 *
 * DESCRIPTION: Put up the dialog and wait for the user to dismiss it,
 *					 returning how it was dismissed. The dialog will start
 *					 in either list or detail mode, and may switch modes while it is open.
 *
 * PARAMETERS:  whether to open in list or detail mode, and if detail, which attention to show
 *
 * RETURNED:    Button pressed by user, or AttentionListGotoButton if user
 *					 tapped on an item in the table to exit the dialog.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	07/27/00	Initial Revision
 *
 ***********************************************************************/
static UInt16 PrvAttnDoDialog (UInt8 newUIState, Int16 attentionToShow)
{
	AttnGlobalsType * attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	FormPtr curForm, frm;
	UInt16 buttonHit;
	
	frm = FrmInitForm (AttentionDialog);
	curForm = FrmGetActiveForm ();
	if (curForm)
		FrmSetActiveForm (frm);

	// Keep track of which attention is being shown in the detail dialog.
	if (newUIState == attnDetailsDialog)
		attnGlobalsP->detailAttentionIndex = attentionToShow;

	// Hide and show objects as necessary, then initialize the shown objects
	// and draw them.
	PrvAttnSwitchTo(newUIState);

	// Perform the special effects for new attentions.
	PrvAttnGetUsersAttentions();

	// Set the event handler for the dialog.
	FrmSetEventHandler (frm, PrvAttnHandleEvent);
	
	// The dialog's event loop. The dialog was already drawn above.
	buttonHit = FrmDoDialog (frm);
 	FrmDeleteForm (frm);
	FrmSetActiveForm (curForm);

	attnGlobalsP->currentUIState = attnNoDialog;

	return buttonHit;
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnOpen
 *
 * DESCRIPTION: Put up the dialog and wait for the user to
 *					 dismiss it, then act accordingly.
 *
 * PARAMETERS:  whether to show list or detail, and if detail, which attention to show
 *
 * RETURNED:    none
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	07/27/00	Initial Revision
 *
 ***********************************************************************/
static void PrvAttnOpen(UInt8 newUIState, Int16 attentionToShow)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	UInt16 buttonHit;
	SysNotifyParamType notify;
	WChar commandCharacter;
	Err err;
	
	// Flush the high level event queue. This queue might contain events
	// that can only be interpreted properly by the form underneath the
	// attention dialog, not by the dialog itself. For example, if the
	// user just tapped a button when an alarm goes off, the ctlSelectEvent
	// might still be in the queue, and it would not be properly interpreted
	// by the dialog.
	SysEventInitialize();
	
	attnGlobalsP->savedUIState = attnNoDialog;

	// Remember enough to be able to find attention in case it moves.
	if (newUIState == attnDetailsDialog)
	{
		attnGlobalsP->gotoCardNo = attnGlobalsP->attentions[attentionToShow].cardNo;
		attnGlobalsP->gotodbID = attnGlobalsP->attentions[attentionToShow].dbID;
		attnGlobalsP->gotoID = attnGlobalsP->attentions[attentionToShow].userData;
	}
	
	// Clear the isNew flag only on the one attention being shown if we're doing
	// a detail dialog, so that any new subtle attentions still around after the
	// dialog is closed will cause the attention indicator to blink properly.
	PrvAttnClearChangeFlags(newUIState == attnDetailsDialog ? attentionToShow : allAttentions);
	
	// Find where this attention moved (due to attentions marked for deletion being removed).
	if (newUIState == attnDetailsDialog)
		attentionToShow = PrvAttnFindAttention(attnGlobalsP->gotoCardNo,
															attnGlobalsP->gotodbID,
															attnGlobalsP->gotoID);
	
	// Tell system that we are using the UI. Open the dialog on top of any other dialogs
	// currently making the UI busy.
	SysUIBusy(true, true);

	buttonHit = PrvAttnDoDialog(newUIState, attentionToShow);

	// Release system UI Busy flag.
	SysUIBusy(true, false);
	
	// If the user caused the dialog to close, stop nagging.
	// If the dialog is being closed briefly so that the security lockout dialog
	// can open, keep nagging.
	if (buttonHit != AttentionCloseButton)
		PrvAttnStopNagging();

	switch (buttonHit)
	{
		case AttentionDetailGotoButton:
		case AttentionListGotoButton:
			PrvAttnGoto();				// If successful, most apps call AttnForgetIt, marking it deleted
			break;
		case AttentionDetailOkButton:
			// New subtle attentions may have been added, but that won't affect the index of
			// the attention being shown.
			
			// Notify the application that the attention request is being removed and
			// then remove it.
			PrvAttnRemoveAttention(attnGlobalsP->detailAttentionIndex);
			break;
		case AttentionDetailSnoozeButton:
			// DOLATER - gap  what do we do if setting the snooze alarm fails?
			PrvAttnSetSnoozeTimer(attnDetailsDialog);
			break;
		case AttentionListOkButton: // the Done button.
			PrvAttnDemoteAttentions();
			break;
		case AttentionListSnoozeButton:
			if (attnGlobalsP->numberOfAttentions > 0)		// Don't allow snooze on empty list.
				PrvAttnSetSnoozeTimer(attnListDialog);
			break;
		case AttentionCloseButton:
			// We're being asked to close so that the security lockout dialog
			// can be opened, after which we'll be asked to re-open.
			break;
		default:
			ErrNonFatalDisplay("Invalid button");
	}		

	// Turn the indicator on or off as required. Note that if a goto was just
	// completed successfully, the application should have called AttnForgetIt,
	// causing the attention to be marked as deleted. That means we can no longer
	// use numberOfAttentions to count attentions. If we were in detail mode, we
	// could also have new subtle attentions which were created either before or
	// after the dialog opened.
	PrvAttnUpdateIndicator();
	
#if	EMULATION_LEVEL == EMULATION_NONE
	// Unless there are new subtle attentions which the user hasn't seen because
	// they were in detail mode, we know the user has seen all attentions, so the
	// LED should be turned off.
	if (PrvAttnGetCounts(NULL, NULL, true) == 0)
		PrvAttnLED(attnLEDOff);
#endif
	
	if (buttonHit != AttentionCloseButton)
	{
		// Get the Alarm Manager to display any alarms which have expired by
		// posting a vchr to the key queue, just as the alarm manager itself does.
		// If no alarms have expired, this does nothing. If the queue is full at
		// this time, set our own flag to ensure we have another chance.
		if (EvtEnqueueKey(vchrAlarm, 0, commandKeyMask))
		{
			// Arrange to try again on next EvtGetEvent using deferred notification.
			notify.notifyType 		= sysNotifyRetryEnqueueKey;
			notify.broadcaster 		= sysNotifyBroadcasterCode;
			notify.handled 			= false;
			notify.notifyDetailsP 	= &commandCharacter;
			commandCharacter = vchrAlarm;
			err = SysNotifyBroadcastDeferred(&notify, sizeof(commandCharacter));
			ErrNonFatalDisplayIf(err != errNone, "Can't broadcast for retry enqueue key");
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnUnsnooze
 *
 * DESCRIPTION: The snooze timer has expired. Put up the appropriate
 *					 dialog and wait for it to close before returning. This
 *					 routine is called by AttnHandleEvent and PrvAttnHandleEvent
 *					 when they process a vchrAttnUnsnooze keyDownEvent.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    none
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	07/21/00	Initial Revision
 *
 ***********************************************************************/
static void PrvAttnUnsnooze(void)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	UInt16 totalCount, insistentCount;
	Int16 onlyInsistent, newestInsistent;

	// If all snoozed events have been removed from the attention manager 
	// queue during the snooze period, this routine wouldn't have been called.
	// However, the snoozed attentions could all be removed after the timer
	// expired but before this routine was called. We don't want to bring up
	// the dialog if there is nothing to show in it.
	totalCount = PrvAttnGetCounts(&insistentCount, NULL, false);
	if (totalCount == 0)
		return;
	
	// Now that we know we've got something to do, but before doing anything,
	// turn on the screen if it isn't already on, and make sure it stays on
	// for a while. We don't want to rely on being able to draw stuff on the
	// screen before we turn it back on.
	EvtResetAutoOffTimer();

	// Show the detail dialog if there is only one insistent item unless the
	// user entered the sleep state from the list view.
	if ( (insistentCount == 1) && (attnGlobalsP->snoozeUIState != attnListDialog) )
	{
		onlyInsistent = PrvAttnFindOnlyInsistent();
		
		// We want to return to the detail dialog, but a dialog might already
		// be open. If so, just leave that dialog open, whether it's detail
		// or list, and get the user's attention.
		if (attnGlobalsP->currentUIState == attnNoDialog)
			PrvAttnOpen(attnDetailsDialog, onlyInsistent);
		else
			// Play sound, etc to get the user's attention.
			PrvAttnGetUsersAttentions();
	}
	else
	{
		newestInsistent = PrvAttnFindMostRecentInsistent();	// May be subtle if no insistent requests.
				
		// We want to return to the list dialog, but a dialog might already
		// be open.
		switch (attnGlobalsP->currentUIState)
		{
			case attnNoDialog:
				// Open the list. This will cause the sound and other
				// special effects to be played as well.
				PrvAttnOpen(attnListDialog, noAttention);
				return;
			
			case attnDetailsDialog:
				// Switch to the list.
				PrvAttnClearChangeFlags(allAttentions);
				PrvAttnSwitchTo(attnListDialog);

				// Arrange to play sound, etc to get the user's attention.
				// Rather than calling PrvAttnGetUsersAttentions directly,
				// we do this so that the vchrLateWakeup key placed in the
				// key queue by the call to EvtResetAutoOffTimer above has
				// time to be processed and removed before the sound starts
				// playing. A non-empty key queue stops the sound.
				PrvAttnStateChanged();
				break;
			
			case attnListDialog:
				// The list is already showing.
				//DOLATER - peter: Consider scrolling down to show the snoozed attentions.

				// Arrange to play sound, etc to get the user's attention, as in above case.
				PrvAttnStateChanged();
				break;
		}
	}
}


/***********************************************************************
 *
 * FUNCTION: 	 PrvAttnSelectDescription
 *
 * DESCRIPTION: This routine is called when a pen down event occurs in
 *              the attention table. This routine will track the pen
 *           	 until it is released. This is done instead of allowing
 *					 the table to deal with highlighting the row in order to
 *					 get better highlighting on color screens.
 *
 * PARAMETERS:	 eventP - the event that started it all
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	11/28/00	Initial Revision
 *
 ***********************************************************************/
static void PrvAttnSelectDescription(EventType * eventP)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	TableType * tableP = eventP->data.tblEnter.pTable;
	Int16 row = eventP->data.tblEnter.row;
	Int16 column = eventP->data.tblEnter.column;
	RectangleType r;
	Coord x, y;
	Boolean penDown = true;
	Boolean selected = true;
	EventType newEvent;
	UInt16 attentionToGoto;
	
	ErrNonFatalDisplayIf (row >= tableP->numRows, "Invalid tableP row");
	ErrNonFatalDisplayIf (column >= tableP->numColumns, "Invalid tableP column");
	
	// Get the bounds of the table item.
	TblGetItemBounds (tableP, row, column, &r);
	
	if (! RctPtInRectangle(eventP->screenX, eventP->screenY, &r))
		return;	
	
	WinPushDrawState();

	PrvAttnListDrawItem(tableP, row, column, &r, true);

	do {
		PenGetPoint (&x, &y, &penDown);

		if (RctPtInRectangle (x, y, &r)) {
			if (! selected)
				PrvAttnListDrawItem(tableP, row, column, &r, selected = true);
		}

		else if (selected) {
			PrvAttnListDrawItem(tableP, row, column, &r, selected = false);
		}
	} while (penDown);
		
	if (selected) {
		attentionToGoto = TblGetRowID (tableP, row);
		
		// We need to close the dialog before we do the goto.
		// Remember which attention request the user tapped on. Because we're going
		// to go around the event loop, it's possible for the attention request to
		// move in the list, or even be deleted altogether, so we can't use an index
		// to refer to it.
		attnGlobalsP->gotoCardNo = attnGlobalsP->attentions[attentionToGoto].cardNo;
		attnGlobalsP->gotodbID = attnGlobalsP->attentions[attentionToGoto].dbID;
		attnGlobalsP->gotoID = attnGlobalsP->attentions[attentionToGoto].userData;
		
		// Press the invisible Goto button to get the FrmDoDialog event loop to exit.
		MemSet(&newEvent, sizeof(newEvent), 0);
		newEvent.eType = ctlSelectEvent;
		newEvent.data.ctlSelect.controlID = AttentionListGotoButton;
		newEvent.data.ctlSelect.pControl = GetObjectPtr(newEvent.data.ctlSelect.controlID);
		EvtAddEventToQueue(&newEvent);
	}
	
	WinPopDrawState();
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnUpdateIfNeeded
 *
 * DESCRIPTION: Either the device was just turned on or a minute has
 *					 elapsed since the last time this routine was called.
 *					 In either case, it's time to check for things that need
 *					 to be updated. The time is updated in the top left
 *					 corner of the screen every minute, all attentions are
 *					 redrawn at midnight so that they can use terms like
 *					 "today" to refer to dates, and any attentions which
 *					 were updated via AttnUpdate are redrawn here, so that
 *					 they never get drawn while the screen is off.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	01/16/01	Initial Revision
 *
 ***********************************************************************/
static void PrvAttnUpdateIfNeeded(void)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	
	// If an attention manager dialog is open and the date has changed
	// update the current display in the event the application has displayed
	// any date specific text (ie. DateBook's list view "Today" string)
	//	This code must execute BEFORE PrvAttnDrawDialogTitle as PrvAttnDrawDialogTitle
	// updates attnGlobalsP->dateTimeShown. This code should execute BEFORE
	// PrvAttnRedrawUpdatedAttentions to avoid a double-update if an update
	// occurs just before the date changes.
	PrvAttnUpdateContentIfDateChanged();
	
	// Refresh time when minute changes.
	PrvAttnDrawDialogTitle(&attnGlobalsP->dateTimeShown, false, attnGlobalsP->currentUIState);
	
	// Update any attentions that are marked as needing to be updated while
	// also being visible on the screen. This is done here so that it only
	// occurs when the device is on, delaying if necessary if the device is
	// off when AttnUpdate is called.
	PrvAttnRedrawUpdatedAttentions();
}


/***********************************************************************
 *
 * FUNCTION:    PrvAtHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the attention
 *					 manager dialog.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	07/07/00	Initial Revision
 *			gap	10/25/00	Add an update the current attn mgr display when
 *								it is open and the device time crosses midnight.
 *
 ***********************************************************************/
static Boolean PrvAttnHandleEvent (EventType * event)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	FormPtr frm;
	TablePtr table;
	EventType newEvent;
	UInt16 newCount, newInsistentCount;
	WChar hardButton;
	UInt16 controlID;
	
	// We don't want to allow another application to be launched 
	// while this dialog is displayed, so we intercept appStopEvents,
	// except when running under simulation, or when trying to close
	// in order to let the system lockout dialog come up after a call
	// to AttnClose().
	if (event->eType == appStopEvent)
	{
#if	EMULATION_LEVEL == EMULATION_NONE
		if (attnGlobalsP->savedUIState != attnGlobalsP->currentUIState)
			return true;
#endif
	}
	
	// An item in the table has been tapped on (not the check-box).
	// Deal with highlighting it while the pen is over it, rather than
	// letting the table do its standard color swapping trick, so that
	// icons are always shown with their proper colors.
	else if (event->eType == tblEnterEvent &&
				event->data.tblEnter.tableID == AttentionListAttentionsTable &&
				event->data.tblEnter.column == descColumn)
	{
		PrvAttnSelectDescription(event);
		return true;		// Don't let table handle the event.
	}

	// A check-box has been checked.
	else if (event->eType == tblSelectEvent &&
				event->data.tblSelect.tableID == AttentionListAttentionsTable &&
				event->data.tblSelect.column == completedColumn)
	{
		Int16 on = TblGetItemInt (event->data.tblSelect.pTable,
			event->data.tblEnter.row, event->data.tblSelect.column);
		ErrNonFatalDisplayIf(!on,
			"Checked off items should be removed, so unchecking should never occur");
		PrvAttnListDismissAttention (event->data.tblSelect.row);
	}
	
	else if (event->eType == ctlSelectEvent)
	{
		if (event->data.ctlSelect.controlID == AttentionListClearAllButton)
		{
			PrvAttnListClearAll();
			return true;			// Don't let FrmDoDialog handle event or it'll close dialog
		}
	}
	
	else if (event->eType == ctlRepeatEvent)
		{
		switch (event->data.ctlRepeat.controlID)
			{
         case AttentionListUpButton:
				PrvAttnListScroll (winUp);
				// leave unhandled so the buttons can repeat
            break;
				
         case AttentionListDownButton:
				PrvAttnListScroll (winDown);
				// leave unhandled so the buttons can repeat
            break;
			}
		}
	
	else if (event->eType == frmUpdateEvent)
	{
		frm = FrmGetActiveForm ();
		FrmDrawForm (frm);
		PrvAttnDrawDialogTitle (&attnGlobalsP->dateTimeShown, true, attnGlobalsP->currentUIState);
		switch (attnGlobalsP->currentUIState)
		{
			case attnDetailsDialog:
				PrvAttnDetailDraw (attnGlobalsP->detailAttentionIndex);
				break;
			case attnListDialog:
				TblRedrawTable (GetObjectPtr (AttentionListAttentionsTable));
				break;
		}
		return true;
	}
	
	// Refresh time, etc.
	else if (event->eType == nilEvent)
		PrvAttnUpdateIfNeeded();

	else if (event->eType == keyDownEvent && EvtKeydownIsVirtual(event))
	{
		// First, check if it's the optional snooze hard button.
		switch (attnGlobalsP->currentUIState)
		{
			case attnDetailsDialog:
				hardButton = ResLoadConstant(AttentionDetailSnoozeHardButton);
				controlID = AttentionDetailSnoozeButton;
				break;
			case attnListDialog:
				hardButton = ResLoadConstant(AttentionListSnoozeHardButton);
				controlID = AttentionListSnoozeButton;
				break;
		}
		if (hardButton != 0 && event->data.keyDown.chr == hardButton)
		{
			// Press the Snooze button to get the FrmDoDialog event loop to exit.
			MemSet (&newEvent, sizeof(newEvent), 0);
			newEvent.eType = ctlSelectEvent;
			newEvent.data.ctlSelect.controlID = controlID;
			newEvent.data.ctlSelect.pControl = GetObjectPtr(controlID);
			EvtAddEventToQueue(&newEvent);
		}

		if (!TxtCharIsHardKey(event->data.keyDown.modifiers, event->data.keyDown.chr))
		{
			if (event->data.keyDown.chr == vchrLateWakeup)
				PrvAttnUpdateIfNeeded();
			else if (event->data.keyDown.chr == vchrPageUp)
			{
				switch (attnGlobalsP->currentUIState)
				{
					case attnDetailsDialog:
						// disable 'up' scroll arrow from ticking while our detail dialog is up.
						return true;
					case attnListDialog:
						// In Palm OS 3.5 and earlier, the Datebook alarm dialog disabled the 'up'
						// scroll button from ticking. This was necessary to avoid hearing the sound
						// when the up button was pressed by the leather cover of the Palm V.
						// In this case, we really need the scroll buttons to scroll the list, but
						// we can keep them from ticking when they don't actually have any effect.
						// Since the dialog opens scrolled to the top, this means the 'up' scroll
						// button won't tick if pressed in the default state.				
						return !PrvAttnListScroll(winUp);
				}
			}
			else if (event->data.keyDown.chr == vchrPageDown)
			{
				if (attnGlobalsP->currentUIState == attnListDialog)
					return !PrvAttnListScroll(winDown);
			}
			else if (event->data.keyDown.chr == vchrAttnStateChanged)
			{
				// The event was successfully delivered.
				attnGlobalsP->stateChangeInTransit = false;
				
				frm = FrmGetActiveForm ();
				switch (attnGlobalsP->currentUIState)
				{
					case attnDetailsDialog:
						// The attentions changed. Check whether we need to switch modes. Don't try
						// to display new subtle requests, but switch to the list for new insistent
						// requests, or if the insistent request being shown is removed.
						PrvAttnGetCounts(&newInsistentCount, NULL, true);
						if (newInsistentCount > 0 ||
							attnGlobalsP->attentions[attnGlobalsP->detailAttentionIndex].isDeleted)
						{
							PrvAttnClearChangeFlags(allAttentions);
							PrvAttnSwitchTo(attnListDialog);
						}
						else
						{
							// Ignore all other changes, but don't clear them out. That way, when
							// the dialog closes, if there are still new subtle attentions present,
							// it can make the indicator and LED blink accordingly.
							
							// Delay the handling of updates if the device is off, but update
							// right away if the device is on..
							EvtSetNullEventTick(TimGetTicks());
						}
						break;

					case attnListDialog:
						// The list changed in some way; find out how.
						table = GetObjectPtr (AttentionListAttentionsTable);
						newCount = PrvAttnGetCounts(NULL, NULL, true);
						if (newCount > 0)
						{
							// AttnGetAttention was called at least once, so we need to scroll to
							// the top and reload the entire table to show the user the new items.
							PrvAttnClearChangeFlags(allAttentions);
							attnGlobalsP->topVisibleAttention = attnGlobalsP->numberOfAttentions - 1;
							PrvAttnListLoadTable (frm);
						}
						else if (PrvAttnHasDeleted())
						{
							// AttnForgetIt was called at least once, so we need to validate the
							// scroll position and reload the entire table.
							PrvAttnClearChangeFlags(allAttentions);
							if (attnGlobalsP->topVisibleAttention > attnGlobalsP->numberOfAttentions - 1)
								attnGlobalsP->topVisibleAttention = attnGlobalsP->numberOfAttentions - 1;
							PrvAttnListLoadTable (frm);
						}
						else
						{
							// No calls were made to AttnGetAttention or AttnForgetIt, but AttnUpdate
							// may have been called (possibly more than once). We need to redraw
							// the updated attentions which are visible on the screen.
							
							// Delay the handling of updates if the device is off, but update
							// right away if the device is on.
							EvtSetNullEventTick(TimGetTicks());
							break;		// Don't redraw the table.
						}

						// Redraw the invalidated portions of the table.
						TblRedrawTable (table);
						break;
				}
				
				// Perform the special effects for new attentions.
				PrvAttnGetUsersAttentions();
				
				return true;
			}
			else if (event->data.keyDown.chr == vchrAttnUnsnooze)
			{
				// The event was successfully delivered.
				attnGlobalsP->unsnoozeInTransit = false;

				// The snooze timer expired while the dialog was open.
				// We still need to play the sound, but we don't need to
				// open the dialog. We may need to switch from detail to
				// list mode to show all the snoozed attentions.
				PrvAttnUnsnooze();
				return true;
			}
			else if (event->data.keyDown.chr == vchrAttnIndicatorTapped)
			{
				// It shouldn't be possible to tap on the indicator
				// while the attention dialog is up, but just to be
				// safe, we ignore the event without allowing SysHandleEvent
				// to get it.
				ErrNonFatalDisplay("Attn Mgr dialog already open");
				return true;
			}
			else if (event->data.keyDown.chr == vchrAttnAllowClose)
			{
				// The dialog is up, so we need to remember whether we're
				// in list or detail mode and then close the dialog so that
				// the security dialog can open before the attention dialog
				// is re-opened.
				attnGlobalsP->savedUIState = attnGlobalsP->currentUIState;

				// We want to press the invisible Close button to get the
				// FrmDoDialog() event loop to exit, but we can't simply
				// post a ctlSelectEvent to do that, because such an event
				// would be processed AFTER the appStopEvent put in the
				// queue by the caller of AttnClose(). Instead, we use the
				// savedUIState to remember that we're trying to close,
				// and let the appStopEvent be handled normally in this
				// case, causing the hidden close button to be pressed
				// because it's the default.
			
				return true;
			}
			else if (event->data.keyDown.chr == vchrAlarm)
			{
				// The alarm manager wants to open the alarm dialog(s).
				// We don't want this to happen when the attention dialog
				// is open because we want the attentions to stay in front,
				// letting the user see the most recent stuff from the new
				// applications first. So, we simply don't let SysHandleEvent
				// get this event. When we ask the alarm dialog(s) to open
				// if there are any.
				return true;
			}
		}
	}
	
	SendNullEventNextMinute();
	return false;
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnIndicatorBlinkInterval
 *
 * DESCRIPTION: This routine computes the interval before the next blink
 *					 of the attention indicator occurs.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    ticks between last blink and next blink
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	09/08/00	Initial Revision
 *
 ***********************************************************************/
static UInt32 PrvAttnIndicatorBlinkInterval(void)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;

	switch (attnGlobalsP->indicatorBlinkPattern)
	{
		case kAttnIndicatorBlinkPatternNone:
			return kAttnIndicatorOffRetryTime;
				// Keep trying to turn it off.
		case kAttnIndicatorBlinkPatternOld:
			if (attnGlobalsP->indicatorFrame != kAttnIndicatorFrameOff)
				return kAttnIndicatorOldOnTime;
			else
				return kAttnIndicatorOldOffTime;
		case kAttnIndicatorBlinkPatternNew:
			switch (attnGlobalsP->indicatorFrame)
			{
				case kAttnIndicatorFrameOff:
					return kAttnIndicatorNewOffTime;
				case kAttnIndicatorFrameOn:
					return kAttnIndicatorNewOnTime;
				case kAttnIndicatorFrameNew:
					return kAttnIndicatorNewTime;
			}
		default:
			ErrNonFatalDisplay("Invalid pattern");
			return 0;
	}
}


/***********************************************************************
 *
 * FUNCTION:    PrvAttnIndicatorSwitchToFrame
 *
 * DESCRIPTION: This routine draws a specific frame in the attention
 *					 indicator animation.
 *
 * PARAMETERS:  frame - index of frame to show
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	10/20/00	Initial Revision
 *
 ***********************************************************************/
static void PrvAttnIndicatorSwitchToFrame(UInt16 frame)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	MemHandle resH;
	BitmapPtr resP;
	RectangleType indicatorBounds;
	UInt16 error;
	WinHandle drawWindow;
	IndexedColorType oldFore, oldBack;
	
	if (attnGlobalsP->indicatorFrame == frame)
		return;
	drawWindow = WinSetDrawWindow(WinGetDisplayWindow());
	switch (frame)
	{
		case kAttnIndicatorFrameOff:
			// Restore the bits behind the indicator.
			WinRestoreBits(attnGlobalsP->indicatorBitsBehind, kAttnIndicatorLeft, kAttnIndicatorTop);
			break;

		case kAttnIndicatorFrameOn:
			RctSetRectangle(&indicatorBounds,
				kAttnIndicatorLeft, kAttnIndicatorTop, kAttnIndicatorWidth, kAttnIndicatorHeight);
			resH = DmGetResource(bitmapRsc, AttentionIndicatorBitmap);
			ErrNonFatalDisplayIf(resH == NULL, "Couldn't find indicator bitmap");
			
			// Save the area under the indicator so we can restore it later.
			attnGlobalsP->indicatorBitsBehind = WinSaveBits(&indicatorBounds, &error);
			ErrNonFatalDisplayIf(error, "Couldn't save bits under indicator");
			
			// Set the owner of indicatorBitsBehind's window and its associated bitmap to the system.  
			// This is needed for badly behaved apps that do not close forms when exiting (which would   
			// call AttnIndicatorAllow() and restore the bits).  Otherwise, the system deallocates the 
			// application-owned bits in SysAppExit(), and crashes when trying to restore them.
			MemPtrSetOwner(((WinPtr) attnGlobalsP->indicatorBitsBehind)->bitmapP, 0);
			MemPtrSetOwner(attnGlobalsP->indicatorBitsBehind, 0);
			
			// Draw the indicator bitmap over the form title.
			resP = MemHandleLock(resH);
			oldFore = WinSetForeColor(UIColorGetIndex(UIFormFrame));
			oldBack = WinSetBackColor(UIColorGetIndex(UIFormFill));
			WinDrawBitmap(resP, kAttnIndicatorLeft, kAttnIndicatorTop);
			WinSetForeColor(oldFore);
			WinSetBackColor(oldBack);
			MemPtrUnlock(resP);
			DmReleaseResource(resH);
			break;
		
		case kAttnIndicatorFrameNew:
			resH = DmGetResource(bitmapRsc, AttentionIndicatorNewBitmap);
			ErrNonFatalDisplayIf(resH == NULL, "Couldn't find indicator bitmap");
			
			// Draw the new frame over the last frame.
			resP = MemHandleLock(resH);
			oldFore = WinSetForeColor(UIColorGetIndex(UIFormFrame));
			oldBack = WinSetBackColor(UIColorGetIndex(UIFormFill));
			WinDrawBitmap(resP, kAttnIndicatorLeft, kAttnIndicatorTop);
			WinSetForeColor(oldFore);
			WinSetBackColor(oldBack);
			MemPtrUnlock(resP);
			DmReleaseResource(resH);
			break;
		
	}
	WinSetDrawWindow(drawWindow);
	attnGlobalsP->indicatorFrame = frame;
}

#pragma mark -		// Private traps used by OS

/***********************************************************************
 *
 * FUNCTION:    AttnInitialize
 *
 * DESCRIPTION: This routine initializes the attention manager.
 *
 * PARAMETERS:  nothing
 *
 * RETURNS:		 errNone if successful; otherwise { attnErrMemory, etc }
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	06/12/00	Initial Revision
 *
 ***********************************************************************/
Err AttnInitialize(void)
{
	//Err err;
	AttnGlobalsType * attnGlobalsP = NULL;
	MemHandle attentionsH = NULL;
	Err err;
	UInt32 capabilities;
	Boolean hasVibrate, hasLED;
#if	EMULATION_LEVEL == EMULATION_NONE
	Boolean vibrateActive, ledActive;
#endif

	// We shouldn't be called more than once without being freed in between.
	ErrFatalDisplayIf(GAttnGlobalsP, "already initialized");
	
	// Allocate and initialize the Attention Manager Globals
	attnGlobalsP = (AttnGlobalsType *)MemPtrNew(sizeof(AttnGlobalsType));
	if (!attnGlobalsP)
		goto ErrorMem;
	MemSet(attnGlobalsP, sizeof(AttnGlobalsType), 0);	// initialize
	attnGlobalsP->numberOfAttentions = 0;
	attnGlobalsP->topVisibleAttention = noAttention;
	attnGlobalsP->currentUIState = attnNoDialog;
	attnGlobalsP->snoozing = false;
	attnGlobalsP->waitingToNag = false;
	attnGlobalsP->stateChangeInTransit = false;
	attnGlobalsP->unsnoozeInTransit = false;
	attnGlobalsP->hasStateChanged = false;
	attnGlobalsP->detailAttentionIndex = noDetail;
	attnGlobalsP->attentionsH = NULL;
	attnGlobalsP->attentions = NULL;
	attnGlobalsP->indicatorIsAllowed = false;				// not until a form draws
	attnGlobalsP->indicatorIsEnabled = true;				// enabled until app disables
	attnGlobalsP->indicatorFrame = kAttnIndicatorFrameOff;
	attnGlobalsP->indicatorLastTick = 0;
	attnGlobalsP->indicatorBlinkPattern = kAttnIndicatorBlinkPatternNone;
	
	// Link system globals to Attention Manager globals
	GAttnGlobalsP = (MemPtr)attnGlobalsP;

	// Set the feature used to inform applications about the capabilities
	// of the hardware for getting the user's attention and about the current
	// user preferences (read only).
#if	EMULATION_LEVEL == EMULATION_NONE
	// Check whether the hardware has an LED and/or vibrator.
	hasLED = (HwrLEDAttributes(false, kHwrLEDActive, &ledActive) != kHwrErrNotSupported);
	hasVibrate = (HwrVibrateAttributes(false, kHwrVibrateActive, &vibrateActive) != kHwrErrNotSupported);
#else
	hasLED = false;			// simulator doesn't have an LED
	hasVibrate = false;		// simulator doesn't have a vibrator
#endif
	capabilities = PrefGetPreference(prefAttentionFlags);
	capabilities |= kAttnFlagsHasSound;		// all devices have sound
	if (hasLED)
		capabilities = capabilities | kAttnFlagsHasLED;
	if (hasVibrate)
		capabilities = capabilities | kAttnFlagsHasVibrate;
	err = FtrSet(kAttnFtrCreator, kAttnFtrCapabilities, capabilities);
	if (err != errNone)
		return attnErrMemory;

	err = SysNotifyRegister(0, sysNotifyBroadcasterCode, sysNotifyRetryEnqueueKey,
		&PrvAttnRetryEnqueueKey, 0, 0);
	ErrNonFatalDisplayIf(err != errNone, "Can't register for retry enqueue key");

// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.

	err = SysNotifyRegister(0, sysNotifyBroadcasterCode, sysNotifyTimeChangeEvent,
		&PrvAttnTimeChanged, 0, 0);
	ErrNonFatalDisplayIf(err != errNone, "Can't register for time change");

	err = SysNotifyRegister(0, sysNotifyBroadcasterCode, sysNotifyDBDeletedEvent,
		&PrvAttnDatabaseRemoved, 0, 0);
	ErrNonFatalDisplayIf(err != errNone, "Can't register for DB deleted");

	return err;

ErrorMem:
	// Free anything allocated here.
	if (attnGlobalsP)
		MemPtrFree (attnGlobalsP);
	if (attentionsH)
		MemHandleFree (attentionsH);

	return attnErrMemory;
}


/***********************************************************************
 *
 * FUNCTION:    AttnPerformEmergencySpecialEffects
 *
 * DESCRIPTION: The attention dialog normally plays sounds and performs
 *					 other special effects after it opens. This allows long
 *					 sounds to be played. However, if it can't open for some
 *					 reason, it's imperative that the sound be played. This
 *					 routine is called periodically to play any sounds when
 *					 this happens.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    none
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	10/13/00	Initial Revision
 *
 ***********************************************************************/
void AttnDoEmergencySpecialEffects(void)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	
	if (attnGlobalsP->stateChangeInTransit || attnGlobalsP->unsnoozeInTransit)
	{
		// The event was not delivered, but we took care of it.
		attnGlobalsP->stateChangeInTransit = false;
		attnGlobalsP->unsnoozeInTransit = false;

		// Perform the special effects for new attentions.
		PrvAttnGetUsersAttentions();
		
		// For insistent attentions, we don't want to make the indicator start
		// blinking when we get the request because we want it off when the
		// dialog opens, so the saved bits don't include it. Otherwise, when
		// the dialog closes, the indicator would appear briefly. So, we turn
		// the indicator off when we get a new insistent attention. If it fails
		// to open for any reason, we'd better make the indicator blink so that
		// the user has some indication that the device needs their attention
		// if they aren't present to see/hear/feel the special effects just
		// performed.
		PrvAttnUpdateIndicator();
	}
}


/***********************************************************************
 *
 * FUNCTION:    AttnAllowClose
 *
 * DESCRIPTION: Arranges for the attention dialog to close if it is open,
 *					 and remembers whether it was open, and whether it was in
 *					 list or detail mode. Call AttnReopen to restore. This
 *					 routine just posts an event that will cause the dialog to
 *					 close when it later receives an appStopEvent, so the
 *					 dialog is not closed when this routine returns.
 *					 These are used by security lockout to get the attention
 *					 dialog to be on top of the lockout dialog, even when the
 *					 lockout occurs when the attention dialog is open.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    none
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	08/25/00	Initial Revision
 *			peter	09/18/00	Rename from AttnClose() to AttnAllowClose()
 *
 ***********************************************************************/
void AttnAllowClose(void)
{
	EventType newEvent;

	// Add a vchrAttnAllowClose event to get the user interface to close if necessary.
	MemSet (&newEvent, sizeof(EventType), 0);
	newEvent.eType = keyDownEvent;
	newEvent.data.keyDown.chr = vchrAttnAllowClose;
	newEvent.data.keyDown.keyCode = 0;
	newEvent.data.keyDown.modifiers = commandKeyMask;
	EvtAddEventToQueue (&newEvent);
}	


/***********************************************************************
 *
 * FUNCTION:    AttnReopen
 *
 * DESCRIPTION: Reopens the attention dialog if it was open when AttnClose
 *					 was called, restoring it to the same mode it was in (if
 *					 possible). This routine just posts an event that will cause
 *					 the dialog to open, so the dialog is not opened when this
 *					 routine returns. This is used by security lockout to get the
 *					 attention dialog to be on top of the lockout dialog, even
 *					 when the lockout occurs when the attention dialog is open.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    none
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	08/25/00	Initial Revision
 *			peter	09/18/00	Rename from AttnOpen() to AttnReopen()
 *
 ***********************************************************************/
void AttnReopen(void)
{
	EventType newEvent;

	// Add a vchrAttnOpen event to get the user interface to reopen if necessary.
	MemSet (&newEvent, sizeof(EventType), 0);
	newEvent.eType = keyDownEvent;
	newEvent.data.keyDown.chr = vchrAttnReopen;
	newEvent.data.keyDown.keyCode = 0;
	newEvent.data.keyDown.modifiers = commandKeyMask;
	EvtAddEventToQueue (&newEvent);
}	


/***********************************************************************
 *
 * FUNCTION:    AttnEffectOfEvent
 *
 * DESCRIPTION: Determine what AttnHandleEvent() will do when called with
 *					 this virtual character event. This API is used by LstPopupList()
 *					 to decide whether the popup list needs to close before the
 *					 event is handled. This is necessary whenever a dialog is
 *					 being opened. In many cases, the vchrAlarm event would occur,
 *					 which would cause the popup list to close, but not if a
 *					 procedure alarm or wireless packet receipt triggers a call to
 *					 AttnGetAttention().
 *
 * PARAMETERS:  pointer to system event
 *
 * RETURNED:    which dialog will be opened when AttnHandleEvent() is called
 *					 with this event, if any
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	08/31/00	Initial Revision
 *
 ***********************************************************************/
UInt8 AttnEffectOfEvent(EventType * eventP)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	UInt16 newCount, newInsistentCount, insistentCount;

	switch (eventP->data.keyDown.chr)
	{
		case vchrAttnStateChanged:
			// If the dialog is open, let it handle the event. If the dialog is in the wrong mode,
			// it'll react by switching to the other mode.
			if (attnGlobalsP->currentUIState != attnNoDialog)
				return attnNoDialog;
			
			// There are three kinds of state changes that can be made, and they can be combined.
			// They include adding new attentions, updating existing ones, and removing existing
			// ones. We need to use this information to decide whether to open the dialog, and if
			// so, in what mode, and if in detail mode, which attention to show.
			
			// If new insistent attention requests were added, show them. Don't open the dialog
			// to show subtle attention requests; they only show up if you happen to already be
			// showing the dialog in list mode.
			newCount = PrvAttnGetCounts(&newInsistentCount, NULL, true);
			if (newInsistentCount == 1)
				// Put up the dialog in detail mode to show the one new insistent attention request.
				return attnDetailsDialog;
			else if (newInsistentCount > 1)
				// Put up the dialog in list mode to show multiple insistent attention requests.
				return attnListDialog;
			else
				return attnNoDialog;

		case vchrAttnUnsnooze:
			// If the attention dialog is already open, then it'll stay open.
			if (attnGlobalsP->currentUIState != attnNoDialog)
				return attnNoDialog;
			// Show the detail dialog if there is only one insistent item unless the
			// user entered the sleep state from the list view.
			PrvAttnGetCounts(&insistentCount, NULL, false);
			if ( (insistentCount == 1) && (attnGlobalsP->snoozeUIState != attnListDialog) )
				// We want to return to the detail dialog, but a dialog might already
				// be open. If so, just leave that dialog open, whether it's detail
				// or list.
				return attnDetailsDialog;
			else
				// We want to return to the list dialog, but a dialog might already
				// be open.
				return attnListDialog;

		case vchrAttnIndicatorTapped:
			return attnListDialog;
		
		case vchrAttnAllowClose:
			return attnNoDialog;

		case vchrAttnReopen:
			// Reopen the dialog in the same mode it was in. No other
			// attention manager calls should have been made in the interim.
			if (attnGlobalsP->savedUIState == attnDetailsDialog)
				// Put up the dialog in detail mode to show the one new insistent attention request.
				return attnDetailsDialog;
			else if (attnGlobalsP->savedUIState == attnListDialog)
				// Put up the dialog in list mode to show multiple insistent attention requests.
				return attnListDialog;

		default:
			return attnNoDialog;
	}
}


/***********************************************************************
 *
 * FUNCTION:    AttnHandleEvent
 *
 * DESCRIPTION: Handle virtual character events used to trigger the
 *					 attention manager to do things. This can be one of the
 *					 following events:
 *
 *			vchrAttnStateChanged:
 *					 Someone called AttnGetAttention, AttnUpdate, or AttnForgetIt.
 *					 The global state has been changed accordingly, but the
 *					 user interface needs to be updated. If no dialog is present,
 *					 one may need to be opened. If so, this routine will put up
 *					 the dialog and return only when it is closed. This routine
 *					 is called by SysHandleEvent when it processes a vchrAttnStateChanged
 *					 keyDownEvent. It should never be called directly by an
 *					 application.
 *
 *			vchrAttnUnsnooze:
 *					 The snooze timer has expired. Put up the dialog in the appropriate
 *					 mode and wait for it to close before returning. If the dialog
 *					 is already open, switch modes if necessary and return immediately.
 *
 *			vchrAttnIndicatorTapped:
 *					 The indicator was tapped, so open the list dialog.
 *
 *			vchrAttnAllowClose:
 *					 The security lockout dialog needs to open, so close the
 *					 attention dialog if it's open.
 *
 *			vchrAttnReopen:
 *					 The security lockout dialog just opened, so reopen the
 *					 attention dialog if it was open.
 *
 * PARAMETERS:  pointer to system event
 *
 * RETURNED:    whether event was handled
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	07/27/00	Initial Revision
 *
 ***********************************************************************/
Boolean AttnHandleEvent(SysEventType * eventP)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	UInt16 newCount, newInsistentCount;
	
	// Only handle certain key down events.
	if (eventP->eType != sysEventKeyDownEvent)
		return false;
	
	switch (eventP->data.keyDown.chr)
	{
		case vchrAttnStateChanged:
			// The event was successfully delivered.
			attnGlobalsP->stateChangeInTransit = false;
			
			// If the dialog is open, let it handle the event. If the dialog is in the wrong mode,
			// it'll react by switching to the other mode.
			if (attnGlobalsP->currentUIState != attnNoDialog)
				return false;
			
			// There are three kinds of state changes that can be made, and they can be combined.
			// They include adding new attentions, updating existing ones, and removing existing
			// ones. We need to use this information to decide whether to open the dialog, and if
			// so, in what mode, and if in detail mode, which attention to show. Removing attention
			// requests doesn't cause the dialog to open, but updating an attention request might,
			// if it were allowed to change from being subtle to being insistent. By disallowing
			// this, we make our lives easier.
			
			// If new insistent attention requests were added, show them. Don't open the dialog
			// to show subtle attention requests; they only show up if you happen to already be
			// showing the dialog in list mode.
			newCount = PrvAttnGetCounts(&newInsistentCount, NULL, true);
			if (newInsistentCount == 1)
				// Put up the dialog in detail mode to show the one new insistent attention request.
				PrvAttnOpen(attnDetailsDialog, PrvAttnFindNewInsistent());
			else if (newInsistentCount > 1)
				// Put up the dialog in list mode to show multiple insistent attention requests.
				PrvAttnOpen(attnListDialog, noAttention);
			else
			{
				// Perform the special effects for new attentions.
				PrvAttnGetUsersAttentions();

				// We don't need to open the dialog. We don't want to forget about any new
				// subtle attentions, since they may cause the indicator to blink differently
				// after the detail dialog caused by an insistent attention is closed.
			}
			return true;

		case vchrAttnUnsnooze:
			// The event was successfully delivered.
			attnGlobalsP->unsnoozeInTransit = false;
			
			PrvAttnUnsnooze();
			return true;

		case vchrAttnIndicatorTapped:
			AttnListOpen();
			return true;
		
		case vchrAttnAllowClose:
			// If the dialog were up, it'd handle this event, so the dialog must
			// be closed. Remember this fact so we don't open it after the security
			// dialog is brought up.
			attnGlobalsP->savedUIState = attnNoDialog;
			return true;

		case vchrAttnReopen:
			// The dialog should never be open at this point.
			ErrNonFatalDisplayIf(attnGlobalsP->currentUIState != attnNoDialog, "Already open");
			
			// Re-open the dialog in the same mode it was in. No other
			// attention manager calls should have been made in the interim.
			if (attnGlobalsP->savedUIState == attnDetailsDialog)
				// Put up the dialog in detail mode to show the one new insistent attention request.
				PrvAttnOpen(attnDetailsDialog, attnGlobalsP->detailAttentionIndex);
			else if (attnGlobalsP->savedUIState == attnListDialog)
				// Put up the dialog in list mode to show multiple insistent attention requests.
				PrvAttnOpen(attnListDialog, noAttention);
			return true;

		default:
			// Do not handle any other key down events.
			return false;
	}
}

/***********************************************************************
 *
 * FUNCTION:    AttnEnableNotification
 *
 * DESCRIPTION: Enables/disables Attention Manager's notifications.
 *					 Must be called from UI thread.
 *
 * PARAMETERS:  enable	- if true, notifications are enabled; otherwise,
 *								  notifications are disabled;
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	10/25/00	Initial Revision
 *
 ***********************************************************************/
void AttnEnableNotification(Boolean enable)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;

	// We should have been initialized by now
	ErrFatalDisplayIf(!attnGlobalsP, "not initialized");
	
	if (enable)
	{
		if (attnGlobalsP->disableCount)
		{
			attnGlobalsP->disableCount--;
			
			// If the notification is now enabled, ensure that any attentions
			// which were triggered while notification was disabled will
			// be processed.
			if (attnGlobalsP->disableCount == 0)
			{
				AttnIndicatorEnable(true);
				PrvAttnStateChanged();
			}
		}
		else
			ErrDisplay("disable underflow");
	}
	else
	{
		if (attnGlobalsP->disableCount == 0)
			AttnIndicatorEnable(false);
				//DOLATER - peter: change this to be another condition for
				// the attention indicator being allowed to show and blink
				// when integrating the attention indicator into this file.
		attnGlobalsP->disableCount++;
		ErrFatalDisplayIf(!attnGlobalsP->disableCount, "disable overflow");
	}
}


/***********************************************************************
 *
 * FUNCTION:    AttnIndicatorTicksTillNextBlink
 *
 * DESCRIPTION: This routine returns the number of ticks until the next
 *					 time the indicator will blink.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    number of ticks until the next blink, or -1 if never
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	10/12/00	Initial Revision
 *
 ***********************************************************************/
Int32 AttnIndicatorTicksTillNextBlink(void)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	Int32 nextBlink, result;
	
	// We should have been initialized by now, but in order to allow
	// ErrDisplayFileLineMsg to call SysEventGet which calls this routine,
	// it's best to just do nothing if not yet initialized.
	if (!attnGlobalsP)
		return -1;
	
	if (!attnGlobalsP->indicatorIsAllowed || !attnGlobalsP->indicatorIsEnabled ||
		attnGlobalsP->indicatorBlinkPattern == kAttnIndicatorBlinkPatternNone)
	{
		return -1;
	}
	
	nextBlink = attnGlobalsP->indicatorLastTick + PrvAttnIndicatorBlinkInterval();
	result = nextBlink - TimGetTicks();
	if (result < 0)
		result = 1;
	return result;
}


/***********************************************************************
 *
 * FUNCTION:    AttnIndicatorCheckBlink
 *
 * DESCRIPTION: This routine blinks the attention indicator if it needs to
 *              be blinked.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	10/12/00	Initial Revision
 *
 ***********************************************************************/
void AttnIndicatorCheckBlink(void)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	UInt32 tick;
	
	// We should have been initialized by now, but in order to allow
	// ErrDisplayFileLineMsg to call SysEventGet which calls this routine,
	// it's best to just do nothing if not yet initialized.
	if (!attnGlobalsP)
		return;
	
	// We're not allowed to change the state of the indicator, so leave it alone.
	if (!attnGlobalsP->indicatorIsAllowed || !attnGlobalsP->indicatorIsEnabled ||
		attnGlobalsP->indicatorBlinkPattern == kAttnIndicatorBlinkPatternNone)
	{
		return;
	}
	
	// Is it time yet?
	tick = TimGetTicks();
	if (((UInt32) (tick - attnGlobalsP->indicatorLastTick)) < PrvAttnIndicatorBlinkInterval())
		return;
	
	// Go to the next frame in the indicator animation.
	switch (attnGlobalsP->indicatorFrame)
	{
		case kAttnIndicatorFrameOff:
			PrvAttnIndicatorSwitchToFrame(kAttnIndicatorFrameOn);
			break;
		case kAttnIndicatorFrameOn:
			PrvAttnIndicatorSwitchToFrame
			  (attnGlobalsP->indicatorBlinkPattern == kAttnIndicatorBlinkPatternNew
				? kAttnIndicatorFrameNew : kAttnIndicatorFrameOff);
			break;
		case kAttnIndicatorFrameNew:
			PrvAttnIndicatorSwitchToFrame(kAttnIndicatorFrameOff);
			break;
	}
	
	// Start timing for the next blink. Time relative to when we actually
	// blinked rather than when we wanted to blink.
	attnGlobalsP->indicatorLastTick = TimGetTicks();
}


/***********************************************************************
 *
 * FUNCTION:    AttnIndicatorAllow
 *
 * DESCRIPTION: This function is used by the operating system to stop blinking
 *					 the attention indicator during times when it should not
 *					 be allowed to blink, such as when the menu bar is up, or
 *					 when a modal dialog is on top of the form. The indicator
 *					 will only blink when it is allowed to show AND it is
 *					 enabled AND it is being asked to blink by the attention
 *					 manager. Calling this routine with FALSE turns it off
 *					 and stops it from blinking.
 *
 * PARAMETERS:  allowIt  TRUE to allow, FALSE to disallow
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	06/21/00	Initial Revision
 *
 ***********************************************************************/
void AttnIndicatorAllow(Boolean allowIt)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;

	if (attnGlobalsP->indicatorIsAllowed == allowIt)
		return;
	
	// Turn off the indicator if it's on.
	if (attnGlobalsP->indicatorFrame != kAttnIndicatorFrameOff && !allowIt)
		PrvAttnIndicatorSwitchToFrame(kAttnIndicatorFrameOff);
	
	attnGlobalsP->indicatorIsAllowed = allowIt;
	
	// Start timing from now if allowed.
	if (attnGlobalsP->indicatorIsAllowed && attnGlobalsP->indicatorIsEnabled &&
		attnGlobalsP->indicatorBlinkPattern != kAttnIndicatorBlinkPatternNone)
	{
		attnGlobalsP->indicatorLastTick = TimGetTicks();
	}
}


/***********************************************************************
 *
 * FUNCTION:    AttnIndicatorAllowed
 *
 * DESCRIPTION: This function is used by the operating system to determine
 *					 whether the attention indicator is allowed to blink.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    whether it can blink
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	06/21/00	Initial Revision
 *
 ***********************************************************************/
Boolean AttnIndicatorAllowed(void)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;

	return attnGlobalsP->indicatorIsAllowed;
}


/***********************************************************************
 *
 * FUNCTION:    AttnIndicatorSetBlinkPattern
 *
 * DESCRIPTION: This function is used by the attention manager to enable
 *					 or disable the attention indicator. The indicator
 *					 will only blink when it is allowed to show AND it is
 *					 enabled by the attention manager. Calling this routine with
 *					 FALSE turns it off and stops it from blinking.
 *
 * PARAMETERS:  blink pattern (zero to disable, non-zero to enable)
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	06/12/00	Initial Revision
 *			CS		08/15/00	Initialize new event structure.
 *			peter	08/29/00	Switch from enable/disable to set pattern
 *
 ***********************************************************************/
void AttnIndicatorSetBlinkPattern(UInt16 blinkPattern)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;

	if (attnGlobalsP->indicatorBlinkPattern == blinkPattern)
		return;

	// Turn off the indicator if it's on.
	if (attnGlobalsP->indicatorFrame != kAttnIndicatorFrameOff &&
			blinkPattern == kAttnIndicatorBlinkPatternNone)
		PrvAttnIndicatorSwitchToFrame(kAttnIndicatorFrameOff);
	
	attnGlobalsP->indicatorBlinkPattern = blinkPattern;

	// Start timing from now if blinking pattern just set.
	if (attnGlobalsP->indicatorIsAllowed && attnGlobalsP->indicatorIsEnabled &&
		attnGlobalsP->indicatorBlinkPattern != kAttnIndicatorBlinkPatternNone)
	{
		attnGlobalsP->indicatorLastTick = TimGetTicks();
	}
}


/***********************************************************************
 *
 * FUNCTION:    AttnIndicatorGetBlinkPattern
 *
 * DESCRIPTION: This routine returns the current blink pattern of the 
 *					 attention indicator, as set by the attention manager.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:   blink pattern (zero for disabled, non-zero for enabled)
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	06/12/00	Initial Revision
 *			peter	08/29/00	Switch from is enabled to get pattern
 *
 ***********************************************************************/
UInt16 AttnIndicatorGetBlinkPattern(void)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;

	return attnGlobalsP->indicatorBlinkPattern;
}

#pragma mark -		// Public API

/***********************************************************************
 *
 * FUNCTION:    AttnGetAttention
 *
 * DESCRIPTION: Get the user's attention for something. If an attention
 *					 manager dialog is already up, just add the new request to
 *					 the list, switching to the list if necessary. If no
 *					 attention mananger dialog is up, and the new request is
 *					 insistent, arrange for the proper dialog to come up.
 *					 This routine always returns immediately. This routine
 *					 may only be called by the UI thread. That means using
 *					 deferred notification or using the alarm manager's
 *					 sysAppLaunchCmdAlarmTriggered launch code to trigger calls
 *					 to this routine. Note that the sysAppLaunchCmdDisplayAlarm
 *					 launch code should not be used for calling this routine
 *					 because it sets the UI busy when in fact it is not.
 *					 If you pass an invalid soundUniqueID, it will play
 *					 the standard system alarm sound, but at the user's setting
 *					 for alarm volume, not the volume specified.
 *					 Calling this routine will always turn the screen on if it
 *					 is not already on, and reset the auto-off timer.
 *					 In order to force sound to play even when the user preference
 *					 is not to, set the AttnFlags_AlwaysSound bit in the flags AND
 *					 pass a non-zero volume you want to play the sound at.
 *					 In order to force no sound even when the user preference is
 *					 to play sound, set the AttnFlags_NoSound bit in the flags and
 *					 pass anything for volume.
 *
 * PARAMETERS:  cardNo		- the card on which the specified db resided
 *					 dbID			- identifies application making the request
 *					 userData	- arbitrary data application uses to distinguish these
 *					 callbackFnP- pointer to callback function, or NULL to use launch codes
 *					 level		- subtle or insistent (only the latter puts up a dialog)
 *					 flags		- how to get user's attention
 *					 soundUniqueID - what sound to play
 *					 nagRateInSeconds - time between nag attempts
 *					 nagRepeatLimit	- number of times to nag, excluding first attempt
 *
 * RETURNS:		 Error code: - errNone if no problem
 *									 - attnErrMemory if no more room for attention requests
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	06/12/00	Initial Revision
 *
 ***********************************************************************/
Err AttnGetAttention(UInt16 cardNo, LocalID dbID, UInt32 userData, AttnCallbackProc *callbackFnP,
	AttnLevelType level, AttnFlagsType flags,
	UInt16 nagRateInSeconds, UInt16 nagRepeatLimit)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	AttentionType attention;
	
	// We should have been initialized by now
	ErrFatalDisplayIf(!attnGlobalsP, "not initialized");

	// Verify that the new attention does not match the dbID and userData of
	// any existing ones. Skip ones marked for deletion.
	ErrNonFatalDisplayIf(PrvAttnFindAttention(cardNo, dbID, userData) != noAttention,
		"Attempt to add duplicate attention");

	// Make room for one more.
	if (PrvAttnGrowTable() != errNone)
		return attnErrMemory;
	
	// Before doing anything else, turn on the screen if it isn't already
	// on, and make sure it stays on for a while. Do this even for subtle
	// attentions which don't get the user's attention in any way other than
	// the on-screen attention indicator, so that we have a simple consistent
	// policy.
	EvtResetAutoOffTimer();

	// Add the new attention on the end. This order is used so that the attentions being
	// shown in the dialog don't move when the API updates the global state.
	attention.cardNo = cardNo;
	attention.dbID = dbID;
	attention.userData = userData;
	attention.callbackFnP = callbackFnP;
	attention.level = level;
	attention.flags = flags;
	attention.nagRateInSeconds = nagRateInSeconds;
	attention.nagRepeatLimit = nagRepeatLimit;
	attention.lastNagTime = TimGetSeconds();
	attention.nagsRemaining = nagRepeatLimit;
	attention.isNew = true;
	attention.isUpdated = false;
	attention.isDeleted = false;
	attention.isSnoozed = false;
	attention.isFirstTime = true;
	attention.getUsersAttention = true;
	attention.isDemoted = false;
	attnGlobalsP->attentions[attnGlobalsP->numberOfAttentions] = attention;
	attnGlobalsP->numberOfAttentions++;
	
	// Don't play sound yet. Wait till the dialog opens, if it will open, before playing sound.
	
	// Don't start the LED blinking yet. Wait till the dialog opens, if it will open, before
	// starting the LED blinking.
	
	// For insistent attentions, we try to open the dialog, but we may not succeed if the
	// currently running event loop doesn't allow the vchrAttnStateChanged event to get through to
	// SysHandleEvent(). We use special checks to see if the event is discarded, and if so, play
	// the sound. That way, at least the sound and other special effects are always played.
	// While we could play the special effects for subtle attentions right away, it's better to
	// wait until the dialog is updated. That way, adding a bunch of new subtle attentions all at
	// once causes only one performance of each attention getting mechanism.
	
	// Turn the indicator on or off. For insistent attention requests, we turn the
	// indicator off, so it's off when the dialog saves the area under itself. That way, the
	// indicator doesn't appear briefly after the dialog is closed. If, however, the dialog
	// fails to open, must then turn the indicator on. That way, at least there will be some
	// indication that something happened. For subtle attention requests, we always turn the
	// indicator on because we don't open the dialog.
	switch (level)
	{
		case kAttnLevelInsistent:
			AttnIndicatorSetBlinkPattern(kAttnIndicatorBlinkPatternNone);
			break;
		case kAttnLevelSubtle:
			AttnIndicatorSetBlinkPattern(kAttnIndicatorBlinkPatternNew);
			break;
		default:
			ErrNonFatalDisplay("Invalid level");
	}
	
	// Add a vchrAttnStateChanged event to get the user interface to update or open as necessary.
	PrvAttnStateChanged();
	
	// Start or adjust the nag timer as necessary. If this new attention will nag before any
	// others, we need to reset the alarm manager timer.
	PrvAttnSetNagTimer();
	
	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:    AttnUpdate
 *
 * DESCRIPTION: Update an existing request to get the user's attention.
 *					 This routine may only be called by the UI thread. That means using
 *					 deferred notification or waiting for the display alarm
 *					 launch code to call this routine. Note that the level cannot
 *					 be changed because allowing it to change would complicate
 *					 implementation significantly. This routine will not move the
 *					 attention to the top of the list or cause any special effects
 *					 such as sounds to occur. If you want to do this, remove the
 *					 request with AttnForgetIt and re-add it with AttnGetAttention.
 *					 Calling this routine will not turn the screen on if it is off.
 *					 Instead, the update will be delayed until the screen is next
 *					 turned on.
 *
 * PARAMETERS:  cardNo		- the card on which the specified db resided
 *					 dbID			- identifies application making the request
 *					 userData	- arbitrary data application uses to distinguish these
 *					 callbackFnP- pointer to callback function, or NULL to use launch codes
 *					 flagsP		- pointer to how to get user's attention, or NULL to leave unchanged
 *					 soundUniqueIDP - pointer to what sound to play, or NULL to leave unchanged
 *					 vibrateDurationInSecondsP - pointer to how long to vibrate, or NULL to leave unchanged
 *					 nagRateInSecondsP - pointer to time between nag attempts, or NULL to leave unchanged
 *					 nagRepeatLimitP	- pointer to number of times to nag, excluding first attempt, or NULL to leave unchanged
 *
 * RETURNS:		 whether successful
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	06/12/00	Initial Revision
 *			peter	08/01/00	Change API to allow most arguments to be NULL to leave unchanged
 *
 ***********************************************************************/
Boolean AttnUpdate(UInt16 cardNo, LocalID dbID, UInt32 userData, AttnCallbackProc *callbackFnP,
	AttnFlagsType *flagsP, UInt16 *nagRateInSecondsP, UInt16 *nagRepeatLimitP)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	Int16 index;

	// We should have been initialized by now
	ErrFatalDisplayIf(!attnGlobalsP, "not initialized");
	
	// Scan the attention requests looking for one with matching dbID and userData.
	index = PrvAttnFindAttention(cardNo, dbID, userData);
	if (index == noAttention)
		return false;
	
	// Update this attention request in the list.
	attnGlobalsP->attentions[index].callbackFnP = callbackFnP;
	if (nagRateInSecondsP)
		attnGlobalsP->attentions[index].nagRateInSeconds = *nagRateInSecondsP;
	if (nagRepeatLimitP)
		attnGlobalsP->attentions[index].nagRepeatLimit = *nagRepeatLimitP;
	if (flagsP)
		attnGlobalsP->attentions[index].flags = *flagsP;
	attnGlobalsP->attentions[index].isUpdated = true;
	
	// Add a vchrAttnStateChanged event to get the user interface to update or open as necessary.
	PrvAttnStateChanged();

	// Start or adjust the nag timer as necessary. If the nag policy of this attention was
	// changed, it may now nag before any others, where before the update it did not, or
	// vice versa, so we may need to reset the alarm manager timer.
	PrvAttnSetNagTimer();

	return true;
}


/***********************************************************************
 *
 * FUNCTION:    AttnForgetIt
 *
 * DESCRIPTION: Stop trying to get the user's attention for something.
 *					 Other requests are not affected.
 *					 This routine may only be called by the UI thread. That means using
 *					 deferred notification or waiting for the display alarm
 *					 launch code to call this routine. 
 *
 * PARAMETERS:  cardNo		- the card on which the specified db resided
 *					 dbID			- identifies application making the request
 *					 userData	- arbitrary data application uses to distinguish these
 *
 * RETURNS:		 whether successful
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	06/12/00	Initial Revision
 *
 ***********************************************************************/
Boolean AttnForgetIt(UInt16 cardNo, LocalID dbID, UInt32 userData)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	Int16 index;
	AttnCommandArgsType args;

	// We should have been initialized by now
	ErrFatalDisplayIf(!attnGlobalsP, "not initialized");
	
	// Scan the attention requests looking for one with matching dbID and userData.
	index = PrvAttnFindAttention(cardNo, dbID, userData);
	if (index == noAttention)
		return false;

	// Before doing anything else, turn on the screen if it isn't already
	// on, and make sure it stays on for a while. It'd be better to delay
	// the update of the UI if necessary to avoid turning the screen on,
	// but this is simpler.
	EvtResetAutoOffTimer();

	// Let the UI know that the item was removed.
	attnGlobalsP->attentions[index].isDeleted = true;

	// Notify the application that the attention request is being removed.
	args.gotIt.dismissedByUser = false;
	PrvAttnNotify(index, kAttnCommandGotIt, &args);

	// Add a vchrAttnStateChanged event to get the user interface to update as necessary.
	PrvAttnStateChanged();
	
	// Start or adjust the nag timer as necessary. If the removed attention was the next
	// to nag, we need to reset the alarm manager timer.
	PrvAttnSetNagTimer();
	
	// Turn the indicator off, or switch from new to old blink pattern if necessary.
	PrvAttnUpdateIndicator();
	
	return true;
}


/***********************************************************************
 *
 * FUNCTION:    AttnGetCounts
 *
 * DESCRIPTION: Find out how many things are trying to get the user's
 *					 attention. This routine may only be called by the UI thread.
 *
 * PARAMETERS:  cardNo				- the card on which the specified db resided
 *					 dbID					- identifies application whose attentions should be counted
 *											  (if both cardNo & dbID are 0, values returned are the
 *												total number of subtle & insistents attentions from all
 *												clients posted to attention manager)
 *					 insistentCountP	- where to write number of insistent requests
 *					 subtleCountP		- where to write number of subtle requests
 *										   Either of the above may be NULL
 *
 * RETURNS:		 Total number of attention requests (subtle + insistent)
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	06/12/00	Initial Revision
 *			peter	08/31/00	Don't count attention requests marked for deletion
 *			gap	10/13/00	Added ability to return total for specified client
 *								designated by cardNo/dbID as well as total counts
 *
 ***********************************************************************/
UInt16 AttnGetCounts(UInt16 cardNo, LocalID dbID, UInt16 *insistentCountP, UInt16 *subtleCountP)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	UInt16 insistentCount = 0;
	UInt16 subtleCount = 0;
	UInt16 index;
	Boolean countAllAttentions;

	// We should have been initialized by now
	ErrFatalDisplayIf(!attnGlobalsP, "not initialized");
	
	// Determine if the user wants totals for all posted attentions
	// or totals of attentions posted from one particular client.
	// cardNo = 0 and dbID = 0 specify all attentions should be counted.
	countAllAttentions = ((dbID == 0) && (cardNo == 0));
	
	//DOLATER - Replacing numberOfAttentions count with two counts, or keeping it and
	//				add two new counts for insistent and subtle would speed up this routine.
	//				These counts would exclude those marked for deletion and include those
	//				marked as new or updated, so no iteration would be required here.
	
	for (index = 0; index < attnGlobalsP->numberOfAttentions; index++)
		if ( (!attnGlobalsP->attentions[index].isDeleted) &&
			  ( (countAllAttentions) || ((dbID == attnGlobalsP->attentions[index].dbID) && (cardNo == attnGlobalsP->attentions[index].cardNo)))  )
			switch (attnGlobalsP->attentions[index].level)
			{
				case kAttnLevelInsistent:
					insistentCount++;
					break;
				case kAttnLevelSubtle:
					subtleCount++;
					break;
				default:
					ErrNonFatalDisplay("Invalid level");
			}
	
	if (insistentCountP != NULL)
		*insistentCountP = insistentCount;
	if (subtleCountP != NULL)
		*subtleCountP = subtleCount;
	
	return insistentCount + subtleCount;
}


/***********************************************************************
 *
 * FUNCTION:    AttnListOpen
 *
 * DESCRIPTION: Put up the dialog and wait for the user to dismiss it,
 *					 then act accordingly based on how it was dismissed. If
 *					 dialog is already open, do nothing, even if in detail
 *					 rather than list mode.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    none
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	07/17/00	Initial Revision
 *			peter	11/06/00	If dialog is already open, do nothing.
 *
 ***********************************************************************/
void AttnListOpen(void)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;

	// We should have been initialized by now
	ErrFatalDisplayIf(!attnGlobalsP, "not initialized");
	
	if (attnGlobalsP->currentUIState == attnNoDialog)
		PrvAttnOpen(attnListDialog, noAttention);
}	


/***********************************************************************
 *
 * FUNCTION:    AttnIterate
 *
 * DESCRIPTION: Iterate through all the attention requests made by this
 *					 application. For each, use the callback or launch code
 *					 to inform the application about the attention request.
 *					 This routine is typically used when an application is
 *					 notified that a HotSync has occurred, so it can remove
 *					 attention requests for records removed during the HotSync.
 *					 It is legal to call AttnForgetIt inside the iteration
 *					 because this only marks the record for deletion, so the
 *					 iteration is not confused by it.
 *
 * PARAMETERS:  cardNo			- the card on which the specified db resided
 *					 dbID				- identifies application making the request
 *					 iterationData	- data app may need in order to process the 
 *										  iterate callback/launchcode
 *
 * RETURNS:		 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	08/11/00	Initial Revision
 *
 ***********************************************************************/
void AttnIterate(UInt16 cardNo, LocalID dbID, UInt32 iterationData)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;
	UInt16 index;
	AttnCommandArgsType args;
	
	// We should have been initialized by now
	ErrFatalDisplayIf(!attnGlobalsP, "not initialized");
	
	for (index = 0; index < attnGlobalsP->numberOfAttentions; index++)
	{
		if (attnGlobalsP->attentions[index].cardNo == cardNo &&
			 attnGlobalsP->attentions[index].dbID == dbID &&
			 !attnGlobalsP->attentions[index].isDeleted)
		{
			// Notify the application that the attention request is being removed.
			args.iterate.iterationData = iterationData;
			PrvAttnNotify(index, kAttnCommandIterate, &args);
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:    AttnDoSpecialEffects
 *
 * DESCRIPTION: Blink the LED, vibrate, etc to got the user's attention.
 *					 This routine provides a convenience for other applications
 *					 which need to use special effects. It does the equivalent
 *					 of one nag of an attention manager special effect set as
 *					 if there were no other pending attentions.
 *
 *					 Note that this routine never performs custom special
 *					 effects or plays sounds. The caller is responsible for this.
 *
 * PARAMETERS:  flags			- how to get user's attention
 *
 * RETURNED:    error code
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	10/17/00	Initial Revision
 *			peter	11/11/00	Don't play sound
 *
 ***********************************************************************/
Err AttnDoSpecialEffects(AttnFlagsType flags)
{
#if	EMULATION_LEVEL == EMULATION_NONE
	if (flags & kAttnFlagsVibrateBit)
		PrvAttnVibrate();
	if (flags & kAttnFlagsLEDBit)
		PrvAttnLED(attnLEDInitial);
#endif

	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:    AttnIndicatorEnable
 *
 * DESCRIPTION: This function is used by applications to enable
 *					 or disable the attention indicator. The indicator
 *					 will only blink when it is allowed to show AND it is
 *					 enabled AND it is being asked to blink by the attention
 *					 manager. Calling this routine with FALSE turns it off
 *					 and stops it from blinking.
 *
 * PARAMETERS:  whether to enable or disable
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	06/12/00	Initial Revision
 *			CS		08/15/00	Initialize new event structure.
 *
 ***********************************************************************/
void AttnIndicatorEnable(Boolean enableIt)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;

	// We should have been initialized by now
	ErrFatalDisplayIf(!attnGlobalsP, "not initialized");
	
	if (attnGlobalsP->indicatorIsEnabled == enableIt)
		return;
	
	// Turn off the indicator if it's on.
	if (attnGlobalsP->indicatorFrame != kAttnIndicatorFrameOff && !enableIt)
		PrvAttnIndicatorSwitchToFrame(kAttnIndicatorFrameOff);
	
	attnGlobalsP->indicatorIsEnabled = enableIt;
	
	// Start timing from now if enabled.
	if (attnGlobalsP->indicatorIsAllowed && attnGlobalsP->indicatorIsEnabled &&
		attnGlobalsP->indicatorBlinkPattern != kAttnIndicatorBlinkPatternNone)
	{
		attnGlobalsP->indicatorLastTick = TimGetTicks();
	}
}


/***********************************************************************
 *
 * FUNCTION:    AttnIndicatorEnabled
 *
 * DESCRIPTION: This routine returns whether the attention indicator is
 *					 currently enabled. Applications can disable the indicator
 *					 if they don't want it to blink.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:	 whether enabled
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	06/12/00	Initial Revision
 *
 ***********************************************************************/
Boolean AttnIndicatorEnabled(void)
{
	AttnGlobalsType	* attnGlobalsP = (AttnGlobalsType *)GAttnGlobalsP;

	// We should have been initialized by now
	ErrFatalDisplayIf(!attnGlobalsP, "not initialized");
	
	return attnGlobalsP->indicatorIsEnabled;
}
