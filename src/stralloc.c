/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#include "stralloc.h"
#include "macros.h"
#include "dynamic_buffer.h"
#include "macros.h"
#include "memory.h"
#include "error.h"

static struct lpc_string *base_table[HTABLE_SIZE];
static unsigned INT32 full_hash_value;

/*
 */
static unsigned int StrHash(const char *s,int len)
{
  full_hash_value=hashmem((unsigned char *)s, len, 20);
  return full_hash_value % HTABLE_SIZE;
}

#ifdef DEBUG
void check_string(struct lpc_string *s)
{
  if(debug_findstring(s) !=s)
    fatal("Shared string not shared.\n");

  if(s->str[s->len])
    fatal("Shared string is not zero terminated properly.\n");
}

void verify_shared_strings_tables()
{
  unsigned int e, h;
  struct lpc_string *s;

  for(e=0;e<HTABLE_SIZE;e++)
  {
    h=0;
    for(s=base_table[e];s;s=s->next)
    {
      h++;
      if(s->len < 0)
	fatal("Shared string shorter than zero bytes.\n");

      if(s->refs <= 0)
	fatal("Shared string had too few references.\n");

      if(s->str[s->len])
	fatal("Shared string didn't end with a zero.\n");

      if(StrHash(s->str, s->len) != e)
	fatal("Shared string hashed to wrong place.\n");

      if(s->hval != full_hash_value)
	fatal("Shared string hashed to other number.\n");
	
      if(h>10000)
      {
	struct lpc_string *s2;
	for(s2=s;s2;s2=s2->next)
	  if(s2 == s)
	    fatal("Shared string table is cyclic.\n");
	h=0;
      }
    }
  }
}
#endif

/*
 * find a string in the shared string table.
 */
static struct lpc_string *internal_findstring(const char *s,int len,int h)
{
  struct lpc_string *curr,**prev, **base;

  for(base = prev = base_table + h;( curr=*prev ); prev=&curr->next)
  {
#ifdef DEBUG
    if(curr->refs<1)
      fatal("String with no references.\n");
#endif

    if (full_hash_value == curr->hval &&
	len==curr->len &&
	!MEMCMP(curr->str, s,len)) /* found it */
    {
      *prev = curr->next;
      curr->next = *base;
      *base = curr;
      return curr;		/* pointer to string */
    }
  }
  return 0; /* not found */
}

struct lpc_string *findstring(const char *foo)
{
  int l;
  l=strlen(foo);
  return internal_findstring(foo, l, StrHash(foo,l));
}

/*
 * find a string that is already shared and move it to the head
 * of that list in the hastable
 */
static struct lpc_string *propagate_shared_string(const struct lpc_string *s,int h)
{
  struct lpc_string *curr, **prev, **base;

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
      fatal("String with no references.\n");
#endif
  }
  return 0; /* not found */
}

#ifdef DEBUG
struct lpc_string *debug_findstring(const struct lpc_string *foo)
{
  return propagate_shared_string(foo, StrHash(foo->str, foo->len));
}
#endif

/* note that begin_shared_string expects the _exact_ size of the string,
 * not the maximum size
 */
struct lpc_string *begin_shared_string(int len)
{
  struct lpc_string *t;
  t=(struct lpc_string *)xalloc(len + sizeof(struct lpc_string));
  t->len=len;
  return t;
}

struct lpc_string *end_shared_string(struct lpc_string *s)
{
  int len,h;
  struct lpc_string *s2;

  len=s->len;
  h=StrHash(s->str,len);
  s2=internal_findstring(s->str,len,h);

  if(s2)
  {
    free((char *)s);
    s=s2;
  }else{
    s->str[len]=0;
    s->refs = 0;
    s->next = base_table[h];
    base_table[h] = s;
    s->hval=full_hash_value;
  }
  s->refs++;

  return s;
}

struct lpc_string * make_shared_binary_string(const char *str,int len)
{
  struct lpc_string *s;
  int h=StrHash(str,len);

  s = internal_findstring(str,len,h);
  if (!s) 
  {
    s=begin_shared_string(len);
    MEMCPY(s->str, str, len);
    s->str[len] = 0;
    s->refs = 0;
    s->next = base_table[h];
    base_table[h] = s;
    s->hval=full_hash_value;
  }

  s->refs++;

  return s;
}

struct lpc_string *make_shared_string(const char *str)
{
  return make_shared_binary_string(str, strlen(str));
}

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
int my_quick_strcmp(struct lpc_string *a,struct lpc_string *b)
{
  if(a==b) return 0;

  return low_quick_binary_strcmp(a->str,a->len,b->str,b->len);
}

/* Does take locale into account */
int my_strcmp(struct lpc_string *a,struct lpc_string *b)
{
  if(a==b) return 0;

  return low_binary_strcmp(a->str,a->len,b->str,b->len);
}

void really_free_string(struct lpc_string *s)
{
  int h;

  h=StrHash(s->str,s->len);
  propagate_shared_string(s,h);
  base_table[h]=s->next;
  free((char *)s);
}

/*
 *
 */
struct lpc_string *add_string_status(int verbose)
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
    int e;
    struct lpc_string *p;
    for(e=0;e<HTABLE_SIZE;e++)
    {
      for(p=base_table[e];p;p=p->next)
      {
	num_distinct_strings++;
	bytes_distinct_strings+=MY_ALIGN(p->len);
	allocd_strings+=p->refs;
	allocd_bytes+=p->refs*MY_ALIGN(p->len+3);
      }

    }
    overhead_bytes=(sizeof(struct lpc_string)-1)*num_distinct_strings;
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

void dump_stralloc_strings()
{
  int e;
  struct lpc_string *p;
  for(e=0;e<HTABLE_SIZE;e++)
    for(p=base_table[e];p;p=p->next)
      printf("%ld refs \"%s\"\n",(long)p->refs,p->str);
}

struct lpc_string *add_shared_strings(struct lpc_string *a,
					 struct lpc_string *b)
{
  INT32 size;
  struct lpc_string *ret;
  char *buf;

  size = a->len + b->len;

  ret=begin_shared_string(size);
  buf=ret->str;
  MEMCPY(buf,a->str,a->len);
  MEMCPY(buf+a->len,b->str,b->len);
  ret=end_shared_string(ret);

  return ret;
}

struct lpc_string *string_replace(struct lpc_string *str,
				     struct lpc_string *del,
				     struct lpc_string *to)
{
  struct lpc_string *ret;
  INT32 delimeters;
  char *s,*tmp,*r,*end;

  s=str->str;
  end=s+str->len;
  delimeters=0;

  while((s=MEMMEM(del->str,del->len,s,end-s)))
  {
    delimeters++;
    s+=del->len;
  }

  if(!delimeters)
  {
    str->refs++;
    return str;
  }

  ret=begin_shared_string(str->len + (to->len-del->len)*delimeters);
  s=str->str;
  r=ret->str;

  while((tmp=MEMMEM(del->str,del->len,s,end-s)))
  {
    MEMCPY(r,s,tmp-s);
    r+=tmp-s;
    MEMCPY(r,to->str,to->len);
    r+=to->len;
    s=tmp+del->len;
  }
  MEMCPY(r,s,end-s);

  return end_shared_string(ret);
}

void cleanup_shared_string_table()
{
  int e;
  struct lpc_string *s,*next;
  for(e=0;e<HTABLE_SIZE;e++)
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
}
