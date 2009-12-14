/******************************************************************************
 *
 * Copyright (c) 1996-1999 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: FloatMgr.c
 *
 * Release: 
 *
 * Description:
 *		New Floating point routines, provided by new IEEE arithmetic
 *		68K software floating point emulator (sfpe) code.
 *
 * History:
 *    9/23/96 - Created by Steve Lemke
 *   10/16/96 - Fixed rounding problem in PrvFConvert (use div not mul)
 *   11/26/96 - Added FlpCorrectedAdd and FlpCorrectedSub routines
 *   12/30/96 - Added FlpVersion and FlpSelectorErrPrv routines
 *   12/10/97 - Added ThisShouldGenerateALinkError code to make sure
 *						flpSoftFPSCR doesn't move without our knowing about it!
 *   12/10/97 - FlpBase10Info now returns _positive_ zero, not _negative_
 *   12/18/97 - flpSoftFPSCR validation is now only done on ROM builds 
 *		7/21/99 - NewFloatMgr.h has become FloatMgr.h.
 *
 *****************************************************************************/

#define NON_PORTABLE

#include <PalmTypes.h>

#include "ErrorMgr.h"
#include "FloatMgr.h"
#include "StringMgr.h"

#include "Globals.h"		// for LowMemHdrType


#define kPowerBase 				99		// no longer used




#define FlpIsAnyInfinite(x) ( (__HI32(x)<<1)==(0xfff00000<<1) && __LO32(x)==0 )
#define FlpIsPosInfinite(x) ( __HI32(x)==0x7ff00000 && __LO32(x)==0 )
#define FlpIsNegInfinite(x) ( __HI32(x)==0xfff00000 && __LO32(x)==0 )
#define FlpIsNotANumber(x) ( __HI32(x)==0x7FFFFFFF && __LO32(x)==0xFFFFFFFF )




/************************************************************************
 * Private Routines
 ***********************************************************************/
static Err					PrvFConvert(FlpDouble a, Int16* exp, UInt32* iRes);
static double PrvPow10(Int16 i);

void ThisShouldGenerateALinkError(void);



/***********************************************************************
 *
 * FUNCTION: 	 FlpBase10Info
 *
 * DESCRIPTION: Extract detailed information on the base 10 form of a floating point
 *              number : the base 10 mantissa, exponent, and sign.
 *              The mantissa will be normalized such that it contains at least 
 *              8 significant digits when printed as an integer value.
 *
 * PARAMETERS:  a - the floating point number
 *              *mantissaP - will contain the base 10 mantissa
 *              *exponentP - will contain the base 10 exponent
 *              *signP - will contain the sign bit, 0 or 1; NOT 1 or -1
 *
 * RETURNED:	 error code, 0 = no error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			SCL	12/10/97	v3.0: if a is zero, signP now returns zero instead of one
 *			PKW	11/14/00	changed to use new version of PrvFConvert()
 *
 ***********************************************************************/
Err FlpBase10Info(FlpDouble a, UInt32* mantissaP, Int16* exponentP, Int16* signP)
{
	Int16			exp;
	UInt32			iRes;
	Err			error;
	
	// Make sure our floating point global didn't move!!
	// If the following fails, it means that "flpSoftFPSCR" (in <Globals.h>)
	// moved.  This is very bad, unless the corresponding assembly language
	// equate is fixed in Pilot:Libraries:NewFloatMgr:mc68ksfpe.s!!!
	// Of course, this is not necessary when building the simulator (it fails!).
#if EMULATION_LEVEL == EMULATION_NONE
	if ( (UInt16)(&(((LowMemHdrType*)PilotGlobalsP)->globals.flpSoftFPSCR)) != 0x0298)
		ThisShouldGenerateALinkError();
#endif


	// Check for 0
	if ( FlpIsZero(a) ) {
		*mantissaP = 0;
		*exponentP = 1;
		*signP = 0;
		return 0;
		}
	
	
	// convert the number to something usable, and check its validity
	error = PrvFConvert(a, &exp, &iRes);
	if (error) return error;
	
	// set results
	*mantissaP = iRes;
	*exponentP = exp;
	*signP = FlpGetSign(a);
	return 0;
}


