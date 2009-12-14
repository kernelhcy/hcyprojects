/******************************************************************************
 *
 * Copyright (c) 1997-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: ExgDB.c
 *
 * Release: 
 *
 * Description:
 * ------------
 * This module supports transfer of Pilot database files ONLY and is not
 * guaranteed to be compatible in the future if the format of Pilot
 * Databases changes!!! Notice that it includes <DataPrv.h> which is a
 * NON_PORTABLE header file. Future versions of PalmOS might change the
 * definitions in this header.
 *
 * History:
 * ----------
 * 970922	Original, based on CmdFTP.c (Marco Frigino)
 * 7/28/00	kja	Tweaked ExgDBWrite to not send out multiple record lists (that
 *				makes a file format that nothing else understands). The current,
 *				and original, ExgDBRead can read both old and new format.
 * 11/08/00	kja	Removed regeneration of unique IDs on write: previously, unique IDs
 *				for output records were regenerated, starting at 1 for the first record,
 *				and increasing by one for each subsequent record. Now the unique IDs
 *				are copied directly into the output.
 * 11/09/00	kja	Add several checks to ExgDBRead to return a specific error
 *				(dmErrCorruptDatabase) if one of the (few) constraints of the prc
 *				format is violated, instead of wild and crazy things with
 *				negative numbers. Also modified ReadDmData and ReadHandleData to
 *				detect the end of the input in the middle of a record.
 *				
 *
 *****************************************************************************/

// =============================================================================
// ================================== Includes =================================
// =============================================================================

#define	NON_PORTABLE

#include <PalmTypes.h>

#include <PalmUtils.h>

#include "DataMgr.h"
#include "ExgMgr.h"
#include "MemoryMgr.h"
#include "StringMgr.h"
#include "ErrorBase.h"

#include "DataPrv.h"


// =============================================================================
// ================================== Macros ===================================
// =============================================================================

#define	GOTO_WITH_ERR(label, value) \
			err = (value); goto label;

#define	GOTO_IF_NULL(label, val) \
			if (val == 0) goto label;

#define	GOTO_IF_ERR(label) \
			if (err != 0) goto label;

#define	GOTO_IF_NULL_SET_ERR(label, value, errVal) \
			if (value == NULL) {err = errVal; goto label;}

#define	GOTO_IF_NULL_SET_DM_LAST_ERR(label, value) \
			GOTO_IF_NULL_SET_ERR(label, value, DmGetLastErr())

#define	GOTO_IF_ZERO_SET_ERR(label, value, errVal) \
			if (value == 0) {err = errVal; goto label;}

#define	GOTO_IF_ZERO_SET_DM_LAST_ERR(label, value) \
			GOTO_IF_ZERO_SET_ERR(label, value, DmGetLastErr())

// =============================================================================
// ================================= Typedefs ==================================
// =============================================================================

// -----------------------------------------------------------------------------
//	CountRecordListsInfo
// -----------------------------------------------------------------------------
typedef struct {
	UInt32					numOfEntries;
	UInt32					firstUsableOffset;
	UInt16					numOfRecordLists;
} CountRecordListsInfo, *CountRecordListsInfoPtr;


// -----------------------------------------------------------------------------
//	ResizeHandleProcPtr
// -----------------------------------------------------------------------------
typedef Err (*ResizeHandleProcPtr)(
	MemHandle*					dataHP,
	void*					resizeInfoP,
	UInt32					newSize);


// -----------------------------------------------------------------------------
//	ResizeHandleInfo
// -----------------------------------------------------------------------------
typedef struct {
	DmOpenRef				dbRef;
} ResizeHandleInfo, *ResizeHandleInfoPtr;


// -----------------------------------------------------------------------------
//	ResizeRecordInfo
// -----------------------------------------------------------------------------
typedef struct {
	DmOpenRef				dbRef;
	UInt16					recIdx;
} ResizeRecordInfo, *ResizeRecordInfoPtr;


// -----------------------------------------------------------------------------
//	ReadListProcPtr
// -----------------------------------------------------------------------------
typedef Err (*ReadListProcPtr)(
	UInt16					numEntries,
	DatabaseHdrType*		hdrP,
	void*					userDataP);


// -----------------------------------------------------------------------------
//	ReadEntryInfo
// -----------------------------------------------------------------------------
typedef struct {
	ExgDBReadProcPtr		readProcP;
	void*					userDataP;
	UInt32*					readCountP;
	UInt32					prevEntryDataOffset;
	UInt32					firstEntryDataOffset;
	UInt16					nextEntryIdx;
	UInt16					totalEntries;
	DmOpenRef				dbRef;
	UInt16					cardNo;
} ReadEntryInfo, *ReadEntryInfoPtr;


// -----------------------------------------------------------------------------
//	WriteListProcPtr
// -----------------------------------------------------------------------------
typedef Err (*WriteListProcPtr)(
	const RecordListType*	recListP,
	const DatabaseHdrType*	hdrP,
	void*					userDataP);


// -----------------------------------------------------------------------------
//	WriteEntryInfo
// -----------------------------------------------------------------------------
typedef struct {
	ExgDBWriteProcPtr		writeProcP;
//	ExgDBProgressProcPtr	progressProcP;
	void*					userDataP;
	UInt32*					writeCountP;
	UInt32					nextEntryDataOffset;
	UInt16					cardNo;
} WriteEntryInfo, *WriteEntryInfoPtr;

// =============================================================================
// ================================= Constants =================================
// =============================================================================

// Change the size of this buffer to optimize read speed
#define		kReadBufSize  1024

// =============================================================================
// ================================= Prototypes ================================
// =============================================================================

static Err			GetRecordHandle(
							MemHandle*					recHP,
							UInt32*					uniqueIDP,
							UInt16*					attrP,
							DmOpenRef				dbRef,
							UInt16					recNum);

static void			GetChunkSize(
							LocalID					chunkID,
							UInt32*					sizeP,
							UInt16					cardNo);

static Err			ResizeHandle(
							DmOpenRef				dbRef,
							MemHandle*					dataHP,
							UInt32					newSize);

static Err			ResizeHandleCallback(
							MemHandle*					dataHP,
							void*					resizeInfoP,
							UInt32					newSize);

static Err			ResizeResourceCallback(
							MemHandle*					dataHP,
							void*					resizeInfoP,
							UInt32					newSize);

static Err			ResizeRecordCallback(
							MemHandle*					dataHP,
							void*					resizeInfoP,
							UInt32					newSize);

static inline Err	ReadHandleData(
							ExgDBReadProcPtr		readProcP,
							void*					userDataP,
							UInt8 *					readBufP,
							UInt32*					readCountP,
							MemHandle					dataH);

static Err			ReadHandleUntilEnd(
							ExgDBReadProcPtr		readProcP,
//							ExgDBProgressProcPtr	progressProcP,
							void*					userDataP,
							UInt8 *					readBufP,
							UInt32*					readCountP,
							ResizeHandleProcPtr		resizeProcP,
							void*					resizeInfoP,
							MemHandle*					dataHP);

static Err			ReadDmData(
							ExgDBReadProcPtr		readProcP,
							void*					userDataP,
							UInt8 *					readBufP,
							void*					dataP,
							UInt32					dataSize,
							UInt32*					readCountP);

static inline Err	ReadData(
							ExgDBReadProcPtr		readProcP,
							void*					userDataP,
							void*					dataP,
							UInt32					dataSize,
							UInt32*					readCountP);

static inline Err	SkipData(
							ExgDBReadProcPtr		readProcP,
							void*					userDataP,
							void*					bufferP,
							UInt32					bufferSize,
							UInt32					skipSize,
							UInt32*					readCountP);

