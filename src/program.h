/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: program.h,v 1.212 2004/10/22 23:24:50 nilsson Exp $
*/

#ifndef PROGRAM_H
#define PROGRAM_H

#include <stdarg.h>
#include "global.h"
#include "pike_macros.h"
#include "pike_error.h"
#include "svalue.h"
#include "dmalloc.h"
#include "time_stuff.h"
#include "program_id.h"
#include "pike_rusage.h"
#include "block_alloc_h.h"

/* Needed to support dynamic loading on NT */
PMOD_PROTO extern struct program_state * Pike_compiler;

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

#define LFUN___INIT 0
#define LFUN_CREATE 1
#define LFUN_DESTROY 2
#define LFUN_ADD 3
#define LFUN_SUBTRACT 4
#define LFUN_AND 5
#define LFUN_OR 6
#define LFUN_XOR 7
#define LFUN_LSH 8
#define LFUN_RSH 9
#define LFUN_MULTIPLY 10
#define LFUN_DIVIDE 11
#define LFUN_MOD 12
#define LFUN_COMPL 13
#define LFUN_EQ 14
#define LFUN_LT 15
#define LFUN_GT 16
#define LFUN___HASH 17
#define LFUN_CAST 18
#define LFUN_NOT 19
#define LFUN_INDEX 20
#define LFUN_ASSIGN_INDEX 21
#define LFUN_ARROW 22
#define LFUN_ASSIGN_ARROW 23
#define LFUN__SIZEOF 24
#define LFUN__INDICES 25
#define LFUN__VALUES 26
#define LFUN_CALL 27
#define LFUN_RADD 28
#define LFUN_RSUBTRACT 29
#define LFUN_RAND 30
#define LFUN_ROR 31
#define LFUN_RXOR 32
#define LFUN_RLSH 33
#define LFUN_RRSH 34
#define LFUN_RMULTIPLY 35
#define LFUN_RDIVIDE 36
#define LFUN_RMOD 37
#define LFUN_ADD_EQ 38
#define LFUN__IS_TYPE 39
#define LFUN__SPRINTF 40
#define LFUN__EQUAL 41
#define LFUN__M_DELETE 42
#define LFUN__GET_ITERATOR 43

#define NUM_LFUNS 44

/* NOTE: After this point there are only fake lfuns.
 *       ie use low_find_lfun(), and NOT FIND_LFUN()!
 */
#define LFUN__SEARCH 44

extern const char *const lfun_names[];

extern struct pike_string *lfun_strings[];

#ifndef STRUCT_NODE_S_DECLARED
#define STRUCT_NODE_S_DECLARED
struct node_s;
typedef struct node_s node;
#endif

#ifndef STRUCT_OBJECT_DECLARED
#define STRUCT_OBJECT_DECLARED
struct object;
#endif

#define STRUCT
#include "compilation.h"

#define EXTERN
#include "compilation.h"

/* Byte-code method identification. */
#define PIKE_BYTECODE_PORTABLE	-1	/* Only used by the codec. */
#define PIKE_BYTECODE_DEFAULT	0
#define PIKE_BYTECODE_GOTO	1
#define PIKE_BYTECODE_SPARC	2
#define PIKE_BYTECODE_IA32	3
#define PIKE_BYTECODE_PPC32     4

#if PIKE_BYTECODE_METHOD == PIKE_BYTECODE_IA32
#define PIKE_OPCODE_T unsigned INT8
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_SPARC
#define PIKE_OPCODE_T unsigned INT32
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_PPC32
#define PIKE_OPCODE_T unsigned INT32
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_GOTO
#define PIKE_OPCODE_T void *
#define PIKE_INSTR_T void *
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
  /* C function pointer. */
  void (*c_fun)(INT32);

  /* For variables: Offset of the variable in the storage pointed to
   * by inherit.storage_offset in the struct inherit that corresponds
   * to the identifier. See LOW_GET_GLOBAL and GET_GLOBAL. The stored
   * variable may be either a normal or a short svalue, depending on
   * identifier.run_time_type.
   *
   * For constants: Offset of the struct program_constant in
   * program.constants in the program pointed to by prog in the struct
   * inherit that corresponds to the identifier.
   *
   * For pike functions: Offset to the start of the function in
   * program.program in the program pointed to by prog in the struct
   * inherit that corresponds to the identifier. Or -1 if a prototype.
   */
  ptrdiff_t offset;
};

