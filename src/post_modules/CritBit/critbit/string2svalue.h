/* -*- mode: C; c-basic-offset: 4; -*- */
#ifdef CB_NAMESPACE
# undef CB_NAMESPACE
#endif
#define CB_NAMESPACE	string2svalue

#ifndef CB_STATIC
# define CB_STATIC
#else
# undef CB_STATIC
# define CB_STATIC static
#endif

#ifndef CB_STRING2SVALUE_H
# define CB_STRING2SVALUE_H
# include "critbit.h"
# include "key_pikestring.h"
# include "value_svalue.h"
# include "tree_low.h"
#endif
