/******************************************************************************
 *
 * Copyright (c) 1997-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: Progress.c
 *
 * Release: 
 *
 * Description:
 * 	   Generic progress dialog routines. These are taken from the
 *	progress dialogs in the network library. But they are made generic so 
 *	that they can be used by any process that wants to display advancing 
 *	progress. The idea is that you can display the dialog and then go into
 *	an event loop that calls a progress dialog handler to MemHandle any dialog
 *	specific events. The dialog can display a few lines of text that are 
 *	automatically centered and formated to display correctly. You can also 
 *  specify an icon the indicates the operation in effect. The dialog has one
 *	optional button that can as a cancel or an ok button.
 *     These routines are designed to to used from the UI task, but they also 
 *	support being updated from another task that my be doing lower level
 *  transactions.
 *
 * History:
 *		8/13/97	Created by Gavin Peacock
 *      mm/dd/yy   initials - brief revision comment
 *
 *****************************************************************************/

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_FORMS
#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS
#define ALLOW_ACCESS_TO_INTERNALS_OF_PROGRESS

#include <PalmTypes.h>
#include <SystemPublic.h>

#include "Form.h"
#include "Menu.h"
#include "Progress.h"
#include "UIResourcesPrv.h"

//---------------------------------------------------------------------------
// Gremins Constants / Globals
//---------------------------------------------------------------------------
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	// Gremlins support is well-abstracted and general.  Should be easy to
	// apply to different apps.
	
	// Gremlins support is DUPLICATED in Clipper's "Content.c".
	// TBD: There should be some common file where staticly-linked helpers can go.

#define cancelSuccessRate 2  // percent of cancels allowed to get through
#define randN(N) ((int) (((long) SysRandom(0) * (N)) / ((long) sysRandomMax + 1)))
#define randPercent() (randN(100))

#endif // ERROR_CHECK_LEVEL == ERROR_CHECK_FULL

//---------------------------------------------------------------------------
// Private Constants
//---------------------------------------------------------------------------
#define	prvStrIndexOK					1				// "OK"
#define	prvStrIndexCancelling		2				// "Cancelling"
#define	prvStrIndexError				3				// "Error: "
#define	prvStrIndexUnnamed			4				// "Unnamed"

#define	prvProgressIconLeft			4				// Left coordinate for icon
#define	prvProgressIconTop			12				// Top coordinate for icon 12
#define	prvProgressIconWidth			33				// Width of dial progress icons
#define	prvProgressIconHeight		41				// Height of dial progress icons	

#define	prvProgressTextLeft			40				// Left coordinate for text 40
#define	prvProgressTextTop			12				// Top coordinate for text
#define	prvProgressTextHeight		52				// height of text
#define	prvProgressTextWidth			110			// width of text 110


//---------------------------------------------------------------------------
// Private Functions
//---------------------------------------------------------------------------

#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL

/***********************************************************************
 *
 * FUNCTION:    GrmHandleTransactionCancel
 *
 * DESCRIPTION: This function should be called to let gremlins try to
 *				absorb transaction cancel events.  Basically, if gremlins
 *				is running, we log this cancel in our measurements
 *				and then decide whether it should be absorved.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    true if the cancel was handled (absorbed); false otherwise.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		----------- 
 *			dia 	08/14/98	Initial Revision
 *
 ***********************************************************************/

static Boolean GrmHandleTransactionCancel (void)
{
	Boolean handled = false;
	if (SysGremlins (GremlinIsOn, NULL))
		{
		handled = (randPercent() <= cancelSuccessRate);
		if (!handled)
			{
			DbgMessage ("...Transaction cancelled\n");
			}
		else
			{
			DbgMessage ("c");
			}
		}
	return handled;
}

#else // ERROR_CHECK_LEVEL == ERROR_CHECK_FULL

// If we're not doing full error checking, this macro do nothing (gremlins didn't 
// MemHandle the cancel)
#define GrmHandleTransactionCancel()	false

#endif // ERROR_CHECK_LEVEL == ERROR_CHECK_FULL



static void PrvUpdateDialog(ProgressPtr prgP);

