/******************************************************************************
 *
 * Copyright (c) 1995-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: Table.c
 *
 * Release: 
 *
 * Description:
 *	  This file contains the table object routines.
 *
 * History:
 *		February 7, 1995	Created by Art Lamb
 *		10/31/96	art - Initial Revision
 *		 2/22/99	bob - Updated for color
 *		 5/18/99	rbb - Updated for multi-tap selection
 *     7/27/00	gap - Add support for hiding tables.
 *		10/20/00	peter - Add support for tallCustomTableItem.
 *
 *****************************************************************************/

#define NON_PORTABLE

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_CLIPBOARDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_FIELDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS
#define ALLOW_ACCESS_TO_INTERNALS_OF_TABLES
#define ALLOW_ACCESS_TO_INTERNALS_OF_CONTROLS
#define ALLOW_ACCESS_TO_INTERNALS_OF_FORMS

#include <PalmTypes.h>
#include <SystemPublic.h>

#include <PalmUtils.h>

#include "SysTrapsFastPrv.h"

#include "Control.h"
#include "Field.h"
#include "Table.h"
#include "UIGlobals.h"
#include "UIColorPrv.h"
#include "UIResources.h"
#include "UIResourcesPrv.h"

// DOLATER:  We need to go through and consistently set the fore, back,
// and text colors here!  Some routines in the table code set the colors
// themselves, while others expect the callers to set them.  This definitely
// leads to inconsistent use of colors in tables.  Apps can always
// override the default table colors by creating custom table entries and
// setting the colors themselves when the custom table entry callbacks
// are called.  But we definitely need to clean up the default behavior
// for tables in general!  -- jmp, 11/08/99

/***********************************************************************
 *
 *	Internal constants and functions
 *
 ***********************************************************************/
#define tableSelectIndictorHeight	11
#define tableFont							boldFont


/***********************************************************************
 *
 * FUNCTION:    ECValidateTable
 *
 * DESCRIPTION: This routine performs various edit checks on a tableP 
 *              object.
 *
 * PARAMETERS:	 tableP  pointer to a TableType structure.
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	10/31/96	Initial Revision
 *
 ***********************************************************************/
static void ECValidateTable (TableType * tableP)
{
	ErrFatalDisplayIf (tableP->currentRow >= tableP->numRows, 
		"Invalid tableP row");

	ErrFatalDisplayIf (tableP->currentColumn >= tableP->numColumns, 
		"Invalid tableP column");
	
	// If the table is using a field, validate it.
	if (tableP->attr.editing)
		{
		ErrNonFatalDisplayIf(!tableP->rowAttrs[tableP->currentRow].usable, "Text not usable");
		
		// Validate the field.
		FldGetTextPtr(&tableP->currentField);
		}
}


/***********************************************************************
 *
 * FUNCTION:    InitCheckboxItem
 *
 * DESCRIPTION: This routine initializes a checkbox tableP item.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              clt    - pointer to a control structure
 *              x      - left bound to the item
 *              y      - top bound of the item
 *              on     - true if checkbox is checked
 *              
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/26/95	Initial Revision
 *
 ***********************************************************************/
static void InitCheckboxItem (ControlType * ctlP, Coord x, Coord y, Coord width, Boolean on)
{
	FontID curFont;

	MemSet (ctlP, sizeof(ControlType), 0);
	ctlP->attr.usable = true;
	ctlP->attr.enabled = true;
	ctlP->style = checkboxCtl;
	ctlP->bounds.topLeft.x = x;
	ctlP->bounds.topLeft.y = y;
	ctlP->bounds.extent.x = width;

	curFont = FntSetFont (tableFont);
	ctlP->bounds.extent.y = FntLineHeight ();
	FntSetFont (curFont);
	
	CtlSetValue (ctlP, on);
}


/***********************************************************************
 *
 * FUNCTION:    InitTextItem
 *
 * DESCRIPTION: This routine initializes a editable text tableP item.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *
 * RETURNED:	 Error code or errNone (0) if no error.
 *
 * HISTORY:
 *		05/03/95	art	Created by Art Lamb.
 *    04/12/96	art	Set current field editable before calling load
 *                     data callback routine.
 *		06/18/96	art	Add fld parameter to load data callback routine.
 *		08/26/97	art	Set font id form value in a table structure.
 *		10/01/98	kwk	Removed hasFocus parameter.
 *		10/27/98	Bob	use editable param for field init instead of true.
 *		09/15/99	kwk	Once again use <editable> param to set field's attribute.
 *
 ***********************************************************************/
static Err InitTextItem (TableType * tableP, FieldType * fldP, Int16 row, 
	Int16 column, Coord x, Coord y, Boolean editable, Boolean visible)
{
	Err err;
	MemHandle textH;
	Int16 textOffset;
	Int16 textAllocSize;
	TableItemType * item;
	FontID currFont;
	
	item = item = &tableP->items[row*tableP->numColumns+column];

	MemSet (fldP, sizeof(FieldType), 0);
	
	fldP->attr.usable = true;
	fldP->attr.visible = visible;
	fldP->attr.editable = editable;
	fldP->attr.underlined = true;
	fldP->attr.insPtVisible = true;
	fldP->attr.hasFocus = false;
	fldP->attr.dynamicSize = ( ! tableP->rowAttrs[row].staticHeight);
	fldP->attr.hasScrollBar = tableP->attr.hasScrollBar;

	fldP->rect.topLeft.x = x;
	fldP->rect.topLeft.y = y;
	fldP->rect.extent.x = tableP->columnAttrs[column].width;
	fldP->rect.extent.y = min (tableP->rowAttrs[row].height,
		tableP->bounds.topLeft.y + tableP->bounds.extent.y - y);

	fldP->maxChars = tableMaxTextItemSize;

	fldP->fontID = item->fontID;

	if (fldP->attr.dynamicSize)
	{
		currFont = FntSetFont(item->fontID);
		fldP->maxVisibleLines = tableP->bounds.extent.y / FntLineHeight();
		FntSetFont(currFont);
	}
	else
		fldP->maxVisibleLines = 0;
		
	// If the item has a note, leave space for the note indicator.
	if (item->itemType == textWithNoteTableItem)
		fldP->rect.extent.x -= tableNoteIndicatorWidth;
		
	// If the application has reserved space to the rigth of the text item,
	// leave space for it.
	else if (item->itemType == narrowTextTableItem)
		fldP->rect.extent.x -= item->intValue;

	// inconsistent state if you allow focus on non-text items, so don't.  bug 46955
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	else if (item->itemType != textTableItem)
		ErrDisplay("Can't focus on non-text item.");
#endif

	if (editable)
		tableP->attr.editing = true;

	
	// Call the application to get the text to display in the field.
	if (! tableP->columnAttrs[column].loadDataCallback)
		return (0);
		
	err = tableP->columnAttrs[column].loadDataCallback (tableP, row, column,
		editable, &textH, &textOffset, &textAllocSize, fldP);

	if (! FldGetTextHandle (fldP))
		FldSetText (fldP, textH, textOffset, textAllocSize);
	
	return (err);
}



/***********************************************************************
 *
 * FUNCTION:    DrawLableItem
 *
 * DESCRIPTION: This routine draw a label item in table object.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/7/95	Initial Revision
 *
 ***********************************************************************/
static void DrawLabelItem (TableType * tableP, TableItemType * item, Int16 column, 
	Coord x, Coord y)
{
	Int16 len;
	Coord width;
	
	WinSetTextColor(UIColorGetIndex(UIObjectForeground));
	FntSetFont (stdFont);
	
	len = StrLen (item->ptr);
	width = FntCharsWidth (item->ptr, len);
	x += tableP->columnAttrs[column].width - width - FntCharWidth (colonChr);
	WinDrawChars (item->ptr, len, x, y);
	x += width;
	WinDrawChar (colonChr, x, y);
}


/***********************************************************************
 *
 * FUNCTION:    DrawNumericItem
 *
 * DESCRIPTION: This routine draw a numeric tableP item.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              item   - pointer to the item
 *              column - 
 *              
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/7/95	Initial Revision
 *
 ***********************************************************************/
static void DrawNumericItem (TableType * tableP, TableItemType * item, Int16 column, 
	Coord x, Coord y)
{
	Int16 len;
	Coord width;
	Char str[8];
	
	FntSetFont (boldFont);
	WinSetTextColor(UIColorGetIndex(UIObjectForeground));
	
	StrIToA (str, item->intValue);
	len = StrLen (str);
	
	width = FntCharsWidth (str, len) - 1;
	x += (tableP->columnAttrs[column].width - width) / 2;
	WinDrawChars (str, len, x, y);
}


/***********************************************************************
 *
 * FUNCTION:    DrawTextItem
 *
 * DESCRIPTION: This routine draw a text item in table object.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *
 * RETURNED:	 nothing
 *
 * HISTORY:
 *		02/07/95	art	Initial Revision
 *		10/27/98	bob	re-use field if possible.
 *		09/15/99	kwk	Turn on editable flag immediately before FldDrawField call.
 *
 ***********************************************************************/
static void DrawTextItem (TableType * tableP, TableItemType * item, 
	Int16 row, Int16 col, Coord x, Coord y)
{
	Char noteChr;
	Coord drawX;
	FieldType field;
	
	if (tableP->attr.editing &&
		row == tableP->currentRow && 
		col == tableP->currentColumn)
		{
		FldDrawField(&tableP->currentField);
		}
	else
		{
		FieldAttrType attributes;
		
		InitTextItem (tableP, &field, row, col, x, y, false, false);

		// Turn on the editable attribute so that the field gets drawn
		// correctly on color devices. Note that we do it here, as
		// otherwise the field code complains about editable fields
		// with overlocked handles (if a record contains multiple
		// fields, one of which is currently being edited in the
		// table when we're asked to redraw).
		FldGetAttributes (&field, &attributes);
		attributes.editable = true;
		FldSetAttributes (&field, &attributes);
		FldDrawField (&field);

		// Note setting the texthandle to NULL before calling FldFreeMemory
		// can result in a memory leak.  Unfortunately, calling FldFreeMemory
		// before FldSetTextHandle will cause problems if the handle is in
		// the storage heap.  
		FldSetTextHandle (&field, NULL);
		FldFreeMemory (&field);
		}
			
	// If the item has a note, draw the note indicator.
	if (item->itemType == textWithNoteTableItem)
		{
		FntSetFont (symbolFont);
		noteChr = symbolNote;
		drawX = x + tableP->columnAttrs[col].width - FntCharWidth (noteChr);
		WinDrawChars (&noteChr, 1, drawX, y);
		}
}


/***********************************************************************
 *
 * FUNCTION:    DrawCheckboxItem
 *
 * DESCRIPTION: This routine draws a checkbox tableP item.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              item   - pointer to an item in the tableP
 *              row    - row the item is in
 *              column - column the item is in
 *              x      - left bound to the item
 *              y      - top bound of the item
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/26/95	Initial Revision
 *
 ***********************************************************************/
static void DrawCheckboxItem (TableType * tableP, TableItemType * item, 
	Int16 column, Coord x, Coord y)
{
	ControlType ctl;
		
	InitCheckboxItem (&ctl, x, y, tableP->columnAttrs[column].width,
				item->intValue != 0);
		
	CtlDrawControl (&ctl);
}


/***********************************************************************
 *
 * FUNCTION:    DrawPopupTriggerItem
 *
 * DESCRIPTION: This routine draws a popup trigger item of table object.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              item   - pointer to an item in the tableP
 *              row    - row the item is in
 *              column - column the item is in
 *              x      - left bound to the item
 *              y      - top bound of the item
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/7/95	Initial Revision
 *
 ***********************************************************************/
static void DrawPopupTriggerItem (TableType * tableP, TableItemType * item, 
	Int16 column, Coord x, Coord y)
{
	ControlType ctl;
	Char * textP;
			
	FntSetFont (stdFont);
	WinSetForeColor(UIColorGetIndex(UIObjectForeground));
	WinSetTextColor(UIColorGetIndex(UIObjectForeground));

	MemSet (&ctl, sizeof(ControlType), 0);
	ctl.attr.usable = true;
	ctl.attr.enabled = true;
	ctl.style = popupTriggerCtl;
	ctl.bounds.topLeft.x = x;
	ctl.bounds.topLeft.y = y;
	ctl.bounds.extent.x = tableP->columnAttrs[column].width;
	ctl.bounds.extent.y = FntLineHeight ();
	
	textP = LstGetSelectionText ((ListType *) item->ptr, item->intValue);
	CtlSetLabel (&ctl, textP);
	CtlDrawControl (&ctl);
	WinDrawChar (colonChr, x + tableP->columnAttrs[column].width - FntCharWidth(colonChr), y);
}


/***********************************************************************
 *
 * FUNCTION:    DrawDateItem
 *
 * DESCRIPTION: This routine draws a checkbox tableP item.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              item   - pointer to an item in the tableP
 *              row    - row the item is in
 *              column - column the item is in
 *              x      - left bound to the item
 *              y      - top bound of the item
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/26/95	Initial Revision
 *
 ***********************************************************************/
static void DrawDateItem (TableType * tableP, TableItemType * item,
	Int16 column, Coord x, Coord y)
{
	Char dayStr [3];
	Char monthStr [3];
	Int16 dayStrLen;
	Int16 monthStrLen;
	Coord drawX, drawY;
	DateType date;
	DateTimeType today;
	Int32 todayL, dateL;
	
	WinSetForeColor(UIColorGetIndex(UIObjectForeground));
	WinSetTextColor(UIColorGetIndex(UIObjectForeground));
	FntSetFont (stdFont);

	// If there is no date draw a dash to indicate such.
	if (item->intValue == -1)
		{
		drawX = x + tableP->columnAttrs[column].width - 7;
		drawY = y + ((FntLineHeight () + 1) / 2);
		WinDrawLine (drawX, drawY, drawX+5, drawY);		
		return;
		}
	
	*((Int16 *) (&date)) = item->intValue;

	StrIToA (monthStr, date.month);
	StrIToA (dayStr, date.day);
	
	monthStrLen = StrLen (monthStr);
	dayStrLen = StrLen (dayStr);
	
	// Right align the day values.
	drawX = x + tableP->columnAttrs[column].width - 3 -
		FntCharsWidth (monthStr, monthStrLen) -
		FntCharWidth ('/') - 
		FntCharsWidth (dayStr, dayStrLen);	
	drawY = y;
	
	// Draw the date.
	WinDrawChars (monthStr, monthStrLen, drawX, drawY);
	drawX += FntCharsWidth (monthStr, monthStrLen) ;
	
	WinDrawChar ('/', drawX, drawY);
	drawX += FntCharWidth ('/');

	WinDrawChars (dayStr, dayStrLen, drawX, drawY);	
	
	// If the date is on or before today draw an exclamation mark.
	TimSecondsToDateTime (TimGetSeconds(), &today);

	todayL = ( ((Int32) today.year) << 16) + 
				( ((Int32) today.month) << 8) + 
				  ((Int32) today.day);

	dateL = ( ((Int32) date.year + firstYear) << 16) + 
			  ( ((Int32) date.month) << 8) + 
				 ((Int32) date.day);
	
	if (dateL <= todayL)
		{
		drawX += FntCharsWidth (dayStr, dayStrLen) + 1;
		
		FntSetFont (boldFont);
		WinDrawChar ('!', drawX, drawY);
		}

	return;
}


