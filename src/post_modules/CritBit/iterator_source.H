/* -*- mode: C; c-basic-offset: 4; -*- */
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
	flags ID_PROTECTED;
    {
	CVAR cb_node_t lastnode, tree;
	CVAR cb_key lastkey;
	CVAR cb_value lastval;
	CVAR size_t * revv, lastrev;
	CVAR INT_TYPE step;
	CVAR cb_key start;
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
	    SET_SVAL_TYPE(THIS->lastval, T_VOID);
	    THIS->lastnode = NULL;
	    THIS->step = 1;
	    THIS->start.len.chars = 0;
	    THIS->start.len.bits = 0;
	    THIS->start.str = (cb_string)0;
	    THIS->stop.len.chars = 0;
	    THIS->stop.len.bits = 0;
	    THIS->stop.str = (cb_string)0;
	}

	EXIT {
	    if (THIS->start.len.chars || THIS->start.len.bits ||
		THIS->start.str) {
		CB_FREE_KEY(THIS->start);
	    }

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
                struct object * tree = TREE_CLASSIFY(_get_iterator_find_parent)();
                TREE_T *T;

                if (!tree->prog) return 0;

                T = GET_TREE_STORAGE(tree);

		THIS->lastrev = * THIS->revv;
                THIS->tree = T->tree.root;

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
			    void|mixed stop)
	  flags ID_PROTECTED;
	{
	    cb_node_t t = NULL;
	    struct object * tree = TREE_CLASSIFY(_get_iterator_find_parent)();
            TREE_T *T = GET_TREE_STORAGE(tree);
	    INT32 encode_fun;
	    THIS->revv = &(T->rev);
	    THIS->tree = T->tree.root;

	    THIS->start.len.chars = 0;
	    THIS->start.len.bits = 0;
	    THIS->start.str = 0;

	    THIS->stop.len.chars = 0;
	    THIS->stop.len.bits = 0;
	    THIS->stop.str = 0;

	    encode_fun = T->encode_fun;

	    if (THIS->tree) {
		if (!step) {
		    THIS->step = 1;
		} else {
		    THIS->step = step->u.integer;
		    if (THIS->step == 0)
			SIMPLE_ARG_TYPE_ERROR("create", 1, "int(..-1)|int(1..)");
		}

                if (stop) {
		    if (encode_fun >= 0) {
			push_svalue(stop);
			apply_low(tree, encode_fun, 1);
			assign_svalue(stop, Pike_sp-1);
			pop_stack();
		    }
#ifdef CB_FIX_SVALUE
		    CB_FIX_SVALUE(stop);
#endif
		    if (!((1<<TYPEOF(*stop)) & T_KEY))
			SIMPLE_ARG_TYPE_ERROR("create", 3, cmod_STRFY_EVAL(key_ptype));
		    THIS->stop = CB_LOW_KEY_FROM_SVALUE(stop);
		    CB_ADD_KEY_REF(THIS->stop);
                }

                if (start) {
		    if (encode_fun >= 0) {
			push_svalue(start);
			apply_low(tree, encode_fun, 1);
			assign_svalue(start, Pike_sp-1);
			pop_stack();
		    }
#ifdef CB_FIX_SVALUE
		    CB_FIX_SVALUE(start);
#endif
		    if (!((1<<TYPEOF(*start)) & T_KEY))
			SIMPLE_ARG_TYPE_ERROR("create", 2, cmod_STRFY_EVAL(key_ptype));
		    THIS->start = CB_LOW_KEY_FROM_SVALUE(start);
		    CB_ADD_KEY_REF(THIS->start);
                }

		THIS->lastnode = NULL;
            }
            pop_n_elems(args);
        }

	PIKEFUN object _get_iterator()
	  flags ID_PROTECTED;
	{
	    ref_push_object(Pike_fp->current_object);
	}

	PIKEFUN mixed _iterator_next()
	  flags ID_PROTECTED;
	{
	    struct object *tree = TREE_CLASSIFY(_get_iterator_find_parent)();
	    TREE_T *T = GET_TREE_STORAGE(tree);

	    if (!THIS->lastnode) {
		cb_node_t t = NULL;

		if (THIS->start.len.chars || THIS->start.len.bits || THIS->start.str) {
		    t = cb_index(THIS->tree, THIS->start);

		    if (!t) {
			if (THIS->step < 0) {
			    t = cb_find_previous(THIS->tree, THIS->start);
			} else {
			    t = cb_find_next(THIS->tree, THIS->start);
			}
		    }
		} else {
		    if (THIS->step < 0) {
			t = cb_find_last(THIS->tree);
		    } else {
			t = cb_find_first(THIS->tree);
		    }
		}

		if (t && (THIS->stop.len.chars || THIS->stop.len.bits || THIS->stop.str)) {
		    if (THIS->step < 0) {
			if (CB_KEY_LT(t->key, THIS->stop)) {
			    t = NULL;
			}
		    } else {
			if (CB_KEY_LT(THIS->stop, t->key)) {
			    t = NULL;
			}
		    }
		}

		if (!t) {
		    push_undefined();
		    return;
		}

		CB_GET_VALUE(t, &THIS->lastval);
		THIS->lastkey = t->key;
		CB_ADD_KEY_REF(THIS->lastkey);
		THIS->lastnode = t;
	    } else if (!cmod_CONCAT_EVAL(tree_class,_,iterator_class,_step)()) {
		push_undefined();
		return;
	    }

	    CB_PUSH_KEY(THIS->lastkey);
	    if (T->decode_fun >= 0) {
		apply_low(tree, T->decode_fun, 1);
	    }
	    return;
	}

	PIKEFUN key_ptype _iterator_index()
	  flags ID_PROTECTED;
	{
	    if (THIS->lastnode) {
		struct object * tree = TREE_CLASSIFY(_get_iterator_find_parent)();
                TREE_T *T = GET_TREE_STORAGE(tree);
		INT32 fun = T->decode_fun;
		CB_PUSH_KEY(THIS->lastkey);
		if (fun >= 0)
		    apply_low(tree, fun, 1);
	    } else push_undefined();
	}

	PIKEFUN mixed _iterator_value()
	  flags ID_PROTECTED;
	{
	    if (THIS->lastnode) {
		CB_PUSH_VALUE(THIS->lastval);
		return;
	    }
	    push_undefined();
	}
    }
    DOCSTART() @endclass DOCEND()
