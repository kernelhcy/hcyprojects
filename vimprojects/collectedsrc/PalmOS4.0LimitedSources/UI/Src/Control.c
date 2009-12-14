/******************************************************************************
 *
 * Copyright (c) 1994-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: Control.c
 *
 * Release: 
 *
 * Description:
 *	  This file contain control object routines.
 *
 * History:
 *		October 28, 1994	Created by Roger Flores
 *		2/9/99	bob	Fix up const stuff, add color support
 *
 *****************************************************************************/

#define NON_PORTABLE

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_CONTROLS
#define ALLOW_ACCESS_TO_INTERNALS_OF_FIELDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_BITMAPS
#define ALLOW_ACCESS_TO_INTERNALS_OF_CLIPBOARDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS

#include <SystemPublic.h>
#include "SysTrapsFastPrv.h"

#include "Control.h"
#include "Form.h"
#include "UIGlobals.h"
#include "UIResourcesPrv.h"

#define popupIndicatorGap				5
#define popupIndicatorWidth			7
#define popupIndicatorHeight			((popupIndicatorWidth+1)/2)
#define horizontalSpace					3

#define minButtonWidth					36

// ticks (1/60 s) between repeat ctlRepeatEvents
#define firstControlRepeatInterval	30
#define controlRepeatInterval			9

/***********************************************************************
 * Private Data Structures
 ***********************************************************************/

typedef struct {
	MemHandle		thumbH, backgroundH;
	BitmapType		*thumbP, *backgroundP;
	Int32				thumbX;
	Int16				newValue;
	WinHandle		offScreenWinH;
	Boolean			trackingPen;
	UInt8				reserved;
} ActiveSliderType;



/***********************************************************************
 * Functions
 ***********************************************************************/

// These routines WERE placed in the beginning to avoid declaring them.
// However, doing so now makes CtlHandleEvent unreachable from ROMInstallUI1.c!
// So, we get to declare them after all...  :-)
static FrameType ButtonFrameToFrameType (ControlType * ctlP);
static void PrvDrawOldGraphicControl(ControlType * ctlP, Boolean selected, RectangleType * rP, 
											  FrameBitsType * frameP);
static void PrvDrawCheckBoxControl(ControlType * ctlP, Boolean selected);
static void PrvDrawSliderControl(SliderControlType * sldP);
static void PrvDrawGraphicControl(GraphicControlType * ctlP, Boolean selected);
static void PrvDrawTextControl(ControlType * ctlP);
static void PrvDrawPopupIndicator (Coord x, Coord y);
static Boolean PrvSliderTrackPen (SliderControlType * sldP, EventType * eventP);




/***********************************************************************
 *
 * FUNCTION:	PrvDrawControl
 *
 * DESCRIPTION: Draw any control.
 *
 * PARAMETERS: 	ctlP		->	pointer to control object to draw
 *						selected	->	whether control is selected
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	10/28/94	Initial Revision
 *			bob	2/9/99	Turned InvertControl into PrvDrawControl
 *			bob	7/30/99	Fix graphic control hiliting
 *			grant 9/30/99	If the control's text contains just a single space,
 *								consider that empty and draw as an old graphic control.
 *			peter	01/25/00	Fix white/white display of B&W selected graphic buttons
 *
 ***********************************************************************/
static void PrvDrawControl (ControlType * ctlP, Boolean selected)
{
	RectangleType	clip, saveClip;
	FrameType 		frame;
	IndexedColorType	oldFore, oldBack;

	// no op for unusable controls
	if (!ctlP->attr.usable)
		return;

	WinPushDrawState();

	// Draw the frame around the bounds.  Do this now before
	// we restrict the clipping to the control's bounds.
	oldFore = WinSetForeColor(UIColorGetIndex(UIObjectFrame));
	
	if (ctlP->style == selectorTriggerCtl) {
		frame = simpleFrame;
		WinDrawGrayRectangleFrame (frame, &(ctlP->bounds));
	}
	else {
		if ((frame = ButtonFrameToFrameType(ctlP)) != noFrame)
			if (UIOptions.drawBordersAsLines)
				WinDrawRectangleFrame (frame, &(ctlP->bounds));
			// Draw the frame in the fill color so that buttons appear the same size
			else if (ctlP->style == buttonCtl) {
				if (selected)
					WinSetForeColor(UIColorGetIndex(UIObjectSelectedFill));
				else
					WinSetForeColor(UIColorGetIndex(UIObjectFill));
				WinDrawRectangleFrame (frame, &(ctlP->bounds));
			}
	}

	// Restrict the clipping to the control's bounds.
	WinGetClip (&saveClip);
	RctCopyRectangle (&(ctlP->bounds), &clip);
	WinClipRectangle (&clip);
	WinSetClip (&clip);

	// set up the colors
	// different colors if selected, but not for checkboxes or sliders
	if (selected && (ctlP->style != checkboxCtl) && (ctlP->style < sliderCtl)) {
		WinSetTextColor(UIColorGetIndex(UIObjectSelectedForeground));

		// Special case for graphic controls with no frames where
		// the developer provides 2 bitmaps -- don't change background
		// or foreground colors to reflect selection, just draw new bitmap.
		// This only affects 1 bit deep bitmaps.
		if (!ctlP->attr.graphical || frame != noFrame ||
			((GraphicControlType *)ctlP)->selectedBitmapID == 0)
		{
			oldBack = WinSetBackColor(UIColorGetIndex(UIObjectSelectedFill));
			WinSetForeColor(UIColorGetIndex(UIObjectSelectedForeground));
		}
	}
	// unselected, checkbox, and slider colors here
	else {
		WinSetTextColor(UIColorGetIndex(UIObjectForeground));
		WinSetForeColor(UIColorGetIndex(UIObjectForeground));
		
		// only change background color if control is framed,
		// otherwise, leave it to (presumably) window background color
		if (frame != noFrame)
			oldBack = WinSetBackColor(UIColorGetIndex(UIObjectFill));
	}
	
	// draw the appropriate control
	if (ctlP->style == checkboxCtl) {
		// checkboxes don't need to be erased, will redraw every pixel that counts
		PrvDrawCheckBoxControl(ctlP, selected);
	}
	else if (ctlP->style >= sliderCtl) {
		// erasing is done in slider draw fn for better animation
		PrvDrawSliderControl((SliderControlType *)ctlP);
	}
	else if (ctlP->attr.graphical) {
		WinEraseRectangle (&clip, ((FrameBitsType*)&frame)->bits.cornerDiam);
		PrvDrawGraphicControl((GraphicControlType *)ctlP, selected);
	}
	else if ((ctlP->style == popupTriggerCtl) ||
		ctlP->text && StrLen(ctlP->text) && !(ctlP->text[0] == ' ' && ctlP->text[1] == '\0')) {
		WinEraseRectangle (&clip, ((FrameBitsType*)&frame)->bits.cornerDiam);
		PrvDrawTextControl(ctlP);
	}
	else {
		// special case -- don't erase old controls with empty text, assuming
		// there was a graphic drawn behind it.
		// generally applied to buttons (repeating and non), and selector triggers.
		// not supported for popup triggers.
		PrvDrawOldGraphicControl(ctlP, selected, &clip, (FrameBitsType*)&frame);
	}
		
	// mark control as drawn
	ctlP->attr.visible = true;

	// restore drawing stuff
	WinSetClip (&saveClip);
	WinPopDrawState();
}


/***********************************************************************
 *
 * FUNCTION: 	 CtlDrawControl
 *
 * DESCRIPTION: Draw a control object on screen.
 *					 The control is drawn only if it is usable.
 *
 * PARAMETERS:  ctlP     - pointer to control object to draw
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	10/28/94	Initial Revision
 *
 ***********************************************************************/