/***********************************************************************
 *
 * FUNCTION: 	 FplFToA
 *
 * DESCRIPTION: Convert a floating point number to a zero terminated ascii string 
 *              in exponential format : [-]x.yyyyyyyye[-]zz
 *
 * PARAMETERS:  a - the floating point number
 *              s - pointer to buffer to contain the ascii string
 *                  presumed to be long enough!
 *
 * RETURNED:	 error code, 0 = no error
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			PKW	11/14/00	rewrite: outputs 0 as 0.0000000e00; outputs
 *								INF, -INF, and NaN if appropriate;
 *								outputs values whose exponent is 3 digits
 *
 ***********************************************************************/
Err FlpFToA(FlpDouble a, Char* s)
{
	Int16			exp;
	UInt32			iRes;
	Char*			s2 = s;
	Err				error;
	int i;

	if (FlpIsZero(a))
	{
		// Right now both positive and negative 0 are printed the
		// same.  We might want to call FlpGetSign() to determin the sign and
		// stick an "-" in front of the output. 
		StrCopy(s, "0.0000000e00");
		return 0;
	}
	else if (FlpIsPosInfinite(a))
	{
		StrCopy(s, "INF");
		return flpErrOutOfRange;
	}
	else if (FlpIsNegInfinite(a))
	{
		StrCopy(s, "-INF");
		return flpErrOutOfRange;
	}
	else if (FlpIsNotANumber(a))
	{
		StrCopy(s, "NaN");
		return flpErrOutOfRange;	// perhaps we need a flpErrNaN constant?
	}
	else
	{
		// convert the number to something usable, and check its validity
		error = PrvFConvert(a, &exp, &iRes);
		if (error) return error;
	}


	// add minus sign if necessary
	if ( FlpGetSign(a) ) *s2++ = '-';
	
	// Since we'll shift the decimal point 7 digits when we
	// output the string...
	exp += 7; 



	// Construct the decimal porition.  Since the / and % work on the
	// integer "backwards," start by skipping to the end of the string
	// and moving backwards through the string.
	// Documentation specifically says that output is of form "x.yyyyyyyy"
	// So, don't quit generating digits just because the "iRes" is zero.
	// Also, although the documentation specifies a total of 9 digits of
	// accuracy, the previous version of the OS only output 8.  So, continue
	// to output only 8 for backwards compatibility.
	s2 += (8 + 1);  // 8 digits and a decimal point
	for (i=0; i<8; i++)
	{
		if (i == 7) *--s2 = '.';
		*--s2 = iRes % 10 + '0';
		iRes /= 10;
	}
	s2 += (8 + 1);


	// Construct the exponent at the end of the existing string;
	// add a minus sign if necessary.
	// If we're going to need 3 digits for the exponent, remove the last digit from s2
	// so that we don't create a string larger than the previous maximum.
	// Optionally, we could leave the extra digit if the exponent is
	// > 99 because the total maximum string size won't increase in this case.
	if (exp < -99 || exp > 99) s2--;
	*s2++ = 'e';
	if (exp < 0)
	{
		exp = -exp;
		*s2++ = '-';
	}
	if (exp > 99)
	{
		*s2++ = (exp / 100) + '0';
		exp %= 100;
	}
	*s2++ = (exp / 10) + '0';
	*s2++ = (exp % 10) + '0';
	*s2++ = '\0';
	
	return 0;
}


