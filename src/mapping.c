/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: mapping.c,v 1.177 2004/03/16 14:09:23 mast Exp $
*/

#include "global.h"
RCSID("$Id: mapping.c,v 1.177 2004/03/16 14:09:23 mast Exp $");
#include "main.h"
#include "object.h"
#include "mapping.h"
#include "svalue.h"
#include "array.h"
#include "pike_macros.h"
#include "pike_error.h"
#include "pike_memory.h"
#include "dynamic_buffer.h"
#include "interpret.h"
#include "las.h"
#include "gc.h"
#include "stralloc.h"
#include "security.h"
#include "block_alloc.h"
#include "opcodes.h"
#include "stuff.h"

#define AVG_LINK_LENGTH 4
#define MIN_LINK_LENGTH 1
#define MAP_SLOTS(X) ((X)?((X)+((X)>>4)+8):0)

struct mapping *first_mapping;

struct mapping *gc_internal_mapping = 0;
static struct mapping *gc_mark_mapping_pos = 0;

#define unlink_mapping_data(M) do{				\
 struct mapping_data *md_=(M);					\
 if(md_->hardlinks) { md_->hardlinks--; md_->valrefs--; }	\
 free_mapping_data(M);						\
}while(0)

#define MAPPING_DATA_SIZE(HSIZE, KEYPAIRS) \
   PTR_TO_INT(MD_KEYPAIRS(0, HSIZE) + KEYPAIRS)

#undef EXIT_BLOCK
#define EXIT_BLOCK(m)	do{						\
DO_IF_DEBUG(								\
  if(m->refs) {								\
    DO_IF_DMALLOC(describe_something(m, T_MAPPING, 0,2,0, NULL));	\
    Pike_fatal("really free mapping on mapping with %d refs.\n", m->refs); \
  }									\
)									\
									\
  FREE_PROT(m);								\
									\
  unlink_mapping_data(m->data);						\
									\
  DOUBLEUNLINK(first_mapping, m);					\
									\
  GC_FREE(m);                                                           \
}while(0)


#undef COUNT_OTHER

#define COUNT_OTHER() do{				\
  struct mapping *m;					\
  double datasize = 0.0;				\
  for(m=first_mapping;m;m=m->next)			\
  {							\
    datasize+=MAPPING_DATA_SIZE(m->data->hashsize, m->data->num_keypairs) / \
      (double) m->data->refs;						\
  }							\
  size += (INT32) datasize;				\
}while(0)

BLOCK_ALLOC_FILL_PAGES(mapping, 2)

#ifndef PIKE_MAPPING_KEYPAIR_LOOP
#define FREE_KEYPAIR(md, k) do {	\
    k->next = md->free_list;		\
    md->free_list = k;			\
  } while(0)
#else /* PIKE_MAPPING_KEYPAIR_LOOP */
#define FREE_KEYPAIR(md, k) do {			\
    md->free_list--;					\
    if (k != md->free_list) {				\
      struct keypair **prev_;				\
      unsigned INT32 h_;				\
      /* Move the last keypair to the new hole. */	\
      *k = *(md->free_list);				\
      h_ = k->hval % md->hashsize;			\
      prev_ = md->hash + h_;				\
      DO_IF_DEBUG(					\
	if (!*prev_) {					\
	  Pike_fatal("Node to move not found!\n");		\
	}						\
      );						\
      while (*prev_ != md->free_list) {			\
	prev_ = &((*prev_)->next);			\
        DO_IF_DEBUG(					\
	  if (!*prev_) {				\
	    Pike_fatal("Node to move not found!\n");		\
	  }						\
	);						\
      }							\
      *prev_ = k;					\
    }							\
  } while(0)
#endif /* !PIKE_MAPPING_KEYPAIR_LOOP */

#ifdef PIKE_DEBUG
/* This function checks that the type field isn't lacking any bits.
 * It is used for debugging purposes only.
 */
static void check_mapping_type_fields(struct mapping *m)
{
  INT32 e;
  struct keypair *k=0,**prev;
  TYPE_FIELD ind_types, val_types;

  ind_types=val_types=0;

  MAPPING_LOOP(m) 
    {
      val_types |= 1 << k->val.type;
      ind_types |= 1 << k->ind.type;
    }

  if(val_types & ~(m->data->val_types))
    Pike_fatal("Mapping value types out of order!\n");

  if(ind_types & ~(m->data->ind_types))
    Pike_fatal("Mapping indices types out of order!\n");
}
#endif

static struct mapping_data empty_data =
  { PIKE_CONSTANT_MEMOBJ_INIT(1), 1, 0,0,0,0,0,0, 0, 0,{0}};
static struct mapping_data weak_ind_empty_data =
  { PIKE_CONSTANT_MEMOBJ_INIT(1), 1, 0,0,0,0,0,0, MAPPING_WEAK_INDICES, 0,{0}};
static struct mapping_data weak_val_empty_data =
  { PIKE_CONSTANT_MEMOBJ_INIT(1), 1, 0,0,0,0,0,0, MAPPING_WEAK_VALUES, 0,{0}};
static struct mapping_data weak_both_empty_data =
  { PIKE_CONSTANT_MEMOBJ_INIT(1), 1, 0,0,0,0,0,0, MAPPING_WEAK, 0,{0}};

/* This function allocates the hash table and svalue space for a mapping
 * struct. The size is the max number of indices that can fit in the
 * allocated space.
 */
static void init_mapping(struct mapping *m,
			 INT32 size,
			 INT16 flags)
{
  struct mapping_data *md;
  ptrdiff_t e;
  INT32 hashsize;

  debug_malloc_touch(m);
#ifdef PIKE_DEBUG
  if (Pike_in_gc > GC_PASS_PREPARE && Pike_in_gc < GC_PASS_ZAP_WEAK)
    Pike_fatal("Can't allocate a new mapping_data inside gc.\n");
  if(size < 0) Pike_fatal("init_mapping with negative value.\n");
#endif
  if(size)
  {
    hashsize=find_good_hash_size(size / AVG_LINK_LENGTH + 1);

    e=MAPPING_DATA_SIZE(hashsize, size);

    md=(struct mapping_data *)xalloc(e);

    m->data=md;
    md->hashsize=hashsize;
    
    MEMSET((char *)md->hash, 0, sizeof(struct keypair *) * md->hashsize);
    
    md->free_list=MD_KEYPAIRS(md, hashsize);
#ifndef PIKE_MAPPING_KEYPAIR_LOOP
    for(e=1;e<size;e++)
    {
      md->free_list[e-1].next = md->free_list + e;
      md->free_list[e-1].ind.type=T_INT;
      md->free_list[e-1].val.type=T_INT;
    }
    md->free_list[e-1].next=0;
    md->free_list[e-1].ind.type=T_INT;
    md->free_list[e-1].val.type=T_INT;
#endif /* !PIKE_MAPPING_KEYPAIR_LOOP */
    md->ind_types = 0;
    md->val_types = 0;
    md->flags = flags;
    md->size = 0;
    md->refs=0;
    md->valrefs=0;
    md->hardlinks=0;
    md->num_keypairs=size;
  }else{
    switch (flags & MAPPING_WEAK) {
      case 0: md = &empty_data; break;
      case MAPPING_WEAK_INDICES: md = &weak_ind_empty_data; break;
      case MAPPING_WEAK_VALUES: md = &weak_val_empty_data; break;
      default: md = &weak_both_empty_data; break;
    }
  }
  add_ref(md);
  m->data=md;
#ifdef MAPPING_SIZE_DEBUG
  m->debug_size = md->size;
#endif
}

/* This function allocates an empty mapping with initial room
 * for 'size' values.
 */
PMOD_EXPORT struct mapping *debug_allocate_mapping(int size)
{
  struct mapping *m;

  m=alloc_mapping();

  GC_ALLOC(m);

  INITIALIZE_PROT(m);
  init_mapping(m,size,0);

  m->refs = 0;
  add_ref(m);	/* For DMALLOC... */

  DOUBLELINK(first_mapping, m);

  return m;
}


PMOD_EXPORT void really_free_mapping_data(struct mapping_data *md)
{
  INT32 e;
  struct keypair *k;
  debug_malloc_touch(md);

#ifdef PIKE_DEBUG
  if (md->refs) {
    Pike_fatal("really_free_mapping_data(): md has non-zero refs: %d\n",
	  md->refs);
  }
#endif /* PIKE_DEBUG */

  NEW_MAPPING_LOOP(md)
  {
    free_svalue(& k->val);
    free_svalue(& k->ind);
  }

  free((char *) md);
  GC_FREE_BLOCK(md);
}

PMOD_EXPORT void do_free_mapping(struct mapping *m)
{
  if (m)
    free_mapping(m);
}

/* This function is used to rehash a mapping without loosing the internal
 * order in each hash chain. This is to prevent mappings from becoming
 * inefficient just after being rehashed.
 */
static void mapping_rehash_backwards_evil(struct mapping_data *md,
					  struct keypair *from)
{
  unsigned INT32 h;
  struct keypair *k;

  if(!from) return;
  mapping_rehash_backwards_evil(md,from->next);

  /* unlink */
  k=md->free_list;
#ifndef PIKE_MAPPING_KEYPAIR_LOOP
#ifdef PIKE_DEBUG
  if(!k) Pike_fatal("Error in rehash: not enough keypairs.\n");
#endif
  md->free_list=k->next;
#else /* PIKE_MAPPING_KEYPAIR_LOOP */
  md->free_list++;
#endif /* !PIKE_MAPPING_KEYPAIR_LOOP */

  /* initialize */
  *k=*from;

  /* link */
  h=k->hval;
  h%=md->hashsize;
  k->next=md->hash[h];
  md->hash[h]=k;

  /* update */
  md->ind_types |= 1<< (k->ind.type);
  md->val_types |= 1<< (k->val.type);
  md->size++;
}

