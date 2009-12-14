/******************************************************************************
 *
 * Copyright (c) 1999-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: OverlayPrv.h
 *
 * Release: 
 *
 * Description:
 *		Private header for routines that support overlays & locales.
 *
 * History:
 *		10/08/99	kwk	Created by Ken Krugler.
 *		05/18/00	kwk	Added more private stuff from OverlayMgr.h.
 *		05/23/00	kwk	Once more, fixed up omOverlayKindHide comment.
 *		08/17/00	CS		Defined new omSpecAttrSorted flag.  Note that I did NOT
 *							bump the version number, since the order was essentially
 *							undefined previously (and the new flag will be 0 in them).
 *
 *****************************************************************************/

#ifndef	__OVERLAYPRV_H__
#define	__OVERLAYPRV_H__

#include <PalmTypes.h>
#include <DataMgr.h>
#include	<OverlayMgr.h>

#ifdef _WIN32
  #pragma warning(disable: 4200)  // nonstandard extension used : zero-sized array in struct/union
#endif

/***********************************************************************
 * Private Overlay Manager constants
 **********************************************************************/

#define	omOverlayVersion		0x0004	// Version of OmOverlaySpecType/OmOverlayRscType

// Flags for OmOverlaySpecType.flags field
#define	omSpecAttrForBase		1			//	'ovly' (in base) describes base itself
#define	omSpecAttrStripped	2			// Localized resources stripped (base only)
#define	omSpecAttrSorted		4			//	'ovly' elements sorted by type/ID

// Values for OmOverlayKind
#define	omOverlayKindHide		0		// Hide base resource (not supported in version <= 4)
#define	omOverlayKindAdd		1		// Add new resource (not support in version <= 2)
#define	omOverlayKindReplace	2		// Replace base resource
#define	omOverlayKindBase		3		// Description of base resource itself (not supported in version <= 2)

/***********************************************************************
 * Private Overlay Manager types
 **********************************************************************/

typedef UInt16 OmOverlayKind;

typedef struct {
	OmOverlayKind	overlayType;		// Replace, delete, etc.
	UInt32			rscType;				// Resource type to overlay
	UInt16			rscID;				// Resource ID to overlay
	UInt32			rscLength;			// Length of base resource
	UInt32			rscChecksum;		// Checksum of base resource data
} OmOverlayRscType;

// Definition of the Overlay Description Resource ('ovly')
typedef struct {
	UInt16				version;				// Version of this structure
	UInt32				flags;				// Flags
	UInt32				baseChecksum;		// Checksum of all overlays[].checksum
	LmLocaleType		targetLocale;		// Language, & country of overlay resources
	UInt32				baseDBType;			// Type of base DB to overlay
	UInt32				baseDBCreator;		// Creator of base DB to overlay
	UInt32				baseDBCreateDate; // Date base DB was created
	UInt32				baseDBModDate;		// Date base DB was last modified
	UInt16				numOverlays;		// Number of resources to overlay
	OmOverlayRscType	overlays[0];		// Descriptions of resources to overlay
} OmOverlaySpecType;

/***********************************************************************
 * Private Overlay Manager routines
 **********************************************************************/

#ifdef __cplusplus
	extern "C" {
#endif

#if (EMULATION_LEVEL == EMULATION_NONE)
void OmDispatch(void);
#endif

// Initialize the Overlay Manager.

void OmInit(void)
			OMDISPATCH_TRAP(omInit);

// Return the DmOpenRef of the overlay for the open database identified
// by <baseRef> in the overlayRef parameter, or 0 if no such overlay exists.
// If no overlay is found, and <baseRef> is stripped, then return an error.

Err OmOpenOverlayDatabase(DmOpenRef baseRef, const OmLocaleType* overlayLocale,
									DmOpenRef* overlayRef)
			OMDISPATCH_TRAP(omOpenOverlayDatabase);

#ifdef __cplusplus
	}
#endif

#endif
