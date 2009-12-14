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

#include <config.h>
#include <stdio.h>
#include "Bencode.h"
#include "c-ctype.h"

BeNode::BeNode(long long n)
   : type(BE_INT),num(n)
{}
BeNode::BeNode(const char *s,int len)
   : type(BE_STR),str(s,len)
{}
BeNode::BeNode(xarray_p<BeNode> *a)
   : type(BE_LIST)
{
   list.move_here(*a);
}
BeNode::BeNode(xmap_p<BeNode> *m)
   : type(BE_DICT)
{
   dict.move_here(*m);
}

BeNode *BeNode::Parse(const char *s,int s_len,int *rest)
{
   if(s_len<2)
      return 0;
   switch(*s)
   {
   case 'i':
   {
      s++;
      s_len--;
      bool neg=false;
      if(*s=='-') {
	 neg=true;
	 s++;
	 s_len--;
      }
      if(s_len<2)
	 return 0;
      if(c_isdigit(*s))
      {
	 if(*s=='0' && s[1]!='e')
	    return 0;
	 long long n=*s++-'0';
	 s_len--;
	 while(s_len>1 && c_isdigit(*s)) {
	    n=n*10+*s++-'0';
	    s_len--;
	 }
	 if(s_len<1 || *s!='e')
	    return 0;
	 *rest=s_len-1;
	 return new BeNode(neg?-n:n);
      }
      return 0;
   }
   case 'l':
   {
      s++;
      s_len--;
      xarray_p<BeNode> a;
      while(s_len>1 && *s!='e')
      {
	 int rest1;
	 BeNode *n=Parse(s,s_len,&rest1);
	 if(!n)
	    return 0;
	 a.append(n);
	 s+=(s_len-rest1);
	 s_len=rest1;
      }
      if(s_len<1 || *s!='e')
	 return 0;
      *rest=s_len-1;
      return new BeNode(&a);
   }
   case 'd':
   {
      const char *d_begin=s;
      s++;
      s_len--;
      xmap_p<BeNode> map;
      while(s_len>1 && *s!='e')
      {
	 int rest1;
	 BeNode *n=Parse(s,s_len,&rest1);
	 if(!n || n->type!=BE_STR)
	    return 0;
	 s+=(s_len-rest1);
	 s_len=rest1;
	 BeNode *v=Parse(s,s_len,&rest1);
	 if(!v)
	    return 0;
	 map.add(n->str,v);
	 delete n;
	 s+=(s_len-rest1);
	 s_len=rest1;
      }
      if(s_len<1 || *s!='e')
	 return 0;
      s++;
      s_len--;
      *rest=s_len;
      BeNode *node=new BeNode(&map);
      node->str.nset(d_begin,s-d_begin);
      return node;
   }
   default:
      if(c_isdigit(*s))
      {
	 int n=*s++-'0';
	 s_len--;
	 while(s_len>0 && c_isdigit(*s)) {
	    if(n>=s_len)
	       return 0;
	    n=n*10+*s++-'0';
	    s_len--;
	 }
	 if(s_len<1 || *s!=':')
	    return 0;
	 s++;
	 s_len--;
	 if(s_len<n)
	    return 0;
	 *rest=s_len-n;
	 return new BeNode(s,n);
      }
      return 0;
   }
}

BeNode::~BeNode()
{
   for(int i=0; i<list.count(); i++) {
      delete list[i];
      list[i]=0;
   }
   for(BeNode *e=dict.each_begin(); e; e=dict.each_next())
   {
      delete e;
      dict.each_set(0);
   }
}

void BeNode::Format(xstring &buf,int level)
{
   int i;
   for(i=0; i<level; i++)
      buf.append('\t');
   switch(type)
   {
   case BE_STR:
      buf.append("STR: ");
      (str_lc?str_lc:str).dump_to(buf);
      buf.append("\n");
      break;
   case BE_INT:
      buf.appendf("INT: %lld\n",num);
      break;
   case BE_LIST:
      buf.appendf("LIST: %d items\n",list.count());
      for(i=0; i<list.count(); i++)
	 list[i]->Format(buf,level+1);
      break;
   case BE_DICT:
      buf.appendf("DICT: %d items\n",dict.count());
      for(BeNode *e=dict.each_begin(); e; e=dict.each_next())
      {
	 for(i=0; i<level+1; i++)
	    buf.append('\t');
	 buf.appendf("KEY=%s:\n",dict.each_key()->get());
	 e->Format(buf,level+2);
      }
      break;
   }
}

const char *BeNode::Format()
{
   static xstring buf;
   buf.set("");
   Format(buf,0);
   return buf;
}

const char *BeNode::TypeName(be_type_t t)
{
   static const char *table[]={
      "STR",
      "INT",
      "LIST",
      "DICT"
   };
   return table[t];
}
