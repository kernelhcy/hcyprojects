/******************************************************************************
 *
 * Copyright (c) 1994-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: Keyboard.c
 *
 * Release: 
 *
 * Description:
 *		This file contain routines to handle the onscreen keyboard.
 *		Included are the keyboard object and the keyboard form.
 *
 * History:
 *		03/29/95	rsf	Created by Roger Flores
 *		11/06/99	jmp	Updated to support selection/unselection for color environments.
 *		06/05/00	kwk	Use resource ids from the locale module range.
 *
 *****************************************************************************/

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_CLIPBOARDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_FIELDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS
#define ALLOW_ACCESS_TO_INTERNALS_OF_BITMAPS

#include <PalmTypes.h>
#include <SystemPublic.h>

#include "Field.h"
#include "Form.h"
#include "Keyboard.h"
#include "Menu.h"
#include "Event.h"

#define NON_PORTABLE
#include "SystemPrv.h"
#include "Globals.h"
#include "UIColorPrv.h"
#include "UIGlobals.h"
#include "UIResourcesPrv.h"
#include "IntlPrv.rh"		// kIntlLocaleModuleResBaseID, kSystemKeyboardResID


#define kbdNoKeyPtr		NULL
#define noItemSelection kbdNoKeyPtr
#define noLabel			0

#define keyboardRows		4
#define numKeyboards		3	// alpha, punc/number, and int'l

#define keyboardFont		stdFont


// KeybardShiftState - helper for KbdSet/GetShiftState
#define KeyboardShiftState(SHIFT, CAPS)   (((SHIFT) ? KeyboardShiftFlag : 0) | ((CAPS) ? KeyboardCapslockFlag : 0))


// These two structures also exist in MakeKbd.c.
// This is the format of the data in the keyboard resource.
typedef UInt16 Offset;		// offset from the start of the resource

typedef struct {
	char		shiftedKey;		// key to use if shift is on
	char		capsKey;			// key to use if caps lock is on
	char		unshiftedKey;
	UInt8		width;			// the width of the key.  The text is drawn centered.
	Offset	labelOffset;	// if there is a label display it instead
									// of a key (all modes)
} KeyboardKey;

typedef struct {
	RectangleType	bounds;
	UInt8 rowHeight;
	FontID font;
	Offset keys;
	UInt8 keysPerRow[keyboardRows];
	UInt8 shiftKey;			// can be kbdNoKey
	UInt8 capsKey;				// can be kbdNoKey
	Boolean lastLayoutInKeyboard;
	UInt8 reserved;
} KeyboardLayout;


// This structure holds the current state of the keyboard.
struct KeyboardStatus {
	Boolean	shiftOn;
	Boolean	capsOn;
	UInt8 * kbdResource;
	KeyboardLayout *layout[numKeyboards];
	UInt8 currentKeyboard;
	UInt8 flags;
	PointType drawOffset;
};

#define KeyboardFlagVisible		0x01

#define KeyboardFlagSet(KS, F)		((KS)->flags |= (F))
#define KeyboardFlagClear(KS, F)		((KS)->flags &= ~(F))
#define KeyboardFlagTest(KS, F)		((KS)->flags & (F))


// This structure holds info about the orignal field.
typedef struct {
	FieldType *fieldP;
	UInt16 offset;
	UInt16 blockSize;
	UInt16 height;
} OriginalFieldInfo;


// These are form constants.
#define KeyboardDoneButton		(kIntlLocaleModuleResBaseID+2)
#define KeyboardTextField		(kIntlLocaleModuleResBaseID+3)
#define KeyboardAbcPushBtn		(kIntlLocaleModuleResBaseID+5)
#define Keyboard123PushBtn		(kIntlLocaleModuleResBaseID+6)
#define KeyboardIntlPushBtn	(kIntlLocaleModuleResBaseID+7)
#define KeyboardUpButton		(kIntlLocaleModuleResBaseID+8)
#define KeyboardDownButton		(kIntlLocaleModuleResBaseID+9)
#define KeyboardGadget			(kIntlLocaleModuleResBaseID+10)

// These are menu constants.
#define sysKeyboardEditUndoCmd			100
#define sysKeyboardEditCutCmd				101
#define sysKeyboardEditCopyCmd			102
#define sysKeyboardEditPasteCmd			103
#define sysKeyboardEditSelectAllCmd		104


#define linesInKeyboardField	4

#define keyboardDrawOffsetX	-1
#define keyboardDrawOffsetY	59

// These are KbdDraw constants.
#define kbdDrawKeyTopsOnly 	true
#define kbdDrawEverything		false

#define kbdIgnoreModifiers		true
#define kbdSelectModifiers		false

// These are KbdSelectItem()/KbdSelectRectangle() constants.
#define kbdDrawItemSelected	true
#define kbdDrawItemUnselected	false

#define kbdDrawKeyboard			true
#define kbdDontDrawKeyboard	false

// In certain builds, we're having trouble with the compiler's optimizer
// on this file.  So, if SPECIFY_OPT_LEVEL is defined, we set the optimization_level
// to 1.  We'd like to get rid of this ASAP.
//
#ifdef SPECIFY_OPT_LEVEL
//#pragma optimization_level 1
#endif

/***********************************************************************
 *
 * FUNCTION: 	 ProcessKey
 *
 * DESCRIPTION: Return the key for the passed selection considering 
 *		the shift and caps lock statuses.
 *
 * PARAMETERS:  KeyboardStatus	- pointer to keyboard status
 *					 newSelection		- the key selected
 *
 * RETURNED:	 a character for the selected key
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	3/30/95	Initial Revision
 *			kwk	07/23/98	Now returns WChar with no sign extension.
 *
 ***********************************************************************/
static WChar ProcessKey(KeyboardStatus *ks, KeyboardKey *newSelection)
{
	UInt8 theChar;
	
	if (ks->shiftOn) {
		// shift and caps cancel each other on some keys!
		if (ks->capsOn && ((newSelection->unshiftedKey != newSelection->capsKey))) {
			theChar = newSelection->unshiftedKey;
		} else {
			theChar = newSelection->shiftedKey;
		}
	} else if (ks->capsOn) {
		theChar = newSelection->capsKey;
	} else {
		theChar = newSelection->unshiftedKey;		
	}

	return(theChar);
} // ProcessKey


/***********************************************************************
 *
 * FUNCTION: 	 MapPointToItem
 *
 * DESCRIPTION: Return the KeyboardKey at x, y.
 *
 * PARAMETERS:  x, y - coordinate
 *					 ks - the KeyboardStatus in use
 *              passedKeyBounds - pointer to get the bounds of the key at x, y
 *
 * RETURNED:	 KeyboardKey pointer containing x,y or NULL
 *					if (passedKeyBounds != NULL) it gets the key's bounds
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	3/30/95	Initial Revision
 *
 ***********************************************************************/
