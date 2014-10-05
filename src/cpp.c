/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "stralloc.h"
#include "module_support.h"
#include "interpret.h"
#include "svalue.h"
#include "pike_macros.h"
#include "hashtable.h"
#include "program.h"
#include "object.h"
#include "pike_error.h"
#include "array.h"
#include "mapping.h"
#include "builtin_functions.h"
#include "operators.h"
#include "constants.h"
#include "time.h"
#include "stuff.h"
#include "version.h"
#include "pike_types.h"
#include "cpp.h"
#include "lex.h"

#include <ctype.h>

#define sp Pike_sp

#define CPP_NO_OUTPUT 1		/* Inside false section of #if/#else */
#define CPP_EXPECT_ELSE 2	/* Expect #else/#elif/#elseif. */
#define CPP_EXPECT_ENDIF 4	/* Expect #endif */
#define CPP_REALLY_NO_OUTPUT 8	/* Entire preprocessor is in false section. */
#define CPP_END_AT_NEWLINE 16	/* Halt at end of line. */
#define CPP_DO_IF 32
#define CPP_NO_EXPAND 64

#define OUTP() (!(flags & (CPP_NO_OUTPUT | CPP_REALLY_NO_OUTPUT)))
#define PUTNL() string_builder_putchar(&this->buf, '\n')
#define GOBBLE(X) (INDEX_PCHARP(data,pos)==(X)?++pos,1:0)
#define PUTC(C) do { \
 int c_=(C); if(OUTP() || c_=='\n') string_builder_putchar(&this->buf, c_); }while(0)

#define MAX_ARGS            255
#define DEF_ARG_STRINGIFY   0x100000
#define DEF_ARG_NOPRESPACE  0x200000
#define DEF_ARG_NOPOSTSPACE 0x400000
#define DEF_ARG_NEED_COMMA  0x800000
#define DEF_ARG_MASK        0x0fffff

struct define_part;
struct define_argument;
struct define;
struct cpp;

#ifdef __GNUC__
#pragma GCC optimize "-Os"
#endif

/* Return true if compat version is equal or less than MAJOR.MINOR */
#define CPP_TEST_COMPAT(THIS,MAJOR,MINOR)      \
  (THIS->compat_major < (MAJOR) ||	       \
   (THIS->compat_major == (MAJOR) &&	       \
    THIS->compat_minor <= (MINOR)))

#if 0
#define CALC_DUMPPOS(X)	DUMPPOS(X)
#else /* !0 */
#define CALC_DUMPPOS(X)
#endif /* 0 */

static struct pike_string *efun_str;
static struct pike_string *constant_str;
static struct pike_string *defined_str;

struct pike_predef_s
{
  struct pike_predef_s *next;
  char *name;
  char *value;
};

static int use_initial_predefs;
static struct pike_predef_s *first_predef = NULL, *last_predef = NULL;

struct define_part
{
  int argument;
  struct pike_string *postfix;
};

struct define_argument {
  PCHARP arg;
  ptrdiff_t len;
};


struct cpp;
struct define;
typedef void (*magic_define_fun)(struct cpp *,
				 struct define *,
				 struct define_argument *,
				 struct string_builder *);


struct define
{
  struct hash_entry link; /* must be first */
  magic_define_fun magic;
  int args;
  ptrdiff_t num_parts;
  short inside;		/* 1 - Don't expand. 2 - In use. */
  short varargs;
  struct pike_string *first;
  struct define_part parts[1];
};

#define FIND_DEFINE(N) \
  (this->defines?BASEOF(hash_lookup(this->defines, N), define, link):0)

struct cpp
{
  struct hash_table *defines;
  INT_TYPE current_line;
  INT32 compile_errors;
  struct pike_string *current_file;
  struct string_builder buf;
  struct object *handler;
  struct object *compat_handler;
  INT_TYPE compat_major;
  INT_TYPE compat_minor;
  struct pike_string *data;
  struct pike_string *prefix;
  INT_TYPE picky_cpp, keep_comments, dependencies_fail;
};

static void cpp_error(struct cpp *this, const char *err) ATTRIBUTE((noinline));
static void cpp_error_vsprintf (struct cpp *this, const char *fmt,
				va_list args) ATTRIBUTE((noinline));
static void cpp_error_sprintf(struct cpp *this, const char *fmt, ...) ATTRIBUTE((noinline));
static void cpp_handle_exception(struct cpp *this,
				 const char *cpp_error_fmt, ...) ATTRIBUTE((noinline));
static void cpp_warning(struct cpp *this, const char *cpp_warn_fmt, ...) ATTRIBUTE((noinline));
struct define *defined_macro =0;

static void cpp_error(struct cpp *this, const char *err)
{
  this->compile_errors++;
  if(this->compile_errors > 10) return;
  if((this->handler && this->handler->prog) || get_master())
  {
    ref_push_string(this->current_file);
    push_int(this->current_line);
    push_text(err);
    low_safe_apply_handler("compile_error", this->handler,
			   this->compat_handler, 3);
    pop_stack();
  }else{
    (void)fprintf(stderr, "%s:%ld: %s\n",
		  this->current_file->str,
		  (long)this->current_line,
		  err);
    fflush(stderr);
  }
}

static void cpp_error_vsprintf (struct cpp *this, const char *fmt,
				va_list args)
{
  struct string_builder s;
  struct pike_string *msg;

  this->compile_errors++;
  if (this->compile_errors > 10) return;

  init_string_builder(&s, 0);

  string_builder_vsprintf(&s, fmt, args);

  msg = finish_string_builder(&s);

  if((this->handler && this->handler->prog) || get_master())
  {
    ref_push_string(this->current_file);
    push_int(this->current_line);
    push_string(msg);
    low_safe_apply_handler("compile_error", this->handler,
			   this->compat_handler, 3);
    pop_stack();
    return;
  }

  if (this->current_file->size_shift) {
    fprintf(stderr, "WIDE:%ld: ", (long)this->current_line);
  } else {
    fprintf(stderr, "%s:%ld: ",
	    this->current_file->str, (long)this->current_line);
  }

  if (!msg->size_shift) {
    fprintf(stderr, "%s\n", msg->str);
  } else {
    fprintf(stderr, "WIDE (fmt: %s)\n", fmt);
  }
  free_string(msg);
  fflush(stderr);
}

static void cpp_error_sprintf(struct cpp *this, const char *fmt, ...)
{
  va_list args;
  va_start(args,fmt);
  cpp_error_vsprintf (this, fmt, args);
  va_end(args);
}

static void cpp_handle_exception(struct cpp *this,
				 const char *cpp_error_fmt, ...)
{
  struct svalue thrown;
  move_svalue (&thrown, &throw_value);
  mark_free_svalue (&throw_value);

  if (cpp_error_fmt) {
    va_list args;
    va_start (args, cpp_error_fmt);
    cpp_error_vsprintf (this, cpp_error_fmt, args);
    va_end (args);
  }

  push_svalue(&thrown);
  low_safe_apply_handler("compile_exception",
			 this->handler, this->compat_handler, 1);

  if (SAFE_IS_ZERO(sp-1)) {
    struct pike_string *s = format_exception_for_error_msg (&thrown);
    if (s) {
      cpp_error_sprintf(this, "%S", s);
      free_string (s);
    }
  }

  pop_stack();
  free_svalue(&thrown);
}

static void cpp_warning(struct cpp *this, const char *cpp_warn_fmt, ...)
{
  struct string_builder sb;
  va_list args;

  init_string_builder (&sb, 0);
  va_start(args, cpp_warn_fmt);
  string_builder_vsprintf (&sb, cpp_warn_fmt, args);
  va_end(args);

  if((this->handler && this->handler->prog) || get_master())
  {
    ref_push_string(this->current_file);
    push_int(this->current_line);
    push_string (finish_string_builder (&sb));
    low_safe_apply_handler("compile_warning", this->handler,
			   this->compat_handler, 3);
    pop_stack();
  }else{
    (void)fprintf(stderr, "%s:%ld: %s\n",
		  this->current_file->str,
		  (long)this->current_line,
		  sb.s->str);
    fflush(stderr);
    free_string_builder (&sb);
  }
}

/*! @class MasterObject
 */

/*! @decl inherit CompilationHandler
 *!
 *! The master object acts as fallback compilation handler for
 *! @[compile()] and @[cpp()].
 */

/*! @decl CompilationHandler get_compilation_handler(int major, int minor)
 *!
 *!   Get compilation handler for simulation of Pike v@[major].@[minor].
 *!
 *!   This function is called by @[cpp()] when it encounters
 *!   @expr{#pike@} directives.
 *!
 *! @param major
 *!   Major version.
 *!
 *! @param minor
 *!   Minor version.
 *!
 *! @returns
 *!   Returns a compilation handler for Pike >= @[major].@[minor].
 */

/*! @decl string decode_charset(string raw, string charset)
 *!
 *!   Convert @[raw] from encoding @[charset] to UNICODE.
 *!
 *!   This function is called by @[cpp()] when it encounters
 *!   @expr{#charset@} directives.
 *!
 *! @param raw
 *!   String to convert.
 *!
 *! @param charset
 *!   Name of encoding that @[raw] uses.
 *!
 *! @returns
 *!   @[raw] decoded to UNICODE, or @expr{0@} (zero) if the decoding failed.
 *!
 *! @seealso
 *!   @[Charset]
 */

/*! @endclass
 */

/*! @class CompilationHandler
 *!
 *!   Objects used by the compiler to handle references to global symbols,
 *!   modules, external files, etc.
 *!
 *!   There can be up to three compilation handlers active at the same
 *!   time during a compilation. They are in order of precedence:
 *!
 *!   @ol
 *!     @item
 *!       The error handler
 *!
 *!       This is the object passed to @[compile()] as
 *!       the second argument (if any). This object is returned by
 *!       @[get_active_error_handler()] during a compilation.
 *!
 *!     @item
 *!       The compatibility handler
 *!
 *!       This is the object returned by
 *!       @[master()->get_compilation_handler()] (if any), which
 *!       the compiler calls when it sees @tt{#pike@}-directives,
 *!       or expressions using the version scope
 *!       (eg @expr{7.4::rusage@}). This object is returned by
 *!       @[get_active_compilation_handler()] during a compilation.
 *!
 *!     @item
 *!       The master object.
 *!
 *!       This is returned by @[master()] at any time.
 *!   @endol
 *!
 *!   Any of the objects may implement a subset of the @[CompilationHandler]
 *!   functions, and the first object that implements a function will be
 *!   used. The error handler object can thus be used to block certain
 *!   functionality (eg to restrict the number of available functions).
 *!
 *! @seealso
 *!   @[master()->get_compilation_handler()], @[get_active_error_handler()],
 *!   @[get_active_compilation_handler()], @[compile()]
 */

/*! @decl void compile_error(string filename, int line, string msg)
 *!
 *!   Called by @[compile()] and @[cpp()] when they encounter
 *!   errors in the code they compile.
 *!
 *! @param filename
 *!   File where the error was detected.
 *!
 *! @param line
 *!   Line where the error was detected.
 *!
 *! @param msg
 *!   Description of error.
 *!
 *! @seealso
 *!   @[compile_warning()].
 */

/*! @decl void compile_exception(mixed exception)
 *!
 *!   Called by @[compile()] and @[cpp()] if they trigger
 *!   exceptions.
 */

/*! @decl mapping(string:mixed) get_predefines()
 *!
 *!   Called by @[cpp()] to get the set of global symbols.
 *!
 *! @returns
 *!   Returns a mapping from symbol name to symbol value.
 *!   Returns zero on failure.
 *!
 *! @seealso
 *!   @[resolv()], @[get_default_module()]
 */

/*! @decl mixed resolv(string symbol, string filename, @
 *!                    CompilationHandler handler)
 *!
 *!   Called by @[compile()] and @[cpp()] to resolv
 *!   module references.
 *!
 *! @returns
 *!   Returns the resolved value, or @[UNDEFINED] on failure.
 *!
 *! @seealso
 *!   @[get_predefines()]
 */

/*! @decl mixed handle_import(string path, string filename, @
 *!                           CompilationHandler handler)
 *!
 *!   Called by @[compile()] and @[cpp()] to handle import
 *!   directives specifying specific paths.
 *!
 *! @returns
 *!   Returns the resolved value, or @[UNDEFINED] on failure.
 */

/*! @decl string handle_include(string header_file, string current_file, @
 *!                             int(0..1) is_local_ref)
 *!
 *!   Called by @[cpp()] to resolv @expr{#include@} and @expr{#string@}
 *!   directives.
 *!
 *! @param header_file
 *!   File that was requested for inclusion.
 *!
 *! @param current_file
 *!   File where the directive was found.
 *!
 *! @param is_local_ref
 *!   Specifies reference method.
 *!   @int
 *!     @value 0
 *!       Directive was @expr{#include <header_file>@}.
 *!     @value 1
 *!       Directive was @expr{#include "header_file"@}.
 *!   @endint
 *!
 *! @returns
 *!   Returns the filename to pass to @[read_include()] if found,
 *!   and @expr{0@} (zero) on failure.
 *!
 *! @seealso
 *!   @[read_include()]
 */

/*! @decl string read_include(string filename)
 *!
 *!   Called by @[cpp()] to read included files.
 *!
 *! @param filename
 *!   Filename as returned by @[handle_include()].
 *!
 *! @returns
 *!   Returns a string with the content of the header file on success,
 *!   and @expr{0@} (zero) on failure.
 *!
 *! @seealso
 *!   @[handle_include()]
 */

/*! @endclass
 */

/*! @namespace predef:: */
/*! @decl import cpp:: */
/*! @endnamespace */

/* #pike handling. */

