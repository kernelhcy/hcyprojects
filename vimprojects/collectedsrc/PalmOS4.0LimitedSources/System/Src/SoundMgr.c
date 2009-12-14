/******************************************************************************
 *
 * Copyright (c) 1995-1999 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: SoundMgr.c
 *
 * Release: 
 *
 * Description:
 *	  This file contains the Sound Manager routines.
 *
 * History:
 *		Apr 11, 1995	Created by Vitaly Kruglikov
 *
 *****************************************************************************/

#define NON_PORTABLE

#include <PalmTypes.h>

// public system includes
#include <ErrorMgr.h>
#include <SoundMgr.h>
#include <StringMgr.h>
#include <SysUtils.h>

#include <PalmUtils.h>

// private system includes
#include "SoundPrv.h"
#include "Globals.h"
#include "HwrSound.h"

#include "AttentionMgr.h"
#include "Hardware.h"



//-------------------------------------------------------------------
//		Private constants used only by this module
//-------------------------------------------------------------------

//-------------------------------------------------------------------
//		Private routines used only by this module
//-------------------------------------------------------------------
static void	PrvDoFreqDurationAmp(SndCommandPtr cmdP);
static void	PrvPlayAlarmSound(void);
static void PrvSoundKillTimerProc(Int32 timerID, Int32 param);


//-------------------------------------------------------------------
//		Sound Manager Initialization routines
//-------------------------------------------------------------------



/*********************************************************************************
 *  FUNCTION: SndInit
 *
 *  DESCRIPTION: Initializes the sound manager.
 *
 *		Allocate the sound globals and the shared channel in the dynamic
 *		heap.
 *
 *  PARAMETERS: void
 *
 *  RETURNS: 0 if successful; otherwise: sndErrMemory
 *
 *  CREATED: 4/12/95
 *
 *  BY: Vitaly Kruglikov
 *
 *		vmk	9/3/97	Added a call to ISndMidiInit
 *
 *********************************************************************************/
Err SndInit(void)
{
	Err				err = 0;
	SndGlobalsPtr	gP = 0;

// timer used only under emulation
#if EMULATION_LEVEL != EMULATION_NONE
	UInt32				timerTag = 'sndt';				
#endif

#if EMULATION_LEVEL == EMULATION_NONE
	UInt32				smID = 0;
	UInt32				tag = 'SndM';
#endif // EMULATION_LEVEL == EMULATION_NONE


	// We shouldn't be called more than once without being freed in between
	ErrFatalDisplayIf( GSndGlobalsP, "already initialized" );
	
	// Allocate and initialize the Sound Manager globals
	gP = (SndGlobalsPtr)MemPtrNew( sizeof(SndGlobalsType) );
	if ( !gP ) goto ErrorMem;
	MemSet( gP, sizeof(SndGlobalsType), 0 );	// initialize

	gP->defAmp = sndDefaultAmp;					// initialize default amplitude

#if EMULATION_LEVEL == EMULATION_NONE
	// Allocate the Sound Manager semaphore
	// We use a semaphore to protect data integrity during task switching
	// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
	if ( err ) goto ErrorMem;
	gP->smID = smID;
#endif // EMULATION_LEVEL == EMULATION_NONE

	// Load the sound preferences
	gP->sysAmp = (UInt8) PrefGetPreference(prefSysSoundVolume);
	gP->alarmAmp = (UInt8) PrefGetPreference(prefAlarmSoundVolume);
	
// timer used only under emulation
#if EMULATION_LEVEL != EMULATION_NONE
	// Create the "sound kill" timer
	// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
	// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
	// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
	ErrFatalDisplayIf(err, "unable to create sound-kill timer");
#endif
	
	// Initialize the MIDI engine
	ISndMidiInit(gP);
						
	// Link system globals to sound globals
	GSndGlobalsP = (MemPtr)gP;
	
	// Make sure sound is off at reset
	HwrSoundOff();
	
	return( err );

ErrorMem:
	// Free anything allocated here
	if ( gP )
		MemPtrFree( gP );
	
#if EMULATION_LEVEL == EMULATION_NONE
	if ( smID )
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
#endif // EMULATION_LEVEL == EMULATION_NONE
		
	return( sndErrMemory );
}



//-------------------------------------------------------------------
//		Sound Manager API routines
//-------------------------------------------------------------------



