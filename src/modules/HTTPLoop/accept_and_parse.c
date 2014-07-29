/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/* Hohum. Here we go. This is try number four for a more optimized
 *  Roxen.
 */

#include "config.h"
/* #define AAP_DEBUG */

#define PARSE_FAILED ("HTTP/1.0 500 Internal Server Error\r\nContent-Type: text/html\r\n\r\nRequest parsing failed.\r\n")

#include "global.h"
/*! @module HTTPLoop 
 *!
 *! High performance webserver optimized for somewhat static content.
 *!
 *! HTTPLoop is a less capable WWW-server than the
 *! Protocols.HTTP.Server server, but for some applications it can be
 *! preferable. It is significantly more optimized, for most uses, and
 *! can handle a very high number of requests per second on even
 *! low end machines.
 */

/*! @class Loop
 */

#include "bignum.h"
#include "backend.h"
#include "machine.h"
#include "mapping.h"
#include "module_support.h"
#include "object.h"
#include "operators.h"
#include "pike_macros.h"
#include "fdlib.h"
#include "program.h"
#include "stralloc.h"
#include "svalue.h"
#include "threads.h"
#include "builtin_functions.h"

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

#include "pike_netlib.h"
#include "accept_and_parse.h"
#include "log.h"
#include "cache.h"
#include "requestobject.h"
#include "util.h"
#include "timeout.h"

#endif /* _REENTRANT */


#ifdef _REENTRANT

#define sp Pike_sp

static struct callback *my_callback;
struct program *request_program;
struct program *c_request_program;
struct program *accept_loop_program;


/* Define external global string variables ... */
#define STRING(X,Y) extern struct pike_string *X
#include "static_strings.h"
#undef STRING
/* there.. */

#define MAXLEN (1024*1024*10)
static MUTEX_T queue_mutex;
static struct args *request, *last;

static MUTEX_T arg_lock;
static int next_free_arg, num_args;
static struct args *free_arg_list[100];

void free_args( struct args *arg )
{
  num_args--;

  free( arg->res.data );
  if( arg->fd )       fd_close( arg->fd );

  mt_lock( &arg_lock );
  if( next_free_arg < 100 )
    free_arg_list[ next_free_arg++ ] = arg;
  else
    free(arg);
  mt_unlock( &arg_lock );
}

struct args *new_args(void)
{
  struct args *res;
  mt_lock( &arg_lock );
  num_args++;
  if( next_free_arg )
    res = free_arg_list[--next_free_arg];
  else
    res = malloc( sizeof( struct args ) );
  mt_unlock( &arg_lock );
  return res;
}


/* This should probably be improved to include the reason for the 
 * failure. Currently, all failed requests get the same (hardcoded) 
 * error message.
 * 
 * It is virtually impossible to call a pike function from here, so that
 * is not an option.
 */
static void failed(struct args *arg)
{
  WRITE(arg->fd, PARSE_FAILED, strlen(PARSE_FAILED));
#ifdef AAP_DEBUG
  fprintf(stderr, "AAP: Failed\n");
#endif /* AAP_DEBUG */
  free_args( arg ); 
}


/* Parse a request. This function is somewhat obscure for performance
 * reasons.
 */
