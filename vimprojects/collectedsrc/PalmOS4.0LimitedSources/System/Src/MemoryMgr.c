/******************************************************************************
 *
 * Copyright (c) 1994-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: MemoryMgr.c
 *
 * Release: 
 *
 * Description:
 *		New Memory Manager for Pilot. This version supports heaps of >64K in size. 
 *
 * History:
 *   	10/25/94		RM		- Created by Ron Marianetti
 *		5/3/95		VMK	- Modified MemMove to handle overlapping ranges
 *		11/11/96		RM		- Modified to support >64K heaps
 *		05/31/00		CS		- lEnglish, cUnitedState, etc. are now in PalmLocale.h.
 *
 *****************************************************************************/

// #define USE_RISKY_PERFORMANCE_TRICK

// #undef ERROR_CHECK_LEVEL
// #define ERROR_CHECK_LEVEL ERROR_CHECK_FULL

#define	NON_PORTABLE

#include "PalmOptErrorCheckLevel.h"

#include <PalmTypes.h>

#include <Crc.h>
#include <DataMgr.h>
#include <DebugMgr.h>
#include <ErrorMgr.h>
#include <MemoryMgr.h>
#include <StringMgr.h>
#include <SystemMgr.h>
//#include <Window.h>

#if EMULATION_LEVEL != EMULATION_NONE
#include <PalmLocale.h>	// For lEnglish & cUnitedStates
#include <LocaleMgr.h>	// For LanguageType & CountryType
#endif

#include "HAL.h"

#include "MemoryPrv.h"
#include "SystemPrv.h"
#include "SysTrapsFastPrv.h"

#define 	memFillFreeValue 0x5555

// These values may need to be moved into some header file
#define mstrPtrTblFreeEntryMask 0x00000001
#define mstrPtrTblFirstEntryMask 0x80000001

// The following define enables a debug mode where all MemSet calls are checked to see if they
// were needed or not.  Enable the mode to identify and eliminate unneeded calls to MemSet.
// Basically it just makes sure that at least byte will change values when MemSet is called.
// There's an additional twist in that often new chunks are cleared.  False reports are greatly
// reduced if new chunks are changed to never be completely clear.

//#define CHECK_MEMSET_IS_NEEDED


/********************************************************************
 * Memory Manager Private Routines
 ********************************************************************/
static MemHeapHeaderPtr
		PrvHeapPtrAndID(const void * chunkDataP, UInt16* heapIDP);

static MemHeapHeaderPtr  
		PrvGetHeapPtr(UInt16 heapID, UInt16 * saveDSP);

static MemChunkHeaderPtr
		PrvCreateFreeChunk(MemHeapHeaderPtr heapP, MemChunkHeaderPtr prevFreeChunkP,
								 MemChunkHeaderPtr chunkP, UInt32 size);
									 
static MemChunkHeaderPtr
		PrvUseFreeChunk(MemHeapHeaderPtr heapP, MemChunkHeaderPtr prevFreeChunkP,
							 MemChunkHeaderPtr chunkP);
static Err 
		PrvCompactHeap(MemHeapHeaderPtr heapP);

static MemChunkHeaderPtr  
		PrvFindBiggestMovableChunk(MemChunkHeaderPtr startP, UInt32 maxSize);

static void
		PrvMoveChunk(MemChunkHeaderPtr dstChunkP, MemChunkHeaderPtr srcChunkP);

static CardHeaderPtr 
		PrvGetCardHeaderInfo(UInt16 cardNo, CardInfoPtr* cardInfoPP, Boolean* ramOnlyCardP);
		
static Err 
		PrvInitHeapPtr(MemHeapHeaderPtr heapP, UInt32 size, Int16 numHandles, Boolean zeroData);

static void 
		PrvUnlockAllChunks(UInt16 heapID);

static Err  
		PrvFreeChunkPtr(MemChunkHeaderPtr chunkP);

static void  
		PrvAdjustForNewAddress(UInt16 heapID);


static UInt32	
		PrvCalcStoreCRC(StorageHeaderPtr storeP);

static void 
		ThisShouldGenerateALinkErrorMemoryMgrNew(void);

static UInt16 
		PrvCurOwnerID(void);

static UInt32 
		PrvHostToTargetOffset(UInt32 hostOffset, CardInfoPtr infoP);

static UInt32 
		PrvTargetToHostOffset(UInt32 hostOffset, CardInfoPtr infoP);

static void 
		PrvDebugAction(UInt16 heapID, void * p, MemHandle h, 
				Boolean alwaysScramble, Boolean postModify);

static void
		PrvFillFreeChunks(UInt16 heapID);

static UInt16
		PrvHeapScramble(MemHeapHeaderPtr heapP, UInt32 firstOffset, UInt32 lastOffset);


static void *	
		PrvChunkNew(UInt16 heapID, UInt32 reqSize, UInt16 attributes);

static Err			
		PrvPtrFree(void * p);

static Err		
		PrvPtrResize(void * chunkDataP, UInt32 newSize);


static Err	
		PrvPtrUnlock(const void * p);

static void *	
		PrvHandleCheck(MemHandle	h);
		
static MemChunkHeaderPtr		
		PrvPtrCheck(const void * p);
		



/************************************************************
 *
 *  FUNCTION: MemNumCards 
 *
 *  DESCRIPTION: Returns the number of Memory card slots in the system.
 *   Not all of the slots need to be populated.
 *
 *  PARAMETERS: 
 *
 *  RETURNS: Number of slots
 *
 *  CREATED: 6/15/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt16		MemNumCards(void)
{
	return GMemCardSlots;
}



/************************************************************
 *
 *	FUNCTION:		MemFormatCard
 *
 *  DESCRIPTION:	Initializes a card's RAM area to empty heaps
 *		If the card is an all-RAM card, it also sets up the card's
 *		header.
 *
 *  PARAMETERS: 
 *		cardNo		 	-> either 0 or 1
 *		cardNameP 		-> card header name if it's RAM only card
 *		manufNameP		-> card header manufacturer name if it's RAM only card
 *		ramStoreNameP	-> name to give RAM store
 *
 *	RETURNS:			0 if no error
 *
 *	HISTORY:
 *		10/27/94	ron	Created by Ron Marianetti
 *		08/04/99	kwk	Init some of the nvParam fields w/rom store header values.
 *		09/15/99	kwk	Always set ram store locale to enUS for Simulator builds.
 *		07/23/00	kwk	Set new initCodeOffset3 field to 0.
 *		07/25/00	kwk	Commented out unnecessary set of ramStoreP fields to
 *							0, since the entire structure is cleared initially.
 *
 *************************************************************/
#define	kMaxBlocks		8						// Max # of discontiguous RAM blocks

Err		MemCardFormat(UInt16 cardNo, const Char* cardNameP, 
					const Char* manufNameP, const Char* ramStoreNameP)
{
	UInt16*					wP;
	Int16						i;
	StorageHeaderPtr		ramStoreP;
	UInt32					ramStoreOffset;
	Boolean					ramOnlyCard;
	CardHeaderPtr			cardP;
	UInt16					numBlocks;
	UInt32 *					physBlockInfoP;
	UInt32					logBlockInfo[kMaxBlocks*2];
	

	UInt32					firstHeapSize;
	
	Err						error;
	CardInfoPtr				cardInfoP;

	Err						err;
	UInt16					firstHeapMasterPtrs;
	
#if EMULATION_LEVEL == EMULATION_NONE
	CardInfoPtr				card0InfoP;
	CardHeaderPtr			card0P;
	Boolean					ramOnlyCard0;
	StorageHeaderPtr		romStoreP;
#endif

	// Wait for ownership
	MemSemaphoreReserve(true);

	
	// Get the card info
	cardP = PrvGetCardHeaderInfo(cardNo, &cardInfoP, &ramOnlyCard);
	if (!cardP) {err = -1; goto Exit;}
	

	// Get pointer to the RAM Store structure on this card
	ramStoreP = cardInfoP->ramStoreP;
	ramStoreOffset = MemPtrToLocalID(ramStoreP);
	

	// If it's a RAM only card, init the card header
	if (ramOnlyCard) {
		
		cardP->initStack = 0;
		cardP->resetVector = 0;
		cardP->signature = sysCardSignature;
		cardP->hdrVersion = 1;
		cardP->flags = memCardHeaderFlagRAMOnly;
		StrCopy((Char *)cardP->name, cardNameP);
		StrCopy((Char *)cardP->manuf, manufNameP);
		cardP->version = 1;
		cardP->creationDate = 0x12345678L;
		cardP->numRAMBlocks = 1;
		cardP->blockListOffset = (UInt32)(&cardP->reserved[0]) - (UInt32)(cardInfoP->baseP);
		
		physBlockInfoP = (UInt32 *)(cardInfoP->baseP + cardP->blockListOffset);
		*physBlockInfoP++ = 0;
		*physBlockInfoP++ = cardInfoP->firstRAMBlockSize;
		}



	//------------------------------------------------------------
	// Initialize the RAM Storage Header
	//------------------------------------------------------------
	// Get pointer to the RAM Block list
	numBlocks = cardP->numRAMBlocks;
	if (numBlocks) {
		UInt32		rtcHours, rtcHourMinSecCopy;
		
		// Save some of the non-volatile params
		rtcHours = ramStoreP->nvParams.rtcHours;
		rtcHourMinSecCopy = ramStoreP->nvParams.rtcHourMinSecCopy;
		if (rtcHours < 0x000C67F8 || rtcHours > 0x00111ee8)
			rtcHours = 0x000CD740;		// 1/1/2000
					// 1/1/99 was 0x000CB508; 1/1/98 was 0x000C92D0; 12/1/97 was 0x000C8FE8
	
		// Clear the RAM Storage header
		wP = (UInt16 *)(ramStoreP);
		for (i=0; i<sysStoreHeaderSize/2; i++)
			*wP++ = 0;
	
	
		// Init the RAM Storage header
		ramStoreP->signature = sysStoreSignature;
		ramStoreP->version = 1;
		ramStoreP->flags = memStoreHeaderFlagRAMOnly;
		StrCopy((Char *)ramStoreP->name, ramStoreNameP);
		// ramStoreP->creationDate = 0;
		// ramStoreP->backupDate = 0;
		ramStoreP->heapListOffset = ramStoreOffset + sysStoreHeaderSize;
		// ramStoreP->initCodeOffset1 = 0;
		// ramStoreP->initCodeOffset2 = 0;
		// ramStoreP->initCodeOffset3 = 0;
		// ramStoreP->databaseDirID = 0;
		ramStoreP->rsvSpace = cardInfoP->rsvSpace;
		ramStoreP->dynHeapSpace = cardInfoP->dynHeapSpace;
		ramStoreP->nvParams.rtcHours = rtcHours;			 
		ramStoreP->nvParams.rtcHourMinSecCopy = rtcHourMinSecCopy;			 
		// ramStoreP->nvParams.swrLCDContrastValue = 0;		// unitialized-- code that uses this
		// ramStoreP->nvParams.swrLCDBrightnessValue = 0;  // is required to deal with this...
																		
		
		// Get info about rom store on card 0.
#if EMULATION_LEVEL == EMULATION_NONE
		card0P = PrvGetCardHeaderInfo(0, &card0InfoP, &ramOnlyCard0);
		ErrNonFatalDisplayIf(card0P == NULL, "No header for card 0");
		ErrNonFatalDisplayIf(ramOnlyCard0, "No rom store in card 0");
		
		romStoreP = (StorageHeaderType*)MemLocalIDToPtr(card0InfoP->cardHeaderOffset
							 + sysROMStoreRelOffset, 0);
		
		ramStoreP->nvParams.splashScreenPtr = romStoreP->nvParams.splashScreenPtr;
		ramStoreP->nvParams.hardResetScreenPtr = romStoreP->nvParams.hardResetScreenPtr;
		ramStoreP->nvParams.localeLanguage = romStoreP->nvParams.localeLanguage;
		ramStoreP->nvParams.localeCountry = romStoreP->nvParams.localeCountry;
#else
		// There is no ROM store in the simulator, so just fill in some default values.
		ramStoreP->nvParams.localeLanguage = lEnglish;
		ramStoreP->nvParams.localeCountry = cUnitedStates;
#endif

		// Make copy of cardinfo firstRAMBlockSize so that we can detect
		//  when we're rebooting with a different amount of RAM and consequently
		//  re-format the RAM area. This is probably only applicable during development
		//  when we're occasionally using part of RAM to hold the ROM image. 
		//  ROMSysLaunch compares this field in the store header with it's
		//  calculated RAM size in order to determine if the store header is valid.
		ramStoreP->firstRAMBlockSize = cardInfoP->firstRAMBlockSize;
	
	
		// Create the BlockInfo array for this card by starting with the
		//  block info as defined in the card header, mapping it to
		//  logical addresses, and then removing blocks for the reserve bytes 
		//  at the start of the RAM store.
		// 
		//  NOTE: The HwrGetRAMMapping call always creates
		//  a logical block that starts at the RAM Store Header location. That way, it
		//  is easier to find and skip the store header area when creating the heaps in 
		//  MemStoreInit(). 
		physBlockInfoP = (UInt32 *)MemLocalIDToPtr(cardP->blockListOffset, cardNo);
		HwrGetRAMMapping((void *)cardInfoP, &numBlocks, physBlockInfoP, logBlockInfo);
				
				
		// Remove a chunk from first block for the system reserved area
		logBlockInfo[0] += cardInfoP->rsvSpace;
		logBlockInfo[1] -= cardInfoP->rsvSpace;
		
		
		// Figure out the desired size of the first heap (the dynamic heap if card 0)
		if (cardInfoP->dynHeapSpace)
			firstHeapSize = cardInfoP->dynHeapSpace;
		else
			firstHeapSize = 0;			// use all available space and create 1 heap
			
		
		//------------------------------------------------------------
		// Init the RAM heap list and the RAM heaps. This routine takes a number
		// of arguments:
		//
		//    firstHeapSize 			- size of first heap (special for dynamic heap in card 0)
 		//		firstHeapMstrPtrs 	- A way to change the initial # of master ptrs
 		//										for the first heap. Used for setting the # of master
		//										pts for the dynamic heap differently than for the
 		//										storage heaps.
 		//		cardNo 					- which card number store is in
		//		numBlocks 				- number of memory blocks on card
		//    logBlockInfo			- array of block pointers and block sizes for the RAM area
		//    ramStoreP 				-  MemStoreInit will use this pointer as a designation
		//										 of a NO-HEAP-ZONE.
		//    memMstrPtrTableInitSizeS - - Number of master pointers to pre-allocate in each heap
 		//									after the first heap
		//
		//       
		//------------------------------------------------------------
		if (cardNo == 0) firstHeapMasterPtrs = memMstrPtrTableInitSizeD;
		else firstHeapMasterPtrs = memMstrPtrTableInitSizeS;
		error = MemStoreInit(firstHeapSize, firstHeapMasterPtrs,
					cardNo,  numBlocks,  logBlockInfo,
					ramStoreP, 
					memMstrPtrTableInitSizeS);
					
		// If no error, set the ramStore crc
		if (!error) ramStoreP->crc = PrvCalcStoreCRC(ramStoreP);
		}
	else
		error = memErrNoRAMOnCard;
					
					
Exit:
	// Release ownership
	MemSemaphoreRelease(true);
	return error;
}




/************************************************************
 *
 *  FUNCTION: MemStoreInit
 *
 *  DESCRIPTION: Initializes a memory store on a card by setting up
 *   the heap list given the block list and intializing all the heaps.
 *
 *   This routine assumes that THERE IS A MEMORY BLOCK THAT STARTS AT
 *		storeP. It further assumes that the heapListOffset of the store
 *    header has already been initialized to be the first free byte in 
 *		this memory block after the store header and whatever other data 
 *    may be stored after the store headser. 
 *
 *    Using these assumptions makes it easy to skip the first N bytes of that
 *    block when allocating heaps, where N is the size of the store header 
 *		+ reserve bytes +  heap list (size determined by this routine).
 *
 *   The firstHeapSize parameter is used to create the desired dynamic heap
 *    size when initializing card 0.
 *    
 *
 *  PARAMETERS: 
 *			firstHeapSize 	- A way to change the size of the first heap. Used
 *									for setting the size of the dynamic heap in card 0.
 *									Pass 0 to make the first heap as large as possible.
 *			firstHeapMstrPtrs 	- A way to change the initial # of master ptrs
 *									for the first heap. Used for setting the # of master
 *									pts for the dynamic heap differently than for the
 *									storage heaps.
 *			cardNo 			- which card number store is in
 *			numBlocks 		- number of memory blocks on card
 *			blockInfoP 		- pointer to array of block ptr/block size pairs
 *			storeP   		- pointer to where storage header is on card. No heaps
 * 								will be placed over the storage header
 *			numMstrPtrs    - Number of master pointers to pre-allocate in each heap
 *									after the first heap.
 *
 *  NOTE: If numMstrPtrs is nil, it is assumed that the store is a ROM
 *   store so the read only bit will be set in the heap headers
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 10/27/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err		MemStoreInit(UInt32 firstHeapSize, UInt16 firstHeapMstrPtrs, 
						UInt16 cardNo,  UInt16 numBlocks, UInt32 * blockInfoP, 
						StorageHeaderPtr storeP, UInt16 numMstrPtrs)
{
	UInt8 *					heapStart;
	UInt32						heapSize;
	
	UInt32						blockSize;
	
	UInt16						numHeaps;
	HeapListType*			heapListP;
	
	UInt16						maxHeaps;
	UInt16						saveNumBlocks;
	UInt32 *					saveBlockInfoP;
	
	UInt32						rsvSize;
	CardInfoPtr				infoP;
	
	Boolean					initContents;
	UInt16						masterPtrs;


	// Wait for ownership
	MemSemaphoreReserve(true);

	// Get the card info
	infoP = memCardInfoP(cardNo);

	// Put in default firstHeapSize, if necessary
	if (!firstHeapSize)	firstHeapSize = 0x00FFFFFF;

	//--------------------------------------------------------------
	// Do a pre-flight check to see how many heaps we can make - this
	//  is so we reserve the right amount of space for the heap list
	//   at the front of the first block
	//--------------------------------------------------------------
	maxHeaps = 0;
	saveNumBlocks = numBlocks;
	saveBlockInfoP = blockInfoP;
	
	// DOLATER... in a number of places, this routine tests the block size to contain
	// at least 0x100 bytes;  however, our default master pointer table sizes can be several
	// times greater than that.  If a memory card image contains multiple RAM blocks, the heap
	// could be formatted incorrectly.  vmk 1/2/98

	blockSize = blockInfoP[1];
	while(numBlocks) {
		
		// If not enough room left in this block for another heap, go
		// on to the next block
		if (blockSize <= 0x100) {
			numBlocks--;
			if (numBlocks <= 0) break;
			blockInfoP += 2;
			blockSize = blockInfoP[1];
			}

		// If this is the first heap, allocate the desired amount
		if (maxHeaps == 0)
			heapSize = firstHeapSize;
			
		// Else, allocate the default
		else
			heapSize = blockSize;
			
		// Clip to the available space in the block
		if (heapSize > blockSize) heapSize = blockSize;
			
		blockSize -= heapSize;
		maxHeaps++;
		}
		
	numBlocks = saveNumBlocks;
	blockInfoP = saveBlockInfoP;


	//--------------------------------------------------------------
	// Now, do the real thing and init each heap
	//--------------------------------------------------------------
	heapListP = (HeapListType*)MemLocalIDToPtr(storeP->heapListOffset, cardNo);
	heapListP->numHeaps = numHeaps = 0;
	
	// Figure out how much space we need for the storage header, other
	//  stuff, heapList area.
	rsvSize = (UInt32)heapListP 
					+ sizeof(HeapListType) +  maxHeaps * sizeof(UInt32)
					- (UInt32)storeP;
					
	
	// For the first time through loop, 
	blockSize = 0;

	while(numBlocks || blockSize>0) {
		
		// If not enough room left in this block for another heap, go
		// on to the next block
		if (blockSize <= 0x100) {
			if (numBlocks <= 0) break;
			
			// Get next block info
			heapStart = (UInt8 *)blockInfoP[0];
			blockSize = blockInfoP[1];
			blockInfoP += 2;
			numBlocks--;
			if (blockSize <= 0x100) continue;
			
			// If this block is where the store header is, start the heap
			//  after the store header.
			if (heapStart == (UInt8 *)storeP) {
				heapStart += rsvSize;
				blockSize -= rsvSize;
				}
			}

		// DOLATER... code here assumes that there is going to be only one dynamic heap;
		// code elsewhere in the module tries to support the possibility of more than one
		// dynamic heap.  This is something to be aware of.  vmk 1/2/98

		// If this is the first heap, allocate the desired amount
		initContents = true;
		if (numHeaps == 0) {
			heapSize = firstHeapSize;
			masterPtrs = firstHeapMstrPtrs;
			if (cardNo == 0) initContents = false;
			}
			
		// Else, allocate the default
		else {
			heapSize = blockSize;
			masterPtrs = numMstrPtrs;
			}
			
		// Clip to the available space in the block
		if (heapSize > blockSize) {
			masterPtrs = (UInt16) ((UInt32)masterPtrs * (UInt32)blockSize / (UInt32)heapSize);
			heapSize = blockSize;
			}
			
						
		// Take this heap space out of the block
		blockSize -= heapSize;

		// Store the heap offset in the card's RAM heap list
		heapListP->heapOffset[numHeaps++] = MemPtrToLocalID(heapStart);

		// Init the heap. If it's not the dynamic heap, init the contents too.
		PrvInitHeapPtr((MemHeapHeaderPtr)heapStart, heapSize, masterPtrs, initContents);

		// offset to next heap
		heapStart += heapSize;
		}

	// Store the number of heaps created in the first word of the heap list
	heapListP->numHeaps = numHeaps;


	// Release ownership
	MemSemaphoreRelease(true);

	return 0;
}




/************************************************************
 *
 *  FUNCTION: MemStoreSearch
 *
 *  DESCRIPTION: Searches the given address range for a Memory Store. This
 *   Routine looks at every 4K boundary within the memory range for the start
 *   of a Memory Store and returns the pointer to the store if found. A store
 *   must have a valid signature and CRC check.
 *
 *  PARAMETERS: 
 *			startP 			- address to start looking at
 *			range 			- range of addresses
 *			*storePP 		- return pointer to store, if found
 *
 *  RETURNS: 0 if no error (found)
 *
 *  CREATED: 5/2/95
 *
 *  BY: Ron Marianetti
 *
 *  HISTORY:
 *		 5/ 2/95	ron	Created by Ron Marianetti
 *		 8/27/99	rbb	Reduced block size under emulation
 *		11/24/99	SCL	Zero out *storePP in error case
 *		03/30/00 jmp	Removed emulation-only block size reduction because
 *							we fixed the sysLowMemSize to be correct for the
 *							Simulator.
 *
 *************************************************************/
Err		MemStoreSearch(void * startP, UInt32 range, StorageHeaderPtr* storePP)
{
	const UInt32			blockSize = 0x1000;
	UInt8						* p;
	UInt8						* lastP;
	StorageHeaderPtr		storeP;
	
	// Start at the first boundary
	p = (UInt8 *)startP;
	
	// Figure out the last valid block to check
	lastP = (UInt8 *)startP + range - blockSize;
	
	// Scan for a signature
	for (; p <= lastP; p += blockSize) {
		storeP = (StorageHeaderPtr)p;
		
		// Validate the signature
		if (storeP->signature != sysStoreSignature) continue;
		
		// Validate the checksum
		if (storeP->crc != PrvCalcStoreCRC(storeP)) continue;
		
		// Found!!
		*storePP = storeP;
		return 0;
		}
		
	// Not Found
	*storePP = 0;					// Important! (for ROMSysLaunch)
	return memErrNoStore;
	
}




