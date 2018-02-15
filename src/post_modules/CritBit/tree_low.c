#include <stdlib.h>
#include <sys/types.h>

#include "pike_int_types.h"

#ifndef HAS___BUILTIN_EXPECT
# define __builtin_expect(x, expected_value) (x)
#endif

#define likely(x)	__builtin_expect((x), 1)
#define unlikely(x)	__builtin_expect((x), 0)

#ifndef cb_check_node
# define cb_check_node(node)	do {} while(0)
#endif

#ifndef MAX
#define MAX(a, b)	((a) > (b) ? (a) : (b))
#endif

#ifndef CB_FATAL
# define CB_FATAL(x)	(printf x, exit(4))
#endif

#ifndef CB_ERROR
# define CB_ERROR	CB_FATAL
#endif

#ifndef cb_prefix_count
# define cb_prefix_count		cb_prefix_count_fallback
#endif

#ifndef CB_SOURCE
# define CB_SOURCE
#endif

static inline void cb_zap_node(const struct cb_tree*, cb_node_t);
static inline cb_node_t cb_node_from_string(const struct cb_tree*,
					    const cb_key, const cb_value *);


static inline cb_key CB_KEY_FROM_STRING(const cb_string string) {
    cb_key key;
    /* printf("key from string: %p (%d, %d)\n",
      string, key.len.chars, key.len.bits); */
    key.str = string;
    key.len.chars = CB_LENGTH(string);
    key.len.bits = 0;
    return key;
}

static inline cb_size cb_prefix_count_fallback(const cb_string s1,
					       const cb_string s2,
					       const cb_size len,
					       cb_size start) {
    ptrdiff_t i;
    unsigned INT32 width = MAX(CB_WIDTH(s1), CB_WIDTH(s2));

    for (i = start.chars; i < len.chars; i++) {
	unsigned INT32 diffbit = CB_COUNT_PREFIX(s1, s2, i);
	start.bits = 0;

	if (diffbit < width) { /*  are different */
#ifdef ANNOY_DEBUG
	    fprintf(stderr, "diff in bit %d (byte %d) %d\n", diffbit, i, __LINE__);
#endif
	    start.chars = i;
	    start.bits = diffbit;
	    return start;
	}
    }

    if (len.bits > start.bits) {
	unsigned INT32 diffbit = CB_COUNT_PREFIX(s1, s2, len.chars);
	if (diffbit < len.bits) { /*  are different */
#ifdef ANNOY_DEBUG
	    fprintf(stderr, "diff in bit %d (byte %d) %d\n", diffbit, len.chars, __LINE__);
#endif
	    start.chars = len.chars;
	    start.bits = diffbit;
	    return start;
	} else return len;
    }

    return len;
}


static inline cb_node_t node_init() {
    cb_node_t tree;

    tree = CB_NODE_ALLOC();
    memset(tree, 0, sizeof(cb_node));
    CB_INIT_VALUE(tree);

    return tree;
}

CB_STATIC CB_INLINE void cb_get_range(const struct cb_tree * src,
				      struct cb_tree * dst,
				      const cb_key a, const cb_key b) {
    const cb_node_t tree = src->root;
    cb_node_t node = cb_index(tree, a);
    cb_node_t end = cb_index(tree, b);
    /* printf("start: %p, stop: %p in line %d\n", node, end, __LINE__); */
    if (!node) node = cb_find_next(tree, a);
    /* printf("start: %p, stop: %p in line %d\n", node, end, __LINE__); */
    if (node) {

	if ((end && !CB_HAS_VALUE(end)) || (end = cb_find_next(tree, b))) {
    /* printf("start: %p, stop: %p in line %d\n", node, end, __LINE__); */
	    if (end == node) return;

	    WALK_BACKWARD(end, {
		if (CB_HAS_VALUE(_)) {
		    if (_ == node) {
			cb_insert(dst, node->key, &node->value);
		    }
		    break;
		}
		if (_ == node) return;
	    });
    /* printf("start: %p, stop: %p in line %d\n", node, end, __LINE__); */
	}
	if (node && !CB_HAS_VALUE(node)) {
	    if (end == node) return;
	    WALK_FORWARD(node, {
		if (_ == end) return;
		if (CB_HAS_VALUE(_)) break;
	    });
	}
    /* printf("start: %p, stop: %p in line %d\n", node, end, __LINE__); */
	cb_insert(dst, node->key, &node->value);

	if (node != end) WALK_FORWARD(node, {
	    if (CB_HAS_VALUE(_)) {
		/* printf("adding %p\n", _); */
		cb_insert(dst, _->key, &_->value);
		if (_ == end) break;
	    }
	});

	/* printf("new range has %d members.\n", dst->root->size); */
    }
}

