/* Hohum. Here we go. This is try number four for a more optimized
 *  Roxen.
 */

#include "config.h"
/* #define AAP_DEBUG */

#define PARSE_FAILED ("HTTP/1.0 500 Internal Server Error\r\nContent-Type: text/html\r\n\r\nRequest parsing failed.\r\n")

#include "global.h"
	  
#include "array.h"
#include "bignum.h"
#include "backend.h"
#include "machine.h"
#include "mapping.h"
#include "module_support.h"
#include "multiset.h"
#include "object.h"
#include "operators.h"
#include "pike_memory.h"
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

#include "accept_and_parse.h"
#include "log.h"
#include "cache.h"
#include "requestobject.h"
#include "util.h"
#include "timeout.h"

/* This must be included last! */
#include "module_magic.h"

static struct callback *my_callback;
struct program *request_program;
struct program *c_request_program;
struct program *accept_loop_program;


/* Define external global string variables ... */
#define STRING(X,Y) extern struct pike_string *X
#include "static_strings.h"
#undef STRING
/* there.. */

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)<(b)?(b):(a))
#endif

#define MAXLEN (1024*1024*10)
static MUTEX_T queue_mutex;
static struct args *request, *last;

static MUTEX_T arg_lock;
static int next_free_arg, num_args;
static struct args *free_arg_list[100];

int aap_total_bytes;
void *debug_aap_malloc(int nbytes)
{
  int *data;
  aap_total_bytes += nbytes;
  data = malloc( nbytes+16 );
  data[0] = nbytes;
  return ((char *)data)+16; /* keep it aligned.. */
}

void debug_aap_free( void *what )
{
  int *data = (void *)(((char *)what-16));
  aap_total_bytes -= data[0];
  fprintf( stderr, "%d Kbytes\n", aap_total_bytes/1024 );
  free( data );
}


void free_args( struct args *arg )
{
  num_args--;

  if( arg->res.data ) aap_free( arg->res.data );
  if( arg->fd )       fd_close( arg->fd );

  mt_lock( &arg_lock );
  if( next_free_arg < 100 )
    free_arg_list[ next_free_arg++ ] = arg;
  else
    aap_free(arg);
  mt_unlock( &arg_lock );
}

struct args *new_args( )
{
  struct args *res;
  mt_lock( &arg_lock );
  num_args++;
  if( next_free_arg )
    res = free_arg_list[--next_free_arg];
  else
    res = aap_malloc( sizeof( struct args ) );
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
	  LOG(len, arg, atoi(ce->data->str+MY_MIN(ce->data->len, 9))); 
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
    buffer_len = MAX(arg->res.data_len,8192);
    arg->res.data=0;
  }
  else 
    p = buffer = aap_malloc(8192);

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
    if((tmp = my_memmem("\r\n\r\n", 4, MAX(p-3, buffer), 
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
	mt_lock_interpreter();
	for(i=0; i<CACHE_HTABLE_SIZE; i++)
	{
	  e = arg->cache->htable[i];
	  while(e)
	  {
	    t = e;
	    e = e->next;
	    t->next = 0;
	    free_string(t->data);
	    aap_free(t->url);
	    aap_free(t);
	  }
	}
	while(arg->log->log_head)
	{
	  struct log_entry *l = arg->log->log_head->next;
	  aap_free(arg->log->log_head);
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
	  aap_free(c);
	}


	l = aap_first_log;
	while(l && l != arg->log) {n=l;l = l->next;}
	if(l)
	{
	  if(n)    n->next = l->next;
	  else     aap_first_log = l->next;
	  aap_free(l);
	}
	mt_unlock_interpreter();
	aap_free(arg2);
	aap_free(arg);
	return; /* No more accept here.. */
      }
    }
  }
}


static void finished_p(struct callback *foo, void *b, void *c)
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

/*     { */
/*       JMP_BUF recovery; */

/*       free_svalue(& throw_value); */
/*       throw_value.type=T_INT; */

/*       if(SETJMP(recovery)) */
/*       { */
/*       } */
/*       else */
/*       { */
        apply_svalue(&arg->cb, 2);
/*       } */
/*     } */
    pop_stack();
  }
}

