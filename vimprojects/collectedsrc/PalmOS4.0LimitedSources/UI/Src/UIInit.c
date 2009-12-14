/******************************************************************************
 *
 * Copyright (c) 1995-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: UIInit.c
 *
 * Release: 
 *
 * Description:
 * 	This file contains routines to initialize the user interface.
 *
 * History:
 *		02/03/99	bob	Created by Bob Ebert
 *		04/26/99	BRM	Merged UIColor into UIInit.
 *		08/09/00 peter	Moved AttnIndicatorInitialize from UIReset to UIInitialize.
 *
 *****************************************************************************/

#define NON_PORTABLE

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_CLIPBOARDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_FIELDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS

#include <PalmTypes.h>
#include <SystemPublic.h>

#if EMULATION_LEVEL == EMULATION_NONE
#include <FatalAlert.h>			// For SysFatalAlertInit
#endif

#include "FontPrv.h"
#include "HwrGlobals.h"
#include "HwrMiscFlags.h"
#include "SystemResourcesPrv.h"
#include "TextServicesPrv.h"
#include "IntlPrv.rh"			// For kFontIndexResID
#include "AttentionPrv.h"

#include "UIInit.h"
#include "UIGlobals.h"
#include "UIColorPrv.h"
#include "UIResourcesPrv.h"

extern void PrvInitWindowVariables (void);	// in WindowColor.c

void PrvUpdateUIColorTableIndexValues(void);

static Err  PrvDepthChangeNotification(SysNotifyParamType *notify);
static void PrvLoadUIColorTable (UInt32 depth);

void ThisShouldGenerateALinkErrorUIInit();



/********************************************************************
 * This pointer is created under emulation mode just to make it easier
 * to look at the UI globals from the source level debugger
 *******************************************************************/
#if EMULATION_LEVEL != EMULATION_NONE
	UIGlobalsPtr	UIGlobalsP = 0;
#endif




/***********************************************************************
 *
 * FUNCTION:    UIInitialize
 *
 * DESCRIPTION: This routine returns the decimal seperator of the 
 *              default country (the country specified in the 
 *              Preferences application).
 *
 * PARAMETERS:  none
 *
 * RETURNED:    decimal seperator
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/5/97	Initial Revision
 *			CS		08/08/00	Removed code that loads 'cnty' resource, since
 *								it's obsolete, and we weren't using it anyway.
 *
 ***********************************************************************/
static Char UIGetDecimalSeparator (void)
{
	Char 			thousandSeparator;
	Char 			decimalSeparator;

	// Get the decimal separator for the home country and store it in 
	// a global variable.
	LocGetNumberSeparators (
		(NumberFormatType) PrefGetPreference (prefNumberFormat), 
		&thousandSeparator, &decimalSeparator);

	return (decimalSeparator);
}


/***********************************************************************
 *
 * FUNCTION:    UILoadFonts
 *
 * DESCRIPTION: This routine load the font resources. 
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	03/02/98	Initial Revision
 *			vivek 06/13/00 Allocate app sub-font table
 *
 ***********************************************************************/
