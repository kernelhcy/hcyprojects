/******************************************************************************
 *
 * Copyright (c) 1995-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: ScrUtils.c
 *
 * Release: 
 *
 * Description:
 *			Screen compression utility functions
 *
 * History:
 *			Created by Ron Marianetti
 *			Rewrite of "ScanLine" and PackBits compression/decompression. -- pkw
 *
 *****************************************************************************/

#include <PalmTypes.h>
#include <DebugMgr.h>
#include "ScrUtils.h"

#include <ErrorMgr.h>


// *************************************************************************
// Private Compression/Decompression state objects
// *************************************************************************

typedef struct ScanLineCompStateType {
	UInt8* prevLineP;
	Boolean notFirstLine;		// false if this is the first line of the compression.
	UInt8	reserved;			
} ScanLineCompStateType;

typedef ScanLineCompStateType *ScanLineCompStatePtr;


typedef struct RLEStateType {
	UInt8 prevByteCount;
	UInt8 prevByte;
} RLEStateType;

typedef RLEStateType *RLEStatePtr;



// *************************************************************************
// Prototypes
// *************************************************************************
static Int32 ScrCompressRLE(UInt8* srcP, UInt32 srcBufLen, UInt8* dstP, UInt32 dstBufLen);
static Int32 ScrDecompressRLE(UInt8* srcP, UInt8* dstP, UInt32 dstBufLen, UInt8* prevByteCount, UInt8* prevByte);
						
	

 /***********************************************************************
 *
 * FUNCTION:	ScrCompressScanLine
 *
 * DESCRIPTION: Utility routine used by Screen driver in order to compress
 *		a scan line of a bitmap. Each scan-line is compressed as a sequence of 
 *		8 byte sections where each section starts with a byte of flags followed by
 *		up to 8 bytes of data. If a bit in the flags byte is clear, the byte is taken
 *		from the previous scan-line, else the byte is taken from the 0-8 bytes of data
 *		following the flags byte.
 *
 * bytes: 		   1  |       N                           | 1     |        N
 *            ----------------------------------------------------------------------
 *             flags | 1 byte for each bit set in flags  | flags | 
 *
 *	PARAMETERS:
 *		lineP 		- pointer to source scan line
 *		prevLineP 	- pointer to previous source scan line
 *		width 		- width of source scan line in bytes.
 *		dstP 			- pointer to space for compressed data. 
 *		firstLine 	- true if this is the first scan line.
 *
 * RETURNED:	# of bytes placed in dstP.
 *
 * CALLED BY:
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			RM		3/3/95	Initial Revision
 *			PKW		12/07/00	Rewrote for greater speed.
 *
 ***********************************************************************/
