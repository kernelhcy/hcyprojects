/******************************************************************************
 *
 * Copyright (c) 1995-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: Category.c
 *
 * Release: 
 *
 * Description:
 *	  This file contains routine for managing categories. 
 *
 *	HISTORY
 *		02/24/95	art	Created by Art Lamb.
 *		07/07/99	kwk	Use soft constants for maxCategoryWidth & extra
 *							inline conversion bytes.
 *		08/12/99	gap	Update to use new constants categoryHideEditCategory & categoryDefaultEditCategoryString.
 *
 *****************************************************************************/

#define NON_PORTABLE

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_FIELDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_LISTS

#include <SystemPublic.h>

#include <PalmUtils.h>

#include "Globals.h"

#include "Category.h"
#include "Event.h"
#include "UIResourcesPrv.h"
#include "TextServicesPrv.h"		// tsmFtrCreator, tsmFtrNumFepFieldExtra

#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
#include "MemoryPrv.h"
#endif

// Extract the bit at position index from bitfield.  0 is the high bit.
#define BitAtPosition(pos)	((UInt16)1 << (pos))
#define GetBitMacro(bitfield, index)	((bitfield) & BitAtPosition(index))
#define SetBitMacro(bitfield, index)	((bitfield) |= BitAtPosition(index))
#define RemoveBitMacro(bitfield, index)	((bitfield) &= ~BitAtPosition(index))

// copied from Control.c ... prolly should actually be in a header somewhere
#define popupIndicatorGap				5
#define popupIndicatorWidth			7
#define popupIndicatorHeight			((popupIndicatorWidth+1)/2)
#define horizontalSpace					3

// Uniq IDs for category names used for synchronation.
#define firstDeviceUniqID		0x00		// high bit clear
#define lastDeviceUniqID		0x7F		// high bit clear
#define firstPCUniqID			0x80		// high bit set
#define lastPCUniqID				0xFF		// high bit set

/***********************************************************************
 *
 * FUNCTION:    GetCategories
 *
 * DESCRIPTION: Get the categories from the specified  database.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    MemHandle of the AppInfoType block
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	2/22/95	Initial Revision
 *
 ***********************************************************************/
static AppInfoType * GetCategories (DmOpenRef db)
{
	UInt16 		cardNo;
	LocalID 	dbID;
	LocalID 	appInfoID;

	// Get the categories from the database.	
	if (DmOpenDatabaseInfo (db, &dbID, NULL, NULL, &cardNo, NULL))
		{
		ErrFatalDisplay ("Bad database ID");
		return (0);
		}
		
	if (DmDatabaseInfo (cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
			&appInfoID, NULL, NULL, NULL))
		{
		ErrFatalDisplay ("Bad database ID");
		return (0);
		}
	return MemLocalIDToLockedPtr (appInfoID, cardNo);
}


/***********************************************************************
 *
 * FUNCTION:    GiveCategoryNewUniqID
 *
 * DESCRIPTION: Give the category a uniq ID.  To be used whenever a 
 *					 category is created and kept until deletion.
 *
 * PARAMETERS:  appCategories - pointer to the categories
 *					 index - which category to change
 *
 * RETURNED:    nothing
 *					 the category's uniq id is set
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	9/22/95	Initial Revision
 *
 ***********************************************************************/
static void GiveCategoryNewUniqID (AppInfoType * appCategories, UInt16 index)
{
	UInt8 	newUniqID;
	UInt16	i;
	AppInfoType*	nilP=0;
	UInt8		newValue;
	
	newUniqID = appCategories->lastUniqID;
	if (newUniqID >= lastDeviceUniqID)
		newUniqID = firstDeviceUniqID;
	else
		newUniqID++;
	
	do {
		// Look to see if the new Uniq ID is already used
		for (i = 0; i < dmRecNumCategories; i++)
			if (appCategories->categoryUniqIDs[i] == newUniqID)
				break;
		
		// If the new Uniq ID is used by a category try the next one
		if (i < dmRecNumCategories)
			{
			if (newUniqID >= lastDeviceUniqID)
				newUniqID = firstDeviceUniqID;
			else
				newUniqID++;
			continue;
			}
		else
			{
			break;
			}
		}
	while (true);
	
	// Write the change
	newValue = newUniqID;
	DmWrite(appCategories, OffsetOf(AppInfoType, categoryUniqIDs[index]),
					&newValue, sizeof(newValue));
	DmWrite(appCategories, OffsetOf(AppInfoType, lastUniqID), 
					&newValue, sizeof(newValue));
}


/***********************************************************************
 *
 * FUNCTION:    CategoryCompare
 *
 * DESCRIPTION: Case blind comparision of two category names. This routine is 
 *              used as a callback routine when sorting the the category list.
 *
 * PARAMETERS:  s1 - a category name (null-terminated)
 *              s2 - a category name (null-terminated)
 *
 * RETURNED:    if s1 > s2  - a positive integer
 *              if s1 = s2  - zero
 *              if s1 < se  - a negative integer
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	7/18/95	Initial Revision
 *
 ***********************************************************************/
static Int16 CategoryCompare (Char **s1, Char **s2, Int32 /* other */)
{
	return StrCompare(*s1, *s2);
}


/***********************************************************************
 *
 * FUNCTION:    CategoryCreateList
 *
 * DESCRIPTION: Read a database's categories and set the categories 
 *              list's items.
 *
 * PARAMETERS:  db - opened database containing category info
 *					 listP - list in which to place the categories
 *					 currentCategory - category to select
 *					 showAll - true to have an all category
 *					 showHidden - true to show hidden categories
 *					 numUneditableCategories - number of categories considered 
 *							uneditable.  These are stored first.
 *					 editingString - A resource type to string to edit categories.
 *					 resizeList - true to resize the list to the number of 
 *							categories.  Set true for popups, false otherwise.
 *
 * RETURNED: A memory block is allocated containing the list of categories. The
 *		listP is set to use the memory block. CategoryFreeList must be called after
 *		this.
 *
 * HISTORY:
 *		02/22/95	rsf	Created by Roger Flores
 *		08/08/96	rsf	Added Uneditables and the editing string code.
 *		07/05/99	kwk	Only call LstMakeItemVisible if we have a selected item.
 *		08/13/99	gap	Update to use new constants categoryHideEditCategory &
 *							categoryDefaultEditCategoryString.
 *		11/13/00	kwk	Check for invalid currentCategory, numUneditableCategories,
 *							and editingStrID parameters.
 *		11/15/00	kwk	Cleaned up some of the DOLATER comments, for future generations.
 *
 ***********************************************************************/
 
