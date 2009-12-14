/******************************************************************************
 *
 * Copyright (c) 1995-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: CharAttr.c
 *
 * Release: 
 *
 * Description:
 *		This file contains (obsolete) routines that return pointers to various
 *		tables, which in the past could be indexed into using byte character
 *		values. For multi-byte encodings that doesn't work, so these routines
 *		have all been deprecated.
 *
 * History:
 *		April 21, 1995	Created by Art Lamb
 *
 *****************************************************************************/

#include <PalmTypes.h>
#include <SystemMgr.h>		// sysFtrCreator
#include <FeatureMgr.h>		// FtrGet
#include <ErrorMgr.h>		// ErrNonFatalDisplayIf

#define NON_INTERNATIONAL
#include <CharAttr.h>

#define NON_PORTABLE
#include "Globals.h"			// GIntlMgrGlobalsP

#include "IntlPrv.h"			// IntlGlobalsType
#include "TextTablePrv.h"	// TTGet8BitIndexedData

#if (ERROR_CHECK_LEVEL == ERROR_CHECK_FULL)
#define	ECIntlStrict(msg)		PrvCheckIntlStrict(msg)
static void PrvCheckIntlStrict(const Char* iMsg);
#else
#define	ECIntlStrict(msg)
#endif

/***********************************************************************
 * Private PC-relative data tables used by routines in this file when
 * the IntlMgr global structure doesn't contain a byte-indexed attribute
 * table. In that case, it's assumed that the character encoding is _not_
 * Latin, and thus entries > 127 are all zero (unknown).
 ***********************************************************************/
