/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef BUFFER_H
#define BUFFER_H

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "bitvector.h"
#include "pike_memory.h"
#include "global.h"

#define MACRO   static inline PIKE_UNUSED_ATTRIBUTE

struct byte_buffer {
    void * restrict dst;
    void * end;
    size_t length;
    unsigned INT16 flags;
};

static const INT16 BUFFER_GROW_EXACT = 0x1;
static const INT16 BUFFER_GROW_MEXEC = 0x2;

#define BUFFER_INIT() CAST_STRUCT_LITERAL(struct byte_buffer){ NULL, NULL, 0, 0 }

PMOD_EXPORT void buffer_free(struct byte_buffer *b);
PMOD_EXPORT int buffer_grow_nothrow(struct byte_buffer *b, size_t len);
PMOD_EXPORT void buffer_grow(struct byte_buffer *b, size_t len);
PMOD_EXPORT const char *buffer_get_string(struct byte_buffer *b);

PIKE_WARN_UNUSED_RESULT_ATTRIBUTE
PMOD_EXPORT void* buffer_finish(struct byte_buffer *b);

PIKE_WARN_UNUSED_RESULT_ATTRIBUTE
PMOD_EXPORT struct pike_string *buffer_finish_pike_string(struct byte_buffer *b);

PIKE_WARN_UNUSED_RESULT_ATTRIBUTE
MACRO void* buffer_ptr(const struct byte_buffer *b) {
    return (char*)b->end - b->length;
}

MALLOC_FUNCTION MACRO void *buffer_dst(const struct byte_buffer *b) {
    return b->dst;
}

MACRO void buffer_set_flags(struct byte_buffer *b, INT16 flags) {
    b->flags = flags;
}

MACRO void buffer_add_flags(struct byte_buffer *b, INT16 flags) {
    b->flags |= flags;
}

PIKE_WARN_UNUSED_RESULT_ATTRIBUTE
MACRO size_t buffer_content_length(const struct byte_buffer *b) {
    return (char*)b->dst - (char*)buffer_ptr(b);
}

PIKE_WARN_UNUSED_RESULT_ATTRIBUTE
MACRO size_t buffer_length(const struct byte_buffer *b) {
    return b->length;
}

PIKE_WARN_UNUSED_RESULT_ATTRIBUTE
MACRO size_t buffer_space(struct byte_buffer *b) {
    return (char*)b->end - (char*)b->dst;
}

MACRO void buffer_init(struct byte_buffer *buf) {
    struct byte_buffer b = BUFFER_INIT();
    *buf = b;
}

MACRO void buffer_clear(struct byte_buffer *b) {
    b->dst = buffer_ptr(b);
}

#ifdef PIKE_DEBUG
PMOD_EXPORT void buffer_remove(struct byte_buffer *b, size_t len);
#else
MACRO void buffer_remove(struct byte_buffer *b, size_t len) {
    b->dst = (char*)b->dst - len;
}
#endif

#ifdef PIKE_DEBUG
PMOD_EXPORT void buffer_check_space(struct byte_buffer *b, size_t len);
PMOD_EXPORT void buffer_check_position(struct byte_buffer *b, size_t pos);
#else
MACRO void buffer_check_space(struct byte_buffer *PIKE_UNUSED(b), size_t PIKE_UNUSED(len)) { }
MACRO void buffer_check_position(struct byte_buffer *PIKE_UNUSED(b), size_t PIKE_UNUSED(pos)) { }
#endif

MACRO void buffer_advance(struct byte_buffer *b, size_t len) {
    char *dst = buffer_dst(b);
    buffer_check_space(b, len);
    b->dst = dst + len;
}

MACRO void buffer_init_from_mem(struct byte_buffer *b, void *ptr, size_t len, size_t pos) {
    b->dst = ptr;
    b->end = (char*)ptr + len;
    b->length = len;
    buffer_advance(b, pos);
}

/**
 * Ensures that the buffer has at least len bytes free space.
 * If more space is be allocated, it will be power of 2 bytes.
 * Throws an exception in out-of-memory situations.
 */
MACRO void* buffer_ensure_space(struct byte_buffer *b, size_t len) {
    if (UNLIKELY((char*)b->dst + len > (char*)b->end)) {
        buffer_grow(b, len);

        STATIC_ASSUME((char*)b->dst + len <= (char*)b->end);
    }
    return buffer_dst(b);
}

/**
 * Same as buffer_ensure_space() but returns 0 in out-of-memory situations
 * instead of throwing an exception.
 */
MACRO int buffer_ensure_space_nothrow(struct byte_buffer *b, size_t len) {
    if (UNLIKELY((char*)b->dst + len > (char*)b->end)) {
        if (!buffer_grow_nothrow(b, len)) return 0;

        STATIC_ASSUME((char*)b->dst + len <= (char*)b->end);
    }

    return 1;
}

#define ASSUME_UNCHANGED(b, CODE) do {          \
    const struct byte_buffer __B = *b;          \
    do CODE while(0);                           \
    STATIC_ASSUME(__B.dst == b->dst);           \
    STATIC_ASSUME(__B.end == b->end);           \
    STATIC_ASSUME(__B.length == b->length);     \
    STATIC_ASSUME(__B.flags == b->flags);       \
} while (0)

