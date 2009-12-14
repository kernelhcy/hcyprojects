/******************************************************************************
 *
 * Copyright (c) 1995-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: UIResource.c
 *
 * Release: 
 *
 * Description:
 *	  This file contains routines that convert Macintosh resources to
 *   Pilot resources.
 *
 * History:
 *		January 29, 1995	Created by Art Lamb
 *
 *****************************************************************************/

/* Routines:
 *     ResLoadForm
 *     ResLoadMenu
 */

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_FORMS
#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS
#define ALLOW_ACCESS_TO_INTERNALS_OF_CONTROLS
#define ALLOW_ACCESS_TO_INTERNALS_OF_LISTS
#define ALLOW_ACCESS_TO_INTERNALS_OF_FIELDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_TABLES
#define ALLOW_ACCESS_TO_INTERNALS_OF_MENUS

#include <PalmTypes.h>
#include <ErrorMgr.h>
#include <StringMgr.h>

#include <Form.h>
#include <Menu.h>
#include "UIResourcesPrv.h"



/***********************************************************************
 *
 * FUNCTION: 	 ResLoadForm
 *
 * DESCRIPTION: This routine copies and initializes a form resource.  The
 *					 structures are complete except pointers updating.
 *					 Pointers are stored as offsets from the beginning of the
 *					 form.
 *
 * PARAMETERS:	 rscID	the resource id of the form.
 *					
 * RETURNED:	 The handle of the memory block that the form is in,  since 
 *              the form structure begins with the WindowType structure this
 *              is also a WindowHandle.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/5/95	Initial Revision
 *			art	2/19/96	Added scroll bar
 *			gap	7/27/00	Added support for usable flag for tables.  Setting value
 *								of usable attr to 1 here for backwards compatibility.
 *
 ***********************************************************************/
void * ResLoadForm (UInt16 rscID)
{
	MemHandle					formROMHandle;		// form in ROM
	MemHandle					formHandle;			// copy of form
	FormType 				*frm, *frmROM;
	Int16						i, j;
	char *					ptr;
	UInt16					frmSize;
	

	// The form is in ROM.  We want to write to it to set its pointers.
	// Copy it to RAM so we can do this.
	formROMHandle = DmGetResource(formRscType, rscID);
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	if (formROMHandle == NULL)
		{
		char message[60];
		
		StrPrintF(message, "Resource for %s form %hu not found", 
			rscID >= 10000 ? "system" : "app",
			rscID);
		ErrDisplay(message);
		}
#endif
	
		
	frmROM = (FormType *) MemHandleLock (formROMHandle);
	frmSize = (UInt16) MemPtrSize(frmROM);
	
	formHandle = MemHandleNew(frmSize);
	ErrNonFatalDisplayIf(!formHandle, "Out of dynamic memory");
	frm = (FormType *) MemHandleLock (formHandle);

	MemMove(frm, frmROM, frmSize);
	MemHandleUnlock (formROMHandle);
	
	
	// Update all of the pointers within the form object
	frm->objects = (FormObjListType *) ((char *) frm + sizeof (FormType));


	// Loop through each object used by the form and update each one.
	for (i = 0; i < frm->numObjects; i++)
		{
		frm->objects[i].object.ptr = ((char *) frm) + (Int16) frm->objects[i].object.ptr;
		switch (frm->objects[i].objectType)
			{
			case frmListObj:
				// The array of list items needs to be updated.  The format in the
				// resource of the list is the ListType structure followed
				// by a ragged array of list item strings.  The array of pointers
				// is stored before the sequence of item strings.
				
				// Update itemsText to point to the ragged array of list items.
				frm->objects[i].object.list->itemsText = (char **) 
					(frm->objects[i].object.list + 1);
					
				// Now build the array of pointers by walking through the strings
				ptr = (Char *) frm->objects[i].object.list + sizeof (ListType) + 
					frm->objects[i].object.list->numItems * sizeof (Char *);
				for (j = 0; j < frm->objects[i].object.list->numItems; j++)
					{
					frm->objects[i].object.list->itemsText[j] = ptr;
					ptr = ptr + (StrLen(ptr) + 1);
					}
				break;

			case frmControlObj:
				// Update textual controls to point to the label provided.
				if (!frm->objects[i].object.control->attr.graphical)
					frm->objects[i].object.control->text = (Char *) frm->objects[i].object.control + 
						sizeof (ControlType);
				break;

			case frmFieldObj:
				// have to make sure this uninitialized (pre-4.0) value is zero
				// so dynamic sized fields work properly
				frm->objects[i].object.field->maxVisibleLines = 0;
				break;


			case frmTableObj:
				ptr = (Char *) frm->objects[i].object.table;
				frm->objects[i].object.table->attr.usable = true;
				frm->objects[i].object.table->columnAttrs = (TableColumnAttrType *) (ptr + 
					sizeof (TableType));

				frm->objects[i].object.table->rowAttrs = (TableRowAttrType *) (ptr + 
					sizeof (TableType) +
					sizeof (TableColumnAttrType) * frm->objects[i].object.table->numColumns);
		
				frm->objects[i].object.table->items = (TableItemPtr) (ptr + 
					sizeof (TableType) +
					sizeof (TableColumnAttrType) * frm->objects[i].object.table->numColumns +
					sizeof (TableRowAttrType) * frm->objects[i].object.table->numRows); 
	
				break;

			case frmPopupObj:
				break;

			case frmTitleObj:
				frm->objects[i].object.title->text = (Char *) frm->objects[i].object.title + 
					sizeof (FormTitleType);
				break;

			case frmLabelObj:
				frm->objects[i].object.label->text = (Char *) frm->objects[i].object.label + 
					sizeof (FormLabelType);
				break;

			case frmBitmapObj:
				break;

			case frmGraffitiStateObj:
				break;

			case frmGadgetObj:
				break;

			case frmScrollBarObj:
				break;

			default:
				ErrDisplay ("Unknown rsrc type");
			}
		}
	
	return (frm);
}