static Err			ReadEntryData(
							ExgDBReadProcPtr		readProcP,
//							ExgDBProgressProcPtr	progressProcP,
							void*					userDataP,
							UInt8 *					readBufP,
							DatabaseHdrType*		hdrP,
							UInt32*					readCountP,
							ReadEntryInfoPtr		infoP,
							DmOpenRef				dbRef);

static Err			DoForEachReadEntryList(
							ReadListProcPtr			procP,
							DatabaseHdrType*		hdrP,
							void*					userDataP);

static inline Err	CreateRecordEntry(
							DmOpenRef				dbRef,
							RecordEntryPtr			recEntryP,
							UInt32					entrySize,
							UInt16*					nextEntryIdxP);

static inline Err	CreateResourceEntry(
							DmOpenRef				dbRef,
							RsrcEntryPtr			resEntryP,
							UInt32					entrySize);

static Err			ReadListCallback(
							UInt16					numEntries,
							DatabaseHdrType*		hdrP,
							void*					userDataP);

static Err			ReadInfoHandleData(
							ExgDBReadProcPtr		readProcP,
//							ExgDBProgressProcPtr	progressProcP,
							void*					userDataP,
							UInt8 *					readBufP,
							UInt32*					readCountP,
							DmOpenRef				dbRef,
							MemHandle*					infoHP,
							ReadEntryInfoPtr		infoP,
							UInt32					infoOffset);

static inline Err	ReadEntryLists(
							ExgDBReadProcPtr		readProcP,
//							ExgDBProgressProcPtr	progressProcP,
							void*					userDataP,
							DatabaseHdrType*		hdrP,
							UInt32*					readCountP,
							ReadEntryInfoPtr		infoP,
							DmOpenRef				dbRef,
							UInt16					cardNo);

static Err			ReadDatabase(
							ExgDBReadProcPtr		readProcP,
//							ExgDBProgressProcPtr	progressProcP,
							void*					userDataP,
							UInt8 *					readBufP,
							DatabaseHdrType*		hdrP,
							UInt32*					readCountP,
							DmOpenRef				dbRef,
							LocalID					dbID,
							UInt16					cardNo);

static Err			DoForEachWriteEntryList(
							const WriteListProcPtr	procP,
							const DatabaseHdrType*	hdrP,
							void*					userDataP,
							UInt16					cardNo);

static Err			CountEntriesCallback(
							const RecordListType*	listP,
							const DatabaseHdrType*	hdrP,
							void*					userDataP);

static inline Err	CalcEntryInfo(
							const DatabaseHdrType*	hdrP,
							CountRecordListsInfoPtr	infoP,
							UInt16					cardNo);

static inline Err	WriteData(
							ExgDBWriteProcPtr		writeProcP,
							void*					userDataP,
							const void*				dataP,
							UInt32					dataSize,
							UInt32*					writeCountP);

static Err			WriteChunkData(
							ExgDBWriteProcPtr		writeProcP,
//							ExgDBProgressProcPtr	progressProcP,
							void*					userDataP,
							UInt32*					writeCountP,
							LocalID					chunkID,
							UInt16					cardNo);

static inline Err	WriteEntryLists(
							ExgDBWriteProcPtr		writeProcP,
//							ExgDBProgressProcPtr	progressProcP,
							void*					userDataP,
							const DatabaseHdrType*	hdrP,
							UInt32*					writeCountP,
							UInt32					firstEntryDataOffset,
							UInt16					cardNo,
							UInt16					totalRecordCount);

static Err			WriteListCallback(
							const RecordListType*	listP,
							const DatabaseHdrType*	hdrP,
							void*					userDataP);

static Err			WriteDatabase(
							ExgDBWriteProcPtr		writeProcP,
//							ExgDBProgressProcPtr	progressProcP,
							void*					userDataP,
							const DatabaseHdrType*	hdrP,
							UInt32*					writeCountP,
							DmOpenRef				dbRef,
							UInt16					cardNo);


// =============================================================================
// ================================= Functions =================================
// =============================================================================

// -----------------------------------------------------------------------------
//	GetRecordHandle
// -----------------------------------------------------------------------------
Err GetRecordHandle(
	MemHandle*					recHP,
	UInt32*					uniqueIDP,
	UInt16*					attrP,
	DmOpenRef				dbRef,
	UInt16					recNum)
{
	LocalID					recChunkID;
	Err						err = 0;

	// Try to get the record info
	err = DmRecordInfo(dbRef, recNum, attrP, uniqueIDP, &recChunkID);
	// Exit if error
	GOTO_IF_ERR(Exit);
	// If the record is marked as deleted, temporarily un-delete it so
	// that we can get the MemHandle to it's data, if any
	if (((*attrP & dmRecAttrDelete) != 0) && (recChunkID != 0))
		{
		// Mask out delete bit
		*attrP &= ~dmRecAttrDelete;
		// Try to set record info
		err = DmSetRecordInfo(dbRef, recNum, attrP, 0);
		// Exit if error
		GOTO_IF_ERR(Exit);
		// Get record MemHandle
		*recHP = DmQueryRecord(dbRef, recNum);
		// Set delete bit
		*attrP |= dmRecAttrDelete;
		// Try to set record info
		err = DmSetRecordInfo(dbRef, recNum, attrP, 0);
		// Exit if error
		GOTO_IF_ERR(Exit);
		}
	else
		{
		// Get record MemHandle
		*recHP = DmQueryRecord(dbRef, recNum);
		}
Exit:
	// Return error code
	return err;
}


// -----------------------------------------------------------------------------
//	GetChunkSize
// -----------------------------------------------------------------------------
void GetChunkSize(
	LocalID					chunkID,
	UInt32*					sizeP,
	UInt16					cardNo)
{
	// If info ID valid
	if (chunkID != 0)
		{
		// Get locked MemPtr to chunk
		MemPtr	chunkP = MemLocalIDToLockedPtr(chunkID, cardNo);
		// Get chunk size
		*sizeP = MemPtrSize(chunkP);
		// Unlock chunk
		MemPtrUnlock(chunkP);
		}
	else
		{
		// Clear size
		*sizeP = 0;
		}
}


#pragma mark ---------------------------
// -----------------------------------------------------------------------------
//	ResizeHandle
// -----------------------------------------------------------------------------
static Err ResizeHandle(
	DmOpenRef				dbRef,
	MemHandle*					dataHP,
	UInt32					newSize)
{
	MemHandle					newH;
	UInt16 *					srcP;
	UInt16 *					newP;
	Err						err;

	// Try to resize MemHandle
	err = MemHandleResize(*dataHP, newSize);
	// If failed
	if (err != 0)
		{
		// Try to reallocate it in another heap
		newH = DmNewHandle((DmAccessPtr)dbRef, newSize);
		// Exit if reallocation failed
		// DOLATER: Calling DmGetLastErr is invalid here
		// because the lastErr is not set by DmNewHandle.
		GOTO_IF_NULL_SET_DM_LAST_ERR(Exit, newH);
		// Get MemPtr to src MemHandle
		srcP = (UInt16 *)MemHandleLock(*dataHP);
		// Get MemPtr to dst MemHandle
		newP = (UInt16 *)MemHandleLock(newH);
		// Move data to new MemHandle
		err = DmWrite(newP, 0, srcP, MemHandleSize(*dataHP));
		// Unlock new MemHandle
		MemHandleUnlock(newH);
		// Free given MemHandle
		MemHandleFree(*dataHP);
		// Exit if error
		GOTO_IF_ERR(Exit);
		// Delete old MemHandle
		MemHandleFree(*dataHP);
		// Save new MemHandle
		*dataHP = newH;
		// Clear error
		err = 0;
		}
Exit:
	// Return error code
	return err;
}


