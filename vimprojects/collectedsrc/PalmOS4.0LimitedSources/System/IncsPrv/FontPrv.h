/******************************************************************************
 *
 * Copyright (c) 1994-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: FontPrv.h
 *
 * Release: 
 *
 * Description:
 *	  This file defines private font structures and routines.
 *
 * History:
 *		10/20/99	kwk	Created by Ken Krugler.
 *		06/07/00	vivek	Added defintions for application defined font types.
 *		06/19/00 acs	Move FntPrvGetFontList here.
 *
 *****************************************************************************/

#ifndef __FONTPRV_H__
#define __FONTPRV_H__

#include <PalmTypes.h>
#include <CoreTraps.h>
#include <Font.h>


// Font types in FontType structure.
#define fntSysFontMap		0xC000		// System defined font map
#define fntAppFontMap		0xC800		// Application defined font map
#define fntAppSubFontMask	0x0400		// Application defined sub font must have
													// bit 10 set.
#define fntFontMapMask		0xC000

typedef struct {
	UInt32		rscType;
	UInt16		rscID;
} FontIndexEntryType, * FontIndexEntryPtr;

// Font mapping state table.
#define	fntStateIsChar			1
#define	fntStateNextIsChar	2

typedef struct {
	UInt8		flags;
	UInt8		state;
	UInt16	value;
} FontMapType, * FontMapPtr;


FontTablePtr FntPrvGetFontList (Int16 fontType)
							SYS_TRAP(sysTrapFntPrvGetFontList);


#endif	// __FONTPRV_H__
