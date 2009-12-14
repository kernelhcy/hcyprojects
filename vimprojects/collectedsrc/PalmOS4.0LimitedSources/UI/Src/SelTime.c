/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: SelTime.c
 *
 * Release: 
 *
 * Description:
 *	  This file contain the selection time routines.
 *   It might be worthwhile using the struct tm instead of TimeType.
 *
 * History:
 *		December 2, 1994	Created by Roger Flores
 *      mm/dd/yy   initials - brief revision comment
 *
 *****************************************************************************/

#include <PalmTypes.h>
#include <SystemPublic.h>

#include <PalmUtils.h>

#include "Day.h"
#include "Form.h"
#include "SelTime.h"
#include "Event.h"
#include "UIResourcesPrv.h"

/***********************************************************************
 *
 *	Internal Constants
 *
 ***********************************************************************/
#define defaultTimeButton		timeHourButton  // default button in One time dlog
#define noDisplayOfAllDay		-1
typedef enum {
	changingStartTime, changingEndTime, changingNoTime
	} ChangingTimeType;
	

/***********************************************************************
 *
 * FUNCTION:    TimeDifference
 *
 * DESCRIPTION: Subtract pTime2 from pTime1 and place the result in
 *					 pTimeDiff.
 *
 *					 Note that TimeType is now unsigned so this doesn't
 *					 work for negative times (which no longer exist!).
 *
 * PARAMETERS:  pTime1   - pointer to TimeType
 *              pTime2   - pointer to TimeType
 *              pTimeDiff- pointer to TimeType
 *
 * RETURNED:	 pTimeDiff is set to pTime1 - pTime2.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	12/2/94	Initial Revision
 *
 ***********************************************************************/

static void TimeDifference(TimePtr pTime1, TimePtr pTime2, TimePtr pTimeDiff)
	{
	pTimeDiff->hours = pTime1->hours - pTime2->hours;

	if (pTime1->minutes < pTime2->minutes)
		{
		pTimeDiff->minutes = pTime1->minutes + hoursInMinutes - pTime2->minutes;
		pTimeDiff->hours--;
		}
	else
		pTimeDiff->minutes = pTime1->minutes - pTime2->minutes;
	
	}



/***********************************************************************
 *
 * FUNCTION: 	 Hours12Array, private
 *
 * DESCRIPTION: Returns pointer to array of hours
 *
 *  PARAMETERS:  	void
 *
 *  RETURNS: UInt8 *
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/2/95	Initial Revision
 *			SCL	9/18/98	Changed "numSpace" from 0x80 to new (3.1+) value (0x19)
 *
 ***********************************************************************/

// DOLATER kwk - This array (and the 24 hour array) should be a resource, not a
// hard-coded table...
#define numSpace	"\x19"
#define nullTerm	"\x00"

#define hours12Array		\
	"12A"		 nullTerm	\
	numSpace "1" nullTerm	\
	numSpace "2" nullTerm	\
	numSpace "3" nullTerm	\
	numSpace "4" nullTerm	\
	numSpace "5" nullTerm	\
	numSpace "6" nullTerm	\
	numSpace "7" nullTerm	\
	numSpace "8" nullTerm	\
	numSpace "9" nullTerm	\
	"10"		 nullTerm	\
	"11"		 nullTerm	\
	"12P"		 nullTerm	\
	numSpace "1" nullTerm	\
	numSpace "2" nullTerm	\
	numSpace "3" nullTerm	\
	numSpace "4" nullTerm	\
	numSpace "5" nullTerm	\
	numSpace "6" nullTerm	\
	numSpace "7" nullTerm	\
	numSpace "8" nullTerm	\
	numSpace "9" nullTerm	\
	"10"		 nullTerm	\
	"11"		 nullTerm

static UInt8 *	Hours12Array(void)
{
	return (UInt8 *)hours12Array;
}


/***********************************************************************
 *
 * FUNCTION: 	 Hours24Array, private
 *
 * DESCRIPTION: 	Returns pointer to array of hours
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
#define hours24Array		\
	numSpace "0" nullTerm	\
	numSpace "1" nullTerm	\
	numSpace "2" nullTerm	\
	numSpace "3" nullTerm	\
	numSpace "4" nullTerm	\
	numSpace "5" nullTerm	\
	numSpace "6" nullTerm	\
	numSpace "7" nullTerm	\
	numSpace "8" nullTerm	\
	numSpace "9" nullTerm	\
	"10"		 nullTerm	\
	"11"		 nullTerm	\
	"12"		 nullTerm	\
	"13"		 nullTerm	\
	"14"		 nullTerm	\
	"15"		 nullTerm	\
	"16"		 nullTerm	\
	"17"		 nullTerm	\
	"18"		 nullTerm	\
	"19"		 nullTerm	\
	"20"		 nullTerm	\
	"21"		 nullTerm	\
	"22"		 nullTerm	\
	"23"		 nullTerm

static UInt8 *	Hours24Array(void)
{
	return (UInt8 *)hours24Array;
}


/***********************************************************************
 *
 * FUNCTION:    Translate12HourTime
 *
 * DESCRIPTION: This routine handles key events in the Date Picker.  
 *              Key events are translated into times.
 *
 * PARAMETERS:  chr         - character written
 *              firstHour   - first valid hour
 *              timeStr     - passed: previously written character
 *                            returned: written character, including char passed
 *					 time			 - passed: previously translated time
 *                            returned: the translated time
 *
 * RETURNED:	 true if sucessful
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/2/96	Initial Revision
 *
 ***********************************************************************/
