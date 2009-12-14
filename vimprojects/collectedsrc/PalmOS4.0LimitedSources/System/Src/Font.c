/******************************************************************************
 *
 * Copyright (c) 1994-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: Font.c
 *
 * Release: 
 *
 * Description:
 *	  This file contains the font routines.
 *
 * History:
 *		Dec 14, 1994	Created by Art Lamb
 *		08/20/00	CS		Cleaned up usage of TxtGluePrv.h so that it works for
 *							all the PalmRez/MakeOverlay targets.
 *
 *****************************************************************************/

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_FONTS
#define ALLOW_ACCESS_TO_INTERNALS_OF_CLIPBOARDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_FIELDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS

#include <PalmTypes.h>
#include <ErrorMgr.h>
#include <Chars.h>
#include <TextMgr.h>

#define NON_PORTABLE
#include "Globals.h"
#include "UIGlobals.h"
#include "FontPrv.h"
#include "IntlPrv.h"		// IntlData, kStrictChecksFlag

// When we're being built as part of the PalmRez or MakeOverlay MPW tools,
// the PalmRez CodeWarrior plugin, the MakeOverlay app, or the PalmRez test app,
// we get linked with an explicit Latin version of the Text Mgr.  Note that
// this means that even if we pass a Japanese font file on the PalmRez/MakeOverlay
// MPW command line or use the MakeOverlay-J app, we still get only Latin locale
// glue.  It's a good thing we're only checking for invalid byte sequences.
//
// DOLATER CS -	We could make the locale module part of the Japanese Support
//						file used by the CodeWarrior plugin.  We could also pass a
//						this entire Japanese Support file to the MPW tools via the
//						command line.  Another option (since we're really only using
//						the byteAttrTable), would be to call IntlInit and let it set
//						just this table up via its simulator support code.  In the
//						end, though, it's hardly worth worrying about as PalmRez
//						and MakeOverlay should be replaced soon(?) by the new
//						Constructor.
//
#if defined(BUILD_TARGET) && defined(MAKE_OVERLAY)
	#include "TxtGluePrv.h"
		
	#define	TxtByteAttr	TxtLatinByteAttr
#endif

/***********************************************************************
 * Private functions
 ***********************************************************************/
 
static Boolean FindReverseByte (const Char* text, Char byteToFind, UInt16 len, UInt16* pos);

static UInt16 PrvMultibyteWrap(const Char* iStringP, UInt16 iMaxWidth);

/***********************************************************************
 *
 * FUNCTION:    FindReverseByte
 *
 * DESCRIPTION: This routine searches a string in reverse order for the
 *              the character specified.
 *
 * PARAMETERS:	fld          a pointer to the string to search
 *             byteToFind   the character to find
 *             len
 *             
 *
 * RETURNED:	nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/22/94	Initial Revision
 *
 ***********************************************************************/
static Boolean FindReverseByte (Char const * text, Char byteToFind, UInt16 len, 
	UInt16 * pos)
{
	Char const * p;
	
	p = text + len - 1;
	
	while (len)
		{
		if (*p == byteToFind)
			{
			*pos = len-1;
			return (true);
			}
		p--;
		len--;
		};
	
	return (false);
}


/***********************************************************************
 *
 * FUNCTION:    GetCharsWidth
 *
 * DESCRIPTION: This routine returns width of a character that is reference
 *              via a font mapping table.
 *
 * PARAMETERS:  charsP - pointer to the character to measure
 *              charSizeP - length of the character measured (return value)
 *
 * RETURNED:    width of the character in pixels
 *
 * HISTORY:
 *		03/02/98	art	Created by Art Lamb.
 *		07/16/98	kwk	Re-enabled check for invalid low byte before assuming
 *							that a missing character is single byte.
 *		11/30/00	kwk	Alter flow so that valid double-byte check is done if
 *							low-byte is in range but undefined (e.g. 0x7F).
 *
 ***********************************************************************/
static Int16 GetCharsWidth (const Char* charsP, Int16* charSizeP)
{
	Int16 				ch;
	Int32 				index;
	Int16					width;
	Int16					charSize;
	Int16 				firstChar;
	Int16 				lastChar;
	FontPtr				fontP;
	FontTablePtr		fontListP;
	FontMapPtr			fontMap;
	FontCharInfoType * charInfo;

	charSize = 1;

	index = *(UInt8 *)charsP;
	fontMap = (FontMapPtr) (UICurrentFontPtr+1);
	if ((UICurrentFontPtr->fontType & fntAppFontMap) == fntAppFontMap)
		fontListP = GAppSubFontListPtr;
	else
		fontListP = UIFontListPtr;
	fontP = fontListP [fontMap[index].value];
	if (fontMap[index].state == fntStateNextIsChar)
		{
		charsP++;
		charSize = 2;
		}

	firstChar = fontP->firstChar;
	lastChar = fontP->lastChar;
	charInfo = (FontCharInfoType *) (&fontP->owTLoc) + fontP->owTLoc;

	ch = *(UInt8 *)charsP++;

	*charSizeP = charSize;
	if (ch >= firstChar && ch <= lastChar)
		{
		width = charInfo[ch - firstChar].width;
		if (width != fntMissingChar)
			return(width);
		}

	// If the character is a double byte character and the low byte 
	// is invalid then the low byte is the first byte of the 
	// next character.
	// DOLATER kwk - This means that we're no longer truely
	// int'l, as we've got checks for character size = 2 here. Better would
	// be to use the font map table to decide if the byte is valid.
	
	width = charInfo[lastChar - firstChar +1].width;
	if ((charSize == 2) && ((TxtByteAttr(ch) & byteAttrLast) == 0))
		{

#if (ERROR_CHECK_LEVEL == ERROR_CHECK_FULL)
		if ((GIntlData->intlFlags & kStrictChecksFlag) != 0)
		{
			ErrNonFatalDisplay("Measuring width of invalid double-byte character");
		}
#endif

		*charSizeP = 1;
		}

	return (width);
}