// -----------------------------------------------------------------------------
//	ResizeHandleCallback
// -----------------------------------------------------------------------------
static Err ResizeHandleCallback(
	MemHandle*					dataHP,
	void*					resizeInfoP,
	UInt32					newSize)
{
	ResizeHandleInfoPtr		infoP = (ResizeHandleInfoPtr)resizeInfoP;

	// Try to resize given MemHandle and return error
	return ResizeHandle(infoP->dbRef, dataHP, newSize);
}


// -----------------------------------------------------------------------------
//	ResizeResourceCallback
// -----------------------------------------------------------------------------
static Err ResizeResourceCallback(
	MemHandle*				dataHP,
	void*					/* resizeInfoP */,
	UInt32					newSize)
{
	// Try to resize resource MemHandle
	*dataHP = DmResizeResource(*dataHP, newSize);
	// Return error code
	return DmGetLastErr();
}


// -----------------------------------------------------------------------------
//	ResizeRecordCallback
// -----------------------------------------------------------------------------
static Err ResizeRecordCallback(
	MemHandle*					dataHP,
	void*					resizeInfoP,
	UInt32					newSize)
{
	ResizeRecordInfoPtr		infoP = (ResizeRecordInfoPtr)resizeInfoP;

	// Try to resize record MemHandle
	*dataHP = DmResizeRecord(infoP->dbRef, infoP->recIdx, newSize);
	// Return error code
	return DmGetLastErr();
}


// -----------------------------------------------------------------------------
//	ReadHandleData
// -----------------------------------------------------------------------------
// Reads exactly the number of bytes that fit in the given MemHandle
static inline Err ReadHandleData(
	ExgDBReadProcPtr		readProcP,
	void*					userDataP,
	UInt8 *					readBufP,
	UInt32*					readCountP,
	MemHandle				dataH)
{
	UInt8 *					dataP;
	Err						err;

	// Get MemPtr to locked entry data
	dataP = MemHandleLock(dataH);
	// Try to read data into protected memory
	err = ReadDmData(readProcP, userDataP, readBufP, dataP, MemHandleSize(dataH),
					readCountP);
	
	// Generate an error if we didn't read as many bytes as we were expecting
	if (!err && (*readCountP != MemHandleSize(dataH)))
		{
		err = dmErrCorruptDatabase;
		}
	
	// Unlock entry data
	MemHandleUnlock(dataH);
	// Return error code
	return err;
}


// -----------------------------------------------------------------------------
//	ReadData
// -----------------------------------------------------------------------------
static inline Err ReadData(
	ExgDBReadProcPtr		readProcP,
	void*					userDataP,
	void*					dataP,
	UInt32					dataSize,
	UInt32*					readCountP)
{
	UInt32					readCount;
	Err						err;

	// Init read count from given data size
	readCount = dataSize;
	// Try to read data
	err = (*readProcP)(dataP, &readCount, userDataP);
	// Add read data to count, regardless of error
	*readCountP += readCount;
	// Return error code
	return err;
}


// -----------------------------------------------------------------------------
//	SkipData
// -----------------------------------------------------------------------------
static inline Err SkipData(
	ExgDBReadProcPtr		readProcP,
	void*					userDataP,
	void*					bufferP,
	UInt32					bufferSize,
	UInt32					skipSize,
	UInt32*					readCountP)
{
	UInt32					readCount;
	Err						err;

	while (skipSize > 0)
		{
		// Init read count from given skip size
		readCount = min(bufferSize, skipSize);
		// Try to read data
		err = (*readProcP)(bufferP, &readCount, userDataP);
		// Add read data to count, regardless of error
		*readCountP += readCount;
		// Return error code
		if (err)
			return err;
			
		skipSize -= readCount;
	}
	return 0;
}


// -----------------------------------------------------------------------------
//	ReadHandleUntilEnd
// -----------------------------------------------------------------------------
// We don't know how much to read, so we keep reading
// until we read less bytes than a whole buffer
static Err ReadHandleUntilEnd(
	ExgDBReadProcPtr		readProcP,
//	ExgDBProgressProcPtr	progressProcP,
	void*					userDataP,
	UInt8 *					readBufP,
	UInt32*					readCountP,
	ResizeHandleProcPtr		resizeProcP,
	void*					resizeInfoP,
	MemHandle*					dataHP)
{
	UInt8 *					dstP;
	UInt32					readSize;
	UInt32					dataSize;
	Err						err = 0;

	// Init previous and current MemHandle size
	dataSize = 0;
	// Read an entire buffer until no more data or error
	while (true)
		{
		// Reset read size
		readSize = 0;
		// Try to read a whole buffer of data
		err = ReadData(readProcP, userDataP, readBufP, kReadBufSize, &readSize);
		// Add bytes actually read to total
		*readCountP += readSize;
		// Exit without error maybe if no data was read this time
		GOTO_IF_NULL(Exit, readSize);
		// Try to resize MemHandle to hold new data
		err = (*resizeProcP)(dataHP, resizeInfoP, dataSize + readSize);
		// Exit if resizing failed
		GOTO_IF_ERR(Exit);
		// Get MemPtr to locked data
		dstP = (UInt8 *)MemHandleLock(*dataHP);
		// Write into data from buffer at last offset
		err = DmWrite(dstP, dataSize, readBufP, readSize);
		// Exit if error writing
		GOTO_IF_ERR(Exit);
		// Advance offset for next write by the bytes read
		dataSize += readSize;
		// Unlock data
		MemHandleUnlock(*dataHP);
		}
Exit:
	// Return error code
	return err;
}


// -----------------------------------------------------------------------------
//	ReadDmData
// -----------------------------------------------------------------------------
static Err ReadDmData(
	ExgDBReadProcPtr		readProcP,
	void*					userDataP,
	UInt8 *					readBufP,
	void*					dataP,
	UInt32					dataSize,
	UInt32*					readCountP)
{
	Err						err = 0;

	// If data to read is > buffer, read it in pieces
	if (dataSize > kReadBufSize)
		{
		UInt32	bytesToRead = dataSize;
		UInt32	bytesRead = 0;

		while (err == 0 && bytesToRead > 0)
			{
			// Split into kReadBufSize bytes or less
			if (bytesToRead > kReadBufSize)
				bytesToRead = kReadBufSize;
			// Read a piece
			err = (*readProcP)(readBufP, &bytesToRead, userDataP);
			GOTO_IF_ERR(Exit);
			
			// hit eof when we should not have done so
			if (bytesToRead == 0)
				{
				GOTO_WITH_ERR(Exit,dmErrCorruptDatabase);
				}
			
			// Transfer data to protected memory
			err = DmWrite(dataP, bytesRead, readBufP, bytesToRead);
			GOTO_IF_ERR(Exit);
			// Adjust bytes read
			bytesRead += bytesToRead;
			// Calc bytes left to read
			bytesToRead = dataSize - bytesRead;
			}
		// Return total read
		*readCountP = bytesRead;
		}
	else
		{
		// Read all data
		err = (*readProcP)(readBufP, &dataSize, userDataP);
		GOTO_IF_ERR(Exit);
		// Transfer data to dm memory
		err = DmWrite(dataP, 0, readBufP, dataSize);
		GOTO_IF_ERR(Exit);
		// Return total read
		*readCountP = dataSize;
		}

Exit:
	// Return error code
	return err;
}