#define IDENTIFIER_VARIABLE 0
#define IDENTIFIER_PIKE_FUNCTION 1
#define IDENTIFIER_C_FUNCTION 2
#define IDENTIFIER_FUNCTION 3
#define IDENTIFIER_CONSTANT 4
#define IDENTIFIER_TYPE_MASK 7

#define IDENTIFIER_VARARGS 8	/* Used for functions only. */
#define IDENTIFIER_NO_THIS_REF 8 /* Used for variables only: Don't count refs to self. */
#define IDENTIFIER_HAS_BODY 16  /* Function has a body (set already in pass 1). */
#define IDENTIFIER_SCOPED 32   /* This is used for local functions only */
#define IDENTIFIER_SCOPE_USED 64 /* contains scoped local functions */
#define IDENTIFIER_ALIAS 128   /* This identifier is an alias. */

#define IDENTIFIER_IS_FUNCTION(X) ((X) & IDENTIFIER_FUNCTION)
#define IDENTIFIER_IS_PIKE_FUNCTION(X) ((X) & IDENTIFIER_PIKE_FUNCTION)
#define IDENTIFIER_IS_C_FUNCTION(X) ((X) & IDENTIFIER_C_FUNCTION)
#define IDENTIFIER_IS_CONSTANT(X) ((X) & IDENTIFIER_CONSTANT)
#define IDENTIFIER_IS_VARIABLE(X) (!((X) & IDENTIFIER_TYPE_MASK))
#define IDENTIFIER_IS_ALIAS(X)	((X) & IDENTIFIER_ALIAS)

#define IDENTIFIER_MASK 255

/*
 * Every constant, class, function and variable
 * gets exactly one of these.
 */
struct identifier
{
  struct pike_string *name;
  struct pike_type *type;
  unsigned INT8 identifier_flags;	/* IDENTIFIER_??? */
  unsigned INT8 run_time_type;		/* PIKE_T_??? */
  unsigned INT16 opt_flags;		/* OPT_??? */
#ifdef PROFILING
  unsigned INT32 num_calls;		/* Total number of calls. */
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

#define ID_STATIC          0x01	/* Symbol is not visible by indexing */
#define ID_PRIVATE         0x02	/* Symbol is not visible by inherit */
#define ID_NOMASK          0x04	/* Symbol may not be overloaded */
#define ID_PUBLIC          0x08 /* Anti private */
#define ID_PROTECTED       0x10 /* Not currently used at all */
#define ID_INLINE          0x20 /* Same as local */
#define ID_HIDDEN          0x40	/* Symbols that are private and inherited one step later */
#define ID_INHERITED       0x80 /* Symbol is inherited */
#define ID_OPTIONAL       0x100	/* Symbol is not required by the interface */
#define ID_EXTERN         0x200	/* Symbol is defined later */
#define ID_VARIANT	  0x400 /* Function is overloaded by argument. */
#define ID_ALIAS	  0x800 /* Variable is an overloaded alias. */

#define ID_MODIFIER_MASK 0x0fff

#define ID_STRICT_TYPES  0x8000	/* #pragma strict_types */
#define ID_SAVE_PARENT  0x10000 /* #pragma save_parent */
#define ID_DONT_SAVE_PARENT 0x20000 /* #pragma dont_save_parent */


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

  /* ID_* flags - static, private etc.. */
  INT16 id_flags;
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

  /* All the identifier references in the inherited program has been
   * copied to this program with the first one at this offset. */
  INT16 identifier_level;

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
};


/*
 * Storage struct for a trampoline object
 * (not a part of the program type)
 */
struct pike_trampoline
{
  struct pike_frame *frame;
  INT32 func;
};



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

/* Objects created from this program are constant and shareable */
#define PROGRAM_CONSTANT 0x40

/* Objects have pointers to the parent object. Use LOW_PARENT_INFO or
 * PARENT_INFO to extract it. */
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
 * parent object. Only set if PROGRAM_USES_PARENT is. */
#define PROGRAM_NEEDS_PARENT 0x1000

/* Indicates that the class is a facet or product_class. */
#define PROGRAM_IS_FACET_CLASS 0x1
#define PROGRAM_IS_PRODUCT_CLASS 0x2

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

