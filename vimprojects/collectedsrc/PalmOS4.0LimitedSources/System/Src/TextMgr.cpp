/******************************************************************************
 *
 * Copyright (c) 1998-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: TextMgr.cpp
 *
 * Release: 
 *
 * Description:
 *	Source code for Text Manager routines. These routines use resource tables
 *	to support different device encodings, including simple single-byte (e.g.
 *	Latin), double-byte (e.g. Shift-JIS), and multi-byte (e.g. UTF-8).
 *
 * History:
 *	05/28/98	kwk	Created by Ken Krugler.
 *	06/23/99	kwk	Use BUILDING_PALMOSGLUE_LIB #define for glue lib build.
 *	08/05/99	kwk	Moved IntlInit into IntlMgr.c.
 *	05/15/00	kwk	Renamed from TextMgr.c to TextMgr.cpp. Started recoding to
 *					use resource tables. Removed tweaks for PalmOSGlue builds,
 *					since that library now compiles/links old TextMgr.c source.
 *	05/18/00	kwk	Include TextTablePrv.h, not TextTable.h
 *	06/02/00	CS	Include PalmLocale.h to get at charEncodingAscii, etc.
 *	11/22/00	kwk	Mark private routines as "C", to get rid of mangled names.
 *
 *****************************************************************************/

#include <PalmTypes.h>
#include <ErrorMgr.h>
#include <FeatureMgr.h>
#include <IntlMgr.h>
#include <StringMgr.h>
#include <SystemMgr.h>
#include <PalmLocale.h>	// charEncodingAscii, etc.
#include <TextMgr.h>

// #include <DebugMgr.h>		// For DbgBreak()

#include "TextPrv.h"		// for TxtPrepFindString
#include "TextPrv.rh"		// for kUnmappedCharCode
#include "IntlPrv.h"		// for IntlGlobalsType
#include "TextTablePrv.h"	// for TextTableP, TTConvertValue

#define NON_PORTABLE
#include "Globals.h"		// for GIntlMgrGlobalsP

/***********************************************************************
 * Private types
 ***********************************************************************/

// Structure used to pass around a bunch of sorting information
// (e.g. from TxtCaselessCompare to PrvDoCompare to PrvFastCompare)
typedef struct
{
	const UInt8* 	s1;
	UInt16			s1Len;
	UInt16*			s1MatchLen;
	const UInt8*	s2;
	UInt16			s2Len;
	UInt16*			s2MatchLen;
	UInt16			maxSortLevel;
} SortInfoType;

static const UInt32 kMaxUInt32 = 0xFFFFFFFF;
static const UInt16 kMaxUInt16 = 0xFFFF;

/***********************************************************************
 * Private macros
 ***********************************************************************/

#define	DoubleAttr_(tableP, byte)	*((Int8*)(tableP) + (UInt8)(byte))

/***********************************************************************
 * Private routines
 ***********************************************************************/

extern "C" {

static Int16
PrvDoCompare(const SortInfoType* iSortInfoP);

static Int16
PrvFastCompare(const SortInfoType* iSortInfoP, const UInt8* tableP);

static Boolean
PrvFastEqualCheck(const SortInfoType* iSortInfoP);

static Int16
PrvSlowCompare(const SortInfoType* iSortInfoP, TextTableP tableP);

}

/***********************************************************************
 *
 * FUNCTION: PrvDoCompare
 *
 * DESCRIPTION:	Compare the first <s1Len> bytes of <s1> with the first
 *	<s2Len> bytes of <s2>. Return the results of the comparison: < 0 if
 *	<s1> sorts before <s2>, > 0 if <s1> sorts after <s2>, and 0 if they
 *	are equal. Also return the number of bytes that matched in
 *	<s1MatchLen> and <s2MatchLen> (either one of which can be NULL if
 *	the match length is not needed).
 *
 * PARAMTERS:
 *	iSortInfoP	 ->	Ptr to sort info structure.
 *
 * RETURNS:
 *	0 if strings are equal
 *	- if s1 sorts before s2
 *	+ if s1 sorts after s2
 *
 * HISTORY:
 *	05/20/00	kwk	Created by Ken Krugler, from TxtCompare guts.
 *	05/25/00	kwk	Use new PrvStrNEqualAscii to do fast equality check.
 *	11/06/00	kwk	If either string is empty (zero-length), we still need
 *					to set up the match length result.
 *	2000-12-09	jwm	A string can still be empty even if its length field
 *					is non-zero, because we now consider that length to be
 *					merely an upper limit, not the exact length.
 *
 ***********************************************************************/
static Int16
PrvDoCompare(const SortInfoType* iSortInfoP)
{
	ErrNonFatalDisplayIf(iSortInfoP == NULL, "NULL parameter");
	ErrNonFatalDisplayIf((iSortInfoP->s1 == NULL) || (iSortInfoP->s2 == NULL),
						"NULL string parameter");
	
	Int16 s1Empty = (iSortInfoP->s1Len == 0 || iSortInfoP->s1[0] == chrNull);
	Int16 s2Empty = (iSortInfoP->s2Len == 0 || iSortInfoP->s2[0] == chrNull);
	
	if (s1Empty || s2Empty)
	{
		if (iSortInfoP->s1MatchLen != NULL)
		{
			*iSortInfoP->s1MatchLen = 0;
		}
		if (iSortInfoP->s2MatchLen != NULL)
		{
			*iSortInfoP->s2MatchLen = 0;
		}
		
		return s2Empty - s1Empty;
	}
	
	// OK, so now we know that we've got two real (non-zero length) strings
	// to sort.
	Int16 result = 0;
	UInt16 sortLevel = 0;
	Boolean fastCompare = (GIntlData->intlFlags & kByteSortingFlag) != 0;
	
	while (sortLevel <= iSortInfoP->maxSortLevel)
	{
		void* sortTable = GIntlData->sortTables[sortLevel];
		if (sortTable != NULL)
		{
			// If the fast sorting flag is set, then we know that all of the sorting
			// tables are byte-indexed byte value tables, so we can use the PrvFastCompare
			// routine.
			if (fastCompare)
			{
				result = PrvFastCompare(iSortInfoP, (UInt8*)sortTable);
			}
			else
			{
				result = PrvSlowCompare(iSortInfoP, (TextTableP)sortTable);
			}
		}
		else
		{
			return(result);
		}
		
		// If we've got a real result, return it.
		if (result != 0)
		{
			return(result);
		}
		
		// Check if first sort gave us equality. Often this means that
		// the two strings are identical, so do a fast check for that
		// versus working our way through the remaining tables. Note
		// Note that the call to Fast/SlowCompare has set up the
		// match length return values for us.
		if ((sortLevel == 0)
		&& (PrvFastEqualCheck(iSortInfoP)))
		{
			return(0);
		}
		
		// Go for next level of sorting.
		sortLevel += 1;
	}
	
	// We ran out of sorting tables without finding a difference,
	// so the two strings really are equal. Again, PrvFastCompare has
	// set up the match length return values.
	return(0);
} // PrvDoCompare

			
/***********************************************************************
 *
 * FUNCTION: PrvFastCompare
 *
 * DESCRIPTION:	Compare the first <s1Len> bytes of <s1> with the first
 *	<s2Len> bytes of <s2>. Return the results of the comparison: < 0 if
 *	<s1> sorts before <s2>, > 0 if <s1> sorts after <s2>, and 0 if they
 *	are equal. Also return the number of bytes that matched in
 *	<s1MatchLen> and <s2MatchLen> (either one of which can be NULL if
 *	the match length is not needed). This routine assumes that tableP
 *	is a byte-indexed table of byte values.
 *
 * PARAMTERS:
 *	iSortInfoP	 ->	Ptr to sort info structure.
 *	tableP		 -> Ptr to byte-indexed table of byte values.
 *
 * RETURNS:
 *	0 if strings are equal
 *	- if s1 sorts before s2
 *	+ if s1 sorts after s2
 *
 * HISTORY:
 *	??/??/99	kwk	Created by Ken Krugler.
 *	05/25/00	kwk	Catch case of NULL byte (string is null-limited, versus
 *					being length limited). This lets the caller pass an
 *					arbitrarily large length if they _know_ it's a C string.
 *	11/30/00	kwk	If loop terminates because of null byte, then don't
 *					set result based on string lengths (which might be bogus).
 *				kwk	Minor code mods to help the compiler optimize the loop.
 *	2000-12-09	jwm	Fixed bugs in bogus-string-length handling; slight
 *					code obfuscation to help the compiler optimize again.
 *
 ***********************************************************************/
static Int16
PrvFastCompare(const SortInfoType* sortInfoP, const UInt8* tableP)
{
	const UInt8* str1 = sortInfoP->s1;
	const UInt8* str2 = sortInfoP->s2;
	UInt16 checkLen = (sortInfoP->s1Len < sortInfoP->s2Len ? sortInfoP->s1Len : sortInfoP->s2Len);

	Int16 result;

	while (checkLen != 0)
	{
		UInt8 c1 = *str1++;
		result = tableP[c1] - tableP[*str2++];
		
		if ((result != 0) || (c1 == chrNull))
		{
			// If we find a difference or a real \0 before the lesser
			// stated length runs out, undo the ++ above and escape.
			str1--;
			str2--;
			break;
		}
		
		// Decrement here versus at top to skip a few instructions.
		checkLen--;
	}
	
	if (checkLen == 0)
	{
		// The string with the shorter stated length ran out first.
		// But before assigning +1 or -1, check that the other string
		// really is longer: i.e., that its stated length is greater and
		// that it has at least one more character before its \0, if any.
		if (sortInfoP->s1Len > sortInfoP->s2Len)
		{
			result = (*str1 != chrNull)? +1 : 0;
		}
		else if (sortInfoP->s1Len < sortInfoP->s2Len)
		{
			result = (*str2 != chrNull)? -1 : 0;
		}
		else
		{
			result = 0;
		}
	}
	
	// Invariant:  In all cases, at this point str1 & str2 point to
	// the first non-matched characters.

	if (sortInfoP->s1MatchLen != NULL)
	{
		*(sortInfoP->s1MatchLen) = str1 - sortInfoP->s1;
	}
	
	if (sortInfoP->s2MatchLen != NULL)
	{
		*(sortInfoP->s2MatchLen) = str2 - sortInfoP->s2;
	}
	
	return(result);
} // PrvFastCompare
			

/***********************************************************************
 *
 * FUNCTION: PrvFastEqualCheck
 *
 * DESCRIPTION:	Compare the first <iSortInfoP->s1Len> bytes of <iSortInfoP->s1>
 *	with the first <iSortInfoP->s1Len> bytes of <iSortInfoP->s2>. Return
 *	true if they match.
 *
 * PARAMTERS:
 *	iSortInfoP	 -> Ptr to struct containing data.
 *
 * RETURNS:
 *	True if the two strings are identical to the ends of the strings,
 *	or up to n bytes, whichever is shorter.
 *
 * HISTORY:
 *	05/25/00	kwk	Created by Ken Krugler, from old StrNCompare guts.
 *
 ***********************************************************************/
static Boolean
PrvFastEqualCheck(const SortInfoType* iSortInfoP)
{
	const UInt8* s1 = iSortInfoP->s1;
	const UInt8* s2 = iSortInfoP->s2;
	UInt16 n = iSortInfoP->s1Len;

	while (n--)
	{
		UInt8 c1 = *s1++;
		if (c1 != *s2++)
		{
			return(false);
		}
		else if (c1 == chrNull)
		{
			return(true);
		}
	}
		
	return(true);
} // PrvFastEqualCheck


/***********************************************************************
 *
 * FUNCTION: PrvSlowCompare
 *
 * DESCRIPTION:	Compare the first <s1Len> bytes of <s1> with the first
 *	<s2Len> bytes of <s2>. Return the results of the comparison: < 0 if
 *	<s1> sorts before <s2>, > 0 if <s1> sorts after <s2>, and 0 if they
 *	are equal. Also return the number of bytes that matched in
 *	<s1MatchLen> and <s2MatchLen> (either one of which can be NULL if
 *	the match length is not needed). This routine assumes that tableP
 *	is _not_ a byte-indexed table of byte values, so we have to use
 *	the slower finite state machine calls.
 *
 * PARAMTERS:
 *	iSortInfoP	 ->	Ptr to sort info structure.
 *	tableP		 -> Ptr to TextTable structure.
 *
 * RETURNS:
 *	0 if strings are equal
 *	- if s1 sorts before s2
 *	+ if s1 sorts after s2
 *
 * HISTORY:
 *	05/25/00	kwk	Created by Ken Krugler.
 *	06/01/00	kwk	Fixed bug where match offset was set to one char past
 *						the match length when strings weren't equal.
 *	06/07/00	CS	Flipped sense of TTProcessText function result (true
 *						means not done, so call again.)
 *	06/08/00	CS	Extended TTProcessArgsType for use by TTProcessToken.
 *	07/05/00	kwk	Only save the match length if we're not dumping out
 *					multiple sort values, as otherwise if the first sort
 *					values matched, the offset would move forward, then
 *					when the second sort values didn't match, the match
 *					length would be wrong.
 *				kwk	When two strings match, use the ioOffsets from the
 *					two state records to generate a result. This fixes
 *					a bug where one of the strings has two characters at
 *					the end that wind up mapping to two sort values, where
 *					the first sort value matches the single sort value of
 *					the last character in the second string, thus the
 *					loop terminates with the two strings "matching", but
 *					you still want the longer string to sort after the
 *					shorter string, and the input string lengths are
 *					set to 0xFFFF.
 *	07/14/00	kwk	Fixed case where loop terminates with result == 0,
 *					and we need to attempt to order the strings by their
 *					"lengths", which are actually the count of their
 *					sort values.
 *	08/14/00	kwk	Moved loop termination test to top (while loop instead
 *					of do loop) so that if we get called with an empty string
 *					but a max length (0xFFFF), we'll terminate correctly on
 *					that leading null byte. This fixed a bug w/StrCompare("", "")
 *					returning a non-zero result.
 *	09/20/00	kwk	Explicitly set up signed result when FSM returns different
 *					sort values, since these are unsigned and thus we can't
 *					just subtract them.
 *	12/10/00	kwk	If we're returning multiple sort values for one or more
 *					input characters, guess at how to advance through the
 *					input text so that we have a better chance of returning
 *					a meaningful match length.
 *				kwk	Explicitly handle the case of one string's input text
 *					generating more sort values for a given character (or
 *					match sequence) than the other string. Previously we
 *					might walk off the end of the string with fewer sort
 *					values, but now we stall it out at the end of the
 *					string, using sort values of zero.
 *
 ***********************************************************************/
