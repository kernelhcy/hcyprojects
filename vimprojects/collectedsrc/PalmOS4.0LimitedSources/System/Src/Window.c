/******************************************************************************
 *
 * Copyright (c) 1994-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: Window.c
 *
 * Release: 
 *
 * Description:
 *             	This file contains the window management routines. 
 *
 *						Functions that interface with the blitter should be 
 *						located in WindowGraphics.c
 *
 * History:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	1/99		Created from WindowNew.c for color support
 *
 *****************************************************************************/

#define NON_PORTABLE

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_BITMAPS
#define ALLOW_ACCESS_TO_INTERNALS_OF_CLIPBOARDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_FIELDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS

#include <PalmTypes.h>

#include <DebugMgr.h>
#include <ErrorMgr.h>
#include <FeatureMgr.h>
#include <NotifyMgr.h>
#include <UIResources.h>		// for colorTableRsc

#include <PalmUtils.h>

#include "Blitter.h"
#include "Globals.h"
#include "HwrDisplay.h"
#include "ScreenMgr.h"
#include "SystemResourcesPrv.h"
#include "UIGlobals.h"
#include "WindowPrv.h"

#include "SysTrapsFastPrv.h"


// **************************************************************************
// Private constants
// **************************************************************************

// On debug ROMs, we use a "bad" value for DrawWindow in order
// to catch (bus error) apps that are trying to draw before they
// have set up the window environment correctly.
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
#define badDrawWindowValue 0x80000000
#endif



/***********************************************************************
 *
 * FUNCTION:    WinValidateHandle
 *
 * DESCRIPTION: Validate a window handle.  
 *
 * PARAMETERS:  winH	->	MemHandle of window to validate
 *
 * RETURNED:    true if the MemHandle is ok
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/19/97	Initial Revision
 *			roger	2/6/98	Add check to look for the window in the window list.
 *
 ***********************************************************************/
Boolean WinValidateHandle(WinHandle winH)
{
	WindowType* winP;
	
	winP = WinGetWindowPointer(winH);
	
	if (winP == NULL) 
		{
		ErrNonFatalDisplay("NULL window");
		return false;
		}
	
	if (winP->windowFlags.reserved)
		goto badWindow;
		
	if (winP->bitmapP) 
		{
		Int16 dispMaxDepth;
		
		HwrDisplayAttributes(false, hwrDispMaxDepth, &dispMaxDepth);
		if (winP->bitmapP->pixelSize > dispMaxDepth)
			goto badWindow;
		}
	
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	// Since these assignments were added we might as well get some use
	// from them.  Assert them, but only on debug ROMs.  (Bug 40437)
	if (winP->displayWidthV20 != 0)
		goto badWindow;
	
	if (winP->displayHeightV20 != 0)
		goto badWindow;
	
	if (winP->displayAddrV20 != NULL)
		goto badWindow;
#endif // ERROR_CHECK_LEVEL == ERROR_CHECK_FULL

	// if it's the draw window, validate that drawStateP is 'current'
	if (winH == DrawWindow && winP->drawStateP != GState.drawStateP)
		goto badWindow;

	return true;
	
badWindow:
	ErrNonFatalDisplay("Bad window");
	return false;
}


#pragma mark === Initialize and Allocate ===
/***********************************************************************
 *
 * FUNCTION:	WinScreenInit
 *
 * DESCRIPTION: This routine initializes global variables used by the 
 *              window routines.
 *
 * RETURNED:	Nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			RM		3/3/95	Initial Revision
 *
 ***********************************************************************/
void WinScreenInit()
{
	BitmapType* screenBitmap;
	WindowType* winP;
	WinHandle winH;
	
	screenBitmap = ScrScreenInit();
	
	// Create and initialize the display window.  Also, set the draw window
	// to the display window and add the display window to the window list.
	// We actually allocate just one pointer from the Memory Manager and store
	// the extended window information after the original window structure
	// within the same memory chunk in order to reduce the memory overhead. 
	winH = MemPtrNew(sizeof(WindowType));
	RootWindow = winH;
	DisplayWindow = winH;
	DrawWindow = winH;
	FirstWindow = winH;

	// Initialize to 0's
	winP = WinGetWindowPointer(winH);
	MemSet(winP, sizeof(WindowType), 0);
	
	// Set pointer to bitmap (for screen)
	winP->bitmapP = screenBitmap;

	// Set the bounds of the display window to the width and height of the physical display.
	winP->windowBounds.extent.x = winP->bitmapP->width;
	winP->windowBounds.extent.y = winP->bitmapP->height;
	
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	// break these three fields on purpose to force devs to update their apps
	winP->displayWidthV20  = 0;
	winP->displayHeightV20 = 0;
	winP->displayAddrV20   = NULL;
#else
	// copy the width, height and baseAddr fields for compatibility
	winP->displayWidthV20  = winP->bitmapP->width;
	winP->displayHeightV20 = winP->bitmapP->height;
	winP->displayAddrV20 = BmpGetBits(winP->bitmapP);	
#endif
	
	// allocate drawing state stack & initialize GState
	GState.drawStateStackP = MemPtrNew(DrawStateStackSize * sizeof(DrawStateType));
	MemSet(GState.drawStateStackP, DrawStateStackSize * sizeof(DrawStateType), 0);
	GState.drawStateIndex = 0;
	GState.drawStateP = &GState.drawStateStackP[0];

	// Store a pointer to graphic state in the window structure.
	winP->drawStateP = GState.drawStateP;

	// Start with no active window.
	ActiveWindow = 0;

	// Clear global variables used to track winEnter and winExit events.
	ExitWindowID = 0;
	EnterWindowID = 0;
	
	// ---------------------------------------------------------------
  	//  Set the palette from system default resource and build the color 
  	//  translation tables that map pixels from other depths with
  	//  standard system cluts into the current depth and CLUT. 
  	//  ScrPalette() will allocate the color translation tables for us 
  	//  the first time it is called. 
  	// ---------------------------------------------------------------
  	WinPalette(winPaletteInit, 0 /*startIndex*/, 0 /*numEntries*/, 0 /*tableP*/);
}


/***********************************************************************
 *
 * FUNCTION:    WinCreateWindow
 *
 * DESCRIPTION: This routine creates a new window and adds it to the 
 *              window list.  Windows created by this routine will 
 *              draw to the display.  See WinCreateOffscreenWindow
 *              for windows that draw to a memory buffer.
 *
 *              New windows are created disabled.  They must be enabled 
 *              before they will accept input.
 *
 * PARAMETERS:  bounds    display relative bounds of the window
 *              frame     type of frame around the window
 *              modal     TRUE if the window is modal
 *              focusable TRUE if the window can be the active window
 *              error     pointer to any error encountered by this function
 *              
 * RETURNED:    MemHandle of the new window.
 *
 * REVISION HISTORY:
 *			Name	Date			Description
 *			----	----			-----------
 *			art	12/15/94		Initial Revision
 *
 ***********************************************************************/
WinHandle WinCreateWindow(const RectangleType* boundsP, FrameType frame, 
										Boolean modal, Boolean focusable, UInt16* error)
{
	WinHandle newWinHandle;
	WindowType *newWinP;

	*error = errNone;

	// Create and initialize the window.  
	// We actually allocate just one pointer from the Memory Manager and
	// store the extended window information after the original window
	//  structure within the same memory chunk in order to reduce the
	// memory overhead. 
	newWinHandle = MemPtrNew(sizeof(WindowType));
	if (!newWinHandle) 
		{
		*error = sysErrNoFreeResource;
		return 0;
		}
	
	newWinP = WinGetWindowPointer(newWinHandle);
	MemSet(newWinP, sizeof(WindowType), 0);

	RctCopyRectangle(boundsP, &newWinP->windowBounds);

	newWinP->windowFlags.modal = modal;
	newWinP->windowFlags.focusable = focusable;
	newWinP->frameType.word = frame;

	// Initialize the device dependent information in the WindowType structure
   // and reset the clipping bounds.
	//
	WinInitializeWindow(newWinHandle);

	// Add the window to the window list.
	//
	WinAddWindow(newWinHandle);

	return newWinHandle;
}


/***********************************************************************
 *
 * FUNCTION:    WinDeleteWindow
 *
 * DESCRIPTION: This routine removes a window from the window list and
 *              frees the memory used by the window.
 *
 * PARAMETERS:  winH  		MemHandle of window to delete
 *              eraseIt		if TRUE the window is erased before it is deleted
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/15/94	Initial Revision
 *
 ***********************************************************************/
void WinDeleteWindow(WinHandle winH, Boolean eraseIt)
{
	WinHandle  savedDrawWinH;
	WindowType* winP;
	RectangleType frameRect;

	// The display window may not be deleted.
	ErrFatalDisplayIf((winH == DisplayWindow), 
		"Can't delete display window");
	
	ECWinValidateHandle(winH);
	
	// If the window being deleted is the draw window, make the display
	// window the draw window.
	if (winH == DrawWindow)
		WinSetDrawWindow(DisplayWindow);

	// If the window being deleted is the active window, set the global
	// variable that will cause a winExitEvent to be generated.
	if (winH == ActiveWindow)
		{
		ExitWindowID = winH;
		ActiveWindow = 0;
		}

	// If the eraseIt parameter is true and the window is on screen, erase
	// the window and its frame.
	winP = WinGetWindowPointer(winH);
	if ((! winP->windowFlags.offscreen) && eraseIt)
		{
		WinGetWindowFrameRect (winH, &frameRect);
		savedDrawWinH = WinSetDrawWindow(DisplayWindow);
		WinEraseRectangle (&frameRect, 0);
		WinSetDrawWindow(savedDrawWinH);
		}

	// Remove the window from the window list.
	WinRemoveWindow(winH);

	// free the bitmapP if allocated dynamically by WinCreateOffscreenWindow
	if (winP->windowFlags.freeBitmap)
		BmpDelete(winP->bitmapP);
	
	MemPtrFree (winH);
}