void CtlDrawControl (ControlType * ctlP)
{
	PrvDrawControl(ctlP, ctlP->attr.on);
}


/***********************************************************************
 *
 * FUNCTION: CtlEraseControl
 *
 * DESCRIPTION: 
 *
 * PARAMETERS:	
 *
 * RETURNED:	
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	9/29/94		Initial Revision
 *
 ***********************************************************************/
void CtlEraseControl (ControlType * ctlP)
	{
	RectangleType	r;
	FrameBitsType  frame;


	if (ctlP->attr.usable && ctlP->attr.visible)
		{
		if (ctlP->style == checkboxCtl ||
			 (ctlP->attr.frame == noButtonFrame &&
			  ctlP->style != selectorTriggerCtl))
			{
			WinEraseRectangle (&ctlP->bounds, 0);
			}
		else
			{
			frame.word = ButtonFrameToFrameType(ctlP);
			WinGetFramesRectangle(frame.word, &ctlP->bounds, &r);
			WinEraseRectangle (&r, frame.bits.cornerDiam);
			}
		ctlP->attr.visible = false;
		ctlP->attr.drawnAsSelected = false;
		}
	}


/***********************************************************************
 *
 * FUNCTION:    CtlShowControl
 *
 * DESCRIPTION: Set a control usable and draw it.
 *
 * PARAMETERS:	 ctlP - pointer to control object
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	10/28/94	Initial Revision
 *
 ***********************************************************************/
void CtlShowControl (ControlType * ctlP)
	{
	ctlP->attr.usable = true;
	CtlDrawControl (ctlP);
	}


/***********************************************************************
 *
 * FUNCTION:    CtlHideControl
 *
 * DESCRIPTION: Set a control not usable and erase it.
 *
 * PARAMETERS:	 ctlP - pointer to control object
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	10/28/94	Initial Revision
 *
 ***********************************************************************/
void CtlHideControl (ControlType * ctlP)
	{
	CtlEraseControl (ctlP);
	ctlP->attr.usable = false;
	}



/***********************************************************************
 *
 * FUNCTION:    CtlSetUsable
 *
 * DESCRIPTION: Set a control usable or not usable
 *
 * PARAMETERS:	 ctlP - pointer to control object
 *              usable   - true to set usable, false to set not usable.
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	7/5/95	Initial Revision
 *
 ***********************************************************************/
void CtlSetUsable (ControlType * ctlP, Boolean usable)
	{
	ctlP->attr.usable = usable;
	}

/***********************************************************************
 *
 * FUNCTION:    CtlEnabled
 *
 * DESCRIPTION: REturns true if the control is enabled.  Disabled controls
 *              do not responed to the pen.
 *
 * PARAMETERS:	 ctlP - pointer to control object
 *
 * RETURNED:	 true if enable, false if not usable.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/7/95	Initial Revision
 *			trm	08/07/97	made non modified passed variables constant
 *
 ***********************************************************************/
Boolean CtlEnabled (const ControlType * ctlP)
	{
	return (ctlP->attr.enabled);
	}


/***********************************************************************
 *
 * FUNCTION:    CtlSetEnabled
 *
 * DESCRIPTION: Set a control enabled or disabled.  Disabled controls
 *              do not responed to the pen.
 *
 * PARAMETERS:	 ctlP - pointer to control object
 *              enable  - true to set enable, false to set not usable.
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	7/13/95	Initial Revision
 *			trm	08/07/97	made non modified passed variables constant
 *
 ***********************************************************************/
void CtlSetEnabled (ControlType * ctlP, Boolean enable)
	{
	ctlP->attr.enabled = enable;
	}


/***********************************************************************
 *
 * FUNCTION:    CtlGetValue
 *
 * DESCRIPTION: Return the current value of the specified control.
 *
 * PARAMETERS:  ctlP - pointer to control object
 *
 * RETURNED:	 The cuurent value of the control, 0=off, 1=on.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	10/28/94	Initial Revision
 *			trm	08/07/97	made non modified passed variables constant
 *
 ***********************************************************************/
Int16 CtlGetValue (const ControlType * ctlP)
{
	if (ctlP->style >= sliderCtl)
		return ((SliderControlType *)ctlP)->value;
	
   return ctlP->attr.on;
}


/***********************************************************************
 *
 * FUNCTION:    CtlSetValue
 *
 * DESCRIPTION: Set the current value of the specified control.
 *
 * PARAMETERS:	 ctlP - pointer to control object
 *              newValue  - 0=off, non-zero=on
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	10/28/94	Initial Revision
 *			trm	08/07/97	made non modified passed variables constant
 *			bob	2/9/99	use PrvDrawControl instead of InvertControl
 *
 ***********************************************************************/
void CtlSetValue (ControlType * ctlP, Int16 newValue)
{
	Int16 value;
	
	if (ctlP->style >= sliderCtl) {
		SliderControlType *sldP = (SliderControlType *)ctlP;

		if (newValue > sldP->maxValue)
			value = sldP->maxValue;
		else if (newValue < sldP->minValue)
			value = sldP->minValue;
		else
			value = newValue;
		
		if (value != sldP->value) {
			sldP->value = value;
			if (ctlP->attr.visible)
				PrvDrawControl(ctlP, false);
		}
	}

	else {
		value = (newValue != 0);

		if (ctlP->attr.visible && ctlP->attr.on != value)
			PrvDrawControl (ctlP, value != 0);

		ctlP->attr.on = value;
	}
	
}


/***********************************************************************
 *
 * FUNCTION:    CtlGetLabel
 *
 * DESCRIPTION: Return a string pointer to the control's text label.
 *
 * PARAMETERS:  ctlP - pointer to control object
 *
 * RETURNED:	 The cuurent label of the control.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	10/28/94	Initial Revision
 *			trm	08/07/97	made non modified passed variables constant
 *			bob	??/??/99	for more fun, made return value constant
 *
 ***********************************************************************/
const Char* CtlGetLabel (const ControlType * ctlP)
{
   return ctlP->text;
}


/***********************************************************************
 *
 * FUNCTION:    CtlSetLabel
 *
 * DESCRIPTION: Set the current label of the specified control.
 *
 * PARAMETERS:	 ctlP - pointer to control object
 *              newLabel - pointer to new text label
 *
 * RETURNED:	 nothing
 *
 * NOTE:        Any changes made to this routine that results in a 
 *              different calculation of the bounds of the control
 *              must also be made to the resource builder.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	10/28/94	Initial Revision
 *			trm	08/07/97	made non modified passed variables constant
 *
 ***********************************************************************/
