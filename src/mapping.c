/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/
#include "global.h"
RCSID("$Id: mapping.c,v 1.70 2000/03/07 23:52:16 hubbe Exp $");
#include "main.h"
#include "object.h"
#include "mapping.h"
#include "svalue.h"
#include "array.h"
#include "pike_macros.h"
#include "language.h"
#include "error.h"
#include "pike_memory.h"
#include "dynamic_buffer.h"
#include "interpret.h"
#include "las.h"
#include "gc.h"
#include "stralloc.h"
#include "security.h"
#include "block_alloc.h"

#define AVG_LINK_LENGTH 4
#define MIN_LINK_LENGTH 1
#define MAP_SLOTS(X) ((X)?((X)+((X)>>4)+8):0)

struct mapping *first_mapping;

#define unlink_mapping_data(M) do{				\
 struct mapping_data *md_=(M);					\
 if(md_->hardlinks) { md_->hardlinks--; md_->valrefs--; }	\
 free_mapping_data(M);						\
}while(0)


#define MD_KEYPAIRS(MD, HSIZE) \
   ( (struct keypair *)DO_ALIGN( (long) (((struct mapping_data *)(MD))->hash + HSIZE), ALIGNOF(struct keypair)) )

#define MAPPING_DATA_SIZE(HSIZE, KEYPAIRS) \
   (long)( MD_KEYPAIRS(0, HSIZE) + KEYPAIRS )
   
   

#undef EXIT_BLOCK
#define EXIT_BLOCK(m)							\
  INT32 e;								\
DO_IF_DEBUG(								\
  if(m->refs)								\
    fatal("really free mapping on mapping with nonzero refs.\n");	\
)									\
									\
  FREE_PROT(m);								\
									\
  unlink_mapping_data(m->data);						\
									\
  if(m->prev)								\
    m->prev->next = m->next;						\
  else									\
    first_mapping = m->next;						\
									\
  if(m->next) m->next->prev = m->prev;					\
									\
  GC_FREE();


#undef COUNT_OTHER

#define COUNT_OTHER() do{				\
  struct mapping *m;					\
  for(m=first_mapping;m;m=m->next)			\
  {							\
    num++;						\
    size+=MAPPING_DATA_SIZE(m->data->hashsize, m->data->num_keypairs); \
  }							\
}while(0)

BLOCK_ALLOC(mapping, 511)


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
    fatal("Mapping value types out of order!\n");

  if(ind_types & ~(m->data->ind_types))
    fatal("Mapping indices types out of order!\n");
}
#endif

static struct mapping_data empty_data = { 1, 1, 0,0,0,0,0 };

/* This function allocates the hash table and svalue space for a mapping
 * struct. The size is the max number of indices that can fit in the
 * allocated space.
 */
static void init_mapping(struct mapping *m, INT32 size)
{
  struct mapping_data *md;
  char *tmp;
  INT32 e;
  INT32 hashsize;

  debug_malloc_touch(m);
#ifdef PIKE_DEBUG
  if(size < 0) fatal("init_mapping with negative value.\n");
#endif
  if(size)
  {
    hashsize=size / AVG_LINK_LENGTH + 1;
    if(!(hashsize & 1)) hashsize++;

    e=MAPPING_DATA_SIZE(hashsize, size);

    md=(struct mapping_data *)xalloc(e);
    
    m->data=md;
    md->hashsize=hashsize;
    
    MEMSET((char *)md->hash, 0, sizeof(struct keypair *) * md->hashsize);
    
    md->free_list=MD_KEYPAIRS(md, hashsize);
    for(e=1;e<size;e++)
    {
      md->free_list[e-1].next = md->free_list + e;
      md->free_list[e-1].ind.type=T_INT;
      md->free_list[e-1].val.type=T_INT;
    }
    md->free_list[e-1].next=0;
    md->free_list[e-1].ind.type=T_INT;
    md->free_list[e-1].val.type=T_INT;
    md->ind_types = 0;
    md->val_types = 0;
    md->size = 0;
    md->refs=0;
    md->valrefs=0;
    md->hardlinks=0;
    md->num_keypairs=size;
  }else{
    md=&empty_data;
  }
  add_ref(md);
  m->data=md;
#ifdef PIKE_DEBUG
  m->debug_size = md->size;
#endif
}

/* This function allocates an empty mapping with room for 'size' values
 */
struct mapping *debug_allocate_mapping(int size)
{
  struct mapping *m;

