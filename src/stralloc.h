/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: stralloc.h,v 1.38 1999/10/31 22:01:18 grubba Exp $
 */
#ifndef STRALLOC_H
#define STRALLOC_H
#include "global.h"
#include "pike_macros.h"

#define STRINGS_ARE_SHARED

#ifndef STRUCT_PIKE_STRING_DECLARED
#define STRUCT_PIKE_STRING_DECLARED
#endif
struct pike_string
{
  INT32 refs;
  INT32 len;
  unsigned INT32 hval;
  struct pike_string *next; 
  int size_shift; /* 30 bit waste, but good for alignment... */
  char str[1];
};

struct string_builder
{
  struct pike_string *s;
  int malloced;
  int known_shift;
};

#ifdef PIKE_DEBUG
struct pike_string *debug_findstring(const struct pike_string *foo);
#endif

#define free_string(s) do{ struct pike_string *_=(s); debug_malloc_touch(_); if(--_->refs<=0) really_free_string(_); }while(0)

#define my_hash_string(X) ((unsigned long)(X))
#define my_order_strcmp(X,Y) ((char *)(X)-(char *)(Y))
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

#define INDEX_CHARP(PTR,IND,SHIFT) \
  ((SHIFT)==0?((p_wchar0 *)(PTR))[(IND)]:(SHIFT)==1?((p_wchar1 *)(PTR))[(IND)]:((p_wchar2 *)(PTR))[(IND)])

#define SET_INDEX_CHARP(PTR,IND,SHIFT,VAL) \
  ((SHIFT)==0?(((p_wchar0 *)(PTR))[(IND)]=(VAL)):(SHIFT)==1?(((p_wchar1 *)(PTR))[(IND)]=(VAL)):(((p_wchar2 *)(PTR))[(IND)]=(VAL)))


#define EXTRACT_CHARP(PTR,SHIFT) INDEX_CHARP((PTR),0,(SHIFT))
#define CHARP_ADD(PTR,X,SHIFT) (PTR)+=(X)<<(SHIFT)

#define INDEX_PCHARP(X,Y) INDEX_CHARP((X).ptr,(Y),(X).shift)
#define SET_INDEX_PCHARP(X,Y,Z) SET_INDEX_CHARP((X).ptr,(Y),(X).shift,(Z))
#define EXTRACT_PCHARP(X) INDEX_CHARP((X).ptr,(0),(X).shift)
#define INC_PCHARP(X,Y) (((X).ptr)+=(Y) << (X).shift)

#define LOW_COMPARE_PCHARP(X,CMP,Y) (((char *)((X).ptr)) CMP ((char *)((Y).ptr)))
#define LOW_SUBTRACT_PCHARP(X,Y) (LOW_COMPARE_PCHARP((X),-,(Y))>>(X).shift)

#ifdef PIKE_DEBUG
#define SUBTRACT_PCHARP(X,Y)    ((X).shift!=(Y).shift?(fatal("Subtracting different size charp!\n")),0:LOW_SUBTRACT_PCHARP((X),(Y)))
#define COMPARE_PCHARP(X,CMP,Y) ((X).shift!=(Y).shift?(fatal("Subtracting different size charp!\n")),0:LOW_COMPARE_PCHARP((X),CMP,(Y)))
#else
#define SUBTRACT_PCHARP(X,Y) LOW_SUBTRACT_PCHARP((X),(Y))
#define COMPARE_PCHARP(X,CMP,Y) LOW_COMPARE_PCHARP((X),CMP,(Y))
#endif



static INLINE PCHARP MKPCHARP(void *ptr, int shift)
{
  PCHARP tmp;
  tmp.ptr=ptr;
  tmp.shift=shift;
  return tmp;
}

#define MKPCHARP_OFF(PTR,SHIFT,OFF) MKPCHARP( ((char *)(PTR)) + ((OFF)<<(SHIFT)), (SHIFT))
#define MKPCHARP_STR(STR) MKPCHARP((STR)->str, (STR)->size_shift)
#define MKPCHARP_STR_OFF(STR,OFF) \
 MKPCHARP((STR)->str + ((OFF)<<(STR)->size_shift), (STR)->size_shift)
#define ADD_PCHARP(PTR,I) MKPCHARP_OFF((PTR).ptr,(PTR).shift,(I))


#ifdef DEBUG_MALLOC
#define reference_shared_string(s) do { struct pike_string *S_=(s); debug_malloc_touch(S_); S_->refs++; }while(0)
#define copy_shared_string(to,s) do { struct pike_string *S_=(to)=(s); debug_malloc_touch(S_); S_->refs++; }while(0)

struct shared_string_location
{
  struct pike_string *s;
  struct shared_string_location *next;
};

extern struct shared_string_location *all_shared_string_locations;

