/******************************************************************************
 *
 * Copyright (c) 1995-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: SysUtils.c
 *
 * Release: 
 *
 * Description:
 *  	  This file contains useful miscellaneous routines.
 *
 * History:
 *		April 26, 1995	Created by Roger Flores
 *	         11/06/98 Vivek Magotra 
 *
 *****************************************************************************/

#define	NON_PORTABLE

#include <PalmTypes.h>

// public system includes
#include <DataMgr.h>
#include <ErrorMgr.h>
#include <FeatureMgr.h>
#include <MemoryMgr.h>
#include <Rect.h>
#include <StringMgr.h>
#include <SystemMgr.h>
#include <SysUtils.h>

#include <unix_stdarg.h>

// private system includes
#include "Globals.h"
#include "SysEvtPrv.h"
#include "SystemResourcesPrv.h"
#include "SysTrapsFastPrv.h"
#include "UIResourcesPrv.h"





/************************************************************
 *
 *  FUNCTION: SysBinarySearch
 *
 *  DESCRIPTION: Search elements in an array according to the 
 *  passed comparison function.
 *
 *  PARAMETERS: base pointer to an array of elements, number of elements 
 *  in list, width of an element comparison function, other data passed to
 *  the comparison function, a pointer to the position result,
 *  findFirst should be set true to insure the first matching 
 *  element is returned (only needed if the array contains non unique data)
 *
 *  RETURNS: true if an exact match was found
 *				 a position in the database where the element should be located.
 *
 *  CREATED: 8/13/96
 *
 *  BY: Roger Flores
 *		Bob 1998-04-23: rewrite - crashed when array was empty, performance
 *		mchen 2001-01-08: add assertion that numOfElements is not negative
 *
 *************************************************************/

extern Boolean SysBinarySearch (void const * baseP, const Int16 numOfElements, const Int16 width, 
	SearchFuncPtr searchF, void const * searchData, const Int32 other, 
	Int32 * position, const Boolean /* findFirstNoLongerUsed */)
// note: findFirst (last param) is now ignored, algorithm always returns leftmost match 
{
	Int32 lPos = 0;
	Int32 rPos = numOfElements - 1;
	Int32 mPos;

	// The places in PalmOS that currently use this function are 
	// NotifyMgr, ExgMgr, OverlayMgr, SelTimeZone and unfortunately 
	// they all pass UInt16 values for numOfElements.  Luckily, they
	// don't currently pass values > 32767, but to be certain that
	// noone tries to call this with a negative value, we're adding
	// this nonfatal error.
	ErrNonFatalDisplayIf(numOfElements < 0, "SysBinarySearch argument error");

	// keep going until the pointers cross, that way
	// you get the index of smallest (leftmost) elt >= item
	// NOTE: (may return Length(array))
	while (lPos <= rPos)
	{
		mPos = (lPos + rPos) / 2;
		
		if (searchF(searchData, (UInt8 *)baseP+mPos*width, other) > 0) // mid value < search value
			lPos = mPos + 1;
		else
			rPos = mPos - 1;
	}
	*position = lPos;
	
	// if we've gone off the right end, don't do the final compare
	if (lPos >= numOfElements)
		return false;
	
	// otherwise, may not have checked current element for equality, do it now
	return (searchF(searchData, (UInt8 *)baseP+lPos*width, other) == 0);
}



/************************************************************
 *
 *  FUNCTION: PrvSysInsertionSortHelp
 *
 *  DESCRIPTION: Sorts elements in an array according to the 
 *  passed comparison function.  Only elements which are out of
 *  order move.  Moved elements are moved to the end of the 
 *  range of equal elements.  If a large amount of elements are
 *  being sorted try to use the quick sort.
 *
 *  This the insertion sort algorithm.  Starting with the second
 *  element, each element is compared to the preceeding element.
 *  Each element not greater than the last is inserted into 
 *  sorted position within those already sorted.  A binary insertion
 *  is performed.  A moved element is inserted after any other
 *  equal elements.
 *
 *  PARAMETERS: base pointer to an array of elements, number of elements 
 *  to sort (must be at least 2), width of an element comparison function, 
 *  other data passed to the comparison function.
 *
 *  RETURNS: a sorted portion of the database
 *
 *  CREATED: 11/30/95
 *
 *  BY: Roger Flores
 *
 *************************************************************/