/***********************************************************************
 *
 * FUNCTION:    DrawMask
 *
 * DESCRIPTION: This routine draws the specified item of the tableP.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *						row	- first row to mask
 *						col	- col to mask
 *						rP		- bounds to mask (one item)
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			jaq	7/7/99	Initial Revision
 *			bob	10/28/99	Re-wrote to do masking one entry at a time
 *			peter	05/17/00	Move lock icon up for multi-line masked records
 *
 ***********************************************************************/
static void DrawMask (TableType * tableP, Int16 row, Int16 col, const RectangleType *rP)
{
	Int16 i;
	CustomPatternType pattern = {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55};
	CustomPatternType origPattern;
	MemHandle bitmapH;
	BitmapPtr bitmapP;
	Boolean drawLock = true;
	RectangleType r = *rP;
			
	WinSetForeColor(UIColorGetIndex(UIObjectForeground));

	// if the next usable column is masked, don't draw the lock
	// and extend the masking pattern to cover the inter-column space
	for (i = col + 1; i < tableP->numColumns; i++)
		{
		if (tableP->columnAttrs[i].usable)
			{
			if (tableP->columnAttrs[i].masked)
				{
				drawLock = false;
				r.extent.x += tableP->columnAttrs[col].spacing + 1;
				}
			break;
			}
		}

	// draw lock icon, if necessary and it fits
	if (drawLock && (r.extent.x >= SecLockWidth+1))
		{
		bitmapH = DmGetResource (bitmapRsc, SecLockBitmap);
		if (bitmapH)
			{
			TableItemType *itemP = &tableP->items[row * tableP->numColumns + col];
			FontID oldFont = FntSetFont(itemP->fontID);
			
			r.extent.x -= SecLockWidth;
			bitmapP = MemHandleLock (bitmapH);
			WinDrawBitmap (bitmapP, r.topLeft.x + r.extent.x, 
											r.topLeft.y + ((min(FntLineHeight(), r.extent.y) - SecLockHeight) / 2));
			MemPtrUnlock (bitmapP);
			FntSetFont(oldFont);
			}
		}
	
	//Draw rect
	// Make sure it fits nicely into the display.
	r.topLeft.y++;
	r.extent.y --;
	r.extent.x --;
	
	WinGetPattern(&origPattern);
	WinSetPattern (&pattern);
	WinFillRectangle (&r, 0);
	WinSetPattern(&origPattern);
	
} 
	

/***********************************************************************
 *
 * FUNCTION:    ReplaceTwoColors
 *
 * DESCRIPTION: This routine does a selection or deselection effect by
 *					 replacing foreground and background colors with a new pair
 *					 of colors. In order to reverse the process, you must pass
 *					 the colors in the opposite order, so that the current
 *					 and new colors are known to this routine. This routine
 *					 correctly handling the cases when two or more of these
 *					 four colors are the same, but it requires that the
 *					 affected area of the screen contains neither of the
 *					 given NEW colors, unless these colors are the same as
 *					 one of the old colors.
 *
 * PARAMETERS:	 rP  - pointer to a rectangle to 'invert'
 *					 cornerDiam	- corner diameter
 *					 oldForeground	- UI color currently used for foreground
 *					 oldBackground	- UI color currently used for background
 *					 newForeground	- UI color that you want for foreground
 *					 newBackground	- UI color that you want for background
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	05/19/00	Initial Revision
 *
 ***********************************************************************/
static void ReplaceTwoColors (const RectangleType *rP, UInt16 cornerDiam,
	UIColorTableEntries oldForeground, UIColorTableEntries oldBackground,
	UIColorTableEntries newForeground, UIColorTableEntries newBackground) 
{
	UInt8 oldForegroundIndex = UIColorGetIndex(oldForeground);
	UInt8 oldBackgroundIndex = UIColorGetIndex(oldBackground);
	UInt8 newForegroundIndex = UIColorGetIndex(newForeground);
	UInt8 newBackgroundIndex = UIColorGetIndex(newBackground);
	
	WinPushDrawState();
	WinSetDrawMode(winSwap);
	WinSetPatternType (blackPattern);
	
	if (newBackgroundIndex == oldForegroundIndex)
		if (newForegroundIndex == oldBackgroundIndex)
		{
			// Handle the case when foreground and background colors change places,
			// such as on black and white systems, with a single swap.
			WinSetBackColor(oldBackgroundIndex);
			WinSetForeColor(oldForegroundIndex);
			WinPaintRectangle(rP, cornerDiam);
		}
		else
		{
			// Handle the case when the old foreground and the new background
			// are the same, using two swaps.
			WinSetBackColor(oldForegroundIndex);
			WinSetForeColor(oldBackgroundIndex);
			WinPaintRectangle(rP, cornerDiam);
			WinSetBackColor(oldBackgroundIndex);
			WinSetForeColor(newForegroundIndex);
			WinPaintRectangle(rP, cornerDiam);
		}
	else if (oldBackgroundIndex == newForegroundIndex)
	{
		// Handle the case when the old background and the new foreground
		// are the same, using two swaps.
		WinSetBackColor(newForegroundIndex);
		WinSetForeColor(oldForegroundIndex);
		WinPaintRectangle(rP, cornerDiam);
		WinSetBackColor(newBackgroundIndex);
		WinSetForeColor(oldForegroundIndex);
		WinPaintRectangle(rP, cornerDiam);
	}
	else
	{
		// Handle the case when no two colors are the same, as is typically the case
		// on color systems, using two swaps.
		WinSetBackColor(oldBackgroundIndex);
		WinSetForeColor(newBackgroundIndex);
		WinPaintRectangle(rP, cornerDiam);
		WinSetBackColor(oldForegroundIndex);
		WinSetForeColor(newForegroundIndex);
		WinPaintRectangle(rP, cornerDiam);
	}
	
	WinPopDrawState();
}


/***********************************************************************
 *
 * FUNCTION:    DrawItem
 *
 * DESCRIPTION: This routine draws the specified item of the tableP.
 *					 It assumes the called has already set up a "protected"
 *					 draw state, so it is free to change the current draw
 *					 state without saving/restoring.
 *
 * PARAMETERS:	 tableP  -> pointer to a table object
 *					 row		-> row to draw
 *					 col		-> column to draw
 *					 rP		-> rectangle to invert, if any.  Almost always
 *									top/left is top/left for item.  Rare exception
 *									is note items, where top/left may be for note
 *									icon if selecting that icon.
 *					 selected-> true to draw as selected (invert when done)
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/7/95	Initial Revision
 *			bob	10/28/99	Handle 'selected' state by inverting (swapping)
 *			jmp	10/30/99	Separate textTableItem & textWithNoteTableItem out
 *								due to the fact that since we're now using this
 *								routine for drawing items both selected and un-
 *								selected, we need to reset the {x,y} selection
 *								coordinate of the textWithNoteTableItem corresponding
 *								with the textTableItem.  Fixes bug #23304.
 *			bob	11/16/99	Erase here, and use WinInvert for custom items
 *								with no draw function.  (Fixes Find)
 *			jmp	11/24/99	Limit the extent of the rectangle we're erasing so
 *								as to not erase too much (fixes bugs #24146 and #24193).
 *								Also, eliminated now redundant WinEraseRectangle() in
 *								textWithNoteTableItem case.
 *			peter	05/19/00	Improve selection effect for custom items without callbacks.
 *
 ***********************************************************************/
static void DrawItem (TableType * tableP, Int16 row, Int16 col,
							 const RectangleType *rP, Boolean selected)
{
	TableItemType * item;
	RectangleType boundsR = *rP;
			
	item = &tableP->items[row*tableP->numColumns + col];
	
	// Set the appropriate size of the bounding rectangle, and erase
	// it when necessary.
	boundsR.extent.x = min(tableP->columnAttrs[col].width, boundsR.extent.x);
	boundsR.extent.y = min(tableP->rowAttrs[row].height, boundsR.extent.y);

	if ((item->itemType != customTableItem && item->itemType != tallCustomTableItem) ||
			(tableP->columnAttrs[col].drawCallback))
		WinEraseRectangle(&boundsR, 0);

	// Always draw a masked item masked.
	if (tableP->rowAttrs[row].masked && tableP->columnAttrs[col].masked)
		DrawMask(tableP, row, col, rP);
	else			
		switch (item->itemType)
			{
			case labelTableItem:
				DrawLabelItem (tableP, item, col, rP->topLeft.x, rP->topLeft.y);
				break;

			case numericTableItem:
				DrawNumericItem (tableP, item, col, rP->topLeft.x, rP->topLeft.y);
				break;

			case textTableItem:
				DrawTextItem (tableP, item, row, col, rP->topLeft.x, rP->topLeft.y);
				break;
				
			case textWithNoteTableItem:
				// Must get bounds again because x/y passed in rP may be for the note
				// icon and not for the whole row.
				TblGetItemBounds (tableP, row, col, &boundsR);
				DrawTextItem (tableP, item, row, col, boundsR.topLeft.x, boundsR.topLeft.y);
				break;

			case checkboxTableItem:
				DrawCheckboxItem (tableP, item, col, rP->topLeft.x, rP->topLeft.y);
				break;

			case popupTriggerTableItem:
				DrawPopupTriggerItem (tableP, item, col, rP->topLeft.x, rP->topLeft.y);
				break;

			case dateTableItem:
				DrawDateItem (tableP, item, col, rP->topLeft.x, rP->topLeft.y);
				break;

			case narrowTextTableItem:
				DrawTextItem (tableP, item, row, col, rP->topLeft.x, rP->topLeft.y);
				// fall through

			case customTableItem:
			case tallCustomTableItem:
				// If there is a custom draw routine, call it.
				if (tableP->columnAttrs[col].drawCallback)
					tableP->columnAttrs[col].drawCallback (tableP, row, col, &boundsR);

				// HACK: no custom draw routine, so use the fontID field as a flag to
				// indicate the items selected state, and replace colors in the
				// rectangle if it changes. This will work reliably if the item is
				// drawn using only the object foreground color.
				else {
					if (item->fontID != selected)
						if (selected)		// change from not selected to selected
							ReplaceTwoColors (&boundsR, 0,
								UIObjectForeground, UIDialogFill,
								UIObjectSelectedForeground, UIObjectSelectedFill);
						else					// change from selected to not selected
							ReplaceTwoColors (&boundsR, 0,
								UIObjectSelectedForeground, UIObjectSelectedFill,
								UIObjectForeground, UIDialogFill);
					item->fontID = (FontID)selected;
					return;	// bail out before the normal inversion effect below
				}
				break;
			}
	
	// Swap foreground & background with selected foreground & background to 'invert' indicator.
	if (selected)
		ReplaceTwoColors (rP, 0,
			UIObjectForeground, UIFieldBackground, UIObjectSelectedForeground, UIObjectSelectedFill);

}


/***********************************************************************
 *
 * FUNCTION: 	 DrawRow
 *
 * DESCRIPTION: Draws a whole row of a table.
 *
 * PARAMETERS:	 tableP - pointer to a table object
 *              row    - (masked) row to select
 *              rowR	  - rectangle for the whole row
 *					 selected - true to draw in selected style
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	10/28/99	Initial Revision
 *
 ***********************************************************************/
static void DrawRow(TableType *tableP, Int16 row, const RectangleType *rowRP, Boolean selected)
{
	Int16 i;
	RectangleType itemR = *rowRP;
	
	for (i = 0; i < tableP->numColumns; i++)
		if (tableP->columnAttrs[i].usable)
		{
			itemR.extent.x = tableP->columnAttrs[i].width;
			DrawItem(tableP, row, i, &itemR, false);
			itemR.topLeft.x += itemR.extent.x + tableP->columnAttrs[i].spacing;
		}
	
	// just draw the inversion effect once for the whole row
	if (selected)
		ReplaceTwoColors (rowRP, 0,
			UIObjectForeground, UIFieldBackground, UIObjectSelectedForeground, UIObjectSelectedFill);
}


/***********************************************************************
 *
 * FUNCTION:    DrawEditIndicator
 *
 * DESCRIPTION: This routine inverts the region to the left of the text 
 *              item, to indicate that the item is selected  
 *
 * PARAMETERS:	 tableP   - pointer to a table object
 *					 selected - if true, draw indicator selected
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/16/95	Initial Revision
 *			bob	2/22/99	Made from InvertEditIndicator, for color support
 *								added selected boolean
 *			jmp	11/25/99	When computing the size of the edit indicator, ensure
 *								that we take into account whether the previous column
 *								is currently usable or not.  Fixes bug #24092.
 *
 ***********************************************************************/
