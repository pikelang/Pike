/* An implementation of a threaded red/black balanced binary tree.
 *
 * Created 2001-04-27 by Martin Stjernholm
 *
 * $Id: rbtree.h,v 1.1 2001/04/30 17:30:25 mast Exp $
 */

#ifndef RBTREE_H
#define RBTREE_H

/* A red/black tree is a binary tree with one extra bit of info in
 * each node - the color of it. The following properties holds:
 *
 * o  Every node is either red or black.
 * o  A NULL leaf is considered black.
 * o  If a node is red, its children must be black.
 * o  Every path from a node down to all its leafs have the same
 *    number of black nodes.
 *
 * The longest possible path in a given tree thus have alternating red
 * and black nodes, and the shortest possible path in it have only
 * black nodes. Therefore it's guaranteed that the longest path is at
 * most twice the length as the shortest one.
 */

struct rb_node_hdr {
  struct rb_node_hdr *prev, *next;
  unsigned INT16 flags;		/* Overlays svalue type field in
				 * rb_node_ind and rb_node_indval. */
};

/* Note: Don't access the ind svalue (or at least not its type field)
 * in the following directly, since the flags above overlay that. Use
 * assign_rb_node_ind_no_free() instead. */

struct rb_node_ind {
  struct rb_node_ind *prev, *next; /* Must be first. */
  struct svalue ind;
};

struct rb_node_indval {
  struct rb_node_indval *prev, *next; /* Must be first. */
  struct svalue ind, val;
};

union rb_node {
  struct rb_node_hdr h;
  struct rb_node_ind i;
  struct rb_node_indval iv;
};

#define RB_RED		0x2000
#define RB_THREAD_PREV	0x4000
#define RB_THREAD_NEXT	0x8000
#define RB_FLAG_MASK	0xe000

/* The thread flags indicate whether the prev/next pointers are thread
 * pointers. A thread pointer is used whenever the pointer would
 * otherwise be NULL to point to next smaller/bigger element. More
 * specifically, the next thread pointer points to the closest parent
 * node whose prev pointer subtree contains it, and vice versa for the
 * prev thread pointer:
 *
 * 		 p <.                                  .> p
 * 		/    .                                .    \
 * 	       /      .                              .      \
 * 	      a        .                            .        a
 * 	     / \        .                          .        / \
 * 		\        .                        .        /
 * 		 b        .                      .        b
 * 		/ \       . <- next      prev -> .       / \
 * 		  ...     .    thread pointer    .     ...
 * 		    \     .                      .     /
 * 		     c   .                        .   c
 * 		    / \..                          ../ \
 */

#define RB_FLAG_MARKER	0x1000
#define RB_IND_FLAG_MASK 0xf000
/* The flag marker is set to nail direct uses of the ind type field. */

/* Operations:
 *
 * insert:
 *     Adds a new entry only if one with the same index doesn't exist
 *     already, replaces it otherwise. If a node was added it's
 *     returned, otherwise zero is returned.
 *
 * add:
 *     Adds a new entry, even if one with the same index already
 *     exists. Returns the newly created node.
 *
 * add_after:
 *     Adds a new entry after the given one. Returns the newly created
 *     node.
 *
 * delete:
 *     Deletes an arbitrary entry with the specified index, if one
 *     exists. Returns nonzero in that case, zero otherwise.
 *
 * delete_node:
 *     Deletes the given node from the tree. Useful to get the right
 *     entry when several have the same index. The node is assumed to
 *     exist in the tree. Note that it's a linear search to get the
 *     right node among several with the same index.
 *
 * find_eq:
 *     Returns an arbitrary entry which have the given index, or zero
 *     if none exists.
 *
 * find_lt, find_gt, find_le, find_ge:
 *     find_lt and find_le returns the biggest entry which satisfy the
 *     condition, and vice versa for the other two. This means that
 *     e.g. rb_next when used on the returned node from find_le never
 *     returns an entry with the same index.
 *
 * If cmp_less is nonzero, it's a function pointer used as `< to
 * compare the entries. If it's zero the internal set order is used.
 * All destructive operations might change the tree root.
 */

