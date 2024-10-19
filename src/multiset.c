/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"

/* Multisets using rbtree.
 *
 * Created by Martin Stjernholm 2001-05-07
 */

#include "builtin_functions.h"
#include "gc.h"
#include "interpret.h"
#include "multiset.h"
#include "mapping.h"
#include "object.h"
#include "opcodes.h"
#include "pike_error.h"
#include "rbtree_low.h"
#include "svalue.h"
#include "las.h"

#ifdef TEST_MULTISET
#include "constants.h"
#endif

#include "block_allocator.h"

/* FIXME: Optimize finds and searches on type fields? (But not when
 * objects are involved!) Well.. Although cheap I suspect it pays off
 * so extremely seldom that it isn't worth it. /mast */

/* The following defines the allocation policy. It's almost the same
 * as for mappings. */
#define ALLOC_SIZE(size) ((size) ? (size) + 4 : 0)
#define ENLARGE_SIZE(size) (((size) << 1) + 4)
#define DO_SHRINK(msd, extra) ((((msd)->size + extra) << 2) + 4 <= (msd)->allocsize)

/* For use with TEST_MULTISET: Always goes into the tracking code in
 * the various find functions, as if a destructed index was encountered. */
/* #define TEST_MULTISET_TRACKING_PATHS */

#if defined (PIKE_DEBUG) || defined (TEST_MULTISET)
static void debug_dump_ind_data (struct msnode_ind *node,
				 struct multiset_data *msd);
DECLSPEC(noreturn) static void PIKE_UNUSED_ATTRIBUTE debug_multiset_fatal (
  struct multiset *l, const char *fmt, ...) ATTRIBUTE((noreturn, format (printf, 2, 3)));
#define multiset_fatal (fprintf (stderr, "%s:%d: Fatal in multiset: ", \
				 __FILE__, __LINE__), debug_multiset_fatal)
#endif

#ifdef PIKE_DEBUG

/* To get good type checking. */
static inline union msnode **msnode_ptr_check (union msnode **x)
  {return x;}
static inline struct msnode_ind *msnode_ind_check (struct msnode_ind *x)
  {return x;}

static union msnode *low_multiset_find_eq (struct multiset *l, struct svalue *key);

#define sub_extra_ref(X) do {						\
    if (!sub_ref (X)) Pike_fatal ("Got too few refs to " #X ".\n");	\
  } while (0)

PMOD_EXPORT const char msg_no_multiset_flag_marker[] =
  "Multiset index lacks MULTISET_FLAG_MARKER. It might be externally clobbered.\n";
PMOD_EXPORT const char msg_multiset_no_node_refs[] =
  "Multiset got no node refs.\n";

#else

#define msnode_ptr_check(X) ((union msnode **) (X))
#define msnode_ind_check(X) ((struct msnode_ind *) (X))

#define sub_extra_ref(X) do {sub_ref (X);} while (0)

#endif

/* #define MULTISET_ALLOC_DEBUG */
#ifdef MULTISET_ALLOC_DEBUG
#define ALLOC_TRACE(X) X
#else
#define ALLOC_TRACE(X)
#endif

/* #define MULTISET_CMP_DEBUG */
#if defined (MULTISET_CMP_DEBUG) && defined (PIKE_DEBUG)

#define INTERNAL_CMP(A, B, CMP_RES) do {				\
    struct svalue *_cmp_a_ = (A);					\
    struct svalue *_cmp_b_ = (B);					\
    int _cmp_res_;							\
    if (Pike_interpreter.trace_level) {					\
      fputs ("internal cmp ", stderr);					\
      print_svalue (stderr, _cmp_a_);					\
      fprintf (stderr, " (%p) <=> ", _cmp_a_->u.refs);			\
      print_svalue (stderr, _cmp_b_);					\
      fprintf (stderr, " (%p): ", _cmp_b_->u.refs);			\
    }									\
    _cmp_res_ = (CMP_RES) = set_svalue_cmpfun (_cmp_a_, _cmp_b_);	\
    if (Pike_interpreter.trace_level)					\
      fprintf (stderr, "%d\n", _cmp_res_);				\
  } while (0)

#define EXTERNAL_CMP(CMP_LESS) do {					\
    if (Pike_interpreter.trace_level) {					\
      fputs ("external cmp ", stderr);					\
      print_svalue (stderr, Pike_sp - 2);                               \
      fputs (" <=> ", stderr);						\
      print_svalue (stderr, Pike_sp - 1);                               \
      fputs (": ", stderr);						\
    }									\
    apply_svalue (CMP_LESS, 2);						\
    if (Pike_interpreter.trace_level) {					\
      print_svalue (stderr, Pike_sp - 1);                               \
      fputc ('\n', stderr);						\
    }									\
  } while (0)

#else

#define INTERNAL_CMP(A, B, CMP_RES) do {				\
    (CMP_RES) = set_svalue_cmpfun (A, B);				\
  } while (0)

#define EXTERNAL_CMP(CMP_LESS) do {					\
    apply_svalue (CMP_LESS, 2);						\
  } while (0)

#endif

#define SAME_CMP_LESS(MSD_A, MSD_B)					\
  is_identical (&(MSD_A)->cmp_less, &(MSD_B)->cmp_less)

union nodeptr {
  union msnode *ms;
  struct rb_node_hdr *rb;
};

#define HDR(NODE) ((struct rb_node_hdr *) msnode_check (NODE))
#define PHDR(NODEPTR) (&((union nodeptr *) msnode_ptr_check (NODEPTR))->rb)
#define RBNODE(NODE) ((union msnode *) rb_node_check (NODE))
#define INODE(NODE) ((union msnode *) msnode_ind_check (NODE))

#define NEXT_FREE(NODE) INODE (msnode_check (NODE)->i.next)
#define SET_NEXT_FREE(NODE, NEXT)					\
  (msnode_check (NODE)->i.next = (struct msnode_ind *) msnode_check (NEXT))

#define DELETED_PREV(NODE) INODE (msnode_check (NODE)->i.prev)
#define DELETED_NEXT(NODE) ((union msnode *) msnode_check (NODE)->i.ind.u.ptr)

#define NODE_AT(MSD, TYPE, POS) ((struct TYPE *)((char*)(MSD) + OFFSETOF(multiset_data, nodes)) + (POS))
#define NODE_OFFSET(TYPE, POS)						\
  (OFFSETOF(multiset_data, nodes) + (POS)*sizeof(struct TYPE))

#define SHIFT_PTR(PTR, FROM, TO) ((char *) (PTR) - (char *) (FROM) + (char *) (TO))
#define SHIFT_NODEPTR(NODEPTR, FROM_MSD, TO_MSD)			\
  ((union msnode *) SHIFT_PTR (msnode_check (NODEPTR), FROM_MSD, TO_MSD))
#define SHIFT_HDRPTR(HDRPTR, FROM_MSD, TO_MSD)				\
  ((struct rb_node_hdr *) SHIFT_PTR (rb_node_check (HDRPTR), FROM_MSD, TO_MSD))

#define COPY_NODE_PTRS(OLD, OLDBASE, NEW, NEWBASE, TYPE) do {		\
    (NEW)->prev = (OLD)->prev ?						\
      (struct TYPE *) SHIFT_PTR ((OLD)->prev, OLDBASE, NEWBASE) : NULL;	\
    (NEW)->next = (OLD)->next ?						\
      (struct TYPE *) SHIFT_PTR ((OLD)->next, OLDBASE, NEWBASE) : NULL;	\
  } while (0)

#define COPY_DELETED_PTRS_EXTRA(OLD, OLDBASE, NEW, NEWBASE) do {	\
    (NEW)->ind.u.ptr = (OLD)->ind.u.ptr ?				\
      SHIFT_PTR ((OLD)->ind.u.ptr, OLDBASE, NEWBASE) : NULL;		\
  } while (0)

#define COPY_NODE_IND(OLD, NEW, TYPE) do {				\
    (NEW)->ind = (OLD)->ind;						\
    SET_SVAL_TYPE((NEW)->ind,						\
		  TYPEOF((NEW)->ind) & ~MULTISET_FLAG_MASK);		\
    add_ref_svalue (&(NEW)->ind);					\
    SET_SVAL_TYPE((NEW)->ind, TYPEOF((OLD)->ind));			\
  } while (0)

#define EXPAND_ARG(X) X
#define IGNORE_ARG(X)

#define DO_WITH_NODES(MSD) do {						\
    WITH_NODES_BLOCK (msnode_ind, EXPAND_ARG);				\
  } while (0)

struct multiset *first_multiset = NULL;
struct multiset *gc_internal_multiset = NULL;
static struct multiset *gc_mark_multiset_pos = NULL;

static struct multiset_data empty_ind_msd = {
  GC_HEADER_INIT(1), 0,
  NULL, NULL,
  SVALUE_INIT_INT (0),
  0, 0, 0,
  BIT_INT,
  0,
  {{{NULL, NULL, SVALUE_INIT_INT (0)}}}
};

static struct multiset_data empty_weak_ind_msd = {
  GC_HEADER_INIT(1), 0,
  NULL, NULL,
  SVALUE_INIT_INT (0),
  0, 0, 0,
  BIT_INT,
  MULTISET_WEAK_INDICES,
  {{{NULL, NULL, SVALUE_INIT_INT (0)}}}
};

void free_multiset_data (struct multiset_data *msd);

#define INIT_MULTISET(L) do {						\
    GC_ALLOC (L);							\
    L->refs = 0;							\
    add_ref(L);	/* For DMALLOC... */					\
    L->node_refs = 0;							\
    DOUBLELINK (first_multiset, L);					\
  } while (0)

static struct block_allocator multiset_allocator = BA_INIT_PAGES(sizeof(struct multiset), 2);

PMOD_EXPORT void really_free_multiset (struct multiset *l) {
#ifdef PIKE_DEBUG
    if (l->refs) {
# if DEBUG_MALLOC
	describe_something (l, T_MULTISET, 0,2,0, NULL);
# endif
	Pike_fatal ("Too few refs %d to multiset.n", l->refs);
    }
    if (l->node_refs) {
# if DEBUG_MALLOC
	describe_something (l, T_MULTISET, 0,2,0, NULL);
# endif
	Pike_fatal ("Freeing multiset with %d node refs.n", l->node_refs);
    }
#endif
    if (!sub_ref (l->msd)) free_multiset_data (l->msd);
    DOUBLEUNLINK (first_multiset, l);
    GC_FREE (l);
    ba_free(&multiset_allocator, l);
}

PMOD_EXPORT void count_memory_in_multisets (size_t * n, size_t * s) {
    struct multiset *l;
    double datasize = 0.0;

    ba_count_all(&multiset_allocator, n, s);

    for (l = first_multiset; l; l = l->next)
      datasize += (NODE_OFFSET (msnode_ind, l->msd->allocsize)) /
	(double) l->msd->refs;
    *s += (size_t) datasize;
}


static void multiset_delete_node (struct multiset *l, ptrdiff_t nodepos);

/* Note: The returned block has no refs. */
#ifdef PIKE_DEBUG
#define low_alloc_multiset_data(ALLOCSIZE, FLAGS) \
  low_alloc_multiset_data_2 (ALLOCSIZE, FLAGS, 0)
static struct multiset_data *low_alloc_multiset_data_2 (int allocsize,
							int flags,
							int allow_alloc_in_gc)
#else
static struct multiset_data *low_alloc_multiset_data (int allocsize, int flags)
#endif
{
  struct multiset_data *msd;

#ifdef PIKE_DEBUG
  if (allocsize < 0) Pike_fatal ("Invalid alloc size %d\n", allocsize);
  if (!allow_alloc_in_gc &&
      Pike_in_gc > GC_PASS_PREPARE && Pike_in_gc < GC_PASS_FREE)
    Pike_fatal ("Allocating multiset data blocks within gc is not allowed.\n");
#endif

  msd = xalloc ( NODE_OFFSET (msnode_ind, allocsize) );
  gc_init_marker(msd);
  msd->refs = msd->noval_refs = 0;
  msd->root = NULL;
  msd->allocsize = allocsize;
  msd->size = 0;
  msd->ind_types = 0;
  msd->val_types = BIT_INT;
  msd->flags = flags;
  msd->free_list = NULL;	/* Use fix_free_list to init this. */

  ALLOC_TRACE (fprintf (stderr, "%p alloced size %d\n", msd, allocsize));
  return msd;
}

struct tree_build_data
{
  union msnode *list;		/* List of nodes in msd linked with rb_next() */
  union msnode *node;		/* If set, a single extra node in msd. */
  struct multiset_data *msd;	/* Contains tree finished so far. */
  struct multiset *l;		/* If set, a multiset with an extra
				 * ref (containing another msd). */
  struct multiset_data *msd2;	/* If set, yet another msd with an extra ref. */
};

static void free_tree_build_data (struct tree_build_data *build)
{
  union msnode *node, *next;

#define WITH_NODES_BLOCK(TYPE, IND)					\
  if ((node = build->node)) {						\
    SET_SVAL_TYPE(node->i.ind,						\
		  TYPEOF(node->i.ind) & ~MULTISET_FLAG_MASK);		\
    free_svalue (&node->i.ind);						\
  }									\
  if ((node = build->list))						\
    do {								\
      next = low_multiset_next (node);					\
      SET_SVAL_TYPE(node->i.ind,					\
		    TYPEOF(node->i.ind) & ~MULTISET_FLAG_MASK);		\
      free_svalue (&node->i.ind);					\
    } while ((node = next));

  DO_WITH_NODES (build->msd);

#undef WITH_NODES_BLOCK

  free_multiset_data (build->msd);
  if (build->l) free_multiset (build->l);
  if (build->msd2 && !sub_ref (build->msd2)) free_multiset_data (build->msd2);
}

void free_multiset_data (struct multiset_data *msd)
{
  union msnode *node, *next;

#ifdef PIKE_DEBUG
  if (msd->refs)
    Pike_fatal ("Attempt to free multiset_data with refs.\n");
  if (msd->noval_refs)
    Pike_fatal ("There are forgotten noval_refs.\n");
#endif

  /* We trust as few values as possible here, e.g. size and
   * free_list are ignored. */

  GC_FREE_BLOCK (msd);

  free_svalue (&msd->cmp_less);

  if ((node = low_multiset_first (msd))) {
    /* Note: Can't check for MULTISET_FLAG_MARKER here; see e.g. the
     * error recovery case in mkmultiset_2. */
#define WITH_NODES_BLOCK(TYPE, IND)					\
    do {								\
      next = low_multiset_next (node);					\
      SET_SVAL_TYPE(node->i.ind,					\
		    TYPEOF(node->i.ind) & ~MULTISET_FLAG_MASK);		\
      free_svalue (&node->i.ind);					\
    } while ((node = next));

    DO_WITH_NODES (msd);

#undef WITH_NODES_BLOCK
  }

  ALLOC_TRACE (fprintf (stderr, "%p free\n", msd));
  xfree (msd);
}

static void free_indirect_multiset_data (struct multiset_data **pmsd)
{
  if (*pmsd && !sub_ref (*pmsd)) free_multiset_data (*pmsd);
}

struct recovery_data
{
  struct multiset_data *a_msd;	/* If nonzero, it's freed by free_recovery_data */
  struct multiset_data *b_msd;	/* If nonzero, it's freed by free_recovery_data */
};

static void free_recovery_data (struct recovery_data *rd)
{
  if (rd->a_msd && !sub_ref (rd->a_msd)) free_multiset_data (rd->a_msd);
  if (rd->b_msd && !sub_ref (rd->b_msd)) free_multiset_data (rd->b_msd);
}

/* Links the nodes from and including first_free to the end of the
 * node block onto (the beginning of) msd->free_list. */
static void fix_free_list (struct multiset_data *msd, int first_free)
{
  int alloclast = msd->allocsize - 1;

  if (first_free <= alloclast) {
    union msnode *orig_free_list = msd->free_list;

#define WITH_NODES_BLOCK(TYPE, IND)					\
    msd->free_list = (union msnode *) NODE_AT (msd, TYPE, first_free);	\
    for (; first_free < alloclast; first_free++) {			\
      SET_NEXT_FREE ((union msnode *) NODE_AT (msd, TYPE, first_free),	\
		     (union msnode *) NODE_AT (msd, TYPE, first_free + 1)); \
      /* By setting prev to NULL we avoid shifting around garbage in */	\
      /* COPY_NODE_PTRS. */						\
      NODE_AT (msd, TYPE, first_free)->prev = NULL;			\
      SET_SVAL_TYPE(NODE_AT (msd, TYPE, first_free)->ind,		\
		    PIKE_T_UNKNOWN);					\
    }									\
    SET_NEXT_FREE ((union msnode *) NODE_AT (msd, TYPE, first_free),	\
		   orig_free_list);					\
    NODE_AT (msd, TYPE, first_free)->prev = NULL;			\
    SET_SVAL_TYPE(NODE_AT (msd, TYPE, first_free)->ind,			\
		  PIKE_T_UNKNOWN);

    DO_WITH_NODES (msd);

#undef WITH_NODES_BLOCK
  }
}

static void clear_deleted_on_free_list (struct multiset_data *msd)
{
  union msnode *node = msd->free_list;
  assert (node && TYPEOF(node->i.ind) == T_DELETED);
  do {
    node->i.prev = NULL;
    SET_SVAL_TYPE(node->i.ind, PIKE_T_UNKNOWN);
    msd->size--;
  } while ((node = NEXT_FREE (node)) && TYPEOF(node->i.ind) == T_DELETED);
}

#define CLEAR_DELETED_ON_FREE_LIST(MSD) do {				\
    assert ((MSD)->refs == 1);						\
    if ((MSD)->free_list &&						\
	TYPEOF((MSD)->free_list->i.ind) == T_DELETED)			\
      clear_deleted_on_free_list (MSD);					\
  } while (0)

/* The copy has no refs. The copy is verbatim, i.e. the relative node
 * positions are kept. */
static struct multiset_data *copy_multiset_data (struct multiset_data *old)
{
  /* Note approximate code duplication in resize_multiset_data and
   * multiset_set_flags. */

  int pos = old->allocsize;
  struct multiset_data *new = low_alloc_multiset_data (pos, old->flags);
  assign_svalue_no_free (&new->cmp_less, &old->cmp_less);
  new->ind_types = old->ind_types;
  new->val_types = old->val_types;

#define WITH_NODES_BLOCK(TYPE, IND)					\
  struct TYPE *onode, *nnode;						\
  while (pos-- > 0) {							\
    onode = NODE_AT (old, TYPE, pos);					\
    nnode = NODE_AT (new, TYPE, pos);					\
    COPY_NODE_PTRS (onode, old, nnode, new, TYPE);			\
    switch (TYPEOF(onode->ind)) {					\
      case T_DELETED:							\
	COPY_DELETED_PTRS_EXTRA (onode, old, nnode, new);		\
	/* FALLTHRU */						\
      case PIKE_T_UNKNOWN:						\
	SET_SVAL_TYPE(nnode->ind, TYPEOF(onode->ind));			\
	break;								\
      default:								\
	COPY_NODE_IND (onode, nnode, TYPE);				\
    }									\
  }

  DO_WITH_NODES (new);

#undef WITH_NODES_BLOCK

  if (old->free_list) new->free_list = SHIFT_NODEPTR (old->free_list, old, new);
  if (old->root) new->root = SHIFT_NODEPTR (old->root, old, new);
  new->size = old->size;

  ALLOC_TRACE (fprintf (stderr, "%p -> %p: copied, alloc size %d, data size %d\n",
			old, new, new->allocsize, new->size));
  return new;
}

/* The first part of the new data block is a verbatim copy of the old
 * one if verbatim is nonzero. This mode also handles link structures
 * that aren't proper trees. If verbatim is zero, the tree is
 * rebalanced, since the operation is already linear. The copy has no
 * refs.
 *
 * The resize does not change the refs in referenced svalues, so the
 * old block is always freed. The refs and noval_refs are transferred
 * to the new block. */
static struct multiset_data *resize_multiset_data (struct multiset_data *old,
						   INT64 newsize, int verbatim)
{
  struct multiset_data *new;

#ifdef PIKE_DEBUG
  if (old->refs > 1)
    Pike_fatal ("Attempt to resize multiset_data with several refs.\n");
  if (verbatim) {
    if (newsize < old->allocsize)
      Pike_fatal ("Cannot shrink multiset_data (from %d to %"PRINTINT64"d) in verbatim mode.\n",
	     old->allocsize, newsize);
  }
  else
    if (newsize < old->size)
      Pike_fatal ("Cannot resize multiset_data with %d elements to %"PRINTINT64"d.\n",
	     old->size, newsize);
  if (newsize == old->allocsize)
    Pike_fatal ("Unnecessary resize of multiset_data to same size.\n");
#endif

  /* Note approximate code duplication in copy_multiset_data and
   * multiset_set_flags. */

#ifdef PIKE_DEBUG
  /* We can realloc a block even inside the gc. */
  new = low_alloc_multiset_data_2 (newsize, old->flags, 1);
#else
  new = low_alloc_multiset_data (newsize, old->flags);
#endif
  move_svalue (&new->cmp_less, &old->cmp_less);
  new->ind_types = old->ind_types;
  new->val_types = old->val_types;

  if (verbatim) {
    int pos = old->allocsize;
    fix_free_list (new, pos);

#define WITH_NODES_BLOCK(TYPE, IND)					\
    struct TYPE *oldnodes = (struct TYPE *) old->nodes;			\
    struct TYPE *newnodes = (struct TYPE *) new->nodes;			\
    struct TYPE *onode, *nnode;						\
    while (pos-- > 0) {							\
      onode = NODE_AT (old, TYPE, pos);					\
      nnode = NODE_AT (new, TYPE, pos);					\
      COPY_NODE_PTRS (onode, old, nnode, new, TYPE);			\
      switch (TYPEOF(onode->ind)) {					\
	case T_DELETED:							\
	  COPY_DELETED_PTRS_EXTRA (onode, old, nnode, new);		\
	  /* FALLTHRU */						\
	  case PIKE_T_UNKNOWN:						\
	    SET_SVAL_TYPE(nnode->ind, TYPEOF(onode->ind));		\
	  break;							\
	default:							\
	  nnode->ind = onode->ind;					\
      }									\
    }

    DO_WITH_NODES (new);

#undef WITH_NODES_BLOCK

    if (old->free_list) {
      union msnode *list = SHIFT_NODEPTR (old->free_list, old, new);
      union msnode *node, *next;
      for (node = list; (next = NEXT_FREE (node)); node = next) {}
      SET_NEXT_FREE (node, new->free_list);
      new->free_list = list;
    }
    if (old->root) new->root = SHIFT_NODEPTR (old->root, old, new);
    new->size = old->size;
  }

  else {
    union msnode *oldnode;
    if ((oldnode = low_multiset_first (old))) {
      int pos = 0;

#define WITH_NODES_BLOCK(TYPE, IND)					\
      struct TYPE *node;						\
      while (1) {							\
	node = NODE_AT (new, TYPE, pos);				\
	node->ind = oldnode->i.ind;					\
	if (!(oldnode = low_multiset_next (oldnode))) break;		\
	node->next = NODE_AT (new, TYPE, ++pos);			\
      }									\
      NODE_AT (new, TYPE, pos)->next = NULL;

      DO_WITH_NODES (new);

#undef WITH_NODES_BLOCK

      new->size = ++pos;
      fix_free_list (new, pos);
      new->root = RBNODE (rb_make_tree (HDR (new->nodes), pos));
    }
    else
      fix_free_list (new, 0);
  }

  ALLOC_TRACE (fprintf (stderr, "%p -> %p: resized from %d to %d, data size %d (%s)\n",
			old, new, old->allocsize, new->allocsize, new->size,
			verbatim ? "verbatim" : "rebalance"));

  new->refs = old->refs;
  new->noval_refs = old->noval_refs;

  GC_REALLOC_BLOCK (old, new);
  xfree (old);

  return new;
}