static void DrawEditIndicator (TableType * tableP, Boolean selected)
{
	Int16 col;
	RectangleType itemR, indicatorR;
	Boolean drawn = false;
	Int16 lastColSpacing = 0;

	TblGetItemBounds (tableP, tableP->currentRow, tableP->currentColumn, &itemR);

	indicatorR.topLeft.y = itemR.topLeft.y;
	indicatorR.extent.y = tableSelectIndictorHeight;
	indicatorR.topLeft.x = tableP->bounds.topLeft.x;
	indicatorR.extent.x = 0;
		
	// Find the start of the edit indicator.
	for (col = 0; col < tableP->numColumns; col++)
		{
		if (tableP->columnAttrs[col].usable)
			{
			if (tableP->columnAttrs[col].editIndicator)
				break;
			
			indicatorR.topLeft.x += tableP->columnAttrs[col].width + 
		                			   tableP->columnAttrs[col].spacing;
			}	
		}

	// Compute the width of the edit indicator.
	for (; col < tableP->numColumns; col++)
		{
		if (tableP->columnAttrs[col].usable)
			{
			if (!tableP->columnAttrs[col].editIndicator)
				{
				if (col)
					indicatorR.extent.x -= tableP->columnAttrs[col-1].spacing + lastColSpacing;
				break;
				}
				
			indicatorR.extent.x += tableP->columnAttrs[col].width +
										  tableP->columnAttrs[col].spacing;
			}
		else
			lastColSpacing = tableP->columnAttrs[col].spacing;
		}

	if (selected)
		ReplaceTwoColors (&indicatorR, 3,
			UIObjectForeground, UIFieldBackground, UIObjectSelectedForeground, UIObjectSelectedFill);
		
	// unselecting -- have to redraw each item
	else {
		WinPushDrawState();
		WinSetBackColor(UIColorGetIndex(UIFieldBackground));
		WinSetForeColor(UIColorGetIndex(UIObjectForeground));
		WinSetTextColor(UIColorGetIndex(UIObjectForeground));
		
		// re-use indicatorR.topLeft.x as the left edge of each column
		indicatorR.topLeft.x = tableP->bounds.topLeft.x;

		for (col = 0; col < tableP->numColumns; col++) {
			if (tableP->columnAttrs[col].usable) {
				if (tableP->columnAttrs[col].editIndicator) {
					DrawItem(tableP, tableP->currentRow, col,
								&indicatorR, false);
					drawn = true;
				}
				else
					if (drawn)
						break;
				indicatorR.topLeft.x += tableP->columnAttrs[col].width + 
			                			   tableP->columnAttrs[col].spacing;
			}	
		}
		
		WinPopDrawState();
	}
}

/***********************************************************************
 *
 * FUNCTION: 	 PrvFormBgColor
 *
 * DESCRIPTION: Returns the right background color for the form,
 *              based on the form's characteristics.
 *
 * PARAMETERS:	 formP - the form in question
 *					
 * RETURNED:	 the background color index
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			daf	11/5/99	Initial Revision
 *
 ***********************************************************************/
static IndexedColorType PrvFormBgColor(FormType * formP)
{
	UIColorTableEntries backColor;
	
	ErrNonFatalDisplayIf(!formP, "Drawing table without an active form");
	
	if (formP->formId == CustomAlertDialog) {
		backColor = UIAlertFill;
	}
	else if (formP->window.windowFlags.modal) {
		backColor = UIDialogFill;
	}
	else {
		backColor = UIFormFill;
	}

	return UIColorGetIndex(backColor);
}


/***********************************************************************
 *
 * FUNCTION:    DrawTable
 *
 * DESCRIPTION: This routine draws the specified part of the table.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *					 drawOnlyInvalid - true to draw only invalid items 
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	2/22/99	Merged TblDrawTable and TblRedrawTable through
 *								this, added support for color.
 *			jmp	11/05/99	Initialize erase variable to false -- we were just
 *								getting "lucky" that it was usually false (zero)!
 *			jmp	11/08/99	Set the fore and text colors here, in addition to
 *								the background color that was already being set,
 *								as almost everyone who was eventually calling this
 *								routine was having to set these colors themselves
 *								anyway (or was just getting the colors wrong!).
 *
 ***********************************************************************/
static void DrawTable (TableType * tableP, Boolean drawOnlyInvalid)
{
	Int16				index;
	Int16				row, col;
	Boolean			redrawEditIndicator = false;
	TableItemType  *item;
	RectangleType	r;
	RectangleType	clipR;
	Boolean			erase = false;

	if (!tableP->attr.visible)
		return;
	
	WinPushDrawState();
	FntSetFont(stdFont);
	
	// Tables are drawn with field background, just for looks.  Set the other
	// colors as well (otherwise, everyone else will have to!).
	WinSetBackColor(UIColorGetIndex(UIFieldBackground));
	WinSetForeColor(UIColorGetIndex(UIObjectForeground));
	WinSetTextColor(UIColorGetIndex(UIObjectForeground));
	if (drawOnlyInvalid || (PrvFormBgColor(FrmGetActiveForm()) != UIColorGetIndex(UIFieldBackground)))
		erase = true;
	
	WinGetClip (&clipR);
	RctGetIntersection (&tableP->bounds, &clipR, &r);
	WinSetClip (&r);

	// If we're editing the current row, turn off the insertion point.
	// DOLATER kwk - check this mod with Roger & Bob...Art's change conflicts.
	// also unlock the field handle in case other rows of the table use the
	// same text handle (which results in an overlock error)
	if (tableP->attr.editing &&
		 tableP->rowAttrs[tableP->currentRow].usable &&
		 tableP->rowAttrs[tableP->currentRow].invalid) {
		InsPtEnable (false);
		redrawEditIndicator = true;
	}
					
	r.topLeft.y = tableP->bounds.topLeft.y;
	
	for (row = tableP->topRow; row < tableP->numRows; row++) {
		if (tableP->rowAttrs[row].usable) {
			if (!drawOnlyInvalid || tableP->rowAttrs[row].invalid) {
				tableP->rowAttrs[row].invalid = false;
				
				// If we're editing the current row, turn off the insertion point.
				if (tableP->attr.editing && (row == tableP->currentRow)) {
					RectangleType fieldR;
					
					// Check if the row being edited has moved vertically but the 
					// field's postition has not.  Correct it if so.
					FldGetBounds(&tableP->currentField, &fieldR);
					if (r.topLeft.y != fieldR.topLeft.y) {
						fieldR.topLeft.y = r.topLeft.y;
						FldSetBounds (&tableP->currentField, &fieldR);
					}
				}
					
				// re-set rectangle to match current row
				r.topLeft.x = tableP->bounds.topLeft.x;
				r.extent.x = tableP->bounds.extent.x;
				r.extent.y = tableP->rowAttrs[row].height;

				// erase the row to the proper background color if necessary
				if (erase)
					WinEraseRectangle(&r, 0);

				// Draw the items in the row
				for (col = 0; col < tableP->numColumns; col++) {
					if (tableP->columnAttrs[col].usable) {	
						r.extent.x = tableP->columnAttrs[col].width;
						DrawItem (tableP, row, col, &r, false);
						r.topLeft.x += r.extent.x + tableP->columnAttrs[col].spacing;
					}
				}
			} // draw | invalid

			r.topLeft.y += tableP->rowAttrs[row].height;
		} // usable
	}

	// Erase the region below the last row drawn.
	r.extent.y = tableP->bounds.extent.y - (r.topLeft.y - tableP->bounds.topLeft.y);
	if (r.extent.y > 0) {
		r.topLeft.x = tableP->bounds.topLeft.x;
		r.extent.x = tableP->bounds.extent.x;
		WinEraseRectangle (&r, 0);
	}

	// Make sure the field bounds are correct.
	if (tableP->attr.editing &&
		 tableP->rowAttrs[tableP->currentRow].usable) {
		TblGetItemBounds (tableP, tableP->currentRow, tableP->currentColumn, &r);
		
		// If the item has a note, leave space for the note indicator.
		// If the application has reserved space to the rigth of the text item,
		// leave space for it.
		index = tableP->currentRow * tableP->numColumns + tableP->currentColumn;
		item = item = &tableP->items[index];
		if (item->itemType == textWithNoteTableItem)
			r.extent.x -= tableNoteIndicatorWidth;
		else if (item->itemType == narrowTextTableItem)
			r.extent.x -= item->intValue;
		
		FldSetBounds (&tableP->currentField, &r);
	}
		
	// display insertion point, locking, and the edit indicator.	
	if (redrawEditIndicator) {
		DrawEditIndicator (tableP, true);
		InsPtEnable (true);
	}

	WinSetClip (&clipR);	
	WinPopDrawState();
}


/***********************************************************************
 *
 * FUNCTION:    SendTableSelectEvent
 *
 * DESCRIPTION: This routine add a tableP select eventP to the eventP queue.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              row    - row selected
 *              column - column selected
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/9/95	Initial Revision
 *			roger	1/13/97	Added playSound
 *			roger	11/12/97	Don't play sound when a fldEnterEvent will be generated
 *			CS		08/15/00	Initialize new event structure.
 *			peter	1/8/01	Play sound for masked row since fldEnterEvent won't be generated.
 *
 ***********************************************************************/
static void SendTableSelectEvent (TableType * tableP, Int16 row, Int16 column,
	Boolean playSound)
{
	EventType eventP;
	TableItemStyleType itemStyle;
	
	MemSet(&eventP, sizeof(eventP), 0);	// Initialize new event structure
	eventP.eType = tblSelectEvent;
	eventP.data.tblSelect.tableID = tableP->id;
	eventP.data.tblSelect.pTable = tableP;
	eventP.data.tblSelect.row = row;
	eventP.data.tblSelect.column = column;
	EvtAddEventToQueue (&eventP);
	
	if (playSound)
		{
		itemStyle = tableP->items[row*tableP->numColumns + column].itemType;

		if (tableP->rowAttrs[row].masked ||
			itemStyle != textTableItem &&
			itemStyle != textWithNoteTableItem &&
			itemStyle != narrowTextTableItem)
			{
			SndPlaySystemSound(sndClick);
			}
		}
}


/***********************************************************************
 *
 * FUNCTION:    SendTableExitEvent
 *
 * DESCRIPTION: This routine add a tableP exit eventP to the eventP queue.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              row    - row selected
 *              column - column selected
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/29/95	Initial Revision
 *			CS		08/15/00	Initialize new event structure.
 *
 ***********************************************************************/
static void SendTableExitEvent (TableType * tableP, Int16 row, Int16 column)
{
	EventType eventP;
	
	MemSet(&eventP, sizeof(eventP), 0);	// Initialize new event structure
	eventP.eType = tblExitEvent;
	eventP.data.tblExit.tableID = tableP->id;
	eventP.data.tblExit.pTable = tableP;
	eventP.data.tblExit.row = row;
	eventP.data.tblExit.column = column;
	EvtAddEventToQueue (&eventP);
}


/***********************************************************************
 *
 * FUNCTION:    PointInTableItem
 *
 * DESCRIPTION: This routine determine which, if any, item the passed
 *              coordinate is on.
 *
 * PARAMETERS:	 tableP   - pointer to a table object
 *              x       - coordinate
 *              y       - coordinate
 *              rowP    - row of the item (return value)
 *              colP - column of the item (return value)
 *              rP   - bounds of the item (return value)
 *
 * RETURNED:	 true if the coordinate is on an item.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/7/95	Initial Revision
 *			peter	5/26/00	Add support for multi-line masked records.
 *
 ***********************************************************************/
static Boolean PointInTableItem (TableType * tableP, Coord x, Coord y,
	Int16 * rowP, Int16 * colP, RectangleType * rP)
{
	Int16 row, col;
	FontID curFont;
	TableItemType * item;
	RectangleType r;
	Boolean masked;

	if (! RctPtInRectangle (x, y, &tableP->bounds)) return (false);

	r.topLeft.x = tableP->bounds.topLeft.x;
	r.topLeft.y = tableP->bounds.topLeft.y;
	r.extent.x = tableP->bounds.extent.x;
		
	for (row = tableP->topRow; row < tableP->numRows; row++)
		{
		if (! tableP->rowAttrs[row].usable)
			continue;
		
		// Is the point within the bounds of the row.
		r.extent.y = tableP->rowAttrs[row].height;
		if (RctPtInRectangle (x, y, &r))
			{
			if (! tableP->rowAttrs[row].selectable)
				return (false);
				
			for (col = 0; col < tableP->numColumns; col++)
				{
				if (! tableP->columnAttrs[col].usable) 
					continue;
					
				// Is the point within the bounds of the column
				r.extent.x = tableP->columnAttrs[col].width;
				r.extent.y = tableP->rowAttrs[row].height;
				if (RctPtInRectangle (x, y, &r))
					{
					// If the item is not a text item or a tall custom item, and is not masked,
					// check if the point is in the first line of the column.
					item = &tableP->items[row*tableP->numColumns+col];
					masked = tableP->rowAttrs[row].masked && tableP->columnAttrs[col].masked;
					if (!masked &&
						  (item->itemType != textTableItem) &&
						  (item->itemType != textWithNoteTableItem) &&
						  (item->itemType != narrowTextTableItem) &&
						  (item->itemType != tallCustomTableItem))
						{
						curFont = FntSetFont (tableFont);
						r.extent.y = FntLineHeight ();
						FntSetFont (curFont);
				
						if (! RctPtInRectangle (x, y, &r))
							return (false);
						}

					RctCopyRectangle (&r, rP);
					*rowP = row;
					*colP = col;
					return (true);
					}
					
				// Move to the next column.
				r.topLeft.x += r.extent.x + tableP->columnAttrs[col].spacing;
				}

			// We were in a useable row, but we were not a any of the columns.
			return (false);
			}

		// Move to next row.
		r.topLeft.y += tableP->rowAttrs[row].height;
		}

	return (false);
}

/***********************************************************************
 *
 * FUNCTION: 	 SelectItem
 *
 * DESCRIPTION: This routine is called when a pen down eventP occurs in
 *              a table object that is selectable (selectable items 
 *              highlight when they are pressed).  This routine will track
 *              the pen until it is released.
 *
 * PARAMETERS:	 tableP - pointer to a table object
 *					 eventP - the event that started it all
 *					 row	  - row of item to select
 *					 column - col of item to select
 *              rP     - pointer to the bounds of an item
 *					
 * RETURNED:	 true if the pen is release within the bounds of the item
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/11/95	Initial Revision
 *			bob	10/28/99	Changed drawing to support colorization
 *			bob	6/5/00	Move early exit test to evade WinPushDrawState
 *
 ***********************************************************************/