static void UILoadFonts (void)
{
	MemHandle				rscH;
	MemPtr					rscP;
	Int16					count;
	FontIndexEntryType *	tableP;
	MemHandle				fontH;
	UInt16					fontTableSize;
	UInt16					extendedEntries;
	UInt16 *				listP;

	// Get the resource that contains the list of system font resources.
	rscH = DmGetResource (fontIndexType, kFontIndexResID);
	rscP = MemHandleLock (rscH);

	// Get the number of entries in the font table
	UINumSysFonts = *(Int16 *)rscP;
	
	// Allocate the font tables	
	UISysFontTablePtr = MemPtrNew(UINumSysFonts * sizeof(FontPtr));

	// Load the system font tables and initialize the font global variables.
	tableP = (FontIndexEntryPtr) (((UInt8 *)rscP) + sizeof(UInt16));
	for (count = 0; count < UINumSysFonts; count++)
		{
		fontH = DmGetResource (tableP[count].rscType, tableP[count].rscID);
		ErrNonFatalDisplayIf (!fontH , "System font resource not found");
		if (fontH)
			UISysFontPtr[count] = (FontPtr) MemHandleLock(fontH);
		else
			UISysFontPtr[count] = NULL;
		}

	UICurrentFontPtr = UISysFontPtr[stdFont];
	UICurrentFontID = stdFont;

  	// If the font index table contains extended (hierarchical) fonts,
	// then allocate the separate list of sub-font pointers.
	fontTableSize = sizeof(UInt16) + (sizeof (FontIndexEntryType) * UINumSysFonts);
	extendedEntries =  *(UInt16 *)(((UInt8 *)rscP) + fontTableSize);

	if (extendedEntries)
		{
		UIFontListPtr = MemPtrNew(extendedEntries * sizeof(FontPtr));

		listP = (UInt16 *) (((UInt8 *)rscP) + fontTableSize + sizeof (UInt16));
		for (count = 0; count < extendedEntries; count++)
			{
			fontH = DmGetResource (fontRscType, listP[count]);
			ErrNonFatalDisplayIf (!fontH , "System extended font resource not found");
			if (fontH)
				UIFontListPtr[count] = MemHandleLock (fontH);
			else
				UIFontListPtr[count] = NULL;
			}
		}

	MemHandleUnlock (rscH);
	DmReleaseResource (rscH);
	
	// Initialize the app font table.
	UIAppFontTablePtr = MemPtrNew(UIDefNumAppFonts * sizeof(FontPtr));
	UINumAppFonts = UIDefNumAppFonts;
	
	for (count = 0; count < UINumAppFonts; count++)
		{
		UIAppFontPtr[count] = NULL;
	 	}
	 	
	// Initialize the application-defined sub-fonts table (just one entry to start with)
	GAppSubFontListPtr = MemPtrNew(sizeof(FontPtr));
	((FontTablePtr)GAppSubFontListPtr)[0] = NULL;
}


/***********************************************************************
 *
 * FUNCTION:    UIInitialize
 *
 * DESCRIPTION: This routine initialize the touchdown ui.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * HISTORY:
 *		01/25/95	art	Created by Art Lamb.
 *		12/10/97	SCL	Rewrote to support new (separate) font tables
 *		07/20/98	roger	Moved code to allocate from UIReset.
 *		07/12/99	kwk	Call SysFatalAlertInit.
 *		07/04/00	kwk	Call TsmInit vs. UILoadTextServices.
 *
 ***********************************************************************/
void UIInitialize (void)
{
	// Make sure we haven't outgrown the space reserved for the UI globals
	if (sizeof(UIGlobalsType) > sysUIRsvGlobalsSize)
		ThisShouldGenerateALinkErrorUIInit();
		
		
	// In Emulation mode, save pointer to UI globals in an applicaiton global 
	//  for easier viewing in the Source level debugger
	#if EMULATION_LEVEL != EMULATION_NONE
	UIGlobalsP = (UIGlobalsPtr)GUIGlobalsP;
	#endif
	

	// In Emulation mode, these structures are already initialized to values 
	// which use resources included in the emulation app (to import app.tres
	// and sys.tres).  Now we write in values to use the resources from the
	// app.tres and sys.tres files.

	// Load the font tables
	UILoadFonts ();

	// Load and open the Text Services Library.
	TsmInit();

	// Only true while all forms are being closed.
	AllFormsClosing = false;
	
	// Get the decimal separator for the home country and store it in 
	// a global variable.
	UIDecimalSeparator = UIGetDecimalSeparator ();
	
	// Allocate the event queue
	EventQ = (SysEventStoreType*)MemPtrNew(eventQueueSize * sizeof(SysEventStoreType));
	SysEventInitialize();
	
	// Allocate the field package's undo buffer.
	UndoGlobals.buffer = (Char *) MemPtrNew(undoBufferSize);

	// If necessary, switch back to the 'default' system display mode
	WinScreenMode(winScreenModeSetToDefaults, NULL, NULL, NULL, NULL);	
	
	// Reset the contrast value back to the default, if on a unit
	// that supports it.
	if (GHwrMiscFlags & hwrMiscFlagHasSWContrast) 
		{
		SysLCDContrast(/*set = */ true, SysLCDContrast(/*set = */false, 0));
		}

	// Initialize the UI colors
	UIColorInit();
	
	// This code is also in UIReset.  It was included here because we need to
	// have the UI in a semi usable state in case a fatal alert occurs before
	// UIReset is called.
	DisplayWindow = RootWindow;  
	WinSetDrawWindow(RootWindow);	// do updating of dependent values
	WinResetClip ();
	WinSetDrawMode(winPaint);
	
	// Now we can initialize the Fatal Alert code in a localizable manner.
	// This doesn't exist for the simulator, since the fatal alert code
	// is all stubbed out.
	#if EMULATION_LEVEL == EMULATION_NONE
	SysFatalAlertInit();
	#endif

	// Initialize the attention manager and indicator.
	AttnInitialize();
}


