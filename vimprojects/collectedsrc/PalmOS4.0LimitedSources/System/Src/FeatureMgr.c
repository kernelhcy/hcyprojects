/******************************************************************************
 *
 * Copyright (c) 1994-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: FeatureMgr.c
 *
 * Release: 
 *
 * Description:
 *		Feature Management routines for Pilot
 *
 * History:
 *   	06/21/95	RM		Created by Ron Marianetti
 *		09/02/98	Bob	Reimplemented for performance, added MemPtr stuff
 *		11/24/98	Bob	Switch to normal handles, see comment 11/24/98
 *		06/27/99	kwk	Do double-init w/optional feat=10001 resource.
 *
 *****************************************************************************/

#define	NON_PORTABLE

#include <PalmTypes.h>

#include <DataMgr.h>
#include <ErrorMgr.h>
#include <FeatureMgr.h>
#include <SystemMgr.h>
#include "SystemResourcesPrv.h"

#include "Globals.h"
#include "MemoryPrv.h"
#include "FeaturePrv.h"
#include "DataPrv.h"


/************************************************************
 *	11/24/98 - A word about memory usage for feature pointers
 * 
 *	Initially, the idea was to have FtrPtrs be actual pointers,
 * only stored in the storage heap.  Keeping the at the end of
 * the heap would make sure they stayed out of the way.
 *
 * Unfortunately, DmWrite will not write to pointers that do
 * not have master pointers, DmWriteCheck fails.  So the next
 * thing we tried was to have FtrPtrs be always locked chunks
 * (handles) that are allocated at the end of the storage heap.
 *
 * This works, and keeps them out of the way, but when a feature
 * pointer is resized/moved, the old chunk gets put back on the
 * free list at the beginning (because it's a MemHandle), and so
 * subsequent allocations come from the high memory in the heap.
 * This looks funny in heap dumps and increases fragmentation.
 *
 * So, now FtrPtrs are just handles in the storage heap like any
 * other data manager MemHandle, mixed in with all the other DM-owned
 * chunks.  Since mostly these won't move much and get cleaned up
 * on reset, the fact that they're always locked should not
 * contribute too much to storage heap fragmentation.  ...and
 * typically there's lots of strorage heap free anyway.
 *
 *************************************************************/

/************************************************************
 *	Private routines
 *
 *************************************************************/
static Int16 PrvFtrBinarySearch (const FtrTablePtr ftrTableP, const UInt32 creator,
									  const UInt16 featureNum);

static Err PrvFtrPtrResize(FtrTablePtr ftrTableP, void **ptrP, UInt32 newSize);

static void PrvFtrInitFromResource(	FtrTablePtr ftrTableP,
												UInt16 resID,
												Boolean mustExist);

/************************************************************
 *
 * FUNCTION:	FtrInit
 *
 * DESCRIPTION: Initialize the Feature Manager
 *
 * PARAMETERS:	none
 *
 * RETURNS:		0 if no error
 *
 * HISTORY:
 *		06/21/95	RM		Created by Ron Marianetti
 *		09/03/98 bob	Re-implement for performance
 *		06/25/99	kwk	Moved 'feat'-based init into PrvFtrInitFromResource.
 *		07/25/00	kwk	Deleted bogus call to set up omFtrShowErrorsFlag feature.
 *
 *************************************************************/
