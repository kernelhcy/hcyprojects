/******************************************************************************
 *
 * Copyright (c) 1997-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: ImcUtils.c
 *
 * Release: 
 *
 * Description:
 *      Routines to Handle Internet Mail Consortium specs
 *
 * History:
 *		04/24/97	roger	Created
 *		05/28/98	kwk	In ImcWriteQuotedPrintable, modified to work with
 *							non-Latin character sets.
 *					kwk	In ImcStringIsAscii, call TxtStrEncoding to do
 *							the actual analysis.
 *		06/29/98	kwk	Rolled back in changes, this time conditional
 *							on LANGUAGE == LANGUAGE_JAPANESE.
 *		03/15/99	kwk	Fixed sizeof(char constant) -> sizeOf7BitChar(xxx).
 *		06/02/00	CS		Include PalmLocale.h to get at charEncodingAscii, etc.
 *
 *****************************************************************************/

#include <PalmTypes.h>

#include <Chars.h>
#include <ErrorBase.h>
#include <ExgMgr.h>
#include <IMCUtils.h>
#include <MemoryMgr.h>
#include <StringMgr.h>
#include <TextMgr.h>
#include <PalmLocale.h>	// charEncodingAscii, etc.

#include <PalmUtils.h>

Char * ImcReadFieldNoSemicolon(void * inputStream, GetCharF inputFunc,
											UInt16 * c, const UInt16 maxChars)
{
	MemHandle resultH;
	Char * resultP;
	Int16 resultSize = min(maxChars+1, 16 * sizeof(Char));
	Int16 resultUsed = 0;
	
	
	resultH = (MemHandle) MemHandleNew(resultSize);
	resultP = (Char *) MemHandleLock(resultH);
	
	while ((*c == spaceChr || *c == tabChr) && *c != EOF)
		{
		*c = inputFunc(inputStream);
		}
	
	while (*c != endOfLineChr && *c != ';' && *c != EOF)
		{
		// If the character is an escape character accept the next character, 
		// whatever it is.
		if (*c == '\\')
			*c = inputFunc(inputStream);
		
		// Use the character unless it's the second part of =0D=0A sequence.  In that
		// case overwrite the =0D character.  The issue is PalmOS uses =0A instead of =0D=0A.
		if (*c == 0x0A && resultUsed > 0 && resultP[resultUsed - 1] == 0x0D)
			resultP[resultUsed - 1] = 0x0A;
		// if we have a full buffer, simply don't add more characters, just drop them.
		else if (resultUsed < maxChars)
			resultP[resultUsed++] = *c;
		*c = inputFunc(inputStream);
		
		// Expand the result if there's no more room.  Do this after the last character
		// because there always needs to be room so that a null terminator can be added.
		if (resultUsed == resultSize && resultSize < maxChars+1)
			{
			resultSize += 16;
			if (resultSize > maxChars+1)
				resultSize = maxChars+1;
			
			MemHandleUnlock(resultH);
			if (MemHandleResize(resultH, resultSize))
				{
				MemHandleFree(resultH);
				ErrThrow(exgMemError);
				}
			resultP = (Char *) MemHandleLock(resultH);
			}
		}
	
	
	if (resultUsed > 0)
		{
		// Terminate the field
		resultP[resultUsed] = nullChr;
		}
	else
		{
		MemHandleUnlock(resultH);
		MemHandleFree(resultH);
		resultP = NULL;
		}
	
	// Advance past any semicolon character
	if (*c == ';')
		*c = inputFunc(inputStream);
	
	return resultP;
}


