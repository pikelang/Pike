/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef SVALUE_H
#define SVALUE_H

#include "global.h"
#include "buffer.h"

#ifndef STRUCT_CALLABLE_DECLARED
#define STRUCT_CALLABLE_DECLARED
struct callable;
#endif

#ifndef STRUCT_NODE_S_DECLARED
#define STRUCT_NODE_S_DECLARED
struct node_s;
typedef struct node_s node;
#endif

struct processing
{
  struct processing *next;
  void *pointer_a, *pointer_b;
};

struct ref_dummy
{
  INT32 refs;
};

/** the union of possible types in an svalue.
*/
union anything
{
  INT_TYPE integer;		/**< Union initializations assume this first. */
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
  int identifier;		/**< Used with T_OBJ_INDEX. */
  struct svalue *lval;		/**< Used with T_SVALUE_PTR. */
  void *ptr;
#ifdef DEBUG_MALLOC
  char *loc;			/* Only used for free svalue debugging. */
#endif
};

/* The native types.
 *
 * Note that PIKE_T_INT is zero so that cleared memory
 * is filled with zeroes.
 */
enum PIKE_TYPE {
    PIKE_T_INT=0,
    PIKE_T_FLOAT=1,

    PIKE_T_FUNCTION_ARG = 4,	/* Only used in type strings. */

/* NB: The reference counted types all have bit 3 (8) set. */
    PIKE_T_ARRAY=8,
    PIKE_T_MAPPING=9,
    PIKE_T_MULTISET=10,
    PIKE_T_OBJECT=11,
    PIKE_T_FUNCTION=12,
    PIKE_T_PROGRAM=13,
    PIKE_T_STRING=14,
    PIKE_T_TYPE=15,

/* The types above are valid types in svalues.
 * The following are only used by the internal systems.
 */

/* NB: 6 & 7 below are selected for easy backward compat with Pike 7.8. */
    PIKE_T_ZERO= 6,	/**< Can return 0, but nothing else */
    PIKE_T_UNFINISHED=7,

/*
 * NOTE: The following types may show up on the stack, but are NOT
 *       reference counted there. There they are mainly used for
 *       lvalues. They MUST NOT have a value that has bit 3 (8) set.
 */

    PIKE_T_VOID=       16, /**< Can't return any value. Also used on stack
			    * to fill out the second
			    * svalue on an lvalue when it isn't used. */

    PIKE_T_MANY=       17,

    PIKE_T_INT_UNTYPED= 18, /* Optimization of int type size */


/* Type to put in freed svalues. Only the type field in such svalues
 * is defined. Freeing a PIKE_T_FREE svalue is allowed and does
 * nothing. mark_free_svalue() is preferably used to set this type.
 *
 * Traditionally T_INT has been used for this without setting a proper
 * subtype; if T_INT is to be used then the subtype must be set to
 * NUMBER_NUMBER.
 *
 * PIKE_T_FREE svalues are recorded as BIT_INT in type hint fields.
 */
    PIKE_T_FREE=19,

/** svalue.u.lval points to an svalue. Primarily used in lvalues on
 * stack, but can also occur in arrays containing lvalue pairs.
 */
    PIKE_T_SVALUE_PTR=20,

/** svalue.u.identifer is an identifier index in an object. Primarily
 * used in lvalues on stack, but can also occur in arrays containing
 * lvalue pairs.
 */
    PIKE_T_OBJ_INDEX=21,
    PIKE_T_ARRAY_LVALUE=22,

/* No types above this value should appear on the stack. */
    PIKE_T_STACK_MAX=	PIKE_T_ARRAY_LVALUE,

    PIKE_T_GET_SET= 32,	/* Getter setter.
                         * Only valid in struct identifier */

    /**
     * Values 0x30 - 0x3f reserved for variables.
     * Only 0x30 - 0x39 used currently.
     */
    PIKE_T_VAR_0 = '0',		/* 48 */
    PIKE_T_VAR_1 = '1',
    PIKE_T_VAR_2 = '2',
    PIKE_T_VAR_3 = '3',
    PIKE_T_VAR_4 = '4',
    PIKE_T_VAR_5 = '5',
    PIKE_T_VAR_6 = '6',
    PIKE_T_VAR_7 = '7',
    PIKE_T_VAR_8 = '8',
    PIKE_T_VAR_9 = '9',		/* 57 */

    PIKE_T_GENERIC = 64,
    PIKE_T_BIND = 65,		/* Apply the binding in CAR to CDR. */

    /* Operators. These all have 0x80 in the low 8 bits, and
     * a non-zero function number in the next 8 bits.
     *
     * Bit 15 (0x8000) indicates that cdr is a type-node.
     *
     * Note that this overlaps with the PIKE_T_NO_REF_FLAG,
     * but that should be safe, as operators aren't valid
     * run-time types.
     *
     * Bin 14 (0x4000) indicates that it is an int op.
     */
    PIKE_T_OPERATOR = 0x0080,
    PIKE_T_FIND_LFUN = 0x0180,	/* Look up an lfun in an object type. */
    PIKE_T_GET_RETURN = 0x0280,	/* Get the return type for a function. */
    PIKE_T_THRESHOLD = 0x0380,	/* Coerce type to mixed|void if not UNKNOWN. */

    PIKE_T_APPLY = 0x8180,	/* Apply a function with a single argument. */
    PIKE_T_SET_CAR = 0x8280,	/* Set the CAR of the CAR type to CDR. */
    PIKE_T_SET_CDR = 0x8380,	/* Set the CDR of the CAR type to CDR. */

