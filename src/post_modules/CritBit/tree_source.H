#ifdef PIKE_NEW_BLOCK_ALLOC
    void TREE_CLASSIFY(relocate_simple)(void * start, void * stop, ptrdiff_t diff) {
	cb_node_t node = (cb_node_t)start;

	if (THIS->tree) THIS->tree += diff;

	while (node < (cb_node_t)stop) {
	    if (node->parent) node->parent += diff;
	    if (node->childs[0]) node->childs[0] += diff;
	    if (node->childs[1]) node->childs[1] += diff;
	    node++;
	}
	THIS->rev ++;
    }
    static cb_node_t TREE_CLASSIFY(node_alloc)() {
	return ba_lalloc(&THIS->allocator);
    }
    static void TREE_CLASSIFY(node_free)(cb_node_t node) {
	ba_lfree(&THIS->allocator, node);
    }
#endif

    INIT {
	THIS->tree = NULL;
	THIS->encode_fun = find_identifier
		("encode_key", Pike_fp->current_object->prog);
	THIS->decode_fun=find_identifier
		("decode_key", Pike_fp->current_object->prog);
	THIS->copy_fun = find_identifier ("copy", Pike_fp->current_object->prog);
	THIS->insert_fun = find_identifier ("`[]=", Pike_fp->current_object->prog);
	if (find_identifier("copy", TREE_CLASSIFY(_program)) == THIS->copy_fun) {
	    THIS->copy_fun = -1;
	}

	if (find_identifier("`[]=", TREE_CLASSIFY(_program)) == THIS->insert_fun) {
	    THIS->copy_fun = -1;
	}
#ifdef PIKE_NEW_BLOCK_ALLOC
	ba_init_local(&THIS->allocator, sizeof(struct cb_node), 32, 256,
		      TREE_CLASSIFY(relocate_simple), NULL);
#endif
    }
static inline void TREE_CLASSIFY(_copy_node)(struct object *o, cb_node_t node) {
    if (THIS->copy_fun != -1 && THIS->insert_fun != -1) {
	CB_PUSH_TRANSFORM_KEY(node->key);
	CB_PUSH_VALUE(node->value);
	apply_low(o, THIS->insert_fun, 2);
	pop_stack();
    } else cmod_OBJ2_TREE(o)->tree =
		cb_insert(cmod_OBJ2_TREE(o)->tree, node->key, &node->value);
}

static inline void TREE_CLASSIFY(_copy_tree)(struct object *o, cb_node_t node) {
	if (THIS->copy_fun != -1 && THIS->insert_fun != -1) {
	    WALK_FORWARD(node, {
		if (CB_HAS_VALUE(_)) {
		    CB_PUSH_TRANSFORM_KEY(node->key);
		    push_svalue(&node->value);
		    apply_low(o, THIS->insert_fun, 2);
		    pop_stack();
		}
	    });
	} else cmod_OBJ2_TREE(o)->tree = cb_copy_tree(node);
}

static inline struct object *
TREE_CLASSIFY(_clone_object)(const struct object *o) {
    struct object * t = NULL;
    if (THIS->copy_fun != -1) {
	apply_low(o, THIS->copy_fun, 0);
	if (!Pike_sp[-1].type != PIKE_T_OBJECT)
	    Pike_error("clone() is supposed to return an object.\n");
	t = Pike_sp[-1].u.object;
	add_ref(t);
	pop_stack();
    } else {
	t = clone_object(Pike_fp->current_object->prog, 0);
	if (cmod_OBJ2_TREE(o)->tree)
	    cmod_OBJ2_TREE(t)->tree = cb_copy_tree(cmod_OBJ2_TREE(o)->tree);
    }
    return t;
}

cmod_DEFINE_EVAL(CLONE_OBJECT, TREE_CLASSIFY(_clone_object))

cmod_DEFINE_EVAL(COPY_TREE, TREE_CLASSIFY(_copy_tree))

