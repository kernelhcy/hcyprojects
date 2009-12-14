/******************************************************************************
 *
 * Copyright (c) 1995-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: SoundPrv.h
 *
 * Release: 
 *
 * Description:
 *		Private Include file for Sound Manager
 *
 * History:
 *   	4/11/95  VMK - Created by Vitaly Kruglikov
 *
 *****************************************************************************/

#ifdef	NON_PORTABLE

#ifndef __SOUNDPRV_H__
#define __SOUNDPRV_H__

// Convert milliseconds to PilotOS ticks
#define sndMilliSecToPilotTicks(milliSec)	(((milliSec) * (UInt32)sysTicksPerSecond + 500) / 1000)

#define sndPilotTicksToMilliSec(ticks)	(((ticks) * 1000L) / sysTicksPerSecond)


/************************************************************
 * Sound Manager constants
 *************************************************************/

#define	sndSamplingRate					8000	// 8 kHz


// DOLATER: determine appropriate values experimentally (TBD)
#define	sndBeepDurationMSec				70				// msec
#define	sndConfirmationDurationMSec	70				// msec
#define	sndClickDurationMSec				9				// msec

// These frequencies are more audible on the current hardware.
#define	sndInfoFreq							500			// Hz
#define	sndWarningFreq						500			// Hz
#define	sndErrorFreq						500			// Hz
#define	sndConfirmationFreq				500			// Hz
#define	sndStartUpFreq						1000			// Hz
#define	sndClickFreq						200			// Hz


/*******************************************************************
 * Sound Manager Globals
 *
 *******************************************************************/
typedef struct SndGlobalsType {
	UInt32			smID;					// semaphore id for data access integrity
	UInt8				sysAmp;				// system sound amplitude(0-sndMaxVolume)
	UInt8				alarmAmp;			// alarm sound amplitude(0-sndMaxVolume)
	UInt8				defAmp;				// default amplitude for other sounds(0-sndMaxVolume)
	UInt8				reserved;
	UInt16 *			wMidiFrqTabP;		// pointer to MIDI frequency table
#if EMULATION_LEVEL != EMULATION_NONE
	UInt32			timerID;				// "sound-kill" timer ID (emulation only)
#endif
	UInt32 			PhaseCount;
	UInt32			CurrentPhaseCount;
	Boolean  		CurrentPhase;
	UInt8 			filler[3];
#if 0
	UInt16			StopTick;
#endif
//	UInt8				SquareWaveData[2];
// UInt8				*StreamPtr;
//	UInt32			StreamSize;
//	UInt8				*CurrentPtr;
	
	Boolean     interruptSmfIrregardless;	// Flag to interrupt a non-interruptible sound
    Boolean		isSmfPlaying;				// Flag to specify whether an smf if playing
    
	} SndGlobalsType;

typedef SndGlobalsType*		SndGlobalsPtr;



/********************************************************************
 * Interal Sound Manager Routines
 ********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

//-------------------------------------------------------------------
//		Macros used only by these modules
//-------------------------------------------------------------------


// Macros for starting and stopping the "sound off" countdown.
// On the device, we rely on the timer tick interrupt handler (AmxHardwareTD1.c)
// to handle the "sound off" operation.  In emulation, we're using a "system timer".
//
#if EMULATION_LEVEL == EMULATION_NONE

	#define prvStartSoundOffCountdown(gP, dwTicks)						\
		(GSndOffTicks = (dwTicks))

	#define prvStopSoundOffCountdown(gP)									\
		(GSndOffTicks = 0)

#else		// EMULATION_LEVEL != EMULATION_NONE

	#define prvStartSoundOffCountdown(gP, dwTicks)						\
		(gP->timerID ? SysTimerWrite(gP->timerID, dwTicks) : 0)

	#define prvStopSoundOffCountdown(gP)									\
		(gP->timerID ? SysTimerWrite(gP->timerID, 0) : 0)

#endif

// Internally-used Sound Manager functions
extern Err ISndMidiInit(SndGlobalsPtr gP);
extern Err ISndMidiNoteOn(SndGlobalsPtr gP, UInt8 bKey, UInt8 bVel, UInt16 relAmp, Boolean* soundOnP);


#ifdef __cplusplus 
}
#endif


#endif  // __SOUNDPRV_H__
#endif  // NON_PORTABLE
