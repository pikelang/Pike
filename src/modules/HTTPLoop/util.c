/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#include <global.h>

#include "config.h"

#include <threads.h>
#include <fdlib.h>

#ifdef _REENTRANT
#include <stdlib.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "accept_and_parse.h"
#include "util.h"


int aap_get_time(void)
{
  static int t = 0;
  static int last_time;
  if(!(t++%10)) last_time = TIME(0);
  return last_time;
}

ptrdiff_t aap_swrite(int to, char *buf, size_t towrite)
{
  ptrdiff_t res=0;
  ptrdiff_t sent = 0;
  while(towrite)
  {
    while((res = fd_write(to, buf, towrite)) < 0)
    {
      switch(errno)
      {
       case EAGAIN:
       case EINTR:
	 continue;

       default:
	if(errno != EPIPE)
	  perror("accept_and_parse->request->shuffle: While writing");
	res = 0;
	return sent;
      }
    }
    towrite -= res;
    buf += res;
    sent += res;
  }
  return sent;
}

int aap_get_header(struct args *req, char *header, int operation, void *res)
{
  ptrdiff_t os=0;
  ptrdiff_t i, hl=strlen(header), l=req->res.body_start-req->res.header_start;
  char *in = req->res.data + req->res.header_start;
  for(i=0; i<l; i++)
  {
    switch(in[i])
    {
     case ':':
       /* in[os..i-1] == the header */
       if(i-os == hl)
       {
	 ptrdiff_t j;
	 for(j=0;j<hl; j++)
	   if((in[os+j]&95) != (header[j]&95))
	     break; /* no match */
	 if(j == hl)
	 {
	   struct pstring *r = res;
	   switch(operation)
	   {
	    case H_EXISTS:
	      return 1;
	    case H_INT:
	      *((int *)res) = atoi(in+i+1);
	      return 1;
	    case H_STRING:
	      os = i+1;
	      for(j=os;j<l;j++) if(in[j] == '\r') break;
	      /* okie.. */
	      while(in[os]==' ') os++;
	      r->len = j-os;
	      r->str = in+os;
	      return 1;
	   }
	 }
       }
     case '\r':
     case '\n':
       os = i+1;
    }
  }
  return 0;
} 
#endif