UInt16 ScrCompressScanLine(UInt8* lineP, UInt8* prevLineP, UInt16 width, UInt8* dstParamP,
					Boolean firstLine)
{
	UInt8 * dstP = dstParamP;
	Int16 i;

	if (firstLine)
	{
		// For the first line there is no previous line, so we just output
		// an "uncompressed"--all flags set to indicate that each byte is new data.
		while ((Int16)(width-=8) >= 0)
		{
			*dstP++ = 0xFF;
			i = 2;
			do
			{
				*dstP++ = *lineP++;
				*dstP++ = *lineP++;
				*dstP++ = *lineP++;
				*dstP++ = *lineP++;
			}
			while ((i-=1) > 0);
		}
		width += 8;

		// Finish off any stragglers.  For no apparent
		// reason, the flag byte is padded with 0's after
		// the last byte in the scan line.
		if (width > 0)
		{
			*dstP++ = (0xFF << (8-width));
			i = width;
			do
			{
				*dstP++ = *lineP++;
			}
			while ((i-=1) > 0);
		}
	}
	else		// !firstLine
	{
		// The "flags" holds the accumulated bits.  The "flagsP" points
		// to where those 8 bits are to be written (since we don't finish
		// calculating "flags" until after we pass by its place in the 
		// outgoing stream).
		UInt8 flags = 0;	// initialize to keep the compiler from warning
		UInt8 * flagsP;

		// Process 8 bytes of input at a time.
		while ((Int16)(width-=8) >= 0)
		{
			flagsP = dstP;
			dstP++;
			i = 4;
			do
			{
				// The next 4 lines of code are "optimized" for Metroworks' 68k compiler.
				// Adding "flags" to itself is faster than shifting by one bit.
				// The "if" statement should generate the lovely CMPM instruction
				// with increment of each register.
				// "flags += 1" is the fastest way to set the low bit of a byte
				// when we know that the bit is currently off.
				// The final line accesses "lineP" with negative offset
				// to accomodate the increment in the "if"--more efficient
				// than NOT incrementing at first and then always having to increment
				// as an extra instruction at the end.
				flags += flags;
				if (*lineP++ != *prevLineP++)
				{
					flags += 1;
					*dstP++ = lineP[-1];
				}

				flags += flags;
				if (*lineP++ != *prevLineP++)
				{
					flags += 1;
					*dstP++ = lineP[-1];
				}
			}
			while ((i-=1) > 0);
			
			*flagsP = flags;
		
		}
		width += 8;	// account for the fact that the loop control above over-steps.

		// Finish off the stragglers.
		if (width > 0)
		{
			flagsP = dstP;
			dstP++;
			i = width;
			do
			{
				flags += flags;
				if (*lineP++ != *prevLineP++)
				{
					flags += 1;
					*dstP++ = lineP[-1];
				}
			}
			while ((i-=1) > 0);
			flags <<= (8 - width);
			
			*flagsP = flags;
		} 
	}

	// Return # of bytes in compressed scan line
	return (dstP - dstParamP);

}






 /***********************************************************************
 *
 * FUNCTION:	ScrDeCompressScanLine
 *
 * DESCRIPTION: Utility routine used by Screen driver in order to decompress
 *		a scan line of a bitmap. 
 *
 *	PARAMETERS:
 *		src 	- pointer to source compressed scan line
 *		dstP -  where uncompressed scan line goes, contains previous line of pixels.
 *		width - width of uncompressed scan line in bytes.
 *		firstLine - true if this is the first line of the bitmap
 *
 * RETURNED:	# of bytes of compressed data used up
 *
 * CALLED BY:
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			RM		3/3/95	Initial Revision
 *			PKW		12/07/00	Rewrote for greater speed.
 *
 ***********************************************************************/
UInt16 ScrDeCompressScanLine(UInt8* srcParamP, UInt8* dstP, UInt16 width)
{
	UInt8 flags;
	Int16 i;
	UInt8 *srcP = srcParamP;
	UInt8 highBit = 0x80;

	// This variable is handy in the innermost loop, so
	// make sure compiler puts this into a register.  
	highBit = ~highBit;	
	highBit = ~highBit;


	// Process 8 bytes of output at a time.
	while ((Int16)(width-=8) >= 0)
	{
		flags = *srcP++;
		if (flags == 0)
		{
			// If "flags" is 0, then all 8 bytes are copied from the previous
			// line.  Since we are writing this line over top of the previous
			// line, we can just skip ahead 8 bytes.  When images
			// compress well (like the typical background behind a
			// menu on an 8 bit or higher display), this shortcut
			// more than DOUBLES the speed of this routine.
			dstP += 8;
		}
		else
		{
			i = 4;
			do
			{
				if (flags >= highBit) *dstP = *srcP++;
				dstP++;
				flags += flags;

				if (flags >= highBit) *dstP = *srcP++;
				dstP++;
				flags += flags;
			}
			while ((i-=1) > 0);
		}
	}
	width += 8;

	// Process stragging bytes.
	if (width > 0)
	{
		flags = *srcP++;
		do
		{
			if (flags >= highBit) *dstP = *srcP++;
			dstP++;
			flags += flags;
		}
		while ((width-=1) > 0);
	}



	// Return # of bytes we used up in source
	return (srcP - srcParamP);

}



 /***********************************************************************
 *
 * FUNCTION:		ScrCompressRLE
 *
 * DESCRIPTION: 	Given a pointer to a screen data, compress the data into
 *						RLE format: 1 byte of length (n) followed by the byte of
 *						data to be repeated. Worst case RLE is 2x the src buffer
 *						if every pixel is different. This compression is not
 *						good for data that has high variance from byte to byte.
 *
 *
 *	PARAMETERS:		srcP			-> MemPtr to the data to be compressed.
 *						srcBufLen	-> Length of srcP and dstP
 *						dstP			-> MemPtr to compressed data.
 *						dstBufLen	->	Max Size of dst buffer.
 *
 * RETURNED:		length of compressed data or -1 if dstP was not large
 *						enough for compressed data.
 *
 ***********************************************************************/