/***********************************************************************
 *
 * FUNCTION:    FntGetFont
 *
 * DESCRIPTION: This routine returns the FontID of the current font.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    FontID of the current font.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/14/94		Initial Revision
 *
 ***********************************************************************/
FontID FntGetFont (void)
{
	return (UICurrentFontID);
}


/***********************************************************************
 *
 * FUNCTION:    FntSetFont
 *
 * DESCRIPTION: This routine set the current font.  If the font is a valid
 * font but doesn't exist, the stdFont is used.
 *
 * PARAMETERS:  fontID   id of the font to make the active font
 *
 * RETURNED:    ID of the current font prior to the change.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/14/94	Initial Revision
 *			roger	9/26/97	Now substitutes the stdFont for missing fonts
 *			SCL	12/10/97	Rewrote to support new (separate) font tables
 *			SCL	1/23/98	Now uses stdFont if passed an invalid font
 *			bob	3/5/99	Update GState values with new font
 *
 ***********************************************************************/
FontID FntSetFont (FontID fontID)
{
	FontID newFontID = fontID;
	FontID oldFontID;

	// See if fontID is a valid system font or app font ID
	if ( !FntIsAppDefined(newFontID) ? (newFontID >= UINumSysFonts) :
			(newFontID-fntAppFontCustomBase >= UINumAppFonts) ) {
		ErrNonFatalDisplay("Invalid font");
		newFontID = stdFont;
	}
	
	oldFontID = UICurrentFontID;
	UICurrentFontID = newFontID;
	
	// Set the new font or a default one.
	if ( FntIsAppDefined(newFontID) ) {
		UICurrentFontPtr = UIAppFontPtr[newFontID-fntAppFontCustomBase];
	} else {
		UICurrentFontPtr = UISysFontPtr[newFontID];
	}

	if (UICurrentFontPtr == NULL)
		UICurrentFontPtr = UISysFontPtr[stdFont];

	GState.drawStateP->fontId = UICurrentFontID;
	GState.drawStateP->font = UICurrentFontPtr;

	ErrNonFatalDisplayIf (UICurrentFontPtr == NULL, "Invalid font");
	
	return oldFontID;
}


/***********************************************************************
 *
 * FUNCTION:    FntGetFontPtr
 *
 * DESCRIPTION: This routine returns a pointer to the current font.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    FontPtr of the current font.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/14/94		Initial Revision
 *
 ***********************************************************************/
FontPtr FntGetFontPtr (void)
{
	return (UICurrentFontPtr);
}


/***********************************************************************
 *
 * FUNCTION:    FntGetFontList
 *
 * DESCRIPTION: This routine returns a pointer to the font table.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    Font table of the current font.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			acs	5/27/00	Initial Revision
 *			acs	6/19/00	Make this private to OS
 *
 ***********************************************************************/
FontTablePtr FntPrvGetFontList (Int16 fontType)
{
	if (((UInt16)fontType & fntAppFontMap) == fntAppFontMap)
		 return (FontTablePtr)GAppSubFontListPtr;

	return  UIFontListPtr;
}


/***********************************************************************
 *
 * FUNCTION:    FntDefineFont
 *
 * DESCRIPTION: Define a font.  Only an app font may be defined.  The
 * application is responsible for freeing or releasing the font when
 * the application quits.
 *
 * PARAMETERS:  fontID - id of the font to define.  Should be a app defined
 * 							 font only.
 *					 fontP - Pointer to font to use.
 *
 * RETURNED:    memErrNotEnoughSpace - if it isn't able to allocate
 *						space for the new font in the font table.
 *					 0 - if no error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	 9/26/97	Initial Revision
 *			SCL	12/10/97	Rewrote to support new (separate) app font table
 *			roger	11/ 9/98	Fixed MemMove reading past end of source.
 *			gavin	11/18/98	set UINumAppFonts to count, not index
 *			SCL	11/25/98	Fixed MemMove to copy exact size of old table
 *			vivek 06/07/00 Added support for hierarchical application fonts
 *
 ***********************************************************************/
Err FntDefineFont (FontID fontID, FontPtr fontP)
{
	UInt32			reqSize;
	FontTablePtr	tableP;
	FontTablePtr	newTableP;
	Err				err;
	Int16				appFontIndex;
	Int16				fontIndex;
	Int16				numAppFonts;
	Boolean			isSubFont;

	ErrNonFatalDisplayIf (!FntIsAppDefined(fontID), "App defined fonts only");
	// KEY POINT:  "...Index" is a zero-based array index, whereas
	// "...Num...Fonts" is a one-based number of fonts.
	// Thus, appFontIndex is the array index into which fontP will be installed.
	// The font table must be large enough to support this new font index.
	
	isSubFont = (fontP->fontType & fntAppSubFontMask) ? true : false;
	
	if (isSubFont)
		{
			numAppFonts = MemPtrSize(GAppSubFontListPtr) / sizeof(FontPtr);		
			tableP = GAppSubFontListPtr;
		}
	else
		{
			tableP = UIAppFontTablePtr;
			numAppFonts = UINumAppFonts;
		}
	
	appFontIndex = (FontID) (fontID - fntAppFontCustomBase);
	
		
	// See if there's room in the app font table for this font
	if (appFontIndex >= numAppFonts) 
		{
		// if not, try to resize the table (new size is appFontIndex+1 entries)
		reqSize = (appFontIndex+1) * sizeof(FontPtr);
		err = MemPtrResize(tableP, reqSize);
		
		// If we couldn't resize it, try and reallocate it
		if (err) 
			{
			newTableP = MemPtrNew(reqSize);
			if (!newTableP) return memErrNotEnoughSpace;
			
			// Copy old table (numFonts entries) into new table
			MemMove(newTableP, tableP, numAppFonts * sizeof(FontPtr));
			MemPtrSetOwner(newTableP, 0);

			// Change global to point to new table, and free the old one
			if (isSubFont)
				GAppSubFontListPtr = newTableP;
			else
				UIAppFontTablePtr = newTableP;
			
			MemPtrFree(tableP);
			tableP = newTableP;
			}

		// Zero out (only) the newly added slots in the table
		for (fontIndex = numAppFonts; fontIndex <= appFontIndex; fontIndex++) 
			{
			tableP[fontIndex] = NULL;
			}
		
		// Save the new table size
		if (!isSubFont)
			UINumAppFonts = appFontIndex+1;
		}

	tableP[appFontIndex] = fontP;
		
	return 0;
}


