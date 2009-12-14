/******************************************************************************
 *
 * Copyright (c) 1996-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: PhoneLookup.c
 *
 * Release: 
 *
 * Description:
 *	  Source code to implement the PhoneNumberLookup trap, which uses
 *		the Address app to map a name to a name + phone number.
 *
 * History:
 *		July 23, 1996	Created by Art Lamb
 *		March 24, 2000 Ludovic Ferrandis: Add custom API
 *
 *****************************************************************************/

#include <PalmTypes.h>
#include <SystemPublic.h>

#include "Clipboard.h"
#include "Form.h"
#include "Field.h"
#include "PhoneLookup.h"
#include "UIResourcesPrv.h"

/***********************************************************************
 *
 * FUNCTION:    CallAddressApp
 *
 * DESCRIPTION: This routine sends the Address application a launch 
 *              code to lookup a record
 *
 * PARAMETERS:  key 				- key to search for
 *					lookupField		- Field of the record to display
 *					formatStringP	- Format of the result string
 *
 * RETURNED:    MemHandle of string return by address application.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/26/96	Initial Revision
 *			LFe	03/24/00	Add parameters: lookupField - formatStringP
 *			LFe	09/18/00	Change parameter: use AddrLookupParamsType
 *
 ***********************************************************************/
static MemHandle CallAddressApp (Char * keyP,  AddrLookupParamsType* params)
{
#if EMULATION_LEVEL == EMULATION_NONE
	Err						err;
	UInt16					cardNo;
	UInt32					result;
	LocalID					dbID;
	DmSearchStateType		searchState;
	
	// Get the card number and database id of the Address application.
	err = DmGetNextDatabaseByTypeCreator (true, &searchState, sysFileTApplication, sysFileCAddress, true, &cardNo, &dbID);
	ErrNonFatalDisplayIf(err, "Address app not found");
	
	if (err) return (0);

	MemSet(params->lookupString, addrLookupStringLength, 0);	

	if (keyP)
		StrNCopy(params->lookupString, keyP, addrLookupStringLength-1);
			
	// <RM> 1-19-98, Fixed to pass 0 for flags, instead of sysAppLaunchFlagSubCall
	//  The sysAppLaunchFlagSubCall flag is for internal use by SysAppLaunch only
	//  and should NOT be set  on entry.
	err = SysAppLaunch (cardNo, dbID, 0, sysAppLaunchCmdLookup, (MemPtr)params, &result);

	ErrNonFatalDisplayIf(err, "Error sending lookup action to app");

	if (err) return (0);
	
	return (params->resultStringH);

#else
	{
	Char * p;
	const Char * str = "#LOOKUP#";
	MemHandle h;
	
	h = MemHandleNew (StrLen(str) + 1);
	p = MemHandleLock (h);
	StrCopy (p, str);
	MemPtrUnlock (p);
	return (h);
	}

#endif
}

/***********************************************************************
 *
 * FUNCTION:    PhoneNumberLookupCustom
 *
 * DESCRIPTION: This routine called the Address Book application to
 *              lookup a phone number.
 *
 * PARAMETERS:	fldP				- field object
 *					lookupField		- Field of the record to display
 *					formatStringP	- Format of the result string
 *
 * RETURNED:    nothing
 *
 *	HISTORY:
 *		07/23/96	art	Created by Art Lamb.
 *		08/15/98	kwk	Reworked Int'l version for new results from TxtWordBounds
 *							call.
 *		05/17/99	kwk	Fixed up integration with Instant Karma.
 *		10/27/99 PPl	Fixed a Field Focus Setting problems.
 *		11/03/99 jmp	Backed out the previous change as it does NOT work in
 *							general -- i.e., FrmSetFocus() isn't orthogonal for
 *							releasing/grabbing fields in tables!
 *		03/24/00 LFe	Renamed in custom and add parameters
 *		04/07/00	kwk	Fixed logic to look for word _before_ insertion point
 *							first, then try looking right (this was accidentally
 *							changed by my mod on 08/15/98, but it never causes a
 *							difference in English).
 *		09/18/00	LFe	Change parameter: use AddrLookupParamsType
 *		10/13/00 LFe	Add parameter: useClipboard. If true, try to use clipboard
 *							to paste the result in the field=>enable undo
 *		01/10/01 FPa	Fixed bug #49288
 *
 ***********************************************************************/
