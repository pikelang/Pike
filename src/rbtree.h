/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/* An implementation of a threaded red/black balanced binary tree.
 *
 * Created 2001-04-27 by Martin Stjernholm
 */

#ifndef RBTREE_H
#define RBTREE_H

/* #define RB_STATS */

#include "array.h"

/* A red/black tree is a binary tree with one extra bit of info in
 * each node - the color of it. The following properties holds:
 *
 * o  Every node is either red or black.
 * o  A NULL leaf is considered black.
 * o  If a node is red, its children must be black.
 * o  Every path from a node down to all its leafs have the same
 *    number of black nodes.
 * o  The root node is always black (by convention).
 *
 * The longest possible path in a given tree thus has alternating red
 * and black nodes, and the shortest possible path in it has only
 * black nodes. Therefore it's guaranteed that the longest path is at
 * most twice as long as the shortest one. That ensures O(log n) steps
 * to follow the path down to any node in any tree of size n.
 */

struct rb_node_hdr
{
  struct rb_node_hdr *prev, *next;
  unsigned INT16 flags;		/* Only the top three bits are used;
				 * may be overlaid. */
};

#define RB_RED		0x2000
#define RB_THREAD_PREV	0x4000
#define RB_THREAD_NEXT	0x8000
#define RB_FLAG_MASK	0xe000

/* The thread flags indicate whether the prev/next pointers are thread
 * pointers. A thread pointer is used whenever the pointer would
 * otherwise be NULL, and it points to the next smaller/bigger
 * element. More specifically, the next thread pointer points to the
 * closest parent node whose prev pointer subtree contains it, and
 * vice versa for the prev thread pointer:
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

#define keep_flags(node, code) do {					\
    int kept_flags_ = (node)->flags;					\
    {code;}								\
    (node)->flags =							\
      ((node)->flags & ~RB_FLAG_MASK) | (kept_flags_ & RB_FLAG_MASK);	\
  } while (0)

PMOD_EXPORT struct rb_node_hdr *rb_first (struct rb_node_hdr *root);
PMOD_EXPORT struct rb_node_hdr *rb_last (struct rb_node_hdr *root);
PMOD_EXPORT struct rb_node_hdr *rb_link_prev (struct rb_node_hdr *node);
PMOD_EXPORT struct rb_node_hdr *rb_link_next (struct rb_node_hdr *node);

#define rb_prev(node)							\
  (DO_IF_RB_STATS (rb_num_sidesteps++ COMMA)				\
   (node)->flags & RB_THREAD_PREV ?					\
   DO_IF_RB_STATS (rb_num_sidestep_ops++ COMMA) (node)->prev :		\
   rb_link_prev (node))
#define rb_next(node)							\
  (DO_IF_RB_STATS (rb_num_sidesteps++ COMMA)				\
   (node)->flags & RB_THREAD_NEXT ?					\
   DO_IF_RB_STATS (rb_num_sidestep_ops++ COMMA) (node)->next :		\
   rb_link_next (node))

#ifdef PIKE_DEBUG
/* To get good type checking. */
static INLINE struct rb_node_hdr *rb_node_check (struct rb_node_hdr *node)
  {return node;}
#else
#define rb_node_check(node) ((struct rb_node_hdr *) (node))
#endif

typedef int rb_find_fn (void *key, struct rb_node_hdr *node);
typedef int rb_cmp_fn (struct rb_node_hdr *a, struct rb_node_hdr *b, void *extra);
typedef int rb_equal_fn (struct rb_node_hdr *a, struct rb_node_hdr *b, void *extra);
typedef struct rb_node_hdr *rb_copy_fn (struct rb_node_hdr *node, void *extra);
typedef void rb_free_fn (struct rb_node_hdr *node, void *extra);

/* Operations:
 *
 * insert:
 *     Adds a new entry only if one with the same index doesn't exist
 *     already, replaces it otherwise. If there are several entries
 *     with the same index, the last one of them is replaced. Returns
 *     the added or replaced node.
 *
 * add:
 *     Adds a new entry, even if one with the same index already
 *     exists. The entry is added after all other entries with the
 *     same index. Returns the newly created node.
 *
 * add_after:
 *     Adds a new entry after the given one. Give NULL to add at
 *     front. Returns the newly created node. Note that it's a linear
 *     search to get the right entry among several with the same
 *     index.
 *
 * delete:
 *     Deletes an entry with the specified index, if one exists. If
 *     there are several entries with the specified index, the last
 *     one is deleted. Returns nonzero if a node was deleted, zero
 *     otherwise.
 *
 * delete_node:
 *     Deletes the given node from the tree. Useful to get the right
 *     entry when several have the same index. The node is assumed to
 *     exist in the tree. Note that it's a linear search to get the
 *     right entry among several with the same index.
 *
 * find_eq:
 *     Returns the last entry which has the given index, or zero if
 *     none exists.
 *
 * find_lt, find_gt, find_le, find_ge:
 *     find_lt and find_le returns the biggest entry which satisfy the
 *     condition, and vice versa for the other two. This means that
 *     e.g. rb_next when used on the returned node from find_le never
 *     returns an entry with the same index.
 *
 * get_nth:
 *     Returns the nth entry, counting from the beginning. Note that
 *     this is a linear operation.
 *
 * All destructive operations might change the tree root.
 */

