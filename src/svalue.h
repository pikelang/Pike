/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: svalue.h,v 1.117 2004/09/30 15:22:37 mast Exp $
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
   
struct ref_dummy;

union anything
{
  INT_TYPE integer;		/* Union initializations assume this first. */
  struct callable *efun;
  struct array *array;
  struct mapping *mapping;
  struct multiset *multiset;
  struct object *object;
  struct program *program;
  struct pike_string *string;
  struct pike_type *type;
  INT32 *refs;
  struct ref_dummy *dummy;
  FLOAT_TYPE float_number;
  struct svalue *lval;   /* only used on stack */
  union anything *short_lval;   /* only used on stack */
  void *ptr;
};

#ifndef STRUCT_SVALUE_DECLARED
#define STRUCT_SVALUE_DECLARED
#endif

/* Note: At least multisets overlays the type field and uses the top 4
 * bits in it internally. */

struct svalue
{
  unsigned INT16 type;
  unsigned INT16 subtype;
  union anything u;
};

#define PIKE_T_ARRAY 0
#define PIKE_T_MAPPING 1
#define PIKE_T_MULTISET 2
#define PIKE_T_OBJECT 3
#define PIKE_T_FUNCTION 4
#define PIKE_T_PROGRAM 5
#define PIKE_T_STRING 6
#define PIKE_T_TYPE 7
#define PIKE_T_INT 8
#define PIKE_T_FLOAT 9

#define PIKE_T_ZERO  14	/* Can return 0, but nothing else */
#define T_UNFINISHED 15	/* Reserved for the garbage-collector */
#define T_VOID       16	/* Can't return any value */
#define T_MANY       17

#define PIKE_T_RING 240
#define PIKE_T_NAME 241		/* Named type. */
#define PIKE_T_SCOPE 243	/* Not supported yet */
#define PIKE_T_TUPLE 244	/* Not supported yet */
#define T_ASSIGN 245
#define T_DELETED 246
#define PIKE_T_UNKNOWN 247
#define T_SHORT_LVALUE 248
#define T_LVALUE 249
#define T_ARRAY_LVALUE 250
#define PIKE_T_MIXED 251
#define T_NOT 253
#define T_AND 254
#define T_OR 255

/* These are only used together with describe() and friends. */
#define T_STORAGE 10000
#define T_MAPPING_DATA 10001
#define T_PIKE_FRAME 10002
#define T_MULTISET_DATA 10003

#define tArr(VAL) "\000" VAL
#define tArray tArr(tMix)
#define tMap(IND,VAL) "\001" IND VAL
#define tMapping tMap(tMix,tMix)
#define tSet(IND) "\002" IND
#define tMultiset tSet(tMix)
#define tObj "\003\000\000\000\000\000"

#define tFuncV(ARGS,REST,RET) MagictFuncV(RET,REST,ARGS)
#define tFunc(ARGS,RET) MagictFunc(RET,ARGS)

#define tTuple(T1,T2)		"\364" T1 T2
#define tTriple(T1,T2,T3)	tTuple(T1, tTuple(T2, T3))
#define tQuad(T1,T2,T3,T4)	tTriple(tTuple(T1, T2), T3, T4)

/* These two magic funcions are used to make MSVC++ work
 * even if 'ARGS' is empty.
 */
#define MagictFuncV(RET,REST,ARGS) "\004" ARGS "\021" REST RET
#define MagictFunc(RET,ARGS) tFuncV(ARGS "", tVoid, RET)
#define tFunction tFuncV("" ,tOr(tZero,tVoid),tOr(tMix,tVoid))
#define tNone ""
#define tPrg(X) "\005" X
#define tProgram(X) "\005" X
#define tStr "\006"
#define tString "\006"
#define tType(T) "\007" T
#define tInt "\010\200\000\000\000\177\377\377\377"
#define tInt0 "\010\000\000\000\000\000\000\000\000"
#define tInt1 "\010\000\000\000\001\000\000\000\001"
#define tInt2 "\010\000\000\000\002\000\000\000\002"
#define tInt01 "\010\000\000\000\000\000\000\000\001"
#define tInt02 "\010\000\000\000\000\000\000\000\002"
#define tInt03 "\010\000\000\000\000\000\000\000\003"
#define tInt04 "\010\000\000\000\000\000\000\000\004"
#define tInt05 "\010\000\000\000\000\000\000\000\005"
#define tInt06 "\010\000\000\000\000\000\000\000\006"
#define tIntPos "\010\000\000\000\000\177\377\377\377"
#define tInt1Plus "\010\000\000\000\001\177\377\377\377"
#define tInt2Plus "\010\000\000\000\002\177\377\377\377"
#define tByte "\010\000\000\000\000\000\000\000\377"
#define tFlt "\011"
#define tFloat "\011"

