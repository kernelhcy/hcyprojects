/******************************************************************************
 *
 * Copyright (c) 1994-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: Form.c
 *
 * Release: 
 *
 * Description:
 *	  This file contain the form object routines.
 *
 * History:
 *		November 7, 1994	Created by Art Lamb
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	2/5/99	Added UI color enablement
 *
 *****************************************************************************/

#define NON_PORTABLE

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_CONTROLS
#define ALLOW_ACCESS_TO_INTERNALS_OF_FIELDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_FORMS
#define ALLOW_ACCESS_TO_INTERNALS_OF_LISTS
#define ALLOW_ACCESS_TO_INTERNALS_OF_MENUS
#define ALLOW_ACCESS_TO_INTERNALS_OF_SCROLLBARS
#define ALLOW_ACCESS_TO_INTERNALS_OF_TABLES
#define ALLOW_ACCESS_TO_INTERNALS_OF_BITMAPS
#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS
#define ALLOW_ACCESS_TO_INTERNALS_OF_CLIPBOARDS

#include <PalmTypes.h>
#include <SystemPublic.h>

#include <PalmUtils.h>

#include "Hardware.h"

#include "SysTrapsFastPrv.h"
#include "SystemPrv.h"
#include "TextServicesPrv.h"

#include "Category.h"
#include "Form.h"
#include "GraffitiReference.h"
#include "GraffitiShift.h"
#include "Keyboard.h"
#include "List.h"
#include "Menu.h"
#include "MenuPrv.h"
#include "Table.h"
#include "UIResourcesPrv.h"
#include "AttentionPrv.h"

#include "UIGlobals.h"

#define fmrCornerDiam 				0

#define titleFont						boldFont
#define titleHeight 					15
#define titleMarginX 				3	// Don't forget the blank column after the last char
#define titleMarginY 				2

#define helpButtonWidth				10
#define helpCornerRadius			5

#define maxCategoryLabelWidth		80

#define frmMinFreeBytes		4000	// The bits obscured by a form will not be
											// saved if the free memory is below this
											// limit.

#define prvActiveStateSignature	0x55	// signature used for PrvActiveStateType


// PrvActiveStateType: Internal structure used by FrmActiveState();
// this structure abstracts the saved state data from the caller as this information
// will likely need to change in the future.  Callers use the structure
// FormActiveStateType instead.
//
typedef struct PrvActiveStateType {
	UInt8			sig;					// set to prvActiveStateSignature when state is being saved;
											// this is used by EC code to detect if restore has been
											// envoked without saving first (this field can be reused
											// for other purposes if needed)

	Boolean		insPtState;			// enabled state of insertion point
	FormType *	curForm;				// current active form
	PointType	insPtPos;			// position of insertion point
	WinHandle	savedDrawWin;		// draw window handle
	WinHandle	savedActiveWin;	// active window handle
	Boolean		attnIndicatorState;// state of attention manager's indicator
	UInt8			reserved[3];		// reserved for future extensions
	} PrvActiveStateType;
	
// Macros stolen from SystemMgr.c.  This is an unfortunate link until forms can be
// changed to store their own reference to an environment.
#define PrvIsSegmentRegisterProtected(r)	(((UInt32) (r) & 0x80000000) != 0)
#define PrvProtectSegmentRegister(r)	((MemPtr) ((UInt32) (r) | 0x80000000))
#define PrvUnprotectSegmentRegister(r)	((MemPtr) ((UInt32) (r) & ~0x80000000))
	

// This declaration is used for performing link-time assertions
static void ThisShouldGenerateALinkErrorForm(void);


static FormType * PrvConstructCustomAlert (UInt16 alertId, const Char * s1, const Char * s2, 
	const Char * s3, Int16 textEntryFieldChars, UInt16 * alertTypeP);

/***********************************************************************
 *
 * FUNCTION: 	 FrmGetWindowHandle
 *
 * DESCRIPTION: This routine returns the window handle of a form.
 *
 * PARAMETERS:	 formP	memory block that contains the form.
 *					
 * RETURNED:	 The segment of the memory block that the form is in,  since 
 *              the form structure begins with the WindowType structure this
 *              is also a WinHandle.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/7/94	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
WinHandle FrmGetWindowHandle (const FormType * formP)
{
	return (WinGetWindowHandle (formP));
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmInitForm
 *
 * DESCRIPTION: This routine loads and initializes a form resource.
 *
 * PARAMETERS:	 rscID	the resource id of the form.
 *					
 * RETURNED:	 The handle of the memory block that the form is in,  since 
 *              the form structure begins with the WindowType structure this
 *              is also a WinHandle.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/7/94	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
FormType * FrmInitForm (UInt16 rscID)
{
	FormType * formP;
	MemPtr currentEnvironmentP;


#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	// Check to see if this form is already loaded.  This is a convience check
	// to warn the programmer of trouble.  There are cases though when sub-launched
	// apps display ui with the same form ID as the main app.  To avoid false errors
	// for these cases, don't check when the app is a sublaunched app.
	// Bob: add check for HelpForm (tips dialog) as well, allow more than one tips to open at same time
	if (rscID != HelpForm) {
	SysAppInfoPtr appInfoP;
	SysAppInfoPtr unusedAppInfoP;
	
	unusedAppInfoP = SysGetAppInfo(&unusedAppInfoP, &appInfoP);
	if (appInfoP == NULL)
		{
		formP = FrmGetFormPtr (rscID);
		ErrNonFatalDisplayIf ((formP), "Form already loaded");
		}
	}
#endif	

	formP = ResLoadForm (rscID);

	WinInitializeWindow (FrmGetWindowHandle(formP));

	WinAddWindow (FrmGetWindowHandle(formP));
	
	formP->window.windowFlags.dialog = true;
	
	formP->focus = noFocus;
	
	// Determine if globals should be available for this form.  We do this for the 
	// event handling function to make sure it can access global variables.  We can't do
	// it in FrmSetEventHandler because not all apps do so, and some of those apps use 
	// callbacks like in tables which can get called when RedrawForm calls FrmDrawForm as 
	// the default handler.
	currentEnvironmentP = (MemPtr) SysSetA5(0);
	SysSetA5((UInt32) currentEnvironmentP);
	formP->attr.globalsAvailable = !PrvIsSegmentRegisterProtected(currentEnvironmentP);
	
	formP->attr.attnIndicator = false;	// default until form title is drawn
	
	ErrNonFatalDisplayIf(formP->attr.doingDialog || formP->attr.exitDialog,
		"forms created in wrong state!");
	return(formP);
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmDeleteForm
 *
 * DESCRIPTION: This routine release the memory occupied by a form.  Any
 *              memory allocated by objects in the form is also released.
 *
 * PARAMETERS:	 formP -> pointer to form being deleted
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/7/94	Initial Revision
 *			jaq	10/28/99	send formGadgetDeleteCmd to gadgets to cleanup
 *			tim w	11/05/99	only send formGadgetDeleteCmd if extended!
 *
 ***********************************************************************/
void FrmDeleteForm (FormType * formP)
{
	UInt16 i;
	MenuBarType * menuP;
	FormGadgetHandlerType *handler;
	
	ECFrmValidatePtr(formP);
	
	ErrNonFatalDisplayIf(formP->attr.visible, "Call FrmEraseForm first");
	
	if (formP == CurrentForm) 
		{
		CurrentForm = NULL;
		
		menuP = MenuGetActiveMenu ();
		if (menuP)
			{
			MenuDispose (menuP);
			MenuSetActiveMenu (NULL);
			}
		}
	
	// don't free memory if FrmDoDialog is still using this form!
	// set closeMe instead, FrmDoDialog will exit and presumably
	// caller will clean up
	if (formP->attr.doingDialog)
		{
		formP->attr.exitDialog = true;
		return;
		}

	// Release any memory that objects may have allocated.
	for (i = 0; i < formP->numObjects; i++)
		{
		switch (formP->objects[i].objectType)
			{
			case frmFieldObj:
				FldFreeMemory (formP->objects[i].object.field);
				break;
			
			case frmGadgetObj:
				if (formP->objects[i].object.gadget->attr.extended)
					{
					handler = formP->objects[i].object.gadget->handler;
					
					if (handler != 0)
						(handler)(formP->objects[i].object.gadget, formGadgetDeleteCmd, 0); 
					}
				break;
			}
		}
	
	WinDeleteWindow (FrmGetWindowHandle(formP), false);
}


/***********************************************************************
 *
 * FUNCTION:    PrvSendEventToForm
 *
 * DESCRIPTION: This routine sends an event to the specified form.
 *
 * PARAMETERS:  formP - form to send an event to
 *              event - event to send
 *
 * RETURNED:    true if 
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	10/11/95	Initial Revision
 *			jmp	09/15/99 As part of the fix for bug #21922, added the
 *								validate_and_setForm param so that this routine
 *								can be now called from FrmDispatchEvent.
 *			roger	9/21/99	Moved code to avoid extra paramter added.
 *
 ***********************************************************************/
static Boolean PrvSendEventToForm (FormType * formP, EventType * event)
{
	Boolean handled = false;
	
	
	ECFrmValidatePtr(formP);

	// Call the form's event handler if it has one.
	if (formP->handler)
		handled = formP->handler (event);

	// If the form's event handler didn't handle the event, or
	// there was no form event handler, handle it ourselves if
	// we can.
	if (!handled)
		handled = FrmHandleEvent (formP, event);
		
	return ( handled );
}


/***********************************************************************
 *
 * FUNCTION: 	 PrvDrawHelpIcon
 *
 * DESCRIPTION: This routine draws the 'i' button in a form.
 *					 Assumes the colors are set up for title bar drawing.
 *
 * PARAMETERS:	 windowWidth	->	width of current form.
 *              selected 		->	true to draw 'inverted'
 *					
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	10/05/99	Initial Revision
 *
 ***********************************************************************/
static void PrvDrawHelpIcon(Coord windowWidth, Boolean selected)
{
	Char ch = symbolHelp;
	RectangleType r;
	Coord x;
	
	r.topLeft.x = windowWidth - helpButtonWidth - 2;
	r.topLeft.y = 0;
	r.extent.y = helpButtonWidth;
	r.extent.x = helpButtonWidth;

	FntSetFont (symbolFont);
	x = r.topLeft.x + ((r.extent.x - FntCharWidth (ch)) /2);

	if (selected) {
		WinDrawRectangle(&r, helpCornerRadius);
		WinDrawInvertedChars (&ch, 1, x, 0);
	}
	else {
		WinEraseRectangle (&r, helpCornerRadius);
		WinDrawChars (&ch, 1, x, 0);
	}
}

// definitions for PrvSetAppropriateColors
#define setText 1
#define setFore 2
#define setBack 4

/***********************************************************************
 *
 * FUNCTION: 	 PrvSetAppropriateColors
 *
 * DESCRIPTION: Sets the right Foreground, Background, and/or Text colors
 *              based on the form's characteristics.
 *
 * PARAMETERS:	 formP - the form in question
 *              whichColors - bitfield saying which colors to set.  Create
 *                            this by adding setText, setFore and/or setBack
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			daf	11/2/99	Initial Revision, consolidating code from multiple places
 *
 ***********************************************************************/
static void PrvSetAppropriateColors(FormType * formP, UInt8 whichColors)
{
	UIColorTableEntries textColor, foreColor, backColor;
	
	if (formP->formId == CustomAlertDialog) {
		textColor = UIAlertFrame;
		foreColor = UIAlertFrame;
		backColor = UIAlertFill;
	}
	else if (formP->window.windowFlags.modal) {
		textColor = UIDialogFrame;
		foreColor = UIDialogFrame;
		backColor = UIDialogFill;
	}
	else {
		textColor = UIFormFrame;
		foreColor = UIFormFrame;
		backColor = UIFormFill;
	}

	if (whichColors & setText)
		WinSetTextColor(UIColorGetIndex(textColor));
	if (whichColors & setFore)
		WinSetForeColor(UIColorGetIndex(foreColor));
	if (whichColors & setBack)
		WinSetBackColor(UIColorGetIndex(backColor));
}

/***********************************************************************
 *
 * FUNCTION: 	 PrvDrawTitle
 *
 * DESCRIPTION: This routine draws the title of a form.
 *
 * PARAMETERS:	 frmHandle	the handle of memory block the contains the form.
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/7/94	Initial Revision
 *			jmp	10/27/99	Call FntSetFont() just after pushing window draw
 *								state; previous change caused centering to be
 *								slightly off as textLen was wrong.
 *
 ***********************************************************************/
static void PrvDrawTitle (FormType * formP, FormTitleType * title)
{
	Char * text;
	Int16 textLen;
	Coord x;
	Int16 windowWidth;
	Int16 windowHeight;	
	Int16 titleWidth;
	RectangleType r;
	Boolean allowIndicator;
	
	// Before drawing the new title, make sure the indicator is turned off.
	AttnIndicatorAllow(false);
	
	WinPushDrawState();
	FntSetFont (titleFont);
	
	PrvSetAppropriateColors(formP, setText + setFore + setBack);
	
	text = title->text;
	if (title)
		textLen = StrLen (text);
	else
		textLen = 0;
	WinGetWindowExtent (&windowWidth, &windowHeight);
	
	if (formP->window.windowFlags.modal)
		{
		r.topLeft.x = 0;
		r.topLeft.y = 1;
		r.extent.x = windowWidth;
		r.extent.y = FntLineHeight ();
		x  = (windowWidth - FntCharsWidth (text, textLen) + 1) / 2;

		WinDrawLine (1, 0, r.extent.x-2, 0);
		WinDrawRectangle (&r, 0);
		WinDrawInvertedChars (text, textLen, x, 0);
		
		// Draw the help icon if the dialog has a help message.
		if (formP->helpRscId)
			PrvDrawHelpIcon(windowWidth, false);
			
		}

	else if (title->rect.extent.x == windowWidth)
		{
		x  = (windowWidth - FntCharsWidth (text, textLen) + 1) / 2;
		RctSetRectangle (&r, 0, 0, windowWidth, FntLineHeight() + 2);
		titleWidth = windowWidth;
		WinDrawRectangle (&r, 3);
		WinDrawInvertedChars (text, textLen, x, 1);		
		}

	else
		{
		r.topLeft.x = 0;
		r.topLeft.y = 0;
		r.extent.x = FntCharsWidth (text, textLen) + titleMarginX + (titleMarginX - 1);
		r.extent.y = titleHeight;
		titleWidth = r.extent.x;
		WinDrawRectangle (&r, 3);
		WinDrawInvertedChars (text, textLen, titleMarginX, titleMarginY);
		
		// Draw a line under the title from the left edge of window to the right
		// edge.
		RctSetRectangle (&r, 0, titleHeight - 2, windowWidth, 2);
		WinDrawRectangle (&r, 0);
		}
	
	WinPopDrawState();
	
	// It's now safe to start blinking again.
	allowIndicator = !formP->window.windowFlags.modal &&
		titleWidth + titleMarginX * 2 - 1 >= kAttnIndicatorWidth;
	formP->attr.attnIndicator = allowIndicator;
	if (allowIndicator)
		AttnIndicatorAllow(true);
}


/***********************************************************************
 *
 * FUNCTION: 	 PrvDrawLabel
 *
 * DESCRIPTION: This routine draws a label object of a form.
 *
 * PARAMETERS:	 formP		pointer to memory block the contains the form.
 *              label   pointer to a label object
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/7/94	Initial Revision
 *
 ***********************************************************************/
static void PrvDrawLabel (FormLabelType * label)
{
	Char * text;
	Char * ptr;
	Int16 len;
	Coord y;
	
	if (! label->attr.usable) return;

	WinPushDrawState();
	WinSetTextColor(UIColorGetIndex(UIObjectForeground));

	FntSetFont (label->fontID);
	text = label->text;
	y = label->pos.y;
	
	while (true)
		{
		ptr = StrChr (text, crChr);
		if (ptr)
			len = ptr - text;
		else
			len = StrLen (text);

		WinDrawChars (text, len, label->pos.x, y);
		
		if (! ptr) break;

		text = ptr + 1;
		y += FntLineHeight ();
		}

	WinPopDrawState();
}


/***********************************************************************
 *
 * FUNCTION: 	 PrvGetLabelBounds
 *
 * DESCRIPTION: This routine returns the bounds of a label object.
 *
 * PARAMETERS:	 label   pointer to a label object
 *              rP			pointer to rectangle to hold bounds
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			gavin	1/7/96	Initial Revision extracted from eraseLabel
 *
 ***********************************************************************/
static void PrvGetLabelBounds (FormLabelType * label,RectangleType * rP)
{
	Int16 len;
	Int16 width;
	Int16 height;
	Char * ptr;
	Char * text;
	FontID curFont;

	curFont = FntSetFont (label->fontID);

	// Compute the height of the label
	text = label->text;
	height = 1;
	width = 0;
	while (true)
		{
		ptr = StrChr (text, crChr);
		if (ptr)
			len = ptr - text;
		else
			len = StrLen (text);

		width = max (width, FntCharsWidth (text, len));
		
		if (! ptr) break;

		text = ptr + 1;
		height++;
		}
	
	rP->topLeft.x = label->pos.x;
	rP->topLeft.y = label->pos.y;
	rP->extent.x = width;
	rP->extent.y = FntLineHeight() * height;
	FntSetFont (curFont);
}


/***********************************************************************
 *
 * FUNCTION: 	 PrvEraseLabel
 *
 * DESCRIPTION: This routine erases a label object of a form.
 *
 * PARAMETERS:	 formP		pointer to memory block the contains the form.
 *              label   pointer to a label object
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/5/95	Initial Revision
 *			gavin 1/7/97   Moved code into PrvGetLabelBounds
 *
 ***********************************************************************/
static void PrvEraseLabel (FormType * formP, FormLabelType * label)
{
	RectangleType r;

	if (!formP->attr.visible) return;

	PrvGetLabelBounds(label,&r);
	WinEraseRectangle (&r, 0);
}


/***********************************************************************
 *
 * FUNCTION: 	 GeBitmapBounds
 *
 * DESCRIPTION: This routine returns the bounds of a bitmap object.
 *
 * PARAMETERS:	 label   pointer to a bitmap object
 *              rP			pointer to rectangle to hold bounds
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			gavin	1/7/96	Initial Revision extracted from FrmHideObject
 *
 ***********************************************************************/
static void PrvGetBitmapBounds (FormBitmapType * bitmapP,RectangleType * rP)
{
	MemHandle resH;
	BitmapType * bitmap;

	resH = DmGetResource (bitmapRsc, bitmapP->rscID);
	bitmap = MemHandleLock (resH);
	rP->topLeft.x = bitmapP->pos.x;
	rP->topLeft.y = bitmapP->pos.y;
	rP->extent.x = bitmap->width;
	rP->extent.y = bitmap->height;
	MemHandleUnlock (resH);
	DmReleaseResource(resH);
}

#if 0
/*		11/24/99 mchen The code is no longer used because it wasn't working
 *							for all cases when called from FrmSetActiveForm().
 *							It destroyed any clipping region already set, as in
 *							the case when a dialog appears and then goes away.
 *							We might want to bring it back one day so I'm leaving
 *							it.
 */
 
/***********************************************************************
 *
 * FUNCTION:    PrvSetClippingBounds
 *
 * DESCRIPTION: Set the clipping bounds.
 * This algorithm assumes forms are full screen width and doesn't clip
 * horizontally.  Windows are not included so menus and command bars and 
 * popup list are ignored.
 *
 * PARAMETERS:  formP - form to clip
 *
 * RETURNED:    the clipping bounds are set.
 *
 * HISTORY:
 *		11/8/99	roger	Initial version.
 *
 ***********************************************************************/
static void PrvSetClippingBounds (FormType *clippedFormP)
{
	AbsRectType clippingBounds;
	RectangleType windowBounds;
	WindowType *winP;
	FormType *formP;

	
	clippingBounds.top = clippedFormP->window.windowBounds.topLeft.y;
	clippingBounds.bottom = clippingBounds.top + clippedFormP->window.windowBounds.extent.y - 1;

	// Search the window list for forms.
	winP = WinGetWindowPointer (WinGetFirstWindow ());
	
	// If there are no more windows or if the next window is the form, clipping is done.
	while (winP != NULL && winP != &clippedFormP->window)
		{
		if (winP->windowFlags.dialog && !winP->windowFlags.offscreen)
			{
			formP = (FormType *) winP;
			if (formP->attr.visible)
				{
				WinGetWindowFrameRect (WinGetWindowHandle(winP), &windowBounds);
				
				// If the window covers the form
				if (windowBounds.topLeft.y <= clippingBounds.top &&
					windowBounds.topLeft.y + windowBounds.extent.y >= clippingBounds.bottom)
					{
					// make there no pixels to clip vertically.
					clippingBounds.bottom = clippingBounds.top - 1;
					break;
					}
				
				// Clip the top of the form
				if (windowBounds.topLeft.y <= clippingBounds.top &&
					windowBounds.topLeft.y + windowBounds.extent.y > clippingBounds.top)
					{
					clippingBounds.top = windowBounds.topLeft.y + windowBounds.extent.y + 1;
					}
				// Clip the bottom of the form
				else if (windowBounds.topLeft.y <= clippingBounds.bottom)
					{
					clippingBounds.bottom = windowBounds.topLeft.y - 1;
					}
				}
			}
		winP = WinGetWindowPointer (winP->nextWindow);
		}
	
	clippedFormP->window.clippingBounds.left = 0;
	clippedFormP->window.clippingBounds.top = 0;
	clippedFormP->window.clippingBounds.right = clippedFormP->window.windowBounds.extent.x - 1;
	clippedFormP->window.clippingBounds.bottom = clippingBounds.bottom - clippingBounds.top;
}
#endif

/***********************************************************************
 *
 * FUNCTION:    PrvRedrawDisplay
 *
 * DESCRIPTION: This routine sends a frmUpdate event to all visible forms.
 *
 * PARAMETERS:  updateR - region to update
 *
 * RETURNED:    nothing
 *
 * HISTORY:
 *		10/02/95	art	Created by Art Lamb.
 *		09/09/99	roger	Fixed clipping problems when redrawing forms.
 *		09/20/99	kwk	Set formID for frmUpdateEvent event.
 *		09/21/99	roger	Mangle segment register to provide a proper environment 
 *							when calling event handling functions needing globals.
 *		09/24/99	jmp	Exclude invisible (or just erased) windows from
 *							going through the height/width check as they are
 *							not going to be drawn anyway.
 *		09/29/99	roger	Fix window checks to exempt offscreen and topmost windows.
 *		10/14/99 jmp	Make sure the draw window is set before getting the clipping
 *							rectangle to restore -- part of the fix for bug #22879.
 *
 ***********************************************************************/