    PIKE_T_INT_OP_SUB = 0xc080,	/* INT OP `-. */
    PIKE_T_INT_OP_AND = 0xc180,	/* INT OP `&. */
    PIKE_T_INT_OP_XOR = 0xc280,	/* INT OP `^. */
    PIKE_T_INT_OP_RANGE = 0xc380,	/* int(min(car) .. max(cdr)) */
    PIKE_T_INT_OP_MIN = 0xc480,	/* int(MIN(min(car), min(cdr)) .. MIN(max(car), max(cdr))) */
    PIKE_T_INT_OP_MAX = 0xc580,	/* int(MAX(min(car), min(cdr)) .. MAX(max(car), max(cdr))) */
    PIKE_T_INT_OP_MUL = 0xc680,	/* INT OP `*. */
    PIKE_T_INT_OP_DIV = 0xc780,	/* INT OP `/. */

    PIKE_T_TRANSITIVE = 0x00c0,	/* Repeatedly apply a function type. */

/*
 * The following types are only used in compile-time types and
 * as markers in struct identifier.
 */

    PIKE_T_LARRAY = 236,	/* Limited array. Only for serialization. */
    PIKE_T_LSTRING = 237,	/* Limited string. Only for serialization. */

    PIKE_T_ATTRIBUTE = 238,	/* Attribute node. */
    PIKE_T_NSTRING = 239,	/* Narrow string. Only for serialization. */
    PIKE_T_RING = 240,
    PIKE_T_NAME = 241,		/**< Named type. */
    PIKE_T_SCOPE = 243,		/**< Not supported yet */
    PIKE_T_TUPLE = 244,		/**< Not supported yet */
    PIKE_T_ASSIGN = 245,
    PIKE_T_DELETED = 246,

    /** Used to mark an svalue as free and not valid for input
     *  to the svalue free functions.
     *  Cf assert_free_svalue().
     *
     *  Also used as a placeholder for the empty many type in
     *  serialized function types (eg tFunction) (cf tUnknown).
     */
    PIKE_T_UNKNOWN=247,

/* should only be used while compiling */

    PIKE_T_AUTO = 248,

    /* Historic values:
     * T_OBJ_INDEX = 248,	// Renumbered to 21.
     * T_LVALUE = 249,		// Renamed to T_SVALUE_PTR and renumbered to 20.
     * T_ARRAY_LVALUE = 250,	// Renumbered to 22.
     */

    PIKE_T_MIXED = 251,
    PIKE_T_NOT = 253,
    PIKE_T_AND = 254,
    PIKE_T_OR = 255,

/* This flag is only valid in struct reference, and corresponds
 * to struct identifier identifier_flags IDENTIFIER_NO_THIS_REF. */
#define PIKE_T_NO_REF_FLAG	256
#define PIKE_T_NO_REF_OBJECT	(PIKE_T_NO_REF_FLAG|PIKE_T_OBJECT)
#define PIKE_T_NO_REF_FUNCTION	(PIKE_T_NO_REF_FLAG|PIKE_T_FUNCTION)
#define PIKE_T_NO_REF_MIXED	(PIKE_T_NO_REF_FLAG|PIKE_T_MIXED)
#define PIKE_T_NO_REF_INT	(PIKE_T_NO_REF_FLAG|PIKE_T_INT)
#define PIKE_T_NO_REF_FLOAT	(PIKE_T_NO_REF_FLAG|PIKE_T_FLOAT)

/* These are only used together with describe() and friends. */
#define T_STORAGE 10000
#define T_MAPPING_DATA 10001
#define T_PIKE_FRAME 10002
#define T_MULTISET_DATA 10003
#define T_STRUCT_CALLABLE 10004

/* Mask to get rid of flag bits from the type. */
    PIKE_T_MASK = 255,
};

/**
 */
struct svalue
{
  union {
    struct {
      /* NB: Ought to be "enum PIKE_TYPE type:16", but then the
       *     machine code generator can't use the &-operator to
       *     get the field offset. On the other hand the offset
       *     should always be zero, so...
       */
      unsigned short type; /**< the data type, see PIKE_T_... */
      unsigned short subtype; /**< used to store the zero type, among others */
    } t;
    enum PIKE_TYPE named_type:16;

#if PIKE_BYTEORDER == 1234
    ptrdiff_t type_subtype;
#else
    /* For big-endian 64-bit architectures we don't want to require
     * 64-bit constants when setting the type_subtype field.
     */
    INT32 type_subtype;
#if SIZEOF_CHAR_P == 8
    ptrdiff_t pad__;
#endif
#endif
  } tu;
  union anything u; /**< contains the value */
};

#define PIKE_TYPEOF(SVAL)	((SVAL).tu.t.type)
#define PIKE_SUBTYPEOF(SVAL)	((SVAL).tu.t.subtype)
#define TYPEOF(SVAL)		PIKE_TYPEOF(SVAL)
#define SUBTYPEOF(SVAL)		PIKE_SUBTYPEOF(SVAL)

#define SET_SVAL_TYPE(SVAL, TYPE)	(TYPEOF(SVAL) = (TYPE))
#define SET_SVAL_SUBTYPE(SVAL, TYPE)	(SUBTYPEOF(SVAL) = (TYPE))

#if PIKE_BYTEORDER == 1234
#define TYPE_SUBTYPE(X,Y) ((unsigned int)(X)|((unsigned int)(Y)<<16))
#else
#define TYPE_SUBTYPE(X,Y) ((unsigned int)(Y)|((unsigned int)(X)<<16))
#endif

#define SET_SVAL_TYPE_SUBTYPE(SVAL, TYPE, SUBTYPE) \
  ((SVAL).tu.type_subtype = TYPE_SUBTYPE(TYPE,SUBTYPE))
/* Setting the entire type_subtype field is probably faster than
 * just setting the type field.
 */
#define SET_SVAL_TYPE_DC(SVAL, TYPE)	SET_SVAL_TYPE_SUBTYPE(SVAL, TYPE, 0)