/***********************************************************************
 *
 * FUNCTION:    PrgUpdateDialog
 *
 * DESCRIPTION: 
 *		This procedure can be used by the protocol task to tell the UI
 *		task what to display. Calls to this procedure can be interspersed
 *		throughout the protocol code to display progress messages.
 *		If this is not called from the UI task, updateNow must be false. This 
 *		will cause the UI task to wake up and it can then call PrgHandleEvent to
 *		update the progress dialog. If called from the UI task, set updateNow true
 *		and the dialog will be updated immediately.
 *		
 *
 * CALLED BY:	 Protocol or UI task to tell UI task what to display
 *
 * PARAMETERS:  prgP - progress data pointer
 *				err  - current error if any
 *				stage - current stage of progress (passed to callback)
 *				messageP - additional text describing this stage
 *				updateNow - boolean - true if called from the UI task
 *
 * RETURNED:    0 if no error.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		----------- 
 *			Gavin	8/13/97		Initial Revision
 *
 ***********************************************************************/
void	PrgUpdateDialog(ProgressPtr prgP, UInt16 err, UInt16 stage,
					const Char *	messageP,Boolean updateNow)
{
	
	ErrNonFatalDisplayIf(!prgP,"Null Progress Pointer");
	if (!prgP) return;
	
	// prgP->needUpdate = false;
	prgP->messageChanged = false;  // record if message actually changed

	// Disable task switching so the UI doesn't get partial info
	// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
	
	// Update the stage globals
	if (stage != prgP->stage)
		{
		prgP->stage = stage;
		prgP->needUpdate = true;
		}
		
	if (messageP)
		{ if (StrNCompare(prgP->message,messageP, progressMaxMessage)!=0)
			{
			StrNCopy(prgP->message, messageP, progressMaxMessage);
			prgP->messageChanged = true;
			prgP->needUpdate = true;
			}
		}
	else if (prgP->message[0]) 
		{
		prgP->message[0] = 0;
		prgP->messageChanged = true;
		prgP->needUpdate = true;
		}
		
	// Record error, if any
	if (err != prgP->error)
		{
		prgP->error = err;
		prgP->needUpdate = true;
		}
		
	if (updateNow)
		PrvUpdateDialog(prgP);
	else
		{
		// Renable switching
		// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS

		// Make the UI wake up to update the progress dialog
		EvtWakeup();
		}
}


/***********************************************************************
 *
 * FUNCTION:    PrgHandleEvent
 *
 * DESCRIPTION: This routine is used to process progress dialog events. An
 *				application should first call PrgStartDialog, then drop into a 
 *				loop that gets events and calls this function to MemHandle them. If
 *				This function does not MemHandle an event, the app can check the 
 *				event to see if it should stop. If the user hits the cancel 
 *				button in the progress dialog, this function will return false and
 *				the calling app should call PrgUserCancel() to see if the process
 *				has been canceled.
 *				This routine will not allow autoOff events to be processed unless the
 *				diallog is just displaying an error. It also prevents appStop events.
 * 
 *
 * CALLED BY:	 UI task to check for events and update UI as connection progresses.
 *
 * PARAMETERS:  prgGP - pointer to progress data
 *				eventP - current event to process
 *
 * RETURNED:    true if the event was handled by this routine
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		----------- 
 *			Gavin 8/13/97	Initial Revision
 *			dia	8/4/98	Updated for gremlins
 *			dia 	8/11/98	Now works (correctly) using the feature manager.
 *			dia	8/13/98	Decomposed out gremlins support.
 *
 ***********************************************************************/
