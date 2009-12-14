/******************************************************************************
 *
 * Copyright (c) 1997 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: MidiFrqPrv.h
 *
 * Release: 
 *
 * Description:
 * 	            	This is a table that translates the 128 MIDI notes
 *								into frequencies the PalmPilot will understand.
 *
 * History:
 *		July 25, 1997	Created by Trevor R. Menagh
 *	Name	Date			Description
 *	----	----			-----------
 *
 *****************************************************************************/

// You should define this before MidiFrqPrv.h is
// included in your code.  The default defination is for
// creating an inline assembly lookup table.  If you need
// to create a normal array this macro is different.
//
// NOTE: freq1-freq8 are all DWords.

	/*
	DC.L freq1#L, freq2#L, freq3#L, freq4#L, freq5#L, freq6#L, freq7#L, freq8#L
	*/

// This is the macro if you are creating a normal array	

	/*
	freq1#L, freq2#L, freq3#L, freq4#L, freq5#L, freq6#L, freq7#L, freq8#L,
	*/
	
//#ifndef MIDIFrequencyTableNormalRow
//#define MIDIFrequencyTableNormalRow(freq1, freq2, freq3, freq4, freq5, freq6, freq7, freq8) \
//				DC.L freq1#L, freq2#L, freq3#L, freq4#L, freq5#L, freq6#L, freq7#L, freq8#L
//#endif

// When making an inline assembly lookup table this
// is defined the same as MIDIFrequencyTableNormalRow
// It has to be changed when creating a normal array.
// The default defination is for creating an inline
// assembly lookup table.
//
// NOTE: freq1-freq8 are all DWords.

	/*
	DC.L freq1#L, freq2#L, freq3#L, freq4#L, freq5#L, freq6#L, freq7#L, freq8#L
	*/	

// This is the macro if you are creating a normal array		

	/*
	freq1#L, freq2#L, freq3#L, freq4#L, freq5#L, freq6#L, freq7#L, freq8#L
	*/

	
//#ifndef MIDIFrequencyTableEndRow
//#define MIDIFrequencyTableEndRow(freq1, freq2, freq3, freq4, freq5, freq6, freq7, freq8) \
//				DC.L freq1#L, freq2#L, freq3#L, freq4#L, freq5#L, freq6#L, freq7#L, freq8#L
//#endif
	
// The Hz frequencies should be stored as DWords.  To get to the actual 
// frequency in hertz divid the stored amouts by 10.  These amouts are
//accurate to one decimal place.
//
// NOTE: the frequencies are in their MIDI key# order.

MIDIFrequencyTableNormalRow (   8,     9,     9,    10,    10,    11,    12,    12)
MIDIFrequencyTableNormalRow (  13,    14,    15,    15,    16,    17,    18,    19)
MIDIFrequencyTableNormalRow (  21,    22,    23,    24,    26,    28,    29,    31)
MIDIFrequencyTableNormalRow (  33,    35,    37,    39,    41,    44,    46,    49)
MIDIFrequencyTableNormalRow (  52,    55,    58,    62,    65,    69,    73,    78)
MIDIFrequencyTableNormalRow (  82,    87,    92,    98,   104,   110,   117,   123)
MIDIFrequencyTableNormalRow ( 131,   139,   147,   156,   165,   175,   185,   196)
MIDIFrequencyTableNormalRow ( 208,   220,   233,   247,   262,   277,   294,   311)
MIDIFrequencyTableNormalRow ( 330,   349,   370,   392,   415,   440,   466,   494)
MIDIFrequencyTableNormalRow ( 523,   554,   587,   622,   659,   698,   740,   784)
MIDIFrequencyTableNormalRow ( 831,   880,   932,   988,  1047,  1109,  1175,  1245)
MIDIFrequencyTableNormalRow (1319,  1397,  1480,  1568,  1661,  1760,  1865,  1976)
MIDIFrequencyTableNormalRow (2093,  2218,  2349,  2489,  2637,  2794,  2960,  3136)
MIDIFrequencyTableNormalRow (3322,  3520,  3729,  3951,  4186,  4435,  4699,  4978)
MIDIFrequencyTableNormalRow (5274,  5588,  5920,  6272,  6645,  7040,  7459,  7902)
MIDIFrequencyTableEndRow 	(8372,  8870,  9397,  9956, 10548, 11175, 11840, 12544)

/*
Octave||                     Note Numbers
   #  ||
      || C   | C#  | D   | D#  | E   | F   | F#  | G   | G#  | A   | A#  | B
-----------------------------------------------------------------------------
   0  ||   0 |   1 |   2 |   3 |   4 |   5 |   6 |   7 |   8 |   9 |  10 | 11
   1  ||  12 |  13 |  14 |  15 |  16 |  17 |  18 |  19 |  20 |  21 |  22 | 23
   2  ||  24 |  25 |  26 |  27 |  28 |  29 |  30 |  31 |  32 |  33 |  34 | 35
   3  ||  36 |  37 |  38 |  39 |  40 |  41 |  42 |  43 |  44 |  45 |  46 | 47
   4  ||  48 |  49 |  50 |  51 |  52 |  53 |  54 |  55 |  56 |  57 |  58 | 59
   5  ||  60 |  61 |  62 |  63 |  64 |  65 |  66 |  67 |  68 |  69 |  70 | 71
   6  ||  72 |  73 |  74 |  75 |  76 |  77 |  78 |  79 |  80 |  81 |  82 | 83
   7  ||  84 |  85 |  86 |  87 |  88 |  89 |  90 |  91 |  92 |  93 |  94 | 95
   8  ||  96 |  97 |  98 |  99 | 100 | 101 | 102 | 103 | 104 | 105 | 106 | 107
   9  || 108 | 109 | 110 | 111 | 112 | 113 | 114 | 115 | 116 | 117 | 118 | 119
  10  || 120 | 121 | 122 | 123 | 124 | 125 | 126 | 127 |
*/
