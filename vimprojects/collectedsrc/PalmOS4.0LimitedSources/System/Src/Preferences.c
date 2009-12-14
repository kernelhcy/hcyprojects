/******************************************************************************
 *
 * Copyright (c) 1995-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: Preferences.c
 *
 * Release: 
 *
 * Description:
 *	  This file contains the system preference access routines.  There
 * are two preference files depending on whether the data should be
 * automatically backuped during a synch and get restored if missing or
 * if the data shouldn't be backupped because it either doesn't make
 * sense, or the application's conduit wants to backup the data itself.
 *
 * History:
 *		02/31/95	rsf	Created by Roger Flores.
 *		05/03/00	CS		Include PalmCompatibility.h to get CountryType, etc.
 *		05/16/00	CS		LmCountryType/LmLanguageType are now back to
 *							CountryType/LanguageType.
 *
 *****************************************************************************/

#define NON_PORTABLE

#include <PalmTypes.h>

#include "IntlPrv.rh"
	
// public System includes
#include <Chars.h>
#include <ErrorMgr.h>
#include <FeatureMgr.h>
#include <PalmLocale.h>				// cUnitedStates & lEnglish
#include <Preferences.h>
#include <PrivateRecords.h>
#include <SoundMgr.h>
#include <OverlayMgr.h>
#include <UIResources.h>

#include <PalmUtils.h>

// private System includes
#include "SystemPrv.h"
#include "SystemResourcesPrv.h"	// sysSoftResetAppCreatorConstId
#include "HwrGlobals.h"
#include "HwrMiscFlags.h"

/***********************************************************************
 *
 * FUNCTION: 	 GetPreferenceDBName
 *
 * DESCRIPTION: Returns a pointer to the name the the preference database.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:	null-terminated name to the preference database.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/21/95	Initial Revision
 *			kwk	07/05/99	Use const Char*, punt asm routine.
 *
 ***********************************************************************/
 
static const Char* GetPreferenceDBName (void)
{
	return((const Char*)"Unsaved Preferences");
}


/***********************************************************************
 *
 * FUNCTION: 	 GetPreferenceDBName
 *
 * DESCRIPTION: Returns a pointer to the name the the preference database.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:	null-terminated name to the saved preference database.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/17/96	Initial Revision
 *			kwk	07/05/99	Use const Char*, punt asm routine.
 *
 ***********************************************************************/
 
static const Char* GetSavedPreferenceDBName (void)
{
	return((const Char*)"Saved Preferences");
}


/***********************************************************************
 *
 * FUNCTION: 	 PrefOpenPreferenceDBV10
 *
 * DESCRIPTION: Returns a MemHandle to the preference database.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:	MemHandle or 0 on error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *       art   3/21/95  Initial Revision
 *
 ***********************************************************************/
 
extern DmOpenRef PrefOpenPreferenceDBV10 (void)
	{
	Err	err;
	LocalID	dbID;
	DmOpenRef	dbP;
	const Char * dbName;

	dbName = GetPreferenceDBName ();
	dbID = DmFindDatabase (0, dbName);

	if (!dbID)
		{
		err = DmCreateDatabase (0, dbName, sysFileCSystem, 
			sysFileTSavedPreferences, dmHdrAttrResDB);
		if (err) return 0;
		dbID = DmFindDatabase(0, dbName);
		}

	dbP = DmOpenDBNoOverlay(0, dbID, dmModeReadWrite);

	return dbP;
	}


/***********************************************************************
 *
 * FUNCTION: 	 PrefOpenPreferenceDB
 *
 * DESCRIPTION: Returns a MemHandle to the preference database.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:	MemHandle or 0 on error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *       art   3/21/95  Initial Revision
 *       roger	6/17/96  Revised to MemHandle saved and unsaved preferences
 *       roger	6/18/96  Revised to lookup by creator and type
 *			kwk	07/05/99	Use DmOpenDBNoOverlay to skip overlay check.
 *
 ***********************************************************************/
 
extern DmOpenRef PrefOpenPreferenceDB (Boolean saved)
{
	Err err;
	DmOpenRef dbP = NULL;
	const Char * dbName;
	UInt32 type;
	UInt16 cardNo;
	LocalID dbID;
	DmSearchStateType	searchState;

	type = saved ? sysFileTSavedPreferences : sysFileTPreferences;
	
	// Open the database.  Create it if it doesn't exist.
	
	err = DmGetNextDatabaseByTypeCreator (true, &searchState, type, 
			sysFileCSystem, true, &cardNo, &dbID);
	
	if (err == errNone)
		{
		dbP = DmOpenDBNoOverlay(cardNo, dbID, dmModeReadWrite);
		err = DmGetLastErr();
		}
	
	if (dbP == NULL)
		{
		// Insure that the err was because the database doesn't exist as opposed
		// to it being already open for writing.
		ErrFatalDisplayIf(err && err != dmErrCantFind, "Pref DB Open Error");
		
		if (saved)
			dbName = GetSavedPreferenceDBName ();
		else
			dbName = GetPreferenceDBName ();

		err = DmCreateDatabase (0, dbName, sysFileCSystem, type, dmHdrAttrResDB);
		if (err)
			return NULL;
		
		DmGetNextDatabaseByTypeCreator (true, &searchState, type, 
			sysFileCSystem, true, &cardNo, &dbID);
		dbP = DmOpenDBNoOverlay(cardNo, dbID, dmModeReadWrite);
		
		// Set the backup bit for the conduit to backup the saved database.
		if (saved)
			{
			UInt16 attributes;
			
			DmDatabaseInfo(cardNo, dbID, NULL, &attributes, NULL, NULL, 
				NULL, NULL, NULL, NULL, NULL, NULL, NULL);
			attributes |= dmHdrAttrBackup;
			DmSetDatabaseInfo(cardNo, dbID, NULL, &attributes, NULL, NULL, 
				NULL, NULL, NULL, NULL, NULL, NULL, NULL);
			}
		}


	return dbP;
}


/***********************************************************************
 *
 * FUNCTION: 	 PrvMakePrefDBDirty
 *
 * DESCRIPTION: Resource databases do not have to have a dirty flag and 
 * their modification numbers are only updated through creating new databases, 
 * attaching, detaching, or resizing.  This function will increment the 
 * mod number to force a change in modification time stamp.
 *
 * PARAMETERS:  dbP	Preference DB that needs to be marked as changed.
 *
 * CALLED BY:	PrefSetPreference
 *					PrefSetPreferences
 *					PrefSetAppPreferences
 *
 * RETURNED:	none
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			ADH	6/17/99	Initial Revision
 *
 ***********************************************************************/
static void PrvMakePrefDBDirty(DmOpenRef dBP)
{
	UInt32 modNum = 0;
	Err		err;
	LocalID	dbID = 0;
	UInt16	cardNo = 0;
		
	err = DmOpenDatabaseInfo(dBP, &dbID, NULL, NULL, &cardNo, NULL);
	
	if (!err)
	{
		DmDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL, &modNum, 
					NULL, NULL, NULL, NULL);
		modNum++;
			
		DmSetDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL, &modNum, 
					NULL, NULL, NULL, NULL);
	}
}


/***********************************************************************
 *
 * FUNCTION: 	 PrvNewTimeZoneForOld
 *
 * DESCRIPTION: Returns new time zone value corresponding to old.
 *		The new timeZone is a signed 16 bit integer measuring minutes east
 *		of GMT, so positive values represent places east of GMT, and negative
 *		values represent places west of GMT. The legal range of values is -24
 *		hours to +24 hours, although almost all are within the range of -12
 *		to +12 hours. This value replaces the deprecated prefMinutesWestOfGMT,
 *		which was in fact an unsigned value measuring minutes EAST of GMT,
 *		with the same value of 60 for central Europe as this new pref, and
 *		with values greater than 12 hours being used to represent places west
 *		of GMT. This meant that GMT-12 and GMT+12 could not be distinguished,
 *		and that values outside this range could not be represented. These
 *		two preferences are now linked, so that changing one affects the other.
 *
 * PARAMETERS:  old time zone
 *
 * RETURNED:	 new time zone
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	3/21/00	Initial Revision
 *			peter	4/20/00	Map ambiguous value to GMT+12 rather than -12
 *
 ***********************************************************************/
