/******************************************************************************
 *
 * Copyright (c) 1994-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: Bitmap.c
 *
 * Release: 
 *
 * Description:
 *             	This file contains the bitmap routines.
 *
 * History:
 *			Name	Date		Description
 *			----	----		-----------
 *			BS		9/99		Created
 *
 *****************************************************************************/

#define NON_PORTABLE

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_BITMAPS
#define ALLOW_ACCESS_TO_INTERNALS_OF_CLIPBOARDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_FIELDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS

#include <PalmTypes.h>
#include <ErrorMgr.h>

#include "Blitter.h"
#include "MemoryPrv.h"		// for memNewChunkFlagAllowLarge
#include "SystemPrv.h"		// for SysGetAppInfo
#include "UIGlobals.h"
#include "WindowPrv.h"


 /***********************************************************************
 *
 * FUNCTION:    	BmpCreate
 *
 * DESCRIPTION: 	Create a bitmap structure
 *
 * RETURNED: 		A pointer to an allocated Bitmap structure
 *						To be freed with BmpDelete.
 *						Or NULL if error
 *
 * PARAMETERS: 	width : The with of the Bitmap (!= 0)
 *						height: The height of the Bitmap (!= 0)
 *						depth:	The pixelSize of the Bitmap (1, 2, 4, 8, or 16)
 *						colortableP: A pointer to the color table associated with the
 *								bitmap or NULL if none.  The colortable MUST have the 
 *								number of color corresponding to the depth required.
 *						error:	return the error code if fail, or errNone
 *
 * HISTORY
 *		09/??/99	BS		Created by Bertrand Simon
 *		10/01/99	kwk	Don't throw non-fatal alert if out of memory, since
 *							that's a valid situation when saving bits behind a
 *							window, especially on a color device.
 *		06/20/00	acs	Allow allocating bitmaps greater than 64K
 *
 ***********************************************************************/
BitmapType* BmpCreate(Coord width, Coord height, UInt8 depth, ColorTableType* colortableP, UInt16* error)
{
	BitmapType*    bitmapP;
	UInt32         bmpSize, dataSize, headerSize, rowBytes;
	UInt16		  	cTableEntries;
	BitmapFlagsType flags = {0};
	UInt16		  	cTableSize = 0;
	BitmapDirectInfoType	directInfo;
	SysAppInfoPtr  unusedAppInfoP;
	UInt8*		  	dstP;
	  
	// --------------------------------------------------------------
	// sanity checks    
	// --------------------------------------------------------------
	if (!error)
		{
		ErrNonFatalDisplay("NULL error argument");
		return NULL;
		}
		
	// width and height
	if ((width == 0) || (height == 0))
		{
		*error = sysErrParamErr;
		ErrNonFatalDisplay("Null width or height");
		return NULL;
		}
	// Depth
	if ((depth != 1) && (depth != 2) && (depth != 4) && (depth != 8) && (depth != 16))
		{
		*error = sysErrParamErr;
		ErrNonFatalDisplay("Unsupported depth");
		return NULL;
		}
	
	// # of entries in color table and directColor info
	if (depth <= 8)
		cTableEntries = (1 << depth);
	else
		{
		// If a direct color bitmap includes a CLUT, it's always 8 bit.
		cTableEntries = (1 << 8);
		flags.directColor = true;
		
		MemSet(&directInfo, sizeof(directInfo), 0);
		directInfo.redBits = 5;
		directInfo.greenBits = 6;
		directInfo.blueBits = 5;
		}

	// colortable
	if (colortableP)
		{
		flags.hasColorTable = true;
		
		if (colortableP->numEntries != cTableEntries)
			{
			*error = sysErrParamErr;
			ErrNonFatalDisplay("Invalid colortable");
			return NULL;
			}
		}
	
	*error = errNone;

  // --------------------------------------------------------------------
  // Allocate a bitmap. Round the line width
  // up to an even word boundary. 
  // --------------------------------------------------------------------
	rowBytes = (((width * depth) + 15) >> 4) << 1;	// ((w*d) + 15) / 16 * 2
	dataSize = rowBytes * height;
	
	bmpSize = sizeof(BitmapType) + dataSize;
	
	if (colortableP)
		bmpSize += sizeof(ColorTableType) + (sizeof(RGBColorType) * colortableP->numEntries);
	
	// Direct color?
	if (flags.directColor)
		bmpSize += sizeof(BitmapDirectInfoType);

	// allow allocation of blocks greater than 64K
	bitmapP = MemChunkNew(0, bmpSize, memNewChunkFlagAllowLarge | memNewChunkFlagNonMovable |
													SysGetAppInfo(&unusedAppInfoP, &unusedAppInfoP)->memOwnerID);
	
	if (!bitmapP)
		{
		*error = sysErrNoFreeResource;
		return NULL;
		}

	// init bitmap structure to 0, and init bitmap data to white
	headerSize = bmpSize - dataSize;
	MemSet(bitmapP, headerSize, 0);
	MemSet((void*) ((long) bitmapP + headerSize), dataSize, (depth > 8) ? 0xFF/*RGB*/ : 0x00/*index*/);
	
	bitmapP->width = width;
	bitmapP->height = height;
	bitmapP->rowBytes = rowBytes;
	bitmapP->pixelSize = depth;
	bitmapP->version = BitmapVersionTwo;
	bitmapP->flags = flags;
	
	// Next place for stuff...
	dstP = (UInt8*) (bitmapP + 1);
	
	// Copy in the color table if present
	if (bitmapP->flags.hasColorTable)
		{
		cTableSize = sizeof(ColorTableType) + (sizeof(RGBColorType) * colortableP->numEntries);
		MemMove(dstP, colortableP, cTableSize);			 
		dstP += cTableSize;  
		}
	
	// Put in the directColor info if present
	if (bitmapP->flags.directColor)
		{
		MemMove(dstP, &directInfo, sizeof(directInfo));	  	  			   
		}
	
	return bitmapP;
}


 /***********************************************************************
 *
 * FUNCTION:    	BmpDelete
 *
 * DESCRIPTION: 	Delete a bitmap structure
 *						Does not delete the Bits data if indirect bitmap
 *
 * RETURNED: 		Error if any.
 *				
 * PARAMETERS:  	bitmaP : A pointer to a Bitmap
 *
 ***********************************************************************/
 Err BmpDelete (BitmapType* bitmapP)
 {
 	Err err = errNone;
 
 	// Sanity checks
 	if (!bitmapP || bitmapP->flags.forScreen ||  MemPtrDataStorage(bitmapP) ) 
		{
		ErrNonFatalDisplay("invalid bitmapP");
 		return sysErrParamErr;
 		}
 		
	err = MemChunkFree(bitmapP);
	
	return err;
} 
 

 /***********************************************************************
 *
 * FUNCTION:    	BmpCompress
 *
 * DESCRIPTION: 	Compress a bitmap using given compression type and resize 
 *					 	the allocated memory.  Is able to uncompress a compressed 
 *						bitmap if comptype == BitmapCompressionTypeNone
 *						Can not compress a Bitmap in Data Storage.
 *
 * RETURNED: 		Err : The error code if any
 *
 * PARAMETERS:  	bitmapP	-> Ptr to a bitmap structure.
 *					 	compType	-> BitmapCompressionType
 *
 ***********************************************************************/
