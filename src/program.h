/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef PROGRAM_H
#define PROGRAM_H

#include "global.h"
#include "pike_error.h"
#include "pike_rusage.h"
#include "svalue.h"
#include "time_stuff.h"
#include "program_id.h"
#include "block_allocator.h"
#include "string_builder.h"
#include "gc_header.h"

#include "enum_Pike_opcodes.h"

/* Needed to support dynamic loading on NT */
PMOD_EXPORT extern struct program_state * Pike_compiler;

/* Compilation flags */
#define COMPILATION_CHECK_FINAL         0x01
        /* This flag is set when resolve functions should force the lookup so
         * that we don't get a placeholder back. Used for inherits. */
#define COMPILATION_FORCE_RESOLVE       0x02

/* #define FORCE_RESOLVE_DEBUG */
/* Helper macros for force_resolve */
#ifdef FORCE_RESOLVE_DEBUG
#define DO_IF_FRD(X)  X
#else /* !FORCE_RESOLVE_DEBUG */
#define DO_IF_FRD(X)
#endif /* FORCE_RESOLVE_DEBUG */
#define SET_FORCE_RESOLVE(OLD) do {					\
    int tmp_ = (OLD) = Pike_compiler->flags;				\
    Pike_compiler->flags |= COMPILATION_FORCE_RESOLVE;			\
    DO_IF_FRD(fprintf(stderr,						\
		      "Force resolve on. Flags:0x%04x (0x%04x)\n",      \
		      Pike_compiler->flags, tmp_));                     \
  } while(0)
#define UNSET_FORCE_RESOLVE(OLD) do {					\
    int tmp_ = (Pike_compiler->flags & ~COMPILATION_FORCE_RESOLVE) |	\
      ((OLD) & COMPILATION_FORCE_RESOLVE);				\
    DO_IF_FRD(fprintf(stderr,						\
		      "Force resolve unset. Flags:0x%04x (0x%04x)\n",	\
		      tmp_, Pike_compiler->flags));                     \
    Pike_compiler->flags = tmp_;					\
  } while(0)

extern struct pike_string *this_program_string;
extern struct pike_string *this_string;
extern struct pike_string *args_string;
extern struct pike_string *this_function_string;
extern struct pike_string *predef_scope_string;

extern struct pike_string *Concurrent_Promise_string;
extern struct pike_string *success_string;

/* Common compiler subsystems */
extern struct pike_string *parser_system_string;
extern struct pike_string *type_check_system_string;

/**
 * New LFUN lookup table.
 *
 * The lfun table is an array of INT16, where the first
 * 10 (ie number of groups) entries contain offsets into
 * the same array where the entries for the corresponding
 * group start. An offset of 0 (zero) indicates that the
 * group is empty. Group #0 is always present even if empty.
 *
 * Each group may currently contain at most 16 members. This is
 * due to the way they are encoded in the LFUN_* constants.
 *
 * The groups partition the set of lfuns into subsets of
 * related lfuns that are likely to be implemented together.
 *
 * The most significant nybble of an lfun number contains
 * the group number, and the least significant nybble the
 * offset in the group.
 *
 * Note that the values in this enum MUST match the
 * corresponding tables in program.c.
 */
enum LFUN {
    /* Group 0 */
    LFUN___INIT 		= 0x00,
    LFUN_CREATE,
    LFUN__DESTRUCT,
    LFUN__SPRINTF,
    LFUN___CREATE__,
    LFUN___GENERIC_TYPES__,
    LFUN___GENERIC_BINDINGS__,

    /* Group 1 */
    LFUN_ADD 			= 0x10,
    LFUN_SUBTRACT,
    LFUN_MULTIPLY,
    LFUN_DIVIDE,
    LFUN_MOD,
    LFUN_AND,
    LFUN_OR,
    LFUN_XOR,
    LFUN_LSH,
    LFUN_RSH,
    LFUN_POW,

    /* Group 2 */
    LFUN_RADD			= 0x20,
    LFUN_RSUBTRACT,
    LFUN_RMULTIPLY,
    LFUN_RDIVIDE,
    LFUN_RMOD,
    LFUN_RAND,
    LFUN_ROR,
    LFUN_RXOR,
    LFUN_RLSH,
    LFUN_RRSH,
    LFUN_RPOW,

    /* Group 3 */
    LFUN_COMPL			= 0x30,
    LFUN_NOT,
    LFUN_CALL,
    LFUN_CAST,
    LFUN___HASH,
    LFUN__SQRT,
    LFUN__RANDOM,
    LFUN__REVERSE,

    /* Group 4 */
    LFUN_EQ			= 0x40,
    LFUN_LT,
    LFUN_GT,
    LFUN__EQUAL,
    LFUN__IS_TYPE,

    /* Group 5 */
    LFUN_INDEX			= 0x50,
    LFUN_ARROW,
    LFUN_RANGE,
    LFUN__SEARCH,
    LFUN__SIZE_OBJECT,

    /* Group 6 */
    LFUN__SIZEOF		= 0x60,
    LFUN__INDICES,
    LFUN__VALUES,
    LFUN__TYPES,
    LFUN__ANNOTATIONS,
    LFUN__GET_ITERATOR,

    /* Group 7 */
    LFUN_ADD_EQ			= 0x70,
    LFUN_ASSIGN_INDEX,
    LFUN_ASSIGN_ARROW,
    LFUN__M_DELETE,
    LFUN__M_CLEAR,
    LFUN__M_ADD,
    LFUN__ATOMIC_GET_SET,

    /* Group 8 */
    LFUN__SERIALIZE		= 0x80,
    LFUN__DESERIALIZE,

    /* Group 9 */
    LFUN__ITERATOR_NEXT_FUN	= 0x90,
    LFUN__ITERATOR_INDEX_FUN,
    LFUN__ITERATOR_VALUE_FUN,

    NUM_LFUNS,
};

extern const char *const lfun_names[];

extern struct pike_string *lfun_strings[];
extern struct pike_string *lfun_compat_strings[];

#ifndef STRUCT_NODE_S_DECLARED
#define STRUCT_NODE_S_DECLARED
struct node_s;
typedef struct node_s node;
#endif

#undef EXTERN
#undef STRUCT
#undef PUSH
#undef POP
#undef DECLARE

#define STRUCT
#include "compilation.h"

#define EXTERN
#include "compilation.h"