/***********************************************************************
 *
 * FUNCTION:    WinCreateOffscreenWindow
 *
 * DESCRIPTION: This routine creates a new window and adds it to the 
 *              window list.  Windows created with this routine will
 *              draw to a memory buffer instead of the display.
 *
 *              The memory buffer has two formats: screen format and 
 *              generic format.  Srceen format is the native format of the
 *              video system, window in this format can be copied to the
 *              display faster.  The generic format is device independent.
 *
 * PARAMETERS:  width    width of the window in pixels
 *              height   height of the window in pixels
 *              format   screen (native) or generic
 *              error    pointer to any error encountered by this function
 *
 * RETURNED:    MemHandle of the new window.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/15/94	Initial Revision
 *			jmp	10/24/99	On debug ROMs, use DisplayWindow if DrawWindow
 *								is the badDrawWindowValue, but tell developers
 *								they need to rev.
 *			gcd	11/04/99	Change this function to not create a color table
 *								for the bitmap that is created to support the
 *								offscreen window.  Since this function will
 *								typically be used for a double buffer type of
 *								use, having a color table just slows things down.
 *								If you do need a color table, call BmpCreate to
 *								create a BitMap with a color table and then call
 *								WinCreateBitmapWindow.
 *			bob	11/18/99	always use DisplayWindow -- this is screen specific.
 *
 ***********************************************************************/
WinHandle WinCreateOffscreenWindow(Coord width, Coord height, WindowFormatType format, UInt16* error)
{
	WinHandle	winH;
	WindowType	*newWinP;
	WindowType	*displayWinP;
	BitmapType	*newBitmapP;

	if (!error)
		{
		ErrNonFatalDisplay("NULL error argument");
		return 0;
		}
		
	*error = errNone;

	displayWinP = WinGetWindowPointer(DisplayWindow);

	newBitmapP = BmpCreate(width, height, displayWinP->bitmapP->pixelSize, NULL, error);

	if (*error) 
		return 0;

	newBitmapP->flags.hasTransparency = displayWinP->bitmapP->flags.hasTransparency;
	newBitmapP->transparentIndex = displayWinP->bitmapP->transparentIndex;

	winH = WinCreateBitmapWindow(newBitmapP, error);
	if (*error) 
		return 0;

	newWinP = WinGetWindowPointer (winH);
	newWinP->windowFlags.format = format;
	newWinP->windowFlags.freeBitmap = true;	// so WinDeleteWindow frees bitmap

	return (winH);
}


/***********************************************************************
 *
 * FUNCTION:    WinCreateBitmapWindow
 *
 * DESCRIPTION: 	This routine creates a new window and adds it to the 
 *              	window list.  Windows created with this routine will
 *              	draw to a provided bitmap instead of the display.
 *						The bitmap will NOT be freed by WinDeleteWindow.
 *
 * PARAMETERS:  bitmapP  The initialized bitmap to associate with the window
 *              error    pointer to any error encountered by this function
 *
 * RETURNED:    MemHandle of the new window.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			BS		9/99		Initial Revision
 *			jmp	10/24/99	On debug ROMs, use DisplayWindow if DrawWindow
 *								is the badDrawWindowValue but tell developers
 *								they need to rev.
 *			BS		10/27/99	A window needs a colortable to be drawn onto
 *			bob	12/01/99	Remove dependency on DrawWindow, use GState instead
 *
 ***********************************************************************/
WinHandle WinCreateBitmapWindow(BitmapType* bitmapP, UInt16* error)
{
	MemHandle	winH;
	WinHandle	savedDrawWinH;
	WinHandle	newWinHandle;
	WindowType* newWinP;

	// Check the bitmap	
	if (!bitmapP || !bitmapP->width || !bitmapP->height || bitmapP->flags.indirect || 
		bitmapP->flags.compressed || bitmapP->flags.forScreen ||  MemPtrDataStorage(bitmapP)) 
		{
		*error = sysErrParamErr;
		return NULL;
		}
	
	// Depth
	if ((bitmapP->pixelSize!= 1) && (bitmapP->pixelSize!=2) && (bitmapP->pixelSize!=4) && 
		(bitmapP->pixelSize!=8) && (bitmapP->pixelSize != 16)) 
		{
		*error = sysErrParamErr;
		return NULL;
		}

	*error = errNone;

	// Allocate the window.
	winH = MemHandleNew(sizeof(WindowType));
	if (! winH) 
		{
		*error = sysErrNoFreeResource;
		return (0);		
		}
	
	// Init the Window
	newWinHandle = MemHandleLock(winH);
	newWinP = WinGetWindowPointer(newWinHandle);
	MemSet(newWinP,sizeof(WindowType), 0);

#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	// break these three fields on purpose to force devs to update their apps
	newWinP->displayWidthV20  = 0;
	newWinP->displayHeightV20 = 0;
	newWinP->displayAddrV20   = NULL;
#else
	// copy these three fields for compatibility
	newWinP->displayWidthV20  = bitmapP->width;
	newWinP->displayHeightV20 = bitmapP->height;
	newWinP->displayAddrV20   = BmpGetBits(bitmapP);
#endif

	newWinP->windowFlags.format = genericFormat; // Abstraction not yet used
	newWinP->windowFlags.offscreen = true;
	//newWinP->windowFlags.freeBitmap = false; // Do not delete the bitmap when delete the window

	newWinP->windowBounds.extent.x = bitmapP->width;
	newWinP->windowBounds.extent.y = bitmapP->height;

	newWinP->bitmapP = bitmapP;
	newWinP->drawStateP = GState.drawStateP;

	// Add the window to the window list
	//
	WinAddWindow(newWinHandle);

	// Reset the clipping bounds.
	//
	savedDrawWinH = WinSetDrawWindow(newWinHandle);
	WinResetClip();
	WinSetDrawWindow(savedDrawWinH);

	return newWinHandle;
}


/***********************************************************************
 *
 * FUNCTION:    WinInitializeWindow
 *
 * DESCRIPTION: This routine initializes the screen dependent members
 *              of a WindowType structure and sets the clipping bound
 *              of the window to the bounds of the window.
 *
 * PARAMETERS:  winH - MemHandle of a window
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/16/94	Initial Revision
 *
 ***********************************************************************/
void WinInitializeWindow (WinHandle winH)
{
	WinHandle	savedDrawWinH;
	WindowType*	displayWinP;
	WindowType* winP;
	
	displayWinP = WinGetWindowPointer(DisplayWindow);
	winP = WinGetWindowPointer(winH);

	// We set the bitmapP pointer to be the same as the display window's
	// bitmapP pointer which is the screen driver's bitmap. Since the
	// screen driver's bitmap has the forDriver flag set, it won't get
	// disposed of by WinDeleteWindow.

	ErrNonFatalDisplayIf(winP->windowFlags.offscreen, "WinInitializeWindow is internal only");
	
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	// break these three fields on purpose to force devs to update their apps
	winP->displayWidthV20  = 0;
	winP->displayHeightV20 = 0;
	winP->displayAddrV20   = NULL;
#else
	// copy these three fields for compatibility
	winP->displayWidthV20  = displayWinP->displayWidthV20;
	winP->displayHeightV20 = displayWinP->displayHeightV20;
	winP->displayAddrV20   = displayWinP->displayAddrV20;
#endif

	winP->bitmapP = displayWinP->bitmapP;
	
	winP->drawStateP = displayWinP->drawStateP;
	
	savedDrawWinH = WinSetDrawWindow(winH);
	WinResetClip();
	WinSetDrawWindow(savedDrawWinH);
}


/***********************************************************************
 *
 * FUNCTION:    	WinPrvInitCanvasTrap
 *
 * DESCRIPTION: 	Trap call used by the deprecated Scrxxx blitter 
 *						functions.
 *
 * PARAMETERS:		pointer to WindowType, source of initialization
 *						pointer CanvasType to be initialized
 *
 * REVISION HISTORY:
 *			Name	Date		 Description
 *			----	----		 -----------
 *			acs	06/14/00	 Initial Revision
 *
 ***********************************************************************/
void WinPrvInitCanvasTrap(WindowType* winP, CanvasType* canvasP)
{
	WinPrvInitCanvas(winP, canvasP);
}


/***********************************************************************
 *
 * FUNCTION:    	WinPrvInitCanvas
 *
 * DESCRIPTION: 	Private call used by WindowGraphics.c functions.
 *
 * PARAMETERS:		pointer to WindowType, source of initialization
 *						pointer CanvasType to be initialized
 *
 * REVISION HISTORY:
 *			Name	Date		 Description
 *			----	----		 -----------
 *			acs	06/14/00	 Initial Revision
 *
 ***********************************************************************/
void WinPrvInitCanvas(WindowType* winP, CanvasType* canvasP)
{
	Coord left, top;
	
	canvasP->viewOrigin = winP->windowBounds.topLeft;	
	canvasP->drawStateP = winP->drawStateP;
	canvasP->bitmapP = winP->bitmapP;
	
	left = winP->clippingBounds.left;
	top = winP->clippingBounds.top;
	canvasP->clippingRect.topLeft.x = left;
	canvasP->clippingRect.topLeft.y = top;
	canvasP->clippingRect.extent.x = winP->clippingBounds.right - left + 1;	
	canvasP->clippingRect.extent.y = winP->clippingBounds.bottom - top + 1;	
}


/***********************************************************************
 *
 * FUNCTION:    	WinPrvInitCanvasWithBitmap
 *
 * DESCRIPTION: 	Private call used by BmpCompress().
 *
 * PARAMETERS:		pointers to BitmapType and DrawStateType, source of initialization
 *						pointer CanvasType to be initialized
 *
 * REVISION HISTORY:
 *			Name	Date		 Description
 *			----	----		 -----------
 *			acs	06/14/00	 Initial Revision
 *
 ***********************************************************************/
