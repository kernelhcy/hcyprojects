////////////////////////////////////////////////////////////////////////////////////////////
//
//    
//    File: mesg.h
//
//    Copyright (C): 2005 Searen Network Software Ltd.
//
//    [ This source code is the sole property of Searen Network Software Ltd.  ]
//    [ All rights reserved.  No part of this source code may be reproduced in ]
//    [ any form or by any electronic or mechanical means, including informa-  ]
//    [ tion storage and retrieval system, without the prior written permission]
//    [ of Searen Network Software Ltd.                                        ]
//    [                                                                        ]
//    [   For use by authorized Searen Network Software Ltd. employees only.   ]
//
//    Description:   This class can read, write and watch one serial port.
//					 It sends messages to its owner when something happends on the port
//					 The class creates a thread for reading and writing so the main
//					 program is not blocked.
// 
//
//
//  AUTHOR: Ren Yu.
//  DATE: Sept. 20, 2005
//
//
/////////////////////////////////////////////////////////////////////////////////////////////
//
#ifndef __MESG_H__
#define __MESG_H__

////////////////////////////////////////////////////////////////////////////////////////
//
//
#include "includes.h"

typedef INT16U LPARAM;
typedef INT16U HPARAM;

typedef struct
{
	INT16U  lparam;
	INT16U  hparam;
	union 
	{
		void * param;
	} info;
} MSG, *PMSG;

void SendMessage ( INT8U IDs, LPARAM lParam, HPARAM hParam );
void PostMessage ( INT8U IDs, LPARAM lParam, HPARAM hParam );
void SendMsg     ( INT8U IDs, PSMG pMsg );
void PostMsg     ( INT8U IDs, PSMG pMsg );

PMSG GetMessage  ( void );

//////////////////////////////////////////////////////////////////////////////////////////
//
#endif