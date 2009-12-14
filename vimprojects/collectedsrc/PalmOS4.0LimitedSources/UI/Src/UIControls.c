/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: UIControls.c
 *
 * Release: 
 *
 * Description:
 *             	Contrast & brightness control for devices with
 *						software contrast.  Color picker too.
 *
 * History:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	2/12/98	Initial version
 *			JFS	10/5/98	Synchronized entire file from Sumo source tree
 *			bob	3/18/99	Generalize to support contrast or brightness
 *			bob	08/27/99	Add UIPickColor support, rename from Contrast.c
 *
 *****************************************************************************/

#define NON_PORTABLE

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_CLIPBOARDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_FIELDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS
#define ALLOW_ACCESS_TO_INTERNALS_OF_BITMAPS
#define ALLOW_ACCESS_TO_INTERNALS_OF_CONTROLS
#define ALLOW_ACCESS_TO_INTERNALS_OF_FORMS
#define ALLOW_ACCESS_TO_INTERNALS_OF_LISTS

#include <PalmTypes.h>

#include <SystemPublic.h>

#include "Form.h"
#include "Menu.h"
#include "UIControls.h"
#include "UIResourcesPrv.h"
#include "UIColorPrv.h"
#include "UIGlobals.h"

// system private includes
#include "Globals.h"

// hardware public includes
#include "HwrGlobals.h"
#include "HwrMiscFlags.h"
#include "HwrDisplay.h"

//-----------------------------------------------------------------------
// Global defines for this module
//-----------------------------------------------------------------------
#define minSysLevelValue		0x00
#define maxSysLevelValue		0xFF

#define largerIncrementThreshold		8		// number of times to do "small" adjustment with keys before accellerating
#define largerIncrementValue			4		// increment value for accellerated adjustment

#define maxChitSize						20		// don't let palette chits get bigger than this

static const RGBColorType black = {0x00, 0x00, 0x00, 0x00};
static const RGBColorType white = {0x00, 0xFF, 0xFF, 0xFF};


//-----------------------------------------------------------------------
// Structures for this module
//-----------------------------------------------------------------------

typedef enum {Contrast, Brightness}	SysLevelType;

typedef struct {
	IndexedColorType		newIndex;
	IndexedColorType		changingRGBIndex;
	Boolean					useIndex;
	Boolean					useRGB;
	RGBColorType			newRGB;
	RGBColorType			oldRGBValue;
	UIPickColorStartType current;
} ColorPickerDataType;


//-----------------------------------------------------------------------
// Private functions for this module
//-----------------------------------------------------------------------




/***********************************************************************
 *
 *  FUNCTION:     PrvSetSysLevel
 *
 *  DESCRIPTION:  Sets the system level, using a scale relative to the
 *						slider bitmap
 * 
 *  PARAMETERS:   whatToSet	->	contrast or brightness
 *						newSetting	-> new level setting
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	7/21/98	Initial Revision
 *			bob	3/18/99	Generalize to support contrast or brightness
 *			bob	8/9/99	remove psuedo-globals stuff
 *
 ***********************************************************************/
static Int16 PrvSetSysLevel(SysLevelType whatToSet, Int16 newSetting)
{
	Int16 newSysValue = newSetting;

	if (newSysValue < minSysLevelValue)
	{
		SndPlaySystemSound (sndWarning);
		newSysValue = minSysLevelValue;
	}
	else if (newSysValue > maxSysLevelValue)
	{
		SndPlaySystemSound (sndWarning);
		newSysValue = maxSysLevelValue;
	}

	if (whatToSet == Contrast)
		SysLCDContrast(true, (UInt8) newSysValue);
	else	// (whatToSet == Brightness)
		SysLCDBrightness(true, (UInt8) newSysValue);
	
	return newSysValue;
}


/***********************************************************************
 *
 *  FUNCTION:     PrvGetSysLevel
 *
 *  DESCRIPTION:  Gets the level on a scale relative to the slider bitmap
 * 
 *  PARAMETERS:   whatToSet	->	contrast or brightness
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	7/21/98	Initial Revision
 *			bob	3/18/99	Generalize to support contrast or brightness
 *			bob	8/9/99	remove psuedo-globals stuff
 *
 ***********************************************************************/
static Int16 PrvGetSysLevel(SysLevelType whatToSet)
{
	if (whatToSet == Contrast)
		return SysLCDContrast(false, 0) - minSysLevelValue;
	else	// (whatToSet == Brightness)
		return SysLCDBrightness(false, 0) - minSysLevelValue;
}


