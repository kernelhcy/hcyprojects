/******************************************************************************
 *
 * Copyright (c) 1995-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: DateTime.c
 *
 * Release: 
 *
 * Description:
 *	  This file contains the date & time manager routines.
 *
 * History:
 *		Feb 6, 1995	Created by Roger Flores
 *
 *****************************************************************************/

#include <PalmTypes.h>

#include <Chars.h>
#include <DateTime.h>
#include <DataMgr.h>
#include <ErrorMgr.h>
#include <MemoryMgr.h>
#include <StringMgr.h>
#include <TimeMgr.h>
#include <SysUtils.h>

#include <PalmUtils.h>

#include "UIResourcesPrv.h"
#include "PalmOptErrorCheckLevel.h"


// Max length of any date template string (before expansion), excluding the null.
#define	maxDateTemplateLen				31

// this table tells you how many days since the last 'daysInFourYears' boundary.
// That is, the first 12 entries give you the days in previous months for a leap year,
// e.g. table entry 3 (March) is 60 because there are 31 days in January and 29 in February
// each successive 12 entries give you the days in previous months for that year plus the
// days in previous years.  That allows you to do just one table lookup with (year % 4) + month
// to find out how many days have gone by since the start of the previous leap year.
// (See DateToDays for how this is used.)
//
// There's one extra entry at the end, so that this gives you the days in a given month:
// 		daysInPrevMonths[(year%4)*12+month] - daysInPrevMonth[(year%4)*12+month-1]
//
// It's OK to use just one table for all this, because over our valid date range
// (1904 - 2032) all years divisible by 4 are leap years.

static const UInt16 daysInPrevMonths[] ={
//		 +31		 +29		 +31		 +30		 +31			+30
	0, 		31,		60,		91,		121,		152,		// 1904, 1908, ...

//		 +31		 +31		 +30		 +31		 +30			+31
	182,		213,		244,		274,		305,		335,		// (leap years!)
												
//		   +31		 +28		 +31		 +30		 +31		+30
	366+0,	366+31,	366+59,	366+90,	366+120,	366+151,	// 1905, 1909, ...

//			+31		 +31		 +30		 +31		 +30		+31
	366+181,	366+212,	366+243, 366+273,	366+304,	366+334,
													
	731+0,	731+31,	731+59,	731+90,	731+120,	731+151,	// 1906, 1910, ...
	731+181,	731+212,	731+243, 731+273,	731+304,	731+334,
													
	1096+0,	1096+31,	1096+59,	1096+90,	1096+120,1096+151,// 1907, 1911, ...
	1096+181,1096+212,1096+243,1096+273,1096+304,1096+334,
	
	1461
};
	


/************************************************************
 * Private routines used only in this module
 *************************************************************/

#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL

Err PrvValidateDateTime(DateTimePtr dateTimeP, Boolean checkValue);
Err PrvValidateDate(DatePtr dateP, Boolean checkValue);
Err PrvValidateDateValue(Int16 month, Int16 day, Int16 year, Boolean checkDay);
Err PrvValidateTimeValue(UInt8 hours, UInt8 minutes, TimeFormatType timeFormat);

#define ECTimValidateDateTime(datetimeP, checkValue)			PrvValidateDateTime(dateTimeP, checkValue)
#define ECTimValidateDate(dateP, checkValue)						PrvValidateDate(dateP, checkValue)
#define ECTimValidateDateValue(month, day, year, checkDay)	PrvValidateDateValue(month, day, year, checkDay)
#define ECTimValidateTimeValue(hours, minutes, timeFormat)	PrvValidateTimeValue(hours, minutes, timeFormat)

#else

#define ECTimValidateDateTime(datetimeP, checkValue)
#define ECTimValidateDate(dateP, checkValue)
#define ECTimValidateDateValue(month, day, year, checkDay)
#define ECTimValidateTimeValue(hours, minutes, timeFormat)

#endif

/***********************************************************************
 *
 * FUNCTION:		GetStringListPtr
 *
 * DESCRIPTION:	Utility routine to return ptr to appropriate string from
 *						locked string list resource.
 *
 * PARAMETERS:		resID			- resource id of string list resource.
 *						index			- index into string list (0..n)
 *						resHandle	- Ptr to loaded & locked resource handle.
 *
 * RETURNED:		Ptr to string in locked resource.
 *
 * HISTORY:
 *		07/18/99	kwk	Created by Ken Krugler.
 *		09/03/00	CS		Cast result of MemHandleLock before assignment.
 *
 ***********************************************************************/
 
static const Char* GetStringListPtr(UInt16 resID, UInt16 index, MemHandle* resHandle)
{
	UInt8* resP;
	UInt16 numStrings;
	
	*resHandle = DmGetResource(strListRscType, resID);
	ErrNonFatalDisplayIf(*resHandle == NULL, "Missing string list handle");
	
	resP = (UInt8*)MemHandleLock(*resHandle);
	
	resP += StrLen((Char*)resP) + 1;
	
	numStrings = *resP++ << 8;
	numStrings |= (*resP++);
	
	ErrNonFatalDisplayIf(index >= numStrings, "Invalid string list index");
	
	while (index-- > 0) {
		resP += StrLen((Char*)resP) + 1;
	}
	
	return((Char*)resP);
} // GetStringListPtr