/* Byte-code method identification. */
#define PIKE_BYTECODE_PORTABLE	-1	/* Only used by the codec. */
#define PIKE_BYTECODE_DEFAULT	0
#define PIKE_BYTECODE_GOTO	1       /* Not in use */
#define PIKE_BYTECODE_SPARC	2
#define PIKE_BYTECODE_IA32	3
#define PIKE_BYTECODE_PPC32    4
#define PIKE_BYTECODE_AMD64    5
#define PIKE_BYTECODE_PPC64    6
#define PIKE_BYTECODE_ARM32    7
#define PIKE_BYTECODE_ARM64    8
#define PIKE_BYTECODE_RV32     9
#define PIKE_BYTECODE_RV64     10

#ifndef PIKE_BYTECODE_METHOD
#error PIKE_BYTECODE_METHOD not set.
#endif

#if PIKE_BYTECODE_METHOD == PIKE_BYTECODE_IA32
#define PIKE_OPCODE_T unsigned INT8
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_AMD64
#define PIKE_OPCODE_T unsigned INT8
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_SPARC
#define PIKE_OPCODE_T unsigned INT32
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_PPC32
#define PIKE_OPCODE_T unsigned INT32
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_PPC64
#define PIKE_OPCODE_T unsigned INT32
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_ARM32
#define PIKE_OPCODE_T unsigned INT32
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_ARM64
#define PIKE_OPCODE_T unsigned INT32
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_RV32
#define PIKE_OPCODE_T unsigned INT16
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_RV64
#define PIKE_OPCODE_T unsigned INT16
#else
#define PIKE_OPCODE_T unsigned INT8
#endif

#ifndef PIKE_INSTR_T
/* The type for an opcode instruction identifier (not packed). In all
 * cases but PIKE_BYTECODE_GOTO, this is n - F_OFFSET where n is the
 * number in the Pike_opcodes enum. */
#define PIKE_INSTR_T unsigned int
#endif

/* I need:
 * a) one type that can point to a callable function.
 *    (C function, or object->fun)
 * This can for instance be an svalue.
 *
 * b) one type that once the object/program is known can point
 *    to the C/PIKE function body.
 *
 * c) A number of flags to send to 'add_simul_efun' to specify side effects
 *    and such.
 */


/*
 * Max program dimensions:
 * 2^16 functions + global variables
 * 2^16 inherits
 * 2^16 arguments to pike functions
 * 2^32 efuns
 * 2^8 local variables (and arguments)
 */

union idptr
{
  /* For variables: Offset of the variable in the storage pointed to
   * by inherit.storage_offset in the struct inherit that corresponds
   * to the identifier. See LOW_GET_GLOBAL and GET_GLOBAL. The stored
   * variable may be either a normal or a short svalue, depending on
   * identifier.run_time_type. (IDENTIFIER_VARIABLE)
   *
   * For pike functions: Offset to the start of the function in
   * program.program in the program pointed to by prog in the struct
   * inherit that corresponds to the identifier. Or -1 if a prototype.
   * (IDENTIFIER_PIKE_FUNCTION)
   */
  ptrdiff_t offset;

  /* External symbol reference. (IDENTIFIER_EXTERN)
   *
   * Note that this bit MUST be checked to be zero
   * before looking at the other four cases!
   */
  struct {
    unsigned short depth;	/* Scope count. */
    unsigned short id;		/* Reference number. */
  } ext_ref;

  /* Constant. (IDENTIFIER_CONSTANT)
   *
   * Offset of the struct program_constant in program.constants
   * in the program pointed to by prog in the struct inherit
   * that corresponds to the identifier.
   */
  struct {
    ptrdiff_t offset;	/* Offset in the constants table. */
  } const_info;

  /* Getter/setter reference pair. (IDENTIFIER_VARIABLE && PIKE_T_GET_SET)
   */
  struct {
    INT16 getter;		/* Reference to getter. */
    INT16 setter;		/* Reference to setter. */
  } gs_info;

  /* C function pointer. (IDENTIFIER_C_FUNCTION) */
  void (*c_fun)(INT32);

  /* Direct svalue pointer. Only used in the reference vtable cache. */
  struct svalue *sval;
};

#define IDENTIFIER_VARIABLE 0
#define IDENTIFIER_CONSTANT 1
#define IDENTIFIER_C_FUNCTION 2
#define IDENTIFIER_PIKE_FUNCTION 3
#define IDENTIFIER_FUNCTION 2
#define IDENTIFIER_TYPE_MASK 3

#define IDENTIFIER_ALIAS 4  /* Identifier is an alias for another
			     * (possibly extern) symbol.
			     */
#define IDENTIFIER_VARARGS 8	/* Used for functions only. */
#define IDENTIFIER_NO_THIS_REF 8 /* Used for variables only: Don't count refs to self. */
#define IDENTIFIER_HAS_BODY 16  /* Function has a body (set already in pass 1). */
#define IDENTIFIER_WEAK	16	/* Used for variables only: Weak reference. */
#define IDENTIFIER_SCOPED 32   /* This is used for local functions only */
#define IDENTIFIER_SCOPE_USED 64 /* contains scoped local functions */
#define IDENTIFIER_COPY 128	/* Idenitifer copied from other program. */

#define IDENTIFIER_IS_FUNCTION(X) ((X) & IDENTIFIER_FUNCTION)
#define IDENTIFIER_IS_PIKE_FUNCTION(X) (((X) & IDENTIFIER_TYPE_MASK) == IDENTIFIER_PIKE_FUNCTION)
#define IDENTIFIER_IS_C_FUNCTION(X) (((X) & IDENTIFIER_TYPE_MASK) == IDENTIFIER_C_FUNCTION)
#define IDENTIFIER_IS_CONSTANT(X) (((X) & IDENTIFIER_TYPE_MASK) == IDENTIFIER_CONSTANT)
#define IDENTIFIER_IS_VARIABLE(X) (!((X) & IDENTIFIER_TYPE_MASK))
#define IDENTIFIER_IS_ALIAS(X)	((X) & IDENTIFIER_ALIAS)
#define IDENTIFIER_IS_SCOPED(X) ((X) & IDENTIFIER_SCOPED)

#define IDENTIFIER_MASK 255

/*
 * Every constant, class, function and variable
 * gets exactly one of these.
 */
struct identifier
{
  struct pike_string *name;
  struct pike_type *type;
  INT_TYPE linenumber;
  unsigned INT32 filename_strno;	/* Index in strings. */
  unsigned INT8 identifier_flags;	/* IDENTIFIER_??? */
  unsigned INT8 run_time_type;		/* PIKE_T_??? */
  unsigned INT16 opt_flags;		/* OPT_??? */
#ifdef PROFILING
  unsigned INT32 num_calls;		/* Total number of calls. */
  unsigned INT32 recur_depth;		/* Recursion depth during timing. */
  cpu_time_t total_time;		/* Total time with children. */
  cpu_time_t self_time;			/* Total time excluding children. */
#endif /* PROFILING */
  union idptr func;
};