// Keep separate from SysInsertionSortHelp so PrvSysQuickSortHelp
// doesn't have to pay a sys trap everytime is calls this routine.

static void PrvSysInsertionSortHelp (UInt8 * base, Int16 numOfElements, Int16 width, 
	const CmpFuncPtr comparF, const Int32 other)
{
	UInt8 *			pivotP;
	UInt8 * 			leftP;
	UInt8	*			rightP;
	UInt8 *			binaryLeft;
	UInt8 *			binaryMiddle;
	UInt8	*			binaryRight;
   Int16      		result;
   UInt8 *			tempCopyMemory;


	// Nearly identical code is used in DmPrvQSortHelp.  Make any modification to it too.
	// The variable names are kept similar to those in DmPrvQSortHelp so that
	// changes made to one can be easily made to the other.
	
	tempCopyMemory = MemPtrNew(width);
	ErrFatalDisplayIf(!tempCopyMemory, "Out of memory");
	
	
	leftP = base;
	pivotP = base + width;
	rightP = base + (UInt32) numOfElements * width;	
		// pivotP should never be used when >= rightP
		// Since rightP can be past the end of the DB
	
	// perform an insertion sort.  Scan from left to right.  For each item not
	// >= then the last binary insert it in place. O(n) = n log2 n comparisons but
	// still O(n) = n^2 data movements.
	while (pivotP < rightP)
		{
		// Is the next item >= then the last?
		if (comparF(leftP, pivotP, other) > 0) 
			{
			// Binary insert into place.
			binaryLeft = base;
			binaryRight = leftP;
			
			
			while (binaryLeft < binaryRight)
				{
				binaryMiddle = binaryLeft + ((UInt32) (((binaryRight - binaryLeft) / width)
					/ 2) * width);

				// Get and compare the middle record
				result = comparF(pivotP, binaryMiddle, other);
				
				if (result < 0)
					{
					binaryRight = binaryMiddle - width;
					}
				else
					{
					// result >= 0
					binaryLeft = binaryMiddle + width;
					}
				}
			
			// The binaryLeft and binaryRight have converged to a position.
			// This position may not have been tested.  The record to insert
			// might belong after this position.  Check for this.
			if (binaryLeft >= binaryRight)
				{
				binaryMiddle = binaryLeft;
				result = comparF(pivotP, binaryMiddle, other);
				if (result >= 0)
					binaryMiddle += width;		// insert after 
				}
			
			// Move the record from pivotP to binaryMiddle
			MemMove(tempCopyMemory, pivotP, width);		// save the element
			MemMove(binaryMiddle + width, binaryMiddle, pivotP - binaryMiddle);	// make room
			MemMove(binaryMiddle, tempCopyMemory, width);	// move element into place
			
			// Advance
			leftP = pivotP;
			pivotP += width;
			}
		else
			{
			// Advance
			leftP = pivotP;
			pivotP += width;
			}
		}

	MemPtrFree(tempCopyMemory);
}


/************************************************************
 *
 *  FUNCTION: SysInsertionSort
 *
 *  DESCRIPTION: Sorts elements in an array according to the 
 *  passed comparison function.  Only elements which are out of
 *  order move.  Moved elements are moved to the end of the 
 *  range of equal elements.  If a large amount of elements are
 *  being sorted try to use the quick sort.
 *
 *  This the insertion sort algorithm.  Starting with the second
 *  element, each element is compared to the preceeding element.
 *  Each element not greater than the last is inserted into 
 *  sorted position within those already sorted.  A binary insertion
 *  is performed.  A moved element is inserted after any other
 *  equal elements.
 *
 *  PARAMETERS: base pointer to an array of elements, number of elements 
 *  to sort (must be at least 2), width of an element comparison function, 
 *  other data passed to the comparison function.
 *
 *  RETURNS: a sorted portion of the database
 *
 *  CREATED: 11/30/95
 *
 *  BY: Roger Flores
 *
 *************************************************************/

void SysInsertionSort (void * baseP, Int16 numOfElements, Int16 width, 
	const CmpFuncPtr comparF, const Int32 other)
{
	if (width == 0 || numOfElements < 2)
		return;

	PrvSysInsertionSortHelp (baseP, numOfElements, width, comparF, other);
}


