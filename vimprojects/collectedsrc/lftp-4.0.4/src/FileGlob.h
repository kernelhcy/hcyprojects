/*
 * lftp and utils
 *
 * Copyright (c) 1996-2001 by Alexander V. Lukyanov (lav@yars.free.net)
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

/* $Id: FileGlob.h,v 1.9 2008/11/27 05:56:14 lav Exp $ */

#ifndef GLOB_H
#define GLOB_H

#include "FileAccess.h"

class Glob : public FileAccessOperation
{
protected:
   xstring_c pattern;
   FileSet list;
   bool	 dirs_only;
   bool	 files_only;
   bool	 match_period;
   bool	 inhibit_tilde;
   bool	 casefold;
   void	 add(const FileInfo *info);
   void	 add_force(const FileInfo *info);
public:
   const char *GetPattern() { return pattern; }
   FileSet *GetResult() { return &list; }
   Glob(FileAccess *s,const char *p);
   ~Glob();
   void DirectoriesOnly() { dirs_only=true; }
   void FilesOnly() { files_only=true; }
   void NoMatchPeriod() { match_period=false; }
   void NoInhibitTilde() { inhibit_tilde=false; }
   void CaseFold() { casefold=true; }

   static bool HasWildcards(const char *);
   static void UnquoteWildcards(char *);
};
class NoGlob : public Glob
{
public:
   NoGlob(const char *p);
   const char *Status() { return ""; }
   int Do();
};
class GlobURL
{
   const FileAccessRef& orig_session;
   FileAccessRef my_session;
   FileAccessRefC session;
   xstring_c url_prefix;
public:
   SMTaskRef<Glob> glob;

   enum type_select
   {
      ALL,
      FILES_ONLY,
      DIRS_ONLY
   };

   GlobURL(const FileAccessRef& s,const char *p,type_select t=ALL);
   ~GlobURL();
   FileSet *GetResult();
   bool Done()  { return glob->Done(); }
   bool Error() { return glob->Error(); }
   const char *ErrorText() { return glob->ErrorText(); }
   const char *Status() { return glob->Status(); }

   void NewGlob(const char *p);
   const char *GetPattern() { return glob->GetPattern(); }

   void NoMatchPeriod()	   { if(glob) glob->NoMatchPeriod(); }
   void NoInhibitTilde()   { if(glob) glob->NoInhibitTilde(); }
   void CaseFold()	   { if(glob) glob->CaseFold(); }

private:
   type_select type;
};

class GenericGlob : public Glob
{
   const char *curr_dir;
   FileSet *dir_list;
   SMTaskRef<Glob> updir_glob;
   SMTaskRef<ListInfo> li;

public:
   int	 Do();
   const char *Status();

   GenericGlob(FileAccess *session,const char *n_pattern);
};

#endif