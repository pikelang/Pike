static inline void cb_print_key(struct string_builder *, const cb_key);
static inline size_t _low_cb_check_node(cb_node_t node, const char *, int);

#ifdef DEBUG_CHECKS
# define cb_check_node(node)	do { _low_cb_check_node(node, \
					    __FILE__, __LINE__); } while (0)
#else
# define cb_check_node(node)	do {} while(0)
#endif

#define CB_FATAL(x)	Pike_error x

#include "tree_low.c"

#ifndef CB_FIRST_CHAR
#define CB_FIRST_CHAR(key)	(0)
#endif

static inline void cb_debug_print_key(struct string_builder * buf, cb_key key) {
    cb_size i;

    for (i.chars = CB_FIRST_CHAR(key); i.chars < key.len.chars; i.chars++) {
#ifdef CB_PRINT_CHAR
	CB_PRINT_CHAR(buf, key.str, i.chars);
#else
	string_builder_sprintf(buf, "(%d, %d) b: ", i.chars, CB_WIDTH(s));

	for (i.bits = 0; i.bits < CB_WIDTH(s); i.bits++) {
	    string_builder_sprintf(buf, "%d", CB_GET_BIT(key.str, i));
	}
	string_builder_putchar(buf, ' ');
#endif
    }

    /* string_builder_putchars(buf, ' ', 11-key.len.chars); */

    if (key.len.bits) {
	i.chars = key.len.chars;
	i.bits = 0;

	string_builder_sprintf(buf, "(%d, %d) b: ", key.len.chars, key.len.bits);

	for (; CB_LT(i, key.len); CB_INC(i, 0, 1)) {
	    string_builder_sprintf(buf, "%d", CB_GET_BIT(key.str, i));
	}
	string_builder_sprintf(buf, " %d", CB_GET_BIT(key.str, key.len));
    }
}

static inline void cb_print_key(struct string_builder * buf, const cb_key key) {
    cb_size i;

    for (i.chars = 0; i.chars < key.len.chars; i.chars++) {
	for (i.bits = 0; i.bits < CB_WIDTH(s); i.bits++) {
	    string_builder_putchar(buf, CB_GET_BIT(key.str, i) ? '1' : '0');
	}
    }

    /* string_builder_putchars(buf, ' ', 11-key.len.chars); */

    if (key.len.bits) {
	i.chars = key.len.chars;
	i.bits = 0;

	for (; CB_LT(i, key.len); CB_INC(i, 0, 1)) {
	    string_builder_putchar(buf, CB_GET_BIT(key.str, i) ? '1' : '0');
	}
    }
}

static inline void cb_print_node(struct string_builder * buf,
				 cb_node_t node, int depth) {
    string_builder_putchars(buf, ' ', depth);
    string_builder_sprintf(buf, "%x #%lu (%d) --> ", node,
			   node->size, node->value.type);
    string_builder_putchars(buf, ' ', MAXIMUM(0, 15-depth));
    cb_debug_print_key(buf, node->key);
    if (CB_HAS_VALUE(node)) CB_PRINT_KEY(buf, node->key);
    string_builder_putchar(buf, '\n');
}

static inline void cb_print_tree(struct string_builder *buf,
				 cb_node_t tree, int depth) {
    cb_print_node(buf, tree, depth);
#if 0
    char *spaces = (char*)malloc(depth+1);
    char fmt[4096] = "%s %";
    cb_size i;
    int binary = 0;
    memset(spaces, ' ', depth);
    spaces[depth] = 0;
    if (tree->key.len.bits) {
	sprintf(fmt, "%%s-> %%.%ds %%0%dd \n",
		tree->key.len.chars, tree->key.len.bits);
	i.chars = tree->key.len.chars;
	i.bits = 0;

	for (; CB_LT(i, tree->key.len); CB_INC(i, 0, 1)) {
	    binary *= 10;
	    binary += CB_GET_BIT(tree->key.str, i);
	}
	printf(fmt, spaces, tree->key->str, binary);
    } else {
	sprintf(fmt, "%%s-> %%.%ds \n", tree->key.len.chars);
	printf(fmt, spaces, tree->key->str);
    }
    free(spaces);
#endif
    if (CB_HAS_CHILD(tree, 0)) {
	string_builder_putchar(buf, 'l');
	cb_print_tree(buf, CB_CHILD(tree, 0), depth + 1);
    }
    if (CB_HAS_CHILD(tree, 1)) {
	string_builder_putchar(buf, 'r');
	cb_print_tree(buf, CB_CHILD(tree, 1), depth + 1);
    }
}

