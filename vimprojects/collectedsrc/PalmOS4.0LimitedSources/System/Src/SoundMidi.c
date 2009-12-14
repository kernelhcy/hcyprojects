/******************************************************************************
 *
 * Copyright (c) 1997-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: SoundMidi.c
 *
 * Release: 
 *
 * Description:
 * 	            	Standard MIDI File utility routines
 *
 * History:
 *		Aug 4, 1997	Created by Vitaly M. Kruglikov
 *	Name	Date			Description
 *	----	----			-----------
 *	trm	07/28/97		ReadVarInt function added
 *
 *****************************************************************************/

#define NON_PORTABLE

#include <PalmTypes.h>

// public system includes
#include <ErrorMgr.h>
#include <SoundMgr.h>
#include <SysEvtMgr.h>		// for EvtSysEventAvail and EvtResetAutoOffTimer
#include <TimeMgr.h>

// private system includes
#include "Globals.h"
#include "MidiPrv.h"			// MIDI and SMF definitions
#include "SoundPrv.h"
#include "HwrSound.h"



// Macro for reading a single byte from the MIDI stream.
// Performs MIDI stream bounds validation and updates stream pointer.
// optimized for inline execution
//
// PAREMETERS:	stateP	-- pointer to MidiStateType structure
//					bResult	-- variable to receive the result
//					err		-- variable to receive the error code
#define prvReadSmfByte(stateP, bResult, err)																	\
	do {																													\
																															\
		ErrNonFatalDisplayIf(!(stateP), "null param");														\
																															\
																															\
		/* Make sure we're in bounds of the stream */														\
		if ( (stateP)->streamP >= (stateP)->chunkEndP && (stateP)->chunkEndP )						\
			{																												\
			ErrNonFatalDisplay("stream out of bounds");											\
			bResult = 0;													/* init result */						\
			err = sndErrBadStream;																					\
			}																												\
		else																												\
			{																												\
			bResult = *(stateP)->streamP;								/* get the next byte */				\
			(stateP)->streamP++;											/* update stream pointer */		\
			err = 0;																										\
			}																												\
	} while ( false )


// Sleep until the specified system tick time is reached; deal with early wakeups
// resulting from pen taps, etc.
//
// Will set err in case of error only; otherwize will leave it alone
#if 0
#define prvSleepUntil(__sdwEndWaitTicks__, __bInterruptible__, __err__)						\
		do {																										\
			register Int32		__sdwCurTicks__;															\
			if ( (__sdwCurTicks__ = (Int32)TimGetTicks()) < (__sdwEndWaitTicks__) )			\
				{																									\
				EvtResetAutoOffTimer();			/* so we don't fall asleep */						\
				if ( (__bInterruptible__) && EvtSysEventAvail(true/*ignorePenUps*/) )		\
					__err__ = sndErrInterrupted;															\
				else																								\
					SysTaskDelay((__sdwEndWaitTicks__) - __sdwCurTicks__);						\
				}																									\
		} while ( false )
#endif
		
#define prvInterruptibleSleepIntervalTicks		(sysTicksPerSecond/8)
inline Err PrvSleepUntil(Int32 endWaitTicks, Boolean interruptible, SndCallbackInfoType* blkHookP)
{
	Err					err = 0;
	SndGlobalsPtr    	gP = (SndGlobalsPtr)GSndGlobalsP;
	register Int32		sleepTicks;
	register Int32		maxSleepTicks;
	
	
	// Determine maximum sleep ticks once
	if ( interruptible || blkHookP )
		maxSleepTicks = prvInterruptibleSleepIntervalTicks;	// if we're interruptible, we want to check frequently
	else
		maxSleepTicks = sysTicksPerSecond * 5;						// otherwise, pick something less frequent
	
	while ( (sleepTicks = (endWaitTicks - (Int32)TimGetTicks())) > 0 )
		{
		EvtResetAutoOffTimer();			// so the device won't time out

		// If the interruptSmfIrregardless flag is set to true then we want to interrupt
		// this sound regardless of whether the interruptible flag is set.  Break
		// out of the while loop. 
		if ( gP->interruptSmfIrregardless )
			{
            gP->interruptSmfIrregardless = false;
            err = sndErrInterrupted;
            break;
			}
		
		// Check if the user is interacting with the UI
		// (DOLATER... this is not 100% reliable because the system generates key events on occasion too)
		if ( interruptible && EvtSysEventAvail(true/*ignorePenUps*/) )
			{
			err = sndErrInterrupted;
			break;
			}
		
		// Call the blocking hook function (if any)
		if ( blkHookP )
			{
			ErrNonFatalDisplayIf(!blkHookP->funcP, "null blocking hook func ptr");		// should not happen
			if ( !((*(SndBlockingFuncPtr)blkHookP->funcP)(NULL, blkHookP->dwUserData, sleepTicks)) )
				{
				err = sndErrInterrupted;
				break;
				}
				
			// Update our current time variables
			sleepTicks = endWaitTicks - (Int32)TimGetTicks();
			}
			
		// Sleep for the remainder of the time
		if ( sleepTicks > maxSleepTicks )
			sleepTicks = maxSleepTicks;
		if ( sleepTicks > 0 )
			SysTaskDelay(sleepTicks);
		else
			break;
		} // while ( (curTicks = (Int32)TimGetTicks()) < endWaitTicks )
	
	return err;
}


// Inline function for getting the beginning of the SMF stream.
// If the passed buffer begins with the PalmOS-defined header, skips
// to the SMF data, otherwise assumes we're already pointing to the SMF data
//
// PAREMETERS:	smfP	-- MIDI data buffer
inline void * PrvStartOfSMF(void * smfP)
{
	void *	smfStartP = smfP;
	UInt32	dwSig;
	
	ErrNonFatalDisplayIf(!smfP, "null SMF");
	
	
	// Need to move because smfP might not be on an even byte boundary (accessing
	// that directly as a long would cause the device to crash with address error
	// exception)
	MemMove(&dwSig, smfP, sizeof(dwSig));
	if ( dwSig == sndMidiRecSignature)
		smfStartP = (UInt8 *)smfP + ((SndMidiRecHdrType*)smfP)->bDataOffset;
	
	return smfStartP;
}


// Define the midi frequency table
// We have to use an asm function because system code cannot have globals
// and our tools do not support code-based arrays

#ifdef MIDIFrequencyTableNormalRow
	#undef MIDIFrequencyTableNormalRow
#endif

#ifdef MIDIFrequencyTableEndRow
	#undef MIDIFrequencyTableEndRow
#endif

#define PrvFrq(val)		val

#define MIDIFrequencyTableNormalRow(freq1, freq2, freq3, freq4, freq5, freq6, freq7, freq8) \
				PrvFrq(freq1), PrvFrq(freq2), PrvFrq(freq3), PrvFrq(freq4), PrvFrq(freq5), PrvFrq(freq6), PrvFrq(freq7), PrvFrq(freq8),