void cpp_change_compat(struct cpp *this, int major, int minor)
{
  if(this->compat_major == major &&
     this->compat_minor == minor) return;

  if(this->compat_handler)
  {
    free_object(this->compat_handler);
    this->compat_handler=0;
  }
  if((major == PIKE_MAJOR_VERSION &&
      minor == PIKE_MINOR_VERSION) || major < 0)
  {
    this->compat_major=PIKE_MAJOR_VERSION;
    this->compat_minor=PIKE_MINOR_VERSION;
    return; /* Our work here is done */
  }

  push_int(major);
  push_int(minor);
  SAFE_APPLY_MASTER("get_compilation_handler",2);
  if(TYPEOF(sp[-1]) == T_OBJECT)
  {
    if (SUBTYPEOF(sp[-1])) {
      cpp_error(this,
		"#pike: Subtyped compilation handlers are not supported yet.");
    }
    this->compat_handler=sp[-1].u.object;
    dmalloc_touch_svalue(Pike_sp-1);
    sp--;
  }
  this->compat_major=major;
  this->compat_minor=minor;
}

/* #if macros and functions. */

/*! @namespace cpp::
 *!
 *!   Pike has a builtin C-style preprocessor. It works similar to the
 *!   ANSI-C preprocessor but has a few extra features. These and the
 *!   default set of preprocessor macros are described here.
 */

/*! @directive #!
 *!
 *!   All lines beginning with @[#!] will be regarded as comments,
 *!   to enable shell integration. It is recommended that Pike applications
 *!   begin with the line @tt{"#! /usr/bin/env pike"@} for maximum cross
 *!   platform compatibility.
 */

/*! @directive #charset
 *!
 *!   Inform the preprocessor about which charset the file is encoded
 *!   with. The Charset module is called with this string to decode
 *!   the remainder of the file.
 */

/*! @directive #if
 *!
 *!   The @[#if] directive can evaluate simple expressions and, if
 *!   the expression is evaluates to true, "activate" the code block that
 *!   follows. The code block ends when an @[#endif], @[#else],
 *!   @[#elseif] or @[#elif] block is encountered at the same
 *!   nesting depth.
 *!
 *!   The @[#if] expressions may include defines, integer, string
 *!   and float constants, @tt{?:@}, @tt{||@} and @tt{&&@} operations,
 *!   @tt{~@}, @tt{^@}, @tt{!@}, @tt{|@} and @tt{&@} operations,
 *!   @tt{<@}, @tt{>@}, @tt{<=@}, @tt{>=@}, @tt{==@} and @tt{!=@} operations,
 *!   @tt{+@}, @tt{-@}, @tt{*@}, @tt{/@}, @tt{<<@} and @tt{>>@} operations
 *!   and paranthesis.
 *!
 *!   Strings may also be indexed with the @tt{[]@} index operator.
 *!   Finally there are three special "functions" available in @[#if]
 *!   expressions; @[defined()], @[efun()] and @[constant()].
 *!
 *! @seealso
 *!   @[#ifdef], @[#ifndef], @[#elif], @[#else], @[#endif],
 *!   @[defined()], @[constant()], @[efun()]
 */

/*! @directive #ifdef
 *!
 *!   Check whether an identifier is a macro.
 *!
 *!   The directive
 *!
 *!     @tt{#ifdef @i{<identifier>@}@}
 *!
 *!   is equvivalent to
 *!
 *!     @tt{#if @[defined](@i{<identifier>@})@}
 *!
 *! @seealso
 *!   @[#if], @[#ifndef], @[defined]
 */

/*! @directive #ifndef
 *!
 *!   Check whether an identifier is not a macro.
 *!
 *!   This is the inverse of @[#ifdef].
 *!
 *!   The directive
 *!
 *!     @tt{#ifndef @i{<identifier>@}@}
 *!
 *!   is equvivalent to
 *!
 *!     @tt{#if !@[defined](@i{<identifier>@})@}
 *!
 *! @seealso
 *!   @[#if], @[#ifdef], @[defined]
 */

/*! @directive #require
 *!
 *!   If the directive evaluates to false, the source file will be
 *!   considered to have failed dependencies, and will not be found by
 *!   the resolver. In practical terms the @[cpp()] call will return
 *!   zero.
 *!
 *! @seealso
 *!   @[#if]
 */

/*! @directive #endif
 *!
 *!   End a block opened with @[#if], @[#ifdef], @[#ifndef],
 *!   @[#else], @[#elseif] or @[#elif].
 *!
 *! @example
 *!   @code
 *!   #ifdef DEBUG
 *!     do_debug_stuff();
 *!   #endif // DEBUG
 *!   @endcode
 */

/*! @directive #else
 *!
 *!   This directive is used to divide the current code block into another
 *!   code block with inverse activation.
 *!
 *! @example
 *!   @code
 *!   #ifdef FAST_ALGORITHM
 *!     do_fast_algorithm();
 *!   #elif defined(EXPERIMENTAL_ALGORITHM)
 *!     do_experimental_algorithm();
 *!   #else
 *!     do_default_algorithm();
 *!   #endif
 *!   @endcode
 */

/*! @directive #elif
 *! @directive #elseif
 *!
 *! These work as a combined @[#else] and @[#if] without
 *! adding an extra level of nesting.
 *!
 *! @example
 *!
 *!   The following two are equvivalent:
 *!
 *!   @code
 *!   #ifdef A
 *!     // Code for A.
 *!   #else
 *!   #ifdef B
 *!     // Code for B.
 *!   #else
 *!   #ifdef C
 *!     // Code for C.
 *!   #else
 *!     // Code for D.
 *!   #endif
 *!   #endif
 *!   #endif
 *!   @endcode
 *!
 *!   And
 *!
 *!   @code
 *!   #ifdef A
 *!     // Code for A.
 *!   #elif defined(B)
 *!     // Code for B.
 *!   #elseif defined(C)
 *!     // Code for C.
 *!   #else
 *!     // Code for D.
 *!   #endif
 *!   @endcode
 *!
 *! @seealso
 *!   @[#if], @[#ifdef], @[#else], @[defined()], @[constant()]
 */

/*! @directive #error
 *!
 *!   Throw an error during preprocessing.
 *!
 *!   This directive causes a cpp error. It can be used to notify
 *!   the user that certain functions are missing and similar things.
 *!
 *! @note
 *!   Note that this directive will cause @[cpp()] to throw
 *!   an error at the end of preprocessing, which will cause
 *!   any compilation to fail.
 *!
 *! @example
 *!   @code
 *!   #if !constant(Yp)
 *!   #error Support for NIS not available.
 *!   #endif
 *!   @endcode
 *!
 *! @seealso
 *!   @[#warning]
 */

/*! @directive #warning
 *!
 *!   Generate a warning during preprocessing.
 *!
 *!   This directive causes a cpp warning, it can be used to notify
 *!   the user that certain functions are missing and similar things.
 *!
 *! @example
 *!   @code
 *!   #if !constant(Yp)
 *!   #warning Support for NIS not available. Some features may not work.
 *!   #endif
 *!   @endcode
 *!
 *! @seealso
 *!   @[#error]
 */

/*! @directive #include
 *!
 *!   @[#include] is used to insert the contents of another file into
 *!   the processed file at the place of the include directive.
 *!
 *!   Files can be referenced either by absolute or relative path from the
 *!   source file, or searched for in the include paths.
 *!
 *!   To include a file with absolute or relative path, use double quotes,
 *!   e.g. @tt{#include "constants.pike"@} or @tt{#include "../debug.h"@}.
 *!
 *!   To include from the include paths, use less than and greater than,
 *!   e.g. @tt{#include <profiling.h>@}.
 *!
 *!   It is also possible to include a file whose path is defined in a
 *!   preprocessor macro, e.g. @tt{#include USER_SETTINGS@}.
 */

/*! @directive #line
 *! @directive #<integer>
 *!
 *!   A hash character followed by a number or by the string
 *!   @tt{"line"@} and a number will make the preprocessor line counter
 *!   set this number as the line number for the next line and adjust the
 *!   following lines accordingly.
 *!
 *!   All error messages from Pike will use these line numbers.
 *!
 *!   Optionally the number may be followed by a file name, e.g.
 *!   @tt{#line 1 "/home/pike/program.pike.in"@}. Then this
 *!   filename will be used instead of the current file for error
 *!   messages.
*/

/*! @directive #pike
 *!
 *!   Set the Pike compiler backward compatibility level.
 *!
 *!   This tells the compiler which version of Pike it should
 *!   attempt to emulate from this point on in the current
 *!   compilation unit.
 *!
 *!   This is typically used to "quick-fix" old code to work
 *!   with more recent versions of Pike.
 *!
 *! @example
 *!   @code
 *!   // This code was written for Pike 7.2, and depends on
 *!   // the old behaviour for @[7.2::dirname()].
 *!   #pike 7.2
 *!
 *!   // ... Code that uses @[dirname()] ...
 *!   @endcode
 *!
 *!   This directive is also needed for Pike modules that
 *!   have been installed globally, and might be used by
 *!   a Pike that has been started with the @tt{-V@} flag.
 *!
 *! @example
 *!   @code
 *!   // Pike modules that are bundled with Pike are
 *!   // typically written for the same version of Pike.
 *!   #pike __REAL_VERSION__
 *!   @endcode
 */

/*! @directive #""
 *!   If a string literal is opened with @tt{#"@} newlines in the
 *!   string will end up in the string literal, instead of triggering a
 *!   @tt{"newline in string"@} error.
 *!
 *! @note
 *!   Newlines will be converted to @tt{\n@} characters in the string
 *!   even if the newlines in the file are something else.
 *!
 *!   This preprocessor directive may appear anywhere a string may
 *!   appear.
 *!
 *! @seealso
 *!   @[#string]
 */

/*! @directive #string
 *!   The preprocessor directive @[#string] will load the file in the
 *!   string that follows and insert its contents as a string. This
 *!   preprocessor directive may appear anywhere a string may appear.
 *!
 *! @example
 *!   @code
 *!   do_something(#string "the_file.wks");
 *!   @endcode
 *!
 *! @seealso
 *!   @[#include]
 */

/*! @directive #pragma
 *!
 *!   This is a generic directive for flags to the compiler.
 *!
 *!   These are some of the flags that are available:
 *!   @string
 *!     @value "all_inline"
 *!       This is the same as adding the modifier @tt{inline@}
 *!       to all functions that follow.
 *!     @value "all_final"
 *!       Instructs the compiler to mark all symbols as @tt{final@}.
 *!     @value "save_parent"
 *!       Cause nested classes to save a reference to their
 *!       surrounding class even if not strictly needed.
 *!     @value "dont_save_parent"
 *!       Inverse of @tt{"save_parent"@}. This is needed to override
 *!       if the global symbol @[predef::__pragma_save_parent__]
 *!       has been set.
 *!     @value "strict_types"
 *!       Enable warnings for all cases where the compiler
 *!       isn't certain that the types are correct.
 *!   @endstring
*/

/*! @decl int(0..1) defined(mixed identifier)
 *!
 *!   Check whether an identifier is a cpp macro or not.
 *!
 *! @returns
 *!   @[defined] returns true if the symbol given as argument
 *!   is defined.
 *!
 *! @note
 *!   @tt{#if defined(MY_DEF)@} is equvivalent to
 *!   @tt{#ifdef MY_DEF@}.
 *!
 *! @seealso
 *!   @[#if], @[#ifdef], @[constant()]
 */
static void check_defined(struct cpp *this,
			  struct define *UNUSED(def),
			  struct define_argument *args,
			  struct string_builder *tmp)
{
  struct pike_string *s = NULL;
  PCHARP x = MKPCHARP(args[0].arg.ptr,args[0].arg.shift);
  s = binary_findstring_pcharp( x, args[0].len );

  if(s && FIND_DEFINE(s))
    string_builder_binary_strcat(tmp, " 1 ", 3);
  else
    string_builder_binary_strcat(tmp, " 0 ", 3);
}

static int do_safe_index_call(struct cpp *this, struct pike_string *s)
{
  int res;
  JMP_BUF recovery;
  if(!s) return 0;

  if (SETJMP_SP(recovery, 1)) {
    if(this->picky_cpp)
      cpp_warning (this, "Error indexing module with %S.", s);
    res = 0;
    push_undefined();
  } else {
    ref_push_string(s);
    f_index(2);
    
    res=!(UNSAFE_IS_ZERO(sp-1) && SUBTYPEOF(sp[-1]) == NUMBER_UNDEFINED);
  }
  UNSETJMP(recovery);
  return res;
}

/*! @decl int(0..1) constant(mixed identifier)
 *! @decl __deprecated__ int(0..1) efun(mixed identifier)
 *!
 *!   Check whether the argument resolves to a constant or not.
 *!
 *! @seealso
 *!   @[#if], @[defined()]
 */
