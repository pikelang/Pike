/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: stralloc.c,v 1.187 2004/11/08 10:30:50 grubba Exp $
*/

#include "global.h"
#include "stralloc.h"
#include "pike_macros.h"
#include "dynamic_buffer.h"
#include "pike_macros.h"
#include "pike_memory.h"
#include "pike_error.h"
#include "gc.h"
#include "stuff.h"
#include "bignum.h"
#include "interpret.h"
#include "block_alloc.h"
#include "operators.h"

#include <errno.h>
#include <float.h>
#include <ctype.h>
#include <math.h>

/* #define STRALLOC_USE_PRIMES */

#ifdef STRALLOC_USE_PRIMES 

#define SET_HSIZE(X) htable_size=hashprimes[(X)]
#define HMODULO(X) ((X) % htable_size)

#else

#define SET_HSIZE(X) htable_mask=(htable_size=(1<<(X)))-1
#define HMODULO(X) ((X) & (htable_mask))

unsigned INT32 htable_mask;

#endif

#if (SIZEOF_LONG == 4) && defined(_LP64)
/* Kludge for gcc and the system header files not using the same model... */
#undef LONG_MIN
#undef LONG_MAX
#undef ULONG_MAX
#define LONG_MIN	INT_MIN
#define LONG_MAX	INT_MAX
#define ULONG_MAX	UINT_MAX
#endif

#if PIKE_RUN_UNLOCKED
/* Make this bigger when we get lightweight threads */
#define BUCKET_LOCKS 2048
static PIKE_MUTEX_T *bucket_locks;

#define BUCKETLOCK(HVAL) \
 (bucket_locks + (HMODULO(hval__) & (BUCKET_LOCKS-1)))

#define LOCK_BUCKET(HVAL) do {						    \
  size_t hval__=(HVAL);							    \
  PIKE_MUTEX_T *bucket_lock;						    \
  while(1)								    \
  {									    \
    bucket_lock=BUCKETLOCK(hval__);                                         \
    mt_lock(bucket_lock);						    \
    if(bucket_lock == BUCKETLOCK(hval__))                                   \
      break;								    \
    mt_unlock(bucket_lock);						    \
  }									    \
}while(0)

#define UNLOCK_BUCKET(HVAL) do {			\
  size_t hval__=(HVAL);					\
  mt_unlock(BUCKETLOCK(hval__));                        \
}while(0)

#else
#define LOCK_BUCKET(HVAL)
#define UNLOCK_BUCKET(HVAL)
#endif

#define BEGIN_HASH_SIZE 997
#define MAX_AVG_LINK_LENGTH 3

/* Experimental dynamic hash length */
#ifndef HASH_PREFIX
static unsigned int HASH_PREFIX=64;
static unsigned int need_more_hash_prefix=0;
#endif

static unsigned INT32 htable_size=0;
static unsigned int hashprimes_entry=0;
static struct pike_string **base_table=0;
static unsigned INT32 num_strings=0;
PMOD_EXPORT struct pike_string *empty_pike_string = 0;

/*** Main string hash function ***/

#define StrHash(s,len) low_do_hash(s,len,0)

static INLINE size_t low_do_hash(const void *s,
				 ptrdiff_t len__,
				 int size_shift)
{
  size_t h;
  DO_HASHMEM(h, s, len__<<size_shift, HASH_PREFIX<<size_shift);
  return h;
}

static INLINE size_t do_hash(struct pike_string *s)
{
  return low_do_hash(s->str, s->len, s->size_shift);
}


static INLINE int find_magnitude1(const unsigned INT16 *s, ptrdiff_t len)
{
  while(--len>=0)
    if(s[len]>=256)
      return 1;
  return 0;
}

static INLINE int find_magnitude2(const unsigned INT32 *s, ptrdiff_t len)
{
  while(--len>=0)
  {
    if(s[len]>=256)
    {
      do
      {
	if(s[len]>=65536)
	  return 2;
      }while(--len>=0);
      return 1;
    }
  }
  return 0;
}

static INLINE int min_magnitude(const unsigned INT32 c)
{
  if(c<256) return 0;
  if(c<65536) return 1;
  return 2;
}

static INLINE unsigned INT32 generic_extract (const void *str, int size, ptrdiff_t pos)
{
  switch(size)
  {
    case 0: return ((unsigned char *)str)[pos];
    case 1: return ((unsigned INT16 *)str)[pos];
    case 2: return ((unsigned INT32 *)str)[pos];
  }
  Pike_fatal("Illegal shift size!\n");
  return 0;
}

PMOD_EXPORT unsigned INT32 index_shared_string(struct pike_string *s,
					       ptrdiff_t pos)
{
#ifdef PIKE_DEBUG
  if(pos > s->len || pos<0) {
    if (s->len) {
      Pike_fatal("String index %"PRINTPTRDIFFT"d is out of "
		 "range 0..%"PRINTPTRDIFFT"d.\n",
		 pos, s->len-1);
    } else {
      Pike_fatal("Attempt to index the empty string with %"PRINTPTRDIFFT"d.\n",
		 pos);
    }
  }
#endif
  return generic_extract(s->str,s->size_shift,pos);
}

PMOD_EXPORT void low_set_index(struct pike_string *s, ptrdiff_t pos,
			       int value)
{
#ifdef PIKE_DEBUG
  if(pos > s->len || pos<0) {
    if (s->len) {
      Pike_fatal("String index %"PRINTPTRDIFFT"d is out of "
		 "range 0..%"PRINTPTRDIFFT"d.\n",
		 pos, s->len-1);
    } else {
      Pike_fatal("Attempt to index the empty string with %"PRINTPTRDIFFT"d.\n",
		 pos);
    }
  }

  if(pos == s->len && value)
    Pike_fatal("string zero termination foul!\n");
#endif
  switch(s->size_shift)
  {
    case 0: STR0(s)[pos]=value; break;
    case 1: STR1(s)[pos]=value; break;
    case 2: STR2(s)[pos]=value; break;
    default:
      Pike_fatal("Illegal shift size!\n");
  }
}

#ifdef PIKE_DEBUG
PMOD_EXPORT struct pike_string *debug_check_size_shift(struct pike_string *a,
						       int shift)
{
  if(a->size_shift != shift)
    Pike_fatal("Wrong STRX macro used!\n");
  return a;
}
#endif

#define CONVERT(FROM,TO) \
void PIKE_CONCAT4(convert_,FROM,_to_,TO)(PIKE_CONCAT(p_wchar,TO) *to, const PIKE_CONCAT(p_wchar,FROM) *from, ptrdiff_t len) \
{  while(--len>=0) *(to++)=*(from++); } \
INT32 PIKE_CONCAT4(compare_,FROM,_to_,TO)(const PIKE_CONCAT(p_wchar,TO) *to, const PIKE_CONCAT(p_wchar,FROM) *from, ptrdiff_t len) \
{ int tmp; while(--len>=0) if((tmp=*(to++)-*(from++))) return tmp; return 0; }


CONVERT(0,1)
CONVERT(0,2)
CONVERT(1,0)
CONVERT(1,2)
CONVERT(2,0)
CONVERT(2,1)


PMOD_EXPORT int generic_compare_strings(const void *a, ptrdiff_t alen, int asize,
					const void *b, ptrdiff_t blen, int bsize)
{
#define TWO_SIZES(X,Y) (((X)<<2)+(Y))
  if(alen != blen) return 0;
  if(asize==bsize)
  {
    return !MEMCMP(a,b,alen<<asize);
  }else{
    INT32 pos;
    for(pos=0;pos< alen ;pos++)
      if(generic_extract(a,asize,pos) != generic_extract(b,bsize,pos))
	return 0;
    return 1;
  }
}


PMOD_EXPORT void generic_memcpy(PCHARP to,
				PCHARP from,
				ptrdiff_t len)
{
#ifdef PIKE_DEBUG
  if(len<0)
    Pike_fatal("Cannot copy %ld bytes!\n",
	  DO_NOT_WARN((long)len));
#endif

  switch(TWO_SIZES(from.shift,to.shift))
  {
    case TWO_SIZES(0,0):
      convert_0_to_0((p_wchar0 *)to.ptr,(p_wchar0 *)from.ptr,len);
      break;
    case TWO_SIZES(0,1):
      convert_0_to_1((p_wchar1 *)to.ptr,(p_wchar0 *)from.ptr,len);
      break;
    case TWO_SIZES(0,2):
      convert_0_to_2((p_wchar2 *)to.ptr,(p_wchar0 *)from.ptr,len);
      break;

    case TWO_SIZES(1,0):
      convert_1_to_0((p_wchar0 *)to.ptr,(p_wchar1 *)from.ptr,len);
      break;
    case TWO_SIZES(1,1):
      convert_1_to_1((p_wchar1 *)to.ptr,(p_wchar1 *)from.ptr,len);
      break;
    case TWO_SIZES(1,2):
      convert_1_to_2((p_wchar2 *)to.ptr,(p_wchar1 *)from.ptr,len);
      break;

    case TWO_SIZES(2,0):
      convert_2_to_0((p_wchar0 *)to.ptr,(p_wchar2 *)from.ptr,len);
      break;
    case TWO_SIZES(2,1):
      convert_2_to_1((p_wchar1 *)to.ptr,(p_wchar2 *)from.ptr,len);
      break;
    case TWO_SIZES(2,2):
      convert_2_to_2((p_wchar2 *)to.ptr,(p_wchar2 *)from.ptr,len);
      break;
  }
}

PMOD_EXPORT void pike_string_cpy(PCHARP to, struct pike_string *from)
{
  generic_memcpy(to,MKPCHARP_STR(from),from->len);
}


#ifdef PIKE_DEBUG
#ifdef DEBUG_MALLOC
#define DM(X) X
#else
#define DM(X)
#endif

static void locate_problem(int (*isproblem)(struct pike_string *))
{
  unsigned INT32 e;
  struct pike_string *s;
  DM(struct memhdr *yes=alloc_memhdr());
  DM(struct memhdr *no=alloc_memhdr());

  for(e=0;e<htable_size;e++)
  {
    LOCK_BUCKET(e);
    for(s=base_table[e];s;s=s->next)
    {
      if(isproblem(s))
      {
	fprintf(stderr,"***Guilty string:\n");
	debug_dump_pike_string(s, 70);
	DM(add_marks_to_memhdr(yes,s));
      }else{
	DM(add_marks_to_memhdr(no,s));
      }
    }
    UNLOCK_BUCKET(e);
  }

  DM(fprintf(stderr,"Plausible problem location(s):\n"));
  DM(dump_memhdr_locations(yes,0,0));

  DM(fprintf(stderr,"More Plausible problem location(s):\n"));
  DM(dump_memhdr_locations(yes,no,0));
}

static int bad_pointer(struct pike_string *s)
{
  return (((ptrdiff_t)s)&(sizeof(struct pike_string *)-1));
}

static int has_zero_refs(struct pike_string *s)
{
  return s->refs<=0;
}
static int wrong_hash(struct pike_string *s)
{
  return s->hval != do_hash(s);
}
static int improper_zero_termination(struct pike_string *s)
{
  return index_shared_string(s,s->len);
}
#else
#define locate_problem(X)
#endif

/* Find a string in the shared string table.
 * This assumes that the string is minimized!!!! 
 */