static int parse(struct args *arg)
{
  int s1=0, s2=0, i;
  struct cache_entry *ce ;
  /* get: URL, Protocol, method, headers*/
  for(i=0;i<arg->res.data_len;i++)
    if(arg->res.data[i] == ' ') {
      if(!s1) 
	s1=i; 
      else
	s2=i;
    } else if(arg->res.data[i]=='\r')
      break;

  if(!s1)
  {
    failed( arg );
    return 0;
  }

  /* GET http://www.roxen.com/foo.html HTTP/1.1\r
   * 0  ^s1                           ^s2      ^i
   */

  if(!s2) 
    arg->res.protocol = s_http_09;
  else if(!memcmp("HTTP/1.", arg->res.data+s2+1, 7))
  {
    if(arg->res.data[s2+8]=='0')
      arg->res.protocol = s_http_10;
    else if(arg->res.data[s2+8]=='1')
      arg->res.protocol = s_http_11;
  }
  else
    arg->res.protocol = 0;

  arg->res.method_len = s1;

  if(arg->res.protocol != s_http_09)
    arg->res.header_start = i+2;
  else
    arg->res.header_start = 0;


  /* find content */
  arg->res.content_len=0;
  aap_get_header(arg, "content-length", H_INT, &arg->res.content_len);
  if((arg->res.data_len - arg->res.body_start) < arg->res.content_len)
  {
    ptrdiff_t nr;
    /* read the rest of the request.. OBS: This is done without any
     * timeout right now. It is relatively easy to add one, though.
     * The only problem is that this might be an upload of a _large_ file,
     * if so the upload could take an hour or more. So there can be no 
     * sensible default timeout. The only option is to reshedule the timeout
     * for each and every read.
     *
     * This code should probably not allocate infinite amounts of 
     * memory either...
     * 
     * TODO: rewrite this to use a mmaped file if the size is bigger than
     * 1Mb or so.
     * 
     * This could cause trouble with the leftovers code below, so that 
     * would have to be changed as well.
     */
    arg->res.data=realloc(arg->res.data,
			  arg->res.body_start+arg->res.content_len);
    while( arg->res.data_len < arg->res.body_start+arg->res.content_len)
    {
      while(((nr = fd_read(arg->fd, arg->res.data+arg->res.data_len, 
			   (arg->res.body_start+arg->res.content_len)-
			   arg->res.data_len)) < 0) && errno == EINTR);
      if(nr <= 0)
      {
	failed(arg);
	return 0;
      }
      arg->res.data_len += nr;
    }
  }
  /* ok.. now we should find the leftovers... This is any extra
   * data not belonging to this request that has already been read.
   * Since it is rather hard to unread things in a portable way, we simply
   * store them in the result structure for the next request.
   */

  arg->res.leftovers_len=
    (arg->res.data_len-arg->res.body_start-arg->res.content_len);
  if(arg->res.leftovers_len)
    arg->res.leftovers=
      (arg->res.data+arg->res.body_start+arg->res.content_len);

  arg->res.url = arg->res.data+s1+1;
  arg->res.url_len = (s2?s2:i)-s1-1;
  
  {
    struct pstring h;
    h.len=0;
    h.str="";
    if(aap_get_header(arg, "host", H_STRING, &h))
    {
      arg->res.host = h.str;
      arg->res.host_len = h.len;
    } else {
      arg->res.host = arg->res.data;
      arg->res.host_len = 0;
    }

    if(arg->cache->max_size && arg->res.data[0]== 'G') /* GET, presumably */
    {
      if(!aap_get_header(arg, "pragma", H_EXISTS, 0))
	if((ce = aap_cache_lookup(arg->res.url, arg->res.url_len, 
			      arg->res.host, arg->res.host_len,
			      arg->cache,0, NULL, NULL)) && ce->data)
	{
	  ptrdiff_t len = WRITE(arg->fd, ce->data->str, ce->data->len);
	  LOG(len, arg, atoi(ce->data->str+MINIMUM(ce->data->len, 9)));
	  simple_aap_free_cache_entry( arg->cache, ce );
	  /* if keepalive... */
	  if((arg->res.protocol==s_http_11)
	     ||aap_get_header(arg, "connection", H_EXISTS, 0))
	  {
	    return -1; 
	  }
#ifdef AAP_DEBUG
	  fprintf(stderr, "Closing connection...\n");
#endif /* AAP_DEBUG */
          free_args( arg );
	  return 0;
	}
    }
  }
  return 1;
}

