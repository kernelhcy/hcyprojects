/******************************************************************************
 *
 * Copyright (c) 1994-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: Find.c
 *
 * Release: 
 *
 * Description:
 *	  This file contains the text search routines.
 *
 * History:
 *		April 16, 1994	Created by Art Lamb
 *
 *****************************************************************************/

#define NON_PORTABLE 

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_FINDPARAMS
#define ALLOW_ACCESS_TO_INTERNALS_OF_FIELDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_TABLES

#include <PalmTypes.h>
#include <PrivateRecords.h>
#include <SystemPublic.h>

#include "Globals.h"
#include "SystemPrv.h"
#include "TextPrv.h"		// For TxtPrepFindString

#include "Find.h"
#include "Form.h"
#include "Menu.h"
#include "UIResourcesPrv.h"


/***********************************************************************
 *
 * FUNCTION:    FindStrInStr
 *
 * DESCRIPTION: This routine performs a case-blind partial word search
 *              for a string in another string.
 *
 *              This routine assumes that the string to find has already
 *				been prepared via a call to TxtPrepFindString.
 *
 * PARAMETERS:	strToSeach - string to search
 *              strToFind  - string to find
 *              posP       - pointer to the offset, in the search string.
 *                           of the match
 *
 * RETURNED:	true if the string was found.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art		04/17/95	Initial Revision
 *			trev	08/08/97	made non modified passed variables constant
 *			kwk		03/15/99	Moved guts into TextMgr.c's TxtFindString.
 *
 ***********************************************************************/

Boolean
FindStrInStr(Char const * strToSearch, Char const * strToFind, UInt16 * posP)
{
	UInt32 matchPos;
	UInt16 matchLen;
	
	// Note that we lose the matchLen value, which is why we want apps to
	// call TxtFindString directly.
	if (TxtFindString(strToSearch, strToFind, &matchPos, &matchLen))
		{
		*posP = matchPos;
		return(true);
		}
	else
		{
		return(false);
		}
}


/***********************************************************************
 *
 * FUNCTION: 	 FindLoadFindStr
 *
 * DESCRIPTION: This routine loads the find string from any currently
 *              selected text or from the state file.
 *
 *              Each time the find dialog is confirmed, the find 
 *              setting is saved to the state file.
 *
 * PARAMETERS:  curFrm - current form (before the find dialog)
 *					 findStrLen - to be set to the length of the string (or 0)
 *
 * RETURNED:	 MemHandle of find string
 *					 findStrLen - to be set to the length of the string (or 0)
 *
 * HISTORY:
 *		01/31/95	art	Created by Art Lamb.
 *		03/22/96	roger	Use selected text if available.
 *		12/07/98	kwk	Limit auto-copy length to 15 bytes.
 *								Use TxtTruncate to restrict length.
 *		01/25/99	kwk	Expand max length to 16 bytes, but
 *								limit it to 8 characters for Japanese.
 *		07/13/99	kwk	Use TxtPrepFindString to set up saved text length.
 *		08/28/00	kwk	Use new FrmGetActiveField routine.
 *
 ***********************************************************************/
 static MemHandle FindLoadFindStr (FormType * curFrm, UInt16 * findStrLen)
{
	MemHandle		findStrH = 0;
	Char *			findStrP;
	DmOpenRef		dbP;
	MemHandle		resourceH;
	FieldPtr		fld = 0;
	UInt16			startPosition;
	UInt16			endPosition;

	ErrNonFatalDisplayIf(!findStrLen, "null arg");

	*findStrLen = 0;
	
	// <DemoChange> If this is a demo device, display alert
	if (GSysMiscFlags & sysMiscFlagGrfDisable)
		{
		// Allocate a MemHandle to hold a find string resource.
		findStrH = MemHandleNew (maxFindStrLen+1);
		findStrP = MemHandleLock (findStrH);
		StrCopy(findStrP, "call");					// DOLATER - this should come from a resource for localization
		*findStrLen = StrLen( findStrP );
		MemHandleUnlock(findStrH);
		return findStrH;
		}

	fld = FrmGetActiveField(curFrm);
	
	if (fld != NULL)
		{
		FldGetSelection (fld, &startPosition, &endPosition);
		if (startPosition != endPosition)
			{
			Char * fldText = FldGetTextPtr(fld);
			
			// We have a selection.  Use the selected text for
			// the find dialog's text.
			
			// Do not use more text than find can handle.
			if (endPosition > startPosition + maxFindStrLen)
				{
				endPosition = TxtGetTruncationOffset(fldText, startPosition + maxFindStrLen);
				}
			
			// Now also potentially trim back the amount of text we take, based on how much
			// we can save in the find string buffer. The problem is that for Japanese, a
			// sequence of single-byte ascii gets converted into double-byte equivalents,
			// and thus we can really only search for 8 single-byte chars, thus that's how
			// much we want to save off.
			endPosition = startPosition + TxtPrepFindString(fldText + startPosition,
				endPosition - startPosition, NULL, maxFindStrLen + sizeOf7BitChar('\0'));
		
			*findStrLen = endPosition - startPosition;
			findStrH = MemHandleNew (*findStrLen + 1);
			findStrP = MemHandleLock (findStrH);
	
			// Copy the find string from the resource.
			MemMove(findStrP, fldText + startPosition, *findStrLen);
			findStrP[*findStrLen] = '\0';
			
			MemHandleUnlock (findStrH);
			}
		}


	// If no text was selected use text from the last find.	
	if (*findStrLen == 0)
		{
		dbP = PrefOpenPreferenceDB (true);
	
		if (! dbP) return 0;
		
		resourceH = DmGetResource (sysResTSysPref, sysResIDSysPrefFindStr);
		if (resourceH)
			{	
			// Allocate a MemHandle to hold a find string resource.
			findStrH = MemHandleNew (maxFindStrLen+1);
			findStrP = MemHandleLock (findStrH);
			MemSet (findStrP, maxFindStrLen+1, 0);
	
			// Copy the find string from the resource.
			StrCopy (findStrP, MemHandleLock(resourceH));
			*findStrLen = StrLen (findStrP);
			
			MemHandleUnlock (findStrH);
			MemHandleUnlock(resourceH);
			DmReleaseResource (resourceH);
			}
	
		DmCloseDatabase (dbP);
		}


	return findStrH;
}


