/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#include "stralloc.h"
#include "pike_macros.h"
#include "dynamic_buffer.h"
#include "pike_macros.h"
#include "pike_memory.h"
#include "error.h"
#include "gc.h"
#include "stuff.h"

#include <ctype.h>

RCSID("$Id: stralloc.c,v 1.40 1998/10/11 11:18:53 hubbe Exp $");

#define BEGIN_HASH_SIZE 997
#define MAX_AVG_LINK_LENGTH 3
#define HASH_PREFIX 64

unsigned INT32 htable_size=0;
static unsigned int hashprimes_entry=0;
static struct pike_string **base_table=0;
static unsigned INT32 full_hash_value;
unsigned INT32 num_strings=0;

/*** Main string hash function ***/


#define StrHash(s,len) low_do_hash(s,len,0)

static unsigned int low_do_hash(const void *s, int len, int size_shift)
{
  full_hash_value=hashmem(s,len<<size_shift,HASH_PREFIX<<size_shift);
  return full_hash_value % htable_size;
}

static INLINE unsigned int do_hash(struct pike_string *s)
{
  return low_do_hash(s->str, s->len, s->size_shift);
}


static INLINE int find_magnitude1(const unsigned INT16 *s, int len)
{
  while(--len>=0)
    if(s[len]>=256)
      return 1;
  return 0;
}

static INLINE int find_magnitude2(const unsigned INT32 *s, int len)
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

static INLINE unsigned INT32 generic_extract (const void *str, int size, int pos)
{
  switch(size)
  {
    case 0: return ((unsigned char *)str)[pos];
    case 1: return ((unsigned INT16 *)str)[pos];
    case 2: return ((unsigned INT32 *)str)[pos];
    default:
      fatal("Illegal shift size!\n");
  }
}

INLINE unsigned INT32 index_shared_string(struct pike_string *s, int pos)
{
#ifdef DEBUG
  if(pos > s->len || pos<0)
    fatal("string index out of range!\n");
#endif
  return generic_extract(s->str,s->size_shift,pos);
}

INLINE void low_set_index(struct pike_string *s, int pos, int value)
{
#ifdef DEBUG
  if(pos > s->len || pos<0)
    fatal("string index out of range!\n");
  
  if(pos == s->len && value)
    fatal("string zero termination foul!\n");
#endif
  switch(s->size_shift)
  {
    case 0: STR0(s)[pos]=value; break;
    case 1: STR1(s)[pos]=value; break;
    case 2: STR2(s)[pos]=value; break;
    default:
      fatal("Illegal shift size!\n");
  }
}

#ifdef DEBUG
struct INLINE pike_string *debug_check_size_shift(struct pike_string *a,int shift)
{
  if(a->size_shift != shift)
    fatal("Wrong STRX macro used!\n");
  return a;
}
#endif

#define CONVERT(FROM,TO) \
INLINE void PIKE_CONCAT4(convert_,FROM,_to_,TO)(PIKE_CONCAT(p_wchar,TO) *to, const PIKE_CONCAT(p_wchar,FROM) *from, int len) \
{  while(--len>=0) *(to++)=*(from++); } \
INLINE INT32 PIKE_CONCAT4(compare_,FROM,_to_,TO)(const PIKE_CONCAT(p_wchar,TO) *to, const PIKE_CONCAT(p_wchar,FROM) *from, int len) \
{ int tmp; while(--len>=0) if((tmp=*(to++)-*(from++))) return tmp; return 0; }


CONVERT(0,1)
CONVERT(0,2)
CONVERT(1,0)
CONVERT(1,2)
CONVERT(2,0)
CONVERT(2,1)

#ifdef DEBUG
#ifdef DEBUG_MALLOC
#define DM(X) X
#else
#define DM(X)
#endif

int generic_compare_strings(const void *a,int alen, int asize,
			    const void *b,int blen, int bsize)
{
#define TWO_SIZES(X,Y) (((X)<<2)+(Y))
  if(alen != blen) return 0;
  if(asize==bsize)
  {
    return !MEMCPY(a,b,alen<<asize);
  }else{
    INT32 pos;
    for(pos=0;pos< alen ;pos++)
      if(generic_extract(a,asize,pos) != generic_extract(b,bsize,pos))
	return 0;
    return 1;
  }
}


