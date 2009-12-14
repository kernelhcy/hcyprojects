/******************************************************************************
 *
 * Copyright (c) 1999-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: LocaleMgr.c
 *
 * Release: 
 *
 * Description:
 *		Routines that support locales (information specific to locales and regions).
 *
 * History:
 *	04/29/00	CS		Created by Chris Schneider.
 *	05/16/00	CS		LmCountryType/LmLanguageType are now back to
 *						CountryType/LanguageType.
 *
 *****************************************************************************/

/* Generate jumps to the Locale Manager routines from the dispatch table.

DOLATER kwk -	Figure out if this causes problems w/routines inside of here calling
					other Locale Manager routines directly, versus using the trap.
					To fix this we'd need to have a LocaleDispatch.c file which just
					contained the dispatcher code
*/
#define DIRECT_LOCALE_CALLS 1

#include <PalmTypes.h>

#include <DataMgr.h>
#include <MemoryMgr.h>
#include <StringMgr.h>
#include <ErrorMgr.h>
#include <DebugMgr.h>
#include	<UIResources.h>
#include	<DateTime.h>		// DateFormatType, TimeFormatType, etc.
#include	<Localize.h>		// NumberFormatType, etc.
#include	<Preferences.h>	// MeasurementSystemType, etc.

#include "LocalePrv.h"		// All of my private junk (e.g., LmLocaleSettingsType)

#include <LocaleMgr.h>		// All of my public APIs


/***********************************************************************
 * Private constants
 **********************************************************************/

/***********************************************************************
 * Private forward declarations
 ***********************************************************************/

static LmSettingsType*
PrvGetSettings(void);

#if EMULATION_LEVEL == EMULATION_NONE
static void
PrvDispatchTable(void);
#endif

static void
PrvSelectorError(		LmRoutineSelector	iRoutineSelector);

/***********************************************************************
 * Public routines
 ***********************************************************************/

/***********************************************************************/
UInt16
LmGetNumLocales(void)
/*
Return the number of known locales (maximum locale index + 1).
*/
{
	UInt16			numLocales = 0;
	LmSettingsType*	settingsP = PrvGetSettings();
	
	numLocales = settingsP->numLocales;
	
	if (settingsP)
		MemPtrUnlock(settingsP);
	settingsP = NULL;
	
	return(numLocales);
	
} // LmGetNumLocales


/***********************************************************************/
Err
LmLocaleToIndex(		const
							LmLocaleType*	iLocale,
							UInt16*			oLocaleIndex)
/*
Convert <iLocale> to <oLocaleIndex> by locating it within the set of known
locales.

History:
09/29/00	CS		Made iLocale parm of LmLocaleToIndex const.
11/17/00	CS		Now accept lmAnyCountry and lmAnyLanguage as wildcards.
*/
{
	LmSettingsType*	settingsP = NULL;
	Err					result = errNone;
	
	ErrNonFatalDisplayIf(iLocale == NULL, "NULL iLocale");
	ErrNonFatalDisplayIf(oLocaleIndex == NULL, "NULL oLocaleIndex");

	settingsP = PrvGetSettings();
	for (	*oLocaleIndex = 0;
			(*oLocaleIndex) < settingsP->numLocales;
			(*oLocaleIndex)++) {
		if	(	(	(	settingsP->locales[*oLocaleIndex].language
					== (UInt8)iLocale->language)
				||	(	iLocale->language
					== lmAnyLanguage))
			&&	(	(	settingsP->locales[*oLocaleIndex].country
					== (UInt8)iLocale->country)
				||	(	iLocale->country
					== lmAnyCountry))) {
			break;
		}
	}
	
	if ((*oLocaleIndex) >= settingsP->numLocales) {
		*oLocaleIndex = 0;
		result = lmErrUnknownLocale;
	}
	
	if (settingsP)
		MemPtrUnlock(settingsP);
	settingsP = NULL;
	
	return(result);
	
} // LmLocaleToIndex


/***********************************************************************/
Err
LmGetLocaleSetting(	UInt16			iLocaleIndex,
							LmLocaleSettingChoice iChoice,
							void*				oValue,
							UInt16			iValueSize)