/***********************************************************************
 *
 * FUNCTION: 	 FindSaveFindStr
 *
 * DESCRIPTION: Save the find string from the state file.
 *
 *              Each time the find dialog is confirmed, the find 
 *              sting is saved to the state file.
 *
 * PARAMETERS:  MemHandle of find string
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art		01/31/95	Initial Revision
 *			roger	11/09/98	Fixed read past end of findStrH
 *			kwk		03/16/99	Use TxtGetTruncationOffset when truncating.
 *
 ***********************************************************************/
 static void FindSaveFindStr (MemHandle findStrH)
{
	DmOpenRef	dbP;
	MemHandle	resourceH;
	void *		resourceP;
	void *		findStrP;
	Int16		stringLength;

	dbP = PrefOpenPreferenceDB (true);
	if (! dbP) return;
	
	resourceH = DmGetResource (sysResTSysPref, sysResIDSysPrefFindStr);

	// Create or resize the find string resource.
	if (!resourceH)
		resourceH = DmNewResource (dbP, sysResTSysPref, sysResIDSysPrefFindStr, maxFindStrLen+1);
	else
		resourceH = DmResizeResource (resourceH, maxFindStrLen+1);

	// Initialize find string resource.
	if (resourceH)
		{
		resourceP = MemHandleLock(resourceH);
		findStrP = MemHandleLock(findStrH);
		stringLength = StrLen(findStrP) + 1;
		
		// This should never happen, since the string MemHandle comes to
		// us from the text field, which is limited to maxFindStrLen
		// bytes, but we'll be extra safe.
		if (stringLength > maxFindStrLen + sizeOf7BitChar(nullChr))
			{
			ErrNonFatalDisplay("Find str too long.");
			stringLength = TxtGetTruncationOffset(findStrP, maxFindStrLen)
				+ sizeOf7BitChar(nullChr);
			}
		DmWrite(resourceP, 0, findStrP, stringLength);
		
		MemPtrUnlock(resourceP);
		MemPtrUnlock(findStrP);
		DmReleaseResource (resourceH);
		}

	DmCloseDatabase (dbP);
}



/***********************************************************************
 *
 * FUNCTION:    FindInitParams
 *
 * DESCRIPTION: This routine allocates and initializes the parameter
 *              block that is passed to to each application.
 *
 * PARAMETERS:	 nothing
 *
 * RETURNED:	 Handle of the find parameter block (FindParamsType structure)
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art		04/18/95	Initial Revision
 *			kwk		12/02/98	If resulting search str in empty, return null.
 *
 ***********************************************************************/
