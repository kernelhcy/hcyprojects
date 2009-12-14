/******************************************************************************
 *
 * Copyright (c) 1994-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: StringMgr.c
 *
 * Release: 
 *
 * Description:
 *		String manipulation functions
 *
 * History:
 *		11/09/94 RM		Created by Ron Marianetti
 *
 *****************************************************************************/

#include <PalmTypes.h>

// public system includes
#include <StringMgr.h>
#include <ErrorMgr.h>
#include <MemoryMgr.h>
#include <TextMgr.h>
#include <unix_stdarg.h>

// private system includes
#include "SysTrapsFastPrv.h"
#include "TextPrv.h"			// For kTxtMaxAlwaysSingleByte

// DOLATER kwk - move this to PalmTypes.h???
#define	kMaxUInt16	0xFFFF

/************************************************************
 * Private types
 *************************************************************/

typedef enum 
{
	sizeShort,
	sizeLong,
	sizeInt
} SizeModifierType;


/************************************************************
 *
 *  FUNCTION: StrCopy
 *
 *  DESCRIPTION: Copies 1 string to another
 *
 *  PARAMETERS: 2 string pointers
 *
 *  RETURNS: pointer to dest string
 *
 *  CREATED: 11/09/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Char* StrCopy(Char* dst, const Char* src)
{
	Char*	tmp = dst;

	ErrFatalDisplayIf((dst == NULL) || (src == NULL), "NULL string passed");
	
	while (*src)
	{
		*tmp++ = *src++;
	}
	
	*tmp = 0;
	return(dst);
}



/************************************************************
 *
 * FUNCTION:	StrNCopy
 *
 * DESCRIPTION: Copies up to N characters from srcP string to
 *		dstP string. It does NOT null terminate if N characters
 *		were copied, see history comment.
 *
 * PARAMETERS:
 *		dstP - destination string
 *		srcP - source string
 *		n   - max # of bytes to copy from srcP string
 *
 * RETURNS: pointer to dest string
 *
 *	HISTORY:
 *		05/06/96	ron	Created by Ron Marianetti.
 *		10/24/96	rsf	Fill all of dest
 *		12/18/96	vmk	Removed ec code -- it was trashing data during
 *							"copy in place" operations
 *		08/12/97	ADH	Modified test for null termination to comply with
 *							ANSI specifications.
 *		05/13/00	kwk	Made it work for all char encodings, not just Latin.
 *		11/02/00	kwk	Cast src to unsigned before comparing to kTxtMaxNeverMultiByte.
 *		01/02/01	rsf	Fix bug reading from srcP when n is 0.  Found by Scott Maxwell.
 *
 *************************************************************/
Char* StrNCopy(Char* dstP, const Char* srcP, Int16 n)
{
	Char*	tmp = dstP;
	const Char* src = srcP;
#if (ERROR_CHECK_LEVEL == ERROR_CHECK_FULL)
	Char*	endDestP = dstP + n;
#endif		
	
	// Check for err
	ErrFatalDisplayIf((dstP == NULL) || (srcP == NULL), "NULL string passed");
	
	// Copy N characters from src string
	while (n > 0 && *src)
	{
		*tmp++ = *src++;
		--n;
	}
	
	// Terminate dest string if there's space.
	if (n > 0) 
	{
		*tmp = '\0';
	}
	else if (*(UInt8*)src > kTxtMaxNeverMultiByte)
	{
		// We stopped copying because we had moved N bytes, but we
		// haven't yet hit the end of the source string. This means
		// that the last character copied might be a partial character,
		// if the first uncopied byte could be part of a mult-byte char.
		// We need to find the bounds of the last character in the
		// source string.
		UInt32 offset = src - srcP;
		UInt32 charStart, charEnd;
		TxtCharBounds(srcP, offset, &charStart, &charEnd);
		
		// If we were in the middle of a character, we want to null-
		// terminate the destination string just in case the caller
		// jams a null at dstP[n] in an attempt to terminate the
		// string, as otherwise we would still wind up with an invalid
		// (partial) character at the end. This also sets up properly
		// for the subsequent error check buffer-filling code.
		if (charStart < offset)
		{
			tmp = dstP + charStart;
			*tmp = '\0';
		}
	}
	
	// Fill up the unused dest buffer with bytes to reveal errors.
	// Only copy over the remaining unused bytes.  This works correctly when
	// the src and dest overlap.
#if (ERROR_CHECK_LEVEL == ERROR_CHECK_FULL)
	tmp++; // Advance past null
	if (tmp < endDestP)
	{
		// In debug ROMs MemSet will complain if the unused dest buffer was cleared before.
		// This is common when copying many strings.  Add some extra code to work around it.
		tmp[0] = 0;
		
		// Fill up the destination as much as allowed with a value to reveal errors.
		MemSet(tmp, endDestP - tmp, 0xfe);
	}
#endif
	
	return(dstP);
} // StrNCopy



/************************************************************
 *
 *  FUNCTION: StrCat
 *
 *  DESCRIPTION: Concatenates 1 string to another
 *
 *  PARAMETERS: 2 string pointers
 *
 *  RETURNS: pointer to dest string
 *
 *  CREATED: 11/09/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Char * StrCat(Char * dst, const Char* src)
{
	Char *	tmp = dst;

	// Check for err
	ErrNonFatalDisplayIf(dst == NULL || src == NULL, "NULL string passed");
	ErrNonFatalDisplayIf(dst == src, "dest and source can't be the same");
	
	tmp += StrLen(dst);

	while(*src)
		*tmp++ = *src++;
	*tmp = 0;
	return dst;
}




/************************************************************
 *
 * FUNCTION: StrNCat
 *
 * DESCRIPTION: Concatenates 1 string to another clipping the
 *		destination string to a max of N characters (including null
 *		at end).
 *
 * PARAMETERS:
 *		dstP	<-> String to receive catenated result.
 *		srcP	 -> String to catenate to the end of dstP.
 *		n		 -> Max length of resulting string, including null byte.
 *
 * RETURNS:	pointer to dest string
 *
 * HISTORY:
 *		11/09/94	ron	Created by Ron Marianetti
 *		10/24/96	rsf	Added ec code
 *		12/18/96	vmk	Removed ec code -- it was trashing data during
 *							"copy in place" operations
 *		05/13/00	kwk	Made it work for all char encodings, not just Latin.
 *		11/02/00	kwk	Cast src to unsigned before comparing to kTxtMaxNeverMultiByte.
 *		01/02/01	rsf	Fix bug reading from srcP when n is 0.
 *
 *************************************************************/