/***********************************************************************
 *
 *  FUNCTION:     PrvLevelAdjustEventLoop
 *
 *  DESCRIPTION:  Run a nested event loop for the level form.
 * 
 *  PARAMETERS:   frmP	->	The level form
 *						whatToSet -> contrast or brightness
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	7/21/98	Initial Revision
 *			bob	3/18/99	generalize from contrast to any slider
 *			bob	8/9/99	recode to use slider control
 *			jmp	10/15/99	Prevent the brightness slider from coming up
 *								multiple times while it is already up!  Now, if the
 *								the brightness slider is up and the backlight button
 *								is hit, we dismiss the brightness slider altogether.
 *			SCL	11/ 3/99	Now look for contrastChr instead of hardContrastChr.
 *
 ***********************************************************************/
static void PrvLevelAdjustEventLoop(FormType *frmP, SysLevelType whatToSet)
{
	EventType event;
	Err err;
	Int16 count;
	Int16 newValue;
	RectangleType boundsR;
	Boolean handled;
	SliderControlType *sldP;
	UInt16 thumbWidth;
	Boolean sliderNeedsUpdate = false;
	MemHandle h;
	BitmapType *p;
	UInt16 sliderIndex;

	// get slider width and thumb width for value->pixel conversion
	if (whatToSet == Contrast)
		sliderIndex = FrmGetObjectIndex(frmP, SysContrastAdjustSlider);
	else
		sliderIndex = FrmGetObjectIndex(frmP, SysBrightnessAdjustSlider);
		
	sldP = FrmGetObjectPtr(frmP, sliderIndex);
	FrmGetObjectBounds(frmP, sliderIndex, &boundsR);

	h = DmGetResource(bitmapRsc, sldP->thumbID);
	p = MemHandleLock(h);
	thumbWidth = p->width;
	MemPtrUnlock(p);
	DmReleaseResource(h);
	
	while (true)
	{
		if (sliderNeedsUpdate)
			CtlSetValue((ControlType*)sldP, PrvGetSysLevel(whatToSet));

		EvtGetEvent (&event, evtWaitForever);

		// bail out immediately if we need to close
		if (frmP->attr.exitDialog) {
			EvtAddEventToQueue (&event);
			return;
		}
		
		if (whatToSet == Contrast) {
			// If the contrast char comes through again, exit immediately.
			if (event.eType == keyDownEvent && event.data.keyDown.chr == contrastChr)
				return;

			// If the backlight is going on or off, the slider value may become incorrect
			// because the contrast value may invert (Epson display only).  So, update the
			// slider, but only after SysHandleEvent changes the backlight!
			if (event.eType == keyDownEvent && event.data.keyDown.chr == backlightChr)
				sliderNeedsUpdate = true;
		}
		else { // (whatToSet == Brightness)
			// If the brightness char comes through again, exit immediately.
			if (event.eType == keyDownEvent && event.data.keyDown.chr == brightnessChr)
				return;
		}
		
		
		if (! SysHandleEvent (&event))
			if (! MenuHandleEvent (0, &event, &err))
			{
				handled = false;
				
				// handle the slider controls
				switch (event.eType)
				{
					case ctlSelectEvent:
						if (event.data.ctlSelect.controlID == SysLevelAdjustDoneButton)
							return;
						if (event.data.ctlSelect.controlID == SysContrastAdjustSlider ||
							 event.data.ctlSelect.controlID == SysBrightnessAdjustSlider)
		   				newValue = PrvSetSysLevel(whatToSet, event.data.ctlSelect.value);
						break;
											
					case ctlRepeatEvent:
						if (event.data.ctlSelect.controlID == SysContrastAdjustSlider ||
							 event.data.ctlSelect.controlID == SysBrightnessAdjustSlider)
		   				newValue = PrvSetSysLevel(whatToSet, event.data.ctlRepeat.value);
						break;
						
			   	case keyDownEvent:
						if	(	(!TxtCharIsHardKey(	event.data.keyDown.modifiers,
															event.data.keyDown.chr))
							&&	(EvtKeydownIsVirtual(&event))
							&&	(	(event.data.keyDown.chr == vchrPageUp)
								||	(event.data.keyDown.chr == vchrPageDown)))
						{
							// figure out how far to scroll (one pixel or largerIncrementValue pixels)
							if (event.data.keyDown.modifiers & autoRepeatKeyMask)
								count++;
							else
								count = 0;
								
							if (count > largerIncrementThreshold)
								newValue = sldP->pageSize;
							else {
								// newValue = 1 * (maxSysLevelValue - minSysLevelValue);
								newValue = (maxSysLevelValue - minSysLevelValue);
								newValue += (boundsR.extent.x - thumbWidth) / 2;	// round
								newValue /= (boundsR.extent.x - thumbWidth);
							}
							
							// adjust control in proper direction
							if (event.data.keyDown.chr == vchrPageDown)
								newValue = CtlGetValue((ControlType*)sldP) - newValue;
							else if (event.data.keyDown.chr == vchrPageUp)
								newValue = CtlGetValue((ControlType*)sldP) + newValue;

							// set it
							newValue = PrvSetSysLevel(whatToSet, newValue);
							CtlSetValue((ControlType*)sldP, newValue);
							handled = true;
						}
						break;
											
					case frmTitleSelectEvent:
						// play a warning if they tap in the title
						SndPlaySystemSound(sndError);
						handled = true;
					break;
					
				}

				if (!handled)
					FrmHandleEvent (frmP, &event);
			}
			
		// if we get an app switch command, abort the level form and
		// let the calling app deal with the event!
		if (event.eType == appStopEvent)
		{
			EvtAddEventToQueue (&event);
			return;
		}
	}
}