/***********************************************************************
 *
 * FUNCTION: 	 FlpAToF
 *
 * DESCRIPTION: Convert a zero terminated ascii string to a floating point number.
 *              The string must be in the format : [-]x[.]yyyyyyyy[e[-]zz]
 *
 * PARAMETERS:  s - pointer to the ascii string
 *
 * RETURNED:	 the floating point number
 *
 
 *
 * CHANGES: PKW 11/14/00;  This function is now close to ANSI C library compatible.
 *			ANSI requires the form:
 *			[+|-]digits[.][digits][(e|E)[+|-]digits]
 *			In order to maintain backward compatibility with the previous
 *			versions of PalmOS, the function considers ALL THREE of the
 *			"digits" sections to be optional.  Here's a table showing
 *			some deformed strings and what will happen.
 *
 *			String		old Palm	new Palm	ANSI		notes
 *			"+"			+0			+0			invalid		ANSI requires a digit
 *			".3	"		0.3			0.3			invalid		ANSI requires leading digit
 *			"0.3e123"	0.3e12		0.3e123		0.3e123		old Palm only allows 1 or 2 digit exponenet
 *			"+1"		+0			1			1			old Palm doesn't allow leading '+'
 *			"1e+2"		1			1e2			1e2			old Palm doesn't allow '+' in exponent
 *			"0.3E3"		0.3			0.3e3		0.3e3		old Palm doesn't all capital 'E' exponent
 *			"4294967297"  1			4294967297	4294967297	old Palm used unsigned long and wrapped around 
 *
 ***********************************************************************/

FlpDouble FlpAToF(const Char* s)
{
	double		f;
	Int16		decPlaces, decExp;
	Boolean		negative = false;
	Char		c;


	// Get the sign
	if (s[0] == '-')
	{
		s++;
		negative = true;
	}
	else if (s[0] == '+')  // optional '+' sign
	{
		s++;
	}

	// Init
	decPlaces = -1;
	f = 0;

	{
		// Scan in digits and the '.' (decimal point).
		// Up to 4 decimial digits are held in the mantissa integer.
		// When the mantissa (n unsigned short) is about to overflow,
		// it is accumulated into the double value.  We use a 16-bit
		// unsigned integer for max efficiency on a mc68k CPU.
		Int16 dumpInt = 4;
		UInt16 man = 0;
		while (*s)
		{
			c = *s;
			if (c >= '0' && c<='9')
			{
				man = man * 10 + (c-'0');
				if (decPlaces >= 0) decPlaces ++;
				dumpInt -= 1;
			}
			else if (c == '.') decPlaces = 0;
			else break;
			s++;
			if (dumpInt == 0)
			{
				dumpInt = 4;
				f = f * 10000 + man;
				man = 0;
			}
		}
		
		// Put in remaining part of the mantissa.
		f = f * PrvPow10(4-dumpInt) + man;
			
		if (decPlaces < 0) decPlaces = 0;
	}


	if (negative)
	{
		f = -f;				// negate it
	}

	// Get the exponent
	decExp = 0;
	if (*s == 'e' || *s == 'E')
	{
		s++;
		negative = false;			// reset negative flag
		if (*s == '-')
		{
			negative = true;
			s++;
		}
		else if (*s == '+')		// optional '+' sign
		{
			s++;
		}
		
		while (*s >= '0' && *s <= '9')
		{
			decExp = decExp * 10 + (*s - '0');
			if (decExp > 999) decExp = 999;	// prevent exponent from overflowing
			s++;
		}

		if (negative)
			decExp = -decExp;
	}
	
	// Scale by the decPlace in powers of ten
	decExp -= decPlaces;
	f = f * PrvPow10(decExp);


	return *(FlpDouble*)&f;
}





/***********************************************************************
 *
 * FUNCTION: 	 PrvFConvert
 *
 * DESCRIPTION: Convert a floating point number to an integer number.
 *
 * PARAMETERS:  a - the floating point number
 *              *exp - exponent for the integer number
 *              *iRes - the integer number
 *
 * RETURNED:	 error if the floating point number is out of range.
 *
 * CHANGES: 	PKW 11/14/00;  This function has totally new guts.  it now
 *				handles numbers whose exponent is more than 99 or less
 *				than -99.  Its output is also a little different:
 *				on success, a == iRes * 10^exp .  Surprisingly,
 *				the old function treated the exp output differently.
 *
 ***********************************************************************/
