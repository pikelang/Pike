/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H
#include "global.h"

#include "stralloc.h"

struct string_builder
{
  struct pike_string *s;
  ptrdiff_t malloced;
  enum size_shift known_shift;
};

/* Flags used by string_builder_append_integer() */
#define APPEND_SIGNED		1	/* Value is signed */
/* Note: The following are NOT true flags. */
#define APPEND_WIDTH_HALF	2	/* h-flag. */
#define APPEND_WIDTH_LONG	4	/* l-flag. */
#define APPEND_WIDTH_LONG_LONG	6	/* ll-flag. */
#define APPEND_WIDTH_MASK	6	/* Corresponding mask. */
/* More real flags here. */
#define APPEND_POSITIVE		8	/* Sign positive values too. */
#define APPEND_UPPER_CASE	16	/* Use upper case hex. */
#define APPEND_ZERO_PAD		32	/* Zero pad. */
#define APPEND_LEFT		64	/* Left align. */

#if SIZEOF_CHAR_P == SIZEOF_INT
#define APPEND_WIDTH_PTR	0
#elif SIZEOF_CHAR_P == SIZEOF_LONG
#define APPEND_WIDTH_PTR	APPEND_WIDTH_LONG
#elif SIZEOF_CHAR_P == SIZEOF_LONG_LONG
#define APPEND_WIDTH_PTR	APPEND_WIDTH_LONG_LONG
#else
#error "Unknown way to read pointer-wide integers."
#endif

/* Flags used by string_builder_quote_string() */
#define QUOTE_NO_STRING_CONCAT	1	/* Don't use string concat in output */
#define QUOTE_BREAK_AT_LF	2	/* Break after linefeed */
#define QUOTE_NORMALIZE_WS	4	/* Normalize whitespace */
#define QUOTE_TOKENIZE		8	/* Recognize strings */
#define QUOTE_BREAK_AT_SQUOTE	16	/* Break after single quote */
#define QUOTE_BREAK_AT_DQUOTE	32	/* Break after double quote */


/* Prototypes begin here */
PMOD_EXPORT void init_string_builder(struct string_builder *s, int mag);
PMOD_EXPORT void init_string_builder_alloc(struct string_builder *s, ptrdiff_t length, int mag);
PMOD_EXPORT void init_string_builder_copy(struct string_builder *to,
					  const struct string_builder *from);
PMOD_EXPORT void string_build_mkspace(struct string_builder *s,
				      ptrdiff_t chars, int mag);
PMOD_EXPORT void *string_builder_allocate(struct string_builder *s, ptrdiff_t chars, int mag);
PMOD_EXPORT void string_builder_putchar(struct string_builder *s, int ch);
PMOD_EXPORT void string_builder_putchars(struct string_builder *s, int ch,
					 ptrdiff_t count);
PMOD_EXPORT void string_builder_binary_strcat0(struct string_builder *s,
					       const p_wchar0 *str, ptrdiff_t len);
PMOD_EXPORT void string_builder_binary_strcat1(struct string_builder *s,
					       const p_wchar1 *str, ptrdiff_t len);
PMOD_EXPORT void string_builder_binary_strcat2(struct string_builder *s,
					       const p_wchar2 *str, ptrdiff_t len);
PMOD_EXPORT void string_builder_append(struct string_builder *s,
				       const PCHARP from,
				       ptrdiff_t len);
PMOD_EXPORT void string_builder_fill(struct string_builder *s,
				     ptrdiff_t howmany,
				     const PCHARP from,
				     ptrdiff_t len,
				     ptrdiff_t offset);
PMOD_EXPORT void string_builder_utf16_strcat(struct string_builder *s,
					     const p_wchar1 *utf16str);
PMOD_EXPORT void string_builder_strcat(struct string_builder *s, const char *str);
PMOD_EXPORT void string_builder_shared_strcat(struct string_builder *s,
                                              const struct pike_string *str);
PMOD_EXPORT ptrdiff_t string_builder_quote_string(struct string_builder *buf,
						  const struct pike_string *str,
						  ptrdiff_t start,
						  ptrdiff_t max_len,
						  int flags);
PMOD_EXPORT void string_builder_append_integer(struct string_builder *s,
                                               INT64 val,
					       unsigned int base,
					       int flags,
					       size_t min_width,
					       size_t precision);
PMOD_EXPORT void string_builder_vsprintf(struct string_builder *s,
					 const char *fmt,
					 va_list args);
PMOD_EXPORT void string_builder_sprintf(struct string_builder *s,
					const char *fmt, ...);
PMOD_EXPORT void reset_string_builder(struct string_builder *s);
PMOD_EXPORT void free_string_builder(struct string_builder *s);
PMOD_EXPORT struct pike_string *finish_string_builder(struct string_builder *s);
void init_string_buffer(void);
void exit_string_buffer(void);
/* Prototypes end here */

static inline void PIKE_UNUSED_ATTRIBUTE string_builder_binary_strcat(struct string_builder *s,
						const char *str, ptrdiff_t len)
{
  string_builder_binary_strcat0 (s, (const p_wchar0 *) str, len);
}

#endif /* STRING_BUILDER_H */