#define MIDIFrequencyTableEndRow(freq1, freq2, freq3, freq4, freq5, freq6, freq7, freq8) \
				PrvFrq(freq1), PrvFrq(freq2), PrvFrq(freq3), PrvFrq(freq4), PrvFrq(freq5), PrvFrq(freq6), PrvFrq(freq7), PrvFrq(freq8)


static const UInt16	kMidiFreqList[] =
{
#include "MidiFrqPrv.h"
};

static UInt16 *	MidiFrqTab(void)
{
	return (UInt16 *)kMidiFreqList;
}

	
// Macro for getting a pointer to the MIDI frequency table
#define prvMidiFrqTab()		MidiFrqTab()


/***********************************************************************
 *
 *	FUNCTION:			ISndMidiInit
 *
 *	DESCRIPTION:	Called by SndInit() to initialize the MIDI engine.
 *
 *	PARAMETERS:	gP				-- pointer to the Sound Manager globals
 *
 *	RETURNED:	zero on success
 *
 *	REVISION HISTORY:
 *		Name		Date			Description
 *		----		----			-----------
 *		vmk		9/3/97		Initial version
 *
 ***********************************************************************/
Err ISndMidiInit(SndGlobalsPtr gP)
{
	ErrNonFatalDisplayIf(!gP, "null arg");
	
	// Get a pointer to the MIDI frequency table once so we won't
	// have to call the function for each "note on" command in a stream
	// (optimization)
	gP->wMidiFrqTabP = prvMidiFrqTab();
	
	// Initialize the interruptible flags.  These flags are used by the
	// SndPlayXXXIrregardless functions below as well as in PrvSleepUntil and 
	// SndPlaySmf.
    gP->interruptSmfIrregardless = false;
    gP->isSmfPlaying = false;
	
	return 0;
}


/***********************************************************************
 *
 *	FUNCTION:			ReadSmfVarInt
 *
 *	DESCRIPTION:	Parses a variable length quantity in a Standard MIDI file
 *								into a readable UInt32 value.
 *
 *	PARAMETERS:	stateP		-- pointer to MIDI state structure
 *					dwResultP	-- pointer to variable for result
 *
 *	RETURNED:			If there was an error returns error code, if not returns
 *								readable UInt32 value and advances the MIDI stream to the first byte in
 *								MIDI file after the variable length quantity.
 *
 *	REVISION HISTORY:
 *		Name		Date			Description
 *		----		----			-----------
 *		trm		7/28/97		Initial version
 *		vmk		8/3/97		Converted to use MIDI state and added more error-checking
 *
 ***********************************************************************/
static Err ReadSmfVarInt(MidiStateType* stateP, UInt32 * dwResultP)
	{
	register	UInt8 c;
	register	UInt32	dwResult;
	//Err		err = 0;
	
	
	ErrNonFatalDisplayIf(!stateP || !dwResultP, "null param");
	
	*dwResultP = dwResult = 0;							// init result
		
	// Make sure we're in bounds of the stream
	if ( stateP->streamP >= stateP->chunkEndP && stateP->chunkEndP )
		{
		ErrNonFatalDisplay("stream out of bounds");
		return sndErrBadStream;
		}

	c = *stateP->streamP++;								// get the first byte and advance stream
	
	// If the length of the variable-length int is more than one byte...
	if ( (dwResult = (UInt32)c) & 0x080 )
		{
		dwResult &= 0x7F;
		
		// Loop until we reach the last byte of the int that does not have its MSB set
		do
			{
			// Make sure we're in bounds of the stream
			if ( stateP->streamP >= stateP->chunkEndP && stateP->chunkEndP )
				{
				ErrNonFatalDisplay("stream out of bounds");
				return sndErrBadStream;
				}
			
			// Collect the next 7 bits
			dwResult = (dwResult << 7) + ((c = *stateP->streamP++) & 0x7F);
			} while (c & 0x80);	
		}
		
	*dwResultP = dwResult;									// save result to return variable
	
	return (0);
	}


/***********************************************************************
 *
 *	FUNCTION:			ReadSmfInt
 *
 *	DESCRIPTION:	Reads a fixed length integer from a Standard MIDI file
 *						into a UInt32 value.  Updates MIDI state stream pointer (streamP).
 *						Validates stream bounds.
 *						Multi-byte intergers in SMF are stored with the most significant
 *						byte first.
 *
 *	PARAMETERS:	stateP		-- pointer to midi state structure
 *					nBytes		-- number of bytes in the integer
 *					dwResultP	-- pointer to variable for result
 *
 *	RETURNED:	non-zero on error.
 *
 *	REVISION HISTORY:
 *		Name		Date			Description
 *		----		----			-----------
 *		vmk		8/3/97		Initial version
 *
 ***********************************************************************/
static Err ReadSmfInt(MidiStateType* stateP, Int16 nBytes, UInt32 * dwResultP)
{
	register UInt32 dwResult;
	
	
	ErrNonFatalDisplayIf(!stateP || !dwResultP, "null param");
	ErrNonFatalDisplayIf(nBytes <= 0 || nBytes > 4, "fld len out of range");
	
	
	dwResult = *dwResultP = 0;				// init result
	
	// Make sure we're in bounds of the stream
	if ( (stateP->streamP + nBytes) > stateP->chunkEndP && stateP->chunkEndP )
		{
		ErrNonFatalDisplay("out of bounds");
		return sndErrBadStream;
		}
		
	while ( nBytes-- )
		{
		dwResult = (dwResult << 8) + *stateP->streamP;	// get the next byte
		stateP->streamP++;										// update stream pointer
		}
	
	*dwResultP = dwResult;
	
	return 0;
}



#if 0			// REPLACED WITH MACRO FOR OPTIMIZATION

/***********************************************************************
 *
 *	FUNCTION:			ReadSmfByte
 *
 *	DESCRIPTION:	Reads a byte from a Standard MIDI file
 *						into a UInt32 value.  Updates MIDI state stream pointer (streamP).
 *						Validates stream bounds.
 *
 *	PARAMETERS:	stateP		-- pointer to midi state structure
 *					bResultP	-- pointer to variable for result
 *
 *	RETURNED:	non-zero on error.
 *
 *	REVISION HISTORY:
 *		Name		Date			Description
 *		----		----			-----------
 *		vmk		8/3/97		Initial version
 *
 ***********************************************************************/
static Err ReadSmfByte(MidiStateType* stateP, UInt8 * bResultP)
{
	
	ErrNonFatalDisplayIf(!stateP || !bResultP, "null param");
	
	
	*bResultP = 0;													// init result
	
	// Make sure we're in bounds of the stream
	if ( stateP->streamP >= stateP->chunkEndP && stateP->chunkEndP )
		{
		ErrNonFatalDisplay("stream out of bounds");
		return sndErrBadStream;
		}

	*bResultP = *stateP->streamP;								// get the next byte
	stateP->streamP++;											// update stream pointer
		
	
	return 0;
}
#endif		// REPLACED WITH MACRO FOR OPTIMIZATION