/***********************************************************************
 *
 * FUNCTION:		StrNCopyWithNull
 *
 * DESCRIPTION:	Utility routine to copy up to <n> bytes ala StrNCopy,
 *						and then terminate the resulting string with a null.
 *
 * PARAMETERS:		dstP			- destination string pointer.
 *						srcP			- source string pointer.
 *						n				- max number of bytes to copy.
 *
 * RETURNED:		Ptr to destination string.
 *
 * HISTORY:
 *		07/18/99	kwk	Created by Ken Krugler.
 *
 ***********************************************************************/
 
static Char* StrNCopyWithNull(Char* dstP, const Char* srcP, Int16 n)
{
	StrNCopy(dstP, srcP, n);
	dstP[n] = chrNull;
	return(dstP);
} // StrNCopyWithNull


/***********************************************************************
 *
 * FUNCTION: 	 TimAdjust
 *
 * DESCRIPTION: Return a new date +- the time adjustment.  This is useful
 * for advancing a day or week and not worrying about month and year wrapping.
 * If the time is advanced out of bounds it is is cut at the bounds surpassed.
 *
 * PARAMETERS:  struct tm, the adjustment in seconds
 *
 * RETURNED:	the adjust struct tm
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	1/31/95	Initial Revision
 *
 ***********************************************************************/
 
void TimAdjust(DateTimePtr dateTimeP, Int32 adjustment)
{
	UInt32 seconds;
	
	seconds = TimDateTimeToSeconds(dateTimeP);

	// we must be careful about large dates which appear negative if
	// signed.	
	if (adjustment < 0)
		{
		adjustment = -adjustment;
		if ((UInt32) adjustment > seconds)
			seconds = 0;
		else
			seconds -= (UInt32) adjustment;
		}
	else
		{
		seconds += adjustment;
		if (seconds > (UInt32) maxSeconds)
			seconds = maxSeconds;
		}

	TimSecondsToDateTime(seconds, dateTimeP);
}


/***********************************************************************
 *
 * FUNCTION: 	 TimDateTimeToSeconds
 *
 * DESCRIPTION: Return the date and time in seconds since 1/1/1904
 *
 * PARAMETERS:  struct tm
 *
 * RETURNED:	the time in seconds since 1/1/1904
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	1/31/95	Initial Revision
 *			bob	10/01/99	Re-write to use daysInPrevMonths table
 *
 ***********************************************************************/
 


UInt32 TimDateTimeToSeconds(DateTimePtr dateTimeP)
{
	UInt32 seconds;
	
	ECTimValidateDateTime(dateTimeP, true);
	
	// seconds is actually days right here
	seconds = ((UInt32) (dateTimeP->year - firstYear) / 4) * daysInFourYears;

	seconds += daysInPrevMonths[(dateTimeP->year % 4) * monthsInYear + dateTimeP->month - 1];
	
	seconds += dateTimeP->day - 1;
	
	// now convert days to seconds
	seconds = seconds * daysInSeconds;
	
	// now add in seconds in the day
	seconds += (UInt32) dateTimeP->hour * hoursInSeconds;
	
	seconds += dateTimeP->minute * minutesInSeconds;
	
	seconds += dateTimeP->second;

	return seconds;
}



/***********************************************************************
 *
 * FUNCTION:	TimSecondsToDateTime
 *
 * DESCRIPTION: Return the date and time given seconds
 *
 * PARAMETERS:  struct tm
 *
 * RETURNED:	the date and time given seconds since 1/1/1904
 *
 * REVISION HISTORY:
 *			Name		Date		Description
 *			----		----		-----------
 *			roger		1/31/95	Initial Revision
 *			ludovic	9/28/99	if seconds > maxSeconds set it to maxSeconds
 *			bob		10/01/99	Re-write to use daysInPrevMonths table
 *
 ***********************************************************************/

void TimSecondsToDateTime(UInt32 seconds, DateTimePtr dateTimeP)
{
	UInt32	days;
	UInt16	month;

	if (seconds > maxSeconds)
	{
		ErrNonFatalDisplay("TimSecondsToDateTime: seconds exceed max value");
		seconds = maxSeconds;
	}

	// Now convert from seconds to days.
	days = seconds / daysInSeconds;
	seconds = seconds % daysInSeconds;
	dateTimeP->weekDay = (days + 5) % daysInWeek;
	
	// get year to nearest leap year
	dateTimeP->year = firstYear + (days / daysInFourYears) * 4;

	// now figure out month/year within 4-year range
	days %= daysInFourYears;
	month = 0;
	while (days >= daysInPrevMonths[month])
		month++;
	month--;
	
	dateTimeP->year += month / monthsInYear;
	dateTimeP->month = (month % monthsInYear) + 1;
	dateTimeP->day = days - daysInPrevMonths[month] + 1;

	// now calc the time 
	dateTimeP->hour = seconds / hoursInSeconds;
	seconds = seconds % hoursInSeconds;
	dateTimeP->minute = seconds / minutesInSeconds;
	seconds = seconds % minutesInSeconds;
	dateTimeP->second = seconds;

}