static void mapping_rehash_backwards_good(struct mapping_data *md,
					  struct keypair *from)
{
  unsigned INT32 h;
  struct keypair *k;

  if(!from) return;
  mapping_rehash_backwards_good(md,from->next);

  /* unlink */
  k=md->free_list;
#ifndef PIKE_MAPPING_KEYPAIR_LOOP
#ifdef PIKE_DEBUG
  if(!k) Pike_fatal("Error in rehash: not enough keypairs.\n");
#endif
  md->free_list=k->next;
#else /* PIKE_MAPPING_KEYPAIR_LOOP */
  md->free_list++;
#endif /* !PIKE_MAPPING_KEYPAIR_LOOP */

  /* initialize */
  k->hval=from->hval;
  assign_svalue_no_free(&k->ind, &from->ind);
  assign_svalue_no_free(&k->val, &from->val);

  /* link */
  h=k->hval;
  h%=md->hashsize;
  k->next=md->hash[h];
  md->hash[h]=k;

  /* update */
  md->ind_types |= 1<< (k->ind.type);
  md->val_types |= 1<< (k->val.type);
  md->size++;
}

/* This function re-allocates a mapping. It adjusts the max no. of
 * values can be fitted into the mapping. It takes a bit of time to
 * run, but is used seldom enough not to degrade preformance significantly.
 */
static struct mapping *rehash(struct mapping *m, int new_size)
{
  struct mapping_data *md, *new_md;
#ifdef PIKE_DEBUG
  INT32 tmp=m->data->size;
#endif
  INT32 e;

  md=m->data;
  debug_malloc_touch(md);
#ifdef PIKE_DEBUG
  if(md->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");

  if(d_flag>1)  check_mapping(m);
#endif

  init_mapping(m, new_size, md->flags);
  debug_malloc_touch(m);
  new_md=m->data;

  /* This operation is now 100% atomic - no locking required */
  if(md->refs>1)
  {
    /* good */
    for(e=0;e<md->hashsize;e++)
      mapping_rehash_backwards_good(new_md, md->hash[e]);

    unlink_mapping_data(md);
  }else{
    /* evil */
    for(e=0;e<md->hashsize;e++)
      mapping_rehash_backwards_evil(new_md, md->hash[e]);

    free((char *)md);
    GC_FREE_BLOCK(md);
  }

#ifdef PIKE_DEBUG
  if(m->data->size != tmp)
    Pike_fatal("Rehash failed, size not same any more.\n");
#endif
#ifdef MAPPING_SIZE_DEBUG
  m->debug_size = m->data->size;
#endif

#ifdef PIKE_DEBUG
  if(d_flag>1)  check_mapping(m);
#endif

  return m;
}

/*
 * No need for super efficiency, should not be called very often
 * -Hubbe
 */
#define LOW_RELOC(X) X= (void *) ((char *)(X) + off);
#define RELOC(X) if(X) LOW_RELOC(X)

struct mapping_data *copy_mapping_data(struct mapping_data *md)
{
  long e;
  ptrdiff_t size, off;
  struct mapping_data *nmd;
  struct keypair *keypairs;

#ifdef PIKE_DEBUG
  if (Pike_in_gc > GC_PASS_PREPARE && Pike_in_gc < GC_PASS_ZAP_WEAK)
    Pike_fatal("Can't allocate a new mapping_data inside gc.\n");
#endif

  debug_malloc_touch(md);

  size=MAPPING_DATA_SIZE(md->hashsize, md->num_keypairs);

  nmd=(struct mapping_data *)xalloc(size);
  MEMCPY(nmd, md, size);
  off=((char *)nmd) - ((char *)md);

  RELOC(nmd->free_list);
  for(e=0;e<nmd->hashsize;e++) RELOC(nmd->hash[e]);

  keypairs=MD_KEYPAIRS(nmd, nmd->hashsize);
#ifndef PIKE_MAPPING_KEYPAIR_LOOP
  for(e=0;e<nmd->num_keypairs;e++)
  {
    RELOC(keypairs[e].next);
    add_ref_svalue(& keypairs[e].ind);
    add_ref_svalue(& keypairs[e].val);
  }
#else /* PIKE_MAPPING_KEYPAIR_LOOP */
  for(e=0;e<nmd->size;e++)
  {
    RELOC(keypairs[e].next);
    add_ref_svalue(& keypairs[e].ind);
    add_ref_svalue(& keypairs[e].val);
  }
#endif /* PIKE_MAPPING_KEYPAIR_LOOP */

  nmd->refs=0;
  add_ref(nmd);	/* For DMALLOC... */
  nmd->valrefs=0;
  nmd->hardlinks=0;

  if(md->hardlinks)
  {
#ifdef PIKE_DEBUG
    if(md->refs <= 0 || md->valrefs<=0)
      Pike_fatal("Hardlink without refs/valrefs!\n");
#endif
    md->hardlinks--;
    md->valrefs--;
  }
  sub_ref(md);

  return nmd;
}

#define MAPPING_DATA_IN_USE(MD) ((MD)->refs != (MD)->hardlinks + 1)

#define LOW_FIND(FUN, KEY, FOUND, NOT_FOUND) do {		\
  md=m->data;                                                   \
  add_ref(md);						        \
  if(md->hashsize)						\
  {								\
    h=h2 % md->hashsize;					\
    DO_IF_DEBUG( if(d_flag > 1) check_mapping_type_fields(m); ) \
    if(md->ind_types & ((1 << key->type) | BIT_OBJECT))		\
    {								\
      for(prev= md->hash + h;(k=*prev);prev=&k->next)		\
      {								\
	if(h2 == k->hval && FUN(& k->ind, KEY))		        \
	{							\
	  FOUND;						\
	}							\
      }								\
    }								\
  }								\
  NOT_FOUND;                                                    \
}while(0)


#define LOW_FIND2(FUN, KEY, FOUND, NOT_FOUND) do {		\
  struct keypair *k2;						\
  md=m->data;							\
  add_ref(md);							\
  if(md->hashsize)						\
  {								\
    h=h2 % md->hashsize;					\
    DO_IF_DEBUG( if(d_flag > 1) check_mapping_type_fields(m); ) \
    if(md->ind_types & ((1 << key->type) | BIT_OBJECT))		\
    {								\
      k2=omd->hash[h2 % omd->hashsize];			        \
      prev= md->hash + h;					\
      for(;(k=*prev) && k2;(prev=&k->next),(k2=k2->next))	\
        if(!(h2 == k->hval && is_identical(&k2->ind, &k->ind)))	\
           break;						\
      for(;(k=*prev);prev=&k->next)				\
      {								\
	if(FUN(& k->ind, KEY))					\
	{							\
	  FOUND;						\
	}							\
      }								\
    }								\
  }								\
  NOT_FOUND;							\
}while(0)


#define SAME_DATA(SAME,NOT_SAME) 	\
  if(m->data == md)				\
  {						\
    SAME;					\
  }else{ 					\
    NOT_SAME;					\
  }


/* FIXME: the 'no action' might need to be fixed.... */
#define FIND() do{						\
  LOW_FIND(is_eq, key,						\
	   omd=md;						\
	   SAME_DATA( break,					\
		     struct svalue *tmp=&k->ind;		\
		     LOW_FIND(is_identical, tmp, break, ;);	\
		     free_mapping_data(omd)),			\
	   break);				                \
    free_mapping_data(md);					\
}while(0)

      
/* Assumes md is *NOT* locked */
#define COPYMAP2() do {				\
  ptrdiff_t off;				\
  m->data=copy_mapping_data(m->data);		\
  debug_malloc_touch(m->data);                  \
  DO_IF_DEBUG( if(d_flag>1)  check_mapping(m); ) \
  off=((char *)m->data)-((char *)md);		\
  LOW_RELOC(k);					\
  LOW_RELOC(prev);				\
  md=m->data;                                   \
}while(0)

#define PREPARE_FOR_DATA_CHANGE2() \
 if(md->valrefs) COPYMAP2()

#define PREPARE_FOR_INDEX_CHANGE2() \
  if(md->refs>1) COPYMAP2()

#define PROPAGATE() do {			\
   if(md->refs==1)				\
   {						\
     h=h2 % md->hashsize;                       \
     *prev=k->next;				\
     k->next=md->hash[h];			\
     md->hash[h]=k;				\
   }						\
 }while(0)


/* Assumes md is locked */
#define COPYMAP() do {			        \
  ptrdiff_t off;				\
  m->data=copy_mapping_data(m->data);		\
  debug_malloc_touch(m->data);                  \
  off=((char *)m->data)-((char *)md);		\
  LOW_RELOC(k);					\
  free_mapping_data(md);                        \
  md=m->data;                                   \
  add_ref(md);                                   \
}while(0)

#define PREPARE_FOR_DATA_CHANGE() \
 if(md->valrefs) COPYMAP()

#define PREPARE_FOR_INDEX_CHANGE() \
 if(md->refs>1) COPYMAP()

/* This function brings the type fields in the mapping up to speed.
 * I only use it when the type fields MUST be correct, which is not
 * very often.
 */
PMOD_EXPORT void mapping_fix_type_field(struct mapping *m)
{
  INT32 e;
  struct keypair *k;
  TYPE_FIELD ind_types, val_types;

  val_types = ind_types = 0;

  NEW_MAPPING_LOOP(m->data)
    {
      val_types |= 1 << k->val.type;
      ind_types |= 1 << k->ind.type;
    }

#ifdef PIKE_DEBUG
  if(val_types & ~(m->data->val_types))
    Pike_fatal("Mapping value types out of order!\n");

  if(ind_types & ~(m->data->ind_types))
    Pike_fatal("Mapping indices types out of order!\n");
#endif
  m->data->val_types = val_types;
  m->data->ind_types = ind_types;
}

