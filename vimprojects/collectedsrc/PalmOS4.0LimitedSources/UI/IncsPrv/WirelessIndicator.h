/******************************************************************************
 *
 * Copyright (c) 1995-1999 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: WirelessIndicator.h
 *
 * Release: 
 *
 * History:
 *		Jan 11,1999	Created by David Kammer
 *
 *****************************************************************************/

/* Routines:
 *    WiSet    
 *    PrvGetCurrentRSSIPercentage 
 */

#ifndef __WSIND_H__
#define __WSIND_H__

#define WiErrorNoRamIndex	0x07

#define WiRightShiftLastDrawnTicksBy	6

typedef struct {
	UInt8 IsEnabled			:1;	// Set if part of ui 
	UInt8 IFIndex			:3; // 0-3 or WiErrorNoRamIndex
	UInt8 IFLastDrawn		:3; // TimGetTicks of the last time the indicator was drawn,
						// right-shifted by 6 (/64 ~= 2/3 sec. 3 digits gives only a 1/8
						// probability of accidentally failing to draw indicator - just fine.  
	UInt8 Visible			:1;
	UInt8 reserved;
} WiGlobalType;

typedef enum {
 	wiCmdInit =0,
 	wiCmdClear,
 	wiCmdSetEnabled,
 	wiCmdDraw,
	wiCmdEnabled,
	wiCmdSetLocation,
	wiCmdErase
} WiCmdEnum;

#endif // __WSIND_H__
