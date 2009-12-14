/******************************************************************************
 *
 * Copyright (c) 2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: SelTimeZone.c
 *
 * Release: 
 *
 * Description:
 *	  This file contains the time zone selection routines.
 *
 * History:
 *		03/02/2000	peter	Created by Peter Epstein.
 *		08/21/2000	kwk	Deleted SelectDaylightSavingAdjustment & helper routines.
 *
 *****************************************************************************/

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_LISTS

#include <PalmTypes.h>
#include <SystemPublic.h>

#include <PalmUtils.h>

#include "Day.h"
#include "Form.h"
#include "SelTimeZone.h"
#include "Event.h"
#include "UIResourcesPrv.h"

/***********************************************************************
 * Private constants.
 ***********************************************************************/

// The maximum number of characters in the gmt offset column of the time zone list.
#define maxGmtOffsetLength					10

// Horizontal pixel values for elements in the time zone display list.
#define descriptionColumnOffset			4
#define gmtOffsetColumnWidth				35
#define gmtColumnOffset						4

// How many more items are in time zone list when the caller doesn't want
// the current/new times display.
#define extraTimeZonesToShowWhenNoTimes	2

// The amount of the width of the time zone list object which is usable 
// for the custom draw procedure. Some space is needed to inset the item
// from the border, and space must always be left for the scroll arrows,
// even on items without scroll arrows, so the column lines up.
#define usableListWidth						139

/***********************************************************************
 * Private types.
 ***********************************************************************/

// DOLATER kwk -	Replace tzCountry with tzLocale, since CountryType might
//						get wider?

typedef struct {
	const Char* tzName;		// Ptr to name of time zone.
	Int16			tzOffset;	// GMT offset to time zone.
	CountryType	tzCountry;	// Which country contains the time zone.
	UInt8			tzReserved;	// Unused byte.
} TimeZoneEntryType;


/***********************************************************************
 * Private routines.
 ***********************************************************************/

static TimeZoneEntryType*
PrvCreateTimeZoneArray(MemHandle* timeZoneNames, UInt16* numTimeZones);

static void
PrvDeleteTimeZoneArray(TimeZoneEntryType* tzArrayP, MemHandle tzNamesH);

static Char*
PrvOffsetToString(Char* stringP, Int16 offset);

static Int16
PrvSearchTimeZoneNames(TimeZoneEntryType* tzArrayP, UInt16 numTimeZones, WChar firstChar);

static void
PrvSetTimeField(FormType * frm, UInt16 timeFieldID, MemHandle timeHandle,
	DateTimeType *time, Boolean drawField);

static void
PrvTimeZoneListDrawItem(Int16 itemNum, RectanglePtr bounds, Char** itemsText);

static void
PrvUpdateTimeFields(FormPtr frm,
	DateTimeType *currentTimeP, DateTimeType *newTimeP,
	MemHandle currentTimeHandle, MemHandle newTimeHandle,
	UInt16 currentTimeFieldID, UInt16 newTimeFieldID);


/***********************************************************************
 *
 * FUNCTION:    PrvCompareNameToTimeZoneEntry
 *
 * DESCRIPTION: Compare the string <searchData> to a TimeZoneEntryType
 *		record pointed at by <arrayData>.
 *
 * PARAMETERS:
 *		searchData	 ->	Pointer to string.
 *		arrayData	 ->	Pointer to entry in TimeZoneEntryType array.
 *		nameLen		 ->	Unused data.
 *
 * RETURNED:
 *		0 if equal.
 *		<1 if string sorts before entry name.
 *		>1 if string sorts after entry name.
 *
 * HISTORY:
 *		08/01/00	kwk	Created by Ken Krugler.
 *
 ***********************************************************************/
static Int16 PrvCompareNameToTimeZoneEntry(void const *searchData, void const *arrayData, Int32 nameLen)
{
	const Char* tzName = (const Char*)searchData;
	const TimeZoneEntryType* tzEntry = (const TimeZoneEntryType*)arrayData;
	
	return(StrNCaselessCompare(tzName, tzEntry->tzName, nameLen));
} // PrvCompareNameToTimeZoneEntry