/***********************************************************************
 *
 * FUNCTION:    UIReset
 *
 * DESCRIPTION: This routine reset the ui. It should be called before an
 *              application is launched.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/25/95	Initial Revision
 *			SCL	12/10/97	Rewrote to support new (separate) app font table
 *			roger	7/20/98	Moved code to allocate to UIInitialize.
 *								Moved clip and pattern code from InitWindowVariables here.
 *			SCL	11/19/98	Resize and re-init the app font table
 *			bob	3/5/99	Updates to reset color-related values
 *			bob	09/23/99	Add debug asserts for popping drawstate, colortables
 *
 ***********************************************************************/
void UIReset (void)
{
	Err				err;
	Int16				count;
	UInt32 			depth;

	// Allocate the globals that go in the Dynamic heap.
	
	// Verify that the event queue is empty or has a stop event.
	// This check found some wierd cases.  I suggest this get explored again
	// someday.
//	ErrNonFatalDisplayIf(EventQLength > 1 ||
//		(EventQLength == 1 && EventQ[EventQIndex].event.eType != appStopEvent), 
//		"Event queue not empty");
	
	// Reset the key mask.
	KeySetMask(keyBitsAll);
	
	// Reset the undo globals.  This could be unnessary because the globals are 
	// reset whenever a field gets text or is entered.
//	UndoReset();

	// Initialize the Event Manager
	SysEventInitialize();
	
	// Reset the current font
	UICurrentFontID = stdFont;

	// Reset the app font table to the default size
	err = MemPtrResize(UIAppFontTablePtr, UIDefNumAppFonts * sizeof(FontPtr));
	if (!err)	// (shouldn't fail, but just in case...)
		{
		UINumAppFonts = UIDefNumAppFonts;
		}

	// Initialize the app font table.
	for (count = 0; count < UINumAppFonts; count++) 
		{
		UIAppFontPtr[count] = NULL;
		}

	// Set these to the root window.  These globals can be incorrect if
	// the window they pointed to was deleted by PalmOS:SysAppExit when 
	// the UI app exited.
	DisplayWindow = RootWindow;  
	WinSetDrawWindow(RootWindow);	// do updating of dependent values
	FirstWindow = RootWindow;
	EnterWindowID = 0;
	ExitWindowID = 0;
	
	// Initialize the form manager.
	CurrentForm = 0;
	
	// Initialize the insertion point manager
	InsPtInitialize();
	
	// Initialize the menu manager
	UICurrentMenu = 0;
	UICurrentMenuRscID = 0;
	
	//Initialize the command bar
	MenuCmdBarCurrent = 0;
	
	// Get the decimal separator for the home country and store it in 
	// a global variable.
	UIDecimalSeparator = UIGetDecimalSeparator ();

	// If necessary, switch back to the 'default' system display mode
	WinScreenMode(winScreenModeSetToDefaults, NULL, NULL, NULL, NULL);

	// Set the draw state variables back to defaults
	ErrNonFatalDisplayIf(GState.drawStateIndex, "Window Draw State stack not empty");
	GState.drawStateIndex = 0;
	GState.drawStateP = &GState.drawStateStackP[0];
	
	// Reset window/graphic state stuff.
	WinResetClip ();
	WinSetDrawMode(winPaint);
	WinSetUnderlineMode(noUnderline);
	WinSetPatternType(noPattern);
	FntSetFont(stdFont);
	
	// Set the UI Color tables back to user defaults
	ErrNonFatalDisplayIf(UICState.colorTableStackIndex, "UI Color Table stack not empty");
	UICState.colorTableStackIndex = 0;
	UICState.colorTableP = &UICState.colorTableStackP[0];
	
	WinScreenMode(winScreenModeGet, NULL, NULL, &depth, NULL);
	PrvLoadUIColorTable(depth);
	
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	// debug ROMs only, erase screen to funny color to help detect
	// apps that don't draw right.
	WinSetBackColor(UIColorGetIndex(UICaution));
#else
	WinSetBackColor(UIColorGetIndex(UIFormFill));
#endif

	WinEraseWindow();

	// initialize pens to 'useful' values for default drawing
	WinSetForeColor(UIColorGetIndex(UIObjectForeground));
	WinSetTextColor(UIColorGetIndex(UIObjectForeground));
	
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	// make sure background color is always correct, even on debug
	WinSetBackColor(UIColorGetIndex(UIFormFill));
#endif
}


