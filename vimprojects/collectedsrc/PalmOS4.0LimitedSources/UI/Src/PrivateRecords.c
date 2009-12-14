/******************************************************************************
 *
 * Copyright (c) 1999-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: PrivateRecords.c
 *
 * Release: 
 *
 * Description:
 *	  This file contains security state selectors.
 *
 * NOTES:
 *
 * History:
 *		June 22, 1999	Created by Jameson Quinn
 *		07/07/00  WK    Added Password hinting functionality.
 *
 *****************************************************************************/

#include <PalmTypes.h>
#include <SystemPublic.h>

#include "Form.h"
#include "Password.h"
#include "PrivateRecords.h"
#include "UIResources.h"
#include "UIResourcesPrv.h"
#include "PasswordPrv.h"

#define NON_PORTABLE
#include "SystemPrv.h"

/***********************************************************************
 *
 *	Internal Constants
 *
 ***********************************************************************/
 
#define InvalidPasswordAlert			13250


/***********************************************************************
 *
 *	Internal Functions
 *
 ***********************************************************************/

static Boolean PrvCheckPW (Int16 button, Char * attempt);
static Boolean PrvVerifyPWDontSet (privateRecordViewEnum newSecLevel);


/***********************************************************************
 *
 * FUNCTION:    SecSelectViewStatus
 *
 * DESCRIPTION: Display a form showing the hide, mask and show options of private
 *				records and allow them to select it.
 *
 * PARAMETERS:  None.
 *
 * RETURNED:	The new Private Record View Status value.  See PrivateRecord.h for 
 *				possible values here.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			css		06/23/99	New
 *			aro		12/01/00	Fix Bug #23273 and replace original form save by curState to save the whole state rather than just the active form
 *
 ***********************************************************************/
privateRecordViewEnum SecSelectViewStatus (void)
	{
	FormType					*frm;
	privateRecordViewEnum 		oldView,returnValue;
	EventType					event;
	ListPtr	listP;
	UInt16	item;
	FormActiveStateType			curFrmState;

	oldView = returnValue = (privateRecordViewEnum)PrefGetPreference (prefShowPrivateRecords);
	
	// save active form/window state
	FrmSaveActiveState(&curFrmState);				
	
	frm = (FormType *) FrmInitForm (SecChangeViewForm);
	FrmSetActiveForm (frm);

	// Set the button of the current setting.
	listP = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, SecChangeViewCurPrivacyList));

	//	Convert the preference setting to it's UI list position:
	item = (Int16)PrefGetPreference(prefShowPrivateRecords);
	LstSetSelection (listP, item);
	CtlSetLabel (FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, SecChangeViewCurPrivacyPopTrigger)),
											LstGetSelectionText (listP, item));

	FrmDrawForm (frm);
	
	while (true)
		{
		EvtGetEvent (&event, evtWaitForever);

		if (SysHandleEvent (&event))
			continue;
			
		if (event.eType == appStopEvent)
			{
			// Cancel the dialog and repost this event for the app, also keep
			// the return value as the original value, so there is no change
			// and return control back to the caller.
			EvtAddEventToQueue(&event);
			break;
			}
			
		FrmHandleEvent (frm, &event);

		// If the start or end time buttons are pressed then change
		// the time displayed in the lists.
		if (event.eType == ctlSelectEvent)
			{
				
			// "Ok" button pressed?
			if (event.data.ctlSelect.controlID == SecChangeViewOkButton)
			{
				//already done:
				//returnValue = (privateRecordViewEnum) (FrmGetObjectId(frm, FrmGetControlGroupSelection (frm, SecChangeViewStatusGroup))
				//		 - SecChangeViewShowButton );

				// Fix bug #23273 - Erase the form before opening the next dialog to avoid flashing
				FrmEraseForm(frm);

				if (SecVerifyPW(returnValue))
				{
					break;
				}
				else
				{
					// Fix bug #23273 - Draw it again
					FrmDrawForm(frm);
				}

			}

			// "Cancel" button pressed?
			else if (event.data.ctlSelect.controlID == SecChangeViewCancelButton)
				{
				returnValue = oldView;
				break;
				}
			}
		else if (event.eType == popSelectEvent)
			{
			if (event.data.popSelect.listID == SecChangeViewCurPrivacyList)
				{
				item = event.data.popSelect.selection;
				returnValue = (privateRecordViewEnum)item;
				// leave handled false so that the trigger is updated
				}
			}
		}
		

	// The form might already have been erase, but FrmEraseForm handle that properly
	FrmEraseForm (frm);
	FrmDeleteForm (frm);

	// restore active form/window state
	FrmRestoreActiveState(&curFrmState);				
	
	return (returnValue);
}