/***********************************************************************
 *
 * FUNCTION:    PrvCompareTimeZoneEntries
 *
 * DESCRIPTION: Compare two time zone entries using the time zone name
 *		(if the countries don't match), otherwise the offset from GMT.
 *
 * PARAMETERS:
 *		entryA		 ->	Ptr to time zone entry.
 *		entryB		 ->	Ptr to time zone entry.
 *		other			 ->	Unused argument.
 *
 * RETURNED:
 *		0 if two entries are equal
 *		<0 if entryA sorts before entryB
 *		>0 if entryA sorts after entryB
 *
 * HISTORY:
 *		07/31/00	kwk	Created by Ken Krugler.
 *		08/23/00	kwk	Use GMT offset to compare two entries in the same
 *							country, so that all US entries (for example) are
 *							sorted west-to-east.
 *
 ***********************************************************************/
static Int16 PrvCompareTimeZoneEntries(void* entryA, void* entryB, Int32 /* other */)
{
	const TimeZoneEntryType* tzEntryA = (const TimeZoneEntryType*)entryA;
	const TimeZoneEntryType* tzEntryB = (const TimeZoneEntryType*)entryB;
	
	if (tzEntryA->tzCountry != tzEntryB->tzCountry)
	{
		return(StrCompare(tzEntryA->tzName, tzEntryB->tzName));
	}
	else
	{
		return(tzEntryA->tzOffset - tzEntryB->tzOffset);
	}
} // PrvCompareTimeZoneEntries


/***********************************************************************
 *
 * FUNCTION:    PrvCreateTimeZoneArray
 *
 * DESCRIPTION: Create the array of time zone entries from our string
 *		list, gtm offset list, and country list resources. Sort based on
 *		time zone name.
 *
 *	DOLATER kwk - we could save the time zone array we're creating here
 *		in memory, to avoid the performance hit of creating the array.
 *		On the other hand, then the list of names would need to stay
 *		locked down, unless we also copy those into the memory buffer,
 *		which means another 700+ bytes.
 *
 * PARAMETERS:
 *		timeZoneNames	<->	Ptr to returned handle to list of names.
 *		numTimeZones	<->	Ptr to count of number of time zones.
 *
 * RETURNED:
 *		Ptr to allocated array of time zone entry records.
 *
 * HISTORY:
 *		07/31/00	kwk	Created by Ken Krugler.
 *		08/23/00	kwk	Fixed bug where release ROMs caused ErrNonFatalDisplayIf
 *							to become a no-op, and thus the country and time zone
 *							offset list ptrs weren't skipping the count word.
 *
 ***********************************************************************/
static TimeZoneEntryType* PrvCreateTimeZoneArray(MemHandle* timeZoneNames, UInt16* numTimeZones)
{
	const Char* tzNamesP;
	TimeZoneEntryType* tzEntries;
	MemHandle offsetsH;
	MemHandle countriesH;
	UInt16* resP;
	Int16* gmtOffsetsP;
	UInt16* countriesP;
	UInt16 i;
	
	// Specify the number of items in the list, based on total # of items
	// in our time zone name list resource.
	*timeZoneNames = DmGetResource(strListRscType, TimeZoneNamesStringList);
	ErrNonFatalDisplayIf(*timeZoneNames == NULL, "No time zone names");
	tzNamesP = (const Char*)MemHandleLock(*timeZoneNames);
	
	// Skip over prefix string, then get the entry count.
	tzNamesP += StrLen(tzNamesP) + 1;
	*numTimeZones = *tzNamesP++;
	*numTimeZones = (*numTimeZones << 8) +  *tzNamesP++;
	
	// Allocate the array of time zone records.
	tzEntries = (TimeZoneEntryType*)MemPtrNew(*numTimeZones * sizeof(TimeZoneEntryType));
	ErrFatalDisplayIf(tzEntries == NULL, "Out of memory");
	
	// Find and lock down the gtm offset and country integer lists.
	offsetsH = DmGetResource(wrdListRscType, TimeZoneGMTOffsetsList);
	ErrNonFatalDisplayIf(offsetsH == NULL, "No time zone offsets");
	resP = (UInt16*)MemHandleLock(offsetsH);
	ErrNonFatalDisplayIf(*resP != *numTimeZones, "GMT offset count != name count");
	
	// Skip count at start of list.
	gmtOffsetsP = (Int16*)resP + 1;
	
	countriesH = DmGetResource(wrdListRscType, TimeZoneCountriesList);
	ErrNonFatalDisplayIf(countriesH == NULL, "No time zone countries");
	resP = (UInt16*)MemHandleLock(countriesH);
	ErrNonFatalDisplayIf(*resP != *numTimeZones, "Time zone country count != name count");
	
	// Skip count at start of list.
	countriesP = resP + 1;
	
	// Now loop to fill in all of the records.
	for (i = 0; i < *numTimeZones; i++)
	{
		tzEntries[i].tzName = tzNamesP;
		tzNamesP += StrLen(tzNamesP) + 1;
		tzEntries[i].tzOffset = gmtOffsetsP[i];
		tzEntries[i].tzCountry = (CountryType)countriesP[i];
	}
	
	MemHandleUnlock(offsetsH);
	MemHandleUnlock(countriesH);
	
	// Now sort the list, based on the time zone name.
	SysQSort(tzEntries, *numTimeZones, sizeof(TimeZoneEntryType), PrvCompareTimeZoneEntries, 0);
	
	return(tzEntries);
} // PrvCreateTimeZoneArray


