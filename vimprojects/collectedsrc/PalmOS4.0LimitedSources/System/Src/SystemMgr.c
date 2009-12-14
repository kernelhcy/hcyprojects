/******************************************************************************
 *
 * Copyright (c) 1994-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: SystemMgr.c
 *
 * Release: 
 *
 * Description:
 *		Handles initialization and creation of Pilot globals under
 *	both the native and emulation modes
 *
 * History:
 *   	10/26/94	RM		Created by Ron Marianetti
 *		07/03/95	VMK	Added setup for Serial Link Manager globals to SysReset()
 *		10/07/96 SCL	Added PrvRelocateData routine, modified SysAppStartup
 *		10/30/96 SCL	Changed SysAppStartup to ONLY call PrvRelocateData
 *							if the DATA resource includes relocation data
 *		10/30/96 SCL	SysAppStartup: CODE #0 and DATA #0 are now optional.
 *		05/05/98	art	Add Text Services support.
 *		10/04/98	kwk	In SysAppLaunch, add extra stack space for Japanese.
 *
 *****************************************************************************/

#define	NON_PORTABLE

#define DEBUG_LEVEL DEBUG_LEVEL_NONE

// Allow access to data structure internals
// DOLATER - Could use new bitmap API to get bitmap width/height
#define ALLOW_ACCESS_TO_INTERNALS_OF_BITMAPS
#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS

#include "PalmOptUserMode.h"

#include <PalmTypes.h>

// public system includes
#include <AlarmMgr.h>
#include	<ConsoleMgr.h>
#include <Chars.h>
#include <DataMgr.h>
#include <DateTime.h>
#include <DebugMgr.h>
#include <ExgLib.h>
#include <ErrorMgr.h>
#include	<ExgMgr.h>
#include <FeatureMgr.h>
#include <Graffiti.h>
#include	<HAL.h>
#include <IntlMgr.h>
#include <KeyMgr.h>
#include <LibTraps.h>
#include <NotifyMgr.h>
#include <NetMgr.h>
#include <Preferences.h>
#include <StringMgr.h>
#include <SystemMgr.h>
#include <SysEvtMgr.h>
#include <SysUtils.h>
#include <SoundMgr.h>
#include <TimeMgr.h>
#include <VFSMgr.h>

// public ui includes
#include <Find.h>
#include <Form.h>
#include <GraffitiReference.h>
#include <Keyboard.h>
#include <Launcher.h>
#include <FatalAlert.h>
#include <UIControls.h>

// private hardware includes
#include	"HwrGlobals.h"
#include "HwrMiscFlags.h"
#include "HwrROMToken.h"
#include	"HwrBattery.h"
#include	"HwrDock.h"			// for HwrPluggedIn
#include	"HwrDisplay.h"
#if EMULATION_LEVEL == EMULATION_NONE
#include	"HwrFlash.h"		// for FlashInit
#endif
#if EMULATION_LEVEL != EMULATION_NONE
#include "HwrBoot.h"			// for HwrInit
#include "HwrMemConfig.h"	// for HwrCalcDynamicHeapSize
#include "ExpansionMgr.h"
#include "VFSMgr.h"
#endif
#include	"HwrTimer.h"
#include	"HwrKeys.h"
#include "HwrSound.h"		// for HwrSoundOff

// private system includes
#include "NotifyPrv.h"
#include "DataPrv.h"
#include "SystemPrv.h"
#include "SysEvtPrv.h"
#include "TextServicesPrv.h"
#include "IntlPrv.h"			// For IntlInit & IntlHandleEvent
#include "OverlayPrv.h"		// For OmInit
#include "ConnectionMgrPrv.h" // For PrvCncMgrInit
#include "AlarmPrv.h"
#include "AttentionPrv.h"
#include "PasswordPrv.h"
#include "VFSMgrPrv.h"

// private ui includes
#include "UIInit.h"
#include "UIResourcesPrv.h"

/*
#include "DataPrv.h"
*/

#if EMULATION_LEVEL == EMULATION_NONE
	// Make sure this compiler constant was defined in the Makefile
	#ifndef CONSOLE_SERIAL_LIB
		#error	"The Compiler define CONSOLE_SERIAL_LIB MUST BE DEFINED ON THE COMMAND LINE!!"
	#endif
#endif 	//EMULATION_LEVEL != EMULATION_NONE


// Defines
#define	uiAppStackSize			0xA00
#define	maxParamSize			50

#define INTEGRATED_HSIMGR

// Private routines and types
typedef void 	ResetHandler(void);

static UInt32	PrvSysGetA5(void);

static UInt8 *  	PrvDecompressData(UInt8 * ptr, UInt8 * datasegment);

static UInt8 * 	PrvRelocateData(UInt8 * ptr, UInt8 * segment, long relocbase);

static Int16		PrvCallSafely(ProcPtr procP);

static Int16	 	PrvCallWithNewStack(ProcPtr procP, MemPtr newStack);

static void		TaskTerminationProc(UInt32 taskID, Int32 reason);

static void		InitSystemGlobals(void);

static Boolean	PrvHandleEvent(SysEventType * eventP, Boolean *outNeedsUI, Boolean inProcess);

static void		PrvHandleStackDeletes(void);

void ThisShouldGenerateALinkError(void);

#ifdef INTEGRATED_HSIMGR
// jhl 7/26/00 Integrate HSIMgr functionality
static UInt32 PrvHSIQueryPeripheral(SysHSIResponseType *peripheralResponseP);
static Boolean PrvHSIGetResponseLine(UInt16 openPort, Char *bufferP, UInt16 *bufferLengthP);
#endif // INTEGRATED_HSIMGR

// When we launch an app with no globals world, we set the high bit of A5
//  so that can get a bus error if the called app inadvertently accesses globals.
//  (it shouldn't).
#define PrvProtectSegmentRegister(r)	((MemPtr) ((UInt32) (r) | 0x80000000))
#define PrvUnprotectSegmentRegister(r)	((MemPtr) ((UInt32) (r) & ~0x80000000))


/************************************************************
 *
 *  FUNCTION: SysUnimplemented
 *
 *  DESCRIPTION: The entire trap table is initialized with this routine
 *		before installing the real handlers in case we forget to install any traps
 *
 *  PARAMETERS: void
 *
 *  RETURNS: void
 *
 *  CREATED: 10/26/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
void SysUnimplemented(void)
{
	ErrDisplay("Unimplemented");
}


/************************************************************
 *
 *  FUNCTION: SysColdBoot
 *
 *  DESCRIPTION: Does a cold, card boot. Re-formats all RAM areas
 *   of memory cards.
 *
 *  PARAMETERS: none
 *
 *  RETURNS:
 *
 *  CREATED: 10/31/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
void SysColdBoot(void *card0P, UInt32 card0Size, 
					  void *card1P, UInt32 card1Size, 
					  UInt32 sysCardHeaderOffset)
{

#if EMULATION_LEVEL != EMULATION_NONE
	SysPowerOn(card0P, card0Size, card1P, card1Size, sysCardHeaderOffset, true);

#else
	ResetHandler*		resetP;
	CardHeaderPtr		cardP;
	UInt16*					wP;
	UInt16					offset;
	CardInfoPtr			infoP;
	
	// Fetch the address of the reset vector out of the card header
	infoP = memCardInfoP(0);
	cardP = (CardHeaderPtr)(infoP->baseP + infoP->cardHeaderOffset);
	
	// The hard reset entry point is 2 past the normal one
	wP = (UInt16*)cardP->resetVector;
	
	// On the Pre328 board, the reset vector points to the area near 0
	//  where ROM appears right after reset, so we need to add the 
	//  current location of the ROM to that.
	if (((UInt32)wP & 0xFFFF0000) == 0) 
		wP = (UInt16*)((UInt32)wP + infoP->cardHeaderOffset);
	

	// Our PilotRez tool substitues a JMP *+offset instruction for
	// the first instruction of the startup code in 
	// the reset code resource boot #10000. So, we must calculate
	// the start of the reset code from this instruction.
	offset = wP[1] + 2;
	resetP = (ResetHandler*)((UInt32)wP + (UInt32)offset + 2);
	

	// Call it...
	(*resetP)();
#endif
}






/************************************************************
 *  FUNCTION: 		SysInit 
 *
 *  DESCRIPTION: 	Initializes the System Manager
 *
 *  PARAMETERS:  	void
 *
 *  RETURNS: void
 *
 *  CALLED BY: 
 *		Native Mode: SysLaunch() in ROMSysLaunch.c after initializing the
 *   	 Memory Manager.
 *
 *		Emulated Mode: SysReset() after initializing the Memory Manager
 *
 *  CREATED: 4/28/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
void EmThreadsInit(void);
void SysInit(void)
{
	
	// do 1st stage of battery manager init here. We used to set up some battery threshold
	// globals here, but this has been moved to the battery manager function since many
	// of these values are hardware version dependent and the battery manager "knows" these
	// dependencies -soe- 10/11/98
	
	// Moved battery manager init to ROMSysLaunch, so it happens after the traps are installed.
	// PowerMgr needs its traps. --srj 3/3/99
	
	// The pointer to the array of hard key to application creator types is
	// no longer used. The system preferences now store the creator types for
	//  which app to launch when a key is pressed.
	//GSysHardKeyCreators = HardKeyCreators();
	
	// Under emulation,  we must initialize our multi-tasking emulator
	#if EMULATION_LEVEL != EMULATION_NONE
	EmThreadsInit();
	#endif
	
	// create the initial system library table
	GSysLibTableP = MemPtrNew(sysDefaultLibraries * sizeof(SysLibTblEntryType));
	ErrFatalDisplayIf(!GSysLibTableP, "Out of memory");
	MemSet(GSysLibTableP, sysDefaultLibraries * sizeof(SysLibTblEntryType), 0);
	GSysLibTableEntries = sysDefaultLibraries;

}





/************************************************************
 *
 *  FUNCTION: SysReset
 *
 *  DESCRIPTION: Re-initializes Pilot globals and dynamic
 *		memory heap. 
 *
 *		Under Windows emulation mode, this routine
 *		looks for two data files that represent the memory of card
 *		1 and card 2. If these are found, the Pilot's memory
 *		image is created using them. If they are not found, they
 *		will be created.
 *
 *		Under Native mode, this routine simply looks for the memory
 *		cards at fixed locations.
 *
 *  PARAMETERS: 
 *
 *  RETURNS: void
 *
 *  CREATED: 10/26/94 
 *
 *  BY: Ron Marianetti
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	7/3/95	Added setup for Serial Link Manager globals
 *			ryw	5/15/00	Added HwrSoundOff call
 *
 *************************************************************/
void SysReset(void)
{
#if EMULATION_LEVEL != EMULATION_NONE
	LowMemType*		lowMemP;
	UInt16 *			wP;
	UInt16				i;
	CardInfoType	cardInfo[hwrNumCardSlots];
	UInt8				memCardSlots;
	
	
	// Save the memory card info
	memCardSlots = GMemCardSlots;
	for (i=0; i<hwrNumCardSlots; i++)
		cardInfo[i] = *(memCardInfoP(i));

	// Make sure our globals take an even number of bytes to ensure that
	// the following loop clears all the bytes, including the last one.
	if (sizeof(FixedGlobalsType) % 2 != 0)
		ThisShouldGenerateALinkError();
	
	// Clear all the low memory globals
	lowMemP = (LowMemType*)PilotGlobalsP;
	wP = (UInt16 *)(&lowMemP->fixed.globals);
	for (i=0; i< sizeof(FixedGlobalsType)/2; i++)
		*wP++ = 0;


	// Restore the memory card info
	GMemCardSlots = memCardSlots;
	GMemCardInfoP = (MemPtr)&GMemCardInfo[0];
	for (i=0; i<hwrNumCardSlots; i++)
		((CardInfoPtr)GMemCardInfoP)[i] = cardInfo[i];

	// Init System AppInfo
	GSysAppInfoP = (MemPtr)&GSysAppInfo;
	GCurUIAppInfoP = (MemPtr)&GSysAppInfo;
	
	// Set up Serial Link Globals pointer
	GSlkGlobalsP = (MemPtr)&GSlkGlobals;

	// Init the Hardware Manager
	HwrInit();

	// Init the Memory Manager. This initializes the Pilot globals
	//  and creates the global heap info table for each card.
	MemInit();
	
	// Init the Data Manager
	DmInit();
	
	// Init System Manager
	SysInit();
	
	// Init the Notification Manager
	// <chg 8-11-98 RM> Moved here from SysUILaunch() because Notify Mgr needs to be
	//  initialized before SysUILaunch() is called in order to setup system resources 
	//  from a saved memory card. 
	SysNotifyInit();
	
#else 
	ResetHandler*		resetP;
	CardHeaderPtr		cardP;
	CardInfoPtr			cardInfoP;
	
	// turn off sound so that when system sound is on we don't beep during the 
	// whole reset
	HwrSoundOff();
	
	// Turn off interrupts before starting (First added in 3.1, merged into 3.3)
	SysDisableInts();
	
	// Fetch the address of the reset vector out of the card header
	cardInfoP = memCardInfoP(0);
	cardP = (CardHeaderPtr)(cardInfoP->baseP + cardInfoP->cardHeaderOffset);
	
	// The soft reset entry point is the same one used for reset exceptions
	resetP = (ResetHandler*)(cardP->resetVector);
	
	// On the Pre328 board, the reset vector points to the area near 0
	//  where ROM appears right after reset, so we need to add the 
	//  current location of the ROM to that.
	if (((UInt32)resetP & 0xFFFF0000) == 0) 
		resetP = (ResetHandler*)((UInt32)resetP + cardInfoP->cardHeaderOffset);
	
	// Call it...
	(*resetP)();
#endif
}


/************************************************************
 *
 *  FUNCTION: SysPowerOn
 *
 *  DESCRIPTION: Powers up Pilot in the state it was left
 *		in from last power down. 
 *
 *		Under Emulation mode, this routine operates the same as
 *		SysReset because we can't restore the dynamic heap and globals
 *		since they contain pointers and our globals end up at a different
 *		location in memory every time we launch the emulation application.
 *
 *		In Native mode, this routine uses the dynamic heap and globals
 *		from the last Power-Down.
 *
 *  PARAMETERS: 
 *
 *  RETURNS:
 *
 *  CREATED: 10/26/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
void 	SysPowerOn(void *card0P, UInt32 card0Size, 
					  void *card1P, UInt32 card1Size,
					  UInt32 sysCardHeaderOffset,
					  Boolean reFormat)
{

#if EMULATION_LEVEL != EMULATION_NONE
	CardInfoPtr				infoP;
	StorageHeaderPtr		storeP;
	Err						err;
	UInt16						i;
	UInt8 *					p;
	Boolean					reFormat0, reFormat1;
	LowMemType*				lowMemP;
	UInt32						size;
	
	
	// Init reFormat flags
	reFormat0 = reFormat1 = reFormat;
	
	// Setup pointer to Pilot globals
	PilotGlobalsP = card0P;
	
	
	// Init the globals to 0
	lowMemP = (LowMemType*)PilotGlobalsP;
	p = (UInt8 *)&(lowMemP->fixed.globals);
	size = sizeof(LowMemType) - ((UInt32)p - (UInt32)lowMemP);
	for (i=0; i<size; i++)
		*p++ = 0;
	

	// Set up pointer to cardInfo array
	GMemCardInfoP = (MemPtr)(&GMemCardInfo[0]);
	GMemCardSlots = hwrNumCardSlots;
	

	//-------------------------------------------------------------------------
	// Save card 0 info into emulation globals
	//-------------------------------------------------------------------------
	infoP = memCardInfoP(0);
	infoP->baseP = (UInt8 *)card0P;
	infoP->size = card0Size;
	
	if (sysCardHeaderOffset < card0Size) {
		infoP->firstRAMBlockSize = sysCardHeaderOffset;
		infoP->cardHeaderOffset = sysCardHeaderOffset;
		}
	else {
		infoP->firstRAMBlockSize = card0Size;
		infoP->cardHeaderOffset = sysRAMOnlyCardHeaderOffset;
		}
		
	
	// Search for the RAM store on this card
	err = MemStoreSearch(infoP->baseP, infoP->firstRAMBlockSize, &storeP);
	if (!err) {
		infoP->rsvSpace = storeP->rsvSpace;
		infoP->dynHeapSpace = storeP->dynHeapSpace;
		infoP->ramStoreP = storeP;
		}
		
	// If the RAM store header was not found, fill in default sizes of the
	//  dynamic heap and low memory area and force a re-format.
	else {
		reFormat0 = true;
		infoP->rsvSpace = sysLowMemSize;
		infoP->dynHeapSpace = HwrCalcDynamicHeapSize(infoP->firstRAMBlockSize);
		infoP->ramStoreP = (StorageHeaderPtr)(infoP->baseP + infoP->rsvSpace 			
												+ infoP->dynHeapSpace);
		}


	//-------------------------------------------------------------------------
	// Save card 1 info into emulation globals
	//-------------------------------------------------------------------------
	if (GMemCardSlots > 1) {
		infoP = memCardInfoP(1);
		infoP->baseP = (UInt8 *)card1P;
		infoP->size = card1Size;
		
		if (sysCardHeaderOffset < card1Size) {
			infoP->firstRAMBlockSize = sysCardHeaderOffset;
			infoP->cardHeaderOffset = sysCardHeaderOffset;
			}
		else {
			infoP->firstRAMBlockSize = card1Size;
			infoP->cardHeaderOffset = sysRAMOnlyCardHeaderOffset;
			}
		
		// Search for the RAM store on this card
		err = MemStoreSearch(infoP->baseP, infoP->firstRAMBlockSize, &storeP);
		if (!err) {
			infoP->rsvSpace = storeP->rsvSpace;
			infoP->dynHeapSpace = storeP->dynHeapSpace;
			infoP->ramStoreP = storeP;
			}
			
		// If the RAM store header was not found, fill in default sizes of the
		//  dynamic heap and low memory area and force a re-format.
		else {
			reFormat1 = true;
			infoP->rsvSpace = sysCardHeaderSize;
			infoP->dynHeapSpace = 0;
			infoP->ramStoreP = (StorageHeaderPtr)(infoP->baseP + infoP->rsvSpace 
												+ infoP->dynHeapSpace);
			}
	
		}


	// Format the cards if desired
	if (reFormat0) {
		MemCardFormat(0, "Card 0", "Palm Computing", "RAM Store 0");
		if (GMemCardSlots > 1 && reFormat1) 
			MemCardFormat(1, "Card 1", "Palm Computing", "RAM Store 1");
		}

	// Reset the system
	SysReset();
	
#endif

}


/************************************************************
 *
 *  FUNCTION: SysUILaunch
 *
 *  DESCRIPTION: Called during startup sequence in order to
 *   install and initialize the UI Managers. This routine
 *	 is called from the AMX restart procedure called "PilotRestart"
 *	 after AMX has been installed.
 *
 *  PARAMETERS: void
 *
 *  RETURNS: void
 *
 *	HISTORY:
 *		03/13/95	ron	Created by Ron Marianetti
 *		09/03/96	vmk	Install net lib in emulation mode
 *		03/11/97	scl	Do FtrInit before PrefGetPreference in emulation mode
 *		08/19/97	gmp	Add ExgInit and Serial library load for emulation
 *		12/12/97	rsf	Change to use SysCurAppInfo
 *		05/27/98	kwk	Call IntlInit().
 *		05/28/98	jhl	Added FlashInit()
 *		05/18/00	kwk	Call OmInit() vs. FtrInit().
 *		05/23/00	kwk	Call IntlInit() sooner, since the Text Mgr needs
 *							to be alive before anybody calls StrCompare or
 *							other String/Text routines.
 *		07/04/00	kwk	Call GrfInit() after UIInitialize(), since TsmInit()
 *							has to be called before Graffiti (currently).
 *		11/20/00 scl	Added call to HwrModelInitStage3() to device build.
 *
 *************************************************************/
typedef void (*InstallProcPtr)(MemPtr dispatchTableP);

Err SrmInstallForSimulator();
Err NetPrvInstallDispatcher(UInt16 refNum, SysLibTblEntryPtr entryP);
Err RMPLibPrvInstallDispatcher(UInt16 refNum, SysLibTblEntryPtr entryP);
Err INetLibPrvInstallDispatcher(UInt16 refNum, SysLibTblEntryPtr entryP);
Err WebLibPrvInstallDispatcher(UInt16 refNum, SysLibTblEntryPtr entryP);
Err SecLibPrvInstallDispatcher(UInt16 refNum, SysLibTblEntryPtr entryP);
Err PdiPrvInstallDispatcher(UInt16 refNum, SysLibTblEntryPtr entryP);

void 	SysUILaunch()
{
#if (EMULATION_LEVEL != EMULATION_NONE)
	SysAppInfoPtr			appInfoP;
	SysLibEntryProcPtr	libP=0;
	UInt16					refNum;

	// Init the Overlay Mgr, which in turn initializes the Feature Manager
	OmInit();
	
	// Initialize the int'l manager. This has to happen soon, since anybody
	// calling text or string routines will need to have the Text Mgr alive.
	IntlInit();

	// Init the Notification Manager
	// <chg 8-11-98 RM> Moved into SysReset() because Notify Mgr needs to be initialized
	//  before SysUILaunch() is called in order to setup system resources from a
	//  saved memory card. 
	//SysNotifyInit();
	
	// Init the GSysPrefFlags and GSysBatteryKind globals
	//  from the preferences.
	GSysPrefFlags = (UInt16) PrefGetPreference(prefSysPrefFlags);
	GSysBatteryKind = (SysBatteryKind) PrefGetPreference(prefSysBatteryKind);

	// Initialize the screen driver
	WinScreenInit();

	// Intialize the low level event manager
	EvtSysInit();
	
	// Initialize the UI
	UIInitialize ();

	// Initialize Graffiti
	GrfInit();

	// <RM> 1-16-98,  Mark it as initialized so that ErrDisplay() knows it's
	//  safe to display error alerts
	GSysMiscFlags |= sysMiscFlagUIInitialized;
	
	// Initialize Sound Manager
	SndInit();
	
	// Initialize Time Manager
	TimInit();
	
	// Initialize Alarm Manager (MUST BE AFTER TIME MANAGER INITIALIZATION)
	AlmInit();
	
	// Intialize the key manager
	KeyInit();

	// Initialize the connection manager
	PrvCncMgrInit();
	
	// Setup run-time features. 
	if (GHwrMiscFlags & hwrMiscFlagHasBacklight) 
		FtrSet(sysFtrCreator, sysFtrNumBacklight, 0x01);
		
	// Setup run-time features. 
	if (INCLUDE_DES == INCLUDE_DES_ON) 
		FtrSet(sysFtrCreator, sysFtrNumEncryption, 0x01);
	else
		FtrSet(sysFtrCreator, sysFtrNumEncryption, 0x00);
		
	// Setup hardware features. 
	FtrSet(sysFtrCreator, sysFtrNumHwrMiscFlags, GHwrMiscFlags);
	FtrSet(sysFtrCreator, sysFtrNumHwrMiscFlagsExt, GHwrMiscFlagsExt);
	
	// Set NotifyMgr feature:
	FtrSet(sysFtrCreator, sysFtrNumNotifyMgrVersion, sysNotifyVersionNum);
	
	// Setup debug/release feature (for Palm API test tools)
	FtrSet(sysFtrCreator, sysFtrNumErrorCheckLevel, ERROR_CHECK_LEVEL);
		
	// Setup OEM/Device/HAL identification info
	FtrSet(sysFtrCreator, sysFtrNumOEMCompanyID, GHwrOEMCompanyID);
	FtrSet(sysFtrCreator, sysFtrNumOEMDeviceID, GHwrOEMDeviceID);
	FtrSet(sysFtrCreator, sysFtrNumOEMHALID, GHwrOEMHALID);
		
	//-------------------------------------------------------------------------
	// Install the Serial Library. 
	//-------------------------------------------------------------------------
	SrmInstallForSimulator();

	// Initialize the Exg manager (Must be after serial library is installed)
	ExgInit();  
	
	// Initialize the simulator version of the VFS Mgr.
	// Note: In a real ROM, this is loaded & initialized via a system extension.
	
	ExpInit();
	VFSInit();
	
	//-------------------------------------------------------------------------
	// Install the Net Library. The NetPrvInstallDispatcher() routine
	//  is smart enough not to install on the "Personal" version of the ROM. 
	//-------------------------------------------------------------------------
	libP = NetPrvInstallDispatcher;
	SysLibInstall(libP, &refNum);
	
	// Install the Reliable Message Protocol plug-in for the Net Library.
	// The RMPLibPrvInstallDispatcher is smart enough not to install unless
	//  the NetLib is installed.
	libP = RMPLibPrvInstallDispatcher;
	SysLibInstall(libP, &refNum);

	// Install the Internet Library. 
	// The InetLibPrvInstallDispatcher is smart enough not to install unless
	//  the NetLib is installed.
	libP = INetLibPrvInstallDispatcher;
	SysLibInstall(libP, &refNum);
	
	// Install the Web Library. 
	libP = WebLibPrvInstallDispatcher;
	SysLibInstall(libP, &refNum);
	
	// Install the Security Library. 
	libP = SecLibPrvInstallDispatcher;
	SysLibInstall(libP, &refNum);
	
	// Install the PDI Library. 
	libP = PdiPrvInstallDispatcher;
	SysLibInstall(libP, &refNum);
	
	// Allocate a fake chunk to simulate the memory used by the AMX kernel
	// Also add to this miscellaneous data used on the real hardware
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
	
	// Allocate a fake chunk to simulate the UIShell app info block and stack
	GSysUIShellAppInfoP = MemPtrNew(sizeof(SysAppInfoType));
	appInfoP = (SysAppInfoPtr)GSysUIShellAppInfoP;
	MemSet(appInfoP, sizeof(SysAppInfoType), 0);
	appInfoP->stackP = MemPtrNew(uiAppStackSize);
	appInfoP->stackEndP = (appInfoP->stackP + uiAppStackSize);
	appInfoP->globalsChunkP = MemPtrNew(0x38);

	// Create an appInfo structure for the "one" app.
	GCurUIAppInfoP = MemPtrNew(sizeof(SysAppInfoType));
	MemSet(GCurUIAppInfoP, sizeof(SysAppInfoType), 0);
	((SysAppInfoPtr)GCurUIAppInfoP)->memOwnerID = SysNewOwnerID();

	// Put in pre-opened system databases into the DmAccess List
	((SysAppInfoPtr)GCurUIAppInfoP)->dmAccessP = (MemPtr)((SysAppInfoPtr)GSysAppInfoP)->dmAccessP;
	
	// Disable Graffiti if this is a DEMO ROM
	#if USER_MODE == USER_MODE_DEMO
	GSysMiscFlags |= sysMiscFlagGrfDisable;
	#endif
	
	// Under simulator, broadcast ResetFinished notification now...
	{
		SysNotifyParamType notify;		// for broadcast of reset finished notification
		
		notify.broadcaster = sysNotifyBroadcasterCode;
		notify.notifyType = sysNotifyResetFinishedEvent;
		notify.notifyDetailsP = NULL;
		
		SysNotifyBroadcast(&notify);
	}	
	
#else

	// Device version of SysUILaunch()...
	
	MemHandle						codeH;
	InstallProcPtr				codeP;
	UInt16						cardNo;
	LocalID						dbID;
	Err							err;
	UInt32						result;
	UInt16						rid;
	DmSearchStateType			searchState;
	SysLibEntryProcPtr		libP=0;
	UInt16						refNum;
	
	//-------------------------------------------------------------------------
	// Get the UI code resources and install them
	//-------------------------------------------------------------------------
	for (rid = sysResIDBootUICodeStart; 1; rid++) {
		codeH = DmGetResource(sysResTBootCode, rid);
		if (!codeH)  break;
		
		codeP = (InstallProcPtr)MemHandleLock(codeH);
		(*codeP)((MemPtr)GSysDispatchTableP);
	}
	
	// Make sure we got the minimum set
	if (rid <= sysResIDBootUICodeMin) DbgBreak();
	
	

	//-------------------------------------------------------------------------
	// Init the Higher level managers
	//-------------------------------------------------------------------------
	
	// Initialize the int'l manager. This has to happen soon, since anybody
	// calling text or string routines will need to have the Text Mgr alive.
	IntlInit();
		
	// Init the GSysPrefFlags from the preferences
	GSysPrefFlags = PrefGetPreference(prefSysPrefFlags);

	// Init the Notification Manager
	SysNotifyInit();
	
	// Initialize flash manager
	FlashInit();
		
	// Initialize the screen driver
	WinScreenInit();
	
	// Initialize the System Event Manager
	EvtSysInit();
	
	//Initialize the Keyboard Manager
	KeyInit();						 
	
	// Initialize the connection manager
	PrvCncMgrInit();
	
	// Initialize Sound Manager
	SndInit();
	
	// Setup run-time features. 
	if (GHwrMiscFlags & hwrMiscFlagHasBacklight) 
		FtrSet(sysFtrCreator, sysFtrNumBacklight, 0x01);
		
	// Setup run-time features. 
	if (INCLUDE_DES == INCLUDE_DES_ON) 
		FtrSet(sysFtrCreator, sysFtrNumEncryption, 0x01);
	else
		FtrSet(sysFtrCreator, sysFtrNumEncryption, 0x00);
		
	// If in poser, take out the HasJerryHW flag
	if(HostGetHostID() == hostIDPalmOSEmulator)
		GHwrMiscFlags &= ~hwrMiscFlagHasJerryHW;

	// Setup hardware features. 
	FtrSet(sysFtrCreator, sysFtrNumHwrMiscFlags, GHwrMiscFlags);
	FtrSet(sysFtrCreator, sysFtrNumHwrMiscFlagsExt, GHwrMiscFlagsExt);
	
	// Set NotifyMgr feature:
	FtrSet(sysFtrCreator, sysFtrNumNotifyMgrVersion, sysNotifyVersionNum);
	
	// Setup debug/release feature (for Palm API test tools)
	FtrSet(sysFtrCreator, sysFtrNumErrorCheckLevel, ERROR_CHECK_LEVEL);
		
	// Setup OEM/Device/HAL identification info
	FtrSet(sysFtrCreator, sysFtrNumOEMCompanyID, GHwrOEMCompanyID);
	FtrSet(sysFtrCreator, sysFtrNumOEMDeviceID, GHwrOEMDeviceID);
	FtrSet(sysFtrCreator, sysFtrNumOEMHALID, GHwrOEMHALID);
		
	// Initialize the Exg manager
	if (! (GSysResetFlags & sysResetFlagNoExtensions)) 
		ExgInit();  
	
	//-------------------------------------------------------------------------
	// Install the Net Library, if found and extensions are not disabled.
	//-------------------------------------------------------------------------
	if (! (GSysResetFlags & sysResetFlagNoExtensions)) {
		SysLibLoad(sysFileTLibrary, sysFileCNet, &refNum);
		}

	//-------------------------------------------------------------------------
	// Install the Internet Library, if found and extensions are not disabled.
	//-------------------------------------------------------------------------
	if (! (GSysResetFlags & sysResetFlagNoExtensions)) {
		SysLibLoad(sysFileTLibrary, sysFileCINetLib, &refNum);
		}

	//-------------------------------------------------------------------------
	// Install the Security Library, if found and extensions are not disabled.
	//-------------------------------------------------------------------------
	if (! (GSysResetFlags & sysResetFlagNoExtensions)) {
		SysLibLoad(sysFileTLibrary, sysFileCSecLib, &refNum);		
		}

	//-------------------------------------------------------------------------
	// Install the Web Library, if found and extensions are not disabled.
	//-------------------------------------------------------------------------
	if (! (GSysResetFlags & sysResetFlagNoExtensions)) {
		SysLibLoad(sysFileTLibrary, sysFileCWebLib, &refNum);		
		}

	//-------------------------------------------------------------------------
	// Install the RMP Library, if found and extensions are not disabled.
	//-------------------------------------------------------------------------
	if (! (GSysResetFlags & sysResetFlagNoExtensions)) {
		SysLibLoad(sysFileTLibrary, sysFileCRmpLib, &refNum);		
		}

	// Initialize the UI
	UIInitialize();
	
	// Initialize Graffiti
	GrfInit();

	// <RM> 1-16-98,  Mark it as initialized so that ErrDisplay() knows it's
	//  safe to display error alerts
	GSysMiscFlags |= sysMiscFlagUIInitialized;
	
	// Disable Graffiti if this is a DEMO ROM
	#if USER_MODE == USER_MODE_DEMO
	GSysMiscFlags |= sysMiscFlagGrfDisable;
	#endif
	
	
	//-------------------------------------------------------------------------
	// Do the 2nd stage of init for the HwrBattery Manager. This is initialized quite late
	// in the boot sequence because it relies on alarms. 
	//-------------------------------------------------------------------------
	HwrBattery(hwrBatteryCmdInitStage2, 0);
	

	//-------------------------------------------------------------------------
	// Give the HAL one last chance to do stuff before launching UIAppShell.
	// In particular, now that all managers have been initialized, the HAL
	// can do things like register handlers with the NotificationMgr.
	//-------------------------------------------------------------------------
	HwrModelInitStage3();
	

	//-------------------------------------------------------------------------
	// Launch The UI Application Shell. This "application" gets launched as
	//  a separate task and acts as the shell for the current UI app. UI apps
	//  are in turn launched one at a time as subroutines from the UIAppShell.
	//-------------------------------------------------------------------------
	DmGetNextDatabaseByTypeCreator(true, &searchState, sysFileTUIAppShell, sysFileCSystem, true,
			&cardNo, &dbID);
	ErrFatalDisplayIf(!dbID, "Could not find UIAppShell");
	
	//-------------------------------------------------------------------------
	// Up 'till now we've been running out of the (1K) kernel stack, and sometimes
	// people modify the boot code to use more stack space than it used to.
	// When this happens, the kernel stack overflows into the interrupt stack
	// and this leads to hard to reproduce problems during booting -- they happen
	// only if an interrupt occurs at the right time.  This debug check verifies
	// that the stack guard bytes between the kernel stack and interrupt
	// stack are intact.  If they are not, there are three possible causes:
	//    1) we're not using the AMX kernel, and the XXXXXXXXXXXXXXX struct
	//		   is in a different format.
	//		2) the XXXXXXXXXXXXXXX struct format has changed since this code
	// 		was written (note offset 0x300 is hardcoded because kernel
	//			globals is private and messy data structure)
	//		3) the boot code overflowed the stack.
	//
	//	It's very likely case 3.  To verify, connect PalmDebugger, do a 'hd 0'
	// and find the kernel globals, then do a 'dm' on that chunk and look
	// for sets of '55 55 55 55' values.  There should be three: one at the
	// top of the interrupt stack, one between the kernel and interrupt
	// stacks, and one at the bottom of the kernel stack.  If the one in the
	// middle is missing, the overflow occurred.
	//-------------------------------------------------------------------------

// <11/23/99 SCL> This test breaks all Debug ROMs any time someone changes the
// kernel parameters (as I just did).  If someone links in another kernel binary
// when they build the ROM, they'll be in trouble.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
//								"Kernel stack overflowed during boot.");
//
//
// Here is a potentially better way to do it, but it needs to be flushed out...
//
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
//	if (err) {
//		ErrNonFatalDisplay("Error getting kernel task info");
//		}
//	else {
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
//		}
//
// Finally, here's another (w)hacky way to do this, which is specific to the
// AMX kernel, but works better than just assuming where the end of the stack is.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// Fetch the pointer and check THERE.  Thus if we change the kernel parameters,
// the value of this pointer will be updated, but will not actually move itself.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
								"Kernel stack overflowed during boot.");
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.

	
	// Now go launch the UI Application Shell.	
	if (dbID) {
		err = SysAppLaunch(cardNo, dbID, 
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
					sysAppLaunchFlagNewStack | 
					sysAppLaunchFlagNewGlobals,
					0, 0, &result);
		ErrFatalDisplayIf(err, "Could not launch UIAppShell");
		}
