/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "program.h"
#include "object.h"
#include "buffer.h"
#include "pike_types.h"
#include "stralloc.h"
#include "las.h"
#include "lex.h"
#include "pike_macros.h"
#include "fsort.h"
#include "pike_error.h"
#include "docode.h"
#include "interpret.h"
#include "main.h"
#include "pike_memory.h"
#include "gc.h"
#include "threads.h"
#include "constants.h"
#include "operators.h"
#include "builtin_functions.h"
#include "mapping.h"
#include "cyclic.h"
#include "opcodes.h"
#include "version.h"
#include "block_allocator.h"
#include "block_alloc.h"
#include "pikecode.h"
#include "pike_compiler.h"
#include "module_support.h"
#include "bitvector.h"
#include "sprintf.h"

#include <errno.h>
#include <fcntl.h>

static void exit_program_struct(struct program *);
static size_t add_xstorage(size_t size,
			   size_t alignment,
			   ptrdiff_t modulo_orig);

/* mapping(int:string) */
static struct mapping *reverse_symbol_table = NULL;

static struct block_allocator program_allocator = BA_INIT_PAGES(sizeof(struct program), 4);

ATTRIBUTE((malloc))
struct program * alloc_program(void) {
    return ba_alloc(&program_allocator);
}

void really_free_program(struct program * p) {
    exit_program_struct(p);
    ba_free(&program_allocator, p);
}

void count_memory_in_programs(size_t *num, size_t *_size) {
    size_t size;
    struct program *p;
    ba_count_all(&program_allocator, num, &size);
    for(p=first_program;p;p=p->next) {
	size+=p->total_size - sizeof (struct program);
    }
    *_size = size;
}

void free_all_program_blocks(void) {
    ba_destroy(&program_allocator);
}

/* #define COMPILER_DEBUG */
/* #define PROGRAM_BUILD_DEBUG */

#ifdef COMPILER_DEBUG
#define CDFPRINTF(...)	fprintf(stderr, __VA_ARGS__)
#ifndef PIKE_THREADS
/* The CDFPRINTF lines wants to print lock_depth, so fake one of those */
static const int lock_depth = 1;
#endif
#else /* !COMPILER_DEBUG */
#define CDFPRINTF(...)
#endif /* COMPILER_DEBUG */

/*
 * These two values should probably be fine-tuned, but doing so
 * more or less requires running a predictable 'typical' application
 * and testing different hashsizes and tresholds. I tried to do it
 * mathematically by measuring the extremes (no cache hits, 100%
 * cache hits etc.) but it seems that the processor cache becomes
 * exhausted in some of my measurements, which renders my mathematical
 * model useless.
 *
 * Further measurements seems to indicate that this cache can slow
 * things down a bit if the hit/miss rate is not fairly high.
 * For normal applications, the hitrate is most likely well over 90%,
 * but that should be verified.
 * - Hubbe
 */

/* Define the size of the cache that is used for method lookup. */
/* A value of zero disables this cache */
#define FIND_FUNCTION_HASHSIZE 16384

/* Programs with less methods will not use the cache for method lookups.. */
#define FIND_FUNCTION_HASH_TRESHOLD 0


#define DECLARE
#include "compilation.h"

struct pike_string *this_program_string, *this_string, *args_string;
static struct pike_string *this_function_string;
static struct pike_string *UNDEFINED_string;

/* Common compiler subsystems */
struct pike_string *parser_system_string;
struct pike_string *type_check_system_string;

struct pike_string *compat_lfun_destroy_string;

/* NOTE: There is a corresponding list to this one in
   Tools.AutoDoc.PikeObjects

   If new lfuns are added it might be beneficial to also add them to
   that list.
*/
const char *const lfun_names[]  = {
  "__INIT",
  "create",
  "_destruct",
  "`+",
  "`-",
  "`&",
  "`|",
  "`^",
  "`<<",
  "`>>",
  "`*",
  "`/",
  "`%",
  "`~",
  "`==",
  "`<",
  "`>",
  "__hash",
  "cast",
  "`!",
  "`[]",
  "`[]=",
  "`->",
  "`->=",
  "_sizeof",
  "_indices",
  "_values",
  "`()",
  "``+",
  "``-",
  "``&",
  "``|",
  "``^",
  "``<<",
  "``>>",
  "``*",
  "``/",
  "``%",
  "`+=",
  "_is_type",
  "_sprintf",
  "_equal",
  "_m_delete",
  "_get_iterator",
  "`[..]",
  /* NOTE: After this point there are only fake lfuns. */
  "_search",
  "_types",
  "_serialize",
  "_deserialize",
  "_size_object",
  "_random",
  "`**",
  "``**",
  "_sqrt",
};

struct pike_string *lfun_strings[NELEM(lfun_names)];

static struct mapping *lfun_ids;

/* mapping(string:type) */
static struct mapping *lfun_types;

static const char *const raw_lfun_types[] = {
  tFuncV(tNone,tVoid,tVoid),	/* "__INIT", */
  tFuncV(tNone,tZero,tVoid),	/* "create", */
  tFuncV(tOr(tVoid,tInt),tVoid,tVoid), /* "_destruct", */
  tFuncV(tZero,tZero,tMix),	/* "`+", */
  tFunc(tOr(tVoid,tZero),tMix),	/* "`-", */
  tFuncV(tNone,tZero,tMix),	/* "`&", */
  tFuncV(tNone,tZero,tMix),	/* "`|", */
  tFuncV(tNone,tZero,tMix),	/* "`^", */
  tFuncV(tZero,tVoid,tMix),	/* "`<<", */
  tFuncV(tZero,tVoid,tMix),	/* "`>>", */
  tFuncV(tNone,tZero,tMix),	/* "`*", */
  tFuncV(tNone,tZero,tMix),	/* "`/", */
  tFuncV(tNone,tZero,tMix),	/* "`%", */
  tFuncV(tNone,tVoid,tMix),	/* "`~", */
  tFuncV(tMix,tVoid,tInt),	/* "`==", */
  tFuncV(tMix,tVoid,tInt),	/* "`<", */
  tFuncV(tMix,tVoid,tInt),	/* "`>", */
  tFuncV(tNone,tVoid,tInt),	/* "__hash", */
  tFuncV(tString,tVoid,tMix),	/* "cast", */
  tFuncV(tNone,tVoid,tInt),	/* "`!", */
  tFuncV(tZero,tVoid,tMix),	/* "`[]", */
  tFuncV(tZero tSetvar(0,tZero),tVoid,tVar(0)),	/* "`[]=", */
  tFuncV(tStr tOr(tVoid,tObj) tOr(tVoid,tInt),tVoid,tMix),	/* "`->", */
  tFuncV(tStr tSetvar(0,tZero) tOr(tVoid,tObj) tOr(tVoid,tInt),tVoid,tVar(0)),	/* "`->=", */
  tFuncV(tOr(tVoid,tObj) tOr(tVoid,tInt),tVoid,tInt),	/* "_sizeof", */
  tFuncV(tOr(tVoid,tObj) tOr(tVoid,tInt),tVoid,tArray),	/* "_indices", */
  tFuncV(tOr(tVoid,tObj) tOr(tVoid,tInt),tVoid,tArray),	/* "_values", */
  tFuncV(tNone,tZero,tMix),	/* "`()", */
  tFuncV(tZero,tZero,tMix),	/* "``+", */
  tFuncV(tZero,tVoid,tMix),	/* "``-", */
  tFuncV(tNone,tZero,tMix),	/* "``&", */
  tFuncV(tNone,tZero,tMix),	/* "``|", */
  tFuncV(tNone,tZero,tMix),	/* "``^", */
  tFuncV(tZero,tVoid,tMix),	/* "``<<", */
  tFuncV(tZero,tVoid,tMix),	/* "``>>", */
  tFuncV(tNone,tZero,tMix),	/* "``*", */
  tFuncV(tNone,tZero,tMix),	/* "``/", */
  tFuncV(tNone,tZero,tMix),	/* "``%", */
  tFuncV(tZero,tZero,tMix),	/* "`+=", */
  tFuncV(tStr,tVoid,tInt),	/* "_is_type", */
  tFuncV(tInt tOr(tMap(tStr,tInt),tVoid),tVoid,tStr),	/* "_sprintf", */
  tFuncV(tMix,tVoid,tInt),	/* "_equal", */
  tFuncV(tZero,tVoid,tMix),	/* "_m_delete", */
  tFuncV(tNone,tVoid,tObj),	/* "_get_iterator", */
  tFuncV(tZero tRangeBound tZero tRangeBound, tVoid, tMix), /* "`[..]" */
  /* NOTE: After this point there are only fake lfuns. */
  tFuncV(tZero tOr(tZero, tVoid), tZero, tMix), /* "_search", */
  tFuncV(tNone,tVoid,tArray),	/* "_types", */
  tFuncV(tObj tZero, tVoid, tVoid),	/* "_serialize", */
  tFuncV(tObj tZero, tVoid, tVoid),	/* "_deserialize", */
  tFuncV(tNone, tVoid, tInt),	/* "_size_object", */
  tFuncV(tFunction tFunction, tVoid, tMix),	/* "_random", */
  tFuncV(tOr3(tInt,tFloat,tObj),tVoid,tOr3(tObj,tInt,tFloat)),	/* "pow", */
  tFuncV(tOr3(tInt,tFloat,tObj),tVoid,tOr3(tObj,tInt,tFloat)),	/* "rpow", */
  tFunc(tVoid,tMix),/* "_sqrt* */
};

/* These two are not true LFUNs! */
static struct pike_type *lfun_getter_type_string = NULL;
static struct pike_type *lfun_setter_type_string = NULL;

/*! @namespace lfun::
 *!
 *! Callback functions used to overload various builtin functions.
 *!
 *! The functions can be grouped into a few sets:
 *!
 *! @ul
 *!   @item
 *!     Object initialization and destruction.
 *!
 *!     @[__INIT()], @[create()], @[_destruct()]
 *!
 *!   @item
 *!     Unary operator overloading.
 *!
 *!     @[`~()], @[`!()],
 *!     @[_values()], @[cast()],
 *!     @[_sizeof()], @[_indices()],
 *!     @[__hash()]
 *!
 *!   @item
 *!     Binary asymmetric operator overloading.
 *!
 *!     @[`+()], @[``+()],
 *!     @[`-()], @[``-()],
 *!     @[`&()], @[``&()],
 *!     @[`|()], @[``|()],
 *!     @[`^()], @[``^()],
 *!     @[`<<()], @[``<<()],
 *!     @[`>>()], @[``>>()],
 *!     @[`*()], @[``*()],
 *!     @[`/()], @[``/()],
 *!     @[`%()], @[``%()]
 *!
 *!   @item
 *!     Binary symmetric operator overloading.
 *!
 *!     The optimizer will make assumptions about the relations
 *!     between these functions.
 *!
 *!     @[`==()], @[_equal()], @[`<()], @[`>()]
 *!
 *!   @item
 *!     Other binary operator overloading.
 *!
 *!     @[`[]()], @[`[]=()], @[`->()],
 *!     @[`->=()], @[`+=()], @[`()()]
 *!
 *!   @item
 *!     Overloading of other builtin functions.
 *!
 *!     @[_is_type()], @[_sprintf()], @[_m_delete()],
 *!     @[_get_iterator()], @[_search()]
 *! @endul
 *!
 *! @note
 *!   Although these functions are called from outside the object they
 *!   exist in, they will still be used even if they are declared
 *!   @expr{protected@}. It is in fact recommended to declare them
 *!   @expr{protected@}, since that will hinder them being used for
 *!   other purposes.
 *!
 *! @seealso
 *!   @[::]
 */

/*! @decl void lfun::__INIT()
 *!
 *!   Inherit and variable initialization.
 *!
 *!   This function is generated automatically by the compiler. It's
 *!   called just before @[lfun::create()] when an object is
 *!   instantiated.
 *!
 *!   It first calls any @expr{__INIT@} functions in inherited classes
 *!   (regardless of modifiers on the inherits). It then executes all
 *!   the variable initialization expressions in this class, in the
 *!   order they occur.
 *!
 *! @note
 *!   This function can not be overloaded or blocked from executing.
 *!
 *! @seealso
 *!   @[lfun::create()]
 */

/*! @decl void lfun::create(zero ... args)
 *!
 *!   Object creation callback.
 *!
 *!   This function is called right after @[lfun::__INIT()].
 *!
 *!   @[args] are the arguments passed when the program was called.
 *!
 *! @note
 *!   This function can be created implicitly
 *!   by the compiler using the syntax:
 *! @code
 *! class Foo(int foo) {
 *!   int bar;
 *! }
 *! @endcode
 *!   In the above case an implicit @[lfun::create()] is created, and
 *!   it's equivalent to:
 *! @code
 *! class Foo {
 *!   int foo;
 *!   int bar;
 *!   protected void create(int foo)
 *!   {
 *!     this::foo = foo;
 *!   }
 *! }
 *! @endcode
 *!
 *! @seealso
 *!   @[lfun::__INIT()], @[lfun::_destruct()]
 */

/*! @decl void lfun::_destruct (void|int reason)
 *!
 *!   Object destruction callback.
 *!
 *!   This function is called right before the object is destructed.
 *!   That can happen either through a call to @[predef::destruct()],
 *!   when there are no more references to the object, or when the
 *!   garbage collector discovers that it's part of a cyclic data
 *!   structure that has become garbage.
 *!
 *! @param reason
 *!   A flag that tells why the object is destructed:
 *!
 *!   @int
 *!     @value Object.DESTRUCT_EXPLICIT
 *!       Destructed explicitly by @[predef::destruct].
 *!     @value Object.DESTRUCT_NO_REFS
 *!       Destructed due to running out of references.
 *!     @value Object.DESTRUCT_GC
 *!       Destructed by the garbage collector.
 *!     @value Object.DESTRUCT_CLEANUP
 *!       Destructed as part of the cleanup when the pike process
 *!       exits. Occurs only if Pike has been compiled with the
 *!       configure option @tt{--with-cleanup-on-exit@}. See note
 *!       below.
 *!   @endint
 *!
 *! @note
 *! Objects are normally not destructed when a process exits, so
 *! @expr{_destruct@} functions aren't called then. Use @[atexit] to get
 *! called when the process exits.
 *!
 *! @note
 *! Regarding destruction order during garbage collection:
 *!
 *! If an object is destructed by the garbage collector, it's part of
 *! a reference cycle with other things but with no external
 *! references. If there are other objects with @expr{_destruct@}
 *! functions in the same cycle, it becomes a problem which to call
 *! first.
 *!
 *! E.g. if this object has a variable with another object which
 *! (directly or indirectly) points back to this one, you might find
 *! that the other object already has been destructed and the variable
 *! thus contains zero.
 *!
 *! The garbage collector tries to minimize such problems by defining
 *! an order as far as possible:
 *!
 *! @ul
 *! @item
 *!   If an object A contains an @[lfun::_destruct] and an object B does
 *!   not, then A is destructed before B.
 *! @item
 *!   If A references B single way, then A is destructed before B.
 *! @item
 *!   If A and B are in a cycle, and there is a reference somewhere
 *!   from B to A that is weaker than any reference from A to B, then
 *!   A is destructed before B.
 *! @item
 *!   If a cycle is resolved according to the rule above by ignoring a
 *!   weaker reference, and there is another ambiguous cycle that
 *!   would get resolved by ignoring the same reference, then the
 *!   latter cycle will be resolved by ignoring that reference.
 *! @item
 *!   Weak references (e.g. set with @[predef::set_weak_flag()]) are
 *!   considered weaker than normal references, and both are
 *!   considered weaker than strong references.
 *! @item
 *!   Strong references are those from objects to the objects of their
 *!   lexically surrounding classes. There can never be a cycle
 *!   consisting only of strong references. (This means the gc never
 *!   destructs a parent object before all children have been
 *!   destructed.)
 *! @endul
 *!
 *! An example with well defined destruct order due to strong
 *! references:
 *!
 *! @code
 *! class Super {
 *!   class Sub {
 *!     protected void _destruct() {
 *!       if (!Super::this)
 *!         error ("My parent has been destructed!\n");
 *!     }
 *!   }
 *!   Sub sub = Sub();
 *!   protected void _destruct() {
 *!     if (!sub)
 *!       werror ("sub already destructed.\n");
 *!   }
 *! }
 *! @endcode
 *!
 *! The garbage collector ensures that these objects are destructed in
 *! an order so that @expr{werror@} in @expr{Super@} is called and not
 *! @expr{error@} in @expr{Sub@}.
 *!
 *! @note
 *! When the garbage collector calls @[lfun::_destruct], all accessible
 *! non-objects and objects without @expr{_destruct@} functions are
 *! still intact. They are not freed if the @expr{_destruct@} function
 *! adds external references to them. However, all objects with
 *! @[lfun::_destruct] in the cycle are already scheduled for
 *! destruction and will therefore be destroyed even if external
 *! references are added to them.
 *!
 *! @seealso
 *!   @[lfun::create()], @[predef::destruct()]
 */

/*! @decl mixed lfun::`+(zero arg, zero ... rest)
 *!
 *!   Left side addition/concatenation callback.
 *!
 *!   This is used by @[predef::`+]. It's called with any arguments
 *!   that follow this object in the argument list of the call to
 *!   @[predef::`+]. The returned value should be a new instance that
 *!   represents the addition/concatenation between this object and
 *!   the arguments in the order they are given.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[lfun::``+()], @[lfun::`+=()], @[predef::`+()]
 */

/*! @decl this_program lfun::`+=(zero arg, zero ... rest)
 *!
 *!   Destructive addition/concatenation callback.
 *!
 *!   This is used by @[predef::`+]. It's called with any arguments
 *!   that follow this object in the argument list of the call to
 *!   @[predef::`+]. It should update this object to represent the
 *!   addition/concatenation between it and the arguments in the order
 *!   they are given. It should always return this object.
 *!
 *! @note
 *!   This function should only be implemented if @[lfun::`+()] also
 *!   is. It should only work as a more optimized alternative to that
 *!   one, for the case when it's safe to change the object
 *!   destructively and use it directly as the result.
 *!
 *! @note
 *!   This function is not an lfun for the @expr{+=@} operator. It's
 *!   only whether or not it's safe to do a destructive change that
 *!   decides if this function or @[lfun::`+()] is called; both the
 *!   @expr{+@} operator and the @expr{+=@} operator can call either
 *!   one.
 *!
 *! @seealso
 *!   @[lfun::`+()], @[predef::`+()]
 */

/*! @decl mixed lfun::``+(zero arg, zero ... rest)
 *!
 *!   Right side addition/concatenation callback.
 *!
 *!   This is used by @[predef::`+]. It's called with any arguments
 *!   that precede this object in the argument list of the call to
 *!   @[predef::`+]. The returned value should be a new instance that
 *!   represents the addition/concatenation between the arguments in
 *!   the order they are given and this object.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[lfun::`+()], @[predef::`+()]
 */

/*! @decl mixed lfun::`-(void|zero arg)
 *!
 *!   Negation and left side subtraction/set difference callback.
 *!
 *!   This is used by @[predef::`-]. When called without an argument
 *!   the result should be a new instance that represents the negation
 *!   of this object, otherwise the result should be a new instance
 *!   that represents the difference between this object and @[arg].
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[lfun::``-()], @[predef::`-()]
 */

/*! @decl mixed lfun::``-(zero arg)
 *!
 *!   Right side subtraction/set difference callback.
 *!
 *!   This is used by @[predef::`-]. The result should be a new
 *!   instance that represents the difference between @[arg] and this
 *!   object.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[lfun::`-()], @[predef::`-()]
 */

/*! @decl mixed lfun::`&(zero ... args)
 *!
 *!   Left side bitwise and/intersection callback.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[lfun::``&()], @[predef::`&()]
 */

/*! @decl mixed lfun::``&(zero ... args)
 *!
 *!   Right side bitwise and/intersection callback.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[lfun::`&()], @[predef::`&()]
 */

/*! @decl mixed lfun::`|(zero ... args)
 *!
 *!   Left side bitwise or/union callback.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[lfun::``|()], @[predef::`|()]
 */

/*! @decl mixed lfun::``|(zero ... args)
 *!
 *!   Right side bitwise or/union callback.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[lfun::`|()], @[predef::`|()]
 */

/*! @decl mixed lfun::`^(zero ... args)
 *!
 *!   Left side exclusive or callback.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[lfun::``^()], @[predef::`^()]
 */

/*! @decl mixed lfun::``^(zero ... args)
 *!
 *!   Right side exclusive or callback.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[lfun::`^()], @[predef::`^()]
 */

/*! @decl mixed lfun::`<<(zero arg)
 *!
 *!   Left side left shift callback.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[lfun::``<<()], @[predef::`<<()]
 */

/*! @decl mixed lfun::``<<(zero arg)
 *!
 *!   Right side left shift callback.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[lfun::`<<()], @[predef::`<<()]
 */

/*! @decl mixed lfun::`>>(zero arg)
 *!
 *!   Left side right shift callback.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[lfun::``>>()], @[predef::`>>()]
 */

/*! @decl mixed lfun::``>>(zero arg)
 *!
 *!   Right side right shift callback.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[lfun::`>>()], @[predef::`>>()]
 */

/*! @decl mixed lfun::`*(zero ... args)
 *!
 *!   Left side multiplication/repetition/implosion callback.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[lfun::``*()], @[predef::`*()]
 */

/*! @decl mixed lfun::``*(zero ... args)
 *!
 *!   Right side multiplication/repetition/implosion callback.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[lfun::`*()], @[predef::`*()]
 */

/*! @decl mixed lfun::`/(zero ... args)
 *!
 *!   Left side division/split callback.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[lfun::``/()], @[predef::`/()]
 */

/*! @decl mixed lfun::``/(zero ... args)
 *!
 *!   Right side division/split callback.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[lfun::`/()], @[predef::`/()]
 */

/*! @decl mixed lfun::`%(zero ... args)
 *!
 *!   Left side modulo callback.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[lfun::``%()], @[predef::`%()]
 */

/*! @decl mixed lfun::``%(zero ... args)
 *!
 *!   Right side modulo callback.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[lfun::`%()], @[predef::`%()]
 */

/*! @decl int lfun::`!()
 *!
 *!   Logical not callback.
 *!
 *! @returns
 *!   Returns non-zero if the object should be evaluated as false,
 *!   and @expr{0@} (zero) otherwise.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[predef::`!()]
 */

/*! @decl mixed lfun::`~()
 *!
 *!   Complement/inversion callback.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[predef::`~()]
 */

/*! @decl int(0..1) lfun::`==(mixed arg)
 *!
 *!   Equivalence test callback.
 *!
 *! @returns
 *!   Is expected to return @expr{1@} if the current object is
 *!   equivalent to @[arg] (ie may be replaced with @[arg], with
 *!   no semantic differences (disregarding the effects of @[destruct()])),
 *!   and @expr{0@} (zero) otherwise.
 *!
 *! @note
 *!   If this is implemented it may be necessary to implement
 *!   @[lfun::__hash] too. Otherwise mappings may hold several
 *!   objects as indices which are duplicates according to this
 *!   function. This may also affect various other functions
 *!   that use hashing internally, e.g. @[predef::Array.uniq].
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @note
 *!   It's recommended to only implement this function for
 *!   immutable objects, as otherwise stuff may get confusing
 *!   when things that once were equivalent no longer are so,
 *!   or the reverse.
 *!
 *! @seealso
 *!   @[predef::`==()], @[lfun::__hash]
 */

/*! @decl int(0..1) lfun::`<(mixed arg)
 *!
 *!   Less than test callback.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[predef::`<()]
 */

/*! @decl int(0..1) lfun::`>(mixed arg)
 *!
 *!   Greater than test callback.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[predef::`>()]
 */

/*! @decl int lfun::__hash()
 *!
 *!   Hashing callback.
 *!
 *!   The main caller of this function is @[predef::hash_value()]
 *!   or the low-level equivalent, which get called by various
 *!   mapping operations when the object is used as index in a mapping.
 *!
 *! @returns
 *!   It should return an integer that corresponds to the object
 *!   in such a way that all values which @[lfun::`==] considers
 *!   equal to the object get the same hash value.
 *!
 *! @note
 *!   The function @[predef::hash] does not return hash values that
 *!   are compatible with this one.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[lfun::`==], @[predef::hash_value()]
 */

/*! @decl mixed lfun::cast(string requested_type)
 *!
 *!   Value cast callback.
 *!
 *! @param requested_type
 *!   Type to cast to.
 *!
 *! @returns
 *!   Expected to return the object value-casted (converted) to
 *!   the type described by @[requested_type].
 *!
 *! @note
 *!   The argument is currently a string with the name
 *!   of the type, but might in the future be a value of the type type.
 *!
 *! @note
 *!   Currently casting between object types is a noop.
 *!
 *! @note
 *!   If the returned value is not deemed to be of the requested type
 *!   a runtime error may be thrown.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 */

/*! @decl mixed lfun::`[](zero arg1, zero|void arg2)
 *!
 *!   Indexing callback.
 *!
 *!   For compatibility, this is also called to do subranges unless
 *!   there is a @[`[..]] in the class. See @[predef::`[..]] for
 *!   details.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[predef::`[]()], @[predef::`[..]]
 */

/*! @decl void lfun::`[]=(zero index, zero value, @
 *!                       object|void context, int|void access)
 *!
 *!   Index assignment callback.
 *!
 *! @param index
 *!   Index to change the value of.
 *!
 *! @param value
 *!   The new value.
 *!
 *! @param context
 *!   Context in the current object to index.
 *!
 *!   If @expr{UNDEFINED@} or left out, @expr{this_program::this@}
 *!   is to be used (ie start at the current context and ignore
 *!   any overloaded symbols).
 *!
 *! @param access
 *!   Access permission override. One of the following:
 *!   @int
 *!     @value 0
 *!     @value UNDEFINED
 *!       See only public symbols.
 *!     @value 1
 *!       See protected symbols as well.
 *!   @endint
 *!
 *! This function is to set the value at index @[index] of the current
 *! object to @[value].
 *!
 *! @seealso
 *!   @[predef::`[]=()], @[lfun::`->=()]
 */

/*! @decl mixed lfun::`->(string index, object|void context, int|void access)
 *!
 *!   Arrow index callback.
 *!
 *! @param index
 *!   Symbol in @[context] to access.
 *!
 *! @param context
 *!   Context in the current object to start the search from.
 *!   If @expr{UNDEFINED@} or left out, @expr{this_program::this@}
 *!   is to be be used (ie start at the current context and ignore
 *!   any overloaded symbols).
 *!
 *! @param access
 *!   Access permission override. One of the following:
 *!   @int
 *!     @value 0
 *!     @value UNDEFINED
 *!       See only public symbols.
 *!     @value 1
 *!       See protected symbols as well.
 *!   @endint
 *!
 *! @returns
 *!   Returns the value at @[index] if it exists, and
 *!   @expr{UNDEFINED@} otherwise.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[predef::`->()], @[::`->()]
 */

/*! @decl void lfun::`->=(string index, zero value, @
 *!                        object|void context, int|void access)
 *!
 *!   Arrow index assignment callback.
 *!
 *! @param index
 *!   Symbol in @[context] to change the value of.
 *!
 *! @param value
 *!   The new value.
 *!
 *! @param context
 *!   Context in the current object to index.
 *!
 *!   If @expr{UNDEFINED@} or left out, @expr{this_program::this@}
 *!   is to be used (ie start at the current context and ignore
 *!   any overloaded symbols).
 *!
 *! @param access
 *!   Access permission override. One of the following:
 *!   @int
 *!     @value 0
 *!     @value UNDEFINED
 *!       See only public symbols.
 *!     @value 1
 *!       See protected symbols as well.
 *!   @endint
 *!
 *! This function is to set the value at symbol @[index] of the current
 *! object to @[value].
 *!
 *! @seealso
 *!   @[predef::`->=()], @[::`->=()], @[lfun::`[]=()]
 */

/*! @decl int lfun::_sizeof()
 *!
 *!   Size query callback.
 *!
 *!   Called by @[predef::sizeof()] to determine the number of elements
 *!   in an object. If this function is not present, the number
 *!   of public symbols in the object will be returned.
 *!
 *! @returns
 *!   Expected to return the number of valid indices in the object.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[predef::sizeof()]
 */

/*! @decl array lfun::_indices(object|void context, int|void access)
 *!
 *!   List indices callback.
 *!
 *! @returns
 *!   Expected to return an array with the valid indices in the object.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[predef::indices()], @[lfun::_values()], @[lfun::_types()],
 *!   @[::_indices()]
 */

/*! @decl array lfun::_values(object|void context, int|void access)
 *!
 *!   List values callback.
 *!
 *! @returns
 *!   Expected to return an array with the values corresponding to
 *!   the indices returned by @[lfun::_indices()].
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[predef::values()], @[lfun::_indices()], @[lfun::_types()],
 *!   @[::_values()]
 */

/*! @decl mixed lfun::`()(zero ... args)
 *!
 *!   Apply callback.
 *!
 *! @seealso
 *!   @[predef::`()]
 */

/*! @decl int(0..1) lfun::_is_type(string basic_type)
 *!
 *!   Type comparison callback.
 *!
 *!   Called by the cast operator to determine if an object
 *!   simulates a basic type.
 *!
 *! @param basic_type
 *!   One of:
 *!   @string
 *!     @value "array"
 *!     @value "float"
 *!     @value "function"
 *!     @value "int"
 *!     @value "mapping"
 *!     @value "multiset"
 *!     @value "object"
 *!     @value "program"
 *!     @value "string"
 *!     @value "type"
 *!     @value "void"
 *!     @value "zero"
 *!   @endstring
 *!
 *!   The following five shouldn't occurr, but are here for completeness:
 *!   @string
 *!     @value "lvalue"
 *!     @value "mapping_data"
 *!     @value "object storage"
 *!     @value "pike_frame"
 *!     @value "unknown"
 *!   @endstring
 *!
 *! @returns
 *!   Expected to return @expr{1@} if the object is to be regarded as a
 *!   simulation of the type specified by @[basic_type].
 *!
 *! @note
 *!   The argument is currently a string with the name
 *!   of the type, but might in the future be a value of the type type.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 */

/*! @decl string lfun::_sprintf(int conversion_type, @
 *!                             mapping(string:int)|void params)
 *!
 *!   Sprintf callback.
 *!
 *!   This method is called by @[predef::sprintf()] to print objects. If it is
 *!   not present, printing of the object will not be supported for any
 *!   conversion-type except for the @tt{%O@}-conversion-type, which
 *!   will output @expr{"object"@}.
 *!
 *! @param conversion_type
 *!   One of:
 *!   @int
 *!     @value 'b'
 *!       Signed binary integer.
 *!     @value 'd'
 *!       Signed decimal integer.
 *!     @value 'u'
 *!       Unsigned decimal integer.
 *!     @value 'o'
 *!       Signed octal integer.
 *!     @value 'x'
 *!       Lowercase signed hexadecimal integer.
 *!     @value 'X'
 *!       Uppercase signed hexadecimal integer.
 *!     @value 'c'
 *!       Character. If a fieldsize has been specified this will output
 *!       the low-order bytes of the integer in network byte order.
 *!     @value 'f'
 *!       Float.
 *!     @value 'g'
 *!       Heuristically chosen representation of float.
 *!     @value 'G'
 *!       Like @tt{%g@}, but uses uppercase @tt{E@} for exponent.
 *!     @value 'e'
 *!       Exponential notation float.
 *!     @value 'E'
 *!       Like @tt{%e@}, but uses uppercase @tt{E@} for exponent.
 *!     @value 's'
 *!       String.
 *!     @value 'O'
 *!       Any value (debug style).
 *!     @value 't'
 *!       Type of the argument.
 *!   @endint
 *!
 *! @param params
 *!   Conversion parameters. The following parameters may be supplied:
 *!   @mapping
 *!     @member int "precision"
 *!       Precision.
 *!     @member int "width"
 *!       Field width.
 *!     @member int(1..1) "flag_left"
 *!       Indicates that the output should be left-aligned.
 *!     @member int "indent"
 *!       Indentation level in @tt{%O@}-mode.
 *!   @endmapping
 *!
 *! @returns
 *!   Is expected to return a string describing the object formatted
 *!   according to @[conversion_type].
 *!
 *! @note
 *!   @[_sprintf()] is currently not called for the following
 *!   conversion-types:
 *!   @int
 *!     @value 'F'
 *!       Binary IEEE representation of float (@tt{%4F@} gives
 *!       single precision, @tt{%8F@} gives double precision.)
 *!   @endint
 *!
 *! @note
 *!   This function might be called at odd times, e.g. before
 *!   @[lfun::create] has been called or when an error has occurred.
 *!   The reason is typically that it gets called when a backtrace is
 *!   being formatted to report an error. It should therefore be very
 *!   robust and not make any assumptions about its own internal
 *!   state, at least not when @[conversion_type] is @expr{'O'@}.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!    @[predef::sprintf()]
 */

/*! @decl int lfun::_equal(mixed arg)
 *!
 *!   Recursive equality callback.
 *!
 *! @returns
 *!   Is expected to return @expr{1@} if the current object is
 *!   equal to @[arg], and @expr{0@} (zero) otherwise.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @note
 *!   Note that this function may return different values at different
 *!   times for the same argument due to the mutability of the object.
 *!
 *! @seealso
 *!   @[predef::equal()], @[lfun::`==()]
 */

/*! @decl mixed lfun::_m_delete(mixed arg)
 *!
 *!   Delete index callback.
 *!
 *! @seealso
 *!   @[predef::m_delete()]
 */

/*! @decl predef::Iterator lfun::_get_iterator()
 *!
 *!   Iterator creation callback.
 *!
 *!   The returned @[predef::Iterator] instance works as a cursor that
 *!   references a specific item contained (in some arbitrary sense)
 *!   in this one.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[predef::Iterator], @[predef::get_iterator], @[predef::foreach()]
 */

/*! @decl mixed lfun::`[..](zero low, int low_bound_type, @
 *!                         zero high, int high_bound_type)
 *!
 *!   Subrange callback.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[predef::`[..]]
 */

/**** END TRUE LFUNS ****/
/**** BEGIN FAKE LFUNS ****/

/*! @decl mixed lfun::_search(mixed needle, mixed|void start, @
 *!                           mixed ... extra_args)
 *!
 *!   Search callback.
 *!
 *!   The arguments are sent straight from @[search()], and are
 *!   as follows:
 *!
 *! @param needle
 *!   Value to search for.
 *!
 *! @param start
 *!   The first position to search.
 *!
 *! @param extra_args
 *!   Optional extra arguments as passed to @[search()].
 *!
 *! @seealso
 *!   @[predef::search()]
 */

/*! @decl array lfun::_types(object|void context, int|void access)
 *!
 *!   List types callback.
 *!
 *!   This callback is typically called via @[predef::types()].
 *!
 *! @returns
 *!   Expected to return an array with the types corresponding to
 *!   the indices returned by @[lfun::_indices()].
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @note
 *!   @[predef::types()] was added in Pike 7.9.
 *!
 *! @seealso
 *!   @[predef::types()], @[lfun::_indices()], @[lfun::_values()],
 *!   @[::_types()]
 */

/*! @decl void lfun::_serialize(object o, @
 *!                             function(mixed, string, type:void) serializer)
 *!
 *!   Dispatch function for @[Serializer.serialize()].
 *!
 *! @param o
 *!   Object to serialize. Always a context of the current object.
 *!
 *! @param serializer
 *!   Function to be called once for every variable
 *!   to serialize.
 *!
 *! The @[serializer] function expects to be called with three arguments:
 *!   @dl
 *!     @item
 *!       @tt{value@} - The value of the symbol.
 *!     @item
 *!       @tt{symbol@} - The symbol name.
 *!     @item
 *!       @tt{symbol_type@} - The type of the symbol.
 *!   @enddl
 *!
 *! @note
 *!   A default implementation of @[lfun::_serialize()] and
 *!   @[lfun::_deserialize()] is available in @[Serializer.Serializable].
 *!
 *! @seealso
 *!   @[lfun::_deserialize()], @[Serializer.serialize()],
 *!   @[Serializer.Serializable()->_serialize()]
 */

/*! @decl void lfun::_deserialize(object o, @
 *!                    function(function(mixed:void), @
 *!                             string, type: mixed) deserializer)
 *!
 *!   Dispatch function for @[Serialization.deserialize()].
 *!
 *! @param o
 *!   Object to serialize. Always a context of the current object.
 *!
 *! @param deserializer
 *!   Function to be called once for every variable
 *!   to serialize.
 *!
 *! The @[deserializer] function expects to be called with three arguments:
 *!   @dl
 *!     @item
 *!       @tt{setter@} - Function that sets the symbol value.
 *!     @item
 *!       @tt{symbol@} - The symbol name.
 *!     @item
 *!       @tt{symbol_type@} - The type of the symbol.
 *!   @enddl
 *!
 *! @note
 *!   A default implementation of @[lfun::_serialize()] and
 *!   @[lfun::_deserialize()] is available in @[Serializer.Serializable].
 *!
 *! @seealso
 *!   @[lfun::_serialize()], @[Serializer.deserialize()],
 *!   @[Serializer.Serializable()->_deserialize()]
 */

/*! @decl int lfun::_size_object()
 *!
 *! @[Debug.size_object()] callback.
 *!
 *! @returns
 *!   Returns an approximation of the memory use in bytes for the object.
 *!
 *! @seealso
 *!   @[Debug.size_object()], @[lfun::_sizeof()]
 */

/*! @decl mixed lfun::_random(function(int(0..):string(8bit)) random_string, @
 *!                           function(mixed:mixed) random)
 *!   Called by @[random()]. Typical use is when the object implements
 *!   a ADT, when a call to this lfun should return a random member of
 *!   the ADT or range implied by the ADT.
 *!
 *! @param random_string
 *!   A @[RandomInterface()->random_string] function that returns
 *!   a string(8bit) of the specified length.
 *!
 *! @param random
 *!   A @[RandomInterface()->random] function.
 *!
 *! @seealso
 *!   @[predef::random()], @[RandomInterface]
 */

/**** END FAKE LFUNS ****/
/**** BEGIN MAGIC LFUNS ****/

/*! @decl mixed lfun::`symbol()
 *! @decl mixed lfun::`->symbol()
 *!
 *!   Variable retrieval callback (aka "getter").
 *!
 *! @note
 *!   Note that the @expr{symbol@} in the name can be any symbol.
 *!
 *! @note
 *!   This is not a true LFUN, since it is even more low level!
 *!
 *! @note
 *!   This function WILL be called even by inheriting programs
 *!   when they attempt to access the variable named @expr{symbol@}.
 *!
 *! @seealso
 *!   @[lfun::`->symbol=()], @[lfun::`->()]
 */

/*! @decl void lfun::`symbol=(zero value)
 *! @decl void lfun::`->symbol=(zero value)
 *!
 *!   Variable assignment callback (aka "setter").
 *!
 *! @note
 *!   Note that the @expr{symbol@} in the name can be any symbol.
 *!
 *! @note
 *!   This is not a true LFUN, since it is even more low level!
 *!
 *! @note
 *!   This function WILL be called even by inheriting programs
 *!   when they attempt to set the variable named @expr{symbol@}.
 *!
 *! @seealso
 *!   @[lfun::`->symbol()], @[lfun::`->=()]
 */

/*! @endnamespace
 */

/*! @class MasterObject
 */

/*! @decl void unregister(program p)
 *!
 *!   Unregister a program that was only partially compiled.
 *!
 *!   Called by @[compile()] to clean up references to partially compiled
 *!   programs.
 *!
 *! @param p
 *!   Partially compiled program that should no longer be referenced.
 *!
 *! @fixme
 *!   Shouldn't this function be in the compilation handler?
 */