#define SET_SVAL(SVAL, TYPE, SUBTYPE, FIELD, EXPR) do { \
    /* Set the type afterwards to avoid a clobbered	\
     * svalue in case EXPR throws. */			\
    struct svalue * __sv_ptr = &( SVAL );		\
    __sv_ptr->u.FIELD = (EXPR);				\
    SET_SVAL_TYPE_SUBTYPE(*__sv_ptr, TYPE, SUBTYPE);	\
  } while(0)

/*
*/
#define INVALIDATE_SVAL(SVAL) SET_SVAL_TYPE_DC(SVAL, 99) /* an invalid type */


/* Macros for generating strings compatible with
 * pike_types.cmod:make_pike_type() et al, as well
 * as encode.c:decode_type().
 *
 * NOTE: The t* macros below currently use the old type encoding
 *       to be compatible with __parse_pike_type() in older
 *       versions of Pike.
 */
#define tArr(VAL) "\000" VAL
#define tArray tArr(tMix)
#define tLArr(LEN, VAL) "\354" LEN VAL
#define tMap(IND,VAL) "\001" IND VAL
#define tMapping tMap(tMix,tMix)
#define tSet(IND) "\002" IND
#define tMultiset tSet(tMix)
#define tObj "\003\000\000\000\000\000"

#define tFuncV(ARGS,REST,RET) MagictFuncV(RET,REST,ARGS)
#define tFunc(ARGS,RET) MagictFunc(RET,ARGS)
#define tFuncArg(ARG,REST) "\014" ARG REST

#define tTuple(T1,T2)		"\364" T1 T2
#define tTriple(T1,T2,T3)	tTuple(T1, tTuple(T2, T3))
#define tQuad(T1,T2,T3,T4)	tTriple(tTuple(T1, T2), T3, T4)

/* These two magic funcions are used to make MSVC++ work
 * even if 'ARGS' is empty.
 */
#define MagictFuncV(RET,REST,ARGS) "\004" ARGS "\021" REST RET
#define MagictFunc(RET,ARGS) tFuncV(ARGS tNone, tVoid, RET)
#define tFunction tFuncV(tNone, tUnknown, tOr(tMix,tVoid))
#define tNone ""
#define tUnknown "\367"
#define tPrg(X) "\005" X
#define tProgram(X) "\005" X
#define tStr "\006"
#define tString "\006"
#define tNStr(T) "\357" T
/* NB: tStr0 is a misnomer as it refers to the length of
 *     the string rather than the content.
 */
#define tStr0 tLStr(tZero, tUnknown)
#define tStr7 tNStr(tInt7bit)
#define tStr8 tNStr(tInt8bit)
#define tStr16 tNStr(tInt16bit)
#define tStr32 "\006"
#define tLStr(LEN, VAL) "\355" LEN VAL
#define tType(T) "\007" T
#define tInt "\022"
#define tInt0 "\010\000\000\000\000\000\000\000\000"
#define tInt1 "\010\000\000\000\001\000\000\000\001"
#define tInt2 "\010\000\000\000\002\000\000\000\002"
#define tInt3 "\010\000\000\000\003\000\000\000\003"
#define tInt4 "\010\000\000\000\004\000\000\000\004"
#define tInt127 "\010\000\000\000\177\000\000\000\177"
#define tInt128 "\010\000\000\000\200\000\000\000\200"
#define tInt254 "\010\000\000\000\376\000\000\000\376"
#define tInt255 "\010\000\000\000\377\000\000\000\377"
#define tInt256 "\010\000\000\001\000\000\000\001\000"
#define tInt376 "\010\000\000\001\170\000\000\001\170"
#define tInt65535 "\010\000\000\377\377\000\000\377\377"
#define tInt65536 "\010\000\001\000\000\000\001\000\000"
#define tInt01 "\010\000\000\000\000\000\000\000\001"
#define tInt02 "\010\000\000\000\000\000\000\000\002"
#define tInt03 "\010\000\000\000\000\000\000\000\003"
#define tInt04 "\010\000\000\000\000\000\000\000\004"
#define tInt05 "\010\000\000\000\000\000\000\000\005"
#define tInt06 "\010\000\000\000\000\000\000\000\006"
#define tInt07 "\010\000\000\000\000\000\000\000\007"
#define tInt08 "\010\000\000\000\000\000\000\000\010"
#define tInt09 "\010\000\000\000\000\000\000\000\011"
#define tInt7bit "\010\000\000\000\000\000\000\000\177"
#define tInt8bit "\010\000\000\000\000\000\000\000\377"
#define tInt16bit "\010\000\000\000\000\000\000\377\377"
#define tIntPos "\010\000\000\000\000\177\377\377\377"
#define tIntNeg "\010\200\000\000\000\000\000\000\000"
#define tInt1Plus "\010\000\000\000\001\177\377\377\377"
#define tInt2Plus "\010\000\000\000\002\177\377\377\377"
#define tIntMinus "\010\200\000\000\000\377\377\377\377"
#define tInt_1 "\010\377\377\377\377\377\377\377\377"
#define tInt_10 "\010\377\377\377\377\000\000\000\000"
#define tInt_11 "\010\377\377\377\377\000\000\000\001"
#define tIntMax "\010\177\377\377\377\177\377\377\377"
#define tIntCharRange(LOW,HIGH) "\010\000\000\000" LOW "\000\000\000" HIGH
#define tIntChar(CHAR) tIntCharRange(CHAR, CHAR)
#define tIntDigits tIntCharRange("0", "9")
#define tIntPlusSign tIntChar("+")
#define tIntMinusSign tIntChar("-")
#define tIntDecimal tIntChar(".")
#define tIntSlash tIntChar("/")
#define tIntExp tIntChar("e")
#define tByte "\010\000\000\000\000\000\000\000\377"
#define tWord "\010\000\000\000\000\000\000\377\377"
#define tFlt "\011"
#define tFloat "\011"

