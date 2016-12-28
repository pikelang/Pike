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

#endif
