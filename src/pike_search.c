/* New memory searcher functions */

#include "global.h"
#include "stuff.h"
#include "mapping.h"
#include "object.h"
#include "block_alloc.h"
#include "pike_memory.h"
#include "stralloc.h"
#include "pike_error.h"
#include "module_support.h"
#include "interpret.h"
#include "pike_macros.h"
#include "pike_search.h"
#include "bignum.h"

ptrdiff_t pike_search_struct_offset;
#define OB2MSEARCH(O) ((struct pike_mem_searcher *)((O)->storage+pike_search_struct_offset))
#define THIS_MSEARCH ((struct pike_mem_searcher *)(Pike_fp->current_storage))

static struct mapping *memsearch_cache;
struct program *pike_search_program;


void *nil_search(void *no_data,
		 void *haystack,
		 ptrdiff_t haystacklen)
{
  return haystack;
}

/* Needed on architectures where struct returns have
 * incompatible calling conventions (sparc v8).
 */
PCHARP nil_searchN(void *no_data,
		   PCHARP haystack,
		   ptrdiff_t haystacklen)
{
  return haystack;
}

void nil_search_free(void *data) {}
#define memchr_memcmp2_free nil_search_free
#define memchr_memcmp3_free nil_search_free
#define memchr_memcmp4_free nil_search_free
#define memchr_memcmp5_free nil_search_free
#define memchr_memcmp6_free nil_search_free
#define memchr_search_free nil_search_free


struct SearchMojtVtable nil_search_vtable = {
  (SearchMojtFunc0) nil_search,
  (SearchMojtFunc1) nil_search,
  (SearchMojtFunc2) nil_search,
  (SearchMojtFuncN) nil_searchN,
  nil_search_free
};


void free_mem_searcher(void *m)
{
  free_object(*(struct object **)m);
}

#define boyer_moore_hubbe_free free_mem_searcher
#define hubbe_search_free  free_mem_searcher

/* magic stuff for hubbesearch */
/* NOTE: GENERIC_GET4_CHARS(PTR) must be compatible with
 *       the GET_4_{,UN}ALIGNED_CHARS0() variants!
 */
#if PIKE_BYTEORDER == 4321
#define GENERIC_GET4_CHARS(PTR) \
 ( ((PTR)[0] << 24) + ( (PTR)[1] << 16 ) +( (PTR)[2] << 8 ) +  (PTR)[3] )
#else /* PIKE_BYTEORDER != 4321 */
#define GENERIC_GET4_CHARS(PTR) \
 ( ((PTR)[3] << 24) + ( (PTR)[2] << 16 ) +( (PTR)[1] << 8 ) +  (PTR)[0] )
#endif /* PIKE_BYTEORDER == 4321 */

#define HUBBE_ALIGN0(q) q=(char *)( ((ptrdiff_t)q) & -sizeof(INT32))
#define GET_4_ALIGNED_CHARS0(PTR)  (*(INT32 *)(PTR))
#define GET_4_UNALIGNED_CHARS0(PTR)  EXTRACT_INT(PTR)

#define HUBBE_ALIGN1(q) 
#define GET_4_ALIGNED_CHARS1 GENERIC_GET4_CHARS
#define GET_4_UNALIGNED_CHARS1 GENERIC_GET4_CHARS

#define HUBBE_ALIGN2(q) 
#define GET_4_ALIGNED_CHARS2 GENERIC_GET4_CHARS
#define GET_4_UNALIGNED_CHARS2 GENERIC_GET4_CHARS


#define PxC(X,Y) PIKE_CONCAT(X,Y)
#define PxC2(X,Y) PIKE_CONCAT(X,Y)
#define PxC3(X,Y,Z) PIKE_CONCAT3(X,Y,Z)
#define PxC4(X,Y,Z,Q) PIKE_CONCAT4(X,Y,Z,Q)
#define NameN(X) PxC2(X,NSHIFT)
#define NameH(X) PxC2(X,HSHIFT)
#define NameNH(X) PxC3(X,NSHIFT,HSHIFT)

#define BMHASH00(X) (X)
#define BMHASH01(X) ((X)>CHARS ? CHARS : (X))
#define BMHASH02    BMHASH01

#define BMHASH10(X) (((X)*997)&(CHARS-1))
#define BMHASH11 BMHASH10
#define BMHASH12 BMHASH10

#define BMHASH20 BMHASH10
#define BMHASH21 BMHASH20
#define BMHASH22 BMHASH20

#define NCHAR NameN(p_wchar)
#define HCHAR NameH(p_wchar)

#define NEEDLE ((NCHAR *)(s->needle))
#define NEEDLELEN s->needlelen

#define NSHIFT 0
#include "pike_search_engine.c"
#undef NSHIFT

#define NSHIFT 1
#include "pike_search_engine.c"
#undef NSHIFT

#define NSHIFT 2
#include "pike_search_engine.c"
#undef NSHIFT


#define NSHIFT 0