static Err PrvFConvert (FlpDouble a, Int16* exp, UInt32* iRes)
{
	Int16 x = 0;
	Int16 xExtra = 0;
	double result;

	_fp_set_fpscr(0);
	
	if (FlpIsZero(a))
	{
		*exp = 8;
		*iRes = 0;
		return 0;
	}

	if (FlpIsNotANumber(a) || FlpIsAnyInfinite(a))
	{
		return flpErrOutOfRange; 
	}

	
	// Force it positive since we do all our work with positive numbers and then
	// compute the sign seperately.
	FlpSetPositive(a);

	// If the number we are converting is amazingly close to 0, scale it up some
	// right here.  That way, when we calculate "x" below we won't be forced
	// into an "x" that will overflow the floating-point system.
	if (*(double*)&a < 1e-300)
	{
		xExtra = -128;
		*(double*)&a = *(double*)&a * PrvPow10(128);
	}

	// Find x such that 10,000,000 <= a * 10^x < 100,000,000 .
	while ((result=*(double*)&a * PrvPow10(x)) >= 1e08)
	{
		if (result > 1.0e128) x -= 128;
		else if (result > 1.0e32) x -= 32;
		else if (result > 1.0e8) x -= 8;
		else x -= 4;
	}
	while ((result=*(double*)&a * PrvPow10(x)) < 1e07)
	{
		if (result < 1.0e-120) x += 128;
		else if (result < 1.0e-24) x += 32;
		else if (result < 1.0e00) x += 8;
		else if (result < 1.0e04) x += 4;
		else if (result < 1.0e06) x += 2;
		else x += 1;
	}

	// Now round the last digit up if necessary.
	// Since the _fp_round() routine doesn't seem to do the trick, 
	// we modify the result here.
	// _fp_round(flpToNearest);
	result += 0.5;
	if (result >= 1e08)
	{
		result *= 0.1;
		x -= 1;
	}

	
	// OK, now we have result = a * 10^x where the
	// result (as an integer) is exactly an 8 digit number.
	// (We could actually produce a 9-digit integer,
	// but need to maintain compatibility with previous version of converter routine.)
	*exp = -x + xExtra;
	*iRes = _d_dtou(*(FlpDouble*)&result);


	return 0;
}