/***********************************************************************
 *
 * FUNCTION: 	 SysQSortExchange
 *
 * DESCRIPTION: Exchanges the elements pointed to
 *              by leftP and rightP.
 *
 * PARAMETERS:	 leftP  - pointer to bytes to exchange
 *              rightP - pointer to bytes to exchange
 *					 width - width of elements in bytes
 *					
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	4/26/95	Initial Revision
 *
 ***********************************************************************/
static void SysQSortExchange (void *leftP, void *rightP, Int16 width)
{
	int  i;
	char  c;
	char *c1, *c2;
	
	c1 = (char *) leftP;		// The compiler doesn't MemHandle void * well
	c2 = (char *) rightP;	// in expressions so we use these.
	
	for (i = 0; i < width; i++)
		{
			c = *c2;
			*c2++ = *c1;
			*c1++ = c;
		}
}


/************************************************************
 *
 *  FUNCTION: PrvSysQuickSortHelp
 *
 *  DESCRIPTION: Sorts elements in an array according to the 
 *  passed comparison function.  Remember that
 *  equal records can be in any position relative to each other
 *  because a quick sort tends to scramble the ordering of records.
 *  This also means that calling PrvSysQuickSortHelp multiple times
 *  can result in different orderings if the records are not 
 *  completely unique.  If you do not want this behavior you do
 *  not want a quick sort.  Try the insertion sort algorithm.
 *
 *  This the common quick sort algorithm.  The pivot point is picked
 *  by picking the middle of three records picked from around the 
 *  middle of the records to sort to take advantage of partially
 *  sorted data.  The routine contains it's own stack to limit
 *  uncontrolled recursion.  When the stack is full an insertion 
 *  sort is used instead because it doesn't require more stack 
 *  space.  An insertion sort is also used when the number of records
 *  is low.  This avoids the overhead of a quick sort which is
 *  noticeable for small numbers of records.  The last optimization
 *  is that if the records seem mostly sorted an insertion sort is 
 *  performed to move only those few records needing moving.
 *
 *  PARAMETERS: base pointer to an array of elements, number of elements 
 *  to sort (must be at least 2), width of an element comparison function, 
 *  other data passed to the comparison function.
 *
 *  RETURNS: a sorted portion of the database
 *
 *  CREATED: 11/30/95
 *
 *  BY: Roger Flores
 *
 *  MODIFIED : 11/06/98 - Vivek Magotra (moved memory allocation
 *						for temp copy out of critical loop)
 *************************************************************/
 
    		
typedef struct {
	UInt8 *	base;
	Int16		numOfElements;
} SortStackType;

#define stackSpace		20		// On avg. the mose used should be about 12 for a 512K card.

#define insertionSortThreshold	12