/***********************************************************************
 *
 * FUNCTION:	TimTimeZoneToUTC
 *
 * DESCRIPTION: Convert a second count from a given time zone to Greenwich
 *				Mean Time (Universal Coordinated Time). Note that the result
 *				is not necessarily the time in Greenwich, since Greenwich
 *				may be observing daylight saving time.
 *
 *  PARAMETERS:	seconds since 1/1/1904
 *				positive (east) or negative (west) offset from GMT in minutes
 *				daylight saving adjustment in minutes (0 or 60)
 *
 *  RETURNS:	seconds since 1/1/1904
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	3/29/00	Initial Revision
 *
 ***********************************************************************/
UInt32 TimTimeZoneToUTC (UInt32 seconds, Int16 timeZone, Int16 daylightSavingAdjustment)
{
	Int32 offsetInSeconds;
	UInt32 result;

	ErrNonFatalDisplayIf(Abs(timeZone) > hoursInMinutes * hoursPerDay,
		"Invalid time zone");
	offsetInSeconds = (timeZone + daylightSavingAdjustment) * minutesInSeconds;
	
	// Do the math without losing the high bit of the seconds argument.
	if (offsetInSeconds >= 0)
	{
		result = seconds - (UInt32)offsetInSeconds;
		ErrNonFatalDisplayIf(result > seconds,
			"TimTimeZoneToUTC: seconds exceed min value");
	}
	else
	{
		result = seconds + (UInt32)(-offsetInSeconds);
		ErrNonFatalDisplayIf(result < seconds,
			"TimTimeZoneToUTC: seconds exceed max value");
	}
	return result;
}


/***********************************************************************
 *
 * FUNCTION:	TimUTCToTimeZone
 *
 * DESCRIPTION: Convert a second count from Greenwich Mean Time (Universal
 *				Coordinated Time) to a given time zone. Note that the input
 *				is not necessarily the time in Greenwich, since Greenwich
 *				may be observing daylight saving time.
 *
 *  PARAMETERS:	seconds since 1/1/1904
 *				positive (east) or negative (west) offset from GMT in minutes
 *				daylight saving adjustment in minutes (0 or 60)
 *
 *  RETURNS:	seconds since 1/1/1904
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			peter	3/29/00	Initial Revision
 *
 ***********************************************************************/
UInt32 TimUTCToTimeZone (UInt32 seconds, Int16 timeZone, Int16 daylightSavingAdjustment)
{
	Int32 offsetInSeconds;
	UInt32 result;

	ErrNonFatalDisplayIf(Abs(timeZone) > hoursInMinutes * hoursPerDay,
		"Invalid time zone");
	offsetInSeconds = (timeZone + daylightSavingAdjustment) * minutesInSeconds;
	
	// Do the math without losing the high bit of the seconds argument.
	if (offsetInSeconds >= 0)
	{
		result = seconds + (UInt32)offsetInSeconds;
		ErrNonFatalDisplayIf(result < seconds,
			"TimUTCToTimeZone: seconds exceed max value");
	}
	else
	{
		result = seconds - (UInt32)(-offsetInSeconds);
		ErrNonFatalDisplayIf(result > seconds,
			"TimUTCToTimeZone: seconds exceed min value");
	}
	return result;
}


/***********************************************************************
 *
 * FUNCTION: 	 DayOfWeekArray, private
 *
 * DESCRIPTION: Returns pointer to array of day of week offsets
 *
 *  PARAMETERS:  	void
 *
 *  RETURNS: UInt8 *
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/2/95	Initial Revision
 *
 ***********************************************************************/

static const UInt8 dayOfWeekArray[] = {4,0,6,2,4,0,2,5,1,3,6,1};

static UInt8 *	DayOfWeekArray(void)
{
	return (UInt8 *)dayOfWeekArray;
}


/***********************************************************************
 *
 * FUNCTION: 	 DayOfWeek
 *
 * DESCRIPTION: Returns the day of the week.
 *
 * PARAMETERS:  month, day, year (ex: 1995)
 *
 * RETURNED:	the day of the week (sunday = 0, monday=1, etc.)
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	1/31/95		Initial Revision
 *
 ***********************************************************************/

Int16 DayOfWeek (Int16 month, Int16 day, Int16 year)
{
	UInt16 year2000Correction = 0;
	
	ECTimValidateDateValue(month, day, year, true);
	
	// The year 2000 is a leap year.
	if (year > 2000)
		year2000Correction = 1;
	else if (year == 2000 && month > 2)
		year2000Correction = 1;
		
	year -= month < 3;
	return (year + year / 4 - year / 100 + DayOfWeekArray()[month-1] + 
		day + year2000Correction) % daysInWeek;
}


