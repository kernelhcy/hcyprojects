/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: ClipBoard.c
 *
 * Release: 
 *
 * Description:
 *	  This file contains the clipboard routines.
 *
 * History:
 *		December 14, 1994	Created by Art Lamb
 *			Name	Date		Description
 *			----	----		-----------
 *			Ben		12/14/98	Added ClipboardAppendItem
 *			bob		12/16/98	Added notes, describe text item format
 *
 *****************************************************************************/

#define NON_PORTABLE

// Allow access to data structure internals
#define ALLOW_ACCESS_TO_INTERNALS_OF_CLIPBOARDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_FIELDS
#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS

#include <SystemPublic.h>

#include "Clipboard.h"
#include "UIGlobals.h"


/*

Clipboard Notes,
added 12/16/98
--Bob

Clipboard is currently set up to support 3 formats: text, ink, and bitmap,
defined as follows:

	Text:
		simply a run of characters, NOT null-terminated.  This facilitates
		the field package, which adds text directly from the text MemHandle.
		(May have to re-think this for multi-byte character support?)
	
	Ink:
		undefined (?)
		
	Bitmap:
		presumably a BitmapType, from WindowNew.h

To the best of our knowledge, only text is actually used by any client.
We might consider getting rid of ink and bitmap types at some point,
to plug potential dynamic heap leaks.


Each clipboard format is stored seperately, and nothing is done to attempt
to keep them in sync.  The only time an old item is deleted is when a
new item is added.  Adding a new item does not change the state of any
other format item.  That is, the clipboard is really THREE clipboards, one
each for text, ink, and bitmap.  (Again, only text is commonly used.)


Memory for the clipboard is allocated from the dynamic heap.  In the future,
it may instead be allocated on the storage heap, to help reduce impact
on the dynamic heap.  This should not impact clients that do not attempt
unsupported operations on the MemHandle returned by ClipboardGetItem.

A clipboard item can be cleared by calling ClipboardAddItem with either
a zero length or a NULL pointer.


ClipboardGetItem returns the MemHandle to the actual system-owned clipboard
chunk.  It should be treated as read-only, and quickly used and discarded.

That is, for long term operations the client must move the data to its own
storage, and then forget about the MemHandle.  e.g. paste the characters into
a field.  The next ClipboardAddItem call of the same item type will cause
this MemHandle to be invalidated (it is freed.)

A client should not modify the MemHandle returned from ClipboardGetItem.
Similarly, it is not suitable for passing to ClipboardAddItem, or other
API which modifies that memory (like FldSetTextHandle)


ClipboardAppendItem was added for 3.2, mainly for Clipper.  It facilitates
creating a large text item from several small disjoint pieces.  It simply
does a binary append of the new item to the end of the old item, no attempt
is made to understand/parse the format.  For text formats, binary append
makes sense, but it is probbaly meaningless for other formats.

The intent of ClipboardAppendItem is that it is called several times over
a relatively small interval, building up a composite text run that other
apps will see as the real clipboard.  It's an interesting (and untested)
edge case for some other app or hack to try to read the clipboard before
the appending is complete.  (Since text runs are self contained, it should
not be a bug, but... who knows.)


Some thought was given to adding API to better manage clipboard memory,
e.g. ClipboardAssumeItem, which would not make a new copy of the passed
handle but merely assume ownership, and ClipboardReleaseItem, which would
release ownership of the MemHandle to the calling application, emptying the
clipboard.  These were rejected in favor of the ability to move the clipboard
memory to the storage heap in the future.
 
*/

/***********************************************************************
 *
 * FUNCTION:    PrvAddItem
 *
 * DESCRIPTION: Add the item passed to the clipboard specified.  The 
 *              format parameter determines which which clipboard 
 *              (text, ink, etc) the item is added to.
 *
 * PARAMETERS:  format	text, ink, bitmap, etc
 *              ptr		pointer to the item to place on the clipboard
 *              length  size of the the item to place on the clipboard
 *
 * RETURNED:    Zero on success, or memErrNotEnoughSpace on failure.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art		12/14/94	Initial Revision
 *			trm		08/07/97	made non modified passed variabls constant
 *			Ben		12/14/98	Moved to PrvAddItem with return Err,
 *								added checks for allocation failures.
 *
 ***********************************************************************/