/************************************************************
 *
 *  FUNCTION: MemInit
 *
 *  DESCRIPTION: Called by the system during Reset. This function
 *		scans both memory cards for heaps and builds the Memory Manager's
 *		heap list. It also initializes the Pilot globals to 0.
 *
 *  PARAMETERS:  
 *		reserveBytes	-> how many bytes to reserve before RAM heap list 
 *								  and heap #0.
 *							   if 0xFFFFFFFF specified, the default value of sysLowMemSize 
 *									will be used.
 *
 *  RETURNS: void
 *
 *  CREATED: 10/27/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err 		MemInit(void)
{
	UInt16						i, numHeaps;
	UInt16					card;
	UInt16					heapID;
	Boolean					ramOnlyCard0 = false;
	CardHeaderPtr			cardP;
	CardInfoPtr				cardInfoP, infoP;
	

#ifdef USE_RISKY_PERFORMANCE_TRICK
	(((LowMemHdrType*)PilotGlobalsP)->globals.sysReserved31DWord1) = 0;
#endif

	//------------------------------------------------------------
	// Make sure the globals have been setup
	//------------------------------------------------------------
	ErrFatalDisplayIf(GMemCardInfoP==0, "MemCardInfoP global not init.");
	
	
	//------------------------------------------------------------
	// Initialize the debug mode.
	//------------------------------------------------------------
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
//	GMemDebugMode = memDebugModeFillFree;
#endif


	//------------------------------------------------------------
	// See if card 0 is a RAM only card (only possible under
	//  emulation mode)
	//------------------------------------------------------------
	// Get the card header
	cardP = PrvGetCardHeaderInfo(0, &cardInfoP, &ramOnlyCard0);
	

	//------------------------------------------------------------
	// Check to make sure our globals didn't grow too much.
	// We reserve more than enough space for our globals so that we
	//  can add more globals in the future without having to re-format 
	//  existing memory cards.
	//------------------------------------------------------------
#if EMULATION_LEVEL != EMULATION_NONE
#ifdef __MWERKS__
#pragma optimization_level 1
#endif
	i = sizeof(LowMemType); // Set a breakpoint here to see what these...
	i = sysLowMemSize;		// ...values are.
#ifdef __MWERKS
#pragma optimization_level reset
#endif
#endif
	if (sizeof(LowMemType) > sysLowMemSize)
		ThisShouldGenerateALinkErrorMemoryMgrNew();

#if ERROR_CHECK_LEVEL <= ERROR_CHECK_PARTIAL
	//------------------------------------------------------------
	// In release builds, make sure we have at least 16 extra bytes
	// for expansion.
	//------------------------------------------------------------
	if (sizeof(LowMemType) + 16 > sysLowMemSize)
		ThisShouldGenerateALinkErrorMemoryMgrNew();
#endif


	//------------------------------------------------------------
	// Create the Global Heap Info table for each card
	//------------------------------------------------------------
	infoP = memCardInfoP(0);
	for (card=0; card<GMemCardSlots; card++) {
		if (infoP->size)  
			MemInitHeapTable(card);
		infoP++;
		}
			

	//------------------------------------------------------------
	// Re-Init the dynamic heaps 
	//------------------------------------------------------------
	for (i=0; i<memDynamicHeaps; i++) {
		MemHeapHeaderPtr heapP;
		
		// Re-initialize the dynamic heap size, in case it got trashed
		//  last time due to some memory trashing error. This is a kind of hack
		//  and only works if there is 1 dynamic heap. If we later support
		//  more than 1 dynamic heap, this will have to be more general.
		infoP = memCardInfoP(0);
		heapP = MemHeapPtr(i);
		heapP->size = infoP->dynHeapSpace;
		MemHeapInit(i, memMstrPtrTableInitSizeD, false);
		}



	//------------------------------------------------------------
	// Go through and unlock all movable chunks in other heaps and
	//  adjust heaps for their new address, in case it changed
	//------------------------------------------------------------
	infoP = memCardInfoP(0);
	for (card = 0; card < GMemCardSlots; card++) {
		if (!infoP->size)  continue;
		
		numHeaps = MemNumRAMHeaps(card);  
		for (i=0; i<numHeaps; i++) {
			heapID = MemHeapID(card, i);
			PrvUnlockAllChunks(heapID);
			PrvAdjustForNewAddress(heapID);
			MemHeapCompact(heapID);
			}
			
		infoP++;
		}


	return 0;


}




/************************************************************
 *
 *  FUNCTION: MemKernelInit
 *
 *  DESCRIPTION: Called by the system after MemInit and after 
 *		the multi-tasking kernel has been installed. This routine will 
 *    setup the semaphore(s) that the Memory Manager uses to share
 *    heaps among tasks.
 *
 *  PARAMETERS:  
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 5/10/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err 		MemKernelInit(void)
{
	UInt32				tag = 'MemM';
	Err 				err;
	
	// Create the Memory Manager semaphore. We use a semaphore so that we 
	// don't get switched out in the middle of modifying a heap
	// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
	if (err) DbgBreak();
	
	return err;
}




/************************************************************
 *
 *  FUNCTION: MemHeapInitTable
 *
 *  DESCRIPTION: Called by the system during Reset. This function
 *		builds the global heap info table up for the given card. This
 *		table contains the address of each heap.
 *
 *  PARAMETERS: card number
 *
 *  RETURNS: void
 *
 *  CREATED: 10/27/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err 		MemInitHeapTable(UInt16 cardNo)
{
	StorageHeaderPtr		storeP;
	
	Boolean					ramOnlyCard;
	HeapListType*			heapListP;
	
	CardHeaderPtr			cardP;
	CardInfoPtr				cardInfoP;
	


	// Get pointer to the card header
	cardP = PrvGetCardHeaderInfo(cardNo, &cardInfoP, &ramOnlyCard);
	if (!cardP) return 0;
	

	// Check the validity of the RAM store header
	storeP = cardInfoP->ramStoreP;
	if (storeP) {
		if (storeP->signature != sysStoreSignature) 
			MemCardFormat(cardNo,  (Char *)"PalmCard", 
						(Char *)"Palm Computing", (Char *)"RAM Store");
		}


	// Get the pointer to the RAM heap list offset table on the card
	heapListP = (HeapListType*)MemLocalIDToPtr(storeP->heapListOffset, cardNo);
	
	
	// Cache the heap info in the card info
	cardInfoP->numRAMHeaps = heapListP->numHeaps;
	cardInfoP->ramHeapOffsetsP = (UInt32 *)&heapListP->heapOffset[0];
	

	//------------------------------------------------------------
	// Append the ROM heaps after the RAM heaps
	//------------------------------------------------------------
	cardInfoP->numROMHeaps = 0;
	if (!ramOnlyCard) {
		storeP = (StorageHeaderPtr)MemLocalIDToPtr(cardInfoP->cardHeaderOffset + sysROMStoreRelOffset, cardNo);
		if (storeP->heapListOffset) {
			heapListP = (HeapListType*)MemLocalIDToPtr(storeP->heapListOffset, cardNo);
			
			cardInfoP->numROMHeaps = heapListP->numHeaps;
			cardInfoP->romHeapOffsetsP = (UInt32 *)&heapListP->heapOffset[0];
			}
		}
			
	return 0;
}




/************************************************************
 *
 *  FUNCTION: MemCardInfo
 *
 *  DESCRIPTION: Returns the info on a card
 *
 *  PARAMETERS: card number, pointers to return variables
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 11/09/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err		MemCardInfo(UInt16 cardNo, Char * nameP, Char * manufNameP,
				UInt16 * versionP, UInt32 * crDateP,
				UInt32 * romSizeP, UInt32 * ramSizeP,
				UInt32 * freeBytesP)
{
	
	// Get the card header pointer
	CardHeaderPtr			cardP;
	Boolean					ramCard;
	StorageHeaderType*	storeP;
	
	HeapListType*			heapListP;
	MemHeapHeaderPtr			heapP;
	UInt32						romSize, ramSize;
	UInt16						i, heapID;
	UInt32						free, max, freeSize, numHeaps;
	
	UInt32 *					blockListP;
	CardInfoPtr				cardInfoP;
	

	// Get card info
	cardP = PrvGetCardHeaderInfo(cardNo, &cardInfoP, &ramCard);
	if (!cardP) 
		return memErrNoCardHeader;
		
	if (cardP->signature != sysCardSignature) 
		return memErrNoCardHeader;
		

	// Get info
	if (nameP)
		StrCopy(nameP, (Char *)cardP->name);
		
	if (manufNameP)
		StrCopy(manufNameP, (Char *)cardP->manuf);
		
	if (versionP)
		*versionP = cardP->version;
		
	if (crDateP)
		*crDateP = cardP->creationDate;
		
	if (romSizeP) {
		if (ramCard) *romSizeP = 0;
		else {
			storeP = (StorageHeaderType*)MemLocalIDToPtr(cardInfoP->cardHeaderOffset
							 + sysROMStoreRelOffset, cardNo);
		
		
			// Start with the card header, store header and RAM block list
			//  which are all stored next to each other starting at the cardHeader
			romSize = storeP->heapListOffset - cardInfoP->cardHeaderOffset;
			heapListP = (HeapListType*)MemLocalIDToPtr(storeP->heapListOffset, cardNo);
			numHeaps = heapListP->numHeaps;
			romSize += sizeof(HeapListType) + (numHeaps-1) * sizeof(UInt32);
		
			for (i=0; i<numHeaps; i++) {
				heapP = (MemHeapHeaderPtr)MemLocalIDToPtr(heapListP->heapOffset[i], cardNo);
				romSize += heapP->size;
				if (!heapP->size) romSize += 0x010000L;
				}
		
			*romSizeP = romSize;
			}
		}
		
		
	// For RAM Size, just add the sizes of every block of RAM
	if (ramSizeP) {
		ramSize = 0;
		blockListP = (UInt32 *)MemLocalIDToPtr(cardP->blockListOffset, cardNo);
		for (i=0; i<cardP->numRAMBlocks; i++) {
			blockListP++;										// skip offset to block
			if (*blockListP != 0xFFFFFFFF)
				ramSize += *blockListP++;
			else
				ramSize += cardInfoP->firstRAMBlockSize;
			}
			
		*ramSizeP = ramSize;
		}
		
		
	// For free bytes, go through each RAM heap and accumulate the free bytes
	if (freeBytesP) {
		freeSize = 0;
		numHeaps = cardInfoP->numRAMHeaps;
		for (i=0; i<numHeaps; i++) {
			heapID = MemHeapID(cardNo, i);
			MemHeapFreeBytes(heapID, &free, &max);
			freeSize += free;
			}
			
		*freeBytesP = freeSize;
		}
			
	return 0;	
}




/************************************************************
 *
 *  FUNCTION: MemStoreInfo
 *
 *  DESCRIPTION: Returns the store information for either the
 *		ROM or RAM store on a memory card
 *
 *  PARAMETERS: card number, store number (0:ROM, 1:RAM), 
 *		references to return info variables any of which can be nil.
 *
 *  RETURNS: version number
 *
 *	HISTORY:
 *		11/09/94	RM		Created by Ron Marianetti
 *		10/28/99	kwk	Return new memErrROMOnlyCard result.
 *
 *************************************************************/
Err		MemStoreInfo(UInt16 cardNo, UInt16 storeNumber,
					UInt16 * versionP, UInt16 * flagsP, Char * nameP,
					UInt32 *	crDateP,  UInt32 * bckUpDateP,
					UInt32 *	heapListOffsetP, UInt32 * initCodeOffset1P,
					UInt32 * initCodeOffset2P, LocalID*	databaseDirIDP)
{
	Boolean				ramCard;
	StorageHeaderPtr	storeP;
	CardHeaderPtr		cardP;
	CardInfoPtr			cardInfoP;


	// Check bounds on the card number
	if (cardNo >= GMemCardSlots) return memErrCardNotPresent;
	

	//------------------------------------------------------------
	// Get the card Info.
	//------------------------------------------------------------
	cardP = PrvGetCardHeaderInfo(cardNo, &cardInfoP, &ramCard);
	
	// If it's a RAM only card, can't return info on ROM
	if (ramCard && (storeNumber == memRomStoreNum)) 
		return memErrRAMOnlyCard;
		
	// Get the offset to the store
	if (storeNumber == memRomStoreNum) 
		storeP = (StorageHeaderType*)MemLocalIDToPtr(cardInfoP->cardHeaderOffset
						 + sysROMStoreRelOffset, cardNo);
	else if (storeNumber == memRamStoreNum) {
		storeP = cardInfoP->ramStoreP;
		if (storeP == NULL)
			return memErrROMOnlyCard;
		}
	else
		return memErrInvalidStoreHeader;

	if (storeP->signature != sysStoreSignature)
		return memErrInvalidStoreHeader;


	// Return the info
	if (versionP) *versionP = storeP->version;
	if (flagsP) *flagsP = storeP->flags;
	if (nameP) StrCopy(nameP, (Char *)storeP->name);
	if (crDateP) *crDateP = storeP->creationDate;
	if (bckUpDateP) *bckUpDateP = storeP->backupDate;
	if (heapListOffsetP) *heapListOffsetP = storeP->heapListOffset;
	if (initCodeOffset1P) *initCodeOffset1P = storeP->initCodeOffset1;
	if (initCodeOffset2P) *initCodeOffset2P = storeP->initCodeOffset2;
	if (databaseDirIDP) 
		*databaseDirIDP = storeP->databaseDirID;

	return 0;
}




/************************************************************
 *
 *  FUNCTION: MemStoreSetInfo
 *
 *  DESCRIPTION: Sets the store information for the
 *		RAM store on a memory card
 *
 *  PARAMETERS: card number,  
 *		references to new info variables - any of which could be nil ptrs
 *
 *  RETURNS: version number
 *
 *  CREATED: 11/09/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err			MemStoreSetInfo(UInt16 cardNo, UInt16 storeNumber,
					UInt16 * versionP, UInt16 * flagsP,  Char * nameP, 
					UInt32 * crDateP, UInt32 * bckUpDateP, 
					UInt32 * heapListOffsetP, UInt32 * initCodeOffset1P,
					UInt32 * initCodeOffset2P, LocalID*	databaseDirIDP)
{
	Boolean				ramCard;
	StorageHeaderPtr	storeP;
	CardHeaderPtr		cardP;
	CardInfoPtr			cardInfoP;

	if (storeNumber == memRomStoreNum) return memErrWriteProtect;

	//------------------------------------------------------------
	// Get the card Info.
	//------------------------------------------------------------
	cardP = PrvGetCardHeaderInfo(cardNo, &cardInfoP, &ramCard);
	
	storeP = cardInfoP->ramStoreP;
	if (!storeP) return memErrNoRAMOnCard;
	
	if (storeP->signature != sysStoreSignature) 
		return memErrInvalidStoreHeader;


	// Wait for ownership
	MemSemaphoreReserve(true);


	// Set the info
	if (versionP)
		storeP->version = *versionP;
		
	if (flagsP) 
		storeP->flags = *flagsP;

	if (nameP)
		StrCopy((Char *)storeP->name, nameP);

	if (crDateP)
		storeP->creationDate = *crDateP;

	if (bckUpDateP)
		storeP->backupDate = *bckUpDateP;

	if (heapListOffsetP)
		storeP->heapListOffset = *heapListOffsetP;

	if (initCodeOffset1P)
		storeP->initCodeOffset1 = *initCodeOffset1P;

	if (initCodeOffset2P)
		storeP->initCodeOffset2 = *initCodeOffset2P;

	if (databaseDirIDP)
		storeP->databaseDirID = *databaseDirIDP;
		
		
		
	// Recalculate the crc check on this store and save it
	storeP->crc = PrvCalcStoreCRC(storeP);
	
	
	// Release ownership
	MemSemaphoreRelease(true);

	return 0;
}




/************************************************************
 *
 *	FUNCTION:		MemNVParamInfo
 *
 *	DESCRIPTION: Used internally by other System code in order to
 *   get and set the System Non-volatile parameters. These parameters include
 *   the hours since Jan,1904 used for tracking the current
 *	  time and any other information that is inconvenient to store in
 *	  a database because it must be accessed or updated at interrupt time.
 *
 *	  The Non-volatile params are stored in the RAM store header on card #0
 *
 *   If called with set=false, this routine simply returns a copy of the current
 *	  params in *paramsP. If called with set=true, this routine
 *	  will change the current params to match what's in *paramsP.
 *		
 *		If set=false and the card 0 RAM store header is invalid, then
 *		<paramsP> will be filled in with data from the rom store, but
 *		memErrNoRAMOnCard or memErrInvalidStoreHeader.
 *		
 *		WARNING:
 *		Since MemNVParams(true,...) disables write protection on the storage heap
 *		without reserving the memory semaphore, and because it can be called at
 *		interrupt time, you must disable interrupts when setting the NV Params.
 *		The caller should do this during the entire process of changing an NV param,
 *		so that interrupts are off when they initially get the NVParams, change the 
 *		field of interest, and then write the changes back.  Otherwise you run the 
 *		risk of writing stale data (if something else changed the NV params in between
 *		your get/set calls).
 *		
 *  PARAMETERS: 
 *		set	- if true, params will be changed according to *paramsP
 *				  if false, current params will be returned in *paramsP
 *		*paramsP - new params if set is true, returned params if set is false.
 *
 *	RETURNS:				0 if no error.
 *
 *	HISTORY:
 *		09/04/95	ron	Created by Ron Marianetti
 *		08/04/99	kwk	Always fill in <paramsP> with rom store
 *							info, so that if we can't find the ram store
 *							we're returning default values. Also do
 *		08/09/99 tlw	Fixed this to only set paramsP if set is false
 *							(otherwise it blows away the data passed in)
 *							and only do it if there is an err (faster that way)
 *
 *************************************************************/
Err			MemNVParams(Boolean set, SysNVParamsPtr paramsP)
{
	Boolean				ramCard;
	Err					err = 0;
	StorageHeaderPtr	storeP;
	CardHeaderPtr		cardP;
	SysNVParamsPtr		dataP;
	CardInfoPtr			cardInfoP;

	ErrNonFatalDisplayIf(paramsP == NULL, "NULL parameter");
	
	cardP = PrvGetCardHeaderInfo(0, &cardInfoP, &ramCard);
#if EMULATION_LEVEL == EMULATION_NONE
	ErrNonFatalDisplayIf(ramCard, "No ROM store on card 0");
#endif

	//------------------------------------------------------------
	// Get the storage header for the RAM store on card 0.
	//------------------------------------------------------------
	storeP = cardInfoP->ramStoreP;
	if (!storeP) {
		err = memErrNoRAMOnCard;
		goto ErrExit;
		}
	
	if ((storeP->signature != sysStoreSignature) 
		|| (storeP->crc != PrvCalcStoreCRC(storeP))) {
		err = memErrInvalidStoreHeader;
		goto ErrExit;
		}
	
	// Get ptr to NV Params
	dataP = &storeP->nvParams;
	
	
	// If setting, enable writes, set the data, and update the CRC
	if (set) {
		Boolean		restoreProtection;
		
		// Set new params
		restoreProtection = HwrEnableDataWrites();
		MemMove(dataP, paramsP, sizeof(SysNVParamsType));
		storeP->crc = PrvCalcStoreCRC(storeP);
		if (restoreProtection) HwrDisableDataWrites();
		}
		
	// If getting, copy data to caller's buffer
	else {
		MemMove(paramsP, dataP, sizeof(SysNVParamsType));

#if EMULATION_LEVEL == EMULATION_NONE
// DOLATER - Technically this is more correct, but at one point it didn't seem to work...
// 			Could put a DbgBreak here to see if it actually does the right thing or not.
//				The expected value of storeP is 0x10C08000 (especially on a '328 device).
//		storeP = (StorageHeaderType*)MemLocalIDToPtr(cardInfoP->cardHeaderOffset
//								 + sysROMStoreRelOffset, 0);
		storeP = (StorageHeaderType*)(cardInfoP->baseP +
													cardInfoP->cardHeaderOffset +
													sysROMStoreRelOffset);
#endif
		}

	return 0;


ErrExit:
	// If we're exiting with an error and we were called to 'get'
	// the NVParam info, then use 'default' data from ROM
	// This was originally for the splash screen pointers, but the
	// international manager needs default locale data too...
	if (!set) {
#if EMULATION_LEVEL == EMULATION_NONE
// DOLATER - Technically this is more correct, but at one point it didn't seem to work...
// 			Could put a DbgBreak here to see if it actually does the right thing or not.
//				The expected value of storeP is 0x10C08000 (especially on a '328 device).
//		storeP = (StorageHeaderType*)MemLocalIDToPtr(cardInfoP->cardHeaderOffset
//								 + sysROMStoreRelOffset, 0);
		storeP = (StorageHeaderType*)(cardInfoP->baseP +
													cardInfoP->cardHeaderOffset +
													sysROMStoreRelOffset);
		MemMove(paramsP, &storeP->nvParams, sizeof(SysNVParamsType));
#else
		MemSet(paramsP, sizeof(SysNVParamsType), 0);
#endif
		}
		
	return err;
}




/************************************************************
 *
 *  FUNCTION: MemNumHeaps
 *
 *  DESCRIPTION: Returns the number of heaps in the given card
 *
 *  PARAMETERS: card number
 *
 *  RETURNS: number of heaps
 *
 *  CREATED: 10/28/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt16		MemNumHeaps(UInt16 cardNo)
{
	CardInfoPtr	infoP;
	
	ErrFatalDisplayIf(cardNo >= GMemCardSlots, "Invalid Card #");
	
	// Optimized for 2 cards only
	infoP = memCardInfoP(0);
	if (cardNo == 1)
		infoP++;
		
	return infoP->numRAMHeaps + infoP->numROMHeaps;
		
}




/************************************************************
 *
 *  FUNCTION: MemNumRAMHeaps
 *
 *  DESCRIPTION: Returns the number of RAM heaps in the given card
 *
 *  PARAMETERS: card number
 *
 *  RETURNS: number of RAM heaps
 *
 *  CREATED: 10/28/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt16		MemNumRAMHeaps(UInt16 cardNo)
{
	CardInfoPtr	infoP;
	
	ErrFatalDisplayIf(cardNo >= GMemCardSlots, "Invalid Card #");
	
	// Optimized for 2 cards only
	infoP = memCardInfoP(0);
	if (cardNo == 1)
		infoP++;
		
	return infoP->numRAMHeaps ;
		
}




/************************************************************
 *
 *  FUNCTION: MemHeapID
 *
 *  DESCRIPTION: Returns the heap ID for a given card number and
 *		heap index.
 *
 *  PARAMETERS: card number, heap number
 *
 *  RETURNS: heapID
 *
 *  CREATED: 10/28/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt16 	MemHeapID(UInt16 cardNo, UInt16 heapNo)
{

	ErrFatalDisplayIf(cardNo >= GMemCardSlots, "Invalid Card #");
	ErrFatalDisplayIf(heapNo >= MemNumHeaps(cardNo), "Invalid heap #");

	if (cardNo == 0) return heapNo;
	else return heapNo | 0x8000;
}




/************************************************************
 *
 *  FUNCTION: MemHeapDynamic
 *
 *  DESCRIPTION: Returns true if the given heap is a dynamic heap
 *
 *  PARAMETERS: heapID
 *
 *  RETURNS: 
 *		true if dynamic, false if not.
 *
 *  CREATED: 8/1/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Boolean 		MemHeapDynamic(UInt16 heapID)
{
	// This logic assumes heapID = heapIndex for card 0 and that
	//  all dynamic heaps are in card 0.
	if (heapID < memDynamicHeaps) return true;
	return false;
} 




/************************************************************
 *
 *  FUNCTION: MemHeapPtr
 *
 *  DESCRIPTION: Returns Pointer to heap given heap ID
 *
 *  PARAMETERS: card number, heap number
 *
 *  RETURNS: Heap Pointer, or nil
 *
 *  CREATED: 10/28/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
void * MemHeapPtr(UInt16 heapID)
{
	UInt16						heapNo;
	CardInfoPtr				infoP;
	#if EMULATION_LEVEL != EMULATION_NONE
	UInt32						offset;
	#endif

	heapNo = heapID & 0x7FFF;


	// Get the card Info pointer
	if (heapID & 0x8000) {
		ErrFatalDisplayIf(hwrNumCardSlots < 2, "Invalid Card #");
		infoP = memCardInfoP(1);
		}
	else
		infoP = memCardInfoP(0);
		
		
	//---------------------------------------------------------------
	// Under Emulation mode, we use the slower method which works for
	//   Memory card images with shifted ROM locations
	//---------------------------------------------------------------
	#if EMULATION_LEVEL != EMULATION_NONE
	
	// See which heap it is
	if (heapNo >= infoP->numRAMHeaps) {
		heapNo -= infoP->numRAMHeaps;
		ErrFatalDisplayIf(heapNo >= infoP->numROMHeaps, "Invalid heapID");
		offset = infoP->romHeapOffsetsP[heapNo];
		}
	else 
		offset = infoP->ramHeapOffsetsP[heapNo];
		
	offset = PrvTargetToHostOffset(offset, infoP);
	return (MemHeapHeaderPtr)(infoP->baseP + offset);
	
	
	#else
	//---------------------------------------------------------------
	// Under native mode, the pointer is simple the base address of the card
	//  + the heap offset.
	//---------------------------------------------------------------
	if (heapNo >= infoP->numRAMHeaps) {
		heapNo -= infoP->numRAMHeaps;
		ErrFatalDisplayIf(heapNo >= infoP->numROMHeaps, "Invalid heapID");
		return (MemHeapHeaderPtr)(infoP->baseP + infoP->romHeapOffsetsP[heapNo]);
		}
	else 
		// This works on a 328 because for some reason ramHeapOffsetsP[0] was set to 0xF0001800
		// (I suppose this is the result of an overflowed calculation somewhere).
		// Whatever the reason, the right result is returned.
		return (MemHeapHeaderPtr)(infoP->baseP + infoP->ramHeapOffsetsP[heapNo]);

	#endif	
	
		
} 




/************************************************************
 *
 *  FUNCTION: MemHeapFreeBytes
 *
 *  DESCRIPTION: Computes the total amount of free space in a heap
 *
 *  PARAMETERS: heapID, reference to free bytes, largest free chunk
 *
 *  RETURNS: non-zero if error
 *
 *  CREATED: 11/09/94 
 *
 *  BY: Ron Marianetti
 *		7/24/98 - Bob Ebert - Updated to use free list for performance
 *		10/20/98 - Bob Ebert - Returns 0/0 for read only heaps
 *
 *************************************************************/