/***********************************************************************
 *
 * FUNCTION:    UIColorInit
 *
 * DESCRIPTION: Initialize the UI color stacks and values
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	2/3/99	Initial Revision
 *
 ***********************************************************************/
Err UIColorInit(void)
{
	UInt32 depth;
	
	// allocate UI color table stack & initialize UICState
	UICState.colorTableStackP = MemPtrNew(UIColorTableStackSize * sizeof(UIColorTableEntryType));
	if (UICState.colorTableStackP == NULL)
		return memErrNotEnoughSpace;
		
	UICState.colorTableStackIndex = 0;
	UICState.colorTableP = &UICState.colorTableStackP[UICState.colorTableStackIndex * sizeof(UIColorTableEntryType)];

	SysNotifyRegister(0, sysNotifyNoDatabaseID, sysNotifyDisplayChangeEvent,
							&PrvDepthChangeNotification, -64, 0);

	WinScreenMode(winScreenModeGet, NULL, NULL, &depth, NULL);
	PrvLoadUIColorTable(depth);
	
	UIOptions.drawBordersAsLines = true;
	UIOptions.reserved = false;

	return 0;
}


/***********************************************************************
 *
 * FUNCTION:    UIColorGetTableEntryIndex
 *
 * DESCRIPTION: Get the index value for the UI color at the current
 *					 (screen) depth.
 *
 * PARAMETERS:  which	->	UI color table value to get
 *
 * RETURNED:    index value of the UI color
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	2/3/99	Initial Revision
 *
 ***********************************************************************/
IndexedColorType UIColorGetTableEntryIndex(UIColorTableEntries which)
{
	return UICState.colorTableP[which].index;
}


/***********************************************************************
 *
 * FUNCTION:    UIColorGetTableEntryRGB
 *
 * DESCRIPTION: Get the RGB value for the UI color, valid across any
 *					 (screen) depth.
 *
 * PARAMETERS:  which	->	UI color table value to get
 *					 rgbP		<-	set to RGB value of color table entry
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	2/3/99	Initial Revision
 *
 ***********************************************************************/
void UIColorGetTableEntryRGB(UIColorTableEntries which, RGBColorType *rgbP)
{
	*rgbP = UIColorToRGBColor(UICState.colorTableP[which]);
	rgbP->index = 0;		// normalize RBG value
}