/***********************************************************************
 *
 *  FUNCTION:     PrvSettingAdjust
 *
 *  DESCRIPTION:  Handles user interaction with the slider dialog.
 *						Dialog is modal, and will be dismissed if the user
 *						invokes it again, or presses the Done
 *						button, or any hardware button to launch an app.
 *
 *	PARAMETERS:		whatToSet	-> contrast or brightness?
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	7/21/98	Initial Revision
 *			bob	3/18/99	generalize from contrast to any byte setting
 *			bob	8/9/99	Remove psuedoGlobals stuff, use new slider control
 *			bob	09/23/99	Test SysUIBusy rather than WinModal
 *
 ***********************************************************************/
static void PrvSettingAdjust(SysLevelType whatToSet)
{
	FormType *frmP;
	FormActiveStateType	curFrmState;
	Boolean	bailedOut = false;

	// don't allow find dialog to come up if UI is marked busy
	// or, if the form is already open then just beep and return
	if (SysUIBusy(false, false) > 0 ||
		 (whatToSet == Contrast && FrmGetFormPtr(SysContrastAdjustForm) != NULL) ||
		 (whatToSet == Brightness && FrmGetFormPtr(SysBrightnessAdjustForm) != NULL))
		{
      SndPlaySystemSound (sndError);
		return;
		}
/*
	{
		// If the active window is modal, don't allow the slider dialog to appear.
		FormType * curFrm;
		WinHandle activeWin;
		activeWin = WinGetActiveWindow();
		if (activeWin && WinModal(activeWin))
			return;
		curFrm = FrmGetActiveForm();
		if (curFrm && WinModal (FrmGetWindowHandle(curFrm)))
			return;
	}
*/

	// save active form/window state
	FrmSaveActiveState(&curFrmState);
				
	// ...the slider form itself
	if (whatToSet == Contrast)
		frmP = FrmInitForm (SysContrastAdjustForm);
	else
		frmP = FrmInitForm (SysBrightnessAdjustForm);
	
	frmP->attr.doingDialog = true;		// mark this as a dialog w/nested event loop
		
	ECFrmValidatePtr(frmP);
	FrmSetActiveForm (frmP);
	
	
	// initialize slider value
	CtlSetValue(FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP,
					(whatToSet == Contrast) ? SysContrastAdjustSlider : SysBrightnessAdjustSlider)),
					PrvGetSysLevel(whatToSet));

	// actually drive the form
	if (!frmP->attr.visible)
	 	FrmDrawForm(frmP);

	PrvLevelAdjustEventLoop(frmP, whatToSet);

	FrmEraseForm(frmP);
	bailedOut = frmP->attr.exitDialog;
	frmP->attr.doingDialog = false;
	FrmDeleteForm(frmP);

	// restore active form/window state
	if (!bailedOut)
		FrmRestoreActiveState(&curFrmState);
}


/***********************************************************************
 *
 *  FUNCTION:     ContrastAdjust
 *
 *  DESCRIPTION:  Handles user interaction with the contrast dialog.
 *						Dialog is modal, and will be dismissed if the user
 *						presses the contrast button again, or presses the Done
 *						button, or any hardware button to launch an app.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	7/21/98	Initial Revision
 *			bob	3/15/99	Moved bulk of code to PrvSettingAdjust
 *
 ***********************************************************************/
void UIContrastAdjust()
{
	if (!(GHwrMiscFlags & hwrMiscFlagHasSWContrast))
	{
		ErrNonFatalDisplay("Device does not have software contrast");
		return;
	}

	PrvSettingAdjust(Contrast);
}



/***********************************************************************
 *
 *  FUNCTION:     BrightnessAdjust
 *
 *  DESCRIPTION:  Handles user interaction with the brightness dialog.
 *						Dialog is modal, and will be dismissed if the user
 *						hits the backlightChr again, or presses the Done
 *						button, or any hardware button to launch an app.
 *						If the Hwr does not support brightness, just toggle
 *						the backlight.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	3/15/99	Initial revision
 *			bob	08/27/99	Just toggle if hwr does not support backlight
 *
 ***********************************************************************/