#define tZero "\016"
#define tVoid "\020"
#define tVar(X) #X
#define tSetvar(X,Y) "\365" #X Y
#define tScope(X,T) "\363" #X Y
#define tNot(X) "\375" X
#define tAnd(X,Y) "\376" X Y
#define tOr(X,Y) "\377" X Y
#define tOr3(X,Y,Z) tOr(X,tOr(Y,Z))
#define tOr4(X,Y,Z,A) tOr(X,tOr(Y,tOr(Z,A)))
#define tOr5(X,Y,Z,A,B) tOr(X,tOr(Y,tOr(Z,tOr(A,B))))
#define tOr6(X,Y,Z,A,B,C) tOr(X,tOr(Y,tOr(Z,tOr(A,tOr(B,C)))))
#define tOr7(X,Y,Z,A,B,C,D) tOr(X,tOr(Y,tOr(Z,tOr(A,tOr(B,tOr(C,D))))))
#define tOr8(A,B,C,D,E,F,G,H) tOr(A,tOr7(B,C,D,E,F,G,H))
#define tOr9(A,B,C,D,E,F,G,H,I) tOr(A,tOr8(B,C,D,E,F,G,H,I))
#define tMix "\373"
#define tMixed "\373"
#define tComplex tOr6(tArray,tMapping,tMultiset,tObj,tFunction,tPrg(tObj))
#define tStringIndicable tOr5(tMapping,tObj,tFunction,tPrg(tObj),tMultiset)
#define tRef tOr(tString,tComplex)
#define tIfnot(X,Y) tAnd(tNot(X),Y)
#define tAny tOr(tVoid,tMix)
#define tName(X,Y) "\361\0"X"\0"Y
#if PIKE_BYTEORDER == 1234
/* Little endian */
#define tName1(X,Y) "\361\5"X"\0\0"Y
#define tName2(X,Y) "\361\6"X"\0\0\0\0"Y
#else /* PIKE_BYTEORDER != 1234 */
/* Big endian */
#define tName1(X,Y) "\361\1"X"\0\0"Y
#define tName2(X,Y) "\361\2"X"\0\0\0\0"Y
#endif /* PIKE_BYTEORDER == 1234 */

#define tSimpleCallable tOr3(tArray,tFunction,tObj)
#define tCallable tOr3(tArr(tSimpleCallable),tFunction,tObj)

#define BIT_ARRAY (1<<PIKE_T_ARRAY)
#define BIT_MAPPING (1<<PIKE_T_MAPPING)
#define BIT_MULTISET (1<<PIKE_T_MULTISET)
#define BIT_OBJECT (1<<PIKE_T_OBJECT)
#define BIT_FUNCTION (1<<PIKE_T_FUNCTION)
#define BIT_PROGRAM (1<<PIKE_T_PROGRAM)
#define BIT_STRING (1<<PIKE_T_STRING)
#define BIT_TYPE (1<<PIKE_T_TYPE)
#define BIT_INT (1<<PIKE_T_INT)
#define BIT_FLOAT (1<<PIKE_T_FLOAT)

#define BIT_ZERO (1<<PIKE_T_ZERO)

/* Used to signify that this array might not be finished yet */
/* garbage collect uses this */
#define BIT_UNFINISHED (1 << T_UNFINISHED)

/* This is only used in typechecking to signify that this 
 * argument may be omitted.
 */
#define BIT_VOID (1 << T_VOID)

/* This is used in typechecking to signify that the rest of the
 * arguments has to be of this type.
 */