#endif
}





/************************************************************
 *
 *  FUNCTION: SysDoze
 *
 *  DESCRIPTION:  Puts the processor to sleep but keeps system clock
 *		running and all peripherals awake
 *
 *  PARAMETERS:   
 *		onlyNMI  - if set, only an NMI can wake up the CPU
 *
 *  RETURNS: void
 *
 *  CREATED: 5/25/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
void SysDoze(Boolean onlyNMI)
{
	HwrDoze(onlyNMI);
}


/************************************************************
 *
 *  FUNCTION: SysSleep
 *
 *  DESCRIPTION:  Puts the system into lowest power mode by
 *			shutting down all peripherals, CPU, and system clock
 *
 *  PARAMETERS: 
 *			untilReset - if true, system won't come out of sleep unless system
 *								is reset.
 *		emergency - if true, system will shut down faster than normal. Mainly,
 *							any key debouncing is bypassed.
 *
 *  RETURNS: void
 *
 *  CREATED: 5/25/95
 *
 *  BY: Ron Marianetti
 *	
 *	MODIFIED:
 *		11/27/98	Moved in NetLibPowerOff call from PrvHandleEvent. JB
 *		12/8/98	Added return code to AlmDisplayAlarm so that if
 *					an appl. alarm needs to be processed, it will.  This
 *					is necessary because the Mobitex stack will enable alarms
 *					on exit.  If we ignore them on the way to sleep, we
 *					won't see them until the device is turned back on. JB
 *
 *		12/8/98	If gotUserEvent is already false, don't bother with
 *					NetLib. JB
 *
 *************************************************************/
void SysSleep(Boolean untilReset, Boolean emergency)
{
	SysEvtMgrGlobalsPtr	sysEvtMgrP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
	SysEventType event;	// dummy for the call to NetLibHandlePowerOff

	//--------------------------------------------------------------
	// <chg 4-1-98 RM> 
	// Clear the gotUserEvent flag in System Event Manager. It stays
	//  clear until we wakeup and get a user event. See the
	//  comments in EvtResetAutoOffTimer(). 
	//
	// Also, reset the auto-off timer so that our timer task doesn't
	//  try and put us right back to sleep after we wake. If no one
	//  calls EvtResetAutoOffTimer() before the next cycle through
	//  the event loop after waking, then EvtGetSysEvent() will put
	//  us back to sleep again.
	//--------------------------------------------------------------
	GSysAutoOffEvtTicks = GHwrCurTicks;
	if ( sysEvtMgrP->gotUserEvent == false )
		goto sleep;
	
	sysEvtMgrP->gotUserEvent = false;

	// If the NetLib is installed and up, give it a chance to close
	// the current connection. The NetLibHandlePowerOff call returns
	// non-zero if the power off should be ignored.		
	
	if ( emergency == false )
	{
		MemSet ( &event, sizeof (SysEventType), 0 );
		if (GSysLibNet) 
			NetLibHandlePowerOff(GSysLibNet, &event);
			
		if ( AlmDisplayAlarm ( false ) )
		{	
		  	EvtResetAutoOffTimer ();
		  	return;
		}
	}

sleep:	
	HwrSleep(untilReset, emergency);
}


/************************************************************
 *
 *  FUNCTION: SysGetROMToken
 *
 *  DESCRIPTION:	This routine is used to locate a "ROM token" from
 *						the list burned into ROM by the manufacturer of the
 *						device.
 *
 *  PARAMETERS:	cardNo	memory card number
 *						token		4-byte ROM token to search for
 *						dataP		pointer to a container which will receive the
 *									address of the token's data, or NULL if this
 *									info is not needed
 *						sizeP		pointer to a container which will receive the
 *									size (in bytes) of the token's data, or NULL
 *									if this info is not needed
 *
 *  RETURNS:		NULL if successful, "cheesy" error code otherwise
 *						*dataP	address of token's data
 *						*sizeP	size (in bytes) of the token's data
 *
 *  CREATED:		2/5/98
 *
 *  BY:				Steve Lemke
 *
 *************************************************************/
#if EMULATION_LEVEL != EMULATION_NONE
// This "wrapper" routine calls HwrGetROMToken (in HardwareMgrEmu.c),
// but ONLY in Simulator builds. In a device build, the function
// prototype for SysGetROMToken (in <SystemMgr.h>) refers directly
// to the HwrGetROMToken trap word...
Err SysGetROMToken (UInt16 cardNo, UInt32 token,	UInt8 ** dataP,
						UInt16 * sizeP)
{
	return( HwrGetROMToken (cardNo, token, dataP, sizeP) );
}
#endif


/************************************************************
 *
 *  FUNCTION: SysSetPerformance
 *
 *  DESCRIPTION:  Sets various performance options for the
 *   system.
 *
 *  PARAMETERS: 
 *			*sysClockP - On Entrance: desired system clock frequency.
 *											 or 0 to not change it.
 *							 On Exit: actual system clock.
 *			*cpuDutyP  - On Entrance: desired CPU duty cycle, 
 *												or 0 to not change it.
 *							 On Exit, current duty cycle value.
 *
 *      NOTE: the sysClockP and cpuDutyP pointers must NEVER BE NIL!
 *					in order to just retrive the current value of either
 *					one, pass a pointer to a 0 value, not a nil pointer.						
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 5/25/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
#if EMULATION_LEVEL == EMULATION_NONE
Err SysSetPerformance(UInt32 * sysClockP, UInt16 * cpuDutyP)
#else
Err SysSetPerformance(UInt32 *, UInt16 *)
#endif
{

#if EMULATION_LEVEL == EMULATION_NONE
	UInt16				status;
	Boolean			clockChange = false;
	UInt16				openSerial = 0;
	
	// First disable interrupts
	status = SysDisableInts();
	
	// Set boolean if we're changing the system clock and shut down
	//  peripherals that might be affected by it
	if (*sysClockP) {
		clockChange = true;
		
	SrmSleep();

		// We never shut down timer 1 because it's needed for refreshes.	
		//HwrTimerShutdown(1);
		HwrTimerSleep(2);
		HwrDisplaySleep(false, false);
		}
	
	// Set the system clock
	HwrSetSystemClock(sysClockP);
	
	
	// If the system clock changed, we must re-init the peripherals
	if (clockChange) {
	
		// Re-init the timers
		HwrTimerInit(1);
		HwrTimerInit(2);

		// Re-init the LCD
		HwrDisplayInit(0 /*hwrDisplayPeriod*/, 0, 0, 0, 0);   
		}
		
	// Set the CPU duty cycle
	HwrSetCPUDutyCycle(cpuDutyP);
		

	// Restore interrupts
	SysRestoreStatus(status);
	
	SrmWake();			// For 68328 Drvr, baud rate clock will automatically reset if
							// GSysClockFreq changes.
		
#endif

	return 0;
}



/************************************************************
 *
 *  FUNCTION: SysSetAutoOffTime
 *
 *  DESCRIPTION: This routine sets the timeout value in seconds for
 *					auto power-off. 0 Means never power off.
 *
 *  PARAMETERS: 
 *		seconds	- timeout in seconds, or 0 for no timeout
 *
 *  RETURNS: previous value of timeout in seconds.
 *
 *  CREATED: 8/21/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt16 		SysSetAutoOffTime(UInt16	seconds)
{	
	UInt16						oldValue;
	
	// Get the old value
	oldValue = GSysAutoOffSeconds;
	
	// Save new value low memory global
	GSysAutoOffSeconds = seconds; 
		
		
	// If we're Debugging, don't enable auto-off
	#if DEBUG_LEVEL == DEBUG_LEVEL_FULL
	GSysAutoOffSeconds = 0;
	#endif

	return oldValue;
}


/************************************************************
 *
 *  FUNCTION: SysTicksPerSecond
 *
 *  DESCRIPTION: This routine returns the number of ticks per
 *		second. It is provided so that apps can be tolerant of
 *		changes to the ticks per second rate in the system. 
 *
 *  PARAMETERS: 
 *		void
 *
 *  RETURNS: # of ticks per second.
 *
 *  CREATED: 7/3/96
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt16 		SysTicksPerSecond(void)
{	
	return sysTicksPerSecond;
}




/************************************************************
 *
 *  FUNCTION: SysBatteryInfo
 *
 *  DESCRIPTION: This routine can be used to retrieve or change settings for
 *		the Batteries. If the parameter 'set' is true, any one or more of the battery
 *		settings can be changed. If 'set' is false, any one or more of the battery
 *		settings can be retrieved.
 *
 *		'warnThresholdP' and 'maxTicksP' are the battery warning voltage threshold and
 *		timeout. If the battery voltage falls below the threshold or, if the timeout
 *		expires, a lowBatteryChr key event will be enqueued. Normally, apps call
 *		SysHandleEvent which will put call SysBatteryWarningDialog in response to
 *		this event. 
 *
 *		'criticalThresholdP' is the critical battery voltage threshold. If the battery
 *		voltage falls below this level, the System will turn off without warning and will
 *		not turn on.
 *
 *		The battery voltage is equal to:
 *				
 *				voltage = (level + hwrVoltStepsOffset) 
 *							  --------------------------
 *							      hwrStepsPerVolt
 *
 *  PARAMETERS: 
 *		set 				-  if true, parameters with non-nil pointers will be changed except
 *									for pluggedIn, which can only be queried.
 *			   				   if false, parameters will non-nil pointers will be retrieved
 *		warnThresholdP 		-  Pointer to battery voltage warning threshold in volts*100, or nil.
 *		criticalThresholdP 	-  Pointer to battery voltage critical threshold in volts*100, or nil.
 *		maxTicksP			-  Pointer to the battery timeout, or nil.
 *		kindP				-  Pointer to the battery kind, or nil.
 *		plugggedInP			-  Pointer to pluggedIn return value, or nil.
 *		percentP			-  Pointer to percent return value, or nil.  Cannot be set.
 *
 *
 *  RETURNS: the current battery voltage in volts*100.
 *
 *  CREATED: 10/23/95
 *
 *  BY: Ron Marianetti
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	11/13/97	Added percentP
 *			JB		5/12/98	Added nicad value and boolean for doing percentP when Jerry device
 *			JB		7/14/98	Changed percentP to use global.  Moved PrvBatteryPercent() to ROM
 *								HiHardwareTD1.c
 *      	srj   	4/1/98 	Changed to use battery table values.
 *								Warning! changing the battery KIND now
 *                     			changes ALL the threshold values back to their defaults.
 *			srj		8/15/98	Changed to set global if the critical voltage level is 0; this
 *								is the signal to use time-based critical warnings.
 *
 *************************************************************/
UInt16	SysBatteryInfo(Boolean set, UInt16 * warnThresholdP, UInt16 * criticalThresholdP,
			Int16 * maxTicksP, SysBatteryKind* kindP, Boolean* pluggedInP, UInt8 * percentP)
{	
	UInt16 batteryLevel = 0;
	SysBatteryDataStructP BTableP;

	// Fetch the pointer to the battery table
	BTableP = (SysBatteryDataStructP) GSysBatteryDataP;
	
	// don't do this under simulator... sometimes gremlins will enqueue a 
	// lowBatteryChr before we've initialized this stuff and then, this check fails.
#if EMULATION_LEVEL == EMULATION_NONE
	// Ensure that we aren't called before we've initialized the battery data
	// (except when we DO initialize it, of course)
	ErrFatalDisplayIf(GSysBatteryDataP == 0 && (set==false || kindP == 0), "no battery info");
#endif
	
	// If changing settings...
	if (set) {
		if (warnThresholdP) GSysBatteryWarnThreshold = 
						(((*warnThresholdP)
						 * BTableP->sysBattStepsPerVolt +50)/100) 
						- BTableP->sysBattVoltageStepOffset;
			
		if (criticalThresholdP) GSysBatteryMinThreshold = 
						(((*criticalThresholdP)
						 * BTableP->sysBattStepsPerVolt +50)/100)
						- BTableP->sysBattVoltageStepOffset;
		if (maxTicksP)
			GSysNextBatteryAlertTimer = *maxTicksP;
			
		if (kindP)
		// Warning.  This fratzes all other variables you might have just changed. 
		  	{ 
		  		HwrBatCmdBatteryKindType cmdStruct;
		  		
		  		// Call into PowerManager to set the battery tables up...
		  		cmdStruct.kind = *kindP;
				HwrBattery(hwrBatteryCmdSetBatteryKind, &cmdStruct);

			} // if(kindP)
		} // if(set)
		
	
	
	// Retrieve current settings
	if (warnThresholdP) 
		*warnThresholdP = 	
					  ((GSysBatteryWarnThreshold +
						BTableP->sysBattVoltageStepOffset) * 100 
					  + BTableP->sysBattStepsPerVolt / 2) 
						/ BTableP->sysBattStepsPerVolt;
		
	if (criticalThresholdP) 
		*criticalThresholdP = 				
					  ((GSysBatteryMinThreshold +
						BTableP->sysBattVoltageStepOffset) * 100 
					  + BTableP->sysBattStepsPerVolt / 2) 
						/ BTableP->sysBattStepsPerVolt;
		
	if (maxTicksP)
		*maxTicksP = GSysNextBatteryAlertTimer;
		
	if (kindP)
		*kindP = (SysBatteryKind) GSysBatteryKind;
		
	if (pluggedInP)
		*pluggedInP = HwrPluggedIn();
			
	if (percentP)  
		*percentP = GHwrBatteryPercent;

	// Calculate the stored average battery level
	batteryLevel = 
				(((UInt16)GHwrBatteryLevel + 
				 BTableP->sysBattVoltageStepOffset) * 100 
				  + BTableP->sysBattStepsPerVolt / 2) 
				 / BTableP->sysBattStepsPerVolt;
	
	return batteryLevel;
}


/************************************************************
 *
 *  FUNCTION: SysBatteryInfoV20
 *
 *  DESCRIPTION: This routine can be used to retrieve or change settings for
 *		the Batteries. If the parameter 'set' is true, any one or more of the battery
 *		settings can be changed. If 'set' is false, any one or more of the battery
 *		settings can be retrieved.
 *
 *		'warnThresholdP' and 'maxTicksP' are the battery warning voltage threshold and
 *		timeout. If the battery voltage falls below the threshold or, if the timeout
 *		expires, a lowBatteryChr key event will be enqueued. Normally, apps call
 *		SysHandleEvent which will put call SysBatteryWarningDialog in response to
 *		this event. 
 *
 *		'criticalThresholdP' is the critical battery voltage threshold. If the battery
 *		voltage falls below this level, the System will turn off without warning and will
 *		not turn on.
 *
 *		The battery voltage is equal to:
 *				
 *				voltage = (level + hwrVoltStepsOffset) 
 *							  --------------------------
 *							      hwrStepsPerVolt
 *
 *  PARAMETERS: 
 *		set 				-  if true, parameters with non-nil pointers will be changed except
 *									for pluggedIn, which can only be queried.
 *			   				   if false, parameters will non-nil pointers will be retrieved
 *		warnThresholdP 		-  Pointer to battery voltage warning threshold in volts*100, or nil.
 *		criticalThresholdP 	-  Pointer to battery voltage critical threshold in volts*100, or nil.
 *		maxTicksP			-  Pointer to the battery timeout, or nil.
 *		kindP				-  Pointer to the battery kind, or nil.
 *		plugggedInP			-  Pointer to pluggedIn return value, or nil.
 *
 *
 *  RETURNS: the current battery voltage in volts*100.
 *
 *  CREATED: 10/23/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt16	SysBatteryInfoV20(Boolean set, UInt16 * warnThresholdP, UInt16 * criticalThresholdP,
			Int16 * maxTicksP, SysBatteryKind* kindP, Boolean* pluggedInP)
{
	return 	SysBatteryInfo(set, warnThresholdP, criticalThresholdP,
			maxTicksP, kindP, pluggedInP, NULL);
}


/***********************************************************************
 *
 *  FUNCTION:     SysLCDContrast
 *
 *  DESCRIPTION:  Sets/returns the contrast of the LCD. 
 *
 *						To get the current state, pass false for 'set' and
 *						check the return value.
 *
 *						To change the contrast, pass true for 'set' and
 *						the desired new value in 'newContrastLevel'. The return value
 *						is the old state.
 * 
 *  PARAMETERS:   
 *						set		 		  - if true, contrast will be changed,
 *												 depending on value of newState.
 *												 if false, contrast will not be
 *												 changed.
 *						newContrastLevel - desired new contrast value, if
 *												 'set' is true.
 *
 *  RETURNS:   	  if 'set' is true, the previous contrast level
 *				  if 'set' is false, the current contrast level.
 *
 *  CALLED BY:    Contrast Panel
 *
 *  CREATED:      6/30/98
 *
 *  BY:           	lyl	Modified from Sumo source
 *		7/22/98 	Bob Invert contrast value if Epson screen and backlight on
 *		02/19/99  	lyl	sync with Sumo.
 *						limit range/min/max passed to HwrLCDContrast to avoid 
 *						tech support calls		
 *		7/20/99		ADH	Moved scaling code to inside HwrLCDInit
 *		10/7/99		srj	Rewritten to call HwrDisplayAttributes.
 *						NVParams have been pushed into the HAL.
 *
 ***********************************************************************/
UInt8 SysLCDContrast(Boolean set, UInt8 newContrastLevel)
{
	UInt8 temp;

	// Set the new value if required:
	temp = newContrastLevel;
	
	HwrDisplayAttributes(set, hwrDispContrast, &temp);

	return (temp);
}


/***********************************************************************
 *
 *  FUNCTION:     SysLCDBrightness
 *
 *  DESCRIPTION:  Sets/returns the brightness of the LCD. 
 *
 *						To get the current state, pass false for 'set' and
 *						check the return value.
 *
 *						To change the contrast, pass true for 'set' and
 *						the desired new value in 'newContrastLevel'. The return
 *						value is the old state.
 * 
 *  PARAMETERS:   set						->	true to change
 *						newBrightnessLevel	-> new level for brightness, 0..255
 *
 *  RETURNS:  		if 'set' is true, the previous brightness level
 *				  		if 'set' is false, the current brightness level
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			bob	3/15/99	Cloned from SysLCDContrast
 *			jesse	9/15/99	Save value in NVParams
 *			jesse	9/29/99	Fatal error if we don't support SW brightness.
 *			srj	10/7/99	Rewritten to call HwrDisplayAttributes.
 *								NVParams have been pushed into the HAL.
 *
 ***********************************************************************/

UInt8		SysLCDBrightness(Boolean set, UInt8 newBrightnessLevel)
{
	UInt8 result = newBrightnessLevel;
		
	// Set the new value if required:
	HwrDisplayAttributes(set, hwrDispBrightness, (UInt32*)&result);
	return(result);
}

/************************************************************
 *
 *  FUNCTION: SysLaunchConsole
 *
 *  DESCRIPTION: This routine will launch the Console task as a 
 *   separate kernel task if it's not already launched. It is called
 *	  by PilotRestart() in the AMX launching code if the system
 *	  was booted up into the Debugger. It is also called by 
 *	  SysHandleEvent in order to MemHandle the startConsoleChr keyboard
 *	  event.
 *
 *  PARAMETERS: void
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 8/7/95
 *
 *  BY: Ron Marianetti
 *
 *		vmk	8/1/96	Display UI and abort if serial port already open
 *
 *************************************************************/

// -- private debug option to run the console off hacked up Gordon card.
// The serial library we use for the console depends on a build option
// #if CONSOLE_SERIAL_LIB == CONSOLE_SERIAL_LIB_16C650
// 	#define prvConsoleSerialPort 	'u650'
// #else
	#define prvConsoleSerialPort	serPortConsolePort
// #endif

static void		PrvConsoleProc(void);
Err	SysLaunchConsole(void)
{	
	Err		err=0;
	
#if	EMULATION_LEVEL == EMULATION_NONE
	UInt16		socket;
	UInt32		taskID;
	UInt32		stackSize = 0x600;
	UInt16						portID;
	SysTCBUserInfoPtr 	tcbInfoP;
	UInt32		baud, flags;
	SrmOpenConfigType	config;

	// If it's already launched, do nothing
	if (GSysConsoleStackChunkP)
		return 0;
	

	//----------------------------------------------------------------------
	// Allocate space for the console task stack.
	//----------------------------------------------------------------------
	GSysConsoleStackChunkP = MemPtrNew(stackSize);
	if (!GSysConsoleStackChunkP)
		return memErrNotEnoughSpace;
	MemPtrSetOwner(GSysConsoleStackChunkP, 0);
	

	//----------------------------------------------------------------------
	// Open up the Serial Library to establish a console connection.
	//----------------------------------------------------------------------

	// If the debugger is already using a comm library, get it's baud
	//   rate and use that as the default baud for the console comm library
	baud = 0; flags = 0;
	err = DbgCommSettings(&baud, &flags);
	if (err || baud == 0)
		baud = sysDefaultSerBaud;
	
	MemSet(&config, sizeof(SrmOpenConfigType), 0);
	config.baud = baud;
	config.function = serFncConsole;
	
	err = SrmExtOpen(prvConsoleSerialPort, &config, sizeof(SrmOpenConfigType), &portID);
	
	if ( err )
		{
		if (err == serErrAlreadyOpen)
			{
			// This is the most likely error that developers can encounter -
			// having the comm library open by their or some other application
			// and attempting to launch the console.  We do not use the
			// ErrFatalDisplayIf macro because it frequently results in sending
			// the text to the serial port instead of displaying it on the Pilot
			// screen.
			if (!SysFatalAlert("Console error: serial manager is already in use"))
				SysReset();
			}
		goto NativeExit;
		}

	//----------------------------------------------------------------------
	// Open up the Console and RemoteUI Sockets.
	//----------------------------------------------------------------------
	socket = slkSocketConsole;
	SlkOpenSocket(portID, &socket, true);

	socket = slkSocketRemoteUI;
	SlkOpenSocket(portID, &socket, true);

	
	//----------------------------------------------------------------------
	// Launch the Console task
	//----------------------------------------------------------------------
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
	if (err)
		{
		SrmClose(portID);
		goto NativeExit;
		}

// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
	*((UInt16 *)tcbInfoP) = (UInt16) portID;

// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
	
NativeExit:											// added: vmk 8/1/96
	if ( err )
		{
		if ( GSysConsoleStackChunkP )			// free the console stack if could not launch
			{
			MemPtrFree(GSysConsoleStackChunkP);
			GSysConsoleStackChunkP = 0;
			}
		}
#endif	//EMULATION_LEVEL == EMULATION_NONE	
	return err;
	
}



/***********************************************************************
 *
 *  FUNCTION:     PrvConsoleProc, private
 *
 *  DESCRIPTION:  The console task for Pilot. This task runs in order
 *    to support console commands from the host. It sits in a loop
 *		calling ConGetS() until a message arrives. ConGetS() looks at packets
 *		and if they are message packets it returns with the message string.
 *
 *		If the packet is instead a remote procedure call packet, ConGetS() will 
 *		automatically process the remote procedure call and wait for another
 *		packet. This is in fact how most console commands are implemented from
 *		the host so ConGetS hardly ever returns with a message.
 *
 *  PARAMETERS:  	void 
 *
 *  RETURNS:      void
 *
 *  CALLED BY:    AMX Kernel after this task is installed and triggerred by 
 *		SysLaunchConsole.
 *
 *  CREATED:      8/7/95
 *
 *  BY:           Ron Marianetti
 *
 ***********************************************************************/
