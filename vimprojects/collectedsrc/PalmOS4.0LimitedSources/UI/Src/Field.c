/******************************************************************************
 *
 * Copyright (c) 1994-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: Field.c
 *
 * Release: 
 *
 * Description:
 *	  This file contains the field object routines.
 *
 * History:
 *		11/19/94	art	Created by Art Lamb
 *		03/14/99	bob	update for color
 *		08/31/99	gap	Fix cursor/hilite handling for double-tap selection in FldHandleEvent()
 *		09/01/99	gap	Add full line selection for triple tap in CalculateSelection()
 *		10/31/00	kwk	Added checks for null FieldType* param to all externally
 *							visible routines. Use simple null check for simple getter
 *							routines, otherwise call full validation routine.
 *
 *****************************************************************************/

#define	NON_PORTABLE

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_FIELDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_MENUS
#define ALLOW_ACCESS_TO_INTERNALS_OF_CLIPBOARDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS

#include <PalmTypes.h>
#include <SystemPublic.h>

#include <PalmUtils.h>

#include "SysEvtMgr.h"
#include "SystemPrv.h"
#include "SysTrapsFastPrv.h"
#include "TextServicesPrv.h"

#include "Form.h"
#include "UIGlobals.h"
#include "UIColor.h"
#include "UIColorPrv.h"
#include "UIResourcesPrv.h"

// This is the value used in fldP->insPtYPos to indicate that the
// fldP->insPtXPos field contains a byte offset from the beginning
// of the text to the insertion point, versus a byte offset from
// the beginning of the line. When this is set, then the insertion
// point visible attribute flag better be off.
#define	absolutePosInX	0x8000

/***********************************************************************
 *
 * Static functions
 *
 ***********************************************************************/

static Boolean	AtEndOfLine (const FieldType * fldP, UInt16 pos);
static void 	CalculateSelection(const FieldType* fldP, UInt16* startPos, UInt16* endPos, UInt8 tapCount );
static void		CancelSelection ( FieldType * fldP);
static Boolean	CompactFieldHeight (FieldType * fldP, UInt16 pos);
static Boolean ConstrainXPos(FieldType* fldP);
static void		DeleteFromText (FieldType * fldP, UInt16 startPos, UInt16 charsToDelete);
static Boolean	ExpandFieldHeight (FieldType * fldP, UInt16 pos);
static Boolean	ExpandTextString (FieldType * fldP, UInt16 newSize);
static UInt16	FormatLine (const FieldType * fldP, UInt16 start);
static void 	FormatAndDrawToFill (FieldType * fldP, Boolean draw);
static void 	FormatAndDrawLines (FieldType * fldP, Boolean draw);
static void 	FormatAndDrawFromPos (FieldType * fldP, UInt16 pos, Boolean draw);
static void 	FormatAndDrawLineIfChanged (FieldType * fldP, UInt16 lineNumber);
static void 	FormatAndDrawIfWrappingChanged (FieldType * fldP, UInt16 lineNumber);
static UInt16	GetLineCharsWidth (const FieldType * fldP, UInt16 lineNum);
static UInt16	GetStringPosOfInsPtPos (const FieldType * fldP);
static Boolean FindReverseChar (Char * text, Char charToFind, UInt16 len, UInt16 * pos);
static UInt16	GetVisibleCharsInLine (const FieldType * fldP, UInt16 lineNum);
static UInt16	GetVisibleLines (const FieldType * fldP);
static void		InsPtValid (FieldType * fldP);
static Boolean	IsSelection (const FieldType * fldP);
static void		SetBlinkingInsPtLocation (FieldType * fldP);
static void		SetInsPtFromStringPos (FieldType * fldP, UInt16 pos);
static UInt16	StartOfLine (const FieldType * fldP, UInt16 pos);
static UInt16	StartOfPreviousLine (const FieldType * fldP, UInt16 startOfLine);
static Boolean TextServicesHandleEvent (FieldType * fldP, EventType * eventP);
static void		TextServicesReset (FieldType * fldP, Boolean doUpdate);
static void		TextServicesValidate(FieldType * fldP, TsmFepStatusType* status);
static void		UpdateStartOfLines (FieldType * fldP, Int16 amount, UInt16 pos);
static void 	ModifyText (FieldType * fldP, UInt16 offset, const Char * charsP, UInt16 len);
static void 	UndoReset (void);
static Boolean	UndoRecordTyping (FieldType * fldP,  UInt16 pos);
static Boolean	UndoRecordBackspaces (FieldType * fldP, UInt16 pos);
static Boolean	UndoRecordPaste (FieldType * fldP, UInt16 pos);
static void 	UndoRecordCut (FieldType * fldP);

#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	#define	ECTextServices	TextServicesValidate
	#define	ECValidateField	ValidateField
	#define	ECNullFieldPtr	CheckNullFieldPtr
#else
	#define	ECTextServices(fldP, status)
	#define	ECValidateField(fldP, checkLock, checkSel)
	#define	ECNullFieldPtr(fldP)
#endif



#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
/***********************************************************************
 *
 * FUNCTION:    CheckNullFieldPtr 
 *
 * DESCRIPTION: Make sure the field pointer isn't NULL. We use this
 *		routine versus ErrNonFatalDisplayIf(xxx, "NULL parameter") because
 *		this saves a bit of ROM space.
 *
 * PARAMETERS:	fldP  		pointer to a FieldType structure.
 *
 * RETURNED:	 nothing
 *
 * HISTORY:
 *		10/31/00	kwk	Created by Ken Krugler.
 *
 ***********************************************************************/
static void CheckNullFieldPtr(const FieldType* fldP)
{
	ErrNonFatalDisplayIf(fldP == NULL, "NULL field");
} // 	CheckNullFieldPtr


/***********************************************************************
 *
 * FUNCTION:    ValidateField 
 *
 * DESCRIPTION: This routine performs various edit checks on a field 
 *              object.
 *
 * PARAMETERS:	fldP  		pointer to a FieldType structure.
 *					checkLock	T=>check text handle lock status.
 *					checkSel		T=>check selection/insertion point.
 *
 * RETURNED:	 nothing
 *
 * HISTORY:
 *		06/23/95	art	Initial Revision
 *		10/27/98	bob	Careful lock counting w/editable cases.
 *		12/05/98	kwk	Beefed up selection range/ins pt checks.
 *		12/06/98	kwk	Added checkSel param, since some calls
 *							to us are only for checking handle/ptr data.
 *		04/19/99	grant	Make sure text actually fits within textBlockSize
 *		09/15/99	kwk	Skip blocksize warning if textLen & block size are zero.
 *							Also skip string length check if blocksize is zero.
 *
 ***********************************************************************/
static void ValidateField (const FieldType * fldP, Boolean checkLock, Boolean checkSel)
{
	UInt16 i;
	UInt16 pos;
	UInt16 numLines;
	FontID curFont;
	
	
	ErrNonFatalDisplayIf(fldP == NULL, "NULL field");
	
	curFont = FntSetFont (fldP->fontID);	

	// The field has to be tall enough to display at 
	numLines = GetVisibleLines (fldP);

	// Check that the insertion point position is reasonable.
	if (fldP->attr.insPtVisible && checkSel)
		{
		ErrFatalDisplayIf (fldP->insPtYPos >= numLines, 
			"Invalid insertion point position");
		}
	
	if (fldP->text)
		{
		UInt32 charStart, charEnd;

		if (checkLock)
			{
			ErrNonFatalDisplayIf(fldP->textHandle && fldP->attr.editable &&
				MemHandleLockCount(fldP->textHandle) != 1, 
				"Field object improperly locked");
			}
		
		// Some apps use a zero-size field to hold empty strings, versus
		// a one-byte field with a null char, so don't complain if both
		// the block size and the text length are zero.
		ErrNonFatalDisplayIf((fldP->textHandle != NULL) 
				&& (fldP->textBlockSize <= fldP->textLen)
				&& (fldP->textLen > 0),
			"Text block size smaller than text");

		// Same comment as above; don't check the string length if the app
		// is using a zero-size block for an empty field.
		ErrFatalDisplayIf((fldP->textBlockSize > 0)
				&& (StrLen(fldP->text) != fldP->textLen), 
			"Invalid field length");
		
		if (checkSel)
			{
			UInt16 pos;
			
			if (IsSelection(fldP))
				{
				ErrFatalDisplayIf (fldP->selFirstPos > fldP->selLastPos, 
					"selection range start > end");
				
				ErrFatalDisplayIf (fldP->selFirstPos > fldP->textLen, 
					"Invalid selection range start");
				TxtCharBounds(fldP->text, fldP->selFirstPos, &charStart, &charEnd);
				ErrFatalDisplayIf (fldP->selFirstPos != charStart, 
					"Selection range start inside of character");
				
				ErrFatalDisplayIf (fldP->selLastPos > fldP->textLen, 
					"Invalid selection range end");
				TxtCharBounds(fldP->text, fldP->selLastPos, &charStart, &charEnd);
				ErrFatalDisplayIf (fldP->selLastPos != charStart, 
					"Selection range end inside of character");
				}

			// 981210 kwk Always check the insertion point, since it can get 'activated'
			// by calling FldSetSelection with start == end.
			
			pos = fldP->insPtXPos;
			
			// If the insertion point is visible, then the yPos better not be the
			// special 'absolute position' value, and vice versa.
			ErrFatalDisplayIf(fldP->attr.insPtVisible != (fldP->insPtYPos != absolutePosInX),
				"x/y coords but ins pt not visible");
				
			if (fldP->insPtYPos != absolutePosInX)
				pos = GetStringPosOfInsPtPos (fldP);
			
			ErrFatalDisplayIf (pos > fldP->textLen, 
				"Invalid insertion point position");
			TxtCharBounds(fldP->text, pos, &charStart, &charEnd);
			ErrFatalDisplayIf (pos != charStart, 
				"Insertion point inside of character");
			}
		}
	else if (checkSel)
		{
		ErrFatalDisplayIf ( ((fldP->insPtXPos > 0) || (fldP->insPtYPos > 0)),
			"Invalid insertion point position");
		}

	// Validate the word wrapping info.
	if ( (! fldP->attr.singleLine) && (fldP->text))
		{
		ErrFatalDisplayIf (!fldP->lines, "No word wrapping info");
		
		pos = fldP->lines->start;
		for (i = 0; i < numLines; i++)
			{
			ErrFatalDisplayIf (pos != fldP->lines[i].start, 
				"Invalid word wrapping info");
			pos += fldP->lines[i].length;

			ErrFatalDisplayIf (pos > fldP->textLen, 
				"Invalid word wrapping info");
			}
		}
	
	FntSetFont (curFont);
}
#endif


/***********************************************************************
 *
 * FUNCTION:    	DrawChars
 *
 * DESCRIPTION:	Draw text that might contain inline text, for which we'll
 *						need to do special underlinining/colorizing.
 *
 * PARAMETERS:	 	textP			pointer to text.
 *						drawStart	offset to beginning of text to draw. This
 *										gets updated with the amount of text drawn.
 *						drawEnd		offset to end of text to draw.
 *						inlineStart	offset to beginning of inline text.
 *						inlineEnd	offset to end of inline text.
 *						drawX			location (x pos) to draw text.
 *						drawY			location (y pos) to draw text.
 *						selected		True => text being drawn is selected.
 *						hasFocus		True => field has current focus.
 *
 * RETURNED:		width of text drawn.
 *
 * HISTORY:
 *		07/27/99	kwk	Created by Ken Krugler.
 *		08/20/99	kwk	Added support for color inline if depth >= 8bpp.
 *		08/25/99	kwk	Compare FEP vs. std color indexes to decide if we
 *							need to use underlining.
 *		09/12/99	kwk	Don't do special inline processing if field doesn't have focus.
 *		10/06/99	kwk	Use form frame color for underlining.
 *		10/09/99	kwk	OK, all right, use new UI color for underlining conversion text.
 *		09/26/00	kwk	Handle clause offset, so that clause can be someplace
 *							other than the beginning of the inline text.
 *
 ***********************************************************************/
static Int16 DrawChars(	const Char* textP,
								UInt16* drawStart,
								UInt16 drawEnd,
								UInt16 lineStart,
								Coord drawX,
								Coord drawY,
								Boolean selected,
								Boolean hasFocus)
{
	Int16 chunkWidth;
	UInt16 bytesToDraw;
	UInt16 underlineSize = 0;
	UInt16 inlineStart, inlineEnd;
	UInt16 clauseStart, clauseEnd;
	UInt16 originalEnd = drawEnd;
	UnderlineModeType savedUnderline;
	IndexedColorType savedTextColor;
	IndexedColorType savedBackColor;
	IndexedColorType savedForeColor;
	IndexedColorType colorIndex;
	Boolean changedTextColor = false;
	Boolean changedBackColor = false;
	Boolean changedForeColor = false;
	
	if (*drawStart == drawEnd) {
		return(0);
	}

	if ((GTsmFepLibRefNum == sysInvalidRefNum) || (!GInlineActive) || (!hasFocus)) {
		inlineStart = drawEnd;
		inlineEnd = drawEnd;
		clauseStart = drawEnd;
		clauseEnd = drawEnd;
	} else {
		inlineStart = (GInlineStart > lineStart) ? GInlineStart - lineStart : 0;
		inlineEnd = (GInlineEnd > lineStart) ? GInlineEnd - lineStart : 0;
		clauseStart = GInlineStart + GInlineClauseOffset;
		clauseStart = (clauseStart > lineStart) ? clauseStart - lineStart : 0;
		clauseEnd = GInlineStart + GInlineClauseOffset + GInlineClauseLen;
		clauseEnd = (clauseEnd > lineStart) ? clauseEnd - lineStart : 0;
	}

	// Figure out which segment we're drawing - regular text completely before or
	// after the inline area, regular text that occurs immediately before the start
	// of inline, but extends into the inline area, raw inline text before the
	// clause (converted) text, converted text, or raw inline text after the
	// clause text (which may be followed by more regular text after the inline area).
	if ((drawEnd <= inlineStart) || (*drawStart >= inlineEnd)) {
		// drawEnd is set up.
	} else if (*drawStart < inlineStart) {
		drawEnd = min(drawEnd, inlineStart);
	} else if ((*drawStart < clauseStart) || (*drawStart >= clauseEnd)) {
		// We're drawing raw inline text, either before the start of the clause,
		// or after the end of the clause. This is also the case if there is
		// no clause text, and thus GInlineClauseLen == 0.
		if (*drawStart < clauseStart) {
			drawEnd = min(clauseStart, drawEnd);
		} else {
			drawEnd = min(inlineEnd, drawEnd);
		}
		
		// If we have a different text color, then go use it.
		colorIndex = UIColorGetIndex(UIFieldFepRawText);
		if (colorIndex != UIColorGetIndex(UIFieldText)) {
			savedTextColor = WinSetTextColor(colorIndex);
			changedTextColor = true;
		}
		
		// If we're selected, just use the standard selection color, which means
		// that the Fep raw text color better work well with it.
		if (!selected) {
			colorIndex = UIColorGetIndex(UIFieldFepRawBackground);
			if (colorIndex != UIColorGetIndex(UIFieldBackground)) {
				savedBackColor = WinSetBackColor(colorIndex);
				changedBackColor = true;
			}
		}
		
		// If both the text & background colors are unchanged, then we need some
		// other way to specify converted text, so underline it.
		// DOLATER kwk - if we're on a color device, and the text color
		// is the same as normal, and the text is selected, then we'll always
		// wind up underlining even if our raw background color is different.
		if (!changedTextColor && !changedBackColor) {
			savedForeColor = WinSetForeColor(UIColorGetIndex(UIFieldFepUnderline));
			savedUnderline = WinSetUnderlineMode(solidUnderline);
			underlineSize = 1;
			changedForeColor = true;
		}
	} else {
		drawEnd = min(clauseEnd, drawEnd);

		// If we have a different text color, then go use it.
		colorIndex = UIColorGetIndex(UIFieldFepConvertedText);
		if (colorIndex != UIColorGetIndex(UIFieldText)) {
			savedTextColor = WinSetTextColor(colorIndex);
			changedTextColor = true;
		}
		
		// If we have a different background color, then go use it.
		colorIndex = UIColorGetIndex(UIFieldFepConvertedBackground);
		if (colorIndex != UIColorGetIndex(UIFieldBackground)) {
			savedBackColor = WinSetBackColor(colorIndex);
			changedBackColor = true;
		}
		
		// If both the text & background colors are unchanged, then we need some
		// other way to specify converted text, so underline it.
		if (!changedTextColor && !changedBackColor) {
			savedForeColor = WinSetForeColor(UIColorGetIndex(UIFieldFepUnderline));
			savedUnderline = WinSetUnderlineMode(noUnderline);
			underlineSize = 2;
			changedForeColor = true;
		}
	}
	
	bytesToDraw = drawEnd - *drawStart;
	WinDrawChars(textP + *drawStart, bytesToDraw, drawX, drawY);
	chunkWidth = FntLineWidth(textP + *drawStart, bytesToDraw);
	
	// DOLATER kwk - will WinDrawRectangle use the same color as the
	// solid text underline? If not, need to set it up. We might also
	// want to use a different color for the underline, to make it easier
	// to see the difference on a 4bpp display.
	if (underlineSize == 2) {
		RectangleType r;
		r.topLeft.x = drawX;
		r.topLeft.y = drawY + FntLineHeight () - underlineSize;
		r.extent.x = chunkWidth;
		r.extent.y = underlineSize;
		
		// If the converted clause text is preceeded by raw inline text,
		// leave a small pixel gap at the start of the bold underlining.
		if (GInlineClauseOffset > 0) {
			r.topLeft.x += 1;
			r.extent.x -= 1;
		}
		
		// If converted clause text is followed by raw inline text, leave a small
		// pixel gap between the two underlines.
		if ((clauseEnd < inlineEnd)
		&& (originalEnd > clauseEnd)) {
			r.extent.x -= 1;
		}
		
		WinDrawRectangle (&r, 0);
	}

	// We changed the underline mode if we had to draw either raw or converted
	// text, so reset it now.
	if (underlineSize > 0) {
		WinSetUnderlineMode(savedUnderline);
	}
	
	if (changedTextColor) {
		WinSetTextColor(savedTextColor);
	}
	
	if (changedBackColor) {
		WinSetBackColor(savedBackColor);
	}
	
	if (changedForeColor) {
		WinSetForeColor(savedForeColor);
	}
	
	// Advance the text offset to the end of what we just handled.
	*drawStart += bytesToDraw;
	return(chunkWidth);
} // DrawChars


/***********************************************************************
 *
 * FUNCTION:    DrawOneLineRange
 *
 * DESCRIPTION: Draw a specified characer range on the specified line.
 *
 * PARAMETERS:	 fldP      pointer to a FieldType structure.
 *              lineNum  line number
 *              startPos character position of start of range
 *              endPos   character position of end of range.
 *              erase	 true to erase to end of line
 *              selected true to draw in highlight colors
 *
 * RETURNED:	 nothing.
 *
 * HISTORY:
 *		11/28/98	art	Created by Art Lamb.
 *		??/??/99	bob	Rewrote it for color support.
 *		07/04/99	kwk	Only error-check computed text rect if not single-line.
 *		07/28/99	kwk	Moved drawing guts into DrawChars, for inline support.
 *		11/07/99	gap	Fix highlight to also include tabs, spaces, and carriage returns.
 *		11/09/99	gap	Added changes from code review, and bug fix for text display when 
 *							fldP->rect.topLeft.x != 0.
 *
 ***********************************************************************/
static void DrawOneLineRange (FieldType * fldP, UInt16 lineNum, UInt16 startPos,
	UInt16 endPos, Boolean erase, Boolean selected)
{
	Char *			chars;
	UInt16			lineLength;
	UInt16			lineStart;
	UInt16			chunkEndPos;
	RectangleType	r;
	RectangleType	savedClipR;
	RectangleType	clip;
	Coord				curXPos;
	UInt16			origEndPos;
	UInt16			origLineLength;
	
	// get strings relative to current line
	if (fldP->lines == NULL) {
		chars = fldP->text;
		lineLength = fldP->textLen;
		lineStart = 0;
		}
	else {
		ErrNonFatalDisplayIf(fldP->attr.singleLine, "Single line field has lines struct!");
		lineStart = fldP->lines[lineNum].start;
		chars = &fldP->text[lineStart];
		startPos -= lineStart;
		endPos -= lineStart;
		lineLength = fldP->lines[lineNum].length;
	}

	// storing off the original values of these fields for carriage return hilight calculation later.
	origEndPos = endPos;
	origLineLength = lineLength;

	// if we're drawing to end of line, ignore some trailing invisible characters when necessary
	if (endPos == lineLength)
		{
		if (fldP->attr.justification == rightAlign)
			{
			while (endPos > startPos &&
					 (chars[endPos-1] == spaceChr || chars[endPos-1] == tabChr || chars[endPos-1] == linefeedChr))
				endPos--;
			lineLength = endPos;
			}
		else
			{
			if ((endPos > startPos) && (chars[endPos-1] == linefeedChr))
				endPos--;
			lineLength = endPos;
			}
		}

	// figure out text rectangle
	r.topLeft.x = fldP->rect.topLeft.x;
	r.extent.y = FntLineHeight ();
	r.topLeft.y = fldP->rect.topLeft.y + (lineNum * r.extent.y);
	
	// if no text, it's easy
	if (endPos == startPos)
		r.extent.x = 0;

	// right alignment done by indenting from right edge (tabs ignored)
	else if (fldP->attr.justification == rightAlign) {
		r.extent.x = FntCharsWidth (&chars[startPos], endPos - startPos);
		r.topLeft.x += fldP->rect.extent.x - r.extent.x;
				
		// if we're not at the right edge, subtract space for the remaining text on the line
		if (endPos < lineLength)
			r.topLeft.x -= FntCharsWidth (&chars[endPos], lineLength - endPos);
	}
	
	// left aligned, skip over non-drawn text (tabs considered)
	else {
		if (startPos > 0) {
			r.extent.x = FntLineWidth(chars, startPos);
			r.topLeft.x += r.extent.x;
			r.extent.x = FntLineWidth(chars, endPos) - r.extent.x;
		}
		else
			r.extent.x = FntLineWidth(chars, endPos);
	}
	
	// a couple of sanity checks
	if (!fldP->attr.singleLine) {
		ErrNonFatalDisplayIf(r.topLeft.x < fldP->rect.topLeft.x,
									"Computed rectangle too far left");
									
		// removed this sanity check as the text extent could be greater than the field's
		// extent as a result of a large number of spaces between to line-wrapped words.
		// The extra spaces will be cropped off in the character blitter routine.
//		ErrNonFatalDisplayIf(r.topLeft.x + r.extent.x > fldP->rect.topLeft.x + fldP->rect.extent.x,
//									"Computed rectangle too wide");
	}
	
	// finally, we can start drawing
	WinPushDrawState();	
	WinGetClip (&savedClipR);
	RctCopyRectangle (&(fldP->rect), &clip);
	WinClipRectangle (&clip);
	WinSetClip (&clip);
	WinSetUnderlineMode ( (UnderlineModeType) fldP->attr.underlined);

	// set up colors
	if (selected) {
		if (fldP->attr.editable) {
			WinSetTextColor(UIColorGetIndex(UIFieldTextHighlightForeground));
			WinSetForeColor(UIColorGetIndex(UIFieldTextHighlightForeground));
			WinSetBackColor(UIColorGetIndex(UIFieldTextHighlightBackground));
		}
		else {
			WinSetTextColor(UIColorGetIndex(UIObjectSelectedForeground));
			WinSetForeColor(UIColorGetIndex(UIObjectSelectedForeground));
			WinSetBackColor(UIColorGetIndex(UIObjectSelectedFill));
		}
	}
	else {
		if (fldP->attr.editable) {
			WinSetTextColor(UIColorGetIndex(UIFieldText));
			WinSetForeColor(UIColorGetIndex(UIFieldTextLines));
			WinSetBackColor(UIColorGetIndex(UIFieldBackground));
		}
		else {
			WinSetTextColor(UIColorGetIndex(UIObjectForeground));
			WinSetForeColor(UIColorGetIndex(UIObjectFrame));
			// don't set background color, use whatever was there
		}
	}

	// if underlined, decrease the height of the rectangle to avoid underline flicker
	if (fldP->attr.underlined)
		r.extent.y--;

	// Loop over the text, drawing chunks. We have to segment by tabs (if not right-
	// justified), and also by regular versus inline (converted) versus inline (raw).
	curXPos = r.topLeft.x;
	
	while (startPos < endPos) {
		// if right aligned, just draw with no tabs (above align calculation doesn't use them)
		if (fldP->attr.justification == rightAlign) {
			curXPos += DrawChars(chars, &startPos, endPos, lineStart, curXPos, r.topLeft.y, selected, fldP->attr.hasFocus);
		}
	
		// left aligned, so handle tabs. We might have to repeat this scanning if we're drawing
		// segments of regular vs. inline text, but that's an unusual case when combined w/a tab.
		// Note that a tab character will never be part of a multi-byte character, so we can do
		// this byte-by-byte scan.
		else {
			chunkEndPos = startPos;
			while ((chunkEndPos < endPos) && (chars[chunkEndPos] != chrHorizontalTabulation)) {
				chunkEndPos++;
			}
			
			curXPos += DrawChars(chars, &startPos, chunkEndPos, lineStart, curXPos, r.topLeft.y, selected, fldP->attr.hasFocus);
			
			// If we found a tab & have drawn all of the text up to it, erase & potentially underline it.
			if ((startPos == chunkEndPos) && (chunkEndPos < endPos)) {
				r.topLeft.x = curXPos;
				r.extent.x = fntTabChrWidth - ((r.topLeft.x - fldP->rect.topLeft.x) % fntTabChrWidth);
				
				switch(fldP->attr.underlined)
				{
					case grayUnderline:
						WinDrawGrayLine(r.topLeft.x, r.topLeft.y + r.extent.y, 
								r.topLeft.x + r.extent.x - 1, r.topLeft.y + r.extent.y);
						break;
					case solidUnderline:
					case colorUnderline:
						WinDrawLine(r.topLeft.x, r.topLeft.y + r.extent.y, 
								r.topLeft.x + r.extent.x - 1, r.topLeft.y + r.extent.y);
						break;
				}
				
				WinEraseRectangle (&r, 0);
				
				curXPos += r.extent.x;
				startPos += sizeOf7BitChar(chrHorizontalTabulation);
			}
		}
	} // while

	// Paint the carriage return if
	//		- there is a range of characters selected   			(startPos != origEndPos)
	//		& the selection range contains a carriage return	(chars[origEndPos-1] == linefeedChr)
	//
	// The carriage return is always processed for left justified text.  However, if the text is
	// right justified, only hilight the carriage return if it is the only character on the line
	// to prevent the appearance of discontinuous selections.  Otherwise ignore it and let the text
	// draw directly up to the right of the field bounds
	//
	// DOLATER gap  - we may only need to do this if the text is selected.  I am currently always
	// painting the carriage with the background color to avoid leaving the bits of the higlight
	// around after deselecting text in the event there are calls to DrawOneLineRange() to clear
	// a selection but the erase parameter has not been set to TRUE. 
	if ( (startPos != origEndPos) && (chars[origEndPos-1] == linefeedChr) &&
		  ( (fldP->attr.justification == leftAlign) || 
			 (fldP->attr.justification == rightAlign) && (origLineLength == 1)) )
		{
			// reset r to be the bounds of the carriage return.
		if (fldP->attr.justification == leftAlign)
			{
  			if ((startPos > 0) && (startPos == endPos))
				// In this case r.topLeft.x has not been initialized. Need to do it here.
  				r.topLeft.x = fldP->rect.topLeft.x + FntLineWidth(chars, startPos);
  			else
  				r.topLeft.x += r.extent.x;
			r.extent.x = FntCharWidth(linefeedChr);
			curXPos += r.extent.x;
			}
		else
			{
			r.extent.x = FntCharWidth(linefeedChr);
			r.topLeft.x = fldP->rect.topLeft.x + (fldP->rect.extent.x - r.extent.x);
			}
			
			// Trailing spaces may cause the update region to be outside the bounds 
			// of the field so clip extent of textbound to be within the field bounds.
		if (r.topLeft.x + r.extent.x > fldP->rect.topLeft.x + fldP->rect.extent.x)
			r.extent.x = fldP->rect.extent.x - (r.topLeft.x - fldP->rect.topLeft.x);
			
			// If textbounds intersects the field bounds draw the highlight
		if (r.topLeft.x < fldP->rect.topLeft.x + fldP->rect.extent.x)		
			{
			WinEraseRectangle (&r, 0);
			
			// If the field text is underlined, draw the underline.
			switch(fldP->attr.underlined)
				{
				case grayUnderline:
					WinDrawGrayLine(r.topLeft.x, r.topLeft.y + r.extent.y, 
							r.topLeft.x + r.extent.x - 1, r.topLeft.y + r.extent.y);
					break;
				case solidUnderline:
				case colorUnderline:
					WinDrawLine(r.topLeft.x, r.topLeft.y + r.extent.y, 
							r.topLeft.x + r.extent.x - 1, r.topLeft.y + r.extent.y);
					break;
				}
			}
		}	

	//	Erase the region after the last character of the line.
	if (erase) {
		if (fldP->attr.justification == rightAlign) {
			r.topLeft.x = fldP->rect.topLeft.x;
			r.extent.x = fldP->rect.extent.x - r.extent.x;
		}
		else {
			r.topLeft.x = curXPos;
			r.extent.x = fldP->rect.topLeft.x + fldP->rect.extent.x - r.topLeft.x;
		}
			
		if (r.extent.x > 0) {
			switch(fldP->attr.underlined)
			{
				case grayUnderline:
					WinDrawGrayLine(r.topLeft.x, r.topLeft.y + r.extent.y, 
							r.topLeft.x + r.extent.x - 1, r.topLeft.y + r.extent.y);
					break;
				case solidUnderline:
				case colorUnderline:
					WinDrawLine(r.topLeft.x, r.topLeft.y + r.extent.y, 
							r.topLeft.x + r.extent.x - 1, r.topLeft.y + r.extent.y);
					break;
			}
			WinEraseRectangle (&r, 0);
		}
	}

	WinSetClip (&savedClipR);
	WinPopDrawState();
}