#define tZero "\016"
#define tVoid "\020"
#define tVar(X) #X
#define tSetvar(X,Y) "\365" #X Y
#define tAssign(X,Y) "\365" X Y
#define tScope(X,Y) "\363" #X Y
#define tGeneric(OBJ,X)	"\100" OBJ tVar(X)
#define tBind(ASS, EXPR)	"\101" ASS EXPR
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
#define tIfexists(X, FUN) tIfnot(tFuncV(tNone, tNot(X), tMix), FUN)
#define tAny tOr(tVoid,tMix)
#define tAttr(X,Y) "\356\0" X "\0" Y
#define tName(X,Y) "\361\0" X "\0" Y
#if PIKE_BYTEORDER == 1234
/* Little endian */
#define tAttr1(X,Y) "\356\5" X "\0\0" Y
#define tAttr2(X,Y) "\356\6" X "\0\0\0\0" Y
#define tName1(X,Y) "\361\5" X "\0\0" Y
#define tName2(X,Y) "\361\6" X "\0\0\0\0" Y
#else /* PIKE_BYTEORDER != 1234 */
/* Big endian */
#define tAttr1(X,Y) "\356\1" X "\0\0" Y
#define tAttr2(X,Y) "\356\2" X "\0\0\0\0" Y
#define tName1(X,Y) "\361\1" X "\0\0" Y
#define tName2(X,Y) "\361\2" X "\0\0\0\0" Y
#endif /* PIKE_BYTEORDER == 1234 */

/* Some convenience macros for common attributes. */
#define tSprintfFormat(X)	tAttr("sprintf_format", X)
#define tSprintfArgs(X)		tAttr("sprintf_args", X)
#define tDeprecated(X)		tAttr("deprecated", X)
#define tUtf8Str		tAttr("utf8", tStr8)

#define tGetReturn(X)		"\200\002" X
#define tThreshold(X)		"\200\003" X
#define tApply(FUN, ARG)	"\200\201" FUN ARG
#define tFindLFun(X, LFUN)	"\200\001" X LFUN "\0"
#define tSetCar(X, Y)		"\200\202" X Y
#define tSetCdr(X, Y)		"\200\203" X Y

#define tSubInt(X, Y)		"\200\300" X Y
#define tNegateInt(X)		tSubInt(tInt0, X)
#define tAddInt(X, Y)		tSubInt(X, tNegateInt(Y))
#define tInvertInt(X)		tSubInt(tInt_1, X)
#define tIncInt(X)		tSubInt(X, tInt_1)
#define tDecInt(X)		tSubInt(X, tInt1)

#define tAndInt(X, Y)		"\200\301" X Y
#define tOrInt(X, Y)		tInvertInt(tAndInt(tInvertInt(X), tInvertInt(Y)))
#define tXorInt(X, Y)		"\200\302" X Y

#define tRangeInt(X, Y)		"\200\303" X Y
#define tMinInt(X, Y)		"\200\304" X Y
#define tMaxInt(X, Y)		"\200\305" X Y

#define tMulInt(X, Y)		"\200\306" X Y
#define tDivInt(X, Y)		"\200\307" X Y

#define tTransitive(X, Y)	"\300" X Y

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

/** Used to signify that the type field hasn't been set according to
 * reality. */
#define BIT_UNFINISHED (1 << PIKE_T_UNFINISHED)

/** This is only used in typechecking to signify that this
 * argument may be omitted.
 */
#define BIT_VOID (1 << PIKE_T_VOID)

/** This is used in typechecking to signify that the rest of the
 * arguments have to be of this type.
 */
#define BIT_MANY (1 << PIKE_T_MANY)

#define BIT_NOTHING 0
#define BIT_MIXED 0xff7f
#define BIT_BASIC (BIT_INT|BIT_FLOAT|BIT_STRING|BIT_TYPE)
#define BIT_COMPLEX (BIT_ARRAY|BIT_MULTISET|BIT_OBJECT|BIT_PROGRAM|BIT_MAPPING|BIT_FUNCTION)
#define BIT_CALLABLE (BIT_FUNCTION|BIT_PROGRAM|BIT_ARRAY|BIT_OBJECT)
#define BIT_REF_TYPES (BIT_STRING|BIT_TYPE|BIT_COMPLEX)

/* Max type which contains svalues */
#define MAX_COMPLEX PIKE_T_PROGRAM
/* Min type with ref count */
#define MIN_REF_TYPE PIKE_T_ARRAY
/* Max type handled by svalue primitives */
#define MAX_TYPE PIKE_T_TYPE

#define REFCOUNTED_TYPE(T)	(((T) & ~(MIN_REF_TYPE - 1)) == MIN_REF_TYPE)

/**
 * Returns true if the given type is one of the complex types defined
 * by BIT_COMPLEX.
 */
static inline int IS_COMPLEX_TYPE(enum PIKE_TYPE type) {
  return type <= MAX_TYPE && (1 << type) & BIT_COMPLEX;
}

#define NUMBER_NUMBER 0
#define NUMBER_UNDEFINED 1
#define NUMBER_DESTRUCTED 2

#define FUNCTION_BUILTIN USHRT_MAX

extern PMOD_EXPORT const  struct svalue svalue_undefined, svalue_int_zero;
extern PMOD_EXPORT const  struct svalue svalue_int_one;

/*
 * Storage struct for a trampoline object
 * (not a part of the program type)
 */
struct pike_trampoline
{
  struct pike_frame *frame;
  INT32 func;
};

/* NB: From builtin.cmod. */
extern struct program *pike_trampoline_program;

#define is_gt(a,b) is_lt(b,a)
#define is_ge(a,b) is_le(b,a)

/* SAFE_IS_ZERO is compatible with the old IS_ZERO, but you should
 * consider using UNSAFE_IS_ZERO instead, since exceptions thrown from
 * `! functions will be propagated correctly then. */