// -----------------------------------------------------------------------------
//	ReadEntryData
// -----------------------------------------------------------------------------
static Err ReadEntryData(
	ExgDBReadProcPtr		readProcP,
//	ExgDBProgressProcPtr	progressProcP,
	void*					userDataP,
	UInt8 *					readBufP,
	DatabaseHdrType*		hdrP,
	UInt32*					readCountP,
	ReadEntryInfoPtr		infoP,
	DmOpenRef				dbRef)
{
	MemHandle				dataH = NULL;	//Bug if totalEntries==0 do not release garbage, Pfz 09/18/00
	UInt32					uniqueID;
	UInt32					offset = 0;
	UInt16					attr;
	UInt16					i;
	Err						err = 0;	//dje- add initializer to fix bug #26001
	Boolean					resDB;

	// Determine if db is a record or resource db
	resDB = ((hdrP->attributes & dmHdrAttrResDB) != 0);

	// Read data for each entry
	for (i = 0; i < infoP->totalEntries; i++)
		{
		// If db has resources
		if (resDB)
			{
			// Try to get resource MemHandle
			dataH = DmGetResourceIndex(dbRef, i);
			// Exit if MemHandle fetch failed
			GOTO_IF_NULL_SET_DM_LAST_ERR(Exit, dataH);
			}
		else
			{
			// Try to get record data MemHandle
			err = GetRecordHandle(&dataH, &uniqueID, &attr, dbRef, i);
			// Exit if error
			GOTO_IF_ERR(Exit);
			}

		// If MemHandle has no data skip it
		if (dataH == NULL)
			continue;
		// If this isnt the last entry
		if (i < infoP->totalEntries - 1)
			{
			// Try to read data to fill MemHandle
			err = ReadHandleData(readProcP, userDataP, readBufP, readCountP, dataH);
			// Exit if error
			GOTO_IF_ERR(Exit);

			// Call progress proc
//			if (progressProcP != NULL)
//				(*progressProcP)(totalExpectedBytes, totalBytes, userDataP);
			}
		else
			{
			// This is the last record
			// If db has resources
			if (resDB)
				// Read resource data until EOF
				err = ReadHandleUntilEnd(readProcP, userDataP, readBufP, readCountP,
									ResizeResourceCallback, NULL, &dataH);
			else
				{
				// Read record data until EOF
				ResizeRecordInfo	info;
				info.dbRef = dbRef;
				info.recIdx = i;
				err = ReadHandleUntilEnd(readProcP, userDataP, readBufP, readCountP,
									ResizeRecordCallback, &info, &dataH);
				}
			// Exit if error
			GOTO_IF_ERR(Exit);
			}
		// If db has resources
		if (resDB)
			{
			// Release resource
			DmReleaseResource(dataH);
			// Last resource was released
			dataH = NULL;
			}
		}
Exit:
	// If db has resources and last res was not released
	if (resDB && (dataH != NULL))
		{
		// Release resource
		DmReleaseResource(dataH);
		}
	// Call progress proc
//	if (progressProcP != NULL)
//		(*progressProcP)(totalExpectedBytes, totalBytes, userDataP);

	// Return error code
	return err;
}


// -----------------------------------------------------------------------------
//	DoForEachReadEntryList
// -----------------------------------------------------------------------------
static Err DoForEachReadEntryList(
	ReadListProcPtr			procP,
	DatabaseHdrType*		hdrP,
	void*					userDataP)
{
	ReadEntryInfoPtr		infoP;
	RecordListType			list;
	Err						err;

	// Get read info MemPtr
	infoP = (ReadEntryInfoPtr)userDataP;
	// Reset number of entries
	infoP->totalEntries = 0;
	// Process each entry in the list
	do
		{
		// Try to read an entry list header
		err = ReadData(infoP->readProcP, infoP->userDataP, &list,
			(sizeof(RecordListType) - sizeof(((RecordListPtr)(NULL))->firstEntry)),
			infoP->readCountP);
		// Exit if error
		GOTO_IF_ERR(Exit);
		// Add number of entries to total entries
		infoP->totalEntries += list.numRecords;
		// Call callback proc with this entry list header
		if (procP != NULL)
			err = (*procP)(list.numRecords, hdrP, userDataP);
		}
	while (err == 0 && list.nextRecordListID != 0);
Exit:
	// Return error code
	return err;
}


// -----------------------------------------------------------------------------
//	CreateRecordEntry
// -----------------------------------------------------------------------------
static inline Err CreateRecordEntry(
	DmOpenRef				dbRef,
	RecordEntryPtr			recEntryP,
	UInt32					entrySize,
	UInt16*					nextEntryIdxP)
{
	MemHandle					newEntryH;
	UInt32					id;
	UInt16					attr;
	Err						err = 0;

	// If the record is 0 size
	if (entrySize == 0)
		{
		// Attach a nil MemHandle to it
		err = DmAttachRecord(dbRef, nextEntryIdxP, NULL, 0);
		// Exit if error
		GOTO_IF_ERR(Exit);
		}
	else
		{
		// Try to allocate exact size record MemHandle
		newEntryH = (MemHandle)DmNewRecord(dbRef, nextEntryIdxP, entrySize);
		// Exit if allocation failed
		GOTO_IF_NULL_SET_DM_LAST_ERR(Exit, newEntryH);
		}
	// Set record attributes
	attr = recEntryP->attributes;
	id = recEntryP->uniqueID[0];
	id = (id << 8) | recEntryP->uniqueID[1];
	id = (id << 8) | recEntryP->uniqueID[2];
	//  Set attributes in record info
	err = DmSetRecordInfo(dbRef, *nextEntryIdxP, &attr, &id);
	// Exit if error
	GOTO_IF_ERR(Exit);
	// Release new record
	err = DmReleaseRecord(dbRef, *nextEntryIdxP, false);
Exit:
	// Return error code
	return err;
}


// -----------------------------------------------------------------------------
//	CreateResourceEntry
// -----------------------------------------------------------------------------
static inline Err CreateResourceEntry(
	DmOpenRef				dbRef,
	RsrcEntryPtr			resEntryP,
	UInt32					entrySize)
{
	MemHandle					newEntryH;
	Err						err = 0;

	// Try to allocate new resource space
	newEntryH = DmNewResource(dbRef, resEntryP->type,
							resEntryP->id, entrySize);
	// Exit if allocation failed
	GOTO_IF_NULL_SET_DM_LAST_ERR(Exit, newEntryH);
	// Release new resource
	err = DmReleaseResource(newEntryH);
Exit:
	// Return error code
	return err;
}