static KeyboardKey * MapPointToItem (Int16 x, Int16 y, KeyboardStatus *ks,
	RectangleType *passedKeyBounds)
{
	RectangleType	keyBounds;
	KeyboardLayout	*k;
	KeyboardKey 	*kkPtr;
	UInt16			row;
	UInt16			key;
	PointType		position;

	k = ks->layout[ks->currentKeyboard];
	while (true)
		{
		// setup row information for the loop
		position.x = k->bounds.topLeft.x + ks->drawOffset.x;
		position.y = k->bounds.topLeft.y + ks->drawOffset.y;
		keyBounds.topLeft.y = position.y - k->rowHeight;
		keyBounds.extent.y = k->rowHeight;

		// if the point is above or to the left of this layout check the next one!
		if (y < keyBounds.topLeft.y || x < position.x)
			{
			// Stop if this is the last layout in the keyboard			
			if (k->lastLayoutInKeyboard)
				break;
			else
				k++;
				continue;
			}
			

		kkPtr = (KeyboardKey *) (ks->kbdResource + k->keys);
		for (row = 0; row < keyboardRows; row++)
			{
			// skip empty rows
			if (k->keysPerRow[row] == 0)
				continue;
				
			keyBounds.topLeft.y = keyBounds.topLeft.y + keyBounds.extent.y;
//			keyBounds.extent.y = k->rowHeight;
			keyBounds.topLeft.x = position.x;

			// skip this row if the point is below it (below the bottom line)
			if (y > keyBounds.topLeft.y + keyBounds.extent.y)
				{
				kkPtr += k->keysPerRow[row];
				continue;
				}
				
			for (key = 0; key < k->keysPerRow[row]; key++, kkPtr++)
				{
				keyBounds.extent.x = kkPtr->width;
			
				// if x is to the left of the right side of the key then this is the key!
				if (x < keyBounds.topLeft.x + keyBounds.extent.x)
					{
					if (passedKeyBounds != NULL)
						{
						passedKeyBounds->topLeft.x = keyBounds.topLeft.x;
						passedKeyBounds->topLeft.y = keyBounds.topLeft.y;
						passedKeyBounds->extent.x = keyBounds.extent.x;
						passedKeyBounds->extent.y = keyBounds.extent.y + 1;
						}
					return kkPtr;
					}
				
				// advance to the new key
				keyBounds.topLeft.x += keyBounds.extent.x;
				}
			}
			
			
		// Stop if this is the last layout in the keyboard			
		if (k->lastLayoutInKeyboard)
			break;
		else
			k++;
		}	

	return NULL;
}


/***********************************************************************
 *
 * FUNCTION: 	 GetFieldHeight
 *
 * DESCRIPTION: This routine returns the height of the specified text
 *              if shown in the original field.
 *
 * PARAMETERS:  systemKeyboard - the keyboard status info
 *              textP          - string to calculate height of
 *
 * RETURNED:	 number of line needed to display the string
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/13/95	Initial Revision
 *
 ***********************************************************************/
static UInt16 GetFieldHeight (OriginalFieldInfo *originalField, Char * textP)
{
	UInt16 height;
	FontID curFont;
	RectangleType bounds;

	FldGetBounds (originalField->fieldP, &bounds);
	curFont = FntSetFont (FldGetFont (originalField->fieldP));
	height = FldCalcFieldHeight (textP, bounds.extent.x);
	FntSetFont (curFont);
	
	return (height);
}


/***********************************************************************
 *
 * FUNCTION:    KbdDrawInversionEffect
 *
 * DESCRIPTION: This routine does an inversion effect by swapping colors
 *					 this is NOT undoable by calling it a second time, rather
 *					 it just applies a selected look on top of already
 *					 rendered data.  (It's kind of a hack.)
 *
 * PARAMETERS:	 rP  - pointer to a rectangle to 'invert'
 *					 cornerDiam	- corner diameter
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	10/28/99	Initial revision
 *			jmp	11/06/99	Modified for use with the keyboard (i.e., backcolor
 *								changed to UIObjectFill).
 *
 ***********************************************************************/
static void KbdDrawInversionEffect (const RectangleType *rP, UInt16 cornerDiam) 
{
	WinPushDrawState();
	WinSetDrawMode(winSwap);
	WinSetPatternType (blackPattern);
	WinSetBackColor(UIColorGetIndex(UIObjectFill));
	WinSetForeColor(UIColorGetIndex(UIObjectSelectedFill));
	WinPaintRectangle(rP, cornerDiam);
	
	if (UIColorGetIndex(UIObjectSelectedFill) != UIColorGetIndex(UIObjectForeground)) {
		WinSetBackColor(UIColorGetIndex(UIObjectForeground));
		WinSetForeColor(UIColorGetIndex(UIObjectSelectedForeground));
		WinPaintRectangle(rP, cornerDiam);
	}
	WinPopDrawState();
}


/***********************************************************************
 *
 * FUNCTION: 	 KbdSelectRectangle/KbdSelectItem
 *
 * DESCRIPTION: KbdSelectRectangle() is a utility function KbdSelectItem().
 *					 KbdSelectItem() selects or unselect a key.  
 *
 * PARAMETERS:  ks     		- pointer to KeyboardStatus in use
 *              selectKey  - KeyboardKey to select or unselect
 *					 selected	- select if true, unselect otherwise
 *					 redrawKbd	- redraw keyboard if true
 *					 
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	3/30/95	Initial Revision
 *			jmp	11/06/99	Updated to support selection/unselection instead
 *								of simple inversion.
 *			jmp	11/19/99 Bracket all the drawing with WinScreenLock()/WinScreenUnlock()
 *								calls to eliminate flicker.
 *       jmp   12/02/99 Fix bug #24377.  Don't call WinScreenUnlock() if WinScreenLock()
 *                      fails.
 *                          
 ***********************************************************************/
static void KbdSelectRectangle (KeyboardStatus *ks, RectangleType *bounds, Boolean selected, Boolean redrawKbd)
{
	RectangleType savedClip;
	UInt8 * lockedWinP;
	
	lockedWinP = WinScreenLock(winLockCopy);
	WinGetClip(&savedClip);
	WinSetClip(bounds);
	
	if (redrawKbd)
		KbdDraw(ks, kbdDrawEverything, kbdIgnoreModifiers);
	
	if (selected)
		KbdDrawInversionEffect(bounds, 0);

	WinSetClip(&savedClip);
	if (lockedWinP)
		WinScreenUnlock();
}

static void KbdSelectItem (KeyboardStatus *ks, KeyboardKey *selectKey, Boolean selected, Boolean redrawKbd)
{
	RectangleType	keyBounds;
	KeyboardLayout *k;
	UInt16			row;
	UInt16			key;
	KeyboardKey 	*kkPtr;
	PointType		position;
	
	if (selectKey == NULL)
		return;
		
	k = ks->layout[ks->currentKeyboard];
	while (true)
		{
		// setup row information for the loop
		position.x = k->bounds.topLeft.x + ks->drawOffset.x;
		position.y = k->bounds.topLeft.y + ks->drawOffset.y;
		keyBounds.topLeft.y = position.y - k->rowHeight;
		keyBounds.extent.y = k->rowHeight;

	
		kkPtr = (KeyboardKey *) (ks->kbdResource + k->keys);
		for (row = 0; row < keyboardRows; row++)
			{
			// skip empty rows
			if (k->keysPerRow[row] == 0)
				continue;
				
			keyBounds.topLeft.y = keyBounds.topLeft.y + keyBounds.extent.y;
			keyBounds.extent.y = k->rowHeight;
			keyBounds.topLeft.x = position.x;

			// skip this row if the key isn't in the row
			if (selectKey > (kkPtr + k->keysPerRow[row] - 1))
				{
				kkPtr += k->keysPerRow[row];
				continue;
				}
				
			for (key = 0; key < k->keysPerRow[row]; key++, kkPtr++)
				{
				keyBounds.extent.x = kkPtr->width;
			
				// if this is the key, select or unselect it and exit
				if (selectKey == kkPtr)
					{
					keyBounds.topLeft.x++;
					keyBounds.topLeft.y++;
					keyBounds.extent.x--;
					keyBounds.extent.y--;
					KbdSelectRectangle (ks, &keyBounds, selected, redrawKbd);
					return;
					}
				
				// advance to the new key
				keyBounds.topLeft.x += keyBounds.extent.x;
				}
			}
			
		// Stop if this is the last layout in the keyboard			
		if (k->lastLayoutInKeyboard)
			break;
		else
			k++;
		}	

}


/***********************************************************************
 *
 * FUNCTION: 	 KbdDraw
 *
 * DESCRIPTION: Draw the keyboard(s).
 *
 *		Once the keyboard is drawn, it is visible (visible bit is set).
 *
 * PARAMETERS:  KeyboardStatus   - pointer to keyboard status
 *					 keyTopsOnly		- true if only the key tops need drawing
 *											  Used when shift is pressed
 *					 ignoreModifiers  - true when we don't want to touch the
 *											  the the state of the modifier keys
 *					 
 * RETURNED:	 nothing
 *
 * HISTORY:
 *		03/30/95	rsf	Created by Roger Flores.
 *		11/06/99	jmp	Updated to support drawing in color by selecting
 *							and unselecting keys rather than simply inverting
 *							them.
 *
 ***********************************************************************/