Err 		MemHeapFreeBytes(UInt16 heapID, UInt32 * freeP, UInt32 * maxP)
{
	UInt32							freeBytes = 0;
	MemHeapHeaderPtr		   	heapP;
	MemChunkHeaderPtr				chunkP;
	UInt32							maxSize=0;
	UInt32							size;

	// Error checking
	if (freeP == NULL || maxP == NULL)
		{
		ErrNonFatalDisplay("Passing NULL to MemHeapFreeBytes");
		return memErrInvalidParam;
		}

	// Wait for ownership
	MemSemaphoreReserve(false);

	// Get a near heap pointer from the heap ID 
	heapP = MemHeapPtr(heapID);

	// <10/20/98 SCL> Removed this test after fixing it by moving the "Exit" label
	// to right before we set the return values (which were initialized to zero above).
	// Adding this test actually breaks our own MakeCard ROM-building tool.
	// If we want to turn this back on, we should put a conditional around it so
	// we can turn it off with a "-d" in MakeCard's Makefile, as demonstrated.
	// However, this still means that we're going to lie to everyone else and say
	// that there's zero bytes free just because it's a ROM heap.  I'm not sure this
	// is the best thing to do...
//#if !defined(ALLOW_MEMHEAPFREEBYTES_IN_ROM_HEAPS)
	// If it's read only, return zero bytes
//	if (heapP->flags & memHeapFlagReadOnly) {
//		goto Exit;
//		}
//#endif

	// Start at the first free chunk
	chunkP = (MemChunkHeaderPtr)((UInt16 *)heapP + heapP->firstFreeChunkOffset);

	// Walk the free list and accumulate sizes of chunks
	while((size = chunkP->size) != 0) {

		freeBytes += size;
		if (size - sizeof(MemChunkHeaderType) > maxSize)
			maxSize = size - sizeof(MemChunkHeaderType);

		// Go on to next free chunk
		chunkP = (MemChunkHeaderPtr)((UInt16 *)chunkP + chunkP->hOffset);
		}

Exit:
	// Set return values
	*freeP = freeBytes;
	*maxP = maxSize;

	// Release ownership
	MemSemaphoreRelease(false);

	return 0;
}




/************************************************************
 *
 *  FUNCTION: MemHeapSize
 *
 *  DESCRIPTION: Returns the total size of the heap
 *
 *  PARAMETERS: heapID 
 *
 *  RETURNS: size, or 0 if error
 *
 *  CREATED: 11/10/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt32 	MemHeapSize(UInt16 heapID)
{
	UInt32							size = 0;
	MemHeapHeaderPtr		   	heapP;


	// Get a near heap pointer from the heap ID
	heapP = MemHeapPtr(heapID);


	size = heapP->size;
	if (!size) size = 0x010000L;
		  

	// exit
	return size;
}




/************************************************************
 *
 *  FUNCTION: MemHeapFlags
 *
 *  DESCRIPTION: Returns the heap flags
 *
 *  PARAMETERS: heapID 
 *
 *  RETURNS: flags
 *
 *  CREATED: 11/10/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt16 	MemHeapFlags(UInt16 heapID)
{
	UInt16								flags;
	MemHeapHeaderPtr		   		heapP;



	// Get a near heap pointer from the heap ID
	heapP = MemHeapPtr(heapID);


	flags = heapP->flags;
		  
	// return
	return flags;
}




/************************************************************
 *
 *  FUNCTION: MemHeapCompact
 *
 *  DESCRIPTION: Compacts a heap 
 *
 *  PARAMETERS: heap ID
 *
 *  RETURNS:
 *
 *  CREATED: 11/03/94 
 *
 *  BY: Ron Marianetti
 *		10/20/98 - Bob Ebert - Skip compaction of read only heaps
 *
 *************************************************************/
Err 		MemHeapCompact(UInt16 heapID)
{
	MemHeapHeaderPtr	heapP;
	Err				err;


	// Wait for ownership
	MemSemaphoreReserve(true);

	// Error Checking
	if (GMemDebugMode) PrvDebugAction(heapID, 0, 0, true, false);

	// Get a near heap pointer from the heap ID
	heapP = MemHeapPtr(heapID);

	// If it's read only, don't do anything
	if (heapP->flags & memHeapFlagReadOnly) {
		err = 0;
		goto Exit;
		}

	err = PrvCompactHeap(heapP);

	// Error Checking
	if (GMemDebugMode) PrvDebugAction(heapID, 0, 0, false, true);

Exit:
	// Release ownership
	MemSemaphoreRelease(true);

	return err;
}




/************************************************************
 *
 *  FUNCTION: MemInitHeap
 *
 *  DESCRIPTION: Re-initializes a heap and accepts parameter
 *		for the number of offsets to reserve for movable chunks
 *
 *  PARAMETERS: heapID, number of offsets to reserve or -1
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 11/18/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err	MemHeapInit(UInt16 heapID, Int16 numHandles, Boolean initContents)
{
	MemHeapHeaderPtr	heapP;
	UInt32				size;
	Err				err;


	// Wait for ownership
	MemSemaphoreReserve(true);

	// Get a pointer to the heap
	heapP = MemHeapPtr(heapID);

	// if no numHandles, take existing value
	if (numHandles < 0) numHandles = heapP->mstrPtrTbl.numEntries;

	// Use the same size
	size = heapP->size;
	if (size == 0) size = 0x10000L;

	// Init it
	err = PrvInitHeapPtr(heapP, size, numHandles, initContents);

	// Error Checking
	if (GMemDebugMode) PrvDebugAction(heapID, 0, 0, false, true);

	// Release ownership
	MemSemaphoreRelease(true);

	return err;
}




/************************************************************
 *
 *  FUNCTION: MemHeapFreeByOwnerID
 *
 *  DESCRIPTION: Scans the heap and frees all chunks with the
 *   given owner ID.
 *
 *  PARAMETERS: heapID, owner ID
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 4/13/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err  MemHeapFreeByOwnerID(UInt16 heapID, UInt16 ownerID)
{
	MemHeapHeaderPtr		heapP;
	MemChunkHeaderPtr		chunkP;
	Err					err;
	UInt32					size;

	// Wait for ownership
	MemSemaphoreReserve(true);


	// Get a near heap pointer from the heapID
	heapP = MemHeapPtr(heapID);

	// Walk the heap and look for chunks with the given owner ID
	chunkP = (MemChunkHeaderPtr)((UInt8 *)heapP + sizeof(MemHeapHeaderType)
					+ (heapP->mstrPtrTbl.numEntries << 2) );

	while((size = chunkP->size) != 0) {
		if (chunkP->owner == ownerID)
			MemChunkFree(memChunkData(chunkP));

		// Go on to next chunk
		chunkP = (MemChunkHeaderPtr) ((UInt8 *)chunkP + size);
		}
		
		
	// Compact the heap
	err = MemHeapCompact(heapID);


	// Release ownership
	MemSemaphoreRelease(true);

	return err;
}




/************************************************************
 *
 *  FUNCTION: MemChunkNew
 *
 *  DESCRIPTION: Allocates a new chunk in the given heap
 *
 *  PARAMETERS: heapID, size of chunk, chunk attribytes
 *		The chunk attributes can be one or more of:
 *			memNewChunkFlagNonMovable
 *			memNewChunkFlagParAlign
 *			ORed with the owner ID
 *			
 *
 *  RETURNS: ChunkID, or 0 if error
 *
 *  CREATED: 10/25/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
void *	MemChunkNew(UInt16 heapID, UInt32 reqSize, UInt16 attributes)
{
	void *						retP;


	// Wait for ownership
	MemSemaphoreReserve(true);

	// Debug mode
	if (GMemDebugMode) PrvDebugAction(heapID, 0, 0, true, false);
	
	// Call guts
	retP = PrvChunkNew(heapID, reqSize, attributes);

	// Debug mode
	if (GMemDebugMode) PrvDebugAction(heapID, 0, 0, false, true);
	
	// Release ownership
	MemSemaphoreRelease(true);


	// If we're really allocating a handle and not a ptr or a pre-locked movable
	// chunk, then protect the handle.
	if (retP && ((attributes & (memNewChunkFlagNonMovable | memNewChunkFlagPreLock)) == 0))
		return memHandleProtect(retP);
		
	else
		return retP;
}




/************************************************************
 *
 *  FUNCTION: MemChunkFree
 *
 *  DESCRIPTION: Disposes of a chunk
 *
 *  PARAMETERS: Chunk ID
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 11/02/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err		MemChunkFree(void * p)
{
	Err			err;


	// Check for error
	if (! PrvPtrCheck(p)) 
		return memErrInvalidParam;


	// Wait for ownership
	MemSemaphoreReserve(true);

	// Error Checking
	if (GMemDebugMode) {
		UInt16	heapID = MemPtrHeapID(p);
		PrvDebugAction(heapID, 0, 0, true, false);
	
		// Call guts
		err = PrvPtrFree(p);
		
		// Error Checking
		PrvDebugAction(heapID, 0, 0, false, true);
		}
		
	// Normal...
	else
		err = PrvPtrFree(p);
	
	// Release ownership
	MemSemaphoreRelease(true);
	
	return err;

}




/************************************************************
 *
 *  FUNCTION: MemPtrNew
 *
 *  DESCRIPTION: Allocates a non-movable chunk in the dynamic heap
 *
 *  PARAMETERS: requested size of chunk
 *
 *  RETURNS: Ptr to chunk, or 0 if error
 *
 *  CREATED: 11/17/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
void *	MemPtrNew(UInt32 size)
{
	void *						retP;
	UInt16 attributes;


	// Wait for ownership
	MemSemaphoreReserve(false);

	attributes = memNewChunkFlagNonMovable | PrvCurOwnerID();
	
	// Error Checking
	if (GMemDebugMode) {
		PrvDebugAction(0, 0, 0, true, false);

		// Call guts
		retP = PrvChunkNew(0, size, attributes);
		
		// Error Checking
		PrvDebugAction(0, 0, 0, false, true);
		}
		
	// Normal..
	else 
		retP = PrvChunkNew(0, size, attributes);


	// Release ownership
	MemSemaphoreRelease(false);
	
	return retP;
}




/************************************************************
 *
 *  FUNCTION: MemPtrRecoverHandle
 *
 *  DESCRIPTION: recovers the handle to a movable chunk given the Ptr
 *
 *  PARAMETERS: pointer to chunk data
 *
 *  RETURNS: handle, or 0 if error
 *
 *  CREATED: 11/10/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
MemHandle	MemPtrRecoverHandle(void * chunkDataP)
{
	MemChunkHeaderPtr	chunkP;
	MemHandle				ptrP;
	Int32					hOffset;

	// Check for errors and get chunk header
	if (! (chunkP = PrvPtrCheck(chunkDataP))) 
		return memHandleProtect(0);

	// Error Checking
	if (GMemDebugMode) PrvDebugAction(0, chunkDataP, 0, false, false);

	// Get it's handle
	if ((hOffset = chunkP->hOffset) != 0 && !chunkP->free)  {
		ptrP = (MemHandle)((UInt32)chunkP - (hOffset << 1));
		}
	else {
		ErrFatalDisplay("Not a handle");
		ptrP = 0;
		}

	return  memHandleProtect(ptrP);
}




/************************************************************
 *
 *  FUNCTION: MemPtrFlags
 *
 *  DESCRIPTION: Returns the flags for a chunk given the chunk data Ptr
 *
 *  PARAMETERS: chunkID
 *
 *  RETURNS: chunk flags
 *
 *  CREATED: 10/25/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt16		MemPtrFlags(void * chunkDataP)
{
	MemChunkHeaderPtr				chunkP;
	UInt16							flags;

	// Check for error
	if (! (chunkP = PrvPtrCheck(chunkDataP))) return 0;

	// Heap Validity
	if (GMemDebugMode) PrvDebugAction(0, chunkDataP, 0, false, false);

	// Get the flags
	flags = memChunkFlags(chunkP);


	return flags;
} 




/************************************************************
 *
 *  FUNCTION: MemPtrSize
 *
 *  DESCRIPTION: Returns the requested size of a chunk
 *
 *  PARAMETERS: chunk data pointer
 *
 *  RETURNS: chunk size
 *
 *  CREATED: 10/25/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt32		MemPtrSize(void * chunkDataP)
{
	MemChunkHeaderPtr				chunkP;
	UInt32							size;


	// Check for error
	if (! (chunkP = PrvPtrCheck(chunkDataP))) return 0;

	// Heap Validity
	if (GMemDebugMode) PrvDebugAction(0, chunkDataP, 0, false, false);

	// Get the size
	size = chunkP->size - (chunkP->sizeAdj) - sizeof(MemChunkHeaderType);

	return size;
} 




/************************************************************
 *
 *  FUNCTION: MemPtrOwner
 *
 *  DESCRIPTION: Returns the owner ID of a chunk
 *
 *  PARAMETERS: chunk data pointer
 *
 *  RETURNS: owner ID
 *
 *  CREATED: 10/25/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt16		MemPtrOwner(void * chunkDataP)
{
	MemChunkHeaderPtr				chunkP;
	UInt16							owner;

	// Check for error
	if (! (chunkP = PrvPtrCheck(chunkDataP))) return 0;

	// Heap Validity
	if (GMemDebugMode) PrvDebugAction(0, chunkDataP, 0, false, false);

	// Get the owner
	owner = chunkP->owner;

	return owner;
} 




/************************************************************
 *
 *  FUNCTION: MemPtrHeapID
 *
 *  DESCRIPTION: Returns the heapID of a chunk
 *
 *  PARAMETERS: chunk data pointer
 *
 *  RETURNS: heap ID
 *
 *  CREATED: 10/25/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt16		MemPtrHeapID(void * chunkDataP)
{
	MemChunkHeaderPtr		chunkP;
	UInt16					heapID;
	
	// Check for error
	if (! (chunkP = PrvPtrCheck(chunkDataP))) return 0;

	PrvHeapPtrAndID(chunkDataP, &heapID);

	return heapID;
} 




/************************************************************
 *
 *  FUNCTION: MemPtrDataStorage
 *
 *  DESCRIPTION: Returns true if the given pointer is part of a 
 *   Data Storage Heap, else it is a pointer in the dynamic heap.
 *
 *  PARAMETERS: chunk data pointer
 *
 *  RETURNS: true if part of a data storage heap
 *
 *  CALLED BY: Fields package to determine if it needs to worry
 *    about data storage write-protection when editing a text field.
 *
 *  CREATED: 5/8/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Boolean 	MemPtrDataStorage(void * chunkDataP)
{
	MemChunkHeaderPtr		chunkP;
	UInt16					heapID;
	
	// Check for error
	if (! (chunkP = PrvPtrCheck(chunkDataP))) return false;

	// Get the heap ID
	PrvHeapPtrAndID(chunkDataP, &heapID);

	return (heapID != 0);
} 




/************************************************************
 *
 *  FUNCTION: MemPtrCardNo
 *
 *  DESCRIPTION: Returns the card number given a pointer to a chunk
 *
 *  PARAMETERS: chunk ID
 *
 *  RETURNS: card number
 *
 *  CREATED: 11/16/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt16	MemPtrCardNo(void * p)
{
	CardInfoPtr	infoP;
	UInt16			i;


	// Search in each card
	infoP = memCardInfoP(0);
	for (i=0; i<GMemCardSlots; i++) {
		if (((UInt32)p < infoP->dynHeapSpace + infoP->rsvSpace) ||
				((UInt32)p > (UInt32)infoP->baseP  &&  (UInt32)p < (UInt32)infoP->baseP + infoP->size)) {
			// Heap Validity
			if (GMemDebugMode) PrvDebugAction(0, p, 0, false, false);
			return i;
			}
		infoP++;
		}
		
	ErrDisplay("Invalid Chunk Ptr");
	return 0;
}




/************************************************************
 *
 *  FUNCTION: MemPtrSetOwner
 *
 *  DESCRIPTION: Sets the owner ID of a chunk
 *
 *  PARAMETERS: chunk data pointer
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 10/25/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err		MemPtrSetOwner(void * chunkDataP, UInt16 owner){
	MemChunkHeaderPtr				chunkP;

	// Check for error
	if (! (chunkP = PrvPtrCheck(chunkDataP))) return memErrInvalidParam;

	// Check arguments
	if (owner > 14) 
		return memErrInvalidParam;

	// Enable writes
	MemSemaphoreReserve(true);
	
	// Heap Validity
	if (GMemDebugMode)	PrvDebugAction(0, chunkDataP, 0, false, false);

	// Set the owner
	chunkP->owner = owner;

	// Restore write protection
	MemSemaphoreRelease(true);
	
	return 0;
} 




/************************************************************
 *
 *  FUNCTION: MemPtrResize
 *
 *  DESCRIPTION: Resizes a chunk
 *
 *  PARAMETERS: chunk data pointer, new size
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 11/10/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err		MemPtrResize(void * chunkDataP, UInt32 newSize)
{
	MemChunkHeaderPtr		chunkP;
	Err					err;

	// Check for error
	if (! (chunkP = PrvPtrCheck(chunkDataP))) return memErrInvalidParam;

	// Wait for ownership
	MemSemaphoreReserve(true);

	// Error Checking
	if (GMemDebugMode) {
		UInt16	heapID;
		heapID = MemPtrHeapID(chunkDataP);
		PrvDebugAction(heapID, 0, 0, true, false);
	
		// Call Guts
		err = PrvPtrResize(chunkDataP, newSize);
		
		// Heap Validity
		if (GMemDebugMode) PrvDebugAction(heapID, 0, 0, false, true);
		}
		
	// Normal...
	else
		err = PrvPtrResize(chunkDataP, newSize);
		

	// Release ownership
	MemSemaphoreRelease(true);

	return err;
}




/************************************************************
 *
 *  FUNCTION: MemPtrUnlock
 *
 *  DESCRIPTION: Unlocks a chunk given a pointer to the chunk
 *
 *  PARAMETERS: pointer to chunk
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 11/17/94 
 *
 *  BY: Ron Marianetti
 * 	Bob 5/6/98 - Perf: check hOffset before locking semaphore
 *
 *************************************************************/
Err	MemPtrUnlock(void * p)
{
	MemChunkHeaderPtr				chunkP;
	Err							err;


	// Check for error
	if (! (chunkP = PrvPtrCheck(p))) return memErrInvalidParam;

	// Performance: If it's a non-movable chunk, ignore
	// PrvPtrUnlock is going to do this anyway, but if we do it here we avoid
	// having to get/release the semaphore.  1998-05-06
	if (!chunkP->hOffset)
		return 0;	// noErr

	
	// Enable writes
	MemSemaphoreReserve(true);
	
	// DebugMode
	if (GMemDebugMode) {
		UInt16	heapID;
		heapID = MemPtrHeapID(p);
	
		// Do it
		err = PrvPtrUnlock(p);
		
		// Thrash the heap
		PrvDebugAction(heapID, 0, 0, true, false);
		}
		
	// Normal..
	else
		err = PrvPtrUnlock(p);
	
	// Restore write-protection
	MemSemaphoreRelease(true);

	
	return err;
}




