/* -*- mode: C; c-basic-offset: 4; -*- */
    DOCSTART() @decl key_ptype|int common_prefix()
     *! Returns the common prefix of all keys.
     *! If the tree has no elements, @[UNDEFINED] is returned.
    DOCEND()
    PIKEFUN key_ptype|int common_prefix() {
	if (!THIS->tree.root) {
	    push_undefined();
	    return;
	}
	CB_ASSIGN_SVALUE_KEY(THIS->tree.root->key, Pike_sp++);
    }