#define UNSAFE_IS_ZERO(X) (PIKE_TYPEOF(*(X))==PIKE_T_INT?(X)->u.integer==0:(1<<PIKE_TYPEOF(*(X)))&(BIT_OBJECT|BIT_FUNCTION)?!complex_svalue_is_true(X):0)
#define SAFE_IS_ZERO(X) (PIKE_TYPEOF(*(X))==PIKE_T_INT?(X)->u.integer==0:(1<<PIKE_TYPEOF(*(X)))&(BIT_OBJECT|BIT_FUNCTION)?!safe_svalue_is_true(X):0)

#define IS_UNDEFINED(X) (check_svalue (X), PIKE_TYPEOF(*(X))==PIKE_T_INT&&PIKE_SUBTYPEOF(*(X))==NUMBER_UNDEFINED)

#define IS_DESTRUCTED(X)                                                \
  ((PIKE_TYPEOF(*(X)) == PIKE_T_OBJECT && !(X)->u.object->prog) ||	\
 (PIKE_TYPEOF(*(X)) == PIKE_T_FUNCTION &&				\
  PIKE_SUBTYPEOF(*(X)) != FUNCTION_BUILTIN				\
  && (!(X)->u.object->prog                                              \
      || ((X)->u.object->prog == pike_trampoline_program                \
          && !((struct pike_trampoline *)(X)->u.object->storage)        \
          ->frame->current_object->prog                                 \
          && PIKE_SUBTYPEOF(*(X)) ==					\
	  QUICK_FIND_LFUN(pike_trampoline_program, LFUN_CALL)))))

/* var MUST be a variable!!! */
#define safe_check_destructed(var) do{ \
    if(IS_DESTRUCTED(var))	       \
      var=&svalue_int_zero;	       \
  }while(0)

/* FIXME: Is this actually used for functions? */
#define check_short_destructed(U,T) \
do{ \
  union anything *_u=(U); \
  if(( (1<<(T)) & (BIT_OBJECT | BIT_FUNCTION) ) && \
     _u->object && !_u->object->prog) { \
    free_object(_u->object); \
    _u->object = 0; \
  } \
}while(0)


#define add_ref(X) ((void)((X)->refs++))
#define sub_ref(X) (--(X)->refs > 0)

static inline void safe_add_ref(void *ptr)
{
  if (!ptr) return;
  add_ref((struct ref_dummy *)ptr);
}

#ifdef PIKE_DEBUG
PMOD_EXPORT extern void describe(const void *); /* defined in gc.c */
PMOD_EXPORT extern const char msg_type_error[];
PMOD_EXPORT extern const char msg_assign_svalue_error[];

#define IS_INVALID_TYPE(T) is_invalid_stack_type(T)

#define check_type(T) do {						\
    TYPE_T typ_ = (T);							\
    if (IS_INVALID_TYPE (typ_)) Pike_fatal(msg_type_error, typ_);	\
  } while (0)

#define check_svalue_type(S) do {					\
    const struct svalue *sval_ = (S);					\
    TYPE_T typ_ = PIKE_TYPEOF(*sval_);					\
    if (IS_INVALID_TYPE (typ_)) debug_svalue_type_error (sval_);	\
  } while (0)

#define check_svalue(S) debug_check_svalue(dmalloc_check_svalue(S,DMALLOC_LOCATION()))

void low_thorough_check_short_svalue (const union anything *u, TYPE_T type);
#define thorough_check_short_svalue(U, T) do {				\
    union anything *anyth_ = (U);					\
    TYPE_T typ_ = (T);							\
    check_short_svalue (anyth_, typ_);					\
    if (d_flag <= 50) /* Done directly by check_svalue otherwise. */	\
      if (REFCOUNTED_TYPE(typ_))					\
	low_thorough_check_short_svalue (anyth_, typ_);			\
  } while (0)
#define thorough_check_svalue(S) do {					\
    struct svalue *sval_ = (S);						\
    check_svalue (sval_);						\
    if (d_flag <= 50) /* Done directly by check_svalue otherwise. */	\
      if (REFCOUNTED_TYPE(PIKE_TYPEOF(*sval_)))				\
	low_thorough_check_short_svalue (&sval_->u, PIKE_TYPEOF(*sval_)); \
  } while (0)

void check_short_svalue(const union anything *u, TYPE_T type);
PMOD_EXPORT void debug_svalue_type_error (const struct svalue *s);
PMOD_EXPORT int is_invalid_stack_type(unsigned int t);
PMOD_EXPORT void debug_check_svalue(const struct svalue *s);
void debug_check_type_hint (const struct svalue *svals, size_t num, TYPE_FIELD type_hint);
PMOD_EXPORT void real_gc_mark_external_svalues(const struct svalue *s, ptrdiff_t num,
					       const char *place);

PMOD_EXPORT extern const char msg_sval_obj_wo_refs[];
#define check_refs(S) do {						\
    if(REFCOUNTED_TYPE(PIKE_TYPEOF(*(S))) &&				\
       (!(S)->u.refs || (S)->u.refs[0] < 0)) {				\
   fprintf (stderr, "%s", msg_sval_obj_wo_refs);			\
   describe((S)->u.refs);						\
   Pike_fatal("%s", msg_sval_obj_wo_refs);				\
 } }while(0)

PMOD_EXPORT extern const char msg_ssval_obj_wo_refs[];
#define check_refs2(S,T) do { \
if(REFCOUNTED_TYPE(T) && (S)->refs && (S)->refs[0] <= 0) {\
  fprintf (stderr, "%s", msg_ssval_obj_wo_refs);	   \
  describe((S)->refs);					   \
  Pike_fatal("%s", msg_ssval_obj_wo_refs);		   \
} }while(0)

#define check_type_hint(SVALS, NUM, TYPE_HINT)				\
  debug_check_type_hint ((SVALS), (NUM), (TYPE_HINT))

#ifdef DEBUG_MALLOC
static inline struct svalue PIKE_UNUSED_ATTRIBUTE *dmalloc_check_svalue(struct svalue *s, char *l)
{
  if(s && REFCOUNTED_TYPE(PIKE_TYPEOF(*s)))
    debug_malloc_update_location(s->u.refs,l);
  return s;
}