Boolean	PrgHandleEvent(ProgressPtr prgP,EventType * eventP)
{
	UInt16			error;
	Boolean			handled = true;  // assume we MemHandle most all events

				
	if (!prgP) 
		{
		// allow system events to be processed...
		//if (eventP)
		//	{
		//	if (! SysHandleEvent (eventP))
		//		if (! MenuHandleEvent (0, eventP, &error))
		//			FrmHandleEvent (FrmGetActiveForm(),eventP);
		//	}
		return false;
		}
	if (!eventP) {	// you can pass null for event to just update the dialog
		PrvUpdateDialog(prgP);
		return false;
		}

	// Filter out some key events
	if	(	(eventP->eType == keyDownEvent)
	 	&&	(!TxtCharIsHardKey(	eventP->data.keyDown.modifiers,
										eventP->data.keyDown.chr))
		&&	(EvtKeydownIsVirtual(eventP))) {
		switch (eventP->data.keyDown.chr) {
			case vchrAutoOff:
				// Don't allow auto-off events unless we're just displaying an error and
				//	waiting for the user to hit OK
				// Note: handled set to true at routine entry
				if (!prgP->error) goto Exit;
				break;
				
			case vchrRonamatic:
				// Disallow bringing up anything while the dialog is up
				// Note: handled set to true at routine entry
				goto Exit;
				break;
				
			case vchrPageDown:
				prgP->showDetails = true;
				prgP->needUpdate = true;
				break;
			}
		}
		
	// Pass system events off to system.	
	if (! SysHandleEvent ((EventType *)eventP))
		if (! MenuHandleEvent (0, eventP, &error))
			FrmHandleEvent (prgP->frmP,eventP);

	//---------------------------------------------------------------------------
	// Handle our own events here.
	//---------------------------------------------------------------------------
	if (eventP->eType == ctlSelectEvent) {
		switch (eventP->data.ctlSelect.controlID) {
			// The "Cancel" button has a dual purpose - it is renamed to the "OK" button
			//  if we've encountered and displayed an error message in the dialog 
			//  and are waiting for the user to acknowledge the error
			case prgProgressBtnCancel:
				// If we were just waiting for an OK, exit
				if (prgP->waitingForOK || prgP->error) {handled = false; goto Exit;}

				if (!GrmHandleTransactionCancel())
					{
					// post a cancel
					prgP->cancel = true;
					prgP->needUpdate = true;
					handled = false;  // indicate not handled so app knows about user cancel
					}
				break;
		
			}
		}

	// Quitting the app is not allowed while the progress dialog is up
	else if (eventP->eType == appStopEvent) 
		SndPlaySystemSound(sndWarning);
		
		
	// update the progress dialog
	PrvUpdateDialog(prgP);
	
Exit:
	return  handled;
}
	
			
/***********************************************************************
 *
 * FUNCTION:    PrvUpdateDialog
 *
 * DESCRIPTION: Updates the display of a dialog opened with PrgStartDialog
 *				This checks the needUpdate field of the progress structure.
 *				If there are changes that need to be displayed, this will 
 *				display them. 
 *
 * CALLED BY:	 Any UI task routine that wants to update the progress dialog.
 *				This function does not change the state of the dialog, it just
 *				updates the display. 
 *
 * PARAMETERS:  prgP - pointer to progress info Started with PrgStartDialog.
 *
 * RETURNED:    0 if no error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		----------- 
 *			gavin	8/13/97	Initial Revision
 *			jmp	11/16/99	Add WinEraseRectangle() to up drawing of dial progress icons so
 *								they don't get drawn on top of each other!
 *       jmp   12/02/99 Fix bug #24377.  Don't call WinScreenUnlock() if WinScreenLock()
 *                      fails.
 *                          
 ***********************************************************************/