static Int16
PrvSlowCompare(const SortInfoType* iSortInfoP, TextTableP tableP)
{
	TTProcessArgsType str1Args;
	TTProcessArgsType str2Args;
	Boolean firstTime = true;
	Boolean str1Done = true;
	Boolean str2Done = true;
	Boolean str1End;
	Boolean str2End;
	Int16 result = 0;
	
	str1Args.input.text.iTextP = reinterpret_cast<const Char*>(iSortInfoP->s1);
	str1Args.input.text.iLength = iSortInfoP->s1Len;
	str1Args.iTableP = tableP;
	str1Args.input.text.ioOffset = 0;
	
	str2Args.input.text.iTextP = reinterpret_cast<const Char*>(iSortInfoP->s2);
	str2Args.input.text.iLength = iSortInfoP->s2Len;
	str2Args.iTableP = tableP;
	str2Args.input.text.ioOffset = 0;
	
	str1End = (str1Args.input.text.iLength == 0) || (str1Args.input.text.iTextP[0] == chrNull);
	str2End = (str2Args.input.text.iLength == 0) || (str2Args.input.text.iTextP[0] == chrNull);

	// Don't mind this complicated termination test. We keep looping until
	// either something doesn't match, or we've reached the end of the string
	// (either by offset == length or hitting a null byte), but ignore the
	// end of string check if TTProcessText says we're not yet done, as there
	// could be additional sort values to compare (e.g. when one character
	// maps to multiple sort values, such as '§' => 'SS').
	while (((!str1End) || !str1Done) && ((!str2End) || !str2Done))
	{
		UInt16 savedStr1Offset;
		UInt16 savedStr2Offset;
		
		// If we're not in the middle of returning multiple sort values for
		// one match (one or more characters of input data), then we can go
		// ahead and just save it off.
		
		// We only want to record the "matched up to" offset if
		// we're not in the middle of returning multiple sort
		// values for the current character (or set of matched characters).
		// This might possibly lead to a wrong match length, if we have
		// two multi-character sequences that both map to multiple sort
		// results, and the first two sort results (which wind up corresponding
		// to the first character in each string) match, but the subsequent
		// sort values don't match. Currently no such case exists.
		if (str1Done)
		{
			savedStr1Offset = str1Args.input.text.ioOffset;
		}
		
		// Things get much more complicated here. We're getting back multiple
		// sort values from TTProcessText, one sort value at a time. We don't
		// _really_ know which sort values correspond to what offset in the
		// text, so our best guess is to just advance through the text one
		// character at a time, under the assumption that there's one sort
		// value per character. We could hit the end of the match text, though,
		// since one input character can map to multiple sort values (e.g.
		// the weird four-in-one Kanji characters). In that case we pin at
		// the end-of-match offset. This could cause a weird case where the
		// match length == the input text length, but the routine result is
		// not zero, since the second string could wind up generating a single
		// sort value that matches the _first_ of the N sort values being
		// returned for the first character in the first string. Confusing?
		// You bet. We also can't handle correctly the case of processing
		// a sequence of single-byte Katakana (e.g. 'ha' + nigori + chou-on),
		// since that could generate two sort values for the three input
		// characters, and thus we don't know how to map the input text
		// characters to the sort values.
		else
		{
			// The done flag isn't set. We know that str1Done started
			// out as true in this loop, so savedStr1Offset recorded the
			// offset of the start of the match text, and str1Args.input.text.ioOffset
			// has the offset for the end of the match (scary implicit assumption
			// about how TextTables returns multiple sort values), so advance by
			// one character until we hit the limit.
			if (savedStr1Offset < str1Args.input.text.ioOffset)
			{
				savedStr1Offset += TxtNextCharSize(str1Args.input.text.iTextP, savedStr1Offset);
			}
		}
		
		// All the same comments as above.
		if (str2Done)
		{
			savedStr2Offset = str2Args.input.text.ioOffset;
		}
		else
		{
			if (savedStr2Offset < str2Args.input.text.ioOffset)
			{
				savedStr2Offset += TxtNextCharSize(str2Args.input.text.iTextP, savedStr2Offset);
			}
		}
		
		// If we're in the middle of returning multiple sort values, then we have to
		// call TTProcessText. Otherwise, don't call the routine if we're at the
		// end of our string.
		if (!str1Done || !str1End)
		{
			str1Done = !TTProcessText(&str1Args, firstTime);
		}
		else
		{
			str1Args.oResult = 0;
		}
		
		if (!str2Done || !str2End)
		{
			str2Done = !TTProcessText(&str2Args, firstTime);
		}
		else
		{
			str2Args.oResult = 0;
		}
		
		firstTime = false;
		
		if (str1Args.oResult != str2Args.oResult)
		{
			// The FSM returns two UInt16s (we only use low-16 bits
			// of 32 bit value), so we need to do unsigned compare to
			// set up signed result.
			result = str1Args.oResult < str2Args.oResult ? -1 : 1;
			str1Args.input.text.ioOffset = savedStr1Offset;
			str2Args.input.text.ioOffset = savedStr2Offset;
			break;
		}
		
		str1End = (str1Args.input.text.ioOffset >= str1Args.input.text.iLength) || (str1Args.input.text.iTextP[str1Args.input.text.ioOffset] == chrNull);
		str2End = (str2Args.input.text.ioOffset >= str2Args.input.text.iLength) || (str2Args.input.text.iTextP[str2Args.input.text.ioOffset] == chrNull);
	}
	
	// If we matched for the min length of the two strings, then we
	// need to figure out which string is shorter and thus return
	// the real result (shorter string sorts before longer string).
	if (result == 0)
	{
		if (str1End)
		{
			if (!str2End)
			{
				// string 1 is shorter than string 2, so s1 < s2.
				result = -1;
			}
			else if (!str1Done)
			{
				// If there's still stuff remaining to be returned from
				// string 1, then we know that str2Done must be true
				// (otherwise we wouldn't have terminated), so treat
				// string 1 as being longer (having more sort values)
				// than string 2.
				result = 1;
			}
			else if (!str2Done)
			{
				// Conversely, in this case we know string 1 must have
				// no more sort values, so string 2 is longer than string 1.
				result = -1;
			}
			// The remaining case is when both strings terminated, and
			// neither had pending sort values, in which case result is
			// set up correctly as zero.
		}
		else
		{
			// str2End must be true (both can't be false), which means that
			// string 2 is shorter than string 1, thus s1 > s2.
			result = 1;
		}
	}
	
	if (iSortInfoP->s1MatchLen != NULL)
	{
		*(iSortInfoP->s1MatchLen) = str1Args.input.text.ioOffset;
	}
	
	if (iSortInfoP->s2MatchLen != NULL)
	{
		*(iSortInfoP->s2MatchLen) = str2Args.input.text.ioOffset;
	}
	
	return(result);
} // PrvSlowCompare


/***********************************************************************
 *
 * FUNCTION: TxtByteAttr
 *
 * DESCRIPTION:	Return back 8 bits of byte attribute flags. These tell
 *	whether the byte value could be a single, high, low, or middle
 *	byte of a character.
 *
 * PARAMTERS:
 *	iByte	->	Byte value.
 *
 * RETURNS:
 *	8 bits of byte attribute flags.
 *
 * HISTORY:
 *	05/15/00	kwk	Modified to use a table, if it exists.
 *
 ***********************************************************************/
UInt8 TxtByteAttr(UInt8 iByte)
{
	TextTableP tableP = GIntlData->byteAttrTable;
	
	if (tableP != NULL)
	{
		return(TTConvertValue(iByte, tableP));

	}
	else
	{
		return(byteAttrSingle);
	}
} // TxtByteAttr


/***********************************************************************
 *
 * FUNCTION: TxtCaselessCompare
 *
 * DESCRIPTION:	Compare the first <s1Len> bytes of <s1> with the first
 *	<s2Len> bytes of <s2>. Return the results of the comparison: < 0 if
 *	<s1> sorts before <s2>, > 0 if <s1> sorts after <s2>, and 0 if they
 *	are equal. Also return the number of bytes that matched in
 *	<s1MatchLen> and <s2MatchLen> (either one of which can be NULL if
 *	the match length is not needed).
 *
 *	This comparison is caseless, which means that we only care about
 *	the results of the first two sort levels. The third level is where
 *	case matters, but we ignore that here.
 *
 * PARAMTERS:
 *	s1			 ->	Ptr to first string.
 *	s1Len		 ->	Length of first string in bytes.
 *	s1MatchLen	<-	Number of bytes in s1 that matched (can be NULL)
 *	s2			 ->	Ptr to second string.
 *	s2Len		 ->	Length of second string in bytes.
 *	s2MatchLen	<-	Number of bytes in s2 that matched (can be NULL)
 *
 * RETURNS:
 *	0 if strings are equal
 *	- if s1 sorts before s2
 *	+ if s1 sorts after s2
 *
 * HISTORY:
 *	05/20/00	kwk	Use PrvDoCompare for all the real work.
 *	09/11/00	kwk	Don't rely on compiler to fill in record, since it
 *					generates extra bogus instructions...weird.
 *	10/25/00	kwk	Set max sort level to 0, not 1. On previous releases
 *					of Palm OS, Latin caseless comparison also folded
 *					together accented characters (stripped diacriticals),
 *					and Japanese "caseless" wants Hiragana & Katakana to
 *					be treated as equal if the only difference is a nigori
 *					or maru (e.g. "ha" vs. "ba"), so the appropriate sort
 *					level is 0, not 0 & 1.
 *
 ***********************************************************************/
Int16 TxtCaselessCompare(	const Char* s1,
							UInt16 s1Len,
							UInt16 * s1MatchLen,
							const Char* s2,
							UInt16 s2Len,
							UInt16 * s2MatchLen)
{
	SortInfoType sortInfo;
	sortInfo.s1 = reinterpret_cast<const UInt8*>(s1);
	sortInfo.s1Len = s1Len;
	sortInfo.s1MatchLen = s1MatchLen;
	sortInfo.s2 = reinterpret_cast<const UInt8*>(s2);
	sortInfo.s2Len = s2Len;
	sortInfo.s2MatchLen = s2MatchLen;
	
	// Only sort level 0...if the strings are the same at that
	// level then treat them as equal.
	sortInfo.maxSortLevel = 0;
	
	return(PrvDoCompare(&sortInfo));
} // TxtCaselessCompare

			
/***********************************************************************
 *
 * FUNCTION: TxtCharAttr
 *
 * DESCRIPTION:	Return back 16 bits of char attribute flags. These tell
 *	whether the char value is a digit, alphabetic, etc.
 *
 * PARAMTERS:
 *	iChar	->	Character value.
 *
 * RETURNS:
 *	16 bits of character attribute flags.
 *
 * HISTORY:
 *	05/15/00	kwk	Modified to use a table, and call TxtCharIsValid.
 *	11/22/00	kwk	Check kFastCharAttrFlag, and do direct index if set.
 *					Since nothing really bad happens with bogus character
 *					codes, only fix up sign extension on debug ROMs, thus
 *					making us faster on release ROMs.
 *	12/11/00	CS	Check kStageCharAttrFlag, and walk the stage index map
 *					table if set.
 *
 ***********************************************************************/
static UInt16 asm PrvCharAttr(TTDetailsStageIndexMapType* stageDetailsP:__A0,
	WChar iChar:__D0);
UInt16 TxtCharAttr(WChar iChar)
{
	ErrNonFatalDisplayIf(GIntlData->charAttrTable == NULL, "No char attribute table");
	
	// If it looks like a byte value was sign-extended by the compiler,
	// complain on debug ROMs, and then fix up the value & continue.
#if (ERROR_CHECK_LEVEL == ERROR_CHECK_FULL)
	if (iChar >= 0xFF80)
	{
		ErrNonFatalDisplay("Sign-extended char passed to TxtCharAttr");
		iChar &= 0x00FF;
	}
#endif

	// On debug ROMs, make sure the character is valid (e.g. not a
	// virtual character).
	ErrNonFatalDisplayIf(!TxtCharIsValid(iChar), "Invalid char passed to TxtCharAttr");
	
	if ((GIntlData->intlFlags & kByteCharAttrFlag) != 0)
	{
		return(*((UInt16*)GIntlData->charAttrTable + iChar));
	}
	else if ((GIntlData->intlFlags & kStageCharAttrFlag) != 0)
	{
		register
		TTDetailsStageIndexMapType* stageDetailsP = (TTDetailsStageIndexMapType*)GIntlData->charAttrTable;
		UInt16 subTableOffset;
		UInt8 resultIndex;
		
		subTableOffset = stageDetailsP->subTableOffsets[iChar >> 8];
		resultIndex = ((UInt8*)stageDetailsP)[subTableOffset + (iChar & 0xFF)];
		return(((UInt16*)stageDetailsP->dataBytes)[resultIndex]);
	}
	else
	{
		return(TTConvertValue(iChar, (TextTableP)GIntlData->charAttrTable));
	}
} // TxtCharAttr


/***********************************************************************
 *
 * FUNCTION: TxtCharBounds
 *
 * DESCRIPTION:	Given an arbitrary offset into some text, which might
 *	not be on a character boundary, return the bounds of the character
 *	which either contains or starts with <iOffset>. Also return the
 *	actual character given by the bounds.
 *
 * PARAMTERS:
 *	iTextP		 ->	Ptr to text.
 *	iOffset		 ->	Byte offset in text.
 *	oCharStart	<-	Byte offset to start of character.
 *	oCharEnd	<-	Byte offset to end of character.
 *
 * RETURNS:
 *	Character containing <iOffset>. If <iOffset> is between characters,
 *	then the character immediately following the offset is returned.
 *
 * HISTORY:
 *	05/30/00	kwk	Modified to use a table.
 *	06/08/00	CS	Extended TTProcessArgsType for use by TTProcessToken.
 *
 ***********************************************************************/
WChar TxtCharBounds(const Char* iTextP, UInt32 iOffset, UInt32* oCharStart, UInt32* oCharEnd)
{
	ErrNonFatalDisplayIf(iTextP == NULL, "NULL string parameter");
	ErrNonFatalDisplayIf((oCharStart == NULL) || (oCharEnd == NULL), "NULL result ptr");
	
	TextTableP tableP = GIntlData->charBoundsSizeTable;
	if (tableP == NULL)
	{
		*oCharStart = iOffset;
		*oCharEnd = iOffset + 1;
		return(*((UInt8 *)iTextP + iOffset));
	}
	else
	{
		TTProcessArgsType fsmArgs;
		fsmArgs.input.text.iTextP = iTextP;
		fsmArgs.input.text.iLength = kMaxUInt32;
		fsmArgs.input.text.ioOffset = iOffset;
		fsmArgs.iTableP = tableP;
		
		TTProcessText(&fsmArgs, true);
		*oCharStart = iOffset - fsmArgs.oResult;
		
		WChar theChar;
		UInt16 charSize = TxtGetNextChar(iTextP, *oCharStart, &theChar);
		ErrNonFatalDisplayIf(charSize <= fsmArgs.oResult, "Invalid char size");
		*oCharEnd = *oCharStart + charSize;
		return(theChar);
	}
} // TxtCharBounds


/***********************************************************************
 *
 * FUNCTION: TxtCharEncoding
 *
 * DESCRIPTION:	Return the minimum (lowest) encoding required for <iChar>.
 *	If we don't know about the character, return an unknown encoding.
 *
 * PARAMTERS:
 *	iChar	->	Character value.
 *
 * RETURNS:
 *	Character encoding.
 *
 * HISTORY:
 *	05/30/00	kwk	Modified to use a table, and call TxtCharIsValid.
 *
 ***********************************************************************/
CharEncodingType TxtCharEncoding(WChar iChar)
{
	// If it looks like a byte value was sign-extended by the compiler,
	// complain on debug ROMs, and then fix up the value & continue.
	if (iChar >= 0xFF80)
	{
		ErrNonFatalDisplay("Sign-extended char passed to TxtCharEncoding");
		iChar &= 0x00FF;
	}
	
	// On debug ROMs, make sure the character is valid (e.g. not a
	// virtual character).
	ErrNonFatalDisplayIf(!TxtCharIsValid(iChar), "Invalid char passed to TxtCharEncoding");
	
	ErrNonFatalDisplayIf(GIntlData->charEncodingTable == NULL, "Missing char encoding table");
	return(TTConvertValue(iChar, GIntlData->charEncodingTable));
} // TxtCharEncoding