static Int32 ScrCompressRLE(UInt8* srcP, UInt32 srcBufLen, UInt8* dstP, UInt32 dstBufLen)
{
	Int32	totalBytes;
	UInt8 curByte, curByteSize;

	totalBytes = 0;
	
	// Compress until all bytes are read.
	while (srcBufLen > 0) {
		// Load the first byte.
		curByte = *srcP++;
		curByteSize = 1;
		srcBufLen--;			// we've already used one byte

		// Read the number of like bytes or until there are 255 of them.
		while ((srcBufLen) && (*srcP == curByte) && (curByteSize < 255)) {
			curByteSize++;
			srcP++;
			srcBufLen--;
		}
		
		// Increment our compressed size and check to make sure we won't overfill dstP.
		totalBytes += 2;
		if (totalBytes > dstBufLen)
			return -1;
			
		// Save the length and byte
		*dstP++ = curByteSize;
		*dstP++ = curByte;
	}
	
	return totalBytes;
}



 /***********************************************************************
 *
 * FUNCTION:		ScrDecompressRLE
 *
 * DESCRIPTION: 	Given a pointer to a compressed data, uncompress the data using
 *						RLE format: 1 byte of length (n) followed by the byte of
 *						data to be repeated.
 *
 * PARAMETERS:		srcP				-> MemPtr to the data to be uncompressed.
 *						dstP				-> MemPtr for uncompressed data.
 *						dstBufLen		-> Max space for uncompressed data.
 *						prevByteCount 	-> Number of bytes to fill from previous decompress iteration.
 *											<- Number of bytes to fill for next iteration.
 *						prevByte			-> UInt8 to decompressed from previous iteration.
 *											<- UInt8 to be decompressed for next iteration.
 *
 * RETURNED:		 # of bytes of compressed data used up
 *
 ***********************************************************************/
static Int32 ScrDecompressRLE(UInt8* srcParamP, UInt8* dstP, UInt32 dstBufLen,
							  UInt8* prevByteCount, UInt8* prevByte)
{
	Int32	compressedBytesRead = 0;
	Int32 	decompressedBytes = 0;
	UInt8 *srcP = srcParamP;
	UInt8 	i, curByte, curByteSize;
	Boolean	leaveEarly = false;

	// Are there bytes from the last iterative call? If so, fill them into the dstP first.
	if (*prevByteCount) {
		// Calculate if there's enough space in the dstP. If not, prepare to leave early.
		if (*prevByteCount >= dstBufLen) {
			// Case we already have enough
			*prevByteCount -= dstBufLen;
			for (i = 0; i < dstBufLen; i++)
				*dstP++ = *prevByte;
			return 0;
		}

		for (i = 0; i < *prevByteCount; i++)
			*dstP++ = *prevByte;
			
		decompressedBytes = *prevByteCount;
		*prevByteCount = 0;
	}
		
	while (true) {
		// Get the byte count and byte
		curByteSize = *srcP++;
		curByte = *srcP++;
		compressedBytesRead += 2;
		
		// Calculate if there's enough space in the dstP. If not, prepare to leave early.
		if ((decompressedBytes + curByteSize) >= dstBufLen) {
			leaveEarly = true;
			*prevByteCount = (UInt8) ((decompressedBytes + curByteSize) - dstBufLen);
			*prevByte = curByte;
			curByteSize -= *prevByteCount;
		}
		
		// Repeat the byte curByteSize times.
		for (i = 0; i < curByteSize; i++)
			*dstP++ = curByte;
	
		// Increment the decompressed size.
		decompressedBytes += curByteSize;
		
		if (leaveEarly)
			break;
	}
	
	return compressedBytesRead;		// Return # of bytes we used up in source
}




