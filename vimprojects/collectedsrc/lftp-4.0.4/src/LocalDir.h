/*
 * lftp and utils
 *
 * Copyright (c) 2002 by Alexander V. Lukyanov (lav@yars.free.net)
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

/* $Id: LocalDir.h,v 1.4 2008/11/27 05:56:21 lav Exp $ */

#ifndef LOCALDIR_H
#define LOCALDIR_H

#include "xstring.h"

class LocalDirectory
{
   int fd;
   xstring_c name;

public:
   LocalDirectory();
   LocalDirectory(const LocalDirectory *);
   ~LocalDirectory();

   const char *GetName();
   const char *Chdir();	// returns error message or NULL
   void SetFromCWD();
   void Unset();

   LocalDirectory *Clone() const;
};

#endif