static inline struct svalue PIKE_UNUSED_ATTRIBUTE *dmalloc_check_svalues(struct svalue *s, size_t num, char *l)
{
  while (num--) dmalloc_check_svalue (s + num, l);
  return s;
}

static inline union anything PIKE_UNUSED_ATTRIBUTE *dmalloc_check_union(union anything *u,int type, char * l)
{
  if(u && REFCOUNTED_TYPE(type))
    debug_malloc_update_location(u->refs,l);
  return u;
}

#undef add_ref
#define add_ref(X) (((INT32 *)debug_malloc_update_location( &((X)->refs), DMALLOC_NAMED_LOCATION(" add_ref")))[0]++)
#undef sub_ref
#define sub_ref(X) (--((INT32 *)debug_malloc_update_location( &((X)->refs), DMALLOC_NAMED_LOCATION(" sub_ref")))[0] > 0)

#else  /* !DEBUG_MALLOC */
#define dmalloc_check_svalue(S,L) (S)
#define dmalloc_check_svalues(S,L,N) (S)
#define dmalloc_check_union(U,T,L) (U)
#endif	/* !DEBUG_MALLOC */

/* To be used for type checking in macros. */
static inline struct array PIKE_UNUSED_ATTRIBUTE *pass_array (struct array *a) {return a;}
static inline struct mapping PIKE_UNUSED_ATTRIBUTE *pass_mapping (struct mapping *m) {return m;}
static inline struct multiset PIKE_UNUSED_ATTRIBUTE *pass_multiset (struct multiset *l) {return l;}
static inline struct object PIKE_UNUSED_ATTRIBUTE *pass_object (struct object *o) {return o;}
static inline struct program PIKE_UNUSED_ATTRIBUTE *pass_program (struct program *p) {return p;}
static inline struct pike_string PIKE_UNUSED_ATTRIBUTE *pass_string (struct pike_string *s) {return s;}
static inline struct pike_type PIKE_UNUSED_ATTRIBUTE *pass_type (struct pike_type *t) {return t;}
static inline struct callable PIKE_UNUSED_ATTRIBUTE *pass_callable (struct callable *c) {return c;}

#else  /* !PIKE_DEBUG */

#define check_svalue(S) ((void) 0)
#define check_short_svalue(U, T) ((void) 0)
#define check_type(T) do {} while (0)
#define check_svalue_type(S) do {} while (0)
#define check_refs(S) do {} while (0)
#define check_refs2(S,T) do {} while (0)
#define check_type_hint(SVALS, NUM, TYPE_HINT) ((void) 0)
#define dmalloc_check_svalue(S,L) (S)
#define dmalloc_check_svalues(S,L,N) (S)
#define dmalloc_check_union(U,T,L) (U)

#define pass_array(A) (A)
#define pass_mapping(M) (M)
#define pass_multiset(L) (L)
#define pass_object(O) (O)
#define pass_program(P) (P)
#define pass_string(S) (S)
#define pass_type(T) (T)
#define pass_callable(C) (C)

#endif	/* !PIKE_DEBUG */

/* This marks an svalue as free. After this it may only be used as
 * input to the svalue free functions (which do nothing with it). Only
 * the type field is defined (see PIKE_T_FREE above). */
#define mark_free_svalue(X) do {					\
    struct svalue *_X__ = (X);						\
    DO_IF_DMALLOC (							\
      _X__->u.loc = " " __FILE__ ":" DEFINETOSTR (__LINE__);		\
      _X__->u.loc++; /* Attempt to achieve an odd address. */		\
    );									\
    PIKE_MEM_WO(*_X__);							\
    SET_SVAL_TYPE(*_X__, PIKE_T_FREE);					\
  } while (0)

/* This is a debug macro to assert that an svalue is free and
 * shouldn't be read at all until it's overwritten. As opposed to
 * mark_free_svalue, it is not valid input to the svalue free
 * functions and no field in it is defined. */
#define assert_free_svalue(X) do {					\
    DO_IF_DEBUG (							\
      struct svalue *_X__ = (X);					\
      SET_SVAL_TYPE(*_X__, PIKE_T_UNKNOWN);				\
      DO_IF_DMALLOC (							\
	_X__->u.loc = " " __FILE__ ":" DEFINETOSTR (__LINE__);		\
	_X__->u.loc++; /* Attempt to achieve an odd address. */		\
      );								\
      DO_IF_NOT_DMALLOC (_X__->u.ptr = (void *) -1);			\
      PIKE_MEM_WO (*_X__);						\
    );									\
  } while (0)

#define swap_svalues(X,Y)  do {		\
  struct svalue *_a=(X);				\
  struct svalue *_b=(Y);				\
  struct svalue _tmp;					\
  dmalloc_touch_svalue(_a);				\
  dmalloc_touch_svalue(_b);				\
  _tmp=*_a; *_a=*_b; *_b=_tmp;				\
}while(0)

/* Handles PIKE_T_FREE. */
#define free_svalue(X) do {				\
  struct svalue *_s=(X);					\
  DO_IF_DEBUG (							\
    if (PIKE_TYPEOF(*_s) != PIKE_T_FREE) {			\
      check_svalue_type(_s);					\
      check_refs(_s);						\
    }								\
  );								\
  if (!REFCOUNTED_TYPE(PIKE_TYPEOF(*_s)))			\
    assert_free_svalue (_s);					\
  else {							\
    DO_IF_DEBUG (						\
      DO_IF_PIKE_CLEANUP (					\
	if (gc_external_refs_zapped)				\
	  gc_check_zapped (_s->u.ptr, PIKE_TYPEOF(*_s), __FILE__, __LINE__))); \
    if (sub_ref(_s->u.dummy) <=0)				\
      really_free_svalue(_s);					\
    else							\
      assert_free_svalue (_s);					\
  }								\
}while(0)

