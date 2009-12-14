/******************************************************************************
 *
 * Copyright (c) 1999-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: OverlayMgr.c
 *
 * Release: 
 *
 * Description:
 *		Routines that support overlays & locales. Some information about
 *		the meanings of the three different locales follows:
 *
 *		Default locale - this is based on information stored in the ROM
 *			store NVParams record, and is only used when the card 0 RAM
 *			store doesn't exist or isn't valid. We stash this in a 'default locale'
 *			feature, which gets used when we can't find a valid overlay for
 *			a stripped base using the current locale.
 *		System locale - this is based on information stored in the card
 *			0 RAM store NVParams record. This information is used to determine
 *			the correct system overlay to use at device reset time.
 *		Current locale - this is based on the country & language features.
 *			At reset time it's initialized to be the same as the system locale.
 *			If somebody changes the system locale, then current might not be
 *			the same as system. Also during testing sometimes we'll change
 *			current to be different than system.
 *
 * History:
 *		06/24/99	kwk	Created by Ken Krugler
 *		07/12/99	kwk	Remove dmModeExclusive from default mode, so that a base
 *							DB can be opened multiple times (e.g. for AbtShowAbout).
 *		08/27/99	kwk	Display error msg w/DB name, but only if the "show errors"
 *							flag is set for the Overlay Mgr feature.
 *		09/20/99 gap	Added additional country codes to maintain sync with
 *							cXXXX country values defined in Preferences.h.
 *		09/20/99	gap	cPRC -> cRepChina.
 *		04/30/00	CS		Rolled all language and country codes from ISO 639/3166
 *							into LanguageCode and CountryCode arrays.
 *		05/16/00	CS		LmCountryType/LmLanguageType are now back to
 *							CountryType/LanguageType.
 *		05/29/00	kwk	Moved PC-relative data after OmDispatch routine, so that
 *							boot=10001 will link w/o 16-bit reference errors.
 *		05/31/00	CS		Moved country and language codes to new PalmLocale.h and
 *							removed kLanguageFirst, etc.
 *
 *****************************************************************************/

#define	NON_PORTABLE

#include <PalmTypes.h>

#include <DataMgr.h>
#include <SystemMgr.h>
#include <MemoryMgr.h>
#include <StringMgr.h>
#include <ErrorMgr.h>
#include <Preferences.h>
#include <TextMgr.h>
#include <Crc.h>
#include <FeatureMgr.h>
#include <DebugMgr.h>
#include	<UIResources.h>
#include <SysUtils.h>		// SysBinarySearch
#include <PalmLocale.h>		// lEnglish & cUnitedStates

// Generate jumps to the routines from the dispatch table. DOLATER kwk - 
// figure out if this causes problems w/routines inside of here calling
// other Overlay Mgr routines directly, versus using the trap. To fix this
// we'd need to have an OverlayDispatch.c file which just contained the
// dispatcher code.

#define	DIRECT_OVERLAY_CALLS
#include <OverlayMgr.h>

#include <PalmUtils.h>

#include "MemoryPrv.h"			// MemNVParams
#include	"DataPrv.h"				// DmAccessType, DmOpenInfoType, DatabaseHdrType
#include "OverlayPrv.h"			// OmDispatch, OmInit, OmOpenOverlayDatabase
#include "UIResourcesPrv.h"	// For localeModuleNameStrID

/***********************************************************************
 * Private types
 **********************************************************************/

typedef Char CountryCode[2];
typedef Char LanguageCode[2];

/***********************************************************************
 * Private constants
 **********************************************************************/

const Int32 localeStrLen = 5;		// '_' + language (2 bytes) + country (2 bytes)
const Int32 maxBaseDBName = dmDBNameLength - sizeOf7BitChar('\0') - localeStrLen;

const UInt16 maxUInt16 = 0xFFFF;	// DOLATER kwk - move to PalmTypes.h

// Mode flags we want to copy from the base when opening the overlay.
const UInt16 overlayBaseMode = dmModeLeaveOpen | dmModeExclusive;
const UInt16 overlayDefaultMode = dmModeReadOnly;

/***********************************************************************
 * Private routines
 ***********************************************************************/

#if (ERROR_CHECK_LEVEL == ERROR_CHECK_FULL)
#define	ECDisplayError(dbRef, msg)		DisplayError((dbRef), (msg))
#define	ErrDbgBreakIf(condition)		do {if (condition) DbgBreak();} while (0)
#else
#define	ECDisplayError(dbRef, msg)
#define	ErrDbgBreakIf(condition)
#endif

#if (EMULATION_LEVEL == EMULATION_NONE)
static void OmDispatchTable(void);
#endif

static Err
FindOverlayDatabaseByLocale(	DmOpenRef baseRef,
										const LmLocaleType* overlayLocale,
										LocalID* overlayID);

static Boolean
IsBaseStripped(DmOpenRef inBase);

static Err
LocaleToStr(const LmLocaleType* targetLocale, Char* localeStr);

static void
OmSelectorError(OmSelector inSelector);

static Err
PrvLoadScreens(UInt16 cardNo, const LmLocaleType* systemLocale, MemHandle* splashImage, MemHandle* resetImage);

static void
PrvStrNCopy(Char* dstP, const Char* srcP, Int16 n);

static Int16
SearchOverlaySpec(void const *searchData, void const *arrayData, Int32 other);

static Err
StrToLocale(const Char* localeStr, LmLocaleType* targetLocale);

static Boolean
VerifySystemOverlay(UInt16 inCard, const LmLocaleType* sysLocale, DmOpenRef inBase, LocalID inOverlay);

static Boolean
VerifyOverlayDatabase(DmOpenRef baseRef, DmOpenRef overlayRef);

#if (EMULATION_LEVEL == EMULATION_NONE)

/***********************************************************************
 * OmDispatch
 *
 * Dispatch to the appropriate Overlay Mgr routine. The selector is
 * passed in register d2.w; there are a variable number of parameters
 * on the stack, which we ignore.
 ***********************************************************************/

asm void
OmDispatch(void)
{
	cmp.w		#omMaxSelector,d2
	bhi		@badSelector
	add.w		d2,d2
	add.w		d2,d2						// Convert to long index.
	opword	0x4efb,0x2002			// jmp 4(pc,d2.w)

	// The order of routines in this table MUST match the enumerated set
	// of routine selectors in the OverlayMgr.h file.

	entry extern OmDispatchTable

	// New in 3.5
	jmp		OmInit								// 0
	jmp		OmOpenOverlayDatabase			// 1
	jmp		OmLocaleToOverlayDBName			// 2
	jmp		OmOverlayDBNameToLocale			// 3
	jmp		OmGetCurrentLocale				// 4
	jmp		OmGetIndexedLocale				// 5
	jmp		OmGetSystemLocale					// 6
	jmp		OmSetSystemLocale					// 7
	jmp		OmGetRoutineAddress				// 8
	
	// New in 4.0
	jmp		OmGetNextSystemLocale			// 9
	
@badSelector:
	move.w	d2,-(sp)					// push the bad selector
	jmp		OmSelectorError		// and display it.
} // OmDispatch


/***********************************************************************
 * OmSelectorError
 *
 * Somebody tried to call one of the selector-based Overlay Mgr routines
 * with a bogus selector. Report a fatal error.
 ***********************************************************************/

static void
OmSelectorError(OmSelector inSelector)
{
	char text[64];
	char str[16];
	
	StrCopy(text, "Unsupported Overlay Manager call: ");
	StrIToA(str, inSelector);
	StrCat(text, str);
	ErrDisplay(text);
} // OmSelectorError

#endif // EMULATION_LEVEL == EMULATION_NONE

/***********************************************************************
 * Private data
 **********************************************************************/

