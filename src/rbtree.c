/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: rbtree.c,v 1.24 2004/09/18 20:50:55 nilsson Exp $
*/

/* An implementation of a threaded red/black balanced binary tree.
 *
 * Created 2001-04-27 by Martin Stjernholm <mast@lysator.liu.se>
 */

#include "global.h"

#include "interpret.h"
#include "pike_error.h"
#include "rbtree_low.h"

#include <assert.h>
#include <stdlib.h>

#if defined (PIKE_DEBUG) || defined (TEST_MULTISET)

DECLSPEC(noreturn) static void debug_rb_fatal (
  struct rb_node_hdr *tree,
  const char *fmt, ...) ATTRIBUTE((noreturn, format (printf, 2, 3)));
#define rb_fatal							\
  (fprintf (stderr, "%s:%d: Fatal in rbtree: ", __FILE__, __LINE__),	\
   debug_rb_fatal)
DECLSPEC(noreturn) static void debug_custom_rb_fatal (
  struct rb_node_hdr *tree, dump_data_fn *dump_data,
  void *extra, const char *fmt, ...) ATTRIBUTE((noreturn, format (printf, 4, 5)));
#define custom_rb_fatal							\
  (fprintf (stderr, "%s:%d: Fatal in rbtree: ", __FILE__, __LINE__),	\
   debug_custom_rb_fatal)

#endif

void rbstack_push (struct rbstack_ptr *rbstack, struct rb_node_hdr *node)
{
  struct rbstack_slice *new = ALLOC_STRUCT (rbstack_slice);
  new->up = rbstack->slice;
  new->stack[0] = node;
#ifdef RB_STATS
  rbstack_slice_allocs++;
  new->depth = rbstack->slice->depth;
  new->maxdepth = rbstack->slice->maxdepth;
#endif
  rbstack->slice = new;
  rbstack->ssp = 1;
}

void rbstack_pop (struct rbstack_ptr *rbstack)
{
  struct rbstack_slice *old = rbstack->slice;
  rbstack->slice = old->up;
#ifdef RB_STATS
  rbstack->slice->maxdepth = old->maxdepth;
#endif
  xfree (old);
  rbstack->ssp = STACK_SLICE_SIZE;
}

void rbstack_up (struct rbstack_ptr *rbstack)
{
  rbstack->slice = rbstack->slice->up;
  rbstack->ssp = STACK_SLICE_SIZE;
}

void rbstack_up_to_root (struct rbstack_ptr *rbstack)
{
  struct rbstack_slice *up;
  while ((up = rbstack->slice->up)) rbstack->slice = up;
  rbstack->ssp = 0;
}

void rbstack_free (struct rbstack_ptr *rbstack)
{
  struct rbstack_slice *ptr = rbstack->slice;
  do {
    struct rbstack_slice *old = ptr;
    ptr = ptr->up;
#ifdef RB_STATS
    ptr->maxdepth = old->maxdepth;
#endif
    xfree (old);
  } while (ptr->up);
  rbstack->slice = ptr;
  rbstack->ssp = 0;
}

/* Inserts the given node at *pos and advances *pos. *top is the top
 * of the stack, which also is advanced to keep track of the top. */
void rbstack_insert (struct rbstack_ptr *top, struct rbstack_ptr *pos,
		     struct rb_node_hdr *node)
{
  struct rbstack_ptr rbp1 = *top, rbp2 = *top, rbpos = *pos;
  RBSTACK_PUSH (rbp2, NULL);
  *top = rbp2;

  while (rbp1.ssp != rbpos.ssp || rbp1.slice != rbpos.slice) {
    rbp2.slice->stack[--rbp2.ssp] = rbp1.slice->stack[--rbp1.ssp];
    if (!rbp2.ssp) rbp2.slice = rbp1.slice, rbp2.ssp = STACK_SLICE_SIZE;
    if (!rbp1.ssp) {
      if (rbp1.slice->up) rbstack_up (&rbp1);
#ifdef PIKE_DEBUG
      else if (rbp1.ssp != rbpos.ssp || rbp1.slice != rbpos.slice)
	Pike_fatal ("Didn't find the given position on the stack.\n");
#endif
    }
  }

  RBSTACK_POKE (rbp2, node);
  *pos = rbp2;
}

#if 0
/* Disabled since these aren't tested and not currently used. */

void rbstack_assign (struct rbstack_ptr *target, struct rbstack_ptr *source)
{
  struct rbstack_slice *src_slice = source->slice;
  struct rbstack_slice *tgt_slice = target->slice;

#ifdef PIKE_DEBUG
  if (target->ssp) Pike_fatal ("target rbstack not empty.\n");
#endif

  target->ssp = source->ssp;
  source->ssp = 0;

  if (src_slice->up) {
    struct rbstack_slice *prev_slice;
    target->slice = src_slice;
    do {
      prev_slice = src_slice;
      src_slice = src_slice->up;
    } while (src_slice->up);
    MEMCPY ((char *) &tgt_slice->stack, (char *) &src_slice->stack,
	    STACK_SLICE_SIZE * sizeof (struct rb_node_hdr *));
    prev_slice->up = tgt_slice;
    source->slice = src_slice;
  }
  else {
    MEMCPY ((char *) &tgt_slice->stack, (char *) &src_slice->stack,
	    target->ssp * sizeof (struct rb_node_hdr *));
#ifdef RB_STATS
    tgt_slice->maxdepth = src_slice->maxdepth;
    tgt_slice->depth = src_slice->depth;
#endif
  }
}

void rbstack_copy (struct rbstack_ptr *target, struct rbstack_ptr *source)
{
  struct rbstack_slice *src_slice = source->slice;
  struct rbstack_slice *tgt_stack_slice = target->slice;
  size_t ssp = source->ssp;

#ifdef PIKE_DEBUG
  if (target->ssp) Pike_fatal ("target rbstack not empty.\n");
#endif

  target->ssp = ssp;

  if (src_slice->up) {
    struct rbstack_slice *tgt_slice =
      target->slice = ALLOC_STRUCT (rbstack_slice);
#ifdef RB_STATS
    rbstack_slice_allocs++;
#endif
    MEMCPY ((char *) &tgt_slice->stack, (char *) &src_slice->stack,
	    ssp * sizeof (struct rb_node_hdr *));
    ssp = STACK_SLICE_SIZE;
    while ((src_slice = src_slice->up)->up) {
      tgt_slice->up = ALLOC_STRUCT (rbstack_slice);
      tgt_slice = tgt_slice->up;
#ifdef RB_STATS
      rbstack_slice_allocs++;
#endif
      MEMCPY ((char *) &tgt_slice->stack, (char *) &src_slice->stack,
	      STACK_SLICE_SIZE * sizeof (struct rb_node_hdr *));
    }
    tgt_slice->up = tgt_stack_slice;
  }

  MEMCPY ((char *) &tgt_stack_slice->stack, (char *) &src_slice->stack,
	  ssp * sizeof (struct rb_node_hdr *));

#ifdef RB_STATS
  target->slice->maxdepth = target->slice->depth = source->slice->depth;
#endif
}

#endif	/* Not tested or used. */

/* Offsets all the rb_node_hdr pointers in rbstack from their offsets
 * relative oldbase to the same relative newbase. Useful if the memory
 * block containing the tree has been moved. */