static const UInt16 kCharAttrTable[256] =
{
	_BB,			// 00
	_BB,			// 01
	_BB,			// 02
	_BB,			// 03
	_BB,			// 04
	_BB,			// 05
	_BB,			// 06
	_BB,			// 07
	_BB,			// 08
	_CN,			// 09
	_CN,			// 0a
	_CN,			// 0b
	_CN,			// 0c
	_CN,			// 0d
	_BB,			// 0e
	_BB,			// 0f
	_BB,			// 10
	_BB,			// 11
	_BB,			// 12
	_BB,			// 13
	_PU,			// 14 chrOtaSecure
	_PU,			// 15 chrOta
	_PU,			// 16 chrCommandStroke
	_PU,			// 17 chrShortcutStroke
	_PU,			// 18	chrEllipsis
	_SP|_XS,		// 19 chrNumericSpace
	_BB,			// 1a
	_BB,			// 1b
	_BB,			// 1c
	_BB,			// 1d
	_BB,			// 1e
	_BB,			// 1f
	_SP,			// 20 " "
	_PU,			// 21 "!"
	_PU,			// 22 """
	_PU,			// 23 "#"
	_PU,			// 24 "$"
	_PU,			// 25 "%"
	_PU,			// 26 "&"
	_PU,			// 27 "'"
	_PU,			// 28 "("
	_PU,			// 29 ")"
	_PU,			// 2a "*"
	_PU,			// 2b "+"
	_PU,			// 2c ","
	_PU,			// 2d "-"
	_PU,			// 2e "."
	_PU,			// 2f "/"
	_DI|_XD,		// 30 "0"
	_DI|_XD,		// 31 "1"
	_DI|_XD,		// 32 "2"
	_DI|_XD,		// 33 "3"
	_DI|_XD,		// 34 "4"
	_DI|_XD,		// 35 "5"
	_DI|_XD,		// 36 "6"
	_DI|_XD,		// 37 "7"
	_DI|_XD,		// 38 "8"
	_DI|_XD,		// 39 "9"
	_PU,			// 3a ":"
	_PU,			// 3b ";"
	_PU,			// 3c "<"
	_PU,			// 3d "="
	_PU,			// 3e ">"
	_PU,			// 3f "?"
	_PU,			// 40 "@"
	_UP|_XD,		// 41 "A"
	_UP|_XD,		// 42 "B"
	_UP|_XD,		// 43 "C"
	_UP|_XD,		// 44 "D"
	_UP|_XD,		// 45 "E"
	_UP|_XD,		// 46 "F"
	_UP,			// 47 "G"
	_UP,			// 48 "H"
	_UP,			// 49 "I"
	_UP,			// 4a "J"
	_UP,			// 4b "K"
	_UP,			// 4c "L"
	_UP,			// 4d "M"
	_UP,			// 4e "N"
	_UP,			// 4f "O"
	_UP,			// 50 "P"
	_UP,			// 51 "Q"
	_UP,			// 52 "R"
	_UP,			// 53 "S"
	_UP,			// 54 "T"
	_UP,			// 55 "U"
	_UP,			// 56 "V"
	_UP,			// 57 "W"
	_UP,			// 58 "X"
	_UP,			// 59 "Y"
	_UP,			// 5a "Z"
	_PU,			// 5b "["
	_PU,			// 5c "\"
	_PU,			// 5d "]"
	_PU,			// 5e "^"
	_PU,			// 5f "_"
	_PU,			// 60 "`"
	_LO|_XD,		// 61 "a"
	_LO|_XD,		// 62 "b"
	_LO|_XD,		// 63 "c"
	_LO|_XD,		// 64 "d"
	_LO|_XD,		// 65 "e"
	_LO|_XD,		// 66 "f"
	_LO,			// 67 "g"
	_LO,			// 68 "h"
	_LO,			// 69 "i"
	_LO,			// 6a "j"
	_LO,			// 6b "k"
	_LO,			// 6c "l"
	_LO,			// 6d "m"
	_LO,			// 6e "n"
	_LO,			// 6f "o"
	_LO,			// 70 "p"
	_LO,			// 71 "q"
	_LO,			// 72 "r"
	_LO,			// 73 "s"
	_LO,			// 74 "t"
	_LO,			// 75 "u"
	_LO,			// 76 "v"
	_LO,			// 77 "w"
	_LO,			// 78 "x"
	_LO,			// 79 "y"
	_LO,			// 7a "z"
	_PU,			// 7b "{"
	_PU,			// 7c "|"
	_PU,			// 7d "}"
	_PU,			// 7e "~"
	_BB,			// 7f
	0,				// 80
	0,				// 81
	0,				// 82
	0,				// 83
	0,				// 84
	0,				// 85
	0,				// 86
	0,				// 87
	0,				// 88
	0,				// 89
	0,				// 8a
	0,				// 8b
	0,				// 8c
	0,				// 8d
	0,				// 8e
	0,				// 8f
	0,				// 90
	0,				// 91
	0,				// 92
	0,				// 93
	0,				// 94
	0,				// 95
	0,				// 96
	0,				// 97
	0,				// 98
	0,				// 99
	0,				// 9a
	0,				// 9b
	0,				// 9c
	0,				// 9d
	0,				// 9e
	0,				// 9f
	0,				// a0
	0,				// a1
	0,				// a2
	0,				// a3
	0,				// a4
	0,				// a5
	0,				// a6
	0,				// a7
	0,				// a8
	0,				// a9
	0,				// aa
	0,				// ab
	0,				// ac
	0,				// ad
	0,				// ae
	0,				// af
	0,				// b0
	0,				// b1
	0,				// b2
	0,				// b3
	0,				// b4
	0,				// b5
	0,				// b6
	0,				// b7
	0,				// b8
	0,				// b9
	0,				// ba
	0,				// bb
	0,				// bc
	0,				// bd
	0,				// be
	0,				// bf
	0,				// c0
	0,				// c1
	0,				// c2
	0,				// c3
	0,				// c4
	0,				// c5
	0,				// c6
	0,				// c7
	0,				// c8
	0,				// c9
	0,				// ca
	0,				// cb
	0,				// cc
	0,				// cd
	0,				// ce
	0,				// cf
	0,				// d0
	0,				// d1
	0,				// d2
	0,				// d3
	0,				// d4
	0,				// d5
	0,				// d6
	0,				// d7
	0,				// d8
	0,				// d9
	0,				// da
	0,				// db
	0,				// dc
	0,				// dd
	0,				// de
	0,				// df
	0,				// e0
	0,				// e1
	0,				// e2
	0,				// e3
	0,				// e4
	0,				// e5
	0,				// e6
	0,				// e7
	0,				// e8
	0,				// e9
	0,				// ea
	0,				// eb
	0,				// ec
	0,				// ed
	0,				// ee
	0,				// ef
	0,				// f0
	0,				// f1
	0,				// f2
	0,				// f3
	0,				// f4
	0,				// f5
	0,				// f6
	0,				// f7
	0,				// f8
	0,				// f9
	0,				// fa
	0,				// fb
	0,				// fc
	0,				// fd
	0,				// fe
	0				// ff		
};

