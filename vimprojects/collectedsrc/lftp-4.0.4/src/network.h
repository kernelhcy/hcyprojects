/*
 * lftp and utils
 *
 * Copyright (c) 2008 by Alexander V. Lukyanov (lav@yars.free.net)
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

/* $Id: network.h,v 1.5 2009/09/15 07:56:30 lav Exp $ */

#ifndef NETWORK_H
#define NETWORK_H

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

union sockaddr_u
{
   struct sockaddr	sa;
   struct sockaddr_in	in;
#if INET6
   struct sockaddr_in6	in6;
#endif

   socklen_t addr_len() const {
      if(sa.sa_family==AF_INET)
	 return sizeof(in);
#if INET6
      if(sa.sa_family==AF_INET6)
	 return sizeof(in6);
#endif
      return sizeof(*this);
   }
   int operator==(const sockaddr_u &o) const {
      return !memcmp(this,&o,addr_len());
   }
   const char *address() const;
   int port() const;
   int bind_to(int s) { return bind(s,&sa,addr_len()); }
   sockaddr_u();
   bool is_reserved();
   bool is_multicast();
   bool is_loopback();
   bool is_private();
};

class Networker
{
protected:
   static void NonBlock(int fd);
   static void CloseOnExec(int fd);
   static void KeepAlive(int sock);
   static void MinimizeLatency(int sock);
   static void MaximizeThroughput(int sock);
   static void ReuseAddress(int sock);
   static int SocketBuffered(int sock);
   static const char *SocketNumericAddress(const sockaddr_u *u) { return u->address(); }
   static int SocketPort(const sockaddr_u *u) { return u->port(); }
   static socklen_t SocketAddrLen(const sockaddr_u *u) { return u->addr_len(); }
   static int SocketConnect(int fd,const sockaddr_u *u);
   static int SocketAccept(int fd,sockaddr_u *u,const char *hostname=0);
   static void SetSocketBuffer(int sock,int socket_buffer);
   static void SetSocketMaxseg(int sock,int socket_maxseg);
   static void SocketBindStd(int s,int af,const char *hostname);
   static int SocketCreate(int af,int type,int proto,const char *hostname);
   static void SocketTuneTCP(int s,const char *hostname);
   static int SocketCreateTCP(int af,const char *hostname);
   static int SocketCreateUnbound(int af,int type,int proto,const char *hostname);
   static int SocketCreateUnboundTCP(int af,const char *hostname);
};

#endif //NETWORK_H
