/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: rbtree_low.h,v 1.6 2004/05/28 09:42:44 mast Exp $
*/

/* The lower level api for using rbtree. This is in a separate file
 * since it's quite macro heavy.
 *
 * Created 2001-04-27 by Martin Stjernholm
 */

#ifndef RBTREE_LOW_H
#define RBTREE_LOW_H

#include "rbtree.h"

/* A sliced stack is used to track the way down in a tree, so we can
 * back up again easily while rebalancing it. The first slice is
 * allocated on the C stack. */

#define STACK_SLICE_SIZE 20
/* This is in the worst possible case enough for trees of size
 * 2^(20/2) = 1024 before allocating another slice, but in the typical
 * case it's enough for trees with up to about 2^(20-1) = 524288
 * elements. */

struct rbstack_slice
{
#ifdef RB_STATS
  size_t depth, maxdepth;
#endif
  struct rbstack_slice *up;
  struct rb_node_hdr *stack[STACK_SLICE_SIZE];
};

struct rbstack_ptr
{
  struct rbstack_slice *slice;
  size_t ssp;			/* Only zero when the stack is empty. */
};

void rbstack_push (struct rbstack_ptr *rbstack, struct rb_node_hdr *node);
void rbstack_pop (struct rbstack_ptr *rbstack);
void rbstack_up (struct rbstack_ptr *rbstack);
void rbstack_up_to_root (struct rbstack_ptr *rbstack);
void rbstack_free (struct rbstack_ptr *rbstack);
void rbstack_insert (struct rbstack_ptr *top, struct rbstack_ptr *pos,
		     struct rb_node_hdr *node);
void rbstack_assign (struct rbstack_ptr *target, struct rbstack_ptr *source);
void rbstack_copy (struct rbstack_ptr *target, struct rbstack_ptr *source);
void rbstack_shift (struct rbstack_ptr rbstack,
		    struct rb_node_hdr *oldbase,
		    struct rb_node_hdr *newbase);

#define RBSTACK_INIT(rbstack)						\
  struct rbstack_slice PIKE_CONCAT3 (_, rbstack, _top_) = {		\
    DO_IF_RB_STATS (0 COMMA 0 COMMA)					\
    NULL,								\
    {NULL,}								\
  };									\
  struct rbstack_ptr rbstack = {					\
    (PIKE_CONCAT3 (_, rbstack, _top_).up = NULL,			\
     &PIKE_CONCAT3 (_, rbstack, _top_)),				\
    0									\
  }

#define RBSTACK_PUSH(rbstack, node) do {				\
    if ((rbstack).ssp < STACK_SLICE_SIZE) {				\
      (rbstack).slice->stack[(rbstack).ssp++] = (node);			\
    }									\
    else rbstack_push (&(rbstack), node);				\
    DO_IF_RB_STATS (							\
      if (++(rbstack).slice->depth > (rbstack).slice->maxdepth)		\
	(rbstack).slice->maxdepth = (rbstack).slice->depth;		\
    );									\
  } while (0)

#define RBSTACK_POP(rbstack, node) do {					\
    if ((rbstack).ssp) {						\
      (node) = (rbstack).slice->stack[--(rbstack).ssp];			\
      DO_IF_RB_STATS ((rbstack).slice->depth--);			\
      if (!(rbstack).ssp && (rbstack).slice->up)			\
	rbstack_pop (&(rbstack));					\
    }									\
    else (node) = NULL;							\
  } while (0)

#define RBSTACK_POP_IGNORE(rbstack) do {				\
    if ((rbstack).ssp && !--(rbstack).ssp) {				\
      DO_IF_RB_STATS ((rbstack).slice->depth--);			\
      if ((rbstack).slice->up)						\
	rbstack_pop (&(rbstack));					\
    }									\
  } while (0)

#define RBSTACK_UP(rbstack, node) do {					\
    if ((rbstack).ssp) {						\
      (node) = (rbstack).slice->stack[--(rbstack).ssp];			\
      if (!(rbstack).ssp && (rbstack).slice->up)			\
	rbstack_up (&(rbstack));					\
    }									\
    else (node) = NULL;							\
  } while (0)