static void PrvUpdateDialog(ProgressPtr prgP)
{		
	const int			maxTextLen = progressMaxMessage; 
	Char * 				textP = 0;
	UInt16				bitmapID;
	UInt16 				curError;
	UInt16 				curStage;
	Char * 				curMessageP = 0;
	Boolean	 			curCancel;
	RectangleType		r;
	ControlPtr 			ctlP;
	Int16					chars;
	Char *				cP;
	Int16					textLen, lines, lineHeight;
	FontID				oldFont;
	MemHandle			resH;
	Boolean				delay = false;
	PrgCallbackData	tcP;
	Boolean 				timedOut = false;
	
	if (!prgP) return;
	
	// The textcallback can set a timeout value in ticks
	// if the timeout expires, we clear the timeout value 
	// and force an update  The textCallback will be passed a timedOut flag
	if (prgP->timeout && (TimGetTicks() > prgP->timeout))
		{
		prgP->timeout = 0;
		// set timedout cause only if there wasn't already an update pending...
		if (!prgP->needUpdate) timedOut = true; 
		prgP->needUpdate = true;
		}
	//------------------------------------------------------------------------
	// See if our connection stage has changed, and if so update the string
	//------------------------------------------------------------------------
	if (prgP->needUpdate ) 
		{
		
		// Allocate a buffer for holding the new text
		textP = MemPtrNew(maxTextLen+1);
		if (!textP) {
			goto Exit;
			}
		// Init text variable
		textP[0] = 0;

		// Allocate a buffer for message text from comm layer
		curMessageP = MemPtrNew(progressMaxMessage+1);
		if (!curMessageP) {
			goto Exit;
			}
		// Disable task switching while we fetch the current info and message
		// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
		
		curStage = prgP->stage;
		curCancel = prgP->cancel;
		StrNCopy(curMessageP, prgP->message, progressMaxMessage);
		curError = prgP->error;
		prgP->needUpdate = false;
		
		// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
		
		tcP.stage = curStage;
		tcP.textP = textP;
		tcP.textLen = maxTextLen;
		tcP.message = curMessageP;
		tcP.canceled = curCancel;
		tcP.error = curError;
		tcP.bitmapId = prgP->lastBitmapID;
		tcP.showDetails = prgP->showDetails;
		tcP.timedOut = timedOut;
		tcP.textChanged = true;		// assume text changed unless this is cleared
		tcP.timeout = prgP->timeout;
		tcP.delay = delay;					// preset to false so callback doesn't have to change it
		tcP.userDataP = prgP->userDataP;	// context pointer for callback routine
		
		//if (!(prgP->textCallback)(curStage,prgP->showDetails,curMessage,curCancel,curError,textP,maxTextLen,&bitmapID))
		if (!(prgP->textCallback)(&tcP))
			{
			// add some default handlers in case the callback does not MemHandle it...
			
			// If the protocol task hasn't recognized the cancel yet, the boolean
			//  will still be set with no error yet
			if (curCancel && !curError ) {
				SysStringByIndex(prgStringList, prvStrIndexCancelling, textP, maxTextLen);	
				}
				
			// If we got a connection error, display it.
			else if (curError) {
				SysStringByIndex(prgStringList, prvStrIndexError, textP, maxTextLen);	
				SysErrString(curError, textP+StrLen(textP), 
													maxTextLen-StrLen(textP));
				bitmapID = prgPictError;
				}
			else 
				{	
				// unknown stage message....			
				SysStringByIndex(prgStringList, prvStrIndexUnnamed, textP, maxTextLen);	
				}
			}
		else
			{
			// the text callback can modify any of these values...
			prgP->timeout = tcP.timeout;
			curCancel = tcP.canceled;
			curError = tcP.error;
			bitmapID = tcP.bitmapId;
			delay = tcP.delay;
			}
		//-----------------------------------------------------------------------
		// Update the text
		//-----------------------------------------------------------------------
		if (tcP.textChanged)
			{
			// Form the enclosing rect for the text area
			r.topLeft.x = prvProgressTextLeft;
			r.topLeft.y = prvProgressTextTop;
			r.extent.x = prvProgressTextWidth;
			r.extent.y = prvProgressTextHeight;

			// Get the height of each line
			oldFont = FntSetFont(boldFont);
			lineHeight = FntLineHeight();
			
			// Calculate how many lines we'll need to display our text
			textLen = StrLen(textP);
			cP = textP;
			for (lines = 1; 1; lines++) {
				chars = FldWordWrap(cP, r.extent.x);
				if (!chars) break;
				textLen -= chars;
				cP += chars;
				if (!textLen) break;
				}
				
			// Disable task switching while we change the text
			// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS

			// Erase the old text
			WinEraseRectangle(&r, 0);

			// Leave an equal amount on top and bottom of text
			if (lineHeight * lines < r.extent.y) 
				r.topLeft.y += (r.extent.y - lineHeight * lines) / 2;
			else 
				lines = r.extent.y / lineHeight;
				
			// Draw the new text, Get rid of newLines because they can't be drawn.
			cP = textP;
			while(lines--) {
				chars = FldWordWrap(cP, r.extent.x);
				if (!chars) break; // catch case where text is empty...
				if (cP[chars-1] == '\n') cP[chars-1] = ' ';
				WinDrawChars(cP, chars, r.topLeft.x, r.topLeft.y);
				cP += chars;
				r.topLeft.y += FntLineHeight();
				}
			
			// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS
			
			// Restore font
			FntSetFont(oldFont);
		}		

		//-----------------------------------------------------------------------
		// Update the ICON if it changed
		//-----------------------------------------------------------------------
		if (bitmapID != prgP->lastBitmapID) {
			resH = DmGetResource(bitmapRsc, bitmapID);
			if (resH) {
				RectangleType eraseRectangle;
				UInt8 * lockedWinP;

				eraseRectangle.topLeft.x	= prvProgressIconLeft;
				eraseRectangle.topLeft.y	= prvProgressIconTop;
				eraseRectangle.extent.x		= prvProgressIconWidth;
				eraseRectangle.extent.y		= prvProgressIconHeight;
				
				lockedWinP = WinScreenLock(winLockCopy);
				WinEraseRectangle(&eraseRectangle, 0);
				WinDrawBitmap(MemHandleLock(resH), prvProgressIconLeft, prvProgressIconTop);
				if (lockedWinP)
					WinScreenUnlock();
				
				MemHandleUnlock(resH);
				prgP->lastBitmapID = bitmapID;
				}
			}


		ctlP = FrmGetObjectPtr (prgP->frmP, FrmGetObjectIndex (prgP->frmP, prgProgressBtnCancel));
		
		if (curCancel && !curError ) 
			{
			// hide the cancel button while cancelling
			CtlHideControl(ctlP);
			}
		// If we got a connection error
		else if (curError && !curCancel)  //  && curError != obxErrUserCancel) 
			{
			// Show the control in the somewhat unlikely case that the Network Interface code encountered
			//  an error during the cancel process (which hid the control) and needs to display it. 
			CtlShowControl(ctlP);
			CtlSetLabel(ctlP, SysStringByIndex(prgStringList, prvStrIndexOK, 
													prgP->ctlLabel, sizeof(prgP->ctlLabel)-1));
			CtlDrawControl(ctlP);
			
			// Set boolean indicating we need a hit on the OK button before
			//  we can exit
			prgP->waitingForOK = true;
			}
				

	// Display success and connect speed messages for a second before we close dialog
	if (delay)
		SysTaskDelay(sysTicksPerSecond*1);

	}
	
Exit:
	if (textP)
		MemPtrFree(textP);
	if (curMessageP)
		MemPtrFree(curMessageP);
		
}




