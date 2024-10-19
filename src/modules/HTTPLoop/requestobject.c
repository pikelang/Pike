/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "config.h"

#include "machine.h"
#include "mapping.h"
#include "module_support.h"
#include "multiset.h"
#include "object.h"
#include "operators.h"
#include "pike_memory.h"
#include "constants.h"
#include "stralloc.h"
#include "svalue.h"
#include "threads.h"
#include "fdlib.h"
#include "fd_control.h"
#include "builtin_functions.h"
#include "bignum.h"

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
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif


#define sp Pike_sp

#ifdef _REENTRANT
#include "pike_netlib.h"
#include "accept_and_parse.h"
#include "log.h"
#include "util.h"
#include "cache.h"
#include "requestobject.h"

/* Used when Pike_fatal() can't be. */
#define FATAL(X)	fd_write(2, X, sizeof(X) - sizeof(""))

#ifdef AAP_DEBUG
#define DWERROR(...) fprintf(stderr, __VA_ARGS__)
#else
#define DWERROR(...)
#endif

/* All current implementations of sendfile(2) are broken. */
#ifndef HAVE_BROKEN_SENDFILE
#define HAVE_BROKEN_SENDFILE
#endif /* !HAVE_BROKEN_SENDFILE */

#ifdef HAVE_BROKEN_SENDFILE
#ifdef HAVE_SENDFILE
#undef HAVE_SENDFILE
#endif /* HAVE_SENDFILE */
#ifdef HAVE_FREEBSD_SENDFILE
#undef HAVE_FREEBSD_SENDFILE
#endif /* HAVE_FREEBSD_SENDFILE */
#ifdef CAN_HAVE_SENDFILE
#undef CAN_HAVE_SENDFILE
#endif /* CAN_HAVE_SENDFILE */
#ifdef CAN_HAVE_LINUX_SYSCALL4
#undef CAN_HAVE_LINUX_SYSCALL4
#endif /* CAN_HAVE_LINUX_SYSCALL4 */
#ifdef CAN_HAVE_NONSHARED_LINUX_SYSCALL4
#undef CAN_HAVE_NONSHARED_LINUX_SYSCALL4
#endif /* CAN_HAVE_NONSHARED_LINUX_SYSCALL4 */
#endif /* HAVE_BROKEN_SENDFILE */

/* Define local global string variables ... */
#define STRING(X,Y) /*extern*/ struct pike_string *X
#include "static_strings.h"
#undef STRING
/* there.. */

#define SINSERT(MAP,INDEX,VAL) \
do { 						\
  push_string(VAL);				\
  push_string(INDEX);                           \
  mapping_insert((MAP),(sp-1),(sp-2));		\
  sp-=2;                                        \
} while(0)

#define IINSERT(MAP,INDEX,VAL)\
do { 						\
  push_int64(VAL);				\
  push_string(INDEX);                           \
  mapping_insert((MAP),sp-1,sp-2);		\
  sp -= 2;                                      \
} while(0)

#define TINSERT(MAP,INDEX,VAL,LEN)\
do { 						   \
  push_string(make_shared_binary_string((VAL),(LEN))); \
  push_string(INDEX);                           \
  mapping_insert((MAP),sp-1,sp-2);		\
  sp--;                                          \
  pop_stack();					   \
} while(0)

static int dhex(char what)
{
  if(what >= '0' && what <= '9') return what - '0';
  if(what >= 'A' && what <= 'F')  return 10+what-'A';
  if(what >= 'a' && what <= 'f')  return 10+what-'a';
  return 0;
}


void f_aap_scan_for_query(INT32 args)
{
  char *s, *work_area;
  ptrdiff_t len, i,j;
  int c;
  if(args)
  {
    struct pike_string *_s;
    get_all_args(NULL, args, "%n", &_s);
    s = (char *)_s->str;
    len = _s->len;
  }
  else
  {
    s = THIS->request->res.url;
    len = THIS->request->res.url_len;
  }

  /*  [http://host:port]/[url[?query]] */
  work_area = malloc(len);

  /* find '?' if any */
  for(j=i=0;i<len;i++)
  {
    switch(c=s[i])
    {
     case '?':
       goto done;
     case '%':
       if(i<len-2)
       {
	 c = (dhex(s[i+1])<<4) + dhex(s[i+2]);
	 i+=2;
       }
    }
    work_area[j++]=c;
  }

 done:
  TINSERT(THIS->misc_variables, s_not_query, work_area, j);
  free(work_area);

  if(i < len)
    TINSERT(THIS->misc_variables, s_query, s+i+1, (len-i)-1);
  else
    IINSERT(THIS->misc_variables, s_query, 0);

  push_string(s_variables);  map_delete(THIS->misc_variables, sp-1); sp--;
  push_string(s_rest_query); map_delete(THIS->misc_variables, sp-1); sp--;
}