/***********************************************************************
 *
 * FUNCTION: 	 DayOfMonth
 *
 * DESCRIPTION: Returns the day of a month (ex: first Friday) on 
 *              which the specified date occurs.
 *
 * PARAMETERS:  month, day, year (ex: 1995)
 *
 * RETURNED:	the day of the month
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/14/95	Initial Revision
 *
 ***********************************************************************/

Int16 DayOfMonth (Int16 month, Int16 day, Int16 year)
{
	Int16 dayOfWeek;
	
	dayOfWeek = DayOfWeek (month, day, year);
	
	return ((day-1) / daysInWeek * daysInWeek + dayOfWeek);
}


/***********************************************************************
 *
 * FUNCTION: 	 DaysInMonth
 *
 * DESCRIPTION: Returns the number of days in the month
 *
 * PARAMETERS:  year, month
 *
 * RETURNED:	days in the month
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	1/31/95	Initial Revision
 *			bob	10/01/99	Re-write to use daysInPrevMonths table
 *
 ***********************************************************************/ 

Int16 DaysInMonth(Int16 month, Int16 year)
{
	ECTimValidateDateValue(month, 0, year, false);

	// add in previous years so we're relative to the last leap year
	month += (year % 4) * monthsInYear;

	// use the table
	return daysInPrevMonths[month] - daysInPrevMonths[month-1];
}


/***********************************************************************
 *
 * FUNCTION:    TimeToAscii
 *
 * DESCRIPTION:  Convert the time passed to an ascii string.
 *
 * PARAMETERS:  hours    - hours (0-23)
 *					 minutes	 - minutes (0-59)
 *					 timeFormat - how to format the time string
 *					 pString  - pointer to string which gets the result. The
 *									string must be of length timeStringLength
 *
 * RETURNED:	 pointer to the text of the current selection
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	12/2/94	Initial Revision
 *			roger 2/28/95	Changed and made available to system
 *			roger 8/5/96	Added new tfComma24h format
 *
 ***********************************************************************/

void TimeToAscii(UInt8 hours, UInt8 minutes, TimeFormatType timeFormat, Char * pString)
	{
	char t;
	
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	// Fill up the destination as much as allowed with a value to reveal errors.
	MemSet(pString, timeStringLength, 0xfe);	
#endif
	
	t = hours;
	if ( (timeFormat == tfColon24h) || (timeFormat == tfDot24h) ||
		  (timeFormat == tfHours24h) || (timeFormat == tfComma24h) )
		{
		ECTimValidateTimeValue(t, minutes, timeFormat);
	
		if (t >= 20)
			{
			t -= 20;
			*pString++ = '2';
			}
		}
	else
		{
		t %= 12;
	
		if (t == 0)
			{
			t = 2;
			*pString++ = '1';
			}
		else
			{
			ECTimValidateTimeValue(t, minutes, timeFormat);
			}
		}

	if (t >= 10)
		{
		t -= 10;
		*pString++ = '1';
		}
	*pString++ = '0' + t;


	// Now add the minutes
	if ( (timeFormat != tfHoursAMPM) && (timeFormat != tfHours24h) )
		{
		*pString++ = TimeSeparator(timeFormat);
	
		// Translate minutes in to text characters:
		*pString++ = '0' + minutes / 10;
		*pString++ = '0' + minutes % 10;
		}
			
	
	if ( (timeFormat == tfColonAMPM) || (timeFormat == tfDotAMPM) ||
		(timeFormat == tfHoursAMPM))
		{
		*pString++ = ' ';
		if (hours >= 12)
			*pString++ = 'p';
		else
			*pString++ = 'a';
		*pString++ = 'm';
		}

	*pString++ = '\0';
	}


/***********************************************************************
 *
 * FUNCTION: 	 DateAdjust
 *
 * DESCRIPTION: Return a new date +- the days adjustment.  This is useful
 * for advancing a day or week and not worrying about month and year wrapping.
 * If the time is advanced out of bounds it is is cut at the bounds surpassed.
 *
 * PARAMETERS:  struct tm, the adjustment in seconds
 *
 * RETURNED:	the adjust struct tm
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/21/95	Initial Revision
 *
 ***********************************************************************/
 
void DateAdjust (DatePtr dateP, Int32 adjustment)
{
	UInt32 days;
	
	ECTimValidateDate(dateP, false);
	
	days = DateToDays (*dateP);

	// we must be careful about large dates which appear negative if
	// signed.	
	if (adjustment < 0)
		{
		adjustment = -adjustment;
		if ((UInt32) adjustment > days)
			days = 0;
		else
			days -= (UInt32) adjustment;
		}
	else
		{
		days += adjustment;
		if (days > (UInt32) maxDays)
			days = maxDays;
		}

	DateDaysToDate (days, dateP);
}