void rbstack_shift (struct rbstack_ptr rbstack,
		    struct rb_node_hdr *oldbase,
		    struct rb_node_hdr *newbase)
{
  struct rb_node_hdr *node;
  while ((node = RBSTACK_PEEK (rbstack))) {
    rbstack.slice->stack[rbstack.ssp - 1] =
      (struct rb_node_hdr *) ((char *) node - (char *) oldbase + (char *) newbase);
    RBSTACK_UP_IGNORE (rbstack);
  }
}

/* Each of these step functions is O(log n), but when used to loop
 * through a tree they still sum up to O(n) since every node is
 * visited at most twice. */

PMOD_EXPORT struct rb_node_hdr *rb_first (struct rb_node_hdr *root)
{
  DO_IF_RB_STATS (rb_num_sidesteps++; rb_num_sidestep_ops++);
  if (root)
    while (root->prev) {
      root = root->prev;
      DO_IF_RB_STATS (rb_num_sidestep_ops++);
    }
  return root;
}

PMOD_EXPORT struct rb_node_hdr *rb_last (struct rb_node_hdr *root)
{
  DO_IF_RB_STATS (rb_num_sidesteps++; rb_num_sidestep_ops++);
  if (root)
    while (root->next) {
      root = root->next;
      DO_IF_RB_STATS (rb_num_sidestep_ops++);
    }
  return root;
}

PMOD_EXPORT struct rb_node_hdr *rb_link_prev (struct rb_node_hdr *node)
{
  node = node->prev;
  DO_IF_RB_STATS (rb_num_sidestep_ops++);
  while (!(node->flags & RB_THREAD_NEXT)) {
    node = node->next;
    DO_IF_RB_STATS (rb_num_sidestep_ops++);
  }
  return node;
}

PMOD_EXPORT struct rb_node_hdr *rb_link_next (struct rb_node_hdr *node)
{
  node = node->next;
  DO_IF_RB_STATS (rb_num_sidestep_ops++);
  while (!(node->flags & RB_THREAD_PREV)) {
    node = node->prev;
    DO_IF_RB_STATS (rb_num_sidestep_ops++);
  }
  return node;
}