/************************************************************
 *
 *  FUNCTION: MemPtrResetLock
 *
 *  DESCRIPTION: Unlocks a chunk all the way to 0
 *
 *  PARAMETERS: handle to chunk
 *
 *  RETURNS:
 *
 *  CREATED: 11/03/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err 		MemPtrResetLock(void * p)
{
	MemChunkHeaderPtr				chunkP;
	UInt16							heapID;


	// Check for error
	if (! (chunkP = PrvPtrCheck(p))) return memErrInvalidParam;

	// Enable writes
	MemSemaphoreReserve(true);
	
	
	// Save heapID if debug mode
	if (GMemDebugMode) heapID = MemPtrHeapID(p);
	
	
	// If movable, clear lock count
	if (chunkP->lockCount  != memPtrLockCount) {
		chunkP->lockCount = 0;
		}

	// Restore write-protection
	MemSemaphoreRelease(true);
	

	// Thrash the heap
	if (GMemDebugMode) PrvDebugAction(heapID, 0, 0, true, false);

	return 0;
}




/************************************************************
 *
 *  FUNCTION: MemHandleNew
 *
 *  DESCRIPTION: Allocates a movable chunk in the dynamic heap
 *
 *  PARAMETERS: requested size of chunk
 *
 *  RETURNS: Ptr to chunk, or 0 if error
 *
 *  CREATED: 11/17/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
MemHandle	MemHandleNew(UInt32 size)
{
	MemHandle	h;
	UInt16 ownerID;


	// Wait for ownership
	MemSemaphoreReserve(false);
	
	ownerID = PrvCurOwnerID();
	
	// Error Checking
	if (GMemDebugMode) {
		PrvDebugAction(0, 0, 0, true, false);

		// Call guts
		h = PrvChunkNew(0, size, ownerID);
		
		// Error Checking
		PrvDebugAction(0, 0, 0, false, true);
		}
		
	// Normal
	else
		h = PrvChunkNew(0, size, ownerID);
		

	// Release ownership
	MemSemaphoreRelease(false);

	// Return result
	if (h) 
		return  memHandleProtect(h);
	else 
		return 0;
}




/************************************************************
 *
 *  FUNCTION: MemHandleFree
 *
 *  DESCRIPTION: Disposes of a movable chunk
 *
 *  PARAMETERS: handle to chunk
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 5/11/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err		MemHandleFree(MemHandle h)
{
	Err 		err;
	register void *	p;
	
	
	// Wait for ownership
	MemSemaphoreReserve(true);
	
	// Get pointer to the chunk and error check the handle
	if (! (p = PrvHandleCheck(h))) {err = memErrInvalidParam; goto Exit;}
	

	// Error Checking Mode
	if (GMemDebugMode) {
		UInt16		heapID;
		
		// Thrash the heap
		heapID = MemPtrHeapID(p);
		PrvDebugAction(heapID, 0, 0, true, false);
		p = PrvHandleCheck(h);						// Re-get pointer
		
		// Free it
		err = PrvPtrFree(p);
		
		// Fill Free Chunks
		PrvDebugAction(heapID, 0, 0, false, true);
		}
		
	// Normal..
	else
		err = PrvPtrFree(p);
		
		
Exit:
	// Release ownership
	MemSemaphoreRelease(true);
	
	// Exit
	return err;
}




/************************************************************
 *
 *  FUNCTION: MemHandleFlags
 *
 *  DESCRIPTION: Returns the flags for a chunk given the chunk handle
 *
 *  PARAMETERS: handle
 *
 *  RETURNS: chunk flags
 *
 *  CREATED: 5/11/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt16		MemHandleFlags(MemHandle h)
{
	UInt16					flags;
	register void *	p;
	MemChunkHeaderPtr		chunkP;

	// Wait for ownership
	MemSemaphoreReserve(false);
	
	// Heap Validity
	if (GMemDebugMode) PrvDebugAction(0, 0, h, false, false);

	// Get pointer to chunk and check for errors
	if (! (p = PrvHandleCheck(h))) {flags = 0; goto Exit;}
	
	// Get the flags
	chunkP = memChunkHeader(p);
	
	// this should really be changed to use the endian-aware macros, but because
	// the current behavior returns more bits (the returned value includes the
	// sizeAdj portion of the chunkHeader), we felt it too risky to change at
	// this time - mchen 11/5/99
	flags = *((UInt8 *)chunkP);

Exit:
	// Release ownership
	MemSemaphoreRelease(false);
	
	return flags;
} 




/************************************************************
 *
 *  FUNCTION: MemHandleSize
 *
 *  DESCRIPTION: Returns the requested size of a chunk
 *
 *  PARAMETERS: chunk handle
 *
 *  RETURNS: chunk size
 *
 *  CREATED: 5/11/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt32		MemHandleSize(MemHandle h)
{
	UInt32					size;
	register void *		p;
	MemChunkHeaderPtr		chunkP;


	// Wait for ownership
	MemSemaphoreReserve(false);
	
	// Heap Validity
	if (GMemDebugMode) PrvDebugAction(0, 0, h, false, false);

	// Get pointer to chunk and check for errors
	if (! (p = PrvHandleCheck(h))) {size = 0; goto Exit;}
	
	// Get the size
	chunkP = memChunkHeader(p);
	size = chunkP->size - (chunkP->sizeAdj) - sizeof(MemChunkHeaderType);


Exit:
	// Release ownership
	MemSemaphoreRelease(false);
	
	return size;
} 




/************************************************************
 *
 *  FUNCTION: MemHandleOwner
 *
 *  DESCRIPTION: Returns the owner ID of a chunk
 *
 *  PARAMETERS: chunk data handle
 *
 *  RETURNS: owner ID
 *
 *  CREATED: 5/11/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt16		MemHandleOwner(MemHandle h)
{
	UInt16					owner;
	register void *	p;
	MemChunkHeaderPtr		chunkP;

	// Wait for ownership
	MemSemaphoreReserve(false);
	
	// Heap Validity
	if (GMemDebugMode) PrvDebugAction(0, 0, h, false, false);

	// Get pointer to chunk and check for errors
	if (! (p = PrvHandleCheck(h))) {owner = 0; goto Exit;}
	
	// Get the owner
	chunkP = memChunkHeader(p);
	owner = chunkP->owner;

Exit:
	// Release ownership
	MemSemaphoreRelease(false);

	return owner;
} 




/************************************************************
 *
 *  FUNCTION: MemHandleLockCount
 *
 *  DESCRIPTION: Returns the lock count of a chunk
 *
 *  PARAMETERS: handle to chunk
 *
 *  RETURNS: lock count
 *
 *  CREATED: 10/25/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt16		MemHandleLockCount(MemHandle h)
{
	MemChunkHeaderPtr		chunkP;
	UInt16					lock;
	register UInt8 *	p;

	// Wait for ownership
	MemSemaphoreReserve(false);

	// Heap Validity
	if (GMemDebugMode) PrvDebugAction(0, 0, h, false, false);

	// Get pointer to chunk and check for errors
	if (! (p = PrvHandleCheck(h))) {lock = 0; goto Exit;}
	
	// Get pointer to the chunk header
	chunkP = memChunkHeader(p);
	
	// Get the lock count
	lock = chunkP->lockCount;

Exit:
	// Release ownership
	MemSemaphoreRelease(false);

	return lock;
} 




/************************************************************
 *
 *  FUNCTION: MemHandleHeapID
 *
 *  DESCRIPTION: Returns the heapID of a chunk
 *
 *  PARAMETERS: chunk data handle
 *
 *  RETURNS: heap ID
 *
 *  CREATED: 5/11/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt16		MemHandleHeapID(MemHandle h)
{
	UInt16					heapID;
	register UInt8 *	p;
	
	// Wait for ownership
	MemSemaphoreReserve(false);
	
	// Heap Validity
	if (GMemDebugMode) PrvDebugAction(0, 0, h, false, false);

	// Get pointer to chunk and check for errors
	if (! (p = PrvHandleCheck(h))) {heapID = 0; goto Exit;}
	
	// Call the root procedure
	PrvHeapPtrAndID((void *)p, &heapID);


Exit:
	// Release ownership
	MemSemaphoreRelease(false);

	return heapID;
} 




/************************************************************
 *
 *  FUNCTION: MemHandleDataStorage
 *
 *  DESCRIPTION: Returns true if the given handle is part of a 
 *   Data Storage Heap, else it is a handle in the dynamic heap.
 *
 *  PARAMETERS: chunk data handle
 *
 *  RETURNS: true if part of a data storage heap
 *
 *  CALLED BY: Fields package to determine if it needs to worry
 *    about data storage write-protection when editing a text field.
 *
 *  CREATED: 5/11/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Boolean 	MemHandleDataStorage(MemHandle h)
{
	UInt16					heapID;
	register void *	p;
	
	// Wait for ownership
	MemSemaphoreReserve(false);
	
	// Heap Validity
	if (GMemDebugMode) PrvDebugAction(0, 0, h, false, false);

	// Get pointer to chunk and check for errors
	if (! (p = PrvHandleCheck(h))) {heapID = 0; goto Exit;}
	
	// Get the heap ID
	PrvHeapPtrAndID(p, &heapID);

Exit:
	// Release ownership
	MemSemaphoreRelease(false);

	return (!MemHeapDynamic(heapID));
} 




/************************************************************
 *
 *  FUNCTION: MemHandleCardNo
 *
 *  DESCRIPTION: Returns the card number given a handle to a chunk
 *
 *  PARAMETERS: chunk handle
 *
 *  RETURNS: card number
 *
 *  CREATED: 11/16/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt16	MemHandleCardNo(MemHandle h)
{
	UInt16					cardNo;
	register void *	p;

	// Wait for ownership
	MemSemaphoreReserve(false);
	
	// Heap Validity
	if (GMemDebugMode) PrvDebugAction(0, 0, h, false, false);

	// Get pointer to chunk and check for errors
	if (! (p = PrvHandleCheck(h))) {cardNo = 0; goto Exit;}
	
	// Get card number
	cardNo = MemPtrCardNo(p);

Exit:
	// Release ownership
	MemSemaphoreRelease(false);
	
	return cardNo;

}




/************************************************************
 *
 *  FUNCTION: MemHandleSetOwner
 *
 *  DESCRIPTION: Sets the owner ID of a chunk
 *
 *  PARAMETERS: chunk data handle
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 10/25/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err		MemHandleSetOwner(MemHandle h, UInt16 owner)
{
	Err					err = 0;
	register void *	p;
	MemChunkHeaderPtr		chunkP;

	// Wait for ownership
	MemSemaphoreReserve(true);
	
	// Heap Validity
	if (GMemDebugMode) PrvDebugAction(0, 0, h, false, false);

	// Get pointer to chunk and check for errors
	if (! (p = PrvHandleCheck(h))) {err = memErrInvalidParam; goto Exit;}
	
	// Set Owner
	chunkP = memChunkHeader(p);
	chunkP->owner = owner;

Exit:
	// Release ownership
	MemSemaphoreRelease(true);
	
	return err;
} 




/************************************************************
 *
 *  FUNCTION: MemHandleResize
 *
 *  DESCRIPTION: Resizes a chunk
 *
 *  PARAMETERS: chunk data handle, new size
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 11/10/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err		MemHandleResize(MemHandle h, UInt32 newSize)
{
	Err						err;
	register void *		p;
	
	// Wait for ownership
	MemSemaphoreReserve(true);
	
	// Get pointer to chunk and check for errors
	if (! (p = PrvHandleCheck(h))) {err = memErrInvalidParam; goto Exit;}
	
	// Error check mode
	if (GMemDebugMode) {
	
		// Thrash the heap
		PrvDebugAction(0, 0, h, true, false);
		p = PrvHandleCheck(h);
	
		// Resize
		err = PrvPtrResize(p, newSize);

		// Fill free chunks
		PrvDebugAction(0, 0, h, false, true);
		}
		
	// Normal
	else
		err = PrvPtrResize(p, newSize);

Exit:
	// Release ownership
	MemSemaphoreRelease(true);
	
	return err;
}




/************************************************************
 *
 *  FUNCTION: MemHandleLock
 *
 *  DESCRIPTION: Locks down a chunk
 *
 *  PARAMETERS: handle to chunk
 *
 *  RETURNS: pointer to start of chunk data
 *
 *  CREATED: 10/25/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
void *		MemHandleLock(MemHandle h)
{
	MemChunkHeaderPtr				chunkP;
	UInt8 *						p;

	// Wait for ownership
	MemSemaphoreReserve(true);

	// Heap Validity
	if (GMemDebugMode) PrvDebugAction(0, 0, h, false, false);

	// Get pointer to chunk and check for errors
	if (! (p = PrvHandleCheck(h))) goto Exit;
	
	// Get pointer to the chunk header
	chunkP = memChunkHeader(p);
	
	// If it's a non-movable chunk, ignore. This could happen if we try
	//  and lock down a resource handle returned from a ROM database.
	if (!chunkP->hOffset || chunkP->free) goto Exit;
	
	// Bump the lock count
	chunkP->lockCount++;

	// Check for overflow
	if (chunkP->lockCount == memPtrLockCount) {
		ErrDisplay("Chunk over-locked");
		chunkP->lockCount--;
		}


Exit:
	// Release ownership
	MemSemaphoreRelease(true);

	return p;
} 




/************************************************************
 *
 *  FUNCTION: MemHandleUnlock
 *
 *  DESCRIPTION: Unlocks a chunk given a handle to the chunk
 *
 *  PARAMETERS: handle to chunk
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 11/17/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err	MemHandleUnlock(MemHandle h)
{
	Err							err;
	register	void *			p;

	// Wait for ownership
	MemSemaphoreReserve(true);
	
	// Heap Validity
	if (GMemDebugMode) PrvDebugAction(0, 0, h, false, false);

	// Get pointer to chunk and check for errors
	if (! (p = PrvHandleCheck(h))) {err = memErrInvalidParam; goto Exit;}
	
	// Unlock it
	err = PrvPtrUnlock(p);

	// Thrash the heap
	if (GMemDebugMode) PrvDebugAction(0, 0, h, true, false);

Exit:
	// Release ownership
	MemSemaphoreRelease(true);
	
	return err;
}




/************************************************************
 *
 *  FUNCTION: MemHandleResetLock
 *
 *  DESCRIPTION: Unlocks a chunk all the way to 0
 *
 *  PARAMETERS: handle to chunk
 *
 *  RETURNS:
 *
 *  CREATED: 11/03/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err 		MemHandleResetLock(MemHandle h)
{
	Err							err;
	register	void *			p;
	MemChunkHeaderPtr				chunkP;

	// Wait for ownership
	MemSemaphoreReserve(true);

	// Get pointer to chunk and check for errors
	if (! (p = PrvHandleCheck(h))) {err = memErrInvalidParam; goto Exit;}
	
	// Do it
	chunkP = memChunkHeader(p);
	err = MemPtrResetLock(p);
	
	
Exit:
	// Release ownership
	MemSemaphoreRelease(true);
	
	return err;
}




/************************************************************
 *
 *  FUNCTION: MemLocalIDToGlobal
 *
 *  DESCRIPTION: Converts a local chunkID, which is card relative, into
 *		a global pointer in the designated card
 *
 *  PARAMETERS: local chunkID, card number
 *
 *  RETURNS: global chunk ID
 *
 *  CREATED: 11/10/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
void *	MemLocalIDToGlobal(LocalID local,  UInt16 cardNo)
{
	void *	p;
	
	if (local==0) {
		ErrNonFatalDisplay("Nil ID");
		return 0;
	}

	ErrFatalDisplayIf(cardNo >= GMemCardSlots, "Invalid card #");

	// Under emulation mode, shift the offset accordingly
	#if	EMULATION_LEVEL != EMULATION_NONE
	local = PrvTargetToHostOffset(local, memCardInfoP(cardNo));
	#endif
	
	if (cardNo == 0)
		p = (void *)((local & 0xFFFFFFFE) + memCardInfoP(0)->baseP);
	else 
		p = (void *)((local & 0xFFFFFFFE) + memCardInfoP(1)->baseP);
		
		
	// If it's a handle, "protect" it
	if (local & 0x01)
		return memHandleProtect(p);
	else
		return p;
}




/************************************************************
 *
 *  FUNCTION: MemLocalIDToPtr
 *
 *  DESCRIPTION: Returns a pointer to a chunk designated by local ID
 *		and card number. 
 *
 *    Note: If the Local ID references a movable chunk and that chunk
 *     is not locked, this routine will return 0 as an error condition
 *
 *  PARAMETERS: local chunkID, card number
 *
 *  RETURNS: Ptr to chunk, or 0 if error
 *
 *  CREATED: 11/10/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
void *	MemLocalIDToPtr(LocalID local,  UInt16 cardNo)
{
	UInt8 *	p;
	
	if (local==0) {
		ErrNonFatalDisplay("Nil ID");
		return 0;
	}

	ErrFatalDisplayIf(cardNo >= GMemCardSlots, "Invalid card #");
	
	
	// Under emulation mode, shift the offset accordingly
	#if	EMULATION_LEVEL != EMULATION_NONE
	local = PrvTargetToHostOffset(local, memCardInfoP(cardNo));
	#endif
	
	
	// Get the pointer or handle
	if (cardNo == 0)
		p = (UInt8 *)((local & 0xFFFFFFFE) + memCardInfoP(0)->baseP);
	else
		p = (UInt8 *)((local & 0xFFFFFFFE) + memCardInfoP(1)->baseP);


	// If it's a handle, dereference it if it's locked
	if (local & 0x01) {
		
		#if MEMORY_FORCE_LOCK == MEMORY_FORCE_LOCK_ON
		// If handle locking is enforced, make sure this handle is locked
		//  before we dereference it
		if (!MemHandleLockCount((MemHandle)p)) {
			ErrDisplay("Chunk not locked");
			return 0;
			}
		#endif
		
		return *((void **)p);
		}
		
		
	else
		return p;

}




/************************************************************
 *
 *  FUNCTION: MemLocalIDToLockedPtr
 *
 *  DESCRIPTION: Returns a pointer to a chunk designated by local ID
 *		and card number. 
 *
 *    Note: If the Local ID references a movable chunk this routine
 *			will automatically lock the chunk before returning.
 *
 *  PARAMETERS: local chunkID, card number
 *
 *  RETURNS: Ptr to chunk, or 0 if error
 *
 *  CREATED: 11/10/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
void *	MemLocalIDToLockedPtr(LocalID local,  UInt16 cardNo)
{
	UInt8 *	p;
	
	if (local==0) {
		ErrNonFatalDisplay("Nil ID");
		return 0;
	}

	ErrFatalDisplayIf(cardNo >= GMemCardSlots, "Invalid card #");
	
	
	// Under emulation mode, shift the offset accordingly
	#if	EMULATION_LEVEL != EMULATION_NONE
	local = PrvTargetToHostOffset(local, memCardInfoP(cardNo));
	#endif
	
	
	// Get the pointer or handle
	if (cardNo == 0)
		p = (UInt8 *)((local & 0xFFFFFFFE) + memCardInfoP(0)->baseP);
	else
		p = (UInt8 *)((local & 0xFFFFFFFE) + memCardInfoP(1)->baseP);


	// If it's a handle, lock it
	if (local & 0x01) 
		return MemHandleLock((MemHandle)p);
		
	else
		return p;

}




/************************************************************
 *
 *  FUNCTION: MemHandleToLocalID
 *
 *  DESCRIPTION: Converts a handle into a local chunk ID which is
 *		card relative. The only difference between local ID's to handles
 *		and local ID's to pointers is that the low bit is set for
 *		handle based IDs.
 *
 *  PARAMETERS: handle
 *
 *  RETURNS:
 *
 *  CREATED: 11/10/94 
 *
 *  BY: Ron Marianetti
 *
 *		scl	9/10/98	Fixed native-mode dynamic heap case
 *
 *************************************************************/
LocalID MemHandleToLocalID(MemHandle h)
{
	UInt32			uH;

// -------------------- Emulation mode--------------------
#if EMULATION_LEVEL != EMULATION_NONE
	CardInfoPtr	infoP;
	UInt16			i;
	UInt32			offset;

	if (PrvHandleCheck(h) == 0)
	{
		ErrNonFatalDisplay("MemHandleToLocalID was passed an invalid handle");
		return 0;
	}

	// Unprotect it
	uH = (UInt32)memHandleUnProtect(h);
	
	// Search in each card
	infoP = memCardInfoP(0);
	for (i=0; i<GMemCardSlots; i++) {
		if (	uH > (UInt32)infoP->baseP  &&  
				uH < (UInt32)infoP->baseP + infoP->size) {
			offset = uH - (UInt32)infoP->baseP;
			return PrvHostToTargetOffset(offset, infoP) | 0x0001;
			}
		infoP++;
		}

	ErrDisplay("Invalid handle");
	return 0;
		
// ---------------------- Optimized Native Mode---------------------	
#else 
	// Unprotect it
	CardInfoPtr infoP = memCardInfoP(0);

	if (PrvHandleCheck(h) == 0)
	{
		ErrNonFatalDisplay("MemHandleToLocalID was passed an invalid handle");
		return 0;
	}

	uH = (UInt32)memHandleUnProtect(h);
	
	// Safety check (in debug builds)
	// The premise here is that LocalID's are ONLY relevant in storage heaps.
	//
	// This check could be even better; it could check each storage heap
	// somewhat like the simulator code above does (excluding dynamic heaps)
	// to ensure that the given handle actually lives within a valid storage heap.
	//
	// memDynamicHeaps is the number of dynamic heaps (typically one).
	// The dynamic heap is heap zero, and thus heap one is the storage heap.
	// Thus MemHeapPtr(memDynamicHeaps) returns the base address of the first storage heap.
	ErrNonFatalDisplayIf(uH < (UInt32)MemHeapPtr(memDynamicHeaps),
			"MemHandleToLocalID: handle not in storage heap");

	// Convert it to a local ID
	return (uH & infoP->cardOffsetMask) | 0x0001;	// was hwrCardOffsetMask

#endif		
		
}




/************************************************************
 *
 *  FUNCTION: MemPtrToLocalID
 *
 *  DESCRIPTION: Converts a pointer into a local chunk ID which is
 *		card relative. The only difference between local ID's to handles
 *		and local ID's to pointers is that the low bit is set for
 *		handle based IDs.
 *
 *  PARAMETERS: pointer
 *
 *  RETURNS:
 *
 *  CREATED: 11/10/94 
 *
 *  BY: Ron Marianetti
 *
 *		scl	9/22/98	Fixed native-mode dynamic heap case
 *
 *************************************************************/
LocalID MemPtrToLocalID(void * p)
{
	
	
// -------------------- Emulation mode--------------------
#if EMULATION_LEVEL != EMULATION_NONE
	CardInfoPtr	infoP;
	UInt16			i;
	UInt32			offset;

	// See which card this pointer belongs to
	infoP = memCardInfoP(0);
	for (i=0; i<GMemCardSlots; i++) {
		if (	(UInt32)p > (UInt32)infoP->baseP  &&  
				(UInt32)p < (UInt32)infoP->baseP + infoP->size) {
			offset =  (UInt32)p - (UInt32)infoP->baseP;
			return PrvHostToTargetOffset(offset, infoP);
			}
		infoP++;
		}

	ErrDisplay("Invalid Ptr");
	return 0;
		
// ---------------------- Optimized Native Mode---------------------	
#else
	// Despite what you might think, this routine is called (by MemStoreInit)
	// to calculate a "LocalID" for a pointer in (actually TO) the dynamic heap.
	// Before spending a great deal of time trying to validate the given pointer
	// as a valid storage heap pointer, walk through MemStoreInit so you don't
	// waste as much time that I did in trying to figure this all out... <SCL 9/22/98>

	// If it's a pointer in (to) the dynamic heap, subtract the card base...

	CardInfoPtr infoP = memCardInfoP(0);

	if ((UInt32)p < (UInt32)infoP->baseP) 				// was hwrCardBase0
		return ((UInt32)p - (UInt32)infoP->baseP);	// was hwrCardBase0
	else
		return ((UInt32)p & infoP->cardOffsetMask);	// was hwrCardOffsetMask

#endif		
		
}




/************************************************************
 *
 *  FUNCTION: MemLocalIDKind
 *
 *  DESCRIPTION: Returns whether or not a localID references a handle
 *		or a pointer.
 *
 *  PARAMETERS: local ID
 *
 *  RETURNS:
 *
 *  CREATED: 11/10/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
LocalIDKind MemLocalIDKind(LocalID local)
{
	if (local & 0x0001) return memIDHandle;
	else return memIDPtr;
}




/************************************************************
 *
 *  FUNCTION: MemMove
 *
 *  DESCRIPTION: A Block move routine. 
 *
 *  PARAMETERS: src, dst, numBytes
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 11/10/94 
 *
 *  BY: Ron Marianetti
 *
 *  REVISION HISTORY
 *			Name	Date		Description
 *			----	----		-----------
 *			scl	1/22/98	MemMove FROM null changed from fatal to NONfatal error.
 *
 *************************************************************/
Err MemMove(void * dP, const void * sP, Int32 bytes)
{
	UInt8		* dstP;
	const UInt8		* srcP;
	register	UInt16 * dstWP;
	register	const UInt16 * srcWP;
	UInt32	words;
	
	// Return if no bytes
	if (!bytes)
		return 0;
	
	// Fatal = Show dialog (or drop to debugger if we've
	//   been there before) in ALL builds!
	// Writing to null is bad.  We do not allow this to happen!
	ErrFatalDisplayIf(dP == NULL, "MemMove to NULL");

	// NonFatal = Show dialog (or drop to debugger if we've
	//   been there before), but ONLY in debug builds!
	// Reading from null is bad, but not fatal.  Chances are that the
	//   app probably wrote to null (but didn't use MemMove).  However,
	//   if we die on a release build (as we did in early 3.0 builds)
	//   it costs us compatibility with a lot of apps that "seemed" to
	//   work fine on earlier systems (like 1.0 and 2.0).
	ErrNonFatalDisplayIf(sP == NULL, "MemMove from NULL");
	
	
	//----------------------------------------------------------------------
	// If destination is before the source, copy from beginning to end
	//----------------------------------------------------------------------
	if ( dP <= sP ) {
		// Copy a byte if we're add aligned
		if ((UInt16)dP & 0x0001) {
			*((UInt8 *)dP) = *((UInt8 *)sP);
			bytes--;
			dstWP = (UInt16 *)((UInt8 *)dP + 1);
			srcWP = (UInt16 *)((UInt8 *)sP + 1);
			}
		else {
			dstWP = dP;
			srcWP = sP;
			}
			
			
		// If we're even aligned, copy words
		if ((((UInt16)dstWP | (UInt16)srcWP) & 0x0001) == 0) {
			words = bytes >> 1;
			if (words) {
				do {
					*dstWP++ = *srcWP++;
					} while (--words);
				bytes &= 0x01;
				}
			}
		

		// Copy remaining bytes
		if (bytes) {
			dstP = (UInt8 *)dstWP;
			srcP = (UInt8 *)srcWP;
			do {
				*dstP++ = *srcP++;
				} while (--bytes);
			}
		}
	
	
	//----------------------------------------------------------------------
	// Otherwise, copy from end to beginning
	//----------------------------------------------------------------------
	else {
		// Copy a byte if we're add aligned
		dstWP = (UInt16 *)((UInt8 *)dP + bytes);
		srcWP = (UInt16 *)((UInt8 *)sP + bytes);
		if ((UInt16)dstWP & 0x0001) {
			dstWP = (UInt16 *)((UInt8 *)dstWP - 1);
			srcWP = (UInt16 *)((UInt8 *)srcWP - 1);
			*((UInt8 *)dstWP) = *((UInt8 *)srcWP);
			bytes--;
			}
			
			
		// If we're even aligned, copy words
		if ((((UInt16)dstWP | (UInt16)srcWP) & 0x0001) == 0) {
			words = bytes >> 1;
			if (words) {
				do {
					*(--dstWP) = *(--srcWP);
					} while (--words);
				bytes &= 0x01;
				}
			}
		

		// Copy remaining bytes
		if (bytes) {
			dstP = (UInt8 *)dstWP;
			srcP = (UInt8 *)srcWP;
			do {
				*(--dstP) = *(--srcP);
				} while (--bytes);
			}
				
		}
		
		
	return 0;
}




/************************************************************
 *
 *  FUNCTION: MemSet
 *
 *  DESCRIPTION: Set Memory to a specific value
 *		DOLATER... optimize this...
 *
 *  PARAMETERS: dstP, numBytes, value
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 4/6/95
 *
 *  BY: Ron Marianetti
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	10/24/96	Rewrote to set using ULongs.  Tuned by disassembly.
 *			vmk	1/15/98	Bail out if numBytes is zero
 *			roger	4/9/99	Added CHECK_MEMSET_IS_NEEDED.
 *************************************************************/
Err MemSet(void * dstP, Int32 numBytes, UInt8 value)
{
	UInt8		* dP;
	UInt32	longValue;
	UInt32	setCount;


	// Return if no bytes to set
	if (!numBytes)
		return 0;
	
	
	ErrFatalDisplayIf(dstP == NULL, "MemSet to NULL");
	
	// Check for cases where no work is performed.  Report them with the idea that 
	// the operation can be avoided.
#ifdef CHECK_MEMSET_IS_NEEDED

	setCount = numBytes;
	dP = dstP;
	while (setCount > 0)
		{
		if (*dP++ != value) break;
		setCount--;
		}
	ErrFatalDisplayIf(setCount == 0, "MemSet unneeded.");

#endif

	
	dP = dstP;
	
	// Set values one byte at a time until the destination is at
	// a four byte boundary.
	setCount = 4 - ((UInt32) dP & 0x03);
	if (setCount > 0)
		do {
			if (numBytes-- == 0)
				goto Exit;
			
			*dP++ = value;
			} while (--setCount);
	
	
	// Write the value using longs for better speed.
	longValue = value;
	longValue |= (longValue << 8);
	longValue |= (longValue << 16);
	
	// Performance: unroll loop for better efficiency, Bob/Roger 6/16/98
	
	// first, copy in 32 byte chunks
	setCount = (numBytes >> 5);		// Number of 8-long (32 byte) chunks
	numBytes &= 0x1F;
	while (setCount != 0) {
		*((UInt32 *)dP)++ = longValue;
		*((UInt32 *)dP)++ = longValue;
		*((UInt32 *)dP)++ = longValue;
		*((UInt32 *)dP)++ = longValue;
		*((UInt32 *)dP)++ = longValue;
		*((UInt32 *)dP)++ = longValue;
		*((UInt32 *)dP)++ = longValue;
		*((UInt32 *)dP)++ = longValue;
		setCount--;
		}
	
	// move remaining longs
	while (numBytes >= 4) {
		*((UInt32 *)dP)++ = longValue;
		numBytes -= 4;
		}

	// now move remaining bytes
	while (numBytes != 0) {
		*dP++ = value;
		numBytes--;
		}

	
Exit:	
	return 0;
}




