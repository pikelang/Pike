/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "main.h"
#include "object.h"
#include "mapping.h"
#include "svalue.h"
#include "array.h"
#include "pike_macros.h"
#include "pike_error.h"
#include "pike_memory.h"
#include "pike_types.h"
#include "buffer.h"
#include "interpret.h"
#include "las.h"
#include "gc.h"
#include "stralloc.h"
#include "block_allocator.h"
#include "opcodes.h"
#include "pike_cpulib.h"
#include "bignum.h"

/* Average number of keypairs per slot when allocating. */
#define AVG_LINK_LENGTH 4

/* Minimum number of elements in a hashtable is half of the slots. */
#define MIN_LINK_LENGTH_NUMERATOR	1
#define MIN_LINK_LENGTH_DENOMINATOR	2

/* Number of keypairs to allocate for a given size. */
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

static struct block_allocator mapping_allocator = BA_INIT_PAGES(sizeof(struct mapping), 2);
void count_memory_in_mappings(size_t * num, size_t * size) {
    struct mapping *m;
    double datasize = 0.0;
    ba_count_all(&mapping_allocator, num, size);
    for(m=first_mapping;m;m=m->next) {
	datasize+=MAPPING_DATA_SIZE(m->data->hashsize, m->data->num_keypairs) / (double) m->data->refs;
    }
    *size += (size_t) datasize;
}

void really_free_mapping(struct mapping * m) {
#ifdef PIKE_DEBUG
  if (m->refs) {
# ifdef DEBUG_MALLOC
    describe_something(m, T_MAPPING, 0,2,0, NULL);
# endif
    Pike_fatal("really free mapping on mapping with %d refs.\n", m->refs);
  }
#endif
  unlink_mapping_data(m->data);
  DOUBLEUNLINK(first_mapping, m);
  GC_FREE(m);
  ba_free(&mapping_allocator, m);
}

ATTRIBUTE((malloc))
static struct mapping * alloc_mapping(void) {
    return ba_alloc(&mapping_allocator);
}

void free_all_mapping_blocks(void) {
    ba_destroy(&mapping_allocator);
}

#ifndef PIKE_MAPPING_KEYPAIR_LOOP
#define IF_ELSE_KEYPAIR_LOOP(X, Y)	Y
#define FREE_KEYPAIR(md, k) do {	\
    k->next = md->free_list;		\
    md->free_list = k;			\
  } while(0)
#else /* PIKE_MAPPING_KEYPAIR_LOOP */
#define IF_ELSE_KEYPAIR_LOOP(X, Y)	X
#define FREE_KEYPAIR(md, k) do {			\
    md->free_list--;					\
    if (k != md->free_list) {				\
      struct keypair **prev_;				\
      unsigned INT32 h_;				\
      INT32 e;						\
      /* Move the last keypair to the new hole. */	\
      *k = *(md->free_list);				\
      h_ = k->hval & ( md->hashsize - 1);		\
      prev_ = md->hash + h_;				\
      DO_IF_DEBUG(					\
	if (!*prev_) {					\
	  Pike_fatal("Node to move not found!\n");	\
	}						\
      );						\
      while (*prev_ != md->free_list) {			\
	prev_ = &((*prev_)->next);			\
        DO_IF_DEBUG(					\
	  if (!*prev_) {				\
	    Pike_fatal("Node to move not found!\n");	\
	  }						\
	);						\
      }							\
      *prev_ = k;					\
    }							\
  } while(0)
#endif /* !PIKE_MAPPING_KEYPAIR_LOOP */

void mapping_free_keypair(struct mapping_data *md, struct keypair *k)
{
  FREE_KEYPAIR(md, k);
}

static inline int check_type_contains(TYPE_FIELD types, const struct svalue * s) {
    return (TYPEOF(*s) == PIKE_T_OBJECT || types & (BIT_OBJECT|(1 << TYPEOF(*s))));
}

static inline int check_type_overlaps(TYPE_FIELD t1, TYPE_FIELD t2) {
    return (!t1 && !t2) || t1 & t2 || (t1|t2) & BIT_OBJECT;
}

#ifdef PIKE_DEBUG

/** This function checks that the type field isn't lacking any bits.
 * It is used for debugging purposes only.
 */
static void check_mapping_type_fields(const struct mapping *m)
{
  INT32 e;
  const struct keypair *k=0;
  const struct mapping_data *md;
  TYPE_FIELD ind_types, val_types;

  ind_types=val_types=0;

  md = m->data;
  NEW_MAPPING_LOOP(md)
    {
      if (TYPEOF(k->val) > MAX_TYPE)
	Pike_fatal("Invalid mapping keypair value type: %s\n",
		   get_name_of_type(TYPEOF(k->val)));
      val_types |= 1 << TYPEOF(k->val);
      if (!(val_types & BIT_INT) && (TYPEOF(k->val) == PIKE_T_OBJECT)) {
        if (is_bignum_object(k->val.u.object)) {
          val_types |= BIT_INT;
        }
      }
      if (TYPEOF(k->ind) > MAX_TYPE)
	Pike_fatal("Invalid maping keypair index type: %s\n",
		   get_name_of_type(TYPEOF(k->ind)));
      ind_types |= 1 << TYPEOF(k->ind);
      if (!(ind_types & BIT_INT) && (TYPEOF(k->ind) == PIKE_T_OBJECT)) {
        if (is_bignum_object(k->ind.u.object)) {
          ind_types |= BIT_INT;
        }
      }
    }

  if(val_types & ~(m->data->val_types))
    Pike_fatal("Mapping value types out of order!\n");

  if(ind_types & ~(m->data->ind_types))
    Pike_fatal("Mapping indices types out of order!\n");
}
#endif

static struct mapping_data empty_data =
  { PIKE_CONSTANT_MEMOBJ_INIT(1, T_MAPPING_DATA), 1, 0,0,0,0,0,0, 0,
    IF_ELSE_KEYPAIR_LOOP((struct keypair *)&empty_data.hash, 0), {0}};
static struct mapping_data weak_ind_empty_data =
  { PIKE_CONSTANT_MEMOBJ_INIT(1, T_MAPPING_DATA), 1, 0,0,0,0,0,0, MAPPING_WEAK_INDICES,
    IF_ELSE_KEYPAIR_LOOP((struct keypair *)&weak_ind_empty_data.hash, 0), {0}};
static struct mapping_data weak_val_empty_data =
  { PIKE_CONSTANT_MEMOBJ_INIT(1, T_MAPPING_DATA), 1, 0,0,0,0,0,0, MAPPING_WEAK_VALUES,
    IF_ELSE_KEYPAIR_LOOP((struct keypair *)&weak_val_empty_data.hash, 0), {0}};
static struct mapping_data weak_both_empty_data =
  { PIKE_CONSTANT_MEMOBJ_INIT(1, T_MAPPING_DATA), 1, 0,0,0,0,0,0, MAPPING_WEAK,
    IF_ELSE_KEYPAIR_LOOP((struct keypair *)&weak_both_empty_data.hash, 0), {0}};

/*
 * This rounds an integer up to the next power of two. For x a power
 * of two, this will just return the same again.
 */
static unsigned INT32 find_next_power(unsigned INT32 x)
{
    if( x == 0 ) return 1;
    return 1<<(my_log2(x-1)+1);
}

/** This function allocates the hash table and svalue space for a mapping
 * struct. The size is the max number of indices that can fit in the
 * allocated space.
 */