#define MOVE_MSD_REF(L, MSD) do {					\
    sub_extra_ref (MSD);						\
    add_ref ((MSD) = (L)->msd);						\
  } while (0)

#define MOVE_MSD_REF_AND_FREE(L, MSD) do {				\
    if (!sub_ref (MSD)) free_multiset_data (MSD);			\
    add_ref ((MSD) = (L)->msd);						\
  } while (0)

/* There are several occasions when we might get "inflated" msd
 * blocks, i.e. ones that are larger than the allocation strategy
 * allows. This happens e.g. when combining node references with
 * shared data blocks, and when the gc removes nodes in shared data
 * blocks. Therefore all the copy-on-write functions tries to shrink
 * them. */

static int prepare_for_change (struct multiset *l, int verbatim)
{
  struct multiset_data *msd = l->msd;
  int msd_changed = 0;

#ifdef PIKE_DEBUG
  if (!verbatim && l->node_refs)
    Pike_fatal ("The verbatim flag not set for multiset with node refs.\n");
#endif

  if (msd->refs > 1) {
    l->msd = copy_multiset_data (msd);
    MOVE_MSD_REF (l, msd);
    msd_changed = 1;
    if (!l->node_refs)
      /* Look at l->node_refs and not verbatim here, since when
       * verbatim is nonzero while l->node_refs is zero, we're only
       * interested in keeping the tree structure for the allocated
       * nodes. */
      CLEAR_DELETED_ON_FREE_LIST (msd);
  }

  if (!verbatim && DO_SHRINK (msd, 0)) {
#ifdef PIKE_DEBUG
    if (d_flag > 1) check_multiset (l, 1);
#endif
    l->msd = resize_multiset_data (msd, ALLOC_SIZE (msd->size), 0);
    msd_changed = 1;
  }

  return msd_changed;
}

static int prepare_for_add (struct multiset *l, int verbatim)
{
  struct multiset_data *msd = l->msd;
  int msd_changed = 0;

#ifdef PIKE_DEBUG
  if (!verbatim && l->node_refs)
    Pike_fatal ("The verbatim flag not set for multiset with node refs.\n");
#endif

  if (msd->refs > 1) {
    l->msd = copy_multiset_data (msd);
    MOVE_MSD_REF (l, msd);
    msd_changed = 1;
    if (!l->node_refs) CLEAR_DELETED_ON_FREE_LIST (msd);
  }

  if (msd->size == msd->allocsize) {
    if (!l->node_refs) CLEAR_DELETED_ON_FREE_LIST (msd);
    if (msd->size == msd->allocsize) {
      /* Can't call check_multiset here, since it might not even be a
       * proper tree in verbatim mode. */
      l->msd = resize_multiset_data (msd, ENLARGE_SIZE (msd->allocsize), verbatim);
      return 1;
    }
  }

  if (!verbatim && DO_SHRINK (msd, 1)) {
#ifdef PIKE_DEBUG
    if (d_flag > 1) check_multiset (l, 1);
#endif
    l->msd = resize_multiset_data (msd, ALLOC_SIZE (msd->size + 1), 0);
    return 1;
  }

  return msd_changed;
}

static union msnode *alloc_msnode_verbatim (struct multiset_data *msd)
{
  union msnode *node = msd->free_list;
#ifdef PIKE_DEBUG
  if (!node) Pike_fatal ("Verbatim multiset data block unexpectedly full.\n");
#endif

  if (TYPEOF(node->i.ind) == T_DELETED) {
    union msnode *prev;
    do {
      prev = node;
      node = NEXT_FREE (node);
#ifdef PIKE_DEBUG
      if (!node) Pike_fatal ("Verbatim multiset data block unexpectedly full.\n");
#endif
    } while (TYPEOF(node->i.ind) == T_DELETED);
    SET_NEXT_FREE (prev, NEXT_FREE (node));
  }
  else
    msd->free_list = NEXT_FREE (node);

  msd->size++;
  return node;
}

#define ALLOC_MSNODE(MSD, GOT_NODE_REFS, NODE) do {			\
    if (GOT_NODE_REFS)							\
      (NODE) = alloc_msnode_verbatim (MSD);				\
    else {								\
      (NODE) = (MSD)->free_list;					\
      DO_IF_DEBUG (							\
	if (!(NODE)) Pike_fatal ("Multiset data block unexpectedly full.\n"); \
      );								\
      (MSD)->free_list = NEXT_FREE (NODE);				\
      if (TYPEOF((NODE)->i.ind) == PIKE_T_UNKNOWN) (MSD)->size++;	\
    }									\
  } while (0)

#define ADD_TO_FREE_LIST(MSD, NODE) do {				\
    SET_NEXT_FREE (NODE, (MSD)->free_list);				\
    (MSD)->free_list = (NODE);						\
  } while (0)

static void unlink_msnode (struct multiset *l, struct rbstack_ptr *track,
			   int keep_rbstack)
{
  struct multiset_data *msd = l->msd;
  struct rbstack_ptr rbstack = *track;
  union msnode *unlinked_node;

  if (prepare_for_change (l, 1)) {
    rbstack_shift (rbstack, HDR (msd->nodes), HDR (l->msd->nodes));
    msd = l->msd;
  }

  /* Note: Similar code in gc_unlink_node_shared. */

  if (l->node_refs) {
    union msnode *prev, *next;
    unlinked_node = RBNODE (RBSTACK_PEEK (rbstack));
    prev = low_multiset_prev (unlinked_node);
    next = low_multiset_next (unlinked_node);
    low_rb_unlink_without_move (PHDR (&msd->root), &rbstack, keep_rbstack);
    ADD_TO_FREE_LIST (msd, unlinked_node);
    SET_SVAL_TYPE(unlinked_node->i.ind, T_DELETED);
    unlinked_node->i.prev = (struct msnode_ind *) prev;
    unlinked_node->i.ind.u.ptr = next;
  }

  else {
    unlinked_node =
      RBNODE (low_rb_unlink_with_move (
		PHDR (&msd->root), &rbstack, keep_rbstack,
		sizeof (struct msnode_ind)));
    CLEAR_DELETED_ON_FREE_LIST (msd);
    ADD_TO_FREE_LIST (msd, unlinked_node);
    SET_SVAL_TYPE(unlinked_node->i.ind, PIKE_T_UNKNOWN);
    unlinked_node->i.prev = NULL;
    msd->size--;
    if (!keep_rbstack && DO_SHRINK (msd, 0)) {
#ifdef PIKE_DEBUG
      if (d_flag > 1) check_multiset (l, 1);
#endif
      l->msd = resize_multiset_data (msd, ALLOC_SIZE (msd->size), 0);
    }
  }

  *track = rbstack;
}

PMOD_EXPORT void multiset_clear_node_refs (struct multiset *l)
{
  struct multiset_data *msd = l->msd;

  debug_malloc_touch (l);
  debug_malloc_touch (msd);
  assert (!l->node_refs);

  CLEAR_DELETED_ON_FREE_LIST (msd);
  if (DO_SHRINK (msd, 0)) {
#ifdef PIKE_DEBUG
    if (d_flag > 1) check_multiset (l, 1);
#endif
    l->msd = resize_multiset_data (msd, ALLOC_SIZE (msd->size), 0);
  }
}

PMOD_EXPORT INT32 multiset_sizeof (struct multiset *l)
{
  INT32 size = l->msd->size;
  union msnode *node = l->msd->free_list;
  for (; node && TYPEOF(node->i.ind) == T_DELETED; node = NEXT_FREE (node))
    size--;
  return size;
}

struct multiset *real_allocate_multiset (int allocsize,
						     int flags,
						     struct svalue *cmp_less)
{
  struct multiset *l = ba_alloc(&multiset_allocator);

  /* FIXME: There's currently little use in making "inflated"
   * multisets with allocsize, since prepare_for_add shrinks them. */

#ifdef PIKE_DEBUG
  if (cmp_less) check_svalue (cmp_less);
#endif

  if (allocsize ||
      (cmp_less && TYPEOF(*cmp_less) != T_INT) ||
      (flags)) {
    l->msd = low_alloc_multiset_data (allocsize, flags);
    add_ref (l->msd);
    fix_free_list (l->msd, 0);
    if (cmp_less) {
#ifdef PIKE_DEBUG
      if (TYPEOF(*cmp_less) == T_INT) {
	assert (SUBTYPEOF(*cmp_less) == NUMBER_NUMBER);
	assert (cmp_less->u.integer == 0);
      }
#endif
      assign_svalue_no_free (&l->msd->cmp_less, cmp_less);
    }
    else {
      SET_SVAL(l->msd->cmp_less, T_INT, NUMBER_NUMBER, integer, 0);
    }
  }
  else {
    l->msd = &empty_ind_msd;
    add_ref (l->msd);
  }

  INIT_MULTISET (l);
  return l;
}

PMOD_EXPORT void do_free_multiset (struct multiset *l)
{
  if (l) {
    debug_malloc_touch (l);
    debug_malloc_touch (l->msd);
    free_multiset (l);
  }
}

PMOD_EXPORT void do_sub_msnode_ref (struct multiset *l)
{
  if (l) {
    debug_malloc_touch (l);
    debug_malloc_touch (l->msd);
    sub_msnode_ref (l);
  }
}

PMOD_EXPORT void clear_multiset(struct multiset *l)
{
  if (l) {
    struct multiset_data *msd = l->msd;
    debug_malloc_touch(l);
    debug_malloc_touch(l->msd);
    if (msd->flags & MULTISET_WEAK_INDICES) {
      l->msd = &empty_weak_ind_msd;
    } else {
      l->msd = &empty_ind_msd;
    }
    add_ref(l->msd);
    if (!sub_ref(msd)) free_multiset_data(msd);
  }
}

enum find_types {
  FIND_EQUAL,
  /* FIND_NOEQUAL, */
  FIND_LESS,
  FIND_GREATER,
  FIND_NOROOT,
  FIND_DESTRUCTED,
  FIND_KEY_DESTRUCTED
};

static enum find_types low_multiset_track_eq (
  struct multiset_data *msd, struct svalue *key, struct rbstack_ptr *track);
static enum find_types low_multiset_track_le_gt (
  struct multiset_data *msd, struct svalue *key, struct rbstack_ptr *track);

static void midflight_remove_node_fast (struct multiset *l,
					struct rbstack_ptr *track,
					int keep_rbstack)
{
  /* If the node index is destructed, we could in principle ignore the
   * copy-on-write here and remove it in all copies, but then we'd
   * have to find another way than (l->msd != msd) to signal the tree
   * change to the calling code. */
  struct svalue ind;
  union msnode *node = RBNODE (RBSTACK_PEEK (*track));

  /* Postpone free since the msd might be copied in unlink_node. */
  low_use_multiset_index (node, ind);

  unlink_msnode (l, track, keep_rbstack);

  free_svalue (&ind);
}

/* Like midflight_remove_node_fast but doesn't bother with concurrent
 * changes of the multiset or resizing of the msd. There must not be
 * any node refs to it. */
static void midflight_remove_node_faster (struct multiset_data *msd,
					  struct rbstack_ptr rbstack)
{
  struct svalue ind;
  union msnode *node = RBNODE (RBSTACK_PEEK (rbstack));

  free_svalue (low_use_multiset_index (node, ind));

  node = RBNODE (low_rb_unlink_with_move (
		   PHDR (&msd->root), &rbstack, 0,
		   sizeof (struct msnode_ind)));
  CLEAR_DELETED_ON_FREE_LIST (msd);
  ADD_TO_FREE_LIST (msd, node);
  SET_SVAL_TYPE(node->i.ind, PIKE_T_UNKNOWN);
  msd->size--;
}

PMOD_EXPORT void multiset_set_flags (struct multiset *l, int flags)
{
  struct multiset_data *old = l->msd;

  debug_malloc_touch (l);
  debug_malloc_touch (old);

  if (flags != old->flags) {
    prepare_for_change (l, l->node_refs);
    l->msd->flags = flags;
  }
}

static void multiset_set_cmp_less (struct multiset *l,
					struct svalue *cmp_less)
{
  struct multiset_data *old = l->msd;

#ifdef PIKE_DEBUG
  debug_malloc_touch (l);
  debug_malloc_touch (old);
  if (cmp_less) check_svalue (cmp_less);
#endif

again:
  if (cmp_less ?
      is_identical (cmp_less, &old->cmp_less) :
      TYPEOF(old->cmp_less) == T_INT)
    {}

  else if (!old->root) {
    if (prepare_for_change (l, l->node_refs)) old = l->msd;
    free_svalue (&old->cmp_less);
    if (cmp_less) {
#ifdef PIKE_DEBUG
      if (TYPEOF(*cmp_less) == T_INT) {
	assert (SUBTYPEOF(*cmp_less) == NUMBER_NUMBER);
	assert (cmp_less->u.integer == 0);
      }
#endif
      assign_svalue_no_free (&old->cmp_less, cmp_less);
    }
    else {
      SET_SVAL(old->cmp_less, T_INT, NUMBER_NUMBER, integer, 0);
    }
  }

  else {
    struct tree_build_data new;
    union msnode *next;
    struct svalue ind;
    ONERROR uwp;

    SET_ONERROR (uwp, free_tree_build_data, &new);

    new.l = NULL, new.msd2 = NULL;
    new.msd = copy_multiset_data (old);
    new.list = low_multiset_first (new.msd);
    new.node = NULL;
    new.msd->root = NULL;

    free_svalue (&new.msd->cmp_less);
    if (cmp_less) {
#ifdef PIKE_DEBUG
      if (TYPEOF(*cmp_less) == T_INT) {
	assert (SUBTYPEOF(*cmp_less) == NUMBER_NUMBER);
	assert (cmp_less->u.integer == 0);
      }
#endif
      assign_svalue_no_free (&new.msd->cmp_less, cmp_less);
    }
    else {
      SET_SVAL(new.msd->cmp_less, T_INT, NUMBER_NUMBER, integer, 0);
    }

    do {
      low_use_multiset_index (new.list, ind);
      next = low_multiset_next (new.list);

      /* Note: Similar code in mkmultiset_2 and copy_multiset_recursively. */

      if (IS_DESTRUCTED (&ind))
	new.msd->size--;
      else {
	while (1) {
	  RBSTACK_INIT (rbstack);

	  if (!new.msd->root) {
	    low_rb_init_root (HDR (new.msd->root = new.list));
	    goto node_done;
	  }

	  switch (low_multiset_track_le_gt (new.msd, &ind, &rbstack)) {
	    case FIND_LESS:
	      low_rb_link_at_next (PHDR (&new.msd->root),
				   rbstack, HDR (new.list));
	      goto node_done;
	    case FIND_GREATER:
	      low_rb_link_at_prev (PHDR (&new.msd->root),
				   rbstack, HDR (new.list));
	      goto node_done;
	    case FIND_DESTRUCTED:
	      midflight_remove_node_faster (new.msd, rbstack);
	      new.msd->size--;
	      break;
	    case FIND_KEY_DESTRUCTED:
	      new.msd->size--;
	      goto node_done;
	    default: DO_IF_DEBUG (Pike_fatal ("Invalid find_type.\n"));
	  }
	}
        UNREACHABLE();
      }

    node_done:
      if (l->msd != old) {
	/* l changed. Have to start over to guarantee no loss of data. */
	CALL_AND_UNSET_ONERROR (uwp);
	old = l->msd;
	goto again;
      }
    } while ((new.list = next));

    UNSET_ONERROR (uwp);
    if (!sub_ref (old)) free_multiset_data (old);
    add_ref (l->msd = new.msd);
  }
}
static struct multiset *mkmultiset_2 (struct array *indices,
				      struct svalue *cmp_less);

PMOD_EXPORT struct multiset *mkmultiset (struct array *indices)
{
  debug_malloc_touch (indices);
  return mkmultiset_2 (indices, NULL);
}

/* values may be NULL to make a multiset with indices only. */
static struct multiset *mkmultiset_2 (struct array *indices,
				      struct svalue *cmp_less)
{
  struct multiset *l;
  struct tree_build_data new;

#ifdef PIKE_DEBUG
  debug_malloc_touch (indices);
  if (cmp_less) check_svalue (cmp_less);
#endif

  new.l = NULL, new.msd2 = NULL;
  new.msd = low_alloc_multiset_data (ALLOC_SIZE (indices->size), 0);

  if (cmp_less) {
#ifdef PIKE_DEBUG
    if (TYPEOF(*cmp_less) == T_INT) {
      assert (SUBTYPEOF(*cmp_less) == NUMBER_NUMBER);
      assert (cmp_less->u.integer == 0);
    }
#endif
    assign_svalue_no_free (&new.msd->cmp_less, cmp_less);
  }
  else {
    SET_SVAL(new.msd->cmp_less, T_INT, NUMBER_NUMBER, integer, 0);
  }

  if (!indices->size)
    fix_free_list (new.msd, 0);
  else {
    int pos, size = indices->size;
    ONERROR uwp;

    new.list = NULL;
    SET_ONERROR (uwp, free_tree_build_data, &new);
    new.msd->ind_types = indices->type_field;

    for (pos = 0; pos < size; pos++) {
      new.node = INODE (NODE_AT (new.msd, msnode_ind, pos));
      assign_svalue_no_free (&new.node->i.ind, &ITEM (indices)[pos]);

      /* Note: Similar code in multiset_set_cmp_less and
       * copy_multiset_recursively. */

      /* Note: It would perhaps be a bit faster to use quicksort. */

      if (!IS_DESTRUCTED (&new.node->i.ind)) {
	while (1) {
	  RBSTACK_INIT (rbstack);

	  if (!new.msd->root) {
	    low_rb_init_root (HDR (new.msd->root = new.node));
	    goto node_added;
	  }

	  switch (low_multiset_track_le_gt (new.msd,
					    /* Not clobbered yet. */
					    &new.node->i.ind,
					    &rbstack)) {
	    case FIND_LESS:
	      low_rb_link_at_next (PHDR (&new.msd->root),
				   rbstack, HDR (new.node));
	      goto node_added;
	    case FIND_GREATER:
	      low_rb_link_at_prev (PHDR (&new.msd->root),
				   rbstack, HDR (new.node));
	      goto node_added;
	    case FIND_DESTRUCTED:
	      midflight_remove_node_faster (new.msd, rbstack);
	      break;
	    case FIND_KEY_DESTRUCTED:
	      goto node_skipped;
	    default: DO_IF_DEBUG (Pike_fatal ("Invalid find_type.\n"));
	  }
	}
        UNREACHABLE();

      node_added:
#ifdef PIKE_DEBUG
	SET_SVAL_TYPE(new.node->i.ind,
		      TYPEOF(new.node->i.ind) | MULTISET_FLAG_MARKER);
#endif
	new.msd->size++;
      }

    node_skipped:;
    }

    UNSET_ONERROR (uwp);
    fix_free_list (new.msd, indices->size);
  }

  l = ba_alloc(&multiset_allocator);
  l->msd = new.msd;
  add_ref (new.msd);
  INIT_MULTISET (l);
  return l;
}

PMOD_EXPORT int msnode_is_deleted (struct multiset *l, ptrdiff_t nodepos)
{
  struct multiset_data *msd = l->msd;
  union msnode *node;
  struct svalue ind;

  debug_malloc_touch (l);
  debug_malloc_touch (msd);
  check_msnode (l, nodepos, 1);

  node = OFF2MSNODE (msd, nodepos);

  if (IS_DESTRUCTED (low_use_multiset_index (node, ind))) {
    if (msd->refs == 1) {
      ONERROR uwp;
      add_msnode_ref (l);
      SET_ONERROR (uwp, do_sub_msnode_ref, l);
      multiset_delete_node (l, nodepos);
      UNSET_ONERROR (uwp);
    }
    return 1;
  }

  return TYPEOF(node->i.ind) == T_DELETED;
}

static union msnode *low_multiset_find_eq (struct multiset *l, struct svalue *key)
{
  struct multiset_data *msd = l->msd;
  struct rb_node_hdr *node;
  ONERROR uwp;

  /* Note: Similar code in low_multiset_track_eq. */

  if (IS_DESTRUCTED (key)) {
    /* We only check for a destructed key initially - the result will
     * be bogus if it's destructed during the search. */
    return NULL;
  }

  add_ref (msd);
  SET_ONERROR (uwp, free_indirect_multiset_data, &msd);

