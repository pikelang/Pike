#include "global.h"
#include "module.h"

#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "array.h"
#include "pike_macros.h"
#include "program.h"
#include "stralloc.h"
#include "object.h"
#include "pike_types.h"
#include "pike_error.h"
#include "builtin_functions.h"
#include "module_support.h"
#include "bignum.h"
#include "operators.h"
#include "gc.h"
#include <stdio.h>

extern void pike_init_inttree_module();
#ifdef AUTO_BIGNUM
extern void pike_init_bignumtree_module();
#endif
extern void pike_init_tree_module();
extern void pike_init_floattree_module();

extern void pike_exit_inttree_module();
#ifdef AUTO_BIGNUM
extern void pike_exit_bignumtree_module();
#endif
extern void pike_exit_tree_module();
extern void pike_exit_floattree_module();

PIKE_MODULE_INIT {
    pike_init_inttree_module();
    pike_init_tree_module();
    pike_init_floattree_module();
#ifdef AUTO_BIGNUM
    pike_init_bignumtree_module();
#endif
}

PIKE_MODULE_EXIT {
    pike_exit_inttree_module();
    pike_exit_tree_module();
    pike_exit_floattree_module();
#ifdef AUTO_BIGNUM
    pike_exit_bignumtree_module();
#endif
}
