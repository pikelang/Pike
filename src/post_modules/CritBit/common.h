/* -*- mode: C; c-basic-offset: 4; -*- */
#ifdef __SSE4_2__
# include <smmintrin.h>
#endif
#include "module.h"
#include "cyclic.h"
#include "interpret.h"
#include "pike_error.h"
#include "pike_float.h"
#include "pike_types.h"
#include "operators.h"
#include "builtin_functions.h"

#ifndef CB_NODE_ALLOC
# define CB_NODE_ALLOC()	((cb_node_t)xalloc(sizeof(cb_node)))
#endif
#ifndef CB_NODE_FREE
# define CB_NODE_FREE(p)	xfree(p)
#endif

#define CB_STATIC
#define CB_INLINE
#define CB_SOURCE
#define CB_NAMESPACE

static int identifier_is_native(struct program *p, struct program *origp, int fun_num) {
    struct reference *ref;
    struct inherit *inh;

    ref = PTR_FROM_INT(p, fun_num);
    inh = INHERIT_FROM_PTR(p, ref);

    if (inh->prog != origp) return 0;

    return 1;
}