#define VAR_MAP_INSERT() 	 \
         struct svalue *s;						 \
	 push_string(make_shared_binary_string( dec+lamp, leq-lamp));	 \
	 if((s = low_mapping_lookup(v, sp-1)))				 \
	 {								 \
	   dec[leq] = 0;						 \
	   s->u.string->refs++;						 \
	   map_delete(v, sp-1);						 \
	   push_string(s->u.string);					 \
	   push_string(make_shared_binary_string( dec+leq, dl-leq));	 \
	   f_add(2);							 \
	 } else 							 \
	   push_string(make_shared_binary_string( dec+leq+1, dl-leq-1)); \
	 mapping_insert(v, sp-2, sp-1);					 \
	 pop_n_elems(2)


static void decode_x_url_mixed(char *in, ptrdiff_t l, struct mapping *v,
			       char *dec, char *UNUSED(rest_query), char **rp)
{
  ptrdiff_t pos = 0, lamp = 0, leq=0, dl;

  for(dl = pos = 0; pos<l; pos++, dl++)
  {
    unsigned char c;
    switch(c=in[pos])
    {
     case '=': leq = dl; break;
     case '+': c = ' '; break;
     case '&':
       if(leq)
       {
	 VAR_MAP_INSERT();
       } else if(rp) {
	 ptrdiff_t i;
	 for(i=lamp-1;i<dl;i++) *((*rp)++)=dec[i];
       }
       leq = 0;
       lamp = dl+1;
       break;
     case '%':
       if(pos<l-2)
       {
	 c = (dhex(in[pos+1])<<4) + dhex(in[pos+2]);
	 pos+=2;
       }
    }
    dec[dl]=c;
  }
  if(leq)
  {
    VAR_MAP_INSERT();
  } else if(rp) {
    ptrdiff_t i;
    for(i=lamp-1;i<dl;i++) *((*rp)++)=dec[i];
  }
}

static void parse_query(void)
{
  struct svalue *q;
  struct mapping *v = allocate_mapping(10); /* variables */
  push_string(s_query);
  if(!(q = low_mapping_lookup(THIS->misc_variables, sp-1))) {
    f_aap_scan_for_query(0);
    /* q will not be 0 below, as we have inserted the value now */
    q = low_mapping_lookup(THIS->misc_variables, sp-1);
  }
  sp--;

  if(TYPEOF(*q) == T_STRING)
  {
    char *dec = malloc(q->u.string->len*2+1);
    char *rest_query = dec+q->u.string->len+1, *rp=rest_query;
    decode_x_url_mixed(q->u.string->str,q->u.string->len,v,dec,rest_query,&rp);
    push_string(make_shared_binary_string(rest_query,rp-rest_query));
    push_string(s_rest_query);
    mapping_insert(THIS->misc_variables, sp-1, sp-2);
    sp--; pop_stack();
    free(dec);
  } else {
    push_int(0); push_string(s_rest_query);
    mapping_insert(THIS->misc_variables, sp-1, sp-2);
    sp--; pop_stack();
  }

  push_mapping(v); push_string(s_variables);
  mapping_insert(THIS->misc_variables, sp-1, sp-2);
  sp--; pop_stack();
}

static void parse_headers(void)
{
  struct svalue *tmp;
  struct mapping *headers = THIS->done_headers;
  ptrdiff_t os=0, i, j, l = THIS->request->res.body_start-THIS->request->res.header_start;
  unsigned char *in=
    (unsigned char*)THIS->request->res.data + THIS->request->res.header_start;

  THIS->headers_parsed = 1;
  for(i=0; i<l; i++)
  {
    switch(in[i])
    {
     case ':':
       /* in[os..i-1] == the header */
       for(j=os;j<i;j++) if(in[j] > 63 && in[j] < 91) in[j]+=32;
       push_string(make_shared_binary_string((char*)in+os,i-os));
       os = i+1;
       while(in[os]==' ') os++;
       for(j=os;j<l;j++) if(in[j] == '\r') break;
       push_string(make_shared_binary_string((char*)in+os,j-os));
       f_aggregate(1);
       if((tmp = low_mapping_lookup(headers, sp-2)))
       {
	 tmp->u.array->refs++;
	 push_array(tmp->u.array);
	 map_delete(headers, sp-3);
	 f_add(2);
       }
       mapping_insert(headers, sp-2, sp-1);
       pop_n_elems(2);
       os = i = j+2;
    }
  }
}

