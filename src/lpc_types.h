#ifndef LPC_TYPES_H
#define LPC_TYPES_H

extern struct lpc_string *string_type_string;
extern struct lpc_string *int_type_string;
extern struct lpc_string *float_type_string;
extern struct lpc_string *object_type_string;
extern struct lpc_string *function_type_string;
extern struct lpc_string *program_type_string;
extern struct lpc_string *array_type_string;
extern struct lpc_string *list_type_string;
extern struct lpc_string *mapping_type_string;
extern struct lpc_string *mixed_type_string;
extern struct lpc_string *void_type_string;

/* Prototypes begin here */
void init_types();
struct lpc_string *parse_type(char *s);
void stupid_describe_type(char *a,INT32 len);
char *low_describe_type(char *t);
TYPE_T compile_type_to_runtime_type(struct lpc_string *s);
int match_types(struct lpc_string *a,struct lpc_string *b);
void reset_type_stack();
void type_stack_mark();
void pop_stack_mark();
void pop_type_stack();
void type_stack_reverse();
void push_type(unsigned char tmp);
void push_unfinished_type(char *s);
void push_finished_type(struct lpc_string *type);
struct lpc_string *pop_type();
struct lpc_string *index_type(struct lpc_string *type);
int check_indexing(struct lpc_string *type,
		   struct lpc_string *index_type);
int count_arguments(struct lpc_string *s);
struct lpc_string *check_call(struct lpc_string *args,
				 struct lpc_string *type);
void check_array_type(struct array *a);
struct lpc_string *get_type_of_svalue(struct svalue *s);
char *get_name_of_type(int t);
void cleanup_lpc_types();
/* Prototypes end here */

#endif
