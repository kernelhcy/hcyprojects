/******************************************************************************
 *
 * Copyright (c) 1999-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: SystemResourcesPrv.h
 *
 * Release: 
 *
 * Description:
 *		Include file for both PilotRez and the C Compiler. This file contains
 *  equates used by both tools. When compiling using the C compiler
 *  the variable RESOURCE_COMPILER must be defined.
 *
 * History:
 *   	??/??/99 ???	Created by ????.
 *		11/24/00	kwk	Moved 'tint' resource constants here from SystemPrv.h,
 *							renumbered to correct range for system resources (32000+
 *							is reserved for locale modules).
 *
 *****************************************************************************/

#ifndef 	__SYSTEMRESOURCESPRV_H__
#define	__SYSTEMRESOURCESPRV_H__

#include <SystemResources.h>

// System colorTables
#define systemPaletteBase					10000		// base + depth = palette resource number
#define systemPalette1						10001		// 1bpp palette
#define systemPalette2						10002		// 2bpp palette
#define systemPalette4						10004		// 4bpp palette
#define systemPalette8						10008		// 8bpp palette

#define systemDefaultUIColorsBase		10100		// base + depth = ui color resource number
#define systemDefaultUIColors1			10101		// 1bpp UI colors
#define systemDefaultUIColors2			10102		// 2bpp UI colors
#define systemDefaultUIColors4			10104		// 4bpp UI colors
#define systemDefaultUIColors8			10108		// 8bpp UI colors
								
//  Equates for 'tint' rsrc Ids that hold the hard-reset App's Creator  
//  and the soft-reset App's Creator.
// The 'tint' resources are in the System.rsrc file.
#define sysHardResetAppCreatorConstId	10002
#define sysSoftResetAppCreatorConstId	10003


#endif //__SYSTEMRESOURCESPRV_H__