static void cpp_constant(struct cpp *this, int value)
{
  struct svalue *save_stack=sp;
  struct array *arr;
  INT_TYPE res = 0;
  int n;

  /* FIXME: Protection against errors. */
  /* Remove extra whitespace. */
  push_text(" ");
  o_subtract();
  push_text("\t");
  o_subtract();
  /* Split on . */
  push_text(".");
  o_divide();
#ifdef PIKE_DEBUG
  if (TYPEOF(Pike_sp[-1]) != T_ARRAY) {
    Pike_fatal("Bad result from division in constant(): %s "
	       "(expected array(string)).\n",
	       get_name_of_type(TYPEOF(Pike_sp[-1])));
  }
#endif /* PIKE_DEBUG */
  arr = Pike_sp[-1].u.array;
#ifdef PIKE_DEBUG
  if (!arr->size) {
    Pike_fatal("Got an empty array from division in constant().\n");
  }
  if ((arr->type_field & ~BIT_STRING) &&
      (array_fix_type_field(arr) & ~BIT_STRING)) {
    Pike_fatal("Bad result from division in constant(): type_field: 0x%08x "
	       "(expected array(string)).\n",
	       arr->type_field & ~BIT_STRING);
  }
#endif /* PIKE_DEBUG */

  if (arr->item[0].u.string->len) {
    struct pike_string *str = arr->item[0].u.string;
    struct svalue *sv;
    if((sv=low_mapping_string_lookup(get_builtin_constants(), str)))
    {
      /* efun */
      push_svalue(sv);
      res=1;
    } else if(get_master()) {
      /* Module. */
      ref_push_string(str);
      ref_push_string(this->current_file);
      if (this->handler) {
	ref_push_object(this->handler);
      } else {
	push_int(0);
      }

      if (safe_apply_handler("resolv", this->handler,
			     this->compat_handler, 3, 0)) {
	if ((TYPEOF(Pike_sp[-1]) == T_OBJECT &&
	     Pike_sp[-1].u.object == placeholder_object) ||
	    (TYPEOF(Pike_sp[-1]) == T_PROGRAM &&
	     Pike_sp[-1].u.program == placeholder_program)) {
	  cpp_error_sprintf (this, "Got placeholder %s (resolver problem) "
			     "when resolving %S.",
			     get_name_of_type(TYPEOF(Pike_sp[-1])),
			     str);
	}
	else
	  res = !(SAFE_IS_ZERO(sp-1) && SUBTYPEOF(sp[-1]) == NUMBER_UNDEFINED);
      }
      else if (TYPEOF(throw_value) == T_STRING &&
	       !throw_value.u.string->size_shift) {
	cpp_error(this, throw_value.u.string->str);
	free_svalue(&throw_value);
	mark_free_svalue (&throw_value);
	res = 0;
      } else if(this->picky_cpp) {
	cpp_warning (this, "Error resolving %S.", str);
	res = 0;
      }
    }
  } else {
    /* Handle constant(.foo) */
    push_text(".");
    ref_push_string(this->current_file);

    if (this->handler) {
      ref_push_object(this->handler);
    } else {
      push_int(0);
    }

    if (safe_apply_handler("handle_import", this->handler,
			   this->compat_handler, 3,
			   BIT_MAPPING|BIT_OBJECT|BIT_PROGRAM))
      res = !(SAFE_IS_ZERO(sp-1) && SUBTYPEOF(sp[-1]) == NUMBER_UNDEFINED);
    else {
      cpp_handle_exception (this, "Error importing '.'.");
    }
  }

  for (n = 1; res && (n < arr->size); n++) {
    res = do_safe_index_call(this, arr->item[n].u.string);
  }

  if (value && res) {
    if (TYPEOF(sp[-1]) == T_INT)
      res = sp[-1].u.integer;
    else
      res = 0;
  }

  pop_n_elems(1 + sp - save_stack);
  push_int(res);
}

static struct mapping *initial_predefs_mapping(void)
{
  struct pike_predef_s *def;
  struct mapping *map = allocate_mapping (0);
#ifdef PIKE_DEBUG
  if (!use_initial_predefs) Pike_fatal ("Initial predefs has been taken over.\n");
#endif
  for (def = first_predef; def; def = def->next) {
    push_text (def->name);
    push_text (def->value);
    mapping_insert (map, sp - 2, sp - 1);
    pop_n_elems (2);
  }
  return map;
}

static p_wchar2 readchar( PCHARP data, ptrdiff_t *pos, struct cpp *this )
{
  ptrdiff_t l;
  p_wchar2 C;
  INC_PCHARP(data,*pos);
  switch(parse_esc_seq_pcharp (data, &C, &l))
  {
  case 0:
    *pos += l;
    break;
  case 1:
    C = '\r';
    *pos += 1;
    break;
  case 3:
    /* The eof will get caught in the next round. */
    C = 0;
    *pos += 1;
    break;
  case 4: case 5: case 6:
    cpp_error (this, "Too large character value in escape.");
    C = (int) MAX_UINT32;
    *pos += l;
    break;
  case 7:
    cpp_error (this, "Too few hex digits in \\u escape.");
    C = '\\';
    break;
  case 8:
    cpp_error (this, "Too few hex digits in \\U escape.");
    C = '\\';
    break;
#ifdef PIKE_DEBUG
  case 2: Pike_fatal ("Not supposed to happen.\n");
  default: Pike_fatal ("Unknown error from parse_esc_seq.\n");
#endif
  }
  return C;
}


static ptrdiff_t readstring( struct cpp *this, const PCHARP data, ptrdiff_t len, ptrdiff_t pos,
			     struct string_builder*nf, int  nl_ok)
{
  while(1)
  {
    pos++;
    if(pos>=len)
    {
      cpp_error(this,"End of file in string.");
      break;
    }
    switch(INDEX_PCHARP(data,pos))
    {
    case '"':  break;
    case '\\':
      {
	pos++;
	if(INDEX_PCHARP(data,pos)=='\n')
	{
	  this->current_line++;
	  PUTNL();
	  continue;
	}
	if(INDEX_PCHARP(data,pos)=='\r' && INDEX_PCHARP(data,pos+1)=='\n')
	{
	  pos++;
	  this->current_line++;
	  PUTNL();
	  continue;
	}
	string_builder_putchar(nf, readchar(data,&pos,this));
	pos--;
	continue;
      }
    case '\r':
      continue; /* ignored */
    case '\n':
      this->current_line++;
      if( nl_ok )
	PUTNL();
      else
	cpp_error(this,"Newline in string.");
    default:
      string_builder_putchar(nf, INDEX_PCHARP(data,pos));
      continue;
    }
    pos++;
    break;
  }
  return pos;
}

static ptrdiff_t readstring_lit( struct cpp *this, const PCHARP data, ptrdiff_t len, ptrdiff_t pos,
			     struct string_builder*nf, INT32 ec)
{
  INT32 ch;
  while(1)
    if(++pos>=len)
      cpp_error(this,"End of file in string.");
    else if((ch=INDEX_PCHARP(data,pos)) == '#' && INDEX_PCHARP(data,pos+1)==ec)
        return pos + 2;
    else
        string_builder_putchar(nf, ch);
  return pos;
}

static ptrdiff_t fixstring(struct cpp *this, const PCHARP data, ptrdiff_t len,
			   ptrdiff_t pos, struct string_builder *nf, int outp)
{
  int trailing_newlines=0;
  if(outp) string_builder_putchar(nf, '"');
  while(1)
  {
    if(pos>=len)
    {
      cpp_error(this,"End of file in string.");
      break;
    }

    switch(INDEX_PCHARP(data,pos++))
    {
    case '\n':
      cpp_error(this,"Newline in string.");
      this->current_line++;
      break;
    case '"':  break;
    case '\\':
      if(INDEX_PCHARP(data,pos)=='\n')
      {
	pos++;
	trailing_newlines++;
	this->current_line++;
	continue;
      }
      if(INDEX_PCHARP(data,pos)=='\r' && INDEX_PCHARP(data,pos+1)=='\n')
      {
	pos+=2;
	trailing_newlines++;
	this->current_line++;
	continue;
      }
      if(outp) string_builder_putchar(nf, '\\');
      pos++;
      /* Fall through. */
    default:
      if(outp) string_builder_putchar(nf, INDEX_PCHARP(data,pos-1));
      continue;
    }
    break;
  }
  if(outp) string_builder_putchar(nf, '"');
  while(trailing_newlines--) PUTNL();
  return pos;
}

static ptrdiff_t find_end_of_line( struct cpp *this, const PCHARP data,
                                   ptrdiff_t len, ptrdiff_t pos, int emit )
{
  while(pos < len) {
    switch (INDEX_PCHARP(data,pos++)) {
    case '\n':
      return pos-1;
    case '\\':
      if (INDEX_PCHARP(data,pos) == '\n') {
	pos+=2;
      } else if ((INDEX_PCHARP(data,pos) == '\r') &&
		 (INDEX_PCHARP(data,pos+1) == '\n')) {
	pos+=3;
      } else {
	pos++;
	continue;
      }
      this->current_line++;
      if( emit ) PUTNL();
    }
  }
  return pos;
}


static ptrdiff_t find_end_of_comment( struct cpp *this, const PCHARP data, ptrdiff_t len,
                                      ptrdiff_t pos, int emit)
{
  pos++;

  while(INDEX_PCHARP(data,pos)!='*' || INDEX_PCHARP(data,pos+1)!='/')
  {	
    if(pos+2>=len)
    {
      cpp_error(this,"End of file in comment.");
      break;
    }

    if(INDEX_PCHARP(data,pos)=='\n')
    {
      this->current_line++;
      if( emit )PUTNL();
    }
    pos++;
  }
  return pos + 2;
}

static ptrdiff_t find_end_of_string2( struct cpp *this, const PCHARP data,
                                      ptrdiff_t len, ptrdiff_t pos)
{
  while(1)
  {
    if(pos>=len)
    {
      cpp_error(this,"End of file in string.");
      break;
    }
    switch(INDEX_PCHARP(data,pos++))
    {
    case '\n':
      this->current_line++;
      PUTNL();
      continue;
    case '"': return pos;
    case '\\':
      if(INDEX_PCHARP(data,pos)=='\n') {
	this->current_line++;
	PUTNL();
      }
      else if ((INDEX_PCHARP(data,pos) == '\r') && (INDEX_PCHARP(data,pos+1) == '\n')) {
	this->current_line++;
	pos++;
	PUTNL();
      }
      pos++;
    }
  }
  return pos;
}


static ptrdiff_t find_end_of_string( struct cpp *this, const PCHARP data,
                                     ptrdiff_t len, ptrdiff_t pos)
{
  while(1)
  {
    if(pos>=len)
    {
      cpp_error(this,"End of file in string.");
      break;
    }
    switch(INDEX_PCHARP(data,pos++))
    {
    case '\n':
      cpp_error(this,"Newline in string.");
      this->current_line++;
      PUTNL();
      break;
    case '"': return pos;
    case '\\':
      if(INDEX_PCHARP(data,pos)=='\n') {
	this->current_line++;
	PUTNL();
      }
      else if ((INDEX_PCHARP(data,pos) == '\r') && (INDEX_PCHARP(data,pos+1) == '\n')) {
	this->current_line++;
	pos++;
	PUTNL();
      }
      pos++;
    }
  } 
  return pos;
}

static ptrdiff_t find_end_of_char( struct cpp *this, const PCHARP data, ptrdiff_t len, ptrdiff_t pos)
{
  int e=0;
  while(1)
  {
    if(pos>=len)
    {
      cpp_error(this,"End of file in character constant.");
      break;
    }

    if(e++>32)
    {
      cpp_error(this,"Too long character constant.");
      break;
    }

    switch(INDEX_PCHARP(data,pos++))
    {
    case '\n':
      cpp_error(this,"Newline in char.");
      this->current_line++;
      PUTNL();
      break;
    case '\'': 
      return pos;
    case '\\':
      if(INDEX_PCHARP(data,pos)=='\n') {
	this->current_line++;
	PUTNL();
      }
      else if ((INDEX_PCHARP(data,pos) == '\r') && (INDEX_PCHARP(data,pos+1) == '\n'))
      {
	this->current_line++;
	pos++;
	PUTNL();
      }
      pos++;
    }
  }
  return pos;
}


static ptrdiff_t find_end_brace(struct cpp *this,
				PCHARP data,
				ptrdiff_t len,
				ptrdiff_t pos);

static ptrdiff_t find_end_parenthesis(struct cpp *this,
				      PCHARP data,
				      ptrdiff_t len,
				      ptrdiff_t pos)
/* pos is after the open paren. Returns the position after the close paren. */
{
  INT_TYPE start_line = this->current_line;
  while(1)
  {
    if(pos+1>=len)
    {
      INT_TYPE save_line = this->current_line;
      this->current_line = start_line;
      cpp_error(this, "End of file while looking for end parenthesis.");
      this->current_line = save_line;
      return pos;
    }

    switch(INDEX_PCHARP(data,pos++))
    {
    case '\n': PUTNL(); this->current_line++; break;
    case '\'': pos=find_end_of_char(this,data,len,pos);  break;
    case '"':  pos=find_end_of_string(this,data,len,pos); break;
    case '(':  pos=find_end_parenthesis(this, data, len, pos); break;
    case '{':  pos=find_end_brace(this, data, len, pos); break;
    case ')':  return pos;
    case '/':
      if (INDEX_PCHARP(data,pos) == '*') {
	pos = find_end_of_comment(this,data,len,pos+1,0);
      } else if (INDEX_PCHARP(data,pos) == '/') {
	pos = find_end_of_line(this,data,len,pos+1,0);
      }
    }
  }
}


static ptrdiff_t find_end_brace(struct cpp *this,
				const PCHARP data,
				ptrdiff_t len,
				ptrdiff_t pos)
/* pos is after the open brace. Returns the position after the close brace. */
{
  INT_TYPE start_line = this->current_line;
  while(1)
  {
    if(pos+1>=len)
    {
      INT_TYPE save_line = this->current_line;
      this->current_line = start_line;
      cpp_error(this, "End of file while looking for end brace.");
      this->current_line = save_line;
      return pos;
    }

    switch(INDEX_PCHARP(data,pos++))
    {
    case '\n': PUTNL(); this->current_line++; break;
    case '\'': pos=find_end_of_char(this,data,len,pos);  break;
    case '"':  pos=find_end_of_string(this,data,len,pos);  break;
    case '{':  pos=find_end_brace(this, data, len, pos); break;
    case '}':  return pos;
    case '/':
      if (INDEX_PCHARP(data,pos) == '*') {
	pos=find_end_of_comment(this,data,len,pos,0);
      } else if (INDEX_PCHARP(data,pos) == '/') {
	pos=find_end_of_line(this,data,len,pos,0);
      }
    }
  }
}