static void init_mapping(struct mapping *m,
			 INT32 hashsize,
			 INT16 flags)
{
  struct mapping_data *md;
  ptrdiff_t e;
  INT32 size;

  debug_malloc_touch(m);
#ifdef PIKE_DEBUG
  if (Pike_in_gc > GC_PASS_PREPARE && Pike_in_gc < GC_PASS_ZAP_WEAK)
    Pike_fatal("Can't allocate a new mapping_data inside gc.\n");
  if(hashsize < 0) Pike_fatal("init_mapping with negative value.\n");
#endif
  if(hashsize)
  {
    if (hashsize & (hashsize - 1)) {
      hashsize = find_next_power(hashsize);
    }

    size = hashsize * AVG_LINK_LENGTH;

    e=MAPPING_DATA_SIZE(hashsize, size);

    md=xcalloc(1,e);

    m->data=md;
    md->hashsize=hashsize;

    md->free_list=MD_KEYPAIRS(md, hashsize);
#ifndef PIKE_MAPPING_KEYPAIR_LOOP
    for(e=1;e<size;e++)
    {
      md->free_list[e-1].next = md->free_list + e;
      mark_free_svalue (&md->free_list[e-1].ind);
      mark_free_svalue (&md->free_list[e-1].val);
    }
    /* md->free_list[e-1].next=0; */
    mark_free_svalue (&md->free_list[e-1].ind);
    mark_free_svalue (&md->free_list[e-1].val);
#endif /* !PIKE_MAPPING_KEYPAIR_LOOP */
    /* md->ind_types = 0; */
    /* md->val_types = 0; */
    md->flags = flags;
    /* md->size = 0; */
    /* md->refs=0; */
    /* md->valrefs=0; */
    /* md->hardlinks=0; */
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
  gc_init_marker(md);
  m->data=md;
#ifdef MAPPING_SIZE_DEBUG
  m->debug_size = md->size;
#endif
}

static struct mapping *allocate_mapping_no_init(void)
{
  struct mapping *m=alloc_mapping();
  GC_ALLOC(m);
  m->refs = 0;
  add_ref(m);	/* For DMALLOC... */
  DOUBLELINK(first_mapping, m);
  return m;
}

/** This function allocates an empty mapping with initial room
 * for 'size' values.
 *
 * @param size initial number of values
 * @return the newly allocated mapping
 * @see do_free_mapping
 * @see free_mapping
 */
PMOD_EXPORT struct mapping *debug_allocate_mapping(int size)
{
  struct mapping *m = allocate_mapping_no_init();
  init_mapping(m, (size + AVG_LINK_LENGTH - 1) / AVG_LINK_LENGTH, 0);
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

  if (!md->size) {
    /* Paranoia and keep gcc happy. */
    if (md == &empty_data) {
      Pike_fatal("really_free_mapping_data(): md is empty_data!\n");
    }
    if (md == &weak_ind_empty_data) {
      Pike_fatal("really_free_mapping_data(): md is weak_ind_empty_data!\n");
    }
    if (md == &weak_val_empty_data) {
      Pike_fatal("really_free_mapping_data(): md is weak_val_empty_data!\n");
    }
    if (md == &weak_both_empty_data) {
      Pike_fatal("really_free_mapping_data(): md is weak_both_empty_data!\n");
    }
  }
#endif /* PIKE_DEBUG */

  NEW_MAPPING_LOOP(md)
  {
    free_svalue(& k->val);
    free_svalue(& k->ind);
  }

  free(md);
  GC_FREE_BLOCK(md);
}

PMOD_EXPORT void do_free_mapping(struct mapping *m)
{
  if (m)
    inl_free_mapping(m);
}

/* This function is used to rehash a mapping without losing the internal
 * order in each hash chain. This is to prevent mappings from becoming
 * inefficient just after being rehashed.
 */
/* Evil: Steal the svalues from the old md. */
static void mapping_rehash_backwards_evil(struct mapping_data *md,
					  struct keypair *from)
{
  unsigned INT32 h;
  struct keypair *k, *prev = NULL, *next;

  if(!(k = from)) return;

  /* Reverse the hash chain. */
  while ((next = k->next)) {
    k->next = prev;
    prev = k;
    k = next;
  }
  k->next = prev;

  prev = k;

  /* Rehash and reverse the hash chain. */
  while ((from = prev)) {
    /* Reverse */
    prev = from->next;
    from->next = next;
    next = from;

    if (md->flags & MAPPING_WEAK) {

      switch(md->flags & MAPPING_WEAK) {
      default:
	Pike_fatal("Instable mapping data flags.\n");
	break;
      case MAPPING_WEAK_INDICES:
	if (!REFCOUNTED_TYPE(TYPEOF(from->ind)) ||
	    (*from->ind.u.refs > 1)) {
	  goto keep_keypair;
	}
	break;
      case MAPPING_WEAK_VALUES:
	if (!REFCOUNTED_TYPE(TYPEOF(from->val)) ||
	    (*from->val.u.refs > 1)) {
	  goto keep_keypair;
	}
	break;
      case MAPPING_WEAK:
	/* NB: Compat: Unreference counted values are counted
	 *             as multi-referenced here.
	 */
	if ((!REFCOUNTED_TYPE(TYPEOF(from->ind)) ||
	     (*from->ind.u.refs > 1)) &&
	    (!REFCOUNTED_TYPE(TYPEOF(from->val)) ||
	     (*from->val.u.refs > 1))) {
	  goto keep_keypair;
	}
	break;
      }

      /* Free.
       * Note that we don't need to free or unlink the keypair,
       * since that will be done by the caller anyway. */
      free_svalue(&from->ind);
      free_svalue(&from->val);

      continue;
    }
  keep_keypair:

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
    h&=md->hashsize - 1;
    k->next=md->hash[h];
    md->hash[h]=k;

    /* update */
    md->ind_types |= 1<< (TYPEOF(k->ind));
    if (!(md->ind_types & BIT_INT) && (TYPEOF(k->ind) == PIKE_T_OBJECT)) {
      if (is_bignum_object(k->ind.u.object)) {
        md->ind_types |= BIT_INT;
      }
    }
    md->val_types |= 1<< (TYPEOF(k->val));
    if (!(md->val_types & BIT_INT) && (TYPEOF(k->val) == PIKE_T_OBJECT)) {
      if (is_bignum_object(k->val.u.object)) {
        md->val_types |= BIT_INT;
      }
    }
    md->size++;
  }
}

/* Good: Copy the svalues from the old md. */
static void mapping_rehash_backwards_good(struct mapping_data *md,
					  struct keypair *from)
{
  unsigned INT32 h;
  struct keypair *k, *prev = NULL, *next;

  if(!(k = from)) return;

  /* Reverse the hash chain. */
  while ((next = k->next)) {
    k->next = prev;
    prev = k;
    k = next;
  }
  k->next = prev;

  prev = k;

  /* Rehash and reverse the hash chain. */
  while ((from = prev)) {
    /* Reverse */
    prev = from->next;
    from->next = next;
    next = from;

    if (md->flags & MAPPING_WEAK) {

      switch(md->flags & MAPPING_WEAK) {
      default:
	Pike_fatal("Instable mapping data flags.\n");
	break;
      case MAPPING_WEAK_INDICES:
	if (REFCOUNTED_TYPE(TYPEOF(from->ind)) &&
	    (*from->ind.u.refs > 1)) {
	  goto keep_keypair;
	}
	break;
      case MAPPING_WEAK_VALUES:
	if (REFCOUNTED_TYPE(TYPEOF(from->val)) &&
	    (*from->val.u.refs > 1)) {
	  goto keep_keypair;
	}
	break;
      case MAPPING_WEAK:
	/* NB: Compat: Unreference counted values are counted
	 *             as multi-referenced here.
	 */
	if ((!REFCOUNTED_TYPE(TYPEOF(from->ind)) ||
	     (*from->ind.u.refs > 1)) &&
	    (!REFCOUNTED_TYPE(TYPEOF(from->val)) ||
	     (*from->val.u.refs > 1))) {
	  goto keep_keypair;
	}
	break;
      }

      /* Skip copying of this keypair.
       *
       * NB: We can't mess with the original md here,
       *     since it might be in use by an iterator
       *     or similar.
       */

      continue;
    }
  keep_keypair:

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
    h&=md->hashsize - 1;
    k->next=md->hash[h];
    md->hash[h]=k;

    /* update */
    md->ind_types |= 1<< (TYPEOF(k->ind));
    if (!(md->ind_types & BIT_INT) && (TYPEOF(k->ind) == PIKE_T_OBJECT)) {
      if (is_bignum_object(k->ind.u.object)) {
        md->ind_types |= BIT_INT;
      }
    }
    md->val_types |= 1<< (TYPEOF(k->val));
    if (!(md->val_types & BIT_INT) && (TYPEOF(k->val) == PIKE_T_OBJECT)) {
      if (is_bignum_object(k->val.u.object)) {
        md->val_types |= BIT_INT;
      }
    }
    md->size++;
  }
}

/** This function re-allocates a mapping. It adjusts the max no. of
 * values can be fitted into the mapping. It takes a bit of time to
 * run, but is used seldom enough not to degrade preformance significantly.
 *
 * @param m the mapping to be rehashed
 * @param hashsize new mappingsize
 * @return the rehashed mapping
 */
static struct mapping *rehash(struct mapping *m, int hashsize)
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

  /* NB: Code duplication from init_mapping(). */
  if (hashsize & (hashsize - 1))
    hashsize = find_next_power(hashsize);
  if ((md->hashsize == hashsize) && (md->refs == 1)
      /* FIXME: Paranoia check below; is this needed? */
      && !MD_FULLP(md))
    return m;

  init_mapping(m, hashsize, md->flags);
  debug_malloc_touch(m);
  new_md=m->data;

  /* This operation is now 100% atomic - no locking required */
  if(md->refs>1)
  {
    /* good */
    /* More than one reference to the md ==> We need to
     * keep it afterwards.
     */
    for(e=0;e<md->hashsize;e++)
      mapping_rehash_backwards_good(new_md, md->hash[e]);

    unlink_mapping_data(md);
  }else{
    /* evil */
    /* We have the only reference to the md,
     * so we can just copy the svalues without
     * bothering about type checking.
     */
    for(e=0;e<md->hashsize;e++)
      mapping_rehash_backwards_evil(new_md, md->hash[e]);

    free(md);
    GC_FREE_BLOCK(md);
  }

#ifdef PIKE_DEBUG
  if((m->data->size != tmp) &&
     ((m->data->size > tmp) || !(m->data->flags & MAPPING_WEAK)))
    Pike_fatal("Rehash failed, size not same any more (%ld != %ld).\n",
	       (long)m->data->size, (long)tmp);
#endif
#ifdef MAPPING_SIZE_DEBUG
  m->debug_size = m->data->size;
#endif

#ifdef PIKE_DEBUG
  if(d_flag>1)  check_mapping(m);
#endif

  return m;
}