static void PrvSysQuickSortHelp (UInt8 * base, Int16 numOfElements, Int16 width, 
	const CmpFuncPtr comparF, Int32 other)
{
	UInt8 *			pivotP;
	UInt8 *			pivotLocation;
	UInt8 * 			leftP;
	UInt8 *			rightP;
	UInt8 *			tempP;
   UInt16			movedRecords;
	SortStackType	stack[stackSpace];
	SortStackType	*sp = stack;
   UInt8 *			tempCopyMemory;


	tempCopyMemory = MemPtrNew(width);
	ErrFatalDisplayIf(!tempCopyMemory, "Out of memory");
	
	while (true)
		{
		// base and numOfElements must be set for the range to sort
		
		
		// This is the first part of the quick sort algorithm.  It actually
		// is an insertion sort.  It is used when the number of elements is
		// low (quick sort has too much overhead) or when the function's stack
		// is out of space.
		if (numOfElements <= insertionSortThreshold || 
			sp >= &stack[stackSpace])
			{
doInsertionSort:
			PrvSysInsertionSortHelp (base, numOfElements, width, comparF, other);
			
			// We are done with portion of the data.  Check the stack for
			// another portion to sort.  If there isn't any then we're done.
			if (sp != stack)
				{
				sp--;
				base = sp->base;
				numOfElements = sp->numOfElements;
				}
			else
				break;							// done with the sort.
			}
		else
			{
			// This is the second part of the quick sort.  Here is the sorting
			// performed is the real quick sort algorithm.  A record is picked
			// close to the middle of the sort order and the records are
			// separated in half to those less and those greater than the pivot.
			// One half is placed on the stack for later sorting.  The algorithm 
			// loops and then sorts the other half.
			
			
			// A good deal of the data we sort is already partially sorted.  Try
			// to pick the pivot from around the middle of the data.
			pivotP = (UInt32) (numOfElements / 2) * width + base;
			leftP = pivotP - ((UInt32) (numOfElements / 16) | 1) * width;		// 1/16 offcenter or at least 1
			rightP = pivotP + ((UInt32) (numOfElements / 16) | 1) * width;		// 1/16 offcenter or at least 1
			
			// Make pivotP point to the middle most record of the three.
		   if (comparF(leftP, pivotP, other) > 0) 
				{
				tempP = leftP;
				leftP = pivotP;
				pivotP = tempP;
				}

		   if (comparF(pivotP, rightP, other) > 0) 
				{
				tempP = pivotP;
				pivotP = rightP;
				rightP = tempP;
				}

		   if (comparF(leftP, pivotP, other) > 0) 
				{
				tempP = leftP;
				leftP = pivotP;
				pivotP = tempP;
				}
			
			
			// Move the pivot to the beginning so that it isn't overrun during the
			// swapping of elements.
			SysQSortExchange (base, pivotP, width);
			pivotLocation = pivotP;
			pivotP = base;
			
			// Set the end points of the swap run
			leftP = base + width;
			rightP = base + ((UInt32) numOfElements - 1) * width;
			
			movedRecords = 0;
			
			// repetitively swap elements
			while (true)
				{
				// Scan from the left to the right for an element greater than the pivot
				while (comparF(leftP, pivotP, other) < 0)
					{
					leftP += width;
					}
				
				// Scan from the right to the left for an element less than the pivot
				while (comparF(pivotP, rightP, other) < 0)
					{
					rightP -= width;
					}
				
				// Check to see if leftP crossed rightP
				if (leftP > rightP)
					break;
				
				// Exchange the two records and scan again
				SysQSortExchange (leftP, rightP, width);
				leftP += width;
				rightP -= width;
				
				movedRecords++;
				}
				
			// Done moving all the records.  Restore the pivot.
			// If the records seem ordered take extra care
			if (movedRecords > (numOfElements >> 2) )
				{
				SysQSortExchange (pivotP, rightP, width);
				}
			else
				{
				// if pivot really was in middle (well sorted case)
				if (rightP == pivotLocation)
					{
					// Moves the first record back into place
					SysQSortExchange (pivotP, rightP, width);	// safe to do
					}
				else
					{
					// Move the record from pivotP to rightP.  The first
					// record isn't back in place - it's lost.  The move
					// does prevent a high position record from being swapped
					// from rightP to the first position.
	
					// Move the record from rightP to pivotP
					MemMove(tempCopyMemory, rightP, width);		// save the element
					MemMove(pivotP + width, pivotP, rightP - pivotP);	// make room
					MemMove(pivotP, tempCopyMemory, width);	// move element into place

					}
				}


			
			// Check if the data was mostly sorted.
			// We could avoid the goto but it means slowing down the algorithm.
			// If less than four records were swapped out of at least 32 then 
			// just move those records into place.  (least orderly case)
			if ( (movedRecords - 1) < (numOfElements >> 5) )
				{
				goto doInsertionSort;
				}
			else
				{
				// Now sort the smaller of the two halves and place the larger
				// one on the stack for later.  The comparison is based in widths.
				if ((rightP - base + width) > ((base + (UInt32) numOfElements * width) - leftP))
					{
					// the left half is larger
					sp->numOfElements = (Int16) ((rightP - base) / width + 1);
					sp->base = base;
					numOfElements -= (Int16) ((leftP - base) / width);
					base = leftP;
					}
				else
					{
					// the right half is larger
					sp->numOfElements = (Int16) (((base + (UInt32) numOfElements * width) - leftP) / width);
					sp->base = leftP;
					numOfElements = (Int16) ((rightP - base) / width + 1);
					}
				sp++;
				}
			}

		}
			
	MemPtrFree(tempCopyMemory);
	
	// exit
}


