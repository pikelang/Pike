/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef DYNAMIC_BUFFER_H
#define DYNAMIC_BUFFER_H

#include "buffer.h"

typedef struct byte_buffer dynamic_buffer;
typedef struct byte_buffer dynbuf_string;

#define MACRO static inline PIKE_UNUSED_ATTRIBUTE

/* Prototypes begin here */
MACRO char *low_make_buf_space(ptrdiff_t space, dynamic_buffer *buf) {
    if (space < 0) {
        void * ret = buffer_dst(buf);
        buffer_remove(buf, -space);
        return ret;
    } else {
        return buffer_alloc(buf, space);
    }
}
MACRO void low_my_putchar(int b,dynamic_buffer *buf) {
    buffer_add_char(buf, b);
}
MACRO void low_my_binary_strcat(const char *b, size_t l, dynamic_buffer *buf) {
    buffer_memcpy(buf, b, l);
}

// NOTE: dynamic buffer did zero terminate
MACRO void initialize_buf(dynamic_buffer *buf) {
    buffer_init(buf);
}
MACRO void low_reinit_buf(dynamic_buffer *buf) {
    buffer_clear(buf);
}
MACRO void toss_buffer(dynamic_buffer *buf) {
    buffer_free(buf);
}
MACRO struct pike_string *low_free_buf(dynamic_buffer *buf) {
    return buffer_finish_pike_string(buf);
}

/* GLOBAL BUFFER LOGIC */

PMOD_EXPORT extern dynamic_buffer pike_global_buffer;

MACRO dynbuf_string complex_free_buf(dynamic_buffer *old_buf) {
  dynbuf_string tmp;
  if(!buffer_ptr(&pike_global_buffer)) return pike_global_buffer;
  tmp=pike_global_buffer;
  buffer_add_char(&tmp, 0);
  buffer_remove(&tmp, 1);
  pike_global_buffer = *old_buf;
  return tmp;
}
MACRO char *simple_free_buf(dynamic_buffer *old_buf) {
  dynbuf_string tmp;
  if(!buffer_ptr(&pike_global_buffer)) return 0;
  tmp = complex_free_buf(old_buf);
  buffer_add_char(&tmp, 0);
  return buffer_ptr(&tmp);
}
MACRO struct pike_string *free_buf(dynamic_buffer *old_buf) {
  struct pike_string *res = low_free_buf(&pike_global_buffer);
  pike_global_buffer = *old_buf;
  return res;
}

MACRO void abandon_buf(dynamic_buffer *old_buf) {
  toss_buffer(&pike_global_buffer);
  pike_global_buffer = *old_buf;
}

MACRO char *make_buf_space(INT32 space) {
  return low_make_buf_space(space,&pike_global_buffer);
}

MACRO void my_putchar(int b) {
  low_my_putchar(b,&pike_global_buffer);
}

MACRO void my_binary_strcat(const char *b, ptrdiff_t l) {
  low_my_binary_strcat(b,l,&pike_global_buffer);
}
MACRO void my_strcat(const char *b) {
  my_binary_strcat(b,strlen(b));
}
MACRO void init_buf(dynamic_buffer *old_buf) {
  *old_buf = pike_global_buffer;
  initialize_buf(&pike_global_buffer);
}
MACRO void save_buffer (dynamic_buffer *save_buf) {
  *save_buf = pike_global_buffer;
  initialize_buf(&pike_global_buffer);
}
MACRO void restore_buffer (const dynamic_buffer *save_buf) {
#ifdef PIKE_DEBUG
  if (buffer_content_length(&pike_global_buffer))
    Pike_fatal ("Global buffer already in use.\n");
#endif
  pike_global_buffer = *save_buf;
}

#endif