void KbdDraw(KeyboardStatus *ks, Boolean keyTopsOnly, Boolean ignoreModifiers)
{
	RectangleType	keyBounds;
	RectangleType	eraseBounds;		// Bounds of key top to erase
	KeyboardLayout *k;
	Char				*keyChars;
	UInt16			keyCharsLen;
	UInt16			keyCharsX;
	UInt16			keyCharsY;
	char				keyChar;
	UInt16			row;
	UInt16			key;
	MemHandle		bitmapH;
	BitmapPtr 		keytopBitmapP;
	KeyboardKey 	*kkPtr;
	PointType		position;
	
	
	WinPushDrawState ();
	WinSetBackColor(UIColorGetIndex(UIObjectFill));
	WinSetForeColor(UIColorGetIndex(UIObjectForeground));
	WinSetTextColor(UIColorGetIndex(UIObjectForeground));
	
	FntSetFont (keyboardFont);
	
	k = ks->layout[ks->currentKeyboard];
	while (true)
		{
		position.x = k->bounds.topLeft.x + ks->drawOffset.x;
		position.y = k->bounds.topLeft.y + ks->drawOffset.y;
		eraseBounds.topLeft = position;
		eraseBounds.extent = k->bounds.extent;
		
		if (!keyTopsOnly)
			WinEraseRectangle (&eraseBounds, 0);
		FntSetFont (k->font);
		
		// setup row information for the loop
		keyBounds.topLeft.y = position.y - k->rowHeight;
		keyBounds.extent.y = k->rowHeight;
		
		
		kkPtr = (KeyboardKey *) (ks->kbdResource + k->keys);
		for (row = 0; row < keyboardRows; row++)
			{
			// Skip empty rows
			if (k->keysPerRow[row] == 0)
				continue;
			
			keyBounds.topLeft.y = keyBounds.topLeft.y + keyBounds.extent.y;
			keyBounds.extent.y = k->rowHeight;
			keyBounds.topLeft.x = position.x;
			keyCharsY = keyBounds.topLeft.y + (keyBounds.extent.y - FntLineHeight() + 1) / 2;
			
			// draw the line above this row
			if (!keyTopsOnly)
				WinDrawLine(position.x + 1, keyBounds.topLeft.y, 
					position.x + k->bounds.extent.x - 2, keyBounds.topLeft.y);
			
			for (key = 0; key < k->keysPerRow[row]; key++)
				{
				keyBounds.extent.x = kkPtr->width;
			
				// Draw the line on the right side of the key
				if (!keyTopsOnly)
					WinDrawLine(keyBounds.topLeft.x + keyBounds.extent.x, keyBounds.topLeft.y, 
						keyBounds.topLeft.x + keyBounds.extent.x, keyBounds.topLeft.y + keyBounds.extent.y - 1);

				// If the key has a label or bitmap display it; otherwise display the 
				// proper character considering the keyboard mode
				if (kkPtr->labelOffset != noLabel)
					{
					bitmapH = NULL;
					if (kkPtr->labelOffset == kbdBackspaceKey)
						bitmapH = DmGetResource (bitmapRsc, kKeyboardBackspaceBitmapResID);
					else if (kkPtr->labelOffset == kbdTabKey)
						bitmapH = DmGetResource (bitmapRsc, kKeyboardTabBitmapResID);
					else if (kkPtr->labelOffset == kbdReturnKey)
						bitmapH = DmGetResource (bitmapRsc, kKeyboardReturnBitmapResID);
					else if (kkPtr->labelOffset == kbdShiftKey)
						bitmapH = DmGetResource (bitmapRsc, kKeyboardShiftBitmapResID);
					else if (kkPtr->labelOffset == kbdCapsKey)
						bitmapH = DmGetResource (bitmapRsc, kKeyboardCapBitmapResID);
					
					if (bitmapH != NULL)
						{
						// Erase the entire key top because these may be inverted.
						if (kkPtr->labelOffset == kbdShiftKey ||
							kkPtr->labelOffset == kbdCapsKey)
							{
							// Erase the key top
							eraseBounds.topLeft.x = keyBounds.topLeft.x + 1;
							eraseBounds.topLeft.y = keyBounds.topLeft.y + 1;
							eraseBounds.extent.x = keyBounds.extent.x - 1;
							eraseBounds.extent.y = keyBounds.extent.y - 1;
							WinEraseRectangle(&eraseBounds, 0);
							}
						
						keytopBitmapP = MemHandleLock(bitmapH);
						// Draw the bitmap centered within the key's bounds
						WinDrawBitmap (keytopBitmapP, 
							keyBounds.topLeft.x + (keyBounds.extent.x - keytopBitmapP->width) / 2,
							keyBounds.topLeft.y + (keyBounds.extent.y + 1 - keytopBitmapP->height) / 2);
						MemPtrUnlock(keytopBitmapP);
						keyChars = NULL;
						}
					else
						{
						keyChars = (Char *) (ks->kbdResource + kkPtr->labelOffset);
						keyCharsLen = StrLen(keyChars);
						// center the label within the key (don't count right line!)
						keyCharsX = keyBounds.topLeft.x + (keyBounds.extent.x - 1 - 
							(FntCharsWidth(keyChars, keyCharsLen) - 1) + 1) / 2;
						}
					}
				else
					{
					keyChar = ProcessKey(ks, kkPtr);
					keyChars = &keyChar;
					keyCharsLen = 1;
					// center the character within the key
					keyCharsX = keyBounds.topLeft.x + (keyBounds.extent.x - 1 - 
						(FntCharWidth(keyChar) - 1) + 1) / 2 + 1;
					}
					
				if (keyChars != NULL && keyCharsLen > 0)
					{
					// Erase the key top
					eraseBounds.topLeft.x = keyBounds.topLeft.x + 1;
					eraseBounds.topLeft.y = keyBounds.topLeft.y + 1;
					eraseBounds.extent.x = keyBounds.extent.x - 1;
					eraseBounds.extent.y = keyBounds.extent.y - 1;
					WinEraseRectangle(&eraseBounds, 0);
					
					WinDrawChars(keyChars, keyCharsLen, keyCharsX, keyCharsY);
					}
				
				// advance to the new key
				kkPtr++;
				keyBounds.topLeft.x += keyBounds.extent.x;
				}
			}
		// Draw a line at the bottom of the keyboard (each row had a line drawn above it).
		if (!keyTopsOnly)
			WinDrawLine(position.x + 1, keyBounds.topLeft.y + keyBounds.extent.y, 
				position.x + k->bounds.extent.x - 2, keyBounds.topLeft.y + keyBounds.extent.y);

		// Draw a line for the left edge of the keyboard
		if (!keyTopsOnly)
			WinDrawLine(position.x, position.y + 1, 
				position.x, keyBounds.topLeft.y + keyBounds.extent.y - 1);
		
		// Erase a pixel at the top right corner of the keyboard.  It's filled in when the
		// right edge of top right key is drawn.
		if (!keyTopsOnly)
			WinEraseLine(position.x + k->bounds.extent.x - 1, position.y, 
				position.x + k->bounds.extent.x - 1, position.y);

		// Now that the keys are drawn, highlight the shift or caps key if the mode is on and
		// were not ignoring the modifiers.
		if (!ignoreModifiers)
			{
			kkPtr = (KeyboardKey *) (ks->kbdResource + k->keys);
			if (ks->shiftOn && (k->shiftKey != kbdNoKey))
				KbdSelectItem(ks, kkPtr + k->shiftKey, kbdDrawItemSelected, kbdDontDrawKeyboard);

			if (ks->capsOn && (k->capsKey != kbdNoKey))
				KbdSelectItem(ks, kkPtr + k->capsKey, kbdDrawItemSelected, kbdDontDrawKeyboard);
			}
			
		// Stop if this is the last layout in the keyboard			
		if (k->lastLayoutInKeyboard)
			break;
		else
			k++;
		}

	WinPopDrawState ();
	
	// The keyboard is now visible.
	KeyboardFlagSet(ks, KeyboardFlagVisible);
}