static void PrvConsoleProc(void)
{	
	Char		buffer[256];
	Err		err;
	UInt16					portID;
	UInt32						taskID;
	SysTCBUserInfoPtr 	tcbInfoP;
	const Char *	strP = "\nReady...\n";
	
	
	// Setup A5 world so that 0(A5) gets the pointer
	//  to the SysAppInfo structure for the system.
	SysSetA5((UInt32)&GSysAppInfoP);
	
	
	// Get the taskID and suck the current serial portID out of the SysTaskUserInfoP that has fields which
	// are not being used. Granted, it's a hack. The portID parameter is set before the task is triggered in
	// SysLaunchConsole.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
	portID = *((UInt16 *)tcbInfoP); 

	// Wait for commands
	err = 1;
	while(1) {
		if (err) {
			ConPutS(strP);
			SrmReceiveFlush(portID, sysTicksPerSecond/4);
			}
			
		// Wait for command
		err = ConGetS(buffer, -1); 
		
		// echo it
		if (!err && buffer[0]) 
			ConPutS(buffer);
		}
}




/***********************************************************************
 *
 * FUNCTION:    PrvSendTaxi
 *
 * DESCRIPTION: This routine sends the Taxi across the screen
 *
 * PARAMETERS:  
 *			y 			- initial y coordinate
 *			speed 	- how fast the taxi drives across given in pixels/second.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			ron	11/13/95	Initial Revision
 *
 ***********************************************************************/
#define		kTaxiBitmapID			11000			// Bitmap of Taxi
#define		kTaxiMaskID				11001			// Mask of Taxi
#define		kTaxiSmoke				8				// size of smoke of right side of bitmap
#define		kTaxiBounce				8				// vertical bounce in each direction
#define		kTaxiDBounce			2				// delta bounce each frame
#define 		kTransparentIndex		210

static void PrvSendTaxi (UInt16 speed)
{
	WinHandle			oldDrawWinH, saveWinH=0, workWinH=0;
	WinHandle			taxiWinH=0, maskWinH=0;
	RectangleType		screenR, saveR, workR, taxiR, srcR;
	
	BitmapPtr			taxiBitsP=0;
	BitmapPtr			maskBitsP=0;
	MemHandle				taxiH, maskH;
	
	Int16					screenWidth, screenHeight;
	UInt16					error;
	Int32					lastTicks, framePeriod, wait;
	Int16					delta;
	Int16					bounce=0, dBounce=kTaxiDBounce;
	Int16					y;
	UInt16				depth;
	Boolean				color;
	
	// Pick a random y coordinate to come out from
	y = SysRandom(0) % 140;
	
	HwrDisplayAttributes(false, hwrDispDepth, &depth);
	color = (depth >= 8);
	
	//...................................................................
	// Figure out the delta amount and the delay between frams.
	// The delta is how many pixels away each frame is drawn from
	//  the previous frame.
	// The framePeriod is how many ticks to wait between frames
	//...................................................................
	delta = 8;
	framePeriod = delta * sysTicksPerSecond/speed;
	
	//...................................................................
	// Set the draw window to the display
	//...................................................................
	oldDrawWinH = WinGetDrawWindow();
	WinSetDrawWindow(WinGetDisplayWindow());
	WinGetDisplayExtent(&screenWidth, &screenHeight);
	


	//...................................................................
	// Get the taxi bitmap and mask resources and form window structures
	//  for them.
	//...................................................................
	taxiH = DmGetResource(bitmapRsc, kTaxiBitmapID);
	if (!taxiH) goto Exit;
	taxiBitsP = (BitmapPtr)MemHandleLock(taxiH);
	taxiWinH = WinCreateOffscreenWindow(taxiBitsP->width, taxiBitsP->height,
		screenFormat, &error);
	if (error) goto Exit;
	WinSetDrawWindow(taxiWinH);
	WinDrawBitmap(taxiBitsP, 0, 0);
	WinGetDrawWindowBounds(&taxiR);
	
	if(color) {
		WindowType *winP = WinGetWindowPointer (taxiWinH);
		
		winP->bitmapP->flags.hasTransparency = true;
		winP->bitmapP->transparentIndex = kTransparentIndex;
		
		// initialize the RGB transparentColor field needed for direct bitmaps
		if (depth == 16) {
			// BitmapDirectInfoType appended to the BitmapType structure
			BitmapDirectInfoType* infoP = (BitmapDirectInfoType*) (winP->bitmapP + 1);
			
			// WinIndexToRGB needs to use the display's color table
			WinSetDrawWindow(WinGetDisplayWindow());
			WinIndexToRGB(kTransparentIndex, &infoP->transparentColor);
			WinSetDrawWindow(taxiWinH);
		}
	}
	
	maskH = DmGetResource(bitmapRsc, kTaxiMaskID);
	if (!maskH) goto Exit;
	maskBitsP = (BitmapPtr)MemHandleLock(maskH);
	maskWinH = WinCreateOffscreenWindow(maskBitsP->width, maskBitsP->height,
		screenFormat, &error);
	if (error) goto Exit;
	WinSetDrawWindow(maskWinH);
	WinDrawBitmap(maskBitsP, 0, 0);
	
	
	
	// Clip the delta 
	if (delta > taxiR.extent.x) delta = taxiR.extent.x;
	if (delta < 1) delta = 1;
	
	
	
	
	//...................................................................
	// Form the starting position on the screen
	//...................................................................
	screenR.topLeft.x = screenWidth - delta;
	screenR.topLeft.y = y;
	screenR.extent.x = taxiR.extent.x;
	screenR.extent.y = taxiR.extent.y + kTaxiBounce;
	
	// Form our save window
	saveR = screenR;
	saveR.topLeft.x = saveR.topLeft.y = 0;
	saveWinH = WinCreateOffscreenWindow(saveR.extent.x, saveR.extent.y,
		screenFormat, &error);
	if (error) goto Exit;
	
	
	
	//...................................................................
	// Form the work rectangle and window. This window must be the width of the bitmap
	//  plus the delta width for each step
	//...................................................................
	workR = saveR;
	workR.extent.x += delta;
	workWinH = WinCreateOffscreenWindow(workR.extent.x, workR.extent.y,
		screenFormat, &error);
	if (error) goto Exit;
	
	
	
	//...................................................................
	// Save the bits on the screen to our save window
	//...................................................................
	WinSetDrawWindow(WinGetDisplayWindow());
	WinCopyRectangle(0, saveWinH, &screenR, 0, 0, winPaint);
	
	
	// Init the time and the lastSmoke x coordinate
	lastTicks = 0;
	do {
		
		// Copy the save area to the left side of the work area
		WinCopyRectangle(saveWinH, workWinH, &saveR, 0, 0, winPaint);
		
		
		// Erase the mask portion of the work area
		// If we're at the low point of the bounce, add smoke
		srcR = taxiR;
		if (bounce != kTaxiBounce) {
			srcR.extent.x -= kTaxiSmoke;
			}
			
		// OR in the taxi
		if(color)
			WinCopyRectangle(taxiWinH, workWinH, &srcR, 0, bounce, winPaint);
		else {
			WinCopyRectangle(maskWinH, workWinH, &srcR, 0, bounce, winMask);
			WinCopyRectangle(taxiWinH, workWinH, &srcR, 0, bounce, winOverlay);
		}
		
		// Copy the work rect to the screen after our frame timeout
		wait = lastTicks + framePeriod - TimGetTicks();
		if (wait > 0) SysTaskDelay(wait);
		WinCopyRectangle(workWinH, 0, &workR, 
			screenR.topLeft.x, screenR.topLeft.y, winPaint);
		lastTicks = TimGetTicks();
			
			
		// Shift to the left and add new bounce
		screenR.topLeft.x -= delta;
		bounce += dBounce;
		if (bounce > kTaxiBounce) {
			bounce = kTaxiBounce;
			dBounce = -kTaxiDBounce;
			}
		else if (bounce < 0) {
			bounce = 0;
			dBounce = kTaxiDBounce;
			}
		
		
		// Restore the work area that the taxi will move out of from our save window.
		//  This is the right edge of the save window goes to the right edge of
		//  the work window.
		srcR.topLeft.x = saveR.extent.x - delta;
		srcR.topLeft.y = 0;
		srcR.extent.x = delta;
		srcR.extent.y = saveR.extent.y;
		WinCopyRectangle(saveWinH, workWinH, &srcR,
			workR.extent.x - delta, 0, winPaint);
			
			

		// Shift the save image over to the right
		srcR = saveR;
		srcR.extent.x = saveR.extent.x - delta;
		WinCopyRectangle(saveWinH, saveWinH, &srcR,
			delta, 0, winPaint);
			
		// Save the new left area of the screen to the save window
		srcR = screenR;
		srcR.extent.x = delta;
		WinCopyRectangle(0, saveWinH, &srcR, 0, 0, winPaint);
		
		// Break out if we're off the screen
		} while((screenR.topLeft.x + taxiR.extent.x + delta) > 0);
	
	
	
Exit:
	// Release resources
	if (taxiBitsP) {
		MemHandleUnlock(taxiH);
		DmReleaseResource(taxiH);
		}
	if (maskBitsP) {
		MemHandleUnlock(maskH);
		DmReleaseResource(maskH);
		}
	if (saveWinH) 
		WinDeleteWindow(saveWinH, false);
	if (workWinH) 
		WinDeleteWindow(workWinH, false);
	if (taxiWinH) 
		WinDeleteWindow(taxiWinH, false);
	if (maskWinH) 
		WinDeleteWindow(maskWinH, false);
	
	// Restore the draw window
	WinSetDrawWindow(oldDrawWinH);
}

/************************************************************
 *
 *  FUNCTION: PrvShowTaxi
 *
 *  DESCRIPTION: Return true if we should show the taxi or not.
 *
 *  PARAMETERS: 
 *			forIdle - true if checking at idle time
 *
 *  RETURNS: Boolean
 *
 *  CREATED: 11/14/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
static Boolean 		PrvShowTaxi(Boolean forIdle)
{
	UInt32		time;
	UInt32		flag;
	Boolean	enable=false;
	
	
	// Get the system time
	time  = TimGetSeconds();


	//..................................................................
	// Update the state of the sysPrefFlagNoTaxiIdle flag. If set,
	//  meaning the taxi just showed up recently, clear it if
	//  the 16th bit of the system time has changed since we
	//  set it. This will happen about once every 18 hours. This way,
	//  we make sure the Taxi doesn't show up more than about once
	//  a day.
	//..................................................................
	if (GSysPrefFlags & sysPrefFlagTaxiDisIdle) {
		flag = GSysPrefFlags & sysPrefFlagTaxiDisIdleTime;
		flag <<= 2;						// because flag is either 0x4000 or 0
		if ((time & 0x00010000) != flag)
			GSysPrefFlags &= ~sysPrefFlagTaxiDisIdle;
		}
	
	//..................................................................
	// Return true if the NoTaxi bit is not set.
	//..................................................................
	if (GSysPrefFlags & sysPrefFlagEnableEasterEggs) 
		enable = true;
	
	// If this is not a check to see if it should show up at idle, return
	//   the result now
	if (!forIdle || !enable) return enable;
	
	
	//.........................................................................
	// If this is a check to see if it should show up during idle,
	//  be a little more picky. We will not show it if it just showed up
	//  recently (sysPrefFlagTaxiDisIdle set). 
	// If it didn't show up recently, only show it if the low N bits of
	//  the time equal the top N bits of the time.
	//.........................................................................
	if (GSysPrefFlags & sysPrefFlagTaxiDisIdle) return false;
	if  (((time >> 16) & 0x0FFE) == (time & 0x000FFE))	{		
		GSysPrefFlags |= sysPrefFlagTaxiDisIdle;
		GSysPrefFlags &= ~sysPrefFlagTaxiDisIdleTime;		// clear flag first
		GSysPrefFlags |= (time & 0x00010000) >> 2;			// get current bit value into flag
		return true;
		}

	return false;
}	

/***********************************************************************
 *
 *  FUNCTION:     PrvAutoLockAlarmProc
 *
 *  DESCRIPTION:  This procedure gets called by the Alarm Manager
 *		as a result of the device auto lock alarm going off. 
 *
 *		The alarm manager calls this with one of the AlmProcCmdEnum
 *		commands, most often almProcCmdTriggered or almProcCmdReschedule.
 *
 *
 *  PARAMETERS:   
 *				almProcCmd	-> Which function to perform
 *				paramP	   -> alarm parameters
 *
 *  RETURNS:   	0 if no err
 *
 *  CALLED BY:   Alarm Manager when our alarm triggers or when the time
 *							setting changes
 *
 *  HISTORY:
 *
 *			Name	Date		Description
 *			----	----		-----------
 *			WK		6/21/00	Initial Revision
 *
 ***********************************************************************/
static UInt32 PrvAutoLockAlarmProc (UInt16  /* AlmProcCmdEnum*/ almProcCmd, 
				SysAlarmTriggeredParamType* paramP) 
{

	SysEvtMgrGlobalsPtr	gP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
	Err								err=0, tmpErr;
	SecurityAutoLockType 	autoLockType;
	UInt32						autoLockTime;
	
	switch (almProcCmd) {
	
		case almProcCmdTriggered:


         autoLockTime = paramP->alarmSeconds;
			autoLockType = (SecurityAutoLockType)PrefGetPreference(prefAutoLockType);

				
         
			if (gP->gotUserEvent == false) {  // If device is NOT active (asleep), lock the device now.	
			
				// Between now and the time the device is locked, various screen updates may be performed,
				// such as updating the battery or clock.  If the LCD controller has been put to sleep, and  
				// if accessing the controller while it is off will cause a bus error, turn it on here.
				// The security app will turn it off again.  This will cause the screen to flash on and 
				// off again, but that is better then a bus error.
				if (GHwrWakeUp & hwrWakeUpLCD) {
					Boolean accessOK;
					Err err = HwrDisplayAttributes(false, hwrDispMemAccessOK, &accessOK);
					
					// Avoid the screen flashing on and off if possible.
					// Drivers that have the bus access problem must always respond with dispErrNoErr.  If 
					// HwrDisplayAttributes returns an error, the driver has not been updated to support 
					// this attribute, and it is not necessary to call HwrDisplayWake.
					if ((err == dispErrNoErr) && (accessOK == false))
						HwrDisplayDoze(false);
				}
					
		   	EvtEnqueueKey(lockChr, 0, commandKeyMask);
			}
			else  //otherwise lock the device upon power off.
				GSysAutoLockUponPowerOff = true;
			   
			//Calculate the next auto lock preset time.  
			if (autoLockType == atPresetTime) {
				autoLockTime += daysInSeconds; 

				//Resubmit the next auto lock alarm time.
				tmpErr = AlmSetProcAlarm(&PrvAutoLockAlarmProc, 0, autoLockTime);
				if (!err) err = tmpErr;
				ErrNonFatalDisplayIf(err, "");
			}			 					 		
			break;
			
		//-----------------------------------------------------------
		// For time change, just update tne nextAlarmTIme
		//--------------------------------------------------------------
		case almProcCmdReschedule:
		
			break;

		default:
			ErrNonFatalDisplay("not implemented");
		}			


	return err;
}






					
/************************************************************
 *
 *	FUNCTION: PrvCheckAutoLock
 *
 *  DESCRIPTION: This routine checks whether the device needs to be locked. 	
 *
 *  PARAMETERS:   
 *
 *  RETURNS:  
 *				lockedAndOff: true if it is lock time and the device is about to be locked 
 *										and turned off. 
 *								  false if the it is not lock time. So, we set an alarm to go off
 *								 		when lock time occurs.
 *				   
 *
 *  CALLED BY:  PrvHandleEvent when vchrAutoOff or vchrHardPower events occur.
 *
 *  HISTORY:
 *
 *			Name	Date		Description
 *			----	----		-----------
 *			WK		6/28/00	Initial Revision
 *
 *************************************************************/
static Boolean
PrvCheckAutoLock(SysEventType * eventP)
{	
	SecurityAutoLockType  autoLockType;
    UInt32    				nextLockTime;
	UInt32					tickDelta;
	UInt32					autoLockEvtTicks = GSysAutoOffEvtTicks;
	Boolean					lockedAndOff = false;
	Err						err=0, tmpErr;


		if (PrefGetPreference(prefDeviceLocked))
		    return lockedAndOff;
		    
		    
		autoLockType = (SecurityAutoLockType)PrefGetPreference(prefAutoLockType);


		switch (autoLockType) {
		
		case uponPowerOff:

			//Lock and turn off now.
			eventP->data.keyDown.chr = vchrLock;
			SysEventAddToQueue(eventP);
			lockedAndOff = true;							
			break;

		case atPresetTime:
		 
		   //We get to this point as a result of the preset time lock alarm triggered 
		   //when the device was active. Since we do not want to lock the device in the middle
		   //of activity. Now that it is going to sleep, we can lock it.
		   
			if (GSysAutoLockUponPowerOff) {

				eventP->data.keyDown.chr = vchrLock;
				SysEventAddToQueue(eventP);
				GSysAutoLockUponPowerOff = false;
				lockedAndOff = true;							
			}	
			break;

		case afterPresetDelay:

			if (GSysAutoLockUponPowerOff) {

				eventP->data.keyDown.chr = vchrLock;
				SysEventAddToQueue(eventP);
				GSysAutoLockUponPowerOff = false;
				lockedAndOff = true;							
				break;
			}	


	      if (eventP->data.keyDown.chr == vchrAutoOff) 
		   	 autoLockEvtTicks = GSysAutoOffEvtTicks;
		    
	      else if (eventP->data.keyDown.chr == vchrHardPower) 
		   		
		   	 autoLockEvtTicks = GSysHardPowerEvtTicks;

		
			
			
         //Calculate the ticks since the start of the auto off idle timer,
         //which is basically the same as the auto lock idle timer.	
			tickDelta = GHwrCurTicks - autoLockEvtTicks;

 			// Calculate the time remaining till auto locking occurs
			nextLockTime = GSysAutoLockTimeoutSecs - (tickDelta / sysTicksPerSecond);
		
	
	      
      		if (tickDelta > (Int32)GSysAutoLockTimeoutSecs*sysTicksPerSecond) {
					eventP->data.keyDown.chr = vchrLock;
					SysEventAddToQueue(eventP);				
					lockedAndOff = true;							
	    	}
	    	else {
					nextLockTime += TimGetSeconds();
		
					//submit an alarm for the next lock time.
					tmpErr = AlmSetProcAlarm(&PrvAutoLockAlarmProc, 0, nextLockTime);
					if (!err) err = tmpErr;
					ErrNonFatalDisplayIf(err, "");			 					 			      	
			}
  			break;
  			
  			 

			
	}
return lockedAndOff;

}
/************************************************************
 *
 *	FUNCTION:	SysHandleEvent
 *
 *	DESCRIPTION: Default event handling for System events such as
 *		hard and soft key presses. Applications should call this routine
 *		immediately after calling EvtGetEvent unless they want to 
 *		override the default system behavior.
 *
 *	PARAMETERS: eventP	Pointer to event record.
 *
 *	RETURNS: true if the System handled the event
 *
 *	HISTORY:
 *		04/28/95	RM		Created by Ron Marianetti
 *		07/??/98	kwk	Moved guts to PrvHandleEvent.
 *
 *************************************************************/
Boolean SysHandleEvent(EventType * eventP)
{	
	Boolean requiresUI;
	return(PrvHandleEvent((SysEventType *)eventP, &requiresUI, true));
} // SysHandleEvent


/************************************************************
 *
 *	FUNCTION:	SysWantEvent
 *
 *	DESCRIPTION: Return true if <eventP> is an event that
 *		SysHandleEvent will handle. Set <needsUI> to true if
 *		the act of handling the event will require user interface
 *		interaction (typically a form, or an app launch/sublaunch).
 *
 *	PARAMETERS:
 *		eventP		-> Pointer to event record.
 *		outNeedsUI	-> Return true if event handling would
 *							require UI activity.
 *
 *	RETURNS:	true if the system will handle the event.
 *
 * HISTORY:
 *		??/??/98	kwk	Created by Ken Krugler.
 *
 *************************************************************/
Boolean SysWantEvent(EventType * eventP, Boolean *needsUI)
{
	return(PrvHandleEvent((SysEventType *)eventP, needsUI, false));
} // SysWantEvent


/************************************************************
 *
 *	FUNCTION: PrvHandleEvent
 *
 *	DESCRIPTION:	Guts of SysHandleEvent, which also let us determine
 *		whether the system will handle the event (and require UI interaction)
 * 	without actually processing the event. If <inProcess> is true
 * 	then we'll actually do something, otherwise we'll just set
 * 	up the return result and <outNeedsUI> booleans.
 *
 *	PARAMETERS:	eventP - Pointer to event record.
 *					outNeedsUI - Return true if event handling would
 *					require UI activity.
 *					inProcess - True=>actually handle the event
 *
 *	RETURNS:		true if the System handled the event
 *
 * HISTORY:
 *		04/28/95	RM		Created by Ron Marianetti
 *		07/??/98	kwk	Moved to here from SysHandleEvent.
 *		10/05/98	jfs	Added hardContrastChr case
 *		10/13/98	kwk	Return false (no UI action) for tsm keydown
 *							events, since they don't trigger form activity.
 *		07/15/99	kwk	Changed pen-up event handling so that silkscreen
 *							buttons are processed before Graffiti area check.
 *		08/05/99	kwk	Call TsmHandleEvent versus PrvTsmMapEvent.
 *		11/03/99 SCL	contrastChr is to hardContrastChr as brightnessChr is
 *							to hardBrightnessChr.  The UI slider responds only to
 *							contrastChr; hardContrastChr simply posts one.
 *		11/04/99	jmp	Don't bring up System Launcher in the Simulator!
 *		02/29/00	jmp	Fixed bug #25290:  A clean up of stack-space usage
 *							introduced a problem where the wrong database ID was
 *							being passed to SysUIAppSwitch().  Fixed by reintroducing
 *							a more local database ID that doesn't wipe out the outer-
 *							most scope's database ID.
 *		07/26/00	jhl	Integrate HSIMgr functionality.
 *		12/01/00	kwk	Use StrCompareAscii when comparing PQA DB names.
 *
 *************************************************************/