void f_aap_index_op(INT32 args)
{
  struct svalue *res;
  struct pike_string *s;
  if (!args) Pike_error("HTTP C object->`[]: too few args\n");
  pop_n_elems(args-1);
  if(!THIS->misc_variables)
  {
    struct svalue s;
    object_index_no_free2(&s, Pike_fp->current_object, 0, sp-1);
    pop_stack();
    *sp=s;
    sp++;
    return;
  }

  if((res = low_mapping_lookup(THIS->misc_variables, sp-1)))
  {
    pop_stack();
    assign_svalue_no_free( sp++, res );
    return;
  }

  if(!THIS->request) Pike_error("Reply called. No data available\n");
  get_all_args(NULL, args, "%n", &s);

  if(s == s_not_query || s==s_query )
  {
    f_aap_scan_for_query(0);
    f_aap_index_op(1);
    return;
  }

  if(s == s_my_fd)
  {
    /* 0x3800 is from modules/files/file.h,
     * FILE_READ|FILE_WRITE|FILE_SET_CLOSE_ON_EXEC
     */
    push_object(file_make_object_from_fd
		(fd_dup(THIS->request->fd),0x3800,
		 SOCKET_CAPABILITIES|PIPE_CAPABILITIES));
    push_string(s_my_fd);
    mapping_insert(THIS->misc_variables, sp-1, sp-2);
    sp--;
    return; /* the wanted value is on the stack now */
  }

  if(s == s_remoteaddr)
  {
#ifdef fd_inet_ntop
    char buffer[64];
    if (fd_inet_ntop(SOCKADDR_FAMILY(THIS->request->from),
		     SOCKADDR_IN_ADDR(THIS->request->from),
		     buffer, sizeof(buffer)))
      push_text(buffer);
    else
#endif
      push_text(inet_ntoa(*SOCKADDR_IN_ADDR(THIS->request->from)));

    push_string(s_remoteaddr);
    mapping_insert(THIS->misc_variables, sp-1, sp-2);
    sp--;
    return;
  }

  if(s == s_method)
  {
    push_string(make_shared_binary_string(THIS->request->res.data,
					  THIS->request->res.method_len));
    push_string(s_method);
    mapping_insert(THIS->misc_variables, sp-1, sp-2);
    sp--;
    return;
  }

  if(s == s_raw)
  {
    pop_stack();
    push_string(make_shared_binary_string(THIS->request->res.data,
					  THIS->request->res.data_len));
    push_string(s_raw);
    mapping_insert(THIS->misc_variables, sp-1, sp-2);
    sp--;
    return;
  }

  if(s == s_rest_query || s == s_variables)
  {
    parse_query();
    f_aap_index_op(1);
    return;
  }

  if(s == s_headers)
  {
    if(!THIS->headers_parsed) parse_headers();
    THIS->done_headers->refs++;
    push_mapping(THIS->done_headers);
    return;
  }

  if(s == s_pragma)
  {
    struct svalue *tmp;
    if(!THIS->headers_parsed) parse_headers();
    if((tmp = low_mapping_lookup(THIS->done_headers, sp-1)))
    {
      pop_stack();
      push_multiset(mkmultiset( tmp->u.array ));
    } else {
      pop_stack();
      f_aggregate_multiset(0);
    }
    push_string(s_pragma);
    mapping_insert(THIS->misc_variables, sp-1, sp-2);
    sp--;
    return;
  }

  if(s == s_client)
  {
    struct svalue *tmp;
    if(!THIS->headers_parsed) parse_headers();
    pop_stack();
    push_string(s_user_agent);
    if((tmp = low_mapping_lookup(THIS->done_headers, sp-1)))
      assign_svalue_no_free( sp-1, tmp );
    else
    {
      sp--;
      f_aggregate(0);
    }
    push_string(s_client);
    mapping_insert(THIS->misc_variables, sp-1, sp-2);
    sp--;
    return;
  }

  if(s == s_referer)
  {
    struct svalue *tmp;
    if(!THIS->headers_parsed) parse_headers();
    if((tmp = low_mapping_lookup(THIS->done_headers, sp-1)))
      assign_svalue( sp-1, tmp );
    else
    {
      pop_stack();
      push_int(0);
    }
    push_string(s_referer);
    mapping_insert(THIS->misc_variables, sp-1, sp-2);
    sp--;
    return;
  }

  if(s == s_since)
  {
    struct svalue *tmp;
    if(!THIS->headers_parsed) parse_headers();
    pop_stack();
    push_string(s_if_modified_since);
    if((tmp = low_mapping_lookup(THIS->done_headers, sp-1)))
    {
      tmp->u.array->item[0].u.string->refs++;
      sp[-1] = tmp->u.array->item[0];
    }
    else
    {
      SET_SVAL(sp[-1], T_INT, NUMBER_NUMBER, integer, 0);
    }
    push_string(s_since);
    mapping_insert(THIS->misc_variables, sp-1, sp-2);
    sp--;

    f_aap_index_op(1);
    return;
  }

  if(s == s_data)
  {
    pop_stack();
    push_string(make_shared_binary_string(THIS->request->res.data
					  +  THIS->request->res.body_start,
					  THIS->request->res.content_len));
    push_string(s_data);
    mapping_insert(THIS->misc_variables, sp-1, sp-2);
    sp--;
    return;
  }
  {
    struct svalue s;
    object_index_no_free2(&s, Pike_fp->current_object, 0, sp-1);
    pop_stack();
    *sp=s;
    sp++;
  }
}