static void f_accept_with_http_parse(INT32 nargs)
{
/* From socket.c */
  struct port 
  {
    int fd;
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
  get_all_args("accept_http_loop", nargs, "%o%*%*%*%d%d%d", &port, &program,
	       &fun, &cb, &ms, &dolog, &to);
  MEMSET(args, 0, sizeof(struct args));
  if(dolog)
  {
    struct log *log = aap_malloc(sizeof(struct log));
    MEMSET(log, 0, sizeof(struct log));
    mt_init(&log->log_lock);
    args->log = log;
    log->next = aap_first_log;
    aap_first_log = log;
  }
  c = aap_malloc(sizeof(struct cache));
  MEMSET(c, 0, sizeof(struct cache));
  mt_init(&c->mutex);
  c->next = first_cache;
  first_cache = c;
  args->cache = c;
  c->max_size = ms;
  args->fd = ((struct port *)port->storage)->fd;
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

static void f_cache_status(INT32 args)
{
  struct cache *c = LTHIS->cache;
  pop_n_elems(args);
  push_constant_text("hits"); 
  push_int64(c->hits);
  push_constant_text("misses"); 
  push_int64(c->misses);
  push_constant_text("stale"); 
  push_int64(c->stale);
  push_constant_text("size"); 
  push_int64(c->size);
  push_constant_text("entries"); 
  push_int64(c->entries);
  push_constant_text("max_size"); 
  push_int64(c->max_size);

  /* Relative from last call */
  push_constant_text("sent_bytes"); 
  push_int(c->sent_data);        c->sent_data=0;
  push_constant_text("num_request"); 
  push_int(c->num_requests);    c->num_requests=0;
  push_constant_text("received_bytes"); 
  push_int(c->received_data);c->received_data=0;
  f_aggregate_mapping( 18 );
}

#endif /* _REENTRANT */

void f_aap_add_filesystem( INT32 args )
{
  INT_TYPE nosyms = 0;
  struct pike_string *basedir, *mountpoint;
  struct array *noparse;

  if(args == 4)
    get_all_args( "add_filesystem", args, 
                  "%s%s%a%d", &basedir, &mountpoint, &noparse, &nosyms );
  else
    get_all_args( "add_filesystem", args, 
                  "%s%s%a", &basedir, &mountpoint, &noparse );
}




#ifndef OFFSETOF
#define OFFSETOF(str_type, field) ((long)& (((struct str_type *)0)->field))
#endif

void pike_module_init()
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
#ifdef ADD_STORAGE
  ADD_STORAGE(struct args);
#else
  add_storage(sizeof(struct args));
#endif
  add_function("create", f_accept_with_http_parse, 
	       "function(object,program,function,mixed,int,int,int:void)", 0);
  add_function("cache_status", f_cache_status,"function(void:mapping)", 0);
  add_function("log_as_array",f_aap_log_as_array,"function(void:array(object))",0);
  add_function("log_as_commonlog_to_file", f_aap_log_as_commonlog_to_file, 
	       "function(object:int)", 0);

  add_function("log_size", f_aap_log_size, "function(void:int)", 0);
  add_function("logp", f_aap_log_exists, "function(void:int)", 0);

#if 0
  add_function( "add_filesystem", f_aap_add_filesystem, 
                "function(string,string,array(string),int|void:void)", 0);
  add_function( "add_contenttype", f_aap_add_contenttype, 
                "function(strign,string:void)", 0);
#ifdef FS_STATS
  add_function( "filesystem_stats", f_filesystem_stats,
                "function(void:mapping)", 0);
#endif
#endif

  add_program_constant("Loop", accept_loop_program = end_program(), 0);

  start_new_program();
#define OFFSET(X) (offset + OFFSETOF(log_object,X))
#ifdef ADD_STORAGE
  offset=ADD_STORAGE(struct log_object);
#else
  offset=add_storage(sizeof(struct log_object));
#endif
  map_variable("time", "int", 0, OFFSET(time), T_INT);
  map_variable("sent_bytes", "int", 0, OFFSET(sent_bytes), T_INT);
  map_variable("reply", "int", 0, OFFSET(reply), T_INT);
  map_variable("received_bytes", "int", 0, OFFSET(received_bytes), T_INT);
  map_variable("raw", "string", 0, OFFSET(raw), T_STRING);
  map_variable("url", "string", 0, OFFSET(url), T_STRING);
  map_variable("method", "string", 0, OFFSET(method), T_STRING);
  map_variable("protocol", "string", 0, OFFSET(protocol), T_STRING);
  map_variable("from", "string", 0, OFFSET(from), T_STRING);
  add_program_constant("logentry", (aap_log_object_program=end_program()), 0);


  start_new_program();
#ifdef ADD_STORAGE
  ADD_STORAGE(struct c_request_object);
#else
  add_storage(sizeof(struct c_request_object));
#endif

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
#endif /* _REENTRANT */
}

void pike_module_exit() 
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
#ifdef HAVE_TIMEOUTS
  mt_lock( &aap_timeout_mutex );
#endif
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
	aap_free(log_head);
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
	aap_free(t->url);
	aap_free(t);
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