Err FtrInit(void)
{
	Err					err = 0;
	MemHandle			ftrTableH;
	FtrTablePtr			ftrTableP;
	FtrTablePtr			initTableP;
	UInt16				i, numHeaps;
	
	// copy ROM features into (initial) feature table
	initTableP = MemPtrNew(sizeof(FtrTableType));
	initTableP->numEntries = 0;
	initTableP->allocEntries = ftrTableSizeInit;
	
	PrvFtrInitFromResource(initTableP, sysResIDFeatures, true);
	PrvFtrInitFromResource(initTableP, sysResIDOverlayFeatures, false);

	// pour the ROM resource features into the initial table.  If we add
	// a bunch of ROM features to the resource, then there may be too many for
	// the initial allocation.  If so, bump up the size of ftrTableSizeInit
	ErrNonFatalDisplayIf(initTableP->numEntries > ftrTableSizeInit, "Feature table init size too small");
	

	// allocate from the first storage heap with sufficient free space;
	// this works partucularly well on cards with only 1 storage heap,
	// as is the case now with the new large heap memory model
	// DOLATER... need to re-examine when we have enough RAM store to necessitate
	// multiple storage heaps again
	numHeaps = MemNumRAMHeaps(ftrTableCardNo);
	for (i=0; i < numHeaps; i++) {
		initTableP->heapID = MemHeapID(ftrTableCardNo, i);
		
		// Don't allocate in the dynamic heap
		if (MemHeapDynamic(initTableP->heapID))
			continue;
		
		// attempt to allocate from this storage heap, (use dmOrphanOwnerID so DmInit cleans up)
		ftrTableH = MemChunkNew(initTableP->heapID, sizeof(FtrTableType), dmOrphanOwnerID);
		if ( ftrTableH ) break;
	}
	ErrFatalDisplayIf(ftrTableH == NULL, "Not enough space for feature table");
	
	ftrTableP = MemHandleLock(ftrTableH);

	// write the temp table to the real space
	if ((err = DmWrite(ftrTableP, 0, initTableP, sizeof(FtrTableType))) != 0) goto Exit;

	// delete the initial table
	if ((err = MemPtrFree(initTableP)) != 0) goto Exit;

	// store the (still locked) MemPtr to table in globals
	GFtrGlobalsP = (MemPtr)ftrTableP;
	
Exit:
	ErrNonFatalDisplayIf(err, "Creating feature database");
	return err;
}





/************************************************************
 *
 *  FUNCTION: FtrUnregister
 *
 *  DESCRIPTION: Unregister a feature
 *
 *  PARAMETERS: 
 *		creator 		- creator type, should be same as the app that owns this
 *									feature
 *		featureNum 	- feature number of the feature
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 6/21/95
 *
 *  BY: Ron Marianetti
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	9/3/98 	Re-implement for performance
 *************************************************************/
Err FtrUnregister(UInt32 creator, UInt16 featureNum)
{
	Err				err;
	FtrTablePtr		ftrTableP;
	Int16				pos;
	
	MemSemaphoreReserve(true);
	
	ftrTableP = (FtrTablePtr)GFtrGlobalsP;
	pos = PrvFtrBinarySearch(ftrTableP, creator, featureNum);
	
	// See if feature exists
	if (pos < ftrTableP->numEntries
	&& ftrTableP->features[pos].creator == creator
	&& ftrTableP->features[pos].featureNum == featureNum) {
		// shift down
		UInt16 temp = ftrTableP->numEntries - 1;
		if ((err = DmWrite(ftrTableP, OffsetOf(FtrTableType, numEntries), &temp, sizeof(UInt16))) != 0)
			goto Exit;
		err = DmWrite(ftrTableP, (UInt8 *)&ftrTableP->features[pos] - (UInt8 *)ftrTableP,
					     &ftrTableP->features[pos+1], (ftrTableP->numEntries - pos) * sizeof (FtrFeatureType));
	}
	else
		err = ftrErrNoSuchFeature;

Exit:
	MemSemaphoreRelease(true);
	ErrNonFatalDisplayIf(err && err != ftrErrNoSuchFeature, "err");
	return err;
}



/************************************************************
 *
 *  FUNCTION: FtrGet
 *
 *  DESCRIPTION: Get a feature
 *
 *  PARAMETERS: 
 *		creator 		- creator type, should be same as the app that owns this
 *									feature
 *		featureNum 	- feature number of the feature
 *		*valueP		- value of the feature is returned here
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 6/21/95
 *
 *  BY: Ron Marianetti
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	8/21/95	Excluded ftrErrNoSuchFeature from causing an
 *								error display
 *			bob	9/3/98 	Re-implement for performance
 *			scl	10/6/98 	Now returns valueP=0 in error case
 *************************************************************/