static MemHandle FindInitParams (Char * strToFind)
{
	UInt16						numBytes;
	MemHandle					tempH;
	FindParamsPtr 				params;
	Err							err;
	

	// Allocate and zero out the parameter block.
	numBytes = sizeof (FindParamsType);
	tempH = MemHandleNew (numBytes);
	params = (FindParamsPtr) MemHandleLock(tempH);
	MemSet (params, numBytes, 0);
	params->newSearch = true;

	// Save the search string as typed and in all lower case.  We'll
	// use the lower case string to speed up the search.
	StrCopy (params->strAsTyped, strToFind);
	
	// Convert the string into a 'normalized' format suitable for performing
	// case-insensitive matching.
	TxtPrepFindString(strToFind, StrLen(strToFind), params->strToFind, sizeof(params->strToFind));

	// If we wind up with nothing to search for (e.g. the user entered just a
	// chou-on character, which is ignored and thus stripped), then we don't
	// want to do the search.
	if (params->strToFind[0] == 0) {
		MemHandleFree(tempH);
		return(NULL);
	}

	// Determime if secert records should be shown.
	if (PrefGetPreference (prefShowPrivateRecords) == showPrivateRecords)
		params->dbAccesMode = dmModeReadOnly | dmModeShowSecret;
	else
		params->dbAccesMode = dmModeReadOnly;
		
	// Get the info on the current app
	err = SysCurAppDatabase(&params->callerAppCardNo, &params->callerAppDbID);
	ErrFatalDisplayIf(err, "Error getting app info");

	// Make it the app we start searching
	params->appDbID = params->callerAppDbID;
	params->appCardNo = params->callerAppCardNo;	
	
	MemHandleUnlock(tempH);
	return (tempH);
}


/***********************************************************************
 *
 * FUNCTION:    FindGetLineBounds
 *
 * DESCRIPTION: This routine returns the bound of the next available 
 *              line, for displaying a match, in the find results
 *              dialog. 
 *
 * PARAMETERS:	 findParams  MemHandle of FindParamsPtr
 *              r           pointer to a structure to hold the bound 
 *                          of the next results line.
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/19/95	Initial Revision
 *			roger	3/22/96	Leave a border column unwritten in on both sides.
 *			trev	08/08/97	made non modified passed variables constant
 *
 ***********************************************************************/
void FindGetLineBounds (const FindParamsType *findParams, RectanglePtr r)
{
	FormType * frm;
	TablePtr table;

	// Get the bounds of the next line in the results table.
	frm = FrmGetFormPtr (FindResultsDialog);
	table = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, FindResultsTable));
	TblGetItemBounds (table, findParams->lineNumber, 0, r);
	
	// Trim off the outside column of pixels so that nothing is drawn
	// in them so they remain unbroken.
	r->topLeft.x++;
	r->extent.x -= 2;
}


/***********************************************************************
 *
 * FUNCTION:    FindDrawHeader
 *
 * DESCRIPTION: This routine draws the header line the seperates, by
 *              database, the list of found items.
 *
 * PARAMETERS:	 findParams  MemHandle of FindParamsPtr
 *              title       description of the database (i.e. Memos)
 *
 * RETURNED:	 true if find screen is filled up. App should exit from
 *						the search if this occurs.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/19/95	Initial Revision
 *			trev	08/08/97	made non modified passed variables constant
 *
 ***********************************************************************/
Boolean FindDrawHeader (FindParamsPtr findParams, Char const * title)
{
	Int16 x, y;
	Int16 width;
	Int16 titleWidth;
	Int16 continueWidth;
	Char * continueStr;
	MemHandle continueStrH;
	FormType * frm;
	TablePtr table;
	RectangleType r;


	// If there's no room for the header, set the more flag and return.
	if (findParams->lineNumber == maxFinds) {
		findParams->more = true;			
		return (true);
		}


	frm = FrmGetFormPtr (FindResultsDialog);
	table = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, FindResultsTable));
	TblGetItemBounds (table, findParams->lineNumber, 0, &r);

	// Compute the width of the header text.
	titleWidth = FntCharsWidth (title, StrLen (title));
	if (findParams->continuation)
		{
		continueStrH = DmGetResource (strRsc, FindResultsContinueStr);
		continueStr = MemHandleLock (continueStrH);
		continueWidth = FntCharsWidth (continueStr, StrLen(continueStr));
		MemHandleUnlock (continueStrH);
		}
	else 
		continueWidth = 0;
	width = titleWidth + continueWidth - 1;
	
			
	// Draw the line to left of the title
	x = (r.extent.x - width) / 2 - 4;
	y = r.topLeft.y + (r.extent.y / 2);
	WinDrawLine (r.topLeft.x, y, x, y);
	x += 4;
	
	// Draw the header text.
	WinDrawChars (title, StrLen (title), x, r.topLeft.y);
	if (findParams->continuation)
		{
		continueStr = MemHandleLock (continueStrH);		
		WinDrawChars (continueStr, StrLen(continueStr), x+titleWidth, 
		 	r.topLeft.y);
		MemHandleUnlock (continueStrH);
		}
	x += width + 4;
	
	// Draw the line to the right of the title.
	WinDrawLine (x, y, r.topLeft.x + r.extent.x - 1, y);
	

	// Make the line non-selectable.
	TblSetRowUsable (table, findParams->lineNumber, true);
	TblSetRowSelectable (table, findParams->lineNumber, false);

	findParams->lineNumber++;
	return false;
}



