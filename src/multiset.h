/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: multiset.h,v 1.36 2004/09/26 22:07:02 mast Exp $
*/

#ifndef MULTISET_H
#define MULTISET_H

/* Multisets using rbtree.
 *
 * Created by Martin Stjernholm 2001-05-07
 */

/* #define TEST_MULTISET */

#include "svalue.h"
#include "dmalloc.h"
#include "rbtree.h"
#include "block_alloc_h.h"

/* Note: Don't access the ind svalue (or at least not its type field)
 * in the following directly, since the rbtree flags overlay that. Use
 * assign_multiset_index_no_free() or similar instead. */

struct msnode_ind
{
  struct msnode_ind *prev, *next; /* Must be first. */
  struct svalue ind;		/* Must be second. */
};

struct msnode_indval
{
  struct msnode_indval *prev, *next; /* Must be first. */
  struct svalue ind;		/* Must be second. */
  struct svalue val;
};

union msnode
{
  struct msnode_ind i;
  struct msnode_indval iv;
};

#define MULTISET_FLAG_MARKER	0x1000
#define MULTISET_FLAG_MASK	(RB_FLAG_MASK | MULTISET_FLAG_MARKER)
/* The flag marker is used to reliably nail direct uses of the ind
 * type field. */

struct multiset_data
{
  INT32 refs, noval_refs;
#ifdef PIKE_RUN_UNLOCKED
#error multiset_data has not been adapted for running unlocked.
#endif
  union msnode *root, *free_list;
  struct svalue cmp_less;
  INT32 size, allocsize;
  TYPE_FIELD ind_types;
  TYPE_FIELD val_types;		/* Always BIT_INT in valueless multisets. */
  INT16 flags;
  union msnode nodes[1];
};

struct multiset
{
  PIKE_MEMORY_OBJECT_MEMBERS;
  struct multiset_data *msd;
  struct multiset *next, *prev;
  INT32 node_refs;
};

/* Data structure notes:
 *
 * o  The node free list through multiset_data.free_list is singly
 *    linked by the next pointers. Nodes on the free list are either
 *    free or deleted (see below). Free nodes have ind.type set to
 *    PIKE_T_UNKNOWN, and deleted nodes are flagged with T_DELETED.
 *
 *    Deleted nodes are always before free nodes on the free list.
 *    multiset_data.size counts both the allocated and the deleted
 *    nodes. Note that deleted nodes might still be on the free list
 *    even when there are no node references (see below).
 *
 * o  multiset_data.cmp_less.type is T_INT when the internal set order
 *    is used.
 *
 * o  multset_data.refs counts the number of "independent" references
 *    to the data block. When it's greater than one, the data block
 *    must always be copied if anything but a node value is changed.
 *    Changes of node values might be allowed depending on
 *    multset_data.noval_refs.
 *
 *    A direct pointer to or into the data block can only be kept if
 *    it's accounted for by this ref counter, otherwise an indirection
 *    via a multiset struct must be used.
 *
 *    When the gc runs, it won't touch a multiset_data block that got
 *    direct external references, i.e. if multiset_data.refs is
 *    greater than the number of multiset objects that points to
 *    it.
 *
 * o  multiset_data.noval_refs is the number of the multiset_data.refs
 *    references that don't lock the value part of the data block.
 *    Thus, values may be changed without copy if (refs - noval_refs)
 *    is 1 (if youself have a value lock) or 0 (if you don't). The
 *    functions below that change only values assume that the caller
 *    has a value lock.
 *
 *    Note that you should not do any operation that might cause a
 *    copy of the data block (which includes insert and delete) when
 *    you've increased noval_refs, since it then won't be possible for
 *    the copying function to know whether one ref in noval_refs
 *    should be moved to the copy or not. All copy operations let
 *    noval_refs stay with the original.
 *
 * o  multiset.node_refs counts the number of references to nodes in
 *    the multiset. The references are array offsets in
 *    multiset.msd->nodes, so the data block may be reallocated but no
 *    nodes may be moved relatively within it. Values but not indices
 *    may be changed when there are node references, nodes may be
 *    added and removed, and the order may be changed.
 *
 *    Nodes that are deleted during nonzero node_refs are linked in
 *    first on the free list as usual, but ind.type is set to
 *    T_DELETED. They are thereby flagged to not be used again.
 *    Deleted nodes on the free list are flagged as free as soon as
 *    possible after all node references are gone. There are however
 *    some combinations of node references and shared data blocks that
 *    results in deleted nodes on the free list even after all node
 *    references are gone.
 *
 *    The prev and ind.u.ptr pointers in deleted nodes point to the
 *    previous and next neighbor, respectively, of the node at the
 *    time it was deleted. Thus the relative position of the node is
 *    remembered even after it has been deleted.
 */

