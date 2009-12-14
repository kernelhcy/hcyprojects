/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: FileStreamPrv.h
 *
 * Release: 
 *
 * Description:
 *		Pilot File Stream private equates
 *
 * History:
 *		12/2/97	vmk		- Created by Vitaly Kruglikov 
 *
 *****************************************************************************/

#ifndef __FILESTREAMPRV_H__
#define __FILESTREAMPRV_H__

#include <PalmTypes.h>
#include <DataMgr.h>

/************************************************************
 * Internal structures
 *************************************************************/

// File descriptor block structure
#define fileDescriptorSignature		('STRM')

#define fileStateOpenComplete		(0x8000)				// set when opening completes
#define fileStateWriteAccess		(0x4000)				// set if file was opened with write access
#define fileStateDestructiveRead	(0x2000)				// set if file is in "destructive read" mode
#define fileStateAppend				(0x1000)				// set if file is in "append" mode
#define fileStateCreated			(0x0800)				// set if the open call caused the file to be created
#define fileStateCompressed		(0x0400)				// set if the file is in a compressed state
#define fileStateIOError			(0x0001)				// set if I/O error occurred (cleared w/ clearerr or seek)
#define fileStateEOF					(0x0002)				// set if read attempted beyond end of file (cleared by clearerr or seek)


// For error-checking -- ***ADD ALL NEW STATES TO THIS DEFINE***
#define fileAllStates			(	fileStateOpenComplete		|	\
											fileStateWriteAccess			|	\
											fileStateDestructiveRead	|	\
											fileStateAppend				|	\
											fileStateCreated				|	\
											fileStateCompressed			|	\
											fileStateIOError				|	\
											fileStateEOF )

typedef struct FileCachedInfoType {
	MemHandle		blockH;					// cached block handle: set to null if none
	UInt32		blockIndex;				// block's record index in the database (0-based)
	Boolean		forWrite;				// true if got the block for write, false if for read-only
	UInt8			bReserved;
	} FileCachedInfoType;
	

typedef struct FileDescriptorType {
	UInt32		sig;					// signature (for debugging): set to fileDescriptorSignature
	UInt32		openMode;			// mode flags passed to FileOpen (fileMode...)
	UInt16		state;				// current file state flags (fileState...)
	DmOpenRef	dbR;					// pointer to open database (NULL if not open)
	Err			lastError;			// error value from last file call
	Int32			fileSize;			// cached file size (for perf. optimization)
	Int32			numBlocks;			// cached number of data blocks (records) (for perf. optimization)
	Int32			curOffset;			// current offset for reading and/or writing
	FileCachedInfoType	cached;	// cached block information
	UInt32		dwReserved1;		// reserved -- set to zero
	UInt32		dwReserved2;		// reserved -- set to zero
	} FileDescriptorType;

// File data block structure
#define fileBlockDataSize	(1024L * 4)	/* unit of stream storage */

#define fileDataBlockSignature		('DBLK')
#define fileDataBlockCategory			(dmUnfiledCategory)	/* record category used for data blocks */

typedef struct FileBlockHeaderType {
	UInt32		sig;						// signature (for debugging and validation): set to fileDataBlockSignature
	Int32	dataSize;				// size of valid data in the block (can be less than fileBlockDataSize)
	} FileBlockHeaderType;
	
typedef struct FileDataBlockType {
	FileBlockHeaderType	hdr;								// block header
	UInt8						data[fileBlockDataSize];	// data storage
	} FileDataBlockType;


/************************************************************
 * File Stream Private procedures
 *************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif


#endif //  #ifndef __FILESTREAMPRV_H__