Char* StrNCat(Char* dstP, const Char* srcP, Int16 n)
{
	Char*			tmp;
	const Char*	savedSrcP;
	Int16			len;
#if (ERROR_CHECK_LEVEL == ERROR_CHECK_FULL)
	Char*			endDstP;
#endif		
	
	
	// Check for err
	ErrFatalDisplayIf((dstP == NULL) || (srcP == NULL), "NULL string passed");
	ErrNonFatalDisplayIf(dstP == srcP, "dest and source can't be the same");
	
	// If we already are full, we can quickly return. This means that
	// following code knows there's enough space to copy at least one
	// byte of data plus a terminating null.
	len = StrLen(dstP);
	if (len >= n - 1) {
		return(dstP);
	}
	
	// Do the cat. Note that we leave space for the terminating null, thus
	// N is total size limit of the dst string, including the null.
	tmp = dstP + len;
	savedSrcP = srcP;
	while ((len < (n - 1)) && *srcP)
	{
		*tmp++ = *srcP++;
		len++;
	}
	
	// Null terminate the string if there's room
	if (len < n)
	{
		*tmp = '\0';
	}
	
	// Make sure we didn't wind up copying over a partial byte. If we
	// still have source data then we must have been limited by n, and
	// thus need to check the bounds of the last character copied in the
	// source string. Though this check only needs to be made if the
	// first byte of the remaining source _could_ be part of a multi-
	// byte character.
	
	if (*(UInt8*)srcP > kTxtMaxNeverMultiByte)
	{
		UInt32 charStart, charEnd;
		UInt32 offset = srcP - savedSrcP;
		TxtCharBounds(savedSrcP, offset, &charStart, &charEnd);
		
		if (charStart < offset)
		{
			tmp -= (offset - charStart);
			*tmp = '\0';
		}
	}

	// Fill up the unused dest buffer with bytes to reveal errors.
	// Only copy over the remaining unused bytes.  This works correctly when
	// the src and dest overlap.
#if (ERROR_CHECK_LEVEL == ERROR_CHECK_FULL)
	tmp++;	// skip terminating null
	endDstP = dstP + n;
	if (tmp < endDstP)
	{
		// Make sure that the MemSet call changes at least one value or it can 
		// complain that it didn't change anything and thus didn't need to be called.
		// This is basically extra debug code to help out other debug code.
		tmp[0] = 0;
		
		// Fill up the destination as much as allowed with a value to reveal errors.
		MemSet(tmp, endDstP - tmp, 0xfe);
	}
#endif
	
	return(dstP);
} // StrNCat


/************************************************************
 *
 *  FUNCTION: StrLen
 *
 *  DESCRIPTION: Computes the length of a string
 *
 *  PARAMETERS: string pointer
 *
 *  RETURNS: length of string
 *
 *  CREATED: 11/09/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Int16 StrLen(const Char* str)
{
	Int16	i=0;

	// Check for err
	ErrNonFatalDisplayIf(str == NULL, "NULL string passed");
	
	while(*str++)
		i++;

	return i;
}


/************************************************************
 *
 *	FUNCTION: StrCompareAscii
 *
 *	DESCRIPTION: Compares two strings. It assumes that
 *		both strings are 7-bit ASCII, and the comparison is case-
 *		sensitive. Use this to do a fast, simple byte-by-byte 
 *		comparison which is guaranteed to never change.
 *
 *	PARAMETERS: 2 string pointers
 *
 *	RETURNS: 0 if they match, non-zero if not
 *				+ if s1 > s2
 *				- if s1 < s2
 *
 *	HISTORY:
 *		11/17/99	kwk	Created by Ken Krugler.
 *		05/14/00	vsm	Renamed from PrvDBNameCompare (and made
 *							public)
 *
 *************************************************************/
Int16 StrCompareAscii(const Char* s1, const Char* s2)
{
	const	UInt8* p1 = (UInt8*)s1;
	const	UInt8* p2 = (UInt8*)s2;
	UInt8 c1, c2;
	
	while ((c1 = *p1++) == (c2 = *p2++))
	{
		if (!c1)
		{
			return(0);
		}
	}

	return(c1 - c2);
}


/************************************************************
 *
 * FUNCTION: StrCompare
 *
 * DESCRIPTION: Compares two strings. This should be used
 *		to sort strings but not find them. TxtCompare should
 *		be used in place of this routine if OS >= 3.1.
 *
 * PARAMETERS: 2 string pointers
 *
 *  RETURNS: 0 if they match, non-zero if not
 *				 + if s1 > s2
 *				 - if s1 < s2
 *
 *	HISTORY:
 *		11/09/94	ron	Created by Ron Marianetti & Roger Flores.
 *		05/13/00	kwk	Use TxtCompare.
 *		05/31/00	kwk	Pass kMaxUInt16 vs. StrLen() to TxtCompare,
 *							since it now handles NULL as end-of-string.
 *
 *************************************************************/
Int16 StrCompare(const Char* s1, const Char* s2)
{
	return(TxtCompare(s1, kMaxUInt16, NULL, s2, kMaxUInt16, NULL));
}


/************************************************************
 *
 *	FUNCTION: StrNCompareAscii
 *
 *	DESCRIPTION: Compares two strings, up to a maximum of
 *		n bytes. It assumes that both strings are 7-bit ASCII,
 *		and the comparison is case-sensitive.
 *
 *	PARAMETERS: 2 string pointers, max length to compare.
 *
 *	RETURNS: 0 if they match, non-zero if not
 *				+ if s1 > s2
 *				- if s1 < s2
 *
 *	HISTORY:
 *		08/18/00	kwk	Created by Ken Krugler.
 *
 *************************************************************/
