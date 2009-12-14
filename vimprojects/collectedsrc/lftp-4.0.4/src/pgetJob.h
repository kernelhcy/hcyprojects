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

/* $Id: pgetJob.h,v 1.19 2008/11/27 05:56:44 lav Exp $ */

#ifndef PGETJOB_H
#define PGETJOB_H

#include "CopyJob.h"

class pgetJob : public CopyJob
{
   class ChunkXfer : public CopyJob
   {
      friend class pgetJob;

      off_t start;
      off_t limit;

      ChunkXfer(FileCopy *c,const char *n,off_t start,off_t limit);
   };

   TaskRefArray<ChunkXfer> chunks;
   int	 max_chunks;
   off_t chunks_bytes;
   void InitChunks(off_t offset,off_t size);

   off_t start0;
   off_t limit0;

   off_t total_xferred;
   float total_xfer_rate;

   bool	no_parallel:1;
   bool chunks_done:1;
   bool pget_cont:1;

   void free_chunks();
   ChunkXfer *NewChunk(const char *remote,off_t start,off_t limit);

   long total_eta;

   Timer status_timer;
   xstring status_file;
   void SaveStatus();
   void LoadStatus();
   void LoadStatus0();

protected:
   void NextFile();

public:
   int Do();
   void ShowRunStatus(const SMTaskRef<StatusLine>&);
   void PrintStatus(int,const char *);
   void ListJobs(int verbose,int indent=0);

   pgetJob(FileCopy *c1,const char *n,int m=0);
   ~pgetJob();

   void SetMaxConn(int n) { max_chunks=n; }

   off_t GetBytesCount() { return chunks_bytes+c->GetBytesCount(); }
};

class pCopyJobCreator : public CopyJobCreator
{
public:
   int max_chunks;
   pCopyJobCreator(int n) : max_chunks(n) {}
   CopyJob *New(FileCopy *c,const char *n,const char *o) const {
      return new pgetJob(c,n,max_chunks);
   }
};

#endif//PGETJOB_H