void WinPrvInitCanvasWithBitmap(BitmapType* bitmapP, DrawStateType* drawStateP, CanvasType* canvasP)
{
	canvasP->viewOrigin.x = 0;
	canvasP->viewOrigin.y = 0;
	canvasP->drawStateP = drawStateP;
	canvasP->bitmapP = bitmapP;

	canvasP->clippingRect.topLeft.x = 0;
	canvasP->clippingRect.topLeft.y = 0;
	canvasP->clippingRect.extent.x = bitmapP->width;	
	canvasP->clippingRect.extent.y = bitmapP->height;
}


#pragma mark === Color Management ===
/***********************************************************************
 *
 * FUNCTION:		WinPalette
 *
 * DESCRIPTION: 	Set or get the palette for the draw window.
 *
 * RETURNED:		Err
 *
 ***********************************************************************/
Err WinPalette(UInt8 operation, Int16 startIndex, UInt16 numEntries, RGBColorType* tableP)
{
	Int16           	i, index;
	MemHandle			paletteH;
	RGBColorType*   	rgbEntryP;
	ColorTableType* 	paletteP;
	ColorTableType* 	dstTableP;
	UInt16          	maxEntry;
	SysNotifyParamType notifyParams;
	Err             	err = errNone;
	Boolean 				sendNotify = true;
	BitmapType*     	drawBitmapP = WinGetWindowPointer(DrawWindow)->bitmapP;

  	// Get the draw window's color table
	if (drawBitmapP->flags.hasColorTable == false) 
		{
	  	ErrNonFatalDisplay("Draw window has no palette");
	  	return winErrPalette;
		}

  	if (startIndex < -1) 
		{
	  	ErrNonFatalDisplay("WinPalette: invalid startIndex");
	  	return sysErrParamErr;
		}

	if (operation == winPaletteInit)
		sendNotify = false;				
	
	switch(operation) 
		{
		// ===================================================================
		//	Set To Defaults
		// ===================================================================
		case winPaletteInit:
		case winPaletteSetToDefault:
			{
			UInt16	clutIndex;
		
			// --------------------------------------------------------------
			// Set the colortable to the default system palette. For direct modes,
			// use the 8-bit palette as a virtual CLUT.  
			// 
			// This logic is duplicated in ScrScreenInit (ScreenMgr.c), and similar logic 
			// is used in PrvLoadUIColorTable (UInit.c) when loading the UI palette.  
			// Alternatively, the 8-bit palette could be duplicated for 16 bit mode, but 
			// I chose to add this hack to save ROM space.
			// --------------------------------------------------------------
			clutIndex = drawBitmapP->pixelSize;
			if (clutIndex > 8)
				clutIndex = 8;
			
			paletteH = DmGetResource(colorTableRsc, systemPaletteBase + clutIndex);
			ErrNonFatalDisplayIf(!paletteH, "DmGetResource palette error");
			paletteP = (ColorTableType*) MemHandleLock(paletteH);
			
			numEntries = paletteP->numEntries;
			startIndex = 0;
			tableP = ColorTableEntries(paletteP);
			}
			// fall through
	
		// ===================================================================
		//	Set from passed in table
		// ===================================================================
		case winPaletteSet:
		
			// --------------------------------------------------------------
			// Set the colortable to the given palette
			// --------------------------------------------------------------
			dstTableP = BmpGetColortable(drawBitmapP);
			if (!dstTableP)
				return winErrPalette;
	
			rgbEntryP = ColorTableEntries(dstTableP);
			maxEntry = dstTableP->numEntries;
			
			// If not for the screen, copy entries manually into the Draw bitmap's color table. 
			if (!drawBitmapP->flags.forScreen) 
				{
				if (! tableP)
					{
	  				ErrNonFatalDisplay("WinPalette:  null palette");
	  				return sysErrParamErr;
					}
					
				for (i = 0; i < numEntries; i++) 
					{
					if (startIndex < 0)
						index = tableP[i].index;	// use index in table
					else
						index = i + startIndex;		// use linear values
	
					if (index >= maxEntry) 
						{
						ErrNonFatalDisplay("Palette overflow");
						return winErrPalette;
						}
	
					rgbEntryP[index].r = tableP[i].r;
					rgbEntryP[index].g = tableP[i].g;
					rgbEntryP[index].b = tableP[i].b;
					}
				}
	
			if (drawBitmapP->flags.forScreen) 
				{
				Int16 totalClutEntries = (drawBitmapP->pixelSize >= 8) ? 256 : 1 << drawBitmapP->pixelSize;

				if ((numEntries > totalClutEntries)	||					// numEntries > total entries in table
					(startIndex >= totalClutEntries) ||					// startIndex > total entries in table
					((startIndex + numEntries) > totalClutEntries))	// no wrapping
					{
					ErrNonFatalDisplay("WinPalette: invalid parameters");
					return sysErrParamErr;
					}
				
				err = ScrPalette(startIndex, numEntries, tableP, drawBitmapP);
				}
			
			if (operation == winPaletteSetToDefault) 
				{
				MemPtrUnlock(paletteP);
				DmReleaseResource(paletteH);
				}

			if (err != errNone)
				return err;			// trouble!

			// --------------------------------------------------------------
			// broadcast a notification to tell people palette has changed
			// --------------------------------------------------------------
			if (sendNotify) 
				{
				SysNotifyDisplayChangeDetailsType depthChangeDetails;
				
				err = HwrDisplayAttributes(false, hwrDispDepth, &depthChangeDetails.oldDepth);
				depthChangeDetails.newDepth = depthChangeDetails.oldDepth;
	
				notifyParams.notifyType = sysNotifyDisplayChangeEvent;
				notifyParams.broadcaster = sysFileCSystem;
				notifyParams.notifyDetailsP = &depthChangeDetails;
				notifyParams.handled = false;
				
				SysNotifyBroadcast(&notifyParams);
				}
	
		  break;					// winPaletteSet
	
		// ===================================================================
		//	Get current palette
		// ===================================================================
		case winPaletteGet:
		
			// getting values from the draw color table
			rgbEntryP = ColorTableEntries(BmpGetColortable(drawBitmapP));
			maxEntry = BmpGetColortable(drawBitmapP)->numEntries;
			for (i = 0; i < numEntries; i++) 
				{
				if (startIndex < 0)
					index = tableP[i].index;	// use index in table
				else
					index = i + startIndex;		// use linear values
	
				if (index >= maxEntry) 
					{
					ErrNonFatalDisplay("Palette overflow");
					return winErrPalette;
					}
	
				tableP[i].index = index;
				tableP[i].r = rgbEntryP[index].r;
				tableP[i].g = rgbEntryP[index].g;
				tableP[i].b = rgbEntryP[index].b;
				}
		}		// end switch

  	return err;
}


/***********************************************************************
 *
 * FUNCTION:    WinSetForeColor
 *
 * DESCRIPTION: Set the color to use for foreground in draw operations
 *
 * PARAMETERS:  foreColor	->	index value of color to use for foreground
 *
 * RETURNED:    index value of old foreground color
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob		1/22/99		Initial Revision
 *
 ***********************************************************************/
IndexedColorType WinSetForeColor(IndexedColorType foreColor)
{
	WindowType* winP = WinGetWindowPointer(DrawWindow);
	IndexedColorType result = winP->drawStateP->foreColor;

	if (foreColor >= (1L << winP->bitmapP->pixelSize))
		ErrNonFatalDisplay("Index out of range");
	else
		winP->drawStateP->foreColor = foreColor;

  	// Update the RGB value too
  	WinPrvIndexToRGB(foreColor, &winP->drawStateP->foreColorRGB);

	return result;
}


/***********************************************************************
 *
 * FUNCTION:    WinSetBackColor
 *
 * DESCRIPTION: Set the color to use for background in draw operations
 *
 * PARAMETERS:  backColor	->	index value of color to use for background
 *
 * RETURNED:    index value of old background color
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob		1/22/99		Initial Revision
 *
 ***********************************************************************/
IndexedColorType WinSetBackColor(IndexedColorType backColor)
{
	WindowType* winP = WinGetWindowPointer(DrawWindow);
	IndexedColorType result = winP->drawStateP->backColor;

	if (backColor >= (1L << winP->bitmapP->pixelSize))
		ErrNonFatalDisplay("Index out of range");
	else
		winP->drawStateP->backColor = backColor;
		
	// Update the RGB value too
  	WinPrvIndexToRGB(backColor, &winP->drawStateP->backColorRGB);
		
	return result;
}


/***********************************************************************
 *
 * FUNCTION:    WinSetTextColor
 *
 * DESCRIPTION: Set the color to use for drawing characters
 *
 * PARAMETERS:  textColor	->	index value of color to use for text
 *
 * RETURNED:    index value of old text color
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob		1/22/99		Initial Revision
 *
 ***********************************************************************/
IndexedColorType WinSetTextColor(IndexedColorType textColor)
{
	WindowType* winP = WinGetWindowPointer(DrawWindow);
	IndexedColorType result = winP->drawStateP->textColor;

	if (textColor >= (1L << winP->bitmapP->pixelSize))
		ErrNonFatalDisplay("Index out of range");
	else
		winP->drawStateP->textColor = textColor;
		
	// Update the RGB value too
	WinPrvIndexToRGB(textColor, &winP->drawStateP->textColorRGB);

	return result;
}


/***********************************************************************
 *
 * FUNCTION:    WinSetForeColorRGB
 *
 * DESCRIPTION: Set the color to use for foreground in draw operations
 *
 * PARAMETERS:  
 *				*newRgbP	IN		new color
 *				*prevRgbP	OUT		previous color
 *
 * RETURNED:    void
 *
 * REVISION HISTORY:
 *	2-Mar-2000	  RM	  Created
 *
 ***********************************************************************/
void WinSetForeColorRGB(const RGBColorType* newRgbP, RGBColorType* prevRgbP)
{
  	WindowType*    winP = WinGetWindowPointer(DrawWindow);

  	// Return the previous color
  	if (prevRgbP)
		*prevRgbP = winP->drawStateP->foreColorRGB;

  	// Set new RGB color
  	if (newRgbP) 
		{
	  	winP->drawStateP->foreColorRGB = *newRgbP;

	  	// Set the index too
	  	winP->drawStateP->foreColor = WinRGBToIndex(newRgbP);

	  	// Make sure the index value in the rgbP matches to indicate that the RGB value is valid
	  	winP->drawStateP->foreColorRGB.index = winP->drawStateP->foreColor;
		}
}