/***********************************************************************
 *
 *	FUNCTION:			ParseSmfChunkHeader
 *
 *	DESCRIPTION:	Parses a chunk header in a Standard MIDI file.
 *						Updates MIDI state stream pointer (streamP).
 *						Validates stream bounds.
 *						Stores chunk type, size, and end pointer in the MIDI state.
 *
 *			All SMF chunks begin with the following header:
 *
 *			  32      32 (bits)
 *			<type> <length>
 *
 *			<type> is ASCII 'MThd' for Header Chunks and 'MTrk' for Track Chunks
 *			<length> is length of subsequent chunk data
 *						
 *
 *	PARAMETERS:	stateP		-- pointer to midi state structure
 *
 *	RETURNED:	non-zero on error.
 *
 *	REVISION HISTORY:
 *		Name		Date			Description
 *		----		----			-----------
 *		vmk		8/3/97		Initial version
 *
 ***********************************************************************/
static Err ParseSmfChunkHeader(MidiStateType* stateP)
{
	Err	err;
	
	ErrNonFatalDisplayIf(!stateP, "null param");
	
	
	stateP->dwChunkType = stateP->dwChunkSize = 0;
	stateP->chunkEndP = NULL;
	
	
	// Read chunk type
	err = ReadSmfInt(stateP, midiSmfChunkTypeBytes, &stateP->dwChunkType);
	if ( err )
		return err;
		
	// Read chunk length
	err = ReadSmfInt(stateP, midiSmfChunkLengthBytes, &stateP->dwChunkSize);
	if ( err )
		return err;
	
	// Save chunk end pointer for bounds checking
	stateP->chunkEndP = stateP->streamP + stateP->dwChunkSize;
	
	return 0;
}



/***********************************************************************
 *
 *	FUNCTION:			ParseSmfHeaderChunk
 *
 *	DESCRIPTION:	Parses the Header Chunk in a Standard MIDI file.
 *						Updates MIDI state stream pointer (streamP).
 *						Validates stream bounds.
 *						Fills the SMF header info in the MIDI state structure.
 *						Extracts SMF Foramt, # of tracks, and Division values.
 *
 *			The SMF Header Chunk has the following format:
 *
 *			  32      32       16      16        16 (bits)
 *			<type> <length> <format> <ntrks> <division>
 *
 *			<type> must be ASCII 'MThd'
 *			<length> is length of subsequent chunk data (format, ntrks, etc)
 *			<format>		0:	file contains single multi-channel track
 *							1: 1 or more simulataneous tracks
 *							2:	1 or more sequentially-independent single track patterns.
 *			<ntrks> is the number of traks (should be 1 for format #1)
 *
 *			<division> specifies the meaning of SMF delta-times;  there are two formats:
 *			
 *				1. Metrical (Division bit 15 (MSB) clear) and
 *				2. SMPTE time-code based (Division bit 15 (MSB) set)
 *
 *			Metrical format is in ticks per quarter note.  This is used together with
 *			Temp (microseconds per quarter note) to convert delta-time ticks to time values.
 *
 *			SMPTE format specifies SMPTE format as a negative value in bits 8-14:
 *				-24 = 24 frames per second
 *				-25 = 25 frames per second
 *				-29 = 30 frames per second (Drop Frame)
 *				-30 = 30 frames per second (Non-Drop)
 *			and the number of ticks per frame in bits 0-7.  By multiplying the number of
 *			frames per second by the number of ticks per frame, we obtain the number of
 *			SMF delta-time ticks per second.
 *
 *	PARAMETERS:	stateP		-- pointer to MIDI state structure
 *					smfP			-- pointer to SMF stream
 *
 *	RETURNED:	non-zero on error.
 *
 *	REVISION HISTORY:
 *		Name		Date			Description
 *		----		----			-----------
 *		vmk		8/3/97		Initial version
 *
 ***********************************************************************/
static Err ParseSmfHeaderChunk(MidiStateType* stateP, UInt8 * smfP)
{
	Err		err;
	UInt32		dw;
	UInt32		dwDiv;
	UInt32		dwFramesPerSec;
	UInt32		dwTicksPerFrame;
	
	
	ErrNonFatalDisplayIf(!stateP || !smfP, "null param");

	// Init state machine
	MemSet(stateP, sizeof(*stateP), 0);
	stateP->hdr.wFmt = midiSmfFmtUnknown;
	stateP->dwTempo = midiDefTempo;
	stateP->streamP = smfP;

	
	// Parse the header of the SMF header chunk
	err = ParseSmfChunkHeader(stateP);
	if ( err )
		return err;
	
	// Validate chunk type
	if ( stateP->dwChunkType != midiSmfHeaderChunkType )
		{
		ErrNonFatalDisplay("bad header chunk type");
		return sndErrBadStream;
		}
	
	// Validate chunk length
	if ( stateP->dwChunkSize < midiSmfMinHdrChunkDataSize )
		{
		ErrNonFatalDisplay("SMF hdr too small");
		return sndErrBadStream;
		}
		
	// Get SMF Format
	err = ReadSmfInt(stateP, midiSmfHdrFmtBytes, &dw);
	if ( err )
		return err;
	// Validate SMF format range
	if ( dw > midiSmfMaxValidFmt )
		{
		ErrNonFatalDisplay("bad SMF fmt");
		return sndErrBadStream;
		}
	// For now, we only MemHandle SMF Format #0
	if ( dw != 0 )
		{
		ErrNonFatalDisplay("unsupported SMF fmt");
		return sndErrFormat;
		}
	stateP->hdr.wFmt = (UInt16)dw;						// save format to state
	
	// Get # of tracks
	err = ReadSmfInt(stateP, midiSmfHdrTrkBytes, &dw);
	if ( err )
		return err;
	stateP->hdr.wNumTracks = (UInt16)dw;				// save # of tracks to state
	
	// Get SMF Division
	err = ReadSmfInt(stateP, midiSmfHdrDivBytes, &dwDiv);
	if ( err )
		return err;
	stateP->hdr.wDivRaw = (UInt16)dwDiv;				// save raw Division to state
	
	// If Division is in SMPTE time code
	if ( dwDiv & midiSmfSMPTEDivBit )
		{
		Int8		sbFmt;
		stateP->hdr.bUsingSMPTE = true;				// save use of SMPTE time code to state
		
		// Determine frames per second
		sbFmt = (Int8)(dwDiv >> 8);					// get the SMPTE format
		switch ( sbFmt )
			{
			case midiSmfNegSMPTEFmt24:
				dwFramesPerSec = 24;
				break;
				
			case midiSmfNegSMPTEFmt25:
				dwFramesPerSec = 25;
				break;
				
			case midiSmfNegSMPTEFmt30Drop:
			case midiSmfNegSMPTEFmt30NonDrop:
				dwFramesPerSec = 30;
				break;
				
			default:
				dwFramesPerSec = 0;
				break;
			}
			
		if ( dwFramesPerSec == 0 )
			{
			ErrNonFatalDisplay("bad SMPTE div format");
			return sndErrBadStream;
			}
		
		// Compute SMF ticks per second
		dwTicksPerFrame = dwDiv & midiSmfSMPTEDivTicksMask;
		stateP->hdr.dwTicksPerSec = dwFramesPerSec * dwTicksPerFrame;
		}
		
	else		// Division is metrical time in ticks per quarter note
		{
		stateP->hdr.bUsingSMPTE = false;
		stateP->hdr.wTicksPerQN = (UInt16)dwDiv;
		}
	
	// Skip to the end of the header chunk (it may have been extended in subsequent revisions)
	stateP->streamP = stateP->chunkEndP;

	return 0;
}