/***********************************************************************
 *
 * FUNCTION:    PrvDeleteTimeZoneArray
 *
 * DESCRIPTION: Undo what PrvCreateTimeZoneArray has done, by deleting
 *		the array of time zone entries, and unlocking the list of names.
 *
 * PARAMETERS:
 *		tzArrayP		 ->	Ptr to allocated array of time zone entries.
 *		tzNamesH		 ->	Handle to string list resource containing time zone names.
 *
 * RETURNED:
 *		Nothing.
 *
 * HISTORY:
 *		07/31/00	kwk	Created by Ken Krugler.
 *
 ***********************************************************************/
static void PrvDeleteTimeZoneArray(TimeZoneEntryType* tzArrayP, MemHandle tzNamesH)
{
	MemPtrFree((MemPtr)tzArrayP);
	MemHandleUnlock(tzNamesH);
} // PrvDeleteTimeZoneArray


/***********************************************************************
 *
 * FUNCTION:    PrvOffsetToString
 *
 * DESCRIPTION: Convert a GMT offset into a string.
 *
 * PARAMETERS:
 *		stringP		<-		Buffer for resulting string.
 *		offset		 ->	Minutes +/- GMT.
 *
 * RETURNED:
 *		stringP	
 *
 * HISTORY:
 *		07/31/00	kwk	Created from GetTimeZoneTriggerText.
 *
 ***********************************************************************/
static Char* PrvOffsetToString(Char* stringP, Int16 offset)
{
	Char* savedStrP = stringP;
	Int16 hours, minutes;
	*stringP++ = offset > 0 ? chrPlusSign : chrHyphenMinus;
	offset = Abs(offset);
	hours = offset / hoursInMinutes;
	if (hours > 9)
	{
		*stringP++ = chrDigitZero + hours / 10;
	}
	
	*stringP++ = chrDigitZero + hours % 10;
	*stringP++ = chrColon;
	minutes = offset % hoursInMinutes;
	*stringP++ = chrDigitZero + minutes / 10;
	*stringP++ = chrDigitZero + minutes % 10;
	*stringP = chrNull;
	
	return(savedStrP);
} // PrvOffsetToString


/***********************************************************************
 *
 * FUNCTION:    PrvSearchTimeZoneNames
 *
 * DESCRIPTION: Search for the first name that starts with <chr>. If
 *		we find a match, return the index, otherwise return noListSelection.
 *
 * PARAMETERS:
 *		tzArrayP			 ->	Pointer to array of TimeZoneEntryType records.
 *		numTimeZones	 ->	Number of records in tzArrayP array.
 *		firstChar		 ->	Character to match to beginning of name.
 *
 * RETURNED:
 *		Index of matching entry (0...numTimeZones-1) or noListSelection if
 *		no match is found.
 *
 * HISTORY:
 *		08/01/00	kwk	Created by Ken Krugler.
 *
 ***********************************************************************/
