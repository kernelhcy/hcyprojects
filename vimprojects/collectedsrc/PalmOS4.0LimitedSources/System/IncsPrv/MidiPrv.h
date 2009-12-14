/******************************************************************************
 *
 * Copyright (c) 1997-1999 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: MidiPrv.h
 *
 * Release: 
 *
 * Description:
 * 	            	Private MIDI and SMF (Standard MIDI File) implementation declarations
 *
 * History:
 *		August 1, 1997	Created by Vitaly M. Kruglikov
 *		Name	Date			Description
 *		----	----			-----------
 *		vmk	8/1/1997		Created
 *
 *****************************************************************************/

#ifndef __MIDIPRV_H__
#define __MIDIPRV_H__

#include <PalmTypes.h>

// SMF chunk types (ASCII)
#define midiSmfHeaderChunkType	'MThd'		// SMF Header Chunk
#define midiSmfTrackChunkType		'MTrk'		// SMF Track Chunk


// SMF file formats
#define	midiSmfFmt0				(0)
#define	midiSmfFmt1				(1)
#define	midiSmfFmt2				(2)
#define	midiSmfFmtUnknown		(0xFFFF)

#define midiSmfMaxValidFmt		midiSmfFmt2


// SMPTE time code division formats
#define midiSmfNegSMPTEFmt24				(-24)
#define midiSmfNegSMPTEFmt25				(-25)
#define midiSmfNegSMPTEFmt30Drop			(-29)
#define midiSmfNegSMPTEFmt30NonDrop		(-30)


// MIDI Message Status values
#define midiMsgStatusNone		(0)				// no  status
#define midiMaxChanMsgStatus	(0xEF)			// maximum MIDI channel message status value
#define midiMsgBaseNoteOff		(0x080)			// note off
#define midiMsgBaseNoteOn		(0x090)			// note on
#define midiMsgBasePolyPress	(0x0A0)			// poliphonic key pressure (aftertouch)
#define midiMsgBaseCtlChange	(0x0B0)			// controller change
#define midiMsgBaseProgChange	(0x0C0)			// program change
#define midiMsgBaseChanPress	(0x0D0)			// channel pressure (aftertouch)
#define midiMsgBasePitchBend	(0x0E0)			// pitch bend change


// SMF event status values
#define midiSmfEvtStatusMeta		(0xFF)		// SMF Meta event
#define midiSmfEvtStatusSysExF0	(0xF0)		// SMF System Exclusive event beginning with 0xF0
#define midiSmfEvtStatusSysExF7	(0xF7)		// SMF System Exclusive event beginning with 0xF7


// SMF Meta event type values of interest
#define midiSmfMetaEvtTypeEOT		(0x2F)		// End Of Track (requred at the end of all tracks)
#define midiSmfMetaEvtTypeTempo	(0x51)		// Set Tempo


// MIDI and SMF masks and special bits
#define midiStatusBit			(0x80)			// this bit is set in event/message status bytes
#define midiChanNumMask			(0x0F)			// channel number mask for channel message status
#define midiChanMsgMask			(0xF0)			// channel message id mask for channel message status
#define midiSmfSMPTEDivBit		(0x8000)			// this bit is set in SMPTE Division values
#define midiSmfSMPTEDivTicksMask	(0x00FF)		// ticks per second value mask SMPTE Division values


// MIDI and SMF value bounds
#define midiMinChan				(0)				// minimum MIDI channel number
#define midiMaxChan				(15)				// maximum MIDI channel number

#define midiMinKey				(0)				// mininum note key #
#define midiMaxKey				(127)				// maximum note key #

#define midiMinVel				(0)				// minimum key velocity
#define midiMaxVel				(127)				// maximum key velocity


#define midiSmfMinHdrChunkDataSize	(6)		// minimum SMF header chunk data size
#define midiSmfMinTrkChunkDataSize	(4)		// minimum SMF track chunk data size (minimum
															// delta-time + "end of track" event)


// MIDI value defaults
#define midiDefTempo				(500000L)		// default tempo in microseconds per quarter note
#define midiDefVel				(64)				// default key velocity



// Lengths (in # of bytes) of SMF fixed-length integers
#define midiSmfHdrFmtBytes			(2)			// size of Format field in the SMF header chunk
#define midiSmfHdrTrkBytes			(2)			// size of "# of Tracks" field in the SMF header chunk
#define midiSmfHdrDivBytes			(2)			// size of "Division" field in the SMF header chunk
#define midiSmfChunkTypeBytes		(4)
#define midiSmfChunkLengthBytes	(4)
#define midiSmfTempoBytes			(3)


