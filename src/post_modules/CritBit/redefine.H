/* -*- mode: C; c-basic-offset: 4; -*- */
cmod_REDEFINE(iterator_class)
cmod_REDEFINE(tree_class)
cmod_REDEFINE(cmod_OBJ2_TREE)
cmod_REDEFINE(T_KEY)
cmod_REDEFINE(key_ptype)
cmod_REDEFINE(sval_type)
cmod_REDEFINE(sample_key)
cmod_REDEFINE(sample_keys)
cmod_REDEFINE(ordered_keys)
cmod_REDEFINE(reverse_ordered_keys)
cmod_REDEFINE(sample_value)
cmod_REDEFINE(extra_doc)
#ifndef STRFY2
# define STRFY2(x)	#x
# ifdef STRFY
#  undef STRFY
# endif
# define STRFY(x)	STRFY2(x)
#endif
#cmod_ifdef TREE_CLASSIFY
# cmod_undef TREE_CLASSIFY
#cmod_endif
#cmod_define TREE_CLASSIFY(x...)	cmod_CONCAT_EVAL(tree_class, ##x)
