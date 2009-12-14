/******************************************************************************
 *
 * Copyright (c) 1994-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: Launcher.c
 *
 * Release: 
 *
 * Description:
 *	  These are the routines for the launcher.
 *
 * History:
 *		April 21, 1995	Created by Roger Flores
 *		July 21, 2000	Deleted (mostly) by Bob Ebert
 *
 *****************************************************************************/

#include <PalmTypes.h>

// system public includes
#include <DataMgr.h>
#include <ErrorMgr.h>
#include <SystemResources.h>
#include <SystemMgr.h>
#include <Preferences.h>

// UI public includes
#include "Launcher.h"

/***********************************************************************
 *
 * FUNCTION:    SysAppLauncherDialog
 *
 * DESCRIPTION: Display the launcher, get a choice, ask the system
 *		to launch the selected app, clean up, and leave.
 *		If there are no apps to launch then nothing is done.
 *
 * PARAMETERS:	 nothing
 *
 * RETURNED:	 The system may be asked to launch an application.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	4/21/95	Initial Revision
 *			roger	5/5/96	Changed to use SysCreateDataBaseList
 *			bob	7/21/00	Delete 'old' launcher, launch real launcher instead
 *
 ***********************************************************************/

extern void SysAppLauncherDialog()
{
	LocalID dbID;
	UInt16 cardNo;
	DmSearchStateType state;
	UInt32 creator;
	
	ErrNonFatalDisplay("Launcher dialog is gone");
	
	creator = PrefGetPreference(prefLauncherAppCreator);
	
	// use the default OS launcher if there is no pref, or if the preferred app is not found
	if ((creator == sysFileCNullApp) ||
		(dmErrCantFind == DmGetNextDatabaseByTypeCreator(true, &state, sysFileTApplication,
		    															 creator, true, &cardNo, &dbID)))
	{
		DmGetNextDatabaseByTypeCreator(true, &state, sysFileTApplication, sysFileCLauncher, true, &cardNo, &dbID);
	}
	
	SysUIAppSwitch(cardNo, dbID, sysAppLaunchCmdNormalLaunch, NULL);
}
