/*
 * Copyright (c) 2004-2008 Darron M Broad
 * All rights reserved.
 *
 * Licensed under the terms of the BSD license, see file LICENSE
 * for details.
 *
 * $Id$
 */

#ifndef _TESTTHREAD_H
#define _TESTTHREAD_H

#include "testapp.h"

class Testthread : public Thread
{
private:
	int rc; /* TEST RETURN CODE */

        virtual int thread(void);

protected:

public:
        Testthread(void);
        virtual ~Testthread(void);
};

#endif /* !_TESTTHREAD_H */