  GC_ALLOC();

  m=alloc_mapping();

  INITIALIZE_PROT(m);
  init_mapping(m,size);

  m->next = first_mapping;
  m->prev = 0;
  m->refs = 1;
  m->flags = 0;

  if(first_mapping) first_mapping->prev = m;
  first_mapping=m;

  return m;
}


void really_free_mapping_data(struct mapping_data *md)
{
  INT32 e;
  struct keypair *k;
  debug_malloc_touch(md);
#ifdef PIKE_DEBUG
  if (md->refs) {
    fatal("really_free_mapping_data(): md has non-zero refs: %d\n",
	  md->refs);
  }
#endif /* PIKE_DEBUG */
  NEW_MAPPING_LOOP(md)
  {
    free_svalue(& k->val);
    free_svalue(& k->ind);
  }

  free((char *) md);
}

/* This function is used to rehash a mapping without loosing the internal
 * order in each hash chain. This is to prevent mappings from becoming
 * inefficient just after being rehashed.
 */
static void mapping_rehash_backwards_evil(struct mapping_data *md,
					  struct keypair *from)
{
  unsigned INT32 h;
  struct keypair *tmp;
  struct keypair *k;

  if(!from) return;
  mapping_rehash_backwards_evil(md,from->next);

  /* unlink */
  k=md->free_list;
#ifdef PIKE_DEBUG
  if(!k) fatal("Error in rehash: not enough keypairs.\n");
#endif
  md->free_list=k->next;

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
  struct keypair *tmp;
  struct keypair *k;

  if(!from) return;
  mapping_rehash_backwards_good(md,from->next);

  /* unlink */
  k=md->free_list;
#ifdef PIKE_DEBUG
  if(!k) fatal("Error in rehash: not enough keypairs.\n");
#endif
  md->free_list=k->next;

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
    fatal("Zero refs in mapping->data\n");

  if(d_flag>1)  check_mapping(m);
#endif

  init_mapping(m, new_size);
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
  }

#ifdef PIKE_DEBUG
  if(m->data->size != tmp)
    fatal("Rehash failed, size not same any more.\n");

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
  long off,size,e;
  struct mapping_data *nmd;
  struct keypair *keypairs;

  debug_malloc_touch(md);

  size=MAPPING_DATA_SIZE(md->hashsize, md->num_keypairs);

  nmd=(struct mapping_data *)xalloc(size);
  MEMCPY(nmd, md, size);
  off=((char *)nmd) - ((char *)md);

  RELOC(nmd->free_list);
  for(e=0;e<nmd->hashsize;e++) RELOC(nmd->hash[e]);

  keypairs=MD_KEYPAIRS(nmd, nmd->hashsize);
  for(e=0;e<nmd->num_keypairs;e++)
  {
    RELOC(keypairs[e].next);
    add_ref_svalue(& keypairs[e].ind);
    add_ref_svalue(& keypairs[e].val);
  }

  nmd->refs=1;
  nmd->valrefs=0;
  nmd->hardlinks=0;

  if(md->hardlinks)
  {
#ifdef PIKE_DEBUG
    if(md->refs <= 0 || md->valrefs<=0)
      fatal("Hardlink without refs/valrefs!\n");
#endif
    md->hardlinks--;
    md->valrefs--;
  }
  md->refs--;

  return nmd;
}

#define LOW_FIND(FUN, KEY, FOUND, NOT_FOUND) do {		\
  md=m->data;                                                   \
  add_ref(md);						        \
  if(md->hashsize)						\
  {								\
    h=h2 % md->hashsize;					\
    DO_IF_DEBUG( if(d_flag > 1) check_mapping_type_fields(m); ) \
    if(md->ind_types & (1 << key->type))			\
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
    if(md->ind_types & (1 << key->type))			\
    {								\
      k2=omd->hash[h2 % md->hashsize];			        \
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
  long off;					\
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
  long off;					\
  m->data=copy_mapping_data(m->data);		\
  debug_malloc_touch(m->data);                  \
  off=((char *)m->data)-((char *)md);		\
  LOW_RELOC(k);					\
  free_mapping_data(md);                        \
  md=m->data;                                   \
  md->refs++;                                   \
}while(0)

#define PREPARE_FOR_DATA_CHANGE() \
 if(md->valrefs) COPYMAP()

#define PREPARE_FOR_INDEX_CHANGE() \
 if(md->refs>1) COPYMAP()

/* This function brings the type fields in the mapping up to speed.
 * I only use it when the type fields MUST be correct, which is not
 * very often.
 */
void mapping_fix_type_field(struct mapping *m)
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
    fatal("Mapping value types out of order!\n");

  if(ind_types & ~(m->data->ind_types))
    fatal("Mapping indices types out of order!\n");
#endif
  m->data->val_types = val_types;
  m->data->ind_types = ind_types;
}

