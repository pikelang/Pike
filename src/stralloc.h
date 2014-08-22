/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef STRALLOC_H
#define STRALLOC_H
#include "global.h"

#include "pike_macros.h"

#define STRINGS_ARE_SHARED

#ifndef STRUCT_PIKE_STRING_DECLARED
#define STRUCT_PIKE_STRING_DECLARED
#endif

enum size_shift {
    eightbit=0,
    sixteenbit=1,
    thirtytwobit=2,
};


struct pike_string
{
  INT32 refs;
  unsigned char flags;
  enum size_shift  size_shift:8;
  unsigned char  min;
  unsigned char  max;
  ptrdiff_t len; /* Not counting terminating NUL. */
  size_t hval;
  struct pike_string *next;
  char * str;			/* NUL terminated. */
};

struct string_builder
{
  struct pike_string *s;
  ptrdiff_t malloced;
  INT32 known_shift;
};

/* Flags used in pike_string->flags. */
#define STRING_NOT_HASHED	    1	/* Hash value is invalid. */
#define STRING_NOT_SHARED	    2	/* String not shared. */
#define STRING_IS_SHORT		    4	/* String is blockalloced. */
#define STRING_CLEAR_ON_EXIT    8   /* Overwrite before free. */

#define STRING_CONTENT_CHECKED 16 /* if true, min and max are valid */
#define STRING_IS_LOWERCASE    32
#define STRING_IS_UPPERCASE    64

#define CLEAR_STRING_CHECKED(X) do{(X)->flags &= 15;}while(0)

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

#ifdef PIKE_DEBUG
struct pike_string *debug_findstring(const struct pike_string *foo);
#endif

#define free_string(s) do{ \
    struct pike_string *_=(s); \
    debug_malloc_touch(_); \
    if(sub_ref(_)<=0) \
      really_free_string(_); \
  } while(0)

#define my_hash_string(X) PTR_TO_INT(X)
#define is_same_string(X,Y) ((X)==(Y))

#ifdef PIKE_DEBUG
#define STR0(X) ((p_wchar0 *)debug_check_size_shift((X),0)->str)
#define STR1(X) ((p_wchar1 *)debug_check_size_shift((X),1)->str)
#define STR2(X) ((p_wchar2 *)debug_check_size_shift((X),2)->str)
#else
#define STR0(X) ((p_wchar0 *)(X)->str)
#define STR1(X) ((p_wchar1 *)(X)->str)
#define STR2(X) ((p_wchar2 *)(X)->str)
#endif

#ifndef PIKE_DEBUG
static p_wchar2 generic_extract (const void *str, int size, ptrdiff_t pos) ATTRIBUTE((pure));

static INLINE p_wchar2 __attribute__((unused)) generic_extract (const void *str, int size, ptrdiff_t pos)
{
/* this gives better code than a lot of other versions I have tested.

When inlined the ret/eax is of course somewhat different, it can be
less or more optimal, but this is at least actually smaller than the
expanded code for the oldINDEX_CHARP.
*/
  if( LIKELY(size == 0) ) return ((p_wchar0 *)str)[pos];
  if( LIKELY(size == 1) ) return ((p_wchar1 *)str)[pos];
  return ((p_wchar2 *)str)[pos];
}

static INLINE p_wchar2 __attribute__((unused)) index_shared_string(const struct pike_string *s,  ptrdiff_t pos)
{
  return generic_extract(s->str,s->size_shift,pos);
}
#else
PMOD_EXPORT p_wchar2 generic_extract (const void *str, int size, ptrdiff_t pos);
PMOD_EXPORT p_wchar2 index_shared_string(const struct pike_string *s, ptrdiff_t pos);
#endif

#define INDEX_CHARP(PTR,IND,SHIFT) generic_extract(PTR,SHIFT,IND)