/***********************************************************************
 *
 * FUNCTION:    PrvCheckPW
 *
 * DESCRIPTION: Callback for FrmCustomResponseAlert. Verifies the password.
 *				Returns true for cancel button or valid password; otherwise
 *				displays "try again" and returns false.
 *
 * PARAMETERS:  Button - 0=OK, 1=cancel
 *				attempt - user's attempted password entry, or NULL
 *
 * RETURNED:	true if the user has entered the password correctly or 
 *				pressed cancel.
 *
 *	HISTORY:
 *		07/23/99	jaq	Created by Jameson Quinn
 *		08/10/99	kwk	Lowercase password before calling PwdVerify.
 *		09/26/99	kwk	No longer need to do special lower-casing, since
 *							PwdVerify does that for us.
 *		09/30/99 jaq	changed to switch, added frmResponseCreate
 *
 ***********************************************************************/
static Boolean PrvCheckPW (Int16 button, Char * attempt)
{
	Char* hintP;
	
	switch (button)
		{
		case frmResponseCreate:
			TsmSetFepMode(NULL, tsmFepModeOff);
			return true;
			
			
		case 0: //OK
			if (!PwdVerify(attempt))
				{
				hintP = SecGetPwdHint();
				
				FrmCustomAlert(secInvalidPasswordAlert, hintP, NULL, NULL);
				MemPtrFree(hintP);
				return  false;			// Don't exit form
				}
			//no break... success falls through
		case 1: //Lost password
		case 2: //Cancel
		case frmResponseQuit:
		default:
			return true;
		}
}

/***********************************************************************
 *
 * FUNCTION:    	PrvVerifyPWDontSet
 *
 * DESCRIPTION:	Verifies the password after a user has tapped on a private 
 *						record.
 *
 * PARAMETERS:  	None.
 *
 * RETURNED:		True if the user has entered the password correctly and 
 *						pressed OK, all other conditions return false.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			css	06/23/99	New
 *
 ***********************************************************************/
Boolean PrvVerifyPWDontSet (privateRecordViewEnum newSecLevel)
{
	privateRecordViewEnum oldSecLevel = (privateRecordViewEnum)PrefGetPreference(prefShowPrivateRecords);
	UInt16		dialog;
	
	
	if (SysCheckPwdTimeout())
		return true;

	if (newSecLevel == oldSecLevel) //No change, always success
		return true;
		
	else if (newSecLevel < oldSecLevel && PwdExists()) //decreased security, password check
		{
		
		UInt16 dialogId = (newSecLevel == maskPrivateRecords) ?
				secShowMaskedPrivatePermanentPassEntryAlert : secShowPrivatePermanentPassEntryAlert;
		//else assume showPrivateRecords
		
		if (FrmCustomResponseAlert(dialogId, NULL, NULL, NULL, NULL, pwdLength, PrvCheckPW) != 0)
			//FrmCustomResponseAlert only terminates on cancels or valid OK's - this is a cancel
			return false;
		else
			//The user has just seen a dialog which explains masking to some degree. They don't want another.
			//break, rather than falling through to dialog.
			return true;
			
		}
	
	//password checks out, show alert and return true for OK
	switch (newSecLevel)
		{
		case maskPrivateRecords:
			dialog = secMaskRecordsAlert;
			break;
			
		case hidePrivateRecords:
			dialog = secHideRecordsAlert;
			break;
			
		default:
			return true; //no need for a dialog, just return from here
		}
		
	return (FrmAlert(dialog) == secHideMaskRecordsOK);
}

/***********************************************************************
 *
 * FUNCTION:    SecVerifyPW
 *
 * DESCRIPTION: Does whatever password verification is needed for a change
 *				of security level. Also does the actual setting the preference
 *
 * PARAMETERS:  newSecLevel - new security level.
 *
 * RETURNED:	true if the user has entered the password correctly and 
 *				pressed OK, all other conditions return false.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			css		06/23/99	New
 *
 ***********************************************************************/
Boolean SecVerifyPW (privateRecordViewEnum newSecLevel)
{
	if (PrvVerifyPWDontSet(newSecLevel))
		{
		PrefSetPreference(prefShowPrivateRecords,newSecLevel);
		return true;
		}
	else
		return false;
}