cmod_DEFINE_EVAL(COPY_NODE, TREE_CLASSIFY(_copy_node))

    /*! @decl mixed _m_delete(mixed key)
     *! m_delete callback.
     */
    PIKEFUN mixed _m_delete(mixed key) {
	size_t size, oldsize;
	cb_key k;

	k = CB_KEY_FROM_SVALUE(key);

	oldsize = (THIS->tree) ? THIS->tree->size : 0;
	if (oldsize) {
	    THIS->tree = cb_delete(THIS->tree, k, Pike_sp++);
	    size = (THIS->tree) ? THIS->tree->size : 0;
	    if (size < oldsize) {
		THIS->rev++;
		stack_pop_keep_top();
		return;
	    }
	}
	pop_stack();
	push_undefined();
    }

    PIKEFUN int check_parenting_skills() {
	push_int(cb_rec_check_parents(THIS->tree));
    }

    DOCSTART() @decl void create(array|mapping|void o)
     *! Create a tree_class from o.
    DOCEND()
    PIKEFUN void create (array|mapping|void o)
      optflags OPT_TRY_OPTIMIZE;
    {

	if (args == 1) {
	    switch (o->type) {
	    case PIKE_T_MAPPING: {
		struct keypair *k;
		INT32 e;
		struct mapping_data *md = o->u.mapping->data;

		NEW_MAPPING_LOOP(md) {
		    cb_key key = CB_KEY_FROM_SVALUE(&k->ind);
		    THIS->tree = cb_insert(THIS->tree, key, &k->val);
		}
		return;
	    }
	    case PIKE_T_ARRAY: {
		INT32 i;

		if (o->u.array->size & 1) goto ARG_ERROR;

		for (i = 0; i < o->u.array->size; i+=2) {
		    cb_key key = CB_KEY_FROM_SVALUE(ITEM(o->u.array)+i);
		    THIS->tree = cb_insert(THIS->tree, key, ITEM(o->u.array)+i+1);
		}
		return;
	    }
	    }
ARG_ERROR:
	    SIMPLE_BAD_ARG_ERROR("create", 1, "mapping(" cmod_STRFY_EVAL(key_ptype) ":mixed)|array");
	}
    }

DOCSTART() @decl int _sizeof()
 *! Gives the number of entries in the @[tree_class].
DOCEND()
    PIKEFUN int _sizeof() {
	push_int(THIS->tree ? THIS->tree->size : 0);
    }

DOCSTART() @decl array _indices()
 *! Returns a sorted array of indices of the @[tree_class].
DOCEND()
    PIKEFUN array _indices() {
	size_t size = THIS->tree ? THIS->tree->size : 0;
	if (size) {
	    size_t start = 0;
	    struct svalue * s;
	    cb_key k;
	    cb_node_t node = THIS->tree;
	    struct array * a = allocate_array_no_init(size, 0);
	    push_array(a);
	    if (CB_HAS_VALUE(node)) {
		s = ITEM(a)+start;
		s->type = T_VOID;
		k = node->key;
		CB_ASSIGN_SVALUE_KEY(k, s);
		start ++;
	    }
	    WALK_FORWARD(node, {
		if (CB_HAS_VALUE(_)) {
		    if (start == size) {
			Pike_error("super bad!! tree has hidden entries.\n");
		    }
		    s = ITEM(a)+start;
		    s->type = T_VOID;
		    CB_ASSIGN_SVALUE_KEY(_->key, s);
		    start ++;
		}
	    });
	    return;
	}
	ref_push_array(&empty_array);
    }

DOCSTART() @decl array _values()
 *! Returns an array of values of the @[tree_class] object. The returned
 *! array matches
 *! @[_indices] so that @expr{mkmapping(indices(tree), values(tree))@} would
 *! create a mapping with the same contents as this @[tree_class].