PMOD_EXPORT void mapping_set_flags(struct mapping *m, int flags)
{
  struct mapping_data *md = m->data;

  if ((md->flags != flags) && (md->refs > 1)) {
    struct keypair *k = NULL, *prev = NULL;
    COPYMAP2();
    md = m->data;
  }
#ifdef PIKE_DEBUG
  if(flags & MAPPING_WEAK)
  {
    debug_malloc_touch(m);
    debug_malloc_touch(md);
  }
  else
  {
    debug_malloc_touch(m);
    debug_malloc_touch(md);
  }
#endif
  md->flags = flags;
}


/* This function inserts key:val into the mapping m.
 * Same as doing m[key]=val; in pike.
 *
 * overwrite:
 *   0: Do not replace the value if the entry exists.
 *   1: Replace the value if the entry exists.
 *   2: Replace both the index and the value if the entry exists.
 */
PMOD_EXPORT void low_mapping_insert(struct mapping *m,
			struct svalue *key,
			struct svalue *val,
			int overwrite)
{
  unsigned INT32 h,h2;
  struct keypair *k, **prev;
  struct mapping_data *md, *omd;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
#endif

  h2=hash_svalue(key);
  
#ifdef PIKE_DEBUG
  if(d_flag>1)  check_mapping(m);
#endif

  LOW_FIND(is_eq, key,
	   struct svalue *tmp=&k->ind;
	   SAME_DATA(goto mi_set_value,
		     omd=md;
		     LOW_FIND(is_identical, tmp,
			      free_mapping_data(omd);
			      goto mi_set_value,
			      free_mapping_data(omd);
			      goto mi_do_nothing)),
	   Z:
	   SAME_DATA(goto mi_insert,
		     omd=md;
		     LOW_FIND2(is_eq, key,
			       free_mapping_data(omd);
			       goto mi_do_nothing,
			       free_mapping_data(omd);
			       goto Z)));

 mi_do_nothing:
  free_mapping_data(md);
  return;

 mi_set_value:
#ifdef PIKE_DEBUG
  if(m->data != md)
    Pike_fatal("Wrong dataset in mapping_insert!\n");
  if(d_flag>1)  check_mapping(m);
#endif
  free_mapping_data(md);
  if(!overwrite) return;
  PREPARE_FOR_DATA_CHANGE2();
  PROPAGATE(); /* propagate after preparing */
  md->val_types |= 1 << val->type;
  if (overwrite == 2 && key->type == T_OBJECT)
    /* Should replace the index too. It's only for objects that it's
     * possible to tell the difference. */
    assign_svalue (&k->ind, key);
  assign_svalue(& k->val, val);
#ifdef PIKE_DEBUG
  if(d_flag>1)  check_mapping(m);
#endif
  return;

 mi_insert:
#ifdef PIKE_DEBUG
  if(m->data != md)
    Pike_fatal("Wrong dataset in mapping_insert!\n");
  if(d_flag>1)  check_mapping(m);
#endif
  free_mapping_data(md);
  /* We do a re-hash here instead of copying the mapping. */
  if(
#ifndef PIKE_MAPPING_KEYPAIR_LOOP
     (!md->free_list) ||
#else /* PIKE_MAPPING_KEYPAIR_LOOP */
     (md->size >= md->num_keypairs) ||
#endif /* !PIKE_MAPPING_KEYPAIR_LOOP */
     md->refs>1)
  {
    debug_malloc_touch(m);
    rehash(m, md->size * 2 + 2);
    md=m->data;
  }
  h=h2 % md->hashsize;

  /* no need to lock here since we are not calling is_eq - Hubbe */

  k=md->free_list;
#ifndef PIKE_MAPPING_KEYPAIR_LOOP
  md->free_list=k->next;
#else /* PIKE_MAPPING_KEYPAIR_LOOP */
  md->free_list++;
#endif /* !PIKE_MAPPING_KEYPAIR_LOOP */
  k->next=md->hash[h];
  md->hash[h]=k;
  md->ind_types |= 1 << key->type;
  md->val_types |= 1 << val->type;
  assign_svalue_no_free(& k->ind, key);
  assign_svalue_no_free(& k->val, val);
  k->hval = h2;
  md->size++;
#ifdef MAPPING_SIZE_DEBUG
  if(m->data ==md)
    m->debug_size++;
#endif

#ifdef PIKE_DEBUG
  if(d_flag>1)  check_mapping(m);
#endif
}

PMOD_EXPORT void mapping_insert(struct mapping *m,
		    struct svalue *key,
		    struct svalue *val)
{
  low_mapping_insert(m,key,val,1);
}

/* Inline the above in this file. */
#define mapping_insert(M, KEY, VAL) low_mapping_insert ((M), (KEY), (VAL), 1)

PMOD_EXPORT union anything *mapping_get_item_ptr(struct mapping *m,
				     struct svalue *key,
				     TYPE_T t)
{
  unsigned INT32 h, h2;
  struct keypair *k, **prev;
  struct mapping_data *md,*omd;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");

  if(d_flag>1)  check_mapping(m);

  debug_malloc_touch(m);
  debug_malloc_touch(m->data);
#endif

  h2=hash_svalue(key);

  LOW_FIND(is_eq, key,
	   struct svalue *tmp=&k->ind;
	   SAME_DATA(goto mg_set_value,
		     omd=md;
		     LOW_FIND(is_identical, tmp,
			      free_mapping_data(omd);
			      goto mg_set_value,
			      free_mapping_data(omd);
			      goto mg_do_nothing)),
	   Z:
	   SAME_DATA(goto mg_insert,
		     omd=md;
		     LOW_FIND2(is_eq, key,
			       free_mapping_data(omd);
			       goto mg_do_nothing,
			       free_mapping_data(omd);
			       goto Z)));



 mg_do_nothing:
  free_mapping_data(md);
  return 0;

 mg_set_value:
#ifdef PIKE_DEBUG
  if(m->data != md)
    Pike_fatal("Wrong dataset in mapping_get_item_ptr!\n");
  if(d_flag)
    check_mapping(m);
#endif
  free_mapping_data(md);
  if(k->val.type == t)
  {
    PREPARE_FOR_DATA_CHANGE2();
    PROPAGATE(); /* prepare then propagate */
    
    return & ( k->val.u );
  }
  
  return 0;
  
 mg_insert:
#ifdef PIKE_DEBUG
  if(m->data != md)
    Pike_fatal("Wrong dataset in mapping_get_item_ptr!\n");
  if(d_flag)
    check_mapping(m);
#endif
  free_mapping_data(md);

  if(t != T_INT) return 0;

  /* no need to call PREPARE_* because we re-hash instead */
  if(
#ifndef PIKE_MAPPING_KEYPAIR_LOOP
     !(md->free_list) ||
#else /* PIKE_MAPPING_KEYPAIR_LOOP */
     (md->size >= md->num_keypairs) ||
#endif /* !PIKE_MAPPING_KEYPAIR_LOOP */
     md->refs>1)
  {
    debug_malloc_touch(m);
    rehash(m, md->size * 2 + 2);
    md=m->data;
  }
  h=h2 % md->hashsize;

  k=md->free_list;
#ifndef PIKE_MAPPING_KEYPAIR_LOOP
  md->free_list=k->next;
#else /* PIKE_MAPPING_KEYPAIR_LOOP */
  md->free_list++;
#endif /* !PIKE_MAPPING_KEYPAIR_LOOP */
  k->next=md->hash[h];
  md->hash[h]=k;
  assign_svalue_no_free(& k->ind, key);
  k->val.type=T_INT;
  k->val.subtype=NUMBER_NUMBER;
  k->val.u.integer=0;
  k->hval = h2;
  md->ind_types |= 1 << key->type;
  md->val_types |= BIT_INT;
  md->size++;
#ifdef MAPPING_SIZE_DEBUG
  if(m->data ==md)
    m->debug_size++;
#endif

#ifdef PIKE_DEBUG
  if(d_flag > 1) check_mapping_type_fields(m);
#endif

  return & ( k->val.u );
}

PMOD_EXPORT void map_delete_no_free(struct mapping *m,
			struct svalue *key,
			struct svalue *to)
{
  unsigned INT32 h,h2;
  struct keypair *k, **prev;
  struct mapping_data *md,*omd;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
  if(d_flag>1)  check_mapping(m);
  debug_malloc_touch(m);
#endif

  h2=hash_svalue(key);

  LOW_FIND(is_eq, key,
	   struct svalue *tmp=&k->ind;
	   SAME_DATA(goto md_remove_value,
		     omd=md;
		     LOW_FIND(is_identical, tmp,
			      free_mapping_data(omd);
			      goto md_remove_value,
			      free_mapping_data(omd);
			      goto md_do_nothing)),
	   goto md_do_nothing);

 md_do_nothing:
  debug_malloc_touch(m);
  free_mapping_data(md);
  if(to)
  {
    to->type=T_INT;
    to->subtype=NUMBER_UNDEFINED;
    to->u.integer=0;
  }
  return;

 md_remove_value:
#ifdef PIKE_DEBUG
  if(md->refs <= 1)
    Pike_fatal("Too few refs i mapping->data\n");
  if(m->data != md)
    Pike_fatal("Wrong dataset in mapping_delete!\n");
  if(d_flag>1)  check_mapping(m);
  debug_malloc_touch(m);
#endif
  free_mapping_data(md);
  PREPARE_FOR_INDEX_CHANGE2();
  /* No need to propagate */
  *prev=k->next;
  free_svalue(& k->ind);
  if(to)
    to[0]=k->val;
  else
    free_svalue(& k->val);

  FREE_KEYPAIR(md, k);

  md->free_list->ind.type=T_INT;
  md->free_list->val.type=T_INT;

  md->size--;
#ifdef MAPPING_SIZE_DEBUG
  if(m->data ==md)
    m->debug_size--;
#endif
  