static Int16 PrvSearchTimeZoneNames(TimeZoneEntryType* tzArrayP, UInt16 numTimeZones, WChar firstChar)
{
	Int32 position;
	Char buffer[maxCharBytes+1];
	buffer[TxtSetNextChar(buffer, 0, firstChar)] = '\0';
	
	if (SysBinarySearch(	tzArrayP,
								numTimeZones,
								sizeof(TimeZoneEntryType),
								PrvCompareNameToTimeZoneEntry,
								buffer,
								StrLen(buffer),
								&position,
								true))
	{
		return(position);
	}
	else
	{	
		return(noListSelection);
	}
} // PrvSearchTimeZoneNames


/***********************************************************************
 *
 * FUNCTION:    PrvSetTimeField
 *
 * DESCRIPTION: Set the given field's text to show a time and day of week.
 *
 * PARAMETERS:  frm - a pointer to the form containing the field to set
 *					 timeFieldID - the ID of the field to set
 *					 timeHandle - the handle used for storing the text for this field
 *					 time - a pointer to the date and time to show in the field
 *					 drawField - whether to draw field after setting its text
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	3/7/00	Initial Revision
 *
 ***********************************************************************/
static void PrvSetTimeField(FormType * frm, UInt16 timeFieldID, MemHandle timeHandle,
	DateTimeType *time, Boolean drawField)
{
	FieldType * timeFieldP;
	Char * timeString, * timeZoneDOWFormatString, * currentDOWString;
	MemHandle resHandle;
	TimeFormatType timeFormat;			// Format to display time in

	timeFormat = (TimeFormatType)PrefGetPreference(prefTimeFormat);
	
	timeString = MemHandleLock(timeHandle);
	TimeToAscii(time->hour, time->minute, timeFormat, timeString);
	currentDOWString = timeString + StrLen(timeString);
	currentDOWString[0] = ' ';
	currentDOWString++;

	resHandle = DmGetResource(strRsc, DOWformatString);
	ErrNonFatalDisplayIf(resHandle == NULL, "Missing string resource");
	timeZoneDOWFormatString = MemHandleLock(resHandle);
	DateTemplateToAscii(timeZoneDOWFormatString, time->month, time->day, time->year,
		currentDOWString, dowLongDateStrLength);
	MemHandleUnlock(resHandle);
	
	MemHandleUnlock(timeHandle);
	timeFieldP = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, timeFieldID));
	FldSetTextHandle(timeFieldP, timeHandle);
	
	if (drawField)
		FldDrawField(timeFieldP);
}


/***********************************************************************
 *
 * FUNCTION:    PrvTimeZoneListDrawItem
 *
 * DESCRIPTION: Draw the itemNum item of the time zone list. Used as draw
 *		callback routine for the time zone list object.
 *
 * PARAMETERS:
 *		itemNum		 ->	what item to draw
 *		bounds		 ->	rectangle to draw into 
 *		itemsText	 ->	ptr to array of TimeZoneEntryType records.
 *
 * RETURNED: nothing	
 *
 * HISTORY:
 *		03/07/00	peter	initial revision
 *		07/31/00	kwk	Modified to use TimeZoneEntryType records.
 *
 ***********************************************************************/
static void PrvTimeZoneListDrawItem(Int16 itemNum, RectanglePtr bounds, Char** itemsText)
{
	const TimeZoneEntryType* tzList = (const TimeZoneEntryType*)itemsText;
	Char gmtOffset[maxGmtOffsetLength + 1];
	
	// Skip to appropriate item to draw.
	tzList += itemNum;
	
	// Draw the name left-justified, and truncated.
	// DOLATER kwk - should we worry about the text in ZeroTimeZoneOffsetString
	// being wider than gmtOffsetColumnWidth?
	WinDrawTruncChars(tzList->tzName,
							StrLen(tzList->tzName),
							bounds->topLeft.x + descriptionColumnOffset,
							bounds->topLeft.y,
							usableListWidth - descriptionColumnOffset - gmtOffsetColumnWidth);
	
	// Draw the offset right-justified.
	if (tzList->tzOffset == 0)
	{
		SysCopyStringResource(gmtOffset, ZeroTimeZoneOffsetString);
	}
	else
	{
		PrvOffsetToString(gmtOffset, tzList->tzOffset);
	}
	
	WinDrawChars(	gmtOffset,
						StrLen(gmtOffset),
						bounds->topLeft.x + usableListWidth - gmtColumnOffset - FntCharsWidth(gmtOffset, StrLen(gmtOffset)),
						bounds->topLeft.y);
} // PrvTimeZoneListDrawItem