/***********************************************************************
 *
 * FUNCTION: TxtCharSize
 *
 * DESCRIPTION:	Return back the size (in bytes) of the character iChar.
 *	This represents how many bytes would be required to store the character
 *	in a string.
 *
 * PARAMTERS:
 *	iChar	->	Character value.
 *
 * RETURNS:
 *	Size of character in a string.
 *
 * HISTORY:
 *	05/17/00	kwk	Modified to use a table, and call TxtCharIsValid.
 *
 ***********************************************************************/
UInt16 TxtCharSize(WChar iChar)
{
	// If it looks like a byte value was sign-extended by the compiler,
	// complain on debug ROMs, and then fix up the value & continue.
	if (iChar >= 0xFF80)
	{
		ErrNonFatalDisplay("Sign-extended char passed to TxtCharSize");
		iChar &= 0x00FF;
	}
	
	// On debug ROMs, make sure the character is valid (e.g. not a
	// virtual character).
	ErrNonFatalDisplayIf(!TxtCharIsValid(iChar), "Invalid char passed to TxtCharSize");
	
	// Normally we'd call TxtSetNextChar to determine the size of the
	// character, but if there is no table for doing a set, then we
	// know it's always single-byte.
	TextTableP tableP = GIntlData->nextCharSetTable;
	if (tableP == NULL)
	{
		return(1);
	}
	else
	{
		Char buffer[maxCharBytes];
		return(TxtSetNextChar(buffer, 0, iChar));
	}
} // TxtCharSize
	

/***********************************************************************
 * TxtCharWidth
 *
 * Return the width (in pixels) of the character <iChar>.
 * WARNING!!! This routine has been deprecated. Callers should use the
 * FntWCharWidth routine instead, or FntGlueWCharWidth.
 ***********************************************************************/
Int16 TxtCharWidth(WChar iChar)
{
	return(FntWCharWidth(iChar));
} // TxtCharWidth


/***********************************************************************
 *
 * FUNCTION: TxtCharXAttr
 *
 * DESCRIPTION:	Return back 16 bits of extended character attribute flags,
 *	if the table exists.
 *
 * PARAMTERS:
 *	iChar	->	Character value.
 *
 * RETURNS:
 *	16 bits of extended character attribute flags.
 *
 * HISTORY:
 *	05/15/00	kwk	Modified to use a table, and call TxtCharIsValid.
 *
 ***********************************************************************/
UInt16 TxtCharXAttr(WChar iChar)
{
	TextTableP tableP = GIntlData->charXAttrTable;
	if (tableP == NULL)
	{
		return(0);
	}
	
	// If it looks like a byte value was sign-extended by the compiler,
	// complain on debug ROMs, and then fix up the value & continue.
	if (iChar >= 0xFF80)
	{
		ErrNonFatalDisplay("Sign-extended char passed to TxtCharXAttr");
		iChar &= 0x00FF;
	}
	
	// On debug ROMs, make sure the character is valid (e.g. not a
	// virtual character).
	ErrNonFatalDisplayIf(!TxtCharIsValid(iChar), "Invalid char passed to TxtCharXAttr");
	return(TTConvertValue(iChar, tableP));
} // TxtCharXAttr
	

/***********************************************************************
 *
 * FUNCTION: TxtCompare
 *
 * DESCRIPTION:	Compare the first <s1Len> bytes of <s1> with the first
 *	<s2Len> bytes of <s2>. Return the results of the comparison: < 0 if
 *	<s1> sorts before <s2>, > 0 if <s1> sorts after <s2>, and 0 if they
 *	are equal. Also return the number of bytes that matched in
 *	<s1MatchLen> and <s2MatchLen> (either one of which can be NULL if
 *	the match length is not needed).
 *
 * PARAMTERS:
 *	s1			 ->	Ptr to first string.
 *	s1Len		 ->	Length of first string in bytes.
 *	s1MatchLen	<-	Number of bytes in s1 that matched (can be NULL)
 *	s2			 ->	Ptr to second string.
 *	s2Len		 ->	Length of second string in bytes.
 *	s2MatchLen	<-	Number of bytes in s2 that matched (can be NULL)
 *
 * RETURNS:
 *	0 if strings are equal
 *	- if s1 sorts before s2
 *	+ if s1 sorts after s2
 *
 * HISTORY:
 *	05/19/00	kwk	Modified to use a table, for the fast sorting case.
 *	05/20/00	kwk	Use PrvDoCompare for all the real work.
 *	09/11/00	kwk	Don't rely on compiler to fill in record, since it
 *					generates extra bogus instructions...weird.
 *	11/30/00	kwk	Check for common Latin sort case, and do immediate
 *					comparison here versus calling PrvDoCompare.
 *	12/10/00	kwk	Clarified comments on the fast sort termination logic,
 *					and fixed bug with calculating result if two strings
 *					matched up to the min length, and one or both of the
 *					strings had a length > 0x7FFF.
 *
 ***********************************************************************/
Int16 TxtCompare(const Char* s1, UInt16 s1Len, UInt16* s1MatchLen,
				const Char* s2, UInt16 s2Len, UInt16* s2MatchLen)
{
	// The most common case for Latin comparison is that this routine
	// gets called with two non-null strings, and the caller doesn't
	// care about the match length results. In that case, avoid a lot
	// of overhead and handle the comparison directly here. If the
	// result is non-zero, then we can immediately return it. If the
	// result is zero, then it could be because the two strings are
	// weakly equal, so we have to do the slower full comparison.
	if (((GIntlData->intlFlags & kByteSortingFlag) != 0)
	 && (s1Len > 0)
	 && (s2Len > 0)
	 && (s1MatchLen == NULL)
	 && (s2MatchLen == NULL))
	{
		const UInt8* tableP = (const UInt8*)GIntlData->sortTables[0];
		UInt16 checkLen = (s1Len < s2Len ? s1Len : s2Len);
		const UInt8* str1 = (const UInt8*)s1;
		const UInt8* str2 = (const UInt8*)s2;
		UInt8 c1;
		while (checkLen != 0)
		{
			c1 = *str1++;
			Int16 result = tableP[c1] - tableP[*str2++];
			
			if (result != 0)
			{
				return(result);
			}
			
			// Catch case of null byte, so that we terminate even if both
			// strings match up to the null, and the specified text length
			// is greater than the string length. Note that in this case
			// we have to continue with the full comparison, because the
			// two strings might be weakly equal at level 0, but not
			// identical. We could skip doing this check again by adding
			// a 'start sort level' value to the SortInfoType structure,
			// but this isn't a very common case.
			if (c1 == chrNull)
			{
				break;
			}
			
			checkLen--;
		}
		
		// If we matched for the min length of the two strings, _and_ we
		// didn't terminate because we hit a null at the string end, and
		// the strings aren't the same length (otherwise we have to do
		// comparisons at higher sort orders) then we need to figure out
		// which string is shorter and return that as the sort result
		// (shorter string sorts before longer string).
		if ((c1 != chrNull) && (s1Len != s2Len))
		{
			// Not that this would ever happen, but handle the case of
			// either of the lengths being > 0x7FFF, and thus we can't
			// do a direct subtraction to generate a signed result.
			if (s1Len < s2Len)
			{
				// Ouch. We can only return str1 < str2 if str2 isn't
				// terminated by a null byte, as otherwise the two strings
				// are equal at sort level 0. The issue is that if
				// str2 is null-terminated at the min length, then its
				// real length is equal to str1's length, and they match.
				if (*str2 != chrNull)
				{
					return(-1);
				}
			}
			else
			{
				// The inverse case applies here, since if str1's next
				// byte is a null, its real length is equal to str2's
				// length, and the strings match at sort level 0.
				if (*str1 != chrNull)
				{
					return(1);
				}
			}
		}
	}
	
	SortInfoType sortInfo;
	sortInfo.s1 = reinterpret_cast<const UInt8*>(s1);
	sortInfo.s1Len = s1Len;
	sortInfo.s1MatchLen = s1MatchLen;
	sortInfo.s2 = reinterpret_cast<const UInt8*>(s2);
	sortInfo.s2Len = s2Len;
	sortInfo.s2MatchLen = s2MatchLen;
	sortInfo.maxSortLevel = kIntlMaxSortLevels - 1;
	
	return(PrvDoCompare(&sortInfo));
} // TxtCompare

			
/***********************************************************************
 *
 * FUNCTION: TxtConvertEncodingV35
 *
 * DESCRIPTION:
 *	This is the original API that Sony & Steve Minns implemented for
 *	the patch to Palm OS 3.5. Since Steve did a good job, and also
 *	patched IntlGetRoutineAddress so that it returns back the address
 *	of the TxtConvertEncoding routine, we have to leave this API around
 *	versus re-using the selector.
 *
 * PARAMTERS:
 *	srcTextP		 ->	Ptr to source text.
 *	ioSrcBytes		<->	Length of source text in bytes on entry, and number
 *						of bytes successfully processed on exit.
 *	srcEncoding		 ->	Character encoding for source text.
 *	dstTextP		 ->	Ptr to destination buffer, or NULL.
 *	ioDstBytes		 ->	Size of dest buffer in bytes on entry, and number
 *						of bytes of resulting converted text on exit.
 *	dstEncoding		 ->	Character encoding for destination text.
 *
 * RETURNS:
 *	errNone if all text was successfully converted
 *	txtErrUnknownEncoding if source or dest encodings are unknown or can't
 *		be handled.
 *	txtErrConvertUnderflow if the end of the source text buffer contains
 *		a partial character.
 *	txtErrConvertOverflow if the destination buffer isn't large enough
 *		for the resulting converted text.
 *
 * HISTORY:
 *	07/27/00	kwk	Added to maintain backwards compatibility.
 *	08/19/00	kwk	Pass NULL for TxtConvertStateType to TxtConvertEncoding.
 *
 ***********************************************************************/
Err TxtConvertEncodingV35(	const Char* srcTextP,
							UInt16* ioSrcBytes,
							CharEncodingType srcEncoding,
							Char* dstTextP,
							UInt16* ioDstBytes,
							CharEncodingType dstEncoding)
{
	return(TxtConvertEncoding(	true, NULL,
								srcTextP, ioSrcBytes, srcEncoding,
								dstTextP, ioDstBytes, dstEncoding,
								NULL, 0));
} // TxtConvertEncodingV35


/***********************************************************************
 *
 * FUNCTION: TxtConvertEncoding
 *
 * DESCRIPTION:	
 *	Convert <*ioSrcBytes> of text from <srcTextP> between the <srcEncoding>
 *	and <dstEncoding> character encodings. If <dstTextP> is not NULL, write
 *	the resulting bytes to the buffer, and always return the number of
 *	resulting bytes in <*ioDstBytes>. Update <*srcBytes> with the number of
 *	bytes from the beginning of <*srcTextP> that were successfully converted.
 *	When the routine is called with <srcTextP> pointing to the beginning of
 *	a string or text buffer, <newConversion> should be true; if the text is
 *	processed in multiple chunks, either because errors occurred or due to
 *	source/destination buffer size constraints, then subsequent calls to
 *	this routine should pass false for <newConversion>. The TxtConvertStateType
 *	record maintains state information so that if the source or destination
 *	character encodings have state or modes (e.g. JIS), processing a single
 *	sequence of text with multiple calls will work correctly.
 *
 *	When an error occurs due to an unconvertable character, the behavior of
 *	the routine will depend on the <substitutionStr> parameter. If it is NULL,
 *	then <*ioSrcBytes> will be set to the offset of the unconvertable character,
 *	<ioDstBytes> will be set to the number of successfully converted resulting
 *	bytes, and <dstTextP>, in not NULL, will contain conversion results up to
 *	the point of the error. The routine will return an appropriate error code,
 *	and it is up to the caller to either terminate conversion or skip over the
 *	unconvertable character and continue the conversion process (passing false
 *	for the <newConversion> parameter in subsequent calls to TxtConvertEncoding).
 *	If <substitutionStr> is not NULL, then this string is written to the
 *	destination buffer when an unconvertable character is encountered in the
 *	source text, and the source character is skipped. Processing continues, though
 *	the error code will still be returned when the routine terminates. Note that
 *	if a more serious error occurs during processing (e.g. buffer overflow) then
 *	that error will be returned even if there was an earlier unconvertable character.
 *	Note that the substitution string must use the destination character encoding.
 *
 *	We do not handle surrogates in any of the Unicode encodings. Also note
 *	that UCS-2 is implicitly big-endian, and we don't support a byte-order-mark.
 *
 * PARAMTERS:
 *	newConversion	 -> True if srcTextP points to the start of a stream
 *						of text (beginning of string or text buffer).
 *	ioStateP		<->	Ptr to state record used to maintain state across
 *						multiple calls to routine, if a single stream of
 *						text requires multiple calls to process.
 *	srcTextP		 ->	Ptr to source text.
 *	ioSrcBytes		<->	Length of source text in bytes on entry, and number
 *						of bytes successfully processed on exit.
 *	srcEncoding		 ->	Character encoding for source text.
 *	dstTextP		 ->	Ptr to destination buffer, or NULL.
 *	ioDstBytes		 ->	Size of dest buffer in bytes on entry, and number
 *						of bytes of resulting converted text on exit.
 *	dstEncoding		 ->	Character encoding for destination text.
 *	substitutionStr	 ->	Ptr to string to use in dest buffer for unmappable
 *						source characters, or null.
 *	substitutionLen	 -> Length in bytes of substitutionStr.
 *
 * RETURNS:
 *	errNone if all text was successfully converted
 *	txtErrUnknownEncoding if source or dest encodings are unknown or can't
 *		be handled.
 *	txtErrConvertUnderflow if the end of the source text buffer contains
 *		a partial character.
 *	txtErrConvertOverflow if the destination buffer isn't large enough
 *		for the resulting converted text.
 *
 * HISTORY:
 *	05/30/00	kwk	Created by Ken Krugler.
 *	06/08/00	CS	Extended TTProcessArgsType for use by TTProcessToken.
 *	07/23/00	kwk	If dstTextP is NULL, and dstEncoding is UCS2, then
 *					we weren't incrementing ioDstBytes previously.
 *				kwk	Added support for maintaining state across calls.
 *				kwk	Added support for a substitution string parameter.
 *				kwk	Added support for UTF16LE and UTF16BE encodings,
 *					though note that we DO NOT support surrogates.
 *					This effectively means the UTF16 encodings == UCS2,
 *					but with an explicit byte order.
 *				kwk	If source encoding == dest encoding, do fast exit.
 *	08/19/00	kwk	If we get passed null for ioStateP, don't complain
 *					unless newConversion is false, and don't try to copy
 *					to/from the state record.
 *	08/20/00	kwk	Fixed bug where *ioDstBytes wasn't getting set up if
 *					source encoding == dest encoding.
 *
 ***********************************************************************/
