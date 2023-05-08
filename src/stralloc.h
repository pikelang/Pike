/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef STRALLOC_H
#define STRALLOC_H
#include "global.h"

#include "pike_macros.h"
#include "gc_header.h"

#define STRINGS_ARE_SHARED

enum string_type {
    STRING_ALLOC_STATIC   =0,
    STRING_ALLOC_MALLOC   =1,
    STRING_ALLOC_BA       =2,
    STRING_ALLOC_SUBSTRING=3,
};


enum struct_type {
    STRING_STRUCT_STRING    =0,
    STRING_STRUCT_SUBSTRING =1,
};

struct pike_string
{
#ifdef PIKE_DEBUG
  GC_MARKER_MEMBERS;
#else
  INT32 refs;
#endif
  unsigned char flags;
#ifdef __GCC__
  enum size_shift   size_shift:2;
  enum string_type  alloc_type:5;
  enum struct_type struct_type:1;
#else /* !__GCC__ */
  /* NB: Some compliers (eg MSVC) use signed integers for the
   *     enum bit fields, causing thirtytwobit (2) to be -2
   *     when extracted from the size_shift field.
   */
  unsigned char size_shift:2;
  unsigned char alloc_type:5;
  unsigned char struct_type:1;
#endif /* __GCC__ */
  unsigned char  min;
  unsigned char  max;
  ptrdiff_t len; /* Not counting terminating NUL. */
  size_t hval;
  struct pike_string *next;
  char * str;			/* NUL terminated. */
};

struct substring_pike_string {
  struct pike_string str;
  struct pike_string *parent;
};

/* Flags used in pike_string->flags. */
#define STRING_NOT_HASHED	    1	/* Hash value is invalid. */
#define STRING_NOT_SHARED	    2	/* String not shared. */
#define STRING_CLEAR_ON_EXIT        4   /* Overwrite before free. */

#define STRING_CONTENT_CHECKED      8 /* if true, min and max are valid */
#define STRING_IS_LOWERCASE        16
#define STRING_IS_UPPERCASE        32

#define STRING_IS_LOCKED	   64	/* The str field MUST NOT be reallocated. */
#define STRING_CONVERT_SURROGATES 128	/* Convert surrogates when done. */

#define STRING_CHECKED_MASK (STRING_IS_UPPERCASE|STRING_IS_LOWERCASE|STRING_CONTENT_CHECKED)

#define CLEAR_STRING_CHECKED(X) do{(X)->flags &= ~STRING_CHECKED_MASK;}while(0)

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

PMOD_EXPORT extern const char Pike_isidchar_vector[];
#define isidchar(X)	(Pike_isidchar_vector[((unsigned)(X))&0xff] == '1')

#ifndef PIKE_DEBUG
static p_wchar2 generic_extract (const void *str, enum size_shift size,
				 ptrdiff_t pos) ATTRIBUTE((pure));

static inline p_wchar2 PIKE_UNUSED_ATTRIBUTE generic_extract (const void *str,
							      enum size_shift size,
							      ptrdiff_t pos)
{
/* this gives better code than a lot of other versions I have tested.

When inlined the ret/eax is of course somewhat different, it can be
less or more optimal, but this is at least actually smaller than the
expanded code for the old INDEX_CHARP.
*/
  if( LIKELY(size == 0) ) return ((p_wchar0 *)str)[pos];
  if( LIKELY(size == 1) ) return ((p_wchar1 *)str)[pos];
  return ((p_wchar2 *)str)[pos];
}

static inline p_wchar2 PIKE_UNUSED_ATTRIBUTE index_shared_string(const struct pike_string *s,  ptrdiff_t pos)
{
  return generic_extract(s->str,s->size_shift,pos);
}
#else
PMOD_EXPORT p_wchar2 generic_extract (const void *str, enum size_shift size,
				      ptrdiff_t pos);
PMOD_EXPORT p_wchar2 index_shared_string(const struct pike_string *s, ptrdiff_t pos);
#endif

#define INDEX_CHARP(PTR,IND,SHIFT) generic_extract(PTR,SHIFT,IND)

#define SET_INDEX_CHARP(PTR,IND,SHIFT,VAL) \
  (LIKELY((SHIFT)==0)?                                                  \
   (((p_wchar0 *)(PTR))[(IND)] = (p_wchar0) (VAL)):	\
  LIKELY((SHIFT)==1)?(((p_wchar1 *)(PTR))[(IND)] = (p_wchar1) (VAL)): \
   (((p_wchar2 *)(PTR))[(IND)] = (p_wchar2) (VAL)))