  if(md->size < (md->hashsize + 1) * MIN_LINK_LENGTH)
  {
    debug_malloc_touch(m);
    rehash(m, MAP_SLOTS(m->data->size));
  }
  
#ifdef PIKE_DEBUG
  if(d_flag>1)  check_mapping(m);
#endif
  return;
}

PMOD_EXPORT void check_mapping_for_destruct(struct mapping *m)
{
  INT32 e;
  struct keypair *k, **prev;
  TYPE_FIELD ind_types, val_types;
  struct mapping_data *md=m->data;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
  if(d_flag>1)  check_mapping(m);
  debug_malloc_touch(m);
  if (Pike_in_gc > GC_PASS_PREPARE && Pike_in_gc < GC_PASS_FREE)
    Pike_fatal("check_mapping_for_destruct called in invalid pass inside gc.\n");
#endif

  /* no is_eq -> no locking */

  if(!md->size) return;

  if((md->ind_types | md->val_types) & (BIT_OBJECT | BIT_FUNCTION))
  {
    val_types = ind_types = 0;
    md->val_types |= BIT_INT;
    for(e=0;e<md->hashsize;e++)
    {
      for(prev= md->hash + e;(k=*prev);)
      {
	check_destructed(& k->val);
	
	if((k->ind.type == T_OBJECT || k->ind.type == T_FUNCTION) &&
	   !k->ind.u.object->prog)
	{
	  debug_malloc_touch(m);
	  debug_malloc_touch(md);
	  PREPARE_FOR_INDEX_CHANGE2();
	  *prev=k->next;
	  free_svalue(& k->ind);
	  free_svalue(& k->val);
	  FREE_KEYPAIR(md, k);
	  md->free_list->ind.type = T_INT;
	  md->free_list->val.type = T_INT;
	  md->size--;
#ifdef MAPPING_SIZE_DEBUG
	  if(m->data ==md)
	  {
	    m->debug_size++;
	    debug_malloc_touch(m);
	  }
#endif
	  debug_malloc_touch(md);
	}else{
	  val_types |= 1 << k->val.type;
	  ind_types |= 1 << k->ind.type;
	  prev=&k->next;
	}
      }
    }

    md->val_types = val_types;
    md->ind_types = ind_types;

    if(MAP_SLOTS(md->size) < md->hashsize * MIN_LINK_LENGTH)
    {
      debug_malloc_touch(m);
      rehash(m, MAP_SLOTS(md->size));
    }

#ifdef PIKE_DEBUG
    if(d_flag>1)  check_mapping(m);
#endif
  }
}

PMOD_EXPORT struct svalue *low_mapping_lookup(struct mapping *m,
				  struct svalue *key)
{
  unsigned INT32 h,h2;
  struct keypair *k=0, **prev=0;
  struct mapping_data *md, *omd;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
  if(d_flag>1)  check_mapping(m);
#endif

  h2=hash_svalue(key);
#ifdef PIKE_DEBUG
  if(d_flag > 1) check_mapping_type_fields(m);
#endif
  FIND();
  if(k)
  {
    PROPAGATE();
    return &k->val;
  }
  return 0;
}

PMOD_EXPORT struct svalue *low_mapping_string_lookup(struct mapping *m,
					 struct pike_string *p)
{
  struct svalue tmp;
  tmp.type=T_STRING;
  tmp.u.string=p;
  return low_mapping_lookup(m, &tmp);
}

PMOD_EXPORT void mapping_string_insert(struct mapping *m,
			   struct pike_string *p,
			   struct svalue *val)
{
  struct svalue tmp;
  tmp.type=T_STRING;
  tmp.u.string=p;
  mapping_insert(m, &tmp, val);
}

PMOD_EXPORT void mapping_string_insert_string(struct mapping *m,
				  struct pike_string *p,
				  struct pike_string *val)
{
  struct svalue tmp;
  tmp.type=T_STRING;
  tmp.u.string=val;
  mapping_string_insert(m, p, &tmp);
}

PMOD_EXPORT struct svalue *simple_mapping_string_lookup(struct mapping *m,
					    char *p)
{
  struct pike_string *tmp;
  if((tmp=findstring(p)))
    return low_mapping_string_lookup(m,tmp);
  return 0;
}

PMOD_EXPORT struct svalue *mapping_mapping_lookup(struct mapping *m,
				      struct svalue *key1,
				      struct svalue *key2,
				      int create)
{
  struct svalue tmp;
  struct mapping *m2;
  struct svalue *s=low_mapping_lookup(m, key1);
  debug_malloc_touch(m);

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
#endif

  if(!s || !s->type==T_MAPPING)
  {
    if(!create) return 0;
    tmp.u.mapping=allocate_mapping(5);
    tmp.type=T_MAPPING;
    mapping_insert(m, key1, &tmp);
    debug_malloc_touch(m);
    debug_malloc_touch(tmp.u.mapping);
    free_mapping(tmp.u.mapping); /* There is one ref in 'm' */
    s=&tmp;
  }

  m2=s->u.mapping;
  debug_malloc_touch(m2);
  s=low_mapping_lookup(m2, key2);
  if(s) return s;
  if(!create) return 0;

  tmp.type=T_INT;
  tmp.subtype=NUMBER_UNDEFINED;
  tmp.u.integer=0;

  mapping_insert(m2, key2, &tmp);
  debug_malloc_touch(m2);

  return low_mapping_lookup(m2, key2);
}


PMOD_EXPORT struct svalue *mapping_mapping_string_lookup(struct mapping *m,
				      struct pike_string *key1,
				      struct pike_string *key2,
				      int create)
{
  struct svalue k1,k2;
  k1.type=T_STRING;
  k1.u.string=key1;
  k2.type=T_STRING;
  k2.u.string=key2;
  return mapping_mapping_lookup(m,&k1,&k2,create);
}



PMOD_EXPORT void mapping_index_no_free(struct svalue *dest,
			   struct mapping *m,
			   struct svalue *key)
{
  struct svalue *p;

  if((p=low_mapping_lookup(m,key)))
  {
    if(p->type==T_INT)
      p->subtype=NUMBER_NUMBER;

    assign_svalue_no_free(dest, p);
  }else{
    dest->type=T_INT;
    dest->u.integer=0;
    dest->subtype=NUMBER_UNDEFINED;
  }
}

PMOD_EXPORT struct array *mapping_indices(struct mapping *m)
{
  INT32 e;
  struct array *a;
  struct svalue *s;
  struct keypair *k;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
#endif

  check_mapping_for_destruct(m);

  a=allocate_array(m->data->size);
  s=ITEM(a);

  /* no locking required */
  NEW_MAPPING_LOOP(m->data) assign_svalue(s++, & k->ind);

  a->type_field = m->data->ind_types;
  
#ifdef PIKE_DEBUG
  if(d_flag > 1) check_mapping_type_fields(m);
#endif

  return a;
}

PMOD_EXPORT struct array *mapping_values(struct mapping *m)
{
  INT32 e;
  struct keypair *k;
  struct array *a;
  struct svalue *s;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
#endif

  check_mapping_for_destruct(m);

  a=allocate_array(m->data->size);
  s=ITEM(a);

  /* no locking required */
  NEW_MAPPING_LOOP(m->data) assign_svalue(s++, & k->val);

  a->type_field = m->data->val_types;

#ifdef PIKE_DEBUG
  if(d_flag > 1) check_mapping_type_fields(m);
#endif
  
  return a;
}

PMOD_EXPORT struct array *mapping_to_array(struct mapping *m)
{
  INT32 e;
  struct keypair *k;
  struct array *a;
  struct svalue *s;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
#endif

  a=allocate_array(m->data->size);
  s=ITEM(a);

  /* no locking required */
  NEW_MAPPING_LOOP(m->data)
    {
      struct array *b=allocate_array(2);
      assign_svalue(b->item+0, & k->ind);
      assign_svalue(b->item+1, & k->val);
      b->type_field = (1 << k->ind.type) | (1 << k->val.type);
      s->u.array=b;
      s->type=T_ARRAY;
      s++;
    }
  a->type_field = BIT_ARRAY;

  return a;
}

PMOD_EXPORT void mapping_replace(struct mapping *m,struct svalue *from, struct svalue *to)
{
  INT32 e;
  struct keypair *k;
  struct mapping_data *md;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
#endif

  md=m->data;
  if(md->size)
  {
    add_ref(md);
    NEW_MAPPING_LOOP(md)
      {
	if(is_eq(& k->val, from))
	{
	  PREPARE_FOR_DATA_CHANGE();
	  assign_svalue(& k->val, to);
	  md->val_types|=1<<to->type;
	}
      }
    free_mapping_data(md);
  }

#ifdef PIKE_DEBUG
  if(d_flag > 1) check_mapping_type_fields(m);
#endif
}

PMOD_EXPORT struct mapping *mkmapping(struct array *ind, struct array *val)
{
  struct mapping *m;
  struct svalue *i,*v;
  INT32 e;

#ifdef PIKE_DEBUG
  if(ind->size != val->size)
    Pike_fatal("mkmapping on different sized arrays.\n");
#endif

  m=allocate_mapping(MAP_SLOTS(ind->size));
  i=ITEM(ind);
  v=ITEM(val);
  for(e=0;e<ind->size;e++) low_mapping_insert(m, i++, v++, 2);

  return m;
}

#if 0
PMOD_EXPORT struct mapping *copy_mapping(struct mapping *m)
{
  INT32 e;
  struct mapping *n;
  struct keypair *k;
  struct mapping_data *md;

  md=m->data;
  n=allocate_mapping(MAP_SLOTS(md->size));

  md->valrefs++;
  add_ref(md);
  NEW_MAPPING_LOOP(md) mapping_insert(n, &k->ind, &k->val);
  md->valrefs--;
  free_mapping_data(md);
  
  return n;
}
#else