Err TxtConvertEncoding(	Boolean newConversion,
						TxtConvertStateType* ioStateP,
						const Char* srcTextP,
						UInt16* ioSrcBytes,
						CharEncodingType srcEncoding,
						Char* dstTextP,
						UInt16* ioDstBytes,
						CharEncodingType dstEncoding,
						const Char* substitutionStr,
						UInt16 substitutionLen)
{
	ErrNonFatalDisplayIf(srcTextP == NULL, "Null source string");
	ErrNonFatalDisplayIf(!newConversion && (ioStateP == NULL), "Null state pointer");
	
	// Verify that the source and destination encodings are ones that we
	// support. Currently they either have to be the device encoding, or
	// one of the Unicode variations that we support.
	UInt32 deviceEncoding = charEncodingUnknown;
	void* indexToUnicodeTableP = NULL;
	void* unicodeToDeviceTableP = NULL;
	TTProcessArgsType fsmDeviceToIndexArgs;
	
	if ((srcEncoding == charEncodingUCS2)
	 || (srcEncoding == charEncodingUTF8)
	 || (srcEncoding == charEncodingUTF16LE)
	 || (srcEncoding == charEncodingUTF16BE))
	{
	}
	
	// If we can figure out the device encoding, and it matches the incoming
	// text encoding, then load the optional device to index mapping table
	// (not needed for simple one-byte character encodings), and the required
	// index to UCS-2 mapping table.
	else if ((FtrGet(sysFtrCreator, sysFtrNumEncoding, &deviceEncoding) == errNone)
		  && (deviceEncoding == srcEncoding))
	{
		fsmDeviceToIndexArgs.iTableP = GIntlData->deviceToIndexTable;
		if (fsmDeviceToIndexArgs.iTableP != NULL)
		{
			fsmDeviceToIndexArgs.input.text.iTextP = srcTextP;
			fsmDeviceToIndexArgs.input.text.iLength = *ioSrcBytes;
		}
		
		indexToUnicodeTableP = GIntlData->indexToUnicodeTable;
		ErrNonFatalDisplayIf(indexToUnicodeTableP == NULL, "Missing index to Unicode table");
	}
	else
	{
		return(txtErrUnknownEncoding);
	}
	
	// Now figure out what type of destination encoding we're using.
	if ((dstEncoding == charEncodingUCS2)
	 || (dstEncoding == charEncodingUTF8)
	 || (dstEncoding == charEncodingUTF16LE)
	 || (dstEncoding == charEncodingUTF16BE))
	{
	}
	else if (((deviceEncoding != charEncodingUnknown)
		   || (FtrGet(sysFtrCreator, sysFtrNumEncoding, &deviceEncoding) == errNone))
		  && (deviceEncoding == dstEncoding))
	{
		unicodeToDeviceTableP = GIntlData->unicodeToDeviceTable;
		ErrNonFatalDisplayIf(unicodeToDeviceTableP == NULL, "Missing Unicode to device table");
	}
	else
	{
		return(txtErrUnknownEncoding);
	}
	
	// If source encoding == dest encoding, just copy over the data.
	Err result = errNone;
	if (srcEncoding == dstEncoding)
	{
		if (dstTextP != NULL)
		{
			if (*ioSrcBytes > *ioDstBytes)
			{
				result = txtErrConvertOverflow;
				*ioSrcBytes = *ioDstBytes;
			}
			
			MemMove(dstTextP, srcTextP, *ioSrcBytes);
		}
		
		*ioDstBytes = *ioSrcBytes;
		return(result);
	}
	
	// Loop until we've processed all of the source bytes.
	Err tempResult = errNone;
	Boolean firstTime = newConversion;
	UInt16 srcOffset = 0;
	UInt16 dstOffset = 0;
	const UInt8* srcText = (const UInt8*)srcTextP;
	UInt16 savedSrcOffset;
	
	while (srcOffset < *ioSrcBytes)
	{
		savedSrcOffset = srcOffset;
		WChar srcChar;
		
		// First we need to get the next character, as a UCS-2 16-bit value.
		if ((srcEncoding == charEncodingUCS2)
		||  (srcEncoding == charEncodingUTF16BE))
		{
			srcChar = srcText[srcOffset++];
			if (srcOffset == *ioSrcBytes)
			{
				result = txtErrConvertUnderflow;
				break;
			}
			
			srcChar = (srcChar << 8) | srcText[srcOffset++];
		}
		
		// Do byte-swapping of it's little-endian
		else if (srcEncoding == charEncodingUTF16LE)
		{
			srcChar = srcText[srcOffset++];
			if (srcOffset == *ioSrcBytes)
			{
				result = txtErrConvertUnderflow;
				break;
			}
			
			srcChar = (srcText[srcOffset++] << 8) | srcChar;
		}
		
		// If it's UTF-8, then handle algorithmic conversion of one, two, or three bytes
		// of source data into one UCS-2 character. Note that we don't handle surrogates,
		// thus the end result always fits in a 16-bit WChar.
		else if (srcEncoding == charEncodingUTF8)
		{
			UInt8 theChar = srcText[srcOffset++];
			switch (theChar >> 4)
			{
				case 0:
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
					srcChar = theChar;
				break;
				
				case 0x0C:
					srcChar = (theChar & 0x0F) << 6;
					if (srcOffset == *ioSrcBytes)
					{
						result = txtErrConvertUnderflow;
						break;
					}
					
					theChar = srcText[srcOffset++];
					ErrNonFatalDisplayIf((theChar & 0xC0) != 0x80, "Invalid UTF-8 sequence");
					srcChar |= theChar & 0x3F;
				break;
				
				case 0x0E:
					srcChar = theChar << 12;
					
					if (srcOffset == *ioSrcBytes)
					{
						result = txtErrConvertUnderflow;
						break;
					}

					theChar = srcText[srcOffset++];
					ErrNonFatalDisplayIf((theChar & 0xC0) != 0x80, "Invalid UTF-8 sequence");
					srcChar |= ((theChar & 0x003F) << 6);
					
					if (srcOffset == *ioSrcBytes)
					{
						result = txtErrConvertUnderflow;
						break;
					}
					
					theChar = srcText[srcOffset++];
					ErrNonFatalDisplayIf((theChar & 0xC0) != 0x80, "Invalid UTF-8 sequence");
					srcChar |= (theChar & 0x003F);
				break;
				
				default:
					ErrNonFatalDisplay("Invalid UTF-8 sequence");
					srcChar = kUnmappedCharCode;
				break;
			}
		}
		
		// Not UTF-8 or UCS-2, so it must be in the device's encoding. If we have
		// a table for converting from the device encoding to an index, use that
		// to extra the next character's worth of data, otherwise we assume that
		// it's single-byte only.
		else
		{
			UInt16 index;
			if (fsmDeviceToIndexArgs.iTableP != NULL)
			{
				if (firstTime && !newConversion)
				{
					// If this is our first call to TTProcessText, but it's not
					// the first time that this routine is getting called to
					// process text, then ioStateP contains saved state from
					// the previous call.
					MemMove((void*)fsmDeviceToIndexArgs.ioState, (void*)ioStateP->ioSrcState, kTxtConvertStateSize);
				}
				
				fsmDeviceToIndexArgs.input.text.ioOffset = srcOffset;
				TTProcessText(&fsmDeviceToIndexArgs, firstTime);
				firstTime = false;
				
				srcOffset = fsmDeviceToIndexArgs.input.text.ioOffset;
				if (srcOffset > *ioSrcBytes)
				{
					result = txtErrConvertUnderflow;
					break;
				}
				
				// DOLATER kwk - handle case of only getting a shift sequence
				// at the end of the text, thus we don't have a character here.
				// Maybe indicate with 0xFFFF as the result?
				index = fsmDeviceToIndexArgs.oResult;
			}
			else
			{
				index = srcText[srcOffset++];
			}
			
			// OK, now convert to a unicode character code.
			srcChar = TTConvertValue(index, indexToUnicodeTableP);
		}
		
		if (srcChar == kUnmappedCharCode)
		{
			// Delay processing until after code to write out destination bytes, as
			// that might also generate a no char mapping error.
			result = txtErrNoCharMapping;
		}
		
		// OK, we've got the source character as a UCS-2 Unicode character. Figure
		// out what we need to do to write it out.
		else if ((dstEncoding == charEncodingUCS2)
			  || (dstEncoding == charEncodingUTF16BE)
			  || (dstEncoding == charEncodingUTF16LE))
		{
			if (dstTextP != NULL)
			{
				if (dstOffset + 1 >= *ioDstBytes)
				{
					result = txtErrConvertOverflow;
					break;
				}
			
				if (dstEncoding== charEncodingUTF16LE)
				{
					dstTextP[dstOffset++] = srcChar;
					dstTextP[dstOffset++] = srcChar >> 8;
				}
				else
				{
					dstTextP[dstOffset++] = srcChar >> 8;
					dstTextP[dstOffset++] = srcChar;
				}
			}
			else
			{
				dstOffset += sizeof(WChar);
			}
		}
		
		// If it's UTF-8, then we're going to be writing out one, two, or
		// three bytes depending on the range of the source character.
		else if (dstEncoding == charEncodingUTF8)
		{
			UInt16 bytesToWrite;
			UInt8 firstByteMark;
			
			if (srcChar < 0x0080)
			{
				bytesToWrite = 1;
				firstByteMark = 0x00;
			}
			else if (srcChar < 0x0800)
			{
				bytesToWrite = 2;
				firstByteMark = 0xC0;
			}
			else
			{
				bytesToWrite = 3;
				firstByteMark = 0xE0;
			}
			
			if (dstTextP != NULL)
			{
				if (dstOffset + bytesToWrite >= *ioDstBytes)
				{
					result = txtErrConvertOverflow;
					break;
				}
				
				Char* dstText = dstTextP + dstOffset + bytesToWrite;
				switch (bytesToWrite)
				{
					case 3:	*--dstText = (srcChar | 0x80) & 0xBF; srcChar >>= 6;
					case 2:	*--dstText = (srcChar | 0x80) & 0xBF; srcChar >>= 6;
					case 1:	*--dstText =  srcChar | firstByteMark;
				}
			}
			
			dstOffset += bytesToWrite;
		}
		
		// We must be writing data out to in the device's encoding, so we have
		// to convert from UCS-2 to the device.
		else
		{
			// DOLATER kwk - if the destination encoding is something like JIS,
			// then we need to be calling TTProcessText.
			WChar dstChar = TTConvertValue(srcChar, unicodeToDeviceTableP);
			if (dstChar == kUnmappedCharCode)
			{
				result = txtErrNoCharMapping;
			}
			else
			{
				UInt16 dstSize = TxtCharSize(dstChar);
				
				if (dstTextP != NULL)
				{
					if (dstOffset + dstSize >= *ioDstBytes)
					{
						result = txtErrConvertOverflow;
						break;
					}
					TxtSetNextChar(dstTextP, dstOffset, dstChar);
				}
				
				dstOffset += dstSize;
			}
		}
		
		// If the result is the unmappable character code, then we know that
		// conversion wasn't successful. Return an error immediately if
		// we don't have a substitution string, otherwise try to write that
		// out, set up our temp result, and keep going.
		if (result == txtErrNoCharMapping)
		{
			if (substitutionStr == NULL)
			{
				break;
			}
			else
			{
				result = errNone;
				tempResult = txtErrNoCharMapping;
				
				if ((dstTextP != NULL) && (substitutionLen > 0))
				{
					if (dstOffset + substitutionLen >= *ioDstBytes)
					{
						result = txtErrConvertOverflow;
						break;
					}
					else
					{
						MemMove(dstTextP + dstOffset, substitutionStr, substitutionLen);
					}
				}
				
				dstOffset += substitutionLen;
			}
		}
	}
	
	// If we ran into an error, we need to reset the source offset to be
	// before the character that triggered the error.
	if (result != errNone)
	{
		srcOffset = savedSrcOffset;
	}
	else if (tempResult != errNone)
	{
		result = tempResult;
	}
	
	if (ioStateP != NULL)
	{
		// The caller might want to call back in to us, which means we need to update
		// our state info which the results from calling the FSM.
		// DOLATER kwk - we also need to save the Unicode=>dest state, since if
		// the destination is a stateful encoding, we need to keep track of that.
		MemMove((void*)ioStateP->ioSrcState, (void*)fsmDeviceToIndexArgs.ioState, kTxtConvertStateSize);
	}
	
	// Update the resulting source and destination offsets.
	*ioSrcBytes = srcOffset;
	*ioDstBytes = dstOffset;
	
	return(result);
} // TxtConvertEncoding


/***********************************************************************
 *
 * FUNCTION: TxtNameToEncoding
 *
 * DESCRIPTION:	Convert a character encoding name to a CharEncodingType.
 *
 * PARAMTERS:
 *	iEncodingName	 ->	Character set name.
 *
 * RETURNS:
 *	CharEncodingType, or charEncodingUnknown if the name can't be found.
 *
 * HISTORY:
 *	07/13/00	kwk	Created by Ken Krugler.
 *
 ***********************************************************************/
CharEncodingType TxtNameToEncoding(const Char* iEncodingName)
{
	ErrNonFatalDisplayIf(iEncodingName == NULL, "Null encoding name param");
	const UInt8* charsetP = (const UInt8*)GIntlData->nameToEncodingList;
	ErrNonFatalDisplayIf(charsetP == NULL, "No charset name list");
	
	UInt16 numEntries = *charsetP++;
	numEntries = (numEntries << 8) + *charsetP++;
	
	while (numEntries-- > 0)
	{
		UInt16 nameSize = StrLen((const Char*)charsetP) + 1;
		if (StrCaselessCompare((const Char*)charsetP, iEncodingName) == 0)
		{
			return(*(CharEncodingType*)(charsetP + nameSize));
		}
		else
		{
			charsetP += nameSize + sizeof(CharEncodingType);
		}
	}
	
	return(charEncodingUnknown);
} // TxtNameToEncoding


/***********************************************************************
 *
 * FUNCTION: TxtEncodingName
 *
 * DESCRIPTION:	Return a const pointer to the 'standard' name for
 *	<iEncoding>. If the encoding is unknown, return a pointer to an
 *	empty string.
 *
 * PARAMTERS:
 *	iEncoding	 ->	Character encoding.
 *
 * RETURNS:
 *	Pointer to const encoding name string.
 *
 * HISTORY:
 *	??/??/98	kwk	Created by Ken Krugler.
 *	06/05/00	kwk	Now use a string list resource.
 *
 ***********************************************************************/
const Char* TxtEncodingName(CharEncodingType iEncoding)
{
	// Use the maxEncodingTable to map from <iEncoding> to an encoding
	// rank, which we'll use as the string index. If the encoding is
	// unknown, then <iEncoding> will return back kEncodingRankUnknown,
	// which is > the max string index in our resource, and thus we'll
	// return back the empty string.
	void* tableP = GIntlData->maxEncodingTable;
	ErrNonFatalDisplayIf(tableP == NULL, "No max encoding table");
	UInt16 stringIndex = TTConvertValue(iEncoding, tableP);
	const Char* strP = (const Char*)GIntlData->encodingToNameList;
	ErrNonFatalDisplayIf(strP == NULL, "No char encoding name table");
	
	// Skip over prefix (should always be the empty string).
	strP += StrLen(strP) + 1;
	
	UInt16 numStrings = *(UInt8*)strP++;
	numStrings = (numStrings << 8) +  *(UInt8*)strP++;
	
	if (stringIndex >= numStrings)
	{
		return((const Char*)"");
	}
	
	for (UInt16 i = 0; i < stringIndex; i++)
	{
		strP += StrLen(strP) + 1;
	}
	
	return(strP);
} // TxtEncodingName


#if 0
// DOLATER kwk - get rid of this routine.
/***********************************************************************
 * OldTxtFindString
 *
 * C-based routine for searching text. if we find <inTargetStr> in
 * <inSourceStr>, return true and set <outPos> to the byte offset in
 * <inSourceStr> where the match occurred, and set <outLength> to the
 * number of bytes matched in the source string. We assume that
 * <inTargetStr> has been transformed into the standard search format.
 ***********************************************************************/

