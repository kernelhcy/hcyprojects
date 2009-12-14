/******************************************************************************
 *
 * Copyright (c) 1994-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: FileStream.c
 *
 * Release: 
 *
 * Description:
 *		File stream implementation.  Each file is implemented as a record database
 *		with zero or more iniformely sized records.
 *
 *		DOLATER... this implementation could use some optimization:	vmk	12/1/97
 *
 * History:
 *   	11/24/97	vmk		- Created by Vitaly Kruglikov
 *		12/15/97	vmk		- Performed first stage of optimization
 *		12/19/97	vmk		- Performed record block MemHandle-caching optimization
 *		1/10/98	vmk		- Implemented compression/uncompression of unused space at the end of file
 *
 *****************************************************************************/

#include <PalmTypes.h>

#include <DataMgr.h>
#include <ErrorMgr.h>
#include <MemoryMgr.h>
#include <SystemMgr.h>


// Our own includes
#include "FileStream.h"
#include "FileStreamPrv.h"


/********************************************************************
 * Internal macros
 ********************************************************************/

// FILESTREAM_RIGOROUS_ERROR_CHECK: set to 1 to enable very regourous
// error checking (VERY SLOW)
#ifndef FILESTREAM_RIGOROUS_ERROR_CHECK
	#define FILESTREAM_RIGOROUS_ERROR_CHECK	0
	//#define FILESTREAM_RIGOROUS_ERROR_CHECK	1
#endif

#if FILESTREAM_RIGOROUS_ERROR_CHECK == 1
	#pragma FILESTREAM_RIGOROUS_ERROR_CHECK_IS_ON_THINGS_WILL_BE_VERY_SLOW
#endif

#if (FILESTREAM_RIGOROUS_ERROR_CHECK == 1) && (ERROR_CHECK_LEVEL != ERROR_CHECK_FULL)
	ERROR: RIGOUROUS ERROR CHECK LEVEL MUST BE OFF ON NON-EC BUILDS
#endif


#define prvMemberOffset(structType, memberName)	\
	( (UInt16)(&((structType*)0)->memberName) )

#define prvMemberSize(structType, memberName)	\
	( sizeof((structType*)0)->memberName )

#define prvSizeIncludingMember(structType, memberName)	\
	( prvMemberOffset(structType, memberName) +				\
	prvMemberSize(structType, memberName) )

#define prvFilePosToBlockIndex(filePos, blkIndexP, blkDataOffsetP)	\
	do	{																					\
		*blkIndexP = filePos / fileBlockDataSize;								\
		*blkDataOffsetP = filePos % fileBlockDataSize;						\
		} while (false)


//
// Macros for locking and unlocking data blocks
//
#define prvLockDataBlockReadOnly(fdP, blockIndex)				\
	PrvLockDataBlock((fdP), (blockIndex), false/*forWrite*/, false/*uncompress*/)

#define prvLockDataBlockReadWrite(fdP, blockIndex, uncompress)		\
	PrvLockDataBlock((fdP), (blockIndex), true/*forWrite*/, (uncompress))

#define prvUnlockDataBlockReadOnly(fdP, blkP, blockIndex)	\
	PrvUnlockDataBlock((fdP), (blkP), (blockIndex), false/*dirty*/, false/*forWrite*/)

#define prvUnlockDataBlockReadWrite(fdP, blkP, blockIndex)	\
	PrvUnlockDataBlock((fdP), (blkP), (blockIndex), true/*dirty*/, true/*forWrite*/)


//
// Macros for dealing with compression/uncompression
//
#define prvIsUncompressionNeeded(fdP, blockIndex)		\
		( ((fdP)->state & fileStateCompressed) && (blockIndex) == ((fdP)->numBlocks - 1) )


//
// Macros for block MemHandle caching support
//
#define prvSetCached(fdP, __blkH__, __blockIndex__, __forWrite__)			\
	do {																						\
		ErrNonFatalDisplayIf((fdP)->cached.blockH, "cache not flushed");	\
		ErrNonFatalDisplayIf(															\
			DmQueryRecord((fdP)->dbR, (__blockIndex__)) != (__blkH__), 		\
			"block index/MemHandle mismatch");											\
		(fdP)->cached.blockH = (__blkH__);											\
		(fdP)->cached.blockIndex = (__blockIndex__);								\
		(fdP)->cached.forWrite = (__forWrite__);									\
		} while (false)


#define prvFlushCached(fdP)																		\
	do {																									\
		ErrNonFatalDisplayIf((fdP)->cached.blockH &&											\
			DmQueryRecord((fdP)->dbR, (fdP)->cached.blockIndex) !=						\
			(fdP)->cached.blockH, "cached block index/MemHandle mismatch");				\
		if ( (fdP)->cached.blockH && (fdP)->cached.forWrite )								\
			{																								\
			DmReleaseRecord((fdP)->dbR, (fdP)->cached.blockIndex, true/*dirty*/);	\
			}																								\
		(fdP)->cached.blockH = NULL;		/* clear cache */									\
		} while (false)
		


#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL

	#define ECValidateFileDescriptor(__fdP__)			PrvValidateFileDescriptor(__fdP__)

	#define ECValidateDataBlock(__fdP__, __blkP__)	PrvIsValidDataBlock(__fdP__, __blkP__)
	
	#define ECValidateFileStream(__fdP__)				PrvValidateFileSizeInfo(__fdP__)
	
#else

	#define ECValidateFileDescriptor(__fdP__)
	
	#define ECValidateDataBlock(__fdP__, __blkP__)			
	
	#define ECValidateFileStream(__fdP__)

#endif	


/********************************************************************
 * Internal function prototypes
 ********************************************************************/

static Err		PrvLockBlockByPos(FileDescriptorType* fdP, Int32 pos, FileDataBlockType** blkPP,
						Int32 * blockIndexP, Int32 * dataOffsetP, Boolean forWrite);
static Err		PrvCalcFileSizeInfo(FileDescriptorType* fdP);
static Err		PrvValidateFileSizeInfo(FileDescriptorType* fdP);
static FileDescriptorType* PrvLockFileDescriptor(FileHand stream, Err* errP);
static void		PrvUnlockFileDescriptor(FileDescriptorType* fdP, Err* lastErrorP);
static Err		PrvValidateFileDescriptor(FileDescriptorType* fdP);
static FileDataBlockType* PrvLockDataBlock(FileDescriptorType* fdP, Int32 blockIndex,
						Boolean forWrite, Boolean uncompress);
static void		PrvUnlockDataBlock(FileDescriptorType* fdP, FileDataBlockType* blkP,
						Int32 blockIndex, Boolean dirty, Boolean forWrite);
static Boolean	PrvIsValidDataBlock(FileDescriptorType* fdP, FileDataBlockType* blkP);
static Err		PrvSwitchToDestructiveRead(FileDescriptorType* fdP, FileHand stream);
static Err		PrvExtendFile(FileDescriptorType* fdP, Int32 pos, Boolean asMuchAsPossible);
static Err		PrvAddDataBlock(FileDescriptorType* fdP, Int32 dataSize);
static Err		PrvSetDataBlockSize(FileDescriptorType* fdP, Int32 blkIndex,
						Int32 newSize, Int32 * oldSizeP, Boolean okToUncompress);
static Err		PrvGetDataBlockSize(FileDescriptorType* fdP, Int32 blkIndex, Int32 * sizeP, UInt32 * recSizeP);
static Err		PrvTruncateFile(FileDescriptorType* fdP, Int32 newSize);
static void		PrvCompressFile(FileDescriptorType* fdP);
static Err		PrvUncompressFile(FileDescriptorType* fdP);



/***********************************************************************
 *
 * FUNCTION:		FileOpen
 *
 * DESCRIPTION:	Open a file stream (creating one if requested)
 *
 * PARAMETERS:	cardNo		-- memory card number
 *					nameP			-- file name (must be non-null and non-empty)
 *					type			-- file type; may be zero for wildcard, in which case sysFileTFileStream
 *										will be used if the file needs to be created and fileModeTemporary is
 *										not specified; if type is zero and fileModeTemporary is specified, then
 *										sysFileTTemp will be used for the file's type if it needs to be created
 *					creator		-- file creator;  may be zero for wildcard, in which case the current app's
 *										creator id will be used if the file needs to be created
 *					openMode		-- file open mode (fileMode...)
 *					errP			-- ptr to variable for returning the error code (fileErr...)
 *										(OPTIONAL -- pass NULL to ignore)
 *
 * RETURNED:	file MemHandle on success; zero on failure
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	11/24/97	Initial version
 *
 ***********************************************************************/