ptrdiff_t do_gc_weak_mapping(struct mapping *m)
{
  struct mapping_data *md = m->data;
  ptrdiff_t initial_size = md->size;
  INT32 e;

  if (!(md->flags & MAPPING_WEAK) || (md->refs != 1)) {
    // Nothing to do.
    return 0;
  }

  for (e = 0; e < md->hashsize; e++) {
    struct keypair **kp = md->hash + e;
    struct keypair *k;
    while ((k = *kp)) {
      switch(md->flags & MAPPING_WEAK) {
      default:
	Pike_fatal("Unstable mapping data flags.\n");
	break;
      case MAPPING_WEAK_INDICES:
	if (REFCOUNTED_TYPE(TYPEOF(k->ind)) && (*k->ind.u.refs > 1)) {
	  kp = &k->next;
	  continue;
	}
	break;
      case MAPPING_WEAK_VALUES:
	if (REFCOUNTED_TYPE(TYPEOF(k->val)) && (*k->val.u.refs > 1)) {
	  kp = &k->next;
	  continue;
	}
	break;
      case MAPPING_WEAK:
	/* NB: Compat: Unreferenced counted values are counted
	 *             as multi-referenced here.
	 */
	if ((!REFCOUNTED_TYPE(TYPEOF(k->ind)) || (*k->ind.u.refs > 1)) &&
	    (!REFCOUNTED_TYPE(TYPEOF(k->val)) || (*k->val.u.refs > 1))) {
	  kp = &k->next;
	  continue;
	}
	break;
      }
      /* Unlink. */
      *kp = k->next;

      /* Free. */
      free_svalue(&k->ind);
      free_svalue(&k->val);

      k->next = md->free_list;
      md->free_list = k;
      md->size--;

      /* Note: Do not advance kp here, as it has been done by the unlink. */
    }
  }
  return initial_size - md->size;
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
  memcpy(nmd, md, size);
  gc_init_marker(nmd);
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
  nmd->ind_types = md->ind_types;
  nmd->val_types = md->val_types;

  /* FIXME: What about nmd->flags? */

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

/**
 * void LOW_FIND(int (*FUN)(const struct svalue *IND, const struct svalue *KEY),
 *               const struct svalue *KEY,
 *               CODE_IF_FOUND,
 *               CODE_IF_NOT_FOUND)
 *
 * Needs local variables:
 *   struct mapping *m;
 *   struct mapping_data *md;
 *   size_t h, h2;
 *   struct svalue *key;
 *   struct keypair *k, **prev;
 *
 * CODE_IF_FOUND must end with a return, goto or longjmp to
 * avoid CODE_IF_NOT_FOUND being called afterwards, or for
 * potential multiple matches.
 *
 * On entry:
 *   m is the mapping to search.
 *   key === KEY.
 *   h2 = hash_svalue(key).
 *
 * Internals:
 *   md = m->data.
 *   md gets an extra ref.
 *
 * When CODE_IF_FOUND gets executed k points to the matching keypair.
 */
#define LOW_FIND(FUN, KEY, FOUND, NOT_FOUND) do {		\
  md=m->data;                                                   \
  add_ref(md);						        \
  if(md->hashsize)						\
  {								\
    h=h2 & (md->hashsize - 1);					\
    DO_IF_DEBUG( if(d_flag > 1) check_mapping_type_fields(m); ) \
    if(check_type_contains(md->ind_types, key))			\
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
    h=h2 & (md->hashsize-1);					\
    DO_IF_DEBUG( if(d_flag > 1) check_mapping_type_fields(m); ) \
    if(check_type_contains(md->ind_types, key))			\
    {								\
      k2=omd->hash[h2 & (omd->hashsize - 1)];			        \
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

#if 0
#define PROPAGATE() do {			\
   if(md->refs==1)				\
   {						\
     h=h2 & (md->hashsize - 1);                 \
     *prev=k->next;				\
     k->next=md->hash[h];			\
     md->hash[h]=k;				\
   }						\
 }while(0)
#else
#define PROPAGATE()
#endif


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

/** This function brings the type fields in the mapping up to speed.
 * It should be used only when the type fields MUST be correct, which is not
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
      val_types |= 1 << TYPEOF(k->val);
      if (!(val_types & BIT_INT) && (TYPEOF(k->val) == PIKE_T_OBJECT)) {
        if (is_bignum_object(k->val.u.object)) {
          val_types |= BIT_INT;
        }
      }
      ind_types |= 1 << TYPEOF(k->ind);
      if (!(ind_types & BIT_INT) && (TYPEOF(k->ind) == PIKE_T_OBJECT)) {
        if (is_bignum_object(k->ind.u.object)) {
          ind_types |= BIT_INT;
        }
      }
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

PMOD_EXPORT void low_mapping_insert(struct mapping *m,
				    const struct svalue *key,
				    const struct svalue *val,
				    int overwrite)
{
  size_t h,h2;
  struct keypair *k, **prev;
  struct mapping_data *md, *omd;
  int grow_md;
  int refs;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
#endif

  h2=hash_svalue(key);

#ifdef PIKE_DEBUG
  if(d_flag>1)  check_mapping(m);
#endif

  LOW_FIND(is_eq, key,
	   /* Key found. */
	   struct svalue *tmp=&k->ind;
	   SAME_DATA(goto mi_set_value,
		     /* Mapping data has changed. */
		     omd=md;
		     LOW_FIND(is_identical, tmp,
			      /* The key is still unmodified in the mapping. */
			      free_mapping_data(omd);
			      goto mi_set_value,
			      /* The key has changed??? */
			      free_mapping_data(omd);
			      goto mi_do_nothing)),
	   /* Key not found. */
	   Z:
	   SAME_DATA(goto mi_insert,
		     /* Mapping data has changed. */
		     omd=md;
		     LOW_FIND2(is_eq, key,
			       /* The key has appeared in the mapping??? */
			       free_mapping_data(omd);
			       goto mi_do_nothing,
			       /* The key is still not in the mapping. */
			       free_mapping_data(omd);
			       goto Z)));

 mi_do_nothing:
  /* Don't do anything??? */
  free_mapping_data(md);
  return;

 mi_set_value:
#ifdef PIKE_DEBUG
  if(m->data != md)
    Pike_fatal("Wrong dataset in mapping_insert!\n");
  if(d_flag>1)  check_mapping(m);
#endif
  /* NB: We know that md has a reference from the mapping
   *     in addition to our reference.
   *
   * The use of sub_ref() silences warnings from Coverity, as well as
   * on the off chance of a reference counting error avoids accessing
   * freed memory.
   */
  refs = sub_ref(md);	/* free_mapping_data(md); */
  assert(refs);

  if(!overwrite) return;
  PREPARE_FOR_DATA_CHANGE2();
  PROPAGATE(); /* propagate after preparing */
  md->val_types |= 1 << TYPEOF(*val);
  if (overwrite == 2 && TYPEOF(*key) == T_OBJECT)
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
  /* NB: We know that md has a reference from the mapping
   *     in addition to our reference.
   *
   * The use of sub_ref() silences warnings from Coverity, as well as
   * on the off chance of a reference counting error avoids accessing
   * freed memory.
   */
  refs = sub_ref(md);	/* free_mapping_data(md); */
  assert(refs);

  grow_md = MD_FULLP(md);

  /* We do a re-hash here instead of copying the mapping. */
  if(grow_md || md->refs>1)
  {
    debug_malloc_touch(m);
    rehash(m, md->hashsize ? md->hashsize << grow_md : AVG_LINK_LENGTH);
    md=m->data;
  }
  h=h2 & ( md->hashsize - 1);

  /* no need to lock here since we are not calling is_eq - Hubbe */

  k=md->free_list;
#ifndef PIKE_MAPPING_KEYPAIR_LOOP
  md->free_list=k->next;
#else /* PIKE_MAPPING_KEYPAIR_LOOP */
  md->free_list++;
#endif /* !PIKE_MAPPING_KEYPAIR_LOOP */
  k->next=md->hash[h];
  md->hash[h]=k;
  md->ind_types |= 1 << TYPEOF(*key);
  if (!(md->ind_types & BIT_INT) && (TYPEOF(*key) == PIKE_T_OBJECT)) {
    if (is_bignum_object(key->u.object)) {
      md->ind_types |= BIT_INT;
    }
  }
  md->val_types |= 1 << TYPEOF(*val);
  if (!(md->val_types & BIT_INT) && (TYPEOF(*val) == PIKE_T_OBJECT)) {
    if (is_bignum_object(val->u.object)) {
      md->val_types |= BIT_INT;
    }
  }
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
				const struct svalue *key,
				const struct svalue *val)
{
  low_mapping_insert(m,key,val,1);
}