/***********************************************************************
 *
 *	FUNCTION:			ParseSmfTrackChunkHeader
 *
 *	DESCRIPTION:	Parses and validates the header of the Track Chunk in a Standard MIDI file.
 *						Validates stream bounds.
 *
 *			The SMF Track Chunk has the following format:
 *
 *			  32      32 (bits)
 *			<type> <length> <MTrk event>+
 *
 *			<type> must be ASCII 'MTrk'
 *			<length> is length of subsequent chunk data (events)
 *			<MTrk event>+ is one or more SMF events
 *
 *			                (var len)
 *			<MTrk event> = <delta-time> <event>
 *			<delta-time> is the amount of time before the follwing event in MIDI ticks
 *
 *			<event> = <MIDI event> | <sysex event> | <meta-event>
 *			<MIDI event> is any MIDI channel messages; running status is used, and
 *					occurs across delta-times.
 *			<sysex event> specifiesMIDI System Exclusive messages.
 *			                   (val len)
 *			<sysex event> = F0 <length> <bytes to transmit after F0>
 *										or
 *			                F7 <length> <bytes to transmit>
 *
 *			The F7 form is a special escape form that does not imply that F0 (or F7) is
 *			to be transmitted.
 *
 *			<meta-event> is a non-MIDI information useful to the SMF format or to sequencers.
 *			                  byte   (var len)
 *			<meta-event> = FF <type> <length> <bytes>
 *			                  0-127
 *
 *	PARAMETERS:	stateP		-- pointer to MIDI state structure
 *
 *	RETURNED:	non-zero on error.
 *
 *	REVISION HISTORY:
 *		Name		Date			Description
 *		----		----			-----------
 *		vmk		8/3/97		Initial version
 *
 ***********************************************************************/
static Err ParseSmfTrackChunkHeader(MidiStateType* stateP)
{
	Err		err;	
	
	ErrNonFatalDisplayIf(!stateP, "null param");


	//// Init SMF header info
	//stateP->hdr.wFmt = midiSmfFmtUnknown;
	
	// Parse the header of the SMF Track Chunk
	err = ParseSmfChunkHeader(stateP);
	if ( err )
		return err;
	
	// Validate chunk type
	if ( stateP->dwChunkType != midiSmfTrackChunkType )
		{
		ErrNonFatalDisplay("bad track chunk type");
		return sndErrBadStream;
		}
	
	// Validate chunk length
	if ( stateP->dwChunkSize < midiSmfMinTrkChunkDataSize )
		{
		ErrNonFatalDisplay("SMF track chunk too small");
		return sndErrBadStream;
		}

	return 0;
}


/***********************************************************************
 *
 *	FUNCTION:			MidiTicksToPilotTicks
 *
 *	DESCRIPTION:	Converts MIDI ticks to PilotOS ticks.
 *
 *
 *	PARAMETERS:	stateP		-- pointer to MIDI state structure
 *					dwMidiTicks	-- number of midi ticks
 *
 *	RETURNED:	number of PilotOS ticks.
 *
 *	REVISION HISTORY:
 *		Name		Date			Description
 *		----		----			-----------
 *		vmk		8/3/97		Initial version
 *
 ***********************************************************************/
static UInt32 MidiTicksToPilotTicks(MidiStateType* stateP, UInt32 dwMidiTicks)
{
	UInt32		dwPilotTicks = 0;


	if ( dwMidiTicks == 0 )
		return 0;
		
	
	// If using metrical time...
	if ( !stateP->hdr.bUsingSMPTE )
		{
		dwPilotTicks = dwMidiTicks * (stateP->dwTempo / stateP->hdr.wTicksPerQN) *
							sysTicksPerSecond / 1000000L;
		}
	
	// Using SMPTE time code based time
	else
		{
		dwPilotTicks = dwMidiTicks * sysTicksPerSecond / stateP->hdr.dwTicksPerSec;
		}
	
	// Make sure the minimum is one PilotOS tick
	if ( dwPilotTicks == 0 )
		dwPilotTicks = 1;
		
	return dwPilotTicks;
}


/***********************************************************************
 *
 *	FUNCTION:			GetNextSmfEvent
 *
 *	DESCRIPTION:	Gets the next SMF delta-time and event from the SMF stream.
 *
 *			The SMF Track event has the following format:
 *
 *
 *			                (var len)
 *			<MTrk event> = <delta-time> <event>
 *			<delta-time> is the amount of time before the follwing event in MIDI ticks
 *
 *			<event> = <MIDI event> | <sysex event> | <meta-event>
 *			<MIDI event> is any MIDI channel messages; running status is used, and
 *					occurs across delta-times.
 *			<sysex event> specifiesMIDI System Exclusive messages.
 *			                   (val len)
 *			<sysex event> = F0 <length> <bytes to transmit after F0>
 *										or
 *			                F7 <length> <bytes to transmit>
 *
 *			The F7 form is a special escape form that does not imply that F0 (or F7) is
 *			to be transmitted.
 *
 *			<meta-event> is a non-MIDI information useful to the SMF format or to sequencers.
 *			                  byte   (var len)
 *			<meta-event> = FF <type> <length> <bytes>
 *			                  0-127
 *
 *	PARAMETERS:	stateP		-- pointer to MIDI state structure
 *
 *	RETURNED:	non-zero on error.
 *
 *	REVISION HISTORY:
 *		Name		Date			Description
 *		----		----			-----------
 *		vmk		8/3/97		Initial version
 *
 ***********************************************************************/