static Boolean
PrvHandleEvent(SysEventType * eventP, Boolean *outNeedsUI, Boolean inProcess)
{	
	SysEvtMgrGlobalsPtr	gP = (SysEvtMgrGlobalsPtr)GSysEvtMgrGlobalsP;
	UInt32					creator;
	UInt16					cardNo;
	LocalID					dbID;
	PointType*				startPtP;
	PointType*				endPtP;
	Int16						x, y;
	Boolean					penDown;
	UInt32					curCreator;	
	Err						err;
	UInt16					cmd;
	UInt32					endTime;
	SysAppLaunchCmdOpenDBType*	cmdPBP = 0;
	UInt16					paramID = 0;
	DmSearchStateType		searchState;									
	SysNotifyParamType	notifyParam;
	SleepEventParamType	sleepParam;
	SecurityAutoLockType autoLockType;
	UInt32 					refP;
	Boolean					lockedAndOff;
#ifdef INTEGRATED_HSIMGR
	SysHSIResponseType	peripheralResponse;
#endif // INTEGRATED_HSIMGR
			
	*outNeedsUI = false;
	
	//-------------------------------------------------------------------
	// <chg 11/19/98 TLW> Check for any pending stack deletes...
	//-------------------------------------------------------------------
	if (GSysPendingStackFrees) {
		// Handle any pending stack deletes...
		PrvHandleStackDeletes();
		}
	
	//-------------------------------------------------------------------
	// Check w/various parts of the system that might want to handle
	// events very early on.
	//-------------------------------------------------------------------

	if ((TsmHandleEvent(eventP, inProcess))
	|| (IntlHandleEvent(eventP, inProcess)))
		return(true);
	
	//-------------------------------------------------------------------
	// If it's a pen-down event in the system area, track the pen until
	//  it comes up. This is required under emulation mode because calling
	//  EvtGetPen repeatedly allows the emulator to enqueue the points that
	//  comprise the stroke.
	//-------------------------------------------------------------------
	if (eventP->eType == penDownEvent) {
		
		// Convert coordinate into display space.
		x = eventP->screenX;
		y = eventP->screenY;
		if (WinGetDrawWindow())
			WinWindowToDisplayPt(&x, &y);
		
		// If the start point is in the application area, ignore it
		if (y < gP->appAreaBottom) 
			return false;
			
		// Else, track the pen until it comes up. 
		// DOLATER... we should add a flag to SysEvtMgr so that it doesn't
		//  have to wake up the event loop for penMoved events when we don't
		//  need them...
		else	{
			if (inProcess) {
				do {
					EvtGetPen(&x, &y, &penDown);
					} while (penDown);
				}
			return(true);
			}
		}	


	//-------------------------------------------------------------------
	// If it's a pen-up event and it's in the system area, try and
	//  recognize it.
	//-------------------------------------------------------------------
	if (eventP->eType == penUpEvent) {
		
		// If the start point is in the application area, ignore it
		if (eventP->data.penUp.start.y < gP->appAreaBottom) 
			return false;
			
		// Pointers for quick reference
		startPtP = &eventP->data.penUp.start;
		endPtP = &eventP->data.penUp.end;

		// See if it's one of the silkscreen buttons. This includes the two
		// keyboard buttons that overlap with the Graffiti area, which is why
		// we have to check the buttons first. DOLATER kwk - this code currently
		// will skip silkscreen button checks if the inProcess flag is false,
		// which means we'd need to pass that parameter on through to the
		// EvtProcessSoftKeyStroke routine...gross.
		
		if (gP->enableSoftKeys && inProcess) {
			if (EvtProcessSoftKeyStroke(startPtP, endPtP) == errNone) {
				return(true);
				}
			}

		// If the stroke started in the writing area, send it to Graffiti
		//  to recognize it. The GrfProcessStroke routine will flush 
		//  the stroke out for us.
		if (RctPtInRectangle(startPtP->x, startPtP->y, &gP->writingR)) {
		
			// Send the taxi if the stroke ends at the left edge of the screen
			if (endPtP->x < 2)
				if (KeyCurrentState() & keyBitPageDown)
					if (PrvShowTaxi(false)) {		
						*outNeedsUI = true;
						if (inProcess)	{
							PrvSendTaxi(64);
							}
						}
						
			// Process Graffiti Stroke
			if (gP->enableGraffiti) {
				if (inProcess)
					GrfProcessStroke(startPtP, endPtP, false);
				return true;
				}
			else
				return false;
			}
				
		// We didn't find a match, so just return false. Note that to mimic
		// previous behavior, we'll flush the stroke if enableSoftKeys is true,
		// since the call to EvtProcessSoftKeyStroke used to always flush the
		// stroke data.
		// NOTE: Return true if EvtFlushNextPenStroke is called, because otherwise
		// anyone else handling penUp events will assume there is a stroke available.
		
		else {
			if (gP->enableSoftKeys && inProcess) {
				EvtFlushNextPenStroke();
				return true;
				}
			
			return false;
			}
		}
		
	//-------------------------------------------------------------------
	// Have some fun with nil events...
	//-------------------------------------------------------------------
	else if (eventP->eType == nilEvent) {

		// If idle for at least 10 seconds
		if (GHwrCurTicks - GSysAutoOffEvtTicks > 10*sysTicksPerSecond) {

			// And taxi enabled
			if (PrvShowTaxi(true)) {
				*outNeedsUI = true;
				if (inProcess) {
					PrvSendTaxi(64);
					}
				}
			}
		}
	
	
	//-------------------------------------------------------------------
	// Check for System keyboard events
	//-------------------------------------------------------------------
	else if (eventP->eType == keyDownEvent) {

		if (eventP->data.keyDown.modifiers & appEvtHookKeyMask) 
			{
//				UInt16	cardNo;														
//				LocalID	dbID;		
				UInt32   result;
//				UInt32   creator;									
				
				// creator is stored in chr for high two bytes and keycode for lower bytes			
				creator	= eventP->data.keyDown.chr;
				creator <<= 16;
				creator |= eventP->data.keyDown.keyCode;										
				/* Find the application */											
				err = DmGetNextDatabaseByTypeCreator(						
						true/*newSearch*/, &searchState,							
						sysFileTApplication, creator,										
						true/*onlyLatestVers*/, &cardNo, &dbID);		
				if ( !err)
					{
					*outNeedsUI = true;
					if (inProcess)
						err = SysAppLaunch(cardNo, dbID,					
							0/*launchFlags*/, sysAppLaunchCmdEventHook, (MemPtr)eventP, &result);	
					}
			return true;
			}
		else if (eventP->data.keyDown.modifiers & libEvtHookKeyMask) 
			{
				// we call exgLib here
				// In the emulator this will always call exglib,
				// but on the real unit, this will call the first custom trap in 
				// the library referenced by the refnum in keycode.
				if (inProcess)
					ExgLibHandleEvent(eventP->data.keyDown.keyCode,eventP);
				return true;
			}

	
		if (!(EvtKeydownIsVirtual(eventP))) return false;
		
		if ( eventP->data.keyDown.chr == vchrAlarm )
			{
			//------------------------------------------------------------------------
			// Call the Alarm Manager to display any alarms which have expired
			//------------------------------------------------------------------------
			*outNeedsUI = true;
			if (inProcess)
				AlmDisplayAlarm( true/*displayOnly*/);
			return true;
			}

#ifdef INTEGRATED_HSIMGR
		//============================================================================
		// If either cradle key, initiate cradle action.
		// jhl 7/26/00 Integrate HSIMgr functionality
		//============================================================================
		//
		// On receipt of either cradle character:
		//
		//		If it's a cradle character, post an 'rs2c' (cradle) notification.
		//		Else if it's a cradle2 character, post an 'rs2p' (peripheral) notification.
		//
		// If the notification is not handled (normally the case):
		//
		//		If it's a cradle character, perform normal cradle key activity.
		//		Else if serial port is already open, post 'hsiu' (in use) notification.
		//		Else
		//			Open serial port at 9600 baud, send the inquiry string "ATI3\r" and wait
		//			up to 100 ms for a response.
		//			Close the serial port.
		//			If a response is received, post 'hspr' (peripheral responded)
		//				notification and include response.
		//			Else post 'hspn' (peripheral not responding) notification.
		//
		// If the notification is not handled, invoke default action:
		//
		//		If 'hsiu', perform normal cradle2 key activity.
		//		Else if 'hspr', perform normal cradle2 key activity.
		//		Else if 'hspn', perform normal cradle2 key activity.
		//
		//============================================================================
		if (eventP->data.keyDown.chr == vchrHardCradle ||
				eventP->data.keyDown.chr == vchrHardCradle2) {

			// What about the inProcess flag?
			// if (!inProcess) ...

			if (eventP->data.keyDown.chr == vchrHardCradle2)
				notifyParam.notifyType = sysNotifyHSIRS232PeripheralEvent;
			else
				notifyParam.notifyType = sysNotifyHSIRS232CradleEvent;
			notifyParam.broadcaster = sysFileCSystem;
			notifyParam.notifyDetailsP = 0;
			notifyParam.handled = false;
			SysNotifyBroadcast(&notifyParam);
			// If cradle notification was handled, we're done (not normally the case)
			if (notifyParam.handled) return(true);

			// If cradle2, try to ID the peripheral
			if (eventP->data.keyDown.chr == vchrHardCradle2) {
				// Try to ID the peripheral
				notifyParam.notifyType = PrvHSIQueryPeripheral(&peripheralResponse);
				if (notifyParam.notifyType) {
					notifyParam.broadcaster = sysFileCSystem;
					notifyParam.notifyDetailsP = &peripheralResponse;
					notifyParam.handled = false;
					SysNotifyBroadcast(&notifyParam);
					// If notification was handled, we're done
					if (notifyParam.handled) return(true);
				}
			}

			// If we get here, we're falling through to perform
			// the default cradle/cradle2 action.
		}
#endif // INTEGRATED_HSIMGR

		// Play click sound except for some command keys that should stay silent
		if (inProcess &&
			 eventP->data.keyDown.chr != vchrHardPower &&
			 eventP->data.keyDown.chr != vchrLowBattery &&
			 eventP->data.keyDown.chr != vchrRonamatic &&
			 eventP->data.keyDown.chr != vchrSendData &&
			 eventP->data.keyDown.chr != vchrBacklight &&
			 eventP->data.keyDown.chr != vchrAutoOff &&
			 eventP->data.keyDown.chr != vchrPowerOff &&
			 eventP->data.keyDown.chr != vchrLateWakeup &&
			 eventP->data.keyDown.chr != vchrResumeSleep &&
			 eventP->data.keyDown.chr != vchrLock &&
			 eventP->data.keyDown.chr != vchrAttnStateChanged &&
			 eventP->data.keyDown.chr != vchrAttnUnsnooze &&
			 eventP->data.keyDown.chr != vchrAttnIndicatorTapped &&
			 eventP->data.keyDown.chr != vchrAttnAllowClose &&
			 eventP->data.keyDown.chr != vchrAttnReopen &&
			 eventP->data.keyDown.chr != vchrRadioCoverageOK &&
			 eventP->data.keyDown.chr != vchrRadioCoverageFail &&
			 ((eventP->data.keyDown.chr != vchrMenu) ||
			 	((eventP->data.keyDown.modifiers & autoRepeatKeyMask) == 0)) &&
			 ((eventP->data.keyDown.chr != vchrCommand) ||
			 	((eventP->data.keyDown.modifiers & autoRepeatKeyMask) == 0)))
			 		// Don't click when processing the second vchrMenu or vchrCommand key down
			 		// event. This event has the autoRepeatKeyMask bit set to indicate this.
			 		// This should make the "evil hack" in MenuHandleEvent actually work. - dje
			SndPlaySystemSound(sndClick);
	
		// Get the creator type of the app we need to switch to
		cmd = sysAppLaunchCmdNormalLaunch;
		switch (eventP->data.keyDown.chr) {
			case vchrHardPower:
				if (!inProcess)
					return(true);

				// If the power key is held down for more than keyMinBacklightTicks,
				//  we toggle the backlight. Else, we go to sleep
				endTime = TimGetTicks() + keyMinBacklightTicks;
				
        		// Make sure we have a backlight.
        		// 12/1/98 JAQ: removed check for sysMiscFlagBacklightDisable.
        		//	It is only a legacy flag - new power manager makes it obsolete.
        		// It is included in 3.2 IRLib, but nowhere else in 3.2. Moreover, as
        		// the following noise that used to be in this comment block indicates, 
        		//	if this check were appropriate it would belong elsewhere.
						// What *IS* sysMiscFlagBacklightDisable?  IR disabling?  If so, that
						// *should* be handled by HwrBacklight's call to hwrBatteryCmdAddload.
				if ((GHwrMiscFlags & hwrMiscFlagHasBacklight)
					|| (GHwrMiscFlagsExt & hwrMiscFlagExtHasSWBright))
					do {
						if (!(KeyCurrentState() & keyBitPower))
							break;
						SysTaskDelay(1);							// to conserve power..
						} while (TimGetTicks() < endTime);
				
				// Key held down for a "long" time? If so, toggle backlight
				if (TimGetTicks() >= endTime) {
					// NOTE: eventP already contains a keyDownEvent w/commandKeyMask
					// so we just change the keyDown.chr and re-queue it. 
					if (GHwrMiscFlagsExt & hwrMiscFlagExtHasSWBright)
						eventP->data.keyDown.chr = brightnessChr;
					else
						eventP->data.keyDown.chr = backlightChr;
					
					
					// Replaced EvtEnqueueKey() with EvtAddEventToQueue(). 8/14/98 SCL
					SysEventAddToQueue(eventP);
					}
				// If not, turn off. 
				else {

					lockedAndOff = PrvCheckAutoLock(eventP);
					
					if(!lockedAndOff) {

						// Replaced SysSleep() with EvtEnqueueKey().  Apps need to be
						// notified in case they have active connections. 8/14/98 JB	
						// NOTE: eventP already contains a keyDownEvent w/commandKeyMask
						// so we just change the keyDown.chr and re-queue it. 
						eventP->data.keyDown.chr = powerOffChr;
					
						// Broadcast the sleep notification:
						sleepParam.reason		= sysSleepPowerButton;
						sleepParam.deferSleep= 0;
					
						notifyParam.notifyType		= sysNotifySleepRequestEvent;
						notifyParam.broadcaster		= sysFileCSystem;
						notifyParam.notifyDetailsP	= &sleepParam;
						notifyParam.handled			= false;
					
						SysNotifyBroadcast(&notifyParam);
					
						// if nobody deferred the sleep request, enqueue the key.
						// Otherwise, wait for a resumeSleepChr...
						if(sleepParam.deferSleep == 0)
								SysEventAddToQueue(eventP);
						
					}
				}
				return true;
				
			case vchrAutoOff:
				if (!inProcess)
					return(true);

				// Check if dev needs to be locked
				lockedAndOff = PrvCheckAutoLock(eventP);
					     

				//  Reset the auto-off timer so that our timer task doesn't
				//  try and put us right back to sleep after we wake. If no one
				//  calls EvtResetAutoOffTimer() before the next cycle through
				//  the event loop after waking, then EvtGetSysEvent() will put
				//  us back to sleep again. Added since we no longer call
				// SysSleep() directly.  8/14/98 JB
				// JED 5/14/99 Replace global smash with new API call. 
				EvtSetAutoOffTimer(ResetTimer,0);
				
				if (PrefGetPreference(prefStayOnWhenPluggedIn) && HwrPluggedIn()) 
				{
					//If auto locking is enabled, then override Stay on in cradle.
					autoLockType = (SecurityAutoLockType)PrefGetPreference(prefAutoLockType);			
				   if (autoLockType == never) {
					// user wants to stay on, so reset timeout counter so we won't
					// do this again for a while, and turn off backlight (unless told not to)
						if (!PrefGetPreference(prefStayLitWhenPluggedIn) && (!(GHwrMiscFlagsExt & hwrMiscFlagExtHasSWBright)))
							HwrBacklight(true, false);
						return true;
					}
				}
				
					
				if(!lockedAndOff) {

					// Replaced SysSleep() with EvtEnqueueKey().  Apps need to be
					// notified in case they have active connections. 8/14/98 JB
					// Replaced EvtEnqueueKey() with EvtAddEventToQueue(). 8/14/98 SCL
					// NOTE: eventP already contains a keyDownEvent w/commandKeyMask
					// so we just change the keyDown.chr and re-queue it. 
					eventP->data.keyDown.chr = powerOffChr;
				
					// Broadcast a sleep request since we will be going to sleep soon.
					{
					// Broadcast the sleep notification:
					sleepParam.reason		= sysSleepAutoOff;
					sleepParam.deferSleep= 0;
					
					notifyParam.notifyType		= sysNotifySleepRequestEvent;
					notifyParam.broadcaster		= sysFileCSystem;
					notifyParam.notifyDetailsP	= &sleepParam;
					notifyParam.handled			= false;
					
					SysNotifyBroadcast(&notifyParam);
					
					// if nobody deferred the sleep request, enqueue the key.
					// Otherwise, wait for a resumeSleepChr...
					if(sleepParam.deferSleep == 0)
						  	SysEventAddToQueue(eventP);

				    }
					
				}
				
				return true;
			
		  	case vchrResumeSleep:
				if (!inProcess)
					return(true);
				
				// Broadcast a sleep notification since we will be going to sleep now.
				{
					// Replaced SysSleep() with EvtEnqueueKey().  Apps need to be
					// notified in case they have active connections. 8/14/98 JB	
					// NOTE: eventP already contains a keyDownEvent w/commandKeyMask
					// so we just change the keyDown.chr and re-queue it. 
					eventP->data.keyDown.chr = powerOffChr;
					
					// Broadcast the sleep notification:
					sleepParam.reason		= sysSleepResumed;
					sleepParam.deferSleep= 0;
					
					notifyParam.notifyType		= sysNotifySleepRequestEvent;
					notifyParam.broadcaster		= sysFileCSystem;
					notifyParam.notifyDetailsP	= &sleepParam;
					notifyParam.handled			= false;
					
					SysNotifyBroadcast(&notifyParam);
					
					// if nobody deferred the sleep request, enqueue the key.
					// Otherwise, wait for a resumeSleepChr...
					if(sleepParam.deferSleep == 0)
						{
						SysEventAddToQueue(eventP);
						}
					
				}
				return true;
				
		  	case vchrLateWakeup:
				if (!inProcess)
					return(true);

				{
				//Now that the device is turned on, delete any auto lock 
				//alarm for the preset delay type. Basically, resetting
				//Auto lock to synchronize with Auto off timer.
				if (!(GHwrWakeUp & hwrWakeUpLCD)) {
					autoLockType = (SecurityAutoLockType)PrefGetPreference(prefAutoLockType);

					if (autoLockType == afterPresetDelay && AlmGetProcAlarm(&PrvAutoLockAlarmProc, &refP)) {
						err = AlmSetProcAlarm(&PrvAutoLockAlarmProc, 0, 0);
						ErrNonFatalDisplayIf(err, "");
					}
				}


				// Broadcast a wakeup notification since we are fully awake now
					notifyParam.notifyType		= sysNotifyLateWakeupEvent;
					notifyParam.broadcaster		= sysFileCSystem;
					notifyParam.notifyDetailsP	= 0;
					notifyParam.handled			= false;
					
					SysNotifyBroadcast(&notifyParam);
				}
				return true;
				
		  	case vchrPowerOff:
				if (!inProcess)
					return(true);

		  		{
					// Broadcast the sleep notification:
					notifyParam.notifyType		= sysNotifySleepNotifyEvent;
					notifyParam.broadcaster		= sysFileCSystem;
					notifyParam.notifyDetailsP	= 0;
					notifyParam.handled			= false;
					
					SysNotifyBroadcast(&notifyParam);
			  		
			  		// Moved call to NetLibPowerOff to SysSleep() 11/28/98 JB
			  		// Queued as a result of hardPowerChr.  Now we can assume the app
			  		// has done what it needs to do.   8/14/98 JB
					
			  		SysSleep (false, false);
		  			
					// Broadcast the early wakeup notification:
					notifyParam.notifyType		= sysNotifyEarlyWakeupEvent;
					notifyParam.broadcaster		= sysFileCSystem;
					notifyParam.notifyDetailsP	= 0;
					notifyParam.handled			= false;
					
					SysNotifyBroadcast(&notifyParam);
					
					// Enqueue key to cause late wakeup notification broadcast:
					eventP->data.keyDown.chr = lateWakeupChr;
					SysEventAddToQueue(eventP);
					
	  			}
		  		return true;
			

			case vchrRonamatic:
				if (inProcess)
					{
					UInt16 keyCode;
					keyCode = (UInt16) PrefGetPreference(prefRonamaticChar);
					// Replaced EvtEnqueueKey() with EvtAddEventToQueue(). 8/14/98 SCL
					// NOTE: eventP already contains a keyDownEvent w/commandKeyMask
					// so we just change the keyDown.chr and re-queue it. 
					eventP->data.keyDown.chr = keyCode;
					SysEventAddToQueue(eventP);
					}
				return true;
				

			case vchrKeyboard:
				*outNeedsUI = true;
				if (inProcess)
					SysKeyboardDialog(kbdDefault);
				return true;

			case vchrKeyboardAlpha:
				*outNeedsUI = true;
				if (inProcess)
					SysKeyboardDialog(kbdAlpha);
				return true;

			case vchrKeyboardNumeric:
				*outNeedsUI = true;
				if (inProcess)
					SysKeyboardDialog(kbdNumbersAndPunc);
				return true;

			case vchrGraffitiReference:
				*outNeedsUI = true;
				if (inProcess)
					SysGraffitiReferenceDialog(referenceDefault);
				return true;

			case vchrLaunch:
				creator = PrefGetPreference(prefLauncherAppCreator);
				
				// Fall into the code to switch to a launcher app
				break;

			case vchrFind: {
				*outNeedsUI = true;
				if (inProcess)
					{
					GoToParamsType goTo;
					Find (&goTo);
					}
				return true;
				}

			case vchrLock:
				{
				// Get the database ID of the Security app given the creator type
				err = DmGetNextDatabaseByTypeCreator(true, &searchState, 
					sysFileTApplication, sysFileCSecurity, true, &cardNo, &dbID);
				
				// Try and switch apps passing the lock command.
				if (dbID && !err)
					{
					*outNeedsUI = true;
					if (inProcess) {
					   //We need to close the attention manager dialog if it is open
					   //and open it in the Security app after the system lockout screen
					   //is displayed and the device is locked.
						AttnAllowClose();
						
						SysUIAppSwitch(cardNo, dbID, sysAppLaunchCmdSystemLock, 0);
						}
					}
					
				return true;
				}
				
			case vchrBacklight:
				if (inProcess)
					HwrBacklight(true, !HwrBacklight(false,false));	// toggle or put up adjust dialog depending on hwr
				return true;
				break;

			case vchrContrast:
				*outNeedsUI = true;
				if (inProcess)
					UIContrastAdjust();		// put up adjust dialog depending on hwr
				return true;
				break;

			case vchrBrightness:
				*outNeedsUI = true;
				if (inProcess)
					UIBrightnessAdjust();	// toggle or put up adjust dialog depending on hwr
				return true;
				break;

			case vchrCalc:
				creator = PrefGetPreference(prefCalcCharAppCreator);
				paramID = sysResIDCalcButtonParam;
				break;

			case vchrHardCradle:
				creator = PrefGetPreference(prefHardCradleCharAppCreator);
				paramID = sysResIDCradleParam;
				break;

			case vchrHardCradle2:
				creator = PrefGetPreference(prefHardCradle2CharAppCreator);
				paramID = sysResIDModemParam;
				break;

			case vchrHard1:
				creator = PrefGetPreference(prefHard1CharAppCreator);
				paramID = sysResIDButton1Param;
				break;

			case vchrHard2:
				creator = PrefGetPreference(prefHard2CharAppCreator);
				paramID = sysResIDButton2Param;
				break;

			case vchrHard3:
				creator = PrefGetPreference(prefHard3CharAppCreator);
				paramID = sysResIDButton3Param;
				break;

			case vchrHard4:
				creator = PrefGetPreference(prefHard4CharAppCreator);
				paramID = sysResIDButton4Param;
				break;

			case vchrHardAntenna:
				if (!inProcess)
					return(true);

				{
					notifyParam.notifyType		= sysNotifyAntennaRaisedEvent;
					notifyParam.broadcaster		= sysFileCSystem;
					notifyParam.notifyDetailsP	= 0;
					notifyParam.handled			= false;
					
					SysNotifyBroadcast(&notifyParam);
					
					// If broadcast was handled, 
					// then don't launch the antenna app or anything.
					if(notifyParam.handled != false)
						{
						return true;
						}
				}
				
			   // Now we must check against the alert for the antenna being up, and if it is, then this is
			   // a special case and we must take care of it.
				// We will now check to see if the antenna down alert is being displayed and if
				// so, then we will send an event to close that alert form BEFORE we send the 
				// event to indicate that the antenna has gone up.  The event that we send is one
				// to simulate that the "Cancel" button has been pressed, which is the only button on
				// the form, therefore represented by CustomAlertFirstButton.
//mgmg
/*				if (GCommActivityFlags & sysCommActivityAntennaAlert)
				{
					event.eType = ctlSelectEvent;
					event.data.ctlSelect.pControl = FrmGetObjectPtr (FrmGetActiveForm (), 
												FrmGetObjectIndex (FrmGetActiveForm (), CustomAlertFirstButton));
					event.data.ctlSelect.controlID = CustomAlertFirstButton;
					EvtAddEventToQueue (&event);
					// Now repost the hardAntennaChr and get out.  NOTE:  The next time this event is processed
					// by this function, the "GCommActivityFlags & sysCommActivityAntennaAlert" will not allow us
					// to drop in here because the previous event that we have placed on the queue will cause
					// the ErrorMgr to clear that bit and we should drop through just fine to the next stuff.
					EvtEnqueueKey(hardAntennaChr, 0, commandKeyMask);
				   return (true);
				}
				// if the sysCommActivityDiscardAntKey bit is set, then eat the event.
				// if the app wants the event, then they need to get it before SysHandleEvent()
				else if (GCommActivityFlags & sysCommActivityDiscardAntKey)
					{
					return (true);
					}
*/					
				creator = PrefGetPreference(prefAntennaCharAppCreator);
				paramID = sysResIDAntennaButtonParam;
				break;

			case vchrLowBattery: 
				// Put up the battery warning and schedule the next one
				if (!gP->displayingBatteryAlert)
					{
					*outNeedsUI = true;
					if (inProcess)
						{
						gP->displayingBatteryAlert = true;
						SysBatteryDialog();
						gP->displayingBatteryAlert = false;
						}
					}
				return true;
				
			case vchrEnterDebugger: 
				// Go into Debugger. Set the GDbgWasEntered global so that the
				//  Debugger exception handler doesn't reset the device.
				if (inProcess)
					{
					GDbgWasEntered |= dbgEnteredFlagTrue;
					DbgBreak();
					}
				return true;
				
			case vchrStartConsole: 
				// Startup the console task
				if (inProcess)
					SysLaunchConsole();
				return true;
				
			case vchrRadioCoverageOK:
				// radio coverage check completed, success.
				if (inProcess)
					SndPlaySmfResource(midiRsc, coverageOKMidiID, prefSysSoundVolume);
				return true;

			case vchrRadioCoverageFail:
				// radio coverage check completed, failed.
				if (inProcess)
					SndPlaySystemSound(sndError);
				return true;

			case vchrHardContrast:
				// handle the contrast hard key (if any)
				if (inProcess) {
					// NOTE: eventP already contains a keyDownEvent w/commandKeyMask
					// so we just change the keyDown.chr and re-queue it. 
					eventP->data.keyDown.chr = vchrContrast;
					SysEventAddToQueue(eventP);
				}
				return true;

			case vchrHardBrightness:
				// handle the brightness hard key (if any)
				if (inProcess) {
					// NOTE: eventP already contains a keyDownEvent w/commandKeyMask
					// so we just change the keyDown.chr and re-queue it. 
					eventP->data.keyDown.chr = vchrBrightness;
					SysEventAddToQueue(eventP);
				}
				return true;

			case vchrAttnStateChanged:
			case vchrAttnUnsnooze:
			case vchrAttnIndicatorTapped:
			case vchrAttnAllowClose:
			case vchrAttnReopen:
				*outNeedsUI = true;
				if (inProcess)
					if (AttnHandleEvent(eventP))
						return true;
				break;

			default:
				return false;
			}
		
		
		// Under emulation, we can't launch another app so don't
		//  handle the event
		#if EMULATION_LEVEL != EMULATION_NONE
		return false;
		#endif


		// Get the creator type of the currently running app to see if the
		//  User hit the same button
		curCreator = 0;
		err = SysCurAppDatabase(&cardNo, &dbID);
		if (!err) 
			err = DmDatabaseInfo(cardNo, dbID, 0,
						0, 0, 0,
						0, 0,
						0, 0,
						0, 0,
						&curCreator);
						
		// If this app is already running, don't launch it again and say
		//  we didn't handle the event so that the app ends up getting the
		//  key event.  Now, if this is the clipper application, then we must do
		// further analysis to make sure that the current running pqa is different than
		// the one selected.  If we are clipper and have to check against the pqa name, then
		// that is done later on when the pqa's are referenced.
		if (curCreator == creator && curCreator != sysFileCClipper)
		{
		  return false;
		}
		
		// if a parameter ID exists and the creator is clipper (because clipper cannot be called 
		// directly - ever) it is an indicator that we are to goto a pqa (database) by calling up
		// clipper and having the pqa represented by the parameter block passed in to clipper so that 
		// clipper will startup that particular pqa..
		if (paramID && creator == sysFileCClipper)
			{
			
			Int16 				version;
			UInt16 				size = 0;	// get the pref size by passing zero sized buffer
			Char 					*pqaName;
			SysDBListItemType *dbListIDsP;
			MemHandle 			dbListIDsH;
			UInt16   			dbCount = 0;
			Boolean 				createStatus;
			UInt16 				counter;
			UInt32				cardNo;		// defined as UInt32 for use with FtrGet (see below)
			LocalID				pqaDBId;
			
			cmdPBP = 0;			// Initialize so if we cannot find, nothing gets sent in parameter block.
			size = maxParamSize;
			pqaName = MemPtrNew(size);
			ErrNonFatalDisplayIf (!pqaName, "Cannot allocate memory for pqaName");
			
			if (pqaName)
				{
				version = PrefGetAppPreferences (sysResTSysPref, paramID, pqaName, &size, true);
				if (version != noPreferenceFound && size > 0)
					{
					
					// Get a list of all the databases associated with the pqa files and then search them
					// for the one that matches the one in the preferences.
					createStatus = SysCreateDataBaseList (sysFileTpqa, 0, &dbCount, &dbListIDsH, false);
					if (createStatus == true && dbCount > 0)
						{
						dbListIDsP = MemHandleLock (dbListIDsH);
						for (counter = 0; counter < dbCount; counter++)
							{
							// If we have found the one that we are looking for, then lets make its data the
							// parameter block data. Note that we can use StrCompareAscii since SysCreateDataBaseList
							// was called with false for the lookupName parameter, so we know the names are
							// 7-bit ASCII values from the DB header.
							if (StrCompareAscii (dbListIDsP [counter].name, pqaName) == 0)
								{
								// Now that we have the matching name, this is the pqa to goto.  What we need
								// to do now is check this pqa against the pqa that is currently running in clipper
								// if clipper is currently running.  If it is and the pqa we are to run is the
								// current one executing, then do nothing about switching and just get out now.
								if (curCreator == sysFileCClipper)
									{
									err = FtrGet (sysFileCClipper, sysClipperPQADbIDIndex, (UInt32 *) &pqaDBId);
									if (err == 0 && pqaDBId == dbListIDsP [counter].dbID )
										{
										err = FtrGet (sysFileCClipper, sysClipperPQACardNoIndex, &cardNo);
										if (err == 0 && cardNo == dbListIDsP [counter].cardNo)
											{
											// Free the memory allocated above.
											MemPtrFree (dbListIDsP);
											MemPtrFree (pqaName);
											return (false);
											}
										}
									}
								
								
								// Create the param block
								cmdPBP = MemPtrNew (sizeof (SysAppLaunchCmdOpenDBType));
								ErrNonFatalDisplayIf (!cmdPBP, "Cannot allocate memory for cmdPBP");
								
								// Fill it in
								MemPtrSetOwner (cmdPBP, 0);
								cmdPBP->cardNo 	= dbListIDsP [counter].cardNo;
								cmdPBP->dbID 		= dbListIDsP [counter].dbID;
								cmd 					= sysAppLaunchCmdOpenDB;		// The command is to represent the pqa in the parameter block.
								break;
								}
							}
						// Free the memory allocated above.
						MemPtrFree (dbListIDsP);
						}
					}
					MemPtrFree (pqaName);
				}
			}
		
		// Get the database ID of the app given the creator type
		err = DmGetNextDatabaseByTypeCreator(true, &searchState, 
			sysFileTApplication, creator, true, &cardNo, &dbID);
			
		// If the app DB is not found or the PQA to be launched
		// is not found and it's one of the "button" apps then
		// revert back to the default app assigned to the button
		if (err == dmErrCantFind || (creator == sysFileCClipper && paramID && cmdPBP == 0)) {
			MemHandle buttonDefaultResourceH;
			ButtonDefaultListType *buttonDefaults;
			UInt16 i;
			
			// clear any launch code or parameter block that may have been set
			cmd = sysAppLaunchCmdNormalLaunch;
			if (cmdPBP) {
				MemPtrFree(cmdPBP);
				cmdPBP = NULL;
			}
			
			// the default is that we do not find an app 
			err = dmErrCantFind;
			
			// get the resource that holds the default button assignments
			buttonDefaultResourceH = DmGetResource(sysResTButtonDefaults, sysResIDButtonDefaults);
			ErrNonFatalDisplayIf(buttonDefaultResourceH == NULL, "button defaults resource not found");
			buttonDefaults = MemHandleLock(buttonDefaultResourceH);
			
			// look for an assignment that matches the button's key code
			for (i = 0; i < buttonDefaults->numButtons; i++) {
				if (eventP->data.keyDown.chr == buttonDefaults->button[i].keyCode) {
					creator = buttonDefaults->button[i].creator;
					err = 0;
					break;
				}
			}
			MemHandleUnlock(buttonDefaultResourceH);
				
			if (err == 0) {
				err = DmGetNextDatabaseByTypeCreator(true, &searchState, 
					sysFileTApplication, creator, true, &cardNo, &dbID);
				}
			
			// If not found, launch the preferences app with normal action code
			if (err) {
				err = DmGetNextDatabaseByTypeCreator(true, &searchState, 
					sysFileTApplication, sysFileCPreferences, true, &cardNo, &dbID);
				if (err) dbID = 0;
				}
			}
		
		// set special launch commands for specific button/app combinations
		switch (eventP->data.keyDown.chr)
		{
			case vchrHardCradle:
				if (creator == sysFileCSync)
					cmd = sysAppLaunchCmdSyncRequestLocal;
				break;
			
			case vchrHardCradle2:
				if (creator == sysFileCSync)
					cmd = sysAppLaunchCmdSyncRequestRemote;
				break;
			
			case vchrHardAntenna:
				if (creator == sysFileCLauncher)
					cmd = sysAppLaunchCmdAntennaUp;
				break;			
		}
		
		// Try and switch apps.
		if (dbID)
			{
			*outNeedsUI = true;
			if (inProcess)
				SysUIAppSwitch(cardNo, dbID, cmd, (MemPtr)cmdPBP);
			}
		
		return true;
		}
		
		
	// Else, don't handle it.
	return false;
} // PrvHandleEvent



/************************************************************
 *
 *	FUNCTION: PrvHandleStackDeletes
 *
 *	DESCRIPTION: Handle any pending stack deletes.
 *
 *	PARAMETERS: none
 *
 *	RETURNS: none
 *
 *	HISTORY:
 *		11/19/98 TLW	Created by Tim Wiegman
 *		10/11/99	SCL	Moved to separate function
 *
 *************************************************************/
static void PrvHandleStackDeletes(void)
{	
	UInt16					status;
	SysAppInfoPtr			pendingStackFreeP;
	SysAppInfoPtr			prevPendingStackFreeP;

	// Disable interrupts
	status = SysDisableInts();
	
	// Find a stack that can be freed
	prevPendingStackFreeP = NULL;
	pendingStackFreeP = (SysAppInfoPtr)GSysPendingStackFrees;
	while (pendingStackFreeP && (pendingStackFreeP->taskID != 0)) {
		prevPendingStackFreeP = pendingStackFreeP;
		pendingStackFreeP = (SysAppInfoPtr)pendingStackFreeP->extraP;
		}
		
	// Remove it from the list
	if (pendingStackFreeP) {
		if (!prevPendingStackFreeP)
			GSysPendingStackFrees = pendingStackFreeP->extraP;
		else
			prevPendingStackFreeP->extraP = pendingStackFreeP->extraP;
		}

	// Restore interrupt state
	SysRestoreStatus(status);
	
	// Free the stack (if it exists - why wouldn't it?)
	if (pendingStackFreeP->stackP) 
		MemPtrFree(pendingStackFreeP->stackP);
		
	// Free the appInfo chunk (used as the pendingStackFree chunk now)
	MemPtrFree(pendingStackFreeP);
}


/************************************************************
 *
 *	FUNCTION: SysUIAppSwitch
 *
 *	DESCRIPTION: Tries to make the current UI application quit and
 *   then launches the given UI application by card number and database ID.
 *
 *	PARAMETERS: cardNo, dbID
 *
 *	RETURNS: errNone if no err
 *
 *	HISTORY:
 *		04/26/95	ron	Created by Ron Marianetti
 *
 *************************************************************/
