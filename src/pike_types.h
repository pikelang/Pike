/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef PIKE_TYPES_H
#define PIKE_TYPES_H

#include "las.h"

extern int max_correct_args;
extern struct pike_string *string_type_string;
extern struct pike_string *int_type_string;
extern struct pike_string *float_type_string;
extern struct pike_string *object_type_string;
extern struct pike_string *function_type_string;
extern struct pike_string *program_type_string;
extern struct pike_string *array_type_string;
extern struct pike_string *list_type_string;
extern struct pike_string *mapping_type_string;
extern struct pike_string *mixed_type_string;
extern struct pike_string *void_type_string;
extern struct pike_string *any_type_string;

#define init_type_stack type_stack_mark
#define exit_type_stack pop_stack_mark

/* Prototypes begin here */
void init_types(void);
void push_type(unsigned char tmp);
void type_stack_mark(void);
INT32 pop_stack_mark(void);
void pop_type_stack(void);
void type_stack_pop_to_mark(void);
void reset_type_stack(void);
void type_stack_reverse(void);
void push_type_int(unsigned INT32 i);
void push_unfinished_type(char *s);
void push_finished_type(struct pike_string *type);
struct pike_string *pop_unfinished_type(void);
struct pike_string *pop_type(void);
struct pike_string *parse_type(char *s);
void stupid_describe_type(char *a,INT32 len);
void simple_describe_type(struct pike_string *s);
char *low_describe_type(char *t);
struct pike_string *describe_type(struct pike_string *type);
TYPE_T compile_type_to_runtime_type(struct pike_string *s);
int match_types(struct pike_string *a,struct pike_string *b);
struct pike_string *index_type(struct pike_string *type, node *n);
int check_indexing(struct pike_string *type,
		   struct pike_string *index_type,
		   node *n);
int count_arguments(struct pike_string *s);
struct pike_string *check_call(struct pike_string *args,
			       struct pike_string *type);
struct pike_string *get_type_of_svalue(struct svalue *s);
char *get_name_of_type(int t);
void cleanup_pike_types(void);
/* Prototypes end here */

#endif