static Err GetNextSmfEvent(MidiStateType* stateP, MidiEventType* evtP)
{
	Err		err;
	UInt32		dwDeltaTicks;
	UInt32		dwLen;
	UInt32		dwTempo;
	UInt8 *	bEndP;
	UInt8		bStatus;
	UInt8		b1;
	UInt8		b2;
	UInt8		bType;
	Boolean	bNewStatus = false;
	
	ErrNonFatalDisplayIf(!stateP || !evtP, "null param");

	b1 = b2 = 0;
	evtP->id = eMidiSmfEvtUnknown;
	evtP->evtClass = eMidiSmfEvtClassUnknown;
	
	//
	// Parse the SMF event delta-time
	
	// Get event delta-time
	err = ReadSmfVarInt(stateP, &dwDeltaTicks);
	if ( err )
		return err;
		
	// Convert to PilotOS ticks and save to event
	evtP->dwDeltaTicks = MidiTicksToPilotTicks(stateP, dwDeltaTicks);
	
	//
	// Parse the SMF event
	//
	
	// Get the first byte.  (This may be a new status byte or the first arg of
	// the current running status)
	prvReadSmfByte(stateP, b1, err);
	if ( err )
		return err;
	
	bStatus = stateP->bMsgStatus;					// get current running status, if any
	
	// If we have a new status...
	if ( b1 & midiStatusBit )
		{
		bNewStatus = true;
		bStatus = stateP->bMsgStatus = b1;		// save the new status for now
		}
	
	// Make sure we have a status byte
	if ( bStatus == midiMsgStatusNone )
		{
		ErrNonFatalDisplay("no MIDI status");
		return sndErrBadStream;
		}
	
	//
	// MemHandle channel messages first
	//
	if ( bStatus <= midiMaxChanMsgStatus )
		{
		// They have at least one arg
		
		// Get the first arg if necessary
		if ( bNewStatus )
			{
			prvReadSmfByte(stateP, b1, err);
			if ( err )
				return err;
			}
		
		// Some channel messages have a second arg
		if ( bStatus < midiMsgBaseProgChange || bStatus >= midiMsgBasePitchBend )
			{
			prvReadSmfByte(stateP, b2, err);
			if ( err )
				return err;
			}
		
		// Store event id and class, and channel # to the event structure
		evtP->id = (MidiSmfEvtEnum)(bStatus & midiChanMsgMask);
		evtP->evtClass = eMidiSmfEvtClassChannel;
		evtP->bChan = (UInt8) (bStatus & midiChanNumMask);
		
		// Digest parameters for channel messages of interest
		if ( evtP->id == eMidiSmfEvtNoteOff || evtP->id == eMidiSmfEvtNoteOn )
			{
			// Special case: "note on" with velocity of 0 = "note off"
			if ( evtP->id == eMidiSmfEvtNoteOn && b2 == 0 )
				{
				evtP->id = eMidiSmfEvtNoteOff;
				b2 = midiDefVel;
				}
				
			evtP->evtData.note.bKey = b1;				// MIDI key #
			ErrNonFatalDisplayIf(b1 > midiMaxKey, "MIDI key out of bounds");
			
			evtP->evtData.note.bVel = b2;				// MIDI vel
			ErrNonFatalDisplayIf(b2 > midiMaxVel, "MIDI vel out of bounds");
			}
		
		return 0;
		} // MemHandle channel messages first
		
	
	// All other SMF messages reset running status
	stateP->bMsgStatus = midiMsgStatusNone;
	
	
	//
	// MemHandle SMF Meta events
	//
	
	if ( bStatus == midiSmfEvtStatusMeta )
		{
		evtP->id = eMidiSmfMetaEvtOther;
		evtP->evtClass = eMidiSmfEvtClassMeta;
		
		// Get type and length
		prvReadSmfByte(stateP, bType, err);
		if ( err )
			return err;
		
		err = ReadSmfVarInt(stateP, &dwLen);
		if ( err )
			return err;
			
		// Get poiner to end of the event data
		bEndP = stateP->streamP + dwLen;
		
		// MemHandle meta events of interest
		
		// End of track?
		if ( bType == midiSmfMetaEvtTypeEOT )
			{
			evtP->id = eMidiSmfMetaEvtEOT;
			}
		
		// Set tempo ?
		else
		if ( bType == midiSmfMetaEvtTypeTempo )
			{
			if ( dwLen < midiSmfTempoBytes )
				{
				ErrNonFatalDisplay("invalid tempo fld size");
				return sndErrBadStream;
				}
			// Read the tempo value
			err = ReadSmfInt(stateP, midiSmfTempoBytes, &dwTempo);
			if ( err )
				return err;
			evtP->id = eMidiSmfMetaEvtTempo;
			evtP->evtData.setTempo.dwTempo = dwTempo;
			}
		
		// Advance the MIDI stream past the remaining meta-event data
		stateP->streamP = bEndP;
		
		return 0;
		}
	
	
	//
	// MemHandle System Exclusive events
	//
	
	if ( bStatus == midiSmfEvtStatusSysExF0 || bStatus == midiSmfEvtStatusSysExF0 )
		{
		// Get the event data length
		err = ReadSmfVarInt(stateP, &dwLen);
		if ( err )
			return err;

		// Advance the MIDI stream past the event
		stateP->streamP += dwLen;
		
		evtP->id = (bStatus == midiSmfEvtStatusSysExF0) ? eMidiSmfEvtSysEx1 : eMidiSmfEvtSysEx2;
		evtP->evtClass = eMidiSmfEvtClassSysEx;
		
		return 0;
		}


	// If we're here, there was an unrecognized SMF event
	ErrNonFatalDisplay("unknown SMF evt");
	return sndErrBadStream;
}



/***********************************************************************
 *
 *	FUNCTION:			ISndMidiNoteOn
 *
 *	DESCRIPTION:	Turns a note on.
 *
 *
 *	PARAMETERS:	gP				-- pointer to sound manager globals
 *					bKey			-- MIDI key number (0-127)
 *					bVel			-- key velocity (0-127) used for amplitude control
 *					soundOnP		-- pointer to variable for returning a flag indicating
 *									   whether the sound was turned on; the sound might not be
 *									   turned on if the resulting amplitude is zero
 *					relAmp		-- relative sound manager amplitude (0 - sndMaxAmp)
 *
 *	RETURNED:	non-zero on error.
 *
 *	REVISION HISTORY:
 *		Name		Date			Description
 *		----		----			-----------
 *		vmk		8/4/97		Initial version
 *
 ***********************************************************************/
Err ISndMidiNoteOn(SndGlobalsPtr gP, UInt8 bKey, UInt8 bVel, UInt16 relAmp, Boolean* soundOnP)
{
	UInt16		wFrq;
	UInt16		wAmp;
	
	ErrNonFatalDisplayIf(!gP || !soundOnP, "null param");
	ErrNonFatalDisplayIf(!gP->wMidiFrqTabP, "null MIDI tab");
	ErrNonFatalDisplayIf(relAmp > sndMaxAmp, "relative amplitude out of bounds");
	
	if ( bKey > midiMaxKey )
		{
		ErrNonFatalDisplay("MIDI key out of bounds");
		*soundOnP = false;
		return sndErrBadStream;
		}
	
	wFrq = gP->wMidiFrqTabP[bKey];
	wAmp = (UInt16)((relAmp * bVel) / midiMaxVel);
	*soundOnP = HwrSoundOn(wFrq, wAmp);
		
	return 0;
}



/***********************************************************************
 *
 *	FUNCTION:			GetSmfDuration
 *
 *	DESCRIPTION:	Measures duration of a Standard MIDI File.  Only SMF Format #0 is supported.
 *
 *
 *	PARAMETERS:	stateP		-- pointer to structure to use for storing MIDI state
 *					evtP			-- pointer to structure to use for storing SMF event data
 *					smfP			-- pointer to SMF stream
 *					optP			-- pointer to structure for returning duration in milliseconds
 *									   the dwStartMilliSec field will be set to zero;
 *										the dwEndMilliSec field will be set to duration in milliseconds
 *
 *	RETURNED:	non-zero on error.
 *
 *	REVISION HISTORY:
 *		Name		Date			Description
 *		----		----			-----------
 *		vmk		8/6/97		Initial version
 *
 ***********************************************************************/