#define free_short_svalue(X,T) do {				\
  union anything *_s=(X); TYPE_T _t=(T);				\
  check_type(_t); check_refs2(_s,_t);					\
  if(REFCOUNTED_TYPE(_t) && _s->refs) {					\
    DO_IF_DEBUG (							\
      DO_IF_PIKE_CLEANUP (						\
	if (gc_external_refs_zapped)					\
	  gc_check_zapped (_s->ptr, _t, __FILE__, __LINE__)));		\
    if(sub_ref(_s->dummy) <= 0) really_free_short_svalue(_s,_t);	\
  }									\
  DO_IF_DMALLOC(_s->refs=(void *)-1);					\
  PIKE_MEM_WO(_s->refs);						\
}while(0)

/* Handles PIKE_T_FREE. */
#define add_ref_svalue(X) do {				\
  struct svalue *_tmp=(X);					\
  DO_IF_DEBUG (							\
    if (PIKE_TYPEOF(*_tmp) != PIKE_T_FREE) {			\
      check_svalue_type(_tmp);					\
      check_refs(_tmp);						\
    }								\
  );								\
  if(REFCOUNTED_TYPE(PIKE_TYPEOF(*_tmp))) add_ref(_tmp->u.dummy);	\
}while(0)

/* Handles PIKE_T_FREE. */
#define assign_svalue_no_free(X,Y) do {	\
  struct svalue *_to=(X);				\
  const struct svalue *_from=(Y);			\
  DO_IF_DEBUG (						\
    if (PIKE_TYPEOF(*_from) != PIKE_T_FREE) {		\
      check_svalue_type(_from);				\
      check_refs(_from);				\
    }							\
    if (_to == _from)					\
      Pike_fatal(msg_assign_svalue_error, _to);		\
  );							\
  *_to=*_from;						\
  if(REFCOUNTED_TYPE(PIKE_TYPEOF(*_to))) add_ref(_to->u.dummy); \
}while(0)

/* Handles PIKE_T_FREE. */
#define assign_svalue(X,Y) do {	\
  struct svalue *_to2=(X);			\
  const struct svalue *_from2=(Y);		\
  if (_to2 != _from2) {				\
    free_svalue(_to2);				\
     assign_svalue_no_free(_to2, _from2);	\
  }						\
}while(0)

/* Handles PIKE_T_FREE. */
#define move_svalue(TO, FROM) do {					\
    struct svalue *_to = (TO);						\
    struct svalue *_from = (FROM);					\
    dmalloc_touch_svalue(_from);					\
    *_to = *_from;							\
    assert_free_svalue (_from);						\
  } while (0)

/* Handles PIKE_T_FREE. */
#define free_mixed_svalues(X,Y) do {		\
  struct svalue *s_=(X);			\
  ptrdiff_t num_=(Y);				\
  while(num_--)					\
  {						\
    dmalloc_touch_svalue(s_);			\
    free_svalue(s_++);				\
  }						\
}while(0)

/* Handles PIKE_T_FREE. */
#ifdef DEBUG_MALLOC
#define free_svalues(X,Y,Z) debug_free_svalues((X),(Y),(Z), DMALLOC_NAMED_LOCATION(" free_svalues"));
#else
#define free_svalues(X,Y,Z) debug_free_svalues((X),(Y),(Z));
#endif

#define low_clear_svalues(X,Y,N) do {		\
  struct svalue *s_=(X);			\
  ptrdiff_t num_=(Y);				\
  for(;num_-- > 0;s_++)				\
  {						\
    SET_SVAL(*s_, PIKE_T_INT, (N), integer, 0);	\
  }						\
}while(0)

#define clear_svalues(X,Y) low_clear_svalues((X),(Y),NUMBER_NUMBER)
#define clear_svalues_undefined(X,Y) low_clear_svalues((X),(Y),NUMBER_UNDEFINED)

#define really_free_short_svalue(U, TYPE) do {				\
    union anything *any_ = (U);						\
    debug_malloc_touch (any_->ptr);					\
    really_free_short_svalue_ptr (&any_->ptr, (TYPE));			\
  } while (0)

/* Prototypes begin here */
PMOD_EXPORT void check_destructed(struct svalue *s);
PMOD_EXPORT void really_free_short_svalue_ptr(void **s, TYPE_T type);
PMOD_EXPORT void really_free_svalue(struct svalue *s);
PMOD_EXPORT void do_free_svalue(struct svalue *s);
PMOD_EXPORT void debug_free_svalues(struct svalue *s, size_t num, INT32 type_hint DMALLOC_LINE_ARGS);
PMOD_EXPORT void debug_free_mixed_svalues(struct svalue *s, size_t num, INT32 type_hint DMALLOC_LINE_ARGS);
PMOD_EXPORT TYPE_FIELD assign_svalues_no_free(struct svalue *to,
					      const struct svalue *from,
					      size_t num,
					      TYPE_FIELD type_hint);
PMOD_EXPORT TYPE_FIELD assign_svalues(struct svalue *to,
				      const struct svalue *from,
				      size_t num,
				      TYPE_FIELD type_hint);
PMOD_EXPORT void assign_no_ref_svalue(struct svalue *to,
				      const struct svalue *val,
				      const struct object *owner);
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
PMOD_EXPORT size_t hash_svalue(const struct svalue *s);
PMOD_EXPORT int complex_svalue_is_true(const struct svalue *s); /* only handles object + function */
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
PMOD_EXPORT int is_le(const struct svalue *a, const struct svalue *b);
PMOD_EXPORT void describe_svalue(struct byte_buffer *buf, const struct svalue *s,int indent,
                                 struct processing *p);
PMOD_EXPORT void safe_describe_svalue(struct byte_buffer *buf, const struct svalue *s,
                                      int indent,struct processing *p);