  while (1) {
    if ((node = HDR (msd->root))) {
#ifdef TEST_MULTISET_TRACKING_PATHS
      goto index_destructed;
#endif

      if (TYPEOF(msd->cmp_less) == T_INT) {
	struct svalue tmp;
	if (!((msd->ind_types | (1 << TYPEOF(*key))) &
	      (BIT_OBJECT | BIT_FUNCTION))) {
	  /* Can assume an internal order which defines a total order
	   * for all values. */
	  LOW_RB_FIND (
	    node,
	    {
	      low_use_multiset_index (RBNODE (node), tmp);
	      assert (!IS_DESTRUCTED (&tmp));
	      INTERNAL_CMP (key, &tmp, cmp_res);
	      assert (cmp_res != CMPFUN_UNORDERED);
	    },
	    node = NULL, ;, node = NULL);
	}

	else {
	  /* Find the biggest node less or order-wise equal to key. */
	  LOW_RB_FIND_NEQ (
	    node,
	    {
	      low_use_multiset_index (RBNODE (node), tmp);
	      if (IS_DESTRUCTED (&tmp)) goto index_destructed;
	      INTERNAL_CMP (key, &tmp, cmp_res);
	      if (!cmp_res) cmp_res = 1;
	      /* Note: CMPFUN_UNORDERED > 0 - treated as equal. */
	    },
	    {},			/* Got less or equal or unordered. */
	    {node = node->prev;}); /* Got greater - step back one. */

	  /* Step backwards until a less or really equal node is found. */
	  for (; node; node = rb_prev (node)) {
	    int cmp_res;
	    low_use_multiset_index (RBNODE (node), tmp);
	    if (IS_DESTRUCTED (&tmp)) goto index_destructed;
	    if (is_eq (&tmp, key)) break;
	    INTERNAL_CMP (&tmp, key, cmp_res);
	    /* Note: CMPFUN_UNORDERED > 0 - treated as equal. */
	    if (cmp_res < 0) {node = NULL; break;}
	  }
	}
      }

      else {
	/* Find the biggest node less or order-wise equal to key. */
	LOW_RB_FIND_NEQ (
	  node,
	  {
	    push_svalue (key);
	    low_push_multiset_index (RBNODE (node));
            if (IS_DESTRUCTED (Pike_sp - 1)) {
              pop_2_elems();
              goto index_destructed;
            }
	    EXTERNAL_CMP (&msd->cmp_less);
	    cmp_res = UNSAFE_IS_ZERO (Pike_sp - 1) ? 1 : -1;
	    pop_stack();
	  },
	  {},			/* Got less or equal. */
	  {node = node->prev;}); /* Got greater - step back one. */

	/* Step backwards until a less or really equal node is found. */
	for (; node; node = rb_prev (node)) {
	  low_push_multiset_index (RBNODE (node));
          if (IS_DESTRUCTED (Pike_sp - 1)) {
            pop_stack();
            goto index_destructed;
          }
	  if (is_eq (Pike_sp - 1, key)) {pop_stack(); break;}
	  push_svalue (key);
	  EXTERNAL_CMP (&msd->cmp_less);
	  if (!UNSAFE_IS_ZERO (Pike_sp - 1)) {pop_stack(); node = NULL; break;}
	  pop_stack();
	}
      }
    }

    if (l->msd == msd) break;
    /* Will always go into the first if clause below. */

  index_destructed:
    if (l->msd != msd)
      MOVE_MSD_REF_AND_FREE (l, msd);

    else {
      /* Try again with tracking to be able to remove the destructed node. */
      RBSTACK_INIT (rbstack);

      while (msd->root) {
	enum find_types find_type = low_multiset_track_eq (msd, key, &rbstack);

	if (l->msd != msd)
	  MOVE_MSD_REF_AND_FREE (l, msd);

	else if (find_type == FIND_DESTRUCTED) {
	  sub_extra_ref (msd);
	  midflight_remove_node_fast (l, &rbstack, 0);
	  msd = l->msd;
	  add_ref (msd);
	  /* Let's stay in tracking mode - if there's one destructed
	   * index, there might be many. */
	}

	else {
	  node = find_type == FIND_EQUAL ? RBSTACK_PEEK (rbstack) : NULL;
	  RBSTACK_FREE (rbstack);
	  goto done;
	}
      }
      node = NULL;
      RBSTACK_FREE (rbstack);
      goto done;
    }
  } /* while(1) */

done:
  UNSET_ONERROR (uwp);
  sub_extra_ref (msd);
  return RBNODE (node);
}

PMOD_EXPORT ptrdiff_t multiset_first (struct multiset *l)
{
  struct multiset_data *msd = l->msd;
  union msnode *node;
  struct svalue ind;

  debug_malloc_touch (l);
  debug_malloc_touch (msd);

  node = low_multiset_first (msd);
  while (node && IS_DESTRUCTED (low_use_multiset_index (node, ind)))
    if (msd->refs == 1) {
      ONERROR uwp;
      add_msnode_ref (l);
      SET_ONERROR (uwp, do_sub_msnode_ref, l);
      multiset_delete_node (l, MSNODE2OFF (msd, node));
      UNSET_ONERROR (uwp);
      msd = l->msd;
      node = low_multiset_first (msd);
    }
    else
      node = low_multiset_next (node);

  if (node) {
    add_msnode_ref (l);
    return MSNODE2OFF (msd, node);
  }
  else return -1;
}

PMOD_EXPORT ptrdiff_t multiset_last (struct multiset *l)
{
  struct multiset_data *msd = l->msd;
  union msnode *node;
  struct svalue ind;

  debug_malloc_touch (l);
  debug_malloc_touch (msd);

  node = low_multiset_last (msd);
  while (node && IS_DESTRUCTED (low_use_multiset_index (node, ind)))
    if (msd->refs == 1) {
      ONERROR uwp;
      add_msnode_ref (l);
      SET_ONERROR (uwp, do_sub_msnode_ref, l);
      multiset_delete_node (l, MSNODE2OFF (msd, node));
      UNSET_ONERROR (uwp);
      msd = l->msd;
      node = low_multiset_last (msd);
    }
    else
      node = low_multiset_prev (node);

  if (node) {
    add_msnode_ref (l);
    return MSNODE2OFF (msd, node);
  }
  else return -1;
}

/* Returns -1 if there's no predecessor. If the node is deleted, the
 * predecessor of the closest following nondeleted node is returned.
 * If there is no following nondeleted node, the last node is
 * returned. Note that this function never alters the noderefs; it has
 * no opinion whether you "walk" to the previous node or only "peek"
 * at it. */
PMOD_EXPORT ptrdiff_t multiset_prev (struct multiset *l, ptrdiff_t nodepos)
{
  struct multiset_data *msd = l->msd;
  union msnode *node;
  struct svalue ind;

  debug_malloc_touch (l);
  debug_malloc_touch (msd);
  check_msnode (l, nodepos, 1);

  node = OFF2MSNODE (msd, nodepos);

  if (TYPEOF(node->i.ind) == T_DELETED)
    do {
      node = DELETED_NEXT (node);
      if (!node) {
	node = low_multiset_last (msd);
	return node ? MSNODE2OFF (msd, node) : -1;
      }
    } while (TYPEOF(node->i.ind) == T_DELETED);

  node = low_multiset_prev (node);

  while (node && IS_DESTRUCTED (low_use_multiset_index (node, ind))) {
    union msnode *prev = low_multiset_prev (node);
    if (msd->refs == 1) {
      ONERROR uwp;
      nodepos = prev ? MSNODE2OFF (msd, prev) : -1;
      add_msnode_ref (l);
      SET_ONERROR (uwp, do_sub_msnode_ref, l);
      multiset_delete_node (l, MSNODE2OFF (msd, node));
      UNSET_ONERROR (uwp);
      msd = l->msd;
      node = nodepos >= 0 ? OFF2MSNODE (msd, nodepos) : NULL;
    }
    else
      node = prev;
  }

  return node ? MSNODE2OFF (msd, node) : -1;
}

/* Returns -1 if there's no successor. If the node is deleted, the
 * successor of the closest preceding nondeleted node is returned. If
 * there is no preceding nondeleted node, the first node is returned.
 * Note that this function never alters the noderefs; it has no
 * opinion whether you "walk" to the next node or only "peek" at
 * it. */
PMOD_EXPORT ptrdiff_t multiset_next (struct multiset *l, ptrdiff_t nodepos)
{
  struct multiset_data *msd = l->msd;
  union msnode *node;
  struct svalue ind;

  debug_malloc_touch (l);
  debug_malloc_touch (msd);
  check_msnode (l, nodepos, 1);

  node = OFF2MSNODE (msd, nodepos);

  if (TYPEOF(node->i.ind) == T_DELETED)
    do {
      node = DELETED_PREV (node);
      if (!node) {
	node = low_multiset_first (msd);
	return node ? MSNODE2OFF (msd, node) : -1;
      }
    } while (TYPEOF(node->i.ind) == T_DELETED);

  node = low_multiset_next (node);

  while (node && IS_DESTRUCTED (low_use_multiset_index (node, ind))) {
    union msnode *next = low_multiset_next (node);
    if (msd->refs == 1) {
      ONERROR uwp;
      nodepos = next ? MSNODE2OFF (msd, next) : -1;
      add_msnode_ref (l);
      SET_ONERROR (uwp, do_sub_msnode_ref, l);
      multiset_delete_node (l, MSNODE2OFF (msd, node));
      UNSET_ONERROR (uwp);
      msd = l->msd;
      node = nodepos >= 0 ? OFF2MSNODE (msd, nodepos) : NULL;
    }
    else
      node = next;
  }

  return node ? MSNODE2OFF (msd, node) : -1;
}

static enum find_types low_multiset_track_eq (
  struct multiset_data *msd, struct svalue *key, struct rbstack_ptr *track)
{
  struct rb_node_hdr *node = HDR (msd->root);
  struct rbstack_ptr rbstack = *track;
  enum find_types find_type;
  ONERROR uwp;

  /* Note: Similar code in multiset_find_eq. */

#ifdef PIKE_DEBUG
  /* Allow zero refs too since that's used during initial building. */
  if (msd->refs == 1) Pike_fatal ("Copy-on-write assumed here.\n");
  if (!msd->root) Pike_fatal ("Tree assumed to not be empty here.\n");
#endif

  SET_ONERROR (uwp, rbstack_do_free, track);

  if (TYPEOF(msd->cmp_less) == T_INT) {
    struct svalue tmp;
    if (!(msd->ind_types & (BIT_OBJECT | BIT_FUNCTION))) {
      /* Can assume an internal order which defines a total order for
       * all values. */
      LOW_RB_TRACK (
	rbstack, node,
	{
	  low_use_multiset_index (RBNODE (node), tmp);
	  assert (!IS_DESTRUCTED (&tmp));
	  /* TODO: Use special variant of set_svalue_cmpfun so we don't
	   * have to copy the index svalues. */
	  INTERNAL_CMP (key, &tmp, cmp_res);
	  assert (cmp_res != CMPFUN_UNORDERED);
	},
	{find_type = FIND_LESS; goto done;},
	{find_type = FIND_EQUAL; goto done;},
	{find_type = FIND_GREATER; goto done;});
      UNREACHABLE();
    }

    else {
      /* Find the biggest node less or order-wise equal to key. */
      struct rb_node_hdr *found_node;
      int step_count;

      LOW_RB_TRACK_NEQ (
	rbstack, node,
	{
	  low_use_multiset_index (RBNODE (node), tmp);
	  if (IS_DESTRUCTED (&tmp)) {
	    UNSET_ONERROR (uwp);
	    *track = rbstack;
	    return FIND_DESTRUCTED;
	  }
	  INTERNAL_CMP (key, &tmp, cmp_res);
	  if (!cmp_res) cmp_res = 1;
	  /* Note: CMPFUN_UNORDERED > 0 - treated as equal. */
	}, {
	  find_type = FIND_LESS;
	  found_node = node;
	  step_count = 0;
	}, {
	  find_type = FIND_GREATER;
	  found_node = node;
	  node = node->prev;
	  step_count = 1;
	});

      if (IS_DESTRUCTED (key)) {
	/* An extra check for a destructed key before entering the loop below,
	 * lest we might go backwards through the whole multiset. */
	RBSTACK_FREE (rbstack);
	UNSET_ONERROR (uwp);
	*track = rbstack;
	return FIND_KEY_DESTRUCTED;
      }

      /* Step backwards until a less or really equal node is found. */
      while (1) {
	int cmp_res;
	if (!node) goto done;
	low_use_multiset_index (RBNODE (node), tmp);
	if (IS_DESTRUCTED (&tmp)) {find_type = FIND_DESTRUCTED; break;}
	if (is_eq (&tmp, key)) {find_type = FIND_EQUAL; break;}
	INTERNAL_CMP (&tmp, key, cmp_res);
	/* Note: CMPFUN_UNORDERED > 0 - treated as equal. */
	if (cmp_res < 0) goto done;
	node = rb_prev (node);
	step_count++;
      }

      /* A node was found during stepping. Adjust rbstack. */
      while (step_count--) LOW_RB_TRACK_PREV (rbstack, found_node);
#ifdef PIKE_DEBUG
      if (node != RBSTACK_PEEK (rbstack)) Pike_fatal ("Stack stepping failed.\n");
#endif
    }
  }

  else {
    /* Find the biggest node less or order-wise equal to key. */
    struct rb_node_hdr *found_node;
    int step_count;

    LOW_RB_TRACK_NEQ (
      rbstack, node,
      {
	push_svalue (key);
	low_push_multiset_index (RBNODE (node));
	if (IS_DESTRUCTED (Pike_sp - 1)) {
	  pop_2_elems();
	  UNSET_ONERROR (uwp);
	  *track = rbstack;
	  return FIND_DESTRUCTED;
	}
	EXTERNAL_CMP (&msd->cmp_less);
	cmp_res = UNSAFE_IS_ZERO (Pike_sp - 1) ? 1 : -1;
	pop_stack();
      }, {
	find_type = FIND_LESS;
	found_node = node;
	step_count = 0;
      }, {
	find_type = FIND_GREATER;
	found_node = node;
	node = node->prev;
	step_count = 1;
      });

    if (IS_DESTRUCTED (key)) {
      /* An extra check for a destructed key before entering the loop below,
       * lest we might go backwards through the whole multiset. */
      RBSTACK_FREE (rbstack);
      UNSET_ONERROR (uwp);
      *track = rbstack;
      return FIND_KEY_DESTRUCTED;
    }

    /* Step backwards until a less or really equal node is found. */
    while (1) {
      if (!node) goto done;
      low_push_multiset_index (RBNODE (node));
      if (IS_DESTRUCTED (Pike_sp - 1))
	{pop_stack(); find_type = FIND_DESTRUCTED; break;}
      if (is_eq (Pike_sp - 1, key))
	{pop_stack(); find_type = FIND_EQUAL; break;}
      push_svalue (key);
      EXTERNAL_CMP (&msd->cmp_less);
      if (!UNSAFE_IS_ZERO (Pike_sp - 1)) {pop_stack(); goto done;}
      pop_stack();
      node = rb_prev (node);
      step_count++;
    }

    /* A node was found during stepping. Adjust rbstack. */
    while (step_count--) LOW_RB_TRACK_PREV (rbstack, found_node);
#ifdef PIKE_DEBUG
    if (node != RBSTACK_PEEK (rbstack)) Pike_fatal ("Stack stepping failed.\n");
#endif
  }

done:
  UNSET_ONERROR (uwp);
  if (IS_DESTRUCTED (key) && find_type != FIND_DESTRUCTED) {
    RBSTACK_FREE (rbstack);
    *track = rbstack;
    return FIND_KEY_DESTRUCTED;
  }
  *track = rbstack;
  return find_type;
}

static enum find_types low_multiset_track_le_gt (
  struct multiset_data *msd, struct svalue *key, struct rbstack_ptr *track)
{
  struct rb_node_hdr *node = HDR (msd->root);
  struct rbstack_ptr rbstack = *track;
  enum find_types find_type;
  ONERROR uwp;

  /* Note: Similar code in low_multiset_find_le_gt. */

#ifdef PIKE_DEBUG
  /* Allow zero refs too since that's used during initial building. */
  if (msd->refs == 1) Pike_fatal ("Copy-on-write assumed here.\n");
  if (!msd->root) Pike_fatal ("Tree assumed to not be empty here.\n");
#endif

  SET_ONERROR (uwp, rbstack_do_free, track);

  if (TYPEOF(msd->cmp_less) == T_INT) {
    struct svalue tmp;
    LOW_RB_TRACK_NEQ (
      rbstack, node,
      {
	low_use_multiset_index (RBNODE (node), tmp);
	if (IS_DESTRUCTED (&tmp)) {
	  UNSET_ONERROR (uwp);
	  *track = rbstack;
	  return FIND_DESTRUCTED;
	}
	/* TODO: Use special variant of set_svalue_cmpfun so we don't
	 * have to copy the index svalues. */
	INTERNAL_CMP (key, low_use_multiset_index (RBNODE (node), tmp),
		      cmp_res);
	/* Note: CMPFUN_UNORDERED > 0 - treated as equal. */
	cmp_res = cmp_res >= 0 ? 1 : -1;
      },
      {find_type = FIND_LESS; goto done;},
      {find_type = FIND_GREATER; goto done;});
    UNREACHABLE();
  }

  else {
    LOW_RB_TRACK_NEQ (
      rbstack, node,
      {
	push_svalue (key);
	low_push_multiset_index (RBNODE (node));
	if (IS_DESTRUCTED (Pike_sp - 1)) {
	  pop_2_elems();
	  UNSET_ONERROR (uwp);
	  *track = rbstack;
	  return FIND_DESTRUCTED;
	}
	EXTERNAL_CMP (&msd->cmp_less);
	cmp_res = UNSAFE_IS_ZERO (Pike_sp - 1) ? 1 : -1;
	pop_stack();
      },
      {find_type = FIND_LESS; goto done;},
      {find_type = FIND_GREATER; goto done;});
    UNREACHABLE();
  }

done:
  UNSET_ONERROR (uwp);
  if (IS_DESTRUCTED (key)) {
    RBSTACK_FREE (rbstack);
    *track = rbstack;
    return FIND_KEY_DESTRUCTED;
  }
  *track = rbstack;
  return find_type;
}

PMOD_EXPORT void multiset_fix_type_field (struct multiset *l)
{
  struct multiset_data *msd = l->msd;
  union msnode *node;
  TYPE_FIELD ind_types = 0, val_types = 0;

  debug_malloc_touch (l);
  debug_malloc_touch (msd);

  if ((node = low_multiset_first (msd))) {
#define WITH_NODES_BLOCK(TYPE, IND)					\
    IND (val_types = BIT_INT);						\
    do {								\
      ind_types |= 1 << (TYPEOF(node->i.ind) & ~MULTISET_FLAG_MASK);	\
    } while ((node = low_multiset_next (node)));

    DO_WITH_NODES (msd);

#undef WITH_NODES_BLOCK
  }
  else
    val_types = BIT_INT;

#ifdef PIKE_DEBUG
  if (ind_types & ~msd->ind_types)
    Pike_fatal ("Multiset indices type field lacks 0x%x.\n", ind_types & ~msd->ind_types);
  if (val_types & ~msd->val_types)
    Pike_fatal ("Multiset values type field lacks 0x%x.\n", val_types & ~msd->val_types);
#endif

  msd->ind_types = ind_types;
  msd->val_types = val_types;
}

#ifdef PIKE_DEBUG
static void check_multiset_type_fields (struct multiset *l)
{
  struct multiset_data *msd = l->msd;
  union msnode *node;
  TYPE_FIELD ind_types = 0, val_types = 0;

  if ((node = low_multiset_first (msd))) {
#define WITH_NODES_BLOCK(TYPE, IND)					\
    IND (val_types = BIT_INT);						\
    do {								\
      ind_types |= 1 << (TYPEOF(node->i.ind) & ~MULTISET_FLAG_MASK);	\
    } while ((node = low_multiset_next (node)));

    DO_WITH_NODES (msd);

#undef WITH_NODES_BLOCK
  }
  else
    val_types = BIT_INT;

  if (ind_types & ~msd->ind_types)
    Pike_fatal ("Multiset indices type field lacks 0x%x.\n", ind_types & ~msd->ind_types);
  if (val_types & ~msd->val_types)
    Pike_fatal ("Multiset values type field lacks 0x%x.\n", val_types & ~msd->val_types);
}
#endif

#define ADD_NODE(MSD, RBSTACK, NEW, IND, FIND_TYPE) do {		\
    assign_svalue_no_free (&NEW->i.ind, IND);				\
    MSD->ind_types |= 1 << TYPEOF(*(IND));				\
    DO_IF_DEBUG (SET_SVAL_TYPE(NEW->i.ind,				\
			       TYPEOF(NEW->i.ind) |			\
			       MULTISET_FLAG_MARKER));			\
    switch (FIND_TYPE) {						\
      case FIND_LESS:							\
	low_rb_link_at_next (PHDR (&MSD->root), RBSTACK, HDR (NEW));	\
	break;								\
      case FIND_GREATER:						\
	low_rb_link_at_prev (PHDR (&MSD->root), RBSTACK, HDR (NEW));	\
	break;								\
      case FIND_NOROOT:							\
	MSD->root = NEW;						\
	low_rb_init_root (HDR (NEW));					\
	break;								\
      default: DO_IF_DEBUG (Pike_fatal ("Invalid find_type.\n"));	\
    }									\
  } while (0)

/* val may be zero. If the multiset has values, the integer 1 will be
 * used as value in that case. val is ignored if the multiset has no
 * values. The value of an existing entry will be replaced iff replace
 * is nonzero (done under the assumption the caller has one value
 * lock), otherwise nothing will be done in that case. Nothing is
 * done if ind is destructed. */
static ptrdiff_t multiset_insert_2 (struct multiset *l,
				    struct svalue *ind)
{
  struct multiset_data *msd = l->msd;
  union msnode *new;
  enum find_types find_type;
  ONERROR uwp;
  RBSTACK_INIT (rbstack);

  /* Note: Similar code in multiset_add, multiset_add_after,
   * multiset_delete_2 and multiset_delete_node. */

#ifdef PIKE_DEBUG
  debug_malloc_touch (l);
  debug_malloc_touch (msd);
  check_svalue (ind);
#endif

  SET_ONERROR (uwp, free_indirect_multiset_data, &msd);

  while (1) {
    if (!msd->root) {
      if (IS_DESTRUCTED (ind)) {
	UNSET_ONERROR (uwp);
	return -1;
      }
      if (prepare_for_add (l, l->node_refs)) msd = l->msd;
      ALLOC_MSNODE (msd, l->node_refs, new);
      find_type = FIND_NOROOT;
      goto insert;
    }

    if (!msd->free_list && !l->node_refs && msd->refs == 1) {
      /* Enlarge now if possible, anticipating there will be an
       * insert. Otherwise we either have to redo the search or don't
       * use a rebalancing resize. */
#ifdef PIKE_DEBUG
      if (d_flag > 1) check_multiset (l, 1);
#endif
      l->msd = resize_multiset_data (msd, ENLARGE_SIZE (msd->allocsize), 0);
      msd = l->msd;
    }

    add_ref (msd);
    find_type = low_multiset_track_eq (msd, ind, &rbstack);

    if (l->msd != msd) {
      RBSTACK_FREE (rbstack);
      if (!sub_ref (msd)) free_multiset_data (msd);
      msd = l->msd;
    }

    else
      switch (find_type) {
	case FIND_LESS:
	case FIND_GREATER:
	  sub_extra_ref (msd);
	  if (prepare_for_add (l, 1)) {
	    rbstack_shift (rbstack, HDR (msd->nodes), HDR (l->msd->nodes));
	    msd = l->msd;
	  }
	  ALLOC_MSNODE (msd, l->node_refs, new);
	  goto insert;

	case FIND_EQUAL: {
	  struct rb_node_hdr *node = RBSTACK_PEEK (rbstack);
	  RBSTACK_FREE (rbstack);
	  UNSET_ONERROR (uwp);
	  sub_extra_ref (msd);
	  return MSNODE2OFF (msd, RBNODE (node));
	}

	case FIND_DESTRUCTED:
	  sub_extra_ref (msd);
	  midflight_remove_node_fast (l, &rbstack, 0);
	  msd = l->msd;
	  break;

	case FIND_KEY_DESTRUCTED:
	  sub_extra_ref (msd);
	  UNSET_ONERROR (uwp);
	  return -1;

	default: DO_IF_DEBUG (Pike_fatal ("Invalid find_type.\n"));
      }
  }

insert:
  UNSET_ONERROR (uwp);
  ADD_NODE (msd, rbstack, new, ind, find_type);
  return MSNODE2OFF (msd, new);
}