extern void CategoryCreateList(DmOpenRef db, ListType * listP, 
	UInt16 currentCategory, Boolean showAll, 
	Boolean showUneditables, UInt8 numUneditableCategories, 
	UInt32 editingStrID, Boolean resizeList)
{
	Int16		index;
	Int16		length;
	AppInfoType * appCategories;
	MemHandle	 itemsHandle;
	void *  	itemsPtr;			// Freed by CategoryFreeList
	Char *	listItems[dmRecNumCategories + 2];	// Optional 'All' & 'Edit Categories..."
	Int16		listIndex;
	Char *	str;
	MemHandle	allStringH;
	MemHandle	editCategoriesStringH;
	
	if (numUneditableCategories > dmRecNumCategories)
		{
		ErrNonFatalDisplay("Too many uneditable categories");
		numUneditableCategories = dmRecNumCategories;
		}
	
	if ((currentCategory != dmAllCategories)
	 && (currentCategory >= dmRecNumCategories))
		{
		ErrNonFatalDisplay("Invalid current category");
		currentCategory = dmAllCategories;
		}
	
	appCategories = GetCategories (db);

	if (showAll)
		{
		allStringH = DmGetResource(strRsc, categoryAllStrID);
		listItems[0] = MemHandleLock(allStringH);
		listIndex = 1;
		}
	else
		listIndex = 0;

	// For each category not empty, copy its string ptr to the list's
	// item list.
	for (index = numUneditableCategories; index < dmRecNumCategories; index++)
		{
		if (appCategories->categoryLabels[index][0] != '\0')
			{
			listItems[listIndex++] = appCategories->categoryLabels[index];
			}
		}
		
	// Sort the category list.  The "All" item should always be first
	// if it's present.
	if (showAll)
		SysQSort(&listItems[1], listIndex - 1, sizeof(Char *), (CmpFuncPtr)CategoryCompare, 0);
	else
		SysQSort(&listItems[0], listIndex, sizeof(Char *), (CmpFuncPtr)CategoryCompare, 0);
	
	// Add "uneditable" items to the end of the list.
	if (showUneditables)
		{
		for (index = 0; index < numUneditableCategories; index++)
			{
			if (appCategories->categoryLabels[index][0] != '\0')
				{
				listItems[listIndex++] = appCategories->categoryLabels[index];
				}
			}
		}

	// 0 in edit string specifies that an edit categories selection should not be appended to the menu
 	if (editingStrID == 0)
		editingStrID = categoryHideEditCategory;	

	if (editingStrID == categoryDefaultEditCategoryString)  
		editingStrID = categoryEditStrID;

	
	// Add "edit categories" item to the end of the list.
	if (editingStrID != categoryHideEditCategory)
		{
		editCategoriesStringH = DmGetResource(strRsc, (DmResID)editingStrID);
		ErrNonFatalDisplayIf(editCategoriesStringH == NULL, "Missing string resource");
		listItems[listIndex++] = MemHandleLock(editCategoriesStringH);
		}
	

	// Allocate a block to hold the items in the list.  This block is freed
	// when the list is disposed of.
	if (listIndex > 0)
		{
		length = (listIndex) * sizeof (Char *);
		itemsHandle = MemHandleNew (length);
		itemsPtr = MemHandleLock (itemsHandle);
		MemMove(itemsPtr, listItems, length);
		}
	else
		{
		itemsPtr = NULL;
		}
	
	LstSetListChoices (listP, itemsPtr, listIndex);
	if (resizeList)
		LstSetHeight (listP, listIndex);


	// Is the current category one of the uneditable categories? If so, then
	// we 
	if (currentCategory < numUneditableCategories)
		{
		if (showUneditables)
			// DOLATER - If any of the strings in the app info block are "",
			// then currentCategory won't match entries in the list. Should
			// currentCategory get decremented when a "" name is encountered?
			// Should this code find the current category in the list using
			// the name, to guarantee that it works?
			LstSetSelection (listP, listIndex - ((editingStrID != categoryHideEditCategory) ? 2 : 1) - 
				(numUneditableCategories - 1 - currentCategory));
		else
			LstSetSelection (listP, noListSelection);
		}
		
	// Is the current category the "All" categories?
	else if (currentCategory == dmAllCategories)
		{
		if (showAll)
			{
			LstSetSelection (listP, 0);
			}
		else
			{
			LstSetSelection (listP, noListSelection);
			}
		}
	
	// Find the current category in the in the sorted category list.
	else if (*appCategories->categoryLabels[currentCategory])
		{
		str = appCategories->categoryLabels[currentCategory];
		for (index = showAll ? 1 : 0; index < listIndex; index++)	// skip All if present
			if (StrCompare (str, listItems[index]) == 0)
				{ 
				LstSetSelection (listP, index);
				break;
				}
		}
	
	// If all else fails set the current list selection to the "unfiled" item.
	else
		{
		// DOLATER - What is trying to be selected here? There is no unfiled
		// item - all that might be known is the All and the Edit Categories items.
		// And the same as above, if any entries in the app info block are
		// "", then numUneditableCategories might extend beyond the # of
		// entries in the derived list, and thus this call to LstSetSelection
		// will generate a non-fatal alert...unless we add a check for the
		// case of numUneditableCategories containing "" strings, and adjust
		// it (or report a non-fatal alert at that time).
		if (listIndex > 0)
			LstSetSelection (listP, listIndex - ((editingStrID != categoryHideEditCategory) ? 2 : 1) - (numUneditableCategories - 1));
		else
			LstSetSelection (listP, noListSelection);
		}
	
	
	// Make the item visible if it's selected.
	index = LstGetSelection (listP);
	if (index != noListSelection)
		LstMakeItemVisible (listP, index);
	
	// Don't unlock the app info block because there are pointers into it	
}


/***********************************************************************
 *
 * FUNCTION:    CategoryFreeList
 *
 * DESCRIPTION: This routine unlocks or frees memory locked or allocated
 *              by CategoryCreateList.
 *
 * PARAMETERS:  db - database containing the categories
 *					 listP - pointer to the category list
 *					 showAll - true if the list was created with an All category
 *					 allowEditing - true if the list was created to allow editing
 *
 * RETURNED:    nothing
 *
 * HISTORY:
 *		02/23/95	art	Created by Art Lamb.
 *		04/18/96	rsf	Revised to handle multiple lists
 *		08/13/99	gap	Update to use new constants categoryHideEditCategory
 *							& categoryDefaultEditCategoryString.
 *		11/13/00	kwk	listP isn't const, since we free the itemsText ptr.
 *							Clear listP->itemsText after freeing it.
 *							Remap categoryDefaultEditCategoryString to categoryEditStrID.
 *							Catch NULL edit string resource handle.
 *
 ***********************************************************************/
