/*
 * lftp and utils
 *
 * Copyright (c) 2004 by Alexander V. Lukyanov (lav@yars.free.net)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* $Id: buffer_std.h,v 1.2 2008/11/27 05:56:35 lav Exp $ */

#ifndef BUFFER_STD_H
#define BUFFER_STD_H

#include "Job.h"
#include "buffer.h"

class IOBuffer_STDOUT : public IOBuffer
{
   Job *master;
   int Put_LL(const char *buf,int size);

public:
   IOBuffer_STDOUT(Job *m) : IOBuffer(PUT) { master=m; }
};

#endif //BUFFER_STD_H
