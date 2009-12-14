/******************************************************************************
 *
 * Copyright (c) 1994-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: ErrorMgr.c
 *
 * Release: 
 *
 * Description:
 *		Error Management routines for Pilot
 *
 * History:
 *   	10/25/94  RM - Created by Ron Marianetti
 *
 *****************************************************************************/

#define	NON_PORTABLE

#include <BuildDefines.h>
#ifdef HAS_LOCAL_BUILD_DEFAULTS
#include "LocalBuildDefaults.h"
#endif
#include <PalmTypes.h>

#include <DebugMgr.h>
#include <DateTime.h>
#include <ErrorMgr.h>
#include <SysEvent.h>
#include <FatalAlert.h>
#include <SystemMgr.h>
#include <Preferences.h>
#include <StringMgr.h>
#include <SysEvtMgr.h>
#include <SysUtils.h>
#include <Form.h>
#include <NetMgr.h>		// SPECIAL CASE
#include <KeyMgr.h>		// SPECIAL CASE

#include "SystemPrv.h"
#include "EmuStubs.h"
#include "Globals.h"
#include "HwrGlobals.h"		// for GHwrMiscFlags
#include "HwrMiscFlags.h"	// for hwrMiscFlagHasJerryHW

#include "UIResourcesPrv.h"

#define maxErrNeumonicLength		5

//#define _DEBUG_ERROR_MGR

#if EMULATION_LEVEL == EMULATION_NONE

/************************************************************
 *
 *  FUNCTION: ErrSetJump
 *
 *  DESCRIPTION: Saves PC and all processor registers except those that
 *		can change after calling a C routine into a structure of
 *		type ErrJumpBuf.
 *
 *		ErrSetJump is always used with ErrLongJump. After saving the
 *		context with ErrSetJump, an application can at any later time
 *		make a call to ErrLongJump and return immediately to the
 *		instruction following the ErrSetJump call with all
 *		processor registers and stack pointer set to the same values
 *		they had when the context was saved with ErrSetJump.
 *
 *		On return from ErrSetJump as a result of a ErrLongJump, the
 *		result will be non-zero.
 *
 *  PARAMETERS: Pointer to ErrJumpBuf structure
 *
 *  RETURNS: 0 when saving context, non 0 if returning from a ErrLongJump
 *
 *  CREATED: 3/2/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Int16 asm ErrSetJump(ErrJumpBuf buf)
{
		MOVE.L	(SP)+, A1							// get return address
		MOVE.L	(SP), A0							// pointer to buf
		MOVEM.L	D3-D7/A1-A7, (A0)					// save registers
		MOVEQ	#0, D0								// result code
		JMP		(A1)								// return to caller
}


/************************************************************
 *
 *  FUNCTION: ErrLongJump
 *
 *  DESCRIPTION: Restores PC and all processor registers except those that
 *		can change after calling a C routine from a structure of
 *		type ErrJumpBuf.
 *
 *		ErrSetJump is always used with ErrLongJump. After saving the
 *		context with ErrSetJump, an application can at any later time
 *		make a call to ErrLongJump and return immediately to the
 *		instruction following the ErrSetJump call with all
 *		processor registers and stack pointer set to the same values
 *		they had when the context was saved with ErrSetJump.
 *
 *		On return from ErrSetJump as a result of a ErrLongJump, the
 *		result will be non-zero.
 *
 *  PARAMETERS: Pointer to ErrJumpBuf structure, non-zero result code
 *		for "return" from ErrSetJump
 *
 *  RETURNS: void
 *
 *  CREATED: 3/2/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
void asm ErrLongJump(ErrJumpBuf buf, Int16 result)
{
		ADD.W	#4, SP							// ignore return address
		MOVE.L	(SP)+, A0						// get pointer to buf
		MOVE.W	(SP), D0						// result code
		BNE.S	@0								// if non-zero -->
		MOVEQ	#1, D0							// force non-zero
@0:
		MOVEM.L	(A0), D3-D7/A1-A7				// restore registers
		JMP		(A1)							// return to original PC
}


#endif // EMULATION_LEVEL == EMULATION_NONE

/************************************************************
 *
 *  FUNCTION: ErrExceptionList
 *
 *  DESCRIPTION: Returns the address of the pointer to the
 *   first ErrExceptionType structure linked into the
 *   exception list. Used by the ErrTry, ErrCatch, and ErrThrow
 *   macros in order to find the exception list.
 *
 *   When called from an application task, this routine returns
 *   the application's exception list which is stored in the
 *   SysAppInfoPtr of the app. When called from a system thread,
 *   this routine returns the system's exception list, which is
 *   stored in a low memory global.
 *
 *  PARAMETERS: none
 *
 *  RETURNS: address of the pointer.
 *
 *  CREATED: 5/16/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
MemPtr* ErrExceptionList(void)
{
	SysAppInfoPtr appInfoP;
	
	// We do this so we don't have to link in the SystemMgr.c module
	//  with every app on the host that uses the Error Manager. Under emulation
	//  mode, there are no separate tasks so we might as well use the
	//  the System List stored at GErrFirstExceptionP.
	#if EMULATION_LEVEL == EMULATION_NONE
	SysAppInfoPtr unusedAppInfoP;
	
	appInfoP = SysGetAppInfo(&unusedAppInfoP, &unusedAppInfoP);
	#else
	appInfoP = (SysAppInfoPtr)GCurUIAppInfoP;
	#endif
	
	return &appInfoP->errExceptionP;
}




/************************************************************
 *
 *  FUNCTION: ErrThrow
 *
 *  DESCRIPTION: A Throw causes a jump to the nearest Catch block.
 *
 *  PARAMETERS: error code
 *
 *  RETURNS: never returns
 *
 *  CREATED: 3/2/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
void ErrThrow(Int32 err)
{
	ErrExceptionType*	tryP;
	
	tryP = (ErrExceptionPtr)*ErrExceptionList();
	
	if (tryP) {
		tryP->err = err;
		ErrLongJump(tryP->state, 1);
		}
		
	else {
		ErrDisplay("ErrThrow called without a pending ErrTry");
		}

}


/************************************************************
 *
 * FUNCTION:	PrvStrNCat
 *
 * DESCRIPTION: Concatenates 1 string to another clipping the
 *		destination string to a max of N characters (including null
 *		at end).
 *
 * PARAMETERS:
 *		dstP	<-> String to receive catenated result.
 *		srcP	 -> String to catenate to the end of dstP.
 *		n		 -> Max length of resulting string, including null byte.
 *
 * RETURNS:		nothing
 *
 * HISTORY:
 *		11/22/00	kwk	New from 3.1/4.0 version of StrNCat.
 *
 *************************************************************/