Err BmpCompress(BitmapType* bitmapP, BitmapCompressionType compType )
{
	BitmapPtr tmpBmpP;
	Err err;
	DrawStateType drawState;
	UInt16 bmpSize;
	CanvasType canvas;
	RectangleType dstRect;
	RGBColorType color = {0,0xFF,0xFF,0xFF}; // White

	// sanity checks
	// bitmapP
	if (!bitmapP || bitmapP->flags.indirect ||
		(bitmapP->flags.compressed && (compType != BitmapCompressionTypeNone)) || 
		bitmapP->flags.forScreen || MemPtrDataStorage(bitmapP)) 
		{
		ErrNonFatalDisplay("invalid bitmapP");
		return sysErrParamErr;
		}
	// compType
	if ((compType != BitmapCompressionTypeScanLine) && (compType != BitmapCompressionTypeRLE) && 
		 (compType != BitmapCompressionTypePackBits) && (compType != BitmapCompressionTypeNone)) 
		{
		ErrNonFatalDisplay("invalid compType");
		return sysErrParamErr;
		}
		
	// exit if this is a bitmap family
	if (bitmapP->nextDepthOffset)
		{
		ErrNonFatalDisplay("cannot compress bitmap family");
		return sysErrParamErr;
		}

	tmpBmpP = BmpCreate(bitmapP->width, bitmapP->height, bitmapP->pixelSize, BmpGetColortable(bitmapP), &err);
	
	if (err)
		return err;
	
	// Do the compression/decompression of bitmapP into tmpBmpP
	tmpBmpP->flags.compressed = (compType != BitmapCompressionTypeNone);
	tmpBmpP->compressionType = compType;
	
	drawState.transferMode = winPaint;
	drawState.pattern = noPattern;
	
	drawState.backColor = WinRGBToIndex(&color); 		// White
	color.r = 0x00; color.g = 0x00; color.b = 0x00;		// Black
	drawState.foreColor = WinRGBToIndex(&color);

	WinPrvInitCanvasWithBitmap(tmpBmpP, &drawState, &canvas);
	RctSetRectangle(&dstRect, 0, 0, tmpBmpP->width, tmpBmpP->height);
	BltCopyRectangle(&canvas, bitmapP, &dstRect, 0, 0);
	
	// resize the pointer used to store the bits
	bmpSize = BmpSize(tmpBmpP);
	err = MemPtrResize(bitmapP, bmpSize);
	if (err) 
		{
		MemPtrFree(tmpBmpP);
		return err;
		}
	MemMove(bitmapP, tmpBmpP, bmpSize);
	
	BmpDelete(tmpBmpP);

	return errNone;
}


 /***********************************************************************
 *
 * FUNCTION:    	BmpGetBits
 *
 * DESCRIPTION: 	Retrieves the address of bitmap data from a BitMap ptr.
 *						Works also if the bitmap as indirect bits
 *
 * RETURNED: 		A pointer to the Bits (Data of the picture) 
 *
 * PARAMETERS:  	bitmapP	->	Ptr to a bitmap structure.
 *
 ***********************************************************************/