/* Sets the pointer in parent that points to child. */
#define SET_PTR_TO_CHILD(PARENT, CHILD, PREV_VAL, NEXT_VAL) do {	\
    if (CHILD == PARENT->prev)						\
      PARENT->prev = PREV_VAL;						\
    else {								\
      DO_IF_DEBUG(							\
	if (CHILD != PARENT->next)					\
	  rb_fatal (PARENT, "Got invalid parent to %p "			\
		    "(stack probably wrong).\n", CHILD);		\
      );								\
      PARENT->next = NEXT_VAL;						\
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
static INLINE struct rb_node_hdr *rot_right (struct rb_node_hdr *node)
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
static INLINE struct rb_node_hdr *rot_left (struct rb_node_hdr *node)
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
#ifdef RB_STATS
    rb_add_rebalance_cnt++;
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

/* Returns the root node, which might have changed. The rbstack
 * pointed to by rbstack_ptr goes down to the parent of the node that
 * was unlinked. node is the value of the pointer in the parent that
 * used to point to the unlinked node.
 *
 * *rbstack_ptr is freed if keep_rbstack is zero, otherwise it's set
 * to the new stack down to the parent node. */
static struct rb_node_hdr *rebalance_after_delete (struct rb_node_hdr *node,
						   struct rbstack_ptr *rbstack_ptr,
						   int keep_rbstack)
{
  struct rb_node_hdr *parent, *sibling, *top = node;
  struct rbstack_ptr rbstack = *rbstack_ptr, rbstack_top;

  if (keep_rbstack) {
    rbstack_top = rbstack;
    RBSTACK_UP (rbstack, parent);
  }
  else
    RBSTACK_POP (rbstack, parent);

  if (!parent) {
    if (!node) return NULL;
    node->flags &= ~RB_RED;
  }
  else do {
    top = parent;
#ifdef RB_STATS
    rb_add_rebalance_cnt++;
#endif

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
	top = RBSTACK_PEEK (rbstack);
	if (top)
	  SET_PTR_TO_CHILD (top, parent, sibling, sibling);
	else
	  top = sibling;
	if (keep_rbstack) rbstack_insert (&rbstack_top, &rbstack, sibling);
	else RBSTACK_PUSH (rbstack, sibling);
#ifdef PIKE_DEBUG
	if (parent->flags & RB_THREAD_NEXT)
	  rb_fatal (parent, "Sibling in next is null; tree was unbalanced.\n");
#endif
	sibling = parent->next;
	goto prev_node_red_parent;
      }

      else if (!(parent->flags & RB_RED)) {
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
	  if (keep_rbstack) RBSTACK_UP (rbstack, parent);
	  else RBSTACK_POP (rbstack, parent);
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
	 *         parent(?)                        parent(?)
	 *          /    \                           /    \
	 *         /      \                         /      \
	 *  *node(2B)    sibling(B)          *node(2B)  new sibling(B)
	 *                /    \                           /    \
	 *               /      \      ==>                /      \
	 *      new sibling(R)  (B)                     (B)     sibling(R)
	 *          /    \                                       /    \
	 *         /      \                                     /      \
	 *       (B)      (B)                                 (B)      (B)
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
      assert (!(sibling->flags & RB_RED));
      rot_left (parent);
      if (keep_rbstack) top = RBSTACK_PEEK (rbstack);
      else RBSTACK_POP (rbstack, top);
      if (top) {
	SET_PTR_TO_CHILD (top, parent, sibling, sibling);
	if (parent->flags & RB_RED) {
	  sibling->flags |= RB_RED;
	  parent->flags &= ~RB_RED;
	}
      }
      else {
	top = sibling;
	/* parent was the root, which always is black. */
	assert (!(parent->flags & RB_RED));
      }
      sibling->next->flags &= ~RB_RED;
      if (keep_rbstack) {
	/* Keep rbstack above the inserted element, so that we don't
	 * need to use RBSTACK_UP. */
	struct rbstack_ptr tmp = rbstack;
	rbstack_insert (&rbstack_top, &tmp, sibling);
      }
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
	top = RBSTACK_PEEK (rbstack);
	if (top)
	  SET_PTR_TO_CHILD (top, parent, sibling, sibling);
	else
	  top = sibling;
	if (keep_rbstack) rbstack_insert (&rbstack_top, &rbstack, sibling);
	else RBSTACK_PUSH (rbstack, sibling);
#ifdef PIKE_DEBUG
	if (parent->flags & RB_THREAD_PREV)
	  rb_fatal (parent, "Sibling in prev is null; tree was unbalanced.\n");
#endif
	sibling = parent->prev;
	goto next_node_red_parent;
      }

      else if (!(parent->flags & RB_RED)) {
	if ((sibling->flags & RB_THREAD_PREV || !(sibling->prev->flags & RB_RED)) &&
	    (sibling->flags & RB_THREAD_NEXT || !(sibling->next->flags & RB_RED))) {
	  /* Case 2a */
	  sibling->flags |= RB_RED;
	  node = parent;
	  if (keep_rbstack) RBSTACK_UP (rbstack, parent);
	  else RBSTACK_POP (rbstack, parent);
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
      assert (!(sibling->flags & RB_RED));
      rot_right (parent);
      if (keep_rbstack) top = RBSTACK_PEEK (rbstack);
      else RBSTACK_POP (rbstack, top);
      if (top) {
	SET_PTR_TO_CHILD (top, parent, sibling, sibling);
	if (parent->flags & RB_RED) {
	  sibling->flags |= RB_RED;
	  parent->flags &= ~RB_RED;
	}
      }
      else {
	top = sibling;
	assert (!(parent->flags & RB_RED));
      }
      sibling->prev->flags &= ~RB_RED;
      if (keep_rbstack) {
	struct rbstack_ptr tmp = rbstack;
	rbstack_insert (&rbstack_top, &tmp, sibling);
      }
      break;
    }
  } while (parent);

  if (keep_rbstack) {
    RBSTACK_UP_TO_ROOT (rbstack, top);
    *rbstack_ptr = rbstack_top;
  }
  else {
    RBSTACK_FREE_SET_ROOT (rbstack, top);
    *rbstack_ptr = rbstack;
  }

#ifdef PIKE_DEBUG
  if (top->flags & RB_RED) rb_fatal (top, "Root node not black.\n");
#endif
  return top;
}

void low_rb_init_root (struct rb_node_hdr *node)
{
#ifdef PIKE_DEBUG
  if (!node) Pike_fatal ("New node is null.\n");
#endif
#ifdef RB_STATS
  rb_num_adds++;
#endif
  node->flags = (node->flags & ~RB_RED) | RB_THREAD_PREV | RB_THREAD_NEXT;
  node->prev = node->next = NULL;
}

/* The passed stack is freed. */
void low_rb_link_at_prev (struct rb_node_hdr **root, struct rbstack_ptr rbstack,
			  struct rb_node_hdr *new)
{
  struct rb_node_hdr *parent = RBSTACK_PEEK (rbstack);

#ifdef PIKE_DEBUG
  if (!new) Pike_fatal ("New node is null.\n");
  if (!parent) Pike_fatal ("Cannot link in root node.\n");
  if (!(parent->flags & RB_THREAD_PREV))
    Pike_fatal ("Cannot link in node at interior prev link.\n");
#endif
#ifdef RB_STATS
  rb_num_adds++;
#endif

  new->flags |= RB_RED | RB_THREAD_PREV | RB_THREAD_NEXT;
  new->prev = parent->prev;
  new->next = parent;
  parent->flags &= ~RB_THREAD_PREV;
  parent->prev = new;

  *root = rebalance_after_add (new, rbstack);
}

/* The passed stack is freed. */
void low_rb_link_at_next (struct rb_node_hdr **root, struct rbstack_ptr rbstack,
			  struct rb_node_hdr *new)
{
  struct rb_node_hdr *parent = RBSTACK_PEEK (rbstack);

#ifdef PIKE_DEBUG
  if (!new) Pike_fatal ("New node is null.\n");
  if (!parent) Pike_fatal ("Cannot link in root node.\n");
  if (!(parent->flags & RB_THREAD_NEXT))
    Pike_fatal ("Cannot link in node at interior next link.\n");
#endif
#ifdef RB_STATS
  rb_num_adds++;
#endif

  new->flags |= RB_RED | RB_THREAD_PREV | RB_THREAD_NEXT;
  new->prev = parent;
  new->next = parent->next;
  parent->flags &= ~RB_THREAD_NEXT;
  parent->next = new;

  *root = rebalance_after_add (new, rbstack);
}

#define DO_SIMPLE_UNLINK(UNLINK, PARENT, NODE)				\
  do {									\
    PARENT = RBSTACK_PEEK (rbstack);					\
									\
    if (UNLINK->flags & RB_THREAD_PREV) {				\
      if (UNLINK->flags & RB_THREAD_NEXT) {				\
	if (PARENT)							\
	  SET_PTR_TO_CHILD (						\
	    PARENT, UNLINK,						\
	    (PARENT->flags |= RB_THREAD_PREV, NODE = UNLINK->prev),	\
	    (PARENT->flags |= RB_THREAD_NEXT, NODE = UNLINK->next));	\
	else								\
	  NODE = NULL;							\
	break; /* We're done. */					\
      }									\
									\
      else {			/* prev is null. */			\
	NODE = UNLINK->next;						\
	DO_IF_DEBUG (							\
	  /* Since UNLINK->prev is null, UNLINK->next must be red */	\
	  /* and can't have children, or else the tree is */		\
	  /* unbalanced. */						\
	  if (!(NODE->flags & RB_THREAD_PREV))				\
	    rb_fatal (PARENT, "Expected thread prev pointer in %p; "	\
		      "tree is unbalanced.\n", NODE);			\
	);								\
	NODE->prev = UNLINK->prev;					\
      }									\
    }									\
									\
    else {			/* next is null. */			\
      NODE = UNLINK->prev;						\
      DO_IF_DEBUG (							\
	if (!(NODE->flags & RB_THREAD_NEXT))				\
	  rb_fatal (PARENT, "Expected thread next pointer in %p; "	\
		    "tree is unbalanced.\n", NODE);			\
      );								\
      NODE->next = UNLINK->next;					\
    }									\
									\
    if (PARENT) SET_PTR_TO_CHILD (PARENT, UNLINK, NODE, NODE);		\
  } while (0)

#define ADJUST_STACK_TO_NEXT(RBSTACK, NEXT) do {			\
    struct rb_node_hdr *node = RBSTACK_PEEK (RBSTACK);			\
    if (NEXT && (!node ||						\
		 (!(node->flags & RB_THREAD_PREV) && node->prev == NEXT) || \
		 (!(node->flags & RB_THREAD_NEXT) && node->next == NEXT))) { \
      RBSTACK_PUSH (RBSTACK, NEXT);					\
      while (!(NEXT->flags & RB_THREAD_PREV))				\
	RBSTACK_PUSH (RBSTACK, NEXT = NEXT->prev);			\
    }									\
    else								\
      while (node != NEXT) {						\
	RBSTACK_POP (RBSTACK, node);					\
	assert (!NEXT || node != NULL);					\
	node = RBSTACK_PEEK (RBSTACK);					\
      }									\
  } while (0)

/* The node to unlink is the one on top of the stack pointed to by
 * rbstack_ptr. If that node can't be unlinked (i.e. it got two
 * children), another node that can is found and its contents is
 * copied to the first one. Returns the node that actually got
 * unlinked.
 *
 * *rbstack_ptr is freed if keep_rbstack is zero, otherwise it's set
 * to the new stack down to the next node from the one that was
 * removed.
 *
 * See rb_remove_with_move about node_size. */