/***********************************************************************
 *
 * FUNCTION:    FindSaveMatch
 *
 * DESCRIPTION: This routine saves the record and position within the record
 *              of a text search match.  This information is save so
 *              that we can later navigate to the match.
 *
 * PARAMETERS:	 params		 FindParamsPtr
 *              recordNum   record index
 *              pos         offset of the macth string from start of record
 *					 dbCardNo	 card number of database that contains the match
 *					 dbID        LocalID of database that contains the match
 *
 * RETURNED:	 true if the maximum displayable items has been exceeded.
 *
 * CALLED BY:   application code when it gets a match.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art		04/19/95	Initial Revision
 *
 ***********************************************************************/
Boolean FindSaveMatch (FindParamsPtr findParams, UInt16 recordNum, 
	UInt16 pos, UInt16 fieldNum, UInt32 appCustom, UInt16 cardNo, LocalID dbID)
{
	UInt16 row;
	FormType * frm;
	TablePtr table;
	FindMatchPtr match;
	
	// Save the last record displayed.  If the search is continued it
	// continues from this record.  Note that it doesn't resume from the 
	// last record checked.  It was decided not to burden the app with 
	// this responsibility.  Retaining control of this parameter is also
	// desired.
	findParams->recordNum = recordNum;
	
	if (findParams->lineNumber == maxFinds)
		{
		findParams->more = true;			
		return (true);
		}

	// Save the match info.
	match = &findParams->match[findParams->numMatches];
	match->appCardNo = findParams->appCardNo;
	match->appDbID = findParams->appDbID;
	
	match->dbCardNo = cardNo;
	match->dbID = dbID;
	
	match->recordNum = recordNum;
	match->matchPos = pos;
	match->matchFieldNum = fieldNum;
	match->matchCustom = appCustom;
	if (findParams->appCardNo == findParams->callerAppCardNo &&
		 findParams->appDbID == findParams->callerAppDbID)
		match->foundInCaller = true;
	else
		match->foundInCaller = false;
		
		
	
	// Enable the line in the results table that will display the match.
	frm = FrmGetFormPtr (FindResultsDialog);
	table = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, FindResultsTable));

	row = findParams->lineNumber;
	TblSetItemInt (table, row, 0, findParams->numMatches);
	TblSetItemStyle (table, row, 0, customTableItem);
	TblSetRowUsable (table, row, true);
	TblSetRowSelectable (table, row, true);

	findParams->numMatches++;

	return (false);
}


/***********************************************************************
 *
 * FUNCTION:    FindDisplayNumMatches
 *
 * DESCRIPTION: This routine displays the status line that reports the 
 *              number of matches that were found.
 *
 * PARAMETERS:	findParams	FindParamsPtr
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art		04/16/95	Initial Revision
 *			art		??/??/98	Don't use curly quotes.
 *			kwk		07/17/98	Updated for int'l.
 *
 ***********************************************************************/