void* BmpGetBits(BitmapType* bitmapP)
{
	UInt8 *result;

	if (!bitmapP) 
		{
		ErrNonFatalDisplay("NULL bitmap");
		return NULL;
		}
	
	// by default, bitmaps start right after the header
	result = (UInt8*)(bitmapP + 1);

	// if there's a colortable, skip it.
	result += BmpColortableSize(bitmapP);
	
  	// If there's direct color info, skip it
  	if (bitmapP->flags.directColor)
	  result += sizeof(BitmapDirectInfoType);

	// if the bitmap is indirect, follow the pointer
	if (bitmapP->flags.indirect)
		result = *(void**)result;
	
	return result;
}


 /***********************************************************************
 *
 * FUNCTION:    	BmpGetColortable
 *
 * DESCRIPTION: 	Retrieves the colortable of bitmap or null if no colortable
 *
 * RETURNED: 		A pointer to the color table or NULL
 *
 * PARAMETERS:  	bitmapP	->	Ptr to a bitmap structure.
 *
 ***********************************************************************/
ColorTableType* BmpGetColortable(BitmapType* bitmapP)
{
	if (!bitmapP) 
		{
		ErrNonFatalDisplay("NULL bitmap");
		return NULL;
		}
	
	if (bitmapP->flags.hasColorTable)
		return (ColorTableType*)(bitmapP + 1);
	else
		return NULL;
}


 /***********************************************************************
 *
 * FUNCTION:    	BmpSize
 *
 * DESCRIPTION: 	return the size in bytes of a bitmap 
 *						including the colortable if present and bits.
 *						Excluding the bits (data) if it is an indirect bitmap.
 *
 * RETURNED: 		Size of the contiguous memory allocated for the bitmap.
 *						Does not include the Bits if indirect bitmap.
 *
 * PARAMETERS:  	bitmapP	->	Ptr to a bitmap structure.
 *
 ***********************************************************************/
UInt16 BmpSize(const BitmapType* bitmapP)
{
	UInt16 bmpSize;

	if (!bitmapP) 
		{
		ErrNonFatalDisplay("NULL bitmap");
		return 0;
		}
	
	// Header and Color table
	bmpSize = sizeof(BitmapType) + BmpColortableSize(bitmapP);

  	// Add directColor info if present
  	if (bitmapP->flags.directColor)
		bmpSize += sizeof(BitmapDirectInfoType);

	// data (may be indirect)
	if (bitmapP->flags.indirect)
		bmpSize += sizeof(void*);
	else
		bmpSize += BmpBitsSize(bitmapP);
		
	return bmpSize;
}


 /***********************************************************************
 *
 * FUNCTION:    	BmpBitsSize
 *
 * DESCRIPTION: 	return the DATA size in bytes of a bitmap (the BITS)
 *						the Bits can be compressed and/or indirect
 *
 * RETURNED: 		The size in Bytes of the Bits (Data)
 *
 * PARAMETERS:  	bitmapP		->	Ptr to a bitmap structure.
 *
 ***********************************************************************/
UInt16 BmpBitsSize(const BitmapType* bitmapP)
{
	UInt32 size = 0;
	BmpGetSizes (bitmapP, &size, NULL);

	ErrNonFatalDisplayIf (size >= 65536, "BmpBitsSize >= 64K");
	return (UInt16) size;
}


 /***********************************************************************
 *
 * FUNCTION:    	BmpGetSizes
 *
 * DESCRIPTION: 	This function combines BmpSize and BmpBitsSize, and
 *						handles bitmaps with sizes greater than 64Kb.
 *              	Parameters can be NULL if caller doesn't want that info.
 *
 * RETURNED: 		Return the data size in bytes of the bitmap data, which
 *						can be compressed and/or indirect.  Return the total 
 *						size in bytes of the BitmapType structure, color table 
 *						(if any), and BitmapDirectInfoType structure (if any). 
 *
 * PARAMETERS:  	bitmapP		->	Ptr to a bitmap structure
 *						dataSizeP	<- size of bitmap data
 *						headerSizeP	<- total size of bitmap structures
 *
 ***********************************************************************/