/*
 * This is used to store constants, both
 * inline constants and those defined explicitly with
 * the constant keyword.
 */
struct program_constant
{
  struct svalue sval;	/* Value. */
  ptrdiff_t offset;	/* Offset in identifiers to initialization function. */
};

/*
 * in the bytecode, a function starts with:
 * char num_args
 * char num_locals
 * char code[]
 */

#define ID_PROTECTED       0x01 /* Symbol is not visible by indexing */
#define ID_STATIC          0x01	/* Symbol is not visible by indexing */
#define ID_PRIVATE         0x02	/* Symbol is not visible by inherit */
#define ID_FINAL           0x04	/* Symbol may not be overloaded */
#define ID_NOMASK          0x04	/* Symbol may not be overloaded (deprecated) */
#define ID_PUBLIC          0x08 /* Anti private */
#define ID_USED		   0x10 /* This identifier has been used. Check
				 * that the type is compatible when
				 * overloading. */
#define ID_LOCAL           0x20 /* Locally referenced symbol (not virtual) */
#define ID_INLINE          0x20 /* Same as local */
#define ID_HIDDEN          0x40	/* Symbols that are private and inherited one step later */
#define ID_INHERITED       0x80 /* Symbol is inherited */
#define ID_OPTIONAL       0x100	/* Symbol is not required by the interface */
#define ID_EXTERN         0x200	/* Symbol is defined later */
#define ID_VARIANT	  0x400 /* Function is overloaded by argument. */
#define ID_WEAK		  0x800 /* Variable has weak references. */
#define ID_GENERATOR	 0x1000	/* Function is a generator. */
#define ID_ASYNC	 0x2000	/* Function is an async function. */

#define ID_MODIFIER_MASK 0x2fff

#define ID_STRICT_TYPES             0x8000 /* #pragma strict_types */
#define ID_SAVE_PARENT             0x10000 /* #pragma save_parent */
#define ID_DONT_SAVE_PARENT        0x20000 /* #pragma dont_save_parent */
#define ID_NO_DEPRECATION_WARNINGS 0x40000 /* #pragma no_deprecation_warnings */
#define ID_DISASSEMBLE             0x80000 /* #pragma disassemble */
#define ID_DYNAMIC_DOT            0x100000 /* #pragma dynamic_dot */
#define ID_COMPILER_TRACE	  0x200000 /* #pragma compiler_trace */
#define ID_NO_EXPERIMENTAL_WARNINGS 0x400000 /* #pragma no_experimental_warnings */


/*
 * All identifiers in this program
 * and all identifiers in inherited programs
 * need to have a 'struct reference' in this
 * program. When we overload a function, we simply
 * change the reference to point to the new 'struct identifier'.
 *
 * When an identifier is represented as an integer, it's typically the
 * offset of the corresponding struct reference in
 * program.identifier_references.
 */
struct reference
{
  /* Offset of the struct inherit in program.inherits for the program
   * that the struct identifier is in. See INHERIT_FROM_PTR and
   * INHERIT_FROM_INT. */
  unsigned INT16 inherit_offset;

  /* Offset of the struct identifier in program.identifiers in the
   * program pointed to by the struct inherit through inherit_offset
   * above. See ID_FROM_PTR and ID_FROM_INT. */
  unsigned INT16 identifier_offset;

  /* ID_* flags - protected, private etc.. */
  unsigned INT16 id_flags;

  /* V-table cache information from this point on. */

  /* Run-time type for the value field if initialized.
   *   PIKE_T_UNKNOWN indicates uninitialized.
   */
  INT16 run_time_type;

  /* Cached value of the lookup.
   */
  union idptr func;
};

/* Magic value used as identifier reference integer to refer to this. */
#define IDREF_MAGIC_THIS -1

/* Magic values in inherit.parent_offset; see below. */
#define OBJECT_PARENT -18
#define INHERIT_PARENT -17

/*
 * Each program has an array of these,
 * the first entry points to itself, the
 * rest are from inherited programs.
 * Note that when a program is inherited,
 * all 'struct inherit' from that program are
 * copied, so the whole tree of inherits is
 * represented.
 */
struct inherit
{
  /* The depth of the inherited program in this program. I.e. the
   * number of times the program has been inherited, directly or
   * indirectly.
   *
   * Note that the struct inherit for the program that directly
   * inherited the program represented by this struct inherit can be
   * found by going backwards in program.inherits from this struct
   * until one is found with an inherit_level less than this one. */
  INT16 inherit_level;

  /* All the identifier references in the inherited program have been
   * copied to this program with the first one at this offset. */
  INT16 identifier_level;
  /* TODO: why is this signed. */

  /* The index of the identifier reference in the parent program for
   * the identifier from which this inherit was done. -1 if there's no
   * such thing. It's always -1 in the inherit struct for the top
   * level program. */
  INT16 parent_identifier;

  /* Describes how to find the parent object for the external
   * identifier references associated with this inherit:
   *
   * OBJECT_PARENT: Follow the object parent, providing
   *   PROGRAM_USES_PARENT is set in the program containing this
   *   inherit. See PARENT_INFO. This is used for external references
   *   in the top level program (i.e. the one containing the inherit
   *   table).
   *
   * INHERIT_PARENT: Follow the parent pointer in this inherit. This
   *   is used when finished programs with parent objects are
   *   inherited.
   *
   * A non-negative integer: The parent is found by following this
   *   number of parent pointers in the program that directly
   *   inherited the program in this inherit, i.e. in the closest
   *   lower level inherit. This is used when a program is inherited
   *   whose parent is still being compiled, so it's parent object is
   *   fake. That implies that that program also contains the current
   *   program on some level, and that level is stored here. An
   *   example:
   *
   *  	 class A {
   *  	   class B {}
   *  	   class C {
   *  	     class D {
   *  	       inherit B;
   *  	     }
   *  	   }
   *  	 }
   *
   *   The parent program of B is A, which is still being compiled
   *   when B is inherited in D, so it has a fake object. A is also
   *   the parent of D, but two levels out, and hence 2 is stored in
   *   parent_offset.
   *
   *   Note that parent_offset can be 0:
   *
   *     class A {
   *       class B {}
   *       inherit B;
   *     }
   */
  INT16 parent_offset;

  /* The offset of the first entry in prog->identifier_references that
   * comes from the program in this inherit. prog is in this case the
   * program that directly inherited it, and not the top level
   * program. I.e. for inherits on level 1, this is always the same as
   * identifier_level.
   *
   * Are both really necessary? /mast */
  size_t identifier_ref_offset;