static Int16 PrvNewTimeZoneForOld(UInt32 minutesWestOfGMT)
{
	if (minutesWestOfGMT <= hoursPerDay * hoursInMinutes / 2)
		return minutesWestOfGMT;
	else
		return minutesWestOfGMT - hoursPerDay * hoursInMinutes;
}


/***********************************************************************
 *
 * FUNCTION: 	 PrvOldTimeZoneForNew
 *
 * DESCRIPTION: Returns old time zone value corresponding to new.
 *		See PrvNewTimeZoneForOld for a description of these two values.
 *
 * PARAMETERS:  old time zone
 *
 * RETURNED:	 new time zone
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	3/21/00	Initial Revision
 *
 ***********************************************************************/
static UInt32 PrvOldTimeZoneForNew(Int16 timeZone)
{
	if (timeZone > 0)											// East of GMT
		if (timeZone <= hoursPerDay * hoursInMinutes / 2)
			return timeZone;									// despite the name!
		else
			return hoursPerDay * hoursInMinutes / 2;	// -12 = +12
	else
		if (-timeZone <= hoursPerDay * hoursInMinutes / 2)
			return timeZone + hoursPerDay * hoursInMinutes;
		else
			return hoursPerDay * hoursInMinutes / 2;	// -12 = +12
}


/***********************************************************************
 *
 * FUNCTION: 	 GetPreferenceResource
 *
 * DESCRIPTION: Returns a MemHandle to the preference resource or 0 on error
 *
 * The goal is to quickly return the resource if it is the correct version.
 * If it's an older version, read it in and add data from each newer version
 * to it.  This preserves user's older preferences while updating to the newest
 * settings.  NOTE:  If the pref structure changes, THE VERSION SHOULD TOO!!
 *
 * PARAMETERS:
 *		prefDB		<-		Reference to opened Preference DB.
 *		saved			 ->	Which preferences DB do we want.
 *
 * RETURNED:	MemHandle or NULL on error
 *					prefDB = dbP of open database if success
 *								else 0 (meaning the database has been closed)
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	1/31/95	Initial Revision
 *			scl	3/7/97	Added initialization of sysFtrNumCountry
 *			scl	6/16/97	Added initialization of sysFtrNumLanguage
 *			roger	8/13/97	Rearranged to update older preferences
 *			scl	1/22/98	No longer resize invalid prefs in debug build
 *			scl	1/22/98	Fixed overlocking bug if we found "newer" prefs
 *			scl	1/26/98	Fixed pref resizing code (when version is different)
 *			kwk	08/13/98	Added yet another build case for Japan.
 *			kwk	08/19/98	Removed feature setup for country/language.
 *			scl	08/25/98	Changed DmGetResource to DmGet1Resource.
 *			scl	02/03/99	Moved SystemPreferencesType from stack to dynamic heap
 *			jed	5/11//99	Add default battery for Austin
 *			CS		06/30/99	Add measurement system for Mandalay.
 *			jmp	10/4/99	Add autoOffDurationSecs for Mandalay.
 *			jmp	10/5/99	Bump Mandalay's pref version number to 8 since
 *								a new field was added -- better safe than sorry.
 *			peter 3/21/00	Add new time zone and daylight saving time.
 *			CS		08/01/00	Initialize new language preference from language
 *								feature if preferences are out of date.
 *			kwk	08/01/00	Added timeZoneCountry setup.
 *			WK  	08/07/00	Add Auto lock defaults.
 *			CS		08/08/00	No longer depend on obsolete 'cnty' resource.
 *								Instead, load unknown preferences via new Locale
 *								Manager (after an elaborate dance to grab the
 *								"best" known locale).
 *			peter	08/11/00	Add attention flags default.
 *			CS		08/23/00	Don't wrap ErrNonFatalDisplayIf macros around
 *								calls to LmGetLocaleSetting (how many times am I
 *								going to make this same mistake?)
 *			vivek 08/26/00 		Added default app pref.
 *			grant	11/10/00 Load hard/soft button default apps from resource.
 *			CS		11/17/00	Use lmChoiceLocale insetad of lmChoiceCountry 
 *								and lmChoiceLanguage.
 *
 ***********************************************************************/
 