/* The following are compatible with PIKE_WEAK_INDICES and PIKE_WEAK_VALUES. */
#define MULTISET_WEAK_INDICES	2
#define MULTISET_WEAK_VALUES	4
#define MULTISET_WEAK		6

#define MULTISET_INDVAL		8

extern struct multiset *first_multiset;
extern struct multiset *gc_internal_multiset;

PMOD_EXPORT extern struct svalue svalue_int_one;

PMOD_EXPORT void multiset_clear_node_refs (struct multiset *l);

#ifdef PIKE_DEBUG
/* To get good type checking. */
static INLINE union msnode *msnode_check (union msnode *x)
  {return x;}
PMOD_EXPORT extern const char msg_no_multiset_flag_marker[];
#else
#define msnode_check(X) ((union msnode *) (X))
#endif

#define MULTISET_STEP_FUNC(FUNC, NODE)					\
  ((union msnode *) FUNC ((struct rb_node_hdr *) msnode_check (NODE)))
#define low_multiset_first(MSD) MULTISET_STEP_FUNC (rb_first, (MSD)->root)
#define low_multiset_last(MSD) MULTISET_STEP_FUNC (rb_last, (MSD)->root)
#define low_multiset_prev(NODE) MULTISET_STEP_FUNC (rb_prev, NODE)
#define low_multiset_next(NODE) MULTISET_STEP_FUNC (rb_next, NODE)
#define low_multiset_get_nth(MSD, N)					\
  ((union msnode *) rb_get_nth ((struct rb_node_hdr *) (MSD)->root, (N)))
union msnode *low_multiset_find_eq (struct multiset *l, struct svalue *key);

#define low_assign_multiset_index_no_free(TO, NODE) do {		\
    struct svalue *_ms_index_to_ = (TO);				\
    *_ms_index_to_ = msnode_check (NODE)->i.ind;			\
    DO_IF_DEBUG (							\
      if (!(_ms_index_to_->type & MULTISET_FLAG_MARKER))		\
	Pike_fatal (msg_no_multiset_flag_marker);			\
    );									\
    _ms_index_to_->type &= ~MULTISET_FLAG_MASK;				\
    add_ref_svalue (_ms_index_to_);					\
  } while (0)
#define low_assign_multiset_index(TO, NODE) do {			\
    struct svalue *_ms_index_to2_ = (TO);				\
    free_svalue (_ms_index_to2_);					\
    low_assign_multiset_index_no_free (_ms_index_to2_, (NODE));		\
  } while (0)
#define low_push_multiset_index(NODE)					\
  low_assign_multiset_index_no_free (Pike_sp++, (NODE))
#define low_use_multiset_index(NODE, VAR)				\
  ((VAR) = msnode_check (NODE)->i.ind,					\
   DO_IF_DEBUG ((VAR).type & MULTISET_FLAG_MARKER ? 0 :			\
		(Pike_fatal (msg_no_multiset_flag_marker), 0) COMMA)	\
   (VAR).type &= ~MULTISET_FLAG_MASK,					\
   &(VAR))

#define low_get_multiset_value(MSD, NODE)				\
  ((MSD)->flags & MULTISET_INDVAL ? &(NODE)->iv.val : &svalue_int_one)
#define low_set_multiset_value(MSD, NODE, VAL) do {			\
    if ((MSD)->flags & MULTISET_INDVAL)					\
      assign_svalue (&(NODE)->iv.val, VAL);				\
  } while (0)

