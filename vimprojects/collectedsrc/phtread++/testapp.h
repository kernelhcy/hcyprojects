/*
 * Copyright (c) 2004-2008 Darron M Broad
 * All rights reserved.
 *
 * Licensed under the terms of the BSD license, see file LICENSE
 * for details.
 *
 * $Id: testapp.h,v 1.3 2004/06/09 01:16:36 darron Exp $
 */

#ifndef _TESTAPP_H
#define _TESTAPP_H

/* STL */
using namespace std;
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

/* system headers */
extern "C" {
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <sysexits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
};

#include "thread.h"
#include "testthread.h"

#endif /* !_TESTAPP_H */