#define MAKE_CONSTANT_SHARED_STRING(var, text) do {	\
  static struct shared_string_location str_;		\
  if(!str_.s) { 					\
    str_.s=make_shared_binary_string((text),CONSTANT_STRLEN(text));			\
    str_.next=all_shared_string_locations;		\
    all_shared_string_locations=&str_;			\
  }							\
 copy_shared_string((var),str_.s);			\
}while(0)


#else

#define reference_shared_string(s) (s)->refs++
#define copy_shared_string(to,s) ((to)=(s))->refs++

#define MAKE_CONSTANT_SHARED_STRING(var, text)	\
 do { static struct pike_string *str_;		\
    if(!str_) str_=make_shared_string((text));	\
    copy_shared_string((var), str_);		\
 }while(0)

#endif

#define convert_0_to_0(X,Y,Z) MEMCPY((char *)(X),(char *)(Y),(Z))
#define convert_1_to_1(X,Y,Z) MEMCPY((char *)(X),(char *)(Y),(Z)<<1)
#define convert_2_to_2(X,Y,Z) MEMCPY((char *)(X),(char *)(Y),(Z)<<2)

#define compare_0_to_0(X,Y,Z) MEMCMP((char *)(X),(char *)(Y),(Z))
#define compare_1_to_1(X,Y,Z) MEMCMP((char *)(X),(char *)(Y),(Z)<<1)
#define compare_2_to_2(X,Y,Z) MEMCMP((char *)(X),(char *)(Y),(Z)<<2)

#define CONVERT(FROM,TO) \
INLINE void PIKE_CONCAT4(convert_,FROM,_to_,TO)(PIKE_CONCAT(p_wchar,TO) *to, const PIKE_CONCAT(p_wchar,FROM) *from, int len); \
INLINE INT32 PIKE_CONCAT4(compare_,FROM,_to_,TO)(const PIKE_CONCAT(p_wchar,TO) *to, const PIKE_CONCAT(p_wchar,FROM) *from, int len);


/* Prototypes begin here */
INLINE unsigned INT32 index_shared_string(struct pike_string *s, int pos);
INLINE void low_set_index(struct pike_string *s, int pos, int value);
INLINE struct pike_string *debug_check_size_shift(struct pike_string *a,int shift);
CONVERT(0,1)
CONVERT(0,2)
CONVERT(1,0)
CONVERT(1,2)
CONVERT(2,0)
CONVERT(2,1)
int generic_compare_strings(const void *a,int alen, int asize,
			    const void *b,int blen, int bsize);
void generic_memcpy(PCHARP to,
		    PCHARP from,
		    int len);
INLINE void pike_string_cpy(PCHARP to,
			    struct pike_string *from);
struct pike_string *binary_findstring(const char *foo, INT32 l);
struct pike_string *findstring(const char *foo);
struct pike_string *debug_begin_shared_string(int len);
struct pike_string *debug_begin_wide_shared_string(int len, int shift);
struct pike_string *low_end_shared_string(struct pike_string *s);
struct pike_string *end_shared_string(struct pike_string *s);
struct pike_string * debug_make_shared_binary_string(const char *str,int len);
struct pike_string * debug_make_shared_binary_pcharp(const PCHARP str,int len);
struct pike_string * debug_make_shared_pcharp(const PCHARP str);
struct pike_string * debug_make_shared_binary_string0(const p_wchar0 *str,int len);
struct pike_string * debug_make_shared_binary_string1(const p_wchar1 *str,int len);
struct pike_string * debug_make_shared_binary_string2(const p_wchar2 *str,int len);
struct pike_string *debug_make_shared_string(const char *str);
struct pike_string *debug_make_shared_string0(const p_wchar0 *str);
struct pike_string *debug_make_shared_string1(const p_wchar1 *str);
struct pike_string *debug_make_shared_string2(const p_wchar2 *str);
void unlink_pike_string(struct pike_string *s);
void do_free_string(struct pike_string *s);
void really_free_string(struct pike_string *s);
void debug_free_string(struct pike_string *s);
struct pike_string *add_string_status(int verbose);
void check_string(struct pike_string *s);
void verify_shared_strings_tables(void);
int safe_debug_findstring(struct pike_string *foo);
struct pike_string *debug_findstring(const struct pike_string *foo);
void debug_dump_pike_string(struct pike_string *s, INT32 max);
void dump_stralloc_strings(void);
int low_quick_binary_strcmp(char *a,INT32 alen,
			    char *b,INT32 blen);
int generic_quick_binary_strcmp(const char *a,INT32 alen, int asize,
				const char *b,INT32 blen, int bsize);
int c_compare_string(struct pike_string *s, char *foo, int len);
int my_quick_strcmp(struct pike_string *a,struct pike_string *b);
int my_strcmp(struct pike_string *a,struct pike_string *b);
struct pike_string *realloc_unlinked_string(struct pike_string *a, INT32 size);
struct pike_string *realloc_shared_string(struct pike_string *a, INT32 size);
struct pike_string *new_realloc_shared_string(struct pike_string *a, INT32 size, int shift);
struct pike_string *modify_shared_string(struct pike_string *a,
					 INT32 index,
					 INT32 c);