static inline int wide_isspace( int c ) {
  return WIDE_ISSPACE(c);
}

static inline int wide_isidchar( int c ) {
  return WIDE_ISIDCHAR(c);
}

static struct pike_string *gobble_identifier (struct cpp *this, const PCHARP data, ptrdiff_t *pos)
{
  ptrdiff_t p = *pos;
  struct string_builder sb;
  p_wchar2 tmp;
  if( !wide_isidchar( tmp = INDEX_PCHARP(data,*pos)) && tmp != '\\' )
    return NULL;

  init_string_builder (&sb, 0); /* in fact, 0 is more likely than data.shift */

  while (1) {
    ptrdiff_t start = p;
    while (wide_isidchar (INDEX_PCHARP(data,p))) 
      p++;
    if (p != start)
    {
      PCHARP x = data;
      INC_PCHARP(x,start);
      string_builder_append(&sb,x, p - start);
    }
    if (INDEX_PCHARP(data,p) != '\\') goto past_identifier;

    switch (INDEX_PCHARP(data,p + 1)) {
      case '\r':
	if (INDEX_PCHARP(data,p + 2) != '\n')
	  goto past_identifier;
	p++;
	/* Fall through */
      case '\n':
	this->current_line++;
	PUTNL();
	p += 2;
	break;

      case 'u':
      case 'U': {
	/* Note: Code dup in parse_esc_seq in lexer.h. */
	/* Don't have to bother checking for an even number of
	 * preceding backslashes since they aren't valid outside
	 * string and char literals in the lexer input. */
	unsigned INT32 c = 0;
	ptrdiff_t stop, q;
	if (INDEX_PCHARP(data,p + 2) == INDEX_PCHARP(data,p + 1))
	  /* A quoted \u escape means we got "\uxxxx" dequoted here,
	   * and that can't be part of an identifier. */
	  goto past_identifier;
	if (INDEX_PCHARP(data,p + 1) == 'u')
	  stop = p + 6;
	else
	  stop = p + 10;
	for (q = p + 2; q < stop; q++)
        {
	  int tmp;
	  switch (tmp=INDEX_PCHARP(data,q)) {
	    case '0': case '1': case '2': case '3': case '4':
	    case '5': case '6': case '7': case '8': case '9':
	      c = 16 * c + tmp - '0';
	      break;
	    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
	      c = 16 * c + tmp - 'a' + 10;
	      break;
	    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
	      c = 16 * c + tmp - 'A' + 10;
	      break;
	    default:
	      cpp_error_sprintf (this, "Too few hex digits in \\%c escape.",
				 INDEX_PCHARP(data,p + 1));
	      goto past_identifier;
	  }
	}
	if (!wide_isidchar (c)) goto past_identifier;
	string_builder_putchar (&sb, c);
	p = q;
	break;
      }

      default:
	goto past_identifier;
    }
  }

past_identifier:
  if (p != *pos) {
    *pos = p;
    return finish_string_builder (&sb);
  }
  free_string_builder (&sb);
  return NULL;
}

/* devours one reference to 'name'! */
static struct define *alloc_empty_define(struct pike_string *name,
					 ptrdiff_t parts)
{
  struct define *def;
  def=xalloc(sizeof(struct define)+
             sizeof(struct define_part) * (parts -1));
  def->magic=0;
  def->args=-1;
  def->inside=0;
  def->varargs=0;
  def->num_parts=parts;
  def->first=0;
  def->link.s=name;
  debug_malloc_touch(name);
  return def;
}

/*! @directive #undef
 *! @directive #undefine
 *!
 *!   This removes the effect of a @[#define], all subsequent occurances of
 *!   the undefined identifier will not be replaced by anything.
 *!
 *! @note
 *!   Note that when undefining a macro, you just give the identifer,
 *!   not the arguments.
 *!
 *! @example
 *!   // Strip debug
 *!   #define werror(X ...) 0
 *!   #include "/home/someone/experimental/stuff.h"
 *!   #undef werror
 *!
 *! @seealso
 *!   @[#define], @[defined()]
 */
static void undefine(struct cpp *this,
		     const struct pike_string *name)
{
  INT32 e;
  struct define *d;

  d=FIND_DEFINE(name);

  if(!d) return;

  if (d->inside) {
    cpp_error(this, "Illegal to undefine a macro during its expansion.");
    return;
  }

  this->defines=hash_unlink(this->defines, & d->link);

  for(e=0;e<d->num_parts;e++)
    free_string(d->parts[e].postfix);
  free_string(d->link.s);
  if(d->first)
    free_string(d->first);
  free(d);
}

/*! @directive #define
 *!
 *!   This directive is used to define or redefine a cpp macro.
 *!
 *!   The simplest way to use define is to write
 *!
 *!   @code
 *!     #define @b{@i{<identifier>@}@} @i{<replacement string>@}
 *!   @endcode
 *!
 *!   which will cause all subsequent occurances of @tt{@b{@i{<identifier@}@}@}
 *!   to be replaced with the @tt{@i{<replacement string>@}@}.
 *!
 *!   Define also has the capability to use arguments, thus a line like
 *!
 *!   @code
 *!     #define @b{@i{<identifier>@}@}(arg1, arg2) @i{<replacement string>@}
 *!   @endcode
 *!
 *!   would cause @tt{@b{@i{<identifer>@}@}@} to be a macro. All occurances of
 *!   '@tt{@b{@i{<identifier>@}@}(something1,something2d)@}' would be replaced
 *!   with the @tt{@i{<replacement string>@}@}.
 *!   And in the @tt{@i{<replacement string>@}@}, @tt{arg1@} and @tt{arg2@}
 *!   will be replaced with @tt{something1@} and @tt{something2@}.
 */

static struct define *do_magic_define(struct cpp *this,
				      const char *name,
				      magic_define_fun fun)
{
  struct define* def;

  if (this->prefix) {
    struct string_builder s;
    int len = strlen(name);

    init_string_builder(&s, 0);
    string_builder_append(&s, MKPCHARP_STR(this->prefix),
			  this->prefix->len);
    string_builder_putchar(&s, '_');
    string_builder_binary_strcat(&s, name, len);
    def = alloc_empty_define(finish_string_builder(&s),0);
  } else
    def = alloc_empty_define(make_shared_string(name),0);
  def->magic=fun;
  this->defines=hash_insert(this->defines, & def->link);

  return def;
}

static void add_define(struct cpp *this,
		       struct pike_string *name,
		       struct pike_string *what)
{
  struct define* def;
  add_ref (name);
  def=alloc_empty_define(name,0);
  add_ref (def->first = what);
  this->defines=hash_insert(this->defines, & def->link);
}

static void simple_add_define(struct cpp *this,
                              const char *name,
                              const char *what)
{
  struct define* def;

  if (this->prefix) {
    struct string_builder s;
    int len = strlen(name);

    init_string_builder(&s, 0);
    string_builder_append(&s, MKPCHARP_STR(this->prefix),
			  this->prefix->len);
    string_builder_putchar(&s, '_');
    string_builder_binary_strcat(&s, name, len);
    def = alloc_empty_define(finish_string_builder(&s),0);
  } else
    def = alloc_empty_define(make_shared_string(name),0);

  def->first=make_shared_string(what);
  this->defines=hash_insert(this->defines, & def->link);
}

static struct pike_string *recode_string(struct cpp *this, struct pike_string *data)
{
  /* Observations:
   *
   * * At least a prefix of two bytes need to be 7bit in a valid
   *   Pike program.
   *
   * * NUL isn't valid in a Pike program.
   */
  /* Heuristic:
   *
   * Index 0 | Index 1 | Interpretation
   * --------+---------+------------------------------------------
   *       0 |       0 | 32bit wide string.
   *       0 |      >0 | 16bit Unicode string.
   *      >0 |       0 | 16bit Unicode string reverse byte order.
   *    0xfe |    0xff | 16bit Unicode string.
   *    0xff |    0xfe | 16bit Unicode string reverse byte order.
   *    0x7b |    0x83 | EBCDIC-US ("#c").
   *    0x7b |    0x40 | EBCDIC-US ("# ").
   *    0x7b |    0x09 | EBCDIC-US ("#\t").
   * --------+---------+------------------------------------------
   *   Other |   Other | 8bit standard string.
   *
   * Note that the tests below are more lenient than the table above.
   * This shouldn't matter, since the other cases would be erroneus
   * anyway.
   */

  /* Add an extra reference to data, since we may return it as is. */
  add_ref(data);

  if ((!((unsigned char *)data->str)[0]) ||
      (((unsigned char *)data->str)[0] == 0xfe) ||
      (((unsigned char *)data->str)[0] == 0xff) ||
      (!((unsigned char *)data->str)[1])) {
    /* Unicode */
    if ((!((unsigned char *)data->str)[0]) &&
	(!((unsigned char *)data->str)[1])) {
      /* 32bit Unicode (UCS4) */
      struct pike_string *new_str;
      ptrdiff_t len;
      ptrdiff_t i;
      ptrdiff_t j;
      p_wchar0 *orig = STR0(data);
      p_wchar2 *dest;

      if (data->len & 3) {
	/* String len is not a multiple of 4 */
	return data;
      }
      len = data->len/4;
      new_str = begin_wide_shared_string(len, 2);

      dest = STR2(new_str);

      j = 0;
      for(i=0; i<len; i++) {
	dest[i] = (orig[j]<<24) | (orig[j+1]<<16) | (orig[j+2]<<8) | orig[j+3];
	j += 4;
      }

      free_string(data);
      return(end_shared_string(new_str));
    } else {
      /* 16bit Unicode (UCS2) */
      if (data->len & 1) {
	/* String len is not a multiple of 2 */
	return data;
      }
      if ((!((unsigned char *)data->str)[1]) ||
	  (((unsigned char *)data->str)[1] == 0xfe)) {
	/* Reverse Byte-order */
	struct pike_string *new_str = begin_shared_string(data->len);
	int i;
	for(i=0; i<data->len; i++) {
	  new_str->str[i^1] = data->str[i];
	}
	free_string(data);
	data = end_shared_string(new_str);
      }
      /* Note: We lose the extra reference to data here. */
      push_string(data);
      f_unicode_to_string(1);
      add_ref(data = sp[-1].u.string);
      pop_stack();
      return data;
    }
  } else if (data->str[0] == '{') {
    /* EBCDIC */
    /* Notes on EBCDIC:
     *
     * * EBCDIC conversion needs to first convert the first line
     *   according to EBCDIC-US, and then the rest of the string
     *   according to the encoding specified by the first line.
     *
     * * It's an error for a program written in EBCDIC not to
     *   start with a #charset directive.
     *
     * Obfuscation note:
     *
     * * This still allows the rest of the file to be written in
     *   another encoding than EBCDIC.
     */

    /* First split out the first line.
     *
     * Note that codes 0x00 - 0x1f are the same in ASCII and EBCDIC.
     */
    struct pike_string *new_str;
    char *p = strchr(data->str, '\n');
    char *p2;
    size_t len;

    if (!p) {
      return data;
    }

    len = p - data->str;

    if (len < CONSTANT_STRLEN("#charset ")) {
      return data;
    }

    new_str = begin_shared_string(len);

    memcpy(new_str->str, data->str, len);

    push_string(end_shared_string(new_str));

    push_text("ebcdic-us");

    if (safe_apply_handler ("decode_charset", this->handler, this->compat_handler,
			    2, BIT_STRING)) {
      /* Various consistency checks. */
      if ((sp[-1].u.string->size_shift) ||
	  (((size_t)sp[-1].u.string->len) < CONSTANT_STRLEN("#charset")) ||
	  (sp[-1].u.string->str[0] != '#')) {
	pop_stack();
	return data;
      }
    }
    else {
      cpp_handle_exception (this, "Error decoding with charset 'ebcdic-us'");
      return data;
    }

    /* At this point the decoded first line is on the stack. */

    /* Extract the charset name */

    p = sp[-1].u.string->str + 1;
    while (*p && isspace(*((unsigned char *)p))) {
      p++;
    }

    if (strncmp(p, "charset", CONSTANT_STRLEN("charset")) ||
	!isspace(((unsigned char *)p)[CONSTANT_STRLEN("charset")])) {
      pop_stack();
      return data;
    }

    p += CONSTANT_STRLEN("charset") + 1;

    while (*p && isspace(*((unsigned char *)p))) {
      p++;
    }

    if (!*p) {
      pop_stack();
      return data;
    }

    /* Build a string of the trailing data
     * NOTE:
     *   Keep the newline, so the linenumber info stays correct.
     */

    new_str = begin_shared_string(data->len - len);

    memcpy(new_str->str, data->str + len, data->len - len);

    push_string(end_shared_string(new_str));

    stack_swap();

    /* Build a string of the charset name */

    p2 = p;
    while(*p2 && !isspace(*((unsigned char *)p2))) {
      p2++;
    }

    len = p2 - p;

    new_str = begin_shared_string(len);

    memcpy(new_str->str, p, len);

    pop_stack();
    ref_push_string(new_str = end_shared_string(new_str));

    /* Decode the string */

    if (!safe_apply_handler ("decode_charset", this->handler, this->compat_handler,
			     2, BIT_STRING)) {
      cpp_handle_exception (this, "Error decoding with charset %S", new_str);
      free_string (new_str);
      return data;
    }
    free_string (new_str);

    /* Accept the new string */

    free_string(data);
    add_ref(data = sp[-1].u.string);
    pop_stack();
  }
  return data;
}

static struct pike_string *filter_bom(struct pike_string *data)
{
  /* More notes:
   *
   * * Character 0xfeff (ZERO WIDTH NO-BREAK SPACE = BYTE ORDER MARK = BOM)
   *   needs to be filtered away before processing continues.
   */
  ptrdiff_t i;
  ptrdiff_t j = 0;
  ptrdiff_t len = data->len;
  struct string_builder buf;