static void FindDisplayNumMatches (FindParamsPtr params, RectanglePtr r)
{
	Char * str;
	MemHandle rscH;
	Char * displayStr;
	UInt16 maxLength;
	Int16 remainingWidth;
	UInt16 truncStrLen;
	Char truncStr[maxFindStrLen + 1];

	FntSetFont (boldFont);

	// Erase the previous message.
	
	WinEraseRectangle (r, 0);

	// Display the "Matches for" or "No matches" string. This string
	// is a template - in English it says "Matches for Ò^0Ó" or "No
	// matches for Ò^0Ó"
	
	if (params->numMatches == 0)
		rscH = DmGetResource (strRsc, FindResultsNoMatchesStr);
	else
		rscH = DmGetResource  (strRsc, FindResultsMatchesStr);

	str = MemHandleLock (rscH);
	
	// We need to truncate the entered string so that it fits in the
	// display window. First allocate a buffer where we can mess with
	// the template.
	
	maxLength = StrLen(str) + maxFindStrLen - 2; // less the '^0' param.
	displayStr = (Char *)MemPtrNew(maxLength + sizeOf7BitChar('\0'));
	ErrFatalDisplayIf(displayStr == NULL, "Out of memory");
	
	// Copy in the template and put nothing into the search string. From
	// this we can calculate the remaining space for the user's find string.
	
	StrCopy(displayStr, str);
	TxtReplaceStr(displayStr, maxLength, NULL, 0);
	remainingWidth = r->extent.x - FntCharsWidth(displayStr, StrLen(displayStr));
	
	// Now figure out if the string fits. If not, we'll have to truncate
	// it accordingly.
	
	StrCopy(truncStr, params->strAsTyped);
	truncStrLen = StrLen(truncStr);
	
	if (FntCharsWidth(truncStr, truncStrLen) > remainingWidth)
		{
		truncStrLen = FntWidthToOffset(	truncStr,
										truncStrLen,
										remainingWidth - FntCharWidth(chrEllipsis),
										NULL,
										NULL);
		truncStrLen += TxtSetNextChar(truncStr, truncStrLen, chrEllipsis);
		truncStr[truncStrLen] = '\0';
		}
	
	// Now rebuild the string for display.
	
	StrCopy(displayStr, str);
	TxtReplaceStr(displayStr, maxLength, truncStr, 0);
	WinDrawChars(displayStr, StrLen(displayStr), r->topLeft.x, r->topLeft.y);
	MemPtrFree((void *)displayStr);
	MemPtrUnlock (str);
}

/***********************************************************************
 *
 * FUNCTION:    FindSearch
 *
 * DESCRIPTION: This routine 
 *
 * PARAMETERS:	 findParams
 *				resumeInPlace - true to continue the search from the last line
 *								false to start new or continue at top of table
 *
 * RETURNED:	 nothing
 *
 * HISTORY:
 *		04/16/95	art	Created by Art Lamb.
 *		03/22/96	rsf	Don't select the first item
 *		08/12/98	kwk	With simulator build, set up more and continuation
 *							fields after call to app.
 *		09/07/99	kwk	Skip reporting of stripped base error.
 *
 ***********************************************************************/
static void FindSearch (FormType * frm, MemHandle findParams, Boolean resumeInPlace)
{
	Int16 x, y;
	UInt16 row;
	UInt16 numRows;
	UInt16 ctlIndex;
	UInt16 msgIndex;
	FontID curFont;
	Char * str;
	TablePtr table;
	RectangleType r;
	FindParamsPtr params;
	UInt32 result;
	Err err=0;
	

	// Display the "searching..." message.
	curFont = FntSetFont (boldFont);
	msgIndex = FrmGetObjectIndex (frm, FindResultsMsgLabel);
	FrmGetObjectPosition (frm, msgIndex, &x, &y);
	r.topLeft.x = x;
	r.topLeft.y = y;
	r.extent.y = FntLineHeight ();
	WinGetWindowExtent (&r.extent.x, NULL);
	r.extent.x -= x;
	WinEraseRectangle (&r, 0);

	str = MemHandleLock (DmGetResource (strRsc, FindResultsSearchingStr));
	WinDrawChars (str, StrLen (str), x, y);
	MemPtrUnlock (str);	

	// Re-initial the find parameters.
	params = (FindParamsPtr) MemHandleLock(findParams);
	params->more = 0;
	if (!resumeInPlace)
		{
		params->numMatches = 0;
		params->lineNumber = 0;
		}

	// Enable / disable the find more button.
	ctlIndex = FrmGetObjectIndex (frm, FindResultsMoreButton);
	FrmHideObject (frm, ctlIndex);
	
	
	if (!resumeInPlace)
		{
		// Erase the current contents of the find results table.
		table = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, FindResultsTable));
		TblUnhighlightSelection (table);
		WinEraseRectangle (&table->bounds, 0);

		TblSetColumnUsable (table, 0, true);

		// Disable all the items in the table.
		numRows = TblGetNumberOfRows (table);
		for (row = 0; row < numRows; row++)
			{
			TblSetRowUsable (table, row, false);
			TblSetRowSelectable (table, row, false);
			}
		}

	
	// If this is the first time, start by searching the currently running
	// application.
	FntSetFont (stdFont);