/* This function inserts key:val into the mapping m.
 * Same as doing m[key]=val; in pike.
 */
void low_mapping_insert(struct mapping *m,
			struct svalue *key,
			struct svalue *val,
			int overwrite)
{
  unsigned INT32 h,h2;
  struct keypair *k, **prev;
  struct mapping_data *md, *omd;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    fatal("Zero refs in mapping->data\n");
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
    fatal("Wrong dataset in mapping_insert!\n");
  if(d_flag>1)  check_mapping(m);
#endif
  free_mapping_data(md);
  if(!overwrite) return;
  PREPARE_FOR_DATA_CHANGE2();
  PROPAGATE(); /* propagate after preparing */
  md->val_types |= 1 << val->type;
  assign_svalue(& k->val, val);
#ifdef PIKE_DEBUG
  if(d_flag>1)  check_mapping(m);
#endif
  return;

 mi_insert:
#ifdef PIKE_DEBUG
  if(m->data != md)
    fatal("Wrong dataset in mapping_insert!\n");
  if(d_flag>1)  check_mapping(m);
#endif
  free_mapping_data(md);
  /* We do a re-hash here instead of copying the mapping. */
  if((!(k=md->free_list)) || md->refs>1)
  {
    debug_malloc_touch(m);
    rehash(m, md->size * 2 + 2);
    md=m->data;
    k=md->free_list;
  }
  h=h2 % md->hashsize;

  /* no need to lock here since we are not calling is_eq - Hubbe */

  k=md->free_list;
  md->free_list=k->next;
  k->next=md->hash[h];
  md->hash[h]=k;
  md->ind_types |= 1 << key->type;
  md->val_types |= 1 << val->type;
  assign_svalue_no_free(& k->ind, key);
  assign_svalue_no_free(& k->val, val);
  k->hval = h2;
  md->size++;
#ifdef PIKE_DEBUG
  m->debug_size++;
#endif

#ifdef PIKE_DEBUG
  if(d_flag>1)  check_mapping(m);
#endif
}

void mapping_insert(struct mapping *m,
		    struct svalue *key,
		    struct svalue *val)
{
  low_mapping_insert(m,key,val,1);
}

union anything *mapping_get_item_ptr(struct mapping *m,
				     struct svalue *key,
				     TYPE_T t)
{
  unsigned INT32 h, h2;
  struct keypair *k, **prev;
  struct mapping_data *md,*omd;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    fatal("Zero refs in mapping->data\n");

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
    fatal("Wrong dataset in mapping_get_item_ptr!\n");
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
    fatal("Wrong dataset in mapping_get_item_ptr!\n");
  if(d_flag)
    check_mapping(m);
#endif
  free_mapping_data(md);

  if(t != T_INT) return 0;

  /* no need to call PREPARE_* because we re-hash instead */
  if(!(k=md->free_list) || md->refs>1)
  {
    debug_malloc_touch(m);
    rehash(m, md->size * 2 + 2);
    md=m->data;
    k=md->free_list;
  }
  h=h2 % md->hashsize;

  md->free_list=k->next;
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
#ifdef PIKE_DEBUG
  m->debug_size++;
#endif

#ifdef PIKE_DEBUG
  if(d_flag > 1) check_mapping_type_fields(m);
#endif

  return & ( k->val.u );
}

void map_delete_no_free(struct mapping *m,
			struct svalue *key,
			struct svalue *to)
{
  unsigned INT32 h,h2;
  struct keypair *k, **prev;
  struct mapping_data *md,*omd;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    fatal("Zero refs in mapping->data\n");
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
    fatal("Too few refs i mapping->data\n");
  if(m->data != md)
    fatal("Wrong dataset in mapping_delete!\n");
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

  k->ind.type=T_INT;
  k->val.type=T_INT;