/* Inline the above in this file. */
#define mapping_insert(M, KEY, VAL) low_mapping_insert ((M), (KEY), (VAL), 1)

PMOD_EXPORT union anything *mapping_get_item_ptr(struct mapping *m,
				     const struct svalue *key,
				     TYPE_T t)
{
  size_t h, h2;
  struct keypair *k, **prev;
  struct mapping_data *md,*omd;
  int grow_md;
  int refs;

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
  /* NB: We know that md has a reference from the mapping
   *     in addition to our reference.
   *
   * The use of sub_ref() silences warnings from Coverity, as well as
   * on the off chance of a reference counting error avoids accessing
   * freed memory.
   */
  refs = sub_ref(md);	/* free_mapping_data(md); */
  assert(refs);

  if(TYPEOF(k->val) == t)
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
  /* NB: We know that md has a reference from the mapping
   *     in addition to our reference.
   *
   * The use of sub_ref() silences warnings from Coverity, as well as
   * on the off chance of a reference counting error avoids accessing
   * freed memory.
   */
  refs = sub_ref(md);	/* free_mapping_data(md); */
  assert(refs);

  if(t != T_INT) return 0;

  grow_md = MD_FULLP(md);

  /* no need to call PREPARE_* because we re-hash instead */
  if(grow_md || md->refs>1)
  {
    debug_malloc_touch(m);
    rehash(m, md->hashsize ? md->hashsize << grow_md : AVG_LINK_LENGTH);
    md=m->data;
  }
  h=h2 & ( md->hashsize - 1);

  k=md->free_list;
#ifndef PIKE_MAPPING_KEYPAIR_LOOP
  md->free_list=k->next;
#else /* PIKE_MAPPING_KEYPAIR_LOOP */
  md->free_list++;
#endif /* !PIKE_MAPPING_KEYPAIR_LOOP */
  k->next=md->hash[h];
  md->hash[h]=k;
  assign_svalue_no_free(& k->ind, key);
  SET_SVAL(k->val, T_INT, NUMBER_NUMBER, integer, 0);
  k->hval = h2;
  md->ind_types |= 1 << TYPEOF(*key);
  if (!(md->ind_types & BIT_INT) && (TYPEOF(*key) == PIKE_T_OBJECT)) {
    if (is_bignum_object(key->u.object)) {
      md->ind_types |= BIT_INT;
    }
  }
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
			const struct svalue *key,
			struct svalue *to)
{
  size_t h,h2;
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
    SET_SVAL(*to, T_INT, NUMBER_UNDEFINED, integer, 0);
  }
  return;

 md_remove_value:
#ifdef PIKE_DEBUG
  if(md->refs <= 1)
    Pike_fatal("Too few refs in mapping->data\n");
  if(m->data != md)
    Pike_fatal("Wrong dataset in mapping_delete!\n");
  if(d_flag>1)  check_mapping(m);
  debug_malloc_touch(m);
#endif
  sub_ref(md);
  PREPARE_FOR_INDEX_CHANGE2();
  /* No need to propagate */
  *prev=k->next;
  free_svalue(& k->ind);
  if(to)
    move_svalue (to, &k->val);
  else
    free_svalue(& k->val);

  FREE_KEYPAIR(md, k);

  mark_free_svalue (&md->free_list->ind);
  mark_free_svalue (&md->free_list->val);

  md->size--;
#ifdef MAPPING_SIZE_DEBUG
  if(m->data ==md)
    m->debug_size--;
#endif

  if (UNLIKELY(!md->size)) {
    md->ind_types = md->val_types = 0;
  }

  if (!(md->flags & MAPPING_FLAG_NO_SHRINK)) {
    if((md->size * MIN_LINK_LENGTH_DENOMINATOR <
	md->hashsize * MIN_LINK_LENGTH_NUMERATOR) &&
       (md->hashsize > AVG_LINK_LENGTH)) {
      debug_malloc_touch(m);
      rehash(m, md->hashsize>>1);
    }
  }

#ifdef PIKE_DEBUG
  if(d_flag>1)  check_mapping(m);
#endif
  return;
}


PMOD_EXPORT void map_atomic_get_set(struct mapping *m,
				    const struct svalue *key,
				    struct svalue *from_to)
{
  size_t h,h2;
  struct keypair *k, **prev;
  struct mapping_data *md,*omd;
  struct svalue tmp;
  int grow_md;
  int refs;

  if (IS_UNDEFINED(from_to)) {
    /* NB: Overwrites the UNDEFINED without looking,
     *     but that is fine.
     */
    map_delete_no_free(m, key, from_to);
    return;
  }

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
#endif

  h2=hash_svalue(key);

#ifdef PIKE_DEBUG
  if(d_flag>1)  check_mapping(m);
#endif

  LOW_FIND(is_eq, key,
	   /* Key found. */
	   struct svalue *tmp=&k->ind;
	   SAME_DATA(goto mr_replace,
		     /* Mapping data has changed. */
		     omd=md;
		     LOW_FIND(is_identical, tmp,
			      /* The key is still unmodified in the mapping. */
			      free_mapping_data(omd);
			      goto mr_replace,
			      /* The key has changed??? */
			      free_mapping_data(omd);
			      goto mr_do_nothing)),
	   /* Key not found. */
	   Z:
	   SAME_DATA(goto mr_insert,
		     /* Mapping data has changed. */
		     omd=md;
		     LOW_FIND2(is_eq, key,
			       /* The key has appeared in the mapping??? */
			       free_mapping_data(omd);
			       goto mr_do_nothing,
			       /* The key is still not in the mapping. */
			       free_mapping_data(omd);
			       goto Z)));

 mr_do_nothing:
  /* Don't do anything??? */
  free_mapping_data(md);
  return;

 mr_replace:
#ifdef PIKE_DEBUG
  if(m->data != md)
    Pike_fatal("Wrong dataset in mapping_insert!\n");
  if(d_flag>1)  check_mapping(m);
#endif
  /* NB: We know that md has a reference from the mapping
   *     in addition to our reference.
   *
   * The use of sub_ref() silences warnings from Coverity, as well as
   * on the off chance of a reference counting error avoids accessing
   * freed memory.
   */
  refs = sub_ref(md);	/* free_mapping_data(md); */
  assert(refs);