void CtlSetLabel (ControlType * ctlP, const Char * newLabel)
	{
	Coord 	width;
	FontID	currFont;
	RectangleType	r;

#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	// Check for the all-too-often occurrence of calling
	// this function with a pointer that is not a control.
	// Often clients mis-use this to change text of a form label.
	FormType * formP;
	UInt16 objIndex;
	UInt16 maxObjects;
	
	formP = FrmGetActiveForm();
	if (formP)
		{
		// Can't use FrmGetObjectIndexFromPtr, because it displays
		// an ErrNonFatalDisplay message if control is not found,
		// which is way too irritating.  This is supposed to help
		// find real problems, not show phantom problems.
		maxObjects = FrmGetNumberOfObjects(formP);
		for (objIndex = 0; objIndex < maxObjects; objIndex++)
			{
			if (FrmGetObjectPtr(formP, objIndex) == (void*)ctlP)
				break;
			}
		if (objIndex < maxObjects)
			{
			if (FrmGetObjectType (formP, objIndex) != frmControlObj)
				{
				ErrNonFatalDisplay("Not a control object");
				return;
				}
			}
		}
#endif

	ctlP->text = (Char *)newLabel;

	if (ctlP->style == popupTriggerCtl ||
		 ctlP->style == selectorTriggerCtl ||
		 ctlP->style == checkboxCtl)
		{
		currFont = FntSetFont (ctlP->font);
		// Don't include the blank column in the last letter
		if (newLabel)
			width = FntCharsWidth(newLabel, StrLen(newLabel)) - 1;
		else
			width = 0;

		// If the control is a selector, round the width up to and odd
		// value so that the corners of the gray frame draw properly.
		if (ctlP->style == selectorTriggerCtl)
			width = (width + (2 * horizontalSpace -1)) | 1;
			
		if (ctlP->style == popupTriggerCtl)
			width += (2 * horizontalSpace) + popupIndicatorWidth + 
				popupIndicatorGap;

		if (ctlP->style == checkboxCtl)
			{
			FntSetFont (checkboxFont);
			width += horizontalSpace + FntCharWidth(symbolCheckboxOn) + 
				popupIndicatorGap;
			}

		if (ctlP->attr.leftAnchor)
			{
			// if the width shrinks record the area no longer
			// part of the control.
			RctCopyRectangle (&(ctlP->bounds), &r);
			r.topLeft.x += (width - horizontalSpace);
			r.extent.x -= (width - horizontalSpace);
			
			ctlP->bounds.extent.x = width;
			}
		else
			{
			// if the width shrinks record the area no longer
			// part of the control.
			RctCopyRectangle (&(ctlP->bounds), &r);
			// We also want to erase the popupIndicator and the
			// popupIndicatorGap incase the label is smaller
			r.extent.x -= (width - horizontalSpace - popupIndicatorWidth -
				popupIndicatorGap);

			ctlP->bounds.topLeft.x = ctlP->bounds.topLeft.x +
				ctlP->bounds.extent.x - width;
			ctlP->bounds.extent.x = width;
			}
		FntSetFont (currFont);
		}
	else
		{
		// For all other types of buttons set r
		// Unoptimized way to insure the old text of a push button is cleared.
		RctCopyRectangle (&(ctlP->bounds), &r);

		if (ctlP->attr.usable && ctlP->attr.visible)
			WinEraseRectangle (&r, 0);
		}
		


	if (ctlP->attr.usable && ctlP->attr.visible)
		{
		// erase the area no longer part of the control (because the
		// CtlDrawControl can't).
		if (r.extent.x > 0)
			{
			// If the control is a selector erase the frame as well
			if (ctlP->style == selectorTriggerCtl)
				{
				r.topLeft.x--;
				r.topLeft.y--;
				r.extent.x += 2;
				r.extent.y += 2;
				}
				
			WinEraseRectangle (&r, 0);
			}
		else if (r.extent.x < 0)
			{
			// If the selection trigger is growing, erase the old right line.
			// This doesn't get completely erased when drawing if the selection
			// trigger is significantly taller than the text it contains.
			if (ctlP->style == selectorTriggerCtl)
				{
				// We use the rectangle "r" that was computed as the "extra" portion
				// of the control to erase to figure out where the right line is.
				const Int16 rightLineX = r.topLeft.x + r.extent.x;
				const Int16 rightLineY1 = r.topLeft.y;
				const Int16 rightLineY2 = r.topLeft.y + r.extent.y - 1;

				WinEraseLine (rightLineX, rightLineY1, rightLineX, rightLineY2);
				}
			}
		CtlDrawControl (ctlP);
		}
	}

/***********************************************************************
 *
 * FUNCTION:    CtlSetGraphics
 *
 * DESCRIPTION: Set the current bitmaps of the specified control.
 *
 * PARAMETERS:	 ctlP - pointer to control object
 *              newBitmapID - pointer to new unselected graphic (or 0)
 *              newSelectedBitmapID - pointer to new selected graphic (or 0)
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	09/22/99	Initial Revision
 *
 ***********************************************************************/
void CtlSetGraphics (ControlType * ctlP, DmResID newBitmapID, DmResID newSelectedBitmapID)
{
	GraphicControlType *gctlP = (GraphicControlType *)ctlP;

	if (ctlP->attr.graphical == 0)
		{
		ErrNonFatalDisplay("Not a graphic control");
		return;
		}
		
	if (newBitmapID != 0)
		gctlP->bitmapID = newBitmapID;
	
	if (newSelectedBitmapID != 0)
		gctlP->selectedBitmapID = newSelectedBitmapID;

	if (ctlP->attr.visible)
		PrvDrawControl (ctlP, ctlP->attr.on);	// PrvDrawControl also erases
}


/***********************************************************************
 *
 * FUNCTION:    CtlSetSliderValues
 *
 * DESCRIPTION: Change slider specific values. 
 *
 * PARAMETERS:  ctlP - pointer to a slider control
 *					 minValueP - pointer to new minValue, or NULL for no change
 *					 maxValueP - pointer to new minValue, or NULL for no change
 *					 pageSizeP - pointer to new minValue, or NULL for no change
 *					 valueP - pointer to new minValue, or NULL for no change
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	09/22/99	Initial Revision
 *
 ***********************************************************************/
void CtlSetSliderValues(ControlType *ctlP, const UInt16 *minValueP, const UInt16 *maxValueP,
					const UInt16 *pageSizeP, const UInt16 *valueP)
{
	SliderControlType *sCtlP = (SliderControlType *)ctlP;
	
	if (ctlP->style < sliderCtl || ctlP->style > feedbackSliderCtl)
		{
		ErrNonFatalDisplay("Not a slider");
		return;
		}
	if (sCtlP->activeSliderP)
		{
		ErrNonFatalDisplay("Can't change active slider");
		return;
	}
	
	if (minValueP)
		sCtlP->minValue = *minValueP;
	if (maxValueP)
		sCtlP->maxValue = *maxValueP;
	if (pageSizeP)
		sCtlP->pageSize = *pageSizeP;
	if (valueP)
		sCtlP->value = *valueP;

	if (ctlP->attr.visible)
		PrvDrawControl(ctlP, false);
}


/***********************************************************************
 *
 * FUNCTION:    CtlGetSliderValues
 *
 * DESCRIPTION: Retrieve slider specific values. 
 *
 * PARAMETERS:  ctlP - pointer to a slider control
 *					 minValueP - pointer to result for minValue, or NULL
 *					 maxValueP - pointer to result for minValue, or NULL
 *					 pageSizeP - pointer to result for minValue, or NULL
 *					 valueP - pointer to result for minValue, or NULL
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	09/22/99	Initial Revision
 *
 ***********************************************************************/
void CtlGetSliderValues(const ControlType *ctlP, UInt16 *minValueP, UInt16 *maxValueP,
					UInt16 *pageSizeP, UInt16 *valueP)
{
	SliderControlType *sCtlP = (SliderControlType *)ctlP;
	
	if (ctlP->style < sliderCtl || ctlP->style > feedbackSliderCtl)
		{
		ErrNonFatalDisplay("Not a slider");
		return;
		}

	if (minValueP)
		*minValueP = sCtlP->minValue;
	if (maxValueP)
		*maxValueP = sCtlP->maxValue;
	if (pageSizeP)
		*pageSizeP = sCtlP->pageSize;
	if (valueP)
		*valueP = sCtlP->value;
}


/***********************************************************************
 *
 * FUNCTION:    CtlHitControl
 *
 * DESCRIPTION: This routine simulates the pressing of a control.
 *
 * PARAMETERS:  ctlP - pointer to control object
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/6/95	Initial Revision
 *			trm	08/07/97	made non modified passed variables constant
 *
 ***********************************************************************/
void CtlHitControl (const ControlType * ctlP)
{
	EventType event;
	
	MemSet (&event,sizeof (EventType), 0);
	
	event.eType = ctlSelectEvent;
	event.penDown = false;
	event.screenX = ctlP->bounds.topLeft.x + (ctlP->bounds.extent.x / 2);
	event.screenY = ctlP->bounds.topLeft.y + (ctlP->bounds.extent.y / 2);
	event.data.ctlSelect.controlID = ctlP->id;
	event.data.ctlSelect.pControl = (ControlType *)ctlP;
	event.data.ctlSelect.on = false;
	EvtAddEventToQueue (&event);
}