/*
Return in <oValue> the setting identified by <iChoice> which is appropriate for
the locale identified by <iLocaleIndex>.  Return lmErrSettingDataOverflow if the
data for <iChoice> occupies more than <iValueSize> bytes.  Display a non-fatal
error if <iValueSize> is larger than the data for a fixed-size setting.

History:
07/28/00	CS		Replaced lmChoiceMinutesWestOfGMT & lmChoiceDaylightSavings
					selectors with lmChoiceTimeZone.
08/08/00	CS		Renamed <iMaxSize> parameter <iValueSize> and check to make sure
					that <oValue> is the correct size for all fixed-size settings.
11/17/00	CS		Removed support for lmChoiceLanguage & lmChoiceCountry,
					since these guys were returning UInt8's, which probably
					won't cut it at some point in the future.  Callers can use
					lmChoiceLocale, which returns an LmLocaleType struct that
					places the country and language into UInt16 fields.
*/
{
	LmSettingsType*	settingsP = NULL;
	Err					result = errNone;
	
	ErrNonFatalDisplayIf(oValue == NULL, "NULL oValue");

	settingsP = PrvGetSettings();
	if (iLocaleIndex >= settingsP->numLocales) {
		result = lmErrBadLocaleIndex;
	
	} else {
		LmLocaleSettingsType*	localeSettingsP = &settingsP->locales[iLocaleIndex];
		
		switch (iChoice) {
			case lmChoiceLocale:
				ErrNonFatalDisplayIf(sizeof(LmLocaleType) != iValueSize,
											"Incorrect size for fixed-size locale setting");
				if (sizeof(LmLocaleType) > iValueSize) {
					result = lmErrSettingDataOverflow;
				} else {
					((LmLocaleType*)oValue)->language
						= (LanguageType)localeSettingsP->language;
					((LmLocaleType*)oValue)->country
						= (CountryType)localeSettingsP->country;
				}
			break;
			
#if SUPPORT_LANGUAGE_NAME
			case lmChoiceLanguageName:
				if (kMaxLanguageNameLen >= iValueSize) {
					result = lmErrSettingDataOverflow;
				} else {
					MemMove(oValue, localeSettingsP->LanguageName, kMaxLanguageNameLen+1);
				}
			break;
#endif

			case lmChoiceCountryName:
				if (kMaxCountryNameLen >= iValueSize) {
					result = lmErrSettingDataOverflow;
				} else {
					MemMove(oValue, localeSettingsP->countryName, kMaxCountryNameLen+1);
				}
			break;
			
			case lmChoiceDateFormat:
				ErrNonFatalDisplayIf(sizeof(DateFormatType) != iValueSize,
											"Incorrect size for fixed-size locale setting");
				if (sizeof(DateFormatType) > iValueSize) {
					result = lmErrSettingDataOverflow;
				} else {
					*((DateFormatType*)oValue)
						= (DateFormatType)localeSettingsP->dateFormat;
				}
			break;
			
			case lmChoiceLongDateFormat:
				ErrNonFatalDisplayIf(sizeof(DateFormatType) != iValueSize,
											"Incorrect size for fixed-size locale setting");
				if (sizeof(DateFormatType) > iValueSize) {
					result = lmErrSettingDataOverflow;
				} else {
					*((DateFormatType*)oValue)
						= (DateFormatType)localeSettingsP->longDateFormat;
				}
			break;
			
			case lmChoiceTimeFormat:
				ErrNonFatalDisplayIf(sizeof(TimeFormatType) != iValueSize,
											"Incorrect size for fixed-size locale setting");
				if (sizeof(TimeFormatType) > iValueSize) {
					result = lmErrSettingDataOverflow;
				} else {
					*((TimeFormatType*)oValue)
						= (TimeFormatType)localeSettingsP->timeFormat;
				}
			break;
			
			case lmChoiceWeekStartDay:
				ErrNonFatalDisplayIf(sizeof(UInt16) != iValueSize,
											"Incorrect size for fixed-size locale setting");
				if (sizeof(UInt16) > iValueSize) {
					result = lmErrSettingDataOverflow;
				} else {
					*((UInt16*)oValue)
						= (UInt16)localeSettingsP->weekStartDay;
				}
			break;
			
			case lmChoiceTimeZone:
				ErrNonFatalDisplayIf(sizeof(Int16) != iValueSize,
											"Incorrect size for fixed-size locale setting");
				if (sizeof(Int16) > iValueSize) {
					result = lmErrSettingDataOverflow;
				} else {
					*((Int16*)oValue)
						= (Int16)localeSettingsP->timeZone;
				}
			break;
			
			case lmChoiceNumberFormat:
				ErrNonFatalDisplayIf(sizeof(NumberFormatType) != iValueSize,
											"Incorrect size for fixed-size locale setting");
				if (sizeof(NumberFormatType) > iValueSize) {
					result = lmErrSettingDataOverflow;
				} else {
					*((NumberFormatType*)oValue)
						= (NumberFormatType)localeSettingsP->numberFormat;
				}
			break;
			
			case lmChoiceCurrencyName:
				if (kMaxCurrencyNameLen >= iValueSize) {
					result = lmErrSettingDataOverflow;
				} else {
					MemMove(oValue, localeSettingsP->currencyName, kMaxCurrencyNameLen+1);
				}
			break;
			
			case lmChoiceCurrencySymbol:
				if (kMaxCurrencySymbolLen >= iValueSize) {
					result = lmErrSettingDataOverflow;
				} else {
					MemMove(oValue, localeSettingsP->currencySymbol, kMaxCurrencySymbolLen+1);
				}
			break;
			
			case lmChoiceUniqueCurrencySymbol:
				if (kMaxCurrencySymbolLen >= iValueSize) {
					result = lmErrSettingDataOverflow;
				} else {
					MemMove(	oValue,
								localeSettingsP->uniqueCurrencySymbol,
								kMaxCurrencySymbolLen+1);
				}
			break;
			
			case lmChoiceCurrencyDecimalPlaces:
				ErrNonFatalDisplayIf(sizeof(UInt16) != iValueSize,
											"Incorrect size for fixed-size locale setting");
				if (sizeof(UInt16) > iValueSize) {
					result = lmErrSettingDataOverflow;
				} else {
					*((UInt16*)oValue)
						= (UInt16)localeSettingsP->currencyDecimalPlaces;
				}
			break;
			
			case lmChoiceMeasurementSystem:
				ErrNonFatalDisplayIf(sizeof(MeasurementSystemType) != iValueSize,
											"Incorrect size for fixed-size locale setting");
				if (sizeof(MeasurementSystemType) > iValueSize) {
					result = lmErrSettingDataOverflow;
				} else {
					*((MeasurementSystemType*)oValue)
						= (MeasurementSystemType)localeSettingsP->measurementSystem;
				}
			break;
			
			default:
				result = lmErrBadLocaleSettingChoice;
			break;
		}
	}
	
	if (settingsP)
		MemPtrUnlock(settingsP);
	settingsP = NULL;
	
	return(result);
	
} // LmGetLocaleSetting