/* deferred mapping copy! */
PMOD_EXPORT struct mapping *copy_mapping(struct mapping *m)
{
  struct mapping *n;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
#endif

  n=allocate_mapping(0);
  debug_malloc_touch(n->data);
  free_mapping_data(n->data);
  n->data=m->data;
#ifdef MAPPING_SIZE_DEBUG
  n->debug_size=n->data->size;
#endif
  add_ref(n->data);
  n->data->valrefs++;
  n->data->hardlinks++;
  debug_malloc_touch(n->data);
  return n;
}

#endif

static struct mapping *subtract_mappings(struct mapping *a, struct mapping *b)
{
  struct mapping *res;
  struct keypair *k;
  struct mapping_data *a_md = a->data;
  struct mapping_data *b_md = b->data;
  INT32 e;
  ONERROR err;

  /* First some special cases. */
  if (!a_md->size || !b_md->size || !a_md->hashsize || !b_md->hashsize) {
    return copy_mapping(a);
  }
  if (a_md == b_md) {
    return allocate_mapping(0);
  }
  /* FIXME: The break-even point should probably be researched. */
  if (a_md->size < b_md->size) {
    /* Add the elements in a that aren't in b. */
    res = allocate_mapping(a_md->size);
    SET_ONERROR(err, do_free_mapping, res);
    NEW_MAPPING_LOOP(a_md) {
      size_t h = k->hval % b_md->hashsize;
      struct keypair *k2;
      for (k2 = b_md->hash[h]; k2; k2 = k2->next) {
	if ((k2->hval == k->hval) && is_eq(&k2->ind, &k->ind)) {
	  break;
	}
      }
      if (!k2) {
	mapping_insert(res, &k->ind, &k->val);
      }
    }
  } else {
    /* Remove the elements in a that are in b. */
    res = copy_mapping(a);
    SET_ONERROR(err, do_free_mapping, res);
    NEW_MAPPING_LOOP(b_md) {
      map_delete(res, &k->ind);
    }
  }
  UNSET_ONERROR(err);
  return res;
}

static struct mapping *and_mappings(struct mapping *a, struct mapping *b)
{
  struct mapping *res;
  struct keypair *k;
  struct mapping_data *a_md = a->data;
  struct mapping_data *b_md = b->data;
  INT32 e;
  ONERROR err;

  /* First some special cases. */
  if (!a_md->size || !b_md->size) return allocate_mapping(0);
  if (a_md == b_md) return copy_mapping(a);

  /* Copy the second mapping. */
  res = copy_mapping(b);
  SET_ONERROR(err, do_free_mapping, res);

  /* Remove elements in res that aren't in a. */
  NEW_MAPPING_LOOP(b_md) {
    size_t h = k->hval % a_md->hashsize;
    struct keypair *k2;
    for (k2 = a_md->hash[h]; k2; k2 = k2->next) {
      if ((k2->hval == k->hval) && is_eq(&k2->ind, &k->ind)) {
	break;
      }
    }
    if (!k2) {
      map_delete(res, &k->ind);
    }
  }
  UNSET_ONERROR(err);
  return res;
}

static struct mapping *or_mappings(struct mapping *a, struct mapping *b)
{
  struct mapping *res;
  struct keypair *k;
  struct mapping_data *a_md = a->data;
  struct mapping_data *b_md = b->data;
  INT32 e;
  ONERROR err;

  /* First some special cases. */
  if (!a_md->size) return copy_mapping(b);
  if (!b_md->size) return copy_mapping(a);
  if (a_md == b_md) return copy_mapping(a);

  /* Copy the second mapping. */
  res = copy_mapping(b);
  SET_ONERROR(err, do_free_mapping, res);

  /* Add elements in a that aren't in b. */
  NEW_MAPPING_LOOP(a_md) {
    size_t h = k->hval % b_md->hashsize;
    struct keypair *k2;
    for (k2 = b_md->hash[h]; k2; k2 = k2->next) {
      if ((k2->hval == k->hval) && is_eq(&k2->ind, &k->ind)) {
	break;
      }
    }
    if (!k2) {
      mapping_insert(res, &k->ind, &k->val);
    }
  }
  UNSET_ONERROR(err);
  return res;
}

static struct mapping *xor_mappings(struct mapping *a, struct mapping *b)
{
  struct mapping *res;
  struct keypair *k;
  struct mapping_data *a_md = a->data;
  struct mapping_data *b_md = b->data;
  INT32 e;
  ONERROR err;

  /* First some special cases. */
  if (!a_md->size) return copy_mapping(b);
  if (!b_md->size) return copy_mapping(a);
  if (a_md == b_md) return allocate_mapping(0);

  /* Copy the largest mapping. */
  if (a_md->size > b_md->size) {
    struct mapping *tmp = a;
    a = b;
    b = tmp;
    a_md = b_md;
    b_md = b->data;
  }    
  res = copy_mapping(b);
  SET_ONERROR(err, do_free_mapping, res);

  /* Add elements in a that aren't in b, and remove those that are. */
  NEW_MAPPING_LOOP(a_md) {
    size_t h = k->hval % b_md->hashsize;
    struct keypair *k2;
    for (k2 = b_md->hash[h]; k2; k2 = k2->next) {
      if ((k2->hval == k->hval) && is_eq(&k2->ind, &k->ind)) {
	break;
      }
    }
    if (!k2) {
      mapping_insert(res, &k->ind, &k->val);
    } else {
      map_delete(res, &k2->ind);
    }
  }
  UNSET_ONERROR(err);
  return res;
}

