/******************************************************************************
 *
 * Copyright (c) 1999 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: MenuPrv.h
 *
 * Release: 
 *
 * Description:
 * 	This file defines structs and functions for the Menu Manager
 *		that are private to Palm OS but are not private to the
 *		Menu Manager itself -- that is, we don't want developers
 *		using these things, but Managers other than the Menu Manager
 *		can use them.
 *
 * History:
 *			Name	Date		Description
 *			----	----		-----------
 *			jmp	9/22/99	Initial Revision
 *
 *****************************************************************************/

#ifndef __MENUPRV_H__
#define __MENUPRV_H__

#include <PalmTypes.h>
#include <CoreTraps.h>

#include "Menu.h"

// For use with the MenuEraseMenu() call.  
#define removeCompleteMenu 		true
#define dontRemoveCompleteMenu	false

#ifdef __cplusplus
extern "C" {
#endif

extern void MenuEraseMenu (MenuBarType * menuP, Boolean removingCompleteMenu)
							SYS_TRAP(sysTrapMenuEraseMenu);

#ifdef __cplusplus 
}
#endif
#endif //__MENUPRV_H__