FileHand FileOpen(UInt16 cardNo, const Char * nameP, UInt32 type, UInt32 creator, UInt32 openMode,
	Err* errP)
{
	Err			err = 0;
	Err			dbgErr;
	UInt16			dbOpenMode;
	Int16			numFlags;
	LocalID		dbId = 0;
	UInt16			dbAttrs;
	DmOpenRef	dbR = NULL;
	UInt32			actType, actCreator;		// actual type and creator
	FileDescriptorType*	fdP = NULL;
	LocalID		curAppDB = 0;
	UInt16			curAppCardNo = 0;
	Boolean		okToCreate = false;
	Boolean		discardPrevious = false;
	Boolean		writeAccess = false;
	Boolean		created = false;


	//
	// Perform some basic error checking
	//
	if ( !nameP || !*nameP )
		{
		ErrNonFatalDisplay("null file name passed");
		err = fileErrInvalidParam;
		goto Exit;
		}
			
	if ( openMode & ~fileModeAllFlags )
		{
		ErrNonFatalDisplay("invalid open mode flags");
		err = fileErrInvalidParam;
		goto Exit;
		}

	//
	// Process the open mode flags
	//	
	
	// Process mutually-exclusive open modes
	numFlags = 0;								// for checking mutually-exclusive flag use
	if ( openMode & fileModeReadOnly )
		{
		dbOpenMode = dmModeReadOnly;
		numFlags++;
		}
		
	if ( openMode & fileModeReadWrite )
		{
		dbOpenMode = dmModeReadWrite;
		okToCreate = discardPrevious = writeAccess = true;
		numFlags++;
		}
		
	if ( openMode & fileModeUpdate )
		{
		dbOpenMode = dmModeReadWrite;
		okToCreate = writeAccess = true;
		numFlags++;
		}
		
	if ( openMode & fileModeAppend )
		{
		dbOpenMode = dmModeReadWrite;
		okToCreate = writeAccess = true;
		numFlags++;
		}
	
	// Exactly one of these mode flags is required
	if ( numFlags != 1 )
		{
		ErrNonFatalDisplay("invalid open mode");
		err = fileErrInvalidParam;
		goto Exit;
		}
	
	
	// MemHandle other mode flags
	if ( openMode & fileModeLeaveOpen )
		{
		dbOpenMode |= dmModeLeaveOpen;
		}
	if ( openMode & fileModeExclusive )
		{
		dbOpenMode |= dmModeExclusive;
		}
		
	
	//
	// Allocate and init the file descriptor block
	//
	fdP = (FileDescriptorType*)MemPtrNew(sizeof(FileDescriptorType));
	if ( !fdP )
		{
		err = fileErrMemError;
		goto Exit;
		}
	
	// If opening in "leave open" mode, set the descriptor's chunk owner
	// to "system" so it will not be automatically deleted by the system
	// when the current app quits
	if ( openMode & fileModeLeaveOpen )
		MemPtrSetOwner(fdP, 0);
		
	MemSet(fdP, sizeof(*fdP), 0);
	
	
	//
	// Look for an existing file by name
	//
	
	dbId = DmFindDatabase(cardNo, nameP);
	
	if ( dbId )
		{
		dbgErr = DmDatabaseInfo(cardNo, dbId, NULL, &dbAttrs, NULL, NULL,
					NULL, NULL, NULL, NULL, NULL, &actType, &actCreator);
		ErrNonFatalDisplayIf(dbgErr, "DmDatabaseInfo failed");		// should not happen
			
		// If using existing file, make sure it is a stream
		if ( !discardPrevious )
			{
			if ( !(dbAttrs & dmHdrAttrStream) )
				{
				err = fileErrNotStream;
				goto Exit;
				}
			}
		
		// Validate type/creator
		if ( !(openMode & fileModeAnyTypeCreator) )
			{
			if ( (type && actType != type) || (creator && actCreator != creator) )
				{
				err = fileErrTypeCreatorMismatch;
				goto Exit;
				}
			}
		
		// Discard the existing file, if requested
		if ( discardPrevious )
			{
			// If overwrites are allowed, delete the file
			err = fileErrReplaceError;										// assume error
			if ( !(openMode & fileModeDontOverwrite) )
				err = DmDeleteDatabase(cardNo, dbId);
			if ( err )
				{
				if (err == dmErrDatabaseOpen)
					err = fileErrInUse;
				else
					err = fileErrReplaceError;
				goto Exit;
				}
			dbId = 0;															// so a new one may be created
			}
		} // if ( dbId )
	
	// Create a new file if necessary
	if ( !dbId && okToCreate )
		{
		// Fix up type and creator if either is zero
		if ( type == 0 )
			{
			if ( openMode & fileModeTemporary )
				type = sysFileTTemp;											// temporary file stream db type (system may auto-cleanup after crash in future versions)
			else
				type = sysFileTFileStream;									// default file stream db type
			}
		if ( creator == 0 )
			{																		// get creator from current app
			// Get current app database ID and card #
			err = SysCurAppDatabase(&curAppCardNo, &curAppDB);
			#if EMULATION_LEVEL == EMULATION_NONE
				ErrNonFatalDisplayIf(err, "SysCurAppDatabase failed");
			#endif
			
			// Get app's creator id
			if ( err || !curAppDB)
				{
				// SysCurAppDatabase is presently not supported in the Simulator environment
				
				#if EMULATION_LEVEL != EMULATION_NONE
					creator = '????';
					err = 0;
				#endif
				}
			else
				err = DmDatabaseInfo(curAppCardNo, curAppDB, NULL, NULL, NULL, NULL,
					NULL, NULL, NULL, NULL, NULL, NULL, &creator);
			
			if ( err || creator == 0 )
				{
				err = fileErrCreateError;
				goto Exit;
				}
			}
		
		err = DmCreateDatabase(cardNo, nameP, creator, type, false);
		if ( err )
			{
			err = fileErrCreateError;
			goto Exit;
			}
		
		// Get the file's local ID
		dbId = DmFindDatabase(cardNo, nameP);
		ErrNonFatalDisplayIf(!dbId, "created db not found");		// should not happen
		
		// Set the "stream" flag in the db header
		dbgErr = DmDatabaseInfo(cardNo, dbId, NULL, &dbAttrs, NULL, NULL, NULL, NULL, NULL, NULL,
			NULL, NULL, NULL);
		dbAttrs |= dmHdrAttrStream;
		dbgErr = DmSetDatabaseInfo(cardNo, dbId, NULL, &dbAttrs, NULL, NULL, NULL, NULL, NULL, NULL,
			NULL, NULL, NULL);
			
		created = true;
		}
	
	// Check if we have a database to work with
	if ( !dbId )
		{
		err = fileErrNotFound;
		goto Exit;
		}
	
	// Open the file
	dbR = DmOpenDatabase(cardNo, dbId, dbOpenMode);
	if ( !dbR )
		{
		err = DmGetLastErr();
		ErrNonFatalDisplayIf(!err, "unexpected error code");
		if ( err == dmErrMemError )
			err = fileErrMemError;
		else
		if ( err == dmErrDatabaseOpen || err == dmErrAlreadyOpenForWrites || err == dmErrOpenedByAnotherTask )
			err = fileErrInUse;
		else
		if ( err == dmErrReadOnly )
			err = fileErrReadOnly;
		else
			err = fileErrOpenError;

		goto Exit;
		}
	
	// Init the file descriptor
	fdP->sig = fileDescriptorSignature;
	fdP->openMode = openMode;
	if ( writeAccess )
		fdP->state |= fileStateWriteAccess;
	if ( openMode & fileModeAppend )
		fdP->state |= fileStateAppend;
	if ( created )
		fdP->state |= fileStateCreated;
	else
		fdP->state |= fileStateCompressed;				// existing files are always compressed
		
	fdP->dbR = dbR;
	
	#if 0		// CASTRATION -- will uncompress only when and if needed to prevent causing unnecessary backups (because uncompressing marks the db modified)
		// Uncompress writeable files before using
		if ( fdP->state & fileStateWriteAccess )
			{
			err = PrvUncompressFile(fdP);
			// If uncompression failed, abort the open operation
			if ( err )
				goto Exit;
			}
	#endif
	
	err = PrvCalcFileSizeInfo(fdP);
	if ( !err )
		fdP->state |= fileStateOpenComplete;

Exit:
	// Unlock file descriptor structure
	if ( !err )
		PrvUnlockFileDescriptor(fdP, &err);
	
	// Clean up on error
	else
		{
		if ( fdP )
		{
			MemPtrFree(fdP);
			fdP = NULL;		// Don't return a dangling pointer
		}
		if ( dbR )
			DmCloseDatabase(dbR);
		}
		
	if ( errP )										// set up optional return value
		*errP = err;
		
	//return (FileHand)fdH;
	return (FileHand)fdP;
}


/***********************************************************************
 *
 * FUNCTION:		FileClose
 *
 * DESCRIPTION:	Close the file and destory the file MemHandle
 *
 * PARAMETERS:	stream		-- MemHandle of open file
 *
 * RETURNED:	zero on success, fileErr... on error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	11/25/97	Initial version
 *			vmk	12/19/97	Added block MemHandle-caching optimization
 *
 ***********************************************************************/
Err FileClose(FileHand stream)
{
	Err			err = 0;
	Err			dbgErr;
	FileDescriptorType*	fdP;
	LocalID		delDbID = 0;
	UInt16			delDbCardNo = 0;
	
	
	// Lock down the file descriptor
	fdP = PrvLockFileDescriptor(stream, &err);
	if ( err )
		return err;
	ErrNonFatalDisplayIf(!fdP, "null file descriptor ptr");
	
	// Validate the file
	ECValidateFileStream(fdP);
	
	// Compress writeable files before closing
	if ( fdP->state & fileStateWriteAccess )
		PrvCompressFile(fdP);
	
	prvFlushCached(fdP);
	
	// Check if this is a temporary file, and if so, get its card number and database id
	if ( fdP->openMode & fileModeTemporary )
		{
		dbgErr = DmOpenDatabaseInfo(fdP->dbR, &delDbID, NULL, NULL, &delDbCardNo, NULL);
		ErrNonFatalDisplayIf(dbgErr, "DmOpenDatabaseInfo failed");
		}
	
	// Close the database
	err = DmCloseDatabase(fdP->dbR);
	ErrNonFatalDisplayIf(err, "DmCloseDatabase failed");
	if ( err )
		{
		err = fileErrCloseError;
		PrvUnlockFileDescriptor(fdP, &err);
		return err;
		}
	
	// If this was a temporary file, delete it
	if ( delDbID )
		{
		dbgErr = DmDeleteDatabase(delDbCardNo, delDbID);
		}
	
	// Destroy the file MemHandle
	MemPtrFree(fdP);
	
	return 0;
}


/***********************************************************************
 *
 * FUNCTION:		FileDelete
 *
 * DESCRIPTION:	Delete a file.  The file must already be closed.
 *
 * PARAMETERS:	cardNo		-- memory card number
 *					nameP			-- file name
 *
 * RETURNED:	zero on success, fileErr... on error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	11/26/97	Initial version
 *
 ***********************************************************************/
