/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: svalue.h,v 1.24 1999/04/07 23:10:11 hubbe Exp $
 */
#ifndef SVALUE_H
#define SVALUE_H

#include "global.h"

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
  INT_TYPE integer;
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

#define T_VOID 16
#define T_MANY 17

#define T_ASSIGN 245
#define T_DELETED 246
#define T_NOT 247
#define T_AND 248
#define T_UNKNOWN 249
#define T_OR 251
#define T_SHORT_LVALUE 252
#define T_LVALUE 253
#define T_ARRAY_LVALUE 254
#define T_MIXED 255

extern char *type_name[];

#define tArr(VAL) "\000" VAL
#define tArray tArr(tMix)
#define tMap(IND,VAL) "\001" IND VAL
#define tMapping tMap(tMix,tMix)
#define tSet(IND) "\002" IND
#define tMultiset tSet(tMix)
#define tObj "\003\000\000\000\000\000"
#define tFuncV(ARGS,REST,RET) "\004" ARGS "\021" REST RET
#define tFunc(ARGS,RET) tFuncV(ARGS,tVoid,RET)
#define tFunction tFuncV(,tMix,tMix)
#define tPrg "\005"
#define tProgram "\005"
#define tStr "\006"
#define tString "\006"
#define tFlt "\007"
#define tFloat "\007"
#define tInt "\010\200\000\000\000\177\377\377\377"

#define tVoid "\020"
#define tVar(X) #X
#define tSetvar(X,Y) "\365" #X Y
#define tNot(X) "\367" X
#define tAnd(X,Y) "\370" X Y
#define tOr(X,Y) "\373" X Y
#define tOr3(X,Y,Z) tOr(X,tOr(Y,Z))
#define tOr4(X,Y,Z,A) tOr(X,tOr(Y,tOr(Z,A)))
#define tOr5(X,Y,Z,A,B) tOr(X,tOr(Y,tOr(Z,tOr(A,B))))
#define tOr6(X,Y,Z,A,B,C) tOr(X,tOr(Y,tOr(Z,tOr(A,tOr(B,C)))))
#define tOr7(X,Y,Z,A,B,C,D) tOr(X,tOr(Y,tOr(Z,tOr(A,tOr(B,tOr(C,D))))))
#define tMix "\377"
#define tMixed "\377"
#define tComplex tOr6(tArray,tMapping,tMultiset,tObj,tFunction,tProgram)
#define tRef tOr(tString,tComplex)
#define tIfnot(X,Y) tAnd(tNot(X),Y)

#define BIT_ARRAY (1<<T_ARRAY)
#define BIT_MAPPING (1<<T_MAPPING)
#define BIT_MULTISET (1<<T_MULTISET)
#define BIT_OBJECT (1<<T_OBJECT)
#define BIT_FUNCTION (1<<T_FUNCTION)
#define BIT_PROGRAM (1<<T_PROGRAM)
#define BIT_STRING (1<<T_STRING)
#define BIT_INT (1<<T_INT)
#define BIT_FLOAT (1<<T_FLOAT)

/* Used to signify that this array might not be finished yet */
/* garbage collect uses this */
#define BIT_UNFINISHED (1<<15)

/* This is only used in typechecking to signify that this 
 * argument may be omitted.
 */
#define BIT_VOID (1<< T_VOID)

/* This is used in typechecking to signify that the rest of the
 * arguments has to be of this type.
 */
#define BIT_MANY (1 << T_MANY)

#define BIT_NOTHING 0
#define BIT_MIXED 0x7fff
#define BIT_BASIC (BIT_INT|BIT_FLOAT|BIT_STRING)
#define BIT_COMPLEX (BIT_ARRAY|BIT_MULTISET|BIT_OBJECT|BIT_PROGRAM|BIT_MAPPING|BIT_FUNCTION)

/* Max type which contains svalues */
#define MAX_COMPLEX T_PROGRAM
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

#ifdef PIKE_DEBUG
#define check_type(T) if(T > MAX_TYPE && T!=T_LVALUE && T!=T_SHORT_LVALUE && T!=T_VOID && T!=T_DELETED && T!=T_ARRAY_LVALUE) fatal("Type error: %d\n",T)
#define check_refs(S) if((S)->type < MAX_REF_TYPE && (!(S)->u.refs || (S)->u.refs[0] < 0)) fatal("Svalue to object without references.\n")
#define check_refs2(S,T) if((T) < MAX_REF_TYPE && (S)->refs && (S)->refs[0] <= 0) fatal("Svalue to object without references.\n")

#ifdef DEBUG_MALLOC
#define add_ref(X) ((INT32 *)debug_malloc_pass( &((X)->refs)))[0]++
#else
#define add_ref(X) (X)->refs++
#endif


#else

#define check_type(T)
#define check_refs(S)
#define check_refs2(S,T)
#define add_ref(X) (X)->refs++

#endif

#define free_svalue(X) do { struct svalue *_s=(X); check_type(_s->type); check_refs(_s); if(_s->type<=MAX_REF_TYPE) { debug_malloc_touch(_s->u.refs); if(--*(_s->u.refs) <=0) really_free_svalue(_s); } }while(0)
#define free_short_svalue(X,T) do { union anything *_s=(X); TYPE_T _t=(T); check_type(_t); check_refs2(_s,_t); if(_t<=MAX_REF_TYPE && _s->refs) if(--*(_s->refs) <= 0) really_free_short_svalue(_s,_t); }while(0)
#define assign_svalue_no_free(X,Y) do { struct svalue _tmp, *_to=(X), *_from=(Y); check_type(_from->type); check_refs(_from);  *_to=_tmp=*_from; if(_tmp.type <= MAX_REF_TYPE) { debug_malloc_touch(_tmp.u.refs); _tmp.u.refs[0]++; } }while(0)
#define assign_svalue(X,Y) do { struct svalue *_to2=(X), *_from2=(Y); free_svalue(_to2); assign_svalue_no_free(_to2, _from2);  }while(0)

extern struct svalue dest_ob_zero;

#ifdef DEBUG_MALLOC
#define LINE_ARGS  , int line, char * file
#define free_svalues(X,Y,Z) debug_free_svalues((X),(Y),(Z),__LINE__,__FILE__)
#else
#define LINE_ARGS
#define free_svalues(X,Y,Z) debug_free_svalues((X),(Y),(Z))
#endif

/* Prototypes begin here */
void really_free_short_svalue(union anything *s, TYPE_T type);
void really_free_svalue(struct svalue *s);
void do_free_svalue(struct svalue *s);
void debug_free_svalues(struct svalue *s,INT32 num, INT32 type_hint LINE_ARGS);
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
void check_short_svalue(union anything *u, TYPE_T type);
void check_svalue(struct svalue *s);
TYPE_FIELD gc_check_svalues(struct svalue *s, int num);
void gc_xmark_svalues(struct svalue *s, int num);
void gc_check_short_svalue(union anything *u, TYPE_T type);
void gc_mark_svalues(struct svalue *s, int num);
void gc_mark_short_svalue(union anything *u, TYPE_T type);
INT32 pike_sizeof(struct svalue *s);
/* Prototypes end here */

#endif
