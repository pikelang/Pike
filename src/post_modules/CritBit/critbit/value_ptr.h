/* -*- mode: C; c-basic-offset: 4; -*- */
#ifndef _PTR_VALUE_H
#define _PTR_VALUE_H

#define CB_SET_VALUE(node, v)	\
	do { (node)->value = (cb_value)(*(v)); } while (0)
#define CB_GET_VALUE(node, v)	do { (*(v)) = (node)->value; } while(0)
#define CB_FREE_VALUE(v)	do {} while(0)
#define CB_HAS_VALUE(node)	(!!((node)->value))
#define CB_INIT_VALUE(node)	do { (node)->value = NULL; } while(0)
#define CB_RM_VALUE(node)	do { CB_INIT_VALUE(node); } while(0)
#endif /* _PTR_VALUE_H */
