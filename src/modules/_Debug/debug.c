/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "module.h"
#include "pike_error.h"
#include "interpret.h"
#include "pike_embed.h"
#include "module_support.h"

#ifdef PIKE_DEBUG
/* This function is for debugging *ONLY*
 * do not document please. /Hubbe
 */
PMOD_EXPORT void f__leak(INT32 args)
{
  INT32 i;

  if(!args)
    SIMPLE_TOO_FEW_ARGS_ERROR("_leak", 1);

  if(!REFCOUNTED_TYPE(TYPEOF(Pike_sp[-args])))
    SIMPLE_BAD_ARG_ERROR("_leak", 1,
			 "array|mapping|multiset|object|"
			 "function|program|string");

  add_ref(Pike_sp[-args].u.dummy);
  i=Pike_sp[-args].u.refs[0];
  pop_n_elems(args);
  push_int(i);
}

/*! @decl int(0..) debug(int(0..) level)
 *! @belongs Debug
 *!
 *!   Set the run-time debug level.
 *!
 *! @returns
 *!   The old debug level will be returned.
 *! 
 *! @note
 *!   This function is only available if the Pike runtime has been compiled
 *!   with RTL debug.
 */
PMOD_EXPORT void f_debug(INT32 args)
{
  INT_TYPE d;
  get_all_args("debug", args, "%+", &d);
  pop_n_elems(args);
  push_int(d_flag);
  d_flag = d;
}

/*! @decl int(0..) optimizer_debug(int(0..) level)
 *! @belongs Debug
 *!
 *!   Set the optimizer debug level.
 *!
 *! @returns
 *!   The old optimizer debug level will be returned.
 *! 
 *! @note
 *!   This function is only available if the Pike runtime has been compiled
 *!   with RTL debug.
 */
PMOD_EXPORT void f_optimizer_debug(INT32 args)
{
  INT_TYPE l;
  get_all_args("optimizer_debug", args, "%+", &l);
  pop_n_elems(args);
  push_int(l_flag);
  l_flag = l;
}

/*! @decl int(0..) assembler_debug(int(0..) level)
 *! @belongs Debug
 *!
 *!   Set the assembler debug level.
 *!
 *! @returns
 *!   The old assembler debug level will be returned.
 *! 
 *! @note
 *!   This function is only available if the Pike runtime has been compiled
 *!   with RTL debug.
 */
PMOD_EXPORT void f_assembler_debug(INT32 args)
{
  INT_TYPE l;
  get_all_args("assembler_debug", args, "%+", &l);
  pop_n_elems(args);
  push_int(a_flag);
  a_flag = l;
}

/*! @decl void dump_program_tables(program p, int(0..)|void indent)
 *! @belongs Debug
 *!
 *! Dumps the internal tables for the program @[p] on stderr.
 *!
 *! @param p
 *!   Program to dump.
 *!
 *! @param indent
 *!   Number of spaces to indent the output.
 */
void f_dump_program_tables(INT32 args)
{
  struct program *p;
  INT_TYPE indent = 0;

  get_all_args("dump_program_tables", args, "%p.%+", &p, &indent);

  dump_program_tables(p, indent);
  pop_n_elems(args);
}

#ifdef YYDEBUG

/*! @decl int(0..) compiler_trace(int(0..) level)
 *! @belongs Debug
 *!
 *!   Set the compiler trace level.
 *!
 *! @returns
 *!   The old compiler trace level will be returned.
 *! 
 *! @note
 *!   This function is only available if the Pike runtime has been compiled
 *!   with RTL debug.
 */
PMOD_EXPORT void f_compiler_trace(INT32 args)
{
  extern int yydebug;
  INT_TYPE yyd;

  get_all_args("compiler_trace", args, "%i", &yyd);
  pop_n_elems(args);
  push_int(yydebug);
  yydebug = yyd;
}

#endif /* YYDEBUG */

#endif /* PIKE_DEBUG */

PIKE_MODULE_INIT
{
#ifdef PIKE_DEBUG
  ADD_INT_CONSTANT("HAVE_DEBUG", 1, 0);
  ADD_FUNCTION("_leak", f__leak, tFunc(tRef,tInt), OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION("debug", f_debug,
               tFunc(tIntPos,tIntPos), OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION("optimizer_debug", f_optimizer_debug,
               tFunc(tIntPos,tIntPos), OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION("assembler_debug", f_assembler_debug,
               tFunc(tInt,tIntPos), OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION("dump_program_tables", f_dump_program_tables,
               tFunc(tPrg(tObj) tIntPos,tVoid),
               OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);
#ifdef YYDEBUG
  ADD_FUNCTION("compiler_trace", f_compiler_trace,
               tFunc(tIntPos,tIntPos), OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);
#endif /* YYDEBUG */
#endif /* PIKE_DEBUG */
}

PIKE_MODULE_EXIT
{
}