void UIBrightnessAdjust()
{
	// if the hardware supports an adjustable brightness, use the slider
	if ((GHwrMiscFlagsExt & hwrMiscFlagExtHasSWBright) != 0)
		PrvSettingAdjust(Brightness);
	
	// otherwise, just toggle the backlight
	else
		// 12/1/98 JAQ : removed comment noise about sysMiscFlagBacklightDisable legacy flag.
		if ((GHwrMiscFlags & hwrMiscFlagHasBacklight) != 0) {
			Boolean oldState;
			HwrDisplayAttributes(false, hwrDispBacklight, &oldState);
			oldState = !oldState;
			HwrDisplayAttributes(true, hwrDispBacklight, &oldState);
		}
}



#pragma mark --------- Color Pickers ---------




/***********************************************************************
 *
 * FUNCTION:		PrvSampleGadgetHandler
 *
 * DESCRIPTION:	Handle gadget stuff for sample swatch, drawing the
 *						current color selection.
 *
 * PARAMETERS:		gadgetP	->	the gadget
 *						cmd	 	->	what to do (draw or handle event)
 *						paramP	->	EventPtr for event cmd	
 *
 ***********************************************************************/
static Boolean PrvSampleGadgetHandler(FormGadgetType *gadgetP, UInt16 cmd, void * /*paramP*/)
{
	RectangleType r = gadgetP->rect;
	ColorPickerDataType *cpDataP = (ColorPickerDataType *)gadgetP->data;

	if (cmd != formGadgetDrawCmd)
		return false;
	
	WinPushDrawState();
	WinSetForeColor(UIColorGetIndex(UIObjectFrame));
	RctInsetRectangle(&r, 1);
	WinDrawRectangleFrame(rectangleFrame, &r);
	WinSetForeColor(cpDataP->changingRGBIndex);
	WinDrawRectangle(&r, 0);
	WinPopDrawState();
	
	return true;
}


/***********************************************************************
 *
 * FUNCTION:		PrvChitGadgetHandler
 *
 * DESCRIPTION:	Handle gadget stuff for color chit, maintaining the
 *						highlighting of the current color selection.
 *
 * PARAMETERS:		gadgetP	->	the gadget
 *						cmd	 	->	what to do (draw or handle event)
 *						paramP	->	EventPtr for event cmd	
 *
 ***********************************************************************/