// Language codes (ISO 639).  The first 8 preserve the old values for the
// deprecated LanguageType; the rest are sorted by the 2-character language code.
//
// WARNING! Keep in sync with BOTH:
//				1)	LanguageType #defines in PalmLocale.h
//				2)	localeLanguage #define in UIResDefs.r
//
#define kLanguageCount (sizeof(kLanguages) / sizeof(kLanguages[0]))
static const LanguageCode kLanguages[] = {
	'e', 'n',	// lEnglish
	'f', 'r',	// lFrench
	'd', 'e',	// lGerman
	'i', 't',	// lItalian
	'e', 's',	// lSpanish

	'e', 'n',	// lUnused LANGUAGE_WORKPAD (IBM WorkPad - English)

	'j', 'p',	// lJapanese (According to ISO 639, this should be JA)
	'n', 'l',	// lDutch

	'a', 'a',	// lAfar
	'a', 'b',	// lAbkhazian
	'a', 'f',	// lAfrikaans
	'a', 'm',	// lAmharic
	'a', 'r',	// lArabic
	'a', 's',	// lAssamese
	'a', 'y',	// lAymara
	'a', 'z',	// lAzerbaijani
	'b', 'a',	// lBashkir
	'b', 'e',	// lByelorussian
	'b', 'g',	// lBulgarian
	'b', 'h',	// lBihari
	'b', 'i',	// lBislama
	'b', 'n',	// lBengali
	'b', 'o',	// lTibetan
	'b', 'r',	// lBreton
	'c', 'a',	// lCatalan
	'c', 'o',	// lCorsican
	'c', 's',	// lCzech
	'c', 'y',	// lWelsh
	'd', 'a',	// lDanish
	'd', 'z',	// lBhutani
	'e', 'l',	// lGreek
	'e', 'o',	// lEsperanto
	'e', 't',	// lEstonian
	'e', 'u',	// lBasque
	'f', 'a',	// lFarsi
	'f', 'i',	// lFinnish
	'f', 'j',	// lFiji
	'f', 'o',	// lFaroese
	'f', 'y',	// lFrisian
	'g', 'a',	// lIrish
	'g', 'd',	// lScotsGaelic
	'g', 'l',	// lGalician
	'g', 'n',	// lGuarani
	'g', 'u',	// lGujarati
	'h', 'a',	// lHausa
	'h', 'i',	// lHindi
	'h', 'r',	// lCroatian
	'h', 'u',	// lHungarian
	'h', 'y',	// lArmenian
	'i', 'a',	// lInterlingua
	'i', 'e',	// lInterlingue
	'i', 'k',	// lInupiak
	'i', 'n',	// lIndonesian
	'i', 's',	// lIcelandic
	'i', 'w',	// lHebrew
	'j', 'i',	// lYiddish
	'j', 'w',	// lJavanese
	'k', 'a',	// lGeorgian
	'k', 'k',	// lKazakh
	'k', 'l',	// lGreenlandic
	'k', 'm',	// lCambodian
	'k', 'n',	// lKannada
	'k', 'o',	// lKorean
	'k', 's',	// lKashmiri
	'k', 'u',	// lKurdish
	'k', 'y',	// lKirghiz
	'l', 'a',	// lLatin
	'l', 'n',	// lLingala
	'l', 'o',	// lLaothian
	'l', 't',	// lLithuanian
	'l', 'v',	// lLatvian
	'm', 'g',	// lMalagasy
	'm', 'i',	// lMaori
	'm', 'k',	// lMacedonian
	'm', 'l',	// lMalayalam
	'm', 'n',	// lMongolian
	'm', 'o',	// lMoldavian
	'm', 'r',	// lMarathi
	'm', 's',	// lMalay
	'm', 't',	// lMaltese
	'm', 'y',	// lBurmese
	'n', 'a',	// lNauru
	'n', 'e',	// lNepali
	'n', 'o',	// lNorwegian
	'o', 'c',	// lOccitan
	'o', 'm',	// lAfan
	'o', 'r',	// lOriya
	'p', 'a',	// lPunjabi
	'p', 'l',	// lPolish
	'p', 's',	// lPashto
	'p', 't',	// lPortuguese
	'q', 'u',	// lQuechua
	'r', 'm',	// lRhaetoRomance
	'r', 'n',	// lKirundi
	'r', 'o',	// lRomanian
	'r', 'u',	// lRussian
	'r', 'w',	// lKinyarwanda
	's', 'a',	// lSanskrit
	's', 'd',	// lSindhi
	's', 'g',	// lSangro
	's', 'h',	// lSerboCroatian
	's', 'i',	// lSinghalese
	's', 'k',	// lSlovak
	's', 'l',	// lSlovenian
	's', 'm',	// lSamoan
	's', 'n',	// lShona
	's', 'o',	// lSomali
	's', 'q',	// lAlbanian
	's', 'r',	// lSerbian
	's', 's',	// lSiswati
	's', 't',	// lSesotho
	's', 'u',	// lSudanese
	's', 'v',	// lSwedish
	's', 'w',	// lSwahili
	't', 'a',	// lTamil
	't', 'e',	// lTegulu
	't', 'g',	// lTajik
	't', 'h',	// lThai
	't', 'i',	// lTigrinya
	't', 'k',	// lTurkmen
	't', 'l',	// lTagalog
	't', 'n',	// lSetswana
	't', 'o',	// lTonga
	't', 'r',	// lTurkish
	't', 's',	// lTsonga
	't', 't',	// lTatar
	't', 'w',	// lTwi
	'u', 'k',	// lUkrainian
	'u', 'r',	// lUrdu
	'u', 'z',	// lUzbek
	'v', 'i',	// lVietnamese
	'v', 'o',	// lVolapuk
	'w', 'o',	// lWolof
	'x', 'h',	// lXhosa
	'y', 'o',	// lYoruba
	'z', 'h',	// lChinese
	'z', 'u',	// lZulu
};

// Country codes (ISO 3166).  The first 33 preserve the old values for the
// deprecated CountryType; the rest are sorted by the 2-character country code.
//
// WARNING! Keep in sync with BOTH:
//				1)	CountryType #defines in PalmLocale.h
//				2)	localeCountry #define in UIResDefs.r
//
#define kCountryCount (sizeof(kCountries) / sizeof(kCountries[0]))
static const CountryCode kCountries[] = {
	'A', 'U',	// cAustralia
	'A', 'T',	// cAustria
	'B', 'E',	// cBelgium
	'B', 'R',	// cBrazil
	'C', 'A',	// cCanada
	'D', 'K',	// cDenmark
	'F', 'I',	// cFinland
	'F', 'R',	// cFrance
	'D', 'E',	// cGermany
	'H', 'K',	// cHongKong
	'I', 'S',	// cIceland
	'I', 'E',	// cIreland
	'I', 'T',	// cItaly
	'J', 'P',	// cJapan
	'L', 'U',	// cLuxembourg
	'M', 'X',	// cMexico
	'N', 'L',	// cNetherlands
	'N', 'Z',	// cNewZealand
	'N', 'O',	// cNorway
	'E', 'S',	// cSpain
	'S', 'E',	// cSweden
	'C', 'H',	// cSwitzerland
	'G', 'B',	// cUnitedKingdom (UK)
	'U', 'S',	// cUnitedStates
	'I', 'N',	// cIndia
	'I', 'D',	// cIndonesia
	'K', 'R',	// cRepublicOfKorea
	'M', 'Y',	// cMalaysia
	'C', 'N',	// cChina
	'P', 'H',	// cPhilippines
	'S', 'G',	// cSingapore
	'T', 'H',	// cThailand
	'T', 'W',	// cTaiwan

	'A', 'D',	// cAndorra
	'A', 'E',	// cUnitedArabEmirates
	'A', 'F',	// cAfghanistan
	'A', 'G',	// cAntiguaAndBarbuda
	'A', 'I',	// cAnguilla
	'A', 'L',	// cAlbania
	'A', 'M',	// cArmenia
	'A', 'N',	// cNetherlandsAntilles
	'A', 'O',	// cAngola
	'A', 'Q',	// cAntarctica
	'A', 'R',	// cArgentina
	'A', 'S',	// cAmericanSamoa
	'A', 'W',	// cAruba
	'A', 'Z',	// cAzerbaijan
	'B', 'A',	// cBosniaAndHerzegovina
	'B', 'B',	// cBarbados
	'B', 'D',	// cBangladesh
	'B', 'F',	// cBurkinaFaso
	'B', 'G',	// cBulgaria
	'B', 'H',	// cBahrain
	'B', 'I',	// cBurundi
	'B', 'J',	// cBenin
	'B', 'M',	// cBermuda
	'B', 'N',	// cBruneiDarussalam
	'B', 'O',	// cBolivia
	'B', 'S',	// cBahamas
	'B', 'T',	// cBhutan
	'B', 'V',	// cBouvetIsland
	'B', 'W',	// cBotswana
	'B', 'Y',	// cBelarus
	'B', 'Z',	// cBelize
	'C', 'C',	// cCocosIslands
	'C', 'D',	// cDemocraticRepublicOfTheCongo
	'C', 'F',	// cCentralAfricanRepublic
	'C', 'G',	// cCongo
	'C', 'I',	// cIvoryCoast
	'C', 'K',	// cCookIslands
	'C', 'L',	// cChile
	'C', 'M',	// cCameroon
	'C', 'O',	// cColumbia
	'C', 'R',	// cCostaRica
	'C', 'U',	// cCuba
	'C', 'V',	// cCapeVerde
	'C', 'X',	// cChristmasIsland
	'C', 'Y',	// cCyprus
	'C', 'Z',	// cCzechRepublic
	'D', 'J',	// cDjibouti
	'D', 'M',	// cDominica
	'D', 'O',	// cDominicanRepublic
	'D', 'Z',	// cAlgeria
	'E', 'C',	// cEcuador
	'E', 'E',	// cEstonia
	'E', 'G',	// cEgypt
	'E', 'H',	// cWesternSahara
	'E', 'R',	// cEritrea
	'E', 'T',	// cEthiopia
	'F', 'J',	// cFiji
	'F', 'K',	// cFalklandIslands
	'F', 'M',	// cMicronesia
	'F', 'O',	// cFaeroeIslands
	'F', 'X',	// cMetropolitanFrance
	'G', 'A',	// cGabon
	'G', 'D',	// cGrenada
	'G', 'E',	// cGeorgia
	'G', 'F',	// cFrenchGuiana
	'G', 'H',	// cGhana
	'G', 'I',	// cGibraltar
	'G', 'L',	// cGreenland
	'G', 'M',	// cGambia
	'G', 'N',	// cGuinea
	'G', 'P',	// cGuadeloupe
	'G', 'Q',	// cEquatorialGuinea
	'G', 'R',	// cGreece
	'G', 'S',	// cSouthGeorgiaAndTheSouthSandwichIslands
	'G', 'T',	// cGuatemala
	'G', 'U',	// cGuam
	'G', 'W',	// cGuineaBisseu
	'G', 'Y',	// cGuyana
	'H', 'M',	// cHeardAndMcDonaldIslands
	'H', 'N',	// cHonduras
	'H', 'R',	// cCroatia
	'H', 'T',	// cHaiti
	'H', 'U',	// cHungary
	'I', 'L',	// cIsrael
	'I', 'O',	// cBritishIndianOceanTerritory
	'I', 'Q',	// cIraq
	'I', 'R',	// cIran
	'J', 'M',	// cJamaica
	'J', 'O',	// cJordan
	'K', 'E',	// cKenya
	'K', 'G',	// cKyrgyzstan (Kirgistan)
	'K', 'H',	// cCambodia
	'K', 'I',	// cKiribati
	'K', 'M',	// cComoros
	'K', 'N',	// cStKittsAndNevis
	'K', 'P',	// cDemocraticPeoplesRepublicOfKorea
	'K', 'W',	// cKuwait
	'K', 'Y',	// cCaymanIslands
	'K', 'K',	// cKazakhstan
	'L', 'A',	// cLaos
	'L', 'B',	// cLebanon
	'L', 'C',	// cStLucia
	'L', 'I',	// cLiechtenstein
	'L', 'K',	// cSriLanka
	'L', 'R',	// cLiberia
	'L', 'S',	// cLesotho
	'L', 'T',	// cLithuania
	'L', 'V',	// cLatvia
	'L', 'Y',	// cLibya
	'M', 'A',	// cMorrocco
	'M', 'C',	// cMonaco
	'M', 'D',	// cMoldova
	'M', 'G',	// cMadagascar
	'M', 'H',	// cMarshallIslands
	'M', 'K',	// cMacedonia
	'M', 'L',	// cMali
	'M', 'M',	// cMyanmar
	'M', 'N',	// cMongolia
	'M', 'O',	// cMacau
	'M', 'P',	// cNorthernMarianaIslands
	'M', 'Q',	// cMartinique
	'M', 'R',	// cMauritania
	'M', 'S',	// cMontserrat
	'M', 'T',	// cMalta
	'M', 'U',	// cMauritius
	'M', 'V',	// cMaldives
	'M', 'W',	// cMalawi
	'M', 'Z',	// cMozambique
	'N', 'A',	// cNamibia
	'N', 'C',	// cNewCalidonia
	'N', 'E',	// cNiger
	'N', 'F',	// cNorfolkIsland
	'N', 'G',	// cNigeria
	'N', 'I',	// cNicaragua
	'N', 'P',	// cNepal
	'N', 'R',	// cNauru
	'N', 'U',	// cNiue
	'O', 'M',	// cOman
	'P', 'A',	// cPanama
	'P', 'E',	// cPeru
	'P', 'F',	// cFrenchPolynesia
	'P', 'G',	// cPapuaNewGuinea
	'P', 'K',	// cPakistan
	'P', 'L',	// cPoland
	'P', 'M',	// cStPierreAndMiquelon
	'P', 'N',	// cPitcairn
	'P', 'R',	// cPuertoRico
	'P', 'T',	// cPortugal
	'P', 'W',	// cPalau
	'P', 'Y',	// cParaguay
	'Q', 'A',	// cQatar
	'R', 'E',	// cReunion
	'R', 'O',	// cRomania
	'R', 'U',	// cRussianFederation
	'R', 'W',	// cRwanda
	'S', 'A',	// cSaudiArabia
	'S', 'B',	// cSolomonIslands
	'S', 'C',	// cSeychelles
	'S', 'D',	// cSudan
	'S', 'H',	// cStHelena
	'S', 'I',	// cSlovenia
	'S', 'J',	// cSvalbardAndJanMayenIslands
	'S', 'K',	// cSlovakia
	'S', 'L',	// cSierraLeone
	'S', 'M',	// cSanMarino
	'S', 'N',	// cSenegal
	'S', 'O',	// cSomalia
	'S', 'R',	// cSuriname
	'S', 'T',	// cSaoTomeAndPrincipe
	'S', 'V',	// cElSalvador
	'S', 'Y',	// cSyranArabRepublic
	'S', 'Z',	// cSwaziland
	'T', 'C',	// cTurksAndCaicosIslands
	'T', 'D',	// cChad
	'T', 'F',	// cFrenchSouthernTerritories
	'T', 'G',	// cTogo
	'T', 'J',	// cTajikistan
	'T', 'K',	// cTokelau
	'T', 'M',	// cTurkmenistan
	'T', 'N',	// cTunisia
	'T', 'O',	// cTonga
	'T', 'P',	// cEastTimor
	'T', 'R',	// cTurkey
	'T', 'T',	// cTrinidadAndTobago
	'T', 'V',	// cTuvalu
	'T', 'Z',	// cTanzania
	'U', 'A',	// cUkraine
	'U', 'G',	// cUganda
	'U', 'M',	// cUnitedStatesMinorOutlyingIslands
	'U', 'Y',	// cUruguay
	'U', 'Z',	// cUzbekistan
	'V', 'A',	// cHolySee
	'V', 'C',	// cStVincentAndTheGrenadines
	'V', 'E',	// cVenezuela
	'V', 'G',	// cBritishVirginIslands
	'V', 'I',	// cUSVirginIslands
	'V', 'N',	// cVietNam
	'V', 'U',	// cVanuatu
	'W', 'F',	// cWallisAndFutunaIslands
	'W', 'S',	// cSamoa
	'Y', 'E',	// cYemen
	'Y', 'T',	// cMayotte
	'Y', 'U',	// cYugoslavia
	'Z', 'A',	// cSouthAfrica
	'Z', 'M',	// cZambia
	'Z', 'W',	// cZimbabwe
};