static Boolean Translate12HourTime (Char chr, Int16 firstHour, 
	Char * timeStr, TimePtr time)
	{
	UInt16 i;
	Int16 len;
	UInt8 num;
	UInt8 hours, minutes;
	Boolean accept = false;
	Boolean am = false;
	Boolean pm = false;
	
	
	len = StrLen (timeStr);
	
	// Was the last character accepted an AM or PM character.
	if (len && (timeStr[len-1] < '0' || timeStr[len-1] > '9'))
		;
	
	// Was a digit written?
	else if (chr >= '0' && chr <= '9')
		{
		// The first character can be 0 - 9.
		if (len == 0)
			accept = true;

		// The second character can be:
		//		0 - 9  if the first character is a 0.
		//		0 - 6  if the first character is a 1.
		//		0 - 6  if the first character is greater than 1 (first minute 
		//           digit).
		else if (len == 1 && (
						(*timeStr == '0' || chr < '6')))
			accept = true;

		// The third character can be:
		//		0 - 6   if the first character is 0 (first minute digit).
		//		0 - 6   if the first character is 1 and the second character is 
		//            less then or equal to 2 (first minute digit).
		//		0 or 5  if the first character is greater than 1 (second munute digit). 
		else if (len == 2 && (
						(*timeStr == '0' && chr < '6') ||
						(*timeStr == '1' && timeStr[1] <= '2' && chr < '6') ||
						(chr == '0' || chr == '5')))
			accept = true;

		// The forth character can be:
		//		0 or 5  if the if the first character is 0 (second minute digit).
		//		0 or 5  if the first character is 1 and the second character is 
		//            less then or equal to 2 (second minute digit).
		else if (len == 3 && 
					(chr == '0' || chr == '5') && 
					(timeStr[0] == '0' || (timeStr[0] == '1' && timeStr[1] <= '2')))
			accept = true;
		}
			
	// Was the AM character written?
	else if (len && (chr == 'a' || chr == 'A'))
		{
		accept = true;
		am = true;
		}

	// Was the PM character written?
	else if ( len && (chr == 'p' || chr == 'P'))
		{
		accept = true;
		pm = true;
		}

	
	// Add the character passed to the time string if it is valid, 
	// otherwise exit.
	if (accept)
		{
		timeStr[len++] = chr;
		timeStr[len] = 0;
		}
	else
		return (false);



	// Translate the written character into a time.
	for (i = 0; i < len; i++)
		{
		// Have we reached the end of the numeric characters?
		if (! (timeStr[i] >= '0' && timeStr[i] <= '9'))
			break;

		num = timeStr[i] - '0';
		
		// The first characters is always an hours digit.
		if (i == 0)
			{
			hours = num;
			if (hours && hours < firstHour)
				hours += 12;
			minutes = 0;
			}

		// The second characters may be the second hours digit or the
		// first minutes digit.
		else if (i == 1)
			{
			if (hours == 0)
				{
				hours = num;
				if (hours && hours < firstHour)
					hours += 12;
				}
			else if ((hours == 1 || hours == 13) && (num <= 2))
				{
				hours = 10 + num;
				if (hours && hours < firstHour)
					hours += 12;
				}
			else
				minutes = num * 10;
			}

		// The third characters is	may be the first or second minutes digit		
		else if (i == 2)
			{
			if ((hours > 9 && hours < 13) || hours > 21)
				minutes = num * 10;
			else if (*timeStr == '0')
				minutes = num * 10;
			else
				minutes += num;
			}
			
		// The forth characters is always the second minutes digit..
		else if (i == 3)
			minutes += num;
		}
		
	if (am && hours > 12)
		hours -= 12;
	
	if (pm && hours < 12)
		hours += 12;
		
	// Return the translated time.
	time->hours = hours;
	time->minutes = minutes;
	
	return (true);
	}


/***********************************************************************
 *
 * FUNCTION:    Translate24HourTime
 *
 * DESCRIPTION: This routine translated a series of character into a 
 *              24 hour times.
 *
 * PARAMETERS:  chr         - character written
 *              firstHour   - first valid hour
 *              timeStr     - passed: previously written character
 *                            returned: written character, including char passed
 *					 time			 - passed: previously translated time
 *                            returned: the translated time
 *
 * RETURNED:	 true if sucessful
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/2/96	Initial Revision
 *
 ***********************************************************************/
