/******************************************************************************
 *
 * Copyright (c) 1999-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: IntlMgr.c
 *
 * Release: 
 *
 * Description:
 *		International Mgr routines.
 *
 * History:
 *		08/05/99	kwk	Created by Ken Krugler.
 *		07/13/00	kwk	Added realWordTable and encodingNameList to set of fields
 *							in the Intl Mgr globals that we know about.
 *		12/20/00	kwk	Added doubleCharTable to the set of fields in the Intl Mgr
 *							globals that we know about.
 *
 *****************************************************************************/

#include <PalmTypes.h>
#include <IntlMgr.h>
#include <TextMgr.h>
#include <ErrorMgr.h>
#include <FeatureMgr.h>
#include <SystemMgr.h>
#include <Event.h>
#include <SysEvent.h>
#include <Menu.h>
#include <Field.h>
#include <StringMgr.h>
#include <Form.h>
#include <Clipboard.h>
#include <OverlayMgr.h>
#include <DebugMgr.h>			// For DbgBreak()

#include "TextPrv.h"
#include "IntlPrv.h"
#include "IntlPrv.rh"
#include "UIResourcesPrv.h"	// For localeModuleNameStrID
#include "MemoryPrv.h"			// For MemGetRomNVParams
#include "TextTablePrv.h"		// For TTGet8BitIndexedData

#define	NON_PORTABLE
#include	"Globals.h"
#include "SystemPrv.h"			// For SysNVParamsType
#include "FeaturePrv.h"			// For ROMFtrTableType
#include "DataPrv.h"				// For DmFindDatabaseWithTypeCreator

#if (EMULATION_LEVEL != EMULATION_NONE)
#include "SimStubs.h"			// For StubPalmOSInitLocaleModule.
#endif

/***********************************************************************
 * Static data used to map from a table index to an offset into the
 * IntlMgr global data structure, where the corresponding table ptr
 * is located. This relies on PC-relative data!
 ***********************************************************************/

#define offsetof(type, member)       ((UInt32) &(((type *) 0)->member))