/***********************************************************************
 *
 * FUNCTION: 	 DateToDays
 *
 * DESCRIPTION: Return the date in days since 1/1/1904
 *
 * PARAMETERS:  date - DateType structure
 *
 * RETURNED:	the days since 1/1/1904
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/14/95	Initial Revision
 *			bob	10/01/99	Re-write to use daysInPrevMonths table
 *
 ***********************************************************************/

UInt32 DateToDays (DateType date)
{
	UInt32 days;

	ECTimValidateDate(&date, true);

	days = ((UInt32) date.year / 4) * daysInFourYears;
	days += daysInPrevMonths[(date.year % 4) * monthsInYear + date.month - 1];
	days += date.day - 1;

	return days;
}


/***********************************************************************
 *
 * FUNCTION:	DateSecondsToDate
 *
 * DESCRIPTION: Return the date given seconds
 *
 * PARAMETERS:  seconds - seconds since 1/1/1904
 *              dateP - pointer to DateType structure (retuned) 
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	9/15/95	Initial Revision
 *
 ***********************************************************************/

void DateSecondsToDate (UInt32 seconds, DatePtr dateP)
{
	DateTimeType dateTime;

	ECTimValidateDate(dateP, false);

	TimSecondsToDateTime (seconds, &dateTime);
	dateP->year = dateTime.year - firstYear;
	dateP->month = dateTime.month;
	dateP->day = dateTime.day;
} 

/***********************************************************************
 *
 * FUNCTION:	DateDaysToDate
 *
 * DESCRIPTION: Return the date given days
 *
 * PARAMETERS:  days  -  days since 1/1/1904
 *              dateP - pointer to DateType structure
 *
 * RETURNED:	date
 *
 * REVISION HISTORY:
 *			Name		Date		Description
 *			----		----		-----------
 *			art		6/21/95	Initial Revision
 *			ludovic	9/28/99	if days > maxDays set it to maxDays
 *			bob		10/01/99	Re-write to use daysInPrevMonths table
 *
 ***********************************************************************/

void DateDaysToDate(UInt32 days, DatePtr dateP)
{
	UInt16 month;

	ECTimValidateDate(dateP, false);

	if (days > maxDays)
	{
		ErrNonFatalDisplay("DateDaysToDate: days exceed max value");
		days = maxDays;
	}

#if 1
	/* this is to work around a PPC compiler bug */
	{
		UInt32 tmp = (days / daysInFourYears) * 4;
	    dateP->year = tmp;
	}
#else
	dateP->year = (days / daysInFourYears) * 4;
#endif

	days %= daysInFourYears;

	month = 0;
	while (days >= daysInPrevMonths[month])
		month++;
	month--;
	
	dateP->year += month / monthsInYear;
	dateP->month = (month % monthsInYear) + 1;
	dateP->day = days - daysInPrevMonths[month] + 1;
}


/***********************************************************************
 *
 * FUNCTION:		IToShortYear
 *
 * DESCRIPTION:	Convert the passed year to a two-digit year and
 *						print it in the passed string.
 *
 * PARAMETERS:		Char *				pString - string to fill with result
 *						UInt16					years - number to convert
 *
 * RETURNED:		nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			kcr	10/4/95	Initial Revision
 *
 ***********************************************************************/

static void IToShortYear (Char * pString, UInt16 years)
	{
	if (years < 10)
		{	//	Single digit years must be preceeded by '0'
		*pString = '0';
		pString++;
		}

	StrIToA (pString, years);
	}	//	end of IToShortYear


/***********************************************************************
 *
 * FUNCTION:		DateToAscii
 *
 * DESCRIPTION:	Convert the date passed to an ascii string in the
 *						passed DateFormatType.
 *
 *						NOTE: this routine handles the long AND short forms
 *						of the date formats.
 *
 * PARAMETERS:  months   - months (1-12)
 *					 days		 - days (1-31)
 *					 years	 - years (ex: 1995)  If year is < 10 then a zero 
 *									precedes the year's digit resulting in at least 
 *									two digits.  This ruins displaying years < 10 A.D.
 *					 dateFormat - DateFormatType
 *					 pString  - pointer to string which gets the result. The
 *									string must be of length dateStringLength for
 *									standard formats or longDateStrLength for
 *									long date formats.
 *
 * RETURNED:		nothing
 *
 * HISTORY:
 *		02/28/95	roger	Created by Roger Flores.
 *		09/13/95	kcr	added dfDMYLongWithComma, dfYMDLongWithDot formats
 *		09/15/95	kcr	fixed two minor bugs in dfDMYLongWithComma & dfDMYLong
 *		09/22/95	kcr	dfDMYLongNoDay now uses full month names
 *		09/26/95	kcr	fixed 'long assignment to char str' problem
 *		10/03/95	kcr	added dfMYMed
 *		11/13/96	roger	Changed years to always print at least two digits
 *		03/11/97	scl	added dfMYMedNoPost for French 2.0 ROM
 *		07/18/99	kwk	Rewrote completely to use new DateTemplateToAscii routine.
 *		08/31/00	kwk	Use long length for filling buffer w/medium formats.
 *							Pass maxSize to DateTemplateToAscii, since it actually
 *							includes the terminating null byte. Added check for
 *							unknown date format.
 *
 ***********************************************************************/