void f_aap_end(INT32 UNUSED(args))
{
  /* end connection. */
}

void f_aap_output(INT32 UNUSED(args))
{
  if(TYPEOF(sp[-1]) != T_STRING) Pike_error("Bad argument 1 to output\n");
  WRITE(THIS->request->fd, sp[-1].u.string->str, sp[-1].u.string->len);
}

#define BUFFER 8192
static struct thread_args *done;

struct send_args
{
  struct args *to;
  int from_fd;
  struct pike_string *data;
  ptrdiff_t len;
  ptrdiff_t sent;
  char buffer[BUFFER];
};

static int num_send_args;
static struct send_args *new_send_args(void)
{
  num_send_args++;
  return malloc( sizeof( struct send_args ) );
}

static void free_send_args(struct send_args *s)
{
  num_send_args--;
  if( s->data )    aap_enqueue_string_to_free( s->data );
  if( s->from_fd ) fd_close( s->from_fd );
  free( s );
}

/* WARNING! This function is running _without_ any stack etc. */
static void actually_send(struct send_args *a)
{
  int first=0;
  int oldbulkmode = 0;
  char foo[10];
  unsigned char *data = NULL;
  ptrdiff_t fail, data_len = 0;
  foo[9]=0; foo[6]=0;
  if( a->data )
  {
    data = (unsigned char *)a->data->str;
    data_len = a->data->len;
  }
#ifdef HAVE_FREEBSD_SENDFILE
  if (a->len)
  {
    struct iovec vec;
    struct sf_hdtr headers = { NULL, 0, NULL, 0 };
    off_t off = 0;
    off_t sent = 0;
    size_t len = a->len;

    DWERROR("sendfile... \n");

    if (data) {
      /* Set up the iovec */
      vec.iov_base = data;
      vec.iov_len = data_len;
      headers.headers = &vec;
      headers.hdr_cnt = 1;
    }

    if (a->len < 0) {
      /* Negative: Send all */
      len = 0;
    }

    if ((off = lseek(a->from_fd, 0, SEEK_CUR)) < 0) {
      /* Probably a pipe, so sendfile() will probably fail anyway,
       * but it doesn't hurt to try...
       */
      off = 0;
    }

    if (sendfile(a->from_fd, a->to->fd, off, len,
		 &headers, &sent, 0) < 0) {
      /* FIXME: We aren't looking very hard at the errno, since
       * sent usually tells us what to do.
       */
      switch(errno) {
      case EBADF:	/* Either of the fds is invalid */
      case ENOTSOCK:	/* to_fd is not a socket */
      case EINVAL:	/* from_fd is not a file, to_fd not a SOCK_STREAM,
			 * or offset is negative or out of range.
			 */
      case ENOTCONN:	/* to_fd is not a connected socket */
      case EPIPE:	/* to_fd is closed at the other end */
      case EIO:		/* Error reading from from_fd */
	if (!sent) {
	  /* Probably a bad fd-combo. Try sending by hand. */
	  goto send_data;
	}
	break;
      case EFAULT:	/* Invalid address specified as arg */
	/* NOTE: Can't use Pike_fatal(), since we're not in a valid Pike context. */
        FATAL("FreeBSD-style sendfile() returned EFAULT.\n");
	abort();
	break;
      case EAGAIN:	/* socket in nonblocking mode, sent is valid */
	break;
      }
    }

    /* At this point sent will always contain the actual number
     * of bytes sent.
     */
    a->sent += sent;

    if (data) {
      if (sent < data_len) {
	data += sent;
	data_len -= sent;
	sent = 0;

	/* Make sure we don't terminate early due to len being 0. */
	goto send_data;
      } else {
	sent -= data_len;
	data = NULL;
	data_len = 0;
      }
    }

    if (len) {
      a->len -= sent;
      if (!a->len) {
	goto end;
      }
      goto normal;
    } else {
      /* Assume all was sent */
      a->len = 0;
      goto end;
    }
  }

 send_data:
#endif /* !HAVE_FREEBSD_SENDFILE */

  DWERROR("actually_send... \n");
  if(data)
  {
    memcpy(foo, data+MINIMUM((data_len-4),9), 4);
    first=1;
    DWERROR("cork... \n");
    oldbulkmode = bulkmode_start(a->to->fd);
    fail = WRITE(a->to->fd, (char *)data, data_len);
    a->sent += fail;
    if(fail != data_len)
      goto end;
  }
  fail = 0;

#if !defined(HAVE_FREEBSD_SENDFILE) && defined(HAVE_SENDFILE)
  if(a->len)
  {
    DWERROR("pre sendfile... \n");
    if(!first)
    {
      first=1;
      fail = fd_read(a->from_fd, foo, 10);
      if(fail < 0)
        goto end;
      WRITE( a->to->fd, foo, fail );
      a->len -= fail;
    }
    DWERROR("sendfile... \n");
    switch(fail = sendfile(a->to->fd, a->from_fd, NULL, a->len ))
    {
     case -ENOSYS:
       DWERROR("syscall does not exist.\n");
       goto normal;
     default:
       if(fail != a->len)
         fprintf(stderr, "sendfile returned %ld; len=%ld\n",
                 (long)fail, (long)a->len);
    }
    goto end;
  }
#endif /* HAVE_SENDFILE && !HAVE_FREEBSD_SENDFILE */

#if defined(HAVE_FREEBSD_SENDFILE) || defined(HAVE_SENDFILE)
 normal:
#endif /* HAVE_FREEBSD_SENDFILE || HAVE_SENDFILE */

  DWERROR("using normal fallback... \n");
#ifdef DIRECTIO_ON
  if(a->len > (65536*4))
    directio(a->from_fd, DIRECTIO_ON);
#endif

  /*
   * Ugly optimization...
   * Removes the sign-bit from the len.
   * => -1 => 0x7fffffff, which means that cgi will work.
   *
   *	/grubba 1998-08-30
   */
  a->len &= 0x7fffffff;

  while(a->len)
  {
    ptrdiff_t nread, written=0;
    nread = fd_read(a->from_fd, a->buffer, MINIMUM(BUFFER,a->len));
    DWERROR("writing %d bytes... \n", nread);
    if(!first)
    {
      first=1;
      memcpy(foo,a->buffer+9,5);
    }
    if(nread <= 0)
    {
      if((nread < 0) && (errno == EINTR))
	continue;
      else
      {
	/* CGI's will come here at eof (nread == 0),
	 * and get keep-alive disabled.
	 */
	fail = 1;
	break;
      }
    }
    if(fail || ((written=WRITE(a->to->fd, a->buffer, nread)) != nread))
      goto end;
    a->len -= nread;
    a->sent += written;
  }

 end:
  DWERROR("all written.. \n");
  bulkmode_restore(a->to->fd, oldbulkmode);
  {
    struct args *arg = a->to;
    LOG(a->sent, a->to, atoi(foo));
    free_send_args( a );

    if(!fail &&
       ((arg->res.protocol==s_http_11)||
        aap_get_header(arg, "connection", H_EXISTS, 0)))
    {
      aap_handle_connection(arg);
    }
    else
    {
      DWERROR("no keep alive, closing fd.. \n");
      free_args( arg );
    }
  }
}