/***********************************************************************
 *
 * FUNCTION:    PrvUpdateTimeFields
 *
 * DESCRIPTION: Update the current time and new time displayed if they have
 *					 changed in the time zone dialog. Also used to update these
 *					 times in the daylight saving time dialog.
 *
 * PARAMETERS:  frm	 - the time zone or daylight saving time dialog
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	4/13/00	Initial Revision
 *
 ***********************************************************************/
static void PrvUpdateTimeFields (FormPtr frm,
	DateTimeType *currentTimeP, DateTimeType *newTimeP,
	MemHandle currentTimeHandle, MemHandle newTimeHandle,
	UInt16 currentTimeFieldID, UInt16 newTimeFieldID)
{
	DateTimeType now, then;
	UInt32 delta;

	TimSecondsToDateTime(TimGetSeconds(), &now);

	if (now.minute != currentTimeP->minute ||
		now.hour != currentTimeP->hour ||
		now.day != currentTimeP->day ||
		now.month != currentTimeP->month ||
		now.year != currentTimeP->year)
	{
		then = *currentTimeP;
		*currentTimeP = now;
		PrvSetTimeField(frm, currentTimeFieldID, currentTimeHandle, currentTimeP, true);
		if (FldGetTextLength(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, newTimeFieldID))) != 0)
		{
			delta = TimDateTimeToSeconds(newTimeP) - TimDateTimeToSeconds(&then);
			TimSecondsToDateTime(TimDateTimeToSeconds(currentTimeP) + delta, newTimeP);
			PrvSetTimeField(frm, newTimeFieldID, newTimeHandle, newTimeP, true);
		}
	}
} // PrvUpdateTimeFields


#pragma mark ------- Public Routines -------

/***********************************************************************
 *
 * FUNCTION:    TimTimeZoneToAscii
 *
 * DESCRIPTION: Convert the time zone passed to an ascii string.
 *
 * PARAMETERS:
 *		timeZone				 ->	time zone to show
 *		localeP				 ->	locale found in time zone to show.
 *		stringP				 ->	string in which to store ascii
 *										(must fit timeZoneStringLength),
 *					
 * RETURNED: nothing
 *
 * HISTORY:
 *		03/09/00	peter	Created by Peter Epstein
 *		04/12/00	peter	API changed to truncate to fit given width
 *		04/14/00	peter	Add font argument
 *		04/27/00	peter	Wait to delete form until listP is done being used
 *		08/01/00	kwk	Modified to use time zone array.
 *		08/02/00	kwk	Added countryInTimeZone to API.
 *		11/17/00	CS		Change GetTimeZoneTriggerText's countryInTimeZone
 *							parameter to localeInTimeZoneP, since CountryType is
 *							only a UInt8, and this may change someday.
 *		12/11/00	peter	Rename function and elminate width and font parameters
 *							Caller should truncate and add ellipsis if needed.
 *
 ***********************************************************************/
void TimeZoneToAscii(Int16 timeZone, const LmLocaleType* localeP, Char* string)
{
	TimeZoneEntryType* tzArrayP;
	UInt16 numTimeZones;
	MemHandle tzNamesH;
	Int16 timeZoneIndex;
	Boolean foundMatch = false;

	tzArrayP = PrvCreateTimeZoneArray(&tzNamesH, &numTimeZones);
	for (timeZoneIndex = 0; (timeZoneIndex < numTimeZones) && !foundMatch; timeZoneIndex++)
	{
		if ((tzArrayP[timeZoneIndex].tzOffset == timeZone)
		 && (tzArrayP[timeZoneIndex].tzCountry == localeP->country))
		{
			foundMatch = true;
			StrCopy(string, tzArrayP[timeZoneIndex].tzName);
		}
	}
	
	// If no match was found, display the time zone as an offset from GMT
	if (!foundMatch)
	{
		Char* s;

		SysCopyStringResource(string, UTCString);
		s = string + StrLen(string);		// where to store offset from GMT when necessary
		
		if (timeZone != 0)
		{
			PrvOffsetToString(s, timeZone);
		}
	}

	PrvDeleteTimeZoneArray(tzArrayP, tzNamesH);
}