#define OFF2MSNODE(MSD, OFFSET)						\
  ((MSD)->flags & MULTISET_INDVAL ?					\
   (union msnode *) (&(MSD)->nodes->iv + (OFFSET)) :			\
   (union msnode *) (&(MSD)->nodes->i + (OFFSET)))
#define MSNODE2OFF(MSD, NODE)						\
  ((MSD)->flags & MULTISET_INDVAL ?					\
   &(NODE)->iv - &(MSD)->nodes->iv : &(NODE)->i - &(MSD)->nodes->i)

PMOD_EXPORT INT32 multiset_sizeof (struct multiset *l);
#define l_sizeof(L) multiset_sizeof (L)
#define multiset_ind_types(L) ((L)->msd->ind_types)
#define multiset_val_types(L) ((L)->msd->val_types)
#define multiset_get_flags(L) ((L)->msd->flags)
#define multiset_get_cmp_less(L) (&(L)->msd->cmp_less)
#define multiset_indval(L) ((L)->msd->flags & MULTISET_INDVAL)

/* This is somewhat faster than using multiset_sizeof just to
 * check whether or not the multiset has no elements at all. */
#define multiset_is_empty(L) (!(L)->msd->root)

PMOD_PROTO void really_free_multiset (struct multiset *l);

#define free_multiset(L) do {						\
    struct multiset *_ms_ = (L);					\
    debug_malloc_touch (_ms_);						\
    DO_IF_PIKE_CLEANUP (						\
      if (gc_external_refs_zapped)					\
	gc_check_zapped (_ms_, PIKE_T_MULTISET, __FILE__, __LINE__);	\
    );									\
    if (!sub_ref (_ms_)) really_free_multiset (_ms_);			\
  } while (0)

#ifdef PIKE_DEBUG

union msnode *debug_check_msnode (
  struct multiset *l, ptrdiff_t nodepos, int allow_deleted,
  char *file, int line);
#define check_msnode(L, NODEPOS, ALLOW_DELETED)				\
  debug_check_msnode ((L), (NODEPOS), (ALLOW_DELETED), __FILE__, __LINE__)
#define access_msnode(L, NODEPOS)					\
  check_msnode ((L), (NODEPOS), 0)

#else

#define check_msnode(L, NODEPOS, ALLOW_DELETED)
#define access_msnode(L, NODEPOS) OFF2MSNODE ((L)->msd, (NODEPOS))

#endif

BLOCK_ALLOC_FILL_PAGES (multiset, 2)

/* See rbtree.h for a description of the operations.
 *
 * If cmp_less is used, it's a function pointer used as `< to compare
 * the entries, otherwise the internal set order is used. `< need not
 * define a total order for the possible indices; if neither a < b nor
 * b < a is true then a and b are considered equal orderwise. The
 * order between such indices is arbitrary and stable. The orderwise
 * equality doesn't affect searches on equality, however; if several
 * orderwise equal values are found, then they are searched linearly
 * backwards until one is found which is equal to the key according to
 * `==.
 *
 * It's possible to keep references to individual nodes. The node
 * offset within the multiset data block is used then, which together
 * with the multiset struct can access the node. Use add_msnode_ref
 * when you store a node reference and sub_msnode_ref when you throw
 * it away. The multiset_find_*, multiset_first, multiset_last and
 * multiset_get_nth functions do add_msnode_ref for you (if they
 * return a match). Other functions, like multiset_insert_2, doesn't,
 * even though they might return a node offset.
 *
 * msnode_is_deleted tells whether the referenced node has been
 * deleted. The relative position of a deleted node is remembered by
 * keeping pointers to the neighbors it had when it was deleted. A
 * "defensive" strategy is employed when a deleted node is used in a
 * function: If going forward then the previous neighbor links are
 * followed until a nondeleted neighbor is found, which is then used
 * as the base for the forward movement. Vice versa in the backward
 * direction. This has the effect that if nodes are added and removed
 * in a multiset that is being traversed in some direction, then no
 * newly added nodes in the vicinity of the current one are missed. It
 * also has the effect that the node returned by multiset_next for a
 * deleted node might be before the one returned by multiset_prev.
 *
 * Since the functions might run pike code when comparing entries
 * (even when cmp_less isn't used), the multiset may change during the
 * search in it. If that happens for a destructive operation, it's
 * remade in one way or the other to ensure that the change has been
 * made in the multiset that is current upon return. This normally has
 * no caller visible effects, except for multiset_add_after, which
 * might fail to add the requested entry (it returns less than zero in
 * that case).
 */