void generic_memcpy(void *to, int to_shift,
		    void *from, int from_shift,
		    int len)
{
  switch(TWO_SIZES(from_shift,to_shift))
  {
    case TWO_SIZES(0,0):
      convert_0_to_0((p_wchar0 *)to,(p_wchar0 *)from,len);
      break;
    case TWO_SIZES(0,1):
      convert_0_to_1((p_wchar1 *)to,(p_wchar0 *)from,len);
      break;
    case TWO_SIZES(0,2):
      convert_0_to_2((p_wchar2 *)to,(p_wchar0 *)from,len);
      break;

    case TWO_SIZES(1,0):
      convert_1_to_0((p_wchar0 *)to,(p_wchar1 *)from,len);
      break;
    case TWO_SIZES(1,1):
      convert_1_to_1((p_wchar1 *)to,(p_wchar1 *)from,len);
      break;
    case TWO_SIZES(1,2):
      convert_1_to_2((p_wchar2 *)to,(p_wchar1 *)from,len);
      break;

    case TWO_SIZES(2,0):
      convert_2_to_0((p_wchar0 *)to,(p_wchar2 *)from,len);
      break;
    case TWO_SIZES(2,1):
      convert_2_to_1((p_wchar1 *)to,(p_wchar2 *)from,len);
      break;
    case TWO_SIZES(2,2):
      convert_2_to_2((p_wchar2 *)to,(p_wchar2 *)from,len);
      break;
  }
}

INLINE void pike_string_cpy(void *to,
		     int to_shift,
		     struct pike_string *from)
{
  generic_memcpy(to,to_shift,from->str,from->size_shift,from->len);
}

static void locate_problem(int (*isproblem)(struct pike_string *))
{
  unsigned INT32 e;
  struct pike_string *s;
  DM(struct memhdr *yes=alloc_memhdr());
  DM(struct memhdr *no=alloc_memhdr());

  for(e=0;e<htable_size;e++)
  {
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
  }

  DM(fprintf(stderr,"Plausible problem location(s):\n"));
  DM(dump_memhdr_locations(yes,0));

  DM(fprintf(stderr,"More Plausible problem location(s):\n"));
  DM(dump_memhdr_locations(yes,no));
}

static int has_zero_refs(struct pike_string *s)
{
  return s->refs<=0;
}
static int wrong_hash(struct pike_string *s)
{
  return (s->hval % htable_size) != do_hash(s);
}
static int improper_zero_termination(struct pike_string *s)
{
  return index_shared_string(s,s->len);
}
#else
#define locate_problem(X)
#endif

/*\ find a string in the shared string table.
||| This assumes that the string is minimized!!!! 
\*/
static struct pike_string *internal_findstring(const char *s,
					       int len,
					       int size_shift,
					       int h)
{
  struct pike_string *curr,**prev, **base;

  for(base = prev = base_table + h;( curr=*prev ); prev=&curr->next)
  {
#ifdef DEBUG
    if(curr->refs<1)
    {
      debug_dump_pike_string(curr, 70);
      locate_problem(has_zero_refs);
      fatal("String with no references.\n");
    }
#endif

    if (full_hash_value == curr->hval &&
	len==curr->len &&
	size_shift==curr->size_shift &&
	!MEMCMP(curr->str, s,len<<size_shift)) /* found it */
    {
      *prev = curr->next;
      curr->next = *base;
      *base = curr;
      return curr;		/* pointer to string */
    }
  }
  return 0; /* not found */
}

struct pike_string *binary_findstring(const char *foo, INT32 l)
{
  return internal_findstring(foo, l, 0, StrHash(foo,l));
}

struct pike_string *findstring(const char *foo)
{
  return binary_findstring(foo, strlen(foo));
}

/*
 * find a string that is already shared and move it to the head
 * of that list in the hastable
 */
