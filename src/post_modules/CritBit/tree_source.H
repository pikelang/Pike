/* -*- mode: C; c-basic-offset: 4; -*- */
#ifndef CB_GC_RECURSE_KEY
# define CB_GC_RECURSE_KEY(X)
#endif
#ifndef CB_GC_CHECK_KEY
# define CB_GC_CHECK_KEY(X)
#endif

    INIT {
        struct program *prog = Pike_fp->current_object->prog;
	THIS->tree.root = NULL;
	THIS->encode_fun = find_identifier("encode_key", prog);
	THIS->decode_fun = find_identifier("decode_key", prog);
	THIS->copy_fun = find_identifier("copy", prog);
	THIS->insert_fun = find_identifier("`[]=", prog);

        if (THIS->copy_fun != -1 && identifier_is_native(prog, TREE_CLASSIFY(_program), THIS->copy_fun))
            THIS->copy_fun = -1;

        if (THIS->insert_fun != -1 && identifier_is_native(prog, TREE_CLASSIFY(_program), THIS->insert_fun))
            THIS->insert_fun = -1;
    }

static inline void TREE_CLASSIFY(_copy_node)(struct object *o, TREE_T *T, cb_node_t node) {
    if (T->insert_fun != -1) {
	CB_PUSH_TRANSFORM_KEY(node->key);
	CB_PUSH_VALUE(node->value);
	apply_low(o, T->insert_fun, 2);
	pop_stack();
    } else cb_insert(& T->tree, node->key, &node->value);
}

static inline void TREE_CLASSIFY(_copy_tree)(struct object *o, TREE_T *T, cb_node_t node) {
    if (T->insert_fun != -1) {
        int i = 0;
        WALK_FORWARD(node, {
            if (CB_HAS_VALUE(_)) {
                cb_node_t n = _; 
                i++;
                CB_PUSH_TRANSFORM_KEY(n->key);
                push_svalue(&n->value);
                apply_low(o, T->insert_fun, 2);
                pop_stack();
            }
        });
    } else cb_copy_tree(& T->tree, node);
}

static inline struct object *
TREE_CLASSIFY(_clone_object)(struct object *o) {
    struct object * t = NULL;
    if (THIS->copy_fun != -1) {
	apply_low(o, THIS->copy_fun, 0);
	if (TYPEOF(Pike_sp[-1]) != PIKE_T_OBJECT)
	    Pike_error("clone() is supposed to return an object.\n");
	t = Pike_sp[-1].u.object;
	add_ref(t);
	pop_stack();
    } else {
	t = clone_object(Pike_fp->current_object->prog, 0);
	cb_copy_tree(& GET_TREE_STORAGE(t)->tree, GET_TREE_STORAGE(o)->tree.root);
    }
    return t;
}

cmod_DEFINE_EVAL(CLONE_OBJECT, TREE_CLASSIFY(_clone_object))

cmod_DEFINE_EVAL(COPY_TREE, TREE_CLASSIFY(_copy_tree))