void CategoryFreeList (DmOpenRef db, ListType * listP, 
	Boolean showAll, UInt32 editingStrID)
{
	MemPtr categoriesP;
	
	if (listP->itemsText)
		{
		MemPtrFree (listP->itemsText);
		listP->itemsText = NULL;
		}
	
	categoriesP = GetCategories (db);
	MemPtrUnlock(categoriesP);
	MemPtrUnlock(categoriesP);	// unlock lock before this call

	// Depending on how CategoryCreateList was called it locked certain 
	// resources.  Unlock those it used.
	if (showAll)
		{
		// DOLATER - need to call DmReleaseResource on handle, so how
		// to keep around the original handle?
		MemHandleUnlock(DmGetResource(strRsc, categoryAllStrID));
		}
	
	if (editingStrID == categoryDefaultEditCategoryString)  
		editingStrID = categoryEditStrID;

	if ( ! ((editingStrID == 0) || (editingStrID == categoryHideEditCategory)) )
		{
		// DOLATER - need to call DmReleaseResource on handle, so how
		// to keep around the original handle?
		MemHandle h = DmGetResource(strRsc, editingStrID);
		ErrNonFatalDisplayIf(h == NULL, "Can't get editing string resource");
		MemHandleUnlock(h);
		}
}


/***********************************************************************
 *
 * FUNCTION:    CategoryFind
 *
 * DESCRIPTION: This routine returns the index of the category that 
 *              match the name passed.
 *
 * PARAMETERS:  name - category name
 *
 * RETURNED:    category index
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/6/95	Initial Revision
 *			roger	4/12/96	Changed to return dmAllCategories when no category is found
 *			trm	08/07/97	made non modified passed variabls constant
 *
 ***********************************************************************/
UInt16 CategoryFind (DmOpenRef db, const Char * name)
{
	AppInfoType * appCategories;
	UInt16		     index;


	appCategories = GetCategories (db);
	for (index = 0; index < dmRecNumCategories; index++)
		{
		if (StrCompare (appCategories->categoryLabels[index], name) == 0)
			{
			MemPtrUnlock(appCategories);
			return (index);
			}
		}
	MemPtrUnlock(appCategories);

	return (dmAllCategories);			
}


/***********************************************************************
 *
 * FUNCTION:    CategoryGetName
 *
 * DESCRIPTION: This routine returns name the specified category.
 *
 * PARAMETERS:  db    - database the contains the categories
 *              index - category index
 *              name  - buffer to hold category name
 *
 * RETURNED:    category name in buffer passed
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/7/95	Initial Revision
 *			trm	08/07/97	made non modified passed variabls constant
 *
 ***********************************************************************/
void CategoryGetName (DmOpenRef db, UInt16 index, Char* name)
{
	AppInfoType * appCategories;


	if (index == dmAllCategories)
		{
		SysCopyStringResource(name, categoryAllStrID);
		}
	else
		{
		ErrFatalDisplayIf(index >= dmRecNumCategories, "Bad Category");
		appCategories = GetCategories (db);
		StrCopy (name, appCategories->categoryLabels[index]);
		MemPtrUnlock(appCategories);
		}
}


/***********************************************************************
 *
 * FUNCTION:    CategorySetName
 *
 * DESCRIPTION: Set the category name and rename bits. A NULL ptr
 * removes the category name.  Set the uniq ID if the name is new
 * (replaces an empty string name).
 *
 * PARAMETERS:  db - database containing the categories to change
 *					 index - index of category to set
 *              name - a category name (null-terminated) or 
 *							  NULL ptr to remove the category
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	4/16/96	Initial Revision
 *			trm	08/07/97	made non modified passed variabls constant
 *
 ***********************************************************************/
void CategorySetName (DmOpenRef db, UInt16 index, const Char * nameP)
{
	AppInfoType *	appCategories;
	AppInfoType *	nilP = 0;
	UInt32				offset;
	Char				zero = 0;
	UInt16				renamedCategories;


	appCategories = GetCategories (db);
	offset = (UInt32)&nilP->categoryLabels[index];
	renamedCategories = appCategories->renamedCategories;
	
	if (nameP == NULL)
		{
		DmWrite(appCategories, offset, &zero, 1);
		RemoveBitMacro(renamedCategories, index);
		}
	else
		{
		// If the category is new give it a unique ID.
		if (appCategories->categoryLabels[index][0] == nullChr)
			GiveCategoryNewUniqID(appCategories, index);
		
		DmStrCopy(appCategories, offset, nameP);
		SetBitMacro(renamedCategories, index);
		}
		
	DmWrite(appCategories, (UInt32)(&nilP->renamedCategories),
		&renamedCategories, sizeof(renamedCategories));

	MemPtrUnlock(appCategories);

}

/***********************************************************************
 *
 * FUNCTION: 	 CategoryTruncateName
 *
 * DESCRIPTION: This routine trunctated a category name such that it
 *              is short enough to display.
 *
 * PARAMETERS:	name    - pointer to the name of the new category
 *					maxWidth	- maximum pixel width that name can occupy.
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	10/24/95	Initial Revision
 *			trm	08/07/97	made non modified passed variabls constant
 *			kwk	08/18/98	Modified to use FntWidthToOffset, so that it works
 *								with Japanese.
 *			kwk	08/20/98	Always truncate name to dmCategoryLength-1.
 *			kwk	10/12/98	Only write null byte if we have to truncate, otherwise
 *								we trigger bus errors w/skanky apps that pass us a
 *								ptr into the storage heap.
 *								Added debug check for name ptr in storage heap.
 *
 ***********************************************************************/