static Boolean Translate24HourTime (Char chr, Int16 /* firstHour */, 
	Char * timeStr, TimePtr time)
	{
	UInt16 i;
	Int16 len;
	UInt8 num;
	UInt8 hours, minutes;
	Boolean accept = false;
	
	
	len = StrLen (timeStr);
	
	// Was a digit written?
	if (chr >= '0' && chr <= '9')
		{
		// The first character can be 0 - 9.
		if (len == 0)
			accept = true;

		// The second character can be:
		//		0 - 9  if the first character is a 0.
		//		0 - 9  if the first character is a 1.
		//		0 - 6  if the first character is greater than 1 (first minute 
		//           	digit).
		else if (len == 1 && ((*timeStr <= '1' || chr < '6')))
			accept = true;

		// The third character can be:
		//		0 - 6   if the first two character are between 00 - 23 (first 
		//            		minute digit).
		//		0 or 5  if the first character is greater than 2 (second munute digit). 
		else if (len == 2 && (
						(*timeStr < '2' && chr < '6') ||
						(*timeStr == '2' && timeStr[1] < '4' && chr < '6') ||
						(chr == '0' || chr == '5')))
			accept = true;

		// The forth character can be:
		//		0 or 5  if the first two character are between 00 - 23
		//						(second minute digit).
		else if (len == 3 && 
					(chr == '0' || chr == '5') && 
					(timeStr[0] < '2' || (timeStr[0] == '2' && timeStr[1] < '4')))
			accept = true;
		}
			

	
	// Add the character passed to the time string if it is valid, 
	// otherwise exit.
	if (accept)
		{
		timeStr[len++] = chr;
		timeStr[len] = 0;
		}
	else
		return (false);



	// Translate the written character into a time.
	for (i = 0; i < len; i++)
		{
		num = timeStr[i] - '0';
		
		// The first characters is always an hours digit.
		if (i == 0)
			{
			hours = num;
			minutes = 0;
			}

		// The second characters may be the second hours digit or the
		// first minutes digit.
		else if (i == 1)
			{
			if (hours == 0)
				{
				hours = num;
				}
			else if (hours < 2 || (hours == 2 && (num < 4)))
				{
				hours = (hours * 10) + num;
				}
			else
				minutes = num * 10;
			}

		// The third characters is	may be the first or second minutes digit		
		else if (i == 2)
			{
			if ((hours > 9) || (*timeStr == '0'))
				minutes = num * 10;
			else
				minutes += num;
			}
			
		// The forth characters is always the second minutes digit..
		else if (i == 3)
			minutes += num;
		}
		
		
	// Return the translated time.
	time->hours = hours;
	time->minutes = minutes;
	
	return (true);
	}


/***********************************************************************
 *
 * FUNCTION:    TranslateTime
 *
 * DESCRIPTION: This routine translated a series of character into a 
 *              times.
 *
 * PARAMETERS:  timeFormat  - time format
 *              chr         - character written
 *              firstHour   - first valid hour
 *              timeStr     - passed: previously written character
 *                            returned: written character, including char passed
 *					 time			 - passed: previously translated time
 *                            returned: the translated time
 *
 * RETURNED:	 true if sucessful
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/2/96	Initial Revision
 *
 ***********************************************************************/
static Boolean TranslateTime (TimeFormatType timeFormat, Char chr, 
	Int16 firstHour, Char * timeStr, TimePtr time)
{
	if (timeFormat == tfColon24h 	|| 
		 timeFormat == tfDot24h 	||
		 timeFormat == tfHours24h)
		return (Translate24HourTime (chr, firstHour, timeStr, time));
	else
		return (Translate12HourTime (chr, firstHour, timeStr, time));
}


/***********************************************************************
 *
 * FUNCTION:    SetTimeTriggers
 *
 * DESCRIPTION: This routine sets the text label of the start time and
 *              end time triggers.
 *
 * PARAMETERS:  startTime	    - pointer to TimeType
 *              endTime        - pointer to TimeType
 *              startTimeText  - buffer that holds start time string
 *              emdTimeText    - buffer that holds end time string
 *              timeFormat     - time format
 *              untimed  	    - true if there isn't a time.
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/4/96	Initial Revision
 *
 ***********************************************************************/
static void SetTimeTriggers (TimeType startTime, TimeType endTime,
		Char * startTimeText, Char * endTimeText, 
		TimeFormatType timeFormat, Boolean untimed)
	{

	FormType *		frm;
	ControlPtr 	startTimeCtl, endTimeCtl;

	frm = FrmGetActiveForm ();
	startTimeCtl = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, TimeSelectorStartTimeButton));
	endTimeCtl = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, TimeSelectorEndTimeButton));

	if (! untimed)
		{
		TimeToAscii (startTime.hours, startTime.minutes, timeFormat, startTimeText);
		TimeToAscii (endTime.hours, endTime.minutes, timeFormat, endTimeText);
		}
	else
		{
		// copy two spaces into these fields instead of just a null
		// because controls with empty strings (or one space) cause old-style
		// graphic control behavior, which uses the wrong colors!
		StrCopy(startTimeText, "  ");
		StrCopy(endTimeText, "  ");
		}
		
	CtlSetLabel (startTimeCtl, startTimeText);
	CtlSetLabel(endTimeCtl, endTimeText);
	}