/************************************************************
 *
 *  FUNCTION: MemCmp
 *
 *  DESCRIPTION: Compares two blocks of memory.
 *
 *  PARAMETERS: 2 pointers and the number of bytes to compare
 *
 *  RETURNS: 0 if they match, non-zero if not
 *				 + if s1 > s2
 *				 - if s1 < s2
 *
 *  CREATED: 5/21/96
 *
 *  BY: Art Lamb
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	10/24/96	Rewrote to speed up.  Tuned by disassembly.
 *			grant	2/15/99	Changed result type from UInt8 to Int8.
 *			jwm   99-10-16 Changed result's type to Int16.
 *************************************************************/
Int16 MemCmp (const void * s1, const void * s2, Int32 numBytes)
{
	const UInt8 * p1 = s1;
	const UInt8 * p2 = s2;
	Int16 result;

	ErrNonFatalDisplayIf(s1 == NULL || s2 == NULL, "Passing NULL to MemCmp");
	result = 0;
	
	if (numBytes > 0)
		do {
			// Due to integral promotion, the subtraction is actually done as "int"
			if ((result = *p1++ - *p2++) != 0)
				break;
			} while (--numBytes);
		
	return result;
}


/************************************************************
 *
 *  FUNCTION: MemSemaphoreReserve
 *            NOTE: This call gets replaced when AMX Loads!!!!
 *
 *  DESCRIPTION: 
 *	  Called by Memory Manager and some Data Manager routines on entry.
 *	  If writeAccess is false, this call waits for ownership  of the 
 *		Memory Manager Semaphore and prevents other tasks from changing 
 *		the heaps while the semaphore is held.
 *
 *   If the writeAccess parameter is true, this routine disables task 
 *   switching altogether and enables writes to the data storage area. It is used 
 *   by the Data Manager when it needs to update data in the user storage 
 *   heap. We place this call in this AMX module because it is hardware 
 *	  specific and we want a direct jump to the AMX procedures for performance 
 *   reasons.
 *
 *  This call MUST be balanced with a call to MemSemaphoreRelease!
 *
 *  PARAMETERS: 
 *			writeAccess	- set true to enable writes to data storage area
 *
 *  RETURNS: 0 if no err
 *
 *  Revision History:
 *  7/19/95		ron		Initial Revision
 *  3/30/99		jesse	Increased reserve limit from 5 to 7.
 *
 *************************************************************/
Err		MemSemaphoreReserve(Boolean writeAccess)
{
#if	EMULATION_LEVEL != EMULATION_NONE

	// If we don't need write access, simply reserve the Memory Manager
	//  semaphore
	if (!writeAccess) {
		if (GMemSemaphoreID >= 7) ErrDisplay("Might be unbalanced Reserve...");	
		GMemSemaphoreID++;
		}
		
		
	// If we need write access, disable task switching altogether and enable
	//  writes to the data area
	else {
	
		// Bump the write enable level
		if (GMemDataWELevel >= 5) ErrDisplay("Might be unbalanced Reserve...");	
		GMemDataWELevel++;
		}
	
#else
	#pragma unused(writeAccess)
#endif	

	return 0;
}




/************************************************************
 *
 *  FUNCTION: MemSemaphoreRelease
 *            NOTE: This call gets replaced when AMX Loads!!!!
 *
 *  DESCRIPTION: 
 *		If writeAccess is false, this routine releases ownership of the Memory
 *   Manager semaphore. 
 *
 *   If writeAccess is true, it restores  write-protection to the data storage
 *   area and restores task switching. 
 *
 *   It must be called with the SAME writeAccess parameter that was passed 
 *   to most recent MemSemaphoreReserve.
 *
 *  PARAMETERS: 
 *		writeAccess - should be same value as passed to most recent
 *							MemSemaphoreReserve call.
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 7/19/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err		MemSemaphoreRelease(Boolean writeAccess)
{
#if	EMULATION_LEVEL != EMULATION_NONE
	
	// If write access was not required, just decrement the semaphore
	if (!writeAccess) {
		if (GMemSemaphoreID <= 0) ErrDisplay("Might be unbalanced Reserve...");
		GMemSemaphoreID--;
		}
		
		
	// Else, decrement the Write-Enable level and restore task switching
	// if we reach 0.
	else {
		if (GMemDataWELevel <= 0) ErrDisplay("Might be unbalanced Reserve...");
		GMemDataWELevel--;
		}
		
#else
	#pragma unused(writeAccess)
#endif
	return 0;
}




/************************************************************
 *
 *  FUNCTION: MemDebugMode
 *
 *  DESCRIPTION: Returns the current Debugging mode of the Memory Manager
 *
 *  PARAMETERS: 
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 5/19/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt16 MemDebugMode(void)
{
	return GMemDebugMode;
}




/************************************************************
 *
 *  FUNCTION: MemSetDebugMode
 *
 *  DESCRIPTION: Sets the Debugging mode of the Memory Manager
 *
 *  PARAMETERS: 
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 5/19/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err MemSetDebugMode(UInt16 flags)
{
	Boolean						fillFree = false;
	UInt16						cardNo, heapID;
	UInt16						heapNo;
	CardInfoPtr					infoP;
	UInt16						oldFlags;
	Err							err=0;
	char							text[128];
	char							num[8];
	
	
	// Wait for ownership if not called from Debugger
	MemSemaphoreReserve(true);


	// Save the old flags
	oldFlags = GMemDebugMode;
	
	// If setting the minimum heapFree capture, clear the current setting
	if ((flags & memDebugModeRecordMinDynHeapFree) && !(oldFlags & memDebugModeRecordMinDynHeapFree))
		GMemMinDynHeapFree = 0;
		
	
	// If setting fillFreeChunks, pre-init the free chunks
	if (	 (flags & memDebugModeFillFree) && 
			!(oldFlags & memDebugModeFillFree))
		fillFree = true;
		
	
	//-----------------------------------------------------------------------
	// Check all the heaps and pre-fill free chunks if necessary
	//-----------------------------------------------------------------------
	if (flags) {
		infoP = memCardInfoP(0);
		for (cardNo=0; cardNo < GMemCardSlots; cardNo++, infoP++) {
			if (!infoP->size) continue;
			
			for (heapNo = 0; heapNo < MemNumRAMHeaps(cardNo); heapNo++) {
				heapID = MemHeapID(cardNo, heapNo);
				err = MemHeapCheck(heapID);
				if (err) goto Exit;
				
				if (fillFree)
					PrvFillFreeChunks(heapID);
				}
				
			} // cardNo
		}
		
		
	// Set the new flags
	GMemDebugMode = flags;
	
	
Exit:
	// Display error
	if (err) {
		StrCopy(text, "Heap #");
		StrIToA(num, heapID);
		StrCat(text, num);
		StrCat(text, " is invalid");
		DbgSrcMessage(text);
		}
	
	// Release ownership if not called from Debugger
	MemSemaphoreRelease(true);
	
	return err;
}




/************************************************************
 *
 *  FUNCTION: MemHeapScramble
 *
 *  DESCRIPTION: Scrambles the given heap.
 *		We do multiple passes over the heap, until we get to a pass that can
 *		no longer scramble the heap.
 *
 *		For each movable chunk with moved bit clear {
 *			if chunk next to a free chunk
 *				 shift it up or down into free chunk area
 *				 set moved bit in flags
 *			else 
 *				 if possible to reallocate
 *					 reallocate chunk and move to new location
 *					 set moved bit in flags
 *			}
 *		
 *
 *
 *  PARAMETERS: 
 *		heapID	- heap to scramble
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 5/19/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err MemHeapScramble(UInt16 heapID)
{
	Err							err = -1;
	MemHeapHeaderPtr		   heapP;
	MemChunkHeaderPtr			chunkP;
	UInt32						heapSize, lastOffset, firstOffset;
	UInt16						scrambles;
	UInt32						size;
	
	
	// Wait for ownership
	MemSemaphoreReserve(true);

	// Get a heap pointer from the heap ID
	heapP = MemHeapPtr(heapID);
	
	// If it's read only, don't do anything
	if (heapP->flags & memHeapFlagReadOnly) {
		err = 0;
		goto Exit;
		}
		
	
	// Get offset of end of heap and to first chunk in heap
	heapSize = MemHeapSize(heapID);
	lastOffset = heapSize - sizeof(MemHeapTerminatorType);
	firstOffset = sizeof(MemHeapHeaderType) 
					+ heapP->mstrPtrTbl.numEntries * sizeof(void *);
	
	
	// Scramble the heap until we can't scramble no more....Whenever
	//  a chunk is moved, it's memChunkFlagMovedBit is set so that
	//  we can track which chunks have been scrambled.
	do {
		scrambles = PrvHeapScramble(heapP, firstOffset, lastOffset);
		} while(scrambles);
		
		
	//-------------------------------------------------------------
	// Clear all the moved bits we set during the scramble.
	//-------------------------------------------------------------
	chunkP = (MemChunkHeaderPtr)((UInt8 *)heapP + firstOffset);
	while((size = chunkP->size) != 0) {
		chunkP->moved = false;
		
		// On to next chunk
		chunkP = (MemChunkHeaderPtr)((UInt8 *)chunkP + size);
		} 
					

	// Heap OK
	err = 0;

Exit:
	
	// Release ownership
	MemSemaphoreRelease(true);
	
	// Display error, if any
	ErrFatalDisplayIf(err, "Error occurred while scrambling heap");
	
	return err;
}




/************************************************************
 *
 *  FUNCTION: MemHeapCheck
 *
 *  DESCRIPTION: Checks validity of given heap
 *
 *  PARAMETERS: 
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 5/19/95
 *
 *  BY: Ron Marianetti
 *
 *		vmk	7/18/97	Added memHeapFlagVers2 to valid flag set in flag validity checking
 *		Bob	6/17/98	added memHeapFlagVers3
 *		Bob	6/18/98	Added free list validation
 *		ADH	10/12/99 Added memHeapFlagVers4
 *		mchen 10/26/99 Added check that free list is sorted
 *
 *************************************************************/
Err MemHeapCheck(UInt16 heapID)
{
	Err							err = -1;
	MemHeapHeaderPtr		   heapP;
	MemChunkHeaderPtr			chunkP;
	void *						chunkDataP;
	void **						chunkH;
	UInt32						heapSize, lastOffset, heapOffset;
	UInt16 *						wP;
	UInt16						count;
	Char							text[128];
	Char							num[16];
	UInt16						freeCount = 0;
	

	text[0] = '\0';	
	
	// Wait for ownership
	MemSemaphoreReserve(false);


	// Get a near heap pointer from the heap ID
	heapP = MemHeapPtr(heapID);
	
	
	// If it's read only, don't do anything
	if (heapP->flags & memHeapFlagReadOnly) {
		err = 0;
		goto Exit;
		}

	// Check validity of heap header
	if (heapP->flags & ~(memHeapFlagReadOnly | memHeapFlagVers2 | memHeapFlagVers3 | memHeapFlagVers4)) 
		{
		StrCopy(text, "\nBad flags");
		goto Exit;
		}
	if (heapP->size & 0x01)
		{
		StrCopy(text, "\nBad size");
		goto Exit;
		}

	
	// Get pointer to first chunk
	heapSize = MemHeapSize(heapID);
	lastOffset = heapSize - sizeof(MemHeapTerminatorType);
	
	heapOffset = sizeof(MemHeapHeaderType) 
					+ heapP->mstrPtrTbl.numEntries * sizeof(void *);
					
	chunkP = (MemChunkHeaderPtr)((UInt8 *)heapP + heapOffset);
	
	
	// Walk through the chunks
	do {
		chunkDataP = memChunkData(chunkP);
		chunkH = 0;
		
		// Check validity of chunk header
		if (chunkP->size == 0 || chunkP->size & 0x01) {
			StrCopy(text, "\nBad size");
			goto Exit;
			}
		if (chunkP->moved || chunkP->unused2 || chunkP->unused3) {
			StrCopy(text, "\nBad flags");
			goto Exit;
			}
		
		// Determine the handle
		if (chunkP->hOffset && !chunkP->free) {
			chunkH = (void **)((UInt8 *) chunkP - ((Int32)chunkP->hOffset*2));
			if (*chunkH != (void *)chunkDataP)
				{
				StrCopy(text, "\nBad handle");
				goto Exit;
				}
			}
		
		// validate free chunks
		if (chunkP->free) {
			if (chunkP->lockCount) {
				StrCopy(text, "\nFree chunk locked");
				goto Exit;
				}

			// free chunks are filled, check the contents
			if (GMemDebugMode & memDebugModeFillFree) {
				wP = (UInt16 *)chunkDataP;
				count = chunkP->size  - sizeof(MemChunkHeaderType);
				count = (count >> 1) + 1;
				while(--count) {
					if (*wP++ != memFillFreeValue) {
						StrCopy(text, "\nFree chunk: ");
						StrIToH(num, (UInt32) chunkDataP);
						StrCat(text, num);
						StrCat(text, " overwritten.");
						DbgSrcMessage(text);
						goto Exit;
						}
					}
				}

			// count free chunks found in regular walk
			freeCount++;
			}
				
				
		// On to next chunk
		heapOffset += chunkP->size;
		chunkP = (MemChunkHeaderPtr)((UInt8 *)chunkP + chunkP->size);
		} while(heapOffset < lastOffset);
					
		
	// Make sure the last chunk has 0 in it's size field
	if (chunkP->size) {
			StrCopy(text, "\nBad Terminator");
			goto Exit;
			}
	
	// Check free list, walk it and make sure it all connects up
	chunkP = (MemChunkHeaderPtr)((UInt16 *)heapP + heapP->firstFreeChunkOffset);
	while (chunkP->size != 0) {
		if (!chunkP->free) {
			StrCopy(text, "\nNon-free chunk in free list");
			goto Exit;
			}
		if (chunkP > (MemChunkHeaderPtr)((UInt8 *)heapP + lastOffset)
		|| chunkP < (MemChunkHeaderPtr)((UInt8 *)heapP + sizeof(MemHeapHeaderType) 
					+ heapP->mstrPtrTbl.numEntries * sizeof(void *))) {
			StrCopy(text, "\nFree list jumped out of heap");
			goto Exit;
			}
		if (chunkP->hOffset == 0) {
			StrCopy(text, "\nFree chunk points to self");
			goto Exit;
			}
		if (chunkP->hOffset < 0) {
			StrCopy(text, "\nFree list not sorted by ascending memory address");
			goto Exit;
			}

		freeCount--;		// uncount free chunks in free list
		chunkP = (MemChunkHeaderPtr) ((UInt16 *)chunkP + chunkP->hOffset);
		}
	
	if (chunkP != (MemChunkHeaderPtr)((UInt8 *)heapP + lastOffset)) {
		StrCopy(text, "\nFree list doesn't end at heap terminator");
		goto Exit;
		}
	if (freeCount != 0) {
		StrCopy(text, "\nFree list length and free chunk count don't match: ");
		StrIToA(num, freeCount);
		StrCat(text, num);
		goto Exit;
		}

	// Heap OK
	err = 0;

Exit:
	
	// Release ownership
	MemSemaphoreRelease(false);
	
	// Display error, if any
	if (err) {
		if (text[0] != '\0')
			{
			DbgSrcMessage(text);
			if (!GDbgInDebugger) DbgSrcBreak();
			}

		StrCopy(text, "\nHeap ");
		StrIToA(num, heapID);
		StrCat(text, num);
		StrCat(text, " invalid.");
		DbgSrcMessage(text);
		DbgSrcMessage("\nTurning off checking.\n");
		if (!GDbgInDebugger) DbgSrcBreak();
		GMemDebugMode = 0;
		}
	
	return err;
}




/************************************************************
 *
 *  FUNCTION: PrvHandleCheck, private
 *
 *  DESCRIPTION: Checks the validity of a chunk handle. This routine
 *		is called by every Memory Manager routine that might write
 *		to the data storage area so it MUST BE VERY FAST!!.
 *
 *  PARAMETERS:
 *		h - chunk handle
 *			
 *  RETURNS: 
 *		Ptr to chunk data if valid, else 0
 *
 *  CREATED: 8/8/95
 *
 *  BY: Ron Marianett
 *		Bob - 10/23/98	Expanded detail in debug-only checks
 *		jmp - 09/30/99	Check for non-word-aligned handles.
 *		mchen - 11/15/99 Added check for free flag in handle entry
 *							  for unaligned chunkP in handle.
 *
 *************************************************************/
static void *	PrvHandleCheck(MemHandle h)
{
	UInt8 *				p;
	MemChunkHeaderPtr	chunkP;
	Int32					hOffset;
	void **				uH;
	
	// Check for nil handle, and complain if error checking is on.
	if (!h) {
		ErrFatalDisplay("NULL handle");		
		p = 0;
		goto Exit;
		}
	
	// Check for a non-word-aligned handle, and complain if error checking is on.
	if ((UInt32)h & 0x00000001) {
		ErrFatalDisplay("Non-word-aligned handle");
		p = 0;
		goto Exit;
		}
	
	// Remove the lock protection
	uH = memHandleUnProtect(h);
	
	// Get pointer to the chunk header
	p = *uH;
	if (!p || ((UInt32)p & mstrPtrTblFreeEntryMask))
		{
		ErrFatalDisplay("Free handle");		
		goto Exit;
		}

	chunkP = memChunkHeader(p);
	
	ErrNonFatalDisplayIf(chunkP->free, "Trashed chunk header");
	
	if ((UInt32)chunkP & 0x00000001)
		{
		// Unaligned pointer shouldn't happen. This test should not ever trigger because
		// mstrPtrTblFreeEntryMask is currently the same value and so the earlier test
		// above would have been tripped if we got an unaligned chunk.  However, on a
		// 4-byte alignment architecture, we would change this to 0x03 and this check
		// would be more useful.
		ErrFatalDisplay("Invalid handle");
		goto Exit;
		}

	// Validate that the handle offset references the handle that points
	//  back to the chunk.
	if ((hOffset = chunkP->hOffset) != 0) {
		if ((Int32)chunkP - ((Int32)hOffset*2)  != (Int32)uH) 
			p = 0;
		}
	// If it's not a movable chunk, just make sure it has the right lockcount
	else {
		if (chunkP->lockCount != memPtrLockCount) 
			p = 0;
		}
	
	ErrFatalDisplayIf(!p, "Invalid handle");

Exit:
	return p;
}




/************************************************************
 *
 *  FUNCTION: PrvPtrCheck, private
 *
 *  DESCRIPTION: Checks the validity of a chunk pointer. This routine
 *		is called by every Memory Manager routine that might write
 *		to the data storage area so it MUST BE VERY FAST!!.
 *
 *  PARAMETERS:
 *		p - chunk data pointer
 *			
 *  RETURNS: 
 *		Ptr to chunk header if valid, else 0
 *
 *  CREATED: 8/8/95
 *
 *  BY: Ron Marianetti
 *		Bob - 10/23/98  Expanded detail in debug-only checks
 *
 *************************************************************/
static MemChunkHeaderPtr	PrvPtrCheck(const void *p)
{
	MemChunkHeaderPtr	chunkP;
	Int32				hOffset;
	
	// With error checking on, check for nil handle
	ErrFatalDisplayIf(!p, "Nil Ptr");
	
	// Get pointer to the chunk header
	chunkP = memChunkHeader(p);
	
	// With error checking on, check for trashed or free chunks
	ErrNonFatalDisplayIf((UInt32) p & 0x80000000, "ptr is handle");
	ErrNonFatalDisplayIf(chunkP->free, "Free ptr");
	ErrNonFatalDisplayIf(chunkP->unused2 || chunkP->unused3, "Bad chunk header");
	
	// If it's a movable chunk, validate that the handle offset
	//  references the handle that points back to the chunk.
	if ((hOffset = chunkP->hOffset) != 0) {
		void **h;
		h = (void **)( (UInt8 *)chunkP - ((Int32)hOffset * 2) );
		if ((UInt32)(*h)  !=  (UInt32)p) {chunkP = 0; goto Exit;}
		}
		
	// If it's not a movable chunk, it must have a max lock count
	else {
		if (chunkP->lockCount != memPtrLockCount) 
			chunkP = 0;
		}
	
Exit:
	ErrFatalDisplayIf(!chunkP, "Invalid chunk ptr");
	return chunkP;
}




/************************************************************
 *
 *  FUNCTION: MemPtrCheck
 *
 *  DESCRIPTION: Checks the validity of a chunk pointer. 
 *
 *  PARAMETERS:
 *		p - chunk data pointer
 *			
 *  RETURNS: 
 *		Ptr to chunk header if valid, else 0
 *
 *  CREATED: 8/8/95
 *
 *  BY: Roger Flores
 *		Bob - 6/17/98  Finished checks
 *		Bob - 10/23/98  Commented out because no one can ever calls this
 *
 *************************************************************/
/*
static MemChunkHeaderPtr	MemPtrCheck(void * p)
{
	MemChunkHeaderPtr	chunkP = 0;
	MemHeapHeaderPtr	heapP;
	
	// Check for nil pointer
	ErrFatalDisplayIf(!p, "NULL ptr");
	
	// Check for handle instead of a pointer
	ErrFatalDisplayIf((UInt32) p & 0x80000000, "ptr is handle");
	
	
	// Get pointer to the chunk header
	chunkP = (MemChunkHeaderPtr)((UInt8 *)p - sizeof(MemChunkHeaderType));
	
	
	// Check flags in chunk header
	if (chunkP->free || chunkP->unused2 || chunkP->unused3) {
		ErrNonFatalDisplay("Bad chunk header");
		return 0;
		}


	// Verify that the pointer points to a heap.
	{
	UInt16					heapID;
	if (!(heapP = PrvHeapPtrAndID(p, &heapID))) {
		ErrNonFatalDisplay("ptr not in any heap");
		return 0;
		}
	}


	// Verify that the pointer points to a chunk
	{
	MemChunkHeaderPtr candidateP = (MemChunkHeaderPtr)((UInt8 *)heapP + sizeof(MemHeapHeaderType)
					+ (heapP->mstrPtrTbl.numEntries << 2) );
	while (candidateP->size != 0 && candidateP < chunkP)
		candidateP = (MemChunkHeaderPtr) ((UInt8 *)candidateP + candidateP->size);
	if (candidateP != chunkP) {
		ErrNonFatalDisplay("Pointer is not a chunk");
		return 0;
		}
	}


	// If it's a movable chunk, validate that the handle offset
	//  references the handle that points back to the chunk.
	{
	Int32 hOffset = chunkP->hOffset;
	if (hOffset && !chunkP->free) {
		MemHandle	h = (MemHandle)((UInt32)chunkP - ((Int32)chunkP->hOffset << 1));
		if (*h != p) {
			ErrFatalDisplay("ptr has invalid handle");
			return 0;
			}
		}
		
	// If it's not a movable chunk, it must have a max lock count
	else if (chunkP->lockCount != memPtrLockCount) {
		ErrFatalDisplay("Invalid lock count");
		return 0;
		}
	}
	
	// if we got this far, the pointer is valid
	return chunkP;
}
*/


/************************************************************
 *
 *  FUNCTION: PrvDebugValidateFreeList
 *
 *  DESCRIPTION: Walk the heap and validate a bunch of stuff
 *					  about the free list.  This function is only
 *					  useful when debugging the memory manager itself,
 *					  and it should generally be stubbed out otherwise
 *
 *  PARAMETERS: pointer to heap
 *
 *  CREATED: 1998-05-15 
 *
 *  BY: Bob Ebert
 *
 *************************************************************/