static Boolean SelectItem (TableType * tableP, EventType * eventP, Int16 row, 
	Int16 column, RectangleType * rP)
{
	Coord x, y;
	Boolean penDown = true;
	Boolean selected = true;
		
	ErrNonFatalDisplayIf (row >= tableP->numRows, "Invalid tableP row");
	ErrNonFatalDisplayIf (column >= tableP->numColumns, "Invalid tableP column");
	ErrNonFatalDisplayIf (rP == NULL, "NULL bounds");
	
	if (! RctPtInRectangle (eventP->screenX, eventP->screenY, rP))
		return false;

	WinPushDrawState();
	WinSetBackColor(UIColorGetIndex(UIFieldBackground));
	WinSetForeColor(UIColorGetIndex(UIObjectForeground));
	WinSetTextColor(UIColorGetIndex(UIObjectForeground));

	DrawItem(tableP, row, column, rP, true);

	do {
		PenGetPoint (&x, &y, &penDown);

		if (RctPtInRectangle (x, y, rP)) {
			if (! selected)
				DrawItem(tableP, row, column, rP, selected = true);
		}

		else if (selected) {
			DrawItem(tableP, row, column, rP, selected = false);
		}
	} while (penDown);
		

	if (selected) {
		// draw item as selected, post a table select event
		tableP->attr.selected = true;
		tableP->currentRow = row;
		tableP->currentColumn = column;
		SendTableSelectEvent (tableP, row, column, true);
	}
	else {
		// Post a tableExitEvent.
		SendTableExitEvent (tableP, row, column);
		
		// If a new item was not selected, highlight the prior selection.
		if (tableP->attr.selected)
			TblSelectItem (tableP, tableP->currentRow, tableP->currentColumn);
	}
		
	WinPopDrawState();

	return (selected);
}


/***********************************************************************
 *
 * FUNCTION: 	 SelectMaskedRow
 *
 * DESCRIPTION: This routine is called when a pen down eventP occurs in
 *              a masked row of a table object that is selectable (selectable items 
 *              highlight when they are pressed).  This routine will track
 *              the pen until it is released.
 *
 * PARAMETERS:	 tableP - pointer to a table object
 *					 eventP - event that started it all
 *              row    - (masked) row to select
 *              column - (masked) column that the pen originally went down in
 *					
 * RETURNED:	 true if the pen is release within the bounds of the item
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	10/28/99	Initial Revision
 *			peter	05/22/00	Clip selection effect to table bounds to support multi-line masked records
 *
 ***********************************************************************/
static Boolean SelectMaskedRow (TableType * tableP, EventType * eventP, Int16 row, Int16 column)
{
	Coord x, y;
	Boolean penDown = true;
	Boolean selected = true;
	RectangleType rowR = tableP->bounds;
	Int16 i;
	Boolean wasItemSelected = tableP->attr.selected;

	
	ErrNonFatalDisplayIf (row >= tableP->numRows, "Invalid tableP row");
	
	WinPushDrawState();
	WinSetBackColor(UIColorGetIndex(UIFieldBackground));
	WinSetForeColor(UIColorGetIndex(UIObjectForeground));
	WinSetTextColor(UIColorGetIndex(UIObjectForeground));
	
	// Find the vertical bound of the item.
	for (i = tableP->topRow; i < row; i++)
		if (tableP->rowAttrs[i].usable)
			rowR.topLeft.y += tableP->rowAttrs[i].height;
	rowR.extent.y = tableP->rowAttrs[row].height;
	
	// Clip the row bounds to the table bounds to limit the selection
	// effect from going off the bottom of the table if a multi-line
	// masked record is partially visible at the bottom of the screen.
	if (rowR.topLeft.y + rowR.extent.y > tableP->bounds.topLeft.y + tableP->bounds.extent.y)
		rowR.extent.y = tableP->bounds.topLeft.y + tableP->bounds.extent.y - rowR.topLeft.y;
 
	if (! RctPtInRectangle (eventP->screenX, eventP->screenY, &rowR))
		return (false);

	// draw row as selected
	DrawRow(tableP, row, &rowR, true);

	do {
		PenGetPoint (&x, &y, &penDown);

		if (RctPtInRectangle (x, y, &rowR)) {
			if (! selected)
				DrawRow(tableP, row, &rowR, selected = true);
		}

		else if (selected) {
			WinEraseRectangle(&rowR, 0);
			DrawRow(tableP, row, &rowR, selected = false);
		}
	} while (penDown);
		

	if (selected)
	{
		// don't draw as selected, but send an event
		tableP->attr.selected = false;
		WinEraseRectangle(&rowR, 0);
		DrawRow(tableP, row, &rowR, false);
		SendTableSelectEvent (tableP, row, column, true);
	}
	else
		// Post a tableExitEvent.
		SendTableExitEvent (tableP, row, column);
	
	WinPopDrawState();

	// highlight the prior selection -- if there was one
	if (wasItemSelected)
		TblSelectItem (tableP, tableP->currentRow, tableP->currentColumn);

	return (selected);
}


/***********************************************************************
 *
 * FUNCTION:    CustomItemHandleEvent
 *
 * DESCRIPTION: This routine handles events in custom tableP items.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/7/95	Initial Revision
 *
 ***********************************************************************/
static void CustomItemHandleEvent (TableType * tableP, EventType * eventP,
	Int16 row, Int16 column, RectangleType * rP)
{
	SelectItem (tableP, eventP, row, column, rP);
}


/***********************************************************************
 *
 * FUNCTION:    PopupTriggerHandleEvent
 *
 * DESCRIPTION: This routine handles eventP in a popup trigger tableP item.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/7/95	Initial Revision
 *
 ***********************************************************************/
static void PopupTriggerHandleEvent (TableType * tableP, EventType * eventP,
	Int16 row, Int16 column, RectangleType * rP)
{
	ControlType ctl;
	ListType * lst;
	EventType nextEvent;
	Char * textP;
	Int16 newSelection;
	TableItemType * item;
	Char colonChar;
	FontID currFont;
	
	
	item = &tableP->items[row*tableP->numColumns + column];
	
	ErrFatalDisplayIf(item->itemType != popupTriggerTableItem, "Bad tableP popup item");

	// Create a control object.  Use it now to handle the control selection.
	// We might use it later to draw the control if it changes (list selection).
	MemSet (&ctl, sizeof(ControlType), 0);
	ctl.attr.usable = true;
	ctl.attr.enabled = true;
	ctl.attr.visible = true;
	ctl.style = popupTriggerCtl;
	RctCopyRectangle (rP, &ctl.bounds);
	
	// Setting the text is tricky because the text is stored within
	// a list in the item.  
	lst = (ListType *) item->ptr;
	CtlSetLabel (&ctl, LstGetSelectionText (lst, item->intValue));


	// Now feed the tableSelectEvent into the control.  It will handle
	// the eventP only if it is within the bounds of the control.
	eventP->eType = penDownEvent;		// so the control will handle it
	if (CtlHandleEvent(&ctl, eventP))
		{
		while (true)
			{
			EvtGetEvent (&nextEvent, evtWaitForever);

			if (nextEvent.eType == ctlEnterEvent)
				CtlHandleEvent(&ctl, &nextEvent);

			else if (nextEvent.eType == ctlSelectEvent)
				{
				// Display the popup list.
				LstSetPosition (lst, rP->topLeft.x, rP->topLeft.y);
				LstSetSelection (lst, item->intValue);
				newSelection = LstPopupList (lst);

				// If not selection was made or the currently selected item 
				// was selected again,  don't do anytime else.
				if (newSelection == noListSelection) break;

				// Change the label of the tableP item.
				textP = LstGetSelectionText (lst, newSelection);
				CtlEraseControl (&ctl);
				CtlSetLabel (&ctl, textP);
				CtlDrawControl (&ctl);
				currFont = FntSetFont(stdFont);
				colonChar = ':';
				WinDrawChars(&colonChar, 1, ctl.bounds.topLeft.x + 
					ctl.bounds.extent.x - FntCharWidth(':'), 
					ctl.bounds.topLeft.y);
				FntSetFont (currFont);


				// Save the items selected in the tableP item's data.
				item->intValue = newSelection;

				// Post a tableSelectEvent.
				SendTableSelectEvent (tableP, row, column, false);
				break;
				}

			else if (nextEvent.eType == ctlExitEvent)
				{
				// Post a tableExitEvent.
				SendTableExitEvent (tableP, row, column);
				}
			else
				{
				// If we got an eventP that we didn't expect, put it back in the 
				// queue.
				EvtAddEventToQueue (&nextEvent);
				break;
				}
			}
		}
}


/***********************************************************************
 *
 * FUNCTION:    CheckboxHandleEvent
 *
 * DESCRIPTION: This routine handles eventP in a checkbox tableP item.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/7/95	Initial Revision
 *
 ***********************************************************************/
static void CheckboxHandleEvent (TableType * tableP, EventType * eventP,
	Int16 row, Int16 column, RectangleType * rP)
{
	ControlType ctl;
	EventType newEvent;
	TableItemType * item;
	
	item = &tableP->items[row*tableP->numColumns + column];
				
	InitCheckboxItem (&ctl, rP->topLeft.x, rP->topLeft.y, 
		tableP->columnAttrs[column].width, item->intValue != 0);
	ctl.attr.visible = true;
	
	// Convert the tableP enter event to a field enter event.
	newEvent = *eventP;
	newEvent.eType = ctlEnterEvent;
	newEvent.data.ctlEnter.controlID = ctl.id;
	newEvent.data.ctlEnter.pControl = &ctl;

	while (true)
		{
		if (newEvent.eType == ctlEnterEvent)
			CtlHandleEvent (&ctl, &newEvent);

		else if (newEvent.eType == ctlSelectEvent)
			{
			item->intValue = CtlGetValue (&ctl);
			SendTableSelectEvent (tableP, row, column, true);
			}

		else if (newEvent.eType == ctlExitEvent)
			{
			// Post a tableExitEvent.
			SendTableExitEvent (tableP, row, column);
			}
				
		else
			{
			// If we got an event that we didn't expect, put it back in the 
			// queue.
			EvtAddEventToQueue (&newEvent);
			break;
			}
						
		EvtGetEvent (&newEvent, evtWaitForever);
		}
}


/***********************************************************************
 *
 * FUNCTION:    SaveTextItem
 *
 * DESCRIPTION: This routine checks if the focus is being changed to
 *              another item of the tableP.  If it is and the item
 *              that currently has the focus is a text item, the 
 *              save data callback routine is executed.
 * 
 * PARAMETERS:	 tableP  - pointer to a table object
 *
 * RETURNED:	 true if the tableP should be redrawn
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/5/95	Initial Revision
 *
 ***********************************************************************/
static Boolean SaveTextItem (TableType * tableP)
{
	Int16 index;
	Boolean redraw = false;

	// If we're not editing, return
	if (! tableP->attr.editing)
		return (false);

	// If the item that has the focus is not a text item, returm.
	index = tableP->currentRow * tableP->numColumns + tableP->currentColumn;
	if ((tableP->items[index].itemType != textTableItem) &&
		 (tableP->items[index].itemType != textWithNoteTableItem) &&
		 (tableP->items[index].itemType != narrowTextTableItem))
		return (false);
		

	// Release any extra memory the field routines may have allocated 
	// for editing the text string.
	FldCompactText (&tableP->currentField);


	// Call the application to have it save the current text item.
	if (tableP->columnAttrs[tableP->currentColumn].saveDataCallback)
		{
		redraw = tableP->columnAttrs[tableP->currentColumn].saveDataCallback (
			tableP, tableP->currentRow, tableP->currentColumn);
		}


	// Invert the region to the left of the text item, this highlighted
	// region indicated that the tableP has in edit mode.
	FldReleaseFocus (&tableP->currentField);
	FldSetSelection (&tableP->currentField, 0, 0);
	DrawEditIndicator (tableP, false);


	// We're no longer editing the field, so lets clean up, clear the 
	// editing flag and turn off the inserting point or selection and 
	// release the memory allocated for the field.  Do not free the 
	// block the contains the string we edited.  
	tableP->attr.editing = false;
	FldSetTextHandle (&tableP->currentField, 0);
	FldFreeMemory (&tableP->currentField);
	
	// Set noFocus since there is no longer any field to send text to.
	// Don't do this, at least not yet.  It turns out that this side effect
	// is relied upon heavily in at least most of Palm's PIM apps, if not most 
	// third party apps with tables.	Roger 11/17/99
//	FrmSetFocus(FrmGetActiveForm(), noFocus);
	
	if (redraw)
		TblRedrawTable (tableP);

	return (redraw);
}


/***********************************************************************
 *
 * FUNCTION:    SelectTextItem
 *
 * DESCRIPTION: This routine is called when the table object receives a 
 *              penDown eventP that is within the bounds of a text item.
 *              If the item that the pen is on is not the item that 
 *              is currently being edited,  a field obejct is 
 *              initialized for editing the text item.
 *
 * PARAMETERS:	 tableP   - pointer to a table object
 *              eventP  - a penDawn eventP
 *              item    - a pointer to the tableP item the pen is on
 *              row     - row of the  item
 *              column  - column of the item
 *              rP       - bounds of the item
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/7/95	Initial Revision
 *			rbb	5/18/99	Added support for multiple taps
 *			gap	9/12/99	update for new multi-tap implementation
 *
 ***********************************************************************/