#define BIT_MANY (1 << T_MANY)

#define BIT_NOTHING 0
#define BIT_MIXED 0x7fff
#define BIT_BASIC (BIT_INT|BIT_FLOAT|BIT_STRING|BIT_TYPE)
#define BIT_COMPLEX (BIT_ARRAY|BIT_MULTISET|BIT_OBJECT|BIT_PROGRAM|BIT_MAPPING|BIT_FUNCTION)
#define BIT_CALLABLE (BIT_FUNCTION|BIT_PROGRAM|BIT_ARRAY|BIT_OBJECT)

/* Max type which contains svalues */
#define MAX_COMPLEX PIKE_T_PROGRAM
/* Max type with ref count */
#define MAX_REF_TYPE PIKE_T_TYPE
/* Max type handled by svalue primitives */
#define MAX_TYPE PIKE_T_FLOAT

#define NUMBER_NUMBER     0
#define NUMBER_UNDEFINED  1
#define NUMBER_DESTRUCTED 2
#define NUMBER_LOCKED     USHRT_MAX	/* NOTE: Kludge, only in Pike 7.4. */

#define FUNCTION_BUILTIN USHRT_MAX

#define is_gt(a,b) is_lt(b,a)

/* SAFE_IS_ZERO is compatible with the old IS_ZERO, but you should
 * consider using UNSAFE_IS_ZERO instead, since exceptions thrown from
 * `! functions will be propagated correctly then. */
#define UNSAFE_IS_ZERO(X) ((X)->type==PIKE_T_INT?(X)->u.integer==0:(1<<(X)->type)&(BIT_OBJECT|BIT_FUNCTION)?!svalue_is_true(X):0)
#define SAFE_IS_ZERO(X) ((X)->type==PIKE_T_INT?(X)->u.integer==0:(1<<(X)->type)&(BIT_OBJECT|BIT_FUNCTION)?!safe_svalue_is_true(X):0)

#define IS_UNDEFINED(X) ((X)->type==PIKE_T_INT&&!(X)->u.integer&&(X)->subtype==1)

#define IS_DESTRUCTED(X) \
  (((X)->type == PIKE_T_OBJECT || (X)->type==PIKE_T_FUNCTION) && !(X)->u.object->prog)

#define check_destructed(S) \
do{ \
  struct svalue *_s=(S); \
  if(IS_DESTRUCTED(_s)) { \
    free_object(_s->u.object); \
    _s->type = PIKE_T_INT; \
    _s->subtype = NUMBER_DESTRUCTED ; \
    _s->u.integer = 0; \
  } \
}while(0)

/* var MUST be a variable!!! */
#define safe_check_destructed(var) do{ \
  if((var->type == PIKE_T_OBJECT || var->type==PIKE_T_FUNCTION) && !var->u.object->prog) \
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


#ifdef PIKE_RUN_UNLOCKED
#define add_ref(X) pike_atomic_inc32(&(X)->refs)
#define sub_ref(X) pike_atomic_dec_and_test32(&(X)->refs)

#if 0
#define IF_LOCAL_MUTEX(X) X
#define USE_LOCAL_MUTEX
#define pike_lock_data(X) mt_lock(&(X)->mutex)
#define pike_unlock_data(X) mt_unlock(&(X)->mutex)
#else
#define IF_LOCAL_MUTEX(X)
#define pike_lock_data(X) pike_lockmem((X))
#define pike_unlock_data(X) pike_unlockmem((X))
#endif

#else
#define IF_LOCAL_MUTEX(X)
#define add_ref(X) (void)((X)->refs++)
#define sub_ref(X) (--(X)->refs > 0)
#define pike_lock_data(X) (void)(X)
#define pike_unlock_data(X) (void)(X)
#endif


#ifdef PIKE_DEBUG
PMOD_EXPORT extern void describe(void *); /* defined in gc.c */
PMOD_EXPORT extern const char msg_type_error[];
#define check_type(T) if(T > MAX_TYPE && T!=T_LVALUE && T!=T_SHORT_LVALUE && T!=T_VOID && T!=T_DELETED && T!=T_ARRAY_LVALUE) Pike_fatal(msg_type_error,T)