/***********************************************************************
 *
 * FUNCTION:    WinSetBackColorRGB
 *
 * DESCRIPTION: Set the color to use for background in draw operations
 *
 * PARAMETERS:  
 *				*newRgbP	IN		new color
 *				*prevRgbP	OUT		previous color
 *
 * RETURNED:    void
 *
 * REVISION HISTORY:
 *	2-Mar-2000	  RM	  Created
 *
 ***********************************************************************/
void WinSetBackColorRGB(const RGBColorType* newRgbP, RGBColorType* prevRgbP)
{
  	WindowType*     winP = WinGetWindowPointer(DrawWindow);

  	// Return the previous color
  	if (prevRgbP)
		*prevRgbP = winP->drawStateP->backColorRGB;

  	// Set new color
  	if (newRgbP) 
		{
	 	winP->drawStateP->backColorRGB = *newRgbP;

	  	// Set the index too
	  	winP->drawStateP->backColor = WinRGBToIndex(newRgbP);

	  	// Make sure the index value in the rgbP matches to indicate that the RGB value is valid
	  	winP->drawStateP->backColorRGB.index = winP->drawStateP->backColor;
		}
}


/***********************************************************************
 *
 * FUNCTION:    WinSetTextColorRGB
 *
 * DESCRIPTION: Set the color to use for Text in draw operations
 *
 * PARAMETERS:  
 *				*newRgbP	IN		new color
 *				*prevRgbP	OUT		previous color
 *
 * RETURNED:    void
 *
 * REVISION HISTORY:
 *	2-Mar-2000	  RM	  Created
 *
 ***********************************************************************/
void WinSetTextColorRGB(const RGBColorType* newRgbP, RGBColorType* prevRgbP)
{
  	WindowType*     winP = WinGetWindowPointer(DrawWindow);

  	// Return the previous color
  	if (prevRgbP)
		*prevRgbP = winP->drawStateP->textColorRGB;

  	// Set new color
  	if (newRgbP) 
		{
	  	winP->drawStateP->textColorRGB = *newRgbP;

	  	// Set the index too
	  	winP->drawStateP->textColor = WinRGBToIndex(newRgbP);

	  	// Make sure the index value in the rgbP matches to indicate that the RGB value is valid
	 	winP->drawStateP->textColorRGB.index = winP->drawStateP->textColor;
		}
}


/***********************************************************************
 *
 * FUNCTION:    	WinRGBToIndex
 *
 * DESCRIPTION: 	Converts the RGB Value to index of the closest color
 *					in the currently active Color Lookup Table.
 *
 * PARAMETERS:  	rgbP	->	RGB Color Value
 *
 * RETURNED:    	index	<-	The index of the closest matching color in the CLUT.
 *
 * REVISION HISTORY:
 *			Name		Date		Description
 *			----		----		-----------
 *			Ben		2/19/99	Modified function to call ScrDisplayFindIndexes which
 *									does all the work to find the closest index.
 *
 ***********************************************************************/
IndexedColorType WinRGBToIndex(const RGBColorType* rgbP)
{
	RGBColorType 	theColor = *rgbP;
	ColorTableType*	refTable = 0;
	Err	err;

	// Make sure that DrawWindow is valid before making any assumptions.
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	if (DrawWindow && (DrawWindow != (WinHandle) badDrawWindowValue))
#else
	if (DrawWindow)
#endif
		refTable = BmpGetColortable(DrawWindow->bitmapP);
	if (!refTable)
		refTable = ScrGetColortable();
		
	err = BltFindIndexes(1, &theColor, refTable);
	ErrNonFatalDisplayIf(err, "Error");
	
	return (theColor.index);
}


/***********************************************************************
 *
 * FUNCTION:    	WinIndexToRGB
 *
 * DESCRIPTION: 	Returns the RGB value for the specified index in the
 *					 current color table.
 *
 * PARAMETERS:  	i		->		Index to look up
 *					rgbP	<-		Pointer to RGBColorType to get result
 *
 * REVISION HISTORY:
 *			Name		Date		Description
 *			----		----		-----------
 *			bob			3/4/99		Initial version, use pointer param for result.
 *
 ***********************************************************************/
void WinIndexToRGB(IndexedColorType i, RGBColorType* rgbP)
{
	Err 			err;

	rgbP->index = i;
	err = WinPalette(winPaletteGet, WinUseTableIndexes, 1, rgbP);
	ErrNonFatalDisplayIf(err, "Error");
	rgbP->index = 0;	// normalize
	return;	
}
	

/***********************************************************************
 *
 * FUNCTION:    WinPrvIndexToRGB
 *
 * DESCRIPTION: Convert an index into RGB. This is a faster version
 *	  			than WinIndexToRGB() and is used by the WinSetXXXColor() 
 *				routines, and by WinGetPixelRGB().
 *
 * PARAMETERS:  
 *	  index		IN	  index to convert
 *	  *rgbP		OUT	  RGB
 *
 * RETURNED:    void
 *
 * REVISION HISTORY:
 *	2-Mar-2000	  RM	  Created
 *
 ***********************************************************************/
void WinPrvIndexToRGB(IndexedColorType index, RGBColorType* rgbP)
{
  	ColorTableType* cTableP = 0;
  	RGBColorType*   rgbEntryP;

  	// Get the appropriate color table
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	if (DrawWindow && (DrawWindow != (WinHandle) badDrawWindowValue))
#else
  	if (DrawWindow)
#endif
		cTableP = BmpGetColortable(DrawWindow->bitmapP);
  	if (!cTableP)
		cTableP = ScrGetColortable();

  	// Lookup this color
  	rgbEntryP = ColorTableEntries(cTableP);
  	*rgbP = rgbEntryP[index];
  	rgbP->index = index;
}


/***********************************************************************
 *
 * FUNCTION:    WinSetColors (deprecated)
 *
 * DESCRIPTION	This routine sets the new foreground and background
 *				colors and returns the old colors. 0 can be passed for
 *				parameters that should be ignored.
 *
 * NOTE:		This API is deprecated in favor of WinSet{Fore|Back|Text}Color
 *
 * PARAMETERS: 
 *		newForeColorP	->	new foreground color, don't change if NULL
 *		oldForeColorP	<-	old foreground color is returned if non-NULL
 *		newBackColorP	->	new background color, don't change if NULL
 *		oldBackColorP	<-	old background color is returned  if non-NULL
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			ron		1997		Initial Revision
 *			bob		1/20/99		Switched to use WinSet{Fore|Back|Text}Color
 *
 ***********************************************************************/
void WinSetColors(const RGBColorType* newForeColorP, RGBColorType* oldForeColorP,
					const RGBColorType* newBackColorP, RGBColorType* oldBackColorP)
{
  WinSetForeColorRGB(newForeColorP, oldForeColorP);
  WinSetTextColorRGB(newForeColorP, 0);
  WinSetBackColorRGB(newBackColorP, oldBackColorP);
}


#pragma mark === Window Management ===
/***********************************************************************
 *
 * FUNCTION:    WinEnableWindow
 *
 * DESCRIPTION: This routine enables a window.  Enabled window will
 *              accept pen input and can be made the active window
 *
 *              This routine does not affect the visual appearance of 
 *              the window.
 *
 * PARAMETERS:  winH    MemHandle of window to enable
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/15/94	Initial Revision
 *
 ***********************************************************************/
void WinEnableWindow(WinHandle winH)
{
	WindowType* winP;

	ECWinValidateHandle(winH);
	
	// The display window can not be the active window.
	if (winH != DisplayWindow)
		{
		winP = WinGetWindowPointer(winH);
		if (winP->windowFlags.format == screenFormat)
			winP->windowFlags.enabled = true;
		}
}


/***********************************************************************
 *
 * FUNCTION:    WinDisableWindow
 *
 * DESCRIPTION: This routine disables a window.  Disabled window will
 *              ignore all pen input and can not be made the active
 *              window.
 *
 *              This routine does not affect the visual appearance of 
 *              the window.
 *
 * PARAMETERS:  winH  -  MemHandle of window to disable
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date			Description
 *			----	----			-----------
 *			art	12/15/94		Initial Revision
 *
 ***********************************************************************/
void WinDisableWindow(WinHandle winH)
{
	WindowType* winP;

	ECWinValidateHandle(winH);
	
	winP = WinGetWindowPointer (winH);
	winP->windowFlags.enabled = false;
}


/***********************************************************************
 *
 * FUNCTION:    WinAddWindow
 *
 * DESCRIPTION: This routine adds the specified window to the 
 *              active windows list.
 *
 * PARAMETERS:  winH - MemHandle of the window 
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/15/94	Initial Revision
 *
 ***********************************************************************/
void WinAddWindow(WinHandle winH)
{
	WindowType* winP;
	
	winP = WinGetWindowPointer (winH);
	
	ErrNonFatalDisplayIf(FirstWindow == winP, "Window already added");
	winP->nextWindow = FirstWindow;
	FirstWindow = winH;

	ECWinValidateHandle(winH);
}


/***********************************************************************
 *
 * FUNCTION:    WinRemoveWindow
 *
 * DESCRIPTION: This routine removes the specified window from the 
 *              window list.
 *
 * PARAMETERS:  winH
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/16/94	Initial Revision
 *
 ***********************************************************************/
void WinRemoveWindow(WinHandle winH)
{
	WinHandle MemHandle;
	WindowType* winP;
	WindowType* ptr = 0;

	ECWinValidateHandle(winH);
	
	MemHandle = FirstWindow;
	winP = WinGetWindowPointer(winH);

	while (MemHandle)
		{
		if (MemHandle == winH)
			{
			if (winH == FirstWindow)
				FirstWindow = winP->nextWindow;
			else
				ptr->nextWindow = winP->nextWindow;
			break;
			}

		ptr = WinGetWindowPointer(MemHandle);
		MemHandle = ptr->nextWindow;
		}
}


