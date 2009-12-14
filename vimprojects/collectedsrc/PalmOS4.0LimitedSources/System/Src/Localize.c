/******************************************************************************
 *
 * Copyright (c) 1996-1999 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: Localize.c
 *
 * Release: 
 *
 * Description:
 *		Functions to localize data
 *
 * History:
 *   	8/28/96  Roger - Initial version
 *
 *****************************************************************************/

#include <PalmTypes.h>

#include "Localize.h"

/************************************************************
 *
 *  FUNCTION: LocGetNumberSeparators
 *
 *  DESCRIPTION: Get localized number separators.
 *
 *  PARAMETERS: numberFormat - the format to use
 *					 thousandSeparator - return a localized thousand separator here
 *					 decimalSeparator - return a localized decimal separator here
 *
 *  RETURNS: nothing
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	8/28/96	Initial version
 *			scl	10/16/96	Fixed decimalSeparator of nfSpaceComma. Was '.'.
 *
 *************************************************************/
void LocGetNumberSeparators(NumberFormatType numberFormat, 
	Char *thousandSeparator, Char *decimalSeparator)
{
	switch (numberFormat)
		{
		// US format which is also used internally.  If a program requests a
		// format not understood use US.
		case nfCommaPeriod:
		default:
			*thousandSeparator = ',';
			*decimalSeparator = '.';
			break;
		
		case nfPeriodComma:
			*thousandSeparator = '.';
			*decimalSeparator = ',';
			break;
		
		case nfSpaceComma:
			*thousandSeparator = ' ';
			*decimalSeparator = ',';
			break;
		
		case nfApostrophePeriod:
			*thousandSeparator = '\'';
			*decimalSeparator = '.';
			break;
		
		case nfApostropheComma:
			*thousandSeparator = '\'';
			*decimalSeparator = ',';
			break;
		}
}