/***********************************************************************
 * Crc16CalcBigBlock
 *
 * Calculate the CRC16 checksum for a block of data which is potentially
 * bigger than 64K. DOLATER kwk - currently this isn't declared as static
 * because Crc.h has a declaration for it (but the code isn't associated
 * with a trap, thus it's part of the Simulator Palm OS library, but
 * you can't call it on the device).
 ***********************************************************************/

#if (EMULATION_LEVEL == EMULATION_NONE)
UInt16
Crc16CalcBigBlock(void* bufP, UInt32 count, UInt16 crc)
{
	UInt16 result = crc;
	
	while (count) {
		UInt32 subCount = min(count, maxUInt16);
		result = Crc16CalcBlock(bufP, subCount, result);
		count -= subCount;
		bufP = (void*)((UInt8*)bufP + subCount);
	}
	
	return(result);
} // Crc16CalcBigBlock
#endif


/***********************************************************************
 * DisplayError
 *
 * If error check level is full, then display an error message w/the
 * DB name, but only if the "show errors" feature flag is true.
 *
 *	08/27/99	kwk	New today.
 *	09/03/99	kwk	Add space for terminating null character.
 ***********************************************************************/

#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
static void
DisplayError(DmAccessPtr dbRef, const Char* msg)
{
	UInt32 attributes;
	if ((FtrGet(omFtrCreator, omFtrShowErrorsFlag, &attributes) != errNone)
	|| (attributes == 0)) {
		return;
	}
	
	// If we have a database ref, then we can display the name.
	if (dbRef != NULL) {
		Char* dbName = (Char*)dbRef->openP->hdrP->name;
		Char* newMsg = (Char*)MemPtrNew(StrLen(dbName) + 2 + StrLen(msg) + sizeOf7BitChar(chrNull));
		ErrFatalDisplayIf(newMsg == NULL, "Out of memory");
		
		StrCopy(newMsg, dbName);
		StrCat(newMsg, ": ");
		StrCat(newMsg, msg);
		
		ErrFatalDisplay(newMsg);
		MemPtrFree((MemPtr)newMsg);
	} else {
		ErrFatalDisplay(msg);
	}
} // DisplayError
#endif


/***********************************************************************
 * FindOverlayDatabaseByLocale
 *
 * Return the local ID of the overlay for the open database identified
 * by <baseRef> in the overlayID parameter, or 0 if no such overlay exists.
 * If no overlay is found, and <baseRef> is stripped, then return an error.
 *
 *	06/24/99	kwk	Created by Ken Krugler.
 *	09/29/99	kwk	Replaced MemPtrNew/Free w/static variable - we'll trade
 *						stack space at boot time for faster performance.
 *	05/17/00	kwk	Use new DmFindDatabaseWithTypeCreator, which should
 *						speed things up quite a bit.
 *
 **********************************************************************/

static Err
FindOverlayDatabaseByLocale(
	DmOpenRef baseRef, 						// ref to base database.
	const LmLocaleType* overlayLocale,	// target locale, or NULL.
	LocalID* overlayID)						// returned overlay DB, or 0.
{
	DmAccessType* baseAccessP;
	DmOpenInfoType* baseInfoP;
	DatabaseHdrType* baseHdrP;
	Char overlayDBName[dmDBNameLength];
	Err result;

	ErrNonFatalDisplayIf(baseRef == NULL, "Null baseRef parameter");
	ErrNonFatalDisplayIf(overlayID == NULL, "Null overlayID parameter");
	*overlayID = 0;
	
	// Make sure the base database is being opened in read-only mode.
	// Note that we don't return an error in this case for a stripped
	// base, as we're assuming that if somebody is opening the base
	// in write mode, they're doing something funky anyway, and thus
	// we shouldn't mess them up with weird overlay errors.
	
	baseAccessP = (DmAccessType*)baseRef;
	if ((baseAccessP->mode & dmModeWrite) != 0) {
		return(errNone);
	}
	
	// Make sure the base database is a resource database.
	baseInfoP = baseAccessP->openP;
	if (!baseInfoP->resDB) {
		return(errNone);
	}
	
	// Make sure we're not opening up an overlay database.
	baseHdrP = baseInfoP->hdrP;
	if (baseHdrP->type == sysFileTOverlay) {
		return(errNone);
	}
	
	// Construct the new name for the overlay file, which is something
	// like <appName>_jpJP for Japanese language in Japan.
	result = OmLocaleToOverlayDBName((Char*)baseHdrP->name, overlayLocale, overlayDBName);
	
	if (result != errNone) {
		return(result);
	}
	
	// If we can't find this database, or it doesn't have the right overlay
	// type, or the creator isn't the same as the base resource DB, then return.
	*overlayID = DmFindDatabaseWithTypeCreator(	baseInfoP->cardNo,
												overlayDBName,
												sysFileTOverlay,
												baseHdrP->creator);
	
	if (*overlayID == 0) {
		return(IsBaseStripped(baseRef) ? omErrBaseRequiresOverlay : errNone);
	} else {
		return(errNone);
	}
} // FindOverlayDatabaseByLocale


/***********************************************************************
 * FindResourceInOvly
 *
 * Find the requested resource record in the passed overlay record.
 *
 *	06/25/99	kwk	New today.
 ***********************************************************************/

static Boolean									// T->we found it.
FindResourceInOvly(
	const OmOverlaySpecType* inOverlay,	// ptr to locked 'ovly' resource.
	UInt32 inResType,							// resource type we're looking for.
	UInt16 inResID,							// resource id we're looking for.
	UInt16 *outIndex)							// index where we found it.
{
	
	// If the overlay is sorted, then we can do a binary search, which
	// will speed things up.
	if ((inOverlay->flags & omSpecAttrSorted) != 0)
	{
		OmOverlayRscType resToFind;
		Int32 index;
		
		resToFind.rscType = inResType;
		resToFind.rscID = inResID;
		if (SysBinarySearch(inOverlay->overlays,			// baseP
									inOverlay->numOverlays,		// numOfElements
									sizeof(OmOverlayRscType),	// width
									SearchOverlaySpec,			// searchF
									&resToFind,						// searchData,
									0,									// other
									&index,							// position
									false))							// findFirst
		{
			*outIndex = index;
			return(true);
		}
	}
	else
	{
		UInt16 i;
		const OmOverlayRscType* ovlyRscP = inOverlay->overlays;
		for (i = 0; i < inOverlay->numOverlays; i++, ovlyRscP++) {
			if ((ovlyRscP->rscType == inResType)
			&& (ovlyRscP->rscID == inResID)) {
				*outIndex = i;
				return(true);
			}
		}
	}
	
	*outIndex = 0;
	return(false);
} // FindResourceInOvly