/***********************************************************************
 *
 * FUNCTION:    WinMoveWindowAddr
 *
 * DESCRIPTION: Updates all OS references to a window from an old
 *					location to a new location.  (Used by forms when a form
 *					needs to grow, required because forms encapsulate a
 *					WindowType structure.)
 *
 *				DOLATER ??? -ÊShould be OS only.
 *
 * PARAMETERS:  	oldLocationP	->	from address
 *						newLocationP	->	to address
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/25/97	Initial Revision
 *
 ***********************************************************************/
void WinMoveWindowAddr (WindowType* oldLocationP, WindowType* newLocationP)
{
	WinHandle oldLocationH, newLocationH;
	WindowType* winP;

	newLocationH = WinGetWindowHandle(newLocationP);
	ECWinValidateHandle(newLocationH);
	oldLocationH = WinGetWindowHandle(oldLocationP);
	
	// Update the window in the window list
	winP = WinGetWindowPointer (FirstWindow);
	if (winP == oldLocationP)
		FirstWindow = newLocationH;
	else
		{
		// Search through for a pointer to the old location.
		while (winP->nextWindow)
			{
			if (winP->nextWindow == oldLocationP)
				{
				winP->nextWindow = newLocationP;
				break;
				}

			winP = winP->nextWindow;
			}
		}
	
	if (DrawWindow == oldLocationH)
		DrawWindow = newLocationH;
	
	if (DisplayWindow == oldLocationH)
		DisplayWindow = newLocationH;
	
	if (ActiveWindow == oldLocationH)
		ActiveWindow = newLocationH;
	
}


/***********************************************************************
 *
 * FUNCTION:    WinSaveBits
 *
 * DESCRIPTION: This routine creates an offscreen window and copies the
 *              specified region from the draw window to the offscreen
 *              window.  The offscreen window is the same size as the
 *              region to copy.
 *
 * PARAMETERS:  sourceP    ->	bounds of the region to save
 *              error     <-	pointer to any error encountered by this function
 *
 * RETURNED:    the MemHandle of the window containing the saved image, or
 *              NULL if an error occurred.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/15/94	Initial Revision
 *
 ***********************************************************************/
WinHandle WinSaveBits(const RectangleType* sourceP, UInt16* error)
{
	WinHandle	saveWinH;
	WindowType* saveWinP;
	BitmapType* bitmapP;
	Err			err;
	
	// Create the offscreen window.
	saveWinH = WinCreateOffscreenWindow(sourceP->extent.x, sourceP->extent.y, screenFormat, error);
	if (! saveWinH) 
		return(0);
		
	// Store bits in compressed format
	saveWinP = WinGetWindowPointer(saveWinH);
	bitmapP = WinGetBitmap(saveWinP);
	bitmapP->flags.compressed = true;
	bitmapP->compressionType = BitmapCompressionTypeScanLine;
		
	// initialize the size field of the compressed data to 0 so that we can detect the
	// case where the WinCopyRectangleRoutine returns without doing anything
	// (when totally clipped).  The size of a compressed Bitmap is stored in the first 
	// word of the bits.
	*((UInt16*) BmpGetBits(bitmapP)) = 0;
		
	// Save the bits
	WinCopyRectangle(0, saveWinH, sourceP, 0, 0, winPaint);
	
	// Shrink the pointer used to store the bits if the compression was successful.
	// The size of the compressed data is stored in the first word of the bits.
	// If compression failed, the bits were saved uncompressed, and resizing is not needed.
	if ((bitmapP->flags.compressed) && *((UInt16*) BmpGetBits(bitmapP)))
		{
		err = MemPtrResize(bitmapP, BmpSize(bitmapP));
		ErrFatalDisplayIf(err, "Resize error");
		}

	return saveWinH;
}


/***********************************************************************
 *
 * FUNCTION:    WinRestoreBits
 *
 * DESCRIPTION: This routine copies the contents of the specified window
 *              to the draw window, and deletes the passed window.  This
 *              routine is generally used to restore a region of the
 *              display that were saved with WinSaveBits.
 *
 * PARAMETERS:  winH  		MemHandle of window to copy and delete
 *              destX      x coordinate in the draw window to copy to
 *              destY      y coordinate in the draw window to copy to
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/15/94	Initial Revision
 *
 ***********************************************************************/
void WinRestoreBits(WinHandle winH, Coord destX, Coord destY)
{
	WindowType* winP;
	
	if (! winH) 
		return;
	
	ECWinValidateHandle(winH);
	
	winP = WinGetWindowPointer(winH);
	WinCopyRectangle(winH, 0, &winP->windowBounds, destX, destY, winPaint);
	WinDeleteWindow(winH, false);
}


/***********************************************************************
 *
 * FUNCTION:	WinScreenLock
 *
 * DESCRIPTION: "Locks" the current screen by switching the UI concept
 *				 of the screen base address to an area that is not reflected
 *				 on the LCD.  Screen can be unlocked later by bringing
 *				 the hardware into sync with the UI.  The screen must be
 *				 unlocked as many times as it is locked to actually update
 *				 the display.
 *
 * PARAMETERS:	initMode	->	how to initialize the new screen, can be:
 *						0 - "copy" copy old screen to new
 *						1 - "clear" erase new screen to white
 *						2 - "don't care" don't do anything
 *
 * RETURNED:	pointer to new screen base address, NULL if it failed. 
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	4/9/99	Initial Revision
 *
 ***********************************************************************/
UInt8* WinScreenLock(WinLockInitType initMode)
{
	return ScrScreenLock(initMode, GState.drawStateP->backColor, &GState.drawStateP->backColorRGB);
}


/***********************************************************************
 *
 * FUNCTION:	WinScreenUnlock
 *
 * DESCRIPTION: "Unlocks" the current screen by switching the hardware concept
 *				 of the screen base address to the UI state.  No-op if already
 *				 switched.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	4/9/99	Initial Revision
 *
 ***********************************************************************/
void WinScreenUnlock()
{
	ScrScreenUnlock(WinGetWindowPointer(DisplayWindow)->bitmapP);
}


/***********************************************************************
 *
 * FUNCTION:    WinPushDrawState
 *
 * DESCRIPTION: Push new set of draw state values onto draw state stack.
 *				Current values are copied into new set.  Programmers can
 *				then modify values at will, and restore to old state
 *				with a simple pop operation on the stack.
 *
 * RETURNED:    
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	1/22/99	Initial Revision
 *
 ***********************************************************************/
void WinPushDrawState()
{
	GState.drawStateIndex++;
	
	// if the stack "overflows", we don't generate an error, just degrade and
	// keep counting the pushes.  Result is the "pops" will be ineffective until
	// we get back into the stack range.

	if (GState.drawStateIndex < DrawStateStackSize) 
		{
		MemMove(&GState.drawStateStackP[GState.drawStateIndex],
				  &GState.drawStateStackP[GState.drawStateIndex - 1], sizeof(DrawStateType));
		GState.drawStateP = &GState.drawStateStackP[GState.drawStateIndex];
		DrawWindow->drawStateP = GState.drawStateP;
		}
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	else
		ErrDisplay("Draw state overflow, WinPushDrawState is a no-op.");
	
	// save current depth in drawState's reserved field, and check for depth mismatch
	// when the drawState is popped
	{	
	Err err;
	UInt32 depth;
	
	err = WinScreenMode(winScreenModeGet, 0, 0, &depth, 0);
	ErrNonFatalDisplayIf(err != errNone, "WinScreenMode error");
	GState.drawStateStackP[GState.drawStateIndex - 1].reserved = (UInt8) depth;
	}
#endif
}


/***********************************************************************
 *
 * FUNCTION:    WinPopDrawState
 *
 * DESCRIPTION: Restore drawing state to values before push
 *
 * RETURNED:    
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	1/22/99	Initial Revision
 *
 ***********************************************************************/
void WinPopDrawState()		
{
	GState.drawStateIndex--;
	if (GState.drawStateIndex < 0)
		{
		ErrNonFatalDisplay("Draw State stack underflow");
		GState.drawStateIndex = 0;
		return;
		}
	
	// if we've popped back into a range that's backed by the stack
	if (GState.drawStateIndex < DrawStateStackSize - 1) 
		{

#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
		// invalidate some of the 'popped' drawState to ensure no one uses it.
		GState.drawStateP->transferMode = (WinDrawOperation) 0xFF;
		GState.drawStateP->pattern = (PatternType) 0xFF;
		GState.drawStateP->underlineMode = (UnderlineModeType) 0xFF;
#endif
	
		GState.drawStateP = &GState.drawStateStackP[GState.drawStateIndex];
		DrawWindow->drawStateP = GState.drawStateP;

		// restore the UI font globals too!
		UICurrentFontID = GState.drawStateP->fontId;
		UICurrentFontPtr = GState.drawStateP->font;
		
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	{	
	// verify that depth of popped drawState matches depth at the time the drawState was pushed
	Err err;
	UInt32 depth;
	
	err = WinScreenMode(winScreenModeGet, 0, 0, &depth, 0);
	ErrNonFatalDisplayIf(err != errNone, "WinScreenMode error");
	ErrNonFatalDisplayIf(GState.drawStateP->reserved != (UInt8) depth, 
													"Current depth does not match popped drawState");
	GState.drawStateP->reserved = 0;
	}
#endif
		}
}


/***********************************************************************
 *
 * FUNCTION:    WinEraseWindow
 *
 * DESCRIPTION: This routine erases the contents of the draw window. 
 *              The frame around the draw window is not erased or drawn
 *					 by this routine.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/15/94	Initial Revision
 *
 ***********************************************************************/
void WinEraseWindow()
{
	RectangleType r;
	WindowType* winP;

	winP = WinGetWindowPointer(DrawWindow);
	RctSetRectangle (&r, 0, 0, winP->windowBounds.extent.x, winP->windowBounds.extent.y);

	WinEraseRectangle(&r, 0);
}