/***********************************************************************
 *
 * FUNCTION:    DrawMultiLineRange
 *
 * DESCRIPTION: Redraw a (multi-line) subset of a field.
 *
 * PARAMETERS:	 fldP  		pointer to a FieldType structure
 * 				 startPos	start of range to redraw
 * 				 endPos		end of range to redraw
 * 				 selected	draw range with highlight color
 *
 * RETURNED:	 nothing.
 *
 * REVISION HISTORY:
 *			Name	Date			Description
 *			----	----			-----------
 *			art	11/21/94		Initial Revision
 *			bob	2/19/99		Made into DrawMultiLineRange from InvertSelection
 *
 ***********************************************************************/
static void DrawMultiLineRange (FieldType * fldP, UInt16 startPos, UInt16 endPos, Boolean selected)
{
	LineInfoType * line;
	UInt16 lineNumber, numLines;
	UInt16 lineStartPos, lineEndPos;

	// single line fields
	if (fldP->attr.singleLine)
		{
		DrawOneLineRange (fldP, 0, startPos, endPos, false, selected);
		}

	// multi-line fields, but don't draw anything if no lines structure (no text exists)
	else if (fldP->lines)
		{
		line = fldP->lines;
		numLines = GetVisibleLines (fldP);

		for (lineNumber = 0; lineNumber < numLines; lineNumber++)
			{
			if ((startPos < line->start + line->length) && 
			    (endPos > line->start))
				{
				lineStartPos = max (startPos, line->start);
				lineEndPos = min ( endPos, line->start + line->length);
				DrawOneLineRange (fldP, lineNumber, lineStartPos, lineEndPos, false, selected);
				}
			line++;
			}
		}
}


/***********************************************************************
 *
 * FUNCTION:    DrawAllLines 
 *
 * DESCRIPTION: Draw entire multi-line field.
 *
 * PARAMETERS:	 fldP		pointer to a FieldType structure.
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/30/95	Initial Revision
 *			bob	2/19/99	Rewrote from DrawLines
 *
 ***********************************************************************/
static void DrawAllLines (FieldType * fldP)
{
	UInt16			lineNum;
	UInt16 			numLines;
	LineInfoType * line;
	
	if (fldP->attr.singleLine) {
		DrawOneLineRange (fldP, 0, 0, fldP->textLen, true, false);
	}
	else {
		numLines = fldP->rect.extent.y / FntLineHeight ();
	
		line = fldP->lines;
		if (line) {
			for (lineNum = 0; lineNum < numLines; lineNum ++) {
				DrawOneLineRange (fldP, lineNum, line->start, line->start+line->length, true, false);
				line++;
			}
		}
		// no line info, must be empty field, so just draw blank lines
		else {
			for (lineNum = 0; lineNum < numLines; lineNum ++) {
				DrawOneLineRange (fldP, lineNum, 0, 0, true, false);
		    }
		}	
	}
	
	// draw selection, if any
	if (IsSelection(fldP))
		DrawMultiLineRange(fldP, fldP->selFirstPos, fldP->selLastPos, true);
}


/***********************************************************************
 *
 * FUNCTION:    FormatLine
 *
 * DESCRIPTION: Given a starting byte offset in the field text, determine 
 *              the number of bytes that can be displayed on a line.
 *
 * PARAMETERS:	 fldP         pointer to a FieldType structure.
 *              start       postion in text string of first character
 *
 * RETURNED:	 length of the line in bytes
 *
 * REVISION HISTORY:
 *		11/21/94	art	Created by Art Lamb.
 *		09/12/00	kwk	Added check for zero-length wrap result.
 *
 ***********************************************************************/
static UInt16 FormatLine(const FieldType* fldP, UInt16 start)
{
	if (start >= fldP->textLen)
	{
		return (0);
	}
	else
	{
		UInt16 length = FntWordWrap (&fldP->text[start], fldP->rect.extent.x);
		if (length == 0)
		{
			ErrNonFatalDisplay("Invalid wrap result");
			length = TxtNextCharSize(fldP->text, start);
		}
		return(length);
	}
} // FormatLine


/***********************************************************************
 *
 * FUNCTION:    FormatAndDrawToFill
 *
 * DESCRIPTION: This routine performs the word wrapping calculation
 *              for dynamicly resizable field that have text scrolled
 *              off the top of the field.  The text is format so the 
 *              as much of the text as posible is visible.
 *
 * PARAMETERS:	 fldP  pointer to a FieldType structure.
 *              draw true if lines should be drawn
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/15/95	Initial Revision
 *
 ***********************************************************************/
static void FormatAndDrawToFill (FieldType * fldP, Boolean draw)
{
	UInt16 numLines;
	UInt16 start, length, i;
	LineInfoType * line;
	LineInfoType * firstLine;
	LineInfoType * lastLine;


	// Single line field should not use this routine.
	ErrFatalDisplayIf( (fldP->attr.singleLine), "Invalid attribute");


	// Format the display lines from the start of the first visible line.
	numLines = GetVisibleLines (fldP);
	line = fldP->lines;
	start = StartOfLine (fldP, line->start);

	for (i = 0; i < numLines; i++)
		{
		length = FormatLine (fldP, start);
		line->start  = start;
		line->length = length;
		start += length;
		line++;
		}


	// Scroll the field down to fill in any blank lines at the end the field.
	firstLine = fldP->lines;

	lastLine = &fldP->lines [numLines-1];
	if (fldP->text[fldP->textLen-1] == linefeedChr)
		lastLine--;
		
	while (firstLine->start && (lastLine->length == 0))
		{
		// Move the display line information down one line.
		//
		MemMove (&fldP->lines[1], fldP->lines, 
			sizeof(LineInfoType) * (numLines-1));
	
		// Format and draw the line which scrolled into view.
		//
		line = fldP->lines;
		line->start = StartOfPreviousLine (fldP, line->start);
		line->length = FormatLine (fldP, line->start);
		}

	if (draw)
		DrawAllLines (fldP);
}


/***********************************************************************
 *
 * FUNCTION:    FormatAndDrawLines
 *
 * DESCRIPTION: Starting with the first visible line, perform the word 
 *              wrapping calculation for each line visible line
 *              and redraw the line the "draw" parameter is true.
 *
 * PARAMETERS:	 fldP  pointer to a FieldType structure.
 *              draw true if lines should be drawn
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/23/94	Initial Revision
 *
 ***********************************************************************/
static void FormatAndDrawLines (FieldType * fldP, Boolean draw)
{
	LineInfoType * line;
	UInt16 numLines, start, length, i;

	// Single line field don't have display formatting information.
	if (fldP->attr.singleLine)
		{
		if (draw)
			DrawOneLineRange (fldP, 0, 0, fldP->textLen, true, false);
		goto DrawSelection;
		}

	// Field that dynamicly expand and contract have special behavior when the 
	// top of the the text is scrolled out of view.
	if (fldP->attr.dynamicSize)
		{
		if (fldP->lines->start)
			{
			FormatAndDrawToFill (fldP, draw);
			goto DrawSelection;
			}
		}

	numLines = GetVisibleLines (fldP);

	ErrNonFatalDisplayIf (! fldP->lines, "No word wrapping info");
	
	line = fldP->lines;
	start = line->start;
	
	for (i = 0; i < numLines; i++)
		{
		length = FormatLine (fldP, start);
		line->start  = start;
		line->length = length;
		if (draw)
			DrawOneLineRange (fldP, i, start, start+length, true, false);
		start += length;
		line++;
		}

DrawSelection:
	// draw selection, if any
	// еее DOLATER  could be more careful about doing this while drawing
	// above, to reduce flicker
	if (draw && IsSelection(fldP))
		DrawMultiLineRange(fldP, fldP->selFirstPos, fldP->selLastPos, true);
}



/***********************************************************************
 *
 * FUNCTION:    FormatAndDrawFromPos
 *
 * DESCRIPTION: Staring with the line that contains the character 
 *              position specified, perform the word wrapping 
 *              calculation for each line visible line and redraw 
 *              the lines.
 *
 * PARAMETERS:	 fldP  pointer to a FieldType structure.
 *              pos  character position
 *              draw true if lines should be drawn
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/7/94	Initial Revision
 *			art	7/2/96	Add check for allocated lines structure
 *
 ***********************************************************************/
static void FormatAndDrawFromPos (FieldType * fldP, UInt16 pos, Boolean draw)
{
	// Single line field don't have display formatting information.
	if ((! fldP->attr.singleLine) && fldP->lines)
		{
		fldP->lines->start = StartOfLine (fldP, pos);
		FormatAndDrawLines (fldP, draw);
		}

	else if (draw)
		DrawAllLines (fldP);	
}


/***********************************************************************
 *
 * FUNCTION:    FormatAndDrawLineIfChanged
 *
 * DESCRIPTION: Starting with the display line that the insertion point 
 *              is on, perform the word wrapping calculation for each line 
 *              and redraw the line if it has changeed.
 *
 *              This routine is called when a character is inserted or 
 *              deleted (backspaceChrd out).  It assumes there is no
 *					 selection that needs to be drawn.
 *              
 * PARAMETERS:	 fldP         pointer to a FieldType structure.
 *              lineNumber staring line  number;
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/21/94	Initial Revision
 *
 ***********************************************************************/
static void FormatAndDrawLineIfChanged (FieldType * fldP, UInt16 lineNumber)
{
	LineInfoType *  line;
	UInt16         newLineStart;
	UInt16         newLineLength;
	UInt16         numLines;

	// Single line field don't have display formatting information.
	if (fldP->attr.singleLine)
		{
		DrawOneLineRange (fldP, 0, 0, fldP->textLen, true, false);
		return;
		}

	// Field that dynamicly expand and contract have special behavior when the 
	// top of the the text is scrolled out of view.
	if (fldP->attr.dynamicSize)
		{
		if (fldP->lines->start)
			{
			FormatAndDrawToFill (fldP, true);
			return;
			}
		}

	numLines = GetVisibleLines (fldP);
	line = &fldP->lines[lineNumber];
	newLineStart = line->start;
	do
		{
		// Determine how many character will fit on the the line.
		newLineLength = FormatLine (fldP, newLineStart);

		// Draw lines which changed. The current line always changes.
		if ( (fldP->insPtYPos == lineNumber) ||
			  (line->start	!= newLineStart)	||
			  (line->length != newLineLength) )
			{
				line->start  = newLineStart;
				line->length = newLineLength;
				DrawOneLineRange (fldP, lineNumber, newLineStart, newLineStart+newLineLength, true, false);
			}
		else if (lineNumber >= fldP->insPtYPos)
			// If the line's format has not change then we can assume that
			// the rest of the line have not been changed, so we're done.
			break;

		newLineStart = newLineStart + newLineLength;
		line++;
		lineNumber++;
		}
	while	(lineNumber < numLines);
}


/***********************************************************************
 *
 * FUNCTION:    FormatAndDrawIfWrappingChanged
 *
 * DESCRIPTION: Starting with the display line specified,  
 *              perform the word wrapping calculation for each line 
 *              and redraw the line if it has changeed.
 *
 *              This routine is called when a character is inserted or 
 *              deleted (backspaceChrd out).  It assumes there is no
 *					 selection that needs to be drawn.
 *              
 * PARAMETERS:	 fldP         pointer to a FieldType structure.
 *              lineNumber staring line  number;
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	110/2/95	Initial Revision
 *
 ***********************************************************************/
static void FormatAndDrawIfWrappingChanged (FieldType * fldP, UInt16 lineNumber)
{
	LineInfoType *	line;
	UInt16			start, length;

	if (fldP->attr.singleLine)
		{
		FormatAndDrawLineIfChanged (fldP, lineNumber);
		return;
		}

	// If the prior line does not end with a linefeedChr, check if the
	// current line should be concatenated to the prior line.
	line = &fldP->lines[lineNumber];
	if (line->start && fldP->text[line->start-1] != linefeedChr)
		{
		start = StartOfPreviousLine (fldP, line->start);
		length = FormatLine (fldP, start);
		if (start+length > line->start)
			{
			line->start = start;
			FormatAndDrawLines (fldP, fldP->attr.visible);
			}

		else
	 		FormatAndDrawLineIfChanged (fldP, lineNumber);
		}
	else
		FormatAndDrawLineIfChanged (fldP, lineNumber);
}


/***********************************************************************
 *
 * FUNCTION:    UpdateStartOfLines
 *
 * DESCRIPTION: This routine is called when characters are added to
 *              a field.  This routine will adjust the structure that
 *              keeps track of the starting postion of each visible line.
 *              Only the line that start after the postion passed are
 *              modified.
 *
 * PARAMETERS:	 fldP         pointer to a FieldType structure.
 *              amount      amount a add to starting postion of each line.
 *              pos         position after which adjusts should be made.
 *
 * RETURNED:	nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/21/94	Initial Revision
 *
 ***********************************************************************/
static void UpdateStartOfLines (FieldType * fldP, Int16 amount, UInt16 lineNum)
{
	LineInfoType * line;
	UInt16        numLines;

	if (fldP->attr.singleLine) return;
	
	line = fldP->lines;
	numLines = GetVisibleLines (fldP);
	while (lineNum < numLines)
		{
		line[lineNum].start +=  amount;
		lineNum++;
		}
}


/***********************************************************************
 *
 * FUNCTION:    AutoShift
 *
 * DESCRIPTION: This routine sets Graffiti to upper-case shift in 
 *              appropriate.
 *
 * PARAMETERS:  fldP  - pointer to a FieldType structure.
 *
 * RETURNED:    nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/27/96	Initial Revision
 *
 ***********************************************************************/
static void AutoShift (FieldType * fldP)
{
	register Char chr;
	UInt16 pos;

	if (! fldP->attr.autoShift)
		return;
	
	if (! fldP->textLen)
		{
		GrfSetState (false, false, true);
		return;
		}
		
	pos = GetStringPosOfInsPtPos (fldP);

	// If we have a period followed by a series of spaces then case shift
	// Griffiti.
	if (pos && (fldP->text[pos-1] == spaceChr))
		{
		pos--;
		while (pos && (fldP->text[pos] == spaceChr))
			pos--;

		chr = fldP->text[pos];
		if (pos && (chr == periodChr || chr == '!' || chr == '?'))
			{
			GrfSetState (false, false, true);
			return;
			}
		}

	// If we have a period followed by a series of linefeeds then case shift
	// Griffiti.
	else if (pos && (fldP->text[pos-1] == linefeedChr))
		{
		pos--;
		while (pos && (fldP->text[pos] == linefeedChr))
			pos--;

		chr = fldP->text[pos];
		if (pos && (chr == periodChr || chr == '!' || chr == '?'))
			{
			GrfSetState (false, false, true);
			return;
			}
		}
}


/***********************************************************************
 *
 * FUNCTION:    InsertChar
 *
 * DESCRIPTION: Insert a character into the text of a field and redraw the 
 *              field.  This routine performs any word wrapping or 
 *              scrolling that is necessary.
 *
 * PARAMETERS:	 fldP         pointer to a FieldType structure.
 *              ch          character to insert.
 *
 * RETURNED:	 nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/19/94	Initial Revision
 *       ron   7/21/95  Now use ModifyText routine in all places that modify the
 *                       text in order to support write-protected data storage.
 *			kwk	07/23/98	Fixed up calc of character size, and made
 *								calc conditional on INTERNATIONAL_MGR_OFF.
 *
 ***********************************************************************/
static void InsertChar (FieldType * fldP, WChar ch, Boolean inlineActive)
{
	UInt16 pos;
	UInt16 start;
	UInt16 end;
	UInt16 offset;
	UInt16 charSize;
	UInt16 numLines;
	UInt16 lineNumber;
	UInt16 lineWidth;
	UInt16 blockSize;
	Coord drawX, drawY;
	Char * chars;
	Boolean atEnd;
	Boolean notify = false;
	LineInfoType * line;
	UnderlineModeType underlineMode;
	Char charP[maxCharBytes];
	
	fldP->attr.dirty = true;

	charSize = TxtSetNextChar(charP, 0, ch);

	// If there is a selection, replace it with the character passed.
	//
	if (IsSelection (fldP))
		{
		pos = fldP->selFirstPos;
		if ( ! UndoRecordTyping (fldP, pos))
			return;

		FldInsert (fldP, charP, charSize);
		}

	else if (fldP->textLen + charSize <= fldP->maxChars)
		{
		UndoReset ();
		
		// If necessary, expand the memory block that contains the field's 
		// text string.
		if (fldP->textLen + charSize >= fldP->textBlockSize)
			if (! ExpandTextString (fldP, 0))
				return;

		pos = GetStringPosOfInsPtPos (fldP);

		// Scroll the insertion point into view.
		if (! fldP->attr.insPtVisible)
			{
			SetInsPtFromStringPos (fldP, pos);
			notify = true;
			}

		// Make room for the character in the field's text string.
		chars = fldP->text + pos;		
		blockSize = MemHandleSize (fldP->textHandle);
		if (blockSize == fldP->textBlockSize)
			{
			ModifyText(fldP, pos + charSize, chars, fldP->textLen - pos + 1);
			}
		else
			{
			offset = (UInt8 *) fldP->text - (UInt8 *) MemDeref(fldP->textHandle);
			ModifyText(fldP, pos + charSize, chars, blockSize - pos - offset - charSize);
			}

		// Insert the character into the field's text string.
		ModifyText (fldP, pos, charP, charSize);
		fldP->textLen += charSize;

		UpdateStartOfLines (fldP, charSize, fldP->insPtYPos + 1);
		pos += charSize;


		// If a character is being added to the end of a line, and the field is 
		// left justified, and the character is not a linefeedChr or a tab character,
		// then we can just draw the new character.
		atEnd = ((ch != linefeedChr) && (ch != tabChr) && AtEndOfLine(fldP, pos));

		// Compute the length the the current line plus the new character.
		if (atEnd)
			{
			if (! fldP->attr.singleLine)
				fldP->lines[fldP->insPtYPos].length += charSize;
			lineWidth = GetLineCharsWidth (fldP, fldP->insPtYPos); 
			}

		// If the new character fits on the end of the current line just draw the 
		// new character.
		if (atEnd && lineWidth <= fldP->rect.extent.x)
			{
			drawY = fldP->rect.topLeft.y + (fldP->insPtYPos * FntLineHeight ());
			drawX = fldP->rect.topLeft.x + lineWidth - FntCharsWidth (charP, charSize);

			WinPushDrawState();
			WinSetTextColor(UIColorGetIndex(UIFieldText));
			WinSetForeColor(UIColorGetIndex(UIFieldTextLines));
			WinSetBackColor(UIColorGetIndex(UIFieldBackground));

			if (inlineActive)
				underlineMode = solidUnderline;
			else
				underlineMode = (UnderlineModeType)fldP->attr.underlined;
			underlineMode = WinSetUnderlineMode (underlineMode);

			WinDrawChar(ch, drawX, drawY);
			WinPopDrawState();

			fldP->insPtXPos += charSize;
			}

		else
			{
			// If the field has a scroll bar get the positions of the first and last
			// visible characters.  We will use these values later to determine if the
			// scroll bar needs to be updated.
			// 981211 kwk make sure fldP->lines is not NULL
			if ((fldP->attr.hasScrollBar) && (fldP->lines != NULL))
				{
				numLines = GetVisibleLines(fldP);
				line = &fldP->lines [numLines - 1];
				start = fldP->lines->start;
				end = line->start + line->length;
				if (fldP->insPtYPos+1 == numLines) end++;
				}

			// Reformat the field starting with the previous line, inserting a 
			// white-space character may cause part of the current line to move 
			// onto the previous line.
			lineNumber = fldP->insPtYPos;
			if ((! atEnd) && lineNumber > 0) lineNumber--;

			FormatAndDrawIfWrappingChanged (fldP, lineNumber);
			if (! ExpandFieldHeight (fldP, pos))
				{
				SetInsPtFromStringPos (fldP, pos);

				// Entering a space or a tab may cause the height of a field
				// to decrease.
				if (ch == spaceChr || ch == tabChr)
					CompactFieldHeight (fldP, pos);
				}
			else
				FldSetInsertionPoint (fldP, pos);

			// If the field has a scroll bar and the positions of the first or last
			// visible characters is different then send an event to notify the 
			// application the the scroll bar needs to be updated.
			// 981211 kwk make sure fldP->lines is not NULL
			if ((fldP->attr.hasScrollBar) && (fldP->lines != NULL))
				{
				if (start != fldP->lines->start || end != (line->start + line->length))
					notify = true;
				}				
			}
		
		if (notify)
			FldSendChangeNotification (fldP);

		}
	// The field is full, play the overflow sound.
	else
		{
		SndPlaySystemSound (sndError);
		return;
		}

	if (fldP->attr.autoShift && fldP->textLen) {
			Boolean capsLock, numLock, autoShifted;
			UInt16 tempShift;
			
			if ((GrfGetState(&capsLock, &numLock, &tempShift, &autoShifted) == 0) && (autoShifted))
				GrfSetState(capsLock, numLock, false);
		}
}


/***********************************************************************
 *
 * FUNCTION:    BackspaceChar
 *
 * DESCRIPTION: Delete the character before the insertion point and
 *              redraw the field.
 *
 * PARAMETERS:	 fldP         pointer to a FieldType structure.
 *
 * RETURNED:	 nothing.
 *
 * HISTORY:
 *		11/22/94	art	Created by Art Lamb 
 *		07/15/98	kwk	When deleting from the beginning of the
 *							line, always decrement the y pos by one,
 *							not based on the char size.
 *		11/09/98	roger	Skipped reference to fldP->lines when fldP
 *							is a single line field (no lines struct).
 *		07/28/00	kwk	Call ConstrainXPos to make sure we don't use an
 *							invalid x offset.
 *
 ***********************************************************************/
static void BackspaceChar (FieldType * fldP)
{
	LineInfoType *	line;
	LineInfoType *	lastLine;
	UInt16			insPtXPos;
	UInt16			pos;
	UInt16			firstVisible;
	UInt16			lastVisible;
	UInt16			numLines;
	UInt16			start, length;
	UInt16			charsToDelete;
	UInt16			bytesToDelete;


	if (! fldP->text)
		return;

	fldP->attr.dirty = true;

	// If the field has a scroll bar get the positions of the first and last
	// visible characters.  We will use these values later to determine if the
	// scroll bar needs to be updated.
	// 981211 kwk make sure fldP->lines is not NULL
	if ((fldP->attr.hasScrollBar) && (fldP->lines != NULL))
		{
		numLines = GetVisibleLines(fldP);
		lastLine = &fldP->lines [numLines - 1];
		firstVisible = fldP->lines->start;
		lastVisible = lastLine->start + lastLine->length;
		}


	// If there is a selection, delete the selected text.
	if (IsSelection (fldP))
		{
		if ( ! UndoRecordBackspaces (fldP, fldP->selFirstPos))
			return;
		charsToDelete = fldP->selLastPos - fldP->selFirstPos;
		pos = fldP->selFirstPos;
		CancelSelection (fldP);
		DeleteFromText (fldP, pos, charsToDelete);
		
		if (fldP->attr.singleLine || pos < fldP->lines->start)
			{
			FormatAndDrawFromPos (fldP, pos, true);
			SetInsPtFromStringPos (fldP, pos);
			}
		else
			{
			SetInsPtFromStringPos (fldP, pos);
			UpdateStartOfLines (fldP, -charsToDelete, fldP->insPtYPos+1);
			FormatAndDrawIfWrappingChanged (fldP, fldP->insPtYPos);
			SetInsPtFromStringPos (fldP, pos);			
			}

		if (! CompactFieldHeight (fldP, pos))
			ExpandFieldHeight (fldP, pos);

		// If the field has a scroll bar and the positions of the first or last
		// visible characters is different then send an event to notify the 
		// application the the scroll bar needs to be updated.
		// 981211 kwk make sure fldP->lines is not NULL
		if ((fldP->attr.hasScrollBar) && (fldP->lines != NULL))
			{
			if (firstVisible != fldP->lines->start || 
			    lastVisible - charsToDelete != lastLine->start + lastLine->length)
				FldSendChangeNotification (fldP);
			}				

		return;
		}

	// Make sure we don't have an xPos that's beyond the end of the line.
	ConstrainXPos(fldP);
	
	// Scroll the insertion point into view.
	if (! fldP->attr.insPtVisible)
		{
		pos = GetStringPosOfInsPtPos (fldP);
		SetInsPtFromStringPos (fldP, pos);
		}

	// For single line fields, delete the character before the insertion
	//	point, redraw the line and adjust the insertion point position.
	insPtXPos = fldP->insPtXPos;
	if (fldP->attr.singleLine)
		{
		if (insPtXPos > 0)
			{
			bytesToDelete = TxtPreviousCharSize (fldP->text, insPtXPos);
			
			UndoRecordBackspaces (fldP, insPtXPos - bytesToDelete);
			DeleteFromText (fldP, insPtXPos - bytesToDelete, bytesToDelete);
			DrawOneLineRange (fldP, 0, 0, fldP->textLen, true, false);
			fldP->insPtXPos -= bytesToDelete;
			}
		return;
		}

 
	// Delete a character from a multi-line field.  Fisrt check that the
	// insertion point is beyond the first character of the field.
	pos = GetStringPosOfInsPtPos(fldP);
	if (pos == 0) return;
	
	bytesToDelete = TxtPreviousCharSize (fldP->text, pos);

	pos -= bytesToDelete;
	UndoRecordBackspaces (fldP, pos);
	DeleteFromText (fldP, pos, bytesToDelete);

	// If the insertion point is in the first column of the first row,
	// scroll the field up one line.
	if ((fldP->insPtXPos == 0) && (fldP->insPtYPos == 0))
		{ 
		fldP->lines->start = StartOfPreviousLine (fldP, fldP->lines->start-1);
		FormatAndDrawLines (fldP, true);
		}
	else
 		{
		if (fldP->insPtXPos == 0)
			fldP->insPtYPos -= 1;
		
 		UpdateStartOfLines (fldP, -bytesToDelete, fldP->insPtYPos+1);

		// If the prior line does not end with a linefeedChr, check if the
		// current line should be concatenated to the prior line.
		//
		line = &fldP->lines[fldP->insPtYPos];
		if (line->start && fldP->text[line->start-1] != linefeedChr)
			{
			start = StartOfPreviousLine (fldP, line->start);
			length = FormatLine (fldP, start);
			if (start+length > line->start)
				{
				line->start = start;
				FormatAndDrawLines (fldP, true);
				}
			else
	    		FormatAndDrawLineIfChanged (fldP, fldP->insPtYPos);
			}
		else
    		FormatAndDrawLineIfChanged (fldP, fldP->insPtYPos);
 		}
	SetInsPtFromStringPos (fldP, pos);

	if (! CompactFieldHeight (fldP, pos))
		ExpandFieldHeight (fldP, pos);
		
	// If the field has a scroll bar and the positions of the first or last
	// visible characters is different then send an event to notify the 
	// application the the scroll bar needs to be updated.
	// 981211 kwk make sure fldP->lines is not NULL
	if ((fldP->attr.hasScrollBar) && (fldP->lines != NULL))
		{
		if (firstVisible != fldP->lines->start || 
		    lastVisible - 1 != (lastLine->start + lastLine->length))
			FldSendChangeNotification (fldP);
		}				
}


/***********************************************************************
 *
 * FUNCTION:    InsertTextAndWrap
 *
 * DESCRIPTION: This routine replaces the current selection with the 
 *              string passed.  If there is no current selection, 
 *              the string passed is inserted at the position
 *              of the insertion point.
 *
 * PARAMETERS:	fldP    pointer to a FieldType structure.
 *
 * RETURNED:	True if operation was performed.
 *
 * HISTORY:
 *		07/30/98	kwk	Moved from FldInsert.
 *		09/10/98	kwk	Put in safety check for no action.
 *		09/28/98	kwk	Make sure fldP has focus before calling InsPtEnable.
 *		10/27/98	kwk	Added insPtPos param, since during inline the ins. pt to
 *							use for fldHeightChanged isn't the same as the end of the
 *							text being inserted.
 *		12/07/98	kwk	Call FldSetInsertionPoint if the field height is
 *							changing, so that it gets set appropriately even
 *							if the caller never processes the fldHeightChangedEvent.
 *
 ***********************************************************************/
static Boolean InsertTextAndWrap(FieldType * fldP, const Char * insertChars,
	UInt16 insertLen, UInt16 insertPos, UInt16 charsToDelete, UInt16 insPtPos)
{
	FontID	curFont;
	UInt16	byteToMove;
	UInt16	offset;
	UInt16	newLen;


	// If there's nothing to do, bail out immediately.
	if ((insertLen == 0) && (charsToDelete == 0))
		{
		return (false);
		}
	

	// If necessary, expand the memory block that contains the field's 
	// text string.
	newLen = fldP->textLen + insertLen - charsToDelete ;
	if (newLen >= fldP->textBlockSize)
		{
		if (! ExpandTextString (fldP, newLen))
			{
			return (false);
			}
		}


	curFont = FntSetFont (fldP->fontID);

	fldP->attr.dirty = true;

	// If there's a selection, delete it.
	if (charsToDelete)
		{
		CancelSelection (fldP);
		DeleteFromText (fldP, insertPos, charsToDelete);
		}

					
	// Insert the text passed.
	offset = fldP->text - (Char *) MemDeref(fldP->textHandle);
	byteToMove = MemHandleSize (fldP->textHandle) - insertPos - offset - insertLen;
	ModifyText (fldP, insertPos + insertLen, fldP->text + insertPos, byteToMove);
	ModifyText (fldP, insertPos, insertChars, insertLen);
	fldP->textLen += insertLen;


	// Redraw the field
	InsPtEnable (false);
	
	if (fldP->attr.singleLine)
		FormatAndDrawLines (fldP, fldP->attr.visible);
	else
		{
		if ((insertPos < fldP->lines->start) || (! fldP->attr.insPtVisible))
			FormatAndDrawFromPos (fldP, insertPos, fldP->attr.visible);
		else
			FormatAndDrawFromPos (fldP, fldP->lines->start, fldP->attr.visible);
		}
	
	// DOLATER kwk - Note that generating a fldHeightChanged event with a single pos field assumes that
	// any time a field height is changing you've got an insertion point, which won't be the
	// case for when we've got more advanced text services...currently it works for Japanese
	// because either you're inserting text, or if we're in the converted state, and choosing
	// an option shrinks or expands the # of lines, then the insertion point should always be
	// at the end anyway, and we're always changing all of the text in the inline area from
	// where it starts changing (no sub-string optimization).
	if (! ExpandFieldHeight (fldP, insPtPos))
		{
		SetInsPtFromStringPos (fldP, insPtPos);
		SetBlinkingInsPtLocation (fldP);
		CompactFieldHeight (fldP, insPtPos);
		}
	else
		FldSetInsertionPoint (fldP, insPtPos);


	FldSendChangeNotification (fldP);

	InsPtEnable (fldP->attr.visible && fldP->attr.insPtVisible && fldP->attr.hasFocus);

	FntSetFont (curFont);
	
	return (true);
}