/***********************************************************************
 *
 * FUNCTION:		ScrCompressPackBits
 *
 * DESCRIPTION: 	Compresses some data using PackBits format.  See the
 *					description of the decomperssor or
 *					Apple's technotes on PackBits for details.
 *
 *					Here are some subtle special notes:  a repeat count of
 *					128 is never written (as per Apple's instructions).
 *					However, the decompressor will handle repeat counts of
 *					128.  A "repeat count" of 128 means that the byte
 *					in the compressed data is -128.  A "count" of 0 means
 *					0+1 verbatum bytes.  The compressor won't do a
 *					repeat section until at least 3 bytes are repeated
 *					although the decompressor will happily accept a repeat
 *					of 2.
 *
 *
 * PARAMETERS:		srcP				->	MemPtr to the data to be compressed.
 *					srcLen				->	# of bytes to compress.
 *					dstP				->	MemPtr for compressed output data.
 *
 * RETURNED:		Length of compressed. If less than zero, an error ocurred.
 *
 *
 * YET MORE NOTES:	This algorithm can compress gobs of input data in one
 *					fell swoop.  However, its original intended purpose was
 *					to compress single scan lines of data.  For a given
 *					input of size N, no more than 128*N / 127 + 1 bytes
 *					will be output (I think).
 *
 ***********************************************************************/
static Int32 ScrCompressPackBits(const UInt8 * srcP, UInt32 srcLen, UInt8 * dstP)
{
	UInt8 * dstPStart = dstP;				// make copy
	const UInt8 * srcPEnd = srcP + srcLen;	// stop when we get to here
	const UInt8 * srcPEndMinus2 = srcPEnd - 2;	// a useful constant for inner loop

	while (srcP != srcPEnd)
	{
		Int32 runLength;
		UInt8 byte = *srcP;
		const UInt8 * runP = srcP;
		do { runP+=1; } while (runP != srcPEnd && *runP==byte);
		// runP now points ONE BEYOND the run of duplicate
		// bytes.
		while ((runLength = runP - srcP) >= 3)
		{
			if (runLength > 128) runLength = 128;	// old compressor only works to 127
			*dstP++ = (UInt8)(-runLength + 1);
			*dstP++ = byte;
			srcP += runLength; 
		}

		
		// Now we're either at the end or have a non-run.
		runP = srcP;
		for(;;)
		{
			if (runP >= srcPEndMinus2)	// runP >= srcPEnd - 2
			{
				runP = srcPEnd;
				break;
			}
			
			if (runP[0] == runP[1] && runP[0] == runP[2]) break;

			runP += 1;
		}

		// runP now points ONE BEYOND the run of verbatim bytes.
		while ((runLength = runP - srcP) > 0)
		{
			if (runLength > 127) runLength = 127;
			*dstP++ = (UInt8)(runLength - 1);
			do
			{
				*dstP++ = *srcP++;
			}
			while ((runLength-=1) > 0);
		}

	}
		
	
	return dstP - dstPStart;
}


/***********************************************************************
 *
 * FUNCTION:		ScrCompressPackBits16
 *
 * Note that the srcLen gives the number of WORDs to compress.   Otherwise,
 * this function is the same as the 8-bit version above.
 ***********************************************************************/