/***********************************************************************
 * FindSystemDatabase
 *
 * Locate the system database, and return its ref and card.
 *
 *	06/25/99	kwk	New today.
 *	05/18/00	kwk	Change API to optionally return card and id, and always
 *						return as the function result the DmOpenRef.
 ***********************************************************************/

static DmOpenRef
FindSystemDatabase(
	UInt16* oSystemCard,
	LocalID* oSystemID,
	Char* oSystemName)
{
	DmOpenRef dbRef = NULL;
	
	// Find the opened System.prc file. We'll loop through the opened
	// databases until we find the system file.
	do {
		UInt16 dbCard;
		LocalID dbID;
		UInt32 dbType;
		UInt32 dbCreator;
		
		dbRef = DmNextOpenResDatabase(dbRef);
		
		if ((dbRef != NULL)
		&& (DmOpenDatabaseInfo(dbRef, &dbID, NULL, NULL, &dbCard, NULL) == errNone)
		&& (DmDatabaseInfo(dbCard, dbID, oSystemName, NULL, NULL, NULL, NULL, NULL, NULL,
							NULL, NULL, &dbType, &dbCreator) == errNone)
		&& (dbType == sysFileTSystem)
		&& (dbCreator == sysFileCSystem)) {
			if (oSystemCard != NULL)
			{
				*oSystemCard = dbCard;
			}
			if (oSystemID != NULL)
			{
				*oSystemID = dbID;
			}
			return(dbRef);
		}
	} while (dbRef != NULL);
	
	ErrFatalDisplay("Can't find system database");
	return(NULL);
} // FindSystemDatabase


/***********************************************************************
 * PrvGet1Resource
 *
 * Load a resource from the resource database <inDatabase>.
 *
 *	06/25/99	kwk	New today.
 ***********************************************************************/

static MemHandle
PrvGet1Resource(DmOpenRef inDatabase, UInt32 inType, UInt16 inID)
{
	Int16 index = DmFindResource(inDatabase, inType, inID, NULL);
	if (index != -1) {
		return(DmGetResourceIndex(inDatabase, index));
	} else {
		return(NULL);
	}
} // PrvGet1Resource


/************************************************************
 *
 *	FUNCTION:	PrvLoadScreens
 *
 *	DESCRIPTION: Open splashscreen.prc and load the two images.
 *
 *	PARAMETERS:
 *		systemLocale	->	Locale to use when opening DB
 *		splashImage		-> Ptr to handle to resource image.
 *		resetImage		-> Ptr to handle to resource image.
 *
 *	RETURNS:	Error code or errNone
 *
 * HISTORY:
 *		10/23/99	kwk	Created by Ken Krugler.
 *		05/17/00	kwk	Use DmGetNextDatabaseByTypeCreator to find the
 *							splashscreen DB, since this is faster than
 *							calling DmFindDatabase.
 *
 *************************************************************/
static Err
PrvLoadScreens(UInt16 cardNo, const LmLocaleType* systemLocale,
	MemHandle* splashImage, MemHandle* resetImage)
{
	LocalID dbID;
	Err result;
	DmOpenRef dbRef;
	UInt16 dbCard;
	DmSearchStateType searchState;
	
	// Find and open the splashscreen database.
	result = DmGetNextDatabaseByTypeCreator(true,
														&searchState,
														sysFileTSplash,
														sysFileCSystem,
														true,					// onlyLatestVers
														&dbCard,
														&dbID);
	
	// We'll assume that there's only one splashscreen DB on any card,
	// and it better be on the same card as the system DB.
	if (result != errNone)
	{
		return(result);
	}
	else if (dbCard != cardNo)
	{
		return(omErrInvalidLocale);
	}
	
	dbRef = DmOpenDBWithLocale(cardNo, dbID, dmModeReadOnly, systemLocale);
	if (dbRef == NULL) {
		result = DmGetLastErr();
		if (result == errNone) {
			result = omErrInvalidLocale;
		}
		
		return(result);
	}
	
	// Try loading the two resources that we need.
	*splashImage = DmGet1Resource(bsBitmapRsc, sysResIDBitmapSplash);
	ErrNonFatalDisplayIf(*splashImage == NULL, "Can't find splash screen image");

	*resetImage = DmGet1Resource(bsBitmapRsc, sysResIDBitmapConfirm);
	ErrNonFatalDisplayIf(*resetImage == NULL, "Can't find hard reset confirmation image");
	
	DmCloseDatabase(dbRef);
	
	if ((*splashImage == NULL) || (*resetImage == NULL)) {
		return(omErrInvalidSystemOverlay);
	}
	
	return(errNone);
} // PrvLoadScreens


/***********************************************************************
 * IsBaseStripped
 *
 * Return true if <inBase> has had all of its resources stripped out.
 * We assume that it's a resource database. Note that it's still OK
 * to use a stripped base w/o an overlay if there were no resources
 * that got overlaid, thus we don't return true if the resource count
 * in the ovly resource is 0.
 *
 *	06/25/99	kwk	New today.
 *	08/25/99	kwk	Check for numOverlays > 0 before returning true.
 ***********************************************************************/

static Boolean
IsBaseStripped(DmOpenRef inBase)
{
	Boolean isStripped;
	OmOverlaySpecType* ovlyP;
	
	MemHandle ovlyH = PrvGet1Resource(inBase, omOverlayRscType, omOverlayRscID);
	if (ovlyH == NULL) {
		return(false);
	}
	
	ovlyP = (OmOverlaySpecType*)MemHandleLock(ovlyH);
	isStripped = ((ovlyP->flags & omSpecAttrStripped) != 0) && (ovlyP->numOverlays > 0);
	MemPtrUnlock(ovlyP);
	
	return(isStripped);
} // IsBaseStripped


/***********************************************************************
 * LocaleToStr
 *
 * Return a string description of the locale <targetLocale> in <localeStr>.
 * If no string can be determined, return omErrUnknownLocale. The <localeStr>
 * buffer must be at least localeStrLen+1 bytes.
 *
 *	06/25/99	kwk	New today.
 **********************************************************************/

static Err
LocaleToStr(
	const LmLocaleType* targetLocale,
	Char* localeStr)
{
	const LanguageCode* languages;
	const CountryCode* countries;
	
	ErrNonFatalDisplayIf(targetLocale == NULL, "Null target locale ptr");
	ErrNonFatalDisplayIf(localeStr == NULL, "Null locale string ptr");
	
	if ((targetLocale->country >= kCountryCount)
	|| (targetLocale->language >= kLanguageCount)) {
		return(omErrUnknownLocale);
	}
	
	*localeStr++ = '_';
	
	languages = (const LanguageCode*)kLanguages;
	*localeStr++ = languages[targetLocale->language][0];
	*localeStr++ = languages[targetLocale->language][1];
	
	countries = (const CountryCode*)kCountries;
	*localeStr++ = countries[targetLocale->country][0];
	*localeStr++ = countries[targetLocale->country][1];
	
	*localeStr = '\0';

	return(errNone);
} // LocaleToStr


/***********************************************************************
 * OpenSystemOverlay
 *
 * Find and open the system overlay that's to be used with the current
 * system prc, based on <sysLocale>. Return the opened overlay DB ref
 * in <overlayRef>, or set that to NULL if we don't find it.
 *	07/01/99	kwk	New today.
 **********************************************************************/

static Err
OpenSystemOverlay(const LmLocaleType* sysLocale, DmOpenRef* overlayRef)
{
	DmOpenRef systemRef;
	UInt16 systemCard;
	LocalID overlayID;
	Err result;
	
	*overlayRef = NULL;
	
	// See if we've got a system overlay. First locate the system database,
	// then use that to search for the overlay.
	systemRef = FindSystemDatabase(&systemCard, NULL, NULL);
	result = FindOverlayDatabaseByLocale(systemRef, sysLocale, &overlayID);
	
	if (result != errNone) {
		return(result);
	} else if ((overlayID == 0)
	|| (!VerifySystemOverlay(systemCard, sysLocale, systemRef, overlayID))) {
		return(omErrInvalidLocale);
	}
	
	// Now all we've got to do is open it up.
	
	*overlayRef = DmOpenDBNoOverlay(systemCard, overlayID, dmModeReadOnly | dmModeExclusive);
	if (*overlayRef == NULL) {
		result = DmGetLastErr();
		return(result);
	}
	
	return(errNone);
} // OpenSystemOverlay


/***********************************************************************
 * PrvStrNCopy
 *
 * Copy up to <n> bytes from <srcP> to <dstP>. We know that we're only
 * going to be copying 7-bit ASCII characters (DB name), so we roll our
 * own routine to avoid dependencies on the Text Mgr being alive.
 **********************************************************************/
static void PrvStrNCopy(Char* dstP, const Char* srcP, Int16 n)
{
	while (*srcP && (n > 0))
	{
		*dstP++ = *srcP++;
		--n;
	}
	
	if (n > 0)
	{
		*dstP = 0;
	}
} // PrvStrNCopy


/***********************************************************************
 * SearchOverlaySpec
 *
 * See if the OmOverlayRscType pointed at by <arrayData> matches the
 * resource we're looking for in the OmOverlayRscType pointed at by
 * <searchData>.
 **********************************************************************/
static Int16
SearchOverlaySpec(void const *searchData, void const *arrayData, Int32 other)
{
	const OmOverlayRscType* searchEntry = (const OmOverlayRscType*)searchData;
	const OmOverlayRscType* arrayEntry = (const OmOverlayRscType*)arrayData;
	
	if (searchEntry->rscType < arrayEntry->rscType)
	{
		return(-1);
	}
	else if (searchEntry->rscType > arrayEntry->rscType)
	{
		return(1);
	}
	else if (searchEntry->rscID < arrayEntry->rscID)
	{
		return(-1);
	}
	else if (searchEntry->rscID > arrayEntry->rscID)
	{
		return(1);
	}
	else
	{
		return(0);
	}
} // SearchOverlaySpec


