#cmod_ifdef ARRAY_CLASSIFY
# cmod_undef ARRAY_CLASSIFY
#cmod_endif
#cmod_define ARRAY_CLASSIFY(x...) cmod_CONCAT_EVAL(name_prefix, ##x)
    CVAR c_type *p;
    CVAR unsigned int size;

    PIKEFUN void create(int size)
    flags ID_PROTECTED;
    {
	if (THIS->p) Pike_error("create() called twice.\n");
	THIS->size = size;
	THIS->p = malloc(size * sizeof(*THIS->p));
	/* memset(THIS->p, 0, size * sizeof(*THIS->p)); */
	/* NOTE: This may point to formerly used memory and give access to
	 * the (old) content of that memory.
	 * Make sure to never hand out "partially filled" IntArrays to users
	 * in full.
	 */
    }

    PIKEFUN void clear()
    {
	if (!THIS->p) Pike_error("Nothing to clear.\n");
	memset(THIS->p, 0, THIS->size * sizeof(*THIS->p));
    }

    PIKEFUN void assign(object other)
    {
	if (!other || -1 == low_get_storage(other->prog, ARRAY_CLASSIFY(Array_program)))
	    SIMPLE_BAD_ARG_ERROR("`+", 1, "inherits(MPI." cmod_STRFY_EVAL(name_prefix) "Array)");
	if (THIS->size < ARRAY_STORAGE(other)->size)
	    Pike_error("Cannot assign from MPI." cmod_STRFY_EVAL(name_prefix) "Array of size %d to " cmod_STRFY_EVAL(name_prefix) "Array of size %d.\n",
		       ARRAY_STORAGE(other)->size, THIS->size);
	if (!THIS->p)
	    Pike_error("Nothing to assign to.\n");
	if (!ARRAY_STORAGE(other)->p)
	    Pike_error("Nothing to assign from.\n");

	memcpy(THIS->p, ARRAY_STORAGE(other)->p, ARRAY_STORAGE(other)->size * sizeof(*THIS->p));
	pop_n_elems(args);
    }

    PIKEFUN pike_type `[](int idx)
    {
	c_type r;

	if (!THIS->p) Pike_error("Nothing to index.\n");

	if (idx >= THIS->size || idx < 0)
	    Pike_error("Invalid index %d in MPI." cmod_STRFY_EVAL(name_prefix) "Array of size %d.\n",
		       idx, THIS->size);
	r = THIS->p[idx];
	pop_n_elems(args);
	PUSH(r);
    }

    PIKEFUN pike_type `[]=(int idx, pike_type val)
    {
	if (!THIS->p) Pike_error("Nothing to index.\n");

	if (idx >= THIS->size || idx < 0)
	    Pike_error("Invalid index %d in MPI." cmod_STRFY_EVAL(name_prefix) "Array of size %d.\n",
		       idx, THIS->size);

	THIS->p[idx] = val;
	stack_swap();
	pop_n_elems(1);
    }

    PIKEFUN pike_type sum()
    {
	unsigned int i;
	c_type r = 0;

	if (!THIS->p) Pike_error("Nothing to sum.\n");

	for (i = 0; i < THIS->size; i++) {
	    r += THIS->p[i];
	}

	PUSH(r);
    }

    PIKEFUN int _sizeof()
    {
	push_int(THIS->size);
    }

    EXIT
    {
	if (THIS->p) {
	    free(THIS->p);
	    THIS->p = NULL;
	    THIS->size = 0;
	}
    }
