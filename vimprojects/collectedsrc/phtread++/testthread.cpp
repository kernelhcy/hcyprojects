/*
 * Copyright (c) 2004-2008 Darron M Broad
 * All rights reserved.
 *
 * Licensed under the terms of the BSD license, see file LICENSE
 * for details.
 *
 * $Id$
 */

/*
 * testthread class
 */

#include "testthread.h"

Testthread::Testthread(void) : Thread()
{
	rc= 5; /* TEST RETURN CODE */
}

Testthread::~Testthread(void)
{
}

int
Testthread::thread(void)
{
	cout <<  __PRETTY_FUNCTION__ << " argc: " << argc << " argv[0]: " << argv[0] << endl;

	while(run && rc--)
	{
		sleep(1);
		cout <<  __PRETTY_FUNCTION__ << endl;
	}
	return rc; /* TEST RETURN CODE */
}