PMOD_EXPORT void multiset_insert (struct multiset *l,
				  struct svalue *ind)
{
  debug_malloc_touch (l);
  debug_malloc_touch (l->msd);
  dmalloc_touch_svalue (ind);
  multiset_insert_2 (l, ind);
}

/**
 * Add an entry to a multiset.
 * Nothing is done if ind is destructed.
 *
 * Returns -1 if ind is destructed.
 * Otherwise returns the node offset in the multiset for the
 * new node.
 */
PMOD_EXPORT ptrdiff_t multiset_add (struct multiset *l, struct svalue *ind)
{
  struct multiset_data *msd = l->msd;
  union msnode *new;
  enum find_types find_type;
  ONERROR uwp;
  RBSTACK_INIT (rbstack);

  /* Note: Similar code in multiset_insert_2, multiset_add_after,
   * multiset_delete_2 and multiset_delete_node. */

#ifdef PIKE_DEBUG
  debug_malloc_touch (l);
  debug_malloc_touch (msd);
  check_svalue (ind);
#endif

  SET_ONERROR (uwp, free_indirect_multiset_data, &msd);

  while (1) {
    if (!msd->root) {
      if (IS_DESTRUCTED (ind)) {
	UNSET_ONERROR (uwp);
	return -1;
      }
      if (prepare_for_add (l, l->node_refs)) msd = l->msd;
      ALLOC_MSNODE (msd, l->node_refs, new);
      find_type = FIND_NOROOT;
      goto add;
    }

    if (!msd->free_list && !l->node_refs) {
      /* Enlarge now if possible. Otherwise we either have to redo the
       * search or don't use a rebalancing resize. */
      if (msd->refs > 1) {
	l->msd = copy_multiset_data (msd);
	MOVE_MSD_REF (l, msd);
      }
#ifdef PIKE_DEBUG
      if (d_flag > 1) check_multiset (l, 1);
#endif
      l->msd = resize_multiset_data (msd, ENLARGE_SIZE (msd->allocsize), 0);
      msd = l->msd;
    }
#if 0
    else
      if (msd->size == msd->allocsize)
	fputs ("Can't rebalance multiset tree in multiset_add\n", stderr);
#endif

    add_ref (msd);
    find_type = low_multiset_track_le_gt (msd, ind, &rbstack);

    if (l->msd != msd) {
      RBSTACK_FREE (rbstack);
      if (!sub_ref (msd)) free_multiset_data (msd);
      msd = l->msd;
    }

    else
      switch (find_type) {
	case FIND_LESS:
	case FIND_GREATER:
	  sub_extra_ref (msd);
	  if (prepare_for_add (l, 1)) {
	    rbstack_shift (rbstack, HDR (msd->nodes), HDR (l->msd->nodes));
            msd = l->msd;
	  }
	  ALLOC_MSNODE (msd, l->node_refs, new);
	  goto add;

	case FIND_DESTRUCTED:
	  sub_extra_ref (msd);
	  midflight_remove_node_fast (l, &rbstack, 0);
	  msd = l->msd;
	  break;

	case FIND_KEY_DESTRUCTED:
	  sub_extra_ref (msd);
	  UNSET_ONERROR (uwp);
	  return -1;

	default: DO_IF_DEBUG (Pike_fatal ("Invalid find_type.\n"));
      }
  }

add:
  UNSET_ONERROR (uwp);
  ADD_NODE (msd, rbstack, new, ind, find_type);
  return MSNODE2OFF (msd, new);
}

#define TEST_LESS(MSD, A, B, CMP_RES) do {				\
    if (TYPEOF(MSD->cmp_less) == T_INT)					\
      INTERNAL_CMP (A, B, CMP_RES);					\
    else {								\
      push_svalue (A);							\
      push_svalue (B);							\
      EXTERNAL_CMP (&MSD->cmp_less);					\
      CMP_RES = UNSAFE_IS_ZERO (Pike_sp - 1) ? 1 : -1;			\
      pop_stack();							\
    }									\
  } while (0)


PMOD_EXPORT int multiset_delete (struct multiset *l,
				 struct svalue *ind)
{
  debug_malloc_touch (l);
  debug_malloc_touch (l->msd);
  dmalloc_touch_svalue (ind);
  return multiset_delete_2 (l, ind, NULL);
}

/* If removed_val isn't NULL, the value of the deleted node is stored
 * there, or the integer 1 if the multiset lacks values. The undefined
 * value is stored if no matching entry was found. */
PMOD_EXPORT int multiset_delete_2 (struct multiset *l,
				   struct svalue *ind,
				   struct svalue *removed_val)
{
  struct multiset_data *msd = l->msd;
  enum find_types find_type;
  ONERROR uwp;
  RBSTACK_INIT (rbstack);

  /* Note: Similar code in multiset_insert_2, multiset_add,
   * multiset_add_after and multiset_delete_node. */

  debug_malloc_touch (l);
  debug_malloc_touch (msd);
  check_svalue (ind);

  SET_ONERROR (uwp, free_indirect_multiset_data, &msd);

  while (1) {
    if (!msd->root) goto not_found; /* No difference if ind is destructed. */

    add_ref (msd);
    find_type = low_multiset_track_eq (msd, ind, &rbstack);

    if (l->msd != msd) {
      RBSTACK_FREE (rbstack);
      if (!sub_ref (msd)) free_multiset_data (msd);
      msd = l->msd;
    }

    else
      switch (find_type) {
	case FIND_LESS:
	case FIND_GREATER:
	case FIND_KEY_DESTRUCTED:
	  RBSTACK_FREE (rbstack);
	  sub_extra_ref (msd);
	  goto not_found;

	case FIND_EQUAL: {
          struct svalue ind;
	  struct rb_node_hdr *node = RBSTACK_PEEK (rbstack);

	  UNSET_ONERROR (uwp);
	  sub_extra_ref (msd);

	  /* Postpone free since the msd might be copied in unlink_node. */
	  low_use_multiset_index (RBNODE (node), ind);

	  unlink_msnode (l, &rbstack, 0);

	  free_svalue (&ind);
	  if (removed_val)
	    SET_SVAL(*removed_val, T_INT, NUMBER_NUMBER, integer, 1);

	  return 1;
	}

	case FIND_DESTRUCTED:
	  sub_extra_ref (msd);
	  midflight_remove_node_fast (l, &rbstack, 0);
	  msd = l->msd;
	  break;

	default: DO_IF_DEBUG (Pike_fatal ("Invalid find_type.\n"));
      }
  }

not_found:
  UNSET_ONERROR (uwp);
  if (removed_val) {
    SET_SVAL(*removed_val, T_INT, NUMBER_UNDEFINED, integer, 0);
  }
  return 0;
}

/* Frees the node reference that nodepos represents. */
static void multiset_delete_node (struct multiset *l,
				       ptrdiff_t nodepos)
{
  struct multiset_data *msd = l->msd;
  enum find_types find_type;
  ONERROR uwp;
  RBSTACK_INIT (rbstack);	/* FIXME: Is this always freed properly? */

  /* Note: Similar code in multiset_insert_2, multiset_add,
   * multiset_add_after and multiset_delete_2. */

  debug_malloc_touch (l);
  debug_malloc_touch (msd);
  check_msnode (l, nodepos, 1);

  SET_ONERROR (uwp, free_indirect_multiset_data, &msd);

  while (1) {
    union msnode *existing = OFF2MSNODE (msd, nodepos);
    struct svalue ind;

    if (TYPEOF(existing->i.ind) == T_DELETED) {
      UNSET_ONERROR (uwp);
      sub_msnode_ref (l);
      return;
    }
    low_use_multiset_index (existing, ind);

    add_ref (msd);
    find_type = low_multiset_track_le_gt (msd, &ind, &rbstack);

    if (l->msd != msd) {
      RBSTACK_FREE (rbstack);
      if (!sub_ref (msd)) free_multiset_data (msd);
      msd = l->msd;
    }

    else if (find_type == FIND_DESTRUCTED) {
      sub_extra_ref (msd);
      midflight_remove_node_fast (l, &rbstack, 0);
      msd = l->msd;
    }

    else {
      struct rb_node_hdr *node = RBSTACK_PEEK (rbstack);

      /* Step backwards until the existing node is found. */
      while (RBNODE (node) != existing) {
	if (!node) {
	  /* The index is destructed, or things changed under our
	   * feet, or perhaps there are flaky compare functions. Build
	   * the stack solely from the rb_node structure instead. */
	  RBSTACK_FREE (rbstack);
	  low_rb_build_stack (HDR (msd->root), HDR (existing), &rbstack);
	  break;
	}
	LOW_RB_TRACK_PREV (rbstack, node);
      }

      UNSET_ONERROR (uwp);

      /* Postpone free since the msd might be copied in unlink_node. */
      sub_msnode_ref (l);
      assert (l->msd == msd);
      sub_extra_ref (msd);
      unlink_msnode (l, &rbstack, 0);

      free_svalue (&ind);

      return;
    }
  }
}

PMOD_EXPORT int multiset_member (struct multiset *l, struct svalue *key)
{
  debug_malloc_touch (l);
  debug_malloc_touch (l->msd);
  dmalloc_touch_svalue (key);
  return low_multiset_find_eq (l, key) ? 1 : 0;
}

/* No ref is added for the returned svalue. */
PMOD_EXPORT struct svalue *multiset_lookup (struct multiset *l,
					    struct svalue *key)
{
  union msnode *node;
  debug_malloc_touch (l);
  debug_malloc_touch (l->msd);
  check_svalue (key);
  if ((node = low_multiset_find_eq (l, key)))
    /* Caller better not try to change this. */
    return (struct svalue *) &svalue_int_one;
  else
    return NULL;
}
#define GET_RANGE_SIZE_AND_END(BEGPOS, ENDPOS, MSD, MSD_SIZE, RANGE_SIZE, END) do { \
    if (BEGPOS < 0 && ENDPOS < 0) {					\
      RANGE_SIZE = MSD_SIZE;						\
      END = low_multiset_last (MSD);					\
    }									\
									\
    else {								\
      union msnode *beg, *node;						\
									\
      if (BEGPOS < 0)							\
	beg = NULL;							\
      else {								\
	beg = OFF2MSNODE (MSD, BEGPOS);					\
	if (TYPEOF(beg->i.ind) == T_DELETED) {				\
	  do {								\
	    beg = DELETED_PREV (beg);					\
	  } while (beg && TYPEOF(beg->i.ind) == T_DELETED);		\
	  if (beg) beg = low_multiset_next (beg);			\
	}								\
      }									\
									\
      if (ENDPOS < 0) {							\
	END = low_multiset_last (MSD);					\
	RANGE_SIZE = 1;							\
      }									\
      else {								\
	END = OFF2MSNODE (MSD, ENDPOS);					\
	if (TYPEOF(END->i.ind) == T_DELETED) {				\
	  do {								\
	    END = DELETED_NEXT (END);					\
	  } while (END && TYPEOF(END->i.ind) == T_DELETED);		\
	  if (END) END = low_multiset_prev (END);			\
	  else END = low_multiset_last (MSD);				\
	}								\
	RANGE_SIZE = beg ? 1 : 0;					\
      }									\
									\
      for (node = END; node != beg; node = low_multiset_prev (node)) {	\
	if (!node) {							\
	  RANGE_SIZE = 0;						\
	  break;							\
	}								\
	RANGE_SIZE++;							\
      }									\
    }									\
  } while (0)

/* The range is inclusive. begpos and/or endpos may be -1 to go to the
 * limit in that direction. If begpos points to a deleted node then
 * the next nondeleted node is used instead, which is found in the
 * same way as multiset_next. Vice versa for endpos. If the
 * beginning is after the end then the empty array is returned. */
static struct array *multiset_range_indices (struct multiset *l,
				      ptrdiff_t begpos, ptrdiff_t endpos)
{
  struct multiset_data *msd;
  struct array *indices;
  union msnode *end;
  int msd_size, range_size;

#ifdef PIKE_DEBUG
  debug_malloc_touch (l);
  debug_malloc_touch (l->msd);
  if (begpos >= 0) check_msnode (l, begpos, 1);
  if (endpos >= 0) check_msnode (l, endpos, 1);
#endif

  check_multiset_for_destruct (l);
  msd = l->msd;
  msd_size = multiset_sizeof (l);

  GET_RANGE_SIZE_AND_END (begpos, endpos, msd, msd_size,
			  range_size, end);

  if (range_size) {
    TYPE_FIELD types;
    indices = allocate_array_no_init (1, range_size);
    indices->size = range_size;
    if (range_size == msd_size) {
      types = msd->ind_types;
      while (1) {
	low_assign_multiset_index_no_free (&ITEM (indices)[--range_size], end);
	if (!range_size) break;
	end = low_multiset_prev (end);
      }
    }
    else {
      types = 0;
      while (1) {
	low_assign_multiset_index_no_free (&ITEM (indices)[--range_size], end);
	types |= 1 << TYPEOF(ITEM (indices)[range_size]);
	if (!range_size) break;
	end = low_multiset_prev (end);
      }
    }
    indices->type_field = types;
  }
  else add_ref (indices = &empty_array);

  return indices;
}

/* The range is inclusive. begpos and/or endpos may be -1 to go to the
 * limit in that direction. If begpos points to a deleted node then
 * the next nondeleted node is used instead, which is found in the
 * same way as multiset_next. Vice versa for endpos. If the
 * beginning is after the end then the empty array is returned. */
static struct array *multiset_range_values (struct multiset *l,
				     ptrdiff_t begpos, ptrdiff_t endpos)
{
  struct multiset_data *msd;
  struct array *values;
  union msnode *end;
  int msd_size, range_size;

#ifdef PIKE_DEBUG
  debug_malloc_touch (l);
  debug_malloc_touch (l->msd);
  if (begpos >= 0) check_msnode (l, begpos, 1);
  if (endpos >= 0) check_msnode (l, endpos, 1);
#endif

  check_multiset_for_destruct (l);
  msd = l->msd;
  msd_size = multiset_sizeof (l);

  GET_RANGE_SIZE_AND_END (begpos, endpos, msd, msd_size,
			  range_size, end);

  if (range_size) {
    values = allocate_array_no_init (1, range_size);
    values->size = range_size;
    while(range_size--) {
      SET_SVAL(ITEM (values)[range_size], T_INT, NUMBER_NUMBER,
	       integer, 1);
    }
    values->type_field = BIT_INT;
  }
  else add_ref (values = &empty_array);

  return values;
}


struct array *multiset_indices (struct multiset *l)
{
  debug_malloc_touch (l);
  debug_malloc_touch (l->msd);
  return multiset_range_indices (l, -1, -1);
}

struct array *multiset_values (struct multiset *l)
{
  debug_malloc_touch (l);
  debug_malloc_touch (l->msd);
  return multiset_range_values (l, -1, -1);
}

/* Eliminates all pointers to destructed objects. If an index is such
 * a pointer then the node is removed. */
PMOD_EXPORT void check_multiset_for_destruct (struct multiset *l)
{
  struct multiset_data *msd = l->msd;

#ifdef PIKE_DEBUG
  debug_malloc_touch (l);
  debug_malloc_touch (msd);
  if (Pike_in_gc > GC_PASS_PREPARE && Pike_in_gc < GC_PASS_FREE)
    Pike_fatal("check_multiset_for_destruct called in invalid pass inside gc.\n");
#endif

  if (msd->root &&
      ((msd->ind_types | msd->val_types) & (BIT_OBJECT | BIT_FUNCTION))) {
    struct rb_node_hdr *node = HDR (msd->root);
    struct svalue ind;
    TYPE_FIELD ind_types = 0, val_types = 0;
    RBSTACK_INIT (rbstack);
    LOW_RB_TRACK_FIRST (rbstack, node);

#define WITH_NODES_BLOCK(TYPE, IND)					\
    IND (val_types = BIT_INT);						\
    do {								\
      low_use_multiset_index (RBNODE (node), ind);			\
      if (IS_DESTRUCTED (&ind)) {					\
	midflight_remove_node_fast (l, &rbstack, 1);			\
	msd = l->msd;							\
	node = RBSTACK_PEEK (rbstack);					\
      }									\
      else {								\
	ind_types |= 1 << TYPEOF(ind);					\
	LOW_RB_TRACK_NEXT (rbstack, node);				\
      }									\
    } while (node);

    DO_WITH_NODES (msd);

#undef WITH_NODES_BLOCK

#ifdef PIKE_DEBUG
    if (ind_types & ~msd->ind_types)
      Pike_fatal ("Multiset indices type field lacks 0x%x.\n", ind_types & ~msd->ind_types);
    if (val_types & ~msd->val_types)
      Pike_fatal ("Multiset values type field lacks 0x%x.\n", val_types & ~msd->val_types);
#endif

    msd->ind_types = ind_types;
    msd->val_types = val_types;
  }
}

PMOD_EXPORT struct multiset *copy_multiset (struct multiset *l)
{
  struct multiset_data *msd = l->msd;
  debug_malloc_touch (l);
  debug_malloc_touch (msd);
  l = ba_alloc(&multiset_allocator);
  INIT_MULTISET (l);
  add_ref (l->msd = msd);
  return l;
}

/* Returns NULL if no special case is applicable. */
static struct multiset *merge_special (struct multiset *a,
				       struct multiset *b,
				       int operation)
{
  ONERROR uwp;
  int op;

  debug_malloc_touch (a);
  debug_malloc_touch (a->msd);
  debug_malloc_touch (b);
  debug_malloc_touch (b->msd);

#define COPY_MSD_AND_KEEP_FLAGS(FROM, TO) do {				\
    struct multiset_data *oldmsd = (TO)->msd;				\
    SET_ONERROR (uwp, free_indirect_multiset_data, &oldmsd);		\
    add_ref ((TO)->msd = (FROM)->msd);					\
    multiset_set_flags ((TO), oldmsd->flags);				\
    multiset_set_cmp_less ((TO), &oldmsd->cmp_less);			\
    UNSET_ONERROR (uwp);						\
    if (!sub_ref (oldmsd)) free_multiset_data (oldmsd);			\
  } while (0)

#define EMPTY_MSD_AND_KEEP_FLAGS(L) do {				\
    struct multiset_data *oldmsd = (L)->msd;				\
    add_ref ((L)->msd = low_alloc_multiset_data (0, oldmsd->flags));	\
    assign_svalue_no_free (&(L)->msd->cmp_less, &oldmsd->cmp_less);	\
    if (!sub_ref (oldmsd)) free_multiset_data (oldmsd);			\
  } while (0)

#define ALLOC_COPY_AND_SET_FLAGS(FROM, RES, FLAGSRC) do {		\
    (RES) = copy_multiset (FROM);					\
    SET_ONERROR (uwp, do_free_multiset, (RES));				\
    multiset_set_flags ((RES), (FLAGSRC)->msd->flags);			\
    multiset_set_cmp_less ((RES), &(FLAGSRC)->msd->cmp_less);		\
    UNSET_ONERROR (uwp);						\
  } while (0)

#define ALLOC_EMPTY_AND_SET_FLAGS(RES, FLAGSRC) do {			\
    (RES) = allocate_multiset (0, (FLAGSRC)->msd->flags,		\
			       &(FLAGSRC)->msd->cmp_less);		\
  } while (0)

  if (!a->msd->root)
    if (operation & PIKE_ARRAY_OP_B) /* Result is b. */
      if (operation & PIKE_MERGE_DESTR_A) {
	if (a->node_refs) return NULL;
	COPY_MSD_AND_KEEP_FLAGS (b, a);
	return a;
      }
      else {
	struct multiset *res;
	ALLOC_COPY_AND_SET_FLAGS (b, res, a);
	return res;
      }
    else			/* Result is empty. */
      if (operation & PIKE_MERGE_DESTR_A)
	return a;
      else {
	struct multiset *res;
	ALLOC_EMPTY_AND_SET_FLAGS (res, a);
	return res;
      }

  else if (!b->msd->root)
    if (operation & (PIKE_ARRAY_OP_A << 8)) /* Result is a. */
      if (operation & PIKE_MERGE_DESTR_A)
	return a;
      else {
	struct multiset *res;
	ALLOC_COPY_AND_SET_FLAGS (a, res, a);
	return res;
      }
    else			/* Result is empty. */
      if (operation & PIKE_MERGE_DESTR_A) {
	if (a->node_refs) return NULL;
	EMPTY_MSD_AND_KEEP_FLAGS (a);
	return a;
      }
      else {
	struct multiset *res;
	ALLOC_EMPTY_AND_SET_FLAGS (res, a);
	return res;
      }

  else if (a == b) {
    op = operation & ((PIKE_ARRAY_OP_A|PIKE_ARRAY_OP_B) << 4);
    if (op) {
      if (op != ((PIKE_ARRAY_OP_A|PIKE_ARRAY_OP_B) << 4)) { /* Result is a (or b). */
	if (operation & PIKE_MERGE_DESTR_A)
	  return a;
	else {
	  struct multiset *res;
	  ALLOC_COPY_AND_SET_FLAGS (a, res, a);
	  return res;
	}
      }
    }
    else			/* Result is empty. */
      if (operation & PIKE_MERGE_DESTR_A) {
	if (a->node_refs) return NULL;
	EMPTY_MSD_AND_KEEP_FLAGS (a);
	return a;
      }
      else {
	struct multiset *res;
	ALLOC_EMPTY_AND_SET_FLAGS (res, a);
	return res;
      }
  }

  return NULL;
}

struct merge_data
{
  struct multiset *a, *b, *res, *tmp;
  struct recovery_data rd;
  struct rb_node_hdr *a_node, *b_node, *res_list;
  size_t res_length;
};

static void cleanup_merge_data (struct merge_data *m)
{
  debug_malloc_touch (m->a);
  debug_malloc_touch (m->a ? m->a->msd : NULL);
  debug_malloc_touch (m->b);
  debug_malloc_touch (m->b ? m->b->msd : NULL);
  debug_malloc_touch (m->res);
  debug_malloc_touch (m->res ? m->res->msd : NULL);
  debug_malloc_touch (m->tmp);
  debug_malloc_touch (m->tmp ? m->tmp->msd : NULL);

  if (m->res_list) {
    /* The result msd contains a list and possibly a part of a tree.
     * Knit it together to a tree again. Knowledge that LOW_RB_MERGE
     * traverses the trees backwards is used here. */
    struct rb_node_hdr *list = m->res_list;
    size_t length = m->res_length;
    if (m->res == m->a) {
      struct rb_node_hdr *node;
      for (node = m->a_node; node; node = rb_prev (node)) {
	node->next = list, list = node;
	length++;
      }
    }
    m->res->msd->root = RBNODE (rb_make_tree (list, length));
  }

  sub_msnode_ref (m->res);
  if (m->res != m->a) free_multiset (m->res);
  if (m->tmp) free_multiset (m->tmp);
  free_recovery_data (&m->rd);
}