/***********************************************************************
 *
 * FUNCTION:    WinDrawWindowFrame
 *
 * DESCRIPTION: This routine draws the frame of the window that is
 *              currently the draw window.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/15/94	Initial Revision
 *
 ***********************************************************************/
void WinDrawWindowFrame()
{
	WinHandle drawWindowH;
	WindowType* winP;

	drawWindowH = WinSetDrawWindow(DisplayWindow);
	winP = WinGetWindowPointer(drawWindowH);
	WinDrawRectangleFrame(winP->frameType.word, &winP->windowBounds);
	WinSetDrawWindow(drawWindowH);
}


#pragma mark === Accessors ===
/************************************************************
 *
 *  FUNCTION: 		WinScreenMode
 *
 *  DESCRIPTION: 	Set and/or return display geometry, depth or colorness.
 *
 *			getDefaults - return default settings for display.
 *
 *			get - return current settings for the display.
 *						For this release width/height are fixed and
 *						will always be returned as width: 160, height: 160.
 *						Depth may be 1 or 2. enableColor true/false;
 *
 *			setToDefaults - changes display settings to the defaults above.
 *
 *			set - changes display settings.  Only non-NULL ptr values are used.
 *						NULL ptr values are left as is.  Width, height and enableColor are
 *						not changable for this release and must be 160/160 for this
 *						release if they are specified.  Depth can be 1,2,4 8.  Although the
 *						getSupportedDepths option (see below) should be used to determine
 *						which depths are supported.
 *
 *			getSupportedDepths - returns a "bit map" of supported screen depths.
 *						Each bit in the returned UInt16 indicates support (1) or not (0)
 *						for each display depth, with the bit position being the bit depth - 1.
 *						Some examples:
 *						Support for bit depths of 2 & 1 is indicated by 0x03.
 *						Support for bit depths of 4, 2 & 1 is indicated by 0x0B.
 *						Support for bit depths for 24, 8, 4 & 2 is indicated by 0x80008A.\
 *
 *			getSupportsColor - for a given screen size (only 160x160 for now),
 *						a boolean is returned indicating whether color mode is enabled.
 *
 *			Here's a table of how the arguments are used for each operation
 *			Returned values are indicated by 'out'.  Can be NULL.
 *			Required parameters are indicated by 'in'.  Can be NULL.
 *			Unused parameters are indicated by 'n/a'.
 *
 *			operation			widthP	heightP	depthP	enableColorP
 *			---------			------	-------	------	------------
 *			getDefaults			out		out		out		out
 *			get					out		out		out		out
 *			setToDefaults		n/a		n/a		n/a		n/a
 *			set					in		in		in		in
 *			getSupportedDepths	in		in		out		out
 *			getSupportsColor	in		in		in		out
 *
 *			When setting, null pointers mean use current settings
 *
 *  PARAMETERS: 	operation - operation to perform
 *					widthP - pointer to new / old screen width
 *					heightP - pointer to new / old screen height
 *					depthP - pointer to new / old / possible screen depth(s)
 *					enableColorP - pointer to color enable(d) value
 *
 *  RETURNS: 		error if invalid parameters are given
 *
 *************************************************************/
Err WinScreenMode(WinScreenModeOperation operation, UInt32* widthP, UInt32* heightP, 
						UInt32* depthP, Boolean* enableColorP)
{
	UInt16  	curDepth, dflDepth, setDepth, allDepths;
  	UInt16	dispHeight, dispWidth, dispColor;
  	UInt32  	ftrDepth;
  	Err      err;

	// First get the current depth
	err = HwrDisplayAttributes(false, hwrDispDepth, &curDepth);
	if (err)
		curDepth = 1;
	
	err = HwrDisplayAttributes(false, hwrDispAllDepths, &allDepths);
	
	// Get the default display depth.  If it doesn't exist (or its bad) then initialize it.
	err = FtrGet(sysFtrCreator, sysFtrNumDisplayDepth, &ftrDepth);
	dflDepth = (UInt16) ftrDepth;
	if (err || (((1 << (dflDepth-1)) & allDepths) == 0)) 
		{
		err = HwrDisplayAttributes(false, hwrDispBootDepth, &dflDepth);
		err = FtrSet(sysFtrCreator, sysFtrNumDisplayDepth, dflDepth);
		if (err) 
			return err;
		}
	
	// Initialize set depth to the current value
	setDepth = curDepth;
	
	// Get the width, height, and all depths of the device.
	err = HwrDisplayAttributes(false, hwrDispHorizontal, &dispWidth);
	err = HwrDisplayAttributes(false, hwrDispVertical, &dispHeight);	
	err = HwrDisplayAttributes(false, hwrDispColor, &dispColor);
	
 	// Check for invalid input args for *widthP and *heightP
	// Only one width/height combination is supported for this hardware
 	switch (operation) 
		{
 		case winScreenModeSet:
		case winScreenModeGetSupportedDepths:
		case winScreenModeGetSupportsColor:
			if (widthP != NULL) 
				{
				if (*widthP != dispWidth) 
					return sysErrParamErr;
				}
			if (heightP != NULL) 
				{
				if (*heightP != dispHeight) 
					return sysErrParamErr;
				}
			break;
		}
 	
 	// See what the client wants to do
 	switch (operation) 
		{
		case winScreenModeGetDefaults:
			curDepth = dflDepth;
			// Fall thru to next case on purpose
			
		case winScreenModeGet:
	 		if (widthP != NULL) 
				*widthP = dispWidth;
	 		if (heightP != NULL) 
				*heightP = dispHeight;
	 		if (depthP != NULL) 
				*depthP = curDepth;
	 		if (enableColorP != NULL) 
				*enableColorP = dispColor;
			break;
			
		case winScreenModeGetSupportedDepths:
			if (depthP == NULL) 
				return sysErrParamErr;
			*depthP = allDepths;
	 		if (enableColorP != NULL) 	
				*enableColorP = dispColor;
			break;
			
		case winScreenModeGetSupportsColor:
			if (enableColorP == NULL) 
				return sysErrParamErr;
			if (depthP != NULL)
				{
				// Check that the specified depth is a valid depth
				if (((1 << (*depthP - 1)) & allDepths) == 0) 
					return sysErrParamErr;
				}
			*enableColorP = dispColor;
			break;

		case winScreenModeSetToDefaults:
			setDepth = dflDepth;
			depthP = NULL;
			// Fall thru to next case on purpose
			
		case winScreenModeSet:
			if (enableColorP != NULL) 
				{
				if (*enableColorP != dispColor) 
					return sysErrParamErr;
				}
			
			if (depthP != NULL) 
				{
				setDepth = *depthP;
				// Check that the new depth is a valid depth
				if (((1 << (setDepth - 1)) & allDepths) == 0) 
					return sysErrParamErr;
				}
			
			if (setDepth != curDepth) 
				{
				// -----------------------------------------------------------
				// Change the display to the new depth
				// -----------------------------------------------------------
				err = HwrDisplayAttributes(true, hwrDispDepth, &setDepth);

				// -----------------------------------------------------------
				// Update BitmapType fields affected by the depth change.  The screen colorTable
				// is updated below in the call to WinPalette.
				// -----------------------------------------------------------
				err = ScrUpdateScreenBitmap(setDepth);
				}
				
			if ((setDepth != curDepth) || ! ScrDefaultPaletteState())
				{
				// ----------------------------------------------------------
				// Update the palette for the screen to the default system palette for this depth. 
				// This also rebuilds the color translation tables.
				// ----------------------------------------------------------
				WinHandle oldDrawWinH = DrawWindow;
				DrawWindow = DisplayWindow;
				WinPalette(winPaletteSetToDefault, 0 /*startIndex*/, 1 << setDepth /*numEntries*/, 0 /*tableP*/);
				DrawWindow = oldDrawWinH;
				}
				
				// send notification of depth change				
			if (setDepth != curDepth) 
				{
				SysNotifyParamType notifyParams;
				SysNotifyDisplayChangeDetailsType depthChangeDetails;
				
				depthChangeDetails.oldDepth = curDepth;
				depthChangeDetails.newDepth = setDepth;
	
				notifyParams.notifyType = sysNotifyDisplayChangeEvent;
				notifyParams.broadcaster = sysFileCSystem;
				notifyParams.notifyDetailsP = &depthChangeDetails;
				notifyParams.handled = false;
				
				SysNotifyBroadcast(&notifyParams);
				}
			break;
			
		default:
			return sysErrParamErr;
		}
 	
 	return err;
}


/***********************************************************************
 *
 * FUNCTION:    WinGetActiveWindow
 *
 * DESCRIPTION: This routine returns the window handle of the active 
 *              window.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    MemHandle of active window
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/15/94	Initial Revision
 *
 ***********************************************************************/
WinHandle WinGetActiveWindow()
{
	return ActiveWindow;
}


/***********************************************************************
 *
 * FUNCTION:    WinSetActiveWindow
 *
 * DESCRIPTION: This routine makes a window the active window.
 *              The active window is not actually set in this routine, flags
 *              are set to indicate that a window is being exited and 
 *              another window is being entered.  The routine 
 *              EvtGetEvent will send a winExitEvent and a winEnterEvent
 *              when it detects these flags.  The active window is set
 *              by EvtGetEvent when it sends the winEnterEvent.  The draw
 *              window is also set to the new active window, when the
 *              active window is changed.
 *
 *              The active window is where all user input is directed.
 *
 * PARAMETERS:  winH  MemHandle of the window to make the active window
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/15/94	Initial Revision
 *
 ***********************************************************************/
void WinSetActiveWindow(WinHandle winH)
{
	if (winH != NULL)
		ECWinValidateHandle(winH);
	
	if (winH != ActiveWindow)
		{
		if (ActiveWindow)
			ExitWindowID = ActiveWindow;
		EnterWindowID = winH;
		}
	else
		EnterWindowID = ActiveWindow;
		
	if (winH)
		WinEnableWindow(winH);
}


