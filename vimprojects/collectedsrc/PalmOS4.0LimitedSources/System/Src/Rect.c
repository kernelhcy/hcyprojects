/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: Rect.c
 *
 * Release: 
 *
 * Description:
 *	  This file contains the routines that manipulate rectangles.
 *
 * History:
 *		Dec 14, 1994	Created by Art Lamb
 *			Name	Date		Description
 *			----	----		-----------
 *	  		bob	2/9/99	Use Coord abstraction, fix up consts
 *
 *****************************************************************************/

/* Routines:
 *		RctSetRectangle
 *		RctCopyRectangle
 *		RctInsetRectangle
 *		RctOffsetRectangle
 *		RctPtInRectangle
 */

#include <PalmTypes.h>

#include <PalmUtils.h>

#include "Rect.h"

/***********************************************************************
 *
 * FUNCTION:    RctSetRectangle
 *
 * DESCRIPTION: Set a RectangeType structure to the bound passed.
 *
 * PARAMETERS:  rP			pointer to a RectangleType structure
 *              left		left bound of the rectangle
 *              top		bound of the rectangle
 *              width	width of the rectangle
 *              height	height of the rectangle
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/14/94		Initial Revision
 *			trev	08/11/97	made non modified passed variables constant
 *
 ***********************************************************************/
void RctSetRectangle (RectangleType * rP, Coord left, Coord top, Coord width, Coord height)
{
	rP->topLeft.x = left;
	rP->topLeft.y = top;
	rP->extent.x = width;
	rP->extent.y = height;
}


/***********************************************************************
 *
 * FUNCTION:    RctCopyRectangle
 *
 * DESCRIPTION: Copy a rectangle structure.
 *
 * PARAMETERS:  srcRect  pointer to source rectangle
 *              dstRect  pointer to destination rectangle
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/14/94		Initial Revision
 *			trev	08/11/97	made non modified passed variables constant
 *
 ***********************************************************************/
void RctCopyRectangle (const RectangleType * srcRect, RectangleType * dstRect)
{
	*dstRect = *srcRect;
}


/***********************************************************************
 *
 * FUNCTION:    RctInsetRectangle
 *
 * DESCRIPTION: This routine moves all sides of a rectangle in or out
 *              an equal distance.  Pass a negative value, in insetAmt
 *              to move all size of the rectangle out.
 *
 * PARAMETERS:  rP			  pointer to a RectangleType structure.
 *              insetAmt  number of pixels by which to move all side
 *                        of the rectangle.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/14/94		Initial Revision
 *			trev	08/11/97	made non modified passed variables constant
 *
 ***********************************************************************/
void RctInsetRectangle (RectangleType * rP, Int16 insetAmt)
{
	rP->topLeft.x += insetAmt;
	rP->topLeft.y += insetAmt;

	insetAmt += insetAmt;

	rP->extent.x -= insetAmt;
	rP->extent.y -= insetAmt;
}


/***********************************************************************
 *
 * FUNCTION:    RctOffsetRectangle
 *
 * DESCRIPTION: This routine moves a rectangle a specified amount by
 *              changing coordinates in the RectangleType structure.
 *
 * PARAMETERS:  rP		 pointer to a RectangleType structure
 *              deltaX   amount to adjust x coordinate, in pixels
 *              deltaX   amount to adjust y coordinate, in pixels
 *
 * RETURNED:   nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/14/94		Initial Revision
 *			trev	08/11/97	made non modified passed variables constant
 *
 ***********************************************************************/
void RctOffsetRectangle (RectangleType * rP, Coord deltaX, Coord deltaY)
{
	rP->topLeft.x += deltaX;
	rP->topLeft.y += deltaY;
}



/***********************************************************************
 *
 * FUNCTION:    RctPtInRectangle
 *
 * DESCRIPTION: This routine return TRUE is the coordinate passed is
 *              within the rectangle passed.
 *
 * PARAMETERS:  rP	   pointer to a RectangleType structure
 *              x    coordinate 
 *              y    coordinate 
 *
 * RETURNED:    TRUE if the coordinate is in the rectangle
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/14/94		Initial Revision
 *			trev	08/11/97	made non modified passed variables constant
 *
 ***********************************************************************/
Boolean RctPtInRectangle (Coord x, Coord y, const RectangleType * rP)
{
	return (x >= rP->topLeft.x &&
			  y >= rP->topLeft.y &&
			  x < rP->topLeft.x + rP->extent.x &&
			  y < rP->topLeft.y + rP->extent.y);
}


/***********************************************************************
 *
 * FUNCTION:    RctGetIntersection
 *
 * DESCRIPTION: This routine returns the intersection of two rectangles.
 *
 * PARAMETERS:  r1P	 	pointer to a RectangleType structure
 *              r2P   	pointer to a RectangleType structure 
 *              resultP intersection of r1P and r2P (returned)
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	10/2/94	Initial Revision
 *			bob	3/8/99	Don't set top/left to 0,0 if zero extent rectangle
 *
 ***********************************************************************/
void RctGetIntersection (const RectangleType * r1P, const RectangleType * r2P, 
	RectangleType * resultP)
{
	Coord left, right, top, bottom;
	
	left = max (r1P->topLeft.x, r2P->topLeft.x);
	top = max (r1P->topLeft.y, r2P->topLeft.y);
	right = min (r1P->topLeft.x + r1P->extent.x, r2P->topLeft.x + r2P->extent.x);
	bottom = min (r1P->topLeft.y + r1P->extent.y, r2P->topLeft.y + r2P->extent.y);
	
	resultP->topLeft.x = left;
	resultP->topLeft.y = top;

	right -= left;
	resultP->extent.x = (right < 0) ? 0 : right;

	bottom -= top;
	resultP->extent.y = (bottom < 0) ? 0 : bottom;
}