  k->next=md->free_list;
  md->free_list=k;
  md->size--;
#ifdef PIKE_DEBUG
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

void check_mapping_for_destruct(struct mapping *m)
{
  INT32 e;
  struct keypair *k, **prev;
  TYPE_FIELD ind_types, val_types;
  struct mapping_data *md=m->data;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    fatal("Zero refs in mapping->data\n");
  if(d_flag>1)  check_mapping(m);
  debug_malloc_touch(m);
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
	  PREPARE_FOR_INDEX_CHANGE2();
	  *prev=k->next;
	  free_svalue(& k->ind);
	  free_svalue(& k->val);
	  k->next=md->free_list;
	  md->free_list=k;
	  md->size--;
#ifdef PIKE_DEBUG
	  m->debug_size++;
#endif
	}else{
	  val_types |= 1 << k->val.type;
	  ind_types |= 1 << k->ind.type;
	  prev=&k->next;
	}
      }
    }
    if(MAP_SLOTS(md->size) < md->hashsize * MIN_LINK_LENGTH)
    {
      debug_malloc_touch(m);
      rehash(m, MAP_SLOTS(md->size));
    }

    md->val_types = val_types;
    md->ind_types = ind_types;
#ifdef PIKE_DEBUG
  if(d_flag>1)  check_mapping(m);
#endif
  }
}

