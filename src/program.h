/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef PROGRAM_H
#define PROGRAM_H

#include <stdarg.h>
#include "global.h"
#include "pike_types.h"
#include "svalue.h"

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

#define NUM_LFUNS 38

extern char *lfun_names[];

#ifndef STRUCT_SVALUE_DECLARED
#define STRUCT_SVALUE_DECLARED
struct svalue;
#endif

#ifndef STRUCT_NODE_S_DECLARED
#define STRUCT_NODE_S_DECLARED
struct node_s;
#endif

#ifndef STRUCT_OBJECT_DECLARED
#define STRUCT_OBJECT_DECLARED
struct object;
#endif

struct program_state;

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
  void (*c_fun)(INT32);
  INT32 offset;
};

#define IDENTIFIER_PIKE_FUNCTION 1
#define IDENTIFIER_C_FUNCTION 2
#define IDENTIFIER_FUNCTION 3
#define IDENTIFIER_CONSTANT 4
#define IDENTIFIER_VARARGS 8

#define IDENTIFIER_IS_FUNCTION(X) ((X) & IDENTIFIER_FUNCTION)
#define IDENTIFIER_IS_CONSTANT(X) ((X) & IDENTIFIER_CONSTANT)
#define IDENTIFIER_IS_VARIABLE(X) (!((X) & (IDENTIFIER_FUNCTION | IDENTIFIER_CONSTANT)))

struct identifier
{
  struct pike_string *name;
  struct pike_string *type;
  unsigned INT16 identifier_flags; /* IDENTIFIER_??? */
  unsigned INT16 run_time_type;
#ifdef PROFILING
  unsigned INT32 num_calls;
  unsigned INT32 total_time;
#endif /* PROFILING */
  union idptr func;
};

/*
 * in the bytecode, a function starts with:
 * char num_args
 * char num_locals
 * char code[]
 */

#define ID_STATIC    0x01
#define ID_PRIVATE   0x02
#define ID_NOMASK    0x04
#define ID_PUBLIC    0x08
#define ID_PROTECTED 0x10
#define ID_INLINE    0x20
#define ID_HIDDEN    0x40 /* needed? */
#define ID_INHERITED 0x80

struct reference
{
  unsigned INT16 inherit_offset;
  unsigned INT16 identifier_offset;
  INT16 id_flags; /* static, private etc.. */
};


struct inherit
{
  INT16 inherit_level; /* really needed? */
  INT16 identifier_level;
  INT16 parent_identifier;
  INT16 parent_offset;
  INT32 storage_offset;
  struct object *parent;
  struct program *prog;
  struct pike_string *name;
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

/* Program will be destructed as soon at it runs out of references. */
#define PROGRAM_DESTRUCT_IMMEDIATE 16

struct program
{
  INT32 refs;
  INT32 id;             /* used to identify program in caches */
  INT32 flags;
  INT32 storage_needed; /* storage needed in the object struct */

  struct program *next;
  struct program *prev;

  void (*init)(struct object *);
  void (*exit)(struct object *);
  void (*gc_marked)(struct object *);
#ifdef DEBUG
  unsigned INT32 checksum;
#endif
#ifdef PROFILING
  unsigned INT32 num_clones;
#endif /* PROFILING */

  SIZE_T total_size;

#define FOO(NUMTYPE,TYPE,NAME) TYPE * NAME ;
#include "program_areas.h"

#define FOO(NUMTYPE,TYPE,NAME) NUMTYPE PIKE_CONCAT(num_,NAME) ;
#include "program_areas.h"
  