/***********************************************************************
 *
 * FUNCTION:    CtlValidatePointer
 *
 * DESCRIPTION: Validate a control pointer. 
 *
 * PARAMETERS:  ctlP  pointer of a control
 *
 * RETURNED:    true if the pointer is ok
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/19/97	Initial Revision
 *
 ***********************************************************************/
Boolean CtlValidatePointer (const ControlType * ctlP)
{
	ErrFatalDisplayIf(ctlP == NULL, "NULL control");
	
	if (MemPtrSize((void *)ctlP) < sizeof(ControlType))
		goto badControl;
	
	if (ctlP->attr.frame > rectangleButtonFrame)
		goto badControl;
		
	if (ctlP->style > repeatingButtonCtl)
		goto badControl;
	 
	return true;

badControl:
	ErrNonFatalDisplay("Bad Control");
	return false;
}


/***********************************************************************
 *
 * FUNCTION:    CtlNewControl
 *
 * DESCRIPTION: Create a control. 
 *
 * PARAMETERS:  formPP - pointer to a pointer to a form. Set if the form moves.
 *					 controlID - ID of the new control
 *					 style - style of the new control
 *					 textP - string for the new control.  Copied after the control.
 *					 x - x of the new control
 *					 y - y of the new control
 *					 width - width of the new control.  Zero means to calc a default.
 *					 height - height of the new control.  Zero means to calc a default.
 *					 font - font of the new control
 *					 group - group of the new control
 *					 leftAnchor - true if the control is anchored left.  Affects resizing.
 *
 * RETURNED:    0 if error else a pointer to the control
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/19/97	Initial Revision
 *			bob	10/04/99	bump up size if we're creating a new slider
 *			CS		10/10/00	Reduce default control height (to 12 for stdFont)
 *								so it matches default height used by Constructor.
 *			kwk	11/06/00	Set up frame attribute for repeating button controls
 *							(actually all styles besides buttons & push-buttons).
 *
 ***********************************************************************/
ControlType * CtlNewControl (void **formPP, UInt16 ID, ControlStyleType style, 
	const Char * textP, 
	Coord x, Coord y, Coord width, Coord height, 
	FontID font, UInt8 group, Boolean leftAnchor)
{
	UInt16 size;
	ControlType * controlP;
	Err error;
	FontID currFont;
	
	
	size = sizeof(ControlType);
	
	// if we're creating a new slider, make sure there's enough room for the
	// larger structure
	if (style >= sliderCtl)
		size = sizeof(SliderControlType);
		
	if (textP)
		size += StrLen(textP) + sizeOf7BitChar('\0');	// add space for null terminator.
	
	error = FrmAddSpaceForObject ((FormType  **) formPP, (MemPtr *) &controlP, frmControlObj, size);
	if (error)
		return 0;
	
	controlP->id = ID;
	controlP->style = style;
	switch (style)
		{
		case buttonCtl:
		case pushButtonCtl:
			controlP->attr.frame = standardButtonFrame;
		break;
		
		default:
			controlP->attr.frame = noButtonFrame;
		break;
		}
	controlP->bounds.topLeft.x = x;
	controlP->bounds.topLeft.y = y;
	
	// DOLATER kwk - are we relying on FrmAddSpaceForObject clearing out
	// the new object's chunk of memory? If not, then what about all of
	// the other attributes?
	controlP->attr.usable = true;
	controlP->attr.leftAnchor = leftAnchor;
	controlP->attr.enabled = true;
	controlP->font = font;
	controlP->group = group;
	
	if (width)
		controlP->bounds.extent.x = width;
	else if (textP)
		{
		// Size the control to fit the label.
		CtlSetLabel (controlP, textP);
		if (controlP->bounds.extent.x == 0)
			{
			controlP->bounds.extent.x = FntCharsWidth(textP, StrLen(textP)) - 1 + 
				2 * (horizontalSpace + 2);
			//Minimum size, don't use minButtonWidth
			//controlP->bounds.extent.x = max(controlP->bounds.extent.x, minButtonWidth);
			}
		}
	
	if (height)
		controlP->bounds.extent.y = height;
	else
		{
		currFont = FntSetFont (font);
		// Add 1 to the line height, since we've already got 1 pixel of space above
		// everything but accented characters, so we just need 1 pixel of space below
		// the descenders.
		controlP->bounds.extent.y = FntLineHeight() + 1;
		FntSetFont (currFont);
		}
	
	if (textP)
		{
		controlP->text = (Char *) (controlP + 1);
		StrCopy(controlP->text, textP);
		}
	
	return controlP;
}


/***********************************************************************
 *
 * FUNCTION:    CtlNewGraphicControl
 *
 * DESCRIPTION: Create a graphical control. 
 *
 * PARAMETERS:  formPP - pointer to a pointer to a form. Set if the form moves.
 *					 controlID - ID of the new control
 *					 style - style of the new control
 *					 bitmapID - image to display in the control.
 *					 selectedBitmapID - image to display when control is selected, can be NULL
 *					 x - x of the new control
 *					 y - y of the new control
 *					 width - width of the new control.  Zero means to calc a default.
 *					 height - height of the new control.  Zero means to calc a default.
 *					 group - group of the new control
 *					 leftAnchor - true if the control is anchored left.  Affects resizing.
 *
 * RETURNED:    0 if error else a pointer to the control
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	5/27/99	Initial Revision
 *			kwk	05/29/99	Pass stdFont vs. NULL to CtlNewControl.
 *
 ***********************************************************************/
GraphicControlType * CtlNewGraphicControl (void **formPP, UInt16 ID, 
   ControlStyleType style, DmResID bitmapID, DmResID selectedBitmapID, 
   Coord x, Coord y, Coord width, Coord height, 
   UInt8 group, Boolean leftAnchor)
{
	GraphicControlType *gControlP;
	
	gControlP = (GraphicControlType *)CtlNewControl(formPP, ID, style,
						 NULL, x, y, width, height, stdFont, group, leftAnchor);
	
	gControlP->attr.graphical = 1;
	gControlP->bitmapID = bitmapID;
	gControlP->selectedBitmapID = selectedBitmapID;
	
	return gControlP;
}


/***********************************************************************
 *
 * FUNCTION:    CtlNewSliderControl
 *
 * DESCRIPTION: Create a Slider control. 
 *
 * PARAMETERS:  formPP - pointer to a pointer to a form. Set if the form moves.
 *					 controlID - ID of the new control
 *					 style - sliderCtl or feedbackSliderCtl
 *					 thumbID - image for the thumb
 *					 backgroundID - image to display when control is selected, can be NULL
 *					 x - x of the new control
 *					 y - y of the new control
 *					 width - width of the new control.  Zero means to calc a default.
 *					 height - height of the new control.  Zero means to calc a default.
 *					 minValue - minimum value for slider
 *					 maxValue - maximum value for slider
 *					 pageSize - page size for slider
 *					 value - starting value for slider
 *
 * RETURNED:    0 if error else a pointer to the control
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	09/22/99	Initial Revision
 *
 ***********************************************************************/