static const UInt8 kSortValuesTable[256] =
{
	  0,   1,	// 00
					// 01
	  2,   3,	// 02
					// 03
	  4,   5,	// 04
					// 05
	  6,   7,	// 06
					// 07
	  8,  42,	// 08
					// 09
	 43,  44,	// 0a
					// 0b
	 45,  46,	// 0c
					// 0d
	  9,  10,	// 0e
					// 0f
	 11,  12,	// 10
					// 11
	 13,  14,	// 12
					// 13
	 15,  16,	// 14
					// 15
	 17,  18,	// 16
					// 17
	 19,  20,	// 18
					// 19
	 21,  22,	// 1a
					// 1b
	 23,  24,	// 1c
					// 1d
	 25,  26,	// 1e
					// 1f
	 40,  47,	// 20	" "
					// 21	"!"
	 48,  49,	// 22	"""
					// 23	"#"
	 50,  51,	// 24	"$"
					// 25	"%"
	 52,  35,	// 26	"&"
					// 27	"'"
	 53,  54,	// 28	"("
					// 29	")"
	 55,  90,	// 2a	"*"
					// 2b	"+"
	 56,  36,	// 2c	","
					// 2d	"-"
	 57,  58,	// 2e	"."
					// 2f	"/"
	117, 121,	// 30	"0"
					// 31	"1"
	123, 125,	// 32	"2"
					// 33	"3"
	127, 128,	// 34	"4"
					// 35	"5"
	129, 130,	// 36	"6"
					// 37	"7"
	131, 132,	// 38	"8"
					// 39	"9"
	 59,  60,	// 3a	":"
					// 3b	";"
	 91,  92,	// 3c	"<"
					// 3d	"="
	 93,  61,	// 3e	">"
					// 3f	"?"
	 62, 134,	// 40	"@"
					// 41	"A"
	151, 153,	// 42	"B"
					// 43	"C"
	157, 161,	// 44	"D"
					// 45	"E"
	171, 174,	// 46	"F"
					// 47	"G"
	176, 178,	// 48	"H"
					// 49	"I"
	188, 190,	// 4a	"J"
					// 4b	"K"
	192, 194,	// 4c	"L"
					// 4d	"M"
	196, 200,	// 4e	"N"
					// 4f	"O"
	217, 219,	// 50	"P"
					// 51	"Q"
	221, 223,	// 52	"R"
					// 53	"S"
	228, 233,	// 54	"T"
					// 55	"U"
	243, 245,	// 56	"V"
					// 57	"W"
	247, 249,	// 58	"X"
					// 59	"Y"
	255,  63,	// 5a	"Z"
					// 5b	"["
	 64,  65,	// 5c	"\"
					// 5d	"]"
	 66,  68,	// 5e	"^"
					// 5f	"_"
	 69, 133,	// 60	"`"
					// 61	"a"
	150, 152,	// 62	"b"
					// 63	"c"
	156, 160,	// 64	"d"
					// 65	"e"
	170, 173,	// 66	"f"
					// 67	"g"
	175, 177,	// 68	"h"
					// 69	"i"
	187, 189,	// 6a	"j"
					// 6b	"k"
	191, 193,	// 6c	"l"
					// 6d	"m"
	195, 199,	// 6e	"n"
					// 6f	"o"
	216, 218,	// 70	"p"
					// 71	"q"
	220, 222,	// 72	"r"
					// 73	"s"
	227, 232,	// 74	"t"
					// 75	"u"
	242, 244,	// 76	"v"
					// 77	"w"
	246, 248,	// 78	"x"
					// 79	"y"
	254,  70,	// 7a	"z"
					// 7b	"{"
	 71,  72,	// 7c	"|"
					// 7d	"}"
	 73,  27,	// 7e	"~"
					// 7f
	  0,   0,	// 80
					// 81	
	  0,   0,	// 82
					// 83
	  0,   0,	// 84
					// 85
	  0,   0,	// 86
					// 87
	  0,   0,	// 88
					// 89
	  0,   0,	// 8a
					// 8b
	  0,   0,	// 8c
					// 8d
	  0,   0,	// 8e
					// 8f
	  0,   0,	// 90
					// 91
	  0,   0,	// 92
					// 93
	  0,   0,	// 94
					// 95
	  0,   0,	// 96
					// 97
	  0,   0,	// 98
					// 99
	  0,   0,	// 9a
					// 9b
	  0,   0,	// 9c
					// 9d
	  0,   0,	// 9e
					// 9f
	  0,   0,	// a0
					// a1
	  0,   0,	// a2
					// a3
	  0,   0,	// a4
					// a5
	  0,   0,	// a6
					// a7
	  0,   0,	// a8
					// a9
	  0,   0,	// aa
					// ab
	  0,   0,	// ac
					// ad
	  0,   0,	// ae
					// af
	  0,   0,	// b0
					// b1
	  0,   0,	// b2
					// b3
	  0,   0,	// b4
					// b5
	  0,   0,	// b6
					// b7
	  0,   0,	// b8
					// b9
	  0,   0,	// ba
					// bb
	  0,   0,	// bc
					// bd
	  0,   0,	// be
					// bf
	  0,   0,	// c0
					// c1
	  0,   0,	// c2
					// c3
	  0,   0,	// c4
					// c5
	  0,   0,	// c6
					// c7
	  0,   0,	// c8
					// c9
	  0,   0,	// ca
					// cb
	  0,   0,	// cc
					// cd
	  0,   0,	// ce
					// cf
	  0,   0,	// d0
					// d1
	  0,   0,	// d2
					// d3
	  0,   0,	// d4
					// d5
	  0,   0,	// d6
					// d7
	  0,   0,	// d8
					// d9
	  0,   0,	// da
					// db
	  0,   0,	// dc
					// dd
	  0,   0,	// de
					// df
	  0,   0,	// e0
					// e1
	  0,   0,	// e2
					// e3
	  0,   0,	// e4
					// e5
	  0,   0,	// e6
					// e7
	  0,   0,	// e8
					// e9
	  0,   0,	// ea
					// eb
	  0,   0,	// ec
					// ed
	  0,   0,	// ee
					// ef
	  0,   0,	// f0
					// f1
	  0,   0,	// f2
					// f3
	  0,   0,	// f4
					// f5
	  0,   0,	// f6
					// f7
	  0,   0,	// f8
					// f9
	  0,   0,	// fa
					// fb
	  0,   0,	// fc
					// fd
	  0,   0 	// fe
					// ff
};