static Boolean PrvChitGadgetHandler(FormGadgetType *gadgetP, UInt16 cmd, void *paramP)
{
	UInt32				depth;
	Int32					rows, columns, colors;
	Int32					newX, newY, x, y;
	IndexedColorType	index;
	Coord					colZeroX, rowZeroY;
	RectangleType		r, r2;
	RectanglePtr		boundsP = &gadgetP->rect;
	EventType *			eventP = paramP;
	ColorPickerDataType *cpDataP = (ColorPickerDataType *)gadgetP->data;

	// only handle draw and gadgetEnter
	if (cmd == formGadgetDrawCmd)
		eventP = NULL;
	else if (cmd != formGadgetHandleEventCmd || eventP->eType != frmGadgetEnterEvent)
		return false;
		
	WinScreenMode(winScreenModeGet, NULL, NULL, &depth, NULL);
	switch (depth) {
		case 1:	rows = 1;	columns = 2;	colors = 2;		break;
		case 2:	rows = 1;	columns = 4;	colors = 4;		break;
		case 4:	rows = 2;	columns = 8;	colors = 16;	break;
		case 8:	rows = 15;	columns = 18;	colors = 256;	break;
		default: ErrNonFatalDisplay("Unsupported depth");
	}

	// get box sizes
	r.extent.x = boundsP->extent.x / columns - 1;
	r.extent.y = boundsP->extent.y / rows - 1;
	
	if (r.extent.x > maxChitSize)
		r.extent.x = maxChitSize;

	// make square
	if (r.extent.x > r.extent.y)
		r.extent.x = r.extent.y;
	if (r.extent.y > r.extent.x)
		r.extent.y = r.extent.x;
	
	colZeroX = boundsP->topLeft.x + (boundsP->extent.x - columns * (r.extent.x+1) - 1) / 2;
	rowZeroY = boundsP->topLeft.y + (boundsP->extent.y - rows * (r.extent.y+1) - 1) / 2;

	// track
	while (!eventP || eventP->penDown) {
	
		if (eventP) {
			// figure out new item
			newX = (eventP->screenX - colZeroX) / (r.extent.x + 1);
			if (newX < 0) newX = 0;
			if (newX >= columns) newX = columns-1;

			newY = (eventP->screenY - rowZeroY) / (r.extent.y + 1);
			if (newY < 0) newY = 0;
			if (newY >= rows) newY = rows-1;
			
			// handle potentially incomplete last row w/out overflowing index variable
			if (newY * columns + newX >= colors)
				index = colors-1;
			else
				index = newY * columns + newX;
		}
		
		// just drawing, index is whatever is passed in
		else
			index = cpDataP->newIndex;
		
		// pin index to valid range
		if (index < 0)
			index = 1;
		if (index >= colors)
			index = colors-1;
		
		// recompute x,y from index, also initial computation if just drawing
		newY = index / columns;
		newX = index % columns;

		if (!eventP || cpDataP->newIndex != index) {
			// update saved index with new control
			cpDataP->newIndex = index;
			WinIndexToRGB(index, &(cpDataP->newRGB));
			SndPlaySystemSound(sndClick);

			// erase old indicator by filling in all the cracks with the
			// new selected color
			WinSetBackColor(index);
			
			// fill in areas around outside of chits
			RctCopyRectangle(boundsP, &r2);
			r2.extent.y = rowZeroY - boundsP->topLeft.y;
			WinEraseRectangle(&r2, 0);

			r2.extent.y = boundsP->extent.y - r2.extent.y - rows * (r.extent.y + 1);
			r2.topLeft.y = boundsP->topLeft.y + boundsP->extent.y - r2.extent.y;
			WinEraseRectangle(&r2, 0);
			
			RctCopyRectangle(boundsP, &r2);
			r2.extent.x = colZeroX - boundsP->topLeft.x;
			WinEraseRectangle(&r2, 0);

			r2.extent.x = boundsP->extent.x - r2.extent.x - columns * (r.extent.x + 1);
			r2.topLeft.x = boundsP->topLeft.x + boundsP->extent.x - r2.extent.x;
			WinEraseRectangle(&r2, 0);
			
			// erase any unused blocks at the bottom right
			if (colors % columns != 0) {
				RctCopyRectangle(&r, &r2);
				r2.topLeft.x = colZeroX + (colors % columns) * (r2.extent.x + 1);
				r2.extent.x = (columns - (colors % columns)) * (r2.extent.x + 1);
				r2.topLeft.y = rowZeroY + (colors / columns) * (r2.extent.y + 1);
				r2.extent.y += 1;
				WinEraseRectangle(&r2, 0);
			}
			
			// fill in the lines between the chits
			for (y = 0; y < rows + 1; y++)
				WinEraseLine(colZeroX, rowZeroY + y * (r.extent.y + 1),
								 colZeroX + columns * (r.extent.x + 1), rowZeroY + y * (r.extent.y + 1));
			
			for (x = 0; x < columns + 1; x++)
				WinEraseLine(colZeroX + x * (r.extent.x + 1), rowZeroY,
								 colZeroX + x * (r.extent.x + 1), rowZeroY + rows * (r.extent.y + 1));

			if (!eventP) {
				// loop and draw all the chits in order
				index = 0;
				r.topLeft.y = rowZeroY + 1;
				for (y = 0; y < rows; y++) {
					
					r.topLeft.x = colZeroX + 1;
					for (x = 0; x < columns; x++) {
						WinSetForeColor(index++);
						WinDrawRectangle(&r, 0);
						r.topLeft.x += r.extent.x + 1;
						if (index >= colors || index == 0)
							break;
					}
					r.topLeft.y += r.extent.y + 1;
					if (index >= colors || index == 0)
						break;
				}
			}

			// draw new indicator
			r.topLeft.x = colZeroX + newX * (r.extent.x + 1) + 1;
			r.topLeft.y = rowZeroY + newY * (r.extent.y + 1) + 1;
			
			// pick a contrasting color for the indicator, black or white
			// depending on luminance
			if (3 * cpDataP->newRGB.r + 10 * cpDataP->newRGB.g + cpDataP->newRGB.b < 1785)	// luminance range is 0..3570 inclusive
				WinSetForeColor(WinRGBToIndex(&white));
			else
				WinSetForeColor(WinRGBToIndex(&black));
			WinDrawRectangleFrame(rectangleFrame, &r);
		}
		
		// if we're just drawing, get out of the loop and exit
		if (!eventP)
			break;
			
		// get new point
		PenGetPoint (&eventP->screenX, &eventP->screenY, &eventP->penDown);
	}
	
	return true;
}