static INLINE struct pike_string *internal_findstring(const char *s,
						      ptrdiff_t len,
						      int size_shift,
						      size_t hval)
{
  struct pike_string *curr,**prev, **base;
#ifndef HASH_PREFIX
  unsigned int depth=0;
#endif
  size_t h;
  LOCK_BUCKET(hval);
  h=HMODULO(hval);
  for(base = prev = base_table + h;( curr=*prev ); prev=&curr->next)
  {
#ifdef PIKE_DEBUG
    if(curr->refs<1)
    {
      debug_dump_pike_string(curr, 70);
      locate_problem(has_zero_refs);
      Pike_fatal("String with no references.\n");
    }
#endif
    debug_malloc_touch(curr);

    if (hval == curr->hval &&
	len==curr->len &&
	size_shift==curr->size_shift &&
	( curr->str == s ||
	  !MEMCMP(curr->str, s,len<<size_shift))) /* found it */
    {
      *prev = curr->next;
      curr->next = *base;
      *base = curr;
      UNLOCK_BUCKET(hval);
      return curr;		/* pointer to string */
    }
#ifndef HASH_PREFIX
    depth++;
#endif
  }
#ifndef HASH_PREFIX
  /* These heuruistics might require tuning! /Hubbe */
  if((depth > HASH_PREFIX) && (HASH_PREFIX < (size_t)len))
  {
    need_more_hash_prefix++;
/*    fprintf(stderr,"depth=%d  num_strings=%d need_more_hash_prefix=%d  HASH_PREFIX=%d\n",depth,num_strings,need_more_hash_prefix,HASH_PREFIX); */
  }else{
    if(need_more_hash_prefix)
      need_more_hash_prefix--;
  }
#endif
  UNLOCK_BUCKET(hval);
  return 0; /* not found */
}

PMOD_EXPORT struct pike_string *binary_findstring(const char *foo, ptrdiff_t l)
{
  return internal_findstring(foo, l, 0, StrHash(foo,l));
}

PMOD_EXPORT struct pike_string *findstring(const char *foo)
{
  return binary_findstring(foo, strlen(foo));
}

/*
 * find a string that is already shared and move it to the head
 * of that list in the hastable
 */
static struct pike_string *propagate_shared_string(const struct pike_string *s,
						   ptrdiff_t h)
{
  struct pike_string *curr, **prev, **base;

  for(base = prev = base_table + h;( curr=*prev ); prev=&curr->next)
  {
    if (curr == s) /* found it */
    {
      *prev=curr->next;
      curr->next=*base;
      *base=curr;
      return curr;
    }
#ifdef PIKE_DEBUG
    if(curr->refs<1)
    {
      debug_dump_pike_string(curr, 70);
      locate_problem(has_zero_refs);
      Pike_fatal("String with no references.\n");
    }
#endif
  }
  return 0; /* not found */
}

/*** rehash ***/

static void rehash_string_backwards(struct pike_string *s)
{
  ptrdiff_t h;
  if(!s) return;
  rehash_string_backwards(s->next);
  h = HMODULO(s->hval);
  s->next=base_table[h];
  base_table[h]=s;
}

static void stralloc_rehash(void)
{
  int h,old;
  struct pike_string **old_base;

  old=htable_size;
  old_base=base_table;

#ifdef PIKE_RUN_UNLOCKED
  mt_lock(bucket_locks);
  if(old != htable_size)
  {
    /* Someone got here before us */
    mt_lock(bucket_locks);
    return;
  }

  /* Now that we have bucket zero, the hash table
   * cannot change, go ahead and lock ALL buckets.
   * NOTE: bucket zero is already locked
   */
  for(h=1;h<BUCKET_LOCKS;h++) mt_lock(bucket_locks+h);
#endif

  SET_HSIZE( ++hashprimes_entry );

  base_table=(struct pike_string **)xalloc(sizeof(struct pike_string *)*htable_size);
  MEMSET((char *)base_table,0,sizeof(struct pike_string *)*htable_size);

  for(h=0;h<old;h++)
    rehash_string_backwards(old_base[h]);

  if(old_base)
    free((char *)old_base);

#ifdef PIKE_RUN_UNLOCKED
  for(h=0;h<BUCKET_LOCKS;h++) mt_unlock(bucket_locks + h);
#endif
}

/* Allocation of strings */

/* Allocate some fixed string sizes with BLOCK_ALLOC. */

/* Use the BLOCK_ALLOC() stuff for short strings */

#define SHORT_STRING_BLOCK	256
#define SHORT_STRING_THRESHOLD	15 /* % 4 === 1 */

struct short_pike_string0 {
  PIKE_STRING_CONTENTS;
  p_wchar0 str[SHORT_STRING_THRESHOLD+1];
};

struct short_pike_string1 {
  PIKE_STRING_CONTENTS;
  p_wchar1 str[SHORT_STRING_THRESHOLD+1];
};

struct short_pike_string2 {
  PIKE_STRING_CONTENTS;
  p_wchar2 str[SHORT_STRING_THRESHOLD+1];
};

BLOCK_ALLOC(short_pike_string0, SHORT_STRING_BLOCK)
BLOCK_ALLOC(short_pike_string1, SHORT_STRING_BLOCK)
BLOCK_ALLOC(short_pike_string2, SHORT_STRING_BLOCK)

#define really_free_short_pike_string(s) do { \
     if (!s->size_shift) { \
       really_free_short_pike_string0((struct short_pike_string0 *)s); \
     } else if (s->size_shift == 1) { \
       really_free_short_pike_string1((struct short_pike_string1 *)s); \
     DO_IF_DEBUG( \
     } else if (s->size_shift != 2) { \
       Pike_fatal("Unsupported string shift: %d\n", s->size_shift); \
     ) \
     } else { \
       really_free_short_pike_string2((struct short_pike_string2 *)s); \
     } \
   } while(0)

#define really_free_pike_string(s) do { \
    if (s->len <= SHORT_STRING_THRESHOLD) { \
      really_free_short_pike_string(s); \
    } else { \
      debug_free((char *)s, DMALLOC_LOCATION(), 1); \
    } \
  } while(0)

/* note that begin_shared_string expects the _exact_ size of the string,
 * not the maximum size
 */
PMOD_EXPORT struct pike_string *debug_begin_shared_string(size_t len)
{
  struct pike_string *t;
#ifdef PIKE_DEBUG
  extern int d_flag;
  if(d_flag>10)
    verify_shared_strings_tables();
#endif
  if (len <= SHORT_STRING_THRESHOLD) {
    t=(struct pike_string *)alloc_short_pike_string0();
  } else {
    t=(struct pike_string *)xalloc(len + sizeof(struct pike_string));
  }
  t->str[len]=0;
  t->len=len;
  t->size_shift=0;
  return t;
}

static void link_pike_string(struct pike_string *s, size_t hval)
{
  size_t h;
  LOCK_BUCKET(hval);
  h=HMODULO(hval);
  s->refs = 0;
  s->next = base_table[h];
  base_table[h] = s;
  s->hval=hval;
  num_strings++;
  UNLOCK_BUCKET(hval);

  if(num_strings > MAX_AVG_LINK_LENGTH * htable_size)
    stralloc_rehash();

#ifndef HASH_PREFIX
  /* These heuruistics might require tuning! /Hubbe */
  if(need_more_hash_prefix > ( htable_size >> 4))
  {
    /* This could in theory have a pretty ugly complexity */
    /* /Hubbe
     */

#ifdef PIKE_RUN_UNLOCKED
    mt_lock(bucket_locks);
    if(need_more_hash_prefix <= ( htable_size >> 4))
    {
      /* Someone got here before us */
      mt_lock(bucket_locks);
      return;
    }
    for(h=1;h<BUCKET_LOCKS;h++) mt_lock(bucket_locks+h);
#endif

    need_more_hash_prefix=0;
    HASH_PREFIX=HASH_PREFIX*2;
/*    fprintf(stderr,"Doubling HASH_PREFIX to %d and rehashing\n",HASH_PREFIX); */

    for(h=0;h<htable_size;h++)
    {
      struct pike_string *tmp=base_table[h];
      base_table[h]=0;
      while(tmp)
      {
	size_t h2;
	struct pike_string *tmp2=tmp; /* First unlink */
	tmp=tmp2->next;

	tmp2->hval=do_hash(tmp2); /* compute new hash value */
	h2=HMODULO(tmp2->hval);

	tmp2->next=base_table[h2];    /* and re-hash */
	base_table[h2]=tmp2;
      }
    }
#ifdef PIKE_RUN_UNLOCKED
    for(h=0;h<BUCKET_LOCKS;h++) mt_unlock(bucket_locks + h);
#endif
  }
#endif
}

PMOD_EXPORT struct pike_string *debug_begin_wide_shared_string(size_t len, int shift)
{
  struct pike_string *t = NULL;
#ifdef PIKE_DEBUG
  extern int d_flag;
  if(d_flag>10)
    verify_shared_strings_tables();
#endif
  if (len <= SHORT_STRING_THRESHOLD) {
    if (!shift) {
      t = (struct pike_string *)alloc_short_pike_string0();
    } else if (shift == 1) {
      t = (struct pike_string *)alloc_short_pike_string1();
#ifdef PIKE_DEBUG
    } else if (shift != 2) {
      Pike_fatal("Unsupported string shift: %d\n", shift);
#endif /* PIKE_DEBUG */
    } else {
      t = (struct pike_string *)alloc_short_pike_string2();
    }
  } else {
    t=(struct pike_string *)xalloc((len<<shift) + sizeof(struct pike_string));
  }
  t->len=len;
  t->size_shift=shift;
  low_set_index(t,len,0);
  return t;
}

/*
 * This function assumes that the shift size is already the minimum it
 * can be.
 */
PMOD_EXPORT struct pike_string *low_end_shared_string(struct pike_string *s)
{
  ptrdiff_t len;
  size_t h;
  struct pike_string *s2;

  len = s->len;
  h = do_hash(s);
  s2 = internal_findstring(s->str, len, s->size_shift, h);
#ifdef PIKE_DEBUG
  if(s2==s) 
    Pike_fatal("end_shared_string called twice! (or something like that)\n");
#endif

  if(s2)
  {
    really_free_pike_string(s);
    s = s2;
  }else{
    link_pike_string(s, h);
  }
  add_ref(s);

  return s;
  
}

/*
 * This function checks if the shift size can be decreased before
 * entering the string in the shared string table
 */
PMOD_EXPORT struct pike_string *end_shared_string(struct pike_string *s)
{
  struct pike_string *s2;

  switch(s->size_shift)
  {
    default:
      Pike_fatal("ARGHEL! size_shift:%d\n", s->size_shift);

    case 2:
      switch(find_magnitude2(STR2(s),s->len))
      {
	case 0:
	  s2=begin_shared_string(s->len);
	  convert_2_to_0(STR0(s2),STR2(s),s->len);
	  really_free_pike_string(s);
	  s=s2;
	  break;

	case 1:
	  s2=begin_wide_shared_string(s->len,1);
	  convert_2_to_1(STR1(s2),STR2(s),s->len);
	  really_free_pike_string(s);
	  s=s2;
	  /* Fall though */
      }
      break;
      
    case 1:
      if(!find_magnitude1(STR1(s),s->len))
      {
	s2=begin_shared_string(s->len);
	convert_1_to_0(STR0(s2),STR1(s),s->len);
	really_free_pike_string(s);
	s=s2;
      }
      break;

    case 0: break;
  }

  return low_end_shared_string(s);
}

PMOD_EXPORT struct pike_string *end_and_resize_shared_string(struct pike_string *str, ptrdiff_t len)
{
  struct pike_string *tmp;
#ifdef PIKE_DEBUG
  if(len > str->len)
    Pike_fatal("Cannot extend string here!\n");
#endif
  if( str->len <= SHORT_STRING_THRESHOLD ?
      (len <= SHORT_STRING_THRESHOLD) :
      (len >  SHORT_STRING_THRESHOLD) && str->len  > len/2 )
  {
    str->len=len;
    str->str[len]=0;
    return end_shared_string(str);
  }
  tmp = make_shared_binary_pcharp(MKPCHARP_STR(str),len);
  really_free_pike_string(str);
  return tmp;
}


