/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef BUILTIN_EFUNS_H
#define BUILTIN_EFUNS_H

#define TYPEP(ID,NAME,TYPE) void ID(INT32 args);

#include "callback.h"

/* Prototypes begin here */
void f_equal(INT32 args);
void f_aggregate(INT32 args);
void f_trace(INT32 args);
void f_hash(INT32 args);
void f_copy_value(INT32 args);
void f_ctime(INT32 args);
void f_lower_case(INT32 args);
void f_upper_case(INT32 args);
void f_random(INT32 args);
void f_random_seed(INT32 args);
void f_query_num_arg(INT32 args);
void f_search(INT32 args);
void f_backtrace(INT32 args);
void f_add_constant(INT32 args);
void f_combine_path(INT32 args);
void f_function_object(INT32 args);
void f_function_name(INT32 args);
void f_zero_type(INT32 args);
void f_all_constants(INT32 args);
void f_allocate(INT32 args);
void f_rusage(INT32 args);
void f_this_object(INT32 args);
void f_throw(INT32 args);
void f_exit(INT32 args);
void f_time(INT32 args);
void f_crypt(INT32 args);
void f_destruct(INT32 args);
void f_indices(INT32 args);
void f_values(INT32 args);
void f_next_object(INT32 args);
void f_object_program(INT32 args);
void f_reverse(INT32 args);
struct tupel;
void f_replace(INT32 args);
void f_compile(INT32 args);
void f_mkmapping(INT32 args);
void f_objectp(INT32 args);
void f_functionp(INT32 args);
void f_sleep(INT32 args);
void f_gc(INT32 args);
void f_programp(INT32 args);
TYPEP(f_intp, "intpp", T_INT)
TYPEP(f_mappingp, "mappingp", T_MAPPING)
TYPEP(f_arrayp, "arrayp", T_ARRAY)
TYPEP(f_multisetp, "multisetp", T_MULTISET)
TYPEP(f_stringp, "stringp", T_STRING)
TYPEP(f_floatp, "floatp", T_FLOAT)
void f_sort(INT32 args);
void f_rows(INT32 args);
void f_column(INT32 args);
void f__verify_internals(INT32 args);
void f__debug(INT32 args);
void f_localtime(INT32 args);
void f_glob(INT32 args);
struct callback *add_memory_usage_callback(callback_func call,
					  void *arg,
					  callback_func free_func);
void f__memory_usage(INT32 args);
void f__next(INT32 args);
void f__prev(INT32 args);
void f__refs(INT32 args);
void f_replace_master(INT32 args);
void f_master(INT32 args);
void f_gethrvtime(INT32 args);
void f_gethrtime(INT32 args);
void f_object_variablep(INT32 args);
void init_builtin_efuns(void);
/* Prototypes end here */

#endif
