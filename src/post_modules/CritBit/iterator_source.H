    DOCSTART() @class _get_iterator
     *! Iterator class for tree_class trees. Supports iterating over ranges
     *! with arbitrary stepping and direction.
     *!
     *! This is used by default when calling @expr{foreach@} on an object of
     *! tree_class. In @expr{foreach@} the iterator runs over all elements
     *! from the first to the last.
     *!
     *! @seealso
     *! 	@[predef::Iterator] for a description of the interface.
    DOCEND()
    PIKECLASS _get_iterator
	program_flags PROGRAM_USES_PARENT;
    {
	CVAR cb_node_t lastnode, tree;
	CVAR cb_key lastkey;
	CVAR cb_value lastval;
	CVAR size_t * revv, lastrev;
	CVAR INT_TYPE step;
	CVAR cb_key stop;


	static struct object * TREE_CLASSIFY(_get_iterator_find_parent)()
	{
	  struct external_variable_context loc;

	  loc.o = Pike_fp->current_object;
	  loc.parent_identifier = Pike_fp->fun;
	  loc.inherit = Pike_fp->context;
	  find_external_context(&loc, 1);
	  return loc.o;
	}

	INIT {
	    THIS->lastval.type = T_VOID;
	    THIS->lastnode = NULL;
	    THIS->step = 1;
	    THIS->stop.len.chars = 0;
	    THIS->stop.len.bits = 0;
	    THIS->stop.str = (cb_string)0;
	}

	EXIT {
	    if (THIS->stop.len.chars || THIS->stop.len.bits || THIS->stop.str) {
		CB_FREE_KEY(THIS->stop);
	    }

	    if (THIS->lastnode) {
		CB_FREE_KEY(THIS->lastkey);
		CB_FREE_VALUE(&THIS->lastval);
	    }
	}

	GC_CHECK {
	    debug_gc_check_svalues(&THIS->lastval, 1, " as current CritBit iterator value");
	}

	GC_RECURSE {
	    gc_recurse_svalues(&THIS->lastval, 1);
	}

	static inline int cmod_CONCAT_EVAL(tree_class,_,iterator_class,_step)() {
	    cb_node_t t;
	    INT_TYPE c = THIS->step;

	    if (THIS->lastrev == *THIS->revv) {
		t = THIS->lastnode;
	    } else {
		THIS->lastrev = * THIS->revv;

		if (THIS->tree) {
		    t = cb_index(THIS->tree, THIS->lastkey);
		    if (t) {
			THIS->lastnode = t;
		    } else { /* the last node was removed and freed. */
			if (c > 0) {
			    t = cb_find_next(THIS->tree, THIS->lastkey);
			    c--;
			} else {
			    t = cb_find_previous(THIS->tree, THIS->lastkey);
			    c++;
			}
		    }
		} else {
		    t = NULL;
		}
	    }

	    if (THIS->lastnode) {
		CB_FREE_KEY(THIS->lastkey);
		CB_FREE_VALUE(&THIS->lastval);
	    }

	    if (t) {
		if (c > 0) {
		    WALK_FORWARD(t, {
			if (CB_HAS_VALUE(_)) c--;
			if (c == 0) break;
		    });
		} else if (c < 0) {
		    WALK_BACKWARD(t, {
			if (CB_HAS_VALUE(_)) c++;
			if (c == 0) break;
		    });
		    if (t == THIS->tree && !CB_HAS_VALUE(t)) t = NULL;
		}

		if (t) {
		    /* check for stop
		     * TODO: if we know that the lastnode is a parent
		     * of t, and stop was bigger than both children, then
		     * we could skip this check. */
		    if (THIS->stop.len.chars || THIS->stop.len.bits) {
			if (THIS->step < 0) {
			    if (CB_KEY_LT(t->key, THIS->stop)) t = NULL;
			} else {
			    if (CB_KEY_LT(THIS->stop, t->key)) t = NULL;
			}
		    }
		    if (t) {
			CB_GET_VALUE(t, &THIS->lastval);
			THIS->lastkey = t->key;
			CB_ADD_KEY_REF(THIS->lastkey);
		    }
		}
	    }
	    THIS->lastnode = t;
	    return !!t;
	}

	DOCSTART() @decl void create(void|int step, void|mixed start, void|mixed stop)
	 *! Returns an iterator object that runs from @expr{start@} to
	 *! @expr{stop@} using a stepsize of @expr{step@}. The arguments
	 *! default to @expr{1@}, @expr{tree->first()@} and
	 *! @expr{tree->last()@}, respectively.
	DOCEND()
	PIKEFUN void create(void|int step, void|mixed start,
			    void|mixed stop) {
	    cb_node_t t = NULL;
	    struct object * tree = TREE_CLASSIFY(_get_iterator_find_parent)();
	    INT32 encode_fun;
	    THIS->revv = &(cmod_OBJ2_TREE(tree)->rev);
	    THIS->tree = cmod_OBJ2_TREE(tree)->tree.root;

	    THIS->stop.len.chars = 0;
	    THIS->stop.len.bits = 0;

	    encode_fun = cmod_OBJ2_TREE(tree)->encode_fun;

	    if (THIS->tree) {
		if (!step || IS_UNDEFINED(step)) {
		    THIS->step = 1;
		} else {
		    THIS->step = step->u.integer;
		    if (THIS->step == 0)
			SIMPLE_BAD_ARG_ERROR("create", 1, "int(..-1)|int(1..)");
		}

		switch (args) {
		case 3:
		    if (!IS_UNDEFINED(stop)) {
			if (encode_fun >= 0) {
			    push_svalue(stop);
			    apply_low(tree, encode_fun, 1);
			    assign_svalue(stop, Pike_sp-1);
			    pop_stack();
			    if (!((1<<stop->type) & T_KEY))
				SIMPLE_BAD_ARG_ERROR("create", 3, cmod_STRFY_EVAL(key_ptype));
			}
			THIS->stop = CB_LOW_KEY_FROM_SVALUE(stop);
			CB_ADD_KEY_REF(THIS->stop);
		    }
		    /* FALL_THROUGH */
		case 2:
		    if (!IS_UNDEFINED(start)) {
			cb_key s;
			if (encode_fun >= 0) {
			    push_svalue(start);
			    apply_low(tree, encode_fun, 1);
			    assign_svalue(start, Pike_sp-1);
			    pop_stack();
			    if (!((1<<start->type) & T_KEY))
				SIMPLE_BAD_ARG_ERROR("create", 2, cmod_STRFY_EVAL(key_ptype));
			}
			s = CB_LOW_KEY_FROM_SVALUE(start);

			t = cb_index(THIS->tree, s);

			if (!t) {
			    t = (THIS->step < 0)
				? cb_find_previous(THIS->tree, s)
				: cb_find_next(THIS->tree, s);
			    if (t && (THIS->stop.len.chars || THIS->stop.len.bits)) {
				/* check if we ran over stop */
				if (THIS->step < 0) {
				    if (CB_KEY_LT(t->key, THIS->stop))
					t = NULL;
				} else {
				    if (CB_KEY_LT(THIS->stop, t->key))
					t = NULL;
				}
			    }
			}
		    }
		    /* FALL_THROUGH */
		case 1:
		case 0:
		    if (!t) t = (THIS->step < 0)
				? cb_find_last(THIS->tree)
				: cb_find_first(THIS->tree);
		    break;
		default:
		    Pike_error("Too many arguments.\n");
		}

		if (t) {
		    CB_GET_VALUE(t, &THIS->lastval);
		    THIS->lastkey = t->key;
		    CB_ADD_KEY_REF(THIS->lastkey);
		}
		THIS->lastnode = t;
	    }

	    pop_n_elems(args);
	}

	PIKEFUN object _get_iterator() {
	    ref_push_object(Pike_fp->current_object);
	}

	PIKEFUN int next() {
	    if(cmod_CONCAT_EVAL(tree_class,_,iterator_class,_step)()) {
		push_int(1);
		return;
	    }
	    push_undefined();
	    return;
	}

	PIKEFUN key_ptype index() {
	    if (THIS->lastnode) {
		struct object * tree = TREE_CLASSIFY(_get_iterator_find_parent)();
		INT32 fun = cmod_OBJ2_TREE(tree)->decode_fun;
		CB_PUSH_KEY(THIS->lastkey);
		if (fun >= 0)
		    apply_low(tree, fun, 1);
	    } else push_undefined();
	}

	PIKEFUN mixed value() {
	    if (THIS->lastnode) {
		CB_PUSH_VALUE(THIS->lastval);
		return;
	    }
	    push_undefined();
	}

	PIKEFUN int `!() {
	    push_int(!THIS->lastnode);
	}
    }
    /*! @endclass */
