/* An implementation of a threaded red/black balanced binary tree.
 *
 * Created 2001-04-27 by Martin Stjernholm
 *
 * $Id: rbtree.c,v 1.3 2001/05/01 07:52:19 mast Exp $
 */

#include "global.h"
#include "constants.h"
#include "builtin_functions.h"
#include "interpret.h"
#include "mapping.h"
#include "pike_error.h"
#include "svalue.h"
#include "rbtree.h"
#include "block_alloc.h"

/* #define TEST_RBTREE */

#if defined (PIKE_DEBUG) || defined (TEST_RBTREE)

typedef void dump_data_fn (struct rb_node_hdr *node);
static void debug_dump_ind_data (struct rb_node_ind *node);
static void debug_dump_rb_tree (struct rb_node_hdr *tree, dump_data_fn *dump_data);

DECLSPEC(noreturn) static void debug_rb_fatal (struct rb_node_hdr *tree, const char *fmt, ...) ATTRIBUTE((noreturn, format (printf, 2, 3)));
DECLSPEC(noreturn) static void debug_rb_ind_fatal (struct rb_node_ind *tree, const char *fmt, ...) ATTRIBUTE((noreturn, format (printf, 2, 3)));
DECLSPEC(noreturn) static void debug_rb_indval_fatal (struct rb_node_indval *tree, const char *fmt, ...) ATTRIBUTE((noreturn, format (printf, 2, 3)));
#define rb_fatal (fprintf (stderr, "%s:%d: Fatal in rbtree: ", \
			   __FILE__, __LINE__), debug_rb_fatal)
#define rb_ind_fatal (fprintf (stderr, "%s:%d: Fatal in ind rbtree: ", \
			       __FILE__, __LINE__), debug_rb_ind_fatal)
#define rb_indval_fatal (fprintf (stderr, "%s:%d: Fatal in indval rbtree: ", \
				  __FILE__, __LINE__), debug_rb_indval_fatal)

#endif

#define HDR(node) ((struct rb_node_hdr *) (node))

/* The default compare function for ind and indval. */
PMOD_EXPORT int rb_ind_default_cmp (struct svalue *key, union rb_node *node)
{
  struct svalue tmp;
  return set_svalue_cmpfun (key, &use_rb_node_ind (&(node->i), tmp));
}

struct svalue_cmp_data
{
  struct svalue *key, *cmp_less;
};

static int svalue_cmp_eq (struct svalue_cmp_data *data, union rb_node *node)
{
  int cmp_res;
  struct svalue tmp;
  push_svalue (data->key);
  push_svalue (&use_rb_node_ind (&(node->i), tmp));
  apply_svalue (data->cmp_less, 2);
  if (IS_ZERO (sp - 1))
    cmp_res = !is_eq (data->key, &tmp);
  else
    cmp_res = -1;
  pop_stack();
  return cmp_res;
}

/* Considers equal as greater. */
static int svalue_cmp_ge (struct svalue_cmp_data *data, union rb_node *node)
{
  int cmp_res;
  struct svalue tmp;
  push_svalue (data->key);
  push_svalue (&use_rb_node_ind (&(node->i), tmp));
  apply_svalue (data->cmp_less, 2);
  cmp_res = IS_ZERO (sp - 1) ? 1 : -1;
  pop_stack();
  return cmp_res;
}

/* Considers equal as less. */
static int svalue_cmp_le (struct svalue_cmp_data *data, union rb_node *node)
{
  int cmp_res;
  struct svalue tmp;
  push_svalue (&use_rb_node_ind (&(node->i), tmp));
  push_svalue (data->key);
  apply_svalue (data->cmp_less, 2);
  cmp_res = IS_ZERO (sp - 1) ? -1 : 1;
  pop_stack();
  return cmp_res;
}

/* Functions for handling nodes with index only. */

BLOCK_ALLOC (rb_node_ind, 1024)

static void move_ind_data (union rb_node *to, union rb_node *from)
{
  INT16 flags = to->h.flags & RB_IND_FLAG_MASK;
  to->i.ind = from->i.ind;
  to->h.flags = (to->h.flags & ~RB_IND_FLAG_MASK) | flags;
}

PMOD_EXPORT struct rb_node_ind *rb_ind_insert (struct rb_node_ind **tree,
					       struct svalue *ind,
					       struct svalue *cmp_less)
{
  struct rb_node_ind *node;
  struct svalue tmp;

  if (cmp_less) {
    struct svalue_cmp_data data;
    data.cmp_less = cmp_less;
    data.key = ind;
    LOW_RB_INSERT ((struct rb_node_hdr **) tree, HDR (node), { /* Compare. */
      cmp_res = svalue_cmp_eq (&data, (union rb_node *) node);
    }, {			/* Insert. */
      node = alloc_rb_node_ind();
      assign_svalue_no_free (&node->ind, ind);
      DO_IF_DEBUG (node->ind.type |= RB_FLAG_MARKER);
    }, {			/* Replace. */
      node = 0;
    });
  }

  else {
    LOW_RB_INSERT ((struct rb_node_hdr **) tree, HDR (node), { /* Compare. */
      cmp_res = set_svalue_cmpfun (ind, &use_rb_node_ind (node, tmp));
    }, {			/* Insert. */
      node = alloc_rb_node_ind();
      assign_svalue_no_free (&node->ind, ind);
      DO_IF_DEBUG (node->ind.type |= RB_FLAG_MARKER);
    }, {			/* Replace. */
      node = 0;
    });
  }

  return node;
}

PMOD_EXPORT struct rb_node_ind *rb_ind_add (struct rb_node_ind **tree,
					    struct svalue *ind,
					    struct svalue *cmp_less)
{
  struct rb_node_ind *new = alloc_rb_node_ind();
  assign_svalue_no_free (&new->ind, ind);
#ifdef PIKE_DEBUG
  new->ind.type |= RB_FLAG_MARKER;
#endif
  if (cmp_less) {
    struct svalue_cmp_data data;
    data.cmp_less = cmp_less;
    data.key = ind;
    low_rb_add ((struct rb_node_hdr **) tree,
		(low_rb_cmp_fn *) svalue_cmp_eq, &data,
		HDR (new));
  }
  else
    low_rb_add ((struct rb_node_hdr **) tree,
		(low_rb_cmp_fn *) rb_ind_default_cmp, ind,
		HDR (new));
  return new;
}

PMOD_EXPORT int rb_ind_delete (struct rb_node_ind **tree,
			       struct svalue *ind,
			       struct svalue *cmp_less)
{
  struct rb_node_ind *old;
  if (cmp_less) {
    struct svalue_cmp_data data;
    data.cmp_less = cmp_less;
    data.key = ind;
    HDR (old) = low_rb_delete ((struct rb_node_hdr **) tree,
			       (low_rb_cmp_fn *) svalue_cmp_eq, &data,
			       (low_rb_move_data_fn *) move_ind_data);
  }
  else
    HDR (old) = low_rb_delete ((struct rb_node_hdr **) tree,
			       (low_rb_cmp_fn *) rb_ind_default_cmp, ind,
			       (low_rb_move_data_fn *) move_ind_data);
  if (old) {
    struct svalue tmp;
    free_svalue (&use_rb_node_ind (old, tmp));
    really_free_rb_node_ind (old);
    return 1;
  }
  return 0;
}

static struct rb_node_ind *copy_ind_node (struct rb_node_ind *node)
{
  struct rb_node_ind *new = alloc_rb_node_ind();
  struct svalue tmp;
  assign_svalue_no_free (&new->ind, &use_rb_node_ind (node, tmp));
  DO_IF_DEBUG (new->ind.type |= RB_FLAG_MARKER);
  return new;
}

PMOD_EXPORT struct rb_node_ind *rb_ind_copy (struct rb_node_ind *tree)
{
  return (struct rb_node_ind *)
    low_rb_copy (HDR (tree), (low_rb_copy_fn *) copy_ind_node);
}

PMOD_EXPORT void rb_ind_free (struct rb_node_ind *tree)
{
  RBSTACK_INIT (rbstack);
  LOW_RB_TRAVERSE (1, rbstack, HDR (tree), ;, ;, ;, ;, ;, ;, {
    /* Pop. */
    tree->ind.type &= ~RB_IND_FLAG_MASK;
    free_svalue (&tree->ind);
    really_free_rb_node_ind (tree);
  });
}

/* Functions for handling nodes with index and value. */

BLOCK_ALLOC (rb_node_indval, 1024)

static void move_indval_data (union rb_node *to, union rb_node *from)
{
  INT16 flags = to->h.flags & RB_IND_FLAG_MASK;
  to->iv.ind = from->iv.ind;
  to->iv.val = from->iv.val;
  to->h.flags = (to->h.flags & ~RB_IND_FLAG_MASK) | flags;
}

PMOD_EXPORT struct rb_node_indval *rb_indval_insert (struct rb_node_indval **tree,
						     struct svalue *ind,
						     struct svalue *val,
						     struct svalue *cmp_less)
{
  struct rb_node_indval *node;
  struct svalue tmp;