  /* Offset in object->storage to the start of the storage for the
   * variables from the program in this inherit. */
  ptrdiff_t storage_offset;

  /* The parent object for the program in this inherit, or NULL if
   * there isn't any. */
  struct object *parent;

  /* The program for this inherit. */
  struct program *prog;

  /* The name of the inherit, if there is any. For nested inherits,
   * this can be a string on the form "A::B::C". */
  struct pike_string *name;

  /* The program that holds the identifiers.
   *
   * Typically the same as prog, but is changed when inheriting
   * classes that have generics.
   *
   * NB: NOT reference-counted as it is always the prog of one of
   *     the inherits.
   *
   * NB: This together with identifiers_offset make an indirect pointer
   *     to identifiers_prog->identifiers + identifiers_offset. This
   *     needs to be indirect due to otherwise needing to keep track
   *     of any pointers when reallocating identifiers_prog->identifiers.
   *
   * See also ID_FROM_PTR/ID_FROM_INT.
   */
  struct program *identifiers_prog;

  /* Offset in identifier_prog->identifiers where the identifiers
   * for this program are held.
   *
   * Typically zero, but is changed when inheriting classes that
   * have generics.
   */
  unsigned int identifiers_offset;

  /* The set of annotations for this inherit (if any).
   *
   * For inherit #0 these are set either directly via
   * annotations on the class, or via inherit of classes
   * that have annotations that are annotated as @Inherited.
   *
   * For inherits at level 1 these are set from the annotations
   * on the corresponding inherit declaration, together with
   * the original annotations that haven't been annotated as
   * @Inherited.
   *
   * For inherits at higher levels they are copied verbatim from their
   * previous program.
   */
  struct multiset *annotations;
};

/**
 * Special inherit references.
 *
 * These are used by find_inherited_identifier().
 */
#define INHERIT_SELF	0	/* Self. */
#define INHERIT_LOCAL	-1	/* Self and not overrideable (force local). */
#define INHERIT_GLOBAL	-2	/* Self and overrideable. */
#define INHERIT_ALL	-3	/* All inherits but not self. */
#define INHERIT_GENERATOR -4	/* Only for continue::this_function. */


/* PROGRAM_* flags. Currently INT16. */

/* program parts have been realloced into one block */
#define PROGRAM_OPTIMIZED 1

/* program has gone through pass 1 of compiler, prototypes etc. will
 * not change from now on
 */
#define PROGRAM_FIXED 2

/* Program is done and can be cloned */
#define PROGRAM_FINISHED 4

/* Program has gone through first compiler pass */
#define PROGRAM_PASS_1_DONE 8

/* Program will be destructed as soon at it runs out of references.
 * Normally only used for mutex lock keys and similar
 */
#define PROGRAM_DESTRUCT_IMMEDIATE 0x10

/* Self explanatory, automatically detected */
#define PROGRAM_HAS_C_METHODS 0x20

/* Objects created from this program are immutable and shareable. */
#define PROGRAM_CONSTANT 0x40

/* Objects have pointers to the parent object. Use LOW_PARENT_INFO or
 * PARENT_INFO to extract it.
 * Note that this is also set by #pragma save_parent. */
#define PROGRAM_USES_PARENT 0x80

/* Objects should not be destructed even when they only have weak
 * references left. */
#define PROGRAM_NO_WEAK_FREE 0x100

/* Objects should not be destructed by f_destruct(). */
#define PROGRAM_NO_EXPLICIT_DESTRUCT 0x200

/* Program is in an inconsistant state */
#define PROGRAM_AVOID_CHECK 0x400

/* Program has not yet been used for compilation */
#define PROGRAM_VIRGIN 0x800

/* Don't allow the program to be inherited or cloned if there's no
 * parent object. May only be set if PROGRAM_USES_PARENT is.
 * Set this if the parent pointer is actually used by the program. */
#define PROGRAM_NEEDS_PARENT 0x1000

/* Program has code to be executed when it's destructed. This causes
 * the gc to consider objects of this program to be "live" and will
 * take care to ensure a good destruct sequence so that other
 * referenced things are intact as far as possible when the object is
 * destructed. See the blurb near the top of gc.c for further
 * discussion.
 *
 * This flag gets set automatically when an event handler that might
 * act on PROG_EVENT_EXIT is installed (i.e. through set_exit_callback
 * or pike_set_prog_event_callback). However, if the handler is very
 * simple then this flag may be cleared again to allow the gc to
 * handle objects of this program more efficiently (by considering
 * them "dead" and destroy them in an arbitrary order).
 *
 * Such a "very simple" handler should only do things like freeing
 * pointers and clearing variables in the object storage. It can not
 * assume that any other context is intact when it's called. It might
 * even get called after the module exit function, which means that
 * this optimization trick can never be used in a dynamically loaded
 * module. */
#define PROGRAM_LIVE_OBJ 0x2000

/* Clear the object storage on destruct. */
#define PROGRAM_CLEAR_STORAGE	0x4000

/* Using define instead of enum allows for ifdefs - Hubbe */
#define PROG_EVENT_INIT 0
#define PROG_EVENT_EXIT 1
#define PROG_EVENT_GC_RECURSE 2
#define PROG_EVENT_GC_CHECK 3
#define NUM_PROG_EVENTS 4

/* These macros should only be used if (p->flags & PROGRAM_USES_PARENT)
 * is true
 */
#define LOW_PARENT_INFO(O,P) ((struct parent_info *)(PIKE_OBJ_STORAGE((O)) + (P)->parent_info_storage))
#define PARENT_INFO(O) LOW_PARENT_INFO( (O), (O)->prog)

/*
 * Objects which needs to access their parent
 * have to allocate one of these structs in
 * the object data area.
 * The parent_info_storage member of the program
 * struct tells us where in the object to find this
 * data.
 */
struct parent_info
{
  struct object *parent;
  INT16 parent_identifier;
};

/* Single entry cache for object indexing. */
struct identifier_lookup_cache
{
  INT32 program_id;
  INT32 identifier_id;
};

struct program
{
  GC_MARKER_MEMBERS;
  INT32 id;             /* used to identify program in caches */

  /* storage_needed - storage needed in object struct
   * the first inherit[0].storage_offset bytes are not used and are
   * subtracted when inheriting.
   */
  ptrdiff_t storage_needed; /* storage needed in the object struct */
  ptrdiff_t xstorage; /* Non-inherited storage */
  ptrdiff_t parent_info_storage;