static const UInt8 kCaselessValuesTable[256] =
{
	  0,   1,	// 00
					// 01
	  2,   3,	// 02
					// 03
	  4,   5,	// 04
					// 05
	  6,   7,	// 06
					// 07
	  8,  42,	// 08
					// 09
	 43,  44,	// 0a
					// 0b
	 45,  46,	// 0c
					// 0d
	  9,  10,	// 0e
					// 0f
	 11,  12,	// 10
					// 11
	 13,  14,	// 12
					// 13
	 15,  16,	// 14
					// 15
	 17,  18,	// 16
					// 17
	 19,  20,	// 18
					// 19
	 21,  22,	// 1a
					// 1b
	 23,  24,	// 1c
					// 1d
	 25,  26,	// 1e
					// 1f
	 40,  47,	// 20	" "
					// 21	"!"
	 48,  49,	// 22	"""
					// 23	"#"
	 50,  51,	// 24	"$"
					// 25	"%"
	 52,  35,	// 26	"&"
					// 27	"'"
	 53,  54,	// 28	"("
					// 29	")"
	 55,  90,	// 2a	"*"
					// 2b	"+"
	 56,  36,	// 2c	","
					// 2d	"-"
	 57,  58,	// 2e	"."
					// 2f	"/"
	117, 121,	// 30	"0"
					// 31	"1"
	123, 125,	// 32	"2"
					// 33	"3"
	127, 128,	// 34	"4"
					// 35	"5"
	129, 130,	// 36	"6"
					// 37	"7"
	131, 132,	// 38	"8"
					// 39	"9"
	 59,  60,	// 3a	":"
					// 3b	";"
	 91,  92,	// 3c	"<"
					// 3d	"="
	 93,  61,	// 3e	">"
					// 3f	"?"
	 62, 133,	// 40	"@"
					// 41	"A"
	150, 152,	// 42	"B"
					// 43	"C"
	156, 160,	// 44	"D"
					// 45	"E"
	170, 173,	// 46	"F"
					// 47	"G"
	175, 177,	// 48	"H"
					// 49	"I"
	187, 189,	// 4a	"J"
					// 4b	"K"
	191, 193,	// 4c	"L"
					// 4d	"M"
	195, 199,	// 4e	"N"
					// 4f	"O"
	216, 218,	// 50	"P"
					// 51	"Q"
	220, 222,	// 52	"R"
					// 53	"S"
	227, 232,	// 54	"T"
					// 55	"U"
	242, 244,	// 56	"V"
					// 57	"W"
	246, 248,	// 58	"X"
					// 59	"Y"
	254,  63,	// 5a	"Z"
					// 5b	"["
	 64,  65,	// 5c	"\"
					// 5d	"]"
	 66,  68,	// 5e	"^"
					// 5f	"_"
	 69, 133,	// 60	"`"
					// 61	"a"
	150, 152,	// 62	"b"
					// 63	"c"
	156, 160,	// 64	"d"
					// 65	"e"
	170, 173,	// 66	"f"
					// 67	"g"
	175, 177,	// 68	"h"
					// 69	"i"
	187, 189,	// 6a	"j"
					// 6b	"k"
	191, 193,	// 6c	"l"
					// 6d	"m"
	195, 199,	// 6e	"n"
					// 6f	"o"
	216, 218,	// 70	"p"
					// 71	"q"
	220, 222,	// 72	"r"
					// 73	"s"
	227, 232,	// 74	"t"
					// 75	"u"
	242, 244,	// 76	"v"
					// 77	"w"
	246, 248,	// 78	"x"
					// 79	"y"
	254,  70,	// 7a	"z"
					// 7b	"{"
	 71,  72,	// 7c	"|"
					// 7d	"}"
	 73,  27,	// 7e	"~"
					// 7f
	  0,	 0,	// 80
					// 81	
	  0,	 0,	// 82
					// 83
	  0,	 0,	// 84
					// 85
	  0,	 0,	// 86
					// 87
	  0,	 0,	// 88
					// 89
	  0,	 0,	// 8a
					// 8b
	  0,	 0,	// 8c
					// 8d
	  0,	 0,	// 8e
					// 8f
	  0,	 0,	// 90
					// 91
	  0,	 0,	// 92
					// 93
	  0,	 0,	// 94
					// 95
	  0,	 0,	// 96
					// 97
	  0,	 0,	// 98
					// 99
	  0,	 0,	// 9a
					// 9b
	  0,	 0,	// 9c
					// 9d
	  0,	 0,	// 9e
					// 9f
	  0,	 0,	// a0
					// a1
	  0,	 0,	// a2
					// a3
	  0,	 0,	// a4
					// a5
	  0,	 0,	// a6
					// a7
	  0,	 0,	// a8
					// a9
	  0,	 0,	// aa
					// ab
	  0,	 0,	// ac
					// ad
	  0,	 0,	// ae
					// af
	  0,	 0,	// b0
					// b1
	  0,	 0,	// b2
					// b3
	  0,	 0,	// b4
					// b5
	  0,	 0,	// b6
					// b7
	  0,	 0,	// b8
					// b9
	  0,	 0,	// ba
					// bb
	  0,	 0,	// bc
					// bd
	  0,	 0,	// be
					// bf
	  0,	 0,	// c0
					// c1
	  0,	 0,	// c2
					// c3
	  0,	 0,	// c4
					// c5
	  0,	 0,	// c6
					// c7
	  0,	 0,	// c8
					// c9
	  0,	 0,	// ca
					// cb
	  0,	 0,	// cc
					// cd
	  0,	 0,	// ce
					// cf
	  0,	 0,	// d0
					// d1
	  0,	 0,	// d2
					// d3
	  0,	 0,	// d4
					// d5
	  0,	 0,	// d6
					// d7
	  0,	 0,	// d8
					// d9
	  0,	 0,	// da
					// db
	  0,	 0,	// dc
					// dd
	  0,	 0,	// de
					// df
	  0,	 0,	// e0
					// e1
	  0,	 0,	// e2
					// e3
	  0,	 0,	// e4
					// e5
	  0,	 0,	// e6
					// e7
	  0,	 0,	// e8
					// e9
	  0,	 0,	// ea
					// eb
	  0,	 0,	// ec
					// ed
	  0,	 0,	// ee
					// ef
	  0,	 0,	// f0
					// f1
	  0,	 0,	// f2
					// f3
	  0,	 0,	// f4
					// f5
	  0,	 0,	// f6
					// f7
	  0,	 0,	// f8
					// f9
	  0,	 0,	// fa
					// fb
	  0,	 0,	// fc
					// fd
	  0,	 0,	// fe
					// ff
};

