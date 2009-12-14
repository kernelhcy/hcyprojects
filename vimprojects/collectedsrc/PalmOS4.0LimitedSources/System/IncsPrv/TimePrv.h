/******************************************************************************
 *
 * Copyright (c) 1995 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: TimePrv.h
 *
 * Release: 
 *
 * Description:
 *		Private Time manager functions
 *
 * History:
 *   	1/19/95  roger - Created by Roger Flores
 *
 *****************************************************************************/

#ifdef	NON_PORTABLE

#ifndef __TIMEPRV_H__
#define __TIMEPRV_H__


/*******************************************************************
 * Private Time Manager Constants
 *
 *******************************************************************/

// Alarm repeat interval -- once an alarm is triggered, it will
// repeat every timAlarmRepeatSeconds until it is canceled or
// replaced with a new alarm by the Alarm Manager.  This ensures
// that an alarm will not be missed by the Alarm Manager code which
// is executed on some app's UI thread.
#define timAlarmRepeatSeconds		10

// Alarm delta interval -- if the difference between the alarm time and the current time
// is less than this interval, the alarm will be triggered immediately.  This eliminates
// a race condition which occurs when the alarm time is TOO close to the current time.
#define timAlarmMinDeltaSeconds	2

// In the 3.3 ROM it was possible to create the Saved Preferences database before the time
// globals were initialized.  This would result in a value being referenced off of NULL.
// If the globals are not present then TimGetSeconds should report back this dummy value.
// This number equates to 6/25/74 12:01am.
#define timNoTimeGlobalsDummyValue	0x84928680


/*******************************************************************
 * Time Manager Globals
 *
 *******************************************************************/
typedef struct TimGlobalsType {

	UInt32			rtcHours;				// Added to DragonBall hours to get actual time
	UInt32			alarmSeconds;			// alarm time in seconds since 1/1/1904;
													// 0 = none
													
	//-------------------------------------------------------------------------------
	// The following fields are used exclusively to work around a bug in the 0G58E version
	//  of the DragonBall. This bug causes the rtcHourMinSec register in the DragonBall to
	//  occasionally be "out of date" when read and also prevents alarms from going off
	//  if we're in sleep mode. To get around this bug, we use the 1 minute interrupt while
	//  we're in sleep mode to wake up periodically and see if any alarms need to go off and
	//  we use the 1 second interrupt when we're awake to update our own copy of the hours,
	//  minutes and seconds.
	//-------------------------------------------------------------------------------
	Boolean			rtcBugWorkaround;		// true to work around bug in 0G58E version
													//  of dragonball.
	Boolean			rtcBugAsleep;			// true if we're in sleep mode
													
	// Our copy of the time that we maintain through the 1 second and minute interrupts
	UInt32			rtcBugAlarm;				// when alarm should be triggerred.
	UInt32			rtcBugSeconds;				// seconds since 1/1/1904
														// IMPORTANT: The boolean rtcBugFastMinuteCalc MUST
														//  be cleared whenever rtcBugSeconds is changed!!!!!!!
	UInt32			rtcBugLastSeconds;		// seconds after last 1 sec. or min. interrupt.
	Boolean			rtcBugFastMinuteCalc;	// true when current time is on a minute
														//  boundary. Used by TimHandleInterrupt speed up
														//  the next minute calculation.
	UInt8				reserved;
	} TimGlobalsType;

typedef TimGlobalsType*		TimGlobalsPtr;


/************************************************************
 * Function Prototypes
 *************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

//
// IMPORTANT: TimSetAlarm() and TimGetAlarm() are reserved for use by the
// Alarm Manager!
//

// set an alarm in seconds since 1/1/1904
// (reserved for Alarm Manager use only)
UInt32		TimSetAlarm(UInt32 seconds)
						SYS_TRAP(sysTrapTimSetAlarm);

// get the current alarm setting in seconds since 1/1/1904
// (reserved for Alarm Manager use only)
UInt32		TimGetAlarm(void)
						SYS_TRAP(sysTrapTimGetAlarm);
						
Err			TimSleep(Boolean untilReset, Boolean emergency)
						SYS_TRAP(sysTrapTimSleep);

Err			TimWake(void)
						SYS_TRAP(sysTrapTimWake);


//-------------------------------------------------------------------
// Interrupt Service routine
//-------------------------------------------------------------------
void 			TimHandleInterrupt(Boolean periodicUpdate)
						SYS_TRAP(sysTrapTimHandleInterrupt);


#ifdef __cplusplus 
}
#endif




#endif // __TIMEMGR_H__


#endif // NON_PORTABLE