struct pike_string *add_shared_strings(struct pike_string *a,
					 struct pike_string *b);
struct pike_string *add_and_free_shared_strings(struct pike_string *a,
						struct pike_string *b);
int string_search(struct pike_string *haystack,
		  struct pike_string *needle,
		  int start);
struct pike_string *string_slice(struct pike_string *s,
				 INT32 start,
				 INT32 len);
struct pike_string *string_replace(struct pike_string *str,
				   struct pike_string *del,
				   struct pike_string *to);
void init_shared_string_table(void);
void cleanup_shared_string_table(void);
void count_memory_in_strings(INT32 *num, INT32 *size);
void gc_mark_all_strings(void);
void init_string_builder(struct string_builder *s, int mag);
void string_builder_putchar(struct string_builder *s, int ch);
void string_builder_binary_strcat(struct string_builder *s, char *str, INT32 len);
void string_builder_append(struct string_builder *s,
			   PCHARP from,
			   INT32 len);
void string_builder_fill(struct string_builder *s,
			 int howmany,
			 PCHARP from,
			 INT32 len,
			 INT32 offset);
void string_builder_strcat(struct string_builder *s, char *str);
void string_builder_shared_strcat(struct string_builder *s, struct pike_string *str);
void reset_string_builder(struct string_builder *s);
void free_string_builder(struct string_builder *s);
struct pike_string *finish_string_builder(struct string_builder *s);
PCHARP MEMCHR_PCHARP(PCHARP ptr, int chr, int len);
long STRTOL_PCHARP(PCHARP str, PCHARP *ptr, int base);
int string_to_svalue_inumber(struct svalue *r,
			     char * str,
			     char **ptr,
			     int base,
			     int maxlength);
int wide_string_to_svalue_inumber(struct svalue *r,
				  void * str,
				  void **ptr,
				  int base,
				  int maxlength,
				  int shift);
int pcharp_to_svalue_inumber(struct svalue *r,
			     PCHARP str,
			     PCHARP *ptr,
			     int base,
			     int maxlength);
int convert_stack_top_string_to_inumber(int base);
double STRTOD_PCHARP(PCHARP nptr, PCHARP *endptr);
p_wchar0 *require_wstring0(struct pike_string *s,
			   char **to_free);
p_wchar1 *require_wstring1(struct pike_string *s,
			   char **to_free);
p_wchar2 *require_wstring2(struct pike_string *s,
			   char **to_free);
/* Prototypes end here */

#define ISCONSTSTR(X,Y) c_compare_string((X),Y,sizeof(Y)-sizeof(""))

#ifdef DEBUG_MALLOC
#define make_shared_string(X) \
 ((struct pike_string *)debug_malloc_update_location(debug_make_shared_string(X),__FILE__,__LINE__))
#define make_shared_binary_string(X,Y) \
 ((struct pike_string *)debug_malloc_update_location(debug_make_shared_binary_string((X),(Y)),__FILE__,__LINE__))

#define make_shared_string0(X) \
 ((struct pike_string *)debug_malloc_update_location(debug_make_shared_string0(X),__FILE__,__LINE__))
#define make_shared_binary_string0(X,Y) \
 ((struct pike_string *)debug_malloc_update_location(debug_make_shared_binary_string0((X),(Y)),__FILE__,__LINE__))

#define make_shared_string1(X) \
 ((struct pike_string *)debug_malloc_update_location(debug_make_shared_string1(X),__FILE__,__LINE__))
#define make_shared_binary_string1(X,Y) \
 ((struct pike_string *)debug_malloc_update_location(debug_make_shared_binary_string1((X),(Y)),__FILE__,__LINE__))

#define make_shared_string2(X) \
 ((struct pike_string *)debug_malloc_update_location(debug_make_shared_string2(X),__FILE__,__LINE__))
#define make_shared_binary_string2(X,Y) \
 ((struct pike_string *)debug_malloc_update_location(debug_make_shared_binary_string2((X),(Y)),__FILE__,__LINE__))

#define begin_shared_string(X) \
 ((struct pike_string *)debug_malloc_update_location(debug_begin_shared_string(X),__FILE__,__LINE__))
#define begin_wide_shared_string(X,Y) \
 ((struct pike_string *)debug_malloc_update_location(debug_begin_wide_shared_string((X),(Y)),__FILE__,__LINE__))

#define make_shared_pcharp(X) \
 ((struct pike_string *)debug_malloc_update_location(debug_make_shared_pcharp(X),__FILE__,__LINE__))
#define make_shared_binary_pcharp(X,Y) \
 ((struct pike_string *)debug_malloc_update_location(debug_make_shared_binary_pcharp((X),(Y)),__FILE__,__LINE__))

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

#undef CONVERT

#endif /* STRALLOC_H */