void CategoryTruncateName (Char * name, UInt16 maxWidth)
{
	UInt32	bytesThatFit;
	UInt32	charEnd;
	Int16	truncWidth;
	UInt32	length;
	FontID curFont;

#if EMULATION_LEVEL == EMULATION_NONE
	// On the device, storage heaps always live 'above' the last dynamic
	// heap, and no other usable memory exists above storage heaps, thus
	// we can easily check for a pointer into a storage heap.
	ErrNonFatalDisplayIf(name >= MemHeapPtr(memDynamicHeaps),
			"CategoryTruncateName: name in storage heap");
#endif

	curFont = FntSetFont (stdFont);
	
	maxWidth -= popupIndicatorWidth + popupIndicatorGap;
	
	length = StrLen(name);
	bytesThatFit = FntWidthToOffset(name, length, maxWidth, NULL, &truncWidth);
	
	// If we need to truncate, then figure out how far back to trim to
	// get enough horizongal width for the ellipsis character.
	
	if (bytesThatFit < length) {
		Int16 ellipsisWidth = FntCharWidth(chrEllipsis);
		
		// Not enough space for a single ellipsis, so return an empty string.
		
		if (ellipsisWidth > maxWidth) {
			name[0] = '\0';
			return;
		} else if (ellipsisWidth > maxWidth - truncWidth) {
			bytesThatFit = FntWidthToOffset(name, length, maxWidth - ellipsisWidth, NULL, &truncWidth);
		}
		
		// Should never happen, but make sure we don't create a string that's
		// longer than the max category length (leave room for ellipsis & null)
		bytesThatFit = min(bytesThatFit, dmCategoryLength - 2);
		
		// Also make sure we don't truncate in the middle of a character.
		TxtCharBounds(name, bytesThatFit, &bytesThatFit, &charEnd);
		bytesThatFit += TxtSetNextChar(name, bytesThatFit, chrEllipsis);
		name[bytesThatFit] = '\0';
	} else if (length > dmCategoryLength - 1) {
		TxtCharBounds(name, dmCategoryLength - 1, &bytesThatFit, &charEnd);
		name[bytesThatFit] = '\0';
	}

	FntSetFont(curFont);
}


/***********************************************************************
 *
 * FUNCTION: 	 CategorySetTriggerLabel
 *
 * DESCRIPTION: This routine sets the lable displayed by the category
 *              trigger.  The category name is trunctated if it's to
 *                        long.
 *
 * PARAMETERS:	 ctl      - item number of the object
 *					 label    - pointer to the name of the new category
 *
 * RETURNED:	 nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	10/24/95	Initial Revision
 *			trm	08/07/97	made non modified passed variabls constant
 *			kwk	07/07/99	Use soft constant for truncation width.
 *
 ***********************************************************************/
void CategorySetTriggerLabel (ControlType * ctl, Char * name)
{

// DOLATER kwk - In the future I think we'd
// want a call to the form to get the max category width, which I believe
// is limited by the form title width. We could also generate an event
// when the form title width changes?

	CategoryTruncateName (name, ResLoadConstant(maxCategoryWidthID));
	CtlSetLabel (ctl, name);
}


/***********************************************************************
 *
 * FUNCTION:    CategoryGetNext
 *
 * DESCRIPTION: Given a category index this routine returns the index
 *              of the next category.  Catergory are not stored
 *              sequentially.
 *
 * PARAMETERS:  db    - database the contains the categories
 *              index - category index
 *
 * RETURNED:    category index of next category
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	9/15/95	Initial Revision
 *			art	3/26/96	Add to ignore empty categories
 *			trm	08/07/97	made non modified passed variabls constant
 *
 ***********************************************************************/
UInt16 CategoryGetNext (DmOpenRef db, UInt16 index)
 {
	Int16 i, j, count;
	UInt16 nextCategory;
	Char *	names [dmRecNumCategories];
	AppInfoType * appCategories;


	// The "unfiled" category is always the last category displayed,  and 
	// "show all" is always the first category displayed.  If the 
	// current category is "unfiled" wrap around to the "show all"
	// category.
	if (index == dmUnfiledCategory)
		return (dmAllCategories);


	//
	// Create a sort list of category names.
	//
	appCategories = GetCategories (db);
	count = 0;

	// For each non-blank category, copy a pointer to its name in to a
	// list and sort the list.
	for (i = 1; i < dmRecNumCategories; i++)
		{
		if (appCategories->categoryLabels[i][0] != '\0')
			{
			names[count++] = appCategories->categoryLabels[i];
			}
		}
	SysQSort (names, count, sizeof (Char *), (CmpFuncPtr)CategoryCompare, 0);


	// Assume the next category is the "all" category.
	nextCategory = dmAllCategories;
	
	
	// Are there categories other than "unfiled" and "show all"?  If so
	// find the index, in the sort list of names, of the next category.
	if (count)
		{
		// If the current category is "show all" find the category index
		// of the first name in sorted list of names.
		if (index == dmAllCategories)
			{
			j = 0;
			}
	
		// The current category is not "show all"  or "unfiled".
		// Find the current category in the sorted list of names.
		else
			{
			for (j = 0; j < count; j++)
				{
				if (StrCompare (appCategories->categoryLabels[index], names[j]) == 0)
					break;
				}
			j++;
			}
	
	
		// If the current category is not the last entry in the sorted list of 
		// names, then 
		while  (j < count)
			{
			// Find the category index of the name in unsorted list of names.
			for (i = 1; i < dmRecNumCategories; i++)
				{
				if (StrCompare (appCategories->categoryLabels[i], names[j]) == 0)
					break;
				}
				
			// See if there are any records in the category, if there are we've
			// found the next category.
			index = 0;
			if (DmSeekRecordInCategory (db, &index, 0, dmSeekForward, i) == 0)
				{
				nextCategory = i;
				break;
				}
				
			j++;
			}
		}

	MemPtrUnlock(appCategories);
	
	return (nextCategory);
 }
 

/***********************************************************************
 *
 * FUNCTION:    CategoryHasRecords
 *
 * DESCRIPTION: Returns true if a record exists in the category.
 *
 * PARAMETERS:  db       - database the contains the categories
 *              category - category to find a record in
 *
 * RETURNED:    true if a record exists in the category
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	8/28/95	Initial Revision
 *
 ***********************************************************************/
static Boolean CategoryHasRecords (DmOpenRef db, UInt16 category)
{
	UInt16 firstRecord = 0;
	
	
	DmSeekRecordInCategory (db, &firstRecord, 0, dmSeekForward, category);
	if (DmGetLastErr()) return (false);
	
	return (true);
}