/***********************************************************************
 *
 * FUNCTION:    SndCompareMidiNames
 *
 * DESCRIPTION: Compare one midi record name to another (for sorting purposes).
 *
 * PARAMETERS:	 SndMidiListItemType *a, *b - the items to compare
 *					 other - not used in the comparison
 *
 * RETURNED:	 if a < b return -1
 *					 if a==b return 0
 *					 if a > b return -1
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/5/96	Initial Revision
 *
 ***********************************************************************/
static Int16 SndCompareMidiNames (void *a, void *b, Int32 /*other*/)
{
	return StrCompare(((SndMidiListItemType *)a)->name, ((SndMidiListItemType *)b)->name);
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 *  FUNCTION: SndCreateMidiList
 *
 *  DESCRIPTION: Generate a list of midi samples found in midi databases.
 *
 *  PARAMETERS:	creator		-- creator of the MIDI database(s); pass zero for wildcard
 *						multipleDBs	-- pass true to look for multiples midi databases
 *						wCountP		-- pointer to contain count of matching databases
 *						entHP			-- pointer to MemHandle allocated to contain the MIDI list
 *
 *  RETURNS:	FALSE when no entries are returned;
 *					*wCountP is set to the number of matches
 *					*entHP is set to the list of matches
 *
 *  CREATED: 8/1/97
 *
 *  BY: Roger Flores
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	???		Initial version
 *			vmk	???		Debugged
 *			vmk	10/21/97	Added creator and multipleDBs args
 *			vmk	12/15/97	Fixed to not realloc to zero size
 *
 *************************************************************/
extern Boolean SndCreateMidiList(UInt32 creator, Boolean multipleDBs, UInt16 * wCountP,
		MemHandle *entHP)
{
	Int16						midiIndex;		// UInt16 results in a bug
	
	UInt16 						cardNo;
	LocalID					dbID;
	DmOpenRef 				dbP;
	
	DmSearchStateType		searchState;
	Boolean					newSearch = true;
	UInt16						maxItems = 16;
	
	SndMidiListItemType	*midiListP;
	SndMidiListItemType	*midiItemP;
	Err						err=0;
	UInt16						index;
	MemHandle					recordH;
	SndMidiRecType*		recP;
	UInt16						recordsInDatabase;
	
	
	
	// Allocate a minimum size SndMidiListItemType array to hold
	//  list of midi records.
	*entHP = MemHandleNew(maxItems * sizeof(SndMidiListItemType));
	if (!*entHP)
		return false;
	midiListP = MemHandleLock(*entHP);

	
	// Create a list of matching databases.  We do not care about the order
	// or multiple versions yet.  The assumption is that there won't be many 
	// multiple versions.
	// ½(n)
	midiIndex = 0;
	while (true) 
		{
		// Seach for the next MIDI database
		err = DmGetNextDatabaseByTypeCreator(newSearch, &searchState, 
					sysFileTMidi, creator, false, &cardNo, &dbID);
		if (err)
			break;
		newSearch = false;


		dbP = DmOpenDatabase(cardNo, dbID, dmModeReadOnly);
		recordsInDatabase = DmNumRecords(dbP);
		for (index = 0; index < recordsInDatabase; index++)
			{
			if ((recordH = DmQueryRecord(dbP, index)) != 0)
				{
				// See if we need to grow our SndMidiListItemType array
				if (midiIndex >= maxItems)
					{
					MemPtrUnlock(midiListP);
					midiListP = 0;
					maxItems += 4;
					err = MemHandleResize(*entHP, maxItems * sizeof(SndMidiListItemType));
					midiListP = MemHandleLock(*entHP);
					if (err) 
						{
						maxItems -= 4;
						break;
						}
					}
				
				
								
				// Save the info on this db
				midiItemP = &midiListP[midiIndex];
				recP = (SndMidiRecType*) MemHandleLock(recordH);
				ErrNonFatalDisplayIf(recP->hdr.signature != sndMidiRecSignature,
					"invalid MIDI record");
				if ( recP->hdr.signature == sndMidiRecSignature )
					{
					StrNCopy(midiItemP->name, recP->name, sndMidiNameLength);
					midiItemP->dbID = dbID;
					midiItemP->cardNo = cardNo;
					
					// Get the unique record ID of the MIDI record
					DmRecordInfo(dbP, index, NULL, &midiItemP->uniqueRecID, NULL);
									
					midiIndex++;
					}
				MemPtrUnlock(recP);
				}
			
			}
		DmCloseDatabase(dbP);
		
		if ( !multipleDBs )
			break;
		}
		

	// Sort the midi records by name.  ½(n log n)
	SysQSort (midiListP, midiIndex, sizeof(SndMidiListItemType), SndCompareMidiNames, 0);
	
	
	// We might want to shrink the database list to conserve memory.
	// Extra may have been allocated.
	// ½ (1)
	if (midiIndex < maxItems && midiIndex > 0) 
		{
		MemPtrUnlock(midiListP);
		midiListP = 0;
		err = MemHandleResize(*entHP, midiIndex * sizeof(SndMidiListItemType));
		midiListP = MemHandleLock(*entHP);
		}
		
	
	
	// Record how many matches were found
	*wCountP = midiIndex;
	
Exit:
	if (midiListP)
		MemPtrUnlock(midiListP);
	
	// If nothing was found, remove the memory allocated.	
	if (*wCountP == 0)
		{
		MemHandleFree(*entHP);
		*entHP = 0;
		return false;
		}
		
	return true;
}



/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION: SndSetDefaultVolume
 *
 * DESCRIPTION: Sets the default sound volume levels
 *
 *					Any pointer arguments may be passed as NULL
 *
 * PARAMETERS:	alarmAmpP	-- MemPtr to alarm amplitude(0-sndMaxAmp)
 *					sysAmpP		-- MemPtr to system sound amplitude(0-sndMaxAmp)
 *					defAmpP		-- MemPtr to default amplitude for other sounds(0-sndMaxAmp)
 *
 * RETURNS: void
 *
 * CREATED: 4/14/95
 *
 * BY: Vitaly Kruglikov
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	9/21/95	Added parametes for alarm and system sounds
 *************************************************************/
void SndSetDefaultVolume(UInt16 * alarmAmpP, UInt16 * sysAmpP, UInt16 * defAmpP)
{
	SndGlobalsPtr		gP = (SndGlobalsPtr)GSndGlobalsP;
	
	
	// By this time, we should have been initialized
	ErrFatalDisplayIf( !gP, "not initialized" );

	if ( alarmAmpP )
		{
		ErrNonFatalDisplayIf( *alarmAmpP > sndMaxAmp, "bad alarm amp param" );
		gP->alarmAmp = (UInt8) min(*alarmAmpP, sndMaxAmp);
		}

	if ( sysAmpP )
		{
		ErrNonFatalDisplayIf( *sysAmpP > sndMaxAmp, "bad sys amp param" );
		gP->sysAmp = (UInt8) min(*sysAmpP, sndMaxAmp);
		}

	if ( defAmpP )
		{
		ErrNonFatalDisplayIf( *defAmpP > sndMaxAmp, "bad other amp param" );
		gP->defAmp = min(*defAmpP, sndMaxAmp);
		}

}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 *  FUNCTION: SndGetDefaultVolume
 *
 *  DESCRIPTION: Returns default sound volume levels
 *
 *					Any pointer arguments may be passed as NULL
 *
 * PARAMETERS:	alarmAmpP	-- MemPtr to alarm amplitude(0-sndMaxAmp)
 *					sysAmpP		-- MemPtr to system sound amplitude(0-sndMaxAmp)
 *					defAmpP		-- MemPtr to default amplitude for other sounds(0-sndMaxAmp)
 *
 *  RETURNS: void
 *
 *  CREATED: 4/11/95
 *
 *  BY: Vitaly Kruglikov
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	9/21/95	Added parametes for alarm and system sounds
 *************************************************************/
void SndGetDefaultVolume(UInt16 * alarmAmpP, UInt16 * sysAmpP, UInt16 * defAmpP)
{
	SndGlobalsPtr	gP = (SndGlobalsPtr)GSndGlobalsP;
	
	
	// By this time, we should have been initialized
	ErrFatalDisplayIf( !gP, "not initialized" );

	if ( alarmAmpP )
		*alarmAmpP = gP->alarmAmp;

	if ( sysAmpP )
		*sysAmpP = gP->sysAmp;

	if ( defAmpP )
		*defAmpP = gP->defAmp;
}



/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION: SndDoCmd
 *
 * DESCRIPTION: Sends a Sound Manager command to a sound channel.
 *
 *				NOTE: Passing nil for the channel pointer causes the
 *				command to be sent to the shared sound channel.
 *
 *				Presently, we do not support multiple channels.
 *
 * PARAMETERS:
 *		chanP			-- sound channel pointer, or nil for the shared channel;
 *		cmdP			-- MemPtr to sound command structure;
 *		noWait			-- 0 = await completion, !0 = immediate return;
 *
 * RETURNS: 0 if successful; otherwise:	sndErrBadParam
 *											sndErrBadChannel
 *											sndErrQFull
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		----------- 
 *			vmk	8/28/95	Initial version
 *			tlw	2/4/98	If client asks for sound and dwTicks == 0, use 1
 *
 *************************************************************/
Err SndDoCmd(void * chanP, SndCommandPtr cmdP, Boolean /*noWait*/)
{
	Err				err = 0;
	SndGlobalsPtr	gP = (SndGlobalsPtr)GSndGlobalsP;
	UInt32				dwTicks;
	Boolean			soundOn;


	// By this time, we should have been initialized
	ErrFatalDisplayIf( !gP, "not initialized" );
	// We only support the shared channel for now
	ErrFatalDisplayIf( chanP, "bad channel MemPtr" );

	// Get exclusive access to channel
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
	
	// Process the command
	switch ( cmdP->cmd )
		{			
		case sndCmdFreqDurationAmp:
			PrvDoFreqDurationAmp( cmdP );
			break;

		case sndCmdNoteOn:
		case sndCmdFrqOn:
			// Compute max duration in system ticks
			dwTicks = sndMilliSecToPilotTicks(cmdP->param2);
	
			// If user wanted some sound then give them the minimum (vs no sound)
			if (cmdP->param2 && !dwTicks)
				dwTicks = 1;
			
			// Stop the sound-kill timer
			prvStopSoundOffCountdown(gP);
			
			// Start the sound
			if ( cmdP->cmd == sndCmdNoteOn )
				err = ISndMidiNoteOn(gP, (UInt8)cmdP->param1, (UInt8)cmdP->param3, sndMaxAmp, &soundOn);
			else
				soundOn = HwrSoundOn(cmdP->param1, cmdP->param3);
				
			// If max duration is non-zero, start the sound-kill timer
			if ( dwTicks && soundOn )
				{
				// Add one because the next tick could come immediately.
				// (We could do slightly better by either waiting for the tick before
				//  starting or taking the time remaining until the next tick into
				//  account when rounding milliseconds to ticks.)
				prvStartSoundOffCountdown(gP, dwTicks+1);
				}

			// otherwise, kill the sound immediately
			else
				{
				HwrSoundOff();
				}
				
			break;

		case sndCmdQuiet:
			HwrSoundOff();
			break;
		
		default:		// unhandled command
			ErrNonFatalDisplay("unknown sound cmd" );
			err = sndErrBadParam;
			break;
		}

	// Release exclusive access to channel
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.

	return( err );
}



/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION: SndPlaySystemSound
 *
 * DESCRIPTION: Plays a standard system sound
 *
 *	THOUGHTS: The system beeps should be played immediately,
 *			replacing the command in progress, but without flushing the
 *			shared channel's queue.
 *
 * PARAMETERS: beep id - one of sndBeep... constants.
 *
 * RETURNS: void
 *
 * CREATED: 4/17/95
 *
 * BY: Vitaly Kruglikov
 *
 *************************************************************/ 
void SndPlaySystemSound(SndSysBeepType beepID)
{
	SndGlobalsPtr		gP = (SndGlobalsPtr)GSndGlobalsP;
	SndCommandType		sndCmd;

	// By this time, we should have been initialized
	ErrFatalDisplayIf( !gP, "snd mgr not init" );

	//For Attention Manager, stop LED & Vibrator (along with midi sounds[they are interruptable]) if they are interruptable
#if EMULATION_LEVEL == EMULATION_NONE
	HwrLEDAttributes(true, kHwrLEDInterruptOff, NULL);
	HwrVibrateAttributes(true, kHwrVibrateInterruptOff, NULL);
#endif

	if ( beepID == sndAlarm )
		{
		PrvPlayAlarmSound();
		return;
		}

	// Set up common parameters
	sndCmd.cmd = sndCmdFrqOn;
	sndCmd.param3 = gP->sysAmp;		// default to system sound amplitude
	
	switch ( beepID )
		{
		case sndClick:
			sndCmd.param1 = sndClickFreq;
			sndCmd.param2 = sndClickDurationMSec;
			break;

		case sndInfo:
			sndCmd.param1 = sndInfoFreq;
			sndCmd.param2 = sndBeepDurationMSec;
			break;

		case sndWarning:
			sndCmd.param1 = sndWarningFreq;
			sndCmd.param2 = sndBeepDurationMSec;
			break;

		case sndError:
			sndCmd.param1 = sndErrorFreq;
			sndCmd.param2 = sndBeepDurationMSec;
			break;

		case sndStartUp:
			sndCmd.param1 = sndStartUpFreq;
			sndCmd.param2 = sndBeepDurationMSec;
			break;

		case sndConfirmation:
			sndCmd.param1 = sndConfirmationFreq;
			sndCmd.param2 = sndConfirmationDurationMSec;
			break;

		default:							// unhandled beep ID
			ErrNonFatalDisplay("bad sys sound id" );
			break;
		}
	
	SndDoCmd( 0, &sndCmd, true/*noWait*/ );
}



//-------------------------------------------------------------------
//		Sound Manager private routines
//-------------------------------------------------------------------


/************************************************************
 * FUNCTION: PrvPlayAlarmSound
 *
 * DESCRIPTION: Plays the alaram sound
 *
 * PARAMETERS:	none
 *
 * RETURNS:		nothing
 *
 * CREATED: 9/19/95
 *
 * BY: Vitaly Kruglikov
 *
 *************************************************************/
static void PrvPlayAlarmSound(void)
{
	SndGlobalsPtr		gP = (SndGlobalsPtr)GSndGlobalsP;
	SndCommandType		sndCmd;
	UInt16					i;
	
	// DOLATER... define the real alarm sound

	// Initialize once
	sndCmd.param3 = gP->alarmAmp;
	sndCmd.cmd = sndCmdFreqDurationAmp;


	i = 0;
	do {
		sndCmd.param1 = 2400;	// Hz
		sndCmd.param2 = 100;		// msec
		SndDoCmd( 0, &sndCmd, true/*noWait*/ );
	
		sndCmd.param1 = 2000;	// Hz
		sndCmd.param2 = 400;		// msec
		SndDoCmd( 0, &sndCmd, true/*noWait*/ );
		
		if ( i > 1 )
			break;
			
		SysTaskDelay( (UInt32)150 * sysTicksPerSecond / 1000 );
		
		i++;
		}
	while( true );
}

/*********************************************************************************
 *  FUNCTION: PrvDoFreqDurationAmp
 *
 *  DESCRIPTION:	Plays a sound of the given frequency, amplitude, and duration.
 *
 *  PARAMETERS:	param1		-- desired frequency in Hz
 *						param2		-- duration in milliseconds
 *						param3		-- amplitude (0 - sndMaxAmp)
 *
 *  RETURNS: nothing
 *
 *  CREATED: 8/28/95
 *
 *  BY: Vitaly Kruglikov
 *
 *********************************************************************************/
static void PrvDoFreqDurationAmp(SndCommandPtr cmdP)
{
	UInt32				waitTicks;


	// Stop the "sound kill" countdown to make sure our sound will not be
	// killed before it is finished
	prvStopSoundOffCountdown(((SndGlobalsPtr)GSndGlobalsP));					
	
	
	// Turn on the sound
	if ( !HwrSoundOn(cmdP->param1, cmdP->param3) )
		return;													// return immediately if the sound is off
	
	// Compute the sound duration in ticks
	waitTicks = sndMilliSecToPilotTicks(cmdP->param2);
	
	// If user wanted some sound then give them the minimum (vs no sound)
	if (cmdP->param2 && !waitTicks)
		waitTicks = 1;

	// Wait while sound plays
	// Add one because the next tick could come immediately.
	// (We could do slightly better by either waiting for the tick before
	//  starting or taking the time remaining until the next tick into
	//  account when rounding milliseconds to ticks.)
	if ( waitTicks )
		SysTaskDelay( waitTicks+1 );
	
	// Disable the PWM
	HwrSoundOff();
}





// timer used only under emulation
#if EMULATION_LEVEL != EMULATION_NONE
/***********************************************************************
 *
 * FUNCTION:    PrvSoundKillTimerProc
 *
 * DESCRIPTION:	Turns off the current sound.
 *
 *						This timer procedure gets called by AMX when the 
 *						Tickle timer expires. Because timer procedures run in
 *						their own context, we can not call any blocking routines
 *						from here.
 *
 *
 * CALLED BY:	 AMX when timer expires (used under emulation mode only)
 *						
 *
 * PARAMETERS:  timerID - id of timer
 *					 param 	- sound manager globals (SndGlobalsPtr)
 *
 * RETURNED:    nothing
 *
 * COMMENTS:	This timer is used under emulation mode on.  On the handheld,
 *		this functionality is accomplished from the timer tick interrupt handler.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		----------- 
 *			vmk	8/13/97	Initial version
 *
 ***********************************************************************/
static void PrvSoundKillTimerProc(Int32 /*timerID*/, Int32 /*param*/) 
{
	// Turn sound off
	HwrSoundOff();
}
#endif // EMULATION_LEVEL != EMULATION_NONE