/***********************************************************************
 *
 * FUNCTION: 	 KbdErase
 *
 * DESCRIPTION: Erase the keyboard.
 *
 *		Once the keyboard is erased, it is no longer visible
 *		(visible bit not set).
 *
 * PARAMETERS:  KeyboardStatus   - pointer to keyboard status
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			grant	6/26/00	Initial Revision
 *
 ***********************************************************************/
void KbdErase(KeyboardStatus *ks)
{
	KeyboardLayout *k;
	RectangleType keyboardBounds;
	
	// if no keyboard object is given, bail out
	if (ks == NULL) return;
	
	k = ks->layout[ks->currentKeyboard];
	while (true)
	{
		keyboardBounds.topLeft.x = k->bounds.topLeft.x + ks->drawOffset.x;
		keyboardBounds.topLeft.y = k->bounds.topLeft.y + ks->drawOffset.y;
		keyboardBounds.extent = k->bounds.extent;
		
		WinEraseRectangle (&keyboardBounds, 0);
		if (k->lastLayoutInKeyboard)
			break;
		else
			k++;
	}
	
	// Keyboard is now not visible.
	KeyboardFlagClear(ks, KeyboardFlagVisible);
}


/***********************************************************************
 *
 * FUNCTION:    KbdHandleEvent
 *
 * DESCRIPTION: MemHandle penDownEvents for the kbd object.  Typical
 *		work performed is the point to key mapping, inversion of
 *		the key, tracking of the point, and enqueing the key if 
 *		it's selected.
 *
 * PARAMETERS:	 ks 		- pointer to control object (ControlType)
 *              pEvent	- pointer to an EventType structure.
 *
 * RETURNED:	true if the event was MemHandle or false if it was not.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	03/30/95	Initial Revision
 *			kwk	07/22/98	Use WChar for processedKey, to fix sign extension
 *								problem when calling EvtEnqueueKey.
 *			jmp	11/06/99	Updated to support selection/unselection instead
 *								of using pure inversion to better support color
 *								environments.
 *			peter	12/02/99 Shift turns off caps lock.
 *
 ***********************************************************************/
Boolean KbdHandleEvent (KeyboardStatus *ks, EventType * pEvent)
	{
	Boolean			penDown;
	Boolean			inverted;
	Int16				x, y;
	KeyboardKey 	*newSelection, *proposedSelection;
	RectangleType	keyBounds;
	KeyboardLayout *k;
	WChar				processedKey;
	RectangleType bounds;

	if (pEvent->eType != penDownEvent)
		return false;


	// check the layout to see if the point is within any of the keyboards
	k = ks->layout[ks->currentKeyboard];
	while (true)
		{
		bounds.topLeft.x = k->bounds.topLeft.x + ks->drawOffset.x;
		bounds.topLeft.y = k->bounds.topLeft.y + ks->drawOffset.y;
		bounds.extent = k->bounds.extent;
		if (RctPtInRectangle (pEvent->screenX, pEvent->screenY, &bounds))
			break;
			
		// if the point isn't within a keyboard then return false
		if (k->lastLayoutInKeyboard)
			return false;
		else
			k++;
		}	

		
	newSelection = kbdNoKeyPtr;
	proposedSelection = MapPointToItem (pEvent->screenX, pEvent->screenY, ks, &keyBounds);
	if (newSelection != proposedSelection)
		{
		KbdSelectItem(ks, newSelection, kbdDrawItemUnselected, kbdDrawKeyboard);
		newSelection = proposedSelection;
		KbdSelectItem(ks, newSelection, kbdDrawItemSelected, kbdDrawKeyboard);
		inverted	= true;
		}


	penDown = true;

	do
		{
		PenGetPoint (&x, &y, &penDown);

		if (RctPtInRectangle (x, y, &keyBounds))
			{
			proposedSelection = MapPointToItem (x, y, ks, NULL);
			if (penDown)
				{
				if (newSelection != proposedSelection)
					{
					if (inverted)
						KbdSelectItem(ks, newSelection, kbdDrawItemUnselected, kbdDrawKeyboard);
					newSelection = proposedSelection;
					KbdSelectItem(ks, newSelection, kbdDrawItemSelected, kbdDrawKeyboard);
					inverted = true;
					}
				}
			}
		else     //moved out of bounds
			{
			if (inverted)
				{
				KbdSelectItem(ks, newSelection, kbdDrawItemUnselected, kbdDrawKeyboard);
				inverted = false;
				}
			newSelection = noItemSelection;
			}
		} while (penDown);


	// Uninvert the character unless it's the shift or caps lock key.  These
	// should stay inverted (reduces flashing).
	if (inverted && 
		newSelection->unshiftedKey != kbdCapsKey && 
		newSelection->unshiftedKey != kbdShiftKey)
		{
		KbdSelectItem(ks, newSelection, kbdDrawItemUnselected, kbdDrawKeyboard);
		inverted = false;
		}


	if (newSelection == noItemSelection)
		return true;


	// Send a keypress.
	processedKey = ProcessKey(ks, newSelection);
	SndPlaySystemSound(sndClick);
	if (processedKey == kbdShiftKey)
		{
		// Tapping shift turns off caps lock.
		if (ks->capsOn)
		   ks->capsOn = false;
		else
			ks->shiftOn = !ks->shiftOn;

		// Draw the changed key tops and lit shift key.
		KbdDraw(ks, kbdDrawKeyTopsOnly, kbdSelectModifiers);
		GrfSetState(ks->capsOn, false, ks->shiftOn);
		return true;
		}
	else if (processedKey == kbdCapsKey)
		{
		ks->capsOn = !ks->capsOn;
		// Turn shift off when case is on.
		if (ks->shiftOn)
		   ks->shiftOn = false;
		// Draw the changed key tops and lit caps key.
		KbdDraw(ks, kbdDrawKeyTopsOnly, kbdSelectModifiers);
		GrfSetState(ks->capsOn, false, ks->shiftOn);
		return true;
		}

	if (ks->shiftOn)
		{
		ks->shiftOn = false;
		// Draw the changed key tops and unlit shift key
		KbdDraw(ks, kbdDrawKeyTopsOnly, kbdSelectModifiers);
		GrfSetState(ks->capsOn, false, ks->shiftOn);
		}
		
	EvtEnqueueKey(processedKey, 0, 0);
	return true;
	}


/***********************************************************************
 *
 * FUNCTION:    KbdSetLayout
 *
 * DESCRIPTION: Switch to a different keyboard layout.
 *
 *		Redraws the keyboard if it is visible.
 * 	The KeyboardStatus structure allows up to numKeyboards layouts.
 *
 * PARAMETERS:	KeyboardStatus *ks - the keyboard object
 *					UInt16 layout - the new layout to use
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			grant	06/23/00	Initial Revision
 *
 ***********************************************************************/
void KbdSetLayout(KeyboardStatus *ks, UInt16 layout)
{
	Boolean visible;
	
	// validate arguments
	ErrNonFatalDisplayIf(layout != kbdDefault && layout > numKeyboards, "invalid keyboard layout");
	if (ks == NULL) return;
	
	// If the keyboard is visible, erase the old layout
	// (We have to remember visible locally, because KbdErase sets
	// it to false in the keyboard object!)
	visible = KeyboardFlagTest(ks, KeyboardFlagVisible);
	if (visible)
	{
		KbdErase(ks);
	}
	
	// switch to the new layout
	// The default layout is qwerty (aka alpha).
	if (layout == kbdDefault)
		layout = kbdAlpha;
	ks->currentKeyboard = layout;
	
	// Redraw new layout if visible
	if (visible)
	{
		KbdDraw(ks, kbdDrawEverything, kbdSelectModifiers);
	}
}


/***********************************************************************
 *
 * FUNCTION:    KbdGetLayout
 *
 * DESCRIPTION: Return which keyboard layout is currently in use.
 *
 * 	The KeyboardStatus structure allows up to numKeyboards layouts.
 *
 * PARAMETERS:  KeyboardStatus * ks - the keyboard object
 *
 * RETURNED:    UInt16 - index of current layout, 0 <= x < numKeyboards
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			grant	06/23/00	Initial Revision
 *
 ***********************************************************************/