  /* Add an extra reference to data here, since we may return it as is. */
  add_ref(data);

  if (!data->size_shift) {
    return(data);
  }
  
  init_string_builder(&buf, data->size_shift);
  if (data->size_shift == 1) {
    /* 16 bit string */
    p_wchar1 *ptr = STR1(data);
    for(i = 0; i<len; i++) {
      if (ptr[i] == 0xfeff) {
	if (i != j) {
	  string_builder_binary_strcat1 (&buf, ptr + j, i - j);
	  j = i+1;
	}
      }
    }
    if ((j) && (i != j)) {
      /* Add the trailing string */
      string_builder_binary_strcat1 (&buf, ptr + j, i - j);
      free_string(data);
      data = finish_string_builder(&buf);
    } else {
      /* String didn't contain 0xfeff */
      free_string_builder(&buf);
    }
  } else {
    /* 32 bit string */
    p_wchar2 *ptr = STR2(data);
    for(i = 0; i<len; i++) {
      if (ptr[i] == 0xfeff) {
	if (i != j) {
	  string_builder_binary_strcat2 (&buf, ptr + j, i - j);
	  j = i+1;
	}
      }
    }
    if ((j) && (i != j)) {
      /* Add the trailing string */
      string_builder_binary_strcat2 (&buf, ptr + j, i - j);
      free_string(data);
      data = finish_string_builder(&buf);
    } else {
      /* String didn't contain 0xfeff */
      free_string_builder(&buf);
    }
  }
  return(data);
}

static void free_one_define(struct hash_entry *h)
{
  int e;
  struct define *d=BASEOF(h, define, link);

  for(e=0;e<d->num_parts;e++)
    free_string(d->parts[e].postfix);
  if(d->first)
    free_string(d->first);
  free(d);
}

#define PUSH_STRING0(X,Y,Z) add_quoted_string( X,Y,0,Z)
#define PUSH_STRING_SHIFT(X,Y,Z,A) add_quoted_string(X,Y,Z,A)
#define WC_ISSPACE	wide_isspace
#define WC_ISIDCHAR	wide_isidchar
/*
 * Generic macros
 */

#define CHECK_WORD(X,LEN) (begins_with(X,ADD_PCHARP(data,pos),(LEN),len-pos,1))
#define GOBBLE_WORD(X) (CHECK_WORD(X,NELEM(X)) ? (pos+=NELEM(X)),1 : 0)
#define FIND_END_OF_STRING() (pos=find_end_of_string(this,data,len,pos))
#define FIND_END_OF_STRING2() (pos=find_end_of_string2(this,data,len,pos))
#define FIND_END_OF_CHAR() (pos=find_end_of_char(this,data,len,pos))
#define FIND_EOL_PRETEND() (pos=find_end_of_line(this,data,len,pos,0))
#define FIND_EOL() (pos=find_end_of_line(this,data,len,pos,1))
#define SKIPCOMMENT_INC_LINES() (pos=find_end_of_comment(this,data,len,pos,0))
#define SKIPCOMMENT() (pos=find_end_of_comment(this,data,len,pos,1))
#define FIND_EOS() (pos=find_eos(this,data,len,pos))

/* Skips horizontal whitespace and newlines. */
#define SKIPWHITE() (pos=skipwhite(this,data,pos))

/* Skips horizontal whitespace and escaped newlines. */
#define SKIPSPACE() (pos=skipspace(this,data,pos,1))
#define SKIPSPACE_PRETEND() (pos=skipspace(this,data,pos,0))

/* At entry pos points past the start quote.
 * At exit pos points past the end quote.
 */
#define READSTRING(nf) (pos=readstring(this,data,len,pos,&nf,0))
#define READSTRING2(nf) (pos=readstring(this,data,len,pos,&nf,1))
#define READSTRING3(nf,ec) (pos=readstring_lit(this,data,len,pos,&nf,ec))
#define FIXSTRING(nf,outp) (pos=fixstring(this,data,len,pos,&nf,outp))

/* Gobble an identifier at the current position. */
#define GOBBLE_IDENTIFIER() dmalloc_touch (struct pike_string *, gobble_identifier(this,data,&pos))
#define READCHAR(C) (C=readchar(data,&pos,this))
#define DATA(X) INDEX_PCHARP(data,X)
#define HAS_PREFIX(X) \
  ((begins_with(X,ADD_PCHARP(data,pos),sizeof(X),len-pos,0)) ? (pos += NELEM(X)),1 : 0)


static void add_quoted_string( const void *str, ptrdiff_t len, int shift,
			       struct string_builder *dst )
{
  struct pike_string *x = make_shared_binary_pcharp( MKPCHARP(str,shift), len );
  string_builder_putchar( dst, '"' );
  string_builder_quote_string( dst, x, 0, 0x7fffffff, 0 );
  string_builder_putchar( dst, '"' );
  free_string(x);
}

static ptrdiff_t find_eos( struct cpp *this, const PCHARP data, ptrdiff_t len, ptrdiff_t pos )
{
    while(pos < len)
    {
      switch (DATA(pos++)) {
      case '\n':
	break;
      case '/':
	if (DATA(pos) == '/') {
	  pos = find_end_of_line(this,data,len,pos,0);
	  break;
	} else if (DATA(pos) == '*') {
	  pos = find_end_of_comment(this,data,len,pos,0);
	}
	continue;
      case '\\':
	if (DATA(pos) == '\n') {
	  pos+=1;
	} else if ((DATA(pos) == '\r') &&
		   (DATA(pos+1) == '\n')) {
	  pos+=2;
	} else {
	    continue;
	}
        this->current_line++;
      default:
	continue;
      }
      this->current_line++;
      break;
    }
    return pos;
}

static ptrdiff_t skipwhite(struct cpp *this, const PCHARP data, ptrdiff_t pos)
{
  do 
  {
    int c;
    if(!wide_isspace(c=DATA(pos)))
    {
      if (c == '\\') 
      {
	if (DATA(pos+1) == '\n') 
	{
	  pos += 2;
	  PUTNL();
	  this->current_line++;
	  continue;
	} else if ((DATA(pos+1) == '\r') &&
		   (DATA(pos+2) == '\n')) {
	  pos += 3;
	  PUTNL();
	  this->current_line++;
	  continue;
	}
      }
      break;
    }
    else if(c=='\n') 
    { 
      PUTNL(); this->current_line++; 
    }
    pos++;
  } while(1);
  return pos;
}

static ptrdiff_t skipspace(struct cpp *this, const PCHARP data, ptrdiff_t pos, int emit)
{
  do {
    int c;
    while (wide_isspace(c=DATA(pos)) && c!='\n') {
      pos++;
    }
    if (c == '\\') {
      if (DATA(pos+1) == '\n') {
	pos+=2;
      } else if ((DATA(pos+1) == '\r') &&
		 (DATA(pos+2) == '\n')) {
	pos+=3;
      } else {
	break;
      }
    } else {
      break;
    }
    if( emit )
    {
      PUTNL();
      this->current_line++;
    }
  } while (1);
  return pos;
}

static const char eq_[] = { '=', '=' };
static const char ne_[] = { '!', '=' };
static const char land_[] = { '&', '&' };
static const char lor_[] = { '|', '|' };
static const char string_recur_[] =
    { 's', 't', 'r', 'i', 'n', 'g', '_', 'r', 'e', 'c', 'u', 'r' };
static const char include_recur_[] =
  { 'i', 'n', 'c', 'l', 'u', 'd', 'e', '_', 'r', 'e', 'c', 'u', 'r' };
static const char line_[] = { 'l', 'i', 'n', 'e' };
static const char string_[] = { 's', 't', 'r', 'i', 'n', 'g' };
static const char include_[] = { 'i', 'n', 'c', 'l', 'u', 'd', 'e' };
static const char if_[] = { 'i', 'f' };
static const char ifdef_[] = { 'i', 'f', 'd', 'e', 'f' };
static const char ifndef_[] = { 'i', 'f', 'n', 'd', 'e', 'f' };
static const char endif_[] = { 'e', 'n', 'd', 'i', 'f' };
static const char else_[] = { 'e', 'l', 's', 'e' };
static const char elseif_[] = { 'e', 'l', 's', 'e', 'i', 'f' };
static const char elif_[] = { 'e', 'l', 'i', 'f' };
static const char error_[] = { 'e', 'r', 'r', 'o', 'r' };
static const char define_[] = { 'd', 'e', 'f', 'i', 'n', 'e' };
static const char undef_[] = { 'u', 'n', 'd', 'e', 'f' };
static const char undefine_[] = { 'u', 'n', 'd', 'e', 'f', 'i', 'n', 'e' };
static const char charset_[] = { 'c', 'h', 'a', 'r', 's', 'e', 't' };
static const char pragma_[] = { 'p', 'r', 'a', 'g', 'm', 'a' };
static const char pike_[] = { 'p', 'i', 'k', 'e' };
static const char require_[] = { 'r', 'e', 'q', 'u', 'i', 'r', 'e' };
static const char warning_[] = { 'w', 'a', 'r', 'n', 'i', 'n', 'g' };
static const char lsh_[] = { '<', '<' };
static const char rsh_[] = { '>', '>' };

static int begins_with( const char *prefix, const PCHARP stack, int len, int remain, int whole )
{
  int i;
  if( len > remain )
    return 0;

  for( i=0; i<len; i++ )
    if( INDEX_PCHARP(stack,i) != prefix[i] )
      return 0;

  if( whole && len != remain && wide_isidchar(INDEX_PCHARP(stack,len)) )
    return 0;

  return 1;
}

static ptrdiff_t low_cpp(struct cpp *this,
                         PCHARP data,
                         ptrdiff_t len,
                         int flags,
                         int auto_convert,
                         struct pike_string *charset);
static void insert_callback_define(struct cpp *this,
                                   struct define *def,
                                   struct define_argument *args,
                                   struct string_builder *tmp);
static void insert_callback_define_no_args(struct cpp *this,
                                           struct define *def,
                                           struct define_argument *args,
                                           struct string_builder *tmp);
static void insert_pragma(struct cpp *this,
			  struct define *def,
			  struct define_argument *args,
			  struct string_builder *tmp);

static ptrdiff_t calc1(struct cpp *this, PCHARP data, ptrdiff_t len,
		       ptrdiff_t pos, int flags);

static ptrdiff_t calcC(struct cpp *this, PCHARP data, ptrdiff_t len,
		       ptrdiff_t pos, int flags)
{
  SKIPWHITE();

  CALC_DUMPPOS("calcC");

  switch(DATA(pos))
  {
  case '(':
    pos=calc1(this,data,len,pos+1,flags);
    SKIPWHITE();
    if(!GOBBLE(')'))
      cpp_error(this, "Missing ')'");
    break;

  case '0':
    if(DATA(pos+1)=='x' || DATA(pos+1)=='X')
    {
      PCHARP p = data;
      INC_PCHARP(p,pos + 2);
      push_int(0);
      safe_wide_string_to_svalue_inumber(Pike_sp-1, p.ptr, &p.ptr, 16, 0, p.shift);
      if(!OUTP()) pop_stack();
      pos = SUBTRACT_PCHARP(p,data);
      break;
    }

  case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9':
  {
    PCHARP p1,p2;
    PCHARP p;
    double f;
    long l;

    /* FIXME: Support bignums. */

    p = ADD_PCHARP(data,pos);
    f = STRTOD_PCHARP(p, &p1);
    l = STRTOL_PCHARP(p, &p2, 0);
    if(COMPARE_PCHARP(p1,>,p2))
    {
      if(OUTP())
	push_float(DO_NOT_WARN((FLOAT_TYPE)f));
      pos = SUBTRACT_PCHARP(p1,data);
    }else{
      if(OUTP())
	push_int(l);
      pos = SUBTRACT_PCHARP(p2,data);
    }
    break;
  }

  case '\'':
  {
    p_wchar2 tmp = DATA(++pos);
    if (tmp == '\\') READCHAR(tmp);
    pos++;
    // FIXME: Multi char char constants here as well.
    if(!GOBBLE('\''))
      cpp_error(this, "Missing end quote in character constant.");
    if(OUTP())
      push_int(tmp);
    break;
  }

  case '"':
  {
    struct string_builder s;
    init_string_builder(&s, 0);
    READSTRING(s);
    if(OUTP())
      push_string(finish_string_builder(&s));
    else
      free_string_builder(&s);
    break;
  }

  default: {
    struct pike_string *func_name = gobble_identifier(this,data,&pos);
    if (func_name || DATA(pos) == '.') {
      /* NOTE: defined() can not be handled here,
       *       since the argument must not be expanded.
       */

      SKIPWHITE();

      if ( (func_name==constant_str || func_name==efun_str) &&
           DATA(pos) == '(')
      {
	int start, end;
	int arg = 0;
	INT_TYPE start_line;

        if( func_name==efun_str && !CPP_TEST_COMPAT(this,7,9) )
          cpp_warning(this, "Directive efun() deprecated.");

	pos++; /* GOBBLE('(') */

	start_line = this->current_line;

	SKIPWHITE();

	start = end = pos;
	while (DATA(pos) != ')') {
	  switch(DATA(pos++)) {
	  case '(':
	    pos = find_end_parenthesis(this, data, len, pos);
	    break;
	  case ',':
	    push_string(make_shared_binary_pcharp(ADD_PCHARP(data,start), end-start));
	    arg++;
	    start = pos;
	    break;
	  case '/':
	    if (DATA(pos) == '*') {
	      pos++;
	      if (this->keep_comments) {
		start = pos - 2;
	        SKIPCOMMENT_INC_LINES();
	      } else 
		SKIPCOMMENT();
	    } else if (DATA(pos) == '/') {
	      if (this->keep_comments) {
		start = pos - 1;
		FIND_EOL_PRETEND();
	      } else FIND_EOL();
	    }
	    break;
	  case '\0':
	    if (pos > len) {
	      INT_TYPE old_line = this->current_line;
	      this->current_line = start_line;
	      cpp_error_sprintf(this, "Missing ) in the meta function %S().",
				func_name);
	      this->current_line = old_line;
	      free_string (func_name);
	      return pos-1;
	    }
	    /* FALL_THROUGH */
	  default:
	    if (WC_ISSPACE(DATA(pos-1))) {
	      SKIPWHITE();
	      continue;
	    }
	    break;
	  }
	  end = pos;
	}

	if (start != end) {
	  push_string(make_shared_binary_pcharp(ADD_PCHARP(data,start), end-start));
	  arg++;
	}

	if(!GOBBLE(')')) {
	  INT_TYPE old_line = this->current_line;
	  this->current_line = start_line;
	  cpp_error_sprintf(this, "Missing ) in the meta function %S().",
			    func_name);
	  this->current_line = old_line;
	}
	/* NOTE: cpp_func MUST protect against errors. */
	if(OUTP())
        {
          if (arg != 1) {
            cpp_error_sprintf(this, "Bad number of arguments to %S().",
                              func_name);
            pop_n_elems(arg);
            push_int(0);
          }
          else
            cpp_constant(this, 0);
        }
	else
	  pop_n_elems(arg);
      } else if (DATA(pos) == '.') {
	if (func_name == NULL)
	  add_ref((func_name = empty_pike_string));
	while (GOBBLE('.')) {
	  struct pike_string *ind_name;
	  SKIPWHITE();
	  ind_name = gobble_identifier(this,data,&pos);
	  if (ind_name == NULL) {
	    cpp_error_sprintf(this, "Syntax error in #if missing identifier after '.'.");
	    free_string (func_name);
	    func_name = NULL;
	    break;
	  }
	  if(OUTP()) {
	    push_string (func_name);
	    push_text (".");
	    push_string (ind_name);
	    f_add(3);
	    func_name = Pike_sp[-1].u.string;
	    --Pike_sp;
	  }
	  SKIPWHITE();
	}
	if (func_name == NULL)
	  break;
	if(OUTP())
        {
          ref_push_string(func_name);
          cpp_constant(this, 1);
        }
      } else {
	if(OUTP())
	  push_int(0);
      }
      free_string (func_name);
      break;
    }

    cpp_error_sprintf(this, "Syntax error in #if bad character %c (%d).",
		      DATA(pos), DATA(pos));
    break;
  }
  }

  SKIPWHITE();

  while(GOBBLE('['))
  {
    CALC_DUMPPOS("inside calcC");
    pos=calc1(this,data,len,pos,flags);
    if(OUTP())
      f_index(2);

    SKIPWHITE();
    if(!GOBBLE(']'))
      cpp_error(this, "Missing ']'.");
  }
  CALC_DUMPPOS("after calcC");
  return pos;
}