static MemHandle GetPreferenceResource (DmOpenRef *prefDB, Boolean saved)
	{
	DmOpenRef					dbP;
	MemHandle					preferencesH;
	SystemPreferencesPtr 	preferencesP;
	SystemPreferencesPtr		tempPrefsInDynHeapP;
	UInt32						oldPrefSize, ourPrefSize;
	UInt16						localeIndex;
	Err							result;
	MemHandle					buttonDefaultsResourceH;
	ButtonDefaultListType	*buttonDefaults;
	UInt16						i;
	
// BUMMER - most callers of this routine don't bother checking for zero return values!
//	DOLATER - we should probably enable the following line for failed cases...
//	*prefDB = 0;

	dbP = PrefOpenPreferenceDB (saved);
	if (! dbP) return 0;
	
	preferencesH = (MemHandle) DmGet1Resource(sysResTSysPref, sysResIDSysPrefMain);
	if (preferencesH)
		{
		preferencesP = (SystemPreferencesPtr) MemHandleLock(preferencesH);
		
		// If the preferences are the correct version quickly return them.
		// They BETTER be the right size.  Change prefs?  Bump the version!!
		if (preferencesP->version == preferenceDataVerLatest)
			{
			// During development, let's flag if we find prefs that are the wrong size!
			#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
				if (MemPtrSize(preferencesP) != sizeof(SystemPreferencesType))
					{
					ErrNonFatalDisplay("System Prefs changed size but not version!");
					}
			#endif

			// Saved prefs resource exists, and it's the version we expected, so return it!
			MemHandleUnlock(preferencesH);
			*prefDB = dbP;
			return preferencesH;
			}
		
		// Defend against NEWER or OLDER prefs running on our version of the OS:
		// We assume that prefs will only be added to the end.  It would be
		// bad for future prefs versions to change existing pref definitions.
		oldPrefSize = MemHandleSize(preferencesH);
		ourPrefSize = sizeof(SystemPreferencesType);
		
		// Create a temporary preference structure in the dynamic heap.
		// This used to happen on the stack, but it's getting too big, and
		// cause the kernel stack to overflow in strange, hard to debug ways!
		// To practice what we preach, we should NOT have this be on the stack.
		tempPrefsInDynHeapP = MemPtrNew(ourPrefSize);

		// Return an error if we couldn't allocate the prefs structure.
		if (!tempPrefsInDynHeapP)
			{
			DmCloseDatabase(dbP);
			return 0;
			}

		// If the prefs are the wrong size, unlock, resize, and re-lock them.
		if (oldPrefSize != ourPrefSize)
			{
			MemHandleUnlock(preferencesH);
			if (MemHandleResize(preferencesH, ourPrefSize) != 0)
				{
				DmCloseDatabase(dbP);
				MemPtrFree(tempPrefsInDynHeapP);
				return 0;
				}
			preferencesP = (SystemPreferencesPtr) MemHandleLock(preferencesH);

			// Initialize our local copy of the preference data.
			MemSet(tempPrefsInDynHeapP, ourPrefSize, 0);
			}

		// The (wrong version) prefs are now the correct size.

		// Read in the saved preferences.  We will add values to these.
		// If the old prefs were smaller, we only read in what was there.
		// If the old prefs were bigger, we only read in what we support.
		MemMove(tempPrefsInDynHeapP, preferencesP, 
			(oldPrefSize < ourPrefSize) ? oldPrefSize : ourPrefSize);

		// Fall through to the pref-tweaking code below.

		}
	else	// Saved prefs resource does not exist - create it!
		{
		// Create a temporary preference structure in the dynamic heap.
		// This used to happen on the stack, but it's getting too big, and
		// cause the kernel stack to overflow in strange, hard to debug ways!
		// To practice what we preach, we should NOT have this be on the stack.
		tempPrefsInDynHeapP = MemPtrNew(sizeof(SystemPreferencesType));

		// Return an error if we couldn't allocate the prefs structure.
		if (!tempPrefsInDynHeapP)
			{
			DmCloseDatabase(dbP);
			return 0;
			}

		// Create a [new] preference resource.
		preferencesH = (MemHandle) DmNewResource(dbP, sysResTSysPref, sysResIDSysPrefMain, 
			sizeof (SystemPreferencesType));
		
		// Return an error if we couldn't make the resource.
		if (!preferencesH)
			{
			DmCloseDatabase(dbP);
			MemPtrFree(tempPrefsInDynHeapP);
			return 0;
			}

		preferencesP = (SystemPreferencesPtr) MemHandleLock(preferencesH);
		
		// Initialize our local copy of the preference data.
		// The settings will get filled in below.
		MemSet(tempPrefsInDynHeapP, sizeof(SystemPreferencesType), 0);
		}
		
	// Find the known locale best matching the language & country in the
	// preferences (if present), falling back to features from System.rsrc.
	localeIndex = 0;
		{
		LmLocaleType	prefsLocale;
		UInt32			featureValue;
		
		if (tempPrefsInDynHeapP->version < preferenceDataVer2)
			{
			result = FtrGet(sysFtrCreator, sysFtrNumCountry, &featureValue);
			if (result == errNone)
				{
				tempPrefsInDynHeapP->country = featureValue;
				}
			else
				{
				ErrNonFatalDisplay("Country feature not set");
				tempPrefsInDynHeapP->country = cUnitedStates;
				}
			}
		if (tempPrefsInDynHeapP->version < preferenceDataVer9)
			{
			result = FtrGet(sysFtrCreator, sysFtrNumLanguage, &featureValue);
			if (result == errNone)
				{
				tempPrefsInDynHeapP->language = featureValue;
				}
			else
				{
				ErrNonFatalDisplay("Language feature not set");
				tempPrefsInDynHeapP->language = lEnglish;
				}
			}
		prefsLocale.country = tempPrefsInDynHeapP->country;
		prefsLocale.language = tempPrefsInDynHeapP->language;
		result = LmLocaleToIndex(&prefsLocale, &localeIndex);
		if (result != errNone)
			{
			ErrNonFatalDisplay("Preferences locale not found");
			prefsLocale.language = lmAnyLanguage;
			result = LmLocaleToIndex(&prefsLocale, &localeIndex);
			if (result)
				{
				ErrNonFatalDisplay("Preferences country not found");
				localeIndex = 0;
				}
			}
		}

	// Version 1 and version 2 preferences are written together because they 
	// were intermixed.
	if (tempPrefsInDynHeapP->version < preferenceDataVer2)
		{
		tempPrefsInDynHeapP->version = preferenceDataVer2;
		tempPrefsInDynHeapP->autoOffDuration = defaultAutoOffDuration;
		tempPrefsInDynHeapP->sysSoundLevelV20 = defaultSysSoundLevel;
		tempPrefsInDynHeapP->gameSoundLevelV20 = defaultGameSoundLevel;
		tempPrefsInDynHeapP->alarmSoundLevelV20 = defaultAlarmSoundLevel;
		tempPrefsInDynHeapP->hideSecretRecords = false;
		tempPrefsInDynHeapP->deviceLocked = false;
		tempPrefsInDynHeapP->localSyncRequiresPassword = false;
		tempPrefsInDynHeapP->remoteSyncRequiresPassword = false;
		tempPrefsInDynHeapP->animationLevel = alEventsOnly;
		tempPrefsInDynHeapP->sysPrefFlags = 0;
		
		// Pick the right battery type
		// DOLATER - This is hardcoded because only SystemMgr knows about the battery tables
		// and SystemMgr depends on the preference being set right.
		// DOLATER - In the new hardware-independent world, this should be moved out of here
		// and into the HAL.  Trouble is, there's no equivalent mechanism in the HAL.  More later.
		// Modified to remove the IIIc LiIon battery as it has been deprecated.
		if (GHwrMiscFlagsExt & hwrMiscFlagExtHasLiIon) 
			tempPrefsInDynHeapP->sysBatteryKind = sysBatteryKindLiIon;
		else
			tempPrefsInDynHeapP->sysBatteryKind = sysBatteryKindAlkaline;
		
		// Default action for Ronamatic stroke
		tempPrefsInDynHeapP->ronamaticChar = graffitiReferenceChr;		// popup keyboard in default mode
		
		// Load hard/soft button default app assignments
		buttonDefaultsResourceH = DmGetResource(sysResTButtonDefaults, sysResIDButtonDefaults);
		ErrNonFatalDisplayIf(buttonDefaultsResourceH == NULL, "button defaults resource not found");
		buttonDefaults = MemHandleLock(buttonDefaultsResourceH);
		
		for (i = 0; i < buttonDefaults->numButtons; i++)
			{
			switch (buttonDefaults->button[i].keyCode)
				{
				case vchrHard1:
					tempPrefsInDynHeapP->hard1CharAppCreator = buttonDefaults->button[i].creator;
					break;
				
				case vchrHard2:
					tempPrefsInDynHeapP->hard2CharAppCreator = buttonDefaults->button[i].creator;
					break;
				
				case vchrHard3:
					tempPrefsInDynHeapP->hard3CharAppCreator = buttonDefaults->button[i].creator;
					break;
				
				case vchrHard4:
					tempPrefsInDynHeapP->hard4CharAppCreator = buttonDefaults->button[i].creator;
					break;
				
				case vchrCalc:
					tempPrefsInDynHeapP->calcCharAppCreator = buttonDefaults->button[i].creator;
					break;
				
				case vchrHardCradle:
					tempPrefsInDynHeapP->hardCradleCharAppCreator = buttonDefaults->button[i].creator;
					break;
				
				case vchrHardCradle2:
					tempPrefsInDynHeapP->hardCradle2CharAppCreator = buttonDefaults->button[i].creator;
					break;
				
				case vchrLaunch:
					tempPrefsInDynHeapP->launcherCharAppCreator = buttonDefaults->button[i].creator;
					break;
				}
			}
		MemHandleUnlock(buttonDefaultsResourceH);
		
		// Set the locale-dependent preferences based on the locale found above.
		tempPrefsInDynHeapP->dateFormat = dfMDYWithSlashes;
		result = LmGetLocaleSetting(	localeIndex,
												lmChoiceDateFormat,
												&tempPrefsInDynHeapP->dateFormat,
												sizeof(tempPrefsInDynHeapP->dateFormat));
		ErrNonFatalDisplayIf(result, "Can\'t get locale date format");
		tempPrefsInDynHeapP->longDateFormat = dfMDYLongWithComma;
		result = LmGetLocaleSetting(	localeIndex,
												lmChoiceLongDateFormat,
												&tempPrefsInDynHeapP->longDateFormat,
												sizeof(tempPrefsInDynHeapP->longDateFormat));
		ErrNonFatalDisplayIf(result, "Can\'t get locale long date format");
			{
			UInt16		weekStartDay = sunday;
				
			result = LmGetLocaleSetting(	localeIndex,
													lmChoiceWeekStartDay,
													&weekStartDay,
													sizeof(weekStartDay));
			ErrNonFatalDisplayIf(result, "Can\'t get locale week start day");
			tempPrefsInDynHeapP->weekStartDay = weekStartDay;
			}
		tempPrefsInDynHeapP->timeFormat = tfColonAMPM;
		result = LmGetLocaleSetting(	localeIndex,
												lmChoiceTimeFormat,
												&tempPrefsInDynHeapP->timeFormat,
												sizeof(tempPrefsInDynHeapP->timeFormat));
		ErrNonFatalDisplayIf(result, "Can\'t get locale time format");
		tempPrefsInDynHeapP->numberFormat = nfCommaPeriod;
		result = LmGetLocaleSetting(	localeIndex,
												lmChoiceNumberFormat,
												&tempPrefsInDynHeapP->numberFormat,
												sizeof(tempPrefsInDynHeapP->numberFormat));
		ErrNonFatalDisplayIf(result, "Can\'t get locale number format");
									
		// These two are deprecated, thus aren't known to Locale Manager
		tempPrefsInDynHeapP->daylightSavings = dsNone;
		tempPrefsInDynHeapP->timeZone = 0;
		result = LmGetLocaleSetting(	localeIndex,
												lmChoiceTimeZone,
												&tempPrefsInDynHeapP->timeZone,
												sizeof(tempPrefsInDynHeapP->timeZone));
		ErrNonFatalDisplayIf(result, "Can\'t get locale time zone");
		tempPrefsInDynHeapP->minutesWestOfGMT
			= PrvOldTimeZoneForNew(tempPrefsInDynHeapP->timeZone);
		}
	
	if (tempPrefsInDynHeapP->version < preferenceDataVer3)
		{
		tempPrefsInDynHeapP->version = preferenceDataVer3;
		tempPrefsInDynHeapP->sysSoundVolume = defaultSysSoundVolume;
		tempPrefsInDynHeapP->gameSoundVolume = defaultGameSoundVolume;
		tempPrefsInDynHeapP->alarmSoundVolume = defaultAlarmSoundVolume;
		tempPrefsInDynHeapP->beamReceive = true;
// <SCL 1/26/98> Always force digitizer pref to false.  This disables the digitizer
//						panel on soft reset since nobody seemed to like it...
//						For the record, I STILL think we should force it.  Bummer.
		tempPrefsInDynHeapP->calibrateDigitizerAtReset = false;
		
		// Upgrade some of the older data
		if (tempPrefsInDynHeapP->launcherCharAppCreator == sysFileCNullApp)
			tempPrefsInDynHeapP->launcherCharAppCreator = sysFileCLauncher;
		}
	
	
	if (tempPrefsInDynHeapP->version < preferenceDataVer4)
		{
		tempPrefsInDynHeapP->version = preferenceDataVer4;
		tempPrefsInDynHeapP->systemKeyboardID = kSystemKeyboardResID;
		tempPrefsInDynHeapP->defSerialPlugIn = sysFileCUart328;
		}
	
	//	PalmOS 3.1 was released with prefs version 5
	// (stayOnWhenPluggedIn and stayLitWhenPluggedIn)
	
	// Version 5 and 6 are (for Eleven) handled together here, since previous builds
	// of Eleven were labeled version 5 but included ONLY the antennaCharAppCreator
	// pref in place of the new PalmOS 3.1 (Sumo) prefs.  This should fix that.
	// This is because since they were both [at one time] version 5, the resource will
	// automatically get resized and "upgraded" to version 6 the first time we
	// execute this code in a 3.2 ROM.  It's confusing, but it works.
	if (tempPrefsInDynHeapP->version < preferenceDataVer6)
		{
		tempPrefsInDynHeapP->version = preferenceDataVer6;

		// Additions for PalmOS 3.1:
		tempPrefsInDynHeapP->stayOnWhenPluggedIn = false;
		tempPrefsInDynHeapP->stayLitWhenPluggedIn = false;

		// Additions for PalmOS 3.2:
		// Load antenna button default app assignment
		buttonDefaultsResourceH = DmGetResource(sysResTButtonDefaults, sysResIDButtonDefaults);
		ErrNonFatalDisplayIf(buttonDefaultsResourceH == NULL, "button defaults resource not found");
		buttonDefaults = MemHandleLock(buttonDefaultsResourceH);
		
		for (i = 0; i < buttonDefaults->numButtons; i++)
			{
			if (buttonDefaults->button[i].keyCode == vchrHardAntenna)
				{
				tempPrefsInDynHeapP->antennaCharAppCreator = buttonDefaults->button[i].creator;
				break;
				}
			}
		MemHandleUnlock(buttonDefaultsResourceH);
		}
	
	// Where's version 7?  There isn't such a beast.  It's just a number.  As long
	// the "latest" one is bigger than the last one, who cares?
	
	// PalmOS 3.5 was released with pref version 8 to handle default measurement
	// systems associated with the preferred country.  PalmOS 3.5 also introduces
	// a seconds-based auto-off duration value.
	if (tempPrefsInDynHeapP->version < preferenceDataVer8)
		{
		tempPrefsInDynHeapP->version = preferenceDataVer8;

		// Additions for PalmOS 3.5:
		// Set the locale-dependent preferences based on the locale found above:
		tempPrefsInDynHeapP->measurementSystem = unitsMetric;
		result = LmGetLocaleSetting(	localeIndex,
												lmChoiceMeasurementSystem,
												&tempPrefsInDynHeapP->measurementSystem,
												sizeof(tempPrefsInDynHeapP->measurementSystem));
		ErrNonFatalDisplayIf(result, "Can\'t get locale measurement system");

		// Make sure autoOffDuration & autoOffDurationSecs match!
		tempPrefsInDynHeapP->autoOffDurationSecs = defaultAutoOffDurationSecs;
		tempPrefsInDynHeapP->autoOffDuration = defaultAutoOffDuration;
		}

	// PalmOS 4.0 uses pref version 9 to handle new time zone and daylight saving time.
	if (tempPrefsInDynHeapP->version < preferenceDataVer9)
		{
		tempPrefsInDynHeapP->version = preferenceDataVer9;

		// Additions for PalmOS 4.0:
		tempPrefsInDynHeapP->timeZone = PrvNewTimeZoneForOld(tempPrefsInDynHeapP->minutesWestOfGMT);
		tempPrefsInDynHeapP->daylightSavingAdjustment = 0;
			// Default to standard time. We can't do any better than this until we know the date and time,
			// which are set in the Welcome (Setup) application.

		// Set up time zone country to be the default formats country. We'll guess at this for
		// now, but it gets overridden by the Welcome (Setup) application.
		tempPrefsInDynHeapP->timeZoneCountry = tempPrefsInDynHeapP->country;

		
		//Set up Auto Locking defaults.
		tempPrefsInDynHeapP->autoLockType = defaultAutoLockType;
		tempPrefsInDynHeapP->autoLockTime = defaultAutoLockTime;
		tempPrefsInDynHeapP->autoLockTimeFlag = defaultAutoLockTimeFlag;
		
		// Set up attention flags to be the default, which is:
		//		sound on
		//		LED on
		//		vibrate off
		//		custom effects off

		tempPrefsInDynHeapP->attentionFlags = kAttnFlagsUserWantsSound | kAttnFlagsUserWantsLED;


		// Set the creator for the default app to be launched
		// DOLATER : vivek - Set this to the tint value ?
			tempPrefsInDynHeapP->defaultAppCreator = 0;
		}

	// Add new versions of preferences here
/*	if (tempPrefsInDynHeapP->version < preferenceDataVer10)
		{
		tempPrefsInDynHeapP->version = preferenceDataVer10;
		}
*/	


	// Are the saved preferences a NEWER version than we know about??
	if (tempPrefsInDynHeapP->version > preferenceDataVerLatest)
		{
		// Downgrade the pref version to OUR version.
		tempPrefsInDynHeapP->version = preferenceDataVerLatest;
		}

	// Now that the preferences are set (and locked), update the resource
	DmWrite(preferencesP, 0, tempPrefsInDynHeapP, sizeof(SystemPreferencesType));

	// Dispose of our temporary prefs structure
	MemPtrFree(tempPrefsInDynHeapP);

	// Unlock the prefs resource
	MemHandleUnlock(preferencesH);

	// Return the DmOpenRef and prefs resource handle.
	*prefDB = dbP;
	return preferencesH;
	}


	