SliderControlType * CtlNewSliderControl (void **formPP, UInt16 ID, 
   ControlStyleType style, DmResID thumbID, DmResID backgroundID, 
   Coord x, Coord y, Coord width, Coord height, UInt16 minValue, UInt16 maxValue,
   UInt16 pageSize, UInt16 value)
{
	SliderControlType *sControlP;
	
	if (style < sliderCtl || style > feedbackSliderCtl)
		{
		ErrNonFatalDisplay("Style not a slider");
		return 0;
		}
	
	sControlP = (SliderControlType *)CtlNewControl(formPP, ID, style,
						 NULL, x, y, width, height, stdFont, 0, true);
	
	sControlP->thumbID = thumbID;
	sControlP->backgroundID = backgroundID;
	sControlP->minValue = minValue;
	sControlP->maxValue = maxValue;
	sControlP->pageSize = pageSize;
	sControlP->value = value;
	sControlP->activeSliderP = NULL;
	
	return sControlP;
}



/***********************************************************************
 *
 * FUNCTION:    CtlHandleEvent
 *
 * DESCRIPTION: Handle event in the specified control.  This routine
 *              handles two type of events, penDownEvents and
 *              controlEnterEvent.
 *
 *              When this routine receives a penDownEvents it checks
 *              if the pen position is within the bounds of the
 *              control object, if it is a ctlEnterEvent is
 *              added to the event queue and the routine exits.
 *
 *              When this routine receives a ctlEnterEvent it
 *              checks that the control id in the event record match
 *              the id of the control specified, if they match
 *              this routine will track the pen until it comes up
 *              in the bounds in which case ctlSelectEvent is sent.
 *              If the pen exits the bounds a ctlExitEvent is sent.
 *
 *
 * PARAMETERS:	 ctlP - pointer to control object (ControlType)
 *              eventP    - pointer to an EventType structure.
 *
 * RETURNED:	TRUE if the event was handled or FALSE if it was not.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	10/28/94	Initial Revision
 *			trm	08/07/97	made non modified passed variables constant
 *			jmp	11/07/99	Make sure initialState is initialized!  For
 *								ctlRepeatEvents it was random.  Fixes bug #23348.
 *			CS		08/15/00	Initialize new event structure.
 *
 ***********************************************************************/
Boolean CtlHandleEvent (ControlType * ctlP, EventType * eventP)
{
	EventType		newEvent;
	Boolean			penDown;
	Boolean			hilighted;
	Coord				x, y;
	RectangleType	frameBounds;
	Boolean			initialState = ctlP->attr.on;

	if ( (! ctlP->attr.usable) ||
		  (! ctlP->attr.visible) ||
		  (! ctlP->attr.enabled) )
		return (false);

	WinGetFramesRectangle(ButtonFrameToFrameType(ctlP), &ctlP->bounds,
		&frameBounds);

	MemSet(&newEvent, sizeof(newEvent), 0);	// Initialize new event structure

	switch (eventP->eType)
		{
		case penDownEvent:
			if (RctPtInRectangle (eventP->screenX, eventP->screenY, &frameBounds))
				{
				newEvent = *eventP;
				newEvent.eType = ctlEnterEvent;
				newEvent.data.ctlEnter.controlID = ctlP->id;
				newEvent.data.ctlEnter.pControl = ctlP;
				newEvent.screenX = eventP->screenX;
				newEvent.screenY = eventP->screenY;
				EvtAddEventToQueue (&newEvent);
				return true;
				}
			break;

		case ctlEnterEvent:
				PrvDrawControl(ctlP, !ctlP->attr.on);
				// fall through to ctlRepeatEvent
				
		case ctlRepeatEvent:
			if (eventP->data.ctlEnter.controlID == ctlP->id)
				if (ctlP->style >= sliderCtl)
					return PrvSliderTrackPen((SliderControlType *)ctlP, eventP);
				else {
					hilighted	= true;
					penDown		= true;

					do
						{
						PenGetPoint (&x, &y, &penDown);
						if (RctPtInRectangle (x, y, &frameBounds))
							{
							if (penDown)
								{
								if (!hilighted)
									{
									// Draw the inverse of what's there as if it's activated.
									PrvDrawControl(ctlP, !initialState);
									hilighted = true;
									
									// If we were outside the bounds reset the 
									// repeat timer to right now
									eventP->data.ctlRepeat.time = TimGetTicks();
									}
								// If this is a repeating control and this is the first
								// press or a press after a long enough time interval
								// then send a ctlRepeatEvent
								if (ctlP->style == repeatingButtonCtl &&
									(eventP->eType == ctlEnterEvent ||
										((TimGetTicks() > eventP->data.ctlRepeat.time) &&
											((TimGetTicks() + controlRepeatInterval) >= eventP->data.ctlRepeat.time))))
									{
									newEvent.eType = ctlRepeatEvent;
									newEvent.penDown = penDown;
									newEvent.screenX = x;
									newEvent.screenY = y;
									newEvent.data.ctlRepeat.controlID = ctlP->id;
									newEvent.data.ctlRepeat.pControl = ctlP;
									// The first repeat time is longer than the rest
									// Operates like a repeating keyboard
									newEvent.data.ctlRepeat.time = TimGetTicks();
									if (eventP->eType == ctlEnterEvent)
										newEvent.data.ctlRepeat.time += 
											(firstControlRepeatInterval - controlRepeatInterval);
									EvtAddEventToQueue (&newEvent);
									
									SndPlaySystemSound(sndClick);

									return true;
									}
									
								}
							else /* pen is up */
								{
								if (ctlP->style == checkboxCtl ||
									 ctlP->style == pushButtonCtl)
									{
									// Didn't use CtlSetValue to avoid redraw.
									ctlP->attr.on = ~ctlP->attr.on;
									}
								else
									PrvDrawControl(ctlP, ctlP->attr.on);

								// repeatingButtonCtl never send a ctlSelectEvent
								// They do not send anything when the button is released
								if (ctlP->style != repeatingButtonCtl)
									{
									newEvent.eType = ctlSelectEvent;
									newEvent.penDown = penDown;
									newEvent.screenX = x;
									newEvent.screenY = y;
									newEvent.data.ctlSelect.controlID = ctlP->id;
									newEvent.data.ctlSelect.pControl = ctlP;
									newEvent.data.ctlSelect.on = ctlP->attr.on;
									EvtAddEventToQueue (&newEvent);
									
									SndPlaySystemSound(sndClick);
		
									return true;
									}
								}
							}
						else /* moved out of rectangle */
							{
							if (hilighted)
								{
								PrvDrawControl(ctlP, initialState);
								hilighted = false;
								}

							// The user did not select the control.
							if (!penDown)
								{
								newEvent.eType = ctlExitEvent;
								newEvent.penDown = penDown;
								newEvent.screenX = x;
								newEvent.screenY = y;
								newEvent.data.ctlExit.controlID = ctlP->id;
								newEvent.data.ctlExit.pControl = ctlP;
								EvtAddEventToQueue (&newEvent);
								}
							}
						} while (penDown);

				}
			break;
		}

	return false;

}


/***********************************************************************
 *
 * FUNCTION:    PrvSliderMakeActive
 *
 * DESCRIPTION: Allocate 'state' for an active slider control.
 *
 * PARAMETERS:	 sldP - pointer to slider object
 *
 * RETURNED:	Error codes from dependent routines (ignored).
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	8/9/99	Initial Revision
 *
 ***********************************************************************/