static ptrdiff_t calcB(struct cpp *this, PCHARP data, ptrdiff_t len,
		       ptrdiff_t pos, int flags)
{
  CALC_DUMPPOS("before calcB");

  SKIPWHITE();
  switch(DATA(pos))
  {
    case '-': pos++; pos=calcB(this,data,len,pos,flags);
      if(OUTP()) o_negate(); break;
    case '!': pos++; pos=calcB(this,data,len,pos,flags);
      if(OUTP()) o_not(); break;
    case '~': pos++; pos=calcB(this,data,len,pos,flags);
      if(OUTP()) o_compl(); break;
    default: pos=calcC(this,data,len,pos,flags);
  }
  CALC_DUMPPOS("after calcB");
  return pos;
}

static ptrdiff_t calcA(struct cpp *this,PCHARP data, ptrdiff_t len,
		       ptrdiff_t pos, int flags)
{
  CALC_DUMPPOS("before calcA");

  pos=calcB(this,data,len,pos,flags);
  while(1)
  {
    CALC_DUMPPOS("inside calcA");
    SKIPWHITE();
    switch(DATA(pos))
    {
      case '/':
	if(DATA(pos+1)=='/' ||
	   DATA(pos+1)=='*')
	  return pos;
	pos++;
	pos=calcB(this,data,len,pos,flags);
	if(OUTP())
	  o_divide();
	continue;

      case '*':
	pos++;
	pos=calcB(this,data,len,pos,flags);
	if(OUTP())
	  o_multiply();
	continue;

      case '%':
	pos++;
	pos=calcB(this,data,len,pos,flags);
	if(OUTP())
	  o_mod();
	continue;
    }
    break;
  }
  CALC_DUMPPOS("after calcA");
  return pos;
}

static ptrdiff_t calc9(struct cpp *this, PCHARP data, ptrdiff_t len,
		       ptrdiff_t pos, int flags)
{
  CALC_DUMPPOS("before calc9");

  pos=calcA(this,data,len,pos,flags);

  while(1)
  {
    CALC_DUMPPOS("inside calc9");
    SKIPWHITE();
    switch(DATA(pos))
    {
      case '+':
	pos++;
	pos=calcA(this,data,len,pos,flags);
	if(OUTP())
	  f_add(2);
	continue;

      case '-':
	pos++;
	pos=calcA(this,data,len,pos,flags);
	if(OUTP())
	  o_subtract();
	continue;
    }
    break;
  }

  CALC_DUMPPOS("after calc9");
  return pos;
}

static ptrdiff_t calc8(struct cpp *this, PCHARP data, ptrdiff_t len,
		       ptrdiff_t pos, int flags)
{
  CALC_DUMPPOS("before calc8");

  pos=calc9(this,data,len,pos,flags);

  while(1)
  {
    CALC_DUMPPOS("inside calc8");
    SKIPWHITE();
    if(HAS_PREFIX(lsh_))
    {
      CALC_DUMPPOS("Found <<");
      pos=calc9(this,data,len,pos,flags);
      if(OUTP())
	o_lsh();
      break;
    }

    if(HAS_PREFIX(rsh_))
    {
      CALC_DUMPPOS("Found >>");
      pos=calc9(this,data,len,pos,flags);
      if(OUTP())
	o_rsh();
      break;
    }

    break;
  }
  return pos;
}

static ptrdiff_t calc7b(struct cpp *this, PCHARP data, ptrdiff_t len,
			ptrdiff_t pos, int flags)
{
  CALC_DUMPPOS("before calc7b");

  pos=calc8(this,data,len,pos,flags);

  while(1)
  {
    CALC_DUMPPOS("inside calc7b");

    SKIPWHITE();

    switch(DATA(pos))
    {
      case '<':
	if(DATA(pos+1) == '<') break;
	pos++;
	if(GOBBLE('='))
	{
	   pos=calc8(this,data,len,pos,flags);
	   if(OUTP())
	     f_le(2);
	}else{
	   pos=calc8(this,data,len,pos,flags);
	   if(OUTP())
	     f_lt(2);
	}
	continue;

      case '>':
	if(DATA(pos+1) == '>') break;
	pos++;
	if(GOBBLE('='))
	{
	   pos=calc8(this,data,len,pos,flags);
	   if(OUTP())
	     f_ge(2);
	}else{
	   pos=calc8(this,data,len,pos,flags);
	   if(OUTP())
	     f_gt(2);
	}
	continue;
    }
    break;
  }
  return pos;
}

static ptrdiff_t calc7(struct cpp *this, PCHARP data, ptrdiff_t len,
		       ptrdiff_t pos, int flags)
{
  CALC_DUMPPOS("before calc7");

  pos=calc7b(this,data,len,pos,flags);

  while(1)
  {
    CALC_DUMPPOS("inside calc7");

    SKIPWHITE();
    if(HAS_PREFIX(eq_))
    {
      pos=calc7b(this,data,len,pos,flags);
      if(OUTP())
	f_eq(2);
      continue;
    }

    if(HAS_PREFIX(ne_))
    {
      pos=calc7b(this,data,len,pos,flags);
      if(OUTP())
	f_ne(2);
      continue;
    }

    break;
  }
  return pos;
}

static ptrdiff_t calc6(struct cpp *this, PCHARP data, ptrdiff_t len,
		       ptrdiff_t pos, int flags)
{
  CALC_DUMPPOS("before calc6");

  pos=calc7(this,data,len,pos,flags);

  SKIPWHITE();
  while(DATA(pos) == '&' && DATA(pos+1)!='&')
  {
    CALC_DUMPPOS("inside calc6");

    pos++;
    pos=calc7(this,data,len,pos,flags);
    if(OUTP())
      o_and();
  }
  return pos;
}

static ptrdiff_t calc5(struct cpp *this, PCHARP data, ptrdiff_t len,
		       ptrdiff_t pos, int flags)
{
  CALC_DUMPPOS("before calc5");

  pos=calc6(this,data,len,pos,flags);

  SKIPWHITE();
  while(GOBBLE('^'))
  {
    CALC_DUMPPOS("inside calc5");

    pos=calc6(this,data,len,pos,flags);
    if(OUTP())
      o_xor();
  }
  return pos;
}

static ptrdiff_t calc4(struct cpp *this, PCHARP data, ptrdiff_t len,
		       ptrdiff_t pos, int flags)
{
  CALC_DUMPPOS("before calc4");

  pos=calc5(this,data,len,pos,flags);

  SKIPWHITE();
  while(DATA(pos) == '|' && DATA(pos+1)!='|')
  {
    CALC_DUMPPOS("inside calc4");
    pos++;
    pos=calc5(this,data,len,pos,flags);
    if(OUTP())
      o_or();
  }
  return pos;
}

static ptrdiff_t calc3(struct cpp *this, PCHARP data, ptrdiff_t len,
		       ptrdiff_t pos, int flags)
{

  CALC_DUMPPOS("before calc3");

  pos=calc4(this,data,len,pos,flags);

  SKIPWHITE();
  while(HAS_PREFIX(land_))
  {
    CALC_DUMPPOS("inside calc3");

    if(OUTP()) {
      check_destructed(Pike_sp-1);
      if(UNSAFE_IS_ZERO(Pike_sp-1))
      {
	pos=calc4(this,data,len,pos,flags|CPP_REALLY_NO_OUTPUT);
      }else{
	pop_stack();
	pos=calc4(this,data,len,pos,flags);
      }
    } else
      pos=calc4(this,data,len,pos,flags);
  }
  return pos;
}

static ptrdiff_t calc2(struct cpp *this, PCHARP data, ptrdiff_t len,
		       ptrdiff_t pos, int flags)
{

  CALC_DUMPPOS("before calc2");

  pos=calc3(this,data,len,pos,flags);

  SKIPWHITE();
  while(HAS_PREFIX(lor_))
  {
    CALC_DUMPPOS("inside calc2");

    if(OUTP()) {
      check_destructed(Pike_sp-1);
      if(!UNSAFE_IS_ZERO(Pike_sp-1))
      {
	pos=calc3(this,data,len,pos,flags|CPP_REALLY_NO_OUTPUT);
      }else{
	pop_stack();
	pos=calc3(this,data,len,pos,flags);
      }
    } else
      pos=calc3(this,data,len,pos,flags);
  }
  return pos;
}

static ptrdiff_t calc1(struct cpp *this, PCHARP data, ptrdiff_t len,
		       ptrdiff_t pos, int flags)
{
  CALC_DUMPPOS("before calc1");

  pos=calc2(this,data,len,pos,flags);

  SKIPWHITE();

  if(GOBBLE('?'))
  {
    int select = -1;
    if(OUTP()) {
      check_destructed(Pike_sp-1);
      select = (UNSAFE_IS_ZERO(Pike_sp-1)?0:1);
      pop_stack();
    }
    pos=calc1(this,data,len,pos,(select == 1? flags:(flags|CPP_REALLY_NO_OUTPUT)));
    if(!GOBBLE(':'))
      cpp_error(this, "Colon expected.");
    pos=calc1(this,data,len,pos,(select == 0? flags:(flags|CPP_REALLY_NO_OUTPUT)));
  }
  return pos;
}

static ptrdiff_t calc(struct cpp *this, PCHARP data, ptrdiff_t len,
		      ptrdiff_t tmp, int flags)
{
  JMP_BUF recovery;
  ptrdiff_t pos;

  CALC_DUMPPOS("Calculating");

  if (SETJMP(recovery))
  {
    cpp_handle_exception (this, "Error evaluating expression.");
    pos=tmp;
    FIND_EOL();
    push_int(0);
  }else{
    pos=calc1(this,data,len,tmp,flags);
    check_destructed(Pike_sp-1);
  }
  UNSETJMP(recovery);

  CALC_DUMPPOS("Done");

  return pos;
}

#include "preprocessor.h"

/*** Magic defines ***/

/*! @decl constant __LINE__
 *!
 *! This define contains the current line number, represented as an
 *! integer, in the source file.
 */
static void insert_current_line(struct cpp *this,
				struct define *UNUSED(def),
				struct define_argument *UNUSED(args),
				struct string_builder *tmp)
{
  string_builder_sprintf(tmp, " %ld ", (long)this->current_line);
}

/*! @decl constant __FILE__
 *!
 *! This define contains the file path and name of the source file.
 */
static void insert_current_file_as_string(struct cpp *this,
					  struct define *UNUSED(def),
					  struct define_argument *UNUSED(args),
					  struct string_builder *tmp)
{
  PUSH_STRING_SHIFT(this->current_file->str, this->current_file->len,
		    this->current_file->size_shift, tmp);
}

/*! @decl constant __DIR__
 *!
 *! This define contains the directory path of the source file.
 */