struct rb_node_hdr *low_rb_unlink_with_move (struct rb_node_hdr **root,
					     struct rbstack_ptr *rbstack_ptr,
					     int keep_rbstack,
					     size_t node_size)
{
  struct rb_node_hdr *node, *parent, *unlink, *next;
  struct rbstack_ptr rbstack = *rbstack_ptr;

  RBSTACK_POP (rbstack, node);
#ifdef PIKE_DEBUG
  if (!node) Pike_fatal ("No node to delete on stack.\n");
#endif
#ifdef RB_STATS
  rb_num_deletes++;
#endif

  if (node->flags & (RB_THREAD_PREV|RB_THREAD_NEXT)) {
    next = node->next;
    unlink = node;
    DO_SIMPLE_UNLINK (unlink, parent, node);
  }

  else {
    /* Node has two subtrees, so we can't delete it. Find another one
     * to replace its data with. */
    next = parent = node;
    RBSTACK_PUSH (rbstack, node);
    for (unlink = node->next; !(unlink->flags & RB_THREAD_PREV); unlink = unlink->prev) {
      parent = unlink;
      RBSTACK_PUSH (rbstack, unlink);
    }

    keep_flags (node,
		MEMCPY ((char *) node + OFFSETOF (rb_node_hdr, flags),
			(char *) unlink + OFFSETOF (rb_node_hdr, flags),
			node_size - OFFSETOF (rb_node_hdr, flags)));

    if (parent == node) {
      node = unlink->next;
      if (unlink->flags & RB_THREAD_NEXT)
	parent->flags |= RB_THREAD_NEXT;
      else {
#ifdef PIKE_DEBUG
	if (!(node->flags & RB_THREAD_PREV))
	  rb_fatal (parent, "Expected thread prev pointer in %p; "
		    "tree is unbalanced.\n", node);
#endif
	node->prev = unlink->prev;
      }
      parent->next = node;
    }

    else
      if (unlink->flags & RB_THREAD_NEXT) {
	parent->flags |= RB_THREAD_PREV;
	parent->prev = node = unlink->prev;
      }
      else {
	node = unlink->next;
#ifdef PIKE_DEBUG
	if (!(node->flags & RB_THREAD_PREV))
	  rb_fatal (parent, "Expected thread prev pointer in %p; "
		    "tree is unbalanced.\n", node);
#endif
	node->prev = unlink->prev;
	parent->prev = node;
      }
  }

  if (unlink->flags & RB_RED) {
    if (!keep_rbstack) RBSTACK_FREE (rbstack);
  }
  else
    *root = rebalance_after_delete (node, &rbstack, keep_rbstack);

  if (keep_rbstack) ADJUST_STACK_TO_NEXT (rbstack, next);
  *rbstack_ptr = rbstack;
  return unlink;
}

#if 0
struct rb_node_hdr *low_rb_unlink_with_move (struct rb_node_hdr **root,
					     struct rbstack_ptr *rbstack_ptr,
					     int keep_rbstack,
					     size_t node_size)
{
  struct rb_node_hdr *node = RBSTACK_PEEK (*rbstack_ptr), *next = rb_next (node);
  struct rb_node_hdr *unlink =
    real_low_rb_unlink_with_move (root, rbstack_ptr, 1, node_size);
  debug_check_rbstack (*root, *rbstack_ptr);
  if (node != unlink) next = node;
  if (RBSTACK_PEEK (*rbstack_ptr) != next)
    Pike_fatal ("Stack got %p on top, but next node is %p.\n",
		RBSTACK_PEEK (*rbstack_ptr), next);
  if (!keep_rbstack) RBSTACK_FREE (*rbstack_ptr);
  return unlink;
}
#endif

/* Like low_rb_unlink_with_move, but relinks the nodes instead of
 * moving over the data. Somewhat slower unless the size of the nodes
 * is large. */
void low_rb_unlink_without_move (struct rb_node_hdr **root,
				 struct rbstack_ptr *rbstack_ptr,
				 int keep_rbstack)
{
  struct rb_node_hdr *node, *parent, *unlink, *next;
  struct rbstack_ptr rbstack = *rbstack_ptr;
  int unlink_flags;

  RBSTACK_POP (rbstack, node);
#ifdef PIKE_DEBUG
  if (!node) Pike_fatal ("No node to delete on stack.\n");
#endif
#ifdef RB_STATS
  rb_num_deletes++;
#endif

  if (node->flags & (RB_THREAD_PREV|RB_THREAD_NEXT)) {
    next = node->next;
    unlink = node;
    DO_SIMPLE_UNLINK (unlink, parent, node);
    unlink_flags = unlink->flags;
  }

  else {
    /* Node has two subtrees, so we can't delete it. Find another one
     * to switch places with. */
    struct rb_node_hdr *orig_parent = RBSTACK_PEEK (rbstack), *tmp;
    struct rbstack_ptr pos;
    parent = unlink = node;
    RBSTACK_PUSH (rbstack, NULL);
    pos = rbstack;
    for (node = node->next; !(node->flags & RB_THREAD_PREV); node = node->prev) {
      parent = node;
      RBSTACK_PUSH (rbstack, node);
    }
    RBSTACK_POKE (pos, next = node);
    unlink_flags = node->flags;

    if (orig_parent) SET_PTR_TO_CHILD (orig_parent, unlink, node, node);
    else *root = node;
    for (tmp = unlink->prev; !(tmp->flags & RB_THREAD_NEXT); tmp = tmp->next) {}
    tmp->next = node;
    node->prev = unlink->prev;

    if (parent == unlink) {	/* node replaces its parent. */
      node->flags =		/* Keep the thread next flag. */
	(node->flags & ~(RB_FLAG_MASK & ~RB_THREAD_NEXT)) |
	(unlink->flags & (RB_FLAG_MASK & ~RB_THREAD_NEXT));
      parent = node;
      node = parent->next;
    }

    else {
      if (node->flags & RB_THREAD_NEXT) {
	parent->flags |= RB_THREAD_PREV;
	parent->prev = node;
      }
      else parent->prev = node->next;
      node->flags = (node->flags & ~RB_FLAG_MASK) | (unlink->flags & RB_FLAG_MASK);
      node->next = unlink->next;
      node = parent->prev;
    }
  }

  if (unlink_flags & RB_RED) {
    if (!keep_rbstack) RBSTACK_FREE (rbstack);
  }
  else
    *root = rebalance_after_delete (node, &rbstack, keep_rbstack);

  if (keep_rbstack) ADJUST_STACK_TO_NEXT (rbstack, next);
  *rbstack_ptr = rbstack;
}

#if 0
void low_rb_unlink_without_move (struct rb_node_hdr **root,
				 struct rbstack_ptr *rbstack_ptr,
				 int keep_rbstack)
{
  struct rb_node_hdr *node = RBSTACK_PEEK (*rbstack_ptr), *next = rb_next (node);
  real_low_rb_unlink_without_move (root, rbstack_ptr, 1);
  debug_check_rbstack (*root, *rbstack_ptr);
  if (RBSTACK_PEEK (*rbstack_ptr) != next)
    Pike_fatal ("Stack got %p on top, but next node is %p.\n",
		RBSTACK_PEEK (*rbstack_ptr), next);
  if (!keep_rbstack) RBSTACK_FREE (*rbstack_ptr);
}
#endif

#ifdef ENABLE_UNUSED_RB_FUNCTIONS
/* These functions are disabled since they aren't used from anywhere.
 * If you like to use them, just let me know (they are tested). /mast */

/* Does not replace an existing equal node. Returns the node in the
 * tree. It might not be the passed new node if an equal one was
 * found. */
struct rb_node_hdr *rb_insert (struct rb_node_hdr **root,
			       rb_find_fn *find_fn, void *key,
			       struct rb_node_hdr *new)
{
  if (*root) {
    struct rb_node_hdr *node = *root;
    RBSTACK_INIT (rbstack);
    LOW_RB_TRACK (
      rbstack, node,
      cmp_res = find_fn (key, node),
      {				/* Got less. */
	low_rb_link_at_next (root, rbstack, new);
      }, {			/* Got equal - new not used. */
	RBSTACK_FREE (rbstack);
	return node;
      }, {			/* Got greater. */
	low_rb_link_at_prev (root, rbstack, new);
      });
  }
  else {
    low_rb_init_root (new);
    *root = new;
  }
  return new;
}

void rb_add (struct rb_node_hdr **root,
	     rb_find_fn *find_fn, void *key,
	     struct rb_node_hdr *new)
{
  if (*root) {
    struct rb_node_hdr *node = *root;
    RBSTACK_INIT (rbstack);
    LOW_RB_TRACK_NEQ (
      rbstack, node, cmp_res = find_fn (key, node) >= 0 ? 1 : -1,
      low_rb_link_at_next (root, rbstack, new), /* Got less or equal. */
      low_rb_link_at_prev (root, rbstack, new)	/* Got greater. */
    );
  }
  else {
    low_rb_init_root (new);
    *root = new;
  }
}