PMOD_EXPORT void pike_init_memsearch(struct pike_mem_searcher *s,
				     PCHARP needle,
				     ptrdiff_t needlelen,
				     ptrdiff_t max_haystacklen)
{
  switch(needle.shift)
  {
    case 0:
      init_memsearch0(s,(p_wchar0*)needle.ptr, needlelen, max_haystacklen);
      return;
    case 1:
      init_memsearch1(s,(p_wchar1*)needle.ptr, needlelen, max_haystacklen);
      return;
    case 2:
      init_memsearch2(s,(p_wchar2*)needle.ptr, needlelen, max_haystacklen);
      return;
  }
}

PMOD_EXPORT SearchMojt compile_memsearcher(PCHARP needle,
					   ptrdiff_t needlelen,
					   int max_haystacklen,
					   struct pike_string *hashkey)
{
  switch(needle.shift)
  {
    case 0:
      return compile_memsearcher0((p_wchar0*)needle.ptr, needlelen, max_haystacklen,hashkey);
    case 1:
      return compile_memsearcher1((p_wchar1*)needle.ptr, needlelen, max_haystacklen,hashkey);
    case 2:
      return compile_memsearcher2((p_wchar2*)needle.ptr, needlelen, max_haystacklen,hashkey);
  }
  fatal("Illegal shift\n");
}

PMOD_EXPORT SearchMojt simple_compile_memsearcher(struct pike_string *str)
{
  return compile_memsearcher(MKPCHARP_STR(str),
			     str->len,
			     0x7fffffff,
			     str);
}

/* This function is thread safe, most other functions in this
 * file are not thread safe.
 */
PMOD_EXPORT char *my_memmem(char *needle,
			    size_t needlelen,
			    char *haystack,
			    size_t haystacklen)
{
  struct pike_mem_searcher tmp;
  init_memsearch0(&tmp, needle, (ptrdiff_t)needlelen, (ptrdiff_t)haystacklen);
  return tmp.mojt.vtab->func0(tmp.mojt.data, haystack, (ptrdiff_t)haystacklen);
  /* No free required - Hubbe */
}


static void f_pike_search(INT32 args)
{
  PCHARP ret, in;
  ptrdiff_t start=0;
  struct pike_string *s;

  check_all_args("Search->`()",args,BIT_STRING, BIT_VOID | BIT_INT, 0);
  s=Pike_sp[-args].u.string;

  if(args>1)
  {
    start=Pike_sp[1-args].u.integer;
    if(start < 0)
    {
      bad_arg_error("Search->`()", Pike_sp-args, args, 2, "int(0..)",
		    Pike_sp+2-args,
		    "Start must be greater or equal to zero.\n");
    }
    if(start >= sp[-args].u.string->len)
    {
      pop_n_elems(args);
      push_int(-1);
      return;
    }
  }

  in=MKPCHARP_STR(s);
  ret=THIS_MSEARCH->mojt.vtab->funcN(THIS_MSEARCH->mojt.data,
				     ADD_PCHARP(in,start),
				     s->len - start);

  pop_n_elems(args);
  push_int64( SUBTRACT_PCHARP(in, ret) );  
}

/* Compatibility: All functions using these two functions
 * should really be updated to use compile_memsearcher instead.
 * -Hubbe
 * These two functions are thread-safe.
 */
PMOD_EXPORT void init_generic_memsearcher(struct generic_mem_searcher *s,
			      void *needle,
			      size_t needlelen,
			      char needle_shift,
			      size_t estimated_haystack,
			      char haystack_shift)
{
  pike_init_memsearch(s,
		      MKPCHARP(needle, needle_shift),
		      (ptrdiff_t)needlelen,
		      estimated_haystack);
}

void *generic_memory_search(struct generic_mem_searcher *s,
			    void *haystack,
			    size_t haystacklen,
			    char haystack_shift)
{
  return ( s->mojt.vtab->funcN(s->mojt.data,
			       MKPCHARP(haystack, haystack_shift),
			       (ptrdiff_t)haystacklen) ).ptr;
}

void init_pike_searching(void)
{
  start_new_program();
  pike_search_struct_offset=ADD_STORAGE(struct pike_mem_searcher);
  map_variable("__s","string",0,
	       pike_search_struct_offset + OFFSETOF(pike_mem_searcher,s), PIKE_T_STRING);
  ADD_FUNCTION("`()",f_pike_search,tFunc(tStr tOr(tInt,tVoid), tInt),0);
  pike_search_program=end_program();
  add_program_constant("Search",pike_search_program,0);

  memsearch_cache=allocate_mapping(10);
  memsearch_cache->data->flags |= MAPPING_FLAG_WEAK;
}

void exit_pike_searching(void)
{
  if(pike_search_program)
  {
    free_program(pike_search_program);
    pike_search_program=0;
  }
  if(memsearch_cache)
  {
    free_mapping(memsearch_cache);
    memsearch_cache=0;
  }
}