/***********************************************************************
 *
 * FUNCTION:    FntProportionalFont
 *
 * DESCRIPTION: This function indicates whether the current font is
 *              proportionally spaced or fixed width.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    TRUE if the current font is proportionally 
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/14/94		Initial Revision
 *
 ***********************************************************************/
//Boolean FntProportionalFont (void)
//{
//	return (UICurrentFontPtr->pixelWidth == 0);
//}


/***********************************************************************
 *
 * FUNCTION:    FntBaseLine
 *
 * DESCRIPTION: This function returns the distance from the top of 
 *              character cell to the baseline for the current font.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    Baseline of the font, expressed in pixels.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/14/94		Initial Revision
 *
 ***********************************************************************/
Int16 FntBaseLine (void)
{
	return (UICurrentFontPtr->ascent);
}



/***********************************************************************
 *
 * FUNCTION:    FntCharHeight
 *
 * DESCRIPTION: This function returns the height of the characters in the 
 *              current font.  The value returns includes the height of
 *              character accents and descenders.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    Height of the characters in the current font, , expressed
 *              in pixels.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/14/94		Initial Revision
 *
 ***********************************************************************/
Int16 FntCharHeight (void)
{
	return (UICurrentFontPtr->fRectHeight);
}


/***********************************************************************
 *
 * FUNCTION:    FntLineHeight
 *
 * DESCRIPTION: This function returns the height of a line in the current
 *              font.  The height of a line is the height of the character
 *              cell plus the space between lines (the external leading).
 *
 * PARAMETERS:  none
 *
 * RETURNED:   The height of a line in the current font.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/14/94		Initial Revision
 *
 ***********************************************************************/
Int16 FntLineHeight (void)
{
	return (UICurrentFontPtr->fRectHeight + UICurrentFontPtr->leading);
}


/***********************************************************************
 *
 * FUNCTION:    FntAverageCharWidth
 *
 * DESCRIPTION: This function returns the average width of characters
 *              in the current font.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    The average width of characters in the current font, 
 *              expressed in pixels.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/14/94		Initial Revision
 *
 ***********************************************************************/
Int16 FntAverageCharWidth (void)
{
	return (UICurrentFontPtr->fRectWidth);
}


/***********************************************************************
 *
 * FUNCTION:    FntAccentHeight
 *
 * DESCRIPTION: This routine returns the height of an accent of the 
 *              characters in the current font.  The height of an accent
 *              is the distance between the top of the character cell and
 *              the top a non-accent capital letter.
 *
 * PARAMETERS:  none
 *
 * RETURNED:   Height of an accent, expressed in pixels.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/14/94		Initial Revision
 *
 ***********************************************************************/
//Int16 FntAccentHeight (void)
//{
//	return (UICurrentFontPtr->intLead);
//}


/***********************************************************************
 *
 * FUNCTION:    FntDescenderHeight
 *
 * DESCRIPTION: This routine returns the height of a decender of the 
 *              characters in the current font.  The height of a decender
 *              is the distance between the base line an the bottom
 *              of the character cell.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    Height of a decender, expressed in pixels.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/14/94		Initial Revision
 *
 ***********************************************************************/
Int16 FntDescenderHeight (void)
{
	return (UICurrentFontPtr->descent);
}


/***********************************************************************
 *
 * FUNCTION:    FntAscent
 *
 * DESCRIPTION: This routine returns the ascent of the characters in 
 *              the current font.  The ascent of a character is the 
 *              distance from the top of a non-accent capital letter
 *              to the base line.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    Ascent of the characters expressed in pixels.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/14/94		Initial Revision
 *
 ***********************************************************************/
//Int16 FntAscent (void)
//{
//return (UICurrentFontPtr->baseLine - UICurrentFontPtr->intLead);
//}


/***********************************************************************
 *
 * FUNCTION:    FntCharWidth
 *
 * DESCRIPTION: This routine returns the width of the specified character,
 *		which is assumed to be a single-byte character. If the specified
 *		character does not exist within the current font, the Missing Char
 *		Symbol will be substituted.
 *
 * PARAMETERS:  ch   character whose width is desired
 *
 * RETURNED:    width of the specified character, in pixels
 *
 * HISTORY:
 *		12/14/94	art	Created by Art Lamb.
 *		11/07/95	kcr	returns values for Missing Char Symbol if nesc.
 *		05/06/98	art	Add support for double byte fonts.
 *		12/10/00	kwk	Use buffer for case of measuring char width in multi-
 *							byte font, as otherwise garbage on the stack could
 *							make it look like a valid double-byte char when actually
 *							it's an orphan high byte.
 *
 ***********************************************************************/
