/******************************************************************************
 *
 * Copyright (c) 1997-1999 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: FontSelect.c
 *
 * Release: 
 *
 * Description:
 *	  This file contain the font selector routines.
 *
 * History:
 *		September 10, 1997	Created by Art Lamb
 *
 *****************************************************************************/

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_CONTROLS

#include <PalmTypes.h>
#include <SystemPublic.h>

#include "FontSelect.h"
#include "Form.h"
#include "UIResourcesPrv.h"

#define font1				stdFont
#define font2				boldFont
#define font3				largeBoldFont


/***********************************************************************
 *
 * FUNCTION: 	 FontSelect
 *
 * DESCRIPTION: This routine displays the font selection dialog.
 *
 * PARAMETERS:	 fontID - initial setting of the dialog.
 *					
 * RETURNED:	 the font id of the selected font.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	9/10/97	Initial Revision
 *			art	5/5/98	Select the font based on the font id of the selected 
 *                      push button.
 *			PPl	11/3/99 	Fixed a Field Focus Setting problems.
 *			jmp	11/03/99 Backed out the previous change as it does NOT
 *								work in general -- i.e., FrmSetFocus() isn't
 *								orthogonal for releasing/grabbing fields in tables!
 *
 ***********************************************************************/
FontID FontSelect (FontID fontID)
{
	UInt16		index;
	UInt16		ctlID;
	UInt16		buttonHit;
	UInt16		numObjects;
	FormPtr		frm;
	ControlPtr	ctl;

	frm = FrmInitForm (FontSelectorForm);

	ctlID = FontSelector1Button;

	// Find the push button the has the specified font id.
	numObjects = FrmGetNumberOfObjects (frm);
	for (index = 0; index < numObjects; index++)
		{
		if (FrmGetObjectType (frm, index) == frmControlObj)
			{
			ctl = FrmGetObjectPtr (frm, index);
			if (ctl->group == FontSelectorFontGroup && ctl->font == fontID)
				{
				ctlID = ctl->id;
				}
			}
		}

	FrmSetControlGroupSelection (frm, FontSelectorFontGroup, ctlID);

	// Display the font selector.
	buttonHit = FrmDoDialog (frm);

	// Get the font setting from the dialog if the "OK" button 
	// has pressed.  We will return the font id of the push button that was 
	// selected
	if (buttonHit == FontSelectorOKButton)
		{
		index =  FrmGetControlGroupSelection (frm, FontSelectorFontGroup);
		ctl = FrmGetObjectPtr (frm, index);
		fontID = ctl->font;
		}

	FrmDeleteForm (frm);

	return (fontID);
}