#if EMULATION_LEVEL != EMULATION_NONE

	// With the simulator, we only search the current app, but we still need
	// to handle the case of it returning back more matches than will fit in
	// our results form. We also want to MemHandle the case where the app bailed
	// out early during the search because the user tapped on the screen (pending
	// event).
	
	if (!params->continuation)
		params->recordNum = 0;
	
	result = PilotMain (sysAppLaunchCmdFind, (MemPtr)params, 0);
	if (result == 0)
		{
		if (params->more || EvtSysEventAvail(true)) 
			{
			params->more = true;
			params->continuation = true;
			}
		else
			{
			params->continuation = false;
			}
		}
#else

	// Search applications until we fill the screen
	while (!params->more) 
		{
		// If we're not continuing a search, reset the record number
		if (!params->continuation)
			params->recordNum = 0;
			
	
		// Search the initiating application first
		if (!params->searchedCaller) {
			err = SysAppLaunch(params->appCardNo, params->appDbID, 0,
						sysAppLaunchCmdFind, (MemPtr)params, &result);
			ErrNonFatalDisplayIf(err, "Error launching app for Find");
			if (params->more || EvtSysEventAvail(true)) 
				{
				params->more = true;
				params->continuation = true;
				}
			else
				{
				params->searchedCaller = true;
				params->continuation = false;
				}
			}
			
		// Search every other application
		else {
			UInt16				cardNo;
			LocalID				dbID;
			DmSearchStateType oldState;
			Boolean oldNewSearch;
			
			// Find the next database.
			oldState = params->searchState;
			oldNewSearch = params->newSearch;
			err = DmGetNextDatabaseByTypeCreator(params->newSearch, &params->searchState, 
				sysFileTApplication, 0, true, &cardNo, &dbID);
			params->newSearch = false;
			if (err == dmErrCantFind) break;
			
			if (!err)
				{
				// If this is the caller application, don't search it again
				if (cardNo == params->callerAppCardNo && 
								dbID == params->callerAppDbID) continue;
				
				// Save its info so that FindSaveMatch can capture it
				params->appDbID = dbID;
				params->appCardNo = cardNo;
				
				// Search it
				err = SysAppLaunch(cardNo, dbID, 0,
							sysAppLaunchCmdFind, (MemPtr)params, &result);
				}
			
			// Ignore stripped base error.
			if (err != omErrBaseRequiresOverlay)
				ErrNonFatalDisplayIf(err, "Error launching app for Find");
			
			// If there's more to search in this app, backup the search state
			//  start the search from here when we get called again
			if (params->more || EvtSysEventAvail(true)) {
				params->more = true;
				params->searchState = oldState;
				params->newSearch = oldNewSearch;
				params->continuation = true;
				}
			else 
				params->continuation = false;
			}
		}
			
			
#endif // EMULATION_LEVEL != EMULATION_NONE
	
		

	// Display the number of matches message.
	FindDisplayNumMatches (params, &r);
	
	// Select the first item found.
/*
	for (row = 0; row < numRows; row++)
		{
		if (TblRowSelectable (table, row))
			{
			TblSelectItem (table, row, 0);
			break;
			}
		}
*/

	// Enable / disable the find more button.
	if (params->more)
		FrmShowObject (frm, ctlIndex);
		
	MemPtrUnlock(params);
	FntSetFont (curFont);
}


/***********************************************************************
 *
 * FUNCTION:    FindGetGoToInfo
 *
 * DESCRIPTION: This routine returns information about the selected
 *              item the the "find results table".
 *
 * PARAMETERS:	 goToParams - info about record to go to (returned values)
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/7/95	Initial Revision
 *			roger 8/3/95	Handled no matches
 *
 ***********************************************************************/
static Boolean FindGetGoToInfo (MemHandle findParams, FindMatchPtr goToParams)
{
	Int16 row;
	Int16 column;
	Int16 count;
	FormType * frm;
	TablePtr table;
	FindParamsPtr params;

	frm = FrmGetActiveForm ();
	table = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, FindResultsTable));
	if (!TblGetSelection (table, &row, &column))
		return false;

	// Ingore the header lines.
	for (count = row ; row >= 0; row--) 
		{
		if (!TblRowSelectable (table, row))
			count--;
		}

	params = (FindParamsPtr) MemHandleLock(findParams);
	MemMove(goToParams, &params->match[count], sizeof(FindMatchType));
	MemPtrUnlock(params);
	
	return true;
}