Int16 FntCharWidth(Char ch)
{
	UInt32 	index;
	Int16		chr;
	Int16		charSize;
	Int16		width;
	FontPtr	fontP;

	fontP = UICurrentFontPtr;

	if (((UInt16)fontP->fontType & fntFontMapMask) != fntFontMapMask)
		{
		chr = (UInt8)ch;
		
		//	Is the character in range.
		if ((chr >= fontP->firstChar) && (chr <= fontP->lastChar))
			{
			width = ((FontCharInfoPtr) (&fontP->owTLoc) + 
				fontP->owTLoc + (chr - fontP->firstChar))->width;
			if (width != fntMissingChar)
				return (width);
			}
		
		//	Return the width of the Missing Char Symbol, which is located at index=last-first+1.
		index = fontP->lastChar - fontP->firstChar + 1;
		return ((FontCharInfoPtr) (&fontP->owTLoc) + fontP->owTLoc + index)->width;
		}
	else
		{
		Char buffer[2];
		
		buffer[0] = ch;
		buffer[1] = chrNull;
		width = GetCharsWidth(buffer, &charSize);
		return (width);
		}
}


/***********************************************************************
 *
 * FUNCTION:    FntWCharWidth
 *
 * DESCRIPTION: This routine returns the width of the specified character,
 *		which can be any valid character character. If the specified
 *		character does not exist within the current font, the Missing Char
 *		Symbol will be substituted.
 *
 * PARAMETERS:  ch   character whose width is desired
 *
 * RETURNED:    width of the specified character, in pixels
 *
 * HISTORY:
 *		05/12/00	kwk	Created by Ken Krugler from TxtCharWidth.
 *		05/17/00	kwk	Add call to TxtCharIsValid on debug ROMs.
 *
 ***********************************************************************/
Int16 FntWCharWidth(WChar iChar)
{
	Char buffer[maxCharBytes];

	if (iChar >= 0xFF80) {
		ErrNonFatalDisplay("Sign extended char passed to FntWCharWidth");
		iChar &= 0x00FF;
	}

	// On debug ROMs, make sure the character is valid (e.g. not a
	// virtual character).
	ErrNonFatalDisplayIf(!TxtCharIsValid(iChar), "Invalid char passed to FntWCharWidth");
	
	return(FntCharsWidth(buffer, TxtSetNextChar(buffer, 0, iChar)));
} // FntWCharWidth


/***********************************************************************
 *
 * FUNCTION:    FntCharsWidth
 *
 * DESCRIPTION: This routine returns the width of the specified character
 *              string.  The Missing Character Symbol will be substituted for
 *						any char which does not exist in the current font.
 *
 * PARAMETERS:  charsP  pointer to a string of characters
 *              length  number of character in the string.
 *
 * RETURNED:    width of the string, in pixels
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/14/94		Initial Revision
 *			kcr	11/7/95	Substitutes Missing Char Symbol for missing chars
 *			art	5/6/98	Add support for double byte fonts.
 *
 ***********************************************************************/
Int16 FntCharsWidth (const Char * charsP, Int16 length)
{
	register Int16 					ch;
	register Int16 					firstChar;
	register Int16 					lastChar;
	register Int16						width;
	register Int16 					widths = 0;
	register FontCharInfoType * 	charInfo;
				Int16						charSize;
				FontPtr 					fontP;


	fontP = UICurrentFontPtr;

	if (((UInt16)fontP->fontType & fntFontMapMask) != fntFontMapMask)
		{
		firstChar = fontP->firstChar;
		lastChar = fontP->lastChar;
		charInfo = (FontCharInfoType *) (&fontP->owTLoc) + fontP->owTLoc;

		while (length)
			{
			// Get the index of this character in the font
			ch = *(UInt8 *)charsP++;
			length--;

			if (ch >= firstChar && ch <= lastChar)
				{
				width = charInfo[ch - firstChar].width;
				if (width != fntMissingChar)
					{
					widths += width;
					continue;
					}
				}
			widths += charInfo[lastChar - firstChar +1].width;
			}
		}

	else
		{
		while ((length) > 0)
			{
			widths += GetCharsWidth (charsP, &charSize);
			length -= charSize;
			charsP += charSize;
			}
		}

	
	return (widths);
}


/***********************************************************************
 *
 * FUNCTION:    FntWidthToOffset
 *
 * DESCRIPTION: This routine returns the byte offset which corresponds
 *					to the pixel position (relative to the beginning of the
 *					text). The leadingEdge flag is set if the pixel position
 *					falls on the left side of the character; if this position
 *					is beyond the end of the text then leadingEdge is always
 *					true. Note that both leadingEdge & truncWidth parameters
 *					can be NULL if the caller doesn't want those results
 *					returned.
 *
 * PARAMETERS:	charsP  	pointer to a string of characters
 *					length  	number of character in the string
 *					pixelWidth	position in pixels from beginning of text
 *					leadingEdge	boolean flag => true if left side of char
 *					truncWidth	width of text up to the returned offset
 *
 * RETURNED:	UInt8 offset to inter-character boundary before first
 *					character located beyond pixelWidth.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			kwk	07/01/98	Initial Revision
 *			kwk	08/19/98	Change test to be OK if width <= pixelWidth, versus
 *								less than.
 *
 ***********************************************************************/
