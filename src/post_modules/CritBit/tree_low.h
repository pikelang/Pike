/* -*- mode: C; c-basic-offset: 4; -*- */
#ifndef _TREE_LOW_H_
#define _TREE_LOW_H_

#include <sys/types.h>
#include "gc.h"
#include "dmalloc.h"

#define CB_PRINTCHAR(buf, s, n)	do { \
    cb_size j; 			     \
    j.chars = (size_t)(n); j.bits = 0;		\
    for (; j.bits < 32; j.bits++) {		\
	(buf)[j.bits] = (CB_GET_BIT(s, j) ? '1' : '0'); \
    }\
} while (0)

#ifdef CB_SOURCE

#ifndef CB_LE
# define CB_LE(a, b) ((a).chars < (b).chars || ((a).chars == (b).chars \
						&& (a).bits <= (b).bits))
# define CB_LT(a, b) ((a).chars < (b).chars || ((a).chars == (b).chars \
						&& (a).bits < (b).bits))
# define CB_S_EQ(a, b)	((a).chars == (b).chars && (a).bits == (b).bits)
#endif
#define CB_INC(a, c, b) ((a).chars += (c), (a).bits += (b))
#define CB_MIN(a, b)	(CB_LE((a), (b)) ? a : b)
#define CB_MAX(a, b)	(CB_LE((b), (a)) ? a : b)

#define CB_HAS_PARENT(node)	(!!(node)->parent)
#define CB_PARENT(node)	((node)->parent)
#define CB_BIT(node)		((node) && 		\
				 CB_HAS_PARENT(node) ? 	\
				 (CB_CHILD(CB_PARENT(node), 1) == (node)) \
				  : (CB_FATAL((\
			"CB_BIT does not make any sense without parent.\n" \
					     )), -1))
#define CB_HAS_CHILD(node, bit)	(!!(node)->childs[!!(bit)])
#define CB_SET_CHILD(__parent, __bit, __child) do {	\
    cb_node_t _p = (__parent);				\
    cb_node_t _child = (__child);			\
    unsigned INT32 _bit = (__bit);				\
    if (_child) _child->parent = _p;			\
    CB_CHILD(_p, _bit) = _child;			\
} while(0)
#define CB_CHILD(node, bit)	((node)->childs[!!(bit)])
#define CB_SHORT(a, b)	(CB_LE((a)->len, (b)->len) ? (a) : (b))

#define WALK_UP(node, bit, CODE) do { cb_node_t __ = (node);	do {	\
    cb_node_t _ = __;	    		    				\
    /*printf("one step up: %d %d %d\n", CB_HAS_PARENT(_),		\
       CB_HAS_CHILD(_, 0), CB_HAS_CHILD(_, 1));*/			\
    if (CB_HAS_PARENT(_)) {						\
	bit = CB_BIT(_);						\
	_ = CB_PARENT(_);						\
	while (1) { if (1) CODE if (!CB_HAS_PARENT(_)) break;		\
	    bit = CB_BIT(_); _ = CB_PARENT(_); }			\
	__ = _;								\
    }\
} while(0); (node) = __; } while(0)

#define WALK_FORWARD(node, CODE) do { cb_node_t __ = (node); do {	\
    cb_node_t _ = __;		    		    			\
    while (1) {								\
	/*printf("one step forward: %d %d %d\n", CB_HAS_PARENT(_),
	   CB_HAS_CHILD(_, 0), CB_HAS_CHILD(_, 1));*/			\
	if (CB_HAS_CHILD(_, 0)) { _ = CB_CHILD(_, 0); }			\
	else if (CB_HAS_CHILD(_, 1)) { _ = CB_CHILD(_, 1); }		\
	else {								\
	    unsigned INT32 hit = 0, oldbit = 3; 				\
	    WALK_UP(_, oldbit, { 					\
		/*printf("one step up: %d\n", oldbit);			\
	printf("one step forward: %d %d %d\n", CB_HAS_PARENT(_),
	CB_HAS_CHILD(_, 0), CB_HAS_CHILD(_, 1));*/			\
		if (!oldbit && CB_HAS_CHILD(_, 1)) { 			\
		    hit = 1;_ = CB_CHILD(_, 1); break;			\
		}  							\
	    });								\
	    if (oldbit == 3 || !hit) { _ = NULL; break;	}		\
	}								\
	if (1) CODE							\
    }									\
    __ = _;								\
} while(0); (node) = __; } while(0)
#define WALK_BACKWARD(node, CODE) do { cb_node_t __ = (node); do {		\
    cb_node_t _ = __;		    		    				\
    while (1) {									\
	/*printf("one step forward: %d %d %d\n",
	 * CB_HAS_PARENT(_), CB_HAS_CHILD(_, 0), CB_HAS_CHILD(_, 1));*/		\
	if (CB_HAS_PARENT(_)) {							\
	    unsigned INT32 bit = CB_BIT(_);						\
	    _ = CB_PARENT(_);							\
	    if (bit && CB_HAS_CHILD(_, 0)) {					\
		_ = CB_CHILD(_, 0);						\
		while (1) {							\
		    if (CB_HAS_CHILD(_, 1)) { _ = CB_CHILD(_, 1); continue; }	\
		    if (CB_HAS_CHILD(_, 0)) { _ = CB_CHILD(_, 0); continue; }	\
		    break;							\
		}								\
	    }									\
	} else { _ = NULL; break; }						\
	if (1) CODE								\
    }										\
    __ = _;									\
} while(0); (node) = __; } while(0)