Err FtrGet(UInt32 creator, UInt16 featureNum, UInt32 * valueP)
{
	Err				err;
	FtrTablePtr		ftrTableP;
	Int16				pos;
	
	MemSemaphoreReserve(false);
	
	ftrTableP = (FtrTablePtr)GFtrGlobalsP;
	pos = PrvFtrBinarySearch(ftrTableP, creator, featureNum);
	
	if (pos < ftrTableP->numEntries
	&& ftrTableP->features[pos].creator == creator
	&& ftrTableP->features[pos].featureNum == featureNum) {
		*valueP = ftrTableP->features[pos].value;
		err = 0;
	}
	else {
		*valueP = 0;
		err =  ftrErrNoSuchFeature;
	}
Exit:
	MemSemaphoreRelease(false);
	ErrNonFatalDisplayIf(err && err != ftrErrNoSuchFeature, "err");
	return err;
}




/************************************************************
 *
 *  FUNCTION: FtrSet
 *
 *  DESCRIPTION: Set a feature
 *
 *  PARAMETERS: 
 *		creator 		- creator type, should be same as the app that owns this
 *									feature
 *		featureNum 	- feature number of the feature
 *		newValue		- new value
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 6/21/95
 *
 *  BY: Ron Marianetti
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	9/3/98 	Re-implement for performance
 *************************************************************/
Err FtrSet(UInt32 creator, UInt16 featureNum, UInt32 newValue)
{
	Err					err;
	FtrTablePtr			ftrTableP;
	FtrFeatureType		newFeature;
	UInt16					temp;
	Int16					pos;
	
	MemSemaphoreReserve(true);
	
	ftrTableP = (FtrTablePtr)GFtrGlobalsP;

	// find features position in table
	pos = PrvFtrBinarySearch(ftrTableP, creator, featureNum);
	
	// if feature already exists, just set value
	if (pos < ftrTableP->numEntries
	&& ftrTableP->features[pos].creator == creator
	&& ftrTableP->features[pos].featureNum == featureNum) {
		err = DmWrite(ftrTableP, (UInt8 *)&ftrTableP->features[pos].value - (UInt8 *)ftrTableP, &newValue, sizeof(UInt32));
		goto Exit;
		}
	
	// Grow feature table if necessary
	ErrNonFatalDisplayIf(ftrTableP->numEntries > ftrTableP->allocEntries, "overran table");
	if (ftrTableP->numEntries == ftrTableP->allocEntries) {
		// resize feature table
		if ((err = PrvFtrPtrResize(ftrTableP, &ftrTableP, 
				MemPtrSize(ftrTableP) + ftrTableSizeIncrement * sizeof(FtrFeatureType))) != 0)
			goto Exit;
		
		// update global to (possible) new pointer
		GFtrGlobalsP = (MemPtr)ftrTableP;
		
		// update alloc size in table
		temp = ftrTableP->allocEntries + ftrTableSizeIncrement;
		if ((err = DmWrite(ftrTableP, OffsetOf(FtrTableType, allocEntries), &temp, 
			sizeof(UInt16))) != 0) goto Exit;
	}

	// increment number of entries
	temp = ftrTableP->numEntries + 1;
	if ((err = DmWrite(ftrTableP, OffsetOf(FtrTableType, numEntries), &temp, 
			sizeof(UInt16))) != 0) goto Exit;
	
	// Shift table up to make room
	if ((err = DmWrite(ftrTableP, (UInt8 *)&ftrTableP->features[pos+1] - (UInt8 *)ftrTableP,
				  &ftrTableP->features[pos], (ftrTableP->numEntries - pos - 1) * 
				  		sizeof (FtrFeatureType))) != 0) goto Exit;
	
	// write new feature into table
	newFeature.creator = creator;
	newFeature.featureNum = featureNum;
	newFeature.value = newValue;
	err = DmWrite(ftrTableP, (UInt8 *)&ftrTableP->features[pos] - (UInt8 *)ftrTableP, &newFeature, sizeof(FtrFeatureType));

Exit:
	MemSemaphoreRelease(true);
	ErrNonFatalDisplayIf(err, "err");
	return err;	
}