/***********************************************************************
 *
 * FUNCTION:    WinGetDrawWindow
 *
 * DESCRIPTION: This routine returns the window handle of the draw
 *              window.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    MemHandle of draw window
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/15/94	Initial Revision
 *			roger	10/11/99	Change to return NULL in debug ROMs.
 *			roger	10/11/99	Return DisplayWindow instead of NULL in release ROMs.
 *
 ***********************************************************************/
WinHandle WinGetDrawWindow()
{
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	if (DrawWindow == (WinHandle) badDrawWindowValue)
		return NULL;
#else
	if (DrawWindow == NULL)
		return DisplayWindow;
#endif

	return DrawWindow;
}


/***********************************************************************
 *
 * FUNCTION:    WinSetDrawWindow
 *
 * DESCRIPTION: This routine sets the draw window.  All drawing operation
 *              are relative to the draw window.
 *
 * PARAMETERS:  winH - MemHandle of the window to make the draw window
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/15/94	Initial Revision
 *			bob	4/1/99	Set drawStateP to current global "pen"
 *			jmp	9/28/99	Help bad apps limp along by preventing access
 *								to NULL window pointers and not allowing the
 *								DrawWindow to get set to NULL.
 *			roger	10/11/99	Allow DrawWindow to be set to NULL because there
 *								isn't always a valid window to draw to.
 *								In debug ROMs, map NULL to a value causing a bus error
 *
 ***********************************************************************/
WinHandle WinSetDrawWindow(WinHandle winH)
{
	WinHandle currDrawWindow;
	WindowType* winP = WinGetWindowPointer(winH);
	
	// The debug ROM returns NULL instead of 0x80000000 (badDrawWindowValue)
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
		if (DrawWindow == (WinHandle) badDrawWindowValue)
			currDrawWindow  = NULL;
		else
			currDrawWindow  = DrawWindow;
#else
		currDrawWindow  = DrawWindow;
#endif
	
	// Don't do anything if it's the new draw window is the same as the current.
	if (winH == currDrawWindow)
		return currDrawWindow;
	
	// If the new window is NULL, use it.  Debug ROMs convert it to the badDrawWindowValue.
	if (winH == NULL) 
		{
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
		DrawWindow = (WinHandle) badDrawWindowValue;
#else
		DrawWindow = DisplayWindow;		// disallow NULL for backwards compat. <sigh>
#endif
		return currDrawWindow;
		}
	
	
	// For compatibility with previous Palm OS's, don't dereference
	// winH if it's NULL.  Difference in PalmOS 3.5:  We don't allow
	// DrawWindow to be set to NULL, whereas earlier PalmOS's did.
	// This is because PalmOS 3.5 must access and dereference DrawWindow
	// directly much more extensively than did previous versions of
	// the PalmOS.
	//
	else
		{
		ECWinValidateHandle(winH);
		
#if ERROR_CHECK_LEVEL != ERROR_CHECK_FULL
		// Update the V20 address from the bitmap in case we're 'locked'
		winP->displayAddrV20 = BmpGetBits(winP->bitmapP);	
#endif

		// Tweak the new draw window's drawState to match the rest of the
		// OS's idea of the current pen.  If you don't do this, the drawState
		// may not reflect the current colors, pattern, etc.
		
		// DOLATER  invalidate translation cache?
		
		winP->drawStateP = GState.drawStateP;

		// Set the DrawWindow to the passed in winH if all is well.
		DrawWindow = winH;
		}

	return currDrawWindow;
}


/***********************************************************************
 *
 * FUNCTION:    WinGetDisplayWindow
 *
 * DESCRIPTION: This routine returns the window handle of the display
 *              window.  The display window is create by the system at
 *              startup, it size is the same as the physical display (screen).
 *
 * PARAMETERS:  none
 *
 * RETURNED:    MemHandle of display window
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/15/94	Initial Revision
 *
 ***********************************************************************/
WinHandle WinGetDisplayWindow()
{
	return DisplayWindow;
}


/***********************************************************************
 *
 * FUNCTION:    WinGetFirstWindow
 *
 * DESCRIPTION: This routine returns a pointer to the first window in
 *              the linked list of windows.
 *
 *				DOLATER ??? -ÊShould be OS only.
 *
 * RETURNED:    MemHandle of first window
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/15/94	Initial Revision
 *
 ***********************************************************************/
WinHandle  WinGetFirstWindow()
{
	return FirstWindow;	
}


/***********************************************************************
 *
 * FUNCTION:    WinGetDrawWindowBounds
 *
 * DESCRIPTION: This routine returns the bounds of the DrawWindow 
 *              in display relative coordinates.
 *
 * PARAMETERS:  rP	<-	set to the bounds of the window
 * 
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/3/95	Initial Revision
 *
 ***********************************************************************/
void WinGetDrawWindowBounds(RectangleType* rP)
{
	WindowType* winP;

	winP = WinGetWindowPointer(DrawWindow);
	RctCopyRectangle(&winP->windowBounds, rP);
}


/***********************************************************************
 *
 * FUNCTION:    WinGetBounds
 *
 * DESCRIPTION: This routine returns the bounds of the winH argument
 *              in display relative coordinates.
 *
 * PARAMETERS:  winH -> window for which to get bounds
 *					 rP	<-	set to the bounds of the window
 * 
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			acs	11/10/00	Initial Revision
 *
 ***********************************************************************/
void WinGetBounds(WinHandle winH, RectangleType* rP)
{
	WindowType* winP;
	
	ECWinValidateHandle(winH);
	
	winP = WinGetWindowPointer(winH);
	RctCopyRectangle(&winP->windowBounds, rP);
}


/***********************************************************************
 *
 * FUNCTION:    WinSetBounds
 *
 * DESCRIPTION: Set the bounds of the window to a new value (resize),
 *					new rectangle is in display relative coordinates.
 *
 * PARAMETERS:  	winH	->	window to set the bounds of
 *						rP 	->	new bounds for the window.
 * 
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/14/96	Initial Revision - needs fixing to MemHandle other cases.
 *
 ***********************************************************************/
void WinSetBounds(WinHandle winH, const RectangleType* rP)
{
	WindowType* winP;
	WinHandle	savedDrawWinH;

	ECWinValidateHandle(winH);
	
	winP = WinGetWindowPointer (winH);
	
	if (winP->windowFlags.visible)
		{
		ErrNonFatalDisplay("Visible windows can't change bounds");
		return;
		}
	
	RctCopyRectangle (rP, &winP->windowBounds);
	
	// fix up the clipping region, just in case.
	savedDrawWinH = WinSetDrawWindow(winP);
	WinResetClip();
	WinSetDrawWindow(savedDrawWinH);
}


/***********************************************************************
 *
 * FUNCTION:    WinGetWindowExtent
 *
 * DESCRIPTION: This routine returns the width and height of the current 
 *              draw window.
 *
 * PARAMETERS:  extentX	<-	set to the width of the draw window
 *              extentY	<-	set to the height of the draw window
 * 
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/28/94	Initial Revision
 *
 ***********************************************************************/
void WinGetWindowExtent (Coord* extentX, Coord* extentY)
{
	WindowType* winP;

	winP = WinGetWindowPointer(DrawWindow);

	if (extentX)
		*extentX = winP->windowBounds.extent.x;

	if (extentY)
		*extentY = winP->windowBounds.extent.y;
}


/***********************************************************************
 *
 * FUNCTION:    WinGetDisplayExtent
 *
 * DESCRIPTION: This routine returns the width and height of the display
 *              (the screen).
 *
 * PARAMETERS:  extentX	->	set to the width of the display
 *              extentY	->	set to the height of the display
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/28/94	Initial Revision
 *
 ***********************************************************************/
void WinGetDisplayExtent(Coord* extentX, Coord* extentY)
{
	WindowType* winP;

	winP = WinGetWindowPointer(DisplayWindow);
	if (extentX != NULL)
		*extentX = winP->windowBounds.extent.x;
	if (extentY != NULL)
		*extentY = winP->windowBounds.extent.y;
}


/***********************************************************************
 *
 * FUNCTION:    WinWindowToDisplayPt
 *
 * DESCRIPTION: This routine converts a window-relative coordinate to
 *              a display-relative coordinate.  The coordinate passed is 
 *              assumed to be relative to the draw window.
 *
 * PARAMETERS:  xP	<->	x coordinate to convert
 *              yP	<->	y coordinate to convert
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/28/94	Initial Revision
 *
 ***********************************************************************/
void WinWindowToDisplayPt(Coord* xP, Coord* yP)
{
	WindowType* winP;

	winP = WinGetWindowPointer(DrawWindow);
	*xP += winP->windowBounds.topLeft.x;
	*yP += winP->windowBounds.topLeft.y;
}


/***********************************************************************
 *
 * FUNCTION:    WinDisplayToWindowPt
 *
 * DESCRIPTION: This routine converts a display-relative coordinate to
 *              a window-relative coordinate.  The coordinate returned is 
 *              relative to the draw window.
 *
 * PARAMETERS:  xP	<->	x coordinate to convert
 *              yP	<->	y coordinate to convert
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/28/94	Initial Revision
 *
 ***********************************************************************/
void WinDisplayToWindowPt(Coord* xP, Coord* yP)
{
	WindowType* winP;

	winP = WinGetWindowPointer (DrawWindow);
	*xP -= winP->windowBounds.topLeft.x;
	*yP -= winP->windowBounds.topLeft.y;
}


/***********************************************************************
 *
 * FUNCTION:    WinGetClip
 *
 * DESCRIPTION: This routine returns the clipping rectangle of the 
 *              current draw window.
 *
 * PARAMETERS:  rP	<-	set to the clipping bounds of the draw window
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/28/94	Initial Revision
 *
 ***********************************************************************/