void f_aap_reply(INT32 args)
{
  int reply_string=0, reply_object=0;
  struct send_args *q;
  if(!THIS->request)
    Pike_error("reply already called.\n");
  if(args && TYPEOF(sp[-args]) == T_STRING)
    reply_string = 1;

  if(args>1)
  {
    if(args<3)
      Pike_error("->reply(string|void pre,object(Stdio.file) fd,int len)\n");
    if(TYPEOF(sp[-args+1]) != T_OBJECT)
      Pike_error("Bad argument 2 to reply\n");
    if(TYPEOF(sp[-args+2]) != T_INT)
      Pike_error("Bad argument 3 to reply\n");
    reply_object = 1;
  }

  q = new_send_args();
  q->to = THIS->request;
  THIS->request = 0;

  if(reply_object)
  {
    /* safe_apply() needed to avoid leak of q */
    safe_apply(sp[-2].u.object, "query_fd", 0);
    if((TYPEOF(sp[-1]) != T_INT) || (sp[-1].u.integer <= 0))
    {
      free(q);
      Pike_error("Bad fileobject to request_object->reply()\n");
    }
    if((q->from_fd = fd_dup(sp[-1].u.integer)) == -1)
      Pike_error("Bad file object to request_object->reply()\n");
    pop_stack();

    q->len = sp[-1].u.integer;
  } else {
    q->from_fd = 0;
    q->len = 0;
  }

  if(reply_string)
  {
    q->data = sp[-args].u.string;
    q->data->refs++;
  } else {
    q->data = 0;
  }
  q->sent = 0;

  th_farm( (void (*)(void *))actually_send, (void *)q );

  pop_n_elems(args);
  push_int(0);
}

