/*
 * $Id: requestobject.c,v 1.2 1999/11/23 07:07:28 hubbe Exp $
 */

#include "global.h"
#include "config.h"
	  
#include "array.h"
#include "backend.h"
#include "machine.h"
#include "mapping.h"
#include "module_support.h"
#include "multiset.h"
#include "object.h"
#include "operators.h"
#include "pike_memory.h"
#include "program.h"
#include "constants.h"
#include "stralloc.h"
#include "svalue.h"
#include "threads.h"
#include "fdlib.h"

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
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#ifdef _REENTRANT
#include "accept_and_parse.h"
#include "log.h"
#include "util.h"
#include "cache.h"
#include "requestobject.h"

extern void f_aggregate(INT32 args); /* no prototype? */

/* Used when fatal() can't be. */
#define DWERROR(X)	write(2, X, sizeof(X) - sizeof(""))

#if defined(CAN_HAVE_LINUX_SYSCALL4) || \
    !defined(DYNAMIC_MODULE) && defined(CAN_HAVE_NONSHARED_LINUX_SYSCALL4)

#include <asm/unistd.h>
#ifndef __NR_sendfile
#define __NR_sendfile 187
#endif
_syscall4(ssize_t,sendfile,int,out,int,in,off_t*,off,size_t,size);

#elif defined(CAN_HAVE_SENDFILE)

ssize_t sendfile(int out, int in, off_t *off, size_t size)
{
  ssize_t retval;
  asm volatile(
               "pushl %%ebx\n\t"
               "movl %%edi,%%ebx\n\t"
               "int $0x80\n\t"
               "popl %%ebx"
               :"=a" (retval)
               :"0" (187),
               "D" (out), /* pseudo-ebx (put it in edi)*/
               "c" (in),
               "d" (off),
               "S" (size)
               );
  if ((unsigned long) retval > (unsigned long)-1000) 
  {
    errno = -retval;
    retval = -1;
  }
  return retval;
}
# define HAVE_SENDFILE
#endif


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
  push_int(VAL);				\
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

#define AINSERT(MAP,INDEX,VAL)\
do { 						\
  push_array(VAL);				\
  push_string(INDEX);                           \
  mapping_insert((MAP),sp-1,sp-2);		\
  sp -= 2;                                      \
} while(0)

#define OINSERT(MAP,INDEX,VAL)\
do { 						\
  push_object(VAL);				\
  push_string(INDEX);                           \
  mapping_insert((MAP),sp-1,sp-2);		\
  sp -= 2;                                      \
} while(0)

#define MSINSERT(MAP,INDEX,VAL)\
do { 						\
  push_multiset(val);				\
  push_string(INDEX);                           \
  mapping_insert((MAP),sp-1,sp-2);		\
  sp -= 2;                                      \
} while(0)

#define MAINSERT(MAP,INDEX,VAL)\
do { 						\
  push_mapping(VAL);				\
  push_string(INDEX);                           \
  mapping_insert((MAP),sp-1,sp-2);		\
  sp -= 2;                                      \
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
  struct pike_string *_s;
  char *s, *work_area;
  int len, i,j, begin=0, c;
  if(args)
  {
    get_all_args("HTTP C object->scan_for_query(string f)", args, "%S", &_s);
    s = (char *)_s->str;
    len = _s->len;
  }
  else
  {
    s = THIS->request->res.url;
    len = THIS->request->res.url_len;
  }

  /*  [http://host:port]/(pre,states)/[url[?query]] */
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

/* Now find prestates, if any */
  j--;
  if(j > 3 && work_area[1] == '(' && work_area[0]=='/')
  {
    int k, n=0, last=2;
    for(k = 2; k < j; k++)
    {
      switch(work_area[k])
      {
       case ',':
	 push_string(make_shared_binary_string(work_area+last,k-last));
	 n++;
	 last = k+1;
	 break;
       case ')':
	 push_string(make_shared_binary_string(work_area+last,k-last));
	 n++;
	 begin = k+1;
	 f_aggregate_multiset( n );
	 goto done2;
      }
    }
    pop_n_elems(n);
    f_aggregate_multiset( 0 );
  } else {
    f_aggregate_multiset( 0 );
  }
 done2:
  push_string(s_prestate);
  mapping_insert(THIS->misc_variables, sp-1, sp-2);
  sp--; pop_stack();

  TINSERT(THIS->misc_variables, s_not_query, work_area+begin, j-begin+1);
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


static void decode_x_url_mixed(char *in, int l, struct mapping *v, char *dec, 
			       char *rest_query, char **rp)
{
  int pos = 0, lamp = 0, leq=0, dl;

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
	 int i;
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
    int i;
    for(i=lamp-1;i<dl;i++) *((*rp)++)=dec[i];
  }
}

