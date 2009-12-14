/******************************************************************************
 *
 * Copyright (c) 1997-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: ExgMgr.c
 *
 * Release: 
 *
 * Description:
 *	  This file contains the Exchange Manager routines.
 *
 * History:
 *		May 29, 1997	Created by Gavin Peacock
 *		Mar 16, 2000	Gavin & Danny changes to support multiple exchange libraries
 *
 *****************************************************************************/

#define NON_PORTABLE

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_CLIPBOARDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_FIELDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_FORMS
#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS

#include <PalmTypes.h>

#include <Chars.h>
#include <ErrorMgr.h>
#include <ExgMgr.h>
#include <ExgLib.h>
#include <DateTime.h>
#include <IrLib.h>	// irLibName
#include <SysEvent.h>
#include <SystemMgr.h>
#include <Preferences.h>
#include <SoundMgr.h>
#include <SysEvtMgr.h>
#include <StringMgr.h>
#include <Window.h>
#include <TextMgr.h> // TxtCharIsHardKey
#include <Category.h>
#include <FeatureMgr.h>
#include <SysUtils.h>

#include "Globals.h"
#include "UIGlobals.h"

#include "ExgLocalLib.h"	// exgLocalScheme

// UI layer includes
#include <Find.h>
#include <Form.h>
#include <Menu.h>
#include "UIResourcesPrv.h"


// Amount to subtract from exgRegID when exgUnwrap flag is true.
#define exgRefUnwrapOffset	0x10

// Wildcard used in the registry. Not public yet.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS

// Maximum lengths of short and long string previews used in "do you wanna accept" dialog, excluding
// terminating null. The short string is used to construct a question using tSTR 11710. The result
// must fit in 200 characters, the length of tFLD 11703. (It should do so easily.) The long string
// goes directly in tFLD 11712, which has length 85. These constants were chosen so that a maximum
// size string consisting of typical characters (not very wide or very narrow) will fit in the
// available width.
#define maxShortPreviewLen		35
#define maxLongPreviewLen		85

// The creator to which the last object was delivered is stored in a feature.
#define exgFtrCreator			'exgm'
#define exgFtrNumLastCreator	0


// The Send Via dialog builds a sorted list of these for the user to choose from.
typedef struct {
	Char name[dmDBNameLength];
	UInt32 creatorID;
	} ExgLibInfoType;

// This structure is used to iterate over all the fields in all the registry records.
typedef struct
	{
		DmOpenRef dbR;
		UInt16 numResources;
		UInt16 resourceNumber;
		UInt32 creatorID;
		MemHandle resourceH;
		UInt16 flagBit;
		UInt16 flags;
		Char *typeP;
		Char *descriptionP;
		Boolean hasDescription;
		Boolean hasMultipleDescriptions;
	} ExgSearchStateType;


static Err PrvDoChooseDialog(ExgSocketType *socketP);
static Err PrvGetAppName(UInt16 cardNo, LocalID appDBID, Char *appName);


//---------------------------------------------------------------------------
// DlPrintF - debugging function
//---------------------------------------------------------------------------
// Uncomment if needed for special debugging
// DlPrintf will print to the sync log for debugging
#if 0
#include <unix_stdarg.h>	// Palm OS code uses this instead of <StdArg.h>
#include <DLServer.h>

static Int16 DlPrintF(const Char* formatStr, ...)
{
	va_list	args;
	Int16	result;
	char	s[500];
	
	// Check for err
	ErrNonFatalDisplayIf(s == NULL || formatStr == NULL, "NULL string passed");
	
	va_start(args, formatStr);
	result = StrVPrintF(s, formatStr, args);
	va_end(args);
	
	DlkSetLogEntry(s,StrLen(s),true);
	
	return result;
}
#endif


/************************************************************
 *
 * FUNCTION:		PrvGetRefNum
 *
 * DESCRIPTION:	Find the library reference number of a loaded
 *						exchange library. Assumes that exchange libraries
 *						are of type 'exgl', or they're wrapped in
 *						applications ('appl'). Also assumes that exchange
 *						libraries use the same name for the database and
 *						the last entry in the dispatch table. For the
 *						simulator, assumes that the library name ends
 *						with a dash and the creator ID.
 *
 * PARAMETERS:	creatorID	- the creator ID of the exg lib
 *					refNumP		- return the library reference number
 *
 * RETURNS:		0 if no error, else error code
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			dje	6/23/00	Created
 *
 *************************************************************/
static Err PrvGetRefNum(UInt32 creatorID, UInt16 *refNumP)
{
#if EMULATION_LEVEL == EMULATION_NONE
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
	DmSearchStateType stateInfo;
	UInt16 cardNum;
	UInt32 type;
	UInt32 bestType = 0;
	LocalID localID;
	Err err;
	Boolean newSearch = true;
	Char name[dmDBNameLength];
	Char bestName[dmDBNameLength];
	
	while (true)
		{
		err = DmGetNextDatabaseByTypeCreator(newSearch, &stateInfo, 0 /*wildcard type*/, creatorID, 
			true /*latest vers*/, &cardNum, &localID);
		if (err == dmErrCantFind)
			break;
		if (err)
			return err;
		err = DmDatabaseInfo(cardNum, localID, name, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
			&type, NULL);
		if (err)
			return err;
		if (type == sysFileTExgLib)
			{
			if (bestType != sysFileTExgLib)
				{
				bestType = type;
				StrCopy(bestName, name);
				}
			}
		else if (type == sysFileTApplication)
			{
			if (bestType == 0)
				{
				bestType = type;
				StrCopy(bestName, name);
				}
			}
		newSearch = false;
		}
	
	if (! bestType)
		{
		ErrNonFatalDisplay("Can't find exg lib DB");
		return exgErrBadLibrary;
		}
	
	err = SysLibFind(bestName, refNumP);
	return err;
#else
	Char suffix[6];
	UInt16 suffixLen;
	UInt16 refNum;
	SysLibTblEntryType *entryP;
	Char *libNameP;
	SysLibTblEntryType *tableP = (SysLibTblEntryType *)GSysLibTableP;
	SimDispatchTableType *tabP;
	Err err;
	
	suffix[0] = '-';
	MemMove(&suffix[1], &creatorID, sizeof(creatorID));
	suffix[5] = chrNull;
	suffixLen = StrLen(suffix);
	
	entryP = &(tableP[0]);
	for (refNum = 0; refNum < GSysLibTableEntries; refNum++, entryP++)
		{
		// Skip empty slots
		if (! entryP->dispatchTblP)
			continue;
		
		// Get the name pointer of this library
		tabP = (SimDispatchTableType *)entryP->dispatchTblP;
		libNameP = (Char *)tabP->entries[tabP->numEntries];
		
		// See if it matches
		if (StrCompare(suffix, &libNameP[StrLen(libNameP) - suffixLen]) == 0)
			{
			*refNumP = refNum;
			return errNone;
			}
		}
	if (creatorID == sysFileCIrLib)
		{
		err = SysLibFind(irLibName, refNumP);
		ErrNonFatalDisplayIf(err, "Can't find IR lib");
		return err;
		}
	return sysErrLibNotFound;
#endif
}


/************************************************************
 *
 * FUNCTION:		PrvCopyURLSchemes
 *
 * DESCRIPTION:	Copy the URL schemes from a socket's name field.
 *						Removes the initial '?', if any. Replaces the ';'
 *						separators with tabs. Doesn't include the ':'.
 *						Example:	socketP->name = "?foo;bar:bat.baz"
 *							result = "foo\tbar"
 *
 * PARAMETERS:	socketP	- the socket
 *
 * RETURNS:		a new string containing the schemes, or NULL
 *					if there isn't enough memory
 *
 * NOTE:			Caller must free result.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			dje	6/23/00	Created
 *
 *************************************************************/
static Char *PrvCopyURLSchemes(ExgSocketType *socketP)
{
	UInt32 schemesSize;
	Char *schemes;
	Char *startP = socketP->name;
	Char *endP = StrChr(startP, chrColon);
	UInt16 index;
	
	//ErrNonFatalDisplayIf(! endP, "Not a valid URL");
	if (*startP == chrQuestionMark)
		startP++;
	schemesSize = endP - startP + 1;
	schemes = MemPtrNew(schemesSize);
	if (! schemes)
		return NULL;
	MemMove(schemes, startP, schemesSize - 1);
	schemes[schemesSize - 1] = chrNull;
	for (index = 0; index < schemesSize - 1; index++)
		if (schemes[index] == chrSemicolon)
			schemes[index] = exgSeparatorChar;
	return schemes;
}


/************************************************************
 *
 * FUNCTION:		PrvGetNextType
 *
 * DESCRIPTION:	An iterator for the registry. Iterates over all the tab-separated
 *						extensions, MIME types, or URL schemes in all the registry
 *						records with the given ID.
 *
 * PARAMETERS:	stateP - a pointer to the iterator's state
 *					newSearch - set to true initially
 *					id - indicated whether we're scanning for extensions, MIME types, etc
 *						  and whether we're looking for exgUnwrap registry records
 *					creatorIDP - return the creator ID for the record containing this field
 *					isDefaultP - return whether this field is marked as the default
 *					typeP - pointer to buffer [exgMaxTypeLength + 1] to return the extension,
 *							  MIME type, etc
 *					descriptionP - pointer to buffer [exgMaxDescriptionLength + 1] to return
 *										the description
 *
 * RETURNS:		whether the iteration is complete (if it is, nothing is returned in
 *					creatorIDP, isDefaultP, typeP, or descriptionP)
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			dje	7/26/00	Created
 *
 * USAGE:
 *		ExgSearchStateType state;
 *		Boolean newSearch;
 *		UInt16 id;
 *		UInt32 creatorID;
 *		Boolean isDefault;
 *		Char *type = MemPtrNew(exgMaxTypeLength + 1);
 *		Char *description = MemPtrNew(exgMaxDescriptionLength + 1);
 *		
 *		if (!type || !description)
 *			goto Exit;
 *		id = exgRegExtensionID;	// or exgRegTypeID or exgRegSchemeID, or one of these minus exgRefUnwrapOffset
 *		newSearch = true;
 *		while (! PrvGetNextType(&state, newSearch, id, &creatorID, &isDefault, type, description))
 *			{
 *			...use creatorID, isDefault, type, and description...
 *			newSearch = false;
 *			}
 *		...
 * Exit:
 *		if (type)
 *			MemPtrFree(type);
 *		if (description)
 *			MemPtrFree(description);
 *
 * NOTE: Don't break out of the loop. You must wait until this function returns true for it
 *			to clean up properly.
 *
 *************************************************************/