#define check_svalue(S) debug_check_svalue(dmalloc_check_svalue(S,DMALLOC_LOCATION()))

PMOD_EXPORT extern const char msg_sval_obj_wo_refs[];
#define check_refs(S) do {\
 if((S)->type <= MAX_REF_TYPE && (!(S)->u.refs || (S)->u.refs[0] < 0)) { \
 describe((S)->u.refs); \
 Pike_fatal(msg_sval_obj_wo_refs); \
} }while(0)

PMOD_EXPORT extern const char msg_ssval_obj_wo_refs[];
#define check_refs2(S,T) do { \
if((T) <= MAX_REF_TYPE && (S)->refs && (S)->refs[0] <= 0) {\
 describe((S)->refs); \
 Pike_fatal(msg_ssval_obj_wo_refs); \
} }while(0)

#ifdef DEBUG_MALLOC
static inline struct svalue *dmalloc_check_svalue(struct svalue *s, char *l)
{
#if 0
  debug_malloc_update_location(s,l);
#endif
#if 1
  if(s && s->type <= MAX_REF_TYPE)
    debug_malloc_update_location(s->u.refs,l);
#endif
  return s;
}

static inline struct svalue *dmalloc_check_svalues(struct svalue *s, size_t num, char *l)
{
  while (num--) dmalloc_check_svalue (s + num, l);
  return s;
}

static inline union anything *dmalloc_check_union(union anything *u,int type, char * l)
{
#if 0
  debug_malloc_update_location(u,l);
#endif
#if 1
  if(u && type <= MAX_REF_TYPE)
    debug_malloc_update_location(u->refs,l);
#endif
  return u;
}

#undef add_ref
#undef sub_ref

#ifdef PIKE_RUN_UNLOCKED
#define add_ref(X) pike_atomic_inc32((INT32 *)debug_malloc_pass( &((X)->refs)))
#define sub_ref(X) pike_atomic_dec_and_test32((INT32 *)debug_malloc_pass( &((X)->refs)))
#else
#define add_ref(X) (((INT32 *)debug_malloc_pass( &((X)->refs)))[0]++)
#define sub_ref(X) (--((INT32 *)debug_malloc_pass( &((X)->refs)))[0] > 0)
#endif


#else
#define dmalloc_check_svalue(S,L) (S)
#define dmalloc_check_svalues(S,L,N) (S)
#define dmalloc_check_union(U,T,L) (U)

#endif

#else

#define check_svalue(S)
#define check_type(T)
#define check_refs(S)
#define check_refs2(S,T)
#define dmalloc_check_svalue(S,L) (S)
#define dmalloc_check_svalues(S,L,N) (S)
#define dmalloc_check_union(U,T,L) (U)

#endif


/* This define
 * should check that the svalue address (X) is on the local stack,
 * the processor stack or in a locked memory object
 *
 * Or, it could just try to make sure it's not in an unlocked memory
 * object...
 */
#define assert_svalue_locked(X)


#define swap_svalues_unlocked(X,Y)  do {		\
  struct svalue *_a=(X);				\
  struct svalue *_b=(Y);				\
  struct svalue _tmp;					\
  assert_svalue_locked(_a); assert_svalue_locked(_b);	\
  _tmp=*_a; *_a=*_b; *_b=_tmp;				\
}while(0)

#define free_svalue_unlocked(X) do {				\
  struct svalue *_s=(X);					\
  assert_svalue_locked(_s);					\
  check_type(_s->type); check_refs(_s);				\
  if(_s->type<=MAX_REF_TYPE) {					\
    debug_malloc_touch(_s->u.refs);				\
    if(sub_ref(_s->u.dummy) <=0) { really_free_svalue(_s); }	\
  }								\
  DO_IF_DMALLOC(_s->type=PIKE_T_UNKNOWN;_s->u.refs=(void *)-1); \
  PIKE_MEM_WO(*_s);						\
}while(0)