/*! @endclass
 */

/*! @class CompilationHandler
 */

/*! @decl mapping(string:mixed)|object get_default_module()
 *!
 *!   Returns the default module from which global symbols will
 *!   be fetched.
 *!
 *! @returns
 *!   Returns the default module, or @expr{0@} (zero).
 *!
 *!   If @expr{0@} (zero) is returned the compiler use the mapping
 *!   returned by @[all_constants()] as fallback.
 *!
 *! @seealso
 *!   @[get_predefines()]
 */

/*! @decl void compile_warning(string filename, int line, string msg)
 *!
 *!   Called by @[compile()] to report warnings.
 *!
 *! @param filename
 *!   File which triggered the warning.
 *!
 *! @param line
 *!   Line which triggered the warning.
 *!
 *! @param msg
 *!   Warning message.
 *!
 *! @seealso
 *!   @[compile_error()]
 */

/*! @endclass
 */

struct program *first_program = 0;
static int current_program_id = PROG_DYNAMIC_ID_START;

struct program *gc_internal_program = 0;
static struct program *gc_mark_program_pos = 0;

#define CHECK_FILE_ENTRY(PROG, STRNO)					\
  do {									\
    if ((STRNO < 0) || (STRNO >= PROG->num_strings))			\
      Pike_fatal ("Invalid file entry in linenumber info.\n");		\
  } while (0)

INT_TYPE get_small_number(char **q);

PMOD_EXPORT void do_free_program (struct program *p)
{
  if (p)
    free_program(p);
}

/* So what if we don't have templates? / Hubbe */