void WinGetClip(RectangleType* rP)
{
	WindowType* winP;
	Coord	left, top;

	winP = WinGetWindowPointer(DrawWindow);

	rP->topLeft.x = left = winP->clippingBounds.left;
	rP->topLeft.y = top = winP->clippingBounds.top;
	rP->extent.x =  winP->clippingBounds.right - left + 1;
	rP->extent.y =  winP->clippingBounds.bottom - top + 1;
}


/***********************************************************************
 *
 * FUNCTION:    WinSetClip
 *
 * DESCRIPTION: This routine sets the clipping rectangle of the draw 
 *              window.
 *
 * PARAMETERS:  rP	->	the new clipping bounds
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/28/94	Initial Revision
 *
 ***********************************************************************/
void WinSetClip(const RectangleType* rP)
{
	WindowType*		winP;
	RectangleType	clip;

	// restore clip to full window bounds prior to WinClipRectangle
	WinResetClip();
	
	// clip new rectangle to visible window bounds
	RctCopyRectangle(rP, &clip);
	WinClipRectangle(&clip);

	// set new rectangle as the clipping rectangle
	winP = WinGetWindowPointer(DrawWindow);
	winP->clippingBounds.left = clip.topLeft.x;
	winP->clippingBounds.top = clip.topLeft.y;
	winP->clippingBounds.right = clip.topLeft.x + clip.extent.x - 1;
	winP->clippingBounds.bottom = clip.topLeft.y + clip.extent.y - 1;
}


/***********************************************************************
 *
 * FUNCTION:    WinResetClip
 *
 * DESCRIPTION: This routine resets the clipping rectangle of the draw 
 *              window to the portion of the draw window that is within the 
 *              bounds of the display. 
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/28/94	Initial Revision
 *
 ***********************************************************************/
void WinResetClip()
{
	WindowType* winP;

	winP = WinGetWindowPointer(DrawWindow);

	winP->clippingBounds.left = 0;
	winP->clippingBounds.top = 0;
	
	winP->clippingBounds.right = min(winP->windowBounds.extent.x,
												winP->bitmapP->width - winP->windowBounds.topLeft.x) - 1;

	winP->clippingBounds.bottom = min(winP->windowBounds.extent.y,
		 										 winP->bitmapP->height - winP->windowBounds.topLeft.y) - 1;
}


/***********************************************************************
 *
 * FUNCTION:    WinClipRectangle
 *
 * DESCRIPTION: This routine clips a rectangle to the clipping rectangle
 *              of the draw window.  The rectangle returned is the 
 *              intersection of the rectangle passed and the draw window's
 *              clipping bounds.  The rectangle coordinates are relative
 *					 to the current draw window.
 *
 * PARAMETERS:  rP	<->	the rectangle to clip
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/28/94	Initial Revision
 *			bob	10/07/99	optimized (stole code from RctGetIntersection)
 *
 ***********************************************************************/
void WinClipRectangle(RectangleType* rP)
{
	WindowType*	winP;
	Coord left, right, top, bottom;

	winP = WinGetWindowPointer(DrawWindow);
	
	left = max(winP->clippingBounds.left, rP->topLeft.x);
	top = max(winP->clippingBounds.top, rP->topLeft.y);
	right = min(winP->clippingBounds.right+1, rP->topLeft.x + rP->extent.x);
	bottom = min(winP->clippingBounds.bottom+1, rP->topLeft.y + rP->extent.y);
	
	rP->topLeft.x = left;
	rP->topLeft.y = top;

	right -= left;
	rP->extent.x = (right < 0) ? 0 : right;

	bottom -= top;
	rP->extent.y = (bottom < 0) ? 0 : bottom;
}


/***********************************************************************
 *
 * FUNCTION:    WinGetWindowFrameRect
 *
 * DESCRIPTION: This routine returns a rectangle, in display relative
 *              coordinates, that defines the size and location of a 
 *              window and its frame.
 *
 * PARAMETERS:  winH	 ->	MemHandle of window whose coordinates are desired
 *              rP	<-  result, rectangle of window including frame
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/15/94	Initial Revision
 *
 ***********************************************************************/
void WinGetWindowFrameRect (WinHandle winH, RectangleType* rP)
{
	WindowType* winP;

	ECWinValidateHandle(winH);
	
	winP = WinGetWindowPointer(winH);
	WinGetFramesRectangle(winP->frameType.word, &winP->windowBounds, rP);
}


/***********************************************************************
 *
 * FUNCTION:    WinGetFramesRectangle
 *
 * DESCRIPTION: This function returns the region needed to draw a 
 *              a rectangle with the specified frame around it.  Frames 
 *              are always drawn around (outside) a rectangle.
 *
 * PARAMETERS:  frame		 ->	type of frame drawn around the rectangle
 *              rP			 ->	the rectangle to frame
 *              obscuredRect<-	result, input plus frame area
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/14/94	Initial Revision
 *
 ***********************************************************************/
void WinGetFramesRectangle (FrameType frame, const RectangleType* rP, RectangleType* obscuredRectP)
{
	FrameBitsType frameType;

	frameType.word = frame;
	RctCopyRectangle(rP, obscuredRectP);

	// Outset the rectangle by the width of the frame.
	RctInsetRectangle (obscuredRectP, -frameType.bits.width);

	// Add the width of the shodow to the right and botton sides of the rectangle.
	obscuredRectP->extent.x += frameType.bits.shadowWidth;
	obscuredRectP->extent.y += frameType.bits.shadowWidth;
}


/***********************************************************************
 *
 * FUNCTION:    WinGetPattern
 *
 * DESCRIPTION: This routine returns the current fill pattern.  The fill 
 *              pattern is use by the WinFill... routines.
 *
 * PARAMETERS:  pattern	<-	buffer to hold return value, the pattern
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/30/94	Initial Revision
 *
 ***********************************************************************/
void WinGetPattern(CustomPatternType* patternP)
{
	WindowType* winP = WinGetWindowPointer(DrawWindow);

	MemMove(patternP, winP->drawStateP->patternData, sizeof(CustomPatternType));
}


/***********************************************************************
 *
 * FUNCTION:    WinSetPattern
 *
 * DESCRIPTION: This routine set the current fill pattern.  The fill 
 *              pattern is use by the WinFill... routines.
 *
 * PARAMETERS:  pattern	->	pattern to set to
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/30/94	Initial Revision
 *
 ***********************************************************************/
void WinSetPattern(const CustomPatternType* patternP)
{
	WindowType* winP = WinGetWindowPointer(DrawWindow);

	MemMove(winP->drawStateP->patternData, patternP, sizeof(CustomPatternType));
	winP->drawStateP->pattern = customPattern;
}


/***********************************************************************
 *
 * FUNCTION:    WinGetPatternType
 *
 * DESCRIPTION: This routine returns the current pattern type.
 *
 * PARAMETERS:  none
 *
 * RESULT:		current pattern type
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob		7/27/99		Initial Revision
 *
 ***********************************************************************/
PatternType WinGetPatternType()
{
	WindowType* winP = WinGetWindowPointer(DrawWindow);
	return winP->drawStateP->pattern;
}


/***********************************************************************
 *
 * FUNCTION:    WinSetPatternType
 *
 * DESCRIPTION: Set the pattern type for the draw window.
 *					 Note, use WinSetPattern to set to a custom pattern.
 *
 * PARAMETERS:  newPattern	->	pattern type to set to
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	6/11/99	Initial Revision
 *
 ***********************************************************************/
void WinSetPatternType(PatternType newPattern)
{
	WindowType* winP = WinGetWindowPointer(DrawWindow);

	winP->drawStateP->pattern = newPattern;
	
	if (newPattern != customPattern)
		MemSet(winP->drawStateP->patternData, sizeof(CustomPatternType), 0x00);
}


/***********************************************************************
 *
 * FUNCTION:    WinModal
 *
 * DESCRIPTION: This routine returns true if the specifed window is modal 
 *
 * PARAMETERS:  winH	->	MemHandle of window to check
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	9/14/95	Initial Revision
 *
 ***********************************************************************/
Boolean WinModal(WinHandle winH)
{
	WindowType* winP;

	ECWinValidateHandle(winH);
	
	winP = WinGetWindowPointer(winH);
	return winP->windowFlags.modal;
}


/***********************************************************************
 *
 * FUNCTION:    WinGetBitmap
 *
 * DESCRIPTION: This routine returns a pointer to the bitmap of the 
 *              current draw window.
 *
 * PARAMETERS:  
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			BS		9/7/99	Initial Revision
 *
 ***********************************************************************/
BitmapType* WinGetBitmap(WinHandle winHandle)
{
	WindowType* winP = WinGetWindowPointer(winHandle);
	if (winP == NULL) 
		{
		ErrNonFatalDisplay("NULL window");
		return NULL;
		}
		
	return winP->bitmapP;
}


/***********************************************************************
 *
 * FUNCTION:    WinSetDrawMode
 *
 * DESCRIPTION: Set the transfer mode to use in subsequent draw operations.
 *
 * PARAMETERS:  newMode		->	new transfer mode
 *
 * RETURNED:    old transfer mode
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	1/22/99	Initial Revision
 *
 ***********************************************************************/
WinDrawOperation WinSetDrawMode(WinDrawOperation newMode)
{
	WindowType* winP = WinGetWindowPointer(DrawWindow);
	WinDrawOperation result = winP->drawStateP->transferMode;
	winP->drawStateP->transferMode = newMode;
	
	return result;
}


/***********************************************************************
 *
 * FUNCTION:    WinSetUnderlineMode
 *
 * DESCRIPTION: This function sets the graphic state to enable or disable
 *              the underlining of characters.  
 *
 * PARAMETERS:  mode	-> new type of underlining to use
 *
 * RETURNED:	old underline mode
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/9/95		Initial Revision
 *
 ***********************************************************************/
UnderlineModeType WinSetUnderlineMode(UnderlineModeType mode)
{
	WindowType* winP = WinGetWindowPointer (DrawWindow);
	UnderlineModeType oldMode = winP->drawStateP->underlineMode;
	winP->drawStateP->underlineMode = mode;
	
	return oldMode;
}