/************************************************************
 *
 *  FUNCTION: SysQSort
 *
 *  DESCRIPTION: Sorts elements in an array according to the 
 *  passed comparison function.  Remember that
 *  equal records can be in any position relative to each other
 *  because a quick sort tends to scramble the ordering of records.
 *  This also means that calling PrvSysQuickSortHelp multiple times
 *  can result in different orderings if the records are not 
 *  completely unique.  If you do not want this behavior you do
 *  not want a quick sort.  Try the insertion sort algorithm.
 *
 *  This the common quick sort algorithm.  The pivot point is picked
 *  by picking the middle of three records picked from around the 
 *  middle of the records to sort to take advantage of partially
 *  sorted data.  The routine contains it's own stack to limit
 *  uncontrolled recursion.  When the stack is full an insertion 
 *  sort is used instead because it doesn't require more stack 
 *  space.  An insertion sort is also used when the number of records
 *  is low.  This avoids the overhead of a quick sort which is
 *  noticeable for small numbers of records.  The last optimization
 *  is that if the records seem mostly sorted an insertion sort is 
 *  performed to move only those few records needing moving.
 *
 *  PARAMETERS: base pointer to an array of elements, number of elements 
 *  to sort (must be at least 2), width of an element comparison function, 
 *  other data passed to the comparison function.
 *
 *  RETURNS: a sorted portion of the database
 *
 *  CREATED: 11/30/95
 *
 *  BY: Roger Flores
 *
 *************************************************************/
void SysQSort (void * baseP, Int16 numOfElements, Int16 width, 
	const CmpFuncPtr comparF, const Int32 other)
{
	if (width == 0 || numOfElements < 2)
		return;

	PrvSysQuickSortHelp (baseP, numOfElements, width, comparF, other);
}


/***********************************************************************
 *
 * FUNCTION:    SysCopyStringResource
 *
 * DESCRIPTION: Copies a resource string to a passed string.
 *
 * PARAMETERS:  string - string to copy the resource string to
 *              theID - the resource string ID
 *
 * RETURNED:    string 
 *
 * HISTORY:
 *		07/19/95	rsf	Created by Roger Flores.
 *		11/15/00	kwk	Check for NULL string param, and added missing
 *							DmReleaseResource call.
 *
 ***********************************************************************/
void SysCopyStringResource(Char * string, Int16 theID)
{
	Char* stringP;
	MemHandle stringH;

	ErrNonFatalDisplayIf(string == NULL, "NULL string parameter");
	
	stringH = DmGetResource (strRsc, theID);
	ErrFatalDisplayIf(!stringH, "String resource not found.");
	
	stringP = MemHandleLock(stringH);
	StrCopy(string, stringP);
	MemHandleUnlock(stringH);
	DmReleaseResource(stringH);
}


/***********************************************************************
 *
 * FUNCTION: SysFormPointerArrayToStrings	 
 *
 * DESCRIPTION:	Forms an array of pointers to strings in a block.
 * 					Useful for setting the items of a list.
 *
 * PARAMETERS:		c - pointer to packed block of strings
 *						stringCount - count of strings in block
 *
 * RETURNS:			Unlocked MemHandle to allocated inorder array of pointers
 *						to the strings in the passed block 
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/2/95	Initial Revision
 *
 ***********************************************************************/

MemHandle SysFormPointerArrayToStrings(Char * c, Int16 stringCount)
{
	MemHandle newArrayH;
	Char **newArrayP;
	Int16 i;
	
	newArrayH = MemHandleNew(stringCount * sizeof(Char *));
	ErrNonFatalDisplayIf(!newArrayH, "alloc failed");
	
	if (newArrayH)
	{
		newArrayP = (Char **) MemHandleLock(newArrayH);
		
		for (i=0; i < stringCount; i++)
			{
			newArrayP[i] = c;
			while (*c++)
				;
			}
			
		MemHandleUnlock(newArrayH);
	}
	
	return newArrayH;
}
	
	
/***********************************************************************
 *
 * FUNCTION: SysRandom	 
 *
 * DESCRIPTION:	Returns a random number anywhere from 0 to sysRandomMax.
 *						
 *
 * PARAMETERS:		
 *		newSeed		- new seed value, or 0 to use existing seed
 *
 * RETURNS:			Random number
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			ron	6/2/95	Initial Revision  Random
 *
 ***********************************************************************/