/***********************************************************************
 *
 * FUNCTION: 	 PrefGetPreferences
 *
 * DESCRIPTION: Return a copy of the system preferences
 *
 * THIS IS AN OUTDATED FUNCTION CALL.  USE PrefGetPreference INSTEAD.
 *
 * PARAMETERS:  system preferences
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	1/31/95	Initial Revision
 *
 ***********************************************************************/
 
void PrefGetPreferences(SystemPreferencesPtr p)
	{
	MemHandle prefResource;
	DmOpenRef	prefDB;
	
	prefResource = GetPreferenceResource(&prefDB, true);
	MemMove(p, MemHandleLock(prefResource), sizeof (SystemPreferencesTypeV10));
	MemHandleUnlock(prefResource);
	DmReleaseResource(prefResource);
	DmCloseDatabase(prefDB);
	}
	
	
/***********************************************************************
 *
 * FUNCTION: 	 PrefSetPreferences
 *
 * DESCRIPTION: Set the system preferences
 *
 * THIS IS AN OUTDATED FUNCTION CALL.  USE PrefSetPreference INSTEAD.
 *
 * PARAMETERS:  system preferences
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	1/31/95	Initial Revision
 *
 ***********************************************************************/
 
void PrefSetPreferences(SystemPreferencesPtr p)
	{
	MemHandle		 	prefResourceH;
	DmOpenRef		prefDB;
	void *			prefResourceP;
	
	prefResourceH = GetPreferenceResource(&prefDB, true);
	
	prefResourceP = MemHandleLock(prefResourceH);
	DmWrite(prefResourceP, 0, p, sizeof(SystemPreferencesTypeV10));
	MemPtrUnlock(prefResourceP);

	DmReleaseResource(prefResourceH);
	
	//None of the database operations involved in PrefSetPreference change
	//the modification number of the database.  PrvMakePrefDBDirty will increment
	//the mod number so a new timestamp is set for the modification number.
	PrvMakePrefDBDirty(prefDB);
	
	DmCloseDatabase(prefDB);
	}