/***********************************************************************
 *
 * FUNCTION:    SelectTimeZone
 *
 * DESCRIPTION: Display a form showing a time zone and allow the user
 *              to select a different time zone. This is the time zone
 *					 dialog as seen in Date & Dime panel
 *
 * PARAMETERS:
 *		ioTimeZoneP				<->	pointer to time zone to change
 *		ioLocaleInTimeZoneP	<->	Ptr to locale found in time zone.
 *		titleP					 ->	String title for the dialog.
 *		showTimes				 -> 	True => show current and new times
 *		anyLocale				 ->	True => ignore ioLocaleInTimeZoneP on entry.
 *
 * RETURNED:
 *		true if the OK button was pressed (in which case *ioTimeZoneP and
 *		*ioCountryInTimeZoneP might be changed).
 *
 * HISTORY:
 *		03/02/00	peter	Created by Peter Epstein.
 *		04/03/00	peter	Allow NULL currentTimeP.
 *		04/12/00	peter API changed to get rid of trigger text
 *		04/14/00	peter Update current & new time as time passes
 *		07/31/00	kwk	Use SysTicksPerSecond() routine vs. sysTicksPerSecond macro.
 *					kwk	Re-wrote to use set of resources (name, offset, country),
 *							scrollbar vs. arrows, etc.
 *		08/01/00	kwk	Support scroll-to-key. Fixed scrollbar/list sync bugs.
 *		08/02/00	kwk	New API w/ioCountryInTimeZoneP and anyCountry parameters.
 *					kwk	Call FrmHandleEvent _after_ our event handling code has
 *							decided that it doesn't want to handle the event, not before.
 *		08/03/00	kwk	Call LstSetListChoices before calling LstGetVisibleItems,
 *							as otherwise accessing the time zone picker from the
 *							Setup app (when <showTimes> is false) gives you a two-
 *							line high display because LstGetVisibleItems returns 0.
 *		08/18/00	kwk	Play error sound if user writes letter that doesn't
 *							match any entries.
 *					kwk	Don't select item if doing scroll-to-view for entry
 *							that matches the letter the user wrote.
 *		08/21/00	kwk	Scroll-to-view with text entry now scrolls to the top
 *							of the list, versus the middle.
 *		10/09/00	peter	Get rid of scroll bar and let list do the scrolling.
 *		11/17/00	CS		Change ioCountryInTimeZoneP to ioLocaleInTimeZoneP,
 *							(and anyCountry to anyLocale, but that doesn't matter),
 *							since CountryType is only a UInt8, and this may
 *							change someday.
 *
 ***********************************************************************/