struct svalue *low_mapping_lookup(struct mapping *m,
				  struct svalue *key)
{
  unsigned INT32 h,h2;
  struct keypair *k=0, **prev=0;
  struct mapping_data *md, *omd;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    fatal("Zero refs in mapping->data\n");
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

struct svalue *low_mapping_string_lookup(struct mapping *m,
					 struct pike_string *p)
{
  struct svalue tmp;
  tmp.type=T_STRING;
  tmp.u.string=p;
  return low_mapping_lookup(m, &tmp);
}

void mapping_string_insert(struct mapping *m,
			   struct pike_string *p,
			   struct svalue *val)
{
  struct svalue tmp;
  tmp.type=T_STRING;
  tmp.u.string=p;
  mapping_insert(m, &tmp, val);
}

void mapping_string_insert_string(struct mapping *m,
				  struct pike_string *p,
				  struct pike_string *val)
{
  struct svalue tmp;
  tmp.type=T_STRING;
  tmp.u.string=val;
  mapping_string_insert(m, p, &tmp);
}

struct svalue *simple_mapping_string_lookup(struct mapping *m,
					    char *p)
{
  struct pike_string *tmp;
  if((tmp=findstring(p)))
    return low_mapping_string_lookup(m,tmp);
  return 0;
}

struct svalue *mapping_mapping_lookup(struct mapping *m,
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
    fatal("Zero refs in mapping->data\n");
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


struct svalue *mapping_mapping_string_lookup(struct mapping *m,
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



void mapping_index_no_free(struct svalue *dest,
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

struct array *mapping_indices(struct mapping *m)
{
  INT32 e;
  struct array *a;
  struct svalue *s;
  struct keypair *k;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    fatal("Zero refs in mapping->data\n");
#endif

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

struct array *mapping_values(struct mapping *m)
{
  INT32 e;
  struct keypair *k;
  struct array *a;
  struct svalue *s;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    fatal("Zero refs in mapping->data\n");
#endif

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

struct array *mapping_to_array(struct mapping *m)
{
  INT32 e;
  struct keypair *k;
  struct array *a;
  struct svalue *s;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    fatal("Zero refs in mapping->data\n");
#endif

  a=allocate_array(m->data->size);
  s=ITEM(a);

  /* no locking required */
  NEW_MAPPING_LOOP(m->data)
    {
      struct array *b=allocate_array(2);
      assign_svalue(b->item+0, & k->ind);
      assign_svalue(b->item+1, & k->val);
      s->u.array=b;
      s->type=T_ARRAY;
      s++;
    }
  a->type_field = BIT_ARRAY;

  return a;
}

void mapping_replace(struct mapping *m,struct svalue *from, struct svalue *to)
{
  INT32 e;
  struct keypair *k;
  struct mapping_data *md;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    fatal("Zero refs in mapping->data\n");
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

struct mapping *mkmapping(struct array *ind, struct array *val)
{
  struct mapping *m;
  struct svalue *i,*v;
  INT32 e;

#ifdef PIKE_DEBUG
  if(ind->size != val->size)
    fatal("mkmapping on different sized arrays.\n");
#endif

  m=allocate_mapping(MAP_SLOTS(ind->size));
  i=ITEM(ind);
  v=ITEM(val);
  for(e=0;e<ind->size;e++) mapping_insert(m, i++, v++);

  return m;
}

#if 0
struct mapping *copy_mapping(struct mapping *m)
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
struct mapping *copy_mapping(struct mapping *m)
{
  struct mapping *n;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    fatal("Zero refs in mapping->data\n");
#endif

  n=allocate_mapping(0);
  if(!m_sizeof(m)) return n; /* done */
  debug_malloc_touch(n->data);
  free_mapping_data(n->data);
  n->data=m->data;
#ifdef PIKE_DEBUG
  n->debug_size=n->data->size;
#endif
  n->data->refs++;
  n->data->valrefs++;
  n->data->hardlinks++;
  debug_malloc_touch(n->data);
  return n;
}

#endif

struct mapping *merge_mappings(struct mapping *a, struct mapping *b, INT32 op)
{
  struct array *ai, *av;
  struct array *bi, *bv;
  struct array *ci, *cv;
  INT32 *zipper;
  struct mapping *m;

#ifdef PIKE_DEBUG
  if(a->data->refs <=0)
    fatal("Zero refs in mapping->data\n");
  if(b->data->refs <=0)
    fatal("Zero refs in mapping->data\n");
#endif

  ai=mapping_indices(a);
  av=mapping_values(a);
  if(ai->size > 1)
  {
    zipper=get_set_order(ai);
    order_array(ai, zipper);
    order_array(av, zipper);
    free((char *)zipper);
  }

  bi=mapping_indices(b);
  bv=mapping_values(b);
  if(bi->size > 1)
  {
    zipper=get_set_order(bi);
    order_array(bi, zipper);
    order_array(bv, zipper);
    free((char *)zipper);
  }

  zipper=merge(ai,bi,op);

  ci=array_zip(ai,bi,zipper);
  free_array(ai);
  free_array(bi);

  cv=array_zip(av,bv,zipper);
  free_array(av);
  free_array(bv);
  
  free((char *)zipper);

  m=mkmapping(ci, cv);
  free_array(ci);
  free_array(cv);

  return m;
}

struct mapping *merge_mapping_array_ordered(struct mapping *a, 
					    struct array *b, INT32 op)
{
  struct array *ai, *av;
  struct array *ci, *cv;
  INT32 *zipper;
  struct mapping *m;

  ai=mapping_indices(a);
  av=mapping_values(a);
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
	fatal("merge_mapping_array on other than AND or SUB\n");
  }

  free_array(ai);
  free_array(av);
  
  free((char *)zipper);

  m=mkmapping(ci, cv);
  free_array(ci);
  free_array(cv);

  return m;
}

struct mapping *merge_mapping_array_unordered(struct mapping *a, 
					      struct array *b, INT32 op)
{
  struct array *b_temp;
  INT32 *zipper;
  struct mapping *m;

  if (b->size>1)
  {
    zipper=get_set_order(b);
    b_temp=reorder_and_copy_array(b,zipper);
    m=merge_mapping_array_ordered(a,b_temp,op);
    free_array(b_temp);
  }
  else
    m=merge_mapping_array_ordered(a,b,op);

  return m;
}

struct mapping *add_mappings(struct svalue *argp, INT32 args)
{
  INT32 e,d;
  struct mapping *ret;
  struct keypair *k;

  for(e=d=0;d<args;d++)
  {
#ifdef PIKE_DEBUG
    if(d_flag>1) check_mapping(argp[d].u.mapping);
#endif
    e+=argp[d].u.mapping->data->size;
  }

#if 1
  /* Major optimization */
  for(d=0;d<args;d++)
    if(e==argp[d].u.mapping->data->size)
      return copy_mapping(argp[d].u.mapping);
#endif

  /* FIXME: need locking! */
  ret=allocate_mapping(MAP_SLOTS(e));
  for(d=0;d<args;d++)
    MAPPING_LOOP(argp[d].u.mapping)
      mapping_insert(ret, &k->ind, &k->val);
  return ret;
}

int mapping_equal_p(struct mapping *a, struct mapping *b, struct processing *p)
{
  struct processing curr;
  struct keypair *k;
  struct mapping_data *md;
  INT32 e,eq=1;

#ifdef PIKE_DEBUG
  if(a->data->refs <=0)
    fatal("Zero refs in mapping->data\n");
  if(b->data->refs <=0)
    fatal("Zero refs in mapping->data\n");
#endif

  if(a==b) return 1;
  if(m_sizeof(a) != m_sizeof(b)) return 0;

  curr.pointer_a = a;
  curr.pointer_b = b;
  curr.next = p;

  for( ;p ;p=p->next)
    if(p->pointer_a == (void *)a && p->pointer_b == (void *)b)
      return 1;

  check_mapping_for_destruct(a);
  check_mapping_for_destruct(b);

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
      eq=0;
      break;
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
  struct keypair *k;
  char buf[40];

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    fatal("Zero refs in mapping->data\n");
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
  
  sprintf(buf, m->data->size == 1 ? "([ /* %ld element */\n" :
	                            "([ /* %ld elements */\n",
	  (long)m->data->size);
  my_strcat(buf);

  a = mapping_indices(m);
  SET_ONERROR(err, do_free_array, a);

  if(!SETJMP(catch))
    sort_array_destructively(a);
  UNSETJMP(catch);

  /* no mapping locking required (I hope) */
  for(e = 0; e < a->size; e++)
  {
    struct svalue *tmp;
    if(e)
      my_strcat(",\n");
    
    for(d = 0; d < indent; d++)
      my_putchar(' ');
    
    describe_svalue(ITEM(a)+e, indent+2, &doing);
    my_putchar(':');
    if((tmp=low_mapping_lookup(m, ITEM(a)+e)))
      describe_svalue(tmp, indent+2, &doing);
    else
      my_strcat("** gone **");
  }

  UNSET_ONERROR(err);
  free_array(a);
  
  my_putchar('\n');
  for(e=2; e<indent; e++) my_putchar(' ');
  my_strcat("])");
}

node *make_node_from_mapping(struct mapping *m)
{
  struct keypair *k;
  INT32 e;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    fatal("Zero refs in mapping->data\n");
#endif

  mapping_fix_type_field(m);

  if((m->data->ind_types | m->data->val_types) & (BIT_FUNCTION | BIT_OBJECT))
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

void f_m_delete(INT32 args)
{
  if(args < 2)
    error("Too few arguments to m_delete.\n");
  if(sp[-args].type != T_MAPPING)
    error("Bad argument 1 to m_delete.\n");

  map_delete(sp[-args].u.mapping,sp+1-args);
  pop_n_elems(args-1);
}

void f_aggregate_mapping(INT32 args)
{
  INT32 e;
  struct keypair *k;
  struct mapping *m;

  if(args & 1)
    error("Uneven number of arguments to aggregate_mapping.\n");

  m=allocate_mapping(MAP_SLOTS(args / 2));

  for(e=-args;e<0;e+=2) mapping_insert(m, sp+e, sp+e+1);
  pop_n_elems(args);
#ifdef PIKE_DEBUG
  if(d_flag)
    check_mapping(m);
#endif
  push_mapping(m);
}

struct mapping *copy_mapping_recursively(struct mapping *m,
					 struct processing *p)
{
  struct processing doing;
  struct mapping *ret;
  INT32 e;
  struct keypair *k;
  struct mapping_data  *md;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    fatal("Zero refs in mapping->data\n");
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
  ret->flags=m->flags;
  doing.pointer_b=ret;

  check_stack(2);

  md=m->data;
  md->valrefs++;
  add_ref(md);
  MAPPING_LOOP(m)	/* FIXME: Shouldn't NEW_MAPPING_LOOP() be used? */
  {
    copy_svalues_recursively_no_free(sp,&k->ind, 1, p);
    sp++;
    copy_svalues_recursively_no_free(sp,&k->val, 1, p);
    sp++;
    
    mapping_insert(ret, sp-2, sp-1);
    pop_n_elems(2);
  }
  md->valrefs--;
  free_mapping_data(md);

  return ret;
}


void mapping_search_no_free(struct svalue *to,
			    struct mapping *m,
			    struct svalue *look_for,
			    struct svalue *key /* start */)
{
  struct mapping_data *md, *omd;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    fatal("Zero refs in mapping->data\n");
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
  md=m->data;

  if(m->refs <=0)
    fatal("Mapping has zero refs.\n");

  if(!m->data)
    fatal("Mapping has no data block.\n");

  if (!m->data->refs)
    fatal("Mapping data block has zero refs.\n");

  if(m->next && m->next->prev != m)
    fatal("Mapping ->next->prev != mapping.\n");

  if(m->debug_size != md->size)
    fatal("Mapping zapping detected!\n");

  if(m->prev)
  {
    if(m->prev->next != m)
      fatal("Mapping ->prev->next != mapping.\n");
  }else{
    if(first_mapping != m)
      fatal("Mapping ->prev == 0 but first_mapping != mapping.\n");
  }

  if(md->valrefs <0)
    fatal("md->valrefs  < 0\n");

  if(md->hardlinks <0)
    fatal("md->valrefs  < 0\n");

  if(md->refs < md->valrefs+1)
    fatal("md->refs < md->valrefs+1\n");

  if(md->valrefs < md->hardlinks)
    fatal("md->refs < md->valrefs+1\n");

  if(md->hashsize < 0)
    fatal("Assert: I don't think he's going to make it Jim.\n");

  if(md->size < 0)
    fatal("Core breach, evacuate ship!\n");

  if(md->num_keypairs < 0)
    fatal("Starboard necell on fire!\n");

  if(md->size > md->num_keypairs)
    fatal("Pretty mean hashtable there buster!\n");

  if(md->hashsize > md->num_keypairs)
    fatal("Pretty mean hashtable there buster (2)!\n");

  if(md->num_keypairs > (md->hashsize + 3) * AVG_LINK_LENGTH)
    fatal("Mapping from hell detected, attempting to send it back...\n");
  
  if(md->size > 0 && (!md->ind_types || !md->val_types))
    fatal("Mapping type fields are... wrong.\n");

  num=0;
  NEW_MAPPING_LOOP(md)
    {
      num++;

      if(! ( (1 << k->ind.type) & (md->ind_types) ))
	fatal("Mapping indices type field lies.\n");

      if(! ( (1 << k->val.type) & (md->val_types) ))
	fatal("Mapping values type field lies.\n");

      check_svalue(& k->ind);
      check_svalue(& k->val);

      /* FIXME add check for k->hval
       * beware that hash_svalue may be threaded and locking
       * is required!!
       */
    }
  
  if(md->size != num)
    fatal("Shields are failing, hull integrity down to 20%%\n");
}

void check_all_mappings(void)
{
  struct mapping *m;
  for(m=first_mapping;m;m=m->next)
    check_mapping(m);
}
#endif


void gc_mark_mapping_as_referenced(struct mapping *m)
{
  INT32 e;
  struct keypair *k;

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    fatal("Zero refs in mapping->data\n");
#endif

  if(gc_mark(m) && gc_mark(m->data))
  {
    if((m->data->ind_types | m->data->val_types) & BIT_COMPLEX)
    {
      MAPPING_LOOP(m)
      {
	/* We do not want to count this key:index pair if
	 * the index is a destructed object or function
	 */
	if(((1 << k->ind.type) & (BIT_OBJECT | BIT_FUNCTION)) &&
	   !(k->ind.u.object->prog))
	  continue;

	if (m->flags & MAPPING_FLAG_WEAK)
	{
	  if (k->ind.type == T_OBJECT &&
	      k->ind.u.object->prog->flags & PROGRAM_NO_WEAK_FREE)
	    gc_mark_svalues(&k->ind, 1);
	  if (k->val.type == T_OBJECT && k->val.u.object->prog &&
	      k->val.u.object->prog->flags & PROGRAM_NO_WEAK_FREE)
	    gc_mark_svalues(&k->val, 1);
	}
	else {
	  gc_mark_svalues(&k->ind, 1);
	  gc_mark_svalues(&k->val, 1);
	}
      }
    }
  }
}

void gc_check_all_mappings(void)
{
  INT32 e;
  struct keypair *k;
  struct mapping *m;

  for(m=first_mapping;m;m=m->next)
  {
#ifdef DEBUG_MALLOC
    if (((int)m->data) == 0x55555555) {
      fprintf(stderr, "** Zapped mapping in list of active mappings!\n");
      describe_something(m, T_MAPPING, 1);
      fatal("Zapped mapping in list of active mappings!\n");
    }
#endif /* DEBUG_MALLOC */
    if((m->data->ind_types | m->data->val_types) & BIT_COMPLEX)
    {
      if(gc_check(m->data)) continue;

      MAPPING_LOOP(m)
      {
	/* We do not want to count this key:index pair if
	 * the index is a destructed object or function
	 */
	if(((1 << k->ind.type) & (BIT_OBJECT | BIT_FUNCTION)) &&
	   !(k->ind.u.object->prog))
	  continue;
	  
	debug_gc_check_svalues(&k->ind, 1, T_MAPPING, m);
	m->data->val_types |= debug_gc_check_svalues(&k->val, 1, T_MAPPING, m);
      }

#ifdef PIKE_DEBUG
      if(d_flag > 1) check_mapping_type_fields(m);
#endif

    }
  }
}

void gc_mark_all_mappings(void)
{
  struct mapping *m;
  for(m=first_mapping;m;m=m->next)
    if(gc_is_referenced(m))
      gc_mark_mapping_as_referenced(m);
}

void gc_free_all_unreferenced_mappings(void)
{
  INT32 e;
  struct keypair *k,**prev;
  struct mapping *m,*next;
  struct mapping_data *md;

  for(m=first_mapping;m;m=next)
  {
    check_mapping_for_destruct(m);
    if(gc_do_free(m))
    {
      add_ref(m);
      md = m->data;
      /* Protect against unlink_mapping_data() recursing too far. */
      m->data=&empty_data;
      m->data->refs++;

      unlink_mapping_data(md);
      next=m->next;
#ifdef PIKE_DEBUG
      m->debug_size=0;
#endif

      free_mapping(m);
    }
    else if(m->flags & MAPPING_FLAG_WEAK)
    {
      add_ref(m);
      md=m->data;
      /* no locking required (no is_eq) */
      for(e=0;e<md->hashsize;e++)
      {
	for(prev= md->hash + e;(k=*prev);)
	{
	  if((k->val.type <= MAX_COMPLEX &&
	      !(k->val.type == T_OBJECT &&
		k->val.u.object->prog->flags & PROGRAM_NO_WEAK_FREE) &&
	      gc_do_free(k->val.u.refs)) ||
	     (k->ind.type <= MAX_COMPLEX &&
	      !(k->ind.type == T_OBJECT &&
		k->ind.u.object->prog->flags & PROGRAM_NO_WEAK_FREE) &&
	      gc_do_free(k->ind.u.refs)))
	  {
	    PREPARE_FOR_INDEX_CHANGE();
	    *prev=k->next;
	    free_svalue(& k->ind);
	    free_svalue(& k->val);
	    k->next=md->free_list;
	    md->free_list=k;
	    md->size--;
#ifdef PIKE_DEBUG
	    m->debug_size--;
#endif
	  }else{
	    prev=&k->next;
	  }
	}
      }
      if(MAP_SLOTS(md->size) < md->hashsize * MIN_LINK_LENGTH)
      {
	debug_malloc_touch(m);
	rehash(m, MAP_SLOTS(md->size));
      }
      next=m->next;
      free_mapping(m);
    }
    else
    {
      next=m->next;
    }
  }
}

#ifdef PIKE_DEBUG

void simple_describe_mapping(struct mapping *m)
{
  char *s;
  init_buf();
  describe_mapping(m,0,2);
  s=simple_free_buf();
  fprintf(stderr,"%s\n",s);
  free(s);
}


void debug_dump_mapping(struct mapping *m)
{
  fprintf(stderr, "Refs=%d, next=%p, prev=%p",
	  m->refs, m->next, m->prev);
  if (((int)m->data) & 3) {
    fprintf(stderr, ", data=%p (unaligned)\n", m->data);
  } else {
    fprintf(stderr, ", size=%d, hashsize=%d\n",
	    m->data->size, m->data->hashsize);
    fprintf(stderr, "Indices type field = ");
    debug_dump_type_field(m->data->ind_types);
    fprintf(stderr, "\n");
    fprintf(stderr, "Values type field = ");
    debug_dump_type_field(m->data->val_types);
    fprintf(stderr, "\n");
    simple_describe_mapping(m);
  }
}
#endif

void zap_all_mappings(void)
{
  INT32 e;
  struct keypair *k;
  struct mapping *m,*next;
  struct mapping_data *md;

  for(m=first_mapping;m;m=next)
  {
    add_ref(m);

#if defined(PIKE_DEBUG) && defined(DEBUG_MALLOC)
    if(verbose_debug_exit)
      debug_dump_mapping(m);
#endif

    md=m->data;
    for(e=0;e<md->hashsize;e++)
    {
      while((k=md->hash[e]))
      {
	md->hash[e]=k->next;
	k->next=md->free_list;
	md->free_list=k;
	free_svalue(&k->ind);
	free_svalue(&k->val);
      }
    }
    md->size=0;
#ifdef PIKE_DEBUG
    m->debug_size=0;
#endif
    
    next=m->next;
    
    /* free_mapping(m); */
  }
}