  if (cmp_less) {
    struct svalue_cmp_data data;
    data.cmp_less = cmp_less;
    data.key = ind;
    LOW_RB_INSERT ((struct rb_node_hdr **) tree, HDR (node), { /* Compare. */
      cmp_res = svalue_cmp_eq (&data, (union rb_node *) node);
    }, {			/* Insert. */
      node = alloc_rb_node_indval();
      assign_svalue_no_free (&node->ind, ind);
      assign_svalue_no_free (&node->val, val);
      DO_IF_DEBUG (node->ind.type |= RB_FLAG_MARKER);
    }, {			/* Replace. */
      node = 0;
    });
  }

  else {
    LOW_RB_INSERT ((struct rb_node_hdr **) tree, HDR (node), { /* Compare. */
      cmp_res = set_svalue_cmpfun (ind, &use_rb_node_ind (node, tmp));
    }, {			/* Insert. */
      node = alloc_rb_node_indval();
      assign_svalue_no_free (&node->ind, ind);
      assign_svalue_no_free (&node->val, val);
      DO_IF_DEBUG (node->ind.type |= RB_FLAG_MARKER);
    }, {			/* Replace. */
      node = 0;
    });
  }

  return node;
}

PMOD_EXPORT struct rb_node_indval *rb_indval_add (struct rb_node_indval **tree,
						  struct svalue *ind,
						  struct svalue *val,
						  struct svalue *cmp_less)
{
  struct rb_node_indval *new = alloc_rb_node_indval();
  assign_svalue_no_free (&new->ind, ind);
  assign_svalue_no_free (&new->val, val);
#ifdef PIKE_DEBUG
  new->ind.type |= RB_FLAG_MARKER;
#endif
  if (cmp_less) {
    struct svalue_cmp_data data;
    data.cmp_less = cmp_less;
    data.key = ind;
    low_rb_add ((struct rb_node_hdr **) tree,
		(low_rb_cmp_fn *) svalue_cmp_eq, &data,
		HDR (new));
  }
  else
    low_rb_add ((struct rb_node_hdr **) tree,
		(low_rb_cmp_fn *) rb_ind_default_cmp, ind,
		HDR (new));
  return new;
}

PMOD_EXPORT struct rb_node_indval *rb_indval_add_after (struct rb_node_indval **tree,
							struct rb_node_indval *node,
							struct svalue *ind,
							struct svalue *val,
							struct svalue *cmp_less)
{
  struct rb_node_indval *new = alloc_rb_node_indval();
  assign_svalue_no_free (&new->ind, ind);
  assign_svalue_no_free (&new->val, val);

#ifdef PIKE_DEBUG
  new->ind.type |= RB_FLAG_MARKER;

  if (*tree) {			/* Check that it won't break the order. */
    struct rb_node_indval *tmpnode;
    struct svalue tmp;
    int cmp1, cmp2;

#define DO_CMP(a, b, cmp)						\
    do {								\
      if (cmp_less) {							\
	push_svalue (a);						\
	push_svalue (b);						\
	apply_svalue (cmp_less, 2);					\
	if (IS_ZERO (sp - 1))						\
	  cmp = !is_eq (a, b);						\
	else								\
	  cmp = -1;							\
	pop_stack();							\
      }									\
      else cmp = set_svalue_cmpfun (a, b);				\
    } while (0)

    if (node) {
      DO_CMP (ind, &use_rb_node_ind (node, tmp), cmp1);
      tmpnode = rb_indval_next (node);
      if (tmpnode) DO_CMP (ind, &use_rb_node_ind (tmpnode, tmp), cmp2);
      else cmp2 = -1;
      if (cmp1 < 0 || cmp2 > 0)
	fatal ("Adding at this position would break the order.\n");
    }
    else {
      DO_CMP (ind, &use_rb_node_ind (rb_indval_first (*tree), tmp), cmp1);
      if (cmp1 > 0) fatal ("Adding at beginning would break the order.\n");
    }
#undef DO_CMP
  }
#endif

  if (cmp_less) {
    struct svalue_cmp_data data;
    data.cmp_less = cmp_less;
    data.key = ind;
    low_rb_add_after ((struct rb_node_hdr **) tree,
		      (low_rb_cmp_fn *) svalue_cmp_eq, &data,
		      HDR (new), HDR (node));
  }
  else
    low_rb_add_after ((struct rb_node_hdr **) tree,
		      (low_rb_cmp_fn *) rb_ind_default_cmp, ind,
		      HDR (new), HDR (node));
  return new;
}

PMOD_EXPORT int rb_indval_delete (struct rb_node_indval **tree,
				  struct svalue *ind,
				  struct svalue *cmp_less)
{
  struct rb_node_indval *old;
  if (cmp_less) {
    struct svalue_cmp_data data;
    data.cmp_less = cmp_less;
    data.key = ind;
    HDR (old) = low_rb_delete ((struct rb_node_hdr **) tree,
			       (low_rb_cmp_fn *) svalue_cmp_eq, &data,
			       (low_rb_move_data_fn *) move_indval_data);
  }
  else
    HDR (old) = low_rb_delete ((struct rb_node_hdr **) tree,
			       (low_rb_cmp_fn *) rb_ind_default_cmp, ind,
			       (low_rb_move_data_fn *) move_indval_data);
  if (old) {
    struct svalue tmp;
    free_svalue (&use_rb_node_ind (old, tmp));
    free_svalue (&old->val);
    really_free_rb_node_indval (old);
    return 1;
  }
  return 0;
}

/* Returns the pointer to the node that actually was freed. */
PMOD_EXPORT struct rb_node_indval *rb_indval_delete_node (struct rb_node_indval **tree,
							  struct rb_node_indval *node,
							  struct svalue *cmp_less)
{
  struct svalue tmp;
  struct rb_node_indval *old;
  use_rb_node_ind (node, tmp);
  if (cmp_less) {
    struct svalue_cmp_data data;
    data.cmp_less = cmp_less;
    data.key = &tmp;
    HDR (old) = low_rb_delete_node ((struct rb_node_hdr **) tree,
				    (low_rb_cmp_fn *) svalue_cmp_eq, &data,
				    (low_rb_move_data_fn *) move_ind_data,
				    HDR (node));
  }
  else
    HDR (old) = low_rb_delete_node ((struct rb_node_hdr **) tree,
				    (low_rb_cmp_fn *) rb_ind_default_cmp, &tmp,
				    (low_rb_move_data_fn *) move_ind_data,
				    HDR (node));
  free_svalue (&use_rb_node_ind (old, tmp));
  free_svalue (&old->val);
  really_free_rb_node_indval (old);
  return old;
}

static struct rb_node_indval *copy_indval_node (struct rb_node_indval *node)
{
  struct rb_node_indval *new = alloc_rb_node_indval();
  struct svalue tmp;
  assign_svalue_no_free (&new->ind, &use_rb_node_ind (node, tmp));
  assign_svalue_no_free (&new->val, &node->val);
  DO_IF_DEBUG (new->ind.type |= RB_FLAG_MARKER);
  return new;
}

PMOD_EXPORT struct rb_node_indval *rb_indval_copy (struct rb_node_indval *tree)
{
  return (struct rb_node_indval *)
    low_rb_copy (HDR (tree), (low_rb_copy_fn *) copy_indval_node);
}

PMOD_EXPORT void rb_indval_free (struct rb_node_indval *tree)
{
  RBSTACK_INIT (rbstack);
  LOW_RB_TRAVERSE (1, rbstack, HDR (tree), ;, ;, ;, ;, ;, ;, {
    /* Pop. */
    tree->ind.type &= ~RB_IND_FLAG_MASK;
    free_svalue (&tree->ind);
    free_svalue (&tree->val);
    really_free_rb_node_indval (tree);
  });
}

/* Functions for handling both types of nodes. */

PMOD_EXPORT union rb_node *rb_find_eq_extcmp (union rb_node *tree, struct svalue *key,
					      struct svalue *cmp_less)
{
  struct svalue_cmp_data data;
  data.cmp_less = cmp_less;
  data.key = key;
  return (union rb_node *)
    low_rb_find_eq (HDR (tree), (low_rb_cmp_fn *) svalue_cmp_eq, &data);
}

PMOD_EXPORT union rb_node *rb_find_lt_extcmp (union rb_node *tree, struct svalue *key,
					      struct svalue *cmp_less)
{
  struct svalue_cmp_data data;
  data.cmp_less = cmp_less;
  data.key = key;
  return (union rb_node *)
    low_rb_find_lt (HDR (tree), (low_rb_cmp_fn *) svalue_cmp_le, &data);
}

PMOD_EXPORT union rb_node *rb_find_gt_extcmp (union rb_node *tree, struct svalue *key,
					      struct svalue *cmp_less)
{
  struct svalue_cmp_data data;
  data.cmp_less = cmp_less;
  data.key = key;
  return (union rb_node *)
    low_rb_find_gt (HDR (tree), (low_rb_cmp_fn *) svalue_cmp_ge, &data);
}