/***********************************************************************
 *
 * FUNCTION:    DoCorrectedAddSub
 *
 * DESCRIPTION: Perform desired calculation based on the last result value and the 
 *              provided parameter.  The calculation result is returned
 *
 *	COMMENTS:	 In order to fix the "0.3 - 0.1 = = =" small-but-not-zero bug
 *					 (aka "1.0 - 0.1 = = = = = = = = = ="), in 2.0b2 we introduce
 *					 new code which compares the exponents of the operands to the
 *					 exponent of the result and forces the result to zero if appropriate.
 *					 
 *					 Adding or subtracting a big number and a small number produces a
 *					 result similar in magnitude to the big number.  But two numbers
 *					 that are similar in magnitude MAY produce (depending on their
 *					 signs) a result with a very small exponent (i.e. a large, but
 *					 negative exponent).  If the difference between the result's
 *					 exponent and that of the operands is very large (i.e. close
 *					 to the number of significant bits expressable by the mantissa)
 *					 then it is quite possible that the result should in fact be zero.
 *					 
 *					 Apple's Calculator DA doesn't seem to do anything special to
 *					 handle these cases, though it appears that the Windows95
 *					 Calculator does. The HP-15c and other hand-held calculators
 *					 don't seem to "remember" any digits past the ones they display.
 *					 
 *					 Besides the problem cases above, there exist cases where it
 *					 may be useful to retain accuracy in the low-order bits of
 *					 the mantissa, e.g.: 99999999 + 0.00000001 - 99999999.  However,
 *					 unless the fractional part is an exact (negative) power of two,
 *					 it is doubtful that what few bits of mantissa that are available
 *					 will be enough to properly represent the fractional value.  In
 *					 this example, the 99999999 requires 26 bits, leaving 26 bits for
 *					 the .00000001, which guarantees inaccuracy after the subtraction.
 *					 
 *					 The problem comes from the difficulty in representing decimal
 *					 fractions in binary (like 0.1).  After about three successive
 *					 additions or subtractions, errors begin to appear in the least
 *					 significant bits of the mantissa.  Then when the value represented
 *					 by the most significant bits of the mantissa is subtracted away,
 *					 the lsb's error is normalized and becomes the actual result when
 *					 in fact the result should've been zero.
 *					 
 *					 Deciding where to draw the line between when to force the result
 *					 to zero and when not to is, however, an interesting dilemma...
 *					 
 *					 0.3 - .1 = = = yields (minExp-tempExp) of -4 - -55 or 51
 *					 1.0 - .1 = = = = = = = = = =    yields -4 - -53 or 49
 *					 
 *					 99999999 + .00000001 - 99999999 yields 26 - -26 or 52
 *					 99999999 + .0000001  - 99999999 yields 26 - -24 or 50
 *					 99999999 + .000001   - 99999999 yields 26 - -20 or 46 (*)
 *					  (*)  when not "corrected", this result appears as 9.9837780e-07
 *					 99999999 + .00001    - 99999999 yields 26 - -17 or 43 (**)
 *					  (**) when not "corrected", this result appears as 9.9986792e-06
 *					 99999999 + .1        - 99999999 yields 26 - -4  or 30 (***)
 *					  (**) when not "corrected", this result appears as 9.9999994e-02
 *					 
 *					 Thus, acting on a difference of 49 or higher will catch the two
 *					 widely known problem cases described above.  However, starting with
 *					 a higher initial value (such as 2.0) will not get "corrected".
 *					 In addition, we start to lose accuracy (which admittedly, other
 *					 calculators don't seem to have) in the second set of examples.
 *					 
 *					 We could use a value as low as 30, which should eliminate
 *					 all potential problem cases, but will also sacrifice
 *					 our accuracy in digits "remembered" but not "displayed".
 *					 We have a default "accuracy" (kDefaultAccuracy), but we also
 *					 allow the programmer to override the default if desired.
 *					 
 *					 Incidentally, this is *only* an issue for addition and subtraction.
 *
 * PARAMETERS:  Two operands for the calculation, and the desired accuracy
 *
 * RETURNED:    The result of the calculation
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			steve	11/26/96	initial version
 *
 ***********************************************************************/
#define kDefaultAccuracy 48			// default accuracy if not specified

FlpDouble FlpCorrectedAdd (FlpDouble firstOperand, FlpDouble secondOperand,
												Int16 howAccurate)
{
	FlpDouble	result;
	Int16			minExp, tempExp;		// to keep track of the smallest exponent

	// get the signed (base two) exponents for each operand
	tempExp = FlpIsZero(firstOperand) ? 0 : FlpGetExponent(firstOperand);
	minExp = FlpIsZero(secondOperand) ? 0 : FlpGetExponent(secondOperand);

	// set minExp to be the lesser of the two
	if (tempExp < minExp) {
		minExp = tempExp;
		}

	// perform the addition
	result = _d_add(firstOperand, secondOperand);

	// get the exponent of the result, if the result is non-zero
	if ( !FlpIsZero(result) ) {

		tempExp = FlpGetExponent(result);

		// see if the result is significantly smaller than the smallest
		if (howAccurate == 0) {
			howAccurate = kDefaultAccuracy;		// default accuracy
			}

		if (minExp - tempExp > howAccurate) {	// higher values should be
			(*(FlpCompDouble*)&result).d = 0.0;		// forced to zero
			}

		}

	return(result);
}