static void PrvStrNCat(Char* dstP, const Char* srcP, Int16 n)
{
	Char*		tmp = dstP;
	UInt16	len;
	
	len = StrLen(dstP);

	// Do the cat. We need enough space to copy at least one byte,
	// plus the terminating null.
	if (len < n - 1)
	{
		tmp += len;
	
		while (*srcP && (len < n - 1))
		{
			*tmp++ = *srcP++;
			len++;
		}
	
		*tmp = '\0';
	}
} // PrvStrNCat
 

/************************************************************
 *
 * FUNCTION:	ErrDisplayFileLineMsg
 *
 * DESCRIPTION: Displays an alert with an error message that
 *		contains the passed file name, line number, and message.
 *
 * PARAMETERS:
 *		filename	 ->	Name of C/C++ source file that generated
 *							call to this routine.
 *		lineNo	 ->	Line number in source file of call.
 *		msg		 ->	Error message.
 *
 * RETURNS:		nothing
 *
 * HISTORY:
 *		10/25/94	RM		Created by Ron Marianetti
 *		01/20/98	vmk	Added check for fatalDoNothing, return to caller.
 *		11/22/00	kwk	Reduce stack usage by 152 bytes, by constraining
 *							max length of filename & message. Also use
 *							private routines for string catenation, to
 *							remove dependency on Text Mgr initialization.
 *
 *************************************************************/

