/*
 * lftp and utils
 *
 * Copyright (c) 2001 by Alexander V. Lukyanov (lav@yars.free.net)
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

/* $Id: PtyShell.h,v 1.6 2008/11/27 05:56:25 lav Exp $ */

#ifndef PTYSHELL_H
#define PTYSHELL_H

#include "Filter.h"

class PtyShell : public FDStream
{
   Ref<ArgV> a;
   Ref<ProcWait> w;
   pid_t pg;

   xstring_c oldcwd;

   bool closed;
   bool use_pipes;
   int pipe_in;
   int pipe_out;

   void Init();

public:
   PtyShell(const char *filter);
   PtyShell(ArgV *a);
   ~PtyShell();

   void SetCwd(const char *);
   void UsePipes() { use_pipes=true; }

   int getfd();
   int getfd_pipe_in()  { return pipe_in; }
   int getfd_pipe_out() { return pipe_out; }
   bool Done();

   void Kill(int sig=SIGTERM) { if(w) w->Kill(sig); }
   pid_t GetProcGroup() const { return pg; }

   bool broken();
};

#endif // PTYSHELL_H