void BmpGetSizes(const BitmapType* bitmapP, UInt32* dataSizeP, UInt32* headerSizeP)
{
	if (!bitmapP) 
		{
		ErrNonFatalDisplay("NULL bitmap");
		return;
		}
	
	if (dataSizeP)
		{
		// the size of a compressed bitmap is stored in the first UInt16 of compressed data
		if (bitmapP->flags.compressed)
			{
			// Casting away const is safe because we don't try to modify through
			// the result of BmpGetBits
			*dataSizeP = *((UInt16*) BmpGetBits((BitmapType *) bitmapP));
			}
		else
			*dataSizeP = ((UInt32) bitmapP->rowBytes) * ((UInt32) bitmapP->height);
		}
		
	if (headerSizeP)
		{
		// BitmapType and color table
		*headerSizeP = sizeof(BitmapType) + BmpColortableSize(bitmapP);
	
		// add BitmapDirectInfoType if present
		if (bitmapP->flags.directColor)
			*headerSizeP += sizeof(BitmapDirectInfoType);
	
		// add size of pointer to data if indirect
		if (bitmapP->flags.indirect)
			*headerSizeP += sizeof(void*);
		}
}


 /***********************************************************************
 *
 * FUNCTION:    	BmpColortableSize
 *
 * DESCRIPTION: 	return the color table size in bytes of a bitmap
 *
 * RETURNED: 		size of the colortable
 *
 * PARAMETERS:  	bitmapP		->	Ptr to a bitmap structure.
 *
 ***********************************************************************/
UInt16 BmpColortableSize(const BitmapType* bitmapP)
{
	if (!bitmapP) 
		{
		ErrNonFatalDisplay("NULL bitmap");
		return 0;
		}
	
	if (bitmapP->flags.hasColorTable)
		return ( sizeof(ColorTableType) + 
			(sizeof(RGBColorType) * ((ColorTableType *)(bitmapP +1))->numEntries) );
	else
		return 0;
}


 /***********************************************************************
 *
 * FUNCTION:    	BmpGetDimensions
 *
 * DESCRIPTION: 	returns the width/height/rowBytes of a bitmap.
 *              	Parameters can be NULL if caller doesn't want that info.
 *
 * RETURNED: 		nothing
 *
 * PARAMETERS:  	bitmapP		->	Ptr to a bitmap structure.
 *					 	widthP		-> location to return width
 *					 	heightP		-> location to return height
 *					 	rowBytesP	-> location to return rowBytes
 *
 ***********************************************************************/
void BmpGetDimensions(const BitmapType* bitmapP, Coord* widthP, Coord* heightP, UInt16* rowBytesP)
{
	if (!bitmapP) 
		{
		ErrNonFatalDisplay("NULL bitmap");
		return;
		}
	
	if (widthP)
		*widthP = bitmapP->width;
	if (heightP)
		*heightP = bitmapP->height;
	if (rowBytesP)
		*rowBytesP = bitmapP->rowBytes;
}


 /***********************************************************************
 *
 * FUNCTION:    	BmpGetBitDepth
 *
 * DESCRIPTION: 	returns the bit depth of a bitmap
 *
 * RETURNED: 		bit depth
 *
 * PARAMETERS:  	bitmapP		->	Ptr to a bitmap structure.
 *
 ***********************************************************************/
UInt8 BmpGetBitDepth(const BitmapType* bitmapP)
{
	if (!bitmapP) 
		{
		ErrNonFatalDisplay("NULL bitmap");
		return 0;
		}
	
	// Version 0 of the bitmap struct did not have a depth field
	if (bitmapP->version < 1)
		return 1;
		
	return bitmapP->pixelSize;
}


 /***********************************************************************
 *
 * FUNCTION:    	BmpGetNextBitmap
 *
 * DESCRIPTION: 	returns the next bitmap struct of a bitmap family
 *
 * RETURNED: 		pointer to the next bitmap (or NULL if no more)
 *
 * PARAMETERS:  	bitmapP		->	Ptr to a bitmap structure.
 *
 ***********************************************************************/
BitmapType* BmpGetNextBitmap(BitmapType* bitmapP)
{
	if (!bitmapP) 
		{
		ErrNonFatalDisplay("NULL bitmap");
		return NULL;
		}
	
	// Version 0 of the bitmap struct did not support bitmap 'families'
	if (bitmapP->version < 1)
		return NULL;

	// No more bitmaps in this 'family'
	if (!bitmapP->nextDepthOffset)
		return NULL;
		
	return (BitmapType*)((UInt32*)bitmapP + bitmapP->nextDepthOffset);
}