static const kIntlGlobalOffsets[intlLMNumTableIndexes] = {
	offsetof(IntlGlobalsType, byteAttrTable),					// intlLMByteAttrTable			0
	offsetof(IntlGlobalsType, charAttrTable),					// intlLMCharAttrTable			1
	offsetof(IntlGlobalsType, charXAttrTable),				// intlLMCharXAttrTable			2
	offsetof(IntlGlobalsType, charValidTable),				// intlLMCharValidTable			3
	offsetof(IntlGlobalsType, prevCharSizeTable),			// intlLMPrevCharSizeTable		4
	offsetof(IntlGlobalsType, nextCharGetTable),				// intlLMGetNextCharTable		5
	offsetof(IntlGlobalsType, nextCharSetTable),				// intlLMSetNextCharTable		6
	offsetof(IntlGlobalsType, charBoundsSizeTable),			// intlLMCharBoundsTable		7
	offsetof(IntlGlobalsType, findWordStartTable),			// intlLMFindWordStartTable	8
	offsetof(IntlGlobalsType, findWordMatchTable),			// intlLMFindWordMatchTable	9
	offsetof(IntlGlobalsType, wordStartClassTable),			// intlLMWordStartClassTable	10
	offsetof(IntlGlobalsType, wordStartBreakTable),			// intlLMWordStartBreakTable	11
	offsetof(IntlGlobalsType, wordEndClassTable),			// intlLMWordEndClassTable		12
	offsetof(IntlGlobalsType, wordEndBreakTable),			// intlLMWordEndBreakTable		13
	offsetof(IntlGlobalsType, charEncodingTable),			// intlLMCharEncodingTable		14
	offsetof(IntlGlobalsType, maxEncodingTable),				// intlLMMaxEncodingTable		15
	offsetof(IntlGlobalsType, encodingToNameList),			// intlLMEncodingToNameList	16
	offsetof(IntlGlobalsType, stdTransliterations[0]),		// intlLMStdTrans0Table			17
	offsetof(IntlGlobalsType, stdTransliterations[1]),		// intlLMStdTrans1Table			18
	offsetof(IntlGlobalsType, stdTransliterations[2]),		// intlLMStdTrans2Table			19
	offsetof(IntlGlobalsType, stdTransliterations[3]),		// intlLMStdTrans3Table			20
	offsetof(IntlGlobalsType, customTransliterations[0]),		// intlLMCusTrans0Table		21
	offsetof(IntlGlobalsType, customTransliterations[1]),		// intlLMCusTrans1Table		22
	offsetof(IntlGlobalsType, customTransliterations[2]),		// intlLMCusTrans2Table		23
	offsetof(IntlGlobalsType, customTransliterations[3]),		// intlLMCusTrans3Table		24
	offsetof(IntlGlobalsType, customTransliterations[4]),		// intlLMCusTrans4Table		25
	offsetof(IntlGlobalsType, customTransliterations[5]),		// intlLMCusTrans5Table		26
	offsetof(IntlGlobalsType, customTransliterations[6]),		// intlLMCusTrans6Table		27
	offsetof(IntlGlobalsType, customTransliterations[7]),		// intlLMCusTrans7Table		28
	offsetof(IntlGlobalsType, customTransliterations[8]),		// intlLMCusTrans8Table		29
	offsetof(IntlGlobalsType, customTransliterations[9]),		// intlLMCusTrans9Table		30
	offsetof(IntlGlobalsType, customTransliterations[10]),	// intlLMCusTrans10Table	31
	offsetof(IntlGlobalsType, customTransliterations[11]),	// intlLMCusTrans11Table	32
	offsetof(IntlGlobalsType, customTransliterations[12]),	// intlLMCusTrans12Table	33
	offsetof(IntlGlobalsType, customTransliterations[13]),	// intlLMCusTrans13Table	34
	offsetof(IntlGlobalsType, customTransliterations[14]),	// intlLMCusTrans14Table	35
	offsetof(IntlGlobalsType, customTransliterations[15]),	// intlLMCusTrans15Table	36
	offsetof(IntlGlobalsType, sortTables[0]),					// intlLMSort0Table				37
	offsetof(IntlGlobalsType, sortTables[1]),					// intlLMSort1Table				38
	offsetof(IntlGlobalsType, sortTables[2]),					// intlLMSort2Table				39
	offsetof(IntlGlobalsType, sortTables[3]),					// intlLMSort3Table				40
	offsetof(IntlGlobalsType, sortTables[4]),					// intlLMSort4Table				41
	offsetof(IntlGlobalsType, sortTables[5]),					// intlLMSort5Table				42
	offsetof(IntlGlobalsType, deviceToIndexTable),			// intlLMDeviceToIndexTable	43
	offsetof(IntlGlobalsType, indexToUnicodeTable),			// intlLMIndexToUnicodeTable	44
	offsetof(IntlGlobalsType, unicodeToDeviceTable),		// intlLMUnicodeToDeviceTable	45
	offsetof(IntlGlobalsType, wordWrapClassTable),			// intlLMWordWrapClassTable	46
	offsetof(IntlGlobalsType, wordWrapBreakTable),			// intlLMWordWrapBreakTable	47
	offsetof(IntlGlobalsType, realWordTable),					// intlLMRealWordTable			48
	offsetof(IntlGlobalsType, nameToEncodingList),			// intlLMNameToEncodingList	49
	offsetof(IntlGlobalsType, doubleCharTable)				// intlLMDoubleCharTable		50
};

/***********************************************************************
 * Private functions
 ***********************************************************************/
 
static void* PrvGetByteIndexedTable(TextTableP iTableP, UInt16 iResultSize);
static void* PrvGetStageIndexMapTable(TextTableP iTableP, UInt16 iResultSize);

/***********************************************************************
 *
 *	FUNCTION:		PrvGetByteIndexedTable
 *
 *	DESCRIPTION:	Return ptr to byte indexed data in table, if the table
 *		is the right format.
 *
 *	PARAMETERS:
 *		iTableP	 ->	Ptr to TextTable table, or NULL.
 *
 *	RETURNS:
 *		Ptr to start of data array if <iTableP> is a byte-indexed table of
 *		elements of size <iResultSize>
 *
 *	HISTORY:
 *		05/31/00	kwk	Created by Ken Krugler.
 *		12/11/00	CS		Renamed from PrvGetByteIndexedByteTable to
 *							PrvGetByteIndexedTable, since the table doesn't have
 *							to contain bytes in the result.
 *
 ***********************************************************************/
static void* PrvGetByteIndexedTable(TextTableP iTableP, UInt16 iResultSize)
{
	void* byteDataP;

	if (iTableP == NULL)
	{
		return(NULL);
	}
	
	byteDataP = TTGet8BitIndexedData(iTableP);
	if ((byteDataP == NULL) || (TTGetNumResultBits(iTableP) != iResultSize))
	{
		return(NULL);
	}
	else
	{
		return(byteDataP);
	}
} // PrvGetByteIndexedTable