/* Returns the node offset, or -1 if no match was found. */
PMOD_EXPORT ptrdiff_t multiset_find_eq (struct multiset *l, struct svalue *key);
PMOD_EXPORT ptrdiff_t multiset_find_lt (struct multiset *l, struct svalue *key);
PMOD_EXPORT ptrdiff_t multiset_find_gt (struct multiset *l, struct svalue *key);
PMOD_EXPORT ptrdiff_t multiset_find_le (struct multiset *l, struct svalue *key);
PMOD_EXPORT ptrdiff_t multiset_find_ge (struct multiset *l, struct svalue *key);
PMOD_EXPORT ptrdiff_t multiset_first (struct multiset *l);
PMOD_EXPORT ptrdiff_t multiset_last (struct multiset *l);
PMOD_EXPORT ptrdiff_t multiset_prev (struct multiset *l, ptrdiff_t nodepos);
PMOD_EXPORT ptrdiff_t multiset_next (struct multiset *l, ptrdiff_t nodepos);

#ifdef PIKE_DEBUG
PMOD_EXPORT extern const char msg_multiset_no_node_refs[];
#endif

#define add_msnode_ref(L) do {(L)->node_refs++;} while (0)
#define sub_msnode_ref(L) do {						\
    struct multiset *_ms_ = (L);					\
    DO_IF_DEBUG (							\
      if (!_ms_->node_refs) Pike_fatal (msg_multiset_no_node_refs);	\
    );									\
    if (!--_ms_->node_refs && _ms_->msd->refs == 1)			\
      multiset_clear_node_refs (_ms_);					\
  } while (0)

PMOD_EXPORT void do_sub_msnode_ref (struct multiset *l);
PMOD_EXPORT int msnode_is_deleted (struct multiset *l, ptrdiff_t nodepos);

#define assign_multiset_index_no_free(TO, L, NODEPOS) do {		\
    struct multiset *_ms_ = (L);					\
    union msnode *_ms_node_ = access_msnode (_ms_, (NODEPOS));		\
    low_assign_multiset_index_no_free (TO, _ms_node_);			\
  } while (0)
#define assign_multiset_index(TO, L, NODEPOS) do {			\
    struct multiset *_ms_ = (L);					\
    union msnode *_ms_node_ = access_msnode (_ms_, (NODEPOS));		\
    low_assign_multiset_index (TO, _ms_node_);				\
  } while (0)
#define push_multiset_index(L, NODEPOS)					\
  assign_multiset_index_no_free (Pike_sp++, (L), (NODEPOS))
#define use_multiset_index(L, NODEPOS, VAR)				\
  ((VAR) = access_msnode ((L), (NODEPOS))->i.ind,			\
   (VAR).type &= ~MULTISET_FLAG_MASK,					\
   &(VAR))

#define get_multiset_value(L, NODEPOS)					\
  ((L)->msd->flags & MULTISET_INDVAL ?					\
   &access_msnode ((L), (NODEPOS))->iv.val : &svalue_int_one)
#define set_multiset_value(L, NODEPOS, VAL) do {			\
    if ((L)->msd->flags & MULTISET_INDVAL)				\
      assign_svalue (&access_msnode ((L), (NODEPOS))->iv.val, VAL);	\
  } while (0)
/* Note: It's intentional that the value is silently ignored for
 * index-only multisets. */

#define assign_multiset_value_no_free(TO, L, NODEPOS)			\
  assign_svalue_no_free (TO, get_multiset_value (L, NODEPOS))
#define assign_multiset_value(TO, L, NODEPOS)				\
  assign_svalue (TO, get_multiset_value (L, NODEPOS))
#define push_multiset_value(L, NODEPOS)					\
  push_svalue (get_multiset_value (L, NODEPOS))