typedef struct MidiSmfHdrInfoType {
	UInt16	wFmt;							// one of midiSmfFmt... (default: midiSmfFmtUnknown)
	UInt16	wNumTracks;					// number of tracks in the MIDI file (default: 0)
	UInt16	wDivRaw;						// raw Standard MIDI File division value
	Boolean	bUsingSMPTE;				// true if SMPTE is being used (default: false)
	UInt8		reserved;
	UInt16	wTicksPerQN;				// # of ticks per quarter note for metrical time (default: 0)
	UInt32	dwTicksPerSec;				// # of ticks per second for SMPTE time code time (default: 0)
	} MidiSmfHdrInfoType;
	
	
typedef struct MidiStateType {
	MidiSmfHdrInfoType	hdr;			// digested SMF header info
	
	UInt8 *	streamP;						// pointer to SMF stream
	
	// Current SMF chunk info
	UInt8 *	chunkEndP;					// pointer to end of current SMF chunk (NULL when unknown)
	UInt32	dwChunkType;				// current SMF chunk type
	UInt32	dwChunkSize;				// current SMF chunk size
	
	// Event status info
	UInt8		bMsgStatus;					// current channel message running status value (0 = none)
	UInt8		reserved;
	UInt32	dwTempo;						// metrical time tempo in microseconds per quarter note
												// (default: 500,000)
	
	// Honored channel range
	UInt8		bFirstChan;					// first supported logical MIDI channel in the range 0-15
												// (default 0)
	UInt8		bLastChan;					// last supported logical MIDI channel in the range 0-15
												// (default 15)
												
	} MidiStateType;

	
	
typedef enum MidiSmfEvtClassEnum {
	eMidiSmfEvtClassUnknown = 0,
	eMidiSmfEvtClassChannel,
	eMidiSmfEvtClassSysEx,
	eMidiSmfEvtClassMeta
	} MidiSmfEvtClassEnum;


// SMF events
typedef enum MidiSmfEvtEnum {
	eMidiSmfEvtUnknown		= 0,			// unknown SMF event
	
	// SMF Channel events - these values must match
	// the base status values of MIDI channel messages
	eMidiSmfEvtNoteOff		= 0x080,		// note off
	eMidiSmfEvtNoteOn			= 0x090,		// note on
	eMidiSmfEvtPolyPress		= 0x0A0,		// poliphonic key pressure (aftertouch)
	eMidiSmfEvtCtlChange		= 0x0B0,		// controller change
	eMidiSmfEvtProgChange	= 0x0C0,		// program change
	eMidiSmfEvtChanPress		= 0x0D0,		// channel pressure (aftertouch)
	eMidiSmfEvtPitchBend		= 0x0E0,		// pitch bend change
	
	// SMF System Exclusive events
	eMidiSmfEvtSysEx1			= 0x0F0,		// SMF sys exclusive event beginning with 0xF0
	eMidiSmfEvtSysEx2			= 0x0F7,		// SMF sys exclusive event beginning with 0xF7
	
	// SMF Meta events
	eMidiSmfMetaEvtEOT		= 0xFF2F,	// end of track (required at the end of every track)
	eMidiSmfMetaEvtTempo		= 0xFF51,	// set tempo
	eMidiSmfMetaEvtOther		= 0xFF80		// other SMF meta events we're not presently interested in
	
	} MidiSmfEvtEnum;
	
	
typedef struct MidiEventType {
	MidiSmfEvtEnum			id;			// SMF event id
	MidiSmfEvtClassEnum	evtClass;	// SMF event class
	UInt8						reserved1;
	
	UInt32	dwDeltaTicks;				// number of PilotOS ticks before the event
	UInt8		bChan;						// channel number for channel events (0-15)
	UInt8		reserved2;
	
	// union of event data structures:
	union {
	
		// For the NoteOn/NoteOff events:
		struct {
			UInt8		bKey;						// MIDI note key # (0-127)
			UInt8		bVel;						// MIDI key velocity/amplitude (0-127)
			} note;
		
		// For the Tempo event:
		struct {
			UInt32	dwTempo;					// tempo in microseconds per quarter note
			} setTempo;
	
		} evtData;
	
	} MidiEventType;
	
	

#endif // __MIDIPRV_H__
