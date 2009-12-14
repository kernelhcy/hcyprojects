/******************************************************************************
 *
 * Copyright (c) 1994-1998 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: FeaturePrv.h
 *
 * Release: 
 *
 * Description:
 *		Private Header for the Feature Manager
 *
 * History:
 *   	8/9/95  RM - Created by Ron Marianetti
 *		9/2/98  Bob - Updated for performance
 *
 *****************************************************************************/

#ifdef NON_PORTABLE
#ifndef __FEATUREPRV_H__
#define __FEATUREPRV_H__


#define ftrTableSizeInit			20		// tune for ROM
#define ftrTableSizeIncrement		10
#define ftrTableCardNo				 0


/************************************************************
 * Structure of a 'new' Feature Table
 *************************************************************/
typedef struct FtrFeatureType {
	UInt32				creator;							// feature creator
	UInt16				featureNum;						// feature number
	UInt32				value;							// feature value
	} FtrFeatureType;
typedef FtrFeatureType* FtrFeaturePtr;

typedef struct FtrTableType {
	UInt16				heapID;								// heap # for FtrPtrs
	UInt16				numEntries;							// # of entries
	UInt16				allocEntries;						// space allocated
	FtrFeatureType	features[ftrTableSizeInit];	// var. size array of Features
	} FtrTableType;
typedef FtrTableType*	FtrTablePtr;
typedef FtrTablePtr*		FtrTableHand;



/************************************************************
 * Structure of ROM Feature Table ('feat' resource)
 * (also the old-style RAM feature table)
 *************************************************************/
typedef struct ROMFtrFeatureType {
	UInt16		num;										// feature number
	UInt32		value;									// feature value
	} ROMFtrFeatureType;
typedef ROMFtrFeatureType* ROMFtrFeaturePtr;

typedef struct ROMFtrCreatorType {
	UInt32				creator;							// feature creator
	UInt16				numEntries;						// # of entries
	ROMFtrFeatureType	feature[1];						// variable size array of Features
	} ROMFtrCreatorType;
typedef ROMFtrCreatorType*	ROMFtrCreatorPtr;

typedef struct ROMFtrTableType {
	UInt16				numEntries;						// # of entries
	ROMFtrCreatorType	creator[1];						// var. size array of Creators
	} ROMFtrTableType;
typedef ROMFtrTableType*	ROMFtrTablePtr;




#endif // __FEATUREPRV_H__
#endif // NON_PORTABLE