static void parse_query(void)
{
  struct svalue *q;
  struct mapping *v = allocate_mapping(10); /* variables */
  push_string(s_query);
  if(!(q = low_mapping_lookup(THIS->misc_variables, sp-1))) 
    f_aap_scan_for_query(0);
  q = low_mapping_lookup(THIS->misc_variables, sp-1);
  sp--;

  if(q->type == T_STRING) 
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

  if(THIS->request->res.content_len &&
     THIS->request->res.data[1]=='O')
  {
    struct pstring ct;
    int nope = 0;
    if(aap_get_header(THIS->request, "content-type", T_STRING, &ct))
    {
      if(ct.str[0]=='m') /* multipart is not wanted here... */
	nope=1;
    }
    if(!nope)
    {
      char *tmp = malloc(THIS->request->res.content_len);
      decode_x_url_mixed(THIS->request->res.data+
			 THIS->request->res.body_start,
			 THIS->request->res.content_len,v,tmp,0,0);
      free(tmp);
    }
  }
  push_mapping(v); push_string(s_variables);
  mapping_insert(THIS->misc_variables, sp-1, sp-2);
  sp--; pop_stack();
}

static void parse_headers(void)
{
  struct svalue *tmp;
  struct mapping *headers = THIS->done_headers;
  int os=0,i,j,l=THIS->request->res.body_start-THIS->request->res.header_start;
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
  if (!args) error("HTTP C object->`[]: too few args\n");
  pop_n_elems(args-1);
  if(!THIS->misc_variables) 
  {
    struct svalue s;
    object_index_no_free2(&s, fp->current_object, sp-1);
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
  
  if(!THIS->request) error("Reply called. No data available\n");
  get_all_args("`[]", args, "%S", &s);
  
  if(s == s_not_query || s==s_query || s==s_prestate)
  {
    f_aap_scan_for_query(0);
    f_aap_index_op(1);
    return;
  }

  if(s == s_my_fd)
  {
    struct object *file_make_object_from_fd(int fd, int mode, int guess);
    /* 0x3800 is from modules/files/file.h, 
     * FILE_READ|FILE_WRITE|FILE_SET_CLOSE_ON_EXEC
     */
    push_object(file_make_object_from_fd
		(dup(THIS->request->fd),0x3800,
		 SOCKET_CAPABILITIES|PIPE_CAPABILITIES));
    push_string(s_my_fd);
    mapping_insert(THIS->misc_variables, sp-1, sp-2);
    sp--;
    return; /* the wanted value is on the stack now */
  }

  if(s == s_remoteaddr)
  {
    push_text(inet_ntoa(THIS->request->from.sin_addr));
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

  /* FIXME: Not actually implemented */
/*   if(s == s_cookies || s == s_config) */
/*   { */
/*     if(!THIS->headers_parsed) parse_headers(); */

/*     f_aap_index_op(1); */
/*     return; */
/*   } */

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
      sp[-1].type = T_INT;
      sp[-1].u.integer = 0;
    }
    push_string(s_since);
    mapping_insert(THIS->misc_variables, sp-1, sp-2);
    sp--;

    f_aap_index_op(1);
    return;
  }

  if( s == s_supports )
  {
    struct svalue *tmp;
    pop_stack();
    push_constant_text("roxen");
    if((tmp = low_mapping_lookup(get_builtin_constants(), sp-1)) 
       && tmp->type == T_OBJECT)
    {
      pop_stack( );
      ref_push_object( tmp->u.object );
      push_constant_text( "find_supports" );
      f_index( 2 );
      ref_push_string(s_client);
      f_aap_index_op( 1 );
      push_constant_text("");
      f_multiply( 2 );
      apply_svalue( sp-2, 1 );
      push_string(s_supports);
      mapping_insert(THIS->misc_variables, sp-1, sp-2);
      sp--;
      stack_swap();
      pop_stack();

    } else {
      pop_stack();
      f_aggregate_multiset( 0 );
      push_string(s_supports);
      mapping_insert(THIS->misc_variables, sp-1, sp-2);
      sp--;
    }



      /* 
         if(contents = o->misc["accept-encoding"])
         {
         foreach((contents-" ")/",", string e) {
         if (lower_case(e) == "gzip") {
         o->supports["autogunzip"] = 1;
         }
         }
         }
      */

    return;
  }


/*   if(s == s_realauth || s == s_rawauth) */
/*   { */
/*     pop_stack(); */
/*     push_int(0); */
/*     return; */
/*   } */

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
    object_index_no_free2(&s, fp->current_object, sp-1);
    pop_stack();
    *sp=s;
    sp++;
  }
}

/* static void f_index_equal_op(INT32 args) */
/* { */
  
/* } */

void f_aap_end(INT32 args)
{
  /* end connection. */
}

void f_aap_output(INT32 args)
{
  if(sp[-1].type != T_STRING) error("Bad argument 1 to output\n");
  WRITE(THIS->request->fd, sp[-1].u.string->str, sp[-1].u.string->len);
}

#define BUFFER 8192
struct thread_args *done;

struct send_args
{
  struct args *to;
  int from_fd;
  char *data;
  int data_len;
  int len;
  int sent;
  char buffer[BUFFER];
};

/* WARNING! This function is running _without_ any stack etc. */
void actually_send(struct send_args *a)
{
  int fail, first=0;
  char foo[10];
  foo[9]=0; foo[6]=0;

#ifdef HAVE_FREEBSD_SENDFILE
  if (a->len)
  {
    struct iovec vec;
    struct sf_hdtr headers = { NULL, 0, NULL, 0 };
    off_t off = 0;
    off_t sent = 0;
    size_t len = a->len;

#ifdef AAP_DEBUG
    fprintf(stderr, "sendfile... \n");
#endif

    if (a->data) {
      /* Set up the iovec */
      vec.iov_base = a->data;
      vec.iov_len = a->data_len;
      headers.headers = &vec;
      headers.hdr_cnt = 1;
    }

    if (a->len < 0) {
      /* Negative: Send all */
      len = 0;
    }

    if ((off = tell(a->from_fd)) < 0) {
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
	/* NOTE: Can't use fatal(), since we're not in a valid Pike context. */
	DWERROR("FreeBSD-style sendfile() returned EFAULT.\n");
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

    if (a->data) {
      if (sent < a->data_len) {
	a->data += sent;
	a->data_len -= sent;
	sent = 0;

	/* Make sure we don't terminate early due to len being 0. */
	goto send_data;
      } else {
	sent -= a->data_len;
	a->data = NULL;
	a->data_len = 0;
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

#ifdef AAP_DEBUG
  fprintf(stderr, "actually_send... \n");
#endif
  if(a->data)
  {
    MEMCPY(foo, a->data+MY_MIN((a->data_len-4),9), 4);
    first=1;
#ifdef TCP_CORK
#ifdef AAP_DEBUG
    fprintf(stderr, "cork... \n");
#endif
    {
      int true=1;
      setsockopt( a->to->fd, SOL_TCP, TCP_CORK, &true, 4 );
    }
#endif
    fail = WRITE(a->to->fd, a->data, a->data_len);
    a->sent += fail;
    if(fail != a->data_len)
      goto end;
  }
  fail = 0;

#if !defined(HAVE_FREEBSD_SENDFILE) && defined(HAVE_SENDFILE)
  if(a->len)
  {
#ifdef AAP_DEBUG
    fprintf(stderr, "pre sendfile... \n");
#endif
    if(!first)
    {
      first=1;
      fail = read(a->from_fd, foo, 10);
      if(fail < 0)
        goto end;
      WRITE( a->to->fd, foo, fail );
      a->len  -= fail;
    }
#ifdef AAP_DEBUG
    fprintf(stderr, "sendfile... \n");
#endif
    switch(fail = sendfile(a->to->fd, a->from_fd, NULL, a->len ))
    {
     case -ENOSYS:
#ifdef AAP_DEBUG
       fprintf(stderr, "syscall does not exist.\n");
#endif
       goto normal;
     default:
       if(fail != a->len)
         fprintf(stderr, "sendfile returned %d; len=%d\n", fail, a->len);
    }
    goto end;
  }
#endif /* HAVE_SENDFILE && !HAVE_FREEBSD_SENDFILE */

 normal:
#ifdef AAP_DEBUG
  fprintf(stderr, "using normal fallback... \n");
#endif
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
    int nread, written=0;
    nread = fd_read(a->from_fd, a->buffer, MY_MIN(BUFFER,a->len));
    if(!first)
    {
      first=1;
      MEMCPY(foo,a->buffer+9,5);
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
    if(fail || (WRITE(a->to->fd, a->buffer, nread) != nread))
      goto end;
  }

 end:
#ifdef TCP_CORK
  {
    int false = 0;
    setsockopt( a->to->fd, SOL_TCP, TCP_CORK, &false, 4 );
  }
#endif
  {
    struct args *arg = a->to;
    LOG(a->sent, a->to, atoi(foo));
    if(a->from_fd) close(a->from_fd);
    if(a->data) free(a->data);
    free(a);

    if(!fail && 
       ((arg->res.protocol==s_http_11)
	||aap_get_header(arg, "Connection", H_EXISTS, 0)))
    {
      aap_handle_connection(arg);
    }
    else
    {
      if(arg->res.data) free(arg->res.data);
      if(arg->fd) close(arg->fd);
      free(arg);
    }
  }
}

void f_aap_reply(INT32 args)
{
  int reply_string=0, reply_object=0;
  struct send_args *q;
  if(!THIS->request)
    error("reply already called.\n");
  if(args && sp[-args].type == T_STRING) 
    reply_string = 1;

  if(args>1)
  {
    if(args<3)
      error("->reply(string|void pre,object(Stdio.file) fd,int len)\n");
    if(sp[-args+1].type != T_OBJECT)
      error("Bad argument 2 to reply\n");
    if(sp[-args+2].type != T_INT)
      error("Bad argument 3 to reply\n");
    reply_object = 1;
  }

  if(reply_string && !reply_object)
  {
    if(sp[-1].u.string->len < 8192)
    {
      int amnt = WRITE(THIS->request->fd,sp[-1].u.string->str,
		       sp[-1].u.string->len);
      LOG(amnt, THIS->request, 
	  atoi(sp[-1].u.string->str+MY_MIN(sp[-1].u.string->len,9)));
      if((sp[-1].u.string->len == amnt) &&
	 ((THIS->request->res.protocol==s_http_11)
	  ||aap_get_header(THIS->request, "Connection", H_EXISTS, 0)))
      {
	th_farm((void (*)(void *))aap_handle_connection, 
		(void *)THIS->request);
	THIS->request = 0;
	return;
      } else {
	if(THIS->request->res.data)
	  free(THIS->request->res.data);
	close(THIS->request->fd);
	free(THIS->request);
	THIS->request = 0;
	return;
      }
    }
  }

  q = malloc(sizeof(struct send_args));
  q->to = THIS->request;
  THIS->request = 0;

  if(reply_object)
  {
    /* safe_apply() needed to avoid leak of q */
    safe_apply(sp[-2].u.object, "query_fd", 0);
    if((sp[-1].type != T_INT) || (sp[-1].u.integer <= 0))
    {
      free(q);
      error("Bad fileobject to request_object->reply()\n");
    }
    if((q->from_fd = dup(sp[-1].u.integer)) == -1)
      error("Bad file object to request_object->reply()\n");
    pop_stack();

    q->len = sp[-1].u.integer;
  } else {
    q->from_fd = 0;
    q->len = 0;
  }

  if(reply_string)
  {
    char *s = malloc(sp[-args].u.string->len);
    MEMCPY(s, sp[-args].u.string->str, sp[-args].u.string->len);
    q->data = s;
    q->data_len = sp[-args].u.string->len;
  } else {
    q->data = 0;
    q->data_len = 0;
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
  int time_to_keep, t, freed=0;
  if(!THIS->request)
    error("Reply already called.\n");

  get_all_args("reply_with_cache", args, "%S%d", &reply, &time_to_keep);

  if(reply->len < THIS->request->cache->max_size/2)
  {
    if(THIS->request->cache->gone) /* freed..*/
    {
      close(THIS->request->fd);
      if(THIS->request->res.data)
	free(THIS->request->res.data);
      free(THIS->request);
      THIS->request = 0;
      return;
    }
    t = aap_get_time();
    mt_lock(&THIS->request->cache->mutex);
    if(THIS->request->cache->size > THIS->request->cache->max_size)
    {
      struct cache_entry *p,*pp=0,*ppp=0;
      if(THIS->request->cache->unclean) 
	aap_clean_cache(THIS->request->cache, 1);
      while(THIS->request->cache->size > THIS->request->cache->max_size)
      {
	int i;
	freed=0;
	for(i = 0; i<CACHE_HTABLE_SIZE; i++)
	{
	  p = THIS->request->cache->htable[i];
	  ppp=pp=0;
	  while(p)
	  {
	    ppp = pp;
	    pp = p;
	    p = p->next;
	  }
	  if(pp) aap_free_cache_entry(THIS->request->cache,pp,ppp,i);
	  freed++;
	  if(THIS->request->cache->size < THIS->request->cache->max_size)
	    break;
	}
	if(!freed)  /* no way.. */
	  break;
      }
    }
    ce = malloc(sizeof(struct cache_entry));
    MEMSET(ce, 0, sizeof(struct cache_entry));
    ce->stale_at = t+time_to_keep;
    ce->data = reply;
    ce->url = THIS->request->res.url;
    ce->url_len = THIS->request->res.url_len;
    reply->refs++;

    ce->host = THIS->request->res.host;
    ce->host_len = THIS->request->res.host_len;
    aap_cache_insert(ce, THIS->request->cache);
    mt_unlock(&THIS->request->cache->mutex);
  }
  pop_stack();
  f_aap_reply(1);
}

void f_aap_reqo__init(INT32 args)
{
  if(THIS->request->res.protocol)
    SINSERT(THIS->misc_variables, s_prot, THIS->request->res.protocol);
  IINSERT(THIS->misc_variables, s_time, aap_get_time());
  TINSERT(THIS->misc_variables, s_rawurl, 
	  THIS->request->res.url, THIS->request->res.url_len);
}

void aap_init_request_object(struct object *o)
{
  MEMSET(THIS, 0, sizeof(*THIS));
}

void aap_exit_request_object(struct object *o)
{
  if(THIS->request)
  {
    close(THIS->request->fd);
    if(THIS->request->res.data) free(THIS->request->res.data);
    free(THIS->request);
  }
  if(THIS->misc_variables)
    free_mapping(THIS->misc_variables);
  if(THIS->done_headers)
    free_mapping(THIS->done_headers);
}
#endif