static void merge_shift_ptrs (struct multiset_data *old, struct multiset_data *new,
			      struct merge_data *m)
{
  if (m->a == m->res && m->a_node) m->a_node = SHIFT_HDRPTR (m->a_node, old, new);
  if (m->res_list) m->res_list = SHIFT_HDRPTR (m->res_list, old, new);
}

/* If PIKE_MERGE_DESTR_A is used, a is returned without ref increase.
 * Else the new multiset will inherit flags and cmp_less from a.
 *
 * If destructive on an operand and there is an exception, then some
 * random part(s) of the operand will be left unprocessed. All entries
 * that were in the operand and would remain in the finished result
 * will still be there, and no entries from the other operand that
 * wouldn't be in the finished result. */
PMOD_EXPORT struct multiset *merge_multisets (struct multiset *a,
					      struct multiset *b,
					      int operation)
{
  struct merge_data m;
  int got_node_refs;
  TYPE_FIELD ind_types, val_types;
  ONERROR uwp;

  debug_malloc_touch (a);
  debug_malloc_touch (a->msd);
  debug_malloc_touch (b);
  debug_malloc_touch (b->msd);

#ifdef PIKE_DEBUG
  if (operation & PIKE_MERGE_DESTR_B)
    Pike_fatal ("Destructiveness on second operand not supported.\n");
#endif

#if 1
  if (!a->msd->root || !b->msd->root || a == b) {
    struct multiset *res = merge_special (a, b, operation);
    if (res) return res;
  }
#endif

  m.tmp = NULL;
  m.res_list = NULL;
  m.res_length = 0;

  /* Preparations. Set m.res and make sure the operands have the same
   * form. This can do up to three multiset copies that could be
   * optimized away, but that'd lead to quite a bit of extra code and
   * those situations are so unusual it's not worth bothering
   * about. */
  if (operation & PIKE_MERGE_DESTR_A) {
#ifdef PIKE_DEBUG
    if (a->refs != 1)
      Pike_fatal ("Not safe to do destructive merge with several refs to the operand.\n");
#endif
    m.res = m.a = a;
    if (a == b)
      /* Can't handle the result being the same as both a and b. */
      m.b = m.tmp = copy_multiset (b);
    else m.b = b;
    /* Can't handle a shared data block even though there might be no
     * change in it, since the merge always relinks the tree. */
    prepare_for_change (m.res, got_node_refs = m.res->node_refs);
  }
  else {
    int newsize;
    if (operation & (PIKE_ARRAY_OP_A << 8)) newsize = multiset_sizeof (a);
    else newsize = 0;
    if (operation & PIKE_ARRAY_OP_B) newsize += multiset_sizeof (b);
    m.res = allocate_multiset (newsize, a->msd->flags, &a->msd->cmp_less);
    m.a = a, m.b = b;
    got_node_refs = 0;
  }
  if (!SAME_CMP_LESS (a->msd, b->msd)) {
    if (!m.tmp) m.b = m.tmp = copy_multiset (b);
    multiset_set_cmp_less (m.b, &a->msd->cmp_less);
  }

  ind_types = val_types = 0;
  if (m.res == a) m.rd.a_msd = NULL;
  else add_ref (m.rd.a_msd = m.a->msd);
  add_ref (m.rd.b_msd = m.b->msd);
  add_msnode_ref (m.res);
  SET_ONERROR (uwp, cleanup_merge_data, &m);

#define ALLOC_RES_NODE(RES, RES_MSD, NEW_NODE)				\
  do {									\
    union msnode *node;							\
    if (prepare_for_add (RES, 1)) {					\
      merge_shift_ptrs (RES_MSD, (RES)->msd, &m);			\
      (RES_MSD) = (RES)->msd;						\
    }									\
    ALLOC_MSNODE (RES_MSD, (RES)->node_refs, node);			\
    NEW_NODE = HDR (node);						\
  } while (0)

#define UNLINK_RES_NODE(RES_MSD, RES_LIST, GOT_NODE_REFS, NODE)		\
  do {									\
    if (GOT_NODE_REFS) {						\
      ADD_TO_FREE_LIST (RES_MSD, RBNODE (NODE));			\
      SET_SVAL_TYPE(RBNODE (NODE)->i.ind, T_DELETED);			\
      /* Knowledge that LOW_RB_MERGE traverses the trees backwards */	\
      /* is used here. */						\
      RBNODE (NODE)->i.ind.u.ptr = RES_LIST;				\
      RBNODE (NODE)->i.prev =						\
	(struct msnode_ind *) low_multiset_prev (RBNODE (NODE));	\
    }									\
    else {								\
      CLEAR_DELETED_ON_FREE_LIST (RES_MSD);				\
      ADD_TO_FREE_LIST (RES_MSD, RBNODE (NODE));			\
      SET_SVAL_TYPE(RBNODE (NODE)->i.ind, PIKE_T_UNKNOWN);		\
      RBNODE (NODE)->i.prev = NULL;					\
      RES_MSD->size--;							\
    }									\
  } while (0)

  if (TYPEOF(m.res->msd->cmp_less) == T_INT) {
    struct multiset_data *res_msd = m.res->msd;
    struct svalue a_ind, b_ind;
    m.a_node = HDR (m.a->msd->root), m.b_node = HDR (m.rd.b_msd->root);

    if (m.rd.a_msd)		/* Not destructive on a. */
      LOW_RB_MERGE (
	ic_nd, m.a_node, m.b_node,
	m.res_list, m.res_length, operation,

	{
	  low_use_multiset_index (RBNODE (m.a_node), a_ind);
	  if (IS_DESTRUCTED (&a_ind)) goto ic_nd_free_a;
	}, {
	  low_use_multiset_index (RBNODE (m.b_node), b_ind);
	  if (IS_DESTRUCTED (&b_ind)) goto ic_nd_free_b;
	},

	{
	  INTERNAL_CMP (&a_ind, &b_ind, cmp_res);
	},

	{			/* Copy m.a_node. */
	  ALLOC_RES_NODE (m.res, res_msd, new_node);
	  assign_svalue_no_free (&RBNODE (new_node)->i.ind, &a_ind);
	  ind_types |= 1 << TYPEOF(a_ind);
	  DO_IF_DEBUG (SET_SVAL_TYPE(RBNODE (new_node)->i.ind,
				     TYPEOF(RBNODE (new_node)->i.ind) |
				     MULTISET_FLAG_MARKER));
	}, {			/* Free m.a_node. */
	},

	{			/* Copy m.b_node. */
	  ALLOC_RES_NODE (m.res, res_msd, new_node);
	  assign_svalue_no_free (&RBNODE (new_node)->i.ind, &b_ind);
	  ind_types |= 1 << TYPEOF(b_ind);
	  DO_IF_DEBUG (SET_SVAL_TYPE(RBNODE (new_node)->i.ind,
				     TYPEOF(RBNODE (new_node)->i.ind) |
				     MULTISET_FLAG_MARKER));
	}, {			/* Free m.b_node. */
	});

    else			/* Destructive on a. */
      LOW_RB_MERGE (
	ic_da, m.a_node, m.b_node,
	m.res_list, m.res_length, operation,

	{
	  low_use_multiset_index (RBNODE (m.a_node), a_ind);
	  if (IS_DESTRUCTED (&a_ind)) goto ic_da_free_a;
	}, {
	  low_use_multiset_index (RBNODE (m.b_node), b_ind);
	  if (IS_DESTRUCTED (&b_ind)) goto ic_da_free_b;
	},

	{
	  INTERNAL_CMP (&a_ind, &b_ind, cmp_res);
	},

	{			/* Copy m.a_node. */
	  new_node = m.a_node;
	  ind_types |= 1 << TYPEOF(a_ind);
	}, {			/* Free m.a_node. */
	  free_svalue (&a_ind);
	  UNLINK_RES_NODE (res_msd, m.res_list, got_node_refs, m.a_node);
	},

	{			/* Copy m.b_node. */
	  ALLOC_RES_NODE (m.res, res_msd, new_node);
	  assign_svalue_no_free (&RBNODE (new_node)->i.ind, &b_ind);
	  ind_types |= 1 << TYPEOF(b_ind);
	  DO_IF_DEBUG (SET_SVAL_TYPE(RBNODE (new_node)->i.ind,
				     TYPEOF(RBNODE (new_node)->i.ind) |
				     MULTISET_FLAG_MARKER));
	}, {			/* Free m.b_node. */
	});
  }

  else {
    struct svalue *cmp_less = &m.res->msd->cmp_less;
    struct multiset_data *res_msd = m.res->msd;
    struct svalue a_ind, b_ind;

    Pike_fatal ("TODO: Merge of multisets with external sort function "
	   "not yet implemented.\n");

    LOW_RB_MERGE (
      ec, m.a_node, m.b_node,
      m.res_list, m.res_length, operation,

      {
	low_use_multiset_index (RBNODE (m.a_node), a_ind);
	if (IS_DESTRUCTED (&a_ind)) goto ec_free_a;
      }, {
	low_use_multiset_index (RBNODE (m.b_node), b_ind);
	if (IS_DESTRUCTED (&b_ind)) goto ec_free_b;
      },

      {
	push_svalue (&a_ind);
	push_svalue (&b_ind);
	EXTERNAL_CMP (cmp_less);
	cmp_res = UNSAFE_IS_ZERO (Pike_sp - 1) ? 0 : -1;
	pop_stack();
	if (!cmp_res) {
	  push_svalue (&b_ind);
	  push_svalue (&a_ind);
	  EXTERNAL_CMP (cmp_less);
	  cmp_res = UNSAFE_IS_ZERO (Pike_sp - 1) ? 0 : 1;
	  pop_stack();
	  if (!cmp_res) {
	    /* The values are orderwise equal. Have to check if there
	     * is an orderwise equal sequence in either operand, since
	     * we must do an array-like merge between them in that
	     * case. Knowledge that LOW_RB_MERGE traverses the trees
	     * backwards is used here. */
	    /* TODO */
	  }
	}
      },

      {				/* Copy m.a_node. */
	if (m.rd.a_msd) {
	  ALLOC_RES_NODE (m.res, res_msd, new_node);
	  assign_svalue_no_free (&RBNODE (new_node)->i.ind, &a_ind);
	  ind_types |= 1 << TYPEOF(a_ind);
	  DO_IF_DEBUG (SET_SVAL_TYPE(RBNODE (new_node)->i.ind,
				     TYPEOF(RBNODE (new_node)->i.ind) |
				     MULTISET_FLAG_MARKER));
	}
	else
	  new_node = m.a_node;
      }, {			/* Free m.a_node. */
	if (m.rd.a_msd) {}
	else {
	  free_svalue (&a_ind);
	  UNLINK_RES_NODE (res_msd, m.res_list, got_node_refs, m.a_node);
	}
      },

      {				/* Copy m.b_node. */
	ALLOC_RES_NODE (m.res, res_msd, new_node);
	assign_svalue_no_free (&RBNODE (new_node)->i.ind, &b_ind);
	ind_types |= 1 << TYPEOF(b_ind);
	DO_IF_DEBUG (SET_SVAL_TYPE(RBNODE (new_node)->i.ind,
				   TYPEOF(RBNODE (new_node)->i.ind) |
				   MULTISET_FLAG_MARKER));
      }, {			/* Free m.b_node. */
      });
  }

#undef ALLOC_RES_NODE
#undef UNLINK_RES_NODE

#ifdef PIKE_DEBUG
  if (operation & PIKE_MERGE_DESTR_A) {
    if (a->refs != 1)
      Pike_fatal ("First operand grew external refs during destructive merge.\n");
    if (a->msd->refs > 1)
      Pike_fatal ("Data block of first operand grew external refs "
	     "during destructive merge.\n");
  }
#endif

  UNSET_ONERROR (uwp);
  m.res->msd->root = RBNODE (rb_make_tree (m.res_list, m.res_length));
  m.res->msd->ind_types = ind_types;
  if (m.tmp) free_multiset (m.tmp);
  if (m.rd.a_msd && !sub_ref (m.rd.a_msd)) free_multiset_data (m.rd.a_msd);
  if (!sub_ref (m.rd.b_msd)) free_multiset_data (m.rd.b_msd);
#ifdef PIKE_DEBUG
  if (d_flag > 1) check_multiset (m.res, 1);
#endif
  sub_msnode_ref (m.res);	/* Tries to shrink m.res->msd. */
  return m.res;
}

/* The result has values iff any argument has values. The order is
 * taken from the first argument. No weak flags are propagated to
 * the result. */
PMOD_EXPORT struct multiset *add_multisets (struct svalue *vect, int count)
{
  struct multiset *res, *l;
  int size = 0, idx;
  struct svalue *cmp_less = count ? &vect[0].u.multiset->msd->cmp_less : NULL;
  ONERROR uwp;

  for (idx = 0; idx < count; idx++) {
    struct multiset *l = vect[idx].u.multiset;
    debug_malloc_touch (l);
    debug_malloc_touch (l->msd);
    size += multiset_sizeof (l);
  }

  if (!size)
    return allocate_multiset (0, 0, cmp_less);

  for (idx = 0;; idx++) {
    l = vect[idx].u.multiset;
    if (l->msd->root) break;
  }

  if ((cmp_less ?
       is_identical (cmp_less, &l->msd->cmp_less) :
       TYPEOF(l->msd->cmp_less) == T_INT)) {
    res = copy_multiset (l);
    multiset_set_flags (res, 0);
    idx++;
  }
  else
    res = allocate_multiset (size, 0, cmp_less);
  SET_ONERROR (uwp, do_free_multiset, res);

  for (; idx < count; idx++)
    /* TODO: This is inefficient as long as merge_multisets
     * always is linear. */
    merge_multisets (res, vect[idx].u.multiset,
		     PIKE_MERGE_DESTR_A | PIKE_ARRAY_OP_ADD);

  UNSET_ONERROR (uwp);
  return res;
}

/* Differences in the weak flags are ignored, but not the order
 * function and whether there are values or not. Since the order
 * always is well defined - even in the parts of the multisets where
 * the order function doesn't define it - it's also always
 * significant. */
PMOD_EXPORT int multiset_equal_p (struct multiset *a, struct multiset *b,
				  struct processing *p)
{
  struct processing curr;
  struct recovery_data rd;
  union msnode *a_node, *b_node;
  struct svalue a_ind, b_ind;
  int res;
  ONERROR uwp;

  debug_malloc_touch (a);
  debug_malloc_touch (a->msd);

  if (a == b) return 1;
  if (!a || !b) return 0;

  debug_malloc_touch (b);
  debug_malloc_touch (b->msd);

  if (a->msd == b->msd) return 1;

  check_multiset_for_destruct (a);
  check_multiset_for_destruct (b);

  rd.a_msd = a->msd, rd.b_msd = b->msd;

  if (multiset_sizeof (a) != multiset_sizeof (b) ||
      rd.a_msd->flags || rd.b_msd->flags ||
      !SAME_CMP_LESS (rd.a_msd, rd.b_msd))
    return 0;

  if (!rd.a_msd->root) return !rd.b_msd->root;
  if (!rd.b_msd->root) return 0;

  curr.pointer_a = (void *) a;
  curr.pointer_b = (void *) b;
  curr.next = p;

  for (; p; p = p->next)
    if (p->pointer_a == (void *) a && p->pointer_b == (void *) b)
      return 1;

  add_ref (rd.a_msd);
  add_ref (rd.b_msd);
  SET_ONERROR (uwp, free_recovery_data, &rd);
  a_node = low_multiset_first (rd.a_msd);
  b_node = low_multiset_first (rd.b_msd);
  res = 1;

#define WITH_NODES_BLOCK(TYPE, IND)					\
  do {									\
    if (!low_is_equal (low_use_multiset_index (a_node, a_ind),		\
		       low_use_multiset_index (b_node, b_ind),		\
		       &curr)) {					\
      res = 0;								\
      break;								\
    }									\
    a_node = low_multiset_next (a_node);				\
    b_node = low_multiset_next (b_node);				\
  } while (a_node);

  DO_WITH_NODES (rd.a_msd);

#undef WITH_NODES_BLOCK

  UNSET_ONERROR (uwp);
  if (!sub_ref (rd.a_msd)) free_multiset_data (rd.a_msd);
  if (!sub_ref (rd.b_msd)) free_multiset_data (rd.b_msd);
  return res;
}

void describe_multiset (struct byte_buffer *b, struct multiset *l, struct processing *p, int indent)
{
  struct processing curr;
  struct multiset_data *msd;
  int depth;

  debug_malloc_touch (l);
  debug_malloc_touch (l->msd);

  curr.pointer_a = (void *) l;
  curr.next = p;

  for (depth = 0; p; p = p->next, depth++)
    if (p->pointer_a == (void *) l) {
      char buf[20];
      sprintf (buf, "@%d", depth);
      buffer_add_str (b, buf);
      return;
    }

  check_multiset_for_destruct (l);
  msd = l->msd;

  if (!msd->root)
    buffer_add_str (b, "(< >)");
  else {
    union msnode *node;
    struct svalue ind;
    INT32 size = multiset_sizeof (l);
    int notfirst = 0;
    ONERROR uwp;

    if (size == 1)
      buffer_add_str (b, "(< /* 1 element */\n");
    else {
      char buf[40];
      sprintf (buf, "(< /* %ld elements */\n", (long) size);
      buffer_add_str (b, buf);
    }

    indent += 2;
    add_ref (msd);
    SET_ONERROR (uwp, free_indirect_multiset_data, &msd);
    node = low_multiset_first (msd);

#define WITH_NODES_BLOCK(TYPE, IND)					\
    do {								\
      if (notfirst) buffer_add_str (b, ",\n");				\
      else notfirst = 1;						\
									\
      for (depth = 2; depth < indent; depth++) buffer_add_char (b, ' ');\
      low_use_multiset_index (node, ind);				\
      describe_svalue (b, &ind, indent, &curr);				\
    } while ((node = low_multiset_next (node)));

    DO_WITH_NODES (msd);

#undef WITH_NODES_BLOCK

    buffer_add_char (b, '\n');
    for (depth = 4; depth < indent; depth++) buffer_add_char (b, ' ');
    buffer_add_str (b, ">)");

    UNSET_ONERROR (uwp);
    if (!sub_ref (msd)) free_multiset_data (msd);
  }
}

int multiset_is_constant (struct multiset *l, struct processing *p)
{
  struct multiset_data *msd = l->msd;
  int res = 1;

  debug_malloc_touch (l);
  debug_malloc_touch (msd);

  if (msd->root &&
      (msd->ind_types | msd->val_types) & ~(BIT_INT|BIT_FLOAT|BIT_STRING)) {
    union msnode *node = low_multiset_first (msd);
    struct svalue ind;
    add_ref (msd);

#define WITH_NODES_BLOCK(TYPE, IND)					\
    do {								\
      if (!svalues_are_constant (low_use_multiset_index (node, ind),	\
				 1, msd->ind_types, p)) {		\
	res = 0;							\
	break;								\
      }									\
    } while ((node = low_multiset_next (node)));

    DO_WITH_NODES (msd);

#undef WITH_NODES_BLOCK

    sub_extra_ref (msd);
    assert (msd->refs);
  }

  return res;
}

node *make_node_from_multiset (struct multiset *l)
{
  debug_malloc_touch (l);
  debug_malloc_touch (l->msd);

  multiset_fix_type_field (l);

  if (multiset_is_constant (l, NULL)) {
    struct svalue s;
    if (!l->msd->root) return mkefuncallnode ("aggregate_multiset", NULL);
    SET_SVAL(s, T_MULTISET, 0, multiset, l);
    return mkconstantsvaluenode (&s);
  }

  else {
    struct array *ind = multiset_range_indices (l, -1, -1);
    node *n;

#ifdef PIKE_DEBUG
    if (TYPEOF(l->msd->cmp_less) != T_INT)
      Pike_fatal ("Didn't expect multiset with custom order function.\n");
    if (l->msd->flags & MULTISET_WEAK)
      Pike_fatal ("Didn't expect multiset with weak flag(s).\n");
#endif

    n = mkefuncallnode ("mkmultiset", make_node_from_array (ind));

    free_array (ind);
    return n;
  }
}

/*! @decl multiset aggregate_multiset(mixed ... elems)
 *!
 *! Construct a multiset with the arguments as indices. The multiset
 *! will not contain any values. This method is most useful when
 *! constructing multisets with @[map] or similar; generally, the
 *! multiset literal syntax is handier: @expr{(<elem1, elem2, ...>)@}
 *! With it, it's also possible to construct a multiset with values:
 *! @expr{(<index1: value1, index2: value2, ...>)@}
 *!
 *! @seealso
 *!   @[sizeof()], @[multisetp()], @[mkmultiset()]
 */
PMOD_EXPORT void f_aggregate_multiset (INT32 args)
{
  f_aggregate (args);
  push_multiset (mkmultiset_2 (Pike_sp[-1].u.array, NULL));
  free_array (Pike_sp[-2].u.array);
  Pike_sp[-2] = Pike_sp[-1];
  dmalloc_touch_svalue(Pike_sp-1);
  Pike_sp--;
}

struct multiset *copy_multiset_recursively (struct multiset *l,
					    struct mapping *p)
{
  int not_complex = 0;
  struct tree_build_data new;
  struct multiset_data *msd = l->msd;
  union msnode *node;
  int pos;
  struct svalue ind;
  TYPE_FIELD ind_types = 0, val_types = BIT_INT;
  ONERROR uwp;

  debug_malloc_touch (l);
  debug_malloc_touch (msd);

#ifdef PIKE_DEBUG
  if (d_flag > 1) check_multiset_type_fields (l);
#endif

  if (msd->root && ((msd->ind_types | msd->val_types) & BIT_COMPLEX)) {
    /* Use a dummy empty msd temporarily in the new multiset, since the
     * real one is not suitable for general consumption while it's being
     * built below. This will have the effect that any changes in the
     * multiset made by other code during the build will change the
     * dummy msd and will thus be lost afterwards. */
    new.l = allocate_multiset (0, msd->flags, &msd->cmp_less);
    new.msd = low_alloc_multiset_data (multiset_sizeof (l), msd->flags);
    assign_svalue_no_free (&new.msd->cmp_less, &msd->cmp_less);
    add_ref (new.msd2 = msd);
    SET_ONERROR (uwp, free_tree_build_data, &new);
  }
  else {
    not_complex = 1;
    new.l = copy_multiset (l);
  }

  if (p) {
    struct svalue aa, bb;
    SET_SVAL(aa, T_MULTISET, 0, multiset, l);
    SET_SVAL(bb, T_MULTISET, 0, multiset, new.l);
    mapping_insert(p, &aa, &bb);
  }