Char * ImcReadFieldQuotablePrintable(void * inputStream, GetCharF inputFunc, 
	UInt16 * c, const Char stopAt, const Boolean quotedPrintable, const UInt16 maxChars)
{
	MemHandle resultH;
	Char * resultP;
	Int16 resultSize = min(maxChars+1, 16 * sizeof(Char));
	Int16 resultUsed = 0;
	UInt8 highNibble;
	UInt8 lowNibble;
	Boolean quotedChar = false;
	
	
	resultH = (MemHandle) MemHandleNew(resultSize);
	resultP = (Char *) MemHandleLock(resultH);
	
	while ((*c == spaceChr || *c == tabChr) && *c != EOF)
		{
		*c = inputFunc(inputStream);
		}
	
	goto processChar;
	
	while (quotedChar || (*c != endOfLineChr && *c != stopAt && *c != EOF))
		{
		// Use the character unless it's the second part of =0D=0A sequence.  In that
		// case overwrite the =0D character.  The issue is PalmOS uses =0A instead of =0D=0A.
		if (*c == 0x0A && resultUsed > 0 && resultP[resultUsed - 1] == 0x0D)
			resultP[resultUsed - 1] = 0x0A;
		// if we're maxed out, simply don't add more characters, just drop them.
		else if (resultUsed < maxChars)
			resultP[resultUsed++] = *c;
		*c = inputFunc(inputStream);
		quotedChar = false;
		
		// Handle quoted printable values.  Don't assume a hex value follows a '=' because
		// it certainly doesn't happen at the end of a line.
processChar:
		while (*c == '=' && quotedPrintable && *c != EOF)
			{
			*c = inputFunc(inputStream);
			
			// Handle '=' at the end of the line signifying to continue reading from the next line
			if (*c < '0' || 'F' < *c || ('9' < *c && *c < 'A'))
				{
				if (*c == endOfLineChr)
					{
					*c = inputFunc(inputStream);
					if (*c == 0x0A)
						{
						*c = inputFunc(inputStream);
						}
					}
				}
			else
				{
				// Decode the two digit hex value into a char.  The first digit is checked above
				// to be a hex character.
				if (*c <= '9')
					highNibble = *c - '0';
				else
					highNibble = *c - 'A' + 10;
				
				// Check the second character to make sure it's a hex character.  The spec states
				// it must be but we still make sure and do something intelligent otherwise.
				*c = inputFunc(inputStream);
				if ('0' <= *c && *c <= '9')
					lowNibble = *c - '0';
				else if ('A' <= *c && *c <= 'F')
					lowNibble = *c - 'A' + 10;
				else
					{
					lowNibble = highNibble;
					highNibble = 0;
					}
				
				*c = highNibble << 4 | lowNibble;
				quotedChar = true;
				
				// We have now read in a character.  Use it.  Specifically, avoid the while
				// loop condition because the character might be an encoded '='.
				break;
				}
			}
		
		// Expand the result if there's no more room.  Do this after the last character
		// because there always needs to be room so that a null terminator can be added.
		if (resultUsed == resultSize && resultSize < maxChars+1)
			{
			resultSize += 16;
			if (resultSize > maxChars+1)
				resultSize = maxChars+1;
			
			MemHandleUnlock(resultH);
			if (MemHandleResize(resultH, resultSize))
				{
				MemHandleFree(resultH);
				ErrThrow(exgMemError);
				}
			resultP = (Char *) MemHandleLock(resultH);
			}
		}
	
	
	if (resultUsed > 0)
		{
		// Terminate the field
		resultP[resultUsed] = nullChr;
		}
	else
		{
		MemHandleUnlock(resultH);
		MemHandleFree(resultH);
		resultP = NULL;
		}
	
	// Advance past any stopAt character unless we're at the end of the line.
	if (*c == stopAt && *c != endOfLineChr)
		*c = inputFunc(inputStream);
	
	return resultP;
}


// Read everything up until a property delimiter (;) or end of properties (:) or end of line.
// Return property in two fields, a name and a value.  If the property has no name, then
// nameP will be "" and valueP will contain the (presumably unambiguous) value
// if nameP is null, it won't keep the names at all.
void ImcReadPropertyParameter(void * inputStream, GetCharF inputFunc,
										UInt16 * cP, Char * nameP, Char * valueP)
{
	UInt16 c = *cP;
	Int16 tokenEnd = 0;

	if (nameP != NULL)
		nameP[0] = nullChr;
	
	while (c != valueDelimeterChr && c != parameterDelimeterChr && c != linefeedChr && c != EOF)
	{
		// add non-whitespace characters to value
		// DOLATER ??? - not quite correct, spec is "ws word ws '=' ws word ws', we handle ws in mid-word...
		if (c != spaceChr && c != tabChr && c != paramaterNameDelimiterChr)
	 		valueP[tokenEnd++] = (Char) c;

		// if we found a name delimiter ('='), then move value into name
		// and start parsing value again
		if (c == paramaterNameDelimiterChr)
		{
			if (nameP != NULL)
			{
				StrNCopy(nameP, valueP, tokenEnd);
				nameP[tokenEnd] = nullChr;
			}
			tokenEnd = 0;	// start parsing again
		}
	 	
		c = inputFunc(inputStream);
	}
	valueP[tokenEnd++] = nullChr;
	
	*cP = c;
}


void ImcSkipAllPropertyParameters(void * inputStream, GetCharF inputFunc, UInt16 * cP, Char * identifierP, Boolean *quotedPrintableP)
{
	// Parse the property parameters.
	while (*cP != EOF && *cP != valueDelimeterChr)
		{
		*cP = inputFunc(inputStream);			// consume the valueDelimeterChr
		
		ImcReadPropertyParameter(inputStream, inputFunc, cP, NULL, identifierP);
		
		if (StrCaselessCompare(identifierP, "QUOTED-PRINTABLE") == 0)
			{
			*quotedPrintableP = true;
			}
		}
	if (*cP != EOF)	
		*cP = inputFunc(inputStream);
}