  PREPARE_FOR_DATA_CHANGE2();
  PROPAGATE(); /* propagate after preparing */
  md->val_types |= 1 << TYPEOF(*from_to);
#if 0
  if (overwrite == 2 && TYPEOF(*key) == T_OBJECT)
    /* Should replace the index too. It's only for objects that it's
     * possible to tell the difference. */
    assign_svalue (&k->ind, key);
#endif
  tmp = k->val;
  k->val = *from_to;
  *from_to = tmp;
#ifdef PIKE_DEBUG
  if(d_flag>1)  check_mapping(m);
#endif
  return;

 mr_insert:
#ifdef PIKE_DEBUG
  if(m->data != md)
    Pike_fatal("Wrong dataset in mapping_insert!\n");
  if(d_flag>1)  check_mapping(m);
#endif
  /* NB: We know that md has a reference from the mapping
   *     in addition to our reference.
   *
   * The use of sub_ref() silences warnings from Coverity, as well as
   * on the off chance of a reference counting error avoids accessing
   * freed memory.
   */
  refs = sub_ref(md);	/* free_mapping_data(md); */
  assert(refs);

  grow_md = MD_FULLP(md);

  /* We do a re-hash here instead of copying the mapping. */
  if(grow_md || md->refs>1)
  {
    debug_malloc_touch(m);
    rehash(m, md->hashsize ? md->hashsize << grow_md : AVG_LINK_LENGTH);
    md=m->data;
  }
  h=h2 & ( md->hashsize - 1);

  /* no need to lock here since we are not calling is_eq - Hubbe */

  k=md->free_list;
#ifndef PIKE_MAPPING_KEYPAIR_LOOP
  md->free_list=k->next;
#else /* PIKE_MAPPING_KEYPAIR_LOOP */
  md->free_list++;
#endif /* !PIKE_MAPPING_KEYPAIR_LOOP */
  k->next=md->hash[h];
  md->hash[h]=k;
  md->ind_types |= 1 << TYPEOF(*key);
  if (!(md->ind_types & BIT_INT) && (TYPEOF(*key) == PIKE_T_OBJECT)) {
    if (is_bignum_object(key->u.object)) {
      md->ind_types |= BIT_INT;
    }
  }
  md->val_types |= 1 << TYPEOF(*from_to);
  if (!(md->val_types & BIT_INT) && (TYPEOF(*from_to) == PIKE_T_OBJECT)) {
    if (is_bignum_object(from_to->u.object)) {
      md->val_types |= BIT_INT;
    }
  }
  assign_svalue_no_free(& k->ind, key);
  k->val = *from_to;
  k->hval = h2;
  md->size++;
#ifdef MAPPING_SIZE_DEBUG
  if(m->data ==md)
    m->debug_size++;
#endif
  SET_SVAL(*from_to, T_INT, NUMBER_UNDEFINED, integer, 0);

#ifdef PIKE_DEBUG
  if(d_flag>1)  check_mapping(m);
#endif
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

	if((TYPEOF(k->ind) == T_OBJECT || TYPEOF(k->ind) == T_FUNCTION) &&
	   !k->ind.u.object->prog)
	{
	  debug_malloc_touch(m);
	  debug_malloc_touch(md);
	  PREPARE_FOR_INDEX_CHANGE2();
	  *prev=k->next;
	  free_svalue(& k->ind);
	  free_svalue(& k->val);
	  FREE_KEYPAIR(md, k);
	  mark_free_svalue (&md->free_list->ind);
	  mark_free_svalue (&md->free_list->val);
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
	  val_types |= 1 << TYPEOF(k->val);
          if (!(val_types & BIT_INT) && (TYPEOF(k->val) == PIKE_T_OBJECT)) {
            if (is_bignum_object(k->val.u.object)) {
              val_types |= BIT_INT;
            }
          }
	  ind_types |= 1 << TYPEOF(k->ind);
          if (!(ind_types & BIT_INT) && (TYPEOF(k->ind) == PIKE_T_OBJECT)) {
            if (is_bignum_object(k->ind.u.object)) {
              ind_types |= BIT_INT;
            }
          }
	  prev=&k->next;
	}
      }
    }

    md->val_types = val_types;
    md->ind_types = ind_types;

    if (!(md->flags & MAPPING_FLAG_NO_SHRINK)) {
      if((md->size * MIN_LINK_LENGTH_DENOMINATOR <
	  md->hashsize * MIN_LINK_LENGTH_NUMERATOR) &&
	 (md->hashsize > AVG_LINK_LENGTH)) {
	debug_malloc_touch(m);
	rehash(m, md->hashsize>>1);
      }
    }

#ifdef PIKE_DEBUG
    if(d_flag>1)  check_mapping(m);
#endif
  }
}

PMOD_EXPORT struct svalue *low_mapping_lookup(struct mapping *m,
					      const struct svalue *key)
{
  size_t h,h2;
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
                                                     const struct pike_string *p)
{
  struct svalue tmp;
  /* Expanded SET_SVALUE macro to silence discard const warning. */
  struct svalue *ptr = &tmp;
  ptr->u.string = (struct pike_string *)p;
  SET_SVAL_TYPE_SUBTYPE(*ptr, T_STRING, 0);
  return low_mapping_lookup(m, &tmp);
}

PMOD_EXPORT void mapping_string_insert(struct mapping *m,
                                       struct pike_string *p,
                                       const struct svalue *val)
{
  struct svalue tmp;
  SET_SVAL(tmp, T_STRING, 0, string, p);
  mapping_insert(m, &tmp, val);
}

PMOD_EXPORT void mapping_string_insert_string(struct mapping *m,
				  struct pike_string *p,
				  struct pike_string *val)
{
  struct svalue tmp;
  SET_SVAL(tmp, T_STRING, 0, string, val);
  mapping_string_insert(m, p, &tmp);
}

PMOD_EXPORT struct svalue *simple_mapping_string_lookup(struct mapping *m,
							const char *p)
{
  struct pike_string *tmp;
  if((tmp=findstring(p)))
    return low_mapping_string_lookup(m,tmp);
  return 0;
}