PMOD_EXPORT struct mapping *merge_mappings(struct mapping *a, struct mapping *b, INT32 op)
{
  ONERROR r1,r2,r3,r4;
  struct array *ai, *av;
  struct array *bi, *bv;
  struct array *ci, *cv;
  INT32 *zipper;
  struct mapping *m;

#ifdef PIKE_DEBUG
  if(a->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
  if(b->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
#endif

  switch (op) {
  case PIKE_ARRAY_OP_SUB:
    return subtract_mappings(a, b);
  case PIKE_ARRAY_OP_AND:
    return and_mappings(a, b);
  case PIKE_ARRAY_OP_OR:
    return or_mappings(a, b);
  case PIKE_ARRAY_OP_XOR:
    return xor_mappings(a, b);
  }

  ai=mapping_indices(a);
  SET_ONERROR(r1,do_free_array,ai);

  av=mapping_values(a);
  SET_ONERROR(r2,do_free_array,av);

  if(ai->size > 1)
  {
    zipper=get_set_order(ai);
    order_array(ai, zipper);
    order_array(av, zipper);
    free((char *)zipper);
  }

  bi=mapping_indices(b);
  SET_ONERROR(r3,do_free_array,bi);

  bv=mapping_values(b);
  SET_ONERROR(r4,do_free_array,bv);

  if(bi->size > 1)
  {
    zipper=get_set_order(bi);
    order_array(bi, zipper);
    order_array(bv, zipper);
    free((char *)zipper);
  }

  zipper=merge(ai,bi,op);

  ci=array_zip(ai,bi,zipper);

  UNSET_ONERROR(r4); free_array(bi);
  UNSET_ONERROR(r3); free_array(ai);

  cv=array_zip(av,bv,zipper);

  UNSET_ONERROR(r2); free_array(bv);
  UNSET_ONERROR(r1); free_array(av);

  free((char *)zipper);

  m=mkmapping(ci, cv);
  free_array(ci);
  free_array(cv);

  return m;
}

/* FIXME: What are the semantics for this function?
 * FIXME: It ought to be optimized just like the unordered variant.
 *	/grubba 2003-11-12
 */
PMOD_EXPORT struct mapping *merge_mapping_array_ordered(struct mapping *a, 
					    struct array *b, INT32 op)
{
  ONERROR r1,r2;
  struct array *ai, *av;
  struct array *ci = NULL, *cv = NULL;
  INT32 *zipper = NULL;
  struct mapping *m;

  ai=mapping_indices(a);
  SET_ONERROR(r1,do_free_array,ai);
  av=mapping_values(a);
  SET_ONERROR(r2,do_free_array,av);
  if(ai->size > 1)
  {
    zipper=get_set_order(ai);
    order_array(ai, zipper);
    order_array(av, zipper);
    free((char *)zipper);
  }

  switch (op) /* no elements from »b» may be selected */
  {
     case PIKE_ARRAY_OP_AND:
	zipper=merge(b,ai,op);
	ci=array_zip(b,ai,zipper); /* b must not be used */
	cv=array_zip(b,av,zipper); /* b must not be used */
	break;
     case PIKE_ARRAY_OP_SUB:
	zipper=merge(ai,b,op);
	ci=array_zip(ai,b,zipper); /* b must not be used */
	cv=array_zip(av,b,zipper); /* b must not be used */
	break;
     default:
	Pike_fatal("merge_mapping_array on other than AND or SUB\n");
  }

  UNSET_ONERROR(r2); free_array(av);
  UNSET_ONERROR(r1); free_array(ai);
  
  free((char *)zipper);

  m=mkmapping(ci, cv);
  free_array(ci);
  free_array(cv);

  return m;
}

PMOD_EXPORT struct mapping *merge_mapping_array_unordered(struct mapping *a, 
					      struct array *b, INT32 op)
{
  ONERROR r1;
  struct array *b_temp;
  INT32 *zipper;
  struct mapping *m;

  if (b->size>1)
  {
    zipper=get_set_order(b);
    b_temp=reorder_and_copy_array(b,zipper);
    free (zipper);
    SET_ONERROR(r1,do_free_array,b_temp);
    m=merge_mapping_array_ordered(a,b_temp,op);
    UNSET_ONERROR(r1); free_array(b_temp);
  }
  else
    m=merge_mapping_array_ordered(a,b,op);

  return m;
}

PMOD_EXPORT struct mapping *add_mappings(struct svalue *argp, INT32 args)
{
  INT32 e,d;
  struct mapping *ret=0;
  struct keypair *k;

  for(e=d=0;d<args;d++)
  {
#ifdef PIKE_DEBUG
    if(d_flag>1) check_mapping(argp[d].u.mapping);
#endif
    e+=argp[d].u.mapping->data->size;
  }

  if(!e) return allocate_mapping(0);

  d=0;

  for(;d<args;d++)
  {
    struct mapping *m=argp[d].u.mapping;
    struct mapping_data *md=m->data;

    if(md->size == 0) continue;

    if(!(md->flags  & MAPPING_WEAK))
    {
#if 1 /* major optimization */
      if(e==md->size)
	return copy_mapping(m);
#endif
    
      if(m->refs == 1 && !md->hardlinks)
      {
	add_ref( ret=m );
	d++;
	break;
      }
    }
    ret=allocate_mapping(MAP_SLOTS(e));
    break;

  }

  for(;d<args;d++)
  {
    struct mapping *m=argp[d].u.mapping;
    struct mapping_data *md=m->data;
    
    add_ref(md);
    NEW_MAPPING_LOOP(md)
      low_mapping_insert(ret, &k->ind, &k->val, 2);
    free_mapping_data(md);
  }
#ifdef PIKE_DEBUG
  if(!ret)
    Pike_fatal("add_mappings is confused!\n");
#endif
  return ret;
}

PMOD_EXPORT int mapping_equal_p(struct mapping *a, struct mapping *b, struct processing *p)
{
  struct processing curr;
  struct keypair *k;
  struct mapping_data *md;
  INT32 e,eq=1;

#ifdef PIKE_DEBUG
  if(a->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
  if(b->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
#endif

  if(a==b) return 1;

  if (a->data == b->data) return 1;

  if (a->data->flags || b->data->flags) return 0;

  check_mapping_for_destruct(a);
  check_mapping_for_destruct(b);

  if(m_sizeof(a) != m_sizeof(b)) return 0;

  curr.pointer_a = a;
  curr.pointer_b = b;
  curr.next = p;

  for( ;p ;p=p->next)
    if(p->pointer_a == (void *)a && p->pointer_b == (void *)b)
      return 1;

  md=a->data;
  md->valrefs++;
  add_ref(md);

  NEW_MAPPING_LOOP(md)
  {
    struct svalue *s;
    if((s=low_mapping_lookup(b, & k->ind)))
    {
      if(!low_is_equal(s, &k->val, &curr))
      {
	eq=0;
	break;
      }
    }else{
      INT32 d;
      struct mapping_data *bmd = b->data;
      struct keypair *kp;

      /* This is neither pretty nor fast, but it should
       * perform a bit more like expected... -Hubbe
       */

      bmd->valrefs++;
      add_ref(bmd);

      eq=0;
      for(d=0;d<(bmd)->hashsize;d++)
      {
	for(kp=bmd->hash[d];kp;kp=kp->next)
	{
	  if(low_is_equal(&k->ind, &kp->ind, &curr) &&
	     low_is_equal(&k->val, &kp->val, &curr))
	  {
	    eq=1;
	    break;
	  }
	}
      }

      bmd->valrefs--;
      free_mapping_data(bmd);

      if(!eq) break;
    }
  }
  md->valrefs--;
  free_mapping_data(md);
  return eq;
}

void describe_mapping(struct mapping *m,struct processing *p,int indent)
{
  struct processing doing;
  struct array *a;
  JMP_BUF catch;
  ONERROR err;
  INT32 e,d;
  char buf[40];

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
#endif

  if(! m->data->size)
  {
    my_strcat("([ ])");
    return;
  }

  doing.next=p;
  doing.pointer_a=(void *)m;
  for(e=0;p;e++,p=p->next)
  {
    if(p->pointer_a == (void *)m)
    {
      sprintf(buf,"@%ld",(long)e);
      my_strcat(buf);
      return;
    }
  }

  if (Pike_in_gc) {
    /* Have to do without any temporary allocations. */
    struct keypair *k;
    int notfirst = 0;

    if (m->data->size == 1) {
      my_strcat("([ /* 1 element */\n");
    } else {
      sprintf(buf, "([ /* %ld elements */\n", (long)m->data->size);
      my_strcat(buf);
    }

    NEW_MAPPING_LOOP(m->data) {
      if (notfirst) my_strcat(",\n");
      else notfirst = 1;
      for(d = 0; d < indent; d++)
	my_putchar(' ');
      describe_svalue(&k->ind, indent+2, &doing);
      my_strcat (": ");
      describe_svalue(&k->val, indent+2, &doing);
    }

    my_putchar('\n');
    for(e=2; e<indent; e++) my_putchar(' ');
    my_strcat("])");
    return;
  }

  a = mapping_indices(m);
  SET_ONERROR(err, do_free_array, a);

  if(! m->data->size) {		/* mapping_indices may remove elements */
    my_strcat("([ ])");
  }
  else {
    int save_t_flag = Pike_interpreter.trace_level;

    if (m->data->size == 1) {
      my_strcat("([ /* 1 element */\n");
    } else {
      sprintf(buf, "([ /* %ld elements */\n", (long)m->data->size);
      my_strcat(buf);
    }

    Pike_interpreter.trace_level = 0;
    if(SETJMP(catch)) {
      free_svalue(&throw_value);
      throw_value.type = T_INT;
    }
    else
      sort_array_destructively(a);
    UNSETJMP(catch);
    Pike_interpreter.trace_level = save_t_flag;

    for(e = 0; e < a->size; e++)
    {
      struct svalue *tmp;
      if(e)
	my_strcat(",\n");
    
      for(d = 0; d < indent; d++)
	my_putchar(' ');
    
      describe_svalue(ITEM(a)+e, indent+2, &doing);
      my_strcat (": ");

      {
	int save_t_flag=Pike_interpreter.trace_level;
	Pike_interpreter.trace_level=0;
	
	tmp=low_mapping_lookup(m, ITEM(a)+e);
	
	Pike_interpreter.trace_level=save_t_flag;
      }
      if(tmp)
	describe_svalue(tmp, indent+2, &doing);
      else
	my_strcat("** gone **");
    }

    my_putchar('\n');
    for(e=2; e<indent; e++) my_putchar(' ');
    my_strcat("])");
  }

  UNSET_ONERROR(err);
  free_array(a);
}

node *make_node_from_mapping(struct mapping *m)
{
#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
#endif

  mapping_fix_type_field(m);

  if(!mapping_is_constant(m,0))
  {
    struct array *ind, *val;
    node *n;
    ind=mapping_indices(m);
    val=mapping_values(m);
    n=mkefuncallnode("mkmapping",
		     mknode(F_ARG_LIST,
			    make_node_from_array(ind),
			    make_node_from_array(val)));
    free_array(ind);
    free_array(val);
    return n;
  }else{
    struct svalue s;

    if(!m->data->size)
      return mkefuncallnode("aggregate_mapping",0);

    s.type=T_MAPPING;
    s.subtype=0;
    s.u.mapping=m;
    return mkconstantsvaluenode(&s);
  }
}

/*! @decl mapping aggregate_mapping(mixed ... elems)
 *!
 *! Construct a mapping.
 *!
 *! Groups the arguments together two and two in key-index pairs and
 *! creates a mapping of those pairs. Generally, the mapping literal
 *! syntax is handier: @expr{([ key1:val1, key2:val2, ... ])@}
 *!
 *! @seealso
 *!   @[sizeof()], @[mappingp()], @[mkmapping()]
 */
PMOD_EXPORT void f_aggregate_mapping(INT32 args)
{
  INT32 e;
  struct mapping *m;

  if(args & 1)
    Pike_error("Uneven number of arguments to aggregate_mapping.\n");

  m=allocate_mapping(MAP_SLOTS(args / 2));

  for(e=-args;e<0;e+=2) low_mapping_insert(m, Pike_sp+e, Pike_sp+e+1, 2);
  pop_n_elems(args);
#ifdef PIKE_DEBUG
  if(d_flag)
    check_mapping(m);
#endif
  push_mapping(m);
}

PMOD_EXPORT struct mapping *copy_mapping_recursively(struct mapping *m,
						     struct processing *p)
{
  struct processing doing;
  struct mapping *ret;
  INT32 e;
  struct keypair *k;
  struct mapping_data  *md;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
#endif

  doing.next=p;
  doing.pointer_a=(void *)m;
  for(;p;p=p->next)
  {
    if(p->pointer_a == (void *)m)
    {
      add_ref(ret=(struct mapping *)p->pointer_b);
      return ret;
    }
  }

#ifdef PIKE_DEBUG
  if(d_flag > 1) check_mapping_type_fields(m);
#endif

  if(!((m->data->val_types | m->data->ind_types) & BIT_COMPLEX))
    return copy_mapping(m);

  ret=allocate_mapping(MAP_SLOTS(m->data->size));
  doing.pointer_b=ret;

  ret->data->flags = m->data->flags;

  check_stack(2);

  md=m->data;
  md->valrefs++;
  add_ref(md);
  NEW_MAPPING_LOOP(md)
  {
    copy_svalues_recursively_no_free(Pike_sp,&k->ind, 1, &doing);
    Pike_sp++;
    dmalloc_touch_svalue(Pike_sp-1);
    copy_svalues_recursively_no_free(Pike_sp,&k->val, 1, &doing);
    Pike_sp++;
    dmalloc_touch_svalue(Pike_sp-1);
    
    low_mapping_insert(ret, Pike_sp-2, Pike_sp-1, 2);
    pop_n_elems(2);
  }
  md->valrefs--;
  free_mapping_data(md);

  return ret;
}


PMOD_EXPORT void mapping_search_no_free(struct svalue *to,
			    struct mapping *m,
			    struct svalue *look_for,
			    struct svalue *key /* start */)
{
  struct mapping_data *md, *omd;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
#endif
  md=m->data;

  if(md->size)
  {
    unsigned INT32 h2,h=0;
    struct keypair *k=md->hash[0], **prev;

    if(key)
    {
      h2=hash_svalue(key);
      
      FIND();
      
      if(!k)
      {
	to->type=T_INT;
	to->subtype=NUMBER_UNDEFINED;
	to->u.integer=0;
	return;
      }
      k=k->next;
    }
    
    
    md=m->data;
    if(md->size)
    {
      md->valrefs++;
      add_ref(md);
      
      if(h < (unsigned INT32)md->hashsize)
      {
	while(1)
	{
	  while(k)
	  {
	    if(is_eq(look_for, &k->val))
	    {
	      assign_svalue_no_free(to,&k->ind);
	      
	      md->valrefs--;
	      free_mapping_data(md);
	      return;
	    }
	    k=k->next;
	  }
	  h++;
	  if(h>= (unsigned INT32)md->hashsize)
	    break;
	  k=md->hash[h];
	}
      }
    }
    
    md->valrefs--;
    free_mapping_data(md);
  }

  to->type=T_INT;
  to->subtype=NUMBER_UNDEFINED;
  to->u.integer=0;
}


#ifdef PIKE_DEBUG

void check_mapping(struct mapping *m)
{
  int e,num;
  struct keypair *k;
  struct mapping_data *md;

  static int in_check_mapping;
  if(in_check_mapping) return;
  in_check_mapping=1;

  md=m->data;

  if(m->refs <=0)
    Pike_fatal("Mapping has zero refs.\n");

  if(!m->data)
    Pike_fatal("Mapping has no data block.\n");

  if (!m->data->refs)
    Pike_fatal("Mapping data block has zero refs.\n");

  if(m->next && m->next->prev != m)
    Pike_fatal("Mapping ->next->prev != mapping.\n");

#ifdef MAPPING_SIZE_DEBUG
  if(m->debug_size != md->size)
  {
    if(Pike_in_gc)
    {
      fprintf(stderr,"Pike was in GC stage %d when this fatal occured:\n",Pike_in_gc);
      Pike_in_gc=0;
    }
    
    fprintf(stderr,"--MAPPING ZAPPING (%d!=%d), mapping:\n",m->debug_size,md->size);
    describe(m);
    fprintf(stderr,"--MAPPING ZAPPING (%d!=%d), mapping data:\n",m->debug_size,md->size);
    describe(md);
    Pike_fatal("Mapping zapping detected (%d != %d)!\n",m->debug_size,md->size);
  }
#endif

  if(m->prev)
  {
    if(m->prev->next != m)
      Pike_fatal("Mapping ->prev->next != mapping.\n");
  }else{
    if(first_mapping != m)
      Pike_fatal("Mapping ->prev == 0 but first_mapping != mapping.\n");
  }

  if(md->valrefs <0)
    Pike_fatal("md->valrefs  < 0\n");

  if(md->hardlinks <0)
    Pike_fatal("md->valrefs  < 0\n");

  if(md->refs < md->valrefs+1)
    Pike_fatal("md->refs < md->valrefs+1\n");

  if(md->valrefs < md->hardlinks)
    Pike_fatal("md->refs < md->valrefs+1\n");

  if(md->hashsize < 0)
    Pike_fatal("Assert: I don't think he's going to make it Jim.\n");

  if(md->size < 0)
    Pike_fatal("Core breach, evacuate ship!\n");

  if(md->num_keypairs < 0)
    Pike_fatal("Starboard necell on fire!\n");

  if(md->size > md->num_keypairs)
    Pike_fatal("Pretty mean hashtable there buster!\n");

  if(md->hashsize > md->num_keypairs)
    Pike_fatal("Pretty mean hashtable there buster %d > %d (2)!\n",md->hashsize,md->num_keypairs);

  if(md->num_keypairs > (md->hashsize + 3) * AVG_LINK_LENGTH)
    Pike_fatal("Mapping from hell detected, attempting to send it back...\n");
  
  if(md->size > 0 && (!md->ind_types || !md->val_types))
    Pike_fatal("Mapping type fields are... wrong.\n");

  num=0;
  NEW_MAPPING_LOOP(md)
    {
      num++;

      if(! ( (1 << k->ind.type) & (md->ind_types) ))
	Pike_fatal("Mapping indices type field lies.\n");

      if(! ( (1 << k->val.type) & (md->val_types) ))
	Pike_fatal("Mapping values type field lies.\n");

      check_svalue(& k->ind);
      check_svalue(& k->val);

      /* FIXME add check for k->hval
       * beware that hash_svalue may be threaded and locking
       * is required!!
       */
    }
  
  if(md->size != num)
    Pike_fatal("Shields are failing, hull integrity down to 20%%\n");

  in_check_mapping=0;
}

void check_all_mappings(void)
{
  struct mapping *m;
  for(m=first_mapping;m;m=m->next)
    check_mapping(m);
}
#endif

#ifdef MAPPING_SIZE_DEBUG
#define DO_IF_MAPPING_SIZE_DEBUG(x) x
#else
#define DO_IF_MAPPING_SIZE_DEBUG(x)
#endif

#define GC_RECURSE_MD_IN_USE(MD, RECURSE_FN, IND_TYPES, VAL_TYPES) do { \
  INT32 e;								\
  struct keypair *k;							\
  IND_TYPES = MD->ind_types;						\
  NEW_MAPPING_LOOP(MD) {						\
    if (!IS_DESTRUCTED(&k->ind) && RECURSE_FN(&k->ind, 1)) {		\
      DO_IF_DEBUG(Pike_fatal("Didn't expect an svalue zapping now.\n")); \
    }									\
    RECURSE_FN(&k->val, 1);						\
    VAL_TYPES |= 1 << k->val.type;					\
  }									\
} while (0)

#define GC_RECURSE(M, MD, REC_KEYPAIR, TYPE, IND_TYPES, VAL_TYPES) do {	\
  INT32 e;								\
  int remove;								\
  struct keypair *k,**prev;						\
  /* no locking required (no is_eq) */					\
  for(e=0;e<MD->hashsize;e++)						\
  {									\
    for(prev= MD->hash + e;(k=*prev);)					\
    {									\
      REC_KEYPAIR(remove,						\
		  PIKE_CONCAT(TYPE, _svalues),				\
		  PIKE_CONCAT(TYPE, _weak_svalues),			\
		  PIKE_CONCAT(TYPE, _without_recurse),			\
		  PIKE_CONCAT(TYPE, _weak_without_recurse));		\
      if (remove)							\
      {									\
	*prev=k->next;							\
        FREE_KEYPAIR(MD, k);						\
        MD->free_list->ind.type = T_INT;				\
        MD->free_list->val.type = T_INT;				\
	MD->size--;							\
	DO_IF_MAPPING_SIZE_DEBUG(					\
	  if(M->data ==MD)						\
	    M->debug_size--;						\
	);								\
      }else{								\
	VAL_TYPES |= 1 << k->val.type;					\
	IND_TYPES |= 1 << k->ind.type;					\
	prev=&k->next;							\
      }									\
    }									\
  }									\
} while (0)

#define GC_REC_KP(REMOVE, N_REC, W_REC, N_TST, W_TST) do {		\
  if ((REMOVE = N_REC(&k->ind, 1)))					\
    gc_free_svalue(&k->val);						\
  else									\
    N_REC(&k->val, 1);							\
} while (0)

#define GC_REC_KP_IND(REMOVE, N_REC, W_REC, N_TST, W_TST) do {		\
  if ((REMOVE = W_REC(&k->ind, 1)))					\
    gc_free_svalue(&k->val);						\
  else									\
    N_REC(&k->val, 1);							\
} while (0)