/***********************************************************************
 *
 * FUNCTION: 	 ResLoadMenu
 *
 * DESCRIPTION: This routine copies and initializes a menu resource.  The
 *					 structures are complete except pointers updating.
 *					 Pointers are stored as offsets from the beginning of the
 *					 menu.
 *
 * PARAMETERS:	 rscID	the resource id of the menu.
 *					
 * RETURNED:	 The handle of the memory block that the form is in,  since 
 *              the form structure begins with the WindowType structure this
 *              is also a WindowHandle.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	5/5/95	Initial Revision
 *
 ***********************************************************************/
void * ResLoadMenu (UInt16 rscID)
{
	MemHandle					menuROMHandle;		// menu in ROM
	MemHandle					menuHandle;			// copy of menu
	MenuBarPtr 				menuBar, menuBarROM;
	Int16 i, j;
	UInt16 menuBarSize;
	
	
	// The menu is in ROM.  We want to write to it to set its pointers.
	// Copy it to RAM so we can do this.
	menuROMHandle = DmGetResource(MenuRscType, rscID);
	ErrFatalDisplayIf(menuROMHandle==NULL, "Menu rsrc not found");
	
	menuBarROM = (MenuBarPtr) MemHandleLock (menuROMHandle);
	menuBarSize = (UInt16) MemPtrSize(menuBarROM);
	
	menuHandle = MemHandleNew(menuBarSize);
	menuBar = (MenuBarPtr) MemHandleLock (menuHandle);

	MemMove(menuBar, menuBarROM, menuBarSize);
	MemHandleUnlock (menuROMHandle);
	
	
	
	// Fixup the pointers to the menus
	menuBar->menus = (MenuPullDownPtr) (menuBar+1);


	// Fixup the pointers to the items in each pulldown menu.
	for (i = 0; i < menuBar->numMenus; i++)
		{
		menuBar->menus[i].title = 
			(((char *) menuBar) + (Int32) menuBar->menus[i].title);


		// Pointer to an array of items
		menuBar->menus[i].items = (MenuItemType *) 
			((char *) menuBar + (Int32) menuBar->menus[i].items);


		// Fixup the pointers in each menu item			
		for (j = 0; j < menuBar->menus[i].numItems; j++)
			{
			menuBar->menus[i].items[j].itemStr = 
				(((char *) menuBar) + (Int32) menuBar->menus[i].items[j].itemStr);
			}
			
		}
				
		

	return menuBar;
}

/***********************************************************************
 *
 * FUNCTION: 	 ResLoadConstant
 *
 * DESCRIPTION: This routine returns the value of a constant loaded
 *					 from a resource.  Resources of this type can then be
 *					 overlaid to support alternate locales (e.g., wider
 *					 buttons necessary for localized text).
 *
 * PARAMETERS:	 rscID	the resource id of the constant.
 *					
 * RETURNED:	 The value of the constant in the resource.  Feel free
 *					 to cast as necessary.  If resource not found, returns 0.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			CS		06/29/99	Initial Revision
 *
 ***********************************************************************/
UInt32 ResLoadConstant (UInt16 rscID)
{
	MemHandle constantHdl;
	UInt32 constantValue;
	
	// Load the resource containing the constant and return it.
	constantHdl = DmGetResource(constantRscType, rscID);
	if (!constantHdl)
		{
		ErrNonFatalDisplay("Constant rsrc not found");  // DOLATER remove this after 3.5 beta
		return 0;
		}
	
	constantValue = *(UInt32 *)MemHandleLock(constantHdl);
	MemHandleUnlock(constantHdl);
	return(constantValue);
} // ResLoadConstant