/***********************************************************************
 *
 * FUNCTION:    PrgStartDialogV31
 *
 * DESCRIPTION: Older version of PrgStartDialog for backwards compatibility.
 *				Display a progress dialog that can be updated by another
 *				process. The dialog can contain a cancel or OK button. This
 *				function returns a pointer to an allocated progress structure.
 *				The pointer must be passed to all other progress functions and
 *				MUST be released by calling PrgStopDialog.
 *
 *
 * CALLED BY:	 Any UI task routine that wants to display progress info 
 *
 * PARAMETERS:  textCallBack - pointer to callback function that will supply
 *								text and icons for the current progress state.
 *
 * RETURNED:    0 if no error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		----------- 
 *			gavin	8/13/97	Initial Revision
 *
 ***********************************************************************/
ProgressPtr	PrgStartDialogV31(const Char * titleP,PrgCallbackFunc textCallback)
{
	return PrgStartDialog(titleP, textCallback, NULL);
}

/***********************************************************************
 *
 * FUNCTION:    PrgStartDialog
 *
 * DESCRIPTION: Display a progress dialog that can be updated by another
 *				process. The dialog can contain a cancel or OK button. This
 *				function returns a pointer to an allocated progress structure.
 *				The pointer must be passed to all other progress functions and
 *				MUST be released by calling PrgStopDialog.
 *
 *
 * CALLED BY:	 Any UI task routine that wants to display progress info 
 *
 * PARAMETERS:	textCallBack - pointer to callback function that will supply
 *								text and icons for the current progress state.
 *					userDataP - context pointer passed with callback data so
 *								callback routine can reference its data.
 *
 * RETURNED:    0 if no error
 *
 * REVISION HISTORY:
 *			Name		Date		Description
 *			----		----		----------- 
 *			gavin		8/13/97	Initial Revision
 *			tlw		3/5/97	Added userDataP arg
 *			mgolden	3/18/98	check if for is already loaded, if so, then don't reload it, 
 *									increment the count in the data ptr of the gadget object
 *			meg		9/2/98	removed an 'extra' MemPtrNew()
 *
 ***********************************************************************/
