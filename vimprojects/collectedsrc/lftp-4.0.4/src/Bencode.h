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

/* $Id: xarray.h,v 1.10 2009/03/20 10:22:38 lav Exp $ */

#ifndef BENCODE_H
#define BENCODE_H

#include "xmap.h"

class BeNode
{
public:
   enum be_type_t {
      BE_STR,
      BE_INT,
      BE_LIST,
      BE_DICT
   } type;

   xstring str;
   xstring str_lc;
   xarray_p<BeNode> list;
   xmap_p<BeNode> dict;
   long long num;

   static BeNode *Parse(const char *s,int len,int *rest);

   BeNode(const char *s,int l);
   BeNode(xarray_p<BeNode> *l);
   BeNode(xmap_p<BeNode> *d);
   BeNode(long long);
   ~BeNode();

   void Format(xstring &buf,int level);
   const char *Format();

   static const char *TypeName(be_type_t t);
};

#endif//BENCODE_H