static void PrvColorPickerUpdateControls(FormPtr formP, ColorPickerDataType *cpDataP)
{
	Char textP[16];
	
	if (cpDataP->current == UIPickColorStartPalette) {
		HwrDisplayPalette(true, cpDataP->changingRGBIndex, 1, (HwrDisplayRGBPtr)(&(cpDataP->oldRGBValue)));
		FrmHideObject(formP, FrmGetObjectIndex(formP, ColorPickerRedSlider));
		FrmHideObject(formP, FrmGetObjectIndex(formP, ColorPickerRedLabel));
		FrmHideObject(formP, FrmGetObjectIndex(formP, ColorPickerRedValue));
		FrmHideObject(formP, FrmGetObjectIndex(formP, ColorPickerGreenSlider));
		FrmHideObject(formP, FrmGetObjectIndex(formP, ColorPickerGreenLabel));
		FrmHideObject(formP, FrmGetObjectIndex(formP, ColorPickerGreenValue));
		FrmHideObject(formP, FrmGetObjectIndex(formP, ColorPickerBlueSlider));
		FrmHideObject(formP, FrmGetObjectIndex(formP, ColorPickerBlueLabel));
		FrmHideObject(formP, FrmGetObjectIndex(formP, ColorPickerBlueValue));
		FrmHideObject(formP, FrmGetObjectIndex(formP, ColorPickerSampleColorGadget));
		FrmShowObject(formP, FrmGetObjectIndex(formP, ColorPickerChitBoundsGadget));
	}
	
	else if (cpDataP->current == UIPickColorStartRGB) {
		FrmHideObject(formP, FrmGetObjectIndex(formP, ColorPickerChitBoundsGadget));

		FrmShowObject(formP, FrmGetObjectIndex(formP, ColorPickerRedLabel));
		CtlSetValue(FrmGetObjectPtr(formP, FrmGetObjectIndex(formP, ColorPickerRedSlider)), cpDataP->newRGB.r / 17);
		FrmShowObject(formP, FrmGetObjectIndex(formP, ColorPickerRedSlider));
		StrIToH(textP, cpDataP->newRGB.r);
		FrmCopyLabel (formP, ColorPickerRedValue, &textP[6]);
		FrmShowObject(formP, FrmGetObjectIndex(formP, ColorPickerRedValue));

		FrmShowObject(formP, FrmGetObjectIndex(formP, ColorPickerGreenLabel));
		CtlSetValue(FrmGetObjectPtr(formP, FrmGetObjectIndex(formP, ColorPickerGreenSlider)), cpDataP->newRGB.g / 17);
		FrmShowObject(formP, FrmGetObjectIndex(formP, ColorPickerGreenSlider));
		StrIToH(textP, cpDataP->newRGB.g);
		FrmCopyLabel (formP, ColorPickerGreenValue, &textP[6]);
		FrmShowObject(formP, FrmGetObjectIndex(formP, ColorPickerGreenValue));

		FrmShowObject(formP, FrmGetObjectIndex(formP, ColorPickerBlueLabel));
		CtlSetValue(FrmGetObjectPtr(formP, FrmGetObjectIndex(formP, ColorPickerBlueSlider)), cpDataP->newRGB.b / 17);
		FrmShowObject(formP, FrmGetObjectIndex(formP, ColorPickerBlueSlider));
		StrIToH(textP, cpDataP->newRGB.b);
		FrmCopyLabel (formP, ColorPickerBlueValue, &textP[6]);
		FrmShowObject(formP, FrmGetObjectIndex(formP, ColorPickerBlueValue));

		HwrDisplayPalette(true, cpDataP->changingRGBIndex, 1, (HwrDisplayRGBPtr)(&(cpDataP->newRGB)));
		FrmShowObject(formP, FrmGetObjectIndex(formP, ColorPickerSampleColorGadget));
	}
}


/***********************************************************************
 *
 * FUNCTION:		PrvColorPickerHandleEvent
 *
 * DESCRIPTION:	Handle events for the color picker form.
 *
 * PARAMETERS:		eventP	->	event to handle		
 *
 * RETURNED:		true if a event handled
 *
 *	NOTES:			Updates the color index value in the gadget's
 *						pointer with the user's color choice
 *
 ***********************************************************************/
