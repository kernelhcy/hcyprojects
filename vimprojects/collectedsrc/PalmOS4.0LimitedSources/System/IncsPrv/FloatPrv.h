/******************************************************************************
 *
 * Copyright (c) 1994-1995 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: FloatPrv.h
 *
 * Release: 
 *
 * Description:
 *		Private Floating point header
 *
 * History:
 *   	3/9/95 - Created by Ron Marianetti
 *
 *****************************************************************************/

#ifdef 	NON_PORTABLE
#ifndef __FLOATPRV_H__
#define __FLOATPRV_H__


/************************************************************************
 * Structure of Floating point Library globals
 ***********************************************************************/
typedef struct {
	FloatType	tenPowers[200];				// 10^-99 to 10^99
	} FplGlobalsType;
typedef FplGlobalsType*	FplGlobalsPtr;
 
	


#endif //__FLOATPRV_H__
#endif // NON_PORTABLE