/************************************************************
 *
 *  FUNCTION: FtrPtrNew
 *
 *  DESCRIPTION: Create a new block of feature memory
 *
 *  PARAMETERS: 
 *		creator 		- creator type, should be same as the app that owns this
 *									feature
 *		featureNum 	- feature number of the feature
 *		size			- number of bytes to allocate
 *		newPtrP		- return value, the new pointer
 *
 *  RETURNS: pointer to feature memory, or NULL if error
 *
 *  CREATED: 9/2/98
 *
 *  BY: Bob Ebert
 *
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	09/08/99	Allow FtrPtrs to be larger than 64K
 *
 *************************************************************/
Err FtrPtrNew(UInt32 creator, UInt16 featureNum, UInt32 size, void **newPtrP)
{
	Err					err;
	FtrTablePtr 		ftrTableP;
	MemHandle				newH;

	MemSemaphoreReserve(true);
	
	ftrTableP = (FtrTablePtr)GFtrGlobalsP;
	
	// allocate chunk at end of same heap as feature table, (use dmOrphanOwnerID so DmInit cleans up)
	newH = MemChunkNew(ftrTableP->heapID, size, dmOrphanOwnerID | memNewChunkFlagAllowLarge);
	if (newH) {
		// lock the chunk and return a pointer to it
		*newPtrP = MemHandleLock(newH);

		// set up a feature to hold the reference to the (always locked) chunk
		err = FtrSet(creator, featureNum, (UInt32)*newPtrP);
	}
	else {
		// allocation failed; return zero as well as MemoryMgr error
		*newPtrP = 0;
		err = memErrNotEnoughSpace;
	}
	
Exit:
	MemSemaphoreRelease(true);
	
	return err;
}



/************************************************************
 *
 *  FUNCTION: FtrPtrFree
 *
 *  DESCRIPTION: Delete a block of feature memory, and remove feature
 *
 *  PARAMETERS: 
 *		creator 		- creator type, should be same as the app that owns this
 *									feature
 *		featureNum 	- feature number of the feature
 *
 *  RETURNS: error code
 *
 *  CREATED: 9/2/98
 *
 *  BY: Bob Ebert
 *
 *************************************************************/
Err FtrPtrFree(UInt32 creator, UInt16 featureNum)
{
	Err					err;
	void *				ptrP;

	MemSemaphoreReserve(true);
	
	// find the pointer
	if ((err = FtrGet(creator, featureNum, (UInt32 *)&ptrP)) != 0) goto Exit;
	
	if ((err = MemPtrFree(ptrP)) != 0) goto Exit;
		
	// remove the feature
	FtrUnregister(creator, featureNum);

Exit:
	MemSemaphoreRelease(true);
	ErrNonFatalDisplayIf(err, "err");
	return err;
}



/************************************************************
 *
 *  FUNCTION: FtrPtrResize
 *
 *  DESCRIPTION: Resize a block of feature memory
 *
 *  PARAMETERS: 
 *		creator 		- creator type, should be same as the app that owns this
 *									feature
 *		featureNum 	- feature number of the feature
 *		newSize		- the new size in bytes
 *		newPtrP		- return value, possibly moved pointer
 *
 *  RETURNS: error code
 *
 *  CREATED: 9/2/98
 *
 *  BY: Bob Ebert
 *
 *************************************************************/
Err FtrPtrResize(UInt32 creator, UInt16 featureNum, UInt32 newSize, void **newPtrP)
{
	Err					err;
	FtrTablePtr 		ftrTableP;
	
	MemSemaphoreReserve(true);
	
	// find the pointer
	ftrTableP = (FtrTablePtr)GFtrGlobalsP;
	
	if ((err = FtrGet(creator, featureNum, (UInt32 *)newPtrP)) != 0) goto Exit;
	
	if ((err = PrvFtrPtrResize(ftrTableP, newPtrP, newSize)) != 0) goto Exit;
	
	// update feature
	err = FtrSet(creator, featureNum, (UInt32)*newPtrP);

Exit:
	MemSemaphoreRelease(true);
	ErrNonFatalDisplayIf(err, "err");
	return err;
}