  INT16 lfuns[NUM_LFUNS];
};

#define INHERIT_FROM_PTR(P,X) ((P)->inherits + (X)->inherit_offset)
#define PROG_FROM_PTR(P,X) (INHERIT_FROM_PTR(P,X)->prog)
#define ID_FROM_PTR(P,X) (PROG_FROM_PTR(P,X)->identifiers+(X)->identifier_offset)
#define INHERIT_FROM_INT(P,X) INHERIT_FROM_PTR(P,(P)->identifier_references+(X))
#define PROG_FROM_INT(P,X) PROG_FROM_PTR(P,(P)->identifier_references+(X))
#define ID_FROM_INT(P,X) ID_FROM_PTR(P,(P)->identifier_references+(X))

#define FIND_LFUN(P,N) ((P)->flags & PROGRAM_FIXED?(P)->lfuns[(N)]:find_identifier(lfun_names[(N)],(P)))

#define free_program(p) do{ struct program *_=(p); debug_malloc_touch(_); if(!--_->refs) really_free_program(_); }while(0)

extern struct object *fake_object;
extern struct program *new_program;
extern struct program *first_program;
extern int compiler_pass;
extern long local_class_counter;
extern int catch_level;

#define COMPILER_IN_CATCH 1

#define FOO(NUMTYPE,TYPE,NAME) void PIKE_CONCAT(add_to_,NAME(TYPE ARG));
#include "program_areas.h"

/* Prototypes begin here */
void ins_int(INT32 i, void (*func)(char tmp));
void ins_short(INT16 i, void (*func)(char tmp));
void use_module(struct svalue *s);
struct node_s *find_module_identifier(struct pike_string *ident);
struct program *parent_compilation(int level);
struct program *id_to_program(INT32 id);
void optimize_program(struct program *p);
void fixate_program(void);
void low_start_new_program(struct program *p,
			   struct pike_string *name,
			   int flags);
void start_new_program(void);
void really_free_program(struct program *p);
void dump_program_desc(struct program *p);
void check_program(struct program *p);
struct program *end_first_pass(int finish);
struct program *debug_end_program(void);
SIZE_T add_storage(SIZE_T size);
void set_init_callback(void (*init)(struct object *));
void set_exit_callback(void (*exit)(struct object *));
void set_gc_mark_callback(void (*m)(struct object *));
int low_reference_inherited_identifier(struct program_state *q,
				       int e,
				       struct pike_string *name);
node *reference_inherited_identifier(struct pike_string *super_name,
				   struct pike_string *function_name);
void rename_last_inherit(struct pike_string *n);
void low_inherit(struct program *p,
		 struct object *parent,
		 int parent_identifier,
		 int parent_offset,
		 INT32 flags,
		 struct pike_string *name);
void do_inherit(struct svalue *s,
		INT32 flags,
		struct pike_string *name);
void compiler_do_inherit(node *n,
			 INT32 flags,
			 struct pike_string *name);
void simple_do_inherit(struct pike_string *s,
		       INT32 flags,
		       struct pike_string *name);
int isidentifier(struct pike_string *s);
int low_define_variable(struct pike_string *name,
			struct pike_string *type,
			INT32 flags,
			INT32 offset,
			INT32 run_time_type);
int map_variable(char *name,
		 char *type,
		 INT32 flags,
		 INT32 offset,
		 INT32 run_time_type);
int define_variable(struct pike_string *name,
		    struct pike_string *type,
		    INT32 flags);
int simple_add_variable(char *name,
			char *type,
			INT32 flags);
int add_constant(struct pike_string *name,
		 struct svalue *c,
		 INT32 flags);
int simple_add_constant(char *name, 
			struct svalue *c,
			INT32 flags);
int add_integer_constant(char *name,
			 INT32 i,
			 INT32 flags);
int add_float_constant(char *name,
			 double f,
			 INT32 flags);
int add_string_constant(char *name,
			char *str,
			INT32 flags);
int add_program_constant(char *name,
			 struct program *p,
			 INT32 flags);
int add_function_constant(char *name, void (*cfun)(INT32), char * type, INT16 flags);
int debug_end_class(char *name, INT32 flags);
INT32 define_function(struct pike_string *name,
		      struct pike_string *type,
		      INT16 flags,
		      INT8 function_flags,
		      union idptr *func);
int low_find_shared_string_identifier(struct pike_string *name,
				      struct program *prog);
struct ff_hash;
int find_shared_string_identifier(struct pike_string *name,
				  struct program *prog);
int find_identifier(char *name,struct program *prog);
int store_prog_string(struct pike_string *str);
int store_constant(struct svalue *foo, int equal);
void start_line_numbering(void);
void store_linenumber(INT32 current_line, struct pike_string *current_file);
char *get_line(unsigned char *pc,struct program *prog,INT32 *linep);
void my_yyerror(char *fmt,...)  ATTRIBUTE((format(printf,1,2)));
struct program *compile(struct pike_string *prog);
void add_function(char *name,void (*cfun)(INT32),char *type,INT16 flags);
void check_all_programs(void);
void cleanup_program(void);
void gc_mark_program_as_referenced(struct program *p);
void gc_check_all_programs(void);
void gc_mark_all_programs(void);
void gc_free_all_unreferenced_programs(void);
void count_memory_in_programs(INT32 *num_, INT32 *size_);
void push_compiler_frame(void);
void pop_local_variables(int level);
void pop_compiler_frame(void);
char *get_storage(struct object *o, struct program *p);
struct program *low_program_from_function(struct program *p,
					  INT32 i);
struct program *program_from_function(struct svalue *f);
struct program *program_from_svalue(struct svalue *s);
struct find_child_cache_s;
int find_child(struct program *parent, struct program *child);
void yywarning(char *fmt, ...) ATTRIBUTE((format(printf,1,2)));
/* Prototypes end here */


#endif

#ifdef DEBUG_MALLOC
#define end_program() ((struct program *)debug_malloc_touch(debug_end_program()))
#define end_class(NAME, FLAGS) do { debug_malloc_touch(new_program); debug_end_class(NAME, FLAGS); }while(0)
#else
#define end_class debug_end_class
#define end_program debug_end_program
#endif