// -----------------------------------------------------------------------------
//	ReadListCallback
// -----------------------------------------------------------------------------
static Err ReadListCallback(
	UInt16					numEntries,
	DatabaseHdrType*		hdrP,
	void*					userDataP)
{
	ReadEntryInfoPtr		infoP;
	RsrcEntryType			prevResEntry;
	RsrcEntryType			curResEntry;
	RecordEntryType			prevRecEntry;
	RecordEntryType			curRecEntry;
	UInt32					curEntryDataOffset;
	UInt32					prevEntryDataSize;
	Int16					idx;
	Err						err = 0;
	Boolean					resDB;

	// Exit if no entries
	GOTO_IF_NULL(Exit, numEntries);

	// Determine if db is a record or resource db
	resDB = ((hdrP->attributes & dmHdrAttrResDB) != 0);
	// Get info MemPtr
	infoP = (ReadEntryInfoPtr)userDataP;

	// Read first entry

	// If db has resources
	if (resDB)
		{
		// Read a resource entry
		err = ReadData(infoP->readProcP, infoP->userDataP, &prevResEntry,
						sizeof(prevResEntry), infoP->readCountP);
		// Get entry data offset
		curEntryDataOffset = prevResEntry.localChunkID;
		}
	else
		{
		// Read a record entry
		err = ReadData(infoP->readProcP, infoP->userDataP, &prevRecEntry,
						sizeof(prevRecEntry), infoP->readCountP);
		// Get entry data offset
		curEntryDataOffset = prevRecEntry.localChunkID;
		}
	// Exit if error
	GOTO_IF_ERR(Exit);

	// Save first entry data offset (but not subsequent firsts)
	if (infoP->firstEntryDataOffset == 0)
		infoP->firstEntryDataOffset = curEntryDataOffset;
	// Save current entry data offset into prev entry data offset
	infoP->prevEntryDataOffset = curEntryDataOffset;

	// Read remaining entries

	// For each remaining entry in entry list
	for (idx = 1; idx < numEntries; idx++)
		{
		// If db has resources
		if (resDB)
			{
			// Read a resource entry
			err = ReadData(infoP->readProcP, infoP->userDataP, &curResEntry,
							sizeof(curResEntry), infoP->readCountP);
			// Get entry data offset
			curEntryDataOffset = curResEntry.localChunkID;
			}
		else
			{
			// Read a record entry
			err = ReadData(infoP->readProcP, infoP->userDataP, &curRecEntry,
							sizeof(curRecEntry), infoP->readCountP);
			// Get entry data offset
			curEntryDataOffset = curRecEntry.localChunkID;
			}
		// Exit if error
		GOTO_IF_ERR(Exit);

		// Exit with error if entry offsets are out of order
		if (curEntryDataOffset < infoP->prevEntryDataOffset)
			{
			GOTO_WITH_ERR(Exit, dmErrCorruptDatabase);
			}

		// Calc previous entry data size
		prevEntryDataSize = curEntryDataOffset - infoP->prevEntryDataOffset;
		// If db has resources
		if (resDB)
			{
			// Try to create previous resource entry
			err = CreateResourceEntry(infoP->dbRef, &prevResEntry,
									prevEntryDataSize);
			// Save current entry into previous entry
			prevResEntry = curResEntry;
			}
		else
			{
			// Try to create previous record entry
			err = CreateRecordEntry(infoP->dbRef, &prevRecEntry,
									prevEntryDataSize, &infoP->nextEntryIdx);
			// Save current entry into previous entry
			prevRecEntry = curRecEntry;
			// Advance next entry index
			++infoP->nextEntryIdx;
			}
		// Exit if error
		GOTO_IF_ERR(Exit);
		// Save current entry data offset into previous entry data offset
		infoP->prevEntryDataOffset = curEntryDataOffset;
		}

		// Create last entry (or possibly first and only one) with a
		// bogus size of 1

		// If db has resources
		if (resDB)
			{
			// Try to create previous resource entry
			err = CreateResourceEntry(infoP->dbRef, &prevResEntry, 1);
			}
		else
			{
			// Try to create previous record entry
			err = CreateRecordEntry(infoP->dbRef, &prevRecEntry,
									1, &infoP->nextEntryIdx);
			}
Exit:
	// Return error code
	return err;
}


// -----------------------------------------------------------------------------
//	ReadEntryLists
// -----------------------------------------------------------------------------
//	Reads the list entries
static inline Err ReadEntryLists(
	ExgDBReadProcPtr		readProcP,
//	ExgDBProgressProcPtr	progressProcP,
	void*					userDataP,
	DatabaseHdrType*		hdrP,
	UInt32*					readCountP,
	ReadEntryInfoPtr		infoP,
	DmOpenRef				dbRef,
	UInt16					cardNo)
{
	// Init info
	infoP->readProcP = readProcP;
//	infoP->progressProcP = progressProcP;
	infoP->userDataP = userDataP;
	infoP->readCountP = readCountP;
	infoP->firstEntryDataOffset = 0;
	infoP->nextEntryIdx = 0;
	infoP->dbRef = dbRef;
	infoP->cardNo = cardNo;

	// Read each list in db and return error
	return DoForEachReadEntryList(ReadListCallback, hdrP, infoP);
}


// -----------------------------------------------------------------------------
//	ReadInfoHandleData
// -----------------------------------------------------------------------------
static Err ReadInfoHandleData(
	ExgDBReadProcPtr		readProcP,
//	ExgDBProgressProcPtr	progressProcP,
	void*					userDataP,
	UInt8 *					readBufP,
	UInt32*					readCountP,
	DmOpenRef				dbRef,
	MemHandle*				infoHP,
	ReadEntryInfoPtr		infoP,
	UInt32					infoOffset)
{
	UInt16					recSize;
	Err						err;

	// If db has at least one entry
	if (infoP->totalEntries > 0)
		{
		// Use first entry to calc info size
		recSize = infoP->firstEntryDataOffset - infoOffset;
		// Try to allocate proper size info block
		*infoHP = DmNewHandle(dbRef, recSize);
		// Exit if allocation failed
		// DOLATER: Calling DmGetLastErr is invalid here
		// because the lastErr is not set by DmNewHandle.
		GOTO_IF_NULL_SET_DM_LAST_ERR(Exit, *infoHP);
		// Try to read exact MemHandle data
		err = ReadHandleData(readProcP, userDataP, readBufP, readCountP, *infoHP);
		}
	else
		{
		// Try to allocate info block
		ResizeHandleInfo	info;
		// Setup resize MemHandle callback info
		info.dbRef = dbRef;
		// Create new stub MemHandle
		*infoHP = DmNewHandle(dbRef, 1);
		// Exit if allocation failed
		// DOLATER: Calling DmGetLastErr is invalid here
		// because the lastErr is not set by DmNewHandle.
		GOTO_IF_NULL_SET_DM_LAST_ERR(Exit, *infoHP);
		// Read info until EOF
		err = ReadHandleUntilEnd(readProcP, userDataP, readBufP, readCountP,
							ResizeHandleCallback, infoP, infoHP);
		}

	// Free new MemHandle if error
	if (err != 0 && *infoHP != NULL)
		{
		MemHandleFree(*infoHP);
		*infoHP = NULL;
		}
Exit:
	// Return error code
	return err;
}