/***********************************************************************
 *
 *	FUNCTION:	ImcReadWhiteSpace
 *
 *	DESCRIPTION: Keep reading from <inputFunc> until we skip over all of
 *		the spaces. This routine is only used to access data in the vCard
 *		format which is guaranteed to be low-ascii, thus we don't need to
 *		worry about inputFunc returning either whole or partial double-
 *		byte characters
 *
 *	PARAMETERS:
 *		inputStream		->
 *		inputFunc		->
 *		charAttrP		->	Ptr to Latin char attribute table, or NULL.
 *		c					<-	First non-space character, or EOF.
 *
 *	RETURNS:		Nothing
 *
 *	HISTORY:
 *		??/??/??	rsf	Created by Roger Flores.
 *		10/07/99	kwk	If charAttrP is passed in, use it.
 *		10/05/00	kwk	Use new char attr masks from TextMgr.h, vs. IsSpace().
 *
 ***********************************************************************/
void ImcReadWhiteSpace(void * inputStream, GetCharF inputFunc, 
								const UInt16* charAttrP, UInt16 * c)
{
	if (charAttrP == NULL)
		{
		while ((*c != EOF) && TxtCharIsSpace(*c))
			{
			*c = inputFunc(inputStream);
			}
		}
	else
		{
		while ((*c != EOF) && (charAttrP[(UInt8)(*c)] & (charAttr_CN|charAttr_SP|charAttr_XS)))
			{
			*c = inputFunc(inputStream);
			}
		}
}


/**********************************************************************
 * Export to vCard format.  Useful for other formats.
 *
 * The quoted-printable format needs to be added for support multiple 
 * lines and non ascii letters.
 *
 * Replace the strcat stuff with something that keeps track of the end 
 * of the buffer for efficiency.
 *
 * DoAddrExportImc <access ptr> <file out>
 ***********************************************************************/

#define quotedPrintableLengthMax 76