UInt16 KbdGetLayout(const KeyboardStatus *ks)
{
	if (ks == NULL)
		return 0;
	else
		return ks->currentKeyboard;
}


/***********************************************************************
 *
 * FUNCTION:    KbdSetPosition
 *
 * DESCRIPTION: Set the position of the keyboard object.
 *
 *		The position is where the keyboard is drawn relative to the
 *		containing form.
 *
 *		Redraws keyboard if it is currently visible.
 *
 * PARAMETERS:	KeyboardStatus *ks - the keyboard object
 *					PointType *p - the new position
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			grant	06/26/00	Initial Revision
 *
 ***********************************************************************/
void KbdSetPosition(KeyboardStatus *ks, const PointType *p)
{
	Boolean visible;
	
	// If no keyboard object or point is given, simply return.
	if (ks == NULL || p == NULL) return;
	
	// If the keyboard is visible, erase it from the old position.
	// (We have to remember visible locally, because KbdErase sets
	// it to false in the keyboard object!)
	visible = KeyboardFlagTest(ks, KeyboardFlagVisible);
	if (visible)
	{
		KbdErase(ks);
	}
	
	// Change the position
	ks->drawOffset.x = p->x;
	ks->drawOffset.y = p->y;
	
	// Redraw if visible
	if (visible)
	{
		KbdDraw(ks, kbdDrawEverything, kbdSelectModifiers);
	}
}


/***********************************************************************
 *
 * FUNCTION:    KbdGetPosition
 *
 * DESCRIPTION: Get the position of the keyboard object.
 *
 *		The position is where the keyboard is drawn relative to the
 *		containing form.
 *
 * PARAMETERS:	KeyboardStatus *ks - the keyboard object
 *					PointType *p - position is returned here
 *
 * RETURNED:	nothing
 *					[NOTE: PointType *p is an out parameter]
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			grant	06/26/00	Initial Revision
 *
 ***********************************************************************/
void KbdGetPosition(const KeyboardStatus *ks, PointType *p)
{
	// If no PointType is given (p is NULL), simply return.
	if (p == NULL)  return;
	
	// If no KeyboardStatus is given, return a default position of (0, 0).
	if (ks == NULL)
	{
		p->x = 0;
		p->y = 0;
	}
	else
	{
		// Otherwise, return the position.
		p->x = ks->drawOffset.x;
		p->y = ks->drawOffset.y;
	}
}


/***********************************************************************
 *
 * FUNCTION:    KbdSetShiftState
 *
 * DESCRIPTION: Set the shift state of the keyboard object.
 *
 *		Redraws keyboard if it is currently visible.
 *
 * PARAMETERS:	KeyboardStatus *ks - the keyboard object
 *					UInt16 shiftState - the new shift state;
 *						see shift state flags in Keyboard.h
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			grant	07/18/00	Initial Revision
 *
 ***********************************************************************/
void KbdSetShiftState(KeyboardStatus *ks, UInt16 shiftState)
{
	Boolean shift;
	Boolean capslock;
	
	// if no keyboard object is given, bail out
	if (ks == NULL)  return;
	
	shift = shiftState & KeyboardShiftFlag;
	capslock = shiftState & KeyboardCapslockFlag;
	
	// set the new state only if it has changed
	if (shift != ks->shiftOn || capslock != ks->capsOn)
	{
		ks->shiftOn = shift;
		ks->capsOn = capslock;
		
		// Redraw if keyboard is visible
		if (KeyboardFlagTest(ks, KeyboardFlagVisible))
		{
			KbdDraw(ks, kbdDrawKeyTopsOnly, kbdSelectModifiers);
		}
		
		// Update the graffiti shift state to match.
		GrfSetState(ks->capsOn, false, ks->shiftOn);
	}
}


/***********************************************************************
 *
 * FUNCTION:    KbdGetShiftState
 *
 * DESCRIPTION: Return the shift state of the keyboard object.
 *
 * PARAMETERS:  KeyboardStatus *ks - the keyboard object
 *
 * RETURNED:    UInt16 - shift state; see shift state flags in Keyboard.h
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			grant	07/18/00	Initial Revision
 *
 ***********************************************************************/
UInt16 KbdGetShiftState(const KeyboardStatus *ks)
{
	if (ks == NULL)
		return 0;
	else
		return KeyboardShiftState(ks->shiftOn, ks->capsOn);
}


/***********************************************************************
 *
 * FUNCTION:    KeyboardStatusNew
 *
 * DESCRIPTION: Allocate and initialize a keyboard object.
 *
 *		The keyboard object (pointed to by KeyboardStatus *) maintains
 *		all the runtime information for an on-screen keyboard.
 *
 *		This function gets and locks the keyboard resource (tkbd) with the given ID.
 *		If the keyboard resource is not found, or if the memory cannot
 *		be allocated, it returns NULL.
 *
 *		Caller must call KeyboardStatusFree on the returned KeyboardStatus *
 *		when done with the keyboard.
 *
 *		NOTE: assumes keyboard resource contains 3 layouts.
 *
 * PARAMETERS:  UInt16 keyboardID - ID of keyboard resource to use
 *
 * RETURNED:    KeyboardStatus * - the keyboard object
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			grant	06/13/00	Initial Revision
 *
 ***********************************************************************/
KeyboardStatus *KeyboardStatusNew(UInt16 keyboardID)
{
	KeyboardStatus *ks;
	MemHandle keyboardResourceH;
	KeyboardLayout *layout;
	
	// Get the keyboard resource
	keyboardResourceH = DmGetResource(kbdRscType, keyboardID);
	if (keyboardResourceH == NULL)
		return NULL;
	
	
	// Allocate the memory
	ks = MemPtrNew(sizeof(KeyboardStatus));
	if (ks == NULL)
		return NULL;
	
	
	// Set the default shift and caps lock states
	ks->shiftOn = false;
	ks->capsOn = false;
	
	
	// Use alpha as the default keyboard layout.
	ks->currentKeyboard = kbdAlpha;
	
	
	// Find the layouts
	ks->kbdResource = MemHandleLock(keyboardResourceH);
	layout = (KeyboardLayout *) ks->kbdResource;
	ks->layout[0] = layout;
	
	// Advance through the layouts to the next keyboard
	while (!layout->lastLayoutInKeyboard)
		layout++;
	layout++;
	ks->layout[1] = layout;
	
	// Advance through the layouts to the next keyboard
	while (!layout->lastLayoutInKeyboard)
		layout++;
	layout++;
	ks->layout[2] = layout;
	
	
	// Initial flags - all clear
	ks->flags = 0;
	
	// Set the position (default is 0,0 which means to draw relative
	// to the form origin).
	ks->drawOffset.x = 0;
	ks->drawOffset.y = 0;
	
	return ks;
}


/***********************************************************************
 *
 * FUNCTION:    KeyboardStatusFree
 *
 * DESCRIPTION: Release a KeyboardStatus.
 *
 *		Frees the memory and unlocks the keyboard resource.
 *
 * PARAMETERS:  KeyboardStatus *ks - the KeyboardStatus to free.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			grant	06/23/00	Initial Revision
 *
 ***********************************************************************/
void KeyboardStatusFree(KeyboardStatus *ks)
{
	if (ks != NULL)
	{
		MemPtrUnlock(ks->kbdResource);
		MemPtrFree(ks);
	}
}


/***********************************************************************
 *
 * FUNCTION:    KeyboardUpdateShiftState
 *
 * DESCRIPTION: This routine presses or releases the shift and caps lock
 *					 keys to correspond to the given shift state. This is used
 *					 when a graffiti shift stroke is detected, so that the
 *					 shift and caps lock keys always correspond to the graffiti
 *					 shift state.
 *
 * PARAMETERS:  KeyboardStatus	- pointer to keyboard status
 *					 capsOn				- new value of graffiti caps lock state
 *					 tempShift			- new value of graffiti temp shift state
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	12/02/99	Initial Revision
 *
 ***********************************************************************/