Err FileDelete(UInt16 cardNo, const Char * nameP)
{
	Err	err = 0;
	LocalID	dbId;
	
	// Find the file
	dbId = DmFindDatabase(cardNo, nameP);
	if ( dbId )
		{
		err = DmDeleteDatabase(cardNo, dbId);
		switch( err )
			{
			case 0:
				break;
			case dmErrDatabaseOpen:
			case dmErrDatabaseProtected:
				err = fileErrInUse;
				break;
			default:
				err = fileErrIOError;
				break;
			}
		}
	else
		err = fileErrNotFound;

	return err;
}


/***********************************************************************
 *
 * FUNCTION:		FileReadLow
 *
 * DESCRIPTION:	A low-level routine to read data from a file into a buffer or
 *						a data storage heap-based chunk (record or resource).  Users are expected
 *						to use helper macros FileRead and FileDmRead instead of calling this
 *						function directly.
 *
 * PARAMETERS:	stream		-- MemHandle of open file
 *					baseP			-- ptr to beginning of destination buffer for reading data (must be
 *										beginning of record or resource for data storage heap-based destination)
 *					offset		-- offset from base ptr to the destination area (must be >= 0)
 *					dataStoreBased	-- if true, Data Manager routine(s) will be used to copy data to the
 *											destination
 *					objSize		-- size of each object to read
 *					numObj		-- number of objects to read
 *					errP			-- ptr to variable for returning the error code (fileErr...)
 *										(OPTIONAL -- pass NULL to ignore)
 *
 * RETURNED:	the number of objects that were read - this may be less than
 *					the number of objects requested
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	11/25/97	Initial version
 *			vmk	12/23/97	Implement destructive read mode
 *			vmk	1/7/98	Renamed (was FileRead) and added data storage destination capability
 *
 ***********************************************************************/
Int32 FileReadLow(FileHand stream, void * baseP, Int32 offset, Boolean dataStoreBased, Int32 objSize,
	Int32 numObj, Err* errP)
{
	Err		err = 0;
	FileDescriptorType*	fdP = NULL;
	Int32		objRead = 0;
	Int32		bytesAvail;
	Int32		bytesToRead;
	Int32		bytesLeft;
	FileDataBlockType*	blkP;
	Int32		dataSize;
	Int32		dataOffset;
	Int32		chunkSize;
	Int32		blkIndex;
	UInt8 *	bP = (UInt8 *)baseP + offset;
	
	
	ErrNonFatalDisplayIf(offset < 0, "negative offset not allowed");
	
	// Lock down the file descriptor
	fdP = PrvLockFileDescriptor(stream, &err);
	if ( err )
		goto Exit;
	ErrNonFatalDisplayIf(!fdP, "null file descriptor ptr");
	
	// Validate parameters
	ErrNonFatalDisplayIf(numObj < 0, "invalid object count");
	ErrNonFatalDisplayIf(objSize < 0, "invalid object size");
	if ( numObj < 0 || objSize < 0 )
		{
		err = fileErrInvalidParam;
		goto Exit;
		}
	

	ErrFatalDisplayIf(!baseP, "null baseP passed");
	
	// Calculate number of bytes to read
	bytesToRead = numObj * objSize;
	if ( bytesToRead == 0 )
		{
		// Bail out if there is nothing to do
		err = 0;
		goto Exit;
		}
	bytesAvail = fdP->fileSize - fdP->curOffset;
	if ( bytesToRead > bytesAvail )
		bytesToRead = bytesAvail;
	ErrNonFatalDisplayIf(bytesToRead < 0, "bytesToRead overflow");
	
	// Read the data
	err = 0;
	bytesLeft = bytesToRead;
	while ( !err && bytesLeft > 0 )
		{
		// Lock the data block
		err = PrvLockBlockByPos(fdP, fdP->curOffset, &blkP, &blkIndex, &dataOffset, false/*forWrite*/);
		ErrNonFatalDisplayIf(err || !blkP, "PrvLockBlockByPos for read failed");
		if ( !err )
			{
			// For destructive reads, the block index must always remain at zero
			ErrNonFatalDisplayIf((fdP->state & fileStateDestructiveRead) && blkIndex != 0,
				"invalid destructive read block index");
				
			// Adjust read size for the current data block
			dataSize = blkP->hdr.dataSize;
			chunkSize = dataSize - dataOffset;		// # of bytes available at current pos
			ErrNonFatalDisplayIf(chunkSize < 0, "chunk size underflow");
			ErrNonFatalDisplayIf(chunkSize > fileBlockDataSize, "chunk size overflow");
			
			if ( chunkSize > bytesLeft )
				chunkSize = bytesLeft;
				
			// Copy bytes from data block
			if ( dataStoreBased )
				DmWrite(baseP, bP - (UInt8 *)baseP, &blkP->data[dataOffset], (UInt32)chunkSize);
			else
				MemMove(bP, &blkP->data[dataOffset], (UInt32)chunkSize);
			
			// Update file descriptor and accumulators
			fdP->curOffset += chunkSize;
			bytesLeft -= chunkSize;
			bP += chunkSize;
			
			// Unlock the data block
			prvUnlockDataBlockReadOnly(fdP, blkP, blkIndex);
			
			// For destructive reads, delete blocks as we finish reading them
			if ( (fdP->state & fileStateDestructiveRead) && (dataOffset + chunkSize) == dataSize )
				{
				// Flush the cache
				prvFlushCached(fdP);
				
				// Permanently remove the data block
				err = DmRemoveRecord(fdP->dbR, (UInt16)blkIndex);
				if ( err )
					{
					ErrNonFatalDisplay("DmRemoveRecord failed");
					err = fileErrCorruptFile;
					break;
					}
					
				fdP->numBlocks--;									// update cached file info
				fdP->fileSize -= dataSize;
				fdP->curOffset = 0;
				ErrNonFatalDisplayIf(fdP->numBlocks < 0, "block count underflow");
				ErrNonFatalDisplayIf(fdP->fileSize < 0, "file size underflow");
				} // if ( (fdP->state & fileStateDestructiveRead) && ...)
				
			} // data lock succeeded
			
		} // while ( !err && bytesLeft > 0 )
	
	ErrNonFatalDisplayIf(bytesLeft < 0, "bytesLeft underflow");				// should not happen
	ErrNonFatalDisplayIf(bytesLeft > 0, "not all expected bytes read");	// should not happen
	
	// Compute the number of objects read
	objRead = (bytesToRead - bytesLeft) / objSize;
	
	// If number of objects read is less than requested, set error code
	if ( !err && objRead < numObj )
		{
		err = fileErrEOF;
		fdP->state |= fileStateEOF;
		}
		
	// FALL TRHOUGH TO Exit

Exit:
	// Update last error and unlock the file descriptor
	if ( fdP )
		PrvUnlockFileDescriptor(fdP, &err);
	
	if ( errP )										// set up optional return value
		*errP = err;
		
	return objRead;
}


/***********************************************************************
 *
 * FUNCTION:		FileWrite
 *
 * DESCRIPTION:	Write data to a file.  Writing to files opened without
 *						write access or those in destructive read mode
 *						is not allowed.
 *
 * PARAMETERS:	stream		-- MemHandle of open file
 *					dataP			-- buffer with data to be written
 *					objSize		-- size of each object to write (must be > 0)
 *					numObj		-- number of objects to write
 *					errP			-- ptr to variable for returning the error code (fileErr...)
 *										(OPTIONAL -- pass NULL to ignore)
 *
 * RETURNED:	the number of objects that were written - this may be less than
 *					the number of objects requested on error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	11/26/97	Initial version
 *			vmk	12/1/97	Extend file if needed before writing data
 *			vmk	12/15/97	Optimized so small writes will not trigger automatic
 *								file extension prior to writing the data if the entire
 *								write will fit into an existing data block
 *			vmk	12/23/97	Check for destructive read mode
 *
 ***********************************************************************/