PMOD_EXPORT struct pike_string * debug_make_shared_binary_string(const char *str,size_t len)
{
  struct pike_string *s;
  ptrdiff_t h = StrHash(str, len);

  s = internal_findstring(str,len,0,h);
  if (!s) 
  {
    s=begin_shared_string(len);
    MEMCPY(s->str, str, len);
    link_pike_string(s, h);
  }

  add_ref(s);

  return s;
}

PMOD_EXPORT struct pike_string * debug_make_shared_binary_pcharp(const PCHARP str,size_t len)
{
  switch(str.shift)
  {
    case 0:
      return make_shared_binary_string((char *)(str.ptr),  len);
    case 1:
      return make_shared_binary_string1((p_wchar1 *)(str.ptr),  len);
    case 2:
      return make_shared_binary_string2((p_wchar2 *)(str.ptr),  len);
    default:
      Pike_fatal("Unknown string width!\n");
  }
  /* NOT REACHED */
  return NULL;	/* Keep the compiler happy */
}

PMOD_EXPORT struct pike_string * debug_make_shared_pcharp(const PCHARP str)
{
  return debug_make_shared_binary_pcharp(str, pcharp_strlen(str));
}

PMOD_EXPORT struct pike_string * debug_make_shared_binary_string0(const p_wchar0 *str,size_t len)
{
  return debug_make_shared_binary_string((const char *)str, len);
}

PMOD_EXPORT struct pike_string * debug_make_shared_binary_string1(const p_wchar1 *str,size_t len)
{
  struct pike_string *s;
  ptrdiff_t h;

  if(!find_magnitude1(str,len))
  {
    /* Wrong size, convert */
    s=begin_shared_string(len);
    convert_1_to_0(STR0(s),str,len);
    return end_shared_string(s);
  }

  h=low_do_hash(str, len, 1);

  s = internal_findstring((char *)str,len,1,h);
  if (!s) 
  {
    s=begin_wide_shared_string(len,1);
    MEMCPY(s->str, str, len<<1);
    link_pike_string(s, h);
  }

  add_ref(s);

  return s;
}

PMOD_EXPORT struct pike_string * debug_make_shared_binary_string2(const p_wchar2 *str,size_t len)
{
  struct pike_string *s;
  ptrdiff_t h;

  switch(find_magnitude2(str,len))
  {
    case 0:
      /* Wrong size, convert */
      s=begin_shared_string(len);
      convert_2_to_0(STR0(s),str,len);
      return end_shared_string(s);

    case 1:
      /* Wrong size, convert */
      s=begin_wide_shared_string(len,1);
      convert_2_to_1(STR1(s),str,len);
      return end_shared_string(s); /* not entirely optimal */
  }

  h=low_do_hash(str, len, 2);

  s = internal_findstring((char *)str,len,2,h);
  if (!s) 
  {
    s=begin_wide_shared_string(len,2);
    MEMCPY(s->str, str, len<<2);
    link_pike_string(s, h);
  }

  add_ref(s);

  return s;
}

PMOD_EXPORT struct pike_string *debug_make_shared_string(const char *str)
{
  return make_shared_binary_string(str, strlen(str));
}

PMOD_EXPORT struct pike_string *debug_make_shared_string0(const p_wchar0 *str)
{
  return debug_make_shared_string((const char *)str);
}

PMOD_EXPORT struct pike_string *debug_make_shared_string1(const p_wchar1 *str)
{
  INT32 len;
  for(len=0;str[len];len++);
  return debug_make_shared_binary_string1(str,len);
}

PMOD_EXPORT struct pike_string *debug_make_shared_string2(const p_wchar2 *str)
{
  INT32 len;
  for(len=0;str[len];len++);
  return debug_make_shared_binary_string2(str,len);
}

/*** Free strings ***/

PMOD_EXPORT void unlink_pike_string(struct pike_string *s)
{
  size_t h;
  LOCK_BUCKET(s->hval);
  h= HMODULO(s->hval);
  propagate_shared_string(s,h);
#ifdef PIKE_DEBUG
  if (base_table[h] != s) {
    Pike_fatal("propagate_shared_string() failed. Probably got bogus pike_string.\n");
  }
#endif /* PIKE_DEBUG */
  base_table[h]=s->next;
#ifdef PIKE_DEBUG
  s->next=(struct pike_string *)(ptrdiff_t)-1;
#endif
  num_strings--;
  UNLOCK_BUCKET(s->hval);
}

PMOD_EXPORT void do_free_string(struct pike_string *s)
{
  if (s)
    free_string(s);
}

PMOD_EXPORT void do_really_free_string(struct pike_string *s)
{
  if (s)
    really_free_string(s);
}

PMOD_EXPORT void do_really_free_pike_string(struct pike_string *s)
{
  if (s)
    really_free_pike_string(s);
}

PMOD_EXPORT void really_free_string(struct pike_string *s)
{
#ifdef PIKE_DEBUG
  extern int d_flag;
  if (s->refs) {
#ifdef DEBUG_MALLOC
    describe_something(s, T_STRING, 0,2,0, NULL);
#endif
    Pike_fatal("Freeing string with %d references.\n", s->refs);
  }
  if(d_flag > 2)
  {
    if(s->next == (struct pike_string *)(ptrdiff_t)-1)
      Pike_fatal("Freeing shared string again!\n");

    if(((ptrdiff_t)s->next) & 1)
      Pike_fatal("Freeing shared string again, memory corrupt or other bug!\n");
  }
  if ((s->size_shift < 0) || (s->size_shift > 2)) {
    Pike_fatal("Freeing string with bad shift (0x%08x); could it be a type?\n",
	  s->size_shift);
  }
#endif
  unlink_pike_string(s);
  really_free_pike_string(s);
  GC_FREE_SIMPLE_BLOCK(s);
}

PMOD_EXPORT void debug_free_string(struct pike_string *s)
{
  if(!sub_ref(s))
    really_free_string(s);
}


/*
 * String table status
 */
struct pike_string *add_string_status(int verbose)
{
  dynamic_buffer save_buf;
  char b[200];

  init_buf(&save_buf);

  if (verbose)
  {
    int allocd_strings=0;
    int allocd_bytes=0;
    int num_distinct_strings=0;
    int bytes_distinct_strings=0;
    ptrdiff_t overhead_bytes = 0;
    unsigned INT32 e;
    struct pike_string *p;
    for(e=0;e<htable_size;e++)
    {
      LOCK_BUCKET(e);
      for(p=base_table[e];p;p=p->next)
      {
	num_distinct_strings++;
	bytes_distinct_strings+=DO_ALIGN(p->len,sizeof(void *));
	allocd_strings+=p->refs;
	allocd_bytes+=p->refs*DO_ALIGN(p->len+3,sizeof(void *));
      }
      UNLOCK_BUCKET(e);
    }
    overhead_bytes=(sizeof(struct pike_string)-1)*num_distinct_strings;
    my_strcat("\nShared string hash table:\n");
    my_strcat("-------------------------\t Strings    Bytes\n");

    sprintf(b,"Total asked for\t\t\t%8ld %8ld\n",
	    (long)allocd_strings, (long)allocd_bytes);
    my_strcat(b);
    sprintf(b,"Strings malloced\t\t%8ld %8ld + %ld overhead\n",
	    (long)num_distinct_strings,
	    (long)bytes_distinct_strings,
	    DO_NOT_WARN((long)overhead_bytes));
    my_strcat(b);
    sprintf(b, "Space actually required/total string bytes %ld%%\n",
	    DO_NOT_WARN((long)((bytes_distinct_strings + overhead_bytes)*100 /
			       allocd_bytes)));
    my_strcat(b);
  }
/*
  sprintf(b,"Searches: %ld    Average search length: %6.3f\n",
      (long)num_str_searches, (double)search_len / num_str_searches);
  my_strcat(b);
*/
  return free_buf(&save_buf);
}

/*** PIKE_DEBUG ***/
#ifdef PIKE_DEBUG

static long last_stralloc_verify=0;
extern long current_do_debug_cycle;

PMOD_EXPORT void check_string(struct pike_string *s)
{
  if(current_do_debug_cycle == last_stralloc_verify)
  {
    if(debug_findstring(s) !=s)
      Pike_fatal("Shared string not shared.\n");
  }else{
    if(do_hash(s) != s->hval)
    {
      locate_problem(wrong_hash);
      Pike_fatal("Hash value changed?\n");
    }
    
    if(debug_findstring(s) !=s)
      Pike_fatal("Shared string not shared.\n");

    if(index_shared_string(s,s->len))
    {
      locate_problem(improper_zero_termination);
      Pike_fatal("Shared string is not zero terminated properly.\n");
    }
  }
}

PMOD_EXPORT void verify_shared_strings_tables(void)
{
  unsigned INT32 e, h, num=0;
  struct pike_string *s;

  last_stralloc_verify=current_do_debug_cycle;

  for(e=0;e<htable_size;e++)
  {
    h=0;
    LOCK_BUCKET(e);
    for(s=base_table[e];s;s=s->next)
    {
      num++;
      h++;

      if (bad_pointer(s)) {
	Pike_fatal("Odd string pointer in string table!\n");
      }

      if(s->len < 0)
	Pike_fatal("Shared string shorter than zero bytes.\n");

      if(s->refs <= 0)
      {
	locate_problem(has_zero_refs);
	Pike_fatal("Shared string had too few references.\n");
      }

      if(index_shared_string(s,s->len))
      {
	locate_problem(improper_zero_termination);
	Pike_fatal("Shared string didn't end with a zero.\n");
      }

      if(do_hash(s) != s->hval)
      {
	locate_problem(wrong_hash);
	Pike_fatal("Shared string hashed to other number.\n");
      }

      if(HMODULO(s->hval) != e)
      {
	locate_problem(wrong_hash);
	Pike_fatal("Shared string hashed to wrong place.\n");
      }

      if(h>10000)
      {
	struct pike_string *s2;
	for(s2=s;s2;s2=s2->next)
	  if(s2 == s)
	    Pike_fatal("Shared string table is cyclic.\n");
	h=0;
      }
    }
    UNLOCK_BUCKET(e);
  }
  if(num != num_strings)
    Pike_fatal("Num strings is wrong %d!=%d\n",num,num_strings);
}

PMOD_EXPORT int safe_debug_findstring(struct pike_string *foo)
{
  unsigned INT32 e;
  if(!base_table) return 0;
  for(e=0;e<htable_size;e++)
  {
    struct pike_string *p;
    LOCK_BUCKET(e);
    for(p=base_table[e];p;p=p->next)
    {
      if(p==foo)
      {
	UNLOCK_BUCKET(e);
	return 1;
      }
    }
    UNLOCK_BUCKET(e);
  }
  return 0;
}