Int16 StrNCompareAscii(const Char* s1, const Char* s2, Int32 n)
{
	ErrNonFatalDisplayIf((s1 == NULL) || (s2 == NULL), "NULL string passed");
	
	while (n--)
	{
		UInt8 c1 = *s1++;
		UInt8 c2 = *s2++;
		Int16 result = c1 - c2;
		
		if (result)
		{
			return(result);
		}
		else if (c1 == chrNull)
		{
			return(0);
		}
	}
	
	return(0);
} // StrNCompareAscii


/************************************************************
 *
 * FUNCTION: StrNCompare
 *
 * DESCRIPTION: Compares two strings out to N characters.
 *		TxtCompare should be used in place of this routine if
 *		OS >= 3.1.
 *
 * PARAMETERS: 2 string pointers, max length to compare.
 *
 * RETURNS: 0 if they match, non-zero if not
 *				+ if s1 > s2
 *				- if s1 < s2
 *
 *	HISTORY:
 *		07/18/96	rsf	Created by Roger Flores
 *		05/13/00	kwk	Use TxtCompare.
 *
 *************************************************************/
Int16 StrNCompare(const Char* s1, const Char* s2, Int32 n)
{
	UInt32 s1Len;
	UInt32 s2Len;
	UInt32 charStart;
	UInt32 charEnd;

	// This will slow down comparison (calculating the string length
	// means testing every byte), but currently it's our only option.
	// DOLATER kwk - figure out if TxtCompare can handle being passed
	// an offset to the middle of a character...if so, then we don't
	// need the char bounds check. If so then we could also skip the
	// StrLen calls.
	
	s1Len = StrLen(s1);
	s2Len = StrLen(s2);

	// Now make sure that n falls on an inter-char boundary for both
	// strings..if not, prune it back.
	if (n < s1Len)
	{
		TxtCharBounds(s1, n, &charStart, &charEnd);
		s1Len = charStart;
	}
	
	if (n < s2Len)
	{
		TxtCharBounds(s2, n, &charStart, &charEnd);
		s2Len = charStart;
	}
	
	return(TxtCompare(s1, s1Len, NULL, s2, s2Len, NULL));
} // StrNCompare


/************************************************************
 *
 * FUNCTION: StrCaselessCompare
 *
 * DESCRIPTION: Compares two strings with case and accent
 *		insensitivity. Use TxtCaselessCompare if OS >= 3.1
 *
 *	PARAMETERS: 2 string pointers
 *
 * RETURNS: 0 if they match, non-zero if not
 *				+ if s1 > s2
 *				- if s1 < s2
 *
 *	HISTORY:
 *		11/09/94	rsf	Created by Roger Flores
 *		05/13/00	kwk	Use TxtCaselessCompare.
 *		05/31/00	kwk	Pass kMaxUInt16 vs. StrLen() to TxtCaselessCompare,
 *							since it now handles NULL as end-of-string.
 *
 *************************************************************/
Int16 StrCaselessCompare(const Char* s1, const Char* s2)
{
	return(TxtCaselessCompare(s1, kMaxUInt16, NULL, s2, kMaxUInt16, NULL));
}


/************************************************************
 *
 * FUNCTION: StrNCaselessCompare
 *
 * DESCRIPTION: Compares two strings out to N characters
 *	with case and accent insensitivity.
 *
 * PARAMETERS: 2 string pointers
 *
 * RETURNS: 0 if they match, non-zero if not
 *				 + if s1 > s2
 *				 - if s1 < s2
 *
 * HISTORY:
 *		08/13/96	rsf	Created by Roger Flores.
 *		05/13/00	kwk	Modified to use TxtCaselessCompare, check
 *							for <n> in the middle of a character.
 *
 *************************************************************/
Int16 StrNCaselessCompare(const Char* s1, const Char* s2, Int32 n)
{
	UInt32 s1Len;
	UInt32 s2Len;
	UInt32 charStart;
	UInt32 charEnd;

	// This will slow down comparison (calculating the string length
	// means testing every byte), but currently it's our only option.
	// DOLATER kwk - figure out if TxtCaselessCompare can handle being passed
	// an offset to the middle of a character...if so, then we don't
	// need the char bounds check. If so then we could also skip the
	// StrLen calls. Another option would be to check the IntlMgr
	// flag for simple single-byte, and skip the bounds check, otherwise
	// skip the length calc, but still do the bounds check.
	
	s1Len = StrLen(s1);
	s2Len = StrLen(s2);

	// Now make sure that n falls on an inter-char boundary for both
	// strings..if not, prune it back.
	if (n < s1Len)
	{
		TxtCharBounds(s1, n, &charStart, &charEnd);
		s1Len = charStart;
	}
	
	if (n < s2Len)
	{
		TxtCharBounds(s2, n, &charStart, &charEnd);
		s2Len = charStart;
	}
	
	return(TxtCaselessCompare(s1, s1Len, NULL, s2, s2Len, NULL));
}


/************************************************************
 *
 *  FUNCTION: StrIToA
 *
 *  DESCRIPTION: convert an integer to ascii
 *
 *  PARAMETERS: Integer to convert and string pointer to store results
 *
 *  RETURNS: s
 *
 *  CREATED: 11/09/94 
 *
 *  BY: Roger Flores
 *
 *  REVISION HISTORY:
 *			Name				Date		Description
 *			----				----		-----------
 *			roger				4/2/96	MemHandle negative numbers
 *			eric lapuyade	10/5/99	use internal unsigned variable to allow for
 *											smallest possible negative number (bug no 15914)
 *
 *************************************************************/