void DateToAscii(UInt8 months, UInt8 days, UInt16 years,
					  DateFormatType dateFormat,
					  Char * pString)
{
	Char templateBuffer[maxDateTemplateLen + 1];
	UInt16 maxSize;

	// Calculate the maximum length in a backwards compatible manner. If the
	// format is a long or medium, use the longDateStrLength constant. Note that
	// we need to keep up-to-date with the defined long formats in DateTime.h
	if (((UInt16)dateFormat >= dfMDYLongWithComma)
	 && ((UInt16)dateFormat <= dfMYMedNoPost))
		{
		maxSize = longDateStrLength;
		}
	else
		{
		maxSize = dateStringLength;
		}

#if (ERROR_CHECK_LEVEL == ERROR_CHECK_FULL)
	// Fill up the destination as much as allowed with a value to reveal errors
	// caused by the caller's buffer being too small.
	// DOLATER kwk - should including terminating null.
	MemSet(pString, maxSize, 0xfe);	
#endif
	
	// Load the appropriate template string for <dateFormat>.
	SysStringByIndex(prefDateFormatsStrListID, (UInt16)dateFormat, templateBuffer,
							sizeof(templateBuffer) - 1);
	
	// If the index to SysStringByIndex is out of bounds, the result will
	// be the empty string.
	ErrNonFatalDisplayIf(templateBuffer[0] == chrNull, "Unknown dateFormat");
	
	// Call our low-level formatting routine to do the dirty work.
	DateTemplateToAscii(templateBuffer, months, days, years, pString, maxSize);
} // DateToAscii


/***********************************************************************
 *
 * FUNCTION:	DateToDOWDMFormat
 *
 * DESCRIPTION:  Convert the date passed to an ascii string.
 *
 * PARAMETERS:	month			- months (1-12)
 *					day			- days (1-31)
 *					year			- years
 *					dateFormat	- format to use for rest of date string.
 *					pString		- pointer to string which gets the result. The
 *									  string must be of length dowLongDateStrLength
 *									  or dowDateStringLength, depending on dateFormat.
 *
 * RETURNED:	 nothing
 *
 * HISTORY:
 *		05/30/95	rsf	Created by Roger Flores.
 *		01/18/98	kwk	Modified for days of week names != 4 bytes.
 *		07/18/99	kwk	Completely rewrote to use DateTemplateToAscii.
 *		08/31/00	kwk	Use long length for filling buffer w/medium formats.
 *							Pass maxSize to DateTemplateToAscii, since it actually
 *							includes the terminating null byte. Added check for
 *							unknown date format.
 *
 ***********************************************************************/

void DateToDOWDMFormat(UInt8 month, UInt8 day, UInt16 year,
	DateFormatType dateFormat, Char* pString)
{
	Char templateBuffer[maxDateTemplateLen + 1];
	UInt16 maxSize;
	
	// Calculate the maximum length in a backwards compatible manner. If the
	// format is a long format, use the longDateStrLength constant. Note that
	// we need to keep up-to-date with the defined long formats in Preferences.h
	if (((UInt16)dateFormat >= dfMDYLongWithComma)
	&& ((UInt16)dateFormat <= dfMYMedNoPost))
		{
		maxSize = dowLongDateStrLength;
		}
	else
		{
		maxSize = dowDateStringLength;
		}

#if (ERROR_CHECK_LEVEL == ERROR_CHECK_FULL)
	// Fill up the destination as much as allowed with a value to reveal errors
	// caused by the caller's buffer being too small.
	MemSet(pString, maxSize, 0xfe);	
#endif
	
	// Load the appropriate template string for <dateFormat>.
	SysStringByIndex(prefDOWDateFormatsStrListID, (UInt16)dateFormat, templateBuffer,
							sizeof(templateBuffer) - 1);
	
	// If the index to SysStringByIndex is out of bounds, the result will
	// be the empty string.
	ErrNonFatalDisplayIf(templateBuffer[0] == chrNull, "Unknown dateFormat");
	
	// Call our low-level formatting routine to do the dirty work.
	DateTemplateToAscii(templateBuffer, month, day, year, pString, maxSize);
} // DateToDOWDMFormat


/***********************************************************************
 *
 * FUNCTION:	DateTemplateToAscii
 *
 * DESCRIPTION:  Convert the date passed to an ascii string. The <stringP>
 *					param can be NULL, in which case the required string length
 *					is still returned. 
 *
 * PARAMETERS:	templateP	- ptr to template string used to format date.
 *					month			- months (1-12)
 *					day			- days (1-31)
 *					year			- years
 *					stringP		- pointer to string which gets the result.
 *					stringSize	- size of string buffer, including null.
 *
 * RETURNED:	Length of resulting string.
 *
 * HISTORY:
 *		07/18/99	kwk	Created by Ken Krugler.
 *		10/05/99	kwk	Don't muck w/years when handling short year format,
 *							as we need the unmunged year value for DOW name calcs.
 *		08/22/00	kwk	Fixed a bug where we were complaining about a null
 *						<stringP> parameter on debug ROMs, but that's OK.
 *
 ***********************************************************************/