PMOD_EXPORT union rb_node *rb_find_le_extcmp (union rb_node *tree, struct svalue *key,
					      struct svalue *cmp_less)
{
  struct svalue_cmp_data data;
  data.cmp_less = cmp_less;
  data.key = key;
  return (union rb_node *)
    low_rb_find_le (HDR (tree), (low_rb_cmp_fn *) svalue_cmp_ge, &data);
}

PMOD_EXPORT union rb_node *rb_find_ge_extcmp (union rb_node *tree, struct svalue *key,
					      struct svalue *cmp_less)
{
  struct svalue_cmp_data data;
  data.cmp_less = cmp_less;
  data.key = key;
  return (union rb_node *)
    low_rb_find_ge (HDR (tree), (low_rb_cmp_fn *) svalue_cmp_le, &data);
}

/* Functions for handling any type of node. */

/* Each of these step functions is O(log n), but when used to loop
 * through a tree they still sum up to O(n) since every node is
 * visited at most twice. */

PMOD_EXPORT struct rb_node_hdr *rb_first (struct rb_node_hdr *tree)
{
  if (tree)
    while (tree->prev) tree = tree->prev;
  return tree;
}

PMOD_EXPORT struct rb_node_hdr *rb_last (struct rb_node_hdr *tree)
{
  if (tree)
    while (tree->next) tree = tree->next;
  return tree;
}

PMOD_EXPORT struct rb_node_hdr *rb_prev (struct rb_node_hdr *node)
{
  if (node->flags & RB_THREAD_PREV)
    return node->prev;
  else {
    node = node->prev;
    while (!(node->flags & RB_THREAD_NEXT))
      node = node->next;
    return node;
  }
}

PMOD_EXPORT struct rb_node_hdr *rb_next (struct rb_node_hdr *node)
{
  if (node->flags & RB_THREAD_NEXT)
    return node->next;
  else {
    node = node->next;
    while (!(node->flags & RB_THREAD_PREV))
      node = node->prev;
    return node;
  }
}

/* The low level stuff. */

PMOD_EXPORT void rbstack_push (struct rbstack_ptr *rbstack, struct rb_node_hdr *node)
{
  struct rbstack_slice *new = ALLOC_STRUCT (rbstack_slice);
  new->up = rbstack->slice;
  new->stack[0] = node;
  rbstack->slice = new;
  rbstack->ssp = 1;
}

PMOD_EXPORT void rbstack_pop (struct rbstack_ptr *rbstack)
{
  struct rbstack_slice *old = rbstack->slice;
  rbstack->slice = old->up;
  xfree (old);
  rbstack->ssp = STACK_SLICE_SIZE;
}

PMOD_EXPORT void rbstack_up (struct rbstack_ptr *rbstack)
{
  rbstack->slice = rbstack->slice->up;
  rbstack->ssp = STACK_SLICE_SIZE;
}

PMOD_EXPORT void rbstack_free (struct rbstack_ptr *rbstack)
{
  struct rbstack_slice *ptr = rbstack->slice;
  do {
    struct rbstack_slice *old = ptr;
    ptr = ptr->up;
    xfree (old);
  } while (ptr->up);
  rbstack->slice = ptr;
  rbstack->ssp = 0;
}

/* Sets the pointer in parent that points to child. */
#define SET_PTR_TO_CHILD(parent, child, prev_val, next_val)		\
do {									\
  if (child == parent->prev)						\
    parent->prev = prev_val;						\
  else {								\
    DO_IF_DEBUG(							\
      if (child != parent->next)					\
	rb_fatal (parent, "Got invalid parent to %p "			\
		  "(stack probably wrong).\n", child);			\
    );									\
    parent->next = next_val;						\
  }									\
} while (0)

/*                node               ret
 *                /  \               / \
 *               /    \             /   \
 *             ret     c    ==>    a    node
 *             / \                      /  \
 *            /   \                    /    \
 *           a     b                  b      c
 */
static inline struct rb_node_hdr *rot_right (struct rb_node_hdr *node)
{
  /* Note that we don't need to do anything special to keep the
   * pointers in a, b and c intact, even if they're thread
   * pointers pointing back to node and ret. */
  struct rb_node_hdr *ret = node->prev;
  if (ret->flags & RB_THREAD_NEXT) {
#ifdef PIKE_DEBUG
    if (ret->next != node) rb_fatal (node, "Bogus next thread pointer.\n");
#endif
    ret->flags &= ~RB_THREAD_NEXT;
    node->flags |= RB_THREAD_PREV;
  }
  else {
    node->prev = ret->next;
    ret->next = node;
  }
  return ret;
}

/*             node                      ret
 *             /  \                      / \
 *            /    \                    /   \
 *           a     ret      ==>      node    c
 *                 / \               /  \
 *                /   \             /    \
 *               b     c           a      b
 */
static inline struct rb_node_hdr *rot_left (struct rb_node_hdr *node)
{
  struct rb_node_hdr *ret = node->next;
  if (ret->flags & RB_THREAD_PREV) {
#ifdef PIKE_DEBUG
    if (ret->prev != node) rb_fatal (node, "Bogus prev thread pointer.\n");
#endif
    ret->flags &= ~RB_THREAD_PREV;
    node->flags |= RB_THREAD_NEXT;
  }
  else {
    node->next = ret->prev;
    ret->prev = node;
  }
  return ret;
}

/* Returns the root node, which might have changed. The passed stack
 * is freed. */
static struct rb_node_hdr *rebalance_after_add (struct rb_node_hdr *node,
						struct rbstack_ptr rbstack)
{
  struct rb_node_hdr *parent, *grandparent, *uncle, *top;
  RBSTACK_POP (rbstack, parent);
#ifdef PIKE_DEBUG
  if (!parent) rb_fatal (node, "No parent on stack.\n");
#endif
  RBSTACK_POP (rbstack, grandparent);
  top = grandparent ? grandparent : parent;

  while (parent->flags & RB_RED) {
    /* Since the root always is black we know there's a grandparent. */
#ifdef PIKE_DEBUG
    if (!grandparent) rb_fatal (parent, "No parent for red node.\n");
#endif

    if (parent == grandparent->prev) {
      uncle = grandparent->next;

      if (!(grandparent->flags & RB_THREAD_NEXT) && uncle->flags & RB_RED) {
	/* Case 1:
	 *            grandparent(B)                   *grandparent(R)
	 *               /    \                            /    \
	 *              /      \                          /      \
	 *        parent(R)   uncle(R)    ==>       parent(B)   uncle(B)
	 *         /    \                            /    \
	 *        /      \                          /      \
	 *  *node(R) or *node(R)               node(R) or node(R)
	 */
	parent->flags &= ~RB_RED;
	uncle->flags &= ~RB_RED;
	RBSTACK_POP (rbstack, parent);
	if (!parent) {
	  /* The grandparent is root - leave it black. */
	  top = grandparent;
	  break;
	}
	grandparent->flags |= RB_RED;
	node = grandparent;
	RBSTACK_POP (rbstack, grandparent);
	top = grandparent ? grandparent : parent;
      }

      else {
	if (node == parent->next) {
	  /* Case 2:
	   *          grandparent(B)                    grandparent(B)
	   *             /    \                            /    \
	   *            /      \                          /      \
	   *      parent(R)   uncle(B)    ==>        node(R)    uncle(B)
	   *            \                            /    \
	   *             \                          /      \
	   *            *node(R)             *parent(R)    (B)
	   */
	  node = parent;
	  parent = grandparent->prev = rot_left (node);
	}

	/* Case 3:
	 *            grandparent(B)                  parent(B)
	 *               /    \                        /    \
	 *              /      \                      /      \
	 *        parent(R)   uncle(B)    ==>    node(R)  grandparent(R)
	 *         /    \                                    /    \
	 *        /      \                                  /      \
	 *  *node(R)     (B)                              (B)     uncle(B)
	 *
	 * => Done.
	 */
	rot_right (grandparent);
	grandparent->flags |= RB_RED;
	parent->flags &= ~RB_RED;
	RBSTACK_POP (rbstack, top);
	if (top)
	  SET_PTR_TO_CHILD (top, grandparent, parent, parent);
	else
	  top = parent;
	break;
      }
    }

    else {
#ifdef PIKE_DEBUG
      if (parent != grandparent->next)
	rb_fatal (grandparent,
		  "Childs parent doesn't know about it (stack probably wrong).\n");
#endif
      /* The mirrored version of the above. */
      uncle = grandparent->prev;

      if (!(grandparent->flags & RB_THREAD_PREV) && uncle->flags & RB_RED) {
	/* Case 1 */
	parent->flags &= ~RB_RED;
	uncle->flags &= ~RB_RED;
	RBSTACK_POP (rbstack, parent);
	if (!parent) {
	  top = grandparent;
	  break;
	}
	grandparent->flags |= RB_RED;
	node = grandparent;
	RBSTACK_POP (rbstack, grandparent);
	top = grandparent ? grandparent : parent;
      }

      else {
	if (node == parent->prev) {
	  /* Case 2 */
	  node = parent;
	  parent = grandparent->next = rot_right (node);
	}

	/* Case 3 */
	rot_left (grandparent);
	grandparent->flags |= RB_RED;
	parent->flags &= ~RB_RED;
	RBSTACK_POP (rbstack, top);
	if (top)
	  SET_PTR_TO_CHILD (top, grandparent, parent, parent);
	else
	  top = parent;
	break;
      }
    }
  }