void aap_handle_connection(struct args *arg)
{
  char *buffer, *p, *tmp;
  ptrdiff_t pos, buffer_len;
#ifdef HAVE_TIMEOUTS
  int *timeout = NULL;
#endif
 start:
  pos=0;
  buffer_len=8192;

  if(arg->res.data && arg->res.data_len > 0)
  {
    p = buffer = arg->res.data;
    buffer_len = MAXIMUM(arg->res.data_len,8192);
    arg->res.data=0;
  }
  else 
    p = buffer = malloc(8192);

  if(arg->res.leftovers && arg->res.leftovers_len)
  {
    if(!buffer)
    {
      perror("AAP: Failed to allocate buffer (leftovers)");
      failed(arg);
      return;
    }
    buffer_len = arg->res.leftovers_len;
    MEMCPY(buffer, arg->res.leftovers, arg->res.leftovers_len);
    pos = arg->res.leftovers_len;
    arg->res.leftovers=0;
    if((tmp = my_memmem("\r\n\r\n", 4, buffer, pos)))
      goto ok;
    p += arg->res.leftovers_len;
  }
  
  if(!buffer)
  {
    perror("AAP: Failed to allocate buffer");
    failed(arg);
    return;
  }
#ifdef HAVE_TIMEOUTS
  if( arg->timeout )
    timeout = aap_add_timeout_thr(th_self(), arg->timeout);
  while( !timeout  || !(*timeout) )
#else
  while(1)
#endif /* HAVE_TIMEOUTS */
  {
    ptrdiff_t data_read = fd_read(arg->fd, p, buffer_len-pos);
    if(data_read <= 0)
    {
#ifdef AAP_DEBUG
      fprintf(stderr, "AAP: Read error/eof.\n");
#endif /* AAP_DEBUG */
      arg->res.data = buffer;
      free_args( arg );
#ifdef HAVE_TIMEOUTS
      if( timeout )
      {
        aap_remove_timeout_thr( timeout );
        timeout=NULL;
      }
#endif
      return;
    }
    pos += data_read;
    if((tmp = my_memmem("\r\n\r\n", 4, MAXIMUM(p-3, buffer),
			data_read+(p-3>buffer?3:0))))
      goto ok;
    p += data_read;
    if(pos >= buffer_len)
    {
      buffer_len *= 2;
      if(buffer_len > MAXLEN)
	break;

      buffer = realloc(buffer, buffer_len);
      p = buffer+pos;
      if(!buffer) 
      {
	perror("AAP: Failed to allocate memory (reading)");
	break;
      }
    }
  }

  arg->res.data = buffer;
  failed( arg );
#ifdef HAVE_TIMEOUTS
  if( timeout )
  {
    aap_remove_timeout_thr( timeout );
    timeout=NULL;
  }
#endif
  return;

 ok:
#ifdef HAVE_TIMEOUTS
  if (timeout)
  {
    aap_remove_timeout_thr( timeout );
    timeout=NULL;
  }
#endif /* HAVE_TIMEOUTS */
  arg->res.body_start = (tmp+4)-buffer;
  arg->res.data = buffer;
  arg->res.data_len = pos;
  switch(parse(arg))
  {
   case 1:
    mt_lock(&queue_mutex);
    if(!request)
    {
      request = last = arg;
      arg->next = 0;
    }
    else
    {
      last->next = arg;
      last = arg;
      arg->next = 0;
    }
    mt_unlock(&queue_mutex);
    wake_up_backend();
    return;

   case -1:
     goto start;

   case 0:
    ;
  }
}

#ifndef HAVE_AND_USE_POLL
#undef HAVE_POLL
#endif /* !HAVE_AND_USE_POLL */

#ifdef HAVE_POLL
#ifdef HAVE_POLL_H
#include <poll.h>
#else /* !HAVE_POLL_H */
#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#else  /* !HAVE_SYS_POLL_H */
/* No <poll.h> or <sys/poll.h> */
#undef HAVE_POLL
#endif /* HAVE_SYS_POLL_H */
#endif /* HAVE_POLL_H */

#ifndef POLLRDBAND
#define POLLRDBAND	POLLPRI
#endif /* !POLLRDBAND */

#endif /* HAVE_POLL */