PMOD_EXPORT struct pike_string *debug_findstring(const struct pike_string *foo)
{
  struct pike_string *tmp;
  tmp=propagate_shared_string(foo, HMODULO(foo->hval));

#if 0
  if(!tmp)
  {
    unsigned INT32 e;
    struct pike_string *tmp2;
    fprintf(stderr,"String %p %ld %ld %s\n",
	    foo,
	    (long)foo->hval,
	    (long)foo->len,
	    foo->str);

    LOCK_BUCKET(foo->hval);
    fprintf(stderr,"------ %p %ld\n",
	    base_table[HMODULO(foo->hval)],
	    foo->hval);
    for(tmp2=base_table[HMODULO(foo->hval)];tmp2;tmp2=tmp2->next)
    {
      if(tmp2 == tmp)
	fprintf(stderr,"!!%p!!->",tmp2);
      else
	fprintf(stderr,"%p->",tmp2);
    }
    fprintf(stderr,"0\n");
    UNLOCK_BUCKET(foo->hval);

    for(e=0;e<htable_size;e++)
    {
      LOCK_BUCKET(e);
      for(tmp2=base_table[e];tmp2;tmp2=tmp2->next)
      {
	if(tmp2 == tmp)
	  fprintf(stderr,"String found in hashbin %ld (not %ld)\n",
		  (long)e,
		  (long)HMODULO(foo->hval));
      }
      UNLOCK_BUCKET(e);
    }
  }
#endif
  return tmp;
}

PMOD_EXPORT void debug_dump_pike_string(struct pike_string *s, INT32 max)
{
  INT32 e;
  fprintf(stderr,"0x%p: %ld refs, len=%ld, size_shift=%d, hval=%lux (%lx)\n",
	  s,
	  (long)s->refs,
	  DO_NOT_WARN((long)s->len),
	  s->size_shift,
	  DO_NOT_WARN((unsigned long)s->hval),
	  DO_NOT_WARN((unsigned long)StrHash(s->str, s->len)));
  fprintf(stderr," \"");
  for(e=0;e<s->len && max>0;e++)
  {
    int c=EXTRACT_UCHAR(s->str+e);
    switch(c)
    {
      case '\t': fprintf(stderr,"\\t"); max-=2; break;
      case '\n': fprintf(stderr,"\\n"); max-=2; break;
      case '\r': fprintf(stderr,"\\r"); max-=2; break;
      case '\b': fprintf(stderr,"\\b"); max-=2; break;

      default:
	if(is8bitalnum(c) || c==' ' || isgraph(c))
	{
	  putc(c,stderr);
	  max--;
	}else{
	  fprintf(stderr,"\\%03o",c);
	  max-=4;
	}
    }
  }
  if(!max)
    fprintf(stderr,"...\n");
  else
    fprintf(stderr,"\"\n");
}

void dump_stralloc_strings(void)
{
  unsigned INT32 e;
  struct pike_string *p;
  for(e=0;e<htable_size;e++)
  {
    LOCK_BUCKET(e);
    for(p=base_table[e];p;p=p->next)
      debug_dump_pike_string(p, 70);
    UNLOCK_BUCKET(e);
  }
}

#endif


/*** String compare functions ***/

/* does not take locale into account */
PMOD_EXPORT int low_quick_binary_strcmp(char *a, ptrdiff_t alen,
					char *b, ptrdiff_t blen)
{
  int tmp;
  if(alen > blen)
  {
    tmp=MEMCMP(a, b, blen);
    if(tmp) return tmp;
    return 1;
  }else if(alen < blen){
    tmp=MEMCMP(a, b, alen);
    if(tmp) return tmp;
    return -1;
  }else{
    return MEMCMP(a, b, alen);
  }
}


/* does not take locale into account */
PMOD_EXPORT ptrdiff_t generic_quick_binary_strcmp(const char *a,
						  ptrdiff_t alen, int asize,
						  const char *b,
						  ptrdiff_t blen, int bsize)
{
  if(!asize && !bsize)
  {
    int tmp;
    if(alen > blen)
    {
      tmp=MEMCMP(a, b, blen);
      if(tmp) return tmp;
      return 1;
    }else if(alen < blen){
      tmp=MEMCMP(a, b, alen);
      if(tmp) return tmp;
      return -1;
    }else{
      return MEMCMP(a, b, alen);
    }
  }else{
    INT32 pos;
    for(pos=0;pos< MINIMUM(alen,blen) ;pos++)
    {
      INT32 ac=generic_extract(a,asize,pos);
      INT32 bc=generic_extract(b,bsize,pos);
      if(ac-bc) return ac-bc;
    }
    return alen-blen;
  }
}

PMOD_EXPORT int c_compare_string(struct pike_string *s, char *foo, int len)
{
  return s->len == len && s->size_shift == 0 && !MEMCMP(s->str,foo,len);
}

#ifndef HAVE_STRCOLL
/* No locale function available */
static int low_binary_strcmp(char *a, ptrdiff_t alen,
			     char *b, ptrdiff_t blen)
{
  low_quick_binary_strcmp(a,alen,b,blen);
}
#else

/* takes locale into account */
static int low_binary_strcmp(char *a, ptrdiff_t alen,
			     char *b, ptrdiff_t blen)
{
  while(alen>0 && blen>0)
  {
    int tmp1 = strcoll(a,b);
    ptrdiff_t tmp2;
    if(tmp1) return (int)tmp1;
    tmp2 = strlen(a)+1;
    a += tmp2;
    b += tmp2;
    alen -= tmp2;
    blen -= tmp2;
  }
  if(alen==blen) return 0;
  if(alen > blen) return 1;
  return -1;
}
#endif

/* Does not take locale into account */

PMOD_EXPORT ptrdiff_t my_quick_strcmp(struct pike_string *a,
				      struct pike_string *b)
{
  if(a==b) return 0;

  return generic_quick_binary_strcmp(a->str, a->len, a->size_shift,
				     b->str, b->len, b->size_shift);
}

/* Does take locale into account */
PMOD_EXPORT ptrdiff_t my_strcmp(struct pike_string *a,struct pike_string *b)
{
  if(a==b) return 0;

  switch(TWO_SIZES(a->size_shift,b->size_shift))
  {
    case TWO_SIZES(0,0):
      return low_binary_strcmp(a->str,a->len,b->str,b->len);

    default:
    {
      ptrdiff_t e, l = MINIMUM(a->len, b->len);
      for(e=0;e<l;e++)
      {
	INT32 ac=index_shared_string(a,e);
	INT32 bc=index_shared_string(b,e);

#ifdef HAVE_STRCOLL
	if(ac < 256 && bc < 256)
	{
	  char atmp[2],btmp[2];
	  int tmp;
	  atmp[0]=ac;
	  btmp[0]=bc;
	  atmp[1]=0;
	  btmp[1]=0;
	  if((tmp=strcoll(atmp,btmp)))
	     return tmp;
	}else
#endif
	  if(ac-bc) return ac-bc;
      }
      return a->len - b->len;
    }
  }
}

PMOD_EXPORT struct pike_string *realloc_unlinked_string(struct pike_string *a,
							ptrdiff_t size)
{
  struct pike_string *r = NULL;

  if (a->len <= SHORT_STRING_THRESHOLD) {
    if (size <= SHORT_STRING_THRESHOLD) {
      /* There's already place enough. */
      a->len = size;
      low_set_index(a, size, 0);
      return a;
    }
  } else if (size > SHORT_STRING_THRESHOLD) {
    r=(struct pike_string *)realloc((char *)a,
				    sizeof(struct pike_string)+
				    ((size+1)<<a->size_shift));
  }
	
  if(!r)
  {
    r=begin_wide_shared_string(size, a->size_shift);
    if (a->len <= size) {
      MEMCPY(r->str, a->str, a->len<<a->size_shift);
    } else {
      MEMCPY(r->str, a->str, size<<a->size_shift);
    }
    really_free_pike_string(a);
  }

  r->len=size;
  low_set_index(r,size,0);
  return r;
}

/* Returns an unlinked string ready for end_shared_string */
PMOD_EXPORT struct pike_string *realloc_shared_string(struct pike_string *a,
						      ptrdiff_t size)
{
  struct pike_string *r;
  if(a->refs==1)
  {
    unlink_pike_string(a);
    return realloc_unlinked_string(a, size);
  }else{
    r=begin_wide_shared_string(size,a->size_shift);
    MEMCPY(r->str, a->str, a->len<<a->size_shift);
    free_string(a);
    return r;
  }
}

PMOD_EXPORT struct pike_string *new_realloc_shared_string(struct pike_string *a, INT32 size, int shift)
{
  struct pike_string *r;
  if(shift == a->size_shift) return realloc_shared_string(a,size);

  r=begin_wide_shared_string(size,shift);
  pike_string_cpy(MKPCHARP_STR(r),a);
  free_string(a);
  return r;
}


/* Modify one index in a shared string
 * Not suitable for building new strings or changing multiple characters
 * within a string!
 *
 * Phew, this function become complicated when I inserted magic for wide
 * characters...
 */
PMOD_EXPORT struct pike_string *modify_shared_string(struct pike_string *a,
					 INT32 index,
					 INT32 c)
{
  INT32 old_value;
#ifdef PIKE_DEBUG
  if(index<0 || index>=a->len)
    Pike_fatal("Index out of range in modify_shared_string()\n");
#endif


  old_value=index_shared_string(a,index);
  if(old_value==c) return a;

  /* First test if the string needs to be grown:
   * ie; the new value does not fit in the char size of
   * the old string
   */

  if(min_magnitude(c) > a->size_shift)
  {
    /* String must be grown */
    struct pike_string *b;

    switch(TWO_SIZES(min_magnitude(c),a->size_shift))
    {
      case TWO_SIZES(1,0):
	b=begin_wide_shared_string(a->len,1);
	convert_0_to_1(STR1(b),(p_wchar0 *)a->str,a->len);
	STR1(b)[index]=c;
	free_string(a);
	return end_shared_string(b);

      case TWO_SIZES(2,0):
	b=begin_wide_shared_string(a->len,2);
	convert_0_to_2(STR2(b),(p_wchar0 *)a->str,a->len);
	STR2(b)[index]=c;
	free_string(a);
	return end_shared_string(b);

      case TWO_SIZES(2,1):
	b=begin_wide_shared_string(a->len,2);
	convert_1_to_2(STR2(b),STR1(a),a->len);
	STR2(b)[index]=c;
	free_string(a);
	return end_shared_string(b);

      default:
	Pike_fatal("Odd wide string conversion!\n");
    }
  }


  /* Next we test if the new string can be shrunk
   * if all characters in the new string can fit in a string
   * of a lower magnitude, it must be shrunk
   */
     
  if(min_magnitude(old_value) == a->size_shift &&
     min_magnitude(c) < min_magnitude(old_value))
  {
    /* We *might* need to shrink the string */
    struct pike_string *b;
    int size,tmp;

    switch(a->size_shift)
    {
      case 0:
	Pike_fatal("Unshrinkable!\n");

      case 1:
	/* Test if we *actually* can shrink it.. */
	if(find_magnitude1(STR1(a),index)) break;
	if(find_magnitude1(STR1(a)+index+1,a->len-index-1))
	  break;
	
	b=begin_shared_string(a->len);
	convert_1_to_0((p_wchar0 *)b->str,STR1(a),a->len);
	b->str[index]=c;
	free_string(a);
	return end_shared_string(b);

      case 2:
	/* Test if we *actually* can shrink it.. */
	size=find_magnitude2(STR2(a),index);
	if(size==2) break; /* nope */
	tmp=find_magnitude2(STR2(a)+index+1,a->len-index-1);
	if(tmp==2) break; /* nope */
	size=MAXIMUM(MAXIMUM(size,tmp),min_magnitude(c));

	switch(size)
	{
	  case 0:
	    b=begin_shared_string(a->len);
	    convert_2_to_0((p_wchar0 *)b->str,STR2(a),a->len);
	    b->str[index]=c;
	    free_string(a);
	    return end_shared_string(b);

	  case 1:
	    b=begin_wide_shared_string(a->len,1);
	    convert_2_to_1((p_wchar1 *)b->str,STR2(a),a->len);
	    STR1(b)[index]=c;
	    free_string(a);
	    return end_shared_string(b);
	}
    }
  }
     

  /* We now know that the string has the right character size */
  if(a->refs==1)
  {
    /* One ref - destructive mode */

    if((((unsigned int)index) >= HASH_PREFIX) && (index < a->len-8))
    {
      /* Doesn't change hash value - sneak it in there */
      low_set_index(a,index,c);
      return a;
    }else{
      unlink_pike_string(a);
      low_set_index(a,index,c);
      return end_shared_string(a);
    }
  }else{
    struct pike_string *r;
    r=begin_wide_shared_string(a->len,a->size_shift);
    MEMCPY(r->str, a->str, a->len << a->size_shift);
    low_set_index(r,index,c);
    free_string(a);
    return end_shared_string(r);
  }
}