#define free_short_svalue_unlocked(X,T) do {				\
  union anything *_s=(X); TYPE_T _t=(T);				\
  check_type(_t); check_refs2(_s,_t);					\
  assert_svalue_locked(_s);						\
  if(_t<=MAX_REF_TYPE && _s->refs) {					\
    if(sub_ref(_s->dummy) <= 0) really_free_short_svalue(_s,_t);	\
  }									\
  DO_IF_DMALLOC(_s->refs=(void *)-1);					\
  PIKE_MEM_WO(_s->refs);						\
}while(0)

#define add_ref_svalue_unlocked(X) do {				\
  struct svalue *_tmp=(X);					\
  check_type(_tmp->type); check_refs(_tmp);			\
  if(_tmp->type <= MAX_REF_TYPE) add_ref(_tmp->u.dummy);	\
}while(0)

#define assign_svalue_no_free_unlocked(X,Y) do {	\
  struct svalue _tmp;					\
  struct svalue *_to=(X);				\
  const struct svalue *_from=(Y);			\
  check_type(_from->type); check_refs(_from);		\
  *_to=_tmp=*_from;					\
  if(_tmp.type <= MAX_REF_TYPE) add_ref(_tmp.u.dummy);	\
}while(0)

#define assign_svalue_unlocked(X,Y) do {	\
  struct svalue *_to2=(X);			\
  const struct svalue *_from2=(Y);		\
  if (_to2 != _from2) {				\
    free_svalue(_to2);				\
     assign_svalue_no_free(_to2, _from2);	\
  }						\
}while(0)

#define move_svalue(TO, FROM) do {					\
    struct svalue *_to = (TO);						\
    struct svalue *_from = (FROM);					\
    dmalloc_touch_svalue(_from);					\
    *_to = *_from;							\
    DO_IF_DMALLOC(_from->type = PIKE_T_UNKNOWN; _from->u.refs = (void *) -1); \
    PIKE_MEM_WO(*_from);						\
  } while (0)

extern struct svalue dest_ob_zero;

#define free_mixed_svalues(X,Y) do {		\
  struct svalue *s_=(X);			\
  ptrdiff_t num_=(Y);				\
  while(num_--)					\
  {						\
    dmalloc_touch_svalue(s_);			\
    free_svalue(s_++);				\
  }						\
}while(0);

#ifdef DEBUG_MALLOC
#define free_svalues(X,Y,Z) debug_free_svalues((X),(Y),(Z), DMALLOC_LOCATION());
#else
#define free_svalues(X,Y,Z) debug_free_svalues((X),(Y),(Z));
#endif

#define low_clear_svalues(X,Y,N) do {		\
  struct svalue *s_=(X);			\
  ptrdiff_t num_=(Y);				\
  for(;num_-- > 0;s_++)				\
  {						\
      s_->type=PIKE_T_INT;			\
      s_->subtype=(N);				\
      s_->u.integer=0;				\
  }						\
}while(0)

#define clear_svalues(X,Y) low_clear_svalues((X),(Y),NUMBER_NUMBER)
#define clear_svalues_undefined(X,Y) low_clear_svalues((X),(Y),NUMBER_UNDEFINED)

#define really_free_short_svalue(U, TYPE) do {				\
    union anything *any_ = (U);						\
    really_free_short_svalue_ptr (&any_->ptr, (TYPE));			\
  } while (0)

/* Prototypes begin here */
PMOD_EXPORT void really_free_short_svalue_ptr(void **s, TYPE_T type);
PMOD_EXPORT void really_free_svalue(struct svalue *s);
PMOD_EXPORT void do_free_svalue(struct svalue *s);
PMOD_EXPORT void debug_free_svalues(struct svalue *s, size_t num, INT32 type_hint DMALLOC_LINE_ARGS);
PMOD_EXPORT void debug_free_mixed_svalues(struct svalue *s, size_t num, INT32 type_hint DMALLOC_LINE_ARGS);
PMOD_EXPORT void assign_svalues_no_free(struct svalue *to,
			    const struct svalue *from,
			    size_t num,
			    TYPE_FIELD type_hint);
PMOD_EXPORT void assign_svalues(struct svalue *to,
		    const struct svalue *from,
		    size_t num,
		    TYPE_FIELD types);
PMOD_EXPORT void assign_to_short_svalue(union anything *u,
			    TYPE_T type,
			    const struct svalue *s);