/*
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL

static void PrvDebugValidateFreeList(MemHeapHeaderPtr heapP)
{
	Int16 freeCount = 0;
	MemChunkHeaderPtr chunkP, endOfHeap;
	
	endOfHeap = (MemChunkHeaderPtr)((UInt8 *)heapP + heapP->size - sizeof(MemHeapTerminatorType));
	
	// first, walk the free list and make sure it all connects up
	chunkP = (MemChunkHeaderPtr)((UInt16 *)heapP + heapP->firstFreeChunkOffset);
	
	while (chunkP->size != 0) {
		ErrFatalDisplayIf(!chunkP->free, "non-free chunk in free list");
		ErrFatalDisplayIf(chunkP > endOfHeap, "crossed end of heap");
		ErrFatalDisplayIf(chunkP->hOffset == 0, "zero offset");

		freeCount++;
		chunkP = (MemChunkHeaderPtr) ((UInt16 *)chunkP + chunkP->hOffset);
		}
	ErrFatalDisplayIf(chunkP != endOfHeap, "zero sized chunk too early");

	// next, walk every chunk and count free ones
	chunkP = (MemChunkHeaderPtr)((UInt8 *)heapP + sizeof(MemHeapHeaderType)
					+ heapP->mstrPtrTbl.numEntries * sizeof(void *));
	while (chunkP->size != 0) {
		if (chunkP->free)
			freeCount--;
		chunkP = (MemChunkHeaderPtr) ((UInt8 *)chunkP + chunkP->size);
		}
	ErrFatalDisplayIf(chunkP != endOfHeap, "zero sized chunk too early");
	ErrFatalDisplayIf(freeCount != 0, "free list length and free chunk count don't match");
}

#else

// just stub out the validate checks for nondebug builds
#define PrvDebugValidateFreeList(heapP)

#endif
*/


/************************************************************
 *
 *  FUNCTION: PrvUseFreeChunk
 *
 *  DESCRIPTION: Remove a chunk from the free list
 *
 *  PARAMETERS: the heap
 *					 the free chunk that points to this, or
 *						0 if it's the head or is unknown
 *					 the chunk to free
 *
 *  RETURNS: free chunk that pointed to the one used,
 *           or 0 if used chunk was at head
 *
 *  CREATED: 1998-05-14 
 *
 *  BY: Bob Ebert
 *
 *************************************************************/
static MemChunkHeaderPtr PrvUseFreeChunk(MemHeapHeaderPtr heapP, MemChunkHeaderPtr prevFreeChunkP,
													  MemChunkHeaderPtr chunkP)
{
	ErrNonFatalDisplayIf(!chunkP->free, "trying to use non-free chunk");

	ErrNonFatalDisplayIf(prevFreeChunkP && prevFreeChunkP->size == 0, "prev at end of heap");
	
	// if we didn't pass the prevFreeChunkP, find it
	// this is extra work if the chunkP is the first one, but the loop exits
	// after one iteration in that case, so it's not that bad
	if (!prevFreeChunkP) {
		MemChunkHeaderPtr nextChunkP = (MemChunkHeaderPtr)((UInt16 *)heapP + heapP->firstFreeChunkOffset);
		while (nextChunkP != chunkP) {
			ErrNonFatalDisplayIf(nextChunkP->size == 0, "free search hit end of heap");
			prevFreeChunkP = nextChunkP;
			nextChunkP = (MemChunkHeaderPtr)((UInt16 *)nextChunkP + nextChunkP->hOffset);
			}
		}

	ErrNonFatalDisplayIf(prevFreeChunkP && (MemChunkHeaderPtr) ((UInt16 *)prevFreeChunkP + prevFreeChunkP->hOffset) != chunkP,
								"prevFreeChunkP is wrong");
	ErrNonFatalDisplayIf(!prevFreeChunkP && (MemChunkHeaderPtr)((UInt16 *)heapP + heapP->firstFreeChunkOffset) != chunkP,
								"firstFreeChunkP wrong");
	
	// remove the chunk from the linked list
	if (prevFreeChunkP)
		prevFreeChunkP->hOffset += chunkP->hOffset;
	else
		heapP->firstFreeChunkOffset += chunkP->hOffset;
		
	// mark chunk used
	chunkP->free = false;
	
	return prevFreeChunkP;
}



/************************************************************
 *
 *  FUNCTION: PrvCreateFreeChunk
 *
 *  DESCRIPTION: Create a free chunk at a given place in the heap,
 * 				  insert it in free list, and merges following free chunks
 *					  (previous free chunks can't easily be merged, so they
 *					  are left for now.)
 *
 *  PARAMETERS: pointer to prevFreeChunkP, in case it comes in handy
 *
 *  RETURNS: free chunk that points to newly freed chunk (in case of merge)
 *           or 0 if new chunk was at head
 *
 *  CREATED: 1998-05-14 
 *
 *  BY: Bob Ebert
 * 		mchen	10/26/99 changed to have the free list always be
 *								sorted and to merge a new free chunk with
 *								existing free chunks whenever possible.
 *
 *************************************************************/
static MemChunkHeaderPtr PrvCreateFreeChunk(MemHeapHeaderPtr heapP, MemChunkHeaderPtr prevFreeChunkP,
														  MemChunkHeaderPtr chunkP, UInt32 size)
{

	MemChunkHeaderPtr				freeChunkP;
	MemChunkHeaderPtr				nextChunkP;
	
	if (prevFreeChunkP == 0) {
		// no given previous free chunk to add after, we do an insertion sort

		// Now add the chunk to the free list.  We want to keep the free list sorted
		// because it makes it easier to merge newly freed chunks into existing ones.
		// A sorted list also makes the goal of keeping handles in low memory and
		// ptrs in high memory more achievable.  While searching through the existing
		// free list for the insertion point, we look for the opportunity to merge.
		freeChunkP = (MemChunkHeaderPtr)((UInt16 *)heapP + heapP->firstFreeChunkOffset);
		nextChunkP = (MemChunkHeaderPtr)((UInt8 *)chunkP + size);
		while(freeChunkP->size != 0)
			{
			if (((UInt32)freeChunkP + freeChunkP->size) == (UInt32)chunkP)
				{
				// our chunk is immediately after freeChunkP, add our size to it to merge
				freeChunkP->size += size;
				// so that error checking code can detect free'ing of an already freed
				// chunk, we set the bits here
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
				memInitFreeChunkHeader(chunkP, 1, 0);
#endif
				// check for the special case that the next free chunk can be merged as
				// a result of this free, in which case we will merge with it too
				chunkP = (MemChunkHeaderPtr)((UInt16*)freeChunkP + freeChunkP->hOffset);
				if (chunkP->free && (((UInt32)freeChunkP + freeChunkP->size) == (UInt32)chunkP))
					{
					// merge the next free list entry too
					freeChunkP->size += chunkP->size;
					freeChunkP->hOffset += chunkP->hOffset;
					}
				return 0;
				}
			else if (nextChunkP == freeChunkP)
				{
				// our chunk is immediately before freeChunkP
				UInt32 offset = size >> 1;
				if (prevFreeChunkP) 
					{
					// fixup the pointer to freeChunkP to point to chunkP
					prevFreeChunkP->hOffset -= offset;
					}
				else
					{
					// freeChunkP was the first free chunk so we fix up the heapP instead
					heapP->firstFreeChunkOffset -= offset;
					}
				// just merging freeChunkP and chunkP, we should never have to check about
				// the possibilty of merging with the free chunk pointing to freeChunkP
				// because this would have been handled by the above case
				memInitFreeChunkHeader(chunkP, (size + freeChunkP->size), 
											  (offset + freeChunkP->hOffset));
				return 0;
				}
			else if (chunkP < freeChunkP) 
				{
				// this is the insertion point, break
				break;
				}
			prevFreeChunkP = freeChunkP;
			freeChunkP = (MemChunkHeaderPtr)((UInt16 *)freeChunkP + freeChunkP->hOffset);
			}
		}
	
	// ryw: the next line is a fragmentation check by Roger
	//ErrNonFatalDisplayIf(prevFreeChunkP && freeChunkP != prevFreeChunkP && heapP == MemHeapPtr(memDynamicHeaps - 1), "Fragmenting memory");
	
	// if we passed in a prevFreeChunkP, (or found one during merge)
	// then put the new free chunk in after it
	if (prevFreeChunkP) {
		memInitFreeChunkHeader(chunkP, size,
						prevFreeChunkP->hOffset - ((UInt16 *)chunkP - (UInt16 *)prevFreeChunkP));
		prevFreeChunkP->hOffset = (UInt16 *)chunkP - (UInt16 *)prevFreeChunkP;
		}
	else
		{
		// otherwise insert at the beginning of the free list (faster)
		memInitFreeChunkHeader(chunkP, size,
						heapP->firstFreeChunkOffset - ((UInt16 *)chunkP - (UInt16 *)heapP));
		heapP->firstFreeChunkOffset = (UInt16 *)chunkP - (UInt16 *)heapP;
		}


	return prevFreeChunkP;
}




/************************************************************
 *
 *  FUNCTION: PrvChunkNew, private
 *
 *  DESCRIPTION: Allocates a new chunk in the given heap.  Zero-size allocations
 *						are not allowed.
 *
 *						If zero is passed for size, no allocation takes place and NULL
 *						is returned.  The EC version will trigger a failed assertion
 *						when size of zero is passed.
 *
 *  PARAMETERS: heapID, size of chunk, chunk attribytes
 *		The chunk attributes can be one or more of:
 *			memNewChunkFlagNonMovable
 *			memNewChunkFlagParAlign
 *			ORed with the owner ID
 *			
 *
 *  RETURNS: ChunkID, or 0 if error
 *
 *  CREATED: 10/25/94 
 *
 *  BY: Ron Marianetti
 *
 *		vmk	10/3/97	Add memMaxChunkAllocSize check
 *		vmk	12/12/97	Added non-fatal assertion on size = 0
 *		bob 	06/18/98	Rewrote to use free list
 *		art	05/05/98	Removed 64k allocation limit.
 *		scl 	08/21/98	Conditionalized 64K limit (removed ONLY for MakeCard)
 *		kwk	08/27/98	Also removed 64K limit for Instant Karma simulator.
 *		bob	07/29/99	64K limit bypassed with memNewChunkFlagAllowLarge.
 *		ADH	10/15/99 Added a free list implementation to the master
 *							pointer table.
 *
 *************************************************************/
static void * PrvChunkNew(UInt16 heapID, UInt32 reqSize, UInt16 attributes)
{
	UInt8							* p;
	MemHeapHeaderPtr		  	heapP;

	MemChunkHeaderPtr			chunkP, lastBigChunkP;
	MemChunkHeaderPtr			prevFreeChunkP, lastBigChunkPrevFreeChunkP;

	Boolean						compacted = false;
	UInt32						oldSize;
	void *						retP=0;
	Boolean						atEnd;
	UInt32						actSize;
	
	UInt32 *						ptrP=0;
	MemMstrPtrTablePtr		mstrTblP;
	Int32							i;
	
	UInt32						size;

	UInt32 *						firstEntryP;
	UInt32 						offsetToEntry;

	if (reqSize == 0)
		return 0;
		
	// skip check for too-large allocations if appropriate attribute is set
	if ((attributes & memNewChunkFlagAllowLarge) == 0)
		if (reqSize > memMaxChunkAllocSize)
			return 0;

	// Get a near heap pointer from the heap ID
	heapP = MemHeapPtr(heapID);

	// the Actual size of the new chunk must include the chunk header
	if (reqSize & 0x01) 
		actSize = reqSize + 1 + sizeof(MemChunkHeaderType);
	else 
		actSize = reqSize + sizeof(MemChunkHeaderType);
		


	//------------------------------------------------------------
	// If it's a movable chunk (!atEnd), find a free offset entry in the
	//  offset table and setup offsetP to point to it.
	//------------------------------------------------------------
	lastBigChunkP = 0;
	if (attributes & memNewChunkFlagNonMovable) {
		if (attributes & memNewChunkFlagAtStart)
			atEnd = false;
		else
			atEnd = true;
		}

	else {
		if (attributes & memNewChunkFlagAtEnd)
			atEnd = true;
		else
			atEnd = false;
		
		//Get the first master table pointer entry.
		mstrTblP = &heapP->mstrPtrTbl;
		
		while (true)	{
		
			//Get the first entry in the table.
			firstEntryP = (UInt32 *) ( (UInt8 *)mstrTblP + sizeof(MemMstrPtrTableType) );
		  
			// Verify that the first element in the master pointer table has the high bit set.  
			ErrFatalDisplayIf ((*firstEntryP) & mstrPtrTblFirstEntryMask != mstrPtrTblFirstEntryMask, 
						"Master Table first entry is invalid");
		
			// The offset being the offset from the first element in the table to the
			// first available entry.  
			offsetToEntry = *firstEntryP & ~mstrPtrTblFirstEntryMask;
		
			// If the first entry does not point to the table terminator then go ahead
			// and take it.			
			if (offsetToEntry != 0)	{
		
				// Set ptrP to the first available entry.
				ptrP = (UInt32*) ((UInt32)firstEntryP + offsetToEntry);
			
				ErrFatalDisplayIf(*ptrP & mstrPtrTblFreeEntryMask != mstrPtrTblFreeEntryMask,
						"Master pointer table entry is not free.");
						
				// Set the first entry to point to the next entry following *ptrP
				// Or in first element and free element protection bits to the next
				// available offset.
				*firstEntryP = *ptrP | mstrPtrTblFirstEntryMask;
				break;
			}
			
			// If the first available table entry is full then proceed to the next table.
			else {
		
				if (mstrTblP->nextTblOffset) {
					mstrTblP = (MemMstrPtrTablePtr) ( (UInt8 *)heapP + mstrTblP->nextTblOffset );
					continue;
				}
				
				// There is no next table so we have to make one.
				else {
	
					// Allocate another offset table
					p = MemChunkNew(heapID, 
							sizeof(MemMstrPtrTableType) + memMstrPtrTableGrowBy*sizeof(void *), 
							memNewChunkFlagNonMovable);
					if (!p) goto Exit;
				
					// Set the owner to memOwnerMasterPtrTbl for tracking purposes
					chunkP = memChunkHeader(p);
					chunkP->owner = memOwnerMasterPtrTbl;
				
					// Link the new master pointer table in.
					mstrTblP->nextTblOffset = (UInt32)p - (UInt32)heapP;
					mstrTblP = (MemMstrPtrTablePtr)(p);
					mstrTblP->numEntries = memMstrPtrTableGrowBy;
					mstrTblP->nextTblOffset = 0;
				
					// Get the pointer to the first element of the table.
					firstEntryP = (UInt32 *) ( (UInt8 *)mstrTblP + sizeof(MemMstrPtrTableType) );
							
					offsetToEntry = sizeof (UInt32 *);
					
					// We already know that we are going to allocate the first available entry
					// for ptrP.
					ptrP =  (UInt32*) ((UInt32)firstEntryP + offsetToEntry);
				
					offsetToEntry += sizeof (UInt32 *);
		
					// The first element of the table has the offset to the first available
					// element.  The high and low bits are set to signify that this is the first
					// element in the table and that it is not an allocated entry.  The value
					// is constant because this is a newly created table and we know what the value
					// of the first entry is.  
					*firstEntryP++ = offsetToEntry | mstrPtrTblFirstEntryMask;			
						
					//Increment past the entry we are going to allocate.		
					firstEntryP++;							
					
					// Initialize the table.  The first entry and the last two are special cases.
					// Each element in the table is set to the offset from the front of the table
					// of the next available entry.
					for (i = 2, offsetToEntry += sizeof (UInt32 *); 
									i < memMstrPtrTableGrowBy - 1; 
									i++, offsetToEntry += sizeof (UInt32*) ) {
				
						// Set the low bit on each free entry so that it will not be confused 
						// with an allocated entry.  Set each entry to the offset of the next
						// available entry.
						*firstEntryP++ = offsetToEntry | mstrPtrTblFreeEntryMask;
					}	
					
					// Set the last element in the table to point to zero.
					*firstEntryP = mstrPtrTblFreeEntryMask;
					
					break;
				}
			}
		}
	}
	
	//------------------------------------------------------------
	// Look for a free chunk big enough to hold the request
	//------------------------------------------------------------
	// Start at the first free chunk
	prevFreeChunkP = 0;
	chunkP = (MemChunkHeaderPtr)((UInt16 *)heapP + heapP->firstFreeChunkOffset);
	size = chunkP->size;

	// Look for a free chunk big enough to hold the request
	while(1) {
	
		ErrNonFatalDisplayIf(chunkP->size && !chunkP->free, "Non-free chunk in free list!");

		// If we're at the end of the heap, take special action
		if (size == 0) {

			// If we're intentionally trying to allocate at the end of the heap,
			//  break out if we've already found a free chunk big enough
			if (lastBigChunkP) {
				chunkP = lastBigChunkP;
				prevFreeChunkP = lastBigChunkPrevFreeChunkP;
				break;
				}

			// Compact the heap and try again
			if (!compacted) {
				PrvCompactHeap(heapP);
				chunkP = (MemChunkHeaderPtr)((UInt16 *)heapP + heapP->firstFreeChunkOffset);
				prevFreeChunkP = 0;
				size = chunkP->size;
				compacted = true;
				lastBigChunkP = 0;
				continue;
				}
			else
				goto Exit;
			}
		
		// if it's big enough...
		if (size >= actSize) {
			if (!atEnd)
				break;		// use the first chunk we find
			
			// Trying to allocate at the end, so record this chunk
			// pointer as the lastBigChunkP if it's higher in the heap,
			// and keep searching for an even later chunk
			if (chunkP > lastBigChunkP) {
				lastBigChunkP = chunkP;
				lastBigChunkPrevFreeChunkP = prevFreeChunkP;
				}
			}
			
		// go on to next free chunk
		prevFreeChunkP = chunkP;
		chunkP = (MemChunkHeaderPtr) ((UInt16 *)chunkP + chunkP->hOffset);
		size = chunkP->size;
		}

	// We found a chunk to break up, now do it
	oldSize = chunkP->size;
	
		
	//------------------------------------------------------------
	// Split up the free chunk into a smaller free chunk and
	//  the new chunk, or just reflag it as the new chunk if it's
	//  not big enough
	//------------------------------------------------------------

	// If allocating at the end, put the new chunk at the end
	// of the free chunk
	if (atEnd) {
		// If it's big enough, Chop the free block down to size and place
		//  the new chunk at the end of it
		if (oldSize > actSize + sizeof(MemChunkHeaderType))  {
			// shrink the free chunk so the space at the end can be used
			chunkP->size = oldSize - actSize;
			
			// get/setup the new chunk
			chunkP = (MemChunkHeaderPtr)((UInt8 *)chunkP + chunkP->size);
			memChunkFlagsClear(chunkP);
			chunkP->size = actSize;
			}
		else {
			// need the whole chunk, so take it out of the free list
			PrvUseFreeChunk(heapP, prevFreeChunkP, chunkP);
			actSize = oldSize;
			}
		}
	// if not at the end, take the chunk out of the free list
	else
		PrvUseFreeChunk(heapP, prevFreeChunkP, chunkP);

	
	// If non-movable chunk....
	if (attributes & memNewChunkFlagNonMovable) {	
		memChunkFlagsClear(chunkP);
		chunkP->sizeAdj =  (actSize - reqSize - sizeof(MemChunkHeaderType));
		chunkP->lockCount = memPtrLockCount;
		chunkP->owner = attributes & 0x0F;
		chunkP->hOffset = 0;
		retP = memChunkData(chunkP);
		}
		
	// Else if movable chunk...
	else {
		// Setup the flags for the new chunk
		chunkP->free = false;
		chunkP->sizeAdj = (actSize - reqSize - sizeof(MemChunkHeaderType));
		chunkP->owner = attributes & 0x0F;
		chunkP->hOffset = ((UInt32)chunkP - (UInt32)ptrP) >> 1;
		*ptrP = (UInt32)(memChunkData(chunkP));
		retP = ptrP;
		
		// If the user requested a pre-locked movable chunk, then lock it and fix
		// up the return pointer to point to the data and not the handle.
		if (attributes & memNewChunkFlagPreLock) {
			chunkP->lockCount++;
			retP = memChunkData(chunkP);
			}
		}
	
#ifdef CHECK_MEMSET_IS_NEEDED
	// Often chunks are allocated and then cleared (set to 0).  Make sure at least one byte of the chunk isn't clear.
	*(UInt8 *)(memChunkData(chunkP)) = (UInt8) memFillFreeValue;
#endif		
	
	// If we allocated at the front of the free chunk, break out the unused space
	//  at the end as a new free chunk 
	if (!atEnd) {
		// If the chunk was big enough to split, make a free chunk at the end
		if (oldSize > actSize + sizeof(MemChunkHeaderType))  {
			chunkP->size = actSize;
			chunkP = (MemChunkHeaderPtr)((UInt8 *)chunkP + actSize);
			PrvCreateFreeChunk(heapP, prevFreeChunkP, chunkP, oldSize-actSize);
			}
		// If it wasn't big enough to split, it's actual size was the old size
		//  so we need to fix up the flags field
		else
			chunkP->sizeAdj = oldSize - reqSize - sizeof(MemChunkHeaderType);
		}

Exit:
	
	// If Memory Debug is on, see if we need to record minimum free space in
	//  the dynamic heap
	if (GMemDebugMode) {
		if ((GMemDebugMode & memDebugModeRecordMinDynHeapFree) && heapID==0) {
			UInt32		freeBytes, maxChunk;
			MemHeapFreeBytes(heapID, &freeBytes, &maxChunk);
			if (freeBytes < GMemMinDynHeapFree || GMemMinDynHeapFree==0)
				GMemMinDynHeapFree = freeBytes;
			}
		}

	

	return retP;
}




/************************************************************
 *
 *  FUNCTION: PrvPtrFree, private
 *
 *  DESCRIPTION: Disposes of a chunk
 *
 *  PARAMETERS: Chunk ID
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 11/02/94 
 *
 *  BY: Ron Marianetti
 *		bob	6/18/98	Updated to maintain free list
 *		ADH	10/15/99 Added support for free list of master 
 *							pointer table entries.
 *		mchen 10/26/99 Changed to always add to free list without
 *							a parent so PrvCreateFreeChunk() can keep
 *							the free list sorted.
 *
 *************************************************************/
static Err		PrvPtrFree(void * p)
{
	MemChunkHeaderPtr				chunkP;
	UInt32 *							ptrP;
	UInt16 							heapID;
	MemHeapHeaderPtr 				heapP;
	
	UInt32 *							firstEntryP;
	

	// Get the chunk pointer header pointer
	chunkP = memChunkHeader(p);

	// Check validity, just exit if problems (?)
	if (chunkP->free) {
		ErrNonFatalDisplay( "Freeing a free chunk");
		goto Exit;
		}
	if (chunkP->size & 0x01) {
		ErrNonFatalDisplay( "Freeing Invalid chunk ptr");
		goto Exit;
		}

	// get heap pointer
	heapP = PrvHeapPtrAndID((void *)chunkP, &heapID);

	// If it had an entry in the master pointer table, then this was a handle.
	// Clear the entry.
	if (chunkP->hOffset)  {
	
		// Locate the entry that we are going to free.
		ptrP = (UInt32*)( (Int32)chunkP - ((Int32)chunkP->hOffset << 1) );
		
		firstEntryP = ptrP;
		
		// Work our way backwards until we find the first entry to this table.
		// The first element of the table will have its high bit set.	 
		while ( ((UInt32)(*firstEntryP) & mstrPtrTblFirstEntryMask) != mstrPtrTblFirstEntryMask)
			--firstEntryP;
	
		// Set the freed entry to point to the free entry that the first entry was pointing to. 
		// Remove the first element bit.  Leave the unallocated entry bit set.   
		*ptrP = ( (UInt32)(*firstEntryP) & ~mstrPtrTblFirstEntryMask) | mstrPtrTblFreeEntryMask;

		// Set the first entry to point to the freed entry.  Set the first entry and unallocated
		// bits.  
		*firstEntryP = ( ( (ptrP - firstEntryP) * sizeof(UInt32*) ) | mstrPtrTblFirstEntryMask);
	}

	PrvCreateFreeChunk(heapP, 0, chunkP, chunkP->size);

Exit:

	return 0;
}