/*** Add strings ***/
PMOD_EXPORT struct pike_string *add_shared_strings(struct pike_string *a,
					 struct pike_string *b)
{
  struct pike_string *ret;
  PCHARP tmp;
  int target_size=MAXIMUM(a->size_shift,b->size_shift);

  ret=begin_wide_shared_string(a->len+b->len,target_size);
  tmp=MKPCHARP_STR(ret);
  pike_string_cpy(tmp,a);
  INC_PCHARP(tmp,a->len);
  pike_string_cpy(tmp,b);
  return low_end_shared_string(ret);
}

PMOD_EXPORT struct pike_string *add_and_free_shared_strings(struct pike_string *a,
						struct pike_string *b)
{
  ptrdiff_t alen = a->len;
  if(a->size_shift == b->size_shift)
  {
    a = realloc_shared_string(a,alen + b->len);
    MEMCPY(a->str+(alen<<a->size_shift),b->str,b->len<<b->size_shift);
    free_string(b);
    return end_shared_string(a);
  }else{
    struct pike_string *ret=add_shared_strings(a,b);
    free_string(a);
    free_string(b);
    return ret;
  }
}


PMOD_EXPORT ptrdiff_t string_search(struct pike_string *haystack,
				    struct pike_string *needle,
				    ptrdiff_t start)
{
  SearchMojt mojt;
  char *r;

  if(needle->size_shift > haystack->size_shift ||
     start + needle->len > haystack->len)
    return -1;

  if(!needle->len) return start;

  mojt=compile_memsearcher(MKPCHARP_STR(needle),
			   needle->len,
			   haystack->len,
			   needle);

  r = (char *)mojt.vtab->funcN(mojt.data,
			       ADD_PCHARP(MKPCHARP_STR(haystack), start),
			       haystack->len - start).ptr;

  mojt.vtab->freeme(mojt.data);

  if(!r) return -1;
#ifdef PIKE_DEBUG
  if((r < haystack->str) ||
     (r - haystack->str)>>haystack->size_shift > haystack->len)
    Pike_fatal("string_search did a bobo!\n");
#endif
  return (r-haystack->str)>>haystack->size_shift;
}

PMOD_EXPORT struct pike_string *string_slice(struct pike_string *s,
					     ptrdiff_t start,
					     ptrdiff_t len)
{
#ifdef PIKE_DEBUG
  if(start < 0 || len<0 || start+len>s->len )
  {
    Pike_fatal("string_slice, start = %ld, len = %ld, s->len = %ld\n",
	  DO_NOT_WARN((long)start),
	  DO_NOT_WARN((long)len),
	  DO_NOT_WARN((long)s->len));
  }
#endif

  if(start==0 && len==s->len)
  {
    add_ref(s);
    return s;
  }

  switch(s->size_shift)
  {
    case 0:
      return make_shared_binary_string((char *)STR0(s)+start,len);

    case 1:
      return make_shared_binary_string1(STR1(s)+start,len);

    case 2:
      return make_shared_binary_string2(STR2(s)+start,len);
  }
  Pike_fatal("Illegal shift size!\n");
  return 0;
}

/*** replace function ***/
typedef char *(* replace_searchfunc)(void *,void *,size_t);
PMOD_EXPORT struct pike_string *string_replace(struct pike_string *str,
				   struct pike_string *del,
				   struct pike_string *to)
{
  struct pike_string *ret;
  char *s,*tmp,*end;
  PCHARP r;
  int shift;
  SearchMojt mojt;
  replace_searchfunc f = (replace_searchfunc)0;

  if(!str->len)
  {
    add_ref(str);
    return str;
  }

  shift=MAXIMUM(str->size_shift,to->size_shift);

  if(!del->len)
  {
    int e,pos;
    ret=begin_wide_shared_string(str->len + to->len * (str->len -1),shift);
    low_set_index(ret,0,index_shared_string(str,0));
    for(pos=e=1;e<str->len;e++)
    {
      pike_string_cpy(MKPCHARP_STR_OFF(ret,pos),to);
      pos+=to->len;
      low_set_index(ret,pos++,index_shared_string(str,e));
    }
    return end_shared_string(ret);
  }

  s=str->str;
  end=s+(str->len<<str->size_shift);

  if(del->len == to->len)
  {
    mojt=compile_memsearcher(MKPCHARP_STR(del),
			     del->len,
			     str->len,
			     del);
    ret=begin_wide_shared_string(str->len,shift);
    switch(str->size_shift)
    {
      case 0: f=(replace_searchfunc)mojt.vtab->func0; break;
      case 1: f=(replace_searchfunc)mojt.vtab->func1; break;
      case 2: f=(replace_searchfunc)mojt.vtab->func2; break;
#ifdef PIKE_DEBUG
      default: Pike_fatal("Illegal shift.\n");
#endif
    }

  }else{
    INT32 delimeters=0;
    mojt=compile_memsearcher(MKPCHARP_STR(del),
			     del->len,
			     str->len*2,
			     del);

    switch(str->size_shift)
    {
      case 0: f=(replace_searchfunc)mojt.vtab->func0; break;
      case 1: f=(replace_searchfunc)mojt.vtab->func1; break;
      case 2: f=(replace_searchfunc)mojt.vtab->func2; break;
#ifdef PIKE_DEBUG
      default: Pike_fatal("Illegal shift.\n");
#endif
    }

    while((s = f(mojt.data, s, (end-s)>>str->size_shift)))
    {
      delimeters++;
      s+=del->len << str->size_shift;
    }
    
    if(!delimeters)
    {
      mojt.vtab->freeme(mojt.data);
      add_ref(str);
      return str;
    }

    ret=begin_wide_shared_string(str->len + (to->len-del->len)*delimeters, shift);
  }
  s=str->str;
  r=MKPCHARP_STR(ret);

  while((tmp = f(mojt.data, s, (end-s)>>str->size_shift)))
  {
#ifdef PIKE_DEBUG
    if(tmp + (del->len << str->size_shift) > end)
      Pike_fatal("SearchMojt found a match beyond end of string!!!\n");
#endif
    generic_memcpy(r,MKPCHARP(s,str->size_shift),(tmp-s)>>str->size_shift);
    INC_PCHARP(r,(tmp-s)>>str->size_shift);
    pike_string_cpy(r,to);
    INC_PCHARP(r,to->len);
    s=tmp+(del->len << str->size_shift);
  }
  generic_memcpy(r,MKPCHARP(s,str->size_shift),(end-s)>>str->size_shift);

  mojt.vtab->freeme(mojt.data);
  return end_shared_string(ret);
}

/*** init/exit memory ***/
void init_shared_string_table(void)
{
  init_short_pike_string0_blocks();
  init_short_pike_string1_blocks();
  init_short_pike_string2_blocks();
  for(hashprimes_entry=0;hashprimes[hashprimes_entry]<BEGIN_HASH_SIZE;hashprimes_entry++);
  SET_HSIZE(hashprimes_entry);
  base_table=(struct pike_string **)xalloc(sizeof(struct pike_string *)*htable_size);
  MEMSET((char *)base_table,0,sizeof(struct pike_string *)*htable_size);
#ifdef PIKE_RUN_UNLOCKED
  {
    int h;
    bucket_locks=(PIKE_MUTEX_T *)xalloc(sizeof(PIKE_MUTEX_T)*BUCKET_LOCKS);
    for(h=0;h<BUCKET_LOCKS;h++) mt_init(bucket_locks + h);
  }
#endif
  empty_pike_string = make_shared_string("");
}

#ifdef DO_PIKE_CLEANUP
struct shared_string_location *all_shared_string_locations;
#endif


void cleanup_shared_string_table(void)
{
  unsigned INT32 e;
  struct pike_string *s,*next;

  if (empty_pike_string) {
    free_string(empty_pike_string);
    empty_pike_string = 0;
  }

#ifdef DO_PIKE_CLEANUP
  while(all_shared_string_locations)
  {
    struct shared_string_location *x=all_shared_string_locations;
    all_shared_string_locations=x->next;
    free_string(x->s);
    x->s=0;
  }

  if (exit_with_cleanup)
  {
    INT32 num,size;
    count_memory_in_strings(&num,&size);
    if(num)
    {
      fprintf(stderr,"Strings left: %d (%d bytes) (zapped)\n",num,size);
#ifdef PIKE_DEBUG
      dump_stralloc_strings();
#endif
    }
  }
#endif

  for(e=0;e<htable_size;e++)
  {
    LOCK_BUCKET(e);
    for(s=base_table[e];s;s=next)
    {
      next=s->next;
#ifdef REALLY_FREE
      really_free_pike_string(s);
#else
      s->next=0;
#endif
    }
    base_table[e]=0;
    UNLOCK_BUCKET(e);
  }
  free((char *)base_table);
  base_table=0;
  num_strings=0;

#ifdef DO_PIKE_CLEANUP
  free_all_short_pike_string0_blocks();
  free_all_short_pike_string1_blocks();
  free_all_short_pike_string2_blocks();
#endif /* DO_PIKE_CLEANUP */
}

void count_memory_in_strings(INT32 *num, INT32 *size)
{
  unsigned INT32 e, num_=0, size_=0;
  if(!base_table)
  {
    *num=*size=0;
    return;
  }
  size_+=htable_size * sizeof(struct pike_string *);
  for(e=0;e<htable_size;e++)
  {
    struct pike_string *p;
    LOCK_BUCKET(e);
    for(p=base_table[e];p;p=p->next)
    {
      num_++;
      size_+=sizeof(struct pike_string)+(p->len<<p->size_shift);
    }
    UNLOCK_BUCKET(e);
  }
#ifdef PIKE_DEBUG
  if(num_strings != num_)
    Pike_fatal("Num strings is wrong! %d!=%d.\n",num_strings, num_);
#endif
  num[0]=num_;
  size[0]=size_;
}

#ifdef PIKE_DEBUG
unsigned gc_touch_all_strings(void)
{
  unsigned INT32 e;
  unsigned n = 0;
  if (!base_table) return 0;
  for(e=0;e<htable_size;e++)
  {
    struct pike_string *p;
    for(p=base_table[e];p;p=p->next) debug_gc_touch(p), n++;
  }
  return n;
}

void gc_mark_all_strings(void)
{
  unsigned INT32 e;
  if(!base_table) return;
  for(e=0;e<htable_size;e++)
  {
    struct pike_string *p;
    for(p=base_table[e];p;p=p->next) gc_is_referenced(p);
  }
}
#endif

struct pike_string *next_pike_string (struct pike_string *s)
{
  struct pike_string *next = s->next;
  if (!next) {
    size_t h = s->hval;
    do {
      h++;
      LOCK_BUCKET(h);
      h = HMODULO(h);
      next = base_table[h];
      UNLOCK_BUCKET(h);
    } while (!next);
  }
  return next;
}