/***********************************************************************
 *
 * FUNCTION: 	 PrefGetPreference
 *
 * DESCRIPTION: Return a system preference.
 *
 * PARAMETERS:  system preference choice
 *
 * RETURNED:	 the system preference
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	1/31/95	Created by Roger Flores.
 *			roger	5/7/95	Rewrote to return a single preference
 *			CS		06/30/99	Added prefMeasurementSystem
 *			jmp	10/4/99	Added prefAutoOffDurationSecs.
 *			CS		08/01/00	Added prefLanguage & prefLocale.
 *			kwk	08/01/00	Added prefTimeZoneCountry.
 *			peter	08/08/00	Added prefAttentionFlags.
 *
 ***********************************************************************/
 
UInt32 PrefGetPreference(SystemPreferencesChoice choice)
	{
	MemHandle prefResource, constantHdl;
	DmOpenRef prefDB;
	SystemPreferencesPtr prefP;
	UInt32 result;
	
	
	prefResource = GetPreferenceResource(&prefDB, true);
	prefP = MemHandleLock(prefResource);
	
	// Now get the choice
	switch (choice)
		{
		case prefVersion:
			result = prefP->version;
			break;
			
		case prefCountry:
			result = prefP->country;
			break;
			
		case prefDateFormat:
			result = prefP->dateFormat;
			break;
			
		case prefLongDateFormat:
			result = prefP->longDateFormat;
			break;
			
		case prefWeekStartDay:
			result = prefP->weekStartDay;
			break;
			
		case prefTimeFormat:
			result = prefP->timeFormat;
			break;
			
		case prefNumberFormat:
			result = prefP->numberFormat;
			break;
			
		// For compatibility and to support seconds-based auto-off prefs,
		// we now have two auto-off prefs; prefAutoOffDuration is for
		// minutes, and prefAutoOffDurationSecs is for seconds.
		//
		// Note:  If we've set autoOffDuration to zero because we're using
		// autoOffDurationSecs, return 1 (60 seconds) as a compatibility
		// measure -- i.e., before we implemented seconds as a auto-off pref,
		// 1 minute was the least amount of auto-off time one could officially
		// set.
		//
		case prefAutoOffDuration:
			if ( (0 == prefP->autoOffDuration) && (minutesInSeconds > prefP->autoOffDurationSecs) )
				result = 1;
			else
				result = prefP->autoOffDuration;
			break;
		case prefAutoOffDurationSecs:
			result = prefP->autoOffDurationSecs;
			break;
			
		case prefSysSoundLevelV20:
			if (prefP->sysSoundVolume > 0)
				result = slOn;
			else
				result = slOff;
			break;
			
		case prefGameSoundLevelV20:
			if (prefP->gameSoundVolume > 0)
				result = slOn;
			else
				result = slOff;
			break;
			
		case prefAlarmSoundLevelV20:
			if (prefP->alarmSoundVolume > 0)
				result = slOn;
			else
				result = slOff;
			break;
			
		case prefHidePrivateRecordsV33:
			result = prefP->hideSecretRecords;
			break;
			
		case prefDeviceLocked:
			result = prefP->deviceLocked;
			break;
			
		case prefLocalSyncRequiresPassword:
			result = prefP->localSyncRequiresPassword;
			break;
			
		case prefRemoteSyncRequiresPassword:
			result = prefP->remoteSyncRequiresPassword;
			break;
			
		case prefSysBatteryKind:
			result = prefP->sysBatteryKind;
			break;
			
		case prefAllowEasterEggs:
			result = prefP->sysPrefFlags & sysPrefFlagEnableEasterEggs;
			break;
			
		case prefMinutesWestOfGMT:
			// This value is deprecated (replaced by prefTimeZone).
			// This value is an unsigned integer measuring minutes EAST of GMT,
			// with a value of 60 for central Europe, and with values greater
			// than 12 hours being used to represent places west of GMT. This
			// meant that GMT-12 and GMT+12 could not be distinguished, and
			// that values outside this range could not be represented.
			result = prefP->minutesWestOfGMT;
			break;
			
		case prefDaylightSavings:
			// This value is deprecated (replated by prefDaylightSavingAdjustment).
			// It represents the daylight saving rule in effect, not the current
			// effect of that rule.
			result = prefP->daylightSavings;
			break;
			
		case prefAnimationLevel:
			result = prefP->animationLevel;
			break;
			
		case prefRonamaticChar:
			result = prefP->ronamaticChar;
			break;
			
		case prefHard1CharAppCreator:
			result = prefP->hard1CharAppCreator;
			break;
			
		case prefHard2CharAppCreator:
			result = prefP->hard2CharAppCreator;
			break;
			
		case prefHard3CharAppCreator:
			result = prefP->hard3CharAppCreator;
			break;
			
		case prefHard4CharAppCreator:
			result = prefP->hard4CharAppCreator;
			break;
			
		case prefCalcCharAppCreator:
			result = prefP->calcCharAppCreator;
			break;
			
		case prefHardCradleCharAppCreator:
			result = prefP->hardCradleCharAppCreator;
			break;
			
		case prefHardCradle2CharAppCreator:
			result = prefP->hardCradle2CharAppCreator;
			break;
			
		case prefLauncherAppCreator:
			result = prefP->launcherCharAppCreator;
			break;
			
		case prefSysPrefFlags:
			result = prefP->sysPrefFlags;
			break;
			
		case prefSysSoundVolume:
			result = prefP->sysSoundVolume;
			break;
			
		case prefGameSoundVolume:
			result = prefP->gameSoundVolume;
			break;
			
		case prefAlarmSoundVolume:
			result = prefP->alarmSoundVolume;
			break;
			
		case prefBeamReceive:
			result = prefP->beamReceive;
			break;
			
		case prefCalibrateDigitizerAtReset:
			result = prefP->calibrateDigitizerAtReset;
			break;
			
		case prefSystemKeyboardID:
			result = prefP->systemKeyboardID;
			break;
			
		case prefDefSerialPlugIn:
			result = prefP->defSerialPlugIn;
			break;
			
		case prefStayOnWhenPluggedIn:
			result = prefP->stayOnWhenPluggedIn;
			break;
			
		case prefStayLitWhenPluggedIn:
			result = prefP->stayLitWhenPluggedIn;
			break;
			
		case prefAntennaCharAppCreator:
			result = prefP->antennaCharAppCreator;
			break;		
			
		case prefMeasurementSystem:
			result = prefP->measurementSystem;
			break;		
			
		case prefShowPrivateRecords:
			//From the API point of view, this is a single 3-way enum preference
			//For backward compatibility it is stored in 2 booleans.
			if (prefP->hideSecretRecords)
				{
				if (prefP->maskPrivateRecords)
					{
					result = (maskPrivateRecords);
					}
				else
					result = (hidePrivateRecords);
				}
			else
				result = (showPrivateRecords);
			break;
		
		case prefTimeZone:
			// From the API point of view, this is a signed 16 bit integer
			// measuring minutes east of GMT, so positive values represent
			// places east of GMT, and negative values represent places west
			// of GMT. The legal range of values is -24 hours to +24 hours,
			// although almost all are within the range of -12 to +12 hours.
			// This value replaces the deprecated prefMinutesWestOfGMT.
			result = prefP->timeZone;
			break;
		
		case prefTimeZoneCountry:
			// The time zone picker supports multiple entries with the same
			// time zone offset, so the only way to distinguish between two
			// similar entries is with a country code.
			result = prefP->timeZoneCountry;
			break;
			
		case prefDaylightSavingAdjustment:
			// From the API point of view, this is a signed 16 bit integer
			// measuring the number of minutes that are currently being added
			// to the time for the daylight saving time correction. This will
			// be 0 when on standard time, and 60 when daylight saving time
			// is being observed. This value replaces the deprecated
			// prefDaylightSavings.
			result = prefP->daylightSavingAdjustment;
			break;

      case prefAutoLockType:
			result = prefP->autoLockType;
			break;		
      			
      case prefAutoLockTime:
			result = prefP->autoLockTime;
			break;		
			
      case prefAutoLockTimeFlag:
			result = prefP->autoLockTimeFlag;
			break;		

      case prefLanguage:
			result = prefP->language;
			break;		
			
      case prefLocale:
      	{
	      	LmLocaleType* localeP = (LmLocaleType*)&result;
	      	
	      	localeP->language = prefP->language;
	      	localeP->country = prefP->country;
			}
			break;		
			
		case prefAttentionFlags:
			result = prefP->attentionFlags;
			break;
		
		case prefDefaultAppCreator:
			if (prefP->defaultAppCreator == 0)
			{
				// Always return the default (safe) app creator if this is not set
				constantHdl = DmGetResource(constantRscType, sysSoftResetAppCreatorConstId);
				ErrFatalDisplayIf(constantHdl == NULL, "Constant rsrc for hard reset app not found");
				
				result = *(UInt32 *)MemHandleLock(constantHdl);
				MemHandleUnlock(constantHdl);
			}
			else
				result = prefP->defaultAppCreator;
			break;
		
		
		default:
  			ErrNonFatalDisplay("Getting unknown system preference");
			result = 0;
			break;
		}
			
	MemHandleUnlock(prefResource);
	DmReleaseResource(prefResource);
	DmCloseDatabase(prefDB);
	
	return result;
}
	
	
/***********************************************************************
 *
 * FUNCTION: 	 PrefSetPreference
 *
 * DESCRIPTION: Set a system preference
 *
 * PARAMETERS:  system preferences
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	1/31/95	Initial Revision
 *			roger	5/7/95	Rewrote to set a single preference
 *			tlw	2/11/98	Save sound setting(s) into V20 equivalents, too.
 *			CS		06/30/99	Added support for prefMeasurementSystem
 *			jmp	10/4/99	Added support for second-based auto-off durations.
 *			jmp	10/5/99	Added support for the minutes- and seconds-based
 *								"pegged" values.
 *			peter	4/11/00	Added new time zone & daylight saving time prefs.
 *			CS		08/01/00	Support new prefLanguage & prefLocale prefs.
 *			kwk	08/01/00	Added new prefTimeZoneCountry selector.
 *			peter	08/08/00	Added prefAttentionFlags.
 *
 *
 ***********************************************************************/
 