  if (not_complex)
    return new.l;

  node = low_multiset_first (msd);
  pos = 0;
  do {
    new.node = INODE (NODE_AT (new.msd, msnode_ind, pos));
    pos++;

    SET_SVAL_TYPE(new.node->i.ind, T_INT);
    SET_SVAL_SUBTYPE(new.node->i.ind, NUMBER_NUMBER);

    low_use_multiset_index (node, ind);
    if (!IS_DESTRUCTED (&ind)) {
      copy_svalues_recursively_no_free (&new.node->i.ind, &ind, 1, p);
      ind_types |= 1 << TYPEOF(new.node->i.ind);

      /* Note: Similar code in multiset_set_cmp_less and mkmultiset_2. */

      while (1) {
	RBSTACK_INIT (rbstack);

	if (!new.msd->root) {
	  low_rb_init_root (HDR (new.msd->root = new.node));
	  goto node_added;
	}

	switch (low_multiset_track_le_gt (new.msd,
					  /* Not clobbered yet. */
					  &new.node->i.ind,
					  &rbstack)) {
	  case FIND_LESS:
	    low_rb_link_at_next (PHDR (&new.msd->root),
				 rbstack, HDR (new.node));
	    goto node_added;
	  case FIND_GREATER:
	    low_rb_link_at_prev (PHDR (&new.msd->root),
				 rbstack, HDR (new.node));
	    goto node_added;
	  case FIND_DESTRUCTED:
	    midflight_remove_node_faster (new.msd, rbstack);
	    break;
	  case FIND_KEY_DESTRUCTED:
	    RBSTACK_FREE (rbstack);
	    goto node_skipped;
	  default: DO_IF_DEBUG (Pike_fatal ("Invalid find_type.\n"));
	}
      }
      UNREACHABLE();

    node_added:
#ifdef PIKE_DEBUG
      SET_SVAL_TYPE(new.node->i.ind,
		    TYPEOF(new.node->i.ind) | MULTISET_FLAG_MARKER);
#endif
      new.msd->size++;
    }

  node_skipped:;
  } while ((node = low_multiset_next (node)));
  new.msd->ind_types = ind_types;
  new.msd->val_types = val_types;

  UNSET_ONERROR (uwp);
  if (!sub_ref (msd)) free_multiset_data (msd);
  assert (!new.msd->refs);

  fix_free_list (new.msd, pos);
  if (!sub_ref (new.l->msd)) free_multiset_data (new.l->msd);
  add_ref (new.l->msd = new.msd);

  return new.l;
}

/* Does not handle n being too large. */
PMOD_EXPORT ptrdiff_t multiset_get_nth (struct multiset *l, size_t n)
{
  add_msnode_ref (l);
  return MSNODE2OFF (l->msd, RBNODE (rb_get_nth (HDR (l->msd->root), n)));
}


#define GC_MSD_VISITED GC_USER_1
#define GC_MSD_GOT_EXT_REFS GC_USER_2
#define GC_MSD_GOT_NODE_REFS GC_USER_3

static void visit_multiset_data (struct multiset_data *msd, int action,
				 void *extra)
{
  visit_enter(msd, T_MULTISET_DATA, extra);
  switch (action & VISIT_MODE_MASK) {
#ifdef PIKE_DEBUG
    default:
      Pike_fatal ("Unknown visit action %d.\n", action);
    case VISIT_NORMAL:
    case VISIT_COMPLEX_ONLY:
      break;
#endif
    case VISIT_COUNT_BYTES:
      mc_counted_bytes += (NODE_OFFSET (msnode_ind, msd->allocsize));
      break;
  }

  if (!(action & VISIT_NO_REFS) &&
      msd->root &&
      ((msd->ind_types | msd->val_types) &
       (action & VISIT_COMPLEX_ONLY ? BIT_COMPLEX : BIT_REF_TYPES))) {
    int ind_ref_type =
      msd->flags & MULTISET_WEAK_INDICES ? REF_TYPE_WEAK : REF_TYPE_NORMAL;
    union msnode *node = low_multiset_first (msd);
    struct svalue ind;
    do {
      low_use_multiset_index (node, ind);
      visit_svalue (&ind, ind_ref_type, extra);
    } while ((node = low_multiset_next (node)));
  }
  visit_leave(msd, T_MULTISET_DATA, extra);
}

PMOD_EXPORT void visit_multiset (struct multiset *l, int action, void *extra)
{
  visit_enter(l, T_MULTISET, extra);
  switch (action & VISIT_MODE_MASK) {
#ifdef PIKE_DEBUG
    default:
      Pike_fatal ("Unknown visit action %d.\n", action);
    case VISIT_NORMAL:
    case VISIT_COMPLEX_ONLY:
      break;
#endif
    case VISIT_COUNT_BYTES:
      mc_counted_bytes += sizeof (struct multiset);
      break;
  }

  visit_ref (l->msd, REF_TYPE_INTERNAL,
	     (visit_thing_fn *) &visit_multiset_data, extra);
  visit_leave(l, T_MULTISET, extra);
}

unsigned gc_touch_all_multisets (void)
{
  unsigned n = 0;
  struct multiset *l;
  if (first_multiset && first_multiset->prev)
    Pike_fatal ("Error in multiset link list.\n");
  for (l = first_multiset; l; l = l->next) {
    debug_gc_touch (l);
    n++;
    if (l->next && l->next->prev != l)
      Pike_fatal ("Error in multiset link list.\n");
  }
  return n;
}

void gc_check_all_multisets (void)
{
  struct multiset *l;

  /* Loop twice: First to get the number of internal refs to the msd:s
   * right, and then again to check the svalues in them correctly.
   * This is necessary since we need to know if an msd got external
   * direct refs to avoid checking its svalues as weak. */

  for (l = first_multiset; l; l = l->next) {
    struct multiset_data *msd = l->msd;

#ifdef DEBUG_MALLOC
    if (((int) PTR_TO_INT (msd)) == 0x55555555) {
      fprintf (stderr, "** Zapped multiset in list of active multisets.\n");
      describe_something (l, T_MULTISET, 0, 2, 0, NULL);
      Pike_fatal ("Zapped multiset in list of active multisets.\n");
    }
#endif
#ifdef PIKE_DEBUG
    if (d_flag > 1) check_multiset (l, 1);
#endif

    GC_ENTER (l, T_MULTISET) {
      debug_gc_check (msd, " as multiset data block of a multiset");
    } GC_LEAVE;
  }

  for (l = first_multiset; l; l = l->next) {
    struct multiset_data *msd = l->msd;
    struct marker *m = get_marker (msd);

    if (!(m->gc_flags & GC_MSD_VISITED))
      GC_ENTER (l, T_MULTISET) {
	if (m->gc_refs < msd->refs) m->gc_flags |= GC_MSD_GOT_EXT_REFS;

	if (msd->root) {
	  union msnode *node = low_multiset_first (msd);
	  struct svalue ind;

#define WITH_NODES_BLOCK(TYPE, IND)					\
	  if (!(msd->flags & MULTISET_WEAK) ||				\
	      (m->gc_flags & GC_MSD_GOT_EXT_REFS))				\
	    do {							\
	      low_use_multiset_index (node, ind);			\
	      debug_gc_check_svalues (&ind, 1, " as multiset index");	\
	    } while ((node = low_multiset_next (node)));		\
									\
	  else {							\
	    switch (msd->flags & MULTISET_WEAK) {			\
	      case MULTISET_WEAK_INDICES:				\
		do {							\
		  low_use_multiset_index (node, ind);			\
		  debug_gc_check_weak_svalues (&ind, 1, " as multiset index"); \
		} while ((node = low_multiset_next (node)));		\
		break;							\
									\
        /*   case MULTISET_WEAK_VALUES: \
		do {							\
		  low_use_multiset_index (node, ind);			\
		  debug_gc_check_svalues (&ind, 1, " as multiset index"); \
		} while ((node = low_multiset_next (node)));		\
		break;                                            \
									\
	      default:							\
		do {							\
		  low_use_multiset_index (node, ind);			\
		  debug_gc_check_weak_svalues (&ind, 1, " as multiset index"); \
		} while ((node = low_multiset_next (node)));		\
		break;*/                                            \
	    }								\
	    gc_checked_as_weak (msd);					\
	  }

	  DO_WITH_NODES (msd);

#undef WITH_NODES_BLOCK
	}

	if (l->node_refs) m->gc_flags |= GC_MSD_GOT_NODE_REFS | GC_MSD_VISITED;
	else m->gc_flags |= GC_MSD_VISITED;
      } GC_LEAVE;
  }
}

static void gc_unlink_msnode_shared (struct multiset_data *msd,
				     struct rbstack_ptr *track,
				     int got_node_refs)
{
  struct rbstack_ptr rbstack = *track;
  union msnode *unlinked_node;

  /* Note: Similar code in unlink_msnode. */

  if (got_node_refs) {
    union msnode *prev, *next;
    unlinked_node = RBNODE (RBSTACK_PEEK (rbstack));
    prev = low_multiset_prev (unlinked_node);
    next = low_multiset_next (unlinked_node);
    low_rb_unlink_without_move (PHDR (&msd->root), &rbstack, 1);
    ADD_TO_FREE_LIST (msd, unlinked_node);
    SET_SVAL_TYPE(unlinked_node->i.ind, T_DELETED);
    unlinked_node->i.prev = (struct msnode_ind *) prev;
    unlinked_node->i.ind.u.ptr = next;
  }

  else {
    unlinked_node =
      RBNODE (low_rb_unlink_with_move (
		PHDR (&msd->root), &rbstack, 1,
		sizeof (struct msnode_ind)));
    CLEAR_DELETED_ON_FREE_LIST (msd);
    ADD_TO_FREE_LIST (msd, unlinked_node);
    SET_SVAL_TYPE(unlinked_node->i.ind, PIKE_T_UNKNOWN);
    unlinked_node->i.prev = NULL;
    msd->size--;
  }

  *track = rbstack;
}

#define GC_RECURSE_MSD_IN_USE(MSD, RECURSE_FN, IND_TYPES, VAL_TYPES) do { \
    union msnode *node = low_multiset_first (MSD);			\
    IND_TYPES = msd->ind_types;						\
    if (node) {								\
      struct svalue ind;						\
									\
      do {								\
	low_use_multiset_index (node, ind);				\
	if (!IS_DESTRUCTED (&ind) && RECURSE_FN (&ind, 1)) {		\
	  DO_IF_DEBUG (Pike_fatal ("Didn't expect an svalue zapping now.\n")); \
	}								\
      } while ((node = low_multiset_next (node)));			\
    }									\
  } while (0)

/* This macro assumes that the msd isn't "in use", i.e. there are no
 * external references directly to it. In that case we can zap svalues
 * in it even if the mapping_data block is shared. */
#define GC_RECURSE(MSD, GOT_NODE_REFS, REC_NODE_I, REC_NODE_IV, TYPE,	\
		   IND_TYPES, VAL_TYPES) do {				\
    struct rb_node_hdr *node = HDR (MSD->root);				\
    if (node) {								\
      struct svalue ind;						\
      int remove;							\
      RBSTACK_INIT (rbstack);						\
      LOW_RB_TRACK_FIRST (rbstack, node);				\
									\
      do {								\
	low_use_multiset_index (RBNODE (node), ind);			\
	REC_NODE_I ((&ind),						\
		    remove,						\
		    PIKE_CONCAT (TYPE, _svalues),			\
		    PIKE_CONCAT (TYPE, _weak_svalues));			\
	if (remove) {							\
	  gc_unlink_msnode_shared (MSD, &rbstack, GOT_NODE_REFS);	\
	  node = RBSTACK_PEEK (rbstack);				\
	}								\
	else {								\
	  IND_TYPES |= 1 << TYPEOF(ind);				\
	  LOW_RB_TRACK_NEXT (rbstack, node);				\
	}								\
      } while (node);							\
    }									\
  } while (0)

#define GC_REC_I_WEAK_NONE(IND, REMOVE, N_REC, W_REC) do {		\
    REMOVE = N_REC (IND, 1);						\
  } while (0)

#define GC_REC_I_WEAK_IND(IND, REMOVE, N_REC, W_REC) do {		\
    REMOVE = W_REC (IND, 1);						\
  } while (0)

#define GC_REC_IV_WEAK_NONE(IND, VAL, REMOVE, N_REC, W_REC, N_TST, W_TST) do { \
    if ((REMOVE = N_REC (IND, 1)))					\
      gc_free_svalue (VAL);						\
    else								\
      N_REC (VAL, 1);							\
  } while (0)

#define GC_REC_IV_WEAK_IND(IND, VAL, REMOVE, N_REC, W_REC, N_TST, W_TST) do { \
    if ((REMOVE = W_REC (IND, 1)))					\
      gc_free_svalue (VAL);						\
    else								\
      N_REC (VAL, 1);							\
  } while (0)

#if MULTISET_WEAK_VALUES
#define GC_REC_IV_WEAK_VAL(IND, VAL, REMOVE, N_REC, W_REC, N_TST, W_TST) do { \
    if ((REMOVE = N_TST (IND))) /* Don't recurse now. */		\
      gc_free_svalue (VAL);						\
    else if ((REMOVE = W_REC (VAL, 1)))					\
      gc_free_svalue (IND);						\
    else								\
      N_REC (IND, 1); /* Now we can recurse the index. */		\
  } while (0)

#define GC_REC_IV_WEAK_BOTH(IND, VAL, REMOVE, N_REC, W_REC, N_TST, W_TST) do { \
    if ((REMOVE = W_TST (IND))) /* Don't recurse now. */		\
      gc_free_svalue (VAL);						\
    else if ((REMOVE = W_REC (VAL, 1)))					\
      gc_free_svalue (IND);						\
    else								\
      W_REC (IND, 1); /* Now we can recurse the index. */		\
  } while (0)
#endif

void gc_mark_multiset_as_referenced (struct multiset *l)
{
  if (gc_mark (l, T_MULTISET))
    GC_ENTER (l, T_MULTISET) {
      struct multiset_data *msd = l->msd;

      if (l == gc_mark_multiset_pos)
	gc_mark_multiset_pos = l->next;
      if (l == gc_internal_multiset)
	gc_internal_multiset = l->next;
      else {
	DOUBLEUNLINK (first_multiset, l);
	DOUBLELINK (first_multiset, l); /* Linked in first. */
      }

      if (gc_mark (msd, T_MULTISET_DATA) && msd->root &&
	  ((msd->ind_types | msd->val_types) & BIT_COMPLEX)) {
	struct marker *m = get_marker (msd);
	TYPE_FIELD ind_types = 0, val_types = 0;

	if (m->gc_flags & GC_MSD_GOT_EXT_REFS) {
	  /* Must leave the multiset data untouched if there are direct
	   * external refs to it. */
	  GC_RECURSE_MSD_IN_USE (msd, gc_mark_svalues, ind_types, val_types);
	  gc_assert_checked_as_nonweak (msd);
	}

	else {
	  switch (msd->flags & MULTISET_WEAK) {
	    case 0:
	      GC_RECURSE (msd, m->gc_flags & GC_MSD_GOT_NODE_REFS,
			  GC_REC_I_WEAK_NONE, GC_REC_IV_WEAK_NONE,
			  gc_mark, ind_types, val_types);
	      gc_assert_checked_as_nonweak (msd);
	      break;
	    case MULTISET_WEAK_INDICES:
	      GC_RECURSE (msd, m->gc_flags & GC_MSD_GOT_NODE_REFS,
			  GC_REC_I_WEAK_IND, GC_REC_IV_WEAK_IND,
			  gc_mark, ind_types, val_types);
	      gc_assert_checked_as_weak (msd);
	      break;
#if MULTISET_WEAK_VALUES
	    case MULTISET_WEAK_VALUES:
	      GC_RECURSE (msd, m->gc_flags & GC_MSD_GOT_NODE_REFS,
			  GC_REC_I_WEAK_NONE, GC_REC_IV_WEAK_VAL,
			  gc_mark, ind_types, val_types);
	      gc_assert_checked_as_weak (msd);
	      break;
	    default:
	      GC_RECURSE (msd, m->gc_flags & GC_MSD_GOT_NODE_REFS,
			  GC_REC_I_WEAK_IND, GC_REC_IV_WEAK_BOTH,
			  gc_mark, ind_types, val_types);
	      gc_assert_checked_as_weak (msd);
#endif
	      break;
	  }

	  if (msd->refs == 1 && DO_SHRINK (msd, 0)) {
	    /* Only shrink the multiset if it isn't shared, or else we
	     * can end up with larger memory consumption since the
	     * shrunk data blocks won't be shared. */
	    debug_malloc_touch (msd);
	    l->msd = resize_multiset_data (msd, ALLOC_SIZE (msd->size), 0);
	    debug_malloc_touch (l->msd);
	    msd = l->msd;
	  }
	}

	msd->ind_types = ind_types;
      }
    } GC_LEAVE;
}

void gc_mark_all_multisets (void)
{
  gc_mark_multiset_pos = gc_internal_multiset;
  while (gc_mark_multiset_pos) {
    struct multiset *l = gc_mark_multiset_pos;
    gc_mark_multiset_pos = l->next;
    if (gc_is_referenced (l)) gc_mark_multiset_as_referenced (l);
  }
}

void real_gc_cycle_check_multiset (struct multiset *l, int weak)
{
  GC_CYCLE_ENTER (l, T_MULTISET, weak) {
    struct multiset_data *msd = l->msd;

    if (msd->root && ((msd->ind_types | msd->val_types) & BIT_COMPLEX)) {
      struct marker *m = get_marker (msd);
      TYPE_FIELD ind_types = 0, val_types = 0;

      if (m->gc_flags & GC_MSD_GOT_EXT_REFS) {
	/* Must leave the multiset data untouched if there are direct
	 * external refs to it. */
	GC_RECURSE_MSD_IN_USE (msd, gc_cycle_check_svalues, ind_types, val_types);
	gc_assert_checked_as_nonweak (msd);
      }

      else {
	switch (msd->flags & MULTISET_WEAK) {
	  case 0:
	    GC_RECURSE (msd, m->gc_flags & GC_MSD_GOT_NODE_REFS,
			GC_REC_I_WEAK_NONE, GC_REC_IV_WEAK_NONE,
			gc_cycle_check, ind_types, val_types);
	    gc_assert_checked_as_nonweak (msd);
	    break;
	  case MULTISET_WEAK_INDICES:
	    GC_RECURSE (msd, m->gc_flags & GC_MSD_GOT_NODE_REFS,
			GC_REC_I_WEAK_IND, GC_REC_IV_WEAK_IND,
			gc_cycle_check, ind_types, val_types);
	    gc_assert_checked_as_weak (msd);
	    break;
#if MULTISET_WEAK_VALUES
	  case MULTISET_WEAK_VALUES:
	    GC_RECURSE (msd, m->gc_flags & GC_MSD_GOT_NODE_REFS,
			GC_REC_I_WEAK_NONE, GC_REC_IV_WEAK_VAL,
			gc_cycle_check, ind_types, val_types);
	    gc_assert_checked_as_weak (msd);
	    break;
	  default:
	    GC_RECURSE (msd, m->gc_flags & GC_MSD_GOT_NODE_REFS,
			GC_REC_I_WEAK_IND, GC_REC_IV_WEAK_BOTH,
			gc_cycle_check, ind_types, val_types);
	    gc_assert_checked_as_weak (msd);
	    break;
#endif
	}

	if (msd->refs == 1 && DO_SHRINK (msd, 0)) {
	  /* Only shrink the multiset if it isn't shared, or else we
	   * can end up with larger memory consumption since the
	   * shrunk data blocks won't be shared. */
	  debug_malloc_touch (msd);
	  l->msd = resize_multiset_data (msd, ALLOC_SIZE (msd->size), 0);
	  debug_malloc_touch (l->msd);
	  msd = l->msd;
	}
      }

      msd->ind_types = ind_types;
    }
  } GC_CYCLE_LEAVE;
}

void gc_cycle_check_all_multisets (void)
{
  struct multiset *l;
  for (l = gc_internal_multiset; l; l = l->next) {
    real_gc_cycle_check_multiset (l, 0);
    gc_cycle_run_queue();
  }
}

void gc_zap_ext_weak_refs_in_multisets (void)
{
  gc_mark_multiset_pos = first_multiset;
  while (gc_mark_multiset_pos != gc_internal_multiset && gc_ext_weak_refs) {
    struct multiset *l = gc_mark_multiset_pos;
    gc_mark_multiset_pos = l->next;
    gc_mark_multiset_as_referenced (l);
  }
  gc_mark_discard_queue();
}

size_t gc_free_all_unreferenced_multisets (void)
{
  struct multiset *l, *next;
  size_t unreferenced = 0;

  for (l = gc_internal_multiset; l; l = next) {
    if (gc_do_free (l)) {
      struct multiset_data *msd = l->msd;
      if (msd->root) {
	/* Replace the msd with an empty one to avoid recursion during free. */
	l->msd = &empty_ind_msd;
	add_ref (l->msd);
	if (!sub_ref (msd)) free_multiset_data (msd);
      }
      gc_free_extra_ref (l);
      SET_NEXT_AND_FREE (l, free_multiset);
    }
    else next = l->next;
    unreferenced++;
  }

  return unreferenced;
}

void init_multiset()
{
#ifdef PIKE_DEBUG
  /* This test is buggy in GCC 4.0.1, hence the volatile. */
  union msnode test;
#define msnode_check(X) ((volatile union msnode *) (X))
  HDR (&test)->flags = 0;
  SET_SVAL(test.i.ind, (1 << 8) - 1, (1 << 16) - 1,
	   refs, (INT32 *) (ptrdiff_t) -1);
  if (HDR (&test)->flags & (MULTISET_FLAG_MASK))
    Pike_fatal("The ind svalue overlays the flags field in an unexpected way.\n"
	       "flags: 0x%08x, MULTISET_FLAG_MASK: 0x%08x\n",
	       HDR(&test)->flags, MULTISET_FLAG_MASK);
  HDR (&test)->flags |= RB_FLAG_MASK;
  if (TYPEOF(test.i.ind) & MULTISET_FLAG_MARKER)
    Pike_fatal("The ind svalue overlays the flags field in an unexpected way.\n"
	       "type: 0x%08x, MULTISET_FLAG_MASK: 0x%08x\n"
	       "RB_FLAG_MASK: 0x%08x, MULTISET_FLAG_MARKER: 0x%08x\n",
	       TYPEOF(test.i.ind), MULTISET_FLAG_MASK,
	       RB_FLAG_MASK, MULTISET_FLAG_MARKER);
  SET_SVAL_TYPE(test.i.ind, TYPEOF(test.i.ind) | MULTISET_FLAG_MARKER);
  if ((TYPEOF(test.i.ind) & ~MULTISET_FLAG_MASK) != (1 << 8) - 1)
    Pike_fatal("The ind svalue overlays the flags field in an unexpected way.\n"
	       "flags: 0x%08x, MULTISET_FLAG_MASK: 0x%08x\n"
	       "RB_FLAG_MASK: 0x%08x, MULTISET_FLAG_MARKER: 0x%08x\n"
	       "type: 0x%08x\n",
	       HDR(&test)->flags, MULTISET_FLAG_MASK,
	       RB_FLAG_MASK, MULTISET_FLAG_MARKER,
	       TYPEOF(test.i.ind));
#undef msnode_check
#endif

#ifdef DEBUG_MALLOC
  /* Make some statically allocated structs known to dmalloc. */
  dmalloc_register (&empty_ind_msd, sizeof (empty_ind_msd),
		    DMALLOC_LOCATION());
  dmalloc_accept_leak (&empty_ind_msd);
  dmalloc_register (&empty_weak_ind_msd, sizeof (empty_weak_ind_msd),
		    DMALLOC_LOCATION());
  dmalloc_accept_leak (&empty_weak_ind_msd);
#endif

}