static Boolean PrvColorPickerHandleEvent(EventPtr eventP)
{
	Boolean					handled = false;
	FormPtr					formP = FrmGetActiveForm();
	ColorPickerDataType *cpDataP = FrmGetGadgetData (formP,
												FrmGetObjectIndex(formP, ColorPickerChitBoundsGadget));

	switch (eventP->eType) {
		case popSelectEvent:
			if (eventP->data.popSelect.listID == ColorPickerTypePopupList)
			{
				cpDataP->current = (UIPickColorStartType)eventP->data.popSelect.selection;
				PrvColorPickerUpdateControls(formP, cpDataP);
				handled = false;
			}
			break;
		
		case ctlRepeatEvent:
			if (eventP->data.ctlRepeat.controlID == ColorPickerRedSlider) {
				Char textP[16];
				cpDataP->newRGB.r = eventP->data.ctlRepeat.value * 17;
				cpDataP->newIndex = WinRGBToIndex(&cpDataP->newRGB);
				StrIToH(textP, cpDataP->newRGB.r);
				FrmCopyLabel (formP, ColorPickerRedValue, &textP[6]);
				HwrDisplayPalette(true, cpDataP->changingRGBIndex, 1, (HwrDisplayRGBPtr)(&(cpDataP->newRGB)));
			}
			else if (eventP->data.ctlRepeat.controlID == ColorPickerGreenSlider) {
				Char textP[16];
				cpDataP->newRGB.g = eventP->data.ctlRepeat.value * 17;
				cpDataP->newIndex = WinRGBToIndex(&cpDataP->newRGB);
				StrIToH(textP, cpDataP->newRGB.g);
				FrmCopyLabel (formP, ColorPickerGreenValue, &textP[6]);
				HwrDisplayPalette(true, cpDataP->changingRGBIndex, 1, (HwrDisplayRGBPtr)(&(cpDataP->newRGB)));
			}
			else if (eventP->data.ctlRepeat.controlID == ColorPickerBlueSlider) {
				Char textP[16];
				cpDataP->newRGB.b = eventP->data.ctlRepeat.value * 17;
				cpDataP->newIndex = WinRGBToIndex(&cpDataP->newRGB);
				StrIToH(textP, cpDataP->newRGB.b);
				FrmCopyLabel (formP, ColorPickerBlueValue, &textP[6]);
	
				HwrDisplayPalette(true, cpDataP->changingRGBIndex, 1, (HwrDisplayRGBPtr)(&(cpDataP->newRGB)));
			}

			break;
				
		case ctlSelectEvent:
			if (eventP->data.ctlSelect.controlID == ColorPickerRedSlider) {
				Char textP[16];
				cpDataP->newRGB.r = eventP->data.ctlSelect.value * 17;
				cpDataP->newIndex = WinRGBToIndex(&cpDataP->newRGB);
				StrIToH(textP, cpDataP->newRGB.r);
				FrmCopyLabel (formP, ColorPickerRedValue, &textP[6]);
				HwrDisplayPalette(true, cpDataP->changingRGBIndex, 1, (HwrDisplayRGBPtr)(&(cpDataP->newRGB)));
				handled = true;
			}
			else if (eventP->data.ctlSelect.controlID == ColorPickerGreenSlider) {
				Char textP[16];
				cpDataP->newRGB.g = eventP->data.ctlSelect.value * 17;
				cpDataP->newIndex = WinRGBToIndex(&cpDataP->newRGB);
				StrIToH(textP, cpDataP->newRGB.g);
				FrmCopyLabel (formP, ColorPickerGreenValue, &textP[6]);
				HwrDisplayPalette(true, cpDataP->changingRGBIndex, 1, (HwrDisplayRGBPtr)(&(cpDataP->newRGB)));
				handled = true;
			}
			else if (eventP->data.ctlSelect.controlID == ColorPickerBlueSlider) {
				Char textP[16];
				cpDataP->newRGB.b = eventP->data.ctlSelect.value * 17;
				cpDataP->newIndex = WinRGBToIndex(&cpDataP->newRGB);
				StrIToH(textP, cpDataP->newRGB.b);
				FrmCopyLabel (formP, ColorPickerBlueValue, &textP[6]);
				HwrDisplayPalette(true, cpDataP->changingRGBIndex, 1, (HwrDisplayRGBPtr)(&(cpDataP->newRGB)));
				handled = true;
			}

			break;

   	case keyDownEvent:
			if	(	(!TxtCharIsHardKey(	eventP->data.keyDown.modifiers,
												eventP->data.keyDown.chr))
				&&	(EvtKeydownIsVirtual(eventP))
				&&	(	(eventP->data.keyDown.chr == vchrPageUp)
					||	(eventP->data.keyDown.chr == vchrPageDown))) {
				if (eventP->data.keyDown.chr == vchrPageUp) {
					// have to pin decrementing here because newIndexP is unsigned
					if (cpDataP->newIndex > 0)
						cpDataP->newIndex -= 1;
				}
				else
					cpDataP->newIndex += 1;

				// call the gadget handler to draw the chit selection
				PrvChitGadgetHandler(
					FrmGetObjectPtr(formP, FrmGetObjectIndex(formP, ColorPickerChitBoundsGadget)),
					formGadgetDrawCmd, 0);
				handled = true;
   		}
			break;

		}

	return handled;
}


/***********************************************************************
 *
 * FUNCTION:		UIPickColor
 *
 * DESCRIPTION:	Put up UI to allow the user to pick a color from either
 *						the entire RGB space or just the current palette.
 *
 * PARAMETERS:		indexP	<->	initial index value of picker and also
 *						   				return value of user's choice, can be null	
 * 					rgbP		<->	initial value of color picker and also
 *						    				return value of user's choice, can be null	
 * 					start		->		start with RGB or palette picker, also controls
 *											which input value will be used
 *						titleP	->		optional string to use as a title for
 *											prompting the user.  NULL for default
 *
 * RETURNED:		true if a new color was picked, false if cancelled
 *
 * NOTES:			Must pass a value for one of indexP and rgbP, and can
 *						use both.  If only one is specified, it must match the
 *						start value, and the picker will be limited to only
 *						that UI.  If both are non-null, the user can switch
 *						back and forth between pickers, and the start value is
 *						used only to control which is brought up.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	08/13/99	Initial Revision
 *			bob	09/23/99	Remove WinModal check, let color picked open any time
 *
 ***********************************************************************/