Int32 FileWrite(FileHand stream, const void * dataP, Int32 objSize,
	Int32 numObj, Err* errP)
{
	Err		err = 0;
	FileDescriptorType*	fdP = NULL;
	Int32		bytesToWrite;
	Int32		bytesWritten;
	Int32		bytesLeft;
	Int32	newDataSize;
	Int32		bytesAdded;
	FileDataBlockType*	blkP;
	Int32		dataOffset;
	Int32		chunkSize;
	Int32		blkIndex;
	Int32		newFileSize;
	//Int32		savedFileSize;
	Int32		savedCurOffset;
	Int32		objWritten = 0;
	
	
	// Lock down the file descriptor
	fdP = PrvLockFileDescriptor(stream, &err);
	if ( err )
		goto Exit;
	ErrNonFatalDisplayIf(!fdP, "null file descriptor ptr");
	
	// Validate parameters
	ErrNonFatalDisplayIf(numObj < 0, "invalid object count");
	ErrNonFatalDisplayIf(objSize < 0, "invalid object size");

	// Bail out on invalid parameter(s)
	if ( numObj < 0 || objSize < 0 )
		{
		err = fileErrInvalidParam;
		goto Exit;
		}
	
			
	ErrFatalDisplayIf(!dataP, "null dataP passed");
	
	
	// Make sure the file was open with write access
	if ( !(fdP->state & fileStateWriteAccess) )
		{
		ErrNonFatalDisplay("attempt to write without write access");
		err = fileErrPermissionDenied;
		fdP->state |= fileStateIOError;
		goto Exit;
		}

	// Make sure the file is not in destructive read mode (ok together with append -- pseudo-pipe)
	if ( (fdP->state & (fileStateDestructiveRead | fileStateAppend)) == fileStateDestructiveRead )
		{
		ErrNonFatalDisplay("attempt to write a file in destructive read mode");
		err = fileErrPermissionDenied;
		fdP->state |= fileStateIOError;
		goto Exit;
		}
	
	// Compute the total number of bytes to write
	bytesToWrite = objSize * numObj;
	if ( bytesToWrite <= 0 )
		{
		if ( bytesToWrite == 0 )
			{
			// Bail out if there is nothing to write
			err = 0;
			goto Exit;
			}
		else
			{
			ErrNonFatalDisplay("bytes to write overflow");
			err = fileErrInvalidParam;
			goto Exit;
			}
		}
		
		
	// Save current offset (it needs to be restored if we're appending and the file
	// is in destructive read mode)
	savedCurOffset = fdP->curOffset;
	
	// If we're in append mode, reset current offset to end of file
	if ( fdP->state & fileStateAppend )
		fdP->curOffset = fdP->fileSize;

	
	// Check if the file needs to be extended to fit the new data, and extend it if so.
	// By extending before writing, we can ensure that we will not overwrite unintended
	// data in the event entire write size cannot be accomodated
	newFileSize = fdP->curOffset + bytesToWrite;					// possible new file size
	ErrNonFatalDisplayIf(newFileSize < 0, "new file size underflow");		// shouldn't happen
	if ( newFileSize > fdP->fileSize )
		{
		Boolean	needToExtend = true;									// assume extension is needed
		Boolean	mustUncompress;
		

		// Check if the data being written will fit inside an existing block (perf optimization)
		if ( bytesToWrite < fileBlockDataSize )
			{
			prvFilePosToBlockIndex(fdP->curOffset, &blkIndex, &dataOffset);
			mustUncompress = prvIsUncompressionNeeded(fdP, blkIndex);
			if ( !mustUncompress && blkIndex < fdP->numBlocks && (dataOffset + bytesToWrite) <= fileBlockDataSize )
				needToExtend = false;									// no need for extension
			}
		
		if ( needToExtend )
			{
			//savedFileSize = fdP->fileSize;							// save the filesize before extending
			err = PrvExtendFile(fdP, newFileSize, true);			// extend the file as much as possible
			
			// If couldn't extend to the desired size, adjust the number of bytes to be written, 
			// and truncate the file accordingly
			if ( err )
				{
				ErrNonFatalDisplayIf(fdP->fileSize >= newFileSize, "PrvExtendFile lied");

				// Recalculate the number of bytes which may be written
				bytesToWrite = fdP->fileSize - fdP->curOffset;
				
				#if 0		// CASTRATION
					newFileSize = fdP->curOffset + bytesToWrite;		// possible new filesize
					
					// If the data does not exceed the original file size, truncate back to
					// the original size, otherwise truncate to end with the data we're about to write
					if ( newFileSize < savedFileSize )
						newFileSize = savedFileSize;
					err = PrvTruncateFile(fdP, newFileSize);
					ErrNonFatalDisplayIf(err, "error restoring file size");
				#endif
				}
			} // if ( needToExtend )
		} // if ( newFileSize > fdP->fileSize )
	
	// Start writing -- the file contains enough blocks to accomodate "bytesToWrite" number of bytes
	bytesWritten = 0;
	err = 0;
	while ( !err && bytesWritten < bytesToWrite )
		{
		// Lock the data block
		err = PrvLockBlockByPos(fdP, fdP->curOffset, &blkP, &blkIndex, &dataOffset, true/*forWrite*/);
		ErrNonFatalDisplayIf(err || !blkP, "PrvLockBlockByPos for write failed");
		if ( !err )
			{
			// Adjust write size for the current data block
			chunkSize = fileBlockDataSize - dataOffset;	// # of bytes available at current pos
			ErrNonFatalDisplayIf(chunkSize < 0, "chunk size underflow");
			ErrNonFatalDisplayIf(chunkSize > fileBlockDataSize, "chunk size overflow");
			
			bytesLeft = bytesToWrite - bytesWritten;
			if ( chunkSize > bytesLeft )
				chunkSize = bytesLeft;
				
			// Copy bytes to data block
			
			DmWrite(blkP, prvMemberOffset(FileDataBlockType, data) + dataOffset,
				((UInt8*)dataP) + bytesWritten, (UInt32)chunkSize);
			
			// Update block's data size
			bytesAdded = 0;
			newDataSize = dataOffset + chunkSize;
			ErrNonFatalDisplayIf(newDataSize > fileBlockDataSize, "newDataSize too big");
			if ( newDataSize > blkP->hdr.dataSize )
				{
				bytesAdded = newDataSize - blkP->hdr.dataSize;
				DmWrite(blkP, prvMemberOffset(FileDataBlockType, hdr.dataSize),
					&newDataSize, sizeof(blkP->hdr.dataSize));			
				}

			// Update file descriptor and accumulators
			fdP->curOffset += chunkSize;
			fdP->fileSize += bytesAdded;
			bytesWritten += chunkSize;

			ErrNonFatalDisplayIf(fdP->curOffset > fdP->fileSize, "curOffset overflow");
			
			// Unlock the data block
			prvUnlockDataBlockReadWrite(fdP, blkP, blkIndex);
			
			// Validate file
			#if (FILESTREAM_RIGOROUS_ERROR_CHECK == 1)
				ECValidateFileStream(fdP);
			#endif
			}
		}
	
	ErrNonFatalDisplayIf(bytesWritten > bytesToWrite, "too much written");		// should not happen
	ErrNonFatalDisplayIf(bytesWritten < bytesToWrite, "not enough written");	// should not happen
	
	// Restore current position if we were appending a file in destructive read mode
	if ( (fdP->state & (fileStateDestructiveRead | fileStateAppend)) ==
		(fileStateDestructiveRead | fileStateAppend) )
		{
		fdP->curOffset = savedCurOffset;
		}

		
	objWritten = bytesWritten / objSize;

	// Fix up the error code
	if ( objWritten < numObj && !err )
		{
		err = fileErrMemError;
		fdP->state |= fileStateIOError;
		}

Exit:
	// Update last error and unlock the file descriptor
	if ( fdP )
		PrvUnlockFileDescriptor(fdP, &err);
	
	if ( errP )										// set up optional return value
		*errP = err;
		
	return objWritten;
}


/***********************************************************************
 *
 * FUNCTION:		FileSeek
 *
 * DESCRIPTION:	Set current position for stream.  Not allowed on files
 *						in destructive read mode.
 *
 * PARAMETERS:	stream		-- MemHandle of open file
 *					offset		-- number of bytes from origin
 *					origin		-- origin of position change (beginning, current, or end)
 *
 * RETURNED:	zero on success, fileErr... on error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	11/26/97	Initial version
 *			vmk	12/23/97	Check for destructive read mode
 *
 ***********************************************************************/
Err FileSeek(FileHand stream, Int32 offset, FileOriginEnum origin)
{
	Err		err = 0;
	FileDescriptorType*	fdP = NULL;
	Int32		start;
	Int32		newOffset;
	
	
	// Lock down the file descriptor
	fdP = PrvLockFileDescriptor(stream, &err);
	if ( err )
		goto Exit;
	ErrNonFatalDisplayIf(!fdP, "null file descriptor ptr");

	// Make sure the file is not in destructive read mode
	if ( fdP->state & fileStateDestructiveRead )
		{
		ErrNonFatalDisplay("attempt to seek a file in destructive read mode");
		err = fileErrPermissionDenied;
		fdP->state |= fileStateIOError;
		goto Exit;
		}
	
	// Establish starting position from the "origin" arg
	switch( origin )
		{
		case fileOriginBeginning:
			start = 0;
			break;
		case fileOriginCurrent:
			start = fdP->curOffset;
			break;
		case fileOriginEnd:
			start = fdP->fileSize;
			break;
		default:
			ErrNonFatalDisplay("invalid origin");
			err = fileErrInvalidParam;
			break;
		}
		
	if ( err )
		goto Exit;
		
	// Calculate new file position
	newOffset = start + offset;
	
	// Validate the new file position
	
	// Negative not allowed
	if ( newOffset < 0 )
		{
		ErrNonFatalDisplay("position underflow");
		err = fileErrOutOfBounds;
		fdP->state |= (fileStateIOError);
		goto Exit;
		}
	
	// Position may not exceed file size if file is not writeable
	if ( newOffset > fdP->fileSize )
		{
		if ( (fdP->state & fileStateWriteAccess) )
			{
			// File is writeable - extend the file to accomodate the new position;
			// if the file can't be fully extended, will leave it at the maximum possible size
			err = PrvExtendFile(fdP, newOffset, true);
			if ( err )
				fdP->state |= fileStateIOError;
			}
		else
			{
			// File is not writeable and therefore may not be extended
			ErrNonFatalDisplay("position overflow");
			err = fileErrOutOfBounds;
			fdP->state |= fileStateIOError;
			}
		}
		
	if ( !err )
		{
		fdP->curOffset = newOffset;				// update current position
		fdP->state &= ~(fileStateEOF);			// clear end-of-file state
		}
	
	// FALL THROUGH TO Exit

Exit:
	// Unlock the file descriptor
	if ( fdP )
		PrvUnlockFileDescriptor(fdP, &err);
		
	return err;
}


/***********************************************************************
 *
 * FUNCTION:		FileTell
 *
 * DESCRIPTION:	Get current position within a file and, optionally, the filesize
 *
 * PARAMETERS:	stream		-- MemHandle of open file
 *					fileSizeP	-- ptr to variable for returning the filesize (-1L on error)
 *										(OPTIONAL -- pass NULL to ignore)
 
 *					errP			-- ptr to variable for returning the error code (fileErr...)
 *										(OPTIONAL -- pass NULL to ignore)
 *
 * RETURNED:	current file position for stream or -1L on error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	11/26/97	Initial version
 *
 ***********************************************************************/
Int32 FileTell(FileHand stream, Int32 * fileSizeP, Err* errP)
{
	Err		err = 0;
	FileDescriptorType*	fdP = NULL;
	Int32		pos;
	Int32		size;
	
	
	pos = size = -1L;						// init to signal error
	
	// Lock down the file descriptor
	fdP = PrvLockFileDescriptor(stream, &err);
	if ( err )
		goto Exit;
	ErrNonFatalDisplayIf(!fdP, "null file descriptor ptr");

	pos = fdP->curOffset;										// get the current position
	size = fdP->fileSize;
	
	// FALL THROUGH TO Exit

Exit:
	// Unlock the file descriptor
	if ( fdP )
		PrvUnlockFileDescriptor(fdP, &err);
	
	// Set up optional return variables
	if ( fileSizeP )
		*fileSizeP = size;
	
	if ( errP )
		*errP = err;
		
	return pos;
}