UInt16 DateTemplateToAscii(const Char* templateP, UInt8 months, UInt8 days,
	UInt16 years, Char* stringP, Int16 stringSize)
{
	Int16 dstLen = 0;
	const Char* tempP = templateP;
	UInt16 mungedYears;
	
	ECTimValidateDateValue(months, days, years, true);
	
	ErrNonFatalDisplayIf(templateP == NULL, "Null parameter");
	
	// Walk through the template string until we reach the end of it.
	while (tempP != NULL) {
		Int16 copyLen;
		const Char* nextSubString = StrChr(tempP, dateTemplateChar);
		
		// If we didn't find a substring, then copy up to the end of the template.
		if (nextSubString == NULL) {
			copyLen = StrLen(templateP) - (tempP - templateP);
		} else {
			copyLen = nextSubString - tempP;
		}
		
		// We've got copyLen bytes to move unchanged from the template string
		// to the destination string.
		if (stringP != NULL) {
			StrNCopyWithNull(stringP + dstLen, tempP, min(copyLen, stringSize - 1 - dstLen));
		}
	
		dstLen += copyLen;
		
		// Now if we have a substring pointer, figure out how to deal with it.
		if (nextSubString != NULL)
			{
			MemHandle strListHandle = NULL;
			const Char* srcP = NULL;
			Char dateNum[maxStrIToALen];
			Char subStrType, subStrMod;
			UInt16 copyLen = 0;
			UInt16 strListID;
			
			nextSubString += 1;		// We know dateTemplateChar is single-byte.
			subStrType = *nextSubString++;
			
			// If we get a null, then it was a dateTemplateChar at the end of the
			// string, so we just want to copy that to the destination. Otherwise
			// we want to grab the next character, which should be the modifier.
			// If that's missing, then again treat this as a substring to be
			// copied.
			
			if (subStrType != chrNull)
				{
				subStrMod = *nextSubString++;
			
				switch (subStrMod)
					{
					case dateTemplateShortModifier:
					case dateTemplateRegularModifier:
					case dateTemplateLongModifier:
					case dateTemplateLeadZeroModifier:
					break;
					
					default:
						subStrType = chrNull;
						nextSubString--;
					break;
					}
				}
			
			// Decide how to set up srcP for the copy into the destination string buffer,
			// based on what the template string says to substitute.
			switch (subStrType)
				{
				case dateTemplateDayNum:
					if ((subStrMod == dateTemplateLeadZeroModifier)
					&& (days < 10))
						{
						dateNum[0] = chrDigitZero;
						dateNum[1] = chrNull;
						}
					else
						{
						dateNum[0] = chrNull;
						}
					
					StrIToA(dateNum + StrLen(dateNum), days);
					srcP = dateNum;
				break;
				
				case dateTemplateDOWName:
					if (subStrMod == dateTemplateShortModifier)
						{
						strListID = daysOfWeekShortStrListID;
						}
					else if (subStrMod == dateTemplateRegularModifier)
						{
						strListID = daysOfWeekStdStrListID;
						}
					else
						{
						strListID = daysOfWeekLongStrListID;
						}
					
					srcP = GetStringListPtr(strListID, DayOfWeek(months, days, years), &strListHandle);
				break;
				
				case dateTemplateMonthName:
					if (subStrMod == dateTemplateShortModifier)
						{
						strListID = monthNamesShortStrListID;
						}
					else if (subStrMod == dateTemplateRegularModifier)
						{
						strListID = monthNamesStdStrListID;
						}
					else
						{
						strListID = monthNamesLongStrListID;
						}
					
					srcP = GetStringListPtr(strListID, months - 1, &strListHandle);
				break;
				
				case dateTemplateMonthNum:
					if ((subStrMod == dateTemplateLeadZeroModifier)
					&& (months < 10))
						{
						dateNum[0] = chrDigitZero;
						dateNum[1] = chrNull;
						}
					else
						{
						dateNum[0] = chrNull;
						}
					
					StrIToA(dateNum + StrLen(dateNum), months);
					srcP = dateNum;
				break;
				
				case dateTemplateYearNum:
					if (subStrMod == dateTemplateShortModifier)
						{
						mungedYears = years % 100;
						}
					else
						{
						mungedYears = years;
						}
						
					// If we have a year less than 10, even in long date format,
					// we must pad with 0.
					if (mungedYears < 10)
						{
						dateNum[0] = chrDigitZero;
						dateNum[1] = chrNull;
						}
					else
						{
						dateNum[0] = chrNull;
						}
					
					StrIToA(dateNum + StrLen(dateNum), mungedYears);
					srcP = dateNum;
				break;
				
				// If our type character was a null, then we need to just
				// back up by a single character, as we didn't grab the
				// modifier char. This leaves us pointing at the terminating
				// null character, and we'll copy over the preceeding '^' char.
				
				case chrNull:
					nextSubString--;
					srcP = nextSubString - 1;
					copyLen = 1;
				break;
				
				// If we don't find anything that matches, then we want
				// to assume it's a ^<something else> sequence, so we
				// want to set up the copy ptr to be just the ^, and
				// advance past it.
				
				default:
					nextSubString -= 2;
					srcP = nextSubString - 1;
					copyLen = 1;
				break;
			}
			
			// If we haven't set an explicit copy amount, calc it now.
			if (copyLen == 0)
				{
				copyLen = StrLen(srcP);
				}
			
			// We've got copyLen bytes to move from the srcP string
			// to the destination string.
			if (stringP != NULL)
				{
				StrNCopyWithNull(stringP + dstLen, srcP, min(copyLen, stringSize - 1 - dstLen));
				}
			
			dstLen += copyLen;
			
			// If we loaded a string from a string list, unlock the resource handle now.
			if (strListHandle != NULL)
				{
				MemHandleUnlock(strListHandle);
				}
			}
		
		// Advance tempP to the section of the string following the substring.
		tempP = nextSubString;
		}
	
	return(dstLen);
} // DateTemplateToAscii