/***********************************************************************
 *
 * FUNCTION:    UIColorSetTableEntry
 *
 * DESCRIPTION: Sets the value of a UI color entry to the passed RGB
 *					 value.  Also updates the index to the (current) best
 *					 fit for that RGB value from the palette for the current
 *					 draw window.  Probably best if draw window is on-screen.
 *
 * PARAMETERS:  which -> the entry to change
 *					 rgbP  -> the new value, the 'index' elt of the struct is ignored
 *
 * RETURNED:    error (none currently)
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	2/3/99	Initial Revision
 *
 ***********************************************************************/
Err UIColorSetTableEntry(UIColorTableEntries which, const RGBColorType *rgbP)
{
	UICState.colorTableP[which] = RGBColorToUIColor(*rgbP);
	UICState.colorTableP[which].index = WinRGBToIndex(rgbP);
	return 0;
}


/***********************************************************************
 *
 * FUNCTION:    UIColorPushTable
 *
 * DESCRIPTION: Push a copy of the UI color table on the UI color table
 *					 stack.  Allows apps to modify the new copy and then quickly
 *					 restore the old set upon return.
 *					 
 *					 Note: this is not recommended nor used currently!
 *
 * RETURNED:    0 if no error, -1 if the stack overflowed.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	2/3/99	Initial Revision
 *
 ***********************************************************************/
Err UIColorPushTable(void)
{
	Err err = 0;
	
	if (UICState.colorTableStackIndex < UIColorTableStackSize) {
		if (UICState.tablesDirty)
			PrvUpdateUIColorTableIndexValues();

		UICState.colorTableStackIndex += UILastColorTableEntry;
		UICState.colorTableP = &UICState.colorTableStackP[UICState.colorTableStackIndex];
		MemMove(UICState.colorTableP,
				  &UICState.colorTableStackP[UICState.colorTableStackIndex-UILastColorTableEntry],
				  sizeof(UIColorTableEntryType) * UILastColorTableEntry);
	}
	else
		err = -1;	// еее DOLATER invent a "stack overflow" error code

Exit:
	ErrNonFatalDisplayIf(err, "UI Color stack overflow");
	return err;
}


/***********************************************************************
 *
 * FUNCTION:    UIColorPopTable
 *
 * DESCRIPTION: Opposite of UIColorPushTable.  Restore to previously
 *					 saved state.
 *
 *					 Note: this is not recommended nor used currently!
 *
 * RETURNED:    0 for no error, -1 for stack underflow
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	2/3/99	Initial Revision
 *
 ***********************************************************************/
Err UIColorPopTable(void)
{
	Err err = 0;
	
	if (UICState.colorTableStackIndex > 0) {
		UICState.colorTableStackIndex -= UILastColorTableEntry;
		UICState.colorTableP = &UICState.colorTableStackP[UICState.colorTableStackIndex];
	}
	else
		err = -1;	// еее DOLATER invent a "stack underflow" error code

Exit:
	ErrNonFatalDisplayIf(err, "UI Color stack underflow");
	return err;
}


/***********************************************************************
 *
 * FUNCTION:    PrvUpdateUIColorTableIndexValues
 *
 * DESCRIPTION: Refresh the 'cached' index values of each color table
 *					 entry for the current UI color table set (stack frame)
 *					 by converting their RGB values.  Typically done only
 *					 when a new table is loaded (e.g. pop) or when the palette
 *					 changes.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	2/3/99	Initial Revision
 *
 ***********************************************************************/
void PrvUpdateUIColorTableIndexValues (void)
{
	Int16 i;
	UIColorTableEntryType *ctsP = UICState.colorTableStackP;
	
	// for each stack element currently in use
	for (i = 0; i < UICState.colorTableStackIndex + UILastColorTableEntry; i++) {
			ctsP[i].index = WinRGBToIndex(&(UIColorToRGBColor(ctsP[i])));
	}
	
	UICState.tablesDirty = false;
}


