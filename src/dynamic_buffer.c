/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: dynamic_buffer.c,v 1.25 2004/09/18 20:50:48 nilsson Exp $
*/

#include "global.h"
#include "dynamic_buffer.h"
#include "stralloc.h"
#include "pike_error.h"
#include "pike_memory.h"

dynamic_buffer pike_global_buffer;

PMOD_EXPORT char *low_make_buf_space(size_t space, dynamic_buffer *buf)
{
  char *ret;
#ifdef PIKE_DEBUG
  if(!buf->s.str) Pike_fatal("ARRRRGH! Deadly Trap!\n");
#endif

  if(buf->s.len+space >= buf->bufsize)
  {
    if(!buf->bufsize) buf->bufsize=1;

    do{
      buf->bufsize*=2;
    }while(buf->s.len+space >= buf->bufsize);

    buf->s.str=(char *)realloc(buf->s.str, buf->bufsize);
    if(!buf->s.str)
      Pike_error("Out of memory.\n");
  }
  ret = buf->s.str + buf->s.len;
  buf->s.len += space;
  return ret;
}

PMOD_EXPORT void low_my_putchar(int b,dynamic_buffer *buf)
{
#ifdef PIKE_DEBUG
  if(!buf->s.str)
    Pike_fatal("Error in internal buffering.\n");
#endif
  low_make_buf_space(1,buf)[0]=b;
}

PMOD_EXPORT void low_my_binary_strcat(const char *b, size_t l,
				      dynamic_buffer *buf)
{
#ifdef PIKE_DEBUG
  if(!buf->s.str)
    Pike_fatal("Error in internal buffering.\n");
#endif

  MEMCPY(low_make_buf_space(l, buf),b, l);
}

PMOD_EXPORT void debug_initialize_buf(dynamic_buffer *buf)
{
  buf->s.str=(char *)xalloc((buf->bufsize=BUFFER_BEGIN_SIZE));
  *(buf->s.str)=0;
  buf->s.len=0;
}

PMOD_EXPORT void low_reinit_buf(dynamic_buffer *buf)
{
  if(!buf->s.str)
  {
    initialize_buf(buf);
  }else{
    *(buf->s.str)=0;
    buf->s.len=0;
  }
}

PMOD_EXPORT void low_init_buf_with_string(dynbuf_string s, dynamic_buffer *buf)
{
  if(buf->s.str) { free(buf->s.str); buf->s.str=NULL; } 
  buf->s=s;
  if(!buf->s.str) initialize_buf(buf);
  /* if the string is an old buffer, this realloc will set the old
     the bufsize back */
  for(buf->bufsize=BUFFER_BEGIN_SIZE;buf->bufsize<buf->s.len;buf->bufsize*=2);
  buf->s.str=realloc(buf->s.str,buf->bufsize);
#ifdef PIKE_DEBUG
  if(!buf->s.str)
    Pike_fatal("Realloc failed.\n");
#endif
}

PMOD_EXPORT dynbuf_string complex_free_buf(dynamic_buffer *old_buf)
{
  dynbuf_string tmp;
  if(!pike_global_buffer.s.str) return pike_global_buffer.s;
  my_putchar(0);
  pike_global_buffer.s.len--;
  tmp=pike_global_buffer.s;
  pike_global_buffer = *old_buf;
  return tmp;
}

PMOD_EXPORT void toss_buffer(dynamic_buffer *buf)
{
  if(buf->s.str) free(buf->s.str);
  buf->s.str=0;
}

PMOD_EXPORT char *simple_free_buf(dynamic_buffer *old_buf)
{
  if(!pike_global_buffer.s.str) return 0;
  return complex_free_buf(old_buf).str;
}

PMOD_EXPORT struct pike_string *debug_low_free_buf(dynamic_buffer *buf)
{
  struct pike_string *q;
  if(!buf->s.str) return 0;
  q=make_shared_binary_string(buf->s.str,buf->s.len);
  free(buf->s.str);
  buf->s.str=0;
  buf->s.len=0;
  return q;
}

PMOD_EXPORT struct pike_string *debug_free_buf(dynamic_buffer *old_buf)
{
  struct pike_string *res = low_free_buf(&pike_global_buffer);
  pike_global_buffer = *old_buf;
  return res;
}

PMOD_EXPORT char *make_buf_space(INT32 space)
{
  return low_make_buf_space(space,&pike_global_buffer);
}

PMOD_EXPORT void my_putchar(int b)
{
  low_my_putchar(b,&pike_global_buffer);
}

PMOD_EXPORT void my_binary_strcat(const char *b, ptrdiff_t l)
{
  low_my_binary_strcat(b,l,&pike_global_buffer);
}

PMOD_EXPORT void my_strcat(const char *b)
{
  my_binary_strcat(b,strlen(b));
}

PMOD_EXPORT void init_buf(dynamic_buffer *old_buf)
{
  *old_buf = pike_global_buffer;
  pike_global_buffer.s.str = NULL;
  initialize_buf(&pike_global_buffer);
}

PMOD_EXPORT void init_buf_with_string(dynamic_buffer *old_buf, dynbuf_string s)
{
  *old_buf = pike_global_buffer;
  pike_global_buffer.s.str = NULL;
  low_init_buf_with_string(s,&pike_global_buffer);
}

PMOD_EXPORT void save_buffer (dynamic_buffer *save_buf)
{
  *save_buf = pike_global_buffer;
  pike_global_buffer.s.str = NULL;
}

PMOD_EXPORT void restore_buffer (dynamic_buffer *save_buf)
{
#ifdef PIKE_DEBUG
  if (pike_global_buffer.s.str)
    Pike_fatal ("Global buffer already in use.\n");
#endif
  pike_global_buffer = *save_buf;
}

#if 0
PMOD_EXPORT char *debug_return_buf(void)
{
  my_putchar(0);
  return pike_global_buffer.s.str;
}
#endif

/* int my_get_buf_size() {  return pike_global_buffer->s.len; } */