  RBSTACK_FREE_SET_ROOT (rbstack, top);
#ifdef PIKE_DEBUG
  if (top->flags & RB_RED) rb_fatal (top, "Root node not black.\n");
#endif
  return top;
}

/* Returns the root node, which might have changed. The passed stack
 * is freed. */
static struct rb_node_hdr *rebalance_after_delete (struct rb_node_hdr *node,
						   struct rbstack_ptr rbstack)
{
  struct rb_node_hdr *parent, *sibling, *top = node;
  RBSTACK_POP (rbstack, parent);

  if (!parent) {
    if (!node) return 0;
    node->flags &= ~RB_RED;
  }
  else do {
    top = parent;

    if (node == parent->prev) {
      if (!(parent->flags & RB_THREAD_PREV) && node->flags & RB_RED) {
	node->flags &= ~RB_RED;
	break;
      }
#ifdef PIKE_DEBUG
      if (parent->flags & RB_THREAD_NEXT)
	/* Since the node in the prev subtree is the "concatenation"
	 * of two black nodes, there must be at least two black nodes
	 * in the next subtree too, i.e. at least one non-null
	 * child. */
	rb_fatal (parent, "Sibling in next is null; tree was unbalanced.\n");
#endif
      sibling = parent->next;

      if (sibling->flags & RB_RED) {
	/* Case 1:
	 *            parent(B)                           sibling(B)
	 *             /    \                              /    \
	 *            /      \                            /      \
	 *     *node(2B)    sibling(R)    ==>       parent(R)    (B)
	 *                   /    \                  /    \
	 *                  /      \                /      \
	 *                (B)      (B)       *node(2B)     (B)
	 */
	parent->flags |= RB_RED;
	sibling->flags &= ~RB_RED;
	rot_left (parent);
	RBSTACK_POP (rbstack, top);
	if (top)
	  SET_PTR_TO_CHILD (top, parent, sibling, sibling);
	else
	  top = sibling;
	RBSTACK_PUSH (rbstack, sibling);
#ifdef PIKE_DEBUG
	if (parent->flags & RB_THREAD_NEXT)
	  rb_fatal (parent, "Sibling in next is null; tree was unbalanced.\n");
#endif
	sibling = parent->next;
	goto prev_node_red_parent;
      }

      if (!(parent->flags & RB_RED)) {
	if ((sibling->flags & RB_THREAD_PREV || !(sibling->prev->flags & RB_RED)) &&
	    (sibling->flags & RB_THREAD_NEXT || !(sibling->next->flags & RB_RED))) {
	  /* Case 2a:
	   *            parent(B)                      *parent(2B)
	   *             /    \                          /    \
	   *            /      \                        /      \
	   *     *node(2B)    sibling(B)    ==>    node(B)    sibling(R)
	   *                   /    \                          /    \
	   *                  /      \                        /      \
	   *                (B)      (B)                    (B)      (B)
	   */
	  sibling->flags |= RB_RED;
	  node = parent;
	  RBSTACK_POP (rbstack, parent);
	  continue;
	}
      }

      else {
      prev_node_red_parent:
	if ((sibling->flags & RB_THREAD_PREV || !(sibling->prev->flags & RB_RED)) &&
	    (sibling->flags & RB_THREAD_NEXT || !(sibling->next->flags & RB_RED))) {
	  /* Case 2b:
	   *            parent(R)                       parent(B)
	   *             /    \                          /    \
	   *            /      \                        /      \
	   *     *node(2B)    sibling(B)    ==>    node(B)    sibling(R)
	   *                   /    \                          /    \
	   *                  /      \                        /      \
	   *                (B)      (B)                    (B)      (B)
	   * => Done.
	   */
	  parent->flags &= ~RB_RED;
	  sibling->flags |= RB_RED;
	  break;
	}
      }

      if (sibling->flags & RB_THREAD_NEXT || !(sibling->next->flags & RB_RED)) {
	/* Case 3:
	 *        parent(?)                        parent(?)
	 *         /    \                           /    \
	 *        /      \                         /      \
	 *  *node(2B)   sibling(B)          *node(2B)  new sibling(B)
	 *               /    \                           /    \
	 *              /      \      ==>                /      \
	 *     new sibling(R)  (B)                     (B)     sibling(R)
	 *         /    \                                       /    \
	 *        /      \                                     /      \
	 *      (B)      (B)                                 (B)      (B)
	 */
	sibling->flags |= RB_RED;
	sibling = parent->next = rot_right (sibling);
	sibling->flags &= ~RB_RED;
      }

      /* Case 4:
       *            parent(?)                           sibling(?)
       *             /    \                              /    \
       *            /      \                            /      \
       *     *node(2B)    sibling(B)    ==>       parent(B)    (B)
       *                   /    \                  /    \
       *                  /      \                /      \
       *                (?)      (R)         node(B)     (?)
       * => Done.
       */
      rot_left (parent);
      RBSTACK_POP (rbstack, top);
      if (top) {
	SET_PTR_TO_CHILD (top, parent, sibling, sibling);
	if (parent->flags & RB_RED) {
	  sibling->flags |= RB_RED;
	  parent->flags &= ~RB_RED;
	}
	else sibling->flags &= ~RB_RED;
      }
      else {
	top = sibling;
	/* sibling is the new root, which should always be black.
	 * So don't transfer the color from parent. */
	sibling->flags &= ~RB_RED;
	parent->flags &= ~RB_RED;
      }
      sibling->next->flags &= ~RB_RED;
      break;
    }

    else {
#ifdef PIKE_DEBUG
      if (node != parent->next)
	rb_fatal (parent,
		  "Childs parent doesn't know about it (stack probably wrong).\n");
#endif
      /* The mirrored version of the above. */
      if (!(parent->flags & RB_THREAD_NEXT) && node->flags & RB_RED) {
	node->flags &= ~RB_RED;
	break;
      }
#ifdef PIKE_DEBUG
      if (parent->flags & RB_THREAD_PREV)
	rb_fatal (parent, "Sibling in prev is null; tree was unbalanced.\n");
#endif
      sibling = parent->prev;

      if (sibling->flags & RB_RED) {
	/* Case 1 */
	parent->flags |= RB_RED;
	sibling->flags &= ~RB_RED;
	rot_right (parent);
	RBSTACK_POP (rbstack, top);
	if (top)
	  SET_PTR_TO_CHILD (top, parent, sibling, sibling);
	else
	  top = sibling;
	RBSTACK_PUSH (rbstack, sibling);
#ifdef PIKE_DEBUG
	if (parent->flags & RB_THREAD_PREV)
	  rb_fatal (parent, "Sibling in prev is null; tree was unbalanced.\n");
#endif
	sibling = parent->prev;
	goto next_node_red_parent;
      }

      if (!(parent->flags & RB_RED)) {
	if ((sibling->flags & RB_THREAD_PREV || !(sibling->prev->flags & RB_RED)) &&
	    (sibling->flags & RB_THREAD_NEXT || !(sibling->next->flags & RB_RED))) {
	  /* Case 2a */
	  sibling->flags |= RB_RED;
	  node = parent;
	  RBSTACK_POP (rbstack, parent);
	  continue;
	}
      }

      else {
      next_node_red_parent:
	if ((sibling->flags & RB_THREAD_PREV || !(sibling->prev->flags & RB_RED)) &&
	    (sibling->flags & RB_THREAD_NEXT || !(sibling->next->flags & RB_RED))) {
	  /* Case 2b */
	  parent->flags &= ~RB_RED;
	  sibling->flags |= RB_RED;
	  break;
	}
      }

      if (sibling->flags & RB_THREAD_PREV || !(sibling->prev->flags & RB_RED)) {
	/* Case 3 */
	sibling->flags |= RB_RED;
	sibling = parent->prev = rot_left (sibling);
	sibling->flags &= ~RB_RED;
      }

      /* Case 4 */
      rot_right (parent);
      RBSTACK_POP (rbstack, top);
      if (top) {
	SET_PTR_TO_CHILD (top, parent, sibling, sibling);
	if (parent->flags & RB_RED) {
	  sibling->flags |= RB_RED;
	  parent->flags &= ~RB_RED;
	}
	else sibling->flags &= ~RB_RED;
      }
      else {
	top = sibling;
	sibling->flags &= ~RB_RED;
	parent->flags &= ~RB_RED;
      }
      sibling->prev->flags &= ~RB_RED;
      break;
    }
  } while (parent);