/***********************************************************************
 *
 * FUNCTION:    PrvDepthChangeNotification
 *
 * DESCRIPTION: Callback registered with NotifyMgr to respond to
 *				 	 screen depth/palette changes.  Loads the proper color
 *					 table for the new depth, and refreshes the indexes.
 *
 * PARAMETERS:  notifyParamsP -> data about the depth change.
 *
 * RETURNED:    Error -- currently never is one.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	2/3/99	Initial Revision
 *
 ***********************************************************************/
static Err PrvDepthChangeNotification(SysNotifyParamType* notifyParamsP)
{
	SysNotifyDisplayChangeDetailsType *detailsP = notifyParamsP->notifyDetailsP;
	UICState.tablesDirty = true;

	// if the depth changed, reload the color table for that depth
	if (detailsP->oldDepth != detailsP->newDepth)
		PrvLoadUIColorTable(detailsP->newDepth);	// also updates indexes
	else
		PrvUpdateUIColorTableIndexValues();
	
	// Set draw state colors to soemthing legitimate
	WinSetForeColor(UIColorGetIndex(UIObjectFrame));
	WinSetTextColor(UIColorGetIndex(UIObjectForeground));
	WinSetBackColor(UIColorGetIndex(UIFormFill));

	return 0;
}


/***********************************************************************
 *
 * FUNCTION:    PrvLoadUIColorTable
 *
 * DESCRIPTION: Load the UI color table from user prefs, or from system
 *					 resource if no user prefs.  Use the appropriate depth.
 *					 The index values for each entry will be correct for the
 *					 current draw window when this routine exits.
 *
 * PARAMETERS:  depth -> the depth to load for
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	08/31/99	Initial Revision
 *
 ***********************************************************************/
static void PrvLoadUIColorTable (UInt32 depth)
{
	Int16					count;
	DmOpenRef			dbP;
	MemHandle			uiColorsH = NULL;
	ColorTableType *	uiColorsP;
	
	// --------------------------------------------------------------
	// For direct modes, use the 8-bit UI palette. 
	// 
	// Similar logic is used in WinPalette, and in ScrScreenInit (SceenMgr.c), 
	// when loading the system palette.  Alternatively, the 8-bit palette 
	// could be duplicated for 16 bit mode, but I chose to add this hack 
	// to save ROM space.
	// --------------------------------------------------------------
	if (depth > 8)
		depth = 8;

	// get default colors from prefs DB, or use system resource if none
	// ...or if no system resource, use whatever was there before
	dbP = PrefOpenPreferenceDB (true);
	if (dbP)
		uiColorsH = (MemHandle) DmGet1Resource (sysFileCSystem, sysResIDPrefUIColorTableBase + depth);
	
	// ignore prefs colors if there are the wrong number (in case we add some in the future)
	if (uiColorsH) {
		uiColorsP = MemHandleLock(uiColorsH);
		if (uiColorsP->numEntries != UILastColorTableEntry) {
			MemPtrUnlock(uiColorsP);
			DmReleaseResource(uiColorsH);
			uiColorsH = NULL;
		}
	}
	
	// use system resource if no user prefs
	if (!uiColorsH) {
		uiColorsH = DmGetResource(colorTableRsc, systemDefaultUIColorsBase + depth);
		
		// debug builds -- use 'debugging' colortable on 8-bpp displays
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
		if (depth == 8) {
			DmReleaseResource(uiColorsH);
			uiColorsH = DmGetResource(colorTableRsc, systemDefaultUIColorsBase + depth + 1);
		}
#endif

		uiColorsP = MemHandleLock(uiColorsH);
	}

	if (uiColorsP) {
		ErrNonFatalDisplayIf(uiColorsP->numEntries != UILastColorTableEntry, "Wrong number of UI colors in system resource");

		for (count = 0; count < uiColorsP->numEntries; count++)
			UIColorSetTableEntry((UIColorTableEntries)count, &ColorTableEntries(uiColorsP)[count]);
			
		MemPtrUnlock(uiColorsP);
		DmReleaseResource(uiColorsH);
	}
	
	if (dbP)
		DmCloseDatabase(dbP);
}