static void low_accept_loop(struct args *arg)
{
  struct args *arg2 = new_args();
  ACCEPT_SIZE_T len = sizeof(arg->from);
  while(1)
  {
    MEMCPY(arg2, arg, sizeof(struct args));
    arg2->fd = fd_accept(arg->fd, (struct sockaddr *)&arg2->from, &len);
    if(arg2->fd != -1)
    {
      th_farm((void (*)(void *))aap_handle_connection, arg2);
      arg2 = new_args();
      arg2->res.leftovers = 0;
    } else {
      if(errno == EBADF)
      {
	int i;
	struct cache_entry *e, *t;
	struct cache *c, *p = NULL;
	struct log *l, *n = NULL;
	/* oups. */
	low_mt_lock_interpreter(); /* Can run even if threads_disabled. */
	for(i=0; i<CACHE_HTABLE_SIZE; i++)
	{
	  e = arg->cache->htable[i];
	  while(e)
	  {
	    t = e;
	    e = e->next;
	    t->next = 0;
	    free_string(t->data);
	    free(t->url);
	    free(t);
	  }
	}
	while(arg->log->log_head)
	{
	  struct log_entry *l = arg->log->log_head->next;
	  free(arg->log->log_head);
	  arg->log->log_head = l;
	}

	c = first_cache;
	while(c && c != arg->cache) {p=c;c = c->next;}
	if(c)
	{
	  if(p) 
	    p->next = c->next;
	  else
	    first_cache = c->next;
	  c->gone = 1;
	  free(c);
	}


	l = aap_first_log;
	while(l && l != arg->log) {n=l;l = l->next;}
	if(l)
	{
	  if(n)    n->next = l->next;
	  else     aap_first_log = l->next;
	  free(l);
	}
	mt_unlock_interpreter();
	free(arg2);
	free(arg);
	return; /* No more accept here.. */
      }
    }
  }
}


static void finished_p(struct callback *UNUSED(foo), void *UNUSED(b), void *UNUSED(c))
{
  extern void f_low_aap_reqo__init( struct c_request_object * );

  aap_clean_cache();

  while(request)
  {
    struct args *arg;
    struct object *o;
    struct c_request_object *obj;

    mt_lock(&queue_mutex);
    arg = request;
    request = arg->next;
    mt_unlock(&queue_mutex);

    o = clone_object( request_program, 0 ); /* see requestobject.c */
    obj = (struct c_request_object *)get_storage(o, c_request_program );
    MEMSET(obj, 0, sizeof(struct c_request_object));
    obj->request = arg;
    obj->done_headers   = allocate_mapping( 20 );
    obj->misc_variables = allocate_mapping( 40 );

    f_low_aap_reqo__init( obj );

    push_object( o );
    assign_svalue_no_free(sp++, &arg->args);

    apply_svalue(&arg->cb, 2);
    pop_stack();
  }
}

