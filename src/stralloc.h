/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef STRALLOC_H
#define STRALLOC_H
#include "global.h"

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
  char str[1];
};

#ifdef DEBUG
struct pike_string *debug_findstring(const struct pike_string *foo);
#endif

#define free_string(s) do{ struct pike_string *_=(s); if(--_->refs<=0) really_free_string(_); }while(0)

#define my_hash_string(X) ((unsigned long)(X))
#define my_order_strcmp(X,Y) ((char *)(X)-(char *)(Y))
#define is_same_string(X,Y) ((X)==(Y))

#define reference_shared_string(s) (s)->refs++
#define copy_shared_string(to,s) ((to)=(s))->refs++


#ifdef DEBUG_MALLOC
struct shared_string_location
{
  struct pike_string *s;
  struct shared_string_location *next;
};

extern struct shared_string_location *all_shared_string_locations;

#define MAKE_CONSTANT_SHARED_STRING(var, text) do {	\
  static struct shared_string_location str_;		\
  if(!str_.s) { 					\
    str_.s=make_shared_string((text));			\
    str_.next=all_shared_string_locations;		\
    all_shared_string_locations=&str_;			\
  }							\
 copy_shared_string((var),str_.s);			\
}while(0)


#else
#define MAKE_CONSTANT_SHARED_STRING(var, text)	\
 do { static struct pike_string *str_;		\
    if(!str_) str_=make_shared_string((text));	\
    copy_shared_string((var), str_);		\
 }while(0)

#endif



/* Prototypes begin here */
struct pike_string *binary_findstring(const char *foo, INT32 l);
struct pike_string *findstring(const char *foo);
struct pike_string *debug_begin_shared_string(int len);
struct pike_string *end_shared_string(struct pike_string *s);
struct pike_string * debug_make_shared_binary_string(const char *str,int len);
struct pike_string *debug_make_shared_string(const char *str);
void unlink_pike_string(struct pike_string *s);
void really_free_string(struct pike_string *s);
struct pike_string *add_string_status(int verbose);
void check_string(struct pike_string *s);
void verify_shared_strings_tables(void);
int safe_debug_findstring(struct pike_string *foo);
struct pike_string *debug_findstring(const struct pike_string *foo);
void dump_stralloc_strings(void);
int low_quick_binary_strcmp(char *a,INT32 alen,
			    char *b,INT32 blen);
int my_quick_strcmp(struct pike_string *a,struct pike_string *b);
int my_strcmp(struct pike_string *a,struct pike_string *b);
struct pike_string *realloc_unlinked_string(struct pike_string *a, INT32 size);
struct pike_string *realloc_shared_string(struct pike_string *a, INT32 size);
struct pike_string *modify_shared_string(struct pike_string *a,
					 INT32 index,
					 char c);
struct pike_string *add_shared_strings(struct pike_string *a,
					 struct pike_string *b);
struct pike_string *add_and_free_shared_strings(struct pike_string *a,
						struct pike_string *b);
struct pike_string *string_replace(struct pike_string *str,
				     struct pike_string *del,
				     struct pike_string *to);
void init_shared_string_table(void);
void cleanup_shared_string_table(void);
void count_memory_in_strings(INT32 *num, INT32 *size);
void gc_mark_all_strings(void);
/* Prototypes end here */

#ifdef DEBUG_MALLOC
#define make_shared_string(X) \
 ((struct pike_string *)debug_malloc_update_location(debug_make_shared_string(X),__FILE__,__LINE__))
#define make_shared_binary_string(X,Y) \
 ((struct pike_string *)debug_malloc_update_location(debug_make_shared_binary_string((X),(Y)),__FILE__,__LINE__))
#define begin_shared_string(X) \
 ((struct pike_string *)debug_malloc_update_location(debug_begin_shared_string(X),__FILE__,__LINE__))
#else
#define make_shared_string debug_make_shared_string
#define make_shared_binary_string debug_make_shared_binary_string
#define begin_shared_string debug_begin_shared_string
#endif

#endif /* STRALLOC_H */