/************************************************************
 *
 *  FUNCTION: FtrGetByIndex
 *
 *  DESCRIPTION: Get a feature by index. This routine is normally
 *		only used by shell commands. Most applications will not need it.
 *
 *		For each table (rom,ram) the caller should pass indices starting 
 *		at 0 and incrementing until it gets back ftrErrNoSuchFeature.
 *
 *  PARAMETERS: 
 *		index			- index of feature
 *		romTable 	- if true, index into ROM table, else RAM table
 *		*ftrCreatorP 	- feature creator is returned here
 *		*numP  		- feature number is returned here
 *		*valueP		- feature value is returned here
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 6/21/95
 *
 *  BY: Ron Marianetti
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	9/3/98 	Re-implement for performance
 *************************************************************/
Err		FtrGetByIndex(UInt16 index, Boolean romTable, 
					UInt32 * ftrCreatorP, UInt16 * numP, UInt32 * valueP)
{
	Err				err;
	FtrTablePtr		ftrTableP;
	
	MemSemaphoreReserve(false);
	
	ftrTableP = (FtrTablePtr)GFtrGlobalsP;
	if (!romTable && index < ftrTableP->numEntries) {
		*ftrCreatorP = ftrTableP->features[index].creator;
		*numP = ftrTableP->features[index].featureNum;
		*valueP = ftrTableP->features[index].value;
		err = 0;
	}
	else
		err = ftrErrNoSuchFeature;
		
Exit:
	MemSemaphoreRelease(false);
	ErrNonFatalDisplayIf(err && err != ftrErrNoSuchFeature, "err");
	return err;
}



/************************************************************
 *
 *  FUNCTION: PrvFtrBinarySearch
 *
 *  DESCRIPTION: Binary search in feature for a given
 *					  type/creator/version.
 *
 *  RETURNS: index into feature table of matching elt, or index where
 *				 it would appear if there is no match.  Can return
 *				 ftrTableP->numEntries, to indicate appending to end.
 *
 *  CREATED: 9/3/98 
 *
 *  BY: Bob Ebert
 *
 *************************************************************/
static Int16 PrvFtrBinarySearch (const FtrTablePtr ftrTableP, const UInt32 creator,
									 const UInt16 featureNum)
{
	Int16 lPos = 0;
	Int16 rPos = ftrTableP->numEntries - 1;
	Int16 mPos;
	Int32 result;
	FtrFeaturePtr mFtr;
	
	// keep going until the pointers cross, that way
	// you get the index of smallest (leftmost) elt >= item
	// NOTE: (may return numOfElements to indicate appending at end)
	while (lPos <= rPos)
	{
		mPos = (lPos + rPos) / 2;

		mFtr = &ftrTableP->features[mPos];
		
		// two steps to avoid unsigned/signed errors
		result = mFtr->creator;
		result -= creator;
		if (result == 0) {
			result = mFtr->featureNum;
			result -= featureNum;
		}
		
		if (result < 0) // mid value < search value
			lPos = mPos + 1;
		else
			rPos = mPos - 1;
	}
	return lPos;
}


/************************************************************
 *
 * FUNCTION:		PrvFtrInitFromResource
 *
 * DESCRIPTION:	Initialize the feature table from an 'feat'
 *						resource.
 *
 * PARAMATERS:		ftrTableP	- feature table
 *						resID			- 'feat' resource id.
 *						mustExist	- if true, fail if resource not found.
 * 
 * RETURNS:			nothing
 *
 *	HISTORY:
 *		06/25/99	kwk	New today by Ken Krugler.
 *
 *************************************************************/