PMOD_EXPORT void init_string_builder(struct string_builder *s, int mag)
{
  s->malloced=256;
  s->s=begin_wide_shared_string(256,mag);
  s->s->len=0;
  s->s->str[0] = 0;
  s->known_shift=0;
}

PMOD_EXPORT void init_string_builder_alloc(struct string_builder *s, ptrdiff_t length, int mag)
{
  s->malloced=length;
  s->s=begin_wide_shared_string(length,mag);
  s->s->len=0;
  s->s->str[0] = 0;
  s->known_shift=0;
}

PMOD_EXPORT void init_string_builder_copy(struct string_builder *to,
					  struct string_builder *from)
{
  to->malloced = from->malloced;
  to->s = begin_wide_shared_string (from->s->len, from->s->size_shift);
  MEMCPY (to->s->str, from->s->str, from->s->len << from->s->size_shift);
  to->known_shift = from->known_shift;
}

/* str becomes invalid if successful (i.e. nonzero returned),
 * otherwise nothing happens. */
PMOD_EXPORT int init_string_builder_with_string (struct string_builder *s,
						 struct pike_string *str)
{
  if (str->refs == 1 && str->len > SHORT_STRING_THRESHOLD) {
    /* Unlink the string and use it as buffer directly. */
    unlink_pike_string (str);
    s->s = str;
    s->malloced = str->len;
    s->known_shift = str->size_shift;
    return 1;
  }
  return 0;
}

PMOD_EXPORT void string_build_mkspace(struct string_builder *s,
				      ptrdiff_t chars, int mag)
{
  if(mag > s->s->size_shift)
  {
    struct pike_string *n;
    ptrdiff_t l = s->s->len + chars + s->malloced;
    n=begin_wide_shared_string(l,mag);
    pike_string_cpy(MKPCHARP_STR(n),s->s);
    n->len=s->s->len;
    s->s->len = s->malloced;	/* Restore the real length */
    really_free_pike_string(s->s);
    s->malloced=l;
    s->s=n;
  }
  else if(s->s->len+chars > s->malloced)
  {
    ptrdiff_t newlen = MAXIMUM(s->malloced*2,
			       s->s->len + chars);
    ptrdiff_t oldlen = s->s->len;

    s->s->len = s->malloced;	/* Restore the real length */
    s->s = realloc_unlinked_string(s->s, newlen);
    s->s->len = oldlen;
    s->malloced = newlen;
  }
}

PMOD_EXPORT void *string_builder_allocate(struct string_builder *s, ptrdiff_t chars, int mag)
{
  void *ret;
  string_build_mkspace(s, chars, mag);
  if(chars<0) s->known_shift=0;
  ret = s->s->str + (s->s->len<<s->s->size_shift);
  s->s->len += chars;
  return ret;
}

PMOD_EXPORT void string_builder_putchar(struct string_builder *s, int ch)
{
  ptrdiff_t i;
  int mag = min_magnitude(ch);

  if (mag > s->s->size_shift) {
    string_build_mkspace(s, 1, mag);
    s->known_shift = (size_t)mag;
  } else if (((size_t)s->s->len) >= ((size_t)s->malloced)) {
    string_build_mkspace(s, 1, mag);
    s->known_shift = MAXIMUM((size_t)mag, s->known_shift);
  }
  i = s->s->len++;
  low_set_index(s->s,i,ch);
  /* Ensure NUL-termination */
  s->s->str[s->s->len << s->s->size_shift] = 0;
}


PMOD_EXPORT void string_builder_binary_strcat(struct string_builder *s,
					      const char *str, ptrdiff_t len)
{
  string_build_mkspace(s,len,0);
  switch(s->s->size_shift)
  {
    case 0: convert_0_to_0(STR0(s->s)+s->s->len,(p_wchar0 *)str,len); break;
    case 1: convert_0_to_1(STR1(s->s)+s->s->len,(p_wchar0 *)str,len); break;
    case 2: convert_0_to_2(STR2(s->s)+s->s->len,(p_wchar0 *)str,len); break;
    default:
      Pike_fatal("Illegal magnitude!\n");
  }
  s->s->len+=len;
  /* Ensure NUL-termination */
  s->s->str[s->s->len << s->s->size_shift] = 0;
}


PMOD_EXPORT void string_builder_append(struct string_builder *s,
				       PCHARP from,
				       ptrdiff_t len)
{
  int shift;
  if ((shift = from.shift) > s->s->size_shift) {
    if (shift == 1) {
      shift = find_magnitude1((p_wchar1 *)from.ptr, len);
    } else {
      shift = find_magnitude2((p_wchar2 *)from.ptr, len);
    }
  }
  string_build_mkspace(s, len, shift);
  generic_memcpy(MKPCHARP_STR_OFF(s->s,s->s->len), from, len);
  s->s->len+=len;
  /* Ensure NUL-termination */
  s->s->str[s->s->len << s->s->size_shift] = 0;
}

PMOD_EXPORT void string_builder_fill(struct string_builder *s,
				     ptrdiff_t howmany,
				     PCHARP from,
				     ptrdiff_t len,
				     ptrdiff_t offset)
{
  ptrdiff_t tmp;
  int shift;

#ifdef PIKE_DEBUG
  if(len<=0)
    Pike_fatal("Cannot fill with zero length strings!\n");
#endif
  if(howmany<=0) return;

  if(!s->s->size_shift &&
     len == 1 &&
     (!from.shift || !min_magnitude(EXTRACT_PCHARP(from))))
  {
    MEMSET(string_builder_allocate(s,howmany,0),
	   EXTRACT_PCHARP(from),
	   howmany);
    /* Ensure NUL-termination */
    s->s->str[s->s->len << s->s->size_shift] = 0;
    return;
  }

  if ((shift = from.shift) > s->s->size_shift) {
    /* Check if we really need the extra magnitude. */
    /* FIXME: What about offset? */
    if (shift == 1) {
      shift = find_magnitude1((p_wchar1 *)from.ptr, len);
    } else {
      shift = find_magnitude2((p_wchar2 *)from.ptr, len);
    }
  }

  string_build_mkspace(s, howmany, shift);
  tmp = MINIMUM(howmany, len - offset);

  generic_memcpy(MKPCHARP_STR_OFF(s->s,s->s->len),
		 ADD_PCHARP(from,offset),
		 tmp);
  s->s->len+=tmp;
  howmany-=tmp;
  if(howmany > 0)
  {
    PCHARP to;
    tmp=MINIMUM(howmany, len);
    to=MKPCHARP_STR_OFF(s->s,s->s->len);
    generic_memcpy(to,from, tmp);
    s->s->len+=tmp;
    howmany-=tmp;

    while(howmany > 0)
    {
      tmp = MINIMUM(len, howmany);
      MEMCPY(s->s->str + (s->s->len << s->s->size_shift),
	     to.ptr,
	     tmp << s->s->size_shift);
      len+=tmp;
      howmany-=tmp;
      s->s->len+=tmp;
    }
  }
  /* Ensure NUL-termination */
  s->s->str[s->s->len << s->s->size_shift] = 0;
}

PMOD_EXPORT void string_builder_strcat(struct string_builder *s, char *str)
{
  string_builder_binary_strcat(s,str,strlen(str));
}

PMOD_EXPORT void string_builder_shared_strcat(struct string_builder *s, struct pike_string *str)
{
  string_build_mkspace(s,str->len,str->size_shift);

  pike_string_cpy(MKPCHARP_STR_OFF(s->s,s->s->len), str);
  s->known_shift=MAXIMUM(s->known_shift,(size_t)str->size_shift);
  s->s->len+=str->len;
  /* Ensure NUL-termination */
  s->s->str[s->s->len << s->s->size_shift] = 0;
}

PMOD_EXPORT void string_builder_append_integer(struct string_builder *s,
					       LONGEST val,
					       unsigned int base,
					       int flags,
					       size_t min_width,
					       size_t precision)
{
  unsigned LONGEST tmp;
  size_t len = 1;
  const char *numbers = "0123456789abcdef";
  if ((base < 2) || (base > 16)) {
    Pike_fatal("string_builder_append_int(): Unsupported base %u.\n", base);
  }
  if (flags & APPEND_UPPER_CASE) {
    numbers = "0123456789ABCDEF";
  }
  if ((flags & APPEND_SIGNED) && (val < 0)) {
    string_builder_putchar(s, '-');
    val = -val;
  } else if (flags & APPEND_POSITIVE) {
    string_builder_putchar(s, '+');
  }
  if ((flags & APPEND_ZERO_PAD) && (precision < min_width)) {
    precision = min_width;
  }

  tmp = val;
  if (base & (base - 1)) {
    /* Calculate the output length.
     * Use do-while to ensure that zero isn't output as an empty string.
     */
    len = 0;
    do {
      len++;
      tmp /= base;
    } while (tmp);

    /* Precision is minimum number of digits. */
    if (len < precision) len = precision;

    /* Perform padding. */
    if ((len < min_width) && !(flags & APPEND_LEFT)) {
      string_builder_fill(s, min_width - len, MKPCHARP("    ", 0),
			  4, 0);
      min_width = 0;
    }

    tmp = val;
    switch(s->s->size_shift) {
    case 0:
      {
	p_wchar0 *p = string_builder_allocate(s, len, 0);
	do {
	  p[--len] = numbers[tmp%base];
	  tmp /= base;
	} while (len);
      }
      break;
    case 1:
      {
	p_wchar1 *p = string_builder_allocate(s, len, 0);
	do {
	  p[--len] = numbers[tmp%base];
	  tmp /= base;
	} while (len);
      }
      break;
    case 2:
      {
	p_wchar2 *p = string_builder_allocate(s, len, 0);
	do {
	  p[--len] = numbers[tmp%base];
	  tmp /= base;
	} while (len);
      }
      break;
    }
  } else {
    /* base is a power of two, so we can do without
     * the division and modulo operations.
     */
    int delta;
    size_t shift;
    unsigned int mask;

    for(delta = 1; (base>>delta) > 1; delta++)
      ;

    mask = (1<<delta)-1;	/* Usually base-1. */

    /* Precision is minimum number of digits. */
    if (precision) shift = (len = precision) * delta;
    else shift = delta;

    /* Calculate actual number of digits and initial shift. */
    for (; tmp >> shift; shift += delta, len++)
      ;

    if ((len < min_width) && !(flags & APPEND_LEFT)) {
      /* Perform padding.
       * Note that APPEND_ZERO_PAD can not be active here, since
       * len is at least min_width in that case.
       */
      string_builder_fill(s, min_width - len, MKPCHARP("    ", 0),
			  4, 0);
      min_width = 0;
    }

    while(shift) {
      shift -= delta;
      string_builder_putchar(s, numbers[(tmp>>shift) & mask]);
    }
  }
  if (len < min_width) {
    /* Perform padding.
     * Note that APPEND_ZERO_PAD can not be active here, since
     * len is at least min_width in that case.
     * Note that APPEND_LEFT is always active here, since
     * min_width isn't zero.
     */
    string_builder_fill(s, min_width - len, MKPCHARP("    ", 0),
			4, 0);
  }
}

/* Values used internally in string_builder_vsprintf() */
#define STATE_MIN_WIDTH	1
#define STATE_PRECISION 2

