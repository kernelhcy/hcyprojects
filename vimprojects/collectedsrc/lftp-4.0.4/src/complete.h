/*
 * lftp and utils
 *
 * Copyright (c) 1996-1999 by Alexander V. Lukyanov (lav@yars.free.net)
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

/* $Id: complete.h,v 1.5 2008/11/27 05:56:35 lav Exp $ */

#ifndef COMPLETE_H
#define COMPLETE_H

#include "trio.h"
#include "CmdExec.h"

extern CmdExec *completion_shell;
extern int remote_completion;
int   lftp_rl_getc(FILE *);
extern "C" void lftp_line_complete();
	
#endif //COMPLETE_H
