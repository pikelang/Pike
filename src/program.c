/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: program.c,v 1.569 2004/09/27 15:12:15 grubba Exp $
*/

#include "global.h"
#include "program.h"
#include "object.h"
#include "dynamic_buffer.h"
#include "pike_types.h"
#include "stralloc.h"
#include "las.h"
#include "lex.h"
#include "pike_macros.h"
#include "fsort.h"
#include "pike_error.h"
#include "docode.h"
#include "interpret.h"
#include "hashtable.h"
#include "main.h"
#include "pike_memory.h"
#include "gc.h"
#include "threads.h"
#include "constants.h"
#include "operators.h"
#include "builtin_functions.h"
#include "stuff.h"
#include "mapping.h"
#include "cyclic.h"
#include "pike_security.h"
#include "pike_types.h"
#include "opcodes.h"
#include "version.h"
#include "block_alloc.h"
#include "pikecode.h"

#include <errno.h>
#include <fcntl.h>

#define sp Pike_sp

#undef ATTRIBUTE
#define ATTRIBUTE(X)

static void exit_program_struct(struct program *);
static size_t add_xstorage(size_t size,
			   size_t alignment,
			   ptrdiff_t modulo_orig);

#undef EXIT_BLOCK
#define EXIT_BLOCK(P) exit_program_struct( (P) )

#undef COUNT_OTHER
#define COUNT_OTHER() do{			\
  struct program *p;				\
  for(p=first_program;p;p=p->next)		\
  {						\
    size+=p->total_size;			\
  }						\
}while(0)

BLOCK_ALLOC_FILL_PAGES(program, 4)


/* #define COMPILER_DEBUG */
/* #define PROGRAM_BUILD_DEBUG */

#ifdef COMPILER_DEBUG
#define CDFPRINTF(X)	fprintf X
#else /* !COMPILER_DEBUG */
#define CDFPRINTF(X)
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
 * - Holistiska Centralbyrån (Hubbe)
 */

/* Define the size of the cache that is used for method lookup. */
/* A value of zero disables this cache */
#define FIND_FUNCTION_HASHSIZE 15013

/* Programs with less methods will not use the cache for method lookups.. */
#define FIND_FUNCTION_HASH_TRESHOLD 9


#define DECLARE
#include "compilation.h"

struct pike_string *this_program_string, *this_string;
static struct pike_string *UNDEFINED_string;

const char *const lfun_names[]  = {
  "__INIT",
  "create",
  "destroy",
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
  /* NOTE: After this point there are only fake lfuns. */
  "_search",
};

struct pike_string *lfun_strings[NELEM(lfun_names)];

static struct mapping *lfun_ids;

/* mapping(string:type) */
static struct mapping *lfun_types;

static char *raw_lfun_types[] = {
  tFuncV(tNone,tVoid,tVoid),	/* "__INIT", */
  tFuncV(tNone,tZero,tVoid),	/* "create", */
  tFuncV(tNone,tVoid,tVoid),	/* "destroy", */
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
  tFuncV(tStr,tVoid,tMix),	/* "`->", */
  tFuncV(tStr tSetvar(0,tZero),tVoid,tVar(0)),	/* "`->=", */
  tFuncV(tNone,tVoid,tInt),	/* "_sizeof", */
  tFuncV(tNone,tVoid,tArray),	/* "_indices", */
  tFuncV(tNone,tVoid,tArray),	/* "_values", */
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
  /* NOTE: After this point there are only fake lfuns. */
  tFuncV(tZero tOr(tZero, tVoid), tVoid, tMix), /* "_search", */
};

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
 *!     @[__INIT()], @[create()], @[destroy()]
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
 *!   Although these functions are called from outside the object
 *!   they exist in, they will still be used even if they are
 *!   declared @expr{static@}. It is in fact recommended to declare
 *!   them @expr{static@}, since that will hinder them being used
 *!   for other purposes.
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
 *!   In Pike 7.2 and later this function can be created implicitly
 *!   by the compiler using the new syntax:
 *! @code
 *! class Foo(int foo) {
 *!   int bar;
 *! }
 *! @endcode
 *!   In the above case an implicit @[lfun::create()] is created, and
 *!   it's equvivalent to:
 *! @code
 *! class Foo {
 *!   int foo;
 *!   int bar;
 *!   static void create(int foo)
 *!   {
 *!     local::foo = foo;
 *!   }
 *! }
 *! @endcode
 *!
 *! @seealso
 *!   @[lfun::__INIT()], @[lfun::destroy()]
 */

/*! @decl void lfun::destroy()
 *!
 *!   Object destruction callback.
 *!   
 *!   This function is called by @[predef::destruct()] right before it
 *!   zeroes all the object variables and destroys the object.
 *!
 *! @note
 *!   Note that it's also called on implicit destruct, i.e. when there
 *!   are no more references to the object, or when the garbage
 *!   collector decides to destruct it.
 *!
 *! @note
 *! Regarding destruction order during garbage collection:
 *! 
 *! If an object is destructed by the garbage collector, it's part of
 *! a reference cycle with other things but with no external
 *! references. If there are other objects with @expr{destroy@}
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
 *!   If an object A contains an @[lfun::destroy] and an object B does
 *!   not, then A is destructed before B.
 *! @item
 *!   If A references B single way, then A is destructed before B.
 *! @item
 *!   If A and B are in a cycle, and there is a reference somewhere
 *!   from B to A that is weaker than any reference from A to B, then
 *!   A is destructed before B.
 *! @item
 *!   Weak references (e.g. set with @[predef::set_weak_flag()]) are considered
 *!   weaker than normal references, and both are considered weaker
 *!   than strong references.
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
 *!     static void destroy() {
 *!       if (!Super::this)
 *!         error ("My parent has been destructed!\n");
 *!     }
 *!   }
 *!   Sub sub = Sub();
 *!   static void destroy() {
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
 *! When the garbage collector calls @[lfun::destroy], all accessible
 *! non-objects and objects without @expr{destroy@} functions are
 *! still intact. They are not freed if the @expr{destroy@} function
 *! adds external references to them. However, all objects with
 *! @[lfun::destroy] in the cycle are already scheduled for
 *! destruction and are thus be destroyed even if external references
 *! are added to them.
 *!
 *! @note
 *! The garbage collector had completely random destruct order in
 *! versions prior to 7.2.
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
 *!   that precedes this object in the argument list of the call to
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
 *!   Equality test callback.
 *!
 *! @note
 *!   If this is implemented it might be necessary to implement
 *!   @[lfun::__hash] too. Otherwise mappings might hold several
 *!   objects as indices which are duplicates according to this
 *!   function. Various other functions that use hashing also might
 *!   not work correctly, e.g. @[predef::Array.uniq].
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
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
 *!   This function gets called by various mapping operations when the
 *!   object is used as index in a mapping. It should return an
 *!   integer that corresponds to the object in such a way that all
 *!   values which @[lfun::`==] considers equal to the object gets the
 *!   same hash value.
 *!
 *! @note
 *!   The function @[predef::hash] does not return hash values that
 *!   are compatible with this one.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[lfun::`==]
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
 *!   Index/subrange callback.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[predef::`[]()]
 */

/*! @decl mixed lfun::`[]=(zero arg1, zero arg2)
 *!
 *!   Index assignment callback.
 *!
 *! @seealso
 *!   @[predef::`[]=()], @[lfun::`->=()]
 */

/*! @decl mixed lfun::`->(string arg)
 *!
 *!   Arrow index callback.
 *!
 *! @note
 *!   It's assumed that this function is side-effect free.
 *!
 *! @seealso
 *!   @[predef::`->()]
 */

/*! @decl mixed lfun::`->=(string arg1, zero arg2)
 *!
 *!   Arrow index assignment callback.
 *!
 *! @seealso
 *!   @[predef::`->=()], @[lfun::`[]=()]
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

/*! @decl array lfun::_indices()
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
 *!   @[predef::indices()], @[lfun::_values()]
 */

