/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef SVALUE_H
#define SVALUE_H

#include "types.h"

struct array;
struct mapping;
struct list;
struct object;
struct program;
struct lpc_string;
struct callable;

struct processing
{
  struct processing *next;
  void *pointer_a, *pointer_b;
};

union anything
{
  struct callable *efun;
  struct array *array;
  struct mapping *mapping;
  struct list *list;
  struct object *object;
  struct program *program;
  struct lpc_string *string;
  INT32 *refs;
  INT32 integer;
  FLOAT_TYPE float_number;
  struct svalue *lval;   /* only used on stack */
  union anything *short_lval;   /* only used on stack */
};

struct svalue
{
  INT16 type;
  INT16 subtype;
  union anything u;
};

#define T_ARRAY 0
#define T_MAPPING 1
#define T_LIST 2
#define T_OBJECT 3
#define T_FUNCTION 4
#define T_PROGRAM 5
#define T_STRING 6
#define T_FLOAT 7
#define T_INT 8

#define T_NOT 247
#define T_AND 248
#define T_UNKNOWN 249
#define T_MANY 250
#define T_OR 251
#define T_SHORT_LVALUE 252
#define T_LVALUE 253
#define T_VOID 254
#define T_MIXED 255

#define BIT_ARRAY (1<<T_ARRAY)
#define BIT_MAPPING (1<<T_MAPPING)
#define BIT_LIST (1<<T_LIST)
#define BIT_OBJECT (1<<T_OBJECT)
#define BIT_FUNCTION (1<<T_FUNCTION)
#define BIT_PROGRAM (1<<T_PROGRAM)
#define BIT_STRING (1<<T_STRING)
#define BIT_INT (1<<T_INT)
#define BIT_FLOAT (1<<T_FLOAT)

#define BIT_NOTHING 0
#define BIT_MIXED 0xffff
#define BIT_BASIC (BIT_INT|BIT_FLOAT|BIT_STRING)
#define BIT_COMPLEX (BIT_ARRAY|BIT_LIST|BIT_OBJECT|BIT_PROGRAM|BIT_MAPPING)

/* Max type with ref count */
#define MAX_REF_TYPE T_STRING
/* Max type handled by svalue primitives */
#define MAX_TYPE T_INT

#define NUMBER_NUMBER 0
#define NUMBER_UNDEFINED 1
#define NUMBER_DESTRUCTED 2

#define is_gt(a,b) is_lt(b,a)
#define IS_ZERO(X) ((X)->type==T_INT && (X)->u.integer==0)

#define check_destructed(S) \
do{ \
  struct svalue *_s=(S); \
  if((_s->type == T_OBJECT || _s->type==T_FUNCTION) && !_s->u.object->prog) { \
    free_object(_s->u.object); \
    _s->type = T_INT; \
    _s->subtype = NUMBER_DESTRUCTED ; \
    _s->u.integer = 0; \
  } \
}while(0)

#define check_short_destructed(U,T) \
do{ \
  union anything *_u=(U); \
  if(( (1<<(T)) & (BIT_OBJECT | BIT_FUNCTION) ) && \
     _u->object && !_u->object->prog) { \
    free_object(_u->object); \
    _u->object = 0; \
  } \
}while(0)

#ifdef DEBUG
#define check_type(T) if(T > MAX_TYPE && T!=T_LVALUE && T!=T_SHORT_LVALUE && T!=T_VOID) fatal("Type error\n")
#define check_refs(S) if((S)->type < MAX_REF_TYPE && (!(S)->u.refs || (S)->u.refs[0] < 0)) fatal("Svalue to object without references.\n")
#define check_refs2(S,T) if((T) < MAX_REF_TYPE && (S)->refs && (S)->refs[0] <= 0) fatal("Svalue to object without references.\n")

#else

#define check_type(T)
#define check_refs(S)
#define check_refs2(S,T)

#endif


/* Prototypes begin here */
void free_short_svalue(union anything *s,TYPE_T type);
void free_svalue(struct svalue *s);
void free_svalues(struct svalue *s,INT32 num, INT32 type_hint);
void assign_svalue_no_free(struct svalue *to,
			   struct svalue *from);
void assign_svalues_no_free(struct svalue *to,
			    struct svalue *from,
			    INT32 num,
			    INT32 type_hint);
void assign_svalue(struct svalue *to, struct svalue *from);
void assign_svalues(struct svalue *to,
		    struct svalue *from,
		    INT32 num,
		    TYPE_FIELD types);
void assign_to_short_svalue(union anything *u,
			    TYPE_T type,
			    struct svalue *s);
void assign_to_short_svalue_no_free(union anything *u,
				    TYPE_T type,
				    struct svalue *s);
void assign_from_short_svalue_no_free(struct svalue *s,
					     union anything *u,
					     TYPE_T type);
void assign_short_svalue_no_free(union anything *to,
				 union anything *from,
				 TYPE_T type);
void assign_short_svalue(union anything *to,
			 union anything *from,
			 TYPE_T type);
int is_eq(struct svalue *a, struct svalue *b);
int low_is_equal(struct svalue *a,
		 struct svalue *b,
		 struct processing *p);
int low_short_is_equal(const union anything *a,
		       const union anything *b,
		       TYPE_T type,
		       struct processing *p);
int is_equal(struct svalue *a,struct svalue *b);
int is_lt(const struct svalue *a,const struct svalue *b);
void describe_svalue(struct svalue *s,int indent,struct processing *p);
void clear_svalues(struct svalue *s, INT32 num);
void copy_svalues_recursively_no_free(struct svalue *to,
				      struct svalue *from,
				      INT32 num,
				      struct processing *p);
void check_short_svalue(union anything *u,TYPE_T type);
void check_svalue(struct svalue *s);
void gc_check_svalues(struct svalue *s, int num);
void gc_check_short_svalue(union anything *u, TYPE_T type);
/* Prototypes end here */

#endif