static Err GetSmfDuration(MidiStateType* stateP, MidiEventType* evtP, UInt8 * smfP,
		SndSmfOptionsType* optP)
{
	Err		err = 0;
	register UInt32	dwTotTicks;


	// Some error checking first
	ErrNonFatalDisplayIf(!smfP, "null param");
	ErrFatalDisplayIf(!optP || !stateP, "null param");


	// If the track starts with the PalmOS-defined header, advance to
	// the real SMF data
	smfP = PrvStartOfSMF(smfP);
	
	optP->dwStartMilliSec = optP->dwEndMilliSec = dwTotTicks = 0;	// init accumulators

	// Parse the SMF Header Chunk
	err = ParseSmfHeaderChunk(stateP, smfP);
	if ( err )
		return err;
	
	// Parse the header of the SMF Track Chunk
	err = ParseSmfTrackChunkHeader(stateP);
	if ( err )
		return err;
		
	

	//
	// Enumerate the MIDI events
	//
	
	
		
	while ( true )
		{
		err = GetNextSmfEvent(stateP, evtP);
		if ( err )
			break;
		
		dwTotTicks += evtP->dwDeltaTicks;					// accumulate delta-times in PilotOS ticks

		// MemHandle meta events of interest
		if ( evtP->evtClass == eMidiSmfEvtClassMeta )
			{
			// End of track ?
			if ( evtP->id == eMidiSmfMetaEvtEOT )
				break;												// we're out of here!!!
				
			// Set tempo ?
			if ( evtP->id == eMidiSmfMetaEvtTempo )
				{
				stateP->dwTempo = evtP->evtData.setTempo.dwTempo;
				// Keep going
				continue;
				}
			}
		
		}	// while
	
	
	// Convert PilotOS ticks to milliseconds
	if ( !err )
		{
		optP->dwEndMilliSec = sndPilotTicksToMilliSec(dwTotTicks);
		}
		
		
	return err;
}


/***********************************************************************
 *
 *	FUNCTION:			SndPlaySMF
 *
 *	DESCRIPTION:	Plays a Standard MIDI File.  Only SMF Format #0 is supported.
 *						if chanP is NULL, bNoWait is ignored, and the routine will block
 *						until completion.  Since we do not presently support creation
 *						of new channels (chanP must always be passed as NULL), this routine
 *						will always block in its present incarnation.
 *
 *
 *	PARAMETERS:	chanP			-- pointer to sound channel (must be NULL in present implementation)
 *					smfP			-- pointer to SMF stream
 *					optP			-- pointer to sound selection (start, end); NULL to play entire file
 *					chanRangeP	-- pointer range of MIDI channels to recognize; NULL to play all channels
 *					callbacksP	-- pointer to completion function to be called when playing ends;
 *										pass NULL if this callback is not desired; the callback is passed
 *										the value of chanP and the dwUser value; the callback is made
 *										from an interrupt, and is not allowed to make any system calls
 *										or any other calls which may indirectly call system calls;  since
 *										the A5 globals world is not set up during this call, the caller can
 *										set dwUser to point at an app's global variable to get access to
 *										it; the purpose may be to set a flag in the app's globals indicating
 *										that the the current SMF stream may be released and a new one
 *										can be started during the next "get next event" cycle;  the
 *										function must return quickly (much less than one system tick).
 *					dwUser		-- caller-provided value to be passed as the second parameter in
 *										the completion function;  if complFuncP is NULL, this value
 *										is not used.
 *					bNoWait		-- true to return immediately and play the sound asynchronously;
 *										asynchronous playing is not supported in this version of Sound
 *										Manager; therefore, this parameter is ignored and the call will
 *										always block until the sound play is compelete.
 *
 *	RETURNED:	non-zero on error.
 *
 *	REVISION HISTORY:
 *		Name		Date			Description
 *		----		----			-----------
 *		vmk		8/4/97		Initial version
 *
 ***********************************************************************/