/***********************************************************************
 * StrToLocale
 *
 * Convert the string description of the locale in <localeStr> into a full
 * locale record and place the result in <targetLocale>.
 *
 *	06/25/99	kwk	New today.
 **********************************************************************/

static Err
StrToLocale(
	const Char* localeStr,
	LmLocaleType* targetLocale)
{
	LanguageCode theLanguage;
	const LanguageCode* languages;
	Boolean foundLanguage;
	CountryCode theCountry;
	const CountryCode* countries;
	Boolean foundCountry;
	UInt16 i;
	
	ErrNonFatalDisplayIf(localeStr == NULL, "Null locale string ptr");
	ErrNonFatalDisplayIf(targetLocale == NULL, "Null target locale ptr");
	
	if ((StrLen(localeStr) != localeStrLen)
	|| (localeStr[0] != '_')) {
		return(omErrBadOverlayDBName);
	}
	
	theLanguage[0] = localeStr[1]; theLanguage[1] = localeStr[2];
	languages = (const LanguageCode*)kLanguages;
	foundLanguage = false;
	
	for (i = 0; i < kLanguageCount; i++) {
		if ((languages[i][0] == theLanguage[0])
		&& (languages[i][1] == theLanguage[1])) {
			targetLocale->language = i;
			foundLanguage = true;
			break;
		}
	}
	
	if (!foundLanguage) {
		return(omErrUnknownLocale);
	}
	
	theCountry[0] = localeStr[3]; theCountry[1] = localeStr[4];
	countries = (const CountryCode*)kCountries;
	foundCountry = false;
	
	for (i = 0; i < kCountryCount; i++) {
		if ((countries[i][0] == theCountry[0])
		&& (countries[i][1] == theCountry[1])) {
			targetLocale->country = i;
			foundCountry = true;
			break;
		}
	}
	
	if (!foundCountry) {
		return(omErrUnknownLocale);
	}
	
	return(errNone);
} // StrToLocale


/***********************************************************************
 * VerifyOverlayDatabase
 *
 * Return true if the open overlay database identified by <overlayRef>
 * matches both the open base database idetified by <baseRef> and the current
 * system locale.  Otherwise, return false.
 *
 *	HISTORY:
 *		06/25/99	kwk	Created by Ken Krugler.
 *		06/28/99	kwk	Support overlaying of resources that weren't listed in
 *							the base database's ovly resource. Also check for a
 *							stripped base & one or more missing overlay resources.
 *		06/30/99	kwk	Use xprf=0 resource for disabling overlays.
 *		08/04/99	kwk	Do quick exit if overlay contains no overlaid resources.
 *		08/13/99	kwk	Fixed bug where an add case (resource in overlay DB,
 *							but not in base) would wind up passing NULL to MemHandleSize,
 *							and also tried to validate length/checksum.
 *		09/29/99	kwk	If base DB has an ovly resource, use baseChecksum field
 *							to do a fast validation.
 *		08/21/00	kwk	If the master checksums don't match between the base
 *							and the overlay, do slow comparison versus bailing out.
 *
 **********************************************************************/

static Boolean
VerifyOverlayDatabase(
	DmOpenRef baseRef,
	DmOpenRef overlayRef)
{
	const OmOverlaySpecType* baseOvlyP = NULL;
	MemHandle baseOvlyH;
	MemHandle ovlyH;
	const OmOverlaySpecType* ovlyP;
	Boolean valid = false;
	DmOpenInfoPtr openP;
#if (ERROR_CHECK_LEVEL == ERROR_CHECK_FULL)
	LmLocaleType ovlyLocale;
	DmOpenInfoPtr ovlyInfoP;
#endif
	UInt16 numExtraBaseResources = 0;
	const OmOverlayRscType* rscOvlyP;
	int i;
	
	// First try to get the pref=1 resource from the base...if that exists,
	// and it's got the 'no overlay' flag set, then we're out of here. Note
	// that we don't bother checking the extended prefs version, because
	// we know that the first version had the flags, and fields are only
	// added to the structure, never removed.
	Boolean noOverlay = false;
	MemHandle prefH = PrvGet1Resource(baseRef, sysResTExtPrefs, sysResIDExtPrefs);
	if (prefH != NULL)
	{
		SysExtPrefsType* prefP = (SysExtPrefsType*)MemHandleLock(prefH);
		noOverlay = (prefP->flags & sysExtPrefsNoOverlayFlag) != 0;
		MemPtrUnlock(prefP);
	}
	
	if (noOverlay)
	{
		return(false);
	}

	// Get the overlay resource from the overlay database. This has to
	// exist if we're going to validate the overlay.
	ovlyH = PrvGet1Resource(overlayRef, omOverlayRscType, omOverlayRscID);
	if (ovlyH == NULL)
	{
		ECDisplayError(overlayRef, "No 'ovly' resource in overlay database");
		return(false);
	}
	
	ovlyP = (const OmOverlaySpecType*)MemHandleLock(ovlyH);
	if (ovlyP->version != omOverlayVersion)
	{
		ECDisplayError(overlayRef, "Overlay resource has wrong version");
		goto Exit;
	}
	else if ((((DmAccessPtr)overlayRef)->openP->numRecords == 1)
			&& (ovlyP->numOverlays == 0))
	{
		// If the only resource in the overlay is the ovly, then it's
		// an empty overlay, so exit quickly. Note that in the future
		// the ovly might contain info such as 'hide', thus we also
		// want to check for no entries.
		goto Exit;
	}
	
	// Now grab the base overlay resource, if it exists. If the base has
	// been stripped then this has to exist, otherwise it's optional
	// but does speed up the verification. Note that if the version is
	// wrong, we pretend like it doesn't exist, which could cause problems
	// for a stripped base w/an invalid overlay resource.
	baseOvlyH = PrvGet1Resource(baseRef, omOverlayRscType, omOverlayRscID);
	if (baseOvlyH != NULL)
	{
		baseOvlyP = (const OmOverlaySpecType*)MemHandleLock(baseOvlyH);
		if (baseOvlyP->version != omOverlayVersion)
		{
			MemPtrUnlock((MemPtr)baseOvlyP);
			baseOvlyP = NULL;
		}
	}
	
	// Make sure the country, language, creator and type of the
	// base database is what we expect. This should always be the
	// case, but be extra safe.
	openP = ((DmAccessPtr)baseRef)->openP;
	if ((openP->hdrP->type != ovlyP->baseDBType)
	 || (openP->hdrP->creator != ovlyP->baseDBCreator))
	{
		ECDisplayError(overlayRef, "Overlay resource has wrong type/creator");
		goto Exit;
	}
	
#if (ERROR_CHECK_LEVEL == ERROR_CHECK_FULL)
	ovlyInfoP = ((DmAccessPtr)overlayRef)->openP;
	if (OmOverlayDBNameToLocale((Char*)ovlyInfoP->hdrP->name, &ovlyLocale) != errNone)
	{
		ECDisplayError(overlayRef, "Overlay DB has bad name");
	}
	
	if ((ovlyLocale.country != ovlyP->targetLocale.country)
	 || (ovlyLocale.language != ovlyP->targetLocale.language))
	{
		ECDisplayError(overlayRef, "Overlay DB name locale != overlay resource locale");
	}
#endif

	// If the base overlay resource exists, compare base checksums as a quick
	// way of validating. Note that this means we assume the overlay ovly has
	// been validated against the base ovly (all of the checks we do below in
	// our loop) if the checksums match. If the base checksum is zero, or the
	// overlay checksum is zero, or the checksums do not match, then we fall
	// back to our slower validation (resource by resource). The base checksums
	// might not match, but the overlay is still valid, if one of the overlay
	// resources is sorted but the other one isn't.
	if ((baseOvlyP != NULL)
	 && (baseOvlyP->baseChecksum != 0)
	 && (ovlyP->baseChecksum != 0)
	 && (baseOvlyP->baseChecksum == ovlyP->baseChecksum))
	{
		valid = true;
		goto Exit;
	}
	
	// Make sure that each resource is the appropriate length, and has
	// the expected checksum (data).
	rscOvlyP = ovlyP->overlays;
	for (i = 0; i < ovlyP->numOverlays; i++, rscOvlyP++)
	{
		UInt32 rscLength;
		UInt32 rscChecksum = 0;
		Boolean inBaseOvly = false;
		Boolean rscAdded = false;
		
		// Make sure the overlayType is valid.
#if (ERROR_CHECK_LEVEL == ERROR_CHECK_FULL)
		if ((rscOvlyP->overlayType != omOverlayKindReplace)
		 && (rscOvlyP->overlayType != omOverlayKindAdd))
		{
			ECDisplayError(overlayRef, "Overlay resource has invalid overlayType");
			goto Exit;
		}
#endif

		// If we have an overlay resource in the base DB, see if we can find
		// it listed. If so, then we better not be saying that it's an add.
		if (baseOvlyP != NULL)
		{
			UInt16 index;
			if (FindResourceInOvly(baseOvlyP, rscOvlyP->rscType, rscOvlyP->rscID, &index))
			{
				if (rscOvlyP->overlayType == omOverlayKindAdd)
				{
					ECDisplayError(overlayRef, "Overlay DB adding resource that exists in base ovly");
					goto Exit;
				}
				
				// Must be a replacement (not add), so we're OK.
				rscLength = baseOvlyP->overlays[index].rscLength;
				rscChecksum = baseOvlyP->overlays[index].rscChecksum;
				inBaseOvly = true;
			}
			else
			{
				// If it exists, then we need to make note of it as being a resource
				// that wasn't in the base overlay, but was overlaid.
				numExtraBaseResources += 1;
			}
		}
		
		if (!inBaseOvly)
		{
			MemHandle rscH = PrvGet1Resource(baseRef, rscOvlyP->rscType, rscOvlyP->rscID);
			if (rscH == NULL)
			{
				// The resource doesn't exist in the base, so we better be adding it.
				if (rscOvlyP->overlayType != omOverlayKindAdd)
				{
					ECDisplayError(baseRef, "Base DB is missing a resource");
					goto Exit;
				}
				
				rscAdded = true;
			}
			else
			{
				if (rscOvlyP->overlayType == omOverlayKindAdd)
				{
					ECDisplayError(overlayRef, "Overlay DB adding resource that exists in base DB");
					goto Exit;
				}
				
				// We've got a resource in the overlay DB that's a replacement for
				// a resource in the base DB which was not listed in the base DB's
				// overlay resource (or the overlay resource didn't exist), so we
				// have to calc the length/checksum at run-time.
				rscLength = MemHandleSize(rscH);
				if (rscLength == rscOvlyP->rscLength)
				{
					void* rscP = MemHandleLock(rscH);
					rscChecksum = Crc16CalcBigBlock(rscP, rscLength, 0);
					MemPtrUnlock(rscP);
				}
			}
		}
		
		// If it's not the case of the resource being added by the overlay, then
		// we need to verify that what it was designed to replace matches what the
		// base says it wants to have replaced.
		if (!rscAdded)
		{
			if (rscLength != rscOvlyP->rscLength)
			{
				ECDisplayError(overlayRef, "Overlay DB resource has different length");
				goto Exit;
			}
			
			if (rscChecksum != rscOvlyP->rscChecksum)
			{
				ECDisplayError(overlayRef, "Overlay DB resource has different checksum");
				goto Exit;
			}
		}
	}
	
	// If we've got a base overlay resource, and it indicates that the base has been
	// stripped, and the number of stripped resources + the number of resources that
	// were overlaid but not stripped is greater than the number of overlay resources,
	// we've got problems...at least one stripped resource must be absent from the
	// overlay database.
	if ((baseOvlyP != NULL)
	 && ((baseOvlyP->flags & omSpecAttrStripped) != 0)
	 && ((baseOvlyP->numOverlays + numExtraBaseResources) > ovlyP->numOverlays))
	{
		ECDisplayError(overlayRef, "Overlay DB is missing resource stripped from base");
		goto Exit;
	}
	
	valid = true;
	
Exit:
	if (ovlyP != NULL)
	{
		MemPtrUnlock((MemPtr)ovlyP);
	}
	
	if (baseOvlyP != NULL)
	{
		MemPtrUnlock((MemPtr)baseOvlyP);
	}
	
	return(valid);
} // VerifyOverlayDatabase