Boolean SelectTimeZone(Int16 *ioTimeZoneP, LmLocaleType* ioLocaleInTimeZoneP,
				const Char *titleP, Boolean showTimes, Boolean anyLocale)
{
	FormType* originalForm;
	FormType* dialog;
	EventType event;
	Boolean confirmed = false;
	Boolean done = false;
	Boolean adjustTimes = false;
	Boolean foundLocale = false;
	MemHandle currentTimeHandle, newTimeHandle;
	ListPtr listP;
	Int16 oldTimeZone, newTimeZone, testTimeZone;
	LmLocaleType newTimeZoneLocale;
	Int16 delta, closestDelta, timeZoneIndex, closestIndex;
	DateTimeType currentTime, newTime;
	TimeZoneEntryType* tzArrayP;
	UInt16 numTimeZones;
	MemHandle tzNamesH;
	
	if (showTimes)
	{
		TimSecondsToDateTime(TimGetSeconds(), &currentTime);
	}
	
	oldTimeZone = *ioTimeZoneP;
	newTimeZone = oldTimeZone;
	newTimeZoneLocale = *ioLocaleInTimeZoneP;
	
	originalForm = FrmGetActiveForm();
	dialog = (FormType *) FrmInitForm (TimeZoneDialogForm); 
	listP = FrmGetObjectPtr (dialog, FrmGetObjectIndex (dialog, TimeZoneDialogTimeZoneList));
	
	if (titleP)
	{
		FrmSetTitle (dialog, (Char *) titleP);
	}
	
	FrmSetActiveForm (dialog);
	
	// We need to call LstSetListChoices before calling LstSetHeight below, as otherwise
	// LstGetVisibleItems will return 0.
	tzArrayP = PrvCreateTimeZoneArray(&tzNamesH, &numTimeZones);
	LstSetListChoices(listP, (Char**)tzArrayP, numTimeZones);
	
	if (showTimes)
	{
		currentTimeHandle = MemHandleNew(timeStringLength + 1 + dowLongDateStrLength + 1);
		ErrFatalDisplayIf (!currentTimeHandle, "Out of memory");
		newTimeHandle = MemHandleNew(timeStringLength + 1 + dowLongDateStrLength + 1);
		ErrFatalDisplayIf (!newTimeHandle, "Out of memory");

		PrvSetTimeField(dialog, TimeZoneDialogCurrentTimeField, currentTimeHandle, &currentTime, false);
	}
	else
	{
		// Hide the current and new time.
		FrmHideObject(dialog, FrmGetObjectIndex (dialog, TimeZoneDialogCurrentTimeLabel));
		FrmHideObject(dialog, FrmGetObjectIndex (dialog, TimeZoneDialogCurrentTimeField));
		FrmHideObject(dialog, FrmGetObjectIndex (dialog, TimeZoneDialogNewTimeLabel));
		FrmHideObject(dialog, FrmGetObjectIndex (dialog, TimeZoneDialogNewTimeField));
		
		// Make the list show more items to take up extra the space.
		LstSetHeight(listP, LstGetVisibleItems(listP) + extraTimeZonesToShowWhenNoTimes);
	}
	
	// Find the time zone in the list closest to the current time zone, and that
	// matches <*ioLocaleInTimeZoneP> if <anyLocale> is false.
	closestDelta = hoursInMinutes * hoursPerDay;		// so big that all others will be smaller
	for (timeZoneIndex = 0; timeZoneIndex < numTimeZones; timeZoneIndex++)
	{
		Boolean checkDelta = anyLocale;
		testTimeZone = tzArrayP[timeZoneIndex].tzOffset;
		delta = Abs(testTimeZone - oldTimeZone);
		
		if (!anyLocale)
		{
			if (tzArrayP[timeZoneIndex].tzCountry == ioLocaleInTimeZoneP->country)
			{
				// If we haven't previously found a matching locale, reset the
				// delta so that this entry overrides any previous best entry.
				if (!foundLocale)
				{
					foundLocale = true;
					closestDelta = hoursInMinutes * hoursPerDay;
				}
				
				checkDelta = true;
			}
			
			// If we haven't yet found a matching locale, go for the closest delta.
			else
			{
				checkDelta = !foundLocale;
			}
		}
		
		// If we want to check the time zone delta, do it now.
		if (checkDelta && (delta < closestDelta))
		{
			closestIndex = timeZoneIndex;
			closestDelta = delta;
		}
	}
	
	// Scroll so that time zone is in the center of the screen and select it if it's an exact match.
	LstSetTopItem(listP, max(0, closestIndex - (LstGetVisibleItems(listP) / 2)));
	if ((closestDelta == 0) && (anyLocale || foundLocale))
	{
		LstSetSelection(listP, closestIndex);
		if (showTimes)
		{
			newTime = currentTime;
			PrvSetTimeField(dialog, TimeZoneDialogNewTimeField, newTimeHandle, &newTime, false);
		}
	}
	else
	{
		LstSetSelection(listP, noListSelection);
	}
	
	LstSetDrawFunction(listP, PrvTimeZoneListDrawItem);
	
	FrmDrawForm (dialog);
	
	while (!done)
	{
		Boolean handled = false;
		EvtGetEvent(&event, SysTicksPerSecond());		// so we can update the current and new time

		if (SysHandleEvent ((EventType *)&event))
		{
			continue;
		}
		
		if (event.eType == nilEvent)
		{
			if (showTimes)
			{
				PrvUpdateTimeFields(	dialog,
										&currentTime,
										&newTime,
										currentTimeHandle,
										newTimeHandle,
										TimeZoneDialogCurrentTimeField,
										TimeZoneDialogNewTimeField);
			}
		}
		
		else if (event.eType == ctlSelectEvent)
		{
			handled = true;
			
			switch (event.data.ctlSelect.controlID)
			{
				case TimeZoneDialogOKButton:
					// Set the new time zone.
					*ioTimeZoneP = newTimeZone;
					*ioLocaleInTimeZoneP = newTimeZoneLocale;

					done = true;
					confirmed = true;
				break;

				case TimeZoneDialogCancelButton:
					done = true;
				break;
				
				default:
					ErrNonFatalDisplay("Unknown control in form");
				break;
			}
		}
		
		// User tapped on a time zone in the list.
		else if (event.eType == lstSelectEvent)
		{
			UInt16 localeIndex;
			
			ErrNonFatalDisplayIf(event.data.lstSelect.listID != TimeZoneDialogTimeZoneList,
										"Unknown list in form");
			
			newTimeZone = tzArrayP[event.data.lstSelect.selection].tzOffset;
			newTimeZoneLocale.country =
				tzArrayP[event.data.lstSelect.selection].tzCountry;
			newTimeZoneLocale.language = lmAnyLanguage;
			if (LmLocaleToIndex(&newTimeZoneLocale, &localeIndex) == errNone)
			{
				if (LmGetLocaleSetting(	localeIndex,
												lmChoiceLocale,
												&newTimeZoneLocale,
												sizeof(newTimeZoneLocale)))
				{
					ErrNonFatalDisplay("Can\'t get locale");
				}
			}
			adjustTimes = showTimes;
			handled = true;
		}

		else if (event.eType == keyDownEvent)
		{
			if	(!TxtCharIsHardKey(event.data.keyDown.modifiers, event.data.keyDown.chr))
			{
				//	Hard scroll buttons
				if (EvtKeydownIsVirtual(&event))
				{
					if (event.data.keyDown.chr == vchrPageUp)
					{
						handled = true;
						LstScrollList(listP, winUp, LstGetVisibleItems(listP) - 1);
					}
					else if (event.data.keyDown.chr == vchrPageDown)
					{
						handled = true;
						LstScrollList(listP, winDown, LstGetVisibleItems(listP) - 1);
					}
				}
				else if (TxtCharIsPrint(event.data.keyDown.chr))
				{
					Int16 index;
					
					handled = true;
					index = PrvSearchTimeZoneNames(tzArrayP, numTimeZones, event.data.keyDown.chr);
					
					if (index != noListSelection)
					{
						Int16 delta = index - listP->topItem;
						if (delta < 0)
						{
							LstScrollList(listP, winUp, -delta);
						}
						else if (delta > 0)
						{
							LstScrollList(listP, winDown, delta);
						}
					}
					else
					{
						SndPlaySystemSound(sndError);
					}
				}
			}
		}

		else if (event.eType == appStopEvent)
		{
			EvtAddEventToQueue (&event);
			done = true;
			break;
		}
		
		// If we didn't handle the event, give the form code a crack at it.
		// This simulates the "normal" method of installing an event handler
		// for a form, which gets called, and then if it returns false, the
		// FrmHandleEvent routine gets called.
		if (!handled)
		{
			FrmHandleEvent(dialog, &event); 
		}
		
		// If something changed, and we need to update our time display,
		// do it now.
		if (adjustTimes)
		{
			adjustTimes = false;
			newTime = currentTime;
			TimAdjust(&newTime, (Int32)(newTimeZone - oldTimeZone) * minutesInSeconds);
			PrvSetTimeField(dialog, TimeZoneDialogNewTimeField, newTimeHandle, &newTime, true);
		}
	}	// end while true
		
	if (showTimes)
	{
		FldSetTextHandle(FrmGetObjectPtr (dialog, FrmGetObjectIndex (dialog, TimeZoneDialogCurrentTimeField)), NULL);
		FldSetTextHandle(FrmGetObjectPtr (dialog, FrmGetObjectIndex (dialog, TimeZoneDialogNewTimeField)), NULL);
		MemHandleFree(currentTimeHandle);
		MemHandleFree(newTimeHandle);
	}
	
	FrmEraseForm (dialog);
	FrmDeleteForm (dialog);
	FrmSetActiveForm(originalForm);

	PrvDeleteTimeZoneArray(tzArrayP, tzNamesH);

	return confirmed;
} // SelectTimeZone
