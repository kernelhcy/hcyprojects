/*
 * lftp and utils
 *
 * Copyright (c) 1996-1998 by Alexander V. Lukyanov (lav@yars.free.net)
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

/* $Id: history.h,v 1.6 2008/11/27 05:56:37 lav Exp $ */

#ifndef HISTORY_H
#define HISTORY_H

#include "keyvalue.h"
#include "FileAccess.h"

class History : public KeyValueDB
{
   KeyValueDB *full;
   time_t stamp;

   xstring file;
   int fd;
   bool modified;

   void Load();
   void Close();
   void Refresh();

   const char *extract_url(const char *res);
   time_t extract_stamp(const char *res);

public:
   void Set(const FileAccess *s,const FileAccess::Path &p);
   void Set(const FileAccess *s) { Set(s,s->GetCwd()); }
   const char *Lookup(const FileAccess *s);
   void Save();
   History();
   ~History();

   void Rewind()
      {
	 Refresh();
	 KeyValueDB::Rewind();
      }
};

#endif //HISTORY_H