// Combine char attribute macros for better speed
#define IsSpaceOrDelim(attr,c)		(attr[(UInt8)(c)] & (charAttr_CN|charAttr_SP|charAttr_XS|charAttr_PU))

Boolean
OldTxtFindString(	const Char* inSourceStr,
				const Char* inTargetStr,
				UInt32* outPos,
				UInt16* outLength)
{
	const UInt16* charAttr = (const UInt16*)GetCharAttr();	
	const UInt8* tableP = GetCharCaselessValue();
	register UInt8 firstChr = *inTargetStr;
	register const UInt8 *s0 = (UInt8 *)inSourceStr;
	register UInt8 chr = *s0;

	ErrNonFatalDisplayIf(inSourceStr == NULL, "NULL source");
	ErrNonFatalDisplayIf(inTargetStr == NULL, "NULL target");
	ErrNonFatalDisplayIf((outPos == NULL) || (outLength == NULL), "NULL result ptr");

	// We know that for Latin the match length (in source) will always be
	// the same as the target string length (string to find), so set it up
	// now such that we can use it in our search loop below.
	
	while (chr != '\0') {
		
		// If the current character matches the first character of the 
		// string we're searching for, and the character is the start of 
		// a word, check for a word macth.
		
		chr = tableP[chr];
		if ((chr == firstChr)
		&& ((s0 == (UInt8 *)inSourceStr) || IsSpaceOrDelim (charAttr, *(s0-1)))) {
			UInt8 * s1 = (UInt8 *)inTargetStr;
			const UInt8 * s2 = s0;
			UInt8	c1, c2;
			Int16 result;
			
			do {
				c1 = *s1++;
				c2 = *s2++;
				result = c1 - tableP[c2];
			} while ((result == 0) && (c1 != '\0'));
		
			// If the match is identical or all the characters in inTargetStr 
			// were found then we have a match.
			
			if ((c1 == '\0') || (result == 0)) {
				*outLength = StrLen(inTargetStr);
				*outPos = s0 - (UInt8 *)inSourceStr;
				return(true);
			}
		}
		
		chr = *(++s0);
	}
	
	*outLength = 0;
	return(false);
} // OldTxtFindString
#endif


/***********************************************************************
 *
 * FUNCTION: TxtFindString
 *
 * DESCRIPTION: C-based routine for searching text. if we find <iTargetStrP>
 *	in <iSourceStrP>, return true and set <oMatchPos> to the byte offset in
 * <iSourceStrP> where the match occurred, and set <oMatchLength> to the
 * number of bytes matched in the source string. We assume that
 * <iTargetStrP> has been transformed into the standard search format via
 *	a call to TxtPrepFindString
 *
 * PARAMTERS:
 *	iSourceStrP		 ->	Ptr to string to search.
 *	iTargetStrP		 ->	Ptr to string we're looking for.
 *	oMatchPos		<-	Byte offset in <iSourceStrP> where we found the match.
 *	oMatchLength	<-	Length of matched text in <iSourceStrP>, which might
 *						not be the same as the length of the target string for
 *						multi-byte character encodings such as Shift-JIS.
 *
 * RETURNS:
 *	True if the target string was found, otherwise false.
 *
 * HISTORY:
 *	05/31/00	kwk	Created by Ken Krugler.
 *	06/07/00	CS	Flipped sense of TTProcessText function result (true
 *					means not done, so call again.)
 *	06/08/00	CS	Extended TTProcessArgsType for use by TTProcessToken.
 *	06/17/00	kwk	Fixed bug in GET_TARGET_CHAR, where it was returning
 *					<high byte><high byte>, and where it was still fetching
 *					two bytes even when it reached the terminating null.
 *	09/13/00	kwk	Re-wrote loop logic so that it works with target strings
 *					that start with a word delimiter character such as '+'.
 *					Now the word start table is only used to figure out if
 *					the preceding character is a delimiter or not.
 *
 ***********************************************************************/

// Get the character at <ptr>, in a way that's safe if the size is
// two bytes even if the ptr is odd-aligned. Also note that if we're
// pointing at a null byte, return chrNull even if the char size is two.
#define GET_TARGET_CHAR(ptr, size)	(size == 1 ? (*((UInt8*)(ptr))) \
					: (*ptr == 0 ? chrNull : ((((UInt16)*(UInt8*)(ptr)) << 8) | *(UInt8*)(ptr + 1))))


Boolean TxtFindString(const Char* iSourceStrP, const Char* iTargetStrP,
								UInt32* oMatchPos, UInt16* oMatchLength)
{
	ErrNonFatalDisplayIf(iSourceStrP == NULL, "Null source string");
	ErrNonFatalDisplayIf(iTargetStrP == NULL, "Null target string");
	ErrNonFatalDisplayIf(oMatchPos == NULL, "Null match position param");
	ErrNonFatalDisplayIf(oMatchLength == NULL, "Null match position param");
	
	if ((GIntlData->intlFlags & kByteSearchingFlag) != 0)
	{
		const UInt8* srcStringP =  (const UInt8*)iSourceStrP;
		const UInt8* wordStartTableP = (const UInt8*)GIntlData->findWordStartTable;
		const UInt8* wordMatchTableP = (const UInt8*)GIntlData->findWordMatchTable;
		UInt8 dstFirstChar = *iTargetStrP++;
		UInt8 srcMappedChar = *srcStringP;
		
		if (dstFirstChar == chrNull)
		{
			return(false);
		}
		
		while (srcMappedChar != chrNull)
		{
			// Spin until we find a character in the source text that matches
			// the first character in our target text string.
			srcMappedChar = wordMatchTableP[srcMappedChar];
			
			// We've got a character that's at the beginning of the word.
			// See if it matches the first target character. If it does, and
			// we're at the start of the string, or there's no word start table,
			// or the word start table tells us that the character preceding
			// this character is a delimiter, then we've got a good match.
			if ((srcMappedChar == dstFirstChar)
			 && ((srcStringP == (UInt8*)iSourceStrP)
			  || (wordStartTableP == NULL)
			  || (wordStartTableP[*(srcStringP - 1)] != 0)))
			{
				// Help the compiler out. Even with max optimization settings,
				// it won't save registers on the stack & then re-use them in
				// this loop, nor will it use a0/a1 for pointers, so we'll do
				// the saving for it.
				const UInt8* savedSrcStringP = srcStringP++;
				const Char* savedTargetStringP = iTargetStrP;
				UInt8 c1;
				
				do
				{
					c1 = *iTargetStrP++;
				} while ((wordMatchTableP[*srcStringP++] == c1) && (c1 != chrNull));

				// If we terminated this loop because we reached the end of the
				// target string, then we've got a match.
				if (c1 == chrNull)
				{
					*oMatchPos = ((const Char*)savedSrcStringP - iSourceStrP);
					*oMatchLength = StrLen(savedTargetStringP - 1);
					return(true);
				}
				else
				{
					srcStringP = savedSrcStringP;
					iTargetStrP = savedTargetStringP;
				}
			}
			
			srcMappedChar = *(++srcStringP);
		}
		
		return(false);
	}
	
	// We have to do the slower find operation, since one or both of the word start/match
	// tables is something other than a byte-indexed byte table. In that case,
	// we use the size of elements in the match table to tell us the size of
	// elements in the target string (either one or two bytes). 
	else
	{
		UInt16 targetCharSize = TTGetNumResultBits(GIntlData->findWordMatchTable);
		ErrNonFatalDisplayIf((targetCharSize != 8) && (targetCharSize != 16),
									"Invalid match result bits");
		targetCharSize = targetCharSize / 8;
		
		// Load the first character in the target string. If this is a null,
		// then we got passed an empty find string...return false in that case.
		UInt16 dstFirstChar = GET_TARGET_CHAR(iTargetStrP, targetCharSize);
		if (dstFirstChar == chrNull)
		{
			return(false);
		}
		
		iTargetStrP += targetCharSize;
		
		TTProcessArgsType fsmWordMatchArgs;
		fsmWordMatchArgs.input.text.iTextP = iSourceStrP;
		fsmWordMatchArgs.input.text.ioOffset = 0;
		fsmWordMatchArgs.input.text.iLength = kMaxUInt32;
		fsmWordMatchArgs.iTableP = GIntlData->findWordMatchTable;
		
		while (iSourceStrP[fsmWordMatchArgs.input.text.ioOffset] != chrNull)
		{
			const Char* targetStrP = iTargetStrP;
			WChar curDstChar = dstFirstChar;
			Boolean firstWMTime = true;
			Boolean keepGoing;
			UInt32 wordStartOffset;
			UInt32 nextCharOffset;
			
			do
			{
				// If this is the first character we're grabbing from
				// the source string (for this current match attempt),
				// then save the offset after the character for advancing.
				if (firstWMTime)
				{
					wordStartOffset = fsmWordMatchArgs.input.text.ioOffset;
					keepGoing = TTProcessText(&fsmWordMatchArgs, true);
					nextCharOffset = fsmWordMatchArgs.input.text.ioOffset;
				}
				else
				{
					keepGoing = TTProcessText(&fsmWordMatchArgs, false);
				}
				
				// If it doesn't match, break out of the loop, which will then
				// have us advance to the next character.
				if (fsmWordMatchArgs.oResult != curDstChar)
				{
					break;
				}
				
				// If this is the first character we're processing, then we
				// need to check if it's at the beginning of a word.
				else if (firstWMTime)
				{
					if ((GIntlData->findWordStartTable != NULL) && (wordStartOffset > 0))
					{
						// DOLATER kwk - if the word start table isn't an FSM
						// table, then we need to call TxtGetPreviousChar and
						// TTConvertValue...or perhaps we just enforce the constraint
						// that if you have two tables, they must either be two
						// byte-indexed tables, or two FSMs, where the word start
						// table is a scan-right, and the word match is a scan left.
						TTProcessArgsType fsmWordStartArgs;
						fsmWordStartArgs.input.text.iTextP = iSourceStrP;
						fsmWordStartArgs.input.text.iLength = kMaxUInt32;
						fsmWordStartArgs.input.text.ioOffset = wordStartOffset;
						fsmWordStartArgs.iTableP = GIntlData->findWordStartTable;
						
						TTProcessText(&fsmWordStartArgs, true);
						
						// We better be at the beginning of a word, otherwise this
						// isn't a valid match position.
						if (fsmWordStartArgs.oResult == 0)
						{
							break;
						}
					}
					
					firstWMTime = false;
				}
				
				curDstChar = GET_TARGET_CHAR(targetStrP, targetCharSize);
				targetStrP += targetCharSize;
			} while (curDstChar != chrNull);
			
			// If we hit the end of the target string, but the FSM has additional
			// bytes to pump out, then we didn't really match (e.g. an eszett in
			// the source string might get expanded into 'SS', but we wouldn't want
			// to say we matched if the end of the target string was a single 'S').
			if ((curDstChar == chrNull) && !keepGoing)
			{
				// We have a real match. fsmWordMatchArgs.input.text.ioOffset has the offset of
				// the end of the match.
				*oMatchPos = wordStartOffset;
				*oMatchLength = fsmWordMatchArgs.input.text.ioOffset - wordStartOffset;
				return(true);
			}
			
			// It didn't match, so now we need to reset for the next loop.
			fsmWordMatchArgs.input.text.ioOffset = nextCharOffset;
		}

		return(false);
	}
} // TxtFindString


/***********************************************************************
 *
 * FUNCTION: TxtGetChar
 *
 * DESCRIPTION:	Utility routine that returns the character at offset
 *	<iOffset> in the <iTextP> text. For those callers who don't care
 *	about the size of the character, and don't want to declare a local
 *	variable just so that they can call TxtGetNextChar.
 *
 * PARAMTERS:
 *	iTextP	 ->	Ptr to text.
 *	iOffset	 ->	Offset preceding the character.
 *
 * RETURNS:
 *	Character at offset <iOffset>
 *
 * HISTORY:
 *	05/20/00	kwk	Modified to use TxtGetNextChar if necessary.
 *
 ***********************************************************************/
WChar TxtGetChar(const Char* iTextP, UInt32 iOffset)
{
	ErrNonFatalDisplayIf(iTextP == NULL, "NULL string parameter");
	
	// If there's no table for TxtGetNextChar, then we know that it's
	// got to be a single-byte value, so do the fast fetch thing.
	if (GIntlData->nextCharGetTable == NULL)
	{
		return(*((UInt8*)iTextP + iOffset));
	}
	else
	{
		WChar theChar;
		TxtGetNextChar(iTextP, iOffset, &theChar);
		return(theChar);
	}
} // TxtGetChar


/***********************************************************************
 *
 * FUNCTION: TxtGetNextChar
 *
 * DESCRIPTION:	Load the character following offset <iOffset> in the
 *	<iText> text. Return back the size of the character.
 *
 * PARAMTERS:
 *	iTextP	 ->	Ptr to text.
 *	iOffset	 ->	Offset preceding the character.
 *	oChar	<-	Character following iOffset.
 *
 * RETURNS:
 *	Size of character (bytes it occupies in the text).
 *
 * HISTORY:
 *	05/17/00	kwk	Modified to use a table, and call TxtCharIsValid.
 *	06/01/00	kwk	OK, really use TTProcessText.
 *	06/08/00	CS	Extended TTProcessArgsType for use by TTProcessToken.
 *	12/20/00	kwk	Use fast doubleChar table if it exists.
 *
 ***********************************************************************/
UInt16 TxtGetNextChar(const Char* iTextP, UInt32 iOffset, WChar* oChar)
{
	ErrNonFatalDisplayIf(iTextP == NULL, "NULL string parameter");

	// If there's a double-char table, then do special processing that
	// is fast but only works for double-byte encodings.
	if (GIntlData->doubleCharTable != NULL)
	{
		WChar tempChar;
		WChar* tCharP = (oChar == NULL ? &tempChar : oChar);
		const Int8* tableP = (const Int8*)GIntlData->doubleCharTable;
		
		// Load the first byte at <iOffset> as the result. We always
		// grab this byte, even if oChar == NULL, since we'll need it
		// anyway to index into the double-char table.
		*tCharP = *((UInt8*)iTextP + iOffset);
		
		// Is it single or single/low? If so, then it's a single byte.
		if (DoubleAttr_(tableP, *tCharP) >= doubleAttrSingle)
		{
			return(sizeof(Char));
		}
		else
		{
			// We have a high byte, so grab the low byte.
			WChar lowByte = *((UInt8*)iTextP + iOffset + 1);
			if (DoubleAttr_(tableP, lowByte) == doubleAttrSingle)
			{
				ErrNonFatalDisplay("TxtGetNextChar - Invalid character");
				return(sizeof(Char));
			}
			else
			{
				if (oChar != NULL)
				{
					*tCharP = (*tCharP << 8) | lowByte;
				}
				return(sizeof(WChar));
			}
		}
	}
	
	// If there's no double-char table and no next char table, then
	// assume it's a single-byte encoding.
	else if (GIntlData->nextCharGetTable == NULL)
	{
		if (oChar != NULL)
		{
			*oChar = *(UInt8*)(iTextP + iOffset);
		}
		return(1);
	}
	
	// We have to do the slow FSM thing.
	else
	{
		TTProcessArgsType fsmArgs;
		fsmArgs.input.text.iTextP = iTextP;
		fsmArgs.input.text.iLength = kMaxUInt32;
		fsmArgs.input.text.ioOffset = iOffset;
		fsmArgs.iTableP = GIntlData->nextCharGetTable;

		TTProcessText(&fsmArgs, true);
		
		ErrNonFatalDisplayIf(!TxtCharIsValid(fsmArgs.oResult), "Invalid character");
		
		if (oChar != NULL)
		{
			*oChar = fsmArgs.oResult;
		}
		
		return(fsmArgs.input.text.ioOffset - iOffset);
	}
} // TxtGetNextChar


