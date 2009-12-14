/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: UIGlobals.h
 *
 * Release: 
 *
 * Description:
 * 	This file defines global variables used by the UI routines.
 *
 * History:
 *		September 12, 1994	Created by Art Lamb
 *		5/ 5/98	art	Added a global variable for the virtual fonts. 
 *		9/12/99	gap	Added multi-tap globals. 
 *
 *****************************************************************************/

#ifndef __UIGLOBALS_H__
#define __UIGLOBALS_H__

// #include <SystemPublic.h>
#include "Globals.h"						// private include from System component

#include <Clipboard.h>
#include <Form.h>
#include <Menu.h>
#include <UIColor.h>
#include "UIColorPrv.h"

#define 	UIDefNumSysFonts  	8		// PalmOS 3.0 has eight system fonts
#define	UIDefNumAppFonts		4		// Space is initially allocated for four app fonts

#define 	eventQueueSize	10

typedef struct UIOptionsType {
	Boolean drawBordersAsLines:1;		// true means to draw lines for borders
	unsigned int reserved:7;
	} UIOptionsType;

typedef struct UIGlobalsType {

	// Global variables used by the window routines.
	WinHandle			activeWindow;
	WinHandle			displayWindow;
	WinHandle			drawWindow;
	WinHandle			firstWindow;
	WinHandle			exitWindowID;
	WinHandle			enterWindowID;
	WinHandle			exitedWindowID;
	GraphicStateType	gState;
	UIColorStateType	uicState;				// uses space that used to be in gState
	UInt32				lastTapTime;			// additional extra space that used to be in gState
														// now contains tick of last pen down for use in calculating
														// multi-tap state

	
	// Global variables used by the event routines.
	SysEventStoreType*	eventQ;
	UInt16				eventQIndex;
	UInt16				eventQLength;
	Int16      			lastScreenX;
	Int16      			lastScreenY;
	Boolean    			lastPenDown;
	UInt8 				tapCount;
	Int32					needNullTickCount;		// We want a null Evt when ticks reaches this value
	
	
	// Global variables used by the font routins.
	FontPtr 				uiCurrentFontPtr;
	FontTablePtr		uiSysFontTablePtr;		// new for 3.0
	FontTablePtr		uiAppFontTablePtr;		// new for 3.0
	Int16					uiNumSysFonts;				// new for 3.0
	Int16					uiNumAppFonts;				// new for 3.0
	FontTablePtr		uiFontListPtr;				// new for 3.1
	WinHandle			rootWindow;					// new for 3.2 to fixup after third party apps
	FontID  				uiCurrentFontID;
	Boolean 				allFormsClosing;			// Indicates that all forms are closing.
															// Used to enable a redraw optimization.
															// This could be made a bitfield for more space.
	
	
	// Global variables used by the form routines.
	FormType * 			currentForm;
	
	
	// Global variables used by the insertion point routines.
	Boolean				insPtIsEnabled;
	Boolean				insPtOn;
	PointType			insPtLoc;
	Int16					insPtHeight;
	Int32					insPtLastTick;
	WinHandle			insPtBitsBehind;
	
	// Global variables used by the clipboard routines.
	ClipboardItem 		clipboard[numClipboardFormats];
	
	// Globals used by the Memory Manager
	MenuBarPtr			uiCurrentMenu;
	UInt16				uiCurrentMenuRscID;
	
	// Global variables used by the field routines.
	FieldUndoType 		undoGlobals;

	// Global variables used by the Griffiti shift state indicator routines.
	UInt8					gsiState;
	Boolean				gsiIsEnabled;
	PointType			gsiLocation;

	// Numeric decimal seperator, used in the field routines.
	Char					uiDecimalSeparator;
	UIOptionsType		uiOptions;

	MenuCmdBarType *		menuCmdBarCurrent;	// new for 3.5, for the menu command bar
	

	//-----------------------------------------------------------------------
	// Adding any more UIGlobals beyond this point would be a bad idea...
	// The UIGlobals structure sits inside the system globals.
	// If it grows any larger, system globals will not remain at the
	// same location as they have been in past system releases.
	//-----------------------------------------------------------------------

	
	} UIGlobalsType;
typedef UIGlobalsType* 	UIGlobalsPtr;