  INT16 flags;          /* PROGRAM_* */
  unsigned INT8 alignment_needed;
  unsigned INT8 num_generics;
  /* 4 bytes padding.. */
  struct timeval timestamp;

  struct program *next;
  struct program *prev;
  struct program *parent;

  node *(*optimize)(node *n);

  void (*event_handler)(int);
#ifdef PIKE_DEBUG
  unsigned INT32 checksum;
#endif
#ifdef PROFILING
  unsigned INT32 num_clones;
#endif /* PROFILING */

  size_t total_size;

#define FOO(NUMTYPE,TYPE,ARGTYPE,NAME) TYPE * NAME ;
#include "program_areas.h"

#define FOO(NUMTYPE,TYPE,ARGTYPE,NAME) NUMTYPE PIKE_CONCAT(num_,NAME) ;
#include "program_areas.h"

  INT16 *lfuns;
};

struct local_variable_info
{
  int names[MAX_LOCAL];	/* Offset in strings. */
  int types[MAX_LOCAL];	/* Offset in constants. */
  int num_local;	/* Number of entries in either of the above. */
};

PMOD_EXPORT void dump_program_tables (const struct program *p, int indent);

#ifdef PIKE_DEBUG
PIKE_UNUSED_ATTRIBUTE
static inline unsigned INT16 CHECK_IDREF_RANGE(unsigned INT16 x, const struct program *p) {
  if (x >= p->num_identifier_references) {
    dump_program_tables(p, 4);
    debug_fatal ("Identifier reference index %d out of range 0..%d\n", x,
		 p->num_identifier_references - 1);
  }
  return x;
}
#else /* !PIKE_DEBUG */
PIKE_UNUSED_ATTRIBUTE
static inline unsigned INT16 CHECK_IDREF_RANGE(unsigned INT16 x,
					       const struct program *PIKE_UNUSED(p)) {
  return x;
}
#endif

static inline struct reference *PTR_FROM_INT(const struct program *p, unsigned INT16 x) {
  x = CHECK_IDREF_RANGE(x, p);
  return p->identifier_references + x;
}

static inline struct inherit *INHERIT_FROM_PTR(const struct program *p, const struct reference *x) {
  return dmalloc_touch(struct program *,p)->inherits + x->inherit_offset;
}

static inline struct program *PROG_FROM_PTR(const struct program *p, const struct reference *x) {
  return dmalloc_touch(struct program *, INHERIT_FROM_PTR(p,x)->prog);
}

static inline struct identifier *ID_FROM_PTR(const struct program *p, const struct reference *x) {
  struct inherit *inh = INHERIT_FROM_PTR(p,x);

  return inh->identifiers_prog->identifiers + inh->identifiers_offset +
    x->identifier_offset;
}

static inline struct inherit *INHERIT_FROM_INT(const struct program *p, const unsigned INT16 x) {
  struct reference *ref = PTR_FROM_INT(p, x);
  return INHERIT_FROM_PTR(p, ref);
}

static inline struct program *PROG_FROM_INT(const struct program *p, const unsigned INT16 x) {
  struct reference *ref = PTR_FROM_INT(p, x);
  return PROG_FROM_PTR(p, ref);
}

static inline struct identifier *ID_FROM_INT(const struct program *p, unsigned INT16 x) {
  struct reference *ref = PTR_FROM_INT(p, x);
  return ID_FROM_PTR(p, ref);
}

#define QUICK_FIND_LFUN(P,N) ((P)->lfuns[(N)>>4]?			\
	       (P)->lfuns[(P)->lfuns[(N)>>4] + ((N) & 0x0f)]:-1)

#ifdef DO_PIKE_CLEANUP
PMOD_EXPORT extern int gc_external_refs_zapped;
PMOD_EXPORT void gc_check_zapped (void *a, TYPE_T type, const char *file, INT_TYPE line);
#endif

#if defined (USE_DLL) && defined (DYNAMIC_MODULE)
/* Use the function in modules so we don't have to export the block
 * alloc stuff. */
#define free_program(p) do_free_program(p)
#else
#define free_program(p) do{						\
    struct program *_=(p);						\
    debug_malloc_touch(_);						\
    DO_IF_DEBUG (							\
      DO_IF_PIKE_CLEANUP (						\
	if (gc_external_refs_zapped)					\
	  gc_check_zapped (_, PIKE_T_PROGRAM, __FILE__, __LINE__)));	\
    if(!sub_ref(_))							\
      really_free_program(_);						\
  }while(0)
#endif

ATTRIBUTE((malloc))
PMOD_EXPORT struct program * alloc_program(void);
PMOD_EXPORT void really_free_program(struct program * p);
void count_memory_in_programs(size_t *num, size_t *_size);
void free_all_program_blocks(void);


extern struct program *first_program;
extern struct program *gc_internal_program;

/* Flags for identifier finding... */
#define SEE_STATIC 1
#define SEE_PROTECTED 1
#define SEE_PRIVATE 2

/* Report levels */
#define REPORT_NOTICE	0	/* FYI. */
#define REPORT_WARNING	1	/* Compiler warning. */
#define REPORT_ERROR	2	/* Compilation error. */
#define REPORT_FATAL	3	/* Unrecoverable error. */
#define REPORT_MASK	0x0f	/* Mask for the above. */
#define REPORT_FORCE	0x10	/* Report regardless of compiler pass. */


#define COMPILER_IN_CATCH 1

#define ADD_STORAGE(X) low_add_storage(sizeof(X), ALIGNOF(X),0)

#define STORAGE_NEEDED(X) ((X)->storage_needed - (X)->inherits[0].storage_offset)

#define FOO(NUMTYPE,TYPE,ARGTYPE,NAME) \
  void PIKE_CONCAT(add_to_,NAME)(ARGTYPE ARG);
#define BAR(NUMTYPE,TYPE,ARGTYPE,NAME) \
  void PIKE_CONCAT(low_add_many_to_,NAME)(struct program_state *state, ARGTYPE *ARG, NUMTYPE cnt); \
  void PIKE_CONCAT(add_to_,NAME)(ARGTYPE ARG);
#include "program_areas.h"

/* Prototypes begin here */
PMOD_EXPORT void do_free_program (struct program *p);
void ins_int(INT32 i, void (*func)(unsigned char tmp));
void use_module(struct svalue *s);
void unuse_modules(INT32 howmany);
node *find_module_identifier(struct pike_string *ident,
			     int see_inherit);