void PhoneNumberLookupCustom (FieldType *fldP, AddrLookupParamsType* params, Boolean useClipboard)
{

	UInt16		keyLen;
	UInt16		end;
	UInt16		start;
	UInt16		resultLen;
	MemHandle	keyH = NULL;
	Char*			keyP = NULL;
	Char*			text;
	Char*			resultP;
	MemHandle	resultH;
	Boolean		isSelection;

	// If there is a selection then use it as the key.
	text = FldGetTextPtr (fldP);
	FldGetSelection (fldP, &start, &end);

/* This was the original lookup code - Roger wants to keep it around
	for reference.
	
	if (start == end)
		{
		UInt16 pos = FldGetInsPtPosition (fldP);
		isSelection = false;
		if (text && pos)
			{
			UInt16 * charAttrs = GetCharAttr ();

			// Find the start of the key value. Search backwards from the 
			// insertion point position for a delimiter.
			start = pos;
			end = start;
			while (start && ! 
					 (IsSpace(charAttrs, text[start-1]) || 
				 	  IsPunct(charAttrs, text[start-1])))
				start--;

			// If there are characters between the insertion point and the 
			// delimiter or start of the field if there are no deleimiter,
			// then find the end of the key.
			if (start != pos - 1)
				{
				keyLen = FldGetTextLength (fldP);
				while (end < keyLen && ! 
						 (IsSpace(charAttrs, text[end]) || 
						  IsPunct(charAttrs, text[end])))
					end++;
				}
			}
		}
*/

	// If there is no selection then we need to use TxtWordBounds to
	// come up with the word.
	
	isSelection = (start != end);
	
	if (!isSelection && (text != NULL))
		{
		UInt32 wordStart, wordEnd;
		UInt32 textLength = FldGetTextLength(fldP);
		UInt32 pos = FldGetInsPtPosition (fldP);
		UInt32 leftPos = pos;
		
		// We want to look left first, so if possible get the preceeding
		// character, so that when we call TxtWordBounds we'll get the word
		// to the left.
		// e.g. If you have the following text  (the cursor is symbolized by the | character):
		// abc def| ghi
		// Going 1 character to the left will modify the cursor position as is:
		// abc de|f ghi
		// So that's the "def" word that will be replaced.
		
		if (leftPos > 0)
			{
			leftPos -= TxtPreviousCharSize(text, pos);
			}
		
		// If we didn't wind up with a real word, it might be because
		// we're at the end of the text, or the character to the left of
		// pos is a punct/space char. For either case, see if we can
		// find a word after the insertion point.
		
		if (!TxtWordBounds(text, textLength, leftPos, &wordStart, &wordEnd))
			{
			if ((leftPos != pos) && (pos < textLength))
				{
				if (!TxtWordBounds(text, textLength, pos, &wordStart, &wordEnd))
					{
					end = start;	// Failed finding a word
					}
				else
					{
					start = wordStart;
					end = wordEnd;
					}
				}
			else
				{
				end = start;	// Failed finding a word
				}
			}
		else
			{
			start = wordStart;
			end = wordEnd;
			}
		}

	// Copy the key value for the string.
	
	if (start < end)
		{
		keyLen = end - start;
		keyH = MemHandleNew (keyLen+1);
		keyP = MemHandleLock (keyH);
		MemMove (keyP, &text[start], keyLen);
		keyP[keyLen] = 0;
		}

	resultH = CallAddressApp (keyP, params);
	
	if (keyH) MemHandleFree (keyH);

	// Replace the key string with the value returned by the Address 
	// application.
	if (resultH)
		{
		resultP = MemHandleLock (resultH);
		resultLen = StrLen(resultP);

		// I we can paste the result into the field through the clipboard so that
		// change can be undone with to "undo" command.
		if ((resultLen <= cbdMaxTextLength) && useClipboard)
			{
			ClipboardAddItem (clipboardText, resultP, resultLen);
			if (! isSelection)
				FldSetSelection (fldP, start, end);
			FldPaste (fldP);
			}
		else
			{
			if (start != end)
				FldDelete (fldP, start, end);
			FldInsert (fldP, resultP, resultLen);
			}

		MemHandleFree (resultH);
		}
}

/***********************************************************************
 *
 * FUNCTION:    PhoneNumberLookup
 *
 * DESCRIPTION: This routine called the Address Book application to
 *              lookup a phone number.
 *
 * PARAMETERS:  fldP - field object
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			LFe	03/24/00	Move all the code and comments to PhoneNumberLookupCustom
 *								Call PhoneNumberLookupCustom with previous hardcoded value.
 *			LFe	09/18/00	Change parameter: use AddrLookupParamsType
 *
 ***********************************************************************/
void PhoneNumberLookup (FieldType * fldP)
{
	AddrLookupParamsType	params;
	MemHandle				titleH;
	MemHandle				buttonH;
	MemHandle				formatH;
	
	titleH = DmGetResource (strRsc, phoneLookupTitleStrID);
	buttonH = DmGetResource (strRsc, phoneLookupAddStrID);
	formatH = DmGetResource (strRsc, phoneLookupFormatStrID);

	ErrNonFatalDisplayIf(!titleH, "null resource");
	ErrNonFatalDisplayIf(!buttonH, "null resource");
	ErrNonFatalDisplayIf(!formatH, "null resource");
	
	// parameters for the lookup
	params.title = MemHandleLock (titleH);
	params.pasteButtonText = MemHandleLock (buttonH);
	params.formatStringP = (char*) MemHandleLock (formatH);
	params.field1 = addrLookupSortField;
	params.field2 = addrLookupListPhone;
	params.field2Optional = false;
	params.userShouldInteract = true;

	PhoneNumberLookupCustom (fldP, &params, true);

	MemHandleUnlock(titleH);
	MemHandleUnlock(buttonH);
	MemHandleUnlock(formatH);
	DmReleaseResource(titleH);
	DmReleaseResource(buttonH);
	DmReleaseResource(formatH);
}