static void SelectTextItem (TableType * tableP, EventType * eventP,
	Int16 row, Int16 column, RectangleType * rP)
{
	Err err;
	FieldType * fldP;
	EventType newEvent;
	TableItemType * item;
	RectangleType indicatorR;
	
	ErrNonFatalDisplayIf(tableP->rowAttrs[row].masked, "Can't edit masked row");
	ErrNonFatalDisplayIf ((row >= tableP->numRows) || (column >= tableP->numColumns), "Invalid tableP column");
	
	fldP = &tableP->currentField;

	// Check if the pen is on the note indicator.
	item = &tableP->items[row*tableP->numColumns + column];
	if (item->itemType == textWithNoteTableItem)
		{
		indicatorR.topLeft.x = rP->topLeft.x + rP->extent.x - tableNoteIndicatorWidth;
		indicatorR.topLeft.y = rP->topLeft.y;
		indicatorR.extent.x = tableNoteIndicatorWidth;
		indicatorR.extent.y = tableNoteIndicatorHeight;
		
		if (RctPtInRectangle (eventP->screenX, eventP->screenY, &indicatorR))
			{
			SaveTextItem (tableP);
			SelectItem (tableP, eventP, row, column, &indicatorR);
			return;
			}
		}
		

	// If the item is a nerrow-text-item there is some space, in the column,
	// reserved by the application for drawing sometime other then text.  The
	// width of reserved space is store in "intValue".  Check if the pen is
	// in the reserved space.
	else if (item->itemType == narrowTextTableItem)
		{
		indicatorR.topLeft.x = rP->topLeft.x + rP->extent.x - item->intValue;
		indicatorR.topLeft.y = rP->topLeft.y;
		indicatorR.extent.x = item->intValue;
		indicatorR.extent.y = rP->extent.y;		
		if (RctPtInRectangle (eventP->screenX, eventP->screenY, &indicatorR))
			return;
		}


	// If we are not already editing this item, initialize the field
	// structure that's used for editing text item and highlight the edit
	// indication.
	if ((row != tableP->currentRow) || 
		 (column != tableP->currentColumn) ||
		 (! tableP->attr.editing) )
		{
		err = InitTextItem (tableP, fldP, row, column, rP->topLeft.x, 
			rP->topLeft.y, true, true);

		// If the field does not have a text handle then the initializition
		// must have failed.
		if (err)
			{
			tableP->attr.editing = false;
			return;
			}
	
		// tableP->attr.editing = true;		// InitTextItem sets editing to true
		tableP->currentRow = row;
		tableP->currentColumn = column;
		
		DrawEditIndicator (tableP, true);
		}

	SendTableSelectEvent (tableP, row, column, true);
	
	// Convert the tableP enter eventP to a field enter eventP.
	newEvent = *eventP;
	newEvent.eType = fldEnterEvent;
	newEvent.data.fldEnter.fieldID = fldP->id;
	newEvent.data.fldEnter.pField = fldP;

	FldHandleEvent (fldP, &newEvent);
}


/***********************************************************************
 *
 * FUNCTION:    ResizeTextItem
 *
 * DESCRIPTION: This routine expands or compresses the height of a text 
 *              item, it is called when the table object receives a 
 *              fldHeightChanged eventP.  
 *
 * PARAMETERS:	 tableP   - pointer to a table object
 *              eventP  - a penDawn eventP
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/16/95	Initial Revision
 *			art	8/28/97	Add code to erase partially visible items
 *			bob	1/27/99	If partially visible item is multi-line text,
 *								erase only lines that scrolled off
 *
 ***********************************************************************/
static void ResizeTextItem (TableType * tableP, EventType * eventP)
{
	Int16 scrollAmount;
	Int16 row;
	Coord height;
	Coord maxHeight;
	Coord newHeight;
	Boolean insPtState;
	RectangleType r, vacated, fldR, newBounds, itemR, scrollR;

	// get ready for erasing
	WinPushDrawState();
	WinSetBackColor(UIColorGetIndex(UIFieldBackground));

	insPtState = InsPtEnabled ();
	InsPtEnable (false);

	newHeight = eventP->data.fldHeightChanged.newHeight;

	RctCopyRectangle (&tableP->currentField.rect, &fldR);
	RctCopyRectangle (&fldR, &newBounds);
	newBounds.extent.y = newHeight;

	// The height of the field can not expand below the botton of the tableP.
	maxHeight = tableP->bounds.topLeft.y + tableP->bounds.extent.y - 
		fldR.topLeft.y;
	
	
	// Expanding the height of a text item, scroll the tableP items above the
	// current text item up, the top items will be forced off the display.
	if (newHeight > maxHeight)
		{
		scrollAmount = 0;
		while (tableP->currentRow)
			{
			scrollAmount += tableP->rowAttrs[0].height;

			// Remove the top row from the tableP.
			TblRemoveRow (tableP, 0);
			tableP->currentRow--;

			if (scrollAmount >= (newHeight - maxHeight))
				break;
			}

		if (scrollAmount) {
			// Scroll the tableP up by the height of the item scrolled off the top.
			RctCopyRectangle (&tableP->bounds, &scrollR);
			scrollR.extent.y = fldR.topLeft.y - tableP->bounds.topLeft.y + fldR.extent.y;
			WinScrollRectangle (&scrollR, winUp, scrollAmount, &vacated);

			// Erase from the bottom of the region scrolled to the end of the tableP.
			vacated.extent.y = tableP->bounds.extent.y - (vacated.topLeft.y - 
				tableP->bounds.topLeft.y);
			// WinSetBackColor(...);
			
			WinEraseRectangle (&vacated, 0);
			
			// Offset the new field bounds by the amount we scrolled the tableP.
			newBounds.topLeft.y -= scrollAmount;
			
			// Set the new height of the row.
			tableP->rowAttrs[tableP->currentRow].height = newHeight;
			}
			else if (newBounds.extent.y > maxHeight)
					newBounds.extent.y = maxHeight;
					
		// Invalidate the rows below the current row so that that will be
		// redrawn.
		for (row = tableP->currentRow+1; row < tableP->numRows; row++)
			{
			if (tableP->rowAttrs[row].usable)
				tableP->rowAttrs[row].invalid = true;
			}		
		}


	// Expanding the height of a text item, scoll the tableP items below the
	// text item down. 
	else if (newHeight > fldR.extent.y)
		{
		// Get the bounds of the region that encompasses all the items below the 
		// text item and erase it.
		r.topLeft.x = tableP->bounds.topLeft.x;
		r.topLeft.y = fldR.topLeft.y + fldR.extent.y;
		r.extent.x = tableP->bounds.extent.x;
		r.extent.y = tableP->bounds.extent.y - (r.topLeft.y - tableP->bounds.topLeft.y);

		scrollAmount = newHeight - fldR.extent.y;
		WinScrollRectangle (&r, winDown, scrollAmount, &vacated);
		WinEraseRectangle (&vacated, 0);

		// Set the new height of the row.
		tableP->rowAttrs[tableP->currentRow].height += scrollAmount;

		// Invalidate any items that were forced off the display.
		height = 0;
		for (row = 0; row < tableP->numRows; row++)
			{
			if (tableP->rowAttrs[row].usable)
				{
				if (height >= tableP->bounds.extent.y)
					{
					tableP->rowAttrs[row].invalid = true;
					tableP->rowAttrs[row].usable = false;
					}

				// If an item is partially visible, handle it.
				else if (height + tableP->rowAttrs[row].height > tableP->bounds.extent.y)
					{
					// bob 1/27/99 -- if item contains a multi-line field, try reducing
					// 				   the number of visible lines
					Boolean foundText = false;
					Int16 col;

					// figure out how much can be visible
					maxHeight = tableP->bounds.extent.y - height;
					
					// reduce height to integral number of rows of text
					for (col = 0; col < tableP->numColumns; col++)
						{
						TableItemType * itemP = &tableP->items[row*tableP->numColumns + col];
						if (itemP->itemType == textTableItem
						|| itemP->itemType == textWithNoteTableItem 
						||	itemP->itemType == narrowTextTableItem)
							{
							FontID oldFont = FntSetFont(itemP->fontID);
							Int16 visibleLines = maxHeight / FntLineHeight();
							maxHeight = visibleLines * FntLineHeight();
							FntSetFont(oldFont);
							foundText = true;		
							}
						}
					
					// if maxHeight is non-zero, then some fields were still
					// visible, so just erase any partial line at the bottom
					// and reduce the height of the row
					if (foundText && maxHeight > 0)
						{
						r.topLeft.x = tableP->bounds.topLeft.x;
						r.extent.x = tableP->bounds.extent.x;
						r.topLeft.y = tableP->bounds.topLeft.y + height + maxHeight;
						r.extent.y = tableP->bounds.extent.y - height - maxHeight;
						WinEraseRectangle (&r, 0);
						tableP->rowAttrs[row].height = maxHeight;
						}
					
					// otherwise, no text or no rows visible, erase the
					// whole row and mark it invalid
					else
						{
						r.topLeft.x = tableP->bounds.topLeft.x;
						r.topLeft.y = tableP->bounds.topLeft.y + height;
						r.extent.x = tableP->bounds.extent.x;
						r.extent.y = tableP->bounds.extent.y - height;
						WinEraseRectangle (&r, 0);
						
						tableP->rowAttrs[row].invalid = true;
						tableP->rowAttrs[row].usable = false;
						}
					}
				
				height += tableP->rowAttrs[row].height;
				}
			}
		}
		
	// Compressing the height of a text item, scoll the tableP items below the
	// text item up. 
	else if (newHeight < fldR.extent.y)
		{
		// Get the bounds of the region that encompasses all the items below the 
		// text item.
		r.topLeft.x = tableP->bounds.topLeft.x;
		r.topLeft.y = fldR.topLeft.y + newHeight;
		r.extent.x = tableP->bounds.extent.x;
		r.extent.y = tableP->bounds.extent.y - (r.topLeft.y - tableP->bounds.topLeft.y);

		scrollAmount = fldR.extent.y - newHeight;
		WinScrollRectangle (&r, winUp, scrollAmount, &vacated);
		WinEraseRectangle (&vacated, 0);

		tableP->rowAttrs[tableP->currentRow].height = newHeight;
		

		// If the last visible item is partially off the bottom of the display
		// mark the item invalid so that it will be redrawn.
		row = TblGetLastUsableRow (tableP);
		TblGetItemBounds (tableP, row, tableP->currentColumn, &itemR);
	
		if (itemR.topLeft.y + itemR.extent.y > 
			 tableP->bounds.topLeft.y + tableP->bounds.extent.y - scrollAmount)
			{
			TblMarkRowInvalid (tableP, row);	
			}
		}


	// Change the size of the field.
	FldSetBounds (&tableP->currentField, &newBounds);
	FldSetInsPtPosition (&tableP->currentField, 
		eventP->data.fldHeightChanged.currentPos);

	// Turn the insertion point back on.
	InsPtEnable (insPtState);

	WinPopDrawState();
}		


/***********************************************************************
 *
 * FUNCTION:    TblDrawTable
 *
 * DESCRIPTION: This routine draws a tableP.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/7/95	Initial Revision
 *       bob	2/22/99	Moved bulk of code into DrawTable for color.
 *
 ***********************************************************************/
void TblDrawTable (TableType * tableP)
{
	if (!tableP->attr.usable)
		return;
		
	tableP->attr.visible = true;
	DrawTable(tableP, false);
}


/***********************************************************************
 *
 * FUNCTION:    TblRedrawTable
 *
 * DESCRIPTION: This routine redraws the rows of the tableP that are marked
 *              invalid.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/7/95	Initial Revision
 *       art	5/21/96	Fixed bug when redrawing edit indicator.
 *			trev	08/12/97	made non modified passed variables constant
 *       art	09/23/98	Move logic that turns off insertion point
 *			roger	11/03/98	Fixup current field when current row moves.
 *       bob	2/22/99	Moved bulk of code into DrawTable for color.
 *
 ***********************************************************************/
void TblRedrawTable (TableType * tableP)
{
	if (!tableP->attr.usable)
		return;

	DrawTable(tableP, true);
}


/***********************************************************************
 *
 * FUNCTION:    TblEraseTable
 *
 * DESCRIPTION: This routine erases a table object.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/7/95	Initial Revision
 *
 ***********************************************************************/
void TblEraseTable (TableType * tableP)
{
	if (tableP->attr.usable)
		{
		WinEraseRectangle (&tableP->bounds, 0);
		tableP->attr.visible = false;
		tableP->attr.selected = false;
		}
}


/***********************************************************************
 *
 * FUNCTION:    TblHandleEvent
 *
 * DESCRIPTION:
 *
 * PARAMETERS:	tableP	-> pointer to a table object.
 *					eventP	-> ptr to event record.
 *
 * RETURNED:	 True if event was handled by table.
 *
 *	HISTORY:
 *		02/07/95	art	Created by Art Lamb
 *		09/04/99	kwk	Also do special processing for tsmFepModeEvent.
 *
 ***********************************************************************/
Boolean TblHandleEvent (TableType* tableP, EventType* eventP)
{
	Int16	row, column;
	RectangleType r;
	TableItemType * item;
	EventType newEvent;
	
	#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
		ECValidateTable (tableP);
	#endif

	if (! tableP->attr.editable) return (false);

	if ((eventP->eType == keyDownEvent)
	|| (eventP->eType == tsmFepButtonEvent)
	|| (eventP->eType == tsmFepModeEvent))
		{
		if (! tableP->attr.editing)
			return (false);
		else if (FldHandleEvent (&tableP->currentField, eventP))
			return (true);
		else if (eventP->data.keyDown.chr == confirmChr)
			{
			TblReleaseFocus (tableP);
			return (true);
			}
		}
		
	else if (eventP->eType == fldEnterEvent)
		{
		FldHandleEvent (&tableP->currentField, eventP);
		}
	
	else if ((eventP->eType == menuCmdBarOpenEvent) && (tableP->attr.editing))
		{
		// let the field decide whether to put buttons on the command bar or not.
		return FldHandleEvent (&tableP->currentField, eventP);
		}
		
	else if (eventP->eType == fldHeightChangedEvent)
		{
		ResizeTextItem (tableP, eventP);
		FldHandleEvent (&tableP->currentField, eventP);
		return (true);
		}
		
	else if (eventP->eType == penDownEvent)
		{
		if (PointInTableItem (tableP, eventP->screenX, eventP->screenY,
				 &row, &column, &r))
			{
			TblUnhighlightSelection (tableP);
		
			// If the focus is not being changed, and the current item is a 
			// text item,  save its data.
			if ((row != tableP->currentRow) || (column != tableP->currentColumn))
				SaveTextItem (tableP);

			newEvent = *eventP;
			newEvent.eType = tblEnterEvent;
			newEvent.data.tblEnter.tableID = tableP->id;
			newEvent.data.tblEnter.pTable = tableP;
			newEvent.data.tblEnter.row = row;
			newEvent.data.tblEnter.column = column;
			EvtAddEventToQueue (&newEvent);

			return (true);
			}

		else
			return (false);
		}			


	else if (eventP->eType == tblEnterEvent)
		{
		row = eventP->data.tblEnter.row;
		column = eventP->data.tblEnter.column;
		
		// Get the bounds of the table item.
		TblGetItemBounds (tableP, row, column, &r);
		
		if (!tableP->rowAttrs[row].masked)
			{

			item = &tableP->items[row*tableP->numColumns + column];
			switch (item->itemType)
				{
				case textTableItem:
				case textWithNoteTableItem:
				case narrowTextTableItem:
					SelectTextItem (tableP, eventP, row, column, &r);
					break;

				case checkboxTableItem:
					WinPushDrawState();
					WinSetBackColor(UIColorGetIndex(UIFieldBackground));
					CheckboxHandleEvent (tableP, eventP, row, column, &r);
					WinPopDrawState();
					break;

				case popupTriggerTableItem:
					WinPushDrawState();
					WinSetBackColor(UIColorGetIndex(UIFieldBackground));
					PopupTriggerHandleEvent (tableP, eventP, row, column, &r);
					WinPopDrawState();
					break;

				case customTableItem:
				case tallCustomTableItem:
					CustomItemHandleEvent (tableP, eventP, row, column, &r);
					break;
					
				case numericTableItem:
				case dateTableItem:
				case labelTableItem:
					SelectItem (tableP, eventP, row, column, &r);
					break;

				default:
					return (false);
				}
			}
		else
			SelectMaskedRow (tableP, eventP, row, column);
			
		}
	return (false);	
}


