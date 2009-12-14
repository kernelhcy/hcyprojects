/******************************************************************************
 *
 * Copyright (c) 1995-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: AboutBox.c
 *
 * Release: 
 *
 * Description:
 *	  This is the generic 'about box' that is used by the main applications.
 *
 * History:
 *		Oct 25, 1995	Created by Christopher Raff
 *
 *****************************************************************************/

#include <PalmTypes.h>
#include <SystemPublic.h>
#include "AboutBox.h"
#include "Form.h"
#include "UIResourcesPrv.h"

/***********************************************************************
 *
 * FUNCTION:	ReplaceStrInTemplate
 *
 * DESCRIPTION:	Utility routine for handling insertion into a template
 *				where the template has the format of "text ^1 textXXXX".
 *				The 'XXX' sequence at the end provides space for expansion
 *				in fixed length form strings (titles, labels, etc)
 *
 * PARAMETERS:	ioTemplate - pointer to template string, which we'll munge.
 *				inSubStr - pointer to string to substitute for ^1 in template.
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			kwk		03/16/99	Initial Revision
 *			vivek 	08/14/00 Fixed trunction problem if templates had
 *									text after the "XXX" place holder
 *									e.g. "text ^1 XXXXXXXXXtext"
 *			vivek 	08/14/0  Backed out earlier change.  Instead enforce
 *									it on localizers to always have templates with 
 *									"XXX" at the end and never in the middle.
 ***********************************************************************/

static void
ReplaceStrInTemplate(Char *ioTemplate, const Char *inSubStr)
{
	Int16 count, maxLen;
	Char * extraSpace;
	
	maxLen = StrLen(ioTemplate);
	extraSpace = StrStr(ioTemplate, "XXX");
	
	if (extraSpace != NULL)
		{
		*extraSpace = '\0';
		}
	
	count = TxtReplaceStr(ioTemplate, maxLen, inSubStr, 1);
	ErrNonFatalDisplayIf(count == 0, "No ^1 in template string");
} // ReplaceStrInTemplate


/***********************************************************************
 *
 * FUNCTION:		AbtShowAbout
 *
 * DESCRIPTION:	This routine displays the info dialog box.  The app name
 *				will be picked up from either the tAIN resource of the
 *				app, or the name of the application database.
 *
 * PARAMETERS:	creator - creator ID of this app
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art		4/11/95	Initial Revision
 *			kcr	10/25/95	converted to a generic routine
 *			kcr	11/8/95	use tAIN for app name.
 *			kcr	12/7/95	add app name to title; check length of version str.
 *			mlb	1/30/96	change scan for location of app name in title to look for '%'
 *                      instead of a blank (which caused localization problems).
 *			trm	08/07/97	made non modified passed variabls constant
 *			kwk	03/16/99	Re-worked to use templates w/real ^x parameters
 *							for the title, name, and version number. Also get
 *							real app info for the simulator.
 *			kwk	05/05/99	Use defined types for the simulator file creator/type.
 *			fpa	10/06/99	Only get latest version in DmGetNextDatabaseByTypeCreator
 *
 ***********************************************************************/
void AbtShowAbout (UInt32 creator)
	{
	FormType * 			frm;
	Char *				nameLabelStr;
	Char *				versLabelStr;
	Char *				nameTitleStr;
	Char				appName[dmDBNameLength];
	Char				appVers[16];
	UInt32				dbType;
	MemHandle				strH;
	Char *				strP;
	Err					err;
	DmOpenRef			dbP;
	DmSearchStateType	searchState;
	LocalID				dbID;
	UInt16				cardNo;
	
	frm = FrmInitForm (aboutDialog);

	// Set up pointers to strings we're going to be munging.
	// explicitly cast non-constant because we're going to
	// go in and modify them ourselves.
	nameTitleStr = (Char *) FrmGetTitle (frm);
	nameLabelStr = (Char *)FrmGetLabel (frm, aboutNameLabel);
	versLabelStr = (Char *)FrmGetLabel (frm, aboutVersionLabel);
	
#if EMULATION_LEVEL != EMULATION_NONE
	// In the simulator, the creator of the app.tres file is always sysFileCSimulator,
	// since this gets built at run-time, and we (currently) don't know the
	// creator.
	creator = sysFileCSimulator;
	dbType = sysFileTSimulator;
#else
	dbType = sysFileTApplication;
#endif

	//	This is involved - locate app's resource database, open it, fetch
	//	tAIN resource.  If it exists, use it as name.  If no tAIN
	//	resource, use name of 'app' database.  Close resource database.
#if EMULATION_LEVEL == EMULATION_MAC
	err = DmGetNextDatabaseByTypeCreator (true, &searchState,
			 			dbType, creator, false,
			 			&cardNo, &dbID);
#else
	err = DmGetNextDatabaseByTypeCreator (true, &searchState,
			 			dbType, creator, true,
			 			&cardNo, &dbID);
#endif
	if (err)
		goto exit;

	dbP = DmOpenDatabase(cardNo, dbID, dmModeReadOnly);
	if (!dbP)
		goto exit;

	strH = DmGet1Resource (ainRsc, ainID);
	if (strH)
		{
		strP = MemHandleLock (strH);
		StrCopy (appName, strP);
		MemHandleUnlock (strH);
		DmReleaseResource (strH);
		}
	else
		{
		DmDatabaseInfo(cardNo, dbID, appName, NULL, NULL, NULL, NULL, 
						NULL, NULL, NULL, NULL, NULL, NULL);
		}
	
	// Now grab the app's version number resource, while we've got
	// the database open.
	strH = DmGet1Resource (verRsc, appVersionID);
	if (strH == NULL)
		{
		strH = DmGet1Resource (verRsc, appVersionAlternateID);
		}
		
	if (strH == NULL)
		{
		strH = DmGetResource (strRsc, aboutErrorStr);
		}
	
	strP = MemHandleLock (strH);
	StrNCopy (appVers, strP, sizeof(appVers) - 1);
	MemHandleUnlock (strH);
	DmReleaseResource (strH);
	
	DmCloseDatabase(dbP);

	ReplaceStrInTemplate(nameLabelStr, appName);
	ReplaceStrInTemplate(nameTitleStr, appName);
	ReplaceStrInTemplate(versLabelStr, appVers);
	
	FrmDoDialog (frm);
	
exit:
 	FrmDeleteForm (frm);
	} // AbtShowAbout
