#ifndef STRALLOC_H
#define STRALLOC_H
#include "types.h"

#define STRINGS_ARE_SHARED

struct lpc_string
{
  SIZE_T refs;
  INT32 len;
  unsigned INT32 hval;
  struct lpc_string *next; 
  char str[1];
};

#ifdef DEBUG
struct lpc_string *debug_findstring(const struct lpc_string *foo);
#endif

#define free_string(s) do{ struct lpc_string *_=(s); if(!--_->refs) really_free_string(_); }while(0)

#define my_hash_string(X) ((unsigned int)(X))
#define my_order_strcmp(X,Y) ((char *)(X)-(char *)(Y))
#define is_same_string(X,Y) ((X)==(Y))

#define reference_shared_string(s) (s)->refs++
#define copy_shared_string(to,s) ((to)=(s))->refs++

/* Prototypes begin here */
void check_string(struct lpc_string *s);
void verify_shared_strings_tables(int pass);
struct lpc_string *findstring(const char *foo);
struct lpc_string *debug_findstring(const struct lpc_string *foo);
struct lpc_string *begin_shared_string(int len);
struct lpc_string *end_shared_string(struct lpc_string *s);
struct lpc_string * make_shared_binary_string(const char *str,int len);
struct lpc_string *make_shared_string(const char *str);
int low_quick_binary_strcmp(char *a,INT32 alen,
			    char *b,INT32 blen);
int my_quick_strcmp(struct lpc_string *a,struct lpc_string *b);
int my_strcmp(struct lpc_string *a,struct lpc_string *b);
void really_free_string(struct lpc_string *s);
struct lpc_string *add_string_status(int verbose);
void dump_stralloc_strings();
struct lpc_string *add_shared_strings(struct lpc_string *a,
					 struct lpc_string *b);
struct lpc_string *string_replace(struct lpc_string *str,
				     struct lpc_string *del,
				     struct lpc_string *to);
void cleanup_shared_string_table();
/* Prototypes end here */

#endif /* STRALLOC_H */