MACRO void buffer_memcpy_unsafe(struct byte_buffer *b, const void * src, size_t len) {
    unsigned INT8 * restrict dst = buffer_dst(b);
    buffer_check_space(b, len);
    ASSUME_UNCHANGED(b, {
        memcpy(dst, src, len);
    });
    b->dst = dst + len;
}
MACRO void buffer_memcpy(struct byte_buffer *b, const void * src, size_t len) {
    buffer_ensure_space(b, len);
    buffer_memcpy_unsafe(b, src, len);
}

MACRO void buffer_add_str_unsafe(struct byte_buffer *b, const char *str) {
    buffer_memcpy_unsafe(b, str, strlen(str));
}

MACRO void buffer_add_str(struct byte_buffer *b, const char * str) {
    buffer_memcpy(b, str, strlen(str));
}

MALLOC_FUNCTION
MACRO void* buffer_alloc_unsafe(struct byte_buffer *b, size_t len) {
    char * dst = buffer_dst(b);

    buffer_check_space(b, len);

    b->dst = dst + len;

    return dst;
}

MALLOC_FUNCTION
MACRO void* buffer_alloc(struct byte_buffer *b, size_t len) {
    buffer_ensure_space(b, len);
    return buffer_alloc_unsafe(b, len);
}

#define DO_TRANSFORM(VAR, TRANS)		VAR = TRANS(VAR)
#define GEN_BUFFER_ADD(name, type, transform, transform_back)                   \
MACRO void buffer_add_ ## name ## _unsafe(struct byte_buffer *b, type a) {      \
    unsigned INT8 * dst = buffer_dst(b);                                        \
    buffer_check_space(b, sizeof(a));                                           \
    ASSUME_UNCHANGED(b, {                                                       \
        DO_TRANSFORM(a, transform);						\
        memcpy(dst, &a, sizeof(a));                                             \
    });                                                                         \
    b->dst = dst + sizeof(a);                                                   \
}                                                                               \
MACRO void buffer_add_ ## name (struct byte_buffer *b, type a) {                \
    buffer_ensure_space(b, sizeof(a));                                          \
    buffer_add_##name##_unsafe(b, a);                                           \
}                                                                               \
MACRO void buffer_set_ ## name (struct byte_buffer *b, size_t pos, type a) {    \
    unsigned INT8 * dst = buffer_ptr(b);                                        \
    /* If the second addition overflows, the first check will fail */           \
    buffer_check_position(b, pos);                                              \
    buffer_check_position(b, pos + sizeof(a));                                  \
    dst += pos;                                                                 \
    ASSUME_UNCHANGED(b, {                                                       \
        DO_TRANSFORM(a, transform);						\
        memcpy(dst, &a, sizeof(a));                                             \
    });                                                                         \
}                                                                               \
MACRO type buffer_get_ ## name (struct byte_buffer *b, size_t pos) {            \
    type a;                                                                     \
    unsigned INT8 * src = buffer_ptr(b);                                        \
    /* If the second addition overflows, the first check will fail */           \
    buffer_check_position(b, pos);                                              \
    buffer_check_position(b, pos + sizeof(a));                                  \
    src += pos;                                                                 \
    memcpy(&a, src, sizeof(a));                                                 \
    DO_TRANSFORM(a, transform_back);						\
    return a;                                                                   \
}

GEN_BUFFER_ADD(be16, unsigned INT16, hton16, ntoh16)
GEN_BUFFER_ADD(le16, unsigned INT16, ntoh16, hton16)

GEN_BUFFER_ADD(be32, unsigned INT32, hton32, ntoh32)
GEN_BUFFER_ADD(le32, unsigned INT32, ntoh32, hton32)

GEN_BUFFER_ADD(be64, UINT64, hton64, ntoh64)
GEN_BUFFER_ADD(le64, UINT64, ntoh64, hton64)

#undef DO_TRANSFORM
#define DO_TRANSFORM(VAR, TRANS)

GEN_BUFFER_ADD(u16, unsigned INT16, , )
GEN_BUFFER_ADD(u32, unsigned INT32, , )
GEN_BUFFER_ADD(u64, UINT64, , )

GEN_BUFFER_ADD(u8, unsigned INT8, , )
GEN_BUFFER_ADD(char, char, , )

#undef DO_TRANSFORM
#undef GEN_BUFFER_ADD
#undef MACRO
#undef ASSUME_UNCHANGED

#define BUFFER_ADD_UNSAFE(b, a) do {            \
    struct byte_buffer *__b__ = (b);            \
    unsigned INT8 * dst = buffer_dst(__b__);    \
    buffer_check_space(__b__, sizeof(a));       \
    ASSUME_UNCHANGED(__b__, {                   \
        memcpy(dst, &a, sizeof(a));             \
    });                                         \
    __b__->dst = dst + sizeof(a);               \
} while(0)
#define BUFFER_ADD(b, a) do {                   \
    buffer_ensure_space(b, sizeof(a));          \
    BUFFER_ADD_UNSAFE(b, a);                    \
} while(0)
#define BUFFER_SET(b, pos, a) do {                      \
    struct byte_buffer *__b__ = (b);                    \
    size_t __pos__ = (pos);                             \
    unsigned INT8 * dst = buffer_ptr(__b__);            \
    buffer_check_position(__b__, __pos__);              \
    buffer_check_position(__b__, __pos__ + sizeof(a));  \
    ASSUME_UNCHANGED(__b__, {                           \
        memcpy(dst, &a, sizeof(a));                     \
    });                                                 \
} while(0)

#endif /* BUFFER_H */