/***********************************************************************
 *
 * FUNCTION: 	 GetCharAttr
 *
 * DESCRIPTION: Returns a a pointer to the characters attributes array
 *		which is used by the character classification and character
 *		conversion macros. This routine has been deprecated, since
 *		it assumes you can index into the table using a byte value,
 *		which isn't the case for char encodings such as Shift-JIS. The
 *		new TxtCharAttr routine and TxtCharIsXXX macros should be used
 *		instead.
 *
 * PARAMETERS:	nothing
 *
 * RETURNED:	A pointer to the attributes array.
 *
 * HISTORY:
 *		04/21/95	art	Created by Art Lamb.
 *		07/18/95	david	Added extended char. attributes
 *		05/26/98	kwk	Created externally visible CharAttrTable label.
 *		07/24/98	SCL	Relocated/obsoleted PalmOS-specific characters.
 *		05/18/00	kwk	Try to use IntlMgr table data, otherwise default
 *							to PC-relative data. Also trigger fatal alert on
 *							debug ROMs when in strict Intl Mgr mode.
 *		05/31/00	CS		TTGetNumResultBits is now a separate routine.
 *					kwk	Verify TTGet8BitIndexedData returns NULL, versus
 *							relying on the elementSize result.
 *
 ***********************************************************************/
 const UInt16* GetCharAttr(void)
{
	void* tableP = GIntlData->charAttrTable;
	
	// On a debug ROM with the strict Int'l Mgr flag set, trigger a
	// fatal alert.
	ECIntlStrict("GetCharAttr is obsolete - use TxtCharAttr");

	// If we've got a byte-indexed table of 16-bit values, return a ptr
	// to the first element in the array.
	if (tableP != NULL)
	{
		void* attrTable = TTGet8BitIndexedData(tableP);
		if ((attrTable != NULL) && (TTGetNumResultBits(tableP) == 16))
		{
			return(attrTable);
		}
	}
	
	return(kCharAttrTable);
} // GetCharAttr