PMOD_EXPORT void print_svalue (FILE *out, const struct svalue *s);
PMOD_EXPORT void safe_print_svalue (FILE *out, const struct svalue *s);
PMOD_EXPORT void print_short_svalue (FILE *out, const union anything *a, TYPE_T type);
PMOD_EXPORT void safe_print_short_svalue (FILE *out, const union anything *a, TYPE_T type);
PMOD_EXPORT void print_svalue_compact (FILE *out, const struct svalue *s);
PMOD_EXPORT void safe_print_svalue_compact (FILE *out, const struct svalue *s);
PMOD_EXPORT void print_short_svalue_compact (FILE *out, const union anything *a, TYPE_T type);
PMOD_EXPORT void safe_print_short_svalue_compact (FILE *out, const union anything *a, TYPE_T type);
PMOD_EXPORT void pike_vfprintf (FILE *out, const char *fmt, va_list args);
PMOD_EXPORT void pike_fprintf (FILE *out, const char *fmt, ...)
  ATTRIBUTE((format (printf, 2, 3)));
#ifdef PIKE_DEBUG
PMOD_EXPORT void safe_pike_vfprintf (FILE *out, const char *fmt, va_list args);
PMOD_EXPORT void safe_pike_fprintf (FILE *out, const char *fmt, ...)
  ATTRIBUTE((format (printf, 2, 3)));
#else
#define safe_pike_vfprintf		pike_vfprintf
#define safe_pike_fprintf		pike_fprintf
#endif
PMOD_EXPORT void copy_svalues_recursively_no_free(struct svalue *to,
						  const struct svalue *from,
						  size_t num,
						  struct mapping *m);
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
PMOD_EXPORT INT_TYPE pike_sizeof(const struct svalue *s);
int svalues_are_constant(const struct svalue *s,
			 INT32 num,
			 TYPE_FIELD hint,
			 struct processing *p);

static inline TYPE_FIELD PIKE_UNUSED_ATTRIBUTE BITOF(struct svalue sv) {
    if (PIKE_TYPEOF(sv) >= sizeof(TYPE_FIELD) * 8) {
        return BIT_MIXED | BIT_UNFINISHED;
    }

    return 1 << PIKE_TYPEOF(sv);
}

#define gc_cycle_check_without_recurse gc_mark_without_recurse
#define gc_cycle_check_weak_without_recurse gc_mark_without_recurse

#define gc_mark_external_svalues(S, NUM, PLACE) do {			\
    size_t num__ = (NUM);						\
    real_gc_mark_external_svalues (					\
      dmalloc_check_svalues ((S), num__, DMALLOC_LOCATION()), num__, (PLACE)); \
  } while (0)
#define gc_check_svalues(S, NUM) do {					\
    size_t num__ = (NUM);						\
    real_gc_check_svalues (						\
      dmalloc_check_svalues ((S), num__, DMALLOC_LOCATION()), num__);	\
  } while (0)

#ifdef DEBUG_MALLOC
static inline TYPE_FIELD PIKE_UNUSED_ATTRIBUTE dmalloc_gc_mark_svalues (struct svalue *s, size_t num, char *l)
  {return real_gc_mark_svalues (dmalloc_check_svalues (s, num, l), num);}
#define gc_mark_svalues(S, NUM) dmalloc_gc_mark_svalues ((S), (NUM), DMALLOC_LOCATION())
static inline TYPE_FIELD PIKE_UNUSED_ATTRIBUTE dmalloc_gc_cycle_check_svalues (struct svalue *s, size_t num, char *l)
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

#define T_UNFINISHED	PIKE_T_UNFINISHED
#define T_VOID		PIKE_T_VOID
#define T_MANY		PIKE_T_MANY
#define T_SVALUE_PTR	PIKE_T_SVALUE_PTR
#define T_OBJ_INDEX	PIKE_T_OBJ_INDEX
#define T_ARRAY_LVALUE	PIKE_T_ARRAY_LVALUE
#define T_ASSIGN	PIKE_T_ASSIGN
#define T_DELETED	PIKE_T_DELETED
#define T_NOT		PIKE_T_NOT
#define T_AND		PIKE_T_AND
#define T_OR		PIKE_T_OR

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

#define PIKE_CONSTANT_MEMOBJ_INIT(refs, type) GC_HEADER_INIT(refs)

#define INIT_PIKE_MEMOBJ(X, TYPE) do {                  \
  struct ref_dummy *v_=(struct ref_dummy *)(X);         \
  v_->refs=0;                                           \
  add_ref(v_); /* For DMALLOC... */                     \
}while(0)

#define EXIT_PIKE_MEMOBJ(X) do {                        \
  struct ref_dummy *v_=(struct ref_dummy *)(X); 	\
}while(0)

/* The following macro is useful to initialize static svalues. . */
/* assumes sizeof void* >= all initialization arguments. */
#ifdef HAVE_C99_STRUCT_INIT
#define SVALUE_INIT(TYPE, SUBTYPE, VAL) {.tu.type_subtype=TYPE_SUBTYPE(TYPE, SUBTYPE),.u.refs=(void*)VAL}

/* Depends on T_INT and NUMBER_NUMBER being 0. */
#define SVALUE_INIT_INT(VAL)            {.tu.type_subtype=0,.u.integer=VAL}
#else /* !HAVE_C99_STRUCT_INIT */
/* FIXME: Assumes that INT_TYPE and void * have the same size. */
#define SVALUE_INIT(TYPE, SUBTYPE, VAL) {{{TYPE, SUBTYPE}}, {(INT_TYPE)(void*)VAL}}
#define SVALUE_INIT_INT(VAL)		{{{PIKE_T_INT, NUMBER_NUMBER}}, {VAL}}
#endif /* !HAVE_C99_STRUCT_INIT */

#define SVALUE_INIT_FREE                SVALUE_INIT(PIKE_T_FREE,0,0)

#endif /* !SVALUE_H */
