/******************************************************************************
 *
 * Copyright (c) 1998-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: TextPrv.h
 *
 * Release: 
 *
 * Description:
 *	Private header file for Text Manager.
 *
 * Written by TransPac Software, Inc.
 *
 * History:
 *	07/13/99	kwk	Created by Ken Krugler.
 *	05/13/00	kwk	Added kTxtMaxAlwaysSingleByte & kTxtMaxNeverMultiByte.
 *	07/27/00	kwk	Added TxtConvertEncodingV35 routine declaration.
 *
 *****************************************************************************/

#ifndef __TEXTPRV_H__
#define __TEXTPRV_H__

#include <IntlMgr.h>

// Maximum WChar value that fits in a single byte. With UTF-8,
// characters >= 0x80 require two or three bytes to encode in a string.
#define	kTxtMaxAlwaysSingleByte		0x007F

// Maximum byte value that is never part of a multi-byte character,
// regardless of the device's character encoding.
#define	kTxtMaxNeverMultiByte		0x3F

/***********************************************************************
 * Private routines
 ***********************************************************************/

#ifdef __cplusplus
	extern "C" {
#endif

// Convert the characters in <inSrcP> into an appropriate form for searching,
// and copy up to <inDstSize> bytes of converted characters into <outDstP>. The
// resulting string will be null-terminated. We assume that <inDstSize> includes
// the space required for the null. Return back the number of bytes consumed in
// <inSrcP>. Note that this routine returned nothing (void) in versions of
// the OS previous to 3.5 (and didn't exist before Palm OS 3.1), and that it
// used to not take an <inSrcLen> parameter.

UInt16 TxtPrepFindString(const Char* inSrcP, UInt16 inSrcLen, Char* outDstP, UInt16 inDstSize)
		INTL_TRAP(intlTxtPrepFindString);

// Backwards-compatible API for routine that was only used by Sony's extension.
Err TxtConvertEncodingV35(	const Char* srcTextP,
							UInt16* ioSrcBytes,
							CharEncodingType srcEncoding,
							Char* dstTextP,
							UInt16* ioDstBytes,
							CharEncodingType dstEncoding)
		INTL_TRAP(intlTxtConvertEncodingV35);

#ifdef __cplusplus
	}
#endif

#endif