#ifdef PIKE_DEBUG
#define CHECK_FOO(NUMTYPE,TYPE,NAME)				              \
  if(Pike_compiler->malloc_size_program-> PIKE_CONCAT(num_,NAME) <            \
       Pike_compiler->new_program-> PIKE_CONCAT(num_,NAME))	              \
    Pike_fatal("Pike_compiler->new_program->num_" #NAME " is out of order\n");\
  if(Pike_compiler->new_program->flags & PROGRAM_OPTIMIZED)		      \
    Pike_fatal("Tried to reallocate fixed program.\n")

#else
#define CHECK_FOO(NUMTYPE,TYPE,NAME)
#endif

#ifndef RELOCATE_program
#define RELOCATE_program(ORIG, NEW)
#endif /* !RELOCATE_program */
#define RELOCATE_identifier_cache(ORIG,NEW)
#define RELOCATE_linenumbers(ORIG,NEW)
#define RELOCATE_identifier_index(ORIG,NEW)
#define RELOCATE_variable_index(ORIG,NEW)
#define RELOCATE_identifier_references(ORIG,NEW)
#define RELOCATE_strings(ORIG,NEW)
#define RELOCATE_inherits(ORIG,NEW)
#define RELOCATE_identifiers(ORIG,NEW)
#define RELOCATE_constants(ORIG,NEW)
#define RELOCATE_relocations(ORIG,NEW)

#if SIZEOF_LONG_LONG == 8
/* we have 8 byte ints, hopefully this constant works on all these systems */
#define MAXVARS(NUMTYPE)						\
   (NUMTYPE)(sizeof(NUMTYPE)==1?254:					\
    (sizeof(NUMTYPE)==2?65534:						\
     (sizeof(NUMTYPE)==4?4294967294U:18446744073709551614ULL)))
#else
#define MAXVARS(NUMTYPE) \
   (NUMTYPE)(sizeof(NUMTYPE)==1?254: (sizeof(NUMTYPE)==2?65534:4294967294U))
#endif

#ifdef PIKE_USE_MACHINE_CODE
/* Special cases for low_add_to_program and add_to_program since
 * many OSes require us to use mmap to allocate memory for our
 * machine code. For decoding efficiency we also want a multi copy
 * variant to be used by decode().
 */
#define BAR(NUMTYPE,TYPE,ARGTYPE,NAME)					\
void PIKE_CONCAT(low_add_to_,NAME) (struct program_state *state,	\
                                    TYPE ARG) {				\
  NUMTYPE m = state->malloc_size_program->PIKE_CONCAT(num_,NAME);	\
  CHECK_FOO(NUMTYPE,TYPE,NAME);						\
  if(m == state->new_program->PIKE_CONCAT(num_,NAME)) {			\
    TYPE *tmp;								\
    if(m==MAXVARS(NUMTYPE)) {						\
      yyerror("Too many " #NAME ".");					\
      return;								\
    }									\
    m = MINIMUM(m*2+1,MAXVARS(NUMTYPE));				\
    tmp = mexec_realloc(state->new_program->NAME, sizeof(TYPE) * m);    \
    if(!tmp) Pike_fatal("Out of memory.\n");				\
    PIKE_CONCAT(RELOCATE_,NAME)(state->new_program, tmp);		\
    state->malloc_size_program->PIKE_CONCAT(num_,NAME)=m;		\
    state->new_program->NAME=tmp;					\
  }									\
  state->new_program->							\
    NAME[state->new_program->PIKE_CONCAT(num_,NAME)++]=(ARG);		\
}									\
void PIKE_CONCAT(low_add_many_to_,NAME) (struct program_state *state,	\
					 TYPE *ARG, NUMTYPE cnt) {	\
  NUMTYPE m = state->malloc_size_program->PIKE_CONCAT(num_,NAME);	\
  CHECK_FOO(NUMTYPE,TYPE,NAME);						\
  if((m + cnt) > state->new_program->PIKE_CONCAT(num_,NAME)) {		\
    TYPE *tmp;								\
    NUMTYPE n = m;							\
    do {								\
      if(n==MAXVARS(NUMTYPE)) {						\
	yyerror("Too many " #NAME ".");					\
	return;								\
      }									\
      n = MINIMUM(n*2+1,MAXVARS(NUMTYPE));				\
    } while (m + cnt > n);						\
    m = n;								\
    tmp = mexec_realloc(state->new_program->NAME, sizeof(TYPE) * m);    \
    if(!tmp) Pike_fatal("Out of memory.\n");				\
    PIKE_CONCAT(RELOCATE_,NAME)(state->new_program, tmp);		\
    state->malloc_size_program->PIKE_CONCAT(num_,NAME)=m;		\
    state->new_program->NAME=tmp;					\
  }									\
  memcpy(state->new_program->NAME +					\
	 state->new_program->PIKE_CONCAT(num_,NAME),			\
	 ARG, sizeof(TYPE) * cnt);					\
  state->new_program->PIKE_CONCAT(num_,NAME) += cnt;			\
}									\
void PIKE_CONCAT(add_to_,NAME) (ARGTYPE ARG) {				\
  PIKE_CONCAT(low_add_to_,NAME) ( Pike_compiler, ARG );			\
}
#else /* !PIKE_USE_MACHINE_CODE */
#define BAR(NUMTYPE,TYPE,ARGTYPE,NAME)					\
  FOO(NUMTYPE,TYPE,ARGTYPE,NAME)					\
  void PIKE_CONCAT(low_add_many_to_,NAME) (struct program_state *state,	\
					   TYPE *ARG, NUMTYPE cnt) {	\
  NUMTYPE m = state->malloc_size_program->PIKE_CONCAT(num_,NAME);	\
  CHECK_FOO(NUMTYPE,TYPE,NAME);						\
  if((m + cnt) > state->new_program->PIKE_CONCAT(num_,NAME)) {		\
    TYPE *tmp;								\
    NUMTYPE n = m;							\
    do {								\
      if(n==MAXVARS(NUMTYPE)) {						\
	yyerror("Too many " #NAME ".");					\
	return;								\
      }									\
      n = MINIMUM(n*2+1,MAXVARS(NUMTYPE));				\
    } while (m + cnt > n);						\
    m = n;								\
    tmp = realloc(state->new_program->NAME, sizeof(TYPE) * m);		\
    if(!tmp) Pike_fatal("Out of memory.\n");				\
    PIKE_CONCAT(RELOCATE_,NAME)(state->new_program, tmp);		\
    state->malloc_size_program->PIKE_CONCAT(num_,NAME)=m;		\
    state->new_program->NAME=tmp;					\
  }									\
  memcpy(state->new_program->NAME +					\
	 state->new_program->PIKE_CONCAT(num_,NAME),			\
	 ARG, sizeof(TYPE) * cnt);					\
  state->new_program->PIKE_CONCAT(num_,NAME) += cnt;			\
}
#endif /* PIKE_USE_MACHINE_CODE */

/* Funny guys use the uppermost value for nonexistant variables and
   the like. Hence -2 and not -1. Y2K. */
#define FOO(NUMTYPE,TYPE,ARGTYPE,NAME)					\
void PIKE_CONCAT(low_add_to_,NAME) (struct program_state *state,	\
                                    TYPE ARG) {				\
  NUMTYPE m = state->malloc_size_program->PIKE_CONCAT(num_,NAME);	\
  CHECK_FOO(NUMTYPE,TYPE,NAME);						\
  if(m == state->new_program->PIKE_CONCAT(num_,NAME)) {			\
    TYPE *tmp;								\
    if(m==MAXVARS(NUMTYPE)) {						\
      yyerror("Too many " #NAME ".");					\
      return;								\
    }									\
    m = MINIMUM(m*2+1,MAXVARS(NUMTYPE));				\
    tmp = realloc(state->new_program->NAME, sizeof(TYPE) * m);		\
    if(!tmp) Pike_fatal("Out of memory.\n");				\
    PIKE_CONCAT(RELOCATE_,NAME)(state->new_program, tmp);		\
    state->malloc_size_program->PIKE_CONCAT(num_,NAME)=m;		\
    state->new_program->NAME=tmp;					\
  }									\
  state->new_program->							\
    NAME[state->new_program->PIKE_CONCAT(num_,NAME)++]=(ARG);		\
}									\
void PIKE_CONCAT(add_to_,NAME) (ARGTYPE ARG) {				\
  PIKE_CONCAT(low_add_to_,NAME) ( Pike_compiler, ARG );			\
}

/* Funny guys use the uppermost value for nonexistant variables and
   the like. Hence -2 and not -1. Y2K. */
#define PASS1ONLY(NUMTYPE,TYPE,ARGTYPE,NAME)				\
void PIKE_CONCAT(low_add_to_,NAME) (struct program_state *state,	\
                                    TYPE ARG) {				\
  NUMTYPE m = state->malloc_size_program->PIKE_CONCAT(num_,NAME);	\
  CHECK_FOO(NUMTYPE,TYPE,NAME);						\
  DO_IF_DEBUG(if (state->compiler_pass != COMPILER_PASS_FIRST) {	\
		Pike_fatal("Adding " TOSTR(NAME) " in pass %d.\n",	\
			   state->compiler_pass);			\
	      });							\
  if(m == state->new_program->PIKE_CONCAT(num_,NAME)) {			\
    TYPE *tmp;								\
    if(m==MAXVARS(NUMTYPE)) {						\
      yyerror("Too many " #NAME ".");					\
      return;								\
    }									\
    m = MINIMUM(m*2+1,MAXVARS(NUMTYPE));				\
    tmp = realloc(state->new_program->NAME, sizeof(TYPE) * m);          \
    if(!tmp) Pike_fatal("Out of memory.\n");				\
    PIKE_CONCAT(RELOCATE_,NAME)(state->new_program, tmp);		\
    state->malloc_size_program->PIKE_CONCAT(num_,NAME)=m;		\
    state->new_program->NAME=tmp;					\
  }									\
  state->new_program->							\
    NAME[state->new_program->PIKE_CONCAT(num_,NAME)++]=(ARG);		\
}									\
void PIKE_CONCAT(add_to_,NAME) (ARGTYPE ARG) {				\
  PIKE_CONCAT(low_add_to_,NAME) ( Pike_compiler, ARG );			\
}


#include "program_areas.h"


#define add_to_program(ARG)	do {				\
    debug_malloc_touch(Pike_compiler->new_program->program);	\
    add_to_program(ARG);					\
  } while(0)

void ins_int(INT32 i, void (*func)(char tmp))
{
  int e;
  unsigned char *p = (unsigned char *)&i;
  for(e=0;e<(long)sizeof(i);e++) {
    func(p[e]);
  }
}


#if 0
/* This check is not possible to do since the identifier is added
 * before checking for duplicates in add_constant. */
static void debug_add_to_identifiers (struct identifier id)
{
  if (d_flag) {
    int i;
    for (i = 0; i < Pike_compiler->new_program->num_identifiers; i++)
      if (Pike_compiler->new_program->identifiers[i].name == id.name) {
	dump_program_tables (Pike_compiler->new_program, 0);
	Pike_fatal ("Adding identifier twice, old at %s:%d #%d.\n",
		    Pike_compiler->new_program->identifiers[i].filename?
		    Pike_compiler->new_program->identifiers[i].filename:"-",
		    Pike_compiler->new_program->identifiers[i].linenumber,
		    i);
      }
  }
  add_to_identifiers (id);
}
#else
#define debug_add_to_identifiers(ARG) add_to_identifiers(ARG)
#endif

static int low_add_identifier(struct compilation *c,
			      struct pike_type *type,
			      struct pike_string *name,
			      unsigned int identifier_flags,
			      unsigned int opt_flags,
			      union idptr func,
			      int run_time_type)
{
  struct identifier dummy;
  int n = Pike_compiler->new_program->num_identifiers;

  copy_shared_string(dummy.name, name);
  copy_pike_type(dummy.type, type);
  dummy.filename_strno = store_prog_string(c->lex.current_file);
  dummy.linenumber = c->lex.current_line;
  dummy.identifier_flags = identifier_flags;
  dummy.run_time_type = run_time_type;
  dummy.func = func;
  dummy.opt_flags = opt_flags;
#ifdef PROFILING
  dummy.self_time=0;
  dummy.num_calls=0;
  dummy.recur_depth=0;
  dummy.total_time=0;
#endif
  debug_add_to_identifiers(dummy);

  return n;
}

static int add_identifier(struct compilation *c,
			  struct pike_type *type,
			  struct pike_string *name,
			  unsigned int modifier_flags,
			  unsigned int identifier_flags,
			  unsigned int opt_flags,
			  union idptr func,
			  int run_time_type)
{
  struct reference ref;
  struct identifier dummy;
  int n;

  if (modifier_flags & ID_PRIVATE) modifier_flags |= ID_LOCAL|ID_PROTECTED;

  if (((identifier_flags & (IDENTIFIER_VARIABLE|IDENTIFIER_ALIAS)) ==
      IDENTIFIER_VARIABLE) &&
      (modifier_flags & ID_WEAK)) {
    identifier_flags |= IDENTIFIER_WEAK;
  }

  ref.id_flags = modifier_flags;
  ref.identifier_offset =
    low_add_identifier(c, type, name,
		       identifier_flags, opt_flags,
		       func, run_time_type);
  ref.inherit_offset = 0;
  ref.run_time_type = PIKE_T_UNKNOWN;

  if ((identifier_flags & (IDENTIFIER_VARIABLE|IDENTIFIER_ALIAS)) ==
      IDENTIFIER_VARIABLE) {
    add_to_variable_index(ref.identifier_offset);
  }

  n = Pike_compiler->new_program->num_identifier_references;
  add_to_identifier_references(ref);

  return n;
}

void add_relocated_int_to_program(INT32 i)
{
  add_to_relocations(Pike_compiler->new_program->num_program);
  ins_int(i, (void (*)(char))add_to_program);
}

void use_module(struct svalue *s)
{
  struct compilation *c = THIS_COMPILATION;
  if( (1<<TYPEOF(*s)) & (BIT_MAPPING | BIT_OBJECT | BIT_PROGRAM))
  {
    c->num_used_modules++;
    Pike_compiler->num_used_modules++;
    assign_svalue_no_free((struct svalue *)
			  buffer_alloc(&c->used_modules, sizeof(struct svalue)), s);
    if(Pike_compiler->module_index_cache)
    {
      free_mapping(Pike_compiler->module_index_cache);
      Pike_compiler->module_index_cache=0;
    }
    if(c->resolve_cache)
    {
      free_mapping(c->resolve_cache);
      c->resolve_cache=0;
    }
  }else{
    yyerror("Module is neither mapping nor object");
  }
}

void unuse_modules(INT32 howmany)
{
  struct compilation *c = THIS_COMPILATION;
  struct svalue *s = buffer_dst(&c->used_modules);
  if(!howmany) return;
  s -= howmany;
#ifdef PIKE_DEBUG
  if(howmany *sizeof(struct svalue) > buffer_content_length(&c->used_modules))
    Pike_fatal("Unusing too many modules.\n");
#endif
  c->num_used_modules -= howmany;
  Pike_compiler->num_used_modules-=howmany;
  free_svalues(s, howmany, BIT_MAPPING | BIT_OBJECT | BIT_PROGRAM);
  buffer_remove(&c->used_modules, howmany*sizeof(struct svalue));
#ifdef PIKE_DEBUG
  if (s != buffer_dst(&c->used_modules))
    Pike_fatal("buffer_remove is broken.\n");
#endif
  if(Pike_compiler->module_index_cache)
  {
    free_mapping(Pike_compiler->module_index_cache);
    Pike_compiler->module_index_cache=0;
  }
}

static struct node_s *index_modules(struct pike_string *ident,
				    struct mapping **module_index_cache,
				    const int num_used_modules,
				    struct svalue *modules)
  /* num_used_modules is declared const here to convince the compiler that it is not
   * modified in between setjmp() and longjmp(). This prevents -Wclobbered warnings.
   */
{
  if(*module_index_cache)
  {
    struct svalue *tmp=low_mapping_string_lookup(*module_index_cache,ident);
    if(tmp)
    {
      if(!(SAFE_IS_ZERO(tmp) && SUBTYPEOF(*tmp)==1))
	return mksvaluenode(tmp);
      return 0;
    }
  }

/*  fprintf(stderr,"index_module: %s\n",ident->str); */

  {
    JMP_BUF tmp;

    if(SETJMP(tmp))
      handle_compile_exception ("Couldn't index a module with %S.", ident);
    else {
      int e = num_used_modules;
      struct svalue *m = modules - num_used_modules;

      while(--e>=0)
      {
	push_svalue(m+e);
	ref_push_string(ident);
	f_index(2);

	if(!IS_UNDEFINED(Pike_sp-1))
	{
	  struct node_s *ret;
	  UNSETJMP(tmp);

	  if (Pike_compiler->compiler_pass == COMPILER_PASS_LAST &&
	      ((TYPEOF(Pike_sp[-1]) == T_OBJECT &&
		Pike_sp[-1].u.object == placeholder_object) ||
	       (TYPEOF(Pike_sp[-1]) == T_PROGRAM &&
		Pike_sp[-1].u.program == placeholder_program))) {
	    my_yyerror("Got placeholder %s (resolver problem) "
		       "when indexing a module with %S.",
		       get_name_of_type (TYPEOF(Pike_sp[-1])), ident);
	    ret = 0;
	  }
	  else {
	    if(!*module_index_cache)
	      *module_index_cache = allocate_mapping(10);
	    mapping_string_insert(*module_index_cache, ident, Pike_sp-1);
	    ret = mksvaluenode(Pike_sp-1);
	  }
	  pop_stack();
	  return ret;
	}
	pop_stack();
      }
    }
    UNSETJMP(tmp);
  }


  return 0;
}

struct node_s *resolve_identifier(struct pike_string *ident);

struct node_s *find_module_identifier(struct pike_string *ident,
				      int see_inherit)
{
  struct compilation *c = THIS_COMPILATION;
  struct node_s *ret;

  struct svalue *modules=(struct svalue *)buffer_dst(&c->used_modules);

  {
    struct program_state *p=Pike_compiler;
    int n;
    for(n=0;n<=c->compilation_depth;n++,p=p->previous)
    {
      int i;
      if(see_inherit)
      {
	i=really_low_find_shared_string_identifier(ident,
						   p->new_program,
						   SEE_PROTECTED|SEE_PRIVATE);
	if(i!=-1)
	{
	  if ((p->flags & COMPILATION_FORCE_RESOLVE) &&
	      (p->compiler_pass == COMPILER_PASS_LAST) &&
	      ((p->num_inherits + 1) < p->new_program->num_inherits) &&
	      (PTR_FROM_INT(p->new_program, i)->inherit_offset >
	       p->num_inherits)) {
	    /* Don't look up symbols inherited later, since we need to get
	     * the same symbol in both passes in the force_resolve mode.
	     */
	    continue;
	  }
	  return p == Pike_compiler ?
	    mkidentifiernode(i) :
	    mkexternalnode(p->new_program, i);
	}
      }

      if((ret=index_modules(ident,
			    &p->module_index_cache,
			    p->num_used_modules,
			    modules))) return ret;
      modules-=p->num_used_modules;
#ifdef PIKE_DEBUG
      if( ((char *)modules ) < (char*)buffer_ptr(&c->used_modules))
	Pike_fatal("Modules out of whack!\n");
#endif
    }
  }

  return resolve_identifier(ident);
}

/* Look up a predefined identifier. */
struct node_s *find_predef_identifier(struct pike_string *ident)
{
  struct compilation *c = THIS_COMPILATION;
  node *tmp = mkconstantsvaluenode(&c->default_module);
  node *ret = index_node(tmp, "predef", ident);
  if(ret && !ret->name)
    add_ref(ret->name = ident);
  free_node(tmp);
  return ret;
}

/*! @decl constant UNDEFINED
 *!
 *! The undefined value; ie a zero for which @[zero_type()] returns 1.
 */

int low_resolve_identifier(struct pike_string *ident)
{
  struct compilation *c = THIS_COMPILATION;
  node *ret = NULL;

  /* Handle UNDEFINED */
  if (ident == UNDEFINED_string) {
    push_undefined();
    return 1;
  }

  if(c->resolve_cache)
  {
    struct svalue *tmp=low_mapping_string_lookup(c->resolve_cache,ident);
    if(tmp)
    {
      if(IS_UNDEFINED (tmp)) {
	return 0;
      }

      push_svalue(tmp);
      return 1;
    }
  }

  CHECK_COMPILER();

  ref_push_string(ident);
  if (!safe_apply_current2(PC_RESOLV_FUN_NUM, 1, NULL))
    handle_compile_exception ("Error resolving '%S'.", ident);

  if (Pike_compiler->compiler_pass != COMPILER_PASS_LAST) {
    /* If we get a program that hasn't gone through pass 1 yet then we
     * have to register a dependency now in our pass 1 so that our
     * pass 2 gets delayed. Otherwise the other program might still be
     * just as unfinished when we come back here in pass 2. */
    struct program *p = NULL;
    if (TYPEOF(Pike_sp[-1]) == T_PROGRAM)
      p = Pike_sp[-1].u.program;
    else if (TYPEOF(Pike_sp[-1]) == T_OBJECT ||
	     (TYPEOF(Pike_sp[-1]) == T_FUNCTION &&
	      SUBTYPEOF(Pike_sp[-1]) != FUNCTION_BUILTIN))
      p = Pike_sp[-1].u.object->prog;
    if (p && !(p->flags & PROGRAM_PASS_1_DONE))
      report_compiler_dependency (p);
  }

  if (Pike_compiler->compiler_pass == COMPILER_PASS_LAST &&
      ((TYPEOF(Pike_sp[-1]) == T_OBJECT &&
	Pike_sp[-1].u.object == placeholder_object) ||
       (TYPEOF(Pike_sp[-1]) == T_PROGRAM &&
	Pike_sp[-1].u.program == placeholder_program))) {
    my_yyerror("Got placeholder %s (resolver problem) "
	       "when resolving '%S'.",
	       get_name_of_type (TYPEOF(Pike_sp[-1])), ident);
  } else {
    if(!c->resolve_cache)
      c->resolve_cache=dmalloc_touch(struct mapping *, allocate_mapping(10));
    mapping_string_insert(c->resolve_cache,ident,Pike_sp-1);

    if(!IS_UNDEFINED (Pike_sp-1))
    {
      return 1;
    }
  }
  pop_stack();

  return 0;
}

struct node_s *resolve_identifier(struct pike_string *ident)
{
  node *ret = NULL;

  if (low_resolve_identifier(ident)) {
    ret = mkconstantsvaluenode(Pike_sp-1);
    pop_stack();
  }
  return ret;
}

/**
 * This function is intended to simplify resolving of
 * program symbols during compile-time for C-modules.
 *
 * A typical use-case is for a C-module inheriting
 * code written in Pike.
 */
PMOD_EXPORT struct program *resolve_program(struct pike_string *ident)
{
  struct program *ret = NULL;
  if (low_resolve_identifier(ident)) {
    if ((ret = program_from_svalue(Pike_sp-1))) {
      add_ref(ret);
    }
    pop_stack();
  }
  if (!ret) {
    my_yyerror("Invalid program identifier '%S'.", ident);
  }
  return ret;
}

/**
 * Find the identifier named ident in inherit #inh in the lexical scope
 * given by inherit_state and inherit_depth.
 *
 * @param inherit_state
 *   The program state for the program where the inherit is found.
 *
 * @param inherit_depth
 *   The number of lexical levels from the currently compiling program
 *   (ie Pike_compiler) to the one in inherit_state.
 *
 * @param inh
 *   >0 Specified inherit.
 *   =0 this_program:: or this::.
 *   -1 local::
 *   -2 global::
 *   -3 ::
 */
struct node_s *find_inherited_identifier(struct program_state *inherit_state,
                                         int inherit_depth, int inh,
					 struct pike_string *ident)
{
  int id;
  struct program *p = inherit_state->new_program;

#if 0
  fprintf(stderr, "find_inherited_identifier(%p, %d, %d, \"%s\")\n",
	  inherit_state, inherit_depth, inh, ident->str);
#endif /* 0 */
  if (inh == INHERIT_ALL) {
    /* Unspecified inherit, but not inherit #0. */
    struct node_s *res = NULL;
    for (inh = 1; inh < p->num_inherits; inh++) {
      struct node_s *n;
      if (p->inherits[inh].inherit_level != 1) continue;
      /* NB: We can't recurse here, as that would resolve the magic
       *     identifiers multiple times on multiple inherit.
       */
      id = low_reference_inherited_identifier(inherit_state, inh, ident,
					      SEE_PROTECTED);
      if (id == -1) continue;
      if (inherit_depth) {
	n = mkexternalnode(inherit_state->new_program, id);
      } else {
	n = mkidentifiernode(id);
      }
      if (res) {
	res = mknode(F_ARG_LIST, res, n);
      } else {
	res = n;
      }
    }
    if (res) {
      if (res->token == F_ARG_LIST) res = mkefuncallnode("aggregate", res);
      return res;
    }
    inh = -1;
  } else {
    if (inh > 0) {
      /* Specified inherit. */
      id = low_reference_inherited_identifier(inherit_state, inh, ident,
					      SEE_PROTECTED);
    } else {
      /* this_program:: (0), local:: (-1) or global:: (-2). */
      id = really_low_find_shared_string_identifier(ident,
						    inherit_state->new_program,
						    SEE_PROTECTED|SEE_PRIVATE);
    }

    if (id != -1) {
      if (inh == INHERIT_LOCAL) {
	/* local:: */
	struct reference *ref = p->identifier_references + id;
	if (IDENTIFIER_IS_VARIABLE(ID_FROM_PTR(p, ref)->identifier_flags)) {
	  /* Allowing local:: on variables would lead to pathological
	   * behavior: If a non-local variable in a class is referenced
	   * both with and without local::, both references would
	   * address the same variable in all cases except where an
	   * inheriting program overrides it (c.f. [bug 1252]).
	   *
	   * Furthermore, that's not how it works currently; if this
	   * error is removed then local:: will do nothing on variables
	   * except forcing a lookup in the closest surrounding class
	   * scope. */
	  yyerror ("Cannot make local references to variables.");
	  return NULL;
	}
	if (!(ref->id_flags & ID_LOCAL)) {
	  /* We need to generate a new reference. */
	  int d;
	  struct reference funp = *ref;
	  funp.id_flags = (funp.id_flags & ~ID_INHERITED) | ID_INLINE|ID_HIDDEN;
	  id = -1;
	  for(d = 0; d < (int)p->num_identifier_references; d++) {
	    struct reference *refp;
	    refp = p->identifier_references + d;

	    if (!(refp->id_flags & ID_LOCAL)) continue;

	    if((refp->inherit_offset == funp.inherit_offset) &&
	       (refp->identifier_offset == funp.identifier_offset)) {
	      id = d;
	      break;
	    }
	  }
	  if (id < 0) {
	    low_add_to_identifier_references(inherit_state, funp);
	    id = p->num_identifier_references - 1;
	  }
	}
      }
      if (inherit_depth > 0) {
	return mkexternalnode(inherit_state->new_program, id);
      }
      return mkidentifiernode(id);
    }
    if (inh < 0) inh = -1;
  }

  return program_magic_identifier(inherit_state, inherit_depth, inh, ident, 1);
}

/*! @decl constant this
 *!
 *! Builtin read only variable that evaluates to the current object.
 *!
 *! @seealso
 *!   @[this_program], @[this_object()]
 */

/*! @decl constant this_program
 *!
 *! Builtin constant that evaluates to the current program.
 *!
 *! @seealso
 *!   @[this], @[this_object()]
 */

/*! @decl constant this_function
 *!
 *! Builtin constant that evaluates to the current function.
 *!
 *! @seealso
 *!   @[this], @[this_object()]
 */

/**
 * If the identifier is recognized as one of the magic identifiers,
 * like "this", "this_program" or "`->" when preceded by ::, then a
 * suitable node is returned, NULL otherwise.
 *
 * @param state
 *   Program state containing the inherit (if any), otherwise the
 *   current program state.
 *
 * @param state_depth
 *   Number of lexical scopes to the inherit state.
 *
 * @param inherit_num
 *   Inherit number in state->new_program that the identifier has been
 *   qualified with. -1 when no specific inherit has been specified; ie
 *   either when the identifier has no prefix (colon_colon_ref == 0) or
 *   when the identifier has the prefix :: without any preceding identifier
 *   (colon_colon_ref == 1).
 *
 * @param ident
 *   Identifier to look up.
 *
 * @param colon_colon_ref
 *   Boolean indicating whether state, state_depth and inherit_num
 *   should be regarded (1) or not (0).
 *
 * New in Pike 7.9.5 and later:
 *
 *   If colon_colon_ref is 1 and the selected inherit defines the
 *   `->() lfun, code calling the lfun will be generated as follows:
 *
 *     inh::`->(ident, inh::this, 1)
 *
 * New in Pike 8.1.4 and later:
 *
 *   The symbol this_function may be prefixed with a selected inherit.
 *   It will then refer to the symbol with the same name as the
 *   current function from the selected inherit.
 *
 */
struct node_s *program_magic_identifier (struct program_state *state,
					 int state_depth, int inherit_num,
                                         struct pike_string *ident,
					 int colon_colon_ref)
{
#if 0
  fprintf (stderr, "magic_identifier (state, %d, %d, \"%s\", %d)\n",
	   state_depth, inherit_num, ident->str, colon_colon_ref);
#endif

  if (ident == this_string) {
    /* Handle this. */
    return mkthisnode(state->new_program, inherit_num);
  }

  /* Handle this_program */
  if (ident == this_program_string) {
    node *n;
    if (!state_depth && (inherit_num == -1) && colon_colon_ref &&
	!TEST_COMPAT(7,8) &&
	state->previous && state->previous->new_program) {
      /* ::this_program
       *
       * This refers to the previous definition of the current class
       * in its parent, and is typically used with inherit like:
       *
       *   inherit Foo;
       *
       *   // Override the Bar inherited from Foo.
       *   class Bar {
       *     // Bar is based on the implementation from Foo.
       *     inherit ::this_program;
       *
       *     // ...
       *   }
       */
      struct program *parent;
      struct pike_string *name = NULL;
      int e;
      int i;

      /* Find the name of the current class. */
      parent = state->previous->new_program;
      for (e = parent->num_identifier_references; e--;) {
	struct identifier *id = ID_FROM_INT(parent, e);
	struct svalue *s;
	if (!IDENTIFIER_IS_CONSTANT(id->identifier_flags) ||
	    (id->func.const_info.offset < 0)) {
	  continue;
	}
	s = &PROG_FROM_INT(parent, e)->
	  constants[id->func.const_info.offset].sval;
	if ((TYPEOF(*s) != T_PROGRAM) ||
	    (s->u.program != state->new_program)) {
	  continue;
	}
	/* Found! */
	name = id->name;
	break;
      }
      if (!name) {
	yyerror("Failed to find current class in its parent.");
	return NULL;
      }

      /* Find ::name in the parent. */
      n = find_inherited_identifier(state->previous, state_depth+1,
				    INHERIT_ALL, name);
      if (!n) {
	my_yyerror("Failed to find previous inherited definition of %S "
		   "in parent.", name);
	return NULL;
      }
    } else {
      n = mkefuncallnode("object_program",
			 mkthisnode(state->new_program, inherit_num));
    }
    /* We know this expression is constant. */
    n->node_info &= ~OPT_NOT_CONST;
    n->tree_info &= ~OPT_NOT_CONST;
    return n;
  }

  /* Handle this_function */
  if (ident == this_function_string) {
    int i;
    if ((i = Pike_compiler->compiler_frame->current_function_number) >= 0) {
      struct identifier *id;
      id = ID_FROM_INT(Pike_compiler->new_program, i);
      if (colon_colon_ref) {
	if (inherit_num == -1) inherit_num = INHERIT_ALL;
	return find_inherited_identifier(state, state_depth, inherit_num,
					 id->name);
      }
      if (id->identifier_flags & IDENTIFIER_SCOPED) {
	return mktrampolinenode(i, Pike_compiler->compiler_frame->previous);
      } else {
	return mkidentifiernode(i);
      }
    } else {
      /* FIXME: Fall back to __INIT? */
    }
  }

  if (colon_colon_ref) {
    int i = inherit_num;

    /* These are only recognized when prefixed with the :: operator. */

    if (inherit_num < 0) i = 0;
    if(ident == lfun_strings[LFUN_ARROW] ||
       ident == lfun_strings[LFUN_INDEX]) {
      return mknode(F_MAGIC_INDEX, mknewintnode(i),
		    mknewintnode(state_depth));
    } else if(ident == lfun_strings[LFUN_ASSIGN_ARROW] ||
	      ident == lfun_strings[LFUN_ASSIGN_INDEX]) {
      return mknode(F_MAGIC_SET_INDEX, mknewintnode(i),
		    mknewintnode(state_depth));
    } else if(ident == lfun_strings[LFUN__INDICES]) {
      return mknode(F_MAGIC_INDICES, mknewintnode(i),
		    mknewintnode(state_depth));
    } else if(ident == lfun_strings[LFUN__VALUES]) {
      return mknode(F_MAGIC_VALUES, mknewintnode(i),
		    mknewintnode(state_depth));
    } else if(ident == lfun_strings[LFUN__TYPES]) {
      return mknode(F_MAGIC_TYPES, mknewintnode(i),
		    mknewintnode(state_depth));
    }

    if (inherit_num && !TEST_COMPAT(7, 8) &&
	(state->new_program->num_inherits > 1)) {
      /* Check if there's an inherited lfun::`->() that we can call. */
      int id;
      struct program *prog = state->new_program->inherits[i].prog;
      if (((id = FIND_LFUN(prog, LFUN_ARROW)) == -1) &&
	  (prog == state->new_program)) {
	/* We are allowed to see private symbols in ourselves... */
	id = really_low_find_shared_string_identifier(lfun_strings[LFUN_ARROW],
						      prog,
						      SEE_PROTECTED|SEE_PRIVATE);
      } else if ((id != -1) && (prog != state->new_program)) {
	id = really_low_reference_inherited_identifier(state, i, id);
      }
      if ((id != -1) && (state->compiler_pass == COMPILER_PASS_LAST)) {
	if (inherit_num < 0) {
	  /* Find the closest inherit containing the lfun::`->()
	   * that is about to be called.
	   *
	   * In the single inherit case, this will always
	   * result in inherit_num == 1.
	   */
	  struct inherit *inherits = state->new_program->inherits;
	  inherit_num = PTR_FROM_INT(state->new_program, id)->inherit_offset;
	  while (inherits[inherit_num].inherit_level > 1) {
	    inherit_num--;
	  }
	}
	return mknode(F_APPLY, mkexternalnode(state->new_program, id),
		      mknode(F_ARG_LIST,
			     mkstrnode(ident),
			     mknode(F_ARG_LIST,
				    mkthisnode(state->new_program, inherit_num),
				    mkintnode(1))));
      }
    }
  }

  return NULL;
}

/* Fixme: allow level=0 to return the current level */
struct program *parent_compilation(int level)
{
  struct compilation *c = THIS_COMPILATION;
  int n;
  struct program_state *p=Pike_compiler->previous;
  for(n=0;n<level;n++)
  {
    if(n>=c->compilation_depth) return 0;
    p=p->previous;
    if(!p) return 0;
  }
  return p->new_program;
}

#define ID_TO_PROGRAM_CACHE_SIZE 512
struct program *id_to_program_cache[ID_TO_PROGRAM_CACHE_SIZE];

struct program *id_to_program(INT32 id)
{
  struct program_state *state;
  struct program *p;
  INT32 h;
  if(!id) return 0;

  /* fprintf(stderr, "id_to_program(%d)... ", id); */

  h=id & (ID_TO_PROGRAM_CACHE_SIZE-1);

  if((p=id_to_program_cache[h]))
    if(p->id==id) {
      /* fprintf(stderr, "cached: %p\n", p); */
      return p;
    }

  for(p=first_program;p;p=p->next)
  {
    if(id==p->id)
    {
      id_to_program_cache[h]=p;
      /* fprintf(stderr, "found: %p\n", p); */
      return p;
    }
  }

  /* Check if it is a program being compiled right now. */
  for (state = Pike_compiler; state; state = state->previous) {
    if (state->new_program && state->new_program->id == id) {
      return state->new_program;
    }
  }

  if ((id > 0) && (id < PROG_DYNAMIC_ID_START)) {
    /* Reserved id. Attempt to load the proper dynamic module
     * to resolv the id.
     */
    char *module = NULL;
    DECLARE_CYCLIC();

    if (!BEGIN_CYCLIC(((ptrdiff_t)id), 0)) {
      SET_CYCLIC_RET(1);

      /* fprintf(stderr, "reserved "); */

      switch(id) {
      case PROG_PARSER_HTML_ID:
	module = "Parser._parser";
	break;
      case PROG_GMP_MPZ_ID:
	module = "Gmp";
	break;
      case PROG_MODULE_MIME_ID:
	module = "___MIME";
	break;
      default:
	if ((id >= 100) && (id < 300)) {
	  module = "Image";
	} else if ((id >= 300) && (id < 400)) {
	  module = "Nettle";
	} else if ((id >= 1000) && (id < 2000)) {
	  module = "___GTK1"; /* Deprecated */
	} else if ((id >= 2000) && (id < 3000)) {
	  module = "___GTK2";
	}
	break;
      }
    }

    if (module && get_master()) {
      /* fprintf(stderr, "%s... ", module); */
      push_text(module);
      SAFE_APPLY_MASTER("resolv", 1);
      pop_stack();

      /* Try again... */
      for(p=first_program;p;p=p->next)
      {
	if(id==p->id)
	{
	  id_to_program_cache[h]=p;
	  /* fprintf(stderr, "found: %p\n", p); */
	  END_CYCLIC();
	  return p;
	}
      }
    }
    END_CYCLIC();
  }
  /* fprintf(stderr, "not found\n"); */
  return 0;
}

/* Here starts routines which are used to build new programs */

/*
 * A typical program goes through the following steps:
 *
 * allocate_program  ==> PROGRAM_VIRGIN
 *
 * start_program     ==> !PROGRAM_VIRGIN
 *
 * end_first_pass    ==> PROGRAM_PASS_1_DONE
 *
 * fixate_program    ==> PROGRAM_FIXED
 *
 * optimize_program  ==> PROGRAM_OPTIMIZED
 *
 * end_first_pass(1) ==> PROGRAM_FINISHED
 */

/**
 * Re-allocate all the memory in the program in one chunk. because:
 * @b 1) The individual blocks are much bigger than they need to be
 * @b 2) cuts down on malloc overhead (maybe)
 * @b 3) localizes memory access (decreases paging)
 */
void optimize_program(struct program *p)
{
  size_t size=0;
  char *data;

  /* Already done (shouldn't happen, but who knows?) */
  if(p->flags & PROGRAM_OPTIMIZED) return;

#ifdef PIKE_USE_MACHINE_CODE
  /* Don't move our mexec-allocated memory into the malloc... */
#define BAR(NUMTYPE,TYPE,ARGTYPE,NAME)
#endif /* PIKE_USE_MACHINE_CODE */

#define FOO(NUMTYPE,TYPE,ARGTYPE,NAME) \
  size=DO_ALIGN(size, ALIGNOF(TYPE)); \
  size+=p->PIKE_CONCAT(num_,NAME)*sizeof(p->NAME[0]);
#include "program_areas.h"

  data=malloc(size);
  if(!data)
  {
    make_program_executable(p);
    return; /* We are out of memory, but we don't care! */
  }

  size=0;

#ifdef PIKE_USE_MACHINE_CODE
  /* As above. */
#define BAR(NUMTYPE,TYPE,ARGTYPE,NAME)
#endif /* PIKE_USE_MACHINE_CODE */

#define FOO(NUMTYPE,TYPE,ARGTYPE,NAME) \
  size=DO_ALIGN(size, ALIGNOF(TYPE)); \
  if (p->PIKE_CONCAT (num_, NAME))					\
    memcpy(data+size,p->NAME,p->PIKE_CONCAT(num_,NAME)*sizeof(p->NAME[0])); \
  PIKE_CONCAT(RELOCATE_,NAME)(p, (TYPE *)(data+size)); \
  dmfree(p->NAME); \
  p->NAME=(TYPE *)(data+size); \
  size+=p->PIKE_CONCAT(num_,NAME)*sizeof(p->NAME[0]);
#include "program_areas.h"

  p->total_size=size + sizeof(struct program);

  p->flags |= PROGRAM_OPTIMIZED;
  make_program_executable(p);
}

/* internal function to make the index-table */
static int program_identifier_index_compare(int a, int b,
					    const struct program *p)
{
  size_t val_a = PTR_TO_INT (ID_FROM_INT(p, a)->name);
  size_t val_b = PTR_TO_INT (ID_FROM_INT(p, b)->name);
  return val_a < val_b ? -1 : (val_a == val_b ? (a < b? -1:(a != b)) : 1);
}

#define CMP(X,Y)	program_identifier_index_compare(*(X), *(Y), prog)
#define EXTRA_ARGS	, struct program *prog
#define XARGS		, prog
#define ID		fsort_program_identifier_index
#define TYPE		unsigned short
#include "fsort_template.h"
#undef TYPE
#undef ID
#undef XARGS
#undef EXTRA_ARGS
#undef CMP

#ifdef PIKE_DEBUG
struct pike_string *find_program_name(struct program *p, INT_TYPE *line)
{
  INT_TYPE l;
  if(!line) line=&l;

#ifdef DEBUG_MALLOC
  {
    const char *tmp=dmalloc_find_name(p);
    if (tmp) {
      char *p = strchr (tmp, ':');
      if (p) {
	char *pp;
	while ((pp = strchr (p + 1, ':'))) p = pp;
	*line = atoi (p + 1);
	return make_shared_binary_string (tmp, p - tmp);
      }
      else {
	*line=0;
	return make_shared_string(tmp);
      }
    }
  }
#endif

  return get_program_line(p, line);
}
#endif

int override_identifier (struct reference *new_ref, struct pike_string *name,
			 int required_flags)
{
  struct compilation *c = THIS_COMPILATION;
  int id = -1, cur_id = 0, is_used = 0;

  int new_is_variable =
    IDENTIFIER_IS_VARIABLE(ID_FROM_PTR(Pike_compiler->new_program,
				       new_ref)->identifier_flags);

  /* This loop could possibly be optimized by looping over
   * each inherit and looking up 'name' in each inherit
   * and then see if should be overwritten
   * /Hubbe
   */

  CHECK_COMPILER();

  for(;cur_id<Pike_compiler->new_program->num_identifier_references;cur_id++)
  {
    struct reference *ref =
      Pike_compiler->new_program->identifier_references + cur_id;
    struct identifier *i;

    /* No need to do anything for ourselves. */
    if (ref == new_ref) continue;

    /* Do not zapp hidden identifiers */
    if(ref->id_flags & ID_HIDDEN) continue;

    if(ref->id_flags & ID_VARIANT) continue;

    if ((ref->id_flags & required_flags) != required_flags) continue;

    /* Do not zapp functions with the wrong name... */
    if((i = ID_FROM_PTR(Pike_compiler->new_program, ref))->name != name)
      continue;

    /* Do not zapp inherited inline ('local') identifiers,
     * or inherited externals with new externals,
     * since this makes it hard to identify in encode_value().
     */
    if((ref->id_flags & (ID_INLINE|ID_INHERITED)) == (ID_INLINE|ID_INHERITED)
       || (ref->id_flags & new_ref->id_flags & ID_EXTERN)) {
      /* But we still need to hide them, since we shadow them... */
      ref->id_flags |= ID_HIDDEN;
      continue;
    }

#ifdef PROGRAM_BUILD_DEBUG
    fprintf(stderr, "%*soverloaded reference %d (id_flags:0x%04x)\n",
	    c->compilation_depth, "", cur_id, ref->id_flags);
#endif

    if (!new_is_variable && IDENTIFIER_IS_VARIABLE(i->identifier_flags)) {
      /* Overloading a variable with a constant or a function.
       * This is generally a bad idea.
       */
      ref->id_flags |= ID_INLINE|ID_HIDDEN;
      yywarning("Attempt to override a non local variable %S "
		"with a non-variable.", name);
      continue;
    }

    if ((ref->id_flags & (ID_INHERITED|ID_USED)) == (ID_INHERITED|ID_USED)) {
      struct inherit *inh = INHERIT_FROM_PTR(Pike_compiler->new_program, ref);
      struct reference *sub_ref;

      /* Find the inherit one level away. */
      while (inh->inherit_level > 1) inh--;

#if 0
#ifdef PIKE_DEBUG
      if (!inh->inherit_level) {
	/* FIXME: This is valid for references that are about to be
	 *        overridden by the variant dispatcher.
	 */
	Pike_fatal("Inherit without intermediate levels.\n");
      }
#endif
#endif

      sub_ref = PTR_FROM_INT(inh->prog, cur_id - inh->identifier_level);

      /* Check if the symbol was used before it was inherited. */
      if ((c->lex.pragmas & ID_STRICT_TYPES) &&
	  (sub_ref->id_flags & ID_USED)) {
	struct identifier *sub_id = ID_FROM_PTR(inh->prog, sub_ref);
	if (IDENTIFIER_IS_FUNCTION(sub_id->identifier_flags)) {
	  if ((Pike_compiler->compiler_pass == COMPILER_PASS_LAST) &&
	      !pike_types_le(ID_FROM_PTR(Pike_compiler->new_program,
					 new_ref)->type, sub_id->type)) {
	    yytype_report(REPORT_WARNING,
			  NULL, 0, sub_id->type,
			  NULL, 0, ID_FROM_PTR(Pike_compiler->new_program,
					       new_ref)->type,
			  0, "Type mismatch when overloading function %S.",
			  name);
	  }
	} else {
	  struct identifier *new_id;
	  /* Variable or constant.
	   *
	   * Note: Use weaker check for integers (especially for constants).
	   */
	  if (!pike_types_le(sub_id->type,
			     (new_id = ID_FROM_PTR(Pike_compiler->new_program,
						   new_ref))->type) &&
	      !((i->run_time_type == PIKE_T_INT) &&
		(new_id->run_time_type == PIKE_T_INT) &&
		(IDENTIFIER_IS_CONSTANT(sub_id->identifier_flags) ||
		 match_types(sub_id->type, new_id->type)))) {
	    yytype_report(REPORT_WARNING,
			  NULL, 0, sub_id->type,
			  NULL, 0, new_id->type,
			  0, "Type mismatch when overloading %S.", name);
	  }
	}
      }
    }
    is_used = ref->id_flags & ID_USED;

    *ref=*new_ref;
    ref->id_flags |= is_used;
    id = cur_id;
  }

  return id;
}

void fixate_program(void)
{
  struct compilation *c = THIS_COMPILATION;
  INT32 i,e,t;
  struct program *p=Pike_compiler->new_program;

  CHECK_COMPILER();

  if(p->flags & PROGRAM_FIXED) return;
#ifdef PIKE_DEBUG
  if(p->flags & PROGRAM_OPTIMIZED)
    Pike_fatal("Cannot fixate optimized program\n");
#endif

  /* Fixup the runtime type for functions.
   * (T_MIXED is used as the tentative type marker in pass 1).
   */
  for (i=0; i < p->num_identifiers; i++) {
    if (IDENTIFIER_IS_FUNCTION(p->identifiers[i].identifier_flags) &&
	(p->identifiers[i].run_time_type == T_MIXED)) {
      /* Get rid of the remaining tentative type marker. */
      /* FIXME: Should probably never be reachable.
       *        Consider this a fatal?
       */
      p->identifiers[i].run_time_type = T_FUNCTION;
    }
  }

  /* Fixup identifier overrides. */
  for (i = 0; i < p->num_identifier_references; i++) {
    struct reference *ref = p->identifier_references + i;
    if (ref->id_flags & ID_HIDDEN) continue;
    if (ref->id_flags & ID_VARIANT) continue;
    if (ref->inherit_offset != 0) continue;
    override_identifier (ref, ID_FROM_PTR (p, ref)->name, 0);
  }

  /* Ok, sort for binsearch */
  for(e=i=0;i<(int)p->num_identifier_references;i++)
  {
    struct reference *funp;
    struct identifier *fun;
    funp=p->identifier_references+i;
    if(funp->id_flags & ID_HIDDEN) continue;
    if(funp->id_flags & ID_VARIANT) continue;
    fun=ID_FROM_PTR(p, funp);
    if(funp->id_flags & ID_INHERITED)
    {
      int found_better=-1;
      int funa_is_prototype;

      /* NOTE: Mixin is currently not supported for PRIVATE symbols. */
      if(funp->id_flags & ID_PRIVATE) continue;
      funa_is_prototype = fun->func.offset == -1;
/*    if(fun->func.offset == -1) continue; * prototype */

      /* check for multiple definitions */
      for(t=i+1;t<(int)p->num_identifier_references;t++)
      {
	struct reference *funpb;
	struct identifier *funb;

	funpb=p->identifier_references+t;
	if (funpb->id_flags & ID_HIDDEN) continue;
	if (funpb->id_flags & ID_VARIANT) continue;
	funb=ID_FROM_PTR(p,funpb);
	/* if(funb->func.offset == -1) continue; * prototype */

	if(fun->name==funb->name)
	{
	  if (!(funpb->id_flags & ID_PROTECTED)) {
	    /* Only regard this symbol as better if it
	     * will end up in the index further below.
	     */
	    found_better=t;
	  }

	  /* FIXME: Is this stuff needed?
	   *        It looks like it already is done by define_function().
	   *
	   *        Yes -- It's needed in case of mixin.
	   */
	  if(funa_is_prototype && (funb->func.offset != -1) &&
	     !(funp->id_flags & ID_INLINE))
	  {
	    if ((c->lex.pragmas & ID_STRICT_TYPES) &&
		(funp->id_flags & ID_USED)) {
	      /* Verify that the types are compatible. */
	      if (!pike_types_le(funb->type, fun->type)) {
		yytype_report(REPORT_WARNING, NULL, 0, fun->type,
			      NULL, 0, funb->type,
			      0, "Type mismatch when overloading %S.",
			      fun->name);
	      }
	    }
	    funp->inherit_offset = funpb->inherit_offset;
	    funp->identifier_offset = funpb->identifier_offset;
	  }
	  if(!funa_is_prototype && funb->func.offset == -1)
	  {
	    if ((c->lex.pragmas & ID_STRICT_TYPES) &&
		(funpb->id_flags & ID_USED)) {
	      /* Verify that the types are compatible. */
	      if (!pike_types_le(fun->type, funb->type)) {
		yytype_report(REPORT_WARNING, NULL, 0, funb->type,
			      NULL, 0, fun->type,
			      0, "Type mismatch when overloading %S.",
			      fun->name);
	      }
	    }
	    funpb->inherit_offset = funp->inherit_offset;
	    funpb->identifier_offset = funp->identifier_offset;
	  }
	}
      }
      if(found_better!=-1)
	continue;
    }
    if (IDENTIFIER_IS_PIKE_FUNCTION(fun->identifier_flags) &&
	(fun->func.offset == -1) && (funp->id_flags & ID_INLINE)) {
      my_yyerror("Missing definition for local function %S.",
		 fun->name);
    }
    if (funp->id_flags & ID_PROTECTED) continue;
    add_to_identifier_index(i);
  }
  fsort_program_identifier_index(p->identifier_index,
				 p->identifier_index +
				 p->num_identifier_index - 1,
				 p);

  /* Take care of duplicates in the identifier index table;
   * this can happen eg when the overloading definition is
   * before an inherit being overloaded. This happens for
   * eg the default master object.
   */
  if (p->num_identifier_index) {
    struct identifier *id = ID_FROM_INT(p, p->identifier_index[0]);
    for (e = i = 1; e < p->num_identifier_index; e++) {
      struct identifier *probe = ID_FROM_INT(p, p->identifier_index[e]);
      if (probe == id) {
	/* Duplicate. */
	continue;
      }
      p->identifier_index[i++] = p->identifier_index[e];
      id = probe;
    }
    p->num_identifier_index = i;
  }

  p->flags |= PROGRAM_FIXED;

  /* Yes, it is supposed to start at 1  /Hubbe */
  for(i=1;i<NUM_LFUNS;i++) {
    int id = p->lfuns[i] = low_find_lfun(p, i);
    if (id >= 0) {
      // LFUNs are used.
      p->identifier_references[id].id_flags |= ID_USED;
    }
  }

  /* Complain about unused private symbols. */
  for (i = 0; i < p->num_identifier_references; i++) {
    struct reference *ref = p->identifier_references + i;
    if (ref->id_flags & ID_HIDDEN) continue;
    if (ref->id_flags & ID_VARIANT) continue;
    if (ref->inherit_offset != 0) continue;

    if ((ref->id_flags & (ID_HIDDEN|ID_PRIVATE|ID_USED)) == ID_PRIVATE) {
      yywarning("%S is private but not used anywhere.",
		ID_FROM_PTR(p, ref)->name);
    }
  }

  /* Set the PROGRAM_LIVE_OBJ flag by looking for _destruct() and
   * inherited PROGRAM_LIVE_OBJ flags. This is done at fixation time
   * to allow the user to set and clear that flag while the program is
   * being built. */
  if (!(p->flags & PROGRAM_LIVE_OBJ)) {
    int e, destruct = p->lfuns[LFUN__DESTRUCT];
    if (destruct > -1) {
      struct identifier *id = ID_FROM_INT (p, destruct);
      if (!IDENTIFIER_IS_PIKE_FUNCTION (id->identifier_flags) ||
	  id->func.offset != -1) {
	/* Got a _destruct function that isn't a prototype. */
	p->flags |= PROGRAM_LIVE_OBJ;
	goto program_live_obj_set;
      }
    }

    for (e = p->num_inherits - 1; e >= 0; e--)
      if (p->inherits[e].prog->flags & PROGRAM_LIVE_OBJ) {
	p->flags |= PROGRAM_LIVE_OBJ;
	break;
      }

  program_live_obj_set:;
  }

  if(Pike_compiler->flags & COMPILATION_CHECK_FINAL)
  {
    for(i=0;i<(int)p->num_identifier_references;i++)
    {
      if((p->identifier_references[i].id_flags & (ID_FINAL|ID_HIDDEN)) ==
	 ID_FINAL)
      {
        struct pike_string *name=ID_FROM_INT(p, i)->name;

	e=find_shared_string_identifier(name,p);
	if(e == -1)
	  e=really_low_find_shared_string_identifier(name, p,
						     SEE_PROTECTED|SEE_PRIVATE);

	if((e != i) && (e != -1))
	{
	  my_yyerror("Illegal to redefine final identifier %S", name);
	}
      }
    }
  }

#ifdef DEBUG_MALLOC
  {
#define DBSTR(X) ((X)?(X)->str:"")
    int e,v;
    INT_TYPE line;
    struct pike_string *tmp;
    struct memory_map *m=0;;
    if(c->lex.current_file &&
       c->lex.current_file->str &&
       c->lex.current_file->len &&
       !strcmp(c->lex.current_file->str,"-"))
    {
      m=dmalloc_alloc_mmap( DBSTR(c->lex.current_file), c->lex.current_line);
    }
    else if( (tmp=find_program_name(Pike_compiler->new_program, &line)) )
    {
      m=dmalloc_alloc_mmap( tmp->str, line);
      free_string(tmp);
    }else{
      m=dmalloc_alloc_mmap( "program id", Pike_compiler->new_program->id);
    }

    for(e=0;e<Pike_compiler->new_program->num_inherits;e++)
    {
      struct inherit *i=Pike_compiler->new_program->inherits+e;
      char *tmp;
      struct pike_string *tmp2 = NULL;
      char buffer[50];

      for(v=0;v<i->prog->num_variable_index;v++)
      {
	int d=i->prog->variable_index[v];
	struct identifier *id=i->prog->identifiers+d;

	dmalloc_add_mmap_entry(m,
			       id->name->str,
			       /* OFFSETOF(object,storage) + */ i->storage_offset + id->func.offset,
			       sizeof_variable(id->run_time_type),
			       1, /* count */
			       0,0);
      }

      if(i->name)
      {
	tmp=i->name->str;
      }
      else if(!(tmp2 = find_program_name(i->prog, &line)))
      {
	sprintf(buffer,"inherit[%d]",e);
	tmp=buffer;
      } else {
	tmp = tmp2->str;
      }
      dmalloc_add_mmap_entry(m,
			     tmp,
			     /* OFFSETOF(object, storage) + */ i->storage_offset,
			     i->prog->storage_needed - i->prog->inherits[0].storage_offset,
			     1, /* count */
			     0,0);
      if (tmp2) {
	free_string(tmp2);
      }
    }
    dmalloc_set_mmap_template(Pike_compiler->new_program, m);
  }
#endif
}

struct program *low_allocate_program(void)
{
  struct program *p=alloc_program();
  memset(p, 0, sizeof(struct program));
  gc_init_marker(p);
  p->flags|=PROGRAM_VIRGIN;
  p->alignment_needed=1;

  GC_ALLOC(p);
  p->id=++current_program_id;
  INIT_PIKE_MEMOBJ(p, T_PROGRAM);

  DOUBLELINK(first_program, p);
  ACCURATE_GETTIMEOFDAY(& p->timestamp);
  return p;
}

/**
 * Start building a new program
 */
void low_start_new_program(struct program *p,
			   int pass,
			   struct pike_string *name,
			   int flags,
			   int *idp)
{
  struct compilation *c = THIS_COMPILATION;
  int id=0;
  struct svalue tmp;

  CHECK_COMPILER();

  /* We aren't thread safe, but we are reentrant.
   *
   * Lock out any other threads from the compiler until we are finished.
   */
  lock_pike_compiler();

  c->compilation_depth++;

  SET_SVAL_TYPE(tmp, T_PROGRAM);
  if(!p)
  {
    p=low_allocate_program();
    if(name)
    {
      tmp.u.program=p;
      id=add_constant(name, &tmp, flags & ~ID_EXTERN);
#if 0
      fprintf(stderr,"Compiling class %s, depth=%d\n",
	      name->str, c->compilation_depth);
    }else{
      fprintf(stderr,"Compiling file %s, depth=%d\n",
	      c->lex.current_file ? c->lex.current_file->str : "-",
	      c->compilation_depth);
#endif
    }
  }else{
    tmp.u.program=p;
    add_ref(p);
    if((pass != COMPILER_PASS_FIRST) && name)
    {
      struct identifier *i;
      id=isidentifier(name);
      if (id < 0)
	Pike_fatal("Program constant disappeared in second pass.\n");
      i=ID_FROM_INT(Pike_compiler->new_program, id);
      free_type(i->type);
      i->type=get_type_of_svalue(&tmp);
    }
  }
  if (pass == COMPILER_PASS_FIRST) {
    if(c->compilation_depth >= 1) {
      add_ref(p->parent = Pike_compiler->new_program);
      debug_malloc_touch (p);
    }
  }
  p->flags &=~ PROGRAM_VIRGIN;
  if(idp) *idp=id;

  CDFPRINTF("th(%ld) %p low_start_new_program() %s "
            "pass=%d: lock_depth:%d, compilation_depth:%d\n",
            (long)th_self(), p, name ? name->str : "-",
            Pike_compiler->compiler_pass,
            lock_depth, c->compilation_depth);

  init_type_stack();

#define PUSH
#include "compilation.h"

  ba_init(&Pike_compiler->node_allocator, sizeof(struct node_s), 512);

  Pike_compiler->parent_identifier=id;
  Pike_compiler->compiler_pass = pass;

  Pike_compiler->num_used_modules=0;	/* FIXME: Duplicate? */

  if(p->flags & PROGRAM_FINISHED)
  {
    yyerror("Pass2: Program already done");
  }

  Pike_compiler->malloc_size_program = ALLOC_STRUCT(program);
  Pike_compiler->fake_object=alloc_object();

#ifdef PIKE_DEBUG
  Pike_compiler->fake_object->storage=(char *)malloc(256 * sizeof(struct svalue));
  if (Pike_compiler->fake_object->storage) {
    /* Stipple to find illegal accesses */
    memset(Pike_compiler->fake_object->storage,0x55,256*sizeof(struct svalue));
    PIKE_MEM_WO_RANGE (Pike_compiler->fake_object->storage,
		       256 * sizeof (struct svalue));
  }
#else
  Pike_compiler->fake_object->storage=(char *)malloc(sizeof(struct parent_info));
#endif
  if (!Pike_compiler->fake_object->storage) {
    yyerror("Out of memory when allocating object storage.");
  }
  /* Can't use GC_ALLOC on fake objects, but still it's good to know
   * that they never take over a stale gc marker. */
  if (Pike_in_gc) remove_marker(Pike_compiler->fake_object);

  Pike_compiler->fake_object->next=Pike_compiler->fake_object;
  Pike_compiler->fake_object->prev=Pike_compiler->fake_object;
  Pike_compiler->fake_object->refs=0;
  add_ref(Pike_compiler->fake_object);	/* For DMALLOC... */
  Pike_compiler->fake_object->prog=p;
  add_ref(p);

#ifdef PIKE_DEBUG
  Pike_compiler->fake_object->program_id=p->id;
#endif

  debug_malloc_touch(Pike_compiler->fake_object);
  debug_malloc_touch(Pike_compiler->fake_object->storage);

  if (Pike_compiler->fake_object->storage) {
    if(name)
    {
      /* Fake objects have parents regardless of PROGRAM_USES_PARENT */
      if((((struct parent_info *)Pike_compiler->fake_object->storage)->parent=Pike_compiler->previous->fake_object))
	add_ref(Pike_compiler->previous->fake_object);
      ((struct parent_info *)Pike_compiler->fake_object->storage)->parent_identifier=id;
    }else{
      ((struct parent_info *)Pike_compiler->fake_object->storage)->parent=0;
      ((struct parent_info *)Pike_compiler->fake_object->storage)->parent_identifier=0;
    }
  }

  Pike_compiler->new_program=p;

#ifdef PROGRAM_BUILD_DEBUG
  if (name) {
    fprintf (stderr, "%*sstarting program %d (pass=%d): ",
	     c->compilation_depth, "",
	     Pike_compiler->new_program->id, Pike_compiler->compiler_pass);
    push_string (name);
    print_svalue (stderr, --Pike_sp);
    putc ('\n', stderr);
  }
  else
    fprintf (stderr, "%*sstarting program %d (pass=%d)\n",
	     c->compilation_depth, "",
	     Pike_compiler->new_program->id, Pike_compiler->compiler_pass);
#endif

  if (c->compilation_depth >= 1) {
    if(c->lex.pragmas & ID_SAVE_PARENT)
    {
      p->flags |= PROGRAM_USES_PARENT;
    } else if (!(c->lex.pragmas & ID_DONT_SAVE_PARENT)) {
      struct pike_string *tmp=findstring("__pragma_save_parent__");
      if(tmp)
      {
	struct node_s *n=find_module_identifier(tmp, 0);
	if(n)
	{
	  int do_save_parent = !node_is_false(n); /* Default to true. */
	  free_node(n);
	  if (do_save_parent) p->flags |= PROGRAM_USES_PARENT;
	}
      }
    }
  }

  debug_malloc_touch(Pike_compiler->fake_object);
  debug_malloc_touch(Pike_compiler->fake_object->storage);

  if(Pike_compiler->new_program->program)
  {
#define FOO(NUMTYPE,TYPE,ARGTYPE,NAME)					\
    Pike_compiler->malloc_size_program->PIKE_CONCAT(num_,NAME) =	\
      Pike_compiler->new_program->PIKE_CONCAT(num_,NAME);
#include "program_areas.h"

    {
      INT_TYPE line = 0;
      INT32 off = 0;
      size_t len = 0;
      INT32 shift = 0;
      struct pike_string *file=0;
      char *cnt=Pike_compiler->new_program->linenumbers;

      while(cnt < Pike_compiler->new_program->linenumbers +
	    Pike_compiler->new_program->num_linenumbers)
      {
	if(*cnt == 127)
	{
	  int strno;
	  cnt++;
	  strno = get_small_number(&cnt);
	  CHECK_FILE_ENTRY (Pike_compiler->new_program, strno);
	  file = Pike_compiler->new_program->strings[strno];
	}
	off+=get_small_number(&cnt);
	line+=get_small_number(&cnt);
      }
      Pike_compiler->last_line=line;
      Pike_compiler->last_pc=off;
      if(file)
      {
	if(Pike_compiler->last_file) free_string(Pike_compiler->last_file);
	copy_shared_string(Pike_compiler->last_file, file);
      }
    }

  }else{
    static struct pike_string *s;
    struct inherit i;

    if (Pike_compiler->new_program->strings) {
      struct pike_string **str = Pike_compiler->new_program->strings;
      int j = Pike_compiler->new_program->num_strings;
      while(j--) {
	free_string(*str);
	str++;
      }
    }

#define START_SIZE 64
#ifdef PIKE_USE_MACHINE_CODE
#define BAR(NUMTYPE,TYPE,ARGTYPE,NAME)	\
    if (Pike_compiler->new_program->NAME) {				\
      mexec_free(Pike_compiler->new_program->NAME);			\
      Pike_compiler->new_program->PIKE_CONCAT(num_,NAME) = 0;		\
    }									\
    Pike_compiler->malloc_size_program->PIKE_CONCAT(num_,NAME) =	\
      START_SIZE;							\
    Pike_compiler->new_program->NAME =					\
      (TYPE *)mexec_alloc(sizeof(TYPE) * START_SIZE);
#endif /* PIKE_USE_MACHINE_CODE */
#define FOO(NUMTYPE,TYPE,ARGTYPE,NAME)					\
    if (Pike_compiler->new_program->NAME) {				\
      free (Pike_compiler->new_program->NAME);				\
      Pike_compiler->new_program->PIKE_CONCAT(num_,NAME) = 0;		\
    }									\
    Pike_compiler->malloc_size_program->PIKE_CONCAT(num_,NAME) =	\
      START_SIZE;							\
    Pike_compiler->new_program->NAME =					\
      (TYPE *)xalloc(sizeof(TYPE) * START_SIZE);
#include "program_areas.h"

    i.prog=Pike_compiler->new_program;
    i.identifier_level=0;
    i.storage_offset=0;
    i.inherit_level=0;
    i.identifier_ref_offset=0;
    i.parent=0;
    i.parent_identifier=-1;
    i.parent_offset=OBJECT_PARENT;
    i.name=0;
    add_to_inherits(i);
  }

  Pike_compiler->init_node=0;
  Pike_compiler->num_parse_error=0;

  push_compiler_frame(0);
  copy_pike_type(Pike_compiler->compiler_frame->current_return_type,
		 void_type_string);

  debug_malloc_touch(Pike_compiler->fake_object);
  debug_malloc_touch(Pike_compiler->fake_object->storage);
}

PMOD_EXPORT void debug_start_new_program(INT_TYPE line, const char *file)
{
  struct pike_string *save_file;
  INT_TYPE save_line;
  struct compilation *c;

  CHECK_COMPILER();
  c = THIS_COMPILATION;

  save_file = dmalloc_touch(struct pike_string *, c->lex.current_file);
  save_line = c->lex.current_line;

  { /* Trim off the leading path of the compilation environment. */
    const char *p = DEFINETOSTR(PIKE_SRC_ROOT), *f = file;
    while (*p && *p == *f) p++, f++;
    while (*f == '/' || *f == '\\') f++;

    c->lex.current_file = make_shared_string(f);
    c->lex.current_line = line;
  }

  CDFPRINTF("th(%ld) start_new_program(%ld, %s): "
            "lock_depth:%d, compilation_depth:%d\n",
            (long)th_self(), (long)line, file,
            lock_depth, c->compilation_depth);

  low_start_new_program(0, COMPILER_PASS_FIRST, 0, 0, 0);
  store_linenumber(line,c->lex.current_file);
  debug_malloc_name(Pike_compiler->new_program, file, line);

  free_string(c->lex.current_file);
  c->lex.current_file = dmalloc_touch(struct pike_string *, save_file);
  c->lex.current_line = save_line;
}


static void exit_program_struct(struct program *p)
{
  unsigned e;

#ifdef PIKE_DEBUG
  if (p->refs) {
#ifdef DEBUG_MALLOC
    fprintf (stderr, "Program to be freed still got %d references:\n", p->refs);
    describe_something(p, T_PROGRAM, 0,2,0, NULL);
#endif
    Pike_fatal("Program to be freed still got %d references.\n", p->refs);
  }
#endif

  if(p->parent)
  {
    /* Make sure to break the circularity... */
    struct program *parent = p->parent;
    p->parent = NULL;
#ifdef PIKE_DEBUG
    if (!parent->refs) {
      dump_program_tables(p, 2);
      fprintf(stderr, "Dead parent:\n");
      dump_program_tables(parent, 2);
      Pike_fatal("Program parent is dead.\n");
    }
#endif
    free_program(parent);
  }

  /* fprintf(stderr, "Exiting program: %p, id:%d\n", p, p->id); */

  if(id_to_program_cache[p->id & (ID_TO_PROGRAM_CACHE_SIZE-1)]==p)
    id_to_program_cache[p->id & (ID_TO_PROGRAM_CACHE_SIZE-1)]=0;

  if(p->strings)
    for(e=0; e<p->num_strings; e++)
      if(p->strings[e])
	free_string(p->strings[e]);

  if(p->identifiers)
  {
    for(e=0; e<p->num_identifiers; e++)
    {
      if(p->identifiers[e].name)
	free_string(p->identifiers[e].name);
      if(p->identifiers[e].type)
	free_type(p->identifiers[e].type);
    }
  }

  if(p->constants)
  {
    for(e=0;e<p->num_constants;e++)
    {
      free_svalue(& p->constants[e].sval);
#if 0
      if(p->constants[e].name) free_string(p->constants[e].name);
#endif /* 0 */
    }
  }

  if(p->inherits)
    for(e=0; e<p->num_inherits; e++)
    {
      if(p->inherits[e].name)
	free_string(p->inherits[e].name);
      if(e)
      {
	if(p->inherits[e].prog)
	  free_program(p->inherits[e].prog);
      }
      if(p->inherits[e].parent)
	free_object(p->inherits[e].parent);
    }

  DOUBLEUNLINK(first_program, p);

  if(p->flags & PROGRAM_OPTIMIZED)
  {
#ifdef PIKE_USE_MACHINE_CODE
    do {
      /* NOTE: Assumes all BAR's are before any FOO. */
#define BAR(NUMTYPE,TYPE,ARGTYPE,NAME)		\
      if (p->NAME) mexec_free(p->NAME);
#define FOO(NUMTYPE,TYPE,ARGTYPE,NAME)		\
      if (p->NAME) {				\
	dmfree(p->NAME);			\
	break;					\
      }
#include "program_areas.h"
    } while(0);
#else /* PIKE_USE_MACHINE_CODE */
    if(p->program) {
      dmfree(p->program);
    }
#endif /* PIKE_USE_MACHINE_CODE */
#define FOO(NUMTYPE,TYPE,ARGTYPE,NAME)	do {			     \
      p->NAME=0;						     \
    } while(0);

#include "program_areas.h"
  }else{
#ifdef PIKE_USE_MACHINE_CODE
#define BAR(NUMTYPE,TYPE,ARGTYPE,NAME)				\
    if(p->NAME) {						\
      mexec_free((char *)p->NAME); p->NAME=0;			\
    }
#endif /* PIKE_USE_MACHINE_CODE */
#define FOO(NUMTYPE,TYPE,ARGTYPE,NAME)			\
    if(p->NAME) {					\
      dmfree((char *)p->NAME); p->NAME=0;		\
    }
#include "program_areas.h"
  }

  EXIT_PIKE_MEMOBJ(p);

  GC_FREE(p);
}

#ifdef PIKE_DEBUG
void dump_program_desc(struct program *p)
{
  int e,d,q;
/*  fprintf(stderr,"Program '%s':\n",p->name->str); */

  fprintf(stderr,"$$$$$ dump_program_desc for %p\n", p);

  fprintf(stderr,"All inherits:\n");
  for(e=0;e<p->num_inherits;e++)
  {
    for(d=0;d<p->inherits[e].inherit_level;d++) fprintf(stderr,"  ");
    fprintf(stderr,"%3d:\n",e);

    for(d=0;d<p->inherits[e].inherit_level;d++) fprintf(stderr,"  ");
    fprintf(stderr,"inherited program: %d\n",p->inherits[e].prog->id);

    if(p->inherits[e].name)
    {
      for(d=0;d<p->inherits[e].inherit_level;d++) fprintf(stderr,"  ");
      fprintf(stderr,"name  : %s\n",p->inherits[e].name->str);
    }

    for(d=0;d<p->inherits[e].inherit_level;d++) fprintf(stderr,"  ");
    fprintf(stderr,"inherit_level: %d\n",p->inherits[e].inherit_level);

    for(d=0;d<p->inherits[e].inherit_level;d++) fprintf(stderr,"  ");
    fprintf(stderr,"identifier_level: %d\n",p->inherits[e].identifier_level);

    for(d=0;d<p->inherits[e].inherit_level;d++) fprintf(stderr,"  ");
    fprintf(stderr,"parent_identifier: %d\n",p->inherits[e].parent_identifier);

    for(d=0;d<p->inherits[e].inherit_level;d++) fprintf(stderr,"  ");
    fprintf(stderr,"parent_offset: %d\n",p->inherits[e].parent_offset);

    for(d=0;d<p->inherits[e].inherit_level;d++) fprintf(stderr,"  ");
    fprintf(stderr,"storage_offset: %ld\n",
            (long)p->inherits[e].storage_offset);

    for(d=0;d<p->inherits[e].inherit_level;d++) fprintf(stderr,"  ");
    fprintf(stderr,"parent: %p\n",p->inherits[e].parent);

    if(p->inherits[e].parent &&
      p->inherits[e].parent->prog &&
      p->inherits[e].parent->prog->num_linenumbers>1)
    {
      for(d=0;d<p->inherits[e].inherit_level;d++) fprintf(stderr,"  ");
      fprintf(stderr,"parent: %s\n",p->inherits[e].parent->prog->linenumbers+1);
    }
  }

  fprintf(stderr,"All identifier references:\n");
  for(e=0;e<(int)p->num_identifier_references;e++)
  {
    struct inherit *inh = INHERIT_FROM_INT(p,e);
    fprintf(stderr,"%3d: ",e);
    for(d=0;d<inh->inherit_level;d++) fprintf(stderr,"  ");
    fprintf(stderr,"%-40s  flags 0x%x",ID_FROM_INT(p,e)->name->str,
	    p->identifier_references[e].id_flags);
    for (q = 0; q < p->num_inherits; q++)
      if (p->inherits + q == inh) {
	fprintf(stderr,
		"  inherit %d\n"
		"       type: ",
		q);
	simple_describe_type(ID_FROM_INT(p, e)->type);
	fprintf(stderr, "\n");
	goto inherit_found;
      }
    fprintf (stderr, "  inherit not found!\n");
  inherit_found:;
  }
  fprintf(stderr,"All sorted identifiers:\n");
  for(q=0;q<(int)p->num_identifier_index;q++)
  {
    e=p->identifier_index[q];
    fprintf(stderr,"%3d (%3d):",e,q);
    for(d=0;d<INHERIT_FROM_INT(p,e)->inherit_level;d++) fprintf(stderr,"  ");
    fprintf(stderr,"%s;\n", ID_FROM_INT(p,e)->name->str);
  }

  fprintf(stderr,"$$$$$ dump_program_desc for %p done\n", p);
}
#endif

static void toss_compilation_resources(void)
{
  if(Pike_compiler->fake_object)
  {
    if( ((struct parent_info *)Pike_compiler->fake_object->storage)->parent )
    {
      free_object(((struct parent_info *)Pike_compiler->fake_object->storage)->parent);

      ((struct parent_info *)Pike_compiler->fake_object->storage)->parent=0;
    }

    free_program(Pike_compiler->fake_object->prog);
    Pike_compiler->fake_object->prog=0;
    free_object(Pike_compiler->fake_object);
    Pike_compiler->fake_object=0;
  }

  free_program(Pike_compiler->new_program);
  Pike_compiler->new_program=0;

  if(Pike_compiler->malloc_size_program)
    {
      dmfree((char *)Pike_compiler->malloc_size_program);
      Pike_compiler->malloc_size_program=0;
    }

  if(Pike_compiler->module_index_cache)
  {
    free_mapping(Pike_compiler->module_index_cache);
    Pike_compiler->module_index_cache=0;
  }

  while(Pike_compiler->compiler_frame)
    pop_compiler_frame();

  if(Pike_compiler->last_identifier)
  {
    free_string(Pike_compiler->last_identifier);
    Pike_compiler->last_identifier=0;
  }

  if(Pike_compiler->last_file)
  {
    free_string(Pike_compiler->last_file);
    Pike_compiler->last_file=0;
  }

  if (Pike_compiler->current_attributes) {
    free_node(Pike_compiler->current_attributes);
    Pike_compiler->current_attributes = NULL;
  }

  unuse_modules(Pike_compiler->num_used_modules);

  free_all_nodes();

  ba_destroy(&Pike_compiler->node_allocator);
}

int sizeof_variable(int run_time_type)
{
  switch(run_time_type)
  {
    case T_FUNCTION:
    case T_MIXED: return sizeof(struct svalue);
    case T_FLOAT: return sizeof(FLOAT_TYPE);
    case T_INT: return sizeof(INT_TYPE);
    case PIKE_T_FREE:
    case PIKE_T_GET_SET: return 0;
    default: return sizeof(void *);
  }
}

static ptrdiff_t alignof_variable(int run_time_type)
{
  switch(run_time_type)
  {
    case T_FUNCTION:
    case T_MIXED: return ALIGNOF(struct svalue);
    case T_FLOAT: return ALIGNOF(FLOAT_TYPE);
    case T_INT: return ALIGNOF(INT_TYPE);
    case PIKE_T_FREE:
    case PIKE_T_GET_SET: return 1;
    default: return ALIGNOF(void *);
  }
}

#ifdef PIKE_DEBUG

PMOD_EXPORT void dump_program_tables (const struct program *p, int indent)
{
  int d;

  if (!p) {
    fprintf(stderr, "%*sProgram: NULL\n\n", indent, "");
    return;
  }

  fprintf(stderr,
	  "%*sProgram flags: 0x%04x\n\n",
	  indent, "", p->flags);

  fprintf(stderr,
	  "%*sReference table:\n"
	  "%*s  ####: Flags Inherit Identifier\n",
	  indent, "", indent, "");
  for (d=0; d < p->num_identifier_references; d++) {
    struct reference *ref = p->identifier_references + d;
    struct identifier *id = ID_FROM_PTR(p, ref);
    struct program *inh_p = INHERIT_FROM_PTR(p, ref)->prog;

    fprintf(stderr,
	    "%*s  %4d: %5x %7d %10d  %s\n"
	    "%*s        %s:%ld\n",
	    indent, "",
	    d, ref->id_flags, ref->inherit_offset,
	    ref->identifier_offset,
	    id->name->size_shift ? "(wide)" : id->name->str,
	    indent, "",
	    inh_p->num_strings?inh_p->strings[id->filename_strno]->str:"-",
	    (long)id->linenumber);
    if (IDENTIFIER_IS_ALIAS(id->identifier_flags)) {
      fprintf (stderr, "%*s                                  Alias for %d:%d\n",
	       indent, "", id->func.ext_ref.depth, id->func.ext_ref.id);
    } else if (IDENTIFIER_IS_CONSTANT(id->identifier_flags)) {
      fprintf (stderr, "%*s                                  Constant #%ld\n",
	       indent, "", (long)id->func.const_info.offset);
    } else if (IDENTIFIER_IS_VARIABLE(id->identifier_flags)) {
      fprintf (stderr, "%*s                                  Offset: 0x%08lx\n",
	       indent, "", (long)id->func.offset);
    } else if (IDENTIFIER_IS_PIKE_FUNCTION(id->identifier_flags)) {
      INT_TYPE line;
      struct pike_string *file =
	get_line (ID_FROM_PTR(p,ref)->func.offset + inh_p->program, inh_p, &line);
      if (!file->size_shift)
	fprintf (stderr, "%*s                                  %s:%ld\n",
		 indent, "", file->str, (long)line);
      free_string (file);
    } else {
      fprintf (stderr, "%*s                                  Cfun: %p\n",
	       indent, "", id->func.c_fun);
    }
  }

  fprintf(stderr, "\n"
	  "%*sIdentifier index table:\n"
	  "%*s  ####: Index\tName\n",
	  indent, "", indent, "");
  for (d = 0; d < p->num_identifier_index; d++) {
    struct identifier *id = ID_FROM_INT(p, p->identifier_index[d]);
    fprintf(stderr, "%*s  %4d: %5d\t%s\n",
	    indent, "",
	    d, p->identifier_index[d],
	    id->name->size_shift ? "(wide)" : id->name->str);
  }

  fprintf(stderr, "\n"
	  "%*sInherit table:\n"
	  "%*s  ####: Level prog_id id_level storage_offs "
	  "par_id par_offs par_obj_id id_ref_offs\n",
	  indent, "", indent, "");
  for (d=0; d < p->num_inherits; d++) {
    struct inherit *inh = p->inherits + d;

    fprintf(stderr, "%*s  %4d: %5d %7d %8d %12"PRINTPTRDIFFT"d %6d %8d %10d %11"PRINTSIZET"d\n",
	    indent, "",
	    d, inh->inherit_level,
	    inh->prog ? inh->prog->id : -1,
	    inh->identifier_level, inh->storage_offset,
	    inh->parent_identifier, inh->parent_offset,
	    inh->parent ? inh->parent->program_id : -1,
	    inh->identifier_ref_offset);
  }
  fprintf(stderr, "\n"
	  "%*sIdentifier table:\n"
	  "%*s  ####: Flags Offset Type Name\n",
	  indent, "", indent, "");
  for (d=0; d < p->num_identifiers; d++) {
    struct identifier *id = p->identifiers + d;

    fprintf(stderr,
	    "%*s  %4d: %5x %6"PRINTPTRDIFFT"d %4d \"%s\"\n"
	    "%*s        %s:%ld\n",
	    indent, "",
	    d, id->identifier_flags, id->func.offset,
	    id->run_time_type, id->name->str,
	    indent, "",
	    p->num_strings?p->strings[id->filename_strno]->str:"-",
	    (long)id->linenumber);
  }

  fprintf(stderr, "\n"
	  "%*sVariable table:\n"
	  "%*s  ####: Index\n",
	  indent, "", indent, "");
  for (d = 0; d < p->num_variable_index; d++) {
    fprintf(stderr, "%*s  %4d: %5d\n",
	    indent, "",
	    d, p->variable_index[d]);
  }
  fprintf(stderr, "\n"
	  "%*sConstant table:\n"
	  "%*s  ####: Type            Raw\n",
	  indent, "", indent, "");
  for (d = 0; d < p->num_constants; d++) {
    struct program_constant *c = p->constants + d;
#if 1
    fprintf(stderr, "%*s  %4d: %-15s %p\n",
	    indent, "",
	    d, get_name_of_type (TYPEOF(c->sval)),
	    c->sval.u.ptr);
#else /* !0 */
    fprintf(stderr, "%*s  %4d: %-15s %"PRINTPTRDIFFT"d\n",
	    indent, "",
	    d, get_name_of_type (TYPEOF(c->sval)),
	    c->offset);
#endif /* 0 */
  }

  fprintf(stderr, "\n"
	  "%*sString table:\n"
	  "%*s  ####: Value\n",
	  indent, "", indent, "");
  for (d = 0; d < p->num_strings; d++) {
    fprintf(stderr, "%*s  %4d: [%p]\"%s\"(%"PRINTPTRDIFFT"d characters)\n",
	    indent, "", (int)d, p->strings[d], p->strings[d]->str, p->strings[d]->len);
  }

  fprintf(stderr, "\n"
	  "%*sLFUN table:\n"
	  "%*s  LFUN  Ref# Name\n",
	  indent, "", indent, "");
  for (d = 0; d < NUM_LFUNS; d++) {
    if (p->lfuns[d] != -1) {
      fprintf(stderr, "%*s  %4d: %04d %s\n",
	      indent, "", d, p->lfuns[d], lfun_names[d]);
    }
  }

  fprintf(stderr, "\n"
	  "%*sLinenumber table:\n",
	  indent, "");
  {
    INT32 off = 0;
    INT_TYPE line = 0;
    char *cnt = p->linenumbers;

    while (cnt < p->linenumbers + p->num_linenumbers) {
      if (*cnt == 127) {
	int strno;
	cnt++;
	strno = get_small_number(&cnt);
	fprintf(stderr, "%*s  Filename: String #%d\n", indent, "", strno);
      }
      off += get_small_number(&cnt);
      line += get_small_number(&cnt);
      fprintf(stderr, "%*s    %8d:%8ld\n", indent, "", off, (long)line);
    }
  }

  fprintf(stderr, "\n");
}

void check_program(struct program *p)
{
  unsigned INT32 e;
  int variable_positions[1024];

  if(p->flags & PROGRAM_AVOID_CHECK) return;

  for(e=0;e<NELEM(variable_positions);e++)
    variable_positions[e]=-1;

  if(p->id > current_program_id)
    Pike_fatal("Program id is out of sync! (p->id=%d, current_program_id=%d)\n",p->id,current_program_id);

  if(p->refs <=0)
    Pike_fatal("Program has zero refs.\n");

  if(p->next && p->next->prev != p)
    Pike_fatal("Program ->next->prev != program.\n");

  if(p->prev)
  {
    if(p->prev->next != p)
      Pike_fatal("Program ->prev->next != program.\n");
  }else{
    if(first_program != p)
      Pike_fatal("Program ->prev == 0 but first_program != program.\n");
  }

  if(p->id > current_program_id || p->id <= 0)
    Pike_fatal("Program id is wrong.\n");

  if(p->storage_needed < 0)
    Pike_fatal("Program->storage_needed < 0.\n");

  if(p->num_identifier_index > p->num_identifier_references)
    Pike_fatal("Too many identifier index entries in program!\n");

  for(e=0;e<p->num_constants;e++)
  {
    struct svalue *s = & p->constants[e].sval;
    check_svalue(s);
    if (p->flags & PROGRAM_FINISHED && TYPEOF(*s) == T_OBJECT &&
	s->u.object->next == s->u.object)
      Pike_fatal ("Got fake object in constant in finished program.\n");
#if 0
    if(p->constants[e].name) check_string(p->constants[e].name);
#else /* ! 0 */
    if (p->constants[e].offset >= p->num_identifiers) {
      Pike_fatal("Constant initializer outside num_identifiers (%d >= %d).\n",
		 p->constants[e].offset, p->num_identifiers);
    }
#endif /* 0 */
  }

  for(e=0;e<p->num_strings;e++)
    check_string(p->strings[e]);

  for(e=0;e<p->num_inherits;e++)
  {
    if(!p->inherits[e].prog)
    {
      /* This inherit is not yet initialized, ignore rest of tests.. */
      return;
    }

    if(p->inherits[e].storage_offset < 0)
      Pike_fatal("Inherit->storage_offset is wrong.\n");

    if(p->inherits[e].prog &&
       p->inherits[e].storage_offset + STORAGE_NEEDED(p->inherits[e].prog) >
       p->storage_needed)
      Pike_fatal("Not enough room allocated by inherit!\n");

    if (p->inherits[e].inherit_level == 1 &&
	p->inherits[e].identifier_level != (INT32) p->inherits[e].identifier_ref_offset) {
      dump_program_tables (p, 0);
      Pike_fatal ("Unexpected difference between identifier_level "
		  "and identifier_ref_offset in inherit %d.\n", e);
    }

    if(e)
    {
      if(p->inherits[e-1].storage_offset >
	 p->inherits[e].storage_offset)
	Pike_fatal("Overlapping inherits! (1)\n");

      if(p->inherits[e-1].prog &&
	 p->inherits[e-1].inherit_level >= p->inherits[e].inherit_level &&
	 ( p->inherits[e-1].storage_offset +
	   STORAGE_NEEDED(p->inherits[e-1].prog)) >
	 p->inherits[e].storage_offset)
	Pike_fatal("Overlapping inherits! (3)\n");
    }
  }


  if(p->flags & PROGRAM_FINISHED)
  for(e=0;e<p->num_identifiers;e++)
  {
    check_string(p->identifiers[e].name);

    switch (p->identifiers[e].identifier_flags & IDENTIFIER_TYPE_MASK) {
      case IDENTIFIER_VARIABLE:
      case IDENTIFIER_PIKE_FUNCTION:
      case IDENTIFIER_C_FUNCTION:
      case IDENTIFIER_CONSTANT:
	break;

      default:
	Pike_fatal("Invalid identifier type.\n");
    }

    if(p->identifiers[e].identifier_flags & ~IDENTIFIER_MASK)
      Pike_fatal("Unknown flags in identifier flag field.\n");

    if((p->identifiers[e].run_time_type!=T_MIXED) &&
       (p->identifiers[e].run_time_type!=PIKE_T_FREE) &&
       (p->identifiers[e].run_time_type!=PIKE_T_GET_SET))
      check_type(p->identifiers[e].run_time_type);

    if (!IDENTIFIER_IS_ALIAS(p->identifiers[e].identifier_flags)) {
      if(IDENTIFIER_IS_VARIABLE(p->identifiers[e].identifier_flags))
      {
	if((p->identifiers[e].func.offset /* + OFFSETOF(object,storage)*/ ) &
	    (alignof_variable(p->identifiers[e].run_time_type)-1))
	{
	  dump_program_tables(p, 0);
	  Pike_fatal("Variable %s offset is not properly aligned (%ld).\n",
		     p->identifiers[e].name->str,
                     (long)p->identifiers[e].func.offset);
	}
      }
    } else {
      /* FIXME: Check that ext_ref.depth and ext_ref.id are valid and
       *        have matching identifier_flags.
       */
      if (p->identifiers[e].func.ext_ref.depth &&
	  !(p->flags & PROGRAM_USES_PARENT)) {
	Pike_fatal("Identifier %d is an external reference, but "
		   "PROGRAM_USES_PARENT hasn't been set.\n",
		   e);
      }
    }
  }

  for(e=0;e<p->num_identifier_references;e++)
  {
    struct identifier *i;
    if(p->identifier_references[e].inherit_offset > p->num_inherits)
      Pike_fatal("Inherit offset is wrong!\n");

    if(!p->inherits[p->identifier_references[e].inherit_offset].prog)
    {
      if(!(p->flags & PROGRAM_FINISHED))
	continue;

      Pike_fatal("p->inherit[%d].prog = NULL!\n",p->identifier_references[e].inherit_offset);
    }

    if(p->identifier_references[e].identifier_offset >
       p->inherits[p->identifier_references[e].inherit_offset].prog->num_identifiers)
      Pike_fatal("Identifier offset %d is wrong! %d > %d\n",
	    e,
	    p->identifier_references[e].identifier_offset,
	    p->inherits[p->identifier_references[e].inherit_offset].prog->num_identifiers);

    i=ID_FROM_INT(p, e);

    if (IDENTIFIER_IS_ALIAS(i->identifier_flags)) {
      if ((!i->func.ext_ref.depth) && (i->func.ext_ref.id == e)) {
	dump_program_tables(p, 0);
	Pike_fatal("Circular alias for reference %d!\n", e);
      }
    } else if (IDENTIFIER_IS_VARIABLE(i->identifier_flags)) {
      size_t q, size;
      /* Variable */
      ptrdiff_t offset = INHERIT_FROM_INT(p, e)->storage_offset+i->func.offset;
      if (i->run_time_type == PIKE_T_GET_SET) {
	struct reference *ref = PTR_FROM_INT(p, e);
	if (!ref->inherit_offset) {
	  if (i->func.gs_info.getter >= p->num_identifier_references) {
	    Pike_fatal("Getter outside references.\n");
	  }
	  if (i->func.gs_info.setter >= p->num_identifier_references) {
	    Pike_fatal("Setter outside references.\n");
	  }
	}
	continue;
      }
      size=sizeof_variable(i->run_time_type);

      if(size && ((offset+size > (size_t)p->storage_needed) || offset<0))
	Pike_fatal("Variable outside storage! (%s)\n",i->name->str);

      for(q=0;q<size;q++)
      {
	if(offset+q >= NELEM(variable_positions)) break;

	if(variable_positions[offset+q] != -1)
	{
	  if(ID_FROM_INT(p,variable_positions[offset+q])->run_time_type !=
	     i->run_time_type)
	  {
	    fprintf(stderr, "Storage offset: "
		    "0x%08"PRINTPTRDIFFT"x vs 0x%08"PRINTPTRDIFFT"x\n"
		    "Func offset: 0x%08"PRINTPTRDIFFT"x vs 0x%08"PRINTPTRDIFFT"x\n"
		    "Type: %s vs %s\n",
		    INHERIT_FROM_INT(p, variable_positions[offset+q])->
		    storage_offset,
		    INHERIT_FROM_INT(p, e)->storage_offset,
		    ID_FROM_INT(p, variable_positions[offset+q])->func.offset,
		    i->func.offset,
		    get_name_of_type(ID_FROM_INT(p,variable_positions[offset+q]
						 )->run_time_type),
		    get_name_of_type(i->run_time_type));
	    if (i->name) {
	      Pike_fatal("Variable '%s' and '%s' overlap\n"
			 "Offset 0x%08"PRINTPTRDIFFT"x - 0x%08"PRINTPTRDIFFT"x "
			 "overlaps with "
			 "0x%08"PRINTPTRDIFFT"x - 0x%08"PRINTPTRDIFFT"x\n",
		    ID_FROM_INT(p, variable_positions[offset+q])->name->str,
		    i->name->str,
		    INHERIT_FROM_INT(p, variable_positions[offset+q])->
		    storage_offset +
		    ID_FROM_INT(p, variable_positions[offset+q])->func.offset,
		    INHERIT_FROM_INT(p, variable_positions[offset+q])->
		    storage_offset +
		    ID_FROM_INT(p, variable_positions[offset+q])->func.offset +
		    sizeof_variable(ID_FROM_INT(p, variable_positions[offset+q]
						)->run_time_type)-1,
		    offset, offset+size-1);
	    } else {
	      Pike_fatal("Variable '%s' and anonymous variable (%d) overlap\n"
			 "Offset 0x%08"PRINTPTRDIFFT"x - 0x%08"PRINTPTRDIFFT"x "
			 "overlaps with "
			 "0x%08"PRINTPTRDIFFT"x - 0x%08"PRINTPTRDIFFT"x\n",
		    ID_FROM_INT(p, variable_positions[offset+q])->name->str,
		    e,
		    INHERIT_FROM_INT(p, variable_positions[offset+q])->
		    storage_offset +
		    ID_FROM_INT(p, variable_positions[offset+q])->func.offset,
		    INHERIT_FROM_INT(p, variable_positions[offset+q])->
		    storage_offset +
		    ID_FROM_INT(p, variable_positions[offset+q])->func.offset +
		    sizeof_variable(ID_FROM_INT(p, variable_positions[offset+q]
						)->run_time_type)-1,
		    offset, offset+size-1);
	    }
	  }
	}
	variable_positions[offset+q]=e;
      }
    }
  }

  for(e=0;e<p->num_identifier_index;e++)
  {
    if(p->identifier_index[e] > p->num_identifier_references)
      Pike_fatal("Program->identifier_indexes[%ld] is wrong\n",(long)e);
    if (e && (program_identifier_index_compare(p->identifier_index[e-1],
					       p->identifier_index[e],
					       p) > 0)) {
      Pike_fatal("Program->identifier_index[%ld] > "
		 "Program->identifier_index[%ld]\n",
		 (long)(e-1), (long)e);
    }
  }

}
#endif

static void f_dispatch_variant(INT32 args);

int low_is_variant_dispatcher(struct identifier *id)
{
  if (!id) return 0;
  return (IDENTIFIER_IS_C_FUNCTION(id->identifier_flags) &&
	  !IDENTIFIER_IS_ALIAS(id->identifier_flags) &&
	  (id->func.c_fun == f_dispatch_variant));
}

int is_variant_dispatcher(struct program *prog, int fun)
{
  struct reference *ref;
  struct identifier *id;
  if (fun < 0) return 0;
  ref = PTR_FROM_INT(prog, fun);
  id = ID_FROM_PTR(prog, ref);
  return low_is_variant_dispatcher(id);
}

static int add_variant_dispatcher(struct pike_string *name,
				  struct pike_type *type,
				  int id_flags)
{
  union idptr dispatch_fun;
  dispatch_fun.c_fun = f_dispatch_variant;
  return define_function(name, type, id_flags & ~(ID_VARIANT|ID_LOCAL),
			 IDENTIFIER_C_FUNCTION, &dispatch_fun, 0);
}

/**
 * End the current compilation pass.
 *
 * @param finish
 *   finish-state:
 *
 *   0: First pass.
 *   1: Last pass.
 *   2: Called from decode_value().
 *
 * Note: This function is misnamed, since it's run after all passes.
 */
struct program *end_first_pass(int finish)
{
  struct compilation *c = THIS_COMPILATION;
  int e;
  struct program *prog = Pike_compiler->new_program;
  struct pike_string *init_name;
  int num_refs = prog->num_identifier_references;
  union idptr dispatch_fun;

  dispatch_fun.c_fun = f_dispatch_variant;

  /* Collect variant functions that have been defined in this program,
   * and add the corresponding dispatchers.
   */
  for (e = 0; e < num_refs; e++) {
    struct identifier *id;
    struct pike_string *name;
    struct pike_type *type;
    int id_flags;
    int opt_flags;
    int j;
    if (prog->identifier_references[e].inherit_offset) continue;
    if (!is_variant_dispatcher(prog, e)) continue;
    /* Found a dispatcher. */

    id = ID_FROM_INT(prog, e);
    name = id->name;
    type = NULL;
    id_flags = 0;
    opt_flags = 0;

    CDFPRINTF("Collecting variants of \"%s\"...\n", name->str);

    /* Collect the variants of the function. */
    j = prog->num_identifier_references;
    while ((j = really_low_find_variant_identifier(name, prog, NULL, j,
						   SEE_PROTECTED|SEE_PRIVATE)) >= 0) {
      struct reference *ref = prog->identifier_references + j;
      id = ID_FROM_INT(prog, j);
      id_flags |= ref->id_flags;
      opt_flags |= id->opt_flags;
      /* NB: The dispatcher needs the variant references to
       *     not get overloaded for the ::-operator to work.
       */
      ref->id_flags |= ID_LOCAL;
      if (type)
      {
	struct pike_type * temp = type;
	if ((Pike_compiler->compiler_pass == COMPILER_PASS_LAST) &&
	    !ref->inherit_offset &&
	    !check_variant_overload(id->type, type)) {
	  /* This symbol is shadowed by later variants. */
	  yytype_report(REPORT_WARNING,
			NULL, 0, NULL,
			Pike_compiler->new_program->strings[id->filename_strno],
			id->linenumber, id->type,
			0, "Function %S() masked by later variant.",
			name);
	  ref_push_type_value(type);
	  low_yyreport(REPORT_WARNING,
		       Pike_compiler->new_program->strings[id->filename_strno],
		       id->linenumber,
		       type_check_system_string,
		       1, "Variant : %O.");
	}
	type = or_pike_types(type, id->type, 1);
	free_type(temp);
      } else {
	add_ref(type = id->type);
      }
#ifdef COMPILER_DEBUG
      fprintf(stderr, "type: ");
      simple_describe_type(id->type);
      fprintf(stderr, "\n");
#endif
    }
#ifdef COMPILER_DEBUG
    fprintf(stderr, "Dispatcher type: ");
    simple_describe_type(type);
    fprintf(stderr, "\n");
#endif
    /* Update the type of the dispatcher. */
    id = ID_FROM_INT(prog, e);
    free_type(id->type);
    id->type = type;
    id->opt_flags = opt_flags;
    prog->identifier_references[e].id_flags |= id_flags & ~(ID_VARIANT|ID_LOCAL);
  next_ref:
    ;
  }

  debug_malloc_touch(Pike_compiler->fake_object);
  debug_malloc_touch(Pike_compiler->fake_object->storage);

  init_name = lfun_strings[LFUN___INIT];

  /* Collect references to inherited __INIT functions */
  if (!(Pike_compiler->new_program->flags & PROGRAM_AVOID_CHECK)) {
    for(e=Pike_compiler->new_program->num_inherits-1;e;e--)
    {
      int id;
      if(Pike_compiler->new_program->inherits[e].inherit_level!=1) continue;
      id = Pike_compiler->new_program->inherits[e].prog->lfuns[LFUN___INIT];
      id = really_low_reference_inherited_identifier(0, e, id);
      if(id!=-1)
      {
	Pike_compiler->init_node=mknode(F_COMMA_EXPR,
		         mkcastnode(void_type_string,
				    mkapplynode(mkidentifiernode(id),0)),
		         Pike_compiler->init_node);
      }
    }
  }

  if (finish == 1) {
    if (Pike_compiler->compiler_pass == COMPILER_PASS_FIRST) {
      /* Called from end_program(). */
      if (Pike_compiler->init_node) {
	/* Make sure that the __INIT symbol exists, so that
	 * we won't get a fatal when we add the actual code
	 * further down when we have entered pass 2.
	 *
	 * Also make sure that it is marked as having side effects,
	 * or it will be optimized away when inherited...
	 */
	define_function(init_name, function_type_string, ID_PROTECTED,
			IDENTIFIER_PIKE_FUNCTION, NULL,
			OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);
      }
    }
    Pike_compiler->compiler_pass = COMPILER_PASS_LAST;
  }

  /*
   * Define the __INIT function, but only if there was any code
   * to initialize.
   */

  if(Pike_compiler->init_node)
  {
    /* Inhibit this_function. */
    Pike_compiler->compiler_frame->current_function_number = -2;

    e=dooptcode(init_name,
		mknode(F_COMMA_EXPR,
		       Pike_compiler->init_node,
		       mknode(F_RETURN,mkintnode(0),0)),
		function_type_string,
		ID_PROTECTED);
    Pike_compiler->init_node=0;
  } else if (finish == 2) {
    /* Called from decode_value(). */
    e = low_find_lfun(Pike_compiler->new_program, LFUN___INIT);
    if ((e != -1) &&
	(ID_FROM_INT(Pike_compiler->new_program, e)->func.offset == -1)) {
      /* Just a prototype. Make sure not to call it. */
      e = -1;
    }
  }else{
    /* Note that we may zap an __INIT that existed in pass 1 here.
     * This is intentional to avoid having to keep track of whether
     * __INIT() is just a prototype or not.
     */
    e=-1;
  }
  Pike_compiler->new_program->lfuns[LFUN___INIT]=e;

  pop_compiler_frame(); /* Pop __INIT local variables */

  if(Pike_compiler->num_parse_error > 0)
  {
    CDFPRINTF("th(%ld) %p Compilation errors (%d).\n",
              (long)th_self(), Pike_compiler->new_program,
              Pike_compiler->num_parse_error);
    prog=0;
  }else{
    prog=Pike_compiler->new_program;
    add_ref(prog);

    Pike_compiler->new_program->flags |= PROGRAM_PASS_1_DONE;

    if(finish)
    {
      if(Pike_compiler->new_program->flags & PROGRAM_USES_PARENT)
      {
	Pike_compiler->new_program->parent_info_storage =
	  add_xstorage(sizeof(struct parent_info),
		       ALIGNOF(struct parent_info),
		       0);
      }else{
	/* Cause errors if used hopefully */
	Pike_compiler->new_program->parent_info_storage=-1;
      }

      fixate_program();
      if(Pike_compiler->num_parse_error)
      {
	free_program(prog);
	prog=0;
      }else{
	optimize_program(Pike_compiler->new_program);
	Pike_compiler->new_program->flags |= PROGRAM_FINISHED;
      }
    } else {
      /* All references in prog are now known.
       * Fixup identifier overrides or external symbols,
       * so that inherit is safe.
       */
      for (e = 0; e < prog->num_identifier_references; e++) {
	struct reference *ref = prog->identifier_references + e;
	if (ref->id_flags & ID_HIDDEN) continue;
	if (ref->inherit_offset != 0) continue;
	override_identifier (ref, ID_FROM_PTR (prog, ref)->name,
			     ID_EXTERN);
      }
    }

#ifdef PIKE_DEBUG
    if (prog) {
      check_program(prog);
      if(l_flag)
	dump_program_desc(prog);
    }
#endif
  }

#ifdef PROGRAM_BUILD_DEBUG
  fprintf (stderr, "%*sfinishing program %d (pass=%d)\n",
	   c->compilation_depth, "",
	   Pike_compiler->new_program->id, Pike_compiler->compiler_pass);
#endif

  toss_compilation_resources();

#if 0
  CDFPRINTF("th(%ld) end_first_pass(): "
            "%p compilation_depth:%d, Pike_compiler->compiler_pass:%d\n",
            (long)th_self(), prog,
            c->compilation_depth, Pike_compiler->compiler_pass);
#endif

  if(!Pike_compiler->compiler_frame &&
     (Pike_compiler->compiler_pass == COMPILER_PASS_LAST || !prog) &&
     c->resolve_cache)
  {
    free_mapping(dmalloc_touch(struct mapping *, c->resolve_cache));
    c->resolve_cache=0;
  }

#define POP
#include "compilation.h"

  exit_type_stack();


  CDFPRINTF("th(%ld) %p end_first_pass(%d): "
            "lock_depth:%d, compilation_depth:%d\n",
            (long)th_self(), prog, finish,
            lock_depth, c->compilation_depth);

  c->compilation_depth--;

  unlock_pike_compiler();

#if 0
#ifdef PIKE_USE_MACHINE_CODE
  if (prog &&
      (((unsigned long long *)prog->program)[-1] != 0xdeadfeedf00dfaddLL)) {
    Pike_fatal("Bad mexec magic!\n");
  }
#endif /* PIKE_USE_MACHINE_CODE */
#endif /* 0 */
  return prog;
}

/**
 * Finish this program, returning the newly built program
 */
PMOD_EXPORT struct program *debug_end_program(void)
{
  return end_first_pass(1);
}


/**
 * Allocate space needed for this program in the object structure.
 * An offset to the data is returned.
 */
PMOD_EXPORT size_t low_add_storage(size_t size, size_t alignment,
				   ptrdiff_t modulo_orig)
{
  ptrdiff_t offset;
  ptrdiff_t modulo;

  if(!size) return Pike_compiler->new_program->storage_needed;

#ifdef PIKE_DEBUG
  if(alignment <=0 || (alignment & (alignment-1)) || alignment > 256)
    Pike_fatal("Alignment must be 1,2,4,8,16,32,64,128 or 256 not %ld\n",
               (long)alignment);
#endif
  modulo=( modulo_orig /* +OFFSETOF(object,storage) */ ) % alignment;

  offset=DO_ALIGN(Pike_compiler->new_program->storage_needed-modulo,alignment)+modulo;

  if(!Pike_compiler->new_program->storage_needed) {
    /* Shouldn't Pike_compiler->new_program->storage_needed be set here?
     * Otherwise the debug code below ought to be trigged.
     * But since it isn't, I guess this is dead code?
     *	/grubba 1999-09-28
     *
     * No, the below offset represents the storage in the beginning
     * of obj->storage which is not used because of alignment constraints.
     * However, for historical reasons, prog->storage_offset needs to
     * contain this unused space as well. This means that the real
     * space used by all variables in an object is really:
     * o->prog->storage_needed - o->prog->inherits[0].storage_offset,
     * This can also be written as STORAGE_NEEDED(o->prog)
     * STORAGE_NEEDED() is defined in program.h.
     * /Hubbe 1999-09-29
     *
     * Oops, seems I read the test below the wrong way around.
     *	/grubba 1999-09-29
     */
    Pike_compiler->new_program->inherits[0].storage_offset=offset;
  }

  if(Pike_compiler->new_program->alignment_needed<alignment)
    Pike_compiler->new_program->alignment_needed = (unsigned INT8)alignment;

#ifdef PIKE_DEBUG
  if(offset < Pike_compiler->new_program->storage_needed)
    Pike_fatal("add_storage failed horribly!\n");

  if( (offset /* + OFFSETOF(object,storage) */ - modulo_orig ) % alignment )
    Pike_fatal("add_storage failed horribly(2) %ld %ld %ld %ld!\n",
          (long)offset,
	  (long)0 /* + OFFSETOF(object,storage) */,
          (long)modulo_orig,
          (long)alignment);

#endif

  Pike_compiler->new_program->storage_needed = offset + size;

  return (size_t) offset;
}

/*
 * Internal function.
 * Adds object storage that will *not* be inherited.
 */
static size_t add_xstorage(size_t size,
			   size_t alignment,
			   ptrdiff_t modulo_orig)
{
  ptrdiff_t offset, modulo, available;
  int e;

  if(!size) return Pike_compiler->new_program->xstorage;

  modulo=( modulo_orig /* +OFFSETOF(object,storage) */ ) % alignment;

  offset=DO_ALIGN(Pike_compiler->new_program->xstorage-modulo,alignment)+modulo;

  Pike_compiler->new_program->xstorage = offset + size;

  /* Move all inherits to make room */
  available = Pike_compiler->new_program->inherits[0].storage_offset;
  if(available < (ptrdiff_t)(offset+size))
  {
    available=
      DO_ALIGN( ((offset + size) - available),
		Pike_compiler->new_program->alignment_needed);

    for(e=0;e<Pike_compiler->new_program->num_inherits;e++)
      Pike_compiler->new_program->inherits[e].storage_offset+=available;

    Pike_compiler->new_program->storage_needed+=available;
  }

  return (size_t) offset;
}

typedef void (*oldhandlertype)(struct object *);

static void compat_event_handler(int e)
{
  oldhandlertype handler;
  debug_malloc_touch(Pike_fp->current_object);
  handler=((oldhandlertype *)Pike_fp->context->prog->program)[e];
  if(handler) handler(Pike_fp->current_object);
  debug_malloc_touch(Pike_fp->current_object);
}

static void add_compat_event_handler(void)
{
  if(Pike_compiler->new_program->event_handler != compat_event_handler)
  {
    unsigned int e,d;
    unsigned char *tmp=(unsigned char *)&Pike_compiler->new_program->event_handler;

    for(d=0;d<NUM_PROG_EVENTS;d++) {
      /* FIXME: This looks like it might be broken. */
      /* Broken how? -Hubbe */
      for(e=0;e<sizeof(Pike_compiler->new_program->event_handler);e++)
	add_to_program(tmp[e]);
    }
    Pike_compiler->new_program->event_handler=compat_event_handler;
  }
}

/**
 * Set a callback to be called when this program is cloned.
 *
 * This function is obsolete; see pike_set_prog_event_callback for
 * details.
 */
PMOD_EXPORT void set_init_callback(void (*init)(struct object *))
{
  add_compat_event_handler();
  ((oldhandlertype *)Pike_compiler->new_program->program)[PROG_EVENT_INIT]=init;
}

/**
 * Set a callback to be called when clones of this program are
 * destructed.
 *
 * This function is obsolete; see pike_set_prog_event_callback for
 * details.
 *
 * Note: If the callback only does very trivial stuff, like freeing or
 * clearing a few pointers in the object storage, you can do
 *
 *   Pike_compiler->new_program->flags &= ~PROGRAM_LIVE_OBJ;
 *
 * after the set_exit_callback call. That allows the gc to operate
 * more efficiently, but the effect is that the callback might be
 * called out of order for PROG_EVENT_EXIT events; see the docs for
 * PROGRAM_LIVE_OBJ in program.h for further details.
 */
PMOD_EXPORT void set_exit_callback(void (*exit)(struct object *))
{
  add_compat_event_handler();
  ((oldhandlertype *)Pike_compiler->new_program->program)[PROG_EVENT_EXIT]=exit;
  Pike_compiler->new_program->flags |= PROGRAM_LIVE_OBJ;
}

/**
 * This callback is used by the gc to traverse all references to
 * things in memory. It should call some gc_recurse_* function exactly
 * once for each reference that the pike internals doesn't know about.
 *
 * If a reference is shared between objects, it should be traversed
 * once for every instance sharing it.
 *
 * The callback might be called more than once for the same instance
 * during a gc pass. The gc assumes that the references are enumerated
 * in the same order in that case.
 *
 * The callback is called after any mapped variables on the object
 * have been recursed (and possibly freed).
 *
 * If there are pointers to allocated memory that you keep track of on
 * the C level then you should add something like this to the recurse
 * callback so that Pike.count_memory remains accurate:
 *
 *   if (mc_count_bytes (Pike_fp->current_object))
 *     mc_counted_bytes += <size of the allocated memory block(s)>
 *
 * If the allocated memory is shared between objects then it gets more
 * complicated and you need to write visit_thing_fn callbacks. See
 * e.g. visit_mapping and visit_mapping_data for how to do that.
 *
 * This function is obsolete; see pike_set_prog_event_callback for
 * details.
 */
PMOD_EXPORT void set_gc_recurse_callback(void (*m)(struct object *))
{
  add_compat_event_handler();
  ((oldhandlertype *)Pike_compiler->new_program->program)[PROG_EVENT_GC_RECURSE]=m;
}

/**
 * This callback is used by the gc to count all references to things
 * in memory. It should call gc_check, gc_check_(weak_)svalues or
 * gc_check_(weak_)short_svalue exactly once for each reference that
 * the pike internals don't know about.
 *
 * If a reference is shared between objects, it should be counted once
 * for all shared instances. The return value from gc_check is useful
 * to ensure this; it's zero when called the first time for its
 * argument (it is perfectly fine to use gc_check on things that the
 * pike core doesn't know about, but they must have an INT32 refcount
 * first).
 *
 * This function is obsolete; see pike_set_prog_event_callback for
 * details.
 */
PMOD_EXPORT void set_gc_check_callback(void (*m)(struct object *))
{
  add_compat_event_handler();
  ((oldhandlertype *)Pike_compiler->new_program->program)[PROG_EVENT_GC_CHECK]=m;
}

/**
 * Set a callback to be called when any of the special program events
 * occur. The event type is sent as an integer argument. The events
 * include, but might not be limited to, the following:
 *
 * PROG_EVENT_INIT
 *   An object is being cloned from the program. See set_init_callback
 *   for details.
 * PROG_EVENT_EXIT
 *   An object is being destructed. See set_exit_callback for details.
 * PROG_EVENT_GC_RECURSE
 *   An object is being recursed by the gc. See
 *   set_gc_recurse_callback for details.
 * PROG_EVENT_GC_CHECK
 *   An object is being checked by the gc. See set_gc_check_callback
 *   for details.
 *
 * Note that installing an event callback will set the
 * PROGRAM_LIVE_OBJ flag since the callback might act on the
 * PROG_EVENT_EXIT event. If the callback won't do anything for that
 * event (or if it only does something very trivial for it), you
 * should do
 *
 *   Pike_compiler->new_program->flags &= ~PROGRAM_LIVE_OBJ;
 *
 * afterwards to clear it again. That allows the gc to operate more
 * efficiently, but the effect is that the callback might be called
 * out of order for PROG_EVENT_EXIT events; see the docs for
 * PROGRAM_LIVE_OBJ in program.h for further details.
 */
PMOD_EXPORT void pike_set_prog_event_callback(void (*cb)(int))
{
#ifdef PIKE_DEBUG
  if(Pike_compiler->new_program->event_handler)
    Pike_fatal("Program already has an event handler!\n");
#endif
  Pike_compiler->new_program->event_handler=cb;
  Pike_compiler->new_program->flags |= PROGRAM_LIVE_OBJ;
}

PMOD_EXPORT void pike_set_prog_optimize_callback(node *(*opt)(node *))
{
#ifdef PIKE_DEBUG
  if(Pike_compiler->new_program->optimize)
    Pike_fatal("Program already has an optimize handler!\n");
#endif
  Pike_compiler->new_program->optimize = opt;
}

/**
 * Reference an inherited identifier.
 *
 * @param q
 *   Program state for the program being compiled that will have
 *   the reference added. May be NULL to indicate Pike_compiler.
 *
 * @param i
 *   Inherit number in q->new_program.
 *
 * @param f
 *   Reference number in q->new_program->inherit[i].prog.
 *
 * @return
 *   Returns an equivalent reference that is INLINE|HIDDEN.
 *
 *   Returns -1 if the referenced identifier is -1 or a prototype.
 */
PMOD_EXPORT int really_low_reference_inherited_identifier(struct program_state *q,
							  int i,
							  int f)
{
  struct program *np=(q?q:Pike_compiler)->new_program;
  struct reference funp;
  struct program *p;
  int d, num_id_refs;

  if(f==-1) return -1;

  p = np->inherits[i].prog;

  if ((q?q:Pike_compiler)->compiler_pass == COMPILER_PASS_LAST) {
    struct identifier *id = ID_FROM_INT(p, f);
    if (((id->identifier_flags & IDENTIFIER_TYPE_MASK) ==
	 IDENTIFIER_PIKE_FUNCTION) && (id->func.offset == -1)) {
      /* Prototype. */
      return -1;
    }
  }

  funp = p->identifier_references[f];
  funp.inherit_offset += i;
  funp.id_flags = (funp.id_flags & ~ID_INHERITED) | ID_INLINE|ID_HIDDEN;

  num_id_refs = np->num_identifier_references;

  for(d = 0; d < num_id_refs; d++)
  {
    struct reference *refp;
    refp = np->identifier_references + d;

    if ((refp->inherit_offset == funp.inherit_offset) &&
	(refp->identifier_offset == funp.identifier_offset) &&
	((refp->id_flags | ID_USED) == (funp.id_flags | ID_USED))) {
      return d;
    }
  }

  funp.run_time_type = PIKE_T_UNKNOWN;

  if(q)
    low_add_to_identifier_references(q,funp);
  else
    add_to_identifier_references(funp);
  /* NOTE: np->num_identifier_references has been increased by one by
   *       {low_,}add_to_identifier_references().
   */
#ifdef PIKE_DEBUG
  if (num_id_refs != np->num_identifier_references-1) {
    Pike_fatal("Unexpected number of identifier references: %d != %d\n",
               num_id_refs, np->num_identifier_references-1);
  }
#endif /* PIKE_DEBUG */

  return num_id_refs; /* aka np->num_identifier_references - 1 */
}

PMOD_EXPORT int low_reference_inherited_identifier(struct program_state *q,
						   int e,
                                                   struct pike_string *name,
						   int flags)
{
  struct program *np=(q?q:Pike_compiler)->new_program;
  struct program *p;
  int i;

  p=np->inherits[e].prog;
  i=find_shared_string_identifier(name,p);
  if(i==-1)
  {
    i=really_low_find_shared_string_identifier(name,p, flags);
    if(i==-1) return -1;
  }

  if(p->identifier_references[i].id_flags & ID_HIDDEN)
    return -1;

  if(p->identifier_references[i].id_flags & ID_PRIVATE)
    if(!(flags & SEE_PRIVATE))
      return -1;

  return really_low_reference_inherited_identifier(q, e, i);
}

int find_inherit(const struct program *p, const struct pike_string *name)
{
  int e;
  int level = p->num_inherits;	/* Larger than any inherit_level. */
  int res = 0;

#if 0
  fprintf(stderr, "find_inherit(0x%08lx, \"%s\")...\n",
	  (unsigned long)p, name->str);
#endif /* 0 */

  for(e = p->num_inherits-1; e>0; e--) {
#if 0
    fprintf(stderr, "  %04d: %04d %s\n",
	    e, p->inherits[e].inherit_level,
	    p->inherits[e].name?p->inherits[e].name->str:"NULL");
#endif /* 0 */
    if (p->inherits[e].inherit_level >= level) continue;
    if (name == p->inherits[e].name) {
      res = e;
      level = p->inherits[e].inherit_level;
      if (level == 1) break;
    }
  }
  return res;
}

/**
 * Reference the symbol inherit::name in the lexical context
 * specified by state.
 *
 * @return Returns the reference in state->new_program if found.
 */
PMOD_EXPORT int reference_inherited_identifier(struct program_state *state,
					       struct pike_string *inherit,
					       struct pike_string *name)
{
  int e, id;
  struct program *p;

#ifdef PIKE_DEBUG
  if (name != debug_findstring(name))
    Pike_fatal("reference_inherited_identifier on nonshared string.\n");
#endif

  if (!state) state = Pike_compiler;

  p = state->new_program;

  /* FIXME: This loop could be optimized by advancing by the number
   *        of inherits in the inherit. But in that case the loop
   *        would have to go the other way.
   */
  for (e = p->num_inherits; e--;) {
    if (p->inherits[e].inherit_level != 1) continue;
    if (inherit && (inherit != p->inherits[e].name)) continue;

    id = low_reference_inherited_identifier(state, e, name, SEE_PROTECTED);

    if (id != -1) return id;
  }

  return -1;
}

/* FIXME: This function probably doesn't do what it is intended to do
 *        if the last inherit had inherits of its own. Consider removal.
 */
void rename_last_inherit(struct pike_string *n)
{
  if(Pike_compiler->new_program->inherits[Pike_compiler->new_program->num_inherits].name)
    free_string(Pike_compiler->new_program->inherits[Pike_compiler->new_program->num_inherits].name);
  copy_shared_string(Pike_compiler->new_program->inherits[Pike_compiler->new_program->num_inherits].name,
		     n);
}

#if 0
static int locate_parent_state(struct program_state **state,
			       struct inherit **i,
			       int *parent_identifier,
			       int depth)
{
  int result=1;
  if(depth<=0) return depth;
  while(depth-->0)
  {
    if( (*i)->parent_offset != INHERIT_PARENT)
    {
      int tmp=(*i)->parent_identifier;
      if( (*i)->parent_offset > 0)
      {
	int po=(*i)->parent_offset;
	*parent_identifier = (*state)->parent_identifier;
	*state = (*state)->previous;
	result++;
	fprintf(stderr,"INHERIT: state=state->previous (po=%d)\n",po);

	if(po>1)
	{
	  *i=INHERIT_FROM_INT( (*state)->new_program,
			       *parent_identifier);

	  result+=locate_parent_state(state,i,parent_identifier, po-1);
	}
      }

      if(tmp != -1)
      {
	if( *parent_identifier == -4711)
	{
	  *parent_identifier = tmp;
	}else{
	  *parent_identifier = tmp + INHERIT_FROM_INT( (*state)->new_program,
						      *parent_identifier)->identifier_level;
	}
      }
    }else{
      fprintf(stderr,"INHERIT: Bailout!\n");
      return result+depth+1;
    }
    *i = INHERIT_FROM_INT( (*state)->new_program, *parent_identifier);
  }
  return result;
}


static int find_depth(struct program_state *state,
		      struct inherit *i,
		      int parent_identifier,
		      int depth)
{
#if 0
  int e;
  struct inherit *oi;
  for(e=0;e<=parent_offset;e++) state=state->previous;
  oi=INHERIT_FROM_INT(state->new_program, parent_identifier);
  parent_offset+=i->parent_offset;
#endif

  return locate_parent_state(&state,
			     &i,
			     &parent_identifier,
			     depth);
}
#endif

void lower_inherit(struct program *p,
		   struct object *parent,
		   int parent_identifier,
		   int parent_offset,
		   INT32 flags,
		   struct pike_string *name)
{
  int e;
  ptrdiff_t inherit_offset, storage_offset;
  struct inherit inherit;

#if 0
  fprintf(stderr,"%p low_inherit(pid=%d, parent=%p, parid=%d, "
	  "paroff=%d, flags=0x%x, name=%s);\n",
	  Pike_compiler->new_program,
	  p ? p->id : 0,
	  parent,
	  parent_identifier,
	  parent_offset,
	  flags,
	  name?  name->str : "");
#endif
  CDFPRINTF("th(%ld) %p inherit %p\n",
            (long) th_self(), Pike_compiler->new_program, p);

  if(!p)
  {
    yyerror("Illegal program pointer.");
    return;
  }

  if (Pike_compiler->compiler_pass == COMPILER_PASS_EXTRA) {
    struct program *old_p =
      Pike_compiler->new_program->
      inherits[Pike_compiler->num_inherits+1].prog;
    Pike_compiler->num_inherits += old_p->num_inherits;

    if (old_p != p) {
      yyerror("Got different program for inherit in second pass "
	      "(resolver problem).");
    }
    return;
  }
  if (Pike_compiler->compiler_pass == COMPILER_PASS_LAST) {
    struct program *old_p =
      Pike_compiler->new_program->
      inherits[Pike_compiler->num_inherits+1].prog;
    Pike_compiler->num_inherits += old_p->num_inherits;

    if (old_p != p) {
      yyerror("Got different program for inherit in second pass "
	      "(resolver problem).");
    }

    if (!(p->flags & PROGRAM_FINISHED)) {
      /* Require that the inherited program really is finished in pass
       * 2. Otherwise we might not have all definitions when we
       * fixate our program.
       *
       * FIXME: Maybe this can be relaxed by registering a dependency
       * and delaying compilation here?
       */
      yyerror ("Cannot inherit program in pass 2 "
	       "which is not fully compiled yet.");
      yyerror ("(You probably have a cyclic symbol dependency that the "
	       "compiler cannot handle.)");
    }


    if(p->flags & PROGRAM_NEEDS_PARENT)
    {
      struct program_state *state=Pike_compiler;

      if(!parent && !parent_offset)
      {
	yyerror("Parent pointer lost, cannot inherit!");
	/* We've inherited it anyway, to avoid causing more errors */
      }

#if 0
      /* FIXME: we don't really need to set this flag on ALL
       * previous compilations, but I'm too lazy to figure out
       * exactly how deep down we need to go...
       */
      for(e=0;e<c->compilation_depth;e++,state=state->previous)
	state->new_program->flags |= PROGRAM_USES_PARENT;
#endif
    }

    return;
  }

  if (!(p->flags & (PROGRAM_FINISHED | PROGRAM_PASS_1_DONE))) {
    yyerror ("Cannot inherit program in pass 1 "
	     "which is only a placeholder.");
    yyerror ("(You probably have a cyclic symbol dependency that the "
	     "compiler cannot handle.)");
    return;
  }

  if (p == placeholder_program) {
    yyerror("Trying to inherit placeholder program (resolver problem).");
    return;
  }

  /* Propagate the HAS_C_METHODS, CLEAR_STORAGE and DESTRUCT_IMMEDIATE flags. */
  if (p->flags & (PROGRAM_HAS_C_METHODS|PROGRAM_CLEAR_STORAGE|PROGRAM_DESTRUCT_IMMEDIATE)) {
    Pike_compiler->new_program->flags |=
      (p->flags & (PROGRAM_HAS_C_METHODS|PROGRAM_CLEAR_STORAGE|PROGRAM_DESTRUCT_IMMEDIATE));
  }

 /* parent offset was increased by 42 for above test.. */
  if(parent_offset)
    parent_offset-=42;

  inherit_offset = Pike_compiler->new_program->num_inherits;

  /* alignment magic */
  storage_offset=p->inherits[0].storage_offset % p->alignment_needed;
  storage_offset=low_add_storage(STORAGE_NEEDED(p),
				 p->alignment_needed,
				 storage_offset);

  /* Without this, the inherit becomes skewed */
  storage_offset-=p->inherits[0].storage_offset;

  for(e=0; e<(int)p->num_inherits; e++)
  {
    inherit=p->inherits[e];
    add_ref(inherit.prog);
    inherit.identifier_level += Pike_compiler->new_program->num_identifier_references;
    inherit.storage_offset += storage_offset;
    inherit.inherit_level ++;


    if(!e)
    {
      if(parent)
      {
	if(parent->next == parent)
	{
#if 0
	  struct object *o;
	  inherit.parent_offset=0;
	  for(o=Pike_compiler->fake_object;o!=parent;o=o->parent)
	  {
#ifdef PIKE_DEBUG
	    if(!o) Pike_fatal("low_inherit with odd Pike_compiler->fake_object as parent!\n");
#endif
	    inherit.parent_offset++;
	  }
#else
          struct program_state *state=Pike_compiler;
	  inherit.parent_offset=0;
	  for(;state->fake_object!=parent;state=state->previous)
	  {
#ifdef PIKE_DEBUG
	    if(!state->fake_object)
	      Pike_fatal("low_inherit with odd Pike_compiler->fake_object as parent!\n");
#endif
	    inherit.parent_offset++;
	  }
#endif
	}else{
	  inherit.parent=parent;
	  inherit.parent_identifier=parent_identifier;
	  inherit.parent_offset=INHERIT_PARENT;
	}
      }else{
	inherit.parent_offset=parent_offset;
	inherit.parent_identifier=parent_identifier;
      }
    }else{
      if(!inherit.parent)
      {
	if(parent && parent->next != parent && inherit.parent_offset)
	{
	  struct object *par=parent;
	  int e,pid=parent_identifier;

	  for(e=1;e<inherit.parent_offset;e++)
	  {
	    struct inherit *in;
	    if(!par || !par->prog)
	    {
	      par = NULL;
	      pid = -1;
	      break;
	    }

	    in=INHERIT_FROM_INT(par->prog, pid);
	    switch(in->parent_offset)
	    {
	      default:
	      {
		struct external_variable_context tmp;
		struct inherit *in2=in;
		while(in2->inherit_level >= in->inherit_level) in2--;
		tmp.o=par;
		tmp.inherit=in2;
		tmp.parent_identifier=pid;
		find_external_context(&tmp, in->parent_offset);
		par = tmp.o;
		pid = tmp.parent_identifier;
	      }
	      break;

	      case INHERIT_PARENT:
		pid = in->parent_identifier;
		par = in->parent;
		break;

	      case OBJECT_PARENT:
		/* Ponder: Can we be sure that PROGRAM_USES_PARENT
		 * doesn't get set later? /mast */
		if(par->prog->flags & PROGRAM_USES_PARENT)
		{
		  pid = PARENT_INFO(par)->parent_identifier;
		  par = PARENT_INFO(par)->parent;
		}else{
		  pid = -1;
		  par = NULL;
		}
	    }
	  }

	  inherit.parent=par;
	  inherit.parent_offset=INHERIT_PARENT;
	}
      }
    }
    if(inherit.parent) add_ref(inherit.parent);

    if(name)
    {
      if(e==0)
      {
	copy_shared_string(inherit.name,name);
      }
      else if(inherit.name)
      {
	add_ref(inherit.name);
      }
    }else{
      inherit.name=0;
    }
    add_to_inherits(inherit);
  }

  /* This value is used by encode_value() to reverse the inherit operation. */
  Pike_compiler->new_program->inherits[inherit_offset].identifier_ref_offset =
    Pike_compiler->new_program->num_identifier_references;

  for (e=0; e < (int)p->num_identifier_references; e++)
  {
    struct reference fun;

    fun = p->identifier_references[e]; /* Make a copy */

    fun.inherit_offset += inherit_offset;

    fun.run_time_type = PIKE_T_UNKNOWN; /* Invalidate the vtable cache. */

    if (fun.id_flags & ID_FINAL)
    {
      Pike_compiler->flags |= COMPILATION_CHECK_FINAL;
    }

    if (flags & ID_PUBLIC)
      fun.id_flags &= ~(ID_PRIVATE|ID_PUBLIC);

    if(fun.id_flags & ID_PRIVATE) fun.id_flags|=ID_HIDDEN;

    fun.id_flags |= flags & ~ID_PUBLIC;

    fun.id_flags |= ID_INHERITED;
    add_to_identifier_references(fun);
  }
}

/**
 * Make the program being compiled inherit another program.
 *
 * @param p
 *   Program to inherit.
 *
 * @param parent
 *   Object containing p (if applicable).
 *
 * @param parent_identifier
 *   Identifier reference in parent->prog that is p.
 *
 * @param parent_offset
 *   Lexical scope to parent from the program being compiled
 *   offsetted by 42, or OBJECT_PARENT or INHERIT_PARENT.
 *
 * @param flags
 *   Modifier flags for the inherit.
 *
 * @param name
 *   Optional rename of the inherit.
 */
PMOD_EXPORT void low_inherit(struct program *p,
			     struct object *parent,
			     int parent_identifier,
			     int parent_offset,
			     INT32 flags,
			     struct pike_string *name)
{
  lower_inherit(p, parent, parent_identifier, parent_offset, flags, name);

  /* Don't do this for OBJECT_PARENT or INHERIT_PARENT inherits.
   * They may show up here from decode_value().
   */
  if (parent_offset >= 42) {
    if (p->flags & (PROGRAM_NEEDS_PARENT|PROGRAM_USES_PARENT)) {
      /* We'll need the parent pointer as well... */
      struct program_state *state = Pike_compiler;

      /* parent offset was increased by 42 by the caller... */
      parent_offset -= 42;

      while (state && state->new_program && parent_offset--) {
	state->new_program->flags |= PROGRAM_NEEDS_PARENT|PROGRAM_USES_PARENT;
	state = state->previous;
      }
    }
  }
}

/**
 * Inherit a class from a lexically scoped identifier.
 *
 * @param scope_depth
 *   Scope at which the identifier is expected to be found.
 *
 * @param symbol
 *   Name of symbol to inherit from the specified scope.
 *
 * @param flags
 *   Modifier flags for the inherit.
 *
 * @param failure_severity_level
 *   Severity level to yyreport() failures to perform the inherit.
 *   Typically REPORT_ERROR, but eg REPORT_NOTICE and REPORT_WARNING
 *   are useful if the sybol is expected to sometimes be missing.
 *
 * @returns
 *   Returns NULL on failure and the inherited program on success.
 */
PMOD_EXPORT struct program *lexical_inherit(int scope_depth,
					    struct pike_string *symbol,
					    INT32 flags,
					    int failure_severity_level)
{
  struct program_state *state = Pike_compiler;
  int e;
  int class_fun_num;
  struct program *prog;

  for (e = 0; e < scope_depth; e++) {
    state = state->previous;
    if (!state) {
      my_yyerror("Invalid lexical inherit of symbol %S at depth %d (max_depth: %d).",
		 symbol, scope_depth, e+1);
      return NULL;
    }
  }

  class_fun_num =
    really_low_find_shared_string_identifier(symbol, state->new_program,
					     SEE_PROTECTED|SEE_PRIVATE);
  if (class_fun_num < 0) {
    yyreport(failure_severity_level, parser_system_string, 0,
	     "Symbol to inherit (%S) not found in parent scope #%d.",
	     symbol, scope_depth);
    return NULL;
  }

  prog = low_program_from_function(state->fake_object, class_fun_num);
  if (!prog) {
    yyreport(failure_severity_level, parser_system_string, 0,
	     "Symbol %S in parent scope #%d is not a program.",
	     symbol, scope_depth);
    return NULL;
  }

  if (scope_depth) {
    /* Add a reference to the identifier. */
    class_fun_num =
      really_low_reference_inherited_identifier(state, 0, class_fun_num);
  }
  low_inherit(prog, 0, class_fun_num, 42 + scope_depth, flags, symbol);

  return prog;
}

PMOD_EXPORT void do_inherit(struct svalue *s,
			    INT32 flags,
			    struct pike_string *name)
{
  struct object *parent_obj = NULL;
  int parent_id = -1;
  struct program *p = low_program_from_svalue(s, &parent_obj, &parent_id);
  low_inherit(p, parent_obj, parent_id, 0, flags, name);
}

void compiler_do_inherit(node *n,
			 INT32 flags,
			 struct pike_string *name)
{
  struct program *p;
  struct identifier *i;
  INT32 numid=-1, offset=0;

  if(!n)
  {
    yyerror("Unable to inherit");
    return;
  }

  if ((n->token == F_APPLY) && (CAR(n)->token == F_CONSTANT) &&
      (TYPEOF(CAR(n)->u.sval) == T_FUNCTION) &&
      (SUBTYPEOF(CAR(n)->u.sval) == FUNCTION_BUILTIN) &&
      (CAR(n)->u.sval.u.efun->function == debug_f_aggregate)) {
    /* Disambiguate multiple inherit ::-reference. */
    node *arg;
    while(1) {
      while ((arg = CDR(n))) {
	n = arg;
	if (n->token != F_ARG_LIST) goto found;
      }
      /* Paranoia. */
      if ((arg = CAR(n))) {
	n = arg;
	continue;
      }
      /* FIXME: Ought to go up a level and try the car there...
       *        But as this code probably won't be reached, we
       *        just fail.
       */
      yyerror("Unable to inherit");
      return;
    }
  found:
    /* NB: The traditional C grammar requires a statement after a label. */
    ;
  }

  fix_type_field(n);

  if (!pike_types_le(n->type, inheritable_type_string) &&
      (THIS_COMPILATION->lex.pragmas & ID_STRICT_TYPES)) {
    yytype_report(REPORT_WARNING,
		  n->current_file, n->line_number, inheritable_type_string,
		  n->current_file, n->line_number, n->type,
		  0, "Program required for inherit.\n");
  }

  switch(n->token)
  {
    case F_EXTERNAL:
      {
	struct program_state *state = Pike_compiler;

	offset = 0;
	while (state && (state->new_program->id != n->u.integer.a)) {
	  state = state->previous;
	  offset++;
	}
	if (!state) {
	  yyerror("Failed to resolv external constant.\n");
	  return;
	}
	p = state->new_program;
	numid = n->u.integer.b;
	if ((name == this_program_string) && (offset == 1)) {
	  /* Klugde: Default to renaming ::this_program
	   *         to the name of the current class.
	   *
	   *         Otherwise the this_program:-scope
	   *         will become confusing, as it will
	   *         refer to the inherit and not the
	   *         current class.
	   */
	  name = ID_FROM_INT(p, numid)->name;
	}
      }

  continue_inherit:

      /* FIXME: Support external constants. */
      if(numid != IDREF_MAGIC_THIS &&
	 (IDENTIFIER_IS_CONSTANT((i=ID_FROM_INT(p, numid))->
				 identifier_flags)) &&
	 (i->func.const_info.offset != -1))
      {
	struct svalue *s=&PROG_FROM_INT(p, numid)->
	  constants[i->func.const_info.offset].sval;
	if(TYPEOF(*s) != T_PROGRAM)
	{
	  do_inherit(s,flags,name);
	  return;
	}else{
	  low_inherit(s->u.program,
		      0,
		      numid,
		      offset+42,
		      flags,
		      name);
	}
      }else{
	yyerror("Inherit identifier is not a constant program");
	return;
      }
      break;

    default:
      resolv_class(n);
      do_inherit(Pike_sp-1, flags, name);
      pop_stack();
  }
}

void compiler_do_implement(node *n)
{
  if (!n) {
    yyerror("Invalid implement directive.");
    return;
  }
  /* FIXME: Implement. */
}

int call_handle_inherit(struct pike_string *s)
{
  struct compilation *c = THIS_COMPILATION;

  CHECK_COMPILER();

  ref_push_string(s);
  f_string_to_utf8(1);

  if (safe_apply_current2(PC_HANDLE_INHERIT_FUN_NUM, 1, NULL))
    if (TYPEOF(Pike_sp[-1]) != T_INT)
      return 1;
    else {
      my_yyerror("Couldn't find program %S", s);
    }
  else {
    handle_compile_exception ("Error finding program");
  }
  pop_stack();

  return 0;
}

void simple_do_inherit(struct pike_string *s,
		       INT32 flags,
		       struct pike_string *name)
{
  if (!call_handle_inherit(s)) return;

  if(name)
  {
    free_string(s);
    s=name;
  }
  do_inherit(Pike_sp-1, flags, s);
  free_string(s);
  pop_stack();
}

/**
 * Find an identifier relative to the program being compiled.
 *
 * @return Return the index of the identifier found, otherwise -1.
 */
int isidentifier(const struct pike_string *s)
{
  return really_low_find_shared_string_identifier(s,
						  Pike_compiler->new_program,
						  SEE_PROTECTED|SEE_PRIVATE);
}

/*
 * Definition of identifiers.
 *
 * Pike has three plus one classes of identifiers:
 *
 *   IDENTIFIER_VARIABLE - A value stored in object->storage
 *   IDENTIFIER_CONSTANT - A value stored in program->constants
 *   IDENTIFIER_FUNCTION - Either a C function or a Pike function
 * and
 *   IDENTIFIER_ALIAS    - An alias for a different identifier.
 *
 */


/**
 * Define an alias for a (possibly extern) identifier.
 *
 * Note that both type and name may be NULL. If they are NULL
 * they will be defaulted to the values from the aliased identifier.
 */
int low_define_alias(struct pike_string *name, struct pike_type *type,
		     int flags, int depth, int refno)
{
  int n;
  int e;

  struct compilation *c = THIS_COMPILATION;
  struct program_state *state = Pike_compiler;
  struct identifier *id;
  union idptr func;

#ifdef PIKE_DEBUG
  if(Pike_compiler->new_program->flags & (PROGRAM_FIXED | PROGRAM_OPTIMIZED))
    Pike_fatal("Attempting to add variable to fixed program\n");

  if(Pike_compiler->compiler_pass == COMPILER_PASS_LAST)
    Pike_fatal("Internal error: Not allowed to add more identifiers during second compiler pass.\n"
	  "Added identifier: \"%s\"\n", name->str);
#endif

  for(e = 0; DO_IF_DEBUG_ELSE(state, 1) && (e < depth); e++) {
    state = state->previous;
  }

#ifdef PIKE_DEBUG
  if (!state) {
    Pike_fatal("Internal error: External symbol buried too deep.\n");
  }
  if (state->new_program->num_identifier_references <= refno) {
    Pike_fatal("Internal error: Reference out of bounds: %d (max: %d).\n",
	       refno, state->new_program->num_identifier_references);
  }
#endif

  id = ID_FROM_INT(state->new_program, refno);

  func.ext_ref.depth = depth;
  func.ext_ref.id = refno;

  if (flags & ID_PRIVATE) flags |= ID_INLINE;

  return add_identifier(c, type ? type : id->type, name ? name : id->name,
			flags, id->identifier_flags | IDENTIFIER_ALIAS, 0,
			func, id->run_time_type);
}

PMOD_EXPORT int define_alias(struct pike_string *name, struct pike_type *type,
			     int flags, int depth, int refno)
{
  /* FIXME: Support NULL name and type. */
  int n = isidentifier(name);

  if(Pike_compiler->new_program->flags & PROGRAM_PASS_1_DONE)
  {
    if(n==-1)
      yyerror("Pass2: Alias disappeared!");
    else {
      struct identifier *id = ID_FROM_INT(Pike_compiler->new_program, n);
      if (!IDENTIFIER_IS_ALIAS(id->identifier_flags)) {
	if (IDENTIFIER_IS_CONSTANT(id->identifier_flags)) {
	  /* Convert a placeholder constant into an alias. */
	  struct program_state *state = Pike_compiler;
	  int e;

	  for(e = 0; state && (e < depth); e++) {
	    state = state->previous;
	  }

#ifdef PIKE_DEBUG
	  if (!state) {
	    Pike_fatal("Internal error: External symbol buried too deep.\n");
	  }
	  if (state->new_program->num_identifier_references <= refno) {
	    Pike_fatal("Internal error: Reference out of bounds: %d (max: %d).\n",
		       refno, state->new_program->num_identifier_references);
	  }
#endif

	  id->identifier_flags = IDENTIFIER_ALIAS |
	    ID_FROM_INT(state->new_program, refno)->identifier_flags;
	} else {
	  Pike_fatal("Replacing non alias with an alias in second pass!\n");
	}
      }

      free_type(id->type);
      copy_pike_type(id->type, type);
      id->func.ext_ref.depth = depth;
      id->func.ext_ref.id = refno;
      return n;
    }
  }

#ifdef PIKE_DEBUG
  if(Pike_compiler->new_program->flags & (PROGRAM_FIXED | PROGRAM_OPTIMIZED))
    Pike_fatal("Attempting to add variable to fixed program\n");
#endif

  if(n != -1)
  {
    /* not inherited */
    if(Pike_compiler->new_program->identifier_references[n].inherit_offset == 0)
    {
      if (!((IDENTIFIERP(n)->id_flags | flags) & ID_EXTERN)) {
	my_yyerror("Identifier %S defined twice.",name);
	return n;
      }
      if (flags & ID_EXTERN) {
	/* FIXME: Check type */
	return n;
      }
    }

    if (!(IDENTIFIERP(n)->id_flags & ID_EXTERN)) {
      if (IDENTIFIERP(n)->id_flags & ID_FINAL)
	my_yyerror("Illegal to redefine 'final' "
		   "variable/functions %S", name);

      /* FIXME: More. */
    }
  }

  return low_define_alias(name, type, flags, depth, refno);
}

/* argument must be a shared string */
int low_define_variable(struct pike_string *name,
			struct pike_type *type,
			INT32 flags,
			size_t offset,
			INT32 run_time_type)
{
  struct compilation *c = THIS_COMPILATION;
  unsigned int identifier_flags = IDENTIFIER_VARIABLE;
  union idptr func;

#ifdef PIKE_DEBUG
  if(Pike_compiler->new_program->flags & (PROGRAM_FIXED | PROGRAM_OPTIMIZED))
    Pike_fatal("Attempting to add variable to fixed program\n");

  if(Pike_compiler->compiler_pass == COMPILER_PASS_LAST)
    Pike_fatal("Internal error: Not allowed to add more identifiers during second compiler pass.\n"
	  "Added identifier: \"%s\"\n", name->str);
#endif

  func.offset = offset - Pike_compiler->new_program->inherits[0].storage_offset;
  if (run_time_type == PIKE_T_FREE) func.offset = -1;

  if (run_time_type & PIKE_T_NO_REF_FLAG) {
    run_time_type &= ~PIKE_T_NO_REF_FLAG;
    identifier_flags |= IDENTIFIER_NO_THIS_REF;
  }

  if (flags & ID_PRIVATE) flags |= ID_LOCAL|ID_PROTECTED;

  return
    add_identifier(c, type, name,
		   flags, identifier_flags, 0,
		   func,
		   run_time_type);
}

/* type is a serialized tokenized type. */
PMOD_EXPORT int quick_map_variable(const char *name,
		       int name_length,
		       size_t offset,
		       const char *type,
		       int UNUSED(type_length),
		       INT32 run_time_type,
		       INT32 flags)
{
  int ret;
  struct pike_string *n;
  struct pike_type *t;

  n = make_shared_binary_string(name, name_length);
  t = make_pike_type(type);

#ifdef PROGRAM_BUILD_DEBUG
  {
    struct compilation *c = THIS_COMPILATION;
    struct pike_string *d = describe_type (t);
    fprintf (stderr, "%*sdefining variable (pass=%d): %s ",
	     c->compilation_depth, "", Pike_compiler->compiler_pass, d->str);
    free_string (d);
    push_string (n);
    print_svalue (stderr, --Pike_sp);
    putc ('\n', stderr);
  }
#endif

  ret=low_define_variable(n,t,flags|ID_USED,offset,run_time_type);
  free_string(n);
  free_type(t);
  return ret;
}

int is_auto_variable_type( int variable )
{
    struct identifier *id=ID_FROM_INT(Pike_compiler->new_program,variable);
    if( id && id->type && id->type->type == PIKE_T_AUTO )
        return 1;
    return 0;
}

void fix_auto_variable_type( int variable, struct pike_type *type )
{
    /* Update the type of a program variable (basically, if it's
     * marked 'auto', set it to the given type)
     */
    struct identifier *id=ID_FROM_INT(Pike_compiler->new_program,variable);
    if( id )
    {
      free_type(id->type);
      copy_pike_type(id->type, type);
    }
    return;
}

/* argument must be a shared string */
int define_variable(struct pike_string *name,
		    struct pike_type *type,
		    INT32 flags)
{
  int n, run_time_type;
  int no_this = 0;
#ifdef PIKE_DEBUG
  if(name != debug_findstring(name))
    Pike_fatal("define_variable on nonshared string.\n");
#endif

#ifdef PROGRAM_BUILD_DEBUG
  {
    struct compilation *c = THIS_COMPILATION;
    struct pike_string *d = describe_type (type);
    fprintf (stderr, "%*sdefining variable (pass=%d): %s ",
	     c->compilation_depth, "", Pike_compiler->compiler_pass, d->str);
    free_string (d);
    push_string (name);
    print_svalue (stderr, --Pike_sp);
    putc ('\n', stderr);
  }
#endif

  if(type == void_type_string)
    yyerror("Variables can't be of type void.");

  n = isidentifier(name);

  if(Pike_compiler->new_program->flags & PROGRAM_PASS_1_DONE)
  {
    if(n==-1)
      yyerror("Pass2: Variable disappeared!");
    else {
      struct identifier *id=ID_FROM_INT(Pike_compiler->new_program,n);
      free_type(id->type);
      copy_pike_type(id->type, type);
      return n;
    }
  }

#ifdef PIKE_DEBUG
  if(Pike_compiler->new_program->flags & (PROGRAM_FIXED | PROGRAM_OPTIMIZED))
    Pike_fatal("Attempting to add variable to fixed program\n");
#endif

  if(n != -1)
  {
    /* not inherited */
    if(Pike_compiler->new_program->identifier_references[n].inherit_offset == 0)
    {
      if (!((IDENTIFIERP(n)->id_flags | flags) & ID_EXTERN)) {
	my_yyerror("Identifier %S defined twice.",name);
	return n;
      }
      if (flags & ID_EXTERN) {
	/* FIXME: Check type */
	return n;
      }
    }

    if (!(IDENTIFIERP(n)->id_flags & ID_EXTERN)) {
      if (IDENTIFIERP(n)->id_flags & ID_FINAL)
	my_yyerror("Illegal to redefine 'final' "
		   "variable/functions %S", name);

      if(!(IDENTIFIERP(n)->id_flags & ID_INLINE) ||
	 Pike_compiler->compiler_pass != COMPILER_PASS_FIRST)
      {
	int n2;

 	if(ID_FROM_INT(Pike_compiler->new_program, n)->type != type &&
	   !pike_types_le(type,
			  ID_FROM_INT(Pike_compiler->new_program, n)->type)) {
	  int level = REPORT_WARNING;
	  if (!match_types(ID_FROM_INT(Pike_compiler->new_program, n)->type,
			   type)) {
	    level = REPORT_ERROR;
	  }
	  yytype_report(level, NULL, 0,
			ID_FROM_INT(Pike_compiler->new_program, n)->type,
			NULL, 0, type, 0,
			"Illegal to redefine inherited variable %S "
			"with different type.", name);
	}


	if(!IDENTIFIER_IS_VARIABLE(ID_FROM_INT(Pike_compiler->new_program, n)->
				   identifier_flags))
	{
	  my_yyerror("Illegal to redefine inherited symbol %S "
		     "as a variable.", name);
	  return n;
	}

	if ((ID_FROM_INT(Pike_compiler->new_program, n)->run_time_type !=
	     PIKE_T_MIXED) &&
	    (ID_FROM_INT(Pike_compiler->new_program, n)->run_time_type !=
	     compile_type_to_runtime_type(type))) {
	  yytype_report(REPORT_ERROR, NULL, 0,
			ID_FROM_INT(Pike_compiler->new_program, n)->type,
			NULL, 0, type, 0,
			"Illegal to redefine inherited variable %S "
			"with different type.", name);
	  return n;
	}

	/* Create an alias for the old variable reference, so that we
	 * can change the compile-time type. */
	n2 = define_alias(name, type, flags & ~ID_EXTERN, 0, n);

	/* Hide the old variable. */
	Pike_compiler->new_program->identifier_references[n].id_flags |=
	  ID_HIDDEN;
	return n2;
      } else if ((IDENTIFIERP(n)->id_flags & (ID_INLINE|ID_INHERITED)) ==
		 (ID_INLINE|ID_INHERITED)) {
	/* Hide the overloaded inherited symbol. */
	IDENTIFIERP(n)->id_flags |= ID_HIDDEN;
      }
    } else if ((IDENTIFIERP(n)->id_flags & (ID_EXTERN|ID_INHERITED)) ==
	       (ID_EXTERN|ID_INHERITED)) {
      /* Hide the overloaded inherited symbol. */
      IDENTIFIERP(n)->id_flags |= ID_PROTECTED|ID_PRIVATE|ID_USED;
    }
  }

  if (flags & ID_EXTERN) {
    run_time_type = PIKE_T_FREE;
  } else {
    run_time_type=compile_type_to_runtime_type(type);

    /* FIXME: Shouldn't these special cases be
     *        in compile_type_to_runtime_type()?
     */
    switch(run_time_type)
    {
    case T_OBJECT:
      /* Make place for the object subtype. */
    case T_MIXED:
    case T_FUNCTION:
      no_this = 1;
      /* FALLTHRU */
    case T_PROGRAM:
      run_time_type = T_MIXED;
      break;
    case T_INT:
      {
	INT_TYPE int_range[] = { MAX_INT32, MIN_INT32 };
	run_time_type = T_MIXED;
	if (get_int_type_range(type, int_range) &&
	    (int_range[0] > MIN_INT32) && (int_range[1] < MAX_INT32) ) {
	  run_time_type = T_INT;
	}
      }
    }
  }

  n=low_define_variable(name,type,flags,
			low_add_storage(sizeof_variable(run_time_type),
					alignof_variable(run_time_type),0),
			run_time_type);

  if( no_this )
    ID_FROM_INT(Pike_compiler->new_program, n)->identifier_flags |= IDENTIFIER_NO_THIS_REF;

  return n;
}

PMOD_EXPORT int add_constant(struct pike_string *name,
			     const struct svalue *c,
			     INT32 flags)
{
  int n;
  struct compilation *cc = THIS_COMPILATION;
  struct identifier dummy;
  struct reference ref;
  struct pike_type *type;
  unsigned int opt_flags;
  union idptr func;

#ifdef PROGRAM_BUILD_DEBUG
  {
    if (c) {
      struct pike_type *t = get_type_of_svalue(c);
      struct pike_string *d = describe_type (t);
      fprintf (stderr, "%*sdefining constant (pass=%d): %s ",
	       cc->compilation_depth, "",
	       Pike_compiler->compiler_pass, d->str);
      free_type(t);
      free_string (d);
      push_string (name);
      print_svalue (stderr, --Pike_sp);
      fputs (" = ", stderr);
      print_svalue (stderr, c);
    }
    else {
      fprintf (stderr, "%*sdeclaring constant (pass=%d): ",
	       cc->compilation_depth, "",
	       Pike_compiler->compiler_pass);
      push_string (name);
      print_svalue (stderr, --Pike_sp);
    }
    putc ('\n', stderr);
  }
#endif

#if 0
  if (!c) {
    c = &svalue_int_zero;
  }
#endif

#ifdef PIKE_DEBUG
  if(name != debug_findstring(name))
    Pike_fatal("define_constant on nonshared string.\n");
  if (c) {
    check_svalue ((struct svalue*)c);
    if (TYPEOF(*c) > MAX_TYPE)
      /* check_svalue allows some things like T_SVALUE_PTR. */
      Pike_fatal ("Invalid type in svalue: %d\n", TYPEOF(*c));
  }
#endif

  n = isidentifier(name);

  if(
#if 1
    c &&
#endif
    TYPEOF(*c) == T_FUNCTION &&
    SUBTYPEOF(*c) != FUNCTION_BUILTIN &&
    c->u.object->prog)
  {
    struct program_state *state = Pike_compiler;
    struct reference *idref = PTR_FROM_INT(c->u.object->prog, SUBTYPEOF(*c));
    struct program *p = PROG_FROM_PTR(c->u.object->prog, idref);
    struct identifier *id = p->identifiers + idref->identifier_offset;
    int depth = 0;
    while (state && (c->u.object->prog != state->new_program)) {
      depth++;
      state = state->previous;
    }
    if(state) {
      /* Alias for a symbol in the current or surrounding programs.
       */
      if(IDENTIFIER_IS_CONSTANT(id->identifier_flags) &&
	 (id->func.const_info.offset != -1) &&
	 (state == Pike_compiler)) {
	c=& p->constants[id->func.const_info.offset].sval;
      } else if (IDENTIFIER_IS_VARIABLE(id->identifier_flags) &&
		 (state == Pike_compiler)) {
	my_yyerror("Attempt to make a constant %S of a variable.",
		   name);
	c = NULL;
      } else {
	/* Alias for a function or a variable or constant in a surrounding
	 * scope.
	 */
	int n = SUBTYPEOF(*c);
	struct reference *remote_ref = PTR_FROM_INT(state->new_program, n);
	if (!(remote_ref->id_flags & (ID_INLINE|ID_HIDDEN))) {
	  /* We need to get a suitable reference. */
	  n = really_low_reference_inherited_identifier(state, 0, n);
	  if (n < 0) return n;
	}
	return define_alias(name, id->type, flags, depth, n);
      }
    }
  }

  if(
#if 1
    c &&
#endif
    !svalues_are_constant(c,1,BIT_MIXED,0))
    my_yyerror("Constant value %S has a reference to this.", name);

  if(Pike_compiler->new_program->flags & PROGRAM_PASS_1_DONE)
  {
    if(n==-1
#if 1
       || !c
#endif
       )
    {
      yyerror("Pass2: Constant disappeared!");
    }else{
      struct identifier *id;
      id=ID_FROM_INT(Pike_compiler->new_program,n);
      if (IDENTIFIER_IS_ALIAS(id->identifier_flags)) {
	/* FIXME: We probably ought to do something here... */
      } else if(id->func.const_info.offset>=0) {
	/* Update the stored constant. */
	assign_svalue (&PROG_FROM_INT(Pike_compiler->new_program,n)->
		       constants[id->func.const_info.offset].sval, c);
      } else {
	id->run_time_type = (unsigned char) TYPEOF(*c);
	id->func.const_info.offset = store_constant(c, 0, 0);
      }
      free_type(id->type);
      if( !(flags & ID_INLINE) )
          id->type = get_lax_type_of_svalue( c );
      else
          id->type = get_type_of_svalue( c );
#ifdef PROGRAM_BUILD_DEBUG
      fprintf (stderr, "%*sstored constant #%d at %d\n",
	       cc->compilation_depth, "",
	       n, id->func.const_info.offset);
#endif
    }
    return n;
  }

#ifdef PIKE_DEBUG
  if(Pike_compiler->new_program->flags & (PROGRAM_FIXED | PROGRAM_OPTIMIZED))
    Pike_fatal("Attempting to add constant to fixed program\n");

  if(Pike_compiler->compiler_pass == COMPILER_PASS_LAST) {
    dump_program_tables(Pike_compiler->new_program, 2);
    Pike_fatal("Internal error: Not allowed to add more identifiers during second compiler pass.\n"
	       "                Attempted to add the identifier \"%s\"\n",
	       name->str);
  }
#endif

#if 1
  if (c) {
#endif
   if( !(flags & ID_INLINE) )
      type = get_lax_type_of_svalue( c );
    else
      type = get_type_of_svalue( c );
    func.const_info.offset = store_constant(c, 0, 0);
    opt_flags = OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND;
    if(TYPEOF(*c) == PIKE_T_PROGRAM && (c->u.program->flags & PROGRAM_CONSTANT))
       opt_flags = 0;
#if 1
  }
  else {
    copy_pike_type(type, mixed_type_string);
    func.const_info.offset = -1;
    opt_flags = 0;
  }
#endif

  if (flags & ID_PRIVATE) flags |= ID_LOCAL|ID_PROTECTED;

  ref.id_flags=flags;
  ref.identifier_offset =
    low_add_identifier(cc, type, name,
		       IDENTIFIER_CONSTANT, opt_flags,
		       func, c ? TYPEOF(*c) : T_MIXED);
  ref.inherit_offset=0;
  ref.run_time_type = PIKE_T_UNKNOWN;

  free_pike_type(type);

  if(n != -1)
  {
    int overridden;

    if(IDENTIFIERP(n)->id_flags & ID_FINAL)
      my_yyerror("Illegal to redefine 'final' identifier %S", name);

    if(IDENTIFIER_IS_VARIABLE(ID_FROM_INT(Pike_compiler->new_program,
					  n)->identifier_flags))
    {
      my_yyerror("Illegal to redefine variable %S as constant.", name);
    }

    /* not inherited */
    if(Pike_compiler->new_program->identifier_references[n].inherit_offset == 0)
    {
      my_yyerror("Identifier %S defined twice.", name);
      return n;
    }

    /* override */
    if ((overridden = override_identifier (&ref, name, 0)) >= 0) {
#ifdef PIKE_DEBUG
      struct reference *oref =
	Pike_compiler->new_program->identifier_references+overridden;
      if((oref->inherit_offset != ref.inherit_offset) ||
	 (oref->identifier_offset != ref.identifier_offset) ||
	 ((oref->id_flags | ID_USED) != (ref.id_flags | ID_USED))) {
	Pike_fatal("New constant overriding algorithm failed!\n");
      }
#endif
      return overridden;
    }
  }
  n=Pike_compiler->new_program->num_identifier_references;
  add_to_identifier_references(ref);

  return n;
}

PMOD_EXPORT int simple_add_constant(const char *name,
			struct svalue *c,
			INT32 flags)
{
  INT32 ret;
  struct pike_string *id;
  id=make_shared_string(name);
  ret=add_constant(id, c, flags);
  free_string(id);
  return ret;
}

PMOD_EXPORT int add_integer_constant(const char *name,
				     INT_ARG_TYPE i,
				     INT32 flags)
{
  struct svalue tmp;
  SET_SVAL(tmp, T_INT, NUMBER_NUMBER, integer, i);
  return simple_add_constant(name, &tmp, flags);
}

PMOD_EXPORT int low_add_integer_constant(struct pike_string *name,
				     INT_ARG_TYPE i,
				     INT32 flags)
{
  struct svalue tmp;
  SET_SVAL(tmp, T_INT, NUMBER_NUMBER, integer, i);
  return add_constant(name, &tmp, flags);
}

PMOD_EXPORT int quick_add_integer_constant(const char *name,
					   int name_length,
					   INT_ARG_TYPE i,
					   INT32 flags)
{
  INT32 ret;
  struct pike_string *n = make_shared_binary_string(name, name_length);
  ret=low_add_integer_constant(n, i, flags);
  free_string(n);
  return ret;
}

PMOD_EXPORT int add_float_constant(const char *name,
				   FLOAT_ARG_TYPE f,
				   INT32 flags)
{
  struct svalue tmp;
  SET_SVAL(tmp, T_FLOAT, 0, float_number, (FLOAT_TYPE)f);
  return simple_add_constant(name, &tmp, flags);
}

PMOD_EXPORT int quick_add_float_constant(const char *name,
					 int name_length,
					 FLOAT_ARG_TYPE f,
					 INT32 flags)
{
  struct svalue tmp;
  struct pike_string *id;
  INT32 ret;

  SET_SVAL(tmp, T_FLOAT, 0, float_number, (FLOAT_TYPE)f);
  id=make_shared_binary_string(name,name_length);
  ret=add_constant(id, &tmp, flags);
  free_string(id);
  return ret;
}

PMOD_EXPORT int add_string_constant(const char *name,
				    const char *str,
				    INT32 flags)
{
  INT32 ret;
  struct svalue tmp;
  SET_SVAL(tmp, T_STRING, 0, string, make_shared_string(str));
  ret=simple_add_constant(name, &tmp, flags);
  free_svalue(&tmp);
  return ret;
}

PMOD_EXPORT int add_program_constant(const char *name,
				     struct program *p,
				     INT32 flags)
{
  INT32 ret;
  struct svalue tmp;
  if (p) {
    SET_SVAL(tmp, T_PROGRAM, 0, program, p);
  } else {
    /* Probable compilation error in a C-module. */
    SET_SVAL(tmp, T_INT, NUMBER_UNDEFINED, integer, 0);
    my_yyerror("Program constant \"%s\" is NULL.", name);
  }
  if (!Pike_compiler->new_program) return -1;
  ret=simple_add_constant(name, &tmp, flags);
  return ret;
}

PMOD_EXPORT int add_object_constant(const char *name,
			struct object *o,
			INT32 flags)
{
  INT32 ret;
  struct svalue tmp;
  SET_SVAL(tmp, T_OBJECT, 0, object, o);
  ret=simple_add_constant(name, &tmp, flags);
  return ret;
}

PMOD_EXPORT int add_function_constant(const char *name, void (*cfun)(INT32),
				      const char * type, int flags)
{
  struct svalue s;
  INT32 ret;

  SET_SVAL(s, T_FUNCTION, FUNCTION_BUILTIN, efun,
	   make_callable(cfun, name, type, flags, 0, 0));
  ret=simple_add_constant(name, &s, 0);
  free_svalue(&s);
  return ret;
}


PMOD_EXPORT int debug_end_class(const char *name, ptrdiff_t namelen, INT32 flags)
{
  INT32 ret;
  struct svalue tmp;
  struct pike_string *id;

  SET_SVAL(tmp, T_PROGRAM, 0, program, end_program());
  if(!tmp.u.program)
    Pike_fatal("Failed to initialize class '%s'\n",name);

  id=make_shared_binary_string(name,namelen);
  ret=add_constant(id, &tmp, flags);
  /* The following is not really true, but it helps encode_value()... */
  Pike_compiler->new_program->flags |=
    tmp.u.program->flags & PROGRAM_HAS_C_METHODS;
  free_string(id);
  free_svalue(&tmp);
  return ret;
}

/**
 * Define a new function.
 *
 * if func isn't given, it is supposed to be a prototype.
 */
INT32 define_function(struct pike_string *name,
		      struct pike_type *type,
		      unsigned flags,
		      unsigned function_flags,
		      union idptr *func,
		      unsigned opt_flags)
{
  struct compilation *c = THIS_COMPILATION;
  struct program *prog = Pike_compiler->new_program;
  struct identifier *funp;
  union idptr idptr;
  struct reference ref;
  struct svalue *lfun_type;
  int run_time_type = T_FUNCTION;
  INT32 i;
  int getter_setter = -1;
  int is_setter = 0;

  CHECK_COMPILER();

#ifdef PROGRAM_BUILD_DEBUG
  {
    struct pike_string *d = describe_type (type);
    fprintf (stderr, "%*sdefining function (pass=%d): %s ",
	     c->compilation_depth, "", Pike_compiler->compiler_pass, d->str);
    free_string (d);
    push_string (name);
    print_svalue (stderr, --Pike_sp);
    putc ('\n', stderr);
  }
#endif

#ifdef PROFILING
  fun.self_time=0;
  fun.num_calls=0;
  fun.recur_depth=0;
  fun.total_time=0;
#endif

  /* If this is an lfun, match against the predefined type. */
  if ((lfun_type = low_mapping_string_lookup(lfun_types, name))) {
    int orig_pragmas = c->lex.pragmas;
#ifdef PIKE_DEBUG
    if (TYPEOF(*lfun_type) != T_TYPE) {
      Pike_fatal("Bad entry in lfun_types for key \"%s\"\n", name->str);
    }
#endif /* PIKE_DEBUG */
    if (Pike_compiler->compiler_pass == COMPILER_PASS_LAST) {
      /* Inhibit deprecation warnings during the comparison. */
      c->lex.pragmas |= ID_NO_DEPRECATION_WARNINGS;
      if (!pike_types_le(type, lfun_type->u.type)) {
	int level = REPORT_NOTICE;
	if (!match_types(type, lfun_type->u.type)) {
	  level = REPORT_ERROR;
	} else if (c->lex.pragmas & ID_STRICT_TYPES) {
	  level = REPORT_WARNING;
	}
	if (level != REPORT_NOTICE) {
	  yytype_report(level, NULL, 0, lfun_type->u.type,
			NULL, 0, type, 0,
			"Type mismatch for callback function %S:", name);
	}
      }
      c->lex.pragmas = orig_pragmas;
    }
  } else if (((name->len > 3) &&
	      (index_shared_string(name, 0) == '`') &&
	      (index_shared_string(name, 1) == '-') &&
	      (index_shared_string(name, 2) == '>')) ||
	     ((name->len > 1) &&
              (index_shared_string(name, 0) == '`') &&
              wide_isidchar(index_shared_string(name, 1)))) {
    /* Getter setter. */
    struct pike_string *symbol = NULL;
    struct pike_type *symbol_type = NULL;
    struct pike_type *gs_type = NULL;
    int delta = 1;	/* new-style */
    if (index_shared_string(name, 1) == '-') {
      /* Getter setter (old-style). */
      delta = 3;
    }
    if (index_shared_string(name, name->len-1) != '=') {
      /* fprintf(stderr, "Got getter: %s\n", name->str); */
      gs_type = lfun_getter_type_string;
      symbol = string_slice(name, delta, name->len-delta);
      symbol_type = get_argument_type(type, -1);
    } else if (name->len > delta+1) {
      /* fprintf(stderr, "Got setter: %s\n", name->str); */
      gs_type = lfun_setter_type_string;
      is_setter = 1;
      symbol = string_slice(name, delta, name->len-(delta+1));
      symbol_type = get_argument_type(type, 0);
    }

    if (symbol) {
      /* We got a getter or a setter. */
      struct reference *ref;
      int orig_pragmas = c->lex.pragmas;
      /* Inhibit deprecation warnings during the comparison. */
      c->lex.pragmas |= ID_NO_DEPRECATION_WARNINGS;
      if (!pike_types_le(type, gs_type)) {
	int level = REPORT_NOTICE;
	if (!match_types(type, gs_type)) {
	  level = REPORT_ERROR;
	} else if (c->lex.pragmas & ID_STRICT_TYPES) {
	  level = REPORT_WARNING;
	}
	yytype_report(level, NULL, 0, gs_type,
		      NULL, 0, type, 0,
		       "Type mismatch for callback function %S:", name);
      }
      c->lex.pragmas = orig_pragmas;
      if (flags & ID_VARIANT) {
	my_yyerror("Variants not supported for getter/setters: %S", name);
	flags &= ~ID_VARIANT;
      }
      i = isidentifier(symbol);
      if ((i >= 0) &&
	  !((ref = PTR_FROM_INT(prog, i))->id_flags & ID_INHERITED)) {
	/* Not an inherited symbol. */
	struct identifier *id = ID_FROM_INT(prog, i);
	if (!IDENTIFIER_IS_VARIABLE(id->identifier_flags)) {
	  my_yyerror("Illegal to redefine function %S with variable.", symbol);
	} else if (id->run_time_type != PIKE_T_GET_SET) {
	  my_yyerror("Illegal to redefine a current variable with a getter/setter: %S.", symbol);
	} else {
	  if ((ref->id_flags | ID_USED) != (flags | ID_USED)) {
	    if (Pike_compiler->compiler_pass == COMPILER_PASS_FIRST) {
	      yywarning("Modifier mismatch for variable %S.", symbol);
	    }
	    ref->id_flags &= flags | ID_USED;
	  }
	  getter_setter = i;
	}
	/* FIXME: Update id->type here. */
      } else {
	struct identifier *id;
	i = low_define_variable(symbol, symbol_type, flags,
				~0, PIKE_T_GET_SET);
	id = ID_FROM_INT(prog, i);

	/* Paranoia. */
	id->func.gs_info.getter = -1;
	id->func.gs_info.setter = -1;

	getter_setter = i;
      }
      /* NOTE: The function needs to have the same PRIVATE/INLINE
       *       behaviour as the variable for overloading to behave
       *       as expected.
       *
       * FIXME: Force PRIVATE?
       */
      flags |= ID_PROTECTED /* | ID_PRIVATE | ID_INLINE | ID_USED */;
      free_type(symbol_type);
      free_string(symbol);
    }
  }

  if(IDENTIFIER_IS_C_FUNCTION(function_flags))
    prog->flags |= PROGRAM_HAS_C_METHODS;

  if (Pike_compiler->compiler_pass == COMPILER_PASS_FIRST) {
    /* Mark the type as tentative by setting the runtime-type
     * to T_MIXED.
     *
     * NOTE: This should be reset to T_FUNCTION in pass 2.
     */
    run_time_type = T_MIXED;
  }

  i = isidentifier(name);
  if (Pike_compiler->compiler_pass == COMPILER_PASS_FIRST) {
    if (flags & ID_VARIANT) {
      if (i >= 0) {
	if (!is_variant_dispatcher(prog, i)) {
	  /* This function will be the termination function for
	   * our variant dispatcher.
	   */
	  struct reference ref = prog->identifier_references[i];
	  /* Make sure to not get complaints about multiple
	   * definitions when adding the variant dispatcher.
	   */
	  prog->identifier_references[i].id_flags |= ID_INHERITED;
	  add_variant_dispatcher(name, type, flags);
	  /* Restore the termination function as a variant. */
	  ref.id_flags |= ID_VARIANT;
	  if (is_variant_dispatcher(prog, i)) {
	    /* The termination function got replaced with
	     * the variant dispatcher.
	     */
	    add_to_identifier_references(ref);
	  } else {
	    /* The termination function is still in the same place. */
	    prog->identifier_references[i].id_flags = ref.id_flags;
	  }
	} else if (prog->identifier_references[i].inherit_offset) {
	  /* NB: If we are overriding an inherited dispatcher, there's
	   *     no need to go via it, since our new dispatcher can
	   *     just continue on with the old ones variant functions.
	   */
	  add_variant_dispatcher(name, type, flags);
	}
      } else {
	add_variant_dispatcher(name, type, flags);
      }
      i = really_low_find_variant_identifier(name, prog, type,
					     prog->num_identifier_references,
					     SEE_PROTECTED|SEE_PRIVATE);
    } else if (is_variant_dispatcher(prog, i) &&
	       !prog->identifier_references[i].inherit_offset) {
      if (!func || (func->c_fun != f_dispatch_variant) ||
	  !IDENTIFIER_IS_C_FUNCTION(function_flags)) {
	/* FIXME: What about the case
	 *
	 *  non-variant prototype;
	 *
	 *  variant;
	 *
	 *  non-variant definition.
	 */
	my_yyerror("Overriding variant function %S() with "
		   "non-variant in the same class.",
		   name);
      }
    }
  } else if (i >= 0) {
    /* Pass 2 */
    if (is_variant_dispatcher(prog, i)) {
      if (!func || (func->c_fun != f_dispatch_variant) ||
	  !IDENTIFIER_IS_C_FUNCTION(function_flags)) {
	/* Variant or variant termination function in second pass. */
	flags |= ID_VARIANT;
	i = really_low_find_variant_identifier(name, prog, type,
					       prog->num_identifier_references,
					       SEE_PROTECTED|SEE_PRIVATE);
      }
    }
  }

  if(i >= 0)
  {
    int overridden;

    /* already defined */

#ifdef PROGRAM_BUILD_DEBUG
    fprintf(stderr, "%*sexisted as identifier #%d\n",
	  c->compilation_depth, "", i);
#endif

    funp = ID_FROM_INT(prog, i);
    ref = prog->identifier_references[i];

    if (funp->identifier_flags & IDENTIFIER_HAS_BODY)
      /* Keep this flag. */
      function_flags |= IDENTIFIER_HAS_BODY;

    if(!(ref.id_flags & ID_INHERITED)) /* not inherited */
    {

      if( !( IDENTIFIER_IS_FUNCTION(funp->identifier_flags) &&
	     ( (!func || func->offset == -1) || (funp->func.offset == -1))))
      {
	my_yyerror("Identifier %S defined twice.", name);

	if (getter_setter != -1) {
	  struct identifier *id = ID_FROM_INT(prog, getter_setter);
	  (&id->func.gs_info.getter)[is_setter] = i;
	}
	return i;
      }

      /* Note: The type from pass 1 may be incompatible with the one from
       *       pass 2. Only do this in pass 2, and only if the previous
       *       type isn't from pass 1.
       */
      if ((Pike_compiler->compiler_pass == COMPILER_PASS_LAST) &&
	  (funp->run_time_type == T_FUNCTION)) {
	/* match types against earlier prototype or vice versa */
	if(!match_types(type, funp->type))
	{
	  yytype_report(REPORT_ERROR, NULL, 0,
			funp->type,
			NULL, 0, type, 0,
			"Prototype doesn't match for function %S.", name);
	}
      }

      if(func)
	funp->func = *func;
#if 0 /* prototypes does not override non-prototypes, ok? */
      else
	funp->func.offset = -1;
#endif

      funp->identifier_flags=function_flags;
      funp->run_time_type = run_time_type;

      funp->opt_flags &= opt_flags;

      free_type(funp->type);
      copy_pike_type(funp->type, type);
    }else{
#ifdef PROGRAM_BUILD_DEBUG
      fprintf(stderr, "%*sidentifier was inherited\n",
	      c->compilation_depth, "");
#endif

      if((ref.id_flags & ID_FINAL)
#if 0
	 && !(funp->func.offset == -1)
#endif
	)
      {
	my_yyerror("Illegal to redefine 'final' function %S.", name);
      }

      if (!(flags & ID_VARIANT) &&
	  (Pike_compiler->compiler_pass == COMPILER_PASS_FIRST) &&
	  (funp->func.c_fun == f_dispatch_variant) &&
	  (!func || (func->c_fun != f_dispatch_variant) ||
	   !IDENTIFIER_IS_C_FUNCTION(function_flags)) &&
	  IDENTIFIER_IS_C_FUNCTION(funp->identifier_flags)) {
	/* Overriding a variant function dispatcher with
	 * a non-variant function in pass 1.
	 *
	 * Hide the corresponding variant functions.
	 */
	int j = prog->num_identifier_references;
	while ((j = really_low_find_variant_identifier(name, prog, NULL, j,
						       SEE_PROTECTED|SEE_PRIVATE)) != -1) {
	  if (!prog->identifier_references[j].inherit_offset) {
	    /* FIXME: This doesn't catch all cases, and should probably
	     *        be moved to a place where it does.
	     */
	    my_yyerror("Overloading variant function %S with non-variant in same class.",
		       name);
	  }
	  prog->identifier_references[j].id_flags |= ID_HIDDEN;
	}
      }

      if(ref.id_flags & ID_INLINE)
      {
#ifdef PROGRAM_BUILD_DEBUG
	fprintf(stderr, "%*sidentifier is local\n",
		c->compilation_depth, "");
#endif
	/* Hide the previous definition, and make a new definition. */
	prog->identifier_references[i].id_flags |= ID_PROTECTED;
	goto make_a_new_def;
      }

      /* Otherwise we alter the existing definition */
#ifdef PROGRAM_BUILD_DEBUG
      fprintf(stderr, "%*saltering the existing definition\n",
	      c->compilation_depth, "");
#endif

      if(func)
	idptr = *func;
      else
	idptr.offset = -1;

      ref.identifier_offset =
	low_add_identifier(c, type, name,
			   function_flags, opt_flags,
			   idptr, run_time_type);
    }

    if (flags & ID_PRIVATE) flags |= ID_LOCAL|ID_PROTECTED;

    ref.inherit_offset = 0;
    ref.id_flags = flags;
    if (flags & ID_VARIANT) {
      ref.id_flags |= ID_USED;
      prog->identifier_references[i] = ref;
      overridden = i;
    } else {
      overridden = override_identifier(&ref, name, 0);
    }
    if (overridden >= 0) {
#ifdef PIKE_DEBUG
      struct reference *oref = prog->identifier_references+overridden;
      if((oref->inherit_offset != ref.inherit_offset) ||
	 (oref->identifier_offset != ref.identifier_offset) ||
	 ((oref->id_flags | ID_USED) != (ref.id_flags | ID_USED))) {
	fprintf(stderr,
		"ref: %d:%d 0x%04x\n"
		"got: %d:%d 0x%04x (%d)\n",
		ref.inherit_offset, ref.identifier_offset,
		ref.id_flags,
		oref->inherit_offset,
		oref->identifier_offset,
		oref->id_flags,
		overridden);
	Pike_fatal("New function overloading algorithm failed!\n");
      }
#endif

      if (getter_setter != -1) {
	struct identifier *id = ID_FROM_INT(prog, getter_setter);
	INT32 old_i = (&id->func.gs_info.getter)[is_setter];
	if ((old_i >= 0) && (old_i != overridden)) {
	  my_yyerror("Multiple definitions for %S.", name);
	}
	(&id->func.gs_info.getter)[is_setter] = overridden;
      }
      return overridden;
    }
    /* NOTE: At this point we already have the identifier in the
     *       new program, and just need to add the reference.
     */
  } else {
  make_a_new_def:

#ifdef PIKE_DEBUG
    if(Pike_compiler->compiler_pass == COMPILER_PASS_LAST)
      Pike_fatal("Internal error: Not allowed to add more identifiers during second compiler pass.\n"
		 "Added identifier: \"%s\"\n", name->str);
#endif

    /* Define a new function */

    if(func)
      idptr = *func;
    else
      idptr.offset = -1;

#ifdef PIKE_DEBUG
    if (a_flag > 5) {
      fprintf(stderr,
	      "Adding new function #%d: '%s'\n"
	      "  identifier_flags:0x%02x opt_flags:0x%04x\n",
	      prog->num_identifiers,
	      name->str,
	      function_flags, opt_flags);
    }
#endif /* PIKE_DEBUG */

    i = low_add_identifier(c, type, name,
			   function_flags, opt_flags,
			   idptr, run_time_type);

    if (flags & ID_PRIVATE) flags |= ID_LOCAL|ID_PROTECTED;

    ref.id_flags = flags;
    ref.identifier_offset = i;
    ref.inherit_offset = 0;
  }

  ref.run_time_type = PIKE_T_UNKNOWN;

  /* Add the reference. */

  i = prog->num_identifier_references;
  add_to_identifier_references(ref);

#ifdef PROGRAM_BUILD_DEBUG
  fprintf(stderr, "%*sadded new definition #%d\n",
	  c->compilation_depth, "", i);
#endif

  if (getter_setter != -1) {
    struct identifier *id = ID_FROM_INT(prog, getter_setter);
    INT32 old_i = (&id->func.gs_info.getter)[is_setter];
    if (old_i >= 0) {
      my_yyerror("Multiple definitions for %S.", name);
    }
    (&id->func.gs_info.getter)[is_setter] = i;
  }

  return i;
}

#if 0

int add_ext_ref(struct program_state *state, struct program *target, int i)
{
  struct reference ref, *r;
  int j;
  if (state->new_program == target) return i;
  i = add_ext_ref(state->previous, target, i);
  for (r = state->new_program->identifier_references, j = 0;
       j < state->new_program->num_identifier_references;
       j++, r++) {
    if (((r->id_flags & (ID_PARENT_REF|ID_PROTECTED|ID_PRIVATE|ID_HIDDEN)) ==
	 ID_PARENT_REF|ID_PROTECTED|ID_PRIVATE|ID_HIDDEN) &&
	(r->identifier_offset == i) &&
	(!(r->inherit_offset))) {
      return j;
    }
  }
  ref.id_flags = ID_PARENT_REF|ID_PROTECTED|ID_PRIVATE|ID_HIDDEN|ID_INLINE;
  ref.identifier_offset = i;
  ref.inherit_offset = 0;
  ref.run_time_type = PIKE_T_UNKNOWN;
  add_to_identifier_references(ref);
  state->new_program->flags |= PROGRAM_USES_PARENT | PROGRAM_NEEDS_PARENT;
  return j;
}

#endif /* 0 */

/**
 * Identifier lookup
 *
 * The search algorithm has changed several times during Pike 7.3.
 *
 * It now (Pike 7.3.33 and later) looks up the most recent definition
 * in the most recent inherit.
 *
 * In Pike 7.3.23 -- 7.3.32 it looked up the most recent definition
 * with the least inherit depth.
 *
 * In Pike 7.3.22 and prior, it looked up the last definition regardless
 * of inherit depth, unless there was a definition in the current program.
 *
 * Note also that the changed handling of mixin's in Pike 7.7.40 affects
 * the behaviour when using static inherit. See F below.
 *
 * Example:
 *
 * class A {
 *   int foo() {}
 * }
 *
 * class B {
 *   int foo() {}
 *   inherit A;
 * }
 *
 * class C {
 *   inherit B;
 * }
 *
 * class D {
 *   inherit B;
 *   inherit C;
 * }
 *
 * class E {
 *   int foo() {}
 * }
 *
 * class F {
 *   inherit A;
 *   static inherit E;
 * }
 *
 * Lookup of identifier "foo" in D():
 *
 * D-+-B-+-foo		Pike 7.3.23 --- Pike 7.3.32
 *   |   |
 *   |   +-A---foo
 *   |
 *   +-C---B-+-foo	Pike 7.3.33 ---
 *           |
 *           +-A---foo	            --- Pike 7.3.22
 *
 * Lookup of identifier "foo" in C():
 *
 * C---B-+-foo		Pike 7.3.23 ---
 *       |
 *       +-A---foo	            --- Pike 7.3.22
 *
 * Lookup of identifier "foo" in B():
 *
 * B-+-foo		All versions of Pike
 *   |
 *   +-A---foo
 *
 * External lookup of identifier "foo" in F():
 *
 * F-+-A---foo		            --- Pike 7.7.33
 *   |
 *   +-E---foo		Pike 7.7.34 ---
 */
PMOD_EXPORT int really_low_find_shared_string_identifier(const struct pike_string *name,
							 const struct program *prog,
							 int flags)
{
  struct reference *funp;
  struct identifier *fun;
  int id, i, depth, last_inh;

#if 0
  CDFPRINTF("th(%ld) %p Trying to find %s flags=%d\n",
            (long)th_self(), prog, name->str, flags);
#endif

#ifdef PIKE_DEBUG
  if (!prog) {
    Pike_fatal("really_low_find_shared_string_identifier(\"%s\", NULL, %d)\n"
	  "prog is NULL!\n", name->str, flags);
  }
#endif /* PIKE_DEBUG */

  id = -1;
  depth = 0;
  last_inh = prog->num_inherits;
  i = (int)prog->num_identifier_references;
  while(i--)
  {
    funp = prog->identifier_references + i;
    if(funp->id_flags & ID_HIDDEN) continue;
    if(funp->id_flags & ID_VARIANT) continue;
    if(funp->id_flags & ID_PROTECTED)
      if(!(flags & SEE_PROTECTED))
	continue;
    fun = ID_FROM_PTR(prog, funp);
    /* if(fun->func.offset == -1) continue; * Prototype */
    if(!is_same_string(fun->name,name)) continue;
    if(funp->id_flags & ID_INHERITED)
    {
      struct inherit *inh = INHERIT_FROM_PTR(prog, funp);
      if ((funp->id_flags & ID_PRIVATE) && !(flags & SEE_PRIVATE)) continue;
      if (!depth || (depth > inh->inherit_level)) {
	if (id != -1) {
	  int j;
	  int min_level = depth;
	  for (j=last_inh-1; j > funp->inherit_offset; j--) {
	    struct inherit *inh2 = prog->inherits + j;
	    if (inh2->inherit_level >= min_level) {
	      /* Got deeper in the inherit graph */
	      continue;
	    }
	    min_level = inh2->inherit_level;
	  }
	  if (!(inh->inherit_level < min_level)) {
	    continue;
	  }
	  /* Found new identifier on the path from the old identifier to
	   * the root.
	   */
	}
	last_inh = funp->inherit_offset;
	depth = inh->inherit_level;
	id = i;
      }
    } else {
      return i;
    }
  }
  return id;
}

int really_low_find_variant_identifier(struct pike_string *name,
				       struct program *prog,
				       struct pike_type *type,
				       int start_pos,
				       int flags)
{
  struct reference *funp;
  struct identifier *fun;
  int id, i, depth, last_inh;
  int tentative = -1;

#if 1
#ifdef COMPILER_DEBUG
  fprintf(stderr,"th(%ld) %p Trying to find variant \"%s\" start=%d flags=%d\n"
	  "  type: ",
	  (long)th_self(), prog, name->str, start_pos, flags);
  simple_describe_type(type);
  fprintf(stderr, "\n");
#endif
#endif

#ifdef PIKE_DEBUG
  if (!prog) {
    Pike_fatal("really_low_find_variant_identifier(\"%s\", NULL, %p, %d, %d)\n"
	       "prog is NULL!\n", name->str, type, start_pos, flags);
  }
#endif /* PIKE_DEBUG */

  id = -1;
  depth = 0;
  last_inh = prog->num_inherits;
  i = start_pos;
#ifdef PIKE_DEBUG
  if (i > (int)prog->num_identifier_references) {
    Pike_fatal("really_low_find_variant_identifier(\"%s\", %p, %p, %d, %d):\n"
	       "Start position is past max: %d\n",
	       name->str, prog, type, start_pos, flags,
	       prog->num_identifier_references);
  }
#endif /* PIKE_DEBUG */
  while(i--)
  {
    funp = prog->identifier_references + i;
    if(funp->id_flags & ID_HIDDEN) continue;
    if(!(funp->id_flags & ID_VARIANT)) continue;
    if(funp->id_flags & ID_PROTECTED)
      if(!(flags & SEE_PROTECTED))
	continue;
    fun = ID_FROM_PTR(prog, funp);
    /* if(fun->func.offset == -1) continue; * Prototype */
    if(!is_same_string(fun->name,name)) continue;
    if(type && (fun->type != type)) {
      if ((Pike_compiler->compiler_pass != COMPILER_PASS_FIRST) &&
	  !(funp->id_flags & ID_INHERITED) &&
	  match_types(fun->type, type)) {
	tentative = i;
      }
      continue;
    }
    if(funp->id_flags & ID_INHERITED)
    {
      struct inherit *inh = INHERIT_FROM_PTR(prog, funp);
      if ((funp->id_flags & ID_PRIVATE) && !(flags & SEE_PRIVATE)) continue;
      if (!depth || (depth > inh->inherit_level)) {
	if (id != -1) {
	  int j;
	  int min_level = depth;
	  for (j=last_inh-1; j > funp->inherit_offset; j--) {
	    struct inherit *inh2 = prog->inherits + j;
	    if (inh2->inherit_level >= min_level) {
	      /* Got deeper in the inherit graph */
	      continue;
	    }
	    min_level = inh2->inherit_level;
	  }
	  if (!(inh->inherit_level < min_level)) {
	    continue;
	  }
	  /* Found new identifier on the path from the old identifier to
	   * the root.
	   */
	}
	last_inh = funp->inherit_offset;
	depth = inh->inherit_level;
	id = i;
      }
    } else {
      CDFPRINTF("Found %d\n", i);
      return i;
    }
  }
  if ((id < 0) && (tentative >= 0)) {
    if (Pike_compiler->compiler_pass == COMPILER_PASS_LAST) {
      // Usually due to forward-referring types. The tentative match
      // is often correct, but may in some cases be wrong eg due to
      // having fall back implementations that use the mixed type.
      yytype_report(REPORT_WARNING,
		    NULL, 0, ID_FROM_INT(prog, tentative)->type,
		    NULL, 0, type,
		    0, "Variant type mismatch in second pass for %S.",
		    name);
    }
    id = tentative;
  }
  CDFPRINTF("Found %d\n", id);
  return id;
}

/**
 * This is the dispatcher function for variant functions.
 *
 * cf end_first_pass().
 */
static void f_dispatch_variant(INT32 args)
{
  struct pike_frame *fp = Pike_fp;
  struct program *prog = fp->context->prog;
  struct reference *funp = PTR_FROM_INT(fp->current_program, fp->fun);
  struct identifier *id = ID_FROM_PTR(fp->current_program, funp);
  struct pike_string *name = id->name;
  int fun_num = prog->num_identifier_references;
  int flags = 0;
  int best = -1;
  struct pike_type *t;
  int expected = 0;

  /* NB: The following is mostly to support a potential future
   *     case where a mixed set of protections would cause
   *     multiple dispatchers with the same name to be added
   *     (but different protection (and types)).
   */
  if (funp->id_flags & ID_PRIVATE) {
    flags = SEE_PRIVATE|SEE_PROTECTED;
  } else if (funp->id_flags & ID_PROTECTED) {
    flags = SEE_PROTECTED;
  }

  while ((fun_num = really_low_find_variant_identifier(name, prog, NULL,
						       fun_num, flags)) != -1) {
    int i;
    struct pike_type *ret;

    id = ID_FROM_INT(prog, fun_num);
    add_ref(t = id->type);

    /* Check whether the type is compatible with our arguments. */
    for (i = 0; i < args; i++) {
      struct pike_type *cont =
	check_call_svalue(t, 0, Pike_sp+i - (args + expected));
      if (!cont && (i >= best)) {
	if ((i > best) && expected) {
	  pop_n_elems(expected);
	  expected = 0;
	}
	best = i;
	push_type_value(t);
	expected++;
	t = NULL;
	break;
      } else {
	free_type(t);
      }
      if (!(t = cont)) break;
    }
    if (!t) continue;
    ret = new_get_return_type(t, 0);

    if (!ret && (i+1 >= best)) {
      if (((i+1) > best) && expected) {
	pop_n_elems(expected);
	expected = 0;
      }
      best = i+1;
      push_type_value(t);
      expected++;
      continue;
    }
    free_type(t);
    if (!ret) continue;
    free_type(ret);

    if (expected) {
      pop_n_elems(expected);
    }

    /* Found a function to call! */
    apply_current(fun_num, args);
    return;
  }
  if (!expected) {
    /* No variants listed? */
    Pike_error("No variants of %S() to dispatch to!\n", name);
  }
  if (expected > 1) {
    f_or(expected);
  }
  f___get_first_arg_type(1);
  if (!UNSAFE_IS_ZERO(Pike_sp-1)) {
    if (best < args) {
      Pike_error("Bad argument %d to %S(). Expected %O.\n",
		 best + 1, name, Pike_sp-1);
    } else {
      Pike_error("Too few arguments to %S(). Expected %O.\n",
		 name, Pike_sp-1);
    }
  } else {
    Pike_error("Too many arguments to %S().\n", name);
  }
}

PMOD_EXPORT int low_find_lfun(struct program *p, enum LFUN lfun)
{
  const struct pike_string *lfun_name;
  unsigned int flags = 0;
  int i;
  struct identifier *id;
#ifdef PIKE_DEBUG
  if ((size_t)lfun >= NELEM(lfun_strings)) {
    return find_lfun_fatal(p, lfun);
  }
#endif
  lfun_name = lfun_strings[lfun];

  i = really_low_find_shared_string_identifier(lfun_name,
					       dmalloc_touch(struct program *,
							     p),
					       SEE_PROTECTED);

  if ((i < 0) && (lfun == LFUN__DESTRUCT)) {
    /* Try the Pike 8.0 compatibility name. */
    i = really_low_find_shared_string_identifier(compat_lfun_destroy_string,
						 dmalloc_touch(struct program *,
							       p),
						 SEE_PROTECTED);
    if ((i >= 0) && !(p->flags & PROGRAM_FINISHED) && !TEST_COMPAT(8,0)) {
      struct compilation *c = MAYBE_THIS_COMPILATION;
      if (c && !(c->lex.pragmas & ID_NO_DEPRECATION_WARNINGS)) {
	yywarning("Compat: Substituting destroy() for _destruct().");
      }
    }
  }

  if (i < 0 || !(p->flags & PROGRAM_FIXED)) {
    return i;
  }
  id = ID_FROM_INT(p, i);
  if (IDENTIFIER_IS_PIKE_FUNCTION(id->identifier_flags) &&
      (id->func.offset == -1)) {
    /* Function prototype. */
    return -1;
  }
  return i;
}
#ifdef PIKE_DEBUG
PMOD_EXPORT int find_lfun_fatal(struct program *UNUSED(p), enum LFUN lfun)
{
  Pike_fatal("Invalid lfun number: %d\n", lfun);
  UNREACHABLE(return -1);
}
#endif

int lfun_lookup_id(struct pike_string *lfun_name)
{
  struct svalue *id = low_mapping_string_lookup(lfun_ids, lfun_name);
  if (!id) return -1;
  if (TYPEOF(*id) == T_INT) return id->u.integer;
  my_yyerror("Bad entry in lfun lookup table for %S.", lfun_name);
  return -1;
}

/**
 * Lookup the number of a function in a program given the name in
 * a shared_string
 */
int low_find_shared_string_identifier(const struct pike_string *name,
				      const struct program *prog)
{
  int max,min,tst;
  struct identifier *fun;

  if(prog->flags & PROGRAM_FIXED)
  {
    unsigned short *funindex = prog->identifier_index;
    size_t val_n = PTR_TO_INT (name);

#ifdef PIKE_DEBUG
    if(!funindex)
      Pike_fatal("No funindex in fixed program\n");
#endif

    max = prog->num_identifier_index;
    min = 0;
    while(max != min)
    {
      size_t val_t;

      tst=(max + min) >> 1;
      fun = ID_FROM_INT(prog, funindex[tst]);
      if(is_same_string(fun->name,name)) return funindex[tst];
      val_t = PTR_TO_INT (fun->name);
      if (val_n < val_t) {
	max = tst;
      } else {
	min = tst+1;
      }
    }
  }else{
    return really_low_find_shared_string_identifier(name,prog,0);
  }
  return -1;
}

#ifdef FIND_FUNCTION_HASHSIZE
#if FIND_FUNCTION_HASHSIZE == 0
#undef FIND_FUNCTION_HASHSIZE
#endif
#endif

#ifdef FIND_FUNCTION_HASHSIZE
struct ff_hash
{
  struct pike_string *name;
  int id;
  int fun;
};

static struct ff_hash cache[FIND_FUNCTION_HASHSIZE];
#endif

int find_shared_string_identifier(struct pike_string *name,
				  const struct program *prog)
{
#ifdef PIKE_DEBUG
  if (!prog) {
    Pike_fatal("find_shared_string_identifier(): No program!\n"
	  "Identifier: %s%s%s\n",
	  name?"\"":"", name?name->str:"NULL", name?"\"":"");
  }
#endif /* PIKE_DEBUG */
#ifdef FIND_FUNCTION_HASHSIZE
  if(prog -> flags & PROGRAM_FIXED
#if FIND_FUNCTION_HASH_TRESHOLD - 0
     && prog->num_identifier_index >= FIND_FUNCTION_HASH_TRESHOLD
#endif
    )
  {
    size_t hashval;
    hashval = my_hash_string(name);
    hashval += prog->id;
    hashval &= (FIND_FUNCTION_HASHSIZE-1);
    if(is_same_string(cache[hashval].name,name) &&
       cache[hashval].id==prog->id)
      return cache[hashval].fun;

    if(cache[hashval].name) free_string(cache[hashval].name);
    copy_shared_string(cache[hashval].name,name);
    cache[hashval].id=prog->id;
    return cache[hashval].fun=low_find_shared_string_identifier(name,prog);
  }
#endif /* FIND_FUNCTION_HASHSIZE */

  return low_find_shared_string_identifier(name,prog);
}

PMOD_EXPORT int find_identifier(const char *name,const struct program *prog)
{
  struct pike_string *n;
  if(!prog) {
    if (strlen(name) < 1024) {
      Pike_error("Lookup of identifier %s in destructed object.\n", name);
    } else {
      Pike_error("Lookup of long identifier in destructed object.\n");
    }
  }
  n=findstring(name);
  if(!n) return -1;
  return find_shared_string_identifier(n,prog);
}

int store_prog_string(struct pike_string *str)
{
  unsigned int i;
  if( str->refs > 1 )
      for (i=0;i<Pike_compiler->new_program->num_strings;i++)
          if (Pike_compiler->new_program->strings[i] == str)
              return i;

  reference_shared_string(str);
  add_to_strings(str);
  return Pike_compiler->new_program->num_strings-1;
}

/* NOTE: O(n²)! */
int store_constant(const struct svalue *foo,
		   int equal,
		   struct pike_string *UNUSED(constant_name))
{
  struct program_constant tmp;
  volatile unsigned int e;

  JMP_BUF jmp;
  if (SETJMP(jmp)) {
    handle_compile_exception ("Error comparing constants.");
    /* Assume that if `==() throws an error, the svalues aren't equal. */
    e = Pike_compiler->new_program->num_constants;
  } else {
    if(!equal &&
       (TYPEOF(*foo) == PIKE_T_MAPPING ||
        TYPEOF(*foo) == PIKE_T_MULTISET ||
        TYPEOF(*foo) == PIKE_T_STRING ||
        TYPEOF(*foo) == PIKE_T_ARRAY )) /* no possibility of comparator. Check refs first. */
    {
        if(*foo->u.refs == 1 )
        {
            e = Pike_compiler->new_program->num_constants;
            goto not_present;
        }
    }

    for(e=0;e<Pike_compiler->new_program->num_constants;e++)
    {
      struct program_constant *c = Pike_compiler->new_program->constants+e;

      if (TYPEOF(*foo) == TYPEOF(c->sval)) {
	/* Make sure only to compare within the same basic type. */
	if (TYPEOF(*foo) == T_OBJECT) {
	  /* Special case objects -- We don't want strange LFUN effects... */
	  if ((foo->u.object == c->sval.u.object) &&
	      (SUBTYPEOF(*foo) == SUBTYPEOF(c->sval))) {
	    UNSETJMP(jmp);
	    return e;
	  }
	} else if (TYPEOF(*foo) == T_INT) {
	  if (foo->u.integer == c->sval.u.integer) {
	    /* Make sure UNDEFINED is kept (but not in compat mode). */
	    if (foo->u.integer || (SUBTYPEOF(*foo) == SUBTYPEOF(c->sval))){
	      UNSETJMP(jmp);
	      return e;
	    }
	  }
	} else if(equal ? is_equal(& c->sval,foo) : is_eq(& c->sval,foo)) {
	  UNSETJMP(jmp);
	  return e;
	}
      }
    }
  }
not_present:
  UNSETJMP(jmp);
  assign_svalue_no_free(&tmp.sval,foo);
#if 0
  if((tmp.name=constant_name)) add_ref(constant_name);
#else /* !0 */
  tmp.offset = -1;
#endif /* 0 */

  add_to_constants(tmp);

  return e;
}

/*
 * program examination functions available from Pike.
 */

struct array *program_indices(struct program *p)
{
  int i;
  int n = 0;
  struct array *res;
  for (i = p->num_identifier_index; i--; ) {
    struct identifier *id;
    int e = p->identifier_index[i];
    if (p->identifier_references[e].id_flags &
	(ID_HIDDEN|ID_VARIANT|ID_PROTECTED|ID_PRIVATE)) {
      continue;
    }
    id = ID_FROM_INT(p, e);
    if (IDENTIFIER_IS_ALIAS(id->identifier_flags)) {
      /* FIXME!
       */
      continue;
    } else if (IDENTIFIER_IS_CONSTANT(id->identifier_flags)) {
      if (id->func.const_info.offset >= 0) {
	struct program *p2 = PROG_FROM_INT(p, e);
	struct svalue *val = &p2->constants[id->func.const_info.offset].sval;
	if ((TYPEOF(*val) != T_PROGRAM) ||
	    !(val->u.program->flags & PROGRAM_USES_PARENT)) {
	  ref_push_string(ID_FROM_INT(p, e)->name);
	  n++;
	}
      } else {
	/* Prototype constant. */
	ref_push_string(ID_FROM_INT(p, e)->name);
	n++;
      }
    }
  }
  f_aggregate(n);
  res = Pike_sp[-1].u.array;
  add_ref(res);
  pop_stack();
  return(res);
}

struct array *program_values(struct program *p)
{
  int i;
  int n = 0;
  struct array *res;
  for(i = p->num_identifier_index; i--; ) {
    struct identifier *id;
    int e = p->identifier_index[i];
    if (p->identifier_references[e].id_flags &
	(ID_HIDDEN|ID_VARIANT|ID_PROTECTED|ID_PRIVATE)) {
      continue;
    }
    id = ID_FROM_INT(p, e);
    if (IDENTIFIER_IS_ALIAS(id->identifier_flags)) {
      /* FIXME!
       */
      continue;
    } else if (IDENTIFIER_IS_CONSTANT(id->identifier_flags)) {
      if (id->func.const_info.offset >= 0) {
	struct program *p2 = PROG_FROM_INT(p, e);
	struct svalue *val = &p2->constants[id->func.const_info.offset].sval;
	if ((TYPEOF(*val) != T_PROGRAM) ||
	    !(val->u.program->flags & PROGRAM_USES_PARENT)) {
	  push_svalue(val);
	  n++;
	}
      } else {
	/* Prototype constant. */
	push_int(0);
	n++;
      }
    }
  }
  f_aggregate(n);
  res = Pike_sp[-1].u.array;
  add_ref(res);
  pop_stack();
  return(res);
}

struct array *program_types(struct program *p)
{
  int i;
  int n = 0;
  struct array *res;
  for (i = p->num_identifier_index; i--; ) {
    struct identifier *id;
    int e = p->identifier_index[i];
    if (p->identifier_references[e].id_flags &
	(ID_HIDDEN|ID_VARIANT|ID_PROTECTED|ID_PRIVATE)) {
      continue;
    }
    id = ID_FROM_INT(p, e);
    if (IDENTIFIER_IS_ALIAS(id->identifier_flags)) {
      /* FIXME!
       */
      continue;
    } else if (IDENTIFIER_IS_CONSTANT(id->identifier_flags)) {
      if (id->func.const_info.offset >= 0) {
	struct program *p2 = PROG_FROM_INT(p, e);
	struct svalue *val = &p2->constants[id->func.const_info.offset].sval;
	if ((TYPEOF(*val) != T_PROGRAM) ||
	    !(val->u.program->flags & PROGRAM_USES_PARENT)) {
	  ref_push_type_value(ID_FROM_INT(p, e)->type);
	  n++;
	}
      } else {
	/* Prototype constant. */
	ref_push_type_value(ID_FROM_INT(p, e)->type);
	n++;
      }
    }
  }
  f_aggregate(n);
  res = Pike_sp[-1].u.array;
  add_ref(res);
  pop_stack();
  return(res);
}

int low_program_index_no_free(struct svalue *to, struct program *p, int e,
			      struct object *parent, int parent_identifier)
{
  struct identifier *id;
  id=ID_FROM_INT(p, e);

  if (IDENTIFIER_IS_ALIAS(id->identifier_flags)) {
    struct external_variable_context loc;
    struct object fake_object;
    struct parent_info parent_info;
    int refid;

    if (!parent) return 0;

    parent_info.parent = parent;
    parent_info.parent_identifier = parent_identifier;
    fake_object.prog = p;
    fake_object.refs = 1;
    fake_object.next = fake_object.prev = NULL;
    fake_object.storage = ((char *)&parent_info) - p->parent_info_storage;
#ifdef PIKE_DEBUG
    fake_object.program_id = p->id;
#endif

    loc.o = &fake_object;
    loc.inherit = INHERIT_FROM_INT(p, e);
    loc.parent_identifier = 0;

    do {
      find_external_context(&loc, id->func.ext_ref.depth);
      refid = id->func.ext_ref.id;
      id = ID_FROM_INT(loc.o->prog, refid);
    } while (IDENTIFIER_IS_ALIAS(id->identifier_flags));

    if (fake_object.refs != 1) {
      Pike_fatal("Lost track of fake object! refs: %d\n",
		 fake_object.refs);
    }

    if (loc.o != &fake_object) {
      low_object_index_no_free(to, loc.o, refid);
      return 1;
    }
  }

  if (IDENTIFIER_IS_CONSTANT(id->identifier_flags)) {
    if (id->func.const_info.offset >= 0) {
      struct program *p2 = PROG_FROM_INT(p, e);
      struct svalue *val = &p2->constants[id->func.const_info.offset].sval;
      assign_svalue_no_free(to, val);
    } else {
      /* Prototype constant. */
      SET_SVAL(*to, T_INT, NUMBER_NUMBER, integer, 0);
    }
    return 1;
  }
  return 0;
}

int program_index_no_free(struct svalue *to, struct svalue *what,
			  struct svalue *ind)
{
  int e;
  struct object *parent = NULL;
  struct svalue *sub = NULL;
  struct program *p;
  struct identifier *id;
  int parent_identifier = -1;

  if (TYPEOF(*what) == T_PROGRAM) {
    p = what->u.program;
  } else if ((TYPEOF(*what) == T_FUNCTION) &&
	     (SUBTYPEOF(*what) != FUNCTION_BUILTIN) &&
	     ((parent = what->u.object)->prog) &&
	     IDENTIFIER_IS_CONSTANT((id = ID_FROM_INT(parent->prog,
						      SUBTYPEOF(*what)))->identifier_flags) &&
	     (id->func.const_info.offset != -1) &&
	     (TYPEOF(*(sub = &PROG_FROM_INT(parent->prog, SUBTYPEOF(*what))->
		       constants[id->func.const_info.offset].sval)) == T_PROGRAM)) {
    p = sub->u.program;
    parent_identifier = SUBTYPEOF(*what);
  } else {
    /* Not a program. */
    return 0;
  }
  if (TYPEOF(*ind) != T_STRING) {
    Pike_error("Can't index a program with a %s (expected string)\n",
	       get_name_of_type(TYPEOF(*ind)));
  }
  e=find_shared_string_identifier(ind->u.string, p);
  if(e!=-1)
  {
    if (low_program_index_no_free(to, p, e, parent, parent_identifier))
      return 1;
  }

  SET_SVAL(*to, T_INT, NUMBER_UNDEFINED, integer, 0);
  return 1;
}

/*
 * Line number support routines, now also tells what file we are in.
 */

/* program.linenumbers format:
 *
 * Filename entry:
 *   1. char		127 (marker).
 *   2. small number	Filename entry number in string table.
 *
 * Line number entry:
 *   1. small number	Index in program.program (pc).
 * 			Stored as the difference from the pc in the
 * 			closest previous line number entry. The first
 * 			stored entry is absolute.
 *   2. small number	Line number. Stored in the same way as the pc.
 *
 * Small number:
 *   If -127 < n < 127:
 *     1. char		The number.
 *   Else if -32768 <= n < 32768:
 *     1. char		-127 (marker).
 *     2. short		The 16-bit signed number stored in big endian order.
 *   Else if n < -0x80000000 or n > 0x7fffffff:
 *     1. char		-127 (marker).
 *     2. short		Zero (64-bit marker).
 *     3. INT_TYPE	The 64-bit signed number stored in big endian order.
 *   Else:
 *     1. char		-128 (marker).
 *     2. int		The 32-bit signed number stored in big endian order.
 *
 * Whenever the filename changes, a filename entry followed by a line
 * number entry is stored. If only the line number changes, a line
 * number entry is stored. The first stored entry (at pc 0) is the
 * file and line where the program is defined, if they are known. The
 * definition line for a top level program is set to 0.
 */

INT_TYPE get_small_number(char **q)
{
  /* This is a workaround for buggy cc & Tru64 */
  unsigned char *addr = (unsigned char *)*q;
  INT_TYPE ret = *((signed char *)addr);
  addr++;
  switch(ret)
  {
  case -127:
    ret = (INT16)get_unaligned_be16(addr);
    addr += 2;
    if (!ret) {
      /* 64-bit signed number. */
      ret = get_unaligned_be64(addr);
      addr += 8;
    }
    break;

  case -128:
    ret = (INT32)get_unaligned_be32(addr);
    addr += 4;
    break;

#ifdef PIKE_DEBUG
  case 127:
    Pike_fatal("get_small_number used on filename entry\n");
#endif
  }
  *q = (char *)addr;
  return ret;
}

void start_line_numbering(void)
{
  if(Pike_compiler->last_file)
  {
    free_string(Pike_compiler->last_file);
    Pike_compiler->last_file=0;
  }
  Pike_compiler->last_pc=Pike_compiler->last_line=0;
}

static void insert_small_number(INT_TYPE a)
{
#ifdef PIKE_DEBUG
  size_t start = Pike_compiler->new_program->num_linenumbers;
#endif /* PIKE_DEBUG */
  if(a>-127 && a<127)
  {
    add_to_linenumbers(a);
  }else if(a>=-32768 && a<32768){
    add_to_linenumbers(-127);
    add_to_linenumbers(a>>8);
    add_to_linenumbers(a);
#ifdef INT_TYPE_INT32_CONVERSION
  } else if (a < -0x80000000L || a > 0x7fffffffL) {
    /* Overload 16-bit zero as marker for 64-bit. */
    fprintf(stderr, "Saving huge linenumber: %lld\n", (long long)a);
    add_to_linenumbers(-127);
    add_to_linenumbers(0);
    add_to_linenumbers(0);
    add_to_linenumbers(a>>56);
    add_to_linenumbers(a>>48);
    add_to_linenumbers(a>>40);
    add_to_linenumbers(a>>32);
    add_to_linenumbers(a>>24);
    add_to_linenumbers(a>>16);
    add_to_linenumbers(a>>8);
    add_to_linenumbers(a);
#endif
  }else{
    add_to_linenumbers(-128);
    add_to_linenumbers(a>>24);
    add_to_linenumbers(a>>16);
    add_to_linenumbers(a>>8);
    add_to_linenumbers(a);
  }
#ifdef PIKE_DEBUG
  {
    char *tmp = Pike_compiler->new_program->linenumbers + start;
    INT_TYPE res = get_small_number(&tmp);
    if (a != res) {
      tmp = Pike_compiler->new_program->linenumbers + start;
      fprintf(stderr, "0x%p: %02x %02x %02x %02x %02x\n",
	      tmp, (unsigned char) tmp[0], (unsigned char) tmp[1],
	      (unsigned char) tmp[2], (unsigned char) tmp[3], (unsigned char) tmp[4]);
      if ((start + 5) < Pike_compiler->new_program->num_linenumbers) {
	/* 64-bit. Dump the remaining 6 bytes as well. */
	fprintf(stderr, "0x%p: %02x %02x %02x %02x %02x %02x\n",
		tmp, (unsigned char) tmp[5], (unsigned char) tmp[6],
		(unsigned char) tmp[7], (unsigned char) tmp[8],
		(unsigned char) tmp[9], (unsigned char) tmp[10]);
      }
      Pike_fatal("insert_small_number failed: %ld (0x%08lx) != %ld (0x%08lx)\n",
		 (long)a, (long)a, (long)res, (long)res);
    }
  }
#endif /* PIKE_DEBUG */
}

static void ext_insert_small_number (char **ptr, INT_TYPE a)
{
  if(a>-127 && a<127)
  {
    *(*ptr)++ = a;
  }else if(a>=-32768 && a<32768){
    *(*ptr)++ = -127;
    *(*ptr)++ = a>>8;
    *(*ptr)++ = a;
#ifdef INT_TYPE_INT32_CONVERSION
  } else if (a < -0x80000000L || a > 0x7fffffffL) {
    /* Overload 16-bit zero as marker for 64-bit. */
    *(*ptr)++ = -127;
    *(*ptr)++ = 0;
    *(*ptr)++ = 0;
    *(*ptr)++ = a>>56;
    *(*ptr)++ = a>>48;
    *(*ptr)++ = a>>40;
    *(*ptr)++ = a>>32;
    *(*ptr)++ = a>>24;
    *(*ptr)++ = a>>16;
    *(*ptr)++ = a>>8;
    *(*ptr)++ = a;
#endif
  }else{
    *(*ptr)++ = -128;
    *(*ptr)++ = a>>24;
    *(*ptr)++ = a>>16;
    *(*ptr)++ = a>>8;
    *(*ptr)++ = a;
  }
}

void ext_store_program_line (struct program *prog, INT_TYPE line, struct pike_string *file)
{
  char *ptr;

#ifdef PIKE_DEBUG
  if (prog->linenumbers || prog->strings)
    Pike_fatal ("Program already got linenumber info.\n");
  if (Pike_compiler->new_program == prog)
    Pike_fatal ("Use store_linenumber instead when the program is compiled.\n");
#endif

  add_ref((prog->strings = xalloc(sizeof(struct pike_string *)))[0] = file);
  prog->num_strings = 1;

  ptr = prog->linenumbers = xalloc (1 + 1 + 1 + 11);
  *ptr++ = 127;							/* 1 */
  *ptr++ = 0;			/* String #0 */			/* 1 */
  *ptr++ = 0;			/* PC */			/* 1 */
  ext_insert_small_number (&ptr, line);				/* 11 */
  prog->num_linenumbers = ptr - prog->linenumbers;
}

void store_linenumber(INT_TYPE current_line, struct pike_string *current_file)
{
/*  if(!store_linenumbers)  Pike_fatal("Fnord.\n"); */
#ifdef PIKE_DEBUG
  if(a_flag)
  {
    INT_TYPE line=0;
    INT32 off=0;
    char *cnt=Pike_compiler->new_program->linenumbers;
    struct pike_string *file = NULL;

    if (a_flag > 50) {
      fprintf(stderr, "store_linenumber(%ld, \"%s\") at pc %d\n",
	      (long)current_line, current_file->str,
	      (INT32) PIKE_PC);
      fprintf(stderr, "  last_line:%ld last_file:\"%s\"\n",
	      (long)Pike_compiler->last_line,
	      Pike_compiler->last_file?Pike_compiler->last_file->str:"");
    }

    while(cnt < Pike_compiler->new_program->linenumbers +
	  Pike_compiler->new_program->num_linenumbers)
    {
      char *start = cnt;
      if(*cnt == 127)
      {
	int strno;
	cnt++;
	strno = get_small_number(&cnt);
	CHECK_FILE_ENTRY (Pike_compiler->new_program, strno);
	if (a_flag > 100) {
	  file = Pike_compiler->new_program->strings[strno];
	  fprintf(stderr, "Filename entry:\n"
		  "  len: %"PRINTSIZET"d, shift: %d\n",
		  file->len, file->size_shift);
	}
      }
      off+=get_small_number(&cnt);
      line+=get_small_number(&cnt);
      if (a_flag > 100) {
	fprintf(stderr, "  off: %d, line: %ld\n"
		"  raw: ",
		off, (long)line);
	for (;start < cnt; start++) {
	  fprintf(stderr, "%02x ", *((unsigned char *)start));
	}
	fprintf(stderr, "\n");
      }
    }

    if(Pike_compiler->last_line != line ||
       Pike_compiler->last_pc != off ||
       (file && (Pike_compiler->last_file != file)))
    {
      Pike_fatal("Line numbering out of whack\n"
	    "    (line : %ld ?= %ld)!\n"
	    "    (  pc : %d ?= %d)!\n"
	    "    (shift: %d ?= %d)!\n"
	    "    (len  : %"PRINTSIZET"d ?= %"PRINTSIZET"d)!\n"
	    "    (file : %s ?= %s)!\n",
		 (long)Pike_compiler->last_line, (long)line,
	    Pike_compiler->last_pc, off,
	    Pike_compiler->last_file?Pike_compiler->last_file->size_shift:0,
	    file?file->size_shift:0,
	    Pike_compiler->last_file?Pike_compiler->last_file->len:0,
	    file?file->len:0,
	    Pike_compiler->last_file?Pike_compiler->last_file->str:"N/A",
	    file?file->str:"N/A");
    }
  }
#endif
  if(Pike_compiler->last_line != current_line ||
     Pike_compiler->last_file != current_file)
  {
    if(Pike_compiler->last_file != current_file)
    {
      if(Pike_compiler->last_file) free_string(Pike_compiler->last_file);
      add_to_linenumbers(127);
      insert_small_number(store_prog_string(current_file));
      copy_shared_string(Pike_compiler->last_file, current_file);
    }
    insert_small_number((INT32)(PIKE_PC-Pike_compiler->last_pc));
    insert_small_number(current_line-Pike_compiler->last_line);
    Pike_compiler->last_line = current_line;
    Pike_compiler->last_pc = (INT32)PIKE_PC;
  }
}

#define FIND_PROGRAM_LINE(prog, file, line) do {			\
    char *pos = prog->linenumbers;					\
    file = NULL;							\
									\
    if (pos < prog->linenumbers + prog->num_linenumbers) {		\
      if (*pos == 127) {						\
	int strno;							\
	pos++;								\
	strno = get_small_number(&pos);					\
	CHECK_FILE_ENTRY (prog, strno);					\
	file = prog->strings[strno];					\
      }									\
      get_small_number(&pos);	/* Ignore the offset */			\
      line = get_small_number(&pos);					\
    }									\
  } while (0)

PMOD_EXPORT struct pike_string *low_get_program_line (struct program *prog,
						      INT_TYPE *linep)
{
  *linep = 0;

  if (prog->linenumbers) {
    struct pike_string *file;

    FIND_PROGRAM_LINE (prog, file, (*linep));

    if (file) {
      add_ref(file);
      return file;
    }
  }

  return NULL;
}

static char *make_plain_file (struct pike_string *filename, int malloced)
{
  static char buf[1000];
  char *file = filename->str;
  size_t len = filename->len;
  INT32 shift = filename->size_shift;

  if(shift)
  {
    size_t bufsize;
    char *buffer;
    PCHARP from=MKPCHARP(file, shift);
    size_t ptr=0;

    if (malloced) {
      bufsize = len + 21;
      buffer = malloc (bufsize);
      if(!buffer) return NULL;
    }
    else {
      bufsize = NELEM(buf) - 1;
      buffer = buf;
    }

    for (; len--; INC_PCHARP(from, 1))
    {
      size_t space;
      int chr = EXTRACT_PCHARP(from);
      space = chr > 255 ? 20 : 1;

      if (ptr + space > bufsize) {
	if (malloced) {
          char *new_buffer;
	  bufsize = (bufsize << 1) + space + 1;
	  new_buffer = realloc (buffer, bufsize);
          if(!new_buffer)
          {
            free(buffer);
            return NULL;
          }
          buffer = new_buffer;
	}
	else
	  break;
      }

      if(chr > 255)
      {
	sprintf(buffer+ptr,"\\u%04X",chr);
	ptr+=strlen(buffer+ptr);
      }else{
	buffer[ptr++]=chr;
      }
    }

    buffer[ptr]=0;
    return buffer;
  }

  else{
    char *buffer;
    if (malloced)
      buffer = malloc (len + 1);
    else {
      buffer = buf;
      if (len > NELEM (buf) - 1)
	len = NELEM (buf) - 1;
    }
    memcpy (buffer, file, len);
    buffer[len] = 0;
    return buffer;
  }
}

/**
 * Same as low_get_program_line but returns a plain char *. It's
 * malloced if the malloced flag is set, otherwise it's a pointer to a
 * static buffer which might be clobbered by later calls.
 *
 * This function is useful when the shared string table has been freed
 * and in sensitive parts of the gc where the shared string structures
 * can't be touched. It also converts wide strings to ordinary ones
 * with escaping.
 */
PMOD_EXPORT char *low_get_program_line_plain(struct program *prog,
					     INT_TYPE *linep,
					     int malloced)
{
  *linep = 0;

  if (prog->linenumbers) {
    struct pike_string *file;
    FIND_PROGRAM_LINE (prog, file, (*linep));
    if (file)
      return make_plain_file(file, malloced);
  }

  return NULL;
}

/**
 * Returns the file where the program is defined. The line of the
 * class start is written to linep, or 0 if the program is the top
 * level of the file.
 */
PMOD_EXPORT struct pike_string *get_program_line(struct program *prog,
						 INT_TYPE *linep)
{
  struct pike_string *res = low_get_program_line(prog, linep);
  if (!res) {
    struct pike_string *dash;
    REF_MAKE_CONST_STRING(dash, "-");
    return dash;
  }
  return res;
}

PMOD_EXPORT struct pike_string *low_get_line (PIKE_OPCODE_T *pc,
					      struct program *prog,
					      INT_TYPE *linep)
{
  linep[0] = 0;

  if (prog->program && prog->linenumbers) {
    ptrdiff_t offset = pc - prog->program;
    if ((offset < (ptrdiff_t)prog->num_program) && (offset >= 0)) {
      static struct pike_string *file = NULL;
      static char *base, *cnt;
      static ptrdiff_t off;
      static INT32 pid;
      static INT_TYPE line;

      if(prog->linenumbers == base && prog->id == pid && offset > off &&
	 cnt < prog->linenumbers + prog->num_linenumbers)
	goto fromold;

      base = cnt = prog->linenumbers;
      off=line=0;
      pid=prog->id;
      file = 0;

      while(cnt < prog->linenumbers + prog->num_linenumbers)
      {
	if(*cnt == 127)
	{
	  int strno;
	  cnt++;
	  strno = get_small_number(&cnt);
	  CHECK_FILE_ENTRY (prog, strno);
	  file = prog->strings[strno];
	  continue;
	}
	off+=get_small_number(&cnt);
      fromold:
	if(off > offset) break;
	line+=get_small_number(&cnt);
      }
      if (cnt >= prog->linenumbers + prog->num_linenumbers) {
	/* We reached the end of the table. Make sure
	 * we get in sync again next time we're called.
	 */
	base = NULL;
      }
      linep[0]=line;
      if (file) {
	add_ref(file);
	return file;
      }
    } else {
      fprintf(stderr, "Bad offset: pc:%p program:%p (%p)\n",
	      pc, prog->program, (void *)prog->num_program);
    }
  } else {
    fprintf(stderr, "No program of linenumbers program:%p linenumbers:%p\n",
	    prog->program, prog->linenumbers);
  }

  return NULL;
}

/**
 * This is to low_get_line as low_get_program_line_plain is to
 * low_get_program_line.
 */
PMOD_EXPORT char *low_get_line_plain (PIKE_OPCODE_T *pc, struct program *prog,
				      INT_TYPE *linep, int malloced)
{
  linep[0] = 0;

  if (prog->program && prog->linenumbers) {
    ptrdiff_t offset = pc - prog->program;

    if ((offset < (ptrdiff_t)prog->num_program) && (offset >= 0)) {
      char *cnt = prog->linenumbers;
      INT32 off = 0;
      INT_TYPE line = 0;
      struct pike_string *file = NULL;

      while(cnt < prog->linenumbers + prog->num_linenumbers)
      {
	if(*cnt == 127)
	{
	  int strno;
	  cnt++;
	  strno = get_small_number(&cnt);
	  CHECK_FILE_ENTRY (prog, strno);
	  file = prog->strings[strno];
	}
	off+=get_small_number(&cnt);
	if(off > offset) break;
	line+=get_small_number(&cnt);
      }
      linep[0]=line;

      if (file)
	return make_plain_file(file, malloced);
    }
  }

  return NULL;
}

#ifdef PIKE_DEBUG
/* Variants for convenient use from a debugger. */

void gdb_program_line (struct program *prog)
{
  INT_TYPE line;
  char *file = low_get_program_line_plain (prog, &line, 0);
  fprintf (stderr, "%s:%ld\n", file, (long)line);
}

void gdb_get_line (PIKE_OPCODE_T *pc, struct program *prog)
{
  INT_TYPE line;
  char *file = low_get_line_plain (pc, prog, &line, 0);
  fprintf (stderr, "%s:%ld\n", file, (long)line);
}

#endif

/**
 * Return the file in which we were executing. pc should be the
 * program counter (i.e. the address in prog->program), prog the
 * current program, and line will be initialized to the line in that
 * file.
 */
PMOD_EXPORT struct pike_string *get_line(PIKE_OPCODE_T *pc,
					 struct program *prog,
					 INT_TYPE *linep)
{
  struct pike_string *res;

  if (prog == 0) {
    struct pike_string *unknown_program;
    REF_MAKE_CONST_STRING(unknown_program, "Unknown program");
    linep[0] = 0;
    return unknown_program;
  }

  res = low_get_line(pc, prog, linep);
  if (!res) {
    struct pike_string *not_found;
    REF_MAKE_CONST_STRING(not_found, "Line not found");
    return not_found;
  }
  return res;
}

PMOD_EXPORT struct pike_string *low_get_function_line (struct object *o,
						       int fun,
						       INT_TYPE *linep)
{
  if (o->prog) {
    struct program *p;
    struct identifier *id;
    struct pike_string *ret;
    while (1) {
      struct external_variable_context loc;
      struct reference *idref = o->prog->identifier_references + fun;
      p = PROG_FROM_PTR(o->prog, idref);
      id = p->identifiers + idref->identifier_offset;
      if (!IDENTIFIER_IS_ALIAS(id->identifier_flags)) break;
      loc.o = o;
      loc.inherit = INHERIT_FROM_INT(o->prog, fun);
      loc.parent_identifier = fun;
      find_external_context(&loc, id->func.ext_ref.depth);
      fun = id->func.ext_ref.id + loc.inherit->identifier_level;
      o = loc.o;
    }
    if (IDENTIFIER_IS_PIKE_FUNCTION(id->identifier_flags) &&
	(id->func.offset != -1))
      return low_get_line (p->program + id->func.offset, p, linep);
    if ((ret = get_identifier_line(o->prog, fun, linep))) {
      add_ref(ret);
      return ret;
    }
    return low_get_program_line(o->prog, linep);
  }
  *linep = 0;
  return NULL;
}

/**
 * Return the file and line where the identifier with reference number
 * fun was defined.
 *
 * Note: Unlike the other get*line() functions, this one does not
 *       add a reference to the returned string.
 */
PMOD_EXPORT struct pike_string *get_identifier_line(struct program *p,
						    int fun,
						    INT_TYPE *linep)
{
  struct reference *ref = PTR_FROM_INT(p, fun);
  struct identifier *id = ID_FROM_PTR(p, ref);
  p = PROG_FROM_PTR(p, ref);
  if (id->filename_strno >= p->num_strings) return NULL;
  if (linep) *linep = id->linenumber;
  return p->strings[id->filename_strno];
}

PMOD_EXPORT int low_quick_add_function(struct pike_string * name_tmp,
				   void (*cfun)(INT32),
				   const char *type,
				   int UNUSED(type_length),
				   unsigned flags,
				   unsigned opt_flags)
{
  int ret;
  struct pike_type *type_tmp;
  union idptr tmp;
/*  fprintf(stderr,"ADD_FUNC: %s\n",name); */
  type_tmp = make_pike_type(type);

  if(cfun)
  {
    tmp.c_fun=cfun;
    ret=define_function(name_tmp,
			type_tmp,
			flags,
			IDENTIFIER_C_FUNCTION,
			&tmp,
			opt_flags);
  }else{
    ret=define_function(name_tmp,
			type_tmp,
			flags,
			IDENTIFIER_C_FUNCTION,
			0,
			opt_flags);
  }
  free_string(name_tmp);
  free_type(type_tmp);
  return ret;
}

#ifdef PIKE_DEBUG
void check_all_programs(void)
{
  struct program *p;
  for(p=first_program;p;p=p->next)
    check_program(p);

#ifdef FIND_FUNCTION_HASHSIZE
  {
    unsigned long e;
    for(e=0;e<FIND_FUNCTION_HASHSIZE;e++)
    {
      if(cache[e].name)
      {
	check_string(cache[e].name);
	if(cache[e].id<0 || cache[e].id > current_program_id)
	  Pike_fatal("Error in find_function_cache[%ld].id\n",(long)e);

	if(cache[e].fun < -1 || cache[e].fun > 65536)
	  Pike_fatal("Error in find_function_cache[%ld].fun\n",(long)e);
      }
    }
  }
#endif

}
#endif


void init_program(void)
{
  size_t i;
  struct svalue key;
  struct svalue val;
  struct svalue id;

  MAKE_CONST_STRING(this_function_string,"this_function");
  MAKE_CONST_STRING(this_program_string,"this_program");
  MAKE_CONST_STRING(this_string,"this");
  MAKE_CONST_STRING(UNDEFINED_string,"UNDEFINED");
  MAKE_CONST_STRING(args_string, "__args__");

  MAKE_CONST_STRING(parser_system_string, "parser");
  MAKE_CONST_STRING(type_check_system_string, "type_check");

  MAKE_CONST_STRING(compat_lfun_destroy_string, "destroy");

  lfun_ids = allocate_mapping(NUM_LFUNS);
  lfun_types = allocate_mapping(NUM_LFUNS);
  for (i=0; i < NELEM(lfun_names); i++) {
    lfun_strings[i] = make_shared_static_string(lfun_names[i], strlen(lfun_names[i]), eightbit);

    SET_SVAL(id, T_INT, NUMBER_NUMBER, integer, i);
    SET_SVAL(key, T_STRING, 0, string, lfun_strings[i]);
    mapping_insert(lfun_ids, &key, &id);

    SET_SVAL(val, T_TYPE, 0, type, make_pike_type(raw_lfun_types[i]));
    mapping_insert(lfun_types, &key, &val);
    free_type(val.u.type);
  }

  lfun_getter_type_string = make_pike_type(tFuncV(tNone, tVoid, tMix));
  lfun_setter_type_string = make_pike_type(tFuncV(tZero, tVoid, tVoid));

  init_pike_compiler();

  enter_compiler(NULL, 0);

  exit_compiler();
}

void cleanup_program(void)
{
  size_t e;

  free_type(lfun_setter_type_string);
  free_type(lfun_getter_type_string);
  free_mapping(lfun_types);
  free_mapping(lfun_ids);
  for (e=0; e < NELEM(lfun_names); e++) {
    free_string(lfun_strings[e]);
  }
#ifdef FIND_FUNCTION_HASHSIZE
  for(e=0;e<FIND_FUNCTION_HASHSIZE;e++)
  {
    if(cache[e].name)
    {
      free_string(cache[e].name);
      cache[e].name=0;
    }
  }
#endif

#ifdef DO_PIKE_CLEANUP
  if (reverse_symbol_table) {
    free_mapping(reverse_symbol_table);
    reverse_symbol_table = NULL;
  }

  if(null_program)
  {
    free_program(null_program);
    null_program=0;
  }

  if(placeholder_object)
  {
    free_object(placeholder_object);
    placeholder_object=0;
  }

  if(placeholder_program)
  {
    free_program(placeholder_program);
    placeholder_program=0;
  }
#endif

  cleanup_pike_compiler();
}


PMOD_EXPORT void visit_program (struct program *p, int action, void *extra)
{
  visit_enter(p, T_PROGRAM, extra);
  switch (action & VISIT_MODE_MASK) {
#ifdef PIKE_DEBUG
    default:
      Pike_fatal ("Unknown visit action %d.\n", action);
    case VISIT_NORMAL:
    case VISIT_COMPLEX_ONLY:
      break;
#endif
    case VISIT_COUNT_BYTES:
      mc_counted_bytes += p->total_size;
      break;
  }

  if (!(p->flags & PROGRAM_AVOID_CHECK)) {
    int e;
    struct program_constant *consts = p->constants;
    struct inherit *inh = p->inherits;

    for (e = p->num_constants - 1; e >= 0; e--)
      visit_svalue (&consts[e].sval, REF_TYPE_NORMAL, extra);

    for (e = p->num_inherits - 1; e >= 0; e--) {
      if (inh[e].parent)
	visit_object_ref (inh[e].parent, REF_TYPE_NORMAL, extra);

      if (e && inh[e].prog)
	visit_program_ref (inh[e].prog, REF_TYPE_NORMAL, extra);
    }

    if (!(action & VISIT_COMPLEX_ONLY)) {
      struct identifier *ids = p->identifiers;
      struct pike_string **strs = p->strings;

      for (e = p->num_inherits - 1; e >= 0; e--) {
	if (inh[e].name)
	  visit_string_ref (inh[e].name, REF_TYPE_NORMAL, extra);
      }

      for (e = p->num_identifiers - 1; e >= 0; e--) {
	struct identifier *id = ids + e;
	visit_string_ref (id->name, REF_TYPE_NORMAL, extra);
	visit_type_ref (id->type, REF_TYPE_NORMAL, extra);
      }

      for (e = p->num_strings - 1; e >= 0; e--)
	visit_string_ref (strs[e], REF_TYPE_NORMAL, extra);
    }

    /* Strong ref follows. It must be last. */
    if (p->parent)
      visit_program_ref (p->parent, REF_TYPE_STRONG, extra);
  }
  visit_leave(p, T_PROGRAM, extra);
}

static void gc_check_program(struct program *p);

void gc_mark_program_as_referenced(struct program *p)
{
  debug_malloc_touch(p);

  if (p->flags & PROGRAM_AVOID_CHECK) {
    /* Program is in an inconsistent state.
     * don't look closer at it.
     */
    debug_malloc_touch(p);

    if (gc_mark(p, T_PROGRAM)) {
      if (p == gc_mark_program_pos)
	gc_mark_program_pos = p->next;
      if (p == gc_internal_program)
	gc_internal_program = p->next;
      else {
	DOUBLEUNLINK(first_program, p);
	DOUBLELINK(first_program, p); /* Linked in first. */
      }
    }

    return;
  }

  if(gc_mark(p, T_PROGRAM))
    GC_ENTER (p, T_PROGRAM) {
      int e;

      if (p == gc_mark_program_pos)
	gc_mark_program_pos = p->next;
      if (p == gc_internal_program)
	gc_internal_program = p->next;
      else {
	DOUBLEUNLINK(first_program, p);
	DOUBLELINK(first_program, p); /* Linked in first. */
      }

      if(p->parent)
	gc_mark_program_as_referenced(p->parent);

      for(e = p->num_constants - 1; e >= 0; e--)
	gc_mark_svalues(& p->constants[e].sval, 1);

      for(e = p->num_inherits - 1; e >= 0; e--)
      {
	if(p->inherits[e].parent)
	  gc_mark_object_as_referenced(p->inherits[e].parent);

	if(e && p->inherits[e].prog)
	  gc_mark_program_as_referenced(p->inherits[e].prog);
      }

#if defined (PIKE_DEBUG) || defined (DO_PIKE_CLEANUP)
      for (e = p->num_identifiers - 1; e >= 0; e--)
	gc_mark_type_as_referenced (p->identifiers[e].type);
#endif

    } GC_LEAVE;
}

void real_gc_cycle_check_program(struct program *p, int weak)
{
  GC_CYCLE_ENTER(p, T_PROGRAM, weak) {
    int e;

    if (!(p->flags & PROGRAM_AVOID_CHECK))
    {
      for(e = p->num_constants - 1; e >= 0; e--)
	gc_cycle_check_svalues(& p->constants[e].sval, 1);

      for(e = p->num_inherits - 1; e >= 0; e--)
      {
	if(p->inherits[e].parent)
	  gc_cycle_check_object(p->inherits[e].parent, 0);

	if(e && p->inherits[e].prog)
	  gc_cycle_check_program(p->inherits[e].prog, 0);
      }

      /* Strong ref follows. It must be last. */
      if(p->parent)
	gc_cycle_check_program(p->parent, 0);
    }
  } GC_CYCLE_LEAVE;
}

static void gc_check_program(struct program *p)
{
  int e;

  debug_malloc_touch(p);

  if (p->flags & PROGRAM_AVOID_CHECK) {
    /* Program is in an inconsistent state.
     * don't look closer at it.
     */
    debug_malloc_touch(p);
    return;
  }

  GC_ENTER (p, T_PROGRAM) {
    if(p->parent)
      debug_gc_check (p->parent, " as parent program of a program");

    for(e = p->num_constants - 1; e >= 0; e--) {
      debug_gc_check_svalues (&p->constants[e].sval, 1, " as program constant");
    }

    for(e = p->num_inherits - 1; e >= 0; e--)
    {
      if(p->inherits[e].parent)
      {
	debug_gc_check (p->inherits[e].parent,
			" as inherited parent object of a program");
      }

#ifdef PIKE_DEBUG
      if (Pike_in_gc == GC_PASS_LOCATE && p->inherits[e].name)
	debug_gc_check (p->inherits[e].name, " as inherit name");
#endif

      if(e && p->inherits[e].prog)
	debug_gc_check (p->inherits[e].prog, " as inherited program of a program");
    }

#if defined (PIKE_DEBUG) || defined (DO_PIKE_CLEANUP)
    if (gc_keep_markers || Pike_in_gc == GC_PASS_LOCATE)
    {
      for(e = p->num_strings - 1; e >= 0; e--)
	debug_gc_check (p->strings[e], " in the string storage of a program");
      for(e = p->num_identifiers - 1; e >= 0; e--)
	debug_gc_check (p->identifiers[e].name,
			" as identifier name in a program");
    }

    for(e = p->num_identifiers - 1; e >= 0; e--)
      debug_gc_check (p->identifiers[e].type,
		      " as identifier type in a program");
#endif

  } GC_LEAVE;
}

unsigned gc_touch_all_programs(void)
{
  unsigned n = 0;
  struct program *p;
  if (first_program && first_program->prev)
    Pike_fatal("Error in program link list.\n");
  for (p = first_program; p; p = p->next) {
    debug_gc_touch(p);
    n++;
    if (p->next && p->next->prev != p)
      Pike_fatal("Error in program link list.\n");
  }
  return n;
}

void gc_check_all_programs(void)
{
  struct program *p;
  for(p=first_program;p;p=p->next) gc_check_program(p);
}

void gc_mark_all_programs(void)
{
  gc_mark_program_pos = gc_internal_program;
  while (gc_mark_program_pos) {
    struct program *p = gc_mark_program_pos;
    gc_mark_program_pos = p->next;
    if(gc_is_referenced(p)) {
      gc_mark_program_as_referenced(debug_malloc_pass(p));
    }
  }
}

void gc_cycle_check_all_programs(void)
{
  struct program *p;
  for (p = gc_internal_program; p; p = p->next) {
    real_gc_cycle_check_program(p, 0);
    gc_cycle_run_queue();
  }
}

void gc_zap_ext_weak_refs_in_programs(void)
{
  gc_mark_program_pos = first_program;
  while (gc_mark_program_pos != gc_internal_program && gc_ext_weak_refs) {
    struct program *p = gc_mark_program_pos;
    gc_mark_program_pos = p->next;
    gc_mark_program_as_referenced(p);
  }
  gc_mark_discard_queue();
}

size_t gc_free_all_unreferenced_programs(void)
{
  struct program *p,*next;
  size_t unreferenced = 0;
#ifdef PIKE_DEBUG
  int first = 1;
#endif

  for(p=gc_internal_program;p;p=next)
  {
    debug_malloc_touch(p);

    if(gc_do_free(p))
    {
      /* Got an extra ref from gc_cycle_pop_object(). */
      int e;
      if(p->parent)
      {
	free_program(p->parent);
	p->parent=0;
      }
      for(e=0;e<p->num_constants;e++)
      {
	free_svalue(& p->constants[e].sval);
	SET_SVAL(p->constants[e].sval, T_INT, NUMBER_NUMBER, integer, 0);
      }

      for(e=0;e<p->num_inherits;e++)
      {
	if(p->inherits[e].parent)
	{
	  free_object(p->inherits[e].parent);
	  p->inherits[e].parent=0;
	}
	if(e && p->inherits[e].prog)
	{
	  free_program(p->inherits[e].prog);
	  p->inherits[e].prog=0;
	}
      }

      gc_free_extra_ref(p);
      SET_NEXT_AND_FREE(p, free_program);
#ifdef PIKE_DEBUG
      if (first) gc_internal_program = next;
#endif
    }else{
      next=p->next;
#ifdef PIKE_DEBUG
      first = 0;
#endif
    }
    unreferenced++;
  }

#ifdef PIKE_DEBUG
  if (gc_debug)
    for (p = first_program; p != gc_internal_program; p = p->next) {
      int e,tmp=0;
      if (!p)
	Pike_fatal("gc_internal_program was bogus.\n");
      for(e=0;e<p->num_constants;e++)
      {
	if(TYPEOF(p->constants[e].sval) == T_PROGRAM &&
	   p->constants[e].sval.u.program == p)
	  tmp++;
      }
      if(tmp >= p->refs)
	gc_fatal(p, 1 ,"garbage collector failed to free program!!!\n");
    }
#endif

  return unreferenced;
}

PMOD_EXPORT void *get_inherit_storage(struct object *o, int inherit)
{
  if (!o || !o->prog) return NULL;
#ifdef PIKE_DEBUG
  if ((inherit < 0) || (inherit >= o->prog->num_inherits))
    Pike_fatal("Inherit #%d out of range [0..%d]\n",
	       inherit, o->prog->num_inherits-1);
#endif
  return o->storage + o->prog->inherits[inherit].storage_offset;
}

#define GET_STORAGE_CACHE_SIZE 1024
static struct get_storage_cache
{
  INT32 oid, pid;
  ptrdiff_t offset;
} get_storage_cache[GET_STORAGE_CACHE_SIZE];

PMOD_EXPORT ptrdiff_t low_get_storage(struct program *o, struct program *p)
{
  INT32 oid, pid;
  ptrdiff_t offset;
  unsigned INT32 hval;

  if(!o) return -1;
  oid=o->id;
  pid=p->id;
  hval=(unsigned)oid*9248339 + (unsigned)pid;
  hval&=GET_STORAGE_CACHE_SIZE-1;
  if(get_storage_cache[hval].oid == oid &&
     get_storage_cache[hval].pid == pid)
  {
    offset=get_storage_cache[hval].offset;
  }else{
    INT32 e;
    offset=-1;
    for(e=0;e<o->num_inherits;e++)
    {
      if(o->inherits[e].prog==p)
      {
	offset=o->inherits[e].storage_offset;
	break;
      }
    }

    get_storage_cache[hval].oid=oid;
    get_storage_cache[hval].pid=pid;
    get_storage_cache[hval].offset=offset;
  }

  return offset;
}

PMOD_EXPORT void *get_storage(struct object *o, struct program *p)
{
  ptrdiff_t offset;

#ifdef _REENTRANT
  if(d_flag) CHECK_INTERPRETER_LOCK();
#endif

  offset= low_get_storage(o->prog, p);
  if(offset == -1) return 0;
  return o->storage + offset;
}

PMOD_EXPORT struct program *low_program_from_function(struct object *o, INT32 i)
{
  struct svalue *f;
  struct program *p;
  struct identifier *id;
  while(1) {
    struct external_variable_context loc;
    p = o->prog;

    if(!p) return 0;

    id = ID_FROM_INT(p, i);
    if(!IDENTIFIER_IS_ALIAS(id->identifier_flags)) break;

    loc.o = o;
    loc.inherit = INHERIT_FROM_INT(p, i);
    loc.parent_identifier = i;
    find_external_context(&loc, id->func.ext_ref.depth);
    i = id->func.ext_ref.id + loc.inherit->identifier_level;
    o = loc.o;
  }
  if(!IDENTIFIER_IS_CONSTANT(id->identifier_flags)) return 0;
  if(id->func.const_info.offset==-1) return 0;
  f = &PROG_FROM_INT(p,i)->constants[id->func.const_info.offset].sval;
  if(TYPEOF(*f) != T_PROGRAM) return 0;
  return f->u.program;
}

PMOD_EXPORT struct program *program_from_function(const struct svalue *f)
{
  if(TYPEOF(*f) != T_FUNCTION) return 0;
  if(SUBTYPEOF(*f) == FUNCTION_BUILTIN) return 0;

  return low_program_from_function(f->u.object, SUBTYPEOF(*f));
}

PMOD_EXPORT struct program *program_from_type(const struct pike_type *t)
{
  if (!t) return NULL;

  switch(t->type) {
  case T_OBJECT:
    if (t->cdr) break;
    return NULL;
  case T_TUPLE:
  case T_OR:
  case T_AND:
  case PIKE_T_RING:
    {
      struct program *res;
      res = program_from_type(t->car);
      if (res) return res;
    }
    /* FALLTHRU */
  case T_SCOPE:
  case T_ASSIGN:
  case PIKE_T_ATTRIBUTE:
  case PIKE_T_NAME:
    return program_from_type(t->cdr);
    break;
  default:
    return NULL;
  }

  return id_to_program((int)(ptrdiff_t)t->cdr);
}

/* NOTE: Does not add references to the return value! */
PMOD_EXPORT struct program *low_program_from_svalue(const struct svalue *s,
						    struct object **parent_obj,
						    int *parent_id)
{
  switch(TYPEOF(*s))
  {
    case T_OBJECT:
    {
      struct program *p = s->u.object->prog;
#if 0
      int call_fun;
#endif

      if (!p) return 0;

#if 0
      p = p->inherits[SUBTYPEOF(*s)].prog;
      if ((call_fun = FIND_LFUN(p, LFUN_CALL)) >= 0) {
	/* Get the program from the return type. */
	struct identifier *id = ID_FROM_INT(p, call_fun);
	/* FIXME: do it. */
	return 0;
      }
#endif
      push_svalue(s);
      o_cast(program_type_string, T_PROGRAM);
      p = low_program_from_svalue(Pike_sp-1, parent_obj, parent_id);
      pop_stack();
      return p; /* We trust that there is a reference somewhere... */
    }

  case T_FUNCTION:
    if (SUBTYPEOF(*s) == FUNCTION_BUILTIN) return 0;
    return low_program_from_function(*parent_obj = s->u.object,
				     *parent_id = SUBTYPEOF(*s));

  case T_PROGRAM:
    return s->u.program;

  case PIKE_T_TYPE:
    return program_from_type(s->u.type);

  default:
    return 0;
  }
}

/* NOTE: Does not add references to the return value! */
PMOD_EXPORT struct program *program_from_svalue(const struct svalue *s)
{
  struct object *parent_obj = NULL;
  int parent_id = -1;
  return low_program_from_svalue(s, &parent_obj, &parent_id);
}

#define FIND_CHILD_HASHSIZE 5003
struct find_child_cache_s
{
  INT32 pid,cid,id;
};

#if 0
static struct find_child_cache_s find_child_cache[FIND_CHILD_HASHSIZE];

int find_child(struct program *parent, struct program *child)
{
  unsigned INT32 h=(parent->id  * 9248339 + child->id);
  h= h % FIND_CHILD_HASHSIZE;
#ifdef PIKE_DEBUG
  if(h>=FIND_CHILD_HASHSIZE)
    Pike_fatal("find_child failed to hash within boundaries.\n");
#endif
  if(find_child_cache[h].pid == parent->id &&
     find_child_cache[h].cid == child->id)
  {
    return find_child_cache[h].id;
  }else{
    INT32 i;
    for(i=0;i<parent->num_identifier_references;i++)
    {
      if(low_program_from_function(parent, i)==child)
      {
	find_child_cache[h].pid=parent->id;
	find_child_cache[h].cid=child->id;
	find_child_cache[h].id=i;
	return i;
      }
    }
  }
  return -1;
}
#endif /* 0 */



/**
 * @return Returns 1 if a implements b.
 */
static int low_implements(struct program *a, struct program *b)
{
  DECLARE_CYCLIC();
  int e;
  int ret = 1;

  if (BEGIN_CYCLIC(a, b)) {
    END_CYCLIC();
    return 1;	/* Tentatively ok, */
  }
  SET_CYCLIC_RET(1);

  for(e=0;e<b->num_identifier_references;e++)
  {
    struct identifier *bid;
    int i;
    if (b->identifier_references[e].id_flags & (ID_PROTECTED|ID_HIDDEN|ID_VARIANT))
      continue;		/* Skip protected & hidden */
    bid = ID_FROM_INT(b,e);
    if(lfun_strings[LFUN___INIT] == bid->name) continue; /* Skip __INIT */
    i = find_shared_string_identifier(bid->name,a);
    if (i == -1) {
      if (b->identifier_references[e].id_flags & (ID_OPTIONAL))
	continue;		/* It's ok... */
#if 0
      fprintf(stderr, "Missing identifier \"%s\"\n", bid->name->str);
#endif /* 0 */
      ret = 0;
      break;
    }

    if (!pike_types_le(bid->type, ID_FROM_INT(a, i)->type)) {
      if(!match_types(ID_FROM_INT(a,i)->type, bid->type)) {
#if 0
	fprintf(stderr, "Identifier \"%s\" is incompatible.\n",
		bid->name->str);
#endif /* 0 */
	ret = 0;
	break;
      } else {
#if 0
	fprintf(stderr, "Identifier \"%s\" is not strictly compatible.\n",
		bid->name->str);
#endif /* 0 */
      }
    }
  }

  END_CYCLIC();
  return ret;
}

#define IMPLEMENTS_CACHE_SIZE 1024
struct implements_cache_s { INT32 aid, bid, ret; };
static struct implements_cache_s implements_cache[IMPLEMENTS_CACHE_SIZE];

static int implements_hval( INT32 aid, INT32 bid )
{
    return ((aid<<4) ^ bid ^ (aid>>4)) & (IMPLEMENTS_CACHE_SIZE-1);
}

/**
 * @return Returns 1 if a implements b, but faster.
 */
PMOD_EXPORT int implements(struct program *a, struct program *b)
{
  unsigned long hval;
  if(!a || !b) return -1;
  if(a==b) return 1;

  hval = implements_hval(a->id,b->id);
  if(implements_cache[hval].aid==a->id && implements_cache[hval].bid==b->id)
  {
    return implements_cache[hval].ret;
  }
  /* Do it the tedious way */
  implements_cache[hval].aid=a->id;
  implements_cache[hval].bid=b->id;
  implements_cache[hval].ret = 1;	/* Tentatively compatible. */
  implements_cache[hval].ret = low_implements(a,b);
  /* NOTE: If low_implements() returns 0, the cache may have received
   *       some false positives. Those should be cleared.
   */
  return implements_cache[hval].ret;
}

/**
 * @return Returns 1 if a is compatible with b.
 */
static int low_is_compatible(struct program *a, struct program *b)
{
  DECLARE_CYCLIC();
  int e;
  int ret = 1;

  if (BEGIN_CYCLIC(a, b)) {
    END_CYCLIC();
    return 1;
  }
  SET_CYCLIC_RET(1);

  /* Optimize the loop somewhat */
  if (a->num_identifier_references < b->num_identifier_references) {
    struct program *tmp = a;
    a = b;
    b = tmp;
  }

  for(e=0;e<b->num_identifier_references;e++)
  {
    struct identifier *bid;
    int i;
    if (b->identifier_references[e].id_flags & (ID_PROTECTED|ID_HIDDEN|ID_VARIANT))
      continue;		/* Skip protected & hidden */

    /* FIXME: What if they aren't protected & hidden in a? */

    bid = ID_FROM_INT(b,e);
    if(lfun_strings[LFUN___INIT] == bid->name) continue; /* Skip __INIT */
    i = find_shared_string_identifier(bid->name,a);
    if (i == -1) {
      continue;		/* It's ok... */
    }

    /* Note: Uses weaker check for constant integers. */
    if(((bid->run_time_type != PIKE_T_INT) ||
	(ID_FROM_INT(a, i)->run_time_type != PIKE_T_INT)) &&
       !match_types(ID_FROM_INT(a,i)->type, bid->type)) {
#if 0
      fprintf(stderr, "Identifier \"%s\" is incompatible.\n",
	      bid->name->str);
#endif /* 0 */
      ret = 0;
      break;
    }
  }

  END_CYCLIC();
  return ret;
}

static struct implements_cache_s is_compatible_cache[IMPLEMENTS_CACHE_SIZE];
/**
 * Returns 1 if a is compatible with b.
 *
 * ie it's possible to write a hypothetical c that implements both.
 */
PMOD_EXPORT int is_compatible(struct program *a, struct program *b)
{
  unsigned long hval;
  unsigned long rhval;
  int aid, bid;
  if(!a || !b) return -1;
  if(a==b) return 1;

  /* Order the id's so we don't need double entries in the cache. */
  aid = a->id;
  bid = b->id;
  if (aid > bid) {
    int tmp = aid;
    aid = bid;
    bid = tmp;
  }

  hval = implements_hval(aid,bid);
  if(is_compatible_cache[hval].aid==aid &&
     is_compatible_cache[hval].bid==bid)
  {
    return is_compatible_cache[hval].ret;
  }
  if(implements_cache[hval].aid==aid &&
     implements_cache[hval].bid==bid &&
     implements_cache[hval].ret)
  {
    /* a implements b */
    return 1;
  }
  rhval = implements_hval(bid,aid);
  if(implements_cache[rhval].aid==bid &&
     implements_cache[rhval].bid==aid &&
     implements_cache[rhval].ret)
  {
    /* b implements a */
    return 1;
  }
  /* Do it the tedious way */
  is_compatible_cache[hval].aid=aid;
  is_compatible_cache[hval].bid=bid;
  is_compatible_cache[hval].ret = 1;	/* Tentatively compatible. */
  is_compatible_cache[hval].ret = low_is_compatible(a,b);
  /* NOTE: If low_is compatible() returns 0, the cache may have received
   *       some false positives. Those should be cleared.
   */
  return is_compatible_cache[hval].ret;
}

/**
 * Explains why a is not compatible with b.
 */
void yyexplain_not_compatible(int severity_level,
			      struct program *a, struct program *b)
{
  int e;
  int res = 1;
  INT_TYPE a_line = 0;
  INT_TYPE b_line = 0;
  struct pike_string *a_file;
  struct pike_string *b_file;
  DECLARE_CYCLIC();

  /* Optimize the loop somewhat */
  if (a->num_identifier_references < b->num_identifier_references) {
    struct program *tmp = a;
    a = b;
    b = tmp;
  }

  if (BEGIN_CYCLIC(a, b)) {
    END_CYCLIC();
    return;
  }
  SET_CYCLIC_RET(1);

  a_file = get_program_line(a, &a_line);
  b_file = get_program_line(b, &b_line);

  for(e=0;e<b->num_identifier_references;e++)
  {
    struct identifier *bid;
    int i;
    if (b->identifier_references[e].id_flags & (ID_PROTECTED|ID_HIDDEN|ID_VARIANT))
      continue;		/* Skip protected & hidden */

    /* FIXME: What if they aren't protected & hidden in a? */

    bid = ID_FROM_INT(b,e);
    if(lfun_strings[LFUN___INIT] == bid->name) continue; /* Skip __INIT */
    i = find_shared_string_identifier(bid->name,a);
    if (i == -1) {
      continue;		/* It's ok... */
    }

    /* Note: Uses weaker check for constant integers. */
    if(((bid->run_time_type != PIKE_T_INT) ||
	(ID_FROM_INT(a, i)->run_time_type != PIKE_T_INT)) &&
       !match_types(ID_FROM_INT(a,i)->type, bid->type)) {
      INT_TYPE aid_line = a_line;
      INT_TYPE bid_line = b_line;
      struct pike_string *aid_file = get_identifier_line(a, i, &aid_line);
      struct pike_string *bid_file = get_identifier_line(b, e, &bid_line);
      if (!aid_file) aid_file = a_file;
      if (!bid_file) bid_file = b_file;
      ref_push_string(bid->name);
      ref_push_program(a);
      ref_push_program(b);
      yytype_report(severity_level,
		    aid_file, aid_line, ID_FROM_INT(a, i)->type,
		    bid_file, bid_line, bid->type, 3,
		    "Identifier %s in %O is incompatible with the same in %O.");
    }
  }
  free_string(b_file);
  free_string(a_file);
  END_CYCLIC();
  return;
}

/**
 * Explains why a does not implement b.
 */
void yyexplain_not_implements(int severity_level,
			      struct program *a, struct program *b)
{
  int e;
  INT_TYPE a_line = 0;
  INT_TYPE b_line = 0;
  struct pike_string *a_file;
  struct pike_string *b_file;
  DECLARE_CYCLIC();

  if (BEGIN_CYCLIC(a, b)) {
    END_CYCLIC();
    return;
  }
  SET_CYCLIC_RET(1);

  a_file = get_program_line(a, &a_line);
  b_file = get_program_line(b, &b_line);

  for(e=0;e<b->num_identifier_references;e++)
  {
    struct identifier *bid;
    int i;
    if (b->identifier_references[e].id_flags & (ID_PROTECTED|ID_HIDDEN|ID_VARIANT))
      continue;		/* Skip protected & hidden */
    bid = ID_FROM_INT(b,e);
    if(lfun_strings[LFUN___INIT] == bid->name) continue; /* Skip __INIT */
    i = find_shared_string_identifier(bid->name,a);
    if (i == -1) {
      INT_TYPE bid_line = b_line;
      struct pike_string *bid_file;
      if (b->identifier_references[e].id_flags & (ID_OPTIONAL))
	continue;		/* It's ok... */
      bid_file = get_identifier_line(b, e, &bid_line);
      if (!bid_file) bid_file = b_file;
      yytype_report(severity_level,
		    bid_file, bid_line, bid->type,
		    a_file, a_line, NULL,
		    0, "Missing identifier %S.", bid->name);
      continue;
    }

    if (!pike_types_le(bid->type, ID_FROM_INT(a, i)->type)) {
      INT_TYPE aid_line = a_line;
      INT_TYPE bid_line = b_line;
      struct pike_string *aid_file = get_identifier_line(a, i, &aid_line);
      struct pike_string *bid_file = get_identifier_line(b, e, &bid_line);
      if (!aid_file) aid_file = a_file;
      if (!bid_file) bid_file = b_file;
      if(!match_types(ID_FROM_INT(a,i)->type, bid->type)) {
	yytype_report(severity_level,
		      bid_file, bid_line, bid->type,
		      aid_file, aid_line, ID_FROM_INT(a, i)->type,
		      0, "Type of identifier %S does not match.", bid->name);
      } else {
	yytype_report((severity_level < REPORT_WARNING)?
		      severity_level : REPORT_WARNING,
		      bid_file, bid_line, bid->type,
		      aid_file, aid_line, ID_FROM_INT(a, i)->type,
		      0, "Type of identifier %S is not strictly compatible.",
		      bid->name);
      }
      continue;
    }
  }
  free_string(b_file);
  free_string(a_file);
  END_CYCLIC();
}

/* FIXME: Code duplication of yyexplain_not_compatible() above! */
/**
 * Explains why a is not compatible with b.
 */
void string_builder_explain_not_compatible(struct string_builder *s,
					   struct program *a,
					   struct program *b)
{
  int e;
  int res = 1;
  DECLARE_CYCLIC();

  /* Optimize the loop somewhat */
  if (a->num_identifier_references < b->num_identifier_references) {
    struct program *tmp = a;
    a = b;
    b = tmp;
  }

  if (BEGIN_CYCLIC(a, b)) {
    END_CYCLIC();
    return;
  }
  SET_CYCLIC_RET(1);

  for(e=0;e<b->num_identifier_references;e++)
  {
    struct identifier *bid;
    int i;
    if (b->identifier_references[e].id_flags & (ID_PROTECTED|ID_HIDDEN|ID_VARIANT))
      continue;		/* Skip protected & hidden */

    /* FIXME: What if they aren't protected & hidden in a? */

    bid = ID_FROM_INT(b,e);
    if(lfun_strings[LFUN___INIT] == bid->name) continue; /* Skip __INIT */
    i = find_shared_string_identifier(bid->name,a);
    if (i == -1) {
      continue;		/* It's ok... */
    }

    /* Note: Uses weaker check for constant integers. */
    if(((bid->run_time_type != PIKE_T_INT) ||
	(ID_FROM_INT(a, i)->run_time_type != PIKE_T_INT)) &&
       !match_types(ID_FROM_INT(a,i)->type, bid->type)) {
      ref_push_program(a);
      ref_push_program(b);
      ref_push_type_value(ID_FROM_INT(a, i)->type);
      ref_push_type_value(bid->type);
      string_builder_sprintf(s,
			     "Identifier %S in %O is incompatible with "
			     "the same in %O.\n"
			     "Expected: %O\n"
			     "Got     : %O\n",
			     bid->name, Pike_sp-4,
			     Pike_sp-3,
			     Pike_sp-2,
			     Pike_sp-1);
      pop_n_elems(4);
    }
  }
  END_CYCLIC();
  return;
}

/* FIXME: code duplication of yyexplain_not_implements() above! */
/**
 * Explains why a does not implement b.
 */
void string_builder_explain_not_implements(struct string_builder *s,
					   struct program *a,
					   struct program *b)
{
  int e;
  DECLARE_CYCLIC();

  if (BEGIN_CYCLIC(a, b)) {
    END_CYCLIC();
    return;
  }
  SET_CYCLIC_RET(1);

  for(e=0;e<b->num_identifier_references;e++)
  {
    struct identifier *bid;
    int i;
    if (b->identifier_references[e].id_flags & (ID_PROTECTED|ID_HIDDEN|ID_VARIANT))
      continue;		/* Skip protected & hidden */
    bid = ID_FROM_INT(b,e);
    if(lfun_strings[LFUN___INIT] == bid->name) continue; /* Skip __INIT */
    i = find_shared_string_identifier(bid->name,a);
    if (i == -1) {
      if (b->identifier_references[e].id_flags & (ID_OPTIONAL))
	continue;		/* It's ok... */
      ref_push_type_value(bid->type);
      string_builder_sprintf(s,
			     "Missing identifier %O %S.\n",
			     Pike_sp-1, bid->name);
      pop_stack();
      continue;
    }

    if (!pike_types_le(bid->type, ID_FROM_INT(a, i)->type)) {
      ref_push_type_value(bid->type);
      ref_push_type_value(ID_FROM_INT(a, i)->type);
      if(!match_types(ID_FROM_INT(a,i)->type, bid->type)) {
	string_builder_sprintf(s,
			       "Type of identifier %S does not match.\n"
			       "Expected: %O.\n"
			       "Got     : %O.\n",
			       bid->name,
			       Pike_sp-2,
			       Pike_sp-1);
      } else {
	string_builder_sprintf(s,
			       "Type of identifier %S is not strictly compatible."
			       "Expected: %O.\n"
			       "Got     : %O.\n",
			       bid->name,
			       Pike_sp-2,
			       Pike_sp-1);
      }
      pop_n_elems(2);
      continue;
    }
  }
  END_CYCLIC();
}

PMOD_EXPORT void *parent_storage(int depth, struct program *expected)
{
  struct external_variable_context loc;
  int i;

  loc.o = Pike_fp->current_object;
  loc.parent_identifier = 0;
  loc.inherit = Pike_fp->context;

  find_external_context(&loc, depth);

  if (!loc.o->prog)
    Pike_error ("Cannot access storage of destructed parent object.\n");

  for (i = 0; i < loc.inherit->prog->num_inherits; i++) {
    if (loc.inherit[i].prog == expected) {
      /* Found. */
      loc.inherit += i;
      break;
    }
  }

#ifdef PIKE_DEBUG
  if (loc.inherit->prog != expected) {
    Pike_fatal("Failed to find expected parent storage.\n");
  }
#endif

  return loc.o->storage + loc.inherit->storage_offset;
}

PMOD_EXPORT void *get_inherited_storage(int inh,
                                        struct program *DEBUGUSED(expected))
{
  struct inherit *i = Pike_fp->context + inh;

#ifdef PIKE_DEBUG
  struct program *p = Pike_fp->current_program;
  if (i >= (p->inherits + p->num_inherits)) {
    Pike_fatal("Inherit out of range!\n");
  }
  if (i->prog != expected) {
    Pike_fatal("Unexpected program at inherit #%d!\n", inh);
  }
#endif

  return Pike_fp->current_object->storage + i->storage_offset;
}

#ifdef PIKE_USE_MACHINE_CODE

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#if defined(HAVE_SYNC_INSTRUCTION_MEMORY) || defined(FLUSH_INSTRUCTION_CACHE) || !defined(USE_MY_MEXEC_ALLOC)
void make_area_executable (char *start, size_t len)
{
#ifndef USE_MY_MEXEC_ALLOC
  {
    /* Perform page alignment. */
    void *addr = (void *)(((size_t)start) & ~(page_size-1));
    size_t l = ((start + len) - ((char *)addr) +
		(page_size - 1)) & ~(page_size-1);

    if (mprotect(addr, l, PROT_EXEC | PROT_READ | PROT_WRITE) < 0) {
#if 0
      fprintf(stderr, "%p:%d: mprotect(%p, %lu, 0x%04x): errno: %d\n",
	      start, len, addr, l, PROT_EXEC | PROT_READ | PROT_WRITE, errno);
#endif /* 0 */
    }
  }
#endif /* !USE_MY_MEXEC_ALLOC */

#ifdef HAVE_SYNC_INSTRUCTION_MEMORY
  sync_instruction_memory(start, len);
#elif defined(FLUSH_INSTRUCTION_CACHE)
  FLUSH_INSTRUCTION_CACHE(start, len);
#endif /* HAVE_SYNC_INSTRUCTION_MEMORY || FLUSH_INSTRUCTION_CACHE */
}
#else
void make_area_executable (char *UNUSED(start), size_t UNUSED(len))
{
}
#endif

void make_program_executable(struct program *p)
{
  if (!p->num_program) return;
  if ((p->event_handler == compat_event_handler) &&
      ((p->num_program * sizeof(p->program[0]) <=
	(NUM_PROG_EVENTS * sizeof(p->event_handler))))) {
    /* Only event handlers. */
    return;
  }

  make_area_executable ((char *) p->program,
			p->num_program * sizeof (p->program[0]));
}
#endif

PMOD_EXPORT void string_builder_append_disassembly(struct string_builder *s,
						   const PIKE_OPCODE_T *start,
						   const PIKE_OPCODE_T *end,
						   const char *opcode,
						   const char **params,
						   const char *comment)
{
  while ((start < end) || opcode || (params && params[0]) ||
	 (comment && comment[0])) {
    ptrdiff_t field_width = sizeof(PIKE_OPCODE_T) * 2;
    ptrdiff_t i;
    int skip_params = 0;
    int skip_comment = 0;

    if (end) {
      /* Address */
      string_builder_sprintf(s, "0x%016lx  ", start);

      if (start < end) {
	/* Memory dump */
	for (i = 0; i < 8; i += field_width) {
	  if (start < end) {
	    string_builder_sprintf(s, "%0*x ", field_width, start[0]);
	    start++;
	    if (start == end) {
	      end = NULL;
	    }
	  } else {
	    string_builder_sprintf(s, "%*s ", field_width, "");
	  }
	}
      } else {
	end = NULL;
	string_builder_sprintf(s, "%*s  ", 8 + 8/field_width, "");
      }
    } else {
      string_builder_sprintf(s, "%*s  ", 18 + 8 + 8/field_width, "");
    }

    /* Opcode */
    if (opcode) {
      if (strlen(opcode) < 8) {
	string_builder_sprintf(s, " %-8s ", opcode);
      } else if (strlen(opcode) < 32) {
	string_builder_sprintf(s, " %-28s ", opcode);
	skip_params = 1;
      } else {
	string_builder_sprintf(s, " %s", opcode);
	skip_params = skip_comment = 1;
      }
      opcode = NULL;
    } else if ((params && params[0]) || (comment && comment[0])) {
      /* No need to pad if there's no argument and no comment. */
      string_builder_sprintf(s, " %8s ", "");
    }

    /* Params */
    if (skip_params) {
    } else if (params && params[0]) {
      ptrdiff_t bytes_left = 20;
      do {
	string_builder_sprintf(s, "%s", params[0]);
	bytes_left -= strlen(params[0]);
	params++;
	if (params[0]) {
	  string_builder_sprintf(s, ", ");
	  bytes_left -= 2;
	}
      } while (params[0] && (((ptrdiff_t)strlen(params[0])) <= bytes_left));
      if (bytes_left < 0) {
	skip_comment = 1;
      } else if (comment && comment[0]) {
	/* No need to pad if there's no comment. */
	string_builder_sprintf(s, "%*s ", bytes_left-1, "");
      }
    } else if (comment && comment[0]) {
      /* No need to pad if there's no comment. */
      string_builder_sprintf(s, "%*s ", 19, "");
    }

    /* Comment */
    if (!skip_comment && (comment && comment[0])) {
      const char *ptr = strchr(comment, '\n');
      if (ptr) {
	string_builder_sprintf(s, " # %.*s\n", ptr - comment, comment);
	comment = ptr + 1;
	if (!comment[0]) comment = NULL;
      } else {
	string_builder_sprintf(s, " # %s\n", comment);
	comment = NULL;
      }
    } else {
      string_builder_sprintf(s, "\n");
    }
  }
}

PMOD_EXPORT void string_builder_append_pike_opcode(struct string_builder *s,
						   const PIKE_OPCODE_T *addr,
						   enum Pike_opcodes op,
						   int arg1,
						   int arg2)
{
  char buf[3][32];
  const char *params[3] = { NULL, NULL, NULL };
  const struct instr *instr = &instrs[op - F_OFFSET];
  sprintf(buf[0], "%d", arg1);
  sprintf(buf[1], "%d", arg2);
  if (instr->flags & I_HASARG) {
    params[0] = buf[0];
  }
  if (instr->flags & I_HASARG2) {
    params[1] = buf[1];
  }
  sprintf(buf[2], "# %s", instr->name);
  string_builder_append_disassembly(s, addr, addr, buf[2], params, NULL);
}

PMOD_EXPORT void add_reverse_symbol(struct pike_string *sym, void *addr)
{
  struct svalue key;
  struct svalue val;
  SET_SVAL(key, PIKE_T_INT, NUMBER_NUMBER, integer, (ptrdiff_t)addr);
  SET_SVAL(val, PIKE_T_STRING, 0, string, sym);
  low_mapping_insert(reverse_symbol_table, &key, &val, 1);
}

PMOD_EXPORT void simple_add_reverse_symbol(const char *sym, void *addr)
{
  struct pike_string *s = make_shared_string(sym);
  add_reverse_symbol(s, addr);
  free_string(s);
}

PMOD_EXPORT void init_reverse_symbol_table()
{
  int op;

  if (reverse_symbol_table) return;
  reverse_symbol_table = allocate_mapping(256);

  /* Initialize with the most popular symbols. */
#define ADD_SYMBOL(SYM)	simple_add_reverse_symbol(#SYM "()", (void *)(SYM))
#if defined(PIKE_DEBUG) && defined(PIKE_USE_MACHINE_CODE)
  ADD_SYMBOL(simple_debug_instr_prologue_0);
  ADD_SYMBOL(simple_debug_instr_prologue_1);
  ADD_SYMBOL(simple_debug_instr_prologue_2);
#endif
  ADD_SYMBOL(low_return);
  ADD_SYMBOL(low_return_pop);
  ADD_SYMBOL(really_free_svalue);
  ADD_SYMBOL(lvalue_to_svalue_no_free);
  ADD_SYMBOL(f_add);
  ADD_SYMBOL(really_free_string);
  ADD_SYMBOL(pike_sizeof);
  ADD_SYMBOL(o_subtract);
  ADD_SYMBOL(svalue_is_true);
  ADD_SYMBOL(assign_lvalue);
  ADD_SYMBOL(really_free_array);
  ADD_SYMBOL(object_low_set_index);
  ADD_SYMBOL(low_object_index_no_free);
  ADD_SYMBOL(assign_to_short_svalue);
  ADD_SYMBOL(really_free_short_svalue_ptr);
  ADD_SYMBOL(mega_apply);

  simple_add_reverse_symbol("Pike_interpreter_pointer",
			    (void *)&Pike_interpreter_pointer);

#ifdef PIKE_USE_MACHINE_CODE
  /* Add the opcodes as well. */
  for (op = 1; op < (F_MAX_INSTR - F_OFFSET); op++) {
    if (!instrs[op].address) continue;
    simple_add_reverse_symbol(instrs[op].name, instrs[op].address);
  }
#endif /* PIKE_USE_MACHINE_CODE */
}

PMOD_EXPORT struct pike_string *reverse_symbol_lookup(void *addr)
{
  struct svalue key;
  struct svalue *s;
  if (!reverse_symbol_table) {
    init_reverse_symbol_table();
  }
  SET_SVAL(key, PIKE_T_INT, NUMBER_NUMBER, integer, (ptrdiff_t)addr);
  s = low_mapping_lookup(reverse_symbol_table, &key);

  /* FIXME: Fall back to using dladdr() on supported OSes? */
  if (!s || (TYPEOF(*s) != PIKE_T_STRING)) return NULL;
  return s->u.string;
}