/***********************************************************************
 *
 * FUNCTION:		FileTruncate
 *
 * DESCRIPTION:	Truncate a file to a given size; not allowed on files
 *						in destructive read mode or files open in read-only mode
 *
 * PARAMETERS:	stream		-- MemHandle of open file
 *					newSize		-- new file size (must not exceed current file size)
 *
 * RETURNED:	zero on success, fileErr... on error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	11/26/97	Initial version
 *
 ***********************************************************************/
Err FileTruncate(FileHand stream, Int32 newSize)
{
	Err		err = 0;
	FileDescriptorType*	fdP = NULL;
		
	// Lock down the file descriptor
	fdP = PrvLockFileDescriptor(stream, &err);
	if ( err )
		goto Exit;
	ErrNonFatalDisplayIf(!fdP, "null file descriptor ptr");

	// Make sure the file was open with write access
	if ( !(fdP->state & fileStateWriteAccess) )
		{
		ErrNonFatalDisplay("attempt to truncate without write access");
		err = fileErrPermissionDenied;
		fdP->state |= fileStateIOError;
		goto Exit;
		}

	// Make sure the file is not in destructive read mode
	if ( fdP->state & fileStateDestructiveRead )
		{
		ErrNonFatalDisplay("attempt to truncate a file in destructive read mode");
		err = fileErrPermissionDenied;
		fdP->state |= fileStateIOError;
		goto Exit;
		}

	err = PrvTruncateFile(fdP, newSize);
	
	// FALL THROUGH TO Exit

Exit:
	// Unlock the file descriptor
	if ( fdP )
		PrvUnlockFileDescriptor(fdP, &err);
		
	return err;
}


/***********************************************************************
 *
 * FUNCTION:		FileControl
 *
 * DESCRIPTION:	Performs a file stream control operation;
 *
 * PARAMETERS:	op				-- operation to be performed
 *					stream		-- MemHandle of open file (if required for control operation)
 *					valueP		-- pointer to value or buffer (if required, NULL otherwise)
 *					valueLenP	-- pointer to length of value or buffer (if required, NULL otherwise)
 *
 * RETURNED:	zero on success, fileErr... on error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	12/23/97	Initial version
 *
 ***********************************************************************/
Err FileControl(FileOpEnum op, FileHand stream, void * valueP, Int32 * valueLenP)
{
	Err		savedErr = 0;		// this will be saved as last error in the file descriptor
	Err		retErr = 0;			// this will be the return value of this function
	FileDescriptorType*	fdP = NULL;
	
	// Lock down the file descriptor
	fdP = PrvLockFileDescriptor(stream, &retErr);
	if ( retErr )
		goto Exit;
	ErrNonFatalDisplayIf(!fdP, "null file descriptor ptr");
	
	
	switch ( op )
		{
		case fileOpDestructiveReadMode:
			retErr = savedErr = PrvSwitchToDestructiveRead(fdP, stream);
			break;
		
		case fileOpGetEOFStatus:
			retErr = (fdP->state & fileStateEOF) ? fileErrEOF : 0;
			break;
		
		case fileOpGetIOErrorStatus:
			retErr = (fdP->state & fileStateIOError) ? fileErrIOError : 0;
			break;
			
		case fileOpGetLastError:
			retErr = fdP->lastError;
			break;

		case fileOpGetCreatedStatus:
			if ( !valueP || !valueLenP || *valueLenP != sizeof(Boolean) )
				{
				retErr = savedErr = fileErrInvalidParam;
				ErrNonFatalDisplay("invalid arg(s) for fileOpGetCreatedStatus");
				}
			else
				*((Boolean *)valueP) = (fdP->state & fileStateCreated) ? true : false;
			break;

		case fileOpGetOpenDbRef:
			if ( !valueP || !valueLenP || *valueLenP != sizeof(DmOpenRef) )
				{
				retErr = savedErr = fileErrInvalidParam;
				ErrNonFatalDisplay("invalid arg(s) for fileOpGetOpenDbRef");
				}
			else
				*((DmOpenRef*)valueP) = fdP->dbR;
			break;
		
		case fileOpClearError:
			fdP->state &= ~(fileStateIOError | fileStateEOF);
			break;
		
		case fileOpFlush:
			prvFlushCached(fdP);
			break;
		
		default:
			retErr = savedErr = fileErrInvalidParam;
			ErrNonFatalDisplay("unknown FileOpEnum");
			break;
		}
	
Exit:
	// Unlock the file descriptor
	if ( fdP )
		PrvUnlockFileDescriptor(fdP, &savedErr);
	
	return retErr;
}


/***********************************************************************
 *
 * FUNCTION:		PrvSwitchToDestructiveRead
 *
 * DESCRIPTION:	Switch the file to "destructive read" mode.  The file must already
 *						be open as writeable.  Will rewind file to the beginning and mark
 *						the file descriptor for destructive read.
 *
 * PARAMETERS:	fdP			-- ptr to file descriptor with a valid DmOpenRef
 *					stream		-- MemHandle of open file
 *
 * RETURNED:	zero on success, fileErr... on error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	12/23/97	Initial version
 *
 ***********************************************************************/
static Err PrvSwitchToDestructiveRead(FileDescriptorType* fdP, FileHand stream)
{
	Err	err = 0;
	
	
	// Validate the file descriptor
	ECValidateFileDescriptor(fdP);


	// Make sure the file was open with write access
	if ( !(fdP->state & fileStateWriteAccess) )
		{
		ErrNonFatalDisplay("file is not writeable");
		fdP->state |= fileStateIOError;
		return fileErrPermissionDenied;
		}
	
	// Check if file is already in destructive read (it is probably a bug if
	// called more than once)
	if ( fdP->state & fileStateDestructiveRead )
		{
		ErrNonFatalDisplay("file is already in destructive read mode");
		return 0;
		}
	
	// Rewind the file
	err = FileSeek(stream, 0, fileOriginBeginning);
	if ( err )
		return err;
	
	// Set file state
	fdP->state |= fileStateDestructiveRead;
	
	return 0;
	
}


/***********************************************************************
 *
 * FUNCTION:		PrvExtendFile
 *
 * DESCRIPTION:	Extend the file size to accomodate the passed-in file position;
 *						If the position is inside the file already, does nothing;
 *						otherwise, extends file size such that the file size is equal to
 *						the passed-in file position and a data block exists to accomodate
 *						writing to that position;
 *						If extending fails, restores file size and position to that on entry
 *
 * PARAMETERS:	fdP					-- ptr to file descriptor with a valid DmOpenRef
 *					pos					-- absolute file position that belongs on the desired data block
 *					asMuchAsPossible	-- if true, leave at the maximum size that managed to extend
 *
 * RETURNED:	zero on success, fileErr... on error if couldn't extend to the full requested amount
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	11/26/97	Initial version
 *			vmk	12/1/97	Added asMuchAsPossible parameter
 *
 ***********************************************************************/
static Err PrvExtendFile(FileDescriptorType* fdP, Int32 pos, Boolean asMuchAsPossible)
{
	Err		err = 0;
	Int32		lastBlkIndex;
	Int32		lastBlkDataOffset;
	Int32		startBlkCnt;
	Int32		blkIndex;
	Int32		savedFileSize;
	Int32		savedFileOffset;
	Int32	newDataSize;
	
	
	ErrNonFatalDisplayIf(pos < 0, "invalid file pos");

	// Validate the file descriptor
	ECValidateFileDescriptor(fdP);

	ErrNonFatalDisplayIf(!(fdP->state & fileStateWriteAccess), "file is not writeable");
	
	// If position is within the file's data, we're done
	if ( pos < fdP->fileSize )
		return 0;
	
	// Convert file position to desired block index and data offset
	prvFilePosToBlockIndex(pos, &lastBlkIndex, &lastBlkDataOffset);
	
	// Optimization: if the desired block already exists and no file size
	// extension is needed, we're done
	if ( lastBlkIndex == (fdP->numBlocks - 1) && fdP->fileSize == pos )
		return 0;
	
	// Save original file size and position in case we need to restore
	savedFileSize = fdP->fileSize;
	savedFileOffset = fdP->curOffset;

	startBlkCnt = fdP->numBlocks;						// save original block count

	// Get index of last existing block
	if ( startBlkCnt )
		blkIndex = startBlkCnt-1;
	else
		blkIndex = 0;	// fix up block index to MemHandle empty file case
	
	// Add new blocks as needed and fix up block data sizes
	for (/*blank*/; !err && blkIndex <= lastBlkIndex; blkIndex++)
		{
		// Compute new data size for current block
		if ( blkIndex == lastBlkIndex || lastBlkIndex == 0 )
			newDataSize = lastBlkDataOffset;			// last block
		else
			newDataSize = fileBlockDataSize;			// inside block
		
		// Add the block if needed
		if ( blkIndex >= startBlkCnt )
			{
			err = PrvAddDataBlock(fdP, newDataSize);
			}
		
		// otherwise, modify the size of an existing block
		else
			{
			Int32	oldSize;
			err = PrvSetDataBlockSize(fdP, blkIndex, newDataSize, &oldSize, true/*okToUncompress*/);
			if ( !err )
				{
				ErrNonFatalDisplayIf(newDataSize < oldSize, "unexpected new data size");
				// Update file descriptor's file size and block's data size
				fdP->fileSize += (newDataSize - oldSize);		// update size info
				}
			}
		}
	
	// Restore original filesize and position on error, preserving the error code
	if ( err && !asMuchAsPossible)
		{
		Err	tempErr;
		fdP->curOffset = savedFileOffset;
		tempErr = PrvTruncateFile(fdP, savedFileSize);
		ErrNonFatalDisplayIf(tempErr, "error restoring file size");
		}

	// Validate file on exit
	#if (FILESTREAM_RIGOROUS_ERROR_CHECK == 1)
		ECValidateFileStream(fdP);
	#endif
	
	return err;
}


