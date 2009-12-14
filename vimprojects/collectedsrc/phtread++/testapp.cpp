/*
 * Copyright (c) 2003-2008 Darron M Broad
 * All rights reserved.
 *
 * Licensed under the terms of the BSD license, see file LICENSE
 * for details.
 *
 * $Id: testapp.cxx,v 1.3 2004/06/09 02:43:56 darron Exp $
 */

#include "testapp.h"

void usage(const char *reason)
{
	cout <<  __PRETTY_FUNCTION__ << endl;

	cout << "Error: " << reason << endl;
	cout << "Usage: " << "testapp" << endl;
	exit( EX_USAGE );
}

int main(int argc, char **argv)
{
	cout <<  __PRETTY_FUNCTION__ << " argc: " << argc << " argv[0]: " << argv[0] << endl;

	if( argc!=1 )
		usage("Invalid arguments");
	
	Testthread testthread;

	/* start thread */
	if( testthread.start(argc, argv) != 0 )
	{
		cout << "Error: " << strerror(errno) << endl;
		exit( EX_SOFTWARE );
	}

	sleep(2);

	/* stop thread */
	testthread.stop();

	threadrc rc;

	/* wait for thread */
	pthread_join(testthread.getid(), &rc.vp);
	if(rc.i != 0)
	{
		cout << "Error: " << rc.i << " (TEST)" << endl;
		exit( EX_SOFTWARE );
	}

	exit( EX_OK );
}
