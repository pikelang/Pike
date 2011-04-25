/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#include "global.h"
#include "dynamic_buffer.h"
#include "stralloc.h"
#include "pike_error.h"
#include "pike_memory.h"

RCSID("$Id$");

static dynamic_buffer buff;

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

PMOD_EXPORT void low_my_putchar(char b,dynamic_buffer *buf)
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

PMOD_EXPORT dynbuf_string complex_free_buf(void)
{
  dynbuf_string tmp;
  if(!buff.s.str) return buff.s;
  my_putchar(0);
  buff.s.len--;
  tmp=buff.s;
  buff.s.str=0;
  return tmp;
}

PMOD_EXPORT void toss_buffer(dynamic_buffer *buf)
{
  if(buf->s.str) free(buf->s.str);
  buf->s.str=0;
}

PMOD_EXPORT char *simple_free_buf(void)
{
  if(!buff.s.str) return 0;
  return complex_free_buf().str;
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

PMOD_EXPORT struct pike_string *debug_free_buf(void) { return low_free_buf(&buff); }
PMOD_EXPORT char *make_buf_space(INT32 space) { return low_make_buf_space(space,&buff); }
PMOD_EXPORT void my_putchar(char b) { low_my_putchar(b,&buff); }
PMOD_EXPORT void my_binary_strcat(const char *b, ptrdiff_t l) { low_my_binary_strcat(b,l,&buff); }
PMOD_EXPORT void my_strcat(const char *b) { my_binary_strcat(b,strlen(b)); }
PMOD_EXPORT void initialize_global_buf(void) { buff.s.str = NULL; }
PMOD_EXPORT void init_buf(void) { low_reinit_buf(&buff); }
PMOD_EXPORT void init_buf_with_string(dynbuf_string s) { low_init_buf_with_string(s,&buff); }
PMOD_EXPORT char *debug_return_buf(void)
{
  my_putchar(0);
  return buff.s.str;
}
/* int my_get_buf_size() {  return buff->s.len; } */