PMOD_EXPORT void assign_to_short_svalue_no_free(union anything *u,
				    TYPE_T type,
				    const struct svalue *s);
PMOD_EXPORT void assign_from_short_svalue_no_free(struct svalue *s,
				      const union anything *u,
				      TYPE_T type);
PMOD_EXPORT void assign_short_svalue_no_free(union anything *to,
				 const union anything *from,
				 TYPE_T type);
PMOD_EXPORT void assign_short_svalue(union anything *to,
			 const union anything *from,
			 TYPE_T type);
PMOD_EXPORT unsigned INT32 hash_svalue(const struct svalue *s);
PMOD_EXPORT int svalue_is_true(const struct svalue *s);
PMOD_EXPORT int safe_svalue_is_true(const struct svalue *s);
PMOD_EXPORT int is_identical(const struct svalue *a, const struct svalue *b);
PMOD_EXPORT int is_eq(const struct svalue *a, const struct svalue *b);
PMOD_EXPORT int low_is_equal(const struct svalue *a,
		 const struct svalue *b,
		 struct processing *p);
PMOD_EXPORT int low_short_is_equal(const union anything *a,
		       const union anything *b,
		       TYPE_T type,
		       struct processing *p);
PMOD_EXPORT int is_equal(const struct svalue *a, const struct svalue *b);
PMOD_EXPORT int is_lt(const struct svalue *a, const struct svalue *b);
PMOD_EXPORT void describe_svalue(const struct svalue *s,int indent,struct processing *p);
PMOD_EXPORT void print_svalue (FILE *out, const struct svalue *s);
PMOD_EXPORT void print_svalue_compact (FILE *out, const struct svalue *s);
PMOD_EXPORT void copy_svalues_recursively_no_free(struct svalue *to,
				      const struct svalue *from,
				      size_t num,
				      struct processing *p);
void check_short_svalue(const union anything *u, TYPE_T type);
void debug_check_svalue(const struct svalue *s);
PMOD_EXPORT void real_gc_xmark_svalues(const struct svalue *s, ptrdiff_t num);
PMOD_EXPORT void real_gc_check_svalues(const struct svalue *s, size_t num);
void gc_check_weak_svalues(const struct svalue *s, size_t num);
PMOD_EXPORT void real_gc_check_short_svalue(const union anything *u, TYPE_T type);
void gc_check_weak_short_svalue(const union anything *u, TYPE_T type);
PMOD_EXPORT TYPE_FIELD real_gc_mark_svalues(struct svalue *s, size_t num);
TYPE_FIELD gc_mark_weak_svalues(struct svalue *s, size_t num);
int real_gc_mark_short_svalue(union anything *u, TYPE_T type);
int gc_mark_weak_short_svalue(union anything *u, TYPE_T type);
int gc_mark_without_recurse(struct svalue *s);
int gc_mark_weak_without_recurse(struct svalue *s);
PMOD_EXPORT TYPE_FIELD real_gc_cycle_check_svalues(struct svalue *s, size_t num);
TYPE_FIELD gc_cycle_check_weak_svalues(struct svalue *s, size_t num);
PMOD_EXPORT int real_gc_cycle_check_short_svalue(union anything *u, TYPE_T type);
int gc_cycle_check_weak_short_svalue(union anything *u, TYPE_T type);
void real_gc_free_svalue(struct svalue *s);
void real_gc_free_short_svalue(union anything *u, TYPE_T type);
PMOD_EXPORT INT32 pike_sizeof(const struct svalue *s);
int svalues_are_constant(struct svalue *s,
			 INT32 num,
			 TYPE_FIELD hint,
			 struct processing *p);
/* Prototypes end here */

#define gc_cycle_check_without_recurse gc_mark_without_recurse
#define gc_cycle_check_weak_without_recurse gc_mark_without_recurse

#define gc_xmark_svalues(S,N) do {					\
    size_t num__ = (N);							\
    real_gc_xmark_svalues(dmalloc_check_svalues((S),num__,DMALLOC_LOCATION()),num__); \
  } while (0)