node *find_predef_identifier(struct pike_string *ident);
int low_resolve_identifier(struct pike_string *ident);
node *resolve_identifier(struct pike_string *ident);
PMOD_EXPORT struct program *resolve_program(struct pike_string *ident);
node *find_inherited_identifier(struct program_state *inherit_state,
				int inherit_depth, int inh,
				struct pike_string *ident);
node *program_magic_identifier (struct program_state *state,
				int state_depth, int inherit_num,
				struct pike_string *ident,
				int colon_colon_ref);
struct program *parent_compilation(int level);
struct program *low_id_to_program(INT32 id, int inhibit_module_load);
struct program *id_to_program(INT32 id);
void optimize_program(struct program *p);
void fsort_program_identifier_index(unsigned short *start,
				    unsigned short *end,
				    struct program *p);
struct pike_string *find_program_name(struct program *p, INT_TYPE *line);
int override_identifier (struct reference *ref, struct pike_string *name,
			 int required_flags);
void fixate_program(void);
struct program *low_allocate_program(void);
void low_start_new_program(struct program *p,
			   int pass,
			   struct pike_string *name,
			   int flags,
			   int *idp);
PMOD_EXPORT void debug_start_new_program(INT_TYPE line, const char *file);
void dump_program_desc(struct program *p);
int sizeof_variable(int run_time_type);
void check_program(struct program *p);
int low_is_variant_dispatcher(struct identifier *id);
int is_variant_dispatcher(struct program *prog, int fun);
PMOD_EXPORT void add_annotation(int id, struct svalue *sval);
void compiler_add_annotations(int id);
PMOD_EXPORT void add_program_annotation(int id, struct svalue *sval);
void compiler_add_program_annotations(int id, node *annotations);
struct program *end_first_pass(int finish);
PMOD_EXPORT struct program *debug_end_program(void);
PMOD_EXPORT size_t low_add_storage(size_t size, size_t alignment,
				   ptrdiff_t modulo_orig);
size_t add_xstorage(size_t size,
		    size_t alignment,
		    ptrdiff_t modulo_orig);
PMOD_EXPORT void set_init_callback(void (*init_callback)(struct object *));
PMOD_EXPORT void set_exit_callback(void (*exit_callback)(struct object *));
PMOD_EXPORT void set_gc_recurse_callback(void (*m)(struct object *));
PMOD_EXPORT void set_gc_check_callback(void (*m)(struct object *));
PMOD_EXPORT void pike_set_prog_event_callback(void (*cb)(int));
PMOD_EXPORT void pike_set_prog_optimize_callback(node *(*opt)(node *));
PMOD_EXPORT int really_low_reference_inherited_identifier(struct program_state *q,
							  int i,
							  int f);
PMOD_EXPORT int low_reference_inherited_identifier(struct program_state *q,
						   int e,
                                                   struct pike_string *name,
						   int flags);
int find_inherit(const struct program *p, const struct pike_string *name);
PMOD_EXPORT int find_inherited_prog(const struct program *p,
				    const struct program *wanted);
PMOD_EXPORT int find_inherited_fun(const struct program *p, int inherit_number,
				   const struct program *wanted, int fun);
PMOD_EXPORT int reference_inherited_identifier(struct program_state *state,
					       struct pike_string *inherit,
					       struct pike_string *name);
void rename_last_inherit(struct pike_string *n);
void lower_inherit(struct program *p,
		   struct object *parent,
		   int parent_identifier,
		   int parent_offset,
		   INT32 flags,
                   struct pike_string *name,
                   struct array *bindings);
PMOD_EXPORT void low_inherit(struct program *p,
			     struct object *parent,
			     int parent_identifier,
			     int parent_offset,
			     INT32 flags,
                             struct pike_string *name,
                             struct array *bindings);
PMOD_EXPORT struct program *lexical_inherit(int scope_depth,
					    struct pike_string *symbol,
					    INT32 flags,
                                            int failure_severity_level,
                                            struct array *bindings);
PMOD_EXPORT void do_inherit(struct svalue *s,
                            INT32 flags,
                            struct pike_string *name,
                            struct array *bindings);
void compiler_do_inherit(node *n, INT32 flags, struct pike_string *name,
                         struct array *bindings);
int call_handle_inherit(struct pike_string *s);
void simple_do_inherit(struct pike_string *s,
		       INT32 flags,
		       struct pike_string *name);
int isidentifier(const struct pike_string *s);
int low_define_alias(struct pike_string *name, struct pike_type *type,
		     int flags, int depth, int refno);
PMOD_EXPORT int define_alias(struct pike_string *name, struct pike_type *type,
			     int flags, int depth, int refno);
int is_auto_variable_type( int variable );
int low_define_variable(struct pike_string *name,
			struct pike_type *type,
			INT32 flags,
			size_t offset,
			INT32 run_time_type);
void fix_auto_variable_type( int id, struct pike_type *type );
PMOD_EXPORT int quick_map_variable(const char *name,
		       int name_length,
		       size_t offset,
		       const char *type,
		       int type_length,
		       INT32 run_time_type,
		       INT32 flags);
int define_variable(struct pike_string *name,
		    struct pike_type *type,
		    INT32 flags);
PMOD_EXPORT int add_typed_constant(struct pike_string *name,
                                   struct pike_type *type,
                                   const struct svalue *c,
                                   INT32 flags);
PMOD_EXPORT int add_constant(struct pike_string *name,
		 const struct svalue *c,
		 INT32 flags);
PMOD_EXPORT int simple_add_constant(const char *name,
			struct svalue *c,
			INT32 flags);
PMOD_EXPORT int add_integer_constant(const char *name,
				     INT_ARG_TYPE i,
				     INT32 flags);
PMOD_EXPORT int low_add_integer_constant(struct pike_string *name,
					   INT_ARG_TYPE i,
					   INT32 flags);
PMOD_EXPORT int quick_add_integer_constant(const char *name,
					   int name_length,
					   INT_ARG_TYPE i,
					   INT32 flags);
PMOD_EXPORT int add_float_constant(const char *name,
				   FLOAT_ARG_TYPE f,
				   INT32 flags);
PMOD_EXPORT int quick_add_float_constant(const char *name,
					 int name_length,
					 FLOAT_ARG_TYPE f,
					 INT32 flags);
PMOD_EXPORT int add_string_constant(const char *name,
				    const char *str,
				    INT32 flags);
PMOD_EXPORT int add_program_constant(const char *name,
			 struct program *p,
			 INT32 flags);
PMOD_EXPORT int add_object_constant(const char *name,
			struct object *o,
			INT32 flags);
