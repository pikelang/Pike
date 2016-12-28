#ifndef BUFFER_H
#define BUFFER_H

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

#include "bitvector.h"
#include "pike_memory.h"
#include "global.h"

#define MACRO   static inline PIKE_UNUSED_ATTRIBUTE

struct byte_buffer {
    size_t left;
    size_t length;
    void * restrict dst;
};

#define BUFFER_INIT() { 0, 0, NULL }

PMOD_EXPORT void buffer_free(struct byte_buffer *b);
PMOD_EXPORT void buffer_make_space(struct byte_buffer *b, size_t len);
PMOD_EXPORT int buffer_make_space_nothrow(struct byte_buffer *b, size_t len);

PIKE_WARN_UNUSED_RESULT_ATTRIBUTE
PMOD_EXPORT void* buffer_finish(struct byte_buffer *b);

PIKE_WARN_UNUSED_RESULT_ATTRIBUTE
PMOD_EXPORT struct pike_string *buffer_finish_pike_string(struct byte_buffer *b);

PIKE_WARN_UNUSED_RESULT_ATTRIBUTE
MACRO void* buffer_ptr(const struct byte_buffer *b) {
    return (char*)b->dst + b->left - b->length;
}

MALLOC_FUNCTION MACRO void *buffer_dst(const struct byte_buffer *b) {
    return b->dst;
}

PIKE_WARN_UNUSED_RESULT_ATTRIBUTE
MACRO size_t buffer_content_length(const struct byte_buffer *b) {
    return b->length - b->left;
}

PIKE_WARN_UNUSED_RESULT_ATTRIBUTE
MACRO size_t buffer_length(const struct byte_buffer *b) {
    return b->length;
}

PIKE_WARN_UNUSED_RESULT_ATTRIBUTE
MACRO size_t buffer_space(struct byte_buffer *b) {
    return b->left;
}

MACRO void buffer_init(struct byte_buffer *buf) {
    struct byte_buffer b = BUFFER_INIT();
    *buf = b;
}

MACRO void buffer_clear(struct byte_buffer *b) {
    if (b->length) {
        b->dst = buffer_ptr(b);
        b->left = b->length;
    }
}

#ifdef PIKE_DEBUG
PMOD_EXPORT void buffer_remove(struct byte_buffer *b, size_t len);
#else
MACRO void buffer_remove(struct byte_buffer *b, size_t len) {
    char *dst = buffer_dst(b);

    b->dst = dst - len;
    b->left += len;
}
#endif

#ifdef PIKE_DEBUG
PMOD_EXPORT void buffer_check_space(struct byte_buffer *b, size_t len);
#else
MACRO void buffer_check_space(struct byte_buffer *PIKE_UNUSED(b), size_t PIKE_UNUSED(len)) { }
#endif

MACRO void buffer_advance(struct byte_buffer *b, size_t len) {
    char *dst = buffer_dst(b);
    buffer_check_space(b, len);
    b->dst = dst + len;
    b->left -= len;
}

MACRO void* buffer_ensure_space(struct byte_buffer *b, size_t len) {
    if (UNLIKELY(len > b->left)) {
        buffer_make_space(b, len);

        STATIC_ASSUME(len <= b->left);
    }
    return buffer_dst(b);
}

MACRO int buffer_ensure_space_nothrow(struct byte_buffer *b, size_t len) {
    if (UNLIKELY(len > b->left)) {
        if (!buffer_make_space_nothrow(b, len)) return 0;

        STATIC_ASSUME(len <= b->left);
    }

    return 1;
}

#define ASSUME_UNCHANGED(b, CODE) do {          \
    const struct byte_buffer __B = *b;          \
    do CODE while(0);                           \
    STATIC_ASSUME(__B.dst == b->dst);           \
    STATIC_ASSUME(__B.length == b->length);     \
    STATIC_ASSUME(__B.left == b->left);         \
} while (0)

MACRO void buffer_memcpy_unsafe(struct byte_buffer *b, const void * src, size_t len) {
    unsigned INT8 * restrict dst = buffer_dst(b);
    const size_t left = b->left;
    buffer_check_space(b, len);
    ASSUME_UNCHANGED(b, {
        memcpy(dst, src, len);
    });
    b->dst = dst + len;
    b->left = left - len;
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
    b->left -= len;

    return dst;
}

MALLOC_FUNCTION
MACRO void* buffer_alloc(struct byte_buffer *b, size_t len) {
    buffer_ensure_space(b, len);
    return buffer_alloc_unsafe(b, len);
}

#define GEN_BUFFER_ADD(name, type, transform)                                   \
MACRO void buffer_add_ ## name ## _unsafe(struct byte_buffer *b, type a) {      \
    unsigned INT8 * dst = buffer_dst(b);                                        \
    const size_t left = b->left;                                                \
    buffer_check_space(b, sizeof(a));                                           \
    ASSUME_UNCHANGED(b, {                                                       \
        a = transform(a);                                                       \
        memcpy(dst, &a, sizeof(a));                                             \
    });                                                                         \
    b->dst = dst + sizeof(a);                                                   \
    b->left = left - sizeof(a);                                                 \
}                                                                               \
MACRO void buffer_add_ ## name (struct byte_buffer *b, type a) {                \
    buffer_ensure_space(b, sizeof(a));                                          \
    buffer_add_##name##_unsafe(b, a);                                           \
}

GEN_BUFFER_ADD(be16, unsigned INT16, hton16)
GEN_BUFFER_ADD(le16, unsigned INT16, ntoh16)
GEN_BUFFER_ADD(u16, unsigned INT16, )

GEN_BUFFER_ADD(be32, unsigned INT32, hton32)
GEN_BUFFER_ADD(le32, unsigned INT32, ntoh32)
GEN_BUFFER_ADD(u32, unsigned INT32, )

GEN_BUFFER_ADD(be64, unsigned INT64, hton64)
GEN_BUFFER_ADD(le64, unsigned INT64, ntoh64)
GEN_BUFFER_ADD(u64, unsigned INT64, )

GEN_BUFFER_ADD(u8, unsigned INT8, )
GEN_BUFFER_ADD(char, char, )

#undef GEN_BUFFER_ADD
#undef MACRO
#undef ASSUME_UNCHANGED

#define BUFFER_ADD_UNSAFE(b, a) do {            \
    struct byte_buffer *__b__ = (b);            \
    unsigned INT8 * dst = buffer_dst(__b__);    \
    const size_t left = __b__->left;            \
    buffer_check_space(__b__, sizeof(a));       \
    ASSUME_UNCHANGED(__b__, {                   \
        memcpy(dst, &a, sizeof(a));             \
    });                                         \
    __b__->dst = dst + sizeof(a);               \
    __b__->left = left - sizeof(a);             \
} while(0)
#define BUFFER_ADD(b, a) do {                   \
    buffer_ensure_space(b, sizeof(a));          \
    BUFFER_ADD_UNSAFE(b, a);                    \
} while(0)

#endif /* BUFFER_H */