/***********************************************************************
 *
 * FUNCTION:    FindShowResults
 *
 * DESCRIPTION: This routine display the dialog the contains the 
 *              results to a text search.
 *
 * PARAMETERS:	 findParams - text search parameter block
 *              goToParams - info about record to go to (returned values)
 *
 * RETURNED:	 true if the go to button was pressed.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art		4/16/95		Initial Revision
 *			roger	3/22/96		Changes for touching item gotos the record
 *			tlw		2/4/98		Don't restore curForm if it was NULL
 *
 ***********************************************************************/
static Boolean FindShowResults (MemHandle findParams, FindMatchPtr goToParams)
{
	FormType * frm;
	FormType * curForm;
	EventType event;
	Boolean done = false;
	Boolean goToMatch = false;
	FindParamsPtr findParamsP;
	Boolean more;
	SysAppLaunchCmdSaveDataType	cmdPB;
	UInt32 result;
	Boolean stopButtonUsable = true;
	ControlPtr buttonP;
	Boolean handled;
	

	// Before we put up this dialog and start searching apps, send a "warning" to
	// the current application to give it a chance to save data that might be
	//  in an open dialog, close editing sessions, etc.
	findParamsP = (FindParamsPtr) MemHandleLock(findParams);
	cmdPB.uiComing = true;
	SysAppLaunch(findParamsP->callerAppCardNo, findParamsP->callerAppDbID, 0,
						sysAppLaunchCmdSaveData, (MemPtr)&cmdPB, &result);
	more = findParamsP->more;
	MemPtrUnlock(findParamsP);



	curForm = FrmGetActiveForm ();

	frm = FrmInitForm (FindResultsDialog);
	FrmSetActiveForm (frm);
	FrmDrawForm (frm);

	FindSearch (frm, findParams, false);

	while (! done)
		{
		// If the stop Button is usable don't wait forever.  Return and
		// remove the button.
		EvtGetEvent (&event, stopButtonUsable ? 0 : evtWaitForever);
		handled = FrmHandleEvent (frm, &event);
		
		if (SysHandleEvent((EventType *)&event))
			continue;
		
		// If the user tapped somewhere but didn't activate a control or 
		// or select a record in the table, then resume the search. 
		if (event.eType == penDownEvent &&
			!handled &&
			more &&
			stopButtonUsable)	// don't accept penDownEvent after form displayed
			{
			FindSearch (frm, findParams, true);
			}
		
		// If a nilEvent is returned then remove the stop button.  After the 
		// stop button is hidden this loop should not wait for nilEvents.
		if (event.eType == nilEvent)
			{
			if (stopButtonUsable)
				{
				// Hide the stop button
				buttonP = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, FindResultsStopButton));
				CtlHideControl(buttonP);
				
				stopButtonUsable = false;
				
				// Show the cancel button
				buttonP = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, FindResultsCancelButton));
				CtlShowControl(buttonP);
				}
			}
			
		// If the user selects a line go to where the data is.
		if (event.eType == tblSelectEvent)
			{
			if (FindGetGoToInfo (findParams, goToParams))
				goToMatch = true;
			done = true;
			break;
			}
			
		else if (event.eType == ctlSelectEvent)
			{
			// Do nothing for the Stop button because the search has already 
			// exited.
			switch (event.data.ctlSelect.controlID)
				{
				case FindResultsCancelButton:
					done = true;
					break;

				case FindResultsMoreButton:
					// Hide the cancel button
					buttonP = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, FindResultsCancelButton));
					CtlHideControl(buttonP);
					
					// Show the stop button
					buttonP = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, FindResultsStopButton));
					CtlShowControl(buttonP);
					
					stopButtonUsable = true;
					
					FindSearch (frm, findParams, false);
					break;
				}
			}

		else if (event.eType == appStopEvent)
			{
			EvtAddEventToQueue (&event);
			break;
			}
		}

	MemHandleFree (findParams);
	FrmEraseForm (frm);
	FrmDeleteForm (frm);
	
	if (curForm)
		FrmSetActiveForm (curForm);
	
	return (goToMatch);
}



/***********************************************************************
 *
 * FUNCTION:    Find
 *
 * DESCRIPTION: This routine displays the Find Dialog and handles user
 *              interactions with the dialog.
 *              
 * PARAMETERS:	 nothing.
 *
 * RETURNED:	 nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	04/16/95	Initial Revision
 *			tlw	02/04/98	Check if active window is NULL before calling WinModal
 *								and don't use curFrm if it is NULL.
 *			kwk	12/02/98	Handle case of FindInitParams returning NULL.
 *			bob	09/23/99	Test SysUIBusy rathern than WinModal.
 *			gap	09/29/99	Added a call to MenuEraseStatus to be sure command bar is 
 *								closed before Find form is displayed.
 *			jmp	10/02/99 If we set SysUIBusy() make sure we unset it when we're
 *								done!  Fixes bugs #22514 & #22515.
 *			gap	10/22/99	Fixed cmd bar/dialog interation. No longer need MenuEraseStatus here.
 *
 ***********************************************************************/