/***********************************************************************
 *
 * FUNCTION: TxtGetPreviousChar
 *
 * DESCRIPTION:	Load the character before offset <iOffset> in the
 *	<iTextP> text. Return back the size of the character.
 *
 * PARAMTERS:
 *	iTextP	 ->	Ptr to text.
 *	iOffset	 ->	Offset following character.
 *	oChar	<-	Character preceding iOffset.
 *
 * RETURNS:
 *	Size of character (bytes it occupies in the text).
 *
 * HISTORY:
 *	??/??/98	kwk	Created by Ken Krugler.
 *	10/04/99	kwk	Fixed sign extend bug where 0x96 (for example) would
 *					be returned as 0xFF96.
 *	05/17/00	kwk	Modified to use a table.
 *	06/01/00	kwk	OK, really use TTProcessText.
 *	06/08/00	CS	Extended TTProcessArgsType for use by TTProcessToken.
 *	12/20/00	kwk	Use fast doubleChar table if it exists.
 *
 ***********************************************************************/
UInt16 TxtGetPreviousChar(const Char* iTextP, UInt32 iOffset, WChar* oChar)
{
	ErrNonFatalDisplayIf(iTextP == NULL, "NULL string parameter");

	// If we're at the beginning of the string, generate a fatal alert
	// on debug ROMs, and return a size of zero.
	if (iOffset == 0)
	{
		ErrNonFatalDisplay("TxtGetPreviousChar called with 0 offset");
		
		if (oChar != NULL)
		{
			*oChar = 0;
		}
		return(0);
	}
	
	// If there's a double-char table, then do special processing that
	// is fast but only works for double-byte encodings.
	if (GIntlData->doubleCharTable != NULL)
	{
		WChar tempChar;
		WChar* tCharP = (oChar == NULL ? &tempChar : oChar);
		UInt8* byteP = (UInt8*)iTextP + iOffset;
		const Int8* tableP = (const Int8*)GIntlData->doubleCharTable;
		
		*tCharP = *--byteP;
		
		Int8 doubleAttr = DoubleAttr_(tableP, *tCharP);
		if (doubleAttr == doubleAttrSingle)
		{
			return(sizeof(Char));
		}
		
		// We've got <H/L>, so it should be the low byte of a double-
		// byte character...but just to be safe, make sure the high
		// byte actually exists (offset > 1), and that it's valid.
		else if (doubleAttr == doubleAttrHighLow)
		{
			if (iOffset == 1)
			{
				ErrNonFatalDisplay("TxtGetPreviousChar - Invalid character");
				return(sizeof(Char));
			}
			else
			{
				WChar leadChar = *--byteP;
				if (DoubleAttr_(tableP, leadChar) != doubleAttrHighLow)
				{
					ErrNonFatalDisplay("TxtGetPreviousChar - Invalid character");
					return(sizeof(Char));
				}
				else
				{
					if (oChar != NULL)
					{
						*tCharP = (leadChar << 8) | *tCharP;
					}
					return(sizeof(WChar));
				}
			}
		}
		
		// We've got <S/L>, which is the complex case.
		else
		{
			if (iOffset == 1)
			{
				return(sizeof(Char));
			}
			
			// If we have <H/L><S/L> then we could either be looking
			// at a single or a low byte. The only way to know for sure
			// is to determine the character bounds.
			if (DoubleAttr_(tableP, *--byteP) == doubleAttrHighLow)
			{
				// DOLATER kwk - we could do our own custom loop here, which
				// would be faster than calling TxtCharBounds.
				UInt32 charStart, charEnd;
				*tCharP = TxtCharBounds(iTextP, iOffset - 1, &charStart, &charEnd);
				return(charEnd - charStart);
			}
			
			// The only other two cases are <S><S/L> and <S/L><S/L>, both of
			// which mean we've got a single byte.
			else
			{
				return(sizeof(Char));
			}
		}
	}
	
	// If there's no table, assume it's a single-byte encoding.
	else if (GIntlData->prevCharSizeTable == NULL)
	{
		if (oChar != NULL)
		{
			*oChar = *(UInt8*)(iTextP + iOffset - 1);
		}
		return(1);
	}
	
	// We have to do the slow processing thing.
	else
	{
		TTProcessArgsType fsmArgs;
		fsmArgs.input.text.iTextP = iTextP;
		fsmArgs.input.text.iLength = kMaxUInt32;
		fsmArgs.input.text.ioOffset = iOffset;
		fsmArgs.iTableP = GIntlData->prevCharSizeTable;

		// The previous char size table will return as its result
		// the number of bytes occupied by the character preceding
		// iOffset in the text.
		TTProcessText(&fsmArgs, true);
		
		// Quick sanity check - the character better not extend beyond
		// the start of the test stream.
		ErrNonFatalDisplayIf(fsmArgs.oResult > iOffset, "Bad char size");
		
		// TxtGetNextChar will validate the resulting character for us.
		UInt16 checkSize = TxtGetNextChar(iTextP, iOffset - fsmArgs.oResult, oChar);
		ErrNonFatalDisplayIf(checkSize != fsmArgs.oResult, "Get/Set char size different");
		
		return(fsmArgs.oResult);
	}
} // TxtGetPreviousChar


/***********************************************************************
 *
 * FUNCTION:	TxtCharIsValid
 *
 * DESCRIPTION: Return true if <iChar> is a valid (drawable) character.
 *	Note that we'll return false for a virtual character code.
 *
 * PARAMETERS:
 *	iChar	 -> WChar character code.
 *
 * RETURNS:		True if valid, otherwise false.
 *
 * HISTORY:
 *	??/??/98	kwk	Created by Ken Krugler.
 *	05/20/00	kwk	Use TTConvertValue if there's a table.
 *
 ***********************************************************************/
Boolean TxtCharIsValid(WChar iChar)
{
	// If it looks like a byte value was sign-extended by the compiler,
	// complain on debug ROMs, and then fix up the value & continue.
	if (iChar >= 0xFF80)
	{
		ErrNonFatalDisplay("Sign-extended char passed to TxtCharIsValid");
		iChar &= 0x00FF;
	}

	void* tableP = GIntlData->charValidTable;
	if (tableP == NULL)
	{
		return(iChar < 0x0100);
	}
	else
	{
		return(TTConvertValue(iChar, tableP) != 0);
	}
} // TxtCharIsValid


/***********************************************************************
 *
 * FUNCTION:	TxtMaxEncoding
 *
 * DESCRIPTION: Return the higher (max) encoding of <a> and <b>, where
 *	'higher' encodings are more specialized (e.g. 8859-1 is 'higher'
 *	then ascii). The highest encoding is always charEncodingUnknown.
 *
 * PARAMETERS:
 *	a	 ->	Character encoding.
 *	b	 ->	Character encoding.
 *
 * RETURNS:
 *	The higher of the two character encodings.
 *
 * HISTORY:
 *	??/??/98	kwk	Created by Ken Krugler.
 *	05/30/00	kwk	Use table to decide which encoding is higher.
 *
 ***********************************************************************/
CharEncodingType TxtMaxEncoding(CharEncodingType iEncodingA, CharEncodingType iEncodingB)
{
	if (iEncodingA == iEncodingB)
	{
		return(iEncodingA);
	}
	else if ((iEncodingA == charEncodingUnknown) || (iEncodingB == charEncodingUnknown))
	{
		return(charEncodingUnknown);
	}
	
	// Now a != b, and both a & b are something other than charEncodingUnknown.
	// We'll use TTConvertValue to convert both to a relative encoding
	// 'sort' value, which can be compared.
	void* tableP = GIntlData->maxEncodingTable;
	ErrNonFatalDisplayIf(tableP == NULL, "No max encoding table");
	
	if (TTConvertValue(iEncodingA, tableP) > TTConvertValue(iEncodingB, tableP))
	{
		return(iEncodingA);
	}
	else
	{
		return(iEncodingB);
	}	
} // TxtMaxEncoding


/***********************************************************************
 *
 * FUNCTION:	TxtPrepFindString
 *
 * DESCRIPTION: Given a string <iSrcTextP>, convert it into a standard
 *	format search target string and put the result in <oDstStrP> as a
 *	standard C string. Make sure we don't run past the end of <oDstStrP>,
 *	which can hold up to <iDstSize> bytes, including the terminating null
 *	byte. Return back the number of bytes consumed in <iSrcTextP>.
 *
 *	Note that this routine returned nothing (void) in versions of the OS
 *	previous to 3.5 (and didn't exist before Palm OS 3.1), and that it
 *	didn't have an <iSrcLength> parameter prior to 3.5.
 *
 * PARAMETERS:
 *	iSrcTextP	 -> Ptr to text to convert.
 *	iSrcLength	 ->	Bytes to convert from <iSrcTextP>
 *	oDstStrP	 ->	Ptr to buffer to hold result.
 *	iDstSize	 ->	Size of buffer.
 *
 * RETURNS:
 *	Number of bytes converted from <iSrcTextP>.
 *
 * HISTORY:
 *	??/??/98	kwk	Created by Ken Krugler.
 *	07/13/99	kwk	Use new iSrcLength param, and handle case of being
 *					passed a NULL destination buffer ptr.
 *	06/01/00	kwk	Use word match table to create data for dest buffer.
 *	06/08/00	CS	Extended TTProcessArgsType for use by TTProcessToken.
 *	06/17/00	kwk	Loop limit is iDstSize, not iDstSize - 1, since we
 *					pre-decrement the dest size for the null at end.
 *	09/13/00	kwk	Handle case of last char in string being ignorable.
 *	12/21/00	kwk	Catch error case w/null table pointer.
 *
 ***********************************************************************/
UInt16 TxtPrepFindString(const Char* iSrcTextP, UInt16 iSrcLength,
						Char* oDstStrP, UInt16 iDstSize)
{
	ErrNonFatalDisplayIf(iSrcTextP == NULL, "NULL source");
	ErrNonFatalDisplayIf(iDstSize == 0, "Dest buffer too small");
	
	// Leave space for chrNull at end of destination string.
	iDstSize -= 1;
	
	const UInt8* wordMatchTableP;
	Boolean firstTime;
	UInt16 charSize;
	TTProcessArgsType fsmArgs;
	
	// If we're doing fast searching, then the match table is a byte-indexed
	// byte table.
	ErrNonFatalDisplayIf(GIntlData->findWordMatchTable == NULL, "No findWordMatchTable");
	
	if ((GIntlData->intlFlags & kByteSearchingFlag) != 0)
	{
		wordMatchTableP = (const UInt8*)GIntlData->findWordMatchTable;
		charSize = 1;
	}
	else
	{
		wordMatchTableP = NULL;
		firstTime = true;
		fsmArgs.input.text.iTextP = iSrcTextP;
		fsmArgs.input.text.iLength = iSrcLength;
		fsmArgs.iTableP = GIntlData->findWordMatchTable;
		charSize = TTGetNumResultBits(fsmArgs.iTableP);
		ErrNonFatalDisplayIf((charSize != 8) && (charSize != 16), "Invalid result bits");
		charSize = charSize / 8;
	}
	
	fsmArgs.input.text.ioOffset = 0;
	UInt16 dstOffset = 0;
	while ((dstOffset + charSize <= iDstSize) && (fsmArgs.input.text.ioOffset < iSrcLength))
	{
		// Get the next match value from the source string.
		if (wordMatchTableP != NULL)
		{
			fsmArgs.oResult = wordMatchTableP[*(UInt8*)(iSrcTextP + fsmArgs.input.text.ioOffset++)];
		}
		else
		{
			// DOLATER kwk - there's a potential problem here, where if the source
			// expands (e.g. '§' => 'SS') and only the first 'S' fits, then we'd
			// wind up generating invalid results. So really we only want to update
			// the source/dest offsets when TTProcessText returns true, otherwise
			// wait until we're sure that there's enough space.
			TTProcessText(&fsmArgs, firstTime);
			firstTime = false;
		}
		
		// Now write the resulting character out the the destination
		// buffer, if it's not NULL, and the result is not 0. The result could be
		// zero if we got asked to process a string with a null byte in it, or
		// if the last thing in the string is ignorable (e.g. chou--on).
		if (fsmArgs.oResult == 0)
		{
			break;
		}
		else if (oDstStrP != NULL)
		{
			for (UInt16 i = charSize; i > 0; i--)
			{
				oDstStrP[dstOffset + i - 1] = fsmArgs.oResult;
				fsmArgs.oResult >>= 8;
			}
		}
		
		dstOffset += charSize;
	}
	
	// Terminate the destination string, and return the amount of
	// text converted from the source string.
	if (oDstStrP != NULL)
	{
		oDstStrP[dstOffset] = chrNull;
	}
	
	return(fsmArgs.input.text.ioOffset);
} // TxtPrepFindString


/***********************************************************************
 * TxtReplaceStr
 *
 * Replace the substrings ^<inParamNum> in <ioStr> by the string <inParamStr>.
 * If <inParamStr> is nil, don't modify the substring text. Make sure <ioStr>
 * doesn't expand beyond <inMaxLen> bytes, excluding the terminating null.
 * The range for <inParamNum> is 0...9. Multiple instances of the parameter
 * will be replace if found, as long as <inMaxLen> isn't exceeded. Return
 * back the number of occurances replaced.
 ***********************************************************************/

UInt16
TxtReplaceStr(Char * ioStr, UInt16 inMaxLen, const Char* inParamStr, UInt16 inParamNum)
{
	UInt16 replaceCount = 0;
	UInt16 curLength;
	UInt16 paramLength;
	Char * str;
	
	ErrNonFatalDisplayIf(ioStr == NULL, "Invalid string pointer");
	ErrNonFatalDisplayIf(inParamNum > 9, "Invalid parameter number");
	
	// If inParamStr is nil then we act as though it's "^N", where N is
	// the value of inParamNum.
	
	curLength = StrLen(ioStr);
	paramLength = (inParamStr == NULL ? 2 : StrLen(inParamStr));
	
	str = ioStr;
	while (str != NULL) {
		str = StrChr(str, '^');
		if (str != NULL) {
			
			// If we have a match, then see if there's enough room to insert
			// the replacement text. If so, then adjust the gap (remove the
			// ^X text, and insert the param text) and do the copy if we're
			// not just deleting stuff.
			
			WChar nextChar;
			if ((TxtGetNextChar(str, 1, &nextChar) == 1)
			&& ((nextChar - '0') == inParamNum)) {
				if (curLength + paramLength - 2 <= inMaxLen) {
					if (inParamStr != NULL) {
						if (paramLength != 2) {
							MemMove(str + paramLength, str + 2, (ioStr + curLength + 1 - str - 2));
						}
						
						if (paramLength > 0) {
							MemMove(str, inParamStr, paramLength);
						}
					}
					
					replaceCount += 1;
					str += paramLength;
					curLength += (paramLength - 2);
				} else {
					break;
				}
			} else {
			
				// Next character wasn't the param number we're looking for,
				// so advance past the '^' character and keep looking.
				
				str += 1;
			}
		}
	}
	
	return(replaceCount);
} // TxtReplaceStr

				
/***********************************************************************
 *
 * FUNCTION: TxtParamString
 *
 * DESCRIPTION:	Allocate a new string and fill it in, using <iTemplateP>
 *	and up to four parameter strings, any and all of which can be NULL.
 *	The template can contain any number and mix of parameters ^0 through
 *	^3, including multiple occurances of any parameter. Note that this
 *	routine returns the result of locking a handle, thus you could recover
 *	the handle, unlock the ptr, and use the handle in a text field.
 *
 * PARAMTERS:
 *	iTemplateP	 ->	Ptr to template string.
 *	iParam0		 -> Ptr to string to substitute for ^0, or NULL.
 *	iParam1		 -> Ptr to string to substitute for ^1, or NULL.
 *	iParam2		 -> Ptr to string to substitute for ^2, or NULL.
 *	iParam3		 -> Ptr to string to substitute for ^3, or NULL.
 *
 * RETURNS:
 *	Ptr to locked handle containing result of substitution.
 *
 * HISTORY:
 *	09/22/99	kwk	Created by Ken Krugler.
 *	08/20/00	kwk	Re-wrote to fix numerous bugs, including total failure
 *					when a param string contained a subsequent ^x sub-string
 *					(e.g. iParam0 = "blah blah ^1").
 *	11/08/00	kwk	Don't rely on compiler to do auto-initialization of
 *					local variable from parameters.
 *
 ***********************************************************************/

