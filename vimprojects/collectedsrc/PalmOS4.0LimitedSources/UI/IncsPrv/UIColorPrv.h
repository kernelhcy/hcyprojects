/******************************************************************************
 *
 * Copyright (c) 1999 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: UIColorPrv.h
 *
 * Release: 
 *
 * Description:
 * 	This file defines structs and functions for setting the "system
 *		colors" that the UI routines use.
 *
 * History:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	2/3/99	Initial Revision
 *
 *****************************************************************************/

#ifndef __UICOLORPRV_H__
#define __UICOLORPRV_H__

#include <PalmTypes.h>
#include <CoreTraps.h>

#include <UIColor.h>

typedef struct UIColorTableEntryType {
	UInt8 index;
	UInt8 r;
	UInt8 g;
	UInt8 b;
} UIColorTableEntryType;


#define UIColorToRGBColor(uicolor) (*(RGBColorType *)&(uicolor))

#define RGBColorToUIColor(rgbcolor) (*(UIColorTableEntryType *)&(rgbcolor))


// "stack" is just a complete set of 3 tables,
// stored in a single array for simplicity
// the colorTableStackIndex is actually the index of the
// 1st color table entry in that set, that is, a multiple of UILastColorTableEntry
// the colorTableP can be treated as the start of the current array of colors
#define UIColorTableStackSize		(3 * UILastColorTableEntry)

typedef struct {
	UIColorTableEntryType	*colorTableP;
	UIColorTableEntryType	*colorTableStackP;
	Int16							colorTableStackIndex;
	Boolean						tablesDirty;
	UInt8							reserved;
} UIColorStateType;


// UICState.colorTableP[which].curIndex gives the color to use for
// a given UI drawing call - system only, devs us UIColorGetTableEntryIndex trap
#define UIColorGetIndex(which)	(UICState.colorTableP[which].index)


#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------
// System Color Table Manipulation Routines 
//------------------------------------------------------------

extern Err UIColorInit(void)
							SYS_TRAP(sysTrapUIColorInit);

#ifdef __cplusplus 
}
#endif

#endif //__UICOLORPRV_H__