static void insert_current_dir_as_string(struct cpp *this,
                                         struct define *UNUSED(def),
                                         struct define_argument *UNUSED(args),
                                         struct string_builder *tmp)
{
  ref_push_string(this->current_file);
  /* FIXME: This isn't safe if the master hasn't been compiled yet. */
  SAFE_APPLY_MASTER("dirname",1);
  PUSH_STRING_SHIFT(Pike_sp[-1].u.string->str, Pike_sp[-1].u.string->len,
                    Pike_sp[-1].u.string->size_shift, tmp);
  pop_stack();
}

/*! @decl constant __TIME__
 *!
 *! This define contains the current time at the time of compilation,
 *! e.g. "12:20:51".
 */
static void insert_current_time_as_string(struct cpp *UNUSED(this),
					  struct define *UNUSED(def),
					  struct define_argument *UNUSED(args),
					  struct string_builder *tmp)
{
  /* FIXME: Is this code safe? */
  time_t tmp2;
  char *buf;
  time(&tmp2);
  buf=ctime(&tmp2);

  PUSH_STRING0((p_wchar0 *)buf+11, 8, tmp);
}

/*! @decl constant __DATE__
 *!
 *! This define contains the current date at the time of compilation,
 *! e.g. "Jul 28 2001".
 */
static void insert_current_date_as_string(struct cpp *UNUSED(this),
					  struct define *UNUSED(def),
					  struct define_argument *UNUSED(args),
					  struct string_builder *tmp)
{
  /* FIXME: Is this code safe? */
  time_t tmp2;
  char *buf;
  time(&tmp2);
  buf=ctime(&tmp2);

  PUSH_STRING0((p_wchar0 *)buf+4, 6, tmp);
  PUSH_STRING0((p_wchar0 *)buf+19, 5, tmp);
}

/*! @decl constant __VERSION__
 *!
 *! This define contains the current Pike version as a float. If
 *! another Pike version is emulated, this define is updated
 *! accordingly.
 *!
 *! @seealso
 *!   @[__REAL_VERSION__]
 */
static void insert_current_version(struct cpp *this,
				   struct define *UNUSED(def),
				   struct define_argument *UNUSED(args),
				   struct string_builder *tmp)
{
  string_builder_sprintf(tmp, " %d.%d ", this->compat_major,
			 this->compat_minor);
}


/*! @decl constant __MINOR__
 *! This define contains the minor part of the current Pike version,
 *! represented as an integer. If another Pike version is emulated,
 *! this define is updated accordingly.
 *!
 *! @seealso
 *!   @[__REAL_MINOR__]
 */
static void insert_current_minor(struct cpp *this,
				 struct define *UNUSED(def),
				 struct define_argument *UNUSED(args),
				 struct string_builder *tmp)
{
  string_builder_sprintf(tmp, " %d ", this->compat_minor);
}

/*! @decl int(1..) __COUNTER__
 *! This define contains a unique counter (unless it has been expanded
 *! Inte.NATIVE_MAX times) represented as an integer.
 *!
 */
static void insert_current_counter(struct cpp *UNUSED(this),
				 struct define *UNUSED(def),
				 struct define_argument *UNUSED(args),
				 struct string_builder *tmp)
{
  static int counter = 0;
  string_builder_sprintf(tmp, " %d ", ++counter);
}

/*! @decl constant __MAJOR__
 *!
 *! This define contains the major part of the current Pike version,
 *! represented as an integer. If another Pike version is emulated,
 *! this define is updated accordingly.
 *!
 *! @seealso
 *!   @[__REAL_MAJOR__]
 */
static void insert_current_major(struct cpp *this,
				 struct define *UNUSED(def),
				 struct define_argument *UNUSED(args),
				 struct string_builder *tmp)
{
  string_builder_sprintf(tmp, " %d ", this->compat_major);
}

/* _Pragma(STRING) */
/*! @decl void _Pragma(string directive)
 *!
 *! This macro inserts the corresponding @[#pragma] @[directive]
 *! in the source.
 *!
 *! e.g. @expr{_Pragma("strict_types")@} is the same
 *! as @expr{#pragma strict_types@} .
 *!
 *! @seealso
 *!   @[#pragma]
 */
static void insert_pragma(struct cpp *this,
			  struct define *UNUSED(def),
			  struct define_argument *args,
			  struct string_builder *tmp)
{
  int i;
  int in_string = 0;
  PCHARP arg = args->arg;
  ptrdiff_t len = args->len;

  /* Make some reasonable amount of space. */
  string_build_mkspace(tmp, len + 20, arg.shift);

  string_builder_strcat(tmp, "\n#pragma ");

  /* Destringize the argument. */
  for (i = 0; i < len; i++) {
    p_wchar2 ch = INDEX_PCHARP(arg, i);
    switch(ch) {
    case '\n': case '\r':
      ch = ' ';
      /* FALL_THROUGH */
    case ' ': case '\t':
      if (in_string) {
	string_builder_putchar(tmp, ch);
      }
      break;
    case '\"':
      in_string = !in_string;
      break;
    case '\\':
      if (in_string) {
	ch = (++i < len) ? INDEX_PCHARP(arg, i) : '\0';
	if ((ch != '\\') && (ch != '\"')) {
	  cpp_error(this, "Invalid \\-escape in _Pragma().");
	  break;
	}
      }
      /* FALL_THROUGH */
    default:
      if (in_string) {
	string_builder_putchar(tmp, ch);
      } else {
	cpp_error(this, "Invalid character outside of string.");
      }
      break;
    }
  }

  if (in_string) {
    cpp_error(this, "Unterminated string constant.");
  }

  string_builder_sprintf(tmp, "\n#line %ld ", (long)this->current_line);
  PUSH_STRING_SHIFT(this->current_file->str,
		    this->current_file->len,
		    this->current_file->size_shift,
		    tmp);
  string_builder_putchar(tmp, '\n');

}

static void insert_callback_define(struct cpp *this,
                                   struct define *def,
                                   struct define_argument *args,
                                   struct string_builder *tmp)
{
  ref_push_string( def->link.s );
  push_string( make_shared_binary_pcharp( args[0].arg, args[0].len ) );
  if (safe_apply_handler( "evaluate_define",
			  this->handler, this->compat_handler, 2, 0 ) &&
      TYPEOF(sp[-1]) == T_STRING ) {
    string_builder_shared_strcat(tmp, sp[-1].u.string);
    if( !this->prefix ){
        int min;
        check_string_range( sp[-1].u.string, 0, &min, 0 );
        if( min < 32 )
        {
            string_builder_sprintf(tmp, "\n#line %ld ", (long)this->current_line);
            insert_current_file_as_string( this,def,args,tmp);
            string_builder_putchar(tmp, '\n');
        }
    }
    pop_stack();
  }
}

static void insert_callback_define_no_args(struct cpp *this,
                                           struct define *def,
                                           struct define_argument *UNUSED(args),
                                           struct string_builder *tmp)
{
  struct svalue *save_sp = Pike_sp;
  ref_push_string( def->link.s );
  if (safe_apply_handler( "evaluate_define",
			  this->handler, this->compat_handler, 1, 0 ) &&
      TYPEOF(sp[-1]) == T_STRING )
    string_builder_shared_strcat(tmp, sp[-1].u.string);
  if (Pike_sp > save_sp) pop_n_elems(Pike_sp-save_sp);
}


/*! @decl constant __REAL_VERSION__
 *!
 *! This define always contains the version of the current Pike,
 *! represented as a float.
 *!
 *! @seealso
 *!   @[__VERSION__]
 */


/*! @decl constant __REAL_MAJOR__
 *!
 *! This define always contains the major part of the version of the
 *! current Pike, represented as an integer.
 *!
 *! @seealso
 *!   @[__MAJOR__]
 */


/*! @decl constant __REAL_MINOR__
 *!
 *! This define always contains the minor part of the version of the
 *! current Pike, represented as an integer.
 *!
 *! @seealso
 *!   @[__MINOR__]
 */

/*! @decl constant __BUILD__
 *! This constant contains the build number of the current Pike version,
 *! represented as an integer. If another Pike version is emulated,
 *! this constant remains unaltered.
 *!
 *! @seealso
 *!   @[__REAL_MINOR__]
 */

/*! @decl constant __REAL_BUILD__
 *!
 *! This define always contains the minor part of the version of the
 *! current Pike, represented as an integer.
 *!
 *! @seealso
 *!   @[__BUILD__]
 */







/*! @decl constant static_assert
 *!
 *!   This define expands to the symbol @[_Static_assert].
 *!
 *!   It is the preferred way to perform static
 *!   (ie compile-time) assertions.
 *!
 *! @note
 *!   The macro can also be used to check for whether static assertions
 *!   are supported.
 *!
 *! @seealso
 *!   @[predef::_Static_assert()]
 */

/*! @decl constant __PIKE__
 *!
 *! This define is always true.
 */

/*! @decl constant __AUTO_BIGNUM__
 *!
 *! This define is defined when automatic bignum conversion is enabled.
 *! When enabled all integers will automatically be converted to
 *! bignums when they get bigger than what can be represented by
 *! an integer, hampering performance slightly instead of crashing
 *! the program.
 */

/*! @decl constant __NT__
 *!
 *! This define is defined when the Pike is running on a Microsoft Windows OS,
 *! not just Microsoft Windows NT, as the name implies.
 */

/*! @decl constant __amigaos__
 *!
 *! This define is defined when the Pike is running on Amiga OS.
 */

/*! @decl constant __OS2__
 *!
 *! This define is defined when the Pike is running on IBM OS/2.
 */

/*! @endnamespace */

/*! @decl string cpp(string data, mapping|string|void current_file, @
 *!                  int|string|void charset, object|void handler, @
 *!                  void|int compat_major, void|int compat_minor, @
 *!                  void|int picky_cpp)
 *!
 *! Run a string through the preprocessor.
 *!
 *! Preprocesses the string @[data] with Pike's builtin ANSI-C look-alike
 *! preprocessor. If the @[current_file] argument has not been specified,
 *! it will default to @expr{"-"@}. @[charset] defaults to @expr{"ISO-10646"@}.
 *!
 *! If the second argument is a mapping, no other arguments may follow.
 *! Instead, they have to be given as members of the mapping (if wanted).
 *! The following members are recognized:
 *!
 *! @mapping
 *! 	@member string "current_file"
 *!             Name of the current file. It is used for generating
 *!             #line directives and for locating include files.
 *! 	@member int|string "charset"
 *!             Charset to use when processing @expr{data@}.
 *! 	@member object "handler"
 *!             Compilation handler.
 *! 	@member int "compat_major"
 *!             Sets the major pike version used for compat handling.
 *! 	@member int "compat_minor"
 *!             Sets the minor pike version used for compat handling.
 *! 	@member int "picky_cpp"
 *!             Generate more warnings.
 *!	@member int "keep_comments"
 *! 		This option keeps @[cpp()] from removing comments.
 *!             Useful in combination with the prefix feature below.
 *! 	@member string "prefix"
 *! 		If a prefix is given, only prefixed directives will be
 *! 		processed. For example, if the prefix is @expr{"foo"@}, then
 *! 		@expr{#foo_ifdef COND@} and @expr{foo___LINE__@} would be
 *! 		processed, @expr{#ifdef COND@} and @expr{__LINE__@} would not.
 *! @endmapping
 *!
 *! @seealso
 *!   @[compile()]
 */

/* Doesn't free string_builder buf! */
static void free_cpp(struct cpp *this)
{
  if(this->defines)
    free_hashtable(this->defines, free_one_define);

  if(this->current_file)
    free_string(this->current_file);

  if(this->handler) {
    free_object(this->handler);
    this->handler = 0;
  }

  if(this->compat_handler) {
    free_object(this->compat_handler);
    this->compat_handler=0;
  }

  if(this->data)
    free_string(this->data);

  if(this->prefix)
    free_string(this->prefix);
}


void f_cpp(INT32 args)
{
  struct cpp this;
  struct svalue *save_sp = sp - args;
  struct mapping *predefs = NULL;

  struct pike_string *data, *prefix = NULL;

  struct pike_string *current_file = 0;

  struct svalue *charset_sv = 0;
  int auto_convert = 0;
  struct pike_string *charset = NULL;

  struct object *handler = 0;

  int compat_major = 0, compat_minor = 0, picky_cpp = 0;

  ONERROR err;
#ifdef PIKE_DEBUG
  ONERROR tmp;
#endif /* PIKE_DEBUG */

  this.prefix = NULL;
  this.current_line=1;
  this.compile_errors=0;
  this.defines=0;
  this.keep_comments = 0;
  this.dependencies_fail = 0;

#define TTS(type)	(((type) == PIKE_T_STRING && "string")	\
		      || ((type) == PIKE_T_MAPPING && "mapping")\
		      || ((type) == PIKE_T_ARRAY && "array")	\
		      || ((type) == PIKE_T_FLOAT && "float")	\
		      || ((type) == PIKE_T_INT && "int")	\
		      || ((type) == PIKE_T_OBJECT && "object")	\
		      || "mixed")

