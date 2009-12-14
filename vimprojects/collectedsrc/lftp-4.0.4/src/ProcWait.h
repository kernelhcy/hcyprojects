/*
 * lftp and utils
 *
 * Copyright (c) 1996-2007 by Alexander V. Lukyanov (lav@yars.free.net)
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

/* $Id: ProcWait.h,v 1.10 2008/11/27 05:56:24 lav Exp $ */

#ifndef PROCWAIT_H
#define PROCWAIT_H

#include <sys/types.h>
#include <signal.h>
#include "SMTask.h"

class ProcWait : public SMTask
{
public:
   enum	State
   {
      TERMINATED,
      RUNNING,
      ERROR
   };

protected:
   static ProcWait *chain;
   ProcWait *next;

   pid_t pid;
   State status;
   int	 term_info;
   int	 saved_errno;
   bool  auto_die;

   bool  handle_info(int info); // true if finished

public:
   int	 Do();
   State GetState() { return status; }
   int	 GetInfo() { return term_info; }
   int	 Kill(int sig=SIGTERM);

   void Auto() { auto_die=true; }

   ProcWait(pid_t p);
   ~ProcWait();

   static void SIGCHLD_handler(int);

   static void Signal(bool yes);

   static void DeleteAll();
};

#endif /* PROCWAIT_H */