/***********************************************************************
 *
 *	FUNCTION:		PrvGetStageIndexMapTable
 *
 *	DESCRIPTION:	Return ptr to stage index map data in table, if the
 *		table is the right format.
 *
 *	PARAMETERS:
 *		iTableP	 ->	Ptr to TextTable table, or NULL.
 *
 *	RETURNS:
 *		Ptr to stage index map details structure (TTDetailsStageIndexMapType*)
 *		if <iTableP> is a byte-indexed table of elements of size <iResultSize>
 *
 *	HISTORY:
 *		12/11/00	CS		Created by Chris Schneider.
 *
 ***********************************************************************/
static void* PrvGetStageIndexMapTable(TextTableP iTableP, UInt16 iResultSize)
{
	TTDetailsStageIndexMapType* stageDetailsP;

	if (iTableP == NULL)
	{
		return(NULL);
	}
	
	stageDetailsP = TTGetStageIndexMapData(iTableP);
	if ((stageDetailsP == NULL) || (TTGetNumResultBits(iTableP) != iResultSize))
	{
		return(NULL);
	}
	else
	{
		return(stageDetailsP);
	}
} // PrvGetStageIndexMapTable


/***********************************************************************
 *
 *	FUNCTION:		IntlGetRoutineAddress
 *
 *	DESCRIPTION:	Return back the address of the IntlMgr routine indicated
 *						by <inSelector>. If <inSelector> is invalid, return NULL.
 *
 *	PARAMETERS:		inSelector	->	routine selector.
 *
 *	RETURNS:			Address of routine, or NULL
 *
 *	HISTORY
 *		05/24/99	kwk	Created by Ken Krugler
 *		10/08/99	kwk	Don't display non-fatal alert when being asked for
 *							non-existant routine address w/Simulator build.
 *		05/10/00	kwk	Support intlDispatchTable low-memory pointer.
 *		05/16/00	kwk	For simulator build, use TxtCharWidthDeprecated name.
 *
 ***********************************************************************/

void* IntlGetRoutineAddress(IntlSelector inSelector)
{
#if (EMULATION_LEVEL == EMULATION_NONE)
	Int32* tablePtr;
#endif
	
	if (inSelector > intlMaxSelector)
	{
		return(NULL);
	}

#if (EMULATION_LEVEL == EMULATION_NONE)
	tablePtr = (Int32*)GIntlDispatchTableP;
	
	if (tablePtr != NULL)
	{
		return((void*)tablePtr[inSelector]);
	}
	else
	{
		Int16 offset;
		
		tablePtr = (Int32*)IntlDispatchTable;
		offset = tablePtr[inSelector] & 0x0FFFF;
		return((void *)((UInt8 *)&tablePtr[inSelector] + 2 + offset));
	}

#else	// EMULATION_LEVEL != EMULATION_NONE

	switch (inSelector)
	{
		case intlIntlInit:				return(IntlInit);
		case intlTxtByteAttr:			return(TxtByteAttr);
		case intlTxtCharAttr:			return(TxtCharAttr);
		case intlTxtCharXAttr:			return(TxtCharXAttr);
		case intlTxtCharSize:			return(TxtCharSize);
		case intlTxtGetPreviousChar:	return(TxtGetPreviousChar);
		case intlTxtGetNextChar:		return(TxtGetNextChar);
		case intlTxtGetChar:				return(TxtGetChar);
		case intlTxtSetNextChar:		return(TxtSetNextChar);
		case intlTxtCharBounds:			return(TxtCharBounds);
		case intlTxtPrepFindString:	return(TxtPrepFindString);
		case intlTxtFindString:			return(TxtFindString);
		case intlTxtReplaceStr:			return(TxtReplaceStr);
		case intlTxtWordBounds:			return(TxtWordBounds);
		case intlTxtCharEncoding:		return(TxtCharEncoding);
		case intlTxtStrEncoding:		return(TxtStrEncoding);
		case intlTxtEncodingName:		return(TxtEncodingName);
		case intlTxtMaxEncoding:		return(TxtMaxEncoding);
		case intlTxtTransliterate:		return(TxtTransliterate);
		case intlTxtCharIsValid:		return(TxtCharIsValid);
		case intlTxtCompare:				return(TxtCompare);
		case intlTxtCaselessCompare:	return(TxtCaselessCompare);
		case intlTxtCharWidth:			return(TxtCharWidth);
		case intlTxtGetTruncationOffset: return(TxtGetTruncationOffset);
		case intlIntlHandleEvent:		return(IntlHandleEvent);
		case intlIntlGetRoutineAddress:	return(IntlGetRoutineAddress);
		case intlIntlSetRoutineAddress:	return(IntlSetRoutineAddress);
		case intlTxtGetWordWrapOffset:	return(TxtGetWordWrapOffset);

		default:
			return(NULL);
	}
#endif
} // IntlGetRoutineAddress


