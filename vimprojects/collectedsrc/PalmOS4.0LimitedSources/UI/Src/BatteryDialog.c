/******************************************************************************
 *
 * Copyright (c) 1994-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: BatteryDialog.c
 *
 * Release: 
 *
 * Description:
 *	  This file contain routines to handle the Battery low dialog.
 *
 *   Updated 11/98 soe, to use percent of battery remaining rather than absolute voltages
 *	  The power manager (hwrBattery) computes a percent battery life remaining taking in to
 *   account voltage drops due to large loads. Using this compensated percent rather than the
 *   absolute voltage eliminates false low battery warnings
 *
 *   3/7/99 SCL: Crude first attempt to merge Eleven & Main versions...
 *
 * History:
 *		March 29, 1995	Created by Roger Flores
 *
 *****************************************************************************/

#define NON_PORTABLE

#include <PalmTypes.h>

// system public includes
#include <SystemPublic.h>

// system private includes
#include "Globals.h"

// hardware public includes
#include "Hardware.h"
#include "HwrBattery.h"
#include "HwrGlobals.h"
#include "HwrMiscFlags.h"

// UI public includes
#include "Form.h"
#include "UIResourcesPrv.h"

// DOLATER еее Need to take the SysBatteryDialog define and move it into UI somewhere

// These are the battery thresholds in volts*100.
#define	prvThresholdDangerous			180	// Dangerous threshold
#define	prvThresholdLow					210	// Low threshold
#define	prvThresholdDangerousPercent	  4	// Dangerous threshold as a percent
#define	prvThresholdLowPercent			 10	// Low threshold as a percent
#define	prvThresholdHysterisis			10		// hysterisis amount

// Time between warnings for the 2 thresholds.
#define	prvTimeoutDangerous				(sysTicksPerSecond*60*3)
#define	prvTimeoutLow						(sysTicksPerSecond*60*10)


/***********************************************************************
 *
 * FUNCTION:    SysBatteryDialog
 *
 * DESCRIPTION: This routine pops up the Battery low warning dialog.
 *
 *					It Gets called by SysHandleEvent in response to a 
 *					lowBatteryChr keyDown event. This key event is generated
 *					by an interrupt routine when the battery level falls
 *					below a certain thresholds, or after a timeout, whichever
 *					comes sooner. The max timeout value is 0xFFFF ticks.
 *
 *
 * PARAMETERS: 
 *
 * RETURNED:   void
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	10/9/99	Do not display UI if SysUIBusy.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
 *
 ***********************************************************************/
void SysBatteryDialog (void)
{
	HwrBatCmdBatteryStateType	cmdP;
	UInt16							lowAlertID;
	UInt16							veryLowAlertID;
	


	// Get the proper alert IDs
	if (GHwrMiscFlagsExt & hwrMiscFlagExtHasLiIon) {
		lowAlertID = LowCradleChargedBatteryAlert;
		veryLowAlertID = VeryLowCradleChargedBatteryAlert;
	}
	else {
		lowAlertID = LowBatteryAlert;
		veryLowAlertID = VeryLowBatteryAlert;
	}
		
	// Get the current battery level
	HwrBattery(hwrBatteryCmdGetBatteryState, &cmdP);	
	
	// don't allow find dialog to come up if UI is marked busy
	if (SysUIBusy(false, false) > 0)
		return;
	
	SysUIBusy(true, true);  // don't let other processes interrupt UI

	// Check the battery state and if it's below our dangerous threshold, display the
	//  extremely low warning.
		
	if (cmdP.state == sysBatteryStateCritBattery) {
		FrmAlert(veryLowAlertID);
	}		
	else if (cmdP.state == sysBatteryStateLowBattery) {
			FrmAlert(lowAlertID);
	}

	SysUIBusy(true,false);  // let other processes interrupt UI

}