/***********************************************************************
 *
 * FUNCTION:	PrvValidateDateTime
 *
 * DESCRIPTION:  Check the parameter and value for inconsistency. 
 *
 * PARAMETERS:	dateTimeP	- struct tm to check.
 *					checkValue	- if true, check also datetime value consistency
 *
 * RETURNED:	0 if no error.
 *
 * HISTORY:
 *		07/18/99	Created by Ludovic Ferrandis.
 *
 ***********************************************************************/
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL

Err PrvValidateDateTime(DateTimePtr dateTimeP, Boolean checkValue)
{
	if (!dateTimeP) {
		ErrNonFatalDisplay("Invalid parameters");
		return memErrInvalidParam;
		}
	
	if (checkValue)
		return PrvValidateDateValue(dateTimeP->month, dateTimeP->day, dateTimeP->year, true);
	
	return 0;
}

#endif


/***********************************************************************
 *
 * FUNCTION:	PrvValidateDate
 *
 * DESCRIPTION:  Check the parameter for inconsistency value. 
 *
 * PARAMETERS:	dateP			- struct tm to check.
 *					checkValue	- if true, check also date value consistency
 *
 * RETURNED:	0 if no error.
 *
 * HISTORY:
 *		07/18/99	Created by Ludovic Ferrandis.
 *
 ***********************************************************************/
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL

Err PrvValidateDate(DatePtr dateP, Boolean checkValue)
{
	if (!dateP) {
		ErrNonFatalDisplay("Invalid parameters");
		return memErrInvalidParam;
		}
	
	if (checkValue)
		return PrvValidateDateValue(dateP->month, 0, dateP->year, false);
	
	return 0;
}

#endif

/***********************************************************************
 *
 * FUNCTION:	PrvValidateDateValue
 *
 * DESCRIPTION:  Check the parameter for inconsistency value. 
 *
 * PARAMETERS:	month, day, year.
 *
 * RETURNED:	0 if no error.
 *
 * HISTORY:
 *		07/18/99	Created by Ludovic Ferrandis.
 *		10/6/99  Eric Lapuyade : change the day test to allow for day 0. The fact is that
 *								 it is a valid value which means 'day out of system range'.
 *								 the checkDay parameter cannot be removed because it is used
 *								 to control recursivity (DaysInMonth also calls PrvValidateDateValue).
 *
 ***********************************************************************/

#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL

Err PrvValidateDateValue(Int16 month, Int16 day, Int16 year, Boolean checkDay)
{
	if (!((month >= january) && (month <= december) && 
			(year <= lastYear) &&
			(!checkDay || (checkDay && (day >= 0) && (day <= DaysInMonth(month, year)))))) {
		ErrNonFatalDisplay("Invalid parameters value");
		return memErrInvalidParam;
		}
	
	return 0;
}

#endif


/***********************************************************************
 *
 * FUNCTION:	PrvValidateTimeValue
 *
 * DESCRIPTION:  Check the parameter for inconsistency value. 
 *
 * PARAMETERS:	hours, minutes and time format.
 *
 * RETURNED:	0 if no error.
 *
 * HISTORY:
 *		07/18/99	Created by Ludovic Ferrandis.
 *
 ***********************************************************************/

#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL

Err PrvValidateTimeValue(UInt8 hours, UInt8 minutes, TimeFormatType timeFormat)
{
	Boolean use24Format = Use24HourFormat(timeFormat);
	
	if (!(((use24Format && (hours >= 0) && (hours < hoursPerDay)) || (!use24Format && (hours > 0) && (hours < 13))) &&
			((minutes >= 0) && (minutes < hoursInMinutes)))) {
		ErrNonFatalDisplay("Invalid parameters value");
		return memErrInvalidParam;
		}
	
	return 0;
}

#endif