/***********************************************************************
 * Private routines
 ***********************************************************************/

/***********************************************************************/
static LmSettingsType*
PrvGetSettings(void)
/*
Return a locked pointer to the 'locs' resource.
*/
{
	MemHandle		settingsH = DmGetResource(	lmLocaleSettingsRscType,
															lmLocaleSettingsRscID);
	
	if (!settingsH)
		ErrDisplay("Can\'t find Locale Settings resource");
	
	return((LmSettingsType*)MemHandleLock(settingsH));
	
} // PrvGetSettings


#if EMULATION_LEVEL == EMULATION_NONE

/***********************************************************************/
asm void
LmDispatch(void)
/*
Dispatch to the appropriate Locale Manager routine. The selector is
passed in register d2.w; there are a variable number of parameters
on the stack, which we ignore
*/
{
	cmp.w		#lmMaxRoutineSelector,d2
	bhi		@badSelector
	add.w		d2,d2
	add.w		d2,d2						// Convert to long index.
	opword	0x4efb,0x2002			// jmp 4(pc,d2.w)

	/* The order of routines in this table MUST match the enumerated set
	of routine selectors in the LocaleMgr.h file.
	*/
	entry extern PrvDispatchTable

	jmp		@badSelector						// 0
	jmp		LmGetNumLocales					// 1
	jmp		LmLocaleToIndex					// 2
	jmp		LmGetLocaleSetting				// 3
	
@badSelector:
	move.w	d2,-(sp)					// Push the bad selector
	jmp		PrvSelectorError		//	and display it
	
} // LmDispatch


/***********************************************************************/
static void
PrvSelectorError(		LmRoutineSelector	iRoutineSelector)
/*
Report the fatal error that somebody tried to call us with the invalid
Locale Manager <iRoutineSelector>.
*/
{
	char text[64];
	char str[16];
	
	StrCopy(text, "Unsupported Locale Manager call: ");
	StrIToA(str, iRoutineSelector);
	StrCat(text, str);
	ErrDisplay(text);
	
} // PrvSelectorError

#endif // EMULATION_LEVEL == EMULATION_NONE
