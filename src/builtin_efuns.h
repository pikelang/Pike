#ifndef BUILTIN_EFUNS_H
#define BUILTIN_EFUNS_H

#define TYPEP(ID,NAME,TYPE) void ID(INT32 args);


/* Prototypes begin here */
void f_aggregate(INT32 args);
void f_trace(INT32 args);
void f_hash(INT32 args);
void f_copy_value(INT32 args);
void f_ctime(INT32 args);
void f_lower_case(INT32 args);
void f_upper_case(INT32 args);
void f_capitalize(INT32 args);
void f_random(INT32 args);
void f_query_num_arg(INT32 args);
void f_search(INT32 args);
void f_clone(INT32 args);
void f_call_function(INT32 args);
void f_backtrace(INT32 args);
void f_add_efun(INT32 args);
void f_compile_file(INT32 args);
void f_combine_path(INT32 args);
void f_get_function(INT32 args);
void f_implode(INT32 args);
void f_explode(INT32 args);
void f_function_object(INT32 args);
void f_function_name(INT32 args);
void f_zero_type(INT32 args);
void f_all_efuns(INT32 args);
void f_allocate(INT32 args);
void f_sizeof(INT32 args);
void f_rusage(INT32 args);
void f_this_object(INT32 args);
void f_throw(INT32 args);
void f_exit(INT32 args);
void f_query_host_name(INT32 args);
void f_time(INT32 args);
void f_crypt(INT32 args);
void f_destruct(INT32 args);
void f_indices(INT32 args);
void f_values(INT32 args);
void f_next_object(INT32 args);
void f_object_program(INT32 args);
void f_reverse(INT32 args);
struct tupel;
struct lpc_string * replace_many(struct lpc_string *str,
				    struct array *from,
				    struct array *to);
void f_replace(INT32 args);
void f_compile_string(INT32 args);
TYPEP(f_programp, "programp", T_PROGRAM)
TYPEP(f_intp, "intpp", T_INT)
TYPEP(f_mappingp, "mappingp", T_MAPPING)
TYPEP(f_arrayp, "arrayp", T_ARRAY)
TYPEP(f_listp, "listp", T_LIST)
TYPEP(f_stringp, "stringp", T_STRING)
TYPEP(f_objectp, "objectp", T_OBJECT)
TYPEP(f_floatp, "floatp", T_FLOAT)
TYPEP(f_functionp, "functionp", T_FUNCTION)
void init_builtin_efuns();
/* Prototypes end here */

#endif