/* Functions for handling nodes with index only. */
PMOD_EXPORT struct rb_node_ind *rb_ind_insert (struct rb_node_ind **tree,
					       struct svalue *ind,
					       struct svalue *cmp_less);
PMOD_EXPORT struct rb_node_ind *rb_ind_add (struct rb_node_ind **tree,
					    struct svalue *ind,
					    struct svalue *cmp_less);
PMOD_EXPORT int rb_ind_delete (struct rb_node_ind **tree,
			       struct svalue *ind,
			       struct svalue *cmp_less);
PMOD_EXPORT struct rb_node_ind *rb_ind_copy (struct rb_node_ind *tree);
PMOD_EXPORT void rb_ind_free (struct rb_node_ind *tree);

/* Functions for handling nodes with index and value. */
PMOD_EXPORT struct rb_node_indval *rb_indval_insert (struct rb_node_indval **tree,
						     struct svalue *ind,
						     struct svalue *val,
						     struct svalue *cmp_less);
PMOD_EXPORT struct rb_node_indval *rb_indval_add (struct rb_node_indval **tree,
						  struct svalue *ind,
						  struct svalue *val,
						  struct svalue *cmp_less);
PMOD_EXPORT struct rb_node_indval *rb_indval_add_after (struct rb_node_indval **tree,
							struct rb_node_indval *node,
							struct svalue *ind,
							struct svalue *val,
							struct svalue *cmp_less);
PMOD_EXPORT int rb_indval_delete (struct rb_node_indval **tree,
				  struct svalue *ind,
				  struct svalue *cmp_less);
PMOD_EXPORT struct rb_node_indval *rb_indval_delete_node (struct rb_node_indval **tree,
							  struct rb_node_indval *node,
							  struct svalue *cmp_less);
PMOD_EXPORT struct rb_node_indval *rb_indval_copy (struct rb_node_indval *tree);
PMOD_EXPORT void rb_indval_free (struct rb_node_indval *tree);

/* Functions for handling both types of nodes. */
PMOD_EXPORT union rb_node *rb_find_eq (union rb_node *tree, struct svalue *key,
				       struct svalue *cmp_less);
PMOD_EXPORT union rb_node *rb_find_lt (union rb_node *tree, struct svalue *key,
				       struct svalue *cmp_less);
PMOD_EXPORT union rb_node *rb_find_gt (union rb_node *tree, struct svalue *key,
				       struct svalue *cmp_less);
PMOD_EXPORT union rb_node *rb_find_le (union rb_node *tree, struct svalue *key,
				       struct svalue *cmp_less);
PMOD_EXPORT union rb_node *rb_find_ge (union rb_node *tree, struct svalue *key,
				       struct svalue *cmp_less);

/* Functions for handling any type of node. */
PMOD_EXPORT struct rb_node_hdr *rb_first (struct rb_node_hdr *tree);
PMOD_EXPORT struct rb_node_hdr *rb_last (struct rb_node_hdr *tree);
PMOD_EXPORT struct rb_node_hdr *rb_prev (struct rb_node_hdr *node);
PMOD_EXPORT struct rb_node_hdr *rb_next (struct rb_node_hdr *node);

#define RB_IND_FIND_FUNC(func, tree, key, cmp_less) \
  ((struct rb_node_ind *) func ((union rb_node *) (tree), (key), (cmp_less)))
#define RB_IND_STEP_FUNC(func, tree) \
  ((struct rb_node_ind *) func ((struct rb_node_hdr *) (tree)))

#define rb_ind_find_eq(tree, key, cmp_less) \
  RB_IND_FIND_FUNC (rb_find_eq, tree, key, cmp_less)