/***********************************************************************
 *
 *	FUNCTION:		IntlSetRoutineAddress
 *
 *	DESCRIPTION:	Set the international routine indicated by iSelector
 *		to be iProcPtr.
 *
 *	PARAMETERS:		iSelector	->	routine selector.
 *						iProcPtr		-> address of new routine.
 *
 *	RETURNS:			intlErrInvalidSelector if iSelector is out of range.
 *
 *	HISTORY
 *		05/10/00	kwk	Created by Ken Krugler
 *
 ***********************************************************************/

Err IntlSetRoutineAddress(IntlSelector iSelector, void* iProcPtr)
{
#if (EMULATION_LEVEL == EMULATION_NONE)
	void** tablePtr;
	Int16 i;
	
	if (iSelector > intlMaxSelector)
	{
		return(intlErrInvalidSelector);
	}
	
	ErrNonFatalDisplayIf(iProcPtr == NULL, "Null proc ptr parameter");
	
	tablePtr = (void**)GIntlDispatchTableP;
	if (tablePtr == NULL)
	{
		tablePtr = (void**)MemPtrNew((intlMaxSelector + 1) * sizeof(Int32*));
		if (tablePtr == NULL)
		{
			return(memErrNotEnoughSpace);
		}
		
		// Make sure it doesn't get tossed when the current app quits.
		MemPtrSetOwner(tablePtr, 0);
		
		// Initialize the table with all of the default routine addresses.
		for (i = 0; i <= intlMaxSelector; i++)
		{
			tablePtr[i] = IntlGetRoutineAddress(i);
		}
		
		// Set up the low-memory global with our new, valid table.
		GIntlDispatchTableP = tablePtr;
	}
	
	tablePtr[iSelector] = iProcPtr;
	return(errNone);
#else	// EMULATION_LEVEL != EMULATION_NONE
	ErrFatalDisplay("Can't call IntlSetRoutineAddress in Simulator");
	return(errNone);
#endif
} // IntlSetRoutineAddress


/***********************************************************************
 *
 *	FUNCTION:		IntlHandleEvent
 *
 *	DESCRIPTION:	Decide if we want to handle the event for the system.
 *
 *	PARAMETERS:		inEvent		-> Event to be processed
 *						inProcess	-> True if we should actually process it.
 *
 *	RETURNS:			True if we handled the event.
 *
 *	HISTORY
 *		08/05/99	kwk	Created by Ken Krugler
 *
 ***********************************************************************/

Boolean IntlHandleEvent(const SysEventType* /*inEventP*/, Boolean /*inProcess*/)
{
	return(false);
} // IntlHandleEvent


/***********************************************************************
 *
 *	FUNCTION:		IntlSetStrictChecks
 *
 *	DESCRIPTION:	Set the kStrictChecksFlag in GIntlData->intlFlags on
 *		or off, and also set the corresponding intlMgrStrict flag in the
 *		IntlMgr feature.
 *
 *	PARAMETERS:
 *		iStrictChecks	 ->	New setting for strict checking.
 *
 *	RETURNS:
 *		Previous value of strict check flag.
 *
 *	HISTORY
 *		11/29/00	kwk	Created by Ken Krugler
 *
 ***********************************************************************/
 
Boolean IntlSetStrictChecks(Boolean iStrictChecks)
{
	Boolean oldSetting;
	UInt32 flags;
	Err result;
	
	// Global flags has precedence over feature value.
	oldSetting = (GIntlData->intlFlags & kStrictChecksFlag) != 0;
	 
	result = FtrGet(sysFtrCreator, sysFtrNumIntlMgr, &flags);
	ErrNonFatalDisplayIf(result != errNone, "Can't get sysFtrNumIntlMgr feature");
	if (iStrictChecks)
	{
		flags |= intlMgrStrict;
		GIntlData->intlFlags |= kStrictChecksFlag;
	}
	else
	{
		flags &= ~intlMgrStrict;
		GIntlData->intlFlags &= ~kStrictChecksFlag;
	}
	
	result = FtrSet(sysFtrCreator, sysFtrNumIntlMgr, flags);
	ErrNonFatalDisplayIf(result != errNone, "Can't set sysFtrNumIntlMgr feature");
	
	return(oldSetting);
} // IntlSetStrictChecks


