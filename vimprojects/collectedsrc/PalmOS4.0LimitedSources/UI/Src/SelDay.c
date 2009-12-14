/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: SelDay.c
 *
 * Release: 
 *
 * Description:
 *	  This file contain the day selection routines.
 *
 * History:
 *		12/02/94	rsf	Created by Roger Flores
 *
 *****************************************************************************/

#include <PalmTypes.h>
#include <SystemPublic.h>

#include "Day.h"
#include "Form.h"
#include "SelDay.h"
#include "Event.h"
#include "UIResourcesPrv.h"

#define monthGroup	1

/***********************************************************************
 *
 * FUNCTION:    SelectDayHandleEvent
 *
 * DESCRIPTION: This routine handles events in the date picker.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	12/2/94	Initial Revision
 *       art   10/17/95 Add support for update messages
 *			kcr	11/13/95	Converted prior/next year buttons to repeating buttons
 *			roger	4/26/96	MemHandle selecting a month when selectByMonth
 *
 ***********************************************************************/
static Boolean SelectDayHandleEvent (EventType * event)
	{
	char yearString[5];
	UInt16 index;
	FormType * frm;
	Boolean handled = true;
	Boolean redrawYear = false;
	DaySelectorPtr daySelectorInfo;

	
	frm = FrmGetActiveForm ();
	index = FrmGetObjectIndex (frm, DateSelectorDayGadget);
	daySelectorInfo = FrmGetGadgetData (frm, index);
	
	DayHandleEvent (daySelectorInfo, event);

	if (event->eType == frmUpdateEvent)
		{
		FrmDrawForm (frm);
		DayDrawDays(daySelectorInfo);
		}

	else if (event->eType == ctlSelectEvent)
		{
		if (event->data.ctlSelect.controlID >= january &&
			event->data.ctlSelect.controlID <= december)
			{
			daySelectorInfo->visibleMonth = (Int16) event->data.ctlSelect.controlID;
			FrmSetControlGroupSelection (frm, monthGroup, daySelectorInfo->visibleMonth);
			DayDrawDays(daySelectorInfo);
			}
		}
		
	else if (event->eType == ctlRepeatEvent)
		{
		if (event->data.ctlRepeat.controlID == DateSelectorPriorYearButton)
			{
			if (daySelectorInfo->visibleYear > daySelectorMinYear)
				{
				daySelectorInfo->visibleYear--;
				DayDrawDays(daySelectorInfo);
				redrawYear = true;
				}
			}
		else if (event->data.ctlRepeat.controlID == DateSelectorNextYearButton)
			{
			if (daySelectorInfo->visibleYear < daySelectorMaxYear)
				{
				daySelectorInfo->visibleYear++;
				DayDrawDays(daySelectorInfo);
				redrawYear = true;
				}
			}
		handled = false;		// let the ctlRepeat regain control
		}


	// If the user went to the next or prior month we may need to
	// update the day, month, and year areas.
	else if (event->eType == daySelectEvent)
		{
		if (daySelectorInfo->visibleMonth != daySelectorInfo->selected.month)
			{
			daySelectorInfo->visibleMonth = daySelectorInfo->selected.month;
			FrmSetControlGroupSelection (frm, monthGroup, daySelectorInfo->selected.month);
			DayDrawDays(daySelectorInfo);
			}
		if (daySelectorInfo->visibleYear != daySelectorInfo->selected.year)
			{
			daySelectorInfo->visibleYear = daySelectorInfo->selected.year;
			DayDrawDays(daySelectorInfo);
			redrawYear = true;
			}
		}

	else if	(	(event->eType == keyDownEvent)
				&&	(!TxtCharIsHardKey(	event->data.keyDown.modifiers,
												event->data.keyDown.chr))
				&&	(EvtKeydownIsVirtual(event)))
		{		
		switch (event->data.keyDown.chr)
			{
			case vchrPageUp:
				if (daySelectorInfo->visibleMonth == january)
					{
					if (daySelectorInfo->visibleYear > daySelectorMinYear)
						{
						daySelectorInfo->visibleYear--;
						daySelectorInfo->visibleMonth = december;
						FrmSetControlGroupSelection (frm, monthGroup, daySelectorInfo->visibleMonth);
						DayDrawDays(daySelectorInfo);
						redrawYear = true;
						}
					}
				else
					{
					daySelectorInfo->visibleMonth--;
					FrmSetControlGroupSelection (frm, monthGroup, daySelectorInfo->visibleMonth);
					DayDrawDays(daySelectorInfo);
					redrawYear = true;
					}
				handled = true;
				break;
				
			case vchrPageDown:
				if (daySelectorInfo->visibleMonth == december)
					{
					if (daySelectorInfo->visibleYear < daySelectorMaxYear)
						{
						daySelectorInfo->visibleYear++;
						daySelectorInfo->visibleMonth = january;
						FrmSetControlGroupSelection (frm, monthGroup, daySelectorInfo->visibleMonth);
						DayDrawDays(daySelectorInfo);
						redrawYear = true;
						}
					}
				else
					{
					daySelectorInfo->visibleMonth++;
					FrmSetControlGroupSelection (frm, monthGroup, daySelectorInfo->visibleMonth);
					DayDrawDays(daySelectorInfo);
					redrawYear = true;
					}
				handled = true;
				break;

			}
		}

	else 
		handled = false;

		
	if (redrawYear)
		{
		// Change year label
		StrIToA(yearString, daySelectorInfo->visibleYear);
		FrmCopyLabel(frm, DateSelectorYearLabel, yearString);
		}

	return (handled);
	}