static inline int cb_print_path(struct string_builder *buf, cb_node_t node,
				cb_key key, cb_size size, int depth,
				cb_node_t end) {
    cb_print_node(buf, node, depth);

    if (node == end) return 0;

    size = cb_prefix_count(node->key.str, key.str,
			   CB_MIN(node->key.len, key.len), size);

    if (CB_S_EQ(size, key.len)) {
	if (CB_S_EQ(size, node->key.len)) {
	    return 1;
	} else return 0;
    }

    if (CB_S_EQ(size, node->key.len)) {
	size_t bit = CB_GET_BIT(key.str, size);
	if (CB_HAS_CHILD(node, bit))
	    return cb_print_path(buf, CB_CHILD(node, bit), key, size, depth+1,
				 end);
    }

    return 0;
}

static inline void cb_check(cb_node_t node,
                            cb_node_t DEBUGUSED(last),
                            char *issue) {
#ifdef DEBUG_CHECKS
    if (CB_LT(node->key.len, last->key.len)) {
	struct string_builder buf;
	init_string_builder(&buf, 0);
	push_text("ERROR AT %s: %s\n[%d, %d] is shorter than [%d, %d]\n");
	push_text(issue);
	cb_print_tree(&buf, last, 0);
	push_string(finish_string_builder(&buf));
	push_int(node->key.len.chars);
	push_int(node->key.len.bits);
	push_int(last->key.len.chars);
	push_int(last->key.len.bits);
	f_werror(7);
	pop_n_elems(1);
	return;
    }
#endif /* DEBUG_CHECKS */

    if (CB_HAS_CHILD(node, 0)) cb_check(CB_CHILD(node, 0), node, issue);
    if (CB_HAS_CHILD(node, 1)) cb_check(CB_CHILD(node, 1), node, issue);
}

static inline int cb_rec_check_parents(cb_node_t node) {
    if (node == NULL) return 0;
    if (CB_HAS_CHILD(node, 0)) {
	if (CB_CHILD(node, 0)->parent != node) {
	    printf("Damaged 0.\n");
	    return 1;
	}
	if (cb_rec_check_parents(CB_CHILD(node, 0))) return 1;
    }
    if (CB_HAS_CHILD(node, 1)) {
	if (CB_CHILD(node, 1)->parent != node) {
	    printf("Damaged 1.\n");
	    return 1;
	}
	if (cb_rec_check_parents(CB_CHILD(node, 1))) return 1;
    }
    return 0;
}

static inline void cb_aggregate_values(cb_node_t node,
				       struct array * a, size_t start,
				       size_t UNUSED(len)) {
    if (CB_HAS_VALUE(node))
	CB_GET_VALUE(node, ITEM(a)+start++);
    WALK_FORWARD(node, {
	if (CB_HAS_VALUE(_)) CB_GET_VALUE(_, ITEM(a)+start++);
    });
}

static inline size_t _low_cb_check_node(cb_node_t node,
					const char *file, int line) {
    size_t len = 0;
    if (CB_HAS_CHILD(node, 0)) {
	if (CB_GET_BIT(CB_CHILD(node, 0)->key.str, node->key.len) != 0) {
	    Pike_error("%s:%d EVIL DOER FOUND.\n", file, line);
	}
	if (CB_LE(CB_CHILD(node, 0)->key.len, node->key.len)) {
	    Pike_error("%s:%d Child is shorter than parent.\n", file, line);
	}
	len += _low_cb_check_node(CB_CHILD(node, 0), file, line);
    }
    if (CB_HAS_CHILD(node, 1)) {
	if (CB_GET_BIT(CB_CHILD(node, 1)->key.str, node->key.len) != 1) {
	    Pike_error("%s:%d It was the gardener! \n", file, line);
	}
	if (CB_LE(CB_CHILD(node, 1)->key.len, node->key.len)) {
	    Pike_error("%s:%d Child is shorter than parent.\n", file, line);
	}
	len += _low_cb_check_node(CB_CHILD(node, 1), file, line);
    }

    if (len + CB_HAS_VALUE(node) != node->size) {
	/* Pike_error("Found node with wrong size. is: %p\n", node); */
	Pike_error("%s:%d Found node with wrong size. is: 0x%08X\n",
		   file, line, node);
    }

    return node->size;
}