#define RBSTACK_UP_IGNORE(rbstack) do {					\
    if ((rbstack).ssp && !--(rbstack).ssp && (rbstack).slice->up)	\
      rbstack_up (&(rbstack));						\
  } while (0)

#define RBSTACK_PEEK(rbstack)						\
  ((rbstack).ssp ? (rbstack).slice->stack[(rbstack).ssp - 1] : NULL)

#define RBSTACK_POKE(rbstack, node) do {				\
    DO_IF_DEBUG (if (!(rbstack).ssp) Pike_fatal ("Using free stack pointer.\n")); \
    (rbstack).slice->stack[(rbstack).ssp - 1] = (node);			\
  } while (0)

#define RBSTACK_UP_TO_ROOT(rbstack, node) do {				\
    if ((rbstack).ssp) {						\
      rbstack_up_to_root (&(rbstack));					\
      (node) = (rbstack).slice->stack[0];				\
    }									\
  } while (0)

#define RBSTACK_FREE(rbstack) do {					\
    if ((rbstack).ssp) {						\
      if ((rbstack).slice->up) rbstack_free (&(rbstack));		\
      (rbstack).ssp = 0;						\
    }									\
    DO_IF_RB_STATS (							\
      rb_num_tracks++;							\
      rb_track_depth += (rbstack).slice->maxdepth;			\
      if ((rbstack).slice->maxdepth > rb_max_depth)			\
	rb_max_depth = (rbstack).slice->maxdepth;			\
      (rbstack).slice->depth = (rbstack).slice->maxdepth = 0;		\
    );									\
  } while (0)

#define RBSTACK_FREE_SET_ROOT(rbstack, node) do {			\
    if ((rbstack).ssp) {						\
      if ((rbstack).slice->up) rbstack_free (&(rbstack));		\
      (rbstack).ssp = 0;						\
      (node) = (rbstack).slice->stack[0];				\
    }									\
    DO_IF_RB_STATS (							\
      rb_num_tracks++;							\
      rb_track_depth += (rbstack).slice->maxdepth;			\
      if ((rbstack).slice->maxdepth > rb_max_depth)			\
	rb_max_depth = (rbstack).slice->maxdepth;			\
      (rbstack).slice->depth = (rbstack).slice->maxdepth = 0;		\
    );									\
  } while (0)

void low_rb_init_root (struct rb_node_hdr *new_root);
void low_rb_link_at_prev (struct rb_node_hdr **root, struct rbstack_ptr rbstack,
			  struct rb_node_hdr *new);
void low_rb_link_at_next (struct rb_node_hdr **root, struct rbstack_ptr rbstack,
			  struct rb_node_hdr *new);
struct rb_node_hdr *low_rb_unlink_with_move (struct rb_node_hdr **root,
					     struct rbstack_ptr *rbstack_ptr,
					     int keep_rbstack,
					     size_t node_size);
void low_rb_unlink_without_move (struct rb_node_hdr **root,
				 struct rbstack_ptr *rbstack_ptr,
				 int keep_rbstack);
void low_rb_build_stack (struct rb_node_hdr *root, struct rb_node_hdr *node,
			 struct rbstack_ptr *rbstack);

#if defined (PIKE_DEBUG) || defined (TEST_MULTISET)

typedef void dump_data_fn (struct rb_node_hdr *node, void *extra);
void debug_dump_rb_tree (struct rb_node_hdr *root, dump_data_fn *dump_data, void *extra);
void debug_dump_rbstack (struct rbstack_ptr rbstack, struct rbstack_ptr *pos);
void debug_check_rb_tree (struct rb_node_hdr *root, dump_data_fn *dump_data, void *extra);
void debug_check_rbstack (struct rb_node_hdr *root, struct rbstack_ptr rbstack);

#endif