Err SysUIAppSwitch(UInt16 cardNo, LocalID dbID, UInt16 cmd, MemPtr cmdPBP)
{	
#if EMULATION_LEVEL != EMULATION_MAC
	SysEventType event;
#endif

	ErrFatalDisplayIf(cardNo >= GMemCardSlots, "Invalid card");
	
	// If the last launch attempt failed, release the command parameter block, if
	//  any. When a launch succeeds, the UIAppShell will clear this global
	//  and free the chunk itself  when the app quits.
	if (GNextUIAppCmdPBP) {
		MemPtrFree(GNextUIAppCmdPBP);
		GNextUIAppCmdPBP = 0;
		}

// don't switch at all in Mac Simulator builds
#if EMULATION_LEVEL != EMULATION_MAC

	// Save info necessary to launch the next app
	GNextUIAppDBID = dbID;
	GNextUIAppCardNo = cardNo;
	GNextUIAppCmd = cmd;
	GNextUIAppCmdPBP = cmdPBP;
	GSysMiscFlags |= sysMiscFlagAlwaysSwitchApp;
	

	// Send a quit event to the current UI App
	#if EMULATION_LEVEL != EMULATION_NONE
	ErrFatalDisplayIf(GCurUIAppInfoP == 0, "No app running");
	#endif
	
	event.eType = sysEventAppStopEvent;
	SysEventAddToQueue(&event);

#endif

	return errNone;
}


/************************************************************
 *
 *  FUNCTION: SysCurAppDatabase
 *
 *  DESCRIPTION: Returns the card number and Database ID of the 
 *   current application's resource database.  The current app is 
 *   defined as the last running action code or else the UI app.
 *
 *  PARAMETERS: none
 *
 *  RETURNS: 0 if error.
 *
 *  CREATED: 4/27/95
 *
 *  BY: Ron Marianetti
 *
 *	Name	Date		Description
 *	----	----		-----------
 *	roger	12/12/97	Change to use SysCurAppInfo
 *
 *************************************************************/
Err	SysCurAppDatabase(UInt16 * cardNoP, LocalID* dbIDP)
{
	SysAppInfoPtr	infoP;
	SysAppInfoPtr	uiAppP;
	SysAppInfoPtr	actionCodeAppP;
	Err				err;
	
	// Assume error
	*cardNoP = 0;
	*dbIDP = 0;
	
	// Return 0,0 if running emulated mode
	#if EMULATION_LEVEL != EMULATION_NONE
	return 0;
	#endif
	
	// Get pointer to the app info
	SysGetAppInfo(&uiAppP, &actionCodeAppP);
	if (actionCodeAppP)
		infoP = actionCodeAppP;
	else
		infoP = uiAppP;
	if (infoP) 
		err = DmOpenDatabaseInfo((DmOpenRef)infoP->dbP, dbIDP, 0, 
							0, cardNoP, 0);
	else
		err = sysErrParamErr;
		
	return err;
}
	


/************************************************************
 *
 *  FUNCTION: SysCurAppInfoPV20
 *
 *  DESCRIPTION: Returns the SysAppInfoPtr for the current app. 
 *   This routine uses global variables and doesn't rely on A5.
 *
 *  PARAMETERS: none
 *
 *  RETURNS: 0 if error.
 *
 *  CREATED: 4/27/95
 *
 *  BY: Ron Marianetti
 *
 *	Name	Date		Description
 *	----	----		-----------
 *	roger	12/12/97	Change to use the new call
 *
 *************************************************************/
SysAppInfoPtr	SysCurAppInfoPV20(void)
{
	SysAppInfoPtr appInfoP;
	SysAppInfoPtr unusedAppInfoP;
	
	SysGetAppInfo(&appInfoP, &unusedAppInfoP);
	
	return appInfoP;
}
	

/************************************************************
 *
 *  FUNCTION: PrvGetAppInfo
 *
 *  DESCRIPTION: Returns the SysAppInfoPtr for the current app. This
 *   routine assumes that the current application's A5 world has been
 *   setup and that A5 is pointing to it. The SysAppInfoPtr is retrieved
 *   from 0(A5).
 *
 *  PARAMETERS: nothing (A5)
 *				Note that the UInt32 at (A5 & 0x7fffffff) MUST point to an SysAppInfoType!
 *
 *  RETURNS: SysAppInfoPtr for the current thread or NULL
 *
 *  CREATED: 12/16/97
 *
 *  BY: Roger Flores
 *
 *		vmk	2/16/98	Fixed the assembly version so that it doesn't
 *							use A5 as a temporary register, which was causing
 *							the most significant bit of A5 to be unmasked
 *
 *************************************************************/
#if EMULATION_LEVEL != EMULATION_NONE

static SysAppInfoPtr PrvGetAppInfo(void)
{
	return (SysAppInfoPtr)GCurUIAppInfoP;
}

#else

static SysAppInfoPtr asm PrvGetAppInfo(void)
{
	MOVE.L	A5, D0
	BCLR		#31, D0				// clear the high bit to reverse effects of PrvProtectSegmentRegister
	MOVE.L	D0, A0
	MOVE.L	(A0), A0
	RTS
}

#endif


/************************************************************
 *
 *  FUNCTION: SysGetAppInfo
 *
 *  DESCRIPTION: Returns the SysAppInfoPtr for the current app and
 *   for the last current running action code.  This routine uses
 *   A5 and global variables.
 *
 *  PARAMETERS:	rootAppPP - pointer to where to store a SysAppInfoPtr for
 *						  the currently running app
 *						actionCodeAppPP - pointer to where to store a SysAppInfoPtr for
 *						  the currently running action code
 *
 *  RETURNS: rootAppPP is set to the current app's info
 *			 actionCodeAppPP is set the app info used by an action code or else NULL
 *			 The currently running app's appInfo is returned.
 *
 *  CREATED: 12/11/97
 *
 *  BY: Roger Flores
 *
 *************************************************************/
SysAppInfoPtr	SysGetAppInfo(SysAppInfoPtr *rootAppPP, SysAppInfoPtr *actionCodeAppPP)
{
	SysAppInfoPtr currentAppInfoP;
	
	
	ErrNonFatalDisplayIf(rootAppPP == NULL, "Null rootAppPP");
	ErrNonFatalDisplayIf(actionCodeAppPP == NULL, "Null actionCodeAppPP");
	
	currentAppInfoP = PrvGetAppInfo();
	ErrNonFatalDisplayIf(currentAppInfoP == NULL, "Null sys app info");
	if (currentAppInfoP->rootP == NULL)
		{
		// Either the UI app or some other thread is running.  Just use A5.
		*actionCodeAppPP = NULL;
		*rootAppPP = currentAppInfoP;
		}
	else
		{
		// An action code is running
		*rootAppPP = currentAppInfoP->rootP;
		*actionCodeAppPP = currentAppInfoP;
		ErrNonFatalDisplayIf(*actionCodeAppPP == NULL, "Null actionCodeAppP");
		}

	// Validate the return values.
	ErrNonFatalDisplayIf(*rootAppPP == NULL && rootAppPP != actionCodeAppPP, "returning Null uiAppP");
	ErrNonFatalDisplayIf(currentAppInfoP == NULL, "returning Null");
	
#ifndef __UNIX__
	// Structure validation code.
	ErrNonFatalDisplayIf((UInt32) currentAppInfoP->dmAccessP > 0x20000000, "Bad dmAccessP");
	ErrNonFatalDisplayIf((UInt32) (currentAppInfoP->cmdPBP) > 0x20000000, "Bad cmdPBP");
#endif

	// Return the appInfo of the last running app
	return currentAppInfoP;
}
	


/************************************************************
 *
 *  FUNCTION: SysGetStackInfo
 *
 *  DESCRIPTION: Returns the current app's stack bounds as well 
 *  as whether or not the stack is overflowed
 *
 *  PARAMETERS: startPP - where to store the stack's start
 *				endPP - where to store the stack's last byte
 *
 *  RETURNS: true if the stack is valid and false if it's overflowed
 *
 *	This code doesn't correctly MemHandle the case where an action code
 *	allocates it's own stack and then calls another action code.
 *
 *  CREATED: 12/29/97
 *
 *  BY: Roger Flores
 *			mchen	11/5/99	Fixed return check to compare against
 *								stackP instead of stackEndP and fix
 *								the logic of the check.
 *
 *************************************************************/
#if EMULATION_LEVEL != EMULATION_NONE

Boolean SysGetStackInfo(MemPtr *startPP, MemPtr *endPP)
{
	*startPP = NULL;
	*endPP = NULL;
	return true;
}

#else

Boolean SysGetStackInfo(MemPtr *startPP, MemPtr *endPP)
{
	SysAppInfoPtr appInfoP;
	SysAppInfoPtr unusedAppInfoP;
	
	
	appInfoP = SysGetAppInfo(&unusedAppInfoP, &unusedAppInfoP);
	if (!appInfoP->stackP)
		appInfoP = appInfoP->rootP;
		
	*startPP = (MemPtr) appInfoP->stackP;
	*endPP = (MemPtr) appInfoP->stackEndP;
	
	// This could be smartened by only doing this if stackP <= sp <= stackEndP
	return ((appInfoP->stackP != NULL) && (*appInfoP->stackP == 0xFE));
}

#endif


/************************************************************
 *
 *  FUNCTION: SysBroadcastActionCode
 *
 *  DESCRIPTION: Sends the given action code and parameter block to
 *		the latest version of every UI app.
 *
 *  PARAMETERS: 
 *			cmd 		- 	action code to send
 *			cmdPBP	- action code parameter block
 *
 *  RETURNS: 0 if no error.
 *
 *  CREATED: 9/19/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err	SysBroadcastActionCode(UInt16 cmd, MemPtr cmdPBP)
{
	UInt16					index=0;
	Err					err;
	UInt16					cardNo;
	LocalID				dbID;
	UInt32					result;
	Boolean				newSearch=true;
	DmSearchStateType	state;
	
	
	// First, broadcast to all applications
	while(1) {
	
		// Find the next application.
		err = DmGetNextDatabaseByTypeCreator(newSearch, &state, sysFileTApplication,
			0, true, &cardNo, &dbID);
		newSearch = false;
		if (err == dmErrCantFind) {err=0; break; }
		
		// Send the action code
		err = SysAppLaunch(cardNo, dbID, 0, cmd, cmdPBP, &result);
		}
	
	
	// Then, broadcast to all panels
	newSearch = true;
	while(1) {
	
		// Find the next panel.
		err = DmGetNextDatabaseByTypeCreator(newSearch, &state, sysFileTPanel,
			0, true, &cardNo, &dbID);
		newSearch = false;
		if (err == dmErrCantFind) {err=0; break; }
		
		// Send the action code
		err = SysAppLaunch(cardNo, dbID, 0, cmd, cmdPBP, &result);
		}
	
	
	ErrFatalDisplayIf(err, "Error broadcasting action code");
	return	err;
}




/************************************************************
 *
 *  FUNCTION: SysAppLaunch
 *
 *  DESCRIPTION: Given a card number and database ID of an application resource
 *		database, this routine will launch that application with the 
 *		given command line arguments.
 *
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
 *
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
 *		sysAppLaunchFlagNewStack can be set to automatically create a separate 
 *		stack chunk for the application with a size determined by the PREF resource 
 *		of the application.  If the sysAppLaunchFlagNewStack
 *		bit is not set for a nested launch, the launched application will use the
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
 *		will ALWAYS get their own stack regardless of the setting of the 
 *		sysAppLaunchFlagNewStack bit.
 *
 *  If the sysAppLaunchFlagNewGlobals bit is set, the globals world of the application
 *		will be allocated and initialized and register A5 will be set-up to point to the 
 *		globals world. A new globals world implies a new owner ID for memory chunks
 *		allocated by the application. When an application with a globals world quits,
 *		all memory chunks in the dynamic heap which have it's owner ID will be
 *		automatically freed by the system.
 *		
 *
 *  It is expected that an application will be launched with all launch bits
 *		cleared when performing call-back functions to the app, such as when
 *		performing global searches, some syncing functions, etc. This essentially
 *		makes the application a subroutine call from the point of view of the caller.
 *
 *
 *	 Note that a special case launch occurs when we launch the AMX kernel
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
 *		When the task ID is nil we use an ownerID of 0 (system) for memory
 *		manager chunks.
 *
 *	 When an application is launched on the current thread (a Nested Launch),
 *		this procedure doesn't return until the nested application quits.
 *
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
 *
 *
 *	 The Launch process proceeds as follows:
 *	  In SysLaunchApp:
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
 *			the desired stack size is retrieved out of the PREF=0 resource
 *			of the app and a chunk is allocated for the stack.
 *		2.) The CODE=1 resource of the app is located - this is the entry
 *			point to the application's startup code.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
 *			the task ID of the new task is retrieved. If not launching a 
 *			new thread, the task ID of the current task is retrieved.
 *		4.) Cmd and cmdPBP for the new application are stored in the user fields
 *			of the task's TCB for later retrieval by SysAppStartup. If this
 *			is a launch of a new thread, the cmdPBP pointer is also saved in
 *			the rootCmdPBP field of the TCB for eventual disposal by
 *			SysAppExit when the task exits.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
 *			is set to 0. If not a new thread, the current value of launchNestLevel
 *			is incremented. This counter is decremented by SysAppExit and is 
 *			used to determine when the task itself should be deleted and whether or
 *			not the stack chunk and rootCmdPBP chunk should be freed by SysAppExit.
 *		6.) If creating a new thread, the thread is triggerred. If not a new 
 *			thread, the entry point to CODE=1 resource is simple called. 
 *			This transfers control to the application's startup code.
 *
 *	   In Application startup code:
 *		1.) SysAppStartup is called with a copy of the launch flags.
 *		2.) If the sysAppLaunchFlagNewGlobals bit is set, SysAppStartup 
 *			will get the required size of the app globals out of
 *			the CODE=0 resource, allocate and initialize the globals and
 *			sets up register A5 to point to the globals.
 *		3.) SysAppStartup then retrieves the cmd and cmdPBP from the TCB
 *			user info fields of the current task and returns to the
 *			app startup code with the oldA5, a pointer to the globals chunk,
 *			and the cmd and cmdPBP parameters.
 *		4.) The PilotMain routine of the application is called with cmd and cmdPBP.
 *		
 *	   In Application main() routine:
 *		1.) Application runs and returns eventually with 32-bit result code
 *
 *	   Back in Application startup code
 *		1.) If the sysAppLaunchFlagNewGlobals bit was set, the Globals chunk is freed
 *				and A5 is restored.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
 *
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
 *			code will simply return to SysAppLaunch. SysAppLaunch then
 *			frees the chunks used to hold the stack and cmdPBP and returns to 
 *			it's caller.
 *  
 *  CAVEATS: A race condition exists if this is called from a background thread 
 *		to send a launchcode to the current UI app as it is exiting.  If we set up
 *		the subCall flag, then the app finishes exiting, and then we call it's 
 *		PilotMain, the subCall flag in the launchFlags will incorrectly indiciate 
 *		that the application's global variables are available.
 *		
 *  PARAMETERS: 	UInt16		cmd			- application command code
 *						MemPtr		cmdPBP		- application command parameter block
 *						UInt16 		launchFlags	- Launch flags, can be 1 or more of:
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
 * 													¥ sysAppLaunchFlagNewStack
 *														¥ sysAppLaunchFlagNewGlobals 
 *
 *  RETURNS: 0 if no err
 *
 * HISTORY:
 *		02/01/95	ron	Created by Ron Marianetti.
 *		02/17/98	vmk	Added code to fix up the dmAccessList after calls into
 *							PilotMain.
 *		07/07/99	kwk	Load extra stack space from soft constant.
 *		11/03/99	jmp	No longer close sublaunched databases; now, just use
 *							them to help prevent calling chain (e.g., object ID)
 *							conflicts.
 *		11/05/99	mchen Fix so that the sentinel byte (0xFE) is written at the
 *							bottom of the stack (not the end of the stack which is
 *							the top).
 *		11/17/99 jesse	Add a fudge factor to allocated stack space because the 
 *							system uses more stack space than it used to.
 *		07/06/00	kwk	Use TSM feature for FEP extra stack space, versus soft constant.
 *
 *******************************************************************************
 *
 * IMPORTANT!!!!: When launching an app as a separate task, the cmdPBP parameter 
 *		must be allocated as a memory chunk by the caller !! It will be deleted 
 *		for the caller when the application being launched finally quits.
 *
 *************************************************************/
// Function types
typedef UInt32 (*AppEntryPtr)();

Err SysAppLaunch(UInt16 cardNo, LocalID dbID, UInt16 launchFlags,
					UInt16 cmd, MemPtr cmdPBP, UInt32* resultP)
{

#if EMULATION_LEVEL != EMULATION_NONE
	SysAppInfoPtr appInfoP = 0;
	SysAppInfoPtr oldAppInfoP = 0;
	SysAppInfoPtr unusedAppInfoP;
	Err			  err;
	
	
	// Make sure no private flags have been set in the launchFlags
	ErrNonFatalDisplayIf(launchFlags & sysAppLaunchFlagPrivateSet, 
		"Invalid flags passed to SysAppLaunch");
		
	// Clear invalid flags
	launchFlags &= (~sysAppLaunchFlagPrivateSet);

// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
							 sysAppLaunchFlagNewStack |
							 sysAppLaunchFlagNewGlobals |
							 sysAppLaunchFlagUIApp)) {
		ErrDisplay("App launching not supported under emulation mode");
		return sysErrParamErr;
		}
		
	// Allocate an appInfo structure for this app. This structure holds
	//  application specific information like the database MemHandle of the app,
	//  the main code handler, the owner ID, etc.
	appInfoP = MemChunkNew(0, sizeof(SysAppInfoType), memNewChunkFlagNonMovable);
	if (!appInfoP) {
		err = memErrNotEnoughSpace;
		goto Exit;
		}
	oldAppInfoP = SysGetAppInfo(&unusedAppInfoP, &unusedAppInfoP);
	MemMove(appInfoP, oldAppInfoP, sizeof(SysAppInfoType));
	
	if (!appInfoP->rootP)
		appInfoP->rootP = oldAppInfoP;
	
	(SysAppInfoPtr)GCurUIAppInfoP = appInfoP;
	
	// Call PilotMain with selector code
	*resultP = PilotMain(cmd, cmdPBP, launchFlags | sysAppLaunchFlagSubCall);


	//---------------------------------------------------------
	// Restore the previous context (app info)
	//---------------------------------------------------------

	// Clean up action code apps (no globals). In case the action
	//  code app opened or closed databases, mirror the new
	//  dmAccessList into the current app info.	vmk	2/17/98
	if ( !(launchFlags & sysAppLaunchFlagNewGlobals)) 
		oldAppInfoP->dmAccessP = appInfoP->dmAccessP;

	(SysAppInfoPtr)GCurUIAppInfoP = oldAppInfoP;
	
	// Dispose of the appInfo block
	MemPtrFree(appInfoP);

Exit:
	return 0;
	
#else 

	AppEntryPtr				codeP;
	UInt8						*stackP = 0;
	
	MemHandle				prefsH=0;
	SysAppPrefsPtr			prefsP;
	SysAppPrefsType		defPrefs;
	
	UInt16					attributes;
	UInt32					creator;
	UInt32					taskID=0;
	Err						err;
	SysTCBUserInfoPtr		tcbUserP=0;
	SysAppInfoPtr			appInfoP = 0;
	SysAppInfoPtr			rootAppInfoP, curAppInfoP;
	SysAppInfoPtr			saveA5InfoP;


	// Make sure no private flags have been set in the launchFlags
	ErrNonFatalDisplayIf(launchFlags & sysAppLaunchFlagPrivateSet, 
		"Invalid flags passed to SysAppLaunch");
	// Clear invalid flags
	launchFlags &= (~sysAppLaunchFlagPrivateSet);


// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.


	curAppInfoP = SysGetAppInfo(&rootAppInfoP, &appInfoP);
	
	// Allocate an appInfo structure for this app. This structure holds
	//  application specific information like the database MemHandle of the app,
	//  the main code handler, the owner ID, etc.
	appInfoP = MemChunkNew(0, sizeof(SysAppInfoType), memNewChunkFlagNonMovable);
	if (!appInfoP) {
		err = memErrNotEnoughSpace;
		goto Exit;
		}
	MemSet(appInfoP, sizeof(SysAppInfoType), 0);
	
	
	// Put the cmd, cmdPBP, and launch flags in the appInfo for access by
	//  the app startup code
	appInfoP->cmd = cmd;
	appInfoP->cmdPBP = cmdPBP;
	appInfoP->launchFlags = launchFlags;
	
	// Put in pre-opened system databases into the DmAccess List
	//<RM> 1-14-98 
	// was: appInfoP->dmAccessP = rootAppInfoP->dmAccessP;
	appInfoP->dmAccessP = ((SysAppInfoPtr)GSysAppInfoP)->dmAccessP;
	
	
	// Find a free owner ID and store that in the appInfo structure. If we're launching
	//  AMX (the first time this routine is called) taskID will be nil and we'll
	//  use an owner ID of 0
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
	if (taskID && (launchFlags & sysAppLaunchFlagNewGlobals)) {
		appInfoP->memOwnerID = SysNewOwnerID();
		if (!appInfoP->memOwnerID) { err = sysErrOutOfOwnerIDs; goto Exit;}
		}
	else
		appInfoP->memOwnerID = rootAppInfoP->memOwnerID;
	appInfoP->taskID = taskID;
		

	// Open up the database and get the creator ID
	appInfoP->dbP = (MemPtr)DmOpenDatabase(cardNo, dbID, dmModeReadOnly);
	if (!appInfoP->dbP) {
		err = DmGetLastErr();
		goto Exit;
		}
	DmDatabaseInfo(cardNo, dbID, 0,
			&attributes, 0, 0,
			0, 0,
			0, 0,
			0, 0,
			&creator);
	
	// If this is not a resource database, return with an error.  Otherwise, the rest
	// of the SysAppLaunch code will run but will use the first available resource DB
	// in the open database list, which will result in unpredictable behavior.
	if ( (attributes & dmHdrAttrResDB) == 0) {
		err = sysErrParamErr;
		ErrNonFatalDisplay("Can't launch record database.");
		goto Exit;
	}
	
	// If this is the database of the currently running application, we set the
	//  sysAppLaunchFlagSubCall flag in the launch flags which indicates that
	//  we can keep the A5 world valid
	if (!(launchFlags & sysAppLaunchFlagNewGlobals)) {
		LocalID curAppDBID;
		UInt16 curAppCardNo;
		
		// Get the cardNo and dbID of the currently running app and if it matches,
		//  set the sysAppLaunchFlagSubCall bit in the launch flags.
		if (rootAppInfoP != NULL && rootAppInfoP->dbP) {
			err = DmOpenDatabaseInfo(rootAppInfoP->dbP, &curAppDBID, 0, 0, &curAppCardNo, 0);
			if (!err && curAppDBID == dbID && curAppCardNo == cardNo) {
				launchFlags |= sysAppLaunchFlagSubCall;
				appInfoP->launchFlags |= sysAppLaunchFlagSubCall;
				}
			}
		}
			
			
	// Get the resource which contains the application preferences
	prefsP = &defPrefs;
	prefsH = DmGet1Resource(sysResTAppPrefs, sysResIDAppPrefs);
	if (prefsH) {
		MemMove(prefsP, MemHandleLock(prefsH), sizeof(SysAppPrefsType));
		MemHandleUnlock(prefsH);
		DmReleaseResource(prefsH);
		}
	else {
		prefsP->priority = 30;
		prefsP->stackSize = 0xD00;
		prefsP->minHeapSpace = 0x1000;
		}
	if (!(launchFlags & sysAppLaunchFlagNewStack)) 
		prefsP->stackSize = 0;


	// Allocate space for the stack if specified
	if (prefsP->stackSize) {
		UInt32 fepStackExtra;
		
		// Make sure all old apps have a stack size large enough to receive IR transmissions.
		if ((launchFlags & sysAppLaunchFlagUIApp) && (prefsP->stackSize < 0xD00)) {
			prefsP->stackSize = 0xD00;
			}
		
		// Add in extra space (if any) required by non-English locales. For example,
		// with Japanese the FEP and sorting code need extra space.
		// DOLATER kwk - it would be faster to stash this in a low-mem global at
		// SysInit time.
		if (FtrGet(tsmFtrCreator, tsmFtrNumFepStackExtra, &fepStackExtra) == errNone) {
			prefsP->stackSize += fepStackExtra;
			}
			
		// Add a fudge factor because the system uses more stack space than it used to.
		prefsP->stackSize += sysAppStackSpaceFudgeFactor;
		
		appInfoP->stackP = stackP = MemChunkNew(0, prefsP->stackSize, memNewChunkFlagNonMovable);
		if (!appInfoP->stackP) {
			err = memErrNotEnoughSpace;
			goto Exit;
			}
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
		MemSet(stackP, prefsP->stackSize, 0xFE);
#endif
		appInfoP->stackEndP = stackP + prefsP->stackSize - 1;

		// write a sentinel byte to the "bottom" of the stack
		*appInfoP->stackP = 0xFE;
		}


	//-----------------------------------------------------------------
	// Get the code resource of the "new" app, lock it down
	//  and jump to it.
	//
	// We used to run the code that is commented out just below
	//  because the database was already opened, so why open it
	//  again?  Well, that turns out to be a problem because we
	//  may need for the newly sublaunched app's data to be on top
	//  of the data calling chain; otherwise, conflicts in object
	//  IDs are more likely to occur.
	//-----------------------------------------------------------------
	//if (!(launchFlags & sysAppLaunchFlagSubCall)) {
		// Get the pointer to the entry point of code #1 resource
		appInfoP->codeH = DmGet1Resource(sysResTAppCode, 1);
		if (!appInfoP->codeH) {
			err = DmGetLastErr();
			goto Exit;
			}
		else 
			codeP = (AppEntryPtr)((MemPtr)MemHandleLock(appInfoP->codeH));
	//	}
	
	// For Subcalls, copy database info from the app's current appInfoP
	//  In subcall cases, we don't need another open copy of the executable
	//  so we close that and copy the dmAccess list and the dbP
	//  from the app's current appInfoP. SysAppExit() is smart enough
	//  NOT to close databases when it sees the SubCall launch flag.
	//else {
	//	DmCloseDatabase(appInfoP->dbP);
	//	appInfoP->dbP = rootAppInfoP->dbP;		
	//	appInfoP->codeH = rootAppInfoP->codeH;

	//	codeP = (AppEntryPtr)((MemPtr)MemHandleLock(appInfoP->codeH));
	//	MemPtrUnlock(codeP);
	//	}


	//-----------------------------------------------------------------
	// Set up the open database list appropriately. If this is an 
	//  action code call (like a subroutine call), indicated
	//  by no globals, then the action code app gets the current
	//  database list. Otherwise, it simply gets the generic
	//  system list but with the new apps database moved into it. 
	// <RM> 1-26-98
	//-----------------------------------------------------------------
	if (launchFlags & sysAppLaunchFlagNewGlobals)
		DmMoveOpenDBContext((DmAccessPtr*)(&appInfoP->dmAccessP), 
					(DmAccessPtr)appInfoP->dbP);
	else
		appInfoP->dmAccessP = curAppInfoP->dmAccessP;


	//=============================================================
	// If launching as a separate task, create the task now
	//=============================================================
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
		if (err) {
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
			goto Exit;
			}
			
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
		appInfoP->taskID = taskID;
		
		tcbUserP->tmpAppInfoP = appInfoP;
		tcbUserP->rootAppInfoP = appInfoP;
		
		// We must store the current A5 pointer in the TCB because AMX doesn't
		//  set A5 for us when it creates a new task. This A5 is initialy used to
		//  get the current context and thus the list of open databases which includes
		//  the database of the app we're launching. Later on in SysAppStartup, the
		//  open database will be moved into the task's own A5 world context.
		tcbUserP->initialA5 = PrvSysGetA5();

// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
		
			
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
		
		// no result code
		*resultP = 0;
		
		// Return to caller
		return 0;
		}


	//=============================================================
	// Else, just call the app like a subroutine call, but substituting 
	//  new stack pointer if specified.
	//=============================================================
	else {
		UInt32		oldA5;
		MemPtr			rootA5;
		SysAppInfoPtr oldAppInfoP;
	
		// It's possible that AMX isn't launched yet, so we must use 
		//  the low memory global SysAMXAppInfoP to pass the appInfo
		//  pointer to AMX since the TCB structure is not available.
		if (taskID) {
		
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.

			// Use the TCB User info structure to hold the pointer to the 
			//  appInfo structure temporarily, until the app's startup code
			//  runs and retrieves it.
			tcbUserP->tmpAppInfoP = appInfoP;
			
			// Set up the root pointer since this isn't the first call for the thread.
			if (!(launchFlags & sysAppLaunchFlagUIApp)) {
				oldAppInfoP = PrvGetAppInfo();
				if (oldAppInfoP->rootP == NULL)
					appInfoP->rootP = oldAppInfoP;
				else
					appInfoP->rootP = oldAppInfoP->rootP;
				}
			}
			
		// If launching amx, save the pointer to the AMX appinfo structure in
		//  a low memory global specially reserved for it.
		else
			{
			// A lot of code uses GSysAppInfoP to get to the system's DB access list.
			// Most of this runs as the AMX task.  Make sure this code still get's the
			// correct DB access list.
			GSysAMXAppInfoP = (MemPtr)appInfoP;
			GSysAppInfoP = GSysAMXAppInfoP;
			}


		//---------------------------------------------------------
		// Set up the segment register
		//---------------------------------------------------------
		oldA5 = PrvSysGetA5();
		if (launchFlags & sysAppLaunchFlagSubCall)
			{
			// Since we're going back to a running app, set A5 to it's old A5.
			// <RM> 1-14-98
			rootA5 = (MemPtr)appInfoP->rootP->a5Ptr;		// This is setup by SysAppStartup()
			ErrFatalDisplayIf(!rootA5, "nil A5 world in root app!");
			
			// The app being launched is already running.  Reuse the root's A5
			// so that the subcalled app can use the global variables setup
			// by the root app. But, point the old A5 to the new appInfoP
			// because we might need it for cases where there is a new stack,etc.
			saveA5InfoP = *(SysAppInfoPtr*)rootA5;
			*(SysAppInfoPtr*)rootA5 = appInfoP;		
			SysSetA5((UInt32) rootA5);			
			}
		else
			// (A5 & 0x7fffffff)[0] must point to the appInfo.  Use the copy on the stack.
			SysSetA5((UInt32) PrvProtectSegmentRegister(&appInfoP));


		//---------------------------------------------------------
		// Call the App.
		//---------------------------------------------------------
		if (stackP) {
			*resultP = PrvCallWithNewStack((ProcPtr)codeP, stackP+prefsP->stackSize);
			MemPtrFree(stackP);
			}
		else
			*resultP = PrvCallSafely((ProcPtr)codeP);  // make sure our registers are restored


		//---------------------------------------------------------
		// Clean up subcalled apps.  Note that the SysAppInfoType structure for a 
		// subcalled app is referenced only by this routine.  That is why this cleanup
		// must be done here.
		//---------------------------------------------------------
		if (launchFlags & sysAppLaunchFlagSubCall) {
			// <RM> 1-14-98 
			// Restore the appInfoP pointed to by A5
			*(SysAppInfoPtr*)rootA5 = saveA5InfoP;
			}
			
		
		//---------------------------------------------------------
		// Clean up action code apps (no globals). In case the action
		//  code app opened or closed databases, mirror the new
		//  dmAccessList into the current app info.
		//---------------------------------------------------------
		if ( !(launchFlags & sysAppLaunchFlagNewGlobals)) 
			curAppInfoP->dmAccessP = appInfoP->dmAccessP;
			

		//---------------------------------------------------------
		// Free up the appInfo now
		// <RM> 1-26-98 
		//---------------------------------------------------------
		MemPtrFree(appInfoP);

	
		// Restore the A5 value of the caller.
		SysSetA5(oldA5);
		
		// clear err out 	
		err = 0;
		}


	//==================================================================
	// We only get here if an error occurred, or if we're returning from a 
	//  nested launch. If there was no error, the app structures will have been
	//  freed by SysAppExit which is called by the app after it quits.
	//==================================================================
Exit:

	if (err) {
		// Free the cmd paramBlock chunk
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
			MemPtrFree(cmdPBP);
			
	
		// Relese chunks already allocated and stored in the appInfo 
		if (appInfoP) {
			
			// Unlock the code resource and release it
			if (appInfoP->codeH) {	
				MemHandleUnlock(appInfoP->codeH);
				DmReleaseResource(appInfoP->codeH);
				}
				
			// Free the stack chunk
			if (appInfoP->stackP) MemPtrFree(appInfoP->stackP);
			
			// Close the application database
			if (appInfoP->dbP) DmCloseDatabase(appInfoP->dbP);
	
			// Dispose of the appInfo block
			MemPtrFree(appInfoP);
			}
		}
		
	return err;
	
#endif //EMULATION_LEVEL != EMULATION_NONE
}


/************************************************************
 *
 *  FUNCTION: SysAppStartup
 *
 *  DESCRIPTION: This is the routine called by application's startup
 *		code. It sets up the globals for an application and returns
 *		the command line arguments, if any.
 *
 *   The structure of an application is as follows:
 *
 *		The entry point to the application is the 4th byte of the
 *		CODE #1 resource.
 *
 *		The first long word of CODE #0 is the number of bytes to reserve
 *		above A5
 *
 *		The second long word of CODE #0 is the number of bytes to reserve
 *		below A5
 *
 *		DATA #0 contains a compressed version of the initialized application
 *		globals.
 *
 *		The argc and argv for the app have been stuffed into the user fields
 *		of the TCB for the current task by SysAppLaunch.
 *
 *  PARAMETERS: MemPtr to return argc, argv
 *
 *  RETURNS: previous A5 in *oldA5P, pointer to allocated globals in *globalsP.
 *			0 if no error
 *
 *  CREATED: 2/1/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err SysAppStartup(SysAppInfoPtr* appInfoPP, MemPtr* prevGlobalsPtrP, 
							MemPtr* globalsPtrP)
{
	MemHandle				codeH;
	UInt32 *				sizeP;
	UInt32				above, below;
	UInt8 *				globalsP;
	
	Char *				cP;
	MemHandle				dataH;
	UInt8 *				globalRegP;
		
	SysTCBUserInfoPtr	tcbUserP=0;
	UInt32				taskID;
	SysAppInfoPtr		appInfoP;
	
	UInt16				launchFlags;
	UInt8 * 				codeP=0;		// important to clear this!
	UInt8 * 				dataP;
	UInt8 * 				endOfDataP;
	
	
	// Return the appInfo for this app by retreiving it out of the TCB for the task.
	// SysAppLaunch uses the field tmpAppInfoP of this structure as a convenient
	// temporary holding space for the pointer
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
	if (taskID) {
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
		*appInfoPP = appInfoP = tcbUserP->tmpAppInfoP;
		launchFlags = appInfoP->launchFlags;
		
		// If we need new globals, initialize A5 to temporarily point
		// to our appInfoP on the stack so that Database calls can find
		//  the list of open databases
		// <RM> 1-14-98
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
		//			SysSetA5(tcbUserP->initialA5);
		if (launchFlags & sysAppLaunchFlagNewGlobals)
			SysSetA5((UInt32)PrvProtectSegmentRegister(&appInfoP));
		}
	// The only time this happens is when we're launching AMX....
	else {
		*appInfoPP = appInfoP = (SysAppInfoPtr)GSysAMXAppInfoP;
		launchFlags = appInfoP->launchFlags;
		}
	
	ErrNonFatalDisplayIf(appInfoP == NULL, "NULL appInfoP");
	
	
	// If we need to create an A5 world.....
	if (launchFlags & sysAppLaunchFlagNewGlobals) {
	

		// Get the CODE #0 resource
		codeH = DmGet1Resource(sysResTAppCode, 0);
		
		// Did we find the CODE #0 resource?
		if (codeH) {

			// the first 2 long words are the # of bytes needed above A5 and below A5
			sizeP = (UInt32 *)MemHandleLock(codeH);
			above = sizeP[0];
			below = sizeP[1];
			
			// Release the CODE=0 resource
			MemPtrUnlock(sizeP);
			DmReleaseResource(codeH);
			}

		// No CODE #0 resource was found - no biggie!
		else {
		
			// Just allocate enough globals space for appInfoP
			above = 4;
			below = 0;
			}

		
		// Allocate space for globals
		// for normal application globals, we will allocate space in a locked movable
		// block.  This is to handle the case where the application loads some persistent
		// library that in turn allocates ptr memory.  The app globals would leave a
		// free memory hole when the app is switched if it was allocated in ptr memory.
		// We check for the special case where we are actually allocating the kernel
		// globals, which never go away.  In that case, we just allocate ptr memory.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
			// kernel globals
			globalsP = MemChunkNew(0, above + below, memNewChunkFlagNonMovable);
			}
		else {
			// normal app globals
			globalsP = MemChunkNew(0, above + below, memNewChunkFlagPreLock);
			}
		if (!globalsP) return memErrNotEnoughSpace;
		*globalsPtrP = (MemPtr)globalsP;
		
		
		// Save MemPtr to globals chunk in AppInfo for identifcation by the Debugger
		//  when doing a heap dump
		appInfoP->globalsChunkP = globalsP;
		appInfoP->globalEndP = (globalsP + (above + below - 1));
		
		
		// Clear the globals
		cP = (Char *)globalsP;
		MemSet(cP, above+below, 0);
			
		// Create pointer to globals
		globalRegP = globalsP + below;


		// Store the appInfo structure pointer at 0(A5) for quick
		//  access by the Memory Manager when setting owner id's.
		*((SysAppInfoPtr*)globalRegP) = appInfoP;


		// Set A5 to point to the new globals world
		*prevGlobalsPtrP = (MemPtr)SysSetA5((UInt32)globalRegP);
		
		
		// Store the value of A5 in the sysAppInfoType. This is used by SysAppLaunch
		//  when it detects that an app that already has a globals world
		// is being called again due to an action code and we need to set it's A5
		//  up appropriately. We can't use the current A5 at the time because it
		//  might be the fake A5 of another app's action code context. 
		appInfoP->a5Ptr = globalRegP;
		
		// Get the DATA #0 resource
		dataH = DmGet1Resource(sysResTAppGData, 0);

		// Did we find the DATA #0 resource?  If not, don't sweat it!
		if (dataH) {

			// Lock it down and calculate the end
			dataP = MemHandleLock(dataH);
			endOfDataP = dataP + MemPtrSize(dataP);
			
			// DATA resource looks like this:
			// +---------------------------------+
			// | long:   offset of CODE 1 xrefs  |---+
			// +---------------------------------+   |
			// | Char[]: compressed init data    |   |
			// +---------------------------------+   |
			// | Char[]: compressed DATA 0 xrefs |   |
			// +---------------------------------+   |
			// | Char[]: compressed CODE 1 xrefs |<--+
			// +---------------------------------+

			// Get the initialized data and decompress it
			// [skip first long of data 0 (unneeded offset)]
			dataP = PrvDecompressData(dataP + 4, (UInt8 *) globalRegP);

			// dataP now points to DATA 0 xrefs

			// It's probably worth noting that the usual Metrowerks startup code (Startup.c) calls
			//  __relocate__ which in turn calls __reloc_compr__ (PrvRelocateData).  So far, I've
			// only seen code snippets that require the first two calls (below) to PrvRelocateData.

			// NOTE: Since at least one third-party build environment (Pila) only includes
			// the compressed init data in their DATA 0 resource, we only perform the
			// relocation if we haven't yet hit the end of the DATA 0 resource.

			if (dataP < endOfDataP) {

				// Get the CODE #1 resource (for xrefs)
				codeP = MemHandleLock(appInfoP->codeH);

				// Relocate DATA segment references to DATA segment
				dataP = PrvRelocateData(dataP, (UInt8 *)globalRegP, (Int32) globalRegP);

				// Relocate DATA segment references to CODE segment 1
				if (dataP < endOfDataP) {
					dataP = PrvRelocateData(dataP, (UInt8 *)globalRegP, (Int32) codeP);
					}
				}

			// We shouldn't really have to do the remaining relocations because:
			//		(A) Pilot code lives in a single segment (there's no segment loader)
			//		(B) Data references from code segment 1 are via A5
			//		(C) Code segment 1 references from code segment 1 are PC-relative
			//		(D) The last relocate is redundant
			//
			// It might be worth checking with Metrowerks to see if there's any reason any
			// of these four blocks would actually be used in a Pilot application build.
			// Remember that Emulator apps use MW's startup code, NOT this code.

#if 0		// Don't need this (at this time).  See comments immediately above.

			// Relocate DATA segment references to same CODE segment
			if (dataP < endOfDataP) {
				dataP = PrvRelocateData(dataP, globalRegP, (Int32) globalRegP);

				// Relocate CODE segment 1 references to DATA segment
				if (dataP < endOfDataP) {
					dataP = PrvRelocateData(dataP, codeP, (Int32) globalRegP);

					// Relocate CODE segment 1 references to CODE segment 1
					if (dataP < endOfDataP) {
						dataP = PrvRelocateData(dataP, codeP, (Int32) codeP);
					}

					// Relocate CODE segment 1 references to same CODE segment
					if (dataP < endOfDataP) {
						PrvRelocateData(dataP, codeP, (Int32) codeP);
						}
					}
				}
#endif

			MemHandleUnlock(dataH);
			DmReleaseResource(dataH);

			// Unlock the CODE #1 resource -- Don't panic, we were CALLED by CODE #1.
			// SysAppLaunch locked it before calling it.  This was a secondary lock to
			// get a dereferenced pointer to appInfoP->codeH.

			// NOTE: We only Unlock this if we actually locked it (for DATA 0 xrefs)
			if (codeP) MemPtrUnlock(codeP);

			}	// end if (dataH)

		// Flag that the data has been relocated
		// (or at least that our new 2.0 SysAppStartup code has executed)
		appInfoP->launchFlags |= sysAppLaunchFlagDataRelocated;


		// If this is a UI app, reset the UI state.
		if (launchFlags & sysAppLaunchFlagUIApp) {
			UIReset();
			}

		}	// end if (launchFlags & sysAppLaunchFlagNewGlobals)

	else {

		// <12/7/99 SCL> Return current A5 in prevGlobalsPtrP
		*prevGlobalsPtrP = (MemPtr)PrvSysGetA5();

		}

	
	// Return to caller
	return 0;
}



/************************************************************
 *
 * FUNCTION:	PrvFindMemoryLeaks
 *
 * DESCRIPTION: Scans a heap and throws an error when memory owned by 
 *		the app is found.  Although PalmOS protects against app caused memory leaks
 *		by deleting the chunks, memory leaks are often signs of problems.
 *		Often these leaks are cumulative, which can crash the device when
 *		the dynamic heap becomes full.
 *
 * PARAMETERS:
 *		ownerID	 ->	owner ID of chunks to consider as leaks
 *
 * RETURNS:	nothing
 *
 * HISTORY:
 *		07/14/98	rsf	Modified from MemHeapFreeByOwnerID
 *		11/29/00	kwk	Trimmed message so that ErrDisplayFileLineMsg
 *							won't truncate it; added "Minor error" note.
 *
 *************************************************************/
 
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
static void  PrvFindMemoryLeaks(UInt16 ownerID)
{
	MemHeapHeaderPtr		heapP;
	MemChunkHeaderPtr		chunkP;
	UInt32					size;
	
	// NOTE: This is a rather large array to put on the stack, but
	// we know this routine is called after an app exits and presumably
	// all apps need at least 128 bytes of stack space total in order to
	// run so we know we have a lot of stack space at this point. 
	Char						msg[128];

	// Wait for ownership
	MemSemaphoreReserve(false);

	// Get a near heap pointer from the heapID
	heapP = MemHeapPtr(memDynamicHeaps - 1);

	// Walk the heap and look for chunks with the given owner ID
	chunkP = (MemChunkHeaderPtr)((UInt8 *)heapP + sizeof(MemHeapHeaderType)
					+ (heapP->mstrPtrTbl.numEntries << 2) );

	while((size = chunkP->size) != 0) {
		if (chunkP->owner == ownerID)
			{
			StrPrintF(msg, "Minor error found while exiting app, un-freed chunk at 0x%lx (ownerID %d)", chunkP + 1, ownerID);
				
			ErrNonFatalDisplay(msg);
			break;
			}

		// Go on to next chunk
		chunkP = (MemChunkHeaderPtr) ((UInt8 *)chunkP + size);
		}
		
		
	// Release ownership
	MemSemaphoreRelease(false);
}

#else

#define PrvFindMemoryLeaks(ownerID)

#endif


/************************************************************
 *
 *  FUNCTION: PrvFindOpenDatabases
 *
 *  DESCRIPTION: Checks if the application left any databases
 *  open. Databases left open with dmModeLeaveOpen don't
 *  count. If this returns true, we report an error.
 *
 *  PARAMETERS: none
 *
 *  RETURNS: whether any databases were erroneously left open
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			dje	9/15/00	Created
 *
 *************************************************************/
 
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
static void PrvFindOpenDatabases(void)
{
	DmOpenRef			dbR, nextDBR;
	Err					err;
	UInt16				mode, openCount;
	LocalID				dbID;
	UInt16				cardNo;
	
	dbR = DmNextOpenDatabase(NULL);
	while (dbR)
		{
		nextDBR = DmNextOpenDatabase(dbR);
		err = DmOpenDatabaseInfo(dbR, &dbID, &openCount, &mode, &cardNo, NULL);
		if (!err && !(mode & dmModeLeaveOpen) && openCount)
			{
			DmAccessPtr dbP = (DmAccessPtr)dbR;
			UInt32 type;
			
			// Check if this DmOpenRef is an overlay that was opened as a side effect
			// of opening a base. We don't report these because they should be closed
			// when the (following) base is closed. (We do report the base, however.)
			if (dbP->openType != openTypeOverlay)
				{
				Char name[dmDBNameLength];
				
				DmDatabaseInfo(cardNo, dbID, name, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &type, NULL);
				
				// Check if the database is an application. When the Metrowerks debugger
				// installs an application, it keeps the database open and the code
				// resources locked so that it can map between memory addresses and
				// C instructions. So we don't report application databases left open.
				if (type != sysFileTApplication)
					{
					Char msg[80];		// plenty of room
					
					StrPrintF(msg, "Database left open: %s", name);
					ErrNonFatalDisplay(msg);
					}
				}
			}
		dbR = nextDBR;
		}
}

#else

#define PrvFindOpenDatabases()

#endif


/************************************************************
 *
 *  FUNCTION: SysAppExit
 *
 *  DESCRIPTION: This is the routine called by application's startup
 *		code after an app launched in a separate thread exits.
 *
 *		If we're exiting from an App that was launched as a nested launch,
 *		we unlock the code segment, release the code segment, close the
 *		application database, and return. The SysAppLaunch routine will
 *		dispose of the stack chunk after we return. The caller who launched
 *		the app as a nested launch is responsible for disposing the command
 *		parameter block.
 *
 *		If we're exiting from an App on it's own thread, there's no one to
 *		return to so this routine will dispose of the stack chunk
 *		and the rootCmdPB block and delete the task from AMX. AMX will not 
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
 *		as soon as this thread is deleted.
 *
 *  PARAMETERS: result code
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 2/1/95 
 *
 *  BY: Ron Marianetti
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	7/20/98	Added PrvFindMemoryLeaks.
 *			jmp	11/03/99	Always do database cleanup now, regardless of sublauch status.
 *			dje	9/18/00	Fix bugs 41887 & 42462.
 *
 *************************************************************/
Err SysAppExit(SysAppInfoPtr appInfoP, MemPtr prevGlobalsP, MemPtr globalsP)
{
	Err					err;
	UInt32				ownerBit;
	UInt32				taskID;
	DmOpenRef			dbP, nextDBP;
	UInt16				mode, openCount;
	UInt16				status;
	SysAppInfoPtr		pendingStackFreeP;
	UInt32				VFSMgrVersion;
	
	
	// Close all files owned by the exiting application.  For PalmOS 4.0, this function just
	// closes all files not opened with vfsModeLeaveOpen.
	err = FtrGet( sysFileCVFSMgr, vfsFtrIDVersion, &VFSMgrVersion );
	if(!err && appInfoP->memOwnerID
		&& (appInfoP->launchFlags & sysAppLaunchFlagNewGlobals))
	{
		VFSPrvCleanupFiles(appInfoP->memOwnerID);
	}
	
	// Go through and dispose of any memory chunks allocated by this application.
	//  The call to MemHeapFreeByOwnerID also compacts the heap
	// <chg 5-8-98 RM> Changed test to: appInfoP->launchFlags & sysAppLaunchFlagNewGlobals
	//   from rootP==NULL so that it works correcty for sublaunched apps
	//   that get their own globals. 
	if (appInfoP->memOwnerID && (appInfoP->launchFlags & sysAppLaunchFlagNewGlobals)) {
		
		// Report any memory leaks
		PrvFindMemoryLeaks(appInfoP->memOwnerID);
		
		// Clean up any memory leaks.
		MemHeapFreeByOwnerID(0, appInfoP->memOwnerID);

		// Return this owner ID to the free pool
		ownerBit = 1 << appInfoP->memOwnerID;
		GSysOwnerIDsInUse &= ~ownerBit;
		}
		
	
	// No longer check to see if this is a sublaunch or not.  This is because
	//  we no longer close a sublaunched root application's database due
	//  to calling-chain problems.  So, we must now correspondingly ALWAYS
	//  close the database.  Just commented out the previous code for easy
	//  reference.
	//
	// If this was not a sublaunch (a running app being called 
	//  due to a nested action code), close all opened databases. For sub-launches,
	//  the app gets a copy of it's already opened database when it's called.
	//if (! (appInfoP->launchFlags & sysAppLaunchFlagSubCall)) {

		// Free up allocated memory chunks for running this app
		if (appInfoP->codeH) {
			MemHandleUnlock(appInfoP->codeH);
			DmReleaseResource(appInfoP->codeH);
			}
		
		// Close the application's resource database.
		if (appInfoP->dbP)
			DmCloseDatabase(appInfoP->dbP);
			
			
		// If this app had it's own A5 world, Go through and close all databases
		//  left open by the application.
		if (appInfoP->launchFlags & sysAppLaunchFlagNewGlobals) {
			// Report any databases left open.
			PrvFindOpenDatabases();
			
			// Clean up any databases left open.
			dbP = DmNextOpenDatabase(0);
			while(dbP) {
				nextDBP = DmNextOpenDatabase(dbP);
				err = DmOpenDatabaseInfo(dbP, 0,
											&openCount, &mode, 0, 0);
				if (!err && !(mode & dmModeLeaveOpen) && openCount)
					{
					// dje 9/19/00 - Fix bug #41887. Check if this DmOpenRef is an overlay
					// that was opened as a side effect of opening a base. We don't close
					// these because they'll get closed when the (following) base is closed.
					if (((DmAccessPtr)dbP)->openType != openTypeOverlay)
						// dje 9/18/00 - Fixed bug #42462. We used to DmCloseDatabase
						// openCount times even though we enumerate a DmOpenRef for
						// each openCount.
						// while (openCount--)
						DmCloseDatabase(dbP);
					}
				dbP = nextDBP;
				}
				
				
			// If this app wanted any databases left open, include them in the
			//   system list
			dbP = DmNextOpenDatabase(0);
			if (dbP) ((SysAppInfoPtr)GSysAppInfoP)->dmAccessP = (MemPtr)dbP;
			}
	//	}


	// Free the globals chunk (if allocated) and restore A5
	if (appInfoP->launchFlags & sysAppLaunchFlagNewGlobals) {
		MemPtrFree(globalsP);
		SysSetA5((UInt32)prevGlobalsP);
		}


	// If this application was on it's own thread, delete the cmd parameter
	//  block for the caller, and delete the AMX task
	//  associated with the app.  
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
	
		if (appInfoP->cmdPBP)
			MemPtrFree(appInfoP->cmdPBP);

		taskID = appInfoP->taskID;

		// <chg 11/19/98 TLW> Link appInfo block onto chain of to-be-deleted task resources...
		// Coordinate when the appInfo block and stack can be deleted with task delete
		// routine (see TaskTerminationProc) and finally delete the appInfo block and stack
		// during idle loop (See PrvHandleEvent).
		
		// Use the extraP pointer as a next pointer to link them together
		appInfoP->extraP = NULL;

		// Disable interrupts
		status = SysDisableInts();

		// First entry
		pendingStackFreeP = (SysAppInfoPtr)GSysPendingStackFrees;

		// Link to head
		if (pendingStackFreeP == NULL)
			GSysPendingStackFrees = (MemPtr)appInfoP;
		
		// Or append to last
		else {
			while (pendingStackFreeP->extraP)
				pendingStackFreeP = (SysAppInfoPtr)pendingStackFreeP->extraP;
			pendingStackFreeP->extraP = (MemPtr)appInfoP;
			}
		
		// Restore interrupt state
		SysRestoreStatus(status);

 		// Delete the task - this call never returns.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
		if (err) DbgBreak();
		}
			
			
	// If not on our own thread just return to SysAppLaunch.	
	// SysAppLaunch will free the appInfo for us. 
	else 	{
		#if EMULATION_LEVEL != EMULATION_NONE
		(SysAppInfoPtr)GCurUIAppInfoP = appInfoP->rootP;
		#endif
		}
	
	return 0;
}




/************************************************************
 *
 *  FUNCTION: SysNewOwnerID
 *
 *  DESCRIPTION: Returns a new owner ID for use by an application
 *   This owner ID gets placed into all Memory chunks allocated by 
 *		the application.
 *
 *  PARAMETERS: void
 *
 *  CALLED BY: SysAppStartup when allocating globals for the app
 *
 *  RETURNS: 0 if no more owner IDs available
 *
 *  CREATED: 4/5/96
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
UInt16		SysNewOwnerID(void)
{
	UInt32		bits;
	UInt16		owner;
	
	bits = GSysOwnerIDsInUse;
	
	// Find a clear bit starting from bit 1
	owner = 1;
	bits >>= 1;
	
	// Owner IDs can be from 1 to 15
	for (owner=1; owner<=15; owner++) {
		if (!(bits & 0x01)) {
			GSysOwnerIDsInUse |= (1L << owner);
			return owner;
			}
		bits >>= 1;
		}
		
	// If we fell out, no more
	ErrFatalDisplay("No More Owner IDs");
	return 0;
}




/************************************************************
 *
 *  FUNCTION: PrvSysGetA5
 *
 *  DESCRIPTION: This routine gets the value of A5, without
 *						having to do two calls to SysSetA5, and
 *						more importantly, without having to change
 *						A5.  Code used to call SysSetA5(0).
 *
 *  PARAMETERS: result code
 *
 *  RETURNS: previous value of A5
 *
 *  CREATED: 7/2/98 
 *
 *  BY: Steve Lemke
 *
 *************************************************************/
#if defined(__MC68K__)
static asm UInt32 PrvSysGetA5(void)
{
	MOVE.L	A5, D0							// get old value
	RTS
}
#elif defined(__POWERPC__) || defined(__INTEL__) || defined(__UNIX__)
static UInt32 PrvSysGetA5(void)
{
	ErrDisplay("PrvSysGetA5 unsupported on PPC");
	return 0;
}
#else
#error "Processor type not defined"
#endif



/************************************************************
 *
 *  FUNCTION: SysSetA5
 *
 *  DESCRIPTION: This routine sets the value of A5
 *
 *  PARAMETERS: result code
 *
 *  RETURNS: previous value of A5
 *
 *  CREATED: 2/1/95 
 *
 *  BY: Ron Marianetti
 *
 *  REVISION HISTORY:
 *			Name		Date		Description
 *			----		----		-----------
 *			ron		2/1/95	Initial Revision
 *			jmp		9/25/99	Return zero if we're in the simulator.  68K QuickDraw
 *									doesn't like A5 changed while it's doing its thing!
 *			jmp		01/24/00	Ditto for non-68K environments.
 *
 *************************************************************/
#if defined(__MC68K__)
asm UInt32 SysSetA5(UInt32 newValue)
{
	fralloc +									// setup stack frame
#if EMULATION_LEVEL == EMULATION_NONE
	MOVE.L	A5, D0							// get old value
	MOVE.L	newValue, A5					// set new value
#else
	Moveq.l	#0,D0								// Do nothing when we're in the simulator.							
#endif
	frfree										// clean up stack frame
	RTS
}
#elif defined(__POWERPC__) || defined(__INTEL__) || defined(__UNIX__)
UInt32 SysSetA5(UInt32 newValue)
{
#if EMULATION_LEVEL == EMULATION_NONE
	ErrNonFatalDisplay("SysSetA5 only supported on 68K");
#endif
	return 0;
}
#else
#error "Processor type not defined"
#endif



/************************************************************
 *
 *  FUNCTION: SysLibInstall
 *
 *  DESCRIPTION: Installs a library into the Library table
 *			and calls the librarie's install entry point.
 *
 *  PARAMETERS: 
 *			libraryP - pointer to library 
 *			*refNumP - return refNum of library
 *
 *  CALLED BY: Applications that need to install a library
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 6/23/95
 *
 *  BY: Ron Marianetti
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			SCL	11/19/98	Fixed MemMove reading past end of source
 *
 *************************************************************/
Err	SysLibInstall(SysLibEntryProcPtr libraryP, UInt16 * refNumP)
{
	UInt16						refNum;
	Err						err;
	SysLibTblEntryPtr		entryP;
	SysLibTblEntryPtr		tableP;
	UInt32						reqSize;
	MemPtr						oldTableP;
	
	
	// Get pointer to library table
	tableP = (SysLibTblEntryPtr)GSysLibTableP;
	
	// Init return refNum
	*refNumP = sysInvalidRefNum;								// vmk 8/16/96
		
	// Look for a free entry, but don't use the refNum reserved
	//  for use by the debugger comm library.
	for (refNum=0; refNum<GSysLibTableEntries; refNum++) {
		if (refNum == sysDbgCommLibraryRefNum) continue;
		if (!tableP[refNum].dispatchTblP) break;
		}
		
		
	//-----------------------------------------------------------
	// If no free entries, resize the table (grow by one)
	//-----------------------------------------------------------
	if (refNum >= GSysLibTableEntries) {
		reqSize = (GSysLibTableEntries+1) * sizeof(SysLibTblEntryType);
		err = MemPtrResize(tableP, reqSize);
		
		// If we couldn't resize it, try and reallocate it
		if (err) {
			tableP = MemPtrNew(reqSize);
			ErrFatalDisplayIf(!tableP, "Can't grow library table");
			if (!tableP) return memErrNotEnoughSpace;
			//Don't need MemSet since we only grow by one entry, and we init it below
			//MemSet(tableP, reqSize, 0);
			
			// Copy (ONLY the) old table into new chunk
			MemPtrSetOwner(tableP, 0);
			oldTableP = (MemPtr)GSysLibTableP;
			MemMove(tableP, oldTableP, reqSize - sizeof(SysLibTblEntryType));
			
			// Free old table and change global to point to new one
			GSysLibTableP = (MemPtr)tableP;
			MemPtrFree(oldTableP);
			}

		// Increment the table size and get new refNum
		GSysLibTableEntries++;
		refNum = GSysLibTableEntries-1;
		}
		

	//-----------------------------------------------------------
	// Call the library install procedure with a pointer to the
	//  Library entry in the table
	//-----------------------------------------------------------
	*refNumP = refNum;
	entryP = &(tableP[refNum]);
	entryP->dispatchTblP = 0;				// init the new entry
	entryP->globalsP = 0;
	//entryP->cardNo = 0;
	entryP->dbID = 0;
	entryP->codeRscH = 0;
	err = (*libraryP)(refNum, entryP);	// the install procedure will install
													// its dispatch table
	
	// <chg 9-10-98 RM> If there's an error, return invalid refnum
	if (err) *refNumP = sysInvalidRefNum;
	
	// return error code
	return err;
}



