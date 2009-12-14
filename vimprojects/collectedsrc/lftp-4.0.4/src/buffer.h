/*
 * lftp and utils
 *
 * Copyright (c) 1998-2007 by Alexander V. Lukyanov (lav@yars.free.net)
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

/* $Id: buffer.h,v 1.59 2009/08/05 06:27:14 lav Exp $ */

#ifndef BUFFER_H
#define BUFFER_H

#include "SMTask.h"
#include "Filter.h"
#include "Timer.h"
#include "fg.h"
#include "xstring.h"
#include "Speedometer.h"

#include <stdarg.h>

#ifdef HAVE_ICONV
CDECL_BEGIN
# include <iconv.h>
CDECL_END
#endif

#define GET_BUFSIZE 0x10000
#define PUT_LL_MIN  0x2000

class Buffer
{
protected:
   xstring error_text;
   int  saved_errno;
   bool error_fatal;

   xstring buffer;
   int buffer_ptr;
   bool eof;	  // no reads possible (except from mem buffer)
   bool broken;	  // no writes possible

   bool save;  // save skipped data
   int save_max;

   off_t pos;

   SMTaskRef<Speedometer> rate;
   void RateAdd(int n);

   void Allocate(int size);

   void SaveMaxCheck(int addsize);

public:
   bool Error() const { return error_text!=0; }
   bool ErrorFatal() const { return error_fatal; }
   void SetError(const char *e,bool fatal=false);
   void SetErrorCached(const char *e);
   const char *ErrorText() const { return error_text; }
   int Size() const { return buffer.length()-buffer_ptr; }
   bool Eof() const { return eof; }
   bool Broken() const { return broken; }

   const char *Get() const;
   void Get(const char **buf,int *size) const;
   void Skip(int len); // Get(); consume; Skip()
   void UnSkip(int len); // this only works if there were no Put's.
   void Append(const char *buf,int size);
   void Put(const char *buf,int size);
   void Put(const char *buf) { Put(buf,strlen(buf)); }
   void Put(const xstring &s) { Put(s.get(),s.length()); }
   void Format(const char *f,...) PRINTF_LIKE(2,3);
   void vFormat(const char *f, va_list v);
   void PutEOF() { eof=true; }
   char *GetSpace(int size) {
      Allocate(size);
      return buffer.get_non_const()+buffer.length();
   }
   void SpaceAdd(int size) {
      buffer.set_length(buffer.length()+size);
   }
   void Prepend(const char *buf,int size);
   void Prepend(const char *buf) { Prepend(buf,strlen(buf)); }

   unsigned long long UnpackUINT64BE(int offset=0) const;
   unsigned UnpackUINT32BE(int offset=0) const;
   unsigned UnpackUINT16BE(int offset=0) const;
   unsigned UnpackUINT8(int offset=0) const;
   void PackUINT64BE(unsigned long long data);
   void PackUINT32BE(unsigned data);
   void PackUINT16BE(unsigned data);
   void PackUINT8(unsigned data);

   long long UnpackINT64BE(int offset=0) const;
   int UnpackINT32BE(int offset=0) const;
   int UnpackINT16BE(int offset=0) const;
   int UnpackINT8(int offset=0) const;
   void PackINT64BE(long long data);
   void PackINT32BE(int data);
   void PackINT16BE(int data);
   void PackINT8(int data);

   // useful for cache.
   void Save(int m) { save=true; save_max=m; }
   bool IsSaving() const { return save; }
   void GetSaved(const char **buf,int *size) const;
   void SaveRollback(off_t p);

   void SetPos(off_t p) { pos=p; }
   off_t GetPos() const { return pos; }

   void SetSpeedometer(Speedometer *s) { rate=s; }
   const char *GetRateStrS();

   void Empty();

   Buffer();
   ~Buffer();

   const char *Dump() const;
};

class DataTranslator : public Buffer
{
public:
   virtual void PutTranslated(Buffer *dst,const char *buf,int size)=0;
   virtual void ResetTranslation() { Empty(); }
   virtual ~DataTranslator() {}

   // same as PutTranslated, but does not advance pos.
   void AppendTranslated(Buffer *dst,const char *buf,int size);
};