/* Traverses the tree in depth-first order:
 * push		Run when entering the node. Preceded by an enter_* label.
 * p_leaf	Run when the prev pointer of the node isn't a subtree.
 * p_sub	Run when the prev pointer of the node is a subtree.
 * between	Run after the prev subtree has been recursed and before
 *		the next subtree is examined. Preceded by a between_*
 *		label.
 * n_leaf	Run when the next pointer of the node isn't a subtree.
 * n_sub	Run when the next pointer of the node is a subtree.
 * pop		Run when leaving the node. Preceded by a leave_* label.
 */
#define LOW_RB_TRAVERSE(label, rbstack, node, push, p_leaf, p_sub,	\
			between, n_leaf, n_sub, pop)			\
  do {									\
    struct rb_node_hdr *rb_last_;					\
    DO_IF_RB_STATS (rb_num_traverses++);				\
    if (node) {								\
      PIKE_CONCAT (enter_, label):					\
      DO_IF_RB_STATS (rb_num_traverse_ops++);				\
      {push;}								\
      if ((node)->flags & RB_THREAD_PREV)				\
	{p_leaf;}							\
      else {								\
	{p_sub;}							\
	RBSTACK_PUSH (rbstack, node);					\
	(node) = (node)->prev;						\
	goto PIKE_CONCAT (enter_, label);				\
      }									\
      PIKE_CONCAT (between_, label):					\
      {between;}							\
      if ((node)->flags & RB_THREAD_NEXT)				\
	{n_leaf;}							\
      else {								\
	{n_sub;}							\
	RBSTACK_PUSH (rbstack, node);					\
	(node) = (node)->next;						\
	goto PIKE_CONCAT (enter_, label);				\
      }									\
      while (1) {							\
	PIKE_CONCAT (leave_, label):					\
	DO_IF_RB_STATS (rb_num_traverse_ops++);				\
	{pop;}								\
	rb_last_ = (node);						\
	RBSTACK_POP (rbstack, node);					\
	if (!(node)) break;						\
	if (rb_last_ == (node)->prev)					\
	  goto PIKE_CONCAT (between_, label);				\
      }									\
    }									\
  } while (0)

#define LOW_RB_DEBUG_TRAVERSE(label, rbstack, node, push, p_leaf, p_sub, \
			      between, n_leaf, n_sub, pop)		\
  do {									\
    size_t PIKE_CONCAT (depth_, label) = 0;				\
    LOW_RB_TRAVERSE(							\
      label, rbstack, node,						\
      fprintf (stderr, "%*s%p enter\n",					\
	       PIKE_CONCAT (depth_, label)++, "", node); {push;},	\
      fprintf (stderr, "%*s%p prev leaf\n",				\
	       PIKE_CONCAT (depth_, label), "", node); {p_leaf;},	\
      fprintf (stderr, "%*s%p prev subtree\n",				\
	       PIKE_CONCAT (depth_, label), "", node); {p_sub;},	\
      fprintf (stderr, "%*s%p between\n",				\
	       PIKE_CONCAT (depth_, label) - 1, "", node); {between;},	\
      fprintf (stderr, "%*s%p next leaf\n",				\
	       PIKE_CONCAT (depth_, label), "", node); {n_leaf;},	\
      fprintf (stderr, "%*s%p next subtree\n",				\
	       PIKE_CONCAT (depth_, label), "", node); {n_sub;},	\
      fprintf (stderr, "%*s%p leave\n",					\
	       --PIKE_CONCAT (depth_, label), "", node); {pop;});	\
  } while (0)

/* The `cmp' code should set the variable cmp_res to the result of the
 * comparison between the key and the current node `node'. */