Char * StrIToA(Char * s, Int32 i)
{
	Char * s1;
	Char * s2;
	Char c1;
	Char c2;
	UInt32 ui;


	// Check for err
	ErrNonFatalDisplayIf(s == NULL, "NULL string passed");

	
	// Handle negative numbers
	if (i < 0)
		{
		*s = '-';
		ui = -i;
		
		s1 = s + 1;	// place numbers after the negative sign we put
		}
	else
		{
		ui = i;
		s1 = s;		// place numbers after the start
		}
		
		
	// output the string in reverse order
	s2 = s1;
	do {
		*s2++ = ui % 10 + '0';
		ui /= 10;								// CSE should reuse the above division
	} while (ui > 0);
	*s2 = '\0';
	s2--;
	
	
	// Reverse the string
	while(s1 < s2) 
		{
		c1 = *s1;
		c2 = *s2;
		
		*s1++ = c2;
		*s2-- = c1;
		}

	return s;
}


/************************************************************
 *
 *  FUNCTION: StrIToH
 *
 *  DESCRIPTION: convert an integer to hexadecimal ascii
 *
 *  PARAMETERS: Integer to convert and string pointer to store results
 *
 *  RETURNS: s
 *
 *  CREATED: 7/4/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Char * StrIToH(Char * s, UInt32 i)
{
	UInt32		mask;
	UInt32		digit;
	UInt16		j;
	
	// Check for err
	ErrNonFatalDisplayIf(s == NULL, "NULL string passed");
	
	mask = 0xF0000000;
	for (j=0; j<8; j++) {
		digit = (i & mask) >> 28;
		if (digit <= 9) 
			s[j] = digit + '0';
		else
			s[j] = digit - 10 + 'A';
			
		i <<= 4;
		}
	s[j] = 0;

	return s;
}


/************************************************************
 *
 *  FUNCTION: StrLocalizeNumber
 *
 *  DESCRIPTION: Localize a number by converting from US number
 *  notation to a localized version by replacing any thousand
 *  or decimal separators.
 *
 *  PARAMETERS: s - number asci string to localize
 *					 thousandSeparator - the localized thousand separator
 *					 decimalSeparator - the localized decimal separator
 *
 *  RETURNS: s
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	8/28/96	Initial version
 *			roger	4/29/98	Fixed case when thousandSeparator is '.'
 *
 *************************************************************/
Char * StrLocalizeNumber(Char * s, Char thousandSeparator, Char decimalSeparator)
{
	Char * 	str = s;
	
	
	// Check for err
	ErrNonFatalDisplayIf(s == NULL, "NULL string passed");

	
	while (*str != '\0')
		{
		// Check for thousandSeparator
		if (*str == ',') 
			*str = thousandSeparator;
		
		// Check for decimalSeparator
		else if (*str == '.') 
			*str = decimalSeparator;
		
		// Look at the next char.
		str++;
		};
		
		
	return s;
}


/************************************************************
 *
 *  FUNCTION: StrDelocalizeNumber
 *
 *  DESCRIPTION: Delocalize a number by converting from a 
 *  localized number notation to a US version by replacing 
 *  any thousand or decimal separators.
 *
 *  PARAMETERS: s - number asci string to localize
 *					 thousandSeparator - the localized thousand separator
 *					 decimalSeparator - the localized decimal separator
 *
 *  RETURNS: s
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	8/28/96	Initial version
 *			adamh	8/12/97	Corrected functionality
 *			daf	2/9/99	Fixed case when decimalSeparator is ','
 *
 *************************************************************/
Char * StrDelocalizeNumber(Char * s, Char thousandSeparator, Char decimalSeparator)
{
	Char * 	str = s;
	
	
	// Check for err
	ErrNonFatalDisplayIf(s == NULL, "NULL string passed");

	
	while (*str != '\0')
		{
		if (*str == thousandSeparator)		// Check for thousandSeparator
			*str = ',';
		
		else if (*str == decimalSeparator)	// Check for decimalSeparator
			*str = '.';
		
		// Look at the next char.
		str++;
		};
		
		
	return s;
}


/***********************************************************************
 *
 * FUNCTION:	StrChr
 *
 * DESCRIPTION: This routine looks for a character within a string. It
 *		behaves the same as the standard C library function strchr, except
 *		that it works correctly with multi-byte character encodings.
 *
 * PARAMETERS:
 *		str  - string to search
 *		chr  - the character to search for
 *
 * RETURNED:	pointer to the first occurance of character in str, or NULL
 *		if not found.
 *
 * HISTORY:
 *		08/18/95	ron	Created by Ron Marianetti
 *		05/07/96	rsf	Fixed to MemHandle searching for '\0'
 *		10/24/96	rsf	Check chr
 *		08/12/97	ADH	Revised Algorithm
 *		08/26/98	kwk	Changed chr param from Int16 to WChar. Fixed up
 *							sign extension problem.
 *		05/13/00	kwk	Made it work for all char encodings, not just Latin.
 *
 ***********************************************************************/
Char* StrChr(const Char* str, WChar chr)
{
	Char c = (Char)chr;
	register const Char* srcP = str;
	
	ErrNonFatalDisplayIf(str == NULL, "NULL string passed");

	// Correct for sign extension. This will happen if the caller passes
	// a single byte >= 0x80, which the compiler will sign extend.
	if (chr >= 0xFF80)
	{
		ErrNonFatalDisplay("Sign extended character passed to StrChr");
		chr = chr & 0x00FF;
	}
	
	// See if we need to do the slower (multi-byte) search case.
	if (chr > kTxtMaxAlwaysSingleByte)
	{
		Char buffer[maxCharBytes + 1];
		UInt16 len = TxtSetNextChar(buffer, 0, chr);
		if (len > 1)
		{
			buffer[len] = '\0';
			return(StrStr(str, buffer));
		}
	}
	
	// Be sure to handle chr == '\0' correctly
	do
	{
		if (*srcP == c)
		{
			// If the character we're searching for could be part of a
			// multi-byte character, then we need to do an extra check
			// to make sure we didn't find part of a multi-byte character.
			// Since most people use StrChr to search for tabs, returns,
			// etc. this typically never gets executed.
			if (chr > kTxtMaxNeverMultiByte)
			{
				UInt32 charStart, charEnd;
				UInt32 charOffset = srcP - str;
				TxtCharBounds(str, charOffset, &charStart, &charEnd);
				if (charStart == charOffset)
				{
					return((Char*)srcP);
				}
			}
			else
			{
				return((Char*)srcP);
			}
		}
	} while (*srcP++ != 0);
	
	return(NULL);
} // StrChr