static struct pike_string *propagate_shared_string(const struct pike_string *s,
						   int h)
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
#ifdef DEBUG
    if(curr->refs<1)
    {
      debug_dump_pike_string(curr, 70);
      locate_problem(has_zero_refs);
      fatal("String with no references.\n");
    }
#endif
  }
  return 0; /* not found */
}

/*** rehash ***/

static void rehash_string_backwards(struct pike_string *s)
{
  int h;
  if(!s) return;
  rehash_string_backwards(s->next);
  h=s->hval % htable_size;
  s->next=base_table[h];
  base_table[h]=s;
}

static void rehash(void)
{
  int h,old;
  struct pike_string **old_base;

  old=htable_size;
  old_base=base_table;

  htable_size=hashprimes[++hashprimes_entry];
  base_table=(struct pike_string **)xalloc(sizeof(struct pike_string *)*htable_size);
  MEMSET((char *)base_table,0,sizeof(struct pike_string *)*htable_size);

  for(h=0;h<old;h++) rehash_string_backwards(old_base[h]);

  if(old_base)
    free((char *)old_base);
}


/*** Make new strings ***/

/* note that begin_shared_string expects the _exact_ size of the string,
 * not the maximum size
 */
struct pike_string *debug_begin_shared_string(int len)
{
  struct pike_string *t;
#ifdef DEBUG
  extern int d_flag;
  if(d_flag>10)
    verify_shared_strings_tables();
#endif
  t=(struct pike_string *)xalloc(len + sizeof(struct pike_string));
  t->str[len]=0;
  t->len=len;
  t->size_shift=0;
  return t;
}

static void link_pike_string(struct pike_string *s, int h)
{
  s->refs = 0;
  s->next = base_table[h];
  base_table[h] = s;
  s->hval=full_hash_value;
  num_strings++;
  if(num_strings > MAX_AVG_LINK_LENGTH * htable_size)
    rehash();
}

struct pike_string *debug_begin_wide_shared_string(int len, int shift)
{
  struct pike_string *t;
#ifdef DEBUG
  extern int d_flag;
  if(d_flag>10)
    verify_shared_strings_tables();
#endif
  t=(struct pike_string *)xalloc((len<<shift) + sizeof(struct pike_string));
  t->len=len;
  t->size_shift=shift;
  low_set_index(t,len,0);
  return t;
}

/*
 * This function assumes that the shift size is already the minimum it
 * can be.
 */
struct pike_string *low_end_shared_string(struct pike_string *s)
{
  int len,h;
  struct pike_string *s2;

  len=s->len;
  h=do_hash(s);
  s2=internal_findstring(s->str,len,s->size_shift,h);
#ifdef DEBUG
  if(s2==s) 
    fatal("end_shared_string called twice! (or something like that)\n");
#endif