static Err PrvAddItem (const ClipboardFormatType format, const void * ptr, 
	UInt16 length)
{
	MemHandle itemH;
	void * itemP;

	ErrFatalDisplayIf( (format >= numClipboardForamts), 
		"Invalid clipboard format");

	// If the clipboard already contains an item, free it.
	if (Clipboard[format].item)
		{
		MemHandleFree (Clipboard[format].item);
		
		// do this now, in case the allocation below fails:
		Clipboard[format].item = NULL;
		Clipboard[format].length = 0;
		}

	if (ptr && length)
		{
		itemH = MemHandleNew (length);
		if (itemH == NULL)
			{
			return memErrNotEnoughSpace;
			}
		itemP = MemHandleLock (itemH);

		// Change the owner of the clipboard item such that it's owned by the 
		// system,  so that it will not be freed when the current application
		// terminates.
		MemPtrSetOwner (itemP, 0);

		MemMove (itemP, ptr, length);
		MemPtrUnlock (itemP);
		}
	else
		{
		itemH = NULL;
		length = 0;
		}	

	Clipboard[format].item = itemH;
	Clipboard[format].length = length;
	return 0;
}


/***********************************************************************
 *
 * FUNCTION:    ClipboardAddItem
 *
 * DESCRIPTION: Add the item passed to the clipboard specified.  The 
 *              format parameter determines which which clipboard 
 *              (text, ink, etc) the item is added to.
 *
 * PARAMETERS:  format	text, ink, bitmap, etc
 *              ptr		pointer to the item to place on the clipboard
 *              length  size of the the item to place on the clipboard
 *
 * RETURNED:    nothing.
 *
 * REVISION HISTORY:
 *			Name	Date	Description
 *			----	----	-----------
 *			art	12/14/94	Initial Revision
 *			trm	08/07/97	made non modified passed variabls constant
 *			Ben	12/14/98	Moved to PrvAddItem
 *
 ***********************************************************************/
void ClipboardAddItem (const ClipboardFormatType format, const void * ptr, 
	UInt16 length)
{
	PrvAddItem(format, ptr, length);
}


/***********************************************************************
 *
 * FUNCTION:    ClipboardAppendItem
 *
 * DESCRIPTION: Append the item passed to the clipboard specified.  The 
 *              format parameter determines which which clipboard 
 *              (text, ink, etc) the item is appended to.
 *
 * PARAMETERS:  format	text, ink, bitmap, etc
 *              ptr		pointer to the item to append to the clipboard
 *              length  size of the the item to append to the clipboard
 *
 * RETURNED:    Zero on success, or memErrNotEnoughSpace on failure.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			Ben		12/14/98	Initial Revision
 *
 ***********************************************************************/
Err ClipboardAppendItem (const ClipboardFormatType format, const void * ptr, 
	UInt16 length)
{
	MemHandle	itemH;
	Char *		itemP;
	UInt16		oldLength;
	Err			err;

	ErrFatalDisplayIf( (format >= numClipboardForamts), 
		"Invalid clipboard format");

	if (!ptr || !length)
		{
		return 0;
		}

	// If the clipboard does not already contain an item, just add it.
	if (!Clipboard[format].item)
		{
		return PrvAddItem(format, ptr, length);
		}

	itemH = Clipboard[format].item;
	oldLength = Clipboard[format].length;

	if ((err = MemHandleResize(itemH, oldLength + length)) != 0)
		{
		return err;
		}
	
	itemP = MemHandleLock (itemH);
	MemMove(itemP + oldLength, ptr, length);
	MemPtrUnlock (itemP);
	
	Clipboard[format].length = oldLength + length;
	return 0;
}


/***********************************************************************
 *
 * FUNCTION:    ClipboardGetItem
 *
 * DESCRIPTION: This routine returns a pointer to the contents of the
 *              specified clipboard and the length of the clipboard item.
 *
 * PARAMETERS:  format	text, ink, bitmap, etc
 *              length  pointer to the length of the clipboard item
 *
 * RETURNED:    Pointer to clipboard item (owned by system)
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	12/14/94		Initial Revision
 *			trm	08/07/97	made non modified passed variabls constant
 *
 ***********************************************************************/
MemHandle ClipboardGetItem (const ClipboardFormatType format, 
	UInt16 * length)
{
	ErrFatalDisplayIf( (format >= numClipboardForamts), 
		"Invalid clipboard format");

	*length = Clipboard[format].length;
	return (Clipboard[format].item);
}