struct program
{
  PIKE_MEMORY_OBJECT_MEMBERS; /* Must be first */

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
  
  INT16 lfuns[NUM_LFUNS];

  /* Facet related stuff */
  INT16 facet_class;   /* PROGRAM_IS_X_CLASS (X=FACET/PRODUCT) */
  INT32 facet_index;   /* Index to the facet this facet class belongs to */
  struct object *facet_group;
};

#if 0
static INLINE int CHECK_IDREF_RANGE (int x, const struct program *p)
{
  if (x < 0 || x >= p->num_identifier_references)
    debug_fatal ("Identifier reference index %d out of range 0..%d\n", x,
		 p->num_identifier_references - 1);
  return x;
}
#else
#define CHECK_IDREF_RANGE(X, P) (X)
#endif

#define PTR_FROM_INT(P, X)	((P)->identifier_references + \
				 CHECK_IDREF_RANGE((X), (P)))

#define INHERIT_FROM_PTR(P,X) (dmalloc_touch(struct program *,(P))->inherits + (X)->inherit_offset)
#define PROG_FROM_PTR(P,X) (dmalloc_touch(struct program *,INHERIT_FROM_PTR(P,X)->prog))
#define ID_FROM_PTR(P,X) (PROG_FROM_PTR(P,X)->identifiers+(X)->identifier_offset)
#define INHERIT_FROM_INT(P,X) INHERIT_FROM_PTR(P, PTR_FROM_INT(P, X))
#define PROG_FROM_INT(P,X) PROG_FROM_PTR(P, PTR_FROM_INT(P, X))
#define ID_FROM_INT(P,X) ID_FROM_PTR(P, PTR_FROM_INT(P, X))

#define FIND_LFUN(P,N) ( dmalloc_touch(struct program *,(P))->flags & PROGRAM_FIXED?((P)->lfuns[(N)]):low_find_lfun((P), (N)) )
#define QUICK_FIND_LFUN(P,N) (dmalloc_touch(struct program *,(P))->lfuns[N])

#ifdef DO_PIKE_CLEANUP
extern int gc_external_refs_zapped;
void gc_check_zapped (void *a, TYPE_T type, const char *file, int line);
#endif

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

BLOCK_ALLOC_FILL_PAGES(program, n/a)


extern struct object *error_handler;
extern struct object *compat_handler;

extern struct program *first_program;
extern struct program *null_program;
extern struct program *pike_trampoline_program;
extern struct program *gc_internal_program;
extern struct program *placeholder_program;
extern struct object *placeholder_object;

extern int compilation_depth;

/* Flags for identifier finding... */
#define SEE_STATIC 1
#define SEE_PRIVATE 2


#define COMPILER_IN_CATCH 1

#define ADD_STORAGE(X) low_add_storage(sizeof(X), ALIGNOF(X),0)
#define STORAGE_NEEDED(X) ((X)->storage_needed - (X)->inherits[0].storage_offset)

#define FOO(NUMTYPE,TYPE,ARGTYPE,NAME) void PIKE_CONCAT(add_to_,NAME(ARGTYPE ARG));
#include "program_areas.h"

typedef int supporter_callback (void *, int);
struct Supporter
{
#ifdef PIKE_DEBUG
  int magic;
#endif
  struct Supporter *previous;
  struct Supporter *depends_on;
  struct Supporter *dependants;
  struct Supporter *next_dependant;
  supporter_callback *fun;
  void *data;
  struct program *prog;
};



/* Prototypes begin here */
void ins_int(INT32 i, void (*func)(char tmp));
void ins_short(int i, void (*func)(char tmp));
void add_relocated_int_to_program(INT32 i);
void use_module(struct svalue *s);
void unuse_modules(INT32 howmany);
node *find_module_identifier(struct pike_string *ident,
			     int see_inherit);
node *resolve_identifier(struct pike_string *ident);
node *program_magic_identifier (struct program_state *state,
				int state_depth, int inherit_num,
				struct pike_string *ident,
				int colon_colon_ref);
struct program *parent_compilation(int level);
struct program *id_to_program(INT32 id);
void optimize_program(struct program *p);
void fsort_program_identifier_index(unsigned short *start,
				    unsigned short *end,
				    struct program *p);