PMOD_EXPORT int add_function_constant(const char *name, void (*cfun)(INT32), const char * type, int flags);
PMOD_EXPORT int debug_end_class(const char *name, ptrdiff_t namelen, INT32 flags);
int is_lfun_name(struct pike_string *name);
INT32 define_function(struct pike_string *name,
		      struct pike_type *type,
		      unsigned flags,
		      unsigned function_flags,
		      union idptr *func,
		      unsigned opt_flags);
PMOD_EXPORT int really_low_find_shared_string_identifier(const struct pike_string *name,
							 const struct program *prog,
							 int flags);
int really_low_find_variant_identifier(struct pike_string *name,
				       struct program *prog,
				       struct pike_type *type,
				       int start_pos,
				       int flags);
PMOD_EXPORT int low_find_lfun(struct program *p, enum LFUN lfun);
PMOD_EXPORT int find_lfun_fatal(struct program *p, enum LFUN lfun);
int lfun_lookup_id(struct pike_string *lfun_name);
int low_find_shared_string_identifier(const struct pike_string *name,
				      const struct program *prog);
struct ff_hash;
int find_shared_string_identifier(struct pike_string *name,
				  const struct program *prog);
PMOD_EXPORT int find_identifier(const char *name,const struct program *prog);
PMOD_EXPORT int find_identifier_inh(const char *name,
				    const struct program *prog,
				    int inh);
int store_prog_string(struct pike_string *str);
int store_constant(const struct svalue *foo,
		   int equal,
		   struct pike_string *constant_name);
struct array *program_indices(struct program *p);
struct array *program_values(struct program *p);
struct array *program_types(struct program *p);
struct array *program_inherit_annotations(struct program *p);
struct array *program_annotations(struct program *p, int flags);
int low_program_index_no_free(struct svalue *to, struct program *p, int e,
			      struct object *parent, int parent_identifier);
int program_index_no_free(struct svalue *to, struct svalue *what,
			  struct svalue *ind);
INT_TYPE get_small_number(char **q);
void ext_store_program_line (struct program *prog, INT_TYPE line,
			     struct pike_string *file);
void start_line_numbering(void);
int store_linenumber(INT_TYPE current_line, struct pike_string *current_file);
void store_linenumber_local_name(int local_num, int string_num);
void store_linenumber_local_type(int local_num, int constant_num);
void store_linenumber_local_end(int local_num);
PMOD_EXPORT struct pike_string *low_get_program_line(struct program *prog,
						     INT_TYPE *linep);
PMOD_EXPORT struct pike_string *get_program_line(struct program *prog,
						 INT_TYPE *linep);
PMOD_EXPORT char *low_get_program_line_plain (struct program *prog,
					      INT_TYPE *linep,
					      int malloced);
PMOD_EXPORT struct pike_string *low_get_line(PIKE_OPCODE_T *pc,
					     struct program *prog,
					     INT_TYPE *linep,
					     struct local_variable_info *vars);
PMOD_EXPORT char *low_get_line_plain (PIKE_OPCODE_T *pc, struct program *prog,
				      INT_TYPE *linep, int malloced);
PMOD_EXPORT struct pike_string *get_line(PIKE_OPCODE_T *pc,
					 struct program *prog,
					 INT_TYPE *linep);
PMOD_EXPORT struct pike_string *low_get_function_line (struct object *o,
						       int fun,
						       INT_TYPE *linep);
PMOD_EXPORT struct pike_string *get_identifier_line(struct program *p,
						    int fun,
						    INT_TYPE *linep);
struct supporter_marker;
void count_memory_in_supporter_markers(size_t *num, size_t *size);
PMOD_EXPORT int quick_add_function(const char *name, int name_length,
                                   void (*cfun)(INT32),
                                   const char *type,
                                   int type_length,
                                   unsigned flags,
                                   unsigned opt_flags);
void check_all_programs(void);
void init_program(void);
void cleanup_program(void);
PMOD_EXPORT void visit_program (struct program *p, int action, void *extra);
void gc_mark_program_as_referenced(struct program *p);
void real_gc_cycle_check_program(struct program *p, int weak);
unsigned gc_touch_all_programs(void);
void gc_check_all_programs(void);
void gc_mark_all_programs(void);
void gc_cycle_check_all_programs(void);
void gc_zap_ext_weak_refs_in_programs(void);
size_t gc_free_all_unreferenced_programs(void);
PMOD_EXPORT void *get_inherit_storage(struct object *o, int inherit);
PMOD_EXPORT ptrdiff_t low_get_storage(struct program *o, struct program *p);
PMOD_EXPORT void *get_storage(struct object *o, struct program *p);
PMOD_EXPORT struct program *low_program_from_function(struct object *o, INT32 i);
PMOD_EXPORT struct program *program_from_function(const struct svalue *f);
PMOD_EXPORT struct program *program_from_type(const struct pike_type *t);
PMOD_EXPORT struct program *low_program_from_svalue(const struct svalue *s,
						    struct object **parent_obj,
						    int *parent_id);
PMOD_EXPORT struct program *program_from_svalue(const struct svalue *s);
struct find_child_cache_s;
int find_child(struct program *parent, struct program *child);
struct implements_cache_s;
PMOD_EXPORT int implements(struct program *a, struct program *b);
PMOD_EXPORT int is_compatible(struct program *a, struct program *b);
void yyexplain_not_compatible(int severity_level,
			      struct program *a, struct program *b);
void yyexplain_not_implements(int severity_level,
			      struct program *a, struct program *b);
void string_builder_explain_not_compatible(struct string_builder *s,
					   struct program *a,
					   struct program *b);
void string_builder_explain_not_implements(struct string_builder *s,
					   struct program *a,
					   struct program *b);
PMOD_EXPORT struct object *make_promise(void);
PMOD_EXPORT void *parent_storage(int depth, struct program *expected);
PMOD_EXPORT void *get_inherited_storage(int inh, struct program *expected);
void make_area_executable (char *start, size_t len);
void make_program_executable(struct program *p);
PMOD_EXPORT void string_builder_append_pike_opcode(struct string_builder *s,
						   const PIKE_OPCODE_T *addr,
						   enum Pike_opcodes op,
						   int arg1,
						   int arg2);
PMOD_EXPORT void string_builder_append_file_directive(struct string_builder *s,
						      const PIKE_OPCODE_T *addr,
						      const struct pike_string *file);
PMOD_EXPORT void string_builder_append_line_directive(struct string_builder *s,
						      const PIKE_OPCODE_T *addr,
						      INT_TYPE line);
PMOD_EXPORT void string_builder_append_comment(struct string_builder *s,
					       const PIKE_OPCODE_T *addr,
					       const char *comment);