#define rb_ind_find_lt(tree, key, cmp_less) \
  RB_IND_FIND_FUNC (rb_find_lt, tree, key, cmp_less)
#define rb_ind_find_gt(tree, key, cmp_less) \
  RB_IND_FIND_FUNC (rb_find_gt, tree, key, cmp_less)
#define rb_ind_find_le(tree, key, cmp_less) \
  RB_IND_FIND_FUNC (rb_find_le, tree, key, cmp_less)
#define rb_ind_find_ge(tree, key, cmp_less) \
  RB_IND_FIND_FUNC (rb_find_ge, tree, key, cmp_less)
#define rb_ind_first(tree) RB_IND_STEP_FUNC (rb_first, tree)
#define rb_ind_last(tree) RB_IND_STEP_FUNC (rb_last, tree)
#define rb_ind_prev(tree) RB_IND_STEP_FUNC (rb_prev, tree)
#define rb_ind_next(tree) RB_IND_STEP_FUNC (rb_next, tree)

#define RB_INDVAL_FIND_FUNC(func, tree, key, cmp_less) \
  ((struct rb_node_indval *) func ((union rb_node *) (tree), (key), (cmp_less)))
#define RB_INDVAL_STEP_FUNC(func, tree) \
  ((struct rb_node_indval *) func ((struct rb_node_hdr *) (tree)))

#define rb_indval_find_eq(tree, key, cmp_less) \
  RB_INDVAL_FIND_FUNC (rb_find_eq, tree, key, cmp_less)
#define rb_indval_find_lt(tree, key, cmp_less) \
  RB_INDVAL_FIND_FUNC (rb_find_lt, tree, key, cmp_less)
#define rb_indval_find_gt(tree, key, cmp_less) \
  RB_INDVAL_FIND_FUNC (rb_find_gt, tree, key, cmp_less)
#define rb_indval_find_le(tree, key, cmp_less) \
  RB_INDVAL_FIND_FUNC (rb_find_le, tree, key, cmp_less)
#define rb_indval_find_ge(tree, key, cmp_less) \
  RB_INDVAL_FIND_FUNC (rb_find_ge, tree, key, cmp_less)
#define rb_indval_first(tree) RB_INDVAL_STEP_FUNC (rb_first, tree)
#define rb_indval_last(tree) RB_INDVAL_STEP_FUNC (rb_last, tree)
#define rb_indval_prev(tree) RB_INDVAL_STEP_FUNC (rb_prev, tree)
#define rb_indval_next(tree) RB_INDVAL_STEP_FUNC (rb_next, tree)

/* Accessing the index svalue in a node. */
#define assign_rb_node_ind_no_free(to, node) do {			\
  struct svalue *_rb_node_to = (to);					\
  *_rb_node_to = (node)->ind;						\
  _rb_node_to->type &= ~RB_IND_FLAG_MASK;				\
  add_ref_svalue (_rb_node_to);						\
} while (0)

#define assign_rb_node_ind(to, node) do {				\
  struct svalue *_rb_node_to2 = (to);					\
  free_svalue (_rb_node_to2);						\
  assign_rb_node_ind_no_free (_rb_node_to2, (node));			\
} while (0)

#define push_rb_node_ind(node) do {					\
  assign_rb_node_ind_no_free (Pike_sp, (node));				\
  Pike_sp++;								\
} while (0)

#define use_rb_node_ind(node, var)					\
  (var = (node)->ind, var.type &= ~RB_IND_FLAG_MASK, var)

/* Low-level tools, for use with arbitrary data. These only requires
 * that the nodes begin with the rb_node_hdr struct. */

/* A sliced stack is used to track the way down in a tree, so we can
 * back up again easily while rebalancing it. The first slice is
 * allocated on the C stack. */

#define STACK_SLICE_SIZE 20
/* This is in the worst possible case enough for trees of size
 * 2^(20/2) = 1024 before allocating another slice, but in the typical
 * case it's enough for trees with up to about 2^(20-1) = 524288
 * elements. */