/***********************************************************************
 *
 * FUNCTION:    TbltGetItemBounds
 *
 * DESCRIPTION: This routine returns the bounds of an item in a table.
 *
 * PARAMETERS:	 tableP   - pointer to a table object
 *              row      - row of the item (zero based)
 *              column   - column of the item (zero based)
 *              rP       - pointer to a structure that will hold the bound
 *                        of the item
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/7/95	Initial Revision
 *			trev	08/12/97	made non modified passed variables constant
 *
 ***********************************************************************/
void TblGetItemBounds (const TableType * tableP, Int16 row, Int16 column, RectangleType * rP)
{
	Int16 i;
	FontID curFont;
	TableItemType * item;
	
	ErrFatalDisplayIf (column >= tableP->numColumns, "Invalid column");
	ErrFatalDisplayIf (row >= tableP->numRows, "Invalid row");

	rP->topLeft.x = tableP->bounds.topLeft.x;
	rP->topLeft.y = tableP->bounds.topLeft.y;

	// Find the top bound of the item.
	for (i = tableP->topRow; i < row; i++)
		{
		if (tableP->rowAttrs[i].usable)
			rP->topLeft.y += tableP->rowAttrs[i].height;
		}
		
	// Find the left bound of the item.
	for (i = 0; i < column; i++)
		{
		if (tableP->columnAttrs[i].usable)
			rP->topLeft.x += tableP->columnAttrs[i].width + 
			                tableP->columnAttrs[i].spacing;
		}
		
	rP->extent.x = tableP->columnAttrs[column].width;
	rP->extent.y = tableP->rowAttrs[row].height;
	
	item = &tableP->items[row*tableP->numColumns+column];
	if ((item->itemType != textTableItem) &&
		 (item->itemType != textWithNoteTableItem) && 
		 (item->itemType != narrowTextTableItem) &&
		 (item->itemType != tallCustomTableItem))
		{
		curFont = FntSetFont (item->fontID);
		rP->extent.y = FntLineHeight ();
		FntSetFont (curFont);
		}
}


/***********************************************************************
 *
 * FUNCTION:    TblUnhighlightSelection
 *
 * DESCRIPTION: This routine unhighlights the currently selected item.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/15/95	Initial Revision
 *
 ***********************************************************************/
 void TblUnhighlightSelection (TableType * tableP)
{
	RectangleType r;
	
	if (tableP->attr.usable && tableP->attr.selected)
		{
		if (tableP->attr.visible) {
			WinPushDrawState();
			WinSetBackColor(UIColorGetIndex(UIFieldBackground));
			WinSetForeColor(UIColorGetIndex(UIObjectForeground));
			WinSetTextColor(UIColorGetIndex(UIObjectForeground));
		
			if (tableP->rowAttrs[tableP->currentRow].masked)
				{
					// don't need to do anything, it seems
					// because selected can't be true for a masked row!
					ErrNonFatalDisplay("Can't have a selection in a masked row");
				}
			else
				{
				TblGetItemBounds (tableP, tableP->currentRow, tableP->currentColumn, &r);
				DrawItem(tableP, tableP->currentRow, tableP->currentColumn, &r, false);
				}
				
			WinPopDrawState();
			}
		tableP->attr.selected = false;
		}
}


/***********************************************************************
 *
 * FUNCTION:    TbltSelectItem
 *
 * DESCRIPTION: This routine selects (highlights) the item specified,
 *              if there is already a selected item, it is unhighlighed.
 *					 WILL NOT SELECT MASKED ITEMS
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              row    - row of the item to select (zero based)
 *              column - column of the item to select (zero based)
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/7/95	Initial Revision
 *			bob	10/28/99	Set up colors and push/pop draw state
 *			jmp	11/05/99	Before we unselect an item that has already been selected,
 *								make sure we start off with a clean slate by erasing first.
 *								Fixes bug #23293 (and probably others like it).								
 *
 ***********************************************************************/
void TblSelectItem (TableType * tableP, Int16 row, Int16 column)
{
	RectangleType r;
	
	if ((column >= tableP->numColumns) || (row >= tableP->numRows))
		{
		ErrNonFatalDisplay("Invalid parameter");
		return;
		}

	if (!(tableP->attr.visible  && tableP->attr.usable) )
		return;

	// set up colors for drawing
	WinPushDrawState();
	WinSetBackColor(UIColorGetIndex(UIFieldBackground));
	WinSetForeColor(UIColorGetIndex(UIObjectForeground));
	WinSetTextColor(UIColorGetIndex(UIObjectForeground));

	// unselect the old item (if any)
	if (tableP->attr.selected)
		{
		// Don't redraw if we're selecting the already selected item, avoiding flicker
		if (tableP->currentRow == row && tableP->currentColumn == column)
			goto Exit;
			
		TblGetItemBounds (tableP, tableP->currentRow, tableP->currentColumn, &r);
		DrawItem(tableP, tableP->currentRow, tableP->currentColumn, &r, false);
		}
	
	
	// draw the new items as selected, if it's not in a masked row
	if (tableP->rowAttrs[row].masked)
		tableP->attr.selected = false;
	else
		{
		tableP->currentRow = row;
		tableP->currentColumn	= column;

		TblGetItemBounds (tableP, row, column, &r);
		DrawItem(tableP, tableP->currentRow, tableP->currentColumn, &r, true);
		tableP->attr.selected = true;
		}

Exit:
	WinPopDrawState();
}


/***********************************************************************
 *
 * FUNCTION:    TblGetItemInt
 *
 * DESCRIPTION: This routine returned the interger value stores in a tableP
 *              item.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              row    - row of the item to select (zero based)
 *              column - column of the item to select (zero based)
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/7/95	Initial Revision
 *			trev	08/12/97	made non modified passed variables constant
 *
 ***********************************************************************/
Int16 TblGetItemInt (const TableType * tableP, Int16 row, Int16 column)
{
	ErrFatalDisplayIf ((column >= tableP->numColumns) || (row >= tableP->numRows), "Invalid parameter");

	return (tableP->items[row*tableP->numColumns + column].intValue);
}


/***********************************************************************
 *
 * FUNCTION:    TblSetItemInt
 *
 * DESCRIPTION: This routine set the interter value of the item specified.
 *              An application can store whatever it wants in the interter
 *              value of an item.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              row    - row of the item (zero based)
 *              column - column of the item (zero based)
 *              value  - any byte value
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/16/95	Initial Revision
 *
 ***********************************************************************/
 void TblSetItemInt (TableType * tableP, Int16 row, Int16 column, Int16 value)
 {
	if ((column >= tableP->numColumns) || (row >= tableP->numRows))
		{
		ErrNonFatalDisplay("Invalid parameter");
		return;
		}

	tableP->items[row*tableP->numColumns + column].intValue = value;
 }


/***********************************************************************
 *
 * FUNCTION:    TblGetItemPtr
 *
 * DESCRIPTION: This routine gets the pointer value of the item specified.
 *              An application can store whatever it wants in the pointer
 *              value of an item.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              row    - row of the item (zero based)
 *              column - column of the item (zero based)
 *              value  - any byte value
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	2/22/99	Initial Revision - added missing API
 *
 ***********************************************************************/
 void * TblGetItemPtr (const TableType * tableP, Int16 row, Int16 column)
 {
	ErrFatalDisplayIf (column >= tableP->numColumns, "Invalid column");
	ErrFatalDisplayIf (row >= tableP->numRows, "Invalid row");

	return tableP->items[row*tableP->numColumns + column].ptr;
 }


/***********************************************************************
 *
 * FUNCTION:    TblSetItemPtr
 *
 * DESCRIPTION: This routine set the pointer value of the item specified.
 *              An application can store whatever it wants in the pointer
 *              value of an item.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              row    - row of the item (zero based)
 *              column - column of the item (zero based)
 *              value  - any byte value
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/16/95	Initial Revision
 *
 ***********************************************************************/
 void TblSetItemPtr (TableType * tableP, Int16 row, Int16 column, void * value)
 {
	if ((column >= tableP->numColumns) || (row >= tableP->numRows))
		{
		ErrNonFatalDisplay("Invalid parameter");
		return;
		}

	tableP->items[row*tableP->numColumns + column].ptr = value;
 }


/***********************************************************************
 *
 * FUNCTION:    TblSetItemStyle
 *
 * DESCRIPTION: This routine set the style of the item specified.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              row    - row of the item (zero based)
 *              column - column of the item (zero based)
 *              type   - 
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/16/95	Initial Revision
 *       art	5/21/96	Set editIndicator to true for non-text columns
 *
 ***********************************************************************/
 void TblSetItemStyle (TableType * tableP, Int16 row, Int16 column, TableItemStyleType style)
 {
	if ((column >= tableP->numColumns) || (row >= tableP->numRows))
		{
		ErrNonFatalDisplay("Invalid parameter");
		return;
		}

	tableP->items[row*tableP->numColumns + column].itemType = style;
	
	// In version 2.0 I added a column attribute that indicates if a column
	// should be highlighted when we're in edit mode. To maintain
	// compatability with version 1.0 we initialize that attribute.
	if (style != textTableItem &&
		 style != textWithNoteTableItem &&
		 style != narrowTextTableItem)
		tableP->columnAttrs[column].editIndicator = true;
	else
		tableP->columnAttrs[column].editIndicator = false;
 }


/***********************************************************************
 *
 * FUNCTION:    TblGetItemFont
 *
 * DESCRIPTION: This routine returns thefont id stored in a the 
 *              specified tableP item.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              row    - row of the item to select (zero based)
 *              column - column of the item to select (zero based)
 *
 * RETURNED:	 the font id of the specified item
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	7/29/97	Initial Revision
 *
 ***********************************************************************/
FontID TblGetItemFont (const TableType * tableP, Int16 row, Int16 column)
{
	ErrFatalDisplayIf (column >= tableP->numColumns, "Invalid column");
	ErrFatalDisplayIf (row >= tableP->numRows, "Invalid row");

	return (tableP->items[row*tableP->numColumns + column].fontID);
}


/***********************************************************************
 *
 * FUNCTION:    TblSetItemFont
 *
 * DESCRIPTION: This routine set the font id of the item specified.
 *
 * PARAMETERS:	 tableP   - pointer to a table object
 *              row     - row of the item (zero based)
 *              column  - column of the item (zero based)
 *              fontID  - font id
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	7/29/97	Initial Revision
 *
 ***********************************************************************/
 void TblSetItemFont (TableType * tableP, Int16 row, Int16 column, FontID fontID)
 {
	if ((column >= tableP->numColumns) || (row >= tableP->numRows))
		{
		ErrNonFatalDisplay("Invalid parameter");
		return;
		}

	tableP->items[row*tableP->numColumns + column].fontID = fontID;
 }


/***********************************************************************
 *
 * FUNCTION:    TblSetRowUsable
 *
 * DESCRIPTION: This routine returns true if the specified row 
 *              is usable.  Rows that are unusable do not display.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              row    - row number (zero based)
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/10/95	Initial Revision
 *			trev	08/12/97	made non modified passed variables constant
 *
 ***********************************************************************/
 Boolean TblRowUsable  (const TableType * tableP, Int16 row)
 {
	if (row >= tableP->numRows)
		{
		ErrNonFatalDisplay("Invalid row");
		return 0;
		}

	return (tableP->rowAttrs[row].usable);
 }


/***********************************************************************
 *
 * FUNCTION:    TblSetRowUsable
 *
 * DESCRIPTION: This routine sets a row in a tableP usable or unusable.
 *              Rows that are unusable do not display.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              row    - row of the item to select (zero based)
 *              usable - true or false
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/16/95	Initial Revision
 *
 ***********************************************************************/
 void TblSetRowUsable  (TableType * tableP, Int16 row, Boolean usable)
 {
	if (row >= tableP->numRows)
		{
		ErrNonFatalDisplay("Invalid row");
		return;
		}

	tableP->rowAttrs[row].usable = usable;
 }
 
/***********************************************************************
 *
 * FUNCTION:    TblSetRowMasked
 *
 * DESCRIPTION: This routine sets a row in a tableP masked or unmasked.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              row    - row of the item to select (zero based)
 *              masked - true or false
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			jaq	7/16/99	Initial Revision
 *
 ***********************************************************************/
 void TblSetRowMasked  (TableType * tableP, Int16 row, Boolean masked)
 {
	if (row >= tableP->numRows)
		{
		ErrNonFatalDisplay("Invalid row");
		return;
		}

	tableP->rowAttrs[row].masked = masked;
 }


/***********************************************************************
 *
 * FUNCTION:    TblSetRowStaticHeight
 *
 * DESCRIPTION: This routine sets the static height attribute of a row.
 *              A rows that that has its static height attribute set will
 *              not expand / contract the height of the row as test is 
 *              added / removed from a text item.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              row    - row of the item to select (zero based)
 *              static - true or false
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/5/96	Initial Revision
 *
 ***********************************************************************/
 void TblSetRowStaticHeight (TableType * tableP, Int16 row, Boolean staticHeight)
 {
	if (row >= tableP->numRows)
		{
		ErrNonFatalDisplay("Invalid row");
		return;
		}

	tableP->rowAttrs[row].staticHeight = staticHeight;
 }