/***********************************************************************
 *
 * FUNCTION:    AdjustTimes
 *
 * DESCRIPTION: This routine adjusts the start or end times.  It's
 *              called when ever the start or end time are changed. If
 *              the start time has been changed then the end time is adjust
 *              such that the elasped time is maintained.  If the end
 *              time has been changed then the start time is adjusted so
 *              that it will not be before the end time.
 *
 * PARAMETERS:  startTime	    - pointer to TimeType
 *              endTime        - pointer to TimeType
 *              startTimeText  - buffer that holds start time string
 *              emdTimeText    - buffer that holds end time string
 *              timeFormat     - time format
 *              untimed  	    - true if there isn't a time.
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	4/4/96	Initial Revision
 *
 ***********************************************************************/
static Boolean AdjustTimes (TimePtr startTime, TimePtr endTime, 
		TimePtr timeDiff, ChangingTimeType changingTime)
	{
	Boolean changed = false;

	if (changingTime == changingStartTime)
		{
		// The start time has changed so update the end time.
		endTime->minutes = startTime->minutes + timeDiff->minutes;
		endTime->hours = startTime->hours + timeDiff->hours;

		// If there are 60 or more minutes wrap to the next hour
		if (endTime->minutes >= hoursInMinutes)
			{
			endTime->minutes -= hoursInMinutes;
			endTime->hours++;
			}

		// Don't allow the end time past 11:55 PM.
		//if (endTime->hours >= 24)
		//	{
		//	endTime->minutes = 55;
		//	endTime->hours = 23;
		//	}
		changed = true;
		}

	else if (changingTime == changingEndTime)
		{
		// The end time has changed so make sure it's valid
		// and calculate the new difference.
		TimeDifference (endTime, startTime, timeDiff);
		if (endTime->hours < startTime->hours ||
			(endTime->hours == startTime->hours && 
			endTime->minutes < startTime->minutes))
			{
			timeDiff->hours = 0;
			timeDiff->minutes = 0;
			endTime->hours = startTime->hours;
			endTime->minutes = startTime->minutes;	
			changed = true;
			}
		}
	// Don't allow the end time past 11:55 PM.
	if (endTime->hours >= 24)
		{
		endTime->minutes = 55;
		endTime->hours = 23;
		changed = true;
		}
	return (changed);
	}


/***********************************************************************
 *
 * FUNCTION:    SelectTime
 *
 * DESCRIPTION: Display a form showing a start and end time.
 *					 Allow the user to change the time and then
 *              return them.
 *					 pTimeDiff.
 *
 * PARAMETERS:  pStartTime	- pointer to TimeType
 *              pEndTime   - pointer to TimeType
 *              untimed  	- true if there isn't a time.
 *              title	   - string containing the title
 *              startOfDay - used when "All Day" button selected.
 *					 endOfDay	- our used when "All Day" button selected.
 *              startOfDisplay - first hour initially visible
 *
 * RETURNED:	 True if the time was changed by the user.
 * 				 The first three parameters may change.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	12/2/94	Initial Revision
 *			trev	08/12/97	made non modified passed variables constant
 *			css	06/08/99	added new parameter & "All Day" button for gromit change.
 *
 ***********************************************************************/
