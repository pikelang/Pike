    DOCSTART() @decl inherit Filter
    DOCEND()
    INHERIT Filter;

    DOCSTART() @decl array(int) hash_value(INDEX_TYPE k)
            *! 
            *! Returns an array of hash values of @expr{k@}. This is for debugging purposes.
    DOCEND()
    PIKEFUN array(int) hash_value(INDEX_TYPE k) {
        unsigned INT32 i, n = THIS->hashes, shift = THIS->shift;
        struct hash_data d;
        HASH_FUNCTION(k, &d);
        pop_stack();

#ifdef BLOCKED
        push_int(get_hash(&d, shift - CACHE_SHIFT));
        for (i = 1; i < n; i++) {
            push_int(get_hash(&d, CACHE_SHIFT));
        }
#else
        for (i = 0; i < n; i++) {
            push_int(get_hash(&d, shift));
        }
#endif

        f_aggregate(n);
    }

    DOCSTART() @decl int(0..1) `[](INDEX_TYPE k)
            *! 
            *! If @expr{k@} is in the set represented by this bloom filter, @expr{1@} is returned.
            *! If @expr{0@} is returned, @expr{k@} is not an element of the set.
            *! 
    DOCEND()
    PIKEFUN mixed `[](INDEX_TYPE k) {
        struct hash_data d;
        HASH_FUNCTION(k, &d);
        pop_stack();
        push_int(test_hashes(THIS, &d));
    }

    DOCSTART() @decl int(0..1) `[]=(INDEX_TYPE k, int(1..1) v)
            *! 
            *! Add an element @expr{k@}.
            *! 
    DOCEND()
    PIKEFUN mixed `[]=(INDEX_TYPE k, mixed v) {
        struct hash_data d;

        if (TYPEOF(*v) != PIKE_T_INT || SUBTYPEOF(*v) != NUMBER_NUMBER || v->u.integer != 1) {
            SIMPLE_ARG_TYPE_ERROR("`[]=", 1, "int(1..1)");
        }
        HASH_FUNCTION(k, &d);
        set_hashes(THIS, &d);
        stack_pop_keep_top();
    }

    DOCSTART() @decl void add(INDEX_TYPE ... args)
            *! Add elements.
    DOCEND()
    PIKEFUN void add(mixed ... a) {
        struct Filter_struct *this = THIS;
        struct hash_data d;
        INT32 i;

        for (i = 0; i < args; i++) {
            if (!SVALUE_IS_KEY(a+i))
                SIMPLE_ARG_TYPE_ERROR("add", i, cmod_STRFY_EVAL(INDEX_TYPE));
        }

        for (i = 0; i < args; i++) {
            HASH_FUNCTION(SVALUE_TO_KEY(a+i), &d);
            set_hashes(this, &d);
        }

        pop_n_elems(args);
    }

    DOCSTART() @decl void append(array(INDEX_TYPE)|mapping(INDEX_TYPE:mixed) a)
            *! Add all elements of an array or all indices of a mapping to the bloom filter.
    DOCEND()
    PIKEFUN void append(array|mapping arg) {
        struct Filter_struct *this = THIS;
        struct hash_data d;
        if (TYPEOF(*arg) == PIKE_T_ARRAY) {
            const struct array *a = arg->u.array;
            const struct svalue *v = ITEM(a);
            INT32 i, size = a->size;

            for (i = 0; i < size; i++) if (!SVALUE_IS_KEY(v+i)) goto bad_argument;

            for (i = 0; i < size; i++) {
                HASH_FUNCTION(SVALUE_TO_KEY(v+i), &d);
                set_hashes(this, &d);
            }
        } else if (TYPEOF(*arg) == PIKE_T_MAPPING) {
            const struct mapping *m = arg->u.mapping;
            const struct keypair *k;
            const struct mapping_data *md;
            INT32 e;

            md = m->data;

            NEW_MAPPING_LOOP(md) {
                if (!SVALUE_IS_KEY(&k->ind)) goto bad_argument;
            }

            NEW_MAPPING_LOOP(md) {
                const struct svalue *v = &k->ind;
                HASH_FUNCTION(SVALUE_TO_KEY(v), &d);
                set_hashes(this, &d);
            }
        } else {
bad_argument:
            SIMPLE_ARG_TYPE_ERROR("append", 1, "array(" cmod_STRFY_EVAL(INDEX_TYPE) ")|"
                                  "mapping(" cmod_STRFY_EVAL(INDEX_TYPE) ":mixed)");
        }
        pop_stack();
    }