  RBSTACK_FREE_SET_ROOT (rbstack, top);
#ifdef PIKE_DEBUG
  if (top->flags & RB_RED) rb_fatal (top, "Root node not black.\n");
#endif
  return top;
}

void low_rb_init_root (struct rb_node_hdr *node)
{
#ifdef PIKE_DEBUG
  if (!node) fatal ("New node is null.\n");
#endif
  node->flags = (node->flags & ~RB_RED) | RB_THREAD_PREV | RB_THREAD_NEXT;
  node->prev = node->next = 0;
}

/* The passed stack is freed. */
void low_rb_link_at_prev (struct rb_node_hdr **tree, struct rbstack_ptr rbstack,
			  struct rb_node_hdr *new)
{
  struct rb_node_hdr *parent = RBSTACK_PEEK (rbstack);
#ifdef PIKE_DEBUG
  if (!new) fatal ("New node is null.\n");
  if (!parent) fatal ("Cannot link in root node.\n");
  if (!(parent->flags & RB_THREAD_PREV))
    fatal ("Cannot link in node at interior prev link.\n");
#endif
  new->flags |= RB_RED | RB_THREAD_PREV | RB_THREAD_NEXT;
  new->prev = parent->prev;
  new->next = parent;
  parent->flags &= ~RB_THREAD_PREV;
  parent->prev = new;

  *tree = rebalance_after_add (new, rbstack);
}

/* The passed stack is freed. */
void low_rb_link_at_next (struct rb_node_hdr **tree, struct rbstack_ptr rbstack,
			  struct rb_node_hdr *new)
{
  struct rb_node_hdr *parent = RBSTACK_PEEK (rbstack);
#ifdef PIKE_DEBUG
  if (!new) fatal ("New node is null.\n");
  if (!parent) fatal ("Cannot link in root node.\n");
  if (!(parent->flags & RB_THREAD_NEXT))
    fatal ("Cannot link in node at interior next link.\n");
#endif
  new->flags |= RB_RED | RB_THREAD_PREV | RB_THREAD_NEXT;
  new->prev = parent;
  new->next = parent->next;
  parent->flags &= ~RB_THREAD_NEXT;
  parent->next = new;

  *tree = rebalance_after_add (new, rbstack);
}

/* The node to unlink is the one on top of the stack. Returns the node
 * that actually got unlinked. The passed stack is freed. */
struct rb_node_hdr *low_rb_unlink (struct rb_node_hdr **tree,
				   struct rbstack_ptr rbstack,
				   low_rb_move_data_fn *move_data_fn)
{
  struct rb_node_hdr *node, *parent, *unlinked;
  int replace_with;		/* 0: none, 1: prev, 2: next */

  RBSTACK_POP (rbstack, node);
#ifdef PIKE_DEBUG
  if (!node) fatal ("No node to delete on stack.\n");
#endif

  if (node->flags & RB_THREAD_PREV) {
    unlinked = node;
    replace_with = node->flags & RB_THREAD_NEXT ? 0 : 2;
    parent = RBSTACK_PEEK (rbstack);
  }
  else if (node->flags & RB_THREAD_NEXT) {
    unlinked = node;
    replace_with = 1;
    parent = RBSTACK_PEEK (rbstack);
  }
  else {
    /* Node has two subtrees, so we can't delete it. */
    parent = node;
    RBSTACK_PUSH (rbstack, node);
    unlinked = node->next;
    while (!(unlinked->flags & RB_THREAD_PREV)) {
      parent = unlinked;
      RBSTACK_PUSH (rbstack, unlinked);
      unlinked = unlinked->prev;
    }
    replace_with = unlinked->flags & RB_THREAD_NEXT ? 0 : 2;
    move_data_fn (node, unlinked);
  }

  switch (replace_with) {
    case 0:
      if (parent)
	SET_PTR_TO_CHILD (
	  parent, unlinked,
	  (parent->flags |= RB_THREAD_PREV, node = unlinked->prev),
	  (parent->flags |= RB_THREAD_NEXT, node = unlinked->next));
      else
	node = 0;
      break;
    case 1:
      node = unlinked->prev;
      node->next = unlinked->next;
      goto fix_parent;
    case 2:
      node = unlinked->next;
      node->prev = unlinked->prev;
    fix_parent:
      if (parent) SET_PTR_TO_CHILD (parent, unlinked, node, node);
  }
  /* node now contains the value of the pointer in parent that used to
   * point to the unlinked node. */

  if (unlinked->flags & RB_RED)
    RBSTACK_FREE (rbstack);
  else
    *tree = rebalance_after_delete (node, rbstack);

  return unlinked;
}

/* Returns the node in the tree. It might not be the passed new node
 * if an equal one was found. */
struct rb_node_hdr *low_rb_insert (struct rb_node_hdr **tree,
				   low_rb_cmp_fn *cmp_fn, void *key,
				   struct rb_node_hdr *new)
{
  if (*tree) {
    struct rb_node_hdr *node = *tree;
    RBSTACK_INIT (rbstack);
    LOW_RB_TRACK (rbstack, node, cmp_res = cmp_fn (key, node), {
      low_rb_link_at_next (tree, rbstack, new); /* Got less. */
    }, {			/* Got equal - new not used. */
      RBSTACK_FREE (rbstack);
      return node;
    }, {			/* Got greater. */
      low_rb_link_at_prev (tree, rbstack, new);
    });
  }
  else {
    low_rb_init_root (new);
    *tree = new;
  }
  return new;
}

void low_rb_add (struct rb_node_hdr **tree,
		 low_rb_cmp_fn *cmp_fn, void *key,
		 struct rb_node_hdr *new)
{
  if (*tree) {
    struct rb_node_hdr *node = *tree;
    RBSTACK_INIT (rbstack);
    LOW_RB_TRACK (rbstack, node, cmp_res = cmp_fn (key, node), {
      low_rb_link_at_next (tree, rbstack, new); /* Got less. */
    }, {			/* Got equal. */
      if (node->flags & RB_THREAD_PREV)
	low_rb_link_at_prev (tree, rbstack, new);
      else if (node->flags & RB_THREAD_NEXT)
	low_rb_link_at_next (tree, rbstack, new);
      else {
	node = node->next;
	RBSTACK_PUSH (rbstack, node);
	while (!(node->flags & RB_THREAD_PREV)) {
	  node = node->prev;
	  RBSTACK_PUSH (rbstack, node);
	}
	low_rb_link_at_prev (tree, rbstack, new);
      }
    }, {
      low_rb_link_at_prev (tree, rbstack, new); /* Got greater. */
    });
  }
  else {
    low_rb_init_root (new);
    *tree = new;
  }
}

void low_rb_add_after (struct rb_node_hdr **tree,
		       low_rb_cmp_fn *cmp_fn, void *key,
		       struct rb_node_hdr *new,
		       struct rb_node_hdr *existing)
{
  if (*tree) {
    struct rb_node_hdr *node = *tree;
    RBSTACK_INIT (rbstack);

    if (existing) {
#ifdef PIKE_DEBUG
      if (cmp_fn (key, existing)) fatal ("Given key doesn't match the existing node.\n");
#endif
      LOW_RB_TRACK (rbstack, node, cmp_res = cmp_fn (key, node) > 0 ? 1 : -1,
		    node = node->next, ;, ;);
      while (node != existing) {
	LOW_RB_TRACK_NEXT (rbstack, node);
#ifdef PIKE_DEBUG
	if (!node) fatal ("Tree doesn't contain the existing node.\n");
#endif
      }
      if (node->flags & RB_THREAD_NEXT) {
	low_rb_link_at_next (tree, rbstack, new);
	return;
      }
      else node = node->next;
    }

    /* Link at lowest position. */
    RBSTACK_PUSH (rbstack, node);
    while (!(node->flags & RB_THREAD_PREV)) {
      node = node->prev;
      RBSTACK_PUSH (rbstack, node);
    }
    low_rb_link_at_prev (tree, rbstack, new);
  }

  else {
#ifdef PIKE_DEBUG
    if (existing) fatal ("Tree doesn't contain the existing node.\n");
#endif
    low_rb_init_root (new);
    *tree = new;
  }
}

/* Returns the node to free, if any. */
struct rb_node_hdr *low_rb_delete (struct rb_node_hdr **tree,
				   low_rb_cmp_fn *cmp_fn, void *key,
				   low_rb_move_data_fn *move_data_fn)
{
  struct rb_node_hdr *node = *tree;
  if (node) {
    RBSTACK_INIT (rbstack);
    LOW_RB_TRACK (rbstack, node,
		  cmp_res = cmp_fn (key, node),
		  ;,		/* Got less. */
		  return low_rb_unlink (tree, rbstack, move_data_fn), /* Got equal. */
		  ;);		/* Got greater. */
    RBSTACK_FREE (rbstack);
  }
  return 0;
}

