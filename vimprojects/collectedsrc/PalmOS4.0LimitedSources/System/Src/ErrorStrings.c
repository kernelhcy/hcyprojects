/******************************************************************************
 *
 * Copyright (c) 1995-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: ErrorStrings.c
 *
 * Release: 
 *
 * Description:
 *		Routine for mapping Pilot system errors to descriptive strings.
 *
 * History:
 *   	7/6/95  VMK - Created by Vitaly Kruglikov from DSerial.cp
 *
 *****************************************************************************/

#if defined(_WIN32)	// Windows build of PilotDebugger
	#include <stdafx.h>
#endif

#include <stdio.h>						// for sprintf(...)

#include "ErrorBase.h"

// Misc comm managers
#include "SerialMgr.h"					// for serial manager error constants
#include "SerialLinkMgr.h"				// for serial link error constants


/*********************************************************************************
 * FUNCTION: ErrSysErrorText
 *
 * Returns an error string corresponding to the passed system error code.
 *
 * Arguments:
 *		Err err	-- error value
 *
 * Returns:
 *		MemPtr to string corresponding to the error value
 *
 *********************************************************************************/

#ifdef __cplusplus
extern "C"
#endif
const Char * ErrSysErrorText(Err err);

const Char * ErrSysErrorText(Err err)
{
	const Char *	msgP;
	
	static	char 	errBuf[256];
	
	switch ( err )
		{
		case 0:
			msgP = "No error.";
			break;
		
		// Serial Manager errors
		case serErrBadParam:
			msgP = "Invalid parameter.";
			break;
		case serErrBadPort:
			msgP = "Could not open port.";
			break;
		case serErrNoMem:
			msgP = "Out of memory.";
			break;
		case serErrBadConnID:
			msgP = "Invalid serial reference number.";
			break;
		case serErrTimeOut:
			msgP = "timeout.";
			break;
		case serErrLineErr:
			msgP = "Serial line error.";
			break;
		
		// Serial Link errors
		case slkErrChecksum:
			msgP = "invalid packet CRC.";
			break;
		case slkErrFormat:
			msgP = "Invalid packet format.";
			break;
		case slkErrBuffer:
			msgP = "Buffer is too small.";
			break;
		case slkErrTimeOut:
			msgP = "Serial Link timeout.";
			break;
		case slkErrBodyLimit:
			msgP = "Packet body size exceeds limit.";
			break;
		case slkErrTransId:
			msgP = "Unexpected transaction id.";
			break;
		default:
			sprintf(errBuf, "error code: %04X", err);
			msgP = errBuf;
			break;
		}
		
	return( msgP );
}