#define	ActiveWindow			((UIGlobalsPtr)GUIGlobalsP)->activeWindow
#define	DisplayWindow			((UIGlobalsPtr)GUIGlobalsP)->displayWindow
#define	DrawWindow				((UIGlobalsPtr)GUIGlobalsP)->drawWindow
#define	RootWindow				((UIGlobalsPtr)GUIGlobalsP)->rootWindow
#define	FirstWindow				((UIGlobalsPtr)GUIGlobalsP)->firstWindow
#define	ExitWindowID			((UIGlobalsPtr)GUIGlobalsP)->exitWindowID
#define	EnterWindowID			((UIGlobalsPtr)GUIGlobalsP)->enterWindowID
#define	ExitedWindowID			((UIGlobalsPtr)GUIGlobalsP)->exitedWindowID
#define	GState					((UIGlobalsPtr)GUIGlobalsP)->gState
#define	UICState					((UIGlobalsPtr)GUIGlobalsP)->uicState
#define	LastTapTime				((UIGlobalsPtr)GUIGlobalsP)->lastTapTime

#define	EventQ					((UIGlobalsPtr)GUIGlobalsP)->eventQ
#define	EventQIndex				((UIGlobalsPtr)GUIGlobalsP)->eventQIndex
#define	EventQLength			((UIGlobalsPtr)GUIGlobalsP)->eventQLength
#define	LastScreenX				((UIGlobalsPtr)GUIGlobalsP)->lastScreenX
#define	LastScreenY				((UIGlobalsPtr)GUIGlobalsP)->lastScreenY
#define	LastPenDown				((UIGlobalsPtr)GUIGlobalsP)->lastPenDown
#define	TapCount					((UIGlobalsPtr)GUIGlobalsP)->tapCount
#define	NeedNullTickCount		((UIGlobalsPtr)GUIGlobalsP)->needNullTickCount

// UIFontTable no longer exists; use UISysFontPtr and UIAppFontPtr
#define	UICurrentFontPtr		((UIGlobalsPtr)GUIGlobalsP)->uiCurrentFontPtr
#define	UISysFontTablePtr		((UIGlobalsPtr)GUIGlobalsP)->uiSysFontTablePtr
#define	UIAppFontTablePtr		((UIGlobalsPtr)GUIGlobalsP)->uiAppFontTablePtr
#define	UISysFontPtr			(((UIGlobalsPtr)GUIGlobalsP)->uiSysFontTablePtr)
#define	UIAppFontPtr			(((UIGlobalsPtr)GUIGlobalsP)->uiAppFontTablePtr)
#define	UIFontListPtr			(((UIGlobalsPtr)GUIGlobalsP)->uiFontListPtr)
#define	UINumSysFonts			((UIGlobalsPtr)GUIGlobalsP)->uiNumSysFonts
#define	UINumAppFonts			((UIGlobalsPtr)GUIGlobalsP)->uiNumAppFonts
#define	UICurrentFontID		((UIGlobalsPtr)GUIGlobalsP)->uiCurrentFontID

#define	CurrentForm				((UIGlobalsPtr)GUIGlobalsP)->currentForm

#define	InsPtIsEnabled			((UIGlobalsPtr)GUIGlobalsP)->insPtIsEnabled
#define	InsPtOn					((UIGlobalsPtr)GUIGlobalsP)->insPtOn
#define	InsPtLoc					((UIGlobalsPtr)GUIGlobalsP)->insPtLoc
#define	InsPtHeight				((UIGlobalsPtr)GUIGlobalsP)->insPtHeight
#define	InsPtLastTick			((UIGlobalsPtr)GUIGlobalsP)->insPtLastTick
#define	InsPtBitsBehind		((UIGlobalsPtr)GUIGlobalsP)->insPtBitsBehind

#define	Clipboard				((UIGlobalsPtr)GUIGlobalsP)->clipboard

#define	UICurrentMenu			((UIGlobalsPtr)GUIGlobalsP)->uiCurrentMenu
#define	UICurrentMenuRscID	((UIGlobalsPtr)GUIGlobalsP)->uiCurrentMenuRscID
#define	UndoGlobals				((UIGlobalsPtr)GUIGlobalsP)->undoGlobals

#define	GsiState					((UIGlobalsPtr)GUIGlobalsP)->gsiState
#define	GsiIsEnabled			((UIGlobalsPtr)GUIGlobalsP)->gsiIsEnabled
#define	GsiLocation				((UIGlobalsPtr)GUIGlobalsP)->gsiLocation

#define	UIDecimalSeparator	((UIGlobalsPtr)GUIGlobalsP)->uiDecimalSeparator

#define	MenuCmdBarCurrent		((UIGlobalsPtr)GUIGlobalsP)->menuCmdBarCurrent

#define	AllFormsClosing		((UIGlobalsPtr)GUIGlobalsP)->allFormsClosing
#define	UIOptions				((UIGlobalsPtr)GUIGlobalsP)->uiOptions

#endif // __UIGLOBALS_H__