struct pike_string *find_program_name(struct program *p, INT32 *line);
int override_identifier (struct reference *ref, struct pike_string *name);
void fixate_program(void);
struct program *low_allocate_program(void);
void low_start_new_program(struct program *p,
			   int pass,
			   struct pike_string *name,
			   int flags,
			   int *idp);
PMOD_EXPORT void debug_start_new_program(int line, const char *file);
void dump_program_desc(struct program *p);
int sizeof_variable(int run_time_type);
void dump_program_tables (struct program *p, int indent);
void check_program(struct program *p);
struct program *end_first_pass(int finish);
PMOD_EXPORT struct program *debug_end_program(void);
PMOD_EXPORT size_t low_add_storage(size_t size, size_t alignment,
				   ptrdiff_t modulo_orig);
PMOD_EXPORT void set_init_callback(void (*init_callback)(struct object *));
PMOD_EXPORT void set_exit_callback(void (*exit_callback)(struct object *));
PMOD_EXPORT void set_gc_recurse_callback(void (*m)(struct object *));
PMOD_EXPORT void set_gc_check_callback(void (*m)(struct object *));
void pike_set_prog_event_callback(void (*cb)(int));
void pike_set_prog_optimize_callback(node *(*opt)(node *));
int really_low_reference_inherited_identifier(struct program_state *q,
					      int e,
					      int i);
int low_reference_inherited_identifier(struct program_state *q,
				       int e,
				       struct pike_string *name,
				       int flags);
int find_inherit(struct program *p, struct pike_string *name);
node *reference_inherited_identifier(struct pike_string *super_name,
				     struct pike_string *function_name);
void rename_last_inherit(struct pike_string *n);
void low_inherit(struct program *p,
		 struct object *parent,
		 int parent_identifier,
		 int parent_offset,
		 INT32 flags,
		 struct pike_string *name);
PMOD_EXPORT void do_inherit(struct svalue *s,
		INT32 flags,
		struct pike_string *name);
void compiler_do_inherit(node *n, INT32 flags, struct pike_string *name);
int call_handle_inherit(struct pike_string *s);
void simple_do_inherit(struct pike_string *s,
		       INT32 flags,
		       struct pike_string *name);
int isidentifier(struct pike_string *s);
int low_define_variable(struct pike_string *name,
			struct pike_type *type,
			INT32 flags,
			size_t offset,
			INT32 run_time_type);
PMOD_EXPORT int map_variable(const char *name,
		 const char *type,
		 INT32 flags,
		 size_t offset,
		 INT32 run_time_type);
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
PMOD_EXPORT int simple_add_variable(const char *name,
			const char *type,
			INT32 flags);
PMOD_EXPORT int add_constant(struct pike_string *name,
		 struct svalue *c,
		 INT32 flags);
PMOD_EXPORT int simple_add_constant(const char *name,
			struct svalue *c,
			INT32 flags);