Boolean SelectTime (TimeType * startTimeP, TimeType * endTimeP, 
	Boolean untimed, const Char * titleP, Int16 startOfDay, Int16 endOfDay, 
	Int16 startOfDisplay)
{
	Int16							firstHour;
	Char 							startTimeText [timeStringLength];
	Char 							endTimeText [timeStringLength];
	Char							timeChars [timeStringLength];
	TimePtr						timeP;
	FormType 					*originalForm, *frm;
	ListPtr 						hoursLst, minutesLst;
	ControlPtr 					startTimeCtl, endTimeCtl;
	EventType 					event;
	Boolean 						confirmed = false;
	MemHandle 						hoursItems;
	TimeType 					startTime, endTime, timeDiff;
	TimeFormatType 			timeFormat;
	ChangingTimeType			changingTime;

	// Get the time format from the system preerances;
	timeFormat = (TimeFormatType)PrefGetPreference(prefTimeFormat);


	// Because this routine only deals with minutes in five minute
	// intervals we convert the proposed times from those passed.
	startTime.hours = startTimeP->hours;
	startTime.minutes = startTimeP->minutes;
	endTime.hours = endTimeP->hours;
	endTime.minutes = endTimeP->minutes;
	TimeDifference (&endTime, &startTime, &timeDiff);
	
	// Make sure the end time is displayable (clips at 11:55 pm)
	AdjustTimes (&startTime, &endTime, &timeDiff, changingStartTime);
	
	// Clear the buffer that holds written characters.
	*timeChars = 0;

	startOfDisplay = min (startOfDisplay, 12);

	originalForm = FrmGetActiveForm();
	frm = (FormType *) FrmInitForm (TimeSelectorForm);
	FrmSetActiveForm (frm);

	hoursLst = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, TimeSelectorHourList));
	minutesLst = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, TimeSelectorMinuteList));
	startTimeCtl = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, TimeSelectorStartTimeButton));
	endTimeCtl = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, TimeSelectorEndTimeButton));


	// Set list to use either 12 or 24 hour time
	hoursItems = SysFormPointerArrayToStrings (
		((timeFormat == tfColon24h) || (timeFormat == tfDot24h)) ? 
			(Char *) Hours24Array() : (Char *) Hours12Array(), 24);
 
 	LstSetListChoices (hoursLst, MemHandleLock(hoursItems), 24);
	LstSetTopItem (hoursLst, startOfDisplay);
	//	Used to do LstMakeItemVisible (hoursLst, startTime.hours); no longer.


	if (! untimed)
		{
		LstSetSelection (hoursLst, startTime.hours);
		LstSetSelection (minutesLst, startTime.minutes / 5);

		CtlSetValue (startTimeCtl, true);
		changingTime = changingStartTime;
		}
	else
		{
		// The hour list is dynamically created and doesn't have a selection
		LstSetSelection (minutesLst, noListSelection);

		changingTime = changingNoTime;
		}


	// Set the start and end time buttons to the current times or blank them
	// if No Time is selected.
	SetTimeTriggers (startTime, endTime, startTimeText, endTimeText, 
		timeFormat, untimed);
		

	// This needs to be taken out when SelectTimeV33 goes away.  It allows for backward 
	// compatibility for this change of adding the end of day variable as a parameter.
	if (endOfDay != noDisplayOfAllDay)
	{
	  FrmShowObject (frm, FrmGetObjectIndex (frm, TimeSelectorAllDayButton));
	}
	
	FrmSetTitle (frm, (Char *) titleP);
	FrmDrawForm (frm);
	
	
	while (true)
		{
		EvtGetEvent (&event, evtWaitForever);

		if (SysHandleEvent ((EventType *)&event))
			continue;
			
		if (event.eType == appStopEvent)
			{
			// Cancel the dialog and repost this event for the app
			EvtAddEventToQueue(&event);
			confirmed = false;
			break;
			}
			
		// Handle these before the form does to overide the default behavior
		if (changingTime == changingNoTime &&
			event.eType == lstEnterEvent && 
			event.data.lstEnter.listID == TimeSelectorMinuteList)
			{
			SndPlaySystemSound(sndError);
			continue;
			}

		FrmHandleEvent (frm, &event);


		// If the start or end time buttons are pressed then change
		// the time displayed in the lists.
		if (event.eType == ctlSelectEvent)
			{
			// "Ok" button pressed?
			if (event.data.ctlSelect.controlID == TimeSelectorOKButton)
				{
				confirmed = true;
				}

			// "Cancel" button pressed?
			else if (event.data.ctlSelect.controlID == TimeSelectorCancelButton)
				{
				break;
				}

			// Start time button pressed?
			else if (event.data.ctlSelect.controlID == TimeSelectorStartTimeButton)
				{
				if (changingTime != changingStartTime)
					{
					if (changingTime == changingNoTime)
						{
						SetTimeTriggers (startTime, endTime, startTimeText, 
							endTimeText, timeFormat, false);
						}

					CtlSetValue (endTimeCtl, false);
					LstSetSelection (hoursLst, startTime.hours);
					LstSetSelection (minutesLst, startTime.minutes / 5);
					changingTime = changingStartTime;
					}
				else
					CtlSetValue(startTimeCtl, true);
				}

			// End time button pressed?
			else if (event.data.ctlSelect.controlID == TimeSelectorEndTimeButton)
				{
				if (changingTime != changingEndTime)
					{
					if (changingTime == changingNoTime)
						{
						SetTimeTriggers (startTime, endTime, startTimeText, 
							endTimeText, timeFormat, false);
						}

					CtlSetValue(startTimeCtl, false);
					LstSetSelection (hoursLst, endTime.hours);
					LstSetSelection (minutesLst, endTime.minutes / 5);
					changingTime = changingEndTime;
					}
				else
					CtlSetValue(endTimeCtl, true);
				}

			// No time button pressed?
			else if (event.data.ctlSelect.controlID == TimeSelectorNoTimeButton)
				{
				if (changingTime != changingNoTime)
					{
					if (changingTime == changingStartTime)
						CtlSetValue(startTimeCtl, false);
					else
						CtlSetValue(endTimeCtl, false);
					SetTimeTriggers (startTime, endTime, startTimeText, 
						endTimeText, timeFormat, true);
					
					LstSetSelection (hoursLst, noListSelection);
					LstSetSelection (minutesLst, noListSelection);
					changingTime = changingNoTime;
					}
					// Get us out of this form display now.
					confirmed = true;
				}

			// All day button pressed?
			else if (event.data.ctlSelect.controlID == TimeSelectorAllDayButton)
				{
				if (changingTime != changingNoTime)
					{
					if (changingTime == changingStartTime)
						CtlSetValue(startTimeCtl, false);
					else
						CtlSetValue(endTimeCtl, false);
					
					LstSetSelection (hoursLst, noListSelection);
					LstSetSelection (minutesLst, noListSelection);
					changingTime = changingNoTime;
					}
											
					// No matter what, the minutes are 0 for both because only the hour is registered for start/end
					// times.
					startTime.minutes = 0;
					endTime.minutes 	= 0;
					startTime.hours 	= startOfDay;
					endTime.hours 		= endOfDay;
					
					// Set the times to the new times.
					SetTimeTriggers (startTime, endTime, startTimeText, 
						endTimeText, timeFormat, true);
					// Get us out of this form display now.  First set the changing time to anything but the changingNoTime value
					// so that the pointers at the end of this function get set correctly.
					changingTime = changingStartTime;
					confirmed = true;
				}

			// Clear the buffer that holds written characters.
			*timeChars = 0;
			}


		// If either list is changed then get the new time.  If the
		// start time is changed then change the end time so that the
		// the time difference remains the same.  If the end time is
		// changed then make sure the end time isn't before the start
		// time.  Also calculate a new time difference.
		else if (event.eType == lstSelectEvent)
			{
         // First, get the info from the list which has changed.
			if (event.data.lstSelect.listID == TimeSelectorHourList)
				{
				if (changingTime == changingStartTime)
               startTime.hours = (UInt8) LstGetSelection (hoursLst);
           	else if (changingTime == changingEndTime)
               endTime.hours = (UInt8) LstGetSelection (hoursLst);
           	else if (changingTime == changingNoTime)
           		{
               startTime.hours = (UInt8) LstGetSelection (hoursLst);
					SetTimeTriggers (startTime, endTime, startTimeText, 
						endTimeText, timeFormat, false);
					CtlSetValue (endTimeCtl, false);
					LstSetSelection (minutesLst, startTime.minutes / 5);
					changingTime = changingStartTime;
					}
				}
         else if (event.data.lstSelect.listID == TimeSelectorMinuteList)
				{
				if (changingTime == changingStartTime)
               startTime.minutes = (UInt8) LstGetSelection (minutesLst) * 5;
           	else if (changingTime == changingEndTime)
					endTime.minutes = (UInt8) LstGetSelection (minutesLst) * 5;
           	else if (changingTime == changingNoTime)
           		{
               ErrNonFatalDisplay("lstEnterEvent not being filtered.");
               }
				}


			if (AdjustTimes (&startTime, &endTime, &timeDiff, changingTime))
				{
				if (changingTime == changingStartTime)
					{
					TimeToAscii (startTime.hours, startTime.minutes, timeFormat, 
						startTimeText);
					CtlSetLabel (startTimeCtl, startTimeText);
					}
				else if (changingTime == changingEndTime)
					{
					LstSetSelection (hoursLst, startTime.hours);
					LstSetSelection (minutesLst, startTime.minutes / 5);
					}
				}
			TimeToAscii(endTime.hours, endTime.minutes, timeFormat, endTimeText);
			CtlSetLabel(endTimeCtl, endTimeText);


			// Clear the buffer that holds written characters.
			*timeChars = 0;
			}


		// Handle character written in the time picker.
		else if (event.eType == keyDownEvent)
			{
			if (changingTime == changingEndTime)
				{
				timeP = &endTime;
				firstHour = startTime.hours;
				}
			else
				{
				timeP = &startTime;
				firstHour = startOfDisplay;
				}
		
			// If a backspace character was written, change the time picker's
			// current setting to "no-time".
			if (event.data.keyDown.chr == backspaceChr)
				{
				*timeChars = 0;
				if (changingTime != changingNoTime)
					{
					if (changingTime == changingStartTime)
						CtlSetValue (startTimeCtl, false);
					else
						CtlSetValue (endTimeCtl, false);
					SetTimeTriggers (startTime, endTime, startTimeText, 
						endTimeText, timeFormat, true);
					LstSetSelection (hoursLst, noListSelection);
					LstSetSelection (minutesLst, noListSelection);
					changingTime = changingNoTime;
					}
				}

			// A linefeed character confirms the dialog box.
			else if (event.data.keyDown.chr == linefeedChr)
				{
				confirmed = true;
				}

			// If next-field character toggle between start time in end
			// time.
			else if (event.data.keyDown.chr == nextFieldChr)
				{
				*timeChars = 0;
				if (changingTime == changingStartTime)
					{
					CtlSetValue (startTimeCtl, false);
					CtlSetValue (endTimeCtl, true);
					changingTime = changingEndTime;
					}
				else
					{
					CtlSetValue (endTimeCtl, false);
					CtlSetValue (startTimeCtl, true);
					changingTime = changingStartTime;
					}
				}


			// If a valid time character was written, translate the written 
			// character into a time and update the time picker's UI.
			else if (TranslateTime (timeFormat, event.data.keyDown.chr, 
				firstHour, timeChars, timeP))
				{
				if (changingTime == changingNoTime)
					{
					changingTime = changingStartTime;
					CtlSetValue (startTimeCtl, true);
					}

				AdjustTimes (&startTime, &endTime, &timeDiff, changingTime);
				
				SetTimeTriggers (startTime, endTime, startTimeText, 
					endTimeText, timeFormat, false);

				LstSetSelection (hoursLst, timeP->hours);
				LstSetSelection (minutesLst, timeP->minutes / 5);
				}
			}

		// Has the dialog been confirmed.
		if (confirmed)
			{
			if (changingTime != changingNoTime)
				{
				*startTimeP = startTime;
				*endTimeP = endTime;
				}
			else
				{
				TimeToInt(*startTimeP) = noTime;
				TimeToInt(*endTimeP) = noTime;
				}
			break;
			}

		}

	FrmEraseForm (frm);
	FrmDeleteForm (frm);
	MemHandleFree (hoursItems);
	
	FrmSetActiveForm(originalForm);
	
	return (confirmed);
	}