static void KeyboardUpdateShiftState (KeyboardStatus *ks, Boolean capsOn,
	UInt16 tempShift)
{
	ks->capsOn = capsOn;
	ks->shiftOn = (tempShift == grfTempShiftUpper) ||
		(tempShift == grfTempShiftLower);
	// Draw the changed key tops and lit shift key.
	KbdDraw(ks, kbdDrawKeyTopsOnly, kbdSelectModifiers);
}


/***********************************************************************
 *
 * FUNCTION:    KeyboardUpdateScrollers
 *
 * DESCRIPTION: This routine draws or erases the scroll arrow
 *              buttons.
 *
 * PARAMETERS:  frm - pointer to the Edit View form.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	7/31/95	Initial Revision
 *
 ***********************************************************************/
static void KeyboardUpdateScrollers (FormType * frm)
{
	FieldPtr fld;
	UInt16 upIndex;
	UInt16 downIndex;
	Boolean scrollableUp;
	Boolean scrollableDown;
		
	
	fld = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, KeyboardTextField));

	scrollableUp = FldScrollable (fld, winUp);
	scrollableDown = FldScrollable (fld, winDown);


	// Update the scroll button.
	upIndex = FrmGetObjectIndex (frm, KeyboardUpButton);
	downIndex = FrmGetObjectIndex (frm, KeyboardDownButton);
	FrmUpdateScrollers (frm, upIndex, downIndex, scrollableUp, scrollableDown);
}


/***********************************************************************
 *
 * FUNCTION:    KeyboardScroll
 *
 * DESCRIPTION: This routine scrolls the keyboard field a page or a 
 *              line at a time.
 *
 * PARAMETERS:  direction - up or dowm
 *              oneLine   - true if scrolling a single line
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	8/30/95	Initial Revision
 *
 ***********************************************************************/
static void KeyboardScroll (WinDirectionType direction, Boolean oneLine)
{
	UInt16 linesToScroll;
	FieldPtr fld;
	FormType * frm;
	
	
	frm = FrmGetActiveForm ();
	fld = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, KeyboardTextField));
	if (FldScrollable (fld, direction))
		{
		if (oneLine)
			linesToScroll = 1;
		else
			linesToScroll = FldGetVisibleLines (fld) - 1;
		FldScrollField (fld, linesToScroll, direction);
		KeyboardUpdateScrollers (frm);
		}
}


/***********************************************************************
 *
 * FUNCTION:    KeyboardFormDoCommand
 *
 * DESCRIPTION: This routine performs the menu command specified.
 * 				 The keyboard menu cannot be the system edit menu
 *					 because that has shortcuts which can have the keyboard
 *					 which would result in a recursive situation.
 *
 * PARAMETERS:  command  - menu item id
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	9/26/95	Initial Revision
 *
 ***********************************************************************/
static Boolean KeyboardFormDoCommand (UInt16 command)
{	
	FieldPtr fld;
	FormType * frm;
	UInt16 focus;


	frm = FrmGetActiveForm ();
	focus = FrmGetFocus (frm);
	if (focus == noFocus) return true;

	fld = FrmGetObjectPtr (frm, focus);
	if (! fld) return false;

	switch (command)
		{
		case sysKeyboardEditUndoCmd:
			FldUndo (fld);
			KeyboardUpdateScrollers (FrmGetActiveForm ());
			return true;

		case sysKeyboardEditCutCmd:
			FldCut (fld);
			KeyboardUpdateScrollers (FrmGetActiveForm ());
			return true;
		
		case sysKeyboardEditCopyCmd:
			FldCopy (fld);
			return true;
		
		case sysKeyboardEditPasteCmd:
			FldPaste (fld);
			KeyboardUpdateScrollers (FrmGetActiveForm ());
			return true;
		
		case sysKeyboardEditSelectAllCmd:
			FldSetSelection (fld, 0, FldGetTextLength (fld));
			return true;
		}
		
	return false;
}


/***********************************************************************
 *
 * FUNCTION:    KeyboardFormInit
 *
 * DESCRIPTION: Initialize the keyboard form.
 *
 * PARAMETERS:  systemKeyboard - the keyboard status info
 *
 * RETURNED:    frm - pointer to the Keyboard form.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	8/30/95	Initial Revision
 *			grant 5/5/99	Set insPtVisible = true in new text field attrs.
 *								True is the default value for insPtVisible.  At
 *								the point we set the attributes, it is correct
 *								because the field has no text so the insPt would
 *								be visible if the field had focus.
 *			peter	12/03/99	Never auto shift in keyboard dialog.
 *
 ***********************************************************************/
static FormType * KeyboardFormInit (UInt16 formID, OriginalFieldInfo *originalField, UInt16 kbdType)
{
	FormType * originalForm, * frm;
	UInt16 focusObj;
	FieldAttrType attr;
	MemHandle textH;
	FieldPtr textFieldP;
	ControlPtr	pControl;
	UInt16 scrollPosition;
	UInt16 insPtPosition;
	UInt16 startPosition;
	UInt16 endPosition;

	
	originalForm =  FrmGetActiveForm();
	focusObj = FrmGetFocus(originalForm);
	if (FrmGetObjectType(originalForm, focusObj) == frmFieldObj)
		{
		originalField->fieldP = FrmGetObjectPtr(originalForm, focusObj);
		}
	else
		{
		// A table has the focus.  Find it's field object.
		// The keyboard checks for a field in it before appearing.
		ErrFatalDisplayIf(FrmGetObjectType(originalForm, focusObj) != 
			frmTableObj, "An unhandled type of object has the focus.");
			originalField->fieldP = TblGetCurrentField(
			FrmGetObjectPtr(originalForm, focusObj));
		}
		
	frm = FrmInitForm (formID);
	
	// Now set the keyboard's text field to use the original text field
	textFieldP = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, KeyboardTextField));
	FldSetMaxChars(textFieldP, 
		FldGetMaxChars(originalField->fieldP));
		
	FldGetAttributes(originalField->fieldP, &attr);
	attr.visible = false;
	attr.hasFocus = false;
	attr.dynamicSize = false;
	attr.insPtVisible = true;  // true is the default value for insPtVisible
	attr.autoShift = false;		// never auto shift in keyboard dialog
	FldSetAttributes(textFieldP, &attr);
	
	FldSetFont(textFieldP, FldGetFont(originalField->fieldP));
	
	// Save these while the field has data
	scrollPosition = FldGetScrollPosition (originalField->fieldP);
	insPtPosition = FldGetInsPtPosition (originalField->fieldP);
	FldGetSelection(originalField->fieldP, &startPosition, &endPosition);
	FldSetSelection(originalField->fieldP, 0, 0);

	// Get the current height of the field, so that we can check later if the 
	// height has changed.
	originalField->height = GetFieldHeight (originalField, 
		FldGetTextPtr (originalField->fieldP));


	textH = FldGetTextHandle(originalField->fieldP);
	if (textH)
		{
		UInt32 longOffset;
		
		longOffset = (UInt32) FldGetTextPtr(originalField->fieldP);
		longOffset -= (UInt32) MemHandleLock(textH);
		MemHandleUnlock(textH);
		originalField->offset = (UInt16) longOffset;
		originalField->blockSize = FldGetTextAllocatedSize (
			originalField->fieldP);
		}
	else
		{
		originalField->offset = 0;
		originalField->blockSize = 0;
		}
	
	// Remove the text field from the original field so it
	// unlocks it and set the new field to use the text.
	FldSetTextHandle(originalField->fieldP, 0);
	FldSetText(textFieldP, textH, originalField->offset, 
		originalField->blockSize);
		
	// Setting the text of a field will clear its dirty attribute,  so we have to
	// reset it.
	FldSetDirty (textFieldP, (attr.dirty == true));

	// Set the scroll position and insertion point to the same place
	// as in the edited field.
	FldSetScrollPosition(textFieldP, scrollPosition);
	if (startPosition == endPosition)
		FldSetInsPtPosition(textFieldP, insPtPosition);
	else
		FldSetSelection(textFieldP, startPosition, endPosition);
	

	FrmSetFocus(frm, FrmGetObjectIndex (frm, KeyboardTextField));


	// Set the highlight the pushbutton of the keyboard in use		
	pControl = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, KeyboardAbcPushBtn) + kbdType);
	CtlSetValue(pControl, true);

	KeyboardUpdateScrollers(frm);

	return frm;			// return the keyboard form
}


