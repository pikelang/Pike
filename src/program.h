/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef PROGRAM_H
#define PROGRAM_H

#include <stdarg.h>
#include "global.h"

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

#define NUM_LFUNS 28

extern char *lfun_names[];

#ifndef STRUCT_SVALUE_DECLARED
#define STRUCT_SVALUE_DECLARED
struct svalue;
#endif

#ifndef STRUCT_OBJECT_DECLARED
#define STRUCT_OBJECT_DECLARED
struct object;
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
  void (*c_fun)(INT32);
  INT32 offset;
};

#define IDENTIFIER_PIKE_FUNCTION 1
#define IDENTIFIER_C_FUNCTION 2
#define IDENTIFIER_FUNCTION 3
#define IDENTIFIER_VARARGS 4
#define IDENTIFIER_CONSTANT 8

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
#define ID_VARARGS   0x20
#define ID_INLINE    0x40
#define ID_HIDDEN    0x80 /* needed? */
#define ID_INHERITED 0x100

struct reference
{
  unsigned INT16 inherit_offset;
  unsigned INT16 identifier_offset;
  INT16 id_flags; /* ID_* static, private etc.. */
};

struct inherit
{
  struct program *prog;
  INT16 inherit_level; /* really needed? */
  INT16 identifier_level;
  INT32 storage_offset;
};

#define PROG_DESTRUCT_IMMEDIATE 1

struct program
{
  INT32 refs;
  INT32 id;             /* used to identify program in caches */
  INT32 storage_needed; /* storage needed in the object struct */

  struct program *next;
  struct program *prev;
  unsigned char *program;
  struct pike_string **strings;
  struct inherit *inherits;
  struct reference *identifier_references;
  struct identifier *identifiers;
  unsigned INT16 *identifier_index;
  struct svalue *constants;
  char *linenumbers;
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
  SIZE_T num_linenumbers;
  SIZE_T program_size;
  unsigned INT16 flags;
  unsigned INT16 num_constants;
  unsigned INT16 num_strings;
  unsigned INT16 num_identifiers;
  unsigned INT16 num_identifier_references;
  unsigned INT16 num_identifier_indexes;
  unsigned INT16 num_inherits;
  INT16 lfuns[NUM_LFUNS];
};

#define INHERIT_FROM_PTR(P,X) ((P)->inherits + (X)->inherit_offset)
#define PROG_FROM_PTR(P,X) (INHERIT_FROM_PTR(P,X)->prog)
#define ID_FROM_PTR(P,X) (PROG_FROM_PTR(P,X)->identifiers+(X)->identifier_offset)
#define INHERIT_FROM_INT(P,X) INHERIT_FROM_PTR(P,(P)->identifier_references+(X))
#define PROG_FROM_INT(P,X) PROG_FROM_PTR(P,(P)->identifier_references+(X))
#define ID_FROM_INT(P,X) ID_FROM_PTR(P,(P)->identifier_references+(X))

#define free_program(p) do{ struct program *_=(p); if(!--_->refs) really_free_program(_); }while(0)

extern struct object fake_object;
extern struct program fake_program;

/* Prototypes begin here */
void use_module(struct svalue *s);
int find_module_identifier(struct pike_string *ident);
struct program *id_to_program(INT32 id);
void setup_fake_program(void);
void start_new_program(void);
void really_free_program(struct program *p);
void dump_program_desc(struct program *p);
void toss_current_program(void);
void check_program(struct program *p);
struct program *end_program(void);
SIZE_T add_storage(SIZE_T size);
void set_init_callback(void (*init)(struct object *));
void set_exit_callback(void (*exit)(struct object *));
void set_gc_mark_callback(void (*m)(struct object *));
int low_reference_inherited_identifier(int e,struct pike_string *name);
int reference_inherited_identifier(struct pike_string *super_name,
				   struct pike_string *function_name);
void rename_last_inherit(struct pike_string *n);
void do_inherit(struct program *p,INT32 flags, struct pike_string *name);
void simple_do_inherit(struct pike_string *s, INT32 flags,struct pike_string *name);
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
int end_class(char *name, INT32 flags);
INT32 define_function(struct pike_string *name,
		      struct pike_string *type,
		      INT16 flags,
		      INT8 function_flags,
		      union idptr *func);
struct ff_hash;
int find_shared_string_identifier(struct pike_string *name,
				  struct program *prog);
int find_identifier(char *name,struct program *prog);
int store_prog_string(struct pike_string *str);
int store_constant(struct svalue *foo, int equal);
void start_line_numbering(void);
void store_linenumber(INT32 current_line, struct pike_string *current_file);
char *get_line(unsigned char *pc,struct program *prog,INT32 *linep);
void my_yyerror(char *fmt,...);
void compile(void);
struct program *compile_file(struct pike_string *file_name);
struct program *compile_string(struct pike_string *prog,
			       struct pike_string *name);
void add_function(char *name,void (*cfun)(INT32),char *type,INT16 flags);
void check_all_programs(void);
void cleanup_program(void);
void gc_mark_program_as_referenced(struct program *p);
void gc_check_all_programs(void);
void gc_mark_all_programs(void);
void gc_free_all_unreferenced_programs(void);
void count_memory_in_programs(INT32 *num_, INT32 *size_);
void push_locals(void);
void pop_locals(void);
char *get_storage(struct object *o, struct program *p);
/* Prototypes end here */


void my_yyerror(char *fmt,...) ATTRIBUTE((format (printf, 1, 2)));

#endif