cmod_DEFINE_EVAL(COPY_NODE, TREE_CLASSIFY(_copy_node))

    DOCSTART() @decl mixed _m_delete(mixed key)
     *! m_delete callback.
    DOCEND()
    PIKEFUN mixed _m_delete(mixed key)
	flags ID_PROTECTED;
    {
	size_t size, oldsize;
	cb_key k;

	k = CB_KEY_FROM_SVALUE(key);

	oldsize = (THIS->tree.root) ? THIS->tree.root->size : 0;
	if (oldsize) {
	    /* NB: cb_delete() may run pike code (like eg when
	     *     comparing bignums). The stack must thus be
	     *     consistent in case eg a gc() is triggered.
	     */
	    push_undefined();
	    cb_delete(& THIS->tree, k, Pike_sp-1);
	    size = (THIS->tree.root) ? THIS->tree.root->size : 0;
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
	push_int(cb_rec_check_parents(THIS->tree.root));
    }

    DOCSTART() @decl void create(array|mapping|void o)
     *! Create a tree_class from o.
    DOCEND()
    PIKEFUN void create (array|mapping|void o)
	flags ID_PROTECTED;
    {
	if (o) {
	  switch (TYPEOF(*o)) {
	    case PIKE_T_MAPPING: {
		struct keypair *k;
		INT32 e;
		struct mapping_data *md = o->u.mapping->data;

		NEW_MAPPING_LOOP(md) {
		    cb_key key = CB_KEY_FROM_SVALUE(&k->ind);
		    cb_insert(& THIS->tree, key, &k->val);
                }
                pop_stack();
		return;
	    }
	    case PIKE_T_ARRAY: {
		INT32 i;

		if (o->u.array->size & 1) goto ARG_ERROR;

		for (i = 0; i < o->u.array->size; i+=2) {
		    cb_key key = CB_KEY_FROM_SVALUE(ITEM(o->u.array)+i);
		    cb_insert(& THIS->tree, key, ITEM(o->u.array)+i+1);
                }
                pop_stack();
		return;
	    }
	  }
ARG_ERROR:
	  SIMPLE_ARG_TYPE_ERROR("create", 1, "mapping(" cmod_STRFY_EVAL(key_ptype) ":mixed)|array");
	}
    }

DOCSTART() @decl int _sizeof()
 *! Gives the number of entries in the @[tree_class].
DOCEND()
    PIKEFUN int _sizeof()
	flags ID_PROTECTED;
    {
	push_int(THIS->tree.root ? THIS->tree.root->size : 0);
    }

DOCSTART() @decl array _indices()
 *! Returns a sorted array of indices of the @[tree_class].
DOCEND()
    PIKEFUN array _indices()
	flags ID_PROTECTED;
    {
	size_t size = THIS->tree.root ? THIS->tree.root->size : 0;
	if (size) {
	    size_t start = 0;
	    struct svalue * s;
	    cb_key k;
	    cb_node_t node = THIS->tree.root;
	    struct array * a = allocate_array_no_init(size, 0);
	    push_array(a);
	    if (CB_HAS_VALUE(node)) {
		s = ITEM(a)+start;
		SET_SVAL_TYPE(*s, T_VOID);
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
		    SET_SVAL_TYPE(*s, T_VOID);
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
    PIKEFUN array _values()
	flags ID_PROTECTED;
    {
	size_t size = THIS->tree.root ? THIS->tree.root->size : 0;
	if (size) {
	    struct array * a = allocate_array_no_init(size, 0);
	    push_array(a);
	    cb_aggregate_values(THIS->tree.root, a, 0, size);
	    return;
	}
	ref_push_array(&empty_array);
    }

DOCSTART() @decl mixed `[]=(mixed key, mixed val, @
 *!                         mixed|void context, int|void access)
DOCEND()
    PIKEFUN mixed `[]=(mixed key, mixed val,
		       mixed|void context, int|void access)
	flags ID_PROTECTED;
    {
	cb_key k = CB_KEY_FROM_SVALUE(key);
	cb_insert(& THIS->tree, k, val);
#ifdef DEBUG_CHECK
	cb_check(THIS->tree.root, THIS->tree.root, k->str);
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
	cb_insert(& THIS->tree, k, val);
	stack_pop_keep_top();
    }

    DOCSTART() @decl mapping cast(string type)
     *! Cast callback. Supports only cast to mapping and
     *! behaves as the inverse of create().
    DOCEND()
    PIKEFUN mapping cast(string type)
	flags ID_PROTECTED;
    {
      pop_stack(); /* type as at least one more reference. */

      if (type == literal_mapping_string) {
        cmod_CONCAT_EVAL(f_, tree_class, _cq__indices)(0);
        cmod_CONCAT_EVAL(f_, tree_class, _cq__values)(0);
        f_mkmapping(2);
      }
      else
        push_undefined();
    }

    DOCSTART() @decl mixed nth(int(0..) n)
     *! Get the @expr{n@}th entry in order.
     *! @returns
     *! 	An array @expr{({ key, value })@}.
    DOCEND()
    PIKEFUN mixed nth(int n) {
	cb_node_t node;

	if (THIS->tree.root) {
	    if (n >= 0 && (size_t)n < THIS->tree.root->size) {
		node = cb_get_nth(THIS->tree.root, n);
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

    DOCSTART() @decl array _random(function random_string, function random)
     *! Get a random entry.
     *! @returns
     *! 	An array @expr{({ key, value })@}.
    DOCEND()
    PIKEFUN array _random(function random_string, function random)
	flags ID_PROTECTED;
    {
        cb_node_t node;

        if (THIS->tree.root) {
            push_int(THIS->tree.root->size);
            apply_svalue(random, 1);
            if( TYPEOF(Pike_sp[-1])!=T_INT ||
		Pike_sp[-1].u.integer < 0 ||
                (size_t)Pike_sp[-1].u.integer >= THIS->tree.root->size )
              Pike_error("Wrong return type from random.\n");

            node = cb_get_nth(THIS->tree.root, Pike_sp[-1].u.integer);
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

    DOCSTART() @decl mixed `[](mixed key)
    DOCEND()
    PIKEFUN mixed `[](mixed key)
	flags ID_PROTECTED;
    {
	CB_TRANSFORM_KEY(key);
	if ((1 << TYPEOF(*key)) & T_KEY) {
	    cb_node_t n;
	    cb_key k = CB_LOW_KEY_FROM_SVALUE(key);
	    pop_stack();
	    n = cb_index(THIS->tree.root, k);

	    if (n && CB_HAS_VALUE(n)) {
		push_svalue(&n->value);
		return;
	    }
	} else pop_stack();
	push_undefined();
    }

    DOCSTART() @decl int(0..1) _equal(mixed o)
    DOCEND()
    PIKEFUN int _equal(mixed o)
	flags ID_PROTECTED;
    {
	if (TYPEOF(*o) == PIKE_T_OBJECT
	    && o->u.object->prog == Pike_fp->current_object->prog) {
	    cb_node_t tree1, tree2;
	    tree1 = THIS->tree.root;
	    tree2 = GET_TREE_STORAGE(o->u.object)->tree.root;

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

    DOCSTART() @decl mixed `-(mixed o)
     *! Sub[s]tract two trees from each other (key-wise).
    DOCEND()
    PIKEFUN mixed `-(mixed o)
	flags ID_PROTECTED;
    {
	struct object * res;
	cb_node_t node, tree = NULL;

        TREE_T *T;

	if (TYPEOF(*o) != PIKE_T_OBJECT || !(T = GET_TREE_STORAGE(o->u.object))) {
	    SIMPLE_ARG_TYPE_ERROR("`-", 1, "CritBit." cmod_STRFY_EVAL(tree_class));
	}

	node = T->tree.root;

	if (THIS->tree.root) {
	    if (node) {
		cb_node_t t = THIS->tree.root;
                TREE_T *dst;
		res = clone_object(Pike_fp->current_object->prog, 0);
                dst = GET_TREE_STORAGE(res);

		if (node != THIS->tree.root)
		    WALK_FORWARD(t, {
			if (CB_HAS_VALUE(_) && !cb_index(node, _->key))
			    COPY_NODE(res, dst, _);
		    });
	    } else res = CLONE_OBJECT(Pike_fp->current_object);
	} else res = clone_object(Pike_fp->current_object->prog, 0);

	push_object(res);
	return;
    }

    DOCSTART() @decl mixed `+(mixed o)
     *!	Add callback. Returns the union of two trees.
    DOCEND()
    PIKEFUN mixed `+(mixed o)
	flags ID_PROTECTED;
    {
	cb_node_t tree1, tree2;
        TREE_T *T;

	if (TYPEOF(*o) != PIKE_T_OBJECT || !(T = GET_TREE_STORAGE(o->u.object))) {
	    SIMPLE_ARG_TYPE_ERROR("`+", 1, "inherits(CritBit." cmod_STRFY_EVAL(tree_class) ")");
	}
	tree1 = THIS->tree.root;
	tree2 = T->tree.root;

	if (tree1 && tree2) {
	    struct object * res;
            TREE_T *dst;
	    if (tree1->size < tree2->size) {
		tree1 = tree2;
		tree2 = THIS->tree.root;
		res = CLONE_OBJECT(o->u.object);
	    } else
		res = CLONE_OBJECT(Pike_fp->current_object);

            dst = GET_TREE_STORAGE(res);

	    if (tree1 != tree2) {
		if (CB_HAS_VALUE(tree2)) COPY_NODE(res, dst, tree2);
		WALK_FORWARD(tree2, {
		    if (CB_HAS_VALUE(_)) COPY_NODE(res, dst, _);
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

    DOCSTART() @decl mixed `[..](mixed a, int atype, mixed b, int btype)
     *! @seealso
     *! 	@[predef::`[..]]
    DOCEND()
    PIKEFUN mixed `[..](mixed a, int atype, mixed b, int btype)
	flags ID_PROTECTED;
    {
	struct object * o = NULL;

	if (THIS->tree.root) {
            TREE_T *dst;
	    cb_node_t start = NULL, stop = NULL;

	    switch (atype & (INDEX_FROM_BEG | INDEX_FROM_END | OPEN_BOUND)) {
	    case OPEN_BOUND:
		start = cb_find_first(THIS->tree.root);
		break;
	    case INDEX_FROM_BEG:
		start = cb_find_ne(THIS->tree.root, CB_KEY_FROM_SVALUE(a));
		break;
	    case INDEX_FROM_END:
		goto illegal_index;
	    }

	    if (!start) goto range_do_clone;

	    switch (btype & (INDEX_FROM_BEG | INDEX_FROM_END | OPEN_BOUND)) {
	    case OPEN_BOUND:
		stop = cb_find_last(THIS->tree.root);
		break;
	    case INDEX_FROM_BEG:
		stop = cb_find_le(THIS->tree.root, CB_KEY_FROM_SVALUE(b));
		break;
	    case INDEX_FROM_END:
		goto illegal_index;
	    }

	    o = clone_object(Pike_fp->current_object->prog, 0);
            dst = GET_TREE_STORAGE(o);

	    if (!stop) goto range_do_clone;

	    if (start == stop) {
		COPY_NODE(o, dst, start);
		goto range_do_clone;
	    }

	    if (CB_KEY_LT(start->key, stop->key)) {
		COPY_NODE(o, dst, start);
		WALK_FORWARD(start, {
		    if (CB_HAS_VALUE(_))
			COPY_NODE(o, dst, _);
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
    PIKEFUN mixed `[](mixed a, mixed b)
	flags ID_PROTECTED;
    {
	struct object * o = NULL;

	if (THIS->tree.root) {
            TREE_T *dst;
	    cb_node_t start = NULL, stop = NULL;

	    if (TYPEOF(*a) == PIKE_T_INT && !a->u.integer) {
		start = cb_find_first(THIS->tree.root);
	    } else {
		cb_key key = CB_KEY_FROM_SVALUE(a);
		start = cb_find_ne(THIS->tree.root, key);
	    }

	    if (!start) goto range_do_clone;

	    if (TYPEOF(*b) == PIKE_T_INT && b->u.integer == MAX_INT_TYPE) {
		stop = cb_find_last(THIS->tree.root);
	    } else {
		cb_key key = CB_KEY_FROM_SVALUE(b);
		stop = cb_find_le(THIS->tree.root, key);
	    }

	    o = clone_object(Pike_fp->current_object->prog, 0);
            dst = GET_TREE_STORAGE(o);

	    if (!stop) goto range_do_clone;

	    if (start == stop) {
		COPY_NODE(o, dst, start);
		goto range_do_clone;
	    }

	    if (CB_KEY_LT(start->key, stop->key)) {
		COPY_NODE(o, dst, start);
		WALK_FORWARD(start, {
		    if (CB_HAS_VALUE(_))
			COPY_NODE(o, dst, _);
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
	if (THIS->tree.root) {
	    struct string_builder s;

	    init_string_builder(&s, 0);
	    cb_print_tree(&s, THIS->tree.root, 0);
	    push_string(finish_string_builder(&s));
	    return;
	}
	push_static_text("");
    }

    DOCSTART() @decl key_ptype first()
     *! Get the lexicographically first index in the tree.
    DOCEND()
    PIKEFUN key_ptype first() {
	if (THIS->tree.root) {
	    cb_node_t first = cb_find_first(THIS->tree.root);
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
	if (THIS->tree.root) {
	    cb_node_t last = cb_find_last(THIS->tree.root);
	    if (last) {
		CB_PUSH_TRANSFORM_KEY(last->key);
		return;
	    }
	}
	push_undefined();
    }

    DOCSTART() @decl string bkey(mixed key)
     *! Render the internally used binary representation of
     *! the key into a string as a strings of '0's and '1's.
    DOCEND()
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

	if (THIS->tree.root) {
	    cb_node_t previous;

	    previous = cb_find_previous(THIS->tree.root,
					CB_LOW_KEY_FROM_SVALUE(current));
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

	if (THIS->tree.root) {
	    cb_node_t next;

	    next = cb_find_next(THIS->tree.root,
				CB_LOW_KEY_FROM_SVALUE(current));
	    pop_stack();
	    if (next) {
		CB_PUSH_TRANSFORM_KEY(next->key);
		return;
	    }
	}
	push_undefined();
    }

    DOCSTART() @decl int(0..) depth()
     *! Calculate the depth of the tree.
    DOCEND()
    PIKEFUN int depth() {
	if (THIS->tree.root) {
	    push_int(cb_get_depth(THIS->tree.root));
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
	cb_copy_tree(& GET_TREE_STORAGE(copy)->tree, THIS->tree.root);
	push_object(copy);
    }

    DOCSTART() @decl tree_class get_subtree(void|mixed key)
     *! Get a copy of the subtree starting at prefix
     *! @expr{key@}.
    DOCEND()
    PIKEFUN object get_subtree(void|mixed key) {
	struct object * sub;
	cb_node_t node = NULL;
        TREE_T *dst;

	if (THIS->tree.root) {
	    if (key == NULL || IS_UNDEFINED(key))
		node = THIS->tree.root;
	    else
		node = cb_subtree_prefix(THIS->tree.root,
					 CB_LOW_KEY_FROM_SVALUE(key));
	}
	pop_n_elems(args);

	sub = clone_object(Pike_fp->current_object->prog, 0);
        dst = GET_TREE_STORAGE(sub);

	if (node) {
	    COPY_TREE(sub, dst, node);
	}

	push_object(sub);
    }

    EXIT {
	if (THIS->tree.root) {
	    cb_free_node(& THIS->tree, THIS->tree.root);
	    THIS->tree.root = NULL;
	}
        THIS->rev++;
    }

    /* keys are always non-complex, so we dont have to check them with the gc */
    GC_CHECK {
	cb_node_t node = THIS->tree.root;
	if (!node) return;
	WALK_FORWARD(node, {
	    if (CB_HAS_VALUE(_)) {
		CB_GC_CHECK_KEY(_->key);
		debug_gc_check_svalues(&_->value, 1,  " as a CritBit node value");
	    }
	});
    }

    GC_RECURSE {
	cb_node_t node = THIS->tree.root;
	if (!node) return;
	WALK_FORWARD(node, {
	    if (CB_HAS_VALUE(_)) {
		CB_GC_RECURSE_KEY(_->key);
		gc_recurse_svalues(&_->value, 1);
	    }
	});
    }

DOCSTART() @endclass DOCEND()
