/******************************************************************************
 *
 * Copyright (c) 1999-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: IntlPrv.h
 *
 * Release: 
 *
 * Description:
 *	  This file defines private Int'l Mgr structures and routines.
 *
 * History:
 *	10/20/99	kwk	Created by Ken Krugler.
 *	05/11/00	kwk	Added locale module-related constants and types.
 *	05/19/00	kwk	Added kFastSortingFlag & intlFlags field to globals.
 *	05/26/00	kwk	Use 'wrap' in word wrapping class/break table names, to
 *					better distinguish them from the word bounds tables.
 *	05/31/00	kwk	Added kFastSearchingFlag.
 *	06/13/00	CS	Added optional realWordTable to IntlGlobalsType.
 *	07/13/00	kwk	Added nameToEncodingList to IntlGlobalsType, and
 *					the IntlLMCharsetListType.
 *	11/22/00	kwk	Added kFastCharAttrFlag flag.
 *	11/24/00	kwk	Moved the customTransliterations field to the end of the
 *					IntlGlobalsType structure, and made it variable length.
 *					Added numCustomTransliterations field.
 *	11/29/00	kwk	Added kStrictChecksFlag & IntlSetStrictChecks().
 *	12/11/00	CS	Renamed kFast* intlFlags kByte*.
 *				CS	Added kStageCharAttrFlag.
 *	12/20/00	kwk	Added doubleCharTable field to IntlGlobalsType structure.
 *
 *****************************************************************************/

#ifndef __INTLPRV_H__
#define __INTLPRV_H__

#include <PalmTypes.h>
#include <CoreTraps.h>
#include <SysEvent.h>
#include <IntlMgr.h>
#include <DataMgr.h>		// DmResType, DmResID
#include <PalmLocale.h>		// maxEncodingNameLength
#include <TextMgr.h>		// CharEncodingType

#ifdef _WIN32
  #pragma warning(disable: 4200)  // nonstandard extension used : zero-sized array in struct/union
#endif

/***********************************************************************
 * Private constants
 ***********************************************************************/

// Max number of sorting levels (and thus sorting tables)
#define	kIntlMaxSortLevels				6		// Maximum levels of sorting.

// Max number of standard (for all encodings) transliterations
#define	kIntlMaxStdTransliterations		4

// Max number of custom (encoding-specific) transliterations
#define kIntlMaxCustomTransliterations	16

// Flags for the IntlGlobalsType.intlFlags field
#define	kSingleByteOnlyFlag	0x00000001		// Encoding only has single-bytes.
#define	kByteSortingFlag	0x00000002		// All sorting tables are byte-indexed byte tables.
#define	kByteSearchingFlag	0x00000004		// All searching tables are byte-indexed byte tables.
#define	kByteCharAttrFlag	0x00000008		// Char attribute table is byte-indexed UInt16s
#define	kStrictChecksFlag	0x00000010		// Strict error checking
#define	kStageCharAttrFlag	0x00000020		// Char attribute table is stage-index-mapped UInt16s

/***********************************************************************
 * Private types
 ***********************************************************************/

// One entry that maps from a character set name to a CharEncodingType.
typedef struct _IntlLMCharsetEntryType IntlLMCharsetEntryType;
struct _IntlLMCharsetEntryType
{
	Char				name[maxEncodingNameLength+1];
	CharEncodingType	encoding;
};

// Format of the charset mapping resource ('csli'=32767) resource.
typedef struct _IntlLMCharsetListType IntlLMCharsetListType;
struct _IntlLMCharsetListType
{
	UInt16				numCharsets;	// number of following IntlLMCharsetEntryType records.
	IntlLMCharsetEntryType charsets[0];	// Variable number of character sets.
};

// One entry that maps from a resource type/id to an IntlMgr global struct field.
typedef struct _IntlLMTableEntryType IntlLMTableEntryType;
struct _IntlLMTableEntryType
{
	DmResType		resType;
	DmResID			resID;
	UInt16			tableIndex;
};

// Format of the locale module table resource list ('lmod'=32767) resource.
typedef struct _IntlLMTableResType IntlLMTableResType;
struct _IntlLMTableResType
{
	UInt16			numResources;		// number of IntlLMTableEntryType that follow:
	IntlLMTableEntryType resources[0];	// variable number of resources.
};

// The IntlMgr global structure. A pointer to this is saved in the IntlMgrData
// low-memory global.
typedef struct _IntlGlobalsType IntlGlobalsType;
struct _IntlGlobalsType
{
	UInt32 intlFlags;			// Various int'l mgr flags.
	
	void* byteAttrTable;
	void* charAttrTable;
	void* charXAttrTable;
	
	void* charValidTable;
	
	void* prevCharSizeTable;	// Used to calc size of preceding char.
	void* nextCharGetTable;		// Used to get the next character.
	void* nextCharSetTable;		// Used to set the next character.
	void* doubleCharTable;		// Used for fast double-byte get/set processing.
	
	void* charBoundsSizeTable;	// Used to calc bytes in char preceding offset.
	
	void* findWordStartTable;
	void* findWordMatchTable;
	
	void* wordStartClassTable;	// Classify chars into tokens for finding start of word.
	void* wordStartBreakTable;
	void* wordEndClassTable;	// Classify chars into tokens for finding end of word.
	void* wordEndBreakTable;
	
	void* charEncodingTable;
	void* maxEncodingTable;
	void* encodingToNameList;	// For mapping from encoding to encoding name
	void* nameToEncodingList;		// For mapping from encoding name to encoding
	
	void* sortTables[kIntlMaxSortLevels];
	
	void* deviceToIndexTable;
	void* indexToUnicodeTable;
	void* unicodeToDeviceTable;
	
	void* wordWrapClassTable;	// Optional table used to classify chars into
								// tokens, for finding word break position.
	void* wordWrapBreakTable;		// 

	void* realWordTable;		// Optional table used to determine whether a
								// set of chars contains more than just delimeters
								// (e.g., for the TxtWordBounds function result).
								
	void* stdTransliterations[kIntlMaxStdTransliterations];
	
	// Variable number of custom transliteration targets.
	UInt16 numCustomTransliterations;
	void* customTransliterations[0];
};

/***********************************************************************
 * Private macros
 ***********************************************************************/

#define	GIntlData	((IntlGlobalsType*)GIntlMgrGlobalsP)

/***********************************************************************
 * Private routines
 ***********************************************************************/

#ifdef __cplusplus
	extern "C" {
#endif

// Dispatcher that uses contents of register D2 to dispatch to the
// appropriate int'l routine. This routine declaration is only used
// when setting up the trap dispatch table; all callers of routines
// accessed via the dispatcher should use the explicit routine declarations
// (e.g. IntlInit below), which set up D2 before calling IntlDispatch.
void IntlDispatch(void);

// Dispatch table for IntlMgr routines...doesn't exist for the Simulator.
#if EMULATION_LEVEL == EMULATION_NONE
void IntlDispatchTable(void);
#endif

// Initialization routine, called at system reset time by PalmOS..
void IntlInit(void)
		INTL_TRAP(intlIntlInit);

// Return true if the international support wants to handle the event.
Boolean IntlHandleEvent(const SysEventType* inEventP, Boolean inProcess)
		INTL_TRAP(intlIntlHandleEvent);

// Set the kStrictChecksFlag in GIntlData->intlFlags on or off, and also
// set the corresponding intlMgrStrict flag in the IntlMgr feature.
Boolean	IntlSetStrictChecks(Boolean iStrictChecks)
		INTL_TRAP(intlIntlStrictChecks);

#ifdef __cplusplus
	}
#endif

#endif // __INTLPRV_H__