/***********************************************************************
 *
 * FUNCTION:    SelectDay
 *
 * DESCRIPTION: Display a form showing a date and allow the user
 *              to select a different date.
 *
 * PARAMETERS:  month	- month selected
 *					 day		- day selected
 *					 year		- year selected
 *					 title	- string title for the dialog
 *
 * RETURNED:	 true if the OK button was pressed
 *					 if true the parameters passed are changed
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	12/02/94	Initial Revision
 *       art   10/17/95 Revised to support for update messages
 *       roger 04/26/96	Now also supports selection by week and month
 *			trev	08/12/97	made non modified passed variables constant
 *			grant	02/16/99	"This Week" in the week-picker now takes the weekStartDay
 *								preference into account.
 *			kwk	11/06/00	Catch negative invalid month & day parameters, and use
 *								DaysInMonth for max day parameter value.
 *			vivek 11/17/00 Backed out earlier change and changed the height to 
 *								70 (only the first row of the gadget needs to have
 *								line height of 11  and this is handled while drawing).
 ***********************************************************************/
Boolean SelectDay (SelectDayType selectDayBy, Int16 *month, Int16 *day, 
	Int16 *year, const Char * title)
{
	char yearString[5];
	FormType *originalForm, *frm;
	EventType event;
	Boolean confirmed = false;
	DateTimeType today;
	DaySelectorType daySelectorInfo;
	UInt16 buttonToUse;
	UInt8 weekStartDay;
	Int16 selectedDayOfWeek;
	Int16 todayDayOfWeek;


	ErrNonFatalDisplayIf ((*year < firstYear) || (*year > lastYear), "Invalid year");
	ErrNonFatalDisplayIf ((*month < january) || (*month > december), "Invalid month");
	ErrNonFatalDisplayIf ((*day < 1) || (*day > DaysInMonth(*month, *year)), "Invalid day");
	ErrNonFatalDisplayIf (title == NULL, "Invalid title");
	
	daySelectorInfo.selectDayBy = selectDayBy;

	daySelectorInfo.selected.month = *month;
	daySelectorInfo.selected.day = *day;
	daySelectorInfo.selected.year = *year;
	daySelectorInfo.visibleMonth = *month;
	daySelectorInfo.visibleYear = *year;


	// init the time info to 0 (no time)
	daySelectorInfo.selected.second = 0;
	daySelectorInfo.selected.minute = 0;
	daySelectorInfo.selected.hour = 0;

	originalForm = FrmGetActiveForm();
	frm = (FormType *) FrmInitForm (DateSelectorForm);
	FrmSetTitle (frm, (Char *) title);

	StrIToA(yearString, daySelectorInfo.selected.year);
	FrmCopyLabel(frm, DateSelectorYearLabel, yearString);

	FrmSetControlGroupSelection (frm, monthGroup, daySelectorInfo.selected.month);
	
	
	// Enable either the Today, This Week, or This Month button
	switch (selectDayBy)
		{
		case selectDayByDay:
			buttonToUse = DateSelectorTodayButton;
			break;
		
		case selectDayByWeek:
			buttonToUse = DateSelectorThisWeekButton;
			break;
		
		case selectDayByMonth:
			buttonToUse = DateSelectorThisMonthButton;
			break;
		
		default:
			ErrNonFatalDisplay ("Invalid SelectDayType");
			break;
		}
	FrmShowObject(frm, FrmGetObjectIndex(frm, buttonToUse));
	

	FrmSetActiveForm (frm);
	FrmDrawForm (frm);

	RctSetRectangle (&(daySelectorInfo.bounds), 7, 63, 140, 70);
	TimSecondsToDateTime(TimGetSeconds(), &today);
	daySelectorInfo.visible = true;
	DayDrawDaySelector (&daySelectorInfo);

	FrmSetGadgetData (frm, 
		FrmGetObjectIndex (frm, DateSelectorDayGadget), &daySelectorInfo);

	FrmSetEventHandler (frm, SelectDayHandleEvent);
	
	while (true)
		{
		EvtGetEvent (&event, evtWaitForever);

		if (! SysHandleEvent ((EventType *)&event))	
			FrmDispatchEvent (&event); 


		if (event.eType == ctlSelectEvent)
			{
			if (selectDayBy == selectDayByMonth &&
				event.data.ctlSelect.controlID >= january &&
				event.data.ctlSelect.controlID <= december)
				{
				// Return with the user's choice of month.
				*month = daySelectorInfo.visibleMonth;
				*year = daySelectorInfo.visibleYear;
				
				// Make sure that the day is within the month (some months are
				// shorter than others).
				today.day = DaysInMonth(today.month, today.year);
				if (*day > today.day)
					*day = today.day;
				confirmed = true;
				break;
				}
			
			if (event.data.ctlSelect.controlID == DateSelectorCancelButton)
				break;

			// Today
			else if (event.data.ctlSelect.controlID == DateSelectorTodayButton)
				{
				// The Today button now exits the dialog.
				*month = today.month;
				*day = today.day;
				*year = today.year;
				confirmed = true;
				break;
				}

			// This week
			else if (event.data.ctlSelect.controlID == DateSelectorThisWeekButton)
				{
				// Tapping this button should return a day from the current week.
				// The day returned is the same day of the week as the selected day.
				// Just returning today is easier, but it loses the context of the 
				// original day.
				
				// Take the week start day into account, otherwise we may end up
				// returning the wrong week.
				// Here's an example: suppose the weekStartDay is monday, today is a
				// tuesday, and the selected day is a sunday.  Then
				//    selectedDayOfWeek - todayDayOfWeek = 0 - 2 = -2.
				// So we end up with the day two days before today, which is the
				// sunday belonging to the previous week.  We really want the sunday
				// a week later.
				weekStartDay = (UInt8) PrefGetPreference(prefWeekStartDay);
				selectedDayOfWeek = (Int16) DayOfWeek(*month, *day, *year);
				todayDayOfWeek = (Int16) DayOfWeek(today.month, today.day, today.year);
				
				// Put both days of the week into the range [weekStartDay,weekStartDay+6].
				if (selectedDayOfWeek < weekStartDay)
					{
					selectedDayOfWeek += daysInWeek;
					}
				if (todayDayOfWeek < weekStartDay)
					{
					todayDayOfWeek += daysInWeek;
					}
				
				TimAdjust(&today, (selectedDayOfWeek - todayDayOfWeek) * daysInSeconds);
				
				// check that the new day is on the same day of week as the selected day
				ErrNonFatalDisplayIf((Int16) DayOfWeek(today.month, today.day, today.year)
											!= (Int16) DayOfWeek(*month, *day, *year),
											"new day is on wrong day of week");
				
				// The This Week button exits the dialog.
				*month = today.month;
				*day = today.day;
				*year = today.year;
				
				confirmed = true;
				break;
				}

			// This month
			else if (event.data.ctlSelect.controlID == DateSelectorThisMonthButton)
				{
				// The This Month button exits the dialog.
				*month = today.month;
				*year = today.year;
				
				// Make sure that the day is within the month (some months are
				// shorter than others).
				today.day = DaysInMonth(today.month, today.year);
				if (*day > today.day)
					*day = today.day;
				confirmed = true;
				break;
				}
			}

		else if (event.eType == daySelectEvent)
			{
			// We have a selection.  Now return it.
			if (event.data.daySelect.useThisDate)
				{
				*month = daySelectorInfo.selected.month;
				*day = daySelectorInfo.selected.day;
				*year = daySelectorInfo.selected.year;
				confirmed = true;
				break;
				}
			}
			
		else if (event.eType == appStopEvent)
			{
			EvtAddEventToQueue (&event);
			break;
			}
		}

	FrmEraseForm (frm);
	FrmDeleteForm (frm);
	
	FrmSetActiveForm(originalForm);

	return confirmed;
}


/***********************************************************************
 *
 * FUNCTION:    SelectDayV10
 *
 * DESCRIPTION: Display a form showing a date and allow the user
 *              to select a different date.
 *
 * THIS IS AN OUT OF DATE ROUTINE WHICH WILL DISSAPPEAR.
 *
 * PARAMETERS:  month	- month selected
 *					 day		- day selected
 *					 year		- year selected
 *					 title	- string title for the dialog
 *
 * RETURNED:	 true if the OK button was pressed
 *					 if true the parameters passed are changed
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	12/2/94	Initial Revision
 *       art   10/17/95 Revised to support for update messages
 *       roger 4/26/96	Revised to call new routine
 *			trev	08/12/97	made non modified passed variables constant
 *
 ***********************************************************************/
Boolean SelectDayV10 (Int16 *month, Int16 *day, Int16 *year, const Char * title)
{
	return SelectDay (selectDayByDay, month, day, year, title);
}