Int16		SysRandom(Int32 newSeed)
{
	
	// Set the new seed, if passed
	if (newSeed) GSysRandomSeed = newSeed;
	
	// Generate the new seed
	GSysRandomSeed = (0x015A4E35L * GSysRandomSeed) + 1;
	
	// Return the random number
	return (Int16) ((GSysRandomSeed >> 16) & 0x7FFF);

}



/************************************************************
 *
 *  FUNCTION: SysStringByIndex
 *
 *  DESCRIPTION: Copies a string out of a string list resource by index. 
 *		String list resources are of type 'tSTL' and contain a list
 *		of strings and a prefix string. 
 *
 *		The returned string from this call will be the prefix string 
 *		appended with the designated index string. Indices are 0 based,
 *		index 0 is the first string in the resource. 
 *
 *    WARNING: ResEdit always displays the items in the list as starting
 *		at 1, not 0. So, be sure to take this into account when creating
 *		your string list.
 *
 *  PARAMETERS: 
 *			resID		- the resource id of the string list
 *			index		- which string to get out of the list
 *			strP 		- pointer to space to form the string
 *			maxLen	- size of strP buffer.
 *
 *  RETURNS: strP
 *
 *  CREATED: 8/13/96
 *
 *  BY: Ron Marianetti.
 *
 *************************************************************/
Char * SysStringByIndex(UInt16 resID, UInt16 index, Char * strP, UInt16 maxLen)
{
	MemHandle	resH=0;
	UInt16	numStrings;
	Char *	srcP;
	UInt8		* resP=0;
	UInt16	i;
	UInt16	len;
	
	
	// Assume err
	if (!maxLen || !strP) return strP;
	strP[0] = 0;

	// Get the resource
	resH = DmGetResource(sysResTErrStrings, resID);
	
	// If not found, return
	if (!resH) goto Exit;
		
		
	//---------------------------------------------------------------
	// Copy the prefix string over.
	//---------------------------------------------------------------
	resP = MemHandleLock(resH);
	srcP = (Char *)resP;
	StrNCat(strP, srcP, maxLen);
	len = StrLen(strP);
	srcP += len+1;
	
	
	
	//---------------------------------------------------------------
	// Catenate the string for this index
	//---------------------------------------------------------------
	// Get the number of entries.
	numStrings = *srcP++;
	numStrings = (numStrings << 8) +  *srcP++;
	
	// If this index not in the resource, exit err
	if (index >= numStrings) goto Exit;
	
	// Search for this string pointer
	for (i=0; i<index; i++) 
		srcP += StrLen(srcP) + 1;
		
	// Catenate it.
	StrNCat(strP, srcP, maxLen);
	
	
Exit:
	if (resP) MemPtrUnlock(resP);
	if (resH) DmReleaseResource(resH);
	return strP;

}



/************************************************************
 *
 *  FUNCTION: SysErrString
 *
 *  DESCRIPTION: Returns text to describe an error number. This
 *		routine looks up the textual description of a system error
 *		number in the appropriate 'tSTL' resource and creates a string
 *		that can be used to display that error. 
 *
 *		The actual string will be of the form: "<error message> (XXXX)"
 *		where XXXX is the hexadecimal error number. 
 *
 *		This routine looks for a resource of type 'tstl' and resource ID
 *		of (err>>8). It then grabs the string at index (err & 0x00FF) out
 *		of that resource. Note: The first string in the resource is called
 *		index #1 by ResEdit, NOT #0. For example, an error code of 0x0101 will
 *		fetch the first string in the resource. 
 *		
 *  PARAMETERS: 
 *			err 		- the error number
 *			strP 		- pointer to space to form the string
 *			maxLen	- size of strP buffer.
 *
 *  RETURNS: strP
 *
 *  CREATED: 8/13/96
 *
 *  BY: Ron Marianetti.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		----------- 
 *			vmk	12/13/96	Masked out high byte when computing resource id
 *			rm		2/25/98  Changed format of error hex string that's appended 
 *									to end of message to be "0xHHHH"
 *************************************************************/