static void PrvRedrawDisplay (RectangleType * updateR)
{
	WindowType * winP;
	FormType * formP;
	FormType * curForm;
	EventType event;
	WinHandle winHandle;
	RectangleType r;
	RectangleType clipR;
	RectangleType formR;
	RectangleType saveClipR;
	MemPtr currentEnvironmentP;
	SysAppInfoPtr rootAppP, ignoreAppP;
	
		
	// By convention each forms covers the entire width of the display, and
	// is bottom justified.  If this convention has been followed, then we
	// can optimize the display refresh by drawing the forms front-most to
	// rear-most.  First we need to check if our convention has been 
	// followed.
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	{
	Int16 displayWidth, displayHeight;
	
	// I think the algorithm can handle the case where the top most 
	// window doesn't obey the rules.  Exempt that case because it's useful.
	// Because FrmEraseForm has deleted any saved behind bits window, the 
	// onscreen window is topmost.
	WinGetDisplayExtent (&displayWidth, &displayHeight);
	winHandle = WinGetFirstWindow ();
	winHandle = WinGetWindowPointer (winHandle)->nextWindow;
	while (winHandle && winHandle != RootWindow)
		{
		winP = WinGetWindowPointer (winHandle);
		
		// Offscreen windows are not drawn by this function.
		// Onscreen windows (not forms) cannot be drawn.
		// Only visible forms are checked for positioning.
		if (!winP->windowFlags.dialog)
			{
			ErrNonFatalDisplayIf(!winP->windowFlags.offscreen, "Windows cannot be under forms because they can't be redrawn");
			}
		else if (((FormType *)winP)->attr.visible)
			{
			WinGetWindowFrameRect (winHandle, &r);
			ErrNonFatalDisplayIf((r.topLeft.x > 0) || (r.extent.x < displayWidth), "Form must be full width");
			ErrNonFatalDisplayIf(r.topLeft.y + r.extent.y < displayHeight, "Form must be bottom justified");
			}
		winHandle = winP->nextWindow;
		}
	}	
#endif	

	// Save the current form and the current draw window.
	WinGetDrawWindow ();
	curForm = CurrentForm;	
	
	// Save the current app environment.
	currentEnvironmentP = (MemPtr) SysSetA5(0);
	SysSetA5((UInt32) currentEnvironmentP);
	
	// Set the diplay window's clipping region to the update region,  things
	// like frames around windows are drawn in the display window.
	WinSetDrawWindow (WinGetDisplayWindow ());
	WinSetClip (updateR);

	clipR = *updateR;

	// Search the window list for forms.
	winHandle = WinGetFirstWindow ();
	while (winHandle)
		{
		winP = WinGetWindowPointer (winHandle);

		if (winP->windowFlags.dialog && !winP->windowFlags.offscreen)
			{
			formP = (FormType *) winP;
			if (formP->attr.visible)
				{
				// Set the clipping region to the bounds of the region
				// we want to update, first ensuring that the draw window
				// is for the form.
				WinSetDrawWindow (winHandle);
				WinGetClip (&saveClipR);

				WinGetWindowFrameRect (winHandle, &formR);				
				RctGetIntersection (&formR, &clipR, &r);
				if (r.extent.y > 0)
					{
					// Ensure that any drawing is clipped.
					WinDisplayToWindowPt (&r.topLeft.x, &r.topLeft.y);
					WinSetClip (&r);
	
					// Make the form the active form, and prepare to
					// send it an update event.
					FrmSetActiveForm (formP);
					MemSet (&event, sizeof(EventType), 0);
					event.eType = frmUpdateEvent;
					event.data.frmUpdate.updateCode = frmRedrawUpdateCode;
					event.data.frmUpdate.formID = formP->formId;
					
					// Mangle the segment register to either provide globals or not.
					// The problem is that when a sublaunched app is active, the segment 
					// register can be protected.  Forms needing redrawing may need access to 
					// their globals.  The preferred method is to store environment information
					// in the form, assuming sublaunched apps are good things.  Maintaining 
					// data compatibility with old forms means we currently do the less preferred
					// way.
					SysGetAppInfo(&rootAppP, &ignoreAppP);
					if (formP->attr.globalsAvailable)
						SysSetA5((UInt32) rootAppP->a5Ptr);

					// Let the form handle the update event, if it has an event
					// handler defined.  Otherwise, have the default event handler
					// update the display.
					PrvSendEventToForm (formP, &event);
					
					// Make the form no longer active.  When this happens, some draws occur (GSI),
					// and those need to occur with the current clip rectangle.
					FrmSetActiveForm(NULL);
					
					// Restore the current environment.
					if (formP->attr.globalsAvailable)
						SysSetA5((UInt32) currentEnvironmentP);

					// Restore the form's clipping region.
					WinSetDrawWindow (winHandle);
					WinSetClip (&saveClipR);
	
					// Check if we've filled in the entire update region.
					if (formR.topLeft.y <= updateR->topLeft.y) break;			
		
					// Reduce the size of the update region by the area of the 
					// form just drawn.
					clipR.extent.y = formR.topLeft.y - updateR->topLeft.y;
					
					// Now update the DisplayWindow's clip region so that FrmDrawWindow
					// gets the frames clipped properly.
					WinSetDrawWindow (WinGetDisplayWindow ());
					WinSetClip (&clipR);
					}
				}
			}
		winHandle = winP->nextWindow;		
		}
	
	// Reset the clipping bounds of the DisplayWindow to it's full size.
	WinSetDrawWindow (WinGetDisplayWindow ());
	WinResetClip ();
	
	FrmSetActiveForm(curForm);
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmDrawForm
 *
 * DESCRIPTION: This routine draws all the object in a form and the 
 *              frame around the form.
 *
 * PARAMETERS:	 frmHandle	the handle of memory block the contains the form.
 *					
 * RETURNED:	 nothing
 *
 * HISTORY:
 *		11/07/94	art	Created by Art Lamb.
 *		02/20/96	art	Added scroll bar
 *		08/11/97	trev	mode non modified passed variables constant
 *		02/27/99	meg	changed the logic to always erase the area
 *							behind the form. previously, it was only erasing
 *							if the saveBehind bit was set.
 *		09/28/99	grant	Changed the erase logic to be more compatible with
 *							older versions.  Now it does not erase if the
 *							form was already visible.
 *		10/11/99	roger	Clean up Graffiti shift indicator and more.
 *		08/28/00	kwk	Use new FrmGetActiveField routine.
 *		10/13/00	roger	Fill forms when UIFillColor is changed.
 *
 ***********************************************************************/
void FrmDrawForm (FormType * formP)
{
	UInt16 i;
	UInt16 error;
	UInt32 freeBytes;
	UInt32 maxChunk;
	RectangleType frame;
	FormObjectType obj;
	WinHandle win;
	FontID currFont;
	MemHandle resH;
	FieldType *fldP;

	ECFrmValidatePtr(formP);
	
	// Some apps call FrmDrawForm when they notice that the form has become the 
	// active window.
	win = FrmGetWindowHandle (formP);
	if (win != WinGetActiveWindow())
		WinSetActiveWindow (win);

	WinSetDrawWindow (DisplayWindow);
	WinGetWindowFrameRect (win, &frame);

	// Save the bits behind the form if there's enough memory to do so.
	if (formP->attr.saveBehind && !formP->attr.visible)
		{
		WinClipRectangle (&frame);
		
		// if we are going to save the bits behind the window, do it now.
		if (formP->attr.saveBehind)
			{
			MemHeapFreeBytes (0, &freeBytes, &maxChunk);
			if (freeBytes > frmMinFreeBytes)		// DOLATER - frmMinFreeBytes needs to be updated for larger bitmaps!
				formP->bitsBehindForm = WinSaveBits (&frame, &error);
			else
				formP->bitsBehindForm = 0;
			}
		}

	// set up the colors, make text color for generic objects
	WinSetTextColor(UIColorGetIndex(UIObjectForeground));
	PrvSetAppropriateColors(formP, setFore + setBack);
	
	// only erase dialogs, alerts and 'save behind' forms for backward compat.
	// erase if UIFormFill is changed.  Guess by assuming UIFormFill was white.
	// Always erase everything on debug builds
#if ERROR_CHECK_LEVEL < ERROR_CHECK_FULL
	if ((formP->attr.saveBehind && !formP->attr.visible) || 
		formP->formId == CustomAlertDialog || 
		UIColorGetIndex(UIFormFill) != 0 || 	// Fill the form if it's not white.
		formP->window.windowFlags.modal)
#endif
		WinEraseRectangle (&frame, fmrCornerDiam);

	// Set back to the current window, rather than the DisplayWindow
	WinSetDrawWindow (win);

	// draw the form frame
	WinDrawWindowFrame();

	currFont = FntGetFont ();

	for (i = 0; i < formP->numObjects; i++)
		{
		obj = formP->objects[i].object;
		switch (formP->objects[i].objectType)
			{
			case frmFieldObj:
				FldDrawField (obj.field);
				break;

			case frmControlObj:
				// Old-style graphic buttons will be overdrawn by the corresponding bitmap,
				// requiring their selection highlighting to be redrawn
				obj.control->attr.visible = false;
				obj.control->attr.drawnAsSelected = false;
				
				CtlDrawControl (obj.control);
				break;

			case frmListObj:
				LstDrawList (obj.list);
				break;

			case frmTableObj:
				TblDrawTable (obj.table);
				break;

			case frmGadgetObj:
				if (obj.gadget->attr.usable && obj.gadget->attr.extended)
					{
					FormGadgetHandlerType *handler = obj.gadget->handler;
					
					if (handler != 0)
						(handler)(obj.gadget, formGadgetDrawCmd, 0);
					}
				break;
			
			case frmBitmapObj:
				if (obj.bitmap->attr.usable)
					{
					resH = DmGetResource (bitmapRsc, obj.bitmap->rscID);
					WinSetForeColor(UIColorGetIndex(UIObjectForeground));
					WinDrawBitmap (MemHandleLock (resH), 
						obj.bitmap->pos.x, obj.bitmap->pos.y);
					MemHandleUnlock (resH);
					DmReleaseResource(resH);
					}
				break;

			case frmLineObj:
				break;

			case frmFrameObj:
				break;

			case frmRectangleObj:
				break;

			case frmLabelObj:
				PrvDrawLabel (obj.label);
				break;

			case frmTitleObj:
				PrvDrawTitle (formP, obj.title);
				break;

			case frmScrollBarObj:
				SclDrawScrollBar (obj.scrollBar);
				break;

			case frmGraffitiStateObj:
				GsiSetLocation (obj.grfState->pos.x, obj.grfState->pos.y);
				formP->attr.graffitiShift = true;
				GsiEnable (true);
				break;
			}
		}
	
	// Restore the focus in the new current form.
	fldP = FrmGetActiveField(formP);
	if (fldP != NULL)
		{
		FldGrabFocus(fldP);
		}
	
	FntSetFont (currFont);

	formP->attr.visible = true;
	
	// set up default foreground/text color for generic objects
	WinSetTextColor(UIColorGetIndex(UIObjectForeground));
	WinSetForeColor(UIColorGetIndex(UIObjectForeground));
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmEraseForm
 *
 * DESCRIPTION: This routine erases a form from the display.  If the region
 *              obscured by the form was saved by FrmDrawForm, this routine
 *              will restore that region.
 *
 * PARAMETERS:	 formPtr	-> pointer to the form.
 *					
 * RETURNED:	 nothing
 *
 * HISTORY:
 *		11/07/94	art	Created by Art Lamb.
 *		09/9/99	roger	Fixed clipping problems when redrawing forms.
 *							Always redraw forms in debug builds, ignore saved bits.
 *							Fixed not drawing forms under form if no saveBehind flag.
 *		09/27/99	roger	Added AllFormsClosing mode.
 *		10/6/99	jmp	Moved the menu-erasing code from frmCloseEvent to here
 *							to prevent overlapping window redraw problmes from
 *							occurring.  Fixes bug #22584.
 *		08/28/00	kwk	Use new FrmGetActiveField routine.
 *		09/22/00	roger	Don't set the draw window to the display when not drawing (AllFormsClosing).
 *
 ***********************************************************************/
void FrmEraseForm (FormType * formP)
{
	RectangleType r;
	MenuBarType *menuP;
	
	ECFrmValidatePtr(formP);
	
	MenuEraseStatus (0);

	// If there is one, eliminate the menu before
	// we eliminate the form to clean up both
	// visual artifacts as well prevent the Menu
	// Manager from leaving restore-behind objects
	// in the heap.
	menuP = MenuGetActiveMenu();
	if (menuP)
		MenuEraseMenu(menuP, removeCompleteMenu);

	if (!formP->attr.visible) return;
	formP->attr.visible = 0;

	// Disable the insertion point and the graffiti shift state indicator.
	// Preserve the focus information so we can restore it if the form is 
	// redrawn.
	if (formP == FrmGetActiveForm())
		{
		FieldType* fldP = FrmGetActiveField(formP);
		if (fldP != NULL)
			{
			FldReleaseFocus(fldP);
			}
			
		InsPtEnable (false);
		AttnIndicatorAllow (false);
		GsiEnable (false);
		}

	if (AllFormsClosing)
		{
		// Do not draw to the screen.  FrmCloseAllForms will clear the screen by drawing 
		// the root window.  Do cleanup any bits behind the form.
		if (formP->bitsBehindForm)
			{
			WinDeleteWindow (formP->bitsBehindForm, false);
			formP->bitsBehindForm = 0;
			}
		}
	else
		{
		// Get the bounds to the form and its frame.
		WinSetDrawWindow (WinGetDisplayWindow ());
		WinGetWindowFrameRect (FrmGetWindowHandle(formP), &r);
		WinClipRectangle (&r);
		
			
#if ERROR_CHECK_LEVEL != ERROR_CHECK_FULL

		// In debug ROMs, force forms to ignore there saved images and redraw
		// instead.  This continues to use memory as normal but tests the redraw
		// mechanisms.


		// If the image behind the form was save (by the draw routine), restore
		// the display region obscured by the form.
		if ((formP->attr.saveBehind) && (formP->bitsBehindForm))
			{
			WinRestoreBits (formP->bitsBehindForm, r.topLeft.x, r.topLeft.y);
			formP->bitsBehindForm = 0;
			}
		else
		
#else

		// When the code above isn't executed, the saved bits still need to be deleted.
		if (formP->bitsBehindForm)
			{
			WinDeleteWindow (formP->bitsBehindForm, false);
			formP->bitsBehindForm = 0;
			}

#endif
			{
			// no bits to restore; just clear to the form's default fill color
			WinPushDrawState();
			PrvSetAppropriateColors(formP, setBack);
			WinEraseRectangle (&r, fmrCornerDiam);
			WinPopDrawState();
			
			// Always redraw all forms under the one just erased.
			PrvRedrawDisplay (&r);
			
			// If we're returning to a non-modal form rather than another
			// modal dialog, then we need to enable the attention indicator.
//			if (!FrmGetFirstForm()->window.windowFlags.modal)
//				AttnIndicatorAllow(true);
//DOLATER - peter: this also happens when switching forms, but not doing this
//						 means closing details dialog in Memo Pad doesn't start blinking again.
			}
		}
	WinSetDrawWindow (NULL);
	WinSetActiveWindow (NULL);

}



/***********************************************************************
 *
 * FUNCTION: 	 FrmGetUserModifiedState
 *
 * DESCRIPTION: This routine returns TRUE if an object in the form has 
 *              been modified by the user since it was initialized or 
 *              since the last call to FrmSetNotUserModified.
 *
 * PARAMETERS:	 formP	memory block that contains the form.
 *					
 * RETURNED:	 TRUE if an object was modified
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/7/94	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
Boolean FrmGetUserModifiedState (const FormType * formP)
{
	ECFrmValidatePtr(formP);
	
	return (formP->attr.dirty);
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmSetNotUserModified
 *
 * DESCRIPTION: This routine clear the flag the keeps track of whether
 *              or not the form has been modified by the user.
 *
 * PARAMETERS:	 formP	memory block that contains the form.
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/7/94	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
void FrmSetNotUserModified (FormType * formP)
{
	ECFrmValidatePtr(formP);
	
	formP->attr.dirty = false;
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmGetFocus
 *
 * DESCRIPTION: This routine returns the item number of the object 
 *              that has the focus.
 *
 * PARAMETERS:	 formP	memory block that contains the form.
 *					
 * RETURNED:	 object with focus, noFocus if none.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/7/94	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
UInt16 FrmGetFocus (const FormType * formP)
{
	ECFrmValidatePtr(formP);
	
	return (formP->focus);
}


/***********************************************************************
 *
 * FUNCTION: 	 PrvReleaseFocus
 *
 * DESCRIPTION: This routine releases the focus from the form object that
 *              has it.
 *
 * PARAMETERS:	 formP	 memory block that contains the form.
 *              newFocusIndex item number of the object that gets the focus
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	1/8/94	Initial Revision
 *			bob	12/8/98	add call to FldReleaseFocus if new focus is a table
 *								simplify fn, rename some variables for clarity
 *
 ***********************************************************************/
static void PrvReleaseFocus (FormType * formP, UInt16 newFocusIndex)
{
	ECFrmValidatePtr(formP);
	ErrNonFatalDisplayIf (newFocusIndex != noFocus && newFocusIndex >= formP->numObjects, "Invalid index");
	
	
	// If a field or a table has the focus, and isn't getting it again, release it.
	if ((formP->focus != noFocus) &&
		(CurrentForm == formP) &&
		(formP->focus != newFocusIndex))
		{
		FormObjectKind oldFocusType;
		oldFocusType = formP->objects[formP->focus].objectType;

		if (oldFocusType == frmFieldObj)
			{
			FieldType * fldP;
			fldP = formP->objects[formP->focus].object.field;
//			ErrNonFatalDisplayIf(!fldP->attr.hasFocus, "FldReleaseFocus called instead of FrmSetFocus(noFocus).");
			
			// only release focus if new focus is nothing, field, or table
			if (newFocusIndex == noFocus || formP->objects[newFocusIndex].objectType == frmTableObj)
				{
				formP->focus = noFocus;
				FldReleaseFocus (fldP);
				}
			else if (formP->objects[newFocusIndex].objectType == frmFieldObj)
				{
				formP->focus = noFocus;
				FldReleaseFocus (fldP);
				FldSetSelection (fldP, 0, 0);
				}
			}

		else if (oldFocusType == frmTableObj)
			{
			TableType * tableP;
			
			tableP = formP->objects[formP->focus].object.table;
			formP->focus = noFocus;
//			ErrNonFatalDisplayIf(!tableP->attr.editable, "TblReleaseFocus called instead of FrmSetFocus(noFocus).");
			TblReleaseFocus (tableP);
			}
		}
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmSetFocus
 *
 * DESCRIPTION: This routine sets the focus of a form to the object specified.
 *
 * PARAMETERS:	 formP	 memory block that contains the form.
 *              objIndex item number of the object that gets the focus
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/7/94	Initial Revision
 *       art	1/8/97	Broke release logic out into a seperate routine
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
void FrmSetFocus (FormType * formP, UInt16 objIndex)
{
	FormObjectKind newObjType;


	ECFrmValidatePtr(formP);
	ErrNonFatalDisplayIf (objIndex != noFocus && objIndex >= formP->numObjects, "Invalid index");
	
	
	// If a field or a table has the focus, release it.
	PrvReleaseFocus (formP, objIndex);

	if (objIndex == noFocus)
		{
		formP->focus = objIndex;
		return;
		}

	newObjType = formP->objects[objIndex].objectType;
	if (newObjType == frmFieldObj)
		{
		formP->focus = objIndex;

		// Turn on the insertion point, if the object is a field.
		if (CurrentForm == formP && formP->attr.visible)
			{
			FldGrabFocus (formP->objects[objIndex].object.field);
			}
		}
	else if (newObjType == frmTableObj)
		{
		formP->focus = objIndex;
		}
	else
		ErrNonFatalDisplay("Bad object to get focus");
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmGetFormBounds
 *
 * DESCRIPTION: This routine returns the bounds to the form; the region
 *              returned includes the form's frame.
 *
 * PARAMETERS:	 formP	memory block that contains the form.
 *					 rP       pointer to a RectangleType structure
 *
 * RETURNED:	 bounds of the form
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/7/94	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
void FrmGetFormBounds (const FormType * formP, RectangleType * rP)
{
	WinHandle drawWin;

	ECFrmValidatePtr(formP);
	
	drawWin = WinSetDrawWindow (WinGetDisplayWindow ());
	WinGetWindowFrameRect (FrmGetWindowHandle(formP), rP);
	WinSetDrawWindow (drawWin);
}



/***********************************************************************
 *
 * FUNCTION: 	 FrmGetFormId
 *
 * DESCRIPTION: This routine returns the resource id of a form.
 *
 * PARAMETERS:	 formP	memory block that contains the form.
 *					
 * RETURNED:	 form resource id
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/7/94	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
UInt16 FrmGetFormId (const FormType * formP)
{
	ECFrmValidatePtr(formP);
	
	return (formP->formId);
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmGetFormPtr
 *
 * DESCRIPTION: This routine returns a pointer to the form that contains
 *              the id specified.
 *
 * PARAMETERS:	 form id
 *					
 * RETURNED:	 memory block that contains the form or null if the form
 *              is not in memory.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/7/94	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
FormType * FrmGetFormPtr (UInt16 formId)
{
	WinHandle winHandle;
	WindowType * winP;
	FormType * formP;
	
	winHandle = WinGetFirstWindow ();
	while (winHandle)
		{
		winP = WinGetWindowPointer (winHandle);
		if (winP->windowFlags.dialog)
			{
			formP = (FormType *) winP;
			if (formP->formId == formId)
				return (formP);
			}
		winHandle = winP->nextWindow;		
		}

	return (NULL);
}


/***********************************************************************
 *
 * FUNCTION:    FrmGetFirstForm
 *
 * DESCRIPTION: This routine returns the first form in the window list.
 *              The window list is a LIFO stack fo sort,  the last window
 *              created is the first window in the window list.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    a pointer to a formP or NULL if there are not forms.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/29/95	Initial Revision
 *
 ***********************************************************************/
FormType * FrmGetFirstForm (void)
{
	WinHandle winHandle;
	WindowType * winP;
	
	winHandle = WinGetFirstWindow ();
	while (winHandle)
		{
		winP = WinGetWindowPointer (winHandle);
		if (winP->windowFlags.dialog)
			{
			return ((FormType *) winP);
			}
		winHandle = winP->nextWindow;		
		}

	return (NULL);
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmGetNumberOfObjects
 *
 * DESCRIPTION: This routine returns the number of objects in a form.
 *
 * PARAMETERS:	 formP	memory block that contains the form.
 *					
 * RETURNED:	 number of object in the form specified
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/7/94	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
UInt16 FrmGetNumberOfObjects (const FormType * formP)
{
	ECFrmValidatePtr(formP);
	
	return (formP->numObjects);
}



/***********************************************************************
 *
 * FUNCTION: 	 FrmGetObjectId
 *
 * DESCRIPTION: This routine returns the id of the object specified.  
 *              An id is a value specified by the application developer 
 *              that uniquely identifies an object.
 *
 * PARAMETERS:	 formP	 memory block that contains the form.
 *              objIndex item number of the object
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/7/94	Initial Revision
 *			art	2/20/96	Add scroll bar
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
UInt16 FrmGetObjectId (const FormType * formP, UInt16 objIndex)
{
	FormObjectType obj;

	ECFrmValidatePtr(formP);
	ErrNonFatalDisplayIf (objIndex >= formP->numObjects, "Invalid index");
	
	
	obj = formP->objects[objIndex].object;

	switch (formP->objects[objIndex].objectType)
		{
		case frmFieldObj:
			return (obj.field->id);

		case frmControlObj:
			return (obj.control->id);

		case frmListObj:
			return (obj.list->id);

		case frmTableObj:
			return (obj.table->id);
			
		case frmLabelObj:
			return (obj.label->id);

		case frmBitmapObj:
			return (obj.bitmap->rscID);

		case frmGadgetObj:
			return (obj.gadget->id);
			
		case frmScrollBarObj:
			return (obj.scrollBar->id);
			
		
		};

	// invalid object index
	return (frmInvalidObjectId);
}



/***********************************************************************
 *
 * FUNCTION: 	 FrmGetObjectType
 *
 * DESCRIPTION: This routine returns the type of an object.
 *
 * PARAMETERS:	 formP	memory block that contains the form.
 *              objIndex item number of the object
 *					
 * RETURNED:	 FormObjectType of the item specified
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/7/94	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
FormObjectKind FrmGetObjectType (const FormType * formP, UInt16 objIndex)
{
	ECFrmValidatePtr(formP);
	// GMP take out error check for weblib dynamic forms
	//ErrNonFatalDisplayIf (objIndex >= formP->numObjects, "Invalid index");	
	
	return (formP->objects[objIndex].objectType);
}



/***********************************************************************
 *
 * FUNCTION: 	 FrmGetObjectIndex
 *
 * DESCRIPTION: This routine returns the item number of an object, the 
 *              item number is the position of the object in the objects 
 *              list.
 *
 * PARAMETERS:	 formP	memory block that contains the form.
 *              objId   id of an object in the form.
 *					
 * RETURNED:	 item number of na  object (the first item number is 0).
 *
 * HISTORY:
 *		11/07/94	art	Created by Art Lamb.
 *		08/11/97	trev	mode non modified passed variables constant
 *		10/12/00	kwk	Better error message for obj not in form (form id, obj id).
 *							Also no longer display fatal alert on release ROMs.
 *
 ***********************************************************************/
UInt16 FrmGetObjectIndex (const FormType* formP, UInt16 objId)
{
	UInt16 i;

	ECFrmValidatePtr(formP);
	
	for (i = 0; i < formP->numObjects; i++)
		{
		if (FrmGetObjectId (formP, i) == objId)
			return (i);
		}

#if (ERROR_CHECK_LEVEL == ERROR_CHECK_FULL)
	{
	Char msg[45];
	
	StrPrintF(msg, "Object #%d in form #%d is missing", objId, formP->formId);
	ErrDisplay(msg);
	}
#endif

	return (frmInvalidObjectId);
}



/***********************************************************************
 *
 * FUNCTION: 	 FrmGetObjectIndexFromPtr
 *
 * DESCRIPTION: This routine returns the item number of an object, the 
 *              item number is the position of the object in the objects 
 *              list.
 *
 * PARAMETERS:	 formP	memory block that contains the form.
 *              objP   ptr to an object in the form.
 *					
 * RETURNED:	 item number of an object (the first item number is 0).
 *
 * HISTORY:
 *		11/07/94	tlw	Aha.
 *
 ***********************************************************************/
UInt16 FrmGetObjectIndexFromPtr (const FormType* formP, void* objP)
{
	UInt16 i;

	ECFrmValidatePtr(formP);
	
	for (i = 0; i < formP->numObjects; i++)
		{
		if (formP->objects[i].object.ptr == objP)
			return (i);
		}

	ErrNonFatalDisplay("Obj ptr not in form");

	return (frmInvalidObjectId);
}



/***********************************************************************
 *
 * FUNCTION: 	 FrmGetObjectPtr
 *
 * DESCRIPTION: This routine returns a pointer to the data structure of an 
 *              object in a form.
 *
 * PARAMETERS:	 formP	memory block that contains the form.
 *              objIndex item number of the object
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/7/94	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
void * FrmGetObjectPtr (const FormType * formP, UInt16 objIndex)
{
	ECFrmValidatePtr(formP);
	// GMP take out error check for weblib dynamic forms
	//ErrNonFatalDisplayIf (objIndex >= formP->numObjects, "Invalid index");
	
	return (formP->objects[objIndex].object.ptr);
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmGetObjectBounds
 *
 * DESCRIPTION: This routine returns the bounds of the specified form object
 *
 * PARAMETERS:	 formP	 memory block that contains the form.
 *				 objIndex  index of the object
 *				 rP			pointer to returned bounds rectangle
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	8/3/95	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
void FrmGetObjectBounds (const FormType * formP, UInt16 objIndex, RectangleType * rP)
{
	ListType * lstP;
	FieldType * fldP;
	TableType * tblP;
	ControlType * ctlP;
	FormObjectKind objType;
	

	ECFrmValidatePtr(formP);
	ErrNonFatalDisplayIf (objIndex >= formP->numObjects, "Invalid index");
	
	
	objType = formP->objects[objIndex].objectType;

	if (objType == frmControlObj)
		{
		ctlP = formP->objects[objIndex].object.control;
		RctCopyRectangle (&ctlP->bounds, rP);
		return;
		}

	else if (objType == frmFieldObj)
		{
		fldP = formP->objects[objIndex].object.field;
		RctCopyRectangle (&fldP->rect, rP);			
		return;
		}

	else if (objType == frmTableObj)
		{
		tblP = formP->objects[objIndex].object.table;
		RctCopyRectangle (&tblP->bounds, rP);			
		return;
		}

	else if (objType == frmListObj)
		{
		lstP = formP->objects[objIndex].object.list;
		RctCopyRectangle (&lstP->bounds, rP);
		return;
		}
					
	else if (objType == frmGadgetObj)
		{
		RctCopyRectangle (&formP->objects[objIndex].object.gadget->rect, rP);
		return;
		}
					
	else if (objType == frmScrollBarObj)
		{
		RctCopyRectangle (&formP->objects[objIndex].object.scrollBar->bounds, rP);
		return;
		}
					
	else if (objType == frmLabelObj)
		{
		PrvGetLabelBounds(formP->objects[objIndex].object.label,rP);
		return;
		}
					
	else if (objType == frmBitmapObj)
		{
		PrvGetBitmapBounds(formP->objects[objIndex].object.bitmap,rP);
		return;
		}
			
	else if (objType == frmGraffitiStateObj)
		{
		FontID curFont = FntSetFont (symbolFont);
		rP->topLeft = formP->objects[objIndex].object.grfState->pos;
		rP->extent.x = FntAverageCharWidth();
		rP->extent.y = FntLineHeight();
		FntSetFont (curFont);
		return;
		}
			
	else
		{
		RctSetRectangle (rP, 0, 0, 0, 0);
		return;
		}	
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmHideObject
 *
 * DESCRIPTION: This routine erases the specified object and set its 
 *              attribute data so that it will do redraw or respond 
 *              to the pen.
 *
 * PARAMETERS:	 formP	memory block that contains the form.
 *              objIndex item number of the object
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/7/94	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *			roger	7/22/98	Bitmaps now set not usable.
 *			grant	2/15/99	Some changes to be more consistent with FrmShowObject.
 *								The actions taken in each case (except empty cases) are:
 *								1. erase the object if the form is visible
 *								2. set the object to unusable
 *								This fixes a bug where an object was not set unusable
 *								if the containing form was not visible.
 *			jmp	10/11/99	Added support for frmScrollBarObj.
 *
 ***********************************************************************/
void FrmHideObject (FormType * formP, UInt16 objIndex)
{
	RectangleType r;
	FormObjectType obj;

	ECFrmValidatePtr(formP);
	ErrNonFatalDisplayIf (objIndex >= formP->numObjects, "Invalid index");
	
	
	obj = formP->objects[objIndex].object;

	switch (formP->objects[objIndex].objectType)
		{
			case frmFieldObj:
				if (formP->attr.visible)
					{
					FldEraseField (obj.field);
					if (formP->focus == objIndex)
						FldReleaseFocus (obj.field);
					}
				FldSetUsable (obj.field, false);
				break;

			case frmControlObj:
				//CtlHideControl (obj.control);
				if (formP->attr.visible)
					CtlEraseControl (obj.control);
				CtlSetUsable (obj.control, false);
				break;

			case frmListObj:
				if (formP->attr.visible && obj.list->attr.visible)
					LstEraseList(obj.list);
				obj.list->attr.usable = false;
				break;

			case frmTableObj:
				// Tables can now be hidden/shown programmaticly using
				// the FrmHideObject/FrmShowObject commands.  A usable bit 
				// was added that is set to true by default because it is
				// not an editable field supported by existing resource editing tools.
							
				// before hiding a table object, test to see if the object has the
				// focus and release it.
				TblReleaseFocus(obj.table);
				if (formP->attr.visible && obj.table->attr.visible)
					TblEraseTable(obj.table);
				obj.table->attr.usable = false;
				
				break;

			case frmBitmapObj:
				if (formP->attr.visible && obj.bitmap->attr.usable) 
					{
					PrvGetBitmapBounds(obj.bitmap,&r);
					WinEraseRectangle (&r, 0);
					}
				obj.bitmap->attr.usable = false;
				break;

			case frmLineObj:
				break;

			case frmFrameObj:
				break;

			case frmRectangleObj:
				break;

			case frmLabelObj:
				if (formP->attr.visible && obj.label->attr.usable)
					PrvEraseLabel (formP, obj.label);
				obj.label->attr.usable = false;
				break;
			
			case frmGadgetObj:
				if (formP->attr.visible &&
					 obj.gadget->attr.extended && obj.gadget->attr.usable)
				{
					FormGadgetHandlerType *handler = obj.gadget->handler;
					
					// rely on short circuiting to not call the handler if it's null
					// and if there is no handler or if the handler returns false,
					// then erase the gadget bounds rectangle
					if ((handler == NULL) ||
					    ((handler)(obj.gadget, formGadgetEraseCmd, 0) == false))
					    {
						WinEraseRectangle(&(obj.gadget->rect), 0);
						}
					obj.gadget->attr.visible = false;
				}
				obj.gadget->attr.usable = false;
				break;
				
			case frmScrollBarObj:
				if (formP->attr.visible && obj.scrollBar->attr.visible && obj.scrollBar->attr.shown)
					{
					WinEraseRectangle (&obj.scrollBar->bounds, 0);
					obj.scrollBar->attr.shown = false;
					obj.scrollBar->attr.visible = false;
					}
				obj.scrollBar->attr.usable = false;
				break;
		}
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmShowObject
 *
 * DESCRIPTION: This routine sets an object usable and, if the form is 
 *              visible, draw the object.
 *
 * PARAMETERS:	 formP	memory block that contains the form.
 *              objIndex item number of the object
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/7/94	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *			jmp	10/11/99	Added support for frmScrollBarObj.
 *
 ***********************************************************************/
void FrmShowObject (FormType * formP, UInt16 objIndex)
{
	FontID curFont;
	MemHandle resH;
	FormObjectType obj;

	ECFrmValidatePtr(formP);
	ErrNonFatalDisplayIf (objIndex >= formP->numObjects, "Invalid index");
	
	if (formP->attr.visible)
		WinPushDrawState();
/*
	// hmmm, don't do this now in case developer has set a custom
	// background color
	
	PrvSetAppropriateColors(formP, setBack);
*/
	
	obj = formP->objects[objIndex].object;

	switch (formP->objects[objIndex].objectType)
		{
			case frmFieldObj:
				FldSetUsable (obj.field, true);
				if (formP->attr.visible)
					{
					FldDrawField (obj.field);
					if (formP->focus == objIndex)
						FldGrabFocus (obj.field);
					}
				break;

			case frmControlObj:
				CtlSetUsable (obj.control, true);
				if (formP->attr.visible)
					CtlDrawControl (obj.control);
				break;

			case frmListObj:
				{
				UInt16  i;
				Boolean showObj = true;
				// Check if this list object is handled by a popup object. If so, do not show it.
				for (i = 0; i < formP->numObjects; i++)
					if ( (formP->objects[i].objectType == frmPopupObj) &&
						 (formP->objects[i].object.popup->listID == obj.list->id) )
						{
						showObj = false;
						ErrNonFatalDisplay("don't show list");
						break;
						}
				if (showObj)
					{
					obj.list->attr.usable = true;
					if (formP->attr.visible)
						LstDrawList(obj.list);
					}
				}
				break;

			case frmTableObj:
				// Tables can now be hidden/shown programmaticly using
				// the FrmHideObject/FrmShowObject commands.  A usable bit 
				// was added that is set to true by default because it is
				// not an editable field supported by existing resource editing tools.
				obj.table->attr.usable = true;
				if (formP->attr.visible)
					TblDrawTable (obj.table);
				break;

			case frmBitmapObj:
				obj.bitmap->attr.usable = true;
				if (formP->attr.visible) 
					{
					resH = DmGetResource (bitmapRsc, obj.bitmap->rscID);
					WinDrawBitmap (MemHandleLock (resH), 
						obj.bitmap->pos.x, obj.bitmap->pos.y);
					MemHandleUnlock (resH);
					DmReleaseResource(resH);
					}
				break;

			case frmLineObj:
				break;

			case frmFrameObj:
				break;

			case frmRectangleObj:
				break;

			case frmLabelObj:
				obj.label->attr.usable = true;
				if (formP->attr.visible)
					{
					curFont = FntSetFont (obj.label->fontID);
					WinSetTextColor (UIColorGetIndex(UIObjectForeground));
					PrvDrawLabel (obj.label);
					FntSetFont (curFont);
					}
				break;

			case frmGadgetObj:
				obj.gadget->attr.usable = true;
				if (formP->attr.visible && obj.gadget->attr.extended)
					{
					FormGadgetHandlerType *handler = obj.gadget->handler;
					
					// rely on short circuiting to not call the handler if it's null
					if (handler)
					    (handler)(obj.gadget, formGadgetDrawCmd, 0);
					}
				break;

			case frmScrollBarObj:
				obj.scrollBar->attr.usable = true;
				if (formP->attr.visible)
					SclDrawScrollBar (obj.scrollBar);
				break;
		}
	
	if (formP->attr.visible)
		WinPopDrawState();
}



/***********************************************************************
 *
 * FUNCTION: 	 FrmGetActiveForm
 *
 * DESCRIPTION: This routine returns the currently active form.
 *
 * PARAMETERS:	 nothing
 *					
 * RETURNED:	 formP	memory block that contains the form.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/7/94	Initial Revision
 *
 ***********************************************************************/
FormType * FrmGetActiveForm (void)
{
	return (CurrentForm);
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmGetActiveFormID
 *
 * DESCRIPTION: This routine returns the id of the currently active form.
 *
 * PARAMETERS:	 nothing
 *					
 * RETURNED:	 formP	memory block that contains the form.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/13/95	Initial Revision
 *
 ***********************************************************************/
UInt16 FrmGetActiveFormID (void)
{
	if (CurrentForm)
		return (CurrentForm->formId);
	return (0);
}


/***********************************************************************
 *
 * FUNCTION:	FrmGetActiveField
 *
 * DESCRIPTION:	Return the active field for formP, or for the active
 *	form if formP is NULL. If there is no active form, or no active
 *	field for the form, then return NULL
 *
 * PARAMETERS:
 *	formP	 ->	Pointer to the form, or NULL to use the active form.
 *					
 * RETURNED:
 *	Pointer to a field, or NULL if there is no active field.
 *
 * HISTORY:
 *	08/28/00	kwk	Created by Ken Krugler.
 *
 ***********************************************************************/
FieldType* FrmGetActiveField(const FormType* formP)
{
	if (formP == NULL)
	{
		formP = CurrentForm;
	}
	
	if ((formP != NULL)
	 && (formP->focus != noFocus))
	{
		FormObjListType* objListP = formP->objects + formP->focus;
		
		if (objListP->objectType == frmFieldObj)
		{
			return(objListP->object.field);
		}
		else if (objListP->objectType == frmTableObj)
		{
			return(TblGetCurrentField(objListP->object.table));
		}
	}
	
	return(NULL);
} // FrmGetActiveField


/***********************************************************************
 *
 * FUNCTION: 	 FrmSetActiveForm
 *
 * DESCRIPTION: This routine sets the active form, all input (key and pen)
 *              is directed to the active form.
 *
 * PARAMETERS:	 formP	memory block that contains the form.
 *					
 * RETURNED:	 nothing
 *
 * HISTORY:
 *		11/07/94	art	Created by Art Lamb.
 *		05/15/96	art	Don't release focus in table if no current field
 *		01/28/99	kwk	Set active/cur window when shutting down cur form.
 *		10/11/99	roger	Clean up DrawWindow, ActiveWindow, and more
 *		10/26/99	jmp	Remove savedBack from GSI drawing as it caused
 *							more harm than good!
 *		08/28/00	kwk	Use new FrmGetActiveField routine.
 *
 ***********************************************************************/
void FrmSetActiveForm (FormType * formP)
{
	FieldType * fldP;
	MenuBarType * menuP;
	UInt16 i;
	FrmGraffitiStateType *  grfStateObj;
	
	
	if (CurrentForm == formP) return;


	// Make sure the current form exist.
	#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
		if (CurrentForm)
			{
			Boolean exist = false;
			WinHandle winHandle;
			WindowType * winP;
			
			winHandle = WinGetFirstWindow ();
			while (winHandle)
				{
				winP = WinGetWindowPointer (winHandle);
				if (winP->windowFlags.dialog)
					if (CurrentForm == (FormType *) winP)
						{
						exist = true;
						break;
						}
				winHandle = winP->nextWindow;		
				}
			if (! exist)
				ErrDisplay ("Current form does not exist");
			}
	#endif	


	// Release the menu.
	if (CurrentForm)
		{
		menuP = MenuGetActiveMenu ();
		if (menuP)
			MenuDispose (menuP);

		// Before we start calling Field & GSI routines, make sure
		// we're set up to handle drawing calls - for example, MenuDispose
		// currently messes with the active & draw windows.
		
		// The DrawWindow really is wrong.  I think menus mess it up.  Perhaps 
		// menus should set the draw window to the form when disposed? rsf
//		ErrNonFatalDisplayIf(WinGetDrawWindow() != FrmGetWindowHandle (CurrentForm), "Draw window wrong>");
//		ErrNonFatalDisplayIf(WinGetActiveWindow() != FrmGetWindowHandle (CurrentForm), "Draw window wrong>");
		WinSetDrawWindow (FrmGetWindowHandle (CurrentForm));
		WinSetActiveWindow (FrmGetWindowHandle (CurrentForm));
		
		// Release any field object's focus, but don't remove the form's or table's focus.
		// This cancels any partial graffiti shortcut or text conversion.
		// It allows the focus to be restored when the form becomes active again.
		fldP = FrmGetActiveField(CurrentForm);
		if (fldP != NULL)
			{
			FldReleaseFocus(fldP);
			}
		
		GsiEnable (false);
		AttnIndicatorAllow (false);
		}


#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	WinSetDrawWindow (NULL);
	WinSetActiveWindow (NULL);
#else
	WinSetDrawWindow (RootWindow);
	WinSetActiveWindow (RootWindow);
#endif
	
	CurrentForm = formP;
	if (! formP) return;


	ECFrmValidatePtr(formP);
	
	// There is a case when a form is deleted and the one "underneath" 
	// becomes active.  Reset everything that could have been changed 
	// by the deleted form.
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	if (formP->attr.visible)
#endif
		{
		// Make the new current form the draw window and the active window.
		WinSetDrawWindow (FrmGetWindowHandle (formP));
		WinSetActiveWindow (FrmGetWindowHandle (formP));
		}
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	else
		{
		WinSetDrawWindow (NULL);
		WinSetActiveWindow (NULL);
		}
#endif
	
#if 0	/* mchen - removed because it conflicts with clipping region already
		   set in redraw case.  we now assume that clipping region is set for
		   us by caller */
	PrvSetClippingBounds(formP);
#endif	
	
	// If the form is visible draw the graffiti shift state indicator.
	// This happens when returning to a form.
	if (formP->attr.visible)
		{
		// Set up form colors incase any drawing code like WinDrawChars happens.
		PrvSetAppropriateColors(formP, setBack);

		if (formP->attr.graffitiShift)
			{
			for (i = 0; i < formP->numObjects; i++)
				{
				if (formP->objects[i].objectType == frmGraffitiStateObj)
					{
					// Set the position of the graffiti shift state indicator.
					grfStateObj = formP->objects[i].object.grfState;
					GsiSetLocation (grfStateObj->pos.x, grfStateObj->pos.y);
					GsiEnable (true);

					break;
					}
				}
			}
		}

	if (formP->focus == noFocus)
		InsPtEnable (false);

	// Restore the focus in the new current form.
	else if (formP->attr.visible)
		{
		fldP = FrmGetActiveField(formP);
		if (fldP != NULL)
			{
			FldGrabFocus (fldP);
			}
		}
	
	// Restore the attention indicator.
	// If we're returning to a non-modal form rather than another
	// modal dialog, then we need to enable the attention indicator,
	// unless the form we're returning to isn't visible yet, in which case
	// we wait for it to be drawn before turning the indicator back on.
	if (!formP->window.windowFlags.modal && formP->attr.visible && formP->attr.attnIndicator)
		AttnIndicatorAllow(true);

	// Make the form's menu the active menu.
	if (formP->menuRscId)
		{
//		menuP = MenuInit (formP->menuRscId);
		MenuSetActiveMenuRscID (formP->menuRscId);
		}
	else
		MenuSetActiveMenu (NULL);
		
}


/***********************************************************************
 *
 * FUNCTION:    FrmGetControlGroupSelection
 *
 * DESCRIPTION: This routine returns the item number of the selected
 *              control, in a group of controls
 *
 * PARAMETERS:	 formP	 memory block that contains the form.
 *              group    control group number
 *
 * RETURNED:	 item number of selected control or -1 if none
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/16/94		Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *			grant	11/05/99	Change return type to UInt16 (was UInt8)
 *
 ***********************************************************************/
UInt16 FrmGetControlGroupSelection (const FormType * formP, UInt8 group)
{
	ControlType * ctlP;
	Int16 i;


	ECFrmValidatePtr(formP);
	
	for (i = 0; i < formP->numObjects; i++)
		{
		if (formP->objects[i].objectType == frmControlObj)
			{
			ctlP = formP->objects[i].object.control;
			if (ctlP->group == group && ctlP->attr.on)
				return (i);
			}
		}
	return (frmNoSelectedControl);
}


/***********************************************************************
 *
 * FUNCTION:    FrmSetControlGroupSelection
 *
 * DESCRIPTION: This routine set the selected control, in a group of controls
 *
 * PARAMETERS:	 formP	  memory block that contains the form.
 *              group     control group number.
 *              controlID id of control to set.
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/16/94		Initial Revision
 *
 ***********************************************************************/
void FrmSetControlGroupSelection (const FormType * formP, 
	UInt8 group, UInt16 controlID)
{
   UInt16 oldIndex;
   UInt16 newIndex;

	ECFrmValidatePtr(formP);
	
	oldIndex = FrmGetControlGroupSelection (formP, group);
	newIndex = FrmGetObjectIndex (formP, controlID);

	if (oldIndex != newIndex)
		{
		FrmSetControlValue (formP, oldIndex, false);	
		}
	FrmSetControlValue (formP, newIndex, true);
}



/***********************************************************************
 *
 * FUNCTION:    FrmGetControlValue
 *
 * DESCRIPTION: The function return the on / off state of a control.
 *
 * PARAMETERS:	 formP	 memory block that contains the form.
 *              objIndex item number of the object
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/16/94		Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
Int16 FrmGetControlValue (const FormType * formP, UInt16 objIndex)
{
	ECFrmValidatePtr(formP);
	ErrFatalDisplayIf ((objIndex >= formP->numObjects), "Invalid index");

	return (CtlGetValue (FrmGetObjectPtr (formP, objIndex)));
	
}


/***********************************************************************
 *
 * FUNCTION:    FrmSetControlValue
 *
 * DESCRIPTION: The function turns a control on or off.
 *
 * PARAMETERS:	 formP	 memory block that contains the form.
 *              objIndex item number of the object
 *              value	 new control value (non-zero equal on)
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/16/94		Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
void FrmSetControlValue (const FormType * formP, UInt16 objIndex, 
	Int16 value)
{
	ECFrmValidatePtr(formP);
	
	if (objIndex < formP->numObjects)
		CtlSetValue (FrmGetObjectPtr (formP, objIndex), value);
}


/***********************************************************************
 *
 * FUNCTION:    FrmGetGadgetData
 *
 * DESCRIPTION: The function returns the value stored in the "data"
 *              field of the gadget object.  
 *
 *              Gadget objects provide a way for an application to 
 *              attach custom gadgetry to a form.  In general the 
 *              "data" field of a gadget obejct contains a pointer 
 *              to the custom object's data structure. 
 *
 * PARAMETERS:	 formP	 memory block that contains the form.
 *              objIndex item number of the object
 *              data  	 application defined value
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/16/94		Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
void * FrmGetGadgetData (const FormType * formP, UInt16 objIndex)
{
	ECFrmValidatePtr(formP);
	ErrFatalDisplayIf ((objIndex >= formP->numObjects), "Invalid index");
	ErrNonFatalDisplayIf((formP->objects[objIndex].objectType != frmGadgetObj), "Not a gadget");

	// de-const the result for client convenience
	return (void *)(formP->objects[objIndex].object.gadget->data);
}


/***********************************************************************
 *
 * FUNCTION:    FrmSetGadgetData
 *
 * DESCRIPTION: The function store the value passed in the "data"
 *              field of the gadget object.  
 *
 *              Gadget objects provide a way for an application to 
 *              attach custom gadgetry to a form.  In general the 
 *              "data" field of a gadget obejct contains a pointer 
 *              to the custom object's data structure. 
 *
 * PARAMETERS:	 formP	 memory block that contains the form.
 *              objIndex item number of the object
 *              data  	 application defined value
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/16/94		Initial Revision
 *
 ***********************************************************************/
void FrmSetGadgetData (FormType * formP, UInt16 objIndex, const void * data)
{
	ECFrmValidatePtr(formP);
	if (objIndex >= formP->numObjects)
		{
		ErrNonFatalDisplay("Invalid index");
		return;
		}
	ErrNonFatalDisplayIf((formP->objects[objIndex].objectType != frmGadgetObj), "Not a gadget");

	formP->objects[objIndex].object.gadget->data = data;
}


/***********************************************************************
 *
 * FUNCTION:    FrmSetGadgetHandler
 *
 * DESCRIPTION: The function sets the procedure that controls all
 *				gadget behavior for this gadget.  The given procedure
 *				will be called when the gadget needs to be drawn, etc...
 *				
 *              Gadget objects provide a way for an application to 
 *              attach custom gadgetry to a form.  The "handler"
 *              field of a gadget obejct contains a pointer to the
 *              custom object's control procedure.
 *
 * PARAMETERS:	formP		memory block that contains the form.
 *              objIndex	item number of the object
 *              handler		ptr to procedure used to handle gagdet behavior.
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			jesse	8/6/99		Initial Revision
 *
 ***********************************************************************/
void FrmSetGadgetHandler (FormType * formP, UInt16 objIndex, FormGadgetHandlerType *handler)
{
	ECFrmValidatePtr(formP);
	if (objIndex >= formP->numObjects)
		{
		ErrNonFatalDisplay("Invalid index");
		return;
		}
	ErrNonFatalDisplayIf((formP->objects[objIndex].objectType != frmGadgetObj), "Not a gadget");
	if ((formP->objects[objIndex].object.gadget->attr.extended) == false)
		{
		ErrNonFatalDisplay("not extended gadget");
		return;
		}

	formP->objects[objIndex].object.gadget->handler = handler;
}



/***********************************************************************
 *
 * FUNCTION:    FrmPopupList
 *
 * DESCRIPTION: This routine will popup a list object.
 *
 * PARAMETERS:	formP	 memory block that contains the form.
 *             controlID id of the popup trigger that initiated the popup
 *              
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/17/94		Initial Revision
 *			CS		08/15/00	Initialize new event structure.
 *
 ***********************************************************************/
static void FrmPopupList (FormType * formP, EventType * eventP)
{
	Int16				curSelection;
	Int16				newSelection;
	UInt16           i;
	UInt16				controlID;
	UInt16           objIndex;
	UInt16	         numObjects;
	EventType		newEvent;
	ListType *        lstP;
	ControlType *		ctlP;
	FormObjectType obj;

	ECFrmValidatePtr(formP);
	
	ctlP = eventP->data.ctlEnter.pControl;
   controlID = ctlP->id;

	numObjects = formP->numObjects;

	for (i = 0; i < numObjects; i++)
		{
		if (formP->objects[i].objectType == frmPopupObj)
			{
			obj = formP->objects[i].object;
			if (obj.popup->controlID == controlID)
				{
				objIndex = FrmGetObjectIndex (formP, obj.popup->listID);
				lstP = FrmGetObjectPtr (formP, objIndex);
				curSelection = LstGetSelection (lstP);
				newSelection = LstPopupList (lstP);
				if (newSelection != -1)
					{
					MemSet(&newEvent, sizeof(newEvent), 0);	// Initialize new event structure
					newEvent.eType = popSelectEvent;
					newEvent.data.popSelect.controlID = obj.popup->controlID;
					newEvent.data.popSelect.controlP = ctlP;
					newEvent.data.popSelect.listID = obj.popup->listID;
					newEvent.data.popSelect.listP = lstP;
					newEvent.data.popSelect.selection = newSelection;
					newEvent.data.popSelect.priorSelection = curSelection;
					EvtAddEventToQueue (&newEvent);
					}
				}
			}
		}
}


/***********************************************************************
 *
 * FUNCTION: 	 PrvPointInObject
 *
 * DESCRIPTION: This routine returns true if the coordinate passed is within
 *              the bounds of an object of the form.  The item number of the
 *              object is also returned.
 *
 * PARAMETERS:	 formP	 memory block that contains the form.
 *              x        window relitave coordinate
 *              x        window relitave coordinate
 *					pObjIndex pointer to UInt16 
 *
 * RETURNED:	 TRUE if coordinate is an object
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/7/94	Initial Revision
 *			art	2/20/96	Added scroll bar
 *			bob	11/6/00	Reverse loop order so last object drawn is first hit (bug 44855)
 *
 ***********************************************************************/
static Boolean PrvPointInObject (FormType * formP, Coord x, Coord y, 
	UInt16 * pObjIndex)
{
	UInt16 i;
	ListType * lstP;
	FieldType * fldP;
	TableType * tblP;
	FormGadgetType * gadgetP;
	ControlType * ctlP;
	ScrollBarType * scrollBarP;
	RectangleType * rP;
	FormObjectKind objType;
	
	ECFrmValidatePtr(formP);
	
	i = formP->numObjects;
	while (i > 0) {
		i--;
		objType = formP->objects[i].objectType;

		if (objType == frmControlObj)
			{
			ctlP = formP->objects[i].object.control;
			if (ctlP->attr.usable)
				rP = &ctlP->bounds;
			else
				continue;
			}			

		else if (objType == frmListObj)
			{
			lstP = formP->objects[i].object.list;
			if (lstP->attr.usable)
				rP = &lstP->bounds;
			else
				continue;
			}
						
		else if (objType == frmFieldObj)
			{
			fldP = formP->objects[i].object.field;
			if (fldP->attr.usable)
				rP = &fldP->rect;			
			else
				continue;
			}

		else if (objType == frmTableObj)
			{
			tblP = formP->objects[i].object.table;
			if (tblP->attr.usable)
				rP = &tblP->bounds;			
			else
				continue;
			}

		else if (objType == frmGadgetObj)
			{
			gadgetP = formP->objects[i].object.gadget;
			if (gadgetP->attr.extended && gadgetP->attr.usable)
				rP = &gadgetP->rect;			
			else
				continue;
			}

		else if (objType == frmScrollBarObj)
			{
			scrollBarP = formP->objects[i].object.scrollBar;
			if (scrollBarP->attr.usable)
				rP = &scrollBarP->bounds;			
			else
				continue;
			}

		else	
			continue;

		if (RctPtInRectangle (x, y, rP))
			{
			*pObjIndex = i;
			return (true);
			}
	};

	return (false);
}


/***********************************************************************
 *
 * FUNCTION: 	 PrvDoMenuCommand
 *
 * DESCRIPTION: This routine handles system edit menu commands.
 *
 * PARAMETERS:	 formP	  memory block that contains the form.
 *              command  id of a menu item
 *					
 * RETURNED:	 nothing
 *
 * HISTORY:
 *		04/05/95	art	Created by Art Lamb.
 *		02/27/97	scl	Added MenuEraseStatus for international builds
 *		07/21/98	scl	Rewrote menu command status collision detection
 *		08/28/00	kwk	Use new FrmGetActiveField routine.
 *
 ***********************************************************************/
static Boolean PrvDoMenuCommand (FormType * formP, UInt16 command)
{
	FieldType * fldP;
	WinHandle drawWin;
	Coord displayWidth, displayHeight;
	RectangleType r;
	
	ECFrmValidatePtr(formP);
	
	fldP = FrmGetActiveField(formP);
	if (fldP == NULL)
		{
		goto exit;
		}
		
	// Make sure that processing the menu command (which graphically acts
	// upon the active field) doesn't interfere with the menu command bar.
	// If we don't do this, the menu command will overwrite the menu command bar,
	// and when the bar finally goes away, it will restore old (stale) screen bits.
	if (MenuCmdBarCurrent)
		{
		drawWin = WinSetDrawWindow (WinGetDisplayWindow ());
		WinGetDisplayExtent (&displayWidth, &displayHeight);

		r.topLeft.y = MenuCmdBarCurrent->top;
		r.extent.x = displayWidth;						// to be safe, check entire screen width
		r.extent.y = displayHeight - MenuCmdBarCurrent->top;			// height of menu command status
		r.topLeft.x = 0;

		RctGetIntersection (&r, &fldP->rect, &r);	// see if status intersects field
		if ((r.extent.y) && (r.extent.x))
			MenuEraseStatus (0);							// yep -- erase status before proceeding

		WinSetDrawWindow (drawWin);					// cleanup
		}

	switch (command)
		{
		case sysEditMenuUndoCmd:
			FldUndo (fldP);
			return true;
		
		case sysEditMenuCutCmd:
			FldCut (fldP);
			return true;
		
		case sysEditMenuCopyCmd:
			FldCopy (fldP);
			return true;
		
		case sysEditMenuPasteCmd:
			FldPaste (fldP);
			return true;

		case sysEditMenuSelectAllCmd:
			FldSetSelection (fldP, 0, FldGetTextLength (fldP));
			return true;
			
		case sysEditMenuKeyboardCmd:
			SysKeyboardDialog (kbdDefault);
			return true;
			
		case sysEditMenuGraffitiCmd:
			SysGraffitiReferenceDialog (referenceDefault);
			return true;
		}

	return false;

exit:
	SndPlaySystemSound (sndError);
	return false;
}


/***********************************************************************
 *
 * FUNCTION: 	 PrvSendConfirmEvent
 *
 * DESCRIPTION: This routine is called when the "confirm" hard icon
 *              in pressed.  If the form has a default button defined,
 *              this routine simulates the pressing of that button.
 *
 * PARAMETERS:	 formP	  memory block that contains the form.
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/6/95	Initial Revision
 *
 ***********************************************************************/
static void PrvSendConfirmEvent (FormType * formP)
{
	UInt16 index;
	
	ECFrmValidatePtr(formP);
	
	// Exit, if there is not a default button defined.
	if (! formP->defaultButton) return;
	
	index = FrmGetObjectIndex (formP, formP->defaultButton);
	ErrNonFatalDisplayIf ((index == frmInvalidObjectId), "Default button not found");
	
	if (index != frmInvalidObjectId)
		CtlHitControl (FrmGetObjectPtr (formP, index));
}


/***********************************************************************
 *
 * FUNCTION: 	 PrvHelpHandleEvent
 *
 * DESCRIPTION: This routine checks if a pen dowm event has occurred
 *              in the help trigger.  If so, the pen is tracked until
 *              it's release, if it is release on the help icon the 
 *              help dialog is displayed.
 *
 * PARAMETERS:	 formP	  memory block that contains the form.
 *              event     pointer to a pen down event
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/6/95	Initial Revision
 *
 ***********************************************************************/
static Boolean PrvHelpHandleEvent (FormType * formP, EventType * eventP)
{
	Coord x, y;
	Int16 formWidth;
	Boolean penDown;
	Boolean inverted = false;
	RectangleType r;
	
	ECFrmValidatePtr(formP);
	
	if (! formP->helpRscId)
		return (false);
	

	formWidth = formP->window.windowBounds.extent.x;
	r.topLeft.x = formWidth - helpButtonWidth - 2;
	r.topLeft.y = 0;
	r.extent.y = helpButtonWidth;
	r.extent.x = helpButtonWidth;

	x = eventP->screenX;
	y = eventP->screenY;
	if ( ! RctPtInRectangle (x, y, &r))
		return (false);

	// Set to proper color
	WinPushDrawState();
	PrvSetAppropriateColors(formP, setText + setFore + setBack);

	do {
		if (RctPtInRectangle (x, y, &r))
			{
			if (! inverted)
				PrvDrawHelpIcon (formWidth, inverted = true);
			}
		else if (inverted)
			PrvDrawHelpIcon (formWidth, inverted = false);

		PenGetPoint (&x, &y, &penDown);
	} while (penDown);


	if (inverted)
		PrvDrawHelpIcon (formWidth, false);
	
	WinPopDrawState();
	
	if (inverted)
		FrmHelp (formP->helpRscId);

	return (true);
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmCopyLabel
 *
 * DESCRIPTION: This routine copies the passed string into the 
 *              data structure of the specified label object in the active
 *              form.
 *
 * PARAMETERS:	 labelID  - id of form label object
 *              newLable - pointer to a null-terminated string
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/8/95	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
void FrmCopyLabel (FormType * formP, UInt16 labelID, const Char * newLabel)
{
	FontID curFont;
	FormLabelType * obj;
	Int16 oldTextLen;
	
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL

	UInt16 objIndex;
	UInt16 labelLen;

	objIndex = FrmGetObjectIndex (formP, labelID);

	// Check for the all-too-often occurrence of calling
	// this function with a pointer that is not a control.
	// Often clients mis-use this to change text of a control.
	if (objIndex != frmInvalidObjectId)
		{
		if (FrmGetObjectType (formP, objIndex) != frmLabelObj)
			{
			ErrNonFatalDisplay("Not a label object");
			return;
			}
		}

	// A little debugging code to make sure the new label will fit.
	
	if (objIndex < FrmGetNumberOfObjects (formP) - 1)
		{
		labelLen =  ((Char *) FrmGetObjectPtr (formP, objIndex+1)) - 
						((Char *) FrmGetObjectPtr (formP, objIndex)) - 
						sizeof (FormLabelType);
		}
	else
		{
		labelLen = MemPtrSize(formP);
		labelLen -=((Char *) FrmGetObjectPtr (formP, objIndex)) - 
					  ((Char *) formP) - 
					  sizeof (FormLabelType);
		}

	if (labelLen <= StrLen(newLabel))
		ErrDisplay ("Label too long");

#endif

	ECFrmValidatePtr(formP);
	
	obj =  FrmGetObjectPtr (formP, FrmGetObjectIndex (formP, labelID));
	
	// remember old label length, for possible extra erasure
	if ((formP->attr.visible) && (obj->attr.usable))
		{
		curFont = FntSetFont (obj->fontID);
		oldTextLen = FntCharsWidth(obj->text, StrLen(obj->text));
		}
		
	StrCopy (obj->text, newLabel);

	if ((formP->attr.visible) && (obj->attr.usable))
		{
		Int16 newTextLen = FntCharsWidth(newLabel, StrLen(newLabel));
		WinSetTextColor(UIColorGetIndex(UIObjectForeground));
		PrvDrawLabel (obj);
		
		// erase any extra text that's left floating
		if (oldTextLen > newTextLen)
			{
				RectangleType r;
				r.topLeft = obj->pos;
				r.topLeft.x += newTextLen;
				r.extent.y = FntLineHeight();
				r.extent.x = oldTextLen - newTextLen;
				WinEraseRectangle(&r, 0);
			}
		FntSetFont (curFont);
		}
	
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmGetLabel
 *
 * DESCRIPTION: This routine return the text of the specified label object
 *	             in the specified form.
 *
 * PARAMETERS:	 formP	 - memory block that contains the form.
 *              labelID  - id of the label object.
 *					
 * RETURNED:	 pointer to the label string.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/11/95	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
const Char * FrmGetLabel (const FormType * formP, UInt16 labelID)
{
	FormLabelType * obj;

	ECFrmValidatePtr(formP);
	
	ErrFatalDisplayIf (
		FrmGetObjectType (formP, FrmGetObjectIndex (formP, labelID)) != frmLabelObj,
		"Invalid object type");

	obj =  FrmGetObjectPtr (formP, FrmGetObjectIndex (formP, labelID));
	return (obj->text);
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmSetObjectPositon
 *
 * DESCRIPTION: This routine returns the window relative coordinate
 *              of the specified object.
 *
 * PARAMETERS:	 formP	 - memory block that contains the form.
 *              objIndex - item number of the object
 *
 * RETURNED:	 x        - window relative coordinate
 *              y        - window relative coordinate
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/8/95	Initial Revision
 *
 ***********************************************************************/
extern void FrmGetObjectPosition (const FormType * formP, UInt16 objIndex, 
	Coord * x, Coord * y)
{
	FormObjectType obj;

	ECFrmValidatePtr(formP);
	ErrNonFatalDisplayIf (objIndex >= formP->numObjects, "Invalid index");
	
	obj = formP->objects[objIndex].object;

	switch (formP->objects[objIndex].objectType)
		{
			case frmFieldObj:
				*x = obj.field->rect.topLeft.x;
				*y = obj.field->rect.topLeft.y;
				break;

			case frmControlObj:
				*x = obj.control->bounds.topLeft.x;
				*y = obj.control->bounds.topLeft.y;
				break;

			case frmListObj:
				*x = obj.list->bounds.topLeft.x;
				*y = obj.list->bounds.topLeft.y;
				break;

			case frmTableObj:
				*x = obj.table->bounds.topLeft.x;
				*y = obj.table->bounds.topLeft.y;
				break;

			case frmBitmapObj:
				*x = obj.bitmap->pos.x;
				*y = obj.bitmap->pos.y;
				break;

			case frmLabelObj:
				*x = obj.label->pos.x;
				*y = obj.label->pos.y;
				break;

			case frmGraffitiStateObj:
				*x = obj.grfState->pos.x;
				*y = obj.grfState->pos.y;
				break;

			case frmGadgetObj:
				*x = obj.gadget->rect.topLeft.x;
				*y = obj.gadget->rect.topLeft.y;
				break;

			case frmScrollBarObj:
				*x = obj.scrollBar->bounds.topLeft.x;
				*y = obj.scrollBar->bounds.topLeft.y;
				break;

		}
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmSetObjectPosition
 *
 * DESCRIPTION: This routine returns the window relative coordinate
 *              of the specified object.
 *
 * PARAMETERS:	 formP	 - memory block that contains the form.
 *              objIndex - item number of the object
 *          	 x        - window relative coordinate
 *              y        - window relative coordinate
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/8/95	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
extern void FrmSetObjectPosition (FormType * formP, UInt16 objIndex, 
	Coord x, Coord y)
{
	FormObjectType obj;

	ECFrmValidatePtr(formP);
	ErrNonFatalDisplayIf (objIndex >= formP->numObjects, "Invalid index");
	
	obj = formP->objects[objIndex].object;

	switch (formP->objects[objIndex].objectType)
		{
			case frmFieldObj:
				obj.field->rect.topLeft.x = x;
				obj.field->rect.topLeft.y = y;
				break;

			case frmControlObj:
				obj.control->bounds.topLeft.x = x;
				obj.control->bounds.topLeft.y = y;
				break;

			case frmListObj:
				obj.list->bounds.topLeft.x = x;
				obj.list->bounds.topLeft.y = y;
				break;

			case frmTableObj:
				obj.table->bounds.topLeft.x = x;
				obj.table->bounds.topLeft.y = y;
				break;

			case frmBitmapObj:
				obj.bitmap->pos.x = x;
				obj.bitmap->pos.y = y;
				break;

			case frmLabelObj:
				obj.label->pos.x = x;
				obj.label->pos.y = y;
				break;

			case frmGraffitiStateObj:
				obj.grfState->pos.x = x;
				obj.grfState->pos.y = y;
				break;

			case frmGadgetObj:
				obj.gadget->rect.topLeft.x = x;
				obj.gadget->rect.topLeft.y = y;
				break;

			case frmScrollBarObj:
				obj.scrollBar->bounds.topLeft.x = x;
				obj.scrollBar->bounds.topLeft.y = y;
				break;
		}
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmSetObjectBounds
 *
 * DESCRIPTION: This routine set the bounds of the specified form object.
 *              of the specified object.
 *
 * PARAMETERS:	 formP	 - memory block that contains the form.
 *              objIndex - item number of the object
 *          	 bounds   - window relative bounds
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/4/96	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
void FrmSetObjectBounds (FormType * formP, UInt16 objIndex, 
	const RectangleType * bounds)
{
	FormObjectType obj;

	ECFrmValidatePtr(formP);
	ErrNonFatalDisplayIf (objIndex >= formP->numObjects, "Invalid index");
	
	obj = formP->objects[objIndex].object;

	switch (formP->objects[objIndex].objectType)
		{
		case frmFieldObj:
			FldSetBounds (obj.field, bounds);
			break;

		case frmControlObj:
			RctCopyRectangle (bounds, &obj.control->bounds);
			break;

		case frmListObj:
			RctCopyRectangle (bounds, &obj.list->bounds);
			break;

		case frmTableObj:
			TblSetBounds (obj.table, bounds);
			break;

		case frmGadgetObj:
			RctCopyRectangle (bounds, &obj.gadget->rect);
			break;

		case frmScrollBarObj:
			RctCopyRectangle (bounds, &obj.scrollBar->bounds);
			break;

		case frmTitleObj:
			RctCopyRectangle (bounds, &obj.title->rect);
			break;
			
		// gmp: can't really set bounds, but at least set topleft 
		case frmLabelObj:
			obj.label->pos = bounds->topLeft;
			break;

		case frmBitmapObj:
			obj.bitmap->pos = bounds->topLeft;
			break;

		case frmGraffitiStateObj:
			obj.grfState->pos = bounds->topLeft;
			break;

		}

}


/***********************************************************************
 *
 * FUNCTION: 	 FrmSetCategoryLabel
 *
 * DESCRIPTION: This routine sets the category label displayed on the 
 *              title line of a form.
 *
 * PARAMETERS:	 formP	 - memory block that contains the form.
 *              objIndex - item number of the object
 *					 newLabel - pointer to the name of the new category
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/8/95	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
void FrmSetCategoryLabel (const FormType * formP, UInt16 objIndex, Char * newLabel)
{
	FormObjectType obj;
	FontID curFont;
	Int16 len;
	RectangleType r;
	Coord x;

	ECFrmValidatePtr(formP);
	ErrNonFatalDisplayIf (objIndex >= formP->numObjects, "Invalid index");
	
	
	obj = formP->objects[objIndex].object;
	curFont = FntSetFont (obj.label->fontID);

	CategoryTruncateName (newLabel, maxCategoryLabelWidth);

	len = StrLen (newLabel);
	obj.label->text = newLabel;
	
	// Rigth justify the label.
	x = obj.label->pos.x;
	obj.label->pos.x = formP->window.windowBounds.extent.x - 
		FntCharsWidth (newLabel, len);
			
	if (formP->attr.visible)
		{
		// If the new label is shorter then the old label, erase the 
		// vacated region.
		if (x < obj.label->pos.x)
			{
			r.topLeft.x = x;
			r.topLeft.y = obj.label->pos.y;
			r.extent.x = obj.label->pos.x - x;
			r.extent.y = FntLineHeight ();
			WinEraseRectangle (&r, 0);
			}

		FntSetFont (obj.label->fontID);
		RctSetRectangle (&r, obj.label->pos.x-2, obj.label->pos.y,
			2,FntLineHeight ());
		WinEraseRectangle (&r, 0);	
		WinSetTextColor(UIColorGetIndex(UIObjectForeground));
		WinDrawChars (newLabel, len, obj.label->pos.x, obj.label->pos.y);
		}	
	FntSetFont (curFont);
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmGetTitle
 *
 * DESCRIPTION: This routine returns a pointer to the title string of a 
 *              form.
 *
 * PARAMETERS:	 formP	  - memory block that contains the form.
 *					
 * RETURNED:	 pointer to title string
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art		3/17/95		Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *			FPa		11/30/00	Make this function safe it title or newTitle is NULL
 *
 ***********************************************************************/
const Char * FrmGetTitle (const FormType * formP)
{
	UInt16 i;
	Char * str;

	ECFrmValidatePtr(formP);
	
	for (i = 0; i < formP->numObjects; i++)
		{
		if (formP->objects[i].objectType == frmTitleObj)
			{
			str = formP->objects[i].object.title->text;
			
			if ( str == NULL )
				return (NULL);
			
			if (*str)
				return (str);
			else
				return (NULL);
			}
		}
	return (NULL);

}


/***********************************************************************
 *
 * FUNCTION: 	 PrvRedrawTitle
 *
 * DESCRIPTION: This routine draw the title pased.
 *
 * PARAMETERS:	 title    - pointer to the title object
 *              newTitle - pointer to the new title string
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art		8/2/95		Initial Revision
 *			FPa		11/30/00	Make this function safe it title or newTitle is NULL
 *
 ***********************************************************************/
static void PrvRedrawTitle (FormType * formP, FormTitleType * title, const Char * newTitle)
{
	RectangleType r;
	Int16 x;
	Int16 oldWidth;
	Int16 newWidth;
	Char * oldTitle;
	Boolean allowIndicator;

	// Before drawing the new title, make sure the indicator is turned off,
	// so the bits saved under the indicator, which contain the old title,
	// are never restored to the screen.
	AttnIndicatorAllow(false);
	
	WinPushDrawState();
	FntSetFont (titleFont);
	PrvSetAppropriateColors(formP, setText + setFore + setBack);

	oldTitle = title->text;
	
	if ( oldTitle != NULL )
		oldWidth = FntCharsWidth (oldTitle, StrLen (oldTitle));
	else
		oldWidth = 0;
		
	if ( newTitle != NULL )
		newWidth = FntCharsWidth (newTitle, StrLen (newTitle));
	else
		newWidth = 0;

	if (newWidth > oldWidth)
		{
		r.topLeft.x = oldWidth + (titleMarginX + (titleMarginX - 1)) - 2;
		r.topLeft.y = 0;
		r.extent.x = newWidth - oldWidth + 1;
		r.extent.y = titleHeight;
		WinDrawRectangle (&r, 0);
		
		x = r.topLeft.x + r.extent.x;
		WinDrawLine (x, r.topLeft.y+1, x, r.topLeft.y + r.extent.y - 2);
		}

	else if (newWidth < oldWidth)
		{
		r.topLeft.x = newWidth + (titleMarginX + (titleMarginX - 1));
		r.topLeft.y = 0;
		r.extent.x = oldWidth - newWidth;
		r.extent.y = titleHeight - 2;
		WinEraseRectangle (&r, 0);
		WinEraseLine (r.topLeft.x - 1, 0, r.topLeft.x, 0);
		
		r.topLeft.x -= titleMarginX;
		r.extent.x = titleMarginX;
		WinDrawRectangle (&r, 3);
		}

	if ( newTitle != NULL )
		WinDrawInvertedChars (newTitle, StrLen (newTitle), titleMarginX, titleMarginY);

	WinPopDrawState();

	// It's now safe to start blinking again.
	allowIndicator = !formP->window.windowFlags.modal &&
		newWidth + titleMarginX * 2 - 1 >= kAttnIndicatorWidth;
	formP->attr.attnIndicator = allowIndicator;
	if (allowIndicator)
		AttnIndicatorAllow(true);
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmSetTitle
 *
 * DESCRIPTION: This routine set the of the title of a form.  If the form
 *              is visible the new title is drawn.
 *
 * PARAMETERS:	 formP	  - memory block that contains the form.
 *              newTitle - pointer to the new title string
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art		3/17/95		Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
void FrmSetTitle (FormType * formP, Char * newTitle)
{
	UInt16 i;
	FormTitleType * titleObj;
	
	ECFrmValidatePtr(formP);
	
	for (i = 0; i < formP->numObjects; i++)
		{
		if (formP->objects[i].objectType == frmTitleObj)
			{
			titleObj = formP->objects[i].object.title;

			if (! formP->attr.visible)
				{
				titleObj->text = newTitle;
				}
			
			else if (formP->window.windowFlags.modal)
				{
				titleObj->text = newTitle;
				PrvDrawTitle (formP, titleObj);
				}
			else
				{
				PrvRedrawTitle (formP, titleObj, newTitle);
				titleObj->text = newTitle;
				}	
			}
		}

}


/***********************************************************************
 *
 * FUNCTION: 	 FrmCopyTitle
 *
 * DESCRIPTION: This routine copies the title passed over the form's
 *              current title.  If the form is visible the new 
 *              title is drawn.
 *
 * PARAMETERS:	 formP	  - memory block that contains the form.
 *              newTitle - pointer to the new title string
 *					
 * RETURNED:	 nothing
 *
 * HISTORY:
 *		08/02/95	art	Created by Art Lamb.
 *		10/12/00	kwk	Added check for title length > space pre-allocated in form.
 *
 ***********************************************************************/
void FrmCopyTitle (FormType* formP, const Char* newTitle)
{
	UInt16 i;
	FormTitleType * titleObjP;
	
	ECFrmValidatePtr(formP);
	
	for (i = 0; i < formP->numObjects; i++)
		{
		if (formP->objects[i].objectType == frmTitleObj)
			{
			titleObjP = formP->objects[i].object.title;

#if (ERROR_CHECK_LEVEL == ERROR_CHECK_FULL)
			// Make sure the title fits, if the title pointer still
			// references data following this object's structure.
			{
			UInt16 maxTitleSize;
			Char* titleStart = (Char*)FrmGetObjectPtr(formP, i) + sizeof(FormTitleType);
			if (titleObjP->text == titleStart)
				{
				if (i < FrmGetNumberOfObjects(formP) - 1)
					{
					maxTitleSize = (Char*)FrmGetObjectPtr(formP, i+1) - titleStart;
					}
				else
					{
					maxTitleSize = (Char*)formP + MemPtrSize(formP) - titleStart;
					}
				
				if (StrLen(newTitle) >= maxTitleSize)
					{
					ErrDisplay("Title too long");
					}
				}
			}
#endif

			if (! formP->attr.visible)
				{
				StrCopy (titleObjP->text, newTitle);
				}
			
			else if (formP->window.windowFlags.modal)
				{
				StrCopy (titleObjP->text, newTitle);
				PrvDrawTitle (formP, titleObjP);
				}
			else
				{
				PrvRedrawTitle (formP, titleObjP, newTitle);
				StrCopy (titleObjP->text, newTitle);
				}	
			}
		}
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmAlert
 *
 * DESCRIPTION: This routine creates a modal dialog from an alert resource
 *              and displays the dialog until a button in the alert is hit.
 *              The item number of the button that was hit is returned.  
 *              A button's item number is determine by its order in the
 *              alert template, the first button has an item number of
 *              zero.
 *
 * PARAMETERS:	 alertId   - resource id of the alert.
 *					
 * RETURNED:	 the button the was pressed (the first button is zero)
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/22/95	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
UInt16 FrmAlert (UInt16  alertId)
{
	return (FrmCustomAlert (alertId, NULL, NULL, NULL));
}


/***********************************************************************
 *
 * FUNCTION:	PrvConstructCustomAlert
 *
 * DESCRIPTION: This routine creates a modal dialog from an alert resource
 *              for display by FrmCustomAlert.
 *
 *					Up to three strings can be passed to this routine,
 *             they are used to replace the "text replacement variables"
 *             ^1, ^2 and ^3 that are contain in the message string
 *             of the alert.
 *
 *					NOTE: This function assumes enough dynamic memory exists.
 *
 * PARAMETERS:
 *		alertId		-> resource id of the alert.
 *		s1				->	string to replace ^1 (NULL ok)
 *		s2				-> string to replace ^2 (NULL ok)
 *		s3				-> string to replace ^3 (NULL ok)
 *		textEntryFieldChars -> 0 if the form should not include a one-line
 *						text entry field; otherwise, 1 greater than the maxChars
 *						for the field (1 greater for 0-terminator).
 *		alertTypeP	<-	if non-NULL, return type of alert created.
 *					
 * RETURNED:		pointer to the newly-created form struct
 *
 *	HISTORY:
 *		07/27/99	jaq	Pulled out of FrmCustomAlert, added support for string entry.
 *		09/22/99	kwk	Use TxtParamString to create text handle from param strings.
 *
 ***********************************************************************/
#define alertMsgXPos			32
#define alertXMargin			8
#define alertYMargin			7
#define minButtonWidth		36
#define alertButtonMargin	4
#define alertMaxTextLines 	10
#define alertTextLinesForField	2
#define gsiWidth				12
#define frameWidth			0

FormType * PrvConstructCustomAlert (UInt16 alertId, const Char * s1, const Char * s2, 
	const Char * s3, Int16 textEntryFieldChars, UInt16 * alertTypeP)
{
	Int16	alertMaxMsgLines = alertMaxTextLines - (textEntryFieldChars ? alertTextLinesForField : 0);
	AlertTemplateType * alertTempl;
	MemHandle rscHandle;
	MemHandle msgHandle;
	Coord x, y;
	Coord alertHeight;
	Coord displayWidth;
	Coord displayHeight;
	Coord alertMsgYPos;
	UInt16 i;
	Int16 msgHeight;
	FontID curFont;
	Char * titleStrP;
	Char * buttonStrP;
	Char * msg;
	FormType * formP;
	UInt16 bitmapID;
	FieldType * fieldP;
	ControlPtr ctlP;
	Int16	spaceLeft;
	Boolean smallButtons;

	// Get the resource that defines the alert.	
	rscHandle = DmGetResource (alertRscType, alertId);
	alertTempl = (AlertTemplateType *) MemHandleLock(rscHandle);
	
	// Get pointers to the title, message, and first button string in the alert template.
	titleStrP = (Char *) (alertTempl + 1);
	msg = titleStrP + StrLen (titleStrP) + 1;		// skip over title string.
	buttonStrP = msg + StrLen (msg) + 1;			// skip over msg string.
	
	// Create the message text handle via Text Mgr call, which allocates and returns
	// a locked handle
	msgHandle = MemPtrRecoverHandle(TxtParamString(msg, NULL, s1, s2, s3));
	MemHandleUnlock(msgHandle);
	
	// Compute the height the message string.
	WinGetDisplayExtent (&displayWidth, &displayHeight);
	alertMsgYPos = FntLineHeight () + 1 +	// title height
						alertYMargin;				// margin above the messege
	
	formP = FrmNewForm (CustomAlertDialog, titleStrP, 2, 0, displayWidth - 4, displayHeight - 4, true, 
		CustomAlertFirstButton + alertTempl->defaultButton, alertTempl->helpRscID, 0);
	
	// Add the alert icon.
	switch (alertTempl->alertType)
		{
		case informationAlert:
			bitmapID = InformationAlertBitmap;
			break;
		case confirmationAlert:
			bitmapID = ConfirmationAlertBitmap;
			break;
		case warningAlert:
			bitmapID = WarningAlertBitmap;
			break;
		case errorAlert:
			bitmapID = ErrorAlertBitmap;
			break;
		}
	FrmNewBitmap (&formP, CustomAlertBitmap, bitmapID, alertXMargin, alertMsgYPos);
	
	// Add a text field to display the message.  A field is used because it can wrap the text.
	curFont = FntSetFont (boldFont);
	fieldP = FldNewField (&formP, CustomAlertField, alertMsgXPos, alertMsgYPos, displayWidth - alertMsgXPos - 6, 
		FntLineHeight () * alertMaxMsgLines, boldFont, 0, false, false, 
		false, false, leftAlign, false, false, false);
	FldSetTextHandle (fieldP, msgHandle);
	msgHeight = max (FldGetTextHeight (fieldP), (FntLineHeight () * 2));
	fieldP->rect.extent.y = msgHeight;
	
	// Add the text entry field and graffiti indicator
	x = alertButtonMargin + 1;								  // 1 for the frame
	y = alertMsgYPos + msgHeight + alertYMargin + 2;
	FntSetFont (stdFont);
	if (textEntryFieldChars)
		{
		fieldP = FldNewField(&formP, CustomAlertTextEntryField, x,y,displayWidth - ((alertButtonMargin + 1) * 2) - 1, FntLineHeight(),
			stdFont,textEntryFieldChars - 1,true /*editable*/,true/*underlined*/,true/*singleline*/,false,leftAlign,false,false,false);
		
		y += FntLineHeight() + alertYMargin;
		FrmNewGsi(&formP,displayWidth - (gsiWidth + frameWidth + alertButtonMargin),y + alertButtonMargin);
		}
	
	// Add the buttons.
	smallButtons = false;
	for (i = 0; i < alertTempl->numButtons ; i++)
		{
		ctlP = CtlNewControl (&formP, CustomAlertFirstButton + i, buttonCtl, buttonStrP, x, y, 
						0, 0, stdFont, 0, true);
		
		buttonStrP += StrLen(buttonStrP) + sizeOf7BitChar(Char);
		x += ctlP->bounds.extent.x + alertButtonMargin + 2;  // 2 for frames
		if (ctlP->bounds.extent.x < minButtonWidth)
			smallButtons = true; //we may have to come back and enlarge
		}
	
	//Add width to small buttons if there's room
	spaceLeft = displayWidth - (alertButtonMargin + x + frameWidth);
	if (textEntryFieldChars)
		spaceLeft -= gsiWidth;
	if (spaceLeft > 0 && smallButtons)
		{
		Int16	spaceAdded = 0;
		Int16	oldWidth;
		
		for (i = 0; i < alertTempl->numButtons ; i++)
			{
			//Unfortunately, CtlNewControl is rather haphazard about what index it puts controls at
			//so we have to get control by ID, not by index or remembering the pointer from above.
			ctlP = FrmGetObjectPtr(formP, FrmGetObjectIndex(formP,CustomAlertFirstButton + i));
			if (spaceAdded)
				ctlP->bounds.topLeft.x += spaceAdded;
			if (spaceLeft && ctlP->bounds.extent.x < minButtonWidth)
				{
				oldWidth = ctlP->bounds.extent.x;
				ctlP->bounds.extent.x = min(oldWidth + spaceLeft, minButtonWidth);
				spaceAdded += ctlP->bounds.extent.x - oldWidth;
				spaceLeft -= ctlP->bounds.extent.x - oldWidth;
				}
			}
		}
	
	// Resize the form vertically now that we know the message height.
	alertHeight = y + 17;	// 17 = button height + 4 pixels of white space, that is,
									// FntLineHeight(stdFont) + 5, for proper spacing from bottom of form.
	formP->window.windowBounds.topLeft.y = displayHeight - alertHeight - 2;
	formP->window.windowBounds.extent.y = alertHeight;
	
	
	FntSetFont (curFont);						// restore font
	
	//return the alertType to determine the noise to play
	if (alertTypeP)
		*alertTypeP = alertTempl->alertType;
	
	// These can maybe done sooner if the form doesn't depend on strings
	// within these resources.
	MemHandleUnlock(rscHandle);
	
	return formP;
}
	

/***********************************************************************
 *
 * FUNCTION: 	 PrvHandleModalEvent
 *
 * DESCRIPTION: Handle top-level event processing for modal forms
 *	(dialogs, alerts) where a form event handler might be installed.
 *
 * PARAMETERS:
 *	formP		 ->	Ptr to the active form.
 *	eventP		 ->	Ptr to the event to be processed.
 *					
 * RETURNED:
 *	True => the event was handled.
 *
 * HISTORY:
 *	08/03/00	kwk	Created by Ken Krugler.
 *
 ***********************************************************************/
static Boolean PrvHandleModalEvent(FormType* formP, EventType* eventP)
{
	// The first oddity here is that we want to call the event handler
	// _before_ calling SysHandleEvent, in case the app wants to pre-
	// screen some events that the system would otherwise handle (for
	// example, one of the hard keys). Normally the sequence is to
	// call SysHandleEvent, then MenuHandleEvent, then do any app-
	// specific upper-level processing (for things like form open),
	// then call FrmDispatchEvent which will try to call the form
	// event handler, then call FrmHandleEvent. Unfortunately with
	// modal forms, the only way an app can intercept an event is
	// via the form event handler, thus it has to get called before
	// SysHandleEvent.
	
	// But wait! When there's a FEP around, some events such as a
	// linefeed keydown event need to get remapped before the app
	// has a chance to screen it out. So our special hack here is
	// to call TsmHandleEvent first, since it'll only remap events,
	// posting a new event in place of the old one, so it's
	// appropriate to do that before calling the form event handler.
	
	if (TsmHandleEvent((SysEventType*)eventP, true))
		{
		return(true);
		}
	
	if (formP->handler)
		{
		if (formP->handler(eventP))
			{
			return(true);
			}
		}
	
	if (!SysHandleEvent(eventP))
		{
		UInt16 error;
		if (!MenuHandleEvent(NULL, eventP, &error))
			{
			if (!FrmHandleEvent(formP, eventP))
				{
				return(false);
				}
			}
		}
	
	return(true);
} // PrvHandleModalEvent


/***********************************************************************
 *
 * FUNCTION: 	 FrmCustomResponseAlert
 *
 * DESCRIPTION: This routine creates a modal dialog ****with a field for
 *				user entry of data**** from an alert resource
 *              and displays the dialog until a button in the alert is hit.
 *              The item number of the button that was hit is returned.  
 *              A button's item number is determine by its order in the
 *              alert template, the first button has an item number of
 *              zero.
 *
 *				Each time a button is pressed, this function calls its callback
 *				with the button number and the current string. Only if this callback
 *				evaluates to true is the dialog dismissed. Thus, the callback
 *				could check a password and bring up a dialog for failure, and the user
 *				would get a chance to re-enter
 *
 *					Up to three strings can be passed to this routine,
 *             they are used to replace the "text replacement variables"
 *             ^1, ^2 and ^3 that are contain in the message string
 *             of the alert.
 *
 *				After the form is created and made active, a frmResponseCreate dummy button
 *				is sent to the callback. This is the time for it to do 
 *				TsmSetFepMode(NULL, tsmFepModeOff);
 *				if it is a password entry dialog. Before it's destroyed, a frmResponseQuit is sent.
 *
 *					NOTE: This function assumes enough dynamic memory exists.
 *
 * PARAMETERS:	 alertId   - resource id of the alert.
 *              s1        - string to replace ^1 (NULL ok)
 *              s2        - string to replace ^2 (NULL ok)
 *              s3        - string to replace ^3 (NULL ok)
 *				entryStringBuf - on return, the string entered by the user will be copied
 *							into this preallocated buffer. (NULL ok)
 *				entryStringBufLen - the maximum length for user-entered strings, including
 *							terminator.
 *				callback - described above
 *					
 * RETURNED:	 the button that was pressed (the first button is zero)
 *				on appStopEvent, if the default button is invalid (callback returns false),
 *					returns frmResponseQuit.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			jaq	07/27/99	created.
 *			bob	09/22/99	call FrmDeleteForm to stop memory leak.
 *			jaq	09/28/99 pass init and destroy events to callback
 *			RTe	10/12/99 allows NULL callback parameter
 *			gap	10/26/99 Be sure command bar is closed before FrmSaveActiveState is called.
 *			kwk	08/03/00	Call PrvHandleModalEvent to do top-level event processing.
 *
 ***********************************************************************/
UInt16 FrmCustomResponseAlert (UInt16 alertId, const Char * s1, const Char * s2, 
	const Char * s3, Char * entryStringBuf, Int16 entryStringBufLength,
	FormCheckResponseFuncPtr callback)
{
	FormType * formP;
	Char *	verifyPasswordP;
	FieldPtr passFld;
	FormActiveStateType	curFrmState;
	EventType event;
	UInt16 buttonHit;
	
	formP = PrvConstructCustomAlert(alertId,s1,s2,s3,entryStringBufLength,0);
	
	ECFrmValidatePtr(formP);
	
	// Command Bar should always be cleared before opening a dialog
	MenuEraseStatus(0);
	
	FrmSaveActiveState(&curFrmState);		// save active form/window state

	FrmSetActiveForm (formP);
	if (! formP->attr.visible)
		FrmDrawForm (formP);
	FrmSetFocus(formP, FrmGetObjectIndex (formP, CustomAlertTextEntryField));
	passFld =  FrmGetObjectPtr (formP, FrmGetObjectIndex (formP, CustomAlertTextEntryField));
	
	//do init stuff
	//allow the callback to do any setup (e.g. the FEP mode)
	if (callback)
		callback(frmResponseCreate,NULL);

	while (true)
		{
		EvtGetEvent (&event, evtWaitForever);
		
		if (PrvHandleModalEvent(formP, &event))
			{
			continue;
			}
		
		if (event.eType == ctlSelectEvent)
			{
			
			if (((ControlType *)(event.data.ctlSelect.pControl))->style == buttonCtl)
				{
				Char	nullChar = 0;
				buttonHit = event.data.ctlSelect.controlID;
				

				verifyPasswordP = FldGetTextPtr(passFld);
				if (!verifyPasswordP)
					verifyPasswordP = &nullChar;
				if (!callback || callback(buttonHit - CustomAlertFirstButton,verifyPasswordP))
					break;
				else
					{
					// This lets the user easily replace their incorrect
					// password while still letting them see their mistake.
					FldSetSelection(passFld, 0, FldGetTextLength (passFld));
					}
				}
			}


		else if (event.eType == appStopEvent)
			{
			buttonHit = formP->defaultButton;
			
			//Can't just pass the default without running the callback - security problem
			//If (heaven forbid) the callback returns false, the default button is invalid.
			//Then, we will return CustomAlertFirstButton
			
			if (!callback || !callback(buttonHit - CustomAlertFirstButton,verifyPasswordP))
				buttonHit = frmResponseQuit;
			EvtAddEventToQueue (&event);
			break;
			}
		}
	
	//copy return string to entryStringBuf
	if (entryStringBuf)
		{
		
		verifyPasswordP = FldGetTextPtr(passFld);
		if (verifyPasswordP)
			{
			
			int i;
			for (i = 0; (verifyPasswordP[i] != 0) && 
							(i < entryStringBufLength - 1);
				i++)
				{
				entryStringBuf[i] = verifyPasswordP[i];
				}
			entryStringBuf[i] = verifyPasswordP[i];
			}
		else
			entryStringBuf[0] = 0;
		}
	
	//give the callback a chance to cleanup any globals/features it's set up
	if (callback)
		callback(frmResponseQuit,verifyPasswordP);
		
	FrmEraseForm (formP);	
	FrmRestoreActiveState(&curFrmState);		// restore active form/window state
	FrmDeleteForm (formP);

	return (buttonHit - CustomAlertFirstButton);	
}
	
	
/***********************************************************************
 *
 * FUNCTION: 	 FrmCustomAlert
 *
 * DESCRIPTION: This routine creates a modal dialog from an alert resource
 *              and displays the dialog until a button in the alert is hit.
 *              The item number of the button that was hit is returned.  
 *              A button's item number is determine by its order in the
 *              alert template, the first button has an item number of
 *              zero.
 *
 *					Up to three strings can be passed to this routine,
 *             they are used to replace the "text replacement variables"
 *             ^1, ^2 and ^3 that are contain in the message string
 *             of the alert.
 *
 *					NOTE: This function assumes enough dynamic memory exists.
 *
 * PARAMETERS:	 alertId   - resource id of the alert.
 *              s1        - string to replace ^1 (NULL ok)
 *              s2        - string to replace ^2 (NULL ok)
 *              s3        - string to replace ^3 (NULL ok)
 *					
 * RETURNED:	 the button the was pressed (the first button is zero)
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/22/95	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *			rsf	11/25/97	changed to take advantage of the dynamic ui calls
 *								Reserved an id range for the system dialog.
 *			vmk	12/10/97	Use FrmSave/RestoreActiveState to save and restore
 *								active form/window.
 *			dia	07/24/98	Fixed a bug where memory would be trashed when
 *								empty strings were passed in.  Added support for
 *								NULL strings at the same time.  "const Char *"'s
 *								removed.
 *			dia	07/27/98	Cleanup of above code fix.  Re-added const.
 *			jaq	07/27/99	Pulled out PrvConstructCustomAlert.
 *			gap	10/26/99 Be sure command bar is closed before FrmSaveActiveState is called.
 *
 ***********************************************************************/
UInt16 FrmCustomAlert (UInt16 alertId, const Char * s1, const Char * s2, 
	const Char * s3)
{
	FormType * formP;
	FormActiveStateType	curFrmState;
	SndSysBeepType			beep;
	UInt16				alertType;
	UInt16 buttonHit;
	
	formP = PrvConstructCustomAlert(alertId,s1,s2,s3,0,&alertType);

	// Command Bar should always be cleared before opening a dialog
	MenuEraseStatus(0);
	
	// Because the form must be displayed before the sounds are played and FrmDoDialog
	// doesn't have a way for this routine to play the sounds after FrmDoDialog draws
	// the form, we must draw the form here and then play the sounds.
	
	FrmSaveActiveState(&curFrmState);				// save active form/window state
	
	FrmSetActiveForm (formP);      


	// Draw the form 
	FrmDrawForm (formP);

	// Play an alert sound
	beep = (SndSysBeepType)0;
	switch (alertType)
		{
		case informationAlert:
			beep = sndInfo;
			break;
		case confirmationAlert:
			beep = sndConfirmation;
			break;
		case warningAlert:
			beep = sndWarning;
			break;
		case errorAlert:
			beep = sndError;
			break;
		}
	if ( beep )
		SndPlaySystemSound(beep);
		
	
	buttonHit = FrmDoDialog (formP);
	
	FrmDeleteForm (formP);
	
	FrmRestoreActiveState(&curFrmState);				// restore active form/window state
	
	return (buttonHit - CustomAlertFirstButton);
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmDoDialog
 *
 * DESCRIPTION: This routine displays a modal dialog until a button in 
 *              the dilog  is hit. The item number of the button that
 *              was hit is returned.  A button's item number is 
 *              determine by its order in the alert template, 
 *              the first button has an item number of zero.
 *
 * DOLATER ??? - This function should accept a callback to draw to the form.
 *	FrmCustomAlert can use this to play sounds.
 *
 * PARAMETERS:	 formP	  - memory block that contains the form.
 *					
 * RETURNED:	 the id of the button the was pressed
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/22/95	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *			vmk	12/10/97	Use FrmSave/RestoreActiveState to save and restore
 *								active form/window.
 *			gap	10/22/99	Add MenuEraseStatus to be sure cmd bar is cleared 
 *								before a dialog is drawn.
 *			gap	10/26/99 Be sure command bar is closed before FrmSaveActiveState is called.
 *			kwk	08/03/00	Call PrvHandleModalEvent to do top-level event processing.
 *
 ***********************************************************************/
UInt16 FrmDoDialog (FormType * formP)
{
	UInt16 buttonHit;
	EventType event;
	FormActiveStateType	curFrmState;
	
	ECFrmValidatePtr(formP);
	
	// Command Bar should always be cleared before opening a dialog
	MenuEraseStatus(0);

	FrmSaveActiveState(&curFrmState);		// save active form/window state
	
	// open the form
	formP->attr.doingDialog = true;
	FrmSetActiveForm (formP);
	if (! formP->attr.visible)
		FrmDrawForm (formP);
	FrmSetFocus (formP, formP->focus);

	while (true)
		{
		EvtGetEvent (&event, evtWaitForever);
		
		// if form was closed last time through, bail out of here
		// immediately and don't mess with the form state, etc.
		if (formP->attr.exitDialog)
			{
			formP->attr.doingDialog = false;
			EvtAddEventToQueue (&event);
			return (formP->defaultButton);
			}
		
		if (PrvHandleModalEvent(formP, &event))
			{
			continue;
			}
		
		if (event.eType == ctlSelectEvent)
			{
			
			if (((ControlType *)(event.data.ctlSelect.pControl))->style == buttonCtl)
				{
				buttonHit = event.data.ctlSelect.controlID;
				break;
				}
			}

		// if we're switching apps, bail out
		else if (event.eType == appStopEvent)
			{
			buttonHit = formP->defaultButton;
			EvtAddEventToQueue (&event);
			break;
			}
		}

	FrmEraseForm (formP);
	formP->attr.doingDialog = false;
	
	FrmRestoreActiveState(&curFrmState);		// restore active form/window state
	
	return (buttonHit);
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmHelpScroll
 *
 * DESCRIPTION: This routine scrolls the help message in the direction 
 *              specified.
 *
 * PARAMETERS:	 formP       - pointer to the Help dialog
 *              direction - up or down
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/29/95	Initial Revision
 *
 ***********************************************************************/
static void FrmHelpScroll (FormType * formP, WinDirectionType direction)
{
	UInt16 upIndex;
	UInt16 downIndex;
	UInt16 linesToScroll;
	Boolean enableUp;
	Boolean enableDown;
	FieldType * fldP;

	fldP = FrmGetObjectPtr (formP, FrmGetObjectIndex (formP, HelpField));
	linesToScroll = FldGetVisibleLines (fldP) - 1;
	FldScrollField (fldP, linesToScroll, direction);

	enableUp = FldScrollable (fldP, winUp);
	enableDown = FldScrollable (fldP, winDown);
	upIndex = FrmGetObjectIndex (formP, HelpUpButton);
	downIndex = FrmGetObjectIndex (formP, HelpDownButton);

	FrmUpdateScrollers (formP, upIndex, downIndex, enableUp, enableDown);
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmHelp
 *
 * DESCRIPTION: This routine diplays the help message specified until the 
 *              done button, in the help dialog is hit.  The id passed 
 *              is the resource id of a string resource that contains the
 *              help message.
 *
 * PARAMETERS:	 helpMsgId   - resource id of help message string 
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/22/95	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *			vmk	12/10/97	Use FrmSave/RestoreActiveState to save and restore
 *								active form/window.
 *			gap	10/26/99 Be sure command bar is closed before FrmSaveActiveState is called.
 *
 ***********************************************************************/
void FrmHelp (UInt16 helpMsgId)
{
	UInt16 upIndex;
	UInt16 downIndex;
	MemHandle helpStr;
	FormType * formP;
	FieldType * fldP;
	Boolean enableUp;
	Boolean enableDown;
	Boolean done = false;
	EventType event;
	FormActiveStateType	curFrmState;
	

	// Command Bar should always be cleared before opening a dialog
	MenuEraseStatus(0);

	FrmSaveActiveState(&curFrmState);		// save active form/window state

	InsPtEnable (false);

	formP = FrmInitForm (HelpForm);

	helpStr = DmGetResource (strRsc, helpMsgId);
	fldP = FrmGetObjectPtr (formP, FrmGetObjectIndex (formP, HelpField));
	FldSetTextHandle (fldP, helpStr);

	FrmSetActiveForm (formP);
	FrmDrawForm (formP);
	
	upIndex = FrmGetObjectIndex (formP, HelpUpButton);
	downIndex = FrmGetObjectIndex (formP, HelpDownButton);

	enableUp = FldScrollable (fldP, winUp);
	enableDown = FldScrollable (fldP, winDown);
	FrmUpdateScrollers (formP, upIndex, downIndex, enableUp, enableDown);
	
	while (! done)
		{
		EvtGetEvent (&event, evtWaitForever);
		if (! SysHandleEvent (&event))
			FrmHandleEvent (formP, &event);

		if (event.eType == ctlSelectEvent)
			{
			if (event.data.ctlSelect.controlID == HelpDoneButton)
				done = true;
			}
					
		else if (event.eType == ctlRepeatEvent)
			{
			switch (event.data.ctlRepeat.controlID)
				{
				case HelpUpButton:
					FrmHelpScroll (formP, winUp);
					break;
					
				case HelpDownButton:
					FrmHelpScroll (formP, winDown);
					break;
				}
			}


		else if	(	(event.eType == keyDownEvent)
	 				&&	(!TxtCharIsHardKey(	event.data.keyDown.modifiers,
													event.data.keyDown.chr))
					&&	(EvtKeydownIsVirtual(&event)))
			{
			switch (event.data.keyDown.chr)
				{
				case vchrPageUp:
					FrmHelpScroll (formP, winUp);
					break;
					
				case vchrPageDown:
					FrmHelpScroll (formP, winDown);
					break;
				}
			}


		else if (event.eType == appStopEvent)
			{
			EvtAddEventToQueue (&event);
			break;
			}
		}

	FldSetTextHandle (fldP, 0);
	FrmEraseForm (formP);
	FrmDeleteForm (formP);
	
	FrmRestoreActiveState(&curFrmState);		// restore active form/window state

}


/***********************************************************************
 *
 * FUNCTION:    FrmUpdateScrollers
 *
 * DESCRIPTION: This routine draws or erases the field scroll arrow
 *              buttons depending of the contains of a field.
 *
 * PARAMETERS:  formP            - pointer to a form
 *              upIndex        - index of a the up scroller button
 *              downIndex      - index of a the down scroller button
 *              scrollableUp   - true if the up scroll should be active
 *              scrollabledown - true if the down scroll should be active
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/7/95	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *			gap	10/12/99	added safety net to close command bar before
 *								updating scrollers in the event the app did
 *								not already close the command bar.
 *			gap	10/22/99	moved previous change to command bar event handler.
 *			gap	11/24/99	Added a call to MenuEraseStatus to properly handle 
 *								case where scrollers are updated as a result of a cut
 *								or paste with command bar visible.
 *			bob	11/06/00	Take advantage of FrmShowObject/FrmHideObject to simplify
 *								logic and fix bug 43890 
 *
 ***********************************************************************/
void FrmUpdateScrollers (FormType * formP, UInt16 upIndex, 
	UInt16 downIndex, Boolean scrollableUp, Boolean scrollableDown)
{
	Char * scrollerLabel;
	Boolean scrollerVisible;
	Boolean scrollerEnabled;
	ControlType * upCtl;
	ControlType * downCtl;
		
	ECFrmValidatePtr(formP);
	
	// Be sure menu bar is out of the way before modifying the
	// scrollers to be sure they are drawn to/erased from the form
	// not the scroll bar.
	MenuEraseStatus(0);
	
	upCtl = formP->objects[upIndex].object.control;
	downCtl = formP->objects[downIndex].object.control;

	// If not scrollable in either direction,  hide both scrollers.
	if ((! scrollableUp) && (! scrollableDown))
		{
		FrmHideObject(formP, upIndex);
		FrmHideObject(formP, downIndex);
		return;
		}

	// Show the up scroll button with the correct icon.  Enable or 
	// disable as appropriate.
	scrollerVisible = upCtl->attr.usable;
	scrollerEnabled = upCtl->attr.enabled;
	if (scrollableUp != scrollerEnabled)
		{
		scrollerLabel = *&upCtl->text;
		if (scrollableUp)
			*scrollerLabel = symbol7ScrollUp;
		else
			*scrollerLabel = symbol7ScrollUpDisabled;
		CtlSetEnabled (upCtl, scrollableUp);
		}

	if ( (! scrollerVisible) || (scrollableUp != scrollerEnabled))
		FrmShowObject (formP, upIndex);


	// Show the down scroll button with the correct icon.  Enable or 
	// disable as appropriate.
	scrollerVisible = downCtl->attr.usable;
	scrollerEnabled = downCtl->attr.enabled;
	if (scrollableDown != scrollerEnabled)
		{
		scrollerLabel = *&downCtl->text;
		if (scrollableDown)
			*scrollerLabel = symbol7ScrollDown;
		else
			*scrollerLabel = symbol7ScrollDownDisabled;
		CtlSetEnabled (downCtl, scrollableDown);
		}

	if ( (! scrollerVisible) || (scrollableDown != scrollerEnabled))
		FrmShowObject (formP, downIndex);
}


/***********************************************************************
 *
 * FUNCTION:    FrmVisible
 *
 * DESCRIPTION: This routine returns true if the form is visible.
 *
 * PARAMETERS:  formP - pointer to a form
 *
 * RETURNED:    true or false
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/24/95	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
Boolean FrmVisible (const FormType * formP)
{
	ECFrmValidatePtr(formP);
	
	return (formP->attr.visible);
}


/***********************************************************************
 *
 * FUNCTION:    FrmSetEventHandler
 *
 * DESCRIPTION: This routine set the event handle callback routine for
 *              the specificed form.  FrmHandleEvent will call this handler
 *              whenever it receives an event.
 *
 *              This routine should be call right after a form resource 
 *              is loaded.  The callback routine is the mechanics for 
 *              dispatching events to an application.
 *              
 *
 * PARAMETERS:  handler - address of a function
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	9/11/95	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
void FrmSetEventHandler (FormType * formP, FormEventHandlerType * handler)
{
	ECFrmValidatePtr(formP);
	
	ErrNonFatalDisplayIf (handler == NULL, "NULL event handler");
	
	formP->handler = handler;
}


/***********************************************************************
 *
 * FUNCTION:    FrmDispatchEvent
 *
 * DESCRIPTION: This routine dispatches an event to the application's 
 *              handler for the form.
 *              
 * PARAMETERS: eventP - pointer to an event.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	9/11/95	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *			bob	09/07/99	get rid of assert that form has a handler
 *			jmp	09/15/99	To "fix" bug #21922, prevent register A4 from
 *								from being used to jsr to the form handler
 *								routine by calling out to another routine first!
 *
 ***********************************************************************/
Boolean FrmDispatchEvent (EventType * eventP)
{
	FormType * formP;

	// First, determine the type of form we have based on the event.
	// If we can't do that, then just default to the currently set
	// form.  If we couldn't get a form at all, then just leave
	// saying that we didn't handle the event.
	
	if (eventP->eType == frmOpenEvent)
		formP = FrmGetFormPtr (eventP->data.frmOpen.formID);

	else if (eventP->eType == frmGotoEvent)
		formP = FrmGetFormPtr (eventP->data.frmGoto.formID);

	else
		formP = CurrentForm;

	if (!formP)
		return (false);
	
	// If we have a form, just send the event to it now.

	return (PrvSendEventToForm (formP, eventP));
}



/***********************************************************************
 *
 * FUNCTION:    FrmBroadcastEvent
 *
 * DESCRIPTION: This routine sends the passed event to all open
 *              forms.  Events are not posted to the event queue, the 
 *              form's event handlers are called directly.
 *
 * PARAMETERS:  eventP - pointer to the event to broadcast.
 * 				 formIDP - pointer to a formID in the event needing to be set.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	9/11/95	Initial Revision
 *			roger	9/21/99	Update so FrmCloseAllForms can use it
 *								Fix FrmHandleEvent not getting called as default
 *								Fixup data segment register
 *
 ***********************************************************************/
static void FrmBroadcastEvent (EventType * eventP, UInt16 *formIDP)
{
	WinHandle firstWinH, winHandle;
	WindowType * winP;
	FormType * formP;
	WinHandle next;	// remember the next window incase a frmCloseEvent deletes the form
	MemPtr currentEnvironmentP;
	RectangleType saveClipR;
	RectangleType zeroClipR = {{0,0},{0,0}};
	
	
	// Save the current app environment.
	currentEnvironmentP = (MemPtr) SysSetA5(0);
	SysSetA5((UInt32) currentEnvironmentP);
	
	firstWinH = WinGetFirstWindow ();
	winHandle = firstWinH;
	while (winHandle)
		{
		winP = WinGetWindowPointer (winHandle);
		next = winP->nextWindow;
		if (winP->windowFlags.dialog)
			{
			formP = (FormType *) winP;
			
			// Set any formID in the event.
			if (formIDP != NULL)
				*formIDP = formP->formId;
			
			// Mangle the segment register to either provide globals or not.
			// The problem is that when a sublaunched app is active, the segment 
			// register can be protected.  Forms needing redrawing may need access to 
			// their globals.  The preferred method is to store environment information
			// in the form, assuming sublaunched apps are good things.  Maintaining 
			// data compatibility with old forms means we currently do the less preferred
			// way.
			if (formP->attr.globalsAvailable)
				SysSetA5((UInt32) PrvUnprotectSegmentRegister(currentEnvironmentP));
			else
				SysSetA5((UInt32) PrvProtectSegmentRegister(currentEnvironmentP));
			// For a broadcast, we set clipping region to 0 for any form that isn't
			// the topmost before calling FrmSetActiveForm() so that the Form doesn't
			// visually update (otherwise it might write over a form that is actually
			// above it).
			if ((winHandle != firstWinH) && formP->attr.visible) {
				WinSetDrawWindow(winHandle);
				WinGetClip(&saveClipR);
				WinSetClip(&zeroClipR);
				}

			// Make each form the active form so screen draws and object references work.
			FrmSetActiveForm(formP);

			// Send the event			
			PrvSendEventToForm (formP, eventP);

			// restore the clipping rectangle, but check that the form 
			// hasn't been deleted
			if ((CurrentForm == formP) &&
				 (winHandle != firstWinH) && (formP->attr.visible)) {
				// Restore clipping rectangle
				WinSetDrawWindow(winHandle);
				WinSetClip(&saveClipR);
				}

			// Restore the current environment.
			SysSetA5((UInt32) currentEnvironmentP);
			}
		winHandle = next;		
		}
}


/***********************************************************************
 *
 * FUNCTION:    FrmPopupForm
 *
 * DESCRIPTION: This routine sends a frmOpen event to the specified
 *              form.  This routine differs from GoToForm in that
 *              the current form is not closed.
 *
 * PARAMETERS:  formID  - resource id of form to open
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/21/95	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
void FrmPopupForm (UInt16 formId)
{
	EventType event;

	MemSet (&event, sizeof(EventType), 0);

	event.eType = frmLoadEvent;
	event.data.frmLoad.formID = formId;
	EvtAddEventToQueue (&event);
 
	event.eType = frmOpenEvent;
	event.data.frmOpen.formID = formId;
	EvtAddEventToQueue (&event);
}


/***********************************************************************
 *
 * FUNCTION:    FrmGotoForm
 *
 * DESCRIPTION: This routine sends a frmClose event to the current 
 *              form and sends a frmOpen event to the specified form.
 *              The form event handler (FrmHandleEvent) will erase and
 *              dispose of a form when it receives a frmColse event.
 *
 * PARAMETERS:  formId - id of the form to display
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/21/95		Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
void FrmGotoForm (UInt16 formId)
{
	UInt16 curFormId;
	EventType event;

	MemSet (&event, sizeof(EventType), 0);

	curFormId = FrmGetActiveFormID ();
	if (curFormId)
		{
		event.eType = frmCloseEvent;
		event.data.frmClose.formID = curFormId;
		EvtAddEventToQueue (&event);
		}

	event.eType = frmLoadEvent;
	event.data.frmLoad.formID = formId;
	EvtAddEventToQueue (&event);
 
 	event.eType = frmOpenEvent;
	event.data.frmOpen.formID = formId;
	EvtAddEventToQueue (&event);
}


/***********************************************************************
 *
 * FUNCTION:    FrmUpdateForm
 *
 * DESCRIPTION: This routine sends a frmUpdate event to the specified
 *              form.  
 *
 * PARAMETERS:  formID  - resource id of form to open
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/23/95	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
void FrmUpdateForm (UInt16 formId, UInt16 updateCode)
{
	EventType event;

	MemSet (&event, sizeof(EventType), 0);
	event.eType = frmUpdateEvent;
	event.data.frmUpdate.formID = formId;
	event.data.frmUpdate.updateCode = updateCode;
	EvtAddEventToQueue (&event);
}


/***********************************************************************
 *
 * FUNCTION:    FrmReturnToForm
 *
 * DESCRIPTION: This routine erases and deletes the currently active 
 *              form and make the form specified the active form.
 *              It is assumed that the form that we're returning to
 *              is already loaded into memory.  Passing a form id of
 *              zero will return to the first form in the wiwndow list,  
 *              which is the last form to be loaded.
 *
 * PARAMETERS:  formID  - resource id of form to return to 
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	97/95		Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
void FrmReturnToForm  (UInt16 formId)
{
	FormType * formP;

	formP = FrmGetActiveForm();
	MenuEraseStatus (0);
	FrmEraseForm (formP);
	FrmDeleteForm (formP);
	
	// If no form id was specified get the id of the first form in the 
	// wiwndow list,  this will be the last form loaded.
	if (formId == 0)
		{
		formP = FrmGetFirstForm ();
		ErrFatalDisplayIf ( ! formP, "No form to return to");
		FrmSetActiveForm (formP);
		}
	else
		FrmSetActiveForm (FrmGetFormPtr (formId));
}


/***********************************************************************
 *
 * FUNCTION:    FrmCloseAllForms
 *
 * DESCRIPTION: This routine sends a frmClose event to all open forms. 
 * All calls to FrmEraseForm do not draw and instead the entire screen is 
 * cleared at the end.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	9/11/95	Initial Revision
 *			roger	9/21/99	Use FrmBroadcastEvent.  
 *								Make RootWindow active instead of NULL (because it is).
 *			roger	9/27/99	Added AllFormsClosing mode. 
 *
 ***********************************************************************/
void FrmCloseAllForms (void)
{
	EventType event;
	
	
	// Set the AllFormsClosing flag.  This causes FrmEraseForm to not draw to 
	// the screen.  This optimizes drawing from O(n^2) (redraw mode) or O(n) 
	// (saved bits mode) to O(1).  It also make application code structure easier.
	// By suspending redraws, we avoid the case where a main dialog contains information
	// that a sub dialog destroys, erases itself, which forces the main dialog to 
	// redraw, but now that information is destroyed.  And worse, that main dialog is
	// about to get closed too.
	AllFormsClosing = true;
	
	MemSet (&event, sizeof(event), 0);
	event.eType = frmCloseEvent;
	FrmBroadcastEvent(&event, &event.data.frmClose.formID);
	
	AllFormsClosing = false;
	
	FrmSetActiveForm(NULL);
		
	// None of the forms were erased because AllFormsClosing was set.
	// Clear the screen by drawing the root window.  We could potentially delay
	// this draw until FrmDrawForm, but it might also cause problems with apps that 
	// draw without forms existing.
	WinSetDrawWindow (RootWindow);
	WinPushDrawState();
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	// debug ROMs only, erase screen to funny color to help detect
	// apps that don't draw right.
	WinSetBackColor(UIColorGetIndex(UICaution));
#else
	WinSetBackColor(UIColorGetIndex(UIFormFill));
#endif
	WinEraseWindow();
	WinPopDrawState();
}


/***********************************************************************
 *
 * FUNCTION:    FrmSaveAllForms
 *
 * DESCRIPTION: This routine sends a frmSave event to all open forms.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	9/27/95	Initial Revision
 *			roger	9/21/99	Use FrmBroadcastEvent
 *
 ***********************************************************************/
void FrmSaveAllForms (void)
{
	EventType event;
	FormType * curForm;
	
	curForm = CurrentForm;
	
	MemSet (&event, sizeof(event), 0);
	event.eType = frmSaveEvent;
	FrmBroadcastEvent(&event, NULL);
		
	FrmSetActiveForm(curForm);
}


/***********************************************************************
 *
 * FUNCTION:    PrvDismissForms
 *
 * DESCRIPTION: This routine sends a frmClose event to all open forms 
 *              that have the dismissable attribute set.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	10/11/95	Initial Revision
 *
 ***********************************************************************/
#ifdef newcode
static void PrvDismissForms (void)
{
	WindowType * winP;
	FormType * formP;
	EventType event;
	WinHandle next;
	WinHandle winHandle;
	WinHandle curDrawWindow;
	FormType * curForm;

	curDrawWindow = WinGetDrawWindow ();
	curForm = CurrentForm;
	
	// Search the window list for forms.
	winHandle = WinGetFirstWindow ();
	while (winHandle)
		{
		winP = WinGetWindowPointer (winHandle);
		next = winP->nextWindow;
		if (winP->windowFlags.dialog)
			{
			formP = (FormType *) winP;
			if (formP->attr.dismissable)
				{
				if (curForm == formP)
					{
					curForm = 0;
					curDrawWindow = WinGetDisplayWindow ();
					}
		
				MemSet (&event, sizeof(EventType), 0);
				event.eType = frmCloseEvent;
				event.data.frmClose.formID = formP->formId;
				PrvSendEventToForm (formP, &event);
				}
			}
		winHandle = next;	
		}
		
	CurrentForm = curForm;
	WinSetDrawWindow (curDrawWindow);
}
#endif


/***********************************************************************
 *
 * FUNCTION: 	 FrmPointInTitle
 *
 * DESCRIPTION: This routine returns true if the coordinate passed is within
 *              the bounds of form's title
 *
 * PARAMETERS:	 formP	 memory block that contains the form.
 *              x        window relitave coordinate
 *              x        window relitave coordinate
 *
 * RETURNED:	 TRUE if coordinate is in the form's title
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	5/14/96	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
Boolean FrmPointInTitle (const FormType * formP, Int16 x, Int16 y)
{
	UInt16 i;
	Int16 displayWidth;
	Int16 displayHeight;	
	Char * str;
	FontID currFont;
	RectangleType r;

	ECFrmValidatePtr(formP);
	
	for (i = 0; i < formP->numObjects; i++)
		{
		if (formP->objects[i].objectType == frmTitleObj)
			{
			str = formP->objects[i].object.title->text;
			if (! (*str))
				return (false);

			if (formP->window.windowFlags.modal)
				{
				WinGetWindowExtent (&displayWidth, &displayHeight);
				r.topLeft.x = 0;
				r.topLeft.y = 1;
				r.extent.x = displayWidth;
				r.extent.y = FntLineHeight ();
				}
			else
				{
				currFont = FntSetFont (titleFont);
				r.topLeft.x = 0;
				r.topLeft.y = 0;
				r.extent.x = FntCharsWidth (str, StrLen(str)) + (titleMarginX + (titleMarginX - 1));
				r.extent.y = titleHeight;
				FntSetFont (currFont);
				}
				
			return (RctPtInRectangle (x, y, &r));				
			}
		}
	return (false);
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmPointInIndicator
 *
 * DESCRIPTION: This routine returns true if the coordinate passed is within
 *              the bounds of attention indicator
 *
 * PARAMETERS:	 formP	 memory block that contains the form.
 *              x        window relitave coordinate
 *              x        window relitave coordinate
 *
 * RETURNED:	 TRUE if coordinate is in the attention indicator
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	7/19/00	Initial Revision
 *
 ***********************************************************************/
static Boolean FrmPointInIndicator(const FormType * formP, Int16 x, Int16 y)
//DOLATER - peter: Rename with Prv prefix or make public.
{
	RectangleType r;

	if (!AttnIndicatorGetBlinkPattern() != kAttnIndicatorBlinkPatternNone ||
		!AttnIndicatorAllowed() ||
		!AttnIndicatorEnabled())
	{
		return false;
	}
	
	RctSetRectangle(&r, kAttnIndicatorLeft, kAttnIndicatorTop,
		kAttnIndicatorWidth, kAttnIndicatorHeight);
	return RctPtInRectangle(x, y, &r);
}


/***********************************************************************
 *
 * FUNCTION: 	 PrvSelectFormTitle
 *
 * DESCRIPTION: This routine is called whwn the form receives a 
 *              frmTitleEnter event.  It tracks the pen until the pen
 *              is released.  A frmTitleSelect event is sent if the pen
 *              is released within the bounds of the title.
 *
 * PARAMETERS:	 formP	memory block that contains the form.
 *              eventP  frmTitleEnter event
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	7/22/96	Initial Revision
 *
 ***********************************************************************/
static void PrvSelectFormTitle (FormType * formP, EventType * eventP)
{
	Coord x, y;
	Boolean inTitle;
	Boolean penDown;
	EventType event;

	ECFrmValidatePtr(formP);
	
	x = eventP->screenX;
	y = eventP->screenY;

	do 
		{
		//some handlers (datebook) for formEnterEvent change the form's
		//title. Thus, we have to check here not only if the pen is in the
		//form's current title, but also set inTitle true if the pen has
		// not moved too far right (even though the title's shrunk from under
		//it)
		inTitle = FrmPointInTitle (formP, x, y) || 
			((x <= (eventP->screenX + 1)) && (y <= titleHeight));
		PenGetPoint (&x, &y, &penDown);
		} while (penDown);


	if (inTitle)
		{
		MemSet (&event, sizeof(EventType), 0);
		event.eType = frmTitleSelectEvent;
		event.screenX = x;
		event.screenY = y;
		event.data.frmTitleSelect.formID = formP->formId;
		EvtAddEventToQueue (&event);
		}
}


/***********************************************************************
 *
 * FUNCTION: 	 PrvSelectIndicator
 *
 * DESCRIPTION: This routine is called whwn the form receives an
 *              attnIndicatorEnter event.  It tracks the pen until the pen
 *              is released.  An attnIndicatorSelect event is sent if the pen
 *              is released within the bounds of the indicator.
 *
 * PARAMETERS:	 formP	memory block that contains the form.
 *              eventP  frmTitleEnter event
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	10/17/00	Initial Revision
 *
 ***********************************************************************/
static void PrvSelectIndicator (FormType * formP, EventType * eventP)
{
	Coord x, y;
	Boolean inIndicator;
	Boolean penDown;
	EventType event;

	ECFrmValidatePtr(formP);
	
	x = eventP->screenX;
	y = eventP->screenY;

	do 
		{
		inIndicator = FrmPointInIndicator (formP, x, y);
		PenGetPoint (&x, &y, &penDown);
		} while (penDown);

	if (inIndicator)
		{
		MemSet (&event, sizeof(EventType), 0);
		event.eType = attnIndicatorSelectEvent;
		event.screenX = x;
		event.screenY = y;
		event.data.attnIndicatorSelect.formID = formP->formId;
		EvtAddEventToQueue (&event);
		
		SndPlaySystemSound(sndClick);
		}
}


/***********************************************************************
 *
 * FUNCTION: 	 FrmSetMenu
 *
 * DESCRIPTION: This routine changes a form's menu bar. 
 *
 * PARAMETERS:	 formP     	memory block that contains the form.
 *              menuRscID  resource id of the menu
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	7/29/96	Initial Revision
 *			trev	08/11/97	mode non modified passed variables constant
 *
 ***********************************************************************/
void FrmSetMenu (FormType * formP, UInt16 menuRscID)
{
	MenuBarType * menuP;

	ECFrmValidatePtr(formP);
	
	formP->menuRscId = menuRscID;

	if (CurrentForm == formP)
		{
		menuP = MenuGetActiveMenu ();
		if (menuP)
			MenuDispose (menuP);

		// Make the form's menu the active menu.
		if (formP->menuRscId)
			{
			//menuP = MenuInit (formP->menuRscId);
			MenuSetActiveMenuRscID (formP->menuRscId);
			}
		}
}


/***********************************************************************
 *
 * FUNCTION:    FrmValidatePtr
 *
 * DESCRIPTION: Validate a form pointer. 
 *
 * PARAMETERS:  formP  pointer to a form
 *
 * RETURNED:    true if the pointer is ok
 *
 * HISTORY:
 *		11/19/97	roger	Created by Roger Flores.
 *		02/06/98	roger	Removed the check that the object list follows the form 
 *							structure and weakened the size checks.
 *							Added additional checks
 *		10/12/00	kwk	Add checks for form text (title, label, control label)
 *							exceeding bounds of pre-allocated space in form.
 *
 ***********************************************************************/
Boolean FrmValidatePtr (const FormType * formP)
{
	WinHandle windowInList;
	WindowType *    winP;
	
	
	// minor change, don't proceed with rest of checks if we know it's a bad
	// form, this allows you to 'continue' through this error to simulate
	// running on a release ROM.
	if (formP == NULL) {
		ErrNonFatalDisplay("NULL form");
		return false;
	}
	

	// If the object list immeadiately follows the form make sure the chunk size is as big
	// as the size of a form plus the size of object list.
	if ((UInt8 *) formP->objects == (UInt8 *) (formP + 1) &&
		MemPtrSize((MemPtr)formP) < sizeof(FormType) + sizeof(FormObjListType) * formP->numObjects)
		goto badWindow;
		
	// Make sure the size of the chunk is the size of a form object.
	else if (MemPtrSize((MemPtr)formP) < sizeof(FormType))
		goto badWindow;
	
	if (formP->attr.reserved)
		goto badWindow;
		
	if (formP->attr.visible && !formP->attr.usable)
		goto badWindow;
	
//	if (formP->focus != noFocus && formP->focus > formP->numObjects)
//		goto badWindow;
#if 0
	if (formP->focus != noFocus)
		{
		FormObjectKind oldFocusType;
		oldFocusType = formP->objects[formP->focus].objectType;

		if (oldFocusType == frmFieldObj)
			{
			FieldType * fldP;
			fldP = formP->objects[formP->focus].object.field;
			ErrNonFatalDisplayIf(!fldP->attr.hasFocus, "FldReleaseFocus called instead of FrmSetFocus(noFocus).");
			}

		else if (oldFocusType == frmTableObj)
			{
			TableType * tableP;
			
			tableP = formP->objects[formP->focus].object.table;
			ErrNonFatalDisplayIf(!formP->objects[formP->focus].object.table->attr.editable, 
				"TblReleaseFocus called instead of FrmSetFocus(noFocus).");
			}
		}
#endif
	
	// Verify that all of the embedded strings (titles, text labels, control labels)
	// fit into their assigned slots and don't overrun following objects.
	{
	UInt16 objIndex;
	for (objIndex = 0; objIndex < formP->numObjects; objIndex++)
		{
		UInt16 structSize;
		Char* textP = NULL;
		FormObjectKind objKind = formP->objects[objIndex].objectType;
		
		if (objKind == frmTitleObj)
			{
			FormTitleType* titleObjP = formP->objects[objIndex].object.title;
			structSize = sizeof(FormTitleType);
			textP = titleObjP->text;
			}
		else if (objKind == frmLabelObj)
			{
			FormLabelType* labelObjP = formP->objects[objIndex].object.label;
			structSize = sizeof(FormLabelType);
			textP = labelObjP->text;
			}
		else if (objKind == frmControlObj)
			{
			ControlType* ctlObjP = formP->objects[objIndex].object.control;
			structSize = sizeof(ControlType);
			textP = ctlObjP->text;
			}
		
		// Before checking the text ptr, make sure it points to immediately after
		// the object structure.
		if ((textP != NULL)
		 && (textP == ((Char*)formP->objects[objIndex].object.ptr + structSize)))
			{
			UInt16 maxTextLen;
			if (objIndex < formP->numObjects - 1)
				{
				maxTextLen = (Char*)formP->objects[objIndex+1].object.ptr - textP;
				}
			else
				{
				maxTextLen = (Char*)formP + MemPtrSize((MemPtr)formP) - textP;
				}
			
			if (StrLen(textP) >= maxTextLen)
				{
				Char msg[60];
				StrPrintF(msg, "Text for object #%d in form #%d is too long", objIndex, formP->formId);
				ErrDisplay(msg);
				return(false);
				}
			}
		}
	}
	
	// would be nice to verify that formP->defaultButton is valid too
	
	// Verify the bits saved window if it exists.
	if (formP->bitsBehindForm != NULL)
		WinValidateHandle(formP->bitsBehindForm);
	
	// This last check looks for the window in the window list.  If 
	// found then this window seems valid.  If not, then it's a badWindow.
	windowInList = WinGetFirstWindow ();
	while (windowInList)
		{
		if (windowInList == WinGetWindowHandle(formP))
			break;
		
		winP = WinGetWindowPointer (windowInList);
		windowInList = winP->nextWindow;
		}
	if (windowInList != WinGetWindowHandle(formP))
		goto badWindow;
	 

	return WinValidateHandle(WinGetWindowHandle(formP));

badWindow:
	ErrNonFatalDisplay("Bad form");
	return false;
}


/***********************************************************************
 *
 * FUNCTION:    FrmNewForm
 *
 * DESCRIPTION: Create a form. 
 *
 * PARAMETERS:  formID - ID of the new form
 *					 titleStrP - the title
 *					 x - the left position of the form
 *					 y - the top position of the form
 *					 width - the width
 *					 height - the height
 *					 modal - true if the dialog is modal
 *					 defaultButton - the default button
 *					 helpRscID - the help string resource
 *					 menuRscID - the menu resource
 *
 * RETURNED:    pointer to a form or NULL for an error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/19/97	Initial Revision
 *			roger	4/25/00	Fix to set globalsAvailable.
 *			kwk	11/06/00	Even if there's no title, still set up formP->objects
 *								to point to byte following form header.
 *
 ***********************************************************************/
FormType * FrmNewForm (UInt16 formID, const Char * titleStrP, 
	Coord x, Coord y, Coord width, Coord height, Boolean modal, 
	UInt16 defaultButton, UInt16 helpRscID, UInt16 menuRscID)
{
	MemHandle formH;
	FormType * formP;
	UInt16 size;
	UInt16 numObjects;
	FormTitleType *titleP;
	Coord	dispHeight, dispWidth;
	MemPtr currentEnvironmentP;

	size = sizeof (FormType);
	
	if (titleStrP)
		{
		numObjects = 1;
		size += sizeof (FormObjListType) * (numObjects) + sizeof (FormTitleType) + 
			StrLen(titleStrP) + sizeOf7BitChar(Char);
		}
	else
		numObjects = 0;
	
	formH = MemHandleNew(size);
	if (formH == NULL)
		return 0;
	
	formP = (FormType *) MemHandleLock(formH);
	MemSet (formP, size, 0);

	WinGetDisplayExtent(&dispWidth, &dispHeight);
		
	if (modal)
		{
		formP->window.windowFlags.modal = modal;
		formP->window.frameType.word = dialogFrame;
		}
	else
		{
		formP->window.frameType.word = noFrame;
		ErrNonFatalDisplayIf(height != dispHeight, "Non modal dialogs must be fullscreen");
		ErrNonFatalDisplayIf(helpRscID, "Only modal dialogs support help");
		}
	
	// Initialize the window info
	width += 2 * formP->window.frameType.bits.width;
	height += 2 * formP->window.frameType.bits.width;
	ErrNonFatalDisplayIf(x > dispWidth, "form x off screen");
	ErrNonFatalDisplayIf(y > dispHeight, "form y off screen");
	ErrNonFatalDisplayIf(width > dispWidth, "form width larger than screen");
	ErrNonFatalDisplayIf(height > dispHeight, "form height larger than screen");
	formP->window.windowBounds.extent.x = width - (formP->window.frameType.bits.width * 2);
	formP->window.windowBounds.extent.y = height - (formP->window.frameType.bits.width * 2);
	formP->window.windowBounds.topLeft.x = x;
	formP->window.windowBounds.topLeft.y = y;
	formP->window.windowFlags.dialog = true;		// part of a form
	formP->window.windowFlags.focusable = true;
	
	
	formP->attr.usable = true;
	formP->attr.saveBehind = true;
	formP->attr.attnIndicator = false;
	
	// Determine if globals should be available for this form.  We do this for the 
	// event handling function to make sure it can access global variables.
	currentEnvironmentP = (MemPtr) SysSetA5(0);
	SysSetA5((UInt32) currentEnvironmentP);
	formP->attr.globalsAvailable = !PrvIsSegmentRegisterProtected(currentEnvironmentP);
	
	
	formP->formId = formID;
	formP->helpRscId = helpRscID;
	formP->menuRscId = menuRscID;
	formP->defaultButton = defaultButton;
	formP->focus = noFocus;
	
	// Always set up the objects pointer, even if we have no title, so that
	// other "add an object to the form" code doesn't try to pass NULL to
	// MemMove.
	formP->objects = (FormObjListType *) (formP + 1);
	
	// Init the objects if we have a title
	if (numObjects)
		{
		formP->numObjects = numObjects;
		titleP = (FormTitleType *) (formP->objects + numObjects);
		
		// install the title into the object list
		formP->objects[0].objectType = frmTitleObj;
		formP->objects[0].object.ptr = titleP;

		// Initialize the title object
		titleP->text = (Char *) (titleP + 1);
		StrCopy(titleP->text, titleStrP);
		}
	
	WinInitializeWindow (FrmGetWindowHandle(formP));
	WinAddWindow (FrmGetWindowHandle(formP));
	
	return formP;
}


/***********************************************************************
 *
 * FUNCTION:    PrvOffsetPtrIfNeeded
 *
 * DESCRIPTION: adjust the pointer at *ptrPtr by offset if it points 
 *              between start and end. 
 *
 * PARAMETERS:  ptrPtr - location of the pointer in question
 *								 if ptrPtr is NULL, nothing is offset
 *			 	start - start of memory block to look for pointers into
 *				end - pointer to first byte after the target zone
 *				offset - how much to offset it by
 *				incOnOffset - increment this value when a pointer is offset
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			ryw	    5/11/00	    Created
 *
 ***********************************************************************/
static void PrvOffsetPtrIfNeeded(UInt8 **ptrPtr, const void *start, const void *end, Int32 offset)
{
	if (ptrPtr != NULL)
		{
		if( start <= *ptrPtr && *ptrPtr < end )
			{
			*ptrPtr += offset;
			}
		}
}


/***********************************************************************
 *
 * FUNCTION:    PrvRewireForm
 *
 * DESCRIPTION: Update pointers in a form when the form or objects
 *		inside of the form are moved around.
 *
 * PARAMETERS:  formP - form to work on
 *			 	movedAddress - start of memory block to look for pointers into
 *				bytesMoved - bytes in block of memory that was moved
 *				offset - how much to offset by
 *
 * RETURNED:    nothing
 *
 * HISTORY:
 *		05/11/00	ryw	Created
 *		11/20/00	kwk	Handle special case of form w/no objects being rewired.
 *
 ***********************************************************************/
static void PrvRewireForm( FormType *formP, const void *movedAddress, UInt32 bytesMoved, Int32 offset )
{
	UInt32 index;
	void *objP;
	const void *endOfMoved = (const UInt8 *)movedAddress + bytesMoved;
	FormObjectKind kind;

	// ugly macros to make the rest of the function maintainable
	#define OffsetPtrIfNeeded(ptrPtr) PrvOffsetPtrIfNeeded((UInt8 **)ptrPtr, movedAddress, endOfMoved, offset)
	#define OffsetMemberIfNeeded(type, field) OffsetPtrIfNeeded( &( ( (type *)objP )->field ) )

	// Bail out if there's nothing moved; in this case, endOfMoved == movedAddress,
	// which means that PrvOffsetPtrIfNeeded will never do anything anyway.
	if (bytesMoved == 0)
	{
		return;
	}
	
	// Special case situation where form has 0 objects. In that situation,
	// formP->objects points to the very end of the form, and thus the
	// PrvOffsetPtrIfNeeded routine won't update it. Bump end of moved
	// limit by one to trigger an appropriate move. This seems like not
	// a very clean solution to the problem, but it's the safest to do
	// at this late stage in 4.0.
	if ((formP->numObjects == 0) && ((UInt8*)endOfMoved == (UInt8*)formP->objects))
	{
		endOfMoved = (const UInt8*)endOfMoved + 1;	
	}
	
	OffsetPtrIfNeeded(&formP->objects);
	
	for( index = 0; index < formP->numObjects; ++index )
		{
		OffsetPtrIfNeeded(&formP->objects[index].object.ptr);
		objP = formP->objects[index].object.ptr;
		kind = formP->objects[index].objectType;
		switch( kind )
			{
			case frmFieldObj:
				OffsetMemberIfNeeded(FieldType, text);
				OffsetMemberIfNeeded(FieldType, lines);
				break;
			case frmControlObj:
				OffsetMemberIfNeeded(ControlType, text);
				break;
			case frmListObj:
				OffsetMemberIfNeeded(ListType, itemsText);
				// adjust *itemsText too since I don't know where it is
				OffsetPtrIfNeeded( ((ListType *)objP)->itemsText );
				break;
			case frmTableObj:
				OffsetMemberIfNeeded(TableType, columnAttrs);
				OffsetMemberIfNeeded(TableType, rowAttrs);
				OffsetMemberIfNeeded(TableType, items);
				// could iterate over all the ptrs in each cell too
				break;
			case frmLabelObj:
				OffsetMemberIfNeeded(FormLabelType, text);
				break;
			case frmTitleObj:
				OffsetMemberIfNeeded(FormTitleType, text);
				break;
			case frmScrollBarObj:
			case frmGadgetObj:
			case frmGraffitiStateObj:
			case frmPopupObj:
			case frmRectangleObj:
			case frmFrameObj:
			case frmLineObj:
			case frmBitmapObj:
				// nothing to remap
				break;
			default:
				ErrNonFatalDisplay("illegal object type in form");
				break;
			}
		}
}


/***********************************************************************
 *
 * FUNCTION:    PrvMoveFormInternals
 *
 * DESCRIPTION: move a chunk of memory and then rewire the form pointers. 
 *
 * PARAMETERS:  formP - form to work on
 *			 	dest - address to move to
 *				source - address to move from
 *				numBytes - how many bytes to move
 *
 * RETURNED:    nothing
 *
 * HISTORY:
 *		05/11/00	ryw	Created
 *		11/20/00	kwk	Don't bother doing anything if numBytes == 0.
 *
 ***********************************************************************/
static void PrvMoveFormInternals( FormType *formP, void *dest, const void *source, Int32 numBytes )
{
	if (numBytes > 0)
	{
		MemMove(dest, source, numBytes);
		PrvRewireForm(formP, source, numBytes, (UInt8 *)dest - (UInt8 *)source);
	}
}


/***********************************************************************
 *
 * FUNCTION:    FrmAddSpaceForObject
 *
 * DESCRIPTION: Add space to a form for an object and add the object into 
 *					 the form's list of objects. 
 *
 * PARAMETERS:  formPP - pointer to a pointer to a form.  Set if moved.
 *					 objectPP - pointer to a pointer to an object.  Set to the location made.
 *					 objectType - the type of object
 *					 objectSize - size of the object.  Must include space for extras
 *									  like strings.
 *
 * RETURNED:    0 if no error
 *
 * HISTORY:
 *		11/19/97	rsf	Created by Roger Flores.
 *		05/15/00	ryw	use the new methods to fix pointers
 *		11/20/00	kwk	Removed doubled round-up of oldSize w/newSize calc.
 *							Relock form handle if we can't resize it, before returning.
 *
 ***********************************************************************/

Err FrmAddSpaceForObject (FormType * *formPP, MemPtr *objectPP, 
	FormObjectKind objectKind, UInt16 objectSize)
{
	MemHandle formH;
	FormType * formP;
	UInt16 newSize;
	UInt16 oldSize;
	UInt8 *oldLocationP;
	UInt8 *newLocationP;
	Err error;
	UInt16 bytesFromNewObjListEntryToEndOfFormChunk;
	
	ECFrmValidatePtr(*formPP);
	
	oldLocationP = (UInt8 *) *formPP;
	formH = MemPtrRecoverHandle(*formPP);
	oldSize = MemHandleSize(formH);
	oldSize = (oldSize + 1) & (~1);		// Round the oldSize up to a word boundary.
	newSize = oldSize + sizeof(FormObjListType) + objectSize;
	MemHandleUnlock(formH);
	error = MemHandleResize(formH, newSize);
	if (error)
	{
		MemHandleLock(formH);
		return error;
	}
	
	newLocationP = (UInt8 *) MemHandleLock(formH);
	formP = (FormType *) newLocationP;
	*formPP = formP;
	
	// Handle any chunk movement
	if( newLocationP != oldLocationP )
		{
		// fix the pointers 
		PrvRewireForm( formP, oldLocationP, oldSize, newLocationP - oldLocationP );
		
		if (CurrentForm == (FormType *) oldLocationP)
			CurrentForm = (FormType *) newLocationP;
		
		WinMoveWindowAddr((WindowType *) oldLocationP, (WindowType *) newLocationP);
		}
	
	// make a hole to add in another object list item
	bytesFromNewObjListEntryToEndOfFormChunk = ((UInt8 *)newLocationP + oldSize) - 
		(UInt8 *) &formP->objects[formP->numObjects];
	// move the internals and fix the pointers
	PrvMoveFormInternals(formP, &formP->objects[formP->numObjects + 1], &formP->objects[formP->numObjects], 
		bytesFromNewObjListEntryToEndOfFormChunk);
		
	// Set the object list item
	formP->objects[formP->numObjects].objectType = objectKind;
	*objectPP = (MemPtr)((UInt8 *)newLocationP + newSize - objectSize);
	formP->objects[formP->numObjects].object.ptr = *objectPP;
	formP->numObjects += 1;
	
	// zero the new space 
	MemSet(*objectPP, objectSize, 0);
	
	return 0;
}


/***********************************************************************
 *
 * FUNCTION:    FrmRemoveObject
 *
 * DESCRIPTION: Remove an object from a form.  This shrinks the form's 
 * object list and resizes the form chunk to a smaller size, fixing up
 * all the pointers.  It doesn't free any memory referenced by the object.
 *
 * PARAMETERS:  formPP - pointer to a pointer to a form.  Set if moved.
 *					 objIndex - index to an object to remove
 *
 * RETURNED:    0 if no error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	2/6/98	Initial Revision
 *			ryw  	5/11/00	use the new methods to fix pointers and fixed
 *                          a bug caused by rounding the hanlde size up to even
 *
 ***********************************************************************/
Err FrmRemoveObject (FormType * *formPP, UInt16 objIndex)
{
	MemHandle formH;
	FormType * formP;
	UInt16 newSize;
	UInt16 oldSize;
	Err error;
	Int32 sizeToMove;
	UInt8 * locationToRemove;
	UInt8 * locationAfterRemove;
	
	
	ECFrmValidatePtr(*formPP);
	
	
	formH = MemPtrRecoverHandle(*formPP);
	oldSize = MemHandleSize(formH);
	formP = *formPP;
	ErrNonFatalDisplayIf ((objIndex >= formP->numObjects), "Invalid index");
	
	// If the object isn't the last one then fill in the hole
	if (objIndex < formP->numObjects - 1)
		{
		locationToRemove = formP->objects[objIndex].object.ptr;
		locationAfterRemove = formP->objects[objIndex + 1].object.ptr;
		sizeToMove = ( ((UInt8 *)formP) + oldSize ) - locationAfterRemove;
		
		// Move all the objects after the one being removed.
		PrvMoveFormInternals(formP, locationToRemove, locationAfterRemove, sizeToMove);
				
		newSize = oldSize - (locationAfterRemove - locationToRemove);
		}
	else
		{
		newSize = (UInt8 *)formP->objects[objIndex].object.ptr - (UInt8 *)formP;
		}
	
	// Now fixup the object list.
	locationToRemove = (UInt8 *) &formP->objects[objIndex];
	locationAfterRemove = (UInt8 *) &formP->objects[objIndex + 1];
	sizeToMove = (UInt8 *) formP + newSize - locationAfterRemove;
	
	// Remove the object list entry by moving the list entries after the hole
	// as well as all the objects.
	formP->numObjects -= 1;
	PrvMoveFormInternals(formP, locationToRemove, locationAfterRemove, sizeToMove);
	
	newSize -= sizeof(FormObjListType);	
	
	// Now shrink the form chunk to it's new size.
	MemHandleUnlock(formH);
	error = MemHandleResize(formH, newSize);
	*formPP = MemHandleLock(formH);
	
	// If this ever proves false we'll need to fixup all the pointers in
	// the chunk to where the block is moved.
	ErrNonFatalDisplayIf (*formPP != formP, "Form shouldn't move");
	
	
	return error;
}


/***********************************************************************
 *
 * FUNCTION:    FrmNewLabel
 *
 * DESCRIPTION: Create a label. 
 *
 * PARAMETERS:  formPP - pointer to a pointer to a form. Set if the form moves.
 *					 ID - ID of the new label
 *					 textP - string for the new label.  Copied after the label.
 *					 x - x of the new label
 *					 y - y of the new label
 *					 font - font of the new label
 *
 * RETURNED:    0 if error else a pointer to the label
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/19/97	Initial Revision
 *
 ***********************************************************************/
FormLabelType * FrmNewLabel (FormType **formPP, UInt16 ID, const Char * textP, 
	Coord x, Coord y, FontID font)
{
	UInt16 size;
	FormLabelType *labelP;
	Err error;
	
	
	size = sizeof(FormLabelType);
	if (textP)
		size += StrLen(textP) + sizeOf7BitChar(nullChr);
	
	error = FrmAddSpaceForObject ((FormType * *) formPP, (MemPtr *)&labelP, frmLabelObj, size);
	if (error)
		return 0;
	
	labelP->id = ID;
	labelP->pos.x = x;
	labelP->pos.y = y;
	
	labelP->attr.usable = true;
	labelP->fontID = font;
	
	if (textP)
		{
		labelP->text = (Char *) (labelP + 1);
		StrCopy(labelP->text, textP);
		}
	
	return labelP;
}


/***********************************************************************
 *
 * FUNCTION:    FrmNewBitmap
 *
 * DESCRIPTION: Create a form bitmap object to display an existing bitmap resource. 
 *
 * PARAMETERS:  formPP - pointer to a pointer to a form. Set if the form moves.
 *					 ID - ID of the new bitmap
 *					 rscID - resource ID of the bitmap to display.
 *					 x - x of the new bitmap
 *					 y - y of the new bitmap
 *					 font - font of the new bitmap
 *
 * RETURNED:    0 if error else a pointer to the bitmap
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/19/97	Initial Revision
 *
 ***********************************************************************/
FormBitmapType * FrmNewBitmap (FormType **formPP, UInt16 /* id */, UInt16 rscID, 
	Coord x, Coord y)
{
	FormBitmapType *bitmapP;
	Err error;
	
	
	error = FrmAddSpaceForObject ((FormType * *) formPP, (MemPtr *)&bitmapP, frmBitmapObj, 
				sizeof(FormBitmapType));
	if (error)
		return 0;
	
	bitmapP->pos.x = x;
	bitmapP->pos.y = y;
	
	bitmapP->attr.usable = true;
	
	bitmapP->rscID = rscID;
	
	return bitmapP;
}


/***********************************************************************
 *
 * FUNCTION:    FrmNewGadget
 *
 * DESCRIPTION: Create a form gadget object to display an existing gadget resource. 
 *
 * PARAMETERS:  formPP - pointer to a pointer to a form. Set if the form moves.
 *					 ID - ID of the new gadget
 *					 x - x of the new gadget
 *					 y - y of the new gadget
 *
 * RETURNED:    0 if error else a pointer to the gadget
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/19/97	Initial Revision
 *
 ***********************************************************************/
FormGadgetType * FrmNewGadget (FormType **formPP, UInt16 id, 
	Coord x, Coord y, Coord width, Coord height)
{
	FormGadgetType *gadgetP;
	Err error;
	
	
	error = FrmAddSpaceForObject ((FormType * *) formPP, (MemPtr *)&gadgetP, frmGadgetObj, 
				sizeof(FormGadgetType));
	if (error)
		return 0;
	
	gadgetP->id = id;
	gadgetP->rect.topLeft.x = x;
	gadgetP->rect.topLeft.y = y;
	gadgetP->rect.extent.x = width;
	gadgetP->rect.extent.y = height;
	
	gadgetP->attr.usable = true;

	gadgetP->attr.extended = true;
	gadgetP->handler = 0;
	
	return gadgetP;
}


/***********************************************************************
 *
 * FUNCTION:    FrmNewGsi
 *
 * DESCRIPTION: Create a form graffiti shift indicator. 
 *
 * PARAMETERS:  formPP - pointer to a pointer to a form. Set if the form moves.
 *					 x - x of the new gsi object
 *					 y - y of the new gsi object
 *
 * RETURNED:    0 if error else a pointer to the gsi object
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			jaq		7/19/99		Initial Revision
 *
 ***********************************************************************/
FrmGraffitiStateType * FrmNewGsi (FormType **formPP, Coord x, Coord y)
{
	FrmGraffitiStateType *gsiP;
	Err error;
	
	error = FrmAddSpaceForObject ((FormType * *) formPP, (MemPtr *)&gsiP, frmGraffitiStateObj, 
				sizeof(FrmGraffitiStateType));
	if (error)
		return 0;
	
	gsiP->pos.x = x;
	gsiP->pos.y = y;
	
	return gsiP;
}


/***********************************************************************
 *
 * FUNCTION:    FrmActiveState
 *
 * DESCRIPTION: Saves and restores active Form/Window state.
 *
 *					The caller is expected to use the "helper" macros
 *					FrmSaveActiveState and FrmRestoreActiveState
 *					instead of calling this function directly.
 *
 * PARAMETERS:	stateP	-- ptr to state structure
 *					save		-- if true, save, otherwise restore active state	
 *
 * RETURNED:    0 if success
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	12/10/97	Initial Revision
 *			kwk	09/05/98	Always save/restore the draw & active windows,
 *								even if there's a current form.
 *			kwk	09/11/98	Backed out preceeding change until I can
 *								figure out why it breaks some modal forms.
 *
 ***********************************************************************/
Err FrmActiveState (FormActiveStateType* stateP, Boolean save)
{
	Err err = 0;
	PrvActiveStateType* sP = (PrvActiveStateType*)stateP;
	
	if (!sP)
		{
		ErrNonFatalDisplay("null ptr arg");
		return memErrInvalidParam;
		}

	//------------------------------------------------------------
	// Check to make sure our state structure fits in the user param.
	// The compiler will normally compile this line out, unless
	// the assertion fails, in which case a link error will be
	// generated.
	//------------------------------------------------------------
	if (sizeof(PrvActiveStateType) > sizeof(FormActiveStateType))
		ThisShouldGenerateALinkErrorForm();


	if ( save )
		{
		sP->sig = prvActiveStateSignature;
		
		sP->insPtState = InsPtEnabled();
		sP->attnIndicatorState = AttnIndicatorAllowed();
		InsPtGetLocation (&sP->insPtPos.x, &sP->insPtPos.y);

#if 0	// Remove for now, since this introduces other bugs...
		// Always save active form & draw/active windows, in case the
		// windows aren't the same as the form.
		sP->curForm = FrmGetActiveForm ();
		sP->savedDrawWin = WinGetDrawWindow ();
		sP->savedActiveWin = WinGetActiveWindow ();
		
		// But wait...don't save the draw/active window if it's for a menu,
		// since that gets tossed from beneath us when the new form is
		// activated...there has got to be a cleaner way to handle this.
		if (MenuGetActiveMenu() != NULL)
			{
			WinHandle menuWin;
			MenuBarType * menuP = MenuGetActiveMenu();
			
			if (menuP->curMenu != noMenuSelection)
				menuWin = menuP->menus[menuP->curMenu].menuWin;
			else
				menuWin = menuP->barWin;
			
			if (menuWin == sP->savedDrawWin)
				sP->savedDrawWin = NULL;
			
			if (menuWin == sP->savedActiveWin)
				sP->savedActiveWin = NULL;
			}
#else
		sP->curForm = FrmGetActiveForm ();
		if (! sP->curForm)
			{
			sP->savedDrawWin = WinGetDrawWindow ();
			sP->savedActiveWin = WinGetActiveWindow ();
			}
#endif
		}
	else
		{
		// Catch calls to restore state that wasn't saved first
		ErrFatalDisplayIf (sP->sig != prvActiveStateSignature, "invalid state passed");

		InsPtSetLocation (sP->insPtPos.x, sP->insPtPos.y);

		if (sP->curForm)
			{
			FrmSetActiveForm (sP->curForm);

#if 0		// Remove for now, since this fix introduces other bugs...
			// If the draw/active windows aren't null, set them even if we
			// have a form, as this fixes the problem with a form appearing
			// over a popup list.
			if (sP->savedDrawWin != NULL)
				WinSetDrawWindow(sP->savedDrawWin);
			
			if (sP->savedActiveWin != NULL)
				WinSetActiveWindow(sP->savedActiveWin);
#endif
			}
		else
			{
			WinSetDrawWindow (sP->savedDrawWin);
			WinSetActiveWindow (sP->savedActiveWin);
			}
		
		InsPtEnable (sP->insPtState);
		if (sP->attnIndicatorState)
			AttnIndicatorAllow (true);
		}
	
	
	return err;
}


/***********************************************************************
 *
 * FUNCTION:    PrvHandleTsmEvent
 *
 * DESCRIPTION: This routine is call to handle Text Services
 *              event when no object has the focus.  Even without
 *              a focus we support some Text Services function,
 *              like turning the Front End Prcessor on of off.
 *
 * PARAMETERS:	 formP    pointer to a FrmType structure.
 *              event  pointer to an EventType structure.
 *
 * RETURNED:	 false if text services off, true otherwise.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	05/13/98	Initial Revision
 *			srj	07/08/98	Caused routine to return false if the 
 *								text services are off.  This allows routine
 *								to be called whether or not text services are
 *								available (which seems to be the convention)
 *								Bug?--  This routine always returns true if 
 *								text services are on, even if it doesn't do
 *								anything.
 *			kwk	07/30/98	Removed conditional for text services off,
 *								since now we'll return false if the FEP
 *								library refnum is invalid (Sumo default case).
 *								Also set eventInfo.formEvent to true, and
 *								put in error checking code.
 *								Return real result of call (action.handledEvent)
 *			kwk	08/28/98	Only call TsmCommitAction if TsmHandleEvent
 *								says that it handled the event. Check for errors.
 *			kwk	09/28/98	No longer pass action rec ptr to TsmCommitAction.
 *			kwk	10/04/98	Add extra debug info to fatal alert message for
 *								TsmHandleEvent error case.
 *
 ***********************************************************************/
static Boolean
PrvHandleTsmEvent(FormType * /* formP */, EventType* event)
{
	TsmFepEventType		eventInfo;
	TsmFepActionType		action;
	UInt16 					refNum = GTsmFepLibRefNum;
	TsmFepStatusType* 	status = (TsmFepStatusType*)GTsmFepLibStatusP;
	Err						result;
	
	if (refNum == sysInvalidRefNum)
		return (false);

	MemSet(&eventInfo, sizeof(TsmFepEventType), 0);
	eventInfo.formEvent = true;
	
	result = TsmLibFepHandleEvent(refNum, (SysEventType *)event, &eventInfo, status, &action);
	if (result != 0)
		{
		Char errorMsg[64];
		StrPrintF(errorMsg, "TsmHandleEvent returned error %d for event %d", result, event->eType);
		ErrNonFatalDisplay(errorMsg);
		}
	
	ErrNonFatalDisplayIf(action.updateText || action.updateSelection || (action.dumpLength > 0),
		"Input method changing status");
	
	result = TsmLibFepCommitAction(refNum, status);
	ErrNonFatalDisplayIf(result, "TsmCommitAction returned an error");
	
	if (action.updateFepMode)
		GrfSetState(false, false, false);

	return(action.handledEvent);
}


/***********************************************************************
 *
 * FUNCTION:	FrmHandleEvent
 *
 * DESCRIPTION: 
 *
 * PARAMETERS:	formP		memory block that contains the form.
 *					eventP	Ptr to event record.
 *					
 * RETURNED:	true if event was handled by form or form object.
 *
 *	HISTORY:
 *		11/07/94	art	Created by Art Lamb.
 *		02/20/96	art	Added scroll bar
 *		07/22/96	art	Added frm title events
 *		09/16/98	kwk	If the table says that it didn't handle a
 *							tsm button event, do private handling.
 *		09/04/99	kwk	Also handle tsm mode event.
 *		09/22/99 jmp	Call MenuEraseMenu() at frmCloseEvent time.
 *		10/06/99	jmp	Move code from previous change to FrmEraseForm()
 *							to fix bug #22585.
 *
 ***********************************************************************/
Boolean FrmHandleEvent (FormType * formP, EventType * eventP)
{
	FormObjectKind objType;
	FormObjectType obj;
	ControlType * ctlP;
	EventType event;
	TableType * tblP;
	Char * str;
	Boolean handled;
	UInt16 objIndex;

	ECFrmValidatePtr(formP);
	
	handled = false;

	if (eventP->eType == penDownEvent)
		{
		if (FrmPointInIndicator (formP, eventP->screenX, eventP->screenY))
			{
			// If the attention indicator is blinking (even if it's not currently
			// showing), then handle taps in that area by bringing up the attention
			// manager's list dialog indirectly through a sequence of events. This
			// takes priority over the form title and anything else which is under it.
			MemSet (&event, sizeof(EventType), 0);
			event.eType = attnIndicatorEnterEvent;
			event.penDown = eventP->penDown;
			event.screenX = eventP->screenX;
			event.screenY = eventP->screenY;
			event.data.attnIndicatorEnter.formID = formP->formId;
			EvtAddEventToQueue (&event);
			handled = true;
			}
		else if (PrvPointInObject (formP, eventP->screenX, eventP->screenY, &objIndex))
			{
			objType = formP->objects[objIndex].objectType;
			obj = formP->objects[objIndex].object;
			
			PrvReleaseFocus (formP, objIndex);
			if (objType == frmFieldObj || objType == frmTableObj)
				formP->focus = objIndex;

			if (objType == frmFieldObj)
				handled = FldHandleEvent (obj.field, eventP);

			else if (objType == frmTableObj)
				handled = TblHandleEvent (obj.table, eventP);

			else if (objType == frmControlObj)
				handled = CtlHandleEvent (obj.control, eventP);

			else if (objType == frmListObj)
				handled = LstHandleEvent (obj.list, eventP);
			
			else if (objType == frmScrollBarObj)
				handled = SclHandleEvent (obj.scrollBar, eventP);
			
			// If User tapped in a gadget, check if its the new extended type
			// and if it has a gadget handler installed.  If so, then
			// create a frmGadgetEnterEvent for it
			else if (objType == frmGadgetObj)
				{
				if(obj.gadget->attr.extended && obj.gadget->handler != 0)
					{
					MemSet (&event, sizeof(EventType), 0);
					event.eType = frmGadgetEnterEvent;
					event.penDown = eventP->penDown;
					event.screenX = eventP->screenX;
					event.screenY = eventP->screenY;
					event.data.gadgetEnter.gadgetID = obj.gadget->id;
					event.data.gadgetEnter.gadgetP = obj.gadget;
					EvtAddEventToQueue (&event);
					handled = true;
					}
				}
			}
		else if (PrvHelpHandleEvent (formP, eventP))
			{
			handled = true;
			}
		else if (FrmPointInTitle (formP, eventP->screenX, eventP->screenY))
			{
			MemSet (&event, sizeof(EventType), 0);
			event.eType = frmTitleEnterEvent;
			event.penDown = eventP->penDown;
			event.screenX = eventP->screenX;
			event.screenY = eventP->screenY;
			event.data.frmTitleEnter.formID = formP->formId;
			EvtAddEventToQueue (&event);
			handled = true;
			}
		}

	else if ((eventP->eType == keyDownEvent)
	|| (eventP->eType == tsmFepButtonEvent)
	|| (eventP->eType == tsmFepModeEvent))
		{
		if (formP->focus != noFocus)
			{
			objType = formP->objects[formP->focus].objectType;
			obj = formP->objects[formP->focus].object;
		
			if (objType == frmFieldObj)
				handled = FldHandleEvent (obj.field, eventP);
			else if (objType == frmTableObj)
				{
				handled = TblHandleEvent (obj.table, eventP);
				if ((!handled) && ((eventP->eType == tsmFepButtonEvent) || (eventP->eType == tsmFepModeEvent)))
					handled = PrvHandleTsmEvent (formP, eventP);
				}
			}
		else
			{
			handled = PrvHandleTsmEvent (formP, eventP);
			}
			

		if ( (! handled)  &&  
			  (eventP->eType == keyDownEvent)  &&
			  (eventP->data.keyDown.chr == confirmChr) )
			{
			PrvSendConfirmEvent (formP);
			handled = true;
			}
		}

	else if (eventP->eType == tblEnterEvent)
		{
		tblP = eventP->data.tblEnter.pTable;
		handled = TblHandleEvent (tblP, eventP);
		}

	else if (eventP->eType == tblExitEvent)
		{
		// Don't send characters into the table anymore.
		FrmSetFocus(formP, noFocus);
		}

	else if ((eventP->eType == fldEnterEvent) ||
				(eventP->eType == fldHeightChangedEvent))
		{
		if (formP->focus != noFocus)
			{
			objType = formP->objects[formP->focus].objectType;
			obj = formP->objects[formP->focus].object;
		
			if (objType == frmFieldObj)
				handled = FldHandleEvent (obj.field, eventP);
			else if (objType == frmTableObj)
				handled = TblHandleEvent (obj.table, eventP);
			}
		}

	else if (eventP->eType == menuCmdBarOpenEvent)
		{
		SysNotifyParamType notification;

		if (formP->focus != noFocus)
			{
			objType = formP->objects[formP->focus].objectType;
			obj = formP->objects[formP->focus].object;
		
			if (objType == frmFieldObj)
				handled = FldHandleEvent (obj.field, eventP);
			else if (objType == frmTableObj)
				handled = TblHandleEvent (obj.table, eventP);
			}

		//give entities outside the event loop a chance to throw buttons in the command bar
		//by sending a notification.
		
		notification.notifyType = sysNotifyMenuCmdBarOpenEvent;
		notification.broadcaster = sysNotifyBroadcasterCode;
		notification.notifyDetailsP = NULL;
		notification.handled = false;
		
		SysNotifyBroadcast(&notification);
			
		
		// Now everyone has had a chance to add their command buttons; 
		// if nobody says they've handled it, tell the menu system to display the bar.
		if (!handled)
			MenuCmdBarDisplay();
		}

	else if (eventP->eType == ctlEnterEvent)
		{
		// If a grouped control was pressed, turn off the current selection
		//	of the group.
		ctlP = eventP->data.ctlEnter.pControl;
		if (ctlP->group)
			{
			objIndex = FrmGetControlGroupSelection (formP, ctlP->group);
			FrmSetControlValue (formP, objIndex, false);
			}

		handled = CtlHandleEvent(ctlP, eventP);

		// If a grouped control was pressed, but the pen has released outside 
		//	the control, turn the prior selection back on.
		if (ctlP->group && (!ctlP->attr.on))
			{
			FrmSetControlValue (formP, objIndex, true);				
			}
		}

	else if (eventP->eType == ctlRepeatEvent)
		{
		handled = CtlHandleEvent(eventP->data.ctlEnter.pControl, eventP);
		}

	else if (eventP->eType == ctlSelectEvent)
		{
		ctlP = eventP->data.ctlEnter.pControl;
		if (ctlP->style == popupTriggerCtl)
			{
			FrmPopupList (formP, eventP);
			handled = true;		// <3/5/99 SCL> Integrated from Eleven
			}
		}

	else if (eventP->eType == popSelectEvent)
		{
		str = LstGetSelectionText (eventP->data.popSelect.listP, 
		                           eventP->data.popSelect.selection);
		CtlSetLabel (eventP->data.popSelect.controlP, str);
		handled = true;		// <3/5/99 SCL> Integrated from Eleven
		}

	else if (eventP->eType == lstEnterEvent)
		{
		handled = LstHandleEvent(eventP->data.lstEnter.pList, eventP);
		}

	else if (eventP->eType == frmCloseEvent)
		{
		FrmSetFocus (formP, noFocus);
		AttnIndicatorAllow (false);
		FrmEraseForm (formP);
		FrmDeleteForm (formP);
			
		handled = true;
		}
	
	else if (eventP->eType == frmUpdateEvent)
		{
		FrmDrawForm (formP);
		handled = true;
		}
		
	else if (eventP->eType == menuEvent)
		{
		handled = PrvDoMenuCommand (formP, eventP->data.menu.itemID);
		}
		
	else if (eventP->eType == sclEnterEvent || eventP->eType == sclRepeatEvent)
		{
		handled = SclHandleEvent (eventP->data.sclRepeat.pScrollBar, eventP);
		}
		
	else if (eventP->eType == frmTitleEnterEvent)
		{
		PrvSelectFormTitle (formP, eventP);
		handled = true;
		}

	else if (eventP->eType == frmTitleSelectEvent)
		{			
		// Taps in the title produce a menuChr to bring up the menu bar.
		MemSet (&event, sizeof(EventType), 0);
		event.eType = keyDownEvent;
		event.data.keyDown.chr = vchrMenu;
		event.data.keyDown.keyCode = 0;
		event.data.keyDown.modifiers = commandKeyMask;
		EvtAddEventToQueue (&event);
		handled = true;
		}
	
	else if (eventP->eType == attnIndicatorEnterEvent)
		{
		PrvSelectIndicator (formP, eventP);
		handled = true;
		}

	else if (eventP->eType == attnIndicatorSelectEvent)
		{			
 		// Turn off indicator before dialog saves area behind itself.		
		AttnIndicatorSetBlinkPattern(kAttnIndicatorBlinkPatternNone);
		
		// Trigger the dialog to open in list mode.
		//DOLATER - peter: consider just opening the dialog right here.
		MemSet (&event, sizeof(EventType), 0);
		event.eType = keyDownEvent;
		event.data.keyDown.chr = vchrAttnIndicatorTapped;
		event.data.keyDown.keyCode = 0;
		event.data.keyDown.modifiers = commandKeyMask;
		EvtAddEventToQueue (&event);
		handled = true;
		}
	
	// pass events off to extended gadgets
	else if ((eventP->eType == frmGadgetEnterEvent ||
					 eventP->eType == frmGadgetMiscEvent) &&
			 eventP->data.gadgetEnter.gadgetP->attr.extended)
		{
		FormGadgetHandlerType *handler = eventP->data.gadgetEnter.gadgetP->handler;
		
		if (handler != 0)
			(handler)(eventP->data.gadgetEnter.gadgetP, formGadgetHandleEventCmd, eventP);
		}


	return (handled);
}