/*! @decl array lfun::_values()
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
 *!   @[predef::values()], @[lfun::_indices()]
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
 *! @note
 *!   It's assumed that this function is side-effect free.
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

/*! @decl mixed lfun::_search(mixed needle, mixed|void start)
 *!
 *!   Search callback.
 *!
 *! @seealso
 *!   @[predef::search()]
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

struct program *null_program=0;

struct object *error_handler=0;
struct object *compat_handler=0;

struct program *gc_internal_program = 0;
static struct program *gc_mark_program_pos = 0;

int compilation_depth=-1;
dynamic_buffer used_modules;
static struct mapping *resolve_cache=0;

#ifdef PIKE_DEBUG
#define CHECK_FILE_ENTRY(PROG, POS, LEN, SHIFT)				\
  do {									\
    if (SHIFT < 0 || SHIFT > 2 ||					\
	POS + (LEN << SHIFT) > PROG->linenumbers + PROG->num_linenumbers) \
      Pike_fatal ("Invalid file entry in linenumber info.\n");		\
  } while (0)
#else
#define CHECK_FILE_ENTRY(PROG, POS, LEN, SHIFT) do {} while (0)
#endif

int get_small_number(char **q);

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
    tmp = realloc((void *)state->new_program->NAME,			\
		  sizeof(TYPE) * m);					\
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

void ins_short(int i, void (*func)(char tmp))
{
  int e;
  unsigned char *p = (unsigned char *)&i;
  for(e=0;e<(long)sizeof(i);e++) {
    func(p[e]);
  }
}

#ifdef PIKE_DEBUG
static void debug_add_to_identifiers (struct identifier id)
{
  if (d_flag) {
    int i;
    for (i = 0; i < Pike_compiler->new_program->num_identifiers; i++)
      if (Pike_compiler->new_program->identifiers[i].name == id.name) {
	dump_program_tables (Pike_compiler->new_program, 0);
	Pike_fatal ("Adding identifier twice, old at %d.\n", i);
      }
  }
  add_to_identifiers (id);
}
#else
#define debug_add_to_identifiers(ARG) add_to_identifiers(ARG)
#endif

void add_relocated_int_to_program(INT32 i)
{
  add_to_relocations(Pike_compiler->new_program->num_program);
  ins_int(i, (void (*)(char))add_to_program);
}

void use_module(struct svalue *s)
{
  if( (1<<s->type) & (BIT_MAPPING | BIT_OBJECT | BIT_PROGRAM))
  {
    Pike_compiler->num_used_modules++;
    assign_svalue_no_free((struct svalue *)
			  low_make_buf_space(sizeof(struct svalue),
					     &used_modules), s);
    if(Pike_compiler->module_index_cache)
    {
      free_mapping(Pike_compiler->module_index_cache);
      Pike_compiler->module_index_cache=0;
    }
  }else{
    yyerror("Module is neither mapping nor object");
  }
}

void unuse_modules(INT32 howmany)
{
  if(!howmany) return;
#ifdef PIKE_DEBUG
  if(howmany *sizeof(struct svalue) > used_modules.s.len)
    Pike_fatal("Unusing too many modules.\n");
#endif
  Pike_compiler->num_used_modules-=howmany;
  low_make_buf_space(-sizeof(struct svalue)*howmany, &used_modules);
  free_svalues((struct svalue *)low_make_buf_space(0, &used_modules),
	       howmany,
	       BIT_MAPPING | BIT_OBJECT | BIT_PROGRAM);
  if(Pike_compiler->module_index_cache)
  {
    free_mapping(Pike_compiler->module_index_cache);
    Pike_compiler->module_index_cache=0;
  }
}

int low_find_shared_string_identifier(struct pike_string *name,
				      struct program *prog);



static struct node_s *index_modules(struct pike_string *ident,
				    struct mapping **module_index_cache,
				    int num_used_modules,
				    struct svalue *modules)
{
  if(*module_index_cache)
  {
    struct svalue *tmp=low_mapping_string_lookup(*module_index_cache,ident);
    if(tmp)
    {
      if(!(SAFE_IS_ZERO(tmp) && tmp->subtype==1))
	return mksvaluenode(tmp);
      return 0;
    }
  }

/*  fprintf(stderr,"index_module: %s\n",ident->str); */

  {
    JMP_BUF tmp;

    if(SETJMP(tmp))
    {
      if (!ident->size_shift) {
	handle_compile_exception ("Couldn't index a module with '%s'.", ident->str);
      } else {
	handle_compile_exception ("Couldn't index a module.");
      }
    } else {
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

	  if (Pike_compiler->compiler_pass == 2 &&
	      ((Pike_sp[-1].type == T_OBJECT &&
		Pike_sp[-1].u.object == placeholder_object) ||
	       (Pike_sp[-1].type == T_PROGRAM &&
		Pike_sp[-1].u.program == placeholder_program))) {
	    my_yyerror("Got placeholder %s (resolver problem) "
		       "when indexing a module with %S.",
		       get_name_of_type (Pike_sp[-1].type), ident);
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

/*  fprintf(stderr,"***Undefined.\n"); */

  return 0;
}

/*! @decl constant UNDEFINED
 *!
 *! The undefined value; ie a zero for which @[zero_type()] returns 1.
 */

struct node_s *resolve_identifier(struct pike_string *ident);

struct node_s *find_module_identifier(struct pike_string *ident,
				      int see_inherit)
{
  struct node_s *ret;

  struct svalue *modules=(struct svalue *)
    (used_modules.s.str + used_modules.s.len);

  {
    struct program_state *p=Pike_compiler;
    int n;
    for(n=0;n<=compilation_depth;n++,p=p->previous)
    {
      int i;
      if(see_inherit)
      {
	i=really_low_find_shared_string_identifier(ident,
						   p->new_program,
						   SEE_STATIC|SEE_PRIVATE);
	if(i!=-1)
	{
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
      if( ((char *)modules ) < used_modules.s.str)
	Pike_fatal("Modules out of whack!\n");
#endif
    }
  }

  /* Handle UNDEFINED */
  if (ident == UNDEFINED_string) {
    struct svalue s;
    s.type = T_INT;
    s.subtype = NUMBER_UNDEFINED;
    s.u.integer = 0;
    return mkconstantsvaluenode(&s);
  }

  return resolve_identifier(ident);
}

struct node_s *resolve_identifier(struct pike_string *ident)
{
  if(resolve_cache)
  {
    struct svalue *tmp=low_mapping_string_lookup(resolve_cache,ident);
    if(tmp)
    {
      if(!(SAFE_IS_ZERO(tmp) && tmp->subtype==1))
	return mkconstantsvaluenode(tmp);

      return 0;
    }
  }

  if(get_master())
  {
    DECLARE_CYCLIC();
    node *ret=0;
    if(BEGIN_CYCLIC(ident, lex.current_file))
    {
      my_yyerror("Recursive module dependency in %S.", ident);
    }else{
      SET_CYCLIC_RET(1);

      ref_push_string(ident);
      ref_push_string(lex.current_file);
      if (error_handler) {
	ref_push_object(error_handler);
      } else {
	push_int(0);
      }

      if (safe_apply_handler("resolv", error_handler, compat_handler, 3, 0)) {
	if (Pike_compiler->compiler_pass == 2 &&
	    ((Pike_sp[-1].type == T_OBJECT &&
	      Pike_sp[-1].u.object == placeholder_object) ||
	     (Pike_sp[-1].type == T_PROGRAM &&
	      Pike_sp[-1].u.program == placeholder_program))) {
	  my_yyerror("Got placeholder %s (resolver problem) "
		     "when resolving %S.",
		     get_name_of_type (Pike_sp[-1].type), ident->str);
	}
	else {
	  if(!resolve_cache)
	    resolve_cache=dmalloc_touch(struct mapping *, allocate_mapping(10));
	  mapping_string_insert(resolve_cache,ident,Pike_sp-1);

	  if(!(SAFE_IS_ZERO(Pike_sp-1) && Pike_sp[-1].subtype==1))
	  {
	    ret=mkconstantsvaluenode(Pike_sp-1);
	  }
	}
	pop_stack();
      }
      else
	if(Pike_compiler->compiler_pass==2) {
	  if (throw_value.type == T_STRING) {
	    my_yyerror("%O", throw_value);
	    free_svalue(&throw_value);
	    throw_value.type = T_INT;
	  }
	  else {
	    handle_compile_exception ("Error resolving %S.", ident);
	  }
	}
    }
    END_CYCLIC();

    return ret;
  }

  return 0;
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

/* If the identifier is recognized as one of the magic identifiers,
 * like "this", "this_program" or "`->" when preceded by ::, then a
 * suitable node is returned, NULL otherwise. inherit_num is -1 when
 * accessing all inherits (i.e. when :: is used without any identifier
 * before). */
struct node_s *program_magic_identifier (struct program_state *state,
					 int state_depth, int inherit_num,
					 struct pike_string *ident,
					 int colon_colon_ref)
{
#if 0
  fprintf (stderr, "magic_identifier (state, %d, %d, %s, %d)\n",
	   state_depth, inherit_num, ident->str, colon_colon_ref);
#endif

  if (!inherit_num) {
    /* These are only recognized when looking in the current program
     * and not an inherited one. */

    /* Handle this by referring to the magic this identifier at index 0. */
    if (ident == this_string) {
      if (state_depth > 0) 
	return mkexternalnode (state->new_program, IDREF_MAGIC_THIS);
      else
	return mkidentifiernode (IDREF_MAGIC_THIS);
    }

    /* Handle this_program */
    if (ident == this_program_string) {
      node *n = mkefuncallnode ("object_program",
				state_depth > 0 ?
				mkexternalnode (state->new_program, IDREF_MAGIC_THIS) :
				mkidentifiernode (IDREF_MAGIC_THIS));
      /* We know this expression is constant. */
      n->node_info &= ~OPT_NOT_CONST;
      n->tree_info &= ~OPT_NOT_CONST;
      return n;
    }
  }

  if (colon_colon_ref) {
    /* These are only recognized when prefixed with the :: operator. */

    if (inherit_num < 0) inherit_num = 0;
    if(ident == lfun_strings[LFUN_ARROW] ||
       ident == lfun_strings[LFUN_INDEX]) {
      return mknode(F_MAGIC_INDEX, mknewintnode(inherit_num),
		    mknewintnode(state_depth));
    } else if(ident == lfun_strings[LFUN_ASSIGN_ARROW] ||
	      ident == lfun_strings[LFUN_ASSIGN_INDEX]) {
      return mknode(F_MAGIC_SET_INDEX, mknewintnode(inherit_num),
		    mknewintnode(state_depth));
    } else if(ident == lfun_strings[LFUN__INDICES]) {
      return mknode(F_MAGIC_INDICES, mknewintnode(inherit_num),
		    mknewintnode(state_depth));
    } else if(ident == lfun_strings[LFUN__VALUES]) {
      return mknode(F_MAGIC_VALUES, mknewintnode(inherit_num),
		    mknewintnode(state_depth));
    }
  }

  return NULL;
}

/* Fixme: allow level=0 to return the current level */
struct program *parent_compilation(int level)
{
  int n;
  struct program_state *p=Pike_compiler->previous;
  for(n=0;n<level;n++)
  {
    if(n>=compilation_depth) return 0;
    p=p->previous;
    if(!p) return 0;
  }
  return p->new_program;
}

#define ID_TO_PROGRAM_CACHE_SIZE 512
struct program *id_to_program_cache[ID_TO_PROGRAM_CACHE_SIZE];

struct program *id_to_program(INT32 id)
{
  struct program *p;
  INT32 h;
  if(!id) return 0;
  h=id & (ID_TO_PROGRAM_CACHE_SIZE-1);

  if((p=id_to_program_cache[h]))
    if(p->id==id)
      return p;

  for(p=first_program;p;p=p->next)
  {
    if(id==p->id)
    {
      id_to_program_cache[h]=p;
      return p;
    }
  }

  if ((id > 0) && (id < PROG_DYNAMIC_ID_START)) {
    /* Reserved id. Attempt to load the proper dynamic module
     * to resolv the id.
     */
    char *module = NULL;

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
      if ((id >= 100) && (id <= 300)) {
	module = "Image";
      } else if ((id >= 1000) && (id <= 2000)) {
	module = "___GTK";
      }
      break;
    }
    if (module) {
      push_text(module);
      SAFE_APPLY_MASTER("resolv", 1);
      pop_stack();

      /* Try again... */
      for(p=first_program;p;p=p->next)
      {
	if(id==p->id)
	{
	  id_to_program_cache[h]=p;
	  return p;
	}
      }
    }
  }
  return 0;
}

/* Here starts routines which are used to build new programs */

/* Re-allocate all the memory in the program in one chunk. because:
 * 1) The individual blocks are much bigger than they need to be
 * 2) cuts down on malloc overhead (maybe)
 * 3) localizes memory access (decreases paging)
 */
void optimize_program(struct program *p)
{
  size_t size=0;
  char *data;

  /* Already done (shouldn't happen, but who knows?) */
  if(p->flags & PROGRAM_OPTIMIZED) return;

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

#define FOO(NUMTYPE,TYPE,ARGTYPE,NAME) \
  size=DO_ALIGN(size, ALIGNOF(TYPE)); \
  if (p->PIKE_CONCAT (num_, NAME))					\
    MEMCPY(data+size,p->NAME,p->PIKE_CONCAT(num_,NAME)*sizeof(p->NAME[0])); \
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
  return val_a < val_b ? -1 : (val_a == val_b ? 0 : 1);
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
struct pike_string *find_program_name(struct program *p, INT32 *line)
{
  INT32 l;
  if(!line) line=&l;

#ifdef DEBUG_MALLOC
  {
    char *tmp=dmalloc_find_name(p);
    if (tmp) {
      char *p = STRCHR (tmp, ':');
      if (p) {
	char *pp;
	while ((pp = STRCHR (p + 1, ':'))) p = pp;
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

int override_identifier (struct reference *ref, struct pike_string *name)
{
  int id = -1, cur_id = 0;

  int new_is_variable =
    IDENTIFIER_IS_VARIABLE(ID_FROM_PTR(Pike_compiler->new_program,
				       ref)->identifier_flags);

  /* This loop could possibly be optimized by looping over
   * each inherit and looking up 'name' in each inherit
   * and then see if should be overwritten
   * /Hubbe
   */

  for(;cur_id<Pike_compiler->new_program->num_identifier_references;cur_id++)
  {
    /* Do not zapp hidden identifiers */
    if(Pike_compiler->new_program->identifier_references[cur_id].id_flags & ID_HIDDEN)
      continue;

    /* Do not zapp inherited inline ('local') identifiers */
    if((Pike_compiler->new_program->identifier_references[cur_id].id_flags &
	(ID_INLINE|ID_INHERITED)) == (ID_INLINE|ID_INHERITED))
      continue;

    /* Do not zapp functions with the wrong name... */
    if(ID_FROM_INT(Pike_compiler->new_program, cur_id)->name != name)
      continue;

#ifdef PROGRAM_BUILD_DEBUG
    fprintf(stderr, "%.*soverloaded reference %d (id_flags:0x%04x)\n",
	    compilation_depth, "                ", cur_id,
	    Pike_compiler->new_program->identifier_references[cur_id].id_flags);
#endif

    if (!new_is_variable &&
	IDENTIFIER_IS_VARIABLE(ID_FROM_INT(Pike_compiler->new_program,
					   cur_id)->identifier_flags)) {
      /* Overloading a variable with a constant or a function.
       * This is generally a bad idea.
       */
      Pike_compiler->new_program->identifier_references[cur_id].id_flags |=
	ID_INLINE|ID_HIDDEN;
      yywarning("Attempt to override a non local variable %S "
		"with a non variable.", name);
      continue;
    }

    Pike_compiler->new_program->identifier_references[cur_id]=*ref;
    id = cur_id;
  }

  return id;
}

void fixate_program(void)
{
  INT32 i,e,t;
  struct program *p=Pike_compiler->new_program;

  if(p->flags & PROGRAM_FIXED) return;
#ifdef PIKE_DEBUG
  if(p->flags & PROGRAM_OPTIMIZED)
    Pike_fatal("Cannot fixate optimized program\n");
#endif

  /* Fixup identifier_flags. */
  for (i=0; i < p->num_identifiers; i++) {
    if (IDENTIFIER_IS_FUNCTION(p->identifiers[i].identifier_flags) ==
	IDENTIFIER_FUNCTION) {
      /* Get rid of any remaining tentative type markers. */
      p->identifiers[i].identifier_flags &= ~IDENTIFIER_C_FUNCTION;
    }
  }

  /* Fixup identifier overrides. */
  for (i = 0; i < p->num_identifier_references; i++) {
    struct reference *ref = p->identifier_references + i;
    if (ref->id_flags & ID_HIDDEN) continue;
    if (ref->inherit_offset != 0) continue;
    override_identifier (ref, ID_FROM_PTR (p, ref)->name);
  }

  /* Ok, sort for binsearch */
  for(e=i=0;i<(int)p->num_identifier_references;i++)
  {
    struct reference *funp;
    struct identifier *fun;
    funp=p->identifier_references+i;
    if(funp->id_flags & (ID_HIDDEN|ID_STATIC)) continue;
    fun=ID_FROM_PTR(p, funp);
    if(funp->id_flags & ID_INHERITED)
    {
      int found_better=-1;
      int funa_is_prototype;

      if(funp->id_flags & ID_PRIVATE) continue;
      funa_is_prototype = fun->func.offset == -1;
/*    if(fun->func.offset == -1) continue; * prototype */

      /* check for multiple definitions */
      for(t=i+1;t<(int)p->num_identifier_references;t++)
      {
	struct reference *funpb;
	struct identifier *funb;

	funpb=p->identifier_references+t;
	if(funpb->id_flags & (ID_HIDDEN|ID_STATIC)) continue;
	funb=ID_FROM_PTR(p,funpb);
	/* if(funb->func.offset == -1) continue; * prototype */

	if(fun->name==funb->name)
	{
	  found_better=t;

	  /* FIXME: Is this stuff needed?
	   *        It looks like it already is done by define_function().
	   */
	  if(funa_is_prototype && (funb->func.offset != -1) &&
	     !(funp->id_flags & ID_INLINE))
	  {
	    funp->inherit_offset = funpb->inherit_offset;
	    funp->identifier_offset = funpb->identifier_offset;
	  }
	  if(!funa_is_prototype && funb->func.offset == -1)
	  {
	    funpb->inherit_offset = funp->inherit_offset;
	    funpb->identifier_offset = funp->identifier_offset;
	  }
	}
      }
      if(found_better!=-1)
	continue;
    }
    if ((fun->func.offset == -1) && (funp->id_flags & ID_INLINE) &&
	IDENTIFIER_IS_PIKE_FUNCTION(fun->identifier_flags)) {
      if (fun->name->len < 900) {
	my_yyerror("Missing definition for local function %S.",
		   fun->name);
      } else {
	yyerror("Missing definition for local function.");
      }
    }
    add_to_identifier_index(i);
  }
  fsort_program_identifier_index(p->identifier_index,
				 p->identifier_index +
				 p->num_identifier_index - 1,
				 p);


  p->flags |= PROGRAM_FIXED;

  /* Yes, it is supposed to start at 1  /Hubbe */
  for(i=1;i<NUM_LFUNS;i++) {
    p->lfuns[i] = low_find_lfun(p, i);
  }

  if(Pike_compiler->flags & COMPILATION_CHECK_FINAL)
  {
    for(i=0;i<(int)p->num_identifier_references;i++)
    {
      if((p->identifier_references[i].id_flags & (ID_NOMASK|ID_HIDDEN)) ==
	 ID_NOMASK)
      {
	struct pike_string *name=ID_FROM_INT(p, i)->name;

	e=find_shared_string_identifier(name,p);
	if(e == -1)
	  e=really_low_find_shared_string_identifier(name, p,
						     SEE_STATIC|SEE_PRIVATE);

	if((e != i) && (e != -1))
	{
	  if(name->len < 1024)
	    my_yyerror("Illegal to redefine final identifier %S", name);
	  else
	    my_yyerror("Illegal to redefine final identifier (name too large to print).");
	}
      }
    }
  }

#ifdef DEBUG_MALLOC
  {
#define DBSTR(X) ((X)?(X)->str:"")
    int e,v;
    INT32 line;
    struct pike_string *tmp;
    struct memory_map *m=0;;
    if(lex.current_file && 
       lex.current_file->str &&
       lex.current_file->len &&
       !strcmp(lex.current_file->str,"-"))
    {
      m=dmalloc_alloc_mmap( DBSTR(lex.current_file), lex.current_line);
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
  MEMSET(p, 0, sizeof(struct program));
  p->flags|=PROGRAM_VIRGIN;
  p->alignment_needed=1;

  GC_ALLOC(p);
  p->id=++current_program_id;
  INIT_PIKE_MEMOBJ(p);

  DOUBLELINK(first_program, p);
  GETTIMEOFDAY(& p->timestamp);
  return p;
}

/*
 * Start building a new program
 */
void low_start_new_program(struct program *p,
			   int pass,
			   struct pike_string *name,
			   int flags,
			   int *idp)
{
  int id=0;
  struct svalue tmp;

#if 0
#ifdef SHARED_NODES
  if (!node_hash.table) {
    node_hash.table = malloc(sizeof(node *)*32831);
    if (!node_hash.table) {
      Pike_fatal("Out of memory!\n");
    }
    MEMSET(node_hash.table, 0, sizeof(node *)*32831);
    node_hash.size = 32831;
  }
#endif /* SHARED_NODES */
#endif /* 0 */

  /* We don't want to change thread, but we don't want to
   * wait for the other threads to complete.
   */
  low_init_threads_disable();

  compilation_depth++;

  tmp.type=T_PROGRAM;
  if(!p)
  {
    p=low_allocate_program();
    if(name)
    {
      tmp.u.program=p;
      id=add_constant(name, &tmp, flags & ~ID_EXTERN);
#if 0
      fprintf(stderr,"Compiling class %s, depth=%d\n",name->str,compilation_depth);
    }else{
      fprintf(stderr,"Compiling file %s, depth=%d\n",
	      lex.current_file ? lex.current_file->str : "-",
	      compilation_depth);
#endif
    }
  }else{
    tmp.u.program=p;
    add_ref(p);
    if((pass == 2) && name)
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
  if (pass == 1) {
    if(compilation_depth >= 1)
      add_ref(p->parent = Pike_compiler->new_program);
  }
  p->flags &=~ PROGRAM_VIRGIN;
  Pike_compiler->parent_identifier=id;
  if(idp) *idp=id;

  CDFPRINTF((stderr, "th(%ld) %p low_start_new_program() %s "
	     "pass=%d: threads_disabled:%d, compilation_depth:%d\n",
	     (long)th_self(), p, name ? name->str : "-",
	     Pike_compiler->compiler_pass,
	     threads_disabled, compilation_depth));

  init_type_stack();

#define PUSH
#include "compilation.h"

  Pike_compiler->compiler_pass = pass;

  Pike_compiler->num_used_modules=0;

  if(p && (p->flags & PROGRAM_FINISHED))
  {
    yyerror("Pass2: Program already done");
    p=0;
  }

  Pike_compiler->malloc_size_program = ALLOC_STRUCT(program);
  Pike_compiler->fake_object=alloc_object();

#ifdef PIKE_DEBUG
  Pike_compiler->fake_object->storage=(char *)xalloc(256 * sizeof(struct svalue));
  /* Stipple to find illegal accesses */
  MEMSET(Pike_compiler->fake_object->storage,0x55,256*sizeof(struct svalue));
#else
  Pike_compiler->fake_object->storage=(char *)malloc(sizeof(struct parent_info));
#endif
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

#ifdef PIKE_SECURITY
  Pike_compiler->fake_object->prot=0;
#endif

  debug_malloc_touch(Pike_compiler->fake_object);
  debug_malloc_touch(Pike_compiler->fake_object->storage);

  if(name)
  {
    /* Fake objects have parents regardless of PROGRAM_USE_PARENT  */
    if((((struct parent_info *)Pike_compiler->fake_object->storage)->parent=Pike_compiler->previous->fake_object))
      add_ref(Pike_compiler->previous->fake_object);
      ((struct parent_info *)Pike_compiler->fake_object->storage)->parent_identifier=id;
  }else{
    ((struct parent_info *)Pike_compiler->fake_object->storage)->parent=0;
    ((struct parent_info *)Pike_compiler->fake_object->storage)->parent_identifier=0;
  }

  Pike_compiler->new_program=p;

#ifdef PROGRAM_BUILD_DEBUG
  if (name) {
    fprintf (stderr, "%.*sstarting program %d (pass=%d): ",
	     compilation_depth, "                ", Pike_compiler->new_program->id, Pike_compiler->compiler_pass);
    push_string (name);
    print_svalue (stderr, --Pike_sp);
    putc ('\n', stderr);
  }
  else
    fprintf (stderr, "%.*sstarting program %d (pass=%d)\n",
	     compilation_depth, "                ", Pike_compiler->new_program->id, Pike_compiler->compiler_pass);
#endif

  if (compilation_depth >= 1) {
    if(TEST_COMPAT(7,2) || (lex.pragmas & ID_SAVE_PARENT))
    {
      p->flags |= PROGRAM_USES_PARENT;
    }else if (!(lex.pragmas & ID_DONT_SAVE_PARENT)) {
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
#define FOO(NUMTYPE,TYPE,ARGTYPE,NAME) \
    Pike_compiler->malloc_size_program->PIKE_CONCAT(num_,NAME)=Pike_compiler->new_program->PIKE_CONCAT(num_,NAME);
#include "program_areas.h"


    {
      INT32 line=0, off=0;
      size_t len = 0;
      INT32 shift = 0;
      char *file=0;
      char *cnt=Pike_compiler->new_program->linenumbers;

      while(cnt < Pike_compiler->new_program->linenumbers +
	    Pike_compiler->new_program->num_linenumbers)
      {
	if(*cnt == 127)
	{
	  cnt++;
	  len = get_small_number(&cnt);
	  shift = *cnt;
	  file = ++cnt;
	  CHECK_FILE_ENTRY (Pike_compiler->new_program, cnt, len, shift);
	  cnt += len<<shift;
	}
	off+=get_small_number(&cnt);
	line+=get_small_number(&cnt);
      }
      Pike_compiler->last_line=line;
      Pike_compiler->last_pc=off;
      if(file)
      {
	struct pike_string *str = begin_wide_shared_string(len, shift);
	if(Pike_compiler->last_file) free_string(Pike_compiler->last_file);
	memcpy(str->str, file, len<<shift);
	Pike_compiler->last_file = end_shared_string(str);
      }
    }

  }else{
    static struct pike_string *s;
    struct inherit i;

#define START_SIZE 64
#define FOO(NUMTYPE,TYPE,ARGTYPE,NAME)					\
    if (Pike_compiler->new_program->NAME) {				\
      free (Pike_compiler->new_program->NAME);				\
      Pike_compiler->new_program->PIKE_CONCAT(num_,NAME) = 0;		\
    }									\
    Pike_compiler->malloc_size_program->PIKE_CONCAT(num_,NAME)=START_SIZE; \
    Pike_compiler->new_program->NAME=(TYPE *)xalloc(sizeof(TYPE) * START_SIZE);
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

PMOD_EXPORT void debug_start_new_program(int line, const char *file)
{
  struct pike_string *save_file = lex.current_file;
  int save_line = lex.current_line;

  { /* Trim off the leading path of the compilation environment. */
    const char *p = DEFINETOSTR(PIKE_SRC_ROOT), *f = file;
    while (*p && *p == *f) p++, f++;
    while (*f == '/' || *f == '\\') f++;

    lex.current_file = make_shared_string(f);
    lex.current_line = line;
  }

  CDFPRINTF((stderr,
	     "th(%ld) start_new_program(%d, %s): "
	     "threads_disabled:%d, compilation_depth:%d\n",
	     (long)th_self(), line, file, threads_disabled, compilation_depth));

  low_start_new_program(0,1,0,0,0);
  store_linenumber(line,lex.current_file);
  debug_malloc_name(Pike_compiler->new_program, file, line);

  free_string(lex.current_file);
  lex.current_file = save_file;
  lex.current_line = save_line;
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
    free_program(parent);
  }


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
    if(p->program) {
#ifdef PIKE_USE_MACHINE_CODE
#ifdef VALGRIND_DISCARD_TRANSLATIONS
      VALGRIND_DISCARD_TRANSLATIONS(p->program,
				    p->num_program*sizeof(p->program[0]));
#endif /* VALGRIND_DISCARD_TRANSLATIONS */
#endif /* PIKE_USE_MACHINE_CODE */
      dmfree(p->program);
    }
#define FOO(NUMTYPE,TYPE,ARGTYPE,NAME) p->NAME=0;
#include "program_areas.h"
  }else{
#define FOO(NUMTYPE,TYPE,ARGTYPE,NAME) \
    if(p->NAME) { dmfree((char *)p->NAME); p->NAME=0; }
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
	    DO_NOT_WARN((long)p->inherits[e].storage_offset));

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
	fprintf (stderr, "  inherit %d\n", q);
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

  unuse_modules(Pike_compiler->num_used_modules);
}

int sizeof_variable(int run_time_type)
{
  switch(run_time_type)
  {
    case T_FUNCTION:
    case T_MIXED: return sizeof(struct svalue);
    case T_FLOAT: return sizeof(FLOAT_TYPE);
    case T_INT: return sizeof(INT_TYPE);
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
    default: return ALIGNOF(void *);
  }
}

#ifdef PIKE_DEBUG

void dump_program_tables (struct program *p, int indent)
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

    fprintf(stderr, "%*s  %4d: %5x %7d %10d  %s\n",
	    indent, "",
	    d, ref->id_flags, ref->inherit_offset,
	    ref->identifier_offset,
	    ID_FROM_PTR(p,ref)->name->size_shift ? "(wide)" :
	    ID_FROM_PTR(p,ref)->name->str);
    if (IDENTIFIER_IS_PIKE_FUNCTION(ID_FROM_PTR(p,ref)->identifier_flags)) {
      INT32 line;
      struct program *inh_p = INHERIT_FROM_PTR(p,ref)->prog;
      struct pike_string *file =
	get_line (ID_FROM_PTR(p,ref)->func.offset + inh_p->program, inh_p, &line);
      if (!file->size_shift)
	fprintf (stderr, "%*s                                  %s:%d\n",
		 indent, "", file->str, line);
      free_string (file);
    }
  }

  fprintf(stderr, "\n"
	  "%*sIdentifier index table:\n"
	  "%*s  ####: Index\n",
	  indent, "", indent, "");
  for (d = 0; d < p->num_identifier_index; d++) {
    fprintf(stderr, "%*s  %4d: %5d\n",
	    indent, "",
	    d, p->identifier_index[d]);
  }

  fprintf(stderr, "\n"
	  "%*sInherit table:\n"
	  "%*s  ####: Level prog_id id_level storage_offs "
	  "par_id par_offs par_obj_id id_ref_offs\n",
	  indent, "", indent, "");
  for (d=0; d < p->num_inherits; d++) {
    struct inherit *inh = p->inherits + d;

    fprintf(stderr, "%*s  %4d: %5d %7d %8d %12d %6d %8d %10d %11d\n",
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

    fprintf(stderr, "%*s  %4d: %5x %6d %4d \"%s\"\n",
	    indent, "",
	    d, id->identifier_flags, id->func.offset,
	    id->run_time_type, id->name->str);
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
	  "%*s  ####: Type Name\n",
	  indent, "", indent, "");
  for (d = 0; d < p->num_constants; d++) {
    struct program_constant *c = p->constants + d;
#if 0
    fprintf(stderr, "%*s  %4d: %-15s %s%s%s\n",
	    indent, "",
	    d, get_name_of_type (c->sval.type),
	    c->name?"\"":"",c->name?c->name->str:"NULL",c->name?"\"":"");
#else /* !0 */
    fprintf(stderr, "%*s  %4d: %-15s %d\n",
	    indent, "",
	    d, get_name_of_type (c->sval.type),
	    c->offset);
#endif /* 0 */
  }

  fprintf(stderr, "\n"
	  "%*sLinenumber table:\n",
	  indent, "");
  {
    INT32 off = 0, line = 0;
    char *cnt = p->linenumbers;

    while (cnt < p->linenumbers + p->num_linenumbers) {
      if (*cnt == 127) {
	int len, shift;
	char *file;
	cnt++;
	len = get_small_number(&cnt);
	shift = *cnt;
	file = ++cnt;
	CHECK_FILE_ENTRY (p, cnt, len, shift);
	cnt += len << shift;
	if (!shift) {
	  fprintf(stderr, "%*s  Filename: \"%s\"\n", indent, "", file);
	} else {
	  fprintf(stderr, "%*s  Filename: len:%d, shift:%d\n", indent, "", len, shift);
	}
      }
      off += get_small_number(&cnt);
      line += get_small_number(&cnt);
      fprintf(stderr, "%*s    %8d:%8d\n", indent, "", off, line);
    }
  }

  fprintf(stderr, "\n");
}

void check_program(struct program *p)
{
  INT32 size;
  unsigned INT32 checksum, e;
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
    if (p->flags & PROGRAM_FINISHED && s->type == T_OBJECT &&
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
    check_type_string(p->identifiers[e].type);

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

    if(p->identifiers[e].run_time_type!=T_MIXED)
      check_type(p->identifiers[e].run_time_type);

    if(IDENTIFIER_IS_VARIABLE(p->identifiers[e].identifier_flags))
    {
      if( (p->identifiers[e].func.offset /* + OFFSETOF(object,storage)*/ ) &
	 (alignof_variable(p->identifiers[e].run_time_type)-1))
      {
	Pike_fatal("Variable %s offset is not properly aligned (%ld).\n",
	      p->identifiers[e].name->str,
	      PTRDIFF_T_TO_LONG(p->identifiers[e].func.offset));
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

    if(IDENTIFIER_IS_VARIABLE(i->identifier_flags))
    {
      size_t q, size;
      /* Variable */
      ptrdiff_t offset = INHERIT_FROM_INT(p, e)->storage_offset+i->func.offset;
      size=sizeof_variable(i->run_time_type);

      if((offset+size > (size_t)p->storage_needed) || offset<0)
	Pike_fatal("Variable outside storage! (%s)\n",i->name->str);

      for(q=0;q<size;q++)
      {
	if(offset+q >= NELEM(variable_positions)) break;

	if(variable_positions[offset+q] != -1)
	{
	  if(ID_FROM_INT(p,variable_positions[offset+q])->run_time_type !=
	     i->run_time_type)
	  {
	    fprintf(stderr, "Storage offset: 0x%08x vs 0x%08x\n"
		    "Func offset: 0x%08x vs 0x%08x\n"
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
		    "Offset 0x%08x - 0x%08x overlaps with 0x%08x - 0x%08x\n",
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
		    "Offset 0x%08x - 0x%08x overlaps with 0x%08x - 0x%08x\n",
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

/* Note: This function is misnamed, since it's run after both passes. /mast */
/* finish-states:
 *
 *   0: First pass.
 *   1: Last pass.
 *   2: Called from decode_value().
 */
struct program *end_first_pass(int finish)
{
  int e;
  struct program *prog;
  struct pike_string *s;

  debug_malloc_touch(Pike_compiler->fake_object);
  debug_malloc_touch(Pike_compiler->fake_object->storage);

  MAKE_CONST_STRING(s,"__INIT");


  /* Collect references to inherited __INIT functions */
  if (!(Pike_compiler->new_program->flags & PROGRAM_AVOID_CHECK)) {
    for(e=Pike_compiler->new_program->num_inherits-1;e;e--)
    {
      int id;
      if(Pike_compiler->new_program->inherits[e].inherit_level!=1) continue;
      id=low_reference_inherited_identifier(0, e, s, SEE_STATIC);
      if(id!=-1)
      {
	Pike_compiler->init_node=mknode(F_COMMA_EXPR,
		         mkcastnode(void_type_string,
				    mkapplynode(mkidentifiernode(id),0)),
		         Pike_compiler->init_node);
      }
    }
  }

  /*
   * Define the __INIT function, but only if there was any code
   * to initialize.
   */

  if(Pike_compiler->init_node)
  {
    Pike_compiler->compiler_frame->current_function_number = -2;
    e=dooptcode(s,
		mknode(F_COMMA_EXPR,
		       Pike_compiler->init_node,
		       mknode(F_RETURN,mkintnode(0),0)),
		function_type_string,
		ID_STATIC);
    Pike_compiler->init_node=0;
  } else if (finish == 2) {
    /* Called from decode_value(). */
    e = low_find_lfun(Pike_compiler->new_program, LFUN___INIT);
  }else{
    e=-1;
  }
  Pike_compiler->new_program->lfuns[LFUN___INIT]=e;

  pop_compiler_frame(); /* Pop __INIT local variables */

  if(Pike_compiler->num_parse_error > 0)
  {
    CDFPRINTF((stderr, "th(%ld) Compilation errors (%d).\n",
	       (long)th_self(), Pike_compiler->num_parse_error));
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
    }

#ifdef PIKE_DEBUG
    check_program(prog);
    if(l_flag)
      dump_program_desc(prog);
#endif
  }

#ifdef PROGRAM_BUILD_DEBUG
  fprintf (stderr, "%.*sfinishing program %d (pass=%d)\n",
	   compilation_depth, "                ", Pike_compiler->new_program->id, Pike_compiler->compiler_pass);
#endif

  toss_compilation_resources();

#if 0
  CDFPRINTF((stderr,
	     "th(%ld) end_first_pass(): "
	     "compilation_depth:%d, Pike_compiler->compiler_pass:%d\n",
	     (long)th_self(), compilation_depth, Pike_compiler->compiler_pass));
#endif

  if(!Pike_compiler->compiler_frame && (Pike_compiler->compiler_pass==2 || !prog) && resolve_cache)
  {
    free_mapping(dmalloc_touch(struct mapping *, resolve_cache));
    resolve_cache=0;
  }

#ifdef SHARED_NODES
  /* free(node_hash.table); */
#endif /* SHARED_NODES */

#define POP
#include "compilation.h"

  exit_type_stack();

  free_all_nodes();

  CDFPRINTF((stderr,
	     "th(%ld) %p end_first_pass(%d): "
	     "threads_disabled:%d, compilation_depth:%d\n",
	     (long)th_self(), prog, finish,
	     threads_disabled, compilation_depth));

  compilation_depth--;

  exit_threads_disable(NULL);

  return prog;
}

/*
 * Finish this program, returning the newly built program
 */
PMOD_EXPORT struct program *debug_end_program(void)
{
  Pike_compiler->compiler_pass = 2;
  return end_first_pass(1);
}


/*
 * Allocate needed for this program in the object structure.
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
	  PTRDIFF_T_TO_LONG(alignment));
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
    Pike_compiler->new_program->alignment_needed =
      DO_NOT_WARN((unsigned INT8)alignment);

#ifdef PIKE_DEBUG
  if(offset < Pike_compiler->new_program->storage_needed)
    Pike_fatal("add_storage failed horribly!\n");

  if( (offset /* + OFFSETOF(object,storage) */ - modulo_orig ) % alignment )
    Pike_fatal("add_storage failed horribly(2) %ld %ld %ld %ld!\n",
	  DO_NOT_WARN((long)offset),
	  (long)0 /* + OFFSETOF(object,storage) */,
	  DO_NOT_WARN((long)modulo_orig),
	  DO_NOT_WARN((long)alignment));

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
  handler=((oldhandlertype *)Pike_fp->context.prog->program)[e];
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
#ifdef HAVE_COMPUTED_GOTO
      add_to_program(Pike_compiler->new_program->event_handler);
#else /* !HAVE_COMPUTED_GOTO */
      for(e=0;e<sizeof(Pike_compiler->new_program->event_handler);e++)
	add_to_program(tmp[e]);
#endif /* HAVE_COMPUTED_GOTO */
    }    
    Pike_compiler->new_program->event_handler=compat_event_handler;
  }
}

/*
 * set a callback used to initialize clones of this program
 * the init function is called at clone time
 * This function is obsolete, use pike_set_prog_event_callback instead.
 */
PMOD_EXPORT void set_init_callback(void (*init)(struct object *))
{
  add_compat_event_handler();
  ((oldhandlertype *)Pike_compiler->new_program->program)[PROG_EVENT_INIT]=init;
}

/*
 * set a callback used to de-initialize clones of this program
 * the exit function is called at destruct
 * This function is obsolete, use pike_set_prog_event_callback instead.
 */
PMOD_EXPORT void set_exit_callback(void (*exit)(struct object *))
{
  add_compat_event_handler();
  ((oldhandlertype *)Pike_compiler->new_program->program)[PROG_EVENT_EXIT]=exit;
}

/*
 * This callback is used by the gc to traverse all references to
 * objects. It should call some gc_recurse_* function exactly once for
 * each reference that the pike internals doesn't know about.
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
 * This function is obsolete, use pike_set_prog_event_callback instead.
 */
PMOD_EXPORT void set_gc_recurse_callback(void (*m)(struct object *))
{
  add_compat_event_handler();
  ((oldhandlertype *)Pike_compiler->new_program->program)[PROG_EVENT_GC_RECURSE]=m;
}

/*
 * This callback is used by the gc to count all references to objects.
 * It should call gc_check, gc_check_(weak_)svalues or
 * gc_check_(weak_)short_svalue exactly once for each reference that
 * the pike internals doesn't know about.
 *
 * If a reference is shared between objects, it should be counted once
 * for all shared instances. The return value from gc_check is useful
 * to ensure this; it's zero when called the first time for its
 * argument.
 *
 * This function is obsolete, use pike_set_prog_event_callback instead.
 */
PMOD_EXPORT void set_gc_check_callback(void (*m)(struct object *))
{
  add_compat_event_handler();
  ((oldhandlertype *)Pike_compiler->new_program->program)[PROG_EVENT_GC_CHECK]=m;
}

void pike_set_prog_event_callback(void (*cb)(int))
{
#ifdef PIKE_DEBUG
  if(Pike_compiler->new_program->event_handler)
    Pike_fatal("Program already has an event handler!\n");
#endif
  Pike_compiler->new_program->event_handler=cb;
}

void pike_set_prog_optimize_callback(node *(*opt)(node *))
{
#ifdef PIKE_DEBUG
  if(Pike_compiler->new_program->optimize)
    Pike_fatal("Program already has an optimize handler!\n");
#endif
  Pike_compiler->new_program->optimize = opt;
}

int really_low_reference_inherited_identifier(struct program_state *q,
					      int e,
					      int i)
{
  struct program *np=(q?q:Pike_compiler)->new_program;
  struct reference funp;
  struct program *p;
  int d, num_id_refs;

  if(i==-1) return -1;

  p = np->inherits[e].prog;

  funp = p->identifier_references[i];
  funp.inherit_offset += e;
  funp.id_flags = (funp.id_flags & ~ID_INHERITED) | ID_INLINE|ID_HIDDEN;

  num_id_refs = np->num_identifier_references;

  for(d = 0; d < num_id_refs; d++)
  {
    struct reference *refp;
    refp = np->identifier_references + d;

    if(!MEMCMP((char *)refp,(char *)&funp,sizeof funp)) return d;
  }

  if(q)
    low_add_to_identifier_references(q,funp);
  else
    add_to_identifier_references(funp);
  /* NOTE: np->num_identifier_references has been increased by one by
   *       {low_,}add_to_identifier_references().
   */
#ifdef PIKE_DEBUG
  if (num_id_refs != np->num_identifier_references-1) {
    fatal("Unexpected number of identifier references: %d != %d\n",
	  num_id_refs, np->num_identifier_references-1);
  }
#endif /* PIKE_DEBUG */
  return num_id_refs; /* aka np->num_identifier_references - 1 */
}

int low_reference_inherited_identifier(struct program_state *q,
				       int e,
				       struct pike_string *name,
				       int flags)
{
  struct program *np=(q?q:Pike_compiler)->new_program;
  struct reference funp;
  struct program *p;
  int i,d,refs;

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

int find_inherit(struct program *p, struct pike_string *name)
{
  int e;

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
    if (p->inherits[e].inherit_level > 1) continue;
    if (name == p->inherits[e].name) return e;
  }
  return 0;
}

node *reference_inherited_identifier(struct pike_string *super_name,
				     struct pike_string *function_name)
{
  int n,e,id;
  struct program_state *state=Pike_compiler->previous;

  struct program *p;


#ifdef PIKE_DEBUG
  if(function_name!=debug_findstring(function_name))
    Pike_fatal("reference_inherited_function on nonshared string.\n");
#endif

  p=Pike_compiler->new_program;

  for(e=p->num_inherits-1;e>0;e--)
  {
    if(p->inherits[e].inherit_level!=1) continue;
    if(!p->inherits[e].name) continue;

    if(super_name)
      if(super_name != p->inherits[e].name)
	continue;

    id=low_reference_inherited_identifier(0,
					  e,
					  function_name,
					  SEE_STATIC);

    if(id!=-1)
      return mkidentifiernode(id);

    if(ISCONSTSTR(function_name,"`->") ||
       ISCONSTSTR(function_name,"`[]"))
    {
      return mknode(F_MAGIC_INDEX,mknewintnode(e),mknewintnode(0));
    }

    if(ISCONSTSTR(function_name,"`->=") ||
       ISCONSTSTR(function_name,"`[]="))
    {
      return mknode(F_MAGIC_SET_INDEX,mknewintnode(e),mknewintnode(0));
    }

    if(ISCONSTSTR(function_name,"_indices"))
    {
      return mknode(F_MAGIC_INDICES,mknewintnode(e),mknewintnode(0));
    }

    if(ISCONSTSTR(function_name,"_values"))
    {
      return mknode(F_MAGIC_VALUES,mknewintnode(e),mknewintnode(0));
    }
  }


  for(n=0;n<compilation_depth;n++,state=state->previous)
  {
    struct program *p=state->new_program;

    for(e=p->num_inherits-1;e>0;e--)
    {
      if(p->inherits[e].inherit_level!=1) continue;
      if(!p->inherits[e].name) continue;

      if(super_name)
	if(super_name != p->inherits[e].name)
	  continue;

      id=low_reference_inherited_identifier(state,e,function_name,SEE_STATIC);

      if(id!=-1)
	return mkexternalnode(p, id);

      if(ISCONSTSTR(function_name,"`->") ||
	 ISCONSTSTR(function_name,"`[]"))
      {
	return mknode(F_MAGIC_INDEX,
		      mknewintnode(e),mknewintnode(n+1));
      }

      if(ISCONSTSTR(function_name,"`->=") ||
	 ISCONSTSTR(function_name,"`[]="))
      {
	return mknode(F_MAGIC_SET_INDEX,
		      mknewintnode(e),mknewintnode(n+1));
      }

      if(ISCONSTSTR(function_name,"_indices"))
      {
	return mknode(F_MAGIC_INDICES,
		      mknewintnode(e),mknewintnode(n+1));
      }

      if(ISCONSTSTR(function_name,"_values"))
      {
	return mknode(F_MAGIC_VALUES,
		      mknewintnode(e),mknewintnode(n+1));
      }
    }
  }

  return 0;
}

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

/*
 * make this program inherit another program
 */
void low_inherit(struct program *p,
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
  CDFPRINTF((stderr, "th(%ld) %p inherit %p\n",
	     (long) th_self(), Pike_compiler->new_program, p));
	
  if(!p)
  {
    yyerror("Illegal program pointer.");
    return;
  }

  if (p == placeholder_program) {
    yyerror("Trying to inherit placeholder program (resolver problem).");
    return;
  }

  if(p->flags & PROGRAM_NEEDS_PARENT)
  {
    struct program_state *state=Pike_compiler;

    if(!parent && !parent_offset)
    {
      yyerror("Parent pointer lost, cannot inherit!");
      /* We inherit it anyway, to avoid causing more errors */
    }

#if 0
    /* FIXME: we don't really need to set this flag on ALL
     * previous compilations, but I'm too lazy to figure out
     * exactly how deep down we need to go...
     */
    for(e=0;e<compilation_depth;e++,state=state->previous)
      state->new_program->flags |= PROGRAM_USES_PARENT;
#endif
  }

 /* parent offset was increased by 42 for above test.. */
  if(parent_offset)
    parent_offset-=42;


  if(!(p->flags & (PROGRAM_FINISHED | PROGRAM_PASS_1_DONE)))
  {
    yyerror("Cannot inherit program which is not fully compiled yet.");
    return;
  }

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
	    if(!par->prog)
	    {
	      par=0;
	      pid=-1;
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
		  pid=-1;
		  par=0;
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
	/* FIXME: Wide string handling. */
	struct pike_string *s;
	s=begin_shared_string(inherit.name->len + name->len + 2);
	MEMCPY(s->str,name->str,name->len);
	MEMCPY(s->str+name->len,"::",2);
	MEMCPY(s->str+name->len+2,inherit.name->str,inherit.name->len);
	inherit.name=end_shared_string(s);
      }
      else
      {
	inherit.name=0;
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
    struct pike_string *name;

    fun = p->identifier_references[e]; /* Make a copy */

    name=ID_FROM_PTR(p,&fun)->name;
    fun.inherit_offset += inherit_offset;

    if (fun.id_flags & ID_NOMASK)
    {
      Pike_compiler->flags |= COMPILATION_CHECK_FINAL;
    }
    
    if(fun.id_flags & ID_PRIVATE) fun.id_flags|=ID_HIDDEN;

    if (fun.id_flags & ID_PUBLIC)
      fun.id_flags |= flags & ~ID_PRIVATE;
    else
      fun.id_flags |= flags;

    fun.id_flags |= ID_INHERITED;
    add_to_identifier_references(fun);
  }
}

PMOD_EXPORT void do_inherit(struct svalue *s,
		INT32 flags,
		struct pike_string *name)
{
  struct program *p=program_from_svalue(s);
  low_inherit(p,
	      s->type == T_FUNCTION ? s->u.object : 0,
	      s->type == T_FUNCTION ? s->subtype : -1,
	      0,
	      flags,
	      name);
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

  switch(n->token)
  {
    case F_IDENTIFIER:
      p=Pike_compiler->new_program;
      offset=0;
      numid=n->u.id.number;
      goto continue_inherit;

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
      }

  continue_inherit:

      if(numid != IDREF_MAGIC_THIS &&
	 (IDENTIFIER_IS_CONSTANT((i=ID_FROM_INT(p, numid))->
				 identifier_flags)) &&
	 (i->func.offset != -1))
      {
	struct svalue *s=&PROG_FROM_INT(p, numid)->
	  constants[i->func.offset].sval;
	if(s->type != T_PROGRAM)
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

int call_handle_inherit(struct pike_string *s)
{
  int args;

  reference_shared_string(s);
  push_string(s);
  ref_push_string(lex.current_file);
  if (error_handler && error_handler->prog) {
    ref_push_object(error_handler);
    args = 3;
  }
  else args = 2;

  if (safe_apply_handler("handle_inherit", error_handler, compat_handler,
			 args, BIT_PROGRAM|BIT_FUNCTION|BIT_ZERO))
    if (Pike_sp[-1].type != T_INT)
      return 1;
    else {
      pop_stack();
      my_yyerror("Couldn't find program %S", s);
    }
  else {
    handle_compile_exception ("Error finding program");
  }

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

/*
 * Return the index of the identifier found, otherwise -1.
 */
int isidentifier(struct pike_string *s)
{
  return really_low_find_shared_string_identifier(s,
						  Pike_compiler->new_program,
						  SEE_STATIC|SEE_PRIVATE);
}

/* argument must be a shared string */
int low_define_variable(struct pike_string *name,
			struct pike_type *type,
			INT32 flags,
			size_t offset,
			INT32 run_time_type)
{
  int n;

  struct identifier dummy;
  struct reference ref;

#ifdef PIKE_DEBUG
  if(Pike_compiler->new_program->flags & (PROGRAM_FIXED | PROGRAM_OPTIMIZED))
    Pike_fatal("Attempting to add variable to fixed program\n");

  if(Pike_compiler->compiler_pass==2)
    Pike_fatal("Internal error: Not allowed to add more identifiers during second compiler pass.\n"
	  "Added identifier: \"%s\"\n", name->str);
#endif

  copy_shared_string(dummy.name, name);
  copy_pike_type(dummy.type, type);
  if (flags & ID_ALIAS) {
    dummy.identifier_flags = IDENTIFIER_VARIABLE | IDENTIFIER_ALIAS;
  } else {
    dummy.identifier_flags = IDENTIFIER_VARIABLE;
  }
  dummy.run_time_type=run_time_type;
  dummy.func.offset=offset - Pike_compiler->new_program->inherits[0].storage_offset;
#ifdef PROFILING
  dummy.self_time=0;
  dummy.num_calls=0;
  dummy.total_time=0;
#endif

  ref.id_flags=flags;
  ref.identifier_offset=Pike_compiler->new_program->num_identifiers;
  ref.inherit_offset=0;

  add_to_variable_index(ref.identifier_offset);

  debug_add_to_identifiers(dummy);

  n=Pike_compiler->new_program->num_identifier_references;
  add_to_identifier_references(ref);

  return n;
}

PMOD_EXPORT int map_variable(const char *name,
		 const char *type,
		 INT32 flags,
		 size_t offset,
		 INT32 run_time_type)
{
  int ret;
  struct pike_string *n;
  struct pike_type *t;

#ifdef PROGRAM_BUILD_DEBUG
  fprintf (stderr, "%.*sdefining variable (pass=%d): %s %s\n",
	   compilation_depth, "                ", Pike_compiler->compiler_pass, type, name);
#endif

  n=make_shared_string(name);
  t=parse_type(type);
  ret=low_define_variable(n,t,flags,offset,run_time_type);
  free_string(n);
  free_type(t);
  return ret;
}

PMOD_EXPORT int quick_map_variable(const char *name,
		       int name_length,
		       size_t offset,
		       const char *type,
		       int type_length,
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
    struct pike_string *d = describe_type (t);
    fprintf (stderr, "%.*sdefining variable (pass=%d): %s ",
	     compilation_depth, "                ", Pike_compiler->compiler_pass, d->str);
    free_string (d);
    push_string (n);
    print_svalue (stderr, --Pike_sp);
    putc ('\n', stderr);
  }
#endif

  ret=low_define_variable(n,t,flags,offset,run_time_type);
  free_string(n);
  free_type(t);
  return ret;
}

/* argument must be a shared string */
int define_variable(struct pike_string *name,
		    struct pike_type *type,
		    INT32 flags)
{
  int n, run_time_type;

#ifdef PIKE_DEBUG
  if(name!=debug_findstring(name))
    Pike_fatal("define_variable on nonshared string.\n");
#endif

#ifdef PROGRAM_BUILD_DEBUG
  {
    struct pike_string *d = describe_type (type);
    fprintf (stderr, "%.*sdefining variable (pass=%d): %s ",
	     compilation_depth, "                ", Pike_compiler->compiler_pass, d->str);
    free_string (d);
    push_string (name);
    print_svalue (stderr, --Pike_sp);
    putc ('\n', stderr);
  }
#endif

  if(type == void_type_string)
    yyerror("Variables can't be of type void");

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
      if (IDENTIFIERP(n)->id_flags & ID_NOMASK)
	my_yyerror("Illegal to redefine 'nomask/final' "
		   "variable/functions %S", name);

      if(!(IDENTIFIERP(n)->id_flags & ID_INLINE) ||
	 Pike_compiler->compiler_pass!=1)
      {
	int n2;

 	if(ID_FROM_INT(Pike_compiler->new_program, n)->type != type &&
	   !pike_types_le(type,
			  ID_FROM_INT(Pike_compiler->new_program, n)->type)) {
	  if (!match_types(ID_FROM_INT(Pike_compiler->new_program, n)->type,
			   type)) {
	    my_yyerror("Illegal to redefine inherited variable %S "
		       "with different type.", name);
	    return n;
	  } else {
	    yywarning("Redefining inherited variable %S "
		      "with different type.", name);
	  }
	}
	

	if(!IDENTIFIER_IS_VARIABLE(ID_FROM_INT(Pike_compiler->new_program, n)->
				   identifier_flags))
	{
	  my_yyerror("Illegal to redefine inherited variable "
		     "with different type.");
	  return n;
	}

	if ((ID_FROM_INT(Pike_compiler->new_program, n)->run_time_type !=
	     PIKE_T_MIXED) &&
	    (ID_FROM_INT(Pike_compiler->new_program, n)->run_time_type !=
	     compile_type_to_runtime_type(type))) {
	  my_yyerror("Illegal to redefine inherited variable "
		     "with different type.");
	  return n;
	}

	/* Copy the variable reference, so that we can change the
	 * compile-time type. */
	n2 = low_define_variable(name, type,
				 (flags | ID_ALIAS) & ~ID_EXTERN,
				 ID_FROM_INT(Pike_compiler->new_program, n)->
				 func.offset +
				 INHERIT_FROM_INT(Pike_compiler->new_program,
						  n)->storage_offset,
				 ID_FROM_INT(Pike_compiler->new_program, n)->
				 run_time_type);
	/* Copy IDENTIFIER_NO_THIS_REF state from the old variable.
	 */
	ID_FROM_INT(Pike_compiler->new_program, n2)->identifier_flags |=
	  ID_FROM_INT(Pike_compiler->new_program, n)->identifier_flags &
	  IDENTIFIER_NO_THIS_REF;
	/* Hide the old variable. */
	Pike_compiler->new_program->identifier_references[n].id_flags |=
	  ID_HIDDEN;
	return n2;
      }
    }
  }

  run_time_type=compile_type_to_runtime_type(type);

  switch(run_time_type)
  {
#ifdef AUTO_BIGNUM
#if 0
    case T_OBJECT:
      /* This is to allow room for integers in variables declared as
       * 'object', however, this could be changed in the future to only
       * make room for integers if the variable was declared as
       * 'object(Gmp.mpz)'                                     /Hubbe
       */
#endif
    case T_INT:
#endif
    case T_FUNCTION:
    case T_PROGRAM:
      run_time_type = T_MIXED;
  }

  n=low_define_variable(name,type,flags,
			low_add_storage(sizeof_variable(run_time_type),
					alignof_variable(run_time_type),0),
			run_time_type);

  ID_FROM_INT(Pike_compiler->new_program, n)->identifier_flags |= IDENTIFIER_NO_THIS_REF;

  return n;
}

PMOD_EXPORT int simple_add_variable(const char *name,
				    const char *type,
				    INT32 flags)
{
  INT32 ret;
  struct pike_string *name_s;
  struct pike_type *type_s;
  name_s = make_shared_string(name);
  type_s = parse_type(type);

  ret=define_variable(name_s, type_s, flags);
  free_string(name_s);
  free_type(type_s);
  return ret;
}

PMOD_EXPORT int add_constant(struct pike_string *name,
			     struct svalue *c,
			     INT32 flags)
{
  int n;
  struct identifier dummy;
  struct reference ref;
  struct svalue zero;

#ifdef PROGRAM_BUILD_DEBUG
  {
    if (c) {
      struct pike_type *t = get_type_of_svalue(c);
      struct pike_string *d = describe_type (t);
      fprintf (stderr, "%.*sdefining constant (pass=%d): %s ",
	       compilation_depth, "                ",
	       Pike_compiler->compiler_pass, d->str);
      free_type(t);
      free_string (d);
      push_string (name);
      print_svalue (stderr, --Pike_sp);
      fputs (" = ", stderr);
      print_svalue (stderr, c);
    }
    else {
      fprintf (stderr, "%.*sdeclaring constant (pass=%d): ",
	       compilation_depth, "                ",
	       Pike_compiler->compiler_pass);
      push_string (name);
      print_svalue (stderr, --Pike_sp);
    }
    putc ('\n', stderr);
  }
#endif

#if 0
  if (!c) {
    zero.type = T_INT;
    zero.subtype = 0;
    zero.u.integer = 0;
    c = &zero;
  }
#endif

#ifdef PIKE_DEBUG
  if(name!=debug_findstring(name))
    Pike_fatal("define_constant on nonshared string.\n");
#endif

  n = isidentifier(name);

  if(
#if 1
    c &&
#endif
    c->type == T_FUNCTION &&
    c->subtype != FUNCTION_BUILTIN &&
    c->u.object->prog)
  {
    struct reference *idref = PTR_FROM_INT(c->u.object->prog, c->subtype);
    struct program *p = PROG_FROM_PTR(c->u.object->prog, idref);
    struct identifier *id = p->identifiers + idref->identifier_offset;
    if(c->u.object->prog == Pike_compiler->new_program) {
      /* Alias for a symbol in the current program.
       */
      if(IDENTIFIER_IS_CONSTANT(id->identifier_flags) &&
	 id->func.offset != -1) {
	c=& p->constants[id->func.offset].sval;
      }
      else if (IDENTIFIER_IS_FUNCTION(id->identifier_flags)) {
	if (!idref->inherit_offset) {
	  /* Alias for a function defined in this program. */
	  /* FIXME: Does this work for forward references? */
	  return define_function(name,
				 id->type,
				 flags,
				 id->identifier_flags | IDENTIFIER_ALIAS,
				 & id->func,
				 id->opt_flags);
	} else if (Pike_compiler->new_program->flags & PROGRAM_PASS_1_DONE) {
	  /* Alias for a function defined in an inherited program. */
	  yyerror("Aliasing of inherited functions not supported yet.");
	  return define_function(name,
				 id->type,
				 flags,
				 id->identifier_flags | IDENTIFIER_ALIAS,
				 NULL,
				 id->opt_flags);	    
	} else {
	  /* First pass.
	   * Make a prototype for now.
	   */
	  return define_function(name,
				 id->type,
				 flags,
				 id->identifier_flags | IDENTIFIER_ALIAS,
				 NULL,
				 id->opt_flags);
	}
      } else if (IDENTIFIER_IS_VARIABLE(id->identifier_flags)) {
	yyerror("Attempt to make a constant of a variable.");
	c = NULL;
      }
    }
  }
    
  if(
#if 1
    c &&
#endif
    !svalues_are_constant(c,1,BIT_MIXED,0))
    yyerror("Constant values may not have references to this.");

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
      if(id->func.offset>=0)
      {
	struct pike_type *s;
	/* I don't know why this function preferred to retain the
	 * previously stored constant rather than using the one we get
	 * now, but in combination with storing zeroes in pass 1, we
	 * will be better off if we replace it. /mast */
#if 0
	struct svalue *c=&PROG_FROM_INT(Pike_compiler->new_program,n)->
	  constants[id->func.offset].sval;
#else
	assign_svalue (&PROG_FROM_INT(Pike_compiler->new_program,n)->
		       constants[id->func.offset].sval, c);
#endif
	s=get_type_of_svalue(c);
	free_type(id->type);
	id->type=s;
      }
      else {
#if 1
#ifdef PIKE_DEBUG
	if (!c) Pike_fatal("Can't declare constant during second compiler pass\n");
#endif
#endif
	free_type(id->type);
	id->type = get_type_of_svalue(c);
	id->run_time_type = c->type;
	id->func.offset = store_constant(c, 0, 0);
      }
#ifdef PROGRAM_BUILD_DEBUG
      fprintf (stderr, "%.*sstored constant #%d at %d\n",
	       compilation_depth, "                ",
	       n, id->func.offset);
#endif
      return n;
    }
  }

#ifdef PIKE_DEBUG
  if(Pike_compiler->new_program->flags & (PROGRAM_FIXED | PROGRAM_OPTIMIZED))
    Pike_fatal("Attempting to add constant to fixed program\n");

  if(Pike_compiler->compiler_pass==2)
    Pike_fatal("Internal error: Not allowed to add more identifiers during second compiler pass.\n");
#endif

  copy_shared_string(dummy.name, name);
  dummy.identifier_flags = IDENTIFIER_CONSTANT;

#if 1
  if (c) {
#endif
    dummy.type = get_type_of_svalue(c);
    dummy.run_time_type=c->type;
    dummy.func.offset=store_constant(c, 0, 0);
    dummy.opt_flags=OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND;
    if(c->type == PIKE_T_PROGRAM && (c->u.program->flags & PROGRAM_CONSTANT))
       dummy.opt_flags=0;
#if 1
  }
  else {
    copy_pike_type(dummy.type, mixed_type_string);
    dummy.run_time_type=T_MIXED;
    dummy.func.offset=-1;
    dummy.opt_flags=0;
  }
#endif

  ref.id_flags=flags;
  ref.identifier_offset=Pike_compiler->new_program->num_identifiers;
  ref.inherit_offset=0;

#ifdef PROFILING
  dummy.self_time=0;
  dummy.num_calls=0;
  dummy.total_time=0;
#endif

  debug_add_to_identifiers(dummy);

  if(n != -1)
  {
    int overridden;

    if(IDENTIFIERP(n)->id_flags & ID_NOMASK)
      my_yyerror("Illegal to redefine 'nomask' identifier %S", name);

    if(!TEST_COMPAT(7,2) &&
       IDENTIFIER_IS_VARIABLE(ID_FROM_INT(Pike_compiler->new_program,
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
    if ((overridden = override_identifier (&ref, name)) >= 0) {
#ifdef PIKE_DEBUG
      if(MEMCMP(Pike_compiler->new_program->identifier_references+n, &ref,sizeof(ref)))
	Pike_fatal("New constant overriding algorithm failed!\n");
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
  tmp.u.integer=i;
  tmp.type=T_INT;
  tmp.subtype=NUMBER_NUMBER;
  return simple_add_constant(name, &tmp, flags);
}

PMOD_EXPORT int quick_add_integer_constant(const char *name,
					   int name_length,
					   INT_ARG_TYPE i,
					   INT32 flags)
{
  struct svalue tmp;
  struct pike_string *id;
  INT32 ret;

  tmp.u.integer=i;
  tmp.type=T_INT;
  tmp.subtype=NUMBER_NUMBER;
  id=make_shared_binary_string(name,name_length);
  ret=add_constant(id, &tmp, flags);
  free_string(id);
  return ret;
}

PMOD_EXPORT int add_float_constant(const char *name,
				   FLOAT_ARG_TYPE f,
				   INT32 flags)
{
  struct svalue tmp;
  tmp.type=T_FLOAT;
  tmp.u.float_number = f;
  tmp.subtype=0;
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

  tmp.u.float_number=f;
  tmp.type=T_FLOAT;
  tmp.subtype=0;
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
  tmp.type=T_STRING;
  tmp.subtype=0;
  tmp.u.string=make_shared_string(str);
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
    tmp.type=T_PROGRAM;
    tmp.subtype=0;
    tmp.u.program=p;
  } else {
    /* Probable compilation error in a C-module. */
    tmp.type = T_INT;
    tmp.subtype = NUMBER_UNDEFINED;
    tmp.u.integer = 0;
    my_yyerror("Program constant \"%s\" is NULL.", name);
  }
  ret=simple_add_constant(name, &tmp, flags);
  return ret;
}

PMOD_EXPORT int add_object_constant(const char *name,
			struct object *o,
			INT32 flags)
{
  INT32 ret;
  struct svalue tmp;
  tmp.type=T_OBJECT;
  tmp.subtype=0;
  tmp.u.object=o;
  ret=simple_add_constant(name, &tmp, flags);
  return ret;
}

PMOD_EXPORT int add_function_constant(const char *name, void (*cfun)(INT32),
				      const char * type, int flags)
{
  struct svalue s;
  INT32 ret;

  s.type=T_FUNCTION;
  s.subtype=FUNCTION_BUILTIN;
  s.u.efun=make_callable(cfun, name, type, flags, 0, 0);
  ret=simple_add_constant(name, &s, 0);
  free_svalue(&s);
  return ret;
}


PMOD_EXPORT int debug_end_class(const char *name, ptrdiff_t namelen, INT32 flags)
{
  INT32 ret;
  struct svalue tmp;
  struct pike_string *id;

  tmp.type=T_PROGRAM;
  tmp.subtype=0;
  tmp.u.program=end_program();
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

/*
 * define a new function
 * if func isn't given, it is supposed to be a prototype.
 */
INT32 define_function(struct pike_string *name,
		      struct pike_type *type,
		      unsigned flags,
		      unsigned function_flags,
		      union idptr *func,
		      unsigned opt_flags)
{
  struct identifier *funp,fun;
  struct reference ref;
  struct svalue *lfun_type;
  INT32 i;

#ifdef PROGRAM_BUILD_DEBUG
  {
    struct pike_string *d = describe_type (type);
    fprintf (stderr, "%.*sdefining function (pass=%d): %s ",
	     compilation_depth, "                ", Pike_compiler->compiler_pass, d->str);
    free_string (d);
    push_string (name);
    print_svalue (stderr, --Pike_sp);
    putc ('\n', stderr);
  }
#endif

#ifdef PROFILING
  fun.self_time=0;
  fun.num_calls=0;
  fun.total_time=0;
#endif

  /* If this is an lfun, match against the predefined type. */
  if ((lfun_type = low_mapping_string_lookup(lfun_types, name))) {
#ifdef PIKE_DEBUG
    if (lfun_type->type != T_TYPE) {
      Pike_fatal("Bad entry in lfun_types for key \"%s\"\n", name->str);
    }
#endif /* PIKE_DEBUG */
    if (!pike_types_le(type, lfun_type->u.type)) {
      if (!match_types(type, lfun_type->u.type)) {
	my_yyerror("Type mismatch for callback function %S:", name);
	yytype_error(NULL, lfun_type->u.type, type, 0);
      } else if (lex.pragmas & ID_STRICT_TYPES) {
	yywarning("Type mismatch for callback function %S:", name);
	yytype_error(NULL, lfun_type->u.type, type,
		     YYTE_IS_WARNING);
      }
    }
  }

  if(IDENTIFIER_IS_C_FUNCTION(function_flags))
    Pike_compiler->new_program->flags |= PROGRAM_HAS_C_METHODS;

  if (Pike_compiler->compiler_pass == 1) {
    /* Mark the type as tentative by reusing IDENTIFIER_C_FUNCTION.
     *
     * NOTE: This flag MUST be cleared in the second pass.
     */
    function_flags |= IDENTIFIER_C_FUNCTION;
  }

  i=isidentifier(name);

  if(i >= 0)
  {
    int overridden;

    /* already defined */

#ifdef PROGRAM_BUILD_DEBUG
    fprintf(stderr, "%.*sexisted as identifier #%d\n",
	  compilation_depth, "                ", i);
#endif

    funp=ID_FROM_INT(Pike_compiler->new_program, i);
    ref=Pike_compiler->new_program->identifier_references[i];

    if (funp->identifier_flags & IDENTIFIER_HAS_BODY)
      /* Keep this flag. */
      function_flags |= IDENTIFIER_HAS_BODY;

    if(!(ref.id_flags & ID_INHERITED)) /* not inherited */
    {

      if( !( IDENTIFIER_IS_FUNCTION(funp->identifier_flags) &&
	     ( (!func || func->offset == -1) || (funp->func.offset == -1))))
      {
	my_yyerror("Identifier %S defined twice.", name);
	return i;
      }

      if (IDENTIFIER_IS_FUNCTION(funp->identifier_flags) !=
	  IDENTIFIER_FUNCTION) {
	/* match types against earlier prototype or vice versa */
	if(!match_types(type, funp->type))
	{
	  if (!(flags & ID_VARIANT)) {
	    my_yyerror("Prototype doesn't match for function %S.", name);
	    yytype_error(NULL, funp->type, type, 0);
	  }
	}
      }

      if(func)
	funp->func = *func;
#if 0 /* prototypes does not override non-prototypes, ok? */
      else
	funp->func.offset = -1;
#endif

      funp->identifier_flags=function_flags;

      funp->opt_flags &= opt_flags;

      free_type(funp->type);
      copy_pike_type(funp->type, type);
    }else{
#ifdef PROGRAM_BUILD_DEBUG
      fprintf(stderr, "%.*sidentifier was inherited\n",
	      compilation_depth, "                ");
#endif

      if((ref.id_flags & ID_NOMASK)
#if 0
	 && !(funp->func.offset == -1)
#endif
	)
      {
	my_yyerror("Illegal to redefine 'nomask' function %S.", name);
      }


      if(ref.id_flags & ID_INLINE)
      {
#ifdef PROGRAM_BUILD_DEBUG
	fprintf(stderr, "%.*sidentifier is local\n",
		compilation_depth, "                ");
#endif

	goto make_a_new_def;
      }

      /* Otherwise we alter the existing definition */
#ifdef PROGRAM_BUILD_DEBUG
      fprintf(stderr, "%.*saltering the existing definition\n",
	      compilation_depth, "                ");
#endif

      copy_shared_string(fun.name, name);
      copy_pike_type(fun.type, type);

      fun.run_time_type=T_FUNCTION;

      fun.identifier_flags=function_flags;

      if(func)
	fun.func = *func;
      else
	fun.func.offset = -1;

      fun.opt_flags = opt_flags;

      ref.identifier_offset=Pike_compiler->new_program->num_identifiers;
      debug_add_to_identifiers(fun);
    }

    ref.inherit_offset = 0;
    ref.id_flags = flags;
    if ((overridden = override_identifier (&ref, name)) >= 0) {
#ifdef PIKE_DEBUG
      if(MEMCMP(Pike_compiler->new_program->identifier_references+i, &ref,sizeof(ref)))
	Pike_fatal("New function overloading algorithm failed!\n");
#endif
      return overridden;
    }
    /* NOTE: At this point we already have the identifier in the
     *       new program, and just need to add the reference.
     */
  } else {
  make_a_new_def:

#ifdef PIKE_DEBUG
    if(Pike_compiler->compiler_pass==2)
      Pike_fatal("Internal error: Not allowed to add more identifiers during second compiler pass.\n");
#endif

    /* Define a new function */

    copy_shared_string(fun.name, name);
    copy_pike_type(fun.type, type);

    fun.identifier_flags=function_flags;
    fun.run_time_type=T_FUNCTION;

    if(func)
      fun.func = *func;
    else
      fun.func.offset = -1;

    fun.opt_flags = opt_flags;

#ifdef PIKE_DEBUG
    if (a_flag > 5) {
      fprintf(stderr, 
	      "Adding new function #%d: '%s'\n"
	      "  identifier_flags:0x%02x opt_flags:0x%04x\n",
	      Pike_compiler->new_program->num_identifiers,
	      fun.name->str,
	      fun.identifier_flags, fun.opt_flags);
    }
#endif /* PIKE_DEBUG */

    i=Pike_compiler->new_program->num_identifiers;

    debug_add_to_identifiers(fun);

    ref.id_flags = flags;
    ref.identifier_offset = i;
    ref.inherit_offset = 0;
  }

  /* Add the reference. */

  i=Pike_compiler->new_program->num_identifier_references;
  add_to_identifier_references(ref);

#ifdef PROGRAM_BUILD_DEBUG
  fprintf(stderr, "%.*sadded new definition #%d\n",
	  compilation_depth, "                ", i);
#endif

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
    if (((r->id_flags & ID_PARENT_REF|ID_STATIC|ID_PRIVATE|ID_HIDDEN) ==
	 ID_PARENT_REF|ID_STATIC|ID_PRIVATE|ID_HIDDEN) &&
	(r->identifier_offset == i) &&
	(!(r->inherit_offset))) {
      return j;
    }
  }
  ref.id_flags = ID_PARENT_REF|ID_STATIC|ID_PRIVATE|ID_HIDDEN;
  ref.identifier_offset = i;
  ref.inherit_offset = 0;
  add_to_identifier_references(ref);
  state->new_program->flags |= PROGRAM_USES_PARENT | PROGRAM_NEEDS_PARENT;
  return j;
}

#endif /* 0 */

/* Identifier lookup
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
 */
int really_low_find_shared_string_identifier(struct pike_string *name,
					     struct program *prog,
					     int flags)
{
  struct reference *funp;
  struct identifier *fun;
  int id, i, depth, last_inh;

#if 0
  CDFPRINTF((stderr,"th(%ld) Trying to find %s flags=%d\n",
	     (long)th_self(),name->str, flags));
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
    if(funp->id_flags & ID_STATIC)
      if(!(flags & SEE_STATIC))
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

int low_find_lfun(struct program *p, ptrdiff_t lfun)
{
  struct pike_string *lfun_name = lfun_strings[lfun];
  unsigned int flags = 0;
  struct identifier *id;
  int i =
    really_low_find_shared_string_identifier(lfun_name,
					     dmalloc_touch(struct program *,
							   p),
					     SEE_STATIC);
  if (i < 0 || !(p->flags & PROGRAM_FIXED)) return i;
  id = ID_FROM_INT(p, i);
  if (IDENTIFIER_IS_PIKE_FUNCTION(id->identifier_flags) &&
      (id->func.offset == -1)) {
    /* Function prototype. */
    return -1;
  }
  return i;
}

int lfun_lookup_id(struct pike_string *lfun_name)
{
  struct svalue *id = low_mapping_string_lookup(lfun_ids, lfun_name);
  if (!id) return -1;
  if (id->type == T_INT) return id->u.integer;
  my_yyerror("Bad entry in lfun lookup table for %S.", lfun_name);
  return -1;
}

/*
 * lookup the number of a function in a program given the name in
 * a shared_string
 */
int low_find_shared_string_identifier(struct pike_string *name,
				      struct program *prog)
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
				  struct program *prog)
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
    hashval %= FIND_FUNCTION_HASHSIZE;
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

PMOD_EXPORT int find_identifier(const char *name,struct program *prog)
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

  for (i=0;i<Pike_compiler->new_program->num_strings;i++)
    if (Pike_compiler->new_program->strings[i] == str)
      return i;

  reference_shared_string(str);
  add_to_strings(str);
  return i;
}

int store_constant(struct svalue *foo,
		   int equal,
		   struct pike_string *constant_name)
{
  struct program_constant tmp;
  unsigned int e;

  for(e=0;e<Pike_compiler->new_program->num_constants;e++)
  {
    JMP_BUF jmp;
    if (SETJMP(jmp)) {
      handle_compile_exception ("Error comparing constants.");
      /* Assume that if `==() throws an error, the svalues aren't equal. */
    }
    else {
      struct program_constant *c= Pike_compiler->new_program->constants+e;
      if((equal ? is_equal(& c->sval,foo) : is_eq(& c->sval,foo)))
      {
	UNSETJMP(jmp);
	return e;
      }
    }
    UNSETJMP(jmp);
  }
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
  int e;
  int n = 0;
  struct array *res;
  for (e = p->num_identifier_references; e--; ) {
    struct identifier *id;
    if (p->identifier_references[e].id_flags &
	(ID_HIDDEN|ID_STATIC|ID_PRIVATE)) {
      continue;
    }
    id = ID_FROM_INT(p, e);
    if (IDENTIFIER_IS_CONSTANT(id->identifier_flags)) {
      if (id->func.offset >= 0) {
	struct program *p2 = PROG_FROM_INT(p, e);
	struct svalue *val = &p2->constants[id->func.offset].sval;
	if ((val->type != T_PROGRAM) ||
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
  int e;
  int n = 0;
  struct array *res;
  for(e = p->num_identifier_references; e--; ) {
    struct identifier *id;
    if (p->identifier_references[e].id_flags &
	(ID_HIDDEN|ID_STATIC|ID_PRIVATE)) {
      continue;
    }
    id = ID_FROM_INT(p, e);
    if (IDENTIFIER_IS_CONSTANT(id->identifier_flags)) {
      if (id->func.offset >= 0) {
	struct program *p2 = PROG_FROM_INT(p, e);
	struct svalue *val = &p2->constants[id->func.offset].sval;
	if ((val->type != T_PROGRAM) ||
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

void program_index_no_free(struct svalue *to, struct program *p,
			   struct svalue *ind)
{
  int e;
  struct pike_string *s;

  if (ind->type != T_STRING) {
    Pike_error("Can't index a program with a %s (expected string)\n",
	  get_name_of_type(ind->type));
  }
  s = ind->u.string;
  e=find_shared_string_identifier(s, p);
  if(e!=-1)
  {
    struct identifier *id;
    id=ID_FROM_INT(p, e);
    if (IDENTIFIER_IS_CONSTANT(id->identifier_flags)) {
      if (id->func.offset >= 0) {
	struct program *p2 = PROG_FROM_INT(p, e);
	struct svalue *val = &p2->constants[id->func.offset].sval;
	assign_svalue_no_free(to, val);
      } else {
	/* Prototype constant. */
	to->type = T_INT;
	to->subtype = 0;
	to->u.integer = 0;
      }
      return;
    }
  }

  to->type=T_INT;
  to->subtype=NUMBER_UNDEFINED;
  to->u.integer=0;
}

/*
 * Line number support routines, now also tells what file we are in
 */

/* program.linenumbers format:
 *
 * Filename entry:
 *   1. char		127 (marker).
 *   2. small number	Filename string length.
 *   3. char		Filename string size shift.
 *   4. string data	(Possibly wide) filename string without null termination.
 * 			Each character is stored in native byte order.
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
 *     2. short		The number stored in big endian order.
 *   Else:
 *     1. char		-128 (marker).
 *     2. int		The number stored in big endian order.
 *
 * Whenever the filename changes, a filename entry followed by a line
 * number entry is stored. If only the line number changes, a line
 * number entry is stored. The first stored entry (at pc 0) is the
 * file and line where the program is defined, if they are known. The
 * definition line for a top level program is set to 0.
 */

int get_small_number(char **q)
{
  /* This is a workaround for buggy cc & Tru64 */
  unsigned char *addr = (unsigned char *)*q;
  int ret = *((signed char *)addr);
  ret=*(signed char *)*q;
  addr++;
  switch(ret)
  {
  case -127:
    ret = (((signed char *)addr)[0]<<8) | addr[1];
    addr += 2;
    break;

  case -128:
    ret = (((signed char *)addr)[0]<<24) | (addr[1]<<16) |
      (addr[2]<<8) | addr[3];
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

static void insert_small_number(INT32 a)
{
#ifdef PIKE_DEBUG
  int start = Pike_compiler->new_program->num_linenumbers;
#endif /* PIKE_DEBUG */
  if(a>-127 && a<127)
  {
    add_to_linenumbers(a);
  }else if(a>=-32768 && a<32768){
    add_to_linenumbers(-127);
    add_to_linenumbers(a>>8);
    add_to_linenumbers(a);
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
    INT32 res = get_small_number(&tmp);
    if (a != res) {
      tmp = Pike_compiler->new_program->linenumbers + start;
      fprintf(stderr, "0x%p: %02x %02x %02x %02x %02x\n",
	      tmp, (unsigned char) tmp[0], (unsigned char) tmp[1],
	      (unsigned char) tmp[2], (unsigned char) tmp[3], (unsigned char) tmp[4]);
      Pike_fatal("insert_small_number failed: %d (0x%08x) != %d (0x%08x)\n",
	    a, a, res, res);
    }
  }
#endif /* PIKE_DEBUG */
}

static void ext_insert_small_number (char **ptr, INT32 a)
{
  if(a>-127 && a<127)
  {
    *(*ptr)++ = a;
  }else if(a>=-32768 && a<32768){
    *(*ptr)++ = -127;
    *(*ptr)++ = a>>8;
    *(*ptr)++ = a;
  }else{
    *(*ptr)++ = -128;
    *(*ptr)++ = a>>24;
    *(*ptr)++ = a>>16;
    *(*ptr)++ = a>>8;
    *(*ptr)++ = a;
  }
}

void ext_store_program_line (struct program *prog, INT32 line, struct pike_string *file)
{
  char *ptr;

#ifdef PIKE_DEBUG
  if (prog->linenumbers)
    Pike_fatal ("Program already got linenumber info.\n");
  if (Pike_compiler->new_program == prog)
    Pike_fatal ("Use store_linenumber instead when the program is compiled.\n");
#endif

  ptr = prog->linenumbers = xalloc (1 + 5 + 1 + (file->len << file->size_shift) + 5 + 5);
  *ptr++ = 127;
  ext_insert_small_number (&ptr, file->len);
  *ptr++ = file->size_shift;
  MEMCPY (ptr, file->str, file->len << file->size_shift);
  ptr += file->len << file->size_shift;
  *ptr++ = 0;			/* PC */
  ext_insert_small_number (&ptr, line);
  prog->num_linenumbers = ptr - prog->linenumbers;
}

void store_linenumber(INT32 current_line, struct pike_string *current_file)
{
/*  if(!store_linenumbers)  Pike_fatal("Fnord.\n"); */
#ifdef PIKE_DEBUG
  if(a_flag)
  {
    INT32 line=0, off=0;
    size_t len = 0;
    INT32 shift = 0;
    char *file=0;
    char *cnt=Pike_compiler->new_program->linenumbers;

    if (a_flag > 50) {
      fprintf(stderr, "store_linenumber(%d, \"%s\") at pc %d\n",
	      current_line, current_file->str,
	      (INT32) PIKE_PC);
      fprintf(stderr, "  last_line:%d last_file:\"%s\"\n",
	      Pike_compiler->last_line,
	      Pike_compiler->last_file?Pike_compiler->last_file->str:"");
    }

    while(cnt < Pike_compiler->new_program->linenumbers +
	  Pike_compiler->new_program->num_linenumbers)
    {
      char *start = cnt;
      if(*cnt == 127)
      {
	cnt++;
	len = get_small_number(&cnt);
	shift = *cnt;
	file = ++cnt;
	CHECK_FILE_ENTRY (Pike_compiler->new_program, cnt, len, shift);
	cnt += len<<shift;
	if (a_flag > 100) {
	  fprintf(stderr, "Filename entry:\n"
		  "  len: %d, shift: %d\n",
		  len, shift);
	}
      }
      off+=get_small_number(&cnt);
      line+=get_small_number(&cnt);
      if (a_flag > 100) {
	fprintf(stderr, "  off: %d, line: %d\n"
		"  raw: ",
		off, line);
	for (;start < cnt; start++) {
	  fprintf(stderr, "%02x ", *((unsigned char *)start));
	}
	fprintf(stderr, "\n");
      }
    }

    if(Pike_compiler->last_line != line ||
       Pike_compiler->last_pc != off ||
       (Pike_compiler->last_file && file &&
	memcmp(Pike_compiler->last_file->str, file, len<<shift)))
    {
      Pike_fatal("Line numbering out of whack\n"
	    "    (line : %d ?= %d)!\n"
	    "    (  pc : %d ?= %d)!\n"
	    "    (shift: %d ?= %d)!\n"
	    "    (len  : %d ?= %d)!\n"
	    "    (file : %s ?= %s)!\n",
	    Pike_compiler->last_line, line,
	    Pike_compiler->last_pc, off,
	    Pike_compiler->last_file?Pike_compiler->last_file->size_shift:0,
	    shift,
	    Pike_compiler->last_file?Pike_compiler->last_file->len:0, len,
	    Pike_compiler->last_file?Pike_compiler->last_file->str:"N/A",
	    file?file:"N/A");
    }
  }
#endif
  if(Pike_compiler->last_line != current_line ||
     Pike_compiler->last_file != current_file)
  {
    if(Pike_compiler->last_file != current_file)
    {
      char *tmp;
      INT32 remain = DO_NOT_WARN((INT32)current_file->len)<<
	current_file->size_shift;

      if(Pike_compiler->last_file) free_string(Pike_compiler->last_file);
      add_to_linenumbers(127);
      insert_small_number(DO_NOT_WARN((INT32)current_file->len));
      add_to_linenumbers(current_file->size_shift);
      for(tmp=current_file->str; remain-- > 0; tmp++)
	add_to_linenumbers(*tmp);
      copy_shared_string(Pike_compiler->last_file, current_file);
    }
    insert_small_number(DO_NOT_WARN((INT32)(PIKE_PC-Pike_compiler->last_pc)));
    insert_small_number(current_line-Pike_compiler->last_line);
    Pike_compiler->last_line = current_line;
    Pike_compiler->last_pc = DO_NOT_WARN((INT32)PIKE_PC);
  }
}

#define FIND_PROGRAM_LINE(prog, file, len, shift, line) do {		\
    char *pos = prog->linenumbers;					\
    len = 0;								\
    shift = 0;								\
    file = NULL;							\
									\
    if (pos < prog->linenumbers + prog->num_linenumbers) {		\
      if (*pos == 127) {						\
	pos++;								\
	len = get_small_number(&pos);					\
	shift = *pos;							\
	file = ++pos;							\
	CHECK_FILE_ENTRY (prog, pos, len, shift);			\
	pos += len<<shift;						\
      }									\
      get_small_number(&pos);	/* Ignore the offset */			\
      line = get_small_number(&pos);					\
    }									\
  } while (0)

PMOD_EXPORT struct pike_string *low_get_program_line (struct program *prog,
						      INT32 *linep)
{
  *linep = 0;

  if (prog->linenumbers) {
    size_t len;
    INT32 shift;
    char *file;

    FIND_PROGRAM_LINE (prog, file, len, shift, (*linep));

    if (file) {
      struct pike_string *str = begin_wide_shared_string(len, shift);
      memcpy(str->str, file, len<<shift);
      return end_shared_string(str);
    }
  }

  return NULL;
}

static char *make_plain_file (char *file, size_t len, INT32 shift, int malloced)
{
  if(shift)
  {
    static char buf[1000];
    char *buffer = malloced ?
      malloc (len + 1) : (len = NELEM(buf) - 1, buf);
    PCHARP from=MKPCHARP(file, shift);
    int chr;
    size_t ptr=0;

    for (; (chr = EXTRACT_PCHARP(from)); INC_PCHARP(from, 1))
    {
      size_t space = chr > 255 ? 20 : 1;

      if (ptr + space > len) {
	if (malloced)
	  buffer = realloc (buffer, (len = (len << 1) + space) + 1);
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
    char *buffer = malloc (len + 1);
    MEMCPY (buffer, file, len);
    buffer[len] = 0;
    return buffer;
  }
}

/* Same as low_get_program_line but returns a plain char *. It's
 * malloced if the malloced flag is set, otherwise it's a pointer to a
 * static buffer which might be clobbered by later calls.
 *
 * This function is useful when the shared string table has been freed
 * and in sensitive parts of the gc where the shared string structures
 * can't be touched. It also converts wide strings to ordinary ones
 * with escaping.
 */
PMOD_EXPORT char *low_get_program_line_plain(struct program *prog, INT32 *linep,
					     int malloced)
{
  *linep = 0;

  if (prog->linenumbers) {
    char *file;
    size_t len;
    INT32 shift;
    FIND_PROGRAM_LINE (prog, file, len, shift, (*linep));
    if (file) return make_plain_file (file, len, shift, malloced);
  }

  return NULL;
}

/* Returns the file where the program is defined. The line of the
 * class start is written to linep, or 0 if the program is the top
 * level of the file. */
PMOD_EXPORT struct pike_string *get_program_line(struct program *prog,
						 INT32 *linep)
{
  struct pike_string *res = low_get_program_line(prog, linep);
  if (!res) {
    struct pike_string *dash;
    REF_MAKE_CONST_STRING(dash, "-");
    return dash;
  }
  return res;
}

#ifdef PIKE_DEBUG
/* Variant for convenient use from a debugger. */
void gdb_program_line (struct program *prog)
{
  INT32 line;
  char *file = low_get_program_line_plain (prog, &line, 0);
  fprintf (stderr, "%s:%d\n", file, line);
}
#endif

PMOD_EXPORT struct pike_string *low_get_line (PIKE_OPCODE_T *pc,
					      struct program *prog, INT32 *linep)
{
  linep[0] = 0;

  if (prog->program && prog->linenumbers) {
    ptrdiff_t offset = pc - prog->program;
    if ((offset < (ptrdiff_t)prog->num_program) && (offset >= 0)) {
      static char *file = NULL;
      static char *base, *cnt;
      static INT32 off,line,pid;
      static size_t len;
      static INT32 shift;

      if(prog->linenumbers != base || prog->id != pid || offset < off)
      {
	base = cnt = prog->linenumbers;
	off=line=0;
	pid=prog->id;
	file = 0;
      }else{
	if (cnt < prog->linenumbers + prog->num_linenumbers)
	  goto fromold;
      }

      while(cnt < prog->linenumbers + prog->num_linenumbers)
      {
	if(*cnt == 127)
	{
	  cnt++;
	  len = get_small_number(&cnt);
	  shift = *cnt;
	  file = ++cnt;
	  CHECK_FILE_ENTRY (prog, cnt, len, shift);
	  cnt += len<<shift;
	}
	off+=get_small_number(&cnt);
      fromold:
	if(off > offset) break;
	line+=get_small_number(&cnt);
      }
      linep[0]=line;
      if (file) {
	struct pike_string *res = begin_wide_shared_string(len, shift);
	memcpy(res->str, file, len<<shift);
	return end_shared_string(res);
      }
    }
  }

  return NULL;
}

/* This is to low_get_line as low_get_program_line_plain is to
 * low_get_program_line. */
PMOD_EXPORT char *low_get_line_plain (PIKE_OPCODE_T *pc, struct program *prog,
				      INT32 *linep, int malloced)
{
  linep[0] = 0;

  if (prog->program && prog->linenumbers) {
    ptrdiff_t offset = pc - prog->program;

    if ((offset < (ptrdiff_t)prog->num_program) && (offset >= 0)) {
      char *cnt = prog->linenumbers;
      INT32 off = 0, line = 0;
      char *file = NULL;
      size_t len = 0;
      INT32 shift = 0;

      while(cnt < prog->linenumbers + prog->num_linenumbers)
      {
	if(*cnt == 127)
	{
	  cnt++;
	  len = get_small_number(&cnt);
	  shift = *cnt;
	  file = ++cnt;
	  CHECK_FILE_ENTRY (prog, cnt, len, shift);
	  cnt += len<<shift;
	}
	off+=get_small_number(&cnt);
	if(off > offset) break;
	line+=get_small_number(&cnt);
      }
      linep[0]=line;

      if (file) return make_plain_file (file, len, shift, malloced);
    }
  }

  return NULL;
}


/*
 * return the file in which we were executing. pc should be the
 * program counter (i.e. the address in prog->program), prog the
 * current program, and line will be initialized to the line in that
 * file.
 */
PMOD_EXPORT struct pike_string *get_line(PIKE_OPCODE_T *pc,
					 struct program *prog, INT32 *linep)
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
						       int fun, INT32 *linep)
{
  if (o->prog) {
    struct reference *idref = o->prog->identifier_references + fun;
    struct program *p = PROG_FROM_PTR (o->prog, idref);
    struct identifier *id = p->identifiers + idref->identifier_offset;
    return low_get_line (p->program + id->func.offset, p, linep);
  }
  *linep = 0;
  return NULL;
}

void va_yyerror(const char *fmt, va_list args)
{
  char buf[8192];
  Pike_vsnprintf (buf, sizeof (buf), fmt, args);
  yyerror(buf);
}

void my_yyerror(const char *fmt,...)
{
  va_list args;
  va_start(args,fmt);
  va_yyerror (fmt, args);
  va_end(args);
}

struct pike_string *format_exception_for_error_msg (struct svalue *thrown)
{
  struct pike_string *s = NULL;

  push_svalue (thrown);
  SAFE_APPLY_MASTER ("describe_error", 1);

  if (sp[-1].type == T_STRING) {
    f_string_trim_all_whites(1);
    push_constant_text("\n");
    push_constant_text(" ");
    f_replace(3);
    return (--sp)->u.string;
  }
  else {
    pop_stack();
    return NULL;
  }
}

void handle_compile_exception (const char *yyerror_fmt, ...)
{
  struct svalue thrown;
  move_svalue (&thrown, &throw_value);
  throw_value.type = T_INT;

  if (yyerror_fmt) {
    va_list args;
    va_start (args, yyerror_fmt);
    va_yyerror (yyerror_fmt, args);
    va_end (args);
  }

  push_svalue(&thrown);
  low_safe_apply_handler("compile_exception", error_handler, compat_handler, 1);

  if (SAFE_IS_ZERO(sp-1)) {
    struct pike_string *s = format_exception_for_error_msg (&thrown);
    if (s) {
      my_yyerror("%S", s);
      free_string (s);
    }
  }

  pop_stack();
  free_svalue(&thrown);
}

extern void yyparse(void);

#ifdef PIKE_DEBUG
#define do_yyparse() do {				\
  struct svalue *save_sp=Pike_sp;			\
  yyparse();  /* Parse da program */			\
  if(save_sp != Pike_sp)				\
    Pike_fatal("yyparse() left %"PRINTPTRDIFFT"d droppings on the stack!\n",	\
	  Pike_sp - save_sp);				\
}while(0)
#else
#define do_yyparse() yyparse()
#endif

struct Supporter *current_supporter=0;


#ifdef PIKE_DEBUG

struct supporter_marker
{
  struct supporter_marker *next;
  void *data;
  int level, verified;
};

#undef EXIT_BLOCK
#define EXIT_BLOCK(P)
#undef COUNT_OTHER
#define COUNT_OTHER()

#undef INIT_BLOCK
#define INIT_BLOCK(X) do { (X)->level = (X)->verified = 0; }while(0)
PTR_HASH_ALLOC(supporter_marker, 128);

static int supnum;

#define SNUM(X) (get_supporter_marker((X))->level)

static void mark_supporters(struct Supporter *s)
{
  struct supporter_marker *m;

  if(!s) return;
  debug_malloc_touch(s);
  m=get_supporter_marker(s);

  if(m->level) return;
  m->level = -1;

  if(s->magic != 0x500b0127)
  {
#ifdef DEBUG_MALLOC
    describe(s);
#endif
    Pike_fatal("This is not a supporter (addr=%p, magic=%x)!\n",s,s->magic);
  }

  mark_supporters(s->dependants);
  mark_supporters(s->next_dependant);

  m->level=supnum++;

  mark_supporters(s->previous);
  mark_supporters(s->depends_on);
}

static void low_verify_supporters(struct Supporter *s)
{
  struct Supporter *ss;
  struct supporter_marker *m;

  if(!s) return;
  debug_malloc_touch(s);
  m=get_supporter_marker(s);

  if(m->verified) return;
  m->verified = 1;

  low_verify_supporters(s->dependants);
  low_verify_supporters(s->next_dependant);

#if 0
  fprintf(stderr, "low_verify_supporters %p%s, level %d: "
	  "previous %p, depends_on %p, dependants %p, next_dependant %p\n",
	  s, s == current_supporter ? " == current_supporter" : "",
	  m->level, s->previous, s->depends_on, s->dependants, s->next_dependant);
#endif

  if(s->previous && SNUM(s->previous) <= m->level)
    Pike_fatal("Que, numbers out of whack1\n");

  if(s->depends_on && SNUM(s->depends_on) <= m->level)
    Pike_fatal("Que, numbers out of whack2\n");

  for(ss=s->dependants;ss;ss=ss->next_dependant) {
    if (ss->depends_on != s)
      Pike_fatal("Dependant hasn't got depends_on set properly.\n");
    if(SNUM(ss) >= m->level)
      Pike_fatal("Que, numbers out of whack3\n");
  }

  low_verify_supporters(s->previous);
  low_verify_supporters(s->depends_on);
}

void verify_supporters()
{
  if(d_flag)
  {
    supnum=1;
    init_supporter_marker_hash();

#if 0
    fprintf(stderr, "verify_supporters start\n");
#endif

    mark_supporters(current_supporter);
    low_verify_supporters(current_supporter);
#ifdef DO_PIKE_CLEANUP
    {
      size_t e=0;
      struct supporter_marker *h;
      for(e=0;e<supporter_marker_hash_table_size;e++)
	while(supporter_marker_hash_table[e])
	  remove_supporter_marker(supporter_marker_hash_table[e]->data);
    }
#endif
    exit_supporter_marker_hash();

#if 0
    fprintf(stderr, "verify_supporters end\n");
#endif
  }
}
#else
#define verify_supporters();
#endif

void init_supporter(struct Supporter *s,
		    supporter_callback *fun,
		    void *data)
{
  verify_supporters();
#ifdef PIKE_DEBUG
  s->magic = 0x500b0127;
#endif
  s->previous=current_supporter;
  current_supporter=s;

  s->depends_on=0;
  s->dependants=0;
  s->next_dependant=0;
  s->fun=fun;
  s->data=data;
  s->prog=0;
  verify_supporters();
}

int unlink_current_supporter(struct Supporter *c)
{
  int ret=0;
#ifdef PIKE_DEBUG
  if(c != current_supporter)
    Pike_fatal("Previous unlink failed.\n");
#endif
  debug_malloc_touch(c);
  verify_supporters();
  if(c->depends_on)
  {
#ifdef PIKE_DEBUG
    struct Supporter *s;
    for (s = c->depends_on->dependants; s; s = s->next_dependant)
      if (s == c) Pike_fatal("Dependant already linked in.\n");
#endif
    ret++;
    c->next_dependant = c->depends_on->dependants;
    c->depends_on->dependants=c;
  }
  current_supporter=c->previous;
  verify_supporters();
  return ret;
}

void free_supporter(struct Supporter *c)
{
  verify_supporters();
  if (c->depends_on) {
    struct Supporter **s;
    for (s = &c->depends_on->dependants; *s; s = &(*s)->next_dependant)
      if (*s == c) {*s = c->next_dependant; break;}
    c->depends_on = 0;
  }
  verify_supporters();
}

int call_dependants(struct Supporter *s, int finish)
{
  int ok = 1;
  struct Supporter *tmp;
  verify_supporters();
  while((tmp=s->dependants))
  {
    s->dependants=tmp->next_dependant;
#ifdef PIKE_DEBUG
    tmp->next_dependant=0;
#endif
    verify_supporters();
    if (!tmp->fun(tmp->data, finish)) ok = 0;
    verify_supporters();
  }
  return ok;
}

int report_compiler_dependency(struct program *p)
{
  int ret=0;
  struct Supporter *c,*cc;

  if (p == Pike_compiler->new_program) {
    /* Depends on self... */
    return 0;
  }
  verify_supporters();
  if (Pike_compiler->flags & COMPILATION_FORCE_RESOLVE)
    return 0;
  for(cc=current_supporter;cc;cc=cc->previous)
  {
    if(cc->prog &&
       !(cc->prog->flags & PROGRAM_PASS_1_DONE))
    {
      c=cc->depends_on;
      if(!c) c=cc->previous;
      for(;c;c=c->previous)
      {
	if(c->prog == p)
	{
	  cc->depends_on=c;
	  verify_supporters();
	  ret++; /* dependency registred */
	}
      }
    }
  }
  verify_supporters();
  return ret;
}


struct compilation
{
  struct Supporter supporter;
  struct pike_string *prog;
  struct object *handler;
  int major, minor;
  struct program *target;
  struct object *placeholder;
  
  struct program *p;
  struct lex save_lex;
  int save_depth;
  int saved_threads_disabled;
  struct object *saved_handler;
  struct object *saved_compat_handler;
  dynamic_buffer used_modules_save;
  INT32 num_used_modules_save;
  struct mapping *resolve_cache_save;

  struct svalue default_module;
};

static void free_compilation(struct compilation *c)
{
  debug_malloc_touch(c);
  free_string(c->prog);
  if(c->handler) free_object(c->handler);
  if(c->target) free_program(c->target);
  if(c->placeholder) free_object(c->placeholder);
  free_svalue(& c->default_module);
  free_supporter(&c->supporter);
  free((char *)c);
  verify_supporters();
}

static void run_init(struct compilation *c)
{
  debug_malloc_touch(c);
  c->save_depth=compilation_depth;
  compilation_depth=-1;

  c->saved_handler = error_handler;
  if((error_handler = c->handler))
    add_ref(error_handler);

  c->saved_compat_handler = compat_handler;
  compat_handler=0;

  c->used_modules_save = used_modules;
  c->num_used_modules_save = Pike_compiler->num_used_modules;
  Pike_compiler->num_used_modules=0;

  c->resolve_cache_save = resolve_cache;
  resolve_cache = 0;

  c->save_lex=lex;

  lex.current_line=1;
  lex.current_file=make_shared_string("-");

  if (runtime_options & RUNTIME_STRICT_TYPES)
  {
    lex.pragmas = ID_STRICT_TYPES;
  } else {
    lex.pragmas = 0;
  }

  lex.end = c->prog->str + (c->prog->len << c->prog->size_shift);

  switch(c->prog->size_shift)
  {
    case 0: lex.current_lexer = yylex0; break;
    case 1: lex.current_lexer = yylex1; break;
    case 2: lex.current_lexer = yylex2; break;
    default:
      Pike_fatal("Program has bad shift %d!\n", c->prog->size_shift);
      break;
  }

  lex.pos=c->prog->str;
}

static void run_init2(struct compilation *c)
{
  debug_malloc_touch(c);
  Pike_compiler->compiler = c;

  initialize_buf(&used_modules);
  use_module(& c->default_module);

  Pike_compiler->compat_major=PIKE_MAJOR_VERSION;
  Pike_compiler->compat_minor=PIKE_MINOR_VERSION;

  if(c->major>=0)
    change_compiler_compatibility(c->major, c->minor);
}

static void run_exit(struct compilation *c)
{
  debug_malloc_touch(c);
  toss_buffer(&used_modules);
  used_modules = c->used_modules_save;

#ifdef PIKE_DEBUG
  if(Pike_compiler->num_used_modules)
    Pike_fatal("Failed to pop modules properly.\n");
#endif
  Pike_compiler->num_used_modules = c->num_used_modules_save ;

#ifdef PIKE_DEBUG
  if (compilation_depth != -1) {
    fprintf(stderr, "compile(): compilation_depth is %d\n",
	    compilation_depth);
  }
#endif /* PIKE_DEBUG */
  compilation_depth=c->save_depth;

  if (resolve_cache)
    free_mapping(resolve_cache);
  resolve_cache = c->resolve_cache_save;

  if (error_handler) free_object(error_handler);
  error_handler = c->saved_handler;

  if (compat_handler)  free_object(compat_handler);
  compat_handler = c->saved_compat_handler;

  free_string(lex.current_file);
  lex=c->save_lex;
  verify_supporters();
}

static void zap_placeholder(struct compilation *c)
{
  /* fprintf(stderr, "Destructing placeholder.\n"); */
  if (c->placeholder->storage) {
    yyerror("Placeholder already has storage!");
#if 0
    fprintf(stderr, "Placeholder already has storage!\n"
	    "placeholder: %p, storage: %p, prog: %p\n",
	    c->placeholder, c->placeholder->storage, c->placeholder->prog);
#endif
    debug_malloc_touch(c->placeholder);
    destruct(c->placeholder);
  } else {
    /* FIXME: Is this correct? */
    /* It would probably be nicer if it was possible to just call
     * destruct on the object, but this works too. -Hubbe
     */
    free_program(c->placeholder->prog);
    c->placeholder->prog = NULL;
    debug_malloc_touch(c->placeholder);
  }
  free_object(c->placeholder);
  c->placeholder=0;
  verify_supporters();
}

/* NOTE: Must not throw errors! */
static int run_pass1(struct compilation *c)
{
  int ret=0;

  debug_malloc_touch(c);
  run_init(c);

#if 0
  CDFPRINTF((stderr, "th(%ld) compile() starting compilation_depth=%d\n",
	     (long)th_self(),compilation_depth));
#endif

  if(c->placeholder && c->placeholder->prog != null_program) {
    yyerror("Placeholder object is not a null_program clone!");
    return 0;
  }
  debug_malloc_touch(c->placeholder);

  if(c->target && !(c->target->flags & PROGRAM_VIRGIN)) {
    yyerror("Placeholder program is not virgin!");
    return 0;
  }

  low_start_new_program(c->target,1,0,0,0);
  c->supporter.prog = Pike_compiler->new_program;

  CDFPRINTF((stderr,
	     "th(%ld) %p run_pass1() start: "
	     "threads_disabled:%d, compilation_depth:%d\n",
	     (long)th_self(), Pike_compiler->new_program,
	     threads_disabled, compilation_depth));

  run_init2(c);

  if(c->placeholder)
  {
    if(c->placeholder->prog != null_program)
    {
      yyerror("Placeholder argument is not a null_program clone!");
      c->placeholder=0;
      debug_malloc_touch(c->placeholder);
    }else{
      free_program(c->placeholder->prog);
      add_ref(c->placeholder->prog=Pike_compiler->new_program);
      debug_malloc_touch(c->placeholder);
    }
  }

#if 0
  CDFPRINTF((stderr, "th(%ld)   compile(): First pass\n",
	     (long)th_self()));
#endif

  do_yyparse();  /* Parse da program */

  if (!Pike_compiler->new_program->num_linenumbers) {
    /* The lexer didn't write an initial entry. */
    store_linenumber(0, lex.current_file);
#ifdef DEBUG_MALLOC
    if(strcmp(lex.current_file->str,"-"))
      debug_malloc_name(Pike_compiler->new_program, lex.current_file->str, 0);
#endif
  }

  CDFPRINTF((stderr, "th(%ld) %p run_pass1() done for %s\n",
	     (long)th_self(), Pike_compiler->new_program, lex.current_file->str));

  ret=unlink_current_supporter(& c->supporter);

  c->p=end_first_pass(0);

  run_exit(c);

  if(c->placeholder)
  {
    if(!c->p || (c->placeholder->storage))
    {
      debug_malloc_touch(c->placeholder);
      zap_placeholder(c);
    } else {
#ifdef PIKE_DEBUG
      if (c->placeholder->prog != c->p)
	Pike_fatal("Placeholder object got wrong program after first pass.\n");
#endif
      debug_malloc_touch(c->placeholder);
      c->placeholder->storage=c->p->storage_needed ?
	(char *)xalloc(c->p->storage_needed) :
	(char *)NULL;
      call_c_initializers(c->placeholder);
    }
  }

  verify_supporters();
  return ret;
}

void run_pass2(struct compilation *c)
{
  debug_malloc_touch(c);
  debug_malloc_touch(c->placeholder);

  run_init(c);
  low_start_new_program(c->p,2,0,0,0);
  free_program(c->p);
  c->p=0;

  run_init2(c);

  CDFPRINTF((stderr,
	     "th(%ld) %p run_pass2() start: "
	     "threads_disabled:%d, compilation_depth:%d\n",
	     (long)th_self(), Pike_compiler->new_program,
	     threads_disabled, compilation_depth));

  verify_supporters();

  do_yyparse();  /* Parse da program */

  CDFPRINTF((stderr, "th(%ld) %p run_pass2() done for %s\n",
	     (long)th_self(), Pike_compiler->new_program, lex.current_file->str));

  verify_supporters();

  c->p=end_program();

  run_exit(c);
}

static void run_cleanup(struct compilation *c, int delayed)
{
  debug_malloc_touch(c);
  debug_malloc_touch(c->placeholder);
#if 0 /* FIXME */
  if (threads_disabled != c->saved_threads_disabled) {
    Pike_fatal("compile(): threads_disabled:%d saved_threads_disabled:%d\n",
	  threads_disabled, c->saved_threads_disabled);
  }
#endif /* PIKE_DEBUG */

  exit_threads_disable(NULL);

  CDFPRINTF((stderr,
	     "th(%ld) %p run_cleanup(): threads_disabled:%d, compilation_depth:%d\n",
	     (long)th_self(), c->target, threads_disabled, compilation_depth));
  if (!c->p)
  {
    /* fprintf(stderr, "Destructing placeholder.\n"); */
    if(c->placeholder) {
      debug_malloc_touch(c->placeholder);
      zap_placeholder(c);
    }

    if(delayed && c->target)
    {
      struct program *p = c->target;
      /* Free the constants in the failed program, to untangle the
       * cyclic references we might have to this program, typically
       * in parent pointers in nested classes. */
      if (p->constants) {
	int i;
	for (i = 0; i < p->num_constants; i++) {
	  free_svalue(&p->constants[i].sval);
	  p->constants[i].sval.type = T_INT;
	}
      }

      /* We have to notify the master object that
       * a previous compile() actually failed, even
       * if we did not know it at the time
       */
      CDFPRINTF((stderr, "th(%ld) %p unregistering failed delayed compile.\n",
		 (long) th_self(), p));
      ref_push_program(p);
      /* FIXME: Shouldn't the compilation handler be used here? */
      SAFE_APPLY_MASTER("unregister",1);
      pop_stack();

#ifdef PIKE_DEBUG
      if (p->refs > 1) {
	/* Other programs can have indexed out constants from p, which
	 * might be broken themselves and/or keep references to p
	 * through the parent pointer. We should find all those other
	 * programs and invalidate them too, but how can that be done?
	 * The whole delayed compilation thingie is icky icky icky... :P
	 * /mast */
	fprintf(stderr, "Warning: Program still got %d "
		"external refs after unregister:\n", p->refs - 1);
	locate_references(p);
      }
#endif
    }
  }
  else
  {
    if (c->placeholder)
    {
      if (c->target->flags & PROGRAM_FINISHED) {
	JMP_BUF rec;
	/* Initialize the placeholder. */
#ifdef PIKE_DEBUG
	if (c->placeholder->prog != c->p)
	  Pike_fatal("Placeholder object got wrong program after second pass.\n");
#endif
	if(SETJMP(rec))
	{
	  handle_compile_exception (NULL);
	  debug_malloc_touch(c->placeholder);
	  zap_placeholder(c);
	}else{
	  debug_malloc_touch(c->placeholder);
	  call_pike_initializers(c->placeholder,0);
	}
	UNSETJMP(rec);
      }
      else {
	debug_malloc_touch(c->placeholder);
	zap_placeholder(c);
      }
    }
  }
  verify_supporters();
}

static int call_delayed_pass2(struct compilation *cc, int finish)
{
  int ok = 0;
  debug_malloc_touch(cc);

  CDFPRINTF((stderr, "th(%ld) %p %s delayed compile.\n",
	     (long) th_self(), cc->p, finish ? "continuing" : "cleaning up"));

  if(finish && cc->p) run_pass2(cc);
  run_cleanup(cc,1);
  
  debug_malloc_touch(cc);

#ifdef PIKE_DEBUG
  if(cc->supporter.dependants)
    Pike_fatal("Que???\n");
#endif
  if(cc->p) {
    ok = finish;
    free_program(cc->p); /* later */
  }

  CDFPRINTF((stderr, "th(%ld) %p delayed compile %s.\n",
	     (long) th_self(), cc->target, ok ? "done" : "failed"));

  free_compilation(cc);
  verify_supporters();

  return ok;
}

struct program *compile(struct pike_string *aprog,
			struct object *ahandler,/* error handler */
			int amajor, int aminor,
			struct program *atarget,
			struct object *aplaceholder)
{
  int delay, dependants_ok = 1;
  struct program *ret;
#ifdef PIKE_DEBUG
  ONERROR tmp;
#endif
  struct compilation *c=ALLOC_STRUCT(compilation);

  verify_supporters();

  CDFPRINTF((stderr, "th(%ld) %p compile() enter, placeholder=%p\n",
	     (long) th_self(), atarget, aplaceholder));

  debug_malloc_touch(c);
  c->p = NULL;
  add_ref(c->prog=aprog);
  if((c->handler=ahandler)) add_ref(ahandler);
  c->major=amajor;
  c->minor=aminor;
  if((c->target=atarget)) add_ref(atarget);
  if((c->placeholder=aplaceholder)) add_ref(aplaceholder);
  c->default_module.type=T_INT;

  if (c->handler)
  {
    if (safe_apply_handler ("get_default_module", c->handler, NULL,
			    0, BIT_MAPPING|BIT_OBJECT|BIT_ZERO)) {
      if(SAFE_IS_ZERO(Pike_sp-1))
      {
	pop_stack();
	ref_push_mapping(get_builtin_constants());
      }
    } else {
      ref_push_mapping(get_builtin_constants());
    }
  }else{
    ref_push_mapping(get_builtin_constants());
  }
  free_svalue(& c->default_module);
  c->default_module=Pike_sp[-1];
  dmalloc_touch_svalue(Pike_sp-1);
  Pike_sp--;

#ifdef PIKE_DEBUG
  SET_ONERROR(tmp, fatal_on_error,"Compiler exited with longjump!\n");
#endif

  low_init_threads_disable();
  c->saved_threads_disabled = threads_disabled;

  init_supporter(& c->supporter,
		 (supporter_callback *) call_delayed_pass2,
		 (void *)c);

  delay=run_pass1(c) && c->p;
  dependants_ok = call_dependants(& c->supporter, !!c->p );
#ifdef PIKE_DEBUG
  /* FIXME */
  UNSET_ONERROR(tmp);
#endif

  if(delay)
  {
    CDFPRINTF((stderr, "th(%ld) %p compile() finish later, placeholder=%p.\n",
	       (long) th_self(), c->target, c->placeholder));
    /* finish later */
    add_ref(c->p);
    verify_supporters();
    return c->p; /* freed later */
  }else{
    /* finish now */
    if(c->p) run_pass2(c);
    debug_malloc_touch(c);
    run_cleanup(c,0);
    
    ret=c->p;

    debug_malloc_touch(c);
    free_compilation(c);

    if (!dependants_ok) {
      CDFPRINTF((stderr, "th(%ld) %p compile() reporting failure "
		 "since a dependant failed.\n",
		 (long) th_self(), c->target));
      if (ret) free_program(ret);
      throw_error_object(low_clone(compilation_error_program), 0, 0, 0,
			 "Compilation failed.\n");
    }
    if(!ret) {
      CDFPRINTF((stderr, "th(%ld) %p compile() failed.\n",
		 (long) th_self(), c->target));
      throw_error_object(low_clone(compilation_error_program), 0, 0, 0,
			 "Compilation failed.\n");
    }
    debug_malloc_touch(ret);
#ifdef PIKE_DEBUG
    if (a_flag > 2) {
      dump_program_tables(ret, 0);
    }
#endif /* PIKE_DEBUG */
    verify_supporters();
    return ret;
  }
}

PMOD_EXPORT int pike_add_function2(const char *name, void (*cfun)(INT32),
				   const char *type, unsigned flags,
				   unsigned opt_flags)
{
  int ret;
  struct pike_string *name_tmp;
  struct pike_type *type_tmp;
  union idptr tmp;

  name_tmp=make_shared_string(name);
  type_tmp=parse_type(type);

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

PMOD_EXPORT int quick_add_function(const char *name,
				   int name_length,
				   void (*cfun)(INT32),
				   const char *type,
				   int type_length,
				   unsigned flags,
				   unsigned opt_flags)
{
  int ret;
  struct pike_string *name_tmp;
  struct pike_type *type_tmp;
  union idptr tmp;
/*  fprintf(stderr,"ADD_FUNC: %s\n",name); */
  name_tmp = make_shared_binary_string(name, name_length);
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

#undef THIS
#define THIS ((struct pike_trampoline *)(CURRENT_STORAGE))
struct program *pike_trampoline_program=0;

static void apply_trampoline(INT32 args)
{
  Pike_error("Internal error: Trampoline magic failed!\n");
}

static void not_trampoline(INT32 args)
{
  pop_n_elems(args);
  if (!THIS->frame || !THIS->frame->current_object ||
      !THIS->frame->current_object->prog) {
    push_int(1);
  } else {
    push_int(0);
  }
}

static void sprintf_trampoline (INT32 args)
{
  dynamic_buffer save_buf;
  dynbuf_string str;

  if (!args || sp[-args].type != T_INT || sp[-args].u.integer != 'O' ||
      !THIS->frame || !THIS->frame->current_object) {
    pop_n_elems (args);
    push_int (0);
    return;
  }
  pop_n_elems (args);

  ref_push_function (THIS->frame->current_object, THIS->func);
  init_buf(&save_buf);
  describe_svalue (sp - 1, 0, 0);
  str = complex_free_buf(&save_buf);
  pop_stack();
  push_string (make_shared_binary_string (str.str, str.len));
  free (str.str);
}

static void init_trampoline(struct object *o)
{
  THIS->frame=0;
}

static void exit_trampoline(struct object *o)
{
  if(THIS->frame)
  {
    free_pike_scope(THIS->frame);
    THIS->frame=0;
  }
}

static void gc_check_frame(struct pike_frame *f)
{
  if(f->flags & PIKE_FRAME_MALLOCED_LOCALS)
  {
    if(f->current_object)
      debug_gc_check (f->current_object, " as current_object in trampoline frame");
    if(f->context.prog)
      debug_gc_check (f->context.prog, " as context.prog in trampoline frame");
    if(f->context.parent)
      debug_gc_check (f->context.parent, " as context.parent in trampoline frame");
    debug_gc_check_svalues (f->locals, f->num_locals, " in locals of trampoline frame");
    if(f->scope && !debug_gc_check (f->scope, " as scope frame of trampoline frame"))
      gc_check_frame(f->scope);
  }
}

static void gc_check_trampoline(struct object *o)
{
  if (THIS->frame &&
      !debug_gc_check (THIS->frame, " as trampoline frame"))
    gc_check_frame(THIS->frame);
}

static void gc_recurse_frame(struct pike_frame *f)
{
  if(f->current_object) gc_recurse_object(f->current_object);
  if(f->context.prog)   gc_recurse_program(f->context.prog);
  if(f->context.parent) gc_recurse_object(f->context.parent);
  if(f->flags & PIKE_FRAME_MALLOCED_LOCALS)
    gc_recurse_svalues(f->locals,f->num_locals);
  if(f->scope)          gc_recurse_frame(f->scope);
}

static void gc_recurse_trampoline(struct object *o)
{
  if (THIS->frame) gc_recurse_frame(THIS->frame);
}


/* This placeholder should be used
 * in the first compiler pass to take the place
 * of unknown things
 */
struct program *placeholder_program;
struct object *placeholder_object;

void placeholder_index(INT32 args)
{
  pop_n_elems(args);
  ref_push_object(Pike_fp->current_object);
}

static void placeholder_sprintf (INT32 args)
{
  struct pike_string *s;

  if (!args || sp[-args].type != T_INT || sp[-args].u.integer != 'O') {
    pop_n_elems (args);
    push_int (0);
    return;
  }

  pop_n_elems (args);
  MAKE_CONST_STRING (s, "__placeholder_object");
  ref_push_string (s);
}

void init_program(void)
{
  size_t i;
  struct svalue key;
  struct svalue val;
  struct svalue id;
  init_program_blocks();

  MAKE_CONST_STRING(this_program_string,"this_program");
  MAKE_CONST_STRING(this_string,"this");
  MAKE_CONST_STRING(UNDEFINED_string,"UNDEFINED");

  lfun_ids = allocate_mapping(NUM_LFUNS);
  lfun_types = allocate_mapping(NUM_LFUNS);
  for (i=0; i < NELEM(lfun_names); i++) {
    lfun_strings[i] = make_shared_string(lfun_names[i]);

    id.type = T_INT;
    id.subtype = NUMBER_NUMBER;
    id.u.integer = i;
    key.type = T_STRING;
    key.u.string = lfun_strings[i];
    mapping_insert(lfun_ids, &key, &id);

    val.type = T_TYPE;
    val.u.type = make_pike_type(raw_lfun_types[i]);
    mapping_insert(lfun_types, &key, &val);
    free_type(val.u.type);
  }

  start_new_program();
  debug_malloc_touch(Pike_compiler->fake_object);
  debug_malloc_touch(Pike_compiler->fake_object->storage);
  ADD_STORAGE(struct pike_trampoline);
  ADD_FUNCTION("`()",apply_trampoline,tFunction,0);
  ADD_FUNCTION("`!",not_trampoline,tFunc(tVoid,tInt),0);
  ADD_FUNCTION("_sprintf", sprintf_trampoline,
	       tFunc(tInt tOr(tMapping,tVoid),tStr), 0);
  set_init_callback(init_trampoline);
  set_exit_callback(exit_trampoline);
  set_gc_check_callback(gc_check_trampoline);
  set_gc_recurse_callback(gc_recurse_trampoline);
  debug_malloc_touch(Pike_compiler->fake_object);
  debug_malloc_touch(Pike_compiler->fake_object->storage);
  pike_trampoline_program=end_program();

  /*! @decl constant __null_program
   *!
   *! Program used internally by the compiler.
   *!
   *! @seealso
   *!   @[__placeholder_object]
   */
  {
    struct svalue s;
    start_new_program();
    null_program=end_program();
    s.type=T_PROGRAM;
    s.u.program=null_program;
    low_add_constant("__null_program",&s);
    debug_malloc_touch(null_program);
  }

  /*! @decl constant __placeholder_object
   *!
   *! Object used internally by the compiler.
   *!
   *! @seealso
   *!   @[__null_program]
   */
  {
    struct svalue s;
    start_new_program();
    ADD_FUNCTION("`()", placeholder_index, tFuncV(tNone,tMix,tObj), 0);
    ADD_FUNCTION("`[]", placeholder_index, tFunc(tMix,tObj), 0);
    ADD_FUNCTION("_sprintf", placeholder_sprintf,
		 tFunc(tInt tOr(tMapping,tVoid),tStr), 0);
    placeholder_program=end_program();
    placeholder_object=fast_clone_object(placeholder_program);

    s.type=T_OBJECT;
    s.u.object=placeholder_object;
    low_add_constant("__placeholder_object",&s);
    debug_malloc_touch(placeholder_object);
  }
}

void cleanup_program(void)
{
  size_t e;

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
  if(resolve_cache)
  {
    free_mapping(dmalloc_touch (struct mapping *, resolve_cache));
    resolve_cache=0;
  }

  if(pike_trampoline_program)
  {
    free_program(pike_trampoline_program);
    pike_trampoline_program=0;
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
}

void gc_mark_program_as_referenced(struct program *p)
{
  debug_malloc_touch(p);

  if (p->flags & PROGRAM_AVOID_CHECK) {
    /* Program is in an inconsistent state.
     * don't look closer at it.
     */
    debug_malloc_touch(p);

    if (gc_mark(p)) {
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
  
  if(gc_mark(p))
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

      for(e=0;e<p->num_constants;e++)
	gc_mark_svalues(& p->constants[e].sval, 1);

      for(e=0;e<p->num_inherits;e++)
      {
	if(p->inherits[e].parent)
	  gc_mark_object_as_referenced(p->inherits[e].parent);

	if(e && p->inherits[e].prog)
	  gc_mark_program_as_referenced(p->inherits[e].prog);
      }

    } GC_LEAVE;
}

void real_gc_cycle_check_program(struct program *p, int weak)
{
  GC_CYCLE_ENTER(p, T_PROGRAM, weak) {
    int e;

    if (!(p->flags & PROGRAM_AVOID_CHECK))
    {
      for(e=0;e<p->num_constants;e++)
	gc_cycle_check_svalues(& p->constants[e].sval, 1);
      
      for(e=0;e<p->num_inherits;e++)
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
#ifdef PIKE_DEBUG
  extern void * check_for;
#endif
  
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
  
    for(e=0;e<p->num_constants;e++) {
      debug_gc_check_svalues (&p->constants[e].sval, 1, " as program constant");
    }
  
    for(e=0;e<p->num_inherits;e++)
    {
      if(p->inherits[e].parent)
      {
	debug_gc_check (p->inherits[e].parent,
			" as inherited parent object of a program");
      }

#ifdef PIKE_DEBUG
      if(d_flag && p->inherits[e].name && check_for != (void *)(ptrdiff_t)1)
	debug_gc_check (p->inherits[e].name, " as inherit name");
#endif

      if(e && p->inherits[e].prog)
	debug_gc_check (p->inherits[e].prog, " as inherited program of a program");
    }

#ifdef PIKE_DEBUG
    if(d_flag && check_for != (void *)(ptrdiff_t)1)
    {
      int e;
      for(e=0;e<(int)p->num_strings;e++)
	debug_gc_check (p->strings[e], " as program string");

      for(e=0;e<(int)p->num_identifiers;e++)
      {
	debug_gc_check (p->identifiers[e].name, " as program identifier name");
	debug_gc_check (p->identifiers[e].type, " as program identifier type");
      }
    }
#endif
  } GC_LEAVE;
}

unsigned gc_touch_all_programs(void)
{
  unsigned n = 0;
  struct program *p;
  struct program_state *ps;
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
	p->constants[e].sval.type=T_INT;
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
	if(p->constants[e].sval.type == T_PROGRAM && p->constants[e].sval.u.program == p)
	  tmp++;
      }
      if(tmp >= p->refs)
	gc_fatal(p, 1 ,"garbage collector failed to free program!!!\n");
    }
#endif

  return unreferenced;
}


void push_compiler_frame(int lexical_scope)
{
  struct compiler_frame *f;
  f=ALLOC_STRUCT(compiler_frame);
  f->previous=Pike_compiler->compiler_frame;
  f->lexical_scope=lexical_scope;
  f->current_type=0;
  f->current_return_type=0;

  f->current_number_of_locals=0;
  f->max_number_of_locals=0;
  f->min_number_of_locals=0;
  f->last_block_level=-1;

  f->current_function_number=-2; /* no function */
  f->recur_label=-1;
  f->is_inline=0;
  f->num_args=-1;
  f->opt_flags = OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND; /* FIXME: Should be 0. */
  Pike_compiler->compiler_frame=f;
}

void low_pop_local_variables(int level)
{
  while(Pike_compiler->compiler_frame->current_number_of_locals > level)
  {
    int e;
    e=--(Pike_compiler->compiler_frame->current_number_of_locals);
    free_string(Pike_compiler->compiler_frame->variable[e].name);
    free_type(Pike_compiler->compiler_frame->variable[e].type);
    if(Pike_compiler->compiler_frame->variable[e].def)
      free_node(Pike_compiler->compiler_frame->variable[e].def);

    free_string(Pike_compiler->compiler_frame->variable[e].file);
  }
}

void pop_local_variables(int level)
{
#if 1
  /* We need to save the variables Kuppo (but not their names) */
  if(level < Pike_compiler->compiler_frame->min_number_of_locals)
  {
    for(;level<Pike_compiler->compiler_frame->min_number_of_locals;level++)
    {
      free_string(Pike_compiler->compiler_frame->variable[level].name);
      copy_shared_string(Pike_compiler->compiler_frame->variable[level].name,
			 empty_pike_string);
    }
  }
#endif
  low_pop_local_variables(level);
}


void pop_compiler_frame(void)
{
  struct compiler_frame *f;

  f=Pike_compiler->compiler_frame;
#ifdef PIKE_DEBUG
  if(!f)
    Pike_fatal("Popping out of compiler frames\n");
#endif

  low_pop_local_variables(0);
  if(f->current_type)
    free_type(f->current_type);

  if(f->current_return_type)
    free_type(f->current_return_type);

  Pike_compiler->compiler_frame=f->previous;
  dmfree((char *)f);
}


#define GET_STORAGE_CACHE_SIZE 1024
static struct get_storage_cache
{
  INT32 oid, pid;
  ptrdiff_t offset;
} get_storage_cache[GET_STORAGE_CACHE_SIZE];

ptrdiff_t low_get_storage(struct program *o, struct program *p)
{
  INT32 oid, pid;
  ptrdiff_t offset;
  unsigned INT32 hval;

  if(!o) return -1;
  oid=o->id;
  pid=p->id;
  hval=oid*9248339 + pid;
  hval%=GET_STORAGE_CACHE_SIZE;
#ifdef PIKE_DEBUG
  if(hval>GET_STORAGE_CACHE_SIZE)
    Pike_fatal("hval>GET_STORAGE_CACHE_SIZE\n");
#endif
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

PMOD_EXPORT char *get_storage(struct object *o, struct program *p)
{
  ptrdiff_t offset;

#ifdef _REENTRANT
  if(d_flag) CHECK_INTERPRETER_LOCK();
#endif

  offset= low_get_storage(o->prog, p);
  if(offset == -1) return 0;
  return o->storage + offset;
}

struct program *low_program_from_function(struct program *p,
					  INT32 i)
{
  struct svalue *f;
  struct identifier *id=ID_FROM_INT(p, i);
  if(!IDENTIFIER_IS_CONSTANT(id->identifier_flags)) return 0;
  if(id->func.offset==-1) return 0;
  f=& PROG_FROM_INT(p,i)->constants[id->func.offset].sval;
  if(f->type!=T_PROGRAM) return 0;
  return f->u.program;
}

PMOD_EXPORT struct program *program_from_function(const struct svalue *f)
{
  if(f->type != T_FUNCTION) return 0;
  if(f->subtype == FUNCTION_BUILTIN) return 0;
  if(!f->u.object->prog) return 0;
  return low_program_from_function(f->u.object->prog, f->subtype);
}

PMOD_EXPORT struct program *program_from_svalue(const struct svalue *s)
{
  switch(s->type)
  {
    case T_OBJECT:
    {
      struct program *p = s->u.object->prog;
      int call_fun;

      if (!p) return 0;

#if 0
      if ((call_fun = FIND_LFUN(p, LFUN_CALL)) >= 0) {
	/* Get the program from the return type. */
	struct identifier *id = ID_FROM_INT(p, call_fun);
	/* FIXME: do it. */
	return 0;
      }
#endif
      push_svalue(s);
      f_object_program(1);
      p=program_from_svalue(Pike_sp-1);
      pop_stack();
      return p; /* We trust that there is a reference somewhere... */
    }

  case T_FUNCTION:
    return program_from_function(s);
  case T_PROGRAM:
    return s->u.program;
  default:
    return 0;
  }
}

#define FIND_CHILD_HASHSIZE 5003
struct find_child_cache_s
{
  INT32 pid,cid,id;
};

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

void yywarning(char *fmt, ...)
{
  char buf[4711];
  va_list args;

  /* If we have parse errors we might get erroneous warnings,
   * so don't print them.
   * This has the additional benefit of making it easier to
   * visually locate the actual error message.
   */
  if (Pike_compiler->num_parse_error) return;

  va_start(args,fmt);
  Pike_vsnprintf (buf, sizeof (buf), fmt, args);
  va_end(args);

  if ((error_handler && error_handler->prog) || get_master()) {
    ref_push_string(lex.current_file);
    push_int(lex.current_line);
    push_text(buf);

    low_safe_apply_handler("compile_warning", error_handler, compat_handler, 3);
    pop_stack();
  }
}



/* returns 1 if a implements b */
static int low_implements(struct program *a, struct program *b)
{
  int e;
  struct pike_string *s=findstring("__INIT");
  for(e=0;e<b->num_identifier_references;e++)
  {
    struct identifier *bid;
    int i;
    if (b->identifier_references[e].id_flags & (ID_STATIC|ID_HIDDEN))
      continue;		/* Skip static & hidden */
    bid = ID_FROM_INT(b,e);
    if(s == bid->name) continue;	/* Skip __INIT */
    i = find_shared_string_identifier(bid->name,a);
    if (i == -1) {
      if (b->identifier_references[e].id_flags & (ID_OPTIONAL))
	continue;		/* It's ok... */
#if 0
      fprintf(stderr, "Missing identifier \"%s\"\n", bid->name->str);
#endif /* 0 */
      return 0;
    }

    if (!pike_types_le(bid->type, ID_FROM_INT(a, i)->type)) {
      if(!match_types(ID_FROM_INT(a,i)->type, bid->type)) {
#if 0
	fprintf(stderr, "Identifier \"%s\" is incompatible.\n",
		bid->name->str);
#endif /* 0 */
	return 0;
      } else {
#if 0
	fprintf(stderr, "Identifier \"%s\" is not strictly compatible.\n",
		bid->name->str);
#endif /* 0 */
      }
    }
  }
  return 1;
}

#define IMPLEMENTS_CACHE_SIZE 4711
struct implements_cache_s { INT32 aid, bid, ret; };
static struct implements_cache_s implements_cache[IMPLEMENTS_CACHE_SIZE];

/* returns 1 if a implements b, but faster */
PMOD_EXPORT int implements(struct program *a, struct program *b)
{
  unsigned long hval;
  if(!a || !b) return -1;
  if(a==b) return 1;

  hval = a->id*9248339 + b->id;
  hval %= IMPLEMENTS_CACHE_SIZE;
#ifdef PIKE_DEBUG
  if(hval >= IMPLEMENTS_CACHE_SIZE)
    Pike_fatal("Implements_cache failed!\n");
#endif
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

/* Returns 1 if a is compatible with b */
static int low_is_compatible(struct program *a, struct program *b)
{
  int e;
  struct pike_string *s=findstring("__INIT");

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
    if (b->identifier_references[e].id_flags & (ID_STATIC|ID_HIDDEN))
      continue;		/* Skip static & hidden */

    /* FIXME: What if they aren't static & hidden in a? */

    bid = ID_FROM_INT(b,e);
    if(s == bid->name) continue;	/* Skip __INIT */
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
      return 0;
    }
  }
  return 1;
}

static struct implements_cache_s is_compatible_cache[IMPLEMENTS_CACHE_SIZE];
/* Returns 1 if a is compatible with b
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

  hval = aid*9248339 + bid;
  hval %= IMPLEMENTS_CACHE_SIZE;
#ifdef PIKE_DEBUG
  if(hval >= IMPLEMENTS_CACHE_SIZE)
    Pike_fatal("Implements_cache failed!\n");
#endif
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
  rhval = bid*9248339 + aid;
  rhval %= IMPLEMENTS_CACHE_SIZE;
#ifdef PIKE_DEBUG
  if(rhval >= IMPLEMENTS_CACHE_SIZE)
    Pike_fatal("Implements_cache failed!\n");
#endif
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

/* Explains why a is not compatible with b */
void yyexplain_not_compatible(struct program *a, struct program *b, int flags)
{
  int e;
  struct pike_string *s=findstring("__INIT");
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
    if (b->identifier_references[e].id_flags & (ID_STATIC|ID_HIDDEN))
      continue;		/* Skip static & hidden */

    /* FIXME: What if they aren't static & hidden in a? */

    bid = ID_FROM_INT(b,e);
    if(s == bid->name) continue;	/* Skip __INIT */
    i = find_shared_string_identifier(bid->name,a);
    if (i == -1) {
      continue;		/* It's ok... */
    }

    /* Note: Uses weaker check for constant integers. */
    if(((bid->run_time_type != PIKE_T_INT) ||
	(ID_FROM_INT(a, i)->run_time_type != PIKE_T_INT)) &&
       !match_types(ID_FROM_INT(a,i)->type, bid->type)) {
      if (flags & YYTE_IS_WARNING)
	yywarning("Identifier %S is incompatible.", bid->name);
      else
	my_yyerror("Identifier %S is incompatible.", bid->name);
      yytype_error(NULL, ID_FROM_INT(a,i)->type, bid->type, flags);
    }
  }
  END_CYCLIC();
  return;
}

/* Explains why a does not implement b */
void yyexplain_not_implements(struct program *a, struct program *b, int flags)
{
  int e;
  struct pike_string *s=findstring("__INIT");
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
    if (b->identifier_references[e].id_flags & (ID_STATIC|ID_HIDDEN))
      continue;		/* Skip static & hidden */
    bid = ID_FROM_INT(b,e);
    if(s == bid->name) continue;	/* Skip __INIT */
    i = find_shared_string_identifier(bid->name,a);
    if (i == -1) {
      if (b->identifier_references[e].id_flags & (ID_OPTIONAL))
	continue;		/* It's ok... */
      if(flags & YYTE_IS_WARNING)
	yywarning("Missing identifier %S.", bid->name);
      else
	my_yyerror("Missing identifier %S.", bid->name);
      continue;
    }

    if (!pike_types_le(bid->type, ID_FROM_INT(a, i)->type)) {
      if(!match_types(ID_FROM_INT(a,i)->type, bid->type)) {
	my_yyerror("Type of identifier %S does not match.", bid->name);
	yytype_error(NULL, ID_FROM_INT(a,i)->type, bid->type, 0);
      } else {
	yywarning("Type of identifier %S is not strictly compatible.",
		  bid->name);
	yytype_error(NULL, ID_FROM_INT(a,i)->type, bid->type, YYTE_IS_WARNING);
      }
      continue;
    }
  }
  END_CYCLIC();
}

PMOD_EXPORT void *parent_storage(int depth)
{
  struct external_variable_context loc;
  struct program *p;


  loc.o=Pike_fp->current_object;
  p=loc.o->prog;
  if(!p)
  {
    /* magic fallback */
    p=get_program_for_object_being_destructed(loc.o);
    if(!p)
    {
      Pike_error("Cannot access parent of destructed object.\n");
    }
  }

  if((Pike_fp->fun & 0xffff) == 0xffff)
    Pike_error("Cannot access parent storage!\n");

  loc.parent_identifier=Pike_fp->fun;
  loc.inherit=INHERIT_FROM_INT(p, Pike_fp->fun);
  
  find_external_context(&loc, depth);

  if (!loc.o->prog)
    Pike_error ("Cannot access storage of destructed parent object.\n");

  return loc.o->storage + loc.inherit->storage_offset;
}

PMOD_EXPORT void change_compiler_compatibility(int major, int minor)
{
  STACK_LEVEL_START(0);

  if(major == PIKE_MAJOR_VERSION && minor == PIKE_MINOR_VERSION)
  {
    push_int(0); /* optimization */
  } else {
    if(major == Pike_compiler->compat_major &&
       minor == Pike_compiler->compat_minor) {
      /* Optimization -- reuse the current compat handler. */
      if (compat_handler) {
	ref_push_object(compat_handler);
      } else {
	push_int(0);
      }
    } else {
      push_int(major);
      push_int(minor);
      SAFE_APPLY_MASTER("get_compilation_handler",2);
    }
  }

  STACK_LEVEL_CHECK(1);

  if(compat_handler)
  {
    free_object(compat_handler);
    compat_handler = NULL;
  }
  
  if((Pike_sp[-1].type == T_OBJECT) && (Pike_sp[-1].u.object->prog))
  {
    compat_handler = dmalloc_touch(struct object *, Pike_sp[-1].u.object);
    dmalloc_touch_svalue(Pike_sp-1);
    Pike_sp--;
  } else {
    pop_stack();
  }

  if (safe_apply_handler ("get_default_module", error_handler, compat_handler,
			  0, BIT_MAPPING|BIT_OBJECT|BIT_ZERO)) {
    if(Pike_sp[-1].type == T_INT)
    {
      pop_stack();
      ref_push_mapping(get_builtin_constants());
    }
  } else {
    ref_push_mapping(get_builtin_constants());
  }

  STACK_LEVEL_CHECK(1);

  if(Pike_compiler->num_used_modules)
  {
    free_svalue( (struct svalue *)used_modules.s.str );
    ((struct svalue *)used_modules.s.str)[0]=sp[-1];
    sp--;
    dmalloc_touch_svalue(sp);
    if(Pike_compiler->module_index_cache)
    {
      free_mapping(Pike_compiler->module_index_cache);
      Pike_compiler->module_index_cache=0;
    }
  }else{
    use_module(sp-1);
    pop_stack();
  }

  Pike_compiler->compat_major=major;
  Pike_compiler->compat_minor=minor;

  STACK_LEVEL_DONE(0);
}

#ifdef PIKE_USE_MACHINE_CODE

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

void make_program_executable(struct program *p)
{
#ifdef _WIN32
  DWORD old_prot;
  VirtualProtect((void *)p->program, p->num_program*sizeof(p->program[0]),
                 PAGE_EXECUTE_READWRITE, &old_prot);

#else  /* _WIN32 */

  mprotect((void *)p->program, p->num_program*sizeof(p->program[0]),
	   PROT_EXEC | PROT_READ | PROT_WRITE);

#endif /* _WIN32 */

#ifdef FLUSH_INSTRUCTION_CACHE
  FLUSH_INSTRUCTION_CACHE(p->program,
			  p->num_program*sizeof(p->program[0]));
#endif /* FLUSH_INSTRUCTION_CACHE */
}
#endif