/***********************************************************************
 *
 * FUNCTION: 	 GetCharSortValue
 *
 * DESCRIPTION: Returns a pointer to an array that maps all characters
 *		to an assigned sorting value. This routine has been deprecated,
 *		since it assumes that you can index into the table using a byte
 *		value, which isn't the case for char encodings such as Shift-JIS.
 *		The TxtCompare (or StrCompare) routine should be used instead.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:	A pointer to the sorting values array.
 *
 *	HISTORY:
 *		07/11/95	David	Initial Revision
 *		08/05/98	rsf	Fixed rightGuillemetChr to follow leftGuillemetChr
 *		04/14/99	grant	Fixed masculineOrdinalChr, lowIDiaeresisChr
 *							Moved chrEuroSign (0x80) (and shifted others around to accomodate)
 *		05/20/00	kwk	Try to use IntlMgr table data, otherwise default
 *							to PC-relative data. Also trigger fatal alert on
 *							debug ROMs when in strict Intl Mgr mode.
 *		05/31/00	CS		TTGetNumResultBits is now a separate routine.
 *					kwk	Catch case of sort table already pointing to array
 *							(if fast sort flag is set), and double-check that
 *							TTGet8BitIndexedData returns non-NULL.
 *
 ***********************************************************************/
 const UInt8* GetCharSortValue(void)
{
	void* tableP = GIntlData->sortTables[2];
	
	// On a debug ROM with the strict Int'l Mgr flag set, trigger a
	// fatal alert.
	ECIntlStrict("GetCharSortValue is obsolete - use TxtCompare");

	// If we've got a byte-indexed table of 8-bit sort values, return a ptr
	// to the first element in the array.
	if (tableP != NULL)
	{
		void* sortTable;
		
		// If the fast sorting flag is set, then the sort table is
		// already a ptr to the array of byte values.
		if ((GIntlData->intlFlags & kByteSortingFlag) != 0)
		{
			return(tableP);
		}
		
		sortTable = TTGet8BitIndexedData(tableP);
		if ((sortTable != NULL) && (TTGetNumResultBits(tableP) == 8))
		{
			return(sortTable);
		}
	}
	
	return(kSortValuesTable);
} // GetCharSortValue