DOCEND()
    PIKEFUN array _values() {
	size_t size = THIS->tree ? THIS->tree->size : 0;
	if (size) {
	    struct array * a = allocate_array_no_init(size, 0);
	    push_array(a);
	    cb_aggregate_values(THIS->tree, a, 0, size);
	    return;
	}
	ref_push_array(&empty_array);
    }

    /*! @decl mixed `[]=(mixed key, mixed val) */
    PIKEFUN mixed `[]=(mixed key, mixed val) {
	cb_key k = CB_KEY_FROM_SVALUE(key);
	THIS->tree = cb_insert(THIS->tree, k, val);
#ifdef DEBUG_CHECK
	cb_check(THIS->tree, THIS->tree, k->str);
#endif /* DEBUG_CHECK */
	stack_pop_keep_top();
    }

    PIKEFUN mixed ninsert(mixed key, mixed val, int chars, int bits) {
	cb_size len;
	cb_key k;

	k = CB_KEY_FROM_SVALUE(key);
	len.chars = chars;
	len.bits = bits;
	if (CB_LT(k.len, len)) {
	    Pike_error("chars, bits are too big for key.\n");
	}
	THIS->tree = cb_insert(THIS->tree, k, val);
	stack_pop_keep_top();
    }

    /*! @decl mixed cast(string type)
     *! Cast callback. Supports only cast to mapping and
     *! behaves as the inverse of create().
     */
    PIKEFUN mixed cast(string type) {
	struct pike_string *mapping_t;
	REF_MAKE_CONST_STRING(mapping_t, "mapping");

	if (type == mapping_t) {
	    pop_stack();
	    cmod_CONCAT_EVAL(f_, tree_class, _cq__indices)(0);
	    cmod_CONCAT_EVAL(f_, tree_class, _cq__values)(0);
	    f_mkmapping(2);
	    free_string(mapping_t);
	    return;
	}
	free_string(mapping_t);
	Pike_error("Cannot cast to %S\n", type);
    }

    /*! @decl mixed nth(int(0..) n)
     *! Get the @expr{n@}th entry in order.
     *! @returns
     *! 	An array @expr{({ key, value })@}.
     */
    PIKEFUN mixed nth(int n) {
	cb_node_t node;

	if (THIS->tree) {
	    if (n >= 0 && (size_t)n < THIS->tree->size) {
		node = cb_get_nth(THIS->tree, n);
		if (node) {
		    pop_stack();
		    CB_PUSH_TRANSFORM_KEY(node->key);
		    push_svalue(&node->value);
		    f_aggregate(2);
		    return;
		}
	    }
	}

	pop_stack();
	push_undefined();
	return;
    }

    /*! @decl array _random()
     *! Get a random entry.
     *! @returns
     *! 	An array @expr{({ key, value })@}.
     */
    PIKEFUN array _random() {
	cb_node_t node;

	if (THIS->tree) {
	    node = cb_get_nth(THIS->tree, my_rand() % THIS->tree->size);
	    if (node) {
		CB_PUSH_TRANSFORM_KEY(node->key);
		push_svalue(&node->value);
		f_aggregate(2);
		return;
	    }
	}

	push_undefined();
	return;
    }

    /*! @decl mixed `[](mixed key)
     */
    PIKEFUN mixed `[](mixed key) {
	CB_TRANSFORM_KEY(key);
	if (key->type & T_KEY) {
	    cb_node_t n;
	    cb_key k = CB_LOW_KEY_FROM_SVALUE(key);
	    pop_stack();
	    n = cb_index(THIS->tree, k);

	    if (n && CB_HAS_VALUE(n)) {
		push_svalue(&n->value);
		return;
	    }
	} else pop_stack();
	push_undefined();
    }

    /*! @decl int(0..1) _equal(mixed o)
     */
    PIKEFUN int _equal(mixed o) {
	if (o->type == PIKE_T_OBJECT
	    && o->u.object->prog == Pike_fp->current_object->prog) {
	    cb_node_t tree1, tree2;
	    tree1 = THIS->tree;
	    tree2 = cmod_OBJ2_TREE(o->u.object)->tree;

	    if (tree1 == tree2) {
		push_int(1);
		return;
	    }

	    if (tree1 && tree2 && tree1->size == tree2->size) {
		WALK_FORWARD(tree1, {
		    if (CB_HAS_VALUE(_)) {
			cb_node_t n = cb_index(tree2, _->key);
			if (!n || !is_equal(&_->value, &n->value)) {
			    push_int(0);
			    return;
			}
		    }
		});
		push_int(1);
		return;
	    }
	}
	push_int(0);
	return;
    }

    /*! @decl mixed `-(mixed o)
     *! Sub[s]tract two trees from each other (key-wise).
     */
    PIKEFUN mixed `-(mixed o) {
	struct object * res;
	cb_node_t node, tree = NULL;

	if (o->type != PIKE_T_OBJECT || -1
	    == low_get_storage(o->u.object->prog, TREE_CLASSIFY(_program))) {
	    SIMPLE_BAD_ARG_ERROR("`-", 1, "CritBit." cmod_STRFY_EVAL(tree_class));
	}

	node = cmod_OBJ2_TREE(o->u.object)->tree;

	if (THIS->tree) {
	    if (node) {
		cb_node_t t = THIS->tree;
		res = clone_object(Pike_fp->current_object->prog, 0);

		if (node != THIS->tree)
		    WALK_FORWARD(t, {
			if (CB_HAS_VALUE(_) && !cb_index(node, _->key))
			    COPY_NODE(res, _);
		    });
	    } else res = CLONE_OBJECT(Pike_fp->current_object);
	} else res = clone_object(Pike_fp->current_object->prog, 0);

	push_object(res);
	return;
    }

    /* FIXME: we should allow for extra arguments and call += on the copy
     * of the first one
     */
    /*! @decl mixed `+(mixed o)
     *!	Add callback. Returns the union of two trees.
     */
    PIKEFUN mixed `+(mixed o) {
	cb_node_t tree1, tree2;

	if (o->type != PIKE_T_OBJECT || -1
	    == low_get_storage(o->u.object->prog, TREE_CLASSIFY(_program))) {
	    SIMPLE_BAD_ARG_ERROR("`+", 1, "inherits(CritBit." cmod_STRFY_EVAL(tree_class) ")");
	}
	tree1 = THIS->tree;
	tree2 = cmod_OBJ2_TREE(o->u.object)->tree;

	if (tree1 && tree2) {
	    struct object * res = NULL;
	    if (tree1->size < tree2->size) {
		tree1 = tree2;
		tree2 = THIS->tree;
		res = CLONE_OBJECT(o->u.object);
	    } else
		res = CLONE_OBJECT(Pike_fp->current_object);

	    if (tree1 != tree2) {
		if (CB_HAS_VALUE(tree2)) COPY_NODE(res, tree2);
		WALK_FORWARD(tree2, {
		    if (CB_HAS_VALUE(_)) COPY_NODE(res, _);
		});
	    }
	    push_object(res);
	} else if (tree1) {
	    push_object(CLONE_OBJECT(Pike_fp->current_object));
	} else if (tree2) {
	    push_object(CLONE_OBJECT(o->u.object));
	}

	return;
    }

    /*! @decl mixed `[..](mixed a, int atype, mixed b, int btype)
     *! @seealso
     *! 	@[predef::`[..]]
     */
    PIKEFUN mixed `[..](mixed a, int atype, mixed b, int btype) {
	struct object * o = NULL;

	if (THIS->tree) {
	    cb_node_t start = NULL, stop = NULL;

	    switch (atype & (INDEX_FROM_BEG | INDEX_FROM_END | OPEN_BOUND)) {
	    case OPEN_BOUND:
		start = cb_find_first(THIS->tree);
		break;
	    case INDEX_FROM_BEG:
		start = cb_find_ne(THIS->tree, CB_KEY_FROM_SVALUE(a));
		break;
	    case INDEX_FROM_END:
		goto illegal_index;
	    }

	    if (!start) goto range_do_clone;

	    switch (btype & (INDEX_FROM_BEG | INDEX_FROM_END | OPEN_BOUND)) {
	    case OPEN_BOUND:
		stop = cb_find_last(THIS->tree);
		break;
	    case INDEX_FROM_BEG:
		stop = cb_find_le(THIS->tree, CB_KEY_FROM_SVALUE(b));
		break;
	    case INDEX_FROM_END:
		goto illegal_index;
	    }

	    o = clone_object(Pike_fp->current_object->prog, 0);

	    if (!stop) goto range_do_clone;

	    if (start == stop) {
		COPY_NODE(o, start);
		goto range_do_clone;
	    }

	    if (CB_KEY_LT(start->key, stop->key)) {
		COPY_NODE(o, start);
		WALK_FORWARD(start, {
		    if (CB_HAS_VALUE(_))
			COPY_NODE(o, _);
		    if (_ == stop) break;
		});
	    }
	} else {
	    if (atype & INDEX_FROM_END || btype & INDEX_FROM_END)
		goto illegal_index;
	}
range_do_clone:
	if (!o)
	    o = clone_object(Pike_fp->current_object->prog, 0);
	pop_n_elems(args); /* REVIEW: we need this, right? */
	push_object(o);
	return;

illegal_index:
	Pike_error("INDEX_FROM_END is not supported for CritBit `[..].\n");
    }