/***********************************************************************
 *
 * FUNCTION:		PrvTruncateFile
 *
 * DESCRIPTION:	Truncate a file and update its descriptor
 *
 * PARAMETERS:	fdP			-- ptr to file descriptor with a valid DmOpenRef
 *					newSize		-- new file size
 *
 * RETURNED:	zero on success, fileErr... on error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	11/26/97	Initial version
 *			vmk	12/19/97	Added block MemHandle-caching optimization
 *
 ***********************************************************************/
static Err PrvTruncateFile(FileDescriptorType* fdP, Int32 newSize)
{
	Err		err = 0;
	Int32		lastBlkIndex;
	Int32	lastBlkDataSize;
	Int32		firstDelIndex;
	Int32		lastDelIndex;
	//Int32		savedNumBlocks;
	
	
	ErrNonFatalDisplayIf(newSize < 0, "invalid file size passed");

	// Validate the file descriptor
	ECValidateFileDescriptor(fdP);

	ErrNonFatalDisplayIf(!(fdP->state & fileStateWriteAccess), "file is not writeable");
	ErrNonFatalDisplayIf(newSize > fdP->fileSize, "truncation size exceeds file size");
	if ( newSize > fdP->fileSize )
		return fileErrInvalidParam;
		
	// If file is to remain of the same size, we're done
	if ( newSize == fdP->fileSize )
		return 0;
	
	// Flush the cache
	prvFlushCached(fdP);
	
	// Get the index and data size of new last data block
	prvFilePosToBlockIndex(newSize, &lastBlkIndex, &lastBlkDataSize);
	
	//
	// Remove all extra blocks
	//

	// Get index of first block to be deleted
	if ( newSize == 0 )
		firstDelIndex = 0;
	else
		firstDelIndex = lastBlkIndex + 1;
		
	// Get index of last block to be deleted
	lastDelIndex = fdP->numBlocks - 1;
	
	//savedNumBlocks = fdP->numBlocks;
	
	// Remove, starting at the end
	for ( /*blank*/; lastDelIndex >= firstDelIndex; lastDelIndex-- )
		{
		err = DmRemoveRecord(fdP->dbR, (UInt16)lastDelIndex);
		if ( err )
			{
			ErrNonFatalDisplay("DmRemoveRecord failed");
			err = fileErrCorruptFile;
			break;
			}
			
		fdP->numBlocks--;									// update cached block count
		}

	if ( err )
		{
		fdP->state |= fileStateIOError;
		return err;
		}
	
	// Update file descriptor
	fdP->fileSize = newSize;
	if ( fdP->curOffset > fdP->fileSize )
		fdP->curOffset = fdP->fileSize;
		
	// Update data size of last remaining data block
	if ( fdP->numBlocks )
		{
		ErrNonFatalDisplayIf(lastBlkIndex != (fdP->numBlocks - 1), "invalid block index");
		err = PrvSetDataBlockSize(fdP, lastBlkIndex, lastBlkDataSize, NULL, false/*okToUncompress*/);
		}


	// Validate file on exit
	ECValidateFileStream(fdP);
	
	return err;
}


/***********************************************************************
 *
 * FUNCTION:		PrvCalcFileSizeInfo
 *
 * DESCRIPTION:	Compute the file stream size and update the the file descriptor's size info
 *
 * PARAMETERS:	fdP			-- ptr to file descriptor with a valid DmOpenRef
 *
 * RETURNED:	zero on success, fileErr... on error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	11/25/97	Initial version
 *
 ***********************************************************************/
static Err PrvCalcFileSizeInfo(FileDescriptorType* fdP)
{
	Err			err = 0;
	Int32			numBlocks;
	Int32			fileSize;
	UInt16			blkIndex;
	FileDataBlockType*	blkP;	// block pointer
	
	
	// The way to compute file size quickly is:
	//		((numBlocks - 1) * fileBlockDataSize) + "last block data size";
	//	All but the last block have max data size; only the last block is
	// allowed to have less then max; EC code watches for this.
	
	// Validate the file descriptor
	ECValidateFileDescriptor(fdP);
	
	// Scan all records, collecting size info and validating
	fileSize = 0;
	numBlocks = (Int32)DmNumRecords(fdP->dbR);
	if ( numBlocks )
		{
		// Lock the last block and compute data size
		blkIndex = (UInt16)(numBlocks - 1);
		blkP = prvLockDataBlockReadOnly(fdP, blkIndex);
		if ( blkP )
			{
			fileSize = ((numBlocks - 1) * fileBlockDataSize) + blkP->hdr.dataSize;
			prvUnlockDataBlockReadOnly(fdP, blkP, blkIndex);			// unlock the last block
			}
		else
			{
			ErrNonFatalDisplay("prvLockDataBlockReadOnly failed");
			err = fileErrCorruptFile;
			}
		}

	
	// Update file descriptor
	if ( !err )
		{
		fdP->fileSize = fileSize;
		fdP->numBlocks = numBlocks;
		
		// Validate the entire stream info
		ECValidateFileStream(fdP);
		} // if ( !err )
	

	return err;
}


/***********************************************************************
 *
 * FUNCTION:		PrvValidateFileSizeInfo
 *
 * DESCRIPTION:	Validate the stream and the file info in the descriptor;
 *
 * PARAMETERS:	fdP			-- ptr to file descriptor with a valid DmOpenRef
 *
 * RETURNED:	zero on success, fileErr... on error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	12/22/97	Initial version
 *
 ***********************************************************************/
static Err PrvValidateFileSizeInfo(FileDescriptorType* fdP)
{
	Err			err = 0;
	Int32			numBlocks;
	Int32			fileSize;
	UInt16			blkIndex;
	FileDataBlockType*	blkP;	// block pointer
	
	//	All but the last block have max data size; only the last block is
	// allowed to have less then max; EC code watches for this.
	
	// Validate the file descriptor
	ECValidateFileDescriptor(fdP);
	
	
	// Scan all records, collecting size info and validating
	fileSize = 0;
	numBlocks = (Int32)DmNumRecords(fdP->dbR);
	for (blkIndex=0; !err && blkIndex < numBlocks; blkIndex++)
		{
		blkP = prvLockDataBlockReadOnly(fdP, blkIndex);
		if ( blkP )
			{
			// Only the last block may contain less than the max amount of data
			if ( blkP->hdr.dataSize != fileBlockDataSize && blkIndex != (numBlocks - 1) )
				{
				ErrNonFatalDisplay("invalid block data size");
				err = fileErrCorruptFile;
				}
			else
				{
				fileSize += blkP->hdr.dataSize;
				}
			prvUnlockDataBlockReadOnly(fdP, blkP, blkIndex);
			}
		else
			{
			ErrNonFatalDisplay("prvLockDataBlockReadOnly failed");
			err = fileErrCorruptFile;
			}
		}
	
	if ( !err )
		{		
		if ( fdP->fileSize != fileSize )
			{
			ErrNonFatalDisplay("invalid file size in descriptor");
			err = fileErrInvalidDescriptor;
			}
			
		if ( fdP->numBlocks != numBlocks )
			{
			ErrNonFatalDisplay("invalid block count in descriptor");
			err = fileErrInvalidDescriptor;
			}
		} // if ( !err )
	

	return err;
}


/***********************************************************************
 *
 * FUNCTION:		PrvLockFileDescriptor
 *
 * DESCRIPTION:	Lock and validate the file descriptor
 *
 * PARAMETERS:	stream		-- open file MemHandle
 *					errP			-- pointer to variable for returning error code
 *
 * RETURNED:	pointer to file descriptor on success, NULL on failer;
 *					in *errP: zero on success, fileErr... on error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	11/25/97	Initial version
 *
 ***********************************************************************/
static FileDescriptorType* PrvLockFileDescriptor(FileHand stream, Err* errP)
{
	FileDescriptorType*	fdP = NULL;
	
	
	ErrNonFatalDisplayIf(!stream, "null stream");
	ErrNonFatalDisplayIf(!errP, "null err ptr");
	
	if ( !stream )
		{
		*errP = fileErrInvalidDescriptor;
		return NULL;
		}
		
	// Lock down the file descriptor
	//fdP = (FileDescriptorType*)MemHandleLock((MemHandle)stream);
	fdP = (FileDescriptorType*)stream;
	
	
	// For err-checking build, validate the file structure
	#if (FILESTREAM_RIGOROUS_ERROR_CHECK == 1)
		ECValidateFileStream(fdP);
	#endif
	
	// Validate the file descriptor
	*errP = PrvValidateFileDescriptor(fdP);
	if ( *errP )
		{
		//MemPtrUnlock(fdP);
		fdP = NULL;
		}
	
	return fdP;
}


/***********************************************************************
 *
 * FUNCTION:		PrvUnlockFileDescriptor
 *
 * DESCRIPTION:	Unlock and validate the file descriptor
 *
 * PARAMETERS:	fdP			-- pointer to locked file descriptor
 *					lastErrP		-- if not NULL, update the last error value
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	11/25/97	Initial version
 *
 ***********************************************************************/
static void PrvUnlockFileDescriptor(FileDescriptorType* fdP, Err* lastErrorP)
{
	// Validate the file descriptor
	ECValidateFileDescriptor(fdP);
	
	// For err-checking build, validate the file structure
	#if (FILESTREAM_RIGOROUS_ERROR_CHECK == 1)
		ECValidateFileStream(fdP);
	#endif

	// Update last error
	if ( fdP && lastErrorP )
		fdP->lastError = *lastErrorP;
	
	// Unlock the file descriptor
	//MemPtrUnlock(fdP);
}


/***********************************************************************
 *
 * FUNCTION:		PrvValidateFileDescriptor
 *
 * DESCRIPTION:	Validate the file descriptor; check for NULL descriptor,
 *						invalid signature, or NULL database reference
 *
 * PARAMETERS:	fdP			-- ptr to file descriptor with a valid DmOpenRef
 *
 * RETURNED:	zero on success, fileErr... on error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	11/25/97	Initial version
 *
 ***********************************************************************/