static Err PrvSliderMakeActive(SliderControlType * sldP)
{
	ActiveSliderType *asP;
	Err err;
	
	ErrNonFatalDisplayIf(sldP->activeSliderP, "Slider already active.");
	
	asP = MemPtrNew(sizeof(ActiveSliderType));
	if (!asP) {
		ErrNonFatalDisplay("Out of memory");
		return memErrNotEnoughSpace;
	}
	
	// get the graphics for the slider thumb
	if (sldP->thumbID)
		asP->thumbH = DmGetResource(bitmapRsc, sldP->thumbID);
	else
		asP->thumbH = DmGetResource(bitmapRsc, SliderDefaultThumbBitmap);
	asP->thumbP = MemHandleLock(asP->thumbH);

	// get the graphics for the slider background
	if (sldP->backgroundID)
		asP->backgroundH = DmGetResource(bitmapRsc, sldP->backgroundID);
	else
		asP->backgroundH = DmGetResource(bitmapRsc, SliderDefaultBackgroundBitmap);
	asP->backgroundP = MemHandleLock(asP->backgroundH);

	asP->offScreenWinH = WinCreateOffscreenWindow(sldP->bounds.extent.x,
								 sldP->bounds.extent.y, screenFormat, &err);
	if (err)
		return err;
	
	asP->trackingPen = false;
	asP->newValue = sldP->value;
	
	// compute the thumb's left edge, translate value to pixels
	asP->thumbX = (sldP->value - sldP->minValue);						// value offset from minValue
	asP->thumbX *= sldP->bounds.extent.x - asP->thumbP->width;		// times pixel range
	asP->thumbX += (sldP->maxValue - sldP->minValue) / 2;				// round
	asP->thumbX /= (sldP->maxValue - sldP->minValue);					// divided by overall value range

	ErrNonFatalDisplayIf(asP->thumbX + asP->thumbP->width > sldP->bounds.extent.x ||
		asP->thumbX < 0, "Thumb out of range, maybe because value out of range?");

	sldP->activeSliderP = asP;

	return 0;
}


/***********************************************************************
 *
 * FUNCTION:		PrvSliderMakeInactive
 *
 * DESCRIPTION:	Allocate 'state' for an active slider control.
 *
 * PARAMETERS:		sldP - pointer to slider object
 *
 * RETURNED:		Error codes from dependent routines (ignored).
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	8/9/99	Initial Revision
 *
 ***********************************************************************/
static Err PrvSliderMakeInactive(SliderControlType * sldP)
{
	ActiveSliderType *asP = (ActiveSliderType *)sldP->activeSliderP;
	Err err = 0;
	
	ErrNonFatalDisplayIf(!asP, "Slider already inactive.");
	
	// delete the off-screen window
	if (asP->offScreenWinH != NULL)
		WinDeleteWindow(asP->offScreenWinH, false);
	
	// free the background and thumb
	MemPtrUnlock(asP->backgroundP);
	MemPtrUnlock(asP->thumbP);
	DmReleaseResource(asP->backgroundH);
	DmReleaseResource(asP->thumbH);
	
	// delete the active slider struct itself
	MemPtrFree(asP);
	sldP->activeSliderP = NULL;
	
	return err;
}


/***********************************************************************
 *
 * FUNCTION:    PrvSliderTrackPen
 *
 * DESCRIPTION: Track pen in a slider control.
 *
 * PARAMETERS:	 sldP - pointer to slider object
 *              eventP    - pointer to an EventType structure.
 *
 * RETURNED:	TRUE if the event was handled or FALSE if it was not.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	8/4/99	Initial Revision
 *
 ***********************************************************************/
static Boolean PrvSliderTrackPen (SliderControlType * sldP, EventType * eventP)
{
	EventType newEvent = *eventP;
	ActiveSliderType *asP = (ActiveSliderType *)sldP->activeSliderP;
	Int32 newThumbX;
	Int16 oldValue;
	Coord penXinSlider;
	UInt32 lastTime = 0;
	
	// make the slider active
	if (!asP)
		PrvSliderMakeActive(sldP);
	asP = (ActiveSliderType *)sldP->activeSliderP;
	

	// initialize data
	if (eventP->eType == ctlEnterEvent) {
		asP->newValue = sldP->value;
		newEvent = *eventP;
	}
	else {
		asP->newValue = newEvent.data.ctlRepeat.value;
		// don't want to use pen data in ctlRepeatEvent because it may be stale which
		// could lead to an extra pass through the loop if the pen has since come up
		PenGetPoint (&newEvent.screenX, &newEvent.screenY, &newEvent.penDown);
	}

	while (newEvent.penDown) {
		// make sure pen is in slider vertically
		if (newEvent.screenY >= sldP->bounds.topLeft.y &&
			newEvent.screenY <= sldP->bounds.topLeft.y + sldP->bounds.extent.y)
		{
			penXinSlider = newEvent.screenX - sldP->bounds.topLeft.x;

			// if we're tracking the pen, or the pen is over the slider
			// snap the slider to the pen
			if (asP->trackingPen ||
				 (penXinSlider >= asP->thumbX &&
				  penXinSlider <= asP->thumbX + asP->thumbP->width))
			{
				if (!asP->trackingPen) {
					SndPlaySystemSound(sndClick);
					asP->trackingPen = true;
				}

				oldValue = asP->newValue;
	
				// find pen/thumb location
				newThumbX = penXinSlider - (asP->thumbP->width / 2);
				if (newThumbX < 0)
					newThumbX = 0;
				if (newThumbX > sldP->bounds.extent.x - asP->thumbP->width)
					newThumbX = sldP->bounds.extent.x - asP->thumbP->width;
								
				// thumb moved, so update value, redraw
				if (newThumbX != asP->thumbX) {
					asP->thumbX = newThumbX;
					
					// re-use newThumbX variable to compute slider value
					newThumbX *= (sldP->maxValue - sldP->minValue);
					newThumbX += (sldP->bounds.extent.x - asP->thumbP->width) / 2; // round
					newThumbX /= (sldP->bounds.extent.x - asP->thumbP->width);
					
					if (newThumbX != asP->newValue) {
						asP->newValue = newThumbX;
						
						PrvDrawControl((ControlType *)sldP, false);
						
						// if the programmer wants an event, give it to them
						if (sldP->style == feedbackSliderCtl) {
							newEvent.eType = ctlRepeatEvent;
							newEvent.data.ctlRepeat.time = TimGetTicks();
							newEvent.data.ctlRepeat.value = asP->newValue;
							EvtAddEventToQueue (&newEvent);
							return true;
						}
					}
				}
				
			}
			
			// otherwise, pen is not over thumb, so maybe it's in the background.
			else if (penXinSlider >= 0 && penXinSlider <= sldP->bounds.extent.x)
			{
				// if we're giving feedback, get lastTime from old event
				if (eventP->eType == ctlRepeatEvent && sldP->style == feedbackSliderCtl)
					lastTime = eventP->data.ctlRepeat.time;

				// first time through, or if enough time has passed
				if (eventP->eType == ctlEnterEvent || TimGetTicks() + controlRepeatInterval > lastTime)
				{
					// page value in proper direction
					if (penXinSlider < asP->thumbX)
						asP->newValue -= sldP->pageSize;
					else // (penXinSlider > asP->thumbX + asP->thumbP->width)
						asP->newValue += sldP->pageSize;
					
					// pin value
					if (asP->newValue > sldP->maxValue) {
						SndPlaySystemSound (sndWarning);
						asP->newValue = sldP->maxValue;
					}
					if (asP->newValue < sldP->minValue) {
						SndPlaySystemSound (sndWarning);
						asP->newValue = sldP->minValue;
					}
					
					// compute thumb position from value
					newThumbX = (asP->newValue - sldP->minValue);					// value offset from minValue
					newThumbX *= sldP->bounds.extent.x - asP->thumbP->width;		// times pixel range
					newThumbX += (sldP->maxValue - sldP->minValue) / 2;			// round
					newThumbX /= (sldP->maxValue - sldP->minValue);					// divided by overall value range

					ErrNonFatalDisplayIf(newThumbX + asP->thumbP->width > sldP->bounds.extent.x ||
						newThumbX < 0, "Thumb out of range, maybe because value out of range?");

					// if paging caused the thumb to 'jump over' the pen,
					// go straight to tracking the pen directly (instead of bouncing the
					// thumb from one side of the pen to the other)
					if ((newThumbX < penXinSlider && asP->thumbX > penXinSlider) ||
						 (newThumbX > penXinSlider && asP->thumbX < penXinSlider))
					{
						asP->trackingPen = true;
						continue;
					}
					
					asP->thumbX = newThumbX;

					// have to check lastTime too because this may not be a feedback control,
					// in which case we never leave this loop and eventP->eType is always ctlEnterEvent
					if (lastTime == 0 && eventP->eType == ctlEnterEvent)
						lastTime = TimGetTicks() + firstControlRepeatInterval - controlRepeatInterval;
					else
						lastTime = TimGetTicks();
					SndPlaySystemSound(sndClick);
					PrvDrawControl((ControlType *)sldP, false);

					// if the programmer wants a feedback event, give it to them
					if (sldP->style == feedbackSliderCtl) {
						newEvent.eType = ctlRepeatEvent;
						newEvent.data.ctlRepeat.time = lastTime;
						newEvent.data.ctlRepeat.value = asP->newValue;
						EvtAddEventToQueue (&newEvent);
						return true;
					}
				}
			}
			
			// pen outside of slider horizontally
			else {
			}				
		}
		
		// pen outside of slider vertically
		else {
			if (asP->newValue != sldP->value) {
				asP->newValue = sldP->value;

				PrvDrawControl((ControlType *)sldP, false);

				if (sldP->style == feedbackSliderCtl) {
					newEvent.eType = ctlRepeatEvent;
					newEvent.data.ctlRepeat.time = lastTime;
					newEvent.data.ctlRepeat.value = asP->newValue;
					EvtAddEventToQueue (&newEvent);
					return true;
				}
			}
		}
				
		PenGetPoint (&newEvent.screenX, &newEvent.screenY, &newEvent.penDown);
	}	// end of while pendown
	
	// enqueue final select event (even for feedback controls)
	if (newEvent.screenY >= sldP->bounds.topLeft.y &&
		newEvent.screenY <= sldP->bounds.topLeft.y + sldP->bounds.extent.y)
	{
		sldP->value = asP->newValue;
		newEvent.eType = ctlSelectEvent;
		newEvent.data.ctlSelect.value = asP->newValue;
		EvtAddEventToQueue (&newEvent);
	}
	
	// or generate ctlExitEvent if pen went up outside slider (cancel)
	else {
		newEvent.eType = ctlExitEvent;
		EvtAddEventToQueue (&newEvent);
	}

	// clean up
	PrvSliderMakeInactive(sldP);
	
	return true;
}