#define GET_TYPE(type, name)	((tmp = simple_mapping_string_lookup(m, name)) \
   && (TYPEOF(*(tmp)) == PIKE_T_##type || (Pike_error("Expected type %s,"\
       "got type %s for " name ".", TTS(PIKE_T_##type), TTS(TYPEOF(*tmp))), 0)))

  if (args > 1 && TYPEOF(Pike_sp[-args]) == PIKE_T_STRING
    && TYPEOF(Pike_sp[1-args]) == PIKE_T_MAPPING) {
    struct svalue *tmp;
    struct mapping *m = Pike_sp[1-args].u.mapping;

    data = Pike_sp[-args].u.string;

    if (GET_TYPE(STRING, "current_file")) current_file = tmp->u.string;
    if (GET_TYPE(STRING, "charset")) charset_sv = tmp;
    if (GET_TYPE(OBJECT, "handler")) handler = tmp->u.object;
    if (GET_TYPE(INT, "compat_major")) compat_major = tmp->u.integer;
    if (GET_TYPE(INT, "compat_minor")) compat_minor = tmp->u.integer;
    if (GET_TYPE(INT, "picky")) picky_cpp = tmp->u.integer;
    if (GET_TYPE(STRING, "prefix")) prefix = tmp->u.string;
    if (GET_TYPE(INT, "keep_comments")) this.keep_comments = tmp->u.integer;
#undef GET_TYPE
#undef TTS
  } else {
    get_all_args("cpp", args, "%t.%T%*%O%d%d%d%T", &data, &current_file,
		 &charset_sv, &handler, &compat_major, &compat_minor,
		 &picky_cpp, &this.prefix);
  }

  this.data = data;
  add_ref(data);

  if(current_file)
    add_ref(current_file);
  else
    current_file = make_shared_string("-");
  this.current_file = current_file;

  this.compat_major=PIKE_MAJOR_VERSION;
  this.compat_minor=PIKE_MINOR_VERSION;
  this.compat_handler = 0;
  this.handler = handler;
  if(handler)
    add_ref(handler);

  /* Don't call free_cpp before all variables are cleared or set. */
  SET_ONERROR(err, free_cpp, &this);

  if (prefix) {
      int i;
      if (prefix->size_shift) {
	  Pike_error("No widechars allowed in cpp prefix.\n");
      }
      for (i = 0; i < prefix->len; i++) {
	  if (!isalnum(prefix->str[i])) {
	      Pike_error("Invalid char in prefix.\n");
	  }
      }
      this.prefix = prefix;
      add_ref(prefix);
  }


  if(charset_sv) {
    if(TYPEOF(*charset_sv) == T_STRING) {
      charset = charset_sv->u.string;
      push_string(data);
      this.data = data = NULL;
      ref_push_string(charset);
      if (!safe_apply_handler ("decode_charset", this.handler,
			       this.compat_handler, 2, BIT_STRING)) {
	cpp_handle_exception (&this, "Error decoding with charset %S",
			      charset);
	Pike_error("Unknown charset.\n");
      }
      this.data = data = sp[-1].u.string;
      sp--;
      dmalloc_touch_svalue(sp);
    }
    else if(TYPEOF(*charset_sv) == T_INT)
      auto_convert = charset_sv->u.integer;
    else {
      SIMPLE_BAD_ARG_ERROR("cpp", 3, "string|int");
    }
  }

  if(compat_major)
    cpp_change_compat(&this, compat_major, compat_minor);

  this.picky_cpp = picky_cpp;

  if (use_initial_predefs)
    /* Typically compiling the master here. */
    predefs = initial_predefs_mapping();
  else {
    low_unsafe_apply_handler ("get_predefines", this.handler,
			      this.compat_handler, 0);
    if (!UNSAFE_IS_ZERO (sp - 1)) {
      struct keypair *k;
      int e, sprintf_args = 0;
      if (TYPEOF(sp[-1]) != T_MAPPING) {
	push_text ("Invalid return value from get_predefines\n");
	push_text ("Invalid return value from get_predefines, got %O\n");
	push_svalue (sp - 3);
	sprintf_args = 2;
      }
      else {
	predefs = copy_mapping (sp[-1].u.mapping);
	NEW_MAPPING_LOOP (predefs->data) {
	  if (TYPEOF(k->ind) != T_STRING || !k->ind.u.string->len) {
	    push_text ("Expected nonempty string as predefine name\n");
	    push_text ("Expected nonempty string as predefine name, got %O\n");
	    push_svalue (&k->ind);
	    sprintf_args = 2;
	    free_mapping (predefs);
	    predefs = NULL;
	    goto predef_map_error;
	  } else if (!(TYPEOF(k->val) == T_INT && !k->val.u.integer)
		   && TYPEOF(k->val) != T_STRING
		   && TYPEOF(k->val) != T_FUNCTION
		   && TYPEOF(k->val) != T_OBJECT) {

	     push_text ("expected zero, string or function value for"
				 " predefine\n");
	     push_text ("expected zero, string or function value for"
				 " predefine %O\n");
	     push_svalue (&k->ind);
	     sprintf_args = 2;
	     free_mapping (predefs);
	     predefs = NULL;
	     goto predef_map_error;
	  }
	}
      }
      if (!predefs) {
      predef_map_error:
	f_sprintf (sprintf_args);
	Pike_error("%S", sp[-1].u.string);
      }
    }
    pop_stack();
  }

  if (auto_convert && (!data->size_shift) && (data->len > 1)) {
    /* Try to determine if we need to recode the string */
    struct pike_string *new_data = recode_string(&this, data);
    free_string(data);
    this.data = data = new_data;
  }
  if (data->size_shift) {
    /* Get rid of any byte order marks (0xfeff) */
    struct pike_string *new_data = filter_bom(data);
    free_string(data);
    this.data = data = new_data;
  }

  init_string_builder(&this.buf, 0);

  /* These attempt to be compatible with the C standard. */
  do_magic_define(&this,"__LINE__",insert_current_line);
  do_magic_define(&this,"__FILE__",insert_current_file_as_string);
  do_magic_define(&this,"__DATE__",insert_current_date_as_string);
  do_magic_define(&this,"__TIME__",insert_current_time_as_string);

  /* These are from the 201x C standard. */
  do_magic_define(&this,"_Pragma",insert_pragma)->args = 1;
  simple_add_define(&this, "static_assert", "_Static_assert");

  do_magic_define(&this,"__COUNTER__",insert_current_counter);

  /* These are Pike extensions. */
  do_magic_define(&this,"__DIR__",insert_current_dir_as_string);
  do_magic_define(&this,"__VERSION__",insert_current_version);
  do_magic_define(&this,"__MAJOR__",insert_current_major);
  do_magic_define(&this,"__MINOR__",insert_current_minor);

  {
#if 0
    /* Left in place for documentation reference purposes. */
    struct define *def =
      alloc_empty_define(make_shared_string("__deprecated__"), 1);
    def->args = 1;
    REF_MAKE_CONST_STRING(def->first, "__attribute__(\"deprecated\", ");
    def->parts[0].argument = 0;
    REF_MAKE_CONST_STRING(def->parts[0].postfix, ")");
    this.defines = hash_insert(this.defines, &def->link);
#endif /* 0 */

    simple_add_define(&this, "__PIKE__", " 1 ");

    simple_add_define(&this, "__REAL_VERSION__",
		      " " DEFINETOSTR(PIKE_MAJOR_VERSION) "."
		      DEFINETOSTR(PIKE_MINOR_VERSION) " ");
    simple_add_define(&this, "__REAL_MAJOR__",
		      " " DEFINETOSTR(PIKE_MAJOR_VERSION) " ");
    simple_add_define(&this, "__REAL_MINOR__",
		      " " DEFINETOSTR(PIKE_MINOR_VERSION) " ");
    simple_add_define(&this, "__BUILD__",
		      " " DEFINETOSTR(PIKE_BUILD_VERSION) " ");
    simple_add_define(&this, "__REAL_BUILD__",
		      " " DEFINETOSTR(PIKE_BUILD_VERSION) " ");
    simple_add_define(&this, "__AUTO_BIGNUM__", " 1 ");
#ifdef __NT__
    simple_add_define(&this, "__NT__", " 1 ");
#endif
#ifdef __amigaos__
    simple_add_define(&this, "__amigaos__", " 1 ");
#endif
#ifdef __OS2__
    simple_add_define(&this, "__OS2__", " 1 ");
#endif
#ifdef __APPLE__
    simple_add_define(&this, "__APPLE__", " 1 ");
#endif
    simple_add_define(&this, "SIZEOF_INT",
		      " " DEFINETOSTR(SIZEOF_INT) " ");
    simple_add_define(&this, "SIZEOF_FLOAT",
		      " " DEFINETOSTR(SIZEOF_FLOAT) " ");
  }

  if (predefs) {
    struct keypair *k;
    int e;
    NEW_MAPPING_LOOP (predefs->data) {
      if (TYPEOF(k->val) == T_STRING)
	add_define (&this, k->ind.u.string, k->val.u.string);
      else if(TYPEOF(k->val) == T_FUNCTION || TYPEOF(k->val) == T_OBJECT)
      {
        struct define *def;
        if( index_shared_string( k->ind.u.string, k->ind.u.string->len-1) == ')' )
        {
          struct pike_string *s = string_slice( k->ind.u.string, 0, k->ind.u.string->len-2);
          def = alloc_empty_define( s, 0 );
          def->magic = insert_callback_define;
          def->varargs=1;
          def->args=1;
        }
        else
        {
          def = alloc_empty_define( k->ind.u.string, 0 );
          k->ind.u.string->refs++;
          def->magic = insert_callback_define_no_args;
        }
        this.defines = hash_insert( this.defines, &def->link );
      }
      else
	add_define (&this, k->ind.u.string, empty_pike_string);
    }
    free_mapping (predefs);
  }

  string_builder_binary_strcat(&this.buf, "#line 1 ", 8);
  PUSH_STRING_SHIFT(this.current_file->str, this.current_file->len,
		    this.current_file->size_shift, &this.buf);
  string_builder_putchar(&this.buf, '\n');

#ifdef PIKE_DEBUG
  SET_ONERROR(tmp, fatal_on_error, "Preprocessor exited with longjump!\n");
#endif /* PIKE_DEBUG */


  low_cpp(&this, MKPCHARP_STR(data), data->len,
	  0, auto_convert, charset);

#ifdef PIKE_DEBUG
  UNSET_ONERROR(tmp);
#endif /* PIKE_DEBUG */

  UNSET_ONERROR(err);
  free_cpp(&this);

  if(this.compile_errors)
  {
    free_string_builder(&this.buf);
    throw_error_object(fast_clone_object(cpp_error_program), 0, 0, 0,
		       "Cpp() failed\n");
  }
  else if(this.dependencies_fail)
  {
    free_string_builder(&this.buf);
    pop_n_elems(sp - save_sp);
    push_int(0);
  }
  else
  {
    pop_n_elems(sp - save_sp);
    push_string(finish_string_builder(&this.buf));
  }
}

/*! @module Builtin
 */

/*! @decl mapping(string:mixed) _take_over_initial_predefines()
 */
void f__take_over_initial_predefines (INT32 args)
{
  pop_n_elems (args);
  if (use_initial_predefs) {
    struct pike_predef_s *tmp;
    push_mapping (initial_predefs_mapping());
    use_initial_predefs = 0;

    while((tmp=first_predef))
    {
      free(tmp->name);
      free(tmp->value);
      first_predef=tmp->next;
      free(tmp);
    }
    last_predef = 0;
  }
  else Pike_error ("Initial predefines already taken over.\n");
}

/*! @endmodule
 */

void init_cpp()
{
  struct svalue s;

  defined_macro=alloc_empty_define(make_shared_string("defined"),0);
  defined_macro->magic=check_defined;
  defined_macro->args=1;

  efun_str = make_shared_string ("efun");
  constant_str = make_shared_string ("constant");
  defined_str = make_shared_string ("defined");

  use_initial_predefs = 1;

/* function(string, mapping|string|void, int|string|void, object|void, int|void, int|void) */
  ADD_EFUN("cpp", f_cpp, tFunc(tStr tOr(tMapping, tOr(tStr,tVoid))
			       tOr(tInt,tOr(tStr,tVoid))
			       tOr(tObj,tVoid)
			       tOr(tInt,tVoid)
			       tOr(tInt,tVoid)
			       tOr(tInt,tVoid)
			       , tStr),
	   /* OPT_SIDE_EFFECT since we might instantiate modules etc. */
	   OPT_EXTERNAL_DEPEND|OPT_SIDE_EFFECT);

  ADD_INT_CONSTANT("__HAVE_CPP_PREFIX_SUPPORT__", 1, 0);

  /* Somewhat tricky to add a _constant_ function in _static_modules.Builtin. */
  SET_SVAL(s, T_FUNCTION, FUNCTION_BUILTIN, efun,
	   make_callable (f__take_over_initial_predefines,
			  "_take_over_initial_predefines",
			  "function(void:mapping(string:string))",
			  OPT_SIDE_EFFECT, NULL, NULL));
  simple_add_constant ("_take_over_initial_predefines", &s, 0);
  free_svalue (&s);
}


void add_predefine(const char *s)
{
  struct pike_predef_s *tmp=ALLOC_STRUCT(pike_predef_s);
  char * pos=strchr(s,'=');
  if(pos)
  {
    tmp->name=xalloc(pos-s+1);
    memcpy(tmp->name,s,pos-s);
    tmp->name[pos-s]=0;

    tmp->value=xalloc(s+strlen(s)-pos);
    memcpy(tmp->value,pos+1,s+strlen(s)-pos);
  }else{
    tmp->name=xalloc(strlen(s)+1);
    memcpy(tmp->name,s,strlen(s)+1);

    tmp->value=xalloc(4);
    memcpy(tmp->value," 1 ",4);
  }
  tmp->next = NULL;
  if (first_predef) {
    last_predef->next = tmp;
    last_predef = tmp;
  }
  else first_predef = last_predef = tmp;
}

void exit_cpp(void)
{
#ifdef DO_PIKE_CLEANUP
  struct pike_predef_s *tmp;
  while((tmp=first_predef))
  {
    free(tmp->name);
    free(tmp->value);
    first_predef=tmp->next;
    free(tmp);
  }
  free_string(defined_macro->link.s);
  free(defined_macro);

  free_string (efun_str);
  free_string (constant_str);
  free_string (defined_str);
#endif
}