static Boolean PrvGetNextType(ExgSearchStateType *stateP, Boolean newSearch, UInt16 id,
	UInt32 *creatorIDP, Boolean *isDefaultP, Char *typeP, Char *descriptionP)
{
	UInt16 resourceID;
	UInt32 resourceSize;
	Char *resourceP;
	Char *startP, *endP;
	
	if (newSearch)
		{ // Initialize the search state
		stateP->dbR = PrefOpenPreferenceDB(false);
		if (! stateP->dbR)
			return true;
		stateP->numResources = DmNumResources(stateP->dbR);
		if (! stateP->numResources)
			goto Exit;
		stateP->resourceNumber = 0;
		stateP->resourceH = NULL;
		// The remaining fields are only defined when resourceH isn't NULL.
		}
	
	while (true)
		{
		if (! stateP->resourceH)
			{
			DmResourceInfo(stateP->dbR, stateP->resourceNumber, &stateP->creatorID, &resourceID, NULL);
			if (resourceID == id)
				{ 
				stateP->resourceH = DmGetResourceIndex(stateP->dbR, stateP->resourceNumber);
				resourceSize = MemHandleSize(stateP->resourceH);
				resourceP = MemHandleLock(stateP->resourceH);
				stateP->flags = *((UInt16 *)resourceP)++;
				stateP->flagBit = 1;
				stateP->typeP = resourceP;
				stateP->hasDescription = sizeof(UInt16) + StrLen(resourceP) + 1 < resourceSize;
				if (stateP->hasDescription)
					{
					resourceP += StrLen(resourceP) + 1;
					stateP->descriptionP = resourceP;
					stateP->hasMultipleDescriptions = StrChr(resourceP, exgSeparatorChar) != NULL;
					}
				}
			else
				goto NextResource;
			}
		
		if (*stateP->typeP)
			{
			// Return the creatorID
			*creatorIDP = stateP->creatorID;
			
			// Return the next isDefault flag
			//ErrNonFatalDisplayIf(! stateP->flagBit, "More than 16 types");
			*isDefaultP = stateP->flags & stateP->flagBit;
			stateP->flagBit <<= 1;
			
			// Return the next type
			startP = stateP->typeP;
			endP = StrChr(startP, exgSeparatorChar);
			if (! endP)
				endP = StrChr(startP, chrNull);
			//ErrNonFatalDisplayIf(endP - startP > exgMaxTypeLength, "Type too long");
			MemMove(typeP, startP, endP - startP);
			typeP[endP - startP] = chrNull;
			stateP->typeP = endP;
			if (*endP)
				stateP->typeP++;	// skip over separator but not terminator
			
			// Return the next description, the only description, or no description
			if (stateP->hasDescription)
				{
				if (stateP->hasMultipleDescriptions)
					{
					startP = stateP->descriptionP;
					//ErrNonFatalDisplayIf(! *startP, "Not enough descriptions");
					endP = StrChr(startP, exgSeparatorChar);
					if (! endP)
						endP = StrChr(startP, chrNull);
					//ErrNonFatalDisplayIf(endP - startP > exgMaxDescriptionLength, "Description too long");
					MemMove(descriptionP, startP, endP - startP);
					descriptionP[endP - startP] = chrNull;
					stateP->descriptionP = endP;
					if (*endP)
						stateP->descriptionP++;	// skip over separator but not terminator
					}
				else
					{
					//ErrNonFatalDisplayIf(StrLen(stateP->descriptionP) > exgMaxDescriptionLength,
					//	"Description too long");
					StrCopy(descriptionP, stateP->descriptionP);
					}
				}
			else
				descriptionP[0] = chrNull;
			
			return false;
			}
		else
			//ErrNonFatalDisplayIf(stateP->hasDescription && stateP->hasMultipleDescriptions &&
			//	*stateP->descriptionP, "Too many descriptions");
		
NextResource:
		if (stateP->resourceH)
			{
			MemHandleUnlock(stateP->resourceH);
			stateP->resourceH = NULL;
			}
		stateP->resourceNumber++;
		if (stateP->resourceNumber >= stateP->numResources)
			break;
		}
	
Exit:
	DmCloseDatabase(stateP->dbR);
	stateP->dbR = NULL;	// so you can't continue
	return true;
}


/************************************************************
 *
 * FUNCTION:		PrvSetPreviousIsDefault
 *
 * DESCRIPTION:	Set the isDefault bit for the last field iterated over.
 *
 * PARAMETERS:	stateP - a pointer to the iterator's state
 *					id - indicated whether we're scanning for extensions, MIME types, etc
 *						  and whether we're looking for exgUnwrap registry records
 *					creatorID - the creator ID for the record containing the field
 *					isDefault - whether this field is a chosen one
 *
 * RETURNS:		nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			dje	8/4/00	Created
 *
 *************************************************************/
static void PrvSetPreviousIsDefault(ExgSearchStateType *stateP, Boolean isDefault)
{
	UInt16 flagBit;
	UInt16 flags;
	void *resourceP;
	
	// Undo the last iteration.
	flagBit = stateP->flagBit >> 1;
	if (flagBit == 0)	// bit got shifted off the top on last iteration
		flagBit = 0x8000;
	
	// Set or clear the flag bit.
	flags = stateP->flags;
	if (isDefault)
		flags |= flagBit;
	else
		flags &= ~flagBit;
	
	// Update the record.
	resourceP = MemHandleLock(stateP->resourceH);	// already locked by iterator, but we want the ptr
	DmWrite(resourceP, 0, &flags, sizeof(flags));
	MemHandleUnlock(stateP->resourceH);
	
	// No need to update the search state since we already iterated over this field.
}


/************************************************************
 *
 * FUNCTION:		PrvFindTarget
 *
 * DESCRIPTION:	Find the creator ID matching the given extension, MIME
 *						type, etc.
 *
 * PARAMETERS: id - registry id indicating whether a type, extension, etc
 *						  is being searched for, and whether exgUnwrap is set
 *					dataP - the type, extension, etc
 *             creatorIDP - creator ID of the target, or 0 if not found or error
 *					descriptionP - pointer to buffer to return description in
 *										or NULL
 *					descriptionSize - size of this buffer
 *					allowWildcard - whether to treat wildcards specially
 *					bestIsDefaultP - return whether the result is a "chosen one"
 *
 * RETURNS:		0 if no error, else error code (not finding a target isn't
 *					considered an error)
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			gavin				Created
 *			dje	7/26/00	Rewrote to use iterator
 *
 *************************************************************/
static Err PrvFindTarget(UInt16 id, const Char *dataP, UInt32 *creatorIDP, Char *descriptionP, UInt32 descriptionSize,
	Boolean allowWildcard, Boolean *bestIsDefaultP)
{
	ExgSearchStateType state;
	Boolean newSearch;
	UInt32 creatorID;
	UInt32 bestCreatorID = 0;
	Boolean isDefault;
	Boolean bestIsDefault = false;
	Char* type = MemPtrNew(exgMaxTypeLength + 1);
	Char* description = MemPtrNew(exgMaxDescriptionLength + 1);
	Char* bestDescription = MemPtrNew(exgMaxDescriptionLength + 1);
	Err err = errNone;
	
	if (!type || !description || !bestDescription)
		{
		err = exgMemError;
		goto Exit;
		}
	
	newSearch = true;
	while (! PrvGetNextType(&state, newSearch, id, &creatorID, &isDefault, type, description))
		{
		if (StrCaselessCompare(dataP, type) == 0 || allowWildcard && StrCaselessCompare(type, exgWildcard) == 0)
			{
			// We have a match. Remember the first one or the one that's marked as the default.
			if (! bestCreatorID || (isDefault && ! bestIsDefault))
				{
				bestCreatorID = creatorID;
				bestIsDefault = isDefault;
				if (descriptionP && descriptionSize)
					StrCopy(bestDescription, description);
				}
			}
		newSearch = false;
		}
	
	// Return the description
	if (bestCreatorID && descriptionP && descriptionSize)
		{
		StrNCopy(descriptionP, bestDescription, descriptionSize - 1);
		descriptionP[descriptionSize - 1] = chrNull;
		}
	
	// Return whether the result is a "chosen one"
	if (bestIsDefaultP)
		*bestIsDefaultP = bestIsDefault;
	
Exit:
	// Return the creator ID
	*creatorIDP = bestCreatorID;
	
	if (type)
		MemPtrFree(type);
	if (description)
		MemPtrFree(description);
	if (bestDescription)
		MemPtrFree(bestDescription);
	
	return err;
}


/************************************************************
 *
 * FUNCTION:		PrvGetDefaultLibrary
 *
 * DESCRIPTION:	Get the default exg lib registered for the
 *						specified URL scheme and stuff its ref num
 *						into the libraryRef field of the socket. Also
 *						return the exchange library's creator ID if
 *						there is a registry entry but no library with
 *						the creator ID. In this case, we assume it's
 *						an application.
 *
 * PARAMETERS:	socketP		- the socket
 *					creatorIDP	- return the creator ID when result is sysErrLibNotFound
 *
 * RETURNS:		0 if no error, else error code
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			dje	6/23/00	Created
 *
 *************************************************************/
static Err PrvGetDefaultLibrary(ExgSocketType *socketP, UInt32 *creatorIDP)
{
	Char *scheme = NULL;
	UInt32 creatorID;
	Err err = errNone;
	
	scheme = PrvCopyURLSchemes(socketP);
	if (! scheme)
		{
		err = exgMemError;
		goto FreeAndReturn;
		}
	
	err = PrvFindTarget(exgRegSchemeID, scheme, &creatorID, NULL, 0, true, NULL);
	if (err)
		goto FreeAndReturn;
	if (! creatorID)
		{
		err = exgErrBadLibrary;
		goto FreeAndReturn;
		}
	err = PrvGetRefNum(creatorID, &socketP->libraryRef);
	if (err == sysErrLibNotFound && creatorIDP)
		*creatorIDP = creatorID;
	
FreeAndReturn:
	if (scheme)
		MemPtrFree(scheme);
	return err;
}


/************************************************************
 *
 * FUNCTION:		PrvFindExgLib
 *
 * DESCRIPTION:	If the socket has no library reference number,
 *						fill it in using the URL scheme. Also return
 *						the exchange library's creator ID if there is
 *						a registry entry but no library with the
 *						creator ID. In this case, we assume it's an
 *						application. If no URL is specified, we assume
 *						IR, or Local if the localMode flag is set.
 *
 * PARAMETERS:	socketP		- pointer the socket
 *					creatorIDP	- return the creator ID when result is sysErrLibNotFound
 *
 * RETURNS:		0 if no error, else error code
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			dje	6/16/00	created
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
 *
 *************************************************************/
static Err PrvFindExgLib(ExgSocketType *socketP, UInt32 *creatorIDP)
{
	Err err = errNone;
	
	if (! socketP->libraryRef)
		{
		if (socketP->name && StrChr(socketP->name, chrColon))
			{ // We have a URL
			if (socketP->name[0] == chrQuestionMark)
				err = PrvDoChooseDialog(socketP);
			else
				err = PrvGetDefaultLibrary(socketP, creatorIDP);
			}
		else
			{
			UInt32 creatorID;
			
			err = PrvFindTarget(exgRegSchemeID, (socketP->localMode ? exgLocalScheme : exgBeamScheme),
				&creatorID, NULL, 0, true, NULL);
			if (! err && (! creatorID || PrvGetRefNum(creatorID, &socketP->libraryRef)))
				err = exgErrBadLibrary;
			}

		}
	return err;
}


/************************************************************
 *
 * FUNCTION:		PrvCompareTypes
 *
 * DESCRIPTION:	Compare two extensions, MIME types, or URL schemes.
 *
 * PARAMETERS:	type1 - the first extension, MIME type, or URL scheme
 *					type2 - the second extension, MIME type, or URL scheme
 *					other - unused
 *
 * RETURNS:		> 0 if type1 > type2
 *					= 0 if type1 == type2
 *					< 0 if type1 < type2
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			dje	7/31/00	Created
 *
 *************************************************************/
static Int16 PrvCompareTypes(void const *type1, void const *type2, Int32 other)
{
	#pragma unused (other)
	return StrCaselessCompare(*(Char **)type1, *(Char **)type2);
}


/************************************************************
 *
 * FUNCTION:		PrvGetLaunchInfo
 *
 * DESCRIPTION:	Get applicaton card and databaseID given creator
 *
 *
 * PARAMETERS:	creator - creator ID to find
 *					cardP - returns the card ID
 *					dbIDP - returns the databaseID
 *
 * RETURNS: 	error if app was not found
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			gavin	5/29/97	Created
 *
 *************************************************************/
static Err PrvGetLaunchInfo(UInt32 creator, UInt16 *cardP, LocalID *dbIDP)
{
	Err err;
	DmSearchStateType	searchState;
										
	/* Find the application */											
	err = DmGetNextDatabaseByTypeCreator(						
			true, &searchState,sysFileTApplication, creator,											
			true, cardP, dbIDP);	
	if (err) // also check for panels...
		err = DmGetNextDatabaseByTypeCreator(						
				true, &searchState,sysFileTPanel, creator,											
				true, cardP, dbIDP);
					
#if EMULATION_LEVEL!= EMULATION_NONE
		// in emulator, the app will not exist, set app info to zero because
		// Emulator version of SysCurAppDatabase will return those values...
		err = errNone;
		*cardP = 0;
		*dbIDP = 0;
#endif
	return err;
}