#define gc_check_svalues(S,N) do {					\
    size_t num__ = (N);							\
    real_gc_check_svalues(dmalloc_check_svalues((S),num__,DMALLOC_LOCATION()),num__); \
  } while (0)

#ifdef DEBUG_MALLOC
static inline TYPE_FIELD dmalloc_gc_mark_svalues (struct svalue *s, size_t num, char *l)
  {return real_gc_mark_svalues (dmalloc_check_svalues (s, num, l), num);}
#define gc_mark_svalues(S, NUM) dmalloc_gc_mark_svalues ((S), (NUM), DMALLOC_LOCATION())
static inline TYPE_FIELD dmalloc_gc_cycle_check_svalues (struct svalue *s, size_t num, char *l)
  {return real_gc_cycle_check_svalues (dmalloc_check_svalues (s, num, l), num);}
#define gc_cycle_check_svalues(S, NUM) dmalloc_gc_cycle_check_svalues ((S), (NUM), DMALLOC_LOCATION())
#else
#define gc_mark_svalues real_gc_mark_svalues
#define gc_cycle_check_svalues real_gc_cycle_check_svalues
#endif

#define gc_check_short_svalue(U,T) real_gc_check_short_svalue(dmalloc_check_union((U),(T),DMALLOC_LOCATION()),T)
#define gc_mark_short_svalue(U,T) real_gc_mark_short_svalue(dmalloc_check_union((U),(T),DMALLOC_LOCATION()),T)
#define gc_cycle_check_short_svalue(U,T) real_gc_cycle_check_short_svalue(dmalloc_check_union((U),(T),DMALLOC_LOCATION()),(T))
#define gc_free_svalue(S) real_gc_free_svalue(dmalloc_check_svalue(S,DMALLOC_LOCATION()))
#define gc_free_short_svalue(U,T) real_gc_free_short_svalue(dmalloc_check_union((U),(T),DMALLOC_LOCATION()),(T))

#ifndef NO_PIKE_SHORTHAND

#define T_ARRAY    PIKE_T_ARRAY
#define T_MAPPING  PIKE_T_MAPPING
#define T_MULTISET PIKE_T_MULTISET
#define T_OBJECT   PIKE_T_OBJECT
#define T_FUNCTION PIKE_T_FUNCTION
#define T_PROGRAM  PIKE_T_PROGRAM
#define T_STRING   PIKE_T_STRING
#define T_TYPE     PIKE_T_TYPE
#define T_FLOAT    PIKE_T_FLOAT
#define T_INT      PIKE_T_INT

#define T_ZERO	   PIKE_T_ZERO

#define T_TUPLE	   PIKE_T_TUPLE
#define T_SCOPE	   PIKE_T_SCOPE
#define T_MIXED    PIKE_T_MIXED

#endif /* !NO_PIKE_SHORTHAND */

#if 0 /* PIKE_RUN_UNLOCKED */

#include "pike_error.h"

/*#define swap_svalues swap_svalues*/
/*#define free_svalue free_svalue_unlocked*/
/*#define free_short_svalue free_short_svalue_unlocked */
/*#define add_ref_svalue add_ref_svalue_unlocked*/

/*
 * These don't work right now - Hubbe
 */
#define assign_svalue_no_free assign_svalue_no_free_unlocked
#define assign_svalue assign_svalue_unlocked

/* FIXME:
 * These routines assumes that pointers are 32 bit
 * and svalues 64 bit!!!!! - Hubbe
 */

#ifndef swap_svalues 
#define swap_svalues swap_svalues_unlocked
#endif

#ifndef free_svalue
static inline void free_svalue(struct svalue *s)
{
  INT64 tmp;
  struct svalue zero;
  zero.type=PIKE_T_INT;
  tmp=pike_atomic_swap64((INT64 *)s, *(INT64 *)&zero);
  free_svalue_unlocked((struct svalue *)&tmp);
}
#endif

#ifndef free_short_svalue
static inline void free_short_svalue(union anything *s, INT16 t)
{
  if(t <= MAX_REF_TYPE)
  {
    INT32 tmp;
    tmp=pike_atomic_swap32((INT32 *)s, 0);
    free_short_svalue_unlocked((union anything *)&tmp, t);
  }
}
#endif