#cmod_if 0
    PIKEFUN mixed `[](mixed a, mixed b) {
	struct object * o = NULL;

	if (THIS->tree) {
	    cb_node_t start = NULL, stop = NULL;

	    if (TYPEOF(*a) == PIKE_T_INT && !a->u.integer) {
		start = cb_find_first(THIS->tree);
	    } else {
		cb_key key = CB_KEY_FROM_SVALUE(a);
		start = cb_find_ne(THIS->tree, key);
	    }

	    if (!start) goto range_do_clone;

	    if (TYPEOF(*b) == PIKE_T_INT && b->u.integer == MAX_INT_TYPE) {
		stop = cb_find_last(THIS->tree);
	    } else {
		cb_key key = CB_KEY_FROM_SVALUE(b);
		stop = cb_find_le(THIS->tree, key);
	    }

	    o = clone_object(Pike_fp->current_object->prog, 0);

	    if (!stop) goto range_do_clone;

	    if (start == stop) {
		COPY_NODE(o, start);
		goto range_do_clone;
	    }

	    if (CB_KEY_LT(start->key, stop->key)) {
		COPY_NODE(o, start);
		WALK_FORWARD(start, {
		    if (CB_HAS_VALUE(_))
			COPY_NODE(o, _);
		    if (_ == stop) break;
		});
	    }
	}
range_do_clone:
	if (!o)
	    o = clone_object(Pike_fp->current_object->prog, 0);
	pop_n_elems(args); /* REVIEW: we need this, right? */
	push_object(o);
	return;
    }
#cmod_endif

    PIKEFUN string ugly() {
	if (THIS->tree) {
	    struct string_builder s;

	    init_string_builder(&s, 0);
	    cb_print_tree(&s, THIS->tree, 0);
	    push_string(finish_string_builder(&s));
	    return;
	}
	push_text("");
    }

    DOCSTART() @decl key_ptype first()
     *! Get the lexicographically first index in the tree.
    DOCEND()
    PIKEFUN key_ptype first() {
	if (THIS->tree) {
	    cb_node_t first = cb_find_first(THIS->tree);
	    if (first) {
		CB_PUSH_TRANSFORM_KEY(first->key);
		return;
	    }
	}
	push_undefined();
    }

    DOCSTART() @decl key_ptype last()
     *! Get the lexicographically last index in the tree.
    DOCEND()
    PIKEFUN key_ptype last() {
	if (THIS->tree) {
	    cb_node_t last = cb_find_last(THIS->tree);
	    if (last) {
		CB_PUSH_TRANSFORM_KEY(last->key);
		return;
	    }
	}
	push_undefined();
    }

    /*! @decl string bkey(mixed key)
     *! Render the internally used binary representation of
     *! the key into a string as a strings of '0's and '1's.
     */
    PIKEFUN string bkey(mixed key) {
	struct string_builder s;
	cb_key k;

	init_string_builder(&s, 0);
	k = CB_KEY_FROM_SVALUE(key);
	pop_stack();
	cb_print_key(&s, k);

	push_string(finish_string_builder(&s));
    }

    DOCSTART() @decl key_ptype previous(mixed current)
     *! Get the key before @expr{current@} in lexicographical order.
    DOCEND()
    PIKEFUN key_ptype previous(mixed current) {
	CB_CHECK_KEY(current, "previous", 1);

	if (THIS->tree) {
	    cb_node_t previous;

	    previous = cb_find_previous(THIS->tree, CB_LOW_KEY_FROM_SVALUE(current));
	    pop_stack();
	    if (previous) {
		CB_PUSH_TRANSFORM_KEY(previous->key);
		return;
	    }
	}
	push_undefined();
    }

    DOCSTART() @decl key_ptype next(mixed current)
     *! Get the key after @expr{current@} in lexicographical order.
    DOCEND()
    PIKEFUN key_ptype next(mixed current) {
	CB_CHECK_KEY(current, "next", 1);

	if (THIS->tree) {
	    cb_node_t next;

	    next = cb_find_next(THIS->tree, CB_LOW_KEY_FROM_SVALUE(current));
	    pop_stack();
	    if (next) {
		CB_PUSH_TRANSFORM_KEY(next->key);
		return;
	    }
	}
	push_undefined();
    }

    DOCSTART() @decl object _get_iterator()
     *! @returns
     *! This function returns an @[Iterator]-instance as would be created
     *! with @expr{ADT.CritBit.TREE_CLASSIFY(.Iterator)(tree)@}
    DOCEND()
    PIKEFUN object _get_iterator() {
	ref_push_object(Pike_fp->current_object);
	push_object(clone_object(cmod_CONCAT_EVAL(tree_class,_,iterator_class,_program), 1));
    }

    /*! @decl int(0..) depth()
     *! Calculate the depth of the tree.
     */
    PIKEFUN int depth() {
	if (THIS->tree) {
	    push_int(cb_get_depth(THIS->tree));
	    return;
	}

	push_int(0);
	return;
    }

    DOCSTART() @decl tree_class copy()
     *! Create a copy of the tree.
    DOCEND()
    PIKEFUN object copy() {
	struct object * copy;
	copy = clone_object(Pike_fp->current_object->prog, 0);
	if (THIS->tree) {
	    cmod_OBJ2_TREE(copy)->tree = cb_copy_tree(THIS->tree);
	}
	push_object(copy);
    }

    DOCSTART() @decl tree_class get_subtree(void|mixed key)
     *! Get a copy of the subtree starting at prefix
     *! @expr{key@}.
    DOCEND()
    PIKEFUN object get_subtree(void|mixed key) {
	struct object * sub;
	cb_node_t node = NULL;

	if (THIS->tree) {
	    if (args == 0 || IS_UNDEFINED(key))
		node = THIS->tree;
	    else
		node = cb_subtree_prefix(THIS->tree, CB_LOW_KEY_FROM_SVALUE(key));
	}
	sub = clone_object(Pike_fp->current_object->prog, 0);

	pop_n_elems(args);

	if (node) {
	    COPY_TREE(sub, node);
	}

	push_object(sub);
    }

    EXIT {
	cb_node_t node;

#ifdef PIKE_NEW_BLOCK_ALLOC
	if ((node = THIS->tree)) {
	    THIS->tree = NULL;
	    CB_FREE_KEY(node->key);
	    if (CB_HAS_VALUE(node)) {
		CB_FREE_VALUE(&node->value);
	    }
	    WALK_FORWARD(node, {
		CB_FREE_KEY(node->key);
		if (CB_HAS_VALUE(_)) {
		    CB_FREE_VALUE(&node->value);
		}
	    });
	    return;
	}
	ba_ldestroy(&THIS->allocator);
#else
	if ((node = THIS->tree)) {
	    THIS->tree = NULL;
	    cb_free_node(node);
	}
#endif
    }

/*! @endclass */
