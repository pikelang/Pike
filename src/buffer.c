/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "buffer.h"
#include "stralloc.h"
#include "svalue.h"
#include "pike_error.h"
#include "bignum.h"

PMOD_EXPORT void buffer_free(struct byte_buffer *b) {
    if (!b->dst) return;
    free(buffer_ptr(b));
    buffer_init(b);
}

static void *buffer_realloc(struct byte_buffer *b, size_t len) {
    if (b->flags & BUFFER_GROW_MEXEC) {
        return mexec_realloc(buffer_ptr(b), len);
    } else {
        return realloc(buffer_ptr(b), len);
    }
}

PMOD_EXPORT int buffer_grow_nothrow(struct byte_buffer *b, size_t len) {
    const size_t content_length = buffer_content_length(b);
    size_t new_length, needed;
    unsigned INT16 flags = b->flags;
    void *ptr;

#ifdef PIKE_DEBUG
    if (buffer_space(b) >= len) Pike_fatal("Misuse of buffer_grow.\n");
#endif
    if (DO_SIZE_T_ADD_OVERFLOW(len, content_length, &needed)) goto OUT_OF_MEMORY_FATAL;

    if (!(flags & BUFFER_GROW_EXACT)) {
        new_length = b->length;
        if (new_length < 16) new_length = 16;

        while (new_length < needed) {
            if (DO_SIZE_T_MUL_OVERFLOW(new_length, 2, &new_length)) goto OUT_OF_MEMORY_FATAL;
        }
    } else {
        new_length = needed;
    }

#if 0
    fprintf(stderr, "BUF { left=%llu; length=%llu; dst=%p; }\n", buffer_space(b), b->length, b->dst);
#endif

    ptr = buffer_realloc(b, new_length);

    if (UNLIKELY(!ptr)) return 0;

#if 0
    fprintf(stderr, "realloc(%p, %llu) = %p\n", buffer_ptr(b), new_length, ptr);
    fprintf(stderr, "content_length: %llu\n", content_length);
#endif

    b->dst = (char*)ptr + content_length;
    b->end = (char*)ptr + new_length;
    b->length = new_length;
    return 1;
OUT_OF_MEMORY_FATAL:
    /* This happens on overflow. */
#ifdef PIKE_DEBUG
    Pike_fatal(msg_out_of_mem_2, len);
#else
    return 0;
#endif
}

PMOD_EXPORT void buffer_grow(struct byte_buffer *b, size_t len) {
    if (UNLIKELY(!buffer_grow_nothrow(b, len))) Pike_error(msg_out_of_mem_2, len);
}

PMOD_EXPORT void* buffer_finish(struct byte_buffer *b) {
    void *ret;

    if (buffer_content_length(b) < buffer_length(b)) {
        ret = buffer_realloc(b, buffer_content_length(b));
        /* we cannot shrink, that does not matter */
        if (!ret) ret = buffer_ptr(b);
    } else ret = buffer_ptr(b);

    buffer_init(b);

    return ret;
}

PMOD_EXPORT struct pike_string *buffer_finish_pike_string(struct byte_buffer *b) {
    struct pike_string *ret;
    size_t len = buffer_content_length(b);

    if (!len) {
        buffer_free(b);
        ret = empty_pike_string;
        add_ref(ret);
        return ret;
    }

    /* zero terminate */
    buffer_add_char(b, 0);

    return make_shared_malloc_string(buffer_finish(b), len, eightbit);
}

PMOD_EXPORT const char *buffer_get_string(struct byte_buffer *b) {
    buffer_ensure_space(b, 1);

    /* zero terminate */
    *(char*)buffer_dst(b) = 0;

    return buffer_ptr(b);
}

#ifdef PIKE_DEBUG
PMOD_EXPORT void buffer_check_space(struct byte_buffer *b, size_t len) {
    if (len > buffer_space(b))
        Pike_fatal("buffer_check_space failed.\n");
}
PMOD_EXPORT void buffer_check_position(struct byte_buffer *b, size_t pos) {
    if (pos > b->length)
        Pike_fatal("buffer_check_position failed.\n");
}
PMOD_EXPORT void buffer_remove(struct byte_buffer *b, size_t len) {
    if (len > buffer_content_length(b))
        Pike_fatal("Bad call to buffer_remove.\n");

    b->dst = (char*)b->dst - len;
}
#endif
