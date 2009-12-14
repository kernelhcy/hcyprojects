/******************************************************************************
 *
 * Copyright (c) 1995 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: UIInit.h
 *
 * Release: 
 *
 * Description:
 *        This file defines UI initialization routines.
 *
 * History:
 *		Jan 25, 1995	Created by Art Lamb
 *
 *****************************************************************************/

#ifndef __INIT_H__
#define __INIT_H__

#include <PalmTypes.h>
#include <CoreTraps.h>


#ifdef __cplusplus
extern "C" {
#endif

extern void UIInitialize (void)
							SYS_TRAP(sysTrapUIInitialize);

extern void UIReset (void)
							SYS_TRAP(sysTrapUIReset);

#ifdef __cplusplus 
}
#endif


#endif //__INIT_H__