/*! @decl void create( Stdio.Port port, RequestProgram program, @
 *!                    function(RequestProgram:void) request_handler, @
 *!                    int cache_size, @
 *!                    bool keep_log, int timeout )
 *!
 *! Create a new HTTPLoop.
 *!
 *! This will start a new thread that will listen for requests on the
 *! port, parse them and pass on requests, instanced from the
 *! @[program] class (which has to inherit @[RequestProgram] to the
 *! @[request_handler] callback function.
 *!
 *! @[cache_size] is the maximum size of the cache, in bytes.
 *! @[keep_log] indicates if a log of all requests should be kept.
 *! @[timeout] if non-zero indicates a maximum time the server will wait for requests.
 *!
*/
static void f_accept_with_http_parse(INT32 nargs)
{
/* From socket.c */
  struct port 
  {
    struct fd_callback_box box;
    int my_errno;
    struct svalue accept_callback;
    struct svalue id;
  };
  INT_TYPE ms, dolog;
  INT_TYPE to;
  struct object *port;
  struct svalue *fun, *cb, *program;
  struct cache *c;
  struct args *args = LTHIS; 
  get_all_args("accept_http_loop", nargs, "%o%*%*%*%i%i%i", &port, &program,
	       &fun, &cb, &ms, &dolog, &to);
  MEMSET(args, 0, sizeof(struct args));
  if(dolog)
  {
    struct log *log = calloc(1, sizeof(struct log));
    mt_init(&log->log_lock);
    args->log = log;
    log->next = aap_first_log;
    aap_first_log = log;
  }
  c = calloc(1, sizeof(struct cache));
  mt_init(&c->mutex);
  c->next = first_cache;
  first_cache = c;
  args->cache = c;
  c->max_size = ms;
  {
      extern struct program *port_program;
      args->fd = ((struct port *)get_storage( port, port_program))->box.fd;
  }
  args->filesystem = NULL;
#ifdef HAVE_TIMEOUTS
  args->timeout = to;
#endif
  assign_svalue_no_free(&args->cb, fun);   /* FIXME: cb isn't freed on exit */
  assign_svalue_no_free(&args->args, cb);

  request_program = program_from_svalue( program );
  if(!request_program)
  {
    free_args(args);
    Pike_error("Invalid request program\n");
  }
  if(!my_callback)
    my_callback = add_backend_callback( finished_p, 0, 0 );

  {
    int i;
    for( i = 0; i<8; i++ )
      th_farm((void (*)(void *))low_accept_loop, (void *)args);
  }
}

/*! @decl array(LogEntry) log_as_array()
 *!
 *! Return the current log as an array of LogEntry objects.
 */

/*! @decl int log_as_commonlog_to_file(Stdio.Stream fd)
 *!
 *! Write the current log to the specified file in a somewhat common
 *! commonlog format.
 *!
 *! Will return the number of bytes written.
 */

/*! @decl bool logp()
 *! Returns true if logging is enabled
 */

/*! @decl int log_size()
 *!
 *! Returns the number of entries waiting to be logged.
 */

/*! @decl mapping(string:int) cache_status()
 *!
 *! Returns information about the cache.
 *! @mapping result
 *! @member int hits
 *!   The number of hits since start
 *! @member int misses
 *!   The number of misses since start
 *! @member int stale
 *!   The number of misses that were stale hits, and not used
 *! @member int size
 *!   The total current size
 *! @member int entries
 *!   The number of entries in the cache
 *! @member int max_size
 *!   The maximum size of the cache
 *!
 *! @member int sent_bytes
 *!  The number of bytes sent since the last call to cache_status
 *!
 *! @member int received_bytes
 *!  The number of bytes received since the last call to cache_status
 *!
 *! @member int num_requests
 *!  The number of requests received since the last call to cache_status
 *! @endmapping
 */
static void f_cache_status(INT32 args)
{
  struct cache *c = LTHIS->cache;
  pop_n_elems(args);
  push_text("hits"); 
  push_int64(c->hits);
  push_text("misses"); 
  push_int64(c->misses);
  push_text("stale"); 
  push_int64(c->stale);
  push_text("size"); 
  push_int64(c->size);
  push_text("entries"); 
  push_int64(c->entries);
  push_text("max_size"); 
  push_int64(c->max_size);

  /* Relative from last call */
  push_text("sent_bytes"); 
  push_int(c->sent_data);        c->sent_data=0;
  push_text("num_request"); 
  push_int(c->num_requests);    c->num_requests=0;
  push_text("received_bytes"); 
  push_int(c->received_data);c->received_data=0;
  f_aggregate_mapping( 18 );
}
/*! @endclass
 */

/*! @class LogEntry
 *!
 *! @decl int time
 *!
 *! @decl int sent_bytes
 *!
 *! @decl int received_bytes
 *!
 *! @decl int reply
 *!
 *! @decl string raw
 *!
 *! @decl string url
 *!
 *! @decl string method
 *!
 *! @decl string protocol
 *!
 *! @decl string from
 *!
 *! @endclass
 */