/***********************************************************************
 *
 * FUNCTION:	StrStr
 *
 * DESCRIPTION: This routine looks for a substring within a string. It
 *		behaves the same as the standard C library function strstr, except
 *		that it correctly handles multi-byte character encodings.
 *
 * PARAMETERS:
 *		str    - string to search
 *		token  - the string to search for
 *
 * RETURNED:	pointer to the first occurance of token in str, or null
 *		if not found.
 *
 * HISTORY:
 *		08/18/95	ron	Created by Ron Marianetti.
 *		05/13/00	kwk	Made it work for all char encodings, not just Latin.
 *		11/30/00	kwk	Cast token to unsigned before comparing to kTxtMaxNeverMultiByte.
 *
 ***********************************************************************/
Char* StrStr(const Char* str, const Char* token)
{
	register Char * srcP = (Char *)str;
	const Char*	tP;
	const Char* sP;
	
	// Check for err
	ErrNonFatalDisplayIf(str == NULL || token == NULL, "NULL string passed");
	
	// Return null if token is null
	if (*token == '\0')
	{
		return(NULL);
	}
	
	while (*srcP != '\0')
	{
		tP = token;
		sP = srcP;
		
		do
		{
			if (*tP != *sP)
			{
				break;
			}
			
			tP++;
			sP++;
		} while (*tP != '\0');
		
		// We think we have a match. Make sure we didn't start the match
		// in the middle of a character. We only need to do this check if
		// the first byte of token could be in the middle/end of a multi-
		// byte character.
		
		if (*tP == 0)
		{
			if (*(UInt8*)token > kTxtMaxNeverMultiByte)
			{
				UInt32 charStart, charEnd;
				TxtCharBounds(str, srcP - str, &charStart, &charEnd);
				if (str + charStart == srcP)
				{
					return(srcP);
				}
			}
			else
			{
				return(srcP);
			}
		}
		
		srcP++;
	}

	return(NULL);
} // StrStr


/***********************************************************************
 *
 * FUNCTION:    StrAToI
 *
 * DESCRIPTION: This routine converts a string to an integer.  It behaves
 *						the same as the standard C library function atoi. Because
 *						of a bug in the Metrowerks libraries however, we can't link
 *						with the atoi() function in the library when compiling
 *						for the device.
 *
 * PARAMETERS:  str    - string to convert
 *
 * RETURNED:    converted integer
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			ron	8/18/95	Initial Revision
 *			adamh	8/12/97	Added a for Max Int32 Int16 when Error Check Level == Full
 *			art	9/15/97	Fix error checking code.
 *			vmk	1/15/98	Disabled error checking code because it modified function's behavior
 *								and interfered with normal user input testing on debug builds
 *								(with agreement from Ron, Roger)
 *
 ***********************************************************************/
Int32 StrAToI (const Char* str)
{
	Int32 result=0;
	Int16	sign = 1;
	Char	c;
	
	// DOLATER... what is needed here, is a way to signal an overrun error to the caller. vmk 1/15/98

#if 0		// CASTRATION
	#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
		Int32 MaxInt=0x7FFFFFFF;
		Int32 Divisor=1000000000;
		Boolean Risk=0;
		Int16 N=0;
		Int16 MaxDigits=10;
	#endif
#endif
	
	// Check for err
	ErrNonFatalDisplayIf(str == NULL, "NULL string ptr passed");
	
	// First character can be a sign
	c = *str++;
	if (!c) return 0;
	if (c == '+') {sign = 1; c = *str++;}
	else if (c == '-') {sign = -1; c = *str++;}
	
	// Accumulate digits will we reach the end of the string
	while(c)
	   {
		if (c < '0' || c > '9') break;
		result = result * 10 + c -'0';
		c = *str++;
		
#if 0		// CASTRATION
	#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
			if (N++ == MaxDigits)	// ***THIS IS POOR ERROR-CHECKING CODE BECAUSE IT MODIFIES BEHAVIOR	vmk 1/15/98
				break;

			if(MaxInt / Divisor < result)		//If the current number were brought out to maximum length
				Risk = true;						//would it go over maximum size for a Int32 Int16.
			Divisor /= 10;
	#endif
#endif
		}		

#if 0		// CASTRATION
	#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
		ErrNonFatalDisplayIf((Risk && N >=MaxDigits) || result < 0,"Number exceeds maximum value for a long integer");
	#endif
#endif

	return result * sign;
}



/************************************************************
 *
 *	FUNCTION:		StrToLower
 *
 *	DESCRIPTION:	Converts a string to a lower case one.
 *
 *	PARAMETERS: 2 string pointers
 *
 *	RETURNS: pointer to dest string
 *
 *	HISTORY:
 *		11/08/95	rsf	Created by Roger Flores.
 *		10/07/99	kwk	Speed up common case for lower-case 7-bit
 *							ascii, and call TxtTransliterate for other
 *							situations (high ascii, ShiftJIS, etc).
 *
 *************************************************************/
Char* StrToLower(Char* dst, const Char* src)
{
	Char *tmp = dst;
	UInt8 c;

	// Check for err
	ErrNonFatalDisplayIf(dst == NULL || src == NULL, "NULL string passed");
	
	// Optimize for the case of spinning through a bunch of lower-
	// case 7-bit ascii. Once we're into double-byte characters,
	// we have to slow way down to handle lower-casing properly
	// anyway, so this doesn't cost us that much extra as a percentage.
	while ((c = *src++) != chrNull)
		{
		if (c < 'A')
			{
			*tmp++ = c;
			}
		else if (c > 'Z')
			{
			// We assume that any character less than 0x80 can be
			// treated as 7-bit ascii. This isn't strictly true for
			// Shift-JIS, since 0x5C is the yen character, but that
			// can't be lower-cased anyway, so we're OK.
			if (c < 0x80)
				{
				*tmp++ = c;
				}
			else
				{
				// We need to do the slower lower-casing, so set up for
				// the call to TxtTransliterate. We don't know the size
				// of the dst buffer, so make it as big as possible.
				UInt16 dstLength = 0xFFFF;
				
				src--;
				TxtTransliterate(src, StrLen(src), tmp, &dstLength, translitOpLowerCase);
				*(tmp + dstLength) = chrNull;
				return(dst);
				}
			}
		else
			{
			*tmp++ = c + ('a' - 'A');
			}
		}

	*tmp = 0;
	return dst;
}


