/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "config.h"
#include "global.h"

#include "machine.h"
#include "module_support.h"
#include "object.h"
#include "stralloc.h"
#include "threads.h"
#include "fdlib.h"
#include "builtin_functions.h"

#ifdef _REENTRANT
#include <errno.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "pike_netlib.h"
#include "accept_and_parse.h"
#include "log.h"
#include "requestobject.h"
#include "util.h"


#define sp Pike_sp

static int num_log_entries;
static void free_log_entry( struct log_entry *le )
{
  num_log_entries--;
  free( le );
}

static struct log_entry *new_log_entry(ptrdiff_t extra)
{
  num_log_entries++;
  return malloc( sizeof( struct log_entry )+extra );
}


struct program *aap_log_object_program;
struct log *aap_first_log;

static void push_log_entry(struct log_entry *le)
{
  struct object *o = clone_object( aap_log_object_program, 0 );
  struct log_object *lo = (struct log_object*)o->storage;
  lo->time = le->t; /* FIXME: 32-bit truncation on 32-bit platforms. */
  lo->sent_bytes = le->sent_bytes;
  lo->reply = le->reply;
  lo->received_bytes = le->received_bytes;
  lo->raw = make_shared_binary_string(le->raw.str, le->raw.len);
  lo->url = make_shared_binary_string(le->url.str, le->url.len);
  lo->method = make_shared_binary_string(le->method.str, le->method.len);
  lo->protocol = le->protocol;
  le->protocol->refs++;
  {
#ifdef fd_inet_ntop
    char buffer[64];
    if (fd_inet_ntop(SOCKADDR_FAMILY(le->from),
		     SOCKADDR_IN_ADDR(le->from),
		     buffer, sizeof(buffer)))
      lo->from = make_shared_string(buffer);
    else
#endif
      lo->from = make_shared_string( inet_ntoa(*SOCKADDR_IN_ADDR(le->from)) );
  }
  push_object( o );
}

void f_aap_log_as_array(INT32 args)
{
  struct log_entry *le;
  struct log *l = LTHIS->log;
  int n = 0;
  pop_n_elems(args);

  THREADS_ALLOW();
  mt_lock( &l->log_lock );
  le = l->log_head;
  l->log_head = l->log_tail = 0;
  mt_unlock( &l->log_lock );
  THREADS_DISALLOW();

  while(le)
  {
    struct log_entry *l;
    n++;
    push_log_entry(le);
    l = le->next;
    free_log_entry(le);
    le = l;
  }
  {
    f_aggregate(n);
  }
}

void f_aap_log_exists(INT32 UNUSED(args))
{
  struct log *l = LTHIS->log;
  struct log_entry *le;

  THREADS_ALLOW();
  mt_lock( &l->log_lock );
  le = l->log_head;
  mt_unlock( &l->log_lock );
  THREADS_DISALLOW();

  push_int(!!le);
}

void f_aap_log_size(INT32 UNUSED(args))
{
  int n=1;
  struct log *l = LTHIS->log;
  struct log_entry *le;
  if(!l) {
    push_int(0);
    return;
  }
  THREADS_ALLOW();
  mt_lock( &l->log_lock );
  le = l->log_head;
  while((le = le->next))
    n++;
  mt_unlock( &l->log_lock );
  THREADS_DISALLOW();
  push_int(n);
}