void f_aap_reply_with_cache(INT32 args)
{
  struct cache_entry *ce;
  struct pike_string *reply;
  INT_TYPE time_to_keep, freed=0;
  time_t t;

  if(!THIS->request)
    Pike_error("Reply already called.\n");

  get_all_args(NULL, args, "%n%i", &reply, &time_to_keep);

  if((size_t)reply->len < (size_t)THIS->request->cache->max_size/2)
  {
    struct cache *rc = THIS->request->cache;
    struct args *tr = THIS->request;
    if(rc->gone) /* freed..*/
    {
      free_args( tr );
      THIS->request = 0;
      return;
    }
    THREADS_ALLOW();
    t = aap_get_time();
    mt_lock(&rc->mutex);
    if(rc->size > rc->max_size)
    {
      struct cache_entry *p,*pp=0,*ppp=0;
      ptrdiff_t target = rc->max_size - rc->max_size/3;
      while((size_t)rc->size > (size_t)target)
      {
	int i;
	freed=0;
	for(i = 0; i<CACHE_HTABLE_SIZE; i++)
	{
	  p = rc->htable[i];
	  ppp=pp=0;
	  while(p)
	  {
	    ppp = pp;
	    pp = p;
	    p = p->next;
	  }
	  if(pp) aap_free_cache_entry(rc,pp,ppp,i);
	  freed++;
	  if((size_t)rc->size < (size_t)target)
	    break;
	}
	if(!freed)  /* no way.. */
	  break;
      }
    }
    ce = new_cache_entry();
    memset(ce, 0, sizeof(struct cache_entry));
    ce->stale_at = t+time_to_keep;

    ce->data = reply;
    reply->refs++;

    ce->url = tr->res.url;
    ce->url_len = tr->res.url_len;

    ce->host = tr->res.host;
    ce->host_len = tr->res.host_len;
    aap_cache_insert(ce, rc);
    mt_unlock(&rc->mutex);
    THREADS_DISALLOW();
  }
  pop_stack();
  f_aap_reply(1);
}

void f_low_aap_reqo__init(struct c_request_object *o)
{
  if(o->request->res.protocol)
    SINSERT(o->misc_variables, s_prot, o->request->res.protocol);
  IINSERT(o->misc_variables, s_time, aap_get_time());
  TINSERT(o->misc_variables, s_rawurl,
	  o->request->res.url, o->request->res.url_len);
}

void aap_exit_request_object(struct object *UNUSED(o))
{
  if(THIS->request)
    free_args( THIS->request );
  if(THIS->misc_variables)
    free_mapping(THIS->misc_variables);
  if(THIS->done_headers)
    free_mapping(THIS->done_headers);
}
#endif