/***********************************************************************
 *
 * FUNCTION:    ButtonFrameToFrameType
 *
 * DESCRIPTION: Return a frame type.
 *					 Selector triggers are handled specially to return a
 *					 rectangleFrame since there isn't a gray frame type.
 *
 * PARAMETERS:  ctlP - pointer to control object with frame
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/1/94	Initial Revision
 *
 ***********************************************************************/
static FrameType ButtonFrameToFrameType (ControlType * ctlP)
	{
		switch (ctlP->style)
			{
			case buttonCtl:
			case repeatingButtonCtl:
				switch (ctlP->attr.frame)
					{
					case standardButtonFrame:
						return roundFrame;
					case boldButtonFrame:
						return boldRoundFrame;
					case rectangleButtonFrame:
						return simpleFrame;
					}
				return noFrame;
			case pushButtonCtl:
				return rectangleFrame;
			case selectorTriggerCtl:
				return rectangleFrame;
			}
		return noFrame;
	}


/***********************************************************************
 *
 * FUNCTION:		PrvDrawOldGraphicControl
 *
 * DESCRIPTION: 	Handle drawing of old-style graphic controls.
 *						Old-style graphic controls were NIL-text buttons drawn over the
 *						top of form bitmaps, which worked because the old code carefully
 *						inverted the region only when the button changed state.
 *						We don't carefully invert any more, so this routine attempts
 *						to invert the region only on the proper transitions.
 *
 * PARAMETERS: 	ctlP		->	pointer to control object to draw
 *						selected	->	whether control is selected
 *						rP			->	rectangle defining control's bounds
 *						frameP	-> control's frame definition
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	4/16/99	Split out from PrvDrawControl
 *			bob	10/05/99	Switch to just using (ugly) WinInvert for all cases
 *
 ***********************************************************************/
static void PrvDrawOldGraphicControl(ControlType * ctlP, Boolean selected, RectangleType * rP, 
											  FrameBitsType * frameP)
{
	// The first time these buttons are drawn, invert if necessary
	if (!ctlP->attr.visible) {

		// draw the popup trigger, if any
		if (ctlP->style == popupTriggerCtl)
			PrvDrawPopupIndicator (rP->topLeft.x + horizontalSpace, 
					rP->topLeft.y + (rP->extent.y - popupIndicatorHeight + 1) / 2);

		ctlP->attr.drawnAsSelected = selected;
		if (selected)
			WinInvertRectangle(rP, frameP->bits.cornerDiam);
		
	}

	// And any subsequent time these buttons are drawn, we don't carefully invert
	// any more, so keep track of the last drawn state and only invert when
	// the control's drawn state has changed.
	else if (selected != ctlP->attr.drawnAsSelected) {
	
		ctlP->attr.drawnAsSelected = selected;
		WinInvertRectangle(rP, frameP->bits.cornerDiam);

	}
}
		

/***********************************************************************
 *
 * FUNCTION:		PrvDrawCheckBoxControl
 *
 * DESCRIPTION: 	Draw a check box control.
 *
 * PARAMETERS: 	ctlP		->	pointer to control object to draw
 *						selected	->	whether control is selected
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	4/16/99	Split out from PrvDrawControl
 *
 ***********************************************************************/
static void PrvDrawCheckBoxControl(ControlType * ctlP, Boolean selected)
{
	Coord				drawX, drawY;
	Char 				ch;
	RectangleType *	rP = &(ctlP->bounds);
	
	// decide which char to use.
	if (selected)
		ch = symbolCheckboxOn;
	else
		ch = symbolCheckboxOff;

	// The checkbox is drawn first.  Then drawX is incremented to
	// where the text is to be drawn
	FntSetFont (checkboxFont);
	drawX = rP->topLeft.x;
	drawY = rP->topLeft.y + ((rP->extent.y - FntCharHeight ()) / 2);
	WinDrawChar(ch, drawX, drawY);

	// Draw the control's text at (drawX, drawY)
	if (ctlP->text) {
		drawX += FntCharWidth(ch) + popupIndicatorGap;
		FntSetFont (ctlP->font);
		drawY = rP->topLeft.y + ((rP->extent.y - FntBaseLine() + 
			FntDescenderHeight()) / 2) - FntDescenderHeight();
		WinDrawChars(ctlP->text, StrLen (ctlP->text), drawX, drawY);
	}
}


/***********************************************************************
 *
 * FUNCTION:		PrvDrawSliderControl
 *
 * DESCRIPTION: 	Draw a slider control.
 *
 * PARAMETERS: 	sldP		->	pointer to slider object to draw
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	8/3/99	Created from PrvDrawGraphicControl
 *
 ***********************************************************************/