void ImcWriteQuotedPrintable(
	void * outputStream,			// Where to export the record to
	PutStringF outputFunc, 
	const Char * stringP,
	const Boolean noSemicolons)
{
	const Char * c;
	const Char * segmentP;
	Char hexString[4];
	UInt8 hexDigit;
	Boolean quotedPrintable = false;
	CharEncodingType minEncoding = charEncodingAscii;
	Int16 charsWritten;
	Int16 charsToWrite;
	Char charToNotWrite;		// This char cannot appear as is.  It must be quoted.
									// This is used to MemHandle strings with a ';' when 
									// the spec has multiple strings on a line 
									// separated by ';' chars.
	
	if (stringP == NULL)
		return;
	
	c = stringP;
	if (noSemicolons)
		charToNotWrite = ';';
	else
		charToNotWrite = linefeedChr;
	
	// Check to see if we can simply emit the string as is.
//	while ((*c >= ' ' && *c < 127 && *c != '=') || *c == '\t')
//		c++;

	// If the string starts with a spaceChr or tabChr then it must be quoted printable
	// so that the staring spaceChr or tabChr isn't stripped.
	if (stringP[0] == spaceChr || stringP[0] == tabChr)
		quotedPrintable = true;
	
	// DOLATER kwk - Shouldn't this loop be checking for '=' and chars with the
	// high bit on, to set quotedPrintable true?
	
	while (*c != nullChr)
		{
		WChar curChar;
		c += TxtGetNextChar((const Char *)c, 0, &curChar);
		minEncoding = TxtMaxEncoding(minEncoding, TxtCharEncoding(curChar));
		
		if (curChar < spaceChr || curChar == charToNotWrite)
			{
			quotedPrintable = true;
			}
		}
	
	// If the string ends with a spaceChr or tabChr then it must be quoted printable
	// so that the ending spaceChr or tabChr isn't dropped.
	if (c > stringP && (*(c - 1) == spaceChr || *(c - 1) == tabChr))
		quotedPrintable = true;
	
	else if (c - stringP > quotedPrintableLengthMax)
		quotedPrintable = true;
	
	
	if (minEncoding != charEncodingAscii)
		{
		outputFunc(outputStream, ";CHARSET=");
		outputFunc(outputStream, TxtEncodingName(minEncoding));
		}
	
	// If the string didn't have any special characters and 
	// if it doesn't have a space or a tab as the last character and
	// If it doesn't exceed the 72 characters in length then write normally
	if (!quotedPrintable)
		{
		outputFunc(outputStream, ":");
			
		outputFunc(outputStream, stringP);
		}
	else
		{
		outputFunc(outputStream, ";ENCODING=QUOTED-PRINTABLE:=\015\012");
		
		charsWritten = 0;				// We just started a new line so this is reset to zero
		
		// Set these once
		hexString[0] = '=';
		hexString[3] = '\0';
		
		
		c = stringP;
		do
			{
			segmentP = c;
			
			// The chars are written in chunks of two parts.  The first part is as a
			// string of normal characters.  The second is a char as quoted sequence.
			// DOLATER kwk - figure out if this really works for multibyte characters.
			while (((UInt8) *c >= spaceChr || *c == tabChr) && *c != '=' && *c != charToNotWrite)
				c++;
			
			// Are there any characters to write normally?
			charsToWrite = c - segmentP;
			if (charsToWrite > 0)
				{
				Char temp[quotedPrintableLengthMax + sizeOf7BitChar(chrCarriageReturn) + sizeOf7BitChar(chrLineFeed) + sizeOf7BitChar(chrNull)];
				
				while (charsWritten + charsToWrite > quotedPrintableLengthMax - 1)
					{
					charsToWrite = quotedPrintableLengthMax - 1 - charsWritten;
					
					MemMove(temp, segmentP, charsToWrite);
					temp[charsToWrite] = nullChr;
					outputFunc(outputStream, temp);
					outputFunc(outputStream, "=\015\012");
					
					segmentP += charsToWrite;
					charsToWrite = c - segmentP;
					charsWritten = 0;
					}
					
				MemMove(temp, segmentP, charsToWrite);
				temp[charsToWrite] = nullChr;
				outputFunc(outputStream, temp);
				charsWritten += charsToWrite;
				}
			
			// If we found the end of the string then we're done.
			// First make sure that if there were any trailing space or tabs to 
			// follow them with a soft break '='.
			if (*c == '\0')
				{
				if (c > stringP && ((*(c - 1) == spaceChr) || (*(c - 1) == tabChr)))
					{
					// charsWritten < quotedPrintableLengthMax
					// Write a blank new line because otherwise the '=' indicates that the
					// next line is part of this line.
					outputFunc(outputStream, "=\015\012");
					}
				break;
				}
			
			// Now write the quoted-printable sequence
			if (*c == linefeedChr)
				{
				// Make sure there is room for the quoted newline sequence
				if (charsWritten + 7 >= quotedPrintableLengthMax)
					{
					outputFunc(outputStream, "=\015\012");
					charsWritten = 0;
					}
				
				// We aren't required to start a new line but it's a bit more readable.
				outputFunc(outputStream, "=0D=0A=\015\012");
				charsWritten = 0;
				}
			else
				{
				// Convert the byte's high nibble to a ascii hex digit
				hexDigit = (UInt8) *c >> 4;
				if (hexDigit < 10)
					hexString[1] = '0' + hexDigit;
				else
					hexString[1] = 'A' + hexDigit - 10;
				
				// Convert the byte's low nibble to a ascii hex digit
				hexDigit = *c & 0x0F;
				if (hexDigit < 10)
					hexString[2] = '0' + hexDigit;
				else
					hexString[2] = 'A' + hexDigit - 10;
				
				// Make sure there is room for the quoted value
				if (charsWritten + 3 >= quotedPrintableLengthMax)
					{
					outputFunc(outputStream, "=\015\012");
					charsWritten = 0;
					}
				
				outputFunc(outputStream, hexString);
				charsWritten += 3;
				
				}
			
			// Advance to the next section.
			c++;
			} while (*c != '\0');
		}
}


// DOLATER kwk - figure out if this could ever be called with multibyte text.
// If so, then scanning for a semicolon probably won't work.

void ImcWriteNoSemicolon(
	void * outputStream,			// Where to export the record to
	PutStringF outputFunc, 
	const Char * const stringP)
{
	const Char * c;
	Char temp[2];
	
	
	if (stringP == NULL)
		return;
	
	c = stringP;
	
	temp[1] = '\0';
	
	while (*c != nullChr)
		{
		// Semicolons must be escaped with a '\' char 
		if (*c == ';')
			{
			outputFunc(outputStream, "\\;");
			}
		// Linefeeds must be escaped with a '\' char 
		else if (*c == linefeedChr)
			{
			outputFunc(outputStream, "\\\015\012");
			}
		// '\' must be escaped with another '\' char 
		else if (*c == '\\')
			{
			outputFunc(outputStream, "\\\\");
			}
		else
			{
			temp[0] = *c;
			outputFunc(outputStream, temp);
			}

		c++;
		}
}


Boolean ImcStringIsAscii(const Char * const stringP)
{
	if (stringP == NULL) {
		return(true);
	} else {
		return(TxtStrEncoding((Char const *)stringP) == charEncodingAscii);
	}
}