Boolean UIPickColor(IndexedColorType * indexP, RGBColorType * rgbP,
					UIPickColorStartType start, const Char * titleP,
					const Char * /* tipP */)
{
	FormPtr					formP;
	FormActiveStateType	curFrmState;
	EventType				event;
	Boolean					result = false;
	ColorPickerDataType	cpData;
	UInt32 					savedDepth, newDepth;

	// save active form/window state
	FrmSaveActiveState(&curFrmState);
	
	// ...the slider form itself
	formP = FrmInitForm (ColorPickerForm);
	FrmSetActiveForm (formP);
	FrmSetEventHandler(formP, &PrvColorPickerHandleEvent);
	
	if ((cpData.useRGB = (rgbP != NULL)) != false) {
		cpData.current = UIPickColorStartRGB;
		cpData.newRGB = *rgbP;
	}
	
	if ((cpData.useIndex = (indexP != NULL)) != false) {
		cpData.current = UIPickColorStartPalette;
		cpData.newIndex = *indexP;
	}
	
	WinSetDrawWindow(FrmGetWindowHandle(formP));	// so WinIndexToRGB works with the right window

	WinScreenMode(winScreenModeGet, NULL, NULL, &savedDepth, NULL);
	if (savedDepth > 8) {
		newDepth = 8;
		WinScreenMode(winScreenModeSet, NULL, NULL, &newDepth, NULL);		
	}
	else
		newDepth = savedDepth;
	
	cpData.changingRGBIndex = (1<<newDepth)-2;	// 2nd to the last entry
	WinIndexToRGB(cpData.changingRGBIndex, &cpData.oldRGBValue);

	if (titleP)
		FrmSetTitle(formP, (Char *) titleP);
	
	FrmSetGadgetData(formP, FrmGetObjectIndex(formP, ColorPickerChitBoundsGadget),
		&cpData);
	FrmSetGadgetHandler(formP, FrmGetObjectIndex(formP, ColorPickerChitBoundsGadget),
		&PrvChitGadgetHandler);

	FrmSetGadgetData(formP, FrmGetObjectIndex(formP, ColorPickerSampleColorGadget),
		&cpData);
	FrmSetGadgetHandler(formP, FrmGetObjectIndex(formP, ColorPickerSampleColorGadget),
		&PrvSampleGadgetHandler);
	
	// only display the picker if both index and RGB are allowed
	if (cpData.useRGB && cpData.useIndex) {
		UInt16 triggerIndex = FrmGetObjectIndex(formP, ColorPickerTypeTrigger);
		ListType *listP = FrmGetObjectPtr(formP, FrmGetObjectIndex(formP, ColorPickerTypePopupList));

		ErrNonFatalDisplayIf(start < UIPickColorStartPalette || start > UIPickColorStartRGB,
									"UIPickColor start paramater invalid");

		cpData.current = start;
		FrmShowObject(formP, triggerIndex);
		CtlSetLabel(FrmGetObjectPtr(formP, triggerIndex), listP->itemsText[start]);
		LstSetSelection (listP, start);
	}
	
	// initialize the form controls and draw
	PrvColorPickerUpdateControls(formP, &cpData);
	FrmDrawForm(formP);

	// run the event loop for the form
	while (true) {
		EvtGetEvent (&event, evtWaitForever);

		if (! SysHandleEvent (&event))
			FrmDispatchEvent (&event);

		if (event.eType == ctlSelectEvent) {
			if (event.data.ctlSelect.controlID == ColorPickerOKButton) {

				if (cpData.useRGB)
					*rgbP = cpData.newRGB;
				if (cpData.useIndex)
					*indexP = cpData.newIndex;

				result = true;
				break;	// out of loop
			}

			else if (event.data.ctlSelect.controlID == ColorPickerCancelButton) {
				result = false;
				break; 	// out of loop
			}
		}

		// if we get an app switch command, abort the picker form and
		// let the calling app deal with the event!
		if (event.eType == appStopEvent)
		{
			EvtAddEventToQueue (&event);
			break;	// out of loop
		}
	}

	// Restore palette, erase dialog, deallocate, etc.
	HwrDisplayPalette(true, cpData.changingRGBIndex, 1, (HwrDisplayRGBPtr)(&cpData.oldRGBValue));
	if (savedDepth > 8) 
		WinScreenMode(winScreenModeSet, NULL, NULL, &savedDepth, NULL);		
	
	FrmEraseForm(formP);
	FrmDeleteForm(formP);

	// restore active form/window state
	FrmRestoreActiveState(&curFrmState);
	
	return result;
}