struct rb_stack_slice
{
  struct rb_stack_slice *up;
  struct rb_node_hdr *stack[STACK_SLICE_SIZE];
};

#define RBSTACK_INIT(slice, ssp)					\
  struct rb_stack_slice PIKE_CONCAT3 (_top_, slice, _);			\
  struct rb_stack_slice *slice =					\
    (PIKE_CONCAT3 (_top_, slice, _).up = 0,				\
     &PIKE_CONCAT3 (_top_, slice, _));					\
  size_t ssp = 0 /* Only zero when the stack is empty. */

#define RBSTACK_PUSH(slice, ssp, node) do {				\
  if (ssp < STACK_SLICE_SIZE)						\
    slice->stack[ssp++] = (node);					\
  else {								\
    struct rb_stack_slice *_new_ = ALLOC_STRUCT (rb_stack_slice);	\
    _new_->up = slice;							\
    slice = _new_;							\
    slice->stack[0] = (node);						\
    ssp = 1;								\
  }									\
} while (0)

#define RBSTACK_POP(slice, ssp, node) do {				\
  if (ssp) {								\
    (node) = slice->stack[--ssp];					\
    if (!ssp && slice->up) {						\
      struct rb_stack_slice *_old_ = slice;				\
      slice = slice->up;						\
      ssp = STACK_SLICE_SIZE;						\
      xfree (_old_);							\
    }									\
  }									\
  else (node) = 0;							\
} while (0)

#define RBSTACK_POP_IGNORE(slice, ssp) do {				\
  if (ssp && !--ssp && slice->up) {					\
    struct rb_stack_slice *_old_ = slice;				\
    slice = slice->up;							\
    ssp = STACK_SLICE_SIZE;						\
    xfree (_old_);							\
  }									\
} while (0)

#define RBSTACK_UP(slice, ssp, node) do {				\
  if (ssp) {								\
    (node) = slice->stack[--ssp];					\
    if (!ssp && slice->up) {						\
      slice = slice->up;						\
      ssp = STACK_SLICE_SIZE;						\
    }									\
  }									\
  else (node) = 0;							\
} while (0)

#define RBSTACK_PEEK(slice, ssp, node) do {				\
  if (ssp) (node) = slice->stack[ssp - 1];				\
  else (node) = 0;							\
} while (0)

#define RBSTACK_FREE(slice) do {					\
  while (slice->up) {							\
    struct rb_stack_slice *_old_ = slice;				\
    slice = slice->up;							\
    xfree (_old_);							\
  }									\
} while (0)

#define RBSTACK_FREE_SET_ROOT(slice, ssp, node) do {			\
  if (ssp) {								\
    while (slice->up) {							\
      struct rb_stack_slice *_old_ = slice;				\
      slice = slice->up;						\
      xfree (_old_);							\
    }									\
    (node) = slice->stack[0];						\
  }									\
} while (0)

/* Traverses the tree in depth-first order:
 * push		Run when entering a node.
 * p_leaf	Run when the nodes prev pointer isn't a subtree.
 * p_sub	Run when the nodes prev pointer is a subtree.
 * between	Run after the prev subtree has been recursed and before
 *		the next subtree is examined.
 * n_leaf	Run when the nodes next pointer isn't a subtree.
 * n_sub	Run when the nodes next pointer is a subtree.
 * pop		Run when leaving a node.
 */