/***********************************************************************
 *
 *	FUNCTION:	VerifySystemOverlay
 *
 *	DESCRIPTION: Verify that <inOverlay> on card <inCard> is a valid
 *		overlay for <inBase>. Note that we also check for the presence
 *		of a valid splashscreen DB (contains splash & hard reset images).
 *
 *	PARAMETERS:
 *		inCard		-> Card containing the system base DB.
 *		sysLocale	-> Locale to verify.
 *		inBase		->	Ref to opened system base DB.
 *		inOverlay	-> LocalID of system overlay for use w/base.
 *
 *	RETURNS:		True if the overlay is valid for use with the system.
 *
 *	HISTORY:
 *		06/25/99	kwk	Created by Ken Krugler.
 *		09/17/99	kwk	Don't go for exclusive open access, as the overlay might
 *							be opened by somebody else.
 *		10/23/99	kwk	Call PrvLoadScreens to verify that we also have a
 *							splashcreen/hard reset screen to use w/sysLocale.
 *		09/18/00	kwk	Make sure the locale module exists.
 *
 ***********************************************************************/

static Boolean
VerifySystemOverlay(UInt16 inCard, const LmLocaleType* sysLocale, DmOpenRef inBase, LocalID inOverlay)
{
	Boolean valid;

	DmOpenRef overlayRef = DmOpenDBNoOverlay(inCard, inOverlay, dmModeReadOnly);
	if (overlayRef == NULL) {
		return(false);
	}
	
	valid = VerifyOverlayDatabase(inBase, overlayRef);
	
	// If the overlay is valid, then make sure we can also load the
	// two images we need from the splashcreen DB.
	// DOLATER kwk - there's still a problem with a ROM built without
	// overlays, where if you add just the System_jpJP.prc, but not
	// the corresponding splashscreen overlay, it still returns true.
	if (valid) {
		MemHandle splashImage, resetImage;
		DmAccessType* overlayAccessP = (DmAccessType*)overlayRef;
		
		valid = PrvLoadScreens(	overlayAccessP->openP->cardNo,
										sysLocale,
										&splashImage,
										&resetImage) == errNone;
	}
	
	// If the splashscreen images were found, then make sure that the
	// locale module exists, as specified by the locale module name
	// saved in the system overlay.
	if (valid) {
		LocalID moduleID;
		MemHandle localeNameH = DmGetResource(strRsc, localeModuleNameStrID);
		ErrNonFatalDisplayIf(localeNameH == NULL, "Missing locale module name string");
	
		moduleID = DmFindDatabaseWithTypeCreator(	inCard,
																(Char*)MemHandleLock(localeNameH),
																sysFileTLocaleModule,
																sysFileCSystem);
		MemHandleUnlock(localeNameH);
		valid = (moduleID != 0);
	}
	
	DmCloseDatabase(overlayRef);
	return(valid);
} // VerifySystemOverlay


/***********************************************************************
 * OmInit
 *
 * Initialize the Overlay Manager.
 *
 *	06/24/99	kwk	New today.
 *	06/28/99	kwk	Removed overlay preference stuff - rely on features
 *						for current locale, and NVParam for system locale.
 *	09/16/99	kwk	Removed debugging code, since MakeCard should always
 *						be setting up the NVParam block for us.
 *	05/18/00	kwk	Set up default locale for overlays, and call FtrInit()
 *						here vs. higher up.
 *						Protect the splashscreen overlay DB, so that if it's
 *						in ROM, nobody can delete it.
 *	05/23/00	kwk	Don't call MemGetRomNVParams if built for Simulator.
 * 08/29/00	spk	Open DeviceResources.prc right after System.prc if available
 *	11/22/00	kwk	Reduce stack requirements by using dynamic memory for
 *						the DmSearchStateType & SysNVParamsType structs, and
 *						the system/overlay DB names (was 152 bytes, now 26 bytes).
 *
 **********************************************************************/
void
OmInit(void)
{
	LmLocaleType romLocale;

#if (EMULATION_LEVEL == EMULATION_NONE)
	SysNVParamsType* romParamsP;
	Err result;
	DmSearchStateType* searchStateP;
	UInt16 systemCard;
	LocalID systemDB;
	UInt16 deviceCard;
	LocalID deviceDB;
	UInt16 splashCard;
	LocalID splashDB;
	LocalID splashOverlayDB;
	Char* splashName;
	Char* overlayDBName;
	LmLocaleType systemLocale;
	DmOpenRef dbP;
	
	// Find and open the system resource database.
	searchStateP = (DmSearchStateType*)MemPtrNew(sizeof(DmSearchStateType));
	result = DmGetNextDatabaseByTypeCreator(	true,
															searchStateP,
															sysFileTSystem,
															sysFileCSystem,
															true,					// onlyLatestVers
															&systemCard,
															&systemDB);
	
	// If we can't find the system DB, then all we can do is a DbgBreak;
	// the SysFatalAlert trap isn't installed yet, we can't draw text, etc.
	ErrDbgBreakIf((result != errNone) || (systemDB == 0));

	// Open the system DB and keep it opened.
	OmGetSystemLocale(&systemLocale);
	dbP = DmOpenDBWithLocale(systemCard, systemDB, dmModeReadOnly+dmModeLeaveOpen, &systemLocale);
	ErrDbgBreakIf(dbP == NULL);

	//  now open the device DB if available, and keep it opened
	//	try palm device creator id...
	result = DmGetNextDatabaseByTypeCreator(	true,
															searchStateP,
															sysFileTSystem,
															sysFileCPalmDevice,
															true,				// onlyLatestVers
															&deviceCard,
															&deviceDB);
	if (result != errNone)
	{
		//  maybe it's an oem licensee device...
		result = DmGetNextDatabaseByTypeCreator(	true,
														searchStateP,
														sysFileTSystem,
														sysFileCOEMSystem,
														true,					// onlyLatestVers
														&deviceCard,
														&deviceDB);
	}
															
	if (result == errNone)
	{
		DmOpenRef dbDeviceP;
		dbDeviceP = DmOpenDBWithLocale(deviceCard, deviceDB, dmModeReadOnly+dmModeLeaveOpen, &systemLocale);
		ErrDbgBreakIf(dbDeviceP == NULL);
	}

	// Protect the splashscreen overlay DB, so that if it's in RAM, nobody can
	// delete it. First we'll locate the base (should be only one of these, in ROM).
	result = DmGetNextDatabaseByTypeCreator(	true,
															searchStateP,
															sysFileTSplash,
															sysFileCSystem,
															true,					// onlyLatestVers
															&splashCard,
															&splashDB);
	ErrDbgBreakIf((result != errNone) || (splashDB == 0));
	MemPtrFree((MemPtr)searchStateP);
	
	// Construct the name for the splashscreen overlay.
	splashName = (Char*)MemPtrNew(dmDBNameLength);
	result = DmDatabaseInfo(splashCard, splashDB, splashName,
					NULL, NULL, NULL, NULL, NULL, NULL, NULL,
					NULL, NULL, NULL);
	ErrDbgBreakIf(result != errNone);

	overlayDBName = (Char*)MemPtrNew(dmDBNameLength);
	result = OmLocaleToOverlayDBName(splashName, &systemLocale, overlayDBName);
	ErrDbgBreakIf(result != errNone);
	MemPtrFree((MemPtr)splashName);
	
	// We might not have a splashscreen DB overlay, if the ROM was
	// built without overlays.
	splashOverlayDB = DmFindDatabaseWithTypeCreator(splashCard,
																	overlayDBName,
																	sysFileTOverlay,
																	sysFileCSystem);
	MemPtrFree((MemPtr)overlayDBName);
	
	if (splashOverlayDB != 0)
	{
		result = DmDatabaseProtect(splashCard, splashOverlayDB, true);
		if (result != dmErrROMBased)
		{
			ErrDbgBreakIf(result);
		}
	}
#endif

	// Initialize the Feature Mgr. We do this here, since it has to be
	// called _after_ OmInit, but we also have to set up a feature here.
	// We'll let FtrInit set up the country & language features using the
	// 'feat' (10001) resource located inside of the system overlay. These
	// features are what define the current locale, versus the system locale
	// (which is contained by the card 0 RAM store NVParams).
	FtrInit();
	
	// Set up the "default" locale feature, which is the locale to try to
	// use when we can't find an overlay for a stripped base. We'll initialize
	// this to be the ROM locale, so that if somebody installs only a system
	// & splashscreen DB, and then changes the system (and current) locale to
	// be this new locale, we'll be able to find DBs to use for the stripped
	// bases in the ROM. Note that we assume LmLocaleType fits in 32 bits.
#if (EMULATION_LEVEL == EMULATION_NONE)
	romParamsP = (SysNVParamsType*)MemPtrNew(sizeof(SysNVParamsType));
	MemGetRomNVParams(romParamsP);
	romLocale.country = romParamsP->localeCountry;
	romLocale.language = romParamsP->localeLanguage;
	MemPtrFree((MemPtr)romParamsP);
#else
	// Simulator doesn't have a ROM store, so calling MemGetROMNVParams isn't
	// going to work. Default to the enUS locale. We could try harder to
	// do something smart here (e.g. base it on the system locale), but
	// the default feature should never get used w/the Simulator.
	romLocale.country = cUnitedStates;
	romLocale.language = lEnglish;
#endif
	FtrSet(omFtrCreator, omFtrDefaultLocale, *(UInt32*)&romLocale);
} // OmInit