static inline cb_node_t cb_node_from_string(const struct cb_tree * tree, 
					    const cb_key s,
					    const cb_value * val) {
    cb_node_t node = node_init();
    CB_SET_KEY(node, s);
    node->size = 1;
    CB_SET_VALUE(node, val);

#ifdef DEBUG_CHECKS
    if (!CB_HAS_VALUE(node))
	printf("culprit here. %d\n", __LINE__);
#endif

    return node;
}

static inline cb_node_t cb_clone_node(const struct cb_tree * tree,
				      const cb_node_t node) {
    cb_node_t nnode = CB_NODE_ALLOC();

    memcpy(nnode, node, sizeof(cb_node));
    CB_ADD_KEY_REF(node->key);
    CB_INIT_VALUE(node);
    CB_SET_CHILD(nnode, 0, CB_CHILD(nnode, 0));
    CB_SET_CHILD(nnode, 1, CB_CHILD(nnode, 1));
    CB_SET_CHILD(node, 0, NULL);
    CB_SET_CHILD(node, 1, NULL);

    return nnode;
}

CB_STATIC CB_INLINE void cb_copy_tree(struct cb_tree * dst,
				      cb_node_t from) {

    cb_node_t parent;
    cb_node_t node = from;

    if (!from) return;

    parent = from->parent;
    from->parent = NULL;

    if (CB_HAS_VALUE(from))
	cb_insert(dst, from->key, &from->value);

    WALK_FORWARD(node, {
	if (CB_HAS_VALUE(_))
	    cb_insert(dst, _->key, &_->value);
    });

    from->parent = parent;
}

static void cb_free_node(const struct cb_tree * tree, cb_node_t node) {
    if (!node) {
	CB_FATAL(("double free!\n"));
    }
    if (CB_HAS_CHILD(node, 0)) {
	cb_free_node(tree, CB_CHILD(node, 0));
	CB_SET_CHILD(node, 0, NULL);
    }
    if (CB_HAS_CHILD(node, 1)) {
	cb_free_node(tree, CB_CHILD(node, 1));
	CB_SET_CHILD(node, 1, NULL);
    }
    cb_zap_node(tree, node);
}

static inline void cb_zap_node(const struct cb_tree * tree, cb_node_t node) {
    CB_FREE_KEY(node->key);
    CB_RM_VALUE(node);
    CB_NODE_FREE(node);
}

CB_STATIC CB_INLINE cb_node_t cb_find_first(cb_node_t tree) {
    while (tree && !CB_HAS_VALUE(tree)) { tree = CB_CHILD(tree, 0); };

    return tree;
}

CB_STATIC CB_INLINE cb_node_t cb_find_last(cb_node_t tree) {
    while (1) {
	if (CB_HAS_CHILD(tree, 1)) tree = CB_CHILD(tree, 1);
	else if (CB_HAS_CHILD(tree, 0)) tree = CB_CHILD(tree, 0);
	else break;
    }
    return tree;
}

CB_STATIC CB_INLINE size_t cb_get_depth(cb_node_t node) {
    size_t a = 0, b = 0, len = 1;

    if (CB_HAS_CHILD(node, 0)) {
	a = cb_get_depth(CB_CHILD(node, 0));
    }

    if (CB_HAS_CHILD(node, 1)) {
	b = cb_get_depth(CB_CHILD(node, 1));
    }

    return len + MAX(b, a);
}

CB_STATIC CB_INLINE cb_node_t cb_subtree_prefix(cb_node_t node, cb_key key) {
    cb_size start = {0,0};

    do {
	unsigned INT32 bit;
	start = cb_prefix_count(node->key.str, key.str,
				CB_MIN(node->key.len, key.len), start);

	if (CB_S_EQ(start, key.len)) { /*  key is substring */
	    return node;
	}

	bit = CB_GET_BIT(key.str, start);
	node = CB_CHILD(node, bit);
    } while (node);

    return NULL;
}