/*! @class RequestProgram
 *!
 *! @decl string prot
 *!   The protocol part of the request. As an example "HTTP/1.1"
 *!
 *! @decl int time
 *!   The time_t when the request arrived to the server
 *!
 *! @decl string raw_url
 *!   The raw URL received, the part after the method and before the protocol.
 *!
 *! @decl string not_query
 *!   The part of the URL before the first '?'.
 *!
 *! @decl string query
 *!   The part of the URL after the first '?'
 *!
 *! @decl string rest_query
 *!   The part of the URL after the first '?' that does not seem to be query variables.
 *!
 *! @decl mapping(string:string) variables;
 *!   Parsed query variables
 *!
 *! @decl string client
 *!   The user agent
 *!
 *! @decl string referer
 *!   The referer header
 *!
 *! @decl string since
 *!   The get-if-not-modified, if set.
 *!
 *! @decl string data
 *!   Any payload that arrived with the request
 *!
 *! @decl mapping(string:array(string)) headers;
 *!   All received headers
 *!
 *! @decl multiset(string) pragma;
 *!   Tokenized pragma headers
 *!
 *! @decl Stdio.NonblockingStream my_fd
 *!   The filedescriptor for this request.
 *!
 *! @decl string remoteaddr
 *!   The remote address
 *!
 *! @decl string method
 *!   The method (GET, PUT etc)
 *!
 *! @decl string raw
 *!   The full request
 *!
 *! @decl void output(string data)
 *!  Send @[data] directly to the remote side.
 *!
 *! @decl void reply( string data )
 *! @decl void reply( string headers, Stdio.File fd, int len )
 *! @decl void reply( int(0..0) ignore, Stdio.File fd, int len )
 *!   Send a reply to the remote side.
 *!   In the first case the @[data] will be sent.
 *!   In the second case the @[headers] will be sent, then @[len] bytes from @[fd].
 *!   In the last case @[len] bytes from @[fd] will be sent.
 *!
 *! @decl void reply_with_cache( string data, int(1..) stay_time )
 *!  Send @[data] as the reply, and keep it as a cache entry to
 *!  requests to this URL for @[stay_time] seconds.
 *!
 *! @endclass
 */
#endif /* _REENTRANT */


PIKE_MODULE_INIT
{
#ifdef _REENTRANT
  ptrdiff_t offset;
#define STRING(X,Y) X=make_shared_string(Y)
#include "static_strings.h"
#undef STRING

  mt_init(&queue_mutex);
  mt_init(&arg_lock);

  aap_init_cache();

#ifdef HAVE_TIMEOUTS
  aap_init_timeouts();
#endif

  start_new_program();
  ADD_STORAGE(struct args);
  add_function("create", f_accept_with_http_parse, 
	       "function(object,program,function,mixed,int,int,int:void)", 0);
  add_function("cache_status", f_cache_status,"function(void:mapping)", 0);
  add_function("log_as_array",f_aap_log_as_array,"function(void:array(object))",0);
  add_function("log_as_commonlog_to_file", f_aap_log_as_commonlog_to_file, 
	       "function(object:int)", 0);

  add_function("log_size", f_aap_log_size, "function(void:int)", 0);
  add_function("logp", f_aap_log_exists, "function(void:int)", 0);

  add_program_constant("Loop", accept_loop_program = end_program(), 0);

  start_new_program();
#define OFFSET(X) (offset + OFFSETOF(log_object,X))
  offset=ADD_STORAGE(struct log_object);
  map_variable("time", "int", 0, OFFSET(time), T_INT);
  map_variable("sent_bytes", "int", 0, OFFSET(sent_bytes), T_INT);
  map_variable("reply", "int", 0, OFFSET(reply), T_INT);
  map_variable("received_bytes", "int", 0, OFFSET(received_bytes), T_INT);
  map_variable("raw", "string", 0, OFFSET(raw), T_STRING);
  map_variable("url", "string", 0, OFFSET(url), T_STRING);
  map_variable("method", "string", 0, OFFSET(method), T_STRING);
  map_variable("protocol", "string", 0, OFFSET(protocol), T_STRING);
  map_variable("from", "string", 0, OFFSET(from), T_STRING);
  add_program_constant("LogEntry", (aap_log_object_program=end_program()), 0);


  start_new_program();
  ADD_STORAGE(struct c_request_object);
  add_function("`[]", f_aap_index_op, "function(string:mixed)",0);
  add_function("`->", f_aap_index_op, "function(string:mixed)",0);

/* add_function("`->=", f_index_equal_op, "function(string,mixed:mixed)",0); */
/* add_function("`[]=", f_index_equal_op, "function(string,mixed:mixed)",0); */

  add_function("scan_for_query", f_aap_scan_for_query, 
	       "function(string:string)", OPT_TRY_OPTIMIZE);

  add_function("end", f_aap_end, "function(string|void,int|void:void)", 0);
  add_function("send", f_aap_output, "function(string:void)", 0);
  add_function("reply", f_aap_reply,
	       "function(string|void,object|void,int|void:void)", 0);
  add_function("reply_with_cache", f_aap_reply_with_cache, 
	       "function(string,int:void)", 0);
  set_init_callback( aap_init_request_object );
  set_exit_callback( aap_exit_request_object );
  add_program_constant("prog", (c_request_program = end_program()), 0);
  add_program_constant("RequestProgram", c_request_program, 0 );
#else
  HIDE_MODULE();
#endif /* _REENTRANT */
}