/* Lookup in a mapping of mappings */
PMOD_EXPORT struct svalue *mapping_mapping_lookup(struct mapping *m,
						  const struct svalue *key1,
						  const struct svalue *key2,
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

  if(!s || TYPEOF(*s) != T_MAPPING)
  {
    if(!create) return 0;
    SET_SVAL(tmp, T_MAPPING, 0, mapping, allocate_mapping(5));
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

  SET_SVAL(tmp, T_INT, NUMBER_UNDEFINED, integer, 0);

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
  SET_SVAL(k1, T_STRING, 0, string, key1);
  SET_SVAL(k2, T_STRING, 0, string, key2);
  return mapping_mapping_lookup(m,&k1,&k2,create);
}



PMOD_EXPORT void mapping_index_no_free(struct svalue *dest,
			   struct mapping *m,
			   const struct svalue *key)
{
  struct svalue *p;

  if(!IS_DESTRUCTED (key) && (p=low_mapping_lookup(m,key)))
  {
    assign_svalue_no_free(dest, p);

    /* Never return NUMBER_UNDEFINED for existing entries. */
    /* Note: There is code that counts on storing UNDEFINED in mapping
     * values (using e.g. low_mapping_lookup to get them), so we have
     * to fix the subtype here rather than in mapping_insert. */
    if(TYPEOF(*p) == T_INT)
      SET_SVAL_SUBTYPE(*dest, NUMBER_NUMBER);
  }else{
    SET_SVAL(*dest, T_INT, NUMBER_UNDEFINED, integer, 0);
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
      b->type_field = (1 << TYPEOF(k->ind)) | (1 << TYPEOF(k->val));
      SET_SVAL(*s, T_ARRAY, 0, array, b);
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
	  md->val_types |= 1<<TYPEOF(*to);
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

PMOD_EXPORT void clear_mapping(struct mapping *m)
{
  struct mapping_data *md;
  int flags;

  if (!m) return;
  md = m->data;
  flags = md->flags;
  if (!md->size) return;
  unlink_mapping_data(md);

  switch (flags & MAPPING_WEAK) {
  case 0: md = &empty_data; break;
  case MAPPING_WEAK_INDICES: md = &weak_ind_empty_data; break;
  case MAPPING_WEAK_VALUES: md = &weak_val_empty_data; break;
  default: md = &weak_both_empty_data; break;
  }

  /* FIXME: Increment hardlinks & valrefs?
   *    NB: init_mapping() doesn't do this.
   */
  add_ref(md);
  /* gc_init_marker(md); */

  m->data = md;
#ifdef MAPPING_SIZE_DEBUG
  m->debug_size = md->size;
#endif
}

/* deferred mapping copy! */
PMOD_EXPORT struct mapping *copy_mapping(struct mapping *m)
{
  struct mapping *n;

  if (!m) return NULL;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
#endif

  n=allocate_mapping_no_init();
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

/* copy_mapping() for when destructive operations are ok.
 *
 * Note: It destructive operations on the resulting mapping *will*
 *       affect eg NEW_MAPPING_LOOP() on the original mapping.
 */
static struct mapping *destructive_copy_mapping(struct mapping *m)
{
  if (!m) return NULL;

  if ((m->refs == 1) && !m->data->hardlinks &&
      !(m->data->flags & MAPPING_WEAK)) {
    /* We may perform destructive operations on the mapping. */
    add_ref(m);
    return m;
  }
  return copy_mapping(m);
}

/* NOTE: May perform destructive operations on either of the arguments
 *       if it has only a single reference.
 */
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
    return destructive_copy_mapping(a);
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
      size_t h = k->hval & ( b_md->hashsize - 1);
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
    res = destructive_copy_mapping(a);
    SET_ONERROR(err, do_free_mapping, res);
    NEW_MAPPING_LOOP(b_md) {
      map_delete(res, &k->ind);
    }
  }
  UNSET_ONERROR(err);
  return res;
}

/* NOTE: May perform destructive operations on either of the arguments
 *       if it has only a single reference.
 */
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
  if (a_md == b_md) return destructive_copy_mapping(a);

  if (a_md->size >= b_md->size / 2) {
    /* Copy the second mapping. */
    res = copy_mapping(b);
    SET_ONERROR(err, do_free_mapping, res);

    /* Remove elements in res that aren't in a. */
    NEW_MAPPING_LOOP(b_md) {
      size_t h = k->hval & ( a_md->hashsize - 1);
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
  } else {
    /* Loop over second mapping, insert into new one */
    res = allocate_mapping(0);
    SET_ONERROR(err, do_free_mapping, res);

    /* Remove elements in res that aren't in b, copy values for those that
     * are. */
    NEW_MAPPING_LOOP(a_md) {
      size_t h = k->hval & ( b_md->hashsize - 1);
      struct keypair *k2;
      for (k2 = b_md->hash[h]; k2; k2 = k2->next) {
        if ((k2->hval == k->hval) && is_eq(&k2->ind, &k->ind)) {
          mapping_insert(res, &k2->ind, &k2->val);
          break;
        }
      }
    }

    UNSET_ONERROR(err);
  }
  return res;
}

/* NOTE: May perform destructive operations on either of the arguments
 *       if it has only a single reference.
 */
static struct mapping *or_mappings(struct mapping *a, struct mapping *b)
{
  struct mapping *res;
  struct keypair *k;
  struct mapping_data *a_md = a->data;
  struct mapping_data *b_md = b->data;
  INT32 e;
  ONERROR err;

  /* First some special cases. */
  if (!a_md->size) return destructive_copy_mapping(b);
  if (!b_md->size) return destructive_copy_mapping(a);
  if (a_md == b_md) return destructive_copy_mapping(a);

  if (a_md->size <= b_md->size) {
    /* Copy the second mapping. */
    res = destructive_copy_mapping(b);
    SET_ONERROR(err, do_free_mapping, res);

    if (!b_md->hashsize) {
      Pike_fatal("Invalid hashsize.\n");
    }

    /* Add elements in a that aren't in b. */
    NEW_MAPPING_LOOP(a_md) {
      size_t h = k->hval & ( b_md->hashsize - 1);
      struct keypair *k2;
      for (k2 = b_md->hash[h]; k2; k2 = k2->next) {
	if ((k2->hval == k->hval) && is_eq(&k2->ind, &k->ind)) {
	  break;
	}
      }
      if (!k2) {
	mapping_insert(res, &k->ind, &k->val);
	b_md = b->data;
      }
    }
    UNSET_ONERROR(err);
  } else {
    /* Copy the first mapping. */
    res = destructive_copy_mapping(a);
    SET_ONERROR(err, do_free_mapping, res);

    /* Add all elements in b. */
    NEW_MAPPING_LOOP(b_md) {
      mapping_insert(res, &k->ind, &k->val);
    }
    UNSET_ONERROR(err);
  }
  return res;
}

/* NOTE: May perform destructive operations on either of the arguments
 *       if it has only a single reference.
 */
static struct mapping *xor_mappings(struct mapping *a, struct mapping *b)
{
  struct mapping *res;
  struct keypair *k;
  struct mapping_data *a_md = a->data;
  struct mapping_data *b_md = b->data;
  INT32 e;
  ONERROR err;

  /* First some special cases. */
  if (!a_md->size) return destructive_copy_mapping(b);
  if (!b_md->size) return destructive_copy_mapping(a);
  if (a_md == b_md) return allocate_mapping(0);

  /* Copy the largest mapping. */
  if (a_md->size > b_md->size) {
    struct mapping *tmp = a;
    a = b;
    b = tmp;
    a_md = b_md;
    b_md = b->data;
  }
  res = destructive_copy_mapping(b);
  SET_ONERROR(err, do_free_mapping, res);

  /* Add elements in a that aren't in b, and remove those that are. */
  NEW_MAPPING_LOOP(a_md) {
    size_t h = k->hval & ( b_md->hashsize - 1);
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
    b_md = b->data;
  }
  UNSET_ONERROR(err);
  return res;
}

/* NOTE: May perform destructive operations on either of the arguments
 *       if it has only a single reference.
 */
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
    free(zipper);
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
    free(zipper);
  }

  zipper=merge(ai,bi,op);

  ci=array_zip(ai,bi,zipper);

  UNSET_ONERROR(r4); free_array(bi);
  UNSET_ONERROR(r3); free_array(ai);

  cv=array_zip(av,bv,zipper);

  UNSET_ONERROR(r2); free_array(bv);
  UNSET_ONERROR(r1); free_array(av);

  free(zipper);

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
    free(zipper);
  }

  switch (op) /* no elements from �b� may be selected */
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

  free(zipper);

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

void o_append_mapping( INT32 args )
{
  struct svalue *lval = Pike_sp - args;
  struct svalue *val = lval + 2;
  int lvalue_type;

#ifdef PIKE_DEBUG
  if (args < 3) {
    Pike_fatal("Too few arguments to o_append_mapping(): %d\n", args);
  }
#endif
  args -= 3;
  /* Note: val should always be a zero here! */
  lvalue_type = lvalue_to_svalue_no_free(val, lval);

  if ((TYPEOF(*val) == T_MAPPING) && (lvalue_type != PIKE_T_GET_SET)) {
    struct mapping *m = val->u.mapping;
    if( m->refs == 2 )
    {
      if ((TYPEOF(*lval) == T_OBJECT) &&
	  lval->u.object->prog &&
	  ((FIND_LFUN(lval->u.object->prog, LFUN_ASSIGN_INDEX) >= 0) ||
	   (FIND_LFUN(lval->u.object->prog, LFUN_ASSIGN_ARROW) >= 0))) {
	/* There's a function controlling assignments in this object,
	 * so we can't alter the mapping in place.
	 */
      } else {
	int i;
	/* fprintf( stderr, "map_refs==2\n" ); */
	for( i=0; i<args; i+=2 )
	  low_mapping_insert( m, Pike_sp-(i+2), Pike_sp-(i+1), 2 );
	stack_pop_n_elems_keep_top(2+args);
	return;
      }
    }
  }

  f_aggregate_mapping(args);
  f_add(2);
  assign_lvalue(lval, val);
  stack_pop_2_elems_keep_top();
}

/* NOTE: May perform destructive operations on either of the arguments
 *       if it has only a single reference.
 */
