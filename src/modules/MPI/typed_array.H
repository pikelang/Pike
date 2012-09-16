#cmod_ifdef ARRAY_CLASSIFY
# cmod_undef ARRAY_CLASSIFY
#cmod_endif
#cmod_define ARRAY_CLASSIFY(x...) cmod_CONCAT_EVAL(name_prefix, ##x)

    CVAR struct mpi_info i;

/*! @decl void create(int(0..) size) */
    PIKEFUN void create(int(0..) size)
    flags ID_PROTECTED;
    {
	if (THIS->i.data) Pike_error("create() called twice.\n");
	THIS->i.length = size;
	if (size) THIS->i.data = xalloc(size * THIS->i.type_size);
	/* memset(THIS->p, 0, size * sizeof(*THIS->p)); */
	/* NOTE: This may point to formerly used memory and give access to
	 * the (old) content of that memory.
	 * Make sure to never hand out "partially filled" IntArrays to users
	 * in full.
	 */
    }

DOCSTART() @decl void clear()
 *! Sets all elements of the ARRAY_CLASSIFY(Array) to 0.
DOCEND()
    PIKEFUN void clear()
    {
	if (!THIS->i.length) return;
	memset(THIS->i.data, 0, THIS->i.length * THIS->i.type_size);
    }

DOCSTART() @decl void assign(ARRAY_CLASSIFY(Array) other)
 *! Assigns the first @expr{sizeof(other)@} entries from @expr{other@} to the
 *! respective entries in the current array.
 *!
 *! @throws
 *! 	If @expr{sizeof(other) > sizeof(this)@}
 DOCEND()
    PIKEFUN void assign(object other)
    {
	if (!other || -1 == low_get_storage(other->prog, ARRAY_CLASSIFY(Array_program)))
	    SIMPLE_BAD_ARG_ERROR("`+", 1, "inherits(MPI." cmod_STRFY_EVAL(name_prefix) "Array)");
	if (THIS->i.length < ARRAY_STORAGE(other)->i.length)
	    Pike_error("Cannot assign from MPI." cmod_STRFY_EVAL(name_prefix) "Array of size %d to " cmod_STRFY_EVAL(name_prefix) "Array of size %d.\n",
		       ARRAY_STORAGE(other)->i.length, THIS->i.length);

	memcpy(THIS->i.data, ARRAY_STORAGE(other)->i.data, ARRAY_STORAGE(other)->i.length * THIS->i.type_size);
	pop_n_elems(args);
    }

DOCSTART() @decl pike_type `[](int idx)
 *! Gives the @expr{idx@}ths element of the ARRAY_CLASSIFY(Array).
DOCEND()
    PIKEFUN pike_type `[](int idx)
    {
	c_type r;

	if ((unsigned int)idx >= THIS->i.length || idx < 0)
	    Pike_error("Invalid index %d in MPI." cmod_STRFY_EVAL(name_prefix) "Array of size %u.\n",
		       idx, THIS->i.length);

	r = ((c_type*)THIS->i.data)[idx];
	pop_n_elems(args);
	PUSH(r);
    }

DOCSTART() @decl pike_type `[]=(int idx, pike_type val)
 *! Sets the @expr{idx@}ths element of the ARRAY_CLASSIFY(Array) to
 *! @expr{val@}.
DOCEND()
    PIKEFUN pike_type `[]=(int idx, pike_type val)
    {
	if ((unsigned int)idx >= THIS->i.length || idx < 0)
	    Pike_error("Invalid index %d in MPI." cmod_STRFY_EVAL(name_prefix) "Array of size %u.\n",
		       idx, THIS->i.length);

	((c_type*)THIS->i.data)[idx] = val;
	stack_swap();
	pop_n_elems(1);
    }

/* Why did i put this in here? sum()? seriously? */
    PIKEFUN pike_type sum()
    {
	unsigned int i;
	c_type r = 0;

	if (!THIS->i.length) {
	    PUSH(0);
	    return;
	}

	for (i = 0; i < THIS->i.length; i++) {
	    r += ((c_type*)THIS->i.data)[i];
	}

	PUSH(r);
    }

DOCSTART() @decl int _sizeof()
 *! @returns
 *! 	The number of entries in this ARRAY_CLASSIFY(Array).
DOCEND()
    PIKEFUN int _sizeof()
    {
	push_int(THIS->i.length);
    }

DOCSTART() @decl array(pike_type) _values()
 *! @returns
 *! 	A Pike array copy of this array.
DOCEND()
    PIKEFUN array _values()
    flags ID_PROTECTED;
    {
	struct array *a;
	unsigned int i;
	struct svalue sv;

	push_int(THIS->i.length);
	f_allocate(1);

	a = Pike_sp[-1].u.array;

	for (i = 0; i < THIS->i.length; i++) {
	    SET_SVAL((*(ITEM(a)+i)), CPIKE_TYPE, CPIKE_SUBTYPE, U_IND,
		     ((c_type*)THIS->i.data)[i]);
	}
    }

DOCSTART() @decl mixed cast(string type)
 *! Supports casting (by copy) this ARRAY_CLASSIFY(Array) to a Pike array.
 *!
 *! @example
 *! array pike_array = (array)typed_array;
DOCEND()
    PIKEFUN mixed cast(string type)
    flags ID_PROTECTED;
    {
	if (type == MK_STRING("array")) {
	    pop_n_elems(args);
	    apply_current(cmod_CONCAT_EVAL(f_, ARRAY_CLASSIFY(Array_cq__values_fun_num)), 0);
	} else if (type == MK_STRING("object")) {
	    pop_n_elems(args);
	    ref_push_object(Pike_fp->current_object);
	} else
	    Pike_error("Cannot cast to %o.\n", Pike_sp-1);
    }

    INIT
    {
	THIS->i.type = MPI_DATATYPE;
	THIS->i.type_size = sizeof(c_type);
	THIS->i.shift = MPI_SHIFT;
    }

    EXIT
    {
	if (THIS->i.data) {
	    if (!THIS->i.clone) free(THIS->i.data);
	    THIS->i.data = NULL;
	    THIS->i.length = 0;
	}
    }

#cmod_undef pike_type
#cmod_undef c_type
#cmod_undef name_prefix
#cmod_undef ARRAY_STORAGE
#cmod_undef PUSH
#cmod_undef MPI_DATATYPE
#cmod_undef MPI_SHIFT
#cmod_undef CPIKE_TYPE
#cmod_undef CPIKE_SUBTYPE
#cmod_undef U_IND