Err SndPlaySmf(void* chanP, SndSmfCmdEnum cmd, UInt8 * smfP, SndSmfOptionsType* optP,
		SndSmfChanRangeType* chanRangeP, SndSmfCallbacksType* callbacksP, Boolean /*bNoWait*/)
{
	Err		err = 0;
	SndGlobalsPtr	gP = (SndGlobalsPtr)GSndGlobalsP;
	register UInt32		dwWaitTicks;
	register	UInt32		dwSkipTicks = 0;
	register	UInt32		dwStopTicks = 0xFFFFFFFFUL;
	register	UInt32		dwRunTicks;
	register UInt8		bFirstChan, bLastChan;
	Int32	sdwStartWaitTicks;
	Int32	sdwEndWaitTicks;
	MidiStateType	state;
	MidiEventType	evt;
	SndCallbackInfoType*	blkHookP = NULL;
	UInt16		amplitude = sndMaxAmp;
	UInt8		bCurChan = 0xFF;
	UInt8		bCurKey = 0xFF;
	UInt8		bVel = 0;
	Boolean	bPlayingStarted;
	Boolean	bHwEnabled = false;
	Boolean	bInterruptible = true;				// DOLATER... make this a command option
	Boolean	soundOn;


	// Error checking first
	ErrFatalDisplayIf( !gP, "snd mgr not init" );
	ErrFatalDisplayIf(chanP, "invalid channel");
	ErrNonFatalDisplayIf(optP && optP->reserved != 0, "SndSmfOptionsType.reserved not zero");

	// Dispatch the command
	if ( cmd == sndSmfCmdDuration )	
		return GetSmfDuration(&state, &evt, smfP, optP);
	
	// The only other command we MemHandle for now is "Play"
	if ( cmd != sndSmfCmdPlay )
		{
		ErrNonFatalDisplay("invalid SndPlaySmf cmd");
		return sndErrBadParam;
		}
		
	// Some more error checking pertinent to this command
	ErrNonFatalDisplayIf(!smfP, "null param");
	
	// Set up once
	if ( callbacksP && callbacksP->blocking.funcP )
		blkHookP = &callbacksP->blocking;
	
	// If the track starts with the PalmOS-defined header, advance to
	// the real SMF data
	smfP = PrvStartOfSMF(smfP);

	// Get exclusive access to channel
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
	
	// Set the isSmfPlaying flag to true to indicate that a smf file is
	// being played.  This is necesasry for the XXXXIrregardless functions below.
	gP->isSmfPlaying = true;
	
	// Get channel range
	if ( chanRangeP )
		{
		bFirstChan = chanRangeP->bFirstChan;
		bLastChan = chanRangeP->bLastChan;
		}
	else
		{
		bFirstChan = midiMinChan;
		bLastChan = midiMaxChan;
		}
	
	if ( bFirstChan > midiMaxChan || bLastChan > midiMaxChan || bFirstChan > bLastChan )
		{
		ErrNonFatalDisplay("chan #'s out of bounds or order");
		err = sndErrBadParam;
		goto Exit;
		}
		
	
	// Parse the SMF Header Chunk
	err = ParseSmfHeaderChunk(&state, smfP);
	if ( err )
		goto Exit;
	
	// Parse the header of the SMF Track Chunk
	err = ParseSmfTrackChunkHeader(&state);
	if ( err )
		goto Exit;
		
	// Save channel bounds to state
	state.bFirstChan = bFirstChan;
	state.bLastChan = bLastChan;
	
	// Get sound selection
	if ( optP )
		{
		dwSkipTicks = sndMilliSecToPilotTicks(optP->dwStartMilliSec);
		if ( optP->dwEndMilliSec != sndSmfPlayAllMilliSec )
			dwStopTicks = sndMilliSecToPilotTicks(optP->dwEndMilliSec);
		amplitude = optP->amplitude;
		bInterruptible = optP->interruptible;
		ErrNonFatalDisplayIf(amplitude > sndMaxAmp, "invalid SMF amplitude passed");
		}

	
	// If amplitude is zero, skip the play
	if ( amplitude == 0 )
		goto Exit;


	//
	// Process the MIDI events
	//
	
	// Enable the sound harware
	bHwEnabled = true;
	
	
	bPlayingStarted = false;
	dwWaitTicks = dwRunTicks = 0;
	sdwStartWaitTicks = (Int32)TimGetTicks();
	
	while ( true )
		{
		// Exit if we ran out of time
		if ( dwRunTicks >= dwStopTicks )
			break;
					
		err = GetNextSmfEvent(&state, &evt);
		if ( err )
			break;
		
		dwRunTicks += evt.dwDeltaTicks;				// maintain our running tick count
		
		if ( !bPlayingStarted )
			{
			if ( dwRunTicks >= dwSkipTicks )
				{
				bPlayingStarted = true;
				dwWaitTicks = dwRunTicks - dwSkipTicks;
				}
			}
		else
			dwWaitTicks += evt.dwDeltaTicks;			// accumulate delta-times
		
		// MemHandle channel events
		if ( evt.evtClass == eMidiSmfEvtClassChannel && bPlayingStarted )
			{
			// Filter out messages for unwanted channels
			if ( evt.bChan < state.bFirstChan || evt.bChan > state.bLastChan )
				continue;
			
			// Process "note on"
			if ( evt.id == eMidiSmfEvtNoteOn )
				{
				bCurChan = evt.bChan;
				bCurKey = evt.evtData.note.bKey;
				bVel = evt.evtData.note.bVel;
				
				// Sleep until it's time for the event
				sdwEndWaitTicks = sdwStartWaitTicks + dwWaitTicks;
				//prvSleepUntil(sdwEndWaitTicks, bInterruptible, err);
				err = PrvSleepUntil(sdwEndWaitTicks, bInterruptible, blkHookP);
				if ( err )
					break;
				
				// Turn on the note
				err = ISndMidiNoteOn(gP, bCurKey, bVel, amplitude, &soundOn);
				if ( err )
					break;
				
				// Restart the time counters
				dwWaitTicks = 0;
				sdwStartWaitTicks = (Int32)TimGetTicks();

				// Keep going
				continue;
				}
			
			// Process "note off"
			if ( evt.id == eMidiSmfEvtNoteOff && evt.bChan == bCurChan &&
					evt.evtData.note.bKey == bCurKey )
				{
				// Sleep until it's time for the event
				sdwEndWaitTicks = sdwStartWaitTicks + dwWaitTicks;
				//prvSleepUntil(sdwEndWaitTicks, bInterruptible, err);
				err = PrvSleepUntil(sdwEndWaitTicks, bInterruptible, blkHookP);
				if ( err )
					break;
				
				// Turn off the note
				HwrSoundOff();
				
				// Restart the time counters
				dwWaitTicks = 0;
				sdwStartWaitTicks = (Int32)TimGetTicks();
				
				// Reset current note info
				bCurChan = 0xFF;
				bCurKey = 0xFF;

				// Keep going
				continue;
				}
			}

		// MemHandle meta events of interest
		else
		if ( evt.evtClass == eMidiSmfEvtClassMeta )
			{
			// End of track ?
			if ( evt.id == eMidiSmfMetaEvtEOT )
				{
				// Sleep until it's time for the event
				sdwEndWaitTicks = sdwStartWaitTicks + dwWaitTicks;
				//prvSleepUntil(sdwEndWaitTicks, bInterruptible, err);
				err = PrvSleepUntil(sdwEndWaitTicks, bInterruptible, blkHookP);
				break;												// we're out of here!!!
				}
				
			// Set tempo ?
			if ( evt.id == eMidiSmfMetaEvtTempo )
				{
				state.dwTempo = evt.evtData.setTempo.dwTempo;
				// Keep going
				continue;
				}
			}
		
		// We don't MemHandle sysex messages
		
		}	// while
	
	
	
Exit:
	// Make sure sound is OFF and disable the hardware
	if ( bHwEnabled )
		{
		HwrSoundOff();
		}
	
	// Call completion function and exit
	if ( callbacksP && callbacksP->completion.funcP )
		(*(SndComplFuncPtr)(callbacksP->completion.funcP))(chanP, callbacksP->completion.dwUserData);

	// Set the isSmfPlaying flag back to false now that we are finished playing
	// our sound.
	gP->isSmfPlaying = false;

	// Release exclusive access to channel
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.

	return err;
}


/***********************************************************************
 *
 *	FUNCTION:		SndPlaySmfResource
 *
 *	DESCRIPTION:	Plays a MIDI sound which is read out of an open resource database.
 *						Volume will be read out of the system prefs for the given volumeSelector
 *						which should be set to prefSysSoundVolume, prefGameSoundVolume,
 *						or prefAlarmSoundVolume.  The sound will be interrupted by a
 *						key or digitizer event.
 *
 *	PARAMETERS:	resType			-- card number of the database containing the smf record
 *					resID				-- ID of the resource
 *					volumeSelector	-- which volume setting to use (prefAlarmSoundVolume, etc.)
 *
 *	RETURNED:	non-zero on error.
 *
 *	REVISION HISTORY:
 *		Name		Date			Description
 *		----		----			-----------
 *		daf		10/7/98		Initial version
 *
 ***********************************************************************/