PMOD_EXPORT struct multiset *allocate_multiset (int allocsize,
						int flags,
						struct svalue *cmp_less);
PMOD_EXPORT void do_free_multiset (struct multiset *l);
void multiset_fix_type_field (struct multiset *l);
PMOD_EXPORT void multiset_set_flags (struct multiset *l, int flags);
PMOD_EXPORT void multiset_set_cmp_less (struct multiset *l,
					struct svalue *cmp_less);
PMOD_EXPORT struct multiset *mkmultiset (struct array *indices);
PMOD_EXPORT struct multiset *mkmultiset_2 (struct array *indices,
					   struct array *values,
					   struct svalue *cmp_less);
PMOD_EXPORT void multiset_insert (struct multiset *l,
				  struct svalue *ind);
PMOD_EXPORT ptrdiff_t multiset_insert_2 (struct multiset *l,
					 struct svalue *ind,
					 struct svalue *val,
					 int replace);
PMOD_EXPORT ptrdiff_t multiset_add (struct multiset *l,
				    struct svalue *ind,
				    struct svalue *val);
PMOD_EXPORT ptrdiff_t multiset_add_after (struct multiset *l,
					  ptrdiff_t node,
					  struct svalue *ind,
					  struct svalue *val);
PMOD_EXPORT int multiset_delete (struct multiset *l,
				 struct svalue *ind);
PMOD_EXPORT int multiset_delete_2 (struct multiset *l,
				   struct svalue *ind,
				   struct svalue *removed_val);
PMOD_EXPORT void multiset_delete_node (struct multiset *l,
				       ptrdiff_t node);
PMOD_EXPORT int multiset_member (struct multiset *l,
				 struct svalue *key);
PMOD_EXPORT struct svalue *multiset_lookup (struct multiset *l,
					    struct svalue *key);
struct array *multiset_indices (struct multiset *l);
struct array *multiset_values (struct multiset *l);
struct array *multiset_range_indices (struct multiset *l,
				      ptrdiff_t beg, ptrdiff_t end);
struct array *multiset_range_values (struct multiset *l,
				     ptrdiff_t beg, ptrdiff_t end);
PMOD_EXPORT void check_multiset_for_destruct (struct multiset *l);
PMOD_EXPORT struct multiset *copy_multiset (struct multiset *l);
PMOD_EXPORT struct multiset *merge_multisets (struct multiset *a,
					      struct multiset *b,
					      int operation);
PMOD_EXPORT struct multiset *add_multisets (struct svalue *argp, int count);
PMOD_EXPORT int multiset_equal_p (struct multiset *a, struct multiset *b,
				  struct processing *p);
void describe_multiset (struct multiset *l, struct processing *p, int indent);
void simple_describe_multiset (struct multiset *l);
int multiset_is_constant (struct multiset *l, struct processing *p);
node *make_node_from_multiset (struct multiset *l);
PMOD_EXPORT void f_aggregate_multiset (int args);
struct multiset *copy_multiset_recursively (struct multiset *l,
					    struct mapping *p);
PMOD_EXPORT ptrdiff_t multiset_get_nth (struct multiset *l, size_t n);

unsigned gc_touch_all_multisets (void);
void gc_check_all_multisets (void);
void gc_mark_multiset_as_referenced (struct multiset *l);
void gc_mark_all_multisets (void);
void gc_zap_ext_weak_refs_in_multisets (void);
void real_gc_cycle_check_multiset (struct multiset *l, int weak);
void gc_cycle_check_all_multisets (void);
size_t gc_free_all_unreferenced_multisets (void);
#define gc_cycle_check_multiset(X, WEAK) \
  gc_cycle_enqueue ((gc_cycle_check_cb *) real_gc_cycle_check_multiset, (X), (WEAK))

#ifdef PIKE_DEBUG
void check_multiset (struct multiset *l, int safe);
void check_all_multisets (int safe);
void debug_dump_multiset (struct multiset *l);
#endif

void count_memory_in_multisets (INT32 *num, INT32 *size);
void init_multiset (void);
void exit_multiset (void);
void test_multiset (void);

#endif /* MULTISET_H */