/************************************************************
 *
 *  FUNCTION: PrvPtrUnlock
 *
 *  DESCRIPTION: Unlocks a chunk given a pointer to the chunk
 *
 *  PARAMETERS: pointer to chunk
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 11/17/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
static Err	PrvPtrUnlock(const void * p)
{
	MemChunkHeaderPtr				chunkP;


	// Check for error
	ErrFatalDisplayIf(p==0, "nil Chunk ptr");

	// Get pointer to the chunk header
	chunkP = memChunkHeader(p);
	
	// If it's a non-movable chunk, ignore
	if (!chunkP->hOffset) goto Exit;

	// Decrement lock count
	if (chunkP->lockCount  != memPtrLockCount) {
		if (chunkP->lockCount) 
			chunkP->lockCount--;  
		else {
			ErrDisplay("Chunk under-locked");
			}
		}
		
Exit:
	return 0;
}




/************************************************************
 *
 *  FUNCTION: PrvPtrResize
 *
 *  DESCRIPTION: Resizes a chunk
 *
 *		The original implementation accidentally allowed reallocation
 *		to size zero.  To rectify this situation, passing a new size of
 *		zero will cause an assertion to fail in the debug build PalmOS v3.x.
 *
 *  PARAMETERS: chunk data pointer, new size
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 11/10/94 
 *
 *  BY: Ron Marianetti
 *
 *		vmk	10/3/97	Added memMaxChunkAllocSize check
 *		vmk	12/12/97	Added non-fatal assertion on newSize = 0
 *		bob	6/18/98	Updated to use free list
 *		mchen 10/26/99 Changed to call PrvCreateFreeChunk() without
 *							a parent in the case where we move existing
 *							data to a new one for resize.
 *
 *************************************************************/
static Err PrvPtrResize(void * chunkDataP, UInt32 newSize)
{
	MemHeapHeaderPtr		heapP;
	Err						err=0;
	UInt32					oldActSize;
	UInt32					newActSize;
	UInt32					freeSpace;
	MemChunkHeaderPtr		chunkP, freeChunkP, tempChunkP, adjacentChunkP;
	Boolean					compacted;


	// Check for error
	ErrFatalDisplayIf(chunkDataP==0, "nil Chunk ptr");

	// Since zero size allocs are discouraged and are likely bugs, try to discourage
	// developers from reallocating to zero size (unfortunately,
	// earlier versions, 1.0 and 2.0, allowed reallocs to size zero;
	// in the interest of not breaking existing 3rd party apps without
	// sufficient warning, we're adding this non-fatal assertion)
	// DOLATER... in 4.0, consider disallowing zero-size reallocs
	//ErrNonFatalDisplayIf(newSize == 0, "zero-size realloc");

	// Get pointer to the chunk header and heap it's in
	chunkP = memChunkHeader(chunkDataP);
	{
	UInt16 heapID;
	heapP = 	PrvHeapPtrAndID(chunkDataP, &heapID);
	}

	// Figure out the new actual size
	oldActSize = chunkP->size;
	if (newSize & 0x01)
		newActSize = newSize + 1 + sizeof(MemChunkHeaderType);
	else
		newActSize = newSize + sizeof(MemChunkHeaderType);

	// allow resizing of oversize chunks if they were previously oversized
	if ( newSize > memMaxChunkAllocSize && 
	 	  (oldActSize - sizeof(MemChunkHeaderType) - chunkP->sizeAdj) <= memMaxChunkAllocSize) {
		err = memErrInvalidParam;
		goto Exit;
		}


	//------------------------------------------------------------
	// Are we shrinking (or growing less than chunkP->sizeAdj)? It's easy..
	//------------------------------------------------------------
	if (newActSize <= oldActSize) {

		// If it's big enough, create a free chunk at the end...
		if (oldActSize >= newActSize + sizeof(MemChunkHeaderType)) {
			chunkP->size = newActSize;
			freeChunkP = (MemChunkHeaderPtr)((UInt8 *)chunkP + newActSize);
			PrvCreateFreeChunk(heapP, 0, freeChunkP, oldActSize - newActSize);
			}
		else
			newActSize = oldActSize;

		// Adjust the requested size
		chunkP->sizeAdj = (newActSize - newSize - sizeof(MemChunkHeaderType));
		goto Exit;
		}


	//------------------------------------------------------------
	// We're growing...
	//------------------------------------------------------------

	// If the next chunk is free, consider growing in place
	adjacentChunkP = (MemChunkHeaderPtr)((UInt8 *)chunkP + chunkP->size);

#ifdef USE_RISKY_PERFORMANCE_TRICK	
	// if the next chunk is not free, but is smaller than the one we have to grow,
	// move it instead to save a little time
	if (adjacentChunkP->size &&
		 !adjacentChunkP->free &&
		 adjacentChunkP->size < oldActSize &&
		 adjacentChunkP->size > (newActSize - oldActSize) &&
		 adjacentChunkP->lockCount == 0)
		{
			// start at the beginning of the free list
			tempChunkP = 0;
			freeChunkP = (MemChunkHeaderPtr)((UInt16 *)heapP + heapP->firstFreeChunkOffset);

			// walk free list looking for large enough chunk
			while (freeChunkP->size) {
				ErrNonFatalDisplayIf(freeChunkP->size && !freeChunkP->free, "non-free chunk in free list");

				// found one, use it!
				if (freeChunkP->size >= adjacentChunkP->size) {
					(((LowMemHdrType*)PilotGlobalsP)->globals.sysReserved31DWord1)++;
					freeSpace = freeChunkP->size;
					tempChunkP = PrvUseFreeChunk(heapP, tempChunkP, freeChunkP);
					
					// copy the data
					PrvMoveChunk(freeChunkP, adjacentChunkP);
					
					// create a new free chunk if necessary
					if (freeSpace >= freeChunkP->size + sizeof(MemChunkHeaderType)) {
						freeSpace -= freeChunkP->size;
						freeChunkP = (MemChunkHeaderPtr)((UInt8 *)freeChunkP + freeChunkP->size);
						PrvCreateFreeChunk(heapP, tempChunkP, freeChunkP, freeSpace);
						}
					
					// or just mark the space used if not
					else {
						freeChunkP->sizeAdj += freeSpace - freeChunkP->size;
						freeChunkP->size = freeSpace;
						}
					
					// set up values for growing in place
					freeChunkP = adjacentChunkP;
					freeSpace = adjacentChunkP->size;
					goto GrowInPlace;
					}
				
				tempChunkP = freeChunkP;
				freeChunkP = (MemChunkHeaderPtr)((UInt16 *)freeChunkP + freeChunkP->hOffset);
				}
		}
#endif	// USE_RISKY_PERFORMANCE_TRICK
	
	if (adjacentChunkP->size && adjacentChunkP->free) {
	
		freeChunkP = adjacentChunkP;
		
		// take the opportunity to merge adjacent free chunks
		// DOLATER this may mess with free list putting large chunk
		// in a position other than the end, so try to fix that
		tempChunkP = (MemChunkHeaderPtr)((UInt8 *)freeChunkP + freeChunkP->size);
		while (tempChunkP->size && tempChunkP->free) {
			PrvUseFreeChunk(heapP, 0, tempChunkP);
			freeChunkP->size += tempChunkP->size;
			tempChunkP = (MemChunkHeaderPtr)((UInt8 *)tempChunkP + tempChunkP->size);
			}
		
		freeSpace = freeChunkP->size;
		}
	else
		freeSpace = 0;

	// if there's enough space to grow in place, use the free chunk
	if (oldActSize + freeSpace >= newActSize) {
		ErrNonFatalDisplayIf(freeSpace == 0, "How did we get here?");
		tempChunkP = PrvUseFreeChunk(heapP, 0, freeChunkP);
		// goto GrowInPlace;
	}

	// If there's not enough free space following the chunk, we must
	//  look for a free chunk somewhere else in the heap and move our chunk there
	else {
		// make sure it's movable
		if (chunkP->lockCount) {
			err = memErrChunkLocked;
			goto Exit;
			}

		// Look for a free chunk big enough somewhere in the heap
		compacted = false;
		while (1) {

			tempChunkP = 0;
			freeChunkP = (MemChunkHeaderPtr)((UInt16 *)heapP + heapP->firstFreeChunkOffset);

			// walk free list looking for large enough chunk
			// don't bother trying to merge here, too expensive, let heap compaction handle that
			while (freeChunkP->size) {
				ErrNonFatalDisplayIf(freeChunkP->size && !freeChunkP->free, "non-free chunk in free list");

				if (freeChunkP->size >= newActSize)
					goto UseNextChunk;
					
				tempChunkP = freeChunkP;
				freeChunkP = (MemChunkHeaderPtr)((UInt16 *)freeChunkP + freeChunkP->hOffset);
				}
			
			// If we've already compacted, bail
			if (!compacted) {
				MemChunkHeaderPtr *chunkH = (void **)((UInt32)chunkP - ((Int32)chunkP->hOffset << 1));
				PrvCompactHeap(heapP);
				chunkP = memChunkHeader(*chunkH);
				compacted = true;
				}
			else {
				err = memErrNotEnoughSpace;
				goto Exit;
				}
			}
			
UseNextChunk:

		// remove the free chunk from the free list
		PrvUseFreeChunk(heapP, tempChunkP, freeChunkP);
		freeSpace = freeChunkP->size - oldActSize;

		// Move the chunk contents (deal with the handle)
		PrvMoveChunk(freeChunkP, chunkP);

		// mark the old chunk free, insert it without a previous to keep the
		// free list sorted
		PrvCreateFreeChunk(heapP, 0, chunkP, chunkP->size);
		
		// the parent of the chunk we will put back below (in GrowInPlace) is either
		// the chunk we just inserted or the original parent, depending on who is
		// closer.  Also, the chunk we just inserted might have been merged with
		// the original parent!  It is hard to determine a merge because we don't
		// currently clear any bits in the merged header, so we just mark no parent
		// here.
		tempChunkP = 0;
		
		// move our pointer to the relocated chunk
		chunkP = freeChunkP;
		}	// moved chunk to some other heap location
			


GrowInPlace:
	// -----------------------------------------------------------------
	// There is now guaranteed to be enough free space to grow the chunk,
	// freeSpace has the # of bytes,  and that space is NOT in the free list
	// and tempChunkP is a pointer into the free list where a newly created
	// chunk (if any) will be added
	ErrNonFatalDisplayIf(oldActSize + freeSpace < newActSize, "guarantee schmarentee!");
	ErrNonFatalDisplayIf(tempChunkP && !tempChunkP->free, "bad tempChunkP");

	// if there's room for a free chunk, make it
	if (oldActSize + freeSpace >= newActSize + sizeof(MemChunkHeaderType)) {
		freeChunkP = (MemChunkHeaderPtr)((UInt8 *)chunkP + newActSize);
		PrvCreateFreeChunk(heapP, tempChunkP, freeChunkP, freeSpace + oldActSize - newActSize);
		}
	
	// otherwise there's barely enough room
	else 
		newActSize = oldActSize + freeSpace;
	
	// Wipe the new portion of the chunk so that references to data there are more invalid.
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	MemSet((UInt8 *)chunkP + oldActSize, newActSize - oldActSize, 0x55);
#endif
	
	// finish up the chunk we've resized
	chunkP->size = newActSize;
	chunkP->sizeAdj = (newActSize - sizeof(MemChunkHeaderType) - newSize);
	ErrNonFatalDisplayIf(chunkP->size - chunkP->sizeAdj - sizeof(MemChunkHeaderType) != newSize,
							"resize failed");


	// Return
Exit:

	return err;

}




/************************************************************
 *
 *  FUNCTION: PrvHeapScramble
 *
 *  DESCRIPTION: Does a single scramble pass on the heap and returns
 *		the number of chunks it was able to relocate.
 *
 *		For each movable chunk with moved bit clear {
 *			if chunk follows a free chunk
 *				 shift it up into free chunk area
 *				 set moved bit in flags
 *			else 
 *				 if possible to reallocate
 *					 reallocate chunk and move to new location
 *					 set moved bit in flags
 *			}
 *		
 *  PARAMETERS: 
 *		heapP				- pointer to heap header
 *		firstOffset 	- offset to first chunk in heap
 *		lastOffset		- offset to last chunk in heap
 *
 *	 CALLED BY: MemHeapScramble
 *
 *  RETURNS: # of chunks relocated.
 *
 *  CREATED: 7/4/95
 *
 *  BY: Ron Marianetti
 *		Bob	6/18/98	Updated to keep free list in sync
 *
 *************************************************************/
static UInt16	PrvHeapScramble(MemHeapHeaderPtr heapP, UInt32 firstOffset, UInt32 /* lastOffset */)
{
	MemChunkHeaderPtr			prevChunkP, chunkP, nextChunkP;
	MemChunkHeaderPtr			newChunkP, freeChunkP, prevFreeChunkP;
	void **						chunkH;
	UInt16						scrambles = 0;
	UInt32						freeSize, chunkSize, reqSize;
	
	// Start the loop at 0
	chunkP = 0;
	prevChunkP = 0;
	nextChunkP = (MemChunkHeaderPtr)((UInt8 *)heapP + firstOffset);
	
	// Walk through the chunks
	while(1) {
	
		// Point to next chunk
		prevChunkP = chunkP;
		chunkP = nextChunkP;
		if (!chunkP->size) break;		// 0 at end of heap
		nextChunkP = (MemChunkHeaderPtr)((UInt8 *)chunkP + chunkP->size);
		
		// If this chunk was already moved, or is a free chunk, skip it
		if (chunkP->moved || chunkP->free ) continue;
		
		// If this chunk is not movable, skip it
		if (chunkP->lockCount) continue;
		
		// Determine the handle location for this chunk
		chunkH = (void**)((UInt8 *) chunkP - ((Int32)chunkP->hOffset*2));
			
			
		//------------------------------------------------------------------
		// If the previous chunk is free, move this one into that space
		//------------------------------------------------------------------
		if (prevChunkP) {
			if (prevChunkP->free) {
			
				prevFreeChunkP = PrvUseFreeChunk(heapP, 0, prevChunkP);

				freeSize = prevChunkP->size;
				chunkSize = chunkP->size;
				MemMove(prevChunkP, chunkP, chunkSize);
				
				// Point to the new location of the chunk and the free chunk
				newChunkP = prevChunkP;
				chunkP = (MemChunkHeaderPtr)((UInt8 *)prevChunkP + chunkSize);
				
				// Fix up the handle of the chunk and set the moved bit
				newChunkP->hOffset = ((UInt32)newChunkP - (UInt32)chunkH) >> 1;
				*chunkH = memChunkData(newChunkP);
				newChunkP->moved = true;
				scrambles++;
				
				// Free the old chunk
				PrvCreateFreeChunk(heapP, 0, chunkP, freeSize);
		
				// chunks may have been merged, invalidating nextChunkP, so re-generate it
				nextChunkP = (MemChunkHeaderPtr)((UInt8 *)chunkP + chunkP->size);

				// All done
				continue;
				}
			} // prevChunkP
			
			
		//------------------------------------------------------------------
		// If possible, re-allocate this chunk somewhere else in the heap
		//------------------------------------------------------------------
		freeChunkP = (MemChunkHeaderPtr)((UInt16 *)heapP + heapP->firstFreeChunkOffset);
		prevFreeChunkP = 0;
		reqSize =  chunkP->size + sizeof(MemChunkHeaderType);
		while (freeChunkP->size) {
		
			// If there's an available free chunk, and big enough, break out
			if (freeChunkP->size >= reqSize) break;
				
			// Next chunk
			prevFreeChunkP = freeChunkP;
			freeChunkP = (MemChunkHeaderPtr)((UInt16 *)freeChunkP + freeChunkP->hOffset);
			} 
			
		// If we didn't find a vacancy, skip it
		freeSize = freeChunkP->size;
		if (!freeSize) continue;
		
		// DOLATER ??? - Could this be PrvMoveChunk?
		
		// Remove free chunk from free list
		PrvUseFreeChunk(heapP, prevFreeChunkP, freeChunkP);

		// Move this chunk to the free space and fix up it's handle field
		MemMove(freeChunkP, chunkP, chunkP->size);
		newChunkP = freeChunkP;
		newChunkP->hOffset = ((UInt32)newChunkP - (UInt32)chunkH) >> 1;
		*chunkH = memChunkData(newChunkP);
		newChunkP->moved = true;
		scrambles++;
		
		if ((freeSize - newChunkP->size) >= sizeof(MemChunkHeaderType)) {
			// Mark the extra free chunk at the end of the destination
			freeChunkP = (MemChunkHeaderPtr)((UInt8 *)newChunkP + newChunkP->size);
			PrvCreateFreeChunk(heapP, prevFreeChunkP, freeChunkP, freeSize - newChunkP->size);
			}
		else {
			// not enough room for extra free chunk, so just mark space used
			newChunkP->sizeAdj = freeSize - newChunkP->size - sizeof(MemChunkHeaderType);
			}
		
		// Mark the current chunk as free
		PrvCreateFreeChunk(heapP, 0, chunkP, chunkP->size);
		
		// chunks may have been merged, invalidating nextChunkP, so re-generate it
		nextChunkP = (MemChunkHeaderPtr)((UInt8 *)chunkP + chunkP->size);
		} 
					
	// Return number of chunks we moved
	return	scrambles;
}




#if EMULATION_LEVEL != EMULATION_NONE
/************************************************************
 *
 *  FUNCTION: PrvHostToTargetOffset, private
 *
 *  DESCRIPTION: Used only under Emulation mode. We need this
 *   routine when we try and craft a ROM image for use
 *   on the device, but we don't have enough RAM on the host to
 *   contain the entire card's address space. 
 *
 *   For example, a memory card might have only 1 Meg of RAM starting at
 *   address 0 but it's ROM might start at the 8 MByte boundary
 *   or even higher. When this is the case, we "compress" the card image
 *   and place the ROM image immediately after the RAM area in the host
 *   memory. 
 *
 *   For compressed memory cards then, we must add a shift amount
 *   to ROM pointers when calculating a LocalID that can be used on
 *   the device. This shift amount is the targetROMShift field in the
 *   CardInfoType for the card and gives the difference between the
 *   Emulated location of the ROM and the final desired location of the ROM
 *
 *  PARAMETERS: offset to chunk as used on host, CardInfoPtr
 *
 *  RETURNS: offset to chunk that can be used on target.
 *
 *  CREATED: 5/3/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
static UInt32 PrvHostToTargetOffset(UInt32 hostOffset, CardInfoPtr infoP)
{
	// If it's in the ROM area, we must add a shift amount to it
	if (hostOffset >= infoP->cardHeaderOffset - infoP->targetROMShift)
		return hostOffset + infoP->targetROMShift;
		
	else
		return hostOffset;
}




/************************************************************
 *
 *  FUNCTION: PrvTargetToHostOffset
 *
 *  DESCRIPTION: Used only under Emulation mode. We need this
 *   routine when we try and craft a ROM image for use
 *   on the device, but we don't have enough RAM on the host to
 *   contain the entire card's address space. 
 *
 *   For example, a memory card might have only 1 Meg of RAM starting at
 *   address 0 but it's ROM might start at the 8 MByte boundary
 *   or even higher. When this is the case, we "compress" the card image
 *   and place the ROM image immediately after the RAM area in the host
 *   memory. 
 *
 *   For compressed memory cards then, we must add a shift amount
 *   to ROM pointers when calculating a LocalID that can be used on
 *   the device. This shift amount is the targetROMShift field in the
 *   CardInfoType for the card and gives the difference between the
 *   Emulated location of the ROM and the final desired location of the ROM
 *
 *  PARAMETERS: offset to chunk that can be used on target, CardInfoPtr
 *
 *  RETURNS: offset to chunk that can be used on the host
 *
 *  CREATED: 5/3/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
static UInt32 PrvTargetToHostOffset(UInt32 targetOffset, CardInfoPtr infoP)
{	
	// If this offset is in the ROM area, we must subtract the targetROMShift
	if (targetOffset >= infoP->cardHeaderOffset)
		return targetOffset - infoP->targetROMShift;
		
	else 	
		return targetOffset;
}

#endif // EMULATION_LEVEL != EMULATION_NONE




/************************************************************
 * PRIVATE
 *-------------------------------------------------------------
 *  FUNCTION: PrvDebugAction, private
 *
 *  DESCRIPTION: Called by various Memory Manager routines when
 *   GMemDebugMode is not nil in order to scramble the heap,
 *   check the heap, or fill free chunks, etc.
 *
 *   NOTE: If p is not nil, it is used to determine the heapID,
 *				else if h is not nil, it is used to determine the heapID,
 *				else heapID has the heap ID
 *
 *	  This routine checks the flags in GMemDebugMode and takes the
 *		appropriate action given:
 *
 *   If postModify is true, it will simply fill in the free chunks with
 *		initialized data.
 *
 *	  Else {
 *		 It will check the appropriate heap(s).
 *   	 If preModify is true, it will scramble the appropriate heap(s).
 *		 }
 *
 *
 *  PARAMETERS: 
 *   NOTE: the heap I
 *		heapID 			- which heap to act on
 *		p		 			- pointer to chunk in heap to act on, 
 *		h					- handle to chunk in heap to act on
 *		alwaysScramble - If true, heap(s) will be scrambled even if 
 *								memDebugModeScrambleOnAll is false.
 *		postModify		- true when called from end of a routine that 
 *								just modified the heap. This tells us to simply re-fill free chunks
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 5/19/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
static void PrvDebugAction(UInt16 heapID, void * p, MemHandle h, Boolean alwaysScramble,
					Boolean postModify)
{
	Err			err = 0;
	CardInfoPtr	infoP;
	UInt16		cardNo;
	Boolean		check=false, scramble=false;
	UInt16		heapNo;
	void **		uH;

	// Wait for ownership
	MemSemaphoreReserve(false);
	
	// If a chunk pointer is specified, use that to get the heap ID
	if (p) heapID = MemPtrHeapID(p);
	else if (h) {
		uH = memHandleUnProtect(h);
		heapID = MemPtrHeapID(*uH);
		}

	// If the heap has just been modified, re-fill free chunks
	if (postModify) {
		if (GMemDebugMode & memDebugModeFillFree)
			PrvFillFreeChunks(heapID);
		}
		
	else {
		if (GMemDebugMode & memDebugModeCheckOnAll  ||
			 	((GMemDebugMode & memDebugModeCheckOnChange) && alwaysScramble))
			 check = true;
			 
		if (GMemDebugMode & memDebugModeScrambleOnAll  ||
			 ((GMemDebugMode & memDebugModeScrambleOnChange) && alwaysScramble))
			 scramble = true;
			 
			 
		//-----------------------------------------------------------------------
		// Loop through all heaps
		//-----------------------------------------------------------------------
		if (GMemDebugMode & memDebugModeAllHeaps) {
			infoP = memCardInfoP(0);
			for (cardNo=0; cardNo < GMemCardSlots; cardNo++, infoP++) {
				if (!infoP->size) continue;
				for (heapNo = 0; heapNo < MemNumRAMHeaps(cardNo); heapNo++) {
					heapID = MemHeapID(cardNo, heapNo);
					
					if (check)
						err = MemHeapCheck(heapID);
					if (err) goto Exit;
					
					if (scramble) {
						err = MemHeapScramble(heapID);
						if (GMemDebugMode & memDebugModeFillFree)
							PrvFillFreeChunks(heapID);
						}
	
					if (err) goto Exit;
					}
					
				} // cardNo
			
			}
			
		//-----------------------------------------------------------------------
		// Just the one heap
		//-----------------------------------------------------------------------
		else {
			// See if we need to check the heap
			if (check)
				err = MemHeapCheck(heapID);
			if (err) goto Exit;
				
			// See if we need to scramble the heap
			if (scramble) {
				err = MemHeapScramble(heapID);
				if (GMemDebugMode & memDebugModeFillFree)
					PrvFillFreeChunks(heapID);
				}
			}
		}
		
Exit:
	// Release ownership
	MemSemaphoreRelease(false);
	
	return;	
}




/************************************************************
 * PRIVATE
 *-------------------------------------------------------------
 *  FUNCTION: PrvFillFreeChunks, private
 *
 *  DESCRIPTION: Fills all free chunks in a heap with data
 *
 *  PARAMETERS: heapID
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 5/19/95
 *
 *  BY: Ron Marianetti
 *		Bob	6/18/98	Updated to take advantage of free list
 *
 *************************************************************/
static void PrvFillFreeChunks(UInt16 heapID)
{
	MemHeapHeaderPtr		   heapP;
	MemChunkHeaderPtr			chunkP;
	void *						chunkDataP;
	UInt16 *						wP;
	UInt32						count;
	
	MemSemaphoreReserve(true);
	
	// Get a near heap pointer from the heap ID
	heapP = MemHeapPtr(heapID);
	
	// If it's read only, don't do anything
	if (heapP->flags & memHeapFlagReadOnly) {
		return;
		}
		

	// Get pointer to first free chunk
	chunkP = (MemChunkHeaderPtr)((UInt16 *)heapP + heapP->firstFreeChunkOffset);
	
	// Walk through the free chunks
	do {

		// init it's contents
		chunkDataP = memChunkData(chunkP);
		wP = (UInt16 *)chunkDataP;
		count = chunkP->size - sizeof(MemChunkHeaderType);
		count = (count >> 1) + 1;
		while(--count) 
			*wP++ = memFillFreeValue;
			
		// On to next chunk
		chunkP = (MemChunkHeaderPtr) ((UInt16 *)chunkP + chunkP->hOffset);
		} while(chunkP->size);
	
	MemSemaphoreRelease(true);
}