#define SET_INDEX_CHARP(PTR,IND,SHIFT,VAL) \
  (LIKELY((SHIFT)==0)?                                                  \
   (((p_wchar0 *)(PTR))[(IND)] = DO_NOT_WARN ((p_wchar0) (VAL))):	\
  LIKELY((SHIFT)==1)?(((p_wchar1 *)(PTR))[(IND)] = DO_NOT_WARN ((p_wchar1) (VAL))): \
   (((p_wchar2 *)(PTR))[(IND)] = DO_NOT_WARN ((p_wchar2) (VAL))))


/* arithmetic left shift. compilers will understand this
 * and generate one arithmetic left shift. Left shifting
 * a negative integer is undefined and could be optimized
 * away by compilers.
 */
#define SAL(a, b)	((a) < 0 ? -(-(a) << (b)) : (a) << (b))

#define EXTRACT_CHARP(PTR,SHIFT) INDEX_CHARP((PTR),0,(SHIFT))
#define CHARP_ADD(PTR,X,SHIFT) (PTR)+=SAL(X,SHIFT)

#define INDEX_PCHARP(X,Y) INDEX_CHARP((X).ptr,(Y),(X).shift)
#define SET_INDEX_PCHARP(X,Y,Z) SET_INDEX_CHARP((X).ptr,(Y),(X).shift,(Z))
#define EXTRACT_PCHARP(X) INDEX_CHARP((X).ptr,(0),(X).shift)
#define INC_PCHARP(X,Y) (((X).ptr)+= SAL(Y, (X).shift))

#define LOW_COMPARE_PCHARP(X,CMP,Y) (((char *)((X).ptr)) CMP ((char *)((Y).ptr)))
#define LOW_SUBTRACT_PCHARP(X,Y) (LOW_COMPARE_PCHARP((X),-,(Y))>>(X).shift)

#ifdef PIKE_DEBUG
#define SUBTRACT_PCHARP(X,Y)    ((X).shift!=(Y).shift?(Pike_fatal("Subtracting different size charp!\n")),0:LOW_SUBTRACT_PCHARP((X),(Y)))
#define COMPARE_PCHARP(X,CMP,Y) ((X).shift!=(Y).shift?(Pike_fatal("Comparing different size charp!\n")),0:LOW_COMPARE_PCHARP((X),CMP,(Y)))
#else
#define SUBTRACT_PCHARP(X,Y) LOW_SUBTRACT_PCHARP((X),(Y))
#define COMPARE_PCHARP(X,CMP,Y) LOW_COMPARE_PCHARP((X),CMP,(Y))
#endif

static INLINE PCHARP __attribute__((unused)) MKPCHARP(const void *ptr, int shift)
{
  PCHARP tmp;
  tmp.ptr=(p_wchar0 *)ptr;
  tmp.shift=shift;
  return tmp;
}

#define MKPCHARP_OFF(PTR,SHIFT,OFF) MKPCHARP( ((char *)(PTR)) + SAL(OFF, SHIFT), (SHIFT))
#define MKPCHARP_STR(STR) MKPCHARP((STR)->str, (STR)->size_shift)
#define MKPCHARP_STR_OFF(STR,OFF) \
 MKPCHARP((STR)->str + SAL((OFF),(STR)->size_shift), (STR)->size_shift)
#define ADD_PCHARP(PTR,I) MKPCHARP_OFF((PTR).ptr,(PTR).shift,(I))

#define reference_shared_string(s) add_ref(s)
#define copy_shared_string(to,s) add_ref((to)=(s))

#ifdef DO_PIKE_CLEANUP

struct shared_string_location
{
  struct pike_string *s;
  struct shared_string_location *next;
};

PMOD_EXPORT extern struct shared_string_location *all_shared_string_locations;

#define MAKE_CONST_STRING(var, text) do {		\
  static struct shared_string_location str_;		\
  if(!str_.s) { 					\
    str_.s=make_shared_binary_string((text),CONSTANT_STRLEN(text)); \
    str_.next=all_shared_string_locations;		\
    all_shared_string_locations=&str_;			\
  }							\
  var = str_.s;						\
}while(0)