void rb_add_after (struct rb_node_hdr **root,
		   rb_find_fn *find_fn, void *key,
		   struct rb_node_hdr *new,
		   struct rb_node_hdr *existing)
{
  if (*root) {
    struct rb_node_hdr *node = *root;
    RBSTACK_INIT (rbstack);

    if (existing) {
      LOW_RB_TRACK_NEQ (
	rbstack, node, cmp_res = find_fn (key, node) >= 0 ? 1 : -1, ;, ;);
      while (node != existing) {
	LOW_RB_TRACK_PREV (rbstack, node);
#ifdef PIKE_DEBUG
	if (!node) Pike_fatal ("Tree doesn't contain the existing node.\n");
#endif
      }
      if (node->flags & RB_THREAD_NEXT) {
	low_rb_link_at_next (root, rbstack, new);
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
    low_rb_link_at_prev (root, rbstack, new);
  }

  else {
#ifdef PIKE_DEBUG
    if (existing) Pike_fatal ("Tree doesn't contain the existing node.\n");
#endif
    low_rb_init_root (new);
    *root = new;
  }
}

/* Returns the node to free, if any. Note that rb_remove_with_move
 * is usually faster if the node size is small. */
struct rb_node_hdr *rb_remove (struct rb_node_hdr **root,
			       rb_find_fn *find_fn, void *key)
{
  struct rb_node_hdr *node = *root;
  if (node) {
    RBSTACK_INIT (rbstack);
    LOW_RB_TRACK (
      rbstack, node,
      cmp_res = find_fn (key, node),
      {				/* Got less. */
      }, {			/* Got equal. */
	low_rb_unlink_without_move (root, rbstack);
	return node;
      }, {			/* Got greater. */
      });
    RBSTACK_FREE (rbstack);
  }
  return NULL;
}

void rb_remove_node (struct rb_node_hdr **root,
		     rb_find_fn *find_fn, void *key,
		     struct rb_node_hdr *to_delete)
{
  struct rb_node_hdr *node = *root;
  RBSTACK_INIT (rbstack);
#ifdef PIKE_DEBUG
  if (!node) Pike_fatal ("Tree is empty.\n");
#if 0
  if (find_fn (key, to_delete))
    Pike_fatal ("Given key doesn't match the node to delete.\n");
#endif
#endif
  LOW_RB_TRACK_NEQ (rbstack, node, cmp_res = find_fn (key, node) >= 0 ? 1 : -1, ;, ;);
  while (node != to_delete) {
    LOW_RB_TRACK_PREV (rbstack, node);
#ifdef PIKE_DEBUG
    if (!node) Pike_fatal ("Tree doesn't contain the node to delete.\n");
#endif
  }
  low_rb_unlink_without_move (root, rbstack);
}

/* Variant of rb_remove that moves the contents of another node to the
 * one to be removed if it can't be unlinked. node_size is the size of
 * the whole node, including rb_node_hdr. If cleanup_fn isn't NULL,
 * it's called for the node whose contents is removed, while the node
 * being returned is the one to actually free. */
struct rb_node_hdr *rb_remove_with_move (struct rb_node_hdr **root,
					 rb_find_fn *find_fn, void *key,
					 size_t node_size,
					 rb_free_fn *cleanup_fn,
					 void *cleanup_fn_extra)
{
  struct rb_node_hdr *node = *root;
  if (node) {
    RBSTACK_INIT (rbstack);
    LOW_RB_TRACK (
      rbstack, node,
      cmp_res = find_fn (key, node),
      {				/* Got less. */
      }, {			/* Got equal. */
	if (cleanup_fn) cleanup_fn (node, cleanup_fn_extra);
	return low_rb_unlink_with_move (root, rbstack, node_size);
      }, {			/* Got greater. */
      });
    RBSTACK_FREE (rbstack);
  }
  return NULL;
}

/* Returns the node to actually free. See rb_remove about
 * node_size. */
struct rb_node_hdr *rb_remove_node_with_move (struct rb_node_hdr **root,
					      rb_find_fn *find_fn, void *key,
					      struct rb_node_hdr *to_delete,
					      size_t node_size)
{
  struct rb_node_hdr *node = *root;
  RBSTACK_INIT (rbstack);
#ifdef PIKE_DEBUG
  if (!node) Pike_fatal ("Tree is empty.\n");
#if 0
  if (find_fn (key, to_delete))
    Pike_fatal ("Given key doesn't match the node to delete.\n");
#endif
#endif
  LOW_RB_TRACK_NEQ (rbstack, node, cmp_res = find_fn (key, node) >= 0 ? 1 : -1, ;, ;);
  while (node != to_delete) {
    LOW_RB_TRACK_PREV (rbstack, node);
#ifdef PIKE_DEBUG
    if (!node) Pike_fatal ("Tree doesn't contain the node to delete.\n");
#endif
  }
  return low_rb_unlink_with_move (root, rbstack, node_size);
}

struct rb_node_hdr *rb_find_eq (struct rb_node_hdr *root,
				rb_find_fn *find_fn, void *key)
{
  if (root)
    LOW_RB_FIND (root, cmp_res = find_fn (key, root), ;, return root, ;);
  return NULL;
}

struct rb_node_hdr *rb_find_lt (struct rb_node_hdr *root,
				rb_find_fn *find_fn, void *key)
{
  if (root)
    LOW_RB_FIND_NEQ (root, cmp_res = find_fn (key, root) <= 0 ? -1 : 1,
		     return root, return root->prev);
  return NULL;
}

struct rb_node_hdr *rb_find_gt (struct rb_node_hdr *root,
				rb_find_fn *find_fn, void *key)
{
  if (root)
    LOW_RB_FIND_NEQ (root, cmp_res = find_fn (key, root) >= 0 ? 1 : -1,
		     return root->next, return root);
  return NULL;
}

struct rb_node_hdr *rb_find_le (struct rb_node_hdr *root,
				rb_find_fn *find_fn, void *key)
{
  if (root)
    LOW_RB_FIND_NEQ (root, cmp_res = find_fn (key, root) >= 0 ? 1 : -1,
		     return root, return root->prev);
  return NULL;
}

struct rb_node_hdr *rb_find_ge (struct rb_node_hdr *root,
				rb_find_fn *find_fn, void *key)
{
  if (root)
    LOW_RB_FIND_NEQ (root, cmp_res = find_fn (key, root) <= 0 ? -1 : 1,
		     return root->next, return root);
  return NULL;
}

#endif	/* ENABLE_UNUSED_RB_FUNCTIONS */

struct rb_node_hdr *rb_get_nth (struct rb_node_hdr *root, size_t n)
{
#ifdef PIKE_DEBUG
  size_t index = n;
#endif
  if ((root = rb_first (root))) {
    while (n) {
      n--;
      root = rb_next (root);
#ifdef PIKE_DEBUG
      if (!root) goto tree_too_small;
#endif
    }
    return root;
  }
#ifdef PIKE_DEBUG
tree_too_small:
  Pike_fatal ("Tree too small for index %"PRINTSIZET"u.\n", index);
#endif
  return (struct rb_node_hdr *) (ptrdiff_t) -1;
}

size_t rb_sizeof (struct rb_node_hdr *root)
{
  size_t size = 0;
  if ((root = rb_first (root))) {
    do {
      size++;
    } while ((root = rb_next (root)));
  }
  return size;
}

#ifdef ENABLE_UNUSED_RB_FUNCTIONS

void rb_free (struct rb_node_hdr *root, rb_free_fn *free_node_fn, void *extra)
{
  if ((root = rb_first (root))) {
    struct rb_node_hdr *next;
    do {
      next = rb_next (root);
      free_node_fn (root, extra);
      root = next;
    } while (root);
  }
}

int rb_equal (struct rb_node_hdr *a, struct rb_node_hdr *b,
	      rb_equal_fn *node_equal_fn, void *extra)
{
  a = rb_first (a);
  b = rb_first (b);
  while (a && b) {
    if (!node_equal_fn (a, b, extra)) return 0;
    a = rb_next (a);
    b = rb_next (b);
  }
  return !(a || b);
}

struct rb_node_hdr *rb_copy (struct rb_node_hdr *source,
			     rb_copy_fn *copy_node_fn, void *extra)
{
  if (source) {
    struct rb_node_hdr *copy, *target, *new, *t_prev_tgt = NULL, *t_next_src = NULL;
    RBSTACK_INIT (s_stack);
    RBSTACK_INIT (t_stack);

    copy = target = copy_node_fn (source, extra);
    copy->flags = (copy->flags & ~RB_FLAG_MASK) | (source->flags & RB_FLAG_MASK);

    LOW_RB_TRAVERSE (
      1, s_stack, source,
      {				/* Push. */
      }, {			/* prev is leaf. */
	target->prev = t_prev_tgt;
      }, {			/* prev is subtree. */
	new = target->prev = copy_node_fn (source->prev, extra);
	new->flags = (new->flags & ~RB_FLAG_MASK) | (source->prev->flags & RB_FLAG_MASK);
	RBSTACK_PUSH (t_stack, target);
	target = new;
      }, {			/* Between. */
	t_prev_tgt = target;
	if (t_next_src) {
	  t_next_src->next = target;
	  t_next_src = NULL;
	}
      }, {			/* next is leaf. */
	t_next_src = target;
      }, {			/* next is subtree. */
	new = target->next = copy_node_fn (source->next, extra);
	new->flags = (new->flags & ~RB_FLAG_MASK) | (source->next->flags & RB_FLAG_MASK);
	RBSTACK_PUSH (t_stack, target);
	target = new;
      }, {			/* Pop. */
	RBSTACK_POP (t_stack, target);
      });

    if (t_next_src) t_next_src->next = NULL;
    return copy;
  }
  else return NULL;
}

#endif /* ENABLE_UNUSED_RB_FUNCTIONS */

/* Converts a tree to an ordered list of nodes linked by the next
 * pointers. */
struct rb_node_hdr *rb_make_list (struct rb_node_hdr *tree)
{
  struct rb_node_hdr *list = tree = rb_first (tree);
  if (tree)
    while ((tree = tree->next = rb_next (tree))) {}
  return list;
}

/* Converts a list of nodes linked by the next pointers to a perfectly
 * balanced tree (with a minimum of black nodes). Time complexity is
 * O(length). Returns the tree root. */
struct rb_node_hdr *rb_make_tree (struct rb_node_hdr *list, size_t length)
{
  struct rb_node_hdr *root = NULL;

  if (length > 1) {
    unsigned depth, *top_idx;
    size_t deep_end, count = 0, start = 0, idx = 1;
    struct rb_node_hdr **stack, *prev_tgt = NULL, *next_src = NULL;
    int stack_malloced = 0;

    {
      unsigned l = 0, h = sizeof (length) * CHAR_BIT;
      do {
	depth = (l + h) >> 1;
	if ((size_t) 1 << depth <= length)
	  l = depth + 1;
	else
	  h = depth;
      } while (l < h);
      if ((size_t) 1 << depth <= length) depth++;
    }

    deep_end = (length - ((size_t) 1 << (depth - 1))) << 1;
    if (!(length & 1)) deep_end |= 1;

    {
      size_t stack_size =
	depth * sizeof (struct rb_node_hdr *) +
	(depth + 1) * sizeof (unsigned);
#ifdef HAVE_ALLOCA
      stack = alloca (stack_size);
      if (!stack)
#endif
      {
	stack = xalloc (stack_size);
	stack_malloced = 1;
      }
      top_idx = (unsigned *) (stack + depth);
      memset (top_idx, 0, (depth + 1) * sizeof (unsigned));
      depth--;
    }

    while (count < length) {
      struct rb_node_hdr *next;
#ifdef PIKE_DEBUG
      if (!list) Pike_fatal ("Premature end of list at %"PRINTSIZET"u, "
			     "expected %"PRINTSIZET"u.\n", count, length);
#endif
      next = list->next;

      if (idx > start) {	/* Leaf node. */
	idx = start;
	list->flags |= RB_THREAD_PREV|RB_THREAD_NEXT;
	next_src = stack[idx] = list;
	list->prev = prev_tgt;
      }
      else {			/* Interior node. */
	idx = top_idx[idx];
	list->flags &= ~(RB_THREAD_PREV|RB_THREAD_NEXT);
	prev_tgt = next_src->next = stack[idx] = list;
	list->prev = stack[idx - 1];
	if (idx == depth) {
	  assert (!root);
	  root = list;
	}
      }
      assert (idx <= depth);

      if (count++ == deep_end) {
	if (idx) {		/* Interior node is "half a leaf". */
	  next_src = list;
	  list->flags |= RB_THREAD_NEXT;
	}
	start = 1;
      }

      if (idx & 1) list->flags &= ~RB_RED;
      else list->flags |= RB_RED;

      if (top_idx[idx] == top_idx[idx + 1])
	top_idx[idx] = idx + 1;
      else {
	top_idx[idx] = top_idx[idx + 1];
	stack[idx + 1]->next = list;
      }

      list = next;
    }

    assert (deep_end == length - 1 && (length & 1) ? idx == 0 : idx == start);
#ifdef PIKE_DEBUG
    for (; idx <= depth; idx++)
      assert (top_idx[idx] == depth + 1);
#endif
    assert(root);

    next_src->next = NULL;
    root->flags &= ~RB_RED;
    if (stack_malloced) xfree (stack);
  }
  else if (length) {
    root = list;
#ifdef PIKE_DEBUG
    list = list->next;
#endif
    low_rb_init_root (root);
  }

#ifdef PIKE_DEBUG
  if (list) Pike_fatal ("List longer than expected %d.\n", length);
#endif
  return root;
}

#ifdef ENABLE_UNUSED_RB_FUNCTIONS

struct linear_merge_cleanup
{
  int operation;
  struct rb_node_hdr *a, *b, *res;
  rb_merge_free_fn *free_node_fn;
  void *extra;
};

static void do_linear_merge_cleanup (struct linear_merge_cleanup *s)
{
  struct rb_node_hdr *node, *next;
  rb_merge_free_fn *free_node_fn = s->free_node_fn;
  void *extra = s->extra;

  for (node = s->res; node; node = next) {
    next = node->next;
    free_node_fn (node, extra, MERGE_TREE_RES);
  }

  if (s->operation & PIKE_MERGE_DESTR_A)
    for (node = s->a; node; node = next) {
      next = rb_prev (node);
      free_node_fn (node, extra, MERGE_TREE_A);
    }

  if (s->operation & PIKE_MERGE_DESTR_B)
    for (node = s->b; node; node = next) {
      next = rb_prev (node);
      free_node_fn (node, extra, MERGE_TREE_B);
    }
}

/* Merges two trees in linear time (no more, no less). The operation
 * argument describes the way to merge, like the one given to merge()
 * in array.c. Two extra bits, PIKE_MERGE_DESTR_A and
 * PIKE_MERGE_DESTR_B, tells whether or not to be destructive on the
 * two passed trees. If destructive on a tree, the remaining nodes in
 * it will be freed with free_node_fn, otherwise copy_node_fn will be
 * used to copy nodes from it to the result. The two callbacks will
 * receive either MERGE_TREE_A or MERGE_TREE_B as the third arg,
 * depending on whether the node argument comes from tree a or tree b,
 * respectively. free_node_fn might also get MERGE_TREE_RES to clean
 * up any newly created node copies in the result, in case of
 * exception.
 *
 * The function returns a list linked by the next pointers, and if
 * length is nonzero it'll write the length of the list there. The
 * caller can then use these as arguments to rb_make_tree.
 *
 * NB: It doesn't handle making duplicates of the same node, i.e.
 * PIKE_ARRAY_OP_A without PIKE_ARRAY_OP_SKIP_A. Not a problem since
 * none of the currently defined operations use that.
 *
 * NB: It's assumed that a and b aren't part of the same tree. */
struct rb_node_hdr *rb_linear_merge (
  struct rb_node_hdr *a, struct rb_node_hdr *b, int operation,
  rb_cmp_fn *cmp_fn, void *cmp_fn_extra,
  rb_merge_copy_fn *copy_node_fn, void *copy_fn_extra,
  rb_merge_free_fn *free_node_fn, void *free_fn_extra,
  size_t *length)
{
  struct rb_node_hdr *n;
  size_t len = 0;
  int cmp_res, op;
  struct linear_merge_cleanup s;
  ONERROR uwp;

  s.operation = operation;
  s.free_node_fn = free_node_fn;
  s.extra = free_fn_extra;
  SET_ONERROR (uwp, do_linear_merge_cleanup, &s);

  LOW_RB_MERGE (
    x, s.a, s.b, s.res, len, operation, ;, ;,
    {
      cmp_res = cmp_fn (s.a, s.b, cmp_fn_extra);
    }, {			/* Copy a. */
      new_node = operation & PIKE_MERGE_DESTR_A ? s.a :
	copy_node_fn (s.a, copy_fn_extra, MERGE_TREE_A);
    }, {			/* Free a. */
      if (operation & PIKE_MERGE_DESTR_A)
	free_node_fn (s.a, free_fn_extra, MERGE_TREE_A);
    }, {			/* Copy b. */
      new_node = operation & PIKE_MERGE_DESTR_B ? s.b :
	copy_node_fn (s.b, copy_fn_extra, MERGE_TREE_B);
    }, {			/* Free b. */
      if (operation & PIKE_MERGE_DESTR_B)
	free_node_fn (s.b, free_fn_extra, MERGE_TREE_B);
    });

  UNSET_ONERROR (uwp);

  if (length) *length = len;
  return s.res;
}

#endif	/* ENABLE_UNUSED_RB_FUNCTIONS */

#ifdef RB_STATS

size_t rb_num_finds = 0, rb_find_depth = 0;
size_t rb_num_sidesteps = 0, rb_num_sidestep_ops = 0;
size_t rb_num_tracks = 0, rb_track_depth = 0;
size_t rb_num_sidetracks = 0, rb_num_sidetrack_ops = 0;
size_t rb_max_depth = 0;
size_t rb_num_traverses = 0, rb_num_traverse_ops = 0;
size_t rbstack_slice_allocs = 0;
size_t rb_num_adds = 0, rb_add_rebalance_cnt = 0;
size_t rb_num_deletes = 0, rb_del_rebalance_cnt = 0;

void reset_rb_stats()
{
  rb_num_sidesteps = rb_num_sidestep_ops = 0;
  rb_num_finds = rb_find_depth = 0;
  rb_num_tracks = rb_track_depth = 0;
  rb_num_sidetracks = rb_num_sidetrack_ops = 0;
  rb_max_depth = 0;
  rb_num_traverses = rb_num_traverse_ops = 0;
  rbstack_slice_allocs = 0;
  rb_num_adds = rb_add_rebalance_cnt = 0;
  rb_num_deletes = rb_del_rebalance_cnt = 0;
}

void print_rb_stats (int reset)
{
  fprintf (stderr,
	   "rbtree stats:\n"
	   "  number of finds  . . . . . . %10"PRINTSIZET"u\n"
	   "  avg find depth . . . . . . . %10.2f\n"
	   "  number of sidesteps  . . . . %10"PRINTSIZET"u\n"
	   "  avg ops per sidestep . . . . %10.2f\n"
	   "  number of tracks . . . . . . %10"PRINTSIZET"u\n"
	   "  avg track depth  . . . . . . %10.2f\n"
	   "  number of sidetracks . . . . %10"PRINTSIZET"u\n"
	   "  avg ops per sidetrack  . . . %10.2f\n"
	   "  maximum depth  . . . . . . . %10"PRINTSIZET"u\n"
	   "  number of traverses  . . . . %10"PRINTSIZET"u\n"
	   "  avg ops per traverse . . . . %10.2f\n"
	   "  allocated stack slices . . . %10"PRINTSIZET"u\n"
	   "  number of adds . . . . . . . %10"PRINTSIZET"u\n"
	   "  avg add rebalance count  . . %10.2f\n"
	   "  number of deletes  . . . . . %10"PRINTSIZET"u\n"
	   "  avg delete rebalance count . %10.2f\n",
	   rb_num_finds,
	   (double) rb_find_depth / rb_num_finds,
	   rb_num_sidesteps,
	   (double) rb_num_sidestep_ops / rb_num_sidesteps,
	   rb_num_tracks,
	   (double) rb_track_depth / rb_num_tracks,
	   rb_num_sidetracks,
	   (double) rb_num_sidetrack_ops / rb_num_sidetracks,
	   rb_max_depth,
	   rb_num_traverses,
	   (double) rb_num_traverse_ops / rb_num_traverses,
	   rbstack_slice_allocs,
	   rb_num_adds,
	   (double) rb_add_rebalance_cnt / rb_num_adds,
	   rb_num_deletes,
	   (double) rb_del_rebalance_cnt / rb_num_deletes);
  if (reset) reset_rb_stats();
}

#endif	/* RB_STATS */

#if defined (PIKE_DEBUG) || defined (TEST_MULTISET)

void debug_dump_rb_tree (struct rb_node_hdr *root, dump_data_fn *dump_data,
			 void *extra)
{
  if (root) {
    struct rb_node_hdr *node = root;
    struct rb_node_hdr *n, *n2;
    struct rbstack_ptr p;
    size_t depth = 0;
    RBSTACK_INIT (rbstack);

    LOW_RB_TRAVERSE (
      1, rbstack, node,
      {				/* Push. */
	depth++;
	p = rbstack;
	RBSTACK_UP (p, n);
	while (n) {
	  if (n == node) {
	    fprintf (stderr, "[Circular %p]", node);
	    goto skip_node;
	  }
	  RBSTACK_UP (p, n);
	}
	fputc ('(', stderr);
      }, {			/* prev is leaf. */
	p = rbstack;
	n2 = node;
	RBSTACK_UP (p, n);
	while (n && (n->flags & RB_THREAD_NEXT || n->next != n2)) {
	  n2 = n;
	  RBSTACK_UP (p, n);
	}
	if (node->prev != n)
	  fprintf (stderr, "[Thread ptr is %p, expected %p]\n%*s",
		   node->prev, n, depth, "");
      }, {			/* prev is subtree. */
	if (!node->prev) {
	  fputs ("[Zero subtree]", stderr);
	  goto between_1;
	}
      }, {			/* Between nodes. */
	if (!(node->flags & RB_THREAD_PREV))
	  fprintf (stderr, "\n%*s", depth, "");
	if (node == root) fputs ("root=", stderr);
	fprintf (stderr, "%p/%c",
		 node, node->flags & RB_RED ? 'R' : 'B');
	if (dump_data) {
	  fputs (": ", stderr);
	  dump_data (node, extra);
	}
	if (node->prev && node->prev == node->next)
	  fputs (" [prev == next]", stderr);
      }, {			/* next is leaf. */
	p = rbstack;
	n2 = node;
	RBSTACK_UP (p, n);
	while (n && (n->flags & RB_THREAD_PREV || n->prev != n2)) {
	  n2 = n;
	  RBSTACK_UP (p, n);
	}
	if (node->next != n)
	  fprintf (stderr, "\n%*s[Thread ptr is %p, expected %p]",
		   depth, "", node->next, n);
      }, {			/* next is subtree. */
	fprintf (stderr, "\n%*s", depth, "");
	if (!node->next) {
	  fputs ("[Zero subtree]", stderr);
	  goto leave_1;
	}
      }, {			/* Pop. */
	fputc (')', stderr);
      skip_node:
	depth--;
      });
    fputc ('\n', stderr);
  }
  else
    fprintf (stderr, "(empty tree)\n");
}

/* If pos isn't zero, it points to a pointer in the stack that will be
 * marked in the output. */
void debug_dump_rbstack (struct rbstack_ptr rbstack, struct rbstack_ptr *pos)
{
  struct rb_node_hdr *node, *ptr_node = (void *) (ptrdiff_t) -1;
  RBSTACK_INIT (reversed);

  while (1) {
    int here = pos ? pos->ssp == rbstack.ssp && pos->slice == rbstack.slice : 0;
    RBSTACK_UP (rbstack, node);
    if (here) ptr_node = node;
    if (!node) break;
    RBSTACK_PUSH (reversed, node);
  }

  if (pos && !ptr_node) fputs ("* ", stderr);

  while (1) {
    RBSTACK_POP (reversed, node);
    if (!node) break;
    fprintf (stderr, "%p ", node);
    if (pos && ptr_node == node) fputs ("* ", stderr);
  }

  if (pos && ptr_node == (void *) (ptrdiff_t) -1)
    fputs ("(pos outside stack)", stderr);
  fputc ('\n', stderr);
}

static void debug_rb_fatal (struct rb_node_hdr *tree, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  (void) VFPRINTF (stderr, fmt, args);
  fputs ("Dumping tree:\n", stderr);
  debug_dump_rb_tree (tree, NULL, NULL);
  debug_fatal ("\r");
}

static void debug_custom_rb_fatal (struct rb_node_hdr *tree, dump_data_fn *dump_data,
				   void *extra, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  (void) VFPRINTF (stderr, fmt, args);
  fputs ("Dumping tree:\n", stderr);
  debug_dump_rb_tree (tree, dump_data, extra);
  debug_fatal ("\r");
}

void debug_check_rb_tree (struct rb_node_hdr *root, dump_data_fn *dump_data, void *extra)
{
  if (root) {
    struct rb_node_hdr *node = root, *n, *n2;
    struct rbstack_ptr p;
    size_t blacks = 1, max_blacks = 0, depth = 0;
    RBSTACK_INIT (rbstack);

    if (root->flags & RB_RED) custom_rb_fatal (root, dump_data, extra,
					       "Root node not black.\n");

    LOW_RB_TRAVERSE (
      1, rbstack, node,
      {				/* Push. */
	depth++;
	p = rbstack;
	RBSTACK_UP (p, n);
	while (n) {
	  if (n == node) custom_rb_fatal (root, dump_data, extra,
					  "Circular subtrees @ %p.\n", node);
	  RBSTACK_UP (p, n);
	}

	if (!(node->flags & RB_RED)) blacks++;
      }, {			/* prev is leaf. */
	if (max_blacks) {
	  if (blacks != max_blacks)
	    custom_rb_fatal (root, dump_data, extra,
			     "Unbalanced tree - leftmost branch is %d, "
			     "prev @ %p is %d.\n", max_blacks, node, blacks);
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
	  custom_rb_fatal (root, dump_data, extra,
			   "Thread prev pointer @ %p is %p, expected %p.\n",
			   node, node->prev, n);
      }, {			/* prev is subtree. */
	if (!node->prev) custom_rb_fatal (root, dump_data, extra,
					  "Subtree prev is null @ %p.\n", node);
	if (node->flags & RB_RED && node->prev->flags & RB_RED)
	  custom_rb_fatal (root, dump_data, extra,
			   "Red node got red subtree prev node @ %p.\n", node);
      }, {			/* Between nodes. */
      }, {			/* next is leaf. */
	if (blacks != max_blacks)
	  custom_rb_fatal (root, dump_data, extra,
			   "Unbalanced tree - leftmost branch is %d, "
			   "next @ %p is %d.\n", max_blacks, node, blacks);

	p = rbstack;
	n2 = node;
	RBSTACK_UP (p, n);
	while (n && (n->flags & RB_THREAD_PREV || n->prev != n2)) {
	  n2 = n;
	  RBSTACK_UP (p, n);
	}
	if (node->next != n)
	  custom_rb_fatal (root, dump_data, extra,
			   "Thread next pointer @ %p is %p, expected %p.\n",
			   node, node->next, n);
      }, {			/* next is subtree. */
	if (!node->next) custom_rb_fatal (root, dump_data, extra,
					  "Subtree next is null @ %p.\n", node);
	if (node->flags & RB_RED && node->next->flags & RB_RED)
	  custom_rb_fatal (root, dump_data, extra,
			   "Red node got red subtree next node @ %p.\n", node);
      }, {			/* Pop. */
	if (!(node->flags & RB_RED)) blacks--;
	depth--;
      });
  }
}

void debug_check_rbstack (struct rb_node_hdr *root, struct rbstack_ptr rbstack)
{
  struct rbstack_ptr rbstack_top = rbstack;
  struct rb_node_hdr *snode, *tnode = root;
  RBSTACK_INIT (reversed);

  while (1) {
    RBSTACK_UP (rbstack, snode);
    if (!snode) break;
    RBSTACK_PUSH (reversed, snode);
  }

  RBSTACK_POP (reversed, snode);
  if (!snode) return;
  if (snode != tnode) {
    fputs ("rbstack: ", stderr);
    debug_dump_rbstack (rbstack_top, NULL);
    rb_fatal (root, "rbstack root %p != tree root.\n", snode);
  }

  while (1) {
    RBSTACK_POP (reversed, snode);
    if (!snode) break;
    if (!(tnode->flags & RB_THREAD_PREV) && snode == tnode->prev)
      {tnode = snode; continue;}
    if (!(tnode->flags & RB_THREAD_NEXT) && snode == tnode->next)
      {tnode = snode; continue;}
    rb_fatal (root, "rbstack differs from tree: Node %p got no child %p.\n",
	      tnode, snode);
  }
}

#endif /* PIKE_DEBUG || TEST_MULTISET */