/***********************************************************************
 *
 * FUNCTION:    KeyboardRestoreGraffitiState
 *
 * DESCRIPTION: Switching focus causes shift state to be lost, so we
 *					 use this routine to reset it based on the keyboard state.
 *
 * PARAMETERS:  systemKeyboard - the keyboard status info
 *
 * RETURNED:    void
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter 12/03/99	Initial Revision
 *
 ***********************************************************************/
static void KeyboardRestoreGraffitiState(KeyboardStatus *systemKeyboard)
{
	PointType point, startPt, endPt;

	if (systemKeyboard->capsOn)
		GrfSetState(true, false, false);
	else if (systemKeyboard->shiftOn)
	{
		// Can't just call GrfSetState since that'd turn on autoshift.
		// Instead, we pretend the user just entered the upShift stroke.
		EvtFlushPenQueue();
		// DOLATER - shouldn't this be based on the screen size, not hard-coded to 100?
		point.x = 100; point.y = 100;		//absolute position of stroke irrelevant
		PenScreenToRaw(&point);
		EvtEnqueuePenPoint(&point);
		point.x = 100; point.y = 0;
		PenScreenToRaw(&point);
		EvtEnqueuePenPoint(&point);
		point.x = -1; point.y = -1;		// terminate the stroke
		EvtEnqueuePenPoint(&point);
		// Prepare to process stroke.
		EvtDequeuePenStrokeInfo(&startPt, &endPt);	//start and end ignored
		GrfProcessStroke(0, 0, false);
	}
	else
		GrfSetState(false, false, false);
}


/***********************************************************************
 *
 * FUNCTION:    KeyboardFormHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the Keyboard Form
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *					 systemKeyboard - the keyboard status info
 *
 * RETURNED:    true if the event has MemHandle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			rsf	3/29/95	Initial Revision
 *			peter 12/03/99	Restore grf state after switching focus, don't forget when switching kbds
 *
 ***********************************************************************/
 
static Boolean KeyboardFormHandleEvent (EventType * event)
{
	FormType * frm;
	Boolean handled = false;
	EventType newEvent;
	KeyboardStatus *systemKeyboard;


	frm = FrmGetActiveForm ();
	systemKeyboard = FrmGetGadgetData (frm, FrmGetObjectIndex (frm, 
		KeyboardGadget));

	if (KbdHandleEvent (systemKeyboard, event))
		return true;
		
		
	else if (event->eType == ctlRepeatEvent)
		{		
		switch (event->data.ctlRepeat.controlID)
			{
			case KeyboardUpButton:
				KeyboardScroll (winUp, true);
				// leave unhandled so the buttons can repeat
				break;
				
			case KeyboardDownButton:
				KeyboardScroll (winDown, true);
				// leave unhandled so the buttons can repeat
				break;
			}
		}


	else if (event->eType == ctlSelectEvent)
		{
		if (event->data.ctlSelect.controlID == KeyboardDoneButton)
			{
			// Set the event to close so the parent routine closes
			// the dialog.			
			MemSet (&newEvent, sizeof(EventType), 0);
			newEvent.eType = frmCloseEvent;
			newEvent.data.frmOpen.formID = kKeyboardFormResID;
			EvtAddEventToQueue (&newEvent);
			return true;
			}
				
			
		//	We no longer turn off shift or caps lock when switching keyboards because
		// the user can now see the shift state in all keyboards via the shift indicator.
		//		systemKeyboard->shiftOn = false;
		//		systemKeyboard->capsOn = false;
			
		// erase the existing keyboard
		KbdErase(systemKeyboard);

		switch (event->data.ctlSelect.controlID)
			{
			case KeyboardAbcPushBtn:
				systemKeyboard->currentKeyboard = kbdAlpha;				
				break;
				
			case Keyboard123PushBtn:
				systemKeyboard->currentKeyboard = kbdNumbersAndPunc;
				break;
				
			case KeyboardIntlPushBtn:
				systemKeyboard->currentKeyboard = kbdAccent;
				break;	
			}
		
		// Draw the current keyboard.
		KbdDraw(systemKeyboard, kbdDrawEverything, kbdSelectModifiers);
		}


	else if (event->eType == keyDownEvent)
		{
	 	if	(	(!TxtCharIsHardKey(	event->data.keyDown.modifiers,
											event->data.keyDown.chr))
			&&	(EvtKeydownIsVirtual(event))
			&&	(	(event->data.keyDown.chr == vchrPageUp)
				||	(event->data.keyDown.chr == vchrPageDown)))
			{
			if (event->data.keyDown.chr == vchrPageUp)
				{
				KeyboardScroll (winUp, false);
				}
			else
				{
				KeyboardScroll (winDown, false);
				}
			handled = true;
			}
		else
			{
			frm = FrmGetActiveForm ();
			FrmHandleEvent (frm, event);

			KeyboardUpdateScrollers(frm);
			handled = true;
			}
		}


	else if (event->eType == menuEvent)
		{
		return KeyboardFormDoCommand (event->data.menu.itemID);
		}

		
	else if (event->eType == frmCloseEvent)
		{
		return true;
		}
		
	else if (event->eType == frmOpenEvent)
		{
		frm = FrmGetActiveForm ();
		FrmDrawForm (frm);
		KbdDraw(systemKeyboard, kbdDrawEverything, kbdSelectModifiers);
		FrmSetFocus(frm, FrmGetObjectIndex (frm, KeyboardTextField));
		// Switching focus causes shift state to be lost, so we reset it
		// based on the keyboard state.
		KeyboardRestoreGraffitiState(systemKeyboard);
		handled = true;
		}
		
	else if (event->eType == frmUpdateEvent)
		{
		frm = FrmGetActiveForm ();
		FrmDrawForm (frm);
		KbdDraw(systemKeyboard, kbdDrawEverything, kbdSelectModifiers);
		handled = true;
		}
		
	return (handled);
}


/***********************************************************************
 *
 * FUNCTION:    SysKeyboardDialogV10
 *
 * DESCRIPTION: This routine pops up the system keyboard if there
 * 	is a field object with the focus.  The field object's text
 *		chunk is edited directly.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    The field's text chunk is changed.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			rsf	4/27/95	Initial Revision
 *			rsf	6/25/96	Changed to call new version
 *
 ***********************************************************************/
extern void SysKeyboardDialogV10 ()
{
	SysKeyboardDialog(kbdAlpha);
}

/************************************************************
 *
 *  FUNCTION: PrvLaunchGraffitiDemo()
 *
 *  DESCRIPTION: Launches the Graffiti Demo App
 *
 *  PARAMETERS: 
 *
 *  RETURNS: void
 * 
 *  CALLED BY: SysKeyboardDialog
 *
 *  CREATED: 1/7/97
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
static void		PrvLaunchGraffitiDemo(void)
{
	DmSearchStateType	state;
	Err					err;
	UInt16				cardNo;
	LocalID				dbID;
	
	err = DmGetNextDatabaseByTypeCreator(true, &state, 
		sysFileTApplication, sysFileCGraffitiDemo, true, &cardNo, &dbID);
		
	// If not found, return;
	if (err)  return;

	// Otherwise, try and switch apps.
	if (dbID) 
		SysUIAppSwitch(cardNo, dbID, 0, 0);
}


/***********************************************************************
 *
 * FUNCTION:    SysKeyboardDialog
 *
 * DESCRIPTION: This routine pops up the system keyboard if there
 * 	is a field object with the focus.  The field object's text
 *		chunk is edited directly.
 *
 * PARAMETERS:
 *		kdbType	-> The requested keyboard type (alpha, numeric, etc)
 *
 * RETURNED:    nothing
 *
 * HISTORY:
 *		04/27/95	rsf	Created by Roger Flores.
 *		12/02/99	peter	Update kbd shift state when grf shift state changes.
 *		12/03/99	peter	Restore grf shift state when closing dialog.
 *		08/28/00	kwk	Use new FrmGetActiveField routine.
 *
 ***********************************************************************/