#ifndef add_ref_svalue
static inline void add_ref_svalue(struct svalue *s)
{
  INT64 sv;
  sv=pike_atomic_get64((INT64 *)s);
  add_ref_svalue_unlocked((struct svalue *)&sv);
}
#endif

#ifndef assign_svalue_no_free
void assign_svalue_no_free(struct svalue *to, const struct svalue *from)
{
  INT64 tmp, sv;
  sv=pike_atomic_get64((INT64 *)from);
#ifdef PIKE_DEBUG
  if(sv != *(INT64*)from)
  {
    fprintf(stderr,"pike_atomic_get64() is broken %llx != %llx (%08x%08x)!\n",
	    sv,
	    *(INT64*)from,
	    ((INT32*)from)[1], ((INT32*)from)[0]);
    abort();
  }
#endif
  add_ref_svalue_unlocked((struct svalue *)&sv);
  pike_atomic_set64((INT64 *)to, sv);
#ifdef PIKE_DEBUG
  if(*(INT64*)to != *(INT64*)from)
  {
    fprintf(stderr,"pike_atomic_set64() is broken!\n");
    abort();
  }
#endif
}
#endif

#ifndef assign_svalue
static inline void assign_svalue(struct svalue *to, const struct svalue *from)
{
  INT64 tmp, sv;
  if(to != from)
  {
    sv=pike_atomic_get64((INT64 *)from);
    add_ref_svalue_unlocked((struct svalue *)&sv);
    tmp=pike_atomic_swap64((INT64 *)to, sv);
    free_svalue_unlocked((struct svalue *)&tmp);
  }
}
#endif

#else /* FOO_PIKE_RUN_UNLOCKED */
#define swap_svalues swap_svalues
#define free_svalue free_svalue_unlocked
#define free_short_svalue free_short_svalue_unlocked 
#define add_ref_svalue add_ref_svalue_unlocked
#define assign_svalue_no_free assign_svalue_no_free_unlocked
#define assign_svalue assign_svalue_unlocked
#endif /* FOO_PIKE_RUN_UNLOCKED */

#ifdef PIKE_RUN_UNLOCKED
#include "pike_threadlib.h"
#endif

/* 
 * Note to self:
 * It might be better to use a static array of mutexes instead
 * and just lock mutex ptr % array_size instead.
 * That way I wouldn't need a mutex in each memory object,
 * but it would cost a couple of cycles in every lock/unlock
 * operation instead.
 */
#define PIKE_MEMORY_OBJECT_MEMBERS \
  INT32 refs \
  DO_IF_SECURITY(; struct object *prot) \
  IF_LOCAL_MUTEX(; PIKE_MUTEX_T mutex)

#ifdef PIKE_SECURITY
#ifdef USE_LOCAL_MUTEX
#define PIKE_CONSTANT_MEMOBJ_INIT(refs) refs, 0, PTHREAD_MUTEX_INITIALIZER
#else
#define PIKE_CONSTANT_MEMOBJ_INIT(refs) refs, 0
#endif
#else
#ifdef USE_LOCAL_MUTEX
#define PIKE_CONSTANT_MEMOBJ_INIT(refs) refs, PTHREAD_MUTEX_INITIALIZER
#else
#define PIKE_CONSTANT_MEMOBJ_INIT(refs) refs
#endif
#endif

#define INIT_PIKE_MEMOBJ(X) do {			\
  struct ref_dummy *v_=(struct ref_dummy *)(X);		\
  v_->refs=1;						\
  DO_IF_SECURITY( INITIALIZE_PROT(v_) );		\
  IF_LOCAL_MUTEX(mt_init_recursive(&(v_->mutex)));	\
}while(0)

#define EXIT_PIKE_MEMOBJ(X) do {		\
  struct ref_dummy *v_=(struct ref_dummy *)(X);		\
  DO_IF_SECURITY( FREE_PROT(v_) );		\
  IF_LOCAL_MUTEX(mt_destroy(&(v_->mutex)));	\
}while(0)


struct ref_dummy
{
  PIKE_MEMORY_OBJECT_MEMBERS;
};

#endif /* !SVALUE_H */