Int16 FntWidthToOffset(Char const * charsP, UInt16 length, Int16 pixelWidth,
						Boolean *leadingEdge, Int16 * truncWidth)
{
	FontPtr			fontP = UICurrentFontPtr;
	Int16				offset = 0;
	Int16				curWidth = 0;
	Int16				charWidth = 0;
	Int16				charSize;
	
	ErrNonFatalDisplayIf(charsP == NULL, "Null string");
	
	// If it's a simple font (only single-byte characters) then process
	// it directly, otherwise we need to use the GetCharsWidth routine.
	
	if (((UInt16)fontP->fontType & fntFontMapMask) != fntFontMapMask) {
		Int16 firstChar = UICurrentFontPtr->firstChar;
		Int16 lastChar = UICurrentFontPtr->lastChar;
		FontCharInfoType *info = (FontCharInfoType *) (&UICurrentFontPtr->owTLoc) + 
				UICurrentFontPtr->owTLoc;

		while (offset < length) {
			UInt8 ch = *charsP++;
	
			if ((ch >= firstChar) && (ch <= lastChar)) {
				charWidth = info[ch - firstChar].width;
				if (charWidth == fntMissingChar) {
					charWidth = info[lastChar - firstChar +1].width;
				}
			} else {
				charWidth = info[lastChar - firstChar +1].width;
			}
			
			curWidth += charWidth;
			if (curWidth <= pixelWidth) {
				offset += 1;
			} else {
				break;
			}
		}
	} else {
		while (offset < length) {
			charWidth = GetCharsWidth(charsP, &charSize);

			curWidth += charWidth;
			if (curWidth <= pixelWidth) {
				offset += charSize;
				charsP += charSize;
			} else {
				break;
			}
		}
	}

	// If we ran over the end of the string, we know that the leading
	// edge result must be false, and we also want to prune back the
	// returned offset to be the length (just in case we got passed a
	// bogus double-byte character at the end of the string).
	
	if (offset >= length) {
		if (leadingEdge != NULL) {
			*leadingEdge = true;
		}
		
		if (truncWidth != NULL) {
			*truncWidth = curWidth;
		}
		
		return(length);
	} else {
		if (leadingEdge != NULL) {
			*leadingEdge = (curWidth - pixelWidth > (charWidth / 2));
		}
	
		if (truncWidth != NULL) {
			*truncWidth = curWidth - charWidth;
		}
		
		return(offset);
	}
} // FntWidthToOffset


/***********************************************************************
 *
 * FUNCTION:    FntCharsInWidth
 *
 * DESCRIPTION: Finds the number of characters in a string that fit
 * within a passed width.  Spaces at the end of a string are ignored and 
 * removed. Any chars after a carriage return are ignored and the string
 * is considered truncated.
 *
 * PARAMETERS:  string  			- pointer to the char string
 *              stringWidthP   	- maximum width to allow
 *              stringLengthP 	- maximum characters to allow
 *					 assumes current Font
 *
 * RETURNED:    stringWidthP  	- set to the width of the chars allowed
 *              stringLengthP 	- set to the number of chars within the width
 *              fitWithinWidth 	- whether the string is considered truncated
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/20/95	Initial Revision
 *			roger 12/4/95	Added fitWithinWidth to clear up function usage.
 *			art	5/6/98	Add support for double byte fonts.
 *
 ***********************************************************************/
 
void	FntCharsInWidth (Char const * string, Int16 *stringWidthP, 
	Int16 *stringLengthP, Boolean *fitWithinWidth)
{
	Int16 width = 0;
	Int16 charWidth;
	Int16 length = 0;
	Int16 maxLength = *stringLengthP;
	Int16 maxWidth = *stringWidthP;
	Int16 firstChar;
	Int16 lastChar;
	Int16 charSize;
	UInt8 ch;
	Char const * s2;
	FontCharInfoType * info;
	FontPtr	fontP;



	ErrNonFatalDisplayIf(string == NULL, "Null string");
	ErrNonFatalDisplayIf(stringWidthP == NULL, "Null stringWidthP");
	ErrNonFatalDisplayIf(stringLengthP == NULL, "Null stringLengthP");
	ErrNonFatalDisplayIf(fitWithinWidth == NULL, "Null fitWithinWidth");
	
	fontP = UICurrentFontPtr;

	if (((UInt16)fontP->fontType & fntFontMapMask) != fntFontMapMask)
		{
		firstChar = UICurrentFontPtr->firstChar;
		lastChar = UICurrentFontPtr->lastChar;
		info = (FontCharInfoType *) (&UICurrentFontPtr->owTLoc) + 
				UICurrentFontPtr->owTLoc;
	
		
		while (*string != '\0' && *string != linefeedChr && length < maxLength)
			{
			// If the character exist, get its width, otherwise get the width
			// of the Missing Character symbol.
			ch = *string;
	
			if (ch >= firstChar && ch <= lastChar && 
					info[ch - firstChar].width != fntMissingChar)
				charWidth = info[ch - firstChar].width;
			else	
				charWidth = info[lastChar - firstChar +1].width;
	
			if (width + charWidth <= maxWidth)
				{
				width += charWidth;
				length++;
				}
			else
				break;			// can't add any more characters.
				
			string++;
			}
		}

	else
		{
		while (*string != '\0' && *string != linefeedChr && length < maxLength)
			{
			charWidth = GetCharsWidth (string, &charSize);

			if (width + charWidth <= maxWidth)
				{
				width += charWidth;
				length += charSize;
				}
			else
				break;			// can't add any more characters.
				
			string += charSize;
			}
		}
		
	// string is the first character that didn't get included.  A '\0'
	// means the entire string fit, a linefeedChr means the string wrapped
	// (didn't fit), and anything else means the string was too wide.
	// Now, if the string is too wide because of blank characters, then
	// the string shouldn't considered to be truncated.  It's entire visible
	// contents does fit.
	

	// If the character we were adding was a whitespace and
	// all the characters until the end of the string are white spaces
	// then we don't consider the string truncated.
	if (length >= maxLength)		// used up all the characters
		*fitWithinWidth = true;
	else if (*string == linefeedChr)
		*fitWithinWidth = false;
	else if (*string == '\0')
		*fitWithinWidth = true;
	else
		{
		s2 = string;
		
		while (*s2 == ' ' || *s2 == tabChr)
			s2++;
		
		if (*s2 == '\0')
			*fitWithinWidth = true;
		else
			*fitWithinWidth = false;
		}
	

	// Visual optimization.  If the last char was unseen remove it.
	// Drawing it adds nothing of value.
	string--;
	while (length > 0 && 
		(*string == ' ' || *string == linefeedChr || *string == tabChr))
		{
		width -= FntCharWidth(*string);
		string--;
		length--;
		}


	*stringWidthP = width;
	*stringLengthP = length;
}