struct rb_node_hdr *rb_insert (struct rb_node_hdr **root,
			       rb_find_fn *find_fn, void *key,
			       struct rb_node_hdr *new);
void rb_add (struct rb_node_hdr **root,
	     rb_find_fn *find_fn, void *key,
	     struct rb_node_hdr *new);
void rb_add_after (struct rb_node_hdr **root,
		   rb_find_fn *find_fn, void *key,
		   struct rb_node_hdr *new,
		   struct rb_node_hdr *existing);
struct rb_node_hdr *rb_remove (struct rb_node_hdr **root,
			       rb_find_fn *find_fn, void *key);
void rb_remove_node (struct rb_node_hdr **root,
		     rb_find_fn *find_fn, void *key,
		     struct rb_node_hdr *node);
struct rb_node_hdr *rb_remove_with_move (struct rb_node_hdr **root,
					 rb_find_fn *find_fn, void *key,
					 size_t node_size,
					 rb_free_fn *cleanup_fn,
					 void *cleanup_fn_extra);
struct rb_node_hdr *rb_remove_node_with_move (struct rb_node_hdr **root,
					      rb_find_fn *find_fn, void *key,
					      struct rb_node_hdr *node,
					      size_t node_size);

struct rb_node_hdr *rb_find_eq (struct rb_node_hdr *root,
				rb_find_fn *find_fn, void *key);
struct rb_node_hdr *rb_find_lt (struct rb_node_hdr *root,
				rb_find_fn *find_fn, void *key);
struct rb_node_hdr *rb_find_gt (struct rb_node_hdr *root,
				rb_find_fn *find_fn, void *key);
struct rb_node_hdr *rb_find_le (struct rb_node_hdr *root,
				rb_find_fn *find_fn, void *key);
struct rb_node_hdr *rb_find_ge (struct rb_node_hdr *root,
				rb_find_fn *find_fn, void *key);
struct rb_node_hdr *rb_get_nth (struct rb_node_hdr *root, size_t n);
size_t rb_sizeof (struct rb_node_hdr *root);

void rb_free (struct rb_node_hdr *root, rb_free_fn *free_node_fn, void *extra);
int rb_equal (struct rb_node_hdr *a, struct rb_node_hdr *b,
	      rb_equal_fn *node_equal_fn, void *extra);
struct rb_node_hdr *rb_copy (struct rb_node_hdr *source,
			     rb_copy_fn *copy_node_fn, void *extra);

struct rb_node_hdr *rb_make_list (struct rb_node_hdr *tree);
struct rb_node_hdr *rb_make_tree (struct rb_node_hdr *list, size_t length);

#define PIKE_MERGE_DESTR_A	0x2000
#define PIKE_MERGE_DESTR_B	0x1000

enum rb_merge_trees {MERGE_TREE_A, MERGE_TREE_B, MERGE_TREE_RES};

typedef struct rb_node_hdr *rb_merge_copy_fn (struct rb_node_hdr *node, void *extra,
					      enum rb_merge_trees tree);
typedef void rb_merge_free_fn (struct rb_node_hdr *node, void *extra,
			       enum rb_merge_trees tree);

struct rb_node_hdr *rb_linear_merge (
  struct rb_node_hdr *a, struct rb_node_hdr *b, int operation,
  rb_cmp_fn *cmp_fn, void *cmp_fn_extra,
  rb_merge_copy_fn *copy_node_fn, void *copy_fn_extra,
  rb_merge_free_fn *free_node_fn, void *free_fn_extra,
  size_t *length);

#ifdef RB_STATS
extern size_t rb_num_sidesteps, rb_num_sidestep_ops;
extern size_t rb_num_finds, rb_find_depth;
extern size_t rb_num_tracks, rb_track_depth;
extern size_t rb_num_sidetracks, rb_num_sidetrack_ops;
extern size_t rb_max_depth;
extern size_t rb_num_traverses, rb_num_traverse_ops;
extern size_t rbstack_slice_allocs;
extern size_t rb_num_adds, rb_add_rebalance_cnt;
extern size_t rb_num_deletes, rb_del_rebalance_cnt;
void reset_rb_stats();
void print_rb_stats (int reset);
#define DO_IF_RB_STATS(X) X
#else
#define DO_IF_RB_STATS(X)
#endif

#endif	/* RBTREE_H */