void PrefSetPreference(SystemPreferencesChoice choice, UInt32 value)
	{
	MemHandle		prefResourceH;
	DmOpenRef		prefDB;
	void *			prefResourceP;
	SystemPreferencesType pref;
	UInt32			attnMgrFeature;
	
	prefResourceH = GetPreferenceResource(&prefDB, true);
	
	prefResourceP = MemHandleLock(prefResourceH);
	MemMove(&pref, prefResourceP, sizeof (SystemPreferencesType));
	
	// Now get the choice
	switch (choice)
		{
		case prefVersion:
			pref.version = (UInt16) value;
			break;
			
		case prefCountry:
			pref.country = (CountryType) value;
			break;
			
		case prefDateFormat:
			pref.dateFormat = (DateFormatType) value;
			break;
			
		case prefLongDateFormat:
			pref.longDateFormat = (DateFormatType) value;
			break;
			
		case prefWeekStartDay:
			pref.weekStartDay = (Int8) value;
			break;
			
		case prefTimeFormat:
			pref.timeFormat = (TimeFormatType) value;
			break;
			
		case prefNumberFormat:
			pref.numberFormat = (NumberFormatType) value;
			break;
			
		// For compatibility and to support seconds-based auto-off prefs,
		// we now have two auto-off prefs; prefAutoOffDuration is for
		// minutes, and prefAutoOffDurationSecs is for seconds.
		//
		case prefAutoOffDuration:
			pref.autoOffDuration = (UInt8) value;
			
			// If we're passed the minutes-based pegged value, set
			// the seconds value to the seconds-based pegged value.
			// Otherwise, just perform the conversion.
			//
			if ( peggedAutoOffDuration == value )
				pref.autoOffDurationSecs = peggedAutoOffDurationSecs;
			else
				pref.autoOffDurationSecs = (UInt16) (value * minutesInSeconds);
			break;
		
		case prefAutoOffDurationSecs:
			pref.autoOffDurationSecs = (UInt16) value;
			
			// If we're passed the seconds-based pegged value,
			// set the minutes value to the minutes-based pegged
			// value.  If the number of seconds we're passed
			// exceeds the minutes-based pegged value, set the
			// minutes-based value to one less than the minutes-
			// based pegged value.  Otherwise, just perform
			// the conversion.
			//
			if ( peggedAutoOffDurationSecs == value )
				pref.autoOffDuration = peggedAutoOffDuration;
			else
			{
				UInt16 tempValue = (UInt16) (value / minutesInSeconds);
				
				if ( peggedAutoOffDuration > tempValue )
					pref.autoOffDuration = (UInt8) tempValue;
				else
					pref.autoOffDuration = peggedAutoOffDuration-1;
			}
			break;
			
		case prefSysSoundLevelV20:
			if ((SoundLevelTypeV20) value == slOn)
				pref.sysSoundVolume = sndMaxAmp;
			else
				pref.sysSoundVolume = 0;
			break;
			
		case prefGameSoundLevelV20:
			if ((SoundLevelTypeV20) value == slOn)
				pref.gameSoundVolume = sndMaxAmp;
			else
				pref.gameSoundVolume = 0;
			break;
			
		case prefAlarmSoundLevelV20:
			if ((SoundLevelTypeV20) value == slOn)
				pref.alarmSoundVolume = sndMaxAmp;
			else
				pref.alarmSoundVolume = 0;
			break;
			
		case prefHidePrivateRecordsV33:
			pref.hideSecretRecords = (Boolean) value;
			break;
			
		case prefDeviceLocked:
			pref.deviceLocked = (Boolean) value;
			break;
			
		case prefSysBatteryKind:
			pref.sysBatteryKind = (SysBatteryKind) value;
			break;
			
		case prefAllowEasterEggs:
			pref.sysPrefFlags &= ~sysPrefFlagEnableEasterEggs;
			if (value)
				pref.sysPrefFlags |= sysPrefFlagEnableEasterEggs;
			GSysPrefFlags = pref.sysPrefFlags;
			break;
			
		case prefMinutesWestOfGMT:
			// This deprecated preference is linked to the new prefTimeZone, so
			// that changing one affects the other.
			pref.minutesWestOfGMT = value;
			pref.timeZone = PrvNewTimeZoneForOld(pref.minutesWestOfGMT);
			break;
			
		case prefDaylightSavings:
			pref.daylightSavings = (DaylightSavingsTypes) value;
			break;
			
		case prefAnimationLevel:
			pref.animationLevel = (AnimationLevelType) value;
			break;
			
		case prefRonamaticChar:
			pref.ronamaticChar = (UInt16) value;
			break;
			
		case prefHard1CharAppCreator:
			pref.hard1CharAppCreator = value;
			break;
			
		case prefHard2CharAppCreator:
			pref.hard2CharAppCreator = value;
			break;
			
		case prefHard3CharAppCreator:
			pref.hard3CharAppCreator = value;
			break;
			
		case prefHard4CharAppCreator:
			pref.hard4CharAppCreator = value;
			break;
			
		case prefCalcCharAppCreator:
			pref.calcCharAppCreator = value;
			break;
			
		case prefHardCradleCharAppCreator:
			pref.hardCradleCharAppCreator = value;
			break;
			
		case prefHardCradle2CharAppCreator:
			pref.hardCradle2CharAppCreator = value;
			break;
			
		case prefLauncherAppCreator:
			pref.launcherCharAppCreator = value;
			break;
			
		case prefSysSoundVolume:
			pref.sysSoundVolume = (UInt16) value;
			pref.sysSoundLevelV20 = value ? slOn : slOff;
			break;
			
		case prefGameSoundVolume:
			pref.gameSoundVolume = (UInt16) value;
			pref.gameSoundLevelV20 = value ? slOn : slOff;
			break;
			
		case prefAlarmSoundVolume:
			pref.alarmSoundVolume = (UInt16) value;
			pref.alarmSoundLevelV20 = value ? slOn : slOff;
			break;
			
		case prefBeamReceive:
			pref.beamReceive = (Boolean) value;
			break;
			
		case prefCalibrateDigitizerAtReset:
			pref.calibrateDigitizerAtReset = (Boolean) value;
			break;
			
		case prefSystemKeyboardID:
			pref.systemKeyboardID = (UInt16) value;
			break;
			
		case prefDefSerialPlugIn:
			pref.defSerialPlugIn = value;
			break;
			
		case prefStayOnWhenPluggedIn:
			pref.stayOnWhenPluggedIn = (Boolean) value;
			break;
			
		case prefStayLitWhenPluggedIn:
			pref.stayLitWhenPluggedIn = (Boolean) value;
			break;
			
		case prefAntennaCharAppCreator:
			pref.antennaCharAppCreator = value;
			break;
			
		case prefMeasurementSystem:
			pref.measurementSystem = (MeasurementSystemType) value;
			break;		
			
		case prefShowPrivateRecords:
			//From the API point of view, this is a single 3-way enum preference
			//For backward compatibility it is stored in 2 booleans.
			switch (value)
				{
				case showPrivateRecords:
					pref.hideSecretRecords = false;
					pref.maskPrivateRecords = false;
					break;
					
				case maskPrivateRecords:
					pref.hideSecretRecords = true;
					pref.maskPrivateRecords = true;
					break;
					
				case hidePrivateRecords:
					pref.hideSecretRecords = true;
					pref.maskPrivateRecords = false;
					break;
				}
			break;
			
		case prefTimeZone:
			// This preference is linked to the deprecated prefMinutesWestOfGMT, so
			// that changing one affects the other.
			pref.timeZone = (Int16) value;
			pref.minutesWestOfGMT = PrvOldTimeZoneForNew(pref.timeZone);
			break;
		
		case prefTimeZoneCountry:
			pref.timeZoneCountry = (CountryType) value;
			break;
		
		case prefDaylightSavingAdjustment:
			// This preference replaces the deprecated prefDaylightSavings, but the
			// two are not linked, since the old one is a rule while the new one is
			// a current state.
			pref.daylightSavingAdjustment = (Int16) value;
			ErrNonFatalDisplayIf(pref.daylightSavingAdjustment != 0 && pref.daylightSavingAdjustment != hoursInMinutes,
				"Invalid daylight saving time adjustment");
			break;

		case prefAutoLockType:
			pref.autoLockType = (SecurityAutoLockType) value;
			break;		
      			
      case prefAutoLockTime:
			pref.autoLockTime = value;
			break;		

      case prefAutoLockTimeFlag:
			pref.autoLockTimeFlag = value;
			break;		
			
      case prefLanguage:
			pref.language = value;
			break;		
			
      case prefLocale:
      	{
	      	LmLocaleType* localeP = (LmLocaleType*)&value;
	      	
				pref.language = localeP->language;
				pref.country = localeP->country;
			}
			break;		
			
		case prefAttentionFlags:
			pref.attentionFlags = value;
			
			// These user preferences are also stored in a feature with the
			// device capabilities, so update them there.
			FtrGet(kAttnFtrCreator, kAttnFtrCapabilities, &attnMgrFeature);
			attnMgrFeature = (attnMgrFeature & kAttnFlagsCapabilitiesMask) |
								(value & kAttnFlagsUserSettingsMask);
			FtrSet(kAttnFtrCreator, kAttnFtrCapabilities, attnMgrFeature);
			break;
		
		case prefDefaultAppCreator:
			pref.defaultAppCreator = value;
			break;
		
		
		default:
  			ErrNonFatalDisplay("Setting unknown system preference");
			break;
		}
			

	DmWrite(prefResourceP, 0, &pref, sizeof(SystemPreferencesType));
	MemPtrUnlock(prefResourceP);

	DmReleaseResource(prefResourceH);
	
	//None of the database operations involved in PrefSetPreference change
	//the modification number of the database.  PrvMakePrefDBDirty will increment
	//the mod number so a new timestamp is set for the modification number.
	PrvMakePrefDBDirty(prefDB);
	
	DmCloseDatabase(prefDB);
	}