Err SndPlaySmfResource(UInt32 resType, Int16 resID, SystemPreferencesChoice volumeSelector)
{
	Err			err=0;
	MemHandle		midiH;							// MemHandle of MIDI record
	SndMidiRecHdrType*	midiHdrP;			// pointer to MIDI record header
	UInt8 *		midiStreamP;					// pointer to MIDI stream beginning with the 'MThd'
														// SMF header chunk
	SndSmfOptionsType	smfOpt;					// SMF play options
	
	// validate the volumeSelector...
	if ((volumeSelector!=prefSysSoundVolume) &&
	    (volumeSelector!=prefGameSoundVolume) &&
	    (volumeSelector!=prefAlarmSoundVolume))
		return sndErrBadParam;
	
	midiH = DmGetResource(resType, resID);
	if (!midiH)
		err = dmErrCantFind;

	// Lock the record and play the sound
	if (!err) {
		// Find the record MemHandle and lock the record
		midiHdrP = MemHandleLock(midiH);
		
		// Get a pointer to the SMF stream
		midiStreamP = (UInt8 *)midiHdrP + midiHdrP->bDataOffset;
		
		// Play the sound
		// The sound can be interrupted by a key/digitizer event
		smfOpt.dwStartMilliSec = 0;
		smfOpt.dwEndMilliSec = sndSmfPlayAllMilliSec;
		smfOpt.amplitude = (UInt16)PrefGetPreference(volumeSelector);
		smfOpt.interruptible = true;
		smfOpt.reserved = 0;
		err = SndPlaySmf (NULL, sndSmfCmdPlay, midiStreamP, &smfOpt, NULL, NULL, false);
		
		// Unlock the record
		MemPtrUnlock (midiHdrP);
	}
	
	// Release the resource
	if (midiH)
		DmReleaseResource(midiH);
	
	return err;
}

/***********************************************************************
 *
 *	FUNCTION:			SndPlaySmfIrregardless
 *
 *	DESCRIPTION:	Plays a Standard MIDI File.  Only SMF Format #0 is supported.
 *						if chanP is NULL, bNoWait is ignored, and the routine will block
 *						until completion.  Since we do not presently support creation
 *						of new channels (chanP must always be passed as NULL), this routine
 *						will always block in its present incarnation.
 *
 *					This function will interrupt any currently playing smf file to play
 *					the new sound regardless of whether the interruptible flag was set.
 *
 *	PARAMETERS:	chanP			-- pointer to sound channel (must be NULL in present implementation)
 *					smfP			-- pointer to SMF stream
 *					optP			-- pointer to sound selection (start, end); NULL to play entire file
 *					chanRangeP	-- pointer range of MIDI channels to recognize; NULL to play all channels
 *					callbacksP	-- pointer to completion function to be called when playing ends;
 *										pass NULL if this callback is not desired; the callback is passed
 *										the value of chanP and the dwUser value; the callback is made
 *										from an interrupt, and is not allowed to make any system calls
 *										or any other calls which may indirectly call system calls;  since
 *										the A5 globals world is not set up during this call, the caller can
 *										set dwUser to point at an app's global variable to get access to
 *										it; the purpose may be to set a flag in the app's globals indicating
 *										that the the current SMF stream may be released and a new one
 *										can be started during the next "get next event" cycle;  the
 *										function must return quickly (much less than one system tick).
 *					dwUser		-- caller-provided value to be passed as the second parameter in
 *										the completion function;  if complFuncP is NULL, this value
 *										is not used.
 *					bNoWait		-- true to return immediately and play the sound asynchronously;
 *										asynchronous playing is not supported in this version of Sound
 *										Manager; therefore, this parameter is ignored and the call will
 *										always block until the sound play is compelete.
 *
 *	RETURNED:	non-zero on error.
 *
 *	REVISION HISTORY:
 *		Name		Date			Description
 *		----		----			-----------
 *		rkr		9/12/00		Added support for interrupting sounds regardless of the interruptible flag
 *
 ***********************************************************************/
Err SndPlaySmfIrregardless(void* chanP, SndSmfCmdEnum cmd, UInt8 * smfP, SndSmfOptionsType* optP,
        SndSmfChanRangeType* chanRangeP, SndSmfCallbacksType* callbacksP, Boolean bNoWait)
{
	Err				err;
    SndGlobalsPtr   gP = (SndGlobalsPtr)GSndGlobalsP;
    
    
    // If a sound is currently playing then set the interruptSmfIrregardless
    // to true so that the currently playing sound is interrupted.
    if (gP->isSmfPlaying)
   		gP->interruptSmfIrregardless = true;
   		
   	// Call the original function now that we've set our flag
    err = SndPlaySmf( chanP, cmd, smfP, optP, chanRangeP, callbacksP, bNoWait );
    
    return (err);
}

/***********************************************************************
 *
 *	FUNCTION:		SndPlaySmfResourceIrregardless
 *
 *	DESCRIPTION:	Plays a MIDI sound which is read out of an open resource database.
 *						Volume will be read out of the system prefs for the given volumeSelector
 *						which should be set to prefSysSoundVolume, prefGameSoundVolume,
 *						or prefAlarmSoundVolume.  The sound will be interrupted by a
 *						key or digitizer event.
 *
 *					This function will interrupt any currently playing smf file to play
 *					the new sound regardless of whether the interruptible flag was set.
 *
 *	PARAMETERS:	resType			-- card number of the database containing the smf record
 *					resID				-- ID of the resource
 *					volumeSelector	-- which volume setting to use (prefAlarmSoundVolume, etc.)
 *
 *	RETURNED:	non-zero on error.
 *
 *	REVISION HISTORY:
 *		Name		Date			Description
 *		----		----			-----------
 *		rkr		9/12/00		Added support for interrupting sounds regardless of the interruptible flag
 *
 ***********************************************************************/
Err SndPlaySmfResourceIrregardless(UInt32 resType, Int16 resID, SystemPreferencesChoice volumeSelector)
{
	Err				err;
    SndGlobalsPtr   gP = (SndGlobalsPtr)GSndGlobalsP;
    
    // If a sound is currently playing then set the interruptSmfIrregardless
    // to true so that the currently playing sound is interrupted.
    if (gP->isSmfPlaying)
   		gP->interruptSmfIrregardless = true;
    
    // Call the original function now that we've set our flag
    err = SndPlaySmfResource( resType, resID, volumeSelector );
    
    return (err);
}

/***********************************************************************
 *
 *	FUNCTION:		SndInterruptSmfIrregardless
 *
 *	DESCRIPTION:	This function will interrupt any currently playing smf file
 *					regardless of whether the interruptible flag was set.  It is
 *					similar to the above SndPlaySmfXXXIrregardless functions
 *					except that it doesn't play a sound after interrupting the
 *					current sound.
 *
 *	PARAMETERS:		None
 *
 *	RETURNED:		non-zero on error.
 *
 *	REVISION HISTORY:
 *		Name		Date			Description
 *		----		----			-----------
 *		rkr		9/22/00		Added support for interrupting sounds regardless of the interruptible flag
 *
 ***********************************************************************/
Err SndInterruptSmfIrregardless(void)
{
	Err				err = errNone;
    SndGlobalsPtr   gP = (SndGlobalsPtr)GSndGlobalsP;
    
    // If a sound is currently playing then set the interruptSmfIrregardless
    // to true so that the currently playing sound is interrupted.
    if (gP->isSmfPlaying)
   		gP->interruptSmfIrregardless = true;
    
    return (err);
}