/***********************************************************************
 *
 * FUNCTION:    FntLineWidth
 *
 * DESCRIPTION: This routine returns the width of the specified line
 *              of text.  It takes tab characters in to account.  It is
 *					 assumed that the characters passed are left-align and
 *              that the fisrt character of the string is the first
 *              charater draw on a line.  In other words, this routine
 *              will not work for characters that start in a
 *              position other than the start of a line.
 *
 * PARAMETERS:	 charsP  pointer to a string of characters
 *              length  number of character in the string.
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	10/20/95	Initial Revision
 *			kcr	11/7/95	handles missing chars
 *			art	5/6/98	Add support for double byte fonts.
 *
 ***********************************************************************/
Int16 FntLineWidth (Char const * charsP, UInt16 length)
{
	FontCharInfoType * ptr;
	Int16 width = 0;
	Int16 firstChar;
	Int16 lastChar;
	Int16 charSize;
	UInt8 ch;

	ErrNonFatalDisplayIf(length > 0 && charsP == NULL, "Null charsP");
	
	if (((UInt16)UICurrentFontPtr->fontType & fntFontMapMask) != fntFontMapMask)
		{
		firstChar = UICurrentFontPtr->firstChar;
		lastChar = UICurrentFontPtr->lastChar;
		ptr = (FontCharInfoType *) (&UICurrentFontPtr->owTLoc) + 
				UICurrentFontPtr->owTLoc;
				
		while (length)
			{
			ch = *charsP;
			if (ch == tabChr)
				// DOLATER art - calculate tab width base on the size of the font
				width += fntTabChrWidth - (width % fntTabChrWidth);
	
			else if (ch >= firstChar && ch <= lastChar &&
						ptr[ch - firstChar].width != fntMissingChar)	//	char exists in font
				width += ptr[ch - firstChar].width;
	
			else	//	Use width of Missing Char Symbol
				width += ptr[lastChar - firstChar +1].width;
	
			charsP++;
			length--;
			}
		}
		
	else
		{
		while (((Int16)length) > 0)
			{
			if (*charsP == tabChr)
				{
				width += fntTabChrWidth - (width % fntTabChrWidth);
				length--;
				charsP++;
				}
			else
				{
				width += GetCharsWidth (charsP, &charSize);
				length -= charSize;
				charsP += charSize;
				}
			}
		}
	
	return (width);
}


/***********************************************************************
 *
 * FUNCTION: PrvMultibyteWrap
 *
 * DESCRIPTION: Given a string, determine the number of characters that
 *		can be displayed within the specified width. We assume that the
 *		current font is a multi-byte font, and that FntWordWrap has
 *		already screened out odd cases.
 *
 * PARAMETERS:
 *		iStringP	 -> pointer to a null-terminated string
 *		iMaxWidth -> maximum line width in pixels
 *
 * RETURNED:
 *		length of the line in bytes
 *
 * HISTORY:
 *		05/29/00	kwk	Created by Ken Krugler from Art's original code.
 *		06/19/00	kwk	Re-wrote to be faster - do all character processing
 *							here, versus calling other routines.
 *		07/29/00	kwk	Catch case of not even one character fitting on a line.
 *		11/29/00	kwk	Report non-fatal alert if wrapping invalid double-byte
 *							and strict Intl Mgr flag is set.
 *		11/30/00	kwk	If invalid double-byte, keep using sub-font and invalid
 *							low byte to match behavior of draw/measure code.
 *
 ***********************************************************************/