#define LOW_RB_TRAVERSE(label, slice, ssp, node, push, p_leaf, p_sub, between, n_leaf, n_sub, pop) \
do {									\
  struct rb_node_hdr *low_rb_last_;					\
  if (node) {								\
    PIKE_CONCAT (enter_, label):					\
    {push;}								\
    if ((node)->flags & RB_THREAD_PREV)					\
      {p_leaf;}								\
    else {								\
      {p_sub;}								\
      RBSTACK_PUSH (slice, ssp, node);					\
      (node) = (node)->prev;						\
      goto PIKE_CONCAT (enter_, label);					\
    }									\
    PIKE_CONCAT (between_, label):					\
    {between;}								\
    if ((node)->flags & RB_THREAD_NEXT)					\
      {n_leaf;}								\
    else {								\
      {n_sub;}								\
      RBSTACK_PUSH (slice, ssp, node);					\
      (node) = (node)->next;						\
      goto PIKE_CONCAT (enter_, label);					\
    }									\
    PIKE_CONCAT (leave_, label):					\
    while (1) {								\
      {pop;}								\
      low_rb_last_ = (node);						\
      RBSTACK_POP (slice, ssp, node);					\
      if (!(node)) break;						\
      if (low_rb_last_ == (node)->prev)					\
	goto PIKE_CONCAT (between_, label);				\
    }									\
  }									\
} while (0)

#define LOW_RB_DEBUG_TRAVERSE(label, slice, ssp, node, push, p_leaf, p_sub, between, n_leaf, n_sub, pop) \
do {									\
  size_t PIKE_CONCAT (depth_, label) = 0;				\
  LOW_RB_TRAVERSE(							\
    label, slice, ssp, node,						\
    fprintf (stderr, "%*s%p enter\n",					\
	     PIKE_CONCAT (depth_, label)++, "", node); {push;},		\
    fprintf (stderr, "%*s%p prev leaf\n",				\
	     PIKE_CONCAT (depth_, label), "", node); {p_leaf;},		\
    fprintf (stderr, "%*s%p prev subtree\n",				\
	     PIKE_CONCAT (depth_, label), "", node); {p_sub;},		\
    fprintf (stderr, "%*s%p between\n",					\
	     PIKE_CONCAT (depth_, label) - 1, "", node); {between;},	\
    fprintf (stderr, "%*s%p next leaf\n",				\
	     PIKE_CONCAT (depth_, label), "", node); {n_leaf;},		\
    fprintf (stderr, "%*s%p next subtree\n",				\
	     PIKE_CONCAT (depth_, label), "", node); {n_sub;},		\
    fprintf (stderr, "%*s%p leave\n",					\
	     --PIKE_CONCAT (depth_, label), "", node); {pop;});		\
} while (0)

#define LOW_RB_FIND(node, cmp, got_lt, got_eq, got_gt)			\
do {									\
  int cmp_res;								\
  while (1) {								\
    DO_IF_DEBUG (if (!node) fatal ("Recursing into null node.\n"));	\
    {cmp;}								\
    if (cmp_res > 0) {							\
      if ((node)->flags & RB_THREAD_NEXT) {				\
	{got_lt;}							\
	break;								\
      }									\
      (node) = (node)->next;						\
    }									\
    else if (cmp_res < 0) {						\
      if ((node)->flags & RB_THREAD_PREV) {				\
	{got_gt;}							\
	break;								\
      }									\
      (node) = (node)->prev;						\
    }									\
    else {								\
      {got_eq;}								\
      break;								\
    }									\
  }									\
} while (0)

/* Tracks the way down a tree to a specific node and updates the stack
 * as necessary for low_rb_link_* and low_rb_unlink. */
#define LOW_RB_TRACK(slice, ssp, node, cmp, got_lt, got_eq, got_gt)	\
  LOW_RB_FIND (node, {							\
    RBSTACK_PUSH (slice, ssp, node);					\
    {cmp;}								\
  }, got_lt, got_eq, got_gt)

