/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: HSUtil.h
 *
 * Release: 
 *
 * Description:
 *		HotSync utility definitions.
 *
 *		Platform-independence data conversion macros.
 *
 * History:
 *   	7/19/96  vmk	Created by Vitaly Marty Kruglikov
 *
 *****************************************************************************/

#ifndef __HS_UTIL_H__
#define __HS_UTIL_H__

//-----------------------------------------------------------
// Platform-independence macros
//-----------------------------------------------------------

#define	HSUtilReverseWord(__theWord__)						\
	(	(UInt16)															\
		(																	\
		(((UInt16)(__theWord__) << 8) & 0xFF00) |				\
		(((UInt16)(__theWord__) >> 8) & 0x00FF)					\
		)																	\
	)

#define	HSUtilReverseDWord(__theDWord__)						\
	(	(UInt32)															\
		(																	\
		(((UInt32)(__theDWord__) << 24) & 0xFF000000L) |		\
		(((UInt32)(__theDWord__) << 8) & 0x00FF0000L) |		\
		(((UInt32)(__theDWord__) >> 8) & 0x0000FF00L) |		\
		(((UInt32)(__theDWord__) >> 24) & 0x000000FFL)		\
		)																	\
	)

#if defined(CPU_ENDIAN) && (CPU_ENDIAN == CPU_ENDIAN_LITTLE)
	#define	HSUtilPilotToHostWord(__theWord__)				\
			HSUtilReverseWord(__theWord__)
	
	#define	HSUtilPilotToHostDWord(__theDWord__)			\
			HSUtilReverseDWord(__theDWord__)
	
	#define	HSUtilHostToPilotWord(__theWord__)				\
			HSUtilReverseWord(__theWord__)
	
	#define	HSUtilHostToPilotDWord(__theDWord__)			\
			HSUtilReverseDWord(__theDWord__)
			
#else  // CPU_ENDIAN == CPU_ENDIAN_LITTLE

	#define	HSUtilPilotToHostWord(__theWord__)				\
			(__theWord__)
	
	#define	HSUtilPilotToHostDWord(__theDWord__)			\
			(__theDWord__)
	
	#define	HSUtilHostToPilotWord(__theWord__)				\
			(__theWord__)
	
	#define	HSUtilHostToPilotDWord(__theDWord__)			\
			(__theDWord__)
#endif // CPU_ENDIAN == CPU_ENDIAN_LITTLE

#endif // __HS_UTIL_H__
