/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef SVALUE_H
#define SVALUE_H

#include "types.h"

#ifndef STRUCT_ARRAY_DECLARED
#define STRUCT_ARRAY_DECLARED
struct array;
#endif

#ifndef STRUCT_MAPPING_DECLARED
#define STRUCT_MAPPING_DECLARED
struct mapping;
#endif

#ifndef STRUCT_MULTISET_DECLARED
#define STRUCT_MULTISET_DECLARED
struct multiset;
#endif

#ifndef STRUCT_OBJECT_DECLARED
#define STRUCT_OBJECT_DECLARED
struct object;
#endif

#ifndef STRUCT_PROGRAM_DECLARED
#define STRUCT_PROGRAM_DECLARED
struct program;
#endif

#ifndef STRUCT_PIKE_STRING_DECLARED
#define STRUCT_PIKE_STRING_DECLARED
struct pike_string;
#endif

#ifndef STRUCT_CALLABLE_DECLARED
#define STRUCT_CALLABLE_DECLARED
struct callable;
#endif

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
  struct multiset *multiset;
  struct object *object;
  struct program *program;
  struct pike_string *string;
  INT32 *refs;
  INT32 integer;
  FLOAT_TYPE float_number;
  struct svalue *lval;   /* only used on stack */
  union anything *short_lval;   /* only used on stack */
};

#ifndef STRUCT_SVALUE_DECLARED
#define STRUCT_SVALUE_DECLARED
#endif
struct svalue
{
  unsigned INT16 type;
  unsigned INT16 subtype;
  union anything u;
};

#define T_ARRAY 0
#define T_MAPPING 1
#define T_MULTISET 2
#define T_OBJECT 3
#define T_FUNCTION 4
#define T_PROGRAM 5
#define T_STRING 6
#define T_FLOAT 7
#define T_INT 8

#define T_DELETED 246
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
#define BIT_MULTISET (1<<T_MULTISET)
#define BIT_OBJECT (1<<T_OBJECT)
#define BIT_FUNCTION (1<<T_FUNCTION)
#define BIT_PROGRAM (1<<T_PROGRAM)
#define BIT_STRING (1<<T_STRING)
#define BIT_INT (1<<T_INT)
#define BIT_FLOAT (1<<T_FLOAT)

/* Used to signifiy that this array might not be finished yet */
/* garbage collect uses this */
#define BIT_UNFINISHED (1<<15)

#define BIT_NOTHING 0
#define BIT_MIXED 0x7fff
#define BIT_BASIC (BIT_INT|BIT_FLOAT|BIT_STRING)
#define BIT_COMPLEX (BIT_ARRAY|BIT_MULTISET|BIT_OBJECT|BIT_PROGRAM|BIT_MAPPING|BIT_FUNCTION)

/* Max type with ref count */
#define MAX_REF_TYPE T_STRING
/* Max type handled by svalue primitives */
#define MAX_TYPE T_INT

#define NUMBER_NUMBER 0
#define NUMBER_UNDEFINED 1
#define NUMBER_DESTRUCTED 2

#define FUNCTION_BUILTIN USHRT_MAX

#define is_gt(a,b) is_lt(b,a)
#define IS_ZERO(X) ((X)->type==T_INT?(X)->u.integer==0:(1<<(X)->type)&(BIT_OBJECT|BIT_FUNCTION)?!svalue_is_true(X):0)

#define IS_UNDEFINED(X) ((X)->type==T_INT&&!(X)->u.integer&&(X)->subtype==1)

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

/* var MUST be a variable!!! */
#define safe_check_destructed(var) do{ \
  if((var->type == T_OBJECT || var->type==T_FUNCTION) && !var->u.object->prog) \
    var=&dest_ob_zero; \
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
#define check_type(T) if(T > MAX_TYPE && T!=T_LVALUE && T!=T_SHORT_LVALUE && T!=T_VOID && T!=T_DELETED) fatal("Type error\n")
#define check_refs(S) if((S)->type < MAX_REF_TYPE && (!(S)->u.refs || (S)->u.refs[0] < 0)) fatal("Svalue to object without references.\n")
#define check_refs2(S,T) if((T) < MAX_REF_TYPE && (S)->refs && (S)->refs[0] <= 0) fatal("Svalue to object without references.\n")

#else

#define check_type(T)
#define check_refs(S)
#define check_refs2(S,T)

#endif

#define free_svalue(X) do { struct svalue *_s=(X); check_type(_s->type); check_refs(_s); if(_s->type<=MAX_REF_TYPE && --*(_s->u.refs) <=0) really_free_svalue(_s); }while(0)
#define assign_svalue_no_free(X,Y) do { struct svalue _tmp, *_to=(X), *_from=(Y); check_type(_from->type); check_refs(_from);  *_to=_tmp=*_from; if(_tmp.type <= MAX_REF_TYPE) _tmp.u.refs[0]++; }while(0)
#define assign_svalue(X,Y) do { struct svalue *_to2=(X), *_from2=(Y); free_svalue(_to2); assign_svalue_no_free(_to2, _from2);  }while(0)

extern struct svalue dest_ob_zero;

/* Prototypes begin here */
void free_short_svalue(union anything *s,TYPE_T type);
void really_free_svalue(struct svalue *s);
void free_svalues(struct svalue *s,INT32 num, INT32 type_hint);
void assign_svalues_no_free(struct svalue *to,
			    struct svalue *from,
			    INT32 num,
			    INT32 type_hint);
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
unsigned INT32 hash_svalue(struct svalue *s);
int svalue_is_true(struct svalue *s);
int is_eq(struct svalue *a, struct svalue *b);
int low_is_equal(struct svalue *a,
		 struct svalue *b,
		 struct processing *p);
int low_short_is_equal(const union anything *a,
		       const union anything *b,
		       TYPE_T type,
		       struct processing *p);
int is_equal(struct svalue *a,struct svalue *b);
int is_lt(struct svalue *a,struct svalue *b);
void describe_svalue(struct svalue *s,int indent,struct processing *p);
void clear_svalues(struct svalue *s, INT32 num);
void copy_svalues_recursively_no_free(struct svalue *to,
				      struct svalue *from,
				      INT32 num,
				      struct processing *p);
void check_short_svalue(union anything *u,TYPE_T type);
void check_svalue(struct svalue *s);
TYPE_FIELD gc_check_svalues(struct svalue *s, int num);
void gc_check_short_svalue(union anything *u, TYPE_T type);
void gc_mark_svalues(struct svalue *s, int num);
void gc_mark_short_svalue(union anything *u, TYPE_T type);
INT32 pike_sizeof(struct svalue *s);
/* Prototypes end here */

#endif