/* Pike might exit without calling this. */
void exit_multiset()
{
}

#if defined (PIKE_DEBUG) || defined (TEST_MULTISET)

PMOD_EXPORT union msnode *debug_check_msnode (
  struct multiset *l, ptrdiff_t nodepos,
  int allow_deleted, char *file, int line)
{
  struct multiset_data *msd = l->msd;
  union msnode *node;

  if (l->node_refs <= 0)
    Pike_fatal ("%s:%d: Got a node reference to a multiset without any.\n",
		file, line);
  if (nodepos < 0 || nodepos >= msd->allocsize)
    Pike_fatal ("%s:%d: Node offset %"PRINTPTRDIFFT"d "
		"outside storage for multiset (size %d).\n",
		file, line, nodepos, msd->allocsize);

  node = OFF2MSNODE (msd, nodepos);
  switch (TYPEOF(node->i.ind)) {
    case T_DELETED:
      if (!allow_deleted)
	Pike_fatal ("%s:%d: Node at offset %"PRINTPTRDIFFT"d is deleted.\n",
		    file, line, nodepos);
      break;
    case PIKE_T_UNKNOWN:
      Pike_fatal ("%s:%d: Invalid node offset %"PRINTPTRDIFFT"d.\n",
		  file, line, nodepos);
#ifdef PIKE_DEBUG
    default:
      if (!(TYPEOF(node->i.ind) & MULTISET_FLAG_MARKER)) {
#ifdef DEBUG_MALLOC
	fprintf (stderr, "%s:%d: %s", file, line, msg_no_multiset_flag_marker);
	locate_references (l);
#endif
	Pike_fatal ("%s:%d: %s", file, line, msg_no_multiset_flag_marker);
      }
#endif
  }

  return node;
}

#define EXP_NODE_ALLOC 1
#define EXP_NODE_DEL 2
#define EXP_NODE_FREE 4

static void check_low_msnode (struct multiset_data *msd,
			      union msnode *node, int exp_type)
{
  if (node < msd->nodes ||
      node >= (INODE (NODE_AT (msd, msnode_ind, msd->allocsize))))
    Pike_fatal ("Node outside storage for multiset.\n");
  if ((char *) node - (char *) msd->nodes !=
      ((&node->i - &msd->nodes->i) *
       (ptrdiff_t) sizeof (struct msnode_ind)))
    Pike_fatal ("Unaligned node in storage for multiset.\n");

  switch (TYPEOF(node->i.ind)) {
    default:
      if (!(exp_type & EXP_NODE_ALLOC)) Pike_fatal ("Node is in use.\n");
      break;
    case T_DELETED:
      if (!(exp_type & EXP_NODE_DEL)) Pike_fatal ("Node is deleted.\n");
      break;
    case PIKE_T_UNKNOWN:
      if (!(exp_type & EXP_NODE_FREE)) Pike_fatal ("Node is free.\n");
      break;
  }
}

static int inside_check_multiset = 0;

/* The safe flag can be used to avoid the checks that might call pike
 * code or alter memory structures. */
void check_multiset (struct multiset *l, int safe)
{
  struct multiset_data *msd = l->msd;
  int alloc = 0;

  if (inside_check_multiset) return;
  inside_check_multiset = 1;

  if (TYPEOF(msd->cmp_less) == T_INT)
    if (SUBTYPEOF(msd->cmp_less) != NUMBER_NUMBER ||
	msd->cmp_less.u.integer != 0)
      Pike_fatal ("Multiset cmp_less is a nonzero integer: "
		  "subtype=%d, value=%"PRINTPIKEINT"d\n",
		  SUBTYPEOF(msd->cmp_less), msd->cmp_less.u.integer);

  /* Check refs and multiset link list. */

  if (l->refs <= 0)
    Pike_fatal ("Multiset has incorrect refs %d.\n", l->refs);
  if (l->node_refs < 0)
    Pike_fatal ("Multiset has incorrect node_refs %d.\n", l->node_refs);
  if (!msd)
    Pike_fatal ("Multiset has no data block.\n");
  if (msd->refs <= 0)
    Pike_fatal ("Multiset data block has incorrect refs %d.\n", msd->refs);
  if (msd->noval_refs < 0)
    Pike_fatal ("Multiset data block has negative noval_refs %d.\n", msd->noval_refs);
  if (msd->noval_refs > msd->refs)
    Pike_fatal ("Multiset data block has more noval_refs %d than refs %d.\n",
	   msd->noval_refs, msd->refs);

  if (l->next && l->next->prev != l)
    Pike_fatal("multiset->next->prev != multiset.\n");
  if (l->prev) {
    if (l->prev->next != l)
      Pike_fatal ("multiset->prev->next != multiset.\n");
  }
  else
    if (first_multiset != l)
      Pike_fatal ("multiset->prev == 0 but first_multiset != multiset.\n");

  /* Check all node pointers, the tree structure and the type hints. */

  {
    union msnode *node;
    TYPE_FIELD ind_types = 0, val_types = BIT_INT;

    if (msd->root) {
      int pos;

      check_low_msnode (msd, msd->root, EXP_NODE_ALLOC);
      if (msd->free_list)
	check_low_msnode (msd, msd->free_list, EXP_NODE_DEL | EXP_NODE_FREE);

      for (pos = msd->allocsize; pos-- > 0;) {
	node = INODE (NODE_AT (msd, msnode_ind, pos));

	switch (TYPEOF(node->i.ind)) {
	  case T_DELETED:
	    if (node->i.next)
	      /* Don't check the type of the pointed to node; the free
	       * list check below gives a better error for that. */
	      check_low_msnode (msd, INODE (node->i.next),
				EXP_NODE_ALLOC | EXP_NODE_DEL | EXP_NODE_FREE);
	    if (DELETED_PREV (node))
	      check_low_msnode (msd, DELETED_PREV (node), EXP_NODE_ALLOC | EXP_NODE_DEL);
	    if (DELETED_NEXT (node))
	      check_low_msnode (msd, DELETED_NEXT (node), EXP_NODE_ALLOC | EXP_NODE_DEL);
	    break;

	  case PIKE_T_UNKNOWN:
	    if (node->i.next)
	      /* Don't check the type of the pointed to node; the free
	       * list check below gives a better error for that. */
	      check_low_msnode (msd, INODE (node->i.next),
				EXP_NODE_ALLOC | EXP_NODE_DEL | EXP_NODE_FREE);
	    if (node->i.prev)
	      Pike_fatal ("Free node got garbage in prev pointer.\n");
	    break;

	  default:
	    alloc++;
#ifdef PIKE_DEBUG
	    if (!(TYPEOF(node->i.ind) & MULTISET_FLAG_MARKER))
	      Pike_fatal (msg_no_multiset_flag_marker);
#endif
	    ind_types |= 1 << (TYPEOF(node->i.ind) & ~MULTISET_FLAG_MASK);
	    if (node->i.prev)
	      check_low_msnode (msd, INODE (node->i.prev), EXP_NODE_ALLOC);
	    if (node->i.next)
	      check_low_msnode (msd, INODE (node->i.next), EXP_NODE_ALLOC);
	}
      }

#ifdef PIKE_DEBUG
      debug_check_rb_tree (HDR (msd->root),
			   (dump_data_fn *) debug_dump_ind_data,
			   msd);
#endif
    }

    if (ind_types & ~msd->ind_types)
      Pike_fatal ("Multiset indices type field lacked 0x%x.\n", ind_types & ~msd->ind_types);
    if (val_types & ~msd->val_types)
      Pike_fatal ("Multiset values type field lacked 0x%x.\n", val_types & ~msd->val_types);
  }

  /* Check the free list. */

  {
    int deleted = 0, free = 0;
    union msnode *node;

    for (node = msd->free_list; node; node = NEXT_FREE (node)) {
      if (TYPEOF(node->i.ind) == PIKE_T_UNKNOWN) break;
      if (TYPEOF(node->i.ind) != T_DELETED)
	Pike_fatal ("Multiset node in free list got invalid type %d.\n",
		    TYPEOF(node->i.ind));
      deleted++;
    }

    if (node) {
      free++;
      for (node = NEXT_FREE (node); node; node = NEXT_FREE (node)) {
	free++;
	if (TYPEOF(node->i.ind) == T_DELETED)
	  Pike_fatal ("Multiset data got deleted node after free node on free list.\n");
	if (TYPEOF(node->i.ind) != PIKE_T_UNKNOWN)
	  Pike_fatal ("Multiset node in free list got invalid type %d.\n",
		      TYPEOF(node->i.ind));
      }
    }

    if (msd->size != alloc + deleted)
      Pike_fatal ("Multiset data got size %d but tree has %d nodes and %d are deleted.\n",
	     msd->size, alloc, deleted);

    if (free != msd->allocsize - msd->size)
      Pike_fatal ("Multiset data should have %d free nodes but got %d on free list.\n",
	     msd->allocsize - msd->size, free);
  }

  /* Check the order. This can call pike code, so we need to be extra careful. */

  if (!safe && msd->root) {
    JMP_BUF recovery;
    add_msnode_ref (l);
    if (SETJMP (recovery))
      call_handle_error();

    else {
      /* msd duplicated to avoid SETJMP clobber (or at least silence
       * gcc warnings about it). */
      struct multiset_data *msd = l->msd;
      union msnode *node, *next;
      struct svalue tmp1, tmp2;
      ptrdiff_t nextpos;

      node = low_multiset_first (msd);
      low_use_multiset_index (node, tmp1);
#ifdef PIKE_DEBUG
      check_svalue (&tmp1);
#endif

      if (TYPEOF(msd->cmp_less) == T_INT)
	for (; (next = low_multiset_next (node)); node = next) {
	  int cmp_res;
	  low_use_multiset_index (next, tmp2);
	  if (!IS_DESTRUCTED (&tmp2)) {
#ifdef PIKE_DEBUG
	    check_svalue (&tmp2);
#endif

	    nextpos = MSNODE2OFF (msd, next);
	    /* FIXME: Handle destructed index in node. */
	    INTERNAL_CMP (low_use_multiset_index (node, tmp1), &tmp2, cmp_res);
	    if (cmp_res > 0 && cmp_res != CMPFUN_UNORDERED)
	      Pike_fatal ("Order failure in multiset data with internal order.\n");

	    if (l->msd != msd) {
	      msd = l->msd;
	      next = OFF2MSNODE (msd, nextpos);
	      while (next && TYPEOF(next->i.ind) == T_DELETED)
		next = DELETED_PREV (next);
	      if (!next) {
		next = low_multiset_first (msd);
		if (!next) goto order_check_done;
	      }
	    }
	  }
	}

      else
	for (; (next = low_multiset_next (node)); node = next) {
	  low_push_multiset_index (next);
	  if (IS_DESTRUCTED (Pike_sp - 1))
	    pop_stack();
	  else {
#ifdef PIKE_DEBUG
	    check_svalue (Pike_sp - 1);
#endif
	    low_push_multiset_index (node);
	    /* FIXME: Handle destructed index in node. */

	    nextpos = MSNODE2OFF (msd, next);
	    EXTERNAL_CMP (&msd->cmp_less);
	    if (!UNSAFE_IS_ZERO (Pike_sp - 1))
	      Pike_fatal ("Order failure in multiset data with external order.\n");
	    pop_stack();

	    if (l->msd != msd) {
	      msd = l->msd;
	      next = OFF2MSNODE (msd, nextpos);
	      while (next && TYPEOF(next->i.ind) == T_DELETED)
		next = DELETED_PREV (next);
	      if (!next) {
		next = low_multiset_first (msd);
		if (!next) goto order_check_done;
	      }
	    }
	  }
	}

    order_check_done:
      ;
    }

    UNSETJMP (recovery);
    sub_msnode_ref (l);
  }

  inside_check_multiset = 0;
}

void check_all_multisets (int safe)
{
  struct multiset *l;
  for (l = first_multiset; l; l = l->next)
    check_multiset (l, safe);
}

static void debug_dump_ind_data (struct msnode_ind *node,
				 struct multiset_data *msd)
{
  struct svalue tmp;
  print_svalue (stderr, low_use_multiset_index (INODE (node), tmp));
  fprintf (stderr, " (%p) [%"PRINTPTRDIFFT"d]",
	   tmp.u.refs, MSNODE2OFF (msd, INODE (node)));
}

#ifndef PIKE_DEBUG
static void simple_describe_multiset (struct multiset *l)
{
  struct byte_buffer buf = BUFFER_INIT();
  describe_multiset (&buf, l, NULL, 2);
  buffer_add_str(&buf "\n");
  fputs(buffer_get_string(&buf), stderr);
  buffer_free (desc);
}
#endif

void debug_dump_multiset (struct multiset *l)
{
  struct multiset_data *msd = l->msd;

  fprintf (stderr, "Refs=%d, node_refs=%d, next=%p, prev=%p\nmsd=%p",
	   l->refs, l->node_refs, l->next, l->prev, msd);

  if ((ptrdiff_t) msd & 3)
    fputs (" (unaligned)\n", stderr);

  else {
    fprintf (stderr, ", refs=%d, noval_refs=%d, flags=0x%x, size=%d, allocsize=%d\n",
	     msd->refs, msd->noval_refs, msd->flags, msd->size, msd->allocsize);

    if (msd == &empty_ind_msd) {
      fputs ("msd is empty_ind_msd\n", stderr);
    } else if (msd == &empty_weak_ind_msd) {
      fputs ("msd is empty_weak_ind_msd\n", stderr);
    }

#ifdef PIKE_DEBUG
    fputs ("Indices type field =", stderr);
    debug_dump_type_field (msd->ind_types);
    fputs ("\nValues type field =", stderr);
    debug_dump_type_field (msd->val_types);
#endif

    if (TYPEOF(msd->cmp_less) == T_INT)
      fputs ("\nInternal compare function\n", stderr);
    else {
      fputs ("\nCompare function = ", stderr);
      print_svalue (stderr, &msd->cmp_less);
      fputc ('\n', stderr);
    }

#ifdef PIKE_DEBUG
    debug_dump_rb_tree (HDR (msd->root),
			(dump_data_fn *) debug_dump_ind_data,
			msd);
#else
    simple_describe_multiset (l);
#endif

    if (msd->free_list && TYPEOF(msd->free_list->i.ind) == T_DELETED) {
      union msnode *node = msd->free_list;
      fputs ("Deleted nodes:", stderr);
      do {
	if (node != msd->free_list) fputc (',', stderr);
	fprintf (stderr, " %p [%"PRINTPTRDIFFT"d]", node, MSNODE2OFF (msd, node));
      } while ((node = NEXT_FREE (node)) && TYPEOF(node->i.ind) == T_DELETED);
      fputc ('\n', stderr);
    }
  }
}

static void debug_multiset_fatal (struct multiset *l, const char *fmt, ...)
{
  struct multiset_data *msd = l->msd;
  va_list args;
  va_start (args, fmt);
  (void) vfprintf (stderr, fmt, args);
  fprintf (stderr, "Dumping multiset @ %p: ", l);
  debug_dump_multiset (l);
  debug_fatal ("\r");
}

#ifdef TEST_MULTISET

#define TEST_FIND(fn, exp) do {						\
    node = PIKE_CONCAT (multiset_, fn) (l, Pike_sp - 1);                \
    if (node < 0)							\
      multiset_fatal (l, #fn " failed to find %d (%d).\n", exp, i);	\
    if (access_msnode (l, node)->i.ind.u.integer != exp)		\
      multiset_fatal (l, #fn " failed to find %d - "			\
		      "got %"PRINTPIKEINT"d instead (%d).\n",		\
		      exp, access_msnode (l, node)->i.ind.u.integer, i); \
    sub_msnode_ref (l);							\
  } while (0)

#define TEST_NOT_FIND(fn) do {						\
    node = PIKE_CONCAT (multiset_, fn) (l, Pike_sp - 1);                \
    if (node >= 0)							\
      multiset_fatal (l, #fn " failed to not find %"PRINTPIKEINT"d - "	\
		      "got %"PRINTPIKEINT"d (%d).\n",			\
                      Pike_sp[-1].u.integer,                            \
		      access_msnode (l, node)->i.ind.u.integer, i);	\
  } while (0)

#define TEST_STEP_FIND(fn, dir, exp) do {				\
    add_msnode_ref (l);		/* Cheating. */				\
    node = PIKE_CONCAT (multiset_, dir) (l, node);			\
    if (node < 0)							\
      multiset_fatal (l, "Failed to step " #dir " to %d after " #fn	\
		      " of %"PRINTPIKEINT"d (%d).\n",			\
		      exp, Pike_sp[-1].u.integer, i);			\
    if (access_msnode (l, node)->i.ind.u.integer != exp)		\
      multiset_fatal (l, "Failed to step " #dir " to %d after " #fn	\
		      " of %"PRINTPIKEINT"d - "				\
		      "got %"PRINTPIKEINT"d instead (%d).\n",		\
                      exp, Pike_sp[-1].u.integer,                       \
		      access_msnode (l, node)->i.ind.u.integer, i);	\
    sub_msnode_ref (l);							\
  } while (0)

#define TEST_STEP_NOT_FIND(fn, dir) do {				\
    add_msnode_ref (l);		/* Cheating. */				\
    node = PIKE_CONCAT (multiset_, dir) (l, node);			\
    if (node >= 0)							\
      multiset_fatal (l, "Failed to step " #dir " to end after " #fn	\
		      " of %"PRINTPIKEINT"d - "				\
		      "got %"PRINTPIKEINT"d (%d).\n",			\
                      Pike_sp[-1].u.integer,                            \
		      access_msnode (l, node)->i.ind.u.integer, i);	\
    sub_msnode_ref (l);							\
  } while (0)

static int naive_test_equal (struct multiset *a, struct multiset *b)
{
  union msnode *na, *nb;
  struct svalue sa, sb;
  na = low_multiset_first (a->msd);
  nb = low_multiset_first (b->msd);
  while (na && nb) {
    low_use_multiset_index (na, sa);
    low_use_multiset_index (nb, sb);
    if (TYPEOF(sa) != TYPEOF(sb) || sa.u.integer != sb.u.integer ||
	na->iv.val.u.integer != nb->iv.val.u.integer))) return 0;
    na = low_multiset_next (na);
    nb = low_multiset_next (nb);
  }
  return !(na || nb);
}

static void debug_merge_fatal (struct multiset *a, struct multiset *b,
			       struct multiset *exp, struct multiset *got,
			       const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  (void) vfprintf (stderr, fmt, args);
  fputs ("Dumping a: ", stderr);
  debug_dump_multiset (a);
  fputs ("Dumping b: ", stderr);
  debug_dump_multiset (b);
  fputs ("Dumping expected: ", stderr);
  debug_dump_multiset (exp);
  fputs ("Dumping got: ", stderr);
  debug_dump_multiset (got);
  debug_fatal ("\r");
}

#ifdef TEST_MULTISET_VERBOSE
#define TM_VERBOSE(...)	fprintf(stderr, __VA_ARGS__)
#else /* !TEST_MULTISET_VERBOSE */
#define TM_VERBOSE(...)
#endif /* TEST_MULTISET_VERBOSE */