#define MAKE_CONST_STRING_CODE(var, code) do {				\
    static struct shared_string_location str_;				\
    if (!str_.s) {							\
      {code;}								\
      str_.s = var;							\
      DO_IF_DEBUG (							\
	if (!str_.s)							\
	  Pike_fatal ("Code at " __FILE__ ":" DEFINETOSTR (__LINE__)	\
		      " failed to produce a string in " #var ".\n");	\
      );								\
      str_.next=all_shared_string_locations;				\
      all_shared_string_locations=&str_;				\
    }									\
    else var = str_.s;							\
  } while (0)

#else

#define MAKE_CONST_STRING(var, text)						\
 do { static struct pike_string *str_;                                          \
    if(!str_) str_=make_shared_binary_string((text),CONSTANT_STRLEN(text));     \
    var = str_;									\
 }while(0)

#define MAKE_CONST_STRING_CODE(var, code) do {				\
    static struct pike_string *str_;					\
    if (!str_) {							\
      {code;}								\
      str_ = var;							\
      DO_IF_DEBUG (							\
	if (!str_)							\
	  Pike_fatal ("Code at " __FILE__ ":" DEFINETOSTR (__LINE__)	\
		      " failed to produce a string in " #var ".\n");	\
      );								\
    }									\
    else var = str_;							\
  } while (0)

#endif

#define REF_MAKE_CONST_STRING(var, text) do {				\
    MAKE_CONST_STRING(var, text);					\
    reference_shared_string(var);					\
  } while (0)

#define REF_MAKE_CONST_STRING_CODE(var, code) do {			\
    MAKE_CONST_STRING_CODE(var, code);					\
    reference_shared_string(var);					\
  } while (0)

/* Compatibility. */
#define MAKE_CONSTANT_SHARED_STRING(var, text)				\
  REF_MAKE_CONST_STRING(var, text)

#define convert_0_to_0(X,Y,Z) MEMCPY((char *)(X),(char *)(Y),(Z))
#define convert_1_to_1(X,Y,Z) MEMCPY((char *)(X),(char *)(Y),(Z)<<1)
#define convert_2_to_2(X,Y,Z) MEMCPY((char *)(X),(char *)(Y),(Z)<<2)

#define compare_0_to_0(X,Y,Z) MEMCMP((char *)(X),(char *)(Y),(Z))
#define compare_1_to_1(X,Y,Z) MEMCMP((char *)(X),(char *)(Y),(Z)<<1)
#define compare_2_to_2(X,Y,Z) MEMCMP((char *)(X),(char *)(Y),(Z)<<2)

#define CONVERT(FROM,TO) \
void PIKE_CONCAT4(convert_,FROM,_to_,TO)(PIKE_CONCAT(p_wchar,TO) *to, const PIKE_CONCAT(p_wchar,FROM) *from, ptrdiff_t len); \
INT32 PIKE_CONCAT4(compare_,FROM,_to_,TO)(const PIKE_CONCAT(p_wchar,TO) *to, const PIKE_CONCAT(p_wchar,FROM) *from, ptrdiff_t len);

PMOD_EXPORT extern struct pike_string *empty_pike_string;

/* Prototypes begin here */
void low_set_index(struct pike_string *s, ptrdiff_t pos, int value);
PMOD_EXPORT struct pike_string *debug_check_size_shift(const struct pike_string *a,int shift);
CONVERT(0,1)
CONVERT(0,2)
CONVERT(1,0)
CONVERT(1,2)
CONVERT(2,0)
CONVERT(2,1)

#undef CONVERT

void generic_memcpy(PCHARP to,
                    const PCHARP from,
                    ptrdiff_t len);
PMOD_EXPORT void pike_string_cpy(PCHARP to, const struct pike_string *from);
struct pike_string *binary_findstring(const char *foo, ptrdiff_t l);
struct pike_string *binary_findstring_pcharp(PCHARP foo, ptrdiff_t l);
struct pike_string *findstring(const char *foo);