PMOD_EXPORT int add_integer_constant(const char *name,
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
INT32 define_function(struct pike_string *name,
		      struct pike_type *type,
		      unsigned flags,
		      unsigned function_flags,
		      union idptr *func,
		      unsigned opt_flags);
int really_low_find_shared_string_identifier(struct pike_string *name,
					     struct program *prog,
					     int flags);
int low_find_lfun(struct program *p, ptrdiff_t lfun);
int lfun_lookup_id(struct pike_string *lfun_name);
int low_find_shared_string_identifier(struct pike_string *name,
				      struct program *prog);
struct ff_hash;
int find_shared_string_identifier(struct pike_string *name,
				  struct program *prog);
PMOD_EXPORT int find_identifier(const char *name,struct program *prog);
int store_prog_string(struct pike_string *str);
int store_constant(struct svalue *foo,
		   int equal,
		   struct pike_string *constant_name);
struct array *program_indices(struct program *p);
struct array *program_values(struct program *p);
void program_index_no_free(struct svalue *to, struct program *p,
			   struct svalue *ind);
int get_small_number(char **q);
void ext_store_program_line (struct program *prog, INT32 line, struct pike_string *file);
void start_line_numbering(void);
void store_linenumber(INT32 current_line, struct pike_string *current_file);
PMOD_EXPORT struct pike_string *low_get_program_line(struct program *prog,
						     INT32 *linep);
PMOD_EXPORT struct pike_string *get_program_line(struct program *prog,
						 INT32 *linep);
PMOD_EXPORT char *low_get_program_line_plain (struct program *prog, INT32 *linep,
					      int malloced);
PMOD_EXPORT struct pike_string *low_get_line(PIKE_OPCODE_T *pc,
					     struct program *prog, INT32 *linep);
PMOD_EXPORT char *low_get_line_plain (PIKE_OPCODE_T *pc, struct program *prog,
				      INT32 *linep, int malloced);
PMOD_EXPORT struct pike_string *get_line(PIKE_OPCODE_T *pc,
					 struct program *prog, INT32 *linep);
PMOD_EXPORT struct pike_string *low_get_function_line (struct object *o,
						       int fun, INT32 *linep);
void va_yyerror(const char *fmt, va_list args);
void my_yyerror(const char *fmt,...);
struct pike_string *format_exception_for_error_msg (struct svalue *thrown);
void handle_compile_exception (const char *yyerror_fmt, ...);
struct supporter_marker;
void verify_supporters(void);
void init_supporter(struct Supporter *s,
		    supporter_callback *fun,
		    void *data);
int unlink_current_supporter(struct Supporter *c);
int call_dependants(struct Supporter *s, int finish);
int report_compiler_dependency(struct program *p);
struct compilation;
void run_pass2(struct compilation *c);
struct program *compile(struct pike_string *aprog,
			struct object *ahandler,
			int amajor, int aminor,
			struct program *atarget,
			struct object *aplaceholder);
PMOD_EXPORT int pike_add_function2(const char *name, void (*cfun)(INT32),
				   const char *type, unsigned flags,
				   unsigned opt_flags);
PMOD_EXPORT int quick_add_function(const char *name,
				   int name_length,
				   void (*cfun)(INT32),
				   const char *type,
				   int type_length,
				   unsigned flags,
				   unsigned opt_flags);
void check_all_programs(void);
void placeholder_index(INT32 args);
void init_program(void);
void cleanup_program(void);
void gc_mark_program_as_referenced(struct program *p);
void real_gc_cycle_check_program(struct program *p, int weak);
unsigned gc_touch_all_programs(void);
void gc_check_all_programs(void);
void gc_mark_all_programs(void);
void gc_cycle_check_all_programs(void);
void gc_zap_ext_weak_refs_in_programs(void);
size_t gc_free_all_unreferenced_programs(void);
void push_compiler_frame(int lexical_scope);
void low_pop_local_variables(int level);
void pop_local_variables(int level);
void pop_compiler_frame(void);
ptrdiff_t low_get_storage(struct program *o, struct program *p);
PMOD_EXPORT char *get_storage(struct object *o, struct program *p);
struct program *low_program_from_function(struct program *p,
					  INT32 i);
PMOD_EXPORT struct program *program_from_function(const struct svalue *f);
PMOD_EXPORT struct program *program_from_svalue(const struct svalue *s);
struct find_child_cache_s;
int find_child(struct program *parent, struct program *child);
void yywarning(char *fmt, ...);
struct implements_cache_s;
PMOD_EXPORT int implements(struct program *a, struct program *b);
PMOD_EXPORT int is_compatible(struct program *a, struct program *b);
void yyexplain_not_compatible(struct program *a, struct program *b, int flags);
void yyexplain_not_implements(struct program *a, struct program *b, int flags);
PMOD_EXPORT void *parent_storage(int depth);
PMOD_EXPORT void change_compiler_compatibility(int major, int minor);
void make_program_executable(struct program *p);
/* Prototypes end here */

void really_free_program(struct program *);
void count_memory_in_programs(INT32*,INT32*);

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

#define pike_add_function(NAME, CFUN, TYPE, FLAGS)	\
  pike_add_function2(NAME, CFUN, TYPE, FLAGS,		\
                     OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND)

#ifndef NO_PIKE_SHORTHAND
#define add_function pike_add_function
#endif

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

#endif /* PROGRAM_H */

/* Kludge... */
#ifndef LAS_H
/* FIXME: Needed for the OPT_??? macros.
 * Maybe they should be moved here, since las.h includes this file anyway?
 */
#include "las.h"
#endif /* !LAS_H */