/************************************************************
 *
 *  FUNCTION: SysLibLoad
 *
 *  DESCRIPTION:	A utility routine to load a library given its database
 *						creator and type.  After the appliaction is done with the
 *						library which was succesfully loaded via SysLibLoad, it
 *						is responsible for unloading the library by calling
 *						SysLibRemove and passing it the library reference number
 *						returned by SysLibLoad.
 *
 *						DOLATER...
 *							Presently, the "load" functionality is NOT supported for
 *							emulation mode.
 *
 *  PARAMETERS: 
 *			libType			-- type of library database
 *			libCreator		-- creator of library database
 *			refNumP			-- pointer to variable for returning the library
 *								   reference number(on failure, sysInvalidRefNum is
 *									returned in this variable)
 *
 *  CALLED BY: Applications that need to load a library
 *
 *  RETURNS:	0 if no error; otherwise: sysErrLibNotFound, sysErrNoFreeRAM,
 *					sysErrNoFreeLibSlots, or other error returned from the
 *					library's install entry point
 *
 *  COMMENTS:
 *		After the appliaction is done with the library which was SUCCESSFULLY
 *		loaded via SysLibLoad, it is responsible for unloading the library by
 *		calling SysLibRemove and passing it the library reference number
 *		returned by SysLibLoad.
 *
 *  CREATED: 8/22/96
 *
 *  BY: Vitaly Kruglikov
 *
 *		Name	Date		Description
 *		----	----		-----------
 *		vmk	8/27/96	Cleaned up the API and moved unload code to SysLibRemove
 *
 *************************************************************/
Err SysLibLoad(UInt32 libType, UInt32 libCreator, UInt16 * refNumP)
{
	Err						err = 0;
	MemHandle					codeRscH = 0;
	SysLibEntryProcPtr	codeRscP = 0;
	UInt16						cardNo = 0;
	LocalID					dbID = 0;
	DmSearchStateType		searchState;
	DmOpenRef				dbR = 0;
	SysLibTblEntryPtr		entryP;
	Boolean					libInROM = false;


	ErrFatalDisplayIf(!refNumP, "null arg");
		
	
	// Init return values
	*refNumP = sysInvalidRefNum;
	
	
#if EMULATION_LEVEL != EMULATION_NONE

	// DOLATER... add emulation support here
	
	return( sysErrLibNotFound );
	
#endif	// EMULATION_LEVEL != EMULATION_NONE
	
	//
	// NATIVE IMPLEMENTATION
	//
	DmGetNextDatabaseByTypeCreator(true/*newSearch*/, &searchState,
			libType, libCreator, true/*onlyLatestVers*/, &cardNo, &dbID);
	if ( !dbID )
		return( sysErrLibNotFound );
	
	// Find out if database is in ROM
	libInROM = (MemLocalIDKind(dbID) != memIDHandle);
	
	// Open up this resource database
	dbR = DmOpenDatabase(cardNo, dbID, dmModeReadOnly);
	if ( !dbR )
		return( sysErrNoFreeRAM );
	
	// Get the library entry resource
	codeRscH = DmGet1Resource(sysResTLibrary, 0);
	if ( !codeRscH )
		{
		ErrNonFatalDisplay("Can't get library rsrc"); 
		err = sysErrLibNotFound;
		}
	else
		{
		// Lock the library entry resource
		codeRscP = (SysLibEntryProcPtr)MemHandleLock(codeRscH);

		// Install the library
		err = SysLibInstall(codeRscP, refNumP);

		// Protect RAM-based library databases first so they don't get deleted.
		if ( !err && !libInROM)
			DmDatabaseProtect(cardNo, dbID, true);
		}
		
	// We can now close the database
	// (in ROM-based libraries, this call will free the master-pointer list)
	DmCloseDatabase(dbR);
	dbR = 0;
	
	if ( err )
		{
		*refNumP = sysInvalidRefNum;			// invalidate the library refNum
		
		#if EMULATION_LEVEL == EMULATION_NONE		
			// release the library resource
			if ( codeRscP && !libInROM)
				{
				MemPtrUnlock(codeRscP);
				DmReleaseResource(codeRscH);
				}
		#endif	// EMULATION_LEVEL == EMULATION_NONE		
		}
	else
		{
		entryP = SysLibTblEntry(*refNumP);
		//entryP->cardNo = cardNo;
		entryP->dbID = dbID;
		if ( libInROM )
			{
			// Clear the code resource MemHandle so that we'll not attempt to unlock
			// ROM-based chunk which would cause fatal error because its master
			// pointer was freed when the library database was closed
			entryP->codeRscH = 0;	
			}
		else
			{
			// Save the code resource MemHandle so that we can unlock and release
			// it in SysLibRemove
			entryP->codeRscH = codeRscH;
			}
		}

	return( err ); 
}



/************************************************************
 *
 *  FUNCTION: SysLibRemove
 *
 *  DESCRIPTION:	Unloads libraries which were installed via SysLibInstall
 *						or SysLibLoad.  Unlocks the library's coded resource and unprotects
 *						the library database(for libraries loaded via SysLibLoad only)
 *						and removes the library from the library table
 *
 *  WARNING!!! The caller MUST be sure the library is closed
 *  	before removing it!!!!!!
 *    This call WILL NOT CLOSE THE LIBRARY FOR THE CALLER.
 *
 *  PARAMETERS: 
 *			refNum - refNum of library
 *
 *  CALLED BY: Applications that need to uninstall a library
 *
 *  RETURNS: 0 if no error
 *
 *  CREATED: 6/23/95
 *
 *  BY: Ron Marianetti
 *
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	8/27/96	Added code to remove libraries installed by SysLibLoad
 *
 *************************************************************/
Err	SysLibRemove(UInt16 refNum)
{
	SysLibTblEntryPtr		entryP;
	SysLibTblEntryPtr		tableP = (SysLibTblEntryPtr)GSysLibTableP;
	

	if ( refNum == sysInvalidRefNum )
		{
		ErrNonFatalDisplay("invalid ref num");
		return( sysErrParamErr );
		}
	
	// Get a pointer to the library's entry in the library table
	entryP = &(tableP[refNum]);
	
	// Unlock the code resource and unprotect the library database for libraries
	// loaded via SysLibLoad
	if ( entryP->dbID )
		{
		#if EMULATION_LEVEL == EMULATION_NONE
			// Unlock and release the library resource and unprotect the
			// library database(only if it is in RAM)
			if ( entryP->codeRscH )
				{
				UInt16 cardNo;
				
				cardNo = MemHandleCardNo(entryP->codeRscH);
				MemHandleUnlock(entryP->codeRscH);
				DmReleaseResource(entryP->codeRscH);
				// Unprotect the database
				DmDatabaseProtect(cardNo, entryP->dbID, false);
				}
		#endif //EMULATION_LEVEL == EMULATION_NONE
				
		}
	
	
	// Clear the entry out of the library table
	entryP->dispatchTblP = 0;
	entryP->globalsP = 0;
	//entryP->cardNo = 0;
	entryP->dbID = 0;
	entryP->codeRscH = 0;

	// return error code
	return 0;
}


/************************************************************
 *
 *  FUNCTION: SysLibFind
 *
 *  DESCRIPTION: Locates a system library by name
 *
 *  PARAMETERS: 
 *			nameP		 - pointer to name of library
 *			*refNumP  - refNum of library is returned here
 *
 *  CALLED BY: Applications that need to get the refNum of a
 *		library
 *
 *  RETURNS: 0 if no error
 *
 *	HISTORY:
 *		06/23/95	RM		Created by Ron Marianetti.
 *		12/01/00	kwk	Use StrCompareAscii vs. StrCompare.
 *
 *************************************************************/
Err	SysLibFind(const Char * nameP, UInt16 * refNumP)
{
	UInt16					refNum;
	SysLibTblEntryPtr		entryP;
	Char *					libNameP;
	SysLibTblEntryPtr		tableP = (SysLibTblEntryPtr)GSysLibTableP;
#if EMULATION_LEVEL == EMULATION_NONE
	UInt8 *					bP;
	UInt16 *					offsetP;
#else
	SimDispatchTablePtr  tabP;
#endif
	
	
	// Init return refNum
	*refNumP = sysInvalidRefNum;								// vmk 8/16/96

	// Look for a free entry
	entryP = &(tableP[0]);
	for (refNum=0; refNum<GSysLibTableEntries; refNum++, entryP++) {
		// Skip empty slots
		if (!entryP->dispatchTblP) continue;
		
		// Get the name pointer of this library
#if EMULATION_LEVEL == EMULATION_NONE
		bP = (UInt8 *)entryP->dispatchTblP;
		offsetP = (UInt16 *)bP;
		libNameP = (Char *)(bP + offsetP[0]);
#else
		tabP = (SimDispatchTablePtr)entryP->dispatchTblP;
		libNameP = (Char *)tabP->entries[tabP->numEntries];
#endif
		
		// See if it matches
		if (!StrCompareAscii(nameP, libNameP)) {
			*refNumP = refNum;
			return 0;
			}
			
		}
		
	return sysErrLibNotFound;

}



/************************************************************
 *
 *  FUNCTION: SysLibTblEntry
 *
 *  DESCRIPTION: Gets pointer to libraries entry in the library
 *		table.
 *
 *  PARAMETERS: 
 *			refNum - refNum
 *
 *  CALLED BY: Libraries in order to get a pointer to their
 *		globals or when installing their dispatch table.
 *
 *  RETURNS: entry pointer, or 0 if unsuccessful
 *
 *  CREATED: 6/23/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
SysLibTblEntryPtr	SysLibTblEntry(UInt16 refNum)
{
	SysLibTblEntryPtr		entryP;
	SysLibTblEntryPtr		tableP = (SysLibTblEntryPtr)GSysLibTableP;
	
	entryP = &(tableP[refNum]);
	return entryP;
}



/************************************************************
 *
 *  FUNCTION: SysTaskID
 *
 *  DESCRIPTION: This stub gets replaced by AMX as soon as it loads
 *
 *  PARAMETERS: void
 *
 *  RETURNS: nil task ID
 *
 *  CREATED: 2/1/95 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
#if EMULATION_LEVEL == EMULATION_NONE
UInt32 SysTaskID(void)
{
	return 0;
}
#endif

/************************************************************
 *
 *  FUNCTION: SysTaskDelay
 *
 *  DESCRIPTION: The native version of this routine is contained
 *		in the AMXGlue.c module
 *
 *  PARAMETERS: void
 *
 *  RETURNS: nil task ID
 *
 *  CREATED: 2/1/95 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
#if EMULATION_LEVEL == EMULATION_NONE
Err		SysTaskDelay(Int32 delay)
{
	return 0;
}
#endif
							


/************************************************************
 *
 *  FUNCTION: SysTaskCreate
 *
 *  DESCRIPTION: The native version of this routine is contained
 *		in the AMXGlue.c module
 *
 *  PARAMETERS: void
 *
 *  RETURNS: nil task ID
 *
 *  CREATED: 2/1/95 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
#if EMULATION_LEVEL == EMULATION_NONE
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
{
	return 0;
}
#endif

/************************************************************
 *
 *  FUNCTION: SysTaskDelete
 *
 *  DESCRIPTION: The native version of this routine is contained
 *		in the AMXGlue.c module
 *
 *  PARAMETERS: void
 *
 *  RETURNS: nil task ID
 *
 *  CREATED: 2/1/95 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
#if EMULATION_LEVEL == EMULATION_NONE
Err		SysTaskDelete(UInt32 taskID, UInt32 priority)
							
{
	return 0;
}
#endif


/************************************************************
 *
 *  FUNCTION: SysTaskTrigger
 *
 *  DESCRIPTION: This stub gets replaced by AMX as soon as it loads
 *
 *  PARAMETERS: void
 *
 *  RETURNS: nil task ID
 *
 *  CREATED: 2/1/95 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err		SysTaskTrigger(UInt32 /*taskID*/)
{
	return sysErrNotAllowed;
}


/************************************************************
 *
 *  FUNCTION: SysTaskUserInfoPtr
 *
 *  DESCRIPTION: This stub gets replaced by AMX as soon as it loads
 *
 *  PARAMETERS: task ID
 *
 *  RETURNS: nil task ID
 *
 *  CREATED: 2/1/95 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
SysTCBUserInfoPtr	SysTaskUserInfoPtr(UInt32 /*taskID*/)
{
	return 0;
}



/************************************************************
 *
 *  FUNCTION: SysTaskSetTermProc
 *
 *  DESCRIPTION: This stub gets replaced by AMX as soon as it loads
 *
 *  PARAMETERS: void
 *
 *  RETURNS: nil task ID
 *
 *  CREATED: 2/1/95 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err		SysTaskSetTermProc(UInt32 /*taskID*/, SysTermProcPtr /*termProcP*/)
{
	return sysErrNotAllowed;
}



#if EMULATION_LEVEL == EMULATION_NONE
/************************************************************
 *
 *  FUNCTION: SysTimerCreate
 *
 *  DESCRIPTION: This stub gets replaced by AMX as soon as it loads
 *
 *  PARAMETERS: void
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 3/15/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
{
	return 0;
}

/************************************************************
 *
 *  FUNCTION: SysTimerDelete
 *
 *  DESCRIPTION: This stub gets replaced by AMX as soon as it loads
 *
 *  PARAMETERS: void
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 3/15/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err		SysTimerDelete(UInt32 timerID)
{
	return 0;
}


/************************************************************
 *
 *  FUNCTION: SysTimerWrite
 *
 *  DESCRIPTION: This stub gets replaced by AMX as soon as it loads
 *
 *  PARAMETERS: void
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 3/15/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err		SysTimerWrite(UInt32 timerID, UInt32 value)
{
	return 0;
}
#endif //EMULATION_LEVEL == EMULATION_NONE


/************************************************************
 *
 *  FUNCTION: SysSemaphoreCreate
 *
 *  DESCRIPTION: This stub gets replaced by AMX as soon as it loads
 *
 *  PARAMETERS: void
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 3/15/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
{
	return 0;
}

/************************************************************
 *
 *  FUNCTION: SysSemaphoreDelete
 *
 *  DESCRIPTION: This stub gets replaced by AMX as soon as it loads
 *
 *  PARAMETERS: void
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 3/15/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err		SysSemaphoreDelete(UInt32 /*smID*/)
{
	return 0;
}

/************************************************************
 *
 *  FUNCTION: SysSemaphoreWait
 *
 *  DESCRIPTION: This stub gets replaced by AMX as soon as it loads
 *
 *  PARAMETERS: void
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 3/15/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
{
	return 0;
}


/************************************************************
 *
 *  FUNCTION: SysSemaphoreSignal
 *
 *  DESCRIPTION: This stub gets replaced by AMX as soon as it loads
 *
 *  PARAMETERS: void
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 3/15/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err		SysSemaphoreSignal(UInt32 /*smID*/)
{
	return 0;
}


/************************************************************
 *
 *  FUNCTION: SysSemaphoreSet
 *
 *  DESCRIPTION: This stub gets replaced by AMX as soon as it loads
 *
 *  PARAMETERS: void
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 3/15/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err		SysSemaphoreSet(UInt32 /*smID*/)
{
	return 0;
}


/************************************************************
 *
 *  FUNCTION: SysResSemaphoreCreate
 *
 *  DESCRIPTION: This stub gets replaced by AMX as soon as it loads
 *
 *  PARAMETERS: void
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 3/15/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
{
	return 0;
}

/************************************************************
 *
 *  FUNCTION: SysResSemaphoreDelete
 *
 *  DESCRIPTION: This stub gets replaced by AMX as soon as it loads
 *
 *  PARAMETERS: void
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 3/15/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err		SysResSemaphoreDelete(UInt32 /*smID*/)
{
	return 0;
}


/************************************************************
 *
 *  FUNCTION: SysResSemaphoreReserve
 *
 *  DESCRIPTION: This stub gets replaced by AMX as soon as it loads
 *
 *  PARAMETERS: void
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 3/15/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
{
	return 0;
}


/************************************************************
 *
 *  FUNCTION: SysResSemaphoreRelease
 *
 *  DESCRIPTION: This stub gets replaced by AMX as soon as it loads
 *
 *  PARAMETERS: void
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 3/15/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err		SysResSemaphoreRelease(UInt32 /*smID*/)
{
	return 0;
}






/************************************************************
 *
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
 *
 *  DESCRIPTION: This stub gets replaced by AMX as soon as it loads
 *
 *  PARAMETERS: 
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 7/28/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
{
	return sysErrParamErr;
}




/************************************************************
 *
 *  FUNCTION: TaskTerminationProc, private
 *
 *  DESCRIPTION: This routine gets installed as a termination procedure
 *		to every task. It is called by AMX when a task is deleted, killed,
 *		or stopped. 
 *
 *  PARAMETERS: task ID, reason for termination
 *
 *  RETURNS: void
 *
 *  CREATED: 2/1/95 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
static void TaskTerminationProc(UInt32 taskID, Int32 /*reason*/)
{
	UInt16						status;
	SysAppInfoPtr				pendingStackFreeP;
	SysExitingTaskInfoPtr	tcbUserP;

	// <chg 11/19/98 TLW>

	// Disable interrupts
	status = SysDisableInts();

	// Find the pending stack free info for this task
	pendingStackFreeP = (SysAppInfoPtr)GSysPendingStackFrees;
	while (pendingStackFreeP && (pendingStackFreeP->taskID != taskID))
		pendingStackFreeP = (SysAppInfoPtr)pendingStackFreeP->extraP;
	
	// Restore interrupt state
	SysRestoreStatus(status);
	
	// Somethings wrong if our pending stack free is not in the list
	if (!pendingStackFreeP) {
		DbgBreak();
		return;
		}

	// Get a pointer to the tasks user data area (as a SysExitingTaskInfoType)
// THIS LINE OF CODE WAS CENSORED DUE TO LICENSING RESTRICTIONS.
	
	// Let the task delete routine know where our taskID is in the pendingStackFree struct
	// so it can clear the value when it is safe for us to go ahead and delete the stack.
	tcbUserP->stackInUseByP = &pendingStackFreeP->taskID;
}





/************************************************************
 *
 *  FUNCTION: PrvCallSafely, private
 *
 *  DESCRIPTION: This routine calls the given procedure and
 *   ensures the registers are restored correctly afterwards
 *   (except for D0, D1, D2, A0 and A1)
 *
 *  PARAMETERS: address to jump to
 *
 *  RETURNS: result code from procedure call
 *
 *  CREATED: 1/28/98
 *
 *  BY: David Fedor
 *
 *************************************************************/
#if defined(__MC68K__)
static asm Int16 PrvCallSafely(ProcPtr procP)
{
	// Setup stack frame
	fralloc +
	
	// Save registers for our caller's benefit, in case
	// the called app doesn't restore them when it quits.
	MOVEM.L   D3-D7/A2-A4, -(SP)

	MOVE.L	  procP, A0				// get the procedure MemPtr
	JSR		  (A0)					// call it

	MOVEM.L   (SP)+, D3-D7/A2-A4	// Restore the registers
	
	// Return to caller
	frfree
	RTS
}
#elif defined(__POWERPC__) || defined(__INTEL__) || defined(__UNIX__)
static Int16 PrvCallSafely(ProcPtr procP)
{
	ErrDisplay("PrvCallSafely unsupported on PPC");
	return 0;
}
#else
#error "Processor type not defined"
#endif



/************************************************************
 *
 *  FUNCTION: PrvCallWithNewStack, private
 *
 *  DESCRIPTION: This routine calls the given procedure with
 *   the given stack pointer.
 *
 *  PARAMETERS: result code
 *
 *  RETURNS: result code from procedure call
 *
 *  CREATED: 2/1/95 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
#if defined(__MC68K__)
static asm Int16 PrvCallWithNewStack(ProcPtr procP,  MemPtr newStack)
{
	// Setup stack frame
	fralloc +
	
	// Save registers for our caller's benefit, in case
	// the called app doesn't restore them when it quits.
	MOVEM.L   D3-D7/A2-A4, -(SP)
	
	// Push call parameters onto the new stack
	MOVE.L	procP, A1						// procedure MemPtr in A1
	MOVE.L	newStack, A0					// new stack pointer
	MOVE.L	SP, -(A0)						// save old stack pointer
	MOVE.L	A0, SP							// new stack pointer
	JSR		(A1)								// call procedure
	MOVE.L	(SP), SP							// restore old stack pointer
	
	// Restore the registers
	MOVEM.L   (SP)+, D3-D7/A2-A4
	
	// Return to caller
	frfree
	RTS
}
#elif defined(__POWERPC__) || defined(__INTEL__) || defined(__UNIX__)
static Int16 PrvCallWithNewStack(ProcPtr procP,  MemPtr newStack)
{
	ErrDisplay("PrvCallWithNewStack unsupported on PPC");
	return 0;
}
#else
#error "Processor type not defined"
#endif





/************************************************************
 *
 *  FUNCTION: PrvDecompressData, private
 *
 *  DESCRIPTION: Copied from the CodeWarrior library sources.
 *		Decompresses image of an applications initilized global
 *		data.
 *		
 *
 *  PARAMETERS: MemPtr to compressed data, MemPtr to destination
 *
 *  CALLED BY: SysAppStartup
 *
 *  RETURNS: pointer to compressed DATA 0 xrefs
 *
 *  CREATED: 10/26/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
typedef union GetData {
	Char	raw[4];
	Int16	word;
	Int32	lword;
}	GetData; 

//
//	Pack Patterns:
//
//	0x1xxx xxxx: <raw data>		x+1 (1..128)	raw data bytes
//	0x01xx xxxx:			x+1 (1..64)		<x> 0x00 data bytes
//	0x001x xxxx: yyyy yyyy		x+2 (2..33)		<x> <y> data bytes
//	0x0001 xxxx:			x+1 (1..16)		<x> 0xFF data bytes
//	0x0000 0001:			pattern: 0x00000000FFFFXXXX
//	0x0000 0010:			pattern: 0x00000000FFXXXXXX
//	0x0000 0011:			pattern: 0xA9F00000XXXX00XX
//	0x0000 0100:			pattern: 0xA9F000XXXXXX00XX
//	0x0000 0000:	end of data
//

static UInt8 * PrvDecompressData(UInt8 * ptr, UInt8 * datasegment)
{
	GetData		ldata;
	Int16			i,data;
	UInt8			*to,c;
	

	for(i=0; i<3; i++)
	{
		ldata.raw[0]=*ptr++; ldata.raw[1]=*ptr++; ldata.raw[2]=*ptr++; ldata.raw[3]=*ptr++;
		to=(UInt8 *)datasegment+ldata.lword;
		while(1)
		{
			data=*ptr++;

			//	decompress (x&0x7f)+1 raw data bytes
			if(data&0x80) {	
				data&=0x7F; 
				do *to++=*ptr++;
					while(--data>=0); 
				continue;
				}
				
			//	decompress (x&0x3f)+1 0x00 data bytes
			//	data is already initilized to 0x00
			if(data&0x40) {	
				to+=(data&0x3F)+1; 
				continue;	
				}
				
			//	decompress (x&0x1f)+2 repeating data bytes
			if(data&0x20) {	
				data=(data&0x1F)+1; 
				c=*ptr++; 
				goto cloop;
				}
				
			//	decompress (x&0x0f)+1 0xFF data bytes
			if(data&0x10) {	
				data&=0x0F; c=0xFF;
cloop:			
				do *to++=c; 
					while(--data>=0); 
				continue;
				}
				
			switch(data) {
				case 0x00: break;
				case 0x01: to+=4; *to++=0xFF; *to++=0xFF; *to++=*ptr++; *to++=*ptr++; continue;
				case 0x02: to+=4; *to++=0xFF; *to++=*ptr++; *to++=*ptr++; *to++=*ptr++; continue;
				case 0x03: *to++=0xA9; *to++=0xF0; to+=2; *to++=*ptr++; *to++=*ptr++; to++; *to++=*ptr++; continue;
				case 0x04: *to++=0xA9; *to++=0xF0; to++; *to++=*ptr++; *to++=*ptr++; *to++=*ptr++; to++; *to++=*ptr++; continue;
				default:   DbgBreak(); break;
				}
			break;
			} // While (1)
			
		} // for (i=0; i<3; i++)

	return ptr;
}


/************************************************************
 *
 *  FUNCTION: PrvRelocateData, private
 *
 *  DESCRIPTION: Based on the CodeWarrior library sources.
 *		Relocate code/data references of a segment.
 *
 *  PARAMETERS: pointer to relocation data
 *					 pointer to segments base address
 *					 pointer to reloaction base address
 *
 *  CALLED BY: SysAppStartup
 *
 *  RETURNS: pointer to end of relocation data
 *
 *  CREATED: 10/7/96
 *
 *  BY: Steve Lemke
 *
 *************************************************************/
// Metrowerks calls this routine "__reloc_compr__"
static UInt8 *  PrvRelocateData(UInt8 * ptr, UInt8 * segment, Int32 relocbase)
{
	GetData	data;
	Int32	offset,relocations;
	Char	c;

	data.raw[0]=*ptr++; data.raw[1]=*ptr++; data.raw[2]=*ptr++; data.raw[3]=*ptr++;
	relocations=data.lword;

	for(offset=0L; relocations>0; relocations--)
	{
		c=*ptr++;
		if(c&0x80)
		{	//	8-bit signed delta
			c<<=1; offset+=c;
		}
		else
		{
			data.raw[0]=c; data.raw[1]=*ptr++;
			if(c&0x40)
			{	//	15-bit unsigned delta
				
				offset+=(Int16)(data.word<<2)>>1;
			}
			else
			{	//	direct signed 31-bit offset
				data.raw[2]=*ptr++; data.raw[3]=*ptr++;
				offset=(data.lword<<2)>>1;
			}
		}
		*(Int32 *)(segment+offset)+=relocbase;
	}
	return ptr;
}



#if EMULATION_LEVEL != EMULATION_NONE

/************************************************************
 *
 *  FUNCTION: SysCardImageInfo
 *
 *  DESCRIPTION: Returns info on memory card images used during
 *		Emulation mode
 *
 *  PARAMETERS: cardNumber, cardSize variable
 *
 *  RETURNS:
 *
 *  CREATED: 10/26/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
void *SysCardImageInfo(UInt16 cardNo, UInt32 * sizeP)
{
	if (!PilotGlobalsP) {
		*sizeP = 0;
		return 0;
		}
		
	if (cardNo < GMemCardSlots) {
		*sizeP = memCardInfoP(cardNo)->size;
		return (MemPtr)(memCardInfoP(cardNo)->baseP);
		}
		
	else {
		*sizeP = 0;
		return 0;
		}
}


/************************************************************
 *
 *  FUNCTION: SysCardImageDeleted
 *
 *  DESCRIPTION: Call to notify Pilot that memory card
 *		no longer exists
 *
 *  PARAMETERS: cardNumber, cardSize variable
 *
 *  RETURNS:
 *
 *  CREATED: 10/26/94 
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
void SysCardImageDeleted(UInt16 cardNo)
{
	if (cardNo >= GMemCardSlots) return;
	
	memCardInfoP(cardNo)->baseP = 0;
	memCardInfoP(cardNo)->size = 0;
	
	if (cardNo == 0) PilotGlobalsP = 0;
}

#endif  // EMULATION_LEVEL != EMULATION_NONE


/************************************************************
 *
 *  FUNCTION: SysSetTrapAddress
 *
 *  DESCRIPTION: Call to Set the address of a system trap
 *
 *  PARAMETERS: trap number, trap address
 *
 *  RETURNS: 0 if no err
 *
 *  CREATED: 2/22/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
Err SysSetTrapAddress(UInt16 trapNum, void * procP)
{
	trapNum -= sysTrapBase;
	
	if (trapNum >= sysNumTraps)
		return sysErrParamErr;
		
	GSysDispatchTableP[trapNum] = procP;

	// Bump rev of trap table to invalidate host debugger's cache.
	GSysDispatchTableRev++;

	return 0;
}



/************************************************************
 *
 *  FUNCTION: SysGetTrapAddress
 *
 *  DESCRIPTION: Call to Get the address of a system trap
 *
 *  PARAMETERS: trap number
 *
 *  RETURNS: Procedure ptr
 *
 *  CREATED: 2/22/95
 *
 *  REVISION HISTORY:
 *			Name		Date		Description
 *			----		----		-----------
 *			ron	 2/22/95		Initial Revision
 *			scl	10/28/98		v3.1: Added trap number range checking
 *			jwm	00-12-22		v4.0: >= because LastTrapNumber is itself invalid
 *
 *************************************************************/
void * SysGetTrapAddress(UInt16 trapNum)
{
	if ((trapNum < sysTrapBase) || (trapNum >= sysTrapLastTrapNumber)) {
		// invalid trap number; return "unimplemented" trap address
		return GSysDispatchTableP[sysTrapSysUnimplemented - sysTrapBase];
	} else {
		// valid trap number; return address from trap table
		return GSysDispatchTableP[trapNum - sysTrapBase];
	}
}