PMOD_EXPORT struct pike_string *debug_begin_shared_string(size_t len) ATTRIBUTE((malloc));
PMOD_EXPORT struct pike_string *debug_begin_wide_shared_string(size_t len, int shift)  ATTRIBUTE((malloc));
PMOD_EXPORT struct pike_string *low_end_shared_string(struct pike_string *s);
PMOD_EXPORT struct pike_string *end_shared_string(struct pike_string *s);
PMOD_EXPORT struct pike_string *end_and_resize_shared_string(struct pike_string *str, ptrdiff_t len) ;
PMOD_EXPORT struct pike_string * debug_make_shared_binary_string(const char *str,size_t len);
PMOD_EXPORT struct pike_string * debug_make_shared_binary_pcharp(const PCHARP str,size_t len);
PMOD_EXPORT struct pike_string * debug_make_shared_pcharp(const PCHARP str);
PMOD_EXPORT struct pike_string * debug_make_shared_binary_string0(const p_wchar0 *str,size_t len);
PMOD_EXPORT struct pike_string * debug_make_shared_binary_string1(const p_wchar1 *str,size_t len);
PMOD_EXPORT struct pike_string * debug_make_shared_binary_string2(const p_wchar2 *str,size_t len);
PMOD_EXPORT struct pike_string *debug_make_shared_string(const char *str);
PMOD_EXPORT struct pike_string *debug_make_shared_string0(const p_wchar0 *str);
PMOD_EXPORT struct pike_string *debug_make_shared_string1(const p_wchar1 *str);
PMOD_EXPORT struct pike_string *debug_make_shared_string2(const p_wchar2 *str);
PMOD_EXPORT void check_string_range( struct pike_string *str, int loose,
                                     INT32 *min, INT32 *max );
/* Returns true if str1 could contain str2. */
PMOD_EXPORT int string_range_contains_string( struct pike_string *str1,
                                              struct pike_string *str2 );
/* Returns true if str could contain n. */
PMOD_EXPORT int string_range_contains( struct pike_string *str, int n );

PMOD_EXPORT void do_free_string(struct pike_string *s);
PMOD_EXPORT void do_free_unlinked_pike_string(struct pike_string *s);
PMOD_EXPORT void really_free_string(struct pike_string *s);
PMOD_EXPORT void debug_free_string(struct pike_string *s);
struct pike_string *add_string_status(int verbose);
PMOD_EXPORT void check_string(struct pike_string *s);
PMOD_EXPORT void verify_shared_strings_tables(void);
int safe_debug_findstring(struct pike_string *foo);
struct pike_string *debug_findstring(const struct pike_string *foo);
PMOD_EXPORT void debug_dump_pike_string(struct pike_string *s, INT32 max);
void dump_stralloc_strings(void);
int low_quick_binary_strcmp(const char *a, ptrdiff_t alen,
                            const char *b, ptrdiff_t blen) ATTRIBUTE((pure));
ptrdiff_t generic_quick_binary_strcmp(const char *a,
                                      ptrdiff_t alen, int asize,
                                      const char *b,
                                      ptrdiff_t blen, int bsize) ATTRIBUTE((pure));
ptrdiff_t generic_find_binary_prefix(const char *a,
                                     ptrdiff_t alen, int asize,
                                     const char *b,
                                     ptrdiff_t blen, int bsize) ATTRIBUTE((pure));
PMOD_EXPORT int c_compare_string(struct pike_string *s, char *foo, int len) ATTRIBUTE((pure));
PMOD_EXPORT ptrdiff_t my_quick_strcmp(struct pike_string *a,
				      struct pike_string *b) ATTRIBUTE((pure));
PMOD_EXPORT ptrdiff_t my_strcmp(struct pike_string *a,struct pike_string *b) ATTRIBUTE((pure));
struct pike_string *realloc_unlinked_string(struct pike_string *a,
                                            ptrdiff_t size);
struct pike_string *new_realloc_shared_string(struct pike_string *a, INT32 size, int shift) ATTRIBUTE((malloc));
struct pike_string *modify_shared_string(struct pike_string *a,
                                         INT32 position,
                                         INT32 c);