static UInt16 PrvMultibyteWrap(const Char* iStringP, UInt16 iMaxWidth)
{
	const Char* textP = iStringP;
	const FontMapType* fontMapP;
 	UInt16 lineWidth = 0;
	UInt16 charSize;
	UInt32 offset;
	
	// The font map follows the regular font header information.
	fontMapP = (const FontMapType*)(UICurrentFontPtr+1);

	// As soon as lineWidth is == or > iMaxWidth, we can stop looping. When
	// we call TxtGetWordWrapOffset, we only back up the break offset if we
	// exceeded the iMaxWidth limit. Note that this isn't the same as the
	// original FntWordWrap algorithm, which would always back up even if
	// lineWidth == iMaxWidth, thus not using the full width of the display.
	while (lineWidth < iMaxWidth)
	{
		UInt8 curChar = *textP++;
		
		// A null (end of string) immediately terminates processing, and
		// the offset is before (to the left of) the null byte.
		if (curChar == chrNull)
		{
			return(textP - iStringP - 1);
		}
		
		// A linefeed immediately terminates processing, and the offset is
		// to the right of the linefeed character.
		else if (curChar == chrLineFeed)
		{
			return(textP - iStringP);
		}

		// A tab's width is the distance from the current line width to the
		// next tab-stop position (currently fixed at 20 pixels).		
		else if (curChar == chrTab)
		{
			lineWidth += fntTabChrWidth - (lineWidth % fntTabChrWidth);
			charSize = 1;
		}
		
		// We've got a regular character, so figure out its width.
		else 
		{
			const FontType* fontP = UIFontListPtr[fontMapP[curChar].value];
			const FontCharInfoType* charInfoP;
			UInt8 firstChar;
			UInt8 lastChar;
			Int8 width;
	
			// See if we've got a multi-byte character.
			if (fontMapP[curChar].state == fntStateNextIsChar)
			{
				curChar = *textP++;
				if ((curChar == chrNull)
#if (ERROR_CHECK_LEVEL == ERROR_CHECK_FULL)
				|| ((TxtByteAttr(curChar) & byteAttrLast) == 0))
#else
				|| (false))
#endif
				{
					// We've got an invalid low byte, treat it as a single byte
					// character. Note that we'll continue to use the sub-font,
					// and the invalid character index, since the font should
					// have a missing character defined at that location.
					charSize = 1;
					textP--;
					
#if (ERROR_CHECK_LEVEL == ERROR_CHECK_FULL)
					if ((GIntlData->intlFlags & kStrictChecksFlag) != 0)
					{
						ErrNonFatalDisplay("Wrapping an invalid double-byte character");
					}
#endif
				}
				else
				{
					charSize = 2;
				}
			}
			else
			{
				charSize = 1;
			}
			
			firstChar = fontP->firstChar;
			lastChar = fontP->lastChar;
			charInfoP = ((const FontCharInfoType*)(&fontP->owTLoc) + fontP->owTLoc);
			
			if ((curChar >= firstChar) && (curChar <= lastChar))
			{
				width = charInfoP[curChar - firstChar].width;
				//	If the character is missing return the width of the Missing Char Symbol, 
				// which is located at index=last-first+1.
				if (width == fntMissingChar) 
				{
					width = charInfoP[lastChar - firstChar + 1].width;
				}
			}
			else
			{
				width = charInfoP[lastChar - firstChar + 1].width;
			}
			
			lineWidth += width;
		}
	}

	// Figure out the break offset. If not even one character fit, then take
	// the last character anyway. We still want to call TxtGetWordWrapOffset
	// so that trailing spaces/line feed get added to the line.
	offset = textP - iStringP;
	if (lineWidth > iMaxWidth)
	{
		offset -= charSize;
		if (offset == 0)
		{
			offset = charSize;
		}
	}
	
	// We've exceeded the width of the line, before hitting the end of the
	// string or encountering a linefeed character. Use TxtGetWordWrapOffset to
	// determine the appropriate break position. Note that even though the font
	// data drives the character processing, TxtGetWordWrapOffset gets called here.
	// Since all of the Text Manager routines assume that the text being processed
	// uses the device's character encoding, this can create odd results if a Shift-
	// JIS font is being used on a Latin device, or vice-versa. The text will draw
	// and measure correctly, but word wrapping will not be correct. One final note
	// is that 7-bit ASCII text will generally work properly in either case, which
	// is a feature that's exploited by the Japanese WebLib code to render Latin-
	// encoded pages on a Japanese device.
	return(TxtGetWordWrapOffset(iStringP, offset));
} // PrvMultibyteWrap


/***********************************************************************
 *
 * FUNCTION: FntWordWrap
 *
 * DESCRIPTION: Given a string, determine the number of bytes that
 *		can be displayed within the specified width.
 *
 * PARAMETERS:
 *		iStringP		 -> pointer to a null-terminated string
 *		iMaxWidth	 -> maximum line width in pixels
 *
 * RETURNED:
 *		length of the line in bytes
 *
 * HISTORY:
 *		05/23/96	art	Created by Art Lamb.
 *		02/16/99	kwk	Modified to work for multibyte fonts.
 *		05/29/00	kwk	Re-written to use TxtGetWordWrapOffset.
 *		07/29/00	kwk	Catch case of not even one character fitting on a line.
 *
 ***********************************************************************/
UInt16 FntWordWrap(const Char* iStringP, UInt16 iMaxWidth)
{
 	UInt16	lineWidth;
	UInt8		curChar;
	UInt8		firstChar;
	UInt8		lastChar;
	FontCharInfoType* charInfo;
	const Char* textP = iStringP;
	UInt32	offset;
	
	ErrNonFatalDisplayIf(iStringP == NULL, "Null parameter");
	
	// Immediately handle weird case, so our loop can assume that at least
	// one character gets processed.
	if (iMaxWidth == 0)
	{
		return(0);
	}
	
	if (((UInt16)UICurrentFontPtr->fontType & fntFontMapMask) == fntFontMapMask)
	{
		return(PrvMultibyteWrap(iStringP, iMaxWidth));
	}
	
	lineWidth = 0;

	firstChar = UICurrentFontPtr->firstChar;
	lastChar = UICurrentFontPtr->lastChar;
	charInfo = ((FontCharInfoType *) (&UICurrentFontPtr->owTLoc) + 
			UICurrentFontPtr->owTLoc);
	
	// As soon as lineWidth is == or > iMaxWidth, we can stop looping. When
	// we call TxtGetWordWrapOffset, we only back up the break offset if we
	// exceeded the iMaxWidth limit. Note that this isn't the same as the
	// original FntWordWrap algorithm, which would always back up even if
	// lineWidth == iMaxWidth, thus not using the full width of the display.
	while (lineWidth < iMaxWidth)
	{
		curChar = *textP++;
		
		// A null (end of string) immediately terminates processing, and
		// the offset is before (to the left of) the null byte.
		if (curChar == chrNull)
		{
			return(textP - iStringP - 1);
		}
		
		// A linefeed immediately terminates processing, and the offset is
		// to the right of the linefeed character.
		else if (curChar == chrLineFeed)
		{
			return(textP - iStringP);
		}

		// A tab's width is the distance from the current line width to the
		// next tab-stop position (currently fixed at 20 pixels).		
		else if (curChar == chrTab)
		{
			lineWidth += fntTabChrWidth - (lineWidth % fntTabChrWidth);
		}
		
		// We've got a regular character, so figure out its width.
		else 
		{
			Int8 width;
			
			if ((curChar >= firstChar) && (curChar <= lastChar))
			{
				width = charInfo[curChar - firstChar].width;
				//	If the character is missing return the width of the Missing Char Symbol, 
				// which is located at index=last-first+1.
				if (width == fntMissingChar) 
				{
					width = charInfo[lastChar - firstChar + 1].width;
				}
			}
			else
			{
				width = charInfo[lastChar - firstChar + 1].width;
			}
			
			lineWidth += width;
		}
	}

	// Figure out the break offset. If not even one character fit, then take
	// the last character anyway. We still want to call TxtGetWordWrapOffset
	// so that trailing spaces/line feed get added to the line.
	offset = textP - iStringP;
	if (lineWidth > iMaxWidth)
	{
		offset -= 1;
		if (offset == 0)
		{
			offset = 1;
		}
	}
	
	// We've exceeded the width of the line, before hitting the end of the
	// string or encountering a linefeed character. Use TxtGetWordWrapOffset to
	// determine the appropriate break position.
	return(TxtGetWordWrapOffset(iStringP, offset));
} // FntWordWrap