#define LOW_RB_FIND(node, cmp, got_lt, got_eq, got_gt)			\
  do {									\
    int cmp_res, found_eq_ = 0;						\
    DO_IF_RB_STATS (							\
      size_t stat_depth_count_ = 0;					\
      rb_num_finds++;							\
    );									\
    while (1) {								\
      DO_IF_DEBUG (if (!node) Pike_fatal ("Recursing into null node.\n"));	\
      DO_IF_RB_STATS (							\
	if (++stat_depth_count_ > rb_max_depth)				\
	  rb_max_depth = stat_depth_count_;				\
	rb_find_depth++;						\
      );								\
      {cmp;}								\
      if (cmp_res < 0)							\
	if ((node)->flags & RB_THREAD_PREV)				\
	  if (found_eq_)						\
	    (node) = (node)->prev;					\
	  else {							\
	    {got_gt;}							\
	    break;							\
	  }								\
	else {								\
	  (node) = (node)->prev;					\
	  continue;							\
	}								\
      else								\
	if ((node)->flags & RB_THREAD_NEXT)				\
	  if (!cmp_res)							\
	    {}								\
	  else {							\
	    {got_lt;}							\
	    break;							\
	  }								\
	else {								\
	  if (!cmp_res) found_eq_ = 1;					\
	  (node) = (node)->next;					\
	  continue;							\
	}								\
      {got_eq;}								\
      break;								\
    }									\
  } while (0)

/* Variant of LOW_RB_FIND that assumes that `cmp' never returns 0. */
#define LOW_RB_FIND_NEQ(node, cmp, got_lt, got_gt)			\
  do {									\
    int cmp_res;							\
    DO_IF_RB_STATS (							\
      size_t stat_depth_count_ = 0;					\
      rb_num_finds++;							\
    );									\
    while (1) {								\
      DO_IF_DEBUG (if (!node) Pike_fatal ("Recursing into null node.\n"));	\
      DO_IF_RB_STATS (							\
	if (++stat_depth_count_ > rb_max_depth)				\
	  rb_max_depth = stat_depth_count_;				\
	rb_find_depth++;						\
      );								\
      {cmp;}								\
      if (cmp_res < 0) {						\
	if ((node)->flags & RB_THREAD_PREV) {				\
	  {got_gt;}							\
	  break;							\
	}								\
	(node) = (node)->prev;						\
      }									\
      else {								\
	DO_IF_DEBUG (if (!cmp_res) Pike_fatal ("cmp_res 0 not expected.\n")); \
	if ((node)->flags & RB_THREAD_NEXT) {				\
	  {got_lt;}							\
	  break;							\
	}								\
	(node) = (node)->next;						\
      }									\
    }									\
  } while (0)

/* Tracks the way down a tree to a specific node and updates the stack
 * as necessary for low_rb_link_* and low_rb_unlink_*. */
#define LOW_RB_TRACK(rbstack, node, cmp, got_lt, got_eq, got_gt)	\
  do {									\
    DO_IF_DEBUG (							\
      if (RBSTACK_PEEK (rbstack)) Pike_fatal ("The stack is not empty.\n");	\
    );									\
    DO_IF_RB_STATS (rb_num_finds--);					\
    LOW_RB_FIND (							\
      node,								\
      {									\
	DO_IF_RB_STATS (rb_find_depth--);				\
	RBSTACK_PUSH (rbstack, node);					\
	{cmp;}								\
      },								\
      got_lt,								\
      {									\
	while ((node) != RBSTACK_PEEK (rbstack))			\
	  RBSTACK_POP_IGNORE (rbstack);					\
	{got_eq;}							\
      }, got_gt);							\
  } while (0)

#define LOW_RB_TRACK_NEQ(rbstack, node, cmp, got_lt, got_gt)		\
  do {									\
    DO_IF_DEBUG (							\
      if (RBSTACK_PEEK (rbstack)) Pike_fatal ("The stack is not empty.\n");	\
    );									\
    DO_IF_RB_STATS (rb_num_finds--);					\
    LOW_RB_FIND_NEQ (							\
      node,								\
      {									\
	DO_IF_RB_STATS (rb_find_depth--);				\
	RBSTACK_PUSH (rbstack, node);					\
	{cmp;}								\
      },								\
      got_lt, got_gt);							\
  } while (0)