/* Returns the node to actually free. */
struct rb_node_hdr *low_rb_delete_node (struct rb_node_hdr **tree,
					low_rb_cmp_fn *cmp_fn, void *key,
					low_rb_move_data_fn *move_data_fn,
					struct rb_node_hdr *to_delete)
{
  struct rb_node_hdr *node = *tree;
  RBSTACK_INIT (rbstack);
#ifdef PIKE_DEBUG
  if (!node) fatal ("Tree is empty.\n");
  if (cmp_fn (key, to_delete)) fatal ("Given key doesn't match the node to delete.\n");
#endif
  LOW_RB_TRACK (rbstack, node, cmp_res = cmp_fn (key, node) > 0 ? 1 : -1,
		node = node->next, ;, ;);
  while (node != to_delete) {
    LOW_RB_TRACK_NEXT (rbstack, node);
#ifdef PIKE_DEBUG
    if (!node) fatal ("Tree doesn't contain the node to delete.\n");
#endif
  }
  return low_rb_unlink (tree, rbstack, move_data_fn);
}

struct rb_node_hdr *low_rb_copy (struct rb_node_hdr *source,
				 low_rb_copy_fn *copy_node_fn)
{
  if (source) {
    struct rb_node_hdr *copy, *target, *new, *t_prev_tgt = 0, *t_next_src = 0;
    RBSTACK_INIT (s_stack);
    RBSTACK_INIT (t_stack);

    copy = target = copy_node_fn (source);
    copy->flags = (copy->flags & ~RB_FLAG_MASK) | (source->flags & RB_FLAG_MASK);

    LOW_RB_TRAVERSE (1, s_stack, source, { /* Push. */
    }, {			/* prev is leaf. */
      target->prev = t_prev_tgt;
    }, {			/* prev is subtree. */
      new = target->prev = copy_node_fn (source->prev);
      new->flags = (new->flags & ~RB_FLAG_MASK) | (source->prev->flags & RB_FLAG_MASK);
      RBSTACK_PUSH (t_stack, target);
      target = new;
    }, {			/* Between. */
      t_prev_tgt = target;
      if (t_next_src) {
	t_next_src->next = target;
	t_next_src = 0;
      }
    }, {			/* next is leaf. */
      t_next_src = target;
    }, {			/* next is subtree. */
      new = target->next = copy_node_fn (source->next);
      new->flags = (new->flags & ~RB_FLAG_MASK) | (source->next->flags & RB_FLAG_MASK);
      RBSTACK_PUSH (t_stack, target);
      target = new;
    }, {			/* Pop. */
      RBSTACK_POP (t_stack, target);
    });

    if (t_next_src) t_next_src->next = 0;
    return copy;
  }
  else return 0;
}

void low_rb_free (struct rb_node_hdr *tree, low_rb_free_fn *free_node_fn)
{
  RBSTACK_INIT (rbstack);
  LOW_RB_TRAVERSE (1, rbstack, tree, ;, ;, ;, ;, ;, ;, free_node_fn (tree));
}

struct rb_node_hdr *low_rb_find_eq (struct rb_node_hdr *tree,
				    low_rb_cmp_fn *cmp_fn, void *key)
{
  if (tree)
    LOW_RB_FIND (tree, cmp_res = cmp_fn (key, tree), ;, return tree, ;);
  return 0;
}

struct rb_node_hdr *low_rb_find_lt (struct rb_node_hdr *tree,
				    low_rb_cmp_fn *cmp_fn, void *key)
{
  if (tree)
    LOW_RB_FIND (tree, cmp_res = cmp_fn (key, tree) > 0 ? 1 : -1,
		 return tree, ;, return tree->prev);
  return 0;
}

struct rb_node_hdr *low_rb_find_gt (struct rb_node_hdr *tree,
				    low_rb_cmp_fn *cmp_fn, void *key)
{
  if (tree)
    LOW_RB_FIND (tree, cmp_res = cmp_fn (key, tree) >= 0 ? 1 : -1,
		 return tree->next, ;, return tree);
  return 0;
}

struct rb_node_hdr *low_rb_find_le (struct rb_node_hdr *tree,
				    low_rb_cmp_fn *cmp_fn, void *key)
{
  if (tree)
    LOW_RB_FIND (tree, cmp_res = cmp_fn (key, tree) >= 0 ? 1 : -1,
		 return tree, ;, return tree->prev);
  return 0;
}

struct rb_node_hdr *low_rb_find_ge (struct rb_node_hdr *tree,
				    low_rb_cmp_fn *cmp_fn, void *key)
{
  if (tree)
    LOW_RB_FIND (tree, cmp_res = cmp_fn (key, tree) > 0 ? 1 : -1,
		 return tree->next, ;, return tree);
  return 0;
}

void init_rbtree()
{
#ifdef PIKE_DEBUG
  union rb_node test;
  test.h.flags = 0;
  test.i.ind.type = (1 << 8) - 1;
  test.i.ind.subtype = (1 << 16) - 1;
  test.i.ind.u.refs = (INT32 *) (ptrdiff_t) -1;
  if (test.h.flags & (RB_IND_FLAG_MASK))
    fatal ("The ind svalue overlays the flags field in an unexpected way.\n");
  test.h.flags |= RB_FLAG_MASK;
  if (test.i.ind.type & RB_FLAG_MARKER)
    fatal ("The ind svalue overlays the flags field in an unexpected way.\n");
  test.i.ind.type |= RB_FLAG_MARKER;
  if ((test.i.ind.type & ~RB_IND_FLAG_MASK) != (1 << 8) - 1)
    fatal ("The ind svalue overlays the flags field in an unexpected way.\n");
#endif
  init_rb_node_ind_blocks();
  init_rb_node_indval_blocks();
}

/* Pike may exit without calling this. */
void exit_rbtree()
{
  free_all_rb_node_ind_blocks();
  free_all_rb_node_indval_blocks();
}

#if defined (PIKE_DEBUG) || defined (TEST_RBTREE)

static void debug_dump_ind_data (struct rb_node_ind *node)
{
  struct svalue tmp;
  print_svalue (stderr, &use_rb_node_ind (node, tmp));
  fputc (' ', stderr);
}

static void debug_dump_indval_data (struct rb_node_indval *node)
{
  struct svalue tmp;
  print_svalue (stderr, &use_rb_node_ind (node, tmp));
  fputs (": ", stderr);
  print_svalue (stderr, &node->val);
  fputc (' ', stderr);
}

static void debug_dump_rb_tree (struct rb_node_hdr *tree, dump_data_fn *dump_data)
{
  if (tree) {
    struct rb_node_hdr *n;
    struct rbstack_ptr p;
    RBSTACK_INIT (rbstack);

    LOW_RB_TRAVERSE (1, rbstack, tree, { /* Push. */
      p = rbstack;
      RBSTACK_UP (p, n);
      while (n) {
	if (n == tree) {
	  fprintf (stderr, "[Circular! %p]", tree);
	  goto leave_1;
	}
	RBSTACK_UP (p, n);
      }
      fputc ('(', stderr);
    }, {			/* prev is leaf. */
      fprintf (stderr, "[%p]", tree->prev);
    }, {			/* prev is subtree. */
      if (!tree->prev) {
	fputs ("[Zero subtree!]", stderr);
	goto between_1;
      }
    }, {			/* Between nodes. */
      fprintf (stderr, " %p/%c",
	       tree, tree->flags & RB_RED ? 'R' : 'B');
      if (dump_data) {
	fputs (": ", stderr);
	dump_data (tree);
      }
      else fputc (' ', stderr);
    }, {			/* next is leaf. */
      fprintf (stderr, "[%p]", tree->next);
    }, {			/* next is subtree. */
      if (!tree->next) {
	fputs ("[Zero subtree!]", stderr);
	goto leave_1;
      }
    }, {			/* Pop. */
      fputc (')', stderr);
    });
    fputc ('\n', stderr);
  }
  else
    fprintf (stderr, "(empty tree)\n");
}

static void debug_rb_fatal (struct rb_node_hdr *tree, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  (void) VFPRINTF (stderr, fmt, args);
  fputs ("Dumping tree: ", stderr);
  debug_dump_rb_tree (tree, 0);
  debug_fatal ("\r");
}

static void debug_rb_ind_fatal (struct rb_node_ind *tree, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  (void) VFPRINTF (stderr, fmt, args);
  fputs ("Dumping tree: ", stderr);
  debug_dump_rb_tree ((struct rb_node_hdr *) tree,
		      (dump_data_fn *) debug_dump_ind_data);
  debug_fatal ("\r");
}

static void debug_rb_indval_fatal (struct rb_node_indval *tree, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  (void) VFPRINTF (stderr, fmt, args);
  fputs ("Dumping tree: ", stderr);
  debug_dump_rb_tree ((struct rb_node_hdr *) tree,
		      (dump_data_fn *) debug_dump_indval_data);
  debug_fatal ("\r");
}

