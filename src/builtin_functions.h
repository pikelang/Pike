/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: builtin_functions.h,v 1.24 2004/03/31 14:28:21 grubba Exp $
*/

#ifndef BUILTIN_EFUNS_H
#define BUILTIN_EFUNS_H

#define TYPEP(ID,NAME,TYPE) PMOD_EXPORT void ID(INT32 args);

#include "callback.h"

/* Weak flags for arrays, multisets and mappings. 1 is avoided for
 * compatibility reasons. */
#define PIKE_WEAK_INDICES 2
#define PIKE_WEAK_VALUES 4
#define PIKE_WEAK_BOTH 6

/* Prototypes begin here */
PMOD_EXPORT void debug_f_aggregate(INT32 args);
#ifdef DEBUG_MALLOC
#define f_aggregate(X) do { debug_f_aggregate(X); debug_malloc_touch(Pike_sp[-1].u.refs); } while (0)
#else
#define f_aggregate(X) debug_f_aggregate(X)
#endif

PMOD_EXPORT void f_equal(INT32 args);
PMOD_EXPORT void f_trace(INT32 args);
PMOD_EXPORT void f_hash(INT32 args);
PMOD_EXPORT void f_copy_value(INT32 args);
PMOD_EXPORT void f_ctime(INT32 args);
PMOD_EXPORT void f_lower_case(INT32 args);
PMOD_EXPORT void f_upper_case(INT32 args);
PMOD_EXPORT void f_random(INT32 args);
PMOD_EXPORT void f_random_seed(INT32 args);
PMOD_EXPORT void f_query_num_arg(INT32 args);
PMOD_EXPORT void f_search(INT32 args);
PMOD_EXPORT void f_has_index(INT32 args);
PMOD_EXPORT void f_has_value(INT32 args);
PMOD_EXPORT void f_backtrace(INT32 args);
PMOD_EXPORT void f_add_constant(INT32 args);
PMOD_EXPORT void f_combine_path(INT32 args);
PMOD_EXPORT void f_function_object(INT32 args);
PMOD_EXPORT void f_function_name(INT32 args);
PMOD_EXPORT void f_zero_type(INT32 args);
PMOD_EXPORT void f_string_to_unicode(INT32 args);
PMOD_EXPORT void f_unicode_to_string(INT32 args);
PMOD_EXPORT void f_string_to_utf8(INT32 args);
PMOD_EXPORT void f_utf8_to_string(INT32 args);
PMOD_EXPORT void f_all_constants(INT32 args);
PMOD_EXPORT void f_allocate(INT32 args);
PMOD_EXPORT void f_rusage(INT32 args);
PMOD_EXPORT void f_this_object(INT32 args);
PMOD_EXPORT void f_throw(INT32 args);
PMOD_EXPORT void f_exit(INT32 args);
PMOD_EXPORT void f__exit(INT32 args);
PMOD_EXPORT void f_time(INT32 args);
PMOD_EXPORT void f_crypt(INT32 args);
PMOD_EXPORT void f_destruct(INT32 args);
PMOD_EXPORT void f_indices(INT32 args);
PMOD_EXPORT void f_values(INT32 args);
PMOD_EXPORT void f_next_object(INT32 args);
PMOD_EXPORT void f_object_program(INT32 args);
PMOD_EXPORT void f_reverse(INT32 args);
struct tupel;
PMOD_EXPORT void f_replace(INT32 args);
PMOD_EXPORT void f_compile(INT32 args);
PMOD_EXPORT void f_mkmapping(INT32 args);
PMOD_EXPORT void f_objectp(INT32 args);
PMOD_EXPORT void f_functionp(INT32 args);
PMOD_EXPORT void f_sleep(INT32 args);
PMOD_EXPORT void f_gc(INT32 args);
PMOD_EXPORT void f_programp(INT32 args);
TYPEP(f_intp, "intpp", PIKE_T_INT)
TYPEP(f_mappingp, "mappingp", PIKE_T_MAPPING)
TYPEP(f_arrayp, "arrayp", PIKE_T_ARRAY)
TYPEP(f_multisetp, "multisetp", PIKE_T_MULTISET)
TYPEP(f_stringp, "stringp", PIKE_T_STRING)
TYPEP(f_floatp, "floatp", PIKE_T_FLOAT)
PMOD_EXPORT void f_sort(INT32 args);
PMOD_EXPORT void f_rows(INT32 args);
PMOD_EXPORT void f_column(INT32 args);
PMOD_EXPORT void f__verify_internals(INT32 args);
PMOD_EXPORT void f__debug(INT32 args);
PMOD_EXPORT void f__compiler_trace(INT32 args);
PMOD_EXPORT void f_gmtime(INT32 args);
PMOD_EXPORT void f_localtime(INT32 args);
PMOD_EXPORT void f_glob(INT32 args);
PMOD_EXPORT void f_permute(INT32 args);
struct diff_magic_link;
struct diff_magic_link_pool;
struct diff_magic_link_head;
PMOD_EXPORT void f_diff(INT32 args);
PMOD_EXPORT void f_diff_compare_table(INT32 args);
PMOD_EXPORT void f_diff_longest_sequence(INT32 args);
PMOD_EXPORT void f_diff_dyn_longest_sequence(INT32 args);
struct callback *add_memory_usage_callback(callback_func call,
					  void *arg,
					  callback_func free_func);
PMOD_EXPORT void f__memory_usage(INT32 args);
PMOD_EXPORT void f__next(INT32 args);
PMOD_EXPORT void f__prev(INT32 args);
PMOD_EXPORT void f__refs(INT32 args);
PMOD_EXPORT void f_replace_master(INT32 args);
PMOD_EXPORT void f_master(INT32 args);
PMOD_EXPORT void f_gethrvtime(INT32 args);
PMOD_EXPORT void f_gethrtime(INT32 args);
PMOD_EXPORT void f_gethrtime(INT32 args);
PMOD_EXPORT void f_object_variablep(INT32 args);
PMOD_EXPORT void f_splice(INT32 args);
PMOD_EXPORT void f_everynth(INT32 args);
PMOD_EXPORT void f_transpose(INT32 args);
PMOD_EXPORT void f__reset_dmalloc(INT32 args);
PMOD_EXPORT void f__locate_references(INT32 args);
PMOD_EXPORT void f_map_array(INT32 args);
void init_builtin_efuns(void);
void init_builtin(void);
void exit_builtin(void);
void f_werror (INT32 args);
/* Prototypes end here */

/* Prototypes from builtin.cmod. */
void f_string_trim_all_whites(INT32 args);
/* End prototypes from builtin.cmod. */

#endif