static void PrvDrawSliderControl(SliderControlType * sldP)
{
	ActiveSliderType *asP;
	RectangleType boundsR = sldP->bounds;
	Boolean madeActive = false;
	WinHandle saveWindowH;

	if (sldP->attr.vertical)
		{
		ErrNonFatalDisplay("Vertical sliders not supported.  Yet.");
		return;
		}
	
	// if the slider is not already active, turn it on
	if (!sldP->activeSliderP) {
		madeActive = true;
		PrvSliderMakeActive(sldP);
	}
	
	// get the slider bitmaps and stuff
	asP = (ActiveSliderType *)sldP->activeSliderP;
	
	// if we have an off-screen window, use it as a temporary buffer
	if (asP->offScreenWinH != NULL) {
		saveWindowH = WinGetDrawWindow();
		WinSetDrawWindow(asP->offScreenWinH);
		boundsR.topLeft.x = 0;
		boundsR.topLeft.y = 0;
	}
	
	// draw the background
	WinEraseRectangle (&boundsR, 0);

	// if background is the wrong width, draw it in two halves
	if (boundsR.extent.x != asP->backgroundP->width) {
		RectangleType clipR;
		RectangleType saveClipR;
		
		WinGetClip(&saveClipR);
		
		ErrNonFatalDisplayIf(asP->backgroundP->width > boundsR.extent.x / 2,
			"Background must be at least half as wide as slider.");

		// draw left half
		clipR = boundsR;
		clipR.extent.x /= 2;
		RctGetIntersection(&clipR, &saveClipR, &clipR);
		WinSetClip (&clipR);
		WinDrawBitmap(asP->backgroundP, boundsR.topLeft.x,
			boundsR.topLeft.y + ((boundsR.extent.y - asP->backgroundP->height) / 2));
			
		// draw the right half
		clipR = boundsR;
		clipR.topLeft.x += (clipR.extent.x / 2);
		clipR.extent.x -= (clipR.extent.x / 2);
		WinSetClip (&clipR);
		WinDrawBitmap(asP->backgroundP, boundsR.topLeft.x + boundsR.extent.x - asP->backgroundP->width,
			boundsR.topLeft.y + ((boundsR.extent.y - asP->backgroundP->height) / 2));

		WinSetClip(&saveClipR);
	}

	// if background is same width, just draw it
	else
		WinDrawBitmap(asP->backgroundP, boundsR.topLeft.x,
			boundsR.topLeft.y + ((boundsR.extent.y - asP->backgroundP->height) / 2));
	
	// compute the thumb's left edge, translate value to pixels
	asP->thumbX = (asP->newValue - sldP->minValue);						// value offset from minValue
	asP->thumbX *= sldP->bounds.extent.x - asP->thumbP->width;		// times pixel range
	asP->thumbX += (sldP->maxValue - sldP->minValue) / 2;				// round
	asP->thumbX /= (sldP->maxValue - sldP->minValue);					// divided by overall value range

	ErrNonFatalDisplayIf(asP->thumbX + asP->thumbP->width > sldP->bounds.extent.x ||
		asP->thumbX < 0, "Thumb out of range, maybe because value out of range?");

	// draw the thumb
	ErrNonFatalDisplayIf(asP->thumbP->width > boundsR.extent.x, "Thumb too wide for slider!");
	WinDrawBitmap(asP->thumbP, boundsR.topLeft.x + asP->thumbX,
		boundsR.topLeft.y + ((boundsR.extent.y - asP->thumbP->height) / 2));


	// if we drew off-screen, copy the bits on-screen.
	if (asP->offScreenWinH != NULL) {
		WinSetDrawWindow(saveWindowH);
		WinCopyRectangle(asP->offScreenWinH, 0, &WinGetWindowPointer(asP->offScreenWinH)->windowBounds,
			 sldP->bounds.topLeft.x, sldP->bounds.topLeft.y, winPaint);
	}

	// clean up
	if (madeActive)
		PrvSliderMakeInactive(sldP);
}


/***********************************************************************
 *
 * FUNCTION:		PrvDrawGraphicControl
 *
 * DESCRIPTION: 	Draw a graphical control.
 *
 * PARAMETERS: 	ctlP		->	pointer to control object to draw
 *						selected	->	whether control is selected
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	4/16/99	Split out from PrvDrawControl
 *
 ***********************************************************************/
static void PrvDrawGraphicControl(GraphicControlType * ctlP, Boolean selected)
{
	MemHandle		resH;
	BitmapType *	bitmapP;
	Coord			drawX, drawY;
	RectangleType *	rP = &(ctlP->bounds);
	
	// if no 'selected' graphic is provided, re-use the normal one
	if (selected && ctlP->selectedBitmapID != 0)
		resH = DmGetResource(bitmapRsc, ctlP->selectedBitmapID);
	else
		resH = DmGetResource(bitmapRsc, ctlP->bitmapID);
	if (resH == NULL)
		{
		ErrNonFatalDisplay("Can't find bitmap resource");
		return;
		}
	
	bitmapP = MemHandleLock(resH);
	ErrNonFatalDisplayIf(bitmapP == NULL, "Couldn't lock resource?!");
	
	drawY = rP->topLeft.y + ((rP->extent.y - bitmapP->height + 1) / 2);

	switch (ctlP->style) {
	
		case buttonCtl:
		case repeatingButtonCtl:
		case pushButtonCtl:
			// center graphic horizontally
			drawX = rP->topLeft.x + ((rP->extent.x - bitmapP->width + 1) / 2);
			break;
			
		case selectorTriggerCtl:
		case popupTriggerCtl:
			// graphic is at left
			drawX = rP->topLeft.x + horizontalSpace;
			break;

		default:
			ErrNonFatalDisplay("unsupported control");
		}
	
	WinDrawBitmap(bitmapP, drawX, drawY);

	MemPtrUnlock(bitmapP);
	DmReleaseResource(resH);
}


/***********************************************************************
 *
 * FUNCTION:		PrvDrawTextControl
 *
 * DESCRIPTION: 	Draw a text control.
 *
 * PARAMETERS: 	ctlP		->	pointer to control object to draw
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	4/16/99	Split out from PrvDrawControl
 *
 ***********************************************************************/
static void PrvDrawTextControl(ControlType * ctlP)
{
	Coord 			drawX, drawY;
	Int16				textLen;
	RectangleType *	rP = &(ctlP->bounds);

	// Set to the appropriate font, erase control if necessary
	FntSetFont (ctlP->font);

	if (ctlP->text)
		textLen = StrLen (ctlP->text);
	else
		textLen = 0;
		
	// Calculate where to draw the text horizontally.
	switch (ctlP->style) {
	
		case buttonCtl:
		case repeatingButtonCtl:
		case pushButtonCtl:
			// center text string horizontally
			drawX = rP->topLeft.x + ((rP->extent.x -
				FntCharsWidth (ctlP->text, textLen) + 1) / 2);
			
			// If it's a symbol, draw it using the fill color.
			if (!UIOptions.drawBordersAsLines && 
				symbolFont <= ctlP->font && ctlP->font <= symbol7Font)
				WinSetTextColor(UIColorGetIndex(UIObjectFill));
			break;
			
		case selectorTriggerCtl:
		case popupTriggerCtl:
			drawX = rP->topLeft.x + horizontalSpace;
			break;

		default:
			ErrNonFatalDisplay("unsupported control");
		}

	// draw the popup trigger
	if (ctlP->style == popupTriggerCtl)
		{
		PrvDrawPopupIndicator (drawX, 
				rP->topLeft.y + (rP->extent.y - popupIndicatorHeight + 1) / 2);
		drawX += popupIndicatorWidth + popupIndicatorGap;
		}
	
	// Draw the control's text at (drawX, drawY)
	drawY = rP->topLeft.y + ((rP->extent.y - FntBaseLine() + 
		FntDescenderHeight()) / 2) - FntDescenderHeight();
	WinDrawChars (ctlP->text, textLen, drawX, drawY);
}


/***********************************************************************
 *
 * FUNCTION:    PrvDrawPopupIndicator
 *
 * DESCRIPTION: Draw the selector trigger indicator.
 *					 The indicator normally indicates the presence of a
 *					 popup list.
 *
 * PARAMETERS:  x, y - top left corner of the indicator
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	10/28/94	Initial Revision
 *
 ***********************************************************************/

static void PrvDrawPopupIndicator (Coord x, Coord y)
{
	Coord i;

	for (i = popupIndicatorWidth; i > 0; i -= 2) {
		WinDrawLine (x, y, x+i, y);
		x++;
		y++;
	}
}