static void PrvFtrInitFromResource(	FtrTablePtr ftrTableP,
												UInt16 resID,
												Boolean mustExist)
{
	MemHandle romFtrTableH;
	ROMFtrTablePtr romFtrTableP;
	UInt16 i;
	Int16 pos;

	// Try to load the 'feat' resource.
	romFtrTableH = DmGetResource(sysResTFeatures, resID);
	if (romFtrTableH == NULL) {
		ErrFatalDisplayIf(mustExist, "Missing feature table");
		return;
	}
	
	romFtrTableP = (ROMFtrTableType*)MemHandleLock(romFtrTableH);
	ErrNonFatalDisplayIf(romFtrTableP->numEntries > 1, "Only handles 1 creator table so far");
	
	for (i = 0; i < romFtrTableP->creator[0].numEntries; i++) {
		pos = PrvFtrBinarySearch(	ftrTableP,
											romFtrTableP->creator[0].creator,
											romFtrTableP->creator[0].feature[i].num);
											
		// We need to make space if we found a non-matching entry inside of the able.
		if (pos < ftrTableP->numEntries) {
			if ((ftrTableP->features[pos].creator != romFtrTableP->creator[0].creator)
			|| (ftrTableP->features[pos].featureNum != romFtrTableP->creator[0].feature[i].num)) {
				MemMove(	&ftrTableP->features[pos + 1],
							&ftrTableP->features[pos],
							(ftrTableP->numEntries - pos) * sizeof(FtrFeatureType));
				ftrTableP->numEntries++;
			}
		} else {
			ftrTableP->numEntries++;
		}
		
		ftrTableP->features[pos].creator = romFtrTableP->creator[0].creator;
		ftrTableP->features[pos].featureNum = romFtrTableP->creator[0].feature[i].num;
		ftrTableP->features[pos].value = romFtrTableP->creator[0].feature[i].value;
	}

	MemHandleUnlock(romFtrTableH);
	DmReleaseResource(romFtrTableH);
}


/************************************************************
 *
 *  FUNCTION: PrvFtrPtrResize
 *
 *  DESCRIPTION: Resize a feature pointer.  Can't just use
 *					  MemPtrResize, because chunk is locked, and
 *					  can't just use DmResizeRecord because we
 *					  must preserve at-end-of-heap quality.
 *
 *  PARAMATERS: ftrTableP	- feature table
 *					 ptrP			- pointer to pointer to be resized,
 *									  returns new pointer if relocated
 *					 newSize		- new size of pointer
 * 
 *  RETURNS: error code.
 *
 *  CREATED: 9/3/98 
 *
 *  BY: Bob Ebert
 *
 *************************************************************/
static Err PrvFtrPtrResize(FtrTablePtr ftrTableP, void **ptrP, UInt32 newSize)
{
	Err					err;
	
	// try to resize the pointer in place
	if ((err = MemPtrResize(*ptrP, newSize)) == 0)
		goto Exit;
	
	// if that failed, due to locked chunk or request too large, allocate a new chunk
	// can't just use DmResizeRecord, because we want to maintain "at end of heap"-ness
	if (err == memErrChunkLocked || err == memErrInvalidParam) {
		MemHandle	oldH;
		MemHandle oldP = *ptrP;
		MemHandle newH;

		MemSemaphoreReserve(true);
		
		// recover the MemHandle
		oldH = MemPtrRecoverHandle(oldP);
		
		// allocate chunk at end of same heap feature table is in (use dmOrphanOwnerID so DmInit cleans up)
		newH = MemChunkNew(ftrTableP->heapID, newSize, dmOrphanOwnerID | memNewChunkFlagAllowLarge);
		if (newH == NULL) {
			err = memErrNotEnoughSpace;
			goto Exit;
		}
		
		*ptrP = MemHandleLock(newH);

		// copy memory
		if ((err = DmWrite(*ptrP, 0, oldP, MemPtrSize(oldP))) != 0) goto Exit;
			
		// delete old chunk
		err = MemHandleUnlock(oldH);
		err = MemHandleFree(oldH);

		MemSemaphoreRelease(true);
	}

Exit:
	ErrNonFatalDisplayIf(err, "err");
	return err;
}
