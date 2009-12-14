/******************************************************************************
 *
 * Copyright (c) 1994-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: DataMgr.c
 *
 * Release: 
 *
 * Description:
 *		Data Management routines
 *
 * History:
 *   	11/14/94	RM		Created by Ron Marianetti
 *		09/30/99	kwk	Modified to use one DB directory store, in the
 *							RAM store (1), which includes ROM store (0) info.
 *		05/14/00 vsm	Renamed PrvDBNameCompare to StrCompareAscii and moved
 *							it to StringMgr.c.
 *
 *****************************************************************************/

#define	NON_PORTABLE

#include <PalmTypes.h>

// public system includes
#include <AlarmMgr.h>
#include <ErrorMgr.h>
#include <FeatureMgr.h>
#include <OverlayMgr.h>
#include <StringMgr.h>
#include <SysEvtMgr.h>
#include <SysUtils.h>
#include <SystemMgr.h>
#include <TimeMgr.h>

// private system includes
#include "SystemPrv.h"
#include "DataPrv.h"
#include "SysTrapsFastPrv.h"
#include "NotifyPrv.h"
#include	"OverlayPrv.h"		// For OmOpenOverlayDatabase

/************************************************************
 * Private constants used only in this module
 *************************************************************/
#define	overlayMode		(dmModeLeaveOpen | dmModeExclusive)


// dmMaxRecordsInDB - the maximum number of records in a database.
// Index values are UInt16 and so is numOfRecords stored in database
// header.  This means numOfRecords can be at most 65535 (0xFFFF)
// and the valid index values are 0-65534 (0-0xFFFE).
// So 0xFFFF is an invalid index value (0xFFFF is also (UInt16)-1).
// DOLATER - DataMgr.h defines the constant dmMaxRecordIndex to be 0xFFFF
// which is really an invalid index.  This should be changed for consistency
#define dmLastValidRecIndex 	(65534)				// 0xFFFE
#define dmMaxRecordsInDB   		(0xFFFF)
#define dmInvalidRecIndex		((UInt16)(-1))		// 0xFFFF



/************************************************************
 * Private routines used only in this module
 *************************************************************/
static void  				PrvSetUniqueID(DatabaseHdrPtr hdrP, RecordEntryPtr recordP);

static LocalID				PrvFindDatabaseInStore(UInt16 cardNo, UInt16 storeNum,
									const Char* nameP);
		
static RecordEntryPtr	PrvRecordEntryPtr(DmOpenInfoPtr openP, UInt16 index, 
									RecordEntryPtr	prevRecordP);

static RecordListPtr 	PrvGetEntryArrayByIndex(DmOpenInfoPtr openP, UInt16 arrayIndex);
static Boolean 			PrvGetRsrcEntryArrayBounds( DmOpenInfoPtr openP, UInt16 whichArray, 
                        	RsrcEntryPtr *arrayStart, RsrcEntryPtr *arrayEnd );
static Boolean          PrvGetRecordEntryArrayBounds( DmOpenInfoPtr openP, UInt16 whichArray, 
               	            RecordEntryPtr *arrayStart, RecordEntryPtr *arrayEnd );

static RsrcEntryPtr		PrvRsrcEntryPtr(DmOpenInfoPtr openP, UInt16 index,
									RsrcEntryPtr	prevRsrcP);
		
static Err					PrvShiftEntriesUp(DmOpenRef dbR, UInt16 startIndex, 
									UInt16 shiftSize, Boolean adjustSize);

static Err					PrvShiftEntriesDown(DmOpenRef dbR, UInt16 startIndex, 
									UInt16 shiftSize, Boolean adjustSize);

static RecordListPtr		PrvPrevRecordList(DmOpenInfoPtr openP, RecordListPtr currentP);
		
static Err					PrvMergeFree(UInt16	cardNo, UInt32 reqSize);

static MemHandle				PrvDmHandleRealloc(DmOpenRef dbR, MemHandle oldH, 
									MemHandle newH, UInt32 size);

static UInt16 				PrvBinarySearch (const DatabaseDirType* dirP, Int32 cardNo,
									 UInt32 type, UInt32 creator, UInt16 version, Boolean ramDB);

static Err 					PrvDatabaseIsProtected (UInt16 cardNo, LocalID dbID);

static Err					PrvCloseDatabase(DmOpenRef dbR, DmOpenRef* prevDB);

static Err					PrvMoveOpenDBContext(DmAccessPtr* dstHeadP, DmAccessPtr dbP, DmAccessPtr* prevDB);

static MemHandle			PrvDmNewHandle(DmOpenRef dbR, UInt32 size);

static DatabaseDirPtr	PrvAddEntryToDBList(DatabaseDirPtr dirP, DatabaseHdrPtr headerP,
									UInt16 card, LocalID baseID);

static Err PrvSyncLaunchDatabase(void);
static Boolean PrvIsLauncherVisible (UInt32 dbType, UInt32 dbCreator, UInt16 dbAttrs);
static void PrvUpdateLaunchDBEntry(DmOpenRef dbRef, DatabaseHdrPtr hdrP, LocalID dbID, UInt16 *attrP);
static Int16 PrvLaunchCompareFunc(void const * searchData, void const * dbData, Int32 other);
static Boolean PrvSearchDatabase (DmOpenRef dbRef, 
	DmSearchFuncPtr searchF, void const * searchData, const Int32 other, 
	UInt16 * position);
static Int16		PrvRandom(Int32 newSeed);
static void PrvSetLaunchRecordFlags(UInt8 setAttrs, DmOpenRef dbRef, UInt32 type, UInt32 creator, char* name);
static UInt16 PrvRemoveFromLaunchDatabase(DmOpenRef dbRef, UInt32 type, UInt32 creator, char* name, UInt16 dbAttr);
static DmOpenRef PrvOpenLaunchDatabase(void);
static void PrvKillLaunchDBSortInfo(DmOpenRef launchDBRef);
static UInt32 PrvCreateCompositeAppVersion (const DmLaunchDBRecordType* recP);
static Int16 PrvLaunchCompareVers (const DmLaunchDBRecordType* rec1P,
								const DmLaunchDBRecordType* rec2P);

// This declaration is used for performing link-time assertions
static void ThisShouldGenerateALinkErrorDataMgr(void);

static void PrvSetLastErr( Err newLastError );
#define PrvClearLastErr()  PrvSetLastErr( errNone )

/***********************************************************************
 *
 * FUNCTION: PrvRandom	 
 *
 * DESCRIPTION:	Returns a random number anywhere from 0 to sysRandomMax.
 *						Copied from SysRandom until I can figure out what to do 
 *						about the fact that SysRandom isn't installed when DmInit()
 *						is called... move SysRandom to SystemMgr? Have a local copy here?
 *						
 * PARAMETERS:		
 *		newSeed		- new seed value, or 0 to use existing seed
 *
 * RETURNS:			Random number
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			jesse	11/1/99	Copied from SysRandom...
 *
 ***********************************************************************/
Int16		PrvRandom(Int32 newSeed)
{
	
	// Set the new seed, if passed
	if (newSeed) GSysRandomSeed = newSeed;
	
	// Generate the new seed
	GSysRandomSeed = (0x015A4E35L * GSysRandomSeed) + 1;
	
	// Return the random number
	return (Int16) ((GSysRandomSeed >> 16) & 0x7FFF);
}


/************************************************************
 * Check functions
 *************************************************************/

#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL

#include <DebugMgr.h>		// For DbgMessage calls

static void	PrvValidateDmAccessPtr (DmAccessPtr dbP);
static void PrvValidateResourceDB (DmAccessPtr dbP);
static void PrvValidateRamDBList (LocalID ramDirID, LocalID romDirID, UInt16 card);

#define ECDmValidateDmOpenRef(dbAccessPtr)	PrvValidateDmAccessPtr (dbAccessPtr)
#define ECDmValidateResourceDB(dbAccessPtr)	PrvValidateResourceDB (dbAccessPtr)
#define ECDmValidateRamDBList(ramDirID, romDirID, card)	PrvValidateRamDBList(ramDirID, romDirID, card)

#define ErrNonFatalDebuggerIf(condition, msg) \
	do {if (condition) DbgMessage(msg);} while (0)

#else

#define ECDmValidateDmOpenRef(dbAccessPtr)
#define ECDmValidateResourceDB(dbAccessPtr)
#define ECDmValidateRamDBList(ramDirID, romDirID, card)
#define ErrNonFatalDebuggerIf(condition, msg) 

#endif


/************************************************************
 *
 *	FUNCTION:	PrvDatabaseIsProtected
 *
 *	DESCRIPTION: Return if a database is protected
 *
 *	PARAMETERS: 
 *		cardNo			- card number of database
 *		dbID				- database ID
 *
 *	RETURNS:		dmErrDatabaseProtected if protected or 0
 *
 *	HISTORY:
 *		08/06/98	roger	Broke out of DmDeleteDatabase
 *		10/09/99	kwk	Check for overlay that's used by protected base.
 *		05/13/00	kwk	Use MemMove vs. StrNCopy to avoid dependency
 *							on Text Mgr being alive.
 *
 *************************************************************/
static Err PrvDatabaseIsProtected (UInt16 cardNo, LocalID dbID)
{
	Err err = 0;
	
	// Make sure this database is not protected
	if (GDmProtectListH) {
		DmProtectEntryPtr	entryP;
		UInt32				numEntries;
		
		// Lock down list of protected databases
		entryP = MemHandleLock(GDmProtectListH);
		numEntries = MemPtrSize(entryP) / sizeof(DmProtectEntryType);
		
		// See if this database is one of the protected ones
		for (; numEntries; entryP++, numEntries--) {
			if (entryP->cardNo == cardNo && entryP->dbID == dbID) {
				err = dmErrDatabaseProtected;
				break;
				}
			}
		
		MemHandleUnlock(GDmProtectListH);
		
		// If we don't think the DB is protected, but it's an overlay,
		// then do extra check to see if its base is protected, in
		// which case the overlay should be the same.
		if ((err == errNone) && (MemLocalIDKind(dbID) == memIDHandle)) {
			DatabaseHdrPtr hdrP = MemHandleLock(MemLocalIDToGlobal(dbID, cardNo));
			OmLocaleType dbLocale;
			
			// If it's an overlay, and the name looks OK, check the locale.
			if ((hdrP->type == sysFileTOverlay)
			&&  (OmOverlayDBNameToLocale((const Char*)hdrP->name, &dbLocale) == errNone)) {
				OmLocaleType curLocale;
				OmGetCurrentLocale(&curLocale);
				
				// If the locale matches, generate the base name and see if
				// we can find it on this card.
				// DOLATER kwk - should probably have Overlay Mgr support for
				// mapping from overlay name to base name, versus "knowing" about
				// the name format here.
				if ((curLocale.language == dbLocale.language)
				&&  (curLocale.country == dbLocale.country)) {
					LocalID baseID;
					Char baseName[dmDBNameLength];
					UInt16 baseNameLen = StrLen((Char*)hdrP->name) - 5;	// "_llCC"
					
					MemMove(baseName, hdrP->name, baseNameLen);
					baseName[baseNameLen] = '\0';
					
					// If we find it on the card, then the result for this
					// DB (the overlay) is the same as the result for the base.
					baseID = DmFindDatabase(cardNo, baseName);
					if (baseID != 0) {
						err = PrvDatabaseIsProtected(cardNo, baseID);
						}
					}
				}
			
			MemPtrUnlock((MemPtr)hdrP);
			}
		}
	
	return err;
}


/************************************************************
 *
 *  FUNCTION: PrvGetAppInfo
 *
 *  DESCRIPTION: Returns the app info structure.
 *
 *	 Both the current app and all running launch codes share the
 *	 current app's app info block for database related storage.
 *	 Specifically, the dmLastErr and the list of opened databases
 *  are maintained in only one location.  This simplifies some
 *  cases.
 *
 *  PARAMETERS: none
 *
 *  RETURNS: MemPtr to the current app's info.
 *
 *  CREATED: 
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	2/17/99	Factored out of 23 places.
 *			SCL	2/24/99	Now returns appInfoP as returned by SysGetAppInfo.
 *
 *************************************************************/
static SysAppInfoPtr	PrvGetAppInfo(void)
{
	SysAppInfoPtr unusedAppInfoP;
	SysAppInfoPtr appInfoP;
	
	appInfoP = SysGetAppInfo(&appInfoP, &unusedAppInfoP);
	return appInfoP;
}



/************************************************************
 *
 *  FUNCTION: PrvMergeFree
 *
 *  DESCRIPTION: This routine gets called when PrvDmNewHandle is unsuccessful
 *		finding a free chunk large enough. This routine will attempt to
 *		re-arrange chunks in the heaps until it can make a free chunk that
 *		is reqSize in bytes.
 *
 *		It works by walking through all un-opened and un-protected databases
 *		and moving any records or resources they own out of the target heap.
 *		
 *		Initially, the target heap is the first storage heap. But, if the first
 *		pass through all the databases is unsuccessful, this algorithm will
 *		try successively higher target heaps. 
 *		
 *		
 *
 *  PARAMETERS: 
 *			reqSize		- desired amount of free space to form.
 *			
 *  RETURNS: 
 *			0 if enough free space has been formed to allocate a chunk of
 *		reqSize bytes. 
 *
 *	 CALLED BY: 
 *			PrvDmNewHandle
 *
 *  CREATED: 9/26/96
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
static Err	PrvMergeFree(UInt16	cardNo, UInt32 reqSize)
{
	Err				err = dmErrMemError;
	UInt16			dbIndex, targetHeapIndex, numRAMHeaps, newHeapIndex;
	UInt16			targetHeapID, newHeapID;
	MemHandle		recordH;
	DatabaseDirPtr	dirP=0;
	UInt16			firstDBIndex, numDBs;
	LocalID			dbID;
	DmOpenInfoPtr	openP=0;
	Boolean			v20=false;
	UInt32			romVersion;
	DmOpenRef		dbR=0;
	UInt16			attributes;
	UInt16			recHeapID;
	UInt16			numRecords;
	Int32				recIndex;
	MemHandle		newRecordH;
	UInt32			recSize;
	UInt32			resType;
	UInt16			resID;
	LocalID			recLocalID;
	Boolean			movedSome = false;
	UInt16			recAttr;
	UInt32			recUniqueID;
	UInt16			index;
	UInt32			free;
	UInt32			max;
	
	
	// DOLATER... this algorithm ignores the lock information and moves locked records/resources

	// Enable writes to memory
	MemSemaphoreReserve(true);


	// See if we're on version 2.0 of the ROM or later.
	// DOLATER - do we still need this check?
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
	if (romVersion >= 0x02000000) v20 = true;
	
	firstDBIndex = 0;
	numDBs = DmNumDatabases(cardNo);
	
	
	//===================================================================
	// Start trying to make free space in the first storage heap
	// and if that fails, try each successive heap till we reach the
	//  end
	//===================================================================
	numRAMHeaps = MemNumRAMHeaps(cardNo);
	for (targetHeapIndex = 0; targetHeapIndex < numRAMHeaps; targetHeapIndex++) {
		targetHeapID = MemHeapID(cardNo, targetHeapIndex);
		if (MemHeapDynamic(targetHeapID)) continue;
		
	
		//-----------------------------------------------------------------
		// Walk through each of the databases and re-allocate records
		// out of the target heap
		//-----------------------------------------------------------------
		for (dbIndex = firstDBIndex; dbIndex < numDBs; dbIndex++) {

			movedSome = false;
		
			// Get the localID of the database
			dbID = DmGetDatabase(cardNo, dbIndex);
			if (!dbID) {err = dmErrMemError; goto Exit;}
			
			
			// See if this database is open. If so, skip it
			openP = (DmOpenInfoPtr)GDmOpenList;
			while(openP) {
				if (openP->hdrID == dbID && openP->cardNo == cardNo) break;
				openP = (DmOpenInfoPtr)openP->next;
				}
			if (openP) continue;
			
			// On V2.0 and later versions of the ROM, make sure this 
			//  database is not protected
			// DOLATER kwk - use PrvDatabaseIsProtected here?
			if (v20 && GDmProtectListH) {
				DmProtectEntryPtr	entryP;
				UInt16					numEntries;
				
				// Lock down list of protected databases
				err = 0;
				entryP = MemHandleLock(GDmProtectListH);
				numEntries = (UInt32)MemPtrSize(entryP) / (UInt16)sizeof(DmProtectEntryType);
				
				// See if this database is one of the protected ones
				for (; numEntries; entryP++, numEntries--) {
					if (entryP->cardNo == cardNo && entryP->dbID == dbID) {
						err = dmErrDatabaseProtected;
						break;
						}
					}
				
				// Unlock list and skip it if protected
				MemHandleUnlock(GDmProtectListH);
				if (err) continue;
				}


			// This database is neither open nor protected, so it's safe to
			//  move it's records around (i.e. no one has one of its record handles
			//  cached away somewhere). Open it and move any records it has in the
			//  target heap to other heaps
			dbR = DmOpenDBNoOverlay(cardNo, dbID, dmModeReadWrite);
			if (!dbR) continue;
			
			// Records or resources?
			DmDatabaseInfo(cardNo, dbID, 0,
					&attributes, 0, 0,
					0, 0, 0, 0, 0, 0, 0);
					
					
			//---------------------------------------------------------------
			// Resource database
			//---------------------------------------------------------------
			if (attributes & dmHdrAttrResDB) {
				numRecords = DmNumResources(dbR);
				
				// Loop through each of the resources in the database.
				for (recIndex = (Int32)numRecords-1; recIndex >= 0; recIndex--) {
					recordH = DmGetResourceIndex(dbR, (UInt16)recIndex);
					if (!recordH) continue;
					
					// See which heap this record is in.
					recHeapID = MemHandleHeapID(recordH);
					
					// If it's not in the target heap, skip it
					if (recHeapID != targetHeapID) continue;
					
					// Since it's in the target heap, move it out
					recSize = MemHandleSize(recordH);
					for (newHeapIndex=0; newHeapIndex < numRAMHeaps; newHeapIndex++) {
						newHeapID = MemHeapID(cardNo, newHeapIndex);
						if (MemHeapDynamic(newHeapID)) continue;
						if (newHeapID == targetHeapID) continue;

						// Don't completely fill the heaps. Especially in the case where
						//  a brand new database header is in the heap and will need to
						//  be resized in order to add the first record. 
						MemHeapFreeBytes(newHeapID, &free, &max);
						if (free < recSize + 0x200) continue;

						// Try and allocate a new record here.
						newRecordH = MemChunkNew(newHeapID, recSize, dmOrphanOwnerID);
						if (!newRecordH) continue;
						
						// Move the record contents to the new location
						DmResourceInfo(dbR, (UInt16)recIndex, &resType, &resID, &recLocalID);

						// Detach the old MemHandle and attach the new one.
						err = DmDetachResource(dbR, (UInt16)recIndex, &recordH);
						if (err) { MemHandleFree(newRecordH); break; }
						err = DmAttachResource(dbR, newRecordH, resType, resID);
						if (err) {
							err = DmAttachResource(dbR, recordH, resType, resID);
							ErrFatalDisplayIf(err, "Corrupted database");
							MemHandleFree(newRecordH);
							break;
							}
							
						// Copy the contents from the old MemHandle to the new one
						PrvDmHandleRealloc(dbR, recordH, newRecordH, 0);
						movedSome = true;
						break;
						}
					}
				}
				
				
			//---------------------------------------------------------------
			// Record database
			//---------------------------------------------------------------
			else {

				numRecords = DmNumRecords(dbR);
				
				// Loop through each of the records in the database.
				for (recIndex = (Int32)numRecords-1; recIndex >= 0; recIndex--) {
				
					// Use this call so we can move deleted records too.
					err = DmRecordInfo(dbR, (UInt16)recIndex, &recAttr, &recUniqueID, &recLocalID);
					if (err || !recLocalID) continue;
					recordH = MemLocalIDToGlobal(recLocalID, cardNo);
					if (!recordH) continue;
					
					// See which heap this record is in.
					recHeapID = MemHandleHeapID(recordH);
					
					// If it's not in the target heap, skip it
					if (recHeapID != targetHeapID) continue;
					
					// Since it's in the target heap, move it out
					recSize = MemHandleSize(recordH);
					for (newHeapIndex=0; newHeapIndex < numRAMHeaps; newHeapIndex++) {
						newHeapID = MemHeapID(cardNo, newHeapIndex);
						if (MemHeapDynamic(newHeapID)) continue;
						if (newHeapID == targetHeapID) continue;

						// Don't completely fill the heaps. Especially in the case where
						//  a brand new database header is in the heap and will need to
						//  be resized in order to add the first record. 
						MemHeapFreeBytes(newHeapID, &free, &max);
						if (free < recSize + 0x200) continue;

						// Try and allocate the new record here
						newRecordH = MemChunkNew(newHeapID, recSize, dmOrphanOwnerID);
						if (!newRecordH) continue;
						

						// Detach the old MemHandle and attach the new one.
						err = DmDetachRecord(dbR, (UInt16)recIndex, &recordH);
						if (err) { MemHandleFree(newRecordH); break; }
						index = (UInt16)recIndex;
						err = DmAttachRecord(dbR, &index, newRecordH, 0);
						if (err) {
							index = (UInt16)recIndex;
							err = DmAttachRecord(dbR, &index, recordH, 0);
							ErrFatalDisplayIf(err, "Corrupted database");
							MemHandleFree(newRecordH);
							break;
							}
						DmSetRecordInfo(dbR, index, &recAttr, &recUniqueID);
							
						// Copy the contents from the old MemHandle to the new one
						PrvDmHandleRealloc(dbR, recordH, newRecordH, 0);
						movedSome = true;
						break;
						}
					}
				}
				
				
			// Close it
			DmCloseDatabase(dbR);
			dbR = 0;
			
			// If we've moved some data out of the target heap, see if we can
			// allocate a chunk now
			if (movedSome) {
				recordH = MemChunkNew(targetHeapID, reqSize, dmOrphanOwnerID);
				if (recordH) {
					MemHandleFree(recordH);
					err = 0;
					goto Exit;
					}
				}

			} // for (dbIndex = firstDBIndex; dbIndex < numDBs; dbIndex++) 
			
		} //for (heapIndex = 0; heapIndex < numHeaps; heapIndex++) 
	
	// If we fell out of the loop, we were unsuccessful
	err = dmErrMemError;
	
	
Exit:
	if (dbR) DmCloseDatabase(dbR);

	// Restore write-protection
	MemSemaphoreRelease(true);

	return err;
}


/************************************************************
 *
 *  FUNCTION: PrvShiftEntriesUp, private
 *
 *  DESCRIPTION: Shifts a range of database entries up by 1. It will shift 
 *		entry N into entry N-1, and proceed till it moves 'shiftSize' records.
 *		Note that the starting index N must be greater than 0.
 *
 *		The number of records to shift is passed in shiftSize, or 0 can
 *		be passed to shift all records from startIndex to the end.
 *
 *		If adjustSize is true, and shiftSize is 0 (moving all records from
 *		startIndex to the end), the last recordList's size will be decresed
 *		by one record.
 *		
 *		As a special case, in order to support the deletion of the last
 *		record in a database, N can be equal to the index of the last record+1
 *		as long as adjustSize is true and shiftSize is 0.
 *		
 *
 *  PARAMETERS: 
 *			openP 			-	DmOpenInfoPtr of open database
 *			startIndex		-  index of first entry to shift
 *			shiftSize		-  # of entries to shift, or 0 to shift all entries
 *								  from startIndex to the end of the Database
 *			adjustSize		- if true, the necessary changes will be made to the
 *									Database to record the fact that a record was
 *									deleted.
 *									NOTE: if adjustSize is true, shiftSize MUST BE 0.
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 8/23/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
static Err	PrvShiftEntriesUp(DmOpenRef dbR, UInt16 startIndex, 
			UInt16 shiftSize, Boolean adjustSize)
{
	DmOpenInfoPtr	openP = ((DmAccessPtr)dbR)->openP;
	RecordListPtr	listP;
	LocalID			id;
	UInt16			cumIndex;
	UInt8 			* entryP;
	UInt8				* prevEntryP;
	Err				err = dmErrIndexOutOfRange;
	UInt16			i, moveRecords;
	Boolean			movedRecords=false;
	UInt32			entrySize;
	Boolean			done  = false;
	Boolean			deleteLast = false;
	
	
	// Determine the size of each entry, depending on whether this is a 
	//  record or resource database
	if (openP->resDB) entrySize = sizeof(RsrcEntryType);
	else entrySize = sizeof(RecordEntryType);
	
	// Point to the first list (part of the header)
	listP = &openP->hdrP->recordList;
	cumIndex = 0;
	
	
	// Error check
	ErrFatalDisplayIf(adjustSize && shiftSize, "Shiftsize must be 0 if adjustSize is true");
	
	
	//-----------------------------------------------------------------------------
	// If we're shifting up, we start from the beginning and work our way to
	//  the end. We put the record at index N into the space at index N-1.
	//-----------------------------------------------------------------------------

	// Error check start index, can't be less than 1.
	ErrFatalDisplayIf(startIndex < 1, "Invalid start index");
	
	// If the index is equal to the lastIndex+1, we're deleting the last record
	if (startIndex == openP->numRecords) deleteLast = true;

	//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
	// Loop through each record list and shift the entries in each one accordingly.
	//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
	while(1) {
	
		// Get pointer to first entry in list.
		entryP = (UInt8 *)&listP->firstEntry;
		
		//.................................................................
		// Shift all records in this list up if it is in the active area
		//.................................................................
		if (startIndex < cumIndex + listP->numRecords) {
		
			// Assume we need to move all records in this list
			moveRecords = listP->numRecords;
			
			// If we need to start partway through this list, adjust the starting
			//  record pointer, previous record pointer, and # of records to move
			if (startIndex > cumIndex)  {
				i = startIndex - cumIndex;
				moveRecords -= i;
				prevEntryP = entryP + (i-1)*entrySize;
				entryP = prevEntryP + entrySize;
				}
				
			// Clip the # of records we need to move to shiftSize, if it was specified.
			if (shiftSize && shiftSize < moveRecords) 
				moveRecords = shiftSize;
				
			// Decrement the remaining # of records we need to move in the other lists.
			if (shiftSize) {
				shiftSize -= moveRecords;
				if (!shiftSize) done = true;
				}

			// Move the records
			// The compiler generates more efficent code with these types of while loops.
			if (moveRecords) {
				movedRecords = true;
				// Resource entries
				if (openP->resDB) {
					RsrcEntryPtr	resP = (RsrcEntryPtr)entryP;
					RsrcEntryPtr	prevResP = (RsrcEntryPtr)prevEntryP;
					do {
						*prevResP = *resP;
						prevResP = resP;
						resP++;
						} while(--moveRecords);
					prevEntryP = (UInt8 *)prevResP;
					}
					
				else {
					RecordEntryPtr	recP = (RecordEntryPtr)entryP;
					RecordEntryPtr	prevRecP = (RecordEntryPtr)prevEntryP;
					do {
						*prevRecP = *recP;
						prevRecP = recP;
						recP++;
						} while(--moveRecords);
					prevEntryP = (UInt8 *)prevRecP;
					}
				}
				
			// Exit if we're done
			if (done) break;
			}
		// Save pointer to last entry in this list in case we have a case where
		//  we're shifting a record from one list to the previous list.
		else 
			prevEntryP = entryP + (listP->numRecords-1)*entrySize;
		
		
		//.................................................................
		// Calculate start index of the next list
		//.................................................................
		cumIndex += listP->numRecords;
		id = listP->nextRecordListID;
		if (!id) break;

		listP = MemLocalIDToGlobal(id, openP->cardNo);
		
		// If it's not ROM based, we actually get a MemHandle from MemLocalIDToGlobal
		if (!openP->handleTableP)
			listP = (RecordListPtr)MemDeref((MemHandle)listP);
			
		} // while(1)
		
		
	// If we did move all the requested records, no err
	if (movedRecords && !shiftSize) err = 0;
	
	// Or, if we're deleting the last record, no err
	if (deleteLast) err = 0;
	
	
	//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
	// We've finished shifting the necessary records. Now, if adjustSize was true,
	//  we need to decrease the size of the recordList that contains the last record we moved.
	//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
	if (adjustSize && !err) {
		UInt32	reqSize;
		listP->numRecords--;
		
		// If it's the first record list, it's part of the header. We can resize it
		//  without unlocking it because we're making it smaller.
		if (listP == &(openP->hdrP->recordList)) {
			reqSize = sizeof(DatabaseHdrType) + listP->numRecords * entrySize;
			err = MemPtrResize(openP->hdrP, reqSize);
			ErrNonFatalDisplayIf(err, "Err resizing header");
			}					
			
		// Else, it's a separate chunk
		else {
			if (listP->numRecords) {
				reqSize  = sizeof(RecordListType) + listP->numRecords * entrySize;
				err = MemPtrResize(listP, reqSize);						
				ErrNonFatalDisplayIf(err, "Err resizing record list");
				}
			// If no more records in this list, get rid of it
			else {
				RecordListPtr	prevListP;
				prevListP = PrvPrevRecordList(openP, listP);
				prevListP->nextRecordListID = 0;
				MemPtrFree(listP);
				}
			}
		}
		
		
		
	// Return error code
	ErrNonFatalDisplayIf(err, "Err shifting records in DB");
	return err;
}


/************************************************************
 *
 *  FUNCTION: PrvShiftEntriesDown, private
 *
 *  DESCRIPTION: Shifts a range of database entries down by 1. It will shift 
 *		entry N into entry N+1, and proceed till it reaches entry N+shiftSize.
 *
 *		The number of records to shift is passed in shiftSize, or 0 can
 *		be passed to shift all records from startIndex to the end.
 *
 *		If adjustSize is true, and shiftSize is 0 (moving all records from
 *		startIndex to the end), the last recordList's size will be increased
 *		by one record.
 *		
 *		
 *
 *  PARAMETERS: 
 *			openP 			-	DmOpenInfoPtr of open database
 *			startIndex		-  index of first entry to shift
 *			shiftSize		-  # of entries to shift, or 0 to shift all entries
 *								  from startIndex to the end of the Database
 *			adjustSize		- if true, the necessary changes will be made to the
 *									Database to record the fact that a record was
 *									inserted.
 *									NOTE: if adjustSize is true, shiftSize MUST BE 0.
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 8/23/95
 *
 *  BY: Ron Marianetti
 *		vmk	1/13/98	Prevented incrementing record count before successful realloc
 *************************************************************/
static Err	PrvShiftEntriesDown(DmOpenRef dbR, UInt16 startIndex, 
			UInt16 shiftSize, Boolean adjustSize)
{
	DmOpenInfoPtr	openP = ((DmAccessPtr)dbR)->openP;
	RecordListPtr	listP, prevListP=0;
	LocalID			id;
	UInt16			cumIndex;
	UInt8 			* entryP;
	UInt8				* nextEntryP;
	Err				err = dmErrIndexOutOfRange;
	UInt16			moveRecords;
	UInt16			lastIndex;
	UInt32			entrySize;
	
	
	// Determine the size of each entry, depending on whether this is a 
	//  record or resource database
	if (openP->resDB) entrySize = sizeof(RsrcEntryType);
	else entrySize = sizeof(RecordEntryType);
	
	
	// Point to the first list (part of the header)
	listP = &openP->hdrP->recordList;
	cumIndex = 0;
	
	
	// Error check - Shiftsize must be 0 if adjustSize is true
	ErrFatalDisplayIf(adjustSize && shiftSize, "Bad Params");
	
	
	//-----------------------------------------------------------------------------
	// Since we're shifting down, we need to start from the end and work our way to
	//  the beginning. We put the record at index N into the space at index N+1.
	// Look through the record lists till we get to the list that contains the
	//  first destination slot (N+1).
	//-----------------------------------------------------------------------------
	lastIndex = startIndex + shiftSize - 1;
	while(1) {
		
		// Get the ID of the next recordList
		id = listP->nextRecordListID;	

		// See if this recordList is the one that will hold the first record we move
		if (shiftSize) {
			if (lastIndex+1 < listP->numRecords + cumIndex) break;
			}
		else 
			if (id == 0) break;
			
		// Next list
		cumIndex += listP->numRecords;
		
		// We should only go past the end if the caller intends to grow the Database 
		//  (shiftSize==0)
		if (id == 0) {		 
			ErrDisplay("Invalid startIndex or shiftSize");
			shiftSize = 0;
			break;
			}
		prevListP = listP;
		listP = MemLocalIDToGlobal(id, openP->cardNo);
		
		// If it's not ROM based, we actually get a MemHandle from MemLocalIDToGlobal
		if (!openP->handleTableP)
			listP = (RecordListPtr)MemDeref((MemHandle)listP);
		}
		
	// Calculate the actual shiftSize if the user passed 0
	if (!shiftSize)
		shiftSize = cumIndex + listP->numRecords - startIndex;
		
		
		
	//-----------------------------------------------------------------------------
	// Grow the last record list chunk by 1 entry if adjustSize is true.
	//  After this section, we must be sure to keep listP pointing to the
	//  list that has the destination slot of the first record we're going to move.
	//-----------------------------------------------------------------------------
	
	// DOLATER... the code presently attempts to keep each list segment as large as possible,
	// causing resizing and moves of possibly large amounts of data each time a new record is
	// added to a database with large number of records.  It may be worthwhile to investigate
	// further the effect of limiting the size of each record sub-list.  1/6/98 vmk
	
	if (adjustSize) {
		MemHandle			listH;
		UInt32					reqSize;
		
		// There was a huge window for breaking the database's integrity here
		// because the count was incremented before the reallocation.  So if the reallocation
		// forces a comapction that takes a very long time (several minutes is possible with
		// large heaps), the user can interpret that as a system hang and reset the unit.
		// If this happens, numRecords will indicate one more entry than there really are
		// in the list, causing the database integrity to be broken. -- FIXED vmk 1/6/98
		
		//listP->numRecords++;		// !!!don't want to pre-increment in case user resets during reasizing
		
		err = dmErrIndexOutOfRange;			// assume error
			
		// If it's the first list, it's part of the header
		if (!prevListP) {
			reqSize = sizeof(DatabaseHdrType) + ((listP->numRecords + 1) * entrySize);
			
			// if we need to grow at all, grow the chunk by a larger increment
			if (reqSize > MemPtrSize(openP->hdrP)) {
				reqSize = sizeof(DatabaseHdrType) + ((listP->numRecords + 16) * entrySize);
				MemPtrUnlock(openP->hdrP);
				err = MemHandleResize(openP->hdrH, reqSize);
				openP->hdrP = MemHandleLock(openP->hdrH);
				listP = &openP->hdrP->recordList;
				}

			// otherwise the chunk has enough space already, so just use it
			else
				err = 0;
			}
			
		// Else, it's a separate chunk
		else {
			reqSize = sizeof(RecordListType) + ((listP->numRecords + 1) * entrySize);
			
			if (reqSize > MemPtrSize(listP)) {
				reqSize = sizeof(RecordListType) + ((listP->numRecords + 16) * entrySize);
				listH = MemPtrRecoverHandle(listP);
				MemPtrUnlock(listP);
				err = MemHandleResize(listH, reqSize);
				listP = MemHandleLock(listH);
				}

			// otherwise the chunk has enough space already, so just use it
			else
				err = 0;
			}
			
		// If resize succeeded, just increment count
		if ( !err ) {
			listP->numRecords++;
			}
			
		// Else, if we encountered an error resizing the chunk, we have to allocate another
		else {
			MemHandle	newH;
			
			#if 0		// CASTRATION -- we eliminated the over-increment above so don't need to fix up
				// Back off the increase in # of records on the list we couldn't grow
				listP->numRecords--;								// Back out the increase in size
			#endif
			
			// Allocate new record list
			reqSize = sizeof(RecordListType) + 1 * entrySize;
			newH = PrvDmNewHandle(dbR, reqSize);
			if (!newH) {
				err = dmErrMemError; 
				goto ExitNoErrDisplay;
				}
			else err = 0;
			
			// Set the owner ID of the new list record chunk 
			MemHandleSetOwner(newH, dmMgrOwnerID);

			// Link this new list into the previous list and ajust
			//  cumIndex for the new list.
			listP->nextRecordListID = MemHandleToLocalID(newH);
			cumIndex += listP->numRecords;
			prevListP = listP;

			// Intialize and lock the new recordlist
			listP = MemHandleLock(newH);
			MemSet(listP, reqSize, 0);
			listP->numRecords = 1;
			}
		}
		
	//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
	// Loop through each record list and shift the entries in each one accordingly.
	// On entry here, listP points to the recordList that contains the destination slot of the
	//  first record we need to move (since we're copying from the end to the front,
	//  this turns out being the last record in the selected area).
	//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
	err = dmErrIndexOutOfRange;					// assume error
	while(1) {
		Int32	firstRelIndex;
	
		// Get pointer to first record in list.
		entryP = (UInt8 *)&listP->firstEntry;
		

		// Calculate the index of the first record we need to move, relative to the
		//  current list.
		firstRelIndex = (Int32)startIndex + (Int32)shiftSize - (Int32)cumIndex - 1;
		
		
		//..........................................................................
		// If it's in this list, do the necessary moves to this list. The only time this
		// will not be true is when we have just added a new chunk to hold an additional
		//  record.
		//..........................................................................
		if (firstRelIndex >= 0) {
		
			// Point to the first entry we need to move
			entryP = entryP + firstRelIndex*entrySize;

			// If we need to start before the end of this list, adjust the 
			// next record pointer to point to the following entry. Otherwise,
			//  keep it pointing to the first entry in the list we just came from.
			if (firstRelIndex  < listP->numRecords - 1)  
				nextEntryP = entryP + entrySize;
				
			// Calculate how many records to move in this list
			moveRecords = (UInt16)firstRelIndex + 1;
			if (moveRecords > shiftSize) moveRecords = shiftSize;
			
			// Decrement the remaining # of records we need to move in the other lists.
			shiftSize -= moveRecords;
	
			// Move Resource based entries
			if (openP->resDB) {
				RsrcEntryPtr	resP = (RsrcEntryPtr)entryP;
				RsrcEntryPtr	nextResP = (RsrcEntryPtr)nextEntryP;
				do {
					*nextResP = *resP;
					nextResP = resP;
					resP--;
					} while(--moveRecords);
				nextEntryP = (UInt8 *)nextResP;
				}
				
			// Move Record based entries
			else {
				RecordEntryPtr	recP = (RecordEntryPtr)entryP;
				RecordEntryPtr	nextRecP = (RecordEntryPtr)nextEntryP;
				do {
					*nextRecP = *recP;
					nextRecP = recP;
					recP--;
					} while(--moveRecords);
				nextEntryP = (UInt8 *)nextRecP;
				}
					
				
			// Break out if we're done
			if (shiftSize == 0) {err = 0; break;}
			}
			
		// the first index we need to move is not in this list, so just record a reference to
		//  the first slot in this list in the variable nextRecordP because it will hold the
		//  next record we move from the previous list.
		else 
			nextEntryP = entryP;
			
			
		//.................................................................
		// Get pointer to the previous list to this one
		//.................................................................
		listP = PrvPrevRecordList(openP, listP);
		ErrFatalDisplayIf(!listP, "Logic flaw");
		cumIndex -= listP->numRecords;

		} // while(1)
		
Exit:
	// Return error code
	ErrNonFatalDisplayIf(err, "Err shifting records in DB");
	
ExitNoErrDisplay:
	return err;

}


/************************************************************
 *
 *	FUNCTION:		PrvValidateRamDBList
 *
 *	DESCRIPTION:	Make sure the RAM store DB list is valid,
 *		in that every entry in the ROM store DB list has a
 *		corresponding entry in the RAM store DB list, and every
 *		ROM-based entry in the RAM store DB list exists in the
 *		ROM store DB list. If anything doesn't match, then we
 *		have to recreate the RAM store DB list.
 *
 *		This is only necessary for the case where somebody has
 *		done a flash update of the ROM _without_ a hard reset,
 *		which should only be an internal development case,
 *		thus this code only gets used by the debug ROM.
 *
 *  PARAMETERS:
 *		ramDirID	->	chunk that contains list of all DBs on the card.
 *		romDirID -> chunk that contains list of all DBs on card ROM.
 *		card		-> card that contains both stores.
 *
 *	RETURNS:	nothing.
 *
 *	HISTORY:
 *		10/06/99	kwk	Created by Ken Krugler.
 *
 *************************************************************/
static void PrvValidateRamDBList (LocalID ramDirID, LocalID romDirID, UInt16 card)
{
	MemHandle ramDirH;
	DatabaseDirPtr ramDirP;
	DatabaseDirPtr romDirP;
	Boolean rebuildList = false;
	UInt16 i, j;
	UInt16 numRomDBs = 0;
	
	romDirP = MemLocalIDToGlobal(romDirID, card);
	ramDirH = MemLocalIDToGlobal(ramDirID, card);
	ramDirP = MemHandleLock(ramDirH);
	
	// Make sure that every ROM-based DB in the RAM store list exists
	// in this ROM, and count the number of entries.
	for (j = 0; j < ramDirP->numDatabases && !rebuildList; j++) {
		Boolean foundDB = false;
		LocalID dbID = ramDirP->databaseID[j].baseID;
		
		if (MemLocalIDKind(dbID) != memIDPtr) {
			continue;
			}
		
		for (i = 0; i < romDirP->numDatabases; i++) {
			if (romDirP->databaseID[i].baseID == dbID) {
				numRomDBs += 1;
				foundDB = true;
				break;
				}
			}
		
		if (!foundDB) {
			rebuildList = true;
			}
		}
	
	// If we found every ROM-based DB, then make sure that the count
	// matches (no extra DBs in the ROM that we don't know about).
	if ((!rebuildList) && (numRomDBs != romDirP->numDatabases)) {
		rebuildList = true;
		}
	
	// If we have to rebuild the list, we need to allocate a new list which
	// has space for all the ROM DBs, then add the RAM DBs one at a time.
	if (rebuildList) {
		UInt32 romDirListLen;
		UInt16 heapID;
		MemHandle newRamDirH;
		DatabaseDirPtr newRamDirP;
		LocalID newRamDirID;
		Err err;
		
		heapID = MemHandleHeapID(ramDirH);
		romDirListLen = MemPtrSize((MemPtr)romDirP);
		newRamDirH = MemChunkNew(heapID, romDirListLen, dmMgrOwnerID);
		ErrFatalDisplayIf(newRamDirH == NULL, "Can't alloc RAM store DB list");
		
		newRamDirP = MemHandleLock(newRamDirH);
		MemMove(newRamDirP, romDirP, romDirListLen);
		newRamDirP->nextDatabaseListID = 0;
		
		for (j = 0; j < ramDirP->numDatabases; j++) {
			DatabaseHdrPtr hdrP;
			LocalID dbID = ramDirP->databaseID[j].baseID;
			
			if (MemLocalIDKind(dbID) == memIDPtr) {
				continue;
				}
			
			hdrP = MemHandleLock(MemLocalIDToGlobal(dbID, card));
			newRamDirP = PrvAddEntryToDBList(newRamDirP,
														hdrP,
														card,
														dbID);
			MemPtrUnlock((MemPtr)hdrP);
			}
		
		// Set up the new DB list in the card's RAM store.
		newRamDirID = MemHandleToLocalID(newRamDirH);
		err = MemStoreSetInfo(card, memRamStoreNum,
				0, 0, 0, 
				0, 0,
				0, 0,
				0, &newRamDirID);
		
		ErrFatalDisplayIf(err != errNone, "Can't set RAM store DB list");
		
		// Toss the old RAM store DB list memory.
		MemHandleFree(ramDirH);
		ramDirP = newRamDirP;	// so unlock below gets it
		}
	
	// cleanup and exit
	MemPtrUnlock(ramDirP);
}


/************************************************************
 *
 *  FUNCTION: DmInit
 *
 *  DESCRIPTION: Initializes the data manager. Free all records
 *		in the Storage heaps which have been orphaned. A chunk
 *		is orphaned if it was detached from a database or
 *		created and never attached. Orphaned records have a
 *		unique owner ID of dmOrphanOwnerID. We also initialize
 *		all of the RAM store DB lists for all cards, so that
 *		by the time somebody calls any of the DmXXX routines
 *		the lists are ready to be used.
 *		Finally, we run a consistency check on the launch database to 
 *		make sure its in sync with the databases on the device.
 *
 *  PARAMETERS: void
 *
 *  RETURNS: 0 if successful, errorcode if not
 *
 *	HISTORY:
 *		08/01/95	RM		Created by Ron Marianetti
 *		10/01/99	kwk	Initialize DB list for RAM store on all cards.
 *		10/28/99	kwk	Skip ROM-only cards when building DB list.
 *		11/17/99	jed	Call PrvSyncLaunchDatabase()
 *		03/13/00	jmp	Roll in memory-leak bug-fix that went into Update
 *							3.5.1.  (Fixes bugs #26062 & #26088.)
 *
 *************************************************************/
Err	DmInit()
{
	UInt16			card, numCards;
	UInt16			theHeap, numHeaps;
	UInt16			i, heapID, numDBs, dbIndex;
	Err 				err = errNone;
	LocalID			ramDirID, romDirID, dbID;
	MemHandle		dirH = NULL;
	DatabaseDirPtr	dirP = NULL;
	DatabaseHdrPtr	hdrP;
	
	numCards = MemNumCards();

	// Grab the semaphore for exclusive access.
	MemSemaphoreReserve(true);
	
	//-------------------------------------------------------------
	// See if we need to create the DB directory list for any card.
	//-------------------------------------------------------------
	for (card=0; card < numCards; card++) {
		theHeap = 0xFFFF;
		numHeaps = MemNumRAMHeaps(card);  
		for (i=0; i<numHeaps; i++) {
			heapID = MemHeapID(card, i);
			if (MemHeapDynamic(heapID)) continue;
			theHeap = heapID;
			break;
			}
	
		err = MemStoreInfo(card, memRamStoreNum, 
					0, 0, 0, 
					0, 0, 
					0, 0, 
					0, &ramDirID);
		
		// Skip ROM-only cards here.
		if (err == memErrROMOnlyCard) {
			continue;
			}
		else if (err != errNone) {
			goto Exit;
			}
			
		if (MemStoreInfo(card, memRomStoreNum, 0, 0, 0, 0, 0, 0, 0, 0, &romDirID) != errNone) {
			romDirID = 0;
			}
		
		// If we have both a ROM and and RAM store DB directory list, and we're
		// built for full error check, then make sure our RAM list is in sync
		// with the ROM - these can get out of sync if somebody builds a new
		// ROM and then uses PalmDebugger to flash the device, which doesn't
		// do a hard reset.
		if ((ramDirID != 0) && (romDirID != 0)) {
			ECDmValidateRamDBList(ramDirID, romDirID, card);
			}
		else if (ramDirID == 0) {
			UInt32 romDirListLen;
			DatabaseDirPtr romDirP;
			
			// If the ROM store DB directory list exists, use it to seed our
			// RAM store DB directory list.
			if (romDirID != 0) {
				romDirP = MemLocalIDToGlobal(romDirID, card);
				romDirListLen = MemPtrSize((MemPtr)romDirP);
				ErrNonFatalDebuggerIf(romDirListLen != OffsetOf(DatabaseDirType, databaseID) + (romDirP->numDatabases * sizeof(DatabaseDirEntryType)),
										"Invalid ROM DB dir list size");
				}
			else {
				romDirP = NULL;
				romDirListLen = OffsetOf(DatabaseDirType, databaseID);
				}
				
			// Make sure we found the heap to use for the DB list. We assume that
			// if the card is ROM-only, then calling MemStoreInfo (above) for the
			// RAM store will return an error, and we'll bail out before getting here.
			ErrNonFatalDebuggerIf(theHeap == 0xFFFF, "Didn't find storage heap on card for DB list");
			dirH = MemChunkNew(theHeap, romDirListLen, dmMgrOwnerID);
			if (dirH == NULL) {
				goto Exit;
				}
			
			dirP = MemHandleLock(dirH);
			if (romDirP != NULL) {
				MemMove(dirP, romDirP, romDirListLen);
				}
			else {
				dirP->numDatabases = 0;
				}
			
			// Currently this next list id never gets used.
			dirP->nextDatabaseListID = 0;
			
			ramDirID = MemHandleToLocalID(dirH);
			err = MemStoreSetInfo(card, memRamStoreNum,
					0, 0, 0, 
					0, 0,
					0, 0,
					0, &ramDirID);
			
			if (err) {
				goto Exit;
				}

			// Set up for loop to continue.
			MemPtrUnlock(dirP);
			dirP = NULL;
			}
		}

Exit:
	if (dirP)
		MemPtrUnlock(dirP);

	
	//---------------------------------------------------------------
	// Loop through all databases
	//---------------------------------------------------------------
	for (card = 0; card < numCards; card++) {
		numDBs = DmNumDatabases(card);
		
		// loop through databases on this card. 
		for (dbIndex = 0; dbIndex < numDBs; dbIndex++) {
			
			dbID = DmGetDatabase(card, dbIndex);
			
			hdrP = MemLocalIDToLockedPtr(dbID, card);
			
			// Is the database recyclable or temporary? 
			if(((hdrP->attributes) & dmHdrAttrRecyclable) ||
					hdrP->type == sysFileTTemp) {
				
				// If so, then delete it...
				
				// clear recyclable bit before deleting to avoid infinite recursion...
				// We have the memory semaphore w/ write access, so we can just change it(!).
				hdrP->attributes &= (~dmHdrAttrRecyclable);
				
				// unlock the db header so we can delete the database...
				MemPtrUnlock(hdrP);
				
				if(DmDeleteDatabase(card, dbID) == errNone) {
					// tweak loop controls to account for the missing database...
					// Otherwise we would skip one and/or go past the end.
					numDBs--;
					dbIndex--;
					}
				}
			else MemPtrUnlock(hdrP);
			
			} // for (dbIndex = 0; dbIndex < numDBs; dbIndex++)
		} // for (card = 0; card < numCards; card++)
	
	
	//-------------------------------------------------------------
	// Go through and free all orphaned Data Manager chunks.
	//-------------------------------------------------------------
	for (card=0; card < numCards; card++) {
		theHeap = 0xFFFF;
		numHeaps = MemNumRAMHeaps(card);  
		for (i=0; i<numHeaps; i++) {
			heapID = MemHeapID(card, i);
			if (MemHeapDynamic(heapID)) continue;
			MemHeapFreeByOwnerID(heapID, dmOrphanOwnerID);
			}
		}
	
	
	MemSemaphoreRelease(true);
	
	// Update the Launch database so we're sure it's in sync.
	// We do it here, inside the 'Exit' block since we will always want to try it,
	// and we do not need the memory semaphore reserved, so its better to not reserve it.
	// Plus, its been tested here, and it hasn't been tested anywhere else, so I ain't movin' it!
	PrvSyncLaunchDatabase();
	
	return err;
}


/************************************************************
 *
 *	FUNCTION:	PrvMemStoreInfo
 *
 *	DESCRIPTION: Load the DB directory list from the RAM store,
 *		or try to get it from ROM if we've got a ROM-only card.
 *
 *	PARAMETERS:
 *		cardNo			->	Which card (currently only 0 or 1 will work).
 *		databaseDirIDP	<-	Ptr to LocalID of DB dir list.
 *
 *	RETURNS: 0 if successful, errorcode if not
 *
 *	HISTORY:
 *		10/28/99	kwk	Created by Ken Krugler.
 *
 *************************************************************/
static Err PrvMemStoreInfo(UInt16 cardNo, LocalID* databaseDirIDP)
{
	Err result;
	
	result = MemStoreInfo(cardNo, memRamStoreNum, 
				0, 0, 0, 
				0, 0, 
				0, 0, 
				0, databaseDirIDP);
	
	if (result == memErrROMOnlyCard) {
		result = MemStoreInfo(cardNo, memRomStoreNum, 
				0, 0, 0, 
				0, 0, 
				0, 0, 
				0, databaseDirIDP);
		}
	
	return result;
}


/************************************************************
 *
 *  FUNCTION: DmCreateDatabase
 *
 *  DESCRIPTION: Creates a new database on the specified card with
 *		the given name, creator, and type
 *
 *  PARAMETERS: card number, name of database, creator, type
 *
 *  RETURNS: 0 if successful, errorcode if not
 *
 *  CREATED: 11/14/94 
 *
 *  BY: Ron Marianetti
 *		kcr		9/27/95		seed record ID's with semi-random number
 *		kcr		12/7/95		check for NULL database name; initialize version
 *									to 0.
 *		vmk		12/2/96		Added name length check
 *		vmk		12/22/97		Removed explicit compaction and free space check(perf optimization)
 *		jed		10/05/99		Add record to launcher database
 *
 *************************************************************/
Err	DmCreateDatabase(UInt16 cardNo, const Char * const nameP, 
			UInt32 creator, UInt32 type, Boolean resDB)
{
	//UInt32				freeBytes, maxChunk;
	//UInt32				maxFree;

	UInt16			heapID, theHeap, numHeaps, numEntries;
	MemHandle		headerH=0;
	DatabaseHdrPtr	headerP=0;
	LocalID			headerID=0, dirID;
	Err 				err = dmErrMemError;
	UInt16			i;
	MemHandle		dirH=0;
	DatabaseDirPtr	dirP=0;
	UInt32			reqSize, actSize;


	// Grab the semaphore for exclusive access
	MemSemaphoreReserve(true);
	

	// Make sure this database doesn't already exist in the RAM store
	// or has an invalid name
	if ((! *nameP) || StrLen(nameP) >= dmDBNameLength)
		{
		err = dmErrInvalidDatabaseName;
		goto Exit;
		}
	if (PrvFindDatabaseInStore(cardNo, memRamStoreNum, nameP)) {
		err = dmErrAlreadyExists;
		goto Exit;
		}


	//------------------------------------------------------------
	// Allocate the database header in the heap with the most free space
	//------------------------------------------------------------

	#if 0		// REMOVED TO OPTIMIZE FOR PERFORMANCE -- heap compaction and free byte
				// checking takes TOO long in the current memory manager
		numHeaps = MemNumRAMHeaps(cardNo);
		theHeap = 0;
		maxFree = 0;
		for (i=0; i < numHeaps; i++) {
			heapID = MemHeapID(cardNo, i);
			
			// Don't allocate in the dynamic heap
			if (MemHeapDynamic(heapID))
				continue;

			MemHeapCompact(heapID);
			MemHeapFreeBytes(heapID, &freeBytes, &maxChunk);
			if (maxChunk > maxFree) {
				theHeap = heapID;
				maxFree = maxChunk;
				}
			}
		if (theHeap) 
			headerH = MemChunkNew(theHeap, sizeof(DatabaseHdrType), dmMgrOwnerID);
		else
			headerH = 0;
	#endif	// REMOVED TO OPTIMIZE FOR PERFORMANCE


	//#if 0
		// Allocate the database header
		//
		// allocate from the first storage heap with sufficient free space;
		// this works partucularly well on cards with only 1 storage heap,
		// as is the case now with the new large heap memory model
		// DOLATER... need to re-examine when we have enough RAM store to necessitate
		// multiple storage heaps again
		numHeaps = MemNumRAMHeaps(cardNo);
		theHeap = 0xFFFF;
		headerH = 0;
		for (i=0; i < numHeaps; i++) {
			heapID = MemHeapID(cardNo, i);
			
			// Don't allocate in the dynamic heap
			if (MemHeapDynamic(heapID))
				continue;
			
			// attempt to allocate from this storage heap
			headerH = MemChunkNew(heapID, sizeof(DatabaseHdrType), dmMgrOwnerID);
			if ( headerH ) {
				theHeap = heapID;
				break;
				}
			}
	//#endif
	
	if (!headerH)
		goto Exit;



	//------------------------------------------------------------
	// Fill in the header
	//------------------------------------------------------------
	headerP = MemHandleLock(headerH);

	StrCopy((Char *)headerP->name, nameP);
	headerP->attributes = 0;
	if (resDB) headerP->attributes |= dmHdrAttrResDB;
	headerP->version = 0;
	headerP->creationDate = TimGetSeconds();
	headerP->modificationDate = TimGetSeconds();
	headerP->lastBackupDate = 0;
	headerP->modificationNumber = 0;
	headerP->appInfoID = 0;
	headerP->sortInfoID = 0;
	headerP->type = type;
	headerP->creator = creator;

	//	High order bits of uniqueID are seeded with 12
	//	low-order bits of system tick count.
	// TimeGetTicks is reset after a soft or hard reset.
	// TimGetSeconds is reset after a hard reset only if it's value is out 
	//  of range.  This happens when the batteries fail.
	// SysRandom is initialized by xoring part of the store memory before
	//  it is wiped during a hard reset.  The result is a somewhat random word.
	// The values for the high bits 0 and 1 are reserved. 
	//  0 is reserved for default records copied from default databases.
	do
		{
		headerP->uniqueIDSeed = TimGetSeconds () ^ TimGetTicks () ^ PrvRandom(0);
		} while (headerP->uniqueIDSeed <= dmRecordIDReservedRange);
		
	headerP->uniqueIDSeed = headerP->uniqueIDSeed << 12;
	headerP->uniqueIDSeed &= 0x00FFF000;		//	Isolate 12 seed bits.

	headerP->recordList.nextRecordListID = 0;
	headerP->recordList.numRecords = 0;


	//------------------------------------------------------------
	// Add the new database to the directory
	//------------------------------------------------------------
	// Get the database directory ID.
	err = PrvMemStoreInfo(cardNo, &dirID);
	if (err)
		goto Exit;

	ErrFatalDisplayIf(dirID == 0, "No DB list at DmCreateDatabase time");
	dirH = MemLocalIDToGlobal(dirID, cardNo);
	dirP = MemHandleLock(dirH);
	
	// Resize the directory chunk if necessary
	// DOLATER... this area is troublesome if new fails when there are
	// multiple storage heaps -- it does not try to reallocate from another.
	numEntries = dirP->numDatabases + 1;
	actSize = MemHandleSize(dirH);
	reqSize = OffsetOf(DatabaseDirType, databaseID) + numEntries * sizeof(DatabaseDirEntryType);
	if (reqSize > actSize) {
		MemHandleUnlock(dirH);
		ErrNonFatalDisplayIf(MemHandleLockCount(dirH) != 0, "dirH locked");
		err = MemHandleResize(dirH, reqSize);
		dirP = MemHandleLock(dirH);
		if (err)
			goto Exit;
		}


	// Add the database to the directory
	{
		UInt16 position = PrvBinarySearch(dirP, cardNo, headerP->type, headerP->creator, headerP->version, true);
		MemMove(&dirP->databaseID[position+1], &dirP->databaseID[position], (dirP->numDatabases-position) * sizeof(DatabaseDirEntryType));
		dirP->databaseID[position].baseID = MemHandleToLocalID(headerH);
		dirP->numDatabases = numEntries;
	}

	// Add a record to the launch database, if this database belongs:
	PrvUpdateLaunchDBEntry(NULL, headerP, MemHandleToLocalID(headerH), 0);
	
	err = errNone;
	
	
Exit:
	if (headerP)
		MemPtrUnlock(headerP);
	if (dirP)
		MemPtrUnlock(dirP);
	
	if (err) {
		if (headerH)
			MemHandleFree(headerH);
		}

	MemSemaphoreRelease(true);
	return err;
}



/************************************************************
 *
 *  FUNCTION: DmCreateDatabaseFromImage
 *
 *  DESCRIPTION: Can be called to create an entire database
 *			from a single resource that contains in image of the database. 
 *			Usually, this would be called from an app's reset action code during
 *			boot.
 *
 *  PARAMETERS: 
 *			bufferP - pointer to locked resource containing image of the database
 *
 *  RETURNS: void
 *
 *  CREATED: 3/13/95
 *
 *  BY: Ron Marianetti
 *
 *		vmk	12/29/97	Fixed to return non-zero error code in case of failure
 *		roger	11/2/99	Merged three DmSetDatabaseInfo calls into one.
 *
 *************************************************************/
Err	DmCreateDatabaseFromImage(MemPtr bufferP)
{
	UInt32			size;
	LocalID			dbID=0;
	DmOpenRef		dbP=0;
	Err				err = 0;
	UInt8 *			srcP;
	UInt8 *			dstP;
	DatabaseHdrPtr	hdrP = (DatabaseHdrPtr)bufferP;
	UInt16			i;
	UInt32			uniqueIDSeed;
	LocalID			appInfoID = 0;
		
		
	// Get the size and create it
	size = MemPtrSize(hdrP);
	err = DmCreateDatabase(0, (Char *)hdrP->name, hdrP->creator, hdrP->type, 
						hdrP->attributes & dmHdrAttrResDB);
	if (err)
		goto Exit;
	
	
	
	// Set Database info
	dbID = DmFindDatabase(0, (Char *)hdrP->name);
	if (!dbID) {
		err = dmErrCantFind;
		goto Exit;
		}
	
	
	// Open the new database
	dbP = DmOpenDBNoOverlay(0, dbID, dmModeReadWrite);
	if (!dbP) {
		err = dmErrCantOpen;
		goto Exit;
		}

	
	// The new database has been created with a random number in it.
	// We don't want the records copied into the new database to have
	// different unique id's between each hard reset or else the records
	// with the same contents but with different unique id's will appear
	// distinct to HotSync and so will accumulate with hard resets and syncs.
	// To stop this behavior we always set the seed to a constant number,
	// copy the records into the new database, and then we restore the
	// random seed.
	MemSemaphoreReserve(true);
	uniqueIDSeed = ((DmAccessPtr)dbP)->openP->hdrP->uniqueIDSeed;
	((DmAccessPtr)dbP)->openP->hdrP->uniqueIDSeed = dmDefaultRecordsID << 12;
	MemSemaphoreRelease(true);
	

	//------------------------------------------------------------
	// If it's a resource database, Add the Resources
	//------------------------------------------------------------
	if (hdrP->attributes & dmHdrAttrResDB) {
		RsrcEntryPtr	rsrcP;
		UInt32			resSize;
		MemHandle		newResH;

		// Pointer to first resource
		rsrcP  = (RsrcEntryPtr)(&hdrP->recordList.firstEntry);
		for (i=0; i<hdrP->recordList.numRecords; i++) {
		
			// Calculate the size of the resource
			if (i < hdrP->recordList.numRecords - 1) {
				resSize = (UInt32)rsrcP[1].localChunkID;
				resSize -= (UInt32)rsrcP[0].localChunkID;
				}
			else
				resSize = size - (UInt32)rsrcP[0].localChunkID;
			

			// Allocate the new resource
			newResH = DmNewResource(dbP, rsrcP->type, rsrcP->id, resSize);
			if (!newResH) {
				err = dmErrMemError;
				goto Exit;
				}
	
			// Copy the data in
			srcP = (UInt8 *)bufferP + (UInt32)rsrcP->localChunkID;
			dstP = (UInt8 *)MemHandleLock(newResH);
			DmWrite(dstP, 0, srcP, resSize);
			MemHandleUnlock(newResH);
	
			// Done with it
			DmReleaseResource(newResH);
	
			// next one
			rsrcP++;
			}
		}



	//------------------------------------------------------------
	// If it's a Record database, Add the Records
	//------------------------------------------------------------
	else {
		MemHandle		newRecH;
		RecordEntryPtr	recordP;
		UInt32			recSize;
		LocalID			thisLocalID;
		LocalID			nextLocalID;
		
		// SCL 7/21/98: revised appInfo code (based on code from Add2ROM tool).
		// Now allows for databases with ONLY appInfo but no records.

		// ErrFatal's are disabled since if we want to do error checking, there's
		// many places in which we can look for a "bad" PRC/PDB file (image).
		// DmCreateDatabaseFromImage should probably be re-written to look more like
		// what is in Add2ROM (and/or Poser) since those apps should be very picky
		// about the validity of PRC/PDB files they are asked to parse.
		// Similar code can also be found in Tools:MakeCard, and HotSync (desktop).
		
		// get pointer to first record
		recordP  = (RecordEntryPtr)(&hdrP->recordList.firstEntry);
		
		// get local ID of appInfo block, if any
		thisLocalID = hdrP->appInfoID;

		// If there's an appInfo, add that first
		if (thisLocalID) {
//			ErrFatalDisplayIf( (thisLocalID > size), "Bad appInfo offset!");

			// calculate appInfo size and check for errors
			if (hdrP->recordList.numRecords != 0) {
			
				// at least one record: size = LocalID of first record - LocalID of this one
				nextLocalID = (UInt32)recordP[0].localChunkID;
//				ErrFatalDisplayIf( (nextLocalID > size) || (nextLocalID < thisLocalID), "Bad appInfo size!");

				recSize = nextLocalID - thisLocalID;
				}
			else {
				// last record: size = size of PRC file - LocalID of this record
				recSize = size - thisLocalID;
				}

//			ErrFatalDisplayIf( (recSize>memMaxChunkAllocSize), "Bad appInfo size!");


			// Allocate the chunk
			newRecH = (MemHandle)PrvDmNewHandle(dbP, recSize);
			if (!newRecH) {
				err = dmErrMemError;
				goto Exit;
				}
				
			// Copy the data in
			srcP = (UInt8 *)bufferP + (UInt32)hdrP->appInfoID;
			dstP = (UInt8 *)MemHandleLock(newRecH);
			DmWrite(dstP, 0, srcP, recSize);
			MemHandleUnlock(newRecH);
			
			// Store it in the header
			appInfoID = MemHandleToLocalID(newRecH);
			}

		// If we supported sortInfo, we could do it here with code similar to the appInfo code.

		// Add Each of the records.
		for (i=0; i<hdrP->recordList.numRecords; i++) {
			UInt16	index;
			UInt16	attr;
			UInt32	id;
			
			// Calculate the size of the record
			if (i < hdrP->recordList.numRecords - 1) {
				recSize = (UInt32)recordP[1].localChunkID;
				recSize -= (UInt32)recordP[0].localChunkID;
				}
			else
				recSize = size - (UInt32)recordP[0].localChunkID;
			

			// Only add non-nil records. When the default databases are created
			//  they sometimes have nil records if the user had to delete a
			//  record they didn't want.
			if (recSize) {
				// Allocate the new record
				index = 0xFFFF;
				newRecH = (MemHandle)DmNewRecord(dbP, &index, recSize);
				if (!newRecH) {
					err = dmErrMemError;
					goto Exit;
					}
		
				// Copy the data in
				srcP = (UInt8 *)bufferP + (UInt32)recordP->localChunkID;
				dstP = (UInt8 *)MemHandleLock(newRecH);
				DmWrite(dstP, 0, srcP, recSize);
				MemHandleUnlock(newRecH);
						
				// Set the attributes
				attr = recordP->attributes;
				id = recordP->uniqueID[0];
				id = (id << 8) | recordP->uniqueID[1];
				id = (id << 8) | recordP->uniqueID[2];
				DmSetRecordInfo(dbP, index, &attr, &id);
		
				// Done with it
				DmReleaseRecord(dbP, index, true);
				}
		
			// next one
			recordP++;
			}
		}
		
		
	err = DmSetDatabaseInfo(0, dbID, 0,
				&hdrP->attributes, &hdrP->version, &hdrP->creationDate,
				&hdrP->modificationDate, &hdrP->lastBackupDate,
				&hdrP->modificationNumber, &appInfoID,
				0, &hdrP->type,
				&hdrP->creator);
	if (err)
		goto Exit;
	

	
	// Restore the database's uniqueIDSeed to it's random value.
	MemSemaphoreReserve(true);
	((DmAccessPtr)dbP)->openP->hdrP->uniqueIDSeed = uniqueIDSeed;
	MemSemaphoreRelease(true);
	
	// If we got to here, no error 
	DmCloseDatabase(dbP);
	dbP = 0;
		

Exit:
	if (dbP) {
		DmCloseDatabase(dbP);
		if (dbID)
			DmDeleteDatabase(0, dbID);
		}
		
	return err;
}



/************************************************************
 *
 *  FUNCTION: DmDeleteDatabase
 *
 *  DESCRIPTION: Deletes a database and all it's records
 *
 *  PARAMETERS: database ID
 *
 *  RETURNS: 0 if no error cj_kpcopyr
 *
 *  CREATED: 11/17/94 
 *
 *  BY: Ron Marianetti
 *
 *		vmk	9/10/97	Added code to delete alarms that may be associated
 *							with the database being deleted.
 *		roger	11/9/98	Cleared app and sort info fields after deletion for DmCloseDatabase checks.
 *		jesse	3/25/99	Allow context switches during NotifyMgr broadcast.
 *		jesse	10/05/99	Remove record to launcher database
 *		jesse	11/07/00	Add post-mortem database deleted notification
 *		jesse	11/11/00	Incorporate bug fix from Handspring (thanks Doug)
 *
 *************************************************************/
Err	DmDeleteDatabase(UInt16 cardNo, LocalID dbID)
{
	DmAccessPtr			accessP=0;
	DmOpenInfoPtr		openP = 0;
	DatabaseDirPtr		dirP=0;
	MemHandle			dirH = 0;
	LocalID				dirID = 0;
	UInt16				i;
	Err					err = 0;
	UInt32				reqSize;//, type, creator;
	UInt16				numRecords, index, category;//, attributes;
	MemHandle			dbH;
	MemHandle			h;
//	UInt8					dbName[dmDBNameLength];
	LocalID				newDBID;
	SysNotifyParamType	notify;
	SysNotifyDBDeletedType	*notifyDBInfoP = NULL;
	
	// If this is a ROM database, we can't delete it
	if (MemLocalIDKind(dbID) != memIDHandle) {
		return dmErrROMBased;
		}


	// Grab the semaphore 
	MemSemaphoreReserve(true);


	// Make sure this database is not open
	openP = (DmOpenInfoPtr)GDmOpenList;
	while(openP) {
		if (openP->hdrID == dbID && openP->cardNo == cardNo) break;
		openP = (DmOpenInfoPtr)openP->next;
		}

	// if it's open exit with error
	if (openP) { 
		openP = 0; 
		err = dmErrDatabaseOpen; 
		goto Exit;
		}

	
	// Make sure this database is not protected
	err = PrvDatabaseIsProtected(cardNo, dbID);
	if (err)
		goto Exit;
	
	// Fill out some of the SysNotifyDBDeletedType structure:
	notifyDBInfoP = MemPtrNew(sizeof(SysNotifyDBDeletedType));
	if(notifyDBInfoP == NULL)
	{
		err = memErrNotEnoughSpace;
		goto Exit;
	}
	notifyDBInfoP->oldDBID = dbID;
	notifyDBInfoP->cardNo = cardNo;
	
	// Tell the notify manager:
	
	// release the memory semaphore in case the broadcast
	// needs to be deferred to the UI thread (if this is a BG thread).
	// Note that if DmDeleteDatabase is ever called from a background
	// task with the semaphore already reserved, we are guaranteed to
	// deadlock when the NotifyMgr tries to do the broadcast from the 
	// UI thread.
	
/*	
	This should be commented out for pangea because there are some 
	unresolved issues dealing with bradcasts from background threads (like hotsync).
	MemSemaphoreRelease(true);
	ErrFatalDisplayIf(GHwrDataWELevel > 0, "Semaphore still reserved");
	SysNotifyDatabaseRemoved(cardNo, dbID);// Call the NotifyMgr routine to MemHandle everything
	MemSemaphoreReserve(true);	// Obtain the memory semaphore again
*/
	// However, we still need to let the NotifyMgr know, so it can
	// remove the database from its registration list.
	if(GSysNotifyGlobalsH != NULL)
		SysNotifyDatabaseRemoved(cardNo, dbID);// Call the NotifyMgr routine to MemHandle everything
		
	
	// First, delete all the records
	accessP = DmOpenDBNoOverlay(cardNo, dbID, dmModeReadWrite);
	if (!accessP) {
		err = dmErrCantOpen;
		goto Exit;
		}
	
	if (accessP->openP->resDB) {
		numRecords = DmNumResources(accessP);
		while(numRecords--) {
			err = DmRemoveResource(accessP, numRecords);
			if (err)
				break;
			}
		}

	else {
		numRecords = DmNumRecords(accessP);
		while(numRecords--) {
			err = DmRemoveRecord(accessP, numRecords);
			if (err)
				break;
			}
		}

	if (err)
		goto Exit;
	
	
	
	// Delete the sortInfo and appInfo chunks, if allocated
	openP = accessP->openP;
	if (openP->hdrP->appInfoID) {
		h = MemLocalIDToGlobal(openP->hdrP->appInfoID, openP->cardNo);
		MemHandleFree(h);
		openP->hdrP->appInfoID = 0;
		}
	if (openP->hdrP->sortInfoID) {
		h = MemLocalIDToGlobal(openP->hdrP->sortInfoID, openP->cardNo);
		MemHandleFree(h);
		openP->hdrP->sortInfoID = 0;
		}
	
	
	// Remove its record from the launch database, if this database belongs:
	category = PrvRemoveFromLaunchDatabase(0, openP->hdrP->type, openP->hdrP->creator, 
											(char*)openP->hdrP->name, openP->hdrP->attributes);
	
	// Save info for later - we will need to check for a ROM database with the same name.
//	StrCopy((char*)dbName, (char*)openP->hdrP->name);
//	type = openP->hdrP->type;
//	creator = openP->hdrP->creator;
//	attributes = openP->hdrP->attributes;
	
	// If we're deleting an overlay database, mark any related launch database records dirty:
	if(openP->hdrP->type == sysFileTOverlay) {
		PrvSetLaunchRecordFlags(dmLaunchFlagDirty, 0, 0, openP->hdrP->creator, 0);
		}
	
	// Fill out the SysNotifyDBDeletedType structure
	notifyDBInfoP->creator = openP->hdrP->creator;
	notifyDBInfoP->type = openP->hdrP->type;
	notifyDBInfoP->attributes = openP->hdrP->attributes;
	StrCopy((char*)notifyDBInfoP->dbName, (char*)openP->hdrP->name);
	
	// Close the database
	DmCloseDatabase(accessP);
	accessP = 0;
	
	
	//----------------------------------------------------------------
	// Remove it from the directory. 
	//----------------------------------------------------------------
	err = PrvMemStoreInfo(cardNo, &dirID);
	if (err) {
		err = dmErrCantFind;
		goto Exit;
		}

	dirH = MemLocalIDToGlobal(dirID, cardNo);
	dirP = MemHandleLock(dirH);
	for (i=0; i<dirP->numDatabases; i++) {
		if (dirP->databaseID[i].baseID == dbID)
			break;
		}
	if (i >= dirP->numDatabases) {
		err =  dmErrCantFind;
		goto Exit;
		}

	index = i;
	for (i=index; i<dirP->numDatabases-1; i++) 
		dirP->databaseID[i] = dirP->databaseID[i+1];

	dirP->numDatabases--;

	// Resize the directory chunk
	reqSize = OffsetOf(DatabaseDirType, databaseID) + dirP->numDatabases * sizeof(DatabaseDirEntryType);
	MemHandleUnlock(dirH);
	err = MemHandleResize(dirH, reqSize);
	dirP = MemHandleLock(dirH);


	// Free the database header chunk
	dbH = MemLocalIDToGlobal(dbID, cardNo);
	MemHandleFree(dbH);
	
	// Now, check and see if there is a ROM database with the same name.
	// If so, then we may need to add it back to the Launch database...
	if (PrvIsLauncherVisible(notifyDBInfoP->type, notifyDBInfoP->creator, notifyDBInfoP->attributes)) { // don't search all cards if we don't have to...
		
		// Search all cards for databases with same name:
		for(i=0; i<MemNumCards(); i++) {
			newDBID = DmFindDatabase(i, (char*)notifyDBInfoP->dbName);
			
			// If we find one, lock its header down & try to add it to the Launch database:
			if (newDBID) {
				MemPtr hdrP = MemLocalIDToGlobal(newDBID, i);
				Boolean isHndl = (MemLocalIDKind(newDBID) == memIDHandle);
				
				if (isHndl) {
					hdrP = MemHandleLock((MemHandle)hdrP);
					}
				
				PrvUpdateLaunchDBEntry(0, hdrP, newDBID, &category);
				
				if (isHndl) {
					MemPtrUnlock(hdrP);
					}
				
				}
			}
		}
	
	
Exit:
	if (dirP)
		MemPtrUnlock(dirP);
	
	if (accessP)
		DmCloseDatabase(accessP);
	
	MemSemaphoreRelease(true);
	
	//
	// Call the Alarm Manager to delete an alarm associated with
	// this databse, if any.  We're not concerned with the AlmSetAlarm
	// return value.	vmk	9/10/97
	//
	
	// Make sure the database was deleted (the database could have been open, locked, etc.)
	// and the Alarm Manager is initialized 
	if ( err == errNone && GAlmGlobalsP ) {
		AlmSetAlarm(cardNo, dbID, 0/*user ref value*/, 0/*zero clears alarm*/, true/*quiet*/);
		}
	
	// Broadcast post-mortem "database deleted" notification
	// if we deleted the db without error, and if NotifyMgr has been initialized.
	// This is a deferred broadcast in order to reduce stack usage problems,
	// so the notification will go out the next time EvtGetEvent() is called,
	// well after the database has been deleted:
	if(err == errNone	&& GSysNotifyGlobalsH != NULL) {
		notify.notifyType = sysNotifyDBDeletedEvent;
		notify.broadcaster = sysFileCSystem;
		notify.notifyDetailsP = notifyDBInfoP;
		notify.userDataP = NULL;
		notify.handled = false;
		notify.reserved2 = 0;
		
		SysNotifyBroadcastDeferred(&notify, sizeof(SysNotifyDBDeletedType));
		
		}
	
	// free info structure:
	if(notifyDBInfoP)	MemPtrFree(notifyDBInfoP);
	
	return err;
}



/************************************************************
 *
 *  FUNCTION: DmNumDatabases
 *
 *  DESCRIPTION: Returns the number of databases on a card
 *
 *  PARAMETERS: card number
 *
 *  RETURNS: Number of databases found
 *
 *  CREATED: 11/14/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt16	DmNumDatabases(UInt16 cardNo)
{

	UInt16			numEntries = 0;
	DatabaseDirPtr	dirP;
	MemHandle		dirH;
	LocalID			dirID;
	Err				err;
	
	// Get semaphore
	MemSemaphoreReserve(false);

	// Get the database directory ID for the RAM area
	err = PrvMemStoreInfo(cardNo, &dirID);
	if (!err && dirID) {
		dirH = MemLocalIDToGlobal(dirID, cardNo);
		if (dirH)  {
			dirP = MemHandleLock(dirH);
			numEntries = dirP->numDatabases;
			MemPtrUnlock(dirP);
			}
		}

	// Release semaphore
	MemSemaphoreRelease(false);
	return numEntries;			 
}



/************************************************************
 *
 *  FUNCTION: DmGetDatabase
 *
 *  DESCRIPTION: Returns a database header ID by index and card number
 *
 *  PARAMETERS: card number, index
 *
 *  RETURNS: ChunkID of database header
 *
 *  CREATED: 11/14/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
LocalID	DmGetDatabase(UInt16 cardNo, UInt16 index)
{
	UInt16			numEntries = 0;
	DatabaseDirPtr	dirP=0, *dirH=0;
	LocalID			dirID, hdrID=0;
	Err				err;


	// Get semaphore
	MemSemaphoreReserve(false);

	// Get the database directory ID.
	err = PrvMemStoreInfo(cardNo, &dirID);
	if (!err && dirID) {
		dirP = MemLocalIDToLockedPtr(dirID, cardNo);
		if (dirP)  {
			numEntries = dirP->numDatabases;
			}
		}

	// Get the database hdr ID by index
	if (index >= numEntries) {
		err =  dmErrIndexOutOfRange;
		goto Exit;
		}
	
	hdrID = dirP->databaseID[index].baseID;


Exit:
	if (dirP) MemPtrUnlock(dirP);
	
	MemSemaphoreRelease(false);
	if (err) return 0;
	else return hdrID;			 
}



/************************************************************
 *
 *  FUNCTION: DmGetNextDatabaseByTypeCreator
 *
 *  DESCRIPTION: Returns a database header ID and card number
 *   given the type and/or creator. This routine searches all
 *   memory cards for a match. 
 *
 *	  To start the search pass true for 'newSearch' 
 *   To continue a search where the previous one left off, pass false
 *		for 'newSearch'. When continuing a search stateInfoP MUST POINT
 *		to the same structure passed during the previous invocation!!!!!!
 *
 *   If the type parameter is missing, this routine can be called
 *   successively to return all databases of the given creator.
 *
 *   If the creator parameter is missing, this routine can be called
 *   successively to return all databases of the given type.
 *
 *   If the onlyLatestVers parameter is set, only the latest version
 *		of each database with a given creator/type pair is returned.
 *
 *	  If searching for the latest version and either type or creator 
 *    is a wildcard, this routine return the index of the next database
 *		which matches the search criteria which is not superseded by a newer
 *		version of that database with the same type and creator.
 *		Got That?????  (in other words, you'll get exactly one of each
 *		unique type/creator pair, and it will be the highest version,
 *		or the one on the highest store if versions are equal (RAM is higher than ROM))
 *
 *	  IMPLEMENTATION NOTES: Since the DBs on each store are sorted in the DBdir by
 *		type/creator/version, we can step through them to quickly find a
 *		given type/creator/version.  To do this efficiently, we first create
 *		an array of indexes into each store which point to the current
 *		candidates for matching the criteria.  The "lowest" candidate is
 *		selected from the set, where the keys are: type, creator, version,
 *		store.  ROM stores are considered lower than all RAM stores, and card
 *		0 stores are considered lower than card 1 stores.  This candidate is
 *		compared against the search criteria, and if it passes, it is
 *		returned.  If it doesn't, it's stepped over and the next "lowest"
 *		candidate is selected from the new set.
 *	
 *		To make "onlyLatestVers" work, we don't stop running the loop when a
 *		candidate is found.  Instead, keep track of the type/creator and
 *		continue to select "lowest" DBs until one that has a different
 *		type/creator is found.  At this point, we know the last candidate had
 *		the largest version (because the sort order includes versions), so it
 *		is returned.  The set of stores is ordered by ROM<RAM, card0<card1 so
 *		that the last version matched will be in RAM if equal versions are
 *		found.
 *		
 *  PARAMETERS: 
 *		newSearch 		 -> true if starting a new search
 *		stateInfoP		 -> if 'newSearch' is false, THIS MUST POINT TO SAME DATA
 *									used for previous invocation. 
 *		type		 		 -> type of database to search for, pass 0 as a wildcard
 *		creator   		 -> creator of database to search for, pass 0 as a wildcard
 *    onlyLatestVers  -> if true, only the latest version of each database with
 *								  a given type and creator will be returned.
 *		*cardNoP 		<-  On exit, the cardNo of the found database. 
 *		*dbIDP   		<-  database LocalID of the found database.
 *			
 *
 *	RETURNS: 	noErr (0)			- if no error
 *				  	dmErrCantFind 		- if no matches found
 *
 *	HISTORY:
 *		04/26/95	RM		Created by Ron Marianetti
 *		08/26/97	vmk	Fixed to allow the search to continue when a card only
 *							has one store.
 *    04/28/98 bob	Totally rewrote to take advantage of sorted directory lists
 *		10/28/99	kwk	Assume one DB dir list per card.
 *
 *************************************************************/
Err	DmGetNextDatabaseByTypeCreator(Boolean newSearch, DmSearchStatePtr stateInfoP,
			 UInt32	type, UInt32 creator, Boolean onlyLatestVers, 
			 UInt16 * cardNoP, LocalID* dbIDP)
{
	DmPrvSearchStatePtr	stateP = (DmPrvSearchStatePtr)stateInfoP;
	Int16 			i;
	UInt16 			cardNo;
	Int16				storeCount = 0;							// number of stores on device
	Int16				cStoreNum = -1;							// best "candidate" store for search, -1 for none
	DatabaseDirPtr	dirPs[MAX_STORES];						// databaseDirPtr for each store
	DatabaseHdrPtr	hdrPs[MAX_STORES];						// current databaseHdrPtr for each store

	// Moved these locals into stateP to save stack space.
	//	UInt16				cardNos[MAX_STORES];						// cardNo for each store

	//	Int16				latestVersionCandidateStore = -1;	// saved store for latest versions
	//	UInt32				latestVersionCandidateType;			// saved type of latest version candidate
	//	UInt32				latestVersionCandidateCreator;		// saved creator of latest version candidate
	
	
	if (sizeof(DmPrvSearchStateType) > sizeof(DmSearchStateType))
		ThisShouldGenerateALinkErrorDataMgr();
	
	// Grab semaphore
	MemSemaphoreReserve(false);
	
	// Locate the DB dir list for each card.
	for (cardNo = 0; cardNo < GMemCardSlots; cardNo++) {
		LocalID dirID;
		Err err;
		err = PrvMemStoreInfo(cardNo, &dirID);
		if (!err && dirID) {
			stateP->cardNos[storeCount] = cardNo;
			dirPs[storeCount++] = MemLocalIDToLockedPtr(dirID, cardNo);
			}
		}

	// initialize the indexes to all the stores
	// finds the first matching record in each store, vastly
	// speeding up searches where type is not a wildcard
	if (newSearch)
		for (i = 0; i < storeCount; i++)
			// Find first entry, searching for version = 0, in ROM (sorts before RAM)
			stateP->indexes[i] = PrvBinarySearch(dirPs[i], stateP->cardNos[i], type, creator, 0, false);

	// lock down the "current" databaseHdrs for each store
	// loop is deliberately "backwards" so that cStoreNum will be
	// as low as possible to start with, to ensure RAM db's are matched last
	// in onlyLatestVers searches.
	for (i = storeCount-1; i >= 0; i--)
		if (stateP->indexes[i] >= dirPs[i]->numDatabases)
			hdrPs[i] = NULL;
		else
		{
			hdrPs[i] = MemLocalIDToLockedPtr(dirPs[i]->databaseID[stateP->indexes[i]].baseID, stateP->cardNos[i]);
			cStoreNum = i;
		}

	stateP->latestVersionCandidateStore = -1;
	
	// loop to search
	while (cStoreNum >= 0)
	{
		// find the first candidate from all the stores
		for (i = 0; i < storeCount; i++)
			if ((cStoreNum != i) && (hdrPs[i] != NULL))
			{
				if (hdrPs[i]->type < hdrPs[cStoreNum]->type)
					cStoreNum = i;
				else if (hdrPs[i]->type == hdrPs[cStoreNum]->type)
				{
					if (hdrPs[i]->creator < hdrPs[cStoreNum]->creator)
						cStoreNum = i;
					else if (hdrPs[i]->creator == hdrPs[cStoreNum]->creator)
					{
						if (hdrPs[i]->version < hdrPs[cStoreNum]->version)
							cStoreNum = i;
					}
				}
			}
		
		// if we had a match last time through we're looping just to see if there's
		// a later version.  If there IS, then type and creator will match the old
		// candidate.  If there's no match, we've moved on to a new type/creator, so
		// return the old match.
		if ((stateP->latestVersionCandidateStore >= 0)
		&& ((hdrPs[cStoreNum]->type != stateP->latestVersionCandidateType)
			|| (hdrPs[cStoreNum]->creator != stateP->latestVersionCandidateCreator)))
			goto Exit;
		
		// check if current candidate matches search
		if (type && (hdrPs[cStoreNum]->type > type))
		{
			// no more of this type!
			cStoreNum = -1;
			goto Exit;
		}
		else if (type && (hdrPs[cStoreNum]->type < type))
			stateP->indexes[cStoreNum]++;
		else if (creator && (hdrPs[cStoreNum]->creator != creator))
		{
			// no match, skip this one
			stateP->indexes[cStoreNum]++;
			// NOTE: can't return in this case, because type may be a wildcard,
			// and more DBs with matching creator may occur later in list, ends
			// up being a linear search of entire DB.  Not worth it to test if
			// type is not a wildcard, because if that's the case we'll quickly fall
			// out of the loop next pass when the candidate type doesn't match
		}
		else if (onlyLatestVers)
		{
			// keep track of the current store index, and type/creator
			stateP->latestVersionCandidateStore = cStoreNum;
			stateP->latestVersionCandidateType = hdrPs[cStoreNum]->type;
			stateP->latestVersionCandidateCreator = hdrPs[cStoreNum]->creator;
			
			// move on to the next entry
			stateP->indexes[cStoreNum]++;
		}
		else
			// match found!
			goto Exit;
		
		// update the databaseHrdPtr for the candidate store
		MemPtrUnlock(hdrPs[cStoreNum]);
		if (stateP->indexes[cStoreNum] >= dirPs[cStoreNum]->numDatabases)
		{
			hdrPs[cStoreNum] = 0;

			// find a new candidate, make sure we stop on the lowest cStoreNum
			// so identical type/creator/version searches w/onlyLatestVers will favor RAM stores
			cStoreNum = -1;
			for (i = storeCount-1; i >= 0; i--)
				if (hdrPs[i] != NULL)
					cStoreNum = i;
		}
		else
			hdrPs[cStoreNum] = MemLocalIDToLockedPtr(dirPs[cStoreNum]->databaseID[stateP->indexes[cStoreNum]].baseID,
																  stateP->cardNos[cStoreNum]);
	}	// end of while, find the next candidate!

		
Exit:

	// in case we ran out of DBs in a search for a latest candidate
	if (stateP->latestVersionCandidateStore >= 0)
	{
		// have to decrement the old match store to undo the increment done below
		stateP->indexes[stateP->latestVersionCandidateStore]--;
		cStoreNum = stateP->latestVersionCandidateStore;
	}
	
	// If we found a match, save the info, otherwise store 0s
	if (cStoreNum >= 0)
	{
		*cardNoP = stateP->cardNos[cStoreNum];
		*dbIDP = dirPs[cStoreNum]->databaseID[stateP->indexes[cStoreNum]].baseID;
		
		// skip the entry we're returning, so next time this fn is called with
		// the same state data, we'll start on the next record.
		stateP->indexes[cStoreNum]++;
	}
	else
	{
		*dbIDP = 0;
		*cardNoP = 0;
	}
	
	// unlock any locked databaseDir or databaseHdr objects
	for (i = 0; i < storeCount; i++)
	{
		if (hdrPs[i] != NULL)
			MemPtrUnlock(hdrPs[i]);
		if (dirPs[i] != NULL)
			MemPtrUnlock(dirPs[i]);
	};
	
	// Release semahpore
	MemSemaphoreRelease(false);

	if (cStoreNum >= 0)
		return 0;
	else
		return dmErrCantFind;
}



/************************************************************
 *
 *  FUNCTION: DmFindDatabase
 *
 *  DESCRIPTION: Searches for a database by name
 *
 *  PARAMETERS: card number, name pointer
 *
 *  RETURNS: LocalID of database header, or 0 if not found.
 *
 *  CREATED: 11/14/94 
 *
 *  BY: Ron Marianetti
 *
 *	HISTORY:
 *		4/6/2000	ryw	use PrvSetLastErr for error reporting
 *************************************************************/
LocalID	DmFindDatabase(UInt16 cardNo, const Char* nameP)
{
	LocalID			foundID=0;
	Err				err = dmErrCantFind;

	// Grab semaphore
	MemSemaphoreReserve(false);

	// Search our DB list on cardNo for match in RAM store
	foundID = PrvFindDatabaseInStore(cardNo, memRamStoreNum, nameP);
	
	// also search the ROM store!
	if (foundID == 0)
		foundID = PrvFindDatabaseInStore(cardNo, memRomStoreNum, nameP);

	if (foundID != 0)
		err = errNone;
	
Exit:
	// Release semaphore
	MemSemaphoreRelease(false);
	
	PrvSetLastErr(err);
	if (err) return 0;
	else return foundID;			 
}


/************************************************************
 *
 * FUNCTION: DmFindDatabaseWithTypeCreator
 *
 * DESCRIPTION: Searches for a database by name, using type
 *		and creator info to speed up the search. This is about
 *		5.5x faster on the average, for a normal (single locale)
 *		ROM. For an EFIGS ROM, the improvement will be much
 *		more significant. Mostly this routine is used by the
 *		Overlay Mgr to find overlays.
 *
 * PARAMETERS:
 *		cardNo	 ->	Card number that contains the DB.
 *		nameP		 ->	Ptr to DB name.
 *		type		 ->	Database type (no wildcards)
 *		creator	 ->	Database creator
 *
 * RETURNS:
 *		LocalID of database header, or 0 if not found.
 *
 *	HISTORY:
 *		05/17/00	kwk	Created by Ken Krugler.
 *		05/21/00	kwk	Use StrCompareAscii vs. PrvDBNameCompare
 *
 *************************************************************/
LocalID DmFindDatabaseWithTypeCreator(UInt16 cardNo, const Char* nameP,
			UInt32 type, UInt32 creator)
{
	Err				err = dmErrCantFind;
	SysAppInfoPtr	appInfoP;
	DatabaseDirPtr	dirP;
	LocalID			dirID;
	LocalID			foundID = 0;
	UInt16				i;
	DatabaseHdrPtr	hdrP;
	Boolean			done = false;
	Int16				nameLen = StrLen(nameP);
	
	// Grab semaphore
	MemSemaphoreReserve(false);

	// Get the directory ID for this store
	err = PrvMemStoreInfo(cardNo, &dirID);
	if ((err != errNone) || (dirID == 0))
	{
		goto Exit;
	}

	// Get pointer to the directory info
	dirP = MemLocalIDToLockedPtr(dirID, cardNo);

	// Figure out the first entry in the list which has the correct
	// type and creator (use 0 for version, and ask for the ROM DB).
	// Loop over the DBs, until we either have found a match in RAM,
	// or we reach the end, or the type/creator no longer matches.
	for (i = PrvBinarySearch(dirP, cardNo, type, creator, 0, false); i < dirP->numDatabases && !done; i++)
	{
		LocalID dbID = dirP->databaseID[i].baseID;
		hdrP = MemLocalIDToLockedPtr(dbID, cardNo);
		
		if ((hdrP->type != type) || (hdrP->creator != creator))
		{
			done = true;
		}
		
		// Optimization for overlay support - often we have a number of
		// DB names which match, other than the locale suffix. Making sure
		// the last byte matches is a quick way of pre-screening, before
		// calling the comparison routine.
		else if ((nameP[nameLen - 1] == hdrP->name[nameLen - 1])
				&& (StrCompareAscii(nameP, (Char *)hdrP->name) == 0))
		{
			// We found a DB. If it's in RAM, then we know we're done,
			// otherwise we need to keep looking.
			foundID = dbID;
			if (MemLocalIDKind(dbID) == memIDHandle)
			{
				done = true;
			}
		}
		
		MemPtrUnlock(hdrP);
	}
	
	MemPtrUnlock(dirP);

	if (foundID != 0)
	{
		err = errNone;
	}

Exit:
	// Release semaphore and set dmLastErr
	MemSemaphoreRelease(false);
	appInfoP = PrvGetAppInfo();
	appInfoP->dmLastErr = err;
	
	return(foundID);			 
} // DmFindDatabaseWithTypeCreator


/************************************************************
 *
 *	FUNCTION:	PrvFindDatabaseInStore, private
 *
 *	DESCRIPTION: Searches for a database by name in our DB
 *		list. Only consider ROM or RAM-based DBs, depending
 *		on which storeNum is passed.
 *
 *	PARAMETERS: card number, storenumber, name pointer
 *
 *	RETURNS: LocalID of database header, or 0 if not found.
 *
 *	HISTORY:
 *		11/14/94	RM		Created by Ron Marianetti
 *		10/08/99	kwk	Since we only have a single list now,
 *							skip ROM or RAM DBs based on storeNum.
 *
 *************************************************************/
LocalID	PrvFindDatabaseInStore(UInt16 cardNo, UInt16 storeNum, const Char* nameP)
{
	DatabaseDirPtr	dirP=0;
	LocalID			dirID, foundID=0;
	Err				err;
	UInt16				i;
	DatabaseHdrPtr	hdrP;
	LocalIDKind		targetKind;

	// Get the directory ID for this store
	err = MemStoreInfo(cardNo, storeNum, 0, 0, 0, 0, 0, 0, 0, 0, &dirID);
	if (err || !dirID) return 0;

	// Get pointer to the directory info
	dirP = MemLocalIDToLockedPtr(dirID, cardNo);

	// Figure out which type of entry in our DB list we want to find.
	if (storeNum == memRomStoreNum) {
		targetKind = memIDPtr;
		}
	else {
		targetKind = memIDHandle;
		}
	
	// Compare the name of each database
	for (i=0; i<dirP->numDatabases && !foundID; i++) {
		LocalID dbID = dirP->databaseID[i].baseID;
		if (MemLocalIDKind(dbID) != targetKind)
			continue;
		
		hdrP = MemLocalIDToLockedPtr(dbID, cardNo);

		if (StrCompareAscii(nameP, (Char *)hdrP->name) == 0) 
		  	foundID = dirP->databaseID[i].baseID;
		  	
		MemPtrUnlock(hdrP);
		}
		
	MemPtrUnlock(dirP);

Exit:
	if (err) return 0;
	else return foundID;			 
}


/************************************************************
 *
 *  FUNCTION: DmDatabaseInfo
 *
 *  DESCRIPTION: Retrieves database information
 *
 *  PARAMETERS: a ton of pointers, any number of them can be nil
 *
 *  RETURNS: 0 if successful
 *
 *  CREATED: 11/15/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err		DmDatabaseInfo(UInt16 cardNo, LocalID	dbID, Char * nameP,
					UInt16 * attributesP, UInt16 * versionP, UInt32 * crDateP,
					UInt32 *	modDateP, UInt32 * bckUpDateP,
					UInt32 *	modNumP, LocalID* appInfoIDP,
					LocalID* sortInfoIDP, UInt32 * typeP,
					UInt32 * creatorP)
{
	DatabaseHdrPtr	hdrP;

	// Get a pointer to the database header
	hdrP = MemLocalIDToLockedPtr(dbID, cardNo);
	if (!hdrP) return dmErrInvalidParam;

	if (nameP) 
		StrCopy(nameP, (Char *)hdrP->name);

	if (attributesP) 
		*attributesP = hdrP->attributes;

	if (versionP) 
		*versionP = hdrP->version;

	if (crDateP) 
		*crDateP = hdrP->creationDate;

	if (modDateP) 
		*modDateP = hdrP->modificationDate;

	if (bckUpDateP) 
		*bckUpDateP = hdrP->lastBackupDate;

	if (modNumP) 
		*modNumP = hdrP->modificationNumber;

	if (appInfoIDP) 
		*appInfoIDP = hdrP->appInfoID;

	if (sortInfoIDP) 
		*sortInfoIDP = hdrP->sortInfoID;

	if (typeP) 
		*typeP = hdrP->type;

	if (creatorP) 
		*creatorP = hdrP->creator;
		
	MemPtrUnlock(hdrP);

	return 0;
}


/************************************************************
 *
 *	FUNCTION: DmSetDatabaseInfo
 *
 *	DESCRIPTION: Sets database information.
 *					When appInfoID or sortInfoID are changed using
 *					 this call, the old chunkID (if any) will be marked as an orphan
 *					 chunk and the new chunk ID will be un-orphaned.
 *					Consequently, you shouldn't replace an existing appInfoID or
 *					 sortInfoID if that chunk has already been attached to another
 *              database.
 *
 *	PARAMETERS: a ton of pointers, any number of them can be nil
 *
 *	RETURNS: 0 if successful
 *
 *	HISTORY:
 *		11/15/94	RM		Created by Ron Marianetti
 *		09/18/97	vmk	Added check for name length and uniqueness
 *		10/31/97	vmk	Don't force modification number change for backup date change;
 *							and don't rev mod number if mod number is being set by the caller;
 *							if mod number is bumped up, update the modification date also
 *		06/11/99	rsf	Fixed setting mod number and something else (besides date)
 *							didn't update the mod date.
 *		10/05/99	jed	Update launcher database
 *		12/11/00	grant	Check that the database is not it ROM.
 *
 *************************************************************/
Err		DmSetDatabaseInfo(UInt16 cardNo, LocalID	dbID, const Char * nameP,
					UInt16 * attributesP, UInt16 * versionP, UInt32 * crDateP,
					UInt32 *	modDateP, UInt32 * bckUpDateP,
					UInt32 *	modNumP, LocalID* appInfoIDP,
					LocalID* sortInfoIDP, UInt32 * typeP,
					UInt32 * creatorP)
{
	DatabaseHdrPtr	hdrP = NULL;
	Err				err = errNone;
	Boolean			updateModNum = false;
	MemHandle		h;
	Int16				len;
	Boolean			updateSortOrder = false;
	LocalID 			dirID;
	DatabaseDirPtr dirP;
	UInt16 			i;
	Boolean			wasLauncherVisible, willBeLauncherVisible;
	DmOpenRef		launchDBRef = 0;
	UInt16			savedCategory = 0xFFFF;
	
	// If this is a ROM database, we can't set info on it
	if (MemLocalIDKind(dbID) != memIDHandle)
		return dmErrROMBased;

	// Enable writes
	MemSemaphoreReserve(true);
	
	// If changing the name, make sure a database with this name doesn't already exist in
	// the RAM store and the name is valid
	if ( nameP ) {
		// Validate name length
		len = StrLen(nameP);
		if ( len == 0 || len >= dmDBNameLength ) {
			err = dmErrInvalidDatabaseName;
			goto Exit;
			}
		
		// Check name for uniqueness in the RAM store
		if ( PrvFindDatabaseInStore(cardNo, memRamStoreNum, nameP) ) {
			err = dmErrAlreadyExists;
			goto Exit;
			}
		}
	
	// Get a pointer to the database header
	hdrP = MemLocalIDToLockedPtr(dbID, cardNo);
	if (!hdrP) {
		err= dmErrInvalidParam;
		goto Exit;
		}
	
	// Determine if the database was in the launch database before the call,
	// and if it should be after the call.
	wasLauncherVisible = PrvIsLauncherVisible(hdrP->type, hdrP->creator, hdrP->attributes);
	willBeLauncherVisible = PrvIsLauncherVisible(typeP ? (*typeP) : (hdrP->type), 
													creatorP ? (*creatorP) : (hdrP->creator), 
													attributesP ? (*attributesP) : (hdrP->attributes));
	
	// If we're setting info on the Launch database, or 
	// If we're not setting any info that the Launch database cares about,
	// Then don't open the LaunchDB - it doesn't need to be updated.
	if((typeP == 0 && creatorP == 0 && versionP == 0 && attributesP == 0 && nameP == 0) || 
		(hdrP->type == dmLaunchDatabaseType && hdrP->creator == dmLaunchDatabaseCreator))
	{
		launchDBRef = 0;
	}
	else
	{
		launchDBRef = PrvOpenLaunchDatabase();
	}
	
	
	// Save ourselves checking for the little hard-to-find cases, and just
	// remove/add it every time (if its visible and, thus, in the database).
	// We check 'wasLauncherVisible' to avoid a linear search through the database
	// in the case that its an overlay database.  In this case, we will mark related 
	// entries dirty below when we call PrvUpdateLaunchDBEntry().
	if(wasLauncherVisible && launchDBRef) 
		savedCategory = PrvRemoveFromLaunchDatabase(launchDBRef, hdrP->type, hdrP->creator, (char*)hdrP->name, hdrP->attributes);
		
	// Now, update all the info:
	if (nameP) {
		StrCopy((Char *)hdrP->name, nameP);
		updateModNum = true;
		}

	if (attributesP) {
		hdrP->attributes = *attributesP;
		updateModNum = true;
		}

	if (versionP) {
		if (hdrP->version != *versionP)
			updateSortOrder = true;
		hdrP->version = *versionP;
		updateModNum = true;
		}

	if (crDateP) {
		hdrP->creationDate = *crDateP;
		updateModNum = true;
		}

	if (modDateP) {
		hdrP->modificationDate = *modDateP;
		// we're not changing updateModNum here -- if this is the only field being
		// changed, then by default the mod num will not be revved
		}

	if (bckUpDateP) {
		hdrP->lastBackupDate = *bckUpDateP;
		// we're not changing updateModNum here -- if this is the only field being
		// changed, then by default the mod num will not be revved
		}

	if (appInfoIDP) {
		// If there's an existing appInfo block, orphan it
		if (hdrP->appInfoID) {
			h = MemLocalIDToGlobal(hdrP->appInfoID, cardNo);
			MemHandleSetOwner(h, dmOrphanOwnerID);
			}
		// Give the new chunk a valid owner 
		hdrP->appInfoID = *appInfoIDP;
		if (*appInfoIDP) {
			h = MemLocalIDToGlobal(*appInfoIDP, cardNo);
			MemHandleSetOwner(h, dmRecOwnerID);
			}
		updateModNum = true;
		}

	if (sortInfoIDP) {
		// If there's an existing appInfo block, orphan it
		if (hdrP->sortInfoID) {
			h = MemLocalIDToGlobal(hdrP->sortInfoID, cardNo);
			MemHandleSetOwner(h, dmOrphanOwnerID);
			}
		// Give the new chunk a valid owner 
		hdrP->sortInfoID = *sortInfoIDP;
		if (*sortInfoIDP) {
			h = MemLocalIDToGlobal(*sortInfoIDP, cardNo);
			MemHandleSetOwner(h, dmRecOwnerID);
			}
		updateModNum = true;
		}

	if (typeP) {
		if (hdrP->type != *typeP)
			updateSortOrder = true;
		hdrP->type = *typeP;
		updateModNum = true;
		}

	if (creatorP) {
		if (hdrP->creator != *creatorP)
			updateSortOrder = true;
		hdrP->creator = *creatorP;
		updateModNum = true;
		}
	
	// This case MUST be last because of the special case (the mod number itself being set)
	if (modNumP) {
		hdrP->modificationNumber = *modNumP;
		}

	// Bump up the mod number and update the modification date
	if ( updateModNum ) {
		// don't rev modification number when setting the mod number
		if ( !modNumP )
			hdrP->modificationNumber++;
		
		// Don't update the date if it was set
		if ( !modDateP )
			hdrP->modificationDate = TimGetSeconds();
		}
	
	PrvMemStoreInfo(cardNo, &dirID);
	dirP = MemLocalIDToLockedPtr(dirID, cardNo);

	// May have to re-sort the database directory if type, creator, or version changed
	if ( updateSortOrder ) {
		UInt16 pos;
		
		// find the old entry in the DB directory
		for (i=0; i < dirP->numDatabases; i++) {
			if (dirP->databaseID[i].baseID == dbID) {
				pos = i;
				break;
				}
			}
		
		// remove old directory entry
		MemMove(&dirP->databaseID[pos], &dirP->databaseID[pos+1], (dirP->numDatabases-pos) * sizeof(DatabaseDirEntryType));
		dirP->numDatabases--;
		
		// insert new directory entry. We can only set info on RAM-based DBs,
		// so the ramDB param to PrvBinarySearch must be true.
		pos = PrvBinarySearch(dirP, cardNo, hdrP->type, hdrP->creator, hdrP->version, true);
		MemMove(&dirP->databaseID[pos+1], &dirP->databaseID[pos], (dirP->numDatabases-pos) * sizeof(DatabaseDirEntryType));
		dirP->databaseID[pos].baseID = dbID;
		dirP->numDatabases++;
		}
		
	// Finally, add it back to the launch database (or update it)
	if(launchDBRef)
	{
		if(savedCategory != 0xFFFF)
			PrvUpdateLaunchDBEntry(launchDBRef, hdrP, dbID, &savedCategory);
		else
			PrvUpdateLaunchDBEntry(launchDBRef, hdrP, dbID, NULL);
	}
	
	MemPtrUnlock(dirP);
	MemPtrUnlock(hdrP);

Exit:
	// Restore  write-protection
	MemSemaphoreRelease(true);
	
	if(launchDBRef) DmCloseDatabase(launchDBRef);

	return err;
}


/************************************************************
 *
 *	FUNCTION:	DmDatabaseProtect
 *
 *	DESCRIPTION:
 *		 This routine can be used to prevent a database from being deleted (by passing
 *		 true for 'protect'). It will increment the protect count if 'protect' is true
 *		 and decrement it if 'protect' is false. This is used by code that wants to
 *		 keep a particular record or resource in a database locked down but doesn't
 *		 want to keep the database open. This information is keep in the dynamic heap so
 *		 all databases are "unprotected" at system reset. 
 *
 *	PARAMETERS: 
 *		cardNo 			- card number of database to protect/unprotect
 *		dbID				- LocalID of database to protect/unprotect
 *		protect			- if true, protect count will be incremented
 *							  if false, protect count will be decremented.
 *
 *	RETURNS: 0 if successful
 *
 *	HISTORY:
 *		07/23/96	RM		Created by Ron Marianetti
 *		10/01/99	Ludovic	Check the database was not previously deleted
 *							Check the database is not in ROM
 *		10/06/99 Eric Lapuyade : added a check to detect under deprotecting.
 *		10/09/99	kwk	Use non-fatal alert for invalid card # case, since
 *							we can return an error code.
 *
 *************************************************************/
Err		DmDatabaseProtect(UInt16 cardNo, LocalID	dbID, Boolean protect)
{
	Err					err=0;
	DmProtectEntryPtr	listP=0, entryP;
	UInt16				numEntries, entryNo;
	Boolean				found = false;
	DatabaseDirPtr		dirP=0;
	MemHandle			dirH = 0;
	LocalID				dirID = 0;
	UInt16				i;
	
	// Check the cardno. Although we only store it here, it looks like some other functions
	// really don't like it to store a bad number.
	if (cardNo >= GMemCardSlots) {
		ErrNonFatalDisplay("Invalid Card #");
		return memErrCardNotPresent;
		}
	
	// If this is a ROM database, we can't protect it
	if (MemLocalIDKind(dbID) != memIDHandle)
		return dmErrROMBased;

	//----------------------------------------------------------------
	// Check if the DB is in the directory
	//----------------------------------------------------------------
	err = PrvMemStoreInfo(cardNo, &dirID);

	if (!err) {
		dirH = MemLocalIDToGlobal(dirID, cardNo);
		dirP = MemHandleLock(dirH);

		for (i=0; i<dirP->numDatabases; i++) {
			if (dirP->databaseID[i].baseID == dbID)
				break;
			}

		if (i >= dirP->numDatabases)
			err = dmErrCantFind;

		MemHandleUnlock(dirH);
	}
	
	if (err)
		return err;

	// Make sure we have a list to work with
	if (!GDmProtectListH) {
		GDmProtectListH = MemHandleNew(sizeof(DmProtectEntryType));
		if (!GDmProtectListH) {err = memErrNotEnoughSpace; goto ExitDefaultErr;}
		MemHandleSetOwner(GDmProtectListH, 0);
		
		// Init it
		listP = MemHandleLock(GDmProtectListH);
		MemSet(listP, sizeof(DmProtectEntryType), 0);
		
		// Put this database in first entry
		listP->cardNo = cardNo;
		listP->dbID = dbID;

		MemPtrUnlock(listP);
		}
		
	// Get the size of the list
	listP = MemHandleLock(GDmProtectListH);
	numEntries = MemPtrSize(listP) / sizeof(DmProtectEntryType);
	
	// Look for an existing entry in the list
	for (entryP = listP, entryNo=0; entryNo<numEntries; entryNo++, entryP++) {
		if (entryP->cardNo ==  cardNo && entryP->dbID == dbID) {
			found = true;
			break;
			}
		}
		
	// if protecting, we must have an entry so grow the list
	if (!found) {
		if (protect) {
			numEntries++;
			MemPtrUnlock(listP);
			listP = 0;
			
			err = MemHandleResize(GDmProtectListH, numEntries * sizeof(DmProtectEntryType));
			if (err) goto ExitDefaultErr;
			
			// Init new list
			listP = MemHandleLock(GDmProtectListH);
			
			// Point to new entry
			entryNo = numEntries-1;
			entryP = &listP[entryNo];
			entryP->cardNo = cardNo;
			entryP->dbID = dbID;
			entryP->protectCount = 0;
			}
		else {
			// The entry does not exists. Check that we are not trying to deprotect it.
			
			ErrNonFatalDisplay ("DmDatabaseProtect tried to deprotect unprotected database");
			err = dmErrDatabaseNotProtected;
			goto Exit;
			}
		}
		
	// modify protect count
	if (protect) entryP->protectCount++;
	else if (entryP->protectCount) entryP->protectCount--;
	
	
	// If protect count is now 0, get rid of this entry. This makes sure that
	// an entry will not survive once its protect count goes to zero. It will
	// therefore not be possible to under deprotect it.
	if (!entryP->protectCount) {
		MemMove(&listP[entryNo], &listP[entryNo+1], 
					(numEntries - entryNo - 1) * sizeof(DmProtectEntryType));
		numEntries--;
		if (numEntries)
			MemPtrResize(listP, numEntries * sizeof(DmProtectEntryType));
		else {
			MemHandleFree(GDmProtectListH);
			GDmProtectListH = 0;
			listP = 0;
			}
			
		}
		
ExitDefaultErr:
	ErrNonFatalDisplayIf(err, "err in DmDatabaseProtect");

Exit:
	if (listP) MemPtrUnlock(listP);
	
	return err;
}


/************************************************************
 *
 *  FUNCTION: DmGetDatabaseLockState
 *
 *  DESCRIPTION: Return information about locked resources or records in
 *	 a database along with the number of busy records.  This is intended to
 *  enable tracking of app behaviour during debug versions.
 *		
 *  PARAMETERS: 
 *			dbR  - open ref of record or resource database
 *			highest  - variable to set with the highest number of locks found
 *			count  - the number of records or resources with the highest lock count
 *			busy  - the number of records with their busy flag set
 *			
 *  RETURNS: 
 *			highest  - variable to set with the highest number of locks found
 *			count  - the number of records or resources with the highest lock count
 *			busy  - the number of records with their busy flag set
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	8/5/98	Initial version
 *
 *************************************************************/
void DmGetDatabaseLockState(DmOpenRef dbR, UInt8 * highest, UInt32 * count, UInt32 * busy)
{
	DmAccessPtr dbP = (DmAccessPtr)dbR;
	DmOpenInfoPtr openP;
	Boolean isResDB;
	UInt16 index;
	UInt16 numRecords;
	MemHandle	h;
	LocalID id;
	RsrcEntryPtr resourceP;
	RecordEntryPtr	recordP;
	UInt8 lockCount;
	UInt8 lockCountHighest = 0;
	UInt32 lockCountCount = 0;
	UInt32 busyCount = 0;

	ECDmValidateDmOpenRef(dbP);

	openP = dbP->openP;

	// If the database is in ROM then nothing is locked.
	if (!openP->hdrMasterP)	{
		isResDB = openP->resDB;
		numRecords = openP->numRecords;
		for (index = 0; index < numRecords; index++)
			{
			if (isResDB) {
				resourceP = PrvRsrcEntryPtr(openP, index, NULL);
				id = resourceP->localChunkID;
				}
			else {
				recordP = PrvRecordEntryPtr(openP, index, NULL);
				id = recordP->localChunkID;
				
				if (recordP->attributes	& dmRecAttrBusy)
					busyCount++;
				}
			
			// Get a MemHandle to the record/resource, then get it's lock count.
			// id can be zero when a record is deleted.
			if (id) {
				h = MemLocalIDToGlobal(id, openP->cardNo);
				lockCount = MemHandleLockCount(h);
				
				// Record stats.
				if (lockCount > lockCountHighest) {
					lockCountHighest = lockCount;		// for having a place to set a breakpoint
					lockCountCount = 1;
					}
				else if (lockCount == lockCountHighest) {
					lockCountCount++;
					}
				}
			}
		
		if (!isResDB) {
			if (openP->hdrP->appInfoID) {
				// Get a MemHandle to the app info block, then get it's lock count.
				h = MemLocalIDToGlobal(openP->hdrP->appInfoID, openP->cardNo);
				lockCount = MemHandleLockCount(h);
				
				// Record stats.
				if (lockCount > lockCountHighest) {
					lockCountHighest = lockCount;		// for having a place to set a breakpoint
					lockCountCount = 1;
					}
				else if (lockCount == lockCountHighest) {
					lockCountCount++;
					}
				}
			}
		}
	
	if (highest)
		*highest = lockCountHighest;
	if (count)
		*count = lockCountCount;
	if (busy)
		*busy = busyCount;
}


/************************************************************
 *
 *  FUNCTION: DmDatabaseSize
 *
 *  DESCRIPTION: Returns the size of bytes and the total number of
 *		records in a database.  This call is intended to be called for
 *  	apps which the caller controls and knows that the sizes of 
 *		memory used won't change during this call.  If this is not the
 *		case then MemSemaphoreReserve(false) should called prior to this
 *		call.
 *
 *		This routine could be sped up twice as fast if it was to grab
 *		the write memory semaphore for every n records (n = 32).
 *
 *  PARAMETERS: 
 *		cardNo			- card number of database
 *		dbID				- database ID
 *		*numRecordsP	- number of records returned here
 *		*totalBytesP	- total size returned here, including overhead
 *		*dataBytesP		- data size only, does not include overhead bytes.
 *
 *  RETURNS: 0 if no error
 *		 numRecordsP, totalBytesP, dataBytesP - set if not NULL
 *
 *  CREATED: 11/16/94 
 *
 *  BY: Ron Marianetti
 *
 *			roger	7/15/96	Made the results optional
 *			roger	6/26/98	Optimized RAM databases to avoid chunk locking/unlocking
 *								Fixed dangling pointer reference for multi part record lists
 *
 *************************************************************/
Err	DmDatabaseSize(UInt16 cardNo, LocalID	dbID, UInt32 * numRecordsP,
				UInt32 * totalBytesP, UInt32 * dataBytesP)
{
	UInt32				actSize=0, dataSize=0;
	UInt16				totalRecords=0;
	DatabaseHdrPtr		hdrP;
	UInt16					i;
	MemPtr					p;
	MemHandle				h;
	Boolean				resDB;
	UInt8 *				entryP;
	UInt16				entrySize;
	RecordListPtr		listP;
	LocalID				id;
	Boolean				ramBased;
	

	// Get the header ptr
	hdrP = MemLocalIDToLockedPtr(dbID, cardNo);
	if (!hdrP) return dmErrMemError;

	// Get the number of records and the resDB boolean
	resDB = hdrP->attributes & dmHdrAttrResDB;
	if (resDB) entrySize = sizeof(RsrcEntryType);
	else entrySize = sizeof(RecordEntryType);

	// Set boolean if it's RAM based or not
	if (MemLocalIDKind(dbID) == memIDHandle) 
		ramBased = true;
	else
		ramBased = false;


	// Accumulate the sizes of every chunk
	listP = &hdrP->recordList;
	while (true) {
	
		// MemPtr to first entry
		entryP = (UInt8 *)&listP->firstEntry;
	
		// Add sizes of every chunk in this list
		for (i=0; i<listP->numRecords; i++) {
			
			if (resDB) id = ((RsrcEntryPtr)entryP)->localChunkID;
			else id = ((RecordEntryPtr)entryP)->localChunkID;
			
			// We do it this way because it works for ROM based or RAM based records.
			if (id) {
				if (ramBased) {
					// If the chunk is in RAM then we get it's MemHandle then the size instead
					// of locking and unlocking the chunk which is relatively expensive.
					h = MemLocalIDToGlobal(id, cardNo);
					dataSize += MemHandleSize(h);
					}
				else {
					p = MemLocalIDToLockedPtr(id, cardNo);
					dataSize += MemPtrSize(p);
					MemPtrUnlock(p);
					}
				}
			
			// next entry
			entryP += entrySize;
			}
			
		// On to the next list
		totalRecords += listP->numRecords;
		id = listP->nextRecordListID;
		if (listP != &hdrP->recordList) {
			MemPtrUnlock(listP);
			
			// Add the overhead of the record list header
			actSize += sizeof(RecordListType);
			if (ramBased) actSize += sizeof(MemHandle);		// for record list MemHandle
			}
			
		if (!id) break;
		listP = MemLocalIDToLockedPtr(id, cardNo);
		}
	
	
	// Add the overhead to get the actual size
	actSize += dataSize + sizeof(DatabaseHdrType) +
				totalRecords * (sizeof(memChunkHeaderTypeName) + entrySize);
	
	// If it's RAM based, add the space used for master pointers
	if (ramBased)
		actSize += sizeof(MemHandle)									// for database header
					  + (totalRecords * sizeof(MemHandle));		// for each record

	// Return results
	MemPtrUnlock(hdrP);
	
	if (totalBytesP)
		*totalBytesP = actSize;
	if (dataBytesP)
		*dataBytesP = dataSize;
	if (numRecordsP)
		*numRecordsP = totalRecords;
	return 0;
}


/************************************************************
 *
 *  FUNCTION: DmOpenDatabaseByTypeCreator
 *
 *  DESCRIPTION: Opens up the most recent revision of a dabase with
 *		the given type and creator
 *
 *  PARAMETERS: 
 *		type - type of database
 *		creator - creator of database
 *		mode - open mode
 *
 *  RETURNS: DmOpenRef to open database, or 0 if unsuccessful.
 *
 *  CREATED: 9/21/95
 *
 *  BY: Ron Marianetti
 *
 *	HISTORY:
 *		4/6/2000	ryw	set last err to zero, use PrvSetLastErr
 *************************************************************/
DmOpenRef DmOpenDatabaseByTypeCreator(UInt32 type, UInt32 creator, UInt16 mode)
{
	DmSearchStateType	searchState;
	Err					err;
	UInt16				cardNo;
	LocalID				dbID;
	DmOpenRef			dbR=0;
	
	// Grab semaphore
	MemSemaphoreReserve(true);


	// Find the database
	err = DmGetNextDatabaseByTypeCreator (true, &searchState, type, 
		creator, true, &cardNo, &dbID);

	PrvSetLastErr(err);
	if (err) {
		goto Exit;
		}
	

	// Open it.
	dbR = DmOpenDatabase(cardNo, dbID, mode);
	//ErrFatalDisplayIf(!dbR, "Err opening DB");


Exit:
	MemSemaphoreRelease(true);
	return dbR;
}



/************************************************************
 *
 *  FUNCTION:		DmOpenDBNoOverlay
 *
 *  DESCRIPTION:	Opens up a database and returns a reference to it.
 *						This is the same as DmOpenDatabase, except that
 *						it doesn't try to open an overlay.
 *
 *  PARAMETERS:	card number, Database Header Chunk ID, mode
 *
 *  RETURNS:		DmOpenRef to open database, or 0 if unsuccessful.
 *
 *  CREATED: 11/16/94 
 *
 *  BY: Ron Marianetti
 *
 *	Name	Date		Description
 *	----	----		-----------
 *	vmk	10/31/97	Save current modification number
 *	roger	12/12/97	Change to use SysCurAppInfo
 *	roger	08/28/98	Fixed Null dbID goto Exit and write to uninit appInfoP bug.
 *	jesse	03/25/99	Change to allow multiple tasks to open read-only	
 *						and prevent them from opening res DB's for write.
 * jesse 03/31/99	Put back ugly hack allowing multiple tasks to 
 *						open a res DB for hotsyncing the shortcuts DB.
 *	roger	04/07/99	Change dmErrReadOnly to dmErrROMBased when in ROM.
 *						Reject opening databases for write when in ROM and
 *						already opened (Shannon Peckardy).
 *	kwk	06/29/99	Changed name to DmOpenDBNoOverlay.
 *	jesse	10/05/99	Dirty launch database record when opening for write access.
 *	ryw 	4/6/2000		use PrvSetLastErr to report errors
 *
 *************************************************************/
DmOpenRef DmOpenDBNoOverlay(UInt16 cardNo, LocalID dbID, UInt16 mode)
{
	DmAccessPtr		accessP=0;
	DmOpenInfoPtr	openP=0;
	Err				err;
	DatabaseHdrPtr	hdrP;
	MemHandle		hdrH;
	UInt32			size;
	MemPtr*			handleTableP=0;
	UInt16			i;
	LocalID			localID;
	SysAppInfoPtr	appInfoP;
	UInt32			taskID;
	RecordListPtr	listP;


	
	// Grab semaphore
	MemSemaphoreReserve(true);
	
	
	// Get the SysAppInfoPtr for the current app
	appInfoP = PrvGetAppInfo();
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.


	// Error Check
	ErrFatalDisplayIf(!dbID, "Null dbID passed");
	if (!dbID) {err = dmErrInvalidParam; goto Exit;}
	
	
	// Allocate DmAccessType
	accessP = MemChunkNew(0, sizeof(DmAccessType), dmDynOwnerID | memNewChunkFlagPreLock);
	if (!accessP)  {
		err = dmErrMemError;
		goto Exit;
		}
		
	// Link it in
	accessP->next = (DmAccessPtr)(appInfoP->dmAccessP);
	appInfoP->dmAccessP = (MemPtr)accessP;
			
			
			
	// See if this database is already open
	openP = (DmOpenInfoPtr)GDmOpenList;
	while(openP) {
		if (openP->hdrID == dbID && openP->cardNo == cardNo) break;
		openP = (DmOpenInfoPtr)openP->next;
		}
	
	

	//--------------------------------------------------------------------------------
	// If it's already open, check for conflicts and just increment the 
	// open count and init the DmAccessType to reference the open information
	//--------------------------------------------------------------------------------
	if (openP) {
	
		// If it's already opened with exclusive access OR if we need exclusing access, error
		if (openP->exclusive || (mode & dmModeExclusive))  {
			err = dmErrDatabaseOpen;
			openP = 0;					// so it doesn't get freed on error
			goto Exit;
			}
			
		if (mode & dmModeWrite) {
			// can only be open once for write access
			if (openP->writeAccess) {
				err = dmErrAlreadyOpenForWrites;
				openP = 0;				// so it doesn't get freed on error
				goto Exit;
				}
				
			// can open for write access if in ROM
			if (MemLocalIDKind(dbID) != memIDHandle) {
				err = dmErrROMBased;
				openP = 0;				// so it doesn't get freed on error
				goto Exit;
				}
			}
				
		// if multiple tasks open it, NO ONE can have write access but read only is okay.
		// Unfortunately, we MUST allow write access to 1 task if its a resource database
		// because otherwise we can't backup the Grafitti Shortcuts database during hotsync.
		if (!openP->resDB &&
			openP->ownerTaskID != taskID &&
			(openP->writeAccess || (mode & dmModeWrite))) {
			err = dmErrOpenedByAnotherTask;
			openP = 0;					// so it doesn't get freed on error
			goto Exit;
			}
		
		// Fill in the access MemPtr 
		accessP->mode = mode;
		accessP->openP = openP;
		err = 0;
			
		// Update open info
		openP->openCount++;
		if (mode & dmModeWrite) openP->writeAccess = true;
		if (mode & dmModeExclusive) openP->exclusive = true;
		
		// DOLATER kwk - this code used to say:
		// openP = 0;
		// err = 0;
		// Which isn't necessary, and it didn't have the following line, which it does
		// seem like we'll need.
		// accessP->savedModNum = openP->hdrP->modificationNumber;
		
		accessP->openType = openTypeRegular;
		
		goto Exit;
		}
		
		
	// Else, we need to allocate the openInfo structure
	else {
		openP = MemChunkNew(0, sizeof(DmOpenInfoType), dmDynOwnerID | memNewChunkFlagPreLock);
		if (!openP) {
			err = dmErrMemError;
			goto Exit;
			}
		MemSet(openP, sizeof(DmOpenInfoType), 0);

		// Fill in the access MemPtr
		accessP->mode = mode;
		accessP->openP = openP;
		}
	
	
	
	//--------------------------------------------------------------------------------
	// Lock the database header down if it's a movable chunk
	//--------------------------------------------------------------------------------
	if (MemLocalIDKind(dbID) == memIDHandle) {
		hdrH = MemLocalIDToGlobal(dbID, cardNo);
		hdrP = MemHandleLock(hdrH);
		openP->hdrH = hdrH;
		openP->hdrP = hdrP;
		
		// If this database is read only, make sure caller is not trying to get
		//  write access
		if ((hdrP->attributes & dmHdrAttrReadOnly) && (mode & dmModeWrite))
			{err = dmErrReadOnly; goto Exit;}
		}
		
	// If it's not movable (ROM-based), create a fake MemHandle for it and allocate a block
	// for holding the master pointers to the ROM based records
	else {
		// Don't allow ROM based databases to be opened with write access.
		if (mode & dmModeWrite) {err = dmErrROMBased; goto Exit;}
		hdrP = MemLocalIDToGlobal(dbID, cardNo);
		openP->hdrMasterP = (MemPtr)hdrP;
		openP->hdrH = (MemHandle)&openP->hdrMasterP;
		openP->hdrP = hdrP;
		}
		
		
	// Save current mod num for comparison at closing to determine if mod date will
	// need to be changed. DOLATER kwk - move these two lines up above where the
	// case of an open info block already exists, so we don't duplicate code.
	accessP->savedModNum = hdrP->modificationNumber;

	// Not being opened as a base or overlay.
	accessP->openType = openTypeRegular;
	
	//--------------------------------------------------------------------------------
	// Fill in the Open info structure.
	//--------------------------------------------------------------------------------
	openP->next = 0;
	openP->openCount = 1;
	openP->ownerTaskID = taskID;
	if (mode & dmModeExclusive) openP->exclusive = true;
	if (mode & dmModeWrite) openP->writeAccess = true;
	openP->resDB = hdrP->attributes & dmHdrAttrResDB;
	openP->hdrID = dbID;
	openP->cardNo = cardNo;

		
	//--------------------------------------------------------------------------------
	// Go through and lock all the Record lists down and count the total
	//  number of records which we cache in the OpenInfo structure for
	//  better performance
	//--------------------------------------------------------------------------------
	listP = &hdrP->recordList;
	openP->numRecords = listP->numRecords;
	while(1) {
		localID = listP->nextRecordListID;
		if (!localID) break;
		
		// Lock this list down and add it's records
		listP = MemLocalIDToLockedPtr(localID, cardNo);
		openP->numRecords += listP->numRecords;
		}
		
		
	//--------------------------------------------------------------------------------
	// If this is a ROM based database, make master pointers for each record
	//--------------------------------------------------------------------------------
	if (openP->hdrMasterP) {

		// Make a MemHandle table for ROM based databases.
		size = openP->numRecords * sizeof(MemPtr);
		handleTableP = MemChunkNew(0, size, dmDynOwnerID | memNewChunkFlagPreLock);
		if (!handleTableP) {
			err = dmErrMemError;
			goto Exit;
			}
		MemSet(handleTableP, size, 0);
		openP->handleTableP = handleTableP;
		
		
		// If resource database, fill in the MemHandle table with master pointers to each resource
		if (openP->resDB) {
			RsrcEntryPtr	resourceP=0;
			for (i=0; i<openP->numRecords; i++) {
				resourceP = PrvRsrcEntryPtr(openP, i, resourceP);
				ErrFatalDisplayIf (!resourceP, "Logic Flaw");
				localID = resourceP->localChunkID;
				if (localID) 
					handleTableP[i] = MemLocalIDToGlobal(localID, cardNo);
				}
			}
				

		// else, if record database, fill in the MemHandle table with master pointers
		//  to each resource
		else {
			RecordEntryPtr	recP=0;
			for (i=0; i<openP->numRecords; i++) {
				recP = PrvRecordEntryPtr(openP, i, recP);
				localID = recP->localChunkID;
				if (localID) 
					handleTableP[i] = MemLocalIDToGlobal(localID, cardNo);
				}
			}
				
		} // if (openP->hdrMasterP)


	//--------------------------------------------------------------------------------
	// Link in the OpenInfo structure to the linked list of open databases
	//--------------------------------------------------------------------------------
	openP->next = GDmOpenList;
	GDmOpenList = (MemPtr)openP;
	err = 0;


	// If this database was not closed properly last time, clean up the
	// record states
	if (hdrP->attributes & dmHdrAttrOpen) 
		DmResetRecordStates(accessP);
		
		
	// Set the open bit in the header
	if (!handleTableP)
		hdrP->attributes |= dmHdrAttrOpen;
	
	// If we're opening an overlay or a visible item with write access,
	// dirty all related launcher records, if any:
	if((accessP->mode & dmModeWrite) && 
		(PrvIsLauncherVisible(hdrP->type, hdrP->creator, hdrP->attributes) ||
		hdrP->type == sysFileTOverlay))
		PrvSetLaunchRecordFlags(dmLaunchFlagDirty, 0, 0, hdrP->creator, 0);
	

Exit:
	PrvSetLastErr(err);
	if (err) {
		if (accessP) {
			appInfoP->dmAccessP = (MemPtr)accessP->next;
			MemPtrFree(accessP);
			accessP = 0;
			}
		if (openP) {
			if (openP->hdrP) MemPtrUnlock(openP->hdrP);
			if (openP->handleTableP) MemPtrFree(openP->handleTableP);
			MemPtrFree(openP);
			}
		}
		
	// Release semaphore
	MemSemaphoreRelease(true);

	return accessP;
}	



/************************************************************
 *
 *  FUNCTION:		DmOpenDatabase
 *
 *  DESCRIPTION:	Opens up a database and returns a reference to it.
 *
 *  PARAMETERS:	card number, Database Header Chunk ID, mode
 *
 *  RETURNS:		DmOpenRef to open database, or 0 if unsuccessful.
 *
 *  CREATED: 06/29/99
 *
 *  BY: Ken Krugler
 *
 *	Name	Date		Description
 *	----	----		-----------
 *	kwk	07/01/99	Changed to be a cover for DmOpenDBWithLocale.
 *
 *************************************************************/
DmOpenRef DmOpenDatabase(UInt16 cardNo, LocalID dbID, UInt16 mode)
{
	return(DmOpenDBWithLocale(cardNo, dbID, mode, NULL));
}


/************************************************************
 *
 *	FUNCTION:		DmOpenDBWithLocale
 *
 *	DESCRIPTION:	Opens up a database and returns a reference to it.
 *						This version takes an explicit locale that it
 *						passes to OmOpenOverlayDatabase.
 *
 *	PARAMETERS:	card number, Database Header Chunk ID, mode, locale MemPtr
 *
 *	RETURNS:		DmOpenRef to open database, or 0 if unsuccessful.
 *
 *	HISTORY:
 *		06/29/99	kwk	Created by Ken Krugler (cover for DmOpenDBNoOverlay).
 *		10/01/99	kwk	Support caching of overlay DB LocalID in
 *							the RAM store's DB list.
 *		10/24/99	kwk	Made localeP a const.
 *		4/6/2000	ryw	use PrvSetLastErr for error reporting
 *************************************************************/
DmOpenRef DmOpenDBWithLocale(UInt16 cardNo, LocalID dbID, UInt16 mode, const OmLocaleType* localeP)
{
	DmOpenRef baseRef;
	DmOpenRef overlayRef;
	Err result = errNone;
	
	// If an error occurs w/opening the base, dmLastErr is already
	// set by DmOpenDBNoOverlay.
	baseRef = DmOpenDBNoOverlay(cardNo, dbID, mode);
	if (baseRef == NULL) {
		return(NULL);
		}
	
	// OmOpenOverlayDatabase will typically return an error if the caller
	// is trying to open a stripped resource database where no appropriate
	// overlay can be found.
	result = OmOpenOverlayDatabase(baseRef, localeP, &overlayRef);
	if (result != errNone) {
		DmCloseDatabase(baseRef);
		baseRef = NULL;
		}
	
	// If we've got a valid, opened overlay, record the fact so that
	// other routines can treat the base/overlay as a mind-melded DB.
	else if (overlayRef != NULL) {
		((DmAccessPtr)baseRef)->openType = openTypeBase;
		((DmAccessPtr)overlayRef)->openType = openTypeOverlay;
		}

	PrvSetLastErr(result);

	return(baseRef);
}


/************************************************************
 *
 *  FUNCTION: DmResetRecordStates
 *
 *  DESCRIPTION: Unlocks all records and clears the busy bits 
 *
 *  PARAMETERS: database access pointer
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 11/16/94 
 *
 *  BY: Ron Marianetti
 *
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	4/13/99	Explicitly get write access in this routine
 *
 *************************************************************/
Err	DmResetRecordStates(DmOpenRef dbR)
{
	UInt16			cardNo;
	UInt16			i;
	LocalID			id;
	MemPtr			p;
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	DmOpenInfoPtr	openP;
	
	ECDmValidateDmOpenRef(dbP);

	openP = dbP->openP;
		
	// Can't do this for ROM databases
	if (openP->handleTableP)
		return dmErrROMBased;
		
	// get write permission in case we're not called from DmOpenDatabase
	MemSemaphoreReserve(true);
	
	// Unlock all the records and clear the busy bits
	cardNo = openP->cardNo;

	if (openP->resDB) {
		RsrcEntryPtr	resourceP=0;
		for (i=0; i<openP->numRecords; i++) {
			resourceP = PrvRsrcEntryPtr(openP, i, resourceP);
			ErrFatalDisplayIf(!resourceP, "Logic flaw");
			p = MemLocalIDToLockedPtr(resourceP->localChunkID, cardNo);
			MemPtrResetLock(p);
			}
		}

	else {
		RecordEntryPtr recordP=0;
		for (i=0; i<openP->numRecords; i++) {
			recordP = PrvRecordEntryPtr(openP, i, recordP);
		 	recordP->attributes	&= ~dmRecAttrBusy;
		 	id = recordP->localChunkID;
		 	
		 	if (!id) continue;
			p = MemLocalIDToLockedPtr(id, cardNo);
			MemPtrResetLock(p);
			}
		}
		
	MemSemaphoreRelease(true);
	
	return 0;
}



/************************************************************
 *
 *  FUNCTION: DmGetLastErr
 *
 *  DESCRIPTION: Returns the error code from the last Data Manager call
 *
 *  PARAMETERS: none
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 1/26/95
 *
 *  BY: Ron Marianetti
 *
 *  MODIFICATIONS;
 *		06/29/99	ryw	simplified function
 *************************************************************/
Err	DmGetLastErr(void)
{
	return PrvGetAppInfo()->dmLastErr;
}

/************************************************************
 *
 *  FUNCTION: PrvSetLastErr
 *
 *  DESCRIPTION: set the dmLastErr field of currect app
 *
 *  PARAMETERS: new value for error field
 *
 *  RETURNS: nothing
 *
 *  CREATED: 4/6/2000
 *
 *  BY: Russell Y. Webb
 *
 *  MODIFICATIONS;
 *************************************************************/
static void PrvSetLastErr( Err newLastError )
{
	PrvGetAppInfo()->dmLastErr = newLastError;
}

/************************************************************
 *
 *  FUNCTION: DmCloseDatabase
 *
 *  DESCRIPTION: Closes a Database
 *
 *  PARAMETERS: database access pointer
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 06/29/99
 *
 *  BY: Ken Krugler
 *
 *  MODIFICATIONS:
 *		06/29/99	kwk	Moved guts into PrvCloseDatabase, and added
 *							check for closing overlay.
 *
 *************************************************************/
Err DmCloseDatabase(DmOpenRef dbR)
{
	Err err;
	DmOpenRef prevP;
	DmAccessPtr dbP = (DmAccessPtr)dbR;
	UInt8 dbType;

	// This test is also done in PrvCloseDatabase, but we need it here
	// because we need to test dbP->openType;
	ECDmValidateDmOpenRef(dbP);

	dbType = dbP->openType;
	
	// DOLATER kwk - maybe make this a real error???
	ErrNonFatalDisplayIf(dbType == openTypeOverlay, "Closing overlay directly");
	
	// Close the database. If that worked, and the database is a base,
	// then also close down the overlay.
	
	err = PrvCloseDatabase(dbR, &prevP);
	if ((err == errNone) && (dbType == openTypeBase)) {
		ErrNonFatalDisplayIf(prevP == NULL, "No overlay db");
		ErrNonFatalDisplayIf(((DmAccessPtr)prevP)->openType != openTypeOverlay,
								"Previous db is not an overlay");
		err = PrvCloseDatabase(prevP, NULL);
		}
	
	ErrNonFatalDisplayIf(err, "Err closing DB");
	return(err);
}


/************************************************************
 *
 *  FUNCTION: DmNextOpenDatabase
 *
 *  DESCRIPTION: Returns database access MemPtr to next open database
 *
 *  PARAMETERS: current database access MemPtr, or nil
 *
 *  RETURNS: access MemPtr, or nil if no more
 *
 *  CREATED: 11/16/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
DmOpenRef DmNextOpenDatabase(DmOpenRef currentP)
{
	SysAppInfoPtr	appInfoP;
	
	if (!currentP) {
		// <RM> 1-14-98 
		// WAS:
		//		SysGetAppInfo(&appInfoP, &unusedAppInfoP);
		//    return (DmAccessPtr)appInfoP->dmAccessP;		
		appInfoP = PrvGetAppInfo();	
		
		return (DmAccessPtr)appInfoP->dmAccessP;
		}
		
	ECDmValidateDmOpenRef((DmAccessPtr)currentP);
	
	return ((DmAccessPtr)currentP)->next;
}


/************************************************************
 *
 *  FUNCTION: DmOpenDatabaseInfo
 *
 *  DESCRIPTION: Returns information about an open database
 *
 *  PARAMETERS: access MemPtr, ptrs to dbID, openCount, mode
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 11/16/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err DmOpenDatabaseInfo(DmOpenRef dbR, LocalID* dbIDP,
			UInt16 * openCountP, UInt16 * modeP, UInt16 * cardNoP,
			Boolean * resDBP)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	DmOpenInfoPtr	openP;
	
	ECDmValidateDmOpenRef(dbP);

	openP = dbP->openP;
	
	if (dbIDP) 			*dbIDP = openP->hdrID;
	if (openCountP)	*openCountP = openP->openCount;
	if (modeP) 			*modeP = dbP->mode;
	if (cardNoP) 		*cardNoP = openP->cardNo;
	if (resDBP) 		*resDBP = openP->resDB;
	return 0;
}



/************************************************************
 *
 *	FUNCTION:		DmMoveOpenDBContext
 *
 *	DESCRIPTION:	Used by SysAppStartup to move the DmAccess pointer of
 *   a database out of the current context and into another. For example,
 *		an open database can be moved from the app's open database list to the
 *		System list or vice versa.
 *
 *		As a special case, passing 0 for dstHeadP specifies that the database
 *		should be moved to the System Context. This is a special case because
 *		of the way DmAccessPtrs are linked together, the system list always
 *		appears tacked on to the end of the application list so this routine
 *		must not only update the system list but also update the link in
 *		the application's list to the system list. Got That????
 *
 *	PARAMETERS:		dstHeadP - Head pointer of destination list - or 0 for System context
 *						dbP - DmAccessPtr of database we're moving out of ucfdrent list.
 *
 *	RETURNS:			0 if no err
 *
 *	CALLED BY:		SysAppLaunch in order to move an application database that it
 *						is launching out of the System list and into the application's list.
 *
 *	HISTORY:
 *		05/16/96	ron	Created by Ron Marianetti
 *		06/29/99	kwk	Moved guts to PrvMoveOpenDBContext, and added check
 *							for moving base database.
 *
 *************************************************************/
Err DmMoveOpenDBContext(DmAccessPtr* dstHeadP, DmAccessPtr dbP)
{
	Err err;
	DmAccessPtr prevAccessP;
	
	// DOLATER kwk - check for moving an overlay???
	err = PrvMoveOpenDBContext(dstHeadP, dbP, &prevAccessP);
	if ((err == errNone) && (dbP->openType == openTypeBase)) {
		ErrNonFatalDisplayIf((prevAccessP == NULL) || (prevAccessP->openType != openTypeOverlay),
									"Missing overlay for base");
		err = PrvMoveOpenDBContext(dstHeadP, prevAccessP, NULL);
		}

	ErrNonFatalDisplayIf((err != errNone), "Can't move open DB");
	return(err);
}

/************************************************************
 *
 *  FUNCTION: DmGetAppInfoID
 *
 *  DESCRIPTION: Returns the local id of the application 
 *    info block.
 *
 *  PARAMETERS: database access MemPtr
 *
 *  RETURNS: local id of the application info block
 *
 *  CREATED: 6/19/95
 *
 *  BY: Art Lamb
 *
 *************************************************************/
LocalID DmGetAppInfoID (DmOpenRef dbR)
{
	DmAccessPtr				dbP = (DmAccessPtr)dbR;
	DmOpenInfoPtr			openP;
	
	ECDmValidateDmOpenRef(dbP);

	openP = dbP->openP;

	return openP->hdrP->appInfoID;
}


/************************************************************
 *
 *  FUNCTION: DmNumRecords
 *
 *  DESCRIPTION: Returns the number of records in a database
 *
 *  PARAMETERS: database access pointer
 *
 *  RETURNS: Number of records
 *
 *  CREATED: 11/16/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt16	DmNumRecords(DmOpenRef dbP)
{
	ECDmValidateDmOpenRef((DmAccessPtr)dbP);

	return ((DmAccessPtr)dbP)->openP->numRecords;
}


/************************************************************
 *
 *  FUNCTION: DmNumRecordsInCategory
 *
 *  DESCRIPTION: Returns the number of records in a database in the 
 *		specified category.
 *
 *  PARAMETERS: database access pointer, category
 *
 *  RETURNS: Number of records
 *
 *  CREATED: 3/13/95 
 *
 *  BY: Art Lamb
 *
 *		roger	8/5/98	Added check for resource databases
 *
 *************************************************************/
UInt16	DmNumRecordsInCategory(DmOpenRef dbR, UInt16 category)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	UInt16			index;
	UInt16 			count;
	UInt16			numRecords = 0;
	UInt8				attrMask, attrCmp;
	RecordEntryPtr	recordP;
	DmOpenInfoPtr	openP;

	ECDmValidateDmOpenRef(dbP);
		
	openP = dbP->openP;

	ErrNonFatalDisplayIf (openP->resDB, "Record DB only");
	
	count = 	dbP->openP->numRecords;
	
	// Form the mask for comparing the attributes of each record
	if (category == dmAllCategories) {
		attrMask =  dmRecAttrDelete;
		attrCmp = 0;
		}
	else {
		attrMask =  dmRecAttrCategoryMask | dmRecAttrDelete;
		attrCmp = category;
		}
		
	if (!(dbP->mode & dmModeShowSecret))
		attrMask |= dmRecAttrSecret;
		
		
	// Go though and count all records that match our attribute criteria
	recordP = 0;
	for (index = 0; index < count; index++) {
		recordP = PrvRecordEntryPtr(openP, index, recordP);
		if ((recordP->attributes & attrMask) == attrCmp)
			numRecords++;
		}
		
	return numRecords;
}


/************************************************************
 *
 *  FUNCTION: DmRecordInfo
 *
 *  DESCRIPTION: Retreives the record information as stored in
 *		the database header.
 *
 *  PARAMETERS: db access MemPtr, record index, MemPtr to return attribute,
 *			MemPtr to return uniqueID
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 11/17/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err	DmRecordInfo(DmOpenRef dbR, UInt16 index, 
				UInt16 * attrP, UInt32 * uniqueIDP, LocalID* chunkIDP)
{
	DmAccessPtr			dbP = (DmAccessPtr)dbR;
	UInt32 *				lP;
	RecordEntryPtr		recordP;
	Err					err;

	ECDmValidateDmOpenRef(dbP);

	// Get the entry MemPtr
	recordP = PrvRecordEntryPtr(dbP->openP, index, 0);
	if (!recordP) {err = dmErrIndexOutOfRange; goto Exit;}

	if (attrP) *attrP = recordP->attributes;
	if (uniqueIDP) {
		lP = (UInt32 *)&recordP->attributes;
		*uniqueIDP = (*lP)  & 0x00FFFFFF;
		}

	if (chunkIDP) 
		*chunkIDP = recordP->localChunkID;
		
	err = 0;
	
Exit:
	ErrNonFatalDisplayIf(err, "Err getting rec info");
	return err;
}



/************************************************************
 *
 *  FUNCTION: DmSetRecordInfo
 *
 *  DESCRIPTION: Sets the record information stored in
 *		the database header.
 *
 *  PARAMETERS: db access MemPtr, record index, MemPtr to attribute,
 *			MemPtr to uniqueID
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 11/17/94 
 *
 *  BY: Ron Marianetti
 *
 *		roger	8/5/98	Added check for resource databases
 *		dje	11/1/00	Don't change system-only attributes
 *
 *************************************************************/
Err	DmSetRecordInfo(DmOpenRef dbR, UInt16 index, 
				UInt16 * attrP, UInt32 * uniqueIDP)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	UInt32 *			lP;
	UInt32			temp;
	DmOpenInfoPtr	openP;
	Err				err;
	RecordEntryPtr	recordP;

	// Enable writes
	MemSemaphoreReserve(true);

	ECDmValidateDmOpenRef(dbP);

	openP = dbP->openP;
	
	// Check for errors
	if (!openP->writeAccess)
		{err = dmErrReadOnly; goto Exit;}
		
	if (openP->resDB) 
		{err = dmErrNotRecordDB; goto Exit;}
	
	// Get the record entry pointer
	recordP = PrvRecordEntryPtr(openP, index, 0);
	if (!recordP) { err = dmErrIndexOutOfRange; goto Exit;}


	// Bump the mod number
	openP->hdrP->modificationNumber++;

	// Set the info, but leave system-only attributes alone
	if (attrP)
		recordP->attributes = (recordP->attributes & dmSysOnlyRecAttrs) | (*attrP & ~dmSysOnlyRecAttrs);
	
	if (uniqueIDP) {
		lP = (UInt32 *)&recordP->attributes;
		temp = *lP;
		temp = (temp & 0xFF000000L) | (*uniqueIDP & 0x00FFFFFF);
		*lP = temp;
		}
		
	err = 0;
	
Exit:
	// Restore write-protection
	MemSemaphoreRelease(true);
		
	ErrNonFatalDisplayIf(err, "Err setting rec info");
	return err;
}





/************************************************************
 *
 *  FUNCTION: DmRemoveSecretRecords
 *
 *  DESCRIPTION: Removes all secret records.
 *
 *  PARAMETERS: db access MemPtr
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 1/30/95 
 *
 *  BY: Roger Flores
 *
 *		roger	8/5/98	Added check for resource databases
 *
 *************************************************************/
Err	DmRemoveSecretRecords(DmOpenRef dbR)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	UInt16 			index;
	DmOpenInfoPtr	openP;
	Err				err;
	RecordEntryPtr	recordP = 0;

	
	// Enable writes
	MemSemaphoreReserve(true);

	ECDmValidateDmOpenRef(dbP);

	openP = dbP->openP;
		
	// Check for errors
	if (!openP->writeAccess)
		{err = dmErrReadOnly; goto Exit;}
						
	if (openP->resDB) 
		{err = dmErrNotRecordDB; goto Exit;}
	
	// Remove all records with the secret bit set.
	// Start at the end so that shifting records up doesn't cause
	// records to be skipped. Handle numRecords from 0 right up to 0xFFFF.
	if (openP->numRecords != 0)
	{
		index = openP->numRecords;
		do {
			index--;
			recordP = PrvRecordEntryPtr(openP, index, 0);
			if (recordP->attributes & dmRecAttrSecret) 
				 DmRemoveRecord(dbP, index);
		} while (index != 0);
	}
	
	err = 0;
	
Exit:
	MemSemaphoreRelease(true);
	ErrNonFatalDisplayIf(err, "Err Removing Secret Records");
	return err;
} 


/************************************************************
 *
 *  FUNCTION: DmAttachRecord
 *
 *  DESCRIPTION: Attaches a record as specified by a chunk MemHandle to a database.
 *
 *					The function may be used to replace or insert a record:
 *
 *					if oldHP is non-null, then the operation is a replacement, and
 *					the MemHandle of the old record at the given index will be returned;
 *					if oldHP is null, the operation is an insertion.	
 *
 *  PARAMETERS: 
 *		dbR 		- reference to open database
 *		*atP		- desired index to insert record at, actual index on return.
 *		newH		- MemHandle of new record or NULL (if NULL, record's localChunkID
 *					  field will be set to zero in the record list)
 *		*oldHP	- MemHandle of replaced record returned here (this record chunk
 *					  is orphaned - dmOrphanOwnerID).
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 11/16/94 
 *
 *  BY: Ron Marianetti
 *
 *		vmk	11/18/97	Added header comment about dual role of oldHP
 *		roger	8/5/98	Added check for resource databases
 *		mchen	1/2/01	Added check for full database
 *
 *************************************************************/
Err	DmAttachRecord(DmOpenRef dbR, UInt16 * atP, MemHandle newH,
				MemHandle*	oldHP)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	UInt16			numRecords, index;
	RecordEntryPtr	recordP;
	Err				err;
	LocalID			id;
	DmOpenInfoPtr	openP;

	// Enable writes
	MemSemaphoreReserve(true);

	ECDmValidateDmOpenRef(dbP);
		
	openP = dbP->openP;
	
	// Check for errors
	if (!openP->writeAccess)
		{err = dmErrReadOnly; goto Exit;}
	
	if (openP->resDB) 
		{err = dmErrNotRecordDB; goto Exit;}
	
	// Make sure the record is in the right card
	if (newH) {
		if (MemHandleCardNo(newH) != openP->cardNo)
			{err =  dmErrRecordInWrongCard; goto Exit;}
		}

	// If inserting, limit the number of records in the DB.
	numRecords = openP->numRecords;
	if ((oldHP == NULL) && (numRecords >= dmMaxRecordsInDB))
		{ err = dmErrIndexOutOfRange; goto Exit; }   // DOLATER - add a more specific error for this case

	// bob 12/19/00, bug 24389, don't trash low mem on NULL atP
	if (atP == NULL)
		{err =  dmErrInvalidParam; goto Exit;}	

	// Range check if replacing
	index = *atP;
	if (oldHP)
		if (index >= numRecords) {err = dmErrIndexOutOfRange; goto Exit;}


	// Adjust the index given the number of records in the database
	if (index > openP->numRecords) {
		index = openP->numRecords;
		*atP = index;
		}



	// Grow the record entry table to add space for the new record if we're
	//  inserting
	if (!oldHP) {
		// If database is empty, make room for the first entry
		if (openP->numRecords == 0) {
			UInt32		reqSize;
			
			// DOLATER... this can cause a failure in the multiple storage heap
			// model if there isn't enough space to grow the list in its current
			// heap, event if there is plenty of space in other heaps.  For multiple
			// heap support (since we can't just move the db header to a different heap
			// because that would invalidate its database id), we could fall back on leaving
			// the database header with zero record entries and shifting the new entry
			// as is done below for all other cases.  vmk 1/13/98
			
			reqSize = sizeof(DatabaseHdrType) + sizeof(RecordEntryType);
			MemPtrUnlock(openP->hdrP);
			err = MemHandleResize(openP->hdrH, reqSize);
			openP->hdrP = MemHandleLock(openP->hdrH);
			if (err) goto ExitNoErrCheck;
			openP->hdrP->recordList.numRecords = 1;
			}
			
		// Shift existing records down to make room
		else {
			if (index == openP->numRecords)
				err = PrvShiftEntriesDown(dbR, index-1, 0, true);
			else
				err = PrvShiftEntriesDown(dbR, index, 0, true);
			}
		if (err) {
			if (err == dmErrMemError) goto ExitNoErrCheck;
			goto Exit;
			}
		
		// DOLATER... when appending a record at the end of the list (a fairly common case)
		// the call PrvShiftEntriesDown(dbR, index-1, 0, true) causes a bit of unnecessary work
		// to happen in PrvShiftEntriesDown -- a couple of loops and a fair amount of setup
		// code is executed to copy the next-to-last entry into the new record's entry in the list.
		// This common case can possibly be detected and optimized out.  Profiling will need to
		// be done to determine if this is worth it.  1/6/98 vmk
		
		// Bump the cached # of records and initialize the attributes and unique ID
		openP->numRecords++;
		recordP = PrvRecordEntryPtr(openP, index, 0);
		recordP->attributes = 0;
		PrvSetUniqueID(openP->hdrP, recordP);
		}


	// Return the old chunkID if replacing
	else  {
		recordP = PrvRecordEntryPtr(openP, index, 0);
		id =  recordP->localChunkID;
		*oldHP = MemLocalIDToGlobal(id, openP->cardNo);
		MemHandleSetOwner(*oldHP, dmOrphanOwnerID);
		}


	// Put the new one in the list
	if (newH) {
		recordP->localChunkID = MemHandleToLocalID(newH);
		MemHandleSetOwner(newH, dmRecOwnerID);
		}
	else
		recordP->localChunkID = 0;
	
	// Set the dirty bit
	recordP->attributes |= dmRecAttrDirty;

	// Bump the mod number
	openP->hdrP->modificationNumber++;


	err = 0;
	
Exit:
	ErrNonFatalDisplayIf(err, "Err Attaching rec");
	
ExitNoErrCheck:
	// Restore write-protection
	MemSemaphoreRelease(true);
		
	return err;
}


/************************************************************
 *
 *  FUNCTION: DmDetachRecord
 *
 *  DESCRIPTION: Detaches a record from a database but does not
 *    delete the record's chunk
 *
 *  PARAMETERS: 
 *			dbR 			- reference to open database
 *			index			- index of record to detach
 *			*oldHP		- MemHandle of detached record returned here.
 *
 *  RETURNS: ChunkID of record that was detached
 *
 *  CREATED: 11/21/94 
 *
 *  BY: Ron Marianetti
 *
 *		roger	8/5/98	Added check for resource databases
 *
 *************************************************************/
Err	DmDetachRecord(DmOpenRef dbR, UInt16 index, MemHandle* oldHP)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	LocalID			id;
	RecordEntryPtr	recordP;
	Err				err;
	DmOpenInfoPtr	openP;
	
	
	// Enable writes
	MemSemaphoreReserve(true);
		
	ECDmValidateDmOpenRef(dbP);
		
	openP = dbP->openP;
	
	// Check for errors
	if (!openP->writeAccess)
		{err = dmErrReadOnly; goto Exit;}

	if (openP->resDB) 
		{err = dmErrNotRecordDB; goto Exit;}
	
	// Get the entry pointer for the record
	recordP = PrvRecordEntryPtr(openP, index, 0);
	if (!recordP) {err = dmErrIndexOutOfRange; goto Exit;}
	
	
	// Save the MemHandle of the old record
	id = recordP->localChunkID;
	if (id) {
		*oldHP = MemLocalIDToGlobal(id, openP->cardNo);
		MemHandleSetOwner(*oldHP, dmOrphanOwnerID);
		}
	else
		*oldHP = 0;



	// Adjust the number of records and shift later ones up
	err = PrvShiftEntriesUp(dbR, index+1, 0, true);
	if (err) goto Exit;

	// Decrease cached copy of number of records.
	openP->numRecords--;
	
	// Bump the mod number
	openP->hdrP->modificationNumber++;


Exit:
	// Restore Write-protection
	MemSemaphoreRelease(true);
		
	ErrNonFatalDisplayIf(err, "Err Detaching Rec");
	return err;
}





/************************************************************
 *
 *  FUNCTION: DmMoveRecord
 *
 *  DESCRIPTION: Moves a record from one index to another
 *		Will insert the record at the 'to' index and move other
 *		records down.
 *
 *    The to position should be viewed as an insertion position. 
 *		 Note that this value may be one greater than the index of
 *		 the last record in the database.
 *
 *  PARAMETERS: database access MemPtr, src index and dst index
 *
 *  RETURNS: ChunkID of record that was detached
 *
 *  CREATED: 11/21/94 
 *
 *  BY: Ron Marianetti
 *
 *		roger	8/5/98	Added check for resource databases
 *
 *************************************************************/
Err	DmMoveRecord(DmOpenRef dbR, UInt16 from, UInt16 to)
{
	DmAccessPtr			dbP = (DmAccessPtr)dbR;
	RecordEntryType	info;
	RecordEntryPtr		recordP;
	DmOpenInfoPtr		openP;
	Err					err;
	
	
	// Enable writes
	MemSemaphoreReserve(true);

	ECDmValidateDmOpenRef(dbP);
		
	openP = dbP->openP;
		
	// Check for errors
	if (!openP->writeAccess)
		{err = dmErrReadOnly; goto Exit;}

	// Check the range on the to indices
	if (to > openP->numRecords) 
		{err = dmErrIndexOutOfRange; goto Exit;}

	if (openP->resDB) 
		{err = dmErrNotRecordDB; goto Exit;}
	

	// Get the info on the record we're moving
	recordP = PrvRecordEntryPtr(openP, from, 0);
	if (!recordP) { err =  dmErrIndexOutOfRange; goto Exit;}
	info = *recordP;


	// Shift existing records around
	if (from > to) 
		err = PrvShiftEntriesDown(dbR, to, from-to, false);
	// When to is > from, to can actually be equal to numRecords.
	else if (to > from) {
		UInt16 count = to-from-1;
		if (count)
			err = PrvShiftEntriesUp(dbR, from+1, count, false);
		else
			err = 0;
		to--;
		}
	else
		err = 0;
		
		
	// Put the record in it's new place
	recordP = PrvRecordEntryPtr(openP, to, 0); 
	*recordP = info;

	// Bump the mod number
	openP->hdrP->modificationNumber++;


Exit:
	// Restore write-protection
	MemSemaphoreRelease(true);
		
	ErrNonFatalDisplayIf(err, "Err Moving rec");
	return err;
}



/************************************************************
 *
 *  FUNCTION: PrvExchangeRecord
 *
 *  DESCRIPTION: Exchange two database records.
 *
 *  PARAMETERS: database access pointer, one record, another record
 *
 *  RETURNS: the records' positions are exchanged
 *
 *  CREATED: 1/30/95
 *
 *  BY: Roger Flores
 *
 *		mchen	01/02/01	Changed left & right from Int16 to UInt16
 *		ryw  	01/12/01	added semaphore reserving so we can write to storage here
 *
 *************************************************************/
static void PrvExchangeRecord (DmOpenRef dbR, UInt16 left, UInt16 right)
{
	DmAccessPtr			dbP = (DmAccessPtr)dbR;
	RecordEntryType	info;
	DmOpenInfoPtr		openP = dbP->openP;
	RecordEntryPtr		leftP, rightP;

	MemSemaphoreReserve(true);
	
	// Bump the mod number
	openP->hdrP->modificationNumber++;

	// Do the exchange
	leftP = PrvRecordEntryPtr(openP, left, 0);
	rightP = PrvRecordEntryPtr(openP, right, 0);
	info = *leftP;
	*leftP = *rightP;
	*rightP = info;
	
	MemSemaphoreRelease(true);
}


/************************************************************
 *
 *  FUNCTION: PrvFastLockRecord
 *
 *  DESCRIPTION: Return a pointer to a record or a 0 if deleted.
 *               All records are assumed to have been locked before.
 *					  ROM based records fail.
 *
 *	 CALLED BY:	  PrvInsertionSort, PrvQuickSort
 *  PARAMETERS: database access MemPtr, index
 *
 *  RETURNS: record MemPtr or 0 if deleted
 *
 *  CREATED: 11/21/94
 *
 *  BY: Roger Flores
 *
 *************************************************************/
static void * PrvFastLockRecord(DmOpenInfoPtr openP, UInt16 index)
{
	LocalID id;
	MemHandle recordH=0;
	RecordEntryPtr recordP;
	
	
	// Get the entry MemPtr
	recordP = PrvRecordEntryPtr(openP, index, 0);
	
	
	// Check the attributes and make sure it's not marked for deletion
	if (recordP->attributes & dmRecAttrDelete)
		return 0;
	
	id = recordP->localChunkID;
	recordH = MemLocalIDToGlobal(id, openP->cardNo);

	return MemDeref(recordH);
}



/************************************************************
 *
 *  FUNCTION: PrvInsertionSort
 *
 *  DESCRIPTION: Sorts records in a database.  Deleted records 
 *  are placed last in any order.  All others are sorted 
 *  according to the passed comparison function.  Only records 
 *  which are out of order move.  Moved records are moved to the
 *  end of the range of equal records.  If a large amount of 
 *  records are being sorted try to use the quick sort.
 *
 *  This the insertion sort algorithm.  Starting with the second
 *  record, each record is compared to the preceeding record.
 *  Each record not greater than the last is inserted into 
 *  sorted position within those already sorted.  A binary insertion
 *  is performed.  A moved record is inserted after any other
 *  equal records.
 *
 *  PARAMETERS: base pointer to an array of records, number of records 
 *  to sort (must be at least 2), a record comparison function, 
 *  other data passed to the comparison function.
 *
 *  RETURNS: a sorted portion of the database
 *
 *  CREATED: 12/4/95		Copied from PrvSysInsertionSortHelp
 *
 *  BY: Roger Flores
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	7/2/98	release and reserve the write semaphore to permit task switching
 *			ADH	7/15/99	Encountered a problem with 8 meg devices.  It was possible to
 *								install close to Max Int16 records on a device.  During the
 *								sort it was possible to roll the binaryMiddle variable over into
 *								a negative number.  I increased the local variable that were at
 *								risk of rolling over to signed longs.  Since the datamgr calls
 *								use unsigned ints, these variables have to be cast down.
 *			mchen	1/2/01	remove release and reserve of the write semaphore
 *								because it is no longer being held while this
 *								function runs
 *
 *************************************************************/

// Compare records considering if they are deleted.  Does not
// call Fcmp if either record is deleted.  
// Returns results similar to Fcmp.
#define  compareRecords(left, right, leftEntryP, rightEntryP) 		\
		((!left && !right) ? 0 :												\
		 ((!left) ? 1 :															\
		 ((!right) ? -1 :															\
		 Fcmp ( left, right, other, 											\
		 (SortRecordInfoPtr)&leftEntryP->attributes, 					\
		 (SortRecordInfoPtr)&rightEntryP->attributes,					\
		 appInfoH))))

// use MemDeref(MemLocalIDToGlobal()) instead of MemLocalIDToPtr()
// because the former is faster.  The latter checks lock count,
// but we already know we locked the record.
#define  derefRecord(openP, recordP)										\
		((recordP->attributes & dmRecAttrDelete) ? 0 :					\
		MemDeref(MemLocalIDToGlobal (recordP->localChunkID, openP->cardNo)))

 
// Keep separate from DmInsertionSort so PrvSysQuickSort
// doesn't have to pay a sys trap everytime is calls this routine.

// NOTE!  There is a differnce between this algorithm and the one
// embedded within the quick sort routine.   Only this one places
// identical records at the end of those already existing.

static void PrvInsertionSort (const DmAccessPtr dbP, UInt16 base, 
	UInt16 numOfElements, const DmComparF *Fcmp, Int16 other)
{
   Int16      		result;
	Int32 			pivot, left, right;
	Int32				binaryLeft, binaryMiddle, binaryRight;
	
   MemPtr			leftRecP=0, pivotRecP=0, middleRecP;
   RecordEntryPtr	leftEntryP;
   RecordEntryPtr	pivotEntryP;
   RecordEntryPtr	middleEntryP;
	DmOpenInfoPtr	openP = dbP->openP;
	MemHandle		appInfoH;


	// Nearly identical code is used in DmPrvQSortHelp.  Make any modification to it too.
	// The variable names are kept similar to those in DmPrvQSortHelp so that
	// changes made to one can be easily made to the other.
	
	if (openP->hdrP->appInfoID)
		appInfoH = MemLocalIDToGlobal (openP->hdrP->appInfoID, openP->cardNo);
	else
		appInfoH = 0;
	
	left = base;
	pivot = base + 1;
	right = base + numOfElements;	// pivot should never be used when >= right
											// Since right can be past the end of the DB

	leftEntryP = PrvRecordEntryPtr (openP, (UInt16)left, 0);
	leftRecP = derefRecord (openP, leftEntryP);
	
	// perform an insertion sort.  Scan from left to right.  For each item not
	// >= then the last binary insert it in place. O(n) = n log2 n comparisons but
	// still O(n) = n^2 data movements.
	while (pivot < right)
		{
		pivotEntryP = PrvRecordEntryPtr (openP, (UInt16)pivot, 0);
		pivotRecP = derefRecord (openP, pivotEntryP);

		// Is the next item >= then the last?
		if (compareRecords (leftRecP, pivotRecP, leftEntryP, pivotEntryP) > 0) 
			{
			// Binary insert into place.
			binaryLeft = base;
			binaryRight = left;
			
			
			while (binaryLeft < binaryRight)
				{
				binaryMiddle = ((UInt32)(binaryLeft + binaryRight)) / 2;
				
				// Get and compare the middle record
				ErrNonFatalDisplayIf( binaryMiddle < 0, "Attempting to access a record with a negative index");
	
				middleEntryP = PrvRecordEntryPtr (openP, (UInt16)binaryMiddle, 0);
				middleRecP = derefRecord (openP, middleEntryP);
				result = compareRecords (pivotRecP, middleRecP,	pivotEntryP, middleEntryP);
				
				if (result < 0)
					{
					binaryRight = binaryMiddle - 1;
					}
				else
					{
					// result >= 0
					binaryLeft = binaryMiddle + 1;
					}
				}
			
			// The binaryLeft and binaryRight have converged to a position.
			// This position may not have been tested.  The record to insert
			// might belong after this position.  Check for this.
			if (binaryLeft >= binaryRight)
				{
				binaryMiddle = binaryLeft;
				middleEntryP = PrvRecordEntryPtr (openP, (UInt16)binaryMiddle, 0);
				middleRecP = derefRecord (openP, middleEntryP);
				result = compareRecords (pivotRecP, middleRecP, pivotEntryP, middleEntryP);
				if (result >= 0)
					binaryMiddle++;		// insert after 
				}
			
			// Move the record from pivot to binaryMiddle
			DmMoveRecord(dbP, (UInt16)pivot, (UInt16)binaryMiddle);
			
			// Advance
			left = pivot;
			leftEntryP = PrvRecordEntryPtr (openP, (UInt16)left, 0);
			leftRecP = derefRecord (openP, leftEntryP);
			pivot++;
			}
		else
			{
			// Advance
			left = pivot;
			leftRecP = pivotRecP;
			leftEntryP = pivotEntryP;
			pivot++;
			}
		
		// Briefly release and reclaim the write semaphore to allow other tasks to 
		// to get some time.  A read semaphore will still be in effect.
		// Because we have been busy for some unknown length of time, reset the 
		// auto off timer so that the device isn't shut down by the ui thread.  Do 
		// this before releasing the semaphore which can allow the ui thread to run.
		EvtResetAutoOffTimer();
		}

}


/************************************************************
 *
 *  FUNCTION: DmInsertionSort
 *
 *  DESCRIPTION: Sorts records in a database.  Deleted records 
 *  are placed last in any order.  All others are sorted 
 *  according to the passed comparison function.  Only records 
 *  which are out of order move.  Moved records are moved to 
 *  the end of the range of equal records.  If a large amount 
 *  of records are being sorted try to use the quick sort.
 *
 *  This the insertion sort algorithm.  Starting with the second
 *  record, each record is compared to the preceeding record.
 *  Each record not greater than the last is inserted into 
 *  sorted position within those already sorted.  A binary insertion
 *  is performed.  A moved record is inserted after any other
 *  equal records.
 *
 *  PARAMETERS: database access pointer, comparison function, 
 *		other data passed to the comparison function.
 *
 *	 *fcmp, the comparison function, accepts two arguments, elem1
 *  and elem2, each a pointer to an entry in the table. The
 *  comparison function compares each of the pointed-to items
 *  (*elem1 and *elem2), and returns an integer based on the result
 *  of the comparison.
 * 
 *  If the items            fcmp returns
 *  *elem1 <  *elem2         an integer < 0
 *  *elem1 == *elem2         0
 *  *elem1 >  *elem2         an integer > 0
 * 
 *  RETURNS: a sorted database
 *
 *  CREATED: 11/30/95
 *
 *  BY: Roger Flores
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	7/2/98	Also reserve a read semaphore so children can 
 *								release and reserve the write semaphore.
 *			roger	8/5/98	Added check for resource databases
 *			mchen 1/2/01	Change so MemSemaphore is held only when
 *								doing the locking and unlocking.  This
 *								closes the hole where the user comparison
 *								function is called when the storage heap
 *								area is write enabled.
 *
 *************************************************************/

Err DmInsertionSort(const DmOpenRef dbR, DmComparF *compar, Int16 other)
{
	DmAccessPtr			dbP = (DmAccessPtr)dbR;
	DmOpenInfoPtr		openP;
	UInt16				numOfRecords;
	UInt16				recordIndex;
	MemHandle			recordH;
	Err					err = 0;

	ECDmValidateDmOpenRef(dbP);
		
	openP = dbP->openP;

	// Check for errors
	if (!openP->writeAccess)
		{err = dmErrReadOnly; goto Exit;}

	if (openP->resDB) 
		{err = dmErrNotRecordDB; goto Exit;}
	
	// Only sort if 2 or more records need sorting.
	if (openP->numRecords < 2)
		goto Exit;
		
	numOfRecords = openP->numRecords;

	// Enable writes to storage memory because we are changing a lot of memory.  This
	// also speeds up the following lock and unlock code.
	MemSemaphoreReserve(true);
		

	// Lock all the records.  Note that numOfRecords can be at most 0xFFFF (65535)
	// and valid indices are 0-65534 (0-0xFFFE)
	for (recordIndex = 0; recordIndex < numOfRecords; recordIndex++)
		{
		recordH = DmQueryRecord(dbR, recordIndex);
		if (recordH) MemHandleLock(recordH);
		}

	MemSemaphoreRelease(true);

	// Do the insertion sort
	PrvInsertionSort (dbP, 0, numOfRecords, compar, other);

	MemSemaphoreReserve(true);

	// Unlock all the records
	for (recordIndex = 0; recordIndex < numOfRecords; recordIndex++)
		{
		recordH = DmQueryRecord(dbR, recordIndex);
		if (recordH) MemHandleUnlock(recordH);
		}

	// Restore write-protection
	MemSemaphoreRelease(true);

	// Because we have been busy for some unknown length of time, reset the 
	// auto off timer so that the device isn't shut down by the ui thread.  Do 
	// this before releasing the semaphore which can allow the ui thread to run.
	EvtResetAutoOffTimer();

	
Exit:
	ErrNonFatalDisplayIf(err, "Err insertion sort");
	return err;
}


/************************************************************
 *
 *  FUNCTION: PrvQuickSort
 *
 *  DESCRIPTION: Sorts records in a database.  Deleted records 
 *  are placed last in any order.  All others are sorted 
 *  according to the passed comparison function.  Remember that
 *  equal records can be in any position relative to each other
 *  because a quick sort tends to scramble the ordering of records.
 *  All records are presumed to locked.
 *
 *  This the common quick sort algorithm.  The pivot point is picked
 *  by picking the middle of three records picked from around the 
 *  middle of the records to sort to take advantage of partially
 *  sorted data.  The routine contains it's own stack to limit
 *  uncontrolled recursion.  When the stack is full an insertion 
 *  sort is used instead because it doesn't require more stack 
 *  space.  An insertion sort is also used when the number of records
 *  is low.  This avoids the overhead of a quick sort which is
 *  noticeable for small numbers of records.  The last optimization
 *  is that if the records seem mostly sorted an insertion sort is 
 *  performed to move only those few records needing moving.
 *
 *  PARAMETERS: database access pointer, pivot record, number of
 *  records to sort (must be at least 2), comparison function, 
 *  other data passed to the comparison function.
 *
 *  RETURNS: a sorted portion of the database
 *
 *  CREATED: 1/30/95
 *
 *  BY: Roger Flores
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	7/2/98	release and reserve the write semaphore to permit task switching
 *			mchen	1/2/01	remove release and reserve of the write semaphore
 *								because it is no longer being held while this
 *								function runs
 *
 *************************************************************/
 
    		
typedef struct {
	UInt16	base;
	UInt16	numOfElements;
} SortStackType;

#define stackSpace		20		// On avg. the mose used should be about 12 for a 512K card.

#define insertionSortThreshold	12



static void PrvQuickSort (const DmAccessPtr dbP, UInt16 base, UInt16 numOfElements, 
	const DmComparF *Fcmp, Int16 other)
{
   Int16      		result;
	Int32 			pivot, pivotLocation;
	Int32     		left=0, right=0, temp;
	Int32				binaryLeft, binaryMiddle, binaryRight;
   UInt16			movedRecords;
   MemPtr			leftRecP=0, rightRecP=0, pivotRecP=0, middleRecP, tempRecP;
   RecordEntryPtr	leftEntryP;
   RecordEntryPtr rightEntryP;
   RecordEntryPtr	pivotEntryP;
   RecordEntryPtr	middleEntryP;
   RecordEntryPtr	tempEntryP;
	DmOpenInfoPtr	openP = dbP->openP;
	SortStackType	stack[stackSpace];
	SortStackType	*sp = stack;
	MemHandle		appInfoH;

	if (openP->hdrP->appInfoID)
		appInfoH = MemLocalIDToGlobal (openP->hdrP->appInfoID, openP->cardNo);
	else
		appInfoH = 0;
	
	while (true)
		{
		// base and numOfElements must be set for the range to sort
		
		
		// This is the first part of the quick sort algorithm.  It actually
		// is an insertion sort.  It is used when the number of elements is
		// low (quick sort has too much overhead) or when the function's stack
		// is out of space.
		if (numOfElements <= insertionSortThreshold || 
			sp >= &stack[stackSpace])
			{
doInsertionSort:
			left = base;
			pivot = base + 1;
			right = base + numOfElements;	// pivot should never be used when >= right
														// Since right can be past the end of the DB

			leftEntryP = PrvRecordEntryPtr (openP, (UInt16)left, 0);
			leftRecP = derefRecord (openP, leftEntryP);	
			
			// perform an insertion sort.  Scan from left to right.  For each item not
			// >= then the last binary insert it in place. O(n) = n log2 n comparisons but
			// still O(n) = n^2 data movements.
			while (pivot < right)
				{
				pivotEntryP = PrvRecordEntryPtr (openP, (UInt16)pivot, 0);
				pivotRecP = derefRecord (openP, pivotEntryP);

				// Is the next item >= then the last?
				if (compareRecords (leftRecP, pivotRecP, leftEntryP, pivotEntryP) > 0) 
					{
					// Binary insert into place.
					binaryLeft = base;
					binaryRight = left;
					
					
					while (binaryLeft < binaryRight)
						{
						binaryMiddle = ((UInt32)(binaryLeft + binaryRight)) / 2;
						
						// Get and compare the middle record
						middleEntryP = PrvRecordEntryPtr (openP, (UInt16)binaryMiddle, 0);
						middleRecP = derefRecord (openP, middleEntryP);
						result = compareRecords (pivotRecP, middleRecP, pivotEntryP, middleEntryP);
						
						if (result < 0)
							{
							binaryRight = binaryMiddle - 1;
							}
						else if (result > 0)
							{
							binaryLeft = binaryMiddle + 1;
							}
						else
							// Equal.  Just place the item here.
							break;
						}
					
					// The binaryLeft and binaryRight have converged to a position.
					// This position may not have been tested.  The record to insert
					// might belong after this position.  Check for this.
					if (binaryLeft >= binaryRight)
						{
						binaryMiddle = binaryLeft;
						middleEntryP = PrvRecordEntryPtr (openP, (UInt16)binaryMiddle, 0);
						middleRecP = derefRecord (openP, middleEntryP);
						result = compareRecords (pivotRecP, middleRecP, pivotEntryP, middleEntryP);
						if (result > 0)
							binaryMiddle++;		// insert after 
						}
					
					// Move the record from pivot to binaryMiddle
					DmMoveRecord(dbP, (UInt16)pivot, (UInt16)binaryMiddle);
					
					// Advance
					left = pivot;
					leftEntryP = PrvRecordEntryPtr (openP, (UInt16)left, 0);
					leftRecP = derefRecord (openP, leftEntryP);
					pivot++;
					}
				else
					{
					// Advance
					left = pivot;
					leftRecP = pivotRecP;
					leftEntryP = pivotEntryP;
					pivot++;
					}
				
				// Because we have been busy for some unknown length of time, reset the 
				// auto off timer so that the device isn't shut down by the ui thread.
				EvtResetAutoOffTimer();
				}

			
			// We are done with portion of the data.  Check the stack for
			// another portion to sort.  If there isn't any then we're done.
			if (sp != stack)
				{
				sp--;
				base = sp->base;
				numOfElements = sp->numOfElements;
				}
			else
				break;							// done with the sort.
			}
		else
			{
			// This is the second part of the quick sort.  Here is the sorting
			// performed is the real quick sort algorithm.  A record is picked
			// close to the middle of the sort order and the records are
			// separated in half to those less and those greater than the pivot.
			// One half is placed on the stack for later sorting.  The algorithm 
			// loops and then sorts the other half.
			
			
			// A good deal of the data we sort is already partially sorted.  Try
			// to pick the pivot from around the middle of the data.
			pivot = (numOfElements / 2) + base;
			left = pivot - ((numOfElements / 16) | 1);		// 1/16 offcenter - at least 1
			right = pivot + ((numOfElements / 16) | 1);		// 1/16 offcenter - at least 1
			
			leftEntryP = PrvRecordEntryPtr (openP, (UInt16)left, 0);
			leftRecP = derefRecord (openP, leftEntryP);

			rightEntryP = PrvRecordEntryPtr (openP, (UInt16)right, 0);
			rightRecP = derefRecord (openP, rightEntryP);

			pivotEntryP = PrvRecordEntryPtr (openP, (UInt16)pivot, 0);
			pivotRecP = derefRecord (openP, pivotEntryP);

			// Make pivot point to the middle most record of the three.
		   if (compareRecords(leftRecP, pivotRecP, leftEntryP, pivotEntryP) > 0) 
				{
				temp = left;
				tempRecP = leftRecP;
				tempEntryP = leftEntryP;
				left = pivot;
				leftRecP = pivotRecP;
				leftEntryP = pivotEntryP;
				pivot = temp;
				pivotRecP = tempRecP;
				pivotEntryP = tempEntryP;
				}

		   if (compareRecords(pivotRecP, rightRecP, pivotEntryP, rightEntryP) > 0) 
				{
				temp = pivot;
				tempRecP = pivotRecP;
				tempEntryP = pivotEntryP;
				pivot = right;
				pivotRecP = rightRecP;
				pivotEntryP = rightEntryP;
				right = temp;
				rightRecP = tempRecP;
				rightEntryP = tempEntryP;
				}

		   if (compareRecords(leftRecP, pivotRecP, leftEntryP, pivotEntryP) > 0) 
				{
				temp = left;
				tempRecP = leftRecP;
				tempEntryP = leftEntryP;
				left = pivot;
				leftRecP = pivotRecP;
				leftEntryP = pivotEntryP;
				pivot = temp;
				pivotRecP = tempRecP;
				pivotEntryP = tempEntryP;
				}
			
			
			// Move the pivot to the beginning so that it isn't overrun during the
			// swapping of elements.
			PrvExchangeRecord (dbP, (UInt16)base, (UInt16)pivot);
			pivotLocation = pivot;
			pivot = base;
			
			// Set the end points of the swap run
			left = base + 1;
			right = base + numOfElements - 1;

			leftEntryP = PrvRecordEntryPtr (openP, (UInt16)left, 0);
			leftRecP = derefRecord (openP, leftEntryP);

			rightEntryP = PrvRecordEntryPtr (openP, (UInt16)right, 0);
			rightRecP = derefRecord (openP, rightEntryP);

			pivotEntryP = PrvRecordEntryPtr (openP, (UInt16)pivot, 0);
			pivotRecP = derefRecord (openP, pivotEntryP);

			movedRecords = 0;
			
			// repetitively swap elements
			while (true)
				{
				// Scan from the left to the right for an element greater than the pivot
				while (compareRecords(leftRecP, pivotRecP, leftEntryP, pivotEntryP) < 0)
					{
					left++;
					leftEntryP = PrvRecordEntryPtr (openP, (UInt16)left, 0);
					leftRecP = derefRecord (openP, leftEntryP);					
					}
				
				// Scan from the right to the left for an element less than the pivot
				while (compareRecords(pivotRecP, rightRecP, pivotEntryP, rightEntryP) < 0)
					{
					right--;
					rightEntryP = PrvRecordEntryPtr (openP, (UInt16)right, 0);
					rightRecP = derefRecord (openP, rightEntryP);
					}
				
				// Check to see if left crossed right
				if (left > right)
					break;
				
				// Exchange the two records and scan again
				PrvExchangeRecord (dbP, (UInt16)left, (UInt16)right);
				left++;
				right--;

				leftEntryP = PrvRecordEntryPtr (openP, (UInt16)left, 0);
				leftRecP = derefRecord (openP, leftEntryP);
	
				rightEntryP = PrvRecordEntryPtr (openP, (UInt16)right, 0);
				rightRecP = derefRecord (openP, rightEntryP);

				
				movedRecords++;
				
				// Because we have been busy for some unknown length of time, reset the 
				// auto off timer so that the device isn't shut down by the ui thread.
				EvtResetAutoOffTimer();
				}
				
			// Done moving all the records.  Restore the pivot.
			// If the records seem ordered take extra care
			if (movedRecords > (numOfElements >> 2) )
				{
				PrvExchangeRecord (dbP, (UInt16)pivot, (UInt16)right);
				}
			else
				{
				// if pivot really was in middle (well sorted case)
				if (right == pivotLocation)
					{
					// Moves the first record back into place
					PrvExchangeRecord (dbP, (UInt16)pivot, (UInt16)right);	// safe to do
					}
				else
					{
					// Move the record from pivot to right.  The first
					// record isn't back in place - it's lost.  The move
					// does prevent a high position record from being swapped
					// from right to the first position.
					DmMoveRecord(dbP, (UInt16)pivot, (UInt16)right);
					}
				}


			// Check if the data was mostly sorted.
			// We could avoid the goto but it means slowing down the algorithm.
			// If less than four records were swapped out of at least 32 then 
			// just move those records into place.  (least orderly case)
			if ( (movedRecords - 1) < (numOfElements >> 5) )
				{
				goto doInsertionSort;
				}
			else
				{
				// Now sort the smaller of the two halves and place the larger
				// one on the stack for later.
				if ((right - base + 1) > ((base + numOfElements) - left))
					{
					// the left half is larger
					sp->numOfElements = right - base + 1;
					sp->base = base;
					numOfElements -= left - base;
					base = left;
					}
				else
					{
					// the right half is larger
					sp->numOfElements = (base + numOfElements) - left;
					sp->base = left;
					numOfElements = right - base + 1;
					}
				sp++;
				}
			}

		}
			

	// exit
}


/************************************************************
 *
 *  FUNCTION: DmQuickSort
 *
 *  DESCRIPTION: Sorts records in a database.  Deleted records 
 *  are placed last in any order.  All others are sorted 
 *  according to the passed comparison function.
 *
 *  PARAMETERS: database access pointer, comparison function, 
 *		other data passed to the comparison function.
 *
 *	 *fcmp, the comparison function, accepts two arguments, elem1
 *  and elem2, each a pointer to an entry in the table. The
 *  comparison function compares each of the pointed-to items
 *  (*elem1 and *elem2), and returns an integer based on the result
 *  of the comparison.
 * 
 *  If the items            fcmp returns
 *  *elem1 <  *elem2         an integer < 0
 *  *elem1 == *elem2         0
 *  *elem1 >  *elem2         an integer > 0
 * 
 *  RETURNS: a sorted database
 *
 *  CREATED: 1/30/95
 *
 *  BY: Roger Flores
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	7/2/98	Also reserve a read semaphore so children can 
 *								release and reserve the write semaphore.
 *			roger	8/5/98	Added check for resource databases
 *			mchen 1/2/01	Change so MemSemaphore is held only when
 *								doing the locking and unlocking.  This
 *								closes the hole where the user comparison
 *								function is called when the storage heap
 *								area is write enabled.
 *
 *************************************************************/
Err DmQuickSort(const DmOpenRef dbR, DmComparF *compar, Int16 other)
{
	DmAccessPtr			dbP = (DmAccessPtr)dbR;
	DmOpenInfoPtr		openP;
	UInt16				numOfRecords;
	UInt16				recordIndex;
	MemHandle			recordH;
	Err					err = 0;

	ECDmValidateDmOpenRef(dbP);
		
	openP = dbP->openP;

	if (!openP->writeAccess)
		{err = dmErrReadOnly; goto Exit;}

	if (openP->resDB) 
		{err = dmErrNotRecordDB; goto Exit;}
	
	// Only sort if 2 or more records need sorting.
	if (openP->numRecords < 2)
		goto Exit;
		
	numOfRecords = openP->numRecords;

	// Enable writes to storage memory because we are changing a lot of memory.  This
	// also speeds up the following lock and unlock code.
	MemSemaphoreReserve(true);
		
	// Lock all the records

	for (recordIndex = 0; recordIndex < numOfRecords; recordIndex++)
		{
		recordH = DmQueryRecord(dbR, recordIndex);
		if (recordH) MemHandleLock(recordH);
		}
	MemSemaphoreRelease(true);

	// Do the quick sort
	PrvQuickSort (dbP, 0, numOfRecords, compar, other);


	MemSemaphoreReserve(true);

	// Unlock all the records
	for (recordIndex = 0; recordIndex < numOfRecords; recordIndex++)
		{
		recordH = DmQueryRecord(dbR, recordIndex);
		if (recordH) MemHandleUnlock(recordH);
		}
	MemSemaphoreRelease(true);
	
	// Because we have been busy for some unknown length of time, reset the 
	// auto off timer so that the device isn't shut down by the ui thread.  Do 
	// this before releasing the semaphore which can allow the ui thread to run.
	EvtResetAutoOffTimer();

	
Exit:
	ErrNonFatalDisplayIf(err, "Err insertion sort");
	return err;
}


/************************************************************
 *
 *  FUNCTION: DmFindSortPosition
 *
 *  DESCRIPTION: Return where a record is or should be.
 *		Useful to find or find where to insert a record.
 *    Uses a binary search.
 *
 *  PARAMETERS: database access pointer, record MemPtr, 
 *		comparison function pointer, other info for comparison
 *
 *  RETURNS: the position where the record should be inserted.
 *    The position should be viewed as between the record returned
 *    and the record before it.  Note that the return value may
 *    be one greater than the number of records.
 *
 *  CREATED: 6/16/95 
 *
 *  BY: Roger Flores
 *
 *		roger	8/5/98	Added check for resource databases
 *		mchen 1/3/2001 Rewritten to use a binary search that
 *							ends when the pointers cross rather
 *							than dealing with equal values with a
 *							linear search.  The old method was very
 *							slow if the database had many equal
 *							records.
 *
 *************************************************************/
UInt16 DmFindSortPosition(DmOpenRef dbR, void * newRecord, SortRecordInfoPtr newRecordInfo,
	DmComparF *compar, Int16 other)
{
	DmAccessPtr			dbP = (DmAccessPtr)dbR;
	DmOpenInfoPtr		openP;
	MemHandle 			recordH;
	
	Int32 				numOfRecords;
	RecordEntryPtr		recordP;
	void *				*r;
	Int32					lPos;
	Int32					rPos, mPos;
	Int16 				result;						// result of comparing two records
	MemHandle			appInfoH;
	
	ECDmValidateDmOpenRef(dbP);
		
	openP = dbP->openP;

	ErrNonFatalDisplayIf (openP->resDB, "Record DB only");
	
	numOfRecords = openP->numRecords;
	
	if (numOfRecords == 0) {
		return 0;
	}
	
	lPos = 0;
	rPos = numOfRecords - 1;
	
	if (openP->hdrP->appInfoID)
		appInfoH = MemLocalIDToGlobal (openP->hdrP->appInfoID, openP->cardNo);
	else
		appInfoH = 0;
		
	while (lPos <= rPos)
		{
		mPos = ((UInt32)(lPos + rPos)) / 2;
		
		// Compare the two records.  Treat deleted records as greater.
		recordP = PrvRecordEntryPtr(openP, (UInt16)mPos, 0);
		if ((recordP == NULL) || (recordP->attributes & dmRecAttrDelete)) 
			{
			result = -1;
			}
		else
			{
			// If it's pointer based, Get the MemHandle out of our MemHandle table
			if (openP->handleTableP) {
				recordH = (MemHandle)&(openP->handleTableP[mPos]);
				}
			else {
				recordH = MemLocalIDToGlobal(recordP->localChunkID, openP->cardNo);
				}
			r = MemHandleLock(recordH);
			ErrFatalDisplayIf(r == 0, "DmFindSortPosition: data somehow missing");

			result = compar(newRecord, r, other, newRecordInfo, 
				(SortRecordInfoPtr) &recordP->attributes, appInfoH);
				
			MemHandleUnlock(recordH);
			}
		
		
		// comparison is >= so that new records go AFTER equal ones
		if (result >= 0)
			{
				lPos = mPos + 1;
			}
		else
			{
				rPos = mPos - 1;
			}		
		}
		
	// check last comparison
	return (result < 0) ? mPos : mPos + 1;
}

/************************************************************
 *
 *  FUNCTION: DmFindSortPositionV10
 *
 *  DESCRIPTION: Return where a record is or should be.
 *		Useful to find or find where to insert a record.
 *    Uses a binary search.
 *
 *  PARAMETERS: database access pointer, record MemPtr, 
 *		comparison function pointer, other info for comparison
 *
 *  RETURNS: the position where the record should be inserted.
 *    The position should be viewed as between the record returned
 *    and the record before it.  Note that the return value may
 *    be one greater than the number of records.
 *
 *  CREATED: 6/16/95 
 *
 *  BY: Roger Flores
 *
 *************************************************************/
UInt16 DmFindSortPositionV10 (DmOpenRef dbR, void * newRecord, 
	DmComparF *compar, Int16 other)
{
	return (DmFindSortPosition (dbR, newRecord, NULL,	compar, other));
}


/************************************************************
 *
 *  FUNCTION: DmNewRecord
 *
 *  DESCRIPTION: Returns a MemHandle to a new record in 
 *    the database and marks the record busy
 *
 *  PARAMETERS: database access pointer, record index MemPtr, 
 *		desired size.
 *
 *  RETURNS: MemPtr to record data
 *
 *  CREATED: 11/16/94 
 *
 *  BY: Ron Marianetti
 *
 *	HISTORY:
 *		4/6/2000	ryw	set last err to zero, use PrvSetLastErr
 *************************************************************/
MemHandle	DmNewRecord(DmOpenRef dbR, UInt16 * atP, UInt32 size)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	Err				err=0;
	MemHandle		recordH=0;
	DmOpenInfoPtr	openP;
	RecordEntryPtr	recordP;


	// Enable writes
	MemSemaphoreReserve(true);
		
	ECDmValidateDmOpenRef(dbP);
		
	openP = dbP->openP;

	// Check for errors
	if (!openP->writeAccess) {
		err = dmErrReadOnly; 
		goto Exit;
		}
		
	if (openP->resDB) {
		err = dmErrNotRecordDB; 
		goto Exit;
		}
	
	// First, try and allocate a new chunk of the desired size in the
	//  same card as the database header
	recordH = PrvDmNewHandle(dbP, size);
	if (!recordH) {
		err = dmErrMemError; 
		goto ExitNoErrCheck;
		}
	

	// Now, attach it to the database
	err = DmAttachRecord(dbP, atP, recordH, 0);
	if (err) {
		if (err == memErrNotEnoughSpace || err == dmErrMemError) goto ExitNoErrCheck;
		else goto Exit;
		}


	// Mark busy and dirty
	recordP = PrvRecordEntryPtr(openP, *atP, 0);
	recordP->attributes |= (dmRecAttrBusy | dmRecAttrDirty);

	// Bump the mod number
	openP->hdrP->modificationNumber++;


Exit:
	ErrNonFatalDisplayIf(err, "Err creating new rec");

ExitNoErrCheck:

	// Restore write-protection
	MemSemaphoreRelease(true);
	
	PrvSetLastErr(err);
	if (err) {
		if (recordH) MemHandleFree(recordH);
		recordH = 0;
		}

	return recordH;
}



/************************************************************
 *
 *  FUNCTION: DmRemoveRecord
 *
 *  DESCRIPTION: Detaches and deletes a record without a trace
 *
 *  PARAMETERS: database access pointer, record index
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 11/16/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err	DmRemoveRecord(DmOpenRef dbR, UInt16 index)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	Err				err;
	MemHandle			h;
	DmOpenInfoPtr	openP;

	// Enable writes
	MemSemaphoreReserve(true);
		
	ECDmValidateDmOpenRef(dbP);
		
	openP = dbP->openP;

	// Check for errors
	if (!openP->writeAccess)
		{err = dmErrReadOnly; goto Exit;}

	// First, detach it
	err = DmDetachRecord(dbP, index, &h);
	if (err) goto Exit;

	// Now, free the memory used by the record
	if (h) err = MemHandleFree(h);
	
	// Bump the mod number
	openP->hdrP->modificationNumber++;


Exit:
	// Restore write-protection
	MemSemaphoreRelease(true);
		
	ErrNonFatalDisplayIf(err, "Err Removing rec");
	return err;
}



/************************************************************
 *
 *  FUNCTION: DmDeleteRecord
 *
 *  DESCRIPTION: Deletes a record's chunk from a database but
 *		leaves the record entry in the header and sets the delete bit
 *		for the next sync.
 *
 *  PARAMETERS: database access pointer, record index
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 11/16/94 
 *
 *  BY: Ron Marianetti
 *
 *		vmk	11/18/97		Mark the record dirty
 *		jed	01/13/99		clear busy bit
 *		LFe	10/01/99		check record not already deleted or archived
 *
 *************************************************************/
Err	DmDeleteRecord(DmOpenRef dbR, UInt16 index)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	Err				err;
	LocalID			id;
	MemHandle			h;
	DmOpenInfoPtr	openP;
	RecordEntryPtr	recordP;

	// Enable writes
	MemSemaphoreReserve(true);
		
	ECDmValidateDmOpenRef(dbP);
		
	openP = dbP->openP;

	// Check for errors
	if (!openP->writeAccess)
		{err = dmErrReadOnly; goto Exit;}
		
	// Set the delete bit
	recordP = PrvRecordEntryPtr(openP, index, 0);
	if (!recordP) {
		err = dmErrIndexOutOfRange;
		goto Exit;
		}
		
	// Check if the record was not yet deleted or archived
	if (recordP->attributes & dmRecAttrDelete) {
		err = (recordP->localChunkID) ? dmErrRecordArchived: dmErrRecordDeleted;
		goto Exit;
		}

	recordP->attributes |= (dmRecAttrDelete | dmRecAttrDirty);
	
	// clear the busy bit
	recordP->attributes &= ~dmRecAttrBusy;
	
	// Now, delete the chunk and set the chunkID to nil
	id = recordP->localChunkID;
	if (id) {
		h = MemLocalIDToGlobal(id, openP->cardNo);
		err =  MemHandleFree(h);
		}
	
	recordP->localChunkID = 0;


	// Bump the mod number
	openP->hdrP->modificationNumber++;


Exit:
	// Restore write-protection
	MemSemaphoreRelease(true);
		

	ErrNonFatalDisplayIf(err, "Err Deleting rec");
	return err;
}



/************************************************************
 *
 *  FUNCTION: DmArchiveRecord
 *
 *  DESCRIPTION: Marks a record as archived by
 *		leaving the record's chunk around and setting the delete bit
 *		for the next sync.
 *
 *  PARAMETERS: database access pointer, record index
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 11/16/94 
 *
 *  BY: Ron Marianetti
 *
 *		vmk	11/18/97		Mark the record dirty
 *		jed	02/01/99		clear busy bit
 *		grant	08/01/99		Reset the lock count on the chunk
 *		LFe	10/01/99		check record not already deleted or archived
 *
 *************************************************************/
Err	DmArchiveRecord(DmOpenRef dbR, UInt16 index)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	DmOpenInfoPtr	openP;
	Err				err;
	RecordEntryPtr	recordP;
	LocalID			id;
	MemHandle		h;

	// Enable writes
	MemSemaphoreReserve(true);
		
	ECDmValidateDmOpenRef(dbP);
		
	openP = dbP->openP;

	// Check for errors
	if (!openP->writeAccess)
		{err = dmErrReadOnly; goto Exit;}
		
	// Set the delete bit
	recordP = PrvRecordEntryPtr(openP, index, 0);
	if (!recordP) {
		err = dmErrIndexOutOfRange;
		goto Exit;
		}
	
	// Check if the record was not yet deleted or archived
	if (recordP->attributes & dmRecAttrDelete) {
		err = (recordP->localChunkID) ? dmErrRecordArchived: dmErrRecordDeleted;
		goto Exit;
		}
		
	recordP->attributes |= (dmRecAttrDelete | dmRecAttrDirty);
	
	// clear the busy bit
	recordP->attributes &= ~dmRecAttrBusy;
	
	// reset the lock count on the chunk
	id = recordP->localChunkID;
	if (id) {
		h = MemLocalIDToGlobal(id, openP->cardNo);
		err = MemHandleResetLock(h);
		}
	else {
		err = 0;
	}
	
	// Bump the mod number
	openP->hdrP->modificationNumber++;


Exit:
	ErrNonFatalDisplayIf(err, "Err Archiving rec");
		
	// Restore write-protection
	MemSemaphoreRelease(true);
	return err;
}


/************************************************************
 *
 *  FUNCTION: DmFindRecordByID
 *
 *  DESCRIPTION: Returns the index of the record with the given unique ID
 *
 *  PARAMETERS: 
 *			dbP - database access MemPtr
 *			uniqueID - unique ID to search for
 *			*indexP - return index
 *
 *  RETURNS: 0 if found.
 *
 *  CREATED: 6/15/95
 *
 *  BY: Ron Marianetti
 *
 *		roger  08/05/1998	Added check for resource databases
 *		ryw    01/30/2001	optimized linear search, speeding
 *                          hotsyncing to the device by >20%
 *
 *************************************************************/
Err	DmFindRecordByID (DmOpenRef dbR, UInt32 uniqueID, UInt16 * indexP)
{
	DmAccessPtr			dbP = (DmAccessPtr)dbR;
	UInt16				index;
	UInt16 				count;
	RecordEntryPtr		recordP, arrayEnd;
	DmOpenInfoPtr		openP;
	register UInt32	mask;
	UInt8				* idP;
	UInt16              whichArray;
	
	ECDmValidateDmOpenRef(dbP);
		
	openP = dbP->openP;
	
	// Error check
	ErrNonFatalDisplayIf (openP->resDB, "Record DB only");
	
	ErrFatalDisplayIf(uniqueID & 0xFF000000, "Invalid uniqueID passed");

	// Get total number of records
	count = openP->numRecords;

	// Form mask
	mask = 0x00FFFFFF;

	// Optimized linear search of the list of arrays
	index = 0;
	whichArray = 0;
	recordP = arrayEnd = 0;
	while( true ){
		if( recordP == arrayEnd ){
		   	// get next array
		   	if( ! PrvGetRecordEntryArrayBounds(openP, whichArray++, &recordP, &arrayEnd) ){
		      	ErrNonFatalDisplayIf( index != count, "all records were not searched" );
		      	return dmErrUniqueIDNotFound;
		   		}
			}
		else{
			// check for match
			idP = ((UInt8 *)(&recordP->uniqueID)) - 1;
			if( (*((UInt32 *)idP) & mask) == uniqueID ){
				*indexP = index;
				return 0;
				}
			// next entry
			++recordP;
			++index;
			}
		}
}



/************************************************************
 *
 *  FUNCTION: DmQueryRecord
 *
 *  DESCRIPTION: Returns a MemHandle to a record for reading only 
 *		(does not set the busy bit).
 *		If the record is ROM based (pointer accessed) this routine
 *		will return the fake MemHandle to it.
 *
 *  PARAMETERS: database access MemPtr, index
 *
 *  RETURNS: record MemHandle
 *
 *  CREATED: 11/21/94 
 *
 *  BY: Ron Marianetti
 *
 *	HISTORY:
 *		4/6/2000	ryw	use PrvSetLastErr to report errors
 *************************************************************/
MemHandle	DmQueryRecord(DmOpenRef dbR, UInt16 index)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	LocalID 			id;
	MemHandle		recordH = 0;
	RecordEntryPtr	recordP;
	DmOpenInfoPtr	openP;
	Err				err = 0;
	
	ECDmValidateDmOpenRef(dbP);
		
	openP = dbP->openP;

	// Range Check, or else we might get an error message from PrvRecordEntryPtr
 	if (index >= openP->numRecords) {
 		err = dmErrIndexOutOfRange; 
 		goto ExitNoCheck;
 		}

	// Get the entry MemPtr
	recordP = PrvRecordEntryPtr(openP, index, 0);
	if (!recordP) {
		err = dmErrIndexOutOfRange; 
		goto Exit;
		}
	
	// Check the attributes and make sure it's not marked for deletion
	if (recordP->attributes & dmRecAttrDelete) {
		err = dmErrRecordDeleted;
 		goto ExitNoCheck;
		}

	// If it's pointer based, Get the MemHandle out of our MemHandle table
	if (openP->handleTableP) {
		recordH = (MemHandle)&(openP->handleTableP[index]);
		}
	else {
		id = recordP->localChunkID;
		recordH = MemLocalIDToGlobal(id, openP->cardNo);
		}
	
Exit:
	ErrNonFatalDisplayIf(err, "Err Querying rec");
ExitNoCheck:
	PrvSetLastErr(err);
	return recordH;
}



/************************************************************
 *
 *  FUNCTION: DmGetRecord
 *
 *  DESCRIPTION: Returns a MemHandle to a record and sets the busy bit for
 *		the record. If the record is ROM based (pointer accessed) this routine
 *		will make a fake MemHandle to it and store this MemHandle in the
 *		DmAccessType structure.
 *
 *  PARAMETERS: database access ptr, record index
 *
 *	RETURNS: record handle
 *
 *	HISTORY:
 *		11/21/94	RM		Created by Ron Marianetti.
 *		09/30/99	Ludovic	DmGetRecord now returns dmErrRecordDeleted
 *		03/15/00	RYW	DmGetRecord has been minutely optimized now saving about 
 *                   9 instructions and at least one memory accesses (8% faster)
 *		4/6/2000	ryw	use PrvSetLastErr to report errors
 *************************************************************/
MemHandle	DmGetRecord(DmOpenRef dbR, UInt16 index)
{
	MemHandle		recordH = 0;
	Err				err = 0;
	RecordEntryPtr	recordP;
	DmOpenInfoPtr	openP;

	
	// Enable writes
	MemSemaphoreReserve(true);
	
	ECDmValidateDmOpenRef((DmAccessPtr)dbR);
		
	openP = ((DmAccessPtr)dbR)->openP;
	
	// Get record entry MemPtr
	recordP = PrvRecordEntryPtr(openP, index, 0);
	if (!recordP) {
		err = dmErrIndexOutOfRange; 
		goto Exit;
	}

	// Make sure it's not marked for deletion and is not busy
	// Note: we do combined tests here which speeds things up when no error happens
	if (recordP->attributes & (dmRecAttrDelete | dmRecAttrBusy)) {
		if (recordP->attributes & dmRecAttrBusy) {
			err = dmErrRecordBusy;
		}
		else{
			err = dmErrRecordDeleted;
		}
		goto Exit;
	}

	
	// Set the busy bit
	recordP->attributes |= dmRecAttrBusy;

	// If it's pointer based, Get the MemHandle out of our MemHandle table
	if (openP->handleTableP) {
		recordH = (MemHandle)&(openP->handleTableP[index]);
	}
	else {
		//id = recordP->localChunkID;
		recordH = MemLocalIDToGlobal(recordP->localChunkID, openP->cardNo);
	}
	
Exit:
	// Restore write-protection
	MemSemaphoreRelease(true);

	// dmErrRecordDeleted is not to be shouted about
	ErrNonFatalDisplayIf(err != 0 && err != dmErrRecordDeleted, "Err Getting rec");
		
	PrvSetLastErr(err);
	return recordH;
}



/************************************************************
 *
 *  FUNCTION: DmQueryNextInCategory
 *
 *  DESCRIPTION: Returns a MemHandle to the next record in the specified 
 *    category for reading only (does not set the busy bit).
 *		If the record is ROM based (pointer accessed) this routine
 *		will make a fake MemHandle to it and store this MemHandle in the
 *		DmAccessType structure.
 *
 *  PARAMETERS: database access MemPtr, index, category
 *
 *  RETURNS: record MemHandle, index
 *
 *  CREATED: 3/6/96 
 *
 *  BY: Art Lamb
 *
 *	HISTORY:
 *		4/6/2000	ryw	use PrvSetLastErr to report errors
 *************************************************************/
MemHandle	DmQueryNextInCategory (DmOpenRef dbR, UInt16 * indexP, UInt16 category)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	UInt16			index;
	UInt16 			count;
	MemHandle			recordH = 0;
	UInt8				attrMask, attrCmp;
	RecordEntryPtr	recordP;
	DmOpenInfoPtr	openP;
	
	ECDmValidateDmOpenRef(dbP);
		
	openP = dbP->openP;

	PrvClearLastErr();

	count = 	openP->numRecords;

	// Form the mask for comparing the attributes of each record
	if (category == dmAllCategories) {
		attrMask =  dmRecAttrDelete;
		attrCmp = 0;
		}
	else {
		attrMask =  dmRecAttrCategoryMask | dmRecAttrDelete;
		attrCmp = category;
		}
		
	if (!(dbP->mode & dmModeShowSecret))
		attrMask |= dmRecAttrSecret;
		
		

	// Walk through till we find a record that has the right attributes
	index = *indexP;
	recordP = 0;
	for (; index < count; index++) {
		// Get MemPtr to next record
		recordP = PrvRecordEntryPtr(openP, index, recordP);

		if ((recordP->attributes & attrMask) == attrCmp) {
			// If it's pointer based, Get the MemHandle out of our MemHandle table
			if (openP->handleTableP) 
				recordH = (MemHandle)&(openP->handleTableP[index]);	
			else 
				recordH = MemLocalIDToGlobal(recordP->localChunkID, openP->cardNo);
				
			*indexP = index;
			return recordH;
			}
			
		}
		
	PrvSetLastErr(dmErrIndexOutOfRange);
	return 0;
}

/************************************************************
 *
 *  FUNCTION: DmPositionInCategory
 *
 *  DESCRIPTION: Returns a position of a record in the specified 
 *    category.  
 *
 *  PARAMETERS: database access MemPtr, index, category
 *
 *  RETURNS: position (zero-based)
 *
 *  CREATED: 3/13/96 
 *
 *  BY: Art Lamb
 *
 *	HISTORY:
 *		4/6/2000	ryw	use PrvSetLastErr to report errors
 *************************************************************/
UInt16	DmPositionInCategory (DmOpenRef dbR, UInt16 index, UInt16 category)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	UInt16			i;
	UInt16			pos = 0;
	UInt8				attrMask, attrCmp;
	RecordEntryPtr	recordP;
	DmOpenInfoPtr	openP;

	ECDmValidateDmOpenRef(dbP);

	PrvClearLastErr();
	
	openP = dbP->openP;
	
	// Check for parameter error
	if (index >= openP->numRecords) {
		PrvSetLastErr(dmErrIndexOutOfRange);
		ErrDisplay("Err Getting position");
		return 0;
		}
		
	// Form the mask for comparing the attributes of each record
	if (category == dmAllCategories) {
		attrMask =  dmRecAttrDelete;
		attrCmp = 0;
		}
	else {
		attrMask =  dmRecAttrCategoryMask | dmRecAttrDelete;
		attrCmp = category;
		}
		
	if (!(dbP->mode & dmModeShowSecret))
		attrMask |= dmRecAttrSecret;
		


	// Get the position within category
	recordP = 0;
	for (i=0; i<= index; i++) {
		recordP = PrvRecordEntryPtr(openP, i, recordP);
		if ((recordP->attributes & attrMask) == attrCmp)
			pos++;
		}
	
	if (pos) pos--;
	
	return (pos);
}


/************************************************************
 *
 *  FUNCTION: DmSeekRecordInCategory
 *
 *  DESCRIPTION: Return the index of the record at the offset from
 *		the passed record index.  The offset parameter indicates the 
 *		number of records to move forewards or backwards (negitive  
 *		value for backwards).
 *
 *  PARAMETERS: database access MemPtr, index, offset, direction, category
 *
 *              direction is dmSeekForward or dmSeekBackward
 *
 *  RETURNS: record index
 *
 *  CREATED: 3/14/96 
 *
 *  BY: Art Lamb
 *
 *	HISTORY:
 *		4/6/2000	ryw	use PrvSetLastErr to report errors
 *************************************************************/
Err	DmSeekRecordInCategory (DmOpenRef dbR, UInt16 * indexP, UInt16 offset,
	Int16 direction, UInt16 category)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	UInt16			index;
	UInt16			count;
	UInt8				attrMask, attrCmp;
	RecordEntryPtr	recordP;
	DmOpenInfoPtr	openP;

	ErrFatalDisplayIf ( (direction != dmSeekForward) && (direction != dmSeekBackward), 
		"Bad Param");

	ErrFatalDisplayIf ( (offset < 0), "Bad param"); 
	
	ECDmValidateDmOpenRef(dbP);
		
	PrvClearLastErr();

	openP = dbP->openP;

	index = *indexP;

	if (index >= openP->numRecords) {
		if (direction == dmSeekForward)	{ 
			PrvSetLastErr(dmErrIndexOutOfRange);
			return dmErrIndexOutOfRange;
			}
		else {
			index = openP->numRecords - 1;
			}
		}


	// Form the mask for comparing the attributes of each record
	if (category == dmAllCategories) {
		attrMask =  dmRecAttrDelete;
		attrCmp = 0;
		}
	else {
		attrMask =  dmRecAttrCategoryMask | dmRecAttrDelete;
		attrCmp = category;
		}
		
	if (!(dbP->mode & dmModeShowSecret))
		attrMask |= dmRecAttrSecret;
		


	// Moving forward?
	if (direction == dmSeekForward )
		count = openP->numRecords - index;
	else
		count = index + 1;
		
		
	// Loop through the records
	while(count--) {
		// get entry MemPtr
		recordP = PrvRecordEntryPtr(openP, index, 0);

		if ((recordP->attributes & attrMask) == attrCmp) {
			*indexP = index;
			if (offset == 0) return 0;
			offset--;
			}
			
		index += direction;
		}

	// ErrNonFatalDisplayIf(appInfoP->dmLastErr, "Err seeking rec"); // this can never happen (ryw)
	PrvSetLastErr(dmErrSeekFailed);
	return dmErrSeekFailed;
}

/************************************************************
 *
 *  FUNCTION: DmMoveCategory
 *
 *  DESCRIPTION: Move all record in a category to another category.
 *
 *		If the "dirty" argument is TRUE, the moved records will be marked dirty.
 *
 *  PARAMETERS: database access MemPtr, from category, to category, dirty boolean
 *
 *  RETURNS: nothing
 *
 *  CREATED: 3/17/96 
 *
 *  BY: Art Lamb
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	11/27/95	Added the "dirty" argument.
 *
 *************************************************************/
Err	DmMoveCategory (DmOpenRef dbR, UInt16 toCategory, UInt16 fromCategory, Boolean dirty)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	UInt16 			index;
	UInt16 			count;
	UInt8 			attr;
	DmOpenInfoPtr	openP;
	Err				err=0;
	RecordEntryPtr	recordP=0;
	
	
	// Enable writes
	MemSemaphoreReserve(true);
		
	ECDmValidateDmOpenRef(dbP);
		
	openP = dbP->openP;

	// Check for errors
	if (!openP->writeAccess)
		{err = dmErrReadOnly; goto Exit;}
		
	count = openP->numRecords;
	for (index = 0; index < count; index++)
		{
		recordP = PrvRecordEntryPtr(openP, index, recordP);
		
		// Check if the record's category matches the from category
		// and that the record is not marked for deletion.
		attr = recordP->attributes;
		if (((attr & dmRecAttrCategoryMask) == fromCategory) && 
			 (!(attr & dmRecAttrDelete))) {
			// Change the category of a record.
			attr &= ~dmRecAttrCategoryMask;
			attr |= toCategory;
			
			// Mark the record dirty so the PC moves it's record.  We could have
			// had the PC mark it's records dirty instead of the device, but it's
			// faster for the device to go to the next record and if dirty make the
			// PC search for it's copy rather than have the PC make the device
			// search for the matching record ID.  ### THIS IS NOT ENTIRELY
			// ACCURATE: 1. We're changing the record's category and so should mark it dirty.
			// 2. HotSync Manager on the PC gets to find out about this change by
			// having the dirty flag set for the record on Pilot.
			if ( dirty )
				attr |= dmRecAttrDirty;
			
			recordP->attributes = attr;
			}
		}
		
	// Bump the mod number
	openP->hdrP->modificationNumber++;

Exit:
	// Display err
	ErrNonFatalDisplayIf(err, "Err moving category");
	
	// Restore write-protection
	MemSemaphoreRelease(true);
		
	return err;
	
}


/************************************************************
 *
 *  FUNCTION: DmDeleteCategory
 *
 *  DESCRIPTION: Delete all records in a category.  The category
 *  name is not changed.
 *
 *  PARAMETERS: dbR - database access MemPtr
 *					 categoryNum - category of records to delete
 *
 *  RETURNS: 0 if there is no error, an error code otherwise
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	4/18/96	Initial version
 *			vmk	11/18/97	Mark the record dirty
 *
 *************************************************************/
Err	DmDeleteCategory (DmOpenRef dbR, UInt16 categoryNum)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	UInt16 			index;
	UInt16 			count;
	UInt8 			attr;
	DmOpenInfoPtr	openP;
	Err				err=0;
	RecordEntryPtr	recordP=0;
	LocalID			id;
	MemHandle			h;
	
	
	// Enable writes
	MemSemaphoreReserve(true);
		
	ECDmValidateDmOpenRef(dbP);
		
	openP = dbP->openP;

	// Check for errors
	if (!openP->writeAccess)
		{err = dmErrReadOnly; goto Exit;}
		
	count = openP->numRecords;
	for (index = 0; index < count; index++)
		{
		recordP = PrvRecordEntryPtr(openP, index, recordP);
		
		// Check if the record's category matches the from category
		// and that the record is not marked for deletion.
		attr = recordP->attributes;
		if (((attr & dmRecAttrCategoryMask) == categoryNum) && 
			 (!(attr & dmRecAttrDelete))) 
			{
			// Set the delete bit
			recordP->attributes |= (dmRecAttrDelete | dmRecAttrDirty);
		
			// Now, delete the chunk and set the chunkID to nil
			id = recordP->localChunkID;
			if (id) {
				h = MemLocalIDToGlobal(id, openP->cardNo);
				err =  MemHandleFree(h);
				}
			
			recordP->localChunkID = 0;
			}
		}
		
	// Bump the mod number
	openP->hdrP->modificationNumber++;

Exit:
	// Display err
	ErrNonFatalDisplayIf(err, "Err deleting category");
	
	// Restore write-protection
	MemSemaphoreRelease(true);
		
	return err;
	
}


/************************************************************
 *
 *  FUNCTION: DmResizeRecord
 *
 *  DESCRIPTION: Resizes a record and returns new MemHandle. This
 *		routine will automatically allocate the record in another heap
 *		if the current heap is too full.
 *
 *  PARAMETERS: resource MemPtr, new size
 *
 *  RETURNS: Pointer to resized resource, or 0 if error
 *
 *  CREATED: 11/22/94 
 *
 *  BY: Ron Marianetti
 *
 *	HISTORY:
 *		4/6/2000	ryw	set last err to zero, use PrvSetLastErr
 *************************************************************/
MemHandle DmResizeRecord(DmOpenRef dbR, UInt16 index, UInt32 newSize)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	MemHandle			newH=0, oldH;
	UInt16			* srcP=0;
	UInt16			* dstP=0;
	Err				err;
	UInt32			count;
	LocalID			id;
	DmOpenInfoPtr	openP;
	RecordEntryPtr	recordP;


	// Enable writes
	MemSemaphoreReserve(true);
		
	ECDmValidateDmOpenRef(dbP);
		
	openP = dbP->openP;

	// Check for errors
	if (!openP->writeAccess)
		{err = dmErrReadOnly; goto Exit;}

	// Check the type of the database
	ErrFatalDisplayIf(openP->resDB, "Not Record DB");
	
	
	// Get the record entry MemPtr
	recordP = PrvRecordEntryPtr(openP, index, 0);
	if (!recordP) { err = dmErrIndexOutOfRange; goto Exit;}
	
	// Bump the mod number
	openP->hdrP->modificationNumber++;

	// Get the ID
	id = recordP->localChunkID;
	oldH = MemLocalIDToGlobal(id, openP->cardNo);

	// Resize it
	err = MemHandleResize(oldH, newSize);

	// If we couldn't resize it because it's locked, return error
	if (err == memErrChunkLocked) goto Exit;

	// If we couldn't resize it, try and reallocate it in another heap
	if (err) {
		newH = PrvDmNewHandle(dbP, newSize);
		if (!newH) goto ExitNoErrCheck;

		srcP = (UInt16 *)MemHandleLock(oldH);
		dstP = (UInt16 *)MemHandleLock(newH);
		count = MemHandleSize(oldH);
		
		MemMove(dstP, srcP, count);

		MemHandleFree(oldH);
		MemPtrUnlock(dstP);
		recordP->localChunkID = MemHandleToLocalID(newH);
		MemHandleSetOwner(newH, dmRecOwnerID);
		err = 0;
		}
	else
		newH = oldH;


Exit:
	ErrNonFatalDisplayIf(err, "Resizing record");
	
ExitNoErrCheck:
	// Restore write-protection
	MemSemaphoreRelease(true);

	PrvSetLastErr(err);
	if (err) {
		if (newH) MemHandleFree(newH);
		return 0;
		}

	return newH;
}




/************************************************************
 *
 *  FUNCTION: DmReleaseRecord
 *
 *  DESCRIPTION: Clears the busy bit on a record and sets the dirty
 *		bit if dirty is true.
 *
 *  PARAMETERS: database access MemPtr, record index, dirty boolean
 *
 *  RETURNS:
 *
 *  CREATED: 11/16/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err	DmReleaseRecord(DmOpenRef dbR, UInt16 index, Boolean dirty)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	DmOpenInfoPtr	openP;
	Err				err=0;
	RecordEntryPtr	recordP;
	
	// Enable writes
	MemSemaphoreReserve(true);
		
	ECDmValidateDmOpenRef(dbP);
		
	openP = dbP->openP;

	// Get the entry MemPtr
	recordP = PrvRecordEntryPtr(openP, index, 0);
	if (!recordP) {err = dmErrIndexOutOfRange; goto Exit;}
	
	// Clear the busy bit
	ErrNonFatalDisplayIf((recordP->attributes & dmRecAttrBusy) == 0, "Record isn't busy");
	recordP->attributes &= ~dmRecAttrBusy;

	// Set the dirty bit
	if (dirty)
		recordP->attributes |= dmRecAttrDirty;


	// Bump the mod number of the database
	if (dirty) 
		openP->hdrP->modificationNumber++;

Exit:
	// Display message
	ErrNonFatalDisplayIf(err, "Err Releasing rec");
	
	// Restore write-protection
	MemSemaphoreRelease(true);
		
	return err;
}




/************************************************************
 *
 *  FUNCTION: DmFindRecordByHandle
 *
 *  DESCRIPTION: Finds a record by by MemHandle if it is non-nil.
 *
 *  PARAMETERS: database access MemPtr, record MemHandle
 *
 *  RETURNS: Index of resource, or -1 if not found
 *
 *  CREATED: 11/27/95
 *
 *  BY: Art Lamb
 *
 *************************************************************/
static UInt16	DmFindRecordByHandle(DmOpenRef dbR, MemHandle findRecH)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	UInt16			numRecords;
	UInt16			i;
	RecordEntryPtr	recordP=0;
	LocalID			id;
	MemHandle		recH;
	DmOpenInfoPtr	openP;
	
	ECDmValidateDmOpenRef(dbP);
		
	openP = dbP->openP;

	// Cache the number of resources
	numRecords = openP->numRecords;

	// Search by MemHandle
	for (i=0; i<numRecords; i++) {
		recordP = PrvRecordEntryPtr(openP, i, recordP);
		id = recordP->localChunkID;
		if (! id) continue;
		
		// If it's pointer based, Get the MemHandle out of our MemHandle table
		if (openP->handleTableP) 
			recH = (MemHandle)&(openP->handleTableP[i]);
			
		else 
			recH = MemLocalIDToGlobal(id, openP->cardNo);
		
		if (recH == findRecH) break;
		}


	// Return result
	if (i < numRecords)
		return i;
	else
		return dmInvalidRecIndex;
}




/************************************************************
 *
 *  FUNCTION: DmSearchRecord
 *
 *  DESCRIPTION: Searches all open resource databases for a record
 *		with the MemHandle passed.
 *
 *  PARAMETERS: record MemHandle
 *
 *  RETURNS: Index of record and database access MemPtr
 *
 *  CREATED: 11/27/95
 *
 *  BY: Art Lamb
 *
 *************************************************************/
UInt16	DmSearchRecord(MemHandle recH, DmOpenRef* dbPP)
{
	DmAccessPtr		dbP;
	UInt16			index = dmInvalidRecIndex;
	SysAppInfoPtr	appInfoP;

	// Grab semaphore
	MemSemaphoreReserve(false);
	
	// Assume err
	*dbPP = 0;

	// Loop through each open database
	appInfoP = PrvGetAppInfo();
	dbP = (DmAccessPtr)appInfoP->dmAccessP;

	while(dbP) {

		// Search for next non-resource database
		while(dbP) {
			if (! dbP->openP->resDB) break;
			dbP = (DmAccessPtr)dbP->next;
			}
		if (!dbP) break;

		// Look for the resource in this database
		index = DmFindRecordByHandle(dbP, recH);
		if (index != dmInvalidRecIndex) {
			*dbPP = dbP;
			goto Exit;
			}

		dbP = dbP->next;
		}


	// Return result
Exit:
	MemSemaphoreRelease(false);
	return index;
}



/************************************************************
 *
 *  FUNCTION: PrvSetUniqueID
 *
 *  DESCRIPTION: Fills in the unique ID field of a record index entry
 *		with a unique ID
 *
 *  PARAMETERS: MemPtr to record entry structure
 *
 *  RETURNS: void
 *
 *  CREATED: 11/16/94 
 *
 *  BY: Ron Marianetti
 *		roger		11/12/96		moved code to prevent uniqueID's of zero
 *		roger		8/26/98		Fixed wraparound to skip the reserved range
 *
 *************************************************************/
void  PrvSetUniqueID(DatabaseHdrPtr hdrP, RecordEntryPtr recordP)
{
	UInt8 *	seedP;

	// Enable writes
	MemSemaphoreReserve(true);

	// Bump the seed to prevent a uniqueIDSeed of 0 which represents
	// an unassigned uniqueID.
	hdrP->uniqueIDSeed++;

	// Check for wrap around - remember - we are only using the low
	//	three bytes of uniqueIDSeed.
	if (hdrP->uniqueIDSeed & 0xFF000000)
		hdrP->uniqueIDSeed = (dmRecordIDReservedRange + 1) << 12;

	// Copy the unique ID seed from the header into the new record
	seedP = (UInt8 *)(&hdrP->uniqueIDSeed);
	recordP->uniqueID[0] = seedP[1];
	recordP->uniqueID[1] = seedP[2];
	recordP->uniqueID[2] = seedP[3];

	// Restore write-protection
	MemSemaphoreRelease(true);

}
	

/************************************************************
 *
 *  FUNCTION: DmWriteCheck
 *
 *  DESCRIPTION: Can be used to check the parameters of a write operation
 *    to a data storage chunk before actually performing the write.
 *
 *  PARAMETERS: 
 *		recordP			- locked pointer to 'recordH'.
 *    offset			- offset into record to start writing
 *		bytes				- number of bytes to write
 *
 *  RETURNS: 0 if no err
 *
 *  CALLED BY: DmWrite before updating a record
 *					Fields package before editing a field in a data storage heap.
 *
 *  CREATED: 5/8/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err	 DmWriteCheck(void * recordP, UInt32 offset, UInt32 bytes)
{
	memChunkHeaderTypeName*		chunkP;
	Err								err;
	MemHandle							h;
	UInt32							size;
	
	ErrNonFatalDisplayIf(!recordP, "Rec Ptr Null");
	
	// Get the chunk pointer for this record and assume err
	chunkP = (memChunkHeaderTypeName*)((UInt32)recordP - sizeof(memChunkHeaderTypeName));
	err = dmErrNotValidRecord;
	
	// Check the validity of the size
	if (chunkP->size == 0) goto Exit;
	if (chunkP->size & 0x01) goto Exit;
	
	// Check the validity of the flags
	if (memChunkFlags(chunkP))  goto Exit;  //DEBUG!!!!!- test this!!
	
	// Check the validity of the hOffset field
	if (!chunkP->hOffset) goto Exit;
	h = (MemHandle)( (UInt8 *)chunkP - ((Int32)chunkP->hOffset * 2) );
	if ((UInt32)(*(void**)h)  !=  (UInt32)recordP) goto Exit;
	
	// Check the bounds of the write operation
	size = chunkP->size - (chunkP->sizeAdj) - sizeof(MemChunkHeaderType);
	if (offset+bytes > size) {
		err = dmErrWriteOutOfBounds;
		goto Exit;
		}

	// Must be OK....
	err = 0;
	
Exit:
// SCL 7/27/98: DmWriteCheck now returns error to calling routine.
// DmWrite and DmSet will catch any errors with ErrFatalDisplayIf().
//	ErrFatalDisplayIf(err, "DmWriteCheck failed");
	return err;
}



/************************************************************
 *
 *  FUNCTION: DmStrCopy
 *
 *  DESCRIPTION: Must be used to write to Data Manager records
 *    because the Data Storage area is write-protected. This routine
 *    will check the validity of the chunk pointer for the record
 *    and make sure that the write will not exceed the chunk bounds.
 *
 *  PARAMETERS: 
 *		recordP			- pointer to Data Record (chunk pointer)
 *		offset			- offset within record to start writing
 *		srcP				- pointer to 0 terminated string
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 5/4/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err	 DmStrCopy(void * recordP, UInt32 offset, const Char * const srcP)
{
	Int32				len;
	const Char *	p;
	Err				err;
	
	// Get the length of the string
	len = 1;
	p = srcP;
	while(*p++)
		len++;

	err = DmWrite(recordP, offset, srcP, len);
	
	ErrFatalDisplayIf(err, "DmStrCopy failed");
	return err;
}


/************************************************************
 *
 *  FUNCTION: DmWrite
 *
 *  DESCRIPTION: Must be used to write to Data Manager records
 *    because the Data Storage area is write-protected. This routine
 *    will check the validity of the chunk pointer for the record
 *    and make sure that the write will not exceed the chunk bounds.
 *
 *  PARAMETERS: 
 *		recordP			- Pointer to locked data record (chunk pointer)
 *		offset			- offset within record to start writing
 *		srcP				- pointer to data to copy into record
 *		bytes				- number of bytes to write
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 5/4/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err	 DmWrite(void * recordP, UInt32 offset, const void * srcP, UInt32 bytes)
{
	Err					err;
	
	
	// Prepare for writing
	MemSemaphoreReserve(true);
	
	// Check the validity
	err = DmWriteCheck(recordP, offset, bytes);
	if (err) {
		ErrFatalDisplay("DmWrite: DmWriteCheck failed");
		//<chg 7-29-98 RM>, correct errcode is in 'err', not in DmGetLastErr()
		// err = DmGetLastErr();
		goto Exit;
		}
	else err = 0;
	
	// do the write....
	MemMove(((UInt8 *)recordP)+offset, srcP, bytes);
	
Exit:
	MemSemaphoreRelease(true);
	return err;
}




/************************************************************
 *
 *  FUNCTION: DmSet
 *
 *  DESCRIPTION: Must be used to write to Data Manager records
 *    because the Data Storage area is write-protected. This routine
 *    will check the validity of the chunk pointer for the record
 *    and make sure that the write will not exceed the chunk bounds.
 *
 *  PARAMETERS: 
 *		recordP			- Pointer to locked data record (chunk pointer)
 *		offset			- offset within record to start writing
 *		bytes				- number of bytes to write
 *    value				- byte value to write
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 5/4/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err	 DmSet(void * recordP, UInt32 offset, UInt32 bytes, UInt8 value)
{
	Err					err;
	
	
	// Prepare for writing
	MemSemaphoreReserve(true);
	
	// Check the validity
	err = DmWriteCheck(recordP, offset, bytes);
	if (err) {
		ErrFatalDisplay("DmSet: DmWriteCheck failed");
		err = DmGetLastErr();
		goto Exit;
		}
	else err = 0;
	
	// do the set....
	MemSet(((UInt8 *)recordP)+offset, bytes, value);
	
Exit:
	MemSemaphoreRelease(true);
	return err;
}



/************************************************************
 *
 *  FUNCTION: PrvDmNewHandle 
 *
 *  DESCRIPTION: Allocates a new chunk in the same heap or card
 *		as a database header
 *
 *  PARAMETERS:	dbR		--	database reference(if not null, allocation
 *										will be made from the card containing
 *										the database header; otherwise, allocation will
 *										be made from any card with sufficient free space)
 *										
 *						size		--	desired size
 *
 *  RETURNS: chunk MemHandle of new chunk
 *
 *  CREATED: 11/21/94 
 *
 *  BY: Ron Marianetti
 *
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	9/10/96	Enable allocation given null database reference
 *			vmk	12/22/97	Assert zero size allocations and return immediately
 *			vmk	12/22/97	Optimize by suppressing unnecessary comapctions and
 *								free space computations
 *
 *************************************************************/
MemHandle	 PrvDmNewHandle(DmOpenRef dbR, UInt32 size)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	UInt16			numHeaps, i, heapID;
	MemHandle		h = 0;
	UInt16			cardNo;
	UInt16			numStorageHeaps;
	Err				err;


	if ( size == 0 )
		return 0;
		

	// If passed a non-null database reference, allocate the chunk on the same
	// card as the database header
	if ( dbP ) {
		cardNo = dbP->openP->cardNo;
		}
	else {
		// We were passed a null database reference -- assume card number 0
		cardNo = 0;
		}

	// As a last resort, try and allocate in any heap
	numHeaps = MemNumRAMHeaps(cardNo);
	numStorageHeaps = 0;
	for (i=0; i < numHeaps; i++) {
		heapID = MemHeapID(cardNo, i);
		if (MemHeapDynamic(heapID))
			continue;
			
		numStorageHeaps++;											// count storage heaps
		
		h = MemChunkNew(heapID, size, dmOrphanOwnerID);
		if (h)
			goto Exit;
		}


	// If we still failed, try and re-arrange chunks in the heaps until
	//  a free chunk big enough gets formed
	if ( numStorageHeaps > 1 ) {	// this only makes sense if there is more than one storage heap
		err = PrvMergeFree(cardNo, size);
		if (err)
			goto Exit;
		
		for (i=0; i<numHeaps; i++) {
			heapID = MemHeapID(cardNo, i);
			if (MemHeapDynamic(heapID))
				continue;

			h = MemChunkNew(heapID, size, dmOrphanOwnerID);
			if (h)
				goto Exit;
			}
		}

Exit:
	return h;
}



/************************************************************
 *
 *  FUNCTION: DmNewHandle 
 *
 *  DESCRIPTION: Wrapper for PrvDmNewHandle that deals correctly with dmLastErr
 *				(not done in prvDmNewHandle to avoid messing with other error handling on 
 *				DataMgr-internal calls)
 *
 *			Name	Date		Description
 *			----	----		-----------
 *			jaq	9/10/99	created
 *			ryw	4/6/2000	use PrvSetLastErr to report errors
 *
 *************************************************************/
MemHandle	 DmNewHandle(DmOpenRef dbR, UInt32 size)
{
	MemHandle retval;
	Err err = 0;
	
	retval = PrvDmNewHandle(dbR, size);
	if (!retval) {
		err = size == 0 ? dmErrInvalidParam : dmErrMemError ;
	}

	PrvSetLastErr(err);
	return retval;
}

/************************************************************
 *
 *  FUNCTION: DmGetResource
 *
 *  DESCRIPTION: Returns a MemHandle to the resource. Searches
 *		all open resource databases for the resource.
 *
 *  PARAMETERS: resource type, resource id
 *
 *  RETURNS: MemHandle to resource
 *
 *  CREATED: 11/22/94 
 *
 *  BY: Ron Marianetti
 *
 *		4/6/2000	ryw	use PrvSetLastErr to report errors
 *************************************************************/
MemHandle	DmGetResource(UInt32 type, UInt16 id)
{
	UInt16				index;
	DmAccessPtr		dbP;

	PrvClearLastErr();
		
	// First, search for the resource by type and id
	index = DmSearchResource(type, id, 0, &dbP);
	if (index == dmInvalidRecIndex) {
		PrvSetLastErr(dmErrResourceNotFound);
		return 0;
		}

	// Now, Get the MemHandle
	return DmGetResourceIndex(dbP, index);
}



/************************************************************
 *
 *	FUNCTION:		DmGet1Resource
 *
 *	DESCRIPTION:	Returns a MemHandle to the resource. Searches
 *						only the top (most recently opened) resource
 *						file.
 *
 *	PARAMETERS:		resource type, resource id
 *
 *	RETURNS:			MemHandle to resource
 *
 *	HISTORY:
 *		11/22/99	ron	Created by Ron Marianetti.
 *		06/29/99	kwk	Check base if not found in overlay.
 *		4/6/2000	ryw	use PrvSetLastErr to report errors
 *
 *************************************************************/
MemHandle DmGet1Resource(UInt32 type, UInt16 id)
{
	UInt16			index;
	DmAccessPtr		dbP;

	PrvClearLastErr();

	// First, search for the resource
	dbP = DmNextOpenResDatabase(0);
	if (!dbP) {
		PrvSetLastErr(dmErrNoOpenDatabase);
		return 0;
		}
	
	index = DmFindResource(dbP, type, id, 0);
	
	// If we didn't find the resource, and we were searching in
	// an overlay, check the base too.
	if ((index == dmInvalidRecIndex) && (dbP->openType == openTypeOverlay)) {
		dbP = DmNextOpenResDatabase(dbP);
		ErrNonFatalDisplayIf((dbP == NULL) || (dbP->openType != openTypeBase),
			"Overlay without a base");
		index = DmFindResource(dbP, type, id, 0);
		}
		
	if (index == dmInvalidRecIndex) {
		PrvSetLastErr(dmErrResourceNotFound);
		return 0;
		}
	
	return DmGetResourceIndex(dbP, index);	
}



/************************************************************
 *
 *  FUNCTION: DmResizeResource
 *
 *  DESCRIPTION: Unlocks a resource, resizes it,
 *		locks it again, and returns new pointer
 *
 *  PARAMETERS: resource MemPtr, new size
 *
 *  RETURNS: Pointer to resized resource, or 0 if error
 *
 *  CREATED: 11/22/94 
 *
 *  BY: Ron Marianetti
 *
 *	HISTORY:
 *		4/6/2000	ryw	use PrvSetLastErr to report errors
 *************************************************************/
MemHandle DmResizeResource(MemHandle resourceH, UInt32 newSize)
{
	UInt16			index;
	DmAccessPtr		dbP;
	MemHandle		newH=0;
	UInt16 *			srcP=0;
	UInt16 * 		dstP=0;
	Err				err = 0;
	UInt32			count;
	DmOpenInfoPtr	openP;

	// Enable writes
	MemSemaphoreReserve(true);

	// Get the database access MemPtr and index of this resource
	index = DmSearchResource(0, 0, resourceH, &dbP);
	if (index == dmInvalidRecIndex) {
		err = dmErrCantFind;
		goto Exit;
		}

	// Get the open references
	openP = dbP->openP;
	
	// Check for errors
	if (!openP->writeAccess) {
		err = dmErrReadOnly; 
		goto Exit;
		}
		

	// Check the type of the database
	ErrFatalDisplayIf(!openP->resDB, "Not Resource DB");
	
	
	// Check the index
	if (index >= openP->numRecords) { 
		err = dmErrIndexOutOfRange;
		goto Exit;
		}


	// Resize it
	err = MemHandleResize(resourceH, newSize);

	// If we couldn't resize it because it's locked, return error
	if (err == memErrChunkLocked) {
		goto Exit;
		}

	// If we couldn't resize it, try and reallocate it in another heap
	if (err) {
		RsrcEntryPtr	resourceP;
		
		newH = PrvDmNewHandle(dbP, newSize);
		if (!newH) goto ExitNoErrCheck;

		srcP = (UInt16 *)MemHandleLock(resourceH);
		dstP = (UInt16 *)MemHandleLock(newH);
		count = MemHandleSize(resourceH);
		
		MemMove(dstP, srcP, count);

		MemHandleFree(resourceH);
		MemPtrUnlock(dstP);
		resourceP = PrvRsrcEntryPtr(openP, index, 0);
		resourceP->localChunkID = MemHandleToLocalID(newH);
		resourceH = newH;
		err = 0;
		}
		

	// Bump the mod number
	openP->hdrP->modificationNumber++;


Exit:
	ErrNonFatalDisplayIf(err, "Err resizing rsrc");
	
ExitNoErrCheck:
	// Restore write-protection
	MemSemaphoreRelease(true);

	PrvSetLastErr(err);
	if (err) {
		if (newH) MemHandleFree(newH);
		return 0;
		}

	return resourceH;
}



/************************************************************
 *
 *  FUNCTION: DmReleaseResource
 *
 *  DESCRIPTION:	Does nothing on non-ERROR_CHECK_LEVEL builds;
 *						on ERROR_CHECK_LEVEL it validates the passed-in
 *						resource MemHandle.
 *
 *  PARAMETERS: resource MemHandle
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 11/22/94 
 *
 *  BY: Ron Marianetti
 *
 *  REVISION HISTORY
 *		9/30/99	jmp	Eliminated all of the error-check code that
 *							was added here, and just call an inocuous
 *							Memory Manager routine that happens to call
 *							PrvHandleCheck(), which does everything that
 *							was being done here anyway.
 *
 *************************************************************/
Err	DmReleaseResource(MemHandle resourceH)
{
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL

	// Just call MemHandleSize() to save space.  MemHandleSize()
	// calls PrvHandleCheck() which does all of the error checking
	// we wanted to do here.
	//
	MemHandleSize(resourceH);

#endif

	return 0;
}

/************************************************************
 *
 *  FUNCTION: DmNextOpenResDatabase
 *
 *  DESCRIPTION: Returns the DmOpenRef to the top open resource
 *		Database.
 *
 *  PARAMETERS: void
 *
 *  RETURNS: DmOpenRef
 *
 *  CREATED: 11/22/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
DmOpenRef	DmNextOpenResDatabase(DmOpenRef dbR)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	SysAppInfoPtr	appInfoP;
	
	
	// Get semaphore
	MemSemaphoreReserve(false);
	
	if (!dbP)  {
		appInfoP = PrvGetAppInfo();
		dbP = (DmAccessPtr)appInfoP->dmAccessP;
		}
	else {
		ECDmValidateDmOpenRef(dbP);
		dbP = dbP->next;
		}
		
		
	while(dbP) {
		if (dbP->openP->resDB) break;
		dbP = dbP->next;
		}


	// Release semaphore
	MemSemaphoreRelease(false);
	
	return dbP;
}



/************************************************************
 *
 *  FUNCTION: FindResourceType
 *
 *  DESCRIPTION: Returns index of a resource given the type and
 *		the type index.
 *
 *  PARAMETERS: database access MemPtr, resource type, resource type index
 *
 *  RETURNS: resource index, or -1 if not found 
 *
 *  CREATED: 11/22/94 
 *
 *  BY: Ron Marianetti
 *
 *  REVISION HISTORY
 *		01/24/99	jmp	Cast "-1" as a type Err since, strictly speaking,
 *							"-1" can't be of type Err itself since Err is
 *							UInt16.  Not doing this was causing compares
 *							not to work right in the PPC simulator builds.
 *
 *************************************************************/
UInt16	DmFindResourceType(DmOpenRef dbR, UInt32 resType, UInt16 typeIndex)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	UInt16			numResources;
	UInt16			whichTypeIndex;
	UInt16			i;
	RsrcEntryPtr	resourceP=0;
	DmOpenInfoPtr	openP;

	// Check the DB pointer and the type of the database
	ECDmValidateDmOpenRef(dbP);
	ECDmValidateResourceDB(dbP);

	openP = dbP->openP;
	
	// Search for this type	
	numResources = openP->numRecords;
	whichTypeIndex = 0;
	for (i=0; i<numResources; i++) {
		resourceP = PrvRsrcEntryPtr(openP, i, resourceP);
		
		if (resourceP->type != resType) continue;
		if (typeIndex == whichTypeIndex) break;
		whichTypeIndex++;
		}
	// Return the index of the resource
	if (i < numResources)
		return i;
	else
		return dmInvalidRecIndex;
}



/************************************************************
 *
 *  FUNCTION: DmFindResource
 *
 *  DESCRIPTION: Finds a resource by type and ID, OR by handle if
 *		it is non-nil.
 *
 *  PARAMETERS: database access MemPtr, resource type, resouce id, 
 *		resource pointer (optional)
 *
 *  RETURNS: Index of resource, or -1 if not found
 *
 *  CREATED: 11/22/94 
 *
 *  BY: Ron Marianetti
 *
 *  REVISION HISTORY
 *		01/24/99	jmp	Cast "-1" as a type Err since, strictly speaking,
 *							"-1" can't be of type Err itself since Err is
 *							UInt16.  Not doing this was causing compares
 *							not to work right in the PPC simulator builds.
 *		03/31/00	RYW	optimized searching by getting pointers to each entry
 *							array and incrementing the pointers directly
 *
 *************************************************************/

UInt16	DmFindResource(DmOpenRef dbR, UInt32 resType, DmResID resID,
			MemHandle findResH)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	UInt16			numResources;
	RsrcEntryPtr	resourceP=0;
	LocalID			id;
	MemHandle		resH;
	DmOpenInfoPtr	openP;

	UInt16			index;
	UInt16 			whichArray;
	RsrcEntryPtr	arrayEnd;
	
	// Check the DB pointer and the type of the database.
	ECDmValidateDmOpenRef(dbP);
	ECDmValidateResourceDB(dbP);

	openP = dbP->openP;
	
	// Cache the number of resources
	numResources = openP->numRecords;

	// If handle not specified, search by type and id.
	if( !findResH ){
		// optimized linear search (ryw)
		index = 0;
		whichArray = 0;
		resourceP = arrayEnd = 0;
		while( true ){
			if( resourceP == arrayEnd ){
			   // get next array
			   if( ! PrvGetRsrcEntryArrayBounds(openP, whichArray++, &resourceP, &arrayEnd) ){
			      ErrNonFatalDisplayIf( index != numResources, "all resources were not searched" );
			      break;
			   	}
				}
			else{
				// check for match
				if( resourceP->type == resType && resourceP->id == resID ){ 
					break; 
					}
				// next entry
				++resourceP;
				++index;
				}
			}
		}

	// If handle specified, search by handle. 
	// this case isn't used much no optimization (ryw)
	else{
		for( index=0; index<numResources; index++ ){
			resourceP = PrvRsrcEntryPtr(openP, index, resourceP);
			id = resourceP->localChunkID;
			
			// If it's pointer based, get the handle out of our handle table.
			if( openP->handleTableP ){
				resH = (MemHandle)&(openP->handleTableP[index]);
				}	
			else{ 
				resH = MemLocalIDToGlobal(id, openP->cardNo);
				}
			
			if( resH == findResH ){
				break;
				}
			}
		}

	// Return result
	if( index < numResources ){
		return index;
		}
	else{
		return dmInvalidRecIndex;
		}
}




/************************************************************
 *
 *  FUNCTION: DmSearchResource
 *
 *  DESCRIPTION: Searches all open resource databases for a resource
 *		by type and id, OR by pointer
 *
 *  PARAMETERS: resource type, resource id, resource MemPtr (optional)
 *
 *  RETURNS: Index of resource and database access MemPtr
 *
 *  CREATED: 11/22/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt16	DmSearchResource(UInt32 resType, DmResID resID, MemHandle resH,
			DmOpenRef* dbPP)
{
	DmAccessPtr		dbP;
	UInt16			index = dmInvalidRecIndex;
	SysAppInfoPtr	appInfoP;

	// Grab semaphore
	MemSemaphoreReserve(false);
	
	// Assume err
	*dbPP = 0;

	// Loop through each open resource database
	appInfoP = PrvGetAppInfo();
	dbP = (DmAccessPtr)appInfoP->dmAccessP;

	while(dbP) {

		// Search for next resource database
		while(dbP) {
			if (dbP->openP->resDB) break;
			dbP = (DmAccessPtr)dbP->next;
			}
		if (!dbP) break;

		// Look for the resource in this database
		index = DmFindResource(dbP, resType, resID, resH);
		if (index != dmInvalidRecIndex) {
			*dbPP = dbP;
			goto Exit;
			}

		dbP = dbP->next;
		}


	// Return result
Exit:
	MemSemaphoreRelease(false);
	return index;
}






/************************************************************
 *
 *  FUNCTION: DmNumResources
 *
 *  DESCRIPTION: Returns the number of resources in a database
 *
 *  PARAMETERS: database access MemPtr
 *
 *  RETURNS: Number of resources
 *
 *  CREATED: 11/22/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt16	DmNumResources(DmOpenRef dbP)
{
	// Check the DB pointer and the type of the database
	ECDmValidateDmOpenRef((DmAccessPtr)dbP);
	ECDmValidateResourceDB((DmAccessPtr)dbP);

	return ((DmAccessPtr)dbP)->openP->numRecords;
}


/************************************************************
 *
 *  FUNCTION: DmResourceInfo
 *
 *  DESCRIPTION: Returns info out of the database header for a
 *		resource by index
 *
 *  PARAMETERS: database access MemPtr, index of resource,
 *		pointers to return variables
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 11/22/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err	DmResourceInfo(DmOpenRef dbR, UInt16 index,
			UInt32 *	resTypeP, UInt16 * resIDP,
			 LocalID* chunkLocalIDP)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	RsrcEntryPtr	resourceP;
	DmOpenInfoPtr	openP;

	// Check the DB pointer and the type of the database
	ECDmValidateDmOpenRef(dbP);
	ECDmValidateResourceDB(dbP);
		
	openP = dbP->openP;

	resourceP = PrvRsrcEntryPtr(openP, index, 0);
	if (!resourceP) return dmErrIndexOutOfRange;

	// return info
	if (resTypeP) 	*resTypeP = resourceP->type;
	if (resIDP) 	*resIDP = resourceP->id;
	if (chunkLocalIDP) 
		*chunkLocalIDP = resourceP->localChunkID;

	return 0;
}



/************************************************************
 *
 *  FUNCTION: DmSetResourceInfo
 *
 *  DESCRIPTION: Sets info in the database header for a
 *		resource by index
 *
 *  PARAMETERS: database access MemPtr, index of resource,
 *		pointers to new variables
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 11/22/94 
 *
 *  BY: Ron Marianetti
 *
 *		vmk	2/6/98	Added non-fatal checks for null dbR
 *************************************************************/
Err	DmSetResourceInfo(DmOpenRef dbR, UInt16 index,
			UInt32 *	resTypeP, UInt16 * resIDP)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	RsrcEntryPtr	resourceP;
	DmOpenInfoPtr	openP;
	Err				err=0;
	
	// Enable writes
	MemSemaphoreReserve(true);

	// Check the DB pointer and the type of the database
	ECDmValidateDmOpenRef(dbP);
	ECDmValidateResourceDB(dbP);
		
	openP = dbP->openP;

	// Make sure we have write access
	if (!(dbP->mode & dmModeWrite)) {err= dmErrReadOnly; goto Exit;}
	
	// Check boundary conditions
	if (index >= openP->numRecords) {err = dmErrIndexOutOfRange; goto Exit;}

	// Bump the mod number
	openP->hdrP->modificationNumber++;


	resourceP = PrvRsrcEntryPtr(openP, index, 0);

	// set info
	if (resTypeP)	 	resourceP->type = *resTypeP;
	if (resIDP) 		resourceP->id = *resIDP;


Exit:
	// Restore write-protection
	MemSemaphoreRelease(true);

	return err;
}




/************************************************************
 *
 *  FUNCTION: DmAttachResource
 *
 *  DESCRIPTION: Adds a chunk as a new resource to a database
 *
 *  PARAMETERS: database access MemPtr, chunk ID of new resource,
 *		new resource type and id
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 11/22/94 
 *
 *  BY: Ron Marianetti
 *
 *  REVISION HISTORY
 *		01/24/00	jmp	Although DmFindResource() returns "-1", the
 *							Err type is UInt16, so "-1" doesn't properly
 *							compare with "0xFFFF" under PPC builds.  Fixed
 *							this by casting -1 as type Err.
 *
 *************************************************************/
Err DmAttachResource(DmOpenRef dbR, MemHandle newH, UInt32 resType,
		DmResID resID)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	Err				err;
	RsrcEntryPtr	resourceP;
	DmOpenInfoPtr	openP;

	// Enable writes
	MemSemaphoreReserve(true);

	// Check the DB pointer and the type of the database
	ECDmValidateDmOpenRef(dbP);
	ECDmValidateResourceDB(dbP);
		
	openP = dbP->openP;

	// Make sure we have write access
	if (!(dbP->mode & dmModeWrite)) {err = dmErrReadOnly; goto Exit;}

	// Make sure the new resource is in the same card as the header
	if (MemHandleCardNo(newH) != openP->cardNo) 
		{err = dmErrRecordInWrongCard; goto Exit;}

	// limit the number of records in the DB
	if (openP->numRecords >= dmMaxRecordsInDB)
		{ err = dmErrIndexOutOfRange; goto Exit; }   // DOLATER - add a more specific error for this case

	// If this database is empty, make room for the first record
	if (openP->numRecords == 0) {
		UInt32		reqSize;
		
		reqSize = sizeof(DatabaseHdrType) + sizeof(RsrcEntryType);
		MemPtrUnlock(openP->hdrP);
		err = MemHandleResize(openP->hdrH, reqSize);
		openP->hdrP = MemHandleLock(openP->hdrH);
		if (err) goto ExitNoErrCheck;
		openP->hdrP->recordList.numRecords = 1;
		}
		
	// Else, Grow the resource entry table to add space for the new resource 
	else {
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
		//	First, check duplicate resID.
		err = DmFindResource(dbR, resType, resID, 0);
		if (err != dmInvalidRecIndex)
				ErrNonFatalDisplay("Duplicate resource");
#endif
		err = PrvShiftEntriesDown(dbR, openP->numRecords-1, 0, true);
		if (err) {
			if (err == dmErrMemError) goto ExitNoErrCheck;
			goto Exit;
			}
		}
	openP->numRecords++;
	
	// Initialize info
	resourceP = PrvRsrcEntryPtr(openP, openP->numRecords-1, 0);
	resourceP->type = resType;
	resourceP->id = 	resID;
	resourceP->localChunkID = MemHandleToLocalID(newH);
	
	// Set the owner of the chunk so that it's not an orphan anymore
	MemHandleSetOwner(newH, dmRecOwnerID);

	// Bump the mod number
	openP->hdrP->modificationNumber++;

Exit:
	ErrNonFatalDisplayIf(err, "Err adding rsrc");
	
ExitNoErrCheck:
	// Restore write protection
	MemSemaphoreRelease(true);

	return err;
}




/************************************************************
 *
 *  FUNCTION: DmDetachResource
 *
 *  DESCRIPTION: Removes the entry in the database header for
 *		a resource and returns the ChunkID of the resource.
 *
 *  PARAMETERS: database access MemPtr, resource index, MemPtr to return
 *		chunk ID
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 11/22/94 
 *
 *  BY: Ron Marianetti
 *
 *		vmk	2/6/98	Added non-fatal checks for null dbR
 *************************************************************/
Err	DmDetachResource(DmOpenRef dbR, UInt16 index, MemHandle* oldHP)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	LocalID			id;
	RsrcEntryPtr	resourceP;
	Err				err=0;
	DmOpenInfoPtr	openP;


	// Enable writes
	MemSemaphoreReserve(true);

	// Check the DB pointer and the type of the database
	ECDmValidateDmOpenRef(dbP);
	ECDmValidateResourceDB(dbP);
		
	openP = dbP->openP;

	// Make sure we have write access
	if (!(dbP->mode & dmModeWrite)) {err= dmErrReadOnly; goto Exit;}

	// Make sure the index is in range
	if (index >= openP->numRecords) {err = dmErrIndexOutOfRange; goto Exit;}

	// Get the chunkID of the resource we're removing
	resourceP = PrvRsrcEntryPtr(openP, index, 0);
	id = resourceP->localChunkID;
	if (id) 
		*oldHP = MemLocalIDToGlobal(id, openP->cardNo);
	else
		{
		*oldHP = 0;
		err = dmErrCorruptDatabase;
		goto Exit;
		}



	// Adjust the number of records and shift later ones up
	err = PrvShiftEntriesUp(dbR, index+1, 0, true);
	if (err) goto Exit;
	
	openP->numRecords--;
	
	// Bump the mod number
	openP->hdrP->modificationNumber++;


	
Exit:
	ErrNonFatalDisplayIf(err, "Err detaching rsrc");

	// Restore write-protection
	MemSemaphoreRelease(true);


	return err;
}




/************************************************************
 *
 *  FUNCTION: DmNewResource
 *
 *  DESCRIPTION: Creates a new chunk for a resource and adds
 *		an entry to the database header for the new resource
 *
 *  PARAMETERS: database access MemPtr, resource type, resource id,
 *		resource size
 *
 *  RETURNS: Pointer to new locked resource
 *
 *  CREATED: 11/22/94 
 *
 *  BY: Ron Marianetti
 *
 *	HISTORY:
 *		4/6/2000	ryw	set last err to zero, use PrvSetLastErr
 *************************************************************/
MemHandle	DmNewResource(DmOpenRef dbR, UInt32 resType, DmResID resID,
				UInt32 size)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	MemHandle			newH=0;
	Err				err;
	DmOpenInfoPtr	openP;


	// Check the DB pointer and the type of the database
	ECDmValidateDmOpenRef(dbP);
	ECDmValidateResourceDB(dbP);
		
	openP = dbP->openP;

	// Make sure we have write access
	if (!(dbP->mode & dmModeWrite)) {
		err = dmErrReadOnly;
		goto Exit;
		}

	if (!openP->resDB) {
		err = dmErrNotResourceDB; 
		goto Exit;
		}
		

	// First, try and allocate a new chunk of the desired size in the
	//  same card as the database header
	newH = PrvDmNewHandle(dbP, size);
	if (!newH) {
		err = dmErrMemError; 
		goto Exit; 
		}

	// Now, attach it to the database
	err = DmAttachResource(dbP, newH, resType, resID);

Exit:
	PrvSetLastErr(err);
	if (err) {
		if (newH) MemHandleFree(newH);
		newH = 0;
		}
	return newH;
}



/************************************************************
 *
 *  FUNCTION: DmRemoveResource
 *
 *  DESCRIPTION: Deletes a resource's chunk and removes its entry
 *		from the database header
 *
 *  PARAMETERS: database access MemPtr, index to resource
 *
 *  RETURNS:
 *
 *  CREATED: 11/22/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err DmRemoveResource(DmOpenRef dbR, UInt16 index)
{
	Err			err;
	MemHandle		h;
	DmAccessPtr		dbP = (DmAccessPtr)dbR;

	// Check the DB pointer and the type of the database
	ECDmValidateDmOpenRef(dbP);
	ECDmValidateResourceDB(dbP);
		
	// Make sure we have write access
	if (!(dbP->mode & dmModeWrite)) 
		return (dmErrReadOnly);

	// First, detach it
	err = DmDetachResource(dbP, index, &h);
	if (err) return err;

	// Now, delete the resource
	return MemHandleFree(h);
}



/************************************************************
 *
 *  FUNCTION: DmGetResourceIndex
 *
 *  DESCRIPTION: Looks up a resource by index and returns MemHandle to it
 *
 *  PARAMETERS: database access MemPtr, index of resource
 *
 *  RETURNS: MemHandle to resource
 *
 *  CREATED: 11/22/94 
 *
 *  BY: Ron Marianetti
 *
 *		vmk	2/6/98	Added non-fatal checks for null dbR
 *		ryw	4/6/2000 set last err to zero, use PrvSetLastErr
 *************************************************************/
MemHandle	DmGetResourceIndex(DmOpenRef dbR, UInt16 index)
{
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	LocalID 			id;
	MemHandle		resH = 0;
	Err				err = 0;
	DmOpenInfoPtr	openP;
	RsrcEntryPtr	resourceP;

	// Check the DB pointer and the type of the database
	ECDmValidateDmOpenRef(dbP);
	ECDmValidateResourceDB(dbP);

	openP = dbP->openP;
		
	if (index >= openP->numRecords) {
		err = dmErrIndexOutOfRange;
		goto Exit;
		}


	// If it's pointer based, Get the MemHandle out of our MemHandle table
	if (openP->handleTableP) 
		resH = (MemHandle)&(openP->handleTableP[index]);
		
	else {
		resourceP = PrvRsrcEntryPtr(openP, index, 0);
		id = resourceP->localChunkID;
		resH = MemLocalIDToGlobal(id, openP->cardNo);
		}

Exit:	
	PrvSetLastErr(err);
	return resH;
}



/************************************************************
 *
 *  FUNCTION:		PrvCloseDatabase
 *
 *  DESCRIPTION:	Closes a database & returns extra info
 *
 *  PARAMETERS:	Database access pointer
 *						MemPtr to previous opened database, or NULL
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 11/16/94 
 *
 *  BY: Ron Marianetti
 *
 *  MODIFICATIONS:
 *		07/02/96	RM		Update modification date on close.
 *		10/31/97	vmk	Don't update the modification date if mod num hasn't changed
 *		11/05/98 BS		Re-enable check lock or busy leaks in databases
 *		06/29/99	kwk	Renamed as PrvCloseDatabase, with some new return value params.
 *		09/30/99 LFe	Clear dbP->openP & openP->handleTableP after they have been freed
 *
 *************************************************************/
static Err PrvCloseDatabase(DmOpenRef dbR, DmOpenRef* prevDB)
{
	DatabaseHdrPtr	hdrP;
	DmOpenInfoPtr	prevP = 0, openP, curP;
	Err				err = 0;
	SysAppInfoPtr	appInfoP;
	DmAccessPtr		curAccessP, prevAccessP;
	DmAccessPtr*	accessHeadP;
	DmAccessPtr		dbP = (DmAccessPtr)dbR;
	RecordListPtr	listP;
	LocalID			localID, recycleDBID = 0;
	UInt16 			entrySize, recycleCardNo;
		
	
	// Grab semaphore
	MemSemaphoreReserve(true);
	
	// Check params
	ECDmValidateDmOpenRef(dbP);
	
	// Get the MemPtr to the openInfo structure
	openP = dbP->openP;

	// Decrement the open count
	openP->openCount--;
	
	
	// Update the modification date accordingly <ROM2.0>
	// and only if mod number changed <ROM3.0>
	if ( (dbP->mode & dmModeWrite) && openP->hdrP->modificationNumber != dbP->savedModNum )
		openP->hdrP->modificationDate = TimGetSeconds();
		
	//----------------------------------------------------------------- 
	// If no one else has it open, release the openInfo structure 
	//----------------------------------------------------------------- 
	if (openP->openCount == 0) {
		
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	// Make sure this database is not protected
		if (!PrvDatabaseIsProtected(openP->cardNo, openP->hdrID)) {
			// Error checking code to detect lock or busy leaks in databases.
			UInt8 highest;
			UInt32 count;
			UInt32 busy;
			
			DmGetDatabaseLockState(dbR, &highest, &count, &busy);
			ErrNonFatalDisplayIf(highest > 0, "Records left locked in closed unprotected DB");
			ErrNonFatalDisplayIf(busy > 0, "Records left busy in closed unprotected DB");
			}
#endif

		curP = (DmOpenInfoPtr)GDmOpenList;
		prevP  = 0;
		while(curP) {
			if (curP == openP) break;
			prevP = curP;
			curP = (DmOpenInfoPtr)curP->next;
			}
	
		// If not found, error
		if (!curP) {err = dmErrInvalidParam; goto Exit;}

		// Unlink it
		if (prevP)
			prevP->next = openP->next;
		else
			GDmOpenList = openP->next;
	
			

		//;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
		// Go through and unlock all the Record lists.
		//;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
		listP = &openP->hdrP->recordList;
		localID = listP->nextRecordListID;
		if (openP->hdrP->attributes & dmHdrAttrResDB)
			entrySize = sizeof(RsrcEntryType);
		else
			entrySize = sizeof(RecordEntryType);

		while(localID) {
			// Get MemPtr to this list
			listP = MemLocalIDToGlobal(localID, openP->cardNo);
		
			// If it's not ROM based, we actually get a MemHandle from MemLocalIDToGlobal
			if (!openP->handleTableP) 
				listP = (RecordListPtr)MemDeref((MemHandle)listP);

			// Get ID of next list
			localID = listP->nextRecordListID;
			
			// see if we need to shrink the list chunk
			if (sizeof(RecordListType) + (listP->numRecords * entrySize) > MemPtrSize(listP))
				MemPtrResize(listP, sizeof(RecordListType) + (listP->numRecords * entrySize));

			// Unlock this list
			MemPtrUnlock(listP);
			}
		
		

		//;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
		// Dispose of the MemHandle table, if it's ROM based
		//;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
		if (openP->handleTableP) {
			MemPtrFree(openP->handleTableP);
			// Clear the field in db struct
			openP->handleTableP = NULL;
			}
			
		// If RAM based, update the modification date, clear the open bit, 
		// check the recyclable but,  and unlock the header
		else {
			hdrP = MemHandleLock(openP->hdrH);
			
			// Clear the Open attribute bit
			hdrP->attributes &= ~dmHdrAttrOpen;
	
			// see if we need to shrink the header chunk
			if (sizeof(DatabaseHdrType) + (listP->numRecords * entrySize) > MemPtrSize(hdrP))
				MemPtrResize(listP, sizeof(DatabaseHdrType) + (listP->numRecords * entrySize));
			
			// Check the recyclable bit: should the database be deleted when we're finished closing it?
			if((hdrP->attributes) & dmHdrAttrRecyclable)
			{
				// clear recyclable bit to prevent infinite recursion when we call DmDeleteDatabase...
				hdrP->attributes &= ~dmHdrAttrRecyclable;
				
				recycleCardNo = openP->cardNo;
				recycleDBID = openP->hdrID;
			}
			
			// Unlock the database header
			MemPtrResetLock(hdrP);
			}
		
		
		// Release the memory used by the openInfo MemPtr
		MemPtrFree(openP);
		
		// Clear the field in db struct
		dbP->openP = NULL;
		}
		
		
	//----------------------------------------------------------------- 
	// If it's still open by someone else, just update the status
	//----------------------------------------------------------------- 
	else {
		if (dbP->mode & dmModeWrite) openP->writeAccess = 0;
		}
		
	
	// Unlink and Release the dmAccess structure
	appInfoP = PrvGetAppInfo();
	// <RM> 1-14-98, Use the current action code's appInfo if available
	accessHeadP = (DmAccessPtr*)&appInfoP->dmAccessP;
		
	curAccessP = *accessHeadP;
	prevAccessP  = 0;
	while(curAccessP) {
		if (curAccessP == dbP) break;
		prevAccessP = curAccessP;
		curAccessP = curAccessP->next;
		}

	// If not found, error
	if (!curAccessP) {err = dmErrInvalidParam; goto Exit;}

	// Unlink it
	if (prevAccessP)
		prevAccessP->next = dbP->next;
	else
		*accessHeadP = dbP->next;
	
	// Free it
	MemPtrFree(dbP);
	
	// Return previous DB for caller, if they want it.
	if (prevDB != NULL) {
		*prevDB = prevAccessP;
		}
	
	// if the database is recyclable, then attempt to delete it:
	if(recycleDBID != 0)
	{
		DmDeleteDatabase(recycleCardNo, recycleDBID);
	}
	
Exit:
	MemSemaphoreRelease(true);
	return err;
}


/************************************************************
 *
 *  FUNCTION: PrvMoveOpenDBContext
 *
 *  DESCRIPTION: Used by SysAppStartup to move the DmAccess pointer of
 *   a database out of the current context and into another. For example,
 *		an open database can be moved from the app's open database list to the
 *		System list or vice versa.
 *
 *		As a special case, passing 0 for dstHeadP specifies that the database
 *		should be moved to the System Context. This is a special case because
 *		of the way DmAccessPtrs are linked together, the system list always
 *		appears tacked on to the end of the application list so this routine
 *		must not only update the system list but also update the link in
 *		the application's list to the system list. Got That????
 *
 *  PARAMETERS: 
 *		dstHeadP - Head pointer of destination list - or 0 for System context
 *		dbP	- DmAccessPtr of database we're moving out of ucfdrent list.
 *		
 *
 *  RETURNS: 0 if no err
 *
 *	 CALLED BY: SysAppLaunch in order to move an application database that it
 *		is launching out of the System list and into the application's list.
 *
 *  CREATED: 5/16/96
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
static Err PrvMoveOpenDBContext(DmAccessPtr* dstHeadP, DmAccessPtr dbP, DmAccessPtr* prevDB)
{
	DmAccessPtr		curAccessP, prevAccessP;
	DmAccessPtr*	oldHeadP;
	Err				err;
	SysAppInfoPtr	appInfoP;
	Boolean			moveToSystem=false;
	DmAccessPtr*	linkP=0;

	// Grab semaphore
	MemSemaphoreReserve(false);
	
	
	// Get the head of the source context
	appInfoP = PrvGetAppInfo();
	oldHeadP = (DmAccessPtr*)(&appInfoP->dmAccessP);

	// If moving to the system context, get the MemPtr to the first System DmAccessType
	//  in dstHeaderP.
	if (!dstHeadP) {
		moveToSystem = true;
		dstHeadP = (DmAccessPtr*)&((SysAppInfoPtr)GSysAppInfoP)->dmAccessP;
		}

	//-------------------------------------------------------------
	// Unlink the access MemPtr from the current list
	//-------------------------------------------------------------
	curAccessP = *oldHeadP;
	prevAccessP  = 0;
	while(curAccessP) {
		if (curAccessP == dbP) break;
		prevAccessP = curAccessP;
		curAccessP = curAccessP->next;
		}

	// If not found, error
	if (!curAccessP) {err = dmErrInvalidParam; goto Exit;}

	// Unlink it
	if (prevAccessP)
		prevAccessP->next = dbP->next;
	else
		*oldHeadP = dbP->next;
	dbP->next = 0;
	
	if (prevDB != NULL) {
		*prevDB = prevAccessP;
		}
	
	//-------------------------------------------------------------
	// If we're moving it to the System Context, find the link in the app
	//    list that points to the start of the System list
	//-------------------------------------------------------------
	if (moveToSystem) {
		linkP = oldHeadP;
		
		// If there is an app list, look for the end
		if (*linkP) {
			while(*linkP) {
				if (*linkP == *dstHeadP) 
					break;
				linkP = (DmAccessPtr*)(*linkP);
				}
			if (!*linkP) {err = dmErrInvalidParam; goto Exit;}
			}
			
		// Else, we just use the head of the app list
		}
	
	
	//-------------------------------------------------------------
	// Link it into the new  list
	//-------------------------------------------------------------
	dbP->next = *dstHeadP;
	*dstHeadP = dbP;
	
	// If we moved to the system context, update the app's link to the system
	//  context
	if (moveToSystem)
		*linkP = dbP;
		
		
	err = 0;
	
		
Exit:
	ErrFatalDisplayIf(err, "Can't move open DB");
	
	MemSemaphoreRelease(false);
	return err;
			
}



/************************************************************
 *
 *  FUNCTION: PrvGetEntryArrayByIndex, private
 *
 *  DESCRIPTION:  returns a pointer to the ith entry array or
 *                null.  
 *
 *		It assumes that all record list chunks have been locked
 *		as a result of opening the database.
 *
 *  PARAMETERS:  
 *			openP 			-	DmOpenInfoPtr of open database
 *			arrayIndex 		-	zero based list index
 *
 *  RETURNS: pointer to the ith list, or null
 *
 *  CREATED: 3/31/00
 *
 *  BY: Russell Y. Webb
 *
 *************************************************************/

static RecordListPtr PrvGetEntryArrayByIndex(DmOpenInfoPtr openP, UInt16 arrayIndex)
{
	RecordListPtr result;
	
	// 0th list
	result = &openP->hdrP->recordList;

	// interate to the requested list
	while( arrayIndex-- ){

		// is there a next list?
		if( result->nextRecordListID == 0 ){
			return 0;
			}

		// get next list
		result = MemLocalIDToGlobal(result->nextRecordListID, openP->cardNo);

		// If it's not ROM based, we actually got a MemHandle from MemLocalIDToGlobal
		if( !openP->handleTableP ){ 
			result = (RecordListPtr)MemDeref((MemHandle)result);
			}
		}

	return result;
}


/************************************************************
 *
 *  FUNCTION: PrvGetRsrcEntryArrayBounds, private
 *
 *  DESCRIPTION:  Returns pointers to the beginning and end of 
 *                the ith entry array or nulls if that array does
 *						not exist.  
 *
 *		It assumes that all record list chunks have been locked
 *		as a result of opening the database.
 *
 *  PARAMETERS:  
 *			openP 			-	DmOpenInfoPtr of open database
 *			whichArray 		-	zero based array index
 *			arrayStart		-  pointer to first entry in array, or null
 *			arrayEnd			-  pointer to entry just after the last
 *									valid entry in the array, or null
 *
 *  RETURNS: true if whichArray was found, false otherwise
 *
 *  CREATED: 3/31/00
 *
 *  BY: Russell Y. Webb
 *
 *************************************************************/

static Boolean PrvGetRsrcEntryArrayBounds( DmOpenInfoPtr openP, UInt16 whichArray, 
               	RsrcEntryPtr *arrayStart, RsrcEntryPtr *arrayEnd )
{
	RecordListPtr	listP;

	ErrNonFatalDisplayIf (!openP->resDB, "Resource DB required");
	
	// get the list (part of the header)
	listP = PrvGetEntryArrayByIndex(openP, whichArray);

	if( listP ){
		// return array info
		*arrayStart = (RsrcEntryPtr)&listP->firstEntry;
		*arrayEnd = *arrayStart + listP->numRecords;
		return true;
		}
	else{
		// return null
		*arrayStart = 0;
		*arrayEnd = 0;
		return false;
		}
}

/************************************************************
 *
 *  FUNCTION: PrvGetRecordEntryArrayBounds, private
 *
 *  DESCRIPTION:  Returns pointers to the beginning and end of 
 *                the ith entry array or nulls if that array does
 *						not exist.  
 *
 *		It assumes that all record list chunks have been locked
 *		as a result of opening the database.
 *
 *  PARAMETERS:  
 *			openP 			-  DmOpenInfoPtr of open database
 *			whichArray 		-  zero based array index
 *			arrayStart		-  pointer to first entry in array, or null
 *			arrayEnd		-  pointer to entry just after the last
 *									valid entry in the array, or null
 *
 *  RETURNS: true if whichArray was found, false otherwise
 *
 *  CREATED: 3/31/00
 *
 *  BY: Russell Y. Webb
 *
 *************************************************************/

static Boolean PrvGetRecordEntryArrayBounds( DmOpenInfoPtr openP, UInt16 whichArray, 
               	RecordEntryPtr *arrayStart, RecordEntryPtr *arrayEnd )
{
	RecordListPtr	listP;

	ErrNonFatalDisplayIf (openP->resDB, "Record DB required");
	
	// get the list (part of the header)
	listP = PrvGetEntryArrayByIndex(openP, whichArray);

	if( listP ){
		// return array info
		*arrayStart = (RecordEntryPtr)&listP->firstEntry;
		*arrayEnd = *arrayStart + listP->numRecords;
		return true;
		}
	else{
		// return null
		*arrayStart = 0;
		*arrayEnd = 0;
		return false;
		}
}

			
/************************************************************
 *
 *  FUNCTION: PrvRsrcEntryPtr, private
 *
 *  DESCRIPTION: Returns a pointer to the Nth resource entry in the
 *		Database header. This routine supports databases that have
 *		multiple RecordLists stored in multiple Memory chunks.
 *
 *		It assumes that all record list chunks have been locked
 *		as a result of opening the database.
 *
 *		If prevRsrcP is non-nil IT MUST BE A POINTER TO THE RESOURCE
 *		at index-1. If provided by the caller, it allows this routine
 *		to find the desired record entry faster (by avoiding a multiply).
 *
 *  PARAMETERS:  
 *			openP 			-	DmOpenInfoPtr of open database
 *			index			 	-  index of desired resource
 *			prevRsrcP 	- 	pointer to previous resource entry, or nil		
 *
 *  RETURNS: pointer to resource entry
 *
 *  CREATED: 8/23/95
 *
 *  BY: Ron Marianetti
 *
 *		roger	8/5/98	Added check for record databases
 *
 *************************************************************/

static RsrcEntryPtr	PrvRsrcEntryPtr(DmOpenInfoPtr openP, UInt16 index,
			RsrcEntryPtr	prevRsrcP)
{
	RecordListPtr	listP;
	LocalID			id;
	UInt16			cumIndex;
	RsrcEntryPtr	rsrcP;
	
	ErrNonFatalDisplayIf (!openP->resDB, "Resource DB required");
	
	// Point to the first list (part of the header)
	listP = &openP->hdrP->recordList;



	// Optimize for the first record list
	if (index < listP->numRecords) {
	
		// If the previous resource MemPtr was passed, take shortcut.
		if (prevRsrcP) {
			#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
				rsrcP = (RsrcEntryPtr)&listP->firstEntry;
				if (prevRsrcP != &(rsrcP[index-1])) 
					ErrDisplay("Invalid previous rsrc MemPtr");
			#endif
			return prevRsrcP+1;
			}
			
		// Else, do a multiply
		rsrcP = (RsrcEntryPtr)&listP->firstEntry;
		return &(rsrcP[index]);
		}
		
		
		
	// MemHandle the case where the requested index is not in the first record list
	cumIndex = listP->numRecords;
	while(listP) {
		if (!(id = listP->nextRecordListID)) goto ExitErr;
		listP = MemLocalIDToGlobal(id, openP->cardNo);
		
		// If it's not ROM based, we actually get a MemHandle from MemLocalIDToGlobal
		if (!openP->handleTableP) 
			listP = (RecordListPtr)MemDeref((MemHandle)listP);
		
		// If the record is in list list, return MemPtr to it's entry
		if (index < cumIndex + listP->numRecords) {
			UInt16	relIndex = index - cumIndex;
			
			// If the previous resource MemPtr was passed, take shortcut.
			if (index > cumIndex && prevRsrcP) {
				#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
					rsrcP = (RsrcEntryPtr)&listP->firstEntry;
					if (prevRsrcP != &(rsrcP[relIndex-1])) 
						ErrDisplay("Invalid previous rsrc MemPtr");
				#endif
				return prevRsrcP+1;
				}
				
			// Else, do a multiply
			rsrcP = (RsrcEntryPtr)&listP->firstEntry;
			return &(rsrcP[relIndex]);
			}
			
		// On to the next list
		cumIndex += listP->numRecords;
		}
		
		
ExitErr:
	// If we got here, the index is invalid
	ErrDisplay("Index out of range");
	return 0;
}



/************************************************************
 *
 *  FUNCTION: PrvPrevRecordList, private
 *
 *  DESCRIPTION: Returns pointer to the previous record list in a
 *		Database given a record list pointer.
 *
 *		This routine has to start from the top (the database header)
 *		and walk through each record list in order to find the previous one.
 *		
 *
 *  PARAMETERS: 
 *			openP 			-	DmOpenInfoPtr of open database
 *			listP				-  pointer to current record list
 *			
 *
 *  RETURNS: 
 *			RecordListPtr of previous recordList, or 0 if no previous list.
 *
 *	 CALLED BY: 
 *			PrvShiftRecordsDown
 *
 *  CREATED: 8/23/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
static RecordListPtr	PrvPrevRecordList(DmOpenInfoPtr openP, RecordListPtr currentP)
{
	RecordListPtr	listP, prevListP=0;
	LocalID			id;
	
	// Point to the first list (part of the header)
	listP = &openP->hdrP->recordList;

	// If we're at the header, return 0
	if (currentP == listP) return 0;

	// Walk through till we get to the current list
	while(1) {
		prevListP = listP;
		id = listP->nextRecordListID;
		ErrFatalDisplayIf(!id, "Can't find previous list");
		
		// Convert the LocalID to a pointer
		listP = MemLocalIDToGlobal(id, openP->cardNo);
		
		// If it's not ROM based, we actually get a MemHandle from MemLocalIDToGlobal
		if (!openP->handleTableP)
			listP = (RecordListPtr)MemDeref((MemHandle)listP);
			
		if (listP == currentP) return prevListP;
		} 
		
	// Should never get here...
	ErrDisplay("Error in logic");
	return 0;
}



 
/************************************************************
 *
 *  FUNCTION: PrvDmHandleRealloc
 *
 *  DESCRIPTION: This routine will re-allocate a data manager
 *		MemHandle in another heap, copy the contents over, and
 *		release the original MemHandle.
 *		
 *  PARAMETERS: 
 *			dbR  - 		open ref of database this MemHandle goes with
 *			oldH - 		original MemHandle
 *			newH - 		new MemHandle if already allocated
 *			size - 		new size (ignored if newH is non-nil)
 *			
 *  RETURNS: 
 *			new MemHandle
 *
 *	 CALLED BY: 
 *			PrvMergeFree
 *
 *  CREATED: 9/26/96
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
static MemHandle	PrvDmHandleRealloc(DmOpenRef dbR, MemHandle oldH, 
						MemHandle newH, UInt32 size)
{
	UInt8		* srcP;
	UInt8		* dstP;
	UInt32	oldSize;
	
	oldSize = MemHandleSize(oldH);
	
	// If we don't have a new MemHandle yet, make one
	if (!newH) {
		newH = PrvDmNewHandle(dbR, size);
		if (!newH) return 0;
		
		// Set the owner ID
		MemHandleSetOwner(newH, MemHandleOwner(oldH));
		}

	// Copy  the contents over
	srcP = MemHandleLock(oldH);
	dstP = MemHandleLock(newH);
	MemMove(dstP, srcP, oldSize);
	MemPtrUnlock(dstP);
	MemPtrUnlock(srcP);
	MemHandleFree(oldH);
	return newH;
}


/************************************************************
 *
 *	FUNCTION:		PrvAddEntryToDBList
 *
 *	DESCRIPTION:	Add an entry to the DB list. This routine
 *		is only used by the debug ROM when we have to rebuild
 *		the DB list in the RAM store, so for now this code is
 *		kept separate from the same basic functionality in
 *		DmCreateDatabase...eventually it should be combined.
 *
 *	PARAMETERS:
 *		dirP			->	Ptr to list of DBs.
 *		headerP		-> Ptr to database header.
 *		baseID		-> LocalID of base DB to add.
 *		
 *	RETURNS:			Ptr to locked DB list.
 *
 *	HISTORY:
 *		10/06/99	kwk	Created by Ken Krugler.
 *
 *************************************************************/
static DatabaseDirPtr
PrvAddEntryToDBList(DatabaseDirPtr dirP, DatabaseHdrPtr headerP,
			UInt16 card, LocalID baseID)
{
	UInt16 numEntries;
	UInt32 actSize, reqSize;
	UInt16 position;
	
	// Resize the directory chunk if necessary
	// DOLATER... this area is troublesome if new fails when there are
	// multiple storage heaps -- it does not try to reallocate from another.
	numEntries = dirP->numDatabases + 1;
	actSize = MemPtrSize((MemPtr)dirP);
	reqSize = OffsetOf(DatabaseDirType, databaseID) + numEntries * sizeof(DatabaseDirEntryType);
	if (reqSize > actSize) {
		MemHandle dirH = MemPtrRecoverHandle((MemPtr)dirP);
		MemPtrUnlock((MemPtr)dirP);
		ErrFatalDisplayIf(MemHandleResize(dirH, reqSize) != errNone, "Can't add entry to DB list");
		dirP = MemHandleLock(dirH);
		}

	// Add the database to the directory. We know we only get called
	// to add RAM DBs to the list, thus ramDB param is true.
	position = PrvBinarySearch(dirP, card, headerP->type, headerP->creator, headerP->version, true);
	MemMove(&dirP->databaseID[position+1], &dirP->databaseID[position], (dirP->numDatabases-position) * sizeof(DatabaseDirEntryType));
	dirP->databaseID[position].baseID = baseID;
	dirP->numDatabases = numEntries;

	return(dirP);
}


/************************************************************
 *
 *  FUNCTION: PrvSearchDatabase
 *
 *  DESCRIPTION: Search records in a database according to the 
 *  					passed comparison function using binary search
 *
 *  PARAMETERS: DmOpenRef for database to search, comparison function, 
 *					search element, other data passed to the comparison function, 
 *					a pointer to the position result.  We don't use DmFindSortPosition 
 *					because we need the "foundExact" boolean return value.
 *					
 *					Hopefully, this can become a general purpose
 *					system routine someday.
 *					
 *  RETURNS: true if an exact match was found
 *				 a position in the database where the element should be located.
 *
 *  CREATED: 8/13/96
 *
 *  BY: Roger Flores
 *		Jesse 1999-10-01: Copy from SysBinarySearch(), modify to search databases
 *
 *************************************************************/

static Boolean PrvSearchDatabase (DmOpenRef dbRef, 
	DmSearchFuncPtr searchF, void const * searchData, const Int32 other, 
	UInt16 * position)
{
	UInt16 numOfElements = DmNumRecords(dbRef);
	Int32 lPos = 0;
	Int32 rPos = (Int32)numOfElements - 1;
	Int32 mPos;
	MemHandle	recH;
	MemPtr		recP;
	Boolean		result;
	
	// keep going until the pointers cross, that way
	// you get the index of smallest (leftmost) elt >= item
	// NOTE: (may return Length(array))
	while (lPos <= rPos) {
		mPos = ((UInt32)(lPos + rPos)) / 2;
		
		recH = DmGetRecord(dbRef, (UInt16)mPos);
		ErrNonFatalDisplayIf(recH == 0, "bad index");
		recP = MemHandleLock(recH);
		
		if (searchF(searchData, recP, other) > 0) // mid value < search value
			lPos = mPos + 1;
		else
			rPos = mPos - 1;
		
		MemHandleUnlock(recH);
		DmReleaseRecord(dbRef, (UInt16)mPos, false);
		}
	
	*position = (UInt16)lPos;
	
	// if we've gone off the right end, don't do the final compare
	if (lPos >= numOfElements)
		return false;
	
	
	recH = DmGetRecord(dbRef, (UInt16)lPos);
	ErrNonFatalDisplayIf(recH == 0, "bad index");
	recP = MemHandleLock(recH);
	
	result = (searchF(searchData, recP, other) == 0);
	
	MemHandleUnlock(recH);
	DmReleaseRecord(dbRef, (UInt16)lPos, false);
	
	// otherwise, may not have checked current element for equality, do it now
	return result;
}

/************************************************************
 *
 *  FUNCTION: PrvBinarySearch
 *
 *  DESCRIPTION: Binary search in database directory for a given
 *					  type/creator/version.
 *
 *  PARAMETERS: directory array baseP,
 *
 *  RETURNS: index into dir DB of matching elt, or index where
 *				 it would appear if there is no match.  Can return
 *				 dirP->numEntries, to indicate appending to end.
 *	
 *	HISTORY:
 *		05/04/98	bob	Created by Bob Ebert.
 *		11/02/99	kwk	Sort RAM entries after identical ROM entries,
 *							so that DmGetNextDatabaseByTypeCreator works
 *							when doing a latest version only search.
 *
 *************************************************************/
static UInt16 PrvBinarySearch (const DatabaseDirType* dirP, Int32 cardNo,
						UInt32 type, UInt32 creator, UInt16 version, Boolean ramDB)
{
	Int32 lPos = 0;
	Int32 rPos = (Int32)dirP->numDatabases - 1;
	Int32 mPos;
	DatabaseHdrPtr eltHdrP;
	Int32 result;
	
	// keep going until the pointers cross, that way
	// you get the index of smallest (leftmost) elt >= item
	// NOTE: (may return numOfElements to indicate appending at end)
	while (lPos <= rPos)
	{
		mPos = ((UInt32)(lPos + rPos)) / 2;

		eltHdrP = MemLocalIDToLockedPtr(dirP->databaseID[mPos].baseID, cardNo);

		result = eltHdrP->type - type;
		if (result == 0)
			result = eltHdrP->creator - creator;
		if (result == 0)
			result = (Int32)eltHdrP->version - version;
		
		// If the result is still zero, then we might have two DBs with
		// different names, or the same name but one is in ROM and the
		// other is in RAM. Always sort ROM before RAM, so 
		if ((result == 0)
		&& ((MemLocalIDKind(dirP->databaseID[mPos].baseID) == memIDHandle) != ramDB))
			result = ramDB ? -1 : 1;
		
		MemPtrUnlock(eltHdrP);
		
		if (result < 0) // mid value < search value
			lPos = mPos + 1;
		else
			rPos = mPos - 1;
	}
	return (UInt16)lPos;
}

/************************************************************
 *
 *  FUNCTION: PrvRecordEntryPtr, private
 *
 *  DESCRIPTION: Returns a pointer to the Nth record entry in the
 *		Database header. This routine supports databases that have
 *		multiple RecordLists stored in multiple Memory chunks.
 *
 *		It assumes that all record list chunks have been locked
 *		as a result of opening the database.
 *
 *		If prevRecordP is non-nil IT MUST BE A POINTER TO THE RECORD
 *		at index-1. If provided by the caller, it allows this routine
 *		to find the desired record entry faster (by avoiding a multiply).
 *
 *  PARAMETERS: 
 *			openP 			-	DmOpenInfoPtr of open database
 *			index			 	-  index of desired record
 *			prevRecordP 	- 	pointer to previous record entry, or nil		
 *
 *  RETURNS: pointer to record entry
 *
 *  CREATED: 8/23/95
 *
 *  BY: Ron Marianetti
 *
 *		roger	8/5/98	Added check for resource databases
 *
 *************************************************************/
static RecordEntryPtr	PrvRecordEntryPtr(DmOpenInfoPtr openP, UInt16 index, 
			RecordEntryPtr	prevRecordP)
{
	RecordListPtr	listP;
	LocalID			id;
	UInt16			cumIndex;
	RecordEntryPtr	recordP;
	
	ErrNonFatalDisplayIf (openP->resDB, "Record DB only");
	
	// Point to the first list (part of the header)
	listP = &openP->hdrP->recordList;

	// Optimize for the first record list
	if (index < listP->numRecords) {
	
		// If the previous record MemPtr was passed, take shorcut.
		if (prevRecordP) {
			#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
				recordP = (RecordEntryPtr)&listP->firstEntry;
				if (prevRecordP != &(recordP[index-1])) 
					ErrDisplay("Invalid previous rec MemPtr");
			#endif
			return prevRecordP+1;
			}
			
		// Else, do a multiply
		recordP = (RecordEntryPtr)&listP->firstEntry;
		return &(recordP[index]);
		}
		
	// MemHandle the case where the requested index is not in the first record list
	cumIndex = listP->numRecords;
	while(listP) {
		if (!(id = listP->nextRecordListID)) goto ExitErr;
		listP = MemLocalIDToGlobal(id, openP->cardNo);
		
		// If it's not ROM based, we actually get a MemHandle from MemLocalIDToGlobal
		if (!openP->handleTableP) 
			listP = (RecordListPtr)MemDeref((MemHandle)listP);
		
		// If the record is in list list, return MemPtr to it's entry
		if (index < cumIndex + listP->numRecords) {
			UInt16	relIndex = index - cumIndex;

			// If the previous record MemPtr was passed, take shorcut.
			if (prevRecordP && index > cumIndex) {
				#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
					recordP = (RecordEntryPtr)&listP->firstEntry;
					if (prevRecordP != &(recordP[relIndex-1]))
						ErrDisplay("Invalid previous rec MemPtr");
				#endif
				return prevRecordP+1;
				}
				
			// Else, do a multiply
			recordP = (RecordEntryPtr)&listP->firstEntry;
			return &(recordP[relIndex]);
			}
			
		// On to the next list
		cumIndex += listP->numRecords;
		}
		
		
ExitErr:
	// If we got here, the index is invalid
	ErrDisplay("Index out of range");
	return 0;
}


/***********************************************************************
 *
 * FUNCTION:		PrvIsLauncherVisible
 *
 * DESCRIPTION:	Determine if a database is "visible" to the launcher,
 *						and thus should be included in the Launch database.
 *						This includes applications as well as prefs panels, 
 *						patches, extensions, libraries, etc... as well as 
 *						any other database with the dmHdrAttrLaunchableData 
 *						bit set in its header
 *						
 *						Used to determine what can be beamed, and what is displayed 
 *						in the Memory Usage stats list.
 *						
 * PARAMETERS:		
 *						dbType		-> type of database
 *						dbCreator 	-> creator of database
 *						dbAttrs		-> attribute bits (dmHdrAttrXXX)
 *
 * RETURNED:    	true if it should be visible
 *						false otherwise
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			jesse	7/28/98	Initial Revision
 *			jesse	10/1/99	Copied from Launcher, modified for DataMgr Launch Database
 *
 ***********************************************************************/
static Boolean PrvIsLauncherVisible ( UInt32 dbType, UInt32 dbCreator, UInt16 dbAttrs)
{
	
	// Don't show anything concerning the Launcher
	if (dbCreator == sysFileCLauncher)
			return false;
	
	//----------------------------------------------------
	// In the main view, show DB's of type sysFileTApplication as long 
	//  as they don't have heir hidden bit set. 
	// Also show any databases with the dmHdrAttrLaunchableData bit
	//  set. 
	//----------------------------------------------------
	
	// Hidden databases go in the Launch database - the Launcher just doesn't display them.
	if (dbAttrs & dmHdrAttrLaunchableData)
			return true;
	
	//----------------------------------------------------
	// In the beam, delete, and info views:
	// Show apps, panels, patches,
	//  extensions, libraries, and launchable data DBs
	//----------------------------------------------------
	if (dbType == sysFileTApplication || dbType == sysFileTPanel || 
		dbType == sysFileTSystemPatch || dbType == sysFileTExtension || 
		dbType == sysFileTLibrary || dbType == sysFileTExgLib)
			return true;
	
	
	return false;
}



/************************************************************
 *
 *  FUNCTION: PrvValidateDmAccessPtr
 *
 *  DESCRIPTION: Check the validity of the DmOpenRef parameter
 *
 *  PARAMETERS: Database access pointer
 *
 *  RETURNS: 
 *
 *  CREATED: 09/30/1999 
 *
 *  BY: Ludovic Ferrandis
 *
 *************************************************************/
static void PrvValidateDmAccessPtr (DmAccessPtr dbP)
{
	ErrNonFatalDisplayIf(!dbP || MemPtrSize(dbP) != sizeof(*dbP) ||
		!dbP->openP || MemPtrSize(dbP->openP) != sizeof(*dbP->openP), 
		"Bad DBRef");
}

/************************************************************
 *
 *  FUNCTION: PrvSyncLaunchDatabase
 *
 *  DESCRIPTION: Updates the Launch database if necessary
 *					  to ensure that its in sync with all the
 *					  databases on the device.
 *					  Creates the database if necessary.
 *  
 *  RETURNS: errNone if successfull..
 *  
 *  CREATED: 9/30/99 
 *
 *  BY: Jesse Donaldson
 *
 *************************************************************/
Err PrvSyncLaunchDatabase(void)
{
	DmLaunchDBRecordType	launchRec;
	DmLaunchDBRecordPtr	recP;
	DmOpenRef	dbRef = 0;
	UInt16		searchCardNo;
	LocalID		dbID;
	Err			err;
	UInt16		i, numRecords, numDBs, dbIndex;
	MemHandle	recH;
	DatabaseHdrPtr hdrP;
	UInt16 dbAttr, dbVers;
		
	//---------------------------------------------------------------
	// First, get us a Launch database to sync up
	//---------------------------------------------------------------
	// ¥DOLATER¥: open, check version, delete if its incorrect & try again.
	// This ensures future databases won't completely hose current devices.
	dbID = DmFindDatabase(0, dmLaunchDatabaseName);
	
	// If there is no database present...
	if (dbID == 0) {
		// Create a new database...
		err = DmCreateDatabase(0 /*cardNo*/, dmLaunchDatabaseName, dmLaunchDatabaseCreator, 
					dmLaunchDatabaseType, false /*resDB*/);
		if (err) 
			return err;
		
		// ... try to find it again...
		dbID = DmFindDatabase(0, dmLaunchDatabaseName);
		if (dbID == 0) {
			return DmGetLastErr();
			}
		
		// Set the info:
		// This database needs to be backed up to store the users categories, but
		// this can cause problems since it also contains semi-volatile data like the
		// card number & database ID of all the databases, so we also set the 
		// 'reset on install' bit so that when the database is restored, the device is reset.
		// This will cause all of the volatile stuff to be replaced with refreshed versions,
		// and will still maintain the category names & filings.
		dbAttr = dmHdrAttrResetAfterInstall | dmHdrAttrBackup | dmHdrAttrCopyPrevention;
		dbVers = dmLaunchDatabaseVersion;
		
		err = DmSetDatabaseInfo(0, dbID, 0 /*nameP*/, &dbAttr, &dbVers, 
					0 /*crDateP*/, 0 /*modDateP*/, 0 /*bckUpDateP*/,
					0 /*modNumP*/, 0 /*appInfoIDP*/, 0 /*sortInfoIDP*/, 
					0 /*typeP*/, 0 /*creatorP*/);
		}
		
	// finally, open the DB
	dbRef = DmOpenDBNoOverlay(0, dbID, dmModeReadWrite);
	if (dbRef == 0) {
		DmDeleteDatabase(0, dbID);
		return DmGetLastErr();
		}
	
	//---------------------------------------------------------------
	//	Now, iterate through all the records (if any) and mark them as orphaned.
	//---------------------------------------------------------------
	PrvSetLaunchRecordFlags(dmLaunchFlagOrphaned, dbRef, 0, 0, 0);
	
	// initialize fill-in launch record:
	MemSet(&launchRec, sizeof(DmLaunchDBRecordType), 0);
	
	//---------------------------------------------------------------
	//	Now, iterate through all databases on the device and make sure they're in the Launch database
	//---------------------------------------------------------------
	for (searchCardNo = 0; searchCardNo < MemNumCards(); searchCardNo++) {
		numDBs = DmNumDatabases(searchCardNo);
		
		//---------------------------------------------------------------
		// Loop through databases on this card. 
		//---------------------------------------------------------------
		for (dbIndex = 0; dbIndex < numDBs; dbIndex++) {
			
			//--------------------------------------------------------
			// Get info on the next database and see if it should be visible.
			//--------------------------------------------------------
			dbID = DmGetDatabase(searchCardNo, dbIndex);
			
			hdrP = MemLocalIDToLockedPtr(dbID, searchCardNo);
						
			// Check if it belongs to avoid an extra linear
			// search through the Launch database to dirty any related 
			// records that happens if its an overlay.
			if (PrvIsLauncherVisible(hdrP->type, hdrP->creator, hdrP->attributes)) {
				
				// Make sure its represented in our Launch database, if it belongs.  
				// If its already there, this will clear the 'orphaned' bit, 
				// & set the 'dirty' bit so that the launcher will update the 
				// cached data (name, icons, etc).
				PrvUpdateLaunchDBEntry(dbRef, hdrP, dbID, 0);
				}
			
			
			MemPtrUnlock(hdrP);
			
			} // for (dbIndex = 0; dbIndex < numDBs; dbIndex++)
			
		} // for (searchCardNo = 0; searchCardNo < MemNumCards(); searchCardNo++)
	
	
	//---------------------------------------------------------------
	// Now go through and delete any records still marked orphaned
	// these are the ones that we didn't just touch.
	//---------------------------------------------------------------
	numRecords = DmNumRecords(dbRef);
	
	i=0;
	while(i<numRecords) {
		recH = DmGetRecord(dbRef, i);
		ErrNonFatalDisplayIf(recH == 0, "GetRecord failed");
		
		recP = MemHandleLock(recH);
		
		if (recP->flags & dmLaunchFlagOrphaned) {
			// remove the record:
			DmRemoveRecord(dbRef, i);
			
			// one less record now...
			// 'i' is already the index to the next 
			// record, so don't increment it!!
			numRecords--; 
			}
		else {
			MemHandleUnlock(recH);
			DmReleaseRecord(dbRef, i, true);
			
			i++; // check next record...
			}
			
		}
	
	// All done with the database, close it up & return.
	DmCloseDatabase(dbRef);
	
	return errNone;
}


/**************************************************************
 *
 *  FUNCTION: PrvOpenLaunchDatabase
 *
 *  DESCRIPTION: Open the Launch database and return the DmOpenRef
 *					  if successful (fatal error otherwise).
 *  
 *  RETURNS: DmOpenRef for Launch database if successful, fatal error otherwise
 *  
 *  CREATED: 9/30/99 
 *
 *  BY: Jesse Donaldson
 *
 *************************************************************/
DmOpenRef PrvOpenLaunchDatabase(void)
{
	DmOpenRef dbRef = 0;
	LocalID launcherDBID;
	
	// try to open the launch database...
	// dbRef = DmOpenDatabaseByTypeCreator(dmLaunchDatabaseType, 
	// 						dmLaunchDatabaseCreator, dmModeReadWrite);
	
	launcherDBID = DmFindDatabase(0, dmLaunchDatabaseName);
	if (launcherDBID)
		dbRef = DmOpenDBNoOverlay(0, launcherDBID, dmModeReadWrite);

	if (dbRef == 0) {
		switch(DmGetLastErr()) {
			case dmErrAlreadyOpenForWrites:
			case dmErrOpenedByAnotherTask:
			case dmErrDatabaseOpen:
				// tip for developers working with launchDB
				// Should this be fatal??
				ErrFatalDisplay("LaunchDB already open - please close it");
				break;
			
			default:
				// Should this be fatal??
				ErrFatalDisplay("Can't open LaunchDB");
				break;
			
			}
		}
	
	return dbRef;
}




/***********************************************************************
 *
 * FUNCTION:		PrvUpdateLaunchDBEntry
 *
 * DESCRIPTION:	Called to add or update an entry in the Launch database.
 *						Searches the database for a database with the given type,
 *						creator, and database name.  If the new version is the same or 
 *						greater, any entry found will be replaced with this one.
 *						If no entry is found, the new one is inserted at the right spot.
 *						
 *						Possible performance enhancement: Make a 
 *						pass searching for LocalID instead of name?
 *						
 *						DOLATER: make RAM databases always override ROM databases
 *						
 * PARAMETERS:		
 *						dbRef			-> DmOpenRef to open database, or NULL if 
 *											the function should open & close the datbase itself.
 *						headerP		-> Ptr to database header of new database
 *						dbID			-> LocalID of database header since headerP may not be a real handle
 *						categoryP	-> Ptr to new category, or NULL
 *
 * RETURNED:    	none
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			jesse	10/1/99	Initial Revision
 *
 ***********************************************************************/
void PrvUpdateLaunchDBEntry(DmOpenRef dbRef, DatabaseHdrPtr headerP, LocalID dbID, UInt16 *categoryP)
{
	Boolean		closeDatabase = false;
	Boolean		foundExact;
	MemHandle	recH;
	MemPtr		recP;
	UInt16		pos;
	Err			err;
	DmLaunchDBRecordPtr launchRecP = 0;
	
	if (headerP->type == dmLaunchDatabaseType && headerP->creator == dmLaunchDatabaseCreator) 
			return;
	
	//------------------------------------------
	// Open the Launch database if we have to
	//------------------------------------------
	if (dbRef == 0) {
		dbRef = PrvOpenLaunchDatabase();
		if (dbRef == 0) 
				return;
		closeDatabase = true;
		}
	
	// If we're installing/updating an overlay, mark any related entries dirty. 
	if (headerP->type == sysFileTOverlay) {		
		PrvSetLaunchRecordFlags(dmLaunchFlagDirty, dbRef, 0, headerP->creator, 0);
		}
	
	// bail if entry doesn't belong in database
	if (PrvIsLauncherVisible(headerP->type, headerP->creator, headerP->attributes) == false)
		goto Exit;
	
	//------------------------------------------
	// Fill in database info:
	//------------------------------------------
	launchRecP = MemPtrNew(sizeof(DmLaunchDBRecordType));
	ErrFatalDisplayIf(launchRecP == 0, "Out of memory"); // force reset if launchDB cannot be updated.
	MemSet(launchRecP, sizeof(DmLaunchDBRecordType), 0x00);
	
	launchRecP->type = headerP->type;						// type of database
	launchRecP->creator = headerP->creator;				// creator of database
	launchRecP->dbID = dbID;									// LocalID of database
	launchRecP->cardNo = MemPtrCardNo(headerP);			// card number of database
	launchRecP->dbAttrs = headerP->attributes;			// database attributes
	launchRecP->version = headerP->version;				// version of database
	StrCopy((char*)launchRecP->dbName, (char*)headerP->name);			// database name
	
	launchRecP->lgIconOffset = 0;
	launchRecP->smIconOffset = 0;
	launchRecP->iconName[0] = '\0';
	launchRecP->versionStr[0] = '\0';
	launchRecP->flags = dmLaunchFlagDirty;	// launcher info is empty - leave record dirty.
	launchRecP->reserved = 0;
	
	// search database to find the exact element or the place it should be inserted.
	foundExact = PrvSearchDatabase (dbRef, PrvLaunchCompareFunc, launchRecP, 0, &pos);
	
	if (foundExact) {
		// We found an exact match, determine whether to replace the match with the new entry.
		recH = DmGetRecord(dbRef, pos);
		recP = MemHandleLock(recH);
		
      // Replace the entry if the existing entry is orphaned
      // (we were unsure if the database really exists) or if
      // the new one is a newer version.  See PrvLaunchCompareVers().
      // RAM databases will override ROM based ones in the Launch DB.
      if ((((DmLaunchDBRecordPtr)recP)->flags & dmLaunchFlagOrphaned) ||
          PrvLaunchCompareVers (launchRecP, recP) > 0) {
						
			// Delete bitmap data appended to the record, if any.
			// Since we never grow the record, we don't NEED to unlock the handle & lock it again.
			// But if the record size changes, this will ensure that old databases will be usable
			// with only a reset.  Otherwise, we might crash during boot because we really WERE 
			// trying to grow the record - not a good thing.
			MemHandleUnlock(recH);
			err = MemHandleResize(recH, sizeof(DmLaunchDBRecordType));
			ErrNonFatalDisplayIf(err, "shrink failed");
			recP = MemHandleLock(recH);
			
			// If we're not setting the category...	
			if(categoryP == 0) {
				// copy over the 'resetCategory' launch flag in case this item hasn't ever been filed.
				launchRecP->flags |= (((DmLaunchDBRecordPtr)recP)->flags & dmLaunchFlagResetCategory);
				}
				
			// copy new record over the old one.
			// This clears all launcher data (incl. orphaned bit), 
			// sets the dirty bit, and replaces the cached database info.
			err = DmWrite(recP, 0, launchRecP, sizeof(DmLaunchDBRecordType));
			ErrNonFatalDisplayIf(err, "DmWrite failed");
			
			// Launcher will need to update the alternate sort order:
			PrvKillLaunchDBSortInfo(dbRef);
			}
		
		}
	else {
		// No exact match, insert a new record at 'pos' in the database.
		recH = DmNewRecord(dbRef, &pos, sizeof(DmLaunchDBRecordType));
		if (recH == 0) 
				goto Exit;
		
		recP = MemHandleLock(recH);
		
		
		// If we're not setting the category...	
		if(categoryP == 0) {
			// file it into the default category (since its a new entry).
			launchRecP->flags |= dmLaunchFlagResetCategory;
			}
			
		err = DmWrite(recP, 0, launchRecP, sizeof(DmLaunchDBRecordType));
		ErrNonFatalDisplayIf(err, "DmWrite failed");
			
		// Launcher will need to update the alternate sort order:
		PrvKillLaunchDBSortInfo(dbRef);
		}
	
	// if setting category, do it now:
	if (categoryP) {
		UInt16 attr;
		DmRecordInfo(dbRef, pos, &attr,0,0);
		attr &= ~dmRecAttrCategoryMask;
		attr |= ((*categoryP) & dmRecAttrCategoryMask);
		
		DmSetRecordInfo(dbRef, pos, &attr, 0);
		}
	
	MemHandleUnlock(recH);
	DmReleaseRecord(dbRef, pos, true);
	
Exit:

	if (closeDatabase && dbRef) {
		DmCloseDatabase(dbRef);
		}
	
	if (launchRecP) {
		MemPtrFree(launchRecP);
		}
	
	return;
}



/***************************************************************
 *  Function:    PrvCreateCompositeAppVersion
 *
 *  Summary:
 *      Creates a 32-bit version that can be used to compare two
 *      launch records to figure out which one should be used if
 *      they have the same type creator.  Uses the following rules:
 *
 *      - Higher version goes first...
 *      - If same version, RAM goes first...
 *      - If same version and both in same RAM/ROM, lower card number
 *        goes first.
 *
 *  Notes:
 *      Currently supports up to 256 cards, but that's the max number
 *      supported by the current OS according to some docs I read.
 *
 *  Parameters:
 *    recP    IN    The launch record to create a composite version for.
 *
 *  Returns:
 *    A 32-bit composite version such that if you compare two
 *    versions, the higher version is the better app to use.
 *
 *  History:
 *    05-Jul-2000 dia Created
 *   11-nov-2000 jesse Incorporated as part of bug fix from Handspring
 *
 ****************************************************************/

static UInt32 PrvCreateCompositeAppVersion (const DmLaunchDBRecordType* recP)
{
	const UInt16 kMaxCardsSupported = 256;	// Must be a power of 2 <= 2^12
	UInt32 version;

	// Calculate composite version of this app:
	// - top 16 bits is the version, since version overrides all
	// - bottom 16 bits contain extras that break ties between
	//	 same version things...

	// Bits 16 - 32 are DB version
	version = ((UInt32)recP->version) << 16;

	// Bits 13 - 15 unused

	// Bit 12 is 1 if in RAM, 0 if in ROM.
	if (MemLocalIDKind (recP->dbID) == memIDHandle)
		// RAM gets priority over ROM...
		version |= 0x00001000;

	// Bits 8 - 11 unused
	
	// Bits 0 - 7 represent the card number.  Subtract so that lower
	// card numbers make a higher version.
	if (recP->cardNo <= kMaxCardsSupported-1)
	version |= ((kMaxCardsSupported-1) - recP->cardNo);
	
	return version;
}

/************************************************************
 *
 *  FUNCTION: PrvRemoveFromLaunchDatabase
 *
 *  DESCRIPTION: Removes an entry from the Launch database.
 *				
 *				
 *  PARAMETERS:	dbRef		- DmOpenRef of Launch database, or 0.
 *						type		- Database type of entry to remove
 *						creator	- Database creator of entry to remove
 *						name		- Database name of entry to remove
 *						dbAttr	- Database attributes of entry to remove
 *						
 *		
 *  RETURNS:	category of the removed record, or 0xFFFF.
 *
 *  CREATED: 6/15/98
 *
 *  BY: Jesse Donaldson
 *
 *************************************************************/
UInt16 PrvRemoveFromLaunchDatabase(DmOpenRef dbRef, UInt32 type, UInt32 creator, char* name, UInt16 dbAttr)
{
	DmLaunchDBRecordPtr dummyRecP = 0;
	DmLaunchDBRecordPtr launchRecP;
	UInt16	pos;
	UInt16	category = 0xFFFF;
	Boolean	closeLaunchDB = false;
	MemHandle	recH; 
	
	// Bail if this is the launch database itself:
	if (type == dmLaunchDatabaseType && 
		creator == dmLaunchDatabaseCreator &&
		StrCompareAscii(name, dmLaunchDatabaseName) == 0) goto Exit;
	
	if (dbRef == 0) {
		dbRef = PrvOpenLaunchDatabase();
		closeLaunchDB = true;
		}
	
	if (dbRef == 0) 
			goto Exit;
	
	// If we're removing an overlay, mark any related entries dirty.
	if (type == sysFileTOverlay) {
		PrvSetLaunchRecordFlags(dmLaunchFlagDirty, dbRef, 0, creator, 0);
		}
	
	
	// fill out dummy structure for search routine:
	dummyRecP = MemPtrNew(sizeof(DmLaunchDBRecordType));
	ErrFatalDisplayIf(dummyRecP == 0, "Out of memory"); // force reset if launchDB cannot be updated.
	MemSet(dummyRecP, sizeof(DmLaunchDBRecordType), 0x00);
	
	dummyRecP->creator	= creator;
	dummyRecP->type		= type;
	StrCopy((char*)dummyRecP->dbName, name);
	
	// search database to find the exact element and Delete the record if we found it
	if (PrvSearchDatabase (dbRef, PrvLaunchCompareFunc, dummyRecP, 0, &pos)) {
		
		recH = DmGetRecord(dbRef, pos);
		if(recH)
		{
			launchRecP = (DmLaunchDBRecordPtr)MemHandleLock(recH);
			
			// if category is valid, then return it.
			if((launchRecP->flags & dmLaunchFlagResetCategory) == 0)
			{
				DmRecordInfo(dbRef, pos, &category, 0, 0);
				category &= dmRecAttrCategoryMask;
			}
			
			MemHandleUnlock(recH);
			DmReleaseRecord(dbRef, pos, false);
			
			// remove entry from database
			DmRemoveRecord(dbRef, pos);
		}
		// Launcher will need to update the alternate sort order:
		PrvKillLaunchDBSortInfo(dbRef);
		}
	
Exit:
	
	if (closeLaunchDB && dbRef)
			DmCloseDatabase(dbRef);
	
	if (dummyRecP) {
		MemPtrFree(dummyRecP);
		}
	
	return category;
}


/************************************************************
 *
 *  FUNCTION: PrvLaunchCompareFunc
 *
 *  DESCRIPTION: Helper function for PrvSearchDatabase used for 
 *				finding records and determining the index at which 
 *				to insert new records so we can keep the database 
 *				sorted by type, creator, and name.
 *				
 *				
 *  PARAMETERS:	dataP		-	Pointer to a new record to find the slot for
 *						curEntryP-	Pointer to the current database record to compare it to
 *						other		-	Extra application defined data. In this case, 0.
 *				
 *		
 *  RETURNS:	0 if records are equal, 
 *				<0 if record we're searching for is "smaller" than the current one we're looking at, 
 *				>0 if record we're searching for is "larger" than the current one we're looking at.
 *
 *  CREATED: 6/15/98
 *
 *  BY: Jesse Donaldson
 *
 *************************************************************/
static Int16 PrvLaunchCompareFunc(void const * searchData, void const * dbData, Int32 /*other*/)
{
	DmLaunchDBRecordPtr searchP = (DmLaunchDBRecordPtr)searchData;
	DmLaunchDBRecordPtr recP = (DmLaunchDBRecordPtr)dbData;
	UInt32 search, compare;
	

	// Compare types:
	search = searchP->type;
	compare = recP->type;
	
	// be careful since return type is Int16
	if (compare > search ) 
			return -1;
	else if (compare < search) 
			return 1;
	

	// Compare creators:
	search = searchP->creator;
	compare = recP->creator;
	
	// be careful since return type is Int16
	if (compare > search ) 
			return -1;
	else if (compare < search) 
			return 1;
	
	// Okay, so far, everything's the same.
	// Compare the name for the final tie-breaker:
	return (StrCompareAscii((Char*)searchP->dbName, (Char*)recP->dbName));
}

/***************************************************************
 *    Function:      PrvLaunchCompareVers
 *
 *    Summary:
 *      This is our replacement version comparison in Palm's
 *      PrvUpdateLaunchDBEntry().  Palm has some code that tries
 *      to figure out which database to use in the case that we
 *      have two databases with identical type, creator, and
 *      DBName.  The problem is that their code has some problems,
 *      namely that it uses the incorrect test for ROM vs. RAM
 *      and it doesn't properly deal with card 0 vs. card 1
 *      (card 0 should come before card 1--Palm's code doesn't
 *      make the distinction).
 *
 *    Parameters:
 *      rec1P      IN    Pointer to 1st record to compare
 *      rec2P      IN    Pointer to 2nd record to compare
 *
 *
 *    Returns
 *       0 if records are equal,
 *      -1 if rec 1 is worse than rec 2
 *      +1 if rec 1 is better than rec 2
 *
 *    History:
 *      06-jul-2000 dia Created by Doug Anderson
 *      11-nov-2000 jesse Incorporated as part of bug fix from Handspring
 *
 ****************************************************************/
static Int16 PrvLaunchCompareVers (const DmLaunchDBRecordType* rec1P,
								const DmLaunchDBRecordType* rec2P)
{
	UInt32 version1, version2;

	ErrNonFatalDisplayIf (rec1P->type != rec2P->type ||
						rec1P->creator != rec2P->creator,
						"Not the same type/creator");
	
	// Same type and creator, sort by "composite" version...
	version1 = PrvCreateCompositeAppVersion (rec1P);
	version2 = PrvCreateCompositeAppVersion (rec2P);
	
	if (version1 < version2)
		{
			return -1;
		}
	else if (version1 > version2)
		{
			return 1;
		}

	return 0;
}

/************************************************************
 *
 *  FUNCTION: PrvKillLaunchDBSortInfo
 *
 *  DESCRIPTION: Dispose of sortInfo from launchDB.
 *				
 *  PARAMETERS:	launchDBRef		-	DmOpenRef to Launch database.
 *				
 *		
 *  RETURNS:	none
 *
 *  CREATED: 11/15/99
 *
 *  BY: Jesse Donaldson
 *
 *************************************************************/
static void PrvKillLaunchDBSortInfo(DmOpenRef launchDBRef)
{
	LocalID	dbID, sortInfoID;
	MemHandle	sortInfoH;
	UInt16	cardNo;
	Err		err = DmOpenDatabaseInfo(launchDBRef, &dbID, 0 /*openCountP*/, 
												0 /*modeP*/, &cardNo, 0 /*resDBP*/);
	if (err) 
			return;
	
	// Get the sortInfoID
	err = DmDatabaseInfo(cardNo, dbID, 0 /*nameP*/,
					0 /*attributesP*/, 0 /*versionP*/, 0 /*crDateP*/,
					0 /*modDateP*/, 0 /*bckUpDateP*/,
					0 /*modNumP*/, 0 /*appInfoIDP*/,
					&sortInfoID, 0 /*typeP*/,
					0 /*creatorP*/);
	
	// If there was one, dispose of it and clear the field:
	if (err == errNone && sortInfoID != 0) {
		sortInfoH = MemLocalIDToGlobal(sortInfoID, cardNo);
		
		sortInfoID = 0;
		
		err = DmSetDatabaseInfo(cardNo, dbID, 0 /*nameP*/,
					0 /*attributesP*/, 0 /*versionP*/, 0 /*crDateP*/,
					0 /*modDateP*/, 0 /*bckUpDateP*/,
					0 /*modNumP*/, 0 /*appInfoIDP*/,
					&sortInfoID, 0 /*typeP*/,
					0 /*creatorP*/);
		ErrFatalDisplayIf(err != errNone, "can't set info");
		
		// free orphaned chunk:
		MemHandleFree(sortInfoH);
		}
	
	return;
}



/************************************************************
 *
 *  FUNCTION: PrvSetLaunchRecordFlags
 *
 *  DESCRIPTION: Look through all records in the Launch database,
 *					and if they match the criteria, set the passed
 *					flags.  These flags are ORed in, so you can't
 *					use this routine to clear flags - only to set them.
 *					
 *		DOLATER: Performance enhancement: if we have to match the name,
 *					there is at most 1 record, so do a binary search on
 *					the type & creator (if present) to find it instead
 *					of the slow linear search we're doing now.
 *					This is okay in the meanwhile because this routine is not called often.
 *					
 *  PARAMETERS:	setAttrs	-	Attributes to set for each record matching the passed criteria
 *						dbRef		-	DmOpenRef to the Launch database, or 0 if we should open & close it.
 *						type		-	Type code to match against, or 0 to match any.
 *						creator	-	Creator code to match against, or 0 to match any.
 *						dbName	-	Database name to match against, or NULL to match any.
 *				
 *		
 *  RETURNS:	none
 *
 *  CREATED: 6/15/98
 *
 *  BY: Jesse Donaldson
 *
 *************************************************************/
void PrvSetLaunchRecordFlags(UInt8 setAttrs, DmOpenRef dbRef, UInt32 type, UInt32 creator, char* name)
{
	UInt16		numRecords;
	MemHandle	recH;
	DmLaunchDBRecordPtr		recP;
	UInt16		i;
	UInt8			flags;
	Err err;
	Boolean closeDatabase = false;
	
	//------------------------------------------
	// Open the Launch database if we have to
	//------------------------------------------
	if (dbRef == 0) {
		dbRef = PrvOpenLaunchDatabase();
		if (dbRef == 0) 
				return;
		closeDatabase = true;
		}
	
	
	//------------------------------------------
	// Iterate through all records, 
	// mark the ones that fit our criteria dirty
	//------------------------------------------
	numRecords = DmNumRecords(dbRef);
	for (i=0; i<numRecords; i++) {
		recH = DmGetRecord(dbRef, i);
		ErrNonFatalDisplayIf(recH == 0, "GetRecord failed");
		
		recP = MemHandleLock(recH);
		
		if ((type == 0 || type == recP->type) && 
			(creator == 0 || creator == recP->creator) && 
			(name == 0 || StrCompareAscii(name, (Char*)recP->dbName) == 0))  {
			
			flags = (recP->flags) | setAttrs;
			
			err = DmWrite(recP, OffsetOf(DmLaunchDBRecordType, flags), &flags, 1);
			ErrNonFatalDisplayIf(err, "write failed");
			}
		
		MemHandleUnlock(recH);
		DmReleaseRecord(dbRef, i, true);
		}
	
	if (closeDatabase && dbRef)
			DmCloseDatabase(dbRef);
	
	return;
}


/************************************************************
 *
 *  FUNCTION: PrvValidateResDB
 *
 *  DESCRIPTION: Check the database is a Resource database
 *
 *  PARAMETERS: Database access pointer
 *
 *  RETURNS: 
 *
 *  CREATED: 09/30/1999 
 *
 *  BY: Ludovic Ferrandis
 *
 *************************************************************/
static void PrvValidateResourceDB (DmAccessPtr dbP)
{
	ErrFatalDisplayIf(!dbP->openP->resDB, "Not Rsrc DB");
}