/***********************************************************************
 *
 * FUNCTION: 	 GetCharCaselessValue
 *
 * DESCRIPTION: Returns a pointer to an array that maps all characters to
 *		an assigned caseless and accentless value. This routine has been deprecated,
 *		since it assumes that you can index into the table using a byte
 *		value, which isn't the case for char encodings such as Shift-JIS.
 *		The TxtCompare (or StrCompare) routine should be used instead.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:	A pointer to the caseless sorting values array.
 *
 * HISTORY:
 *		09/26/95	rsf	Created by Roger Flores.
 *		08/05/98	rsf	Fixed rightGuillemetChr to follow leftGuillemetChr
 *		04/14/99	grant	Fixed masculineOrdinalChr, lowIDiaeresisChr
 *							Moved chrEuroSign (0x80) (and shifted others around to accomodate)
 *		05/20/00	kwk	Try to use IntlMgr table data, otherwise default
 *							to PC-relative data. Also trigger fatal alert on
 *							debug ROMs when in strict Intl Mgr mode.
 *		05/31/00	CS		TTGetNumResultBits is now a separate routine.
 *					kwk	Catch case of sort table already pointing to array
 *							(if fast sort flag is set), and double-check that
 *							TTGet8BitIndexedData returns non-NULL.
 *
 ***********************************************************************/
 const UInt8* GetCharCaselessValue(void)
{
	void* tableP = GIntlData->sortTables[0];
	
	// On a debug ROM with the strict Int'l Mgr flag set, trigger a
	// fatal alert.
	ECIntlStrict("GetCharCaselessValue is obsolete - use TxtCaselessCompare");

	// If we've got a byte-indexed table of 8-bit sort values, return a ptr
	// to the first element in the array.
	if (tableP != NULL)
	{
		void* sortTable;

		// If the fast sorting flag is set, then the sort table is
		// already a ptr to the array of byte values.
		if ((GIntlData->intlFlags & kByteSortingFlag) != 0)
		{
			return(tableP);
		}
		
		sortTable = TTGet8BitIndexedData(tableP);
		if ((sortTable != NULL) && (TTGetNumResultBits(tableP) == 8))
		{
			return(sortTable);
		}
	}
	
	return(kCaselessValuesTable);
} // GetCharCaselessValue


/***********************************************************************
 *
 * FUNCTION: 	 PrvCheckIntlStrict
 *
 * DESCRIPTION: If strict error checking is on, display <iMsg>.
 *
 * PARAMETERS: 
 *		iMsg	 ->	Message to be displayed.
 *
 * RETURNED:	nothing
 *
 * HISTORY:
 *		05/20/00	kwk	Created by Ken Krugler.
 *		11/29/00	kwk	For speed, use GIntlData->intlFlags vs. feature.
 *
 ***********************************************************************/
#if (ERROR_CHECK_LEVEL == ERROR_CHECK_FULL)
static void PrvCheckIntlStrict(const Char* iMsg)
{
	if ((GIntlData->intlFlags & kStrictChecksFlag) != 0)
	{
		ErrNonFatalDisplay(iMsg);
	}
} // PrvCheckIntlStrict
#endif