// DOLATER kwk - using up a lot of stack space in order to display a
// fatal alert is a bad idea. Another approach would be to reserve
// space for the text message in a low-memory global array, thus
// eliminating the remaining 100+ bytes of temp stack space.
#define	maxErrFilenameLen	31		// 
#define	maxErrMessageLen	64		// The is actually the minimum constrained length, since
											// the file name and line #s will probably be shorter.
#define	maxStrUInt16Len	5		// "65535"
#define	textBufferLen		(maxErrFilenameLen+7+maxStrUInt16Len+2+maxErrMessageLen)	// "<filename>, Line:<number>, <msg>"

void ErrDisplayFileLineMsg(const Char * const filename, UInt16 lineNo, const Char * const msg)
{
#if (EMULATION_LEVEL != EMULATION_NONE)
	StubErrDisplayFileLineMsg(filename, lineNo, msg);
#else
	UInt16	fatalAlertResult;
	Char		text[textBufferLen + 1];
	SysEventType	event;
	
	// Form the error message. Use PrvStrNCat everywhere to reduce the
	// number of utility routines we need to copy into this file.
	text[0] = '\0';
	PrvStrNCat(text, filename, maxErrFilenameLen + 1);
	PrvStrNCat(text, ", Line:", textBufferLen + 1);
	StrIToA(text + StrLen(text), lineNo);
	PrvStrNCat(text, ", ", textBufferLen + 1);
	PrvStrNCat(text, msg, textBufferLen + 1);

	// If the UI has not been initialized yet, we can't put up an alert
	//  so we'll force entry into the Debugger.
	// DOLATER kwk - shouldn't this check be in SysFatalAlert? Also
	// check for interrupt level; currently AMX kernal generates fatal
	// alert if 68K vector checksum has changed, but you can't safely
	// draw at that time.
	if (!(GSysMiscFlags & sysMiscFlagUIInitialized)) {
		GDbgWasEntered |= dbgEnteredFlagTrue;
		DbgBreak();
		}
				
		
	// If the debugger was entered already, go to it
	if (GDbgWasEntered & dbgEnteredFlagTrue) {
		DbgMessage(text);
		DbgMessage("\n");
		DbgBreak();
		}
		
	// Else, show a fatal alert
	else {
		// If we re-entered, something terrible is wrong so just loop indefinitely so that
		//  we don't eat up stack space.
		if (GSysMiscFlags & sysMiscFlagInFatalAlert) 
			while(1)	
				;
				
		// Set flag to detect re-entry
		GSysMiscFlags |= sysMiscFlagInFatalAlert;
	
	
		// Display alert and reset if the first button (0) is pressed.
		if ( (fatalAlertResult = SysFatalAlert(text)) == fatalReset )
		{
#ifdef _DEBUG_ERROR_MGR		
			DbgMessage("Reset Pressed\n");
#endif			
			SysReset();
		}
			
		// If the the second button(1) is pressed (can only happen with
		//  full error checking on) go into Debugger.
		else if ( fatalAlertResult != fatalDoNothing )
		{
#ifdef _DEBUG_ERROR_MGR			
			DbgMessage("Debug Pressed\n");
#endif			
			DbgBreak();
			
		}
#ifdef _DEBUG_ERROR_MGR			
		else
			DbgMessage("Continue Pressed\n");
#endif
			
		// Flush all events out. 
		do {
			SysEventGet(&event, 1);
			} while (event.eType != nilEvent);
			
		// Clear re-entry detector
		GSysMiscFlags &= ~sysMiscFlagInFatalAlert;
		}
		
		

#endif
}