/***********************************************************************
 *
 *	FUNCTION:		IntlInit
 *
 *	DESCRIPTION:	Initialize the Int'l Manager by loading the appropriate
 *						locale module.
 *
 *	PARAMETERS:		nothing
 *
 *	RETURNS:			nothing
 *
 *	HISTORY:
 *		05/24/99	kwk	Created by Ken Krugler
 *		08/08/99	kwk	Now set up character encoding, default std/bold font
 *							features.
 *		08/11/99	kwk	Set up sysFtrNumCharEncodingFlags feature with the
 *							charEncodingOnlySingleByte flag.
 *		05/11/00	kwk	Re-wrote for new table-based architecture.
 *		05/18/00	kwk	Use DmFindDatabaseWithTypeCreator vs. DmFindDatabase
 *							to speed things up. Also removed hard-coded assumptions
 *							about system DB on card 0, and the system DB name.
 *		05/19/00	kwk	Set up for fast sorting if sort tables look right.
 *							Generate fatal error if resource overrides existing
 *							table ptr (set up by 'locm' resource).
 *		05/23/00	kwk	Don't worry about finding the locale module if we're
 *							building for the Simulator.
 *		05/31/00	CS		TTGetNumResultBits is now a separate routine.
 *					kwk	Set up fast searching flag/table ptrs if approriate.
 *							Set internal kSingleByteOnlyFlag based on feature.
 *		06/19/00	kwk	Call StubPalmOSInitLocaleModule if built for the Simulator.
 *		09/14/00	kwk	If we can't find the locale module for the current locale,
 *							and we're trying to find the system overlay, use the
 *							overlay file type, not the locale module file type.
 *					kwk	If we default to the ROM locale, because we couldn't find
 *							a valid locale module for the current locale, handle the
 *							case of the ROM being built without overlays (and thus
 *							there's no system overlay).
 *					kwk	If we find a valid locale module for the ROM locale,
 *							set the current locale to match the ROM locale.
 *		11/22/00	kwk	Reduce stack requirements by using dynamic memory for
 *							the DmSearchStateType & SysNVParamsType structs, and
 *							the system/overlay DB names (was 156 bytes, now 22 bytes).
 *		11/24/00	kwk	Fixed bug where module patch code could set up word
 *							match or char attribute tables, but we'd still try to
 *							convert them into direct pointers for speed optimization.
 *					kwk	Use new 'tint' resource to specify # of custom transliterations,
 *							and only allocate space for actual # of table pointers.
 *							This saves 64 bytes of heap space for the Latin case.
 *		12/11/00	CS		Check to see if the charAttrTable is a stage index
 *							map table (via PrvGetByteIndexedTable), and if so
 *							store a pointer to the details record instead.
 *		12/20/00	kwk	Set up new doubleCharTable field as byte-indexed byte table.
 *
 ***********************************************************************/