void test_multiset (void)
{
  int pass, i, j, v, vv, old_d_flag = d_flag;
  struct svalue *less_efun, *greater_efun, tmp, *orig_sp = Pike_sp;
  struct array *arr;
  struct multiset *l, *l2;
  ptrdiff_t node;
  d_flag = 3;

  push_svalue (simple_mapping_string_lookup (get_builtin_constants(), "`<"));
  less_efun = Pike_sp - 1;
  push_svalue (simple_mapping_string_lookup (get_builtin_constants(), "`>"));
  greater_efun = Pike_sp - 1;

  for (pass = 0; pass < 2; pass++) {
    push_int (1);
    push_int (1);
    push_int (2);
    push_int (4);
    push_int (5);
    push_int (5);
    push_int (7);
    push_int (8);
    push_int (11);
    push_int (14);
    push_int (15);
    push_int (15);
    f_aggregate (12);

    for (i = 1*2*3*4*5*6*7*8*9; i > 0; i--) {
      if (!(i % 1000)) fprintf (stderr, "ind %s %d         \r",
				pass ? "cmp_less" : "internal", i);

      TM_VERBOSE("pass:%d, i:%d\n", pass, i);

      l = allocate_multiset (0, 0, pass ? less_efun : NULL);
      stack_dup();
      push_int (i);
      f_permute (2);
      arr = Pike_sp[-1].u.array;

      TM_VERBOSE("insert: ");
      for (j = 0; j < 12; j++) {
        TM_VERBOSE("arr[%d]=%d ", j, arr->item[j].u.integer);
	multiset_insert_2 (l, &arr->item[j]);
	check_multiset (l, 0);
      }
      if (multiset_sizeof (l) != 9)
	multiset_fatal (l, "Size is wrong: %d (%d)\n", multiset_sizeof (l), i);

      TM_VERBOSE("\nfind 5 ");
      push_int (5);
      TEST_FIND (find_eq, 5);
      TEST_FIND (find_lt, 4);
      TEST_FIND (find_gt, 7);
      TEST_FIND (find_le, 5);
      TEST_FIND (find_ge, 5);
      pop_stack();

      TM_VERBOSE("6 ");
      push_int (6);
      TEST_NOT_FIND (find_eq);
      TEST_FIND (find_lt, 5);
      TEST_FIND (find_gt, 7);
      TEST_FIND (find_le, 5);
      TEST_FIND (find_ge, 7);
      pop_stack();

      TM_VERBOSE("0 ");
      push_int (0);
      TEST_NOT_FIND (find_eq);
      TEST_NOT_FIND (find_lt);
      TEST_FIND (find_gt, 1);
      TEST_NOT_FIND (find_le);
      TEST_FIND (find_ge, 1);
      pop_stack();

      TM_VERBOSE("1 ");
      push_int (1);
      TEST_FIND (find_eq, 1);
      TEST_NOT_FIND (find_lt);
      TEST_FIND (find_gt, 2);
      TEST_FIND (find_le, 1);
      TEST_FIND (find_ge, 1);
      pop_stack();

      TM_VERBOSE("15 ");
      push_int (15);
      TEST_FIND (find_eq, 15);
      TEST_FIND (find_lt, 14);
      TEST_NOT_FIND (find_gt);
      TEST_FIND (find_le, 15);
      TEST_FIND (find_ge, 15);
      pop_stack();

      TM_VERBOSE("17\n");
      push_int (17);
      TEST_NOT_FIND (find_eq);
      TEST_FIND (find_lt, 15);
      TEST_NOT_FIND (find_gt);
      TEST_FIND (find_le, 15);
      TEST_NOT_FIND (find_ge);
      pop_stack();

      l2 = l;
      TM_VERBOSE("delete: ");
      for (j = 0, v = 0; j < 12; j++) {
        TM_VERBOSE("arr[%d]=%d ", j, arr->item[j].u.integer);
	v += !!multiset_delete_2 (l2, &arr->item[j], NULL);
	if (multiset_find_eq (l2, &arr->item[j]) >= 0)
	  multiset_fatal (l2, "Entry %"PRINTPIKEINT"d not deleted (%d).\n",
			  arr->item[j].u.integer, i);
	check_multiset (l2, 0);
      }
      if (v != 9 || l2->msd->root)
	multiset_fatal (l2, "Wrong number of entries deleted: %d (%d)\n", v, i);
      TM_VERBOSE("\n");
      free_multiset (l);
      pop_stack();
    }
    pop_stack();
  }

  for (pass = 0; pass < 2; pass++) {
    push_int (1);
    push_int (1);
    push_int (4);
    push_int (5);
    push_int (5);
    push_int (7);
    push_int (15);
    push_int (15);
    f_aggregate (8);

    for (i = 1*2*3*4*5*6*7*8; i > 0; i--) {
      if (!(i % 1000)) fprintf (stderr, "indval %s %d         \r",
				pass ? "cmp_less" : "internal", i);

      stack_dup();
      push_int (i);
      f_permute (2);
      arr = Pike_sp[-1].u.array;

      {
	ptrdiff_t nodes[8];
	l = allocate_multiset (0, 0, pass ? less_efun : NULL);
	push_int (17);

	for (j = 0; j < 8; j++) {
	  int node_ref = (node = multiset_last (l)) >= 0;
	  for (; node >= 0; node = multiset_prev (l, node)) {
	    if (get_multiset_value (l, node)->u.integer <=
		arr->item[j].u.integer) break;
	  }
	  nodes[j] = multiset_add_after (l, node, Pike_sp - 1, &arr->item[j]);
	  if (node_ref) sub_msnode_ref (l);
	  if (nodes[j] < 0) {
	    if (node < 0)
	      multiset_fatal (l, "Failed to add "
			      "%"PRINTPIKEINT"d:%"PRINTPIKEINT"d first: "
			      "%"PRINTPTRDIFFT"d\n",
			      Pike_sp[-1].u.integer, arr->item[j].u.integer, nodes[j]);
	    else
	      multiset_fatal (l, "Failed to add "
			      "%"PRINTPIKEINT"d:%"PRINTPIKEINT"d after "
			      "%"PRINTPIKEINT"d:%"PRINTPIKEINT"d: "
			      "%"PRINTPTRDIFFT"d\n",
			      Pike_sp[-1].u.integer, arr->item[j].u.integer,
			      use_multiset_index (l, node, tmp)->u.integer,
			      get_multiset_value (l, node)->u.integer,
			      nodes[j]);
	  }
	  add_msnode_ref (l);
	  check_multiset (l, 0);
	}
	if (j != 8) multiset_fatal (l, "Size is wrong: %d (%d)\n", j, i);

	add_msnode_ref (l);
	for (j = 0; j < 8; j++) {
	  multiset_delete_node (l, nodes[j]);
	  check_multiset (l, 0);
	}
	sub_msnode_ref (l);
	if (multiset_sizeof (l))
	  multiset_fatal (l, "Whole tree not deleted (%d)\n", i);
	free_multiset (l);
	pop_stack();
      }

      l = allocate_multiset (0, 0, pass ? less_efun : NULL);

      for (j = 0; j < 8; j++) {
	push_int (arr->item[j].u.integer * 8 + j);
	multiset_add (l, &arr->item[j], Pike_sp - 1);
	check_multiset (l, 0);
	pop_stack();
      }

      for (j = 0, v = 0, node = multiset_first (l);
	   node >= 0; node = multiset_next (l, node), j++) {
	push_multiset_index (l, node);
	if (v >= get_multiset_value (l, node)->u.integer)
	  multiset_fatal (l, "Failed to sort values (%d).\n", i);
	v = get_multiset_value (l, node)->u.integer;
	pop_stack();
      }
      if (j != 8 || multiset_sizeof (l) != j)
	multiset_fatal (l, "Size is wrong: %d (%d)\n", j, i);
      sub_msnode_ref (l);

      push_int (5);
      TEST_FIND (find_eq, 5);
      TEST_STEP_FIND (find_eq, next, 7);
      TEST_FIND (find_lt, 4);
      TEST_FIND (find_gt, 7);
      TEST_FIND (find_le, 5);
      TEST_STEP_FIND (find_le, next, 7);
      TEST_FIND (find_ge, 5);
      TEST_STEP_FIND (find_ge, prev, 4);
      pop_stack();

      push_int (6);
      TEST_NOT_FIND (find_eq);
      TEST_FIND (find_lt, 5);
      TEST_FIND (find_gt, 7);
      TEST_FIND (find_le, 5);
      TEST_STEP_FIND (find_le, next, 7);
      TEST_FIND (find_ge, 7);
      TEST_STEP_FIND (find_ge, prev, 5);
      pop_stack();

      push_int (0);
      TEST_NOT_FIND (find_eq);
      TEST_NOT_FIND (find_lt);
      TEST_FIND (find_gt, 1);
      TEST_STEP_NOT_FIND (find_gt, prev);
      TEST_NOT_FIND (find_le);
      TEST_FIND (find_ge, 1);
      TEST_STEP_FIND (find_ge, next, 1);
      pop_stack();

      push_int (1);
      TEST_FIND (find_eq, 1);
      TEST_STEP_FIND (find_eq, next, 4);
      TEST_NOT_FIND (find_lt);
      TEST_FIND (find_gt, 4);
      TEST_FIND (find_le, 1);
      TEST_STEP_FIND (find_le, next, 4);
      TEST_FIND (find_ge, 1);
      TEST_STEP_NOT_FIND (find_ge, prev);
      pop_stack();

      push_int (15);
      TEST_FIND (find_eq, 15);
      TEST_STEP_NOT_FIND (find_eq, next);
      TEST_FIND (find_lt, 7);
      TEST_NOT_FIND (find_gt);
      TEST_FIND (find_le, 15);
      TEST_STEP_NOT_FIND (find_le, next);
      TEST_FIND (find_ge, 15);
      TEST_STEP_FIND (find_ge, prev, 7);
      pop_stack();

      push_int (17);
      TEST_NOT_FIND (find_eq);
      TEST_FIND (find_lt, 15);
      TEST_STEP_NOT_FIND (find_lt, next);
      TEST_NOT_FIND (find_gt);
      TEST_FIND (find_le, 15);
      TEST_STEP_FIND (find_le, prev, 15);
      TEST_NOT_FIND (find_ge);
      pop_stack();

      l2 = copy_multiset (l);
      check_multiset (l2, 0);
      if (!naive_test_equal (l, l2))
	multiset_fatal (l2, "Copy not equal to original (%d).\n", i);

      push_int (-1);
      for (j = 0; j < 8; j++) {
	multiset_insert_2 (l2, &arr->item[j]);
	if (multiset_sizeof (l2) != multiset_sizeof (l))
	  multiset_fatal (l2, "Duplicate entry "
			  "%"PRINTPIKEINT"d inserted (%d).\n",
			  arr->item[j].u.integer, i);
	if (get_multiset_value (
	      l2, multiset_find_eq (l2, &arr->item[j]))->u.integer == -1)
	  multiset_fatal (l2, "Insert replaced last entry "
			  "%"PRINTPIKEINT"d (%d).\n",
			  arr->item[j].u.integer, i);
	sub_msnode_ref (l2);
      }
      for (j = 0; j < 8; j++) {
	multiset_insert_2 (l2, &arr->item[j]);
	if (multiset_sizeof (l2) != multiset_sizeof (l))
	  multiset_fatal (l2, "Duplicate entry "
			  "%"PRINTPIKEINT"d inserted (%d).\n",
			  arr->item[j].u.integer, i);
	if (get_multiset_value (
	      l2, multiset_find_eq (l2, &arr->item[j]))->u.integer != -1)
	  multiset_fatal (l2, "Insert didn't replace last entry "
			  "%"PRINTPIKEINT"d (%d).\n",
			  arr->item[j].u.integer, i);
	sub_msnode_ref (l2);
      }
      pop_stack();

      for (v = 0; multiset_sizeof (l2); v++) {
	add_msnode_ref (l2);
	multiset_delete_node (l2, MSNODE2OFF (l2->msd, l2->msd->root));
	check_multiset (l2, 0);
      }
      if (v != 8)
	multiset_fatal (l2, "Wrong number of entries deleted: %d (%d)\n", v, i);
      free_multiset (l2);

      for (j = 0, v = 0; j < 8; j++) {
	if (!multiset_delete_2 (l, &arr->item[j], &tmp))
	  multiset_fatal (l, "Entry %"PRINTPIKEINT"d not deleted (%d).\n",
			  arr->item[j].u.integer, i);
	if ((node = multiset_find_eq (l, &arr->item[j])) >= 0) {
	  if (get_multiset_value (l, node)->u.integer >= tmp.u.integer)
	    multiset_fatal (l, "Last entry %"PRINTPIKEINT"d not deleted (%d).\n",
			    arr->item[j].u.integer, i);
	  sub_msnode_ref (l);
	}
	free_svalue (&tmp);
	check_multiset (l, 0);
      }

      free_multiset (l);
      pop_stack();
    }
    pop_stack();
  }

  for (pass = 0; pass < 2; pass++) {
    int max = 1000000;
    l = allocate_multiset (0, 0, pass ? less_efun : NULL);
    srand (0);
#ifdef RB_STATS
    reset_rb_stats();
#endif

    for (i = max, v = 0; i > 0; i--) {
      if (!(i % 10000)) fprintf (stderr, "grow %s %d, %d duplicates         \r",
				 pass ? "cmp_less" : "internal", i, v);
      push_int (rand());
      if (multiset_find_eq (l, Pike_sp - 1) >= 0) {
	v++;
	sub_msnode_ref (l);
      }
      multiset_add (l, Pike_sp - 1, NULL);

      {
	struct multiset_data *msd = l->msd;
	RBSTACK_INIT (rbstack_find);
	add_ref (msd);
	if (low_multiset_track_eq (msd, Pike_sp - 1, &rbstack_find) != FIND_EQUAL)
	  fprintf (stderr, "Round %d: Didn't find node after add.\n", i);
	else {
	  RBSTACK_INIT (rbstack_build);
	  low_rb_build_stack (HDR (msd->root), RBSTACK_PEEK (rbstack_find),
			      &rbstack_build);
	  while (1) {
	    struct rb_node_hdr *f, *b;
	    RBSTACK_POP (rbstack_find, f);
	    RBSTACK_POP (rbstack_build, b);
	    if (f != b) {
	      fprintf (stderr, "Round %d: find and build stacks differ.\n", i);
	      break;
	    }
	    if (!f) break;
	  }
	  RBSTACK_FREE (rbstack_build);
	}
	RBSTACK_FREE (rbstack_find);
	sub_extra_ref (msd);
      }

      pop_stack();
    }

    if (v != 114)
      fprintf (stderr, "Got %d duplicates but expected 114 - "
	       "the pseudorandom sequence isn't as expected\n", v);

#ifdef RB_STATS
    fputc ('\n', stderr), print_rb_stats (1);
#endif
    check_multiset (l, 0);
    srand (0);

    for (i = max; i > 0; i--) {
      if (!(i % 10000)) fprintf (stderr, "shrink %s %d                   \r",
				 pass ? "cmp_less" : "internal", i);
      push_int (rand());
      if (!multiset_delete (l, Pike_sp - 1))
	Pike_fatal ("Pseudo-random sequence didn't repeat.\n");
      pop_stack();
    }

#ifdef RB_STATS
    fputc ('\n', stderr), print_rb_stats (1);
#endif
    if (multiset_sizeof (l))
      multiset_fatal (l, "Multiset not empty.\n");
    free_multiset (l);
  }

  if (1) {
    int max = 400;
    struct array *arr = allocate_array_no_init (0, max);
    struct svalue *sval;
    srand (0);
    for (i = j = 0; i < max; i++) {
      if (!(i % 10)) fprintf (stderr, "maketree %d         \r",
			      max * 10 - arr->size);

      l = mkmultiset_2 (arr, i & 1 ? less_efun : NULL);
      check_multiset (l, 0);
      multiset_set_cmp_less (l, i & 4 ? less_efun : NULL);
      check_multiset (l, 0);
      multiset_set_flags (l, 0);
      check_multiset (l, 0);
      multiset_set_cmp_less (l, greater_efun);
      check_multiset (l, 0);

      if ((node = multiset_first (l)) >= 0) {
        int pos = 0, try_get = (rand() & INT_MAX) % arr->size;
	for (; node >= 0; node = multiset_next (l, node), pos++)
	  if (pos == try_get) {
	    if ((v = use_multiset_index (
		   l, multiset_get_nth (l, try_get), tmp)->u.integer) !=
		arr->size - try_get - 1)
	      multiset_fatal (l, "Element @ %d is %d, but %d was expected (%d).\n",
			      try_get, v, arr->size - try_get - 1, i);
	    sub_msnode_ref (l);
	    if ((v = get_multiset_value (l, node)->u.integer) !=
		(vv = ((i & (8|2)) == (8|2) ? arr->size - try_get - 1 : 1)))
	      multiset_fatal (l, "Element @ %d got value %d, but %d was expected (%d).\n",
			      try_get, v, vv, i);
	    break;
	  }
	sub_msnode_ref (l);
      }

      free_multiset (l);
      arr = resize_array (arr, j + 10);
      for (; j < arr->size; j++)
	ITEM (arr)[j].u.integer = j;
    }
    free_array (arr);
  }

  for (pass = 0; pass < 1; pass++) {
    struct multiset *a =
      allocate_multiset (0, 0, pass ? less_efun : NULL);
    struct multiset *b =
      allocate_multiset (0, 0, pass ? less_efun : NULL);
    struct multiset *and =
      allocate_multiset (0, 0, pass ? less_efun : NULL);
    struct multiset *or =
      allocate_multiset (0, 0, pass ? less_efun : NULL);
    struct multiset *add =
      allocate_multiset (0, 0, pass ? less_efun : NULL);
    struct multiset *sub =
      allocate_multiset (0, 0, pass ? less_efun : NULL);
    struct multiset *xor =
      allocate_multiset (0, 0, pass ? less_efun : NULL);
    int action = 0;

    srand (0);
    for (i = 5000; i >= 0; i--, action = action % 6 + 1) {
      int nr = rand() & INT_MAX; /* Assumes we keep within one period. */

      if (!(i % 100)) fprintf (stderr, "merge %d         \r", i);

      switch (action) {
	case 1:			/* Unique index added to a only. */
	  push_int (nr);
	  push_int (1);
	  multiset_add (a, Pike_sp - 2, Pike_sp - 1);
	  multiset_add (add, Pike_sp - 2, Pike_sp - 1);
	  multiset_add (sub, Pike_sp - 2, Pike_sp - 1);
	  multiset_add (xor, Pike_sp - 2, Pike_sp - 1);
	  goto add_unique;

	case 2:			/* Unique index added to b only. */
	  push_int (nr);
	  push_int (2);
	  multiset_add (b, Pike_sp - 2, Pike_sp - 1);
	  multiset_add (add, Pike_sp - 2, Pike_sp - 1);
	  multiset_add (xor, Pike_sp - 2, Pike_sp - 1);
	  goto add_unique;

	case 3:			/* Unique index added to a and b. */
	  push_int (nr);
	  push_int (1);
	  multiset_add (a, Pike_sp - 2, Pike_sp - 1);
	  multiset_add (add, Pike_sp - 2, Pike_sp - 1);
	  pop_stack();
	  push_int (2);
	  multiset_add (b, Pike_sp - 2, Pike_sp - 1);
	  multiset_add (and, Pike_sp - 2, Pike_sp - 1);
	  multiset_add (add, Pike_sp - 2, Pike_sp - 1);

      add_unique:
	  if (multiset_lookup (or, Pike_sp - 2))
	    multiset_fatal (or, "Duplicate index %d not expected here.\n", nr);
	  multiset_insert_2 (or, Pike_sp - 2);
	  pop_stack();
	  pop_stack();
	  break;

	case 4:			/* Duplicate index added to a only. */
	  nr = low_use_multiset_index (
	    low_multiset_get_nth (
	      sub->msd, nr % multiset_sizeof (sub)), tmp)->u.integer;
	  push_int (nr);
	  push_int (1);
	  multiset_add (a, Pike_sp - 2, Pike_sp - 1);
	  multiset_add (or, Pike_sp - 2, Pike_sp - 1);
	  multiset_add (add, Pike_sp - 2, Pike_sp - 1);
	  multiset_add (sub, Pike_sp - 2, Pike_sp - 1);
	  multiset_add (xor, Pike_sp - 2, Pike_sp - 1);
	  pop_stack();
	  pop_stack();
	  break;

	case 5:			/* Duplicate index added to b only. */
	  nr = low_use_multiset_index (
	    low_multiset_get_nth (
	      b->msd, nr % multiset_sizeof (b)), tmp)->u.integer;
	  push_int (nr);
	  push_int (2);
	  multiset_add (b, Pike_sp - 2, Pike_sp - 1);
	  multiset_add (or, Pike_sp - 2, Pike_sp - 1);
	  multiset_add (add, Pike_sp - 2, Pike_sp - 1);
	  multiset_add (xor, Pike_sp - 2, Pike_sp - 1);
	  pop_stack();
	  pop_stack();
	  break;

	case 6:			/* Duplicate index added to a and b. */
	  nr = low_use_multiset_index (
	    low_multiset_get_nth (
	      b->msd, nr % multiset_sizeof (b)), tmp)->u.integer;
	  push_int (nr);
	  push_int (1);
	  multiset_add (a, Pike_sp - 2, Pike_sp - 1);
	  node = multiset_find_lt (add, Pike_sp - 2);
	  if ((nr = multiset_add_after (add, node, Pike_sp - 2, Pike_sp - 1)) < 0)
	    multiset_fatal (add, "Failed to add "
			    "%"PRINTPIKEINT"d:1 after "
			    "%"PRINTPTRDIFFT"d: %d.\n",
			    Pike_sp[-2].u.integer, node, nr);
	  if (node >= 0) sub_msnode_ref (add);
	  pop_stack();
	  push_int (2);
	  multiset_add (b, Pike_sp - 2, Pike_sp - 1);
	  multiset_add (and, Pike_sp - 2, Pike_sp - 1);
	  multiset_add (or, Pike_sp - 2, Pike_sp - 1);
	  multiset_add (add, Pike_sp - 2, Pike_sp - 1);
	  pop_stack();
	  pop_stack();
	  break;
      }

      if (i % 10) continue;

      l = merge_multisets (a, b, PIKE_ARRAY_OP_AND);
      if (!naive_test_equal (and, l))
	debug_merge_fatal (a, b, and, l, "Invalid 'and' merge (%d).\n", i);
      free_multiset (l);
      l = copy_multiset (a);
      merge_multisets (l, b, PIKE_ARRAY_OP_AND | PIKE_MERGE_DESTR_A);
      if (!naive_test_equal (and, l))
	debug_merge_fatal (a, b, and, l, "Invalid destructive 'and' merge (%d).\n", i);
      free_multiset (l);

      l = merge_multisets (a, b, PIKE_ARRAY_OP_OR);
      if (!naive_test_equal (or, l))
	debug_merge_fatal (a, b, or, l, "Invalid 'or' merge (%d).\n", i);
      free_multiset (l);
      l = copy_multiset (a);
      merge_multisets (l, b, PIKE_ARRAY_OP_OR | PIKE_MERGE_DESTR_A);
      if (!naive_test_equal (or, l))
	debug_merge_fatal (a, b, or, l, "Invalid destructive 'or' merge (%d).\n", i);
      free_multiset (l);

      l = merge_multisets (a, b, PIKE_ARRAY_OP_ADD);
      if (!naive_test_equal (add, l))
	debug_merge_fatal (a, b, add, l, "Invalid 'add' merge (%d).\n", i);
      free_multiset (l);
      l = copy_multiset (a);
      merge_multisets (l, b, PIKE_ARRAY_OP_ADD | PIKE_MERGE_DESTR_A);
      if (!naive_test_equal (add, l))
	debug_merge_fatal (a, b, add, l, "Invalid destructive 'add' merge (%d).\n", i);
      free_multiset (l);

      l = merge_multisets (a, b, PIKE_ARRAY_OP_SUB);
      if (!naive_test_equal (sub, l))
	debug_merge_fatal (a, b, sub, l, "Invalid 'sub' merge (%d).\n", i);
      free_multiset (l);
      l = copy_multiset (a);
      merge_multisets (l, b, PIKE_ARRAY_OP_SUB | PIKE_MERGE_DESTR_A);
      if (!naive_test_equal (sub, l))
	debug_merge_fatal (a, b, sub, l, "Invalid destructive 'sub' merge (%d).\n", i);
      free_multiset (l);

      l = merge_multisets (a, b, PIKE_ARRAY_OP_XOR);
      if (!naive_test_equal (xor, l))
	debug_merge_fatal (a, b, xor, l, "Invalid 'xor' merge (%d).\n", i);
      free_multiset (l);
      l = copy_multiset (a);
      merge_multisets (l, b, PIKE_ARRAY_OP_XOR | PIKE_MERGE_DESTR_A);
      if (!naive_test_equal (xor, l))
	debug_merge_fatal (a, b, xor, l, "Invalid destructive 'xor' merge (%d).\n", i);
      free_multiset (l);

      check_multiset (a, 0);
    }

    free_multiset (a);
    free_multiset (b);
    free_multiset (and);
    free_multiset (or);
    free_multiset (add);
    free_multiset (sub);
    free_multiset (xor);
  }

  pop_2_elems();
  if (orig_sp != Pike_sp)
    Pike_fatal ("Stack wrong: %"PRINTPTRDIFFT"d extra elements.\n", Pike_sp - orig_sp);
  fprintf (stderr, "                            \r");
  d_flag = old_d_flag;
}

#endif /* TEST_MULTISET */

#endif /* PIKE_DEBUG || TEST_MULTISET */