/***********************************************************************
 *
 * FUNCTION: 	 PrefGetAppPreferencesV10
 *
 * DESCRIPTION: Return a copy of an application's preferences
 *
 * PARAMETERS:  type        - application creator type
 *              version     - version number of the application
 *              prefs       - pointer to a buffer to hold preferences
 *              prefsSize   - size the the buffer passed
 *
 * RETURNED:	false if the preference resource was not found or the 
 *             preference resource contains the wrong version number.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/21/95	Initial Revision
 *			scl	8/25/98	Changed DmGetResource to DmGet1Resource
 *
 ***********************************************************************/
 
Boolean PrefGetAppPreferencesV10 (UInt32 type, Int16 version, void * prefs,
	UInt16 prefsSize)
{
	Boolean found = false;
	DmOpenRef	dbP;
	MemHandle	resourceH;
	UInt8 	*resourceP;	

	dbP = PrefOpenPreferenceDB (true);
	if (! dbP) return false;
	
	resourceH = (MemHandle) DmGet1Resource (type, 0);
	if (resourceH)
		{
		// Check that the version numbers match.
		resourceP = MemHandleLock(resourceH);
		if (*((Int16 *)resourceP) == version)
			{
			if (MemPtrSize (resourceP) == prefsSize + sizeof (Int16))
				{
				resourceP += sizeof (Int16);			// skip the version number
				MemMove (prefs, resourceP, prefsSize);
				found = true;
				}
			}
		MemHandleUnlock(resourceH);
		DmReleaseResource (resourceH);
		}

	DmCloseDatabase (dbP);
	return found;
}