PIKE_MODULE_EXIT
{
#ifdef _REENTRANT
  struct log *log = aap_first_log;
  /* This is very dangerous, since the 
   * accept threads are still going strong.
   */

  /* Tell the handle_timeouts thread to go and die.
   * It will be dead in ~1s.
   * We wait at the end of this function to ensure that this time
   * has passed.
   */
#ifdef HAVE_TIMEOUTS
  aap_exit_timeouts();
#endif

  /* Lock all the global mutexes. This will freeze the accept threads
   * sooner or later. At least no more requests will be done to the
   * pike level.
   */
  mt_lock( &queue_mutex );
  /* Now, in theory, if all threads are stopped, we can free all the data.
   * BUT: There is no way to know _when_ all threads are stopped, they may
   * be in read() or something similar. Also, the locking of aap_timeout_mutex
   * above disables the timeout, so they might linger in read() indefinately.
   * Thus, we cannot free _all_ structures, only the ones that are always
   * protected by mutexes. Most notably, we must leave the main 'log' and 
   * 'cache' structures alive. We can, on the other hand, free all data _in_
   * the cache and log structures.
   */

  while(log)
  {
    mt_lock( &log->log_lock ); /* This lock is _not_ unlocked again.. */
    {
      struct log_entry *log_head = log->log_head;
      struct log *next = log->next;
      while(log_head)
      {
	struct log_entry *l = log_head->next;
	free(log_head);
	log_head = l;
      }
      log->next = NULL;
      log->log_head = log->log_tail = NULL;
      log = next;
    }
  }

  aap_clean_cache();
  while(first_cache)
  {
    int i;
    struct cache_entry *e, *t;
    struct cache *next;
    mt_lock(&first_cache->mutex);  /* This lock is _not_ unlocked again.. */
    next = first_cache->next;
    for(i=0; i<CACHE_HTABLE_SIZE; i++)
    {
      e = first_cache->htable[i];
      while(e)
      {
	t = e;
	e = e->next;
	t->next = 0;
        free_string(t->data);
	free(t->url);
	free(t);
      }
      first_cache->htable[i]=0;
    }
    first_cache->next = NULL;
    first_cache = next;
  }

  /* This will free all the string constants. It might be dangerous, 
   * but should not be so. No thread should enter the pike level code 
   * again, since all mutex locks are now locked.
   */
#define STRING(X,Y) free_string(X)
#include "static_strings.h"
#undef STRING

  if (my_callback) {
    remove_callback(my_callback);
  }

  free_program(c_request_program);
  free_program(aap_log_object_program);
  free_program(accept_loop_program);
#endif /* _REENTRANT */
}
/*! @endmodule */
