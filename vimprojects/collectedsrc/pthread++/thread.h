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
 * pthread wrapper class for c++
 *
 * usage:
 *
 *	class Test: public Thread
 *	{
 *	private:
 *		virtual int thread(void)
 *		{
 *			... do stuff ...
 *		}
 *	protected:
 *	public:
 *	        Test(void) : Thread() {}
 *		virtual ~Test(void) {}
 *	};
 *
 *	...
 *
 *	Test test;
 *	if( test.start(argc, argv) != 0 )
 *	{
 *		... error ...
 *	}
 *
 *	threadrc rc;
 *	pthread_join(test.getid(), &rc.vp);
 *	if(rc.i != 0)
 *	{
 *		... error ...
 *	}
 */

#ifndef _THREAD_H
#define _THREAD_H

extern "C" {
#include "pthread.h"
#include "assert.h"
};

typedef union {
	void *vp;
	int i;
}       threadrc;

class Thread
{
private:
        pthread_t id;

	virtual int thread(void) = 0;

        static void *start_routine(void *thisarg)
	{
		Thread *thisthread= (Thread *)thisarg;
		assert(thisthread->id);
		return (void *) thisthread->thread();
	}
        
protected:
        int argc;
        char **argv;
	bool run;

public:
        Thread(void) { id=0; run=true; }
        virtual ~Thread(void) {}

	pthread_t getid(void) { return id; }

	int start(int argc, char **argv)
	{
		this->argc= argc;
		this->argv= argv;
		return pthread_create(&id, NULL, start_routine, this);
	}
	void stop(void) { run=false; }
};

#endif /* !_THREAD_H */