void debug_check_rb_tree (struct rb_node_hdr *tree)
{
  if (tree) {
    struct rb_node_hdr *node = tree, *n, *n2;
    struct rbstack_ptr p;
    size_t blacks = 1, max_blacks = 0, depth = 0;
    RBSTACK_INIT (rbstack);

    if (tree->flags & RB_RED) rb_fatal (tree, "Root node not black.\n");

    LOW_RB_TRAVERSE (1, rbstack, node, { /* Push. */
      depth++;
      p = rbstack;
      RBSTACK_UP (p, n);
      while (n) {
	if (n == node) rb_fatal (tree, "Circular subtrees @ %p.\n", node);
	RBSTACK_UP (p, n);
      }

      if (!(node->flags & RB_RED)) blacks++;
    },
    {				/* prev is leaf. */
      if (max_blacks) {
	if (blacks != max_blacks)
	  rb_fatal (tree, "Unbalanced tree - leftmost branch is %d, this @ %p is %d.\n",
		    max_blacks, node, blacks);
      }
      else max_blacks = blacks;

      p = rbstack;
      n2 = node;
      RBSTACK_UP (p, n);
      while (n && (n->flags & RB_THREAD_NEXT || n->next != n2)) {
	n2 = n;
	RBSTACK_UP (p, n);
      }
      if (node->prev != n)
	rb_fatal (tree, "Thread prev pointer @ %p is %p, expected %p.\n",
		  node, node->prev, n);
    },
    {				/* prev is subtree. */
      if (!node->prev) rb_fatal (tree, "Subtree prev is null @ %p.\n", node);
      if (node->flags & RB_RED && node->prev->flags & RB_RED)
	rb_fatal (tree, "Red node got red subtree prev node @ %p.\n", node);
    },
    {				/* Between nodes. */
    },
    {				/* next is leaf. */
      if (blacks != max_blacks)
	rb_fatal (tree, "Unbalanced tree - leftmost branch is %d, this @ %p is %d.\n",
		  max_blacks, node, blacks);

      p = rbstack;
      n2 = node;
      RBSTACK_UP (p, n);
      while (n && (n->flags & RB_THREAD_PREV || n->prev != n2)) {
	n2 = n;
	RBSTACK_UP (p, n);
      }
      if (node->next != n)
	rb_fatal (tree, "Thread next pointer @ %p is %p, expected %p.\n",
		  node, node->next, n);
    },
    {				/* next is subtree. */
      if (!node->next) rb_fatal (tree, "Subtree next is null @ %p.\n", node);
      if (node->flags & RB_RED && node->next->flags & RB_RED)
	rb_fatal (tree, "Red node got red subtree next node @ %p.\n", node);
    },
    {				/* Pop. */
      if (!(node->flags & RB_RED)) blacks--;
      depth--;
    });
  }
}

#ifdef TEST_RBTREE