static Err PrvValidateFileDescriptor(FileDescriptorType* fdP)
{
	ErrNonFatalDisplayIf(!fdP, "null file descriptor");
	ErrNonFatalDisplayIf(fdP->sig != fileDescriptorSignature, "invalid file descriptor signature");
	ErrNonFatalDisplayIf(!fdP->dbR, "null DmOpenRef");
	#if (FILESTREAM_RIGOROUS_ERROR_CHECK == 1)
		ErrNonFatalDisplayIf((fdP->state & fileStateOpenComplete) && DmNumRecords(fdP->dbR) != fdP->numBlocks,
			"invalid fd block count");
	#endif
	ErrNonFatalDisplayIf(fdP->fileSize < 0 || fdP->fileSize > (fdP->numBlocks * fileBlockDataSize),
		"invalid file size");
	ErrNonFatalDisplayIf(fdP->curOffset < 0 || fdP->curOffset > fdP->fileSize, "invalid file offset");
	ErrNonFatalDisplayIf(fdP->state & (~fileAllStates), "invalid state flags");
	
	if ( !fdP || fdP->sig != fileDescriptorSignature )
		return fileErrInvalidDescriptor;

	return 0;	
}


/***********************************************************************
 *
 * FUNCTION:		PrvAddDataBlock
 *
 * DESCRIPTION:	Add a new data block and initialize its header and update the
 *						file descriptor
 *
 * PARAMETERS:	fdP			-- ptr to file descriptor with a valid DmOpenRef
 *					dataSize		-- data size for new block
 *
 * RETURNED:	zero on success, fileErr... on error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	11/26/97	Initial version
 *			vmk	12/19/97	Added block MemHandle-caching optimization
 *
 ***********************************************************************/
static Err PrvAddDataBlock(FileDescriptorType* fdP, Int32 dataSize)
{
	Err	err = 0;
	UInt16	blkIndex;
	MemHandle	blkH;
	FileDataBlockType*	blkP = NULL;	// block pointer
	FileBlockHeaderType	hdr;
	
	
	ErrNonFatalDisplayIf(dataSize < 0 || dataSize > fileBlockDataSize, "invalid data size");

	// Validate the file descriptor
	ECValidateFileDescriptor(fdP);

	ErrNonFatalDisplayIf(!(fdP->state & fileStateWriteAccess), "file is not writeable");
	
	// Allocate the new data block at the end
	blkIndex = dmMaxRecordIndex;
	blkH = DmNewRecord(fdP->dbR, &blkIndex, sizeof(FileDataBlockType));
	if ( !blkH )
		return fileErrMemError;
	
	// Lock the new data block
	blkP = (FileDataBlockType*)MemHandleLock(blkH);
	
	// Initialize the block's header
	hdr.sig = fileDataBlockSignature;
	hdr.dataSize = dataSize;
	DmWrite(blkP, prvMemberOffset(FileDataBlockType, hdr),
		&hdr, sizeof(blkP->hdr));

	
	// Update file descriptor
	fdP->fileSize += dataSize;
	fdP->numBlocks++;
	
	// Cache the block info
	prvFlushCached(fdP);
	prvSetCached(fdP, blkH, blkIndex, true/*forWrite*/);
	
	// Unlock the data block
	prvUnlockDataBlockReadWrite(fdP, blkP, blkIndex);
	

	// Validate file on exit
	#if (FILESTREAM_RIGOROUS_ERROR_CHECK == 1)
		ECValidateFileStream(fdP);
	#endif
	
	return err;
}


/***********************************************************************
 *
 * FUNCTION:		PrvSetDataBlockSize
 *
 * DESCRIPTION:	Sets size of a data block; requested block is expected to be present.
 *
 * PARAMETERS:	fdP			-- ptr to file descriptor with a valid DmOpenRef
 *					blockIndex	-- block index
 *					newSize		-- new data size
 *					oldSizeP		-- old data size is returned here (optional - set to NULL for none)
 *					okToUncompress	-- if true, the file may be uncompressed
 *
 * RETURNED:	zero on success, fileErr... on error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	11/26/97	Initial version
 *
 ***********************************************************************/
static Err PrvSetDataBlockSize(FileDescriptorType* fdP, Int32 blkIndex,
				Int32 newSize, Int32 * oldSizeP, Boolean okToUncompress)
{
	Err	err = 0;
	FileDataBlockType*	blkP = NULL;	// block pointer

	ErrNonFatalDisplayIf(newSize < 0 || newSize > fileBlockDataSize, "invalid block data size");

	blkP = prvLockDataBlockReadWrite(fdP, blkIndex, okToUncompress);	// lock the data block, uncompressing if necessary
	if ( blkP )
		{
		if ( oldSizeP )
			*oldSizeP = blkP->hdr.dataSize;
			
		// Update block's data size
		DmWrite(blkP, prvMemberOffset(FileDataBlockType, hdr.dataSize),
			&newSize, sizeof(blkP->hdr.dataSize));
		prvUnlockDataBlockReadWrite(fdP, blkP, blkIndex);			// unlock the data block
		}
	else
		err = fileErrCorruptFile;
		
	return err;
}


/***********************************************************************
 *
 * FUNCTION:		PrvGetDataBlockSize
 *
 * DESCRIPTION:	Gets data size of a data block; requested block is expected to be present.
 *
 * PARAMETERS:	fdP			-- ptr to file descriptor with a valid DmOpenRef
 *					blockIndex	-- block index
 *					sizeP			-- ptr to variable for returning block size
 *					recSizeP		-- ptr to variable for returning total record size
 *
 * RETURNED:	zero on success, fileErr... on error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	1/10/98	Initial version
 *
 ***********************************************************************/
static Err PrvGetDataBlockSize(FileDescriptorType* fdP, Int32 blkIndex, Int32 * sizeP, UInt32 * recSizeP)
{
	Err	err = 0;
	FileDataBlockType*	blkP = NULL;	// block pointer

	ErrNonFatalDisplayIf(!sizeP || !recSizeP, "null sizeP or recSizeP");


	blkP = prvLockDataBlockReadOnly(fdP, blkIndex);				// lock the data block
	if ( blkP )
		{
		*sizeP = blkP->hdr.dataSize;									// get the data size
		*recSizeP = MemPtrSize(blkP);									// get the total record size
		prvUnlockDataBlockReadOnly(fdP, blkP, blkIndex);		// unlock the data block
		}
	else
		err = fileErrCorruptFile;
		
	return err;
}


/***********************************************************************
 *
 * FUNCTION:		PrvLockBlockByPos
 *
 * DESCRIPTION:	Locks the data block where the passed position belongs.  The block
 *						is expected to be already allocated.  If not, an error will be returned
 *						and, in debug build, an assertion will fail.
 *
 * PARAMETERS:	fdP			-- ptr to file descriptor with a valid DmOpenRef
 *					pos			-- absolute file position that belongs on the desired data block
 *					blkPP			-- ptr to variable for returning the block pointer
 *					blockIndexP	-- ptr to variable for returning the block index
 *					dataOffsetP	-- ptr to variable for returning the data offset within the block
 *					forWrite		-- if true, the block will be locked for writing (perf optimization)
 *
 * RETURNED:	zero on success, fileErr... on error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	11/26/97	Initial version
 *
 ***********************************************************************/
static Err PrvLockBlockByPos(FileDescriptorType* fdP, Int32 pos, FileDataBlockType** blkPP,
	Int32 * blockIndexP, Int32 * dataOffsetP, Boolean forWrite)
{
	//Err			err = 0;
	Int32			blkIndex;
	Int32			blkDataOffset;
	FileDataBlockType*	blkP = NULL;	// block pointer
	
	
	ErrNonFatalDisplayIf(pos < 0, "invalid file pos");
	ErrNonFatalDisplayIf(!blkPP, "null blkPP");
	ErrNonFatalDisplayIf(!blockIndexP, "null blockIndexP");
	ErrNonFatalDisplayIf(!dataOffsetP, "null dataOffsetP");
	
	*blkPP = NULL;
	
	// Validate the file descriptor
	ECValidateFileDescriptor(fdP);

	// Validate file
	#if (FILESTREAM_RIGOROUS_ERROR_CHECK == 1)
		ECValidateFileStream(fdP);
	#endif
	
	// Make sure the file is big enough to contain a block for the desired position
	#if 0		// CASTRATION -- the file is expected to already be large enough to contain the desired block
		if ( okToExtend )
			{
			err = PrvExtendFile(fdP, pos, false);
			if ( err )
				return err;
			}
	#endif	// CASTRATION
	
	// Make sure the position is no larger than one byte past last byte of file
	if ( pos > fdP->fileSize )
		{
		ErrNonFatalDisplay("file pos out of bounds");	// should not happen
		return fileErrOutOfBounds;
		}
		
	// Convert file position to block index and data offset
	prvFilePosToBlockIndex(pos, &blkIndex, &blkDataOffset);
	ErrNonFatalDisplayIf(blkIndex >= fdP->numBlocks, "block index out of bounds");	// should not happen
	
	// Lock the data block
	blkP = PrvLockDataBlock(fdP, blkIndex, forWrite, false/*uncompress*/);
	if ( !blkP )
		return fileErrCorruptFile;

	// SUCCESS:
	// Set up return variables
	*blkPP = blkP;
	*blockIndexP = blkIndex;
	*dataOffsetP = blkDataOffset;
	return 0;
}


/***********************************************************************
 *
 * FUNCTION:		PrvLockDataBlock
 *
 * DESCRIPTION:	Locks and validates a data block; it is expected that the
 *						requested data block exists
 *
 *						If the MemHandle of the block being locked is already in our cache,
 *						and its write "access" matches the request, will just lock the file
 *						hanlde; otherwise, will flush the cache, obtain the new record MemHandle,
 *						lock it and add it to the cache
 *
 * PARAMETERS:	fdP			-- ptr to file descriptor with a valid DmOpenRef
 *					blockIndex	-- block index
 *					forWrite		-- if true, the block will be marked as busy (optimization)
 *					uncompress	-- if true, forWrite blocks will be uncompressed
 *
 * RETURNED:	pointer to data block, NULL on error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	11/26/97	Initial version
 *			vmk	12/19/97	Added block MemHandle-caching optimization
 *
 ***********************************************************************/
