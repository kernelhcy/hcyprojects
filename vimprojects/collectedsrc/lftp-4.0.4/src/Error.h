/*
 * lftp and utils
 *
 * Copyright (c) 2009 by Alexander V. Lukyanov (lav@yars.free.net)
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

/* $Id: Error.h,v 1.1 2009/07/17 13:24:04 lav Exp $ */

#ifndef ERROR_H
#define ERROR_H

#include "xstring.h"

class Error
{
   xstring text;
   int code;
   bool fatal;
public:
   Error();
   Error(int,const char *,bool);
   ~Error();
   void Set(int,const char *,bool);

   const char *Text() const { return text; }
   int Code() const { return code; }
   bool IsFatal() const { return fatal; }

   static Error *Fatal(const char *s,int c=-1);
};

#endif//ERROR_H
