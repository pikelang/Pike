#ifndef CB_VALUE_SVALUE_H
#define CB_VALUE_SVALUE_H
#include "svalue.h"

typedef struct svalue CB_NAME(value);

#ifdef cb_value
# undef cb_value
#endif
#define cb_value CB_NAME(value)

#define CB_SET_VALUE(node, v)	assign_svalue(&((node)->value), (v))
#define CB_GET_VALUE(node, v)	assign_svalue_no_free((v), &((node)->value))
#define CB_FREE_VALUE(v)	free_svalue(v)
#define CB_HAS_VALUE(node)	(TYPEOF((node)->value) != T_VOID)
#define CB_INIT_VALUE(node)	do { SET_SVAL_TYPE((node)->value, T_VOID); } while(0)
#define CB_RM_VALUE(node)	do { if (CB_HAS_VALUE(node)) \
    free_svalue(&((node)->value)); CB_INIT_VALUE(node); } while(0)
#define CB_PUSH_VALUE(v)	push_svalue(&(v))

#endif