#define TEST_I_FIND(fn, exp)						\
do {									\
  i_node = PIKE_CONCAT (rb_ind_, fn) (i_tree, sp - 1, 0);		\
  if (!i_node)								\
    rb_ind_fatal (i_tree, #fn " failed to find %d (%d).\n", exp, i);	\
  if (i_node->ind.u.integer != exp)					\
    rb_ind_fatal (							\
      i_tree, #fn " failed to find %d - got %d instead (%d).\n",	\
      exp, i_node->ind.u.integer, i);					\
  i_node = PIKE_CONCAT (rb_ind_, fn) (i_tree, sp - 1, less_efun);	\
  if (!i_node)								\
    rb_ind_fatal (i_tree, #fn " failed to find %d "			\
		  "with cmp_less (%d).\n", exp, i);			\
  if (i_node->ind.u.integer != exp)					\
    rb_ind_fatal (i_tree, #fn " failed to find %d "			\
		  "with cmp_less - got %d instead (%d).\n",		\
		  exp, i_node->ind.u.integer, i);			\
} while (0)

#define TEST_I_NOT_FIND(fn)						\
do {									\
  i_node = PIKE_CONCAT (rb_ind_, fn) (i_tree, sp - 1, 0);		\
  if (i_node)								\
    rb_ind_fatal (i_tree, #fn " failed to not find %d - "		\
		  "got %d (%d).\n",					\
		  sp[-1].u.integer, i_node->ind.u.integer, i);		\
  i_node = PIKE_CONCAT (rb_ind_, fn) (i_tree, sp - 1, less_efun);	\
  if (i_node)								\
    rb_ind_fatal (i_tree, #fn " failed to not find %d "			\
		  "with cmp_less - got %d (%d).\n",			\
		  sp[-1].u.integer, i_node->ind.u.integer, i);		\
} while (0)

#define TEST_I_STEP_FIND(fn, dir, exp)					\
do {									\
  i_node = PIKE_CONCAT (rb_ind_, dir) (i_node);				\
  if (!i_node)								\
    rb_ind_fatal (i_tree, "Failed to step " #dir " to %d after " #fn	\
		  " of %d (%d).\n", exp, sp[-1].u.integer, i);		\
  if (i_node->ind.u.integer != exp)					\
    rb_ind_fatal (i_tree, "Failed to step " #dir " to %d after " #fn	\
		  " of %d - got %d instead (%d).\n",			\
		  exp, sp[-1].u.integer, i_node->ind.u.integer, i);	\
} while (0)

#define TEST_I_STEP_NOT_FIND(fn, dir)					\
do {									\
  i_node = PIKE_CONCAT (rb_ind_, dir) (i_node);				\
  if (i_node)								\
    rb_ind_fatal (i_tree, "Failed to step " #dir " to end after " #fn	\
		  " of %d - got %d (%d).\n",				\
		  sp[-1].u.integer, i_node->ind.u.integer, i);		\
} while (0)

#define TEST_IV_FIND(fn, exp)						\
do {									\
  iv_node = PIKE_CONCAT (rb_indval_, fn) (iv_tree, sp - 1, 0);		\
  if (!iv_node)								\
    rb_indval_fatal (iv_tree, #fn " failed to find %d (%d).\n",		\
		     exp, i);						\
  if (iv_node->ind.u.integer != exp)					\
    rb_indval_fatal (							\
      iv_tree, #fn " failed to find %d - got %d instead (%d).\n",	\
      exp, iv_node->ind.u.integer, i);					\
  iv_node = PIKE_CONCAT (rb_indval_, fn) (iv_tree, sp - 1, less_efun);	\
  if (!iv_node)								\
    rb_indval_fatal (iv_tree, #fn " failed to find %d "			\
		     "with cmp_less (%d).\n", exp, i);			\
  if (iv_node->ind.u.integer != exp)					\
    rb_indval_fatal (iv_tree, #fn " failed to find %d "			\
		     "with cmp_less - got %d instead (%d).\n",		\
		     exp, iv_node->ind.u.integer, i);			\
} while (0)

#define TEST_IV_NOT_FIND(fn)						\
do {									\
  iv_node = PIKE_CONCAT (rb_indval_, fn) (iv_tree, sp - 1, 0);		\
  if (iv_node)								\
    rb_indval_fatal (iv_tree, #fn " failed to not find %d - "		\
		     "got %d (%d).\n",					\
		     sp[-1].u.integer, iv_node->ind.u.integer, i);	\
  iv_node = PIKE_CONCAT (rb_indval_, fn) (iv_tree, sp - 1, less_efun);	\
  if (iv_node)								\
    rb_indval_fatal (iv_tree, #fn " failed to not find %d "		\
		     "with cmp_less - got %d (%d).\n",			\
		     sp[-1].u.integer, iv_node->ind.u.integer, i);	\
} while (0)

#define TEST_IV_STEP_FIND(fn, dir, exp)					\
do {									\
  iv_node = PIKE_CONCAT (rb_indval_, dir) (iv_node);			\
  if (!iv_node)								\
    rb_indval_fatal (iv_tree, "Failed to step " #dir			\
		     " to %d after " #fn " of %d (%d).\n",		\
		     exp, sp[-1].u.integer, i);				\
  if (iv_node->ind.u.integer != exp)					\
    rb_indval_fatal (iv_tree, "Failed to step " #dir			\
		     " to %d after " #fn " of %d - "			\
		     "got %d instead (%d).\n",				\
		     exp, sp[-1].u.integer, iv_node->ind.u.integer, i);	\
} while (0)

#define TEST_IV_STEP_NOT_FIND(fn, dir)					\
do {									\
  iv_node = PIKE_CONCAT (rb_indval_, dir) (iv_node);			\
  if (iv_node)								\
    rb_indval_fatal (iv_tree, "Failed to step " #dir			\
		     " to end after " #fn " of %d - got %d (%d).\n",	\
		     sp[-1].u.integer, iv_node->ind.u.integer, i);	\
} while (0)

void test_rbtree()
{
  size_t i;
  struct svalue *less_efun;
  struct array *a;
  struct rb_node_ind *i_tree = 0, *i_node;
  struct rb_node_indval *iv_tree = 0, *iv_node;

  push_svalue (simple_mapping_string_lookup (get_builtin_constants(), "`<"));
  less_efun = sp - 1;

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
    int v;
    size_t j;

    if (!(i % 1000)) fprintf (stderr, "i %d         \r", i);

    stack_dup();
    push_int (i);
    f_permute (2);
    a = sp[-1].u.array;

    for (j = 0; j < 12; j++) {
      rb_ind_insert (&i_tree, &a->item[j], 0);
      debug_check_rb_tree (HDR (i_tree));
    }

    for (j = 0, v = 0, i_node = rb_ind_first (i_tree); i_node;
	 i_node = rb_ind_next (i_node), j++) {
      push_rb_node_ind (i_node);
      if (v >= sp[-1].u.integer) rb_ind_fatal (i_tree, "Failed to sort (%d).\n", i);
      v = sp[-1].u.integer;
      pop_stack();
    }
    if (j != 9) rb_ind_fatal (i_tree, "Size of tree is wrong: %d (%d)\n", j, i);

    push_int (5);
    TEST_I_FIND (find_eq, 5);
    TEST_I_FIND (find_lt, 4);
    TEST_I_FIND (find_gt, 7);
    TEST_I_FIND (find_le, 5);
    TEST_I_FIND (find_ge, 5);
    pop_stack();

    push_int (6);
    TEST_I_NOT_FIND (find_eq);
    TEST_I_FIND (find_lt, 5);
    TEST_I_FIND (find_gt, 7);
    TEST_I_FIND (find_le, 5);
    TEST_I_FIND (find_ge, 7);
    pop_stack();

    push_int (0);
    TEST_I_NOT_FIND (find_eq);
    TEST_I_NOT_FIND (find_lt);
    TEST_I_FIND (find_gt, 1);
    TEST_I_NOT_FIND (find_le);
    TEST_I_FIND (find_ge, 1);
    pop_stack();

    push_int (1);
    TEST_I_FIND (find_eq, 1);
    TEST_I_NOT_FIND (find_lt);
    TEST_I_FIND (find_gt, 2);
    TEST_I_FIND (find_le, 1);
    TEST_I_FIND (find_ge, 1);
    pop_stack();

    push_int (15);
    TEST_I_FIND (find_eq, 15);
    TEST_I_FIND (find_lt, 14);
    TEST_I_NOT_FIND (find_gt);
    TEST_I_FIND (find_le, 15);
    TEST_I_FIND (find_ge, 15);
    pop_stack();

    push_int (17);
    TEST_I_NOT_FIND (find_eq);
    TEST_I_FIND (find_lt, 15);
    TEST_I_NOT_FIND (find_gt);
    TEST_I_FIND (find_le, 15);
    TEST_I_NOT_FIND (find_ge);
    pop_stack();

    i_node = rb_ind_copy (i_tree);
    debug_check_rb_tree (HDR (i_node));

    for (j = 0, v = 0; j < 12; j++) {
      v += !!rb_ind_delete (&i_node, &a->item[j], 0);
      debug_check_rb_tree (HDR (i_node));
    }
    if (v != 9 || i_node)
      rb_ind_fatal (i_node, "rb_ind_delete deleted "
		    "wrong number of entries: %d (%d)\n", v, i);

    rb_ind_free (i_tree), i_tree = 0;
    pop_stack();
  }
  pop_stack();

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
    int v;
    size_t j;

    if (!(i % 1000)) fprintf (stderr, "iv %d       \r", i);

    stack_dup();
    push_int (i);
    f_permute (2);
    a = sp[-1].u.array;

    {
      struct rb_node_indval *nodes[8];
      push_int (17);

      for (j = 0; j < 8; j++) {
	for (iv_node = rb_indval_last (iv_tree);
	     iv_node; iv_node = rb_indval_prev (iv_node)) {
	  if (iv_node->val.u.integer <= a->item[j].u.integer) break;
	}
	nodes[j] = rb_indval_add_after (&iv_tree, iv_node, sp - 1, &a->item[j], 0);
	debug_check_rb_tree (HDR (iv_tree));
      }

      for (j = 0, v = 0, iv_node = rb_indval_first (iv_tree);
	   iv_node; iv_node = rb_indval_next (iv_node), j++) {
	if (v > iv_node->val.u.integer)
	  rb_indval_fatal (iv_tree, "Failed to add in order (%d).\n", i);
	v = iv_node->val.u.integer;
      }
      if (j != 8) rb_indval_fatal (iv_tree, "Size of tree is wrong: %d (%d)\n", j, i);

      for (j = 0; j < 8; j++) {
	iv_node = rb_indval_delete_node (&iv_tree, nodes[j], 0);
	if (iv_node != nodes[j]) {
	  for (v = j + 1;; v++) {
	    if (v >= 8) rb_indval_fatal (iv_tree, "rb_indval_delete_node "
					 "returned a bogus value.\n");
	    if (iv_node == nodes[v]) break;
	  }
	  nodes[v] = nodes[j];
	}
	debug_check_rb_tree (HDR (iv_tree));
      }
      if (iv_tree)
	rb_indval_fatal (iv_node, "rb_indval_delete_node didn't delete "
			 "the whole tree (%d)\n", i);

      pop_stack();
    }

    for (j = 0; j < 8; j++) {
      rb_indval_add (&iv_tree, &a->item[j], &a->item[j], 0);
      debug_check_rb_tree (HDR (iv_tree));
    }

    for (j = 0, v = 0, iv_node = rb_indval_first (iv_tree);
	 iv_node; iv_node = rb_indval_next (iv_node), j++) {
      push_rb_node_ind (iv_node);
      if (v > sp[-1].u.integer) rb_indval_fatal (iv_tree, "Failed to sort (%d).\n", i);
      v = sp[-1].u.integer;
      pop_stack();
    }
    if (j != 8) rb_indval_fatal (iv_tree, "Size of tree is wrong: %d (%d)\n", j, i);

    push_int (5);
    TEST_IV_FIND (find_eq, 5);
    TEST_IV_FIND (find_lt, 4);
    TEST_IV_FIND (find_gt, 7);
    TEST_IV_FIND (find_le, 5);
    TEST_IV_STEP_FIND (find_le, next, 7);
    TEST_IV_FIND (find_ge, 5);
    TEST_IV_STEP_FIND (find_ge, prev, 4);
    pop_stack();

    push_int (6);
    TEST_IV_NOT_FIND (find_eq);
    TEST_IV_FIND (find_lt, 5);
    TEST_IV_FIND (find_gt, 7);
    TEST_IV_FIND (find_le, 5);
    TEST_IV_STEP_FIND (find_le, next, 7);
    TEST_IV_FIND (find_ge, 7);
    TEST_IV_STEP_FIND (find_ge, prev, 5);
    pop_stack();

    push_int (0);
    TEST_IV_NOT_FIND (find_eq);
    TEST_IV_NOT_FIND (find_lt);
    TEST_IV_FIND (find_gt, 1);
    TEST_IV_STEP_NOT_FIND (find_gt, prev);
    TEST_IV_NOT_FIND (find_le);
    TEST_IV_FIND (find_ge, 1);
    TEST_IV_STEP_FIND (find_ge, next, 1);
    pop_stack();

    push_int (1);
    TEST_IV_FIND (find_eq, 1);
    TEST_IV_NOT_FIND (find_lt);
    TEST_IV_FIND (find_gt, 4);
    TEST_IV_FIND (find_le, 1);
    TEST_IV_STEP_FIND (find_le, next, 4);
    TEST_IV_FIND (find_ge, 1);
    TEST_IV_STEP_NOT_FIND (find_ge, prev);
    pop_stack();

    push_int (15);
    TEST_IV_FIND (find_eq, 15);
    TEST_IV_FIND (find_lt, 7);
    TEST_IV_NOT_FIND (find_gt);
    TEST_IV_FIND (find_le, 15);
    TEST_IV_STEP_NOT_FIND (find_le, next);
    TEST_IV_FIND (find_ge, 15);
    TEST_IV_STEP_FIND (find_ge, prev, 7);
    pop_stack();

    push_int (17);
    TEST_IV_NOT_FIND (find_eq);
    TEST_IV_FIND (find_lt, 15);
    TEST_IV_STEP_NOT_FIND (find_lt, next);
    TEST_IV_NOT_FIND (find_gt);
    TEST_IV_FIND (find_le, 15);
    TEST_IV_STEP_FIND (find_le, prev, 15);
    TEST_IV_NOT_FIND (find_ge);
    pop_stack();

    iv_node = rb_indval_copy (iv_tree);
    debug_check_rb_tree (HDR (iv_node));
    for (j = 0, v = 0; j < 8; j++) {
      v += !!rb_indval_delete (&iv_node, &a->item[j], 0);
      debug_check_rb_tree (HDR (iv_node));
    }
    if (v != 8 || iv_node)
      rb_indval_fatal (iv_node, "rb_indval_delete deleted "
		       "wrong number of entries: %d (%d)\n", v, i);

    rb_indval_free (iv_tree), iv_tree = 0;
    pop_stack();
  }
  pop_stack();

  pop_stack();
  fprintf (stderr, "             \r");
}

#endif /* TEST_RBTREE */

#endif /* PIKE_DEBUG */