/******************************************************************************
 *  FUNCTION: PrvAddPadding		<Internal>
 *
 *  DESCRIPTION: Support function vsprintf.  Adds padding to the string that
 *	is passed in.  
 *
 *  PARAMETERS: startOfBlock -Start of buffer containing string to be padded.
 *				dstP		 -End of block for padded string
 *				minimumSize	 -The minimum size of the string plus any additional
 *							 padding.
 *				leftJustify	 -True if the string is to be left justified.  False
 *							 use the default right justification.
 *				zeroPad		 -True if padding should be done with '0's.  False use 
 *							 the default padding, ' '.
 *				sign    	 -The value can be '+', ' ', or 0	
 *
 *  CALLED BY:	StrVPrintF
 *
 *  RETURNS: Char * with padding 
 *
 *  CREATED: 7/2/97
 *
 *  BY: Adam Hampson
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			ADH					Initial Revision
 *			ADH		4/8/99		Added signPresent parameter to determine where
 *								to insert padding.
 *
 *****************************************************************************/


static Char * PrvAddPadding(Char * startOfBlock, Char * dstP, const Int16 minimumSize,
	const Boolean leftJustify, Boolean zeroPad, Char sign)
{	
	Int16 stringSize = StrLen(startOfBlock);
	
	ErrNonFatalDisplayIf( sign && (sign != '+') && (sign != ' '), "illegal sign character specified in PrvAddPadding" );
	
	if (leftJustify && stringSize < minimumSize)		// Left Justified
		{
		MemSet(dstP, minimumSize - stringSize, ' ');
		dstP += minimumSize - stringSize;
		*dstP = '\0';
		}
	else if (!leftJustify && stringSize < minimumSize)	// Right Justified
		{
		
		// If there is a sign present and the calling function as specified padding with
		// zero bytes then do not move the sign character with the rest of the value.
		if ( (sign == '+' || sign == ' ') && zeroPad )
			{
			// If there is a sign present then only MemMove the value and not the sign.
			// The value will be moved minimum size specified for this value minus the
			// actual string size of the value plus the size of the sign (1).
			MemMove(startOfBlock + minimumSize - stringSize + 1, startOfBlock + 1, 
						stringSize - 1);
						
			// MemSet the space between the sign and moved value with the desired
			// padding value.
			MemSet(startOfBlock + 1, minimumSize - stringSize, zeroPad ? '0' : ' ');
			}
		else
			{
			// MemMove the value and any potential sign character to make room for padding
			// characters.  The number of bytes to move is determined by the minimum string
			// specified for the value minus the actual string size of the value.
			MemMove(startOfBlock + minimumSize - stringSize, startOfBlock, stringSize);
			
			// MemSet the space between the sign and moved value with the desired
			// padding value.
			MemSet(startOfBlock, minimumSize - stringSize, zeroPad ? '0' : ' ');
			}	
			
		dstP += minimumSize - stringSize;
		*dstP = '\0';
		}
		
	return dstP;
}



/************************************************************
 *
 *  FUNCTION: PrvStrIToA			<internal>
 *
 *  DESCRIPTION: convert an integer to ascii
 *
 *  PARAMETERS: Integer to convert and string pointer to store results
 *
 *  RETURNS: s
 *
 *  CREATED: 11/09/94 
 *
 *  BY: Roger Flores
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	4/2/96	MemHandle negative numbers
 *			adamh 7/31/97	Modified to return the end of string to work with
 *								StrVPrintF
 *			roger	5/22/98	Added isSigned to MemHandle UInt32 numbers correctly.
 *
 *************************************************************/
static Char * PrvConvertIToA(Char * s, UInt32 i, Boolean isSigned)
{
	Char * s1;
	Char * s2;
	Char c1;
	Char c2;


	// Check for err
	ErrNonFatalDisplayIf(s == NULL, "NULL string passed");

	
	// MemHandle negative numbers
	if (isSigned && (Int32) i < 0)
		{
		*s++ = '-';
		i = -(Int32)i;
		}
	
	s1 = s;	// numbers start after the negative if it exists or at the start of the string.
		
		
	// output the string in reverse order
	do {
		*s++ = i % 10 + '0';
		i /= 10;								// CSE should reuse the above division
	} while (i > 0);
	*s = '\0';
	s2 = s;
	s2--;
	
	
	// Reverse the string
	while(s1 < s2) 
		{
		c1 = *s1;
		c2 = *s2;
		
		*s1++ = c2;
		*s2-- = c1;
		}

	return s;
}


/************************************************************
 *
 *	FUNCTION:	PrvConvertIToH	<internal>
 *
 *	DESCRIPTION: convert an integer to hexadecimal ascii
 *
 *	PARAMETERS: Integer to convert and string pointer to store results
 *
 *	RETURNS: s
 *
 *	HISTORY:
 *		07/04/95	RM		Created by Ron Marianetti
 *		07/31/97	AdamH	Modified to return the end of the string to work with
 *							StrVPrintF.
 *		04/02/98	roger	Added sizeModifier and fixed to emit less than 8 digits when not a long
 *		11/08/99	kwk	Non-fatal alert if unknown sizeModifier.
 *
 *************************************************************/