extern void SysKeyboardDialog (KeyboardType kbdType)
{
	EventType event;
	FormType * frm, * originalForm;
	WinHandle winH;
	MemHandle textFieldH;
	FieldPtr textFieldP;
	FieldAttrType attr;
	Boolean textFieldDirty;
	OriginalFieldInfo originalField;
	KeyboardStatus *systemKeyboard;
	PointType keyboardPos;
	Boolean capslock;
	Boolean numlock;
	UInt16 tempShift;
	Boolean ignored;
	Boolean shift;
	UInt16 newScrollPosition, newInsPtPosition;
	Char * textP;
	Int16 newHeight;
	UInt16 error;
	UInt16 startPosition;
	UInt16 endPosition;
	Boolean expectedCapsOn;
	Boolean newCapsOn;
	UInt16 expectedTempShift, newTempShift;

	if (GSysMiscFlags & sysMiscFlagGrfDisable) {
		if (!(GSysMiscFlags & sysMiscFlagInDemoAlert)) {
			GSysMiscFlags |= sysMiscFlagInDemoAlert;
			if (FrmAlert(DemoUnitAlert) == 0)  	// Go To Graffiti app
				PrvLaunchGraffitiDemo();
			GSysMiscFlags &= ~sysMiscFlagInDemoAlert;
			}
		return;
		}

	// Compare the active form's window to the active window
	// This should prevent
	originalForm =  FrmGetActiveForm();
	winH = WinGetActiveWindow();
	if (winH != WinGetWindowHandle(originalForm))
		{
		SndPlaySystemSound(sndError);
		return;
		}
		
	// If there isn't a text field with the focus set then leave
	textFieldP = FrmGetActiveField(originalForm);
	if (textFieldP == NULL)
		{
		SndPlaySystemSound(sndError);
		return;
		}
	
	// Now make sure that the field is editable.
	FldGetAttributes(textFieldP, &attr);
	if (!attr.editable || !attr.usable)
		{
		SndPlaySystemSound(sndError);
		return;
		}


	// The text field has passed our checks that editting it is valid.
	
	// We no longer turn off Graffiti while the keyboard is active.
	//			EvtEnableGraffiti(false);

	// Turn off the blinking insertion point
	InsPtEnable (false);


	// Set up the onscreen keyboard
	systemKeyboard = KeyboardStatusNew(kSystemKeyboardResID);
	ErrNonFatalDisplayIf(systemKeyboard == NULL, "Missing system keyboard resource.");

	// Set the position relative to the form
	keyboardPos.x = keyboardDrawOffsetX;
	keyboardPos.y = keyboardDrawOffsetY;
	KbdSetPosition(systemKeyboard, &keyboardPos);
	
	// Get the caps lock and shift states from Graffiti
	GrfGetState(&capslock, &numlock, &tempShift, &ignored);
	shift = (tempShift == grfTempShiftUpper) || (tempShift == grfTempShiftLower);
	
	KbdSetShiftState(systemKeyboard, KeyboardShiftState(shift, capslock));
	
	// Pick a keyboard if kbdType is kbdDefault
	if (kbdType == kbdDefault)
	{
		if (attr.numeric || numlock)
			kbdType = kbdNumbersAndPunc;
		else
			kbdType = kbdAlpha;
	}
	KbdSetLayout(systemKeyboard, kbdType);
	
	// Initialize the form
	frm = KeyboardFormInit (kKeyboardFormResID, &originalField, KbdGetLayout(systemKeyboard));
	FrmSetActiveForm (frm);

	// Store the keyboard info in the keyboard gadget.
	FrmSetGadgetData (frm, FrmGetObjectIndex (frm, KeyboardGadget), 
		systemKeyboard);

	// Set the event handler.
	FrmSetEventHandler (frm, KeyboardFormHandleEvent);
	
	// make a frmOpenEvent for the keyboard form.
	MemSet (&event, sizeof(EventType), 0);
	event.eType = frmOpenEvent;
	event.data.frmOpen.formID = kKeyboardFormResID;
	EvtAddEventToQueue (&event);


	// Event loop for the system keyboard form
	while (true)
		{
		EvtGetEvent (&event, evtWaitForever);
		
		// Remember old graffiti state so we can detect shifting strokes.
		GrfGetState(&expectedCapsOn, &numlock, &expectedTempShift, &ignored);

		if (!(event.eType == keyDownEvent && 
			(event.data.keyDown.chr == keyboardChr ||
			event.data.keyDown.chr == keyboardAlphaChr ||
			event.data.keyDown.chr == keyboardNumericChr)))
			if (SysHandleEvent((EventType *)&event))
			{
				// Check graffiti state so we can detect shifting strokes.
				GrfGetState(&newCapsOn, &numlock, &newTempShift, &ignored);
				if (newCapsOn != expectedCapsOn || newTempShift != expectedTempShift)
					KeyboardUpdateShiftState(systemKeyboard, newCapsOn, newTempShift);
			}
			else
				if (! MenuHandleEvent (0, &event, &error))
					FrmDispatchEvent (&event); 
				
		if ((event.eType == frmCloseEvent && 
			event.data.frmClose.formID == kKeyboardFormResID))
			break;
			
		if (event.eType == appStopEvent)
			{
			EvtAddEventToQueue (&event);
			break;
			}
		}


	// Remove the keyboard form.  The user may have pressed the Ok button or
	// they may be switching to another application.
	frm = FrmGetActiveForm();
	
	// Get the data from the keyboard's text field before the from is deleted
	textFieldP = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, KeyboardTextField));
	textFieldH  = FldGetTextHandle(textFieldP);
	textFieldDirty = FldDirty(textFieldP);
	newScrollPosition = FldGetScrollPosition(textFieldP);
	newInsPtPosition = FldGetInsPtPosition(textFieldP);
	FldGetSelection(textFieldP, &startPosition, &endPosition);
	originalField.blockSize = FldGetTextAllocatedSize (textFieldP);
	FldSetTextHandle(textFieldP, 0);		// Unlocks the text string MemHandle
	

	// First set the original text object to match how
	// the user has modified the keyboard's text object
	FldSetText(originalField.fieldP, textFieldH,
		originalField.offset, 
		originalField.blockSize);
	FldSetDirty(originalField.fieldP, textFieldDirty);

	// Do this only after the original text handle is put back.  Sometimes the field is 
	// in a table, and the table expects the field to be valid when it draws in FrmEraseForm.
	FrmEraseForm (frm);
	FrmDeleteForm (frm);

	FrmSetActiveForm (originalForm);

		
	// Check if the height if the field has changed,  if it has then send a fldHeightChange
	// event,  if is has not changed then send a fldChange event.
	textP = FldGetTextPtr (originalField.fieldP);
	newHeight = GetFieldHeight (&originalField, textP);
	FldGetAttributes(originalField.fieldP, &attr);
	if (attr.singleLine || (! attr.dynamicSize) || originalField.height == newHeight)
		{
		FldSetScrollPosition (originalField.fieldP, newScrollPosition);
		if (startPosition == endPosition)
			FldSetInsPtPosition (originalField.fieldP, newInsPtPosition);
		else
			InsPtEnable(false);
		FldDrawField (originalField.fieldP);
		FldSendChangeNotification (originalField.fieldP);
		}
	else
		{
		FldRecalculateField (originalField.fieldP, true);
		FldSendHeightChangeNotification (originalField.fieldP, 
			newInsPtPosition, newHeight);
		}
	FldSetSelection(originalField.fieldP, startPosition, endPosition);

	// Switching focus causes shift state to be lost, so we reset it
	// based on the keyboard state.
	// There is some debate as to whether we should do this. In Palm OS 3.3 and earlier,
	// this wasn't done.
	KeyboardRestoreGraffitiState(systemKeyboard);

	KeyboardStatusFree(systemKeyboard);
	EvtEnableGraffiti(true);
}