// -----------------------------------------------------------------------------
//	ReadDatabase
// -----------------------------------------------------------------------------
static Err ReadDatabase(
	ExgDBReadProcPtr		readProcP,
//	ExgDBProgressProcPtr	progressProcP,
	void*					userDataP,
	UInt8 *					readBufP,
	DatabaseHdrType*		hdrP,
	UInt32*					readCountP,
	DmOpenRef				dbRef,
	LocalID					dbID,
	UInt16					cardNo)
{
	ReadEntryInfo			info;
	MemHandle					newRecH = NULL;
	LocalID					appInfoID;
	LocalID					sortInfoID;
	UInt16					recSize;
	UInt16					filler;
	Err						err;

	// Try to read entry lists
//	err = ReadEntryLists(readProcP, progressProcP, userDataP, hdrP,
	err = ReadEntryLists(readProcP, userDataP, hdrP,
						readCountP, &info, dbRef, cardNo);
	// Exit if error
	GOTO_IF_ERR(Exit);


	// Figure out how many filler bytes there are
	if (hdrP->appInfoID)
		recSize = (UInt32)hdrP->appInfoID - *readCountP;
	else if (hdrP->sortInfoID)
		recSize = (UInt32)hdrP->sortInfoID - *readCountP;
	else if (info.totalEntries > 0)
		recSize = info.firstEntryDataOffset - *readCountP;
	else
		recSize = 0;
	
 	// Try to read filler bytes
	err = SkipData(readProcP, userDataP, &filler, sizeof(filler), recSize, readCountP);

	// Exit if error
	GOTO_IF_ERR(Exit);

	// Read app info if present in db 
	if (hdrP->appInfoID != 0)
		{
		// If db also has sort info
		if (hdrP->sortInfoID != 0)
			{
			// Exit with error if sort info comes before app-info
			if (hdrP->sortInfoID < hdrP->appInfoID)
				{
				GOTO_WITH_ERR(Exit, dmErrCorruptDatabase);
				}

			// Calc app info size as delta from sort info offset
			recSize = (UInt32)hdrP->sortInfoID - (UInt32)hdrP->appInfoID;
			// Try to allocate proper app info block
			newRecH = DmNewHandle(dbRef, recSize);
			// Exit if allocation failed
			GOTO_IF_NULL_SET_DM_LAST_ERR(Exit, newRecH);
			// Try to read exact MemHandle data
			err = ReadHandleData(readProcP, userDataP, readBufP, readCountP, newRecH);
			}
		else
			{
			// Exit with error if first record comes before app-info
			if ((info.totalEntries > 0) && (info.firstEntryDataOffset < hdrP->appInfoID))
				{
				GOTO_WITH_ERR(Exit, dmErrCorruptDatabase);
				}

			// Try to read app info data
//			err = ReadInfoHandleData(readProcP, progressProcP, userDataP, readBufP,
			err = ReadInfoHandleData(readProcP, userDataP, readBufP,
					readCountP, dbRef, &newRecH, &info, hdrP->appInfoID);
			}
		// Exit if error
		GOTO_IF_ERR(Exit);
		// Set app info id from MemHandle id
		appInfoID = MemHandleToLocalID(newRecH);
		// Store app info id in db header
		DmSetDatabaseInfo(cardNo, dbID, 0, 0, 0, 0, 0, 0, 0,
						&appInfoID, 0, 0, 0);
		// MemHandle belongs to db now
		newRecH = NULL;
		}

	// Read sort info if present in db 
	if (hdrP->sortInfoID != 0)
		{
		// Exit with error if first record comes before sort-info
		if ((info.totalEntries > 0) && (info.firstEntryDataOffset < hdrP->sortInfoID))
			{
			GOTO_WITH_ERR(Exit, dmErrCorruptDatabase);
			}

		// Try to read sort info data
//		err = ReadInfoHandleData(readProcP, progressProcP, userDataP, readBufP,
		err = ReadInfoHandleData(readProcP, userDataP, readBufP,
				readCountP, dbRef, &newRecH, &info, hdrP->sortInfoID);
		// Exit if error
		GOTO_IF_ERR(Exit);
		// Set sort info id from MemHandle id
		sortInfoID = MemHandleToLocalID(newRecH);
		// Store sort info id in db header
		DmSetDatabaseInfo(cardNo, dbID, 0, 0, 0, 0, 0, 0, 0, 0,
						&sortInfoID, 0, 0);
		// MemHandle belongs to db now
		newRecH = NULL;
		}

	// Try to read all record/resource entry data
//	err = ReadEntryData(readProcP, progressProcP, userDataP, readBufP, hdrP,
	err = ReadEntryData(readProcP, userDataP, readBufP, hdrP,
					readCountP, &info, dbRef);
Exit:
	// Free lingering new MemHandle if present
	if (newRecH != NULL)
		MemHandleFree(newRecH);
	// Call progress proc
//	if (progressProcP != NULL)
//		(*progressProcP)(totalExpectedBytes, totalBytes, userDataP);
	// Return error code
	return err;
}


// -----------------------------------------------------------------------------
//	ExgDBRead
// -----------------------------------------------------------------------------
//	PARAMETERS:
//		readProcP		- Write callback proc MemPtr
//		progressProcP	- Progress callback proc MemPtr
//		userDataP		- User data MemPtr send to callback procs
//		cardNo			- Card number of database
//		keepDates		- True to preserve dates of db being read
//	RETURNED:
//		0 if no err
//	
//	REVISION HISTORY:
//	jesse	10/13/99	Fixed memory leak in case where user doesn't delete the 
//						existing database where we were "return 0" instead of 
//						"goto Exit", so the read buffer wasn't being freed.
// lrt	07/25/00	Fixed Bug where an already existing database would get 
//						deleted on cleanup (but dmErrAlreadyExists was returned) if 
//						the delete callback returned true without actually deleting
//						the database. needed to avoid improper re-use of dbID
Err ExgDBRead(
	ExgDBReadProcPtr		readProcP,
//	ExgDBProgressProcPtr	progressProcP,
	ExgDBDeleteProcPtr		deleteProcP,
	void*					userDataP,
	LocalID*				dbIDP,
	UInt16					cardNo,
	Boolean*				needResetP,
	Boolean					keepDates)
{
	DmOpenRef				dbRef = NULL;
	DatabaseHdrType		*hdrP = NULL;
	UInt8 *					readBufP;
	UInt32					totalBytes = 0;
	LocalID					dbID = 0;
	Err						err;
	int						idx;
	
	// Try to allocate database header buffer:
	hdrP = MemPtrNew(sizeof(DatabaseHdrType));
	GOTO_IF_NULL_SET_ERR(Exit, hdrP, memErrNotEnoughSpace);
	
	// Try to allocate a buffer for reading
	readBufP = MemPtrNew(kReadBufSize);
	// Exit if error
	GOTO_IF_NULL_SET_ERR(Exit, readBufP, memErrNotEnoughSpace);

	// Try to read header data
	err = ReadData(readProcP, userDataP, hdrP,
					sizeof(DatabaseHdrType) - sizeof(hdrP->recordList), &totalBytes);
	// Exit if error
	GOTO_IF_ERR(Exit);

	// Verify as much header data as possible now

	// Check name. Must be <= 32 chars including terminator
	for (idx=0; idx < dmDBNameLength; idx++)
		if (hdrP->name[idx] == '\0')
			break;
	
	if (idx == dmDBNameLength)
		{
		// Name is too long. Exit with error
		GOTO_WITH_ERR(Exit, dmErrCorruptDatabase);
		}

	// Verification passed

	// Try to find old db with same name
	dbID = DmFindDatabase(cardNo, (char*)hdrP->name);

	// If duplicate found
	if (dbID != 0)
		{
		// If delete proc given
		if (deleteProcP != NULL)
			{
			// If caller doesnt delete existing db, exit without error
			if (!(*deleteProcP)((const char*)hdrP->name, hdrP->version,
									cardNo, dbID, userDataP))
				{
				// Dont need to reset since nothing read
				if (needResetP != NULL)
					*needResetP = false;
				// Clear db id. This implies that no db was read.
				*dbIDP = 0;
				// Return with no error. Since no db was read, this
				// will be taken to imply the user cancelled, otherwise
				// there would have been an error.
				err = errNone;
				goto Exit;
				}
			}
		else
			{
			// Try to delete duplicate db
			err = DmDeleteDatabase(cardNo, dbID);
			// Exit if error
			GOTO_IF_ERR(Exit);
			}
		}
	
	// in case DmCreateDatabase returns dmErrAlreadyExists because the callback
	// returned true without actually deleting the database
	dbID = 0;

	// Try to create new db
	err = DmCreateDatabase(cardNo, (char*)hdrP->name, hdrP->creator, hdrP->type, 
				(hdrP->attributes & dmHdrAttrResDB) != 0);
	// Exit if error
	GOTO_IF_ERR(Exit);
	// Try to get new db id
	dbID = DmFindDatabase(cardNo, (char*)hdrP->name);
	// Exit if db id fetch failed
	GOTO_IF_ZERO_SET_DM_LAST_ERR(Exit, dbID);

	// Try to open db for read/write
	dbRef = DmOpenDBNoOverlay(cardNo, dbID, dmModeReadWrite);
	// Exit if db open failed
	GOTO_IF_NULL_SET_DM_LAST_ERR(Exit, dbRef);
	// Try to read rest of db
//	err = ReadDatabase(readProcP, progressProcP, userDataP, readBufP,
	err = ReadDatabase(readProcP, userDataP, readBufP,
						hdrP, &totalBytes, dbRef, dbID, cardNo);
	// Exit if error
	GOTO_IF_ERR(Exit);
	
	// If not preserving dates, clear them
	if (!keepDates)
		hdrP->creationDate = hdrP->modificationDate = hdrP->lastBackupDate = 0;
	// Try to set db info
	// Do this AFTER we write the data, so we can write to readOnly databases....
	err = DmSetDatabaseInfo(cardNo, dbID, 0, &hdrP->attributes, &hdrP->version,
				&hdrP->creationDate, &hdrP->modificationDate, &hdrP->lastBackupDate,
				&hdrP->modificationNumber, 0, 0, &hdrP->type, &hdrP->creator);

Exit:
	// Close db if open
	if (dbRef != NULL)
		DmCloseDatabase(dbRef);
	// Delete db if error
	if (err != 0)
		{
		if (dbID != 0)
			{
			// Kill new db and clear id
			DmDeleteDatabase(cardNo, dbID);
			dbID = 0;
			}
		}

	// Return db id
	*dbIDP = dbID;

	// Determine if db needs reset after reading
	if (needResetP != NULL)
		*needResetP = (hdrP->attributes & dmHdrAttrResetAfterInstall) != 0;

	// Free read buffer
	if (readBufP != NULL)
		MemPtrFree(readBufP);

	// Free db header buffer
	if (hdrP != NULL)
		MemPtrFree(hdrP);

	// Return error code
	return err;
}