static Char * PrvConvertIToH(Char * s, UInt32 number, SizeModifierType sizeModifier)
{
	UInt32		mask;
	UInt32		digit;
	UInt16		digitCount;
	
	// Check for err
	ErrNonFatalDisplayIf(s == NULL, "NULL string passed");
	
	switch (sizeModifier)
	   {
		case sizeLong:
			digitCount = 8;
		   break;
		case sizeShort: 
			digitCount = 4;
			number <<= 16;
		   break;
		case sizeInt:
			digitCount = 4;
			number <<= 16;
		   break;
		default:
			ErrNonFatalDisplay("Invalid sizeModifier");
			digitCount = 8;
			break;
		}
	
	mask = 0xF0000000;
	while (digitCount-- > 0) 
		{
		digit = (number & mask) >> 28;
		if (digit < 0x0A) 
			*s++ = digit + '0';
		else
			*s++ = digit - 10 + 'A';
			
		number <<= 4;
		}
	*s = '\0';

	return s;
}


/******************************************************************************
 *  FUNCTION: StrVPrintF
 *
 *  DESCRIPTION: Implements a subset of the ANSI C vsprintf() call.
 *						Currently, only %d, %i, %u, %x and %s are implemented
 *						and don't accept field length or format specifications
 *						except for the l (long) modifier.
 *
 * 	Here's an example of how to use this call:
 *
 * #include <unix_stdarg.h>	// Palm OS code uses this instead of <StdArg.h>
 * void MyPrintF(Char * s, Char * formatStr, ...)
 * {
 * 	va_list	args;
 *		Char		text[0x100];
 * 	
 * 	va_start(args, formatStr);
 * 	StrVPrintF(text, formatStr, args);
 * 	va_end(args);
 * 	
 *		MyPutS(text);
 * }
 *
 *
 *  PARAMETERS: s 			- destination string
 *					 formatStr 	- format string
 *					 argParam	- pointer to argument list
 *
 *  RETURNS: # of characters written to destination string
 *
 *  CREATED: 6/18/96
 *
 *  BY: Ron Marianetti
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	10/24/96	Added NULL checks
 *			adamh	08/12/97	Added '+', ' ','-','h',minimum field width, argument based 
 *								field width.
 *			kwk	09/20/98	Added WChar support (uses "C" in format string).
 *			kwk	10/20/98	Added %lc format specifier for WChars.
 *							
 *****************************************************************************/