#define GC_REC_KP_VAL(REMOVE, N_REC, W_REC, N_TST, W_TST) do {		\
  if ((REMOVE = N_TST(&k->ind))) /* Don't recurse now. */		\
    gc_free_svalue(&k->val);						\
  else if ((REMOVE = W_REC(&k->val, 1)))				\
    gc_free_svalue(&k->ind);						\
  else									\
    N_REC(&k->ind, 1);		/* Now we can recurse the index. */	\
} while (0)

#define GC_REC_KP_BOTH(REMOVE, N_REC, W_REC, N_TST, W_TST) do {		\
  if ((REMOVE = W_TST(&k->ind))) /* Don't recurse now. */		\
    gc_free_svalue(&k->val);						\
  else if ((REMOVE = W_REC(&k->val, 1)))				\
    gc_free_svalue(&k->ind);						\
  else									\
    W_REC(&k->ind, 1);		/* Now we can recurse the index. */	\
} while (0)

void gc_mark_mapping_as_referenced(struct mapping *m)
{
#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
#endif
  debug_malloc_touch(m);
  debug_malloc_touch(m->data);

  if(gc_mark(m))
    GC_ENTER (m, T_MAPPING) {
      struct mapping_data *md = m->data;

      if (m == gc_mark_mapping_pos)
	gc_mark_mapping_pos = m->next;
      if (m == gc_internal_mapping)
	gc_internal_mapping = m->next;
      else {
	DOUBLEUNLINK(first_mapping, m);
	DOUBLELINK(first_mapping, m); /* Linked in first. */
      }

      if(gc_mark(md) && ((md->ind_types | md->val_types) & BIT_COMPLEX)) {
	TYPE_FIELD ind_types = 0, val_types = 0;
	if (MAPPING_DATA_IN_USE(md)) {
	  /* Must leave the mapping data untouched if it's busy. */
	  debug_malloc_touch(m);
	  debug_malloc_touch(md);
	  GC_RECURSE_MD_IN_USE(md, gc_mark_svalues, ind_types, val_types);
	  gc_assert_checked_as_nonweak(md);
	}
	else
	  switch (md->flags & MAPPING_WEAK) {
	    case 0:
	      debug_malloc_touch(m);
	      debug_malloc_touch(md);
	      GC_RECURSE(m, md, GC_REC_KP, gc_mark, ind_types, val_types);
	      gc_assert_checked_as_nonweak(md);
	      break;
	    case MAPPING_WEAK_INDICES:
	      debug_malloc_touch(m);
	      debug_malloc_touch(md);
	      GC_RECURSE(m, md, GC_REC_KP_IND, gc_mark, ind_types, val_types);
	      gc_assert_checked_as_weak(md);
	      break;
	    case MAPPING_WEAK_VALUES:
	      debug_malloc_touch(m);
	      debug_malloc_touch(md);
	      GC_RECURSE(m, md, GC_REC_KP_VAL, gc_mark, ind_types, val_types);
	      gc_assert_checked_as_weak(md);
	      break;
	    default:
	      debug_malloc_touch(m);
	      debug_malloc_touch(md);
	      GC_RECURSE(m, md, GC_REC_KP_BOTH, gc_mark, ind_types, val_types);
	      gc_assert_checked_as_weak(md);
	      break;
	  }
	md->val_types = val_types;
	md->ind_types = ind_types;
      }
    } GC_LEAVE;
}