void f_aap_log_as_commonlog_to_file(INT32 args)
{
  struct log_entry *le;
  struct log *l = LTHIS->log;
  int n = 0;
  int mfd;
  time_t ot = INT_MIN;
  struct object *f;
  struct tm tm;
  FILE *foo;
  static const char *month[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Oct", "Sep", "Nov", "Dec",
  };

  get_all_args(NULL, args, "%o", &f);
  f->refs++;

  pop_n_elems(args);
  apply(f, "query_fd", 0);
  mfd = fd_dup(sp[-1].u.integer);
  if(mfd < 1)Pike_error("Bad fileobject to ->log_as_commonlog_to_file\n");
  pop_stack();

  foo = fdopen( mfd, "w" );
  if(!foo)
    Pike_error("Bad fileobject to ->log_as_commonlog_to_file\n");

  THREADS_ALLOW();

  mt_lock( &l->log_lock );
  le = l->log_head;
  l->log_head = l->log_tail = 0;
  mt_unlock( &l->log_lock );

  memset(&tm, 0, sizeof(tm));

  while(le)
  {
    int i;
    struct log_entry *l = le->next;
    /* remotehost rfc931 authuser [date] "request" status bytes */
    if(le->t != ot)
    {
      time_t t = (time_t)le->t;
#ifdef HAVE_GMTIME_R
      gmtime_r( &t, &tm );
#else
      struct tm *tm_p;
      tm_p = gmtime( &t ); /* This will break if two threads run
			    gmtime() at once. */
      if (tm_p) tm = *tm_p;
#endif
      ot = le->t;
    }

    /* date format:  [03/Feb/1998:23:08:20 +0000]  */

    /* GET [URL] HTTP/1.0 */
    for(i=13; i<le->raw.len; i++)
      if(le->raw.str[i] == '\r')
      {
	le->raw.str[i] = 0;
	break;
      }

#ifdef fd_inet_ntop
    if(SOCKADDR_FAMILY(le->from) != AF_INET) {
      char buffer[64];
      fprintf(foo,
	      "%s - %s [%02d/%s/%d:%02d:%02d:%02d +0000] \"%s\" %d %ld\n",
	      fd_inet_ntop(SOCKADDR_FAMILY(le->from),
			   SOCKADDR_IN_ADDR(le->from),
			   buffer, sizeof(buffer)), /* hostname */
	      "-",                          /* remote-user */
	      tm.tm_mday, month[tm.tm_mon], tm.tm_year+1900,
	      tm.tm_hour, tm.tm_min, tm.tm_sec, /* date */
	      le->raw.str, /* request line */
	      le->reply, /* reply code */
              (long)le->sent_bytes); /* bytes transfered */
    } else
#endif /* fd_inet_ntop */
    fprintf(foo,
    "%d.%d.%d.%d - %s [%02d/%s/%d:%02d:%02d:%02d +0000] \"%s\" %d %ld\n",
	    ((unsigned char *)&le->from.ipv4.sin_addr)[ 0 ],
	    ((unsigned char *)&le->from.ipv4.sin_addr)[ 1 ],
	    ((unsigned char *)&le->from.ipv4.sin_addr)[ 2 ],
	    ((unsigned char *)&le->from.ipv4.sin_addr)[ 3 ], /* hostname */
	    "-",                          /* remote-user */
	    tm.tm_mday, month[tm.tm_mon], tm.tm_year+1900,
	    tm.tm_hour, tm.tm_min, tm.tm_sec, /* date */
	    le->raw.str, /* request line */
	    le->reply, /* reply code */
            (long)le->sent_bytes); /* bytes transfered */
    free_log_entry( le );
    n++;
    le = l;
  }
  fclose(foo);
  fd_close(mfd);
  THREADS_DISALLOW();
  push_int(n);
}

void aap_log_append(int sent, struct args *arg, int reply)
{
  struct log *l = arg->log;
  /* we do not incude the body, only the headers et al.. */
  struct log_entry *le=new_log_entry(arg->res.body_start-3);
  char *data_to=((char *)le)+sizeof(struct log_entry);

  le->t = aap_get_time();
  le->sent_bytes = sent;
  le->reply = reply;
  le->received_bytes = arg->res.body_start + arg->res.content_len;
  memcpy(data_to, arg->res.data, arg->res.body_start-4);
  le->raw.str = data_to;
  le->raw.len = arg->res.body_start-4;
  le->url.str = (data_to + (size_t)(arg->res.url-arg->res.data));
  le->url.len = arg->res.url_len;
  le->from = arg->from;
  le->method.str = data_to;
  le->method.len = arg->res.method_len;
  le->protocol = arg->res.protocol;
  le->next = 0;

  mt_lock( &l->log_lock );
  if(l->log_head)
  {
    l->log_tail->next = le;
    l->log_tail = le;
  }
  else
  {
    l->log_head = le;
    l->log_tail = le;
  }
  mt_unlock( &l->log_lock );
}

#endif