/***********************************************************************
 *
 * FUNCTION:    FntWordWrapReverseNLines
 *
 * DESCRIPTION: This routine word wraps a text string backwards by the 
 *              number of lines spacified.  The character position of the 
 *              start of the first line and the number of lines we actually
 *              word wraped are returned.
 *
 * PARAMETERS:	chars          - pointer to a null-terminated string
 *             maxWidth       - maximum line width in pixels
 *             linesToScrollP - passed: lines to scroll
 *                              returned: lines scrolled
 *             scrollPosP       passed: first character
 *                              returned: first character after wrapping
 *
 * RETURNED:	nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/18/96	Initial Revision
 *
 ***********************************************************************/
void FntWordWrapReverseNLines (Char const * chars, UInt16 maxWidth, 
	UInt16 * linesToScrollP, UInt16 * scrollPosP)
{
	UInt16 start;
	UInt16 startOfLine;
	UInt16 endOfPrevLine;
	UInt16 lineStart;
	UInt16 lineCount = 0;
 	UInt16 linesToScroll;


	ErrNonFatalDisplayIf(chars == NULL, "Null chars");
	
	linesToScroll = *linesToScrollP;
	startOfLine = *scrollPosP;

   endOfPrevLine = startOfLine - 1;
	if (endOfPrevLine && chars[endOfPrevLine] == linefeedChr)
		endOfPrevLine--;

	// Find the first line that begins after after a linefeed and is at 
	// or before the line we wish to scroll to.
	do 
		{
		// Search backward for a linefeedChr.  If we find one, our staring
		// postion is the character after the linefeedChr, otherwise out 
		// starting position is the start of the field's text.
		if (FindReverseByte (chars, linefeedChr, endOfPrevLine, &start))
	      start++;					// skip over the linefeedChr
	    else
			start = 0;				// start of text string

		// Count the number of lines we move back through.
		lineStart = start;
		while (lineStart < startOfLine)
			{
     		lineCount++;
      	lineStart += FntWordWrap (&chars[lineStart], maxWidth);
			}

		if (start <= 1) 
			{
			// Is the first line a linefeed?
			if (start == 1)
				{
				start = 0;
				lineCount++;
				}
			break;
			}

		startOfLine = start;
	   endOfPrevLine = startOfLine - 2;		// move before linefeed	
		}
	while (lineCount < linesToScroll);

	// If we're moved back to many line, move foreward until we're at the 
	// correct line.
	while (lineCount > linesToScroll)
		{
      start += FntWordWrap (&chars[start], maxWidth);		
		lineCount--;
		}

	if (lineCount < linesToScroll)
		*linesToScrollP = lineCount;

	*scrollPosP = start;
}
	 

/***********************************************************************
 *
 * FUNCTION:    FntGetScrollValues
 *
 * DESCRIPTION: Given a string and a position within the sting this 
 *              routine returns the values necessary to update a 
 *              scroll bar.
 *
 * PARAMETERS:  chars     - null-terminated string
 *              width     - width to word wrap at, in pixels
 *              scrollPos - character position of the first visible character
 *              linesP    - (returned) number of lines of text
 *              topLineP  - (returned) top visible line
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/7/96	Initial Revision
 *			grant 2/4/99	removed unnecessary increment of topLine
 *			grant 2/12/99	Changed the order of events in the loop so that
 *                      topLine is set after length and lines.  This
 *                      solves a problem with single-line fields - in
 *                      that case, the loop was only executed once and
 *                      topLine got a value one less than it should have.
 *                      (Which explains the old "unnecessary" increment.)
 *
 ***********************************************************************/
void FntGetScrollValues (Char const * chars, UInt16 width, 
	UInt16 scrollPos, UInt16 * linesP, UInt16 * topLineP)
{
	UInt16 length = 0;
	UInt16 lines = 0;
	UInt16 topLine = 0;
	
	ErrNonFatalDisplayIf(chars == NULL, "Null chars");
	
	if (*chars)
		{
		do {
			length += FntWordWrap (&chars[length], width);
			lines++;
			if (length <= scrollPos)
				topLine = lines;
			}
		while (chars[length]);
	
		// If the text end with a linefeed add one to the height.
		if (length && chars[length-1] == linefeedChr) 
			lines++;
		
		}

	*topLineP = topLine;
	*linesP = lines;
}