CB_STATIC CB_INLINE void cb_delete(struct cb_tree * tree,
				   const cb_key key, cb_value * val) {
    cb_node_t node = cb_index(tree->root, key);

    if (node && CB_HAS_VALUE(node)) {
	unsigned INT32 bit;
	cb_node_t t;
	if (val) CB_GET_VALUE(node, val);

	CB_RM_VALUE(node);
	node->size--;

	if (node == tree->root) goto PARENT;

	if (!CB_HAS_PARENT(node)) CB_ERROR(("broken tree\n"));

	t = node;
	WALK_UP(t, bit, {
	    _->size--;
	});

	cb_check_node(tree->root);

	do {
	    switch (CB_HAS_CHILD(node, 0) + CB_HAS_CHILD(node, 1)) {
	    case 2: return;
	    case 1:
		CB_SET_CHILD(CB_PARENT(node), CB_BIT(node),
			     CB_CHILD(node, CB_HAS_CHILD(node, 1)));
		break;
	    case 0:
		CB_SET_CHILD(CB_PARENT(node), CB_BIT(node), NULL);
		break;
	    }
	    t = CB_PARENT(node);
	    cb_zap_node(tree, node);
	    /*  do some deletion */
	    node = t;
	} while (CB_HAS_PARENT(node) && !CB_HAS_VALUE(node));

PARENT:
	cb_check_node(tree->root);
	if (node == tree->root && !CB_HAS_VALUE(node)) {
	    switch (CB_HAS_CHILD(node, 0) + CB_HAS_CHILD(node, 1)) {
	    case 2: return;
	    case 1:
		t = CB_CHILD(node, CB_HAS_CHILD(node, 1));
		cb_zap_node(tree, tree->root);
		cb_check_node(t);
		t->parent = NULL;
		tree->root = t;
		break;
	    case 0:
		cb_zap_node(tree, tree->root);
		tree->root = NULL;
		return;
	    }
	}

    }

    cb_check_node(tree->root);
}

CB_STATIC CB_INLINE cb_node_t cb_index(const cb_node_t tree, const cb_key key) {
    cb_node_t node = tree;
    if (tree) cb_check_node(tree);

    while (node) {
	if (CB_LT(node->key.len, key.len)) {
	    unsigned INT32 bit = CB_GET_BIT(key.str, node->key.len);

	    if (CB_HAS_CHILD(node, bit)) {
		node = CB_CHILD(node, bit);
		continue;
	    }
	} else if (CB_LT(key.len, node->key.len)) {
	    return NULL;
	} else if (CB_KEY_EQ(node->key, key)) {
	    cb_check_node(tree);
	    return node;
	}

	break;
    }

    if (tree) cb_check_node(tree);
    return NULL;
}

CB_STATIC CB_INLINE cb_node_t cb_find_next(const cb_node_t tree,
					   const cb_key key) {
    cb_size size;
    size_t bit;
    cb_node_t node;
    size.bits = size.chars = 0;

    /*  index is cheap. also in many cases its quite likely that we */
    /*  hit. */
    node = cb_index(tree, key);

    if (node) {
	WALK_FORWARD(node, {
	    if (CB_HAS_VALUE(_)) break;
	});
	return node;
    }

    node = tree;

    while (1) {
#ifdef ANNOY_DEBUG
	printf("prefix: (%d,%d)\n", key.len.chars, key.len.bits);
	cb_size min = CB_MIN(node->key.len, key.len);
	printf("(%p) start: (%d,%d) stop: (%d,%d) ", node, size.chars,
	       size.bits, min.chars, min.bits);
#endif
	size = cb_prefix_count(node->key.str, key.str,
			       CB_MIN(node->key.len, key.len), size);
#ifdef ANNOY_DEBUG
	printf("prefix: (%d,%d)\n", size.chars, size.bits);
#endif

	if (CB_S_EQ(size, key.len)) { /*  key is substring */
	    if (!CB_S_EQ(size, node->key.len)) { /*  key is not equal */
		if (CB_HAS_VALUE(node))
		    return node; /*  key is smaller  */
	    } else WALK_FORWARD(node, {
		if (CB_HAS_VALUE(_)) return _;
	    });
	}

	bit = CB_GET_BIT(key.str, size);

	/* printf("bit is %u\n", bit); */

	if (CB_S_EQ(size, node->key.len)) { /*  node is substring */
	    if (CB_HAS_CHILD(node, bit)) {
		node = CB_CHILD(node, bit);
		continue;
	    }
	    if (!bit && CB_HAS_CHILD(node, 1)) {
		WALK_FORWARD(node, {
		    if (CB_HAS_VALUE(_)) return _;
		});
	    }

	    return NULL;
	}

	if (!bit) break;

	WALK_UP(node, bit, {
	    if (!bit && CB_HAS_CHILD(_, 1)) {
		_ = CB_CHILD(_, 1);
		break;
	    }
	});
	if (node == tree) return NULL;
	break;
    }

    if (node && !CB_HAS_VALUE(node))
	WALK_FORWARD(node, {
	    if (CB_HAS_VALUE(_)) break;
	});
    return node;
}

