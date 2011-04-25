/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef DYNAMIC_BUFFER_H
#define DYNAMIC_BUFFER_H

#define BUFFER_BEGIN_SIZE 4080

struct dynbuf_string_s
{
  char *str;
  SIZE_T len;
};

typedef struct dynbuf_string_s dynbuf_string;

struct dynamic_buffer_s
{
  dynbuf_string s;
  SIZE_T bufsize;
};

typedef struct dynamic_buffer_s dynamic_buffer;

/* Prototypes begin here */
PMOD_EXPORT char *low_make_buf_space(size_t space, dynamic_buffer *buf);
PMOD_EXPORT void low_my_putchar(char b,dynamic_buffer *buf);
PMOD_EXPORT void low_my_binary_strcat(const char *b, size_t l, dynamic_buffer *buf);
PMOD_EXPORT void debug_initialize_buf(dynamic_buffer *buf);
PMOD_EXPORT void low_reinit_buf(dynamic_buffer *buf);
PMOD_EXPORT void low_init_buf_with_string(dynbuf_string s, dynamic_buffer *buf);
PMOD_EXPORT dynbuf_string complex_free_buf(void);
PMOD_EXPORT void toss_buffer(dynamic_buffer *buf);
PMOD_EXPORT char *simple_free_buf(void);
PMOD_EXPORT struct pike_string *debug_low_free_buf(dynamic_buffer *buf);
PMOD_EXPORT struct pike_string *debug_free_buf(void);
PMOD_EXPORT char *make_buf_space(INT32 space);
PMOD_EXPORT void my_putchar(char b);
PMOD_EXPORT void my_binary_strcat(const char *b, ptrdiff_t l);
PMOD_EXPORT void my_strcat(const char *b);
PMOD_EXPORT void initialize_global_buf(void);
PMOD_EXPORT void init_buf(void);
PMOD_EXPORT void init_buf_with_string(dynbuf_string s);
PMOD_EXPORT char *debug_return_buf(void);
/* Prototypes end here */

#ifdef DEBUG_MALLOC
#define initialize_buf(X) \
  do { dynamic_buffer *b_=(X); debug_initialize_buf(b_); \
   debug_malloc_touch(b_->s.str); } while(0)
#define low_free_buf(X) \
  ((struct pike_string *)debug_malloc_pass(debug_low_free_buf(X)))

#define free_buf() \
  ((struct pike_string *)debug_malloc_pass(debug_free_buf()))

#define return_buf() \
  ((char *)debug_malloc_pass(debug_return_buf()))

#else
#define initialize_buf debug_initialize_buf
#define low_free_buf debug_low_free_buf
#define free_buf debug_free_buf
#define return_buf debug_return_buf
#endif

#endif