static Int32 ScrCompressPackBits16(const UInt16 * srcP, UInt32 srcLen, UInt8 * dstP)
{
	UInt8 * dstPStart = dstP;				// make copy
	const UInt16 * srcPEnd = srcP + srcLen;	// stop when we get to here
	const UInt16 * srcPEndMinus2 = srcPEnd - 2;	// a useful constant for inner loop

	while (srcP != srcPEnd)
	{
		Int32 runLength;
		UInt16 word = *srcP;
		const UInt16 * runP = srcP;
		do { runP+=1; } while (runP != srcPEnd && *runP==word);
		// runP now points ONE BEYOND the run of duplicate
		// bytes.
		while ((runLength = runP - srcP) >= 3)
		{
			if (runLength > 128) runLength = 128;	// old compressor only works to 127
			*dstP++ = (UInt8)(-runLength + 1);
			*dstP++ = (UInt8)(word >> 8);
			*dstP++ = (UInt8)word;
			srcP += runLength; 
		}

		
		// Now we're either at the end or have a non-run.
		runP = srcP;
		for(;;)
		{
			if (runP >= srcPEndMinus2)	// runP >= srcPEnd - 2
			{
				runP = srcPEnd;
				break;
			}
			
			if (runP[0] == runP[1] && runP[0] == runP[2]) break;

			runP += 1;
		}

		// runP now points ONE BEYOND the run of verbatim bytes.
		while ((runLength = runP - srcP) > 0)
		{
			if (runLength > 127) runLength = 127;
			*dstP++ = (UInt8)(runLength - 1);
			do
			{
				UInt16 word = *srcP++;
				*dstP++ = (UInt8)(word >> 8);
				*dstP++ = (UInt8)word;
			}
			while ((runLength-=1) > 0);
		}

	}
		
	
	return dstP - dstPStart;
}




/***********************************************************************
 *
 * FUNCTION:		ScrDecompressPackBits
 *
 * DESCRIPTION: 	Decompresses some data in PackBits format.  See 
 *					Apple's technotes on PackBits for details.
 *
 * ALGORITHM:		Read a "count" byte from source.  If the count is < 0 then the next
 *					source byte is to be written (-count + 1) times to the
 *					destination.  If the count byte is >= 0, then the next
 *					(count + 1) source bytes are to be copied to the
 *					destination.  If the desired output length hasn't yet
 *					been reached, read another "count" byte from the source
 *					and repeat.
 *
 *
 * PARAMETERS:		srcP				->	MemPtr to the compressed data.
 *					dstP				->	MemPtr for decompressed output data.
 *					dstLen				->	# of bytes to output.
 *
 * RETURNED:		Length of compressed data consumed.
 *					If less than zero, an error ocurred.
 *
 * SPECIAL NOTES:	If a given segement of compressed data overruns the
 *					destination, an error is returned and the destination
 *					may not be fully filled.
 *
 ***********************************************************************/
static Int32 ScrDecompressPackBits(const UInt8 * srcP, UInt8 * dstP, Int32 dstLen)
{
	const UInt8 * srcPStart = srcP;
	UInt8 * dstPStop = dstP + dstLen;
	
	while (dstP < dstPStop)
	{
		Int16 count = (Int8)(*srcP++);
		if (count < 0)
		{
			UInt8 byte = *srcP++;
			count = -count + 1;
			if (dstP + count > dstPStop) return -1;	// overrun

			while ((count-=1) >= 0)
			{
				*dstP++ = byte;
			}
		}
		else
		{
			count += 1;
			if (dstP + count > dstPStop) return -1;  // overrun
			while ((count-=1) >= 0)
			{
				*dstP++ = *srcP++;
			}
		}
	}


	return srcP - srcPStart;
}


/***********************************************************************
 *
 * FUNCTION:		ScrDecompressPackBits16
 *
 * Note that the dstLen gives the number of WORDs to decompress.   Otherwise,
 * this function is the same as the 8-bit version above.
  ***********************************************************************/
static Int32 ScrDecompressPackBits16(const UInt8 * srcP, UInt16 * dstP, Int32 dstLen)
{
	const UInt8 * srcPStart = srcP;
	UInt16 * dstPStop = dstP + dstLen;
	UInt16 word;
	
	while (dstP < dstPStop)
	{
		Int16 count = (Int8)(*srcP++);
		if (count < 0)
		{
			word = ((UInt16)(*srcP++)) << 8;
			word |= (UInt8)(*srcP++);
			count = -count + 1;
			if (dstP + count > dstPStop) return -1;	// overrun

			while ((count-=1) >= 0)
			{
				*dstP++ = word;
			}
		}
		else
		{
			count += 1;
			if (dstP + count > dstPStop) return -1;  // overrun
			while ((count-=1) >= 0)
			{
				word = ((UInt16)(*srcP++)) << 8;
				word |= (UInt8)(*srcP++);
				*dstP++ = word;
			}
		}
	}


	return srcP - srcPStart;
}