static LONGEST pike_va_int(va_list *args, int flags)
{
  switch (flags & (APPEND_WIDTH_MASK|APPEND_SIGNED)) {
  case APPEND_WIDTH_HALF:
    return va_arg(*args, unsigned int) & 0xffff;
  case APPEND_WIDTH_HALF|APPEND_SIGNED:
    return (short)va_arg(*args, int);
  case 0:
    return va_arg(*args, unsigned int);
  case APPEND_SIGNED:
    return va_arg(*args, int);
  case APPEND_WIDTH_LONG:
    return va_arg(*args, unsigned long);
  case APPEND_WIDTH_LONG|APPEND_SIGNED:
    return va_arg(*args, long);
#ifdef INT64
  case APPEND_WIDTH_LONG_LONG:
    return va_arg(*args, unsigned INT64);
  case APPEND_WIDTH_LONG_LONG|APPEND_SIGNED:
    return va_arg(*args, INT64);
#endif /* INT64 */
  }
  Pike_fatal("string_builder_append_integerv(): Unsupported flags: 0x%04x\n",
	     flags);
  return 0;
}

PMOD_EXPORT void string_builder_vsprintf(struct string_builder *s,
					 const char *fmt,
					 va_list args)
{
  while (*fmt) {
    if (*fmt == '%') {
      int flags = 0;
      size_t min_width = 0;
      size_t precision = 0;
      int state = 0;
      
      fmt++;
      while (1) {
	switch (*(fmt++)) {
	case '%':
	  string_builder_putchar(s, '%');
	  break;

	case '+':
	  flags |= APPEND_POSITIVE;
	  continue;
	case '-':
	  flags |= APPEND_LEFT;
	  continue;

	case '0':
	  if (!state) {
	    flags |= APPEND_ZERO_PAD;
	  }
	  /* FALL_THROUGH */
	case '1': case '2': case '3':
	case '4': case '5': case '6':
	case '7': case '8': case '9':
	  if (state == STATE_PRECISION) {
	    precision = precision * 10 + fmt[-1] - '0';
	  } else {
	    state = STATE_MIN_WIDTH;
	    min_width = min_width * 10 + fmt[-1] - '0';
	  }
	  continue;

	case '.':
	  state = STATE_PRECISION;
	  continue;

	case 'h':
	  flags |= APPEND_WIDTH_HALF;
	  continue;

	case 'w':	/* Same as l, but old-style, and only for %s. */
	case 'l':
	  if (flags & APPEND_WIDTH_LONG) {
	    flags |= APPEND_WIDTH_LONG_LONG;
	  } else {
	    flags |= APPEND_WIDTH_LONG;
	  }
	  continue;
	  
	case 'O':
	  {
	    /* FIXME: Doesn't care about field or integer widths yet. */
	    dynamic_buffer old_buf;
	    init_buf(&old_buf);
	    describe_svalue(va_arg(args, struct svalue *), 0, NULL);
	    string_builder_binary_strcat(s, pike_global_buffer.s.str,
					 pike_global_buffer.s.len);
	    toss_buffer(&pike_global_buffer);
	    restore_buffer(&old_buf);
	  }
	  break;
	case 'S':
	  /* Note: On some platforms this is an alias for %ls, so if you
	   *       want to output wchar_t strings, use %ls instead!
	   */
	  {
	    struct pike_string *str = va_arg(args, struct pike_string *);
	    size_t len = str->len;
	    if (precision && (precision < len)) len = precision;
	    if (min_width > len) {
	      if (flags & APPEND_LEFT) {
		string_builder_append(s, MKPCHARP_STR(str), len);
		string_builder_fill(s, min_width - len, MKPCHARP("    ", 0),
				    4, 0);
	      } else {
		string_builder_fill(s, min_width - len, MKPCHARP("    ", 0),
				    4, 0);
		string_builder_append(s, MKPCHARP_STR(str), len);
	      }
	    } else {
	      string_builder_append(s, MKPCHARP_STR(str), len);
	    }
	  }
	  break;
	case 's':
	  if (flags & APPEND_WIDTH_LONG) {
	    /* Wide string: %ws, %ls or %lls
	     */
	    PCHARP str;
	    size_t len;
	    if ((flags & APPEND_WIDTH_LONG)== APPEND_WIDTH_LONG) {
	      str = MKPCHARP(va_arg(args, p_wchar1 *), 1);
	    } else {
	      str = MKPCHARP(va_arg(args, p_wchar2 *), 2);
	    }
	    len = pcharp_strlen(str);
	    if (precision && precision < len) len = precision;
	    if (min_width > len) {
	      if (flags & APPEND_LEFT) {
		string_builder_append(s, str, len);
		string_builder_fill(s, min_width - len, MKPCHARP("    ", 0),
				    4, 0);
	      } else {
		string_builder_fill(s, min_width - len, MKPCHARP("    ", 0),
				    4, 0);
		string_builder_append(s, str, len);
	      }
	    } else {
	      string_builder_append(s, str, len);
	    }
	  } else {
	    const char *str = va_arg(args, char *);
	    size_t len = strlen(str);
	    if (precision && precision < len) len = precision;
	    if (min_width > len) {
	      if (flags & APPEND_LEFT) {
		string_builder_binary_strcat(s, str, len);
		string_builder_fill(s, min_width - len, MKPCHARP("    ", 0),
				    4, 0);
	      } else {
		string_builder_fill(s, min_width - len, MKPCHARP("    ", 0),
				    4, 0);
		string_builder_binary_strcat(s, str, len);
	      }
	    } else {
	      string_builder_binary_strcat(s, str, len);
	    }
	  }
	  break;
	case 'c':
	  /* FIXME: Doesn't care about field or integer widths yet. */
	  string_builder_putchar(s, va_arg(args, int));
	  break;
	case 'b':
	  string_builder_append_integer(s, pike_va_int(&args, flags), 2,
					flags, min_width, precision);
	  break;
	case 'o':
	  string_builder_append_integer(s, pike_va_int(&args, flags), 8,
					flags, min_width, precision);
	  break;
	case 'x':
	  string_builder_append_integer(s, pike_va_int(&args, flags), 16,
					flags, min_width, precision);
	  break;
	case 'X':
	  string_builder_append_integer(s, pike_va_int(&args, flags), 16,
					flags | APPEND_UPPER_CASE,
					min_width, precision);
	  break;
	case 'u':
	  string_builder_append_integer(s, pike_va_int(&args, flags), 10,
					flags, min_width, precision);
	  break;
	case 'd':
	  string_builder_append_integer(s, pike_va_int(&args, flags), 10,
					flags | APPEND_SIGNED,
					min_width, precision);
	  break;

	  /* FIMXE: TODO: Doubles (ie 'a', 'e', 'E', 'f', 'g', 'G'). */

	default:
	  Pike_fatal("string_builder_vsprintf(): Invalid formatting method: "
		     "'%c' 0x%x.\n", fmt[-1], fmt[-1]);
	}
	break;
      }
    } else {
      const char *start = fmt;
      while (*fmt && (*fmt != '%'))
	fmt++;
      string_builder_binary_strcat(s, start, fmt-start);
    }
  }
}

PMOD_EXPORT void string_builder_sprintf(struct string_builder *s,
					const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  string_builder_vsprintf(s, fmt, args);
  va_end(args);
}


PMOD_EXPORT void reset_string_builder(struct string_builder *s)
{
  s->known_shift=0;
  s->s->len=0;
  /* Ensure NUL-termination */
  s->s->str[0] = 0;
}

PMOD_EXPORT void free_string_builder(struct string_builder *s)
{
  s->s->len = s->malloced;
  really_free_pike_string(s->s);
}

PMOD_EXPORT struct pike_string *finish_string_builder(struct string_builder *s)
{
  /* Ensure NUL-termination */
  low_set_index(s->s,s->s->len,0);
  if (s->s->len <= SHORT_STRING_THRESHOLD) {
    ptrdiff_t len = s->s->len;
    s->s->len = s->malloced;
    s->s = realloc_unlinked_string(s->s, len);
  }
  if(s->known_shift == (size_t)s->s->size_shift)
    return low_end_shared_string(s->s);
  return end_shared_string(s->s);
}

PMOD_EXPORT PCHARP MEMCHR_PCHARP(PCHARP ptr, int chr, ptrdiff_t len)
{
  switch(ptr.shift)
  {
    case 0: return MKPCHARP(MEMCHR0(ptr.ptr,chr,len),0);
    case 1: return MKPCHARP(MEMCHR1((p_wchar1 *)ptr.ptr,chr,len),1);
    case 2: return MKPCHARP(MEMCHR2((p_wchar2 *)ptr.ptr,chr,len),2);
  }
  Pike_fatal("Illegal shift in MEMCHR_PCHARP.\n");
  return MKPCHARP(0,0); /* make wcc happy */
}

#define DIGIT(x)	(WIDE_ISDIGIT(x) ? (x) - '0' : \
			 WIDE_ISLOWER(x) ? (x) + 10 - 'a' : (x) + 10 - 'A')
#define MBASE	('z' - 'a' + 1 + 10)

PMOD_EXPORT long STRTOL_PCHARP(PCHARP str, PCHARP *ptr, int base)
{
  /* Note: Code duplication in STRTOL and pcharp_to_svalue_inumber. */

  unsigned long val, mul_limit;
  int c;
  int xx, neg = 0, add_limit, overflow = 0;

  if (ptr)  *ptr = str;
  if (base < 0 || base > MBASE)  return 0;
  if (!WIDE_ISALNUM(c = EXTRACT_PCHARP(str)))
  {
    while (WIDE_ISSPACE(c))
    {
      INC_PCHARP(str,1);
      c=EXTRACT_PCHARP(str);
    }
    switch (c)
    {
    case '-':
      neg++;
    case '+':			/* fall-through */
      INC_PCHARP(str,1);
      c=EXTRACT_PCHARP(str);
    }
  }

  if (!base)
  {
    if (c != '0')
      base = 10;
    else if (INDEX_PCHARP(str,1) == 'x' || INDEX_PCHARP(str,1) == 'X')
      base = 16;
    else
      base = 8;
  }

  if (!WIDE_ISALNUM(c) || (xx = DIGIT(c)) >= base)
    return 0;			/* no number formed */
  if (base == 16 && c == '0' && isxdigit(INDEX_PCHARP(str,2)) &&
      (INDEX_PCHARP(str,1) == 'x' || INDEX_PCHARP(str,1) == 'X'))
  {
    INC_PCHARP(str,2);
    c = EXTRACT_PCHARP(str);		/* skip over leading "0x" or "0X" */
  }

  mul_limit = ((unsigned long)LONG_MAX)/base;
  add_limit = (int) (LONG_MAX % base);
  if (neg) {
    if (++add_limit == base) {
      add_limit = 0;
      mul_limit++;
    }
  }

  val=DIGIT(c);
  while(1)
  {
    INC_PCHARP(str,1);
    c=EXTRACT_PCHARP(str);
    if(!(WIDE_ISALNUM(c)  && (xx=DIGIT(c)) < base)) break;
    if (val > mul_limit || (val == mul_limit && xx > add_limit)) {
      overflow = 1;
    } else
      val = base * val + xx;
  }

  if (ptr) *ptr = str;
  if (overflow) {
    errno = ERANGE;
    return neg ? LONG_MIN : LONG_MAX;
  }
  else {
    if (neg)
      return (long)(~val + 1);
    else
      return (long) val;
  }
}

PMOD_EXPORT int string_to_svalue_inumber(struct svalue *r,
			     char * str,
			     char **ptr,
			     int base,
			     int maxlength)
{
  PCHARP tmp;
  int ret=pcharp_to_svalue_inumber(r,
				   MKPCHARP(str,0),
				   &tmp,
				   base,
				   maxlength);
  if(ptr) *ptr=(char *)tmp.ptr;
  return ret;
}