#define	templateParamStrLen	2		// Length of ^n parameter in template.

Char* TxtParamString(const Char* iTemplateP, const Char* iParam0,
			const Char* iParam1, const Char* iParam2, const Char* iParam3)
{
	ErrNonFatalDisplayIf(iTemplateP == NULL, "Null template string");
	
	const Char* params[4];
	params[0] = iParam0;
	params[1] = iParam1;
	params[2] = iParam2;
	params[3] = iParam3;

	UInt16 resultLen = StrLen(iTemplateP) + sizeOf7BitChar(chrNull);
	UInt16 handleLen = resultLen;
	MemHandle resultH = MemHandleNew(handleLen);
	ErrNonFatalDisplayIf(resultH == NULL, "Out of memory");
	Char* resultP = (Char*)MemHandleLock(resultH);
	StrCopy(resultP, iTemplateP);
	
	for (UInt16 i = 0; i < 4; i++)
	{
		Int16 replaceCount = TxtReplaceStr(resultP, kMaxUInt16, NULL, i);
		
		if (replaceCount > 0)
		{
			const Char* emptyString = "";
			const Char* paramStr = params[i];
			if (paramStr == NULL)
			{
				paramStr = emptyString;
			}

			Int16 deltaLen = StrLen(paramStr) - templateParamStrLen;
			Int16 totalDelta = deltaLen * replaceCount;
			resultLen += totalDelta;
			
			if (resultLen > handleLen)
			{
				MemHandleUnlock(resultH);
				Err result = MemHandleResize(resultH, resultLen);
				ErrNonFatalDisplayIf(result != errNone, "Out of memory");
				handleLen = resultLen;
				resultP = (Char*)MemHandleLock(resultH);
			}
			
			TxtReplaceStr(resultP, handleLen, paramStr, i);
			ErrNonFatalDisplayIf(resultLen != StrLen(resultP) + sizeOf7BitChar(chrNull), "Calculation error");
		}
	}

	return(resultP);
} // TxtParamString

						
/***********************************************************************
 *
 * FUNCTION: TxtSetNextChar
 *
 * DESCRIPTION:	Set the character at offset <iOffset> in the <oTextP>
 *	text, and return back the size of the character in the text.
 *
 * PARAMTERS:
 *	oTextP	 ->	Ptr to text.
 *	iOffset	 -> Byte offset into text.
 *
 * RETURNS:
 *	Size that character occupies in the text string, in bytes.
 *
 * HISTORY:
 *	??/??/98	kwk	Created by Ken Krugler.
 *	06/01/00	kwk	Modified to use table.
 *	06/07/00	CS	Flipped sense of TTProcessText function result (true
 *						means not done, so call again.)
 *	06/08/00	CS	Extended TTProcessArgsType for use by TTProcessToken.
 *	12/20/00	kwk	Use fast doubleChar table if it exists.
 *
 ***********************************************************************/
UInt16 TxtSetNextChar(Char* oTextP, UInt32 iOffset, WChar iChar)
{
	ErrNonFatalDisplayIf(oTextP == NULL, "NULL string parameter");

	// If it looks like a byte value was sign-extended by the compiler,
	// complain on debug ROMs, and then fix up the value & continue.
	if (iChar >= 0xFF80)
	{
		ErrNonFatalDisplay("Sign-extended char passed to TxtSetNextChar");
		iChar &= 0x00FF;
	}

	// On debug ROMs, make sure the character is valid (e.g. not a
	// virtual character).
	ErrNonFatalDisplayIf(!TxtCharIsValid(iChar), "Invalid char passed to TxtSetNextChar");

	// If there's a double-char table, then check whether high byte is
	// a double-byte character code. Note that if it's not, we _assume_
	// that it's zero...if not, then the TxtCharIsValid call will have
	// reported the problem.
	if (GIntlData->doubleCharTable != NULL)
	{
		if (DoubleAttr_(GIntlData->doubleCharTable, iChar >> 8) == doubleAttrHighLow)
		{
			*(oTextP + iOffset++) = (iChar >> 8);
			*(oTextP + iOffset) = iChar;
			return(2);
		}
		else
		{
			*(oTextP + iOffset) = iChar;
			return(1);
		}
	}
	
	// If no next-char table, assume single byte.
	else if (GIntlData->nextCharSetTable == NULL)
	{
		*(oTextP + iOffset) = iChar;
		return(1);
	}
	
	// Do the slow FSM table processing thing.
	else
	{
		TTProcessArgsType fsmArgs;
		fsmArgs.input.text.iTextP = (const Char*)&iChar;
		fsmArgs.input.text.iLength = sizeof(WChar);
		fsmArgs.input.text.ioOffset = 0;
		fsmArgs.iTableP = GIntlData->nextCharSetTable;

		UInt32 dstOffset = iOffset;
		Boolean firstTime = true;
		Boolean keepGoing;
		
		do
		{
			keepGoing = TTProcessText(&fsmArgs, firstTime);
			firstTime = false;
			
			oTextP[dstOffset++] = fsmArgs.oResult;
		} while (keepGoing);
		
		return(dstOffset - iOffset);
	}
} // TxtSetNextChar


/***********************************************************************
 *
 * FUNCTION:	TxtStrEncoding
 *
 * DESCRIPTION: Return the minimum (lowest) encoding required to represent
 *	<iStrP>. This is the maximum encoding of any character in the string,
 *	where highest is unknown, and lowest is ascii.
 *
 * PARAMETERS:
 *	iStrP	 -> Ptr to string.
 *
 * RETURNS:		Minimum necessary character encoding.
 *
 * HISTORY:
 *	??/??/98	kwk	Created by Ken Krugler.
 *	10/07/99	kwk	Do fast loop looking for low-ascii, then slow loop
 *					if required.
 *	05/20/00	kwk	Do safe processing of text if we encounter a non-
 *					ASCII character.
 *
 ***********************************************************************/
CharEncodingType TxtStrEncoding(const Char* iStrP)
{
	UInt8 curChar;
	const UInt8* curPtr = (const UInt8*)iStrP;
	
	ErrNonFatalDisplayIf(iStrP == NULL, "NULL string parameter");

	// First see if the entire string is low-ascii, doing a fast check
	// for characters all < 0x80. Note that this treats 0x5C as a backslash
	// character in Shift-JIS, not a yen character, since Microsoft seems
	// to think that's the right thing to do.
	while ((curChar = *curPtr++) != chrNull)
	{
		if (curChar > kTxtMaxAlwaysSingleByte)
		{
			curPtr--;
			break;
		}
	}
	
	// We didn't hit the end of the string, so now we have to do the
	// slower processing thing of safely loading each character, and
	// then calculating its encoding, and then calling TxtMaxEncoding.
	CharEncodingType curEncoding = charEncodingAscii;
	if (curChar != chrNull)
	{
		WChar theChar = curChar;
		while (theChar != chrNull)
		{
			curPtr += TxtGetNextChar((const Char*)curPtr, 0, &theChar);
			if (theChar != chrNull)
			{
				curEncoding = TxtMaxEncoding(curEncoding, TxtCharEncoding(theChar));
			}
		}
	}
	
	return(curEncoding);
} // TxtStrEncoding


/***********************************************************************
 *
 * FUNCTION: TxtTransliterate
 *
 * DESCRIPTION:	Transliterate <iSrcLength> bytes of text found in <iSrcTextP>,
 *	based on the requested <iTransOp> operation. Place the results in <oDstTextP>,
 *	and set the resulting length in <ioDstLength>. On entry <ioDstLength>
 *	must contain the maximum size of the <oDstTextP> buffer. If the
 *	buffer isn't large enough, return an error (note that oDstTextP
 *	might have been modified during the operation). Note that if <iTransOp>
 *	has the preprocess bit set, then <oDstTextP> is not modified, and
 *	<ioDstLength> will contain the total space required in the destination
 *	buffer in order to perform the operation.
 *
 * PARAMTERS:
 *	iSrcTextP	 ->	Pointer to source text.
 *	iSrcLength	 -> Bytes of source to process. Could be something huge
 *					like 0xFFFF if the string is null-terminated.
 *	oDstTextP	 -> Destination text buffer.
 *	ioDstLength	<-> Size of buffer on entry, length of result on exit.
 *	iTransOp	 ->	Operation to perform.
 *
 * RETURNS:
 *	errNone if successful
 *	txtErrUknownTranslitOp if iTransOp is unknown or unsupported
 *	txtErrTranslitOverflow if the destination buffer is too small
 *	txtErrTranslitOverrun if the source/dest buffers overlap and the
 *		act of writing to the dest buffer would modify unprocessed
 *		source data.
 *	txtErrTranslitUnderflow if the source length is in the middle
 *		of a multi-byte character.
 *
 * HISTORY:
 *	05/16/00	kwk	Modified to use a table, if it exists.
 *	05/30/00	kwk	Re-wrote to rely on TTProcessText.
 *	06/07/00	CS	Flipped sense of TTProcessText function result (true
 *					means not done, so call again.)
 *	06/08/00	CS	Extended TTProcessArgsType for use by TTProcessToken.
 *	06/13/00	kwk	Support tables that can't be used with TTProcessText.
 *	06/15/00	kwk	Handle TTProcessText returning back an unmappable
 *					character code.
 *	07/29/00	kwk	Terminate loop when we hit a null byte in the source.
 *	11/24/00	kwk	Weaken check for over-run case; we now only do this
 *					when the source ptr == dest ptr, as otherwise handling
 *					null-terminated strings (where the source length is
 *					meaningless) becomes a complex problem.
 *				kwk	Constrain custom translit op by value found in new
 *					numCustomTransTables field of int'l globals.
 *
 ***********************************************************************/
Err TxtTransliterate(const Char* iSrcTextP,
					UInt16 iSrcLength,
					Char* oDstTextP,
					UInt16* ioDstLength,
					TranslitOpType iTransOp)
{
	ErrNonFatalDisplayIf(iSrcTextP == NULL, "NULL string parameter");
	ErrNonFatalDisplayIf(ioDstLength == NULL, "NULL length ptr parameter");
	
	// Find the appropriate table to use for the transliteration.
	TextTableP tableP = NULL;
	UInt16 transOp = (UInt16)iTransOp & ~translitOpPreprocess;
	if (transOp < kIntlMaxStdTransliterations)
	{
		tableP = GIntlData->stdTransliterations[transOp];
	}
	else if (transOp < translitOpCustomBase + GIntlData->numCustomTransliterations)
	{
		tableP = GIntlData->customTransliterations[transOp - translitOpCustomBase];
	}
	
	if (tableP == NULL)
	{
		return(txtErrUknownTranslitOp);
	}
	
	Boolean preprocess = (iTransOp & translitOpPreprocess) != 0;
	ErrNonFatalDisplayIf(!preprocess && (oDstTextP == NULL), "Null dest pointer");
	
	// If the table is one that shouldn't be used with TTProcessText, then we'll
	// need to call TxtGetNextChar and TTConvertValue.
	Boolean useFSM = TTProcessTextSupported(tableP);
	
	TTProcessArgsType fsmArgs;
	fsmArgs.input.text.iTextP = iSrcTextP;
	fsmArgs.input.text.iLength = iSrcLength;
	fsmArgs.input.text.ioOffset = 0;
	fsmArgs.iTableP = tableP;
	
	UInt16 dstOffset = 0;
	Boolean firstTime = true;
	Boolean keepGoing = false;
	Err result = errNone;
	
	// Start looping through the text, calling TTProcessText until either
	// (a) we've reached the end of the source text (either by max offset
	// or hitting a terminating null byte) AND TTProcessText returned
	// true (no pending chars to return), or (b) the destination buffer is full.
	while (((fsmArgs.input.text.ioOffset < iSrcLength) 
		 && (fsmArgs.input.text.iTextP[fsmArgs.input.text.ioOffset] != '\0'))
		|| keepGoing)
	{
		if (useFSM)
		{
			Int32 savedOffset = fsmArgs.input.text.ioOffset;
			keepGoing = TTProcessText(&fsmArgs, firstTime);
			firstTime = false;
			
			// If the character can't be mapped, then pretend like it's the same
			// as the input character, and advance the offset ourselves.
			if (fsmArgs.oResult == kUnmappedCharCode)
			{
				WChar curChar;
				fsmArgs.input.text.ioOffset = savedOffset;
				fsmArgs.input.text.ioOffset += TxtGetNextChar(	fsmArgs.input.text.iTextP,
																fsmArgs.input.text.ioOffset,
																&curChar);
				fsmArgs.oResult = curChar;
				
				// Since we're mucking with the FSM record, reset everything.
				keepGoing = false;
				firstTime = true;
			}
		}
		else
		{
			WChar curChar;
			fsmArgs.input.text.ioOffset += TxtGetNextChar(	fsmArgs.input.text.iTextP,
															fsmArgs.input.text.ioOffset,
															&curChar);
			fsmArgs.oResult = TTConvertValue(curChar, fsmArgs.iTableP);
			
			// If the character can't be mapped, then pretend like it's the same as
			// the input character (we wind up doing a copy).
			if (fsmArgs.oResult == kUnmappedCharCode)
			{
				fsmArgs.oResult = curChar;
			}
		}
		
		// We might have read past the end of the source string.
		if (fsmArgs.input.text.ioOffset > iSrcLength)
		{
			result = txtErrTranslitUnderflow;
			break;
		}
		
		WChar curChar = fsmArgs.oResult;
		UInt16 charSize = TxtCharSize(curChar);
		if (!preprocess)
		{
			if (dstOffset + charSize > *ioDstLength)
			{
				result = txtErrTranslitOverflow;
				break;
			}
			// If our output buffer is the same as our input buffer,
			// and what we're writing will over-write what we've still
			// got to process, and we're not at the end of the input
			// data, then return an error.
			else if ((oDstTextP == iSrcTextP)
				  && (oDstTextP + dstOffset + charSize > iSrcTextP + fsmArgs.input.text.ioOffset)
				  && (fsmArgs.input.text.ioOffset < iSrcLength))
			{
				result = txtErrTranslitOverrun;
				break;
			}
			else
			{
				TxtSetNextChar(oDstTextP, dstOffset, curChar);
			}
		}
		
		dstOffset += charSize;
	}

	// Update the resulting destination size.
	*ioDstLength = dstOffset;
	
	return(result);
} // TxtTransliterate