/***********************************************************************
 *
 * FUNCTION:    SelectTimeV33
 *
 * DESCRIPTION: Display a form showing a start and end time.
 *					 Allow the user to change the time and then
 *              return them.
 *					 pTimeDiff.
 *
 * PARAMETERS:  pStartTime	- pointer to TimeType
 *              pEndTime   - pointer to TimeType
 *              untimed  	- true if there isn't a time.
 *              title	   - string containing the title
 *              startOfDay - first hour initially visible
 *
 * RETURNED:	 True if the time was changed by the user.
 * 				 The first three parameters may change.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	12/2/94	Initial Revision
 *			trev	08/12/97	made non modified passed variables constant
 *			css	06/08/99	Phasing out, put new one in its place above.
 *
 ***********************************************************************/
Boolean SelectTimeV33 (const TimePtr startTimeP, const TimePtr endTimeP, 
	const Boolean untimed, const Char * titleP, Int16 startOfDay)
	{
	
	return (SelectTime (startTimeP, endTimeP, untimed, titleP, startOfDay, noDisplayOfAllDay,startOfDay));
	}



/***********************************************************************
 *
 * FUNCTION:    SelectOneTime
 *
 * DESCRIPTION: Display a form showing a time and allow the user
 *              to select a different time. This is the single time
 *					 dialog as seen in general panel
 *
 * PARAMETERS:  hour	- pointer to hour to change
 					 minute - pointer to minute to change
 *				title	- string title for the dialog
 *
 * RETURNED:	 true if the OK button was pressed
 *					 if true the parameters passed are changed
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	12/2/94		Initial Revision in General Panel
 *			gavin	3/20/98		Extracted into separate system function
 *
 ***********************************************************************/