/***********************************************************************
 *
 * FUNCTION: 	 PrefSetAppPreferencesV10
 *
 * DESCRIPTION: Save an application's preferences in the preferences 
 *              database.
 *
 * PARAMETERS:  creator     - application creator type
 *              version     - version number of the application
 *              prefs       - pointer to a buffer that holds preferences
 *              prefsSize   - size the the buffer passed
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/21/95	Initial Revision
 *
 ***********************************************************************/
void PrefSetAppPreferencesV10 (UInt32 creator, Int16 version, void * prefs,
	UInt16 prefsSize)
{
	PrefSetAppPreferences (creator, 0, version, prefs, prefsSize, true);
}


/***********************************************************************
 *
 * FUNCTION: 	 PrefGetAppPreferences
 *
 * DESCRIPTION: Return a copy of an application's preferences.
 *
 * Sometimes, for variable length resources, this routine is called twice.
 * Once with a NULL pointer and size of zero to find out how many bytes 
 * need to be read, and then a second time with a buffer allocated with
 * the correct size.
 *
 * PARAMETERS:  creator     - application creator type
 *              prefs       - pointer to a buffer to hold preferences
 *              prefsSize   - pointer to size the the buffer passed
 *
 * RETURNED:	noPreferenceFound if the preference resource was not found or  
 *             the version number if found
 *
 *					prefsSize is set to the size of the preference found.  If it 
 *					greater than the size passed then some bytes were not retrieved. 
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/21/95	Initial Revision
 *			roger	5/17/96	Rewrote to return any version of app preferences saved.
 *			scl	8/25/98	Changed DmGetResource to DmGet1Resource
 *
 ***********************************************************************/
 
Int16 PrefGetAppPreferences (UInt32 creator, UInt16 id, void * prefs, 
	UInt16 *prefsSize, Boolean saved)
{
	DmOpenRef	dbP;
	MemHandle	resourceH;
	UInt8		*resourceP;
	Int16		version = noPreferenceFound;
	UInt16	prefsSizeToCopy;
	UInt16	resourceSize;
	

	dbP = PrefOpenPreferenceDB (saved);
	if (! dbP) return version;
	
	resourceH = (MemHandle) DmGet1Resource (creator, id);
	if (resourceH)
		{
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
		// Fill up the buffer as much as allowed with a value to reveal errors.
		// Only do this if there are bytes to set.  Otherwise this routine fails
		// when called to find the resource's size.
		if (*prefsSize > 0)
			{
			MemSet(prefs, *prefsSize, 0xfe);	
			}
#endif

		// Check that the version numbers match.
		resourceP = MemHandleLock(resourceH);
		
		// Get the version number.
		version = *((Int16 *)resourceP);
		
		
		// Copy the preference resource into the buffer provided.  Do not copy
		// more bytes than the size of the resource or than will fit in the buffer.
		resourceSize = MemPtrSize (resourceP) - sizeof (Int16);
		prefsSizeToCopy = min(*prefsSize, resourceSize);
		resourceP += sizeof (Int16);			// skip the version number
		if (prefsSizeToCopy > 0)
			{
			MemMove (prefs, resourceP, prefsSizeToCopy);
			}
		
		*prefsSize = resourceSize;

		MemHandleUnlock(resourceH);
		DmReleaseResource (resourceH);
		}

	DmCloseDatabase (dbP);
	return version;
}


/***********************************************************************
 *
 * FUNCTION: 	 PrefSetAppPreferences
 *
 * DESCRIPTION: Save an application's preferences in the preferences 
 *              database.
 *
 * PARAMETERS:  creator     - application creator type
 *              id			 - resource id (usually 0)
 *              version     - version number of the application
 *              prefs       - pointer to a buffer that holds preferences
 *              prefsSize   - size the the buffer passed
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/21/95	Initial Revision
 *			roger	5/17/96	Added a resource id so apps may store multiple resources.
 *			scl	8/25/98	Changed DmGetResource to DmGet1Resource
 *
 ***********************************************************************/
void PrefSetAppPreferences (UInt32 creator, UInt16 id, Int16 version, 
	const void * prefs, UInt16 prefsSize, Boolean saved)
{
	DmOpenRef	dbP;
	MemHandle 	resourceH;
	UInt8			*resourceP;
	UInt32 		resourceSize;
	Int16			resourceIndex;
	Int16			prefVersion;
	Boolean		forceUpdate = false;
	

	dbP = PrefOpenPreferenceDB (saved);
	if (! dbP) return;
	
	// The user might want to delete the resource.  Do this if they pass
	// no data as indicated by zero bytes or a NULL pointer.
	if (prefsSize == 0 ||
		prefs == NULL)
		{
		resourceIndex = DmFindResource(dbP, creator, id, 0);
		// Remove the resource if it exists.
		if (resourceIndex != -1)
			DmRemoveResource(dbP, resourceIndex);
		
		DmCloseDatabase(dbP);
		return;
		}

	resourceSize = prefsSize + sizeof (Int16);	// Make room for version number

	resourceH = (MemHandle) DmGet1Resource (creator, id);
	if (resourceH)
		{
		// Resize the resource if it's not the right size.
		if (resourceSize != MemHandleSize(resourceH) )
			{
			resourceH = DmResizeResource (resourceH, resourceSize);
			forceUpdate = true;
			}
		}
	else
		{
		// Create a new resouce.
		resourceH = (MemHandle) DmNewResource (dbP, creator, id, resourceSize);
		forceUpdate = true;
		}

	if (! resourceH)
		{
		DmCloseDatabase(dbP);
		return;
		}

	// Lock down existing Pref resource
	resourceP = MemHandleLock(resourceH);

	// If the resource existed and the size matched, check the contents
	if (!forceUpdate)
		{
		// If the version doesn't match, write the prefs
		prefVersion = *((Int16 *)resourceP);
		if (prefVersion != version)
			{
			forceUpdate = true;
			//None of the database operations involved in PrefSetPreference change
			//the modification number of the database.  PrvMakePrefDBDirty will increment
			//the mod number so a new timestamp is set for the modification number.
			PrvMakePrefDBDirty(dbP);
			}
		else
			// If the data doesn't match, write the prefs
			if (MemCmp(resourceP+sizeof(version), prefs, prefsSize) != 0)
			{
			forceUpdate = true;
			//None of the database operations involved in PrefSetPreference change
			//the modification number of the database.  PrvMakePrefDBDirty will increment
			//the mod number so a new timestamp is set for the modification number.
			PrvMakePrefDBDirty(dbP);
			}
		}

	// Only write the resource if we NEED to, to avoid touching the database
	// and causing (in particular the Saved) Prefs to be backed up unnecessarily.
	if (forceUpdate)
		{
		// Store the application's version at the beginning the the 
		// preference resource.
		DmWrite(resourceP, 0, &version, sizeof(version));
	
		// Copy the preference to the resource.
		DmWrite(resourceP, sizeof(Int16), prefs, prefsSize);
		}
	
	// Clean up
	MemPtrUnlock(resourceP);
	DmReleaseResource (resourceH);
	DmCloseDatabase (dbP);
}