  if(s2)
  {
    free((char *)s);
    s=s2;
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
struct pike_string *end_shared_string(struct pike_string *s)
{
  struct pike_string *s2;

  switch(s->size_shift)
  {
    default:
      fatal("ARGHEL!\n");

    case 2:
      switch(find_magnitude2(STR2(s),s->len))
      {
	case 0:
	  s2=begin_shared_string(s->len);
	  convert_2_to_0(STR0(s2),STR2(s),s->len);
	  free((char *)s);
	  s=s2;
	  break;

	case 1:
	  s2=begin_wide_shared_string(s->len,2);
	  convert_2_to_1(STR1(s2),STR2(s),s->len);
	  free((char *)s);
	  s=s2;
	  /* Fall though */
      }
      break;
      
    case 1:
      if(!find_magnitude1(STR1(s),s->len))
      {
	s2=begin_shared_string(s->len);
	convert_1_to_0(STR0(s2),STR1(s),s->len);
	free((char *)s);
	s=s2;
      }
      break;

    case 0: break;
  }

  return low_end_shared_string(s);
}


struct pike_string * debug_make_shared_binary_string(const char *str,int len)
{
  struct pike_string *s;
  int h=StrHash(str,len);

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

struct pike_string * debug_make_shared_binary_string1(const INT16 *str,int len)
{
  struct pike_string *s;
  int h;

  if(!find_magnitude1(str,len))
  {
    /* Wrong size, convert */
    s=begin_shared_string(len);
    convert_1_to_0(s->str,str,len);
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

struct pike_string * debug_make_shared_binary_string2(const INT32 *str,int len)
{
  struct pike_string *s;
  int h;

  switch(find_magnitude2(str,len))
  {
    case 0:
      /* Wrong size, convert */
      s=begin_shared_string(len);
      convert_2_to_0(s->str,str,len);
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

struct pike_string *debug_make_shared_string(const char *str)
{
  return make_shared_binary_string(str, strlen(str));
}

struct pike_string *debug_make_shared_string1(const INT16 *str)
{
  INT32 len;
  for(len=0;str[len];len++);
  return debug_make_shared_binary_string1(str,len);
}

struct pike_string *debug_make_shared_string2(const INT32 *str)
{
  INT32 len;
  for(len=0;str[len];len++);
  return debug_make_shared_binary_string2(str,len);
}

/*** Free strings ***/

void unlink_pike_string(struct pike_string *s)
{
  int h;

  h=do_hash(s);
  propagate_shared_string(s,h);
  base_table[h]=s->next;
#ifdef DEBUG
  s->next=(struct pike_string *)-1;
#endif
  num_strings--;
}

void really_free_string(struct pike_string *s)
{
#ifdef DEBUG
  extern int d_flag;
  if(d_flag > 2)
  {
    if(s->next == (struct pike_string *)-1)
      fatal("Freeing shared string again!\n");

    if(((long)s->next) & 1)
      fatal("Freeing shared string again, memory corrupt or other bug!\n");
  }
#endif
  unlink_pike_string(s);
  free((char *)s);
}

void debug_free_string(struct pike_string *s)
{
  if(--s->refs<=0)
    really_free_string(s);
}


/*
 * String table status
 */
struct pike_string *add_string_status(int verbose)
{
  char b[200];

  init_buf();

  if (verbose)
  {
    int allocd_strings=0;
    int allocd_bytes=0;
    int num_distinct_strings=0;
    int bytes_distinct_strings=0;
    int overhead_bytes=0;
    unsigned INT32 e;
    struct pike_string *p;
    for(e=0;e<htable_size;e++)
    {
      for(p=base_table[e];p;p=p->next)
      {
	num_distinct_strings++;
	bytes_distinct_strings+=MY_ALIGN(p->len);
	allocd_strings+=p->refs;
	allocd_bytes+=p->refs*MY_ALIGN(p->len+3);
      }

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
	    (long)overhead_bytes);
    my_strcat(b);
    sprintf(b,"Space actually required/total string bytes %d%%\n",
	    (bytes_distinct_strings + overhead_bytes)*100 / allocd_bytes);
    my_strcat(b);
  }
/*
  sprintf(b,"Searches: %ld    Average search length: %6.3f\n",
      (long)num_str_searches, (double)search_len / num_str_searches);
  my_strcat(b);
*/
  return free_buf();
}

/*** DEBUG ***/
#ifdef DEBUG

void check_string(struct pike_string *s)
{
  do_hash(s);
  if(full_hash_value != s->hval)
  {
    locate_problem(wrong_hash);
    fatal("Hash value changed?\n");
  }

  if(debug_findstring(s) !=s)
    fatal("Shared string not shared.\n");

  if(index_shared_string(s,s->len))
  {
    locate_problem(improper_zero_termination);
    fatal("Shared string is not zero terminated properly.\n");
  }
}

void verify_shared_strings_tables(void)
{
  unsigned INT32 e, h, num=0;
  unsigned INT32 orig_full_hash = full_hash_value;
  struct pike_string *s;

  for(e=0;e<htable_size;e++)
  {
    h=0;
    for(s=base_table[e];s;s=s->next)
    {
      num++;
      h++;
      if(s->len < 0)
	fatal("Shared string shorter than zero bytes.\n");

      if(s->refs <= 0)
      {
	locate_problem(has_zero_refs);
	fatal("Shared string had too few references.\n");
      }

      if(s->str[s->len])
      {
	locate_problem(improper_zero_termination);
	fatal("Shared string didn't end with a zero.\n");
      }

      if(do_hash(s) != e)
      {
	locate_problem(wrong_hash);
	fatal("Shared string hashed to wrong place.\n");
      }

      if(s->hval != full_hash_value)
	fatal("Shared string hashed to other number.\n");
	
      if(h>10000)
      {
	struct pike_string *s2;
	for(s2=s;s2;s2=s2->next)
	  if(s2 == s)
	    fatal("Shared string table is cyclic.\n");
	h=0;
      }
    }
  }
  if(num != num_strings)
    fatal("Num strings is wrong %d!=%d\n",num,num_strings);
  full_hash_value = orig_full_hash;
}

int safe_debug_findstring(struct pike_string *foo)
{
  unsigned INT32 e;
  if(!base_table) return 0;
  for(e=0;e<htable_size;e++)
  {
    struct pike_string *p;
    for(p=base_table[e];p;p=p->next)
      if(p==foo) return 1;
  }
  return 0;
}

struct pike_string *debug_findstring(const struct pike_string *foo)
{
  struct pike_string *tmp;
  tmp=propagate_shared_string(foo, foo->hval % htable_size);

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
    StrHash(foo->str,foo->len);
    fprintf(stderr,"------ %p %ld\n",
	    base_table[foo->hval %htable_size],
	    (long)full_hash_value);
    for(tmp2=base_table[foo->hval % htable_size];tmp2;tmp2=tmp2->next)
    {
      if(tmp2 == tmp)
	fprintf(stderr,"!!%p!!->",tmp2);
      else
	fprintf(stderr,"%p->",tmp2);
    }
    fprintf(stderr,"0\n");

    for(e=0;e<htable_size;e++)
    {
      for(tmp2=base_table[e];tmp2;tmp2=tmp2->next)
      {
	if(tmp2 == tmp)
	  fprintf(stderr,"String found in hashbin %ld (not %ld)\n",
		  (long)e,
		  (long)(foo->hval % htable_size));
      }
    }
  }
#endif
  return tmp;
}

void debug_dump_pike_string(struct pike_string *s, INT32 max)
{
  INT32 e;
  fprintf(stderr,"0x%p: %ld refs, len=%ld, hval=%lux (%lx)\n",
	  s,
	  (long)s->refs,
	  (long)s->len,
	  (unsigned long)s->hval,
	  (unsigned long)StrHash(s->str, s->len));
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
    for(p=base_table[e];p;p=p->next)
      debug_dump_pike_string(p, 70);
}

#endif


/*** String compare functions ***/

/* does not take locale into account */
int low_quick_binary_strcmp(char *a,INT32 alen,
			    char *b,INT32 blen)
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
int generic_quick_binary_strcmp(const char *a,INT32 alen, int asize,
				const char *b,INT32 blen, int bsize)
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

#ifndef HAVE_STRCOLL
/* No locale function available */
static int low_binary_strcmp(char *a,INT32 alen,
		      char *b,INT32 blen)
{
  low_quick_binary_strcmp(a,alen,b,blen);
}
#else

/* takes locale into account */
static int low_binary_strcmp(char *a,INT32 alen,
			     char *b,INT32 blen)
{
  INT32 tmp;
  while(alen>0 && blen>0)
  {
    tmp=strcoll(a,b);
    if(tmp) return (int)tmp;
    tmp=strlen(a)+1;
    a+=tmp;
    b+=tmp;
    alen-=tmp;
    blen-=tmp;
  }
  if(alen==blen) return 0;
  if(alen > blen) return 1;
  return -1;
}
#endif

/* Does not take locale into account */

int my_quick_strcmp(struct pike_string *a,struct pike_string *b)
{
  if(a==b) return 0;

  return generic_quick_binary_strcmp(a->str, a->len, a->size_shift,
				     b->str, b->len, b->size_shift);
}

/* Does take locale into account */
int my_strcmp(struct pike_string *a,struct pike_string *b)
{
  if(a==b) return 0;

  switch(TWO_SIZES(a->size_shift,b->size_shift))
  {
    case TWO_SIZES(0,0):
      return low_binary_strcmp(a->str,a->len,b->str,b->len);

    default:
    {
      INT32 e,l=MINIMUM(a->len,b->len);
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

struct pike_string *realloc_unlinked_string(struct pike_string *a, INT32 size)
{
  struct pike_string *r;
  r=(struct pike_string *)realloc((char *)a,
					sizeof(struct pike_string)+
				  ((size+1)<<a->size_shift)); /* FIXME !! */
	
  if(!r)
  {
    r=begin_shared_string(size);
    MEMCPY(r->str, a->str, a->len<<a->size_shift);
    free((char *)a);
  }

  r->len=size;
  low_set_index(r,size,0);
  return r;
}

/* Returns an unlinked string ready for end_shared_string */
struct pike_string *realloc_shared_string(struct pike_string *a, INT32 size)
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

struct pike_string *new_realloc_shared_string(struct pike_string *a, INT32 size, int shift)
{
  struct pike_string *r;
  if(shift == a->size_shift) return realloc_shared_string(a,size);

  r=begin_wide_shared_string(size,shift);
  pike_string_cpy(r->str,shift,a);
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
struct pike_string *modify_shared_string(struct pike_string *a,
					 INT32 index,
					 INT32 c)
{
  INT32 old_value;
#ifdef DEBUG
  if(index<0 || index>=a->len)
    fatal("Index out of range in modify_shared_string()\n");
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
	convert_0_to_1(STR1(b),a->str,a->len);
	STR1(b)[index]=c;
	return end_shared_string(b);

      case TWO_SIZES(2,0):
	b=begin_wide_shared_string(a->len,2);
	convert_0_to_2(STR2(b),a->str,a->len);
	STR2(b)[index]=c;
	return end_shared_string(b);

      case TWO_SIZES(2,1):
	b=begin_wide_shared_string(a->len,2);
	convert_1_to_2(STR2(b),STR1(a),a->len);
	STR2(b)[index]=c;
	return end_shared_string(b);

      default:
	fatal("Odd wide string conversion!\n");
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
	fatal("Unshrinkable!\n");

      case 1:
	/* Test if we *actually* can shrink it.. */
	if(find_magnitude1(STR1(a),index)) break;
	if(find_magnitude1(STR1(a)+index+1,a->len-index-1))
	  break;
	
	b=begin_shared_string(a->len);
	convert_1_to_0(b->str,STR1(a),a->len);
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
	    convert_2_to_0(b->str,STR2(a),a->len);
	    b->str[index]=c;
	    free_string(a);
	    return end_shared_string(b);

	  case 1:
	    b=begin_wide_shared_string(a->len,1);
	    convert_2_to_1((unsigned INT16 *)b->str,STR2(a),a->len);
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

    if(index>=HASH_PREFIX && index<a->len-8)
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
struct pike_string *add_shared_strings(struct pike_string *a,
					 struct pike_string *b)
{
  struct pike_string *ret;
  int target_size=MAXIMUM(a->size_shift,b->size_shift);

  ret=begin_wide_shared_string(a->len+b->len,target_size);
  pike_string_cpy(ret->str,ret->size_shift,a);
  pike_string_cpy(ret->str+(a->len<<target_size),ret->size_shift,b);
  return end_shared_string(ret);
}

struct pike_string *add_and_free_shared_strings(struct pike_string *a,
						struct pike_string *b)
{
  INT32 alen=a->len;
  if(a->size_shift == b->size_shift)
  {
    a=realloc_shared_string(a,alen + b->len);
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


int string_search(struct pike_string *haystack,
		  struct pike_string *needle,
		  int start)
{
  struct generic_mem_searcher s;
  char *r;

  if(needle->size_shift > haystack->size_shift)
    return -1;

  init_generic_memsearcher(&s,
			   needle->str,
			   needle->len,
			   needle->size_shift,
			   haystack->len,
			   haystack->size_shift);

  
  r=(char *)generic_memory_search(&s,
				  haystack->str,
				  haystack->len,
				  haystack->size_shift);

  if(!r) return -1;
  return (r-haystack->str)>>haystack->size_shift;
}

struct pike_string *string_slice(struct pike_string *s,
				 INT32 start,
				 INT32 len)
{
#ifdef DEBUG
  if(start < 0 || len<0 || start+len>s->len )
  {
    fatal("string_slice, start = %d, len = %d, s->len = %d\n",start,len,s->len);
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
      return make_shared_binary_string(STR0(s)+start,len);

    case 1:
      return make_shared_binary_string1(STR1(s)+start,len);

    case 2:
      return make_shared_binary_string2(STR2(s)+start,len);

    default:
      fatal("Illegal shift size!\n");
  }
}

/*** replace function ***/
struct pike_string *string_replace(struct pike_string *str,
				   struct pike_string *del,
				   struct pike_string *to)
{
  struct pike_string *ret;
  char *s,*tmp,*r,*end;
  int shift;
  struct generic_mem_searcher searcher;

  if(!str->len)
  {
    add_ref(str);
    return str;
  }

  shift=MAXIMUM(str->size_shift,to->size_shift);

  if(!del->len)
  {
    int e;
    int pos;
    ret=begin_wide_shared_string(str->len + to->len * (str->len -1),shift);
    low_set_index(ret,0,index_shared_string(str,0));
    pos=1;
    for(e=1;e<str->len;e++)
    {
      pike_string_cpy(ret->str+(pos<<shift),shift,to);
      pos+=to->len;
      low_set_index(ret,pos++,index_shared_string(str,e));
    }
    return end_shared_string(ret);
  }

  s=str->str;
  end=s+(str->len<<str->size_shift);

  if(del->len == to->len)
  {
    init_generic_memsearcher(&searcher,
			     del->str,
			     del->len,
			     del->size_shift,
			     str->len,
			     str->size_shift);
    ret=begin_wide_shared_string(str->len,shift);
  }else{
    INT32 delimeters=0;
    init_generic_memsearcher(&searcher,
			     del->str,
			     del->len,
			     del->size_shift,
			     str->len*2,
			     str->size_shift);

    while((s=generic_memory_search(&searcher,
				   s,
				   (end-s)>>str->size_shift,
				   str->size_shift)))
    {
      delimeters++;
      s+=del->len << str->size_shift;
    }
    
    if(!delimeters)
    {
      add_ref(str);
      return str;
    }

    ret=begin_wide_shared_string(str->len + (to->len-del->len)*delimeters, shift);
  }
  s=str->str;
  r=ret->str;

  while((tmp=(char *)generic_memory_search(&searcher,
					   s,
					   (end-s)>>str->size_shift,
					   str->size_shift)))
  {
    generic_memcpy(r,shift,s,str->size_shift,(tmp-s)>>str->size_shift);
    r+=tmp-s;
    pike_string_cpy(r,shift,to);
    r+=to->len << shift;
    s=tmp+(del->len << str->size_shift);
  }
  generic_memcpy(r,shift,s,str->size_shift,(end-s)>>str->size_shift);

  return end_shared_string(ret);
}

/*** init/exit memory ***/
void init_shared_string_table(void)
{
  for(hashprimes_entry=0;hashprimes[hashprimes_entry]<BEGIN_HASH_SIZE;hashprimes_entry++);
  htable_size=hashprimes[hashprimes_entry];
  base_table=(struct pike_string **)xalloc(sizeof(struct pike_string *)*htable_size);
  MEMSET((char *)base_table,0,sizeof(struct pike_string *)*htable_size);
}

#ifdef DEBUG_MALLOC
struct shared_string_location *all_shared_string_locations;
#endif


void cleanup_shared_string_table(void)
{
  unsigned INT32 e;
  struct pike_string *s,*next;

#if defined(DEBUG) && defined(DEBUG_MALLOC)
  while(all_shared_string_locations)
  {
    struct shared_string_location *x=all_shared_string_locations;
    all_shared_string_locations=x->next;
    free_string(x->s);
    x->s=0;
  }

  if(verbose_debug_exit)
  {
    INT32 num,size;
    count_memory_in_strings(&num,&size);
    if(num)
    {
      fprintf(stderr,"Strings left: %d (%d bytes) (zapped)\n",num,size);
      dump_stralloc_strings();
    }
  }
#endif
  for(e=0;e<htable_size;e++)
  {
    for(s=base_table[e];s;s=next)
    {
      next=s->next;
#ifdef REALLY_FREE
      free((char *)s);
#else
      s->next=0;
#endif
    }
    base_table[e]=0;
  }
  free((char *)base_table);
  base_table=0;
  num_strings=0;
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
    for(p=base_table[e];p;p=p->next)
    {
      num_++;
      size_+=sizeof(struct pike_string)+(p->len<<p->size_shift);
    }
  }
#ifdef DEBUG
  if(num_strings != num_)
    fatal("Num strings is wrong! %d!=%d.\n",num_strings, num_);
#endif
  num[0]=num_;
  size[0]=size_;
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

void init_string_builder(struct string_builder *s, int mag)
{
  s->malloced=256;
  s->s=begin_wide_shared_string(256,mag);
  s->s->len=0;
  s->known_shift=0;
}

static void string_build_mkspace(struct string_builder *s, int chars, int mag)
{
  if(mag > s->s->size_shift)
  {
    struct pike_string *n;
    int l=s->s->len+chars+s->malloced;
    n=begin_wide_shared_string(l,mag);
    pike_string_cpy(n->str,mag,s->s);
    n->len=s->s->len;
    s->malloced=l;
    free((char *)s->s);
    s->s=n;
  }
  else if(s->s->len+chars > s->malloced)
  {
    int newlen=MAXIMUM(s->malloced*2,s->s->len+chars);

    s->s=(struct pike_string *)realloc((char *)s->s,
				       sizeof(struct pike_string)+
				       ((newlen+1)<<s->s->size_shift));
    if(!s->s)
      fatal("Out of memory.\n");
    s->malloced=newlen;
  }
}

static void *string_builder_allocate(struct string_builder *s, int chars, int mag)
{
  void *ret;
  string_build_mkspace(s,chars,mag);
  if(chars<0) s->known_shift=0;
  ret=s->s->str + (s->s->len<<s->s->size_shift);
  s->s->len+=chars;
  return ret;
}

void string_builder_putchar(struct string_builder *s, int ch)
{
  INT32 i;
  string_build_mkspace(s,1,min_magnitude(ch));
  s->known_shift=MAXIMUM(min_magnitude(ch),s->known_shift);
  i=s->s->len++;
  low_set_index(s->s,i,ch);
}


void string_builder_binary_strcat(struct string_builder *s, char *str, INT32 len)
{
  string_build_mkspace(s,len,1);
  switch(s->s->size_shift)
  {
    case 0: convert_0_to_0(STR0(s->s)+s->s->len,str,len); break;
    case 1: convert_0_to_1(STR1(s->s)+s->s->len,str,len); break;
    case 2: convert_0_to_2(STR2(s->s)+s->s->len,str,len); break;
    default:
      fatal("Illegal magnitude!\n");
  }
  s->s->len+=len;
}


void string_builder_strcat(struct string_builder *s, char *str)
{
  string_builder_binary_strcat(s,str,strlen(str));
}

void string_builder_shared_strcat(struct string_builder *s, struct pike_string *str)
{
  string_build_mkspace(s,str->len,s->s->size_shift);

  pike_string_cpy(s->s->str + (s->s->len << s->s->size_shift),
		  s->s->size_shift,
		  str);
  s->known_shift=MAXIMUM(s->known_shift,str->size_shift);
  s->s->len+=str->len;
}

struct pike_string *finish_string_builder(struct string_builder *s)
{
  low_set_index(s->s,s->s->len,0);
  if(s->known_shift == s->s->size_shift)
    return low_end_shared_string(s->s);
  return end_shared_string(s->s);
}