void Find (GoToParamsPtr goToP)
{
	UInt16 buttonHit;
	UInt16 fldIndex;
	UInt16 strLength;
	FormType * curFrm;
	FormType * frm;
	FieldPtr fld;
	Char * strToFind;
	MemHandle strToFindH;
	MemHandle findParams;
	Boolean goToMatch = false;
	FindMatchType goToParams;
	Int16 searchStrLen;
	Boolean	restoreGrfDisable = false;
	
	// don't allow find dialog to come up if UI is marked busy
	if (SysUIBusy(false, false) > 0)
		{
      SndPlaySystemSound (sndError);
		return;
		}
		
/*
	// If the active window is modal, don't allow the find dialog to appear.
	activeWin = WinGetActiveWindow();
	if (activeWin && WinModal(activeWin))
		return;
*/
	
	curFrm = FrmGetActiveForm();
	if (curFrm && WinModal (FrmGetWindowHandle(curFrm)))
		return;
			
	frm = FrmInitForm (FindDialog);
	fldIndex = FrmGetObjectIndex (frm, FindStrField);
	fld = FrmGetObjectPtr (frm, fldIndex);

	// Load the search string from the state file.
	if (curFrm)
		{
		strToFindH = FindLoadFindStr (curFrm, &strLength);
		FldSetTextHandle (fld, strToFindH);
		}
	else
		strLength = 0;

	FrmSetFocus (frm, fldIndex);
	FldSetSelection (fld, 0, strLength);

	// Enable Graffiti for the life of the find dialog if this is a DEMO ROM
	if (GSysMiscFlags & sysMiscFlagGrfDisable)
		{
		GSysMiscFlags &= ( ~sysMiscFlagGrfDisable );
		restoreGrfDisable = true;
		}

	buttonHit = FrmDoDialog (frm);
	if (buttonHit == FindOKButton)
		{
		strToFindH = FldGetTextHandle (fld);
		FldSetTextHandle (fld, 0);
		FrmDeleteForm (frm);
	
		if (strToFindH)
			{
			strToFind = MemHandleLock(strToFindH);
			if (*strToFind)
				{
				findParams = FindInitParams (strToFind);
				if (findParams != NULL)
					{
					// Save search string length for the goto param block
					searchStrLen = StrLen(strToFind);
					goToMatch = FindShowResults (findParams, &goToParams);
					}
				}

			// Save the search string to the state file.
			FindSaveFindStr (MemPtrRecoverHandle(strToFind));
			
			MemPtrFree (strToFind);
			}
		}

	else
		{
		FrmDeleteForm (frm);
		}
		
	// Disable Graffiti if this is a DEMO ROM
	if (restoreGrfDisable)
		GSysMiscFlags |= sysMiscFlagGrfDisable;

	// Are we are going to navigate to a selected item. 
	if (goToMatch)
		{
		
		// If found in another app, we must allocate our own goTo paramBlock.
		if (!goToParams.foundInCaller) {
			goToP = MemPtrNew(sizeof(GoToParamsType));
			if (!goToP) return;
			MemPtrSetOwner(goToP, 0);
			}
		
		// Copy goTo info into the GoToParamsType structure
		goToP->searchStrLen = searchStrLen;
		goToP->dbCardNo = goToParams.dbCardNo;
		goToP->dbID = goToParams.dbID;
		goToP->recordNum = goToParams.recordNum;
		goToP->matchPos = goToParams.matchPos;
		goToP->matchFieldNum = goToParams.matchFieldNum;
		goToP->matchCustom = goToParams.matchCustom;


		// If the record we're navigating to is owned by the current application,
		// send the action code to the app.
		if (goToParams.foundInCaller) {
			Err		err;
			UInt32		result;
			err = SysAppLaunch(goToParams.appCardNo, goToParams.appDbID, 0,
						sysAppLaunchCmdGoTo, (MemPtr)goToP, &result);
			ErrFatalDisplayIf(err, "Error sending goTo action to app");
			return;
			}
			
			
		// Launch the new app. The UIAppShell will take care of disposing
		//  the goTo parameter block when the app finally quits.
		SysUIAppSwitch(goToParams.appCardNo, goToParams.appDbID, sysAppLaunchCmdGoTo,
									(MemPtr)goToP);
		}

	return;
}