PMOD_EXPORT struct mapping *add_mappings(struct svalue *argp, INT32 args)
{
  INT32 e,d;
  struct mapping *ret=0;
  struct keypair *k;

  for(e=d=0;d<args;d++)
  {
    struct mapping *m = argp[d].u.mapping;
#ifdef PIKE_DEBUG
    if(d_flag>1) check_mapping(m);
#endif
    e += m->data->size;
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

  if (a == b) return 1;
  if (!a || !b) return 0;

#ifdef PIKE_DEBUG
  if(a->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
  if(b->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
#endif

  if (a->data == b->data) return 1;

  /* If either is weak, they're different. */
  if ((a->data->flags | b->data->flags) & MAPPING_WEAK) return 0;

  check_mapping_for_destruct(a);
  check_mapping_for_destruct(b);

  if(m_sizeof(a) != m_sizeof(b)) return 0;

  if (m_sizeof(a) == 0) return 1;

  if (!check_type_overlaps(a->data->ind_types, b->data->ind_types) ||
      !check_type_overlaps(a->data->val_types, b->data->val_types)) return 0;

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

void describe_mapping(struct byte_buffer *b, struct mapping *m,struct processing *p,int indent)
{
  struct processing doing;
  struct array *a;
  JMP_BUF catch;
  ONERROR err;
  INT32 e,d;
  char buf[40];

  if (!m) {
    buffer_add_str(b, "NULL-mapping");
    return;
  }

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
#endif

  if(! m->data->size)
  {
    buffer_add_str(b, "([ ])");
    return;
  }

  doing.next=p;
  doing.pointer_a=(void *)m;
  for(e=0;p;e++,p=p->next)
  {
    if(p->pointer_a == (void *)m)
    {
      sprintf(buf,"@%ld",(long)e);
      buffer_add_str(b, buf);
      return;
    }
  }

  if (Pike_in_gc > GC_PASS_PREPARE && Pike_in_gc < GC_PASS_FREE) {
    /* Have to do without any temporary allocations. */
    struct keypair *k;
    int notfirst = 0;

    if (m->data->size == 1) {
      buffer_add_str(b, "([ /* 1 element */\n");
    } else {
      sprintf(buf, "([ /* %ld elements */\n", (long)m->data->size);
      buffer_add_str(b, buf);
    }

    NEW_MAPPING_LOOP(m->data) {
      if (notfirst) buffer_add_str(b, ",\n");
      else notfirst = 1;
      for(d = 0; d < indent; d++)
	buffer_add_char(b, ' ');
      describe_svalue(b, &k->ind, indent+2, &doing);
      buffer_add_str (b, ": ");
      describe_svalue(b, &k->val, indent+2, &doing);
    }

    buffer_add_char(b, '\n');
    for(e=2; e<indent; e++) buffer_add_char(b, ' ');
    buffer_add_str(b, "])");
    return;
  }

  a = mapping_indices(m);
  SET_ONERROR(err, do_free_array, a);

  if(! m->data->size) {		/* mapping_indices may remove elements */
    buffer_add_str(b, "([ ])");
  }
  else {
    int save_t_flag = Pike_interpreter.trace_level;

    if (m->data->size == 1) {
      buffer_add_str(b, "([ /* 1 element */\n");
    } else {
      sprintf(buf, "([ /* %ld elements */\n", (long)m->data->size);
      buffer_add_str(b, buf);
    }

    Pike_interpreter.trace_level = 0;
    if(SETJMP(catch)) {
      free_svalue(&throw_value);
      mark_free_svalue (&throw_value);
    }
    else
      sort_array_destructively(a);
    UNSETJMP(catch);
    Pike_interpreter.trace_level = save_t_flag;

    for(e = 0; e < a->size; e++)
    {
      struct svalue *tmp;
      if(e)
	buffer_add_str(b, ",\n");

      for(d = 0; d < indent; d++)
	buffer_add_char(b, ' ');

      describe_svalue(b, ITEM(a)+e, indent+2, &doing);
      buffer_add_str (b, ": ");

      {
	int save_t_flag=Pike_interpreter.trace_level;
	Pike_interpreter.trace_level=0;

	tmp=low_mapping_lookup(m, ITEM(a)+e);

	Pike_interpreter.trace_level=save_t_flag;
      }
      if(tmp)
	describe_svalue(b, tmp, indent+2, &doing);
      else
	buffer_add_str(b, "** gone **");
    }

    buffer_add_char(b, '\n');
    for(e=2; e<indent; e++) buffer_add_char(b, ' ');
    buffer_add_str(b, "])");
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

    SET_SVAL(s, T_MAPPING, 0, mapping, m);
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

  for(e=-args;e<0;e+=2) {
    STACK_LEVEL_START(-e);
    low_mapping_insert(m, Pike_sp+e, Pike_sp+e+1, 2);
    STACK_LEVEL_DONE(-e);
  }
  pop_n_elems(args);
#ifdef PIKE_DEBUG
  if(d_flag)
    check_mapping(m);
#endif
  push_mapping(m);
}

PMOD_EXPORT struct mapping *copy_mapping_recursively(struct mapping *m,
						     struct mapping *p)
{
  int not_complex;
  struct mapping *ret;
  INT32 e;
  struct keypair *k;
  struct mapping_data  *md;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
#endif

#ifdef PIKE_DEBUG
  if(d_flag > 1) check_mapping_type_fields(m);
#endif

  if((m->data->val_types | m->data->ind_types) & BIT_COMPLEX) {
    not_complex = 0;
    ret=allocate_mapping(MAP_SLOTS(m->data->size));
  }
  else {
    not_complex = 1;
    ret = copy_mapping(m);
  }

  if (p) {
    struct svalue aa, bb;
    SET_SVAL(aa, T_MAPPING, 0, mapping, m);
    SET_SVAL(bb, T_MAPPING, 0, mapping, ret);
    mapping_insert(p, &aa, &bb);
  }

  if (not_complex)
    return ret;

  ret->data->flags = m->data->flags;

  check_stack(2);

  md=m->data;
  md->valrefs++;
  add_ref(md);
  NEW_MAPPING_LOOP(md)
  {
    /* Place holders.
     *
     * NOTE: copy_svalues_recursively_no_free() may store stuff in
     *       the destination svalue, and then call stuff that uses
     *       the stack (eg itself).
     */
    push_int(0);
    push_int(0);
    copy_svalues_recursively_no_free(Pike_sp-2,&k->ind, 1, p);
    copy_svalues_recursively_no_free(Pike_sp-1,&k->val, 1, p);

    low_mapping_insert(ret, Pike_sp-2, Pike_sp-1, 2);
    pop_n_elems(2);
  }
  md->valrefs--;
  free_mapping_data(md);

  return ret;
}


PMOD_EXPORT void mapping_search_no_free(struct svalue *to,
			    struct mapping *m,
			    const struct svalue *look_for,
			    const struct svalue *key /* start */)
{
  struct mapping_data *md, *omd;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
#endif
  md=m->data;

  if(md->size)
  {
    size_t h2,h=0;
    struct keypair *k=md->hash[0], **prev;

    if(key)
    {
      h2=hash_svalue(key);

      FIND();

      if(!k)
      {
	SET_SVAL(*to, T_INT, NUMBER_UNDEFINED, integer, 0);
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

  SET_SVAL(*to, T_INT, NUMBER_UNDEFINED, integer, 0);
}

#ifdef PIKE_DEBUG

void check_mapping(const struct mapping *m)
{
  int e,num;
  struct keypair *k;
  const struct mapping_data *md;

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
      fprintf(stderr,"Pike was in GC stage %d when this fatal occurred:\n",Pike_in_gc);
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

  if(md->hashsize & (md->hashsize - 1))
    Pike_fatal("Invalid hashtable size: 0x%08lx\n", (long)md->hashsize);

  if(md->hashsize > md->num_keypairs)
    Pike_fatal("Pretty mean hashtable there buster %d > %d (2)!\n",md->hashsize,md->num_keypairs);

  if(md->num_keypairs > (md->hashsize + AVG_LINK_LENGTH - 1) * AVG_LINK_LENGTH)
    Pike_fatal("Mapping from hell detected, attempting to send it back...\n");

  if(md->size > 0 && (!md->ind_types || !md->val_types))
    Pike_fatal("Mapping type fields are... wrong.\n");

  num=0;
  NEW_MAPPING_LOOP(md)
    {
      num++;

      if (TYPEOF(k->ind) > MAX_TYPE)
	Pike_fatal("Invalid maping keypair index type: %s\n",
		   get_name_of_type(TYPEOF(k->ind)));
      if(! ( (1 << TYPEOF(k->ind)) & (md->ind_types) ))
	Pike_fatal("Mapping indices type field lies.\n");

      if (TYPEOF(k->val) > MAX_TYPE)
	Pike_fatal("Invalid mapping keypair value type: %s\n",
		   get_name_of_type(TYPEOF(k->val)));
      if(! ( (1 << TYPEOF(k->val)) & (md->val_types) ))
	Pike_fatal("Mapping values type field lies.\n");

      check_svalue(& k->ind);
      check_svalue(& k->val);
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

static void visit_mapping_data (struct mapping_data *md, int action,
				void *extra)
{
  visit_enter(md, T_MAPPING_DATA, extra);
  switch (action & VISIT_MODE_MASK) {
#ifdef PIKE_DEBUG
    default:
      Pike_fatal ("Unknown visit action %d.\n", action);
    case VISIT_NORMAL:
    case VISIT_COMPLEX_ONLY:
      break;
#endif
    case VISIT_COUNT_BYTES:
      mc_counted_bytes += MAPPING_DATA_SIZE (md->hashsize, md->num_keypairs);
      break;
  }

  if (!(action & VISIT_NO_REFS) &&
      (md->ind_types | md->val_types) &
      (action & VISIT_COMPLEX_ONLY ? BIT_COMPLEX : BIT_REF_TYPES)) {
    int ind_ref_type =
      md->flags & MAPPING_WEAK_INDICES ? REF_TYPE_WEAK : REF_TYPE_NORMAL;
    int val_ref_type =
      md->flags & MAPPING_WEAK_VALUES ? REF_TYPE_WEAK : REF_TYPE_NORMAL;
    INT32 e;
    struct keypair *k;
    NEW_MAPPING_LOOP (md) {
      visit_svalue (&k->ind, ind_ref_type, extra);
      visit_svalue (&k->val, val_ref_type, extra);
    }
  }
  visit_leave(md, T_MAPPING_DATA, extra);
}

PMOD_EXPORT void visit_mapping (struct mapping *m, int action, void *extra)
{
  visit_enter(m, T_MAPPING, extra);
  switch (action & VISIT_MODE_MASK) {
#ifdef PIKE_DEBUG
    default:
      Pike_fatal ("Unknown visit action %d.\n", action);
    case VISIT_NORMAL:
    case VISIT_COMPLEX_ONLY:
      break;
#endif
    case VISIT_COUNT_BYTES:
      mc_counted_bytes += sizeof (struct mapping);
      break;
  }

  visit_ref (m->data, REF_TYPE_INTERNAL,
	     (visit_thing_fn *) &visit_mapping_data, extra);
  visit_leave(m, T_MAPPING, extra);
}

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
    VAL_TYPES |= 1 << TYPEOF(k->val);					\
  }									\
} while (0)

#ifdef PIKE_MAPPING_KEYPAIR_LOOP
#error Broken code below!
#define GC_RECURSE(M, MD, REC_KEYPAIR, TYPE, IND_TYPES, VAL_TYPES) do {	\
    int remove;								\
    struct keypair *k,**prev_;						\
    /* no locking required (no is_eq) */				\
    for(k = MD_KEYPAIRS(md, md->hashsize);k < MD->free_list;)		\
    {									\
      REC_KEYPAIR(remove,						\
		  PIKE_CONCAT(TYPE, _svalues),				\
		  PIKE_CONCAT(TYPE, _weak_svalues),			\
		  PIKE_CONCAT(TYPE, _without_recurse),			\
		  PIKE_CONCAT(TYPE, _weak_without_recurse));		\
      if (remove) {							\
	/* Find and unlink k. */					\
	unsigned INT32 h_;						\
	h_ = k->hval & ( md->hashsize - 1);				\
	prev_ = md->hash + h_;						\
	DO_IF_DEBUG(							\
		    if (!*prev_) {					\
		      Pike_fatal("Node to unlink not found!\n");	\
		    }							\
		    );							\
	while (*prev_ != k) {						\
	  prev_ = &((*prev_)->next);					\
	  DO_IF_DEBUG(							\
		      if (!*prev_) {					\
			Pike_fatal("Node to unlink not found!\n");	\
		      }							\
		      );						\
	}								\
	(*prev_)->next = k->next;					\
        FREE_KEYPAIR(MD, k);						\
	mark_free_svalue (&MD->free_list->ind);				\
	mark_free_svalue (&MD->free_list->val);				\
	MD->size--;							\
	DO_IF_MAPPING_SIZE_DEBUG(					\
	  if(M->data ==MD)						\
	    M->debug_size--;						\
	);								\
      } else {								\
	VAL_TYPES |= 1 << TYPEOF(k->val);				\
        if (!(VAL_TYPES & BIT_INT) &&                                   \
            (TYPEOF(k->val) == PIKE_T_OBJECT)) {                        \
          if (is_bignum_object(k->val.u.object)) {                      \
            VAL_TYPES |= BIT_INT;                                       \
          }                                                             \
        }                                                               \
	IND_TYPES |= 1 << TYPEOF(k->ind);				\
        if (!(IND_TYPES & BIT_INT) &&                                   \
            (TYPEOF(k->ind) == PIKE_T_OBJECT)) {                        \
          if (is_bignum_object(k->ind.u.object)) {                      \
            IND_TYPES |= BIT_INT;                                       \
          }                                                             \
        }                                                               \
	k++;								\
      }									\
    }									\
  } while (0)
#else /* !PIKE_MAPPING_KEYPAIR_LOOP */
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
	mark_free_svalue (&MD->free_list->ind);				\
	mark_free_svalue (&MD->free_list->val);				\
	MD->size--;							\
	DO_IF_MAPPING_SIZE_DEBUG(					\
	  if(M->data ==MD)						\
	    M->debug_size--;						\
	);								\
      }else{								\
	VAL_TYPES |= 1 << TYPEOF(k->val);				\
        if (!(VAL_TYPES & BIT_INT) &&                                   \
            (TYPEOF(k->val) == PIKE_T_OBJECT)) {                        \
          if (is_bignum_object(k->val.u.object)) {                      \
            VAL_TYPES |= BIT_INT;                                       \
          }                                                             \
        }                                                               \
	IND_TYPES |= 1 << TYPEOF(k->ind);				\
        if (!(IND_TYPES & BIT_INT) &&                                   \
            (TYPEOF(k->ind) == PIKE_T_OBJECT)) {                        \
          if (is_bignum_object(k->ind.u.object)) {                      \
            IND_TYPES |= BIT_INT;                                       \
          }                                                             \
        }                                                               \
	prev=&k->next;							\
      }									\
    }									\
  }									\
} while (0)
#endif /* PIKE_MAPPING_KEYPAIR_LOOP */

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
  else if (N_REC(&k->ind, 1)) /* Now we can recurse the index. */	\
    Pike_fatal("Mapping: %s and %s don't agree.\n",			\
	       TOSTR(N_TST), TOSTR(N_REC));				\
} while (0)

#define GC_REC_KP_BOTH(REMOVE, N_REC, W_REC, N_TST, W_TST) do {		\
  if ((REMOVE = W_TST(&k->ind))) /* Don't recurse now. */		\
    gc_free_svalue(&k->val);						\
  else if ((REMOVE = W_REC(&k->val, 1)))				\
    gc_free_svalue(&k->ind);						\
  else if (W_REC(&k->ind, 1)) /* Now we can recurse the index. */	\
    Pike_fatal("Mapping: %s and %s don't agree.\n",			\
	       TOSTR(W_TST), TOSTR(W_REC));				\
} while (0)

void gc_mark_mapping_as_referenced(struct mapping *m)
{
#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
#endif
  debug_malloc_touch(m);
  debug_malloc_touch(m->data);

  if(gc_mark(m, T_MAPPING))
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

      if(gc_mark(md, T_MAPPING_DATA) &&
	 ((md->ind_types | md->val_types) & BIT_COMPLEX)) {
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
    if (((int) PTR_TO_INT (m->data)) == 0x55555555) {
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

void simple_describe_mapping(struct mapping *m)
{
  if (m) {
    struct byte_buffer buf = BUFFER_INIT();
    describe_mapping(&buf,m,0,2);
    buffer_add_str(&buf, "\n");
    fputs(buffer_get_string(&buf), stderr);
    buffer_free(&buf);
  } else {
    fputs("(NULL mapping)",stderr);
  }
}

#ifdef PIKE_DEBUG


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