PMOD_EXPORT struct pike_string *add_shared_strings(struct pike_string *a,
					 struct pike_string *b);
PMOD_EXPORT struct pike_string *add_and_free_shared_strings(struct pike_string *a,
						struct pike_string *b);
PMOD_EXPORT ptrdiff_t string_search(struct pike_string *haystack,
				    struct pike_string *needle,
				    ptrdiff_t start);
PMOD_EXPORT struct pike_string *string_slice(struct pike_string *s,
					     ptrdiff_t start,
					     ptrdiff_t len);
PMOD_EXPORT struct pike_string *string_replace(struct pike_string *str,
				   struct pike_string *del,
				   struct pike_string *to);
void init_shared_string_table(void);
void cleanup_shared_string_table(void);
void count_memory_in_strings(size_t *num, size_t *size);
PMOD_EXPORT void visit_string (struct pike_string *s, int action, void *extra);
void gc_mark_string_as_referenced (struct pike_string *s);
unsigned gc_touch_all_strings(void);
void gc_mark_all_strings(void);
struct pike_string *next_pike_string (struct pike_string *s);
PMOD_EXPORT void init_string_builder(struct string_builder *s, int mag);
PMOD_EXPORT void init_string_builder_alloc(struct string_builder *s, ptrdiff_t length, int mag);
PMOD_EXPORT void init_string_builder_copy(struct string_builder *to,
					  struct string_builder *from);
PMOD_EXPORT int init_string_builder_with_string (struct string_builder *s,
						 struct pike_string *str);
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
				       PCHARP from,
				       ptrdiff_t len);
PMOD_EXPORT void string_builder_fill(struct string_builder *s,
				     ptrdiff_t howmany,
				     PCHARP from,
				     ptrdiff_t len,
				     ptrdiff_t offset);
PMOD_EXPORT void string_builder_utf16_strcat(struct string_builder *s,
					     const p_wchar1 *utf16str);
PMOD_EXPORT void string_builder_strcat(struct string_builder *s, char *str);
PMOD_EXPORT void string_builder_shared_strcat(struct string_builder *s, struct pike_string *str);
PMOD_EXPORT ptrdiff_t string_builder_quote_string(struct string_builder *buf,
						  struct pike_string *str,
						  ptrdiff_t start,
						  ptrdiff_t max_len,
						  int flags);
PMOD_EXPORT void update_flags_for_add( struct pike_string *a, struct pike_string *b);
PMOD_EXPORT void set_flags_for_add( struct pike_string *ret,
                                    unsigned char aflags, unsigned char amin,
                                    unsigned char amax,
                                    struct pike_string *b);