/* Goes to the first node in a tree while keeping the stack updated. */
#define LOW_RB_TRACK_FIRST(rbstack, node)				\
  do {									\
    DO_IF_DEBUG (							\
      if (RBSTACK_PEEK (rbstack)) Pike_fatal ("The stack is not empty.\n");	\
    );									\
    DO_IF_RB_STATS (rb_num_sidetracks++);				\
    if (node) {								\
      struct rb_node_hdr *rb_prev_ = node->prev;			\
      RBSTACK_PUSH (rbstack, node);					\
      DO_IF_RB_STATS (rb_num_sidetrack_ops++);				\
      while (rb_prev_) {						\
	RBSTACK_PUSH (rbstack, node = rb_prev_);			\
	DO_IF_RB_STATS (rb_num_sidetrack_ops++);			\
	rb_prev_ = node->prev;						\
      }									\
    }									\
  } while (0)

/* Goes to the next node in order while keeping the stack updated. */
#define LOW_RB_TRACK_NEXT(rbstack, node)				\
  do {									\
    DO_IF_DEBUG (							\
      if (node != RBSTACK_PEEK (rbstack))				\
	Pike_fatal ("Given node is not on top of stack.\n");			\
    );									\
    DO_IF_RB_STATS (rb_num_sidetracks++);				\
    if (node->flags & RB_THREAD_NEXT) {					\
      struct rb_node_hdr *rb_next_ = node->next;			\
      while ((node = RBSTACK_PEEK (rbstack)) != rb_next_) {		\
	RBSTACK_POP_IGNORE (rbstack);					\
	DO_IF_RB_STATS (rb_num_sidetrack_ops++);			\
      }									\
    }									\
    else {								\
      node = node->next;						\
      while (1) {							\
	RBSTACK_PUSH (rbstack, node);					\
	DO_IF_RB_STATS (rb_num_sidetrack_ops++);			\
	if (node->flags & RB_THREAD_PREV) break;			\
	node = node->prev;						\
      }									\
    }									\
  } while (0)

/* Goes to the previous node in order while keeping the stack updated. */
#define LOW_RB_TRACK_PREV(rbstack, node)				\
  do {									\
    DO_IF_DEBUG (							\
      if (node != RBSTACK_PEEK (rbstack))				\
	Pike_fatal ("Given node is not on top of stack.\n");			\
    );									\
    DO_IF_RB_STATS (rb_num_sidetracks++);				\
    if (node->flags & RB_THREAD_PREV) {					\
      struct rb_node_hdr *rb_prev_ = node->prev;			\
      while ((node = RBSTACK_PEEK (rbstack)) != rb_prev_) {		\
	RBSTACK_POP_IGNORE (rbstack);					\
	DO_IF_RB_STATS (rb_num_sidetrack_ops++);			\
      }									\
    }									\
    else {								\
      node = node->prev;						\
      while (1) {							\
	RBSTACK_PUSH (rbstack, node);					\
	DO_IF_RB_STATS (rb_num_sidetrack_ops++);			\
	if (node->flags & RB_THREAD_NEXT) break;			\
	node = node->next;						\
      }									\
    }									\
  } while (0)

/* An alternative to rb_insert, which might or might not insert the
 * newly created node. This one compares nodes like LOW_RB_FIND and
 * will only run the code `insert' when a new node actually is to be
 * inserted, otherwise it runs the code `replace' on the matching
 * existing node. */
#define LOW_RB_INSERT(tree, node, cmp, insert, replace)			\
  do {									\
    int rb_ins_type_;							\
    RBSTACK_INIT (rbstack);						\
    if (((node) = *(tree))) {						\
      LOW_RB_TRACK (							\
	rbstack, node, cmp,						\
	{								\
	  rb_ins_type_ = 1;	/* Got less - link at next. */		\
	}, {								\
	  rb_ins_type_ = 0;	/* Got equal - replace. */		\
	  {replace;}							\
	  RBSTACK_FREE (rbstack);					\
	}, {								\
	  rb_ins_type_ = 2;	/* Got greater - link at prev. */	\
	});								\
    }									\
    else rb_ins_type_ = 3;						\
    if (rb_ins_type_) {							\
      DO_IF_DEBUG ((node) = 0);						\
      {insert;}								\
      switch (rb_ins_type_) {						\
	case 1: low_rb_link_at_next ((tree), rbstack, (node)); break;	\
	case 2: low_rb_link_at_prev ((tree), rbstack, (node)); break;	\
	case 3: low_rb_init_root (*(tree) = (node)); break;		\
      }									\
    }									\
  } while (0)