void real_gc_cycle_check_mapping(struct mapping *m, int weak)
{
  GC_CYCLE_ENTER(m, T_MAPPING, weak) {
    struct mapping_data *md = m->data;
    debug_malloc_touch(m);
    debug_malloc_touch(md);

#ifdef PIKE_DEBUG
    if(md->refs <=0)
      Pike_fatal("Zero refs in mapping->data\n");
#endif

    if ((md->ind_types | md->val_types) & BIT_COMPLEX) {
      TYPE_FIELD ind_types = 0, val_types = 0;
      if (MAPPING_DATA_IN_USE(md)) {
	/* Must leave the mapping data untouched if it's busy. */
	debug_malloc_touch(m);
	debug_malloc_touch(md);
	GC_RECURSE_MD_IN_USE(md, gc_cycle_check_svalues, ind_types, val_types);
	gc_assert_checked_as_nonweak(md);
      }
      else
	switch (md->flags & MAPPING_WEAK) {
	  case 0:
	    debug_malloc_touch(m);
	    debug_malloc_touch(md);
	    GC_RECURSE(m, md, GC_REC_KP, gc_cycle_check, ind_types, val_types);
	    gc_assert_checked_as_nonweak(md);
	    break;
	  case MAPPING_WEAK_INDICES:
	    debug_malloc_touch(m);
	    debug_malloc_touch(md);
	    GC_RECURSE(m, md, GC_REC_KP_IND, gc_cycle_check, ind_types, val_types);
	    gc_assert_checked_as_weak(md);
	    break;
	  case MAPPING_WEAK_VALUES:
	    debug_malloc_touch(m);
	    debug_malloc_touch(md);
	    GC_RECURSE(m, md, GC_REC_KP_VAL, gc_cycle_check, ind_types, val_types);
	    gc_assert_checked_as_weak(md);
	    break;
	  default:
	    debug_malloc_touch(m);
	    debug_malloc_touch(md);
	    GC_RECURSE(m, md, GC_REC_KP_BOTH, gc_cycle_check, ind_types, val_types);
	    gc_assert_checked_as_weak(md);
	    break;
	}
      md->val_types = val_types;
      md->ind_types = ind_types;
    }
  } GC_CYCLE_LEAVE;
}

static void gc_check_mapping(struct mapping *m)
{
  struct mapping_data *md = m->data;

  if((md->ind_types | md->val_types) & BIT_COMPLEX)
    GC_ENTER (m, T_MAPPING) {
      INT32 e;
      struct keypair *k;

      if(!debug_gc_check (md, " as mapping data block of a mapping")) {
	if (!(md->flags & MAPPING_WEAK) || MAPPING_DATA_IN_USE(md))
	  /* Disregard the weak flag if the mapping data is busy; we must
	   * leave it untouched in that case anyway. */
	  NEW_MAPPING_LOOP(md)
	  {
	    debug_gc_check_svalues(&k->ind, 1, " as mapping index");
	    debug_gc_check_svalues(&k->val, 1, " as mapping value");
	  }
	else {
	  switch (md->flags & MAPPING_WEAK) {
	    case MAPPING_WEAK_INDICES:
	      NEW_MAPPING_LOOP(md)
	      {
		debug_gc_check_weak_svalues(&k->ind, 1, " as mapping index");
		debug_gc_check_svalues(&k->val, 1, " as mapping value");
	      }
	      break;
	    case MAPPING_WEAK_VALUES:
	      NEW_MAPPING_LOOP(md)
	      {
		debug_gc_check_svalues(&k->ind, 1, " as mapping index");
		debug_gc_check_weak_svalues(&k->val, 1, " as mapping value");
	      }
	      break;
	    default:
	      NEW_MAPPING_LOOP(md)
	      {
		debug_gc_check_weak_svalues(&k->ind, 1, " as mapping index");
		debug_gc_check_weak_svalues(&k->val, 1, " as mapping value");
	      }
	      break;
	  }
	  gc_checked_as_weak(md);
	}
      }
    } GC_LEAVE;
}

unsigned gc_touch_all_mappings(void)
{
  unsigned n = 0;
  struct mapping *m;
  if (first_mapping && first_mapping->prev)
    Pike_fatal("Error in mapping link list.\n");
  for (m = first_mapping; m; m = m->next) {
    debug_gc_touch(m);
    n++;
    if (m->next && m->next->prev != m)
      Pike_fatal("Error in mapping link list.\n");
  }
  return n;
}

void gc_check_all_mappings(void)
{
  struct mapping *m;

  for(m=first_mapping;m;m=m->next)
  {
#ifdef DEBUG_MALLOC
    if (((int)m->data) == 0x55555555) {
      fprintf(stderr, "** Zapped mapping in list of active mappings!\n");
      describe_something(m, T_MAPPING, 0,2,0, NULL);
      Pike_fatal("Zapped mapping in list of active mappings!\n");
    }
#endif /* DEBUG_MALLOC */

    gc_check_mapping(m);
#ifdef PIKE_DEBUG
    if(d_flag > 1) check_mapping_type_fields(m);
#endif
  }
}

void gc_mark_all_mappings(void)
{
  gc_mark_mapping_pos = gc_internal_mapping;
  while (gc_mark_mapping_pos) {
    struct mapping *m = gc_mark_mapping_pos;
    gc_mark_mapping_pos = m->next;

    debug_malloc_touch(m);
    debug_malloc_touch(m->data);

    if(gc_is_referenced(m))
      gc_mark_mapping_as_referenced(m);
  }
}

void gc_cycle_check_all_mappings(void)
{
  struct mapping *m;
  for (m = gc_internal_mapping; m; m = m->next) {
    real_gc_cycle_check_mapping(m, 0);
    gc_cycle_run_queue();
  }
}

void gc_zap_ext_weak_refs_in_mappings(void)
{
  gc_mark_mapping_pos = first_mapping;
  while (gc_mark_mapping_pos != gc_internal_mapping && gc_ext_weak_refs) {
    struct mapping *m = gc_mark_mapping_pos;
    gc_mark_mapping_pos = m->next;
    gc_mark_mapping_as_referenced(m);
  }
  gc_mark_discard_queue();
}

size_t gc_free_all_unreferenced_mappings(void)
{
  struct mapping *m,*next;
  struct mapping_data *md;
  size_t unreferenced = 0;

  for(m=gc_internal_mapping;m;m=next)
  {
    debug_malloc_touch(m);
    debug_malloc_touch(m->data);

    if(gc_do_free(m))
    {
      /* Got an extra ref from gc_cycle_pop(). */
      md = m->data;

      debug_malloc_touch(m);
      debug_malloc_touch(md);

      /* Protect against unlink_mapping_data() recursing too far. */
      m->data=&empty_data;
      add_ref(m->data);

      unlink_mapping_data(md);
#ifdef MAPPING_SIZE_DEBUG
      m->debug_size=0;
#endif
      gc_free_extra_ref(m);
      SET_NEXT_AND_FREE(m, free_mapping);
    }
    else
    {
      next=m->next;
    }
    unreferenced++;
  }

  return unreferenced;
}

#ifdef PIKE_DEBUG

void simple_describe_mapping(struct mapping *m)
{
  dynamic_buffer save_buf;
  char *s;
  init_buf(&save_buf);
  describe_mapping(m,0,2);
  s=simple_free_buf(&save_buf);
  fprintf(stderr,"%s\n",s);
  free(s);
}


void debug_dump_mapping(struct mapping *m)
{
  fprintf(stderr, "Refs=%d, next=%p, prev=%p",
	  m->refs, m->next, m->prev);
  if (((ptrdiff_t)m->data) & 3) {
    fprintf(stderr, ", data=%p (unaligned)\n", m->data);
  } else {
    fprintf(stderr, ", flags=0x%x, size=%d, hashsize=%d\n",
	    m->data->flags, m->data->size, m->data->hashsize);
    fprintf(stderr, "Indices type field =");
    debug_dump_type_field(m->data->ind_types);
    fprintf(stderr, "\n");
    fprintf(stderr, "Values type field =");
    debug_dump_type_field(m->data->val_types);
    fprintf(stderr, "\n");
    simple_describe_mapping(m);
  }
}
#endif

int mapping_is_constant(struct mapping *m,
			struct processing *p)
{
  int ret=1;
  INT32 e;
  struct keypair *k;
  struct mapping_data *md=m->data;

  if( (md->ind_types | md->val_types) & ~(BIT_INT|BIT_FLOAT|BIT_STRING))
  {
    md->valrefs++;
    add_ref(md);
    NEW_MAPPING_LOOP(md)
      {
	if(!svalues_are_constant(&k->ind, 1, md->ind_types, p) ||
	   !svalues_are_constant(&k->val, 1, md->val_types, p))
	{
	  ret=0;
	  e=md->hashsize;
	  break;
	}
      }
    md->valrefs--;
    free_mapping_data(md);
  }
  return ret;
}