/***********************************************************************
 *
 * FUNCTION:    TblGetLastUsableRow
 *
 * DESCRIPTION: This routine returns the last row in a tableP that 
 *              in usable (displayable)
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              rowP   - row index, zero based (returned value)
 *
 * RETURNED:	 row   - row index (zero based) or -1 if there are no 
 *                     usable rows.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/29/95	Initial Revision
 *			trev	08/12/97	made non modified passed variables constant
 *
 ***********************************************************************/
 Int16 TblGetLastUsableRow (const TableType * tableP)
 {
	Int16 i;
	Int16 row = tblUnusableRow;
	
	ErrNonFatalDisplayIf(tableP->numRows == 0, "Empty Table!");
	
	for (i = 0; i < tableP->numRows; i++)
		{
		if (tableP->rowAttrs[i].usable)
			row = i;			
		}
	return (row);
 }


/***********************************************************************
 *
 * FUNCTION:    TblSetRowSelectable
 *
 * DESCRIPTION: This routine sets a row in a tableP selectable or
 *              non-selectable. Rows that are non-selectable 
 *              will not highlight when touched.
 *
 * PARAMETERS:	 tableP      - pointer to a table object
 *              row        - row of the item to select (zero based)
 *              selectable - true or false
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/16/95	Initial Revision
 *
 ***********************************************************************/
 void TblSetRowSelectable  (TableType * tableP, Int16 row, Boolean selectable)
 {
	if (row >= tableP->numRows)
		{
		ErrNonFatalDisplay("Invalid row");
		return;
		}

	tableP->rowAttrs[row].selectable = selectable;
 }
 
 
/***********************************************************************
 *
 * FUNCTION:    TbRowSelectable
 *
 * DESCRIPTION: This routine returns true if the row specified is 
 *              selectable. Rows that are non-selectable 
 *              will not highlight when touched.
 *
 * PARAMETERS:	 tableP      - pointer to a table object
 *              row        - row of the item to select (zero based)
 *
 * RETURNED:	 id of the row specied
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/16/95	Initial Revision
 *			trev	08/12/97	made non modified passed variables constant
 *
 ***********************************************************************/
 Boolean TblRowSelectable (const TableType * tableP, Int16 row)
 {
	if (row >= tableP->numRows)
		{
		ErrNonFatalDisplay("Invalid row");
		return 0;
		}

	return (tableP->rowAttrs[row].selectable);
 }
 
/***********************************************************************
 *
 * FUNCTION:    TblFindRowID
 *
 * DESCRIPTION: This routine returns the number of the row that 
 *              matches the specified id.
 *
 * PARAMETERS:	 tableP - pointer to a table object.
 *              id    - row id to find
 *              rowP  - pointer to the row number (return value)
 *
 * RETURNED:	 true is a match was found
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/15/95	Initial Revision
 *			trev	08/12/97	made non modified passed variables constant
 *
 ***********************************************************************/
Boolean TblFindRowID (const TableType * tableP, UInt16 id, Int16 * rowP)
{
	Int16 row;
	
	for (row = 0; row < tableP->numRows; row++)
		{
		if (tableP->rowAttrs[row].usable)
			{
			if (id == tableP->rowAttrs[row].id)
				{
				*rowP = row;
				return (true);
				}
			}				
		}
	return (false);
}


/***********************************************************************
 *
 * FUNCTION:    TblFindRowData
 *
 * DESCRIPTION: This routine returns the row number that contains the
 *              specified data value.
 *
 * PARAMETERS:	 tableP - pointer to a table object.
 *              data  - row data to find
 *              rowP  - pointer to the row number (return value)
 *
 * RETURNED:	 true is a match was found
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	7/28/95	Initial Revision
 *			trev	08/12/97	made non modified passed variables constant
 *
 ***********************************************************************/
Boolean TblFindRowData (const TableType * tableP, UInt32 data, Int16 * rowP)
{
	Int16 row;
	
	for (row = 0; row < tableP->numRows; row++)
		{
		if (tableP->rowAttrs[row].usable)
			{
			if (data == tableP->rowAttrs[row].data)
				{
				*rowP = row;
				return (true);
				}
			}				
		}
	return (false);
}


 /***********************************************************************
 *
 * FUNCTION:    TblGetRowID
 *
 * DESCRIPTION: This routine returns id value of the row specifed.
 *
 * PARAMETERS:	 tableP      - pointer to a table object
 *              row        - row of the item to select (zero based)
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/2/95	Initial Revision
 *			trev	08/12/97	made non modified passed variables constant
 *
 ***********************************************************************/
 UInt16 TblGetRowID (const TableType * tableP, Int16 row)
 {
	ErrFatalDisplayIf (row >= tableP->numRows, "Invalid row");

	return (tableP->rowAttrs[row].id);
 }
 
 
 /***********************************************************************
 *
 * FUNCTION:    TblSetRowID
 *
 * DESCRIPTION: This routine set to id value of the row specifed.
 *
 * PARAMETERS:	 tableP      - pointer to a table object
 *              row        - row of the item to select (zero based)
 *              id         - id to identify a row
 *
 * RETURNED:	 nothinh
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/2/95	Initial Revision
 *
 ***********************************************************************/
 void TblSetRowID (TableType * tableP, Int16 row, UInt16 id)
 {
	if (row >= tableP->numRows)
		{
		ErrNonFatalDisplay("Invalid row");
		return;
		}

	tableP->rowAttrs[row].id = id;
 }
 

 /***********************************************************************
 *
 * FUNCTION:    TblGetRowData
 *
 * DESCRIPTION: This routine returns data value of the row specifed.  
 *              The data value is a place holder for application
 *              specific values.
 *
 * PARAMETERS:	 tableP      - pointer to a table object
 *              row        - row of the item to select (zero based)
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	7/27/95	Initial Revision
 *			trev	08/12/97	made non modified passed variables constant
 *
 ***********************************************************************/
 UInt32 TblGetRowData (const TableType * tableP, Int16 row)
 {
	ErrFatalDisplayIf (row >= tableP->numRows, "Invalid row");

	return (tableP->rowAttrs[row].data);
 }
 
 
 /***********************************************************************
 *
 * FUNCTION:    TblSetRowData
 *
 * DESCRIPTION: This routine set to data value of the row specifed.
 *              The data value is a place holder for application
 *              specific values.
 *
 * PARAMETERS:	 tableP      - pointer to a table object
 *              row        - row of the item to select (zero based)
 *              date       - application specific data
 *
 * RETURNED:	 nothinh
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/2/95	Initial Revision
 *
 ***********************************************************************/
 void TblSetRowData (TableType * tableP, Int16 row, UInt32 data)
 {
	if (row >= tableP->numRows)
		{
		ErrNonFatalDisplay("Invalid row");
		return;
		}

	tableP->rowAttrs[row].data = data;
 }
 

/***********************************************************************
 *
 * FUNCTION:    TblSetRowInvalid
 *
 * DESCRIPTION: This routine returns true if the specified row 
 *              is invalid.  Rows that are invalid need to be redrawn.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              row    - row number (zero based)
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	9/21/95	Initial Revision
 *			trev	08/12/97	made non modified passed variables constant
 *
 ***********************************************************************/
 Boolean TblRowInvalid  (const TableType * tableP, Int16 row)
 {
	ErrFatalDisplayIf (row >= tableP->numRows, "Invalid row");

	return (tableP->rowAttrs[row].invalid);
 }
 
/***********************************************************************
 *
 * FUNCTION:    TblRowMasked
 *
 * DESCRIPTION: This routine returns true if the specified row 
 *              is masked.  .
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              row    - row number (zero based)
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			jaq	7/21/99	Initial Revision
 *
 ***********************************************************************/
 Boolean TblRowMasked  (const TableType * tableP, Int16 row)
 {
	if (row >= tableP->numRows)
		{
		ErrNonFatalDisplay("Invalid row");
		return 0;
		}

	return (tableP->rowAttrs[row].masked);
 }


 /***********************************************************************
 *
 * FUNCTION:    TblMarkRowInvalid
 *
 * DESCRIPTION: This routine marks the image of the row specifed invalid.  
 *              Rows that are mark invalid will be drawn by the tableP 
 *              redraw routine (TblRedrawTable).  Row the are not mark 
 *              invalid will not draw.
 *
 * PARAMETERS:	 tableP      - pointer to a table object
 *              row        - row of the item to select (zero based)
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/2/95	Initial Revision
 *
 ***********************************************************************/
void TblMarkRowInvalid (TableType * tableP, Int16 row)
{
	if (row >= tableP->numRows)
		{
		ErrNonFatalDisplay("Invalid row");
		return;
		}

	tableP->rowAttrs[row].invalid = true;
}


 /***********************************************************************
 *
 * FUNCTION:    TblMarkTableInvalid
 *
 * DESCRIPTION: This routine marks the image of all the rows in a tableP
 *              invalid.  Rows that are mark invalid will be drawn by 
 *              the tableP redraw routine (TblRedrawTable).  Row the are 
 *              not mark invalid will not draw.
 *
 * PARAMETERS:	 tableP      - pointer to a table object
 *              row        - row of the item to select (zero based)
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/2/95	Initial Revision
 *
 ***********************************************************************/
void TblMarkTableInvalid (TableType * tableP)
{
	Int32 row;
	
	for (row = 0; row < tableP->numRows; row++)
		tableP->rowAttrs[row].invalid = true;
}


 /***********************************************************************
 *
 * FUNCTION:    TblInsertRow
 *
 * DESCRIPTION: This routine insert a row into the tableP before the 
 *              specified row.  The number of row in the tableP is 
 *              not increased, the last row in the tableP is 
 *              removed.
 *
 * PARAMETERS:	 tableP    - pointer to a table object
 *              row      - row to insert before (zero based)
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/2/95	Initial Revision
 *
 ***********************************************************************/
void TblInsertRow (TableType * tableP, Int16 row)
{
	Int16 rowsToMove;
	Int16 columns;
	
	ErrFatalDisplayIf (row >= tableP->numRows, "Invalid row");

	columns = tableP->numColumns;
	rowsToMove = tableP->numRows - row - 1;
	if (rowsToMove == 0) return;

	MemMove (&tableP->rowAttrs[row+1], &tableP->rowAttrs[row],
		rowsToMove * sizeof (TableRowAttrType));
		
	tableP->rowAttrs[row].id = 0;
	tableP->rowAttrs[row].invalid = true;
	tableP->rowAttrs[row].usable = false;
	tableP->rowAttrs[row].masked = false;
	
	MemMove (&tableP->items[(row+1)*columns], &tableP->items[row*columns],
		rowsToMove * columns * sizeof (TableItemType));
		
	if ((tableP->attr.editing || tableP->attr.selected) &&
	    (row <= tableP->currentRow))
		{
		tableP->currentRow++;
		ErrNonFatalDisplayIf (tableP->currentRow >= tableP->numRows, "currentRow violated constraint!");
		}
}


 /***********************************************************************
 *
 * FUNCTION:    TblRemoveRow
 *
 * DESCRIPTION: This routine removes the specified row from the tableP 
 *              The number of row in the tableP is not decreased, 
 *              a row is added to the end of the tableP.
 *
 * PARAMETERS:	 tableP    - pointer to a table object
 *              row      - row to insert before (zero based)
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/2/95	Initial Revision
 *
 ***********************************************************************/
void TblRemoveRow (TableType * tableP, Int16 row)
{
	Int16 rowsToMove;
	Int16 columns;
	
	ErrFatalDisplayIf (row >= tableP->numRows, "Invalid row");

	columns = tableP->numColumns;
	rowsToMove = tableP->numRows - row - 1;

	MemMove (&tableP->rowAttrs[row], &tableP->rowAttrs[row+1],
		rowsToMove * sizeof (TableRowAttrType));
	
	MemMove (&tableP->items[row*columns], &tableP->items[(row+1)*columns],
		rowsToMove * columns * sizeof (TableItemType));
	

	tableP->rowAttrs[tableP->numRows-1].id = 0;
	tableP->rowAttrs[tableP->numRows-1].invalid = true;
	tableP->rowAttrs[tableP->numRows-1].usable = false;
	tableP->rowAttrs[tableP->numRows-1].masked = false;
}


 /***********************************************************************
 *
 * FUNCTION:    TblGetSelection
 *
 * DESCRIPTION: This routine returns the row and column of the currently
 *              selected tableP item.
 *
 * PARAMETERS:	 tableP      - pointer to a table object
 *              rowP       - row of the selected item (zero based)
 *              columnP    - column of the selected item (zero based)
 *
 * RETURNED:	 true if the item is highlighed false if not
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/2/95	Initial Revision
 *			trev	08/12/97	made non modified passed variables constant
 *
 ***********************************************************************/
Boolean TblGetSelection (const TableType * tableP, Int16 * rowP, Int16 * columnP)
{
	*rowP = tableP->currentRow;
	*columnP = tableP->currentColumn;

	return (tableP->attr.selected == true);
}


/***********************************************************************
 *
 * FUNCTION:    TblGetNumberOfRows
 *
 * DESCRIPTION: This routine returns the number of rows in a tableP.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *
 * RETURNED:	 number of rows in the specified tableP.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/16/95	Initial Revision
 *			trev	08/12/97	made non modified passed variables constant
 *
 ***********************************************************************/
Int16 TblGetNumberOfRows (const TableType * tableP)
 {
	return (tableP->numRows);
 }

/***********************************************************************
 *
 * FUNCTION:    TblGetBounds
 *
 * DESCRIPTION: This routine returns the bounds of a tableP.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              rP      - pointer to a RectangleType structure
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/27/95	Initial Revision
 *			trev	08/12/97	made non modified passed variables constant
 *
 ***********************************************************************/
 void TblGetBounds (const TableType * tableP, RectangleType * rP)
 {
	RctCopyRectangle (&tableP->bounds, rP);
 }