/***********************************************************************
 *
 * FUNCTION:    AllocateLines
 *
 * DESCRIPTION: Allocate the structure that contains the word wrapping
 *              information.  An array of LineInfoType structure is 
 *              allocated, there is an array element for each visible
 *              line.
 *
 * PARAMETERS:  fldP         pointer to a FieldType structure.
 *
 * RETURNED:    nothing.
 *
 * HISTORY:
 *		12/12/94	art	Created by Art Lamb.
 *		08/19/98	kwk	Make sure font is set up before calling GetVisibleLines.
 *
 ***********************************************************************/
static void AllocateLines(FieldType* fldP)
{
	if (fldP->attr.singleLine)
	{
		return;
	}

	if (fldP->lines == NULL)
		{
		MemHandle handle;
		FontID curFont = FntSetFont(fldP->fontID);
		Int16 numBytes = sizeof(LineInfoType) * GetVisibleLines(fldP);
		
		if (numBytes == 0)
		{
			ErrNonFatalDisplay("No visible lines to allocate");
			numBytes = sizeof(LineInfoType);
		}
		
		handle = MemHandleNew (numBytes);
		if (handle == NULL)
		{
			ErrNonFatalDisplay("Allocation error");
		}
		else
		{
			fldP->lines = MemHandleLock(handle);
			MemSet(fldP->lines, numBytes, 0);
		}
		
		FntSetFont(curFont);
		}
	
	// DOLATER kwk - we could verify that the size of the current
	// linestarts array is big enough for the # of visible lines.
	// We could also return the # of lines allocated, which would
	// save a few extra calls to GetVisibleLines in other places.
}


/***********************************************************************
 *
 * FUNCTION:    GetVisibleLines
 *
 * DESCRIPTION: This function returns the number of lines that can be 
 *              display within the visible bounds of the field.
 *
 * PARAMETERS:	 fldP  pointer to a FieldType structure.
 *
 * RETURNED:	 nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/23/94	Initial Revision
 *			roger	10/8/97	Added ec code
 *
 ***********************************************************************/
static UInt16 GetVisibleLines (const FieldType * fldP)
{
	ErrNonFatalDisplayIf(fldP->fontID != FntGetFont(), "Font set incorrectly");
	
	return (fldP->rect.extent.y / FntLineHeight());
}


/***********************************************************************
 *
 * FUNCTION:    GetMaxLines
 *
 * DESCRIPTION: This function returns the number of visible lines 
 *              that can by a the field.
 *
 * PARAMETERS:	 fldP  pointer to a FieldType structure.
 *
 * RETURNED:	 maximum visible.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/29/97	Initial Revision
 *			bob	11/22/00	use maxVisibleLines to fix 41184
 *
 ***********************************************************************/
static UInt16 GetMaxLines (const FieldType * fldP)
{
	UInt16 maxHeight;
	FontID currFont;
	
	if (fldP->maxVisibleLines != 0)
		return fldP->maxVisibleLines;
		
	if (fldP->fontID == stdFont)
		return (maxFieldLines);
		
	currFont = FntSetFont (stdFont);
	maxHeight = maxFieldLines * FntLineHeight ();
	FntSetFont (currFont);
	return (maxHeight / FntLineHeight ());
}


/***********************************************************************
 *
 * FUNCTION:    GetLineCharsWidth
 *
 * DESCRIPTION: Get the width, in pixels, of the the line specified.
 *
 * PARAMETERS:	 fldP         pointer to a FieldType structure.
 *              lineNum     line number, the first visible line is 0
 *
 * RETURNED:	 width of the character of the line, in pixels.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/19/94	Initial Revision
 *
 ***********************************************************************/
static UInt16 GetLineCharsWidth (const FieldType * fldP, UInt16 lineNum)
{
	Char * chars;
	UInt16 length;

	if (fldP->attr.singleLine)
		{
		chars = fldP->text;
		length = fldP->textLen;
		}
	else
		{
		chars = fldP->text + fldP->lines[lineNum].start;
		length = fldP->lines[lineNum].length;
		}
	if (length && chars[length-1] == linefeedChr) length--;
	return (FntLineWidth (chars, length));
}


/***********************************************************************
 *
 * FUNCTION:    GetVisibleCharsInLine
 *
 * DESCRIPTION: Get the width, in characters, of the the line specified.
 *              linefeedChr character are not treated as visible characters, 
 *              they're not counted.
 *
 * PARAMETERS:	 fldP         pointer to a FieldType structure.
 *              lineNum     line number, the first visible line is 0
 *
 * RETURNED:	 number of characters in the specified line.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/19/94	Initial Revision
 *
 ***********************************************************************/
static UInt16 GetVisibleCharsInLine (const FieldType * fldP, UInt16 lineNum)
{
	Char * chars;
	UInt16 length;
	
	
	ErrNonFatalDisplayIf( fldP->text == NULL, "No field text");
	
	if (fldP->attr.singleLine)
		{
		chars = fldP->text;
		length = fldP->textLen;
		}
	else
		{
		ErrNonFatalDisplayIf( fldP->lines == NULL, "No field lines");
		
		chars = fldP->text + fldP->lines[lineNum].start;
		length = fldP->lines[lineNum].length;
		}
	if (length && chars[length-1] == linefeedChr) length--;
	return (length);
}


/***********************************************************************
 *
 * FUNCTION:    FindReverseChar
 *
 * DESCRIPTION: This routine searches a string in reverse order for the
 *              the character specified.
 *
 * PARAMETERS:	fldP          a pointer to the string to search
 *             charToFind   the character to find
 *             len
 *             
 *
 * RETURNED:	nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/22/94	Initial Revision
 *			kwk	?/?/99	Rename to FindReverseChar
 *
 ***********************************************************************/