Char * SysErrString(Err err, Char * strP, UInt16 maxLen)
{
	UInt16	resID;
	UInt16	index;
	Char *	dstP = strP;
	
	
	// Error Check
	if (!maxLen || !strP) return strP;

	// Calculate the resource ID and index
	// (need to mask out high byte because 'err' is a signed quantity,
	// and if the most significant bit is set, the arithmetic shift right will
	// propagate the 1 bit through all the vacated bit positions)
	resID = sysResIDErrStrings + ((err >> 8) & 0x00FF);
	index = err & 0x00FF;
	
	
	// Get the string
	dstP =  SysStringByIndex( resID,  index,  strP,  maxLen);
	if (!dstP) {
		strP[0] = 0;
		dstP = strP;
		}
		
	// If room, add the hex value
	if (StrLen(strP) + 9 < maxLen) {
		Char	hexStr[10];
		StrIToH(hexStr, err);
		StrCat(strP, " (0x");
		StrCat(strP, hexStr+4);
		StrCat(strP, ")");
		}

	return strP;
}


/************************************************************
 *
 *  FUNCTION: SysGremlins
 *
 *  DESCRIPTION: Various ways to interact with Gremlins.
 *
 *		This function used to be called SysGremlins() and the only parameter
 *		allowed was "GremlinIsOn".  The old call will still work, but in general
 *		this function has changed quite a bit.
 *		- First, THIS FUNCTION SHOULD NOT BE CALLED DIRECTLY.  You should use the
 *		various Emu* calls in EmuTraps.h because they work for Poser, the device,
 *		and the simulator, and they are safer because of the type checking.
 *		- Second, this function has been changed to a variable-argument function
 *		to match the way Poser patches this call.
 *
 *		The three current environments are handled as follows:
 *			Emulator (Poser) - Emulator patches trap and handles calls.
 *			Simulator - This function should not be called except for w/ GremlinIsOn.
 *			Device - This function implements any selectors.
 *
 *  PARAMETERS: 
 *			selector	- the function to perform
 *
 *  RETURNS: various values depending on the selector.  The values will be returned
 *				 in both A0 and D0 so that clients who think this function returns a
 *				 pointer will work.
 *
 *	 REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		----------- 
 *			roger	2/6/98	Initial version
 *			dia	08/03/98	Now handles GremlinsIsOn case.
 *       dia	08/20/98 New function prototype to be compatible with
 *								Poser hack and old style.  Uses var args now.
 *************************************************************/

#if EMULATION_LEVEL != EMULATION_NONE
#include "EmuStubs.h"
#endif

// Quick little function that should cause the given value to go to A0.
static void* PrvMoveToA0 (UInt32 value) { return (void*) value; }


UInt32 HostControl(HostControlTrapNumber selector, ...)
{
#if EMULATION_LEVEL != EMULATION_NONE
	ErrFatalDisplayIf (selector >= hostSelectorGetHostVersion && selector != hostSelectorGremlinIsRunning,
							 "Please use the Emu* function listed in EmuTraps.h");
	return HostGremlinIsRunning();
#else
	const SysEvtMgrGlobalsPtr gP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
	UInt32 result = 0;
	va_list args;

	va_start (args, selector);

	// First MemHandle the old way this routine was called, then switch on the selector
	if (selector < hostSelectorGetHostVersion)
		result = (gP->gremlinsFlags & grmGremlinsOn);
	else
		{
		switch (selector)
			{
			case hostSelectorGetHostVersion:
				FtrGet(sysFtrCreator, sysFtrNumROMVersion, &result);
				break;
			case hostSelectorGetHostID:
				result = hostIDPalmOS;
				break;
			case hostSelectorGetHostPlatform:
				result = hostPlatformPalmOS;
				break;
			case hostSelectorIsSelectorImplemented:
				{
				// TBD: better method of doing this (if we ever implement more).  See EmControl.c?
				const Int32 sel = va_arg (args, Int32);

				result = (sel < hostSelectorGetHostVersion ||
							 sel >= hostSelectorGetHostVersion && sel <= hostSelectorGestalt ||
							 sel == hostSelectorGremlinIsRunning);
				break;
				}
			case hostSelectorGestalt:
				result = hostErrUnknownGestaltSelector;
				break;
				
			case hostSelectorGremlinIsRunning:
				result = (gP->gremlinsFlags & grmGremlinsOn);
				break;
				
			default:
				result = 0;
				break;
			}
		}
		
	va_end (args)
	
	// If this function was called through a macro with a pointer return value
	// (such as EmuFOpen), the caller will be expecting the result in A0.
	PrvMoveToA0 (result);

	return result;
#endif // EMULATION_LEVEL != EMULATION_NONE else
}