/***********************************************************************
 *
 * FUNCTION:    CategoryEdit
 *
 * DESCRIPTION: This routine is the event handler for the "Categories Edit"
 *              dialog.
 *
 * PARAMETERS:  event    - a pointer to an EventType structure
 *              category - current category
 *              titleStrID - string id to use for the dialog's title
 *              numUneditableCategories - how many categories can't be edited.
 *							Uneditable categories start at category zero.
 *
 * RETURNED:    This routine returns true if any of the following conditions
 *					 are true:
 *					 	- the current category is renamed
 *					 	- the current category is deleted
 *					 	- the current category is merged with another category
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	02/21/95	Initial Revision
 *			roger	04/16/96	Changed to MemHandle a different dialog 
 *			roger	08/08/96	Changed to MemHandle a passed title 
 *			trm	08/07/97	made non modified passed variabls constant
 *			roger	10/30/97	Added numUneditableCategories
 *			roger	11/12/97	Fixed rename category to selected category to return true
 *			kwk	10/13/98	If we've got inline conversion, allow length of field
 *								for new/edit category to be temporarily longer.
 *			kwk	10/28/98	Put up an alert if the category name is too long.
 *			kwk	12/13/98	Fixed bug w/not reloading text field ptr after editing.
 *			gap	08/13/99	Update to use new constants categoryHideEditCategory
 *								& categoryDefaultEditCategoryString.
 *			kwk	07/06/00	Use TSM feature for field extra space, versus soft constant.
 *
 ***********************************************************************/