/***********************************************************************
 *
 * FUNCTION:    TblSetBounds
 *
 * DESCRIPTION: This routine sets the bounds of a tableP.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              rP      - pointer to a RectangleType structure
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/27/95	Initial Revision
 *			trev	08/12/97	made non modified passed variables constant
 *
 ***********************************************************************/
 void TblSetBounds (TableType * tableP, const RectangleType * rP)
 {
	RctCopyRectangle (rP, &tableP->bounds);
 }


/***********************************************************************
 *
 * FUNCTION:    TblGetRowHeight
 *
 * DESCRIPTION: This routine returns the height of the specified row.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              row    - row to set (zero based)
 *
 * RETURNED:	 height in pixels
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/19/95	Initial Revision
 *			trev	08/12/97	made non modified passed variables constant
 *
 ***********************************************************************/
 Coord TblGetRowHeight (const TableType * tableP, Int16 row)
 {
	ErrFatalDisplayIf (row >= tableP->numRows, "Invalid parameter");
	ErrNonFatalDisplayIf (row < 0, "Negative row");

 	return (tableP->rowAttrs[row].height);
 }


/***********************************************************************
 *
 * FUNCTION:    TblSetRowHeight
 *
 * DESCRIPTION: This routine sets the height of the specified row.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              row    - row to set (zero based)
 *              height - new height in pixels
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/27/95	Initial Revision
 *
 ***********************************************************************/
 void TblSetRowHeight (TableType * tableP, Int16 row, Coord height)
 {
	if (row >= tableP->numRows)
		{
		ErrNonFatalDisplay("Invalid row");
		return;
		}

 	tableP->rowAttrs[row].height = height;
 }


/***********************************************************************
 *
 * FUNCTION:    TblGetColumnWidth
 *
 * DESCRIPTION: This routine returns the width of the specified column
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              column - column number (zero based)
 *
 * RETURNED:	 width of a column
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/27/95	Initial Revision
 *			trev	08/12/97	made non modified passed variables constant
 *
 ***********************************************************************/
Coord TblGetColumnWidth (const TableType * tableP, Int16 column)
 {
	ErrFatalDisplayIf (column >= tableP->numColumns, "Invalid column");

 	return (tableP->columnAttrs[column].width);
 }


/***********************************************************************
 *
 * FUNCTION:    TblSetColumnWidth
 *
 * DESCRIPTION: This routine sets the width of the specified column.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              column - column number (zero based)
 *              width  - width of the column
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/23/95	Initial Revision
 *
 ***********************************************************************/
void TblSetColumnWidth (TableType * tableP, Int16 column, Coord width)
 {
	if (column >= tableP->numColumns)
		{
		ErrNonFatalDisplay("Invalid column");
		return;
		}

 	tableP->columnAttrs[column].width = width;
 }

/***********************************************************************
 *
 * FUNCTION:    TblGetColumnSpacing
 *
 * DESCRIPTION: This routine returns the spacing after the specified column.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              column - column number (zero based)
 *
 * RETURNED:	 spacing after column
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	9/26/95	Initial Revision
 *			trev	08/12/97	made non modified passed variables constant
 *
 ***********************************************************************/
Coord TblGetColumnSpacing (const TableType * tableP, Int16 column)
 {
	ErrFatalDisplayIf (column >= tableP->numColumns, "Invalid column");

 	return (tableP->columnAttrs[column].spacing);
 }


/***********************************************************************
 *
 * FUNCTION:    TblSetColumnSpacing
 *
 * DESCRIPTION: This routine sets the spacing after the specified column.
 *
 * PARAMETERS:	 tableP    - pointer to a table object
 *              column   - column number (zero based)
 *              spacing  - spacing after the column
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	9/26/95	Initial Revision
 *
 ***********************************************************************/
void TblSetColumnSpacing (TableType * tableP, Int16 column, Coord spacing)
 {
	if (column >= tableP->numColumns)
		{
		ErrNonFatalDisplay("Invalid column");
		return;
		}

 	tableP->columnAttrs[column].spacing = spacing;
 }

/***********************************************************************
 *
 * FUNCTION:    TblSetColumnUsable
 *
 * DESCRIPTION: This routine sets a column in a tableP usable or unusable.
 *              Columns that are unusable do not display.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              column - column of the item to select (zero based)
 *              usable - true or false
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/23/95	Initial Revision
 *
 ***********************************************************************/
 void TblSetColumnUsable  (TableType * tableP, Int16 column, Boolean usable)
 {
	if (column >= tableP->numColumns)
		{
		ErrNonFatalDisplay("Invalid column");
		return;
		}

	tableP->columnAttrs[column].usable = usable;
 }
 
 /***********************************************************************
 *
 * FUNCTION:    TblSetColumnMasked
 *
 * DESCRIPTION: This routine sets a column in a tableP masked or unmasked.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              column - column of the item to select (zero based)
 *              masked - true or false
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			jaq	7/23/99	Initial Revision
 *
 ***********************************************************************/
 void TblSetColumnMasked  (TableType * tableP, Int16 column, Boolean masked)
 {
	if (column >= tableP->numColumns)
		{
		ErrNonFatalDisplay("Invalid column");
		return;
		}

	tableP->columnAttrs[column].masked = masked;
 }


/***********************************************************************
 *
 * FUNCTION:    TblSetColumnEditIndicator
 *
 * DESCRIPTION: This routine set the column attribute that control whether
 *              a column highlights when the tableP is in edit mode.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *              column - column of the item (zero based)
 *              editIndicator
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/21/96	Initial Revision
 *
 ***********************************************************************/
 void TblSetColumnEditIndicator (TableType * tableP, Int16 column, Boolean editIndicator)
 {
	tableP->columnAttrs[column].editIndicator = editIndicator;
 }


/***********************************************************************
 *
 * FUNCTION:    TblSetCustomDrawProcedure
 *
 * DESCRIPTION: This routine sets the custom draw callback routine for
 *              the column specified.  The custom draw callback routine
 *              is used to draw tableP items with a style of
 *              customTableItem or tallCustomTableItem.
 *
 * PARAMETERS:	 tableP       - pointer to a table object
 *              column       - column of tableP
 *              drawCallback - callback routine.
 *
 * NOTE:        The callback routine should have the follow prototye:
 *							void drawCallback (void * tableP, Int16 row,
 *							Int16 column, RectangleType * bounds);
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/16/95	Initial Revision
 *
 ***********************************************************************/
 void TblSetCustomDrawProcedure (TableType * tableP, Int16 column, TableDrawItemFuncPtr drawCallback)
 {
	if (column >= tableP->numColumns)
		{
		ErrNonFatalDisplay("Invalid column");
		return;
		}

	tableP->columnAttrs[column].drawCallback = drawCallback;
 }


/***********************************************************************
 *
 * FUNCTION:    TblSetLoadDataProcedure
 *
 * DESCRIPTION: This routine sets the load data  callback routine for
 *              the column specified.  The callback routine is used
 *              to obtain the data valus of a tableP items.
 *
 * PARAMETERS:	 tableP            - pointer to a table object
 *              column           - column of tableP
 *              loadDataCallback - callback routine.
 *
 * NOTE:        The callback routine should have the follow prototye:
 *							MemHandle  LoadDataCallback (void * tableP, Int16 row,
 *                   Int16 column, Boolean editable, 
 *							UInt16 * dataOffset, UInt16 * dataSize);
 *
 *              The callback routine should return the MemHandle of a block 
 *              that contains a null terminated text string, the 
 *              offset from the start of the block to the start of the
 *              string, and the amount of space allocated for the string.
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/3/95	Initial Revision
 *
 ***********************************************************************/
 void TblSetLoadDataProcedure (TableType * tableP, Int16 column, TableLoadDataFuncPtr loadDataCallback)
 {
	if (column >= tableP->numColumns)
		{
		ErrNonFatalDisplay("Invalid column");
		return;
		}

	tableP->columnAttrs[column].loadDataCallback = loadDataCallback;
 }


/***********************************************************************
 *
 * FUNCTION:    TblSetSaveDataProcedure
 *
 * DESCRIPTION: This routine sets the save data  callback routine for
 *              the column specified.  The callback routine is called when 
 *              the table object determine the the data of a text object
 *              needes to be saved.
 *
 * PARAMETERS:	 tableP            - pointer to a table object
 *              column           - column of tableP
 *              saveDataCallback - callback routine.
 *
 * NOTE:        The callback routine should have the follow prototye:
 *							void *  SaveDataCallback (void * tableP, Int16 row,
 *							Int16 column);
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/3/95	Initial Revision
 *
 ***********************************************************************/
 void TblSetSaveDataProcedure (TableType * tableP, Int16 column, TableSaveDataFuncPtr saveDataCallback)
 {
	if (column >= tableP->numColumns)
		{
		ErrNonFatalDisplay("Invalid column");
		return;
		}

	tableP->columnAttrs[column].saveDataCallback = saveDataCallback;
 }


/***********************************************************************
 *
 * FUNCTION:    TblReleaseFocus
 *
 * DESCRIPTION: This routine release the focus.  If the current item is
 *              a text item, the memory allaocated for editing is released
 *              and the insertion point is turned off.
 *
 * PARAMETERS:	 tableP - pointer to a table object
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/3/95	Initial Revision
 *
 ***********************************************************************/
void TblReleaseFocus (TableType * tableP)
{
	SaveTextItem (tableP);
}


/***********************************************************************
 *
 * FUNCTION:    TblEditing
 *
 * DESCRIPTION: This routine returns true if the tableP is in edit mode.
 *              The tableP is in edit mode when a text item is being
 *              edited.
 *
 * PARAMETERS:	 tableP - pointer to a table object
 *
 * RETURNED:	 true if the tableP is in edit mode
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/3/95	Initial Revision
 *			trev	08/12/97	made non modified passed variables constant
 *
 ***********************************************************************/
Boolean TblEditing (const TableType * tableP)
{
	return (tableP->attr.editing);
}


/***********************************************************************
 *
 * FUNCTION:    TblGetCurrentField
 *
 * DESCRIPTION: This routine returns a pointer to the field structure 
 *              used to edit the current text item
 *
 * PARAMETERS:	 tableP - pointer to a table object
 *
 * RETURNED:	 FieldType * or NULL if the tableP is not is edit mode.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/3/95	Initial Revision
 *			trev	08/12/97	made non modified passed variables constant
 *
 ***********************************************************************/
FieldType * TblGetCurrentField (const TableType * tableP)
{
	if (tableP->attr.editing)
		return ((FieldType *)&tableP->currentField);
	
	return (NULL);
}


/***********************************************************************
 *
 * FUNCTION:    TblGrabFocus
 *
 * DESCRIPTION: This routine will put the tableP into edit mode.
 *					 Note - you should almost always use FrmSetFocus
 *					 to do this, the form will then tell the table to
 *					 grab focus.
 *
 * PARAMETERS:	 tableP - pointer to a table object.
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/30/95	Initial Revision
 *
 ***********************************************************************/
void TblGrabFocus (TableType * tableP, Int16 row, Int16 column)
{
	FieldType * fldP;
	RectangleType itemR;

	ErrFatalDisplayIf(tableP->attr.editing, "Table already has focus");
	ErrNonFatalDisplayIf(tableP->rowAttrs[row].masked, "Can't edit masked row");
	ErrFatalDisplayIf ((column >= tableP->numColumns) || (row >= tableP->numRows), "Invalid parameter");

	if (!tableP->attr.usable)
		{
		ErrNonFatalDisplay("Can't set focus on a hidden table");
		return;
		}

	fldP = &tableP->currentField;
	TblGetItemBounds (tableP, row, column, &itemR);
	InitTextItem (tableP, fldP, row, column, itemR.topLeft.x, itemR.topLeft.y, 
		true, true);

	tableP->attr.editing = true;
	tableP->currentRow = row;
	tableP->currentColumn = column;
		
	DrawEditIndicator (tableP, true);
}


/***********************************************************************
 *
 * FUNCTION:    TblHasScrollBar
 *
 * DESCRIPTION: This routine sets the hasScrollBar attribute the tableP.
 *              A tableP that has its attribute set will initialize it's
 *              field object such that it will send fldChanged events
 *              when that scroll bar needs to be updated.
 *
 * PARAMETERS:	 tableP        - pointer to a table object
 *              hasScrollBar - true or false
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/7/96	Initial Revision
 *
 ***********************************************************************/
void TblHasScrollBar (TableType * tableP, Boolean hasScrollBar)
{
	tableP->attr.hasScrollBar = hasScrollBar;
}


/***********************************************************************
 *
 * FUNCTION:    TblGetNumberOfColumns
 *
 * DESCRIPTION: This routine returns the number of columns in a tableP.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *
 * RETURNED:	 number of columns in the specified tableP.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			tlw		11/07/00	Initial Revision
 *
 ***********************************************************************/
Int16 TblGetNumberOfColumns (const TableType * tableP)
{
	return (tableP->numColumns);
}


/***********************************************************************
 *
 * FUNCTION:    TblGetTopRow
 *
 * DESCRIPTION: This routine returns the top visible row in a tableP.
 *
 * PARAMETERS:	 tableP  - pointer to a table object
 *
 * RETURNED:	 top row (displayed) in the specified tableP.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			tlw		11/07/00	Initial Revision
 *
 ***********************************************************************/
Int16 TblGetTopRow (const TableType * tableP)
{
	return (tableP->topRow);
}


/***********************************************************************
 *
 * FUNCTION:    TblSetSelection
 *
 * DESCRIPTION: This routine sets a table 'cell' as the current selection.
 *              TblDrawTable MUST be called afterwards to update the UI.
 *
 * PARAMETERS:	 tableP - pointer to a table object
 *              row    - row of the item to select (zero based)
 *              column - column of the item to select (zero based)
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			tlw	11/07/00	Initial Revision
 *
 ***********************************************************************/
void TblSetSelection (TableType * tableP, Int16 row, Int16 column)
{
	if ((column >= tableP->numColumns) || (row >= tableP->numRows))
		{
		ErrNonFatalDisplay("Invalid parameter");
		return;
		}

	if (!(tableP->attr.visible  && tableP->attr.usable) )
		return;

	// Draw the new items as selected, if it's not in a masked row
	if (tableP->rowAttrs[row].masked)
		tableP->attr.selected = false;
	else
		{
		tableP->currentRow = row;
		tableP->currentColumn = column;
		tableP->attr.selected = true;
		}
}