/***********************************************************************
 *
 * FUNCTION:		ScrCompress
 *
 * DESCRIPTION: 	Given a source buffer MemPtr and a dst buffer MemPtr, compress
 *						the data using the method designated by compressionType.
 *
 * PARAMETERS:		srcP				->	MemPtr to the data to be uncompressed.
 *						srcLenP			->	Size of compressed data buffer.
 *						dstP				->	MemPtr for uncompressed data.
 *						dstBufLen		->	Max space for uncompressed data.
 *						compStateP		->	Extra variables needed by the specific compression
 *										 		 method to itertively compress data.
 *											<-	Extra state info needed next time the function is
 *												 called for compressing data in small chunks.
 *
 * RETURNED:		length of uncompressed. If less than zero, an error ocurred.
 *
 ***********************************************************************/
Int32 ScrCompress(BitmapCompressionType compressionMethod, UInt8* srcP, UInt32 srcBufLen,
				   		UInt8* dstP, UInt32 dstBufLen, CompStateType* compStateP)
{
	Int32 returnVal;

	switch (compressionMethod) {
		case BitmapCompressionTypeScanLine:
			returnVal = ScrCompressScanLine(srcP, ((ScanLineCompStatePtr) compStateP)->prevLineP, 
													  (UInt16) srcBufLen, dstP,
													  !((ScanLineCompStatePtr) compStateP)->notFirstLine);
			
			// Update the state object for the ScrCompressScanLine function.
			((ScanLineCompStatePtr) compStateP)->prevLineP = srcP;
			((ScanLineCompStatePtr) compStateP)->notFirstLine = true;
			break;
			
		case BitmapCompressionTypeRLE:
			returnVal = ScrCompressRLE(srcP, srcBufLen, dstP, dstBufLen);
			break;
			
		case BitmapCompressionTypePackBits:		// compStateP[0] is source depth
			if (compStateP->data[0] == 16)
				returnVal = ScrCompressPackBits16((const UInt16*)srcP, srcBufLen>>1, dstP);
			else
				returnVal = ScrCompressPackBits(srcP, srcBufLen, dstP);
			break;
			
		default:
			return -1;
	}

	return returnVal;
}



/***********************************************************************
 *
 * FUNCTION:		ScrDecompress
 *
 * DESCRIPTION: 	Given a source buffer MemPtr and a dst buffer MemPtr, decompress
 *						the data using the method designated by compressionType.
 *
 * PARAMETERS:		compressionMethod
 *						srcP				->	MemPtr to the data to be uncompressed.
 *						srcBufLen		->	Size of compressed data buffer.
 *						dstP				->	MemPtr for uncompressed data.
 *						dstBufLen		->	Max space for uncompressed data.
 *						decompStateP	->	Extra variables needed by the specific decompression
 *												 method to itertively decompress data.
 *											<-	Extra state info needed next time the function is
 *												 called for decompressing data in small chunks.
 *
 * RETURNED:		# of bytes of compressed data used up. If less than zero, an error ocurred.
 *
 ***********************************************************************/
Int32 ScrDecompress(BitmapCompressionType compressionMethod, UInt8* srcP, UInt32 /*srcBufLen*/,
						  	UInt8* dstP, UInt32 dstBufLen, CompStateType* decompStateP)
{
	Int32 returnVal;

	switch (compressionMethod) {
		case BitmapCompressionTypeScanLine:
			returnVal = ScrDeCompressScanLine(srcP, dstP, (UInt16) dstBufLen);
			break;
			
		case BitmapCompressionTypeRLE:
			returnVal = ScrDecompressRLE(srcP, dstP, dstBufLen,
										 	&((RLEStatePtr) decompStateP)->prevByteCount,
											&((RLEStatePtr) decompStateP)->prevByte);
			break;
			
		case BitmapCompressionTypePackBits:		// compStateP[0] is source depth
			if (decompStateP->data[0] == 16)
				returnVal = ScrDecompressPackBits16(srcP, (UInt16*)dstP, dstBufLen>>1);
			else
				returnVal = ScrDecompressPackBits(srcP, dstP, dstBufLen);
			break;
						
		default:
			return -1;
	}

	return returnVal;
}