/************************************************************
 *
 *  FUNCTION: Prv4HToI
 *
 *  DESCRIPTION: Converts a 4 character hex string into an integer
 *			Used exclusively by ErrAlertCustom below
 *
 *  PARAMETERS: 
 *			str	-> start of 4 character hex string
 *
 *  RETURNS: integer value of str
 *
 *  CREATED: 2/25/98
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
static UInt16	Prv4HToI(Char * strP)
{
	UInt16	hex = 0;
	UInt16	digit;
	Int16	i;
	Char	c;
	
	for (i=0; i<4; i++) {
		c = *strP++;
		if (c >= 'a') 
			digit = c - 'a' + 10;
		else if (c >= 'A')
			digit = c - 'A' + 10;
		else 
			digit = c - '0';
			
		ErrNonFatalDisplayIf(digit > 0x0F, "invalid hex string");
		hex = (hex << 4) + digit;
		}
		
	return hex;
			
}

#if EMULATION_LEVEL != EMULATION_WINDOWS

/************************************************************
 *
 *  FUNCTION: ErrAlertCustom
 *		NOT present in PalmOS <= 3.0
 *
 *  DESCRIPTION: 
 *
 *		This routine can be used to display an alert for normal
 *		run-time errors that occur in an application. This is most
 *		likely to be used by network applications to display errors
 *		like "server not found", "network down", etc. In most cases
 *		the alert will simply have an OK button and execution will
 *		continue when the OK button is pressed.
 *
 *		This routine will lookup the text assoicated with the given
 *		error code 'errCode' and display that in the Alert. If 'errMsgP'
 *		is not nil however, that text will be used in place of the
 *		text associated with 'errCode'. In addition, the 'preMsgP' and
 *		'postMsgP' text strings, if specified, will be pre-pended and
 *		post-pended respectively. 
 *
 *		The text associated with an error code is looked up from a
 *		'tSTL' resource using the SysErrString() routine. A 'tSTL'
 *		resource contains a list of strings that can be looked up
 *		by index. All of the system defined error code strings are
 *		defined in the 'tSTL' resources numbered 10000 -> (10000+errClass) where
 *		'errClass' is the upper 8 bits of the error number. For example,
 *		the error strings for the DataManager (0x0200 -> 0x02FF) are
 *		found in 'tSTL' resource number 10002. 
 *
 *		The text associated with application specific error codes
 *		are always numbered 0x8000 -> 0x80FF (appErrorClass + X). These
 *		are always looked up from a 'tSTL' resource number 0 contained
 *		within the application itself. 
 *
 *		NOTE: The following paragraph is NOT IMPLEMENTED YET: 
 *		For system error codes that correspond to Libraries (like
 *		netErrorClass, inetErrorClass, etc.) This routine will first
 *		check for a 'tSTL' #0 resource in the library database itself before
 *		checking the appropriately numbered system 'tSTL' resource. In
 *		this way, libraries can contain their own error strings and don't
 *		have to rely on the error strings in the system resource file.
 *		Unfortunately, there is no programmatic way of determining if an
 *		error number corresponds to a library or not. This routine thus
 *		must be updated whenever a new error class is defined for a new
 *		library. 
 *		
 *
 *    Button Command Strings
 *		---------------------------------------------------------------
 *		By default, only an OK button will be displayed in the alert and
 *		no particular action will be taken when the OK button is pressed.
 *		But, a cancel button can be added and special actions can be
 *		specified through control strings inserted in the front of the
 *		error text. Here's the format of the control sequence:
 *
 *		"<" <btnID> [<cmdChar> <cmdVars>] ">"
 *
 *		The <btnID> can be either 0 or 1. 0 is always an OK button and
 *		1 is a Cancel button. If a control sequence with a btnID if 1
 *		is not specified, then the alert will not show a Cancel button. 
 *
 *		The <cmdChar> is a single character command. The only two currently
 *		supported are:  'K' and 'S'. The 'K' command will cause a key event
 *		to be posted to the event queue and the 'S' command will cause 
 *		an application switch through posting a quit event. 
 *
 *		Here's an example text string that specifies an OK and a Cancel button
 *		and makes a switch to the Memo application if the user hits the OK
 *		button:
 *			"<0Smemoappl0000><1>Hit OK to switch to Memos, Cancel to ignore"
 *
 *		Here are the formats of the commands:
 *
 *		Key Command: "K" <ascii> <keyCode> <modifiers>
 *			where <ascii> <keyCode>, and <modifiers> are 4 character hex
 *			strings that specify the various fields of the key event. For
 *			example:
 *				"K011300000008"
 *			will generate a key event with an ascii code of 0x0113 (backlightChr)
 *			a keyCode of 0x0000, and a modifiers of 0x0008 (commandKeyMask).
 *
 *		SwitchApp command: "S" <creator> <type> <launchCmd>
 *			where <creator> and <type> are the 4 character creator/type
 *			of the new application to switch to. 
 *			launchCmd is a 4 character hex string that specifies the
 *			action code to send the application: For example:
 *				"Smemoappl0004" 
 *			will make a call to SysUIAppSwitch to have it launch the
 *			Memo application with a launch code of 4. 
 *		
 *
 *    Aliased Error Strings
 *		---------------------------------------------------------------
 *		Another possible control sequence that can appear as the error
 *		text is:
 *
 *			"<" "*" <aliasErrCode> ">"
 *
 *		This sequence acts as an alias to another error code. The <aliasErrCode>
 *		is a 4 character hex string representing the error code to alias to.
 *		This is useful when two or more error codes should display the
 *		same error string. 
 *		example:
 *			"<*1437>"
 *		will make this routine lookup the error string used by the 0x1437
 *		error code. 
 *
 *
 *  PARAMETERS: 
 *		errCode		-> 16 bit error code to display
 *		errMsgP		-> if not NULL, then this text will be used in place
 *								of the associated 'errCode' text from the string
 *								resource
 *		preMsgP		-> if not NULL, text to prepend to error text
 *		postMsgP		-> if not NULL, text to append to error text
 *		
 *
 *  RETURNS: index of button pressed:
 *						0 is always the OK button
 *						1 is the Cancel button, if present. 
 *
 *  CREATED: 2/25/98
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt16	ErrAlertCustom(Err errCode, Char * errMsgP, Char * preMsgP, 
			Char *	postMsgP)
{
	const		Int16	maxErrLen = errMaxMsgLength+1;
	Char *	errBufP = 0;
	UInt16	result = 0;
	Char		emptyString[1];
	UInt16	index;
	UInt16	resID;
	Char *	cP;
	UInt16	alertID;
	Err		tmpErr;
	Int16		aliasIndex;
	Err		tmpErrCode;
	Char		neumonicString [maxErrNeumonicLength + 1];
	
	// Information parsed from error Text
	Boolean					parseErr = false;
	Boolean					hasCancel = false;
	Char *					okCmdP = 0;
	Char *					cancelCmdP = 0;
	Char *					cmdP = 0;
	
	UInt16						keyAscii, keyCode, keyMod;
	UInt32						appCreator, appType;
	
	// Used for doing appSwitch
	DmSearchStateType		dmState;
	UInt16						cardNo;
	LocalID					dbID;
	UInt16						launchCmd;
	UInt16						okAlertID;
	UInt16						okCancelAlertID;
	UInt16						cancelAlertID;
	UInt16						errClass;
	
	
	
	
	// Allocate a buffer to hold the error text
	errBufP = MemPtrNew(maxErrLen);
	ErrNonFatalDisplayIf (!errBufP, "out of memory");
	if (!errBufP) goto Exit;
	
	//==============================================================
	// Very Very First, Determine the buttons to use.
	//==============================================================
	errClass = errCode & 0xFF00;
	// Must use an If statement here because the complier otherwise generates code that 
	// cannot link
	if ((errClass==errInfoClass) || (errClass==netErrorClass) || (errClass==inetErrorClass) ||
		(errClass==webErrorClass) || (errClass==pwrErrorClass) )
		
	{
	    okAlertID 			= InfoOKAlert;
	    okCancelAlertID 	= InfoOKCancelAlert;
	    cancelAlertID 	= InfoCancelAlert;
	    if (errClass == errInfoClass) errCode = 0;  // ignore errors in this class
	} else
	{
	    okAlertID 			= ErrOKAlert;
	    okCancelAlertID 	= ErrOKCancelAlert;
	    cancelAlertID 	= ErrCancelAlert;
	}
	
	//==============================================================
	// First, lookup the error text if not specified in the 'errMsgP'
	//  parameter
	//==============================================================
	if (!errMsgP) {
		errMsgP = errBufP;
		
		tmpErrCode = errCode;
		
		// Loop till we get the error string. If we find an
		//  alias escape code, we need to follow that to get
		//  the error string. We only follow a limited number of
		//  aliases in order to avoid recursion. 
		for (aliasIndex = 0; aliasIndex < 5; aliasIndex++) {
		
			// Get the index of the error string
			index = tmpErrCode & 0x00FF;
			
			//----------------------------------------------------------
			// Compute which resource ID to fetch
			//----------------------------------------------------------
			// If it's not an application specific error code, get it
			//  out of the appropriate system string resource. 
			if ((tmpErrCode & 0xFF00) != appErrorClass) 
				resID = sysResIDErrStrings + ((tmpErrCode >> 8) & 0x00FF);
				
			// For application custom errors, get it out of the 
			//  #0 resource in the application database. 
			else
				resID = 0;
				
				
			//----------------------------------------------------------
			// Read in the error text string
			//----------------------------------------------------------
			cP = SysStringByIndex(resID, index, errBufP, maxErrLen);
			if (!cP) 
				errBufP[0] = 0;
				
			// If it's an alias string, follow it
			if (cP[0] == '<' && cP[1] == '*') 
				tmpErrCode = Prv4HToI(cP+2);
			else
				break; // out of for(aliasIndex; ;) loop
			}
			
		}
		
	// If the app provided error text, copy it into our temp buffer
	//  so that we can append the error number to it. 
	else { // (errMsgP)  
		if (StrLen(errMsgP) < maxErrLen) {
			StrCopy(errBufP, errMsgP);
			errMsgP = errBufP;
			}
		}
		
		
		
	//==============================================================
	// If room, append the hex value of the error code
	//==============================================================
	if (errCode != 0 && (StrLen(errMsgP) + maxErrNeumonicLength + 8 < maxErrLen)) 
	{
		Char	numStr[10];

		StrCat (errMsgP, " (");

		if ((errCode & 0xFF00) == appErrorClass)
		{
		  StrCat (errMsgP, "APP");
		}
		else
		{
		  StrCat (errMsgP, SysStringByIndex (sysResIDErrStrings, ((errCode >> 8) & 0x00FF) - 1, neumonicString, maxErrNeumonicLength + 1));
		}

		// Place a space between the two fields that represent the error.
		StrCat (errMsgP, " ");
		StrIToH (numStr, errCode);
		// We only want the last four digits of the number because converstion of the hex value gives eight digits
		// with leading 0's if needed.
		StrCat(errMsgP, &numStr [4]);
		StrCat(errMsgP, ")");
	}


	//==============================================================
	// Look for and parse out any escape sequences in the error text
	//==============================================================
	cP = errMsgP;
	while (!parseErr && *cP == '<') {
		cP++;

		// Get the button ID and save ptr to command string
		switch (*cP) {
			case '0': 
				okCmdP = cP + 1; 
				break;
			case '1': 
				cancelCmdP = cP + 1; 
				hasCancel = true;
				break;
			case 'T':
			  switch (*(cP + 1))
			  {
			    case 'E':
			    	okAlertID 			= ErrOKAlert;
	    			okCancelAlertID 	= ErrOKCancelAlert;
	    			cancelAlertID 		= ErrCancelAlert;
			      break;
			    
			    case 'I':
	    			okAlertID 			= InfoOKAlert;
	    			okCancelAlertID 	= InfoOKCancelAlert;
	    			cancelAlertID 		= InfoCancelAlert;
			      break;
			      
			    case 'W':
	    			okAlertID 			= WarningOKAlert;
	    			okCancelAlertID 	= WarningOKCancelAlert;
	    			cancelAlertID 		= WarningCancelAlert;
			      break;
			      
			    case 'C':
	    			okAlertID 			= ConfirmationOKAlert;
	    			okCancelAlertID 	= ConfirmationOKCancelAlert;
	    			cancelAlertID 		= ConfirmationCancelAlert;
			      break;
			  }
			  break;
			  
			default:
				ErrNonFatalDisplay("invalid btn ID");
				parseErr = true;
				break;
			}
		if (parseErr) break;
		
		// Skip to end of command
		while (*cP && *cP != '>') 
			cP++;
		if (*cP == '>') cP++;
		
		errMsgP = cP;
		}

	
	//==============================================================
	// Display the Alert now
	//==============================================================
	
	// FrmCustomAlert doesn't tolerate null string pointers, so
	// substitute empty strings for them
	emptyString[0] = 0;
	if (!preMsgP) preMsgP = emptyString;
	if (!postMsgP) postMsgP = emptyString;
	
	// Display the alert
	if (hasCancel) alertID = okCancelAlertID;
	else alertID = okAlertID;
	
	// Only perform this if we are a jerry device
	if ((GHwrMiscFlags & hwrMiscFlagHasJerryHW) && (errCode == netErrAntennaDown))
	{
	  // Here is the SPECIAL CASE of the day.  The antenna up error is handled
	  // special here, because it is special.
	  // If this is the antenna down error alert, then we must notify the rest 
	  // of the system (specifically the KeyMgr) so that if the antenna is 
	  // switched up it will act as if they pressed the OK button on the form
	  // which actually does not exist because the antenna up is the only way
	  // that the OK button is represented.  Therefore there is a special alert
	  // form created for this error which on has a cancel button on it and
	  // when we return the button pressed back to the caller we must check if
	  // the antenna is up in order to determine how the form was exited to
	  // send the correct "button" pressed back to the caller.
//mgmg
	  //GCommActivityFlags |= sysCommActivityAntennaAlert;
	  alertID = cancelAlertID;
	}
		  
	result = FrmCustomAlert(alertID, preMsgP, errMsgP, postMsgP);
	
	// Now determine the correct result for the antenna down error.
	// Back to the SPECIAL CASE.
	if ((GHwrMiscFlags & hwrMiscFlagHasJerryHW) && (errCode == netErrAntennaDown))
	{
	  // Reset the global indication flag.
//mgmg
//	  GCommActivityFlags &= ~sysCommActivityAntennaAlert;
	  
	  // Since the error was because the antenna was down, we can now presume that
	  // if the antenna is up that the user used the antenna up switch to 
	  // get out of the dialog and that is equivelent to the OK button upon
	  // return.
	  if (KeyCurrentState () & keyBitAntenna)
	  {
	    result = 0;	// OK
	  }
	  else
	  {
	    result = 1;	// Cancel
	  }
	}
	
	//==============================================================
	// Take appropriate actions depending on which button was pressed
	//==============================================================
	if (result == 0 && okCmdP) 
		cmdP = okCmdP;
	else if (result == 1 && cancelCmdP)
		cmdP = cancelCmdP;
	else
		cmdP = 0;
		
	if (cmdP) {
		switch (*cmdP) {
			case '>': break;			// Empty Command
			
			//------------------
			// Post Key Event
			//------------------
			case 'K':					 
				cmdP++;
				keyAscii = Prv4HToI(cmdP);
				cmdP += 4;
				keyCode = Prv4HToI(cmdP);
				cmdP += 4;
				keyMod = Prv4HToI(cmdP);
				cmdP += 4;
				
				// The '>' should follow
				ErrNonFatalDisplayIf(*cmdP != '>', "invalid 'K' sequence");
				
				// Perform the action
				EvtEnqueueKey(keyAscii, keyCode, keyMod);
				break;
				
			//------------------
			// App switch	
			//------------------
			case 'S':					
				cmdP++;
				MemMove((Char *)&appCreator,cmdP,sizeof(appCreator));
				cmdP += 4;
				MemMove((Char *)&appType,cmdP,sizeof(appType));
					cmdP += 4;
				launchCmd = Prv4HToI(cmdP);
				cmdP += 4;
				
				// The '>' should follow
				ErrNonFatalDisplayIf(*cmdP != '>', "invalid 'S' sequence");

				tmpErr = DmGetNextDatabaseByTypeCreator(true, &dmState,
						appType, appCreator, true, &cardNo, &dbID);
				if (!tmpErr && dbID) 
					SysUIAppSwitch(cardNo, dbID, launchCmd, 0);
				break;
				
			} // switch
				
		}
		
		
Exit:
	
	if (errBufP) MemPtrFree(errBufP);
	return result;
}

#endif //EMULATION_LEVEL != EMULATION_WINDOWS