/************************************************************
 *
 *  FUNCTION: SysDisableInts
 *
 *  DESCRIPTION: Disables interrupts and returns saved status
 *		register that can be passed to SysRestoreInts.
 *
 *  PARAMETERS: void
 *
 *  RETURNS: previous status register
 *
 *  CREATED: 3/23/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
#if defined(__MC68K__)
UInt16 asm SysDisableInts(void)
{
	MOVE.W	SR, D0
	ORI.W		#0x0700, SR				// Disable interrupts
	RTS
}
#elif defined(__POWERPC__) || defined(__INTEL__) || defined(__UNIX__)
UInt16 SysDisableInts(void)
{
//	ErrDisplay("SysDisableInts unsupported on PPC");
	// Don't do anything since this is only run in the simulator
	return 0;
}
#else
#error "Processor type not defined"
#endif


/************************************************************
 *
 *  FUNCTION: SysRestoreStatus
 *
 *  DESCRIPTION: Restores processor status register to passed 
 *   value
 *
 *  PARAMETERS: status register
 *
 *  RETURNS: void
 *
 *  CREATED: 3/23/95
 *
 *  BY: Ron Marianetti
 *
 *************************************************************/
#if defined(__MC68K__)
void asm SysRestoreStatus(UInt16 newSR)
{
	MOVE.W	4(SP), SR
	RTS
}
#elif defined(__POWERPC__) || defined(__INTEL__) || defined(__UNIX__)
void SysRestoreStatus(UInt16 newSR)
{
//	ErrDisplay("SysRestoreStatus unsupported on PPC");
	// Don't do anything since this is only run in the simulator
}
#else
#error "Processor type not defined"
#endif



/************************************************************
 *
 *  FUNCTION: SysTranslateKernelErr
 *
 *  DESCRIPTION: Call to translate an error code from an
 *		AMX system call into a Pilot error code
 *
 *  PARAMETERS: AMX error code
 *
 *  RETURNS: Pilot error code
 *
 *  CREATED: 10/26/94 
 *
 *  BY: Ron Marianetti
 *
 *		vmk	11/13/96	Added sysErrDelayWakened mapping
 *		jrb	10/20/98	Added mailbox errors
 *		jesse	9/10/99	Added sysErrNotAsleep
 *		jesse	12/8/99	return sysErrNotAsleep for kernel warning
 *							#2 as well (Task not waiting, >1 wake pending)
 *
 *************************************************************/
Err SysTranslateKernelErr(Err err)
{
	switch(err) {
	
		// Task Errors
		case 0:		return 0;
		case -1: 	return sysErrParamErr;
		case -2:		return sysErrNoFreeResource;
		case -3:		return sysErrParamErr;
		case -4:		return sysErrNoFreeRAM;
		case -5:		return sysErrNotAllowed;
		case -6:		return sysErrNotAllowed;
		
		// Mailbox Errors
		case -15:	return sysErrMbEnv;		//CJ_ERNOENVLOP 	No message envelope available
		case -20:	return sysErrMbId;		//CJ_ERMBID		(-20)	/* Invalid mailbox id			*/
		case -21: 	return sysErrMbNone;		//CJ_ERMBNONE	(-21)	/* No free mailbox			*/
 		case -22: 	return sysErrMbBusy;		//CJ_ERMBBUSY	(-22)	/* Mailbox busy (cannot delete)		*/
		case -23:	return sysErrMbFull;		//CJ_ERMBFULL	(-23)	/* Mailbox full				*/
 		case -24: 	return sysErrMbDepth;	//CJ_ERMBDEPTH	(-24)	/* Invalid mailbox depth		*/

		
		// Semaphore Errors
		case -30:	return sysErrInvalidID;
		case -31:	return sysErrNoFreeResource;
		case -32:	return sysErrNotAllowed;
		case -33:	return sysErrParamErr;
		case -34:	return sysErrNotAllowed;
		case -35:	return sysErrParamErr;
		
		
		// WARNINGS
		case 1:		return sysErrNotAsleep;
		case 2:		return sysErrNotAsleepN;
		case 3:		return sysErrDelayWakened;
		case 4:		return sysErrTimeout;
		case 7:		return sysErrSemInUse;
		default:		return sysErrParamErr;
		}
	
	return sysErrParamErr;
}


/***********************************************************************
 *
 * FUNCTION:    SysCompareDataBaseCreators
 *
 * DESCRIPTION: Compare one database name to another (for sorting purposes).
 *
 * PARAMETERS:	 SysDBListItemType *a, *b - the items to compare
 *					 other - not used in the comparison
 *
 * RETURNED:	 if a < b return -1
 *					 if a==b return 0
 *					 if a > b return -1
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/5/96	Initial Revision
 *
 ***********************************************************************/
static Int16 SysCompareDataBaseCreators (void *a, void *b, Int32 /*other*/)
{
	return ((SysDBListItemType *)b)->creator - ((SysDBListItemType *)a)->creator;
}


/***********************************************************************
 *
 * FUNCTION:    SysCompareDataBaseNames
 *
 * DESCRIPTION: Compare one database name to another (for sorting purposes).
 *		Note that these names are (potentially) from the tAIN resource, and
 *		thus StrCompare has to be used, not StrCompareAscii.
 *
 * PARAMETERS:	 SysDBListItemType *a, *b - the items to compare
 *					 other - not used in the comparison
 *
 * RETURNED:	 if a < b return -1
 *					 if a==b return 0
 *					 if a > b return -1
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	6/5/96	Initial Revision
 *
 ***********************************************************************/
static Int16 SysCompareDataBaseNames (void *a, void *b, Int32 /*other*/)
{
	return StrCompare(((SysDBListItemType *)a)->name, ((SysDBListItemType *)b)->name);
}


/************************************************************
 *
 *  FUNCTION: SysCreateDataBaseList
 *
 *  DESCRIPTION: Generate a list of databases found on the memory cards
 *  matching a specific type and return the result.  If lookupName is
 *  true then a name in a tAIN resource is used instead of the database's
 *  name and the list is sorted.  Only the last version of a database 
 *	 is returned.  Databases with multiple versions are listed only once.
 *
 *  PARAMETERS:	type - type of database to find (0 for wildcard)
 *						creator - creator of database to find (0 for wildcard)
 *						dbCount - pointer to contain count of matching databases
 *						dbIDs - pointer to MemHandle allocated to contain the database list
 *						lookupName - use tAIN names and sort the list
 *
 *  RETURNS: dbCount is set to the number of matches
 *				 dbIDs is set to the list of matches
 *
 *  CREATED: 5/28/96
 *
 *  BY: Roger Flores
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	2/11/97	Fixed last database not using tAIN.
 *			roger	7/20/98	If dbIDs is empty so freed, set it to NULL.
 *       spk   8/02/00	Merge in georg's fix for XXXXX to be fault
 *								tolerant when dbP turn out null during iteration
 *								(one instance of this is when there's a .bprc in
 *								RAM without the corresponding .oprc).
 *
 *************************************************************/
extern Boolean SysCreateDataBaseList(UInt32 type, UInt32 creator, 
	UInt16 * dbCount, MemHandle *dbIDs, Boolean lookupName)
{
	Int16						dbIndex;		// UInt16 results in a bug
	
	UInt16 						cardNo;
	LocalID					dbID;
	DmOpenRef 				dbP;
	
	DmSearchStateType		searchState;
	Boolean					newSearch = true;
	UInt16						maxItems = 16;
	
	SysDBListItemType		*dbListP;
	SysDBListItemType		*dbItemP;
	Err						err=0;
	MemHandle					ainH;
	
	
	
	// Allocate a minimum size LauncherdbInfoType array to hold
	//  list of dblication databases
	*dbIDs = MemHandleNew(maxItems * sizeof(SysDBListItemType));
	if (!*dbIDs) return false;
	dbListP = MemHandleLock(*dbIDs);

	
	// Create a list of matching databases.  We do not care about the order
	// or multiple versions yet.  The assumption is that there won't be many 
	// multiple versions.
	// ½(n)
	dbIndex = 0;
	while (true) 
		{
		// Seach for the next panel
		err = DmGetNextDatabaseByTypeCreator(newSearch, &searchState, 
					type, creator, false, &cardNo, &dbID);
		if (err) break;
		newSearch = false;


		// See if we need to grow our dbList array
		if (dbIndex >= maxItems)
			{
			MemPtrUnlock(dbListP);
			dbListP = 0;
			maxItems += 4;
			err = MemHandleResize(*dbIDs, maxItems * sizeof(SysDBListItemType));
			dbListP = MemHandleLock(*dbIDs);
			if (err) 
				{
				maxItems -= 4;
				break;
				}
			}
			
						
		// Save the info on this db
		dbItemP = &dbListP[dbIndex];
		dbItemP->dbID = dbID;
		dbItemP->cardNo = cardNo;
		dbItemP->iconP = NULL;			// Filled in by LauncherDrawIcons
		
		
		// Get the creator and type of this panel and the version
		DmDatabaseInfo(cardNo, dbID, dbItemP->name, NULL, &dbItemP->version, 
			NULL, NULL, NULL, NULL, NULL, NULL, &dbItemP->type, &dbItemP->creator);
						
		dbIndex++;
		}
		

	// Sort the matches by creator.  This places different versions of apps 
	// next to each other while also ordering the list of apps.  We will
	// need to sort again using the name of the apps should be in a roughly
	// similiar order, especially if the first letter of the creator is the
	// same as the first letter of the app's name.
	// ½(n log n)
	SysQSort (dbListP, dbIndex, sizeof(SysDBListItemType), SysCompareDataBaseCreators, 0);
	
	
	// Now look through the apps looking for multiple versions of an app,
	// discarding any earlier versions.  Ideally there will be at most
	// two versions of an app (one in ROM and one in RAM).  We work
	// backwards so that the MemMove moves only bytes already checked.
	// ½ (n)  (assuming few multiple versions).
	if (type && creator == 0 && type != sysFileTpqa)
		{
		//<chg 4-30-98 RM> Modified to work with CWPro 3
		//dbItemP = &dbListP[dbIndex - 2];
		dbItemP = dbListP + dbIndex - 2;
		while (dbItemP >= dbListP)
			{
			// Same app?  If so remove the older version.
			if (dbItemP->creator == (dbItemP + 1)->creator)
				{
				// Use the first version if it's either newer or if it equal and RAM based.
				// Otherwise use the other one.
				if (dbItemP->version > (dbItemP + 1)->version ||
					 (dbItemP->version == (dbItemP + 1)->version &&
					  MemLocalIDKind(dbItemP->dbID) == memIDHandle))
					MemMove(dbItemP + 1, dbItemP + 2, (UInt8 *) &dbListP[dbIndex] - (UInt8 *) (dbItemP + 2));
				else
					MemMove(dbItemP, dbItemP + 1, (UInt8 *) &dbListP[dbIndex] - (UInt8 *) (dbItemP + 1));
				dbIndex--;		// There is one less database now.
				}
			dbItemP--;
			}
		}
	
	
	// If there aren't any databases then exit now without any to save unneccessary work.
	if (dbIndex == 0)
		{
		*dbCount = dbIndex;
		goto Exit;
		}		
	
	// We might want to shrink the database list to conserve memory.  Either
	// extra may have been allocated or item may have been removed if they
	// were duplicates.
	// ½ (1)
	if (dbIndex < maxItems) 
		{
		MemPtrUnlock(dbListP);
		dbListP = 0;
		err = MemHandleResize(*dbIDs, dbIndex * sizeof(SysDBListItemType));
		dbListP = MemHandleLock(*dbIDs);
		}
		
	
	// If the names were requested then look them up and sort by the names found.
	// ½ (n)   (assuming the records are mostly ordered already because the
	//          creator and name have the same first letter)
	if (lookupName)
		{
		// <chg 4-30-98 RM> fixed for CWPro 3
		// dbItemP = &dbListP[dbIndex - 1];
		dbItemP = dbListP + dbIndex - 1;
		while (dbItemP >= dbListP)
			{
			// Find a name for this database	
			dbP = DmOpenDatabase(dbItemP->cardNo, dbItemP->dbID, dmModeReadOnly);
			
			// only read the name from the tAIN resource & close the database if the database  
			// was successfully opened.
			if (dbP)
				{
				if ((ainH = DmGet1Resource(ainRsc, ainID)) != 0)
					{
					StrCopy(dbItemP->name, MemHandleLock(ainH));
					MemHandleUnlock(ainH);
					DmReleaseResource(ainH);
					}
				
				DmCloseDatabase(dbP);
				}
			// remove the database from the list if we can not open the database to get the name.
			else
				{
				MemMove(dbItemP, dbItemP + 1, (UInt8 *) &dbListP[dbIndex] - (UInt8 *) (dbItemP + 1));
				dbIndex--;
				}

			dbItemP--;
			}
			
			
		// Sort the applications by their name.
		SysQSort (dbListP, dbIndex, sizeof(SysDBListItemType), SysCompareDataBaseNames, 0);
		}	
	
	
	// Record how many matches were found
	*dbCount = dbIndex;
	
Exit:
	if (dbListP)
		MemPtrUnlock(dbListP);
	
	// If nothing was found remove the memory allocated.	
	if (*dbCount == 0)
		{
		MemHandleFree(*dbIDs);
		*dbIDs = NULL;
		return false;
		}
		
	return true;
}



/************************************************************
 *
 *  FUNCTION: SysCreatePanelList
 *
 *  DESCRIPTION: Generate a list of panels found on the memory cards
 *  and return the result.  Multiple versions of a panel are listed once.
 *
 *  PARAMETERS: panelCount - pointer to set to the number of panels
 *					 panelIDs - pointer to MemHandle containing a list of panels
 *
 *  RETURNS: void
 *
 *  CREATED: 5/28/96
 *
 *  BY: Roger Flores
 *
 *************************************************************/
extern Boolean SysCreatePanelList(UInt16 * panelCount, MemHandle *panelIDs)
{
	return SysCreateDataBaseList(sysFileTPanel, 0, panelCount, panelIDs, true);
}


/************************************************************
 *
 *  FUNCTION: SysGetOSVersionString
 *
 *  DESCRIPTION: Get a pointer to a string containing the OS version.
 *
 *  PARAMETERS: nothing
 *
 *  RETURNS: pointer to a string containing the OS version.  The
 *	caller must free the string using MemPtrFree().
 *
 *  CREATED: 11/13/97
 *
 *  BY: Roger Flores
 *
 *  REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			SCL	2/13/98	Ifdef'd out "Pro" StrCat code (not needed for Rocky).
 *
 *************************************************************/
extern Char * SysGetOSVersionString()
{
	MemHandle resultH;
	Char * resultP = NULL;
	UInt16 resultLength;
	MemHandle strH;
	Char * strP;
#if USER_MODE == USER_MODE_DEMO
	Err err;
#endif
	//UInt32 value;
	
	
	// Copy base version name
	strH = DmGetResource(verRsc, systemVersionID);
	if (strH != NULL)
		{
		strP = MemHandleLock(strH);
		resultLength = StrLen(strP) + sizeOf7BitChar(nullChr);
		resultH = MemHandleNew(resultLength);
		if (resultH != NULL)
			{
			resultP = MemHandleLock(resultH);
			StrCopy(resultP, strP);
			MemHandleUnlock(strH);
			}
		DmReleaseResource(strH);
		}
	
	// Add additional feature strings, if necessary (for Jerry, etc.)
	// This routine was originally written with Personal vs. Pro in mind.
	// In 2.0, "Pro" meant that NetLib was present (hence FtrGet netFtrCreator).
	// In 3.0 (Rocky), there is no Personal vs. Pro.  However, future versions
	// may need to extend the OS Version String based on other build "differences".
	// This is where that should be done, after adding the appropriate string to
	// Resources:UI.rsrc and an equate for it in <UIResourcesPrv.h>
#if 0
	if (resultP != NULL) {
		err = FtrGet(netFtrCreator, netFtrNumVersion, &value);
		if (!err) {
			strH = DmGetResource(strRsc, proStrID);
			if (strH != NULL) {
				strP = MemHandleLock(strH);
				// Only append this string if there's something there...
				if (*strP) {
					MemHandleUnlock(resultH);
					resultLength += sizeOf7BitChar(spaceChr) + StrLen(strP);
					err = MemHandleResize(resultH, resultLength);
					resultP = MemHandleLock(resultH);
					if (!err) {
						StrCat(resultP, " ");
						StrCat(resultP, strP);
						}
					}
				MemHandleUnlock(strH);
				DmReleaseResource(strH);
				}
			}
		}
#endif

	// Add "Demo" string, if we're doing a POP (Point of Purchase) build
#if USER_MODE == USER_MODE_DEMO
	if (resultP != NULL) {
		strH = DmGetResource(strRsc, demoStrID);
		if (strH != NULL) {
			strP = MemHandleLock(strH);
			// Only append this string if there's something there...
			if (*strP) {
				MemHandleUnlock(resultH);
				resultLength += sizeOf7BitChar(spaceChr) + StrLen(strP);
				err = MemHandleResize(resultH, resultLength);
				resultP = MemHandleLock(resultH);
				if (!err) {
					StrCat(resultP, " ");
					StrCat(resultP, strP);
					}
				}
			MemHandleUnlock(strH);
			DmReleaseResource(strH);
			}
		}
#endif

	return resultP;
}

/************************************************************
 *
 *  FUNCTION: SysUIBusy
 *
 *  DESCRIPTION: Set or return the System UI busy count. Various
 *				entities in the system can tie up the UI in ways that
 *				should not be interrupted. Anything that does so, should
 *				Set UI busy so that other entities know about this state.
 *
 *  PARAMETERS: set - true if the second paramter is used to set a value
 *				value - true if UI busy should be incremented, false to decrement
 *
 *  RETURNS: 	current UI busy count (0 if not busy)
 *
 *  CREATED: 11/18/97
 *
 *  BY: Gavin Peacock
 *
 *************************************************************/
extern UInt16 SysUIBusy(Boolean set, Boolean value)
{
	if (set)
		{
		if (value)
			GSysUIBusyCount++;
		else
			{
			ErrFatalDisplayIf(GSysUIBusyCount==0,"UIBusyCount underflow");
			GSysUIBusyCount--;
			}
		}
	return GSysUIBusyCount;
}

/************************************************************
 *
 *  FUNCTION: SysSetDevAutoLock
 *
 *  DESCRIPTION: This routine sets the timeout value in seconds for
 *					device auto locking power-off. 
 *
 *					  1. Never:  Disable auto locking
 *
 *					  2. Upon power off: Lock the device when the device turns off
 *						   upon expired Auto off timer or by pressing the hard power button.
 *							The GSysAutoLockUponPowerOff is checked when the device goes to sleep.
 *							
 *
 *					  3. After a preset delay: Lock the device after an X mintues/hours delay.
 *						  Note that, unlike the auto off timer, the delay continues after the
 *						  device is turned off. This is when the auto lock time is longer than 
 *						  the auto off time.
 *
 *					  4. At a preset time: Lock the device every day at HH:MM.
 *	
 *
 *  PARAMETERS: 
 *					autoLockType: never, uponPowerOff, afterPresetDelay or 
 *									  atPresetTime
 *
 *  RETURNS: none
 *
 *  CREATED: 6/2/00
 *
 *  BY: Waddah Kudaimi
 *
 *************************************************************/
void 		SysSetDevAutoLockTime(UInt16 devAutoLockType)
{
	UInt32					autoLockTime;
	UInt32					timeNow;
	UInt32 					refP;
	Err								err=0, tmpErr;
	AlmGlobalsPtr	almGlobalsP = (AlmGlobalsPtr)GAlmGlobalsP;

	

	//We want to make sure any auto lock alarm is deleted before we change
	//the auto lock type.
	
	if(almGlobalsP)	
		if (AlmGetProcAlarm(&PrvAutoLockAlarmProc, &refP) ) {
			tmpErr = AlmSetProcAlarm(&PrvAutoLockAlarmProc, 0, 0);
			if (!err) err = tmpErr;
			ErrNonFatalDisplayIf(err, "");
		}


	switch((SecurityAutoLockType)devAutoLockType)
		{
		case never:

			GSysAutoLockUponPowerOff = false;
			GSysAutoLockTimeoutSecs = 0;
			break;
			
		case uponPowerOff:		
		
			// Set the global so it locks when the device is powered off		
		 	GSysAutoLockUponPowerOff = true;
			GSysAutoLockTimeoutSecs = 0;
		 	break;
		 	
		case afterPresetDelay:
		
			//Get the preset delay after which the device should be locked.
		   GSysAutoLockTimeoutSecs = PrefGetPreference(prefAutoLockTime);
			GSysAutoLockUponPowerOff = false;
			break;
		
		
		case atPresetTime:

			GSysAutoLockUponPowerOff = false;

			//Get the time now
			timeNow = TimGetSeconds();
	
			//Get the preset time when the device should be locked
			autoLockTime = PrefGetPreference(prefAutoLockTime);
						
			if (autoLockTime < timeNow) 
			  // If the lock time has passed for this day set the timeout to the next day.
			  autoLockTime += (hoursPerDay * hoursInSeconds);

			GSysAutoLockTimeoutSecs = autoLockTime;
						  
			tmpErr = AlmSetProcAlarm(&PrvAutoLockAlarmProc, 0, autoLockTime);
			if (!err) err = tmpErr;
			ErrNonFatalDisplayIf(err, "");
		   break;
				
		}


}

/******************************************************************************************
 *
 *  FUNCTION: SysSetPwdTimeout
 *
 *  DESCRIPTION: 	This function sets the timeout value forpassword prompting.
 *					 	This timeout is used when the device is locked and notification for a private 
 *						record occurs. At this point the user can enter the system lockout password only, 
 *						and the user will not be prompted for private record password for the duration of 
 *						the timeout.
 *
 *  PARAMETERS: none
 *
 *  RETURNS: none
 *
 *  CREATED: 9/26/00
 *
 *  BY: Waddah Kudaimi
 *
 ******************************************************************************************/
void 	SysSetPwdTimeout(UInt32 timeout)
{

   GSysSetPwdTimeoutSecs = timeout;
   GSysSetPwdTimeoutTicks = GHwrCurTicks;

}


/******************************************************************************************
 *
 *  FUNCTION: SysCheckPwdTimeout
 *
 *  DESCRIPTION: 	This function checks to see if the timeout value for password prompting is set.
 *					 	This timeout is used when the device is locked and notification for a private 
 *						record occurs. At this point the user can enter the system lockout password only, 
 *						and the user will not be prompted for private record password for the duration of 
 *						the timeout.
 *
 *  PARAMETERS: none
 *
 *  RETURNS: true if we are still within the timeout period, flase otherwise.
 *
 *  CREATED: 9/26/00
 *
 *  BY: Waddah Kudaimi
 *
 ******************************************************************************************/
Boolean 	SysCheckPwdTimeout()
{
	UInt32 delta;
	
   if (GSysSetPwdTimeoutSecs) {
	   delta = GHwrCurTicks - GSysSetPwdTimeoutTicks;
		if (delta <= ((Int32)GSysSetPwdTimeoutSecs*sysTicksPerSecond)) {
		   GSysSetPwdTimeoutSecs = 0;
	      return true;
	   }
   }

return false;
}

/******************************************************************************************
 *
 *  FUNCTION: SysSetDbgLockout
 *
 *  DESCRIPTION: 	This function sets a global that disallows the device to go into big/small ROM.
 *					 	Basically, it is allowing the reset-down_arrow to be disabled. 
 *
 *  PARAMETERS: none
 *
 *  RETURNS: Previous lockout value
 *
 *  CREATED: 11/05/00
 *
 *  BY: Waddah Kudaimi
 *
 ******************************************************************************************/
Boolean 	SysSetDbgLockout(Boolean setLockout)
{
//Get the existing state before setting new state.
Boolean result = (GDbgLockout==dbgLockoutEnabled);
	
	//Set requested state
   if (setLockout) {
		GDbgLockout = dbgLockoutEnabled;
	}
	else
	   GDbgLockout = 0;

return result;
}


#ifdef INTEGRATED_HSIMGR
//============================================================================
//
//	FUNCTION:		PrvHSIQueryPeripheral
//
//	DESCRIPTION:	Attempt to identify the peripheral on the serial port.
//
//	CALLED BY:		PrvHandleEvent().
//
//	PARAMETERS:		
//
//		peripheralResponseP	--> SysHSIResponseType *
//
//	RETURNS:
//
//		sysNotifyHSISerialPortInUseEvent				// when serial port is in use
//		sysNotifyHSIPeripheralRespondedEvent		// with peripheral response
//		sysNotifyHSIPeripheralNotRespondingEvent	// when peripheral does not respond
//			
// jhl 7/26/00 Integrate HSIMgr functionality
//
//============================================================================
//		If serial port is already open, return 'hsiu' (in use).
//		Else if serial open error, return 0 (implied error).
//		Else
//			Open serial port at 9600 baud, send the inquiry string "ATI3\r" and wait
//			up to 100 ms for a response.
//			Close the serial port.
//			If a response is received, return 'hspr' (peripheral responded)
//				and include response.
//			Else return 'hspn' (peripheral not responding).
//============================================================================
static UInt32 PrvHSIQueryPeripheral(SysHSIResponseType *peripheralResponseP)
{
	Char *inquiryP = (Char *)sysHSISerialInquiryString;
	UInt16 openPort,length,dataLen = sizeof(UInt32);
	UInt32 originalBaudRate,originalFlags,originalCtsTimeout;
	UInt32 localBaudRate,localFlags,localCtsTimeout;
	Boolean ok;
	Err err;

	// Open serial port and send inquiry
	err = SrmOpen(sysFileCUart328EZ,sysHSISerialInquiryBaud,&openPort);
	if (err) {
		// if the port is already open, return the sysNotifyHSISerialPortInUseEvent
		if (err == serErrAlreadyOpen) {
			SrmClose(openPort);
			return(sysNotifyHSISerialPortInUseEvent);
		}
		// if other error, return unidentified
		return(0);
	}

	// Configure the port
	SrmControl(openPort,srmCtlGetBaudRate,&originalBaudRate,&dataLen);
	SrmControl(openPort,srmCtlGetFlags,&originalFlags,&dataLen);
	SrmControl(openPort,srmCtlGetCtsTimeout,&originalCtsTimeout,&dataLen);
	localBaudRate = sysHSISerialInquiryBaud;
	localFlags =	srmSettingsFlagStopBits1 |
						srmSettingsFlagRTSAutoM |
						srmSettingsFlagCTSAutoM |
						srmSettingsFlagFlowControlIn |
						srmSettingsFlagBitsPerChar8;
	localCtsTimeout = sysHSISerialInterChrTimeout;
	SrmControl(openPort,srmCtlSetBaudRate,&localBaudRate,&dataLen);
	SrmControl(openPort,srmCtlSetFlags,&localFlags,&dataLen);
	SrmControl(openPort,srmCtlSetCtsTimeout,&localCtsTimeout,&dataLen);

	// Flush, send inquiry and wait for response
	SrmReceiveFlush(openPort,0);
	SrmSend(openPort,inquiryP,sysHSISerialInquiryStringLen,&err);
	if (err) {
		peripheralResponseP->responseLength = 0;
	} else {
		length = sizeof(peripheralResponseP->responseBuffer) - 1;
		ok = PrvHSIGetResponseLine(openPort,peripheralResponseP->responseBuffer,&length);
		if (ok) {
			// If the first response equals the inquiry string (minus the CR/LF), assume
			// it is just an echo of the inquiry command. Disregard it and read a second
			// line, which we assume is the actual response.
			if (length == sysHSISerialInquiryStringLen - 2
					&& StrNCaselessCompare(inquiryP,peripheralResponseP->responseBuffer,length) == 0) {
				length = sizeof(peripheralResponseP->responseBuffer) - 1;
				ok = PrvHSIGetResponseLine(openPort,peripheralResponseP->responseBuffer,&length);
			}
		}
		peripheralResponseP->responseLength = ok ? length : 0;
	}

	// Close the serial port and process the response (if any)
	SrmClearErr(openPort);
	originalFlags &= ~(srmSettingsFlagRTSAutoM | srmSettingsFlagCTSAutoM);
	SrmControl(openPort,srmCtlSetBaudRate,&originalBaudRate,&dataLen);
	SrmControl(openPort,srmCtlSetFlags,&originalFlags,&dataLen);
	SrmControl(openPort,srmCtlSetCtsTimeout,&originalCtsTimeout,&dataLen);
	SrmClose(openPort);
	// add a terminating NUL for convenience
	peripheralResponseP->responseBuffer[peripheralResponseP->responseLength] = 0;

	return(	peripheralResponseP->responseLength
					? sysNotifyHSIPeripheralRespondedEvent
					: sysNotifyHSIPeripheralNotRespondingEvent	);
}


//============================================================================
//
//	FUNCTION:		PrvHSIGetResponseLine
//
//	DESCRIPTION:	Read a CR.LF terminated response line.
//
//	CALLED BY:		PrvHSIQueryPeripheral() to receive a response line.
//
//	PARAMETERS:		
//
//
// jhl 7/26/00 Integrate HSIMgr functionality
//
//============================================================================
//
// according to spec
// 1. stop receiving when:
//		a. CR/LF is received
//		b. at least one character is not received within 20ms
//		c. a total of 100ms has passed
// 2. if a CR/LF is received, return(true) else return(false)
//
// Note:
// The CR/LF (or any other control character) is not stored in the buffer.
//
//============================================================================
static Boolean PrvHSIGetResponseLine(UInt16 openPort, Char *bufferP, UInt16 *bufferLengthP)
{
	UInt32 timeLimit;
	Boolean CRFound;
	Char *cP,ch,*bufferLimitP;
	Err err;

	timeLimit = TimGetTicks() + sysHSISerialInquiryTimeout;
	CRFound = false;
	cP = bufferP;
	bufferLimitP = bufferP + *bufferLengthP;
	for ( ; ; ) {
		if (SrmReceive(openPort,&ch,1,sysHSISerialInterChrTimeout,&err) != 1) break;
		if (CRFound) {
			if (ch == 10) {
				*bufferLengthP = cP - bufferP;
				return(true);
			}
			CRFound = false;
		}
		if (TimGetTicks() > timeLimit) break;
		if (ch == 13) CRFound = true;
		else if (ch >= ' ' && cP < bufferLimitP) *cP++ = ch;
	}
	*bufferLengthP = 0;
	return(false);
}
#endif // INTEGRATED_HSIMGR