#pragma mark ---------------------------
// -----------------------------------------------------------------------------
//	DoForEachWriteEntryList
// -----------------------------------------------------------------------------
static Err DoForEachWriteEntryList(
	const WriteListProcPtr		procP,
	const DatabaseHdrType*	hdrP,
	void*					userDataP,
	UInt16					cardNo)
{
	const RecordListType*	listP;
	LocalID					id;
	Err						err = 0;

	// Accumulate the sizes of every chunk
	listP = &hdrP->recordList;
	while (true) {
		// Call callback proc with this record list
		if (procP != NULL)
			err = (*procP)(listP, hdrP, userDataP);
		// Unlock this record list if it's not in the db header
		if (listP != &hdrP->recordList)
			MemPtrUnlock((RecordListType*)listP);
		// Get next record list id
		id = listP->nextRecordListID;
		// Exit loop if error or last list
		if (err != 0 || id == 0)
			break;
		// Get MemPtr to next record list
		listP = MemLocalIDToLockedPtr(id, cardNo);
		}
	// Return error code
	return err;
}


// -----------------------------------------------------------------------------
//	CountEntriesCallback
// -----------------------------------------------------------------------------
static Err CountEntriesCallback(
	const RecordListType*	listP,
	const DatabaseHdrType*	/* hdrP */,
	void*					userDataP)
{
	CountRecordListsInfoPtr	infoP = (CountRecordListsInfoPtr)userDataP;

	// Add this record list to num of record lists found
	++(infoP->numOfRecordLists);
	// Add number of entries to total
	infoP->numOfEntries += listP->numRecords;
	// Return no error
	return 0;
}


// -----------------------------------------------------------------------------
//	CalcEntryInfo
// -----------------------------------------------------------------------------
static Err CalcEntryInfo(
	const DatabaseHdrType*	hdrP,
	CountRecordListsInfoPtr	infoP,
	UInt16					cardNo)
{
	UInt16					entrySize;
	UInt16					filler = 0;
	Err						err;

	// Init info
	infoP->numOfRecordLists = 0;
	infoP->numOfEntries = 0;
	// Count record lists in db
	err = DoForEachWriteEntryList(CountEntriesCallback, hdrP, infoP, cardNo);
	// Exit if error
	GOTO_IF_ERR(Exit);
	// Add size of each record list to total size
	infoP->firstUsableOffset = (sizeof(RecordListType) - sizeof(((RecordListPtr)(NULL))->firstEntry));
	// Determine whether to use record or resource entry size
	entrySize = ((hdrP->attributes & dmHdrAttrResDB) != 0) ?
				sizeof(RsrcEntryType) : sizeof(RecordEntryType);
	// Add size of each entry
	infoP->firstUsableOffset += infoP->numOfEntries * entrySize;
	// Add db header (minus first record list) size to offset
	infoP->firstUsableOffset += sizeof(DatabaseHdrType) - sizeof(RecordListType);
	// Add filler size to offset
	infoP->firstUsableOffset += sizeof(filler);

Exit:
	// Return error code
	return err;
}


// -----------------------------------------------------------------------------
//	WriteData
// -----------------------------------------------------------------------------
static inline Err WriteData(
	ExgDBWriteProcPtr		writeProcP,
	void*					userDataP,
	const void*				dataP,
	UInt32					dataSize,
	UInt32*					writeCountP)
{
	UInt32					writeCount;
	Err						err;

	// Init write count from given data size
	writeCount = dataSize;
	// Try to write data
	err = (*writeProcP)(dataP, &writeCount, userDataP);
	// Add written data to count, regardless of error
	*writeCountP += writeCount;
	// Return error code
	return err;
}


// -----------------------------------------------------------------------------
//	WriteChunkData
// -----------------------------------------------------------------------------
static Err WriteChunkData(
	ExgDBWriteProcPtr		writeProcP,
//	ExgDBProgressProcPtr	progressProcP,
	void*					userDataP,
	UInt32*					writeCountP,
	LocalID					chunkID,
	UInt16					cardNo)
{
	MemPtr						chunkP;
	UInt32					chunkSize;
	Err						err = 0;

	// If chunk ID valid
	if (chunkID != 0)
		{
		chunkP = MemLocalIDToLockedPtr(chunkID, cardNo);
		// Get chunk size
		chunkSize = MemPtrSize(chunkP);
		// Try to write data
		err = (*writeProcP)(chunkP, &chunkSize, userDataP);
		// Add bytes written to total
		*writeCountP += chunkSize;
		// Unlock chunk
		MemPtrUnlock(chunkP);
		}

	// Return error code
	return err;
}


// -----------------------------------------------------------------------------
//	WriteListCallback
// -----------------------------------------------------------------------------
static Err WriteListCallback(
	const RecordListType*	listP,
	const DatabaseHdrType*	hdrP,
	void*					userDataP)
{
	RsrcEntryType			resEntry;
	RecordEntryType			recEntry;
	const RsrcEntryType*	resEntryP;
	const RecordEntryType*	recEntryP;
	WriteEntryInfoPtr		infoP;
	UInt32					entrySize;
	Int16					idx;
	Err						err;
	Boolean					resDB;

	// Determine if db is a record or resource db
	resDB = ((hdrP->attributes & dmHdrAttrResDB) != 0);

	// Get info MemPtr
	infoP = (WriteEntryInfoPtr)userDataP;

	if (resDB)
		{
		// Get MemPtr to first resource entry
		resEntryP = (const RsrcEntryType*)&listP->firstEntry;
		// For each resource entry in the resource list
		for (idx = 0; idx < listP->numRecords; idx++)
			{
			// Make a copy of record entry
			resEntry = *resEntryP;
			// Get size of entry's record
			GetChunkSize(resEntry.localChunkID, &entrySize, infoP->cardNo);
			// Adjust record entry fields
			resEntry.localChunkID = infoP->nextEntryDataOffset;
			// Try to write record entry
			err = WriteData(infoP->writeProcP, infoP->userDataP, &resEntry,
							sizeof(resEntry), infoP->writeCountP);
			// Exit if error
			GOTO_IF_ERR(Exit);
			// Advance next record offset by size of this record
			infoP->nextEntryDataOffset += entrySize;
			// Advance resource entry MemPtr
			++resEntryP;
			}
		}
	else
		{
		// Get MemPtr to first record entry
		recEntryP = (const RecordEntryType*)&listP->firstEntry;
		// For each record entry in the record list
		for (idx = 0; idx < listP->numRecords; idx++)
			{
			// Make a copy of record entry, including unique ID
			recEntry = *recEntryP;
			// Get size of entry's record
			GetChunkSize(recEntry.localChunkID, &entrySize, infoP->cardNo);
			// Adjust record entry fields
			recEntry.localChunkID = infoP->nextEntryDataOffset;
			// Try to write record entry
			err = WriteData(infoP->writeProcP, infoP->userDataP, &recEntry,
							sizeof(recEntry), infoP->writeCountP);
			// Exit if error
			GOTO_IF_ERR(Exit);
			// Advance next record offset by size of this record
			infoP->nextEntryDataOffset += entrySize;
			// Advance record entry MemPtr
			++recEntryP;
			}
		}
Exit:
	// Return error code
	return err;
}