PMOD_EXPORT int wide_string_to_svalue_inumber(struct svalue *r,
					      void * str,
					      void *ptr,
					      int base,
					      ptrdiff_t maxlength,
					      int shift)
{
  PCHARP tmp;
  int ret=pcharp_to_svalue_inumber(r,
				   MKPCHARP(str,shift),
				   &tmp,
				   base,
				   maxlength);
  if(ptr) *(p_wchar0 **)ptr=tmp.ptr;
  return ret;
}

PMOD_EXPORT int pcharp_to_svalue_inumber(struct svalue *r,
					 PCHARP str,
					 PCHARP *ptr,
					 int base,
					 ptrdiff_t maxlength)
{
  /* Note: Code duplication in STRTOL and STRTOL_PCHARP. */

  PCHARP str_start;
  
  unsigned INT_TYPE val, mul_limit;
  int c;
  int xx, neg = 0, add_limit, overflow = 0;

  maxlength--;   /* max_length <= 0 means no max length. */
  str_start = str;

  /* In case no number is formed. */
  r->type = T_INT;
  r->subtype = NUMBER_NUMBER;
  r->u.integer = 0;
  if(ptr != 0)
    *ptr = str;
  
  if(base < 0 || MBASE < base)
    return 0;
  
  if(!WIDE_ISALNUM(c = EXTRACT_PCHARP(str)))
  {
    while(WIDE_ISSPACE(c))
    {
      INC_PCHARP(str,1);
      c = EXTRACT_PCHARP(str);
    }
    
    switch (c)
    {
    case '-':
      neg++;
      /* Fall-through. */
    case '+':
      INC_PCHARP(str,1);
      c = EXTRACT_PCHARP(str);
    }
  }
  
  if(base == 0)
  {
    if(c != '0')
      base = 10;
    else if(INDEX_PCHARP(str,1) == 'x' || INDEX_PCHARP(str,1) == 'X')
      base = 16;
    else if(INDEX_PCHARP(str,1) == 'b' || INDEX_PCHARP(str,1) == 'B')
      base = 2;
    else
      base = 8;
  }
  
  /*
   * For any base > 10, the digits incrementally following
   * 9 are assumed to be "abc...z" or "ABC...Z".
   */
  if(!WIDE_ISALNUM(c) || (xx = DIGIT(c)) >= base)
    return 0;   /* No number formed. */
  
  if((base==16 || base == 2) && c == '0' &&
     INDEX_PCHARP(str,2) < 256 && /* Don't trust isxdigit... */
     isxdigit(INDEX_PCHARP(str,2)) &&
     ((base==16 && (INDEX_PCHARP(str,1)=='x' || INDEX_PCHARP(str,1)=='X')) ||
      (base==2 && (INDEX_PCHARP(str,1)=='b' || INDEX_PCHARP(str,1)=='B'))))
  {
    /* Skip over leading "0x", "0X", "0b" or "0B". */
    INC_PCHARP(str,2);
    c=EXTRACT_PCHARP(str);
  }
  str_start=str;

  if (neg) {
    mul_limit = (unsigned INT_TYPE) MIN_INT_TYPE / base;
    add_limit = (int) ((unsigned INT_TYPE) MIN_INT_TYPE % base);
  }
  else {
    mul_limit = MAX_INT_TYPE / base;
    add_limit = (int) (MAX_INT_TYPE % base);
  }
  
  for(val = DIGIT(c);
      (INC_PCHARP(str,1), WIDE_ISALNUM(c = EXTRACT_PCHARP(str) )) &&
	(xx = DIGIT(c)) < base &&
	0 != maxlength--; )
  {
    if (val > mul_limit || (val == mul_limit && xx > add_limit))
      overflow = 1;
    else
      val = base * val + xx;
  }
  
  if(ptr != 0)
    *ptr = str;

  if (overflow) {
#ifdef AUTO_BIGNUM
    push_string(make_shared_binary_pcharp(str_start,
					  SUBTRACT_PCHARP(str,str_start)));
    /* Note that this can conceivably throw errors()
     * in some situations that might not be desirable...
     * take care.
     * /Hubbe
     *
     * It could probably also be faster...
     */
    push_int(base);
    convert_stack_top_with_base_to_bignum();
    if(neg) o_negate();
    
    *r = *--Pike_sp;
    dmalloc_touch_svalue (r);
#else  /* !AUTO_BIGNUM */
    r->u.integer = neg ? MIN_INT_TYPE : MAX_INT_TYPE;
#endif
  }
  else {
    if (neg)
      r->u.integer = val > (unsigned INT_TYPE) MAX_INT_TYPE ?
	-(INT_TYPE) (val - (unsigned INT_TYPE) MAX_INT_TYPE) - MAX_INT_TYPE :
	-(INT_TYPE) val;
    else
      r->u.integer = (INT_TYPE) val;
  }

  return 1;
}

PMOD_EXPORT int convert_stack_top_string_to_inumber(int base)
{
  struct svalue r;
  int i;

  if(Pike_sp[-1].type != T_STRING)
    Pike_error("Cannot convert stack top to integer number.\n");
  
  i=pcharp_to_svalue_inumber(&r, MKPCHARP_STR(Pike_sp[-1].u.string), 0, base, 0);
  
  free_string(Pike_sp[-1].u.string);
  Pike_sp[-1] = r;

  return i;
}

/* Convert PCHARP to a double.  If ENDPTR is not NULL, a pointer to the
   character after the last one used in the number is put in *ENDPTR.  */
PMOD_EXPORT double STRTOD_PCHARP(PCHARP nptr, PCHARP *endptr)
{
  /* Note: Code duplication in STRTOD. */

  register PCHARP s;
  short int sign;

  /* The number so far.  */
  double num;

  int got_dot;      /* Found a decimal point.  */
  int got_digit;    /* Seen any digits.  */

  /* The exponent of the number.  */
  long int exponent;

  if (nptr.ptr == NULL)
  {
    errno = EINVAL;
    goto noconv;
  }

  s = nptr;

  /* Eat whitespace.  */
  while (EXTRACT_PCHARP(s) <256 && WIDE_ISSPACE(EXTRACT_PCHARP(s))) INC_PCHARP(s,1);

  /* Get the sign.  */
  sign = EXTRACT_PCHARP(s) == '-' ? -1 : 1;
  if (EXTRACT_PCHARP(s) == '-' || EXTRACT_PCHARP(s) == '+')
    INC_PCHARP(s,1);

  num = 0.0;
  got_dot = 0;
  got_digit = 0;
  exponent = 0;
  for (;; INC_PCHARP(s,1))
  {
    if (EXTRACT_PCHARP(s)<256 && WIDE_ISDIGIT (EXTRACT_PCHARP(s)))
    {
      got_digit = 1;

      /* Make sure that multiplication by 10 will not overflow.  */
      if (num > DBL_MAX * 0.1)
	/* The value of the digit doesn't matter, since we have already
	   gotten as many digits as can be represented in a `double'.
	   This doesn't necessarily mean the result will overflow.
	   The exponent may reduce it to within range.
	   
	   We just need to record that there was another
	   digit so that we can multiply by 10 later.  */
	++exponent;
      else
	num = (num * 10.0) + (EXTRACT_PCHARP(s) - '0');

      /* Keep track of the number of digits after the decimal point.
	 If we just divided by 10 here, we would lose precision.  */
      if (got_dot)
	--exponent;
    }
    else if (!got_dot && (char) EXTRACT_PCHARP(s) == '.')
      /* Record that we have found the decimal point.  */
      got_dot = 1;
    else
      /* Any other character terminates the number.  */
      break;
  }

  if (!got_digit)
    goto noconv;

  if (EXTRACT_PCHARP(s) <256 && tolower(EXTRACT_PCHARP(s)) == 'e')
    {
      /* Get the exponent specified after the `e' or `E'.  */
      int save = errno;
      PCHARP end;
      long int exp;

      errno = 0;
      INC_PCHARP(s,1);
      exp = STRTOL_PCHARP(s, &end, 10);
      if (errno == ERANGE)
      {
	/* The exponent overflowed a `long int'.  It is probably a safe
	   assumption that an exponent that cannot be represented by
	   a `long int' exceeds the limits of a `double'.  */
	/* NOTE: Don't trust the value returned from STRTOL.
	 * We need to find the sign of the exponent by hand.
	 */
	p_wchar2 c;
	while(WIDE_ISSPACE(c = EXTRACT_PCHARP(s))) {
	  INC_PCHARP(s, 1);
	}
	if (endptr != NULL)
	  *endptr = end;
	if (c == '-')
	  goto underflow;
	else
	  goto overflow;
      }
      else if (COMPARE_PCHARP(end,==,s))
	/* There was no exponent.  Reset END to point to
	   the 'e' or 'E', so *ENDPTR will be set there.  */
	end = ADD_PCHARP(s,-1);
      errno = save;
      s = end;
      exponent += exp;
    }

  if(got_dot && INDEX_PCHARP(s,-1)=='.') INC_PCHARP(s,-1);
  if (endptr != NULL)
    *endptr = s;

  if (num == 0.0)
    return 0.0;

  /* Multiply NUM by 10 to the EXPONENT power,
     checking for overflow and underflow.  */

  if (exponent < 0)
  {
    if (num < DBL_MIN * pow(10.0, (double) -exponent))
      goto underflow;
  }
  else if (exponent > 0)
  {
    if (num > DBL_MAX * pow(10.0, (double) -exponent))
      goto overflow;
  }

  if(exponent < 0 && exponent >-100) /* make sure we don't underflow */
    num /= pow(10.0, (double) -exponent);
  else
    num *= pow(10.0, (double) exponent);

  return num * sign;

 overflow:
  /* Return an overflow error.  */
  errno = ERANGE;
  return HUGE_VAL * sign;

 underflow:
  /* Return an underflow error.  */
#if 0
  if (endptr != NULL)
    *endptr = nptr;
#endif
  errno = ERANGE;
  return 0.0;
  
 noconv:
  /* There was no number.  */
  if (endptr != NULL)
    *endptr = nptr;
  return 0.0;
}


PMOD_EXPORT p_wchar0 *require_wstring0(struct pike_string *s,
			   char **to_free)
{
  switch(s->size_shift)
  {
    case 0:
      *to_free=0;
      return STR0(s);
    case 1:
    case 2:
      return 0;

    default:
      Pike_fatal("Illegal shift size in string.\n");
  }
  return 0;
}

PMOD_EXPORT p_wchar1 *require_wstring1(struct pike_string *s,
			   char **to_free)
{
  switch(s->size_shift)
  {
    case 0:
      *to_free=xalloc((s->len+1)*2);
      convert_0_to_1((p_wchar1 *)*to_free, STR0(s),s->len+1);
      return (p_wchar1 *)*to_free;

    case 1:
      *to_free=0;
      return STR1(s);

    case 2:
      return 0;

    default:
      Pike_fatal("Illegal shift size in string.\n");
  }
  return 0;
}


PMOD_EXPORT p_wchar2 *require_wstring2(struct pike_string *s,
			   char **to_free)
{
  switch(s->size_shift)
  {
    case 0:
      *to_free=xalloc((s->len+1)*4);
      convert_0_to_2((p_wchar2 *)*to_free, STR0(s),s->len+1);
      return (p_wchar2 *)*to_free;

    case 1:
      *to_free=xalloc((s->len+1)*4);
      convert_1_to_2((p_wchar2 *)*to_free, STR1(s),s->len+1);
      return (p_wchar2 *)*to_free;

    case 2:
      *to_free=0;
      return STR2(s);

    default:
      Pike_fatal("Illegal shift size in string.\n");
  }
  return 0;
}
