/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: dynamic_buffer.h,v 1.7 2000/03/20 21:00:04 hubbe Exp $
 */
#ifndef DYNAMIC_BUFFER_H
#define DYNAMIC_BUFFER_H

#define BUFFER_BEGIN_SIZE 4080

struct string_s
{
  char *str;
  SIZE_T len;
};

typedef struct string_s string;

struct dynamic_buffer_s
{
  string s;
  SIZE_T bufsize;
};

typedef struct dynamic_buffer_s dynamic_buffer;

/* Prototypes begin here */
char *low_make_buf_space(INT32 space, dynamic_buffer *buf);
void low_my_putchar(char b,dynamic_buffer *buf);
void low_my_binary_strcat(const char *b,INT32 l,dynamic_buffer *buf);
void debug_initialize_buf(dynamic_buffer *buf);
void low_reinit_buf(dynamic_buffer *buf);
void low_init_buf_with_string(string s, dynamic_buffer *buf);
string complex_free_buf(void);
void toss_buffer(dynamic_buffer *buf);
char *simple_free_buf(void);
struct pike_string *debug_low_free_buf(dynamic_buffer *buf);
struct pike_string *debug_free_buf(void);
char *make_buf_space(INT32 space);
void my_putchar(char b);
void my_binary_strcat(const char *b,INT32 l);
void my_strcat(const char *b);
void init_buf(void);
void init_buf_with_string(string s);
char *debug_return_buf(void);
/* Prototypes end here */

#ifdef DEBUG_MALLOC
#define initialize_buf(X) \
  do { dynamic_buffer *b_=(X); debug_initialize_buf(b_); \
   debug_malloc_touch(b_->s.str); } while(0)
#define low_free_buf(X) \
  ((struct pike_string *)debug_malloc_touch(debug_low_free_buf(X)))

#define free_buf() \
  ((struct pike_string *)debug_malloc_touch(debug_free_buf()))

#define return_buf() \
  ((char *)debug_malloc_touch(debug_return_buf()))

#else
#define initialize_buf debug_initialize_buf
#define low_free_buf debug_low_free_buf
#define free_buf debug_free_buf
#define return_buf debug_return_buf
#endif

#endif