Boolean CategoryEdit (DmOpenRef db, UInt16 * categoryP, 
	UInt32 titleStrID, UInt8 numUneditableCategories)
{
	FormType * frm;
	FormType * savedFrm;
	EventType event;
	Boolean categoryChanged = false;
	Int16 selectedItem;
	UInt16 selectedCategory;
	UInt16 newCategory;
	UInt16 alertResponse;
	AppInfoType *	appCategories;
	UInt16 fieldIndex;
	FieldPtr fieldP;
	Char * fieldTextP;
	UInt16 buttonHit;
	MemHandle fieldTextH;
	ListPtr listP;
	Boolean updateList;
	MemHandle titleRscH;
	Char * titleRscP;
	Char * titleP = NULL;
	Char * ellipsesP;
	
	// Init the dialog
	savedFrm = FrmGetActiveForm ();
	frm = FrmInitForm (CategoriesEditForm);

	// Setup the form.  Get the list ready to be updated.	
	listP = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, CategoriesEditList));
	listP->attr.usable = false;
	LstSetSelection(listP, noListSelection);
	selectedCategory = *categoryP;
	updateList = true;						// set it to the editable categories


	// Setup the form title from a passed in string resource.  Remove any trailing
	// ellipses.
	if (titleStrID)
		{
		titleRscH = DmGetResource(strRsc, titleStrID);
		if (titleRscH)
			{
			titleRscP = MemHandleLock(titleRscH);
			titleP = MemPtrNew(StrLen(titleRscP) + sizeOf7BitChar('\0'));
			if (titleP != NULL)
				{
				StrCopy(titleP, titleRscP);
				
				// Now check if the string ends with "..." and if so remove the chars.
				// DOLATER kwk - also check for a real ellipsis character - probably just
				// the new one in low memory, though if char encoding is latin then
				// we might also want to check 0x85 as well.
				ellipsesP = StrStr(titleP, "...");
				if (ellipsesP)
					*ellipsesP = '\0';
				
				FrmSetTitle(frm, titleP);
				}
			
			MemHandleUnlock(titleRscH);
			DmReleaseResource(titleRscH);
			}
		}
	
	
	// Draw the dialog
	FrmSetActiveForm (frm);
	FrmDrawForm (frm);


	// Handle input for the dialog until the user presses the OK button or until
	// an appStopEvent is received.
	while (true)
		{

		// Recreate the list so it reflects any changes
		if (updateList)
			{
			if (listP->attr.usable)
				CategoryFreeList (db, listP, false, 0);
			else
				listP->attr.usable = true;
			
			CategoryCreateList (db, listP, selectedCategory, false, false, 
				numUneditableCategories, categoryHideEditCategory, false);
			LstSetHeight (listP, 9);
			LstDrawList(listP);
			selectedItem = LstGetSelection (listP);

			updateList = false;
			}
			
			
		EvtGetEvent (&event, evtWaitForever);

		if (! SysHandleEvent ((EventType *)&event))
			if (! LstHandleEvent (listP, &event))
				{
				//	Hard scroll buttons
				if	(	(event.eType == keyDownEvent)
					&&	(!TxtCharIsHardKey(	event.data.keyDown.modifiers,
													event.data.keyDown.chr))
					&&	(EvtKeydownIsVirtual(&event))
					&&	(	(event.data.keyDown.chr == vchrPageUp)
						||	(event.data.keyDown.chr == vchrPageDown)))
					{
					if (event.data.keyDown.chr == vchrPageUp)
						{
						LstScrollList(listP, winUp, LstGetVisibleItems(listP) - 1);
						}
					else
						{
						LstScrollList(listP, winDown, LstGetVisibleItems(listP) - 1);
						}
					}
					
				
				else if (event.eType == ctlSelectEvent)
					{
					if (event.data.ctlSelect.controlID == CategoriesEditOKButton)
						break;
					else
						{
						switch (event.data.ctlSelect.controlID)
							{
							case CategoriesEditNewButton:
								// Use the first unused category.
								selectedCategory = CategoryFind (db, "");
								if (selectedCategory == dmAllCategories)
									{
									FrmAlert(categoryAllUsedAlert);
									
									// Restore the selectedCategory if there is a list selection
									if (selectedItem != noListSelection)
										{
										selectedCategory = CategoryFind (db, 
											LstGetSelectionText (listP, selectedItem));
										}
									}
								else
									{
									UInt32 fepExtraBytes;
									UInt16 maxBytes = dmCategoryLength - 1;	// don't include null terminator
									
									if (FtrGet(tsmFtrCreator, tsmFtrNumFepFieldExtra, &fepExtraBytes) == errNone) {
										maxBytes += fepExtraBytes;
									}
									
									// Have the user type in a new category name
									frm = FrmInitForm (categoryNewNameDialog);
									fieldIndex = FrmGetObjectIndex (frm, categoryNewNameField);
									FrmSetFocus (frm, fieldIndex);
									fieldP = FrmGetObjectPtr (frm, fieldIndex);
									FldSetMaxChars(fieldP, maxBytes);			
									
									while (true)
										{
										buttonHit = FrmDoDialog(frm);
										
										if ((buttonHit == categoryNewNameOKButton)
										&& (FldGetTextLength(fieldP) > dmCategoryLength - 1))
											{
											FrmAlert(CategoryTooLongAlert);
											fieldP->attr.visible = false;
											FldSetSelection(fieldP, dmCategoryLength - 1, FldGetTextLength(fieldP));
											}
										else
											break;
										}
									
									fieldTextP = FldGetTextPtr (fieldP);
									if (buttonHit == categoryNewNameOKButton &&
										fieldTextP != NULL &&
										fieldTextP[0] != '\0')
										{
										// If the category already exists notify the user and exit.
										if (CategoryFind (db, fieldTextP) != dmAllCategories)
											FrmCustomAlert (CategoryExistsAlert, fieldTextP, NULL, NULL);
										else
											{
											// Mark the new category with a new uniq ID
											appCategories = GetCategories (db);
											GiveCategoryNewUniqID(appCategories, selectedCategory);
											MemPtrUnlock(appCategories);
											
											// Use the new name
											CategorySetName(db, selectedCategory, fieldTextP);
											
											// Recreate the list so it reflects the change
											updateList = true;
											}
										}

									FrmDeleteForm(frm);
									frm = FrmGetActiveForm();
									}
								break;
		
		
							case CategoriesEditRenameButton:
								if (selectedItem == noListSelection)
									{
									FrmAlert(SelectACategoryAlert);
									}
								else
									{
									UInt32 fepExtraBytes;
									UInt16 maxBytes = dmCategoryLength - 1;	// don't include null terminator

									if (FtrGet(tsmFtrCreator, tsmFtrNumFepFieldExtra, &fepExtraBytes) == errNone) {
										maxBytes += fepExtraBytes;
									}
									
									appCategories = GetCategories (db);
									selectedCategory = CategoryFind (db, LstGetSelectionText (listP, selectedItem));
									
									frm = FrmInitForm (categoryNewNameDialog);
									fieldIndex = FrmGetObjectIndex (frm, categoryNewNameField);
									FrmSetFocus (frm, fieldIndex);
									fieldP = FrmGetObjectPtr (frm, fieldIndex);
									fieldTextH = MemHandleNew(dmCategoryLength);
									fieldTextP = MemHandleLock(fieldTextH);
									StrCopy(fieldTextP, appCategories->categoryLabels[selectedCategory]);
									MemHandleUnlock(fieldTextH);
									FldSetTextHandle(fieldP, fieldTextH);
									FldSetSelection(fieldP, 0, FldGetTextLength(fieldP));
									FldSetMaxChars(fieldP, maxBytes);
									
									while (true)
										{
										buttonHit = FrmDoDialog(frm);

										if ((buttonHit == categoryNewNameOKButton)
										&& (FldGetTextLength(fieldP) > dmCategoryLength - 1))
											{
											FrmAlert(CategoryTooLongAlert);
											fieldP->attr.visible = false;
											FldSetSelection(fieldP, dmCategoryLength - 1, FldGetTextLength(fieldP));
											}
										else
											break;
										}
									
									// Reload the text ptr, since we've unlocked the MemHandle and done
									// editing, thus the ptr might be invalid.
									fieldTextP = FldGetTextPtr (fieldP);
									if (buttonHit == categoryNewNameOKButton &&
										fieldTextP[0] != '\0')
										{
										fieldTextP = FldGetTextPtr (fieldP);
										
										// Check if the category name is already used.
										newCategory = CategoryFind (db, fieldTextP);
										
										// If the category name matches the selected category
										// then the user tapped OK without changing the name.
										if (newCategory != selectedCategory)
											{
											// If the category already exists notify the user
											if (newCategory != dmAllCategories)
												{
												// Ask if the categories should be merged
												buttonHit = FrmCustomAlert (MergeCategoryAlert, 
													appCategories->categoryLabels[newCategory], fieldTextP, NULL);
												if (buttonHit == MergeCategoryYes)
													{
													// Merge (Move) the records into an existing category
													DmMoveCategory(db, newCategory, selectedCategory, true);
													
													// Remove the category name.
													CategorySetName(db, selectedCategory, NULL);

													// Recreate the list so it reflects the change
													updateList = true;
													
													
													// Check if the current category was merged into another
													// or if the selected category was merged into the current 
													// category.
													if (selectedCategory == *categoryP || 
														newCategory == *categoryP)
														{
														categoryChanged = true;
														*categoryP = newCategory;
														}
													
													// The selected category has been merged into another.
													selectedCategory = newCategory;
													}
												}
											else
												{
												// Rename the category to a new name.
												CategorySetName(db, selectedCategory, fieldTextP);

												// Optimization.  If the user renames a category with
												// records in it, assume the user is clarifying the
												// category's name.  If there aren't any records, assume
												// the user is using the category for a new purpose.  
												// Change the uniq ID to reflect this.
												// This solves an annoyance when users rename Business
												// for something else, and then a HotSync adds
												// records down to a Business category.  They used to
												// appear "misfiled".
												if (!CategoryHasRecords (db, selectedCategory))
													{
													// Mark the category with a new uniq ID
													appCategories = GetCategories (db);
													GiveCategoryNewUniqID(appCategories, selectedCategory);
													MemPtrUnlock(appCategories);
													}
												
												// Recreate the list so it reflects the change
												updateList = true;
		
												// Check if the current category was renamed.
												if (selectedCategory == *categoryP)
													{
													categoryChanged = true;
													}
												}
											}
										}
	
									MemPtrUnlock(appCategories);
	
									FrmDeleteForm(frm);
									frm = FrmGetActiveForm();
									}
								break;
		
							case CategoriesEditDeleteButton:
								if (selectedItem == noListSelection)
									{
									FrmAlert(SelectACategoryAlert);
									}
								else
									{
									// Initialize alertResponse to delete the category name incase
									// there aren't any records.  This isn't as fast as skipping
									// the DmMoveCategory call but it is cleaner code.
//									alertResponse = RemoveCategoryNameButton;
									alertResponse = RemoveCategoryYes;
									
									// If there are records ask the user what to delete.
									if (CategoryHasRecords (db, selectedCategory))
										alertResponse = FrmAlert (RemoveCategoryAlert);
									
									// If the delete wasn't canceled do something about it.
									
									/* This code is commented out until 3.0 because it breaks
									   1.0 applications which don't MemHandle records being deleted
									   by this mechanism.
									   
									   See Roger about this.
									   
									if (alertResponse != RemoveCategoryCancelButton)
										{
										if (alertResponse == RemoveCategoryNameButton)
											{
											// Recategorize records as unfiled.
											DmMoveCategory (db, dmUnfiledCategory, selectedCategory, true);
											}
										else
											{
											DmDeleteCategory (db, selectedCategory);
											}
										}
									*/
									
									// This is the code to replace the commented out code.
									if (alertResponse == RemoveCategoryYes)
										{
										// Recategorize records as unfiled.
										DmMoveCategory (db, dmUnfiledCategory, selectedCategory, true);
										
										// Remove the category name.
										CategorySetName(db, selectedCategory, NULL);
	
										// Recreate the list so it reflects the change
										updateList = true;
	
										// Check if the current category was deleted.
										if (selectedCategory == *categoryP)
											{
											categoryChanged = true;
											*categoryP = dmUnfiledCategory;
											}
											
										selectedCategory = dmAllCategories;
										}
									}

								break;
							}
						}
					}
				else if (event.eType == lstSelectEvent)
					{
					selectedItem = event.data.lstSelect.selection;
					selectedCategory = CategoryFind (db, LstGetSelectionText (listP, selectedItem));
					}
/* causes a visual bug
				else if (event.eType == lstExitEvent)
					{
					selectedItem = noListSelection;
					}
*/
				else if (event.eType == appStopEvent)
					{
					EvtAddEventToQueue (&event);
					selectedItem = noListSelection;
					break;
					}
				else FrmHandleEvent (frm, &event);
			}		
		}		


	// Clean up
	CategoryFreeList (db, listP, false, 0);
	
	if (titleP)
		{
		MemPtrFree(titleP);
		
		}
	
	FrmEraseForm (frm);
	FrmDeleteForm (frm);
	FrmSetActiveForm (savedFrm);
	
	return (categoryChanged);
}