/* arithmetic left shift. compilers will understand this
 * and generate one arithmetic left shift. Left shifting
 * a negative integer is implementation defined and
 * not all compilers do arithmetic left shifts.
 */
static inline ptrdiff_t PIKE_UNUSED_ATTRIBUTE SAL(ptrdiff_t a, unsigned int b) {
    if (a < 0) {
        return -(-a << b);
    } else {
        return a << b;
    }
}

#define EXTRACT_CHARP(PTR,SHIFT) INDEX_CHARP((PTR),0,(SHIFT))
#define CHARP_ADD(PTR,X,SHIFT) (PTR)+=SAL(X,SHIFT)

#define INDEX_PCHARP(X,Y) INDEX_CHARP((X).ptr,(Y),(X).shift)
#define SET_INDEX_PCHARP(X,Y,Z) SET_INDEX_CHARP((X).ptr,(Y),(X).shift,(Z))
#define EXTRACT_PCHARP(X) INDEX_CHARP((X).ptr,(0),(X).shift)
#define INC_PCHARP(X,Y) (((X).ptr) = ((char*)((X).ptr))+SAL(Y, (X).shift))

#define LOW_COMPARE_PCHARP(X,CMP,Y) (((char *)((X).ptr)) CMP ((char *)((Y).ptr)))
#define LOW_SUBTRACT_PCHARP(X,Y) (LOW_COMPARE_PCHARP((X),-,(Y))>>(X).shift)

#ifdef PIKE_DEBUG
#define SUBTRACT_PCHARP(X,Y)    ((X).shift!=(Y).shift?(Pike_fatal("Subtracting different size charp!\n")),0:LOW_SUBTRACT_PCHARP((X),(Y)))
#define COMPARE_PCHARP(X,CMP,Y) ((X).shift!=(Y).shift?(Pike_fatal("Comparing different size charp!\n")),0:LOW_COMPARE_PCHARP((X),CMP,(Y)))
#else
#define SUBTRACT_PCHARP(X,Y) LOW_SUBTRACT_PCHARP((X),(Y))
#define COMPARE_PCHARP(X,CMP,Y) LOW_COMPARE_PCHARP((X),CMP,(Y))
#endif