/* Merges the two trees a and b in linear time (no more, no less). The
 * operation argument describes the way to merge, like the one given
 * to merge() in array.c. cmp does the comparison between a and b to
 * cmp_res, copy_a and copy_b copy the nodes a and b resp, to
 * new_node. free_a and free_b free the nodes a and b resp. prep_a and
 * prep_b is run for every visited node in a and b resp, before any of
 * the other code blocks.
 *
 * The result in res is a list linked by the next pointers, and length
 * is set to the length of it. These are suitable for rb_make_tree.
 *
 * NB: It doesn't handle making duplicates of the same node, i.e.
 * PIKE_ARRAY_OP_A without PIKE_ARRAY_OP_SKIP_A. Not a problem since
 * none of the currently defined operations use that. */
#define LOW_RB_MERGE(label, a, b, res, length, operation,		\
		     prep_a, prep_b, cmp, copy_a, free_a, copy_b, free_b) \
  do {									\
    struct rb_node_hdr *new_node;					\
    int cmp_res, op_ = 0; /* Init only to avoid warnings. */		\
    /* Traverse backwards so that the merge "gravitates" towards the */	\
    /* end when duplicate entries are processed, e.g. */		\
    /* (<1:1, 1:2>) | (<1:3>) produces (<1:1, 1:3>) and not */		\
    /* (<1:3, 1:2>). */							\
									\
    a = rb_last (a);							\
    b = rb_last (b);							\
    res = 0;								\
									\
    while (1) {								\
      /* A bit quirky code to avoid expanding the code blocks more */	\
      /* than once. */							\
      if (a) {prep_a;}							\
      if (b) {								\
	{prep_b;}							\
	if (a) {							\
	  {cmp;}							\
	  /* Result reversed due to backward direction. */		\
	  if (cmp_res > 0)						\
	    op_ = operation >> 8;					\
	  else if (cmp_res < 0)						\
	    op_ = operation;						\
	  else								\
	    op_ = operation >> 4;					\
	}								\
	else if (operation & PIKE_ARRAY_OP_B)				\
	  goto PIKE_CONCAT (label, _copy_b);				\
	else								\
	  goto PIKE_CONCAT (label, _free_b);				\
      }									\
      else								\
	if (a)								\
	  if (operation & (PIKE_ARRAY_OP_A << 8))			\
	    goto PIKE_CONCAT (label, _copy_a);				\
	  else								\
	    goto PIKE_CONCAT (label, _free_a);				\
	else								\
	  break;							\
									\
      if (op_ & PIKE_ARRAY_OP_B) {					\
	PIKE_CONCAT (label, _copy_b):;					\
	{copy_b;}							\
	new_node->next = res, res = new_node;				\
	length++;							\
	b = rb_prev (b);						\
      }									\
      else if (op_ & PIKE_ARRAY_OP_SKIP_B) {				\
	PIKE_CONCAT (label, _free_b):;					\
	new_node = rb_prev (b);						\
	{free_b;}							\
	b = new_node;							\
      }									\
									\
      if (a) {								\
	if (op_ & PIKE_ARRAY_OP_A) {					\
	  if (!(op_ & PIKE_ARRAY_OP_B)) {				\
	    PIKE_CONCAT (label, _copy_a):;				\
	    {copy_a;}							\
	    new_node->next = res, res = new_node;			\
	    length++;							\
	    a = rb_prev (a);						\
	  }								\
	}								\
	else if (op_ & PIKE_ARRAY_OP_SKIP_A) {				\
	  PIKE_CONCAT (label, _free_a):;				\
	  new_node = rb_prev (a);					\
	  {free_a;}							\
	  a = new_node;							\
	}								\
      }									\
    }									\
  } while (0)

#endif	/* RBTREE_H */