/***********************************************************************
 *
 * FUNCTION: TxtGetTruncationOffset
 *
 * DESCRIPTION:	Return the appropriate byte position for truncating
 *	<iTextP> such that it is at most <iOffset> bytes long.
 *
 * PARAMTERS:
 *	iTextP	 ->	Ptr to text.
 *	iOffset	 -> Byte offset into text.
 *
 * RETURNS:
 *	Offset to use for truncation.
 *
 * HISTORY:
 *	??/??/98	kwk	Created by Ken Krugler.
 *	06/01/00	kwk	Modified to use table.
 *
 ***********************************************************************/
UInt32 TxtGetTruncationOffset(const Char* iTextP, UInt32 iOffset)
{
	if (iOffset == 0)
	{
		return(0);
	}
	
	// DOLATER kwk - decide if we should check for a char bounds
	// table, and if none exists, assume single-byte. Or we could
	// key off the single-byte system flag in the intlFlags. In
	// general, should we do that to decide about faster processing,
	// versus checking for the presence of tables?
	UInt32 charStart, charEnd;
	TxtCharBounds(iTextP, iOffset, &charStart, &charEnd);
	return(charStart);
} // TxtGetTruncationOffset


/***********************************************************************
 *
 * FUNCTION: TxtWordBounds
 *
 * DESCRIPTION:	Find the bounds of the word that contains the character
 *	at <iOffset>. Return the offsets in <*oWordStart> and <*oWordEnd>.
 *	Return true if the word we found was not empty & not a delimiter
 *	(first char attribute not equal to space or punct).
 *
 * PARAMTERS:
 *	iTextP		 ->	Ptr to text.
 *	iLength		 ->	Length of text in bytes.
 *	iOffset		 -> Byte offset into text.
 *	oWordStart	<-	Byte offset to beginning of word that contains iOffset.
 *	oWordEnd	<-	Byte offset to end of word.
 *
 * RETURNS:
 *	True if word containing <iOffset> is non-punctuation (a 'real' word).
 *
 * HISTORY:
 *	??/??/98	kwk	Created by Ken Krugler.
 *	09/03/99	kwk	Hacked it up to treat "don't" as a word.
 *	06/13/00	CS	Rewrote to make use of TextTable.
 *	06/19/00	CS	Now use TTConvertValue to classify characters, and
 *					no longer rely on wordEndClassTable to calculate the
 *					length of each character (i.e., call GetNextCharacter
 *					instead.)
 *	07/04/00	CS	Fixed bug where I was setting *wordStart to *wordEnd
 *					before I had set up *wordEnd.  How did this ever
 *					work before?
 *	07/19/00	kwk	When using the real word table, return result != 0,
 *					versus just result, so that we give back a well-formed
 *					boolean value in case the table returns something > 1.
 *	08/16/00	CS	Initialize oWordStart to iOffset, in case we hit a
 *					null right away going left.
 *				CS	Don't do the funky "start again from the end of the
 *					word we found" if we didn't find any word at all
 *					(i.e., we ran into a null right away).
 *	09/20/00	kwk	If there's no "real word" table, and we're walking the
 *					found word to see if it has any punctuation, process
 *					it one character at a time, not one byte at a time.
 *
 ***********************************************************************/

Boolean
TxtWordBounds(		const
					Char*			iTextP,
					UInt32			iLength,
					UInt32			iOffset,
					UInt32*			oWordStart,
					UInt32*			oWordEnd)
{
	TTProcessArgsType	breakArgs;
	Boolean			firstTime = true;
	Boolean			keepGoing;
	Int32			curCharOffset = iOffset;
	WChar			curChar;
	
	ErrNonFatalDisplayIf(iTextP == NULL, "NULL string parameter");
	ErrNonFatalDisplayIf((oWordStart == NULL) || (oWordEnd == NULL), "NULL result ptr");

	/* If we're at the end of the text, we didn't find a word.
	*/
	if	(	(iLength == 0)
		||	(iOffset >= iLength)) {
		*oWordStart = *oWordEnd = iLength;
		return(false);
	}
	
	/* Assume that the word starts at the offset passed in (just in case there's
	a null or the beginning of the text immediately before it).
	*/
	*oWordStart = iOffset;

	/* If we're already at the beginning of the text, then there's no reason to
	search left; the word starts right here.
	*/
	if (iOffset == 0) {
	
	/* Otherwise, if we have a separate character classifier table for searching
	left, feed its output tokens to the break table, whose oResult tells us
	whether to save the current offset as the tentative break position.
	*/
	} else if (GIntlData->wordStartClassTable) {
	
		/* After each character is classified by the classifier, its character
		classification (token) is then fed to a second table that looks for the
		start of the word.
		*/
		breakArgs.iTableP = GIntlData->wordStartBreakTable;

		/* Search left from iOffset, looking for the start of the word
		*/
		do {
		
			/* Grab the next character to the left of the offset and classify it
			*/
			curCharOffset -= TxtGetPreviousChar(iTextP, curCharOffset, &curChar);
			breakArgs.input.token.iToken
				= TTConvertValue(curChar, GIntlData->wordStartClassTable);
		
			/* Pass the character classification token to the word start table.
			If it returns 1 in the accumulator, then this character becomes part
			of the word, so move the starting offset before it.
			*/
			keepGoing =	(	TTProcessToken(&breakArgs, firstTime)
						&&	(curCharOffset > 0));
			if (breakArgs.oResult) {
				*oWordStart = curCharOffset;
			}
			firstTime = false;
			
		} while (keepGoing);
	
	/* No separate tokenizer, so we let the break table do all the work in a
	single call, leaving the offset at the break position for us.
	*/
	} else {
		breakArgs.input.text.iTextP = iTextP;
		breakArgs.input.text.iLength = iLength;
		breakArgs.input.text.ioOffset = iOffset;
		breakArgs.iTableP = GIntlData->wordStartBreakTable;
		keepGoing = TTProcessText(&breakArgs, true);
		ErrNonFatalDisplayIf(	keepGoing,
								"Word start break table did not terminate");
		*oWordStart = breakArgs.input.text.ioOffset;
	}
	
	/* If we have a separate character classifier table for searching right,
	feed its output tokens to the break table, whose oResult tells us whether
	to save the current offset as the tentative break position.
	*/
	if (GIntlData->wordEndClassTable) {
		
		/* Again, after each character is classified by the classifier, its
		classification (token) is then fed to a second table that looks for the
		end of the word.
		*/
		breakArgs.iTableP = GIntlData->wordEndBreakTable;

		/* Search right from oWordStart, looking for the end of the word
		*/
		firstTime = true;
		curCharOffset = *oWordStart;
		do {
			UInt32		curCharSize;
		
			/* Grab the next character to the right of the offset and classify it
			*/
			curCharSize = TxtGetNextChar(iTextP, curCharOffset, &curChar);
			breakArgs.input.token.iToken
				= TTConvertValue(curChar, GIntlData->wordEndClassTable);
			
			/* Pass the character classification token to the word end table.
			If it returns 1 in the accumulator, then the previous character
			becomes part of the word, so move the ending offset between the two.
			*/
			keepGoing = TTProcessToken(&breakArgs, firstTime);
			if (breakArgs.oResult) {
				*oWordEnd = curCharOffset;
			}
			firstTime = false;
			curCharOffset += curCharSize;
			
			/* If we run off the end of the string, then the word ends there.
			*/
			if	(	(keepGoing)
				&&	(curCharOffset >= iLength)) {
				*oWordEnd = iLength;
				keepGoing = false;
			
			/* Otherwise, if the word end has been found, then make sure it
			includes the char at the original offset.  If not (and we found
			anything at all), move the word start to the end of the word we
			just found, and begin searching for the end once again.
			*/
			} else if	(	(!keepGoing)
						&&	(*oWordEnd <= iOffset)
						&&	(*oWordEnd > *oWordStart)) {
				*oWordStart = *oWordEnd;
				curCharOffset = *oWordStart;
				firstTime = true;
				keepGoing = true;
			}
		} while (keepGoing);

	/* No separate tokenizer, so we let the break table do all the work, leaving
	the offset at the break position for us.
	*/
	} else {
		breakArgs.input.text.iTextP = iTextP;
		breakArgs.input.text.iLength = iLength;
		breakArgs.iTableP = GIntlData->wordEndBreakTable;
		
		do {

			/* Search from the start of the word for the next break position.
			*/
			breakArgs.input.text.ioOffset = *oWordStart;
			keepGoing = TTProcessText(&breakArgs, true);
			ErrNonFatalDisplayIf(	keepGoing,
									"Word end break table did not terminate");
									
			/* If the word contains the character at the original offset, then
			we found the end of the correct word.
			*/
			if (breakArgs.input.text.ioOffset > iOffset) {
				*oWordEnd = breakArgs.input.text.ioOffset;
			
			/* Otherwise, move the start to the end of the word we just found,
			and begin searching for the end once again.
			*/
			} else {
				*oWordStart = breakArgs.input.text.ioOffset;
			}
			
		} while (breakArgs.input.text.ioOffset <= iOffset);
	}

	/* Complain if non-empty word bounds don't include the original offset
	*/
	ErrNonFatalDisplayIf(	(	(*oWordStart < *oWordEnd)
							&&	(	(*oWordStart > iOffset)
								||	(*oWordEnd <= iOffset))),
						"Word bounds don\'t include original offset");
	
	/* If we've got a table specifying whether something is a "real" word,
	pass our word to it and return the result.
	*/
	if (GIntlData->realWordTable) {
		TTProcessArgsType	realWordArgs;

		realWordArgs.input.text.iTextP = iTextP;
		realWordArgs.input.text.iLength = *oWordEnd;
		realWordArgs.input.text.ioOffset = *oWordStart;
		realWordArgs.iTableP = GIntlData->realWordTable;
		keepGoing = TTProcessText(&realWordArgs, true);
		ErrNonFatalDisplayIf(keepGoing, "Real word table did not terminate");
		return(realWordArgs.oResult != 0);
	
	/* Otherwise, it's a "real" word if it contains a non-delimiter char.
	*/
	} else {
		UInt32 offset = *oWordStart;
		while (offset < *oWordEnd) {
			WChar curChar;
			offset += TxtGetNextChar(iTextP, offset, &curChar);
			if	(	(	TxtCharAttr(curChar)
					&	(charAttrSpace | _PU))
				==	0) {
				return(true);
			}
		}
		return(false);
	}
} // TxtWordBounds


/***********************************************************************
 *
 * FUNCTION: TxtGetWordWrapOffset
 *
 * DESCRIPTION:	Return the offset of the first break position (for text
 *	wrapping) that occurs at or before <iOffset> in <iTextP>. Note that
 *	this routine will also add trailing spaces and a trailing linefeed
 *	to the break position, thus the result could be greater than <iOffset>.
 *
 * PARAMTERS:
 *	iTextP	 ->	Ptr to text.
 *	iOffset	 -> Offset to start searching from (this is typically
 *				the offset of the first character that did not fit
 *				on the line.
 *
 * RETURNS:
 *	Offset to use for end of line (wrap offset).
 *
 * HISTORY:
 *	05/26/00	kwk	Created by Ken Krugler.
 *	05/29/00	kwk	Check for null at <iOffset>, return immediately.
 *	06/08/00	CS	Extended TTProcessArgsType for use by TTProcessToken.
 *	06/27/00	CS	Rewrote to handle separate wordWrapClassTable, if
 *					present.
 *	07/19/00	kwk	Check for case of no break position on line.
 *	10/31/00	kwk	For the no-break case, immediately set curCharOffset
 *					to iOffset before we start looking for trailing special
 *					characters, so a line of 'Sssss' + linefeed, where the
 *					linefeed is the first char that doesn't fit, wraps OK.
 *
 ***********************************************************************/

UInt32
TxtGetWordWrapOffset(const
					Char*			iTextP,
					UInt32			iOffset)
{
	TTProcessArgsType	breakArgs;
	UInt32			curCharOffset = iOffset;

	ErrNonFatalDisplayIf(iTextP == NULL, "Null parameter");

	/* If we're at the end of the string, then that's the break location.
	*/
	if (iTextP[iOffset] == chrNull)
	{
		return(iOffset);
	}
	
	/* First walk the offset right by one character, since we'll be moving
	through the text right to left, and we want to start with the first
	character after <iOffset>.
	*/
	curCharOffset = iOffset + TxtNextCharSize(iTextP, iOffset);
	
	/* If we have a separate character classifier table, feed its output tokens
	to the break table.
	*/
	if (GIntlData->wordWrapClassTable) {
		Boolean			firstTime = true;
		Boolean			keepGoing;
		WChar			curChar;
		UInt16			curCharSize;
	
		/* After each character is classified by the classifier, its character
		classification (token) is then fed to a second table that looks for the
		start of the word.
		*/
		breakArgs.iTableP = GIntlData->wordWrapBreakTable;
		
		/* Search left from curCharOffset, looking for a break character
		*/
		do {
		
			/* Grab the next character to the left of the offset and classify it
			*/
			curCharSize = TxtGetPreviousChar(iTextP, curCharOffset, &curChar);
			curCharOffset -= curCharSize;
			breakArgs.input.token.iToken
				= TTConvertValue(curChar, GIntlData->wordWrapClassTable);
		
			/* Pass the character classification token to the word start table.
			*/
			keepGoing =	TTProcessToken(&breakArgs, firstTime);
			firstTime = false;
			
		} while (keepGoing && (curCharOffset > 0));
		
		/* If we terminated because we reached the beginning of the string,
		use the initial break position (passed to us) as the break offset.
		*/
		if (keepGoing && (curCharOffset == 0)) {
			curCharOffset = iOffset;
		
		/* The breakArgs.oResult contains the number of characters to add to
		the break position (usually one).
		*/
		} else if (breakArgs.oResult) {
			curCharOffset += curCharSize;
			while (--breakArgs.oResult) {
				curCharOffset += TxtPreviousCharSize(iTextP, curCharOffset);
			}
		}

	/* No separate tokenizer, so we let the break table do all the work in a
	single call.  Here breakArgs.oResult contains the number of bytes to
	add to the break position (again, usually just one).
	*/
	} else {
		breakArgs.input.text.iTextP = iTextP;
		breakArgs.input.text.iLength = kMaxUInt32;
		breakArgs.input.text.ioOffset = curCharOffset;
		breakArgs.iTableP = GIntlData->wordWrapBreakTable;
		TTProcessText(&breakArgs, true);
		curCharOffset = breakArgs.input.text.ioOffset + breakArgs.oResult;
		
		/* If we didn't find a break position, use the original offset.
		*/
		if (curCharOffset == 0) {
			curCharOffset = iOffset;
		}
	}
		
	/* Grab a following tab
	*/
	if (iTextP[curCharOffset] == tabChr) {
		curCharOffset += 1;
	}
	
	/* Grab all of the following spaces
	*/
	while (iTextP[curCharOffset] == chrSpace) {
		curCharOffset += 1;
	}
	
	/* Grab a following linefeed
	*/
	if (iTextP[curCharOffset] == chrLineFeed) {
		curCharOffset += 1;
	}
	
	return(curCharOffset);
} // TxtGetWordWrapOffset