#ifdef HAVE_ICONV
class DataRecoder : public DataTranslator
{
   iconv_t backend_translate;
public:
   void PutTranslated(Buffer *dst,const char *buf,int size);
   void ResetTranslation();
   DataRecoder(const char *from_code,const char *to_code,bool translit=true);
   ~DataRecoder();
};
#endif //HAVE_ICONV

class DirectedBuffer : public Buffer
{
public:
   enum dir_t { GET, PUT };

protected:
   Ref<DataTranslator> translator;
   dir_t mode;
   void EmbraceNewData(int len);

public:
   DirectedBuffer(dir_t m) : mode(m) {}
   void SetTranslator(DataTranslator *t) { translator=t; }
   void SetTranslation(const char *be_encoding,bool translit=true)
#ifdef HAVE_ICONV
      ;
#else
      {}
#endif //HAVE_ICONV
   void PutTranslated(const char *buf,int size);
   void PutTranslated(const char *buf) { PutTranslated(buf,strlen(buf)); }
   void PutTranslated(const xstring& s) { PutTranslated(s.get(),s.length()); }
   void ResetTranslation();
   void PutRaw(const char *buf,int size) { Buffer::Put(buf,size); }
   void PutRaw(const char *buf) { Buffer::Put(buf); }
   void Put(const char *buf,int size);
   void Put(const char *buf) { Put(buf,strlen(buf)); }
   dir_t GetDirection() { return mode; }
};

class IOBuffer : public DirectedBuffer, public SMTask
{
protected:
   // low-level for derived classes
   virtual int Get_LL(int size) { return 0; }
   virtual int Put_LL(const char *buf,int size) { return 0; }
   virtual int PutEOF_LL() { return 0; }

   Time event_time; // used to detect timeouts

public:
   IOBuffer(dir_t m);
   virtual ~IOBuffer();
   virtual const Time& EventTime()
      {
	 if(IsSuspended())
	    return now;
	 return event_time;
      }
   virtual bool Done()
      {
	 return(broken || Error() || (eof && (mode==GET || Size()==0)));
      }
   virtual int Do();

   virtual FgData *GetFgData(bool) { return 0; }
   virtual const char *Status() { return ""; }
   virtual int Buffered() { return Size(); }

   // Put method with Put_LL shortcut
   void Put(const char *,int);
   void Put(const char *buf);
   void Put(const xstring &s) { Put(s.get(),s.length()); }
   // anchor to PutEOF_LL
   void PutEOF() { DirectedBuffer::PutEOF(); PutEOF_LL(); }
};

class IOBufferStacked : public IOBuffer
{
   SMTaskRef<IOBuffer> down;

   int Get_LL(int size);
   int Put_LL(const char *buf,int size);

public:
   IOBufferStacked(IOBuffer *b) : IOBuffer(b->GetDirection()), down(b) {}
   const Time& EventTime() { return down->EventTime(); }
   int Do();
   bool Done();
};

class IOBufferFDStream : public IOBuffer
{
   Ref<FDStream> my_stream;
   const Ref<FDStream>& stream;
   Ref<Timer> put_ll_timer;

   int Get_LL(int size);
   int Put_LL(const char *buf,int size);

public:
   IOBufferFDStream(FDStream *o,dir_t m)
      : IOBuffer(m), my_stream(o), stream(my_stream) {}
   IOBufferFDStream(const Ref<FDStream>& o,dir_t m)
      : IOBuffer(m), stream(o) {}
   IOBufferFDStream(FDStream *o,dir_t m,Timer *t)
      : IOBuffer(m), my_stream(o), stream(my_stream), put_ll_timer(t) {}
   IOBufferFDStream(const Ref<FDStream>& o,dir_t m,Timer *t)
      : IOBuffer(m), stream(o), put_ll_timer(t) {}
   ~IOBufferFDStream();
   bool Done();
   FgData *GetFgData(bool fg);
   const char *Status() { return stream->status; }
};

#include <FileAccess.h>

class IOBufferFileAccess : public IOBuffer
{
   const FileAccessRef& session;

   int Get_LL(int size);

public:
   IOBufferFileAccess(const FileAccessRef& i) : IOBuffer(GET), session(i) {}

   void SuspendInternal();
   void ResumeInternal();
   const char *Status();
};

#endif // BUFFER_H