PMOD_EXPORT void add_reverse_symbol(struct pike_string *sym, void *addr);
PMOD_EXPORT void simple_add_reverse_symbol(const char *sym, void *addr);
PMOD_EXPORT void init_reverse_symbol_table();
PMOD_EXPORT struct pike_string *reverse_symbol_lookup(void *addr);
/* Prototypes end here */

/**
 * Look up the given lfun in the given program and returns the
 * function number it has in the program, or -1 if not found.
 */
static inline int PIKE_UNUSED_ATTRIBUTE FIND_LFUN(struct program * p, enum LFUN lfun) {
#ifdef PIKE_DEBUG
    dmalloc_touch(struct program*, p);
    if ((int)lfun < 0) return find_lfun_fatal(p, lfun);
#endif
    if (p->flags & PROGRAM_FIXED && lfun < NUM_LFUNS) {
      return QUICK_FIND_LFUN(p, lfun);
    }
    return low_find_lfun(p, lfun);
}


#ifndef PIKE_USE_MACHINE_CODE
#define make_program_executable(X)
#endif

#define ADD_FUNCTION(NAME, FUNC, TYPE, FLAGS) \
  quick_add_function(NAME, CONSTANT_STRLEN(NAME), FUNC, TYPE,\
                     CONSTANT_STRLEN(TYPE), FLAGS, \
                     OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND)

#define ADD_PROTOTYPE(NAME, TYPE, FLAGS) \
  ADD_FUNCTION(NAME, 0, TYPE, FLAGS)

#define ADD_FUNCTION2(NAME, FUNC, TYPE, FLAGS, OPT_FLAGS) \
  quick_add_function(NAME, CONSTANT_STRLEN(NAME), FUNC, TYPE,\
                     CONSTANT_STRLEN(TYPE), FLAGS, OPT_FLAGS)

#define ADD_PROTOTYPE2(NAME, TYPE, FLAGS, OPT_FLAGS) \
  ADD_FUNCTION2(NAME, 0, TYPE, FLAGS, OPT_FLAGS)

#define ADD_INT_CONSTANT(NAME, CONST, FLAGS) \
  quick_add_integer_constant(NAME, CONSTANT_STRLEN(NAME), CONST, FLAGS)

#define ADD_FLOAT_CONSTANT(NAME, CONST, FLAGS) \
  quick_add_float_constant(NAME, CONSTANT_STRLEN(NAME), CONST, FLAGS)

#define PIKE_MAP_VARIABLE(NAME, OFFSET, TYPE, RTTYPE, FLAGS) \
  quick_map_variable(NAME, CONSTANT_STRLEN(NAME), OFFSET, \
                     TYPE, CONSTANT_STRLEN(TYPE), RTTYPE, FLAGS)

#define ADD_FUNCTION_DTYPE(NAME,FUN,DTYPE,FLAGS) do {		\
  DTYPE_START;							\
  {DTYPE}							\
  {								\
    struct pike_string *_t;					\
    DTYPE_END(_t);						\
    quick_add_function(NAME, CONSTANT_STRLEN(NAME), FUN,	\
                        _t->str, _t->len, FLAGS,		\
                       OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);	\
    free_string(_t);						\
  }								\
} while (0)

#define ADD_INHERIT(PROGRAM, FLAGS) \
  low_inherit((PROGRAM), 0, 0, 0, (FLAGS), 0)

#define START_NEW_PROGRAM_ID(ID) do { \
    start_new_program();  \
    Pike_compiler->new_program->id=PIKE_CONCAT3(PROG_,ID,_ID); \
  }while(0)

#ifdef DEBUG_MALLOC
#define end_program() ((struct program *)debug_malloc_pass(debug_end_program()))
#define end_class(NAME, FLAGS) (debug_malloc_touch(Pike_compiler->new_program), debug_end_class(NAME, CONSTANT_STRLEN(NAME), FLAGS))
#else
#define end_class(NAME,FLAGS) debug_end_class(NAME, CONSTANT_STRLEN(NAME), FLAGS)
#define end_program debug_end_program
#endif


#define start_new_program() debug_start_new_program(__LINE__,__FILE__)

#define visit_program_ref(P, REF_TYPE, EXTRA)			\
  visit_ref (pass_program (P), (REF_TYPE),			\
	     (visit_thing_fn *) &visit_program, (EXTRA))
#define gc_cycle_check_program(X, WEAK) \
  gc_cycle_enqueue((gc_cycle_check_cb *) real_gc_cycle_check_program, (X), (WEAK))

/* This can be used for backwards compatibility
 *  (if added to program.h in Pike 0.6 and Pike 7.0
 * -Hubbe
 */
#define Pike_new_program Pike_compiler->new_program


/* Return true if compat version is equal or less than MAJOR.MINOR */
#define TEST_COMPAT(MAJOR,MINOR) \
  (Pike_compiler->compat_major < (MAJOR) ||  \
    (Pike_compiler->compat_major == (MAJOR) && \
     Pike_compiler->compat_minor <= (MINOR)))

/* Optimizer flags.
 *
 * NB: Only the low 16 bit are used in struct identifier.
 *     The higher flags are only used by the compiler.
 */
#define OPT_OPTIMIZED       0x1    /* has been processed by optimize(),
				    * only used in node_info
				    */
#define OPT_NOT_CONST       0x2    /* isn't constant */
#define OPT_SIDE_EFFECT     0x4    /* has side effects */
#define OPT_ASSIGNMENT      0x8    /* does assignments */
#define OPT_TRY_OPTIMIZE    0x10   /* might be worth optimizing */
#define OPT_EXTERNAL_DEPEND 0x20   /* the value depends on external
				    * influences (such as read_file or so)
				    */
#define OPT_CASE            0x40   /* contains case(s) */
#define OPT_CONTINUE        0x80   /* contains continue(s) */
#define OPT_BREAK           0x100  /* contains break(s) */
#define OPT_RETURN          0x200  /* contains return(s) */
#define OPT_TYPE_NOT_FIXED  0x400  /* type-field might be wrong */
#define OPT_WEAK_TYPE	    0x800  /* don't warn even if strict types */
#define OPT_APPLY           0x1000 /* contains apply */
#define OPT_FLAG_NODE	    0x2000 /* don't optimize away unless the
				    * parent also is optimized away */
#define OPT_SAFE            0x4000 /* Known to not throw error (which normally
				    * isn't counted as side effect). Only used
				    * in tree_info. */

/* This is a statement which got custom break/continue label handling.
 * Set in compiler_frame. Beware: This is not a node flag! -Hubbe */
#define OPT_CUSTOM_LABELS   0x10000

#endif /* PROGRAM_H */