/************************************************************
 *
 *  FUNCTION: PrvCurOwnerID, private
 *
 *  DESCRIPTION: Returns the owner ID of the current application by
 *    referencing the appInfo structure stored at 0(A5).
 *
 *  PARAMETERS: none
 *
 *  RETURNS: owner ID
 *
 *  CREATED: 4/12/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
static UInt16 PrvCurOwnerID(void)
{
	SysAppInfoPtr	unusedAppInfoP;
	
	
	return SysGetAppInfo(&unusedAppInfoP, &unusedAppInfoP)->memOwnerID;
}




/************************************************************
 *
 *  FUNCTION: PrvInitHeapPtr, private
 *
 *  DESCRIPTION: Initializes a block of RAM as a heap and fills it
 *		with one free chunk
 *
 *  PARAMETERS: heap pointer, heap size, number of handles
 *		to pre-allocate
 *
 *  NOTE: if numHandles is 0, it is assumed that the heap is a ROM heap
 *   so the read only bit will be set in the heap header
 *
 *  RETURNS: error code
 *
 *  CREATED: 10/25/94 
 *
 *  BY: Ron Marianetti
 *
 *		ADH	10/15/99 Added a free list implementation to the master
 *							pointer table.
 *
 *************************************************************/
static Err	PrvInitHeapPtr(MemHeapHeaderPtr heapP, UInt32 size, Int16 numHandles,
					Boolean initContents)
{
	MemChunkHeaderPtr	 		chunkP;
	UInt32 *						iP;
	
	register UInt32			free;
	UInt16							i;
	UInt32						offset;

	MemHeapTerminatorType*	termP;
	
	// Init the heap header
	if (numHandles == 0) 
		heapP->flags = memHeapFlagReadOnly | memHeapFlagVers4;
	else 
		heapP->flags = memHeapFlagVers4;
	
	heapP->size = size;
	heapP->mstrPtrTbl.numEntries = numHandles;
	heapP->mstrPtrTbl.nextTblOffset = 0;
	free = size - sizeof(MemHeapHeaderType)
				- heapP->mstrPtrTbl.numEntries * sizeof(void *)
				- sizeof(MemHeapTerminatorType) /* for 0 terminator */;

	// Initialize the master pointer table to 0's.
	iP = (UInt32*) (heapP + 1);
	
	// Set the initial master pointer offsets.  The first and last entries of the
	// master pointer table are special.  The first points to the next available
	// entry.  The last is the table terminator.  Only go through the initialization
	// process if there are handles to be intialized.  If there is only one entry
	// in the table (very odd but possible) then create the table and set it to full.
	if (numHandles > 1) {	

		offset = sizeof(UInt32*);

		// Set the first element
		*iP++ = offset | mstrPtrTblFirstEntryMask;
	
		offset += sizeof(UInt32*);
		
		// Each element should contain the offset to the next element.
		for ( i = 1; i < numHandles - 1; i++, offset += sizeof(UInt32*)) 
			*iP++ =  offset | mstrPtrTblFreeEntryMask;			
	
		// The last element does not point to anything.
		*iP++ = mstrPtrTblFreeEntryMask;	
	}	
	else if (numHandles > 0)
			*iP++ = mstrPtrTblFirstEntryMask;
	
	// Make the first free chunk. It starts after the heap header, and after 
	// the offset table.
	chunkP = (MemChunkHeaderPtr)iP;
	memClearChunkHeader(chunkP);
	chunkP->size = free;			// last item in heap always points to heap terminator
	chunkP->free = true;
	chunkP->hOffset = free/2;		// last item in free list always points to heap terminator
	heapP->firstFreeChunkOffset = (UInt16 *)chunkP - (UInt16 *)heapP;
	
	
	// Initialize the contents, if specified
	if (initContents)
		MemSet(memChunkData(chunkP), chunkP->size - sizeof(MemChunkHeaderType), 0x55);
		
	
	// Make the end of heap marker 
	termP = (MemHeapTerminatorType*)((UInt8 *)chunkP + chunkP->size);
	*termP = 0;                   

   return 0;
}




/************************************************************
 *
 *  FUNCTION: CompactHeap, Private
 *
 *  DESCRIPTION: Compacts a heap 
 *
 *  PARAMETERS: heap pointer, saved DS
 *
 *  RETURNS: error code
 *
 *  CREATED: 10/25/94 
 *
 *  BY: Ron Marianetti
 *
 *		vmk	1/27/98	Fixed heap corruption in master pointer freeing code
 *		Bob	6/18/98	Updated to rebuild free list while compacting
 *
 *************************************************************/
static Err	 PrvCompactHeap(MemHeapHeaderPtr heapP)
{

	MemChunkHeaderPtr			chunkP, nextChunkP, prevFreeChunkP;
	MemMstrPtrTablePtr		mstrTblP;
	MemMstrPtrTablePtr		prevTblP;
	UInt32 *						entryP;
	Int32							i;
	Boolean						firstFreeMstrSkipped = false;


	//-------------------------------------------------------------------
	// We take this time to free up any master pointer chunks that are no longer
	//  used.
	//-------------------------------------------------------------------
	prevTblP = &heapP->mstrPtrTbl;
	while(prevTblP->nextTblOffset) {
	
		// Get pointer to next table
		mstrTblP = (MemMstrPtrTablePtr)((UInt32)heapP + prevTblP->nextTblOffset);
	
		// See if it's all free
		entryP = (UInt32 *) ((UInt32)mstrTblP + sizeof(MemMstrPtrTableType));
	
		// If the first entry of the master pointer table is 0 then the table
		// is full.
		i = (Int32)mstrTblP->numEntries;
		
		ErrFatalDisplayIf((UInt32)*entryP & mstrPtrTblFirstEntryMask != mstrPtrTblFirstEntryMask, 
								"First table entry is not valid");	
		
		if ( ( ( (UInt32)*entryP ) & ~mstrPtrTblFirstEntryMask) != 0) {

			entryP++;
			// Need to walk the master pointer table to see if all the entries are free.  
			// If all the entries are free then the table can be deallocated.
			for (; i > 1; i--, entryP++) 
				if ( !( (UInt32)*entryP & mstrPtrTblFreeEntryMask))
					break;		// not empty
			}
		if (i != 1) {
			// this isn't free, continue
			prevTblP = mstrTblP;
			continue;
			}

		
		// If it's all free, take it out of the linked list and dispose it
		//
		// But never delete the first "unused" master pointer chunk...
		// This fixes the bug where we're allocating a moveable chunk, and, as a result,
		// find a free handle in a master pointer chunk with all free handle slots or a new
		// master pointer chunk is added, but then compaction is triggered while
		// allocating the chunk itself, which deletes the just allocated master pointer
		// chunk before we have a chance to store the new chunk's handle in it.  Subsequently,
		// we ended up storing handles in a master pointer chunk that is marked
		// free (and available for allocation).  This was resulting in heap corruption.
		// It is believed that this was a bug in 1.0 and 2.0 as well.	vmk 1/27/98
		if ( firstFreeMstrSkipped ) {
			prevTblP->nextTblOffset = mstrTblP->nextTblOffset;
			PrvPtrFree(mstrTblP);
			}
		else {
			prevTblP = mstrTblP;					// skip the first "unused" master pointer chunk
			firstFreeMstrSkipped = true;
			}
		}


	//-------------------------------------------------------------------
	// While not at the end of the heap, try and find movable chunks that
	//  we can move into free spaces at the front of the heap.
	//-------------------------------------------------------------------
	for (
		prevFreeChunkP = 0,
		chunkP = (MemChunkHeaderPtr)((UInt8 *)heapP + sizeof(MemHeapHeaderType)
					+ heapP->mstrPtrTbl.numEntries * sizeof(void *));
		chunkP->size != 0;
		chunkP = (MemChunkHeaderPtr)((UInt8 *)chunkP + chunkP->size)) {

		// skip allocated chunks
		if (!chunkP->free)
			continue;
			
		// take the opportunity to merge any adjacent free chunks
		nextChunkP = (MemChunkHeaderPtr)((UInt8 *)chunkP + chunkP->size);
		while (nextChunkP->free) {
			chunkP->size += nextChunkP->size;
			nextChunkP = (MemChunkHeaderPtr)((UInt8 *)chunkP + chunkP->size);
			}
		
		// don't bother trying to move if we hit the end of the heap
		if (nextChunkP->size != 0) {

			// If the following chunk is movable, shift it up to fill the free space.
			if (nextChunkP->lockCount == 0)
				PrvMoveChunk(chunkP, nextChunkP);

			// Else, try and find a movable chunk somewhere else in the heap
			// that will fit into this free space.
			else {
				// Search for a chunk that we can move into this free space
				nextChunkP = PrvFindBiggestMovableChunk(nextChunkP, chunkP->size);
				
				// If we found one, move it
				if (nextChunkP) 
					PrvMoveChunk(chunkP, nextChunkP);
				}
			}

		// if we're passing up a free chunk, put it in the free list
		if (chunkP->free) {
			if (prevFreeChunkP)
				prevFreeChunkP->hOffset = (UInt16 *)chunkP - (UInt16 *)prevFreeChunkP;
			else
				heapP->firstFreeChunkOffset = (UInt16 *)chunkP - (UInt16 *)heapP;
			prevFreeChunkP = chunkP;
			}
		}
	
	// make sure the free list ends at the heap terminator
	if (prevFreeChunkP)
		prevFreeChunkP->hOffset = (UInt16 *)chunkP - (UInt16 *)prevFreeChunkP;
	else
		heapP->firstFreeChunkOffset = (UInt16 *)chunkP - (UInt16 *)heapP;

	return 0;
}




/************************************************************
 *
 *  FUNCTION: PrvFindBiggestMovableChunk, Private
 *
 *  DESCRIPTION: Searches heap for the biggest movable chunk that is
 *		less than or equal to maxSize.
 *
 *  PARAMETERS: heap pointer, start offset, maxSize
 *
 *  RETURNS: chunk pointer, or nil
 *
 *  CREATED: 11/03/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
static MemChunkHeaderPtr PrvFindBiggestMovableChunk(MemChunkHeaderPtr startP, 
										UInt32 maxSize)
{

	MemChunkHeaderPtr	chunkP;
	MemChunkHeaderPtr	bestP = 0;
	UInt32				bestSize = 0;

	// Loop through and find last movable chunk that's <=  maxSize
	// DOLATER ••• is there any reason not to stop if you get a chunk
	// 			   whose size is exactly right?
	chunkP = startP;

	while(chunkP->size) {
		if (! (chunkP->free))
			if ((chunkP->lockCount) == 0)
				if (chunkP->size <= maxSize && chunkP->size >= bestSize) {
					bestP = chunkP;
					bestSize = bestP->size;
					}

		chunkP = (MemChunkHeaderPtr)((UInt8 *)chunkP + chunkP->size);
		}

	// Return it
	return bestP;
}




/************************************************************
 *
 *  FUNCTION: PrvMoveChunk, private
 *
 *  DESCRIPTION: Moves src chunk to dest chunk and creates
 *    a free chunk to fill space left by src chunk and
 *    space at end of dest chunk not taken up by src chunk.
 *
 *    This routine correctly handles the situation where dest chunk
 *    overlaps src chunk as long as dest chunk is BEFORE src chunk.
 *
 *		PrvMoveChunk does not keep the free list up to date.
 *		The caller is responsible for managing the free list.
 *		However, this does create a free header when splitting the
 *		destination chunk, to facilitate PrvHeapCompact which rebuilds
 *		the free list as it walks the heap.
 *
 *  PARAMETERS: dest chunk pointer, src chunk pointer
 *
 *  RETURNS: pointer to free chunk at dest, or 0 if none available
 *
 *  CREATED: 11/03/94 
 *
 *  BY: Ron Marianetti
 *		Bob	6/18/98	Updated to keep free list in mind
 *
 *************************************************************/
static void PrvMoveChunk(MemChunkHeaderPtr dstChunkP, MemChunkHeaderPtr srcChunkP)
{
	UInt32				srcMinActSize, words, dSize;
	UInt32				srcOldActSize, dstOldActSize;
	UInt32				srcReqSize;
	UInt16 *				srcP;
	UInt16 * 			dstP;
	Boolean				overlap;
	void **				ptrP;
	MemChunkHeaderPtr	chunkP;

	// Save the actual sizes
	dstOldActSize = dstChunkP->size;
	srcOldActSize = srcChunkP->size;


	// See if they overlap
	overlap = false;
	if (dstChunkP < srcChunkP) 
		if ((UInt32)dstChunkP + srcOldActSize > (UInt32)srcChunkP) 
			overlap = true;


	// Get the min actual size and the requested sizes of each chunk
	srcReqSize = srcChunkP->size - (srcChunkP->sizeAdj);
	srcMinActSize = srcReqSize;
	if (srcMinActSize & 0x01) srcMinActSize++;

	// Get the address of the master pointer (the handle) of the chunk being moved
	ptrP = (void **)((UInt32)srcChunkP - ((Int32)srcChunkP->hOffset << 1));
	

	// Copy src to dst using the min required size of the src
	srcP = (UInt16 *)srcChunkP;
	dstP = (UInt16 *)dstChunkP;
	words = srcMinActSize >> 1;
	if (words) 
	do {
		*dstP++ = *srcP++;
		} while(--words);


	// Update the handle to point to the new home
	*(ptrP) = memChunkData(dstChunkP);
	
	// Update the hOffset field of the moved chunk to reference the master pointer
	dstChunkP->hOffset = ((UInt32)dstChunkP - (UInt32)ptrP) >> 1;
	

	//-------------------------------------------------------------
	// Figure out the size and location of the free chunk that will
	// go at the end of dst after we've moved the chunk there.
	//-------------------------------------------------------------
		
	chunkP = (MemChunkHeaderPtr)((UInt8 *)dstChunkP + srcMinActSize);
	if (!overlap) {
		dSize = dstOldActSize - srcMinActSize;
		memClearChunkHeader(srcChunkP);
		srcChunkP->size = srcOldActSize;
		srcChunkP->free = true;
		}
	else 
		dSize = (UInt32)(srcChunkP) + srcOldActSize - (UInt32)chunkP;
		

	//-------------------------------------------------------------
	// Create the free chunk at the end of dst, if we have enough 
	// to form a chunk header.
	//-------------------------------------------------------------
	if (dSize >= sizeof(MemChunkHeaderType)) {
		memClearChunkHeader(chunkP);
		chunkP->size = dSize;
		chunkP->free = true;

		// Clip the size of the moved chunk down to a minimum
		dstChunkP->size = srcMinActSize;
		}

	// Else, just grow it slightly to take up the slack
	else {
		dstChunkP->size = dstOldActSize;
	}
	
	// Set the appropriate size adjustment for the moved chunk
	dstChunkP->sizeAdj = dstChunkP->size - srcReqSize;
	
	// Let the caller put the free chunk in the free list,
	// because they know where it should go.
}




/************************************************************
 *
 *  FUNCTION: PrvUnlockAllChunks, Private
 *
 *  DESCRIPTION: Scans the heap and unlocks all movable chunks.
 *
 *  PARAMETERS: heapID
 *
 *  RETURNS: chunk pointer, or nil
 *
 *  CREATED: 11/03/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
static void  PrvUnlockAllChunks(UInt16 heapID)
{
	MemHeapHeaderPtr	heapP;
	MemChunkHeaderPtr	chunkP;

	// Get a near heap pointer from the heapID
	heapP = MemHeapPtr(heapID);

	// Walk the heap
	chunkP = (MemChunkHeaderPtr)((UInt8 *)heapP + sizeof(MemHeapHeaderType)
					+ (heapP->mstrPtrTbl.numEntries << 2) );

	while(chunkP->size) {
		if ((chunkP->lockCount) != memPtrLockCount)
		chunkP->lockCount = 0;

		// Go on to next chunk
		chunkP = (MemChunkHeaderPtr) ((UInt8 *)chunkP + chunkP->size);
		}
		
}




/************************************************************
 *
 *  FUNCTION: PrvAdjustForNewAddress, Private
 *
 *  DESCRIPTION: Adjusts all handles after a memory card had been moved
 *		to a different slot, or when emulation app has re-launched and has
 *		a different address for the memory card. This routine is called by
 *		MemInitHeapTable.
 *
 *  PARAMETERS: heapID
 *
 *  RETURNS: chunk pointer, or nil
 *
 *  CREATED: 11/03/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
static void  PrvAdjustForNewAddress(UInt16 heapID)
{
	MemHeapHeaderPtr	heapP;
	MemChunkHeaderPtr	chunkP;
	void **				ptrP;

	// Get a near heap pointer from the heapID
	heapP = MemHeapPtr(heapID);

	// Walk the heap
	chunkP = (MemChunkHeaderPtr)((UInt8 *)heapP + sizeof(MemHeapHeaderType)
					+ (heapP->mstrPtrTbl.numEntries << 2) );

	while(chunkP->size) {
		// See if this chunk has a handle
		if (!chunkP->free && chunkP->hOffset) {
			
			// Get the master pointer address		
			ptrP = (void **)((UInt32)chunkP - ((Int32)chunkP->hOffset << 1));
	
			// Update the handle to point to the chunk
			*(ptrP) = memChunkData(chunkP);
			}
   
		// Go on to next chunk
		chunkP = (MemChunkHeaderPtr) ((UInt8 *)chunkP + chunkP->size);
		}
		
}




/************************************************************
 * PRIVATE
 *-------------------------------------------------------------
 *  FUNCTION: PrvGetCardHeaderPtr, private
 *
 *  DESCRIPTION: Returns the card header pointer for a card
 *			and a boolean signifying whehter it's RAM only or not
 *
 *  PARAMETERS: card number, reference to boolean
 *
 *  RETURNS: card header pointer
 *
 *  CREATED: 11/09/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
static CardHeaderPtr PrvGetCardHeaderInfo(UInt16 cardNo, 
		CardInfoPtr* cardInfoPP, Boolean* ramOnlyCardP)
{
	CardHeaderPtr		cardP;
	CardInfoPtr			infoP;

	// Get the card size and base address
	ErrFatalDisplayIf(cardNo >= GMemCardSlots, "Invalid Card #");
	
	
	
	// Get the card info (optimized for 2 cards)
	if (cardNo == 0) 
		infoP = memCardInfoP(0);  
	else
		infoP = memCardInfoP(1);

	*cardInfoPP = infoP;
	if (!infoP->size) return 0;



	//------------------------------------------------------------
	// Look for the ROM card header
	//------------------------------------------------------------
	if (infoP->cardHeaderOffset == sysRAMOnlyCardHeaderOffset) {
		*ramOnlyCardP = true;
		cardP = (CardHeaderPtr)infoP->baseP;
		}
	else {
		*ramOnlyCardP = false;

		#if EMULATION_LEVEL != EMULATION_NONE
		cardP = (CardHeaderPtr)MemLocalIDToPtr(infoP->cardHeaderOffset, cardNo);
		#else
		cardP = (CardHeaderPtr)(infoP->baseP + infoP->cardHeaderOffset);
		#endif
		}
	
	
	
	return cardP;
}


/************************************************************
 *
 *	FUNCTION: MemGetROMNVParams
 *
 *	DESCRIPTION: Returns the ROM store NVParam info.
 *
 *	PARAMETERS:
 *		paramsP		-> Where to put the info.
 *
 *	RETURNS:	nothing.
 *
 *	HISTORY:
 *		05/11/00	kwk	Created by Ken Krugler.
 *
 *************************************************************/

void MemGetRomNVParams(SysNVParamsType* paramsP)
{
	Boolean				ramCard;
	StorageHeaderPtr	storeP;
	CardInfoPtr			cardInfoP;

	PrvGetCardHeaderInfo(0, &cardInfoP, &ramCard);
	// DOLATER kwk - verify with Steve that this never happens.
	ErrFatalDisplayIf(ramCard, "Card 0 has no ROM store");
	
	storeP = (StorageHeaderType*)(cardInfoP->baseP +
											cardInfoP->cardHeaderOffset +
											sysROMStoreRelOffset);
	MemMove(paramsP, &storeP->nvParams, sizeof(SysNVParamsType));
}


/************************************************************
 *
 *  FUNCTION: PrvHeapPtrAndID, private
 *
 *  DESCRIPTION: Returns the heap pointer and heapID for a given chunk
 *
 *  PARAMETERS: chunk data pointer
 *
 *  RETURNS: heap ID
 *
 *  CREATED: 10/25/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
static MemHeapHeaderPtr		PrvHeapPtrAndID(const void * chunkDataP, UInt16* heapIDP)
{
	UInt16				i;
	MemHeapHeaderPtr	heapP;
	UInt16				heapID;
	UInt32				size;
	UInt32				chunkOffset, heapOffset;
	
	CardInfoPtr			infoP;
	Boolean				found=false;
	
	
	// Check for error
	ErrFatalDisplayIf(chunkDataP==0, "Nil Chunk ptr");
	
	
	
	// See if it's in card 1
	if (GMemCardSlots > 1) {
		infoP = memCardInfoP(1);
		if (	(UInt32)chunkDataP > (UInt32)infoP->baseP  &&  
				(UInt32)chunkDataP < (UInt32)infoP->baseP + infoP->size) {
			found = true;
			heapID = 0x8000;
			}
		}
		
		
	// If not card 1, check card 0
	if (!found) {
		infoP = memCardInfoP(0);
		// The special test against dynHeapSpace is made to check for the dynamic heap (in the
		// 328 device, the dynamic heap is below the card baseP and the other test would not work).
		if (((UInt32)chunkDataP < infoP->dynHeapSpace + infoP->rsvSpace) ||
			 ((UInt32)chunkDataP > (UInt32)infoP->baseP  &&  
				 (UInt32)chunkDataP < (UInt32)infoP->baseP + infoP->size)) {
			found = true;
			heapID = 0x0000;
			}
		}
		
	// If still not found, see if it's in the dynamic heap
#if EMULATION_LEVEL == EMULATION_NONE
	// <SCL 9/22/98>	Does this really do anything given the card 0 case above
	//						which seems to already have caught the dynamic heap case?
	//						Seems like this is extraneous; if above case fails, so should this.
	if (!found) {
		infoP = memCardInfoP(0);
		if ((UInt32)chunkDataP & ~infoP->cardOffsetMask == 0) {	// was hwrCardOffsetMask
			found = true;
			heapID = 0x0000;
			}
		}
#endif
		
	if (!found) {
		ErrDisplay("Invalid Chunk ptr");
		return 0;
		}
	


	// Get Chunk Info. This is all buggy on the 328 for dynamic heap chuncks because chunkDataP
	// is a small value above zero and baseP is 0x10000000. The result is an overflow and results in
	// a huge positive number because all variables are unsigned.
	
	chunkOffset = (UInt32)chunkDataP - (UInt32)infoP->baseP;

	// Search for the heap pointer in the RAM Heaps
	for (i=0; i<infoP->numRAMHeaps; i++) {
		heapOffset = infoP->ramHeapOffsetsP[i];
		// For an unknown reason (probably another overflowed calculation somewhere, the ramHeapOffset[0]
		// on a 328 is 0xF0001800 (instead of 1800). The comparison below just happens to work because
		// the variables types are luckily unsigned.
		if (heapOffset > chunkOffset) continue;
		
		// calculation of heapP on a 328 really does a 0x10000000 + 0xF0001800. We are lucky again to
		// get back to the right small value where the heap is really (0x1800).
		heapP = (MemHeapHeaderPtr)(infoP->baseP + heapOffset);
		size = heapP->size;
		if (!size) size = 0x00010000L;
		if (heapOffset + size < chunkOffset) continue;
		
		*heapIDP =  heapID | i;
		return heapP;	// We finally return the right value. God probably exists, after all...
		}
		
	// Search for the heap pointer in the ROM Heaps
	for (i=0; i<infoP->numROMHeaps; i++) {
		heapOffset = infoP->romHeapOffsetsP[i];
		if (heapOffset > chunkOffset) continue;
		
		heapP = (MemHeapHeaderPtr)(infoP->baseP + heapOffset);
		size = heapP->size;
		if (!size) size = 0x00010000L;
		if (heapOffset + size < chunkOffset) continue;
		
		// <RM> 1-26-98
		// Now add infoP->numRAMHeaps to the heap index. 
		*heapIDP =  heapID | (i + infoP->numRAMHeaps);
		return heapP;
		}
		
	// If we got here, error
	ErrDisplay("Heap ID not found");
	return 0;
		
} 




/************************************************************
 *
 *  FUNCTION: PrvCalcStoreCRC, private
 *
 *  DESCRIPTION: Calculates the CRC for a Memory Store
 *
 *  PARAMETERS: store pointer
 *
 *  RETURNS: crc
 *
 *  CREATED: 5/2/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
static UInt32	PrvCalcStoreCRC(StorageHeaderPtr storeP)
{
	return Crc16CalcBlock(storeP, 
				sizeof(StorageHeaderType) - sizeof(UInt32), 0);
}