PMOD_EXPORT void string_builder_append_integer(struct string_builder *s,
					       LONGEST val,
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
PMOD_EXPORT PCHARP MEMCHR_PCHARP(PCHARP ptr, int chr, ptrdiff_t len);
PMOD_EXPORT long STRTOL_PCHARP(PCHARP str, PCHARP *ptr, int base);
PMOD_EXPORT int string_to_svalue_inumber(struct svalue *r,
			     char * str,
			     char **ptr,
			     int base,
			     int maxlength);
int wide_string_to_svalue_inumber(struct svalue *r,
					      void * str,
					      void *ptr,
					      int base,
					      ptrdiff_t maxlength,
                          int shift);
int safe_wide_string_to_svalue_inumber(struct svalue *r,
				       void * str,
				       void *ptr,
				       int base,
				       ptrdiff_t maxlength,
				       int shift);
PMOD_EXPORT int pcharp_to_svalue_inumber(struct svalue *r,
					 PCHARP str,
					 PCHARP *ptr,
					 int base,
					 ptrdiff_t maxlength);
PMOD_EXPORT int convert_stack_top_string_to_inumber(int base);
PMOD_EXPORT double STRTOD_PCHARP(PCHARP nptr, PCHARP *endptr);
PMOD_EXPORT p_wchar0 *require_wstring0(struct pike_string *s,
               char **to_free);
PMOD_EXPORT p_wchar1 *require_wstring1(struct pike_string *s,
			   char **to_free);
PMOD_EXPORT p_wchar2 *require_wstring2(struct pike_string *s,
			   char **to_free);
/* Prototypes end here */

/* Compat alias. */
#define do_really_free_pike_string do_free_unlinked_pike_string

static INLINE void __attribute__((unused)) string_builder_binary_strcat(struct string_builder *s,
						const char *str, ptrdiff_t len)
{
  string_builder_binary_strcat0 (s, (const p_wchar0 *) str, len);
}

/* Note: Does not work 100% correctly with shift==2 strings. */
static INLINE int __attribute__((unused)) string_has_null( struct pike_string *x )
{
    INT32 min;
    if( !x->len ) return 0;
    check_string_range(x,0,&min,0);
    return min <= 0;
}

#define ISCONSTSTR(X,Y) c_compare_string((X),Y,sizeof(Y)-sizeof(""))

#define visit_string_ref(S, REF_TYPE, EXTRA)			\
  visit_ref (pass_string (S), (REF_TYPE),			\
	     (visit_thing_fn *) &visit_string, (EXTRA))

#ifdef DEBUG_MALLOC
#define make_shared_string(X) \
 ((struct pike_string *)debug_malloc_pass(debug_make_shared_string(X)))
#define make_shared_binary_string(X,Y) \
 ((struct pike_string *)debug_malloc_pass(debug_make_shared_binary_string((X),(Y))))

#define make_shared_string0(X) \
 ((struct pike_string *)debug_malloc_pass(debug_make_shared_string0(X)))
#define make_shared_binary_string0(X,Y) \
 ((struct pike_string *)debug_malloc_pass(debug_make_shared_binary_string0((X),(Y))))

#define make_shared_string1(X) \
 ((struct pike_string *)debug_malloc_pass(debug_make_shared_string1(X)))
#define make_shared_binary_string1(X,Y) \
 ((struct pike_string *)debug_malloc_pass(debug_make_shared_binary_string1((X),(Y))))

#define make_shared_string2(X) \
 ((struct pike_string *)debug_malloc_pass(debug_make_shared_string2(X)))
#define make_shared_binary_string2(X,Y) \
 ((struct pike_string *)debug_malloc_pass(debug_make_shared_binary_string2((X),(Y))))

#define begin_shared_string(X) \
 ((struct pike_string *)debug_malloc_pass(debug_begin_shared_string(X)))
#define begin_wide_shared_string(X,Y) \
 ((struct pike_string *)debug_malloc_pass(debug_begin_wide_shared_string((X),(Y))))

#define make_shared_pcharp(X) \
 ((struct pike_string *)debug_malloc_pass(debug_make_shared_pcharp(X)))
#define make_shared_binary_pcharp(X,Y) \
 ((struct pike_string *)debug_malloc_pass(debug_make_shared_binary_pcharp((X),(Y))))

#else
#define make_shared_string debug_make_shared_string
#define make_shared_binary_string debug_make_shared_binary_string

#define make_shared_string0 debug_make_shared_string0
#define make_shared_binary_string0 debug_make_shared_binary_string0

#define make_shared_string1 debug_make_shared_string1
#define make_shared_binary_string1 debug_make_shared_binary_string1

#define make_shared_string2 debug_make_shared_string2
#define make_shared_binary_string2 debug_make_shared_binary_string2

#define begin_shared_string debug_begin_shared_string
#define begin_wide_shared_string debug_begin_wide_shared_string

#define make_shared_pcharp debug_make_shared_pcharp
#define make_shared_binary_pcharp debug_make_shared_binary_pcharp

#endif

PMOD_EXPORT void f_sprintf(INT32 num_arg);
void f___handle_sprintf_format(INT32 args);
void low_f_sprintf(INT32 args, int compat_mode, struct string_builder *r);
void init_sprintf(void);
void exit_sprintf(void);

#endif /* STRALLOC_H */