static FileDataBlockType* PrvLockDataBlock(FileDescriptorType* fdP, Int32 blockIndex,
		Boolean forWrite, Boolean uncompress)
{
	Err						err = 0;
	MemHandle					blkH = 0;
	FileDataBlockType*	blkP = NULL;
	Boolean					mustUncompress;
	
	ErrNonFatalDisplayIf(blockIndex < 0 || blockIndex > dmMaxRecordIndex, "block index out of bounds");
	ErrNonFatalDisplayIf(uncompress && !forWrite, "invalid uncompress arg");
	
	// Validate file descriptor
	ECValidateFileDescriptor(fdP);


	// Determine if this block needs to be uncompressed -- only the last block may need uncompression
	mustUncompress = (uncompress && prvIsUncompressionNeeded(fdP, blockIndex));

	// If we have a cached block, see if it is the one we need (perf optimization)
	if ( fdP->cached.blockH )
		{
		if ( !mustUncompress && blockIndex == fdP->cached.blockIndex && (!forWrite || fdP->cached.forWrite) )
			{
			// Validate the block MemHandle cache
			ErrNonFatalDisplayIf(DmQueryRecord(fdP->dbR, fdP->cached.blockIndex) != fdP->cached.blockH,
				"mismatched cached block index or MemHandle");
				
			blkH = fdP->cached.blockH;											// get from cache
			}
		else
			{
			prvFlushCached(fdP);
			ErrNonFatalDisplayIf(fdP->cached.blockH, "block not flushed");
			}
		}

	// Get and lock the data block
	if ( !blkH )
		{
		if ( forWrite )
			{
			err = 0;
			if ( mustUncompress )
				err = PrvUncompressFile(fdP);
			if ( !err )
				blkH = DmGetRecord(fdP->dbR, (UInt16)blockIndex);			// marks the record busy (for writes)
			}
		else
			blkH = DmQueryRecord(fdP->dbR, (UInt16)blockIndex);			// doesn't mark the record busy (perf optimization for reads)

		// Add the block to cached info
		if ( blkH )
			prvSetCached(fdP, blkH, blockIndex, forWrite);
		}
	ErrNonFatalDisplayIf(!blkH && !err, "data block missing");
	if ( !blkH )
		return NULL;
		
	blkP = (FileDataBlockType*)MemHandleLock(blkH);
	
	// Validate the data block
	if ( !PrvIsValidDataBlock(fdP, blkP) )
		{
		MemPtrUnlock(blkP);
		prvFlushCached(fdP);
		return NULL;
		}
	
	
	return blkP;
}



/***********************************************************************
 *
 * FUNCTION:		PrvUnlockDataBlock
 *
 * DESCRIPTION:	Unlocks and validates a data block
 *
 *						It is expected that the block being unlocked is in our
 *						MemHandle cache; we will unlock the pointer, but not release
 *						the block MemHandle; to release the cached MemHandle, prvFlushCached
 *						must be called
 *
 * PARAMETERS:	fdP			-- ptr to file descriptor with a valid DmOpenRef
 *					blkP			-- block pointer
 *					blockIndex	-- block index
 *					dirty			-- if true, the block will be marked dirty
 *					forWrite		-- if true, will make the extra call to release the record (optimization)
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	11/26/97	Initial version
 *			vmk	12/19/97	Added block MemHandle-caching optimization
 *
 ***********************************************************************/
static void PrvUnlockDataBlock(FileDescriptorType* fdP, FileDataBlockType* blkP,
	Int32 blockIndex, Boolean /* dirty */, Boolean forWrite)
{
	ErrNonFatalDisplayIf(blockIndex < 0 || blockIndex > dmMaxRecordIndex, "block index out of bounds");

	// Validate file descriptor
	ECValidateFileDescriptor(fdP);
		
	ECValidateDataBlock(fdP, blkP);
	
	// In our caching scheme, any block being unlocked by this routine must
	// be in the cache
	ErrNonFatalDisplayIf(	!fdP->cached.blockH ||
									fdP->cached.blockIndex != blockIndex ||
									(forWrite && !fdP->cached.forWrite),
									"bug in file block cache scheme"	);
	
	MemPtrUnlock(blkP);
	
	
	#if 0		// we're caching, so don't release when unlocking
		// Release only if it was locked for writing (optimization)
		if ( forWrite )
			DmReleaseRecord(fdP->dbR, blockIndex, dirty);
	#endif
}


/***********************************************************************
 *
 * FUNCTION:		PrvIsValidDataBlock
 *
 * DESCRIPTION:	Validates a data block
 *
 * PARAMETERS:	blkP			-- block pointer
 *
 * RETURNED:	non-zero if valid
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	11/26/97	Initial version
 *
 ***********************************************************************/
static Boolean PrvIsValidDataBlock(FileDescriptorType* fdP, FileDataBlockType* blkP)
{
	ErrNonFatalDisplayIf(!blkP, "null block ptr");
	
	// Validate the data block signature
	if ( blkP->hdr.sig != fileDataBlockSignature )
		{
		ErrNonFatalDisplay("invalid data block signature");
		return false;
		}
	
	// Error-check block size
	// Block size may be less than full if the file has been compressed
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	{
	UInt32		recSize;
	recSize = MemPtrSize(blkP);
	ErrNonFatalDisplayIf(recSize > sizeof(FileDataBlockType), "block rec size is too big");
	ErrNonFatalDisplayIf(!(fdP->state & fileStateCompressed) && recSize != sizeof(FileDataBlockType),
		"invalid block rec size");
	}
#endif
	
	if ( blkP->hdr.dataSize < 0 || blkP->hdr.dataSize > fileBlockDataSize )
		{
		ErrNonFatalDisplay("invalid block data size");
		return false;
		}
		
	return true;
}


/***********************************************************************
 *
 * FUNCTION:		PrvCompressFile
 *
 * DESCRIPTION:	Compress unused data from the last data block.  Called when
 *						a writeable file is being closed.
 *
 * PARAMETERS:	stream		-- MemHandle of open file
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	1/10/98	Initial version
 *
 ***********************************************************************/
static void PrvCompressFile(FileDescriptorType* fdP)
{
	Err		err;
	Int32	blkDataSize;
	UInt32		curRecSize;
	UInt32		newRecSize;
	Int32		blkIndex;
	MemHandle	recH;


	// Validate the file descriptor
	ECValidateFileDescriptor(fdP);
	
	ErrNonFatalDisplayIf(!(fdP->state & fileStateWriteAccess), "can't compress a non-writeable file");

	// If we're still compressed, we're done! (optimization - prevents unnecessary backups)
	if ( fdP->state & fileStateCompressed )
		return;
	
	// Truncate the last data block at the end of valid data
	if ( fdP->numBlocks )
		{
		blkIndex = fdP->numBlocks - 1;
		
		// Get the block's valid data size
		err = PrvGetDataBlockSize(fdP, blkIndex, &blkDataSize, &curRecSize);
		ErrNonFatalDisplayIf(err, "PrvGetDataBlockSize failed");		// shouldn't happen
		ErrNonFatalDisplayIf(blkDataSize > fileBlockDataSize, "invalid block data size");
		
		// Compute new record size
		newRecSize = blkDataSize + sizeof(FileBlockHeaderType);
		ErrNonFatalDisplayIf(newRecSize > curRecSize, "invalid record size");	// should not happen

		// Shrink it
		if ( newRecSize < curRecSize )										// check to avoid unnecessary backups
			{
			// Flush the cached block info to release the block in case it is the one we're resizing
			prvFlushCached(fdP);
			
			// Shrink the record by resizing the memory MemHandle directly to avoid the database being
			// marked modified due to compression (avoids unnecessary backups)
			recH = DmGetRecord(fdP->dbR, (UInt16)blkIndex);
			ErrNonFatalDisplayIf(!recH, "DmGetRecord failed");			// should not happen
			// This should always succeed because we're shrinking it
			err = MemHandleResize(recH, newRecSize);
			ErrNonFatalDisplayIf(err, "MemHandleResize failed");		// should not happen
			DmReleaseRecord(fdP->dbR, (UInt16)blkIndex, false/*dirty*/);
			
			fdP->state |= fileStateCompressed;
			}
		}
	
}


/***********************************************************************
 *
 * FUNCTION:		PrvUncompressFile
 *
 * DESCRIPTION:	Uncompress a writeable file by restoring the last data block
 *						to the standard size.  We can only count on the dbR and state
 *						flags being valid.
 *
 * PARAMETERS:	stream		-- MemHandle of open file
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	1/10/98	Initial version
 *
 ***********************************************************************/
static Err PrvUncompressFile(FileDescriptorType* fdP)
{
	Err		err = 0;
	UInt32		newRecSize;
	Int32		blkIndex;
	MemHandle	newH;
	Int32		numBlocks;

	// Validate the file descriptor
	ECValidateFileDescriptor(fdP);
	
	ErrNonFatalDisplayIf(!(fdP->state & fileStateWriteAccess), "can't uncompress a non-writeable file");

	// If we're not compressed, we're done! (optimization)
	if ( !(fdP->state & fileStateCompressed) )
		return 0;
	
	// Uncompress the last data block
	numBlocks = (Int32)DmNumRecords(fdP->dbR);
	if ( numBlocks )
		{
		blkIndex = numBlocks - 1;										// last block's index
		// Resize the block to the full standard size
		newRecSize = sizeof(FileDataBlockType);
		newH = DmResizeRecord(fdP->dbR, (UInt16)blkIndex, newRecSize);
		if ( !newH )
			err = fileErrMemError;
		}
	
	if ( !err )
		fdP->state &= (~fileStateCompressed);				// clear the compressed state flag (optimization)
		
	return err;
}