/* Goes to the next node in order while keeping the stack updated. */
#define LOW_RB_TRACK_NEXT(slice, ssp, node)				\
do {									\
  DO_IF_DEBUG ({							\
    struct rb_node_hdr *low_rb_stack_top_;				\
    RBSTACK_PEEK (slice, ssp, low_rb_stack_top_);			\
    if (node != low_rb_stack_top_)					\
      fatal ("Given node is not on top of stack.\n");			\
  });									\
  if (node->flags & RB_THREAD_NEXT) {					\
    struct rb_node_hdr *low_rb_next_ = node->next;			\
    while (1) {								\
      RBSTACK_PEEK (slice, ssp, node);					\
      if (node == low_rb_next_) break;					\
      RBSTACK_POP_IGNORE (slice, ssp);					\
    }									\
  }									\
  else {								\
    node = node->next;							\
    RBSTACK_PUSH (slice, ssp, node);					\
    while (!(node->flags & RB_THREAD_PREV)) {				\
      node = node->prev;						\
      RBSTACK_PUSH (slice, ssp, node);					\
    }									\
  }									\
} while (0)

typedef int low_rb_cmp_fn (void *key, struct rb_node_hdr *node);

/* Used when unlinking nodes. Since an interior node can't be unlinked
 * we need to move over the data from a leaf node and unlink that
 * instead. */
typedef void low_rb_move_data_fn (struct rb_node_hdr *to,
				  struct rb_node_hdr *from);

typedef struct rb_node_hdr *low_rb_alloc_copy_fn (struct rb_node_hdr *node);

void low_rb_init_root (struct rb_node_hdr *new_root);
void low_rb_link_at_prev (struct rb_node_hdr **tree,
			  struct rb_stack_slice *slice, size_t ssp,
			  struct rb_node_hdr *new);
void low_rb_link_at_next (struct rb_node_hdr **tree,
			  struct rb_stack_slice *slice, size_t ssp,
			  struct rb_node_hdr *new);
struct rb_node_hdr *low_rb_unlink (struct rb_node_hdr **tree,
				   struct rb_stack_slice *slice, size_t ssp,
				   low_rb_move_data_fn *move_data);

struct rb_node_hdr *low_rb_insert (struct rb_node_hdr **tree,
				   low_rb_cmp_fn *cmpfun, void *key,
				   struct rb_node_hdr *new);
void low_rb_add (struct rb_node_hdr **tree,
		 low_rb_cmp_fn *cmpfun, void *key,
		 struct rb_node_hdr *new);
void low_rb_add_after (struct rb_node_hdr **tree,
		       low_rb_cmp_fn *cmpfun, void *key,
		       struct rb_node_hdr *new,
		       struct rb_node_hdr *existing);
struct rb_node_hdr *low_rb_delete (struct rb_node_hdr **tree,
				   low_rb_cmp_fn *cmpfun, void *key,
				   low_rb_move_data_fn *move_data);
struct rb_node_hdr *low_rb_delete_node (struct rb_node_hdr **tree,
					low_rb_cmp_fn *cmpfun, void *key,
					low_rb_move_data_fn *move_data,
					struct rb_node_hdr *node);
struct rb_node_hdr *low_rb_copy (struct rb_node_hdr *tree,
				 low_rb_alloc_copy_fn *alloc_copy);

struct rb_node_hdr *low_rb_find_eq (struct rb_node_hdr *tree,
				    low_rb_cmp_fn *cmpfun, void *key);
struct rb_node_hdr *low_rb_find_lt (struct rb_node_hdr *tree,
				    low_rb_cmp_fn *cmpfun, void *key);
struct rb_node_hdr *low_rb_find_gt (struct rb_node_hdr *tree,
				    low_rb_cmp_fn *cmpfun, void *key);
struct rb_node_hdr *low_rb_find_le (struct rb_node_hdr *tree,
				    low_rb_cmp_fn *cmpfun, void *key);
struct rb_node_hdr *low_rb_find_ge (struct rb_node_hdr *tree,
				    low_rb_cmp_fn *cmpfun, void *key);

#ifdef PIKE_DEBUG
void debug_check_rb_tree (struct rb_node_hdr *tree);
#endif

void init_rbtree (void);
void exit_rbtree (void);

#endif /* RBTREE_H */