ProgressPtr	PrgStartDialog(const Char * titleP,PrgCallbackFunc textCallback, void* userDataP)
{
	ProgressPtr		prgP;
	FormGadgetType *gadgetP;
	Boolean 			firstTime;
	
	prgP = MemPtrNew(sizeof(*prgP));
	ErrNonFatalDisplayIf(!prgP,"Unable to alloc progress data");
	if (!prgP) return 0;
	
	MemPtrSetOwner(prgP,0);
	MemSet(prgP, sizeof(*prgP), 0);
	
	// save the current form and info
	prgP->oldFrmP = FrmGetActiveForm();
	if (!prgP->oldFrmP) {
		prgP->oldDrawWinH = WinGetDrawWindow();
		prgP->oldActiveWinH = WinGetActiveWindow();
		}
	prgP->oldInsPtState = InsPtEnabled();
	InsPtGetLocation(&prgP->oldInsPtPos.x, &prgP->oldInsPtPos.y);
	
	// check if the form already is loaded
	prgP->frmP = FrmGetFormPtr (prgProgressFrm);
	if (prgP->frmP) 
		{ 
		firstTime = false;
		//it'a alrady loaded, increment the counter in the gadget objecft of the form
		gadgetP = FrmGetObjectPtr (prgP->frmP, FrmGetObjectIndex (prgP->frmP, prgProgressCounterGadget));
		gadgetP->data = (void *)((UInt16)gadgetP->data + 1);
		}
	else
		{
		firstTime = true;
		// Init the form
		prgP->frmP = FrmInitForm(prgProgressFrm);
		
		//set the counter to 1 (the gadget object)
		gadgetP = FrmGetObjectPtr (prgP->frmP, FrmGetObjectIndex (prgP->frmP, prgProgressCounterGadget));
		gadgetP->data = (void *)1;
		}

	ErrNonFatalDisplayIf(!prgP->frmP,"Unable to init progress form");
	if (!prgP->frmP) 
		{
		MemPtrFree(prgP);
		return 0;
		}
	MemPtrSetOwner(prgP->frmP,0);
	FrmSetActiveForm(prgP->frmP);

	// only the original call sets the title of the form
	if (firstTime)
		{
		if (titleP && *titleP)
			StrNCopy(prgP->title,titleP,progressMaxTitle);
		else
			prgP->title[0] = 0;
			
		if (prgP->title[0])
			FrmSetTitle (prgP->frmP,prgP->title);
		}
	else
		prgP->title[0] = 0;
		
	prgP->textCallback = textCallback;
	prgP->userDataP = userDataP;
	prgP->needUpdate = false;
	prgP->cancel = false;
	prgP->waitingForOK = false;
	prgP->error = 0;
	prgP->showDetails = 0;
	prgP->lastBitmapID = 0;
	
	prgP->stage = 0;
	
	prgP->needUpdate = true;				// to force an update in PrvConCheckevents

	prgP->timeout = 0;     					

	// If the down key is pressed, show progress details
	if (KeyCurrentState() & keyBitPageDown)
		prgP->showDetails = true;

	// Display it 
	if (!prgP->frmP->attr.visible)
		FrmDrawForm(prgP->frmP);

	return (prgP);

}		
	
/***********************************************************************
 *
 * FUNCTION:    PrgStopDialog
 *
 * DESCRIPTION: Removes a Progress dialog that was opened with PrgStartDialog.
 *
 * CALLED BY:	 Any UI task routine that wants to remove the progress dialog 
 *
 * PARAMETERS:  prgP - pointer to progress info Started with PrgStartDialog.
 *				force - if true do not wait for the user to confirm an error
 *
 * RETURNED:    0 if no error
 *
 * REVISION HISTORY:
 *			Name		Date		Description
 *			----		----		----------- 
 *			gavin		8/13/97	Initial Revision
 *			mgolden	3/18/98	don't close the form if the gadget counter is > 1
 *			dia		8/4/98	Updated for gremlins
 *			dia 		8/11/98	Now works (correctly) using the feature manager.
 *			dia		8/13/98	Decomposed out gremlins support.
 *
 ***********************************************************************/
void PrgStopDialog(ProgressPtr prgP,Boolean force)
{		
	FormGadgetType *gadgetP;

	ErrNonFatalDisplayIf(!prgP,"Null Progress Pointer");
	if (!prgP) return;
	
	if (prgP->waitingForOK && !force) {
		// wait until user oks the dialog
		EventType event;
		do {
			EvtGetEvent(&event,evtWaitForever);
		} while (PrgHandleEvent(prgP,&event));
	}
	
	//decrement the counter in the gadget object	
	gadgetP = FrmGetObjectPtr (prgP->frmP, FrmGetObjectIndex (prgP->frmP, prgProgressCounterGadget));
	gadgetP->data = (void *)((UInt16)gadgetP->data - 1);

	//check if we should destroy the form
	if (((UInt16)gadgetP->data) < 1)
		{		
		FrmEraseForm(prgP->frmP);
		InsPtSetLocation(prgP->oldInsPtPos.x, prgP->oldInsPtPos.y);
		FrmSetActiveForm(prgP->oldFrmP);
		FrmDeleteForm(prgP->frmP);
		if (!prgP->oldFrmP)
			{
			WinSetDrawWindow(prgP->oldDrawWinH);
			WinSetActiveWindow(prgP->oldActiveWinH);
			FrmSetActiveForm(prgP->oldFrmP);
			}
		}	

	//free the data either way
	MemPtrFree(prgP);
}
