/* -*- mode: C; c-basic-offset: 4; -*-
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "module.h"

#include "interpret.h"
#include "pike_macros.h"
#include "pike_types.h"
#include "pike_error.h"
#include "builtin_functions.h"
#include "module_support.h"
#include "bignum.h"
#include "operators.h"
#include "gc.h"
#include <stdio.h>

extern void pike_init_inttree_module();
extern void pike_init_bignumtree_module();
extern void pike_init_tree_module();
extern void pike_init_floattree_module();

extern void pike_exit_inttree_module();
extern void pike_exit_bignumtree_module();
extern void pike_exit_tree_module();
extern void pike_exit_floattree_module();

PIKE_MODULE_INIT {
    pike_init_inttree_module();
    pike_init_tree_module();
    pike_init_floattree_module();
    pike_init_bignumtree_module();
}

PIKE_MODULE_EXIT {
    pike_exit_inttree_module();
    pike_exit_tree_module();
    pike_exit_floattree_module();
    pike_exit_bignumtree_module();
}