/***********************************************************************
 * OmOpenOverlayDatabase
 *
 * Return the DmOpenRef of the overlay for the open database identified
 * by <baseRef> in the overlayRef parameter, or 0 if no such overlay exists.
 * If no overlay is found, and <baseRef> is stripped, then return an error.
 *
 *	06/30/99	kwk	New today.
 *	05/18/00	kwk	Also try the default locale, if the caller was requesting
 *						the current locale (overlayLocale == NULL) and we couldn't
 *						find an overlay to use with a stripped base.
 **********************************************************************/

Err
OmOpenOverlayDatabase(
	DmOpenRef baseRef, 						// ref to base database.
	const LmLocaleType* overlayLocale,	// target locale, or NULL.
	DmOpenRef* overlayRef)					// returned overlay DB, or 0.
{
	LocalID overlayID;
	Err result;
	DmAccessPtr baseAccessP;

	ErrNonFatalDisplayIf(baseRef == NULL, "Null baseRef parameter");
	ErrNonFatalDisplayIf(overlayRef == NULL, "Null overlayRef parameter");

	// Assume we won't find the database.
	*overlayRef = NULL;
	
	result = FindOverlayDatabaseByLocale(baseRef, overlayLocale, &overlayID);
	
	// If we can't find an overlay for a stripped base, and the requested
	// locale is the current locale (NULL ptr), then also try to get the
	// DB from the default locale.
	if ((result == omErrBaseRequiresOverlay) && (overlayLocale == NULL))
	{
		LmLocaleType defaultLocale;
		result = FtrGet(omFtrCreator, omFtrDefaultLocale, (UInt32*)&defaultLocale);
		ErrNonFatalDisplayIf(result != errNone, "No default locale feature");
		result = FindOverlayDatabaseByLocale(baseRef, &defaultLocale, &overlayID);
	}
	
	// It's valid for us to return errNone & no overlay, if we can't find
	// the overlay but the base isn't stripped.
	if ((result != errNone) || (overlayID == 0)) {
		return(result);
	}
	
	// We found the database, now verify that it's OK to use it with
	// the base. We need to open it up for this to work.
	baseAccessP = (DmAccessPtr)baseRef;
	*overlayRef = DmOpenDBNoOverlay(	baseAccessP->openP->cardNo,
												overlayID,
												overlayDefaultMode | (overlayBaseMode & baseAccessP->mode));

	// We might not be able to open it because of access privileges.
	
	if (*overlayRef == NULL) {
		return(DmGetLastErr());
	}
	
	// Note that we will return a stripped base error, without trying
	// the default locale, if the overlay isn't valid. Our default
	// locale is only used for _missing_ overlays, to enable partial
	// system localizations.
	if (VerifyOverlayDatabase(baseRef, *overlayRef)) {
		return(errNone);
	} else {
		DmCloseDatabase(*overlayRef);
		*overlayRef = NULL;
		return(IsBaseStripped(baseRef) ? omErrBaseRequiresOverlay : errNone);
	}
} // OmOpenOverlayDatabase


/***********************************************************************
 * OmLocaleToOverlayDBName
 *
 * Return in <overlayDBName> an overlay database name that's appropriate
 * for the base name <baseDBName> and the locale <targetLocale>. If the
 * <targetLocale> param in NULL, use the current locale. The <overlayDBName>
 * buffer must be at least dmDBNameLength bytes.
 *
 *	06/30/99	kwk	New today from OmLocaleToStr
 **********************************************************************/

Err
OmLocaleToOverlayDBName(
	const Char* baseDBName,
	const LmLocaleType* targetLocale,
	Char* overlayDBName)
{
	LmLocaleType ourLocale;
	LmLocaleType* theLocaleP;
	
	ErrNonFatalDisplayIf(baseDBName == NULL, "Null base name ptr");
	ErrNonFatalDisplayIf(overlayDBName == NULL, "Null overlay name ptr");
	
	if (targetLocale == NULL) {
		OmGetCurrentLocale(&ourLocale);
		theLocaleP = &ourLocale;
	} else {
		theLocaleP = (LmLocaleType*)targetLocale;
	}
	
	PrvStrNCopy(overlayDBName, baseDBName, maxBaseDBName);
	overlayDBName[maxBaseDBName] = '\0';
	return(LocaleToStr(theLocaleP, &overlayDBName[StrLen(overlayDBName)]));
} // OmLocaleToOverlayDBName
	

/***********************************************************************
 * OmOverlayDBNameToLocale
 *
 * Given the name of an overlay database in <overlayDBName>, return back
 * the overlay in overlayLocale. If the name isn't an overlay name,
 * return omErrBadOverlayDBName.
 *
 *	06/30/99	kwk	New today from OmStrToLocale
 **********************************************************************/

Err
OmOverlayDBNameToLocale(
	const Char* overlayDBName,
	LmLocaleType* overlayLocale)
{
	UInt16 nameLen;
	
	ErrNonFatalDisplayIf(overlayDBName == NULL, "Null overlay name ptr");
	ErrNonFatalDisplayIf(overlayLocale == NULL, "Null locale ptr");
	
	nameLen = StrLen(overlayDBName);
	if (StrLen(overlayDBName) <= localeStrLen) {
		return(omErrBadOverlayDBName);
	}
	
	return(StrToLocale(&overlayDBName[nameLen - localeStrLen], overlayLocale));
} // OmOverlayDBNameToLocale


/***********************************************************************
 * OmGetCurrentLocale
 *
 * Return the current locale in <currentLocale>. This may not be the same as
 * the locale saved in the card 0 RAM store NVParams record, which will
 * take effect after restart.
 *
 *	06/24/99	kwk	New today.
 **********************************************************************/

void
OmGetCurrentLocale(
	LmLocaleType* currentLocale)
{
	UInt32 attribute;
	Err result;
	
	ErrNonFatalDisplayIf(currentLocale == NULL, "NULL parameter");
	
	result = FtrGet(sysFtrCreator, sysFtrNumLanguage, &attribute);
	ErrNonFatalDisplayIf(result != errNone, "No language feature");
	currentLocale->language = attribute;
	
	result = FtrGet(sysFtrCreator, sysFtrNumCountry, &attribute);
	ErrNonFatalDisplayIf(result != errNone, "No country feature");
	currentLocale->country = attribute;
} // OmGetCurrentLocale


/***********************************************************************
 *
 *	FUNCTION:	OmGetNextSystemLocale
 *
 *	DESCRIPTION: Return the next valid system locale in <oLocaleP>.
 *		omErrInvalidLocaleIndex will be returned if there are no more
 *		valid system locales
 *
 *	PARAMETERS:
 *		iNewSearch		 ->	True if this is the first call in a series
 *									of calls to find system locales.
 *		ioStateInfoP	<->	State info passed back and forth, maintained
 *									by the routine
 *		oLocaleP			<-		Set to locale of found system, if result is
 *									noErr.
 *
 *	RETURNED:
 *		noErr if <oLocaleP> has been set to the next valid system locale,
 *		otherwise omErrNoNextSystemLocale.
 *
 *	HISTORY:
 *		05/18/00	kwk	Created by Ken Krugler.
 *		08/18/00	kwk	Use StrNCompareAscii vs. StrNCompare, since the TextMgr
 *							might not be initialized yet, and we're comparing DB
 *							header names (which must be 7-bit ASCII).
 *		09/14/00	kwk	Modified to use extra info in state info record to
 *							support "finding" the system in a non-overlaid ROM.
 *					kwk	Catch case of valid system overlay in both ROM & RAM.
 *		09/18/00	kwk	Return omErrNoNextSystemLocale vs. omErrInvalidLocaleIndex.
 *
 ***********************************************************************/