#endif /* CB_SOURCE */

#ifdef cb_node
# undef cb_node
#endif
#define cb_node		CB_NAME(node)

#ifdef cb_key
# undef cb_key
#endif
#define cb_key		CB_NAME(key)

#ifdef cb_node_t
# undef cb_node_t
#endif
#define cb_node_t	CB_TYPE(node)

#ifdef cb_tree_t
# undef cb_tree_t
#endif
#define cb_tree_t	CB_TYPE(tree)

#ifdef cb_find_ne
# undef cb_find_ne
#endif
#define cb_find_ne	CB_NAME(find_ne)

#ifdef cb_get_nth
# undef cb_get_nth
#endif
#define cb_get_nth	CB_NAME(get_nth)

#ifdef cb_find_previous
# undef cb_find_previous
#endif
#define cb_find_previous	CB_NAME(find_previous)

#ifdef cb_find_le
# undef cb_find_le
#endif
#define cb_find_le	CB_NAME(find_le)

#ifdef cb_subtree_prefix
# undef cb_subtree_prefix
#endif
#define cb_subtree_prefix	CB_NAME(subtree_prefix)

#ifdef cb_get_depth
# undef cb_get_depth
#endif
#define cb_get_depth	CB_NAME(get_depth)

#ifdef cb_find_last
# undef cb_find_last
#endif
#define cb_find_last	CB_NAME(find_last)

#ifdef cb_find_first
# undef cb_find_first
#endif
#define cb_find_first	CB_NAME(find_first)

#ifdef cb_copy_tree
# undef cb_copy_tree
#endif
#define cb_copy_tree	CB_NAME(copy_tree)

#ifdef cb_get_range
# undef cb_get_range
#endif
#define cb_get_range	CB_NAME(get_range)

#ifdef cb_index
# undef cb_index
#endif
#define cb_index	CB_NAME(index)

#ifdef cb_find_next
# undef cb_find_next
#endif
#define cb_find_next	CB_NAME(find_next)

#ifdef cb_insert
# undef cb_insert
#endif
#define cb_insert	CB_NAME(insert)

#endif /* TREE_LOW_H */

typedef struct cb_key {
    cb_string str;
    cb_size len;
} cb_key;

typedef struct cb_node {
    cb_key key;
    cb_value value;
    size_t size; /* size of the subtree */
    struct cb_node * parent;
    struct cb_node * childs[2];
} * cb_node_t;

typedef struct cb_node cb_node;

typedef struct cb_tree {
    cb_node_t root;
} cb_tree;

CB_STATIC inline void cb_insert(cb_tree*, const cb_key,
				   const cb_value *);
CB_STATIC inline cb_node_t cb_find_next(const cb_node_t, const cb_key);
CB_STATIC inline cb_node_t cb_index(const cb_node_t tree, const cb_key);
CB_STATIC inline void cb_get_range(const struct cb_tree *, struct cb_tree *,
				      const cb_key, const cb_key);
CB_STATIC inline void cb_copy_tree(struct cb_tree * dst,
				      cb_node_t from);
CB_STATIC inline cb_node_t cb_find_first(cb_node_t tree);
CB_STATIC inline cb_node_t cb_find_last(cb_node_t tree);
CB_STATIC inline size_t cb_get_depth(cb_node_t node);
CB_STATIC inline cb_node_t cb_subtree_prefix(cb_node_t node, cb_key key);
CB_STATIC inline cb_node_t cb_find_le(const cb_node_t tree,
					 const cb_key key);
CB_STATIC inline cb_node_t cb_find_previous(const cb_node_t tree,
					       const cb_key key);
CB_STATIC inline cb_node_t cb_get_nth(const cb_node_t tree, size_t n);
CB_STATIC inline cb_node_t cb_find_ne(const cb_node_t tree,
					 const cb_key key);
CB_STATIC inline void cb_delete(struct cb_tree*, const cb_key, cb_value*);