/************************************************************
 *
 * FUNCTION:		PrvGetLibOrAppName
 *
 * DESCRIPTION:	Get the "friendly" name of the exchange library
 *						or application with the given creator ID. (If
 *						both exist, use the exchange library.) For
 *						exchange libraries, this is the result of the
 *						exgLibCtlGetTitle operation. For applications,
 *						this is the tAIN 1000 resource. If no "friendly"
 *						name is available, use the database name.
 *
 * PARAMETERS:	name			- return the name of the exg lib or app
 *									  (buffer should be [dmDBNameLength])
 *					creatorID	- the creator ID of the exg lib or app
 *
 * RETURNS:		0 if no error, else error code
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			dje	9/29/00	Created
 *
 *************************************************************/
static Err PrvGetLibOrAppName(Char *name, UInt32 creatorID)
{
	UInt32 type;
	DmSearchStateType stateInfo;
	UInt16 cardNo;
	LocalID dbID;
	Err err;
	
	// Find the exchange library or application database
	type = sysFileTExgLib;
	do
		{
		err = DmGetNextDatabaseByTypeCreator(true /*new search*/, &stateInfo, type, creatorID,
			true /*latest version*/, &cardNo, &dbID);
		if (err)
			{
			if (type == sysFileTExgLib)
				type = sysFileTApplication;
			else
				{
				err = exgErrTargetMissing;
				goto Exit;
				}
			}
		} while (err);
	
	if (type == sysFileTExgLib)
		{
		Char dbName[dmDBNameLength];
		UInt16 refNum;
		
		err = DmDatabaseInfo(cardNo, dbID, dbName, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		if (! err)
			{
			err = SysLibFind(dbName, &refNum);
			if (! err)
				{
				UInt16 nameSize = exgTitleBufferSize;
				
				ErrNonFatalDisplayIf(exgTitleBufferSize > dmDBNameLength, "Exg title > DB name");
				err = ExgLibControl(refNum, exgLibCtlGetTitle, name, &nameSize);
				}
			}
		
		// Use the database name if no "friendly" name is available
		if (err)
			err = DmDatabaseInfo(cardNo, dbID, name, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		}
	else	// type == sysFileTApplication
		err = PrvGetAppName(cardNo, dbID, name);
	
Exit:
#if EMULATION_LEVEL != EMULATION_NONE
	// The current app doesn't exist as a database in the simulator. Neither do the IrDA or Local
	// exchange libraries. If we don't find what we were looking for, we check if it's the IrDA
	// or Local exchange libraries. If it isn't, we assume we're looking for the current app.
	if (err == exgErrTargetMissing)
		{
		switch (creatorID)
			{
			case sysFileCIrLib:
				StrCopy(name, "Beam");	// don't bother localizing the simulator
				break;
			case sysFileCLocalLib:
				StrCopy(name, "Transfer");	// don't bother localizing the simulator
				break;
			default:
				StrCopy(name, "the current application");	// don't bother localizing the simulator
			}
		err = errNone;
		}
#endif
	
	return err;
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION:		ExgInit
 *
 * DESCRIPTION:	Initialize the Exchange system. Currently this is used to
 *						load the ir library
 *
 *
 * PARAMETERS:		none
 *
 * RETURNS:			0 if no error, else error code
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			gavin	5/29/97	Created
 *
 *************************************************************/
Err ExgInit(void)
{
	Err err = errNone;
	UInt16 refNum = 0;
#if EMULATION_LEVEL != EMULATION_NONE
	SysLibEntryProcPtr libP = NULL;
	
	Err IrdPrvInstallDispatcher(UInt16 refNum, SysLibTblEntryType *entryP);
	Err ExgLocalLibPrvInstallDispatcher(UInt16 refNum, SysLibTblEntryType *entryP);

	// Install the Local exchange library
	libP = ExgLocalLibPrvInstallDispatcher;
	err = SysLibInstall(libP, &refNum);
	
	// Install the Irda library
	libP = IrdPrvInstallDispatcher;
	err = SysLibInstall(libP, &refNum);
#else
	SysLibLoad(sysFileTExgLib, sysFileCLocalLib, &refNum);	// ignore errors
	
	//UInt32 creator = PrefGetPreference(prefExgDefaultLibrary);
	//if (! (GSysResetFlags & sysResetFlagNoExtensions)) 
	err = SysLibLoad(sysFileTExgLib,sysFileCIrLib,&refNum);
#endif
	//ErrNonFatalDisplayIf(err,"Ir Library not loaded");
		
	return err;
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION:		ExgRegisterData
 *
 * DESCRIPTION:	Registers a data data type with Exg so that incomming data
 *						with that type will be sent to the registering app.
 *
 *
 * PARAMETERS:	creatorID	-- application's creator ID
 *					dataTypes	-- MemPtr a string buffer with the data types to register
 *
 * RETURNS: 0 if no error, else error code
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			gavin	5/29/97	Created
 *			dje	7/21/00	Call through to new trap
 *
 *************************************************************/
Err ExgRegisterData(UInt32 creatorID, UInt16 id, const Char *dataTypesP)
{
	return ExgRegisterDatatype(creatorID, id, dataTypesP, NULL /*descriptions*/, 0 /*flags*/);
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION:		ExgRegisterDatatype
 *
 * DESCRIPTION:	New version of ExgRegisterData. Registers an application
 *						to receive objects with the specified extensions or
 *						MIME types, or registers an exchange library or
 *						application to handle the specified URL schemes.
 *						Descriptions are optional; pass in NULL to leave out. A
 *						single description string can be used for all the
 *						extensions, MIME types, or URL schemes. Pass in NULL for
 *						dataTypesP to unregister.
 *
 * PARAMETERS:	creatorID		- application or exg lib's creator ID
 *					id					- one of the exgRegID constants
 *					dataTypesP		- tab-separated types
 *					descriptionsP	- tab-separated descriptions
 *					flags				- whether to unwrap attachments
 *
 * RETURNS:		0 if no error, else error code
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			dje	6/21/00	Created from previous version
 *
 *************************************************************/
Err ExgRegisterDatatype(UInt32 creatorID, UInt16 id, const Char *dataTypesP,
	const Char *descriptionsP, UInt16 flags)
{
	Int16 size = 0;
	void *prefP = NULL;
	Err err;
	
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	{
		const Char *startP;
		const Char *endP;
		UInt16 numTypes = 0;
		UInt16 numDescriptions = 0;
		
		// Check that types aren't too long
		startP = dataTypesP;
		while (dataTypesP && *startP)
			{
			endP = StrChr(startP, exgSeparatorChar);
			if (! endP)
				endP = StrChr(startP, chrNull);
			ErrNonFatalDisplayIf(endP - startP > exgMaxTypeLength, "Type too long");
			startP = endP;
			if (*startP)	// skip over separator but not terminator
				startP++;
			numTypes++;
			}
		
		// Check that descriptions aren't too long
		startP = descriptionsP;
		while (descriptionsP && *startP)
			{
			endP = StrChr(startP, exgSeparatorChar);
			if (! endP)
				endP = StrChr(startP, chrNull);
			ErrNonFatalDisplayIf(endP - startP > exgMaxDescriptionLength, "Description too long");
			startP = endP;
			if (*startP)	// skip over separator but not terminator
				startP++;
			numDescriptions++;
			}
		
		// Check that the descriptions correspond to the type, or there's just one for all the types
		if (numDescriptions > 1)
			{
			ErrNonFatalDisplayIf(numDescriptions < numTypes, "Not enough descriptions");
			ErrNonFatalDisplayIf(numDescriptions > numTypes, "Too many descriptions");
			}
		
		// Check that there aren't too many types
		ErrNonFatalDisplayIf(numTypes > 16, "More than 16 types");	// we use a UInt16 for the isDefault bits
	}
#endif
	
	if (dataTypesP)
		{
		size = StrLen(dataTypesP) + 1;
		if (descriptionsP)
			size += StrLen(descriptionsP) + 1;
		prefP = MemPtrNew(size);
		if (! prefP)
			return exgMemError;
		StrCopy(prefP, dataTypesP);
		if (descriptionsP)
			StrCopy(StrChr(prefP, chrNull) + 1, descriptionsP);
		}
	
	if (flags & exgUnwrap)
		id -= exgRefUnwrapOffset;
	
	PrefSetAppPreferences(creatorID, id, exgDataPrefVersion, prefP, size, false);
	
	// Get dmLastErr immediately after PrefSetAppPreferences()
	err = DmGetLastErr();
	// This used to be unsafe because dmLastErr might have been non-zero before PrefSetAppPreferences().
	// Now all routines that set dmLastErr if there's an error also clear it if there isn't, so this is safe.
	
	if (prefP)
		MemPtrFree(prefP);
	return err;
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION:		ExgSetDefaultApplication
 *
 * DESCRIPTION:	Set the default application to receive a particular
 *						extension, MIME type, or creator ID, or the default
 *						exg lib to handle a particular URL scheme. You can
 *						specify the implicitly registered owner as the
 *						default for a creator ID.
 *
 * PARAMETERS:	creatorID	- application or exg lib's creator ID
 *					id				- one of the exgRegID constants
 *					dataTypeP	- extension, MIME type, creator ID, or URL scheme
 *
 * RETURNS:		0 if no error, else error code
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			dje	6/21/00	Created
 *
 *************************************************************/
Err ExgSetDefaultApplication(UInt32 creatorID, UInt16 id, const Char *dataTypeP)
{
	ExgSearchStateType state;
	Boolean newSearch;
	UInt32 creator;
	Boolean isDefault;
	Char *type = MemPtrNew(exgMaxTypeLength + 1);
	Char *description = MemPtrNew(exgMaxDescriptionLength + 1);
	Boolean found = false;
	Err err = errNone;
	
	if (!type || !description)
		{
		err = exgMemError;
		goto Exit;
		}
	
	// See if the given app or lib is registered for the given data type
	newSearch = true;
	while (! PrvGetNextType(&state, newSearch, id, &creator, &isDefault, type, description))
		{
		if (StrCaselessCompare(dataTypeP, type) == 0 && creator == creatorID)
			found = true;
		newSearch = false;
		}
	
	// Allow setting the implicitly registered owner as the default for its creator ID
	if (id == exgRegCreatorID && MemCmp(&creatorID, dataTypeP, sizeof(creatorID)) == 0)
		found = true;
	
	if (! found)
		{
		err = exgErrNoKnownTarget;
		goto Exit;
		}
	
	// Mark the given record as the chosen one, unmarking the previous chosen one
	newSearch = true;
	while (! PrvGetNextType(&state, newSearch, id, &creator, &isDefault, type, description))
		{
		if (StrCaselessCompare(dataTypeP, type) == 0 && (creator == creatorID) != (isDefault != 0))
				// make sure isDefault is 0 or 1 before comparing
			PrvSetPreviousIsDefault(&state, ! isDefault);
				// mark or unmark it as a chosen one.
		newSearch = false;
		}
	
Exit:
	if (type)
		MemPtrFree(type);
	if (description)
		MemPtrFree(description);
	
	return err;
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION:	ExgGetDefaultApplication
 *
 * DESCRIPTION:	Get the default application to receive a particular
 *						extension, MIME type, or creator ID, or the default
 *						exg lib to handle a particular URL scheme. If no
 *						default has been selected, one will be chosen
 *						arbitrarily. If no applications explicitly registered
 *						for a creator ID, or none of the explicitly registered
 *						applications are chosen as the default, then the
 *						implicitly registered owner will be used.
 *
 * PARAMETERS:	creatorIDP - returned application or exg lib creator ID
 *					id - one of the exgRegID constants
 *					dataTypeP - extension, MIME type, or URL scheme
 *
 * RETURNS:		0 if no error, else error code
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			dje	8/4/00	Created
 *
 *************************************************************/
Err ExgGetDefaultApplication(UInt32 *creatorIDP, UInt16 id, const Char *dataTypeP)
{
	UInt32 creator;
	Boolean isDefault;
	Err err;
	
	err = PrvFindTarget(id, dataTypeP, &creator, NULL, 0, false, &isDefault);
	if (err)
		return err;
	if (id == exgRegCreatorID && (! creator || ! isDefault))
		{
		UInt32 appCreatorID;
		UInt16 cardNo;
		LocalID dbID;
		
		MemMove(&appCreatorID, dataTypeP, sizeof(appCreatorID));
		if (PrvGetLaunchInfo(appCreatorID, &cardNo, &dbID) == errNone)
			creator = appCreatorID;
		}
	
	if (creator)
		{
		*creatorIDP = creator;
		return errNone;
		}
	else
		return exgErrNoKnownTarget;
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION:		ExgGetTargetApplication
 *
 * DESCRIPTION:	Get the target application to receive an object.
 *						Also returns the description from the registry for
 *						the extension or MIME type used. If the creator ID
 *						is used, and the application didn't explicitly
 *						register for it, then the description will be an
 *						empty string. Verifies that the application exists
 *						when necessary.
 *
 * PARAMETERS:	socketP - the object
 *					unwrap - whether the object was unwrapped (and should
 *								therefore only be delivered to applications
 *								that registered specially)
 *					creatorIDP - returned application creator ID
 *					descriptionP - returned description from registry
 *					descriptionSize - size of buffer
 *
 * RETURNS:		0 if no error, else error code
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			dje	7/25/00	Created
 *
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
 *
 *************************************************************/
Err ExgGetTargetApplication(ExgSocketType *socketP, Boolean unwrap, UInt32 *creatorIDP,
	Char *descriptionP, UInt32 descriptionSize)
{
	UInt32 target = 0;
	UInt16 id;
	UInt16 cardNo;
	LocalID dbID;
	Err err;
	Err result = exgErrNoKnownTarget;	// error code to return if target is zero
	Char creatorString[5];
	Boolean isDefault;
	
	// Check if anyone registered for the specified creator ID.
	if (socketP->target)
		{
		id = exgRegCreatorID;
		if (unwrap)
			id -= exgRefUnwrapOffset;
		MemMove(creatorString, &socketP->target, sizeof(socketP->target));
		creatorString[sizeof(socketP->target)] = chrNull;
		err = PrvFindTarget(id, creatorString, &target, descriptionP, descriptionSize, true, &isDefault);
		if (err)
			return err;
		// the application must exist because there's a preference record
		}
	
	// Check for application with the specified creator ID.
	if ((! target || ! isDefault) && ! unwrap && socketP->target)
		{
		// Make sure the app exists.
		if (PrvGetLaunchInfo(socketP->target, &cardNo, &dbID) == errNone)
			{
			target = socketP->target;
			if (descriptionP && descriptionSize)
				descriptionP[0] = chrNull;			// no description available in this case
			}
		else
			{
			// try the registry, but if no one registered, return a different error code
			result = exgErrTargetMissing;
			}
		}
	
	// Check if anyone registered for the specified MIME type.
	if (! target && socketP->type)
		{
		id = exgRegTypeID;
		if (unwrap)
			id -= exgRefUnwrapOffset;
		err = PrvFindTarget(id, socketP->type, &target, descriptionP, descriptionSize, true, NULL);
		if (err)
			return err;
		// the application must exist because there's a preference record
		}
	
	// Check if anyone registered for the specified extension.
	if (! target && socketP->name && socketP->name[0])
		{
		UInt16 index = StrLen(socketP->name);
		
		id = exgRegExtensionID;
		if (unwrap)
			id -= exgRefUnwrapOffset;
		
		// scan backwards for start of extension
		do
			{
			index--;
			if (socketP->name[index] == '.')
				{ // don't include dot in extension
				err = PrvFindTarget(id, &socketP->name[index + 1], &target, descriptionP, descriptionSize, true, NULL);
				if (err)
					return err;
				// the application must exist because there's a preference record
				break;
				}
			}
		while (index > 0);	// no dot found -> give up
		}
	
	if (target)
		{
		*creatorIDP = target;
		return errNone;
		}
	else
		return result;
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION:		ExgGetRegisteredApplications
 *
 * DESCRIPTION:	Get a list of applications or exg libs registered for
 *						a specified MIME type, extension, URL scheme, or creator
 *						ID. If several are specified, the union is returned.
 *
 * PARAMETERS:	creatorIDsP		- returned apps or exg libs
 *					numAppsP			- number of items returned
 *					namesP			- returned null-separated names
 *										  corresponding to creator IDs (pass
 *										  NULL to skip)
 *					descriptionsP	- returned null-separated descriptions
 *										  corresponding to creator IDs (pass
 *										  NULL to skip)
 *					id					- one of the exgRegID constants
 *					dataTypeP		- extension, MIME type, or URL scheme (or
 *										  several tab-separated ones
 *
 * RETURNS:		0 if no error, else error code
 *
 * NOTE:			Caller must free creator IDs, names, and descriptions
 *					returned. Names returned are the "friendly" names of the
 *					exchange libraries and applications.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			dje	6/21/00	Created
 *
 *************************************************************/
Err ExgGetRegisteredApplications(UInt32 **creatorIDsP, UInt32 *numAppsP,
	Char **namesP, Char **descriptionsP, UInt16 id, const Char *dataTypeP)
{
	ExgSearchStateType state;
	Boolean newSearch;
	UInt32 creatorID;
	Boolean isDefault;
	Char *type = MemPtrNew(exgMaxTypeLength + 1);
	Char name[dmDBNameLength];
	Char *description = MemPtrNew(exgMaxDescriptionLength + 1);
	Boolean found;
	const Char *patternStart;
	const Char *patternEnd;
	UInt16 patternLen;
	UInt16 numApps;
	UInt32 namesSize, descriptionsSize;
	UInt32 *creatorIDs = NULL;
	UInt16 creatorIDIndex;
	Char *names = NULL;
	Char *descriptions = NULL;
	Char *namesDest, *descriptionsDest;
	Err err = errNone;
	
	if (!type || !description)
		{
		err = exgMemError;
		goto Exit;
		}
	
	// See how big the results will be.
	numApps = 0;
	namesSize = 0;
	descriptionsSize = 0;
	newSearch = true;
	while (! PrvGetNextType(&state, newSearch, id, &creatorID, &isDefault, type, description))
		{
		found = false;
		patternStart = dataTypeP;
		while (*patternStart)
			{
			patternEnd = StrChr(patternStart, exgSeparatorChar);
			if (! patternEnd)
				patternEnd = StrChr(patternStart, chrNull);
			patternLen = patternEnd - patternStart;
			if (StrLen(type) == patternLen && StrNCaselessCompare(patternStart, type, patternLen) == 0)
				found = true;
			if (*patternEnd)
				patternStart = patternEnd + 1;
			else
				patternStart = patternEnd;
			}
		if (found)
			{
			numApps++;
			if (namesP)
				{
				err = PrvGetLibOrAppName(name, creatorID);
				if (err)
					goto Exit;
				
				namesSize += StrLen(name) + 1;
				}
			if (descriptionsP)
				descriptionsSize += StrLen(description) + 1;
			}
		newSearch = false;
		}
	
	// If result is empty or caller just wants a count, return now.
	if (! numApps || ! creatorIDsP)
		{
		if (numAppsP)
			*numAppsP = numApps;
		if (creatorIDsP)
			*creatorIDsP = NULL;
		goto Exit;
		}
	
	// Allocate chunk to return creator IDs.
	creatorIDs = MemPtrNew(numApps * sizeof(UInt32));
	if (! creatorIDs)
		{
		err = exgMemError;
		goto Exit;
		}
	
	// Allocate chunk to return names.
	if (namesP && namesSize)
		{
		names = MemPtrNew(namesSize);
		if (! names)
			{
			err = exgMemError;
			goto Exit;
			}
		}
	
	// Allocate chunk to return descriptions.
	if (descriptionsP && descriptionsSize)
		{
		descriptions = MemPtrNew(descriptionsSize);
		if (! descriptions)
			{
			err = exgMemError;
			goto Exit;
			}
		}
	
	// Fill in results.
	creatorIDIndex = 0;
	newSearch = true;
	namesDest = names;
	descriptionsDest = descriptions;
	while (! PrvGetNextType(&state, newSearch, id, &creatorID, &isDefault, type, description))
		{
		found = false;
		patternStart = dataTypeP;
		while (*patternStart)
			{
			patternEnd = StrChr(patternStart, exgSeparatorChar);
			if (! patternEnd)
				patternEnd = StrChr(patternStart, chrNull);
			patternLen = patternEnd - patternStart;
			if (StrLen(type) == patternLen && StrNCaselessCompare(patternStart, type, patternLen) == 0)
				found = true;
			if (*patternEnd)
				patternStart = patternEnd + 1;
			else
				patternStart = patternEnd;
			}
		if (found)
			{
			creatorIDs[creatorIDIndex++] = creatorID;
			if (namesP && namesSize)
				{
				err = PrvGetLibOrAppName(name, creatorID);
				ErrNonFatalDisplayIf(err, "PrvGetLibOrAppName failed on 2nd pass");
				StrCopy(namesDest, name);
				namesDest = StrChr(namesDest, chrNull) + 1;
				}
			if (descriptionsP)
				{
				StrCopy(descriptionsDest, description);
				descriptionsDest = StrChr(descriptionsDest, chrNull) + 1;
				}
			}
		newSearch = false;
		}
	
	ErrNonFatalDisplayIf(creatorIDIndex != numApps, "2nd pass different");
	ErrNonFatalDisplayIf(namesP && namesSize &&
		namesDest - names != namesSize, "2nd pass different");
	ErrNonFatalDisplayIf(descriptionsP && descriptionsSize &&
		descriptionsDest - descriptions != descriptionsSize, "2nd pass different");
	
	if (numAppsP)
		*numAppsP = numApps;
	*creatorIDsP = creatorIDs;
	creatorIDs = NULL;		// don't free the chunk we're returning
	if (names)
		{
		*namesP = names;
		names = NULL;			// don't free the chunk we're returning
		}
	if (descriptions)
		{
		*descriptionsP = descriptions;
		descriptions = NULL;	// don't free the chunk we're returning
		}
	err = errNone;

Exit:
	if (creatorIDs)
		MemPtrFree(creatorIDs);
	if (names)
		MemPtrFree(names);
	if (descriptions)
		MemPtrFree(descriptions);
	if (type)
		MemPtrFree(type);
	if (description)
		MemPtrFree(description);
	
	return err;
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION:		ExgGetRegisteredTypes
 *
 * DESCRIPTION:	Get a list of MIME types, extensions, URL schemes,
 *						or creator IDs that have been registered for. Result
 *						is sorted.
 *
 * PARAMETERS:	dataTypesP - returned null-separated extensions, MIME
 *									 types, or URL schemes (pass NULL to skip)
 *					sizeP - number of items returned
 *					id - one of the exgRegID constants
 *
 * RETURNS:		0 if no error, else error code
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			dje	7/31/00	Created
 *
 *************************************************************/
Err ExgGetRegisteredTypes(Char **dataTypesP, UInt32 *sizeP, UInt16 id)
{
	ExgSearchStateType state;
	Boolean newSearch;
	UInt32 creatorID;
	Boolean isDefault;
	Char *type = MemPtrNew(exgMaxTypeLength + 1);
	Char *description = MemPtrNew(exgMaxDescriptionLength + 1);
	UInt16 maxNumTypes;
	UInt16 numTypes;
	UInt16 counter;
	UInt32 maxResultSize;
	Int32 position;
	Char **typePs = NULL;
	Char *types = NULL;
	Char *typeP;
	Char *nextType;
	Char *result = NULL;
	Err err = errNone;
	
	if (!type || !description)
		{
		err = exgMemError;
		goto CleanUpAndReturn;
		}
	
	// Count all registrations for extensions, MIME types, URL schemes, or creator IDs. Duplicates
	// will be counted repeatedly, so the results may be larger than necessary.
	maxNumTypes = 0;
	maxResultSize = 0;
	newSearch = true;
	while (! PrvGetNextType(&state, newSearch, id, &creatorID, &isDefault, type, description))
		{
		maxNumTypes++;
		maxResultSize += StrLen(type) + 1;
		newSearch = false;
		}
	
	// Allocate space for array of pointers to the types and for the types themselves. The pointers
	// are kept sorted, but the types themselves aren't.
	if (maxNumTypes)
		{
		typePs = MemPtrNew(maxNumTypes * sizeof(Char *));
		types = MemPtrNew(maxResultSize);
		if (! typePs || ! types)
			{
			err = exgMemError;
			goto CleanUpAndReturn;
			}
		nextType = types;
		}
	numTypes = 0;
	
	// Now fill in the types.
	newSearch = true;
	while (! PrvGetNextType(&state, newSearch, id, &creatorID, &isDefault, type, description))
		{
		// Do a binary search to see if we have this type already, and to find out where to insert
		// it if we don't.
		typeP = type;
		if (! SysBinarySearch(typePs, numTypes, sizeof(Char *), &PrvCompareTypes, &typeP,
									 0 /*other*/, &position, true /*findFirst*/))
			{
			for (counter = numTypes; counter > position; counter--)
				typePs[counter] = typePs[counter - 1];
			typePs[position] = nextType;
			numTypes++;
			StrCopy(nextType, type);
			nextType += StrLen(type) + 1;
			}
		
		newSearch = false;
		}
	
	// Return the types in sorted order.
	if (dataTypesP)
		{
		if (numTypes)
			{
			result = MemPtrNew(nextType - types);
			if (! result)
				{
				err = exgMemError;
				goto CleanUpAndReturn;
				}
			nextType = result;
			for (counter = 0; counter < numTypes; counter++)
				{
				StrCopy(nextType, typePs[counter]);
				nextType += StrLen(nextType) + 1;
				}
			}
		*dataTypesP = result;
		}
	*sizeP = numTypes;
	
CleanUpAndReturn:
	if (type)
		MemPtrFree(type);
	if (description)
		MemPtrFree(description);
	if (typePs)
		MemPtrFree(typePs);
	if (types)
		MemPtrFree(types);
	return err;
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION:		ExgConnect
 *
 * DESCRIPTION:	Establish a connection with a remote socket
 *						This does not tranfer and data content
 *
 *
 * PARAMETERS:	dataPtr pointer a ExgDataType structure
 *
 * RETURNS:		0 if no error, else error code
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			gavin	5/29/97	created
 *
 *************************************************************/
Err ExgConnect(ExgSocketType *socketP)
{
	Err err;
	
	err = PrvFindExgLib(socketP, NULL);
	if (! err)
		err = ExgLibConnect(socketP->libraryRef, socketP);
	
	if (err == exgErrNotEnoughPower)
		ErrAlertCustom(pwrErrBeam,0,0,0);

	return err;
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION:		ExgPut
 *
 * DESCRIPTION:	Sends the data in socketP using the active EXG device
 *						to the active destination.
 *
 *
 * PARAMETERS:	dataPtr pointer a exgDataType structure
 *
 * RETURNS:		0 if no error, else error code
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			gavin	5/29/97	created
 *
 *************************************************************/
Err ExgPut(ExgSocketType *socketP)
{
	Err err;
	
	err = PrvFindExgLib(socketP, NULL);
	if (! err)
		err = ExgLibPut(socketP->libraryRef, socketP);
	
	if (err == exgErrNotEnoughPower)
		ErrAlertCustom(pwrErrBeam,0,0,0);

	return err;
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION:		ExgGet
 *
 * DESCRIPTION:	Gets the data identified in dataP using the active 
 *						EXG device to the active destination.
 *
 *
 * PARAMETERS:	dataPtr pointer a exgDataType structure
 *
 * RETURNS:		0 if no error, else error code
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			gavin	5/29/97	created
 *			dje	3/16/00	loop when user changes libraries
 *
 *************************************************************/
Err ExgGet(ExgSocketType *socketP)
{
	Err err;
	
	err = PrvFindExgLib(socketP, NULL);
	if (! err)
		err = ExgLibGet(socketP->libraryRef, socketP);
	
	if (err == exgErrNotEnoughPower)
		ErrAlertCustom(pwrErrBeam,0,0,0);

	return err;
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION:		ExgRequest
 *
 * DESCRIPTION:	Request an object and have it delivered to the
 *						appropriate application. Compare with ExgGet.
 *
 * PARAMETERS:	socketP	- a socket identifying the object being requested
 *
 * RETURNS:		0 if no error, else error code
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			dje	6/21/00	Created
 *
 *************************************************************/
Err ExgRequest(ExgSocketType *socketP)
{
	UInt32 creatorID;
	Err err;
	
	err = PrvFindExgLib(socketP, &creatorID);
	if (! err)
	{
		err = ExgLibRequest(socketP->libraryRef, socketP);
	
		if (err == exgErrNotEnoughPower)
			ErrAlertCustom(pwrErrBeam,0,0,0);

	}
	else if (err == sysErrLibNotFound)
		{ // Assume an application registered for the scheme. Sublaunch it with sysAppLaunchCmdGoToURL.
		UInt32 result;
#if EMULATION_LEVEL == EMULATION_NONE
		UInt16 cardNo;
		LocalID dbID;
		DmSearchStateType searchState;
		
		err = DmGetNextDatabaseByTypeCreator(true, &searchState, sysFileTApplication,
			creatorID, true, &cardNo, &dbID);
		if (err == dmErrCantFind)
			err = sysErrLibNotFound;		// no app exists - report "lib not found"
		if (! err)
			err = SysAppLaunch(cardNo, dbID, 0, sysAppLaunchCmdGoToURL, (MemPtr)socketP->name, &result);
#else
		result = PilotMain(sysAppLaunchCmdGoToURL, (MemPtr)socketP->name, sysAppLaunchFlagSubCall);
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
#endif
		
		if (! err)
			{ // return result from PilotMain
			if (result <= 0xFFFF)
				err = result;
			else
				err = exgErrUnknown;
			}
		}
	
	return err;
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION:		ExgAccept
 *
 * DESCRIPTION:	Sends the data in dataP using the active Exg device
 *						to the active destination.
 *
 *
 * PARAMETERS:	socketP pointer a exgSocketType structure
 *
 * RETURNS:	0 if no error, else error code
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			gavin	5/29/97	created
 *			dje	3/16/00	error if libraryRef isn't filled in
 *
 *************************************************************/
Err ExgAccept(ExgSocketType *socketP)
{
	ErrFatalDisplayIf(! socketP->libraryRef, "libraryRef not specified");
	return ExgLibAccept(socketP->libraryRef, socketP);
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION:		ExgDisconnect
 *
 * DESCRIPTION:	Terminate an exchange session. This is used for any
 *						exchange session with get or put or receive. If error 
 *						parameter is non-zero, the session will be terminated
 *						and an error dialog displayed.
 *
 *
 * PARAMETERS:	socketP pointer a exgSocketType structure
 *					err - application returned error
 *
 * RETURNS:		0 if no error, else error code
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			gavin	5/29/97	created
 *			dje	3/16/00	error if libraryRef isn't filled in
 *
 *************************************************************/
Err ExgDisconnect(ExgSocketType *socketP, Err error)
{
	ErrFatalDisplayIf(! socketP->libraryRef, "libraryRef not specified");
	return ExgLibDisconnect(socketP->libraryRef, socketP, error);
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION:		ExgSend
 *
 * DESCRIPTION:	Sends data referenced by ExgDataType structure.
 *						An application should call this function after
 *						a ExgPut call.
 *
 *
 * PARAMETERS:	socketP - pointer a exgSocketType structure
 *					bufP - pointer to buffer with data to send
 *					bufLen - # of bytes to try to send
 *					errP - pointer to Error
 *
 * RETURNS: 	# of bytes sent
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			gavin	5/29/97	created
 *			dje	3/16/00	error if libraryRef isn't filled in
 *
 *************************************************************/
UInt32 ExgSend(ExgSocketType *socketP, const void *bufP, UInt32 bufLen, Err* errP)
{
	ErrFatalDisplayIf(! socketP->libraryRef, "libraryRef not specified");
	return ExgLibSend(socketP->libraryRef, socketP, bufP, bufLen, errP);
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION:		ExgReceive
 *
 * DESCRIPTION:	Receives data referenced by ExgDataType structure.
 *						An application should call this function after
 *						being notified that there is incomming data that
 *						it should receive.
 *
 *
 * PARAMETERS:	socketP - pointer a exgSocketType structure
 *					bufP - pointer to buffer to receive data
 *					bufLen - # of bytes to try to read
 *					errP - pointer to Error
 *
 * RETURNS: 	# of bytes read, zero if end of file
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			gavin	5/29/97	created
 *			dje	3/16/00	error if libraryRef isn't filled in
 *
 *************************************************************/
UInt32 ExgReceive(ExgSocketType *socketP, void * bufP, UInt32 bufLen, Err* errP)
{
	ErrFatalDisplayIf(! socketP->libraryRef, "libraryRef not specified");
	return ExgLibReceive(socketP->libraryRef,socketP,bufP,bufLen,errP);
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION:		ExgControl
 *
 * DESCRIPTION:	Performs a control operation on the exchange
 *						library identified by the socket's library
 *						reference number or URL scheme. See comment
 *						on ExgLibControl for operations.
 *
 * PARAMETERS:	<->socketP		- pointer to a socket
 *					->op				- control operation to perform
 *					<->valueP		- ptr to value for operation
 *					<->valueLenP	- ptr to size of value
 *
 * RETURNS:		0 if no error, else error code
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			dje	7/24/00	created
 *
 *************************************************************/
Err ExgControl(ExgSocketType *socketP, UInt16 op, void *valueP, UInt16 *valueLenP)
{
	Err err;
	
	err = PrvFindExgLib(socketP, NULL);
	if (! err)
		err = ExgLibControl(socketP->libraryRef, op, valueP, valueLenP);
	return err;
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION:		ExgNotifyReceiveV35
 *
 * DESCRIPTION:	Sends data to an application. This is called by an
 *						exchange library when it has received data to be
 *						sent to an app. The library provides a socket header 
 *						that contains enough information for this routine to 
 *						identify the destination app.
 *						This may launch another application in many cases.
 *						This routine must be called from the UI task because it
 *						may bring up a user dialog.
 *
 *
 * PARAMETERS:	socketP - pointer a exgSocketType structure
 *
 * RETURNS:		0 if no error, else error code
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			gavin 5/29/97	Created
 *			dje	7/21/00	Call through to new trap
 *
 *************************************************************/
Err ExgNotifyReceiveV35(ExgSocketType *socketP)
{
	Err err;
	
	err = ExgNotifyReceive(socketP, 0 /*flags*/);
	if (! err)
		err = ExgNotifyGoto(socketP, 0 /*flags*/);
	return err;
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION:		ExgNotifyReceive
 *
 * DESCRIPTION:	Sends data to an application. This is called by an
 *						exchange library when it has received data to be
 *						sent to an app. The library provides a socket header 
 *						that contains enough information for this routine to 
 *						identify the destination app.
 *						This may launch another application in many cases.
 *						This routine must be called from the UI task because it
 *						may bring up a user dialog.
 *						This is a new version of ExgNotifyReceive that adds
 *						support for unwrapped attachments and for skipping the
 *						confirmation step, doesn't do the goto launch, and
 *						returns an error if the user doesn't accept.
 *
 * PARAMETERS:	socketP - identifies the object being delivered
 *					flags	- the exgUnwrap flag indicates whether the object
 *							  has been unwrapped by the exg lib and should only
 *							  be delivered to applications that registered with
 *							  this flag; the exgNoAsk flag skips the askUser
 *							  launch and the ExgDoDialog call
 *							  exgGet flag make this call perform a get request
 *
 * RETURNS:		0 if no error, else error code
 *					exgErrUserCancel if the user cancels
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			dje	7/17/00	Created from previous version
 *
 *************************************************************/
Err ExgNotifyReceive(ExgSocketType *socketP, UInt16 flags)
{
	Err err = errNone;
	Boolean doDialog = (flags & exgNoAsk) == 0;
	Boolean doLaunch = true;
	UInt16 cardNo;
	LocalID appDBID;
	UInt32 result;
	UInt32 creator;
	UInt32 lastCreator = 0;
	UInt16 launchCode = sysAppLaunchCmdExgReceiveData;
	
	EvtResetAutoOffTimer();		// reset auto off so the user has a chance to read this
	
	// Find the application (or panel)
	err = ExgGetTargetApplication(socketP, (flags & exgUnwrap) != 0, &creator, NULL, 0);
	if (! err)
		err = PrvGetLaunchInfo(creator, &cardNo, &appDBID);
	if (err)
		return err;
	
	// Zero count field if we're delivering to a different app
	if (socketP->count)
		{
		FtrGet(exgFtrCreator, exgFtrNumLastCreator, &lastCreator);
		if (creator != lastCreator)
			socketP->count = 0;
		}
	else
		FtrSet(exgFtrCreator, exgFtrNumLastCreator, creator);
	
	socketP->target = creator;	// set socket target to match the new creator
	
	// for a Get request, don't display ask dialog and change launch code
	if (flags & exgGet)
		{
		doDialog = false;
		launchCode = sysAppLaunchCmdExgGetData;
		}
		
	
	// call app before dialog to give it a chance to override the dialog
	if (!err && doDialog)
		{
		ExgAskParamType	param;
		param.socketP = socketP;
		param.result = doDialog ? exgAskDialog : exgAskOk;	// set current default
		err = SysAppLaunch(cardNo, appDBID,
			0 /*launchFlags*/, sysAppLaunchCmdExgAskUser, (MemPtr)&param, &result);
		if (!err)
			err = (Err)result;
		if (!err)
			{
			doDialog = param.result == exgAskDialog;
			if (param.result==exgAskCancel)
				doLaunch = false;
			}
		}
	
	// now do the dialog if it is enabled	
	if (!err && doDialog)
		doLaunch = ExgDoDialog(socketP, NULL, &err);
	
	if (! err && ! doLaunch)
		err = exgErrUserCancel;
	
	// now do the data transfer if still OK 
	if (! err)
		{
		// call the app (don't launch) telling it to receive data
		err = SysAppLaunch(cardNo, appDBID, 0 /*launchFlags*/, launchCode,
									(MemPtr)socketP, &result);
		if (! err)
			err = (Err)result;
		
		// reset auto off so we do not shut down while switching apps
		// accepting data in the previous call may take a long time....
		EvtResetAutoOffTimer();
		}
	return err;
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION:		ExgNotifyGoto
 *
 * DESCRIPTION:	Does a goto launch to view the received object. This
 *						is called by an exchange library after it has delivered
 *						all the objects it received using ExgNotifyReceive.
 *						This may launch another application in many cases.
 *						This routine must be called from the UI task.
 *
 * PARAMETERS:	socketP - identifies the object that was delivered
 *					flags	- not currently used
 *
 * RETURNS:		0 if no error, else error code
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			dje	8/4/00	Created from ExgNotifyReceive
 *
 *************************************************************/
Err ExgNotifyGoto(ExgSocketType *socketP, UInt16 flags)
{
#pragma unused (flags)
	Err err = errNone;
	UInt16 cardNo;
	LocalID appDBID;
#if EMULATION_LEVEL!= EMULATION_NONE
	UInt32 result;
#endif
	
	// Goto the new entry (launch the app).
	// Do this only if app specifies a goToCreator.
	// Override this if the noGoTo flag was set by a local sender.
	if (socketP->goToCreator && ! socketP->noGoTo)
		{
		// get the app card/DB info
		err = PrvGetLaunchInfo(socketP->goToCreator, &cardNo, &appDBID);
		
		// Get the current app ID to see if the app is already running.
		//if (!err)
		//	err = SysCurAppDatabase(&curCard, &curDBID);
		
		if (! err)
			{
			// Create a goto parameter block from goto info in socketP.
			// Allocate block to system because the system will delete it later.
			GoToParamsType *cmdPBP = MemPtrNew(sizeof(GoToParamsType));
			
			if (cmdPBP)
				{
				MemPtrSetOwner(cmdPBP, 0);
				MemSet(cmdPBP, sizeof(GoToParamsType), 0);
				cmdPBP->dbCardNo = socketP->goToParams.dbCardNo;
				cmdPBP->dbID = socketP->goToParams.dbID;
				cmdPBP->recordNum = socketP->goToParams.recordNum;
				cmdPBP->matchCustom = socketP->goToParams.matchCustom;
				// Add this line when goto supports uniqueID
				//cmdPBP->uniqueID = socketP->gotoParams.uniqueID;
				
				//if (curCard == cardNo && curDBID == appDBID)	// if this app is active
#if EMULATION_LEVEL!= EMULATION_NONE
				// The simulator will just exit that app if we try to switch to itself,
				// so we will just call it directly (sub-launch).
				err = SysAppLaunch(cardNo, appDBID, 0 /*launchFlags*/, sysAppLaunchCmdGoTo,
					(MemPtr)cmdPBP, &result);
				if (! err)
					err = (Err)result;
				MemPtrFree(cmdPBP); // we must delete block if the app was not launched
#else
				// Always launch app. If it is running, this will cause it to exit and
				// run again. This assures that any modal dialogs will be exited properly.
				err = SysUIAppSwitch(cardNo, appDBID, sysAppLaunchCmdGoTo, (Char *)cmdPBP);
				// The system will delete the cmdPBP when the app terminates.
#endif
				}
			else // unable to allocate cmdPBP
				{
				ErrNonFatalDisplay("No memory for PBP");
				err = memErrNotEnoughSpace;
				}
			}
		}
	
	return err;
}


/************************************************************
 * API
 *-----------------------------------------------------------
 *
 * FUNCTION:		ExgNotifyPreview
 *
 * DESCRIPTION:	Display a preview of an object. This launches the
 *						application registered to receive the object with
 *						sysAppLaunchCmdExgPreview. If the operation is
 *						exgPreviewQuery, we first check to make sure the
 *						library supports preview.
 *
 * PARAMETERS:	infoP	- identifies the object being delivered and the
 *							  preview type, bounds, etc
 *
 * RETURNS:		0 if no error, else error code
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			dje	6/21/00	Created
 *
 *************************************************************/
Err ExgNotifyPreview(ExgPreviewInfoType *infoP)
{
	Err err = errNone;
	UInt16 cardNo;
	LocalID appDBID;
	UInt32 result;
	UInt32 creator;
	Char *description;
	Char *registryDescription = MemPtrNew(exgMaxDescriptionLength + 1);
	Boolean oldNoStatus;
	
	if (infoP->op == exgPreviewQuery)
		{
		Boolean supportsPreview;
		
		infoP->error = ExgControl(infoP->socketP, exgLibCtlGetPreview, &supportsPreview, NULL);
		if (! infoP->error && ! supportsPreview)
			{
			err = exgErrNotSupported;
			infoP->error = err;
			goto Exit;
			}
		}
	
	// Start with an empty registry description (if we have room for one at all).
	if (registryDescription)
		registryDescription[0] = chrNull;
	
	// Find the application
	err = ExgGetTargetApplication(infoP->socketP, false /*unwrap*/, &creator,
		registryDescription, exgMaxDescriptionLength + 1);
	if (! err)
		err = PrvGetLaunchInfo(creator, &cardNo, &appDBID);
	
	if (! err)
		{
		// Let the exchange library know that we're doing a preview and set the noStatus flag if we're
		// drawing a graphical or dialog preview
		infoP->socketP->preview = true;
		oldNoStatus = infoP->socketP->noStatus;
		if (infoP->op == exgPreviewDraw || infoP->op == exgPreviewDialog)
			infoP->socketP->noStatus = true;
		
		if ((infoP->op == exgPreviewShortString || infoP->op == exgPreviewLongString)
				&& infoP->string && infoP->size)
			{
			infoP->string[0] = chrNull;	// start with an empty string so we can tell if preview is supported
			}
		
		// Send it the preview launch code
		err = SysAppLaunch(cardNo, appDBID, 0 /*launchFlags*/, sysAppLaunchCmdExgPreview,
			(MemPtr)infoP, &result);
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
		
		// Restore the noStatus and preview flags
		infoP->socketP->noStatus = oldNoStatus;
		infoP->socketP->preview = false;
		
		// Return result from PilotMain
		if (! err)
			{
			if (result <= 0xFFFF)
				err = result;
			else
				err = exgErrUnknown;
			}
		
		// Return result from info struct
		if (! err)
			err = infoP->error;
		}
	
	// A series of fallbacks that produce short string descriptions
	if ((infoP->op == exgPreviewShortString || infoP->op == exgPreviewLongString)
			&& infoP->string && infoP->size && (err || ! infoP->string[0]))
		{
		if (infoP->socketP->description && infoP->socketP->description[0])
			description = infoP->socketP->description;
		else if (infoP->socketP->name && infoP->socketP->name[0] != '.')
			description = infoP->socketP->name;		// not just an extension
		else if (registryDescription && registryDescription[0])
			description = registryDescription;
		else if (infoP->socketP->type && infoP->socketP->type[0])
			description = infoP->socketP->type;
		else if (infoP->socketP->name && infoP->socketP->name[0])
			description = infoP->socketP->name;
		else
			goto Exit;
		
		StrNCopy(infoP->string, description, infoP->size - 1);
		infoP->string[infoP->size - 1] = chrNull;		// terminate if clipped
		err = errNone;
		}
	
Exit:
	if (registryDescription)
		MemPtrFree(registryDescription);
	return err;
}


// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS


/***********************************************************************
 *
 * FUNCTION:    PrvGetAppName
 *
 * DESCRIPTION:	Given a cardNo and dbId for an app (or panel?), get its name.  
 *						Try for the resource name first - if that fails, then use the 
 *						name stored with the database.
 *						DOLATER: A system trap/routine should be added for this sort of thing, since
 *									it is done in the launcher, here and potentially may be needed by others.
 *
 * PARAMETERS:	cardNo - card that the app is on.
 *					appDBID - database ID of the app.
 *					appName - string buffer that will receive the name. (This buffer must be
 *								 large enough to hold the max possible app name (dmDBNameLength);
 *
 * RETURNED:    Error if unable to find the app.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			gavin	11/9/99	Initial version
 *
 ***********************************************************************/
static Err PrvGetAppName(UInt16 cardNo, LocalID appDBID, Char * appName)
{
	Err err = errNone;
	
#if EMULATION_LEVEL!= EMULATION_NONE
	// The only app in the simulator is the current app, and it does not exist as a database.
	StrCopy(appName, "the current application");
#else

	DmOpenRef appDB;
	MemHandle strH;
	Boolean gotName = false;
	
	*appName = 0;
		
	// First try to get the app name stored as a tAIN resource
	appDB = DmOpenDatabase(cardNo, appDBID, dmModeReadOnly);
	if (appDB != NULL)
		{
		strH = DmGet1Resource(ainRsc, ainID);
		if (strH != NULL)
			{
			gotName = true;
			StrCopy(appName, MemHandleLock(strH));
			MemHandleUnlock(strH);
			DmReleaseResource(strH);
			}
		DmCloseDatabase(appDB);
		}
		
	// Use the database name if no resource name was found
	if (!gotName)
		err = DmDatabaseInfo(cardNo,appDBID,appName,0,0,0,0,0,0,0,0,0,0);
#endif
	
	return err;
}


/***********************************************************************
 *
 * FUNCTION:		PrvExchangeDialogHandleEvent
 *
 * DESCRIPTION:	This routine is the event handler for the "Accept Beam
 *						Dialog Box".
 *
 * PARAMETERS:  eventP - a pointer to an EventType structure
 *
 * RETURNED:	true if the event has handle and should not be passed
 *					to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bhall	9/7/99	Initial Revision
 *			gavin	11/9/99	Incorporated into echange manager and added arrow key stuff
 *
 ***********************************************************************/
static Boolean PrvExchangeDialogHandleEvent(EventType *eventP)
{
	Boolean handled = false;
	FormType *formP;
	
	if (eventP->eType == ctlSelectEvent) 
		{
		switch (eventP->data.ctlSelect.controlID) 
			{
			case ExgAskCategoryTrigger:
				{
				ExgDialogInfoType * infoP;
				Char * categoryName;

				// Get the Exchange Info record
				formP = FrmGetActiveForm();
				infoP = (ExgDialogInfoType *)FrmGetGadgetData(formP, FrmGetObjectIndex(formP, ExgAskDataGadget));
				categoryName = (Char *)FrmGetGadgetData(formP, FrmGetObjectIndex(formP, ExgAskCategoryNameGadget));
				if (infoP)
					// Bring up the Category Select dialog
					(void) CategorySelect(infoP->db, formP,
						ExgAskCategoryTrigger, ExgAskCategoryList,
						false, &infoP->categoryIndex, categoryName,
						1, categoryDefaultEditCategoryString);
				}
				
				handled = true;
				break;
			}
		}
	else if ((eventP->eType == keyDownEvent)
				&&	(!TxtCharIsHardKey(	eventP->data.keyDown.modifiers,
												eventP->data.keyDown.chr))
				&&	(EvtKeydownIsVirtual(eventP))
				&&	(	(eventP->data.keyDown.chr == vchrPageUp)
					||	(eventP->data.keyDown.chr == vchrPageDown)))
		{
		formP = FrmGetActiveForm();
		
		// up key is yes, down key is no....
		// simulate a click on the approprate button so that doDialog can handle this    
		if (eventP->data.keyDown.chr == vchrPageUp)
			CtlHitControl(FrmGetObjectPtr(formP, FrmGetObjectIndex(formP, ExgAskOKButton)));
		else if (eventP->data.keyDown.chr == pageDownChr)
			CtlHitControl(FrmGetObjectPtr(formP, FrmGetObjectIndex(formP, ExgAskCancelButton)));
		} 
	else if (eventP->eType == frmUpdateEvent)
		{
		// enable/disable the field so that it will redraw.
		FieldType *fldP;
		formP = FrmGetActiveForm();
		fldP = FrmGetObjectPtr(formP, FrmGetObjectIndex(formP, ExgAskMessageField));
		FldSetUsable(fldP, true);
		FrmDrawForm (formP);
		FldSetUsable(fldP, false);
		handled = true;
		}

	return handled;
}


/***********************************************************************
 *
 * FUNCTION:    	ExgDoDialog
 *
 * DESCRIPTION:	This displays the dialog that asks the user if they want to 
 *						receive data. It will generate the description of the item 
 *						received and the name of the app that will receive it. It can
 *						also display a category picker if the optional infoP structure is
 *						supplied with a database reference. This can be called from an
 *						application when handling the sysAppLaunchCmdExgAskUser launch code.
 *
 * PARAMETERS:	socketP - a pointer to an exgSocketType structure
 *					infoP - NULL or a pointer to an ExgDialogInfoType struct
 *							  if the db value must reference an open database with categories
 *							  the category index will return the index of the category chosen
 *					errP - retuns any error encountered if non-zero
 *
 * RETURNED:	true if data should be accepted, false if not
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			gavin	11/9/99	Initial Revision
 *
 ***********************************************************************/

#define shortPreviewHeightDelta	47		// shrink dialog by this amount when using short string preview
#define noCategoryHeightDelta		20		// shring dialog by this amount when there's no category pop-up

Boolean ExgDoDialog(ExgSocketType *socketP, ExgDialogInfoType *infoP, Err *errP)
{
	Char title[exgTitleBufferSize];		
	UInt16 bufferSize = exgTitleBufferSize;
	Boolean doLaunch = false;
	FormType *formP;
	FormActiveStateType curFrmState;
	FieldType *askFieldP, *previewFieldP;
	RectangleType previewBounds;
	ExgPreviewInfoType info;
	UInt16 type;
	Err err = errNone;
	UInt16 cardNo;
	LocalID appDBID;
	Char appName[dmDBNameLength];
	MemHandle askH, askHeadingH, unnamedH, msgH;
	Char *askStr, *askHeadingStr, *unnamedStr, *msgP;
	Char categoryName[dmCategoryLength];
	
	// Command Bar should always be cleared before opening a dialog
	MenuEraseStatus(0);
	
	// Get the application name.
	err = PrvGetLaunchInfo(socketP->target, &cardNo, &appDBID);
	if (err)
		goto Exit;
	err = PrvGetAppName(cardNo, appDBID, appName);
	if (err)
		goto Exit;
	
	// Get a title from the library
	title[0] = 0;	// default to title in dialog
	err = ExgLibControl(socketP->libraryRef, exgLibCtlGetTitle, title, &bufferSize);
	//ErrNonFatalDisplayIf(StrLen(title) >= exgTitleBufferSize, "Title too long");
	if (err)
		goto Exit;
	
	// See what kinds of preview the application supports
	MemSet(&info, sizeof(info), 0);
	info.socketP = socketP;
	info.op = exgPreviewQuery;
	err = ExgNotifyPreview(&info);
	if (err)
		type = exgPreviewShortString;
	else
		{
		if (info.types & exgPreviewDraw)
			type = exgPreviewDraw;
		else if (info.types & exgPreviewLongString)
			type = exgPreviewLongString;
		else
			type = exgPreviewShortString;
		// We can't use exgPreviewDialog because we want Yes and No buttons, category picker, etc.
		}
	
	// Generate string preview if appropriate
	if (type == exgPreviewShortString || type == exgPreviewLongString)
		{
		MemSet(&info, sizeof(info), 0);
		info.socketP = socketP;
		info.op = type;
		if (type == exgPreviewShortString)
			info.size = maxShortPreviewLen;
		else
			info.size = maxLongPreviewLen;
		info.string = MemPtrNew(info.size);
		if (! info.string)
			{
			err = exgMemError;
			goto Exit;
			}
		err = ExgNotifyPreview(&info);
		}
	
	// Get the form
	formP = FrmInitForm(ExgAskForm);
	
	// Make the form smaller if we're doing a short string preview (and smaller still if
	// we don't have a category pop up)
	if (type == exgPreviewShortString)
		{
		Coord delta = shortPreviewHeightDelta;
		Coord x, y;
		
		if (infoP && infoP->db)
			{
			FrmGetObjectPosition(formP, FrmGetObjectIndex(formP, ExgAskCategoryLabel), &x, &y);
			y -= delta;
			FrmSetObjectPosition(formP, FrmGetObjectIndex(formP, ExgAskCategoryLabel), x, y);
			
			FrmGetObjectPosition(formP, FrmGetObjectIndex(formP, ExgAskCategoryTrigger), &x, &y);
			y -= delta;
			FrmSetObjectPosition(formP, FrmGetObjectIndex(formP, ExgAskCategoryTrigger), x, y);
			
			FrmGetObjectPosition(formP, FrmGetObjectIndex(formP, ExgAskCategoryList), &x, &y);
			y -= delta;
			FrmSetObjectPosition(formP, FrmGetObjectIndex(formP, ExgAskCategoryList), x, y);
			}
		else
			delta += noCategoryHeightDelta;
		
		FrmGetObjectPosition(formP, FrmGetObjectIndex(formP, ExgAskOKButton), &x, &y);
		y -= delta;
		FrmSetObjectPosition(formP, FrmGetObjectIndex(formP, ExgAskOKButton), x, y);
		
		FrmGetObjectPosition(formP, FrmGetObjectIndex(formP, ExgAskCancelButton), &x, &y);
		y -= delta;
		FrmSetObjectPosition(formP, FrmGetObjectIndex(formP, ExgAskCancelButton), x, y);
		
		formP->window.windowBounds.topLeft.y += delta;
		formP->window.windowBounds.extent.y -= delta;
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
		}
	
	// Make it active
	FrmSaveActiveState(&curFrmState);				// save active form/window state
	FrmSetActiveForm(formP);      
	
	// Set title from name of transport
	if (*title)		// default to title in dialog
		FrmSetTitle(formP, title);
	
	// Get some resources.
	askH = DmGetResource(strRsc, ExchangeQuestionString);
	askStr = MemHandleLock(askH);
	askHeadingH = DmGetResource(strRsc, ExchangeHeadingString);
	askHeadingStr = MemHandleLock(askHeadingH);
	unnamedH = DmGetResource(strRsc, ExchangeUnnamedString);
	unnamedStr = MemHandleLock(unnamedH);
	
	// Set the heading field string, including the short description if appropriate
	askFieldP = FrmGetObjectPtr(formP, FrmGetObjectIndex(formP, ExgAskMessageField));
	if (type == exgPreviewShortString)
		{
		msgH = MemHandleNew(StrLen(askStr) + StrLen(info.string) + StrLen(appName) + 1);
		if (msgH)
			{
			msgP = MemHandleLock(msgH);
			StrPrintF(msgP, askStr, info.string, appName);
			}
		}
	else
		{
		msgH = MemHandleNew(StrLen(askHeadingStr) + StrLen(appName) + 1);
		if (msgH)
			{
			msgP = MemHandleLock(msgH);
			StrPrintF(msgP, askHeadingStr, appName);
			}
		}
	FldSetTextHandle(askFieldP, msgH);
	
	// Set the long description field string
	if (type == exgPreviewLongString)
		{
		msgH = MemHandleNew(StrLen(info.string) + 1);
		if (msgH)
			{
			msgP = MemHandleLock(msgH);
			StrCopy(msgP, info.string);
			}
		previewFieldP = FrmGetObjectPtr(formP, FrmGetObjectIndex(formP, ExgAskPreviewField));
		FldSetTextHandle(previewFieldP, msgH);
		}
	else
		FrmHideObject(formP, FrmGetObjectIndex(formP, ExgAskPreviewField));
	
	// Get category picker
	if (infoP && infoP->db)
		{
		Char *categoryNameP;
		ControlType * ctlP;
		RectangleType listBound;
		// Set the label of the category trigger to unfiled and list selection to same.
		infoP->categoryIndex = dmUnfiledCategory;
		
		ctlP = (ControlType *)FrmGetObjectPtr(formP, FrmGetObjectIndex(formP, ExgAskCategoryTrigger));
		categoryNameP = (Char *)CtlGetLabel(ctlP);
		CategoryGetName(infoP->db, dmUnfiledCategory, categoryNameP);
		FrmGetObjectBounds(formP, FrmGetObjectIndex(formP, ExgAskCategoryList), &listBound);
		CategoryTruncateName(categoryNameP, listBound.extent.x);
		CtlSetLabel(ctlP, categoryNameP);
		if (type == exgPreviewDraw)
			FrmGetObjectBounds(formP, FrmGetObjectIndex(formP, ExgAskPreviewWithCategoryGadget), &previewBounds);
		}
	else // hide category selector
		{
		FrmHideObject(formP, FrmGetObjectIndex(formP, ExgAskCategoryTrigger));
		FrmHideObject(formP, FrmGetObjectIndex(formP, ExgAskCategoryLabel));
		if (type == exgPreviewDraw)
			FrmGetObjectBounds(formP, FrmGetObjectIndex(formP, ExgAskPreviewWithoutCategoryGadget), &previewBounds);
		}
	FrmSetGadgetData(formP, FrmGetObjectIndex(formP, ExgAskDataGadget), infoP);
	FrmSetGadgetData(formP, FrmGetObjectIndex(formP, ExgAskCategoryNameGadget), categoryName);
	
	// Set the event handler
	FrmSetEventHandler(formP, PrvExchangeDialogHandleEvent);

	// Default action must be to cancel
	formP->defaultButton = ExgAskCancelButton;
	
	// Draw the form 
	FrmDrawForm(formP);
	
	// Draw a graphical preview
	if (type == exgPreviewDraw)
		{
		MemSet(&info, sizeof(info), 0);
		info.socketP = socketP;
		info.op = exgPreviewDraw;
		MemMove(&info.bounds, &previewBounds, sizeof(previewBounds));
		err = ExgNotifyPreview(&info);
			// This shouldn't return an error because the app said it could do graphical preview. If it does,
			// we live with an ugly "do you wanna accept" dialog and return an error code.
		}
	
	// Disable the fields so we cannot select the text. Must do this after drawing because
	// unusable fields won't draw.
	FldSetUsable(askFieldP, false);
	if (type == exgPreviewLongString)
		FldSetUsable(previewFieldP, false);
	
#if 0
	// Don't play sound when delivering the object; play it when the IR connection shuts down.
	
	// Notify the user that they have something with a sound
	//SndPlaySystemSound(sndStartUp);				// higher tone
		//dje - Commented out because the second sounds was interrupting the first one anyway.
	SndPlaySystemSound(sndConfirmation);		// lower
#endif
	
	// Run the dialog. FrmDoDialog may be called with a form that is active and drawn.
	doLaunch = (FrmDoDialog(formP) == ExgAskOKButton);
	
	// Clean up
	FrmDeleteForm(formP);	// releases fields' handles
	FrmRestoreActiveState(&curFrmState);				// restore active form/window state
	if (info.string)
		MemPtrFree(info.string);
	MemHandleUnlock(askH);
	MemHandleUnlock(askHeadingH);
	MemHandleUnlock(unnamedH);
	DmReleaseResource(askH);
	DmReleaseResource(askHeadingH);
	DmReleaseResource(unnamedH);
	
Exit:
	if (*errP)
		*errP = err;
	return doLaunch;
}


/***********************************************************************
 *
 * FUNCTION:		PrvCompareLibraries
 *
 * DESCRIPTION:	This compares libraries for sorting alphabetically
 *
 * PARAMETERS:	libraryP1, libraryP2 -> libraries being compared
 *					other unused args
 *
 * RETURNED:	negative for libraryP1 < libraryP2
 *					zero for     libraryP1 = libraryP2
 *					positive for libraryP1 > libraryP2
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			dje	1/21/00	Initial Revision
 *
 ***********************************************************************/
static Int16 PrvCompareLibraries(void *libraryP1, void *libraryP2, Int32 other)
{
#pragma unused (other)
	return StrCaselessCompare((*(ExgLibInfoType *)libraryP1).name, (*(ExgLibInfoType *)libraryP2).name);
}


/***********************************************************************
 *
 * FUNCTION:		PrvExchangeChooseDialogHandleEvent
 *
 * DESCRIPTION:	This routine is the event handler for the dialog where
 *						the user chooses a transport by tapping on its row.
 *
 * PARAMETERS:	eventP  - a pointer to an EventType structure
 *
 * RETURNED:	true if the event has handled and should not be passed
 *					to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			dje	5/22/00	Initial Revision
 *
 ***********************************************************************/
static Boolean PrvExchangeChooseDialogHandleEvent(EventType *eventP)
{
	FormType *formP;
	ListType *listP;
	Boolean handled = false;
	
	switch (eventP->eType)
		{
		case keyDownEvent:
		 	if (EvtKeydownIsVirtual(eventP) &&
				(eventP->data.keyDown.chr == vchrPageUp || eventP->data.keyDown.chr == vchrPageDown))
				{
				formP = FrmGetActiveForm();
				listP = FrmGetObjectPtr(formP, FrmGetObjectIndex(formP, ExchangeChooseList));
				LstScrollList(listP,
					(eventP->data.keyDown.chr == vchrPageUp ? winUp: winDown),
					LstGetVisibleItems(listP) - 1);
				// don't set handled = true so system will produce click sound
				}
			break;
		}
	
	return handled;
}


/***********************************************************************
 *
 * FUNCTION:		PrvExchangeChooseDrawListProc
 *
 * DESCRIPTION:	This routine is installed as the custom draw procedure
 *						for the list items.
 *
 * PARAMETERS:	itemNum		- item number of the item to draw
 *					boundsP		- bound to the draw region
 *					itemsText	- used to store the list of items
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			dje	11/7/00	Initial revision
 *
 ***********************************************************************/
static void PrvExchangeChooseDrawListProc
	(Int16 itemNum, RectangleType *boundsP, Char **itemsText)
{
	ExgLibInfoType *libs = (ExgLibInfoType *)itemsText;
	
	WinDrawTruncChars(libs[itemNum].name, StrLen(libs[itemNum].name),
		boundsP->topLeft.x, boundsP->topLeft.y, boundsP->extent.x);
}


/************************************************************
 *
 * FUNCTION:		PrvDoChooseDialog
 *
 * DESCRIPTION:	Put up a dialog with a line for each exchange
 *						library that registered for any of the specified
 *						URL schemes and a Cancel button.
 *
 * PARAMETERS:	socketP	<-> the socket
 *
 * RETURNS:		0 if no error, else error code
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			dje	5/22/00	Created
 *			dje	6/23/00	Parse URL schemes
 *			dje	11/7/00	Use list & OK button instead of table & scrollbar
 *
 *************************************************************/
static Err PrvDoChooseDialog(ExgSocketType *socketP)
{
	ExgLibInfoType *libs = NULL;
	FormType *formP;
	FormActiveStateType currentFormState;
	Err err = errNone;
	ListType *listP;
	UInt16 index;
	UInt32 numLibs;
	UInt32 *creatorIDs = NULL;
	UInt32 defaultCreatorID = 0;
	Char *names = NULL;
	Char *schemes = NULL;
	Char *firstSchemeEnd;
	Char *firstScheme = MemPtrNew(exgMaxTypeLength + 1);
	Char *startP;
	ExgSearchStateType state;
	Boolean newSearch;
	UInt32 creatorID;
	Boolean isDefault;
	Char *type = MemPtrNew(exgMaxTypeLength + 1);
	Char *description = MemPtrNew(exgMaxDescriptionLength + 1);
	Char *title = MemPtrNew(exgMaxDescriptionLength + 1);
	
	if (!firstScheme || !type || !description || !title)
		{
		err = exgMemError;
		goto FreeAndReturn;
		}
	
	// Copy out URL schemes
	schemes = PrvCopyURLSchemes(socketP);
	if (! schemes)
		{
		err = exgMemError;
		goto FreeAndReturn;
		}
	
	// Get a list of all the exchange libraries registered for the URL schemes
	err = ExgGetRegisteredApplications(&creatorIDs, &numLibs, &names, NULL, exgRegSchemeID, schemes);
	if (err)
		goto FreeAndReturn;
	
	// Get the first (or only) scheme
	firstSchemeEnd = StrChr(schemes, exgSeparatorChar);
	if (firstSchemeEnd)
		{
		StrNCopy(firstScheme, schemes, firstSchemeEnd - schemes);
		firstScheme[firstSchemeEnd - schemes] = chrNull;
		}
	else
		StrCopy(firstScheme, schemes);
	
	// Get the default for the first scheme
	ExgGetDefaultApplication(&defaultCreatorID, exgRegSchemeID, firstScheme);
		// Ignore errors because there might not be any libraries registered for the first scheme.
	
	// Use any description for the first (or only) scheme as the form title
	title[0] = chrNull;
	newSearch = true;
	while (! PrvGetNextType(&state, newSearch, exgRegSchemeID, &creatorID, &isDefault, type, description))
		{
		if (! title[0] && StrCaselessCompare(type, firstScheme) == 0 && description[0])
			StrCopy(title, description);
		newSearch = false;
		}
	
	// Copy the results into an array of ExgLibInfoTypes
	libs = MemPtrNew(numLibs * sizeof(ExgLibInfoType));
	if (! libs)
		{
		err = exgMemError;
		goto FreeAndReturn;
		}
	startP = names;
	for (index = 0; index < numLibs; index++)
		{
		libs[index].creatorID = creatorIDs[index];
		//ErrNonFatalDisplayIf(StrLen(startP) > dmDBNameLength, "Name too long");
		StrCopy(libs[index].name, startP);
		startP += StrLen(startP) + 1;
		}
	
	// Skip dialog if there's only one exg lib
	if (numLibs == 1)
		{
		err = PrvGetRefNum(libs[0].creatorID, &socketP->libraryRef);
		goto FreeAndReturn;
		}
	
	// Sort the libraries alphabetically
	SysInsertionSort(libs, numLibs, sizeof(ExgLibInfoType), &PrvCompareLibraries, 0 /*other*/);
	
	// Command Bar should always be cleared before opening a dialog
	MenuEraseStatus(0);
	
	// Get the form
	formP = FrmInitForm(ExchangeChooseForm);
	
	// Make it active
	FrmSaveActiveState(&currentFormState);
	FrmSetActiveForm(formP);
	
	// Set title
	if (title[0])		// leave title alone if no description is available
		FrmSetTitle(formP, title);
	
	// Set the event handler
	FrmSetEventHandler(formP, PrvExchangeChooseDialogHandleEvent);
	
	// Set the list choices and the display callback
	listP = FrmGetObjectPtr(formP, FrmGetObjectIndex(formP, ExchangeChooseList));
	LstSetListChoices(listP, (Char **)libs, numLibs);
	LstSetDrawFunction(listP, PrvExchangeChooseDrawListProc);
	
	// Select the default library, if any
	if (defaultCreatorID)
		for (index = 0; index < numLibs; index++)
			if (libs[index].creatorID == defaultCreatorID)
				LstSetSelection(listP, index);
	
	// Default action must be to cancel
	formP->defaultButton = ExchangeChooseCancelButton;
	
	// Draw the form 
	FrmDrawForm(formP);
	
	if (FrmDoDialog(formP) == ExchangeChooseCancelButton)
		err = exgErrUserCancel;
	else
		{
		index = LstGetSelection(listP);
		if (index == noListSelection)
			err = exgErrUserCancel;
		else
			{
			err = PrvGetRefNum(libs[index].creatorID, &socketP->libraryRef);
			if (! err)
				ExgSetDefaultApplication(libs[index].creatorID, exgRegSchemeID, firstScheme);
					// Ignore errors because selection might not be registered for the first scheme.
			}
		}
	
	// Delete the form and restore the active form
	FrmDeleteForm(formP);
	FrmRestoreActiveState(&currentFormState);
	
FreeAndReturn:
	if (libs)
		MemPtrFree(libs);
	if (creatorIDs)
		MemPtrFree(creatorIDs);
	if (names)
		MemPtrFree(names);
	if (schemes)
		MemPtrFree(schemes);
	if (firstScheme)
		MemPtrFree(firstScheme);
	if (type)
		MemPtrFree(type);
	if (description)
		MemPtrFree(description);
	if (title)
		MemPtrFree(title);
	
	return err;
}