/***********************************************************************
 *
 * FUNCTION: 	 FlpCorrectedSub
 *
 * DESCRIPTION: Same as FlpCorrectedAdd, above, but for subtraction
 *
 ***********************************************************************/
FlpDouble FlpCorrectedSub (FlpDouble firstOperand, FlpDouble secondOperand,
												Int16 howAccurate)
{
	FlpDouble	result;
	Int16			minExp, tempExp;		// to keep track of the smallest exponent

	// get the signed (base two) exponents for each operand
	tempExp = FlpIsZero(firstOperand) ? 0 : FlpGetExponent(firstOperand);
	minExp = FlpIsZero(secondOperand) ? 0 : FlpGetExponent(secondOperand);

	// set minExp to be the lesser of the two
	if (tempExp < minExp) {
		minExp = tempExp;
		}

	// perform the subtraction
	result = _d_sub(firstOperand, secondOperand);

	// get the exponent of the result, if the result is non-zero
	if ( !FlpIsZero(result) ) {
		tempExp = FlpGetExponent(result);

		// see if the result is significantly smaller than the smallest
		if (howAccurate == 0) {
			howAccurate = 48;							// default accuracy
			}

		if (minExp - tempExp > howAccurate) {	// higher values should be
			(*(FlpCompDouble*)&result).d = 0.0;		// forced to zero
			}

		}

	return(result);
}


/***********************************************************************
 *
 * FUNCTION: 	 FlpVersion
 *
 * DESCRIPTION: Returns version of NewFloatMgr
 *
 ***********************************************************************/
UInt32 FlpVersion (void)
{
	return(flpVersion);
}


/***********************************************************************
 *
 * FUNCTION: 	 FlpSelectorErrPrv
 *
 * DESCRIPTION: Displays error if bad selector is passed to NewFloatDispatch
 *					 Should this use ErrDisplayFileLineMsg instead?
 *					 It's a pretty fatal error...
 *
 ***********************************************************************/
void FlpSelectorErrPrv (UInt16 flpSelector)
{
	char	text[256];
	char	str[10];
	
	// Form the error message
	StrCopy(text, "Unsupported NewFloatMgr call: ");
	StrIToA(str, flpSelector);
	StrCat(text, str);
	ErrDisplay(text);
}




static const double singles[] = { 1e00, 1e01, 1e02, 1e03, 1e04, 1e05, 1e06,
			1e07, 1e08, 1e09, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15 };
static const double twoPowers[] = { /* 1e00 .. 1e08 */ 1e16, 1e32, 1e64, 1e128,
			1e256 };
static const double singlesInverse[] = { 1e-00, 1e-01, 1e-02, 1e-03, 1e-04, 1e-05, 1e-06,
			1e-07, 1e-08, 1e-09, 1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15 };
static const double twoPowersInverse[] = { /* 1e-00 .. 1e-08 */ 1e-16, 1e-32, 1e-64, 1e-128,
			1e-256 };
static double PrvPow10(Int16 x)
{
	// Returns 10.0 ^ x.
	
	double d;
	Int16 i;
	
	if (x > 0)
	{

		if (x > 400) x = 400;

		d = singles[x & 15];
		x >>= 4;
		for (i=0; x != 0; i++)
		{
			if ((x&1) != 0) d *= twoPowers[i];
			x >>= 1;
		}
	}
	else if (x < 0)
	{

		x = -x;
		if (x > 400) x = 400;
		
		d = singlesInverse[x & 15];
		x >>= 4;
		for (i=0; x != 0; i++)
		{
			if ((x&1) != 0) d *= twoPowersInverse[i];
			x >>= 1;
		}
	}
	else d = 1.0;

	
	return d;
}