Boolean SelectOneTime(Int16 *hour,Int16 *minute, const Char * titleP)
{
	FormType * originalForm, * frm;
	EventType event;
	Boolean confirmed = false;
	Boolean handled = false;
	Boolean done = false;
	TimeFormatType timeFormat;			// Format to display time in
	Int16   curHour;
	UInt16	currentTimeButton;
	UInt8	hoursTimeButtonValue;
	Char	hoursTimeButtonString[3];
	UInt8	minuteTensButtonValue;
	Char	minuteTensButtonString[2];
	UInt8	minuteOnesButtonValue;
	Char	minuteOnesButtonString[2];
	Char	separatorString[3];

	timeFormat = (TimeFormatType)PrefGetPreference(prefTimeFormat);

	originalForm = FrmGetActiveForm();
	frm = (FormType *) FrmInitForm (SelectOneTimeDialog); 
	if (titleP)
		FrmSetTitle (frm, (Char *) titleP);

	FrmSetActiveForm (frm);
	
	curHour = *hour;
	
	if (Use24HourFormat(timeFormat))
		{
		// Hide the AM & PM ui
		FrmHideObject (frm, FrmGetObjectIndex (frm, timeAMCheckbox));
		FrmHideObject (frm, FrmGetObjectIndex (frm, timePMCheckbox));
		}
	else 
		{
		if (curHour < 12)
			CtlSetValue(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, timeAMCheckbox)), true);
		else
			{
			CtlSetValue(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, timePMCheckbox)), true);
			curHour -= 12;
			}
		
		if (curHour == 0)
			curHour = 12;
		}

	// Set the time seperator to the system defined one
	separatorString[0] = TimeSeparator(timeFormat);
	separatorString[1] = '\0';
	FrmCopyLabel(frm, timeSeparatorLabel, separatorString);


	// Now set the time displayed in the push buttons.
	hoursTimeButtonValue = curHour;
	StrIToA(hoursTimeButtonString, hoursTimeButtonValue);
	CtlSetLabel(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, timeHourButton)),
		hoursTimeButtonString);
		
	minuteTensButtonValue = *minute / 10;
	StrIToA(minuteTensButtonString, minuteTensButtonValue);
	CtlSetLabel(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, timeMinutesTensButton)),
		minuteTensButtonString);
		
	minuteOnesButtonValue = *minute % 10;
	StrIToA(minuteOnesButtonString, minuteOnesButtonValue);
	CtlSetLabel(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, timeMinutesOnesButton)),
		minuteOnesButtonString);


	// Set the hour time button to be the one set by the arrows
	currentTimeButton = defaultTimeButton;
	CtlSetValue(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, defaultTimeButton)), true);

	
	FrmDrawForm (frm);

	
	while (!done)
		{
		EvtGetEvent (&event, evtWaitForever);

		if (! SysHandleEvent ((EventType *)&event))	
			FrmHandleEvent (frm,&event); 

		if (event.eType == ctlSelectEvent)
			{
			switch (event.data.ctlSelect.controlID)
				{
				case timeOkButton:
					frm = FrmGetActiveForm();

					// Set the new time (seconds are cleared).
					if (Use24HourFormat(timeFormat))
						*hour = hoursTimeButtonValue;
					else
						{
						*hour = hoursTimeButtonValue % 12 + // 12am is 0 hours!
							(CtlGetValue(FrmGetObjectPtr (frm,
																	FrmGetObjectIndex (frm,
																					timePMCheckbox)))
							? 12 : 0);
						}

					*minute = minuteTensButtonValue * 10 + minuteOnesButtonValue;

					done = true;
					confirmed = true;
					break;

				case timeCancelButton:
					done = true;
					break;
					
				case timeDecreaseButton:
				case timeIncreaseButton:
					frm = FrmGetActiveForm();
					switch (currentTimeButton)
						{
						// MemHandle increasing and decreasing the time for each time digit
						case timeHourButton:
							if (event.data.ctlSelect.controlID == timeDecreaseButton)
								{
								if (Use24HourFormat(timeFormat))
									if (hoursTimeButtonValue > 0)
										hoursTimeButtonValue--;
									else
										hoursTimeButtonValue = 23;
								else
									if (hoursTimeButtonValue > 1)
										hoursTimeButtonValue--;
									else
										hoursTimeButtonValue = 12;
								}
							else
								{
								if (Use24HourFormat(timeFormat))
									if (hoursTimeButtonValue < 23)
										hoursTimeButtonValue++;
									else
										hoursTimeButtonValue = 0;
								else
									if (hoursTimeButtonValue < 12)
										hoursTimeButtonValue++;
									else
										hoursTimeButtonValue = 1;
								}
								
							StrIToA(hoursTimeButtonString, hoursTimeButtonValue);
							CtlSetLabel(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, 
								timeHourButton)),	hoursTimeButtonString);
							break;

						case timeMinutesTensButton:
							if (event.data.ctlSelect.controlID == timeDecreaseButton)
								{
								if (minuteTensButtonValue > 0)
									minuteTensButtonValue--;
								else
									minuteTensButtonValue = 5;
								}
							else
								{
								if (minuteTensButtonValue < 5)
									minuteTensButtonValue++;
								else
									minuteTensButtonValue = 0;
								}

							StrIToA(minuteTensButtonString, minuteTensButtonValue);
							CtlSetLabel(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, 
								timeMinutesTensButton)), minuteTensButtonString);
							break;
							
						case timeMinutesOnesButton:
							if (event.data.ctlSelect.controlID == timeDecreaseButton)
								{
								if (minuteOnesButtonValue > 0)
									minuteOnesButtonValue--;
								else
									minuteOnesButtonValue = 9;
								}
							else
								{
								if (minuteOnesButtonValue < 9)
									minuteOnesButtonValue++;
								else
									minuteOnesButtonValue = 0;
								}

							StrIToA(minuteOnesButtonString, minuteOnesButtonValue);
							CtlSetLabel(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, 
								timeMinutesOnesButton)), minuteOnesButtonString);
							break;
						}

					handled = true;
					break;	// timeDecreaseButton & timeIncreaseButton
		
				case timeHourButton:
					currentTimeButton = timeHourButton;
					break;
					
				case timeMinutesTensButton:
					currentTimeButton = timeMinutesTensButton;
					break;
					
				case timeMinutesOnesButton:
					currentTimeButton = timeMinutesOnesButton;
					break;
					
				}
			}

		else if (event.eType == appStopEvent)
			{
			EvtAddEventToQueue (&event);
			done = true;
			break;
			}
			
		}	// end while true
		
	FrmEraseForm (frm);
	FrmDeleteForm (frm);
	
	FrmSetActiveForm(originalForm);
	
	return (confirmed);
}