void IntlInit(void)
{
	Err result;
	MemHandle localeCodeH;
	MemHandle tableListH;
	MemHandle featureTableH;
	ROMFtrTableType* featureTableP;
	ROMFtrCreatorType* creatorTableP;
	UInt16 creators;
	Boolean fastSort = true;
	Boolean fastSearch = true;
	Boolean fastAttr = true;
	UInt16 index;
	UInt32 intlFtrFlags;

#if (EMULATION_LEVEL == EMULATION_NONE)
	UInt16 systemCard;
	LocalID systemID;
	DmSearchStateType* searchStateP;
	MemHandle localeNameH;
	LocalID moduleID;
	DmOpenRef moduleRef;

	// Locate the system DB, so that we know where to look (which card)
	// for the locale module.
	searchStateP = (DmSearchStateType*)MemPtrNew(sizeof(DmSearchStateType));
	result = DmGetNextDatabaseByTypeCreator(	true,
															searchStateP,
															sysFileTSystem,
															sysFileCSystem,
															true,					// onlyLatestVers
															&systemCard,
															&systemID);
	ErrNonFatalDisplayIf((result != errNone) || (systemID == 0), "Can't find system DB");
	MemPtrFree((MemPtr)searchStateP);
	
	localeNameH = DmGetResource(strRsc, localeModuleNameStrID);
	ErrNonFatalDisplayIf(localeNameH == NULL, "Missing locale module name string");
	
	moduleID = DmFindDatabaseWithTypeCreator(	systemCard,
															(Char*)MemHandleLock(localeNameH),
															sysFileTLocaleModule,
															sysFileCSystem);
	MemHandleUnlock(localeNameH);
	
	if (moduleID != 0)
	{
		moduleRef = DmOpenDatabase(systemCard, moduleID, dmModeReadOnly | dmModeLeaveOpen);
		ErrNonFatalDisplayIf(moduleRef == 0, "Can't open locale module");
		
		if (ResLoadConstant(kIntlLMVersResID) != kIntlLMVersion)
		{
			DmCloseDatabase(moduleRef);
			moduleID = 0;
		}
	}
	
	// If we couldn't find the locale module, or it has the wrong version, then
	// default to the ROM locale's locale module.
	if (moduleID == 0)
	{
		SysNVParamsType* romParamsP;
		OmLocaleType romLocale;
		Char* overlayName;
		LocalID overlayID;
		DmOpenRef overlayRef;
		Char* systemName;
		
		// Open up the system overlay that corresponds to the ROM's locale.
		// Note that (assuming the ROM locale != current locale) we have to
		// try to open up the overlay explicitly, since the Overlay Mgr
		// won't be opening it automatically. We also have to handle the case
		// of a ROM built without overlays, in which case the ROM locale's
		// locale module name string is located in the base, which means we
		// have to make a low-level resource call to get it (otherwise we'll
		// get the string from the system overlay).
		romParamsP = (SysNVParamsType*)MemPtrNew(sizeof(SysNVParamsType));
		MemGetRomNVParams(romParamsP);
		romLocale.country = romParamsP->localeCountry;
		romLocale.language = romParamsP->localeLanguage;
		MemPtrFree((MemPtr)romParamsP);
		
		// Get the name of the system DB, so that we can construct the overlay
		// name.
		systemName = (Char*)MemPtrNew(dmDBNameLength);
		overlayName = (Char*)MemPtrNew(dmDBNameLength);

		result = DmDatabaseInfo(systemCard, systemID, systemName,
						NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		ErrNonFatalDisplayIf(result != errNone, "Can't get system DB info");
		
		result = OmLocaleToOverlayDBName(systemName, &romLocale, overlayName);
		ErrNonFatalDisplayIf(result != errNone, "Can't make ROM locale system overlay name");
		
		overlayID = DmFindDatabaseWithTypeCreator(systemCard,
																overlayName,
																sysFileTOverlay,
																sysFileCSystem);
		MemPtrFree(systemName);
		MemPtrFree((MemPtr)overlayName);
		
		if (overlayID != 0)
		{
			overlayRef = DmOpenDatabase(systemCard, overlayID, dmModeReadOnly);
			ErrNonFatalDisplayIf(overlayRef == 0, "Can't open system overlay");
			
			// Now load the locale module name string from this overlay, and use
			// it to find the locale module.
			localeNameH = DmGet1Resource(strRsc, localeModuleNameStrID);
			ErrNonFatalDisplayIf(localeNameH == NULL, "Missing locale module name string");
			moduleID = DmFindDatabaseWithTypeCreator(	systemCard,
																	(Char*)MemHandleLock(localeNameH),
																	sysFileTLocaleModule,
																	sysFileCSystem);
			MemHandleUnlock(localeNameH);
			DmCloseDatabase(overlayRef);
		}
		else
		{
			// Assume it's a ROM built without overlays, in which case we need to
			// get the locale name handle from the system base.
			DmOpenRef sysRef = NULL;
			Int16 index;
			
			do
			{
				LocalID dbID;
				
				sysRef = DmNextOpenResDatabase(sysRef);
				
				if ((sysRef != NULL)
				 && (DmOpenDatabaseInfo(sysRef, &dbID, NULL, NULL, NULL, NULL) == errNone)
				 && (dbID == systemID))
				{
				 	break;
				}
			} while (sysRef != NULL);

			ErrNonFatalDisplayIf(sysRef == NULL, "Can't find system DB");
			index = DmFindResource(sysRef, strRsc, localeModuleNameStrID, NULL);
			ErrNonFatalDisplayIf(index == -1, "Missing locale module name string");
			localeNameH = DmGetResourceIndex(sysRef, index);
			ErrNonFatalDisplayIf(localeNameH == NULL, "Missing locale module name string");
			moduleID = DmFindDatabaseWithTypeCreator(	systemCard,
																	(Char*)MemHandleLock(localeNameH),
																	sysFileTLocaleModule,
																	sysFileCSystem);
		}
		
		// If we found the locale module DB, open it up and make sure it has
		// the right version.
		if (moduleID != 0)
		{
			moduleRef = DmOpenDatabase(systemCard, moduleID, dmModeReadOnly | dmModeLeaveOpen);
			if ((moduleRef == 0)
			 || (ResLoadConstant(kIntlLMVersResID) != kIntlLMVersion))
			{
				// Trigger fatal alert below. No need to close the DB in that case.
				moduleID = 0;
			}
			else
			{
				// We found a valid locale module, using the ROM locale, so set
				// the current locale to be the ROM locale.
				result = FtrSet(sysFtrCreator, sysFtrNumLanguage, romLocale.language);
				
				if (result == errNone)
				{
					result = FtrSet(sysFtrCreator, sysFtrNumCountry, romLocale.country);
				}
				
				ErrNonFatalDisplayIf(result != errNone, "Can't switch current locale to ROM locale");
			}
		}
		
		ErrFatalDisplayIf(moduleID == 0, "No valid locale module");
	}
	
	result = DmDatabaseProtect(systemCard, moduleID, true);
	if (result != dmErrROMBased)
	{
		ErrNonFatalDisplayIf(result != errNone, "Can't protect locale module");
	}
#else
	// Give the locale module code in the Simulator a chance to
	// initialize itself. If there's no such code (e.g. with latin) then the
	// routine in SimStubs.cp does nothing.
	StubPalmOSInitLocaleModule();
#endif

	// If there's a locale module code resource, load & call it now.
	localeCodeH = DmGet1Resource(kIntlLMCodeType, kIntlLMCodeResID);
	if (localeCodeH != NULL)
	{
		ProcPtr localeCodeP = (ProcPtr)MemHandleLock(localeCodeH);
		(*localeCodeP)();
		MemHandleUnlock(localeCodeH);
	}
	
	// If the IntlMgr data ptr hasn't been set up by the locale module code,
	// allocate it now.
	if (GIntlMgrGlobalsP == NULL)
	{
		UInt16 numCustomTransTables;
		MemPtr globalPtr;

		numCustomTransTables = ResLoadConstant(kIntlLMCustomTransCountID);
		ErrNonFatalDisplayIf(numCustomTransTables > kIntlMaxCustomTransliterations,
									"Too many custom transliterations");
		
		globalPtr = MemPtrNew(sizeof(IntlGlobalsType) + (numCustomTransTables * sizeof(void*)));
		ErrNonFatalDisplayIf(globalPtr == NULL, "Out of memory");
		
		MemSet(globalPtr, sizeof(IntlGlobalsType) + (numCustomTransTables * sizeof(void*)), 0);
		GIntlMgrGlobalsP = globalPtr;
		
		GIntlData->numCustomTransliterations = numCustomTransTables;
	}
	else
	{
		// If any of the sorting tables are already configured, we can't do
		// our fast sorting optimization.
		for (index = 0; index < kIntlMaxSortLevels; index++)
		{
			if (GIntlData->sortTables[index] != NULL)
			{
				fastSort = false;
				break;
			}
		}
		
		// If the word start or word match tables are already configured, we
		// can't do our fast word search optimization.
		
		if ((GIntlData->findWordStartTable != NULL)
		 || (GIntlData->findWordMatchTable != NULL))
		{
			fastSearch = false;
		}
		
		// If the character attribute table is already configured, we can't do
		// our fast attribute optimization.
		if (GIntlData->charAttrTable != NULL)
		{
			fastAttr = false;
		}
	}
	
	// Loop over all of the table resources, locking & setting up pointers in
	// the IntlMgr global data structure.
	tableListH = DmGet1Resource(kIntlLMTableListType, kIntlLMTableListResID);
	if (tableListH != NULL)
	{
		IntlLMTableEntryType* curTable;
		UInt8* globalsP = (UInt8*)GIntlMgrGlobalsP;
		IntlLMTableResType* tableListP = (IntlLMTableResType*)MemHandleLock(tableListH);
		
		for (index = 0, curTable = tableListP->resources; index < tableListP->numResources; index++, curTable++)
		{
			void** ptrLocation;
			MemHandle tableH;
			
			tableH = DmGet1Resource(curTable->resType, curTable->resID);
			ErrFatalDisplayIf(tableH == NULL, "Missing table resource from locale module");
			ErrFatalDisplayIf(curTable->tableIndex >= intlLMNumTableIndexes, "Invalid table index");
			
			ptrLocation = (void**)(globalsP + kIntlGlobalOffsets[curTable->tableIndex]);
			ErrFatalDisplayIf(*ptrLocation != NULL, "Resource overriding existing table ptr");
			*ptrLocation = MemHandleLock(tableH);
		}
		
		MemHandleUnlock(tableListH);
	}

	// Note that we can't check for the presence of tables here, as a table that
	// is required by a Text Mgr routine might be NULL if the locale module has
	// patched out that routine.
	
	// As an optimization, see if all of the sorting tables are byte-indexed 8-bit
	// values. If so, then replace the table ptrs with ptrs to the actual data,
	// and set the fast sorting intl mgr flag.
	for (index = 0; (index < kIntlMaxSortLevels) && fastSort; index++)
	{
		void* sortTable = GIntlData->sortTables[index];
		if (sortTable == NULL)
		{
			break;
		}
		else if (PrvGetByteIndexedTable(sortTable, 8) == NULL)
		{
			fastSort = false;
		}
	}
	
	if (fastSort)
	{
		GIntlData->intlFlags |= kByteSortingFlag;
		
		for (index = 0; index < kIntlMaxSortLevels; index++)
		{
			GIntlData->sortTables[index] = PrvGetByteIndexedTable(GIntlData->sortTables[index], 8);
		}
	}
	
	// As an optimization, see if all of the searching tables are byte-indexed 8-bit
	// values. If so, then replace the table ptrs with ptrs to the actual data,
	// and set the fast searching intl mgr flag. Note that PrvGetByteIndexedTable
	// will return NULL if findWordMatchTable is NULL, which is OK because then
	// TxtFindString must be patched out anyway. The findWordStartTable is optional,
	// and thus it can either be NULL or a byte indexed byte table.
	if ((fastSearch)
	 && ((GIntlData->findWordStartTable == NULL)
	  || (PrvGetByteIndexedTable(GIntlData->findWordStartTable, 8) != NULL))
	 && (PrvGetByteIndexedTable(GIntlData->findWordMatchTable, 8) != NULL))
	{
		GIntlData->intlFlags |= kByteSearchingFlag;

		GIntlData->findWordStartTable = PrvGetByteIndexedTable(GIntlData->findWordStartTable, 8);
		GIntlData->findWordMatchTable = PrvGetByteIndexedTable(GIntlData->findWordMatchTable, 8);
	}
	
	// As an optimization, see if the character attribute table is a byte-indexed
	// table of 16-bit values. If so, then replace the table ptr with a ptr to
	// the data, and set the kByteCharAttrFlag flag.
	if (fastAttr && (PrvGetByteIndexedTable(GIntlData->charAttrTable, 16) != NULL))
	{
		GIntlData->intlFlags |= kByteCharAttrFlag;
		GIntlData->charAttrTable = PrvGetByteIndexedTable(GIntlData->charAttrTable, 16);
	
	// As an optimization, see if the character attribute table is a stage index map
	// table of 16-bit values. If so, then replace the table ptr with a ptr to
	// the sub-table offsets, set up the result table ptr, and set the
	// kStageCharAttrFlag flag.
	}
	else if (fastAttr && PrvGetStageIndexMapTable(GIntlData->charAttrTable, 16))
	{
		GIntlData->intlFlags |= kStageCharAttrFlag;
		GIntlData->charAttrTable = PrvGetStageIndexMapTable(GIntlData->charAttrTable, 16);
	}
	
	// If there's a doubleCharTable, we want to (a) verify that it's a byte-indexed
	// byte table, and (b) set up the pointer now.
	if (GIntlData->doubleCharTable != NULL)
	{
		void* tableP = PrvGetByteIndexedTable(GIntlData->doubleCharTable, 8);
		ErrNonFatalDisplayIf(tableP == NULL, "Double-char table isn't byte-indexed byte table");
		GIntlData->doubleCharTable = tableP;
	}
	
	// Load the 'feat' resource from the locale module, and use it to initialize
	// various locale-specific features. Typically this includes sysFtrNumCharEncodingFlags,
	// sysFtrNumEncoding, sysFtrDefaultFont, and sysFtrDefaultBoldFont.
	// Try to load the 'feat' resource.
	featureTableH = DmGet1Resource(sysResTFeatures, kIntlLMFeatureResID);
	ErrFatalDisplayIf(featureTableH == NULL, "Missing feature table");
	featureTableP = (ROMFtrTableType*)MemHandleLock(featureTableH);
	creatorTableP = featureTableP->creator;
	
	for (creators = 0; creators < featureTableP->numEntries; creators++)
	{
		UInt16 features;
		
		for (features = 0; features < creatorTableP->numEntries; features++)
		{
			result = FtrSet(	creatorTableP->creator,
									creatorTableP->feature[features].num,
									creatorTableP->feature[features].value);
			ErrNonFatalDisplayIf(result != errNone, "Can't set locale feature");
		}
		
		// Advance to next creator table.
		creatorTableP = (ROMFtrCreatorType*)((UInt8*)creatorTableP
								+ sizeof(ROMFtrCreatorType)
								+ (creatorTableP->numEntries * sizeof(ROMFtrFeatureType)));
	}
	
	MemHandleUnlock(featureTableH);
	
	// Now, set up our internal kByteSearchingFlag flag based on the sysFtrNumCharEncodingFlags
	// feature value. This means that sysFtrNumCharEncodingFlags is read-only, since if
	// anybody uses FtrSet to change the settings of the flags, that won't update
	// our internal flag(s).
	if ((FtrGet(sysFtrCreator, sysFtrNumCharEncodingFlags, &intlFtrFlags) == errNone)
	 && ((intlFtrFlags & charEncodingOnlySingleByte) != 0))
	{
		GIntlData->intlFlags |= kSingleByteOnlyFlag;
	}
	
	// Finally set the bit that tells everybody we're ready to support IntlMgr/TextMgr calls.
	FtrSet(sysFtrCreator, sysFtrNumIntlMgr, intlMgrExists);
} // IntlInit