// -----------------------------------------------------------------------------
//	WriteEntryLists
// -----------------------------------------------------------------------------
//	Writes the list entries
static inline Err WriteEntryLists(
	ExgDBWriteProcPtr		writeProcP,
//	ExgDBProgressProcPtr	progressProcP,
	void*					userDataP,
	const DatabaseHdrType*	hdrP,
	UInt32*					writeCountP,
	UInt32					firstEntryDataOffset,
	UInt16					cardNo,
	UInt16					totalRecordCount)
{
	RecordListType			list;
	WriteEntryInfo			info;
	Err						err;
	
	list.nextRecordListID = 0;
	list.numRecords = totalRecordCount;
	
	// Try to write entry list header
	err = WriteData(writeProcP, userDataP, &list,
	(sizeof(RecordListType) - sizeof(((RecordListPtr)(NULL))->firstEntry)),
		writeCountP);
	// Exit if error
	GOTO_IF_ERR(Exit);

	// Init info
	info.writeProcP = writeProcP;
//	info.progressProcP = progressProcP;
	info.userDataP = userDataP;
	info.writeCountP = writeCountP;
	info.nextEntryDataOffset = firstEntryDataOffset;
	info.cardNo = cardNo;

	// Write each resource list in db and return error
 	return DoForEachWriteEntryList(WriteListCallback, hdrP, &info, cardNo);

Exit:
	return err;
}


// -----------------------------------------------------------------------------
//	WriteDatabase
// -----------------------------------------------------------------------------
static Err WriteDatabase(
	ExgDBWriteProcPtr		writeProcP,
//	ExgDBProgressProcPtr	progressProcP,
	void*					userDataP,
	const DatabaseHdrType*	hdrP,
	UInt32*					writeCountP,
	DmOpenRef				dbRef,
	UInt16					cardNo)
{
	DatabaseHdrType			hdr;
	CountRecordListsInfo	info;
	LocalID					chunkID;
	Int16					i;
	UInt16					filler = 0;
	Err						err;
	Boolean					resDB;

	// Determine if db is a record or resource db
	resDB = ((hdrP->attributes & dmHdrAttrResDB) != 0);

	// Try to calc db entry list info
	err = CalcEntryInfo(hdrP, &info, cardNo);

	// Make a copy of the header
	hdr = *hdrP;
	// Clear unique id seed
	hdr.uniqueIDSeed = 0;
	// Clear db open attribute bit
	hdr.attributes &= ~dmHdrAttrOpen;

	if (resDB)
		{
		// Clear app info ID
		hdr.appInfoID = 0;
		// Clear sort info ID
		hdr.sortInfoID = 0;
		}
	else
		{
		UInt32		appInfoSize;
		UInt32		sortInfoSize;
		// Try to get app info size
		GetChunkSize(hdr.appInfoID, &appInfoSize, cardNo);
		// Try to get sort info size
		GetChunkSize(hdr.sortInfoID, &sortInfoSize, cardNo);
		// Convert app info ID to offset
		if (hdr.appInfoID != 0)
			hdr.appInfoID = info.firstUsableOffset;
		// Add app info size to offset
		info.firstUsableOffset += appInfoSize;
		// Convert sort info ID to offset
		if (hdr.sortInfoID != 0)
			hdr.sortInfoID = info.firstUsableOffset;
		// Add sort info size to offset
		info.firstUsableOffset += sortInfoSize;
		}
	
	// Try to write db header minus first record list entry
	err = WriteData(writeProcP, userDataP, &hdr,
					sizeof(hdr) - sizeof(hdr.recordList), writeCountP);
	// Exit if error
	GOTO_IF_ERR(Exit);

	// Try to write entry lists
//	err = WriteEntryLists(writeProcP, progressProcP, userDataP,
	err = WriteEntryLists(writeProcP, userDataP,
							hdrP, writeCountP, info.firstUsableOffset, cardNo, info.numOfEntries);
	// Exit if error
	GOTO_IF_ERR(Exit);

	// Try to write filler data
	err = WriteData(writeProcP, userDataP, &filler,
					sizeof(filler), writeCountP);
	// Exit if error
	GOTO_IF_ERR(Exit);

	if (!resDB)
		{
		// Write app Info if present
//		err = WriteChunkData(writeProcP, progressProcP, userDataP,
		err = WriteChunkData(writeProcP, userDataP,
							writeCountP, hdrP->appInfoID, cardNo);
		// Exit if error
		GOTO_IF_ERR(Exit);
		// Write sort Info if present
//		err = WriteChunkData(writeProcP, progressProcP, userDataP,
		err = WriteChunkData(writeProcP, userDataP,
							writeCountP, hdrP->sortInfoID, cardNo);
		// Exit if error
		GOTO_IF_ERR(Exit);
		}

	// Write each entry's data
	for (i = 0; i < info.numOfEntries; i++)
		{
		// Try to get resource or record info
		if (resDB)
			err = DmResourceInfo(dbRef, i, NULL, NULL, &chunkID);
		else
			err = DmRecordInfo(dbRef, i, NULL, NULL, &chunkID);
		// Exit if error
		GOTO_IF_ERR(Exit);
		// Try to write record data
//		err = WriteChunkData(writeProcP, progressProcP, userDataP,
		err = WriteChunkData(writeProcP, userDataP,
							writeCountP, chunkID, cardNo);
		// Exit if error
		GOTO_IF_ERR(Exit);
		}
Exit:
	// Return error code
	return err;
}


// -----------------------------------------------------------------------------
//	ExgDBWrite
// -----------------------------------------------------------------------------
//	Writes a given Pilot db in its internal format using the given callbacks.
//	If a db ID is not given, the given name will be used to search for it.
//
//	PARAMETERS:
//		writeProcP		- Write callback proc MemPtr
//		progressProcP	- Progress callback proc MemPtr
//		userDataP		- User data MemPtr send to callback procs
//		nameP			- Name of database to send
//		dbID			- ID of database to write
//		cardNo			- Card number of database
//	RETURNED:
//		0 if no err
Err ExgDBWrite(
	ExgDBWriteProcPtr		writeProcP,
//	ExgDBProgressProcPtr	progressProcP,
	void*					userDataP,
	const char*				nameP,
	LocalID					dbID,
	UInt16					cardNo)
{
	DatabaseHdrPtr			hdrP = NULL;
	DmOpenRef				dbRef = NULL;
	UInt32					totalBytes = 0;
	Err						err = 0;

	// If db id not given
	if (dbID == 0)
		{
		// Try to find the db by name
		dbID = DmFindDatabase(cardNo, (char*)nameP);
		// Exit if error
		GOTO_IF_ZERO_SET_DM_LAST_ERR(Exit, dbID);
		}
	// Try to open db for read using db id
	dbRef = DmOpenDatabase(cardNo, dbID, dmModeReadOnly);
	// Exit if error
	GOTO_IF_NULL_SET_DM_LAST_ERR(Exit, dbRef);
	// Try to get MemPtr to locked db header
	hdrP = MemLocalIDToLockedPtr(dbID, cardNo);
	// Try to write the rest of the db
//	err = WriteDatabase(writeProcP, progressProcP, userDataP,
	err = WriteDatabase(writeProcP, userDataP,
					hdrP, &totalBytes, dbRef, cardNo);
Exit:
	// Unlock db header MemPtr if locked
	if (hdrP != NULL)
		MemPtrUnlock(hdrP);
	// Close db if opened
	if (dbRef != NULL)
		DmCloseDatabase(dbRef);
	// Return error code
	return err;
}