/***********************************************************************
 *
 * FUNCTION:    CategorySelect
 *
 * DESCRIPTION: This routine process the selection and editting of
 *              categories. 
 *
 * PARAMETERS:  db           - database the contains the categories
 *              frm          - form the contains the category popup list
 *              ctlID        - id of the popup trigger
 *              lstID        - id of the popup list
 *              title        - true if the popup trigger is on the title line
 *											(causes the 'All' category choice to be available)
 *              categoryP    - current category (index into db structure)
 *              categoryName - name of the current category
 *
 * RETURNED:    This routine returns true if any of the following conditions
 *					 are true:
 *					 	- the current category is renamed
 *					 	- the current category is deleted
 *					 	- the current category is merged with another category
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/10/95	Initial Revision
 *			trm	08/07/97	made non modified passed variabls constant
 *			gap	08/12/99	Update to use new constants categoryHideEditCategory & categoryDefaultEditCategoryString.
 *			jed	10/6/99	Copy changes from Launcher's AppCategorySelect to Handle null ctlID & categoryName.
 *
 ***********************************************************************/
Boolean CategorySelect (DmOpenRef db, const FormType * frm, 
	UInt16 ctlID, UInt16 lstID,	Boolean title, 
	UInt16 * categoryP, Char * categoryName, 
	UInt8 numUneditableCategories, UInt32 editingStrID)

{
	ControlPtr 	ctl = 0;
	ListPtr 		lst;
	Char *		str;
	Int16			newSelection;
	Int16			curSelection;
	Char			localCategoryName[dmCategoryLength] = { 0 };
	Boolean		categoryEdited = false;
	UInt16 			category;
	
	// if we were passed a NULL string ptr, re-direct it to the 
	// stack allocated dummy array.  We'll just use that instead.
	if (categoryName == NULL)
		categoryName = localCategoryName;
	
	if(ctlID != 0)
		ctl = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, ctlID));
	
	lst = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, lstID));

	category = *categoryP;
	
	// If editingStrID contains categoryDefaultEditCategoryString or 0 (for backwards compatability) 
	// reset editingStrID to categoryEditStrID which is the resource ID for the default string "edit categories".
	// Otherwise, editingStrID contains a resource ID of an 
	// alternate string the caller desired to be displayed instead of "edit categories"
	if ( (editingStrID == categoryDefaultEditCategoryString) || (editingStrID == 0) )  
		editingStrID = categoryEditStrID;
	
	// If the category trigger is part of the title, then the "all" item
	// should be in the list.
	CategoryCreateList (db, lst, category, title, true, numUneditableCategories, 
		editingStrID, true);
	curSelection = LstGetSelection (lst);
	newSelection = LstPopupList (lst);

	// Was the  "edit categories ..." item  avaliable?  If so, was it selected?
	if ( (editingStrID != categoryHideEditCategory) && (newSelection == LstGetNumberOfItems (lst) - 1) )
		{
		// We'll treat moving a category like selecting a diffent category. 
		categoryEdited = CategoryEdit (db, &category, editingStrID, numUneditableCategories);
		
		// The category name may have been changed or deleted.
		if ((*categoryP != dmAllCategories) && 
			 (*categoryP != dmUnfiledCategory))
			{
			CategoryGetName (db, category, categoryName);
			
			if (ctl != NULL)	// If a trigger was given
				{
				// Set it in the trigger label
				CategorySetTriggerLabel (ctl, categoryName);
				}
			}
		}

	// Was a new category selected?
	else if ((newSelection != curSelection) && (newSelection != -1))
		{
		// Make a copy of the category name, it's going to freed.
		str = LstGetSelectionText (lst, newSelection);
		StrCopy (categoryName, str);
		
		// The first item (item 0) is "show all categories".
		if ((newSelection == 0) && (title))
			category = dmAllCategories;
		else
			{
			category = CategoryFind (db, categoryName);
			ErrFatalDisplayIf (category == dmAllCategories, "Bad category name");
			}


		// The category was changed.  Display the name of the new 
		// category on the popup trigger.
		if (ctl != NULL) // ... but only if a trigger was given
			{
			CategorySetTriggerLabel (ctl, categoryName);
			}
		}

	CategoryFreeList (db, lst, title, editingStrID);
	
	*categoryP = category;
	return (categoryEdited);
}


/************************************************************
 *
 * FUNCTION:	CategoryInitialize
 *
 * DESCRIPTION: Initialize the category names, ids and flages.
 *
 * PARAMETERS:
 *		appInfoP						 ->	application info ptr
 *		localizedAppInfoStrID	 ->	resource id the localized category names
 *
 * RETURNS:     nothing
 *
 * HISTORY:
 *		10/29/96	art	Created by Art Lamb.
 *		11/08/00	kwk	Catch case of app info string having < 16 entries,
 *						and entry length > max category name length.
 *
 *************************************************************/
