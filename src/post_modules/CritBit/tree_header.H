DOCSTART() @class tree_class
 *! This class implements a CritBit-tree/trie that can be used as a
 *! mapping-like data structure. Values of @expr{key_ptype@} can be
 *! used as indices, while any possible type (also @expr{mixed@}) can
 *! be stored.
 *!
 *! CritBit trees are prefixed based search trees that allow for fast random
 *! access as well as prefix and range based lookups. Keys are stored in
 *! alphabetical order and can be iterated over using @expr{foreach@}.
 *! Other than that, it can be used like @expr{mapping(key_ptype:mixed)@}.
 *!
 *! @example
 *! 	@code
 *!ADT.CritBit.tree_class tree = ADT.CritBit.tree_class();
 *!key_ptype key1 = sample_key;
 *!tree[key1] = sample_value;
 *!tree[key1]; /cmod___SLASH__ now is sample_value
 *!m_delete(tree, key1); // tree is empty again
 *!@endcode
 *! @example
 *! 	@code
 *!ADT.CritBit.tree_class tree = ADT.CritBit.tree_class();
 *!array(key_ptype) a = sample_keys;
 *!foreach(a; int idx; key_ptype val) {
 *!	tree[val] = idx;
 *!}
 *!foreach(tree; key_ptype key; mixed val) {
 *!	/cmod___SLASH__ in here the keys will be reached in order ordered_keys.
 *!}
 *!@endcode
 *! @example
 *! 	@code
 *!ADT.CritBit.tree_class tree = ADT.CritBit.tree_class();
 *!array(key_ptype) a = sample_keys;
 *!foreach (a; int idx; key_ptype val) {
 *!	tree[val] = idx;
 *!}
 *!foreach(ADT.CritBit.TREE_CLASSIFY(.Iterator)(tree, -1); @
 *!key_ptype key; mixed val) {
 *!	/cmod___SLASH__ in here the keys will be reached in order reverse_ordered_keys.
 *!}
 *!@endcode
 *! @seealso
 *! 	@[ADT.CritBit.TREE_CLASSIFY(.Iterator)]
 DOCEND()

DOCSTART() @decl key_ptype encode_key(mixed o)
 *! @decl mixed decode_key(key_ptype o)
 *! These callbacks can be implemented when inheriting tree_class in order
 *! to allow for arbitrary key types. @[encode_key] is similar to the
 *! @[lfun::_hash()] callback. This only works as expected when it is possible
 *! to implement a unique representation for keys. These callbacks are called
 *! everytime a key is stored or indexed in the tree.
DOCEND()

    CVAR struct cb_tree tree;
    CVAR size_t rev;
    CVAR INT32 encode_fun;
    CVAR INT32 decode_fun;
    CVAR INT32 copy_fun;
    CVAR INT32 insert_fun;


#undef CB_TRANSFORM_KEY
#define CB_TRANSFORM_KEY(svalue) {				\
    if (THIS->encode_fun >= 0) {				\
	push_svalue(svalue);					\
	apply_low(Pike_fp->current_object, THIS->encode_fun, 1);\
	assign_svalue((svalue), Pike_sp-1);			\
	pop_stack();						\
    }								\
} while (0)
#undef CB_CHECK_KEY
#define CB_CHECK_KEY(svalue, fun, num)	do {			\
    CB_TRANSFORM_KEY(svalue);					\
    if (!((1<<TYPEOF(*svalue)) & T_KEY))   			\
	SIMPLE_BAD_ARG_ERROR(fun, (num), STRFY(key_ptype));	\
} while(0)
#undef CB_PUSH_TRANSFORM_KEY
#define CB_PUSH_TRANSFORM_KEY(key) do {				\
    CB_PUSH_KEY(key);						\
    if (THIS->decode_fun >= 0) {				\
	apply_low(Pike_fp->current_object, THIS->decode_fun, 1);\
    }								\
} while(0)


static inline cb_key
TREE_CLASSIFY(_transform_svalue_to_key)(const struct svalue * s) {
    cb_key key;
    if (THIS->encode_fun >= 0) {
	push_svalue(s);
	apply_low(Pike_fp->current_object, THIS->encode_fun, 1);
	if (!((1<<TYPEOF(Pike_sp[-1])) & T_KEY))
	    Pike_error("encode_key() is expected to return type " cmod_STRFY_EVAL(key_ptype) ".\n");
	key = CB_LOW_KEY_FROM_SVALUE(Pike_sp-1);
	pop_stack();
    } else {
      if (!((1<<TYPEOF(*s)) & T_KEY)) Pike_error("Expected type " cmod_STRFY_EVAL(key_ptype) ".\n");
	key = CB_LOW_KEY_FROM_SVALUE(s);
    }
    return key;
}
cmod_DEFINE_EVAL(CB_KEY_FROM_SVALUE, TREE_CLASSIFY(_transform_svalue_to_key))

static inline void TREE_CLASSIFY(_assign_svalue_key)(const cb_key key,
						  struct svalue * s) {
    if (THIS->decode_fun >= 0) {
	CB_PUSH_KEY(key);
	apply_low(Pike_fp->current_object, THIS->decode_fun, 1);
	assign_svalue((s), Pike_sp-1);
	pop_stack();
    } else {
	CB_LOW_ASSIGN_SVALUE_KEY(key, s);
    }
}
cmod_DEFINE_EVAL(CB_ASSIGN_SVALUE_KEY, TREE_CLASSIFY(_assign_svalue_key))
/*
 * GCC-BUG!!! we use the inline instead!
#undef CB_ASSIGN_SVALUE_KEY
#define CB_ASSIGN_SVALUE_KEY(key, s)	CB_LOW_ASSIGN_SVALUE_KEY(key, s)
#define CB_ASSIGN_SVALUE_KEY(_key, s) do {			\
    if (THIS->decode_fun >= 0) {				\
	CB_PUSH_KEY(_key);					\
	apply_low(Pike_fp->current_object, THIS->decode_fun, 1);\
	assign_svalue((s), Pike_sp-1);				\
	pop_stack();						\
    } else {							\
	CB_LOW_ASSIGN_SVALUE_KEY(_key, s);			\
    }								\
} while(0)
*/