Err OmGetNextSystemLocale(Boolean iNewSearch, OmSearchStateType* ioStateInfoP, LmLocaleType* oLocaleP)
{
	UInt16 theCard;
	LocalID theLocalID;
	
	ErrNonFatalDisplayIf(ioStateInfoP == NULL, "Null state ptr param");
	ErrNonFatalDisplayIf(oLocaleP == NULL, "Null locale ptr param");
	
	if (iNewSearch)
	{
		// First locate the system DB, so we know its name and card number.
		// For the simulator, there is no system database, so set things
		// up such that we'll fall out of the subsequent loop immediately.
#if (EMULATION_LEVEL == EMULATION_NONE)
		ioStateInfoP->systemDBRef = FindSystemDatabase(	&ioStateInfoP->systemDBCard,
																		NULL,
																		ioStateInfoP->systemDBName);
		ioStateInfoP->systemDBNameLen = StrLen(ioStateInfoP->systemDBName);
#else
		ioStateInfoP->systemDBRef = NULL;
		ioStateInfoP->systemDBCard = 0;
		ioStateInfoP->systemDBNameLen = 0;
#endif

		OmGetCurrentLocale(&ioStateInfoP->curLocale);
		ioStateInfoP->didNoOverlaySystem = false;
		ioStateInfoP->foundSystem = false;
	}
	
	// If we previously returned the system DB for a non-overlaid ROM,
	// we want to bail out immediately since otherwise we'll loop forever.
	else if (ioStateInfoP->didNoOverlaySystem)
	{
		return(omErrNoNextSystemLocale);
	}
	
	// Loop over all databases with the appropriate type & creator. For
	// each one we find, make sure it's valid to use with system DB.
	while (DmGetNextDatabaseByTypeCreator(	iNewSearch,
														&ioStateInfoP->searchState,
														sysFileTOverlay,
														sysFileCSystem,
														false, // onlyLatestVers
														&theCard,
														&theLocalID) == errNone)
	{
		Char dbName[dmDBNameLength];
		
		iNewSearch = false;
		
		if (theCard != ioStateInfoP->systemDBCard)
		{
			continue;
		}
		
		if (DmDatabaseInfo(theCard, theLocalID, dbName,
				NULL, NULL, NULL, NULL, NULL, NULL, NULL,
				NULL, NULL, NULL) != errNone)
		{
			continue;
		}
		
		// If we've got a match with the system database name, and the
		// remainder maps to a known locale, then we need to validate the DB.
		if ((StrNCompareAscii(dbName, ioStateInfoP->systemDBName, ioStateInfoP->systemDBNameLen) == 0)
		&& (StrToLocale(&dbName[ioStateInfoP->systemDBNameLen], oLocaleP) == errNone))
		{
			// If this DB is in ROM, see if we can find a DB with the same name
			// in RAM. If so, then ignore the ROM-based database, so that we
			// don't return two results for these two DBs.
			if (MemLocalIDKind(theLocalID) != memIDHandle)
			{
				LocalID dbID = DmFindDatabaseWithTypeCreator(theCard, dbName, sysFileTOverlay, sysFileCSystem);
				if ((dbID != 0) && (dbID != theLocalID))
				{
					// We have a different DB - assume it's coming out of RAM.
					ErrNonFatalDisplayIf(MemLocalIDKind(dbID) != memIDHandle,
												"More than two DBs with same name");
					continue;
				}
			}
			
			// If the locale matches the current locale, then what we've got
			// here is the current system overlay, so we don't need to verify it.
			if ((oLocaleP->language == ioStateInfoP->curLocale.language)
			 && (oLocaleP->country == ioStateInfoP->curLocale.country))
			{
				ioStateInfoP->foundSystem = true;
				return(errNone);
			}
			else if (VerifySystemOverlay(	ioStateInfoP->systemDBCard,
													oLocaleP,
													ioStateInfoP->systemDBRef,
													theLocalID))
			{
				return(errNone);
			}
		}
	} // while
	

	// We didn't find a next database. If we didn't find the system DB, then assume
	// we have a ROM built without overlays, so just return the system locale.
	if (!ioStateInfoP->foundSystem)
	{
		ioStateInfoP->foundSystem = true;
		ioStateInfoP->didNoOverlaySystem = true;
		OmGetSystemLocale(oLocaleP);
		return(errNone);
	}
	else
	{
		return(omErrNoNextSystemLocale);
	}
} // OmGetNextSystemLocale


/***********************************************************************
 * OmGetIndexedLocale
 *
 * Return the nth available locale in <theLocale>. Indexes are zero-based,
 * and the omErrInvalidLocaleIndex result will be returned if <localeIndex>
 * is out of bounds.
 *
 *	06/24/99	kwk	New today.
 *	09/16/99	kwk	For simulator builds, pretend like enUS is the only locale.
 *	09/17/99	kwk	For non-overlay builds, return back the system locale for
 *						localeIndex == 0.
 *	05/18/00	kwk	Call new OmGetNextSystemLocale to do the real work.
 *	09/18/00	kwk	Remap omErrNoNextSystemLocale to omErrInvalidLocaleIndex.
 **********************************************************************/

Err
OmGetIndexedLocale(
	UInt16 localeIndex,
	LmLocaleType* theLocale)
{
	OmSearchStateType searchState;
	UInt16 index;
	
	for (index = 0; index <= localeIndex; index++)
	{
		Err result = OmGetNextSystemLocale(index == 0, &searchState, theLocale);
		if (result != errNone)
		{
			if (result == omErrNoNextSystemLocale)
			{
				result = omErrInvalidLocaleIndex;
			}
			return(result);
		}
	}
	
	return(errNone);
} // OmGetIndexedLocale


/***********************************************************************
 * OmGetSystemLocale
 *
 * Return the system locale in <systemLocale>. This may not be the same as
 * the current locale, which is defined by the country & language features.
 *
 *	06/29/99	kwk	New today.
 **********************************************************************/

void
OmGetSystemLocale(
	LmLocaleType* systemLocale)
{
	SysNVParamsType nvParams;
	
	ErrNonFatalDisplayIf(systemLocale == NULL, "NULL parameter");
	
	if (MemNVParams(false, &nvParams) != errNone) {
		ErrFatalDisplay("Invalid RAM store");
	}

	systemLocale->country = nvParams.localeCountry;
	systemLocale->language = nvParams.localeLanguage;
} // OmGetSystemLocale


/***********************************************************************
 * OmSetSystemLocale
 *
 * Set the post-reset system locale to be <systemLocale>. Return the
 * omErrInvalidLocale error if the passed locale doesnÕt correspond to
 * a valid System.prc overlay. Note that we also need a corresponding
 * Splashscreen.prc, since that's where the splash/hard reset picts
 * are located.
 *
 *	06/24/99	kwk	New today.
 **********************************************************************/
Err
OmSetSystemLocale(const LmLocaleType* systemLocale)
{
	SysNVParamsType nvParams;
	DmOpenRef overlayRef;
	Err result;
	UInt16 savedSR;
	MemHandle splashImage;
	MemHandle resetImage;
	DmAccessType* overlayAccessP;
	
	ErrNonFatalDisplayIf(systemLocale == NULL, "NULL parameter");
	
	// Do quick check to see if the locale is the same.
	if (MemNVParams(false, &nvParams) != errNone) {
		ErrFatalDisplay("Invalid RAM store");
	}
	
	if ((nvParams.localeCountry == systemLocale->country)
	&& (nvParams.localeLanguage == systemLocale->language)) {
		return(errNone);
	}
	
	// Find and open up the system overlay. This will return back the DmOpenRef
	// of the overlay.
	
	result = OpenSystemOverlay(systemLocale, &overlayRef);
	if (result != errNone) {
		return(result);
	}
	
	if (overlayRef == NULL) {
		return(omErrInvalidLocale);
	}
	
	// Open up the splashscreen.prc with the specific locale. This contains two
	// images, one for the splashscreen and one for the hard reset screen.
	
	overlayAccessP = (DmAccessType*)overlayRef;
	result = PrvLoadScreens(overlayAccessP->openP->cardNo, systemLocale, &splashImage, &resetImage);
	
	// We're done, so close the database. It will get opened up at the
	// next OS boot, by OmInit.
	
	DmCloseDatabase(overlayRef);
	
	// Now get/set NVParam, but make sure interrupts are disabled so that
	// we don't wind up setting stale data. We'll do all Data/Memory Mgr
	// operations before disabling interrupts, to avoid deadlock.
	
	if (result == errNone) {
		MemPtr splashImageP = MemHandleLock(splashImage);
		MemPtr resetImageP = MemHandleLock(resetImage);
		
		savedSR = SysDisableInts();	
		
		result = MemNVParams(false, &nvParams);
		ErrFatalDisplayIf(result != errNone, "Invalid RAM store");
		
		nvParams.localeCountry = systemLocale->country;
		nvParams.localeLanguage = systemLocale->language;
		nvParams.splashScreenPtr = splashImageP;
		nvParams.hardResetScreenPtr = resetImageP;
		
		result = MemNVParams(true, &nvParams);
		ErrFatalDisplayIf(result != errNone, "Can't set NVParam locale info");
		
		SysRestoreStatus(savedSR);
		
		// Now reset the device, since we've changed the system locale.
		SysReset();
	}
	
	return(result);
} // OmSetSystemLocale


/***********************************************************************
 *
 *	FUNCTION:		OmGetRoutineAddress
 *
 *	DESCRIPTION:	Return back the address of the Overlay Mgr routine indicated
 *						by <inSelector>. If <inSelector> is invalid, return NULL.
 *
 *	PARAMTERS:		inSelector	->	routine selector.
 *
 *	RETURNS:			Address of routine, or NULL
 *
 *	HISTORY
 *		10/08/99	kwk	Created by Ken Krugler
 *
 ***********************************************************************/

void* OmGetRoutineAddress(OmSelector inSelector)
{
	if (inSelector > omMaxSelector)
		{
		return(NULL);
		}
	else
		{
#if EMULATION_LEVEL == EMULATION_NONE

		Int32 * tablePtr = (Int32 *)OmDispatchTable;
		Int16 offset = tablePtr[inSelector] & 0x0FFFF;
		return((void *)((UInt8 *)&tablePtr[inSelector] + 2 + offset));
		
#else	// EMULATION_LEVEL != EMULATION_NONE

		switch (inSelector)
			{
			case omInit:						return(OmInit);
			case omOpenOverlayDatabase:	return(OmOpenOverlayDatabase);
			case omLocaleToOverlayDBName:	return(OmLocaleToOverlayDBName);
			case omOverlayDBNameToLocale:	return(OmOverlayDBNameToLocale);
			case omGetCurrentLocale:		return(OmGetCurrentLocale);
			case omGetIndexedLocale:		return(OmGetIndexedLocale);
			case omGetSystemLocale:			return(OmGetSystemLocale);
			case omSetSystemLocale:			return(OmSetSystemLocale);
			case omGetRoutineAddress:		return(OmGetRoutineAddress);
			
			default:
				return(NULL);
			}
#endif
		}
} // OmGetRoutineAddress