Int16 StrVPrintF(Char * s, const Char* formatStr, _Palm_va_list arg)
{
	Char *		curFormatP;
	Char *		startOfBlockP;
	Char *		dstP;
	const Char* formatP;
	Int32		   numberToConvert;
	Char *	   strP;
	WChar			wideChar;
	Char			wideStr[maxCharBytes + 1];
	Int16		   minimumSize = 0;
	Char		   signFlag = 0;
	Boolean     leftJustify = false;
	Boolean     zeroPad = false;
	SizeModifierType sizeModifier;

	
	// Check for err
	ErrNonFatalDisplayIf(s == NULL || formatStr == NULL, "NULL string passed");
	
	// Init vars.
	formatP = formatStr;
	dstP=s;													//Save the front of the string
	
	// Look for next format specification
	while(*formatP) {
		// If no more %'s, print the rest
		curFormatP = StrChr(formatP, '%');
		if (!curFormatP)
		 	{
			StrCopy(dstP, formatP); //Move the rest of the format string into the destination string.
			break;
			}
			
		// If there is another %, print everything up to it
		//Move all the characters before the % sign into the destination string
		StrNCopy(dstP, formatP, curFormatP - formatP);
		dstP += curFormatP - formatP;
		*dstP = '\0';
		
		// point to format character
		formatP = curFormatP+1;




		/********* Flags ************
			'+'  Flag will cause a plus or a minus sign to be printed ahead of the number 
			Example:
				printf("%+d  %+d",4,-5);
			Outputs:
					+4  -5
			' '  Flag will cause a space or a minus sign to be printed ahead of the number
			Example:
				printf("% d  % d",4,-5);
			Output:
				 4  -5
			'-'  Flag will cause the result of the conversion to be left justified
			Example:
				printf("%5d%-5d%d",6,9,8);
			Output:
				    69    8
		*/
		
		while ( true )	
			{
			 if(formatP[0] == '+')
				{
				signFlag = '+';
				formatP++;
				}
			 else if(formatP[0] ==' ')
			 	{
				if(signFlag != '+')
					signFlag =' ';
				formatP++;
				}
			else if(formatP[0] =='-')
				{
				leftJustify = true;
				formatP++;	
				}
			else if(formatP[0] == '0')
				{
				zeroPad = true;
				formatP++;
				}
			else
				break;
			}



		/********** Minimum Field Width **************
			'*' modifier will casue the next argument on the list to be taken in and used as
			 the field width.
			 Example:
			 	printf("%*d%d",4,8,5);
			 Output:
			 	8   5
			 Placing a number between the % and the conversion will cause at least that number of 
			 spaces to be placed in the result string for that conversion
			 Example:
			 	printf("%d%5d",4,3);
			 Output:
			 	4    3		
		*/
	   if (formatP[0] == '*')						// MemHandle variable length padding
			{
			minimumSize = va_arg(arg,int);
			formatP++;
			}
		else
			{
			
		    while(formatP[0] <='9' && formatP[0] >= '0')  // MemHandle fixed padding
		   	   {
		       minimumSize = minimumSize * 10 + formatP[0] - '0';
			   formatP++;
			   }
			}



		/*********** Type Modifiers *************
			'l','L' modifiers will allow a Int32 Int16 to be passed through the converison
			Example:
				printf("%ld",999999999);
			Output:
				999999999
			'h' modifiers will allow a Int16 Int16 to be passed through the conversion
			Example:
				printf("%hd",401);
			Output:
				401
		*/
		if (formatP[0] == 'l' || formatP[0] == 'L') 
			{
			sizeModifier = sizeLong;
			formatP++;
			}
		else if (formatP[0] == 'h') 
			{
			sizeModifier = sizeShort;
			formatP++;
			}
		else
			sizeModifier = sizeInt;
	
		/************ Conversions ******************
			Each of the following conversions will convert a variable into a string and insert it 
			into the result string
		
	
		*/
		startOfBlockP=dstP;	//Keep a pointer to the start of the block for when we add the padding.
		switch (formatP[0])
		  {
		  
		  /********	Unsigned Integer Conversion *********
				This will convert to a string representation of an unsigned integer and place it on
				the destination string.
				
				Example:
					printf("%u   %u",4,-4);
				Output:
				4   65532
		   */
			
			case 'u':								
				switch (sizeModifier)
			   {
				case sizeLong:
				   numberToConvert = va_arg(arg, UInt32);
				   break;
				case sizeShort: 
				   numberToConvert = va_arg(arg, UInt16);
				   break;
				case sizeInt:
				   numberToConvert = va_arg(arg, UInt16);
				   break;
				}
				if (signFlag == '+' || signFlag == ' ')
					*dstP++ = signFlag;
				dstP = PrvConvertIToA(dstP, numberToConvert, false);
				dstP = PrvAddPadding(startOfBlockP, dstP, minimumSize, leftJustify, zeroPad, signFlag);
				
				formatP++;
				break;
				
				
			/********* Integer Conversion **********
				This will convert to a string representation of a signed integer andplace it on the
				destination string.
				
				Example:
					printf("%d   %d",4,-4);
				Output:
				4   -4	
			*/	
			
			case 'd':
			case 'i':								
				switch (sizeModifier) 
					{
					case sizeLong:
						numberToConvert = va_arg(arg, long);
						break;
					case sizeShort:
					   numberToConvert = va_arg(arg, Int16);
						break;
					case sizeInt:
						numberToConvert = va_arg(arg, int);
						break;
					}
				if((signFlag == '+' || signFlag == ' ') && numberToConvert >= 0) 
					*dstP++ = signFlag;
				dstP = PrvConvertIToA(dstP, numberToConvert, true);
				dstP = PrvAddPadding(startOfBlockP, dstP, minimumSize, leftJustify, zeroPad, signFlag);
			
				formatP++;
				break;
				
				
			/********** Hexadecimal Conversion ***********
				Converts integer variables into strings of hexadecimal values
				
				Example:
					printf("%x",125);
				Output:
					0000007D
				
			*/
			case 'X':
			case 'x':								
				switch (sizeModifier)
					{
					case sizeLong:
						numberToConvert = va_arg(arg, long);
						break;
					case sizeShort: 
						numberToConvert = va_arg(arg, Int16);
						break;
					case sizeInt:
						numberToConvert = va_arg(arg, int);
					break;
				}
				dstP = PrvConvertIToH(dstP, numberToConvert, sizeModifier);
				dstP = PrvAddPadding(startOfBlockP, dstP, minimumSize, leftJustify, zeroPad, signFlag);
				
				formatP++;
				break;
				
				
			/*********** String Conversion *************
				Will copy a string onto the end of the destination string.
				
				Example:
					printf("United %s","States");
				Output:
					United States
			*/
			
			case 's':							
				strP = va_arg(arg, char *);
				StrCopy(dstP, strP);
				dstP += StrLen(startOfBlockP);
				dstP = PrvAddPadding(startOfBlockP, dstP, minimumSize, leftJustify, zeroPad, signFlag);
			
				formatP++;
				break;
				
				
			/********** Character Conversion *************
				Will copy a single character to the end of the destination string.
				
				Example:
					printf("Telephone%c",'s');;
				Output:
					Telephones
			*/	
			
			case 'c':				
				if (sizeModifier != sizeLong)
					{
					*dstP++ = va_arg(arg, int);	//Read the character from the variable argument list into the destination string.
					*dstP = '\0';
					dstP = PrvAddPadding(startOfBlockP, dstP, minimumSize, leftJustify, zeroPad, signFlag);
					
					formatP++;
					break;
					}
			
				// else for %lc case, fall through into %C code for wide character.
				
			case 'C':
				wideChar = va_arg(arg, WChar);	// Read char from variable argument list.
				wideStr[TxtSetNextChar(wideStr, 0, wideChar)] = '\0';
				StrCopy(dstP, wideStr);
				dstP += StrLen(startOfBlockP);
				dstP = PrvAddPadding(startOfBlockP, dstP, minimumSize, leftJustify, zeroPad, signFlag);
			
				formatP++;
				break;
				
			/********** %% Case **************
				This case allows the user to print out a % sign
				
				Example:
					printf("%%");
				Output:
					%
			*/
			
			case '%':							
				*dstP++ = '%';
				*dstP ='\0';
				formatP++;
				break;
				
		
			default:
				ErrNonFatalDisplay("Unsupported");
				break;
			}
			
		//Clear all flags	
		minimumSize = 0;
		leftJustify = false;
		zeroPad = false;
		signFlag = 0;
		}
	
	va_end(arg);
	return StrLen(s);
}




/******************************************************************************
 *  FUNCTION: StrPrintF
 *
 *  DESCRIPTION: Implements a subset of the ANSI C sprintf() call.
 *  
 *  The following is implemented:
 *
 *  Conversions:	%u - UInt16
 *					%d, %i - signed int
 *					%x, %x - hexadecimal
 *					%s - string
 *					%c - char
 *					%% - '%'
 *
 *  Type Modifier:  l,L - long int (%ld)
 *					h - Int16 int (%hd)
 *
 *  Field Width:	number - right justify result in field of width number (%5d)
 *
 *  Flags:			 + - cause a plus or minus sign to preceed the number (%+d)
 *					' ' - cause a space or minus sign to preceed the number (% d)
 *					'-' - cause the result to be left justified (%-5d)
 *					 * - use the next argument for the field with (%*d)
 *					'0' - pad the front of the value with 0s.
 *
 *  PARAMETERS: s 			- destination string
 *					 formatStr 	- format string
 *					 ...			- arguments for format string.
 *
 *  RETURNS: # of characters written to destination string
 *
 *  CREATED: 6/18/96
 *
 *  BY: Ron Marianetti
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	10/24/96	Added NULL checks
 *****************************************************************************/
Int16 StrPrintF(Char * s, const Char* formatStr, ...)
{
	va_list	args;
	Int16		result;
	
	// Check for err
	ErrNonFatalDisplayIf(s == NULL || formatStr == NULL, "NULL string passed");
	
	va_start(args, formatStr);
	result = StrVPrintF(s, formatStr, args);
	va_end(args);
	
	return result;
}