CB_STATIC CB_INLINE cb_node_t cb_find_ne(const cb_node_t tree, const cb_key key) {
    cb_node_t ne = cb_index(tree, key);
    if (!ne) ne = cb_find_next(tree, key);
    return ne;
}

CB_STATIC CB_INLINE cb_node_t cb_get_nth(const cb_node_t tree, size_t n) {
    cb_node_t node = tree;
    size_t ln;

    while (node) {
	if (n >= node->size) return NULL;

	if (n == 0) return cb_find_first(node);
	else if (n == node->size - 1) return cb_find_last(node);

	if (CB_HAS_VALUE(node)) n--;

	if (CB_HAS_CHILD(node, 0)) {
	    ln = CB_CHILD(node, 0)->size;
	    if (n < ln) {
		node = CB_CHILD(node, 0);
		continue;
	    }
	    n -= ln;
	}

	node = CB_CHILD(node, 1);
    }

    return NULL;
}

CB_STATIC CB_INLINE cb_node_t cb_find_previous(const cb_node_t tree,
					 const cb_key key) {
    cb_node_t node = cb_index(tree, key);
    if (!node) node = cb_find_next(tree, key);
    if (!node) return cb_find_last(tree);
    if (node) WALK_BACKWARD(node, {
	if (CB_HAS_VALUE(_)) break;
    });
    return node;
}

CB_STATIC CB_INLINE cb_node_t cb_find_le(const cb_node_t tree, const cb_key key) {
    cb_node_t ne = cb_index(tree, key);
    if (!ne) ne = cb_find_previous(tree, key);
    return ne;
}

static inline int cb_low_insert(struct cb_tree * tree,
				const cb_key key, const cb_value *val) {
    cb_node_t node = tree->root;
    cb_size size;
    size.bits = 0;
    size.chars = 0;

    while (1) {
	cb_node_t new;
	size_t bit;

	size = cb_prefix_count(node->key.str, key.str,
			       CB_MIN(node->key.len, key.len), size);

	if (CB_S_EQ(size, key.len)) {
	    cb_node_t klon;

	    if (CB_S_EQ(size, node->key.len)) {
		unsigned INT32 bit;

		klon = node;
		if (CB_HAS_VALUE(klon))
		    WALK_UP(klon, bit, {
			_->size--;
		    });
		else node->size++;
		/*  remove ref */
		/* free_svalue(&node->value); */
		CB_SET_KEY(node, key);
		CB_SET_VALUE(node, val);

		return 0;
	    }
	    /*  overwrite not inplace by new key node */
	    klon = cb_clone_node(tree, node);
	    node->size++;
	    bit = CB_GET_BIT(node->key.str, size);

	    /*  add ref for value */
	    CB_SET_KEY(node, key);
	    CB_SET_VALUE(node, val);

	    node->key.len = size;
	    CB_SET_CHILD(node, bit, klon);
	    CB_SET_CHILD(node, !bit, NULL);

	    return 1;
	}

	if (likely(CB_S_EQ(size, node->key.len))) {
	    node->size++;
	    bit = CB_GET_BIT(key.str, size);
	    if (CB_HAS_CHILD(node, bit)) {
		node = CB_CHILD(node, bit);
		continue;
	    }
	    CB_SET_CHILD(node, bit, cb_node_from_string(tree, key, val));
	    return 1;
	}

	new = cb_clone_node(tree, node);
	node->size++;
#ifdef DEBUG_CHECKS
	if (CB_LT(CB_MIN(node->key.len, key.len), size)) {
	    CB_ERROR(("fooo\n"));
	}
	if (CB_LT(node->key.len, size)) {
	    CB_ERROR(("enlarging node [%d, %d] vs [%d, %d]\n", size.chars,
		      size.bits, node->key.len.chars, node->key.len.bits));
	}
#endif /* DEBUG_CHECKS */
	node->key.len = size;
	bit = CB_GET_BIT(key.str, size);
	CB_SET_CHILD(node, bit, cb_node_from_string(tree, key, val));
	CB_SET_CHILD(node, !bit, new);
	CB_RM_VALUE(node); /*  do not free here, clone does take ref */

	return 1;
    }
}

CB_STATIC CB_INLINE void cb_insert(struct cb_tree * tree,
				   const cb_key key,
				   const cb_value *val) {
	if (!tree->root) {
	    tree->root = cb_node_from_string(tree, key, val);
	    return;
	}

	cb_check_node(tree->root);

	cb_low_insert(tree, key, val);

	cb_check_node(tree->root);
}
