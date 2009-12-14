////////////////////////////////////////////////////////////////////////////////////////////
//
//    
//    File: thread.h
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
#ifndef __THREAD_H__
#define __THREAD_H__

////////////////////////////////////////////////////////////////////////////////////////
//
//
#include "includes.h"

typedef INT8U thread_t;
typedef void (* thread_p)( void * argc );

thread_t CreateThread ( thread_p * func, void * arg, int prio );

//////////////////////////////////////////////////////////////////////////////////////////
//
#endif