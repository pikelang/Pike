#include "global.h"
#include "dynamic_buffer.h"
#include "stralloc.h"
#include "error.h"

static dynamic_buffer buff;

char *low_make_buf_space(INT32 space,dynamic_buffer *buf)
{
  char *ret;
  if(buf->s.len+space>=buf->bufsize)
  {
    do{
      buf->bufsize*=2;
    }while(buf->s.len+space>=buf->bufsize);

    buf->s.str=(char *)realloc(buf->s.str,buf->bufsize);
    if(!buf->s.str)
      error("Out of memory.\n");
  }
  ret=buf->s.str + buf->s.len;
  buf->s.len+=space;
  return ret;
}

void low_my_putchar(char b,dynamic_buffer *buf)
{
#ifdef DEBUG
  if(!buf->s.str)
    fatal("Error in internal buffering.\n");
#endif
  low_make_buf_space(1,buf)[0]=b;
}

void low_my_binary_strcat(const char *b,INT32 l,dynamic_buffer *buf)
{
#ifdef DEBUG
  if(!buf->s.str)
    fatal("Error in internal buffering.\n");
#endif

  MEMCPY(low_make_buf_space(l,buf),b,l);
}

void low_init_buf(dynamic_buffer *buf)
{
  if(!buf->s.str)
    buf->s.str=(char *)xalloc((buf->bufsize=BUFFER_BEGIN_SIZE));
  if(!buf->s.str)
    error("Out of memory.\n");
  *(buf->s.str)=0;
  buf->s.len=0;
}

void low_init_buf_with_string(string s,dynamic_buffer *buf)
{
  if(buf->s.str) { free(buf->s.str); buf->s.str=NULL; } 
  buf->s=s;
  if(!buf->s.str) init_buf();
  /* if the string is an old buffer, this realloc will set the old
     the bufsize back */
  for(buf->bufsize=BUFFER_BEGIN_SIZE;buf->bufsize<buf->s.len;buf->bufsize*=2);
  buf->s.str=realloc(buf->s.str,buf->bufsize);
}

string complex_free_buf(void)
{
  string tmp;
  if(!buff.s.str) return buff.s;
  my_putchar(0);
  buff.s.len--;
  tmp=buff.s;
  buff.s.str=0;
  return tmp;
}

void toss_buffer(dynamic_buffer *buf)
{
  if(buf->s.str) free(buf->s.str);
  buf->s.str=0;
}

char *simple_free_buf(void)
{
  if(!buff.s.str) return 0;
  return complex_free_buf().str;
}

struct lpc_string *low_free_buf(dynamic_buffer *buf)
{
  struct lpc_string *q;
  if(!buf->s.str) return 0;
  q=make_shared_binary_string(buf->s.str,buf->s.len);
  free(buf->s.str);
  buf->s.str=0;
  buf->s.len=0;
  return q;
}

struct lpc_string *free_buf(void) { return low_free_buf(&buff); }
char *make_buf_space(INT32 space) { return low_make_buf_space(space,&buff); }
void my_putchar(char b) { low_my_putchar(b,&buff); }
void my_binary_strcat(const char *b,INT32 l) { low_my_binary_strcat(b,l,&buff); }
void my_strcat(const char *b) { my_binary_strcat(b,strlen(b)); }
void init_buf(void) { low_init_buf(&buff); }
void init_buf_with_string(string s) { low_init_buf_with_string(s,&buff); }
char *return_buf(void)
{
  my_putchar(0);
  return buff.s.str;
}
/* int my_get_buf_size() {  return buff->s.len; } */