void CategoryInitialize (AppInfoPtr appInfoP, UInt16 localizedAppInfoStrID)
{
	Int16 			i;
	UInt16			len;
	UInt16			renamedCategories;
	Char*			name;
	Char*	 		localizedAppInfoP;
	MemHandle 		localizedAppInfoH;
	AppInfoPtr		nilP = 0;
	UInt32			localizedSize;

	localizedAppInfoH = DmGetResource (appInfoStringsRsc, localizedAppInfoStrID);
	if (! localizedAppInfoH)
		{
		ErrNonFatalDisplay("Can't load app info strings");
		return;
		}

	localizedSize = MemHandleSize(localizedAppInfoH);
	localizedAppInfoP = MemHandleLock (localizedAppInfoH);
	name = localizedAppInfoP;

	// Copy each category name and initialize the unique category ID.
	renamedCategories = appInfoP->renamedCategories;
	for (i = 0; i < dmRecNumCategories; i++)
		{
		// Catch case of getting passed list of strings with less
		// than dmRecNumCategories.
		if (name >= localizedAppInfoP + localizedSize)
		{
			// Note that we don't trigger a non-fatal alert here, because a
			// number of developers have released apps with short 'tAIS'
			// resources. Instead we'll trigger a warning in PalmRez.
			break;
		}
		
		len = StrLen (name);
		if (len >= dmCategoryLength)
			{
			ErrNonFatalDisplay("App info string too long");
			}
		else if (len > 0)
			{
			DmStrCopy (appInfoP, (UInt32) nilP->categoryLabels[i], name);
			SetBitMacro (renamedCategories, i);
			}
		name += len + 1;
	
		DmSet (appInfoP, (UInt32)&nilP->categoryUniqIDs[i], 
			sizeof (appInfoP->categoryUniqIDs[i]), i);
		}
	
	DmWrite(appInfoP, (UInt32) &nilP->renamedCategories, &renamedCategories,  
		sizeof(renamedCategories));


	// Set the ID of the last unique category ID.
	DmSet (appInfoP, (UInt32)&nilP->lastUniqID, 
		sizeof (appInfoP->lastUniqID), dmRecNumCategories - 1);

	MemPtrUnlock (localizedAppInfoP);
}





/***********************************************************************
 *
 * FUNCTION:    CategoryEditV20
 *
 * DESCRIPTION: This routine is the event handler for the "Categories Edit"
 *              dialog.
 *
 * PARAMETERS:  event    - a pointer to an EventType structure
 *              category - current category
 *              titleStrID - string id to use for the dialog's title
 *
 * RETURNED:    This routine returns true if any of the following conditions
 *					 are true:
 *					 	- the current category is renamed
 *					 	- the current category is deleted
 *					 	- the current category is merged with another category
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	10/30/97	Initial Revision
 *
 ***********************************************************************/
Boolean CategoryEditV20 (DmOpenRef db, UInt16 * categoryP, UInt32 titleStrID)
{
	return CategoryEdit (db, categoryP, titleStrID, 1);
}


/***********************************************************************
 *
 * FUNCTION:    CategoryEditV10
 *
 * DESCRIPTION: This routine is the event handler for the "Categories Edit"
 *              dialog.
 *
 * PARAMETERS:  event    - a pointer to an EventType structure
 *              category - current category
 *
 * RETURNED:    This routine returns true if any of the following conditions
 *					 are true:
 *					 	- the current category is renamed
 *					 	- the current category is deleted
 *					 	- the current category is merged with another category
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	2/21/95	Initial Revision
 *			roger	4/16/96	Changed to MemHandle a different dialog 
 *			trm	08/07/97	made non modified passed variabls constant
 *
 ***********************************************************************/
Boolean CategoryEditV10 (DmOpenRef db, UInt16 * categoryP)
{
	return CategoryEdit (db, categoryP, categoryEditStrID, 1);
}

/***********************************************************************
 *
 * FUNCTION:    CategorySelectV10
 *
 * DESCRIPTION: This routine process the selection and editting of
 *              categories. 
 *
 * PARAMETERS:  db           - database the contains the categories
 *              frm          - form the contains the category popup list
 *              ctlID        - id of the popup trigger
 *              lstID        - id of the popup list
 *              title        - true if the popup trigger is on the title line
 *              categoryP    - current category (index into db structure)
 *              categoryName - name of the current category
 *
 * RETURNED:    This routine returns true if any of the following conditions
 *					 are true:
 *					 	- the current category is renamed
 *					 	- the current category is deleted
 *					 	- the current category is merged with another category
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	3/10/95	Initial Revision
 *			trm	08/07/97	made non modified passed variabls constant
 *			gap	08/13/99	Update to use new constants categoryHideEditCategory & categoryDefaultEditCategoryString.
 *
 ***********************************************************************/
Boolean CategorySelectV10 (DmOpenRef db, const FormType * frm, 
	UInt16 ctlID, UInt16 lstID,	Boolean title, 
	UInt16 * categoryP, Char * categoryName)

{
	return CategorySelect (db, frm, ctlID, lstID,	title, categoryP, categoryName, 1, categoryDefaultEditCategoryString);
}


/***********************************************************************
 *
 * FUNCTION:    CategoryCreateListV10
 *
 * DESCRIPTION: Read a database's categories and set the categories 
 *              list's items.
 *
 *	THIS IS THE OLD VERSION 1.0 CALL WHICH WILL DISAPPEAR IN THE FUTURE.
 *
 * PARAMETERS:  db - opened database containing category info
 *					 lst - list in which to place the categories
 *					 currentCategory - category to select
 *					 showAll - true to have an all category
 *
 * RETURNED:    nothing.
 *
 * HISTORY:
 *		02/22/95	rsf	Created by Roger Flores.
 *		08/13/99	gap	Update to use new constants categoryHideEditCategory
 *							& categoryDefaultEditCategoryString.
 *
 ***********************************************************************/
void CategoryCreateListV10 (DmOpenRef db, ListType * lst, 
	UInt16 currentCategory, Boolean showAll)
{
	CategoryCreateList (db, lst, currentCategory, showAll, true, 1, categoryHideEditCategory, true);
}


/***********************************************************************
 *
 * FUNCTION:    CategoryFreeListV10
 *
 * DESCRIPTION: This routine unlocks or frees memory locked or allocated
 *              by CategoryCreateList.
 *
 * PARAMETERS:  lst   - pointer to the category list
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	2/23/95	Initial Revision
 *			roger	9/9/97	fixed call to CategoryFreeList to not free an editing string
 *
 ***********************************************************************/
void CategoryFreeListV10 (DmOpenRef db, ListType* lst)
{
	CategoryFreeList(db, lst, false, 0);
	
	// The old method didn't know if the All string was used so it
	// reset the lock.  This prevents nested lists.
	// DOLATER - this has a missing DmReleaseResource call, but there
	// needs to be some way of remembering the original resource handle.
	MemHandleResetLock(DmGetResource(strRsc, categoryAllStrID));
}