static inline PCHARP PIKE_UNUSED_ATTRIBUTE MKPCHARP(const void *ptr, enum size_shift shift)
{
  PCHARP tmp;
  tmp.ptr=(void*)ptr;
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
    str_.s=make_shared_static_string((text),CONSTANT_STRLEN(text), eightbit); \
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
    if(!str_) str_=make_shared_static_string((text),CONSTANT_STRLEN(text), eightbit);     \
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

#define convert_0_to_0(X,Y,Z) memcpy((X),(Y),(Z))
#define convert_1_to_1(X,Y,Z) memcpy((X),(Y),(Z)<<1)
#define convert_2_to_2(X,Y,Z) memcpy((X),(Y),(Z)<<2)

#define CONVERT(FROM,TO) \
void PIKE_CONCAT4(convert_,FROM,_to_,TO)(PIKE_CONCAT(p_wchar,TO) *to, const PIKE_CONCAT(p_wchar,FROM) *from, ptrdiff_t len);

CONVERT(0,1)
CONVERT(0,2)
CONVERT(1,0)
CONVERT(1,2)
CONVERT(2,0)
CONVERT(2,1)

#undef CONVERT

PMOD_EXPORT extern struct pike_string *empty_pike_string;

/* Prototypes begin here */
void low_set_index(struct pike_string *s, ptrdiff_t pos, int value);
#ifdef PIKE_DEBUG
PMOD_EXPORT struct pike_string *debug_check_size_shift(const struct pike_string *a,enum size_shift shift);
const struct pike_string *debug_findstring(const struct pike_string *s);
int safe_debug_findstring(const struct pike_string *foo);
#endif

void generic_memcpy(PCHARP to,
                    const PCHARP from,
                    ptrdiff_t len);
PMOD_EXPORT void pike_string_cpy(PCHARP to, const struct pike_string *from);
struct pike_string *binary_findstring(const char *str, ptrdiff_t len);
struct pike_string *findstring(const char *foo);

PMOD_EXPORT struct pike_string *debug_begin_shared_string(size_t len) ATTRIBUTE((malloc));
PMOD_EXPORT struct pike_string *debug_begin_wide_shared_string(size_t len, enum size_shift shift)  ATTRIBUTE((malloc));
PMOD_EXPORT struct pike_string *low_end_shared_string(struct pike_string *s);
PMOD_EXPORT struct pike_string *end_shared_string(struct pike_string *s);
PMOD_EXPORT struct pike_string *end_and_resize_shared_string(struct pike_string *str, ptrdiff_t len) ;
PMOD_EXPORT struct pike_string * debug_make_shared_binary_string(const char *str,size_t len);
PMOD_EXPORT struct pike_string * debug_make_shared_binary_pcharp(const PCHARP str,size_t len);
PMOD_EXPORT struct pike_string * debug_make_shared_pcharp(const PCHARP str);
PMOD_EXPORT struct pike_string * debug_make_shared_binary_string0(const p_wchar0 *str,size_t len);
PMOD_EXPORT struct pike_string * debug_make_shared_binary_string1(const p_wchar1 *str,size_t len);
PMOD_EXPORT struct pike_string * debug_make_shared_binary_string2(const p_wchar2 *str,size_t len);
PMOD_EXPORT struct pike_string * make_shared_static_string(const char *str, size_t len, enum size_shift);
PMOD_EXPORT struct pike_string * make_shared_malloc_string(char *str, size_t len, enum size_shift);
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

void unlink_pike_string(struct pike_string *s);
PMOD_EXPORT void do_free_string(struct pike_string *s);
PMOD_EXPORT void do_free_unlinked_pike_string(struct pike_string *s);
PMOD_EXPORT void really_free_string(struct pike_string *s);
PMOD_EXPORT void debug_free_string(struct pike_string *s);
struct pike_string *add_string_status(int verbose);
PMOD_EXPORT void check_string(struct pike_string *s);
PMOD_EXPORT void verify_shared_strings_tables(void);

PMOD_EXPORT void debug_dump_pike_string(const struct pike_string *s, INT32 max);
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
PMOD_EXPORT int c_compare_string(const struct pike_string *s,
                                 const char *foo, int len) ATTRIBUTE((pure));
PMOD_EXPORT ptrdiff_t my_quick_strcmp(const struct pike_string *a,
				      const struct pike_string *b) ATTRIBUTE((pure));
struct pike_string *realloc_unlinked_string(struct pike_string *a,
                                            ptrdiff_t size);
struct pike_string *new_realloc_shared_string(struct pike_string *a, INT32 size, enum size_shift shift) ATTRIBUTE((malloc));
struct pike_string *modify_shared_string(struct pike_string *a,
                                         INT32 position,
                                         INT32 c);
#ifdef __NT__
PMOD_EXPORT p_wchar1 *pike_string_to_utf16(struct pike_string *s,
                                           unsigned int flags);
#endif
void update_flags_for_add( struct pike_string *a, const struct pike_string *b);
PMOD_EXPORT struct pike_string *add_shared_strings(const struct pike_string *a,
                                                   const struct pike_string *b);
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
void count_string_types();
void count_memory_in_strings(size_t *num, size_t *size);
size_t count_memory_in_string(const struct pike_string *s);
PMOD_EXPORT void visit_string (struct pike_string *s, int action, void *extra);
void gc_mark_string_as_referenced (struct pike_string *s);
unsigned gc_touch_all_strings(void);
void gc_mark_all_strings(void);
PMOD_EXPORT struct pike_string *first_pike_string ();
PMOD_EXPORT struct pike_string *next_pike_string (const struct pike_string *s);
PMOD_EXPORT PCHARP MEMCHR_PCHARP(const PCHARP ptr, int chr, ptrdiff_t len);
PMOD_EXPORT long STRTOL_PCHARP(PCHARP str, PCHARP *ptr, int base);
int wide_string_to_svalue_inumber(struct svalue *r,
					      void * str,
					      void *ptr,
					      int base,
					      ptrdiff_t maxlength,
                          enum size_shift shift);
int safe_wide_string_to_svalue_inumber(struct svalue *r,
				       void * str,
				       void *ptr,
				       int base,
				       ptrdiff_t maxlength,
				       enum size_shift shift);
PMOD_EXPORT int pcharp_to_svalue_inumber(struct svalue *r,
					 PCHARP str,
					 PCHARP *ptr,
					 int base,
					 ptrdiff_t maxlength);
PMOD_EXPORT int convert_stack_top_string_to_inumber(int base);
PMOD_EXPORT double STRTOD_PCHARP(const PCHARP nptr, PCHARP *endptr);
#if SIZEOF_FLOAT_TYPE > SIZEOF_DOUBLE
PMOD_EXPORT long double STRTOLD_PCHARP(const PCHARP nptr, PCHARP *endptr);
#endif
PMOD_EXPORT p_wchar0 *require_wstring0(const struct pike_string *s,
                                       char **to_free);
PMOD_EXPORT p_wchar1 *require_wstring1(const struct pike_string *s,
                                       char **to_free);
PMOD_EXPORT p_wchar2 *require_wstring2(const struct pike_string *s,
                                       char **to_free);
PMOD_EXPORT int wide_isspace(int c);
PMOD_EXPORT int wide_isidchar(int c);
/* Prototypes end here */

/* Note: Does not work 100% correctly with shift==2 strings. */
static inline int PIKE_UNUSED_ATTRIBUTE string_has_null( struct pike_string *x )
{
    INT32 min;
    if( !x->len ) return 0;
    check_string_range(x,0,&min,0);
    return min <= 0;
}

static inline int PIKE_UNUSED_ATTRIBUTE string_is_block_allocated(const struct pike_string * s) {
  return (s->alloc_type == STRING_ALLOC_BA);
}

static inline int PIKE_UNUSED_ATTRIBUTE string_is_malloced(const struct pike_string * s) {
 return (s->alloc_type == STRING_ALLOC_MALLOC);
}

static inline int PIKE_UNUSED_ATTRIBUTE string_is_static(const struct pike_string * s) {
    return s->alloc_type == STRING_ALLOC_STATIC;
}

static inline int PIKE_UNUSED_ATTRIBUTE string_is_substring(const struct pike_string * s) {
    return s->alloc_type == STRING_ALLOC_SUBSTRING;
}

static struct pike_string PIKE_UNUSED_ATTRIBUTE *substring_content_string(const struct pike_string *s)
{
  return ((struct substring_pike_string*)s)->parent;
}

static inline int PIKE_UNUSED_ATTRIBUTE string_may_modify(const struct pike_string * s)
{
    return !string_is_static(s) && !string_is_substring(s)
      && s->refs == 1;
}

static inline int PIKE_UNUSED_ATTRIBUTE string_may_modify_len(const struct pike_string * s)
{
    return s->refs == 1;
}

static inline int PIKE_UNUSED_ATTRIBUTE find_magnitude1(const p_wchar1 *s, ptrdiff_t len)
{
  const p_wchar1 *e=s+len;
  while(s<e)
    if(*s++>=256)
      return 1;
  return 0;
}

static inline int PIKE_UNUSED_ATTRIBUTE find_magnitude2(const p_wchar2 *s, ptrdiff_t len)
{
  const p_wchar2 *e=s+len;
  while(s<e)
  {
    if((unsigned INT32)*s>=256)
    {
      do
      {
	if((unsigned INT32)*s++>=65536)
	  return 2;
      }while(s<e);
      return 1;
    }
    s++;
  }
  return 0;
}

static inline enum size_shift PIKE_UNUSED_ATTRIBUTE min_magnitude(const unsigned c)
{
  return LIKELY(c<256) ? 0 : LIKELY(c<65536) ? 1 : 2;
}

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
#define make_shared_string(s) (STATIC_IS_CONSTANT(s)                                     \
                               ? make_shared_static_string(s, strlen(s), eightbit)       \
                               : debug_make_shared_string(s))

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

/* Warning, these run 'C' more than once */
/* FIXME: Is it that great that every wide char is considered an
 * identifier char? Doesn't strike me as very unicode compliant.
 * isalnum, isdigit and islower also look seriously borken. /mast */
#define WIDE_ISSPACE(C)	wide_isspace(C)
#define WIDE_ISIDCHAR(C) wide_isidchar(C)
#define WIDE_ISALNUM(C)	(((C) < 256)?isidchar(C):0)
#define WIDE_ISDIGIT(C)	((C)>='0' && (C)<='9')

#endif /* STRALLOC_H */