static Boolean FindReverseChar (Char * text, Char charToFind, UInt16 len, UInt16 * pos)
{
	Char * p;
	
	p = text + len - 1;
	
	while (len)
		{
		if (*p == charToFind)
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
 * FUNCTION:    StartOfPreviousLine
 *
 * DESCRIPTION: Given the starting position of a display line, calculate
 *              the starting position of the previous line.
 *
 * PARAMETERS:	fldP          pointer to a FieldType structure.
 *             startOfLine  starting position of a display line
 *
 * RETURNED:	nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/22/94	Initial Revision
 *
 ***********************************************************************/
static UInt16 StartOfPreviousLine (const FieldType * fldP, UInt16 startOfLine)
{
  UInt16 start, endOfPrevLine, retValue;

	startOfLine = min (startOfLine, fldP->textLen);
		
	if (startOfLine <= 0) return (0);

   endOfPrevLine = startOfLine - 1;
	if (endOfPrevLine && fldP->text[endOfPrevLine] == linefeedChr)
		endOfPrevLine--;

	// Search backward for a linefeedChr.  If we find one, our staring postion
	// is the character after the linefeedChr, otherwise out starting position
	// is the start of the field's text.
	if (FindReverseChar (fldP->text, linefeedChr, endOfPrevLine, &start))
      start++;					// skip over the linefeedChr
    else
		start = 0;				// start of text string

   // Format the text until we get to the position passed, and then return
	// the starting position of the prior line. 
	do
      {
      retValue = start;
      start += FormatLine (fldP, start);
      }
    while (start < startOfLine);

  return (retValue);
}


/***********************************************************************
 *
 * FUNCTION:    StartOfLine
 *
 * DESCRIPTION: Given the character position, calculate the starting 
 *              position of the line that the character is in.
 *
 * PARAMETERS:	fldP     pointer to a FieldType structure.
 *             pos     starting position of a display line
 *
 * RETURNED:	nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/7/94	Initial Revision
 *
 ***********************************************************************/
static UInt16 StartOfLine (const FieldType * fldP, UInt16 pos)
{
  UInt16 start, retValue;

	if (pos == 0) return (0);

	// Search backward for a linefeedChr.  If we find one, our staring postion
	// is the character after the linefeedChr, otherwise out starting position
	// is the start of the field's text.
	//
	if (FindReverseChar (fldP->text, linefeedChr, pos-1, &start))
      start++;					// skip over the linefeedChr
    else
		start = 0;				// start of text string

   // Format the text until we get to the position passed, and then return
	// the starting position of the prior line. 
	//
	do
      {
      retValue = start;
      start += FormatLine (fldP, start);
      }
    while (start <= pos && start < fldP->textLen);

  return (retValue);
}


/***********************************************************************
 *
 * FUNCTION:    IsSelection
 *
 * DESCRIPTION: This routine returns true if there is a current selection.
 *
 * PARAMETERS:	fldP         pointer to a FieldType structure.
 *
 * RETURNED:	true of a selection exists.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/21/94	Initial Revision
 *
 ***********************************************************************/
static Boolean IsSelection (const FieldType * fldP)
{
  return (fldP->selFirstPos != fldP->selLastPos);
}


/***********************************************************************
 *
 * FUNCTION:    DeleteFromText
 *
 * DESCRIPTION: Delete the range of character specified.
 *
 * PARAMETERS:	 fldP            pointer to a FieldType structure.
 *              startPos       position of first character to delete
 *              charsToDelete  number of characters to delete
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/21/94	Initial Revision
 *       ron   7/21/95  Now use ModifyText routine in all places that modify the
 *                       text in order to support write-protected data storage.
 *
 ***********************************************************************/
static void DeleteFromText (FieldType * fldP, UInt16 startPos, UInt16 charsToDelete)
{
	Char * chars;
	UInt16    endPos;
	UInt16    offset;
	UInt16    blockSize;

	chars = fldP->text;
	if (chars && fldP->textLen)
		{	
		charsToDelete = min (charsToDelete, fldP->textLen -  startPos);
		endPos = startPos + charsToDelete;

		blockSize = MemHandleSize (fldP->textHandle);
		if (blockSize == fldP->textBlockSize) {
			//<RM> MemMove (&chars[startPos], &chars[endPos], fldP->textLen+1 - endPos);
			ModifyText(fldP, startPos, &chars[endPos], fldP->textLen+1 - endPos);
			}
		else
			{
			offset = fldP->text - (Char *) MemDeref(fldP->textHandle);
			//<RM> MemMove (&chars[startPos], &chars[endPos], blockSize - offset - endPos);
			ModifyText(fldP, startPos, &chars[endPos], blockSize - offset - endPos);
			}

		fldP->textLen -= (endPos - startPos);
		}
}


/***********************************************************************
 *
 * FUNCTION:    ExpandFieldHeight
 *
 * DESCRIPTION: Given the current position of the insertion point,
 *              determine if the height of the field should be
 *              expanded, If it should, send a fldHeightChangedEvent.
 *
 * PARAMETERS:  fldP  pointer to a FieldType structure.
 *              pos  character position of the insertion point.
 *
 * RETURNED:    true if the height of the field should be expanded.
 *
 * HISTORY:
 *		12/13/94	art	Created by Art Lamb.
 *		09/12/00	kwk	Removed fatal alert check for zero-width line.
 *
 ***********************************************************************/
static Boolean ExpandFieldHeight (FieldType * fldP, UInt16 pos)
{
	UInt16 numLines;
	UInt16 start;
	UInt16 length;
	UInt16 maxLines;

	if (! fldP->attr.dynamicSize) return (false);

	// Check if the height of the field may be expanded and if the
	// field is less than the maximum height.
	//
	maxLines = GetMaxLines(fldP);
	numLines = GetVisibleLines (fldP);
	if (numLines < maxLines)
		{
		// Determine how many line we need to add to the field in order to
		//	display the position passed.
		length = fldP->lines[numLines-1].length;
		start = fldP->lines[numLines-1].start + length;

		// If the last character is not a linefeed and is visible, don't 
		// expand the field's height.
		if (start && (start == fldP->textLen) && 
			(fldP->text[start-1] != linefeedChr))
			return (false);
			

		while (fldP->textLen > start)
			{
			length = FormatLine (fldP, start);
			start += length;
			if ( ++numLines == maxLines) break;
			}

		//	If the character position passed contains a line feed, move to the
		// next line.
		if (fldP->textLen && fldP->text[fldP->textLen-1] == linefeedChr && length) 
			numLines = min (numLines+1, maxLines);

		if (numLines == GetVisibleLines(fldP))
			return (false);

		// Send an event containing the new bounds of the field and the 
		// character position of the insertion point.  The bounds are not
		// chanaged in this routine, FldSetBounds preforms that function.
		FldSendHeightChangeNotification (fldP, pos, numLines);

		return (true);
		}

	return (false);
}


/***********************************************************************
 *
 * FUNCTION:    CompactFieldHeight
 *
 * DESCRIPTION: Determine if the height of the field should be
 *              compressed, if it should, send a fldHeightChangedEvent.
 *              Dynamicly sizing field that have blank line need to be
 *              compressed.
 *
 * PARAMETERS:  fldP  pointer to a FieldType structure.
 *              pos  character position of the insertion point.
 *
 * RETURNED:    true if the height of the field should be compressed.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/16/95	Initial Revision
 *
 ***********************************************************************/
static Boolean CompactFieldHeight (FieldType * fldP, UInt16 pos)
{
	UInt16 height;
	UInt16 numLines;
	LineInfoType * line;

	if (! fldP->attr.dynamicSize) return (false);

	numLines = GetVisibleLines (fldP);
	if (numLines == 1) return (false);
	
	// Check for blank line at the end of the field.
	height = numLines;
	line = &fldP->lines [numLines-1];
	while ((line->length == 0) && (height > 1))
		{
		height--;
		line--;
		}

	if (height == numLines) return (false);

	// If the text ends with a linefeed add a line.
	if ( (fldP->textLen) && (fldP->text[fldP->textLen-1] == linefeedChr))
		{
		height++;
		if (height == numLines) return (false);
		}


	// Send an event containing the new bounds of the field and the 
	// character position of the insertion point.  The bounds are not
	// chanaged in this routine, FldSetBounds preforms that function.
	FldSendHeightChangeNotification (fldP, pos, height);

	return (true);
}


/***********************************************************************
 *
 * FUNCTION:    ScrollRect
 *
 * DESCRIPTION: Move the bits of the displayed line up or down a line.
 *
 * PARAMETERS:	 fldP        pointer to a FieldType structure.
 *              direction  up or down.
 *
 * RETURNED:	 nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/22/94	Initial Revision
 *
 ***********************************************************************/
static void ScrollRectangle (FieldType * fldP, WinDirectionType direction)
{
	RectangleType  rect;
	RectangleType vacated;
	UInt16 lineHeight;

	RctCopyRectangle (&fldP->rect, &rect);
	lineHeight = FntLineHeight();
	rect.extent.y = rect.extent.y / lineHeight * lineHeight;
	WinScrollRectangle (&rect, direction, lineHeight, &vacated);
	WinEraseRectangle (&vacated, 0);
}


/***********************************************************************
 *
 * FUNCTION:    ScrollDisplayDown
 *
 * DESCRIPTION: Scroll the visible lines down by one line and draw a
 *              new line at the top of the field.
 *
 * PARAMETERS:	 fldP  pointer to a FieldType structure.
 *
 * RETURNED:	 nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/22/94	Initial Revision
 *			kwk	12/05/98	Call InsPtValid after scrolling.
 *
 ***********************************************************************/
static void ScrollDisplayDown (FieldType * fldP)
{
	LineInfoType * line;
	
	if (fldP->lines->start > 0)
		{
		// Move the display line information down one line.
		//
		MemMove (&fldP->lines[1], fldP->lines, 
			sizeof(LineInfoType) * (GetVisibleLines (fldP)-1));
	
		// Format and draw the line which scrolled into view.
		//
		line = fldP->lines;
		line->start = StartOfPreviousLine (fldP, line->start);
		line->length = FormatLine (fldP, line->start);
		if (fldP->attr.visible)
			{
			ScrollRectangle (fldP, winDown);
			DrawOneLineRange (fldP, 0, line->start, line->start+line->length, true, false);
			}

		InsPtValid (fldP);
		}
}


/***********************************************************************
 *
 * FUNCTION:    ScrollDisplayUp
 *
 * DESCRIPTION: Scroll the visible lines up by one line and draw a
 *              new line at the bottom of the field.
 *
 * PARAMETERS:	 fldP  pointer to a FieldType structure.
 *
 * RETURNED:	 nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/22/94	Initial Revision
 *			kwk	12/05/98	Call InsPtValid after scrolling.
 *
 ***********************************************************************/
static void ScrollDisplayUp (FieldType * fldP)
{
	LineInfoType * line;
	UInt16 numLines;


	// Move the display line information up oneline.
	numLines = GetVisibleLines (fldP);
	MemMove (fldP->lines, &fldP->lines[1], sizeof(LineInfoType) * (numLines-1));

	// Draw the line which scrolled into view.
	line = &fldP->lines[numLines-1];
	line->start = line->start + line->length;
	line->length = FormatLine (fldP, line->start);
	if (fldP->attr.visible)
		{
		ScrollRectangle (fldP, winUp);
		DrawOneLineRange (fldP, numLines-1, line->start, line->start+line->length, true, false);
		}

	// 981210 kwk The real source of the problem...when scrolling up, we want
	// to move the insertion point to the preceeding line, otherwise it
	// winds up on the next line in some random location.
	// For now this fix is too risky, so just patch things up in
	// InsPtValid (which we don't need if this code works). Note that the
	// same change needs to be made to ScrollDisplayDown()
	
#if 0
	if (fldP->attr.insPtVisible)
		{
		if (fldP->insPtYPos == 0)
			{
			fldP->insPtXPos = insPtPos;
			fldP->insPtYPos = absolutePosInX;
			fldP->attr.insPtVisible = false;
			}
		else
			fldP->insPtYPos--;
		}
#endif

	// Make sure that the act of scrolling up didn't cause a problem with
	// the insertion point (which has an x/y value) now being in the middle
	// of a double-byte character.
	
	InsPtValid (fldP);
}


/***********************************************************************
 *
 * FUNCTION:    ScrollUpNLines
 *
 * DESCRIPTION: This routine scrolls a field up by the number of lines 
 *              specified.
 *
 * PARAMETERS:	fldP           pointer to a FieldType structure.
 *             linesToScroll number of lines to scroll.
 *
 * RETURNED:	nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/23/96	Initial Revision
 *			rsf	11/11/98	Do nothing when the field is a single line or empty.
 *
 ***********************************************************************/
static void ScrollUpNLines (FieldType * fldP, UInt16 linesToScroll)
{
	UInt16 start;
	UInt16 startOfLine;
	UInt16 endOfPrevLine;
	UInt16 lineStart;
	UInt16 lineCount = 0;

	if ((fldP->attr.singleLine) || (!fldP->text)) return;

	startOfLine = min (fldP->lines->start, fldP->textLen);
	if (startOfLine <= 0) return;

	endOfPrevLine = startOfLine - 1;
	if (endOfPrevLine && fldP->text[endOfPrevLine] == linefeedChr)
		endOfPrevLine--;

	// Find the first line that begins after after a linefeed and is at 
	// or before the line we wish to scroll to.
	do 
		{
		// Search backward for a linefeedChr.  If we find one, our staring
		// postion is the character after the linefeedChr, otherwise out 
		// starting position is the start of the field's text.
		if (FindReverseChar (fldP->text, linefeedChr, endOfPrevLine, &start))
	      start++;					// skip over the linefeedChr
	    else
			start = 0;				// start of text string

		// Count the number of lines we move back through.
		lineStart = start;
		while (lineStart < startOfLine)
			{
     		lineCount++;
      	lineStart += FormatLine (fldP, lineStart);
			}

		if (start == 0) break;

		startOfLine = start;
		if (startOfLine > 1)
	   	endOfPrevLine = startOfLine - 2;		// move before linefeed	
	   else
	   	endOfPrevLine = 0;
		}
	while (lineCount < linesToScroll);


	// If we've moved back too many lines, move foreward until we're at the 
	// correct line.
	while (lineCount > linesToScroll)
		{
      start += FormatLine (fldP, start);		
		lineCount--;
		}

	fldP->lines->start = start;

	FormatAndDrawLines (fldP, true);
}


/***********************************************************************
 *
 * FUNCTION:    ScrollRangeIntoView
 *
 * DESCRIPTION: Scroll the field such that the text from <start> to <end>
 *				is visible, if possible.
 *
 * PARAMETERS:	fldP - pointer to a FieldType structure.
 *					start - starting byte offset of range.
 *					end - ending byte offset of range.
 *
 * RETURNED:	 nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			kwk	10/27/98	Initial revision.
 *
 ***********************************************************************/
static void ScrollRangeIntoView (FieldType * fldP, UInt16 start, UInt16 end)
{
	Boolean scrolledDown;
	LineInfoType * line;
	UInt16 numLines;

	if ((fldP->attr.singleLine) || (fldP->lines == NULL) || (fldP->textLen == 0))
		return;
	
	scrolledDown = false;
	end = min (end, fldP->textLen);
	start = min (start, end);
	
	// If the starting character position passed is off the top of the 
	// display, scroll the field down until the character is visible.
	line = fldP->lines;
	while (start < line->start) {
		scrolledDown = true;
		ScrollDisplayDown (fldP);
	}

	// If the character at the position passed is off the bottom of the 
	// display, and we didn't already scroll down to get it into view
	// (range spans visible lines), scroll up.
	
	// DOLATER kwk - we could improve this
	// by only scrolling up such that the beginning pos doesn't go off
	// the top, as currently we might do just that.
	if (!scrolledDown) {
		numLines = GetVisibleLines (fldP);
		line = &fldP->lines[numLines - 1];
		while (end > line->start + line->length)
			ScrollDisplayUp (fldP);
	}
}


/***********************************************************************
 *
 * FUNCTION:    SetInsPtFromStringPos
 *
 * DESCRIPTION:  Given a character position in the field's text, position
 *               the insertion point.  This routine updates the 
 *               insertion point postion stored in the field's data
 *               structure, it does not position the blinking insertion
 *               point.
 *
 * PARAMETERS:	 fldP  pointer to a FieldType structure.
 *              pos  character position in field's text
 *
 * RETURNED:	 nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/21/94	Initial Revision
 *
 ***********************************************************************/
static void SetInsPtFromStringPos (FieldType * fldP, UInt16 pos)
{
	LineInfoType * line;
	UInt16        numLines, i;

	fldP->attr.insPtVisible = true;

	pos = min (pos, fldP->textLen);

	if (fldP->attr.singleLine)
		{
		fldP->insPtYPos = 0;
		fldP->insPtXPos = pos;			
		return;
		}
	
	if ((! fldP->lines) || (! fldP->textLen))
		{
		fldP->insPtXPos = 0;
		fldP->insPtYPos = 0;			
		return;
		}

	ScrollRangeIntoView(fldP, pos, pos);

	//	Determine which visible line the position passed is in, and set the
	// y cooridinate of the insertion point to that line.
	//
	line = fldP->lines;
	numLines = GetVisibleLines (fldP);
	for (i = 0; i < numLines; i++)
		{
		if (pos <= line->start + line->length)
			break;
		line++;
		}
	fldP->insPtYPos = i;

	// Set the x position of the insertion point. If the character position 
	// passed contains a line feed, move to the start of the next line.
	//
	fldP->insPtXPos = pos - line->start;
	if (pos && fldP->text[pos-1] == linefeedChr)
		{
		if (fldP->insPtYPos+1 == numLines)
			{
			ScrollDisplayUp (fldP);
			fldP->insPtXPos = 0;
			}
		else if ((line->length) && (fldP->insPtXPos))
			{
			fldP->insPtYPos++;
			fldP->insPtXPos = 0;
			}
		}
}


/***********************************************************************
 *
 * FUNCTION:    GetStringPosOfInsPtPos
 *
 * DESCRIPTION: Return the string position that matches the insertion 
 *              point position.
 *
 * PARAMETERS:	fldP  pointer to a FieldType structure.
 *
 * RETURNED:	character position in field's text of insertion point.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/21/94		Initial Revision
 *
 ***********************************************************************/
static UInt16 GetStringPosOfInsPtPos (const FieldType * fldP)
{
	LineInfoType * line;
	FontID      curFont;
	UInt16	 		numLines;

	if (! fldP->attr.insPtVisible)
		{
		// 981210 kwk Report error if we're not using absolute coordinates.
		ErrNonFatalDisplayIf(fldP->insPtYPos != absolutePosInX,
			"Ins pt not visible but x/y still relative");
		return (fldP->insPtXPos);
		}
	
	curFont = FntSetFont (fldP->fontID);
	numLines = GetVisibleLines(fldP);
	FntSetFont (curFont);
	
	ErrNonFatalDisplayIf( (fldP->insPtYPos > numLines - 1), 
		"Invalid insertion point position");
	
	if (fldP->insPtYPos > numLines - 1)
		return (fldP->textLen);

	if (fldP->attr.singleLine)
		return (fldP->insPtXPos);

	if (! fldP->lines)
		return 0;

	line = &fldP->lines[fldP->insPtYPos];
	if (fldP->insPtXPos < line->length)	
		return (line->start + fldP->insPtXPos);
		
	if (line->length && (fldP->text[line->start + line->length -1] == linefeedChr))
		return (line->start + line->length - 1);
		
	return (line->start + line->length);
}


/***********************************************************************
 *
 * FUNCTION:    PositionVisible
 *
 * DESCRIPTION: This routine returns true if the character position passed
 *              is within the visible text.
 *
 * PARAMETERS:	 fldP    pointer to a FieldType structure.
 *              pos  character position in field's text
 *              
 * RETURNED:	 true if position is visible.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/12/95	Initial Revision
 *
 ***********************************************************************/
static Boolean PositionVisible (FieldType * fldP, UInt16 pos)
{
	UInt16 numLines;
	LineInfoType * line;
	
	if (fldP->attr.singleLine)
		return (true);

	if (! fldP->text)
		return (true);

	numLines = GetVisibleLines (fldP);
	line = &fldP->lines[numLines-1];

//	if (line->length == 0)
//		return (true);

	if (pos < fldP->lines->start)
		return (false);

	else if (pos == fldP->lines->start)
		return (true);

	else if (pos > line->start + line->length)
		return (false);

	else if (pos == line->start + line->length)
		{
		if (fldP->text[pos-1] == linefeedChr)
			return (line->length == 0);
		else
			return (true);
		}

	else 
		return (true);
}


/***********************************************************************
 *
 * FUNCTION:    SetBlinkingInsPtLocation
 *
 * DESCRIPTION: Set the position of the blinking insertion point.
 *
 * PARAMETERS:	 fldP         pointer to a FieldType structure.
 *
 * RETURNED:	 nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/19/94	Initial Revision
 *			roger	10/13/99	Do only if visible.
 *
 ***********************************************************************/
static void SetBlinkingInsPtLocation (FieldType * fldP)
{
	Coord	x, y;
	UInt16 insPtXPos;
	Char * chars;
	
	if (fldP->attr.visible)
		{
		chars = fldP->text;
		x = fldP->rect.topLeft.x;

		if (fldP->attr.singleLine)
			{
			insPtXPos = fldP->insPtXPos;
			if (fldP->attr.justification == rightAlign)
				x += fldP->rect.extent.x - FntCharsWidth (chars, fldP->textLen);
			}
		else if (fldP->textLen)
			{
			insPtXPos = min (fldP->insPtXPos, 
				GetVisibleCharsInLine(fldP, fldP->insPtYPos));
			chars += fldP->lines[fldP->insPtYPos].start;
			}
		else
			insPtXPos = 0;
			
		x += min (FntLineWidth (chars, insPtXPos) - 1, fldP->rect.extent.x);
		x = max (x, 0);

		y = fldP->rect.topLeft.y + (fldP->insPtYPos * FntLineHeight());
		WinWindowToDisplayPt (&x, &y);
		InsPtSetLocation (x, y);
		}
}


/***********************************************************************
 *
 * FUNCTION:    GetPosOfPoint
 *
 * DESCRIPTION: Given a window relitive coordinate, calculate the 
 *              position of the character that the point is on.
 *
 * PARAMETERS:	 fldP  pointer to a FieldType structure.
 *               x,y  window relitive coordinate
 *				 leftSideP true if point is on the left side of the character.
 *
 * RETURNED:	 position of the character the point is on
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art		09/17/97	Initial Revision
 *			art		09/21/98	Added parameter 
 *
 ***********************************************************************/
static UInt16 GetPosOfPoint (FieldType * fldP, Coord x, Coord y, Boolean * leftSideP)
{
	UInt16 pos;
	UInt16 yPos;
	UInt16 width;
	UInt16 lastWidth;
	UInt16 charSize;
	UInt16 charsPerLine;
	Char * chars;


	*leftSideP = false;

	if (! fldP->textLen)
		return (0);


	// Compute the x position of the insertion point.  First, adjust the 
	// x coordinate such that it is within the bounds of field.
	x = max (0, x - fldP->rect.topLeft.x);
	if (fldP->attr.singleLine)
		{
		pos = 0;
		charsPerLine = fldP->textLen;

		if (fldP->attr.justification == rightAlign)
			{
			if (x < fldP->rect.extent.x - FntCharsWidth (fldP->text, charsPerLine))
				{
				*leftSideP = true;
				return (0);
				}
			}
		}

	else 
		{
		// Compute the line that the y-coordinate is on.
		yPos = max (((y - fldP->rect.topLeft.y) / FntLineHeight ()), 0);
		yPos = min (yPos, GetVisibleLines (fldP)-1);

		pos = fldP->lines[yPos].start;
		charsPerLine = fldP->lines[yPos].length;

		if (charsPerLine && (fldP->text[pos + charsPerLine - 1] == linefeedChr))
			charsPerLine--;
		}

	// Determine where in the line (the character position) the x coordinate
	// passed is. 
	if (charsPerLine)
		{
		width = 0;
		lastWidth = 0;
		while (charsPerLine) 
			{
			chars = &fldP->text[pos];
			if (*chars != tabChr)
				{
				charSize = TxtNextCharSize (chars, 0);
				width += FntCharsWidth (chars, charSize);
				}
			else
				{
				width += fntTabChrWidth - (width % fntTabChrWidth);
				charSize = 1;
				}
			
			if (width >= x)
				{
				if ((x - lastWidth) >= ((width - lastWidth) >> 1))
					{
					pos += charSize;
					*leftSideP = true;
					}
				break;
				}

			pos += charSize;
			charsPerLine -= charSize;
			lastWidth = width;
			}
		if (width < x)
			*leftSideP = true;
		}

	return (pos);
}


/***********************************************************************
 *
 * FUNCTION:	ConstrainXPos
 *
 * DESCRIPTION: If we've got an insertion point, and it's visible, then
 *	make sure the value of the x offset isn't past the end of the line.
 *	This happens after an up/down arrow from a longer line to a shorter
 *	line. Currently the field code allows the insPtXPos to be > length
 *	of a line so that doing repeated up/down arrows works properly
 *	(you don't get offset "drift"). However, using byte-based offsets
 *	can cause drift due to varying character widths and (for Japanese)
 *	character byte sizes.
 *
 * PARAMETERS:
 *	fldP		  ->	pointer to a FieldType structure.
 *
 * RETURNED:
 *	True => x position was constrained.
 *
 * HISTORY:
 *	07/28/00	kwk	Created by Ken Krugler
 *
 ***********************************************************************/
static Boolean ConstrainXPos(FieldType* fldP)
 {
 	// If we've got an insertion point, and it's visible, and we've got a
 	// multi-line display, then check for constraining the x offset.
	if ((fldP->selFirstPos == fldP->selLastPos)
	 && (fldP->attr.insPtVisible)
	 && (!fldP->attr.singleLine)
	 && (fldP->lines != NULL))
	{
		LineInfoType* lineP;
		
#if (ERROR_CHECK_LEVEL == ERROR_CHECK_FULL)
		FontID curFont;
		UInt16 numLines;
		
		curFont = FntSetFont(fldP->fontID);
		numLines = GetVisibleLines(fldP);
		FntSetFont(curFont);
	
		ErrNonFatalDisplayIf(fldP->insPtYPos == absolutePosInX,
			"Ins pt visible but x/y is absolute");
		ErrNonFatalDisplayIf((fldP->insPtYPos > numLines - 1), 
									"Invalid insertion point position");
#endif

		lineP = &fldP->lines[fldP->insPtYPos];
		if (fldP->insPtXPos > lineP->length)
		{
			fldP->insPtXPos = lineP->length;
			return(true);
		}
	}
	
	return(false);
} // ConstrainXPos


/***********************************************************************
 *
 * FUNCTION:    ResetInsPt
 *
 * DESCRIPTION: Reset the insertion point to be at the beginning of
 *		the text.
 *
 * PARAMETERS:
 *		fldP	 ->	pointer to a FieldType structure.
 *
 * RETURNED:	nothing
 *
 * HISTORY:
 *		11/07/00	kwk	Created by Ken Krugler.
 *
 ***********************************************************************/
static void ResetInsPt(FieldType *fldP)
{
	// Note that it might make more sense to set the insPtVisible flag
	// to false, and have insPtYPos = absolutePosInX, but that's a change
	// from how the existing reset code works, so leave it alone for now.
	fldP->attr.insPtVisible = true;
	fldP->insPtXPos = 0;
	fldP->insPtYPos = 0;
	fldP->selFirstPos = 0;
	fldP->selLastPos = 0;
} // ResetInsPt


/***********************************************************************
 *
 * FUNCTION:    SetInsPtPosFromPt
 *
 * DESCRIPTION: Given a window relitive coordinate, calculate the 
 *              position of the insertion point.  This routine modifies
 *              the field's data structure that keeps track of the 
 *              location of the insertion point.
 *
 * PARAMETERS:	 fldP  pointer to a FieldType structure.
 *              x,y  window relitive coordinate
 *
 * RETURNED:	 nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/23/94		Initial Revision
 *
 ***********************************************************************/
static void SetInsPtPosFromPt (FieldType * fldP, Coord x, Coord y)
{
	UInt16 xPos, yPos, charsPerLine, width, lastWidth, charSize;
	Char * chars;
	LineInfoType * line;

	fldP->attr.insPtVisible = true;


	// Compute the line that the y-coordinate is on.
	yPos = max (((y - fldP->rect.topLeft.y) / FntLineHeight ()), 0);
	if (fldP->attr.singleLine)
		yPos = 0;
	else
		yPos = min (yPos, GetVisibleLines (fldP)-1);
	fldP->insPtYPos = yPos;

	// If the coordinate is on a blank line, position the insertion point
	// after the last character of the field.
	if (fldP->attr.singleLine)
		charsPerLine = fldP->textLen;
	else if (fldP->lines)
		charsPerLine = fldP->lines[yPos].length;
	else
		charsPerLine = 0;

	if ((charsPerLine == 0))
		{
		if (fldP->textLen)
			SetInsPtFromStringPos (fldP, fldP->textLen);
		else
			{
			fldP->insPtXPos = 0;
			fldP->insPtYPos = 0;			
			}
		return;
		}

	// Compute the x position of the insertion point.  First, adjust the 
	// x coordinate such that it is within the bounds of field.
	x = max (0, x - fldP->rect.topLeft.x);
	if (fldP->attr.singleLine)
		{
		chars = fldP->text;
		if (fldP->attr.justification == rightAlign)
			{
			x -= fldP->rect.extent.x - FntCharsWidth (chars, charsPerLine);
			if (x < 0)
				{
				fldP->insPtXPos = 0;
				return;
				}
			}
		}
	else 
		{
		line = &fldP->lines[yPos];			
		chars = &fldP->text[line->start];			
		if (chars[line->length-1] == linefeedChr)
			charsPerLine--;
		}

	// Determine where in the line (the character position) the x coordinate
	// passed is. 
	xPos = 0;
	width = 0;
	lastWidth = 0;
	while (xPos < charsPerLine) 
		{
		if (chars[xPos] != tabChr)
			{
			charSize = TxtNextCharSize (chars, xPos);
			width += FntCharsWidth (&chars[xPos], charSize);
			}
		else
			{
			charSize = 1;
			width += fntTabChrWidth - (width % fntTabChrWidth);
			}
		
		if (width >= x)
			{
			if ((x - lastWidth) >= ((width - lastWidth) >> 1))
				xPos += charSize;
			break;
			}
		xPos += charSize;
		lastWidth = width;
		}

	fldP->insPtXPos = xPos;
}


/***********************************************************************
 *
 * FUNCTION:    SetInsPt
 *
 * DESCRIPTION: This routine will turn the blinking insertion point
 *              on if the position where the insertion point is
 *              located is visible.  
 *
 *              If the position of the insertion point is not visible
 *              the position is stored in the insPtXPos member of the
 *               field structure.
 *
 * PARAMETERS:	 fldP          - pointer to a FieldType structure.
 *              enableInsPt  - true if the insertion point is to be turned on
 *              pos          - character position of the insertion point
 *
 * RETURNED:	 nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	10/24/97	Initial Revision
 *
 ***********************************************************************/
static void SetInsPt (FieldType * fldP, Boolean enableInsPt, UInt16 pos)
{
	if (PositionVisible (fldP, pos))
		{
		SetInsPtFromStringPos (fldP, pos);
		
		if (fldP->attr.hasFocus)
			{
			if (enableInsPt)
				{
				SetBlinkingInsPtLocation (fldP);
			  	InsPtEnable (true);
			  	}
			}
		}

	else
		{
		fldP->attr.insPtVisible = false;
		fldP->insPtXPos = pos;
		fldP->insPtYPos = absolutePosInX;
		}
}


/***********************************************************************
 *
 * FUNCTION:    UpdateInsPt
 *
 * DESCRIPTION: This routine is called after a field has been
 *              scrolled or the font used to dispaly the field
 *              has been changed.
 *
 *              This routine will turn the blinking insertion point
 *              on if the position where the insertion point is
 *              located is visible.  
 *
 *              If the position of the insertion point was visible
 *              (indicated by the parameter insPtOn) and is not longer
 *              visible, its position is stored in the selFirstPos
 *              member of the field structure.
 *
 * PARAMETERS:	 fldP      - pointer to a FieldType structure.
 *              insPtOn  - true if the insertion point was on
 *              insPtPos - character position of the insertion point
 *
 * RETURNED:	 nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/23/94		Initial Revision
 *
 ***********************************************************************/
static void UpdateInsPt (FieldType * fldP, Boolean insPtOn, UInt16 insPtPos)
{
	Boolean enableInsPt;

	// If the insertion point was on and its position is still visible, 
	// turn it back on.  
	if (insPtOn)
		{
		SetInsPt (fldP, insPtOn, insPtPos);
		}

	else if (fldP->attr.insPtVisible)
		{
		enableInsPt = (! IsSelection (fldP));
		SetInsPt (fldP, enableInsPt, insPtPos);
		}

	// If the insertion point was not on and its position was become 
	// visible, turn it on.
	else 
		{
		enableInsPt = (! IsSelection (fldP));
		SetInsPt (fldP, enableInsPt, fldP->insPtXPos);
		}


/*
	// If the insertion point was on and its position is still visible, 
	// turn it back on.  
	if (insPtOn)
		{
		if (PositionVisible (fldP, insPtPos))
			{
			SetInsPtFromStringPos (fldP, insPtPos);
		  	InsPtEnable (true);
			SetBlinkingInsPtLocation (fldP);
			}
		else
			{
			fldP->selFirstPos = insPtPos;
			fldP->selLastPos = insPtPos;
			fldP->insPtXPos = 0;
			fldP->insPtYPos = 0;
			fldP->attr.insPtVisible = false;
			}
		}

	// If the insertion point was not on and its position was become 
	// visible, turn it on.
	else if (PositionVisible (fldP, fldP->selFirstPos) &&
		     (! IsSelection (fldP)))
		{
		SetInsPtFromStringPos (fldP, fldP->selFirstPos);
		if (fldP->attr.hasFocus)
			{
		  	InsPtEnable (true);
			SetBlinkingInsPtLocation (fldP);
			}
		}

	else if (fldP->attr.insPtVisible)
		{
		 if (! IsSelection (fldP))
		 	{
			fldP->selFirstPos = insPtPos;
			fldP->selLastPos = insPtPos;
			}
		fldP->insPtXPos = 0;
		fldP->insPtYPos = 0;
		fldP->attr.insPtVisible = false;
		}		
*/
}


/***********************************************************************
 *
 * FUNCTION:    MoveInsPtUpOneLine
 *
 * DESCRIPTION: Move the insertion point up one line, if the insertion
 *              point is on the first visible line, scroll the field down.
 *
 * PARAMETERS:	 fldP  pointer to a FieldType structure.
 *
 * RETURNED:	 nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/22/94	Initial Revision
 *			kwk	12/05/98	Make sure X pos is valid after changing line.
 *			grant  2/ 4/99	Send field change notification if we scroll.
 *
 ***********************************************************************/
static void MoveInsPtUpOneLine (FieldType * fldP)
{
	if (fldP->attr.singleLine) return;

	if (fldP->insPtYPos > 0)
		{
		fldP->insPtYPos--;
		InsPtValid (fldP);
		}
	else
		{
		ScrollDisplayDown (fldP);
		FldSendChangeNotification (fldP);
		}
}


/***********************************************************************
 *
 * FUNCTION:    MoveInsPtDownOneLine 
 *
 * DESCRIPTION: Move the insertion point down one line, if the insertion
 *              point is on the last visible line, scroll the field up.
 *
 * PARAMETERS:	fldP  pointer to a FieldType structure.
 *
 * RETURNED:	nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/22/94	Initial Revision
 *			kwk	12/05/98	Make sure X pos is valid after changing line.
 *			grant	 2/ 4/99	Send field change notification if we scroll.
 *
 ***********************************************************************/
static void MoveInsPtDownOneLine  (FieldType * fldP)
{
	UInt16 pos;
	LineInfoType * line;

	if (fldP->attr.singleLine) return;

	// If the current line is not the last line, or if the current line is 
	// the last line and ends with a linefeedChr, move the insertion point down
	// one line.
	//
	line = &fldP->lines[fldP->insPtYPos];
	pos = line->start + line->length;
	if ( (pos < fldP->textLen) || 
	     (line->length && fldP->text[pos-1] == linefeedChr) )
		{
		if (fldP->insPtYPos < GetVisibleLines(fldP)-1)
			{
			fldP->insPtYPos++;
			InsPtValid (fldP);
			}
		else
			{
			ScrollDisplayUp (fldP);
			FldSendChangeNotification (fldP);
			}
		}
}


/***********************************************************************
 *
 * FUNCTION:    InsPtUp
 *
 * DESCRIPTION: Move the insertion point down a line.
 *
 * PARAMETERS:	 fldP  pointer to a FieldType structure.
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/22/94	Initial Revision
 *			kwk	07/28/98	Return immediately if no text.
 *
 ***********************************************************************/
static void InsPtUp (FieldType * fldP)
{
	if (! fldP->text)
		return;

	if (! fldP->attr.insPtVisible)
		SetInsPtFromStringPos (fldP, GetStringPosOfInsPtPos (fldP));

	if (IsSelection (fldP))
		{
		CancelSelection (fldP);
		SetInsPtFromStringPos (fldP, fldP->selFirstPos);
		}
	else
	  MoveInsPtUpOneLine (fldP);
}


/***********************************************************************
 *
 * FUNCTION:    InsPtDown
 *
 * DESCRIPTION: Move the insertion point up a line.
 *
 * PARAMETERS:	 fldP  pointer to a FieldType structure.
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/22/94	Initial Revision
 *			kwk	07/28/98	Return immediately if no text.
 *
 ***********************************************************************/
static void InsPtDown (FieldType * fldP)
{
	UInt16 pos;
	
	if (! fldP->text)
		return;

	if (! fldP->attr.insPtVisible)
		SetInsPtFromStringPos (fldP, GetStringPosOfInsPtPos (fldP));

	if (IsSelection (fldP))
		{
		pos = fldP->selLastPos;
		CancelSelection (fldP);
		SetInsPtFromStringPos (fldP, pos);
		}
	else
	  MoveInsPtDownOneLine (fldP);
}


/***********************************************************************
 *
 * FUNCTION:    InsPtLeft
 *
 * DESCRIPTION: Move the insertion point one character left, if the 
 *              insertion point is at the left bound of the field, wrap
 *              to the end of the previous line.
 *
 * PARAMETERS:	 fldP  pointer to a FieldType structure
 *
 * RETURNED:	 nothing
 *
 * HISTORY:
 *		11/22/94	art	Created by Art Lamb.
 *		07/28/98	kwk	Return immediately if no text.
 *		07/19/00	kwk	Use SetInsPtFromStringPos to set location, versus just
 *							adjusting insPtXPos, as this might be past the end of
 *							the line if the user previously did a down/up arrow.
 *
 ***********************************************************************/
static void InsPtLeft (FieldType * fldP)
{
	if (!fldP->text) return;
	
	if (! fldP->attr.insPtVisible)
		SetInsPtFromStringPos (fldP, GetStringPosOfInsPtPos (fldP));

	if (IsSelection (fldP))
		{
		CancelSelection (fldP);
		SetInsPtFromStringPos (fldP, fldP->selFirstPos);
		}

	else if (fldP->insPtXPos > 0)
		{
		// fldP->insPtXPos -= TxtPreviousCharSize (fldP->text, GetStringPosOfInsPtPos (fldP));
		UInt16 pos = GetStringPosOfInsPtPos(fldP);
		pos -= TxtPreviousCharSize(fldP->text, pos);
		SetInsPtFromStringPos(fldP, pos);
		}

	else if (! fldP->attr.singleLine)
		{
		if (fldP->lines[fldP->insPtYPos].start > 0)
			{
			MoveInsPtUpOneLine (fldP);
			fldP->insPtXPos = GetVisibleCharsInLine (fldP, fldP->insPtYPos);
			}
		}
}


/***********************************************************************
 *
 * FUNCTION:    InsPtRight
 *
 * DESCRIPTION: Move the insertion point one character right, if the 
 *              insertion point is at the end of the line, wrap
 *              to the start of the next line.
 *
 * PARAMETERS:	 fldP  pointer to a FieldType structure.
 *
 * RETURNED:	 nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/22/94	Initial Revision
 *			kwk	07/28/98	Return immediately if no text.
 *
 ***********************************************************************/
static void InsPtRight (FieldType * fldP)
{
	UInt16 pos;

	if (!fldP->text) return;
	
	if (! fldP->attr.insPtVisible)
		SetInsPtFromStringPos (fldP, GetStringPosOfInsPtPos (fldP));


	if (IsSelection (fldP))
		{
		pos = fldP->selLastPos;
		CancelSelection (fldP);
		SetInsPtFromStringPos (fldP, pos);
		}

	else if (fldP->insPtXPos < GetVisibleCharsInLine (fldP, fldP->insPtYPos) )
		fldP->insPtXPos += TxtNextCharSize (fldP->text, GetStringPosOfInsPtPos (fldP));

	else if (! fldP->attr.singleLine)
		{
		if (GetStringPosOfInsPtPos (fldP) < fldP->textLen)
			{
			MoveInsPtDownOneLine (fldP);
			fldP->insPtXPos = 0;
			}
		}
}


/***********************************************************************
 *
 * FUNCTION:	InsPtValid
 *
 * DESCRIPTION:	If we have an insertion point, make sure it's not in
 *				the middle of a double-byte character.
 *
 * PARAMETERS:	fldP  pointer to a FieldType structure.
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			kwk		12/05/98	Initial Revision
 *			kwk		12/10/98	Check xPos even if there's a sel range.
 *
 ***********************************************************************/
static void InsPtValid (FieldType * fldP)
{
	UInt16 pos = fldP->insPtXPos;
	UInt32 charStart, charEnd;
	
	// Don't worry about it if the insertion point is at the beginning of
	// the text.
	
	if (pos == 0)
		return;
	
	// If x position is relative to line start, calc real position.
	
	if (fldP->insPtYPos != absolutePosInX)
		pos = GetStringPosOfInsPtPos (fldP);
	
	if (pos >= fldP->textLen)
		return;
	
	// Find the beginning of the character that contains the insertion
	// point. If we wind up in the middle of a character, then we'll
	// back up to the beginning of the character.
	
	TxtCharBounds(fldP->text, pos, &charStart, &charEnd);
	fldP->insPtXPos -= (pos - charStart);
}


/***********************************************************************
 *
 * FUNCTION:    InsPtInField
 *
 * DESCRIPTION: The routine returns true if the field speciied has the
 *              focus and the insertion point is enabled.
 *
 * PARAMETERS:	 fldP  pointer to a FieldType structure.
 *
 * RETURNED:	 true if the insertion point is blinking in the specified 
 *              field.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/25/94	Initial Revision
 *
 ***********************************************************************/
static Boolean InsPtInField (FieldType * fldP)
{
	if (fldP->attr.hasFocus)
		return (InsPtEnabled());

	return (false);
}

/***********************************************************************
 *
 * FUNCTION:    ModifyText
 *
 * DESCRIPTION: Modifies text in the text handle. This routine correctly
 *					  handles modifying a record contained in the write-protected
 *					  data storage area. 
 *						
 *              This routine assumes the text handle is already locked
 *
 * PARAMETERS:	 fldP      pointer to a FieldType structure.
 *			       offset   offset into text to start writing
 *              charsP   pointer to next text to write
 *					 len		 len of text to write (in bytes)
 *
 * RETURNED:	 nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			ron	7/21/95	Initial Revision
 *
 ***********************************************************************/
static void ModifyText (FieldType * fldP, UInt16 offset, const Char * charsP, UInt16 len)
{
	UInt32	startOffset;
	MemPtr		chunkP;
	
	// Get the offset of the start of the text in the chunk handle
	chunkP = MemDeref(fldP->textHandle);
	startOffset = fldP->text - (Char *)chunkP;
	
	// Write the new text
	DmWrite(chunkP, startOffset+offset, charsP, len); 
}



/***********************************************************************
 *
 * FUNCTION:    ExpandTextString
 *
 * DESCRIPTION: Reallocate the memory block that contain the text of the 
 *              field
 *
 * PARAMETERS:	 fldP      pointer to a FieldType structure.
 *              newSize  amount to grow the memory block or zero to increase
 *                       by a default percentage.
 *
 * RETURNED:	 nothing.
 *
 * NOTE:			 If the handle is a database record, this routine may move
 *              the record to another heap, which will cause the handle to
 *              change.             
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/23/94	Initial Revision
 *       ron   7/21/95  Now use ModifyText routine in all places that modify the
 *                       text in order to support write-protected data storage.
 *
 ***********************************************************************/
static Boolean ExpandTextString (FieldType * fldP, UInt16 newSize)
{
	Err	   error;
	UInt16		offset;
	UInt16		blockSize;
	UInt16		newBlockSize;
	MemHandle handle;
	Char		zero=0;
	MemPtr		chunkP;

	// If a new length is not sepcified then expand the string by
	// 20% or a minimun of 16 bytes.
	if (newSize == 0)
		{
		newSize = min (fldP->maxChars + 1, 
							fldP->textBlockSize + max (fldP->textBlockSize / 5, 16));
		}
	else 
		newSize++;  // add one for null terminator

	
	// If we already have a handle, we'll resize it.  The field's text
	// string does not necessary start at the begining of the memory
	// block or end at the end of the memory block.  In other words
	// the text string may not be the only thing in the block.
	if (fldP->textHandle)
		{
		offset = fldP->text - (Char *) MemDeref(fldP->textHandle);

		blockSize = MemHandleSize(fldP->textHandle);
		newBlockSize = blockSize + newSize - fldP->textBlockSize;

		MemHandleUnlock (fldP->textHandle);
		ErrFatalDisplayIf(MemHandleLockCount(fldP->textHandle) > 0, 
			"Field object overlocked");

		error = MemHandleResize (fldP->textHandle, newBlockSize);
		
		chunkP = MemHandleLock(fldP->textHandle);
		fldP->text = (Char *) chunkP + offset;

		if (error)
			{
			if (MemHandleDataStorage (fldP->textHandle))
				{
				Int16 index;
				MemHandle h;
				DmOpenRef dbP;
				
				MemHandleUnlock (fldP->textHandle);
				
				index = DmSearchRecord (fldP->textHandle, &dbP);
				h = DmResizeRecord (dbP, index, newBlockSize);
				if (h)
					{
					fldP->textHandle = h;
					chunkP = MemHandleLock(fldP->textHandle);
					fldP->text = (Char *) chunkP + offset;
					}
				else
					{
					chunkP = MemHandleLock(fldP->textHandle);
					fldP->text = (Char *) chunkP + offset;
					FrmAlert (DeviceFullAlert);
					return (false);
					}
				}

			else
				ErrDisplay ("Memory reallocation error");
			}
		
		
		// Null terminate.
		if (fldP->textLen == 0)
			ModifyText(fldP, 0, &zero, sizeOf7BitChar('\0'));


		fldP->textBlockSize = newSize;
		}


	// Allocate a new block to hold the text string and null terminate it.
	else
		{
		handle = MemHandleNew (newSize);
		fldP->textHandle = handle;
		fldP->text = MemHandleLock (handle);

		ModifyText(fldP, 0, &zero, sizeOf7BitChar('\0'));
		
		fldP->textBlockSize = newSize;
		
		AllocateLines (fldP);
		}

	return (true);
}

/***********************************************************************
 *
 * FUNCTION:    AtEndOfLine
 *
 * DESCRIPTION: This routine will return true if the insertion point 
 *              is after the last character of a line.
 *
 * PARAMETERS:	 fldP      pointer to a FieldType structure.
 *
 * RETURNED:	 true if after last character of a line.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/25/94		Initial Revision
 *
 ***********************************************************************/
static Boolean AtEndOfLine (const FieldType * fldP, UInt16 pos)
{
	if (fldP->attr.singleLine)
		{
		if (fldP->attr.justification == leftAlign)
			return (fldP->insPtXPos+1 == fldP->textLen);
		else
			return (false);
		}

	if (pos == fldP->textLen)
		return (true);
	
	if (fldP->text[pos] == linefeedChr)
		return (true);
		
	return (false);
}


/***********************************************************************
 *
 * FUNCTION:    GetCharPos
 *
 * DESCRIPTION: Given a string position this routine return the postions
 *              in the string of the start of the character at the 
 *              specified position.
 *
 * PARAMETERS:	 fldP  pointer to a FieldType structure.
 *              pos  character position in the text of the field
 *
 * RETURNED:	 position of the end of the character
 *
 * REVISION HISTORY:
 *			Name	Date			Description
 *			----	----			-----------
 *			art		04/22/98	Initial Revision
 *			kwk		07/06/98	If either pos is 0 or we have no text ptr, return 0.
 *
 ***********************************************************************/
static UInt16 GetCharPos (FieldType * fldP, UInt16 pos)
{
	UInt32 startPos;
	UInt32 endPos;

	if (pos > fldP->textLen)
		pos = fldP->textLen;

	if ((pos == 0) || (fldP->text == NULL))
		return (0);
	
	TxtCharBounds (fldP->text, pos, &startPos, &endPos);
	return (startPos);
}


/***********************************************************************
 *
 * FUNCTION:    InvertCharsRange
 *
 * DESCRIPTION: Invert specified characer range on the specified line.
 *              This rountine performs the actual inverting.
 *
 * PARAMETERS:	 fldP      pointer to a FieldType structure.
 *              lineNum  line number
 *              startPos character position of start of range
 *              endPos   character position of end of range.
 *
 * RETURNED:	 nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/28/94		Initial Revision
 *
 ***********************************************************************/
static void InvertCharsRange (FieldType * fldP, UInt16 lineNum, UInt16 startPos,
	UInt16 endPos)
{
	RectangleType r;
	LineInfoType * line;
	
	if (fldP->attr.singleLine)
		{
		r.topLeft.x = FntCharsWidth (fldP->text, startPos);
		if (fldP->attr.justification == rightAlign)
			{
			r.topLeft.x += fldP->rect.extent.x - 
				FntCharsWidth (fldP->text, fldP->textLen);
			}	

		if ((r.topLeft.x) > fldP->rect.extent.x)
			return;

		r.extent.x = FntCharsWidth (&fldP->text[startPos], endPos-startPos);
		if ((r.topLeft.x + r.extent.x) > fldP->rect.extent.x)
			r.extent.x = fldP->rect.extent.x - r.topLeft.x;
		}
	else
		{
		line = &fldP->lines[lineNum];
		r.topLeft.x = FntLineWidth (&fldP->text[line->start], 
			startPos - line->start);
	
		r.extent.x = FntLineWidth (&fldP->text[line->start], 
			endPos - line->start) - r.topLeft.x;
		}

	r.topLeft.x += fldP->rect.topLeft.x;
	r.extent.y = FntLineHeight ();
	r.topLeft.y = fldP->rect.topLeft.y + (lineNum * r.extent.y);
	

	// Trailing spaces may cause the update region to be outside the bounds 
	// of the field.
	if (r.topLeft.x > fldP->rect.topLeft.x + fldP->rect.extent.x)
		return;
		
	if (r.topLeft.x + r.extent.x > fldP->rect.topLeft.x + fldP->rect.extent.x)
		r.extent.x = fldP->rect.extent.x - (r.topLeft.x - fldP->rect.topLeft.x);

	WinInvertRectangle (&r, 0);
}


/***********************************************************************
 *
 * FUNCTION:    UpdateHighlight
 *
 * DESCRIPTION: Adjust the current highlighted region to include or exclude
 *              the specified range of characters.
 *
 * PARAMETERS:	 fldP      pointer to a FieldType structure.
 *              startPos character position of start of range
 *              endPos   character position of end of range.
 *
 * RETURNED:	 nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/25/94		Initial Revision
 *
 ***********************************************************************/
static void UpdateHighlight (FieldType * fldP, UInt16 startPos, UInt16 endPos)
{

	// no previous selection, so just draw all selected
	if (fldP->selFirstPos == fldP->selLastPos)
		DrawMultiLineRange(fldP, startPos, endPos, true);
	
	// getting rid of entire selection, just draw all normal
	else if (startPos == endPos)
		DrawMultiLineRange(fldP, fldP->selFirstPos, fldP->selLastPos, false);

	// growing or shrinking existing selection
	// at beginning or end, or sometimes both ends
	else {

		// if adjusting beginning, draw some text in appropriate style
		if (fldP->selFirstPos != startPos)
			DrawMultiLineRange(fldP, min (fldP->selFirstPos, startPos),
						      max (fldP->selFirstPos, startPos), startPos < fldP->selFirstPos);

		// if adjusting end, draw some text in appropriate style
		if (fldP->selLastPos != endPos)
			DrawMultiLineRange(fldP, min (fldP->selLastPos, endPos),
						      max (fldP->selLastPos, endPos), endPos > fldP->selLastPos);
	}
}


/***********************************************************************
 *
 * FUNCTION:    InvertSelectionInLine
 *
 * DESCRIPTION: Invert the range of the current selection that is 
 *              in the specified line.
 *
 * PARAMETERS:	 fldP     - pointer to a FieldType structure.
 *              lineNum - line number (zero based).
 *
 * RETURNED:	 nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/21/94		Initial Revision
 *
 ***********************************************************************/
static void InvertSelectionInLine (FieldType * fldP, UInt16 lineNumber)
{
	LineInfoType * line;
	UInt16 startPos;
	UInt16 endPos;

	if (fldP->attr.singleLine)
		{
		DrawOneLineRange (fldP, 0, fldP->selFirstPos, fldP->selLastPos, false, true);
		return;
		}

	line = &fldP->lines[lineNumber];
	if ((fldP->selFirstPos < line->start+ line->length) && 
	    (fldP->selLastPos > line->start))
		{
		startPos = max (fldP->selFirstPos, line->start);
		endPos = min ( fldP->selLastPos, line->start + line->length);
		DrawOneLineRange (fldP, lineNumber, startPos, endPos, false, true);
		}
}


/***********************************************************************
 *
 * FUNCTION:		CalcFieldChecksum
 *
 * DESCRIPTION:	Calculate a checksum for <fldP> that is hopefully
 *						unique even when we've got fields inside of a table.
 *						That case is harder because the TableType object
 *						contains a FieldType struct, thus the fldP is always
 *						the same when switching fields in a table.
 *
 * PARAMETERS:		fldP    pointer to a FieldType structure.
 *
 * RETURNED:		Checksum of field.
 *
 *	HISTORY:
 *		09/10/99	kwk	Created by Ken Krugler.
 *
 ***********************************************************************/
static UInt16 CalcFieldChecksum(const FieldType* fldP)
{
	UInt16 checksum;
	
	// First calc the checksum of the text data.
	checksum = Crc16CalcBlock(fldP->text, fldP->textLen, 0);

	// Now factor in the field id & bounding rectangle.
	checksum = Crc16CalcBlock((const void*)fldP, sizeof(UInt16) + sizeof(RectangleType), checksum);
	
	return(checksum);
}


/***********************************************************************
 *
 * FUNCTION:		CheckDeferredTerm
 *
 * DESCRIPTION:	If <fldP> is the field that contains the suspended
 *						conversion session, toss the deferred session.
 *
 * PARAMETERS:		fldP    pointer to a FieldType structure.
 *
 * RETURNED:		nothing.
 *
 *	HISTORY:
 *		09/12/99	kwk	Created by Ken Krugler.
 *
 ***********************************************************************/
static void CheckDeferredTerm(const FieldType *fldP)
{
	if ((GInlineDefTerm)
	&& (GInlineDefField == fldP)
	&& (GInlineDefChecksum == CalcFieldChecksum(fldP)))
		{
		GInlineDefTerm = false;
		}
}


/***********************************************************************
 *
 * FUNCTION:    CalculateSelection
 *
 * DESCRIPTION: Set a current selection to the range specified and
 *              update the highlighting of the the selection.  If
 *              and startPos and endPos are the same, there will no
 *              highlighted selection.
 *
 * PARAMETERS:	 fldP     pointer to a FieldType structure.
 *              startPos character position of start of selection
 *              endPos   character position of end of selection 
 *					 penState single, double, or triple tap
 *
 * RETURNED:	 startPos character position of start of selection
 *              endPos   character position of end of selection 
 *
 * HISTORY:
 *		04/23/99	jwp	Initial Revision
 *		07/28/99	rbb	Set endPos to startPos, by default
 *							Switch to new TxtWordBounds to avoid debug alert in Gremlins
 *		09/01/99	gap	Add full line selection for triple tap.
 *		09/02/99	kwk	Fixed up triple-tap check for single-line fields. Added
 *							note re bug w/selection range and triple-tap.
 *		09/03/99	kwk	Look for previous word if we don't find anything following
 *							the passed <*startPos> offset.
 *		09/12/99	gap	Update for new multi-tap implementation.
 *
 ***********************************************************************/
void CalculateSelection(const FieldType* fldP, UInt16* startPos, UInt16* endPos, UInt8 tapCount )
{
	// Make sure that we don't leave endPos undefined
	*endPos = *startPos;

	if (tapCount == 2)
		{
		UInt32 start;
		UInt32 end;
		
		if (fldP->text != NULL)
			{
			Boolean foundWord;
			
			// Is there a word to select? We'll go for the word following the passed
			// offset (better would be to use offset + leading edge flag), and if that
			// doesn't work, check the word _before_ the offset.
			
			foundWord = TxtWordBounds(fldP->text, fldP->textLen, *startPos, &start, &end);
			if ((!foundWord) && (*startPos > 0))
				{
				UInt16 prevPos = *startPos - TxtPreviousCharSize(fldP->text, *startPos);
				foundWord = TxtWordBounds(fldP->text, fldP->textLen, prevPos, &start, &end);
				}
				
			if (foundWord)
				{
				*startPos = start;
				*endPos = end;
				}
			// If there's no word, leave the cursor where it was
			}
		}
	if (tapCount > 2)
		{
		// If the field contains text, get the information for the line corresponding to
		// y location of the user's tap position and calculate the beginning and end 
		// selection bounds.
		if (fldP->text != NULL)
			{
			if (fldP->lines != NULL)
				{
				LineInfoType *line;
				
				// DOLATER gap - insPtYPos is only valid if there's an insertion point,
				// (fldP->attr.insPtVisible is true), otherwise if there's a selection
				// range you need to calc the line # from the starting offset.
				line = &fldP->lines[fldP->insPtYPos];
				
				*startPos = line->start;
				*endPos = line->start + line->length;
				}
			else
				{
				*startPos = 0;
				*endPos = fldP->textLen;
				}
			}
		}
	
}


/***********************************************************************
 *
 * FUNCTION:    SetSelection
 *
 * DESCRIPTION: Set a current selection to the range specified and
 *              update the highlighting of the the selection.  If
 *              and startPos and endPos are the same, there will no
 *              highlighted selection.
 *
 * PARAMETERS:	 fldP      pointer to a FieldType structure.
 *              startPos character position of start of selection
 *              endPos   character position of end of selection 
 *
 * RETURNED:	
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/23/94	Initial Revision
 *			kwk	12/08/98	If we had a selection range, make sure the
 *								insertion point position is updated.
 *
 ***********************************************************************/
static void SetSelection (FieldType * fldP, UInt16 startPos, UInt16 endPos)
{
	UInt16 temp;

	if (startPos > endPos) {
    	temp = startPos;
    	startPos = endPos;
		endPos = temp;
	}

	UpdateHighlight (fldP, startPos, endPos);

	fldP->selFirstPos = startPos;
	fldP->selLastPos = endPos;
}


/***********************************************************************
 *
 * FUNCTION:    CancelSelection
 *
 * DESCRIPTION: Unhighlight the current selection an set and start and end
 *              postions of the selection range to zero.
 *
 * PARAMETERS:	 fldP  pointer to a FieldType structure.
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/25/94	Initial Revision
 *			kwk	12/08/98	If we had a selection range, make sure the
 *								insertion point position is updated.
 *
 ***********************************************************************/
static void CancelSelection (FieldType * fldP)
{
	if (IsSelection (fldP))
		UpdateHighlight (fldP, fldP->selFirstPos, fldP->selFirstPos);
	fldP->selLastPos = fldP->selFirstPos;
}


/***********************************************************************
 *
 * FUNCTION:    ScrollSelectionDown
 *
 * DESCRIPTION: Scroll the field down a line and highlight the new line that
 *              is visible at the top of the field.
 *
 *              This routine is called while drag selecting.
 *
 * PARAMETERS:	 fldP       pointer to a FieldType structure.
 *              anchorPos anchor position of selection (characher position)
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date			Description
 *			----	----			-----------
 *			art	11/23/94		Initial Revision
 *			gap	11/04/99		Boolean designating selection for DrawMultiLineRange was incorrect.
 *
 ***********************************************************************/
static void ScrollSelectionDown (FieldType * fldP, UInt16 anchorPos)
{
	if (fldP->attr.singleLine) return;

	if (! fldP->lines) return;

	if (fldP->lines->start)
		{

		if (fldP->lines->start > anchorPos)
			{
			DrawMultiLineRange (fldP, fldP->selFirstPos, fldP->selLastPos, false);
			fldP->selLastPos = StartOfPreviousLine (fldP, fldP->lines->start);
			}

		ScrollDisplayDown (fldP);
		SetSelection (fldP, fldP->lines->start, anchorPos);
		}
}


/***********************************************************************
 *
 * FUNCTION:    ScrollSelectionUp
 *
 * DESCRIPTION: Scroll the field up a line and highlight the new line that
 *              is visible at the bottom of the field.
 *
 *              This routine is called while drag selecting.
 *
 * PARAMETERS:	 fldP  pointer to a FieldType structure.
 *              anchorPos anchor position of selection (characher position)
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date			Description
 *			----	----			-----------
 *			art	11/23/94		Initial Revision
 *			gap	11/04/99		Boolean designating selection for DrawMultiLineRange was incorrect.
 *
 ***********************************************************************/
static void ScrollSelectionUp (FieldType * fldP, UInt16 anchorPos)
{
	UInt16 pos;
	LineInfoType * line;

	if (fldP->attr.singleLine) return;

	if (! fldP->lines) return;

	line = &fldP->lines[GetVisibleLines(fldP)-1];
	pos = line->start + line->length;

	if ( (pos < fldP->textLen) || 
	     (line->length && fldP->text[pos-1] == linefeedChr) )
		{
		if (pos < anchorPos)
			{
			DrawMultiLineRange (fldP, fldP->selFirstPos, fldP->selLastPos, false);
			pos += FormatLine (fldP, pos);
			fldP->selFirstPos = pos;
			}
			
		ScrollDisplayUp (fldP);
		pos = line->start + line->length;
		SetSelection (fldP, anchorPos, pos );
		}
}


/***********************************************************************
 *
 * FUNCTION:    GetBoundsOfInsPtPos
 *
 * DESCRIPTION: Return the bound of the insertion point position.  
 *
 * PARAMETERS:	 fldP  pointer to a FieldType structure.
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/25/94	Initial Revision
 *
 ***********************************************************************/
static void GetBoundsOfInsPtPos (FieldType * fldP, RectangleType * rP)
{
	UInt16 pos, width, charsInLine, charSize;
	Char * chars;
	LineInfoType * line;

	rP->topLeft.x = fldP->rect.topLeft.x;

	if (fldP->attr.singleLine)
		{
		chars = fldP->text;
		charsInLine = fldP->textLen;
		if (fldP->attr.justification == rightAlign)
			rP->topLeft.x += fldP->rect.extent.x - GetLineCharsWidth (fldP, 0);
		}
	else if (fldP->text)
		{
		line = &fldP->lines[fldP->insPtYPos];
		chars = &fldP->text[line->start];
		charsInLine = line->length;
		}
	else
		{
		charsInLine = 0;
		}

	if (charsInLine)
		{
		width = 0;
		pos = fldP->insPtXPos;

		// Calculate the size of half of the width of the character before
		// the insertion point.
		if (pos > 0)
			{
			charSize = TxtPreviousCharSize (chars, pos);

			if (chars[pos-charSize] != tabChr)
				width = (FntCharsWidth (&chars[pos-charSize], charSize) + 1) / 2;
			else if (pos > 1)
				width = (FntLineWidth (chars, pos) - 
							FntLineWidth (chars, pos-charSize) + 1) / 2;
			else
				width = (fntTabChrWidth + 1) / 2;


			rP->topLeft.x += FntLineWidth (chars, pos) - width;
			}

		// Add half of the width of the character following the insertion point.
		charSize = TxtNextCharSize (chars, pos);

		if (chars[pos] != tabChr)
			width += FntCharsWidth (&chars[pos], charSize) / 2;

		else if (charsInLine > 1)
			width += (FntLineWidth (chars, pos+charSize) - 
						 FntLineWidth (chars, pos)) / 2;
		else
			width += fntTabChrWidth / 2;

		rP->extent.x = width;
		}

	// If there are not any character on the line, the bounds extent to the 
	// full width of the field.
	else
		{
		rP->extent.x = fldP->rect.extent.x;
		}


	rP->topLeft.y = fldP->rect.topLeft.y + (fldP->insPtYPos * FntLineHeight()); 
	rP->extent.y = FntLineHeight();
}


/***********************************************************************
 *
 * FUNCTION:     UndoSaveSelection
 *
 * DESCRIPTION:  This routine save the current field selection to 
 *               the undo buffer.
 *
 * PARAMETERS:   fldP - pointer to the field that has the focus
 *               pos - character postion
 *
 * RETURNED:     true if selection was sucessfully saved to the undo
 *               buffer
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/7/95	Initial Revision
 *
 ***********************************************************************/
static Boolean UndoSaveSelection (FieldType * fldP)
{
	UInt16 count;
	
	count = fldP->selLastPos - fldP->selFirstPos;
	if (count < undoBufferSize)
		{
		MemMove (UndoGlobals.buffer, &fldP->text[fldP->selFirstPos], count);
		UndoGlobals.bufferLen = count;
		}
	else
		{
		if (FrmAlert (UndoAlert) == UndoCancelButton) 
			return (false);
		else
			UndoGlobals.bufferLen = 0;
		}
	return (true);
}


/***********************************************************************
 *
 * FUNCTION:     UndoReset
 *
 * DESCRIPTION:  This routine resets the undo gloabal variables.  It 
 *               should be called each time a field object gains the
 *              focus.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/7/95	Initial Revision
 *
 ***********************************************************************/
static void UndoReset (void)
{
	UndoGlobals.mode = undoNone;
}


/***********************************************************************
 *
 * FUNCTION:     UndoRecordTyping
 *
 * DESCRIPTION:  This routine is called wherever a character is input.
 *               If the current undo mode is not typing mode, the undo 
 *               state is cleared, and the starting and ending
 *               positions of the typed character range are initialized 
 *               to the values passed.  If the current undo mode is
 *               typing mode the end position of the typed 
 *               character range is incremented.
 *
 * PARAMETERS:   fldP - pointer to the field that has the focus
 *               pos - character postion
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/7/95	Initial Revision
 *
 ***********************************************************************/
static Boolean UndoRecordTyping (FieldType * fldP,  UInt16 pos)
{
	if (UndoGlobals.mode == undoTyping)
		{
		UndoGlobals.mode = undoNone;
		}
	else
		{
		UndoGlobals.mode = undoTyping;
		UndoGlobals.start = pos;
		UndoGlobals.end = pos + 1;
		UndoGlobals.bufferLen = 0;
		
		if (fldP->selFirstPos != fldP->selLastPos)
			{
			if (! UndoSaveSelection (fldP))
				return (false);
			}
		}
	return (true);
}


/***********************************************************************
 *
 * FUNCTION:     UndoRecordInput
 *
 * DESCRIPTION:  This routine is called wherever a input is received from 
 *               the Text Services Manager. If the current undo mode is not 
 *               input mode, the undo state is cleared.  Oterwise  and the 
 *               starting and ending positions of the input characters  is 
 *               saved and the text replaced by the input text is copied
 *               to the undo buffer.
 *
 * PARAMETERS:   fldP       - pointer to the field that has the focus
 *               pos       - character postion
 *               inputLen  - number of byte added
 *               deleteLen - number of byte replaced
 *
 * RETURNED:		True if we were able to record the undo info, false if
 *						the amount of text to record is greater than the buffer
 *						and the user decided to cancel the operation.
 *
 * HISTORY:
 *		09/23/98	art	Created by Art Lamb.
 *		12/07/99	kwk	Now actually ask the user about continuing without
 *							undo support, and return false if they don't want to.
 *
 ***********************************************************************/
static Boolean UndoRecordInput (FieldType* fldP,  UInt16 pos, UInt16 inputLen, UInt16 deleteLen)
{
	if (UndoGlobals.mode == undoInput)
		{
		UndoGlobals.mode = undoNone;
		}
	else
		{
		UndoGlobals.mode = undoInput;
		UndoGlobals.start = pos;
		UndoGlobals.end = pos + inputLen;
		
		if (deleteLen < undoBufferSize)
			{
			MemMove (UndoGlobals.buffer, &fldP->text[pos], deleteLen);
			UndoGlobals.bufferLen = deleteLen;
			}
		else
			{
			if (FrmAlert (UndoAlert) == UndoCancelButton) 
				return (false);
			else
				UndoGlobals.bufferLen = 0;
			}
		}
	return (true);
}


/***********************************************************************
 *
 * FUNCTION:     UndoRecordBackspaces
 *
 * DESCRIPTION:  This routine is called whenever a backspace character 
 *               is input.  If the current undo mode is not backspace mode,
 *               and there is a selection,  the selection is saved to the 
 *               undo buffer and the undo mode is set to backspace mode.  If
 *               there is not a selection, the character to backspace is 
 *               save to the undo buffer.
 *
 * PARAMETERS:   fldP - pointer to the field that has the focus
 *               pos - character position of the chaacter to backspace
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/7/95	Initial Revision
 *
 ***********************************************************************/
static Boolean UndoRecordBackspaces (FieldType * fldP, UInt16 pos)
{
	UInt16	charSize;

	if (UndoGlobals.mode == undoBackspace)
		{
		charSize = TxtNextCharSize (fldP->text, pos);

		while (UndoGlobals.bufferLen + charSize > undoBufferSize)
			UndoGlobals.bufferLen -= TxtPreviousCharSize (UndoGlobals.buffer, UndoGlobals.bufferLen);

		MemMove (&UndoGlobals.buffer[charSize], UndoGlobals.buffer,	UndoGlobals.bufferLen);
		MemMove (UndoGlobals.buffer, &fldP->text[pos], charSize);
		UndoGlobals.bufferLen += charSize;
		}
	
	else
		{
		if (fldP->selFirstPos != fldP->selLastPos)
			{
			UndoGlobals.mode = undoDelete;
			if (! UndoSaveSelection (fldP))
				{
				UndoGlobals.mode = undoNone;
				return (false);
				}
			}
		else
			{ 
			charSize = TxtNextCharSize (fldP->text, pos);
			UndoGlobals.mode = undoBackspace;
			UndoGlobals.bufferLen = charSize;
			MemMove (UndoGlobals.buffer, &fldP->text[pos], charSize);
			}
		}
	return (true);
}



/***********************************************************************
 *
 * FUNCTION:     UndoRecordPaste
 *
 * DESCRIPTION:  This routine is called wherever a paste editing command
 *               is executed.  The starting and ending positions of the 
 *               character pasted are recorded,  If there is a selection 
 *               the selected character are copied to the undo buffer.
 *
 * PARAMETERS:   fldP - pointer to the field that has the focus
 *               pos - character position where the pasted occurs
 *
 * RETURNED:     false if paste operation was canceled
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/7/95	Initial Revision
 *
 ***********************************************************************/
static Boolean UndoRecordPaste (FieldType * fldP, UInt16 pos)
{
	UInt16 count;

	UndoGlobals.mode = undoPaste;
	UndoGlobals.start = pos;

	ClipboardGetItem (clipboardText, &count);
	UndoGlobals.end = UndoGlobals.start + count;
	
	if (fldP->selFirstPos != fldP->selLastPos)
		{
		return (UndoSaveSelection (fldP));
		}
	else
		UndoGlobals.bufferLen = 0;
		
	return (true);
}


/***********************************************************************
 *
 * FUNCTION:     UndoRecordCut
 *
 * DESCRIPTION:  This routine is called wherever a cat editing command
 *               is executed.  
 *
 * PARAMETERS:   fldP - pointer to the field that has the focus
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/7/95	Initial Revision
 *
 ***********************************************************************/
static void UndoRecordCut (FieldType * /* fldP */)
{
	UndoGlobals.mode = undoCut;
}


#pragma mark ------- Exported API -------


/***********************************************************************
 *
 * FUNCTION:     FldUndo
 *
 * DESCRIPTION:  Always undo the right thing.  Got it?
 *
 * PARAMETERS:   fldP - pointer to the field that has the focus
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/7/95	Initial Revision
 *
 ***********************************************************************/
void FldUndo (FieldType * fldP)
{
	UInt16 count;
	MemHandle charsH;
	Char * charsP;
	
	ECNullFieldPtr(fldP);

	switch (UndoGlobals.mode)
		{
		case undoNone:
			SndPlaySystemSound (sndError);
			break;
			
		// Undo typing.
		case undoTyping:
		case undoInput:
			FldDelete (fldP, UndoGlobals.start, UndoGlobals.end);
			FldInsert (fldP, UndoGlobals.buffer, UndoGlobals.bufferLen);
			break;
			

		// Undo backspaces.
		case undoBackspace:
			FldInsert (fldP, UndoGlobals.buffer, UndoGlobals.bufferLen);
			break;

			
		// Undo delete.
		case undoDelete:
			FldInsert (fldP, UndoGlobals.buffer, UndoGlobals.bufferLen);
			break;
			

		// Undo paste.
		case undoPaste:
			FldDelete (fldP, UndoGlobals.start, UndoGlobals.end);
			FldInsert (fldP, UndoGlobals.buffer, UndoGlobals.bufferLen);
			break;
			

		// Undo cut.
		case undoCut:
			charsH = ClipboardGetItem (clipboardText, &count);
			if (charsH)
				{
				charsP = MemHandleLock (charsH);
				FldInsert (fldP, charsP, count);
				MemPtrUnlock (charsP);
				}
			break;
		}

	UndoReset ();
}


/***********************************************************************
 *
 * FUNCTION:    FldDrawField
 *
 * DESCRIPTION: This routine draw the text of a field and the frame.
 *
 * PARAMETERS:	 fldP   pointer to a FieldType structure.
 *
 * RETURNED:	nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/19/94	Initial Revision
 *
 ***********************************************************************/
void FldDrawField(FieldType* fldP)
{
	FontID      curFont;
	Boolean     insPtOn;

	ECValidateField(fldP, false, false);

	if (! fldP->attr.usable) return;

	curFont = FntSetFont (fldP->fontID);

	// DOLATER kwk - InsPtInField isn't really correct, since it returns true if
	// the field has the focus bit set and the (external) insertion point
	// is enabled. These will both be true for the case where the field
	// has an insertion point which isn't currently visible, but is waiting
	// for the field to auto-expand.
	 
	if ((insPtOn = InsPtInField (fldP)) == true)
		{
		InsPtEnable (false);
		}

	DrawAllLines (fldP);

	if (insPtOn && fldP->attr.insPtVisible)
		{
	  	InsPtEnable (true);
		SetBlinkingInsPtLocation (fldP);
		}
	else if (IsSelection (fldP))
		DrawMultiLineRange (fldP, fldP->selFirstPos, fldP->selLastPos, true);

	fldP->attr.visible = 1;

	FntSetFont (curFont);
}


/***********************************************************************
 *
 * FUNCTION:    FldEraseField
 *
 * DESCRIPTION: This routine erases the text of a field and turns off
 *              the insertion point if its in the field.
 *
 * PARAMETERS:  fldP    pointer to a FieldType structure.
 *
 * RETURNED:   nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/9/94	Initial Revision
 *
 ***********************************************************************/
void FldEraseField(FieldType* fldP)
{
	ECValidateField(fldP, false, false);

	if ( fldP->attr.visible  && fldP->attr.usable)
		{
		if (fldP->attr.hasFocus)
			InsPtEnable (false);

		WinEraseRectangle (&fldP->rect, 0);

		fldP->attr.visible = false;
		}
}


/***********************************************************************
 *
 * FUNCTION:    FldCopy
 *
 * DESCRIPTION: This routine copies the current selection to the text
 *              clipboard.
 *
 * PARAMETERS:  fldP    pointer to a FieldType structure.
 *
 * RETURNED:    nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/9/94	Initial Revision
 *			trev	08/07/97	made non modified passed variabls constant
 *
 ***********************************************************************/
void FldCopy(const FieldType* fldP)
{
	UInt16 charsToCopy;

	ECValidateField(fldP, false, false);

	// Beep if there is not a selection.
	if (! IsSelection (fldP)) 
		{
		SndPlaySystemSound(sndError);
		return;
		}
	
	charsToCopy = fldP->selLastPos - fldP->selFirstPos;

	if (charsToCopy > cbdMaxTextLength)
		{
		FrmAlert(ClipboardLimitAlert);
		return;
		}

	ClipboardAddItem (clipboardText, fldP->text + fldP->selFirstPos, 
		charsToCopy);
}


/***********************************************************************
 *
 * FUNCTION:    FldCut
 *
 * DESCRIPTION: This routine copies the current selection to the text
 *              clipboard, deletes the selection from the field and
 *              redraws the field.
 *
 * PARAMETERS:	 fldP    pointer to a FieldType structure.
 *
 * RETURNED:	 nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/9/94	Initial Revision
 *			art	4/19/96	Added autoshifting
 *			trev	08/07/97	made non modified passed variabls constant
 *			grant 2/5/99	Make sure the field is editable first
 *
 ***********************************************************************/
void FldCut(FieldType* fldP)
{
	UInt16 charsToDelete;

	ECValidateField(fldP, false, false);

	// Beep if there is not a selection or the field is not editable.
	if (! IsSelection (fldP) || !fldP->attr.editable)
		{
		SndPlaySystemSound (sndError);
		return;
		}
	
	charsToDelete = fldP->selLastPos - fldP->selFirstPos;
	if (charsToDelete > cbdMaxTextLength)
		{
		FrmAlert (ClipboardLimitAlert);
		return;
		}

	UndoRecordCut (fldP);

	ClipboardAddItem (clipboardText, fldP->text + fldP->selFirstPos, charsToDelete);
	FldDelete (fldP, fldP->selFirstPos, fldP->selLastPos);
	
	AutoShift (fldP);

	ECValidateField (fldP, true, true);
}


/***********************************************************************
 *
 * FUNCTION:    FldPaste
 *
 * DESCRIPTION: This routine replaces the current selection with the 
 *              contents of the text clipboard.  If there is no current 
 *              selection, the clipboard text is inserted at the position
 *              of the insertion point.
 *
 * PARAMETERS:  fldP    pointer to a FieldType structure.
 *
 * RETURNED:    nothing.
 *
 *	HISTORY:
 *		12/09/94	art	Created by Art Lamb.
 *		07/21/95	ron	Now use ModifyText routine in all places that modify
 *							the text in order to support write-protected data
 *							storage.
 *		08/08/95	art	Code moved to FldInsert
 *		04/19/96	art	Added autoshifting
 *		07/28/99	kwk	Call TextServicesReset if field has focus, not other
 *							way around.
 *		09/12/99	kwk	Call CheckDeferredTerm.
 *		04/08/00	kwk	Pass false for update param to TextServicesReset.
 *		11/13/00	kwk	Disallow pasting non-numeric or extra decimal separators
 *							into numeric field.
 *
 ***********************************************************************/
void FldPaste(FieldType* fldP)
{
	UInt16 pos;
	UInt16 pasteLen;
	MemHandle pasteCharsH;
	Char* pasteChars;
	Boolean canPaste = true;
	
	ECValidateField(fldP, false, false);

	// Get the text of paste from the clipboard,  if there's nothing
	// on the clipboard, beep and exit.
	pasteCharsH = ClipboardGetItem (clipboardText, &pasteLen);
	if ((pasteCharsH == NULL) || (pasteLen == 0))
		{
		canPaste = false;
	 	}
	else if (!fldP->attr.editable)
		{
		canPaste = false;
		}
	else if (fldP->attr.numeric)
		{
		UInt16 foundDecimal = false;
		UInt16 curOffset = 0;
		
		// Make sure every character in the text is either numeric
		// or a decimal separator, and if so, that there's only
		// one decimal separator.
		pasteChars = MemHandleLock(pasteCharsH);
		while (curOffset < pasteLen)
			{
			WChar curChar;
			curOffset += TxtGetNextChar(pasteChars, curOffset, &curChar);
			if (curChar == UIDecimalSeparator)
				{
				// If the paste text already has a decimal character, or
				// one already exists in the field text, bail out.
				if ((foundDecimal)
				 || ((fldP->text != NULL) && (StrChr(fldP->text, UIDecimalSeparator) != NULL)))
					{
					canPaste = false;
					break;
					}
				else
					{
					foundDecimal = true;
					}
				}
			else if (!TxtCharIsDigit(curChar))
				{
				canPaste = false;
				break;
				}
			}

		MemPtrUnlock(pasteChars);
		}
	
	if (!canPaste)
		{
		SndPlaySystemSound(sndError);
	 	return;
		}

	if (fldP->attr.hasFocus)
		{
		TextServicesReset (fldP, false);
		}
	else
		{
		CheckDeferredTerm(fldP);
		}
	
	if (fldP->selLastPos != fldP->selFirstPos)
		pos = fldP->selFirstPos;
	else
		pos = GetStringPosOfInsPtPos (fldP);

	
	// Copy to current selection the undo buffer, if it will fix.
	if (UndoRecordPaste (fldP, pos))
		{
		pasteChars = MemHandleLock (pasteCharsH);

		if (! FldInsert (fldP, pasteChars, pasteLen))
			UndoReset ();

		MemPtrUnlock (pasteChars);
		}

	AutoShift (fldP);
}


/***********************************************************************
 *
 * FUNCTION:    FldInsert
 *
 * DESCRIPTION: This routine replaces the current selection with the 
 *              string passed.  If there is no current selection, 
 *              the string passed is inserted at the position
 *              of the insertion point.
 *
 * PARAMETERS:  fldP    pointer to a FieldType structure.
 *
 * RETURNED:    true if the string was successfully inserted.
 *
 * HISTORY:
 *		08/08/94	art	Created by Art Lamb
 *		08/08/97	trev	made non modified passed variabls constant
 *		09/12/99	kwk	Call CheckDeferredTerm
 *		04/08/00	kwk	Pass false for update param to TextServicesReset.
 *
 ***********************************************************************/
Boolean FldInsert (FieldType * fldP, const Char * insertChars, 
	UInt16 insertLen)
{
	UInt16 newLen;
	UInt16 charsToDelete;
	UInt16 pos;

	ECValidateField(fldP, false, false);

	if (! fldP->attr.editable) return (false);

	if (insertLen == 0) return (false);
	
	if (fldP->attr.hasFocus)
		{
		TextServicesReset (fldP, false);
		}
	else
		{
		CheckDeferredTerm(fldP);
		}
	
	charsToDelete = fldP->selLastPos - fldP->selFirstPos;
	newLen = fldP->textLen + insertLen - charsToDelete ;

	// If inserting the text passed will expand the field 
	// beyond its maximun character limit, don't insert the text.
	if (newLen > fldP->maxChars)
		{
		SndPlaySystemSound (sndError);
		return (false);
		}


	// If necessary, expand the memory block that contains the field's 
	// text string.
//	if (newLen >= fldP->textBlockSize)
//		{
//		if (! ExpandTextString (fldP, newLen))
//			{
//			return (false);
//			}
//		}

	// If there's a selection, use that as the insert pos, otherwise
	// use the insertion point.
	if (charsToDelete)
		pos = fldP->selFirstPos;
	else
		pos = GetStringPosOfInsPtPos (fldP);

	InsertTextAndWrap(fldP, insertChars, insertLen, pos, charsToDelete, pos + insertLen);
	
	ECValidateField (fldP, true, true);
	return (true);
}


/***********************************************************************
 *
 * FUNCTION:    FldDelete
 *
 * DESCRIPTION: This routine deletes the specified range of characters 
 *              from the field and redraws the field.
 *
 * PARAMETERS:	 fldP    pointer to a FieldType structure.
 *              start  starting character position
 *              end    ending character position
 *
 * RETURNED:	 nothing.
 *
 * HISTORY:
 *		08/08/95	art	Created by Art Lamb.
 *		07/02/96	art	Add check text value
 *		11/11/98	roger	Fixed dereference of fldP->lines when it's NULL.
 *		09/12/99	kwk	Call CheckDeferredTerm.
 *		04/08/00	kwk	Pass false for update param to TextServicesReset.
 *
 ***********************************************************************/
void FldDelete (FieldType * fldP, UInt16 start, UInt16 end)
{
	FontID curFont;
	UInt16 charsToDelete;
	UInt16 pos;

	ECValidateField(fldP, false, false);

	if (!fldP->text) return;

	curFont = FntSetFont (fldP->fontID);

	fldP->attr.dirty = true;

	CancelSelection (fldP);

	if (fldP->attr.hasFocus)
		{
		TextServicesReset (fldP, false);
		InsPtEnable (false);
		}
	else
		{
		CheckDeferredTerm(fldP);
		}
	
	charsToDelete = end - start;
	pos = start;

	DeleteFromText (fldP, pos, charsToDelete);

	if (fldP->attr.singleLine || 
		pos < fldP->lines->start || 
		!fldP->attr.insPtVisible)
		{
		FormatAndDrawFromPos (fldP, pos, fldP->attr.visible);
		SetInsPtFromStringPos (fldP, pos);
		}
	else
		{
		SetInsPtFromStringPos (fldP, pos);
		UpdateStartOfLines (fldP, -charsToDelete, fldP->insPtYPos+1);
		FormatAndDrawIfWrappingChanged (fldP, fldP->insPtYPos);
		SetInsPtFromStringPos (fldP, pos);			
		}

	if (fldP->attr.hasFocus)
		{
		SetBlinkingInsPtLocation (fldP);
		InsPtEnable (fldP->attr.visible);
		}

	if (! CompactFieldHeight (fldP, pos))
		ExpandFieldHeight (fldP, pos);

	FldSendChangeNotification (fldP);

	FntSetFont (curFont);
	
	ECValidateField (fldP, true, true);
}


/***********************************************************************
 *
 * FUNCTION:	FldRecalculateField
 *
 * DESCRIPTION: Recalculate the beginnings (starting character position)
 *		and length of all the visible lines of the field. Note that this
 *		routine does nothing for a single-line field.
 *
 * PARAMETERS:
 *		fldP		 ->	pointer to a FieldType structure.
 *		redraw	 ->	True if text should be redrawn.
 *
 * RETURNED:	 nothing.
 *
 * HISTORY:
 *		11/28/94	art	Created by Art Lamb.
 *		12/06/98	kwk	If the ins pt isn't visible, set the yPos to the
 *							special absolutePosInX value, so that ECValidateField
 *							is happy.
 *		09/08/00	kwk	Always reallocate the line starts array, since it might
 *							need to be modified if the font or field height changed.
 *					kwk	Always re-wrap the text, even if <redraw> is false.
 *
 ***********************************************************************/
void FldRecalculateField (FieldType* fldP, Boolean redraw)
{
	FontID curFont;

	ECValidateField(fldP, false, false);

	if (fldP->attr.singleLine)
	{
		return;
	}
	
	curFont = FntSetFont(fldP->fontID);
	
	// Always for the reallocation of the linestarts array, since if the
	// caller has changed the field object height or the font (directly,
	// vs FldSetFont), this needs to be done.
	if (fldP->lines != NULL)
	{
		MemPtrFree(fldP->lines);
		fldP->lines = NULL;
	}
	AllocateLines(fldP);
	
	// Reset the insertion point to be at the beginning of the text.
	fldP->insPtXPos = 0;
	if (!fldP->attr.insPtVisible)
	{
		fldP->insPtYPos = absolutePosInX;
	}
	else
	{
		fldP->insPtYPos = 0;
	}

	// If we're redrawing the text, turn off the insertion point so
	// it doesn't restore stale bits the next time it blinks.
	if (redraw && InsPtInField(fldP))
		{
		InsPtEnable(false);
		}
	
	FormatAndDrawLines(fldP, redraw && fldP->attr.visible);

	FntSetFont (curFont);

	ECValidateField (fldP, false, true);
} // FldRecalculateField


/***********************************************************************
 *
 * FUNCTION:    FldGrabFocus
 *
 * DESCRIPTION: Set the location and height the blinking insertion point
 *              and start the insertion point blinking.
 *
 * PARAMETERS:	 fldP    pointer to a FieldType structure.
 *
 * RETURNED:	 nothing.
 *
 *	HISTORY:
 *		12/01/94	art	Created by Art Lamb
 *		03/27/96	art	Add autoshifting
 *		09/28/98	kwk	Added call to TextServicesReset if not numeric.
 *		08/07/99	kwk	Added support for restarting a deferred inline session.
 *		08/24/99	kwk	Make sure font is set before call to DrawMultiLineRange.
 *		09/10/99	kwk	Call CalcFieldChecksum to derive better checksum.
 *
 ***********************************************************************/
void FldGrabFocus (FieldType* fldP)
{
	FontID curFont;
	
	ECValidateField(fldP, false, false);

	if ((! fldP->attr.editable) || (! fldP->attr.usable)) return;

	if (fldP->attr.hasFocus) return;
	
	fldP->attr.hasFocus = true;

	curFont = FntSetFont (fldP->fontID);
	InsPtSetHeight (FntLineHeight());

	if ( (! IsSelection(fldP)) && fldP->attr.insPtVisible)
		{
		SetBlinkingInsPtLocation (fldP);
		if (fldP->attr.visible)
			InsPtEnable (true);
		}

	// If it's not a numeric field, then we want to make sure the input
	// method is in sync with the field; this should protect against sync
	// problems when switching between non-numeric fields.
	if (!fldP->attr.numeric)
		{
		// Check to see if we can just start using the previous conversion
		// status, otherwise we have to reset.
		if ((GInlineDefTerm)
		&& (GInlineDefField == fldP)
		&& (GInlineDefChecksum == CalcFieldChecksum(fldP)))
			{
			GInlineDefTerm = false;
			GInlineActive = true;
			
			// Redraw all of the inline text so that it gets underlined.
			DrawMultiLineRange (fldP, GInlineStart, GInlineEnd, false);
			
			// If anything is selected we need to also redraw it at this time.
			if (IsSelection (fldP))
				DrawMultiLineRange (fldP, fldP->selFirstPos, fldP->selLastPos, true);
			}
		else
			{
			TextServicesReset (fldP, false);
			}
		}
	
	FntSetFont (curFont);

	// Auto-shift if the field is empty.
	if (fldP->attr.autoShift && (! fldP->textLen))
		GrfSetState (false, false, true);
	else
		GrfSetState (false, false, false);

	ECValidateField (fldP, false, true);
}


/***********************************************************************
 *
 * FUNCTION:    FldReleaseFocus
 *
 * DESCRIPTION: Turn off the blinking insertion point, reset the graffiti
 *              state and the undo state.
 *
 * PARAMETERS:	 fldP    pointer to a FieldType structure.
 *
 * RETURNED:	 nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/8/94	Initial Revision
 *			trev	08/07/97	made non modified passed variabls constant
 *			kwk	09/28/98	Removed call to turn ins pt off after calling
 *								TextServicesReset().
 *
 ***********************************************************************/
void FldReleaseFocus (FieldType * fldP)
{
	UInt16 charsToDelete;
	FontID curFont;
	
	ECValidateField(fldP, false, false);

	if (fldP->attr.hasFocus)
		{
		curFont = FntSetFont (fldP->fontID);
		InsPtEnable (false);

		// Delete partial graffiti macro names.
		GrfFieldChange (true, &charsToDelete);
		
//		if (charsToDelete)
//			{
//			pos = GetStringPosOfInsPtPos (fldP);
//			FldDelete (fldP, pos, pos + charsToDelete);
//			}

		fldP->attr.hasFocus = false;

		// If inline is active, save off the state rather than killing it, so
		// that we can hopefully return to where we left off in FldGrabFocus
		// w/o killing the inline conversion session.
		
		// DOLATER kwk - also add FEP munged count just to make sure nobody else
		// is using the FEP while we're gone?
		if (GInlineActive)
			{
			GInlineDefTerm = true;
			GInlineDefField = fldP;
			GInlineDefChecksum = CalcFieldChecksum(fldP);
			
			GInlineActive = false;
			
			// Redraw all of the inline text so that the underline is removed.
			DrawMultiLineRange (fldP, GInlineStart, GInlineEnd, false);
			
			// If anything is selected we need to also redraw it at this time.
			if (IsSelection (fldP))
				DrawMultiLineRange (fldP, fldP->selFirstPos, fldP->selLastPos, true);
			}
		
		UndoReset ();

		FntSetFont (curFont);
		
		ECValidateField (fldP, false, true);
		}
}


/***********************************************************************
 *
 * FUNCTION:    FldGetSelection
 *
 * DESCRIPTION: This routine returns the current selection of a field.
 *
 * PARAMETERS:  fldP      pointer to a FieldType structure.
 *
 * RETURNED:    startPos starting character position of character range
 *              endPos   ending character posiiton of character range,
 *                       range is exclusive of ending position.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *		art	12/9/94	Initial Revision
 *		art	??/??/97	return ins point if no selection
 *
 ***********************************************************************/
void FldGetSelection (const FieldType * fldP, UInt16 * startPosition, UInt16 * endPosition)
{
	ECValidateField(fldP, false, false);
	ErrNonFatalDisplayIf((startPosition == NULL) || (endPosition == NULL),
								"Null parameters");

	if (IsSelection (fldP))
		{
		*startPosition = fldP->selFirstPos;
		*endPosition = fldP->selLastPos;
		}
	else
		{
		*endPosition = *startPosition = GetStringPosOfInsPtPos (fldP);
		}
}


/***********************************************************************
 *
 * FUNCTION:    FldSetSelection
 *
 * DESCRIPTION: Set the current selection of a field and highlight the 
 *              selection if the field is visible.  If the startPosition
 *              equals the endPosition, the current selection is
 *              unhighlighted.
 *
 * PARAMETERS:  fldP     		pointer to a FieldType structure.
 *              startPosition starting character position of character range
 *              endPosition   ending character posiiton of character range,
 *                       		range is exclusive of ending position.
 *
 * RETURNED:    nothing.
 *
 * HISTORY:
 *		12/09/94	art	Created by Art Lamb.
 *		12/10/98	kwk	Constrain start/end to valid values.
 *		09/12/99	kwk	Kill deferred conversion if we're tossing its field.
 *		04/08/00	kwk	Pass false for update param to TextServicesReset.
 *		09/20/00	kwk	Undo previous change.
 *
 ***********************************************************************/
void FldSetSelection (FieldType * fldP, UInt16 startPosition, 	UInt16 endPosition)
{
	FontID curFont;
	
	ECValidateField(fldP, false, false);
	ErrNonFatalDisplayIf(startPosition > fldP->textLen || endPosition > fldP->textLen, 
		"Bad field selection");
	
	// 981210 kwk constrain the selection range, so that in the release
	// version of the rom it can't be set improperly.
	
	if (startPosition > fldP->textLen)
		startPosition = fldP->textLen;
	
	if (endPosition > fldP->textLen)
		endPosition = fldP->textLen;
	
	curFont = FntSetFont (fldP->fontID);

	// DOLATER kwk - should we do ErrNonFatalDisplay if the passed positions need to
	// be adjusted?
	
	startPosition = GetCharPos (fldP, startPosition);
	endPosition = GetCharPos (fldP, endPosition);

	// If we're setting the selection on the field that contains the current
	// inline session, we want to terminate it since otherwise the FEP's
	// selection state gets out of sync w/the field selection state.
	CheckDeferredTerm(fldP);
	
	if (fldP->attr.visible)
		{

		if (fldP->attr.hasFocus)
			{
			TextServicesReset (fldP, true);
			InsPtEnable (false);
			}

		CancelSelection (fldP);
			
		SetSelection (fldP, startPosition, endPosition);
		if (fldP->attr.hasFocus && startPosition == endPosition)
			InsPtEnable (true);
		else if (MenuCmdBarCurrent)
			{
			// if the selection has changed from an insertion point to a selection range
			// while the command bar is up, the command bar's saved insertion point state
			// must also be cleared to prevent both an insertion point and selection range
			// from both being visible when the command bar goes away.
			MenuCmdBarCurrent->insPtWasEnabled = false;
			}
		}
	else
		{
		fldP->selFirstPos = startPosition;
		fldP->selLastPos = endPosition;
		}

	UndoReset ();

	FntSetFont (curFont);

	ECValidateField (fldP, false, true);
}


/***********************************************************************
 *
 * FUNCTION:    FldGetBounds
 *
 * DESCRIPTION: Return the current visual bounds of a field.
 *
 * PARAMETERS:  fldP         pointer to a FieldType structure.
 *
 * RETURNED:    rect        bounds of the field.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/9/94	Initial Revision
 *
 ***********************************************************************/
void FldGetBounds (const FieldType * fldP, RectangleType * rect)
{
	ECNullFieldPtr(fldP);

	RctCopyRectangle(&fldP->rect, rect);
}


/***********************************************************************
 *
 * FUNCTION:    FldSetBounds
 *
 * DESCRIPTION: This routine changes the bounds of a field and if the
 *              field is visible, redraw the field.  
 *
 * PARAMETERS:  fldP         pointer to a FieldType structure.
 *              rect        new bounds of the field
 *
 * RETURNED:    nothing.
 *
 * KNOWN PROBLEMS:
 *		This routine does support changed the width of a visible field.
 *    The insertion point is assumed to be off when this routine is 
 *    called.
 *
 * HISTORY:
 *		12/09/94	art	Initial Revision
 *		08/07/97	trev	made non modified passed variabls constant
 *		09/14/97	kek	Force newNumLines to be at least 1.
 *		03/17/99	grant	make sure the insPtYPos is valid before doing
 *							SetBlinkingInsPtPosition()
 *
 ***********************************************************************/
void FldSetBounds (FieldType * fldP, const RectangleType * rect)
{
	FontID			curFont;
	LineInfoType *		line;
	MemHandle 			theHandle;
	UInt16				error;
	UInt16				start;
	UInt16				lineNum;
	UInt16				numLines;
	UInt16				newNumLines;

	ECValidateField(fldP, false, false);
	ErrNonFatalDisplayIf(rect == NULL, "NULL parameter");
	
	curFont = FntSetFont (fldP->fontID);

	numLines = GetVisibleLines (fldP);
	newNumLines = (rect->extent.y / FntLineHeight());

	// DOLATER kwk - what should happen here if newNumLines == 0? I constrain it
	// to 1, but would that cause a problem if GetVisibleLines returns 0
	// after we're done setting up the bounding rectangle?
	if (newNumLines == 0)
		newNumLines = 1;

	// If the height of the field is being changed, resize the block that 
	// contains the line word wrapping info.
	if (fldP->lines && fldP->rect.extent.y != rect->extent.y)
		{
		if (newNumLines != numLines)
			{
			theHandle = MemPtrRecoverHandle (fldP->lines);
			MemHandleUnlock (theHandle);
			error = MemHandleResize (theHandle, newNumLines * sizeof(LineInfoType));
			ErrFatalDisplayIf( error, "Memory reallocation error");
			fldP->lines = MemHandleLock (theHandle);
			}
		}


	// Set the new bounds of the field.
	RctCopyRectangle (rect, &fldP->rect);


	// UInt16 wrap and draw the new lines.
	if (fldP->lines && newNumLines > numLines)
		{
		line = &fldP->lines[numLines];
		start = fldP->lines[numLines-1].start +
		        fldP->lines[numLines-1].length;
		for (lineNum = numLines; lineNum < newNumLines; lineNum ++)
			{
			line->start = start;
			line->length = FormatLine (fldP, line->start);
			start += line->length;
			if (fldP->attr.visible)
				DrawOneLineRange (fldP, lineNum, line->start, start, true, false);
			line++;
			}
		}

	if (fldP->attr.hasFocus && fldP->attr.insPtVisible)
		{
		// make sure the insPtYPos is valid
		if (fldP->insPtYPos >= newNumLines)
			{
			fldP->insPtYPos = newNumLines - 1;
			}
		
		SetBlinkingInsPtLocation (fldP);
		}

	FntSetFont (curFont);
	
	ECValidateField(fldP, false, true);
}


/***********************************************************************
 *
 * FUNCTION:    FldSetText
 *
 * DESCRIPTION: This routine sets the text value of the field,  updates
 *              the word wrapping infomation and places the insertion point
 *              after the last visible character.
 *
 *              The pointer pass is stored in the field's structure, 
 *              in other words this routine does not make a copy of
 *              the string passed.
 *
 * PARAMETERS:  fldP        pointer to a FieldType structure.
 *              textHandle  handle of a block that contain a null-terminated
 *                          text string.
 *              offset      offset from the start of the block to the start of 
 *                          the text string.
 *              size        allocated size of the text string, ** NOT ** the
 *                          string length.
 *
 * RETURNED:    nothing.
 *
 * NOTE:
 *		!!! This routine does NOT free the memory block that holds the
 *		current text value. 
 *
 * HISTORY:
 *		12/09/94	art	Initial Revision
 *		12/08/97	roger	Added call to UndoReset.
 *		04/19/99	grant	Make sure text actually fits within textBlockSize
 *		09/15/99	kwk	Added ECValidateField call to catch problems with
 *							handle overlock earlier.
 *		09/16/99 jmp	The ECValidateField lock-check was TOO aggressive;
 *							all of the NoteView fields are locked, on purpose,
 *							by the time this call is made, and they caused
 *							ECValidateField to always complain on entry, which
 *							virtually kills Gremlins.
 *		04/08/00	kwk	Pass false for update param to TextServicesReset.
 *
 ***********************************************************************/
void FldSetText (FieldType * fldP, MemHandle textHandle, UInt16 offset, UInt16 size)
{
	UInt16 pos;
	FontID curFont;
	
	ECNullFieldPtr(fldP);

	// Validate the pass parameters.
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL

	if (textHandle)
		{
		if (MemHandleSize(textHandle) < offset)
			ErrDisplay ("Offset passed out of block");

		if (MemHandleSize(textHandle) < offset + size)
			ErrDisplay ("Invalid size");
		}

#endif


	curFont = FntSetFont (fldP->fontID);

	fldP->attr.dirty = false;
	
	// If we've got the focus, then assume the caller is directly messing with
	// the text that's part of the inline session, so we have to terminate
	if (fldP->attr.hasFocus)
		{
		TextServicesReset (fldP, false);
		}
	else
		{
		CheckDeferredTerm(fldP);
		}

	// Unlock the handle that contian the existing text string.
	if (fldP->textHandle)
		MemHandleUnlock (fldP->textHandle);

	if (textHandle)
		{
		fldP->textHandle = textHandle;
		fldP->text = (Char *) MemHandleLock (textHandle) + offset;
		fldP->textBlockSize = size;

		if (size)
			{
			fldP->textLen = StrLen (fldP->text);
			ErrNonFatalDisplayIf(fldP->textBlockSize <= fldP->textLen,
				"Text block size smaller than text");
			ErrNonFatalDisplayIf(fldP->attr.editable && (fldP->textLen > fldP->maxChars),
				"FldSetText - text longer than maxChars");
			}
		else
			fldP->textLen = 0;

		// Allocate the structure that contains the word wrapping info.
		AllocateLines (fldP);
		
		// Preform the word wrapping calculations, but don't redraw the field
		FormatAndDrawFromPos (fldP, 0, false);
		}

	else
		{
		fldP->text = NULL;
		fldP->textHandle = 0;
		fldP->textLen = 0;
		fldP->textBlockSize = 0;	
		if (fldP->lines)
			{
			MemPtrFree (fldP->lines);
			fldP->lines = NULL;
			}	
		}


	// When the text value of a field is set, the insertion point is 
	// positioned after the last character.  If that character position is
	// visible, will set the position of the "blink" insertion point,
	// otherwise the position of the insertion point is save in the 
	// selFirstPos member of the field structure.
	pos = fldP->textLen;

	fldP->selFirstPos = pos;
	fldP->selLastPos = pos;

	SetInsPt (fldP, false, pos);
		
	if (fldP->attr.hasFocus)
		UndoReset();
	
	FntSetFont (curFont);
	
	// FldSetText() is documented that the lock count for textHandle will
	// be incremented.  So, we don't want to check the lock count here
	// because it will always be at least one.  (Otherwise, if we were
	// to check the lock count here, textHandle would always have to be
	// unlocked BEFORE calling FldSetText(), which is not documented
	// behavior.)
	ECValidateField(fldP, false, false);
}

/***********************************************************************
 *
 * FUNCTION:    FldGetTextHandle
 *
 * DESCRIPTION: Return a handle to the block that contains the text 
 *              string of a field.
 *
 * PARAMETERS:  fldP         pointer to a FieldType structure.
 *
 * RETURNED:    Handle of the text string of a field; NULL is a 
 *              possible value.
 *
 * HISTORY:
 *		05/08/95	art	Initial Revision
 *
 ***********************************************************************/
MemHandle FldGetTextHandle (const FieldType * fldP)
{
	ECValidateField (fldP, true, false);

	return (fldP->textHandle);
}


/***********************************************************************
 *
 * FUNCTION:    FldSetTextHandle
 *
 * DESCRIPTION: Set the handle of the block that contains the text 
 *              string of a field.
 *
 * PARAMETERS:  fldP         pointer to a FieldType structure.
 *              textHandle  handle of the text string of a field, 0 is a 
 *              				 possible value.
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/13/95	Initial Revision
 *			trev	08/07/97	made non modified passed variabls constant
 *
 ***********************************************************************/
void FldSetTextHandle (FieldType * fldP, MemHandle textHandle)
{
	UInt16 blockSize;
	
	ECNullFieldPtr(fldP);

	if (textHandle)
		blockSize = MemHandleSize (textHandle);
	else
		blockSize = 0;

	FldSetText (fldP, textHandle, 0, blockSize);
}


/***********************************************************************
 *
 * FUNCTION:    FldGetTextPtr
 *
 * DESCRIPTION: Return a pointer to the text string of a field.
 *
 * PARAMETERS:  fldP         pointer to a FieldType structure.
 *
 * RETURNED:    pointer to the text string of a field, NULL is a 
 *              possible value.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/ 9/94	Initial Revision
 *			trev	08/07/97	made non modified passed variabls constant
 *
 ***********************************************************************/
Char * FldGetTextPtr (const FieldType * fldP)
{
	ECValidateField (fldP, true, false);

	return (fldP->text);
}


/***********************************************************************
 *
 * FUNCTION:    FldSetTextPtr
 *
 * DESCRIPTION: Set the field to point to a text string.  Since the field
 *              cannot resize a pointer, only handles can be resize, the
 *              field must be not editable.
 *
 *	NOTE:			If setting a multi-line field, the call should be followed
 *					by a call to FldRecalculateField().
 *
 * PARAMETERS:  fldP      pointer to a FieldType structure.
 *              textPtr  pointer to a null-terminated string.
 *
 * RETURNED:    nothing
 *
 * HISTORY:
 *		06/13/95	art	Created by Art Lamb.
 *		10/31/96	vmk	Took out error check for !fldP->attr.singleLine
 *							in agreement with Art.
 *		04/19/99	grant	add check for fld->textHandle != 0 - indicates mixed
 *							use of handles and pointers for field text.
 *		07/19/00	kwk	Don't call StrLen with null pointer.
 *		11/06/00	kwk	Removed called to ECValidateField, as there might not
 *							be a line starts array w/a multi-line field.
 *		11/07/00	kwk	Force line starts to get set up, so validation calls work.
 *							Also reset insertion point to start of text.
 *		11/08/00	LFe	Init all the word wrap info to 0
 *		11/11/00	kwk	Before calling GetVisibleLines, make sure font is set.
 *
 ***********************************************************************/
void FldSetTextPtr(FieldType* fldP, Char* textP)
{
	UInt16	i;
	UInt16	numLines;
	
	ECNullFieldPtr(fldP);

	ErrNonFatalDisplayIf(fldP->attr.editable, "Field cannot be editable");
	ErrNonFatalDisplayIf(fldP->textHandle != NULL,
		"Field is using a handle - setting the pointer is invalid");

	fldP->text = textP;
	fldP->textLen = (textP == NULL ? 0 : StrLen (textP));

	ResetInsPt(fldP);
	
	// Do minimal setup for multi-line field, so that it's got a
	// valid line starts array.
	if (!fldP->attr.singleLine)
	{
		FontID curFont = FntSetFont(fldP->fontID);
		
		AllocateLines(fldP);
		
		fldP->lines[0].start = 0;
		fldP->lines[0].length = fldP->textLen;

		numLines = GetVisibleLines(fldP);
		
		for (i = 1; i < numLines; i++)
		{
			fldP->lines[i].start = fldP->textLen;
			fldP->lines[i].length = 0;
		}
		
		FntSetFont(curFont);
	}
	
	ECValidateField(fldP, false, false);
}


/***********************************************************************
 *
 * FUNCTION:    FldGetTextLength
 *
 * DESCRIPTION: Returns the length of the text string of a field.
 *
 * PARAMETERS:  fldP -  pointer to a FieldType structure.
 *
 * RETURNED:    length to a field's text string
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/30/95	Initial Revision
 *			trev	08/08/97	made non modified passed variabls constant
 *
 ***********************************************************************/
UInt16 FldGetTextLength (const FieldType * fldP)
{
	ECNullFieldPtr(fldP);

	return (fldP->textLen);
}


/***********************************************************************
 *
 * FUNCTION:    FldGetFont
 *
 * DESCRIPTION: This routine returns the id of the font used to draw the 
 *              text of a field.
 *
 * PARAMETERS:  fldP      pointer to a FieldType structure.
 *              fontId   id of new font
 *
 * RETURNED:    nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/23/95	Initial Revision
 *			trev	08/07/97	made non modified passed variabls constant
 *
 ***********************************************************************/
FontID FldGetFont(const FieldType* fldP)
{
	ECNullFieldPtr(fldP);

	return(fldP->fontID);
}


/***********************************************************************
 *
 * FUNCTION:    FldSetFont
 *
 * DESCRIPTION: This routine set the font of a the field, updates
 *              the word wrapping infomation and draws the field if the 
 *              field is visible.
 *
 * PARAMETERS:  fldP      pointer to a FieldType structure.
 *              fontId   id of new font
 *
 * RETURNED:    nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/23/95	Initial Revision
 *			roger	11/11/98	Fixed dereference of fldP->lines when it's NULL.
 *
 ***********************************************************************/
void FldSetFont (FieldType * fldP, FontID fontID)
{
	UInt16 pos;
	UInt16 numLines;
	UInt16 insPtPos;
	FontID curFont;
	Boolean insPtOn;

	ECValidateField(fldP, false, false);

	if (fldP->fontID == fontID)
		return;

	curFont = FntSetFont (fldP->fontID );
	if ((insPtOn = InsPtInField (fldP)) == true)
		InsPtEnable (false);

	insPtPos = GetStringPosOfInsPtPos (fldP);		

	fldP->fontID = fontID;
	FntSetFont (fontID);
	
	if (fldP->attr.visible)
		{
		WinEraseRectangle (&fldP->rect, 0);
		}
		

	// Get the position of the first visible character.
	if (fldP->text && fldP->lines)
		pos = StartOfLine (fldP, fldP->lines->start);
	else
		pos = 0;
	

	// Reallocate the structure that holds the word wrapping info because
	// the number of visible lines may have changed.
	if (fldP->lines)
		{
		MemPtrFree (fldP->lines);
		fldP->lines = NULL;
		AllocateLines (fldP);
		}


	// Preform the word wrapping calculations
	FormatAndDrawFromPos (fldP, pos, false);
		

	// Remove any blank lines from the bottom of the display.
	if (fldP->textLen && fldP->lines)
		{
		numLines = GetVisibleLines (fldP);
		while (fldP->lines->start)
			{
			if (fldP->lines[numLines-1].length)
				break;
				
			if (fldP->text[fldP->textLen-1] == linefeedChr)
				{
				if (numLines < 2) 
					break;
				if (fldP->lines[numLines-2].length)
					break;
				}
			ScrollDisplayDown (fldP);
			}
		}


	// Redraw the field if it's already visible.
	if (fldP->attr.visible)
		DrawAllLines (fldP);	


	// Highlight the current selection.
	if ( fldP->attr.visible && IsSelection (fldP))
		DrawMultiLineRange (fldP, fldP->selFirstPos, fldP->selLastPos, true);


	// If the field has the focus then change the height of the insertion
	// point.
	if (fldP->attr.hasFocus)
		InsPtSetHeight (FntLineHeight());


	// If the insertion point was on and its position is still visible, 
	// turn it back on.  If the insertion point was not on and its 
	// position was become  visible, turn it on. 
	UpdateInsPt (fldP, insPtOn, insPtPos);

	FntSetFont (curFont);
}


/***********************************************************************
 *
 * FUNCTION:    FldSetUsable
 *
 * DESCRIPTION: This routine set a field usable or not usable.  
 *              Non-usable will not display or accept input.
 *
 * PARAMETERS:  fldP      pointer to a FieldType structure.
 *              usable   true to set usable, false to set not usable
 *
 * RETURNED:    nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/23/95	Initial Revision
 *
 ***********************************************************************/
void FldSetUsable (FieldType * fldP, Boolean usable)
{
	ECNullFieldPtr(fldP);

	fldP->attr.usable = usable;
}


/***********************************************************************
 *
 * FUNCTION:    FldFreeMemory
 *
 * DESCRIPTION: This routine releases the memory blocks that were allocated
 *              th hold the text of a field, and the word wrapping 
 *              information.
 *
 * PARAMETERS:
 *		fldP	 ->	pointer to a FieldType structure.
 *
 * RETURNED:    nothing
 *
 *	HISTORY:
 *		12/12/94	art	Created by Art Lamb.
 *		12/06/98	kwk	Set insPtVisible true here.
 *		12/07/98	kwk	Only reset textLen if we toss the text handle.
 *		08/31/99	bob	Also set selection first/last to 0.
 *		09/12/99	kwk	Kill deferred conversion if we're tossing its field.
 *
 ***********************************************************************/
void FldFreeMemory(FieldType* fldP)
{
	ECValidateField(fldP, false, false);

	if (fldP->attr.hasFocus)
		{
		TextServicesReset(fldP, false);
		}
	else
		{
		CheckDeferredTerm(fldP);
		}
	
	if (fldP->textHandle)
		{
		ErrFatalDisplayIf (MemHandleDataStorage (fldP->textHandle),
			"Deleting dm handle");
		
		MemHandleFree (fldP->textHandle);
		fldP->text = NULL;
		fldP->textHandle = 0;
		fldP->textLen = 0;
		fldP->textBlockSize = 0;
		}

	// DOLATER kwk - Seems like we should also set fldP->text to NULL, but that might
	// cause problems w/apps that rely on the current behavior.

	if (fldP->lines)
		{
		MemPtrFree (fldP->lines);
		fldP->lines = NULL;
		}
	
	ResetInsPt(fldP);
} // FldFreeMemory


/***********************************************************************
 *
 * FUNCTION:    FldGetInsPtPosition
 *
 * DESCRIPTION: Return the string position of the insertion point
 *
 * PARAMETERS:	fldP  pointer to a FieldType structure.
 *
 * RETURNED:	character position of insertion point
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/6/95	Initial Revision
 *			trev	08/07/97	made non modified passed variabls constant
 *
 ***********************************************************************/
UInt16 FldGetInsPtPosition (const FieldType * fldP)
{
	ECValidateField(fldP, false, false);

	return (GetStringPosOfInsPtPos (fldP));
}


/***********************************************************************
 *
 * FUNCTION:    FldSetInsPtPosition
 *
 * DESCRIPTION: Given a string position this routine sets the 
 *              location of the insertion point.
 *
 * PARAMETERS:	fldP  pointer to a FieldType structure.
 *
 * RETURNED:	character position of insertion point
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/6/95	Initial Revision
 *			trev	08/07/97	made non modified passed variabls constant
 *
 ***********************************************************************/
void FldSetInsPtPosition (FieldType * fldP, UInt16 pos)
{
	FontID curFont;
	Boolean insPtOn;

	ECValidateField(fldP, false, false);

	curFont = FntSetFont (fldP->fontID);

	if ((insPtOn = InsPtInField (fldP)) == true)
		InsPtEnable (false);

	pos = GetCharPos (fldP, pos);

	SetInsPtFromStringPos (fldP, pos);
	
	if (fldP->attr.hasFocus)
		SetBlinkingInsPtLocation (fldP);

	if (insPtOn)
	  	InsPtEnable (true);

	FntSetFont (curFont);

	ECValidateField (fldP, false, true);
}


/***********************************************************************
 *
 * FUNCTION:    FldSetInsertionPoint
 *
 * DESCRIPTION: Given a string position this routine sets the 
 *              location of the insertion point.  This routine differs 
 *              from FldSetIntPtPosition in that it does not make the 
 *              character position visible.
 *
 * PARAMETERS:	 fldP  pointer to a FieldType structure.
 *              pos  character position in the text of the field
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/19/96	Initial Revision
 *
 ***********************************************************************/
void FldSetInsertionPoint (FieldType * fldP, UInt16 pos)
{
	FontID curFont;
	Boolean insPtOn;

	ECValidateField(fldP, false, false);

	curFont = FntSetFont (fldP->fontID);

	if ((insPtOn = InsPtInField (fldP)) == true)
		InsPtEnable (false);

	CancelSelection (fldP);

	pos = GetCharPos (fldP, pos);

	SetInsPt (fldP, insPtOn, pos);

	FntSetFont (curFont);

	ECValidateField (fldP, false, true);
}


/***********************************************************************
 *
 * FUNCTION:    FldGetScrollPosition
 *
 * DESCRIPTION: Return the string position of the first character in the 
 *              first line of a field.
 *
 * PARAMETERS:	fldP  pointer to a FieldType structure.
 *
 * RETURNED:	character position of first visible character
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/21/95	Initial Revision
 *			trev	08/07/97	made non modified passed variabls constant
 *
 ***********************************************************************/
UInt16 FldGetScrollPosition (const FieldType * fldP)
{
	ECValidateField(fldP, false, false);

	if ((fldP->attr.singleLine) || (!fldP->lines))
		return (0);

	return (fldP->lines[0].start);
}


/***********************************************************************
 *
 * FUNCTION:    FldSetScrollPosition
 *
 * DESCRIPTION: Return the string position of the first character in the 
 *              first line of a field.
 *
 * PARAMETERS:	fldP  pointer to a FieldType structure.
 *
 * RETURNED:	character position of first visible character
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/21/95	Initial Revision
 *
 ***********************************************************************/
void FldSetScrollPosition (FieldType * fldP, UInt16 pos)
{
	UInt16			numLines;
	UInt16			insPtPos;
	FontID		curFont;
	Boolean		insPtOn;
	LineInfoType *	line;

	ECValidateField(fldP, false, false);

	if ((fldP->attr.singleLine) || (!fldP->lines))
		return;

	curFont = FntSetFont (fldP->fontID);

	pos = GetCharPos (fldP, pos);

	insPtPos = GetStringPosOfInsPtPos (fldP);

	if ((insPtOn = InsPtInField (fldP)) == true)
		InsPtEnable (false);

	// UInt16 wrap the lines and if we're setting the scroll position 
	// to the top of the field, draw the field.
	pos = min (pos, fldP->textLen);
	FormatAndDrawFromPos (fldP, pos, ((pos == 0) && (fldP->attr.visible)));

	// Make sure we're showing a full screen of text.
	numLines = GetVisibleLines (fldP);
	if ((pos > 0) && (numLines > 1))
		{
		while (fldP->lines->start)
			{	
			// If the last line is not empty, we're done.
			if (fldP->lines[numLines-1].length)
				break;

			// If the second to the last line is not empty and the line
			// end with a linefeedChr, we're done.
			if ( (fldP->lines[numLines-2].length) &&
				  (fldP->text[fldP->textLen-1] == linefeedChr) )
				break;
							
			// Move the display line information donw one line.
			MemMove (&fldP->lines[1], fldP->lines, 
				sizeof(LineInfoType) * (numLines-1));
	
			// Format the line which scrolled into view.
			line = fldP->lines;
			line->start = StartOfPreviousLine (fldP, line->start);
			line->length = FormatLine (fldP, line->start);
			}
		}

	if (fldP->attr.visible)
		{
		if (pos > 0)
			DrawAllLines (fldP);
	
		if (IsSelection (fldP))
			UpdateHighlight (fldP, fldP->selFirstPos, fldP->selLastPos);
		}

	// If the insertion point was on and its position is still visible, 
	// turn it back on.  If the insertion point was not on and its 
	// position was become  visible, turn it on. 
	UpdateInsPt (fldP, insPtOn, insPtPos);

	FntSetFont (curFont);

	ECValidateField (fldP, false, true);
}


/***********************************************************************
 *
 * FUNCTION:    FldScrollField
 *
 * DESCRIPTION: This routine scrolls a field up or down by the number
 *              of lines specified.
 *
 * PARAMETERS:	fldP           pointer to a FieldType structure.
 *             linesToScroll number of lines to scroll.
 *             direction     up or down
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/30/95	Initial Revision
 *			art	5/23/96	Optimized scrolling up
 *
 ***********************************************************************/
void FldScrollField (FieldType * fldP, UInt16 linesToScroll, WinDirectionType direction)
{
	UInt16			pos;
	UInt16			topPos;
	UInt16			insPtPos;
	UInt16			numLines;
	FontID		curFont;
	Boolean		insPtOn;
	LineInfoType *	line;

	ECValidateField(fldP, false, false);

	if ((fldP->attr.singleLine) || (!fldP->text)) return;

	curFont = FntSetFont (fldP->fontID);

	if ((insPtOn = InsPtInField (fldP)) == true)
		InsPtEnable (false);

		insPtPos = GetStringPosOfInsPtPos (fldP);		
		
	if (linesToScroll == 1)
		{
		if (direction == winUp)
			{
			ScrollDisplayDown (fldP);
			if (IsSelection (fldP))
				InvertSelectionInLine (fldP, 0);
			}
		else if (direction == winDown)
			{
			ScrollDisplayUp (fldP);
			if (IsSelection (fldP))
				InvertSelectionInLine (fldP, GetVisibleLines (fldP) - 1);
			}
		}

	// Scroll the field by more than one line.
	else
		{
		topPos = fldP->lines->start;
		numLines = GetVisibleLines (fldP);

		if (direction == winUp)
			{
			ScrollUpNLines (fldP, linesToScroll);
			}

		else if (direction == winDown)
			{
			while (linesToScroll--)
				{
				// If the end of the text is visible don't scroll anymore.
				line = &fldP->lines[numLines-1];
				pos = line->start + line->length;
				if ( (pos < fldP->textLen) || 
					  (line->length && fldP->text[pos-1] == linefeedChr) )
					{
					// Move the display line information up one line.
					MemMove (fldP->lines, &fldP->lines[1], 
						sizeof(LineInfoType) * (numLines-1));
		
					// Format the line which scrolled into view.
					line->start = line->start + line->length;
					line->length = FormatLine (fldP, line->start);
					}
				}

			// Redraw the field if it was scrolled.
			if (topPos != fldP->lines->start)
				DrawAllLines (fldP);
			}

		// if ( (topPos != fldP->lines->start) && (IsSelection (fldP)))
		//		UpdateHighlight (fldP, fldP->selFirstPos, fldP->selLastPos);
		}

	// If the insertion point was on and its position is still visible, 
	// turn it back on.  If the insertion point was not on and its 
	// position was become  visible, turn it on. 
	UpdateInsPt (fldP, insPtOn, insPtPos);

	FntSetFont (curFont);

	ECValidateField (fldP, false, true);
}


/***********************************************************************
 *
 * FUNCTION:    FldScrollable
 *
 * DESCRIPTION: This routine returns true if the field is scrollable
 *              in the direction specified.
 *
 * PARAMETERS:	fldP           pointer to a FieldType structure.
 *             direction     up or down
 *
 * RETURNED:	true if the field is scrollable
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/30/95	Initial Revision
 *			trev	08/08/97	made non modified passed variabls constant
 *
 ***********************************************************************/
Boolean FldScrollable (const FieldType * fldP, WinDirectionType direction)
{
	UInt16 pos;
	UInt16 numLines;
	FontID curFont;
	LineInfoType * line;

	ECValidateField(fldP, false, false);

	if (fldP->attr.singleLine)
		return (false);
	
	if (! fldP->lines)
		return (false);

	if (direction == winUp)
		return (fldP->lines->start > 0);

	else if (direction == winDown)
		{
		curFont = FntSetFont (fldP->fontID);
		numLines = GetVisibleLines (fldP);
		FntSetFont (curFont);
		line = &fldP->lines[numLines-1];
		pos = line->start + line->length;
		if (pos < fldP->textLen)
			return (true);
			 
		return (line->length && (fldP->text[pos-1] == linefeedChr));
		}

	return (false);
}


/***********************************************************************
 *
 * FUNCTION:    FldGetScrollValues
 *
 * DESCRIPTION: This routine returns the values necessary to update a 
 *              scroll bar.
 *
 * PARAMETERS:  fldP - pointer to a FieldType structure.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/29/96	Initial Revision
 *
 ***********************************************************************/
void FldGetScrollValues (const FieldType * fldP, UInt16 * scrollPosP, 
	UInt16 * textHeightP, UInt16 * fieldHeightP)
{
	UInt16 firstVisible;
	UInt16 maxWidth;
	Char * chars;
	FontID curFont;

	ECValidateField(fldP, false, false);

	ErrFatalDisplayIf (fldP->attr.singleLine, "Not scrollable");


	curFont = FntSetFont (fldP->fontID);
	
	*fieldHeightP = GetVisibleLines (fldP);

	if (! fldP->text)
		{
		*scrollPosP = 0;
		*textHeightP = 0;
		}

	else
		{
		chars = fldP->text;
		firstVisible = fldP->lines[0].start;
		maxWidth = fldP->rect.extent.x;

		FntGetScrollValues (chars, maxWidth, firstVisible, textHeightP, scrollPosP);
		}

	FntSetFont (curFont);	
}


/***********************************************************************
 *
 * FUNCTION:    FldGetVisibleLines
 *
 * DESCRIPTION: This routine returns the number of lines that can be 
 *              display within the visible bounds of the field. 
 *
 * PARAMETERS:	fldP           pointer to a FieldType structure.
 *             direction     up or down
 *
 * RETURNED:	number if lines
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/30/95	Initial Revision
 *			trev	08/08/97	made non modified passed variabls constant
 *
 ***********************************************************************/
UInt16 FldGetVisibleLines (const FieldType * fldP)
{
	UInt16 numLines;
	FontID curFont;
	
	ECNullFieldPtr(fldP);

	curFont = FntSetFont (fldP->fontID);
	numLines = fldP->rect.extent.y / FntLineHeight();
	FntSetFont (curFont);
	return (numLines);
}


/***********************************************************************
 *
 * FUNCTION:    FldGetTextHeight
 *
 * DESCRIPTION: This routine returns the height of lines of text that
 *              the specified field has, empty lines are not counted.
 *
 * PARAMETERS:  fldP -  pointer to a FieldType structure.
 *
 * RETURNED:    number of lines of with text.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/10/95	Initial Revision
 *			trev	08/08/97	made non modified passed variabls constant
 *			roger	5/11/99	Move FntSetFont before GetVisibleLines
 *
 ***********************************************************************/
UInt16 FldGetTextHeight (const FieldType * fldP)
{
	UInt16 height = 1;
	UInt16 numLines;
	FontID curFont;

	ECValidateField(fldP, false, false);

	curFont = FntSetFont (fldP->fontID);

	if ((! fldP->attr.singleLine) && (fldP->text) && (fldP->textLen > 0))
		{
		height = 0;
		numLines = GetVisibleLines (fldP);
		while  (height < numLines)
			{
			if (fldP->lines[height].length == 0)
				break;
			height++;
			}
		}
	
	height *= FntLineHeight ();
	FntSetFont (curFont);
	
	return (height);
}


/***********************************************************************
 *
 * FUNCTION:    FldCalcFieldHeight
 *
 * DESCRIPTION: Given a string, determine the height of a field.
 *
 * PARAMETERS:	 chars       pointer to a null-terminated string
 *              maxWidth    maximum line width in pixels
 *
 * RETURNED:	 number of line needed to draw the string passed
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/26/95	Initial Revision
 *			trev	08/08/97	made non modified passed variabls constant
 *
 ***********************************************************************/
UInt16 FldCalcFieldHeight (const Char* chars, UInt16 maxWidth)
{
	UInt16 length = 0;
	UInt16 height = 0;
	
	if (!chars) return 1;

	do {
		length += FntWordWrap (&chars[length], maxWidth);
		height++;
		}
	while (chars[length]);

	// If the text end with a linefeed add one to the height.
	if (length && chars[length-1] == linefeedChr) 
		height++;

	return (height);
}


/***********************************************************************
 *
 * FUNCTION:    FldWordWrap
 *
 * DESCRIPTION: Given a string and a width, return the number of 
 *              character that can be displayed using the current 
 *              font.
 *
 * PARAMETERS:	 chars       pointer to a null-terminated string
 *              maxWidth    maximum line width in pixels
 *
 * RETURNED:	 number of characters.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/7/95	Initial Revision
 *			trev	08/08/97	made non modified passed variabls constant
 *
 ***********************************************************************/
UInt16 FldWordWrap  (const Char * chars, Int16 maxWidth)
{
	return FntWordWrap (chars, maxWidth);
}

/***********************************************************************
 *
 * FUNCTION:    FldCompactText
 *
 * DESCRIPTION: Compact the memory block that contain the text of the 
 *              field to release any unused space.
 *
 * PARAMETERS:	 fldP      pointer to a FieldType structure.
 *
 * RETURNED:	 nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/8/95	Initial Revision
 *
 ***********************************************************************/
void FldCompactText (FieldType * fldP)
{
	Err	   error;
	UInt16		offset;
	UInt16		blockSize;
	UInt16		freeSpace;

	ECValidateField(fldP, false, false);

	if (! fldP->textHandle) return;
		
	freeSpace = fldP->textBlockSize - fldP->textLen - 1;
	if (!freeSpace) return;

	// The field's text string does not necessary start at the 
	// begining of the memory block or end at the end of the 
	// memory block.  In other words the text string may not be 
	// the only thing in the block.
	//
	// If there is something stored in the block, after the string 
	// we're editting, move it such that the free space is removed.
	blockSize = MemHandleSize(fldP->textHandle);
	offset = fldP->text - (Char *) MemDeref(fldP->textHandle);

	blockSize -= freeSpace;
	MemHandleUnlock (fldP->textHandle);
	error = MemHandleResize (fldP->textHandle, blockSize);
	ErrFatalDisplayIf( error, "Memory reallocation error");
		
	fldP->text = (Char *) MemHandleLock(fldP->textHandle) + offset;
	fldP->textBlockSize -= freeSpace;
}


/***********************************************************************
 *
 * FUNCTION:    FldDirty
 *
 * DESCRIPTION: This routine returns true if the field has been 
 *              modified by the user since the text value was 
 *              set (FldSetText).
 *
 * PARAMETERS:	 fldP      pointer to a FieldType structure.
 *
 * RETURNED:	 true if the field is user modified.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/24/95	Initial Revision
 *			trev	08/08/97	made non modified passed variabls constant
 *
 ***********************************************************************/
Boolean FldDirty (const FieldType * fldP)
{
	ECNullFieldPtr(fldP);

	return (fldP->attr.dirty);
}


/***********************************************************************
 *
 * FUNCTION:    FldSetDirty
 *
 * DESCRIPTION: This routine sets whether the field has been 
 *              modified.
 *
 * PARAMETERS:	 fldP      pointer to a FieldType structure.
 *					 dirty	 true if the text is modified
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	8/28/95	Initial Revision
 *
 ***********************************************************************/
void FldSetDirty (FieldType * fldP, Boolean dirty)
{
	ECNullFieldPtr(fldP);

	fldP->attr.dirty = dirty;
}


/***********************************************************************
 *
 * FUNCTION:    FldGetMaxChars
 *
 * DESCRIPTION: This routine returns the maximum number of character 
 *              that the field will accept.
 *
 * PARAMETERS:	 fldP      pointer to a FieldType structure.
 *
 * RETURNED:	 maximum number of characters that may be input.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/13/95	Initial Revision
 *			trev	08/08/97	made non modified passed variabls constant
 *
 ***********************************************************************/
UInt16 FldGetMaxChars (const FieldType * fldP)
{
	ECNullFieldPtr(fldP);

	return (fldP->maxChars);
}


/***********************************************************************
 *
 * FUNCTION:    FldSetMaxChars
 *
 * DESCRIPTION: This routine set the maximum number of character 
 *              that the field will accept.
 *
 * PARAMETERS:	 fldP       pointer to a FieldType structure.
 *              maxChars  maximum number of bytes that may be input.
 *
 * RETURNED:	 nothing 
 *
 * HISTORY:
 *		06/13/95	art	Created by Art Lamb.
 *		10/01/98	kwk	Terminate inline if field has focus.
 *		04/08/00	kwk	Pass false for update param to TextServicesReset.
 *
 ***********************************************************************/
void FldSetMaxChars (FieldType * fldP, UInt16 maxChars)
{
	ECValidateField(fldP, false, false);

	ErrNonFatalDisplayIf(fldP->textLen > maxChars,
		"FldSetMaxChars - text longer than maxChars");

	// DOLATER kwk - should we only terminate inline if it would affect the inline
	// text area, otherwise we're ok?
	if (maxChars < fldP->maxChars)
		{
		if (fldP->attr.hasFocus)
			{
			TextServicesReset(fldP, false);
			}
		else
			{
			CheckDeferredTerm(fldP);
			}
		}
	
	fldP->maxChars = maxChars;
}


/***********************************************************************
 *
 * FUNCTION:    FldGetTextAllocatedSize
 *
 * DESCRIPTION: This routine returns the number of character 
 *              allocated to hold the field's text string,  this 
 *              value should not be confused the the lenght the of text 
 *              string.
 *
 * PARAMETERS:	 fldP      pointer to a FieldType structure.
 *
 * RETURNED:	 number of characters allocated for the field's text.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/29/95	Initial Revision
 *			trev	08/08/97	made non modified passed variabls constant
 *
 ***********************************************************************/
UInt16 FldGetTextAllocatedSize (const FieldType * fldP)
{
	ECNullFieldPtr(fldP);

	return (fldP->textBlockSize);
}


/***********************************************************************
 *
 * FUNCTION:    FldSetTextAllocatedSize
 *
 * DESCRIPTION: This routine set the number of character 
 *              allocated to hold the field's text string,  this 
 *              value shoulg not be confused the the lenght the of text 
 *              string.
 *
 * PARAMETERS:	 fldP           - pointer to a FieldType structure.
 *              allocatedSize - number of characters allocated for 
 *                              the field's text.
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/29/95	Initial Revision
 *
 ***********************************************************************/
void FldSetTextAllocatedSize (FieldType * fldP, UInt16 allocatedSize)
{
	ECNullFieldPtr(fldP);

	fldP->textBlockSize = allocatedSize;
}


/***********************************************************************
 *
 * FUNCTION:    FldGetAttributes
 *
 * DESCRIPTION: This routine returns the attributes of a field.
 *
 * PARAMETERS:	 fldP      pointer to a FieldType structure.
 *              attrP	 pointer to the attributes
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/29/95	Initial Revision
 *			trev	08/08/97	made non modified passed variabls constant
 *
 ***********************************************************************/
void FldGetAttributes (const FieldType * fldP, FieldAttrPtr attrP)
{
	ECNullFieldPtr(fldP);
	ErrNonFatalDisplayIf(attrP == NULL, "NULL parameter");

	*attrP = fldP->attr;
}


/***********************************************************************
 *
 * FUNCTION:    FldSetAttributes
 *
 * DESCRIPTION: This routine sets the attributes of a field.
 *
 * PARAMETERS:	 fldP      pointer to a FieldType structure.
 *              attrP	 pointer to the attributes
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/29/95	Initial Revision
 *			trev	08/08/97	made non modified passed variabls constant
 *
 ***********************************************************************/
void FldSetAttributes (FieldType * fldP, const FieldAttrType * attrP)
{
	ECNullFieldPtr(fldP);
	ErrNonFatalDisplayIf(attrP == NULL, "NULL parameter");
	
	fldP->attr = *attrP;

	ECValidateField (fldP, false, true);
}


/***********************************************************************
 *
 * FUNCTION:    FldSendChangeNotification
 *
 * DESCRIPTION: This routine sends a fldChangedEvent.
 *
 * PARAMETERS:	 fldP      pointer to a FieldType structure.
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/30/95	Initial Revision
 *			ron   8/21/96	Modified to use EvtAddUniqueEventToQueue so that
 *									event queue doesn't overflow when multiple FldInsert
 *									or FldDelete calls are made in a row.
 *			trev	08/08/97	made non modified passed variabls constant
 *
 ***********************************************************************/
void FldSendChangeNotification (const FieldType * fldP)
{
	EventType event;
	
	ECNullFieldPtr(fldP);

	MemSet (&event, sizeof (event), 0);
	event.eType = fldChangedEvent;
	event.data.fldChanged.fieldID = fldP->id;
	event.data.fldChanged.pField = (FieldType *)fldP;
	EvtAddUniqueEventToQueue (&event, (UInt32)fldP, false);
}


/***********************************************************************
 *
 * FUNCTION:    FldSendHeightChangeNotification
 *
 * DESCRIPTION: This routine sends a fldChangedHeightEvent.
 *
 * PARAMETERS:	 fldP  pointer to a FieldType structure.
 *              pos  character position of the insertion point
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	9/12/95	Initial Revision
 *			ron   8/21/96	Modified to use EvtAddUniqueEventToQueue so that
 *									event queue doesn't overflow when multiple FldInsert
 *									or FldDelete calls are made in a row.
 *			trev	08/08/97	made non modified passed variabls constant
 *
 ***********************************************************************/
void FldSendHeightChangeNotification (const FieldType * fldP, UInt16 pos, Int16 numLines)
{
	FontID curFont;
	EventType event;
	
	ECValidateField(fldP, false, false);

	curFont = FntSetFont (fldP->fontID);

	event.eType = fldHeightChangedEvent;
	event.data.fldHeightChanged.fieldID = fldP->id;
	event.data.fldHeightChanged.pField = (FieldType *)fldP;
	event.data.fldHeightChanged.currentPos = pos;
	event.data.fldHeightChanged.newHeight = 
		min (GetMaxLines (fldP), numLines) * FntLineHeight ();

	EvtAddUniqueEventToQueue (&event, (UInt32)fldP, false);

	FntSetFont (curFont);
}

/***********************************************************************
 *
 * FUNCTION:    FldMakeFullyVisible
 *
 * DESCRIPTION: This routine will cause a dynamicly resizable field to 
 *              expand its height in order to make its text fully
 *              visible, or will return false if the field is already
 *					 fully visible.
 *
 * PARAMETERS:	 fldP - pointer to a field object.
 *
 * RETURNED:	 true it the field was not fully visible.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	10/20/95	Initial Revision
 *			bob	11/22/00	Make use of maxVisibleLines to avoid bug 41184
 *
 ***********************************************************************/
Boolean FldMakeFullyVisible (FieldType * fldP)
{
	UInt16 pos;
	UInt16 numLines;
	FontID curFont;
	Boolean notFullyVisible = false;

	ECNullFieldPtr(fldP);

	curFont = FntSetFont (fldP->fontID);
	
	// If the field is dynanicly resizeable and some of the text is not 
	// visible, then send a height change notification.
	if (fldP->attr.dynamicSize && 
	 	(! PositionVisible (fldP, fldP->textLen)) &&
	 	(GetVisibleLines (fldP) < GetMaxLines (fldP)))
		{
		pos = GetStringPosOfInsPtPos (fldP);
		numLines = FldCalcFieldHeight (fldP->text, fldP->rect.extent.x);
		FldSendHeightChangeNotification (fldP, pos, numLines);

		notFullyVisible = true;
		}

	FntSetFont (curFont);
	return (notFullyVisible);
}


/***********************************************************************
 *
 * FUNCTION:    FldGetNumberOfBlankLines
 *
 * DESCRIPTION: This routine returns the number of blank line that
 *              are display at the end of field.  This routine is 
 *              useful for updating a scroll bar after character have
 *              been remove from the end of a field.
 *
 * PARAMETERS:	 fldP         pointer to a FieldType structure.
 *
 * RETURNED:	 number of blank lines visible.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	7/2/96	Initial Revision
 *			trev	08/08/97	made non modified passed variabls constant
 *
 ***********************************************************************/
UInt16 FldGetNumberOfBlankLines (const FieldType * fldP)
{
	UInt16			numLines;
	FontID curFont;
	UInt16			count = 0;

	ECValidateField(fldP, false, false);

	curFont = FntSetFont (fldP->fontID);
	numLines = GetVisibleLines (fldP);
	FntSetFont (curFont);

	// DOLATER kwk - shouldn't this subtract one if text length > 0,
	// since we assume it's a single-line field, right?
	if (! fldP->lines) return (numLines);
	
	while (numLines--)
		{
		if (fldP->lines[numLines].length != 0)
			{
			if (count && fldP->text[fldP->textLen-1] == linefeedChr)
				count--;
			break;
			}
		count++;
		}

	return (count);
}


/***********************************************************************
 *
 * FUNCTION:    FldNewField
 *
 * DESCRIPTION: Create a field. 
 *
 * PARAMETERS:  formPP - pointer to a pointer to a form. Set if the form moves.
 *					 id - id of the new field
 *					 x - x of the new field
 *					 y - y of the new field
 *					 width - width of the new field
 *					 height - height of the new field
 *					 font - font of the new field
 *					 maxChars - maxChars of the new field
 *					 underlined - true to display an underline.  Normally set for editing.
 *					 singleLine - true if the field is a single line tall
 *					 justification - the justification used for the text
 *					 autoShift - true to enable automatic shifting for Graffiti input
 *					 hasScrollBar - true to display a scrollbar
 *					 numeric - true to restrict input to numbers
 *
 * RETURNED:    0 if error else a pointer to the field
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/19/97	Initial Revision
 *
 ***********************************************************************/
FieldType * FldNewField (void **formPP, UInt16 id, 
	Coord x, Coord y, Coord width, Coord height, 
	FontID font, UInt32 maxChars, Boolean editable, Boolean underlined, 
	Boolean singleLine, Boolean dynamicSize, JustificationType justification, 
	Boolean autoShift, Boolean hasScrollBar, Boolean numeric)
{
	FieldType * fieldP;
	Err error;
	
	ErrNonFatalDisplayIf((formPP == NULL) || (*formPP == NULL), "NULL parameter");
	
	error = FrmAddSpaceForObject ((FormType **) formPP, (MemPtr *)&fieldP, frmFieldObj, 
					sizeof(FieldType));
	if (error)
		return 0;
	
	fieldP->id = id;
	fieldP->rect.topLeft.x = x;
	fieldP->rect.topLeft.y = y;
	fieldP->rect.extent.x = width;
	fieldP->rect.extent.y = height;
	
	fieldP->attr.usable = true;
	fieldP->attr.editable = editable;
	if (underlined)
		fieldP->attr.underlined = grayUnderline;
	fieldP->attr.singleLine = singleLine;
	fieldP->attr.dynamicSize = dynamicSize;
	fieldP->attr.autoShift = autoShift;
	fieldP->attr.hasScrollBar = hasScrollBar;
	fieldP->attr.numeric = numeric;
	fieldP->attr.justification = justification;
	fieldP->attr.insPtVisible = true;	
	fieldP->maxChars = maxChars;
	fieldP->fontID = font;
	
	// set max visible lines to zero, which triggers old behavior
	// of maxing to 121 pixels tall (11 lines of stdFont)
	fieldP->maxVisibleLines = 0;
		
	ECValidateField (fieldP, false, true);
	
	return fieldP;
}


/***********************************************************************
 *
 * FUNCTION:    FldHandleEvent
 *
 * DESCRIPTION: 
 *
 * PARAMETERS:	 fldP    pointer to a FieldType structure.
 *              eventP  pointer to an EventType structure.
 *
 * RETURNED:	 true if the event was handled by the field.
 *
 * HISTORY:
 *		11/22/94	art	Created by Art Lamb.
 *		03/27/96	art	Add autoshifting
 *		08/31/99	gap	Fix cursor/hilite handling for double-tap selection
 *		09/12/99	gap	Updated for new multi-tap implementation
 *		09/14/99	gap	Removed tapstate constants
 *		09/14/99	gap	Fix PenDown handling to once again generate FldEnterEvent
 *		10/19/99	gap	Make drag selection a little bit smarter.  Maintain 2 
 *							anchor points so initial selection is not lost when
 *							dragging left or up following a multi-tap select.
 *		08/15/00	CS	Initialize new event structure.
 *
 ***********************************************************************/
Boolean FldHandleEvent ( FieldType* fldP, EventType * eventP)
{
	WChar ch;
	UInt16 endPos;
	UInt16 startPos;
	UInt16 scrollPos;
	UInt16 charsToDelete;
	Int16 x, y;
	RectangleType r;
	Boolean handled = false;
	Boolean penDown;
	FontID curFont;
	UInt8 tapState;
	EventType newEvent;

	UInt16 firstTapPosition;
	UInt16 downScrollAnchor;
	UInt16 upScrollAnchor;


	ECValidateField (fldP, true, true);

	if (! fldP->attr.usable)
		return (false);


	if (TextServicesHandleEvent (fldP, eventP))
		return (true);
		
	if (eventP->eType == keyDownEvent)
		{
		if (! fldP->attr.editable)
			return (false);
			
		if (EvtKeydownIsVirtual(eventP))
			return (false);

		handled = true;
		InsPtEnable (false);
		curFont = FntSetFont (fldP->fontID);
		ch = eventP->data.keyDown.chr;

		if (TxtCharIsPrint (ch))
			{
			if (fldP->attr.numeric)	
				{
				if (TxtCharIsDigit (ch)  || 
						(ch == UIDecimalSeparator && 
							((!fldP->text) ||
							(fldP->text && StrChr(fldP->text, UIDecimalSeparator) == NULL))))
					InsertChar (fldP, ch, GInlineActive);
				else
					SndPlaySystemSound (sndError);
				}
			else
				{
				InsertChar (fldP, ch, GInlineActive);
				if (ch == spaceChr)
					AutoShift (fldP);
				}
			}

		else if ((ch == linefeedChr) || (ch == tabChr))
			{
			if (! (fldP->attr.singleLine || fldP->attr.justification == rightAlign))
				InsertChar (fldP, ch, GInlineActive);
			if (ch == linefeedChr)
				AutoShift (fldP);
			}
		else if (ch == backspaceChr)
			{
			BackspaceChar (fldP);
			if (EvtKeyQueueEmpty ())
				AutoShift (fldP);
			}
		else if (ch == upArrowChr)
			InsPtUp (fldP);
		else if (ch == downArrowChr)
			InsPtDown (fldP);
		else if (ch == leftArrowChr)
			InsPtLeft (fldP);
		else if (ch == rightArrowChr)
			InsPtRight (fldP);
		else
			handled = false;

		if ((fldP->attr.insPtVisible) && (! IsSelection (fldP)))
			{
			if ( ! fldP->attr.hasFocus)
				FldGrabFocus (fldP);
			else
				{
				SetBlinkingInsPtLocation (fldP);
				InsPtEnable (true);
				}
			}
		FntSetFont (curFont);
		}
		
	else if (eventP->eType == penDownEvent)
		{
		MemSet(&newEvent, sizeof(newEvent), 0);	// Initialize new event structure
		newEvent = *eventP;
		newEvent.eType = fldEnterEvent;
		newEvent.data.fldEnter.fieldID = fldP->id;
		newEvent.data.fldEnter.pField = fldP;
		EvtAddEventToQueue (&newEvent);
		}

	else if (eventP->eType == fldEnterEvent) 
		{
		// The number of "taps" has been determined for us already, by the
		// penDown event.
		tapState = eventP->tapCount;
		curFont = FntSetFont (fldP->fontID);
		CancelSelection (fldP);
		UndoReset ();

		SndPlaySystemSound (sndClick);
		
		GrfFieldChange (false, &charsToDelete);

		x = eventP->screenX;
		y = eventP->screenY;
		SetInsPtPosFromPt (fldP, x, y);
		
		// If the user has double-tapped or triple-tapped be sure that the I-beam cursor is
		// hidden before hiliting the new selection area.
		if ( tapState > 1 )
			InsPtEnable (false);

		// The initial selection needs to be calculated based on the tapState. For example,
		// an entire word needs to be selected if tapState == doubleTap. 
		//
		// firstTapPosition is the location that the user initially (multi)tapped. This position
		// may be in the middle of the current selection.  This position is used when processing a 
		// drag selection to determine the direction of the drag:  left/up or right/down.  
		//
		// downScrollAnchor is the leftmost position of the current selection.  It is used as the anchor
		// point for drag selection in a right/downward direction.
		//
		// upScrollAnchor is the rightmost position of the of the current selection. It is used as the
		// anchor point for a drag selection in the left/upward direction.
		firstTapPosition = downScrollAnchor = GetStringPosOfInsPtPos (fldP);
		CalculateSelection( fldP, &downScrollAnchor, &upScrollAnchor, tapState );
		SetSelection (fldP, downScrollAnchor, upScrollAnchor);

		if (fldP->attr.editable)
			{
			if ( ! fldP->attr.hasFocus)
				FldGrabFocus (fldP);
			else
				{
				GrfSetState (false, false, false);
				SetBlinkingInsPtLocation (fldP);
				// Only enable the I-beam cursor if a single-tap has occurred.
				if (tapState <= 1)
					InsPtEnable (true);
				}
			}

		// If the field is dynamically resizeable and some of the text is not 
		// visible, then send a height change notification.  If the field
		// is already fully visible then start drag-selecting.
		if ( ! FldMakeFullyVisible (fldP))
			{
			scrollPos = FldGetScrollPosition (fldP);
			GetBoundsOfInsPtPos (fldP, &r);
			do
				{
				if (y < fldP->rect.topLeft.y)  /* above field */
					{
					InsPtEnable (false);
					ScrollSelectionDown (fldP, upScrollAnchor);
					}
				else if (y >= fldP->rect.topLeft.y + fldP->rect.extent.y) // below field
					{
					InsPtEnable (false);
					ScrollSelectionUp (fldP, downScrollAnchor);
					}
				else if (! RctPtInRectangle (x, y, &r))
					{
					SetInsPtPosFromPt (fldP, x, y);
					GetBoundsOfInsPtPos (fldP, &r);
					InsPtEnable (false);
					
					startPos = GetStringPosOfInsPtPos(fldP);
					CalculateSelection( fldP, &startPos, &endPos, tapState );
					
					if (startPos < firstTapPosition)  			// left/up drag selection
						SetSelection (fldP, startPos, upScrollAnchor);
					else										//  down/right drag selection
						SetSelection (fldP, downScrollAnchor, endPos);
					}
				if (! IsSelection (fldP) && fldP->attr.editable)
					InsPtEnable (true);
				PenGetPoint (&x, &y, &penDown);
				} while (penDown);
	
			if (scrollPos != FldGetScrollPosition (fldP))
				FldSendChangeNotification (fldP);
			}
		
		FntSetFont (curFont);
		handled = true;
		}


	else if (eventP->eType == fldHeightChangedEvent)
		{
		// Set the insertion point.
//		FldSetInsPtPosition (fldP, eventP->data.fldHeightChanged.currentPos);

		// Turn the insertion point back on.
		if (fldP->attr.hasFocus)
			InsPtEnable (true);
		}


	else if (eventP->eType == menuCmdBarOpenEvent)
		{
		// add undo/cut/copy/paste icons (as appropriate) to the command bar
		UInt16 pasteLen;
		MemHandle pasteCharsH;
		
		if (eventP->data.menuCmdBarOpen.preventFieldButtons == 0) 
			{
			if ((fldP->attr.editable) && (UndoGlobals.mode != undoNone))
				MenuCmdBarAddButton(menuCmdBarOnRight, BarUndoBitmap, menuCmdBarResultMenuItem, sysEditMenuUndoCmd, 0);
			if (IsSelection(fldP)) 
				{
				if (fldP->attr.editable)
					MenuCmdBarAddButton(menuCmdBarOnRight, BarCutBitmap, menuCmdBarResultMenuItem, sysEditMenuCutCmd, 0);
				MenuCmdBarAddButton(menuCmdBarOnRight, BarCopyBitmap, menuCmdBarResultMenuItem, sysEditMenuCopyCmd, 0);
				}

			if (fldP->attr.editable) 
				{
				// Is there anything to paste? If so, add the button.
				pasteCharsH = ClipboardGetItem (clipboardText, &pasteLen);
				if ((pasteLen) && (pasteCharsH))
					MenuCmdBarAddButton(menuCmdBarOnRight, BarPasteBitmap, menuCmdBarResultMenuItem, sysEditMenuPasteCmd, 0);
				}
			}
		
		handled = false;  // mustn't say it is handled, otherwise the OS won't bring up the bar.
		}

	else
		handled = false;
		
	ECValidateField (fldP, true, true);

	return (handled);
}

#pragma mark ------- Text Services -------


#if (ERROR_CHECK_LEVEL == ERROR_CHECK_FULL)

/***********************************************************************
 *
 * FUNCTION:    TextServicesValidate
 *
 * DESCRIPTION: Verify that the input method status is in sync with
 *				the inline data..
 *
 * PARAMETERS:	fldP		pointer to a FieldType structure.
 *              status	pointer to an TsmStatus structure.
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			kwk	07/30/98	Initial Revision.
 *			kwk	05/19/99	Bail out immediately if no input method.
 *
 ***********************************************************************/
static void
TextServicesValidate(FieldType * /* fldP */, TsmFepStatusType * status)
{
	UInt16 fepTextLen;
	UInt16 fldTextLen;
	
	if (GTsmFepLibRefNum == sysInvalidRefNum)
		return;
	
	fepTextLen = status->convertedLen + status->pendingLen;
	fldTextLen = GInlineActive ? (GInlineEnd - GInlineStart) : 0;
	
	ErrFatalDisplayIf(fepTextLen != fldTextLen, "FEP out of sync with field");
} // TextServicesValidate

#endif	// ERROR_CHECK_LEVEL == ERROR_CHECK_FULL


/***********************************************************************
 *
 * FUNCTION:    TextServicesSyncClause
 *
 * DESCRIPTION: Update clause offset & length if we're dumping text.
 *
 * PARAMETERS:
 *		iDumpLength	 ->	Bytes of text being dumped.
 *
 * RETURNED:	 nothing
 *
 * HISTORY:
 *		09/27/00	kwk	Created by Ken Krugler.
 *
 ***********************************************************************/
static void TextServicesSyncClause(UInt16 iDumpLength)
{
	if (iDumpLength > 0)
		{
		if (iDumpLength > GInlineClauseOffset)
			{
			if (iDumpLength >= GInlineClauseOffset + GInlineClauseLen)
				{
				GInlineClauseLen = 0;
				}
			else
				{
				GInlineClauseLen -= (iDumpLength - GInlineClauseOffset);
				}
				
			GInlineClauseOffset = 0;
			}
		else
			{
			GInlineClauseOffset -= iDumpLength;
			}
		}
} // TextServicesSyncClause


/***********************************************************************
 *
 * FUNCTION:    TextServicesHandleAction
 *
 * DESCRIPTION: Handle actions returned by the Text Services Manager.
 *
 * PARAMETERS:	 fldP      - pointer to a FieldType structure.
 *              ioStatus - 
 *              ioAction - 
 *
 * RETURNED:	 nothing
 *
 * HISTORY:
 *		04/01/98	art	Initial Revision
 *		07/15/98	kwk	Only call SetBlinkingInsPtLocation if the
 *							insertion point is visible.
 *		07/15/98	kwk	Play sound if field overflows.
 *		07/30/98	kwk	Use InsertTextAndWrap() for case where there's
 *							text to insert, not FldDelete, as that resets
 *							the input method (which will fail).
 *		08/23/98	kwk	Use new .primedLength field in the action
 *							record to handle sync problems w/the FEP.
 *							Removed action param from TsmCommitAction call.
 *		10/27/98	kwk	If the updateText flag is false, we still need to adjust
 *							the value of GInlineHighlightLen if we're dumping text.
 *					kwk	If we've got converted text, make sure it's scrolled
 *							into view.
 *		08/20/99	kwk	Make sure GInlineActive is on if we're updating text
 *							and we have inline text, as otherwise DrawChars doesn't work.
 *		12/07/99	kwk	When handling the updateText case, see if UndoRecordInput
 *							returns false, and reset the FEP/bail if so. Delay setting
 *							up GInlineActive & other inline globals until after we
 *							pass this test.
 *		12/13/99	kwk	If InsertTextAndWrap returns false, then reset the FEP
 *							and bail out (redrawing if necessary the inline area).
 *		09/27/00	kwk	Handle clause offset > 0 correctly, for renbunsetsu conversion.
 *
 ***********************************************************************/
static void TextServicesHandleAction (FieldType* fldP, TsmFepStatusType* ioStatus, 
		TsmFepActionType* ioAction)
{
	UInt16 				startPos;
	UInt16				savedEndPos;
	UInt16 				textLength;
	UInt16				charsToDelete;
	FontID				curFont;
	UInt16 				refNum = GTsmFepLibRefNum;
	Err					result;
	
	curFont = FntSetFont (fldP->fontID);

	startPos = GInlineStart;
	
	// If we didn't have an active inline session, then see how much
	// of the priming text we passed the input method was actually
	// made part of the FEP's buffer...the rest of it falls off the back
	// end of the inline area.
	
	if (!GInlineActive)
		{
		savedEndPos = startPos;
		GInlineEnd = GInlineStart + ioAction->primedLength;
		}
	else
		{
		savedEndPos = GInlineEnd;
		}
	
	// Advance the beginning of the inline area by the amount that's being
	// removed from the input method's buffer.
	
	GInlineStart += ioAction->dumpLength;

	if (ioAction->updateText) 
		{
		textLength = ioStatus->convertedLen + ioStatus->pendingLen;
		charsToDelete = GInlineEnd - startPos;

		ErrFatalDisplayIf(fldP->textLen + textLength - charsToDelete > fldP->maxChars,
							"Inline conversion exceeded field max size");
		
		// If we primed the FEP save the priming text to the undo buffer.
		if (ioAction->primedLength)
			{
			// Make sure we save off the inline state info when FldReleaseFocus
			// is called on us as the alert gets displayed. We also collapse the
			// inline area, since if there was primed text, this isn't yet part
			// of the inline area, and thus we don't want FldGrabFocus redrawing
			// the area with an underline.
			GInlineActive = true;
			GInlineEnd = GInlineStart;
			if (!UndoRecordInput (fldP, startPos, textLength, charsToDelete))
				{
				if (TsmLibFepReset (refNum, ioStatus) != errNone)
					{
					ErrNonFatalDisplay("TsmLibFepReset returned an error");
					}

				FntSetFont (curFont);
				return;
				}
			}
		else
			{
			UndoReset ();
			}

		CancelSelection (fldP);
		GInlineEnd = startPos + textLength;
		GInlineClauseOffset = ioStatus->clauseStart;
		GInlineClauseLen = ioStatus->clauseEnd - ioStatus->clauseStart;
		GInlineActive = GInlineEnd > GInlineStart;
		
		// Check for case where we're dumping text, and thus need to adjust
		// the clause offset and (possibly) the clause length, if we dump some
		// or all of the clause text.
		TextServicesSyncClause(ioAction->dumpLength);
		
		// If we can't do the insertion, then we're probably out of memory. At
		// this point the FEP and field are out-of-sync, so reset the FEP and bail.
		if (!InsertTextAndWrap (fldP,
										(Char*)ioStatus->inlineText,
										textLength,
										startPos,
										charsToDelete,
										GInlineStart + ioStatus->selectEnd))
			{
				if (TsmLibFepReset (refNum, ioStatus) != errNone)
					{
					ErrNonFatalDisplay ("TsmLibFepReset returned an error");
					}
				
				// We're no longer active. Also say that we didn't handle the event,
				// as otherwise TextServicesHandleEvent will re-active us, and then
				// the FEP is out of sync with the field.
				
				// DOLATER kwk - currently
				// this means the user gets two "Out of memory" messages because
				// the call to InsertTextAndWrap triggers it first, then we return
				// false and thus the field code (without inline) also tries to
				// insert the character, and gets the same error. To fix this we'd
				// want to fix the code in TextServicesHandleEvent, which incorrectly
				// assumes that if the event was handled, inline must be active.
				// Better would be for the inline active state to be always set
				// here, and perhaps more directly controlled by the FEP.
				GInlineActive = false;
				ioAction->handledEvent = false;
				
				// Redraw all of the inline text so that the underline is removed.
				if (startPos < savedEndPos)
					{
					DrawMultiLineRange (fldP, startPos, savedEndPos, false);
					}

				FntSetFont (curFont);
				return;
			}
		}
	else
		{
		TextServicesSyncClause(ioAction->dumpLength);
		}
	
	if (ioAction->updateSelection && fldP->attr.visible) 
		{
		CancelSelection (fldP);

		if (ioStatus->selectStart == ioStatus->selectEnd) 
			{
			SetInsPt (fldP, true, startPos + ioStatus->selectStart);
			}
		else
			{
			InsPtEnable (false);
			SetSelection (	fldP,
						 		startPos + ioStatus->selectStart,
								 startPos + ioStatus->selectEnd);
			}
		}
	

	// Now if we've dumped text, we want to make sure we've got the inline 
	// area underlined properly.
	if (ioAction->dumpLength > 0 && fldP->attr.visible) 
		{
		// We've taken dumpLength characters from the from of the inline
		// area and moved them into the text body, so update our beginning
		// offset.
		if (! ioAction->updateText)
			{
			CancelSelection (fldP);
			InsPtEnable (false);
			DrawMultiLineRange (fldP, startPos, GInlineEnd, false);
			
			// DOLATER kwk - if we call SetBlinkingInsPtLocation when the insertion point isn't
			// visible, it dies, because insPtYPos will be set to absolutePosInX. Seems like the
			// right way to really fix this is to catch that case in SetBlinkingInsPtLocation,
			// but that's a more significant change.
			// OK, it gets uglier. If the field has expanded as a result of
			// inline text being entered, then this call to SetBlinkingInsPtLocation
			// will wind up setting insPtYPos to absolutePosInX, because the field hasn't expanded
			// yet and thus the position isn't visible.
			
			if (fldP->attr.insPtVisible)
				SetBlinkingInsPtLocation (fldP);
			
			InsPtEnable (fldP->attr.hasFocus);
			}
		}
	

	// If we have converted text, and the text has changed, make sure that it's scrolled into view.
	// Forcing the text to be changing prevents a bug where we'd reenable the insertion
	// point when a menu was up.
	// DOLATER kwk - should we save/restore the enabled status of the
	// insertion point here?
	
	// DOLATER kwk - for now this works because if we have converted text we'll never have
	// a selection range, only an insertion point.
	
	// 981217 kwk Don't do this if it's a dynamic field. The problem is that if we scroll up,
	// then the field height is expanded due to the fldHeightChanged event, you wind up with
	// a blank line following the (scrolled) second line. DOLATER kwk - the real fix here is probably
	// to have the FldHandleEvent code catch the case of the field height expanding but
	// blank lines existing, and scroll things down to fill in the space.
	
	if ((GInlineClauseLen != 0) && (ioAction->updateText) && !fldP->attr.dynamicSize)
		{
		UInt16 insPtPos = FldGetInsPtPosition(fldP);
		InsPtEnable(false);
		ScrollRangeIntoView(fldP, GInlineStart + GInlineClauseOffset, GInlineStart + GInlineClauseOffset + GInlineClauseLen);
		SetInsPt(fldP, true, insPtPos);
		}
		
	// Reset the Graffiti Shift State indicator,  this indicator is 
	// also used to display the Text Services Manager's input mode.
	if (ioAction->updateFepMode)
		{
		// Auto-shift if the field is empty.
		if (fldP->attr.autoShift && (! fldP->textLen))
			GrfSetState (false, false, true);
		else
			GrfSetState (false, false, false);
		} 

	// Tell the input method that we've processed the action, so it's
	// safe to release dumped text, unlock handles, etc.
	result = TsmLibFepCommitAction(refNum, ioStatus);
	ErrFatalDisplayIf(result, "FEP couldn't commit action");
	
	FntSetFont (curFont);
}


/***********************************************************************
 *
 * FUNCTION:    		TextServicesReset
 *
 * DESCRIPTION:		Reset the inline text area.
 *
 * PARAMETERS:
 *		fldP		 ->	pointer to a FieldType structure.
 *		doUpdate	 ->	true for updating text & selection.
 *
 * RETURNED:	 		nothing
 *
 * HISTORY:
 *		12/04/97	art	Created by Art Lamb.
 *		09/28/98	kwk	Add doUpdate param, call to TsmReset.
 *		09/29/98	kwk	Always call TsmReset if we don't call TsmTerminate.
 *		10/09/98	kwk	Bail out immediately if no TSM library.
 *		09/20/00	kwk	Call TsmLibFepTerminate if inline is active, even
 *							if <doUpdate> is false, so that the tsm commit
 *							event is generated. Use non-fatal alert for error
 *							conditions.
 *
 ***********************************************************************/
static void TextServicesReset (FieldType* fldP, Boolean doUpdate)
{
	UInt16				pos;
	FontID				curFont;
	TsmFepActionType	tsmAction;
	TsmFepStatusType* status = (TsmFepStatusType*)GTsmFepLibStatusP;
	UInt16 				refNum = GTsmFepLibRefNum;
	Boolean				needReset = true;
	
	if (refNum == sysInvalidRefNum)
		return;
	
	// Can't possibly have deferred termination, since we're terminating now.
	GInlineDefTerm = false;
	
	// The only time we want to call TsmTerminate is if inline is active, so
	// that a tsmCommit event gets generated. Otherwise for safefy we'll just
	// reset the input method and turn off inline.
	if (GInlineActive)
		{
		// If a commit is pending then we don't reset.
		if (TsmLibFepTerminate(refNum, status, &tsmAction) == errNone)
			{
			if (doUpdate)
				{
				TextServicesHandleAction(fldP, status, &tsmAction);
				needReset = false;
				}
			}
		else
			{
			ErrNonFatalDisplay("TsmLibFepTerminate returned an error");
			return;
			}
		}
	
	if (needReset)
		{
		if (TsmLibFepReset (refNum, status) != errNone)
			{
			ErrNonFatalDisplay("TsmLibFepReset returned an error");
			return;
			}
		}

	curFont = FntSetFont (fldP->fontID);

	pos = GetStringPosOfInsPtPos (fldP);
	GInlineStart = GInlineEnd = pos;
	GInlineClauseOffset = GInlineClauseLen = 0;
	GInlineActive = false;
	
	ECTextServices(fldP, status);

	FntSetFont (curFont);
}


/***********************************************************************
 *
 * FUNCTION:	TextServicesCreateEvent
 *
 * DESCRIPTION:	Fill in the structure that is passed to the 
 *				Text Services Library with each event.
 *
 * PARAMETERS:	fldP    pointer to a FieldType structure.
 *				event  pointer to an EventType structure.
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art		03/17/98	Initial Revision
 *			kwk		07/30/98	Initialize new formEvent field to false.
 *			kwk		08/27/98	Set up new maxInline field.
 *			kwk		09/15/98	Fixed maxInline calc.
 *			art		09/21/98	Set up penLeading field.
 *
 ***********************************************************************/
static void TextServicesCreateEvent (EventType* event, FieldType* fldP, 
	TsmFepEventType* outTsmEvent)
{
	UInt16 	pos;
	FontID	curFont;

	curFont = FntSetFont (fldP->fontID);
	
	
	pos = GetPosOfPoint (fldP, event->screenX, event->screenY, &outTsmEvent->penLeading);
	outTsmEvent->penOffset = (Int16)pos - GInlineStart;
	
	// The max size of the inline area is the max size of the field,
	// less text that occurs before & after the inline hole.
	outTsmEvent->maxInline = fldP->maxChars - GInlineStart - (fldP->textLen - GInlineEnd);
	
	outTsmEvent->formEvent = false;
	
	// If inline isn't active, then we want to fill in a 'priming text'
	// pointer & length, otherwise set this to nil & 0. Note that this
	// assumes GInlineStart/GInlineEnd are set up by the time we get
	// called (typically to be the same as the selection range).
	if (GInlineActive || (GInlineEnd == GInlineStart))
		{
		outTsmEvent->primeText = NULL;
		outTsmEvent->primeOffset = 0;
		outTsmEvent->primeLen = 0;
		}
	else 
		{
		outTsmEvent->primeText = fldP->text;
		outTsmEvent->primeOffset = GInlineStart;
		outTsmEvent->primeLen = GInlineEnd - GInlineStart;
		}

	FntSetFont (curFont);
}


/***********************************************************************
 *
 * FUNCTION:    TextServicesHandleEvent
 *
 * DESCRIPTION: 
 *
 * PARAMETERS:	 fldP    pointer to a FieldType structure.
 *              event  pointer to an EventType structure.
 *
 * RETURNED:	 true if the event was handled by the field.
 *
 * HISTORY:
 *		03/13/98	art	Created by Art Lamb.
 *		09/10/99	kwk	Call ECTextServices after check for hasFocus, as
 *							otherwise if we've got deferred termination, and
 *							the field doesn't have the focus yet, we'll get
 *							a fatal alert.
 *
 ***********************************************************************/
static Boolean TextServicesHandleEvent (FieldType* fldP, EventType* eventP)
{
	TsmFepEventType		tsmEvent;
	TsmFepActionType		tsmAction;
	TsmFepStatusType* 	status = (TsmFepStatusType*)GTsmFepLibStatusP;
	UInt16 					refNum = GTsmFepLibRefNum;
	UInt16					selStart;
	UInt16					selEnd;
	Boolean					handled;
	
	// If we don't have an input method, just let the field handle it
	// normally.
	if (refNum == sysInvalidRefNum) 
		{
		return (false);
		}
	
	if (! fldP->attr.editable)
		return (false);

	if ( ! fldP->attr.hasFocus)
		return (false);

	if (fldP->attr.numeric)	
		return (false);

	// If it's a pen down event that's not in the field, just return
	// false.
	if (eventP->eType == penDownEvent)
		{
		return (false);
		}
	
	ECTextServices(fldP, status);

	// If inline isn't active, then set the inline range to be the selected
	// text. Note that we'll have to adjust this later, depending on the
	// primedLength result returnd from the input method.
	
	if (! GInlineActive) 
		{
		FldGetSelection (fldP, &selStart, &selEnd);
		GInlineStart = selStart;
		GInlineEnd = selEnd;

		GInlineClauseOffset = GInlineClauseLen = 0;
		}

	if (eventP->eType != fldEnterEvent)
		{
		TextServicesCreateEvent (eventP, fldP, &tsmEvent);
		TsmLibFepHandleEvent (refNum, (const SysEventType *)eventP, &tsmEvent, status, &tsmAction);
		TextServicesHandleAction (fldP, status, &tsmAction);

		handled = tsmAction.handledEvent;
		}
		
	else
		{
		EventType newEvent = *eventP;
		
		newEvent.eType = penDownEvent;

		// Loop until the pen comes up down, sending a stream of penMoveEvent
		// events and a penUpEvent to the input method.
		while (true)
			{
			TextServicesCreateEvent (&newEvent, fldP, &tsmEvent);
			TsmLibFepHandleEvent(refNum, (SysEventType *)&newEvent, &tsmEvent, status, &tsmAction);
			TextServicesHandleAction (fldP, status, &tsmAction);

			if (newEvent.eType == penDownEvent)
				{
				handled = tsmAction.handledEvent;
				if (! handled)
					break;
				}
			else if (newEvent.eType == penUpEvent)
				{
				// DOLATER kwk - this should probably be:
				// handled = tsmAction.handledEvent;
				// but previously the code wasn't setting up handled in this case,
				// which means it had the value from the previous loop through, which
				// most likely was true (for the penDown case above, which would have
				// the be the first pass through the loop), so set it to true for now.
				handled = true;
				break;
				}

			PenGetPoint (&newEvent.screenX, &newEvent.screenY, &newEvent.penDown);

			if (newEvent.penDown)
				newEvent.eType = penMoveEvent;
			else
				newEvent.eType = penUpEvent;
			} 
		}

	// If the input method handled the event, but we're not yet active, then
	// activate us.
	if (handled) 
		{
		GInlineActive = true;
		}

	// If we don't have an inline area as a result of handling the action, then
	// inline isn't active.
	if (GInlineStart == GInlineEnd) 
		{
		GInlineActive = false;
		}

	// if we primed the inline range with the selected text and the event was
	// not handled by the Text Services Manager then reset the inline range.
	// DOLATER kwk - this doesn't seem to be required, since we don't care about the
	// inline start/end values if inline isn't active, right?
	
	else if (! GInlineActive)
		{
		GInlineStart = GInlineEnd = GetStringPosOfInsPtPos (fldP);
		}

	ECTextServices(fldP, status);
	ECValidateField(fldP, true, true);

	return (handled);
}



/***********************************************************************
 *
 * FUNCTION:    FldSetMaxVisibleLines
 *
 * DESCRIPTION: Fields can by dynamically expandable, and when they are
 *					 the field package needs to know how many lines at most
 *					 will be visible so it knows not to try to expand them
 *					 further.  Since expansion is actually handled by enclosing
 *					 objects (tables or forms), this API exists to allow the
 *					 enclosing object to tell the field how big it can ever get.
 *					 (Tables will assume the field can get as big as the table.)
 *
 * PARAMETERS:	 fldP    	-> pointer to a FieldType structure.
 *              maxLines  	-> max lines that field will visually grow to 
 *
 * HISTORY:
 *		11/22/00	bob	Created to fix 41184.
 *
 ***********************************************************************/
void FldSetMaxVisibleLines (FieldType *fldP, UInt8 maxVisibleLines)
{
	ErrNonFatalDisplayIf(!fldP->attr.dynamicSize, "Pointless for non-expanding fields");
	
	fldP->maxVisibleLines = maxVisibleLines;
}

