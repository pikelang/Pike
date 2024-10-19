/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "interpret.h"
#include "svalue.h"
#include "pike_macros.h"
#include "object.h"
#include "program.h"
#include "array.h"
#include "pike_error.h"
#include "constants.h"
#include "mapping.h"
#include "stralloc.h"
#include "multiset.h"
#include "pike_types.h"
#include "pike_rusage.h"
#include "operators.h"
#include "fsort.h"
#include "callback.h"
#include "gc.h"
#include "backend.h"
#include "main.h"
#include "pike_memory.h"
#include "threads.h"
#include "time_stuff.h"
#include "version.h"
#include "encode.h"
#include <ctype.h>
#include "module_support.h"
#include "opcodes.h"
#include "cyclic.h"
#include "signal_handler.h"
#include "builtin_functions.h"
#include "bignum.h"
#include "peep.h"
#include "docode.h"
#include "lex.h"
#include "pike_float.h"
#include "pike_compiler.h"
#include "port.h"
#include "siphash24.h"

#include <errno.h>

#ifdef HAVE_POLL
#ifdef HAVE_POLL_H
#include <poll.h>
#elif defined(HAVE_SYS_POLL_H)
#include <sys/poll.h>
#endif /* HAVE_POLL_H || HAVE_SYS_POLL_H */
#endif /* HAVE_POLL */

#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif

/* #define DIFF_DEBUG */
/* #define ENABLE_DYN_DIFF */

/*! @decl int equal(mixed a, mixed b)
 *!
 *!   This function checks if the values @[a] and @[b] are equivalent.
 *!
 *! @returns
 *!   If either of the values is an object the (normalized) result
 *!   of calling @[lfun::_equal()] will be returned.
 *!
 *!   Returns @expr{1@} if both values are false (zero, destructed objects,
 *!   prototype functions, etc).
 *!
 *!   Returns @expr{0@} (zero) if the values have different types.
 *!
 *!   Otherwise depending on the type of the values:
 *!   @mixed
 *!     @type int
 *!     @type float
 *!     @type string
 *!     @type program
 *!       Returns the same as @expr{a == b@}.
 *!     @type array
 *!     @type mapping
 *!     @type multiset
 *!     @type object
 *!       The contents of @[a] and @[b] are checked recursively, and
 *!       if all their contents are @[equal] and in the same place,
 *!       they are considered equal.
 *!
 *!       Note that for objects this case is only reached if neither
 *!       @[a] nor @[b] implements @[lfun::_equal()].
 *!     @type type
 *!       Returns @expr{(a <= b) && (b <= a)@}.
 *!  @endmixed
 *!
 *! @seealso
 *!   @[copy_value()], @[`==()]
 */
PMOD_EXPORT void f_equal(INT32 args)
{
  int i;
  if(args != 2)
    SIMPLE_WRONG_NUM_ARGS_ERROR("equal", 2);

  i=is_equal(Pike_sp-2,Pike_sp-1);
  pop_n_elems(args);
  push_int(i);
}

/*! @decl array aggregate(mixed ... elements)
 *!
 *!   Construct an array with the arguments as indices.
 *!
 *!   This function could be written in Pike as:
 *! @code
 *! array aggregate(mixed ... elems) { return elems; }
 *! @endcode
 *!
 *! @note
 *!   Arrays are dynamically allocated there is no need to declare them
 *!   like @expr{int a[10]=allocate(10);@} (and it isn't possible either) like
 *!   in C, just @expr{array(int) a=allocate(10);@} will do.
 *!
 *! @seealso
 *!   @[sizeof()], @[arrayp()], @[allocate()]
 */
PMOD_EXPORT void debug_f_aggregate(INT32 args)
{
  struct array *a;
  a=aggregate_array(args);
  push_array(a); /* beware, macro */
}

static node *optimize_f_aggregate(node *n)
{
  /* Split long argument lists into multiple function calls.
   *
   * aggregate(...) ==> `+(aggregate(...arg32), aggregate(arg33...), ...)
   *
   * Also removes splices.
   *
   * Note: We assume that the argument list is in left-recursive form.
   */
  node *args = CDR(n);
  node *new_args = NULL;
  node *add_args = NULL;
  int count;
  if (!args) return NULL;
  args->parent = NULL;
  for (count = 0; args->token == F_ARG_LIST; args = CAR(args)) {
    if (CDR(args) && CDR(args)->token == F_PUSH_ARRAY) {
      /* Splices have a weight of 16. */
      count += 16;
    } else {
      count++;
    }
    if (!CAR(args)) break;
    CAR(args)->parent = args;
  }
  if (args->token == F_PUSH_ARRAY) {
    /* Last argument is a splice */
    count += 16;
  } else if (args->token != F_ARG_LIST) {
    count++;
  }

  /* Ignore cases with 32 or less arguments. */
  if (count <= 32) {
    CDR(n)->parent = n;
    return NULL;
  }

  /*
   * Perform the actual rewrite.
   *
   * Start with the last arg, and work towards the first.
   */

  count = 0;
  if (args->token != F_ARG_LIST) {
    if (args->token == F_PUSH_ARRAY) {
      /* Splice operator. */
      add_args = copy_node(CAR(args));
    } else {
      new_args = copy_node(args);
      count = 1;
    }
    args = args->parent;
  }

  for(; args; args = args->parent) {
    if (!CDR(args)) continue;
    if (CDR(args)->token == F_PUSH_ARRAY) {
      if (count) {
	add_args = mknode(F_ARG_LIST, add_args,
			  mkapplynode(copy_node(CAR(n)), new_args));
	new_args = NULL;
	count = 0;
      }
      add_args = mknode(F_ARG_LIST, add_args, copy_node(CADR(args)));
    } else {
      new_args = mknode(F_ARG_LIST, new_args, copy_node(CDR(args)));
      count++;
      if (count > 31) {
	add_args = mknode(F_ARG_LIST, add_args,
			  mkapplynode(copy_node(CAR(n)), new_args));
	new_args = NULL;
	count = 0;
      }
    }
  }
  if (count) {
    add_args = mknode(F_ARG_LIST, add_args,
		      mkapplynode(copy_node(CAR(n)), new_args));
    new_args = NULL;
    count = 0;
  }
  CDR(n)->parent = n;
  return mkefuncallnode("`+", add_args);
}

#define MK_HASHMEM(NAME, TYPE)		ATTRIBUTE((const))	\
  static inline size_t NAME(const TYPE *str, ptrdiff_t len, ptrdiff_t maxn) \
  {                                                                         \
      size_t ret,c;                                                         \
                                                                            \
      ret = len*92873743;                                                   \
                                                                            \
      len = MINIMUM(maxn,len);                                              \
      for(; len>=0; len--)                                                  \
      {                                                                     \
          c=str++[0];                                                       \
          ret ^= ( ret << 4 ) + c ;                                         \
          ret &= 0x7fffffff;                                                \
      }                                                                     \
      return ret;                                                           \
  }

MK_HASHMEM(simple_hashmem, unsigned char)
MK_HASHMEM(simple_hashmem1, p_wchar1)
MK_HASHMEM(simple_hashmem2, p_wchar2)

/*! @decl int hash_7_4(string s)
 *! @decl int hash_7_4(string s, int max)
 *!
 *!   Return an integer derived from the string @[s]. The same string
 *!   always hashes to the same value, also between processes.
 *!
 *!   If @[max] is given, the result will be >= 0 and < @[max],
 *!   otherwise the result will be >= 0 and <= 0x7fffffff.
 *!
 *! @note
 *!   This function is provided for backward compatibility with
 *!   code written for Pike up and including version 7.4.
 *!
 *!   This function is byte-order dependant for wide strings.
 *!
 *! @seealso
 *!   @[hash_7_6()], @[hash_7_0]
 */
static void f_hash_7_4(INT32 args)
{
  size_t i = 0;
  struct pike_string *s = Pike_sp[-args].u.string;

  if(!args)
    SIMPLE_WRONG_NUM_ARGS_ERROR("7.4::hash",1);

  if(TYPEOF(Pike_sp[-args]) != T_STRING)
    SIMPLE_ARG_TYPE_ERROR("7.4::hash", 1, "string");

  i = simple_hashmem((unsigned char *)s->str, s->len<<s->size_shift,
		     100<<s->size_shift);

  if(args > 1)
  {
    if(TYPEOF(Pike_sp[1-args]) != T_INT)
      SIMPLE_ARG_TYPE_ERROR("7.4::hash",2,"int");

    if(!Pike_sp[1-args].u.integer)
      PIKE_ERROR("7.4::hash", "Modulo by zero.\n", Pike_sp, args);

    i%=(unsigned INT32)Pike_sp[1-args].u.integer;
  }
  pop_n_elems(args);
  push_int64(i);
}

ATTRIBUTE((const)) static inline size_t hashstr(const unsigned char *str, ptrdiff_t maxn)
{
  size_t ret,c;

  if(!(ret=str++[0]))
    return ret;
  for(; maxn>=0; maxn--)
  {
    c=str++[0];
    if(!c) break;
    ret ^= ( ret << 4 ) + c ;
    ret &= 0x7fffffff;
  }

  return ret;
}

/*! @decl int hash_7_0(string s)
 *! @decl int hash_7_0(string s, int max)
 *!
 *!   Return an integer derived from the string @[s]. The same string
 *!   always hashes to the same value, also between processes.
 *!
 *!   If @[max] is given, the result will be >= 0 and < @[max],
 *!   otherwise the result will be >= 0 and <= 0x7fffffff.
 *!
 *! @note
 *!   This function is provided for backward compatibility with
 *!   code written for Pike up and including version 7.0.
 *!
 *!   This function is not NUL-safe, and is byte-order dependant.
 *!
 *! @seealso
 *!   @[hash()], @[hash_7_4]
 */
static void f_hash_7_0( INT32 args )
{
  struct pike_string *s = Pike_sp[-args].u.string;
  unsigned int i;
  if(!args)
    SIMPLE_WRONG_NUM_ARGS_ERROR("7.0::hash",1);
  if(TYPEOF(Pike_sp[-args]) != T_STRING)
    SIMPLE_ARG_TYPE_ERROR("7.0::hash", 1, "string");

  if( s->size_shift )
  {
    f_hash_7_4( args );
    return;
  }

  i = (unsigned int)hashstr( (unsigned char *)s->str,
                             MINIMUM(100,s->len));
  if(args > 1)
  {
    if(TYPEOF(Pike_sp[1-args]) != T_INT)
      SIMPLE_ARG_TYPE_ERROR("7.0::hash",2,"int");

    if(!Pike_sp[1-args].u.integer)
      PIKE_ERROR("7.0::hash", "Modulo by zero.\n", Pike_sp, args);

    i%=(unsigned INT32)Pike_sp[1-args].u.integer;
  }
  pop_n_elems(args);
  push_int( i );
}

/*! @decl int hash_8_0(string s)
 *! @decl int hash_8_0(string s, int max)
 *!
 *!   Return an integer derived from the string @[s]. The same string
 *!   always hashes to the same value, also between processes,
 *!   architectures, and Pike versions (see compatibility notes below,
 *!   though).
 *!
 *!   If @[max] is given, the result will be >= 0 and < @[max],
 *!   otherwise the result will be >= 0 and <= 0x7fffffff.
 *!
 *! @deprecated
 *!
 *!  Use @[hash_value()] for in-process hashing (eg, for implementing
 *!    @[lfun::_hash()]) or one of the cryptographic hash functions.
 *!
 *! @note
 *!  This function is really bad at hashing strings. Similar string
 *!  often return similar hash values.
 *!
 *!  It is especially bad for url:s, paths and similarly formatted
 *!  strings.
 *!
 *! @note
 *!   The hash algorithm was changed in Pike 7.5. If you want a hash
 *!   that is compatible with Pike 7.4 and earlier, use @[hash_7_4()].
 *!   The difference only affects wide strings.
 *!
 *!   The hash algorithm was also changed in Pike 7.1. If you want a hash
 *!   that is compatible with Pike 7.0 and earlier, use @[hash_7_0()].
 *!
 *! @note
 *!   This hash function differs from the one provided by @[hash_value()],
 *!   in that @[hash_value()] returns a process specific value.
 *!
 *! @seealso
 *!   @[hash()], @[hash_7_0()], @[hash_7_4()], @[hash_value]
 */
static void f_hash_8_0(INT32 args)
{
  size_t i = 0;
  struct pike_string *s;

  if(!args)
    SIMPLE_WRONG_NUM_ARGS_ERROR("hash",1);

  if(TYPEOF(Pike_sp[-args]) != T_STRING)
    SIMPLE_ARG_TYPE_ERROR("hash", 1, "string");

  s = Pike_sp[-args].u.string;
  switch(s->size_shift) {
  case 0:
    i = simple_hashmem(STR0(s), s->len, 100);
    break;
  case 1:
    i = simple_hashmem1(STR1(s), s->len, 100);
    break;
  case 2:
    i = simple_hashmem2(STR2(s), s->len, 100);
    break;
  }

  if(args > 1)
  {
    if(TYPEOF(Pike_sp[1-args]) != T_INT)
      SIMPLE_ARG_TYPE_ERROR("hash_8_0",2,"int");

    if(Pike_sp[1-args].u.integer <= 0)
      PIKE_ERROR("hash_8_0", "Modulo < 1.\n", Pike_sp, args);

    i%=(unsigned INT32)Pike_sp[1-args].u.integer;
  }
  pop_n_elems(args);
  push_int64(i);
}

/*! @decl int hash(string s)
 *! @decl int hash(string s, int max)
 *!
 *!   Return an integer derived from the string @[s]. The same string
 *!   always hashes to the same value, also between processes,
 *!   architectures, and Pike versions (see compatibility notes below,
 *!   though).
 *!
 *!   If @[max] is given, the result will be >= 0 and < @[max],
 *!   otherwise the result will be >= 0 and <= 0x7fffffff.
 *!
 *! @note
 *!   The hash algorithm was changed in Pike 8.1. If you want a hash
 *!   that is compatible with Pike 8.0 and earlier, use @[hash_8_0()].
 *!
 *!   The hash algorithm was also changed in Pike 7.5. If you want a hash
 *!   that is compatible with Pike 7.4 and earlier, use @[hash_7_4()].
 *!   The difference with regards to @[hash_8_0()] only affects wide strings.
 *!
 *!   The hash algorithm was also changed in Pike 7.1. If you want a hash
 *!   that is compatible with Pike 7.0 and earlier, use @[hash_7_0()].
 *!
 *! @note
 *!   This hash function differs from the one provided by @[hash_value()],
 *!   in that @[hash_value()] returns a process specific value.
 *!
 *! @seealso
 *!   @[hash_7_0()], @[hash_7_4()], @[hash_8_0()], @[hash_value]
 */
PMOD_EXPORT void f_hash( INT32 args )
{
  size_t res;

  if( TYPEOF(Pike_sp[-args]) != PIKE_T_STRING )
      PIKE_ERROR("hash","Argument is not a string\n",Pike_sp,args);

  res = pike_string_siphash24(Pike_sp[-args].u.string, 0) & 0x7fffffff;

  if( args > 1 ) {
    if(TYPEOF(Pike_sp[1-args]) != T_INT)
      SIMPLE_ARG_TYPE_ERROR("hash",2,"int");

    if(Pike_sp[1-args].u.integer <= 0)
      PIKE_ERROR("hash", "Modulo < 1.\n", Pike_sp, args);

    res %= Pike_sp[1-args].u.integer;
  }

  pop_n_elems(args);
  push_int(res);
}

/*! @decl int hash_value (mixed value)
 *!
 *! Return a hash value for the argument. It's an integer in the
 *! native integer range.
 *!
 *! The hash will be the same for the same value in the running
 *! process only (the memory address is typically used as the basis
 *! for the hash value).
 *!
 *! If the value is an object with an @[lfun::__hash], that function
 *! is called and its result returned.
 *!
 *! @note
 *!   This is the hashing method used by mappings.
 *!
 *! @seealso
 *!   @[lfun::__hash()]
 */
void f_hash_value(INT32 args)
{
  size_t h;

  if(!args)
    SIMPLE_WRONG_NUM_ARGS_ERROR("hash_value",1);

  h = hash_svalue (Pike_sp - args);
  pop_n_elems (args);
  /* NB: We assume that INT_TYPE has the same width as size_t. */
  push_int (h);
}

/*! @decl mixed copy_value(mixed value)
 *!
 *!   Copy a value recursively.
 *!
 *!   If the result value is changed destructively (only possible for
 *!   multisets, arrays and mappings) the copied value will not be changed.
 *!
 *!   The resulting value will always be equal to the copied (as tested with
 *!   the function @[equal()]), but they may not the the same value (as tested
 *!   with @[`==()]).
 *!
 *! @seealso
 *!   @[equal()]
 */
PMOD_EXPORT void f_copy_value(INT32 args)
{
  if(!args)
    SIMPLE_WRONG_NUM_ARGS_ERROR("copy_value",1);

  pop_n_elems(args-1);
  push_undefined();	/* Placeholder */
  copy_svalues_recursively_no_free(Pike_sp-1,Pike_sp-2,1,0);
  free_svalue(Pike_sp-2);
  move_svalue (Pike_sp - 2, Pike_sp - 1);
  Pike_sp--;
  dmalloc_touch_svalue(Pike_sp-1);
}

struct case_info {
  INT32 low;	/* low end of range. */
  INT16 mode;
  INT16 data;
};

#define CIM_NONE	   0	/* Case-less */
#define CIM_UPPERDELTA	   1	/* Upper-case, delta to lower-case in data */
#define CIM_LOWERDELTA	   2	/* Lower-case, -delta to upper-case in data */
#define CIM_CASEBIT	   3	/* Some case, case mask in data */
#define CIM_CASEBITOFF	   4	/* Same as above, but also offset by data */
#define CIM_LONGUPPERDELTA 5	/* Upper-case, delta + 0x7fff. */
#define CIM_LONGLOWERDELTA 6	/* Lower-case, delta + 0x7fff. */

static const struct case_info case_info[] = {
#include "case_info.h"
  { 0x7fffffff, CIM_NONE, 0x0000, },	/* End sentinel. */
};

static const struct case_info *find_ci(INT32 c)
{
  static const struct case_info *cache = NULL;
  const struct case_info *ci = cache;
  int lo = 0;
  int hi = NELEM(case_info);

  if ((c < 0) || (c > 0xeffff)) {
    /* Negative, or plane 15 and above. */
    return NULL;
  }

  if ((ci) && (ci[0].low <= c) && (ci[1].low > c)) {
    return ci;
  }

  while (lo != hi-1) {
    int mid = (lo + hi)/2;
    if (case_info[mid].low < c) {
      lo = mid;
    } else if (case_info[mid].low == c) {
      lo = mid;
      break;
    } else {
      hi = mid;
    }
  }
  return(cache = (const struct case_info *)case_info + lo);
}

static const struct case_info *find_ci_shift0(INT32 c)
{
  static const struct case_info *cache = NULL;
  const struct case_info *ci = cache;
  int lo = 0;
  int hi = CASE_INFO_SHIFT0_HIGH;

  if ((c < 0) || (c > 0xefffff)) {
    /* Negative, or plane 15 and above. */
    return NULL;
  }

  if ((ci) && (ci[0].low <= c) && (ci[1].low > c)) {
    return ci;
  }

  while (lo != hi-1) {
    int mid = (lo + hi)>>1;
    if (case_info[mid].low < c) {
      lo = mid;
    } else if (case_info[mid].low == c) {
      lo = mid;
      break;
    } else {
      hi = mid;
    }
  }
  return(cache = (const struct case_info *)case_info + lo);
}

#define DO_LOWER_CASE(C) do {\
    INT32 c = C; \
    if(c<0xb5){if(c >= 'A' && c <= 'Z' ) C=c+0x20; } \
    /*else if(c==0xa77d) C=0x1d79;*/ else { \
    const struct case_info *ci = find_ci(c); \
    if (ci) { \
      switch(ci->mode) { \
      case CIM_NONE: case CIM_LOWERDELTA: case CIM_LONGLOWERDELTA: break; \
      case CIM_UPPERDELTA: C = c + ci->data; break; \
      case CIM_CASEBIT: C = c | ci->data; break; \
      case CIM_CASEBITOFF: C = ((c - ci->data) | ci->data) + ci->data; break; \
      case CIM_LONGUPPERDELTA: \
        C = c + ci->data + ( ci->data>0 ? 0x7fff : -0x8000 ); break; \
      DO_IF_DEBUG( default: Pike_fatal("lower_case(): Unknown case_info mode: %d\n", ci->mode); ) \
    } \
   }} \
  } while(0)

#define DO_LOWER_CASE_SHIFT0(C) do {\
    INT32 c = C; \
    if(c<0xb5){if(c >= 'A' && c <= 'Z' ) C=c+0x20;}else {\
    const struct case_info *ci = find_ci_shift0(c); \
    if (ci) { \
      switch(ci->mode) { \
      case CIM_NONE: case CIM_LOWERDELTA: break; \
      case CIM_UPPERDELTA: C = c + ci->data; break; \
      case CIM_CASEBIT: C = c | ci->data; break; \
      case CIM_CASEBITOFF: C = ((c - ci->data) | ci->data) + ci->data; break; \
      DO_IF_DEBUG( default: Pike_fatal("lower_case(): Unknown case_info mode: %d\n", ci->mode); ) \
    } \
   }} \
  } while(0)

#define DO_UPPER_CASE(C) do {\
    INT32 c = C; \
    if(c<0xb5){if(c >= 'a' && c <= 'z' ) C=c-0x20; } \
    /*else if(c==0x1d79) C=0xa77d;*/ else {\
    const struct case_info *ci = find_ci(c); \
    if (ci) { \
      switch(ci->mode) { \
      case CIM_NONE: case CIM_UPPERDELTA: case CIM_LONGUPPERDELTA: break; \
      case CIM_LOWERDELTA: C = c - ci->data; break; \
      case CIM_CASEBIT: C = c & ~ci->data; break; \
      case CIM_CASEBITOFF: C = ((c - ci->data)& ~ci->data) + ci->data; break; \
      case CIM_LONGLOWERDELTA: \
        C = c - ci->data - ( ci->data>0 ? 0x7fff : -0x8000 ); break; \
      DO_IF_DEBUG( default: Pike_fatal("upper_case(): Unknown case_info mode: %d\n", ci->mode); ) \
    } \
   }} \
  } while(0)

#define DO_UPPER_CASE_SHIFT0(C) do {\
    INT32 c = C; \
    if(c<0xb5){if(c >= 'a' && c <= 'z' ) C=c-0x20;}else {\
    const struct case_info *ci = find_ci_shift0(c); \
    if (ci) { \
      switch(ci->mode) { \
      case CIM_NONE: case CIM_UPPERDELTA: break; \
      case CIM_LOWERDELTA: C = c - ci->data; break; \
      case CIM_CASEBIT: C = c & ~ci->data; break; \
      case CIM_CASEBITOFF: C = ((c - ci->data)& ~ci->data) + ci->data; break; \
      DO_IF_DEBUG( default: Pike_fatal("lower_case(): Unknown case_info mode: %d\n", ci->mode); ) \
    } \
   }} \
  } while(0)

/*! @decl string lower_case(string s)
 *! @decl int lower_case(int c)
 *!
 *!   Convert a string or character to lower case.
 *!
 *! @returns
 *!   Returns a copy of the string @[s] with all upper case characters
 *!   converted to lower case, or the character @[c] converted to lower
 *!   case.
 *!
 *! @note
 *!   Assumes the string or character to be coded according to
 *!   ISO-10646 (aka Unicode). If they are not, @[Charset.decoder] can
 *!   do the initial conversion for you.
 *!
 *! @note
 *!   Prior to Pike 7.5 this function only accepted strings.
 *!
 *! @seealso
 *!   @[upper_case()], @[Charset.decoder]
 */
PMOD_EXPORT void f_lower_case(INT32 args)
{
  ptrdiff_t i;
  struct pike_string *orig;
  struct pike_string *ret;

  check_all_args("lower_case", args, BIT_STRING|BIT_INT, 0);

  if (TYPEOF(Pike_sp[-args]) == T_INT) {
    /* NOTE: Performs the case change in place. */
    DO_LOWER_CASE(Pike_sp[-args].u.integer);
    pop_n_elems(args-1);
    return;
  }

  orig = Pike_sp[-args].u.string;

  if( orig->flags & STRING_IS_LOWERCASE )
      return;

  ret = begin_wide_shared_string(orig->len, orig->size_shift);

  memcpy(ret->str, orig->str, orig->len << orig->size_shift);

  i = orig->len;

  if (!orig->size_shift) {
    p_wchar0 *str = STR0(ret);

    while(i--) {
      DO_LOWER_CASE_SHIFT0(str[i]);
    }
  } else if (orig->size_shift == 1) {
    p_wchar1 *str = STR1(ret);

    while(i--) {
      DO_LOWER_CASE(str[i]);
    }
  } else if (orig->size_shift == 2) {
    p_wchar2 *str = STR2(ret);

    while(i--) {
      DO_LOWER_CASE(str[i]);
    }
#ifdef PIKE_DEBUG
  } else {
    Pike_fatal("lower_case(): Bad string shift:%d\n", orig->size_shift);
#endif
  }

  ret = end_shared_string(ret);
  ret->flags |= STRING_IS_LOWERCASE;
  pop_n_elems(args);
  push_string(ret);
}

/*! @decl string upper_case(string s)
 *! @decl int upper_case(int c)
 *!
 *!   Convert a string or character to upper case.
 *!
 *! @returns
 *!   Returns a copy of the string @[s] with all lower case characters
 *!   converted to upper case, or the character @[c] converted to upper
 *!   case.
 *!
 *! @note
 *!   Assumes the string or character to be coded according to
 *!   ISO-10646 (aka Unicode). If they are not, @[Charset.decoder] can
 *!   do the initial conversion for you.
 *!
 *! @note
 *!   Prior to Pike 7.5 this function only accepted strings.
 *!
 *! @seealso
 *!   @[lower_case()], @[Charset.decoder]
 */
PMOD_EXPORT void f_upper_case(INT32 args)
{
  ptrdiff_t i;
  struct pike_string *orig;
  struct pike_string *ret;
  check_all_args("upper_case", args, BIT_STRING|BIT_INT, 0);

  if (TYPEOF(Pike_sp[-args]) == T_INT) {
    /* NOTE: Performs the case change in place. */
    DO_UPPER_CASE(Pike_sp[-args].u.integer);
    pop_n_elems(args-1);
    return;
  }

  orig = Pike_sp[-args].u.string;
  if( orig->flags & STRING_IS_UPPERCASE )
  {
      return;
  }

  ret=begin_wide_shared_string(orig->len,orig->size_shift);
  memcpy(ret->str, orig->str, orig->len << orig->size_shift);

  i = orig->len;

  if (!orig->size_shift) {
    p_wchar0 *str = STR0(ret);

    while(i--) {
      if(str[i]!=0xff && str[i]!=0xb5) {
	DO_UPPER_CASE_SHIFT0(str[i]);
      } else {

	/* Ok, so our shiftsize 0 string contains 0xff or 0xb5 which
	   prompts for a shiftsize 1 string. */
	int j = orig->len;
	struct pike_string *wret = begin_wide_shared_string(j, 1);
	p_wchar1 *wstr = STR1(wret);

	/* Copy what we have done */
	while(--j>i)
	  wstr[j] = str[j];

	/* upper case the rest */
	i++;
	while(i--)
	  switch( str[i] ) {
	  case 0xff: wstr[i] = 0x178; break;
	  case 0xb5: wstr[i] = 0x39c; break;
	  default:
	    DO_UPPER_CASE_SHIFT0(str[i]);
	    wstr[i] = str[i];
	    break;
	  }

	/* Discard the too narrow string and use the new one instead. */
	do_free_unlinked_pike_string(ret);
	ret = wret;
	break;
      }
    }
  } else if (orig->size_shift == 1) {
    p_wchar1 *str = STR1(ret);

    while(i--) {
      DO_UPPER_CASE(str[i]);
    }
  } else if (orig->size_shift == 2) {
    p_wchar2 *str = STR2(ret);

    while(i--) {
      DO_UPPER_CASE(str[i]);
    }
#ifdef PIKE_DEBUG
  } else {
    Pike_fatal("lower_case(): Bad string shift:%d\n", orig->size_shift);
#endif
  }

  pop_n_elems(args);
  ret = end_shared_string(ret);
  ret->flags |= STRING_IS_UPPERCASE;
  push_string(ret);
}

/*! @decl void random_seed(int seed)
 *!
 *!   This function sets the initial value for the random generator.
 *!
 *! @seealso
 *!   @[random()]
 *!
 *! @deprecated
 *!   @[Random.Deterministic]
 */
static void f_random_seed(INT32 args)
{
  pop_n_elems(args);
}

/*! @decl int query_num_arg()
 *!
 *!   Returns the number of arguments given when the previous function was
 *!   called.
 *!
 *!   This is useful for functions that take a variable number of arguments.
 *!
 *! @seealso
 *!   @[call_function()]
 */
void f_query_num_arg(INT32 args)
{
  pop_n_elems(args);
  push_int(Pike_fp ? Pike_fp->args : 0);
}

/*! @decl int search(string haystack, string|int needle, int|void start, @
 *!                  int|void end)
 *! @decl int search(array haystack, mixed needle, int|void start, int|void end)
 *! @decl mixed search(mapping haystack, mixed needle, mixed|void start)
 *! @decl mixed search(object haystack, mixed needle, mixed|void start, @
 *!                    mixed ... extra_args)
 *!
 *!   Search for @[needle] in @[haystack].
 *!
 *! @param haystack
 *!   Item to search in. This can be one of:
 *!   @mixed
 *!     @type string
 *!       When @[haystack] is a string @[needle] must be a string or an int,
 *!       and the first occurrence of the string or int is returned.
 *!
 *!     @type array
 *!       When @[haystack] is an array, @[needle] is compared only to
 *!       one value at a time in @[haystack].
 *!
 *!     @type mapping
 *!       When @[haystack] is a mapping, @[search()] tries to find the index
 *!       connected to the data @[needle]. That is, it tries to lookup the
 *!       mapping backwards.
 *!
 *!     @type object
 *!       When @[haystack] is an object implementing @[lfun::_search()],
 *!       the result of calling @[lfun::_search()] with @[needle], @[start]
 *!       and any @[extra_args] will be returned.
 *!
 *!       If @[haystack] is an object that doesn't implement @[lfun::_search()]
 *!       it is assumed to be an @[Iterator], and implement
 *!       @[Iterator()->index()], @[Iterator()->value()], and
 *!       @[Iterator()->next()]. @[search()] will then start comparing
 *!       elements with @[`==()] until a match with @[needle] is found.
 *!       If @[needle] is found @[haystack] will be advanced to the element,
 *!       and the iterator index will be returned. If @[needle] is not
 *!       found, @[haystack] will be advanced to the end.
 *!   @endmixed
 *!
 *! @param start
 *!   If the optional argument @[start] is present search is started at
 *!   this position. This has no effect on mappings.
 *!
 *! @param end
 *!   If the optional argument @[end] is present, the search will terminate
 *!   at this position (exclusive) if not found earlier.
 *!
 *! @returns
 *!   Returns the position of @[needle] in @[haystack] if found.
 *!
 *!   If not found the returned value depends on the type of @[haystack]:
 *!   @mixed
 *!     @type string|array
 *!       @expr{-1@}.
 *!     @type mapping|object(Iterator)
 *!       @[UNDEFINED].
 *!     @type object
 *!       The value returned by @[lfun::_search()].
 *!   @endmixed
 *!
 *! @note
 *!   If @[start] is supplied to an iterator object without an
 *!   @[lfun::_search()], @[haystack] will need to implement
 *!   @[Iterator()->set_index()].
 *!
 *! @note
 *!   For mappings and object @[UNDEFINED] will be returned when not found.
 *!   In all other cases @expr{-1@} will be returned when not found.
 *!
 *! @seealso
 *!   @[indices()], @[values()], @[zero_type()], @[has_value()],
 *!   @[has_prefix()], @[has_suffix()]
 */
PMOD_EXPORT void f_search(INT32 args)
{
  ptrdiff_t start;
  ptrdiff_t end;

  if(args < 2)
    SIMPLE_WRONG_NUM_ARGS_ERROR("search", 2);

  switch(TYPEOF(Pike_sp[-args]))
  {
  case T_STRING:
  {
    struct pike_string *haystack = Pike_sp[-args].u.string;

    start=0;
    end = haystack->len;
    if(args > 2)
    {
      if(TYPEOF(Pike_sp[2-args]) != T_INT)
	SIMPLE_ARG_TYPE_ERROR("search", 3, "int");

      start=Pike_sp[2-args].u.integer;
      if(start<0) {
        bad_arg_error("search", args, 3, "int(0..)", Pike_sp+2-args,
		   "Start must be greater or equal to zero.\n");
      }

      if (args > 3) {
	if(TYPEOF(Pike_sp[3-args]) != T_INT)
	  SIMPLE_ARG_TYPE_ERROR("search", 4, "int");

	if (Pike_sp[3-args].u.integer < end) {
	  end = Pike_sp[3-args].u.integer;
	  if(start<0) {
            bad_arg_error("search", args, 4, "int(0..)",
			  Pike_sp+3-args,
			  "End must be greater or equal to zero.\n");
	  }
	}
      }
    }

    if(haystack->len < start)
      bad_arg_error("search", args, 3, "int(0..)", Pike_sp-args,
		    "Start must not be greater than the "
		    "length of the string.\n");

    if (end <= start) {
      pop_n_elems(args);
      push_int(-1);
      return;
    }

    if ((TYPEOF(Pike_sp[1-args]) == T_INT) ||
	((TYPEOF(Pike_sp[1-args]) == T_STRING) &&
	 (Pike_sp[1-args].u.string->len == 1))) {
      INT_TYPE val;
      if (TYPEOF(Pike_sp[1-args]) == T_INT) {
	val = Pike_sp[1-args].u.integer;
      } else {
	val = index_shared_string(Pike_sp[1-args].u.string, 0);
      }

      if( !string_range_contains( haystack, val )  )
      {
          pop_n_elems(args);
          push_int( -1 );
          return;
      }
      switch(Pike_sp[-args].u.string->size_shift) {
      case 0:
	{
	  p_wchar0 *str = STR0(haystack);
	  if (val >= 256) {
	    start = -1;
	    break;
	  }
	  while (start < end) {
	    if (str[start] == val) break;
	    start++;
	  }
	}
	break;
      case 1:
	{
	  p_wchar1 *str = STR1(haystack);
	  if (val >= 65536) {
	    start = -1;
	    break;
	  }
	  while (start < end) {
	    if (str[start] == val) break;
	    start++;
	  }
	}
	break;
      case 2:
	{
	  p_wchar2 *str = STR2(haystack);
	  while (start < end) {
	    if (str[start] == (p_wchar2)val) break;
	    start++;
	  }
	}
	break;
      }
    } else if(TYPEOF(Pike_sp[1-args]) == T_STRING) {
      /* Handle searching for the empty string. */
      if (Pike_sp[1-args].u.string->len) {
	start = string_search(haystack,
			      Pike_sp[1-args].u.string,
			      start);
	end -= Pike_sp[1-args].u.string->len-1;
      }
    } else {
      SIMPLE_ARG_TYPE_ERROR("search", 2, "string | int");
    }
    pop_n_elems(args);
    if (start >= end) {
      start = -1;
    }
    push_int64(start);
    break;
  }

  case T_ARRAY:
    start=0;
    end = Pike_sp[-args].u.array->size;
    if(args > 2)
    {
      if(TYPEOF(Pike_sp[2-args]) != T_INT)
	SIMPLE_ARG_TYPE_ERROR("search", 3, "int");

      start=Pike_sp[2-args].u.integer;
      if(start<0) {
        bad_arg_error("search", args, 3, "int(0..)", Pike_sp+2-args,
		   "Start must be greater or equal to zero.\n");
      }

      if (args > 3) {
	if(TYPEOF(Pike_sp[3-args]) != T_INT)
	  SIMPLE_ARG_TYPE_ERROR("search", 4, "int");

	if (Pike_sp[3-args].u.integer < end) {
	  end = Pike_sp[3-args].u.integer;
	  if(end<0) {
            bad_arg_error("search", args, 4, "int(0..)",
			  Pike_sp+3-args,
			  "End must be greater or equal to zero.\n");
	  }
	}
      }
    }
    start=array_search(Pike_sp[-args].u.array,Pike_sp+1-args,start);
    pop_n_elems(args);
    if (start >= end) {
      start = -1;
    }
    push_int64(start);
    break;

  case T_MAPPING:
    if(args > 2) {
      mapping_search_no_free(Pike_sp,Pike_sp[-args].u.mapping,Pike_sp+1-args,Pike_sp+2-args);
    } else {
      mapping_search_no_free(Pike_sp,Pike_sp[-args].u.mapping,Pike_sp+1-args,0);
    }
    free_svalue(Pike_sp-args);
    Pike_sp[-args]=*Pike_sp;
    dmalloc_touch_svalue(Pike_sp);
    pop_n_elems(args-1);
    return;

  case T_OBJECT:
    {
      struct program *p;
      if ((p = (Pike_sp[-args].u.object->prog))) {
	struct object *o = Pike_sp[-args].u.object;
	int id_level = p->inherits[SUBTYPEOF(Pike_sp[-args])].identifier_level;
	int id;
	int next, ind;
	p = p->inherits[SUBTYPEOF(Pike_sp[-args])].prog;

	/* NOTE: Fake lfun! */
	id = low_find_lfun(p, LFUN__SEARCH);
	/* First try lfun::_search(). */
	if (id >= 0) {
	  apply_low(o, id + id_level, args-1);
	  stack_pop_n_elems_keep_top(1);
	  return;
	}

	/* Check if we have an iterator. */
	if (((id = find_identifier("value", p)) >= 0) &&
	    ((next = find_identifier("next", p)) >= 0) &&
	    ((ind = find_identifier("index", p)) >= 0)) {
	  /* We have an iterator. */

	  id += id_level;
	  next += id_level;
	  ind += id_level;

	  /* Set the start position if needed. */
	  if (args > 2) {
	    int fun = find_identifier("set_index", p);
	    if (fun < 0)
              Pike_error ("Cannot call unknown function \"set_index\".\n");
	    apply_low(o, fun + id_level, args-2);
	    pop_stack();
	  }

	  /* At this point we have two values on the stack. */

	  while(1) {
	    apply_low(o, id, 0);
	    if (is_eq(Pike_sp-2, Pike_sp-1)) {
	      /* Found. */
	      apply_low(o, ind, 0);
	      stack_pop_n_elems_keep_top(3);
	      return;
	    }
	    apply_low(o, next, 0);
	    if (UNSAFE_IS_ZERO(Pike_sp-1)) {
	      /* Not found. */
	      pop_n_elems(4);
	      /* FIXME: Should probably indicate not found in some other way.
	       *        On the other hand, the iterator should be false now.
	       */
	      push_undefined();
	      return;
	    }
	    pop_n_elems(2);
	  }
	}
      }
    }
    /* FALLTHRU */
  default:
    SIMPLE_ARG_TYPE_ERROR("search", 1, "string|array|mapping|object");
  }
}

/*! @decl int has_prefix(string|object s, string prefix)
 *!
 *!   Returns @expr{1@} if the string @[s] starts with @[prefix],
 *!   returns @expr{0@} (zero) otherwise.
 *!
 *!   When @[s] is an object, it needs to implement
 *!   @[lfun::_sizeof()] and @[lfun::`[]].
 *!
 *! @seealso
 *!    @[has_suffix()], @[has_value()], @[search()]
 */
PMOD_EXPORT void f_has_prefix(INT32 args)
{
  struct pike_string *a, *b;

  if(args!=2)
    SIMPLE_WRONG_NUM_ARGS_ERROR("has_prefix", 2);
  if((TYPEOF(Pike_sp[-args]) != T_STRING) &&
     (TYPEOF(Pike_sp[-args]) != T_OBJECT))
    SIMPLE_ARG_TYPE_ERROR("has_prefix", 1, "string|object");
  if(TYPEOF(Pike_sp[1-args]) != T_STRING)
    SIMPLE_ARG_TYPE_ERROR("has_prefix", 2, "string");

  b = Pike_sp[1-args].u.string;

  if (TYPEOF(Pike_sp[-args]) == T_OBJECT) {
    ptrdiff_t i;
    struct object *o = Pike_sp[-args].u.object;
    int inherit_no = SUBTYPEOF(Pike_sp[-args]);

    if (!o->prog || FIND_LFUN(o->prog, LFUN__SIZEOF) < 0) {
      Pike_error("Object in argument 1 lacks lfun::_sizeof().\n");
    }

    apply_lfun(o, LFUN__SIZEOF, 0);
    if ((TYPEOF(Pike_sp[-1]) != T_INT) || (Pike_sp[-1].u.integer < b->len)) {
      pop_n_elems(args + 1);
      push_int(0);
      return;
    }

    for (i = 0; i < b->len; i++) {
      p_wchar2 ch = index_shared_string(b, i);
      Pike_sp[-1].u.integer = i;
      /* Note: Integers do not need to be freed. */
      object_index_no_free(Pike_sp-1, o, inherit_no, Pike_sp-1);
      if (TYPEOF(Pike_sp[-1]) != PIKE_T_INT) {
	Pike_error("Unexepected value returned from index operator.\n");
      }
      if (ch != Pike_sp[-1].u.integer) {
	pop_n_elems(args + 1);
	push_int(0);
	return;
      }
    }
    pop_n_elems(args+1);
    push_int(1);
    return;
  }

  a = Pike_sp[-args].u.string;

  /* First handle some common special cases. */
  if ((b->len > a->len) || (b->size_shift > a->size_shift)
      || !string_range_contains_string(a, b)) {
    pop_n_elems(args);
    push_int(0);
    return;
  }

  /* Trivial cases. */
  if ((a == b)||(!b->len)) {
    pop_n_elems(args);
    push_int(1);
    return;
  }

  if (a->size_shift == b->size_shift) {
    int res = !memcmp(a->str, b->str, b->len << b->size_shift);
    pop_n_elems(args);
    push_int(res);
    return;
  }

  /* At this point a->size_shift > b->size_shift */
#define TWO_SHIFTS(S1, S2)	((S1)|((S2)<<2))
  switch(TWO_SHIFTS(a->size_shift, b->size_shift)) {
#define CASE_SHIFT(S1, S2) \
  case TWO_SHIFTS(S1, S2): \
    { \
      PIKE_CONCAT(p_wchar,S1) *s1 = PIKE_CONCAT(STR,S1)(a); \
      PIKE_CONCAT(p_wchar,S2) *s2 = PIKE_CONCAT(STR,S2)(b); \
      ptrdiff_t len = b->len; \
      while(len-- && (s1[len] == s2[len])) \
	; \
      pop_n_elems(args); \
      push_int(len == -1); \
      return; \
    } \
    break

    CASE_SHIFT(1,0);
    CASE_SHIFT(2,0);
    CASE_SHIFT(2,1);
  }
#undef CASE_SHIFT
#undef TWO_SHIFTS
}

/*! @decl int has_suffix(string s, string suffix)
 *!
 *!   Returns @expr{1@} if the string @[s] ends with @[suffix],
 *!   returns @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!    @[has_prefix()], @[has_value()], @[search()]
 */
PMOD_EXPORT void f_has_suffix(INT32 args)
{
  struct pike_string *a, *b;

  if(args!=2)
    SIMPLE_WRONG_NUM_ARGS_ERROR("has_suffix", 2);
  if(TYPEOF(Pike_sp[-args]) != T_STRING)
    SIMPLE_ARG_TYPE_ERROR("has_suffix", 1, "string");
  if(TYPEOF(Pike_sp[1-args]) != T_STRING)
    SIMPLE_ARG_TYPE_ERROR("has_suffix", 2, "string");

  a = Pike_sp[-args].u.string;
  b = Pike_sp[1-args].u.string;

  /* First handle some common special cases. */
  if ((b->len > a->len) || (b->size_shift > a->size_shift)
      || !string_range_contains_string(a, b)) {
    pop_n_elems(args);
    push_int(0);
    return;
  }

  /* Trivial cases. */
  if ((a == b)||(!b->len)) {
    pop_n_elems(args);
    push_int(1);
    return;
  }

  if (a->size_shift == b->size_shift) {
    int res = !memcmp(a->str + ((a->len - b->len)<<b->size_shift), b->str,
		      b->len << b->size_shift);
    pop_n_elems(args);
    push_int(res);
    return;
  }

  /* At this point a->size_shift > b->size_shift */
#define TWO_SHIFTS(S1, S2)	((S1)|((S2)<<2))
  switch(TWO_SHIFTS(a->size_shift, b->size_shift)) {
#define CASE_SHIFT(S1, S2) \
  case TWO_SHIFTS(S1, S2): \
    { \
      PIKE_CONCAT(p_wchar,S1) *s1 = PIKE_CONCAT(STR,S1)(a) + a->len - b->len; \
      PIKE_CONCAT(p_wchar,S2) *s2 = PIKE_CONCAT(STR,S2)(b); \
      ptrdiff_t len = b->len; \
      while(len-- && (s1[len] == s2[len])) \
	; \
      pop_n_elems(args); \
      push_int(len == -1); \
      return; \
    } \
    break

    CASE_SHIFT(1,0);
    CASE_SHIFT(2,0);
    CASE_SHIFT(2,1);
  }
#undef CASE_SHIFT
#undef TWO_SHIFTS
}

/*! @decl int has_index(string haystack, int index)
 *! @decl int has_index(array haystack, int index)
 *! @decl int has_index(mapping|multiset|object|program haystack, mixed index)
 *!
 *!   Search for @[index] in @[haystack].
 *!
 *! @returns
 *!   Returns @expr{1@} if @[index] is in the index domain of @[haystack],
 *!   or @expr{0@} (zero) if not found.
 *!
 *!   This function is equivalent to (but sometimes faster than):
 *!
 *! @code
 *! search(indices(haystack), index) != -1
 *! @endcode
 *!
 *! @note
 *!   A negative index in strings and arrays as recognized by the
 *!   index operators @expr{`[]()@} and @expr{`[]=()@} is not considered
 *!   a proper index by @[has_index()]
 *!
 *! @seealso
 *!   @[has_value()], @[has_prefix()], @[has_suffix()], @[indices()],
 *!   @[search()], @[values()], @[zero_type()]
 */
PMOD_EXPORT void f_has_index(INT32 args)
{
  int t = 0;

  if(args != 2)
    SIMPLE_WRONG_NUM_ARGS_ERROR("has_index", 2);

  switch(TYPEOF(Pike_sp[-2]))
  {
    case T_STRING:
      if(TYPEOF(Pike_sp[-1]) == T_INT)
	t = (0 <= Pike_sp[-1].u.integer && Pike_sp[-1].u.integer < Pike_sp[-2].u.string->len);

      pop_n_elems(args);
      push_int(t);
      break;

    case T_ARRAY:
      if(TYPEOF(Pike_sp[-1]) == T_INT)
	t = (0 <= Pike_sp[-1].u.integer && Pike_sp[-1].u.integer < Pike_sp[-2].u.array->size);

      pop_n_elems(args);
      push_int(t);
      break;

    case T_MAPPING:
        t=!!low_mapping_lookup( Pike_sp[-2].u.mapping, Pike_sp-1 );
        pop_n_elems(2);
        push_int(t);
        break;

    case T_MULTISET:
        t = multiset_member( Pike_sp[-2].u.multiset, Pike_sp-1 );
        pop_n_elems(2);
        push_int(t);
        break;

    case T_OBJECT:
    case T_PROGRAM:
      /* FIXME: If the object behaves like an array, it will throw an
	 error for non-valid indices. Therefore it's not a good idea
	 to use the index operator.

	 Maybe we should use object->_has_index(index) provided that
	 the object implements it.

	 /Noring */
      /* If it is an iterator object we may want to use the iterator
         interface to look for the index. */

      stack_swap();
      f_indices(1);
      stack_swap();
      f_search(2);

      if(TYPEOF(Pike_sp[-1]) == T_INT)
	Pike_sp[-1].u.integer = (Pike_sp[-1].u.integer != -1);
      else
	PIKE_ERROR("has_index",
		   "Function `search' gave incorrect result.\n", Pike_sp, args);
      break;

    default:
      SIMPLE_ARG_TYPE_ERROR ("has_index", 1,
			     "string|array|mapping|multiset|object|program");
  }
}

/*! @decl int has_value(string haystack, string value)
 *! @decl int has_value(string haystack, int value)
 *! @decl int has_value(array|mapping|object|program haystack, mixed value)
 *!
 *!   Search for @[value] in @[haystack].
 *!
 *! @returns
 *!   Returns @expr{1@} if @[value] is in the value domain of @[haystack],
 *!   or @expr{0@} (zero) if not found.
 *!
 *!   This function is in all cases except when both arguments are strings
 *!   equivalent to (but sometimes faster than):
 *!
 *! @code
 *! search(values(@[haystack]), @[value]) != -1
 *! @endcode
 *!
 *!   If both arguments are strings, @[has_value()] is equivalent to:
 *!
 *! @code
 *! search(@[haystack], @[value]) != -1
 *! @endcode
 *!
 *! @seealso
 *!   @[has_index()], @[indices()], @[search()], @[has_prefix()],
 *!   @[has_suffix()], @[values()], @[zero_type()]
 */
PMOD_EXPORT void f_has_value(INT32 args)
{
  if(args != 2)
    SIMPLE_WRONG_NUM_ARGS_ERROR("has_value", 2);

  switch(TYPEOF(Pike_sp[-2]))
  {
    case T_MAPPING:
      f_search(2);
      f_zero_type(1);

      if(TYPEOF(Pike_sp[-1]) == T_INT)
	Pike_sp[-1].u.integer = !Pike_sp[-1].u.integer;
      break;

    case T_PROGRAM:
    case T_OBJECT:
      /* FIXME: It's very sad that we always have to do linear search
	 with `values' in case of objects. The problem is that we cannot
	 use `search' directly since it's undefined whether it returns
	 -1 (array) or 0 (mapping) during e.g. some data type emulation.

	 Maybe we should use object->_has_value(value) provided that
	 the object implements it.

	 /Noring */

      /* FALLTHRU */

    case T_MULTISET:
      /* FIXME: This behavior for multisets isn't clean. It should be
       * compat only. */
      stack_swap();
      f_values(1);
      stack_swap();

      /* FALLTHRU */

    case T_STRING:   /* Strings are odd. /Noring */
    case T_ARRAY:
      f_search(2);

      if(TYPEOF(Pike_sp[-1]) == T_INT)
	Pike_sp[-1].u.integer = (Pike_sp[-1].u.integer != -1);
      break;

    default:
      SIMPLE_ARG_TYPE_ERROR ("has_value", 1, "string|array|mapping|object|program");
  }
}

/*! @decl void add_constant(string name, mixed value)
 *! @decl void add_constant(string name)
 *!
 *!   Add a new predefined constant.
 *!
 *!   This function is often used to add builtin functions.
 *!   All programs compiled after the @[add_constant()] function has been
 *!   called can access @[value] by the name @[name].
 *!
 *!   If there is a constant called @[name] already, it will be replaced by
 *!   by the new definition. This will not affect already compiled programs.
 *!
 *!   Calling @[add_constant()] without a value will remove that name from
 *!   the list of constants. As with replacing, this will not affect already
 *!   compiled programs.
 *!
 *! @seealso
 *!   @[all_constants()]
 */
PMOD_EXPORT void f_add_constant(INT32 args)
{
  if(args<1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("add_constant", 1);

  if(TYPEOF(Pike_sp[-args]) != T_STRING)
    SIMPLE_ARG_TYPE_ERROR("add_constant", 1, "string");

  if(args>1)
  {
    dmalloc_touch_svalue(Pike_sp-args+1);
    low_add_efun(Pike_sp[-args].u.string, Pike_sp-args+1);
  }else{
    low_add_efun(Pike_sp[-args].u.string, 0);
  }
  pop_n_elems(args);
}

/*! @decl string combine_path(string path, string ... paths)
 *! @decl string combine_path_unix(string path, string ... paths)
 *! @decl string combine_path_nt(string path, string ... paths)
 *! @decl string combine_path_amigaos(string path, string ... paths)
 *!
 *!   Concatenate a number of paths to a straightforward path without
 *!   any @expr{"//"@}, @expr{"/.."@} or @expr{"/."@}. If any path
 *!   argument is absolute then the result is absolute and the
 *!   preceding arguments are ignored. If the result is relative then
 *!   it might have leading @expr{".."@} components. If the last
 *!   nonempty argument ends with a directory separator then the
 *!   result ends with that too. If all components in a relative path
 *!   disappear due to subsequent @expr{".."@} components then the
 *!   result is @expr{"."@}.
 *!
 *!   @[combine_path_unix()] concatenates in UNIX style, which also is
 *!   appropriate for e.g. URL:s ("/" separates path components and
 *!   absolute paths start with "/"). @[combine_path_nt()]
 *!   concatenates according to NT filesystem conventions ("/" and "\"
 *!   separates path components and there might be a drive letter in
 *!   front of absolute paths). @[combine_path_amigaos()] concatenates
 *!   according to AmigaOS filesystem conventions.
 *!
 *!   @[combine_path()] is equivalent to @[combine_path_unix()] on UNIX-like
 *!   operating systems, and equivalent to @[combine_path_nt()] on NT-like
 *!   operating systems, and equivalent to @[combine_path_amigaos()] on
 *!   AmigaOS-like operating systems.
 *!
 *! @seealso
 *!   @[getcwd()], @[Stdio.append_path()]
 */

#define NT_COMBINE_PATH
#include "combine_path.h"

#define UNIX_COMBINE_PATH
#include "combine_path.h"

#define AMIGAOS_COMBINE_PATH
#include "combine_path.h"

static ptrdiff_t find_last_path_separator(struct pike_string *path,
					  p_wchar2 sep,
					  ptrdiff_t pos)
{
  switch(path->size_shift) {
  case eightbit:
    {
      p_wchar0 *str = STR0(path);
      while (pos--) {
	if (str[pos] == sep) return pos;
      }
    }
    break;
  case sixteenbit:
    {
      p_wchar1 *str = STR1(path);
      while (pos--) {
	if (str[pos] == sep) return pos;
      }
    }
    break;
  case thirtytwobit:
    {
      p_wchar2 *str = STR2(path);
      while (pos--) {
	if (str[pos] == sep) return pos;
      }
    }
    break;
  default:
    Pike_fatal("Unsupported string width.\n");
    break;
  }
  return -1;
}

/*! @decl string dirname(string path)
 *!
 *! Returns all but the last segment of a path. Some example inputs and
 *! outputs:
 *!
 *! @xml{<matrix>
 *! <r><c><b>Expression</b></c><c><b>Value</b></c></r>
 *! <r><c>dirname("/a/b")</c><c>"/a"</c></r>
 *! <r><c>dirname("/a/")</c><c>"/a"</c></r>
 *! <r><c>dirname("/a")</c><c>"/"</c></r>
 *! <r><c>dirname("/")</c><c>"/"</c></r>
 *! <r><c>dirname("")</c><c>""</c></r>
 *! </matrix>@}
 *!
 *! @seealso
 *! @[basename()], @[explode_path()]
 */
PMOD_EXPORT void f_dirname(INT32 args)
{
  struct pike_string *s = NULL;
  ptrdiff_t pos, pos2 = -1;

#ifdef __NT__
  check_all_args("dirname", args, BIT_STRING, 0);
  push_text("\\");
  push_text("/");
  f_replace(3);
#endif

  get_all_args("dirname", args, "%t", &s);

  pos = find_last_path_separator(s, '/', s->len);
  if (pos < 0) {
    ref_push_string(empty_pike_string);
  } else if (pos) {
    push_string(string_slice(s, 0, pos));
  } else {
    push_text("/");
  }
  stack_pop_n_elems_keep_top(args);
}

/*! @decl string basename(string path)
 *!
 *! Returns the last segment of a path.
 *!
 *! @seealso
 *! @[dirname()], @[explode_path()]
 */
PMOD_EXPORT void f_basename(INT32 args)
{
  struct pike_string *s = NULL;
  ptrdiff_t pos, pos2 = -1;

  get_all_args("basename", args, "%t", &s);

  pos = find_last_path_separator(s, '/', s->len);
#ifdef __NT__
  pos2 = find_last_path_separator(s, '\\', s->len);
  if (pos2 > pos) pos = pos2;
#endif /* __NT__ */
  if (pos < 0) {
    return;
  }
  push_string(string_slice(s, pos+1, s->len - (pos+1)));
  stack_pop_n_elems_keep_top(args);
}


/*! @decl int zero_type(mixed a)
 *!
 *!   Return the type of zero.
 *!
 *!   There are many types of zeros out there, or at least there are two.
 *!   One is returned by normal functions, and one returned by mapping
 *!   lookups and @[find_call_out()] when what you looked for wasn't there.
 *!   The only way to separate these two kinds of zeros is @[zero_type()].
 *!
 *! @returns
 *!   When doing a @[find_call_out()] or mapping lookup, @[zero_type()] on
 *!   this value will return @expr{1@} if there was no such thing present in
 *!   the mapping, or if no such @tt{call_out@} could be found.
 *!
 *!   If the argument to @[zero_type()] is a destructed object or a function
 *!   in a destructed object, @expr{2@} will be returned.
 *!
 *!   In all other cases @[zero_type()] will return @expr{0@} (zero).
 *!
 *! @seealso
 *!   @[find_call_out()]
 */
PMOD_EXPORT void f_zero_type(INT32 args)
{
  if(args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("zero_type",1);

  if(IS_DESTRUCTED(Pike_sp-args))
  {
    pop_n_elems(args);
    push_int(NUMBER_DESTRUCTED);
  }
  else if(TYPEOF(Pike_sp[-args]) != T_INT)
  {
    pop_n_elems(args);
    push_int(0);
  }
  else
  {
    pop_n_elems(args-1);
    Pike_sp[-1].u.integer = SUBTYPEOF(Pike_sp[-1]);
    SET_SVAL_SUBTYPE(Pike_sp[-1], NUMBER_NUMBER);
  }
}

static int generate_arg_for(node *n)
{
  if(count_args(CDR(n)) != 1) return 0;
  if(do_docode(CDR(n),DO_NOT_COPY) != 1)
    Pike_fatal("Count args was wrong in generate_zero_type().\n");
  return 1;
}

static int generate_zero_type(node *n)
{
  struct compilation *c = THIS_COMPILATION;
  if( generate_arg_for( n ) )
      emit0(F_ZERO_TYPE);
  else
      return 0;
  return 1;
}

static int generate_undefinedp(node *n)
{
  struct compilation *c = THIS_COMPILATION;
  if( generate_arg_for(n) )
      emit0(F_UNDEFINEDP);
  else
      return 0;
  return 1;
}

static int generate_destructedp(node *n)
{
  struct compilation *c = THIS_COMPILATION;
  if( generate_arg_for(n) )
      emit0(F_DESTRUCTEDP);
  else
      return 0;
  return 1;
}

/*
 * Some wide-strings related functions
 */

/*! @decl string(0..255) string_to_unicode(string s, int(0..2)|void byteorder)
 *!
 *!   Converts a string into an UTF16 compliant byte-stream.
 *!
 *! @param s
 *!   String to convert to UTF16.
 *!
 *! @param byteorder
 *!   Byte-order for the output. One of:
 *!   @int
 *!     @value 0
 *!       Network (aka big-endian) byte-order (default).
 *!     @value 1
 *!       Little-endian byte-order.
 *!     @value 2
 *!       Native byte-order.
 *!   @endint
 *!
 *! @note
 *!   Throws an error if characters not legal in an UTF16 stream are
 *!   encountered. Valid characters are in the range 0x00000 - 0x10ffff,
 *!   except for characters 0xfffe and 0xffff.
 *!
 *!   Characters in range 0x010000 - 0x10ffff are encoded using surrogates.
 *!
 *! @seealso
 *!   @[Charset.decoder()], @[string_to_utf8()], @[unicode_to_string()],
 *!   @[utf8_to_string()]
 */
PMOD_EXPORT void f_string_to_unicode(INT32 args)
{
  struct pike_string *in;
  struct pike_string *out = NULL;
  ptrdiff_t len;
  ptrdiff_t i;
  unsigned INT_TYPE byteorder = 0;

  get_all_args("string_to_unicode", args, "%t.%i", &in, &byteorder);

  if (byteorder >= 2) {
    if (byteorder == 2) {
#if PIKE_BYTEORDER == 1234
      /* Little endian. */
      byteorder = 1;
#else
      byteorder = 0;
#endif
    } else {
      SIMPLE_ARG_TYPE_ERROR("string_to_unicode", 2, "int(0..2)|void");
    }
  }

  switch(in->size_shift) {
  case 0:
    /* Just 8bit characters */
    len = in->len * 2;
    out = begin_shared_string(len);
    if (len) {
      memset(out->str, 0, len);	/* Clear the upper (and lower) byte */
      for(i = in->len; i--;) {
	out->str[i * 2 + 1 - byteorder] = in->str[i];
      }
    }
    out = end_shared_string(out);
    break;
  case 1:
    /* 16 bit characters */
    /* FIXME: Should we check for 0xfffe & 0xffff here too? */
    len = in->len * 2;
    out = begin_shared_string(len);
    if (byteorder ==
#if (PIKE_BYTEORDER == 4321)
	1	/* Little endian. */
#else
	0	/* Big endian. */
#endif
	) {
      /* Other endianness, may need to do byte-order conversion also. */
      p_wchar1 *str1 = STR1(in);
      for(i = in->len; i--;) {
	unsigned INT32 c = str1[i];
	out->str[i * 2 + 1 - byteorder] = c & 0xff;
	out->str[i * 2 + byteorder] = c >> 8;
      }
    } else {
      /* Native byte order -- We don't need to do much...
       *
       * FIXME: Future optimization: Check if refcount is == 1,
       * and perform sufficient magic to be able to convert in place.
       */
      memcpy(out->str, in->str, len);
    }
    out = end_shared_string(out);
    break;
  case 2:
    /* 32 bit characters -- Is someone writing in Klingon? */
    {
      p_wchar2 *str2 = STR2(in);
      ptrdiff_t j;
      len = in->len * 2;
      /* Check how many extra wide characters there are. */
      for(i = in->len; i--;) {
	if (str2[i] > 0xfffd) {
	  if (str2[i] < 0x10000) {
	    /* 0xfffe: Byte-order detection illegal character.
	     * 0xffff: Illegal character.
	     */
	    Pike_error("Illegal character 0x%04x (index %ld) "
                       "is not a Unicode character.",
                       str2[i], (long)i);
	  }
	  if (str2[i] > 0x10ffff) {
	    Pike_error("Character 0x%08x (index %ld) "
                       "is out of range (0x00000000..0x0010ffff).",
                       str2[i], (long)i);
	  }
	  /* Extra wide characters take two UTF16 characters in space.
	   * ie One UTF16 character extra.
	   */
	  len += 2;
	}
      }
      out = begin_shared_string(len);
      j = len;
      for(i = in->len; i--;) {
	unsigned INT32 c = str2[i];

	j -= 2;

	if (c > 0xffff) {
	  /* Use surrogates */
	  c -= 0x10000;

	  out->str[j + 1 - byteorder] = c & 0xff;
	  out->str[j + byteorder] = 0xdc | ((c >> 8) & 0x03);
	  j -= 2;
	  c >>= 10;
	  c |= 0xd800;
	}
	out->str[j + 1 - byteorder] = c & 0xff;
	out->str[j + byteorder] = c >> 8;
      }
#ifdef PIKE_DEBUG
      if (j) {
	Pike_fatal("string_to_unicode(): Indexing error: len:%ld, j:%ld.\n",
                   (long)len, (long)j);
      }
#endif /* PIKE_DEBUG */
      out = end_shared_string(out);
    }
    break;
  }
  pop_n_elems(args);
  push_string(out);
}

/*! @decl string unicode_to_string(string(0..255) s, int(0..2)|void byteorder)
 *!
 *!   Converts an UTF16 byte-stream into a string.
 *!
 *! @param s
 *!   String to convert to UTF16.
 *!
 *! @param byteorder
 *!   Default input byte-order. One of:
 *!   @int
 *!     @value 0
 *!       Network (aka big-endian) byte-order (default).
 *!     @value 1
 *!       Little-endian byte-order.
 *!     @value 2
 *!       Native byte-order.
 *!   @endint
 *!   Note that this argument is disregarded if @[s] starts with a BOM.
 *!
 *! @seealso
 *!   @[Charset.decoder()], @[string_to_unicode()], @[string_to_utf8()],
 *!   @[utf8_to_string()]
 */
PMOD_EXPORT void f_unicode_to_string(INT32 args)
{
  struct pike_string *in;
  struct pike_string *out = NULL;
  ptrdiff_t len, i, num_surrogates = 0;
  INT_TYPE byteorder = 0;
  int swab=0;
  p_wchar1 surr1, surr2, surrmask, *str0;

  get_all_args("unicode_to_string", args, "%n.%i", &in, &byteorder);

  if (in->len & 1) {
    bad_arg_error("unicode_to_string", args, 1, "string", Pike_sp-args,
		  "String length is odd.\n");
  }

  if (byteorder >= 2) {
    if (byteorder == 2) {
#if PIKE_BYTEORDER == 1234
      /* Little endian. */
      byteorder = 1;
#else
      byteorder = 0;
#endif
    } else {
      SIMPLE_ARG_TYPE_ERROR("unicode_to_string", 2, "int(0..2)|void");
    }
  }

  if (byteorder !=
#if PIKE_BYTEORDER == 1234
      1
#else
      0
#endif
      ) {
    /* Need to swap as the wanted byte-order differs
     * from the native byte-order.
     */
    swab = 1;
  }

  /* Check byteorder of UTF data */
  str0 = (p_wchar1 *)in->str;
  len = in->len;
  if (len && (str0[0] == 0xfeff)) {
    /* Correct byte order mark.  No swap necessary. */
    swab = 0;
    str0 ++;
    len -= 2;
  } else if (len && (str0[0] == 0xfffe)) {
    /* Reversed byte order mark.  Need to swap. */
    swab = 1;
    str0 ++;
    len -= 2;
  } else {
    /* No byte order mark.  Use the user-specified byte-order. */
  }

  /* Indentify surrogates by pre-swapped bitmasks, for efficiency */
  if (swab) {
    surr1 = 0xd8;
    surr2 = 0xdc;
    surrmask = 0xfc;
  } else {
    surr1 = 0xd800;
    surr2 = 0xdc00;
    surrmask = 0xfc00;
  }

  /* Count number of surrogates */
  for (i = len; i >= 4; i -= 2, str0++)
    if ( (str0[0]&surrmask) == surr1 &&
	 (str0[1]&surrmask) == surr2 )
      num_surrogates ++;

  /* Move str0 past the last word */
  str0++;

  len = len / 2 - num_surrogates;

  out = begin_wide_shared_string(len, (num_surrogates? 2 : 1));

  if (!swab) {
    /* Native endian */
    if (num_surrogates) {
      /* Convert surrogates */

      p_wchar2 *str2 = STR2(out);

      for (i = len; i--; --str0)

	if ((str0[-1]&surrmask) == surr2 && num_surrogates &&
	    (str0[-2]&surrmask) == surr1) {

	  str2[i] = ((str0[-2]&0x3ff)<<10) + (str0[-1]&0x3ff) + 0x10000;

	  --str0;
	  --num_surrogates;

	} else

	  str2[i] = str0[-1];

    } else
    /*
     * FIXME: Future optimization: Perform sufficient magic
     * to do the conversion in place if the ref-count is == 1.
     */
      memcpy(out->str, str0-len, len*2);
  } else {
    /* Reverse endian */

    if (num_surrogates) {
      /* Convert surrogates */

      p_wchar2 *str2 = STR2(out);

      for (i = len; i--; --str0) {

	if ((str0[-1]&surrmask) == surr2 && num_surrogates &&
	    (str0[-2]&surrmask) == surr1) {

#if (PIKE_BYTEORDER == 4321)
	  str2[i] = ((((unsigned char *)str0)[-3]&3)<<18) +
	    (((unsigned char *)str0)[-4]<<10) +
	    ((((unsigned char *)str0)[-1]&3)<<8) +
	    ((unsigned char *)str0)[-2] +
	    0x10000;
#else /* PIKE_BYTEORDER != 4321 */
	  str2[i] = ((((unsigned char *)str0)[-4]&3)<<18) +
	    (((unsigned char *)str0)[-3]<<10) +
	    ((((unsigned char *)str0)[-2]&3)<<8) +
	    ((unsigned char *)str0)[-1] +
	    0x10000;
#endif /* PIKE_BYTEORDER == 4321 */
	  --str0;
	  --num_surrogates;

	} else {
#if (PIKE_BYTEORDER == 4321)
	  str2[i] = (((unsigned char *)str0)[-1]<<8) +
	    ((unsigned char *)str0)[-2];
#else /* PIKE_BYTEORDER != 4321 */
	  str2[i] = (((unsigned char *)str0)[-2]<<8) +
	    ((unsigned char *)str0)[-1];
#endif /* PIKE_BYTEORDER == 4321 */
	}
      }
    } else {
      /* No surrogates */

      p_wchar1 *str1 = STR1(out);

      for (i = len; i--; --str0) {
#if (PIKE_BYTEORDER == 4321)
	str1[i] = (((unsigned char *)str0)[-1]<<8) +
	  ((unsigned char *)str0)[-2];
#else /* PIKE_BYTEORDER != 4321 */
	str1[i] = (((unsigned char *)str0)[-2]<<8) +
	  ((unsigned char *)str0)[-1];
#endif /* PIKE_BYTEORDER == 4321 */
      }
    }
  }
  out = end_shared_string(out);
  pop_n_elems(args);
  push_string(out);
}

/*! @decl string(1..) string_filter_non_unicode(string s)
 *!
 *!  Replace the most obviously non-unicode characters from @[s] with
 *!  the unicode replacement character.
 *!
 *! @note
 *!   This will replace characters outside the ranges
 *!   @expr{0x00000000-0x0000d7ff@} and @expr{0x0000e000-0x0010ffff@}
 *!   with 0xffea (the replacement character).
 *!
 *! @seealso
 *!   @[Charset.encoder()], @[string_to_unicode()],
 *!   @[unicode_to_string()], @[utf8_to_string()], @[string_to_utf8()]
 */
static void f_string_filter_non_unicode( INT32 args )
{
  struct pike_string *in;
  INT32 min,max;
  int i;
  static const p_wchar1 replace = 0xfffd;
  static const PCHARP repl_char = {(void*)&replace,1};

  get_all_args("filter_non_unicode", args, "%t", &in);
  check_string_range( in, 1, &min, &max );

  if( !in->len || (min >= 0 && max < 0xd800) )
      return; /* The string is obviously ok. */

  if( (max < 0 || min > 0x10ffff) || (max < 0xe000 && min > 0xd7ff) )
  {
      /* All invalid. Could probably be optimized. */
      debug_make_shared_binary_pcharp( repl_char, 1 );
      push_int( in->len );
      o_multiply();
  }
  else
  {
      /* Note: we could optimize this by not doing any string builder
       * at all unless there is at least one character that needs to
       * be replaced.
       */
      struct string_builder out;
      /* on average shift 1 is more correct than in->size_shift, since
       * there is usually only the one character that is outside the
       * range.
       */
      init_string_builder_alloc( &out, in->len, 1 );
      for( i=0; i<in->len; i++ )
      {
          p_wchar2 c = index_shared_string(in,i);
          if( (c < 0 || c > 0x10ffff) || (c>0xd7ff && c<0xe000) )
              string_builder_append( &out, repl_char, 1 );
          else
              string_builder_putchar( &out, c );
      }
      push_string( finish_string_builder( &out ) );
  }
}

/*! @decl utf8_string string_to_utf8(string s)
 *! @decl utf8_string string_to_utf8(string s, int extended)
 *!
 *!   Convert a string into a UTF-8 compliant byte-stream.
 *!
 *! @param s
 *!   String to encode into UTF-8.
 *!
 *! @param extended
 *!   Bitmask with extension options.
 *!   @int
 *!     @value 1
 *!       Accept and encode the characters outside the valid ranges
 *!       using the same algorithm. Such encoded characters are
 *!       however not UTF-8 compliant.
 *!     @value 2
 *!       Encode characters outside the BMP with UTF-8 encoded UTF-16
 *!       (ie split them into surrogate pairs and encode).
 *!   @endint
 *!
 *! @note
 *!   Throws an error if characters not valid in an UTF-8 stream are
 *!   encountered. Valid characters are in the ranges
 *!   @expr{0x00000000-0x0000d7ff@} and @expr{0x0000e000-0x0010ffff@}.
 *!
 *! @seealso
 *!   @[Charset.encoder()], @[string_to_unicode()],
 *!   @[unicode_to_string()], @[utf8_to_string()]
 */
PMOD_EXPORT void f_string_to_utf8(INT32 args)
{
  ptrdiff_t len;
  struct pike_string *in;
  struct pike_string *out;
  ptrdiff_t i;
  INT_TYPE extended = 0;
  PCHARP src;
  INT32 min, max;
  unsigned char * dst;

  get_all_args("string_to_utf8", args, "%t.%i", &in, &extended);

  len = in->len;

  check_string_range(in, 1, &min, &max);

  if (min >= 0 && max <= 0x7f) {
    /* 7bit string -- already valid utf8. */
    pop_n_elems(args - 1);
    return;
  }

  for(i=0,src=MKPCHARP_STR(in); i < in->len; INC_PCHARP(src,1),i++) {
    unsigned INT32 c = EXTRACT_PCHARP(src);
    if (c & ~0x7f) {
      /* 8bit or more. */
      len++;
      if (c & ~0x7ff) {
	/* 12bit or more. */
	len++;
	if (c & ~0xffff) {
	  /* 17bit or more. */
	  len++;
	  if (!(extended & 1) && c > 0x10ffff)
            bad_arg_error ("string_to_utf8", args, 1,
			   NULL, Pike_sp - args,
			   "Character 0x%08x at index %"PRINTPTRDIFFT"d is "
			   "outside the allowed range.\n",
			   c, i);
	  if ((extended & 2) && (c <= 0x10ffff)) {
	    /* Encode with a surrogate pair. */
	    len += 2;
	  } else if (c & ~0x1fffff) {
	    /* 22bit or more. */
	    len++;
	    if (c & ~0x3ffffff) {
	      /* 27bit or more. */
	      len++;
	      if (c & ~0x7fffffff) {
		/* 32bit or more. */
		len++;
		/* FIXME: Needs fixing when we get 64bit chars... */
	      }
	    }
	  }
	}
	else if (!(extended & 1) && c >= 0xd800 && c <= 0xdfff)
          bad_arg_error ("string_to_utf8", args, 1,
			 NULL, Pike_sp - args,
			 "Character 0x%08x at index %"PRINTPTRDIFFT"d is "
			 "in the surrogate range and therefore invalid.\n",
			 c, i);
      }
    }
  }
  if (len == in->len) {
    /* 7bit string -- already valid utf8. */
    pop_n_elems(args - 1);
    return;
  }
  out = begin_shared_string(len);
  dst = STR0(out);

  for(i=0,src=MKPCHARP_STR(in); i < in->len; INC_PCHARP(src,1),i++) {
    unsigned INT32 c = EXTRACT_PCHARP(src);
    if (!(c & ~0x7f)) {
      /* 7bit */
      *dst++ = c;
    } else if (!(c & ~0x7ff)) {
      /* 11bit */
      *dst++ = 0xc0 | (c >> 6);
      *dst++ = 0x80 | (c & 0x3f);
    } else if (!(c & ~0xffff)) {
      /* 16bit */
      *dst++ = 0xe0 | (c >> 12);
      *dst++ = 0x80 | ((c >> 6) & 0x3f);
      *dst++ = 0x80 | (c & 0x3f);
    } else if ((extended & 2) && (c <= 0x10ffff)) {
      /* Encode with surrogates. */
      c -= 0x10000;
      /* 0xd800 | (c>>10)
       * 0b1101 10cccc cccccc
       * UTF8: 11101101 1010cccc 10cccccc
       */
      *dst++ = 0xed;
      *dst++ = 0xa0 | (c >> 16);
      *dst++ = 0x80 | ((c >> 10) & 0x3f);
      /* 0xdc00 | (c & 0x3ff)
       * 0b1101 11cccc cccccc
       * UTF8: 11101101 1011cccc 10cccccc
       */
      *dst++ = 0xed;
      *dst++ = 0xb0 | ((c >> 6) & 0x3f);
      *dst++ = 0x80 | (c & 0x3f);
    } else if (!(c & ~0x1fffff)) {
      /* 21bit */
      *dst++ = 0xf0 | (c >> 18);
      *dst++ = 0x80 | ((c >> 12) & 0x3f);
      *dst++ = 0x80 | ((c >> 6) & 0x3f);
      *dst++ = 0x80 | (c & 0x3f);
    } else if (!(c & ~0x3ffffff)) {
      /* 26bit */
      *dst++ = 0xf8 | (c >> 24);
      *dst++ = 0x80 | ((c >> 18) & 0x3f);
      *dst++ = 0x80 | ((c >> 12) & 0x3f);
      *dst++ = 0x80 | ((c >> 6) & 0x3f);
      *dst++ = 0x80 | (c & 0x3f);
    } else if (!(c & ~0x7fffffff)) {
      /* 31bit */
      *dst++ = 0xfc | (c >> 30);
      *dst++ = 0x80 | ((c >> 24) & 0x3f);
      *dst++ = 0x80 | ((c >> 18) & 0x3f);
      *dst++ = 0x80 | ((c >> 12) & 0x3f);
      *dst++ = 0x80 | ((c >> 6) & 0x3f);
      *dst++ = 0x80 | (c & 0x3f);
    } else {
      /* 32 - 36bit */
      *dst++ = (char)0xfe;
      *dst++ = 0x80 | ((c >> 30) & 0x3f);
      *dst++ = 0x80 | ((c >> 24) & 0x3f);
      *dst++ = 0x80 | ((c >> 18) & 0x3f);
      *dst++ = 0x80 | ((c >> 12) & 0x3f);
      *dst++ = 0x80 | ((c >> 6) & 0x3f);
      *dst++ = 0x80 | (c & 0x3f);
    }
  }
#ifdef PIKE_DEBUG
  if (len != dst - STR0(out)) {
    Pike_fatal("string_to_utf8(): Calculated and actual lengths differ: "
	       "%"PRINTPTRDIFFT"d != %"PRINTPTRDIFFT"d\n", len, dst - STR0(out));
  }
#endif /* PIKE_DEBUG */
  out = end_shared_string(out);
  pop_n_elems(args);
  push_string(out);
}

/*! @decl string utf8_to_string(utf8_string s)
 *! @decl string utf8_to_string(utf8_string s, int extended)
 *!
 *!   Converts an UTF-8 byte-stream into a string.
 *!
 *! @param s
 *!   String of UTF-8 encoded data to decode.
 *!
 *! @param extended
 *!   Bitmask with extension options.
 *!   @int
 *!     @value 1
 *!       Accept and decode the extension used by @[string_to_utf8()].
 *!     @value 2
 *!       Accept and decode UTF-8 encoded UTF-16 (ie accept and
 *!       decode valid surrogates).
 *!   @endint
 *!
 *! @note
 *!   Throws an error if the stream is not a legal UTF-8 byte-stream.
 *!
 *! @note
 *!   In conformance with @rfc{3629@} and Unicode 3.1 and later,
 *!   non-shortest forms are not decoded. An error is thrown instead.
 *!
 *! @seealso
 *!   @[Charset.encoder()], @[string_to_unicode()], @[string_to_utf8()],
 *!   @[unicode_to_string()], @[validate_utf8()]
 */
PMOD_EXPORT void f_utf8_to_string(INT32 args)
{
  struct pike_string *in;
  struct pike_string *out;
  ptrdiff_t len = 0;
  int shift = 0;
  ptrdiff_t i,j=0;
  INT_TYPE extended = 0;
  INT32 min, max;

  get_all_args("utf8_to_string", args, "%n.%i", &in, &extended);

  check_string_range(in, 1, &min, &max);

  if (min >= 0 && max <= 0x7f) {
    /* 7bit string -- already valid utf8. */
    pop_n_elems(args - 1);
    return;
  }

  for(i=0; i < in->len; i++) {
    unsigned int c = STR0(in)[i];
    len++;
    if (c & 0x80) {
      int cont = 0;

      /* From table 3-6 in the Unicode standard 4.0: Well-Formed UTF-8
       * Byte Sequences
       *
       *  Code Points   1st Byte  2nd Byte  3rd Byte  4th Byte
       * 000000-00007f   00-7f
       * 000080-0007ff   c2-df     80-bf
       * 000800-000fff    e0       a0-bf     80-bf
       * 001000-00cfff   e1-ec     80-bf     80-bf
       * 00d000-00d7ff    ed       80-9f     80-bf
       * 00e000-00ffff   ee-ef     80-bf     80-bf
       * 010000-03ffff    f0       90-bf     80-bf     80-bf
       * 040000-0fffff   f1-f3     80-bf     80-bf     80-bf
       * 100000-10ffff    f4       80-8f     80-bf     80-bf
       */

      if ((c & 0xc0) == 0x80) {
        bad_arg_error ("utf8_to_string", args, 1,
		       NULL, Pike_sp - args,
		       "Invalid continuation character 0x%02x "
		       "at index %"PRINTPTRDIFFT"d.\n",
		       c, i);
      }

#define GET_CHAR(in, i, c) do {						\
	i++;								\
	if (i >= in->len)						\
          bad_arg_error ("utf8_to_string", args, 1,                     \
			 NULL, Pike_sp - args,				\
			 "Truncated UTF-8 sequence at end of string.\n"); \
	c = STR0 (in)[i];						\
      } while(0)
#define GET_CONT_CHAR(in, i, c) do {					\
	GET_CHAR(in, i, c);						\
	if ((c & 0xc0) != 0x80)						\
          bad_arg_error ("utf8_to_string", args, 1,                     \
			 NULL, Pike_sp - args,				\
                         "Expected continuation character at index %td, " \
			 "got 0x%02x.\n",				\
			 i, c);						\
      } while (0)

#define UTF8_SEQ_ERROR(prefix, c, i, problem) do {			\
        bad_arg_error ("utf8_to_string", args, 1,                       \
		       NULL, Pike_sp - args,				\
		       "UTF-8 sequence beginning with %s0x%02x "	\
                       "at index %td %s.\n",				\
		       prefix, c, i, problem);				\
      } while (0)

      if ((c & 0xe0) == 0xc0) {
	/* 11bit */
	if (!(c & 0x1e))
	  UTF8_SEQ_ERROR ("", c, i, "is a non-shortest form");
	cont = 1;
	if (c & 0x1c) {
	  if (shift < 1) {
	    shift = 1;
	  }
	}
      }

      else if ((c & 0xf0) == 0xe0) {
	/* 16bit */
	if (c == 0xe0) {
	  GET_CONT_CHAR (in, i, c);
	  if (!(c & 0x20))
	    UTF8_SEQ_ERROR ("0xe0 ", c, i - 1, "is a non-shortest form");
	  cont = 1;
	}
	else if (!(extended & 1) && c == 0xed) {
	  GET_CONT_CHAR (in, i, c);
	  if (c & 0x20) {
	    /* Surrogate. */
	    if (!(extended & 2)) {
	      UTF8_SEQ_ERROR ("0xed ", c, i - 1, "would decode to "
			      "a UTF-16 surrogate character");
	    }
	    if (c & 0x10) {
	      UTF8_SEQ_ERROR ("0xed ", c, i - 1, "would decode to "
			      "a UTF-16 low surrogate character");
	    }
	    GET_CONT_CHAR(in, i, c);

	    GET_CHAR (in, i, c);
	    if (c != 0xed) {
	      UTF8_SEQ_ERROR ("", c, i-1, "UTF-16 low surrogate "
			      "character required");
	    }
	    GET_CONT_CHAR (in, i, c);
	    if ((c & 0xf0) != 0xb0) {
	      UTF8_SEQ_ERROR ("0xed ", c, i-1, "UTF-16 low surrogate "
			      "character required");
	    }
	    shift = 2;
	  }
	  cont = 1;
	}
	else
	  cont = 2;
	if (shift < 1) {
	  shift = 1;
	}
      }

      else {
	if ((c & 0xf8) == 0xf0) {
	  /* 21bit */
	  if (c == 0xf0) {
	    GET_CONT_CHAR (in, i, c);
	    if (!(c & 0x30))
	      UTF8_SEQ_ERROR ("0xf0 ", c, i - 1, "is a non-shortest form");
	    cont = 2;
	  }
	  else if (!(extended & 1)) {
	    if (c > 0xf4)
	      UTF8_SEQ_ERROR ("", c, i, "would decode to "
			      "a character outside the valid UTF-8 range");
	    else if (c == 0xf4) {
	      GET_CONT_CHAR (in, i, c);
	      if (c > 0x8f)
		UTF8_SEQ_ERROR ("0xf4 ", c, i - 1, "would decode to "
				"a character outside the valid UTF-8 range");
	      cont = 2;
	    }
	    else
	      cont = 3;
	  }
	  else
	    cont = 3;
	}

	else if (c == 0xff)
          bad_arg_error ("utf8_to_string", args, 1,
			 NULL, Pike_sp - args,
			 "Invalid character 0xff at index %"PRINTPTRDIFFT"d.\n",
			 i);

	else if (!(extended & 1))
	  UTF8_SEQ_ERROR ("", c, i, "would decode to "
			  "a character outside the valid UTF-8 range");

	else {
	  if ((c & 0xfc) == 0xf8) {
	    /* 26bit */
	    if (c == 0xf8) {
	      GET_CONT_CHAR (in, i, c);
	      if (!(c & 0x38))
		UTF8_SEQ_ERROR ("0xf8 ", c, i - 1, "is a non-shortest form");
	      cont = 3;
	    }
	    else
	      cont = 4;
	  } else if ((c & 0xfe) == 0xfc) {
	    /* 31bit */
	    if (c == 0xfc) {
	      GET_CONT_CHAR (in, i, c);
	      if (!(c & 0x3c))
		UTF8_SEQ_ERROR ("0xfc ", c, i - 1, "is a non-shortest form");
	      cont = 4;
	    }
	    else
	      cont = 5;
	  } else if (c == 0xfe) {
	    /* 36bit */
	    GET_CONT_CHAR (in, i, c);
	    if (!(c & 0x3e))
	      UTF8_SEQ_ERROR ("0xfe ", c, i - 1, "is a non-shortest form");
	    else if (c & 0x3c)
	      UTF8_SEQ_ERROR ("0xfe ", c, i - 1, "would decode to "
			      "a too large character value");
	    cont = 5;
	  }
	}

	if (shift < 2)
	  shift = 2;
      }

      while(cont--)
	GET_CONT_CHAR (in, i, c);

#undef GET_CHAR
#undef GET_CONT_CHAR
#undef UTF8_SEQ_ERROR
    }
  }
  if (len == in->len) {
    /* 7bit in == 7bit out */
    pop_n_elems(args-1);
    return;
  }

  out = begin_wide_shared_string(len, shift);

  switch (shift) {
    case 0: {
      p_wchar0 *out_str = STR0 (out);
      for(i=0; i < in->len;) {
	unsigned int c = STR0(in)[i++];
	/* NOTE: No tests here since we've already tested the string above. */
	if (c & 0x80) {
	  /* 11bit */
	  unsigned int c2 = STR0(in)[i++] & 0x3f;
	  c &= 0x1f;
	  c = (c << 6) | c2;
	}
	out_str[j++] = c;
      }
      break;
    }

    case 1: {
      p_wchar1 *out_str = STR1 (out);
      for(i=0; i < in->len;) {
	unsigned int c = STR0(in)[i++];
	/* NOTE: No tests here since we've already tested the string above. */
	if (c & 0x80) {
	  if ((c & 0xe0) == 0xc0) {
	    /* 11bit */
	    unsigned int c2 = STR0(in)[i++] & 0x3f;
	    c &= 0x1f;
	    c = (c << 6) | c2;
	  } else {
	    /* 16bit */
	    unsigned int c2 = STR0(in)[i++] & 0x3f;
	    unsigned int c3 = STR0(in)[i++] & 0x3f;
	    c &= 0x0f;
	    c = (c << 12) | (c2 << 6) | c3;
	  }
	}
	out_str[j++] = c;
      }
      break;
    }

    case 2: {
      p_wchar2 *out_str = STR2 (out);
      for(i=0; i < in->len;) {
	unsigned int c = STR0(in)[i++];
	/* NOTE: No tests here since we've already tested the string above. */
	if (c & 0x80) {
	  int cont = 0;
	  if ((c & 0xe0) == 0xc0) {
	    /* 11bit */
	    cont = 1;
	    c &= 0x1f;
	  } else if ((c & 0xf0) == 0xe0) {
	    /* 16bit */
	    cont = 2;
	    c &= 0x0f;
	  } else if ((c & 0xf8) == 0xf0) {
	    /* 21bit */
	    cont = 3;
	    c &= 0x07;
	  } else if ((c & 0xfc) == 0xf8) {
	    /* 26bit */
	    cont = 4;
	    c &= 0x03;
	  } else if ((c & 0xfe) == 0xfc) {
	    /* 31bit */
	    cont = 5;
	    c &= 0x01;
	  } else {
	    /* 36bit */
	    cont = 6;
	    c = 0;
	  }
	  while(cont--) {
	    unsigned int c2 = STR0(in)[i++] & 0x3f;
	    c = (c << 6) | c2;
	  }
	  if ((extended & 2) && (c & 0xfc00) == 0xdc00) {
	    /* Low surrogate */
	    c &= 0x3ff;
	    c |= ((out_str[--j] & 0x3ff)<<10) + 0x10000;
	  }
	}
	out_str[j++] = c;
      }
      break;
    }
  }

#ifdef PIKE_DEBUG
  if (j != len) {
    Pike_fatal("Calculated and actual lengths differ: "
	       "%"PRINTPTRDIFFT"d != %"PRINTPTRDIFFT"d\n",
               len, j);
  }
#endif /* PIKE_DEBUG */
  out = low_end_shared_string(out);
#ifdef PIKE_DEBUG
  check_string (out);
#endif
  pop_n_elems(args);
  push_string(out);
}

/*! @decl int(0..1) validate_utf8(utf8_string s)
 *! @decl int(0..1) validate_utf8(utf8_string s, int extended)
 *!
 *!   Checks whether a string is a valid UTF-8 byte-stream.
 *!
 *! @param s
 *!   String of UTF-8 encoded data to validate.
 *!
 *! @param extended
 *!   Bitmask with extension options.
 *!   @int
 *!     @value 1
 *!       Accept the extension used by @[string_to_utf8()], including
 *!       lone UTF-16 surrogates.
 *!     @value 2
 *!       Accept UTF-8 encoded UTF-16 (ie accept valid surrogate-pairs).
 *!   @endint
 *!
 *! @returns
 *!   Returns @expr{0@} (zero) if the stream is not a legal
 *!   UTF-8 byte-stream, and @expr{1@} if it is.
 *!
 *! @note
 *!   In conformance with @rfc{3629@} and Unicode 3.1 and later,
 *!   non-shortest forms are considered invalid.
 *!
 *! @seealso
 *!   @[Charset.encoder()], @[string_to_unicode()], @[string_to_utf8()],
 *!   @[unicode_to_string()], @[utf8_to_string()]
 */
PMOD_EXPORT void f_validate_utf8(INT32 args)
{
  struct pike_string *in;
  ptrdiff_t i;
  INT_TYPE extended = 0;
  INT32 min, max;
  int ret = 1;
  p_wchar1 expect_low_surrogate = 0;

  get_all_args("validate_utf8", args, "%T.%i", &in, &extended);

  if (in->size_shift) {
    /* Wide string -- not UTF-8. */
    pop_n_elems(args);
    push_int(0);
    return;
  }

  check_string_range(in, 1, &min, &max);

  if (min >= 0 && max <= 0x7f) {
    /* 7bit string -- already valid utf8. */
    pop_n_elems(args);
    push_int(1);
    return;
  }

  for(i=0; ret && (i < in->len); i++) {
    p_wchar0 c = STR0(in)[i];
    /* NB: unsigned INT64 to handle bit 33. */
    unsigned INT64 ch;
    unsigned INT64 full_mask = 0x3f;
    unsigned INT64 hi_mask = 0x3e;

    /* From table 3-6 in the Unicode standard 4.0: Well-Formed UTF-8
     * Byte Sequences
     *
     *  Code Points   1st Byte  2nd Byte  3rd Byte  4th Byte
     * 000000-00007f   00-7f
     * 000080-0007ff   c2-df     80-bf
     * 000800-000fff    e0       a0-bf     80-bf
     * 001000-00cfff   e1-ec     80-bf     80-bf
     * 00d000-00d7ff    ed       80-9f     80-bf
     * 00e000-00ffff   ee-ef     80-bf     80-bf
     * 010000-03ffff    f0       90-bf     80-bf     80-bf
     * 040000-0fffff   f1-f3     80-bf     80-bf     80-bf
     * 100000-10ffff    f4       80-8f     80-bf     80-bf
     */

    if (!(c & 0x80)) {
      if (expect_low_surrogate) ret = 0;	/* Expected low surrogate. */
      continue;
    }

    if (!(c & 0x40)) {
      /* Invalid continuation char. */
      ret = 0;
      break;
    }

    if (c == 0xff) {
      /* Invalid UTF-8 code-point. */
      ret = 0;
      break;
    }

    ch = c;

    while (c & 0x40) {
      /* NB: We rely on the NUL-terminator in pike_strings. */
      p_wchar0 cc = STR0(in)[++i];
      if ((cc & 0xc0) != 0x80) {
	/* Expected continuation char. */
	ret = 0;
	break;
      }
      ch = ch<<6 | (cc & 0x3f);
      full_mask |= full_mask << 5;
      hi_mask <<= 5;
      c <<= 1;
    }

    ch = ch & full_mask;

    if (!(ch & hi_mask) || (ch < 0x80)) {
      /* The 5 most significant bits of ch are all zero.
       * This means that it was a non-minimal form.
       *
       * Note that a special case is needed for the range 0x40..0x7f.
       */
      ret = 0;
      break;
    }

    if ((ch & ~0x7ff) == 0xd800) {
      /* Surrogate */
      if (!(extended & 3)) {
	ret = 0;
	break;
      }
      if (!(extended & 1) && ((ch & 0x400) != expect_low_surrogate)) {
	/* Bad surrogate pair. */
	ret = 0;
	break;
      }
      expect_low_surrogate = (ch & 0x400) ^ 0x400;
    } else if (!(extended & 1) && expect_low_surrogate) {
      ret = 0;
      break;
    } else if (ch >= 0x110000) {
      /* Character out of range. */
      if (!(extended & 1) || (ch >= (((unsigned INT64)1) << 32))) {
	ret = 0;
	break;
      }
    }
  }

  if (!(extended & 1) && expect_low_surrogate) {
    ret = 0;
  }

  pop_n_elems(args);
  push_int(ret);
}

/*! @decl int(0..1) deprecated_typep(type t)
 *!
 *!   Checks if the supplied type has the "deprecated" attribute. This would
 *!   generally only be true in static types of identifiers that have been
 *!   marked as __deprecated__.
 *!
 *! @returns
 *!   @expr{1@} if the type has the "deprecated" attribute,
 *!   @expr{0@} otherwise.
 */
PMOD_EXPORT void f_deprecated_typep(INT32 args)
{
  int ret;

  if (!args || TYPEOF(Pike_sp[-1]) != PIKE_T_TYPE)
    SIMPLE_ARG_TYPE_ERROR("deprecated_typep", 1, "type");

  ret = deprecated_typep(Pike_sp[-1].u.type);
  pop_n_elems(args);
  push_int(ret);
}

/*! @decl type|zero typeof_identifier(program p, string identifier)
 *!
 *!   Allows access to the static type of an identifier ("member") in a
 *!   program.
 *!
 *! @returns
 *!   The static type of @expr{p->identifier@}, or @expr{UNDEFINED@}
 *!   if it can not be determined, be it because @expr{identifier@} does not
 *!   exist in @expr{p@} or for other reasons.
 */
PMOD_EXPORT void f_typeof_identifier(INT32 args)
{
    struct program *prog;
    struct pike_string *identifier;
    int identifier_int;

    get_all_args("typeof_identifier", args, "%p%t", &prog, &identifier);

    identifier_int = find_shared_string_identifier(identifier, prog);

    if (identifier_int != -1)
    {
      struct pike_type *ret;

      copy_pike_type(ret, ID_FROM_INT(prog, identifier_int)->type);
      pop_n_elems(args);
      push_type_value(ret);
    }
    else
    {
      pop_n_elems(args);
      push_undefined();
    }
}

/*! @decl string(0..255) __parse_pike_type(string(0..255) t)
 */
static void f_parse_pike_type( INT32 args )
{
  struct pike_type *t;

  if( !args || TYPEOF(Pike_sp[-1]) != T_STRING ||
      Pike_sp[-1].u.string->size_shift )
    Pike_error( "__parse_pike_type requires a 8bit string as its first argument\n" );
  t = parse_type( (char *)STR0(Pike_sp[-1].u.string) );
  pop_stack();

  push_string(type_to_string(t));
  free_type(t);
}

/*! @module Pike
 */

/*! @decl type|zero soft_cast(type to, type from)
 *!
 *!   Return the resulting type from a soft cast of @[from] to @[to].
 *!
 *! @returns
 *!   Returns @expr{UNDEFINED@} if the cast is invalid.
 *!
 *! @note
 *!   The return value for the invalid case may in the future
 *!   change to @expr{__unknown__@}.
 */
static void f___soft_cast(INT32 args)
{
  struct pike_type *res;
  if (args < 2) Pike_error("Bad number of arguments to __soft_cast().\n");
  if (TYPEOF(Pike_sp[-args]) != PIKE_T_TYPE) {
    Pike_error("Bad argument 1 to __soft_cast() expected type.\n");
  }
  if (TYPEOF(Pike_sp[1-args]) != PIKE_T_TYPE) {
    Pike_error("Bad argument 2 to __soft_cast() expected type.\n");
  }
  if (!(res = soft_cast(Pike_sp[-args].u.type,
			Pike_sp[1-args].u.type, 0))) {
    pop_n_elems(args);
    push_undefined();
  } else {
    pop_n_elems(args);
    push_type_value(res);
  }
}

/*! @decl type|zero low_check_call(type fun_type, type arg_type)
 *! @decl type|zero low_check_call(type fun_type, type arg_type, int flags)
 *! @decl type|zero low_check_call(type fun_type, type arg_type, int flags, @
 *!                                mapping state)
 *! @decl type|zero low_check_call(type fun_type, type arg_type, int flags, @
 *!                                mapping state, mixed val)
 *!
 *!   Check whether a function of type @[fun_type] may be called
 *!   with a first argument of type @[arg_type].
 *!
 *! @param flags
 *!   The following flags are currently defined:
 *!   @int
 *!     @value 1
 *!       Strict types. Fail if not all possible values in @[arg_type]
 *!       are valid as the first argument to @[fun_type].
 *!     @value 2
 *!       Last argument. @[arg_type] is the last argument to @[fun_type].
 *!     @value 3
 *!       Both strict types and last argument as above.
 *!   @endint
 *!
 *! @param state
 *!   State mapping. This mapping may be used by attribute handlers
 *!   to store state between different arguments. Note that attribute
 *!   handlers may alter the contents of the mapping.
 *!
 *! @param val
 *!   Value of the argument if known.
 *!
 *! @returns
 *!   Returns a continuation type on success.
 *!
 *!   Returns @tt{0@} (zero) on failure.
 */
static void f___low_check_call(INT32 args)
{
  struct pike_type *res;
  INT32 flags = CALL_NOT_LAST_ARG;
  struct call_state cs;
  struct svalue *sval = NULL;
  struct mapping *state = NULL;
  if (args < 2) Pike_error("Bad number of arguments to __low_check_call().\n");
  if (TYPEOF(Pike_sp[-args]) != PIKE_T_TYPE) {
    Pike_error("Bad argument 1 to __low_check_call() expected type.\n");
  }
  if (TYPEOF(Pike_sp[1-args]) != PIKE_T_TYPE) {
    Pike_error("Bad argument 2 to __low_check_call() expected type.\n");
  }
  if (args > 2) {
    if (TYPEOF(Pike_sp[2-args]) != PIKE_T_INT) {
      Pike_error("Bad argument 3 to __low_check_call() expected int.\n");
    }
    flags = Pike_sp[2-args].u.integer ^ CALL_NOT_LAST_ARG;
  }
  if (args > 3) {
    switch(TYPEOF(Pike_sp[3-args])) {
    case T_MAPPING:
      state = Pike_sp[3-args].u.mapping;
      break;
    case T_INT:
      if (!Pike_sp[3-args].u.integer) break;
      /* FALLTHRU */
    default:
      Pike_error("Bad argument 4 to __low_check_call() expected mapping.\n");
    }
  }
  if (args > 4) sval = Pike_sp + 4 - args;

  LOW_INIT_CALL_STATE(cs, NULL, 1, state);
  res = low_new_check_call(Pike_sp[-args].u.type,
			   Pike_sp[1-args].u.type,
			   flags, &cs, sval);
  FREE_CALL_STATE(cs);

  pop_n_elems(args);
  push_type_value(res);
}

/*! @decl type|zero get_return_type(type fun_type)
 *! @decl type|zero get_return_type(type fun_type, mapping state)
 *!
 *!   Check what a function of the type @[fun_type] will
 *!   return if called with no further arguments.
 *!
 *! @param state
 *!   State mapping. This mapping may be used by attribute handlers
 *!   to store state between different arguments. Note that attribute
 *!   handlers may alter the contents of the mapping.
 *!
 *! @returns
 *!   Returns the type of the returned value on success
 *!
 *!   Returns @tt{0@} (zero) on failure.
 */
static void f___get_return_type(INT32 args)
{
  struct call_state cs;
  struct pike_type *res;
  struct mapping *state = NULL;
  if (args < 1) {
    Pike_error("Bad number of arguments to __get_return_type().\n");
  }
  if (TYPEOF(Pike_sp[-args]) != PIKE_T_TYPE) {
    Pike_error("Bad argument 1 to __get_return_type() expected type.\n");
  }
  if (args > 1) {
    switch(TYPEOF(Pike_sp[1-args])) {
    case T_MAPPING:
      state = Pike_sp[1-args].u.mapping;
      break;
    case T_INT:
      if (!Pike_sp[1-args].u.integer) break;
      /* FALLTHRU */
    default:
      Pike_error("Bad argument 2 to __get_return_type() expected mapping.\n");
    }
  }

  LOW_INIT_CALL_STATE(cs, NULL, 1, state);
  res = new_get_return_type(Pike_sp[-args].u.type, &cs, 0);
  FREE_CALL_STATE(cs);

  pop_n_elems(args);
  push_type_value(res);
}

/*! @decl type|zero get_first_arg_type(type fun_type)
 *!
 *!   Check if a function of the type @[fun_type] may be called
 *!   with an argument, and return the type of that argument.
 *!
 *! @returns
 *!   Returns the expected type of the first argument to the function.
 *!
 *!   Returns @tt{0@} (zero) if a function of the type @[fun_type]
 *!   may not be called with any argument, or if it is not callable.
 */
void f___get_first_arg_type(INT32 args)
{
  struct pike_type *res;
  if (args != 1) {
    Pike_error("Bad number of arguments to __get_first_arg_type().\n");
  }
  if (TYPEOF(Pike_sp[-1]) != PIKE_T_TYPE) {
    Pike_error("Bad argument 1 to __get_first_arg_type() expected type.\n");
  }
  if (!(res = get_first_arg_type(Pike_sp[-1].u.type, CALL_NOT_LAST_ARG)) &&
      !(res = get_first_arg_type(Pike_sp[-1].u.type, 0))) {
    pop_n_elems(args);
    push_undefined();
  } else {
    pop_n_elems(args);
    push_type_value(res);
  }
}

/*! @decl array(string) get_type_attributes(type t)
 *!
 *!   Get the attribute markers for a type.
 *!
 *! @returns
 *!   Returns an array with the attributes for the type @[t].
 *!
 *! @seealso
 *!   @[get_return_type()], @[get_first_arg_type()]
 */
static void f___get_type_attributes(INT32 args)
{
  struct pike_type *t;
  int count = 0;
  if (args != 1) {
    Pike_error("Bad number of arguments to __get_type_attributes().\n");
  }
  if (TYPEOF(Pike_sp[-1]) != PIKE_T_TYPE) {
    Pike_error("Bad argument 1 to __get_type_attributes() expected type.\n");
  }
  t = Pike_sp[-1].u.type;
  /* Note: We assume that the set of attributes is small
   *       enough that we won't run out of stack. */
  while ((t->type == PIKE_T_ATTRIBUTE) || (t->type == PIKE_T_NAME)) {
    if (t->type == PIKE_T_ATTRIBUTE) {
      ref_push_string((struct pike_string *)t->car);
      count++;
    }
    t = t->cdr;
  }
  f_aggregate(count);
  stack_pop_n_elems_keep_top(args);
}

/*! @endmodule Pike
 */

/*! @decl mapping (string:mixed) all_constants()
 *!
 *!   Returns a mapping containing all global constants, indexed on the name
 *!   of the constant, and with the value of the constant as value.
 *!
 *! @seealso
 *!   @[add_constant()]
 */
PMOD_EXPORT void f_all_constants(INT32 args)
{
  pop_n_elems(args);
  ref_push_mapping(get_builtin_constants());
}

/*! @decl CompilerEnvironment.PikeCompiler|zero get_active_compiler()
 *!
 *!   Returns the most recent of the currently active pike compilers,
 *!   or @[UNDEFINED] if none is active.
 *!
 *! @note
 *!   This function should only be used during a call of @[compile()].
 *!
 *! @seealso
 *!   @[get_active_error_handler()], @[compile()],
 *!   @[master()->get_compilation_handler()], @[CompilationHandler]
 */
PMOD_EXPORT void f_get_active_compiler(INT32 args)
{
  struct compilation *c = NULL;

  /* NB: This is an efun, so we need to keep the stack clean. */
  pop_n_elems(args);

  if (compilation_program) {
    struct pike_frame *compiler_frame = Pike_fp;

    while (compiler_frame &&
	   (compiler_frame->context->prog != compilation_program)) {
      compiler_frame = compiler_frame->next;
    }

    if (compiler_frame && compiler_frame->current_object->prog) {
      ref_push_object(compiler_frame->current_object);
      return;
    }
  }

  push_undefined();
}

/*! @decl array allocate(int size)
 *! @decl array allocate(int size, mixed init)
 *!
 *!   Allocate an array of @[size] elements. If @[init] is specified
 *!   then each element is initialized by copying that value
 *!   recursively.
 *!
 *! @seealso
 *!   @[sizeof()], @[aggregate()], @[arrayp()]
 */
PMOD_EXPORT void f_allocate(INT32 args)
{
  INT_TYPE size;
  struct array *a;
  struct svalue *init = NULL;

  get_all_args("allocate", args, "%+.%*", &size, &init);
  if (size > MAX_INT32)
    SIMPLE_ARG_ERROR ("allocate", 1, "Integer too large to use as array size.");

  a=allocate_array(size);
  if(args>1)
  {
    INT32 e;
    push_array (a);
    if (init) {
      for(e=0;e<size;e++)
	copy_svalues_recursively_no_free(a->item+e, init, 1, 0);
      a->type_field = 1 << TYPEOF(*init);
    }
    else {
      /* It's somewhat quirky that allocate(17) and allocate(17, UNDEFINED)
       * have different behavior, but it's of some use, and it's compatible
       * with previous versions. */
      for(e=0;e<size;e++)
	ITEM (a)[e] = svalue_undefined;
      a->type_field = BIT_INT;
    }
    stack_pop_n_elems_keep_top (args);
  }
  else {
    a->type_field = BIT_INT;
    pop_n_elems(args);
    push_array(a);
  }
}

/*! @decl object this_object(void|int level);
 *!
 *!   Returns the object we are currently evaluating in.
 *!
 *! @param level
 *!   @[level] may be used to access the object of a surrounding
 *!   class: The object at level 0 is the current object, the object
 *!   at level 1 is the one belonging to the class that surrounds
 *!   the class that the object comes from, and so on.
 *!
 *! @note
 *!   As opposed to a qualified @expr{this@} reference such as
 *!   @expr{global::this@}, this function doesn't always access the
 *!   objects belonging to the lexically surrounding classes. If the
 *!   class containing the call has been inherited then the objects
 *!   surrounding the inheriting class are accessed.
 */
void f_this_object(INT32 args)
{
  int level, l;
  struct object *o;

  if (args) {
    if (TYPEOF(Pike_sp[-args]) != T_INT || Pike_sp[-args].u.integer < 0)
      SIMPLE_ARG_TYPE_ERROR ("this_object", 1, "a non-negative integer");
    level = Pike_sp[-args].u.integer;
  }
  else
    level = 0;

  pop_n_elems(args);

  o = Pike_fp->current_object;
  for (l = 0; l < level; l++) {
    struct program *p = o->prog;
    if (!p)
      Pike_error ("Object %d level(s) up is destructed - cannot get the parent.\n", l);
    if (!(p->flags & PROGRAM_USES_PARENT))
      /* FIXME: Ought to write out the object here. */
      Pike_error ("Object %d level(s) up lacks parent reference.\n", l);
    o = PARENT_INFO(o)->parent;
  }
  ref_push_object(o);
}

static node *optimize_this_object(node *n)
{
  int level = 0;

  if (CDR (n)) {
    struct compilation *c = THIS_COMPILATION;
    struct program_state *state = Pike_compiler;

    CHECK_COMPILER();

    if (CDR (n)->token != F_CONSTANT) {
      /* Not a constant expression. Make sure there are parent
       * pointers all the way. */
      int i;
      for (i = 0; i < c->compilation_depth; i++, state = state->previous)
	state->new_program->flags |= PROGRAM_USES_PARENT | PROGRAM_NEEDS_PARENT;
      return NULL;
    }
    else {
      int i;
#ifdef PIKE_DEBUG
      if (TYPEOF(CDR(n)->u.sval) != T_INT || CDR(n)->u.sval.u.integer < 0)
	Pike_fatal ("The type check for this_object() failed.\n");
#endif
      level = CDR (n)->u.sval.u.integer;
      for (i = MINIMUM(level, c->compilation_depth); i;
	   i--, state = state->previous) {
	state->new_program->flags |=
	  PROGRAM_USES_PARENT | PROGRAM_NEEDS_PARENT;
      }
    }
  }

  /* We can only improve the type when accessing the innermost object:
   * Since this_object always follows the object pointers it might not
   * access the lexically surrounding objects. Thus the
   * PROGRAM_USES_PARENT stuff above is a bit of a long shot, but it's
   * better than nothing. */
  if (!level) {
    free_type(n->type);
    type_stack_mark();
    /* We are rather sure that we contain ourselves... */
    /* push_object_type(1, Pike_compiler->new_program->id); */
    /* But it did not work yet, so... */
    push_object_type(0, Pike_compiler->new_program->id);
    n->type = pop_unfinished_type();
    if (n->parent) {
      n->parent->node_info |= OPT_TYPE_NOT_FIXED;
    }
  }

  return NULL;
}

static int generate_this_object(node *n)
{
  int level;
  struct compilation *c = THIS_COMPILATION;
  CHECK_COMPILER();

  if (CDR (n)) {
    if (CDR (n)->token != F_CONSTANT)
      /* Not a constant expression. Make a call to f_this_object. */
      return 0;
    else {
#ifdef PIKE_DEBUG
      if (TYPEOF(CDR(n)->u.sval) != T_INT || CDR(n)->u.sval.u.integer < 0)
	Pike_fatal ("The type check for this_object() failed.\n");
#endif
      level = CDR (n)->u.sval.u.integer;
    }
  }
  else level = 0;

  emit1(F_THIS_OBJECT, level);
  modify_stack_depth(1);
  return 1;
}

/*! @decl mixed|void throw(mixed value)
 *!
 *!   Throw @[value] to a waiting @[catch].
 *!
 *!   If no @[catch] is waiting the global error handling will send the
 *!   value to @[master()->handle_error()].
 *!
 *!   If you throw an array with where the first index contains an error
 *!   message and the second index is a backtrace, (the output from
 *!   @[backtrace()]) then it will be treated exactly like a real error
 *!   by overlying functions.
 *!
 *! @seealso
 *!   @[catch]
 */
PMOD_EXPORT void f_throw(INT32 args)
{
  if(args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("throw", 1);
  assign_svalue(&throw_value,Pike_sp-args);
  pop_n_elems(args);
  throw_severity=0;
  pike_throw();
}

int in_forked_child = 0;

/*! @decl void exit(int returncode, void|string fmt, mixed ... extra)
 *!
 *!   Exit the whole Pike program with the given @[returncode].
 *!
 *!   Using @[exit()] with any other value than @expr{0@} (zero) indicates
 *!   that something went wrong during execution. See your system manuals
 *!   for more information about return codes.
 *!
 *!   The arguments after the @[returncode] will be used for a call to
 *!   @[werror] to output a message on stderr.
 *!
 *! @seealso
 *!   @[_exit()]
 */
PMOD_EXPORT void f_exit(INT32 args)
{
  static int in_exit=0;

  if(args < 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("exit", 1);

  if(TYPEOF(Pike_sp[-args]) != T_INT)
    SIMPLE_ARG_TYPE_ERROR("exit", 1, "int");

  if(in_exit) Pike_error("exit already called!\n");
  in_exit=1;

  if(args>1 && TYPEOF(Pike_sp[1-args]) == T_STRING) {
    struct svalue *s =
      simple_mapping_string_lookup(get_builtin_constants(), "werror");
    if (s) {
      apply_svalue(s, args-1);
      pop_stack();
    } else {
      fprintf(stderr, "No efun::werror() at exit.\n");
      pop_n_elems(args-1);
    }
    args=1;
  }

  if (in_forked_child) {
    /* Don't bother to clean up if we're running in a forked child. */
    f__exit(args);
  }

  assign_svalue(&throw_value, Pike_sp-args);
  throw_severity=THROW_EXIT;
  pike_throw();
}

/*! @decl void _exit(int returncode)
 *!
 *!   This function does the same as @[exit], but doesn't bother to clean
 *!   up the Pike interpreter before exiting. This means that no destructors
 *!   will be called, caches will not be flushed, file locks might not be
 *!   released, and databases might not be closed properly.
 *!
 *!   Use with extreme caution.
 *!
 *! @seealso
 *!   @[exit()]
 */
void f__exit(INT32 args)
{
  int code;

  get_all_args("_exit", args, "%d", &code);

#ifdef PIKE_DEBUG
  {
    /* This will allow -p to work with _exit -Hubbe */
    exit_opcodes();
  }
#endif

  /* FIXME: Shouldn't _exit(2) be called here? */
  exit(code);
}

/*! @decl int time();
 *! @decl int time(int(1..1) one)
 *! @decl float time(int(2..) t)
 *!
 *!   This function returns the number of seconds since 00:00:00 UTC, 1 Jan 1970.
 *!
 *!   The second syntax does not query the system for the current
 *!   time, instead the last time value used by the pike process is returned
 *!   again. It avoids a system call, and thus is slightly faster,
 *!   but can be wildly inaccurate. Pike
 *!   queries the time internally when a thread has waited for
 *!   something, typically in @[sleep] or in a backend (see
 *!   @[Pike.Backend]).
 *!
 *!   The third syntax can be used to measure time more precisely than one
 *!   second. It returns how many seconds have passed since @[t]. The precision
 *!   of this function varies from system to system.
 *!
 *! @seealso
 *!   @[ctime()], @[localtime()], @[mktime()], @[gmtime()],
 *!   @[System.gettimeofday()], @[gethrtime()]
 */
PMOD_EXPORT void f_time(INT32 args)
{
  struct timeval ret;
  if(!args ||
     (TYPEOF(Pike_sp[-args]) == T_INT && Pike_sp[-args].u.integer == 0))
  {
    ACCURATE_GETTIMEOFDAY(&ret);
    pop_n_elems(args);
    push_int(ret.tv_sec);

    return;
  }else{
    if(TYPEOF(Pike_sp[-args]) == T_INT && Pike_sp[-args].u.integer > 1)
    {
      struct timeval tmp;
      ACCURATE_GETTIMEOFDAY(&ret);
      tmp.tv_sec=Pike_sp[-args].u.integer;
      tmp.tv_usec=0;
      my_subtract_timeval(&tmp,&ret);
      pop_n_elems(args);
      push_float( - (FLOAT_TYPE)tmp.tv_sec-((FLOAT_TYPE)tmp.tv_usec)/1000000 );
      return;
    }
  }
  pop_n_elems(args);
  INACCURATE_GETTIMEOFDAY(&ret);
  push_int(ret.tv_sec);
}

/*! @decl string(46..122) crypt(string(1..255) password)
 *! @decl int(0..1) crypt(string(1..255) input_password, @
 *!                       string(46..122) crypted_password)
 *! @decl string(46..122) crypt()
 *!
 *!   This function crypts and verifies a short string (only the first
 *!   8 characters are significant).
 *!
 *!   The first syntax crypts the string @[password] into something that
 *!   is hopefully hard to decrypt.
 *!
 *!   The second syntax is used to verify @[typed_password] against
 *!   @[crypted_password], and returns @expr{1@} if they match, and
 *!   @expr{0@} (zero) otherwise.
 *!
 *!   The third syntax generates a random string and then crypts it,
 *!   creating a string useful as a password.
 *!
 *! @note
 *!   Note that strings containing null characters will only be
 *!   processed up until the null character.
 */
PMOD_EXPORT void f_crypt(INT32 args)
{
  char salt[3];
  char *ret, *pwd = NULL, *saltp = NULL;
  char *alphabet =
    "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
#ifdef HAVE_CRYPT_R
  struct crypt_data crypt_data;
  crypt_data.initialized = 0;
#endif

  get_all_args("crypt", args, ".%c%c", &pwd, &saltp);

  if( !pwd )
  {
    do {
      push_random_string(16);
      push_constant_text("\0");
      f_minus(2);
    } while(Pike_sp[-1].u.string->len<8);
    pwd = Pike_sp[-1].u.string->str;
    args++;
  }

  if(saltp)
  {
    if( Pike_sp[1-args].u.string->len < 2 )
    {
      pop_n_elems(args);
      push_int(0);
      return;
    }
  } else {
    struct svalue *random =
      simple_mapping_string_lookup(get_builtin_constants(), "random");
    if(!random || (TYPEOF(*random) != T_FUNCTION))
      Pike_error("Unable to resolve random function.\n");
    push_int(4096); /* strlen(alphbet)**2 */
    apply_svalue(random, 1);
    if(TYPEOF(Pike_sp[-1])!=T_INT)
      Pike_error("Couldn't generate random number.\n");

    salt[0] = alphabet[ Pike_sp[-1].u.integer & 0x3f ];
    salt[1] = alphabet[ (Pike_sp[-1].u.integer>>6) & 0x3f ];
    salt[2] = 0;
    pop_stack();

    saltp=salt;
    if (args > 1) {
      pop_n_elems(args-1);
      args = 1;
    }
  }

  /* NB: crypt(3C) with modern algorithms may be quite slow,
   *     so release the interpreter lock if possible.
   */
#ifdef HAVE_CRYPT_R
  /* Glibc reentrant version of crypt(3C). */
  THREADS_ALLOW();
  {
    ret = crypt_r(pwd, saltp, &crypt_data);
  }
  THREADS_DISALLOW();
#elif defined(HAVE_CRYPT)
#ifdef SOLARIS
  /* NB: crypt(3C) on Solaris (since at least Solaris 8) is
   *     documented to be thread-safe (due to returning a
   *     thread-local buffer).
   *
   *     On Linux (glibc 2.27) and MacOS X (Darwin 13.4.0) it
   *     is documented NOT to be thread-safe.
   */
  THREADS_ALLOW();
#endif
  ret = (char *)crypt(pwd, saltp);
#ifdef SOLARIS
  THREADS_DISALLOW();
#endif
#elif defined(HAVE__CRYPT)
  ret = (char *)_crypt(pwd, saltp);
#else
#error No crypt function found and fallback failed.
#endif

  if(args < 2)
  {
    if (!ret) {
      switch(errno) {
#ifdef ELIBACC
      case ELIBACC:
	Pike_error("Failed to load a required shared library. "
		   "Unsupported salt.\n");
	break;
#endif
      case ENOMEM:
	Pike_error("Out of memory.\n");
	break;
      case EINVAL:
      default:
	Pike_error("Unsupported salt (%d).\n", errno);
	break;
      }
    }

    pop_n_elems(args);
    push_text(ret);
  }else{
    int i;
    i = ret && !strcmp(ret,saltp);
    pop_n_elems(args);
    push_int(i);
  }
}

/*! @decl int(1bit) destruct(void|object o)
 *!
 *!   Mark an object as destructed.
 *!
 *!   Calls @expr{o->_destruct()@}, and then clears all variables in the
 *!   object. If no argument is given, the current object is destructed.
 *!
 *!   All pointers and function pointers to this object will become zero.
 *!   The destructed object will be freed from memory as soon as possible.
 *!
 *! @returns
 *!   Returns @expr{1@} if @[o] has an @[lfun::_destruct()] that
 *!   returned @expr{1@} and inhibited destruction.
 */
PMOD_EXPORT void f_destruct(INT32 args)
{
  struct object *o;
  int ret;
  if(args)
  {
    if(TYPEOF(Pike_sp[-args]) != T_OBJECT) {
      if ((TYPEOF(Pike_sp[-args]) == T_INT) &&
	  (!Pike_sp[-args].u.integer)) {
	pop_n_elems(args-1);
	return;
      }
      SIMPLE_ARG_TYPE_ERROR("destruct", 1, "object");
    }

    o=Pike_sp[-args].u.object;
  }else{
    if(!Pike_fp) {
      PIKE_ERROR("destruct",
		 "Destruct called without argument from callback function.\n",
		 Pike_sp, args);
    }
    o=Pike_fp->current_object;
  }
  if (o->prog && o->prog->flags & PROGRAM_NO_EXPLICIT_DESTRUCT)
    PIKE_ERROR("destruct", "Object can't be destructed explicitly.\n",
	       Pike_sp, args);
  if (o->inhibit_destruct) {
    /* Destruct the object as soon as the inhibit_destruct
     * counter is back down to zero.
     */
    o->flags |= OBJECT_PENDING_DESTRUCT;
    pop_n_elems(args);
    push_int(1);
    return;
  }
  debug_malloc_touch(o);
  ret = destruct_object (o, DESTRUCT_EXPLICIT);
  pop_n_elems(args);
  push_int(ret);
  destruct_objects_to_destruct();
}

/*! @decl array indices(string|array|mapping|multiset|object x)
 *!
 *!   Return an array of all valid indices for the value @[x].
 *!
 *! @param x
 *!   @mixed
 *!     @type string
 *!     @type array
 *!       For strings and arrays this is simply an array of ascending
 *!       numbers.
 *!
 *!     @type mapping
 *!     @type multiset
 *!       For mappings and multisets, the array might contain any value.
 *!
 *!     @type object
 *!       For objects which define @[lfun::_indices()] that return value
 *!       is used.
 *!
 *!       For other objects an array with the names of all non-protected
 *!       symbols is returned.
 *!  @endmixed
 *!
 *! @seealso
 *!   @[values()], @[types()], @[lfun::_indices()]
 */
PMOD_EXPORT void f_indices(INT32 args)
{
  ptrdiff_t size;
  struct array *a = NULL;

  if(args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("indices", 1);

  switch(TYPEOF(Pike_sp[-args]))
  {
  case T_STRING:
    size=Pike_sp[-args].u.string->len;
    goto qjump;

  case T_ARRAY:
    size=Pike_sp[-args].u.array->size;

  qjump:
    a=allocate_array_no_init(size,0);
    while(--size>=0)
    {
      /* Elements are already integers. */
      ITEM(a)[size].u.integer = (INT_TYPE)size;
    }
    a->type_field = BIT_INT;
    break;

  case T_MAPPING:
    a=mapping_indices(Pike_sp[-args].u.mapping);
    break;

  case T_MULTISET:
    a = multiset_indices (Pike_sp[-args].u.multiset);
    break;

  case T_OBJECT:
    a=object_indices(Pike_sp[-args].u.object, SUBTYPEOF(Pike_sp[-args]));
    break;

  case T_PROGRAM:
    a = program_indices(Pike_sp[-args].u.program);
    break;

  case T_FUNCTION:
    {
      struct program *p = program_from_svalue(Pike_sp-args);
      if (p) {
	a = program_indices(p);
	break;
      }
    }
    /* FALLTHRU */

  default:
    SIMPLE_ARG_TYPE_ERROR("indices", 1,
                          "string|array|mapping|"
                          "multiset|object|program|function");
  }
  pop_n_elems(args);
  push_array(a);
}

/*! @decl array values(string|array|mapping|multiset|object x)
 *!
 *!   Return an array of all possible values from indexing the value
 *!   @[x].
 *!
 *! @param x
 *!   @mixed
 *!     @type string
 *!       For strings an array of int with the ISO10646 codes of the
 *!       characters in the string is returned.
 *!
 *!     @type multiset
 *!       For a multiset an array filled with ones (@expr{1@}) is
 *!       returned.
 *!
 *!     @type array
 *!       For arrays a single-level copy of @[x] is returned.
 *!
 *!     @type mapping
 *!       For mappings the array may contain any value.
 *!
 *!     @type object
 *!       For objects which define @[lfun::_values()] that return value
 *!       is used.
 *!
 *!       For other objects an array with the values of all non-protected
 *!       symbols is returned.
 *!  @endmixed
 *!
 *! @seealso
 *!   @[indices()], @[types()], @[lfun::_values()]
 */
PMOD_EXPORT void f_values(INT32 args)
{
  ptrdiff_t size;
  struct array *a = NULL;
  if(args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("values", 1);

  switch(TYPEOF(Pike_sp[-args]))
  {
  case T_STRING:
    size = Pike_sp[-args].u.string->len;
    a = allocate_array_no_init(size,0);
    while(--size >= 0)
    {
      /* Elements are already integers. */
      ITEM(a)[size].u.integer = index_shared_string(Pike_sp[-args].u.string, size);
    }
    a->type_field = BIT_INT;
    break;

  case T_ARRAY:
    a=copy_array(Pike_sp[-args].u.array);
    break;

  case T_MAPPING:
    a=mapping_values(Pike_sp[-args].u.mapping);
    break;

  case T_MULTISET:
    a = multiset_values (Pike_sp[-args].u.multiset);
    break;

  case T_OBJECT:
    a=object_values(Pike_sp[-args].u.object, SUBTYPEOF(Pike_sp[-args]));
    break;

  case T_PROGRAM:
    a = program_values(Pike_sp[-args].u.program);
    break;

  case T_FUNCTION:
    {
      struct program *p = program_from_svalue(Pike_sp - args);
      if (p) {
	a = program_values(p);
	break;
      }
    }
    /* FALLTHRU */

  default:
    SIMPLE_ARG_TYPE_ERROR("values", 1,
                          "string|array|mapping|multiset|"
                          "object|program|function");
  }
  pop_n_elems(args);
  push_array(a);
}

/*! @decl array(type(mixed)) types(string|array|mapping|multiset|object|program x)
 *!
 *!   Return an array with the types of all valid indices for the value @[x].
 *!
 *! @param x
 *!   @mixed
 *!     @type string
 *!       For strings this is simply an array with @tt{int@}
 *!
 *!     @type array
 *!     @type mapping
 *!     @type multiset
 *!       For arrays, mappings and multisets this is simply
 *!       an array with @tt{mixed@}.
 *!
 *!     @type object
 *!       For objects which define @[lfun::_types()] that return value
 *!       is used.
 *!
 *!       For other objects an array with type types for all non-protected
 *!       symbols is returned.
 *!   @endmixed
 *!
 *! @note
 *!   This function was added in Pike 7.9.
 *!
 *! @seealso
 *!   @[indices()], @[values()], @[lfun::_types()]
 */
PMOD_EXPORT void f_types(INT32 args)
{
  ptrdiff_t size;
  struct array *a = NULL;
  struct pike_type *default_type = mixed_type_string;

  if(args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("types", 1);

  switch(TYPEOF(Pike_sp[-args]))
  {
  case T_STRING:
    default_type = int_type_string;
    size=Pike_sp[-args].u.string->len;
    goto qjump;

  case T_MAPPING:
    size = Pike_sp[-args].u.mapping->data->size;
    goto qjump;

  case T_MULTISET:
    /* FIXME: Ought to be int(1..1). */
    default_type = int_type_string;
    size = Pike_sp[-args].u.multiset->msd->size;
    goto qjump;

  case T_ARRAY:
    size=Pike_sp[-args].u.array->size;

  qjump:
    a=allocate_array_no_init(size,0);
    while(--size>=0)
    {
      /* Elements are already integers. */
      SET_SVAL(ITEM(a)[size], PIKE_T_TYPE, 0, type, default_type);
      add_ref(default_type);
    }
    a->type_field = BIT_TYPE;
    break;

  case T_OBJECT:
    a=object_types(Pike_sp[-args].u.object, SUBTYPEOF(Pike_sp[-args]));
    break;

  case T_PROGRAM:
    a = program_types(Pike_sp[-args].u.program);
    break;

  case T_FUNCTION:
    {
      struct program *p = program_from_svalue(Pike_sp-args);
      if (p) {
	a = program_types(p);
	break;
      }
    }
    /* FALLTHRU */

  default:
    SIMPLE_ARG_TYPE_ERROR("types", 1,
                          "string|array|mapping|"
                          "multiset|object|program|function");
  }
  pop_n_elems(args);
  push_array(a);
}

/*! @decl array(multiset(Pike.Annotation)) annotations(object|program|function x, @
 *!                                                    int(0..1)|void recurse)
 *!
 *!   Return an array with the annotations for all symbols in @[x].
 *!
 *! @param x
 *!   @mixed
 *!     @type object
 *!       For objects which define @[lfun::_annotations()] that return value
 *!       is used.
 *!
 *!       For other objects an array with annotations for all non-protected
 *!       symbols is returned.
 *!     @type program
 *!       Returns an array with annotations for all non-protected
 *!       constant symbols.
 *!   @endmixed
 *!
 *! @param recurse
 *!   Include annotations recursively added via inherits.
 *!
 *! @returns
 *!   The order of the resulting array is the same as that of @[indices()]
 *!   for the same @[x].
 *!
 *! @note
 *!   This function was added in Pike 8.1.
 *!
 *! @seealso
 *!   @[indices()], @[values()], @[types()], @[lfun::_annotations()],
 *!   @[::_annotations()], @[Program.annotations]
 */
PMOD_EXPORT void f_annotations(INT32 args)
{
  struct array *a = NULL;
  struct pike_type *default_type = mixed_type_string;
  struct svalue *arg = NULL;
  ptrdiff_t flags = 0;

  get_all_args("annotations", args, "%*.%i", &arg, &flags);

  if (flags & ~(ptrdiff_t)1) {
    SIMPLE_ARG_TYPE_ERROR("annotations", 2, "int(0..1)|void");
  }

  switch(TYPEOF(*arg))
  {
  case T_OBJECT:
    a = object_annotations(arg->u.object, SUBTYPEOF(Pike_sp[-args]), flags);
    break;

  case T_PROGRAM:
    a = program_annotations(arg->u.program, flags);
    break;

  case T_FUNCTION:
    {
      struct program *p = program_from_svalue(arg);
      if (p) {
	a = program_annotations(p, flags);
	break;
      }
    }
    /* FALLTHRU */

  default:
    SIMPLE_ARG_TYPE_ERROR("annotations", 1, "object|program|function");
  }
  pop_n_elems(args);
  push_array(a);
}


/*! @decl multiset(Pike.Annotation) annotations(program x, @
 *!                                                    int(0..1)|void no_recurse)
 *! @appears Program.annotations
 *!
 *!   Return a multiset with the annotations for all symbols in @[x] attached
 *!   to this program.
 *!
 *! @param x
 *!   Program whose identifiers should be returned.
 *!
 *! @param no_recurse
 *!   Do not include annotations recursively added via inherits.
 *!
 *! @returns
 *!       Returns an multiset with annotations added directly to this program.
 *!   
 *!
 *! @note
 *!   This function was added in Pike 8.1.
 *!
 *! @seealso
 *!   @[indices()], @[values()], @[types()], @[lfun::_annotations()],
 *!   @[::_annotations()]
 */
PMOD_EXPORT void f_direct_program_annotations(INT32 args)
{
  struct array *m = NULL;
  struct pike_type *default_type = mixed_type_string;
  struct svalue *arg = NULL;
  int flag;
  int found = 0;
  
  get_all_args("direct_program_annotations", args, "%*.%i", &arg, &flag);

  switch(TYPEOF(*arg))
  {
  case T_PROGRAM:
    m = program_inherit_annotations(arg->u.program);
    break;

    /* FALLTHRU */

  default:
    SIMPLE_ARG_TYPE_ERROR("direct_program_annotations", 1, "program");
  }  
  
   pop_n_elems(args);
 
   if(m != NULL && m->size > 0)
   {
     struct multiset * set;
	 if(!(flag & 1) && TYPEOF(ITEM(m)[(m->size -1)]) == PIKE_T_MULTISET) {
	  found = 1;
	  set = ITEM(m)[(m->size -1)].u.multiset;
     ref_push_multiset(set);
 	}
    else if((flag & 1) && TYPEOF(ITEM(m)[0]) == PIKE_T_MULTISET) {
  	  found = 1;
  	  set = ITEM(m)[0].u.multiset;
       ref_push_multiset(set);    	
    }

   }
   if(!found){
    push_int(0);
  }
}



/*! @decl program|function object_program(mixed o)
 *!
 *!   Return the program from which @[o] was instantiated. If the
 *!   object was instantiated from a class using parent references
 *!   the generating function will be returned.
 *!
 *!   If @[o] is not an object or has been destructed @expr{0@} (zero)
 *!   will be returned.
 */
PMOD_EXPORT void f_object_program(INT32 args)
{
  if(args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("object_program", 1);

  if(TYPEOF(Pike_sp[-args]) == T_OBJECT)
  {
    struct object *o=Pike_sp[-args].u.object;
    struct program *p = o->prog;

    if(p)
    {
      if (SUBTYPEOF(Pike_sp[-args])) {
	/* FIXME: This probably works for the subtype-less case as well.
	 */
	struct external_variable_context loc;
	loc.o = o;
	p = (loc.inherit = p->inherits + SUBTYPEOF(Pike_sp[-args]))->prog;
	if (p->flags & PROGRAM_USES_PARENT) {
	  loc.parent_identifier = loc.inherit->parent_identifier;
	  find_external_context(&loc, 1);
	  add_ref(loc.o);
	  pop_n_elems(args);
	  push_function(loc.o, loc.parent_identifier);
	  return;
	}
      } else if((p->flags & PROGRAM_USES_PARENT) &&
	 PARENT_INFO(o)->parent &&
	 PARENT_INFO(o)->parent->prog)
      {
	INT32 id=PARENT_INFO(o)->parent_identifier;
	o=PARENT_INFO(o)->parent;
	add_ref(o);
	pop_n_elems(args);
	push_function(o, id);
	return;
      }
      add_ref(p);
      pop_n_elems(args);
      push_program(p);
      return;
    }
  }

  pop_n_elems(args);
  push_int(0);
}

node *fix_object_program_type(node *n)
{
  /* Fix the type for a common case:
   *
   * object_program(object(is|implements foo))
   */
  node *nn;
  struct pike_type *new_type = NULL;

  if (!n->type) {
    copy_pike_type(n->type, program_type_string);
  }
  if (!(nn = CDR(n))) return NULL;
  if ((nn->token == F_ARG_LIST) && (!(nn = CAR(nn)))) return NULL;
  if (!nn->type) return NULL;

  /* Perform the actual conversion. */
  new_type = object_type_to_program_type(nn->type);
  if (new_type) {
    free_type(n->type);
    n->type = new_type;
  }
  return NULL;
}

/*! @decl string reverse(string s, int|void start, int|void end)
 *! @decl array reverse(array a, int|void start, int|void end)
 *! @decl int reverse(int i, int|void start, int|void end)
 *! @decl mixed reverse(object o, mixed... options)
 *!
 *!   Reverses a string, array or int.
 *!
 *!   @param s
 *!     String to reverse.
 *!   @param a
 *!     Array to reverse.
 *!   @param i
 *!     Integer to reverse.
 *!   @param o
 *!     Object to reverse.
 *!   @param start
 *!     Optional start index of the range to reverse.
 *!     Default: @expr{0@} (zero).
 *!   @param end
 *!     Optional end index of the range to reverse.
 *!     Default for strings: @expr{sizeof(s)-1@}.
 *!     Default for arrays: @expr{sizeof(a)-1@}.
 *!     Default for integers: @expr{Pike.get_runtime_info()->int_size - 1@}.
 *!   @param options
 *!     Optional arguments that are to be passed to @[lfun::_reverse()].
 *!
 *!   This function reverses a string, char by char, an array, value
 *!   by value or an int, bit by bit and returns the result. It's not
 *!   destructive on the input value. For objects it simply calls
 *!   @[lfun::_reverse()] in the object, and returns the result.
 *!
 *!   Reversing strings can be particularly useful for parsing difficult
 *!   syntaxes which require scanning backwards.
 *!
 *! @seealso
 *!   @[sscanf()]
 */
PMOD_EXPORT void f_reverse(INT32 args)
{
  struct svalue *sv;
  int start = 0, end = -1;

  get_all_args("reverse", args, "%*.%d%d", &sv, &start, &end);

  switch(TYPEOF(*sv))
  {
  case T_STRING:
  {
    INT32 e;
    struct pike_string *s;
    struct pike_string *orig = sv->u.string;;
    if (start < 0) {
      start = 0;
    } else if (start >= orig->len) {
      /* Noop. */
      pop_n_elems(args-1);
      break;
    }
    if ((end < 0) || (end >= orig->len)) {
      end = orig->len;
    } else if (end <= start) {
      /* Noop. */
      pop_n_elems(args-1);
      break;
    } else {
      end++;
    }
    s=begin_wide_shared_string(orig->len, orig->size_shift);
    if ((orig->len << orig->size_shift) >= 524288) {
      /* More than 512KB. Release the interpreter lock. */
      THREADS_ALLOW();
      switch(orig->size_shift)
      {
      case 0:
	for(e=0;e<start;e++)
	  STR0(s)[e]=STR0(orig)[e];
	for(;e<end;e++)
	  STR0(s)[e]=STR0(orig)[end-1-e+start];
	for(;e<orig->len;e++)
	  STR0(s)[e]=STR0(orig)[e];
	break;

      case 1:
	for(e=0;e<start;e++)
	  STR1(s)[e]=STR1(orig)[e];
	for(;e<end;e++)
	  STR1(s)[e]=STR1(orig)[end-1-e+start];
	for(;e<orig->len;e++)
	  STR1(s)[e]=STR1(orig)[e];
	break;

      case 2:
	for(e=0;e<start;e++)
	  STR2(s)[e]=STR2(orig)[e];
	for(;e<end;e++)
	  STR2(s)[e]=STR2(orig)[end-1-e+start];
	for(;e<orig->len;e++)
	  STR2(s)[e]=STR2(orig)[e];
	break;
      }
      THREADS_DISALLOW();
    } else {
      switch(orig->size_shift)
      {
      case 0:
	for(e=0;e<start;e++)
	  STR0(s)[e]=STR0(orig)[e];
	for(;e<end;e++)
	  STR0(s)[e]=STR0(orig)[end-1-e+start];
	for(;e<orig->len;e++)
	  STR0(s)[e]=STR0(orig)[e];
	break;

      case 1:
	for(e=0;e<start;e++)
	  STR1(s)[e]=STR1(orig)[e];
	for(;e<end;e++)
	  STR1(s)[e]=STR1(orig)[end-1-e+start];
	for(;e<orig->len;e++)
	  STR1(s)[e]=STR1(orig)[e];
	break;

      case 2:
	for(e=0;e<start;e++)
	  STR2(s)[e]=STR2(orig)[e];
	for(;e<end;e++)
	  STR2(s)[e]=STR2(orig)[end-1-e+start];
	for(;e<orig->len;e++)
	  STR2(s)[e]=STR2(orig)[e];
	break;
      }
    }
    s=low_end_shared_string(s);
    pop_n_elems(args);
    push_string(s);
    break;
  }

  case T_INT:
  {
    /* FIXME: Ought to use INT_TYPE! */
    INT32 e;
    e=Pike_sp[-args].u.integer;
    e=((e & 0x55555555UL)<<1) + ((e & 0xaaaaaaaaUL)>>1);
    e=((e & 0x33333333UL)<<2) + ((e & 0xccccccccUL)>>2);
    e=((e & 0x0f0f0f0fUL)<<4) + ((e & 0xf0f0f0f0UL)>>4);
    e=((e & 0x00ff00ffUL)<<8) + ((e & 0xff00ff00UL)>>8);
    e=((e & 0x0000ffffUL)<<16)+ ((e & 0xffff0000UL)>>16);
    Pike_sp[-args].u.integer=e;
    pop_n_elems(args-1);
    break;
  }

  /* FIXME: Bignum support. */

  case T_ARRAY:
  {
    struct array *a = sv->u.array;
    a = reverse_array(a, start, (end < 0)?a->size:end);
    pop_n_elems(args);
    push_array(a);
    break;
  }

  case T_OBJECT:
    {
      apply_lfun(sv->u.object, LFUN__REVERSE, args - 1);
      stack_swap();
      pop_stack();
      break;
    }

  default:
    SIMPLE_ARG_TYPE_ERROR("reverse", 1, "string|int|array");
  }
}

/* Magic, magic and more magic */
/* Returns the index in v for the string that is the longest prefix of
 * str (if any).
 *
 * v is the sorted (according to generic_quick_binary_strcmp()) vector
 * of replacement strings. It also has the prefix forest identified.
 *
 * a is the lower bound.
 * b is the upper bound + 1.
 */
int find_longest_prefix(char *str,
			ptrdiff_t len,
			int size_shift,
			struct replace_many_tupel *v,
			INT32 a,
			INT32 b)
{
  INT32 c, match=-1, match_len=-1;
  ptrdiff_t tmp;

  check_c_stack(2048);

  while(a<b)
  {
    c=(a+b)/2;

    if (v[c].ind->len <= match_len) {
      /* Can't be a suffix of (or is equal to) the current match. */
      b = c;
      continue;
    }

    tmp=generic_find_binary_prefix(v[c].ind->str,
				   v[c].ind->len,
				   v[c].ind->size_shift,
				   str,
				   MINIMUM(len,v[c].ind->len),
				   size_shift);

    if(tmp<0)
    {
      /* Check if we might have a valid prefix that is better than
       * the current match. */
      if (~tmp > match_len) {
	/* We need to look closer to see if we might have a partial prefix. */
	int d = c;
	tmp = -tmp;
	while (((d = v[d].prefix) >= a) && (v[d].ind->len > match_len)) {
	  if (v[d].ind->len < tmp) {
	    /* Found a valid prefix. */
	    match = d;
	    match_len = v[d].ind->len;
	    break;
	  }
	}
      }
      a = c+1;
    }
    else if(tmp>0)
    {
      b=c;
      while ((c = v[b].prefix) > a) {
	if (v[c].ind->len < tmp) {
	  if (v[c].ind->len > match_len) {
	    match = c;
	    match_len = v[c].ind->len;
	  }
	  a = c+1;
	  break;
	}
	b = c;
      }
    }
    else
    {
      if (!v[c].is_prefix) {
	return c;
      }
      a=c+1; /* There might still be a better match... */
      match=c;
      match_len = v[c].ind->len;
    }
  }
  return match;
}


static int replace_sortfun(struct replace_many_tupel *a,
			   struct replace_many_tupel *b)
{
  return (int)my_quick_strcmp(a->ind, b->ind);
}

void free_replace_many_context(struct replace_many_context *ctx)
{
  if (ctx->v) {
    if (ctx->flags) {
      /* Used for the precompiled case. */
      int e = ctx->num;
      while (e--) {
	free_string(ctx->v[e].ind);
	free_string(ctx->v[e].val);
      }
      if (ctx->empty_repl) {
	free_string(ctx->empty_repl);
      }
    }
    free (ctx->v);
    ctx->v = NULL;
  }
}

void compile_replace_many(struct replace_many_context *ctx,
			  struct array *from,
			  struct array *to,
			  int reference_strings)
{
  INT32 e, num;

  ctx->v = NULL;
  ctx->empty_repl = NULL;

#if INT32_MAX >= LONG_MAX
  /* NOTE: The following test is needed, since sizeof(struct tupel)
   *       is somewhat greater than sizeof(struct svalue).
   */
  if (from->size > (ptrdiff_t)(LONG_MAX/sizeof(struct replace_many_tupel)))
    Pike_error("Array too large (size %" PRINTPTRDIFFT "d "
	       "exceeds %" PRINTSIZET "u).\n",
	       from->size,
	       (size_t)(LONG_MAX/sizeof(struct replace_many_tupel)));
#endif
  ctx->v = (struct replace_many_tupel *)
    xalloc(sizeof(struct replace_many_tupel) * from->size);

  for(num=e=0;e<from->size;e++)
  {
    if (!ITEM(from)[e].u.string->len) {
      if (ITEM(to)[e].u.string->len) {
	ctx->empty_repl = ITEM(to)[e].u.string;
      }
      continue;
    }

    ctx->v[num].ind=ITEM(from)[e].u.string;
    ctx->v[num].val=ITEM(to)[e].u.string;
    ctx->v[num].prefix=-2; /* Uninitialized */
    ctx->v[num].is_prefix=0;
    num++;
  }

  ctx->flags = reference_strings;
  if (reference_strings) {
    /* Used for the precompiled compiled case. */
    if (ctx->empty_repl) add_ref(ctx->empty_repl);
    for (e = 0; e < num; e++) {
      add_ref(ctx->v[e].ind);
      add_ref(ctx->v[e].val);
    }
  }

  fsort((char *)ctx->v, num, sizeof(struct replace_many_tupel),
	(fsortfun)replace_sortfun);

  memset(ctx->set_start, 0, sizeof(ctx->set_start));
  memset(ctx->set_end, 0, sizeof(ctx->set_end));
  ctx->other_start = num;

  for(e=0;e<num;e++)
  {
    {
      p_wchar2 x;

      if (ctx->v[num-1-e].ind->len) {
	x=index_shared_string(ctx->v[num-1-e].ind,0);
	if ((size_t) x < NELEM(ctx->set_start))
	  ctx->set_start[x]=num-e-1;
	else
	  ctx->other_start = num-e-1;
      }

      if (ctx->v[e].ind->len) {
	x=index_shared_string(ctx->v[e].ind,0);
	if ((size_t) x < NELEM(ctx->set_end))
	  ctx->set_end[x]=e+1;
      }
    }
    {
      INT32 prefix = e-1;
      if (prefix >= 0) {
	ptrdiff_t tmp =
	  generic_find_binary_prefix(ctx->v[e].ind->str,
				     ctx->v[e].ind->len,
				     ctx->v[e].ind->size_shift,
				     ctx->v[prefix].ind->str,
				     ctx->v[prefix].ind->len,
				     ctx->v[prefix].ind->size_shift);
	if (!tmp) {
	  /* ctx->v[prefix] is a valid prefix to ctx->v[e]. */
        } if (tmp == 1) {
	  /* Optimization. */
	  prefix = -1;
	} else {
#ifdef PIKE_DEBUG
	  if (tmp < 0) Pike_fatal("Sorting with replace_sortfunc failed.\n");
#endif

	  /* Find the first prefix that is shorter than the point at which
	   * the initial strings differed.
	   */
	  while (prefix >= 0) {
	    if (ctx->v[prefix].ind->len < tmp) break;
	    prefix = ctx->v[prefix].prefix;
	  }
	}
	if (prefix >= 0) {
	  ctx->v[prefix].is_prefix = 1;
	}
      }
      ctx->v[e].prefix = prefix;
    }
  }
  ctx->num = num;
}

struct pike_string *execute_replace_many(struct replace_many_context *ctx,
					 struct pike_string *str)
{
  struct string_builder ret;
  ONERROR uwp;

  init_string_builder(&ret, str->size_shift);
  SET_ONERROR(uwp, free_string_builder, &ret);

  /* FIXME: We really ought to build a trie! */

  switch (str->size_shift) {
#define CASE(SZ)					\
    case (SZ):						\
      {							\
	PIKE_CONCAT(p_wchar, SZ) *ss =			\
	  PIKE_CONCAT(STR, SZ)(str);			\
	ptrdiff_t e, s, length = str->len;		\
	for(e = s = 0;length > 0;)			\
	{						\
	  INT32 a, b;					\
	  p_wchar2 ch;					\
							\
	  ch = ss[s];					\
	  if(OPT_IS_CHAR(ch)) {				\
	    b = ctx->set_end[ch];			\
	    if (!b)					\
	      goto PIKE_CONCAT(next_char, SZ);		\
	    a = ctx->set_start[ch];			\
	  } else {					\
	    b = ctx->num;				\
	    a = ctx->other_start;			\
	  }						\
	  if (a >= b)					\
	    goto PIKE_CONCAT(next_char, SZ);		\
							\
	  a = find_longest_prefix((char *)(ss + s),	\
				  length,		\
				  SZ,			\
				  ctx->v, a, b);	\
							\
	  if(a >= 0)					\
	  {						\
	    if (s != e) {				\
	      PIKE_CONCAT(string_builder_binary_strcat,	\
			  SZ)(&ret, ss+e, s-e);		\
	    }						\
	    ch = ctx->v[a].ind->len;			\
	    s += ch;					\
	    length -= ch;				\
	    e = s;					\
	    string_builder_shared_strcat(&ret,		\
					 ctx->v[a].val);	\
	    if (ctx->empty_repl && length) {		\
	      /* Append the replacement for		\
	       * the empty string too. */		\
	      string_builder_shared_strcat(&ret,	\
					   ctx->empty_repl);	\
	    }						\
	    continue;					\
	  }						\
							\
	PIKE_CONCAT(next_char, SZ):			\
	  s++;						\
	  length--;					\
	  if (ctx->empty_repl && length) {		\
	    /* We have a replace with the empty string,	\
	     * and we're not on the last character	\
	     * in the source string.			\
	     */						\
	    string_builder_putchar(&ret, ch);		\
	    string_builder_shared_strcat(&ret,		\
					 ctx->empty_repl);	\
	    e = s;					\
	  }						\
	}						\
	if (e < s) {					\
	  PIKE_CONCAT(string_builder_binary_strcat, SZ)	\
	    (&ret, ss+e, s-e);				\
	}						\
      }							\
    break
#define OPT_IS_CHAR(X)	1
    CASE(0);
#undef OPT_IS_CHAR
#define OPT_IS_CHAR(X)	((size_t) (X) < NELEM(ctx->set_end))
    CASE(1);
    CASE(2);
#undef OPT_IS_CHAR
  }

  UNSET_ONERROR(uwp);
  return finish_string_builder(&ret);
}

static struct pike_string *replace_many(struct pike_string *str,
					struct array *from,
					struct array *to)
{
  struct replace_many_context ctx;
  ONERROR uwp;
  struct pike_string *ret;

  if(from->size != to->size)
    Pike_error("Replace must have equal-sized from and to arrays.\n");

  if(!from->size)
  {
    reference_shared_string(str);
    return str;
  }

  if( (from->type_field & ~BIT_STRING) &&
      (array_fix_type_field(from) & ~BIT_STRING) )
    Pike_error("replace: from array not array(string).\n");

  if( (to->type_field & ~BIT_STRING) &&
      (array_fix_type_field(to) & ~BIT_STRING) )
    Pike_error("replace: to array not array(string).\n");

  if (from->size == 1) {
    /* Just a single string... */
    return string_replace(str, from->item[0].u.string, to->item[0].u.string);
  }

  compile_replace_many(&ctx, from, to, 0);
  SET_ONERROR(uwp, free_replace_many_context, &ctx);

  ret = execute_replace_many(&ctx, str);

  CALL_AND_UNSET_ONERROR(uwp);

  return ret;
}

/*! @decl string replace(string s, string from, string to)
 *! @decl string replace(string s, array(string) from, array(string) to)
 *! @decl string replace(string s, array(string) from, string to)
 *! @decl string replace(string s, mapping(string:string) replacements)
 *! @decl array replace(array a, mixed from, mixed to)
 *! @decl mapping replace(mapping a, mixed from, mixed to)
 *!
 *!   Generic replace function.
 *!
 *!   This function can do several kinds replacement operations, the
 *!   different syntaxes do different things as follows:
 *!
 *!   If all the arguments are strings, a copy of @[s] with every
 *!   occurrence of @[from] replaced with @[to] will be returned.
 *!   Special case: @[to] will be inserted between every character in
 *!   @[s] if @[from] is the empty string.
 *!
 *!   If the first argument is a string, and the others array(string), a string
 *!   with every occurrance of @[from][@i{i@}] in @[s] replaced with
 *!   @[to][@i{i@}] will be returned. Instead of the arrays @[from] and @[to]
 *!   a mapping equivalent to @expr{@[mkmapping](@[from], @[to])@} can be
 *!   used.
 *!
 *!   If the first argument is an array or mapping, the values of @[a] which
 *!   are @[`==()] with @[from] will be replaced with @[to] destructively.
 *!   @[a] will then be returned.
 *!
 *! @note
 *!   Note that @[replace()] on arrays and mappings is a destructive operation.
 */
PMOD_EXPORT void f_replace(INT32 args)
{
  if(args < 3)
  {
     if (args==2 &&
	 TYPEOF(Pike_sp[-1]) == T_MAPPING)
     {
       struct mapping *m = Pike_sp[-1].u.mapping;
       if( (m->data->ind_types & ~BIT_STRING) ||
	   (m->data->val_types & ~BIT_STRING) ) {
	 mapping_fix_type_field(Pike_sp[-1].u.mapping);
	 if( (m->data->ind_types & ~BIT_STRING) ||
	     (m->data->val_types & ~BIT_STRING) ) {
	   SIMPLE_ARG_TYPE_ERROR("replace", 2, "mapping(string:string)");
	 }
       }

	stack_dup();
	f_indices(1);
	stack_swap();
	f_values(1);
	args++;
     }
     else
	SIMPLE_WRONG_NUM_ARGS_ERROR("replace", 3);
  } else if (args > 3) {
    pop_n_elems(args-3);
    args = 3;
  }

  switch(TYPEOF(Pike_sp[-args]))
  {
  case T_ARRAY:
  {
    array_replace(Pike_sp[-args].u.array,Pike_sp+1-args,Pike_sp+2-args);
    pop_n_elems(args-1);
    break;
  }

  case T_MAPPING:
  {
    mapping_replace(Pike_sp[-args].u.mapping,Pike_sp+1-args,Pike_sp+2-args);
    pop_n_elems(args-1);
    break;
  }

  case T_STRING:
  {
    struct pike_string *s;
    switch(TYPEOF(Pike_sp[1-args]))
    {
    default:
      SIMPLE_ARG_TYPE_ERROR("replace", 2, "string|array");

    case T_STRING:
      if(TYPEOF(Pike_sp[2-args]) != T_STRING)
	SIMPLE_ARG_TYPE_ERROR("replace", 3, "string");

      s=string_replace(Pike_sp[-args].u.string,
		       Pike_sp[1-args].u.string,
		       Pike_sp[2-args].u.string);
      break;

    case T_ARRAY:
      if (TYPEOF(Pike_sp[2-args]) == T_STRING) {
	push_int(Pike_sp[1-args].u.array->size);
	stack_swap();
	f_allocate(2);
      } else if(TYPEOF(Pike_sp[2-args]) != T_ARRAY)
	SIMPLE_ARG_TYPE_ERROR("replace", 3, "array|string");

      s=replace_many(Pike_sp[-args].u.string,
		     Pike_sp[1-args].u.array,
		     Pike_sp[2-args].u.array);

    }
    pop_n_elems(args);
    push_string(s);
    break;
  }

  default:
    SIMPLE_ARG_TYPE_ERROR("replace", 1, "array|mapping|string");
  }
}

node *optimize_replace(node *n)
{
  node **arg0 = my_get_arg(&_CDR(n), 0);
  struct pike_type *array_zero;
  struct pike_type *mapping_zero;

  if (!arg0) return NULL;

  MAKE_CONSTANT_TYPE(array_zero, tArr(tZero));
  MAKE_CONSTANT_TYPE(mapping_zero, tMap(tZero, tZero));

  if ((pike_types_le(array_zero, (*arg0)->type, 0, 0) ||
       pike_types_le(mapping_zero, (*arg0)->type, 0, 0))) {
    /* First argument might be an array or a mapping.
     *
     * replace() is destructive on arrays and mappings.
     */
    n->node_info |= OPT_SIDE_EFFECT;
    n->tree_info |= OPT_SIDE_EFFECT;
  } else {
    /* First argument is not an array or mapping,
     *
     * It must thus be a string.
     */
    node **arg1 = my_get_arg(&_CDR(n), 1);
    node **arg2 = my_get_arg(&_CDR(n), 2);

    /* This variable is modified in between setjmp and longjmp,
     * so it needs to be volatile to prevent it from being globbered.
     */
    struct program * volatile replace_compiler = NULL;

    if (arg1 && ((pike_types_le((*arg1)->type, array_type_string, 0, 0) &&
		  arg2 &&
		  (pike_types_le((*arg2)->type, array_type_string, 0, 0) ||
		   pike_types_le((*arg2)->type, string_type_string, 0, 0))) ||
		 (pike_types_le((*arg1)->type, mapping_type_string, 0, 0)))) {
      /* Handle the cases:
       *
       *   replace(string, array, array)
       *   replace(string, array, string)
       *   replace(string, mapping(string:string))
       */
      extern struct program *multi_string_replace_program;
      replace_compiler = multi_string_replace_program;
    } else if (arg1 && pike_types_le((*arg1)->type, string_type_string, 0, 0) &&
	       arg2 && pike_types_le((*arg2)->type, string_type_string, 0, 0)) {
      extern struct program *single_string_replace_program;
      replace_compiler = single_string_replace_program;
    }
    if (replace_compiler && !is_const(*arg0) && is_const(*arg1) &&
	(!arg2 || is_const(*arg2))) {
      /* The second and third (if any) arguments are constants. */
      struct svalue *save_sp = Pike_sp;
      JMP_BUF tmp;
      if (SETJMP(tmp)) {
	struct svalue thrown;
	struct pike_string *s;
	move_svalue (&thrown, &throw_value);
	mark_free_svalue (&throw_value);
	pop_n_elems(Pike_sp - save_sp);
	yywarning("Optimizer failure in replace().");
	s = format_exception_for_error_msg (&thrown);
	if (s) {
          yywarning ("%pS", s);
	  free_string (s);
	}
	free_svalue(&thrown);
      } else {
	INT16 lfun;
	struct object *replace_obj;
	node *ret = NULL;
	INT32 args;
	args = eval_low(*arg1, 1);
	if (args != 1) goto failed;
	if (arg2) {
	  args += eval_low(*arg2, 1);
	  if (!args) {
	    /* eval_low() returned -1. */
	    goto failed;
	  }
	}

	replace_obj = clone_object(replace_compiler, args);

	push_object(replace_obj);
	if (replace_obj->prog &&
	    ((lfun = FIND_LFUN(replace_obj->prog, LFUN_CALL)) != -1)) {
	  SET_SVAL(Pike_sp[-1], PIKE_T_FUNCTION, lfun, object, replace_obj);
	  ADD_NODE_REF2(*arg0,
			ret = mkapplynode(mkconstantsvaluenode(Pike_sp-1),
					  *arg0);
			);

	  UNSETJMP(tmp);
	  pop_n_elems(Pike_sp - save_sp);

	  free_type(array_zero);
	  free_type(mapping_zero);
	  return ret;
	}
      }
    failed:
      UNSETJMP(tmp);
      pop_n_elems(Pike_sp - save_sp);
    }
  }

  free_type(array_zero);
  free_type(mapping_zero);

  return NULL;
}

/*! @decl program compile(string source, CompilationHandler|void handler, @
 *!                       int|void major, int|void minor,@
 *!                       program|void target, object|void placeholder)
 *!
 *!   Compile a string to a program.
 *!
 *!   This function takes a piece of Pike code as a string and
 *!   compiles it into a clonable program.
 *!
 *!   The optional argument @[handler] is used to specify an alternative
 *!   error handler. If it is not specified the current master object will
 *!   be used.
 *!
 *!   The optional arguments @[major] and @[minor] are used to tell the
 *!   compiler to attempt to be compatible with Pike @[major].@[minor].
 *!
 *! @note
 *!   Note that @[source] must contain the complete source for a program.
 *!   It is not possible to compile a single expression or statement.
 *!
 *!   Also note that @[compile()] does not preprocess the program.
 *!   To preprocess the program you can use @[compile_string()] or
 *!   call the preprocessor manually by calling @[cpp()].
 *!
 *! @seealso
 *!   @[compile_string()], @[compile_file()], @[cpp()], @[master()],
 *!   @[CompilationHandler], @[DefaultCompilerEnvironment]
 */
PMOD_EXPORT void f_compile(INT32 args)
{
  apply_low(compilation_environment, CE_COMPILE_FUN_NUM, args);
}


/*! @decl array|mapping|multiset set_weak_flag(array|mapping|multiset m, @
 *!                                            int state)
 *!
 *!   Set the value @[m] to use weak or normal references in its
 *!   indices and/or values (whatever is applicable). @[state] is a
 *!   bitfield built by using @expr{|@} between the following flags:
 *!
 *!   @int
 *!   	@value Pike.WEAK_INDICES
 *!   	  Use weak references for indices. Only applicable for
 *!   	  multisets and mappings.
 *!   	@value Pike.WEAK_VALUES
 *!   	  Use weak references for values. Only applicable for arrays
 *!   	  and mappings.
 *!   	@value Pike.WEAK
 *!   	  Shorthand for @expr{Pike.WEAK_INDICES|Pike.WEAK_VALUES@}.
 *!   @endint
 *!
 *!   If a flag is absent, the corresponding field will use normal
 *!   references. @[state] can also be @expr{1@} as a compatibility
 *!   measure; it's treated like @[Pike.WEAK].
 *!
 *! @returns
 *!   @[m] will be returned.
 */
#define SETFLAG(FLAGS,FLAG,ONOFF) \
  FLAGS = (FLAGS & ~FLAG) | ( ONOFF ? FLAG : 0 )
void f_set_weak_flag(INT32 args)
{
  struct svalue *s;
  INT_TYPE ret;
  int flags;

  get_all_args("set_weak_flag",args,"%*%i",&s,&ret);

  if (ret == 1) ret = PIKE_WEAK_BOTH;

  switch(TYPEOF(*s))
  {
    case T_ARRAY:
      flags = array_get_flags(s->u.array);
      SETFLAG(flags,ARRAY_WEAK_FLAG,ret & PIKE_WEAK_VALUES);
      s->u.array = array_set_flags(s->u.array, flags);
      break;
    case T_MAPPING:
      flags = mapping_get_flags(s->u.mapping);
      flags = (flags & ~PIKE_WEAK_BOTH) | (ret & PIKE_WEAK_BOTH);
      mapping_set_flags(s->u.mapping, flags);
      break;
    case T_MULTISET:
      flags = multiset_get_flags (s->u.multiset);
      flags = (flags & ~PIKE_WEAK_BOTH) | (ret & PIKE_WEAK_BOTH);
      multiset_set_flags (s->u.multiset, flags);
      break;
    default:
      SIMPLE_ARG_TYPE_ERROR("set_weak_flag",1,"array|mapping|multiset");
  }
  pop_n_elems(args-1);
}

/*! @decl int objectp(mixed arg)
 *!
 *!   Returns @expr{1@} if @[arg] is an object, @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!   @[mappingp()], @[programp()], @[arrayp()], @[stringp()], @[functionp()],
 *!   @[multisetp()], @[floatp()], @[intp()]
 */
PMOD_EXPORT void f_objectp(INT32 args)
{
  if(args!=1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("objectp", 1);
  if(TYPEOF(Pike_sp[-args]) != T_OBJECT || !Pike_sp[-args].u.object->prog
     || is_bignum_object(Pike_sp[-args].u.object))
  {
    pop_n_elems(args);
    push_int(0);
  }else{
    pop_n_elems(args);
    push_int(1);
  }
}

/*! @decl int functionp(mixed arg)
 *!
 *!   Returns @expr{1@} if @[arg] is a function, @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!   @[mappingp()], @[programp()], @[arrayp()], @[stringp()], @[objectp()],
 *!   @[multisetp()], @[floatp()], @[intp()]
 */
PMOD_EXPORT void f_functionp(INT32 args)
{
  int res = 0;
  if(args!=1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("functionp", 1);
  if( TYPEOF(Pike_sp[-args]) == T_FUNCTION &&
      (SUBTYPEOF(Pike_sp[-args]) == FUNCTION_BUILTIN ||
       Pike_sp[-args].u.object->prog))
    res=1;
  pop_n_elems(args);
  push_int(res);
}

PMOD_EXPORT int callablep(struct svalue *s)
{
  switch( TYPEOF(*s) )
  {
    case T_FUNCTION:
      if( SUBTYPEOF(*s) == FUNCTION_BUILTIN
	  || s->u.object->prog)
        return 1;
      break;
    case T_PROGRAM:
      return 1;
      break;
    case T_OBJECT:
      {
	struct program *p;
	if((p = s->u.object->prog) &&
	   FIND_LFUN(p->inherits[SUBTYPEOF(*s)].prog,
		     LFUN_CALL ) != -1)
          return 1;
      }
      break;
    case T_ARRAY:
    {
      int ret = 0;
      DECLARE_CYCLIC();

      if (BEGIN_CYCLIC(s, NULL)) {
        END_CYCLIC();
        return 1;
      }

      SET_CYCLIC_RET((ptrdiff_t)1);

      array_fix_type_field(s->u.array);
      if( !s->u.array->type_field) {
        ret = 1;
      }
      else if( !(s->u.array->type_field & ~(BIT_CALLABLE|BIT_INT)) ) {
	struct array *a = s->u.array;
	int i;
	ret = 1;
	for(i=0; i<a->size; i++)
	  if(!IS_UNDEFINED(ITEM(a) + i) && !callablep(&ITEM(a)[i]))
          {
            ret = 0;
            break;
          }
      }
      END_CYCLIC();
      return ret;
      break;
    }
  }

  return 0;
}

/*! @decl int callablep(mixed arg)
 *!
 *!   Returns @expr{1@} if @[arg] is a callable, @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!   @[mappingp()], @[programp()], @[arrayp()], @[stringp()], @[objectp()],
 *!   @[multisetp()], @[floatp()], @[intp()]
 */
PMOD_EXPORT void f_callablep(INT32 args)
{
  int res = 0;
  if(args!=1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("callablep", 1);

  res = callablep(&Pike_sp[-args]);
  pop_n_elems(args);
  push_int(res);
}
#ifndef HAVE_AND_USE_POLL
#undef HAVE_POLL
#endif

static void delaysleep(double delay, unsigned do_abort_on_signal,
 unsigned do_microsleep)
{
#define POLL_SLEEP_LIMIT 0.02

   struct timeval gtod_t0 = {0,0}, gtod_tv = {0,0};
   cpu_time_t t0, tv;

   /* Special case, sleep(0) means 'yield' */
   if(delay == 0.0)
   {
     check_threads_etc();
     /* Since check_threads doesn't yield on every call, we need this
      * to ensure th_yield gets called. */
     pike_thread_yield();
     return;
   }

   if(sizeof(FLOAT_TYPE)<sizeof(double))
     delay += FLT_EPSILON*5;	/* round up */

   t0 = tv = get_real_time();
   if (t0 == -1) {
     /* Paranoia in case get_real_time fails. */
     /* fprintf (stderr, "get_real_time failed in sleep()\n"); */
     ACCURATE_GETTIMEOFDAY (&gtod_t0);
     gtod_tv = gtod_t0;
   }

#define FIX_LEFT()							\
   if (t0 == -1) {							\
     ACCURATE_GETTIMEOFDAY (&gtod_tv);					\
     left = delay - ((gtod_tv.tv_sec-gtod_t0.tv_sec) +			\
		     (gtod_tv.tv_usec-gtod_t0.tv_usec)*1e-6);		\
   }									\
   else {								\
     tv = get_real_time();						\
     left = delay - (tv - t0) * (1.0 / CPU_TIME_TICKS);			\
   }									\
   if (do_microsleep) left-=POLL_SLEEP_LIMIT;

   if (!do_microsleep || delay>POLL_SLEEP_LIMIT)
   {
     for(;;)
     {
       double left;
       /* THREADS_ALLOW may take longer time then POLL_SLEEP_LIMIT */
       THREADS_ALLOW();
       FIX_LEFT();
       if(left>0.0)
	 sysleep(left);
       THREADS_DISALLOW();
       if(do_abort_on_signal) {
	 INVALIDATE_CURRENT_TIME();
	 return;
       }
       FIX_LEFT();
       if(left<=0.0)
	 break;
       check_threads_etc();
     }
     INVALIDATE_CURRENT_TIME();
   }

   if (do_microsleep) {
     if (t0 == -1) {
       while (delay> ((gtod_tv.tv_sec-gtod_t0.tv_sec) +
		      (gtod_tv.tv_usec-gtod_t0.tv_usec)*1e-6))
	 ACCURATE_GETTIMEOFDAY (&gtod_tv);
     }
     else {
       while (delay> (tv - t0) * (1.0 / CPU_TIME_TICKS))
	 tv = get_real_time();
     }
   }

   /* fprintf (stderr, "slept %g\n", (tv - t0) * (1.0 / CPU_TIME_TICKS)); */
}

/*! @decl void sleep(int|float s, void|int abort_on_signal)
 *!
 *!   This function makes the thread stop for @[s] seconds.
 *!
 *!   Only signal handlers can interrupt the sleep, and only when
 *!   @[abort_on_signal] is set. If more than one thread is running
 *!   the signal must be sent to the sleeping thread. Other callbacks
 *!   are not called during sleep.
 *!
 *!   If @[s] is zero then this thread will yield to other threads but
 *!   not sleep otherwise. Note that Pike yields internally at regular
 *!   intervals so it's normally not necessary to do this.
 *!
 *! @seealso
 *!   @[signal()], @[delay()]
 */
PMOD_EXPORT void f_sleep(INT32 args)
{
   double delay=0.0;
   unsigned do_abort_on_signal;

   switch(TYPEOF(Pike_sp[-args]))
   {
      case T_INT:
	 delay=(double)Pike_sp[-args].u.integer;
	 break;

      case T_FLOAT:
	 delay=(double)Pike_sp[-args].u.float_number;
	 break;
   }

   do_abort_on_signal = delay!=0.0 && args > 1
    && !UNSAFE_IS_ZERO(Pike_sp + 1-args);
   pop_n_elems(args);

   delaysleep(delay, do_abort_on_signal, 0);
   low_check_threads_etc();
}

static node *optimize_sleep(node *apply)
{
  if (apply && Pike_compiler->compiler_frame->generator_is_async) {
    /*
     * {
     *   call_out(continue::this_function, X);
     *   continue return;
     * }
     */
    node *ret =
      mknode(F_COMMA_EXPR,
             mkcastnode(void_type_string,
                        mkefuncallnode("call_out",
                                       mknode(F_ARG_LIST,
                                              program_magic_identifier(Pike_compiler, 0,
                                                                       INHERIT_GENERATOR,
                                                                       this_function_string,
                                                                       1),
                                              CDR(apply)))),
             mknode(F_RETURN, NULL, mkintnode(1)));
    ADD_NODE_REF(CDR(apply));
    return ret;
  }

  return NULL;
}

#undef FIX_LEFT
#undef TIME_ELAPSED

/*! @decl void delay(int|float s)
 *!
 *!   This function makes the thread stop for @[s] seconds.
 *!
 *!   Only signal handlers can interrupt the sleep. Other callbacks are
 *!   not called during delay. Beware that this function uses busy-waiting
 *!   to achieve the highest possible accuracy.
 *!
 *! @seealso
 *!   @[signal()], @[sleep()]
 */
PMOD_EXPORT void f_delay(INT32 args)
{
   double delay=0.0;
   unsigned do_abort_on_signal;

   switch(TYPEOF(Pike_sp[-args]))
   {
      case T_INT:
	 delay=(double)Pike_sp[-args].u.integer;
	 break;

      case T_FLOAT:
	 delay=(double)Pike_sp[-args].u.float_number;
	 break;
   }

   do_abort_on_signal = delay!=0.0 && args > 1
    && !UNSAFE_IS_ZERO(Pike_sp + 1-args);
   pop_n_elems(args);

   delaysleep(delay, do_abort_on_signal, !do_abort_on_signal && delay<10);
}

/*! @decl int gc(mapping|array|void quick)
 *!
 *!   Force garbage collection.
 *!
 *! @param quick
 *!   Perform a quick garbage collection on just this value,
 *!   which must have been made weak by @[set_weak_flag()].
 *!   All values that only have a single reference from
 *!   @[quick] will then be freed.
 *!
 *!   When @[quick] hasn't been specified or is @[UNDEFINED],
 *!   this function checks all the memory for cyclic structures such
 *!   as arrays containing themselves and frees them if appropriate.
 *!   It also frees up destructed objects and things with only weak
 *!   references.
 *!
 *!   Normally there is no need to call this function since Pike will
 *!   call it by itself every now and then. (Pike will try to predict
 *!   when 20% of all arrays/object/programs in memory is 'garbage'
 *!   and call this routine then.)
 *!
 *! @returns
 *!   The amount of garbage is returned. This is the number of arrays,
 *!   mappings, multisets, objects and programs that had no nonweak
 *!   external references during the garbage collection. It's normally
 *!   the same as the number of freed things, but there might be some
 *!   difference since _destruct() functions are called during freeing,
 *!   which can cause more things to be freed or allocated.
 *!
 *! @seealso
 *!   @[Pike.gc_parameters], @[Debug.gc_status]
 */
void f_gc(INT32 args)
{
  ptrdiff_t res = 0;
  switch(args? TYPEOF(Pike_sp[-args]): PIKE_T_MIXED) {
  case PIKE_T_MAPPING:
    res = do_gc_weak_mapping(Pike_sp[-args].u.mapping);
    pop_n_elems(args);
    break;
  case PIKE_T_ARRAY:
    res = do_gc_weak_array(Pike_sp[-args].u.array);
    pop_n_elems(args);
    break;
  default:
    pop_n_elems(args);
    res = do_gc(1);
    break;
  }
  push_int(res);
}

#ifdef TYPEP
#undef TYPEP
#endif


#define TYPEP(ID,NAME,TYPE,TYPE_NAME)					\
  PMOD_EXPORT void ID(INT32 args)					\
  {									\
    int t;								\
    struct program *p;							\
    if (args!=1)                                                        \
      SIMPLE_WRONG_NUM_ARGS_ERROR(NAME, 1);				\
    if (TYPEOF(Pike_sp[-args]) == T_OBJECT &&				\
	(p = Pike_sp[-args].u.object->prog))				\
    {									\
      int fun = FIND_LFUN(p->inherits[SUBTYPEOF(Pike_sp[-args])].prog,	\
			  LFUN__IS_TYPE);				\
      if (fun != -1)							\
      {									\
	int id_level =							\
	  p->inherits[SUBTYPEOF(Pike_sp[-args])].identifier_level;	\
	ref_push_string(literal_##TYPE_NAME##_string);			\
	apply_low(Pike_sp[-args-1].u.object, fun + id_level, 1);	\
	stack_unlink(args);						\
	return;								\
      }									\
    }									\
    t = TYPEOF(Pike_sp[-args]) == TYPE;					\
    pop_n_elems(args);							\
    push_int(t);							\
  }

/*! @decl int undefinedp(mixed arg)
 *!
 *! Returns @expr{1@} if @[arg] is undefined, @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!   @[zero_type], @[destructedp], @[intp]
 */
PMOD_EXPORT void f_undefinedp(INT32 args)
{
  if( args!=1 )
    SIMPLE_WRONG_NUM_ARGS_ERROR("undefinedp", 1);
  f_zero_type(args);
  Pike_sp[-1].u.integer = ( Pike_sp[-1].u.integer == NUMBER_UNDEFINED);
}

/*! @decl int destructedp(mixed arg)
 *!
 *! Returns @expr{1@} if @[arg] is a destructed object, @expr{0@}
 *! (zero) otherwise.
 *!
 *! @seealso
 *!   @[zero_type], @[undefinedp], @[intp]
 */
PMOD_EXPORT void f_destructedp(INT32 args)
{
  if( args!=1 )
    SIMPLE_WRONG_NUM_ARGS_ERROR("destructedp", 1);
  f_zero_type(args);
  Pike_sp[-1].u.integer = ( Pike_sp[-1].u.integer == NUMBER_DESTRUCTED);
}

/*! @decl int programp(mixed arg)
 *!
 *!   Returns @expr{1@} if @[arg] is a program, @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!   @[mappingp()], @[intp()], @[arrayp()], @[stringp()], @[objectp()],
 *!   @[multisetp()], @[floatp()], @[functionp()]
 */
PMOD_EXPORT void f_programp(INT32 args)
{
  if(args!=1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("programp", 1);
  switch(TYPEOF(Pike_sp[-args]))
  {
  case T_PROGRAM:
    pop_n_elems(args);
    push_int(1);
    return;

  case T_FUNCTION:
    if(program_from_function(Pike_sp-args))
    {
      pop_n_elems(args);
      push_int(1);
      return;
    }
    /* FALLTHRU */

  default:
    pop_n_elems(args);
    push_int(0);
  }
}

/*! @decl int intp(mixed arg)
 *!
 *!   Returns @expr{1@} if @[arg] is an int, @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!   @[mappingp()], @[programp()], @[arrayp()], @[stringp()], @[objectp()],
 *!   @[multisetp()], @[floatp()], @[functionp()]
 */

/*! @decl int mappingp(mixed arg)
 *!
 *!   Returns @expr{1@} if @[arg] is a mapping, @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!   @[intp()], @[programp()], @[arrayp()], @[stringp()], @[objectp()],
 *!   @[multisetp()], @[floatp()], @[functionp()]
 */

/*! @decl int arrayp(mixed arg)
 *!
 *!   Returns @expr{1@} if @[arg] is an array, @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!   @[intp()], @[programp()], @[mappingp()], @[stringp()], @[objectp()],
 *!   @[multisetp()], @[floatp()], @[functionp()]
 */

/*! @decl int multisetp(mixed arg)
 *!
 *!   Returns @expr{1@} if @[arg] is a multiset, @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!   @[intp()], @[programp()], @[arrayp()], @[stringp()], @[objectp()],
 *!   @[mappingp()], @[floatp()], @[functionp()]
 */

/*! @decl int stringp(mixed arg)
 *!
 *!   Returns @expr{1@} if @[arg] is a string, @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!   @[intp()], @[programp()], @[arrayp()], @[multisetp()], @[objectp()],
 *!   @[mappingp()], @[floatp()], @[functionp()]
 */

/*! @decl int floatp(mixed arg)
 *!
 *!   Returns @expr{1@} if @[arg] is a float, @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!   @[intp()], @[programp()], @[arrayp()], @[multisetp()], @[objectp()],
 *!   @[mappingp()], @[stringp()], @[functionp()]
 */


TYPEP(f_intp, "intp", T_INT, int)
TYPEP(f_mappingp, "mappingp", T_MAPPING, mapping)
TYPEP(f_arrayp, "arrayp", T_ARRAY, array)
TYPEP(f_multisetp, "multisetp", T_MULTISET, multiset)
TYPEP(f_stringp, "stringp", T_STRING, string)
TYPEP(f_floatp, "floatp", T_FLOAT, float)

/*! @decl array sort(array(mixed) index, array(mixed) ... data)
 *!
 *!   Sort arrays destructively.
 *!
 *!   This function sorts the array @[index] destructively. That means
 *!   that the array itself is changed and returned, no copy is created.
 *!
 *!   If extra arguments are given, they are supposed to be arrays of the
 *!   same size as @[index]. Each of these arrays will be modified in the
 *!   same way as @[index]. I.e. if index 3 is moved to position 0 in @[index]
 *!   index 3 will be moved to position 0 in all the other arrays as well.
 *!
 *!   The sort order is as follows:
 *!
 *!   @ul
 *!   @item
 *!     Integers and floats are sorted in ascending order.
 *!   @item
 *!     Strings are sorted primarily on the first characters that are
 *!     different, and secondarily with shorter strings before longer.
 *!     Different characters are sorted in ascending order on the
 *!     character value. Thus the sort order is not locale dependent.
 *!   @item
 *!     Arrays are sorted recursively on the first element. Empty
 *!     arrays are sorted before nonempty ones.
 *!   @item
 *!     Multisets are sorted recursively on the first index. Empty
 *!     multisets are sorted before nonempty ones.
 *!   @item
 *!     Objects are sorted in ascending order according to @[`<()],
 *!     @[`>()] and @[`==()].
 *!   @item
 *!     Other types aren't reordered.
 *!   @item
 *!     Different types are sorted in this order: Arrays, mappings,
 *!     multisets, objects, functions, programs, strings, types,
 *!     integers and floats. Note however that objects can control
 *!     their ordering wrt other types with @[`<], @[`>] and @[`==],
 *!     so this ordering of types only applies to objects without
 *!     those functions.
 *!   @endul
 *!
 *! @returns
 *!   The first argument is returned.
 *!
 *! @note
 *!   The sort is stable, i.e. elements that are compare-wise equal
 *!   aren't reordered.
 *!
 *! @seealso
 *!   @[Array.sort_array], @[reverse()]
 */
PMOD_EXPORT void f_sort(INT32 args)
{
  INT32 e,*order;
  struct array *a;

  if(args < 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("sort", 1);
  if(TYPEOF(Pike_sp[-args]) != T_ARRAY)
    SIMPLE_ARG_TYPE_ERROR("sort", 1, "array");
  a = Pike_sp[-args].u.array;

  for(e=1;e<args;e++)
  {
    if(TYPEOF(Pike_sp[e-args]) != T_ARRAY)
      SIMPLE_ARG_TYPE_ERROR("sort", e+1, "array");

    if(Pike_sp[e-args].u.array->size != a->size)
      bad_arg_error("sort", args, e+1, "array", Pike_sp+e-args,
		    "Argument %d has wrong size.\n", (e+1));
  }

  if(args > 1)
  {
    order = stable_sort_array_destructively(a);
    for(e=1;e<args;e++) order_array(Pike_sp[e-args].u.array,order);
    pop_n_elems(args-1);
    free(order);
  }
  else {
    /* If there are only simple types in the array we can use unstable
     * sorting. */
    array_fix_unfinished_type_field (a);
    if (a->type_field & BIT_COMPLEX)
      free (stable_sort_array_destructively (a));
    else
      sort_array_destructively (a);
  }
}

/*! @decl array rows(mixed data, array index)
 *!
 *!   Select a set of rows from an array.
 *!
 *!   This function is en optimized equivalent to:
 *!
 *! @code
 *! map(@[index], lambda(mixed x) { return @[data][x]; })
 *! @endcode
 *!
 *!   That is, it indices data on every index in the array index and
 *!   returns an array with the results.
 *!
 *! @seealso
 *!   @[column()]
 */
PMOD_EXPORT void f_rows(INT32 args)
{
  INT32 e;
  struct array *a,*tmp;
  struct svalue *val;
  TYPE_FIELD types;

  get_all_args("rows", args, "%*%a", &val, &tmp);

  /* Optimization */
  if(tmp->refs == 1)
  {
    struct svalue sval;
    tmp->type_field = BIT_MIXED | BIT_UNFINISHED;
    types = 0;
    for(e=0;e<tmp->size;e++)
    {
      index_no_free(&sval, val, ITEM(tmp)+e);
      types |= 1 << TYPEOF(sval);
      free_svalue(ITEM(tmp)+e);
      move_svalue (ITEM(tmp) + e, &sval);
    }
    tmp->type_field = types;
    stack_swap();
    pop_stack();
    return;
  }

  push_array(a=allocate_array(tmp->size));
  types = 0;
  for(e=0;e<a->size;e++) {
    index_no_free(ITEM(a)+e, val, ITEM(tmp)+e);
    types |= 1 << TYPEOF(ITEM(a)[e]);
  }
  a->type_field = types;

  Pike_sp--;
  dmalloc_touch_svalue(Pike_sp);
  pop_n_elems(args);
  push_array(a);
}

/*! @decl void verify_internals()
 *! @belongs Debug
 *!
 *!   Perform sanity checks.
 *!
 *!   This function goes through most of the internal Pike structures and
 *!   generates a fatal error if one of them is found to be out of order.
 *!   It is only used for debugging.
 *!
 *! @note
 *!   This function does a more thorough check if the Pike runtime has
 *!   been compiled with RTL debug.
 */
PMOD_EXPORT void f__verify_internals(INT32 args)
{
  INT32 tmp=d_flag;

  /* Keep below calls to low_thorough_check_short_svalue, or else we
   * get O(n!) or so, where n is the number of allocated things. */
  d_flag = 49;

#ifdef PIKE_DEBUG
  do_debug();			/* Calls do_gc() since d_flag > 3. */
#else
  do_gc(1);
#endif
  d_flag=tmp;
  pop_n_elems(args);
}

static void encode_struct_tm(const struct tm *tm, int gmtoffset)
{
  push_static_text("sec");
  push_int(tm->tm_sec);
  push_static_text("min");
  push_int(tm->tm_min);
  push_static_text("hour");
  push_int(tm->tm_hour);

  push_static_text("mday");
  push_int(tm->tm_mday);
  push_static_text("mon");
  push_int(tm->tm_mon);
  push_static_text("year");
  push_int(tm->tm_year);

  push_static_text("wday");
  push_int(tm->tm_wday);
  push_static_text("yday");
  push_int(tm->tm_yday);
  push_static_text("isdst");
  push_int(tm->tm_isdst);

  push_static_text("timezone");
  push_int(gmtoffset);

  f_aggregate_mapping(20);
}

static void encode_tm_tz(const struct tm*tm)
{
  encode_struct_tm(tm,
#ifdef STRUCT_TM_HAS_GMTOFF
   -tm->tm_gmtoff
#elif defined(STRUCT_TM_HAS___TM_GMTOFF)
   -tm->__tm_gmtoff
#elif defined(HAVE_EXTERNAL_TIMEZONE)
   /* Assume dst is one hour. */
   timezone - 3600*tm->tm_isdst
#else
   /* Assume dst is one hour. */
   -3600*tm->tm_isdst
#endif
                                 );
}

/*! @decl mapping(string:int) gmtime(int timestamp)
 *!
 *!   Convert seconds since 00:00:00 UTC, Jan 1, 1970 into components.
 *!
 *!   This function works like @[localtime()] but the result is
 *!   not adjusted for the local time zone.
 *!
 *! @note
 *!   Timestamps prior to 1970-01-01T00:00:00 (ie negative timestamps)
 *!   were not supported on NT and AIX prior to Pike 9.0.
 *!
 *! @seealso
 *!   @[localtime()], @[time()], @[ctime()], @[mktime()],
 *!   @[strptime()]
 */
PMOD_EXPORT void f_gmtime(INT32 args)
{
#if defined (HAVE_GMTIME_R) || defined (HAVE_GMTIME_S)
  struct tm tm_s;
#endif
  struct tm *tm;
  INT64 tt;
  time_t t;
#if defined(__NT__) || defined(_AIX)
  int ydelta = 0;
#endif

  get_all_args("gmtime", args, "%l", &tt);

#if SIZEOF_TIME_T < SIZEOF_INT64
  if (tt > MAX_TIME_T || tt < MIN_TIME_T)
    SIMPLE_ARG_ERROR ("gmtime", 1, "Timestamp outside valid range.");
#endif
#if defined(__NT__) || defined(_AIX)
  if (tt < 0) {
    int wdays = 0;
    while (tt <-11644473600LL) {	/* 1601-01-01T00:00:00 UTC */
      ydelta -= 400;
      tt += 12622780800LL;		/* 400 years in seconds. */
      /* NB: Even number of weeks! */
    }
    while (tt < -3155673600LL) {	/* 1870-01-01T00:00:00 UTC */
      ydelta -= 100;
      tt += 3155673600LL;		/* 100 years in seconds. */
      wdays += 5;			/* (3155673600/3600/24) % 7 */
    }
    if (tt < -2177452800LL) {		/* 1901-01-01T00:00:00 UTC */
      if (tt >= -2208988800LL) {	/* 1900-01-01T00:00:00 UTC */
        ydelta -= 6;
        tt += 189302400;		/* 6 years in seconds. */
      } else {
        tt -= 3600 * 24;		/* Add a fake leap-day for 1900. */
        wdays += 6;
      }
    }
    switch(wdays % 7) {
    case 0: break;
    case 1:
      ydelta += 12;
      tt -= 378691200;			/* 12 years in seconds. */
      break;
    case 2:
      ydelta += 24;
      tt -= 757382400;			/* 24 years in seconds. */
      break;
    case 3:
      ydelta += 8;
      tt -= 252460800;			/* 8 years in seconds. */
      break;
    case 4:
      ydelta += 20;
      tt -= 631152000;			/* 20 years in seconds. */
      break;
    case 5:
      ydelta += 4;
      tt -= 126230400;			/* 4 years in seconds. */
      break;
    case 6:
      ydelta += 16;
      tt -= 504921600;			/* 16 years in seconds. */
      break;
    }
    while (tt < 0) {			/* 1970-01-01T00:00:00 UTC */
      ydelta -= 28;
      tt += 883612800;			/* 28 years in seconds. */
    }
  }
#endif
  t = (time_t) tt;

#ifdef HAVE_GMTIME_R
  tm = gmtime_r (&t, &tm_s);
#elif defined (HAVE_GMTIME_S)
  if (!gmtime_s (&tm_s, &t)) tm = &tm_s; else tm = NULL;
#else
  tm = gmtime(&t);
#endif
  if (!tm) Pike_error ("gmtime() on this system cannot handle "
                       "the timestamp %"PRINTINT64"d.\n", (INT64) t);
#if defined(__NT__) || defined(_AIX)
  tm->tm_year += ydelta;
#endif
  pop_n_elems(args);
  encode_struct_tm(tm, 0);
}

/*! @decl mapping(string:int) localtime(int timestamp)
 *!
 *!   Convert seconds since 00:00:00 UTC, 1 Jan 1970 into components.
 *!
 *! @returns
 *!   This function returns a mapping with the following components:
 *!   @mapping
 *!   	@member int(0..60) "sec"
 *!   	  Seconds over the minute.
 *!   	@member int(0..59) "min"
 *!   	  Minutes over the hour.
 *!   	@member int(0..23) "hour"
 *!   	  Hour of the day.
 *!   	@member int(1..31) "mday"
 *!   	  Day of the month.
 *!   	@member int(0..11) "mon"
 *!   	  Month of the year.
 *!   	@member int(0..) "year"
 *!   	  Year since 1900.
 *!   	@member int(0..6) "wday"
 *!   	  Day of week (0 = Sunday).
 *!   	@member int(0..365) "yday"
 *!   	  Day of the year.
 *!   	@member int(0..1) "isdst"
 *!   	  Is daylight-saving time active.
 *!   	@member int "timezone"
 *!   	  Offset from UTC, including daylight-saving time adjustment.
 *!   @endmapping
 *!
 *! An error is thrown if the localtime(2) call failed on the system.
 *! It's platform dependent what time ranges that function can handle,
 *! e.g. Windows doesn't handle a negative @[timestamp].
 *!
 *! @note
 *!   Prior to Pike 7.5 the field @expr{"timezone"@} was sometimes not
 *!   present, and was sometimes not adjusted for daylight-saving time.
 *!
 *! @note
 *!   Timestamps prior to 1970-01-01T00:00:00 (ie negative timestamps)
 *!   were not supported on NT and AIX prior to Pike 9.0. Note also
 *!   that dst-handling may be incorrect for such timestamps.
 *!
 *! @seealso
 *!   @[Calendar], @[gmtime()], @[time()], @[ctime()], @[mktime()],
 *!   @[strptime()]
 */
PMOD_EXPORT void f_localtime(INT32 args)
{
  struct tm *tm;
  INT64 tt;
  time_t t;
#if defined(__NT__) || defined(_AIX)
  int ydelta = 0;
#endif

  get_all_args("localtime", args, "%l", &tt);

#if SIZEOF_TIME_T < SIZEOF_INT64
  if (tt > MAX_TIME_T || tt < MIN_TIME_T)
    SIMPLE_ARG_ERROR ("localtime", 1, "Timestamp outside valid range.");
#endif
#if defined(__NT__) || defined(_AIX)
  if (tt < 0) {
    /* FIXME: Best effort; the cutoff times should be timezone-dependent. */
    int wdays = 0;
    while (tt <-11644473600LL) {	/* 1601-01-01T00:00:00 UTC */
      ydelta -= 400;
      tt += 12622780800LL;		/* 400 years in seconds. */
      /* NB: Even number of weeks! */
    }
    while (tt < -3155673600LL) {	/* 1870-01-01T00:00:00 UTC */
      ydelta -= 100;
      tt += 3155673600LL;		/* 100 years in seconds. */
      wdays += 5;			/* (3155673600/3600/24) % 7 */
    }
    if (tt < -2177452800LL) {		/* 1901-01-01T00:00:00 UTC */
      if (tt >= -2208988800LL) {	/* 1900-01-01T00:00:00 UTC */
        ydelta -= 6;
        tt += 189302400;		/* 6 years in seconds. */
      } else {
        tt -= 3600 * 24;		/* Add a fake leap-day for 1900. */
        wdays += 6;
      }
    }
    switch(wdays % 7) {
    case 0: break;
    case 1:
      ydelta += 12;
      tt -= 378691200;			/* 12 years in seconds. */
      break;
    case 2:
      ydelta += 24;
      tt -= 757382400;			/* 24 years in seconds. */
      break;
    case 3:
      ydelta += 8;
      tt -= 252460800;			/* 8 years in seconds. */
      break;
    case 4:
      ydelta += 20;
      tt -= 631152000;			/* 20 years in seconds. */
      break;
    case 5:
      ydelta += 4;
      tt -= 126230400;			/* 4 years in seconds. */
      break;
    case 6:
      ydelta += 16;
      tt -= 504921600;			/* 16 years in seconds. */
      break;
    }
    while (tt < 0) {			/* 1970-01-01T00:00:00 UTC */
      ydelta -= 28;
      tt += 883612800;			/* 28 years in seconds. */
    }
  }
#endif
  t = (time_t) tt;

  tm = localtime(&t);
  if (!tm) Pike_error ("localtime() on this system cannot handle "
                       "the timestamp %"PRINTINT64"d.\n", (INT64) t);
#if defined(__NT__) || defined(_AIX)
  tm->tm_year += ydelta;
#endif
  pop_n_elems(args);
  encode_tm_tz(tm);
}

static void set_tz(char *tz)
{
  putenv(tz);
  tzset();
}

time_t mktime_zone(struct tm *date, int other_timezone, int tz)
{
  INT64 retval = 0;
  INT64 ret;
  int normalised_time;
#if defined(__NT__) || defined(_AIX) || defined(__APPLE__)
  INT64 ydelta = 0;	/* Number of years. */
#endif

  if (tz <= -3600*100 || tz >= 3600*100)
    Pike_error("Invalid timezone specified.\n");

  date->tm_wday = -1;		/* flag to determine failure */

#if defined(__NT__) || defined(_AIX) || defined(__APPLE__)
  /* mktime() on NT and AIX do not support years before 1970.
   * mktime() on macOS does not support years before 1900.
   *
   * The calendar repeats every 28 years, so offset it appropriately.
   * Offset years before 1971 in order to avoid issues near 1970-01-01.
   */
  if (date->tm_year <= -300) {
    int y = (100 - date->tm_year)/400;
    date->tm_year += y * 400;
    ydelta += y * 400;
    retval -= y * 12622780800LL;	/* 400 years in seconds. */
  }
#ifndef __APPLE__
  else if (date->tm_year > 130) {
    /* NB: Duplicate of the above due to rounding to zero. */
    int y = (date->tm_year + 300)/400;
    date->tm_year -= y * 400;
    ydelta -= y * 400;
    retval += y * 12622780800LL;	/* 400 years in seconds. */
  }
#endif
  if (date->tm_year <= -30) {		/* Before 1870. */
    int y = -date->tm_year/100;
    date->tm_year += y * 100;
    ydelta += y * 100;
    retval -= y * 3155673600LL;		/* 100 years in seconds. */
  }
  if (date->tm_year < 0) {
    retval += 3600 * 24;		/* Fake a leap day for 1900. */
  } else if (!date->tm_year) {		/* 1900 is not a leap-year. */
    date->tm_year += 6;
    ydelta += 6;
    retval -= 189302400;		/* 6 years in seconds. */
  }
  if (
#ifdef __APPLE__
      date->tm_year < 0
#else
      (date->tm_year < 71) || (date->tm_year > 130)
#endif
      ) {
    int y = ((71 + 27 - date->tm_year)/28);
    date->tm_year += y * 28;
    ydelta += y * 28;
    retval -= 883612800LL * y;		/* 28 years in seconds. */
  }
#elif defined(__HAIKU__)
  /* mktime() on Haiku appears to attempt to support years before 1970
   * albeit it is off by one second. We compensate before calling
   * mktime() in order to be able to handle 1969-12-31T23:59:59 (which
   * is indistinguishable from 1970-01-01T00:00:00 after the call).
   */
  if ((date->tm_year < 70) && !other_timezone) {
    retval = -1;
  }
#endif

  {
    int sec, min, hour;
    sec = date->tm_sec;
    min = date->tm_min;
    hour = date->tm_hour;

    min += sec / 60;
    if ((sec %= 60) < 0)
      min--, sec += 60;
    hour += min / 60;
    if ((min %= 60) < 0)
      hour--, min += 60;
    if ((hour %= 24) < 0)
      hour += 24;
    normalised_time = ((hour * 60) + min) * 60 + sec;
  }

  ret = mktime(date);

  if (date->tm_wday < 0) {
    if (other_timezone) {
      /* NB: This happens for times near {MIN,MAX}_TIME_T. */
      const char *orig_tz = getenv("TZ");
      char tzbuf[20];
      ONERROR uwp;
      char *tzsgn = tz < 0 ? "-" : "+";
      if (tz < 0) tz = -tz;
      sprintf(tzbuf, "TZ=UTC%s%02d:%02d:%02d",
	      tzsgn,
	      tz/3600,
	      (tz/60)%60,
	      tz % 60);
      if (orig_tz) {
        /* NB: orig_tz may point into the buffer that putenv()
         *     writes to, so we need to make a copy here.
         */
        push_text(orig_tz);
      } else {
#ifdef PUTENV_ALWAYS_REQUIRES_EQUAL
        push_text("TZ=");
#else
        push_text("TZ");
#endif
      }
      orig_tz = Pike_sp[-1].u.string->str;
      set_tz(tzbuf);
      SET_ONERROR(uwp, set_tz, orig_tz);
      /* NB: No need to call tzset(); mktime() will call it. */
      ret = mktime_zone(date, 0, 0);
      CALL_AND_UNSET_ONERROR(uwp);
      pop_stack();

#if defined(__NT__) || defined(_AIX) || defined(__APPLE__)
      /* Restore tm_year. */
      date->tm_year -= ydelta;
#endif

      return retval + ret;
    }
    Pike_error("Time conversion unsuccessful.\n");
  }

  if(other_timezone)
  {
    normalised_time -= ((date->tm_hour * 60) + date->tm_min) * 60 + date->tm_sec;
    if (normalised_time < -12*60*60)
      normalised_time += 24*60*60;
    else if (normalised_time > 12*60*60)
      normalised_time -= 24*60*60;

#ifdef STRUCT_TM_HAS___TM_GMTOFF
    retval += date->__tm_gmtoff;
#elif defined(STRUCT_TM_HAS_GMTOFF)
    retval += date->tm_gmtoff;
#elif defined(HAVE_EXTERNAL_TIMEZONE) && defined(HAVE_EXTERNAL_ALTZONE)
    if (date->tm_isdst) {
      retval -= altzone;
    } else {
      retval -= timezone;
    }
#else
    {
      /* NB: The tm from gmtime(3F) will always have tm_isdst == 0,
       *     but mktime() is always in the local time zone, and will
       *     adjust it and tm_hour if the local time zone is in dst.
       *     This causes an error of typically one hour in dst when
       *     used without preadjustment.
       *
       * FIXME: Handle retval outside range of 32-bit time_t.
       */
      time_t t = ret;
      struct tm gmt_tm = *gmtime(&t);
      gmt_tm.tm_isdst = date->tm_isdst;
      normalised_time += ret - mktime(&gmt_tm);
    }
#endif
    ret += normalised_time + tz;
  }

#if defined(__NT__) || defined(_AIX) || defined(__APPLE__)
  /* Restore tm_year. */
  date->tm_year -= ydelta;
#endif

  retval += ret;

  if ((retval < MIN_TIME_T) || (retval > MAX_TIME_T)) {
#ifdef EOVERFLOW
    errno = EOVERFLOW;
#else
    /* NT does not have EOVERFLOW. */
    errno = ERANGE;
#endif
    retval = -1;
  }

  return retval;
}

static int unwind_tm()
{
  push_static_text("sec");
  push_static_text("min");
  push_static_text("hour");
  push_static_text("mday");
  push_static_text("mon");
  push_static_text("year");
  push_static_text("isdst");
  push_static_text("timezone");
  f_aggregate(8);
  f_rows(2);
  Pike_sp--;
  dmalloc_touch_svalue(Pike_sp);
  push_array_items(Pike_sp->u.array);

  return 8;
}

static int get_tm(const char *fname, int args, struct tm *date)
{
  INT_TYPE sec, min, hour, mday, mon, year;
  INT_TYPE isdst = -1, tz = 0;

  get_all_args(fname, args, "%i%i%i%i%i%i.%i%i",
	       &sec, &min, &hour, &mday, &mon, &year, &isdst, &tz);

  memset(date, 0, sizeof(*date));
  date->tm_sec = sec;
  date->tm_min = min;
  date->tm_hour = hour;
  date->tm_mday = mday;
  date->tm_mon = mon;
  date->tm_year = year;
  date->tm_isdst = isdst;
#ifdef NULL_IS_SPECIAL
  date->tm_zone = NULL;
#endif
  return tz;
}

/*! @decl int mktime(mapping(string:int) tm)
 *! @decl int mktime(int sec, int min, int hour, int mday, int mon, int year, @
 *!                  int|void isdst, int|void tz)
 *!
 *!   This function converts information about date and time into an integer
 *!   which contains the number of seconds since 00:00:00 UTC, Jan 1, 1970.
 *!
 *!   You can either call this function with a mapping containing the
 *!   following elements:
 *!   @mapping
 *!   	@member int(0..60) "sec"
 *!   	  Seconds over the minute.
 *!   	@member int(0..59) "min"
 *!   	  Minutes over the hour.
 *!   	@member int(0..23) "hour"
 *!   	  Hour of the day.
 *!   	@member int(1..31) "mday"
 *!   	  Day of the month.
 *!   	@member int(0..11) "mon"
 *!   	  Month of the year.
 *!   	@member int(0..) "year"
 *!   	  Year since 1900.
 *!   	@member int(-1..1) "isdst"
 *!   	  Is daylight-saving time active.  If omitted or set to @expr{-1@},
 *!       it means that the information is not available.
 *!   	@member int "timezone"
 *!   	  The timezone offset from UTC in seconds. If omitted, the time
 *!       will be calculated in the local timezone.
 *!   @endmapping
 *!
 *!   Or you can just send them all on one line as the second syntax suggests.
 *!
 *! @note
 *!   For proper UTC calculations ensure that @expr{isdst = 0@} @b{and@}
 *!   @expr{timezone = 0@}; omitting either one of these parameters
 *!   @b{will@} mess up the UTC calculation.
 *!
 *! @note
 *!   On some operating systems (notably AIX and Win32), dates before
 *!   00:00:00 UTC, Jan 1, 1970 were not supported prior to Pike 9.0.
 *!
 *!   On most 32-bit systems, the supported range of dates is from Dec 13, 1901
 *!   20:45:52 UTC through to Jan 19, 2038 03:14:07 UTC (inclusive).
 *!
 *!   On most 64-bit systems, the supported range of dates is expressed
 *!   in 56 bits and is thus practically
 *!   unlimited (at least up to 1141 milion years in the past
 *!   and into the future).
 *!
 *! @seealso
 *!   @[time()], @[ctime()], @[localtime()], @[gmtime()], @[strftime()]
 */
PMOD_EXPORT void f_mktime (INT32 args)
{
  struct tm date;
  time_t retval;
  int tz;

  if (args<1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("mktime", 1);

  if(args == 1)
  {
    args += unwind_tm() - 1;
  }

  tz = get_tm("mktime", args, &date);

  retval = mktime_zone(&date,
                       args > 7 && SUBTYPEOF(Pike_sp[7-args]) == NUMBER_NUMBER,
	               tz);

  pop_n_elems(args);
#if SIZEOF_TIME_T > SIZEOF_INT_TYPE
  push_int64 (retval);
#else
  push_int(retval);
#endif
}

/*! @decl mapping(string:int) strptime(string(1..255) data, string(1..255) format)
 *!
 *! Parse the given @[data] using the format in @[format] as a date.
 *!
 *! @dl
 *!   @item %%
 *!     The % character.
 *!
 *!   @item %a or %A
 *!     The weekday name according to the C locale, in abbreviated
 *!     form or the full name.
 *!
 *!   @item %b or %B or %h
 *!     The month name according to the C locale, in abbreviated form
 *!     or the full name.
 *!
 *!   @item %c
 *!     The date and time representation for the C locale.
 *!
 *!   @item %C
 *!     The century number (0-99).
 *!
 *!   @item %d or %e
 *!     The day of month (1-31).
 *!
 *!   @item %D
 *!     Equivalent to %m/%d/%y.
 *!
 *!   @item %H
 *!     The hour (0-23).
 *!
 *!   @item %I
 *!     The hour on a 12-hour clock (1-12).
 *!
 *!   @item %j
 *!     The day number in the year (1-366).
 *!
 *!   @item %m
 *!     The month number (1-12).
 *!
 *!   @item %M
 *!     The minute (0-59).
 *!
 *!   @item %n
 *!     Arbitrary whitespace.
 *!
 *!   @item %p
 *!     The C locale's equivalent of AM or PM.
 *!
 *!   @item %R
 *!     Equivalent to %H:%M.
 *!
 *!   @item %S
 *!     The second (0-60; 60 may occur for leap seconds;
 *!     earlier also 61 was allowed).
 *!
 *!   @item %t
 *!     Arbitrary whitespace.
 *!
 *!   @item %T
 *!     Equivalent to %H:%M:%S.
 *!
 *!   @item %U
 *!     The week number with Sunday the first day of the week (0-53).
 *!
 *!   @item %w
 *!     The weekday number (0-6) with Sunday = 0.
 *!
 *!   @item %W
 *!     The week number with Monday the first day of the week (0-53).
 *!
 *!   @item %x
 *!     The date, using the C locale's date format.
 *!
 *!   @item %X
 *!     The time, using the C locale's time format.
 *!
 *!   @item %y
 *!     The year within century (0-99).  When a century is not
 *!     otherwise specified, values in the range 69-99 refer to years
 *!     in the twentieth century (1969-1999); values in the range
 *!     00-68 refer to years in the twenty-first century (2000-2068).
 *!
 *!   @item %Y
 *!     The year, including century (for example, 1991).
 *! @enddl
 *!
 *! @seealso
 *!  @[localtime()], @[gmtime()], @[strftime()]
 */
PMOD_EXPORT void f_strptime (INT32 args)
{
    struct tm tm;
    const char* ret;
    if (Pike_sp[-1].u.string->size_shift || Pike_sp[-2].u.string->size_shift)
      Pike_error("Only 8bit strings are supported\n");
    memset(&tm, 0, sizeof(tm));
    ret = pike_strptime(Pike_sp[-2].u.string->str,
                        Pike_sp[-1].u.string->str,
                        &tm);
    pop_n_elems(args);
    if (ret)
      encode_tm_tz(&tm);
    else
      push_int(0);
}

/*! @decl string(1..255) strftime( string(1..255) format, mapping(string:int) tm)
 *!
 *! Convert the structure to a string.
 *!
 *! @dl
 *!   @item %a
 *!     The abbreviated weekday name according to the current locale
 *!
 *!   @item %A
 *!     The full weekday name according to the current locale.
 *!
 *!   @item %b
 *!     The abbreviated month name according to the current locale.
 *!
 *!   @item %B
 *!     The full month name according to the current locale.
 *!
 *!   @item %c
 *!     The preferred date and time representation for the current locale.
 *!
 *!   @item %C
 *!     The century number (year/100) as a 2-digit integer.
 *!
 *!   @item %d
 *!     The day of the month as a decimal number (range 01 to 31).
 *!
 *!   @item %D
 *!     Equivalent to @expr{%m/%d/%y@}. (for Americans only.
 *!     Americans should note that in other countries @expr{%d/%m/%y@}
 *!     is rather common. This means that in international context
 *!     this format is ambiguous and should not be used.)
 *!
 *!   @item %e
 *!     Like @expr{%d@}, the day of the month as a decimal number,
 *!     but a leading zero is replaced by a space.
 *!
 *!   @item %E
 *!     Modifier: use alternative format, see below.
 *!
 *!   @item %F
 *!     Equivalent to %Y-%m-%d (the ISO 8601 date format). (C99)
 *!
 *!   @item %G
 *!     The ISO 8601 week-based year (see NOTES) with century as a
 *!     decimal number. The 4-digit year corresponding to the ISO
 *!     week number (see @expr{%V@}). This has the same format and
 *!     value as @expr{%Y@}, except that if the ISO week number
 *!     belongs to the previous or next year, that year is used instead.
 *!
 *!   @item %g
 *!     Like @expr{%G@}, but without century, that is,
 *!     with a 2-digit year (00-99). (TZ)
 *!
 *!   @item %h
 *!     Equivalent to %b.
 *!
 *!   @item %H
 *!     The hour as a decimal number using a 24-hour clock (range 00 to 23).
 *!
 *!   @item %I
 *!     The hour as a decimal number using a 12-hour clock (range 01 to 12).
 *!
 *!   @item %j
 *!     The day of the year as a decimal number (range 001 to 366).
 *!
 *!   @item %m
 *!     The month as a decimal number (range 01 to 12).
 *!
 *!   @item %M
 *!     The minute as a decimal number (range 00 to 59).
 *!
 *!   @item %n
 *!     A newline character. (SU)
 *!
 *!   @item %O
 *!     Modifier: use alternative format, see below. (SU)
 *!
 *!   @item %p
 *!     Either @expr{"AM"@} or @expr{"PM"@} according to the given time
 *!     value, or the corresponding strings for the current locale.
 *!     Noon is treated as @expr{"PM"@} and midnight as @expr{"AM"@}.
 *!
 *!   @item %P
 *!     Like @expr{%p@} but in lowercase: @expr{"am"@} or @expr{"pm"@}
 *!     or a corresponding string for the current locale.
 *!
 *!   @item %r
 *!     The time in a.m. or p.m. notation. In the POSIX locale this is
 *!     equivalent to @expr{%I:%M:%S %p@}.
 *!
 *!   @item %R
 *!     The time in 24-hour notation (@expr{%H:%M@}). (SU)
 *!     For a version including the seconds, see @expr{%T@} below.
 *!
 *!   @item %s
 *!     The number of seconds since the Epoch,
 *!     1970-01-01 00:00:00 +0000 (UTC). (TZ)
 *!
 *!   @item %S
 *!     The second as a decimal number (range 00 to 60).
 *!     (The range is up to 60 to allow for occasional leap seconds.)
 *!
 *!   @item %t
 *!     A tab character. (SU)
 *!
 *!   @item %T
 *!     The time in 24-hour notation (@expr{%H:%M:%S@}). (SU)
 *!
 *!   @item %u
 *!     The day of the week as a decimal, range 1 to 7, Monday being 1.
 *!     See also @expr{%w@}. (SU)
 *!
 *!   @item %U
 *!     The week number of the current year as a decimal number,
 *!     range 00 to 53, starting with the first Sunday as the first
 *!     day of week 01. See also @expr{%V@} and @expr{%W@}.
 *!
 *!   @item %V
 *!     The ISO 8601 week number of the current year as a decimal number,
 *!     range 01 to 53, where week 1 is the first week that has at least
 *!     4 days in the new year. See also @expr{%U@} and @expr{%W@}.
 *!
 *!   @item %w
 *!     The day of the week as a decimal, range 0 to 6, Sunday being 0.
 *!     See also @expr{%u@}.
 *! @enddl
 *!
 *! @seealso
 *!  @[ctime()], @[mktime()], @[strptime()], @[Gettext.setlocale]
 */
PMOD_EXPORT void f_strftime (INT32 args)
{
    struct string_builder s;
    struct tm date;

    check_all_args("strftime", args, BIT_STRING, BIT_MAPPING, 0);

    get_tm("strftime", args = unwind_tm(), &date);
    pop_n_elems(args);
    if ((TYPEOF(Pike_sp[-1]) != PIKE_T_STRING) ||
        Pike_sp[-1].u.string->size_shift) {
      Pike_error("Only 8bit strings are supported.\n");
    }

    init_string_builder(&s, 0);

    string_builder_strftime(&s, Pike_sp[-1].u.string->str, &date);

    pop_stack();
    push_string(finish_string_builder(&s));
}

#define DOES_MATCH_CLASS(EXTRACT_M,EXTRACT_S,ML)                      \
  {                                                                   \
      unsigned against = EXTRACT_S;                                   \
      int inverted=0,matched=0;                                       \
      j++;                                                            \
      i++;                                                            \
      if(EXTRACT_M == '!' || EXTRACT_M == '^')                        \
      {                                                               \
        i++;                                                          \
        inverted=1;                                                   \
        matched =1;                                                   \
      }                                                               \
      while(i<ML)                                                     \
      {                                                               \
        unsigned c = EXTRACT_M;                                       \
        unsigned e = 0;                                               \
        if(c==']')                                                    \
          break;                                                      \
        i++;                                                          \
        if( EXTRACT_M == '-' )                                        \
        {                                                             \
          i++;                                                        \
          e = EXTRACT_M;                                              \
          i++;                                                        \
        }                                                             \
        else                                                          \
          e=c;                                                        \
                                                                      \
        if( c <= against && e >= against )                            \
        {                                                             \
          if(inverted)                                                \
            return 0;                                                 \
          matched=1;                                                  \
        }                                                             \
      }                                                               \
      if (!matched || i>=ML) return 0;                                \
      break;                                                          \
  }


/* Common case: both strings are 8bit. */
static int does_match_8_8( const unsigned char *s, int j, int sl,
                          const unsigned char *m, int i, int ml)
{
  for (; i<ml; i++)
  {
    switch (m[i])
    {
     case '?':
       if(j++>=sl) return 0;
       break;

     case '*':
      while(m[i] == '*' && i<ml )
        i++;
      while( m[i] == '?' && i<ml && j<sl)
      {
        i++;
        j++;
      }
      if (i==ml) return 1;

      for (;j<sl;j++)
      {
        if( s[j] == m[i] &&
            does_match_8_8(s,j,sl,m,i,ml))
          return 1;
      }
      return 0;


    case '[':
      DOES_MATCH_CLASS(m[i],s[j],ml);
      break;

     case '\\':
       if (++i==ml) return 0;
       /* FALLTHRU */

     default:
         if(j>=sl || m[i] != s[j] )
             return 0;
         j++;
    }
  }
  return j==sl;
}

static int does_match_16_8( const unsigned short *s, int j, int sl,
                            const unsigned char *m, int i, int ml)
{
  for (; i<ml; i++)
  {
    switch (m[i])
    {
     case '?':
       if(j++>=sl) return 0;
       break;

     case '*':
      while(m[i] == '*' && i<ml )
        i++;
      while( m[i] == '?' && i<ml && j<sl)
      {
        i++;
        j++;
      }
      if (i==ml) return 1;

      for (;j<sl;j++)
      {
        if( s[j] == m[i] &&
            does_match_16_8(s,j,sl,m,i,ml))
          return 1;
      }
      return 0;


    case '[':
      DOES_MATCH_CLASS(m[i],s[j],ml);
      break;

     case '\\':
       if (++i==ml) return 0;
       /* FALLTHRU */

     default:
         if(j>=sl || m[i] != s[j] )
             return 0;
         j++;
    }
  }
  return j==sl;
}


/* Check if the string s[0..len[ matches the glob m[0..mlen[ */
static int does_match_x_x(struct pike_string *s,int j,
                          struct pike_string *m,int i)
{
  for (; i<m->len; i++)
  {
    switch (index_shared_string(m,i))
    {
     case '?':
       if(j++>=s->len) return 0;
       break;

     case '*':
      i++;
      if (i==m->len) return 1;	/* slut */

      for (;j<s->len;j++)
	if (does_match_x_x(s,j,m,i))
	  return 1;

      return 0;

    case '[':
      DOES_MATCH_CLASS(index_shared_string(m,i),index_shared_string(s,j),m->len);
      break;

     case '\\':
       if (++i==m->len) return 0;
       /* FALLTHRU */
     default:
       if(j>=s->len ||
	  index_shared_string(m,i)!=index_shared_string(s,j)) return 0;
       j++;
    }
  }
  return j==s->len;
}

static int does_match(struct pike_string *s,int j,
		      struct pike_string *m,int i)
{
    if( s->size_shift + m->size_shift == 0 )
      return does_match_8_8((const unsigned char*)s->str, j, s->len,
                           (const unsigned char*)m->str, i, m->len);
    if( s->size_shift==1 && m->size_shift == 0 )
      return does_match_16_8((const unsigned short*)s->str, j, s->len,
                             (const unsigned char*)m->str, i, m->len);
    return does_match_x_x( s,j,m,i );
}
/*! @decl int(0..1) glob(string glob, string str)
 *! @decl string glob(array(string) glob, string str)
 *! @decl array(string) glob(string glob, array(string) str)
 *! @decl array(string) glob(array(string) glob, array(string) str)
 *!
 *! Match strings against a glob pattern.
 *!
 *! @param glob
 *!   @mixed
 *!    @type string
 *!      The glob pattern. A question sign ('?') matches any character
 *!      and an asterisk ('*') matches a string of arbitrary length. All
 *!      other characters only match themselves.
 *!    @type array(string)
 *!      the function returns the matching glob if any of the given
 *!      patterns match. Otherwise 0. If the second argument is an array
 *!      it will behave as if the first argument is a string (see
 *!      below)
 *!  @endmixed
 *!
 *! @param str
 *!   @mixed
 *!     @type string
 *!       @expr{1@} is returned if the string @[str] matches @[glob],
 *!       @expr{0@} (zero) otherwise.
 *!
 *!     @type array(string)
 *!       All strings in the array @[str] are matched against @[glob],
 *!       and those that match are returned in an array (in the same
 *!       order).
 *!   @endmixed
 *!
 *! @seealso
 *!   @[sscanf()], @[Regexp]
 */

static struct pike_string *any_does_match( struct svalue *items, int nglobs, struct pike_string *str )
{
   INT32 i;
   for( i =0; i<nglobs; i++ )
   {
     struct pike_string *str2 = items[i].u.string;
     if( str == str2 )
       return str;
     if( does_match(str,0,str2,0) )
       return str2;
   }
   return 0;
}

PMOD_EXPORT void f_glob(INT32 args)
{
  INT32 i;
  struct array *a;
  struct svalue *glob;
  struct pike_string *tmp;
  int nglobs;

  if(args != 2)
    SIMPLE_WRONG_NUM_ARGS_ERROR("glob", 2);

  if (TYPEOF(Pike_sp[-args]) == T_STRING)
  {
      glob=Pike_sp-args;
      nglobs = 1;
  }
  else if( TYPEOF(Pike_sp[-args]) == PIKE_T_ARRAY)
  {
      struct array *ga = Pike_sp[-args].u.array;
      glob = ga->item;
      nglobs = ga->size;
      for( i=0; i<nglobs; i++ )
	  if( TYPEOF(ga->item[i]) != PIKE_T_STRING )
              SIMPLE_ARG_TYPE_ERROR("glob", 1, "string|array(string)");
  }
  else
      SIMPLE_ARG_TYPE_ERROR("glob", 1, "string|array(string)");


  switch(TYPEOF(Pike_sp[1-args]))
  {
  case T_STRING:
    tmp = any_does_match(glob,nglobs,Pike_sp[1-args].u.string);
    if(TYPEOF(Pike_sp[-args]) == PIKE_T_ARRAY && tmp)
      ref_push_string(tmp);
    else
      push_int(!!tmp);
    stack_pop_n_elems_keep_top(2);
    break;

  case T_ARRAY: {
    unsigned matches = 0;
    struct svalue *res;
    a=Pike_sp[1-args].u.array;

    if( (a->type_field & ~BIT_STRING) &&
	(array_fix_type_field(a) & ~BIT_STRING) )
      SIMPLE_ARG_TYPE_ERROR("glob", 2, "string|array(string)");

    check_stack(120);
    BEGIN_AGGREGATE_ARRAY (MINIMUM (a->size, 120)) {
      res = Pike_sp - 1;

      for(i=0;i<a->size;i++)
      {
        if(any_does_match(glob,nglobs,ITEM(a)[i].u.string) )
	{
	  matches++;
	  ref_push_string(ITEM(a)[i].u.string);
	  DO_AGGREGATE_ARRAY (120);
	}
      }
      /* We know what this array contains - avoid array_fix_type_field
       * in END_AGGREGATE_ARRAY. */
      res->u.array->type_field = matches ? BIT_STRING : 0;
    } END_AGGREGATE_ARRAY;

    stack_pop_n_elems_keep_top (2);
    break;
  }

  default:
    SIMPLE_ARG_TYPE_ERROR("glob", 2, "string|array(string)");
  }
}

/* comb_merge */

/*! @module Array
 */

/*! @decl array(int) interleave_array(array(mapping(int:mixed)) tab)
 *!
 *!   Interleave a sparse matrix.
 *!
 *!   Returns an array with offsets that describe how to shift the
 *!   rows of @[tab] so that only at most one non-zero value exists in
 *!   every column.
 */
static void f_interleave_array(INT32 args)
{
  struct array *arr = NULL;
  struct array *min = NULL;
  struct array *order = NULL;
  int max = 0;
  int nelems = 0;
  int i;

  get_all_args(NULL, args, "%a", &arr);

  /* We're not interrested in any other arguments. */
  pop_n_elems(args-1);

  if( (arr->type_field & ~BIT_MAPPING) &&
      (array_fix_type_field(arr) & ~BIT_MAPPING) )
    SIMPLE_ARG_TYPE_ERROR("interleave_array", 1, "array(mapping(int:mixed))");

  /* The order array */
  ref_push_array(arr);
  f_indices(1);
  order = Pike_sp[-1].u.array;

  /* The min array */
  push_array(min = allocate_array(arr->size));

  /* Initialize the min array */
  for (i = 0; i < arr->size; i++) {
    struct mapping_data *md;
    /* e and k are used by NEW_MAPPING_LOOP() */
    INT32 e;
    struct keypair *k;
    INT_TYPE low = MAX_INT_TYPE;
#ifdef PIKE_DEBUG
    if (TYPEOF(ITEM(arr)[i]) != T_MAPPING) {
      Pike_error("Element %d is not a mapping!\n", i);
    }
#endif /* PIKE_DEBUG */
    md = ITEM(arr)[i].u.mapping->data;
    NEW_MAPPING_LOOP(md) {
      if (TYPEOF(k->ind) != T_INT) {
	Pike_error("Index not an integer in mapping %d!\n", i);
      }
      if (low > k->ind.u.integer) {
	low = k->ind.u.integer;
	if (low < 0) {
	  Pike_error("Index %"PRINTPIKEINT"d in mapping %d is negative!\n",
		low, i);
	}
      }
      if (max < k->ind.u.integer) {
	max = k->ind.u.integer;
      }
      nelems++;
    }
    ITEM(min)[i].u.integer = low;
  }

  min->type_field = BIT_INT;
  ref_push_array(order);
  f_sort(2);	/* Sort the order array on the minimum index */

  /* State on stack now:
   *
   * array(mapping(int:mixed))	arr
   * array(int)			order
   * array(int)			min (now sorted)
   */

  /* Now we can start with the real work... */
  {
    char *tab;
    int size;
    int minfree = 0;

    /* Initialize the lookup table */
    max += 1;
    max *= 2;
    /* max will be the padding at the end. */
    size = (nelems + max) * 8;	/* Initial size */
    tab = xcalloc(size + max, 1);

    for (i = 0; i < order->size; i++) {
      int low = ITEM(min)[i].u.integer;
      int j = ITEM(order)[i].u.integer;
      int offset = 0;
      int ok = 0;
      struct mapping *m;
      struct mapping_data *md;
      INT32 e;
      struct keypair *k;

      if (! m_sizeof(m = ITEM(arr)[j].u.mapping)) {
	/* Not available */
	ITEM(min)[i].u.integer = -1;
	continue;
      }

      if (low < minfree) {
	offset = minfree - low;
      } else {
	minfree = offset;
      }

      md = m->data;
      while (!ok) {
	ok = 1;
	NEW_MAPPING_LOOP(md) {
	  int ind = k->ind.u.integer;
	  if (tab[offset + ind]) {
	    ok = 0;
	    while (tab[++offset + ind])
	      ;
	  }
	}
      }
      NEW_MAPPING_LOOP(md) {
	tab[offset + k->ind.u.integer] = 1;
      }
      while(tab[minfree]) {
	minfree++;
      }
      ITEM(min)[i].u.integer = offset;

      /* Check need for realloc */
      if (offset >= size) {
	char *newtab = realloc(tab, size*2 + max);
	if (!newtab) {
	  free(tab);
	  Pike_error("Couldn't extend table!\n");
	}
	tab = newtab;
	memset(tab + size + max, 0, size);
	size = size * 2;
      }
    }
    free(tab);
  }

  /* We want these two to survive the stackpopping. */
  add_ref(min);
  add_ref(order);

  pop_n_elems(3);

  /* Return value */
  ref_push_array(min);

  /* Restore the order */
  push_array(order);
  push_array(min);
  f_sort(2);
  pop_stack();
}

/* longest_ordered_sequence */

static int find_gt(struct array *a, int i, int *stack, int top)
{
  struct svalue *x = a->item + i;
  int l,h;

  /* FIXME: Should it perhaps be is_ge below instead? */
  if (!top || !is_lt(x, a->item + stack[top - 1])) return top;

  l = 0;
  h = top;

  while (l < h) {
    int middle = (l + h)/2;
    if (!is_gt(a->item + stack[middle], x)) {
      l = middle+1;
    } else {
      h = middle;
    }
  }
  return l;
}

static struct array *longest_ordered_sequence(struct array *a)
{
  int *stack;
  int *links;
  int i, top=0, ltop=-1;
  struct array *res;
  ONERROR tmp;
  ONERROR tmp2;

  if(!a->size) {
    add_ref(&empty_array);
    return &empty_array;
  }

  stack = calloc(sizeof(int), a->size);
  links = calloc(sizeof(int), a->size);

  if (!stack || !links)
  {
    if (stack) free(stack);
    if (links) free(links);
    return NULL;
  }

  /* is_gt(), is_lt() and low_allocate_array() can generate errors. */

  SET_ONERROR(tmp, free, stack);
  SET_ONERROR(tmp2, free, links);

  for (i=0; i<a->size; i++) {
    int pos;

    pos = find_gt(a, i, stack, top);

    if (pos == top) {
      top++;
      ltop = i;
    }
    if (pos != 0)
      links[i] = stack[pos-1];
    else
      links[i] = -1;
    stack[pos] = i;
  }

  res = low_allocate_array(top, 0);
  while (ltop != -1)
  {
    ITEM(res)[--top].u.integer = ltop;
    ltop = links[ltop];
  }
  res->type_field = BIT_INT;

  UNSET_ONERROR(tmp2);
  UNSET_ONERROR(tmp);

  free(stack);
  free(links);
  return res;
}

/*! @decl array(int) longest_ordered_sequence(array a)
 *!
 *!   Find the longest ordered sequence of elements.
 *!
 *!   This function returns an array of the indices in the longest
 *!   ordered sequence of elements in the array.
 *!
 *! @seealso
 *!   @[diff()]
 */
static void f_longest_ordered_sequence(INT32 args)
{
  struct array *a = NULL;
  struct array *aa = NULL;

  get_all_args(NULL, args, "%a", &a);

  /* THREADS_ALLOW(); */

  aa = longest_ordered_sequence(a);

  /* THREADS_DISALLOW(); */

  if (!aa) {
    SIMPLE_OUT_OF_MEMORY_ERROR("longest_ordered_sequence",
			       (int)sizeof(int *)*a->size*2);
  }

  push_array(aa);
}

/**** diff ************************************************************/

static struct array* diff_compare_table(struct array *a,struct array *b,int *u)
{
   struct array *res;
   struct mapping *map;
   struct svalue *pval;
   int i;
   TYPE_FIELD types;

   if (u) {
     *u = 0;	/* Unique rows in array b */
   }

   map=allocate_mapping(256);
   push_mapping(map); /* in case of out of memory */

   for (i=0; i<b->size; i++)
   {
      pval=low_mapping_lookup(map,b->item+i);
      if (!pval)
      {
	 struct svalue val;
	 SET_SVAL(val, T_ARRAY, 0, array, low_allocate_array(1,1));
	 ITEM(val.u.array)[0].u.integer=i;
	 val.u.array->type_field = BIT_INT;
	 mapping_insert(map,ITEM(b)+i,&val);
	 free_svalue(&val);
	 if (u) {
	   (*u)++;
	 }
      }
      else
      {
	struct array *a = pval->u.array=
	  resize_array(pval->u.array,pval->u.array->size+1);
	struct svalue *s = ITEM(a) + pval->u.array->size-1;
	SET_SVAL(*s, T_INT, NUMBER_NUMBER, integer, i);
      }
   }

   res=low_allocate_array(a->size,0);
   types = 0;

   for (i=0; i<a->size; i++)
   {
      pval=low_mapping_lookup(map,a->item+i);
      if (!pval)
      {
	 SET_SVAL(ITEM(res)[i], T_ARRAY, 0, array, &empty_array);
	 add_ref(&empty_array);
	 types |= BIT_ARRAY;
      }
      else
      {
	 assign_svalue(ITEM(res)+i,pval);
	 types |= 1 << TYPEOF(ITEM(res)[i]);
      }
   }

   res->type_field = types;
   pop_stack();
   return res;
}

struct diff_magic_link
{
   int x;
   int refs;
   struct diff_magic_link *prev;
};

struct diff_magic_link_pool
{
   struct diff_magic_link *firstfree;
   struct diff_magic_link_pool *next;
   int firstfreenum;
   struct diff_magic_link dml[1];
};

struct diff_magic_link_head
{
  unsigned int depth;
  struct diff_magic_link *link;
};

#define DMLPOOLSIZE 16384

static int dmls=0;

static inline struct diff_magic_link_pool*
         dml_new_pool(struct diff_magic_link_pool **pools)
{
   struct diff_magic_link_pool *new;

   new=malloc(sizeof(struct diff_magic_link_pool)+
	      sizeof(struct diff_magic_link)*DMLPOOLSIZE);
   if (!new) return NULL; /* fail */

   new->firstfreenum=0;
   new->firstfree=NULL;
   new->next=*pools;
   *pools=new;
   return *pools;
}

static inline struct diff_magic_link*
       dml_new(struct diff_magic_link_pool **pools)
{
   struct diff_magic_link *new;
   struct diff_magic_link_pool *pool;

   dmls++;

   if ( *pools && (new=(*pools)->firstfree) )
   {
      (*pools)->firstfree=new->prev;
      new->prev=NULL;
      return new;
   }

   pool=*pools;
   while (pool)
   {
      if (pool->firstfreenum<DMLPOOLSIZE)
	 return pool->dml+(pool->firstfreenum++);
      pool=pool->next;
   }

   if ( (pool=dml_new_pool(pools)) )
   {
      pool->firstfreenum=1;
      return pool->dml;
   }

   return NULL;
}

static inline void dml_free_pools(struct diff_magic_link_pool *pools)
{
   struct diff_magic_link_pool *pool;

   while (pools)
   {
      pool=pools->next;
      free(pools);
      pools=pool;
   }
}

static inline void dml_delete(struct diff_magic_link_pool *pools,
			      struct diff_magic_link *dml)
{
  struct diff_magic_link *prev;
  while(1)
  {
    prev=dml->prev;
    dmls--;
    dml->prev=pools->firstfree;
    pools->firstfree=dml;
    if (prev && !--prev->refs)
      dml=prev;
    else
      break;
  }
}

static inline int diff_ponder_stack(int x,
				    struct diff_magic_link **dml,
				    int top)
{
   int middle,a,b;

   a=0;
   b=top;
   while (b>a)
   {
      middle=(a+b)/2;
      if (dml[middle]->x<x) a=middle+1;
      else if (dml[middle]->x>x) b=middle;
      else return middle;
   }
   if (a<top && dml[a]->x<x) a++;
   return a;
}

static inline int diff_ponder_array(int x,
				    struct svalue *arr,
				    int top)
{
   int middle,a,b;

   a=0;
   b=top;
   while (b>a)
   {
      middle=(a+b)/2;
      if (arr[middle].u.integer<x) a=middle+1;
      else if (arr[middle].u.integer>x) b=middle;
      else return middle;
   }
   if (a<top && arr[a].u.integer<x) a++;
   return a;
}

/*
 * The Grubba-Mirar Longest Common Sequence algorithm.
 *
 * This algorithm is O((Na * Nb / K)*lg(Na * Nb / K)), where:
 *
 *  Na == sizeof(a)
 *  Nb == sizeof(b)
 *  K  == sizeof(correlation(a,b))
 *
 * For binary data:
 *  K == 256 => O(Na * Nb * lg(Na * Nb)),
 *  Na ~= Nb ~= N => O(N� * lg(N))
 *
 * For ascii data:
 *  K ~= C * min(Na, Nb), C constant => O(max(Na, Nb)*lg(max(Na,Nb))),
 *  Na ~= Nb ~= N => O(N * lg(N))
 *
 * diff_longest_sequence() takes two arguments:
 *  cmptbl == diff_compare_table(a, b)
 *  blen == sizeof(b) >= max(@(cmptbl*({})))
 */
static struct array *diff_longest_sequence(struct array *cmptbl, int blen)
{
   int i,j,top=0;
   struct array *a;
   struct diff_magic_link_pool *pools=NULL;
   struct diff_magic_link *dml;
   struct diff_magic_link **stack;
   char *marks;

   if(!cmptbl->size)
     return allocate_array(0);

   stack = xcalloc(sizeof(struct diff_magic_link*), cmptbl->size);

   /* NB: marks is used for optimization purposes only */
   marks = calloc(blen, 1);

   if (!marks && blen) {
     int args = 0;
     free(stack);
     SIMPLE_OUT_OF_MEMORY_ERROR("diff_longest_sequence", blen);
   }

#ifdef DIFF_DEBUG
   fprintf(stderr, "\n\nDIFF: sizeof(cmptbl)=%d, blen=%d\n",
	   cmptbl->size, blen);
#endif /* DIFF_DEBUG */

   for (i = 0; i<cmptbl->size; i++)
   {
      struct svalue *inner=cmptbl->item[i].u.array->item;

#ifdef DIFF_DEBUG
      fprintf(stderr, "DIFF: i=%d\n", i);
#endif /* DIFF_DEBUG */

      for (j = cmptbl->item[i].u.array->size; j--;)
      {
	 int x = inner[j].u.integer;

#ifdef DIFF_DEBUG
	 fprintf(stderr, "DIFF:  j=%d, x=%d\n", j, x);
#endif /* DIFF_DEBUG */
#ifdef PIKE_DEBUG
	 if (x >= blen) {
	   Pike_fatal("diff_longest_sequence(): x:%d >= blen:%d\n", x, blen);
	 } else if (x < 0) {
	   Pike_fatal("diff_longest_sequence(): x:%d < 0\n", x);
	 }
#endif /* PIKE_DEBUG */
	 if (!marks[x]) {
	   int pos;

	   if (top && x<=stack[top-1]->x) {
	     /* Find the insertion point. */
	     pos = diff_ponder_stack(x, stack, top);
	     if (pos != top) {
	       /* Not on the stack anymore. */
	       marks[stack[pos]->x] = 0;
	     }
	   } else
	     pos=top;

#ifdef DIFF_DEBUG
	   fprintf(stderr, "DIFF:  pos=%d\n", pos);
#endif /* DIFF_DEBUG */

	   /* This part is only optimization (j accelleration). */
	   if (pos && j)
	   {
	     if (!marks[inner[j-1].u.integer])
	     {
	       /* Find the element to insert. */
	       j = diff_ponder_array(stack[pos-1]->x+1, inner, j);
	       x = inner[j].u.integer;
	     }
	   }
	   else
	   {
	     j = 0;
	     x = inner->u.integer;
	   }

#ifdef DIFF_DEBUG
	   fprintf(stderr, "DIFF: New j=%d, x=%d\n", j, x);
#endif /* DIFF_DEBUG */
#ifdef PIKE_DEBUG
	   if (x >= blen) {
	     Pike_fatal("diff_longest_sequence(): x:%d >= blen:%d\n", x, blen);
	   } else if (x < 0) {
	     Pike_fatal("diff_longest_sequence(): x:%d < 0\n", x);
	   }
#endif /* PIKE_DEBUG */

	   /* Put x on the stack. */
	   marks[x] = 1;
	   if (pos == top)
	   {
#ifdef DIFF_DEBUG
	     fprintf(stderr, "DIFF:  New top element\n");
#endif /* DIFF_DEBUG */

	     if (! (dml=dml_new(&pools)) )
	     {
	       int args = 0;
	       dml_free_pools(pools);
	       free(stack);
	       SIMPLE_OUT_OF_MEMORY_ERROR("diff_longest_sequence",
					  sizeof(struct diff_magic_link_pool) +
					  sizeof(struct diff_magic_link) *
					  DMLPOOLSIZE);
	     }

	     dml->x = x;
	     dml->refs = 1;

	     if (pos)
	       (dml->prev = stack[pos-1])->refs++;
	     else
	       dml->prev = NULL;

	     top++;

	     stack[pos] = dml;
	   } else if (pos &&
		      stack[pos]->refs == 1 &&
		      stack[pos-1] == stack[pos]->prev)
	   {
#ifdef DIFF_DEBUG
	     fprintf(stderr, "DIFF:  Optimized case\n");
#endif /* DIFF_DEBUG */

	     /* Optimization. */
	     stack[pos]->x = x;
	   } else {
#ifdef DIFF_DEBUG
	     fprintf(stderr, "DIFF:  Generic case\n");
#endif /* DIFF_DEBUG */

	     if (! (dml=dml_new(&pools)) )
	     {
	       int args = 0;
	       dml_free_pools(pools);
	       free(stack);
	       SIMPLE_OUT_OF_MEMORY_ERROR("diff_longest_sequence",
					  sizeof(struct diff_magic_link_pool) +
					  sizeof(struct diff_magic_link) *
					  DMLPOOLSIZE);
	     }

	     dml->x = x;
	     dml->refs = 1;

	     if (pos)
	       (dml->prev = stack[pos-1])->refs++;
	     else
	       dml->prev = NULL;

	     if (!--stack[pos]->refs)
	       dml_delete(pools, stack[pos]);

	     stack[pos] = dml;
	   }
#ifdef DIFF_DEBUG
	 } else {
	   fprintf(stderr, "DIFF:  Already marked (%d)!\n", marks[x]);
#endif /* DIFF_DEBUG */
	 }
      }
#ifdef DIFF_DEBUG
      for(j=0; j < top; j++) {
	fprintf(stderr, "DIFF:  stack:%d, mark:%d\n",
		stack[j]->x, marks[stack[j]->x]);
      }
#endif /* DIFF_DEBUG */
   }

   /* No need for marks anymore. */

   free(marks);

   /* FIXME(?) memory unfreed upon error here. */
   a=low_allocate_array(top,0);
   if (top)
   {
       dml=stack[top-1];
       while (dml)
       {
	  ITEM(a)[--top].u.integer=dml->x;
	  dml=dml->prev;
       }
       a->type_field = BIT_INT;
   }

   free(stack);
   dml_free_pools(pools);
   return a;
}

/*
 * The dynamic programming Longest Common Sequence algorithm.
 *
 * This algorithm is O(Na * Nb), where:
 *
 *  Na == sizeof(a)
 *  Nb == sizeof(b)
 *
 * This makes it faster than the G-M algorithm on binary data,
 * but slower on ascii data.
 *
 * NOT true! The G-M algorithm seems to be faster on most data anyway.
 *	/grubba 1998-05-19
 */
static struct array *diff_dyn_longest_sequence(struct array *cmptbl, int blen)
{
  struct array *res = NULL;
  struct diff_magic_link_head *table;
  struct diff_magic_link_pool *dml_pool = NULL;
  struct diff_magic_link *dml;
  unsigned int sz = (unsigned int)cmptbl->size;
  unsigned int i;
  unsigned int off1 = 0;
  unsigned int off2 = blen + 1;
  ONERROR err;

  table = xcalloc(sizeof(struct diff_magic_link_head)*2, off2);

  /* FIXME: Assumes NULL is represented with all zeroes */
  /* NOTE: Scan strings backwards to get the same result as the G-M
   * algorithm.
   */
  for (i = sz; i--;) {
    struct array *boff = cmptbl->item[i].u.array;

#ifdef DIFF_DEBUG
    fprintf(stderr, "  i:%d\n", i);
#endif /* DIFF_DEBUG */

    if (boff->size) {
      unsigned int bi;
      unsigned int base = blen;
      unsigned int tmp = off1;
      off1 = off2;
      off2 = tmp;

      for (bi = boff->size; bi--;) {
	unsigned int ib = boff->item[bi].u.integer;

#ifdef DIFF_DEBUG
	fprintf(stderr, "    Range [%d - %d] differ\n", base - 1, ib + 1);
#endif /* DIFF_DEBUG */
	while ((--base) > ib) {
	  /* Differ */
	  if (table[off1 + base].link) {
	    if (!--(table[off1 + base].link->refs)) {
	      dml_delete(dml_pool, table[off1 + base].link);
	    }
	  }
	  /* FIXME: Should it be > or >= here to get the same result
	   * as with the G-M algorithm?
	   */
	  if (table[off2 + base].depth > table[off1 + base + 1].depth) {
	    table[off1 + base].depth = table[off2 + base].depth;
	    dml = (table[off1 + base].link = table[off2 + base].link);
	  } else {
	    table[off1 + base].depth = table[off1 + base + 1].depth;
	    dml = (table[off1 + base].link = table[off1 + base + 1].link);
	  }
	  if (dml) {
	    dml->refs++;
	  }
	}
	/* Equal */
#ifdef DIFF_DEBUG
	fprintf(stderr, "    Equal\n");
#endif /* DIFF_DEBUG */

	if (table[off1 + ib].link) {
	  if (!--(table[off1 + ib].link->refs)) {
	    dml_delete(dml_pool, table[off1 + ib].link);
	  }
	}
	table[off1 + ib].depth = table[off2 + ib + 1].depth + 1;
	dml = (table[off1 + ib].link = dml_new(&dml_pool));
	if (!dml) {
	  int args = 0;
	  dml_free_pools(dml_pool);
	  free(table);
	  SIMPLE_OUT_OF_MEMORY_ERROR("diff_dyn_longest_sequence",
				     sizeof(struct diff_magic_link_pool) +
				     sizeof(struct diff_magic_link) *
				     DMLPOOLSIZE);
	}
	dml->refs = 1;
	dml->prev = table[off2 + ib + 1].link;
	if (dml->prev) {
	  dml->prev->refs++;
	}
	dml->x = ib;
      }
#ifdef DIFF_DEBUG
      fprintf(stderr, "    Range [0 - %d] differ\n", base-1);
#endif /* DIFF_DEBUG */
      while (base--) {
	/* Differ */
	if (table[off1 + base].link) {
	  if (!--(table[off1 + base].link->refs)) {
	    dml_delete(dml_pool, table[off1 + base].link);
	  }
	}
	/* FIXME: Should it be > or >= here to get the same result
	 * as with the G-M algorithm?
	 */
	if (table[off2 + base].depth > table[off1 + base + 1].depth) {
	  table[off1 + base].depth = table[off2 + base].depth;
	  dml = (table[off1 + base].link = table[off2 + base].link);
	} else {
	  table[off1 + base].depth = table[off1 + base + 1].depth;
	  dml = (table[off1 + base].link = table[off1 + base + 1].link);
	}
	if (dml) {
	  dml->refs++;
	}
      }
    }
  }

  /* Convert table into res */
  sz = table[off1].depth;
  dml = table[off1].link;
  free(table);
#ifdef DIFF_DEBUG
  fprintf(stderr, "Result array size:%d\n", sz);
#endif /* DIFF_DEBUG */

  if(dml_pool) SET_ONERROR(err, dml_free_pools, dml_pool);
  res = allocate_array(sz);
  if(dml_pool) UNSET_ONERROR(err);

  i = 0;
  while(dml) {
#ifdef PIKE_DEBUG
    if (i >= sz) {
      Pike_fatal("Consistency error in diff_dyn_longest_sequence()\n");
    }
#endif /* PIKE_DEBUG */
#ifdef DIFF_DEBUG
    fprintf(stderr, "  %02d: %d\n", i, dml->x);
#endif /* DIFF_DEBUG */
    res->item[i].u.integer = dml->x;
    dml = dml->prev;
    i++;
  }
  res->type_field = BIT_INT;
#ifdef PIKE_DEBUG
  if (i != sz) {
    Pike_fatal("Consistency error in diff_dyn_longest_sequence()\n");
  }
#endif /* PIKE_DEBUG */

  dml_free_pools(dml_pool);
  return(res);
}

static struct array* diff_build(struct array *a,
				struct array *b,
				struct array *seq)
{
   struct array *ad,*bd;
   ptrdiff_t bi, ai, lbi, lai, i, eqstart;

   /* FIXME(?) memory unfreed upon error here (and later) */
   ad=low_allocate_array(0,32);
   bd=low_allocate_array(0,32);

   eqstart=0;
   lbi=bi=ai=-1;
   for (i=0; i<seq->size; i++)
   {
      bi=seq->item[i].u.integer;

      if (bi!=lbi+1 || !is_equal(a->item+ai+1,b->item+bi))
      {
	 /* insert the equality */
	 if (lbi>=eqstart)
	 {
	    push_array(friendly_slice_array(b,eqstart,lbi+1));
	    ad=append_array(ad,Pike_sp-1);
	    bd=append_array(bd,Pike_sp-1);
	    pop_stack();
	 }
	 /* insert the difference */
	 lai=ai;
	 ai=array_search(a,b->item+bi,ai+1)-1;

	 push_array(friendly_slice_array(b,lbi+1,bi));
	 bd=append_array(bd, Pike_sp-1);
	 pop_stack();

	 push_array(friendly_slice_array(a,lai+1,ai+1));
	 ad=append_array(ad,Pike_sp-1);
	 pop_stack();

	 eqstart=bi;
      }
      ai++;
      lbi=bi;
   }

   if (lbi>=eqstart)
   {
      push_array(friendly_slice_array(b,eqstart,lbi+1));
      ad=append_array(ad,Pike_sp-1);
      bd=append_array(bd,Pike_sp-1);
      pop_stack();
   }

   if (b->size>bi+1 || a->size>ai+1)
   {
      push_array(friendly_slice_array(b,lbi+1,b->size));
      bd=append_array(bd, Pike_sp-1);
      pop_stack();

      push_array(friendly_slice_array(a,ai+1,a->size));
      ad=append_array(ad,Pike_sp-1);
      pop_stack();
   }

   push_array(ad);
   push_array(bd);
   return aggregate_array(2);
}

/*! @decl array permute(array in, int(0..) number)
 *!
 *!   Give a specified permutation of an array.
 *!
 *!   The number of permutations is equal to @expr{sizeof(@[in])!@}
 *!   (the factorial of the size of the given array).
 *!
 *! @seealso
 *!   @[shuffle()]
 */
PMOD_EXPORT void f_permute( INT32 args )
{
  INT_TYPE q, i=0, n;
  struct array *a;
  struct svalue *it;

  get_all_args(NULL, args, "%a%+", &a, &n);

  if( a->refs>1 )
  {
    a = copy_array( a );
    push_array( a );
  }
  else
    pop_n_elems(args-1);

  q = a->size;
  it = a->item;
  while( n && q )
  {
    int x = n % q;
    n /= q;
    q--;
    if( x )
    {
      struct svalue tmp;
      tmp     = it[i];
      it[i]   = it[i+x];
      it[i+x] = tmp;
    }
    i++;
  }
}

/*! @decl array(array(array)) diff(array a, array b)
 *!
 *!   Calculates which parts of the arrays that are common to both, and
 *!   which parts that are not.
 *!
 *! @returns
 *!   Returns an array with two elements, the first is an array of parts in
 *!   array @[a], and the second is an array of parts in array @[b].
 *!
 *! @seealso
 *!   @[diff_compare_table()], @[diff_longest_sequence()],
 *!   @[String.fuzzymatch()]
 */
PMOD_EXPORT void f_diff(INT32 args)
{
   struct array *seq;
   struct array *cmptbl;
   struct array *a, *b;
   int uniq;

   get_all_args(NULL, args, "%a%a", &a, &b);

   if ((a == b) || !a->size || !b->size) {
     if (!a->size && !b->size) {
       /* Both arrays are empty. */
       ref_push_array(a);
       ref_push_array(b);
       f_aggregate(2);
     } else {
       /* The arrays are equal or one of them is empty. */
       ref_push_array(a);
       f_aggregate(1);
       ref_push_array(b);
       f_aggregate(1);
       f_aggregate(2);
     }
     return;
   }

   cmptbl = diff_compare_table(a, b, &uniq);
   push_array(cmptbl);

#ifdef ENABLE_DYN_DIFF
   if (uniq * 100 > cmptbl->size) {
#endif /* ENABLE_DYN_DIFF */
#ifdef DIFF_DEBUG
     fprintf(stderr, "diff: Using G-M algorithm, u:%d, s:%d\n",
	     uniq, cmptbl->size);
#endif /* DIFF_DEBUG */
     seq = diff_longest_sequence(cmptbl, b->size);
#ifdef ENABLE_DYN_DIFF
   } else {
#ifdef DIFF_DEBUG
     fprintf(stderr, "diff: Using dyn algorithm, u:%d, s:%d\n",
	     uniq, cmptbl->size);
#endif /* DIFF_DEBUG */
     seq = diff_dyn_longest_sequence(cmptbl, b->size);
   }
#endif /* ENABLE_DYN_DIFF */

   push_array(seq);
   push_array(diff_build(a,b,seq));
}

/*! @decl array(array(int)) diff_compare_table(array a, array b)
 *!
 *!   Returns an array which maps from index in @[a] to corresponding
 *!   indices in @[b].
 *!
 *! @pre{
 *! > Array.diff_compare_table( ({ "a","b","c" }), ({ "b", "b", "c", "d", "b" }));
 *! Result: ({
 *!             ({ }),
 *!             ({
 *!                 0,
 *!                 1,
 *!                 4
 *!             }),
 *!             ({
 *!                 2
 *! 	        })
 *!         })
 *! @}
 *!
 *! @seealso
 *!   @[diff()], @[diff_longest_sequence()], @[String.fuzzymatch()]
 */
PMOD_EXPORT void f_diff_compare_table(INT32 args)
{
  struct array *a;
  struct array *b;

  get_all_args(NULL, args, "%a%a", &a, &b);

  push_array(diff_compare_table(a, b, NULL));
}

/*! @decl array(int) diff_longest_sequence(array a, array b)
 *!
 *!   Gives the longest sequence of indices in @[b] that have corresponding
 *!   values in the same order in @[a].
 *!
 *! @seealso
 *!   @[diff()], @[diff_compare_table()], @[String.fuzzymatch()]
 */
PMOD_EXPORT void f_diff_longest_sequence(INT32 args)
{
  struct array *a;
  struct array *b;
  struct array *cmptbl;

  get_all_args(NULL, args, "%a%a", &a, &b);

  cmptbl = diff_compare_table(a, b, NULL);
  push_array(cmptbl);

  push_array(diff_longest_sequence(cmptbl, b->size));
}

/*! @decl array(int) diff_dyn_longest_sequence(array a, array b)
 *!
 *!   Gives the longest sequence of indices in @[b] that have corresponding
 *!   values in the same order in @[a].
 *!
 *!   This function performs the same operation as @[diff_longest_sequence()],
 *!   but uses a different algorithm, which in some rare cases might be faster
 *!   (usually it's slower though).
 *!
 *! @seealso
 *!   @[diff_longest_sequence()], @[diff()], @[diff_compare_table()],
 *!   @[String.fuzzymatch()]
 */
PMOD_EXPORT void f_diff_dyn_longest_sequence(INT32 args)
{
  struct array *a;
  struct array *b;
  struct array *cmptbl;

  get_all_args(NULL, args, "%a%a", &a, &b);

  cmptbl=diff_compare_table(a, b, NULL);
  push_array(cmptbl);

  push_array(diff_dyn_longest_sequence(cmptbl, b->size));
}

/*! @endmodule
 */

/**********************************************************************/

static struct callback_list memory_usage_callback;

struct callback *add_memory_usage_callback(callback_func call,
					  void *arg,
					  callback_func free_func)
{
  return add_to_callback(&memory_usage_callback, call, arg, free_func);
}

/*! @decl mapping(string:int) memory_usage()
 *! @belongs Debug
 *!
 *!   Check memory usage.
 *!
 *!   This function is mostly intended for debugging. It delivers a mapping
 *!   with information about how many arrays/mappings/strings etc. there
 *!   are currently allocated and how much memory they use.
 *!
 *!   The entries in the mapping are typically paired, with one
 *!   named @expr{"num_" + SYMBOL + "s"@} containing a count,
 *!   and the other named @expr{SYMBOL + "_bytes"@} containing
 *!   a best effort approximation of the size in bytes.
 *!
 *! @note
 *!   Exactly what fields this function returns is version dependant.
 *!
 *! @seealso
 *!   @[_verify_internals()]
 */
PMOD_EXPORT void f__memory_usage(INT32 args)
{
  size_t num,size;
  struct svalue *ss;
#ifdef USE_DL_MALLOC
  struct mallinfo mi = dlmallinfo();
#elif HAVE_MALLINFO2
  struct mallinfo2 mi = mallinfo2();
#elif HAVE_MALLINFO
  struct mallinfo mi = mallinfo();
#endif

  pop_n_elems(args);
  ss=Pike_sp;

  /* TODO: If USE_DL_MALLOC is defined then this will report the
   * statistics from our bundled Doug Lea malloc, and not the
   * underlying system malloc. Ideally we should include both. */

#if defined(HAVE_MALLINFO2) || defined(HAVE_MALLINFO) || defined(USE_DL_MALLOC)

  push_static_text("num_malloc_blocks");
  push_ulongest(1 + mi.hblks);	/* 1 for the arena. */
  push_static_text("malloc_block_bytes");
#ifdef HAVE_MALLINFO2
  size = mi.arena + mi.hblkhd;
#else
  if (mi.arena < 0) {
    /* Kludge for broken Linux libc, where the fields are ints.
     *
     * 31-bit overflow, so perform an unsigned read.
     */
    size = (unsigned int)mi.arena;
  } else {
    /* On Solaris the fields are unsigned long (and may thus be 64-bit). */
    size = mi.arena;
  }
  /* NB: Kludge for glibc: hblkhd is intended for malloc overhead
   *     according to the Solaris manpages, but glibc keeps the
   *     amount of mmapped memory there, and uses the arena only
   *     for the amount from sbrk.
   *
   *     The hblkhd value on proper implementations should be
   *     small enough not to affect the total much, so no need
   *     for a special case.
   */
  if (mi.hblkhd < 0) {
    size += (unsigned int)mi.hblkhd;
  } else {
    size += mi.hblkhd;
  }
#endif
  push_ulongest(size);

  push_static_text("num_malloc");
  push_ulongest(mi.ordblks + mi.smblks);
  push_static_text("malloc_bytes");
#ifdef HAVE_MALLINFO2
  size = mi.uordblks + mi.usmblks;
#else
  if (mi.uordblks < 0) {
    size = (unsigned int)mi.uordblks;
  } else {
    size = mi.uordblks;
  }
  if (mi.smblks) {
    /* NB: Not dlmalloc where usmblks contains the max uordblks value. */
    if (mi.usmblks < 0) {
      size += (unsigned int)mi.usmblks;
    } else {
      size += mi.usmblks;
    }
  }
#endif
  push_ulongest(size);

  push_static_text("num_free_blocks");
  push_int(1);
  push_static_text("free_block_bytes");
#ifdef HAVE_MALLINFO2
  size = mi.fsmblks + mi.fordblks;
#else
  if (mi.fsmblks < 0) {
    size = (unsigned int)mi.fsmblks;
  } else {
    size = mi.fsmblks;
  }
  if (mi.fordblks < 0) {
    size += (unsigned int)mi.fordblks;
  } else {
    size += mi.fordblks;
  }
#endif
  push_ulongest(size);

  count_string_types();

#endif

#define COUNT(TYPE) do {					\
    PIKE_CONCAT3(count_memory_in_, TYPE, s)(&num, &size);	\
    push_static_text("num_" #TYPE "s");				\
    push_ulongest(num);						\
    push_static_text(#TYPE "_bytes");				\
    push_ulongest(size);					\
  } while(0)

  COUNT(array);
  COUNT(ba_mixed_frame);
  COUNT(callable);
  COUNT(callback);
  COUNT(catch_context);
  COUNT(compat_cb_box);
  COUNT(destruct_called_mark);
  COUNT(gc_rec_frame);
  COUNT(mapping);
  COUNT(mc_marker);
  COUNT(multiset);
  COUNT(node_s);
  COUNT(object);
  COUNT(pike_frame);
  COUNT(pike_list_node);
  COUNT(pike_type);
  COUNT(program);
  COUNT(string);
#ifdef PIKE_DEBUG
  COUNT(supporter_marker);
#endif

#ifdef DEBUG_MALLOC
  {
    extern void count_memory_in_memory_maps(size_t*, size_t*);
    extern void count_memory_in_memory_map_entrys(size_t*, size_t*);
    extern void count_memory_in_memlocs(size_t*, size_t*);
    extern void count_memory_in_memhdrs(size_t*, size_t*);

    COUNT(memory_map);
    COUNT(memory_map_entry);
    COUNT(memloc);
    COUNT(memhdr);
  }
#endif

  call_callback(&memory_usage_callback, NULL);

  f_aggregate_mapping(Pike_sp - ss);
}

/* Estimate the size of an svalue, not including objects.
   this is used from size_object.

   It should not include the size of the svalue itself, so the basic
   types count as 0 bytes.

   This is an estimate mainly because it is very hard to know to whom
   a certain array/mapping/multiset or string "belongs".

   The returned size will be the memory usage of the svalue divided by
   the number of references to it.
*/

unsigned int rec_size_svalue( struct svalue *s, struct mapping **m )
{
    unsigned int res = 0;
    int i;
    ptrdiff_t node_ref;
    INT32 e;
    struct svalue *x;
    struct keypair *k;

    switch( TYPEOF(*s) )
    {
        case PIKE_T_STRING:
            return count_memory_in_string(s->u.string) / s->u.string->refs;
        case PIKE_T_INT:
        case PIKE_T_OBJECT:
        case PIKE_T_FLOAT:
        case PIKE_T_FUNCTION:
        case PIKE_T_TYPE:
            return 0;
    }
    if( !m ) return 0;

    if( !*m )
        *m = allocate_mapping( 10 );
    else if( (x = low_mapping_lookup( *m, s )) )
    {
        /* Already counted. Use the old size. */
        return x->u.integer;
    }

    low_mapping_insert( *m, s, &svalue_int_one, 0 );
    switch( TYPEOF(*s) )
    {
        case PIKE_T_ARRAY:
            res = sizeof( struct array );
            for( i=0; i<s->u.array->size; i++ )
                res += sizeof(struct svalue) + rec_size_svalue( s->u.array->item+i, m );
            break;

        case PIKE_T_MULTISET:
            res = sizeof(struct multiset) + sizeof(struct multiset_data);
            node_ref = multiset_last( s->u.multiset );
            while( node_ref != -1 )
            {
                res += rec_size_svalue( get_multiset_value (s->u.multiset, node_ref), m )
                    /* each node has the index and left/right node pointers. */
                    + sizeof(struct svalue) + (sizeof(void*)*2);
                node_ref = multiset_prev( s->u.multiset, node_ref );
            }
            break;

        case PIKE_T_MAPPING:
            res = sizeof(struct mapping);
            {
                struct mapping_data *d = s->u.mapping->data;
                struct keypair *f = d->free_list;
                int data_size = sizeof( struct mapping_data );
                data_size += d->hashsize * sizeof(struct keypair *) - sizeof(struct keypair *);
                while( f )
                {
                    data_size += sizeof(struct keypair);
                    f = f->next;
                }
                NEW_MAPPING_LOOP( s->u.mapping->data  )
                {
                    data_size += rec_size_svalue( &k->ind, m );
                    data_size += rec_size_svalue( &k->val, m );
                    data_size += sizeof( struct keypair );
                }
                res += data_size / (d->hardlinks+1);
            }
            break;
    }
    res /= *s->u.refs;

    /* NB: We added s to the mapping above the switch, so we should
     *     always find something here, but...
     */
    x = low_mapping_lookup(*m, s);
    assert(x);
    assert(TYPEOF(*x) == PIKE_T_INT);
    assert(!SUBTYPEOF(*x));
    x->u.integer = res;

    return res;
}

/*! @decl int size_object(object o)
 *! @belongs Debug
 *!
 *!  Return the aproximate size of the object, in bytes.
 *!  This might not work very well for native objects
 *!
 *!
 *! The function tries to estimate the memory usage of variables
 *! belonging to the object.
 *!
 *! It will not, however, include the size of objects assigned to
 *! variables in the object.
 *!
 *!
 *! If the object has a @[lfun::_size_object()] it will be called
 *! without arguments, and the return value will be added to the final
 *! size. It is primarily intended to be used by C-objects that
 *! allocate memory that is not normally visible to pike.
 *!
 *! @seealso
 *!   @[lfun::_size_object()], @[sizeof()]
 */
static void f__size_object( INT32 UNUSED(args) )
{
    size_t sum;
    unsigned int i;
    ptrdiff_t fun;
    struct object *o;
    struct program *p;
    struct mapping *map = NULL;
    if( TYPEOF(Pike_sp[-1]) != PIKE_T_OBJECT )
        Pike_error("Expected an object as argument\n");
    o = Pike_sp[-1].u.object;

    if( !(p=o->prog) )
    {
        pop_stack();
        push_int(0);
        return;
    }
    sum = sizeof(struct object);
    sum += p->storage_needed;

    if( (fun = low_find_lfun( p, LFUN__SIZE_OBJECT)) != -1 )
    {
        apply_low( o, fun, 0 );
        if( TYPEOF(Pike_sp[-1]) == PIKE_T_INT )
            sum += Pike_sp[-1].u.integer;
        pop_stack();
    }

    Pike_sp++;
    for (i = 0; i < p->num_identifier_references; i++)
    {
        struct reference *ref = PTR_FROM_INT(p, i);
        struct identifier *id =  ID_FROM_PTR(p, ref);
        struct inherit *inh = p->inherits;
        if (!IDENTIFIER_IS_VARIABLE(id->identifier_flags) ||
            id->run_time_type == PIKE_T_GET_SET)
        {
            continue;
        }

        /* NOTE: makes the assumption that a variable saved in an
         * object has at least one reference.
         */
        low_object_index_no_free(Pike_sp-1, o, i + inh->identifier_level);
        if (REFCOUNTED_TYPE(TYPEOF(Pike_sp[-1])))
            sub_ref( Pike_sp[-1].u.dummy );
        sum += rec_size_svalue(Pike_sp-1, &map);
    }
    Pike_sp--;
    if( map ) free_mapping(map);

    pop_stack();
    push_int(sum);
}

/*! @decl type _typeof(mixed x)
 *!
 *!   Return the runtime type of @[x].
 *!
 *! @seealso
 *!   @[typeof()]
 */
PMOD_EXPORT void f__typeof(INT32 args)
{
  struct pike_type *t;

  if(!args)
    SIMPLE_WRONG_NUM_ARGS_ERROR("_typeof", 1);

  t = get_type_of_svalue(Pike_sp-args);

  if (t && (t->flags & PT_FLAG_MASK_GENERICS) &&
      (TYPEOF(Pike_sp[-args]) == PIKE_T_PROGRAM)) {
    /* NB: Binding of generics is not done by get_type_of_svalue()
     *     for programs.
     */
    struct mapping *bind =
      mkbindings(Pike_sp[-args].u.program, NULL, 1);
    struct pike_type *t2 =
      compiler_apply_bindings(t, bind);
    do_free_mapping(bind);
    free_type(t);
    t = t2;
  }

  pop_n_elems(args);
  push_type_value(t);
}

/*! @decl void replace_master(object o)
 *!
 *!   Replace the master object with @[o].
 *!
 *!   This will let you control many aspects of how Pike works, but beware that
 *!   @tt{master.pike@} may be required to fill certain functions, so it is
 *!   usually a good idea to have your master inherit the original master and
 *!   only re-define certain functions.
 *!
 *!   FIXME: Tell how to inherit the master.
 *!
 *! @seealso
 *!   @[master()]
 */
PMOD_EXPORT void f_replace_master(INT32 args)
{
  struct object *new_master;

  if(!args)
    SIMPLE_WRONG_NUM_ARGS_ERROR("replace_master", 1);
  if(TYPEOF(Pike_sp[-args]) != T_OBJECT)
    SIMPLE_ARG_TYPE_ERROR("replace_master", 1, "object");
  new_master = Pike_sp[-args].u.object;
  if(!new_master->prog)
    bad_arg_error("replace_master", args, 1, "object", Pike_sp-args,
		  "Called with destructed object.\n");

  if (SUBTYPEOF(Pike_sp[-args]))
    bad_arg_error("replace_master", args, 1, "object", Pike_sp-args,
		  "Subtyped master objects are not supported yet.\n");

  push_static_text ("is_pike_master");
  args++;
  object_set_index (new_master, 0, Pike_sp - 1, (const struct svalue *) &svalue_int_one);

  free_object(master_object);
  master_object=new_master;
  add_ref(master_object);

  free_program(master_program);
  master_program=master_object->prog;
  add_ref(master_program);

  pop_n_elems(args);
}

/*! @decl object master();
 *!
 *!   Return the current master object.
 *!
 *! @note
 *!   May return @[UNDEFINED] if no master has been loaded yet.
 *!
 *! @seealso
 *!   @[replace_master()]
 */
PMOD_EXPORT void f_master(INT32 args)
{
  struct object *o;
  pop_n_elems(args);
  o = get_master();
  if (o) ref_push_object(o);
  else push_undefined();
}

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef CPU_TIME_MIGHT_BE_THREAD_LOCAL
void thread_gethrvtime(struct thread_state * ts, INT32 args)
#else
void thread_gethrvtime(INT32 args)
#endif
{
  int nsec = 0;
  cpu_time_t time = get_cpu_time();

  if (time == (cpu_time_t) -1) {
    pop_n_elems (args);
    push_int (-1);
    return;
  }

#ifdef CPU_TIME_MIGHT_BE_THREAD_LOCAL
  if (cpu_time_is_thread_local)
    time -= ts->auto_gc_time;
  else
#endif
  {
#ifdef CPU_TIME_MIGHT_NOT_BE_THREAD_LOCAL
    time -= auto_gc_time;
#endif
  }

  nsec = args && !UNSAFE_IS_ZERO(Pike_sp-args);

  pop_n_elems(args);

  if (nsec) {
    push_int64(time);
#ifndef LONG_CPU_TIME
    push_int (1000000000 / CPU_TIME_TICKS);
    o_multiply();
#endif
  }
  else {
#if CPU_TIME_TICKS_LOW > 1000000
    push_int64(time / (CPU_TIME_TICKS / 1000000));
#else
    push_int64 (time);
    push_int (1000000 / CPU_TIME_TICKS);
    o_multiply();
#endif
  }
}

/*! @decl int gethrvtime (void|int nsec)
 *!
 *! Return the CPU time that has been consumed by this process or
 *! thread. -1 is returned if the system couldn't determine it. The
 *! time is normally returned in microseconds, but if the optional
 *! argument @[nsec] is nonzero it's returned in nanoseconds.
 *!
 *! The CPU time includes both user and system time, i.e. it's
 *! approximately the same thing you would get by adding together the
 *! "utime" and "stime" fields returned by @[System.getrusage] (but
 *! perhaps with better accuracy).
 *!
 *! It's however system dependent whether or not it's the time
 *! consumed in all threads or in the current one only;
 *! @[System.CPU_TIME_IS_THREAD_LOCAL] tells which. If both types are
 *! available then thread local time is preferred.
 *!
 *! @note
 *!   The actual accuracy on many systems is significantly less than
 *!   microseconds or nanoseconds. See @[System.CPU_TIME_RESOLUTION].
 *!
 *! @note
 *!   The garbage collector might run automatically at any time. The
 *!   time it takes is not included in the figure returned by this
 *!   function, so that normal measurements aren't randomly clobbered
 *!   by it. Explicit calls to @[gc] are still included, though.
 *!
 *! @note
 *!   The special function @[gauge] is implemented with this function.
 *!
 *! @seealso
 *!   @[System.CPU_TIME_IS_THREAD_LOCAL], @[System.CPU_TIME_RESOLUTION],
 *!   @[gauge()], @[System.getrusage()], @[gethrtime()]
 */
PMOD_EXPORT void f_gethrvtime(INT32 args)
{
#ifdef CPU_TIME_MIGHT_BE_THREAD_LOCAL
  thread_gethrvtime(Pike_interpreter.thread_state, args);
#else
  thread_gethrvtime(args);
#endif
}

/*! @decl int gethrtime (void|int nsec)
 *!
 *! Return the high resolution real time since some arbitrary event in
 *! the past. The time is normally returned in microseconds, but if
 *! the optional argument @[nsec] is nonzero it's returned in
 *! nanoseconds.
 *!
 *! It's system dependent whether or not this time is monotonic, i.e.
 *! if it's unaffected by adjustments of the calendaric clock in the
 *! system. @[System.REAL_TIME_IS_MONOTONIC] tells what it is. Pike
 *! tries to use monotonic time for this function if it's available.
 *!
 *! @note
 *!   The actual accuracy on many systems is significantly less than
 *!   microseconds or nanoseconds. See @[System.REAL_TIME_RESOLUTION].
 *!
 *! @seealso
 *!   @[System.REAL_TIME_IS_MONOTONIC], @[System.REAL_TIME_RESOLUTION],
 *!   @[time()], @[System.gettimeofday()], @[gethrvtime()],
 *!   @[Pike.implicit_gc_real_time]
 */
PMOD_EXPORT void f_gethrtime(INT32 args)
{
  int nsec = 0;
  cpu_time_t time = get_real_time();

  if (time == (cpu_time_t) -1) {
    pop_n_elems (args);
    push_int (-1);
    return;
  }

  nsec = args && !UNSAFE_IS_ZERO(Pike_sp-args);

  pop_n_elems(args);
  if (nsec) {
    push_int64(time);
#ifndef LONG_CPU_TIME
    push_int (1000000000 / CPU_TIME_TICKS);
    o_multiply();
#endif
  }
  else {
#if CPU_TIME_TICKS_LOW > 1000000
    push_int64(time / (CPU_TIME_TICKS / 1000000));
#else
    push_int64 (time);
    push_int (1000000 / CPU_TIME_TICKS);
    o_multiply();
#endif
  }
}

/*! @decl int gethrdtime(void|int nsec)
 *!
 *! Return the high resolution real time spent with threads disabled
 *! since the Pike interpreter was started. The time is normally
 *! returned in microseconds, but if the optional argument @[nsec]
 *! is nonzero it's returned in nanoseconds.
 *!
 *! @note
 *!   The actual accuracy on many systems is significantly less than
 *!   microseconds or nanoseconds. See @[System.REAL_TIME_RESOLUTION].
 *!
 *! @seealso
 *!   @[_disable_threads()], @[gethrtime()]
 */
static void f_gethrdtime(INT32 args)
{
  int nsec = args && !UNSAFE_IS_ZERO(Pike_sp-args);
  cpu_time_t time;
#ifdef PIKE_THREADS
  time = threads_disabled_acc_time;
  if (threads_disabled) {
    time += get_real_time() - threads_disabled_start;
  }
#else
  time = get_real_time();
#endif
  pop_n_elems(args);

  if (nsec) {
    push_int64(time);
#ifndef LONG_CPU_TIME
    push_int(1000000000 / CPU_TIME_TICKS);
    o_multiply();
#endif
  } else {
#if CPU_TIME_TICKS_LOW > 1000000
    push_int64(time / (CPU_TIME_TICKS / 1000000));
#else
    push_int64 (time);
    push_int (1000000 / CPU_TIME_TICKS);
    o_multiply();
#endif
  }
}

#ifdef PROFILING
/*! @decl array(int|mapping(string:array(int))) @
 *!           get_profiling_info(program prog)
 *!
 *!   Get profiling information.
 *!
 *! @returns
 *!   Returns an array with two elements.
 *!   @array
 *!   	@elem int num_clones
 *!   	  The first element is the number of times the program @[prog] has been
 *!   	  instantiated.
 *!   	@elem mapping(string:array(int)) fun_prof_info
 *!   	  The second element is mapping from function name to an
 *!   	  array with three elements.
 *!   	  @array
 *!   	    @elem int num_calls
 *!   	      The first element is the number of times the function has been
 *!   	      called.
 *!   	    @elem int total_time
 *!   	      The second element is the total time (in milliseconds) spent
 *!   	      executing this function, and any functions called from it.
 *!   	    @elem int self_time
 *!   	      The third element is the time (in milliseconds) actually spent
 *!   	      in this function so far.
 *!   	  @endarray
 *!   @endarray
 *!
 *! @note
 *!   This function is only available if the runtime was compiled with
 *!   the option @tt{--with-profiling@}.
 */
static void f_get_prof_info(INT32 args)
{
  struct program *prog = 0;
  int num_functions;
  int i;

  if (!args) {
    SIMPLE_WRONG_NUM_ARGS_ERROR("get_profiling_info", 1);
  }
  prog = program_from_svalue(Pike_sp-args);
  if(!prog)
    SIMPLE_ARG_TYPE_ERROR("get_profiling_info", 1, "program");

  /* ({ num_clones, ([ "fun_name":({ num_calls, total_time, self_time }) ]) })
   */

  pop_n_elems(args-1);
  args = 1;

  push_int(prog->num_clones);

  for(num_functions=i=0; i<(int)prog->num_identifiers; i++) {
    if (prog->identifiers[i].num_calls)
    {
      num_functions++;
      ref_push_string(prog->identifiers[i].name);

      push_int(prog->identifiers[i].num_calls);
      if (CPU_TIME_TICKS == 1000) {
	push_int64(prog->identifiers[i].total_time);
	push_int64(prog->identifiers[i].self_time);
      } else {
	push_int64(prog->identifiers[i].total_time/1000000);
	push_int64(prog->identifiers[i].self_time/1000000);
      }
      f_aggregate(3);
    }
  }
  f_aggregate_mapping(num_functions * 2);
  f_aggregate(2);

  stack_swap();
  pop_stack();
}
#endif /* PROFILING */

/*! @decl int(0..1) object_variablep(object o, string var)
 *!
 *!   Find out if an object identifier is a variable.
 *!
 *! @returns
 *!   This function returns @expr{1@} if @[var] exists as a
 *!   non-protected variable in @[o], and returns @expr{0@} (zero)
 *!   otherwise.
 *!
 *! @seealso
 *!   @[indices()], @[values()]
 */
PMOD_EXPORT void f_object_variablep(INT32 args)
{
  struct object *o;
  struct pike_string *s;
  int ret;

  get_all_args("variablep",args,"%o%n",&o, &s);

  if(!o->prog)
    bad_arg_error("variablep", args, 1, "object", Pike_sp-args,
		  "Called on destructed object.\n");

  /* FIXME: Ought to be overloadable, since `[]=() is... */

  ret=find_shared_string_identifier(s,o->prog);
  if(ret!=-1)
  {
    ret=IDENTIFIER_IS_VARIABLE(ID_FROM_INT(o->prog, ret)->identifier_flags);
  }else{
    ret=0;
  }
  pop_n_elems(args);
  push_int(!!ret);
}

/*! @module Array
 */

/*! @decl array uniq(array a)
 *!
 *!   Remove elements that are duplicates.
 *!
 *! @returns
 *!   This function returns an copy of the array @[a] with all
 *!   duplicate values removed. The order of the values is kept in the
 *!   result; it's always the first of several equal elements that is
 *!   kept.
 *!
 *! @note
 *!   Elements are compared with @[`==]. They are also hashed (see
 *!   @[lfun::__hash] for further details if the array contains
 *!   objects).
 */
PMOD_EXPORT void f_uniq_array(INT32 args)
{
  struct array *a, *b;
  struct mapping *m;
  int i, j=0,size=0;

  get_all_args(NULL, args, "%a", &a);
  if( !a->size )
  {
    push_empty_array();
    return;
  }

  push_mapping(m = allocate_mapping(a->size));
  b = allocate_array(a->size);

  for(i =0; i< a->size; i++)
  {
    mapping_insert(m, ITEM(a)+i, &svalue_int_one);
    if(m_sizeof(m) != size)
    {
      size=m_sizeof(m);
      assign_svalue_no_free(ITEM(b)+ j++, ITEM(a)+i);
    }
  }

  b->type_field = a->type_field;
  if (j != a->size) {
    /* There are zeros in the unused fields... */
    b->type_field |= BIT_INT;
    b = array_shrink(b, j);
    b->type_field = a->type_field;
  }
  push_array(b);
}

/*! @decl array(mixed) splice(array(mixed) arr1, array(mixed) arr2, @
 *!                           array(mixed) ... more_arrays)
 *!
 *!   Splice two or more arrays.
 *!
 *!   This means that the returned array has the first element in the
 *!   first given array, then the first argument in next array and so
 *!   on for all arrays. Then the second elements are added, etc.
 *!
 *! @seealso
 *!   @[`/()], @[`*()], @[`+()], @[`-()], @[everynth()]
 */
PMOD_EXPORT void f_splice(INT32 args)
{
  struct array *out;
  INT32 size=MAX_INT32;
  INT32 i,j,k;

  for(i=0;i<args;i++)
    if (TYPEOF(Pike_sp[i-args]) != T_ARRAY)
      SIMPLE_ARG_TYPE_ERROR("splice", i+1, "array");
    else
      if (Pike_sp[i-args].u.array->size < size)
	size=Pike_sp[i-args].u.array->size;

  if(!args || !size)
  {
    push_empty_array();
    return;
  }

  out=allocate_array(args * size);
  out->type_field=0;

  for(k=j=0; j<size; j++)
    for(i=-args; i<0; i++)
    {
      assign_svalue_no_free(out->item+k, Pike_sp[i].u.array->item+j);
      out->type_field |= (1<<TYPEOF(*(out->item+(k++))));
    }

  push_array(out);
  return;
}

/*! @decl array(mixed) everynth(array(mixed) a, void|int(1..) n, @
 *!                             void|int(0..) start)
 *!
 *!   Return an array with every @[n]:th element of the array @[a].
 *!
 *!   If @[n] is zero every other element will be returned.
 *!
 *! @seealso
 *!   @[splice()], @[`/()]
 */
PMOD_EXPORT void f_everynth(INT32 args)
{
  INT32 k,n=2;
  INT32 start=0;
  struct array *a;
  struct array *ina;
  TYPE_FIELD types;
  INT32 size=0;

  check_all_args(NULL, args,
		 BIT_ARRAY, BIT_INT | BIT_VOID, BIT_INT | BIT_VOID , 0);

  switch(args)
  {
    default:
    case 3:
     start=Pike_sp[2-args].u.integer;
     if(start<0)
       bad_arg_error("everynth", args, 3, "int", Pike_sp+2-args,
		     "Argument negative.\n");
     /* FALLTHRU */
    case 2:
      n=Pike_sp[1-args].u.integer;
      if(n<1)
        bad_arg_error("everynth", args, 2, "int", Pike_sp+1-args,
		      "Argument negative.\n");
     /* FALLTHRU */
    case 1:
      ina=Pike_sp[-args].u.array;
  }

  a=allocate_array(((size=ina->size)-start+n-1)/n);
  types = 0;
  for(k=0; start<size; k++, start+=n) {
    assign_svalue_no_free(ITEM(a) + k, ina->item+start);
    types |= 1 << TYPEOF(ITEM(a)[k]);
  }
  a->type_field=types;

  push_array(a);
  return;
}

/*! @decl array(array) transpose(array(array) matrix)
 *! Takes an array of equally sized arrays (essentially a matrix of size M*N)
 *! and returns the transposed (N*M) version of it, where rows and columns
 *! are exchanged for one another.
 */
PMOD_EXPORT void f_transpose(INT32 args)
{
  struct array *out;
  struct array *in;
  struct array *outinner;
  INT32 sizeininner=0,sizein=0;
  INT32 j,i;
  TYPE_FIELD type=0;

  if (args!=1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("transpose", 1);

  if (TYPEOF(Pike_sp[-args]) != T_ARRAY)
    SIMPLE_ARG_TYPE_ERROR("transpose", 1, "array(array)");

  in=Pike_sp[-args].u.array;
  sizein=in->size;

  if(!sizein)
  {
    push_empty_array();
    return;
  }

  if( (in->type_field != BIT_ARRAY) &&
      (array_fix_type_field(in) != BIT_ARRAY) )
    SIMPLE_ARG_TYPE_ERROR("transpose", 1, "array(array)");

  sizeininner=in->item->u.array->size;

  for(i=1 ; i<sizein; i++)
    if (sizeininner!=(in->item+i)->u.array->size)
      Pike_error("The array given as argument 1 to transpose must contain arrays of the same size.\n");

  out=allocate_array(sizeininner);

  for(i=0; i<sizein; i++)
    type|=in->item[i].u.array->type_field;

  for(j=0; j<sizeininner; j++)
  {
    struct svalue * ett;
    struct svalue * tva;

    outinner=allocate_array(sizein);
    ett=outinner->item;
    tva=in->item;
    for(i=0; i<sizein; i++)
      assign_svalue_no_free(ett+i, tva[i].u.array->item+j);

    outinner->type_field=type;
    SET_SVAL(out->item[j], T_ARRAY, 0, array, outinner);
  }

  out->type_field=BIT_ARRAY;
  push_array(out);
  return;
}

/*! @endmodule
 */

/*! @decl mixed map(mixed arr, void|mixed fun, mixed ... extra)
 *!
 *!   Applies @[fun] to the elements in @[arr] and collects the results.
 *!
 *!   @[arr] is treated as a set of elements, as follows:
 *!
 *!   @dl
 *!     @item array
 *!     @item multiset
 *!     @item string
 *!       @[fun] is applied in order to each element. The results are
 *!       collected, also in order, to a value of the same type as
 *!       @[arr], which is returned.
 *!
 *!     @item mapping
 *!       @[fun] is applied to the values, and each result is assigned
 *!       to the same index in a new mapping, which is returned.
 *!
 *!     @item program
 *!       The program is treated as a mapping containing the
 *!       identifiers that are indexable from it and their values.
 *!
 *!     @item object
 *!       If there is a @[lfun::cast] method in the object, it's
 *!       called to try to cast the object to an array, a mapping, or
 *!       a multiset, in that order, which is then handled as
 *!       described above.
 *!   @enddl
 *!
 *!   @[fun] is applied in different ways depending on its type:
 *!
 *!   @dl
 *!     @item function
 *!       @[fun] is called for each element. It gets the current
 *!       element as the first argument and @[extra] as the rest. The
 *!       result of the call is collected.
 *!
 *!     @item object
 *!       @[fun] is used as a function like above, i.e. the
 *!       @[lfun::`()] method in it is called.
 *!
 *!     @item array
 *!       Each element of the @[fun] array will be called for each
 *!       element of @[arr].
 *!
 *!     @item multiset
 *!     @item mapping
 *!       @[fun] is indexed with each element. The result of that is
 *!       collected.
 *!
 *!     @item "zero or left out"
 *!       Each element that is callable is called with @[extra] as
 *!       arguments. The result of the calls are collected. Elements
 *!       that aren't callable gets zero as result.
 *!
 *!     @item string
 *!       Each element is indexed with the given string. If the result
 *!       of that is zero then a zero is collected, otherwise it's
 *!       called with @[extra] as arguments and the result of that
 *!       call is collected.
 *!
 *!       This is typically used when @[arr] is a collection of
 *!       objects, and @[fun] is the name of some function in them.
 *!   @enddl
 *!
 *! @note
 *!   The function is never destructive on @[arr].
 *!
 *! @seealso
 *!   @[filter()], @[enumerate()], @[foreach()]
 */
PMOD_EXPORT void f_map(INT32 args)
{
   struct svalue *mysp;
   struct array *a,*d;
   int splice,i,n;
   TYPE_FIELD types;

   if (args<1)
      SIMPLE_WRONG_NUM_ARGS_ERROR("map", 1);
   else if (args<2)
      { push_int(0); args++; }

   switch (TYPEOF(Pike_sp[-args]))
   {
      case T_ARRAY:
	 break;

      case T_MAPPING:
      case T_PROGRAM:
      case T_FUNCTION:
	 /* mapping ret =
	       mkmapping(indices(arr),
	                 map(values(arr),fun,@extra)); */
	 f_aggregate(args-2);
	 mysp=Pike_sp;
	 splice=mysp[-1].u.array->size;

	 push_svalue(mysp-3); /* arr */
	 f_values(1);
	 push_svalue(mysp-2); /* fun */
	 move_svalue (Pike_sp, mysp-1); /* extra */
	 mark_free_svalue (mysp-1);
	 dmalloc_touch_svalue(Pike_sp);
	 push_array_items(Pike_sp->u.array);
	 f_map(splice+2);     /* ... arr fun extra -> ... retval */
	 stack_pop_2_elems_keep_top(); /* arr fun extra ret -> arr retval */
	 stack_swap();        /* retval arr */
	 f_indices(1);        /* retval retind */
	 stack_swap();        /* retind retval */
	 f_mkmapping(2);      /* ret :-) */
	 return;

      case T_MULTISET:
	 /* multiset ret =
	       (multiset)(map(indices(arr),fun,@extra)); */
	 push_svalue(Pike_sp-args);      /* take indices from arr */
	 free_svalue(Pike_sp-args-1);    /* move it to top of stack */
	 mark_free_svalue (Pike_sp-args-1);
	 f_indices(1);              /* call f_indices */
	 Pike_sp--;
	 dmalloc_touch_svalue(Pike_sp);
	 Pike_sp[-args]=Pike_sp[0];           /* move it back */
	 f_map(args);

	 push_multiset (mkmultiset (Pike_sp[-1].u.array));
	 free_array (Pike_sp[-2].u.array);
	 dmalloc_touch_svalue(Pike_sp-1);
	 Pike_sp[-2] = Pike_sp[-1];
	 Pike_sp--;
	 return;

      case T_STRING:
	 /* multiset ret =
	       (string)(map((array)arr,fun,@extra)); */
	 push_svalue(Pike_sp-args);      /* take indices from arr */
	 free_svalue(Pike_sp-args-1);    /* move it to top of stack */
	 mark_free_svalue (Pike_sp-args-1);
	 o_cast(NULL,T_ARRAY);      /* cast the string to an array */
	 Pike_sp--;
	 dmalloc_touch_svalue(Pike_sp);
	 Pike_sp[-args]=Pike_sp[0];           /* move it back */
	 f_map(args);
	 o_cast(NULL,T_STRING);     /* cast the array to a string */
	 return;

      case T_OBJECT:
	 /* if arr->cast :
               try map((array)arr,fun,@extra);
               try map((mapping)arr,fun,@extra);
               try map((multiset)arr,fun,@extra); */

	 mysp=Pike_sp+3-args;

	 {
           struct object *o = mysp[-3].u.object;
           INT16 osub = SUBTYPEOF(mysp[-3]);
           int f = FIND_LFUN(o->prog->inherits[osub].prog,
                             LFUN_CAST);

           if( f!=-1 )
           {

             ref_push_string(literal_array_string);
             apply_low(o, f, 1);
             if (TYPEOF(Pike_sp[-1]) == T_ARRAY)
             {
	       free_svalue(mysp-3);
	       mysp[-3]=*(--Pike_sp);
	       dmalloc_touch_svalue(Pike_sp);
	       f_map(args);
	       return;
             }
             pop_stack();

             ref_push_string(literal_mapping_string);
             apply_low(o, f, 1);
             if (TYPEOF(Pike_sp[-1]) == T_MAPPING)
             {
	       free_svalue(mysp-3);
	       mysp[-3]=*(--Pike_sp);
	       dmalloc_touch_svalue(Pike_sp);
	       f_map(args);
	       return;
             }
             pop_stack();

             ref_push_string(literal_multiset_string);
             apply_low(o, f, 1);
             if (TYPEOF(Pike_sp[-1]) == T_MULTISET)
             {
	       free_svalue(mysp-3);
	       mysp[-3]=*(--Pike_sp);
	       dmalloc_touch_svalue(Pike_sp);
	       f_map(args);
	       return;
             }
             pop_stack();
           }
	 }

	 SIMPLE_ARG_TYPE_ERROR("map",1,
                               "object that works in map");

      default:
	 SIMPLE_ARG_TYPE_ERROR("map",1,
                               "array|mapping|program|function|"
                               "multiset|string|object");
   }

   if (UNSAFE_IS_ZERO (Pike_sp-args+1)) {
     free_svalue (Pike_sp-args+1);
     move_svalue (Pike_sp-args+1, Pike_sp-args);
     mark_free_svalue (Pike_sp-args);
     mega_apply (APPLY_STACK, args-1, 0, 0);
     stack_pop_keep_top();
     return;
   }

   f_aggregate(args-2);
   mysp=Pike_sp;
   splice=mysp[-1].u.array->size;

   a=mysp[-3].u.array;
   n=a->size;

   switch (TYPEOF(mysp[-2]))
   {
      case T_FUNCTION:
      case T_PROGRAM:
      case T_OBJECT:
      case T_ARRAY:
	 /* ret[i]=fun(arr[i],@extra); */
         push_array(d=allocate_array(n));
	 d=Pike_sp[-1].u.array;
	 types = 0;

	 if(TYPEOF(mysp[-2]) == T_FUNCTION &&
	    SUBTYPEOF(mysp[-2]) == FUNCTION_BUILTIN)
	 {
	   c_fun fun=mysp[-2].u.efun->function;
	   struct svalue *spbase=Pike_sp;

	   if(splice)
	   {
	     for (i=0; i<n; i++)
	     {
	       push_svalue(a->item+i);
	       add_ref_svalue(mysp-1);
	       push_array_items(mysp[-1].u.array);
	       (* fun)(1+splice);
	       if(Pike_sp>spbase)
	       {
		 stack_pop_to_no_free (ITEM(d) + i);
		 types |= 1 << TYPEOF(ITEM(d)[i]);
		 pop_n_elems(Pike_sp-spbase);
	       }
	       else
		 types |= BIT_INT;
	     }
	   }else{
	     for (i=0; i<n; i++)
	     {
	       push_svalue(ITEM(a)+i);
	       (* fun)(1);
	       if(Pike_sp>spbase)
	       {
		 stack_pop_to_no_free (ITEM(d) + i);
		 types |= 1 << TYPEOF(ITEM(d)[i]);
		 pop_n_elems(Pike_sp-spbase);
	       }
	       else
		 types |= BIT_INT;
	     }
	   }
	 }else{
	   for (i=0; i<n; i++)
	   {
	     push_svalue(ITEM(a)+i);
	     if (splice)
	     {
	       add_ref_svalue(mysp-1);
	       push_array_items(mysp[-1].u.array);
	       apply_svalue(mysp-2,1+splice);
	     }
	     else
	     {
	       apply_svalue(mysp-2,1);
	     }
	     stack_pop_to_no_free (ITEM(d) + i);
	     types |= 1 << TYPEOF(ITEM(d)[i]);
	   }
	 }
	 d->type_field = types;
	 stack_pop_n_elems_keep_top(3); /* fun arr extra d -> d */
	 return;

      case T_MAPPING:
      case T_MULTISET:
	 /* ret[i]=fun[arr[i]]; */
	 pop_stack();
	 stack_swap();
	 f_rows(2);
	 return;

      case T_STRING:
	 /* ret[i]=arr[i][fun](@extra); */
         push_array(d=allocate_array(n));
	 types = 0;
	 for (i=0; i<n; i++)
	 {
	    push_svalue(ITEM(a)+i);
	    push_svalue(mysp-2);
	    f_arrow(2);
	    if(UNSAFE_IS_ZERO(Pike_sp-1))
	    {
	      types |= BIT_INT;
	      pop_stack();
	      continue;
	    }
	    add_ref_svalue(mysp-1);
	    push_array_items(mysp[-1].u.array);
	    f_call_function(splice+1);
	    stack_pop_to_no_free (ITEM(d) + i);
	    types |= 1 << TYPEOF(ITEM(d)[i]);
	 }
	 d->type_field = types;
	 stack_pop_n_elems_keep_top(3); /* fun arr extra d -> d */
	 return;

      default:
	 SIMPLE_ARG_TYPE_ERROR("map",2,
                               "function|program|object|"
                               "string|int(0..0)|multiset");
   }
}

/*! @decl mixed filter(mixed arr, void|mixed fun, mixed ...extra)
 *!
 *!   Filters the elements in @[arr] through @[fun].
 *!
 *!   @[arr] is treated as a set of elements to be filtered, as
 *!   follows:
 *!
 *!   @dl
 *!     @item array
 *!     @item multiset
 *!     @item string
 *!       Each element is filtered with @[fun]. The return value is of
 *!       the same type as @[arr] and it contains the elements that
 *!       @[fun] accepted. @[fun] is applied in order to each element,
 *!       and that order is retained between the kept elements.
 *!
 *!       If @[fun] is an array, it should have the same length as
 *!       @[arr]. In this case, the elements in @[arr] are kept where
 *!       the corresponding positions in @[fun] are nonzero. Otherwise
 *!       @[fun] is used as described below.
 *!
 *!     @item mapping
 *!       The values are filtered with @[fun], and the index/value
 *!       pairs it accepts are kept in the returned mapping.
 *!
 *!     @item program
 *!       The program is treated as a mapping containing the
 *!       identifiers that are indexable from it and their values.
 *!
 *!     @item object
 *!       If there is a @[lfun::cast] method in the object, it's called
 *!       to try to cast the object to an array, a mapping, or a
 *!       multiset, in that order, which is then filtered as described
 *!       above.
 *!   @enddl
 *!
 *!   Unless something else is mentioned above, @[fun] is used as
 *!   filter like this:
 *!
 *!   @dl
 *!     @item function
 *!       @[fun] is called for each element. It gets the current
 *!       element as the first argument and @[extra] as the rest. The
 *!       element is kept if it returns true, otherwise it's filtered
 *!       out.
 *!
 *!     @item object
 *!       The object is used as a function like above, i.e. the
 *!       @[lfun::`()] method in it is called.
 *!
 *!     @item multiset
 *!     @item mapping
 *!       @[fun] is indexed with each element. The element is kept if
 *!       the result is nonzero, otherwise it's filtered out.
 *!
 *!     @item "zero or left out"
 *!       Each element that is callable is called with @[extra] as
 *!       arguments. The element is kept if the result of the call is
 *!       nonzero, otherwise it's filtered out. Elements that aren't
 *!       callable are also filtered out.
 *!
 *!     @item string
 *!       Each element is indexed with the given string. If the result
 *!       of that is zero then the element is filtered out, otherwise
 *!       the result is called with @[extra] as arguments. The element
 *!       is kept if the return value is nonzero, otherwise it's
 *!       filtered out.
 *!
 *!       This is typically used when @[arr] is a collection of
 *!       objects, and @[fun] is the name of some predicate function
 *!       in them.
 *!   @enddl
 *!
 *! @note
 *!   The function is never destructive on @[arr].
 *!
 *! @seealso
 *!   @[map()], @[foreach()]
 */
PMOD_EXPORT void f_filter(INT32 args)
{
   int n,i,m,k;
   struct array *a,*y,*f;
   struct svalue *mysp;

   if (args<1)
      SIMPLE_WRONG_NUM_ARGS_ERROR("filter", 1);

   switch (TYPEOF(Pike_sp[-args]))
   {
      case T_ARRAY:
	 if (args >= 2 && TYPEOF(Pike_sp[1-args]) == T_ARRAY) {
	   if (Pike_sp[1-args].u.array->size != Pike_sp[-args].u.array->size)
	     SIMPLE_ARG_TYPE_ERROR("filter", 2, "array of same size as the first");
	   pop_n_elems(args-2);
	 }
	 else {
           /* filter(X, mixed ... extra) ==> filter(X, map(X, @extra)) */
	   memmove(Pike_sp-args+1,Pike_sp-args,args*sizeof(*Pike_sp));
	   dmalloc_touch_svalue(Pike_sp);
	   Pike_sp++;
	   add_ref_svalue(Pike_sp-args);
	   f_map(args);
	 }

	 f=Pike_sp[-1].u.array;
	 a=Pike_sp[-2].u.array;
	 n=a->size;
         /* FIXME: Ought to use BEGIN_AGGREGATE_ARRAY() et al. */
	 for (k=m=i=0; i<n; i++)
	    if (!UNSAFE_IS_ZERO(f->item+i))
	    {
	       push_svalue(a->item+i);
	       if (m++>32)
	       {
		  f_aggregate(m);
		  m=0;
		  if (++k>32) {
		    f_add(k);
		    k=1;
		  }
	       }
	    }
	 if (m || !k) {
	   f_aggregate(m);
	   k++;
	 }
	 if (k > 1) f_add(k);
	 stack_pop_2_elems_keep_top();
	 return;

      case T_MAPPING:
      case T_PROGRAM:
      case T_FUNCTION:
	 /* mapping ret =
	       mkmapping(indices(arr),
	                 map(values(arr),fun,@extra)); */
	 memmove(Pike_sp-args+2,Pike_sp-args,args*sizeof(*Pike_sp));
	 Pike_sp+=2;
	 mark_free_svalue (Pike_sp-args-2);
	 mark_free_svalue (Pike_sp-args-1);

	 push_svalue(Pike_sp-args);
	 f_indices(1);
	 dmalloc_touch_svalue(Pike_sp-1);
	 Pike_sp--;
	 Pike_sp[-args-2]=*Pike_sp;
	 dmalloc_touch_svalue(Pike_sp);
	 push_svalue(Pike_sp-args);
	 f_values(1);
	 Pike_sp--;
	 Pike_sp[-args-1]=*Pike_sp;
	 dmalloc_touch_svalue(Pike_sp);

	 assign_svalue(Pike_sp-args,Pike_sp-args-1); /* loop values only */
	 f_map(args);

	 y=Pike_sp[-3].u.array;
	 a=Pike_sp[-2].u.array;
	 f=Pike_sp[-1].u.array;
	 n=a->size;

	 for (m=i=0; i<n; i++)
	    if (!UNSAFE_IS_ZERO(f->item+i)) m++;

	 push_mapping(allocate_mapping(MAXIMUM(m,4)));

	 for (i=0; i<n; i++)
	    if (!UNSAFE_IS_ZERO(f->item+i))
	       mapping_insert(Pike_sp[-1].u.mapping,y->item+i,a->item+i);

	 stack_pop_n_elems_keep_top(3);
	 return;

      case T_MULTISET:
	 push_svalue(Pike_sp-args);      /* take indices from arr */
	 free_svalue(Pike_sp-args-1);    /* move it to top of stack */
	 mark_free_svalue (Pike_sp-args-1);
	 f_indices(1);              /* call f_indices */
	 Pike_sp--;
	 dmalloc_touch_svalue(Pike_sp);
	 Pike_sp[-args]=Pike_sp[0];           /* move it back */
	 f_filter(args);

	 push_multiset (mkmultiset (Pike_sp[-1].u.array));
	 free_array (Pike_sp[-2].u.array);
	 Pike_sp[-2] = Pike_sp[-1];
	 dmalloc_touch_svalue(Pike_sp-1);
	 Pike_sp--;
	 return;

      case T_STRING:
	 push_svalue(Pike_sp-args);      /* take indices from arr */
	 free_svalue(Pike_sp-args-1);    /* move it to top of stack */
	 mark_free_svalue (Pike_sp-args-1);
	 o_cast(NULL,T_ARRAY);      /* cast the string to an array */
	 Pike_sp--;
	 dmalloc_touch_svalue(Pike_sp);
	 Pike_sp[-args]=Pike_sp[0];           /* move it back */
	 f_filter(args);
	 o_cast(NULL,T_STRING);     /* cast the array to a string */
	 return;

      case T_OBJECT:
	 mysp=Pike_sp+3-args;

	 {
            struct object *o = mysp[-3].u.object;
            int f = FIND_LFUN(o->prog->inherits[SUBTYPEOF(mysp[-3])].prog,
                              LFUN_CAST);

            if( f!=-1 )
            {
              ref_push_string(literal_array_string);
              apply_low(o, f, 1);
              if (TYPEOF(Pike_sp[-1]) == T_ARRAY)
              {
                free_svalue(mysp-3);
                mysp[-3]=*(--Pike_sp);
                dmalloc_touch_svalue(Pike_sp);
                f_filter(args);
                return;
              }
              pop_stack();

              ref_push_string(literal_mapping_string);
              apply_low(o, f, 1);
              if (TYPEOF(Pike_sp[-1]) == T_MAPPING)
              {
                free_svalue(mysp-3);
                mysp[-3]=*(--Pike_sp);
                dmalloc_touch_svalue(Pike_sp);
                f_filter(args);
                return;
              }
              pop_stack();

              ref_push_string(literal_multiset_string);
              apply_low(o, f, 1);
              if (TYPEOF(Pike_sp[-1]) == T_MULTISET)
              {
                free_svalue(mysp-3);
                mysp[-3]=*(--Pike_sp);
                dmalloc_touch_svalue(Pike_sp);
                f_filter(args);
                return;
              }
              pop_stack();
            }
	 }

	 SIMPLE_ARG_TYPE_ERROR("filter",1,
                               "...|object that can be cast to array, multiset or mapping");

      default:
	 SIMPLE_ARG_TYPE_ERROR("filter",1,
                               "array|mapping|program|function|"
                               "multiset|string|object");
   }
}

/* map() and filter() inherit sideeffects from their
 * second argument.
 */
static node *fix_map_node_info(node *n)
{
  int argno = 1; /* Note: argument 2 has argno 1. */
  node **cb_;
  /* Assume worst case. */
  int node_info = OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND;

  if (count_args(CDR(n)) == 1) {
    /* Special case for the single argument case. */
    argno = 0;
  }

  for (argno; (cb_ = my_get_arg(&_CDR(n), argno)); argno++) {
    node *cb = *cb_;

    if ((cb->token == F_CONSTANT) &&
	(TYPEOF(cb->u.sval) == T_FUNCTION) &&
	(SUBTYPEOF(cb->u.sval) == FUNCTION_BUILTIN)) {
      if (cb->u.sval.u.efun->optimize == fix_map_node_info) {
	/* map() or filter(). */
	continue;
      }
      node_info &= cb->u.sval.u.efun->flags;
    }
    /* FIXME: Type-checking? */
    break;
  }

  if (!cb_) {
    yyerror("Too few arguments to map() or filter()!\n");
  }

  n->node_info |= node_info;
  n->tree_info |= node_info;

  return 0;	/* continue optimization */
}

/*! @decl array(int) enumerate(int n)
 *! @decl array enumerate(int n, void|mixed step, void|mixed start, @
 *!                       void|function operator)
 *!
 *!   Create an array with an enumeration, useful for initializing arrays
 *!   or as first argument to @[map()] or @[foreach()].
 *!
 *!   The defaults are: @[step] = 1, @[start] = 0, @[operator] = @[`+]
 *!
 *!   @section Advanced use
 *!   	The resulting array is calculated like this:
 *! @code
 *! array enumerate(int n, mixed step, mixed start, function operator)
 *! {
 *!   array res = allocate(n);
 *!   for (int i=0; i < n; i++)
 *!   {
 *!     res[i] = start;
 *!     start = operator(start, step);
 *!   }
 *!   return res;
 *! }
 *! @endcode
 *!   @endsection
 *!
 *! @seealso
 *!   @[map()], @[foreach()]
 */
void f_enumerate(INT32 args)
{
   struct array *d;
   int i;
   INT_TYPE n;

   if (args<1)
      SIMPLE_WRONG_NUM_ARGS_ERROR("enumerate", 1);
   if (args<2)
   {
      push_int(1);
      args++;
   }
   if (args<3)
   {
      push_int(0);
      args++;
   }

   if (args<=3 &&
       (TYPEOF(Pike_sp[1-args]) == T_INT &&
	TYPEOF(Pike_sp[2-args]) == T_INT))
   {
      INT_TYPE step,start;

      get_all_args("enumerate", args, "%+%i%i", &n, &step, &start);

      {
        INT_TYPE tmp;

        /* this checks if
         *      (n - 1) * step + start
         * will overflow. if it does, we continue with the slow path. If it does not,
         * adding step to start repeatedly will not overflow below. This check has
         * false positives, but is much simpler to check than e.g. doing one check
         * for every iteration
         */
        if (DO_INT_TYPE_MUL_OVERFLOW(n-1, step, &tmp) || INT_TYPE_ADD_OVERFLOW(tmp, start))
          goto slow_path;
      }

      pop_n_elems(args);
      push_array(d=allocate_array(n));
      for (i=0; i<n; i++)
      {
	 ITEM(d)[i].u.integer=start;
	 start+=step;
      }
      d->type_field = BIT_INT;
   }
   else if (args<=3 &&
	    ((TYPEOF(Pike_sp[1-args]) == T_INT ||
	      TYPEOF(Pike_sp[1-args]) == T_FLOAT) &&
	     (TYPEOF(Pike_sp[2-args]) == T_INT ||
	      TYPEOF(Pike_sp[2-args]) == T_FLOAT) ) )
   {
      FLOAT_TYPE step, start;

      get_all_args("enumerate", args, "%+%F%F", &n, &step, &start);
      pop_n_elems(args);
      push_array(d=allocate_array(n));
      for (i=0; i<n; i++)
      {
	 SET_SVAL(d->item[i], T_FLOAT, 0, float_number, start);
	 start+=step;
      }
      d->type_field = BIT_FLOAT;
   }
   else
   {
      TYPE_FIELD types;
slow_path:
      types = 0;
      get_all_args("enumerate", args, "%+", &n);
      if (args>4) pop_n_elems(args-4);
      push_array(d=allocate_array(n));
      if (args<4)
      {
	 push_svalue(Pike_sp-2); /* start */
	 for (i=0; i<n; i++)
	 {
	    assign_svalue_no_free(ITEM(d)+i,Pike_sp-1);
	    types |= 1 << TYPEOF(ITEM(d)[i]);
	    if (i<n-1)
	    {
	       push_svalue(Pike_sp-4); /* step */
	       f_add(2);
	    }
	 }
      }
      else
      {
	 push_svalue(Pike_sp-3); /* start */
	 for (i=0; i<n; i++)
	 {
	    assign_svalue_no_free(ITEM(d)+i,Pike_sp-1);
	    types |= 1 << TYPEOF(ITEM(d)[i]);
	    if (i<n-1)
	    {
	       push_svalue(Pike_sp-3); /* function */
	       stack_swap();
	       push_svalue(Pike_sp-6); /* step */
	       f_call_function(3);
	    }
	 }
      }
      d->type_field = types;
      pop_stack();
      stack_pop_n_elems_keep_top(args);
   }
}

/*! @module Program
 */


/*! @decl string|zero defined(program x, string identifier)
 *!
 *!   Returns a string with filename and linenumber where @[identifier]
 *!   in @[x] was defined.
 *!
 *!   Returns @expr{0@} (zero) when no line can be found, e.g. for
 *!   builtin functions.
 *!
 *!   If @[identifier] can not be found in @[x] this function returns
 *!   where the program is defined.
 */
PMOD_EXPORT void f_program_identifier_defined(INT32 args)
{
  struct program *p;
  struct pike_string *ident;
  struct program *id_prog, *p2;
  struct identifier *id;
  INT_TYPE line;
  INT_TYPE offset;
  struct pike_string *file = NULL;

  if( !(p = program_from_svalue(Pike_sp-args)) )
    SIMPLE_ARG_TYPE_ERROR("program_identifier_defined", 1, "program");

  if( TYPEOF(Pike_sp[1-args]) != PIKE_T_STRING )
    SIMPLE_ARG_TYPE_ERROR("program_identifier_defined", 2, "string");
  else
      ident = Pike_sp[-args+1].u.string;

  if( (offset = find_shared_string_identifier( ident, p )) == -1 )
  {
      INT_TYPE line;
      struct pike_string *tmp = low_get_program_line(p, &line);

      if (tmp)
      {
          push_string(tmp);
          if(line >= 1)
          {
              push_static_text(":");
              push_int(line);
              f_add(3);
          }
      }
      else
          push_int(0);
      return;
  }

  id = ID_FROM_INT(p, offset);
  id_prog = PROG_FROM_INT (p, offset);

  if(IDENTIFIER_IS_PIKE_FUNCTION( id->identifier_flags ) &&
     id->func.offset != -1)
    file = low_get_line(id_prog->program + id->func.offset, id_prog,
			&line, NULL);
  else if (IDENTIFIER_IS_CONSTANT (id->identifier_flags) &&
           id->func.const_info.offset >= 0 &&
           (p2 = program_from_svalue (&id_prog->constants[id->func.const_info.offset].sval)))
      file = low_get_program_line (p2, &line);
  else
      /* The program line is better than nothing for C functions. */
      file = low_get_program_line (p, &line);

  if (file)
  {
      if (line) {
          push_string(file);
          push_static_text(":");
          push_int(line);
          f_add(3);
      }
      else
          push_string (file);
      return;
  }

  push_int(0);
}

/*! @decl array(program) inherit_list(program p)
 *!
 *!   Returns an array with the programs that @[p] has inherited.
 */
PMOD_EXPORT void f_inherit_list(INT32 args)
{
  struct program *p;
  struct svalue *arg;
  struct object *par;
  int parid,e,q=0;

  get_all_args(NULL, args, "%*", &arg);
  if(TYPEOF(Pike_sp[-args]) == T_OBJECT)
    f_object_program(1);

  p=program_from_svalue(arg);
  if(!p)
    SIMPLE_ARG_TYPE_ERROR("inherit_list", 1, "program");

  if(TYPEOF(*arg) == T_FUNCTION)
  {
    par=arg->u.object;
    parid = SUBTYPEOF(*arg);
  }else{
    par=0;
    parid=-1;
  }

  check_stack(p->num_inherits);
  for(e=0;e<p->num_inherits;e++)
  {
    struct inherit *in=p->inherits+e;

    if(in->inherit_level==1)
    {
      if(in->prog->flags & PROGRAM_USES_PARENT)
      {
	switch(in->parent_offset)
	{
	  default:
	  {
	    struct external_variable_context tmp;
	    if(!par)
	    {
	      ref_push_program(in->prog);
	    }else{
	      tmp.o=par;
	      tmp.parent_identifier=parid;
	      tmp.inherit=INHERIT_FROM_INT(par->prog,parid);

	      find_external_context(&tmp, in->parent_offset-1);
	      ref_push_function(tmp.o,
				in->parent_identifier +
				tmp.inherit->identifier_level);
	    }
	  }
	  break;

	  case INHERIT_PARENT:
	    ref_push_function(in->parent, in->parent_identifier);
	    break;

	  case OBJECT_PARENT:
	    if(par)
	    {
	      ref_push_function(par, parid);
	    }else{
	      ref_push_program(in->prog);
	    }
	    break;
	}
      }else{
	ref_push_program(in->prog);
      }
      q++;
    }
  }
  f_aggregate(q);
}

/*! @endmodule
 */

/*! @module Function
 */

/*! @decl string defined(function fun)
 *!
 *!   Returns a string with filename and linenumber where @[fun]
 *!   was defined.
 *!
 *!   Returns @expr{0@} (zero) when no line can be found, e.g. for
 *!   builtin functions and functions in destructed objects.
 */
PMOD_EXPORT void f_function_defined(INT32 args)
{
  check_all_args(NULL, args, BIT_FUNCTION, 0);

  if(SUBTYPEOF(Pike_sp[-args]) != FUNCTION_BUILTIN &&
     Pike_sp[-args].u.object->prog)
  {
    struct program *p = Pike_sp[-args].u.object->prog;
    struct program *id_prog, *p2;
    int func = SUBTYPEOF(Pike_sp[-args]);
    struct identifier *id;
    INT_TYPE line;
    struct pike_string *file = NULL;

    if (p == pike_trampoline_program) {
      struct pike_trampoline *t =
	(struct pike_trampoline *) Pike_sp[-args].u.object->storage;
      if (t->frame->current_object->prog) {
	p = t->frame->current_object->prog;
	func = t->func;
      }
    }

    id=ID_FROM_INT(p, func);
    id_prog = PROG_FROM_INT (p, func);

    if(IDENTIFIER_IS_PIKE_FUNCTION( id->identifier_flags ) &&
       id->func.offset != -1)
      file = low_get_line(id_prog->program + id->func.offset, id_prog,
			  &line, NULL);
    else if (IDENTIFIER_IS_CONSTANT (id->identifier_flags) &&
	     id->func.const_info.offset >= 0 &&
	     (p2 = program_from_svalue (&id_prog->constants[id->func.const_info.offset].sval)))
      file = low_get_program_line (p2, &line);
    else
      /* The program line is better than nothing for C functions. */
      file = low_get_program_line (p, &line);

    if (file)
    {
      if (line) {
	push_string(file);
	push_static_text(":");
	push_int(line);
	f_add(3);
      }
      else
	push_string (file);
      return;
    }
  }

  push_int(0);
}

/*! @endmodule Function
 */

/* FIXME: Document catch and gauge. */

void init_builtin_efuns(void)
{
  struct program *pike___master_program;

  ADD_EFUN("gethrvtime",f_gethrvtime,
	   tFunc(tOr(tInt,tVoid),tInt), OPT_EXTERNAL_DEPEND);
  ADD_EFUN("gethrtime", f_gethrtime,
	   tFunc(tOr(tInt,tVoid),tInt), OPT_EXTERNAL_DEPEND);
  ADD_EFUN("gethrdtime", f_gethrdtime,
	   tFunc(tOr(tInt,tVoid),tInt), OPT_EXTERNAL_DEPEND);

#ifdef PROFILING
  ADD_EFUN("get_profiling_info", f_get_prof_info,
	   tFunc(tPrg(tObj),tArray), OPT_EXTERNAL_DEPEND);
#endif /* PROFILING */

  ADD_EFUN("_typeof", f__typeof, tFunc(tSetvar(0, tMix), tType(tVar(0))), 0);

  /*! @module Builtin
   */

  /*! @class __master
   *!
   *! Used to prototype the master object.
   */
  start_new_program();
  /*! @decl void _main(array(string) argv, array(string) env) */
  ADD_PROTOTYPE("_main", tFunc(tArr(tStr) tArr(tStr),tVoid), 0);
  /*! @decl object cast_to_object(string oname, string current_file) */
  ADD_PROTOTYPE("cast_to_object", tFunc(tStr tStr tOr(tVoid, tObj), tObj), 0);
  /*! @decl program cast_to_program(string pname, string current_file, object|void handler) */
  ADD_PROTOTYPE("cast_to_program", tFunc(tStr tStr tOr(tVoid, tObj), tPrg(tObj)), 0);
  /*! @decl void compile_error(string file, int line, string err) */
  ADD_PROTOTYPE("compile_error", tFunc(tStr tInt tStr, tVoid), 0);
  /*! @decl void compile_warning(string file, int line, string warn) */
  ADD_PROTOTYPE("compile_warning", tFunc(tStr tInt tStr, tVoid), 0);
  /*! @decl string decode_charset(string data, string charset) */
  ADD_PROTOTYPE("decode_charset", tFunc(tStr tStr, tStr), 0);
  /*! @decl string describe_backtrace(object|array trace, int|void line_width) */
  ADD_PROTOTYPE("describe_backtrace", tFunc(tOr(tObj, tArr(tMix)) tOr(tVoid, tInt), tStr), 0);
  /*! @decl void handle_error(array|object trace) */
  ADD_PROTOTYPE("handle_error", tFunc(tOr(tArr(tMix),tObj), tVoid), 0);
  /*! @decl mixed handle_import(string what, string|void current_file, object|void handler) */
  ADD_PROTOTYPE("handle_import",
		tFunc(tStr tOr(tStr, tVoid) tOr(tObj, tVoid), tMix), 0);
  /*! @decl string handle_include(string f, string current_file, int(0..1) local_include) */
  ADD_PROTOTYPE("handle_include", tFunc(tStr tStr tInt01, tStr), 0);
  /*! @decl program handle_inherit(string pname, string current_file, object|void handler) */
  ADD_PROTOTYPE("handle_inherit", tFunc(tStr tStr tOr(tObj, tVoid), tPrg(tObj)), 0);
  /*! @decl void write(string fmt, mixed ... args) */
  ADD_PROTOTYPE("write", tFuncV(tStr, tOr(tVoid,tMix), tInt), OPT_SIDE_EFFECT);
  /*! @decl void werror(string fmt, mixed ... args) */
  ADD_PROTOTYPE("werror", tFuncV(tStr, tOr(tVoid,tMix), tInt), OPT_SIDE_EFFECT);
  /*! @decl string read_include(string path) */
  ADD_PROTOTYPE("read_include", tFunc(tStr, tStr), 0);
  /*! @decl mixed resolv(string identifier, string|void current_file, object|void handler) */
  ADD_PROTOTYPE("resolv",
		tFunc(tStr tOr(tStr,tVoid) tOr(tObj,tVoid), tMix), 0);

  pike___master_program = end_program();
  add_program_constant("__master", pike___master_program, 0);
  /*! @endclass
   */
  /*! @endmodule Builtin
   */

  /* FIXME: */
  ADD_EFUN("replace_master", f_replace_master,
	   tFunc(tObj, tVoid), OPT_SIDE_EFFECT);
  ADD_EFUN("master", f_master,
	   tFunc(tNone, tObj), OPT_EXTERNAL_DEPEND);

  /* __master still contains a reference */
  free_program(pike___master_program);

  /* function(string,void|mixed:void) */
  ADD_EFUN("add_constant", f_add_constant,
	   tFunc(tStr tOr(tVoid,tMix),tVoid),OPT_SIDE_EFFECT);

  /* function(0=mixed ...:array(0)) */
  ADD_EFUN2("aggregate",debug_f_aggregate,
	    tOr(tAnd(tFuncV(tMix, tMix, tArr(tMix)),
		     tFuncV(tNone,tSetvar(0,tMix),tArr(tVar(0)))),
		tFunc(tNone, tLArr(tZero, tUnknown))),
	    OPT_TRY_OPTIMIZE, optimize_f_aggregate, 0);

  /* function(0=mixed ...:multiset(0)) */
  ADD_EFUN("aggregate_multiset",f_aggregate_multiset,
	   tFuncV(tNone,tSetvar(0,tMix),tSet(tVar(0))),OPT_TRY_OPTIMIZE);

  /* function(0=mixed ...:mapping(0:0)) */
  ADD_EFUN2("aggregate_mapping",f_aggregate_mapping,
	    tOr(tFunc(tNone, tMap(tUnknown, tUnknown)),
		tTransitive(tFunc(tSetvar(0, tMix) tSetvar(1, tMix),
				  tMap(tVar(0), tVar(1))),
			    tFunc(tMap(tSetvar(2, tMix), tSetvar(3, tMix))
				  tSetvar(0, tMix) tSetvar(1, tMix),
				  tMap(tOr(tVar(2), tVar(0)),
				       tOr(tVar(3), tVar(1)))))),
	    OPT_TRY_OPTIMIZE, NULL, 0);

  /* function(:mapping(string:mixed)) */
  ADD_EFUN("all_constants",f_all_constants,
	   tFunc(tNone,tMap(tStr,tMix)),OPT_EXTERNAL_DEPEND);

  /* function(:object) */
  ADD_EFUN("get_active_compiler", f_get_active_compiler,
	   tFunc(tNone, tOr(tObj, tZero)), OPT_EXTERNAL_DEPEND);

  /* function(int,void|0=mixed:array(0)) */
  ADD_EFUN("allocate", f_allocate,
	   tFuncArg(tSetvar(0, tIntPos),
		    tOr(tFunc(tNone, tLArr(tVar(0), tZero)),
			tFunc(tSetvar(1,tMix),tLArr(tVar(0), tVar(1))))), 0);

  /* function(mixed:int) */
  ADD_EFUN("arrayp", f_arrayp,tFunc(tMix,tInt01),0);

  ADD_EFUN("basename", f_basename,
	   tFunc(tNStr(tSetvar(0, tInt)), tNStr(tVar(0))), 0);

  /* function(string...:string) */
  ADD_EFUN("combine_path_nt", f_combine_path_nt,
	   tFuncV(tNone, tNStr(tSetvar(0, tInt)),
		  tNStr(tOr(tVar(0), tIntSlash))), 0);
  ADD_EFUN("combine_path_unix", f_combine_path_unix,
	   tFuncV(tNone, tNStr(tSetvar(0, tInt)),
		  tNStr(tOr(tVar(0), tIntSlash))), 0);
  ADD_EFUN("combine_path_amigaos", f_combine_path_amigaos,
	   tFuncV(tNone, tNStr(tSetvar(0, tInt)),
		  tNStr(tOr(tVar(0), tIntSlash))), 0);
#ifdef __NT__
  /* NB: cl does not like preprocessor directives in macro arguments. */
  ADD_EFUN("combine_path",
	   f_combine_path_nt,
           tFuncV(tNone, tNStr(tSetvar(0, tInt)),
                  tNStr(tOr(tVar(0), tIntSlash))), 0);
#else /* !NT */
  ADD_EFUN("combine_path",
#if defined(__amigaos__)
	   f_combine_path_amigaos,
#else
	   f_combine_path_unix,
#endif
	   tFuncV(tNone, tNStr(tSetvar(0, tInt)),
		  tNStr(tOr(tVar(0), tIntSlash))), 0);
#endif

  ADD_EFUN("compile", f_compile,
	   tFunc(tStr tOr(tObj, tVoid) tOr(tInt, tVoid) tOr(tInt, tVoid) tOr(tPrg(tObj), tVoid) tOr(tObj, tVoid) ,tPrg(tObj)),
	   OPT_EXTERNAL_DEPEND);

  /* function(1=mixed:1) */
  ADD_EFUN("copy_value",f_copy_value,tFunc(tSetvar(1,tMix),tVar(1)),0);

  /* function(string:string)|function(string,string:int) */
  ADD_EFUN("crypt",f_crypt,
           tOr(tFunc(tOr(tStr,tVoid),tStr7),tFunc(tStr tStr,tInt01)),OPT_EXTERNAL_DEPEND);

  /* function(object|void:int(0..1)) */
  ADD_EFUN("destruct",f_destruct,tFunc(tOr(tObj,tVoid),tInt01),OPT_SIDE_EFFECT);

  ADD_EFUN("dirname", f_dirname,
	   tFunc(tNStr(tSetvar(0, tInt)), tNStr(tVar(0))), 0);

  /* function(mixed,mixed:int) */
  ADD_EFUN("equal",f_equal,tFunc(tMix tMix,tInt01),OPT_TRY_OPTIMIZE);

  /* function(array(0=mixed),int|void,int|void:array(0)) */
  ADD_FUNCTION2("everynth",f_everynth,
                tFunc(tArr(tSetvar(0,tMix)) tOr(tInt1Plus,tVoid) tOr(tIntPos,tVoid),
		      tArr(tVar(0))), 0, OPT_TRY_OPTIMIZE);

  /* function(int:void) */
  ADD_EFUN("exit",f_exit,tFuncV(tInt tOr(tVoid,tStr),tOr(tVoid,tMix),tVoid),
	   OPT_SIDE_EFFECT);

  /* function(int:void) */
  ADD_EFUN("_exit",f__exit,tFunc(tInt,tVoid),OPT_SIDE_EFFECT);

  /* function(mixed:int) */
  ADD_EFUN("floatp",  f_floatp,tFunc(tMix,tInt01),OPT_TRY_OPTIMIZE);

  /* function(mixed:int) */
  ADD_EFUN("functionp",  f_functionp,tFunc(tMix,tInt01),OPT_TRY_OPTIMIZE);

  /* function(mixed:int) */
  ADD_EFUN("callablep",  f_callablep,tFunc(tMix,tInt01),OPT_TRY_OPTIMIZE);

  /* function(string,string:int(0..1))|function(string,string*:array(string)) */
  ADD_EFUN("glob",f_glob,
           tOr3(tFunc(tStr tStr,tInt01),
                tFunc(tArr(tSetvar(0, tStr)) tStr, tOr(tVar(0), tZero)),
                tFunc(tOr(tStr,tArr(tStr)) tSetvar(1,tArr(tStr)),tVar(1))),
	   OPT_TRY_OPTIMIZE);

  /* function(string,int|void:int) */
  ADD_EFUN("hash_7_0",f_hash_7_0,
           tFunc(tStr tOr(tInt1Plus,tVoid),tIntPos),
	   OPT_TRY_OPTIMIZE);

  ADD_EFUN("hash_7_4",f_hash_7_4,
           tFunc(tStr tOr(tInt1Plus,tVoid),tIntPos),
	   OPT_TRY_OPTIMIZE);

  ADD_EFUN("hash_8_0", f_hash_8_0,
           tFunc(tStr tOr(tInt1Plus,tVoid),tIntPos),
	   OPT_TRY_OPTIMIZE);

  ADD_EFUN("hash",f_hash,
	   tFunc(tStr tOr(tInt1Plus,tVoid),tIntPos), OPT_TRY_OPTIMIZE);

  ADD_EFUN("hash_value",f_hash_value,tFunc(tMix,tIntPos),OPT_TRY_OPTIMIZE);

  ADD_EFUN("indices",f_indices,
           tOr6(tFunc(tLArr(tSetvar(1, tIntPos), tMix),
                      tLArr(tVar(1), tRangeInt(tZero, tDecInt(tVar(1))))),
		tFunc(tMap(tSetvar(1, tMix), tMix), tArr(tVar(1))),
		tFunc(tSet(tSetvar(1, tMix)), tArr(tVar(1))),
                tFunc(tLStr(tSetvar(1, tIntPos), tInt),
                      tLArr(tVar(1), tRangeInt(tZero, tDecInt(tVar(1))))),
		tFunc(tPrg(tObj), tArr(tStr)),
		tFuncArg(tSetvar(2, tObj),
			 tOr(tFindLFun(tVar(2), "_indices"),
			     tIfnot(tThreshold(tFindLFun(tVar(2), "_indices")),
				    tFunc(tNone, tArr(tStr)))))),
	   OPT_TRY_OPTIMIZE);

  ADD_EFUN2("undefinedp", f_undefinedp, tFunc(tMix,tInt01), OPT_TRY_OPTIMIZE,
            0, generate_undefinedp);
  ADD_EFUN2("destructedp", f_destructedp, tFunc(tMix,tInt01), OPT_TRY_OPTIMIZE,
            0, generate_destructedp);

  /* function(mixed:int) */
  ADD_EFUN("intp", f_intp,tFunc(tMix,tInt01),OPT_TRY_OPTIMIZE);

  /* function(mixed:int) */
  ADD_EFUN("multisetp", f_multisetp,tFunc(tMix,tInt01),OPT_TRY_OPTIMIZE);

  /* function(string:string)|function(int:int) */
  ADD_EFUN("lower_case",f_lower_case,
           tOr(tAnd(tOr5(tFunc(tStr7, tStr7),
                         tFunc(tNStr(tOr(tRangeInt(tInt128, tInt8bit),
                                         tInt376)),
                               tStr8),
                         tFunc(tNStr(tOr(tRangeInt(tInt256, tDecInt(tInt376)),
                                         tRangeInt(tIncInt(tInt376),
                                                   tInt16bit))),
                               tStr16),
                         tFunc(tNStr(tRangeInt(tInt65536, tInt)),
                               tNStr(tRangeInt(tInt65536, tInt))),
                         tFunc(tNStr(tIntMinus), tNStr(tIntMinus))),
                    tFunc(tLStr(tSetvar(0, tIntPos), tInt),
                          tLStr(tVar(0), tInt))),
               tOr5(tFunc(tInt7bit, tInt7bit),
                    tFunc(tOr(tRangeInt(tInt128, tInt8bit), tInt376),
                          tInt8bit),
                    tFunc(tOr(tRangeInt(tInt256, tDecInt(tInt376)),
                                    tRangeInt(tIncInt(tInt376), tInt16bit)),
                          tInt16bit),
                    tFunc(tRangeInt(tInt65536, tInt),
                          tRangeInt(tInt65536, tInt)),
                    tFunc(tIntMinus, tIntMinus))),
           OPT_TRY_OPTIMIZE);

  /* function(mixed:int) */
  ADD_EFUN("mappingp",f_mappingp,tFunc(tMix,tInt01),OPT_TRY_OPTIMIZE);

  /* function(1=mixed,int:1) */
  ADD_EFUN("set_weak_flag",f_set_weak_flag,
	   tFunc(tSetvar(1,tMix) tInt,tVar(1)),OPT_SIDE_EFFECT);

  ADD_INT_CONSTANT("PIKE_WEAK_INDICES", PIKE_WEAK_INDICES, 0);
  ADD_INT_CONSTANT("PIKE_WEAK_VALUES", PIKE_WEAK_VALUES, 0);

  /* function(mixed:program|function) */
  ADD_EFUN2("object_program", f_object_program,
	    tOr(tFunc(tSetvar(0, tObj), tPrg(tVar(0))),
		tFunc(tNot(tObj), tZero)),
	    OPT_TRY_OPTIMIZE, fix_object_program_type, 0);

  /* function(mixed:int) */
  ADD_EFUN("objectp", f_objectp,tFunc(tMix,tInt01),0);

  /* function(mixed:int) */
  ADD_EFUN("programp",f_programp,tFunc(tMix,tInt01),0);

  /* function(:int) */
  ADD_EFUN("query_num_arg",f_query_num_arg,
	   tFunc(tNone,tInt),OPT_EXTERNAL_DEPEND);

  /* function(int:void) */
  ADD_EFUN("random_seed",f_random_seed,
           tAttr("deprecated",tFunc(tInt,tVoid)),OPT_SIDE_EFFECT);

  ADD_EFUN2("replace", f_replace,
	    tOr5(tFunc(tStr tStr tStr,tStr),
		 tFunc(tStr tArr(tStr) tOr(tArr(tStr), tStr), tStr),
		 tFunc(tStr tMap(tStr,tStr),tStr),
		 tFunc(tSetvar(0,tArray) tMix tMix,tVar(0)),
		 tFunc(tSetvar(1,tMapping) tMix tMix,tVar(1))),
	    OPT_TRY_OPTIMIZE, optimize_replace, 0);

  ADD_EFUN("reverse",f_reverse,
	   tOr4(tFunc(tInt tOr(tVoid, tInt) tOr(tVoid, tInt), tInt),
		tFunc(tSetvar(0, tStr) tOr(tVoid, tInt) tOr(tVoid, tInt),
		      tVar(0)),
		tFunc(tSetvar(0, tArray) tOr(tVoid, tInt) tOr(tVoid, tInt),
		      tVar(0)),
		tFuncArg(tSetvar(0, tObj), tFindLFun(tVar(0), "_reverse"))), 0);

  /* function(mixed,array:array) */
  ADD_EFUN("rows",f_rows,
	   tOr6(tFunc(tMap(tSetvar(0,tMix),tSetvar(1,tMix)) tArr(tVar(0)),
		      tArr(tOr(tVar(1), tZero))),
		tFunc(tSet(tSetvar(0,tMix)) tArr(tVar(0)), tArr(tInt01)),
		tFunc(tString tArr(tInt), tArr(tInt)),
		tFunc(tArr(tSetvar(0,tMix)) tArr(tInt), tArr(tVar(1))),
		tFunc(tArray tArr(tNot(tInt)), tArray),
		tFunc(tOr4(tObj,tFunction,tPrg(tObj),tInt) tArray, tArray)), 0);

  /* FIXME: Is the third arg a good idea when the first is a mapping? */
  /* FIXME: Improve type for the fallback to iterator object case. */
  ADD_EFUN("search",f_search,
	   tOr4(tFunc(tStr tOr(tStr,tInt) tOr(tVoid,tInt) tOr(tVoid,tInt),
		      tInt),
		tFunc(tArr(tSetvar(0,tMix)) tVar(0) tOr(tVoid,tInt) tOr(tVoid,tInt),
		      tInt),
		tFunc(tMap(tSetvar(1,tMix),tSetvar(2,tMix)) tVar(2)
                      tOr(tVoid,tVar(1)), tVar(1)),
		tFuncArg(tSetvar(3, tObj),
			 tOr(tFindLFun(tVar(3), "_search"),
			     tIfnot(tFindLFun(tVar(3), "_search"),
				    tFuncV(tMix, tMix, tMix))))),
	   0);

  ADD_EFUN2("has_prefix", f_has_prefix, tFunc(tOr(tStr,tObj) tStr,tInt01),
	    OPT_TRY_OPTIMIZE, 0, 0);

  ADD_EFUN2("has_suffix", f_has_suffix, tFunc(tStr tStr,tInt01),
	    OPT_TRY_OPTIMIZE, 0, 0);

  ADD_EFUN("has_index",f_has_index,
	   tOr5(tFunc(tStr tIntPos, tInt01),
		tFunc(tArray tIntPos, tInt01),
		tFunc(tSet(tSetvar(0,tMix)) tVar(0), tInt01),
		tFunc(tMap(tSetvar(1,tMix),tMix) tVar(1), tInt01),
		tFunc(tObj tMix, tInt01)),
	   OPT_TRY_OPTIMIZE);

  ADD_EFUN("has_value",f_has_value,
	   tOr5(tFunc(tStr tOr(tStr, tInt), tInt01),
		tFunc(tArr(tSetvar(0,tMix)) tVar(0), tInt01),
		tFunc(tMultiset tInt, tInt01),
		tFunc(tMap(tMix,tSetvar(1,tMix)) tVar(1), tInt01),
		tFunc(tObj tMix, tInt01)),
	   OPT_TRY_OPTIMIZE);

  /* function(float|int,int|void:void) */
  ADD_EFUN2("sleep", f_sleep,
            tFunc(tOr(tFlt,tInt) tOr(tInt,tVoid),tVoid),
            OPT_SIDE_EFFECT | OPT_TRY_OPTIMIZE, optimize_sleep, 0);
  ADD_EFUN("delay", f_delay,
	   tFunc(tOr(tFlt,tInt) tOr(tInt,tVoid),tVoid),OPT_SIDE_EFFECT);

  /* function(array(0=mixed),array(mixed)...:array(0)) */
  ADD_EFUN("sort",f_sort,
	   tFuncV(tArr(tSetvar(0,tMix)),tArr(tMix),tArr(tVar(0))),
	   OPT_SIDE_EFFECT);

  /* function(array(0=mixed)...:array(0)) */
  ADD_FUNCTION2("splice",f_splice,
		tFuncV(tNone,tArr(tSetvar(0,tMix)),tArr(tVar(0))), 0,
		OPT_TRY_OPTIMIZE);

  /* function(array:array) */
  ADD_FUNCTION2("uniq_array", f_uniq_array,
		tFunc(tArr(tSetvar(0,tMix)), tArr(tVar(0))), 0,
		OPT_TRY_OPTIMIZE);

  /* function(mixed:int) */
  ADD_EFUN("stringp", f_stringp, tFunc(tMix,tInt01), 0);

  ADD_EFUN2("this_object", f_this_object,tFunc(tOr(tVoid,tIntPos),tObj),
	    OPT_EXTERNAL_DEPEND, optimize_this_object, generate_this_object);

  /* function(mixed:void) */
  ADD_EFUN("throw",f_throw,tFunc(tMix,tOr(tMix,tVoid)),OPT_SIDE_EFFECT);

  /* function(void|int(0..1):int(2..))|function(int(2..):float) */
  ADD_EFUN("time",f_time,
	   tOr(tFunc(tOr(tVoid,tInt01),tInt2Plus),
	       tFunc(tInt2Plus,tFlt)),
	   OPT_SIDE_EFFECT);

  /* function(array(0=mixed):array(0)) */
  ADD_FUNCTION2("transpose",f_transpose,
		tFunc(tArr(tSetvar(0,tMix)),tArr(tVar(0))), 0,
		OPT_TRY_OPTIMIZE);

  /* function(string:string)|function(int:int) */
  ADD_EFUN("upper_case",f_upper_case,
           tOr(tAnd(tOr6(tFunc(tStr7, tStr7),
                         tFunc(tNStr(tRangeInt(tInt128, tInt254)),
                               tNStr(tRangeInt(tInt128, tInt254))),
                         tFunc(tNStr(tInt255), tNStr(tInt376)),
                         tFunc(tNStr(tRangeInt(tInt256, tInt16bit)),
                               tNStr(tRangeInt(tInt256, tInt16bit))),
                         tFunc(tNStr(tRangeInt(tInt65536, tInt)),
                               tNStr(tRangeInt(tInt65536, tInt))),
                         tFunc(tNStr(tIntMinus), tNStr(tIntMinus))),
                    tFunc(tLStr(tSetvar(0, tIntPos), tInt),
                          tLStr(tVar(0), tInt))),
               tOr6(tFunc(tInt7bit, tInt7bit),
                    tFunc(tRangeInt(tInt128, tInt254),
                          tRangeInt(tInt128, tInt254)),
                    tFunc(tInt255, tInt376),
                    tFunc(tRangeInt(tInt256, tInt16bit),
                          tRangeInt(tInt256, tInt16bit)),
                    tFunc(tRangeInt(tInt65536, tInt),
                          tRangeInt(tInt65536, tInt)),
                    tFunc(tIntMinus, tIntMinus))),
           OPT_TRY_OPTIMIZE);

  /* function(string|multiset:array(int))|function(array(0=mixed)|mapping(mixed:0=mixed)|object|program:array(0)) */
  ADD_EFUN("values",f_values,
	   tOr6(tFunc(tMultiset, tArr(tInt01)),
	        tFunc(tNStr(tSetvar(0, tInt)), tArr(tVar(0))),
	        tFunc(tSetvar(1, tArray), tVar(1)),
	        tFunc(tMap(tMix, tSetvar(0, tMix)), tArr(tVar(0))),
	        tFunc(tPrg(tObj), tArr(tMix)),
	        tFuncArg(tSetvar(2, tObj),
			 tOr(tFindLFun(tVar(2), "_values"),
			     tIfnot(tThreshold(tFindLFun(tVar(2), "_values")),
				    tFunc(tNone, tArr(tMix)))))),
	   0);

  /* function(string|multiset(array(int))|function(array(0=mixed)|mapping(mixed:0=mixed)|object|program:array(0)) */
  ADD_EFUN2("types", f_types,
	    tOr3(tFunc(tOr3(tNStr(tSetvar(0,tInt)),
			    tArr(tSetvar(0,tMix)),
			    tMap(tMix,tSetvar(0,tMix))),
		       tArr(tType(tVar(0)))),
		 tFunc(tMultiset, tArr(tType(tInt1))),
		 tFunc(tOr(tObj,tPrg(tObj)), tArr(tType(tMix)))),0,NULL,0);

  /* function(object|program, int(0..1)|void:array(multiset)) */
  ADD_EFUN2("annotations", f_annotations,
	    tFunc(tOr(tObj,tPrg(tObj) tOr(tInt01,tVoid)), tArr(tSet(tMix))),0,NULL,0);
		
  ADD_FUNCTION2("direct_program_annotations", f_direct_program_annotations,
	    tFunc(tOr(tObj,tPrg(tObj) tOr(tInt01,tVoid)), tSet(tMix)),0,OPT_TRY_OPTIMIZE);
  /* function(mixed:int) */
  ADD_EFUN2("zero_type",f_zero_type,tFunc(tMix,tInt01),0,0,generate_zero_type);

  /* function(string,string:array) */
  ADD_EFUN("array_sscanf", f_sscanf,
	   tFunc(tAttr("sscanf_input", tSetvar(0, tStr))
		 tAttr("sscanf_format", tStr),
		 tArr(tAttr("sscanf_args", tMix))), OPT_TRY_OPTIMIZE);

  ADD_EFUN("array_sscanf_80", f_sscanf_80,
	   tFunc(tAttr("sscanf_input", tSetvar(0, tStr))
		 tAttr("sscanf_80_format", tStr),
		 tArr(tAttr("sscanf_args", tMix))), OPT_TRY_OPTIMIZE);

  ADD_EFUN("__handle_sscanf_format", f___handle_sscanf_format,
	   tFunc(tStr tOr(tStr, tZero) tType(tMix) tType(tMix), tType(tMix)),
	   0);

  /* Some Wide-string stuff */

  /* function(string,int(0..2)|void:string(0..255)) */
  ADD_EFUN("string_to_unicode", f_string_to_unicode,
	   tFunc(tStr tOr(tInt02,tVoid),tStr8), OPT_TRY_OPTIMIZE);

  /* function(string(0..255),int(0..2)|void:string) */
  ADD_EFUN("unicode_to_string", f_unicode_to_string,
	   tFunc(tStr8 tOr(tInt02,tVoid),tStr), OPT_TRY_OPTIMIZE);

  /* function(string,int|void:utf8_string) */
  ADD_EFUN("string_to_utf8", f_string_to_utf8,
	   tFunc(tStr tOr(tInt,tVoid),tUtf8Str), OPT_TRY_OPTIMIZE);

  ADD_EFUN("string_filter_non_unicode", f_string_filter_non_unicode,
	   tFunc(tStr tOr(tInt,tVoid),tStr8), OPT_TRY_OPTIMIZE);

  /* function(utf8_string,int|void:string) */
  ADD_EFUN("utf8_to_string", f_utf8_to_string,
	   tFunc(tUtf8Str tOr(tInt,tVoid),tStr), OPT_TRY_OPTIMIZE);

  /* function(string(8bit),int|void:int(0..1)) */
  ADD_EFUN("validate_utf8", f_validate_utf8,
	   tFunc(tStr8 tOr(tInt,tVoid),tInt01), OPT_TRY_OPTIMIZE);


  ADD_EFUN("__parse_pike_type", f_parse_pike_type,
	   tFunc(tStr8,tStr8),OPT_TRY_OPTIMIZE);

  ADD_EFUN("deprecated_typep", f_deprecated_typep,
           tFunc(tType(tMix),tInt), OPT_TRY_OPTIMIZE);

  ADD_EFUN("typeof_identifier", f_typeof_identifier,
           tFunc(tPrg(tObj) tStr, tOr(tType(tMix), tZero)), OPT_TRY_OPTIMIZE);

  ADD_EFUN("__soft_cast", f___soft_cast,
	   tFunc(tSetvar(0, tType(tMix)) tSetvar(1, tType(tMix)),
		 tOr(tAnd(tVar(0), tVar(1)), tZero)),
	   OPT_TRY_OPTIMIZE);

  ADD_EFUN("__low_check_call", f___low_check_call,
	   tFunc(tType(tCallable) tType(tMix) tOr(tInt, tVoid)
		 tOr(tMapping, tVoid) tOr(tMix, tVoid),
		 tOr(tType(tCallable), tZero)),
	   OPT_TRY_OPTIMIZE);

  /* FIXME: Could have a stricter type. */
  ADD_EFUN("__get_return_type", f___get_return_type,
	   tFunc(tType(tCallable) tOr(tMapping, tVoid),
		 tOr(tType(tMix), tZero)),
	   OPT_TRY_OPTIMIZE);

  /* FIXME: Could have a stricter type. */
  ADD_EFUN("__get_first_arg_type", f___get_first_arg_type,
	   tFunc(tType(tCallable), tOr(tType(tMix), tZero)),
	   OPT_TRY_OPTIMIZE);

  ADD_EFUN("__get_type_attributes", f___get_type_attributes,
	   tFunc(tType(tMix), tArr(tString)),
	   OPT_TRY_OPTIMIZE);

  /* function(int:mapping(string:int)) */
  ADD_EFUN("localtime",f_localtime,
	   tFunc(tInt,tMap(tStr,tInt)),OPT_EXTERNAL_DEPEND);

  /* function(int:mapping(string:int)) */
  ADD_EFUN("gmtime",f_gmtime,tFunc(tInt,tMap(tStr,tInt)),OPT_TRY_OPTIMIZE);

  /* function(int,int,int,int,int,int,int,void|int:int)|function(object|mapping(string:int):int) */
  ADD_EFUN("mktime",f_mktime,
	   tOr(tFunc(tInt tInt tInt tInt tInt tInt
		     tOr(tVoid,tInt) tOr(tVoid,tInt),tInt),
	       tFunc(tOr(tObj, tMap(tStr, tInt)),tInt)),OPT_TRY_OPTIMIZE);

  /* function(string, string:mapping(string:int)) */
  ADD_EFUN("strptime", f_strptime,
           tFunc(tStr tStr, tMap(tStr, tInt)), OPT_TRY_OPTIMIZE);

  /* function(string,mapping(string:int):string) */
  ADD_EFUN("strftime", f_strftime,
           tFunc(tStr tMap(tStr,tInt), tStr), OPT_TRY_OPTIMIZE);

  /* function(:void) */
  ADD_EFUN("_verify_internals",f__verify_internals,
	   tFunc(tNone,tVoid),OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);

  /* function(:mapping(string:int)) */
  ADD_EFUN("_memory_usage",f__memory_usage,
	   tFunc(tNone,tMap(tStr,tInt)),OPT_EXTERNAL_DEPEND);

  ADD_EFUN("_size_object",f__size_object,
	   tFunc(tObj,tInt),OPT_EXTERNAL_DEPEND);


  /* function(:int) */
  ADD_EFUN("gc", f_gc, tFunc(tOr(tMix, tVoid), tInt), OPT_SIDE_EFFECT);

  /* function(:string) */
  ADD_EFUN("version", f_version,tFunc(tNone,tStr), OPT_TRY_OPTIMIZE);

  /* Note: The last argument to the encode and decode functions is
   * intentionally not part of the prototype, to keep it free for
   * other uses in the future. */

  /* function(mixed,void|object,void|object|int:string) */
  ADD_EFUN("encode_value", f_encode_value,
	   tFunc(tMix tOr(tVoid,tObj) tOr3(tVoid, tIntPos, tObj),tStr8),
	   OPT_TRY_OPTIMIZE);

  /* function(mixed,void|object,void|object|int:string) */
  ADD_EFUN("encode_value_canonic", f_encode_value_canonic,
	   tFunc(tMix tOr(tVoid,tObj) tOr3(tVoid, tIntPos, tObj),tStr8),
	   OPT_TRY_OPTIMIZE);

  /* function(string,void|object,void|object|int:mixed) */
  ADD_EFUN("decode_value", f_decode_value,
           tFunc(tStr8 tOr3(tVoid,tObj,tInt_10) tOr3(tVoid,tIntPos,tObj),tMix),
	   OPT_TRY_OPTIMIZE);

  /* function(object,string:int) */
  ADD_EFUN("object_variablep", f_object_variablep,
	   tFunc(tObj tStr,tInt), OPT_EXTERNAL_DEPEND);

  /* function(array(mapping(int:mixed)):array(int)) */
  ADD_FUNCTION2("interleave_array", f_interleave_array,
		tFunc(tArr(tMap(tInt, tMix)), tArr(tInt)), 0,
		OPT_TRY_OPTIMIZE);

  /* function(array(0=mixed),array(1=mixed):array(array(array(0)|array(1))) */
  ADD_FUNCTION2("diff", f_diff,
		tFunc(tArr(tSetvar(0,tMix)) tArr(tSetvar(1,tMix)),
		      tArr(tArr(tOr(tArr(tVar(0)),tArr(tVar(1)))))), 0,
		OPT_TRY_OPTIMIZE);

  ADD_FUNCTION2("permute", f_permute, tFunc(tArray tIntPos,tArray), 0,
		OPT_TRY_OPTIMIZE);

  /* function(array,array:array(int)) */
  ADD_FUNCTION2("diff_longest_sequence", f_diff_longest_sequence,
		tFunc(tArray tArray,tArr(tInt)), 0, OPT_TRY_OPTIMIZE);

  /* function(array,array:array(int)) */
  ADD_FUNCTION2("diff_dyn_longest_sequence", f_diff_dyn_longest_sequence,
		tFunc(tArray tArray,tArr(tInt)), 0, OPT_TRY_OPTIMIZE);

  /* function(array,array:array(array)) */
  ADD_FUNCTION2("diff_compare_table", f_diff_compare_table,
		tFunc(tArray tArray, tArr(tArr(tInt))), 0, OPT_TRY_OPTIMIZE);

  /* function(array:array(int)) */
  ADD_FUNCTION2("longest_ordered_sequence", f_longest_ordered_sequence,
		tFunc(tArray,tArr(tInt)), 0, OPT_TRY_OPTIMIZE);

  /* Type variable use in map():
   *
   * 0: Accumulated argument types for callback function.
   * 1: Iterator value type (aka indices). First arg to callback function.
   * 2: Result value type. Return type for callback function.
   * 3: Mapping index type when looping over mapping.
   * 4: Callback function continuation type for optional arguments and result.
   */
#define tMapStuffLow(IN,SUB,OUTSET,OUTPROG,OUTARR,OUTMAP) \
  tOr3( tFuncV(IN tSet(tMix),tMix,OUTSET), \
	tFuncV(IN tMap(tMix, tSetvar(2,tMix)), tMix, OUTMAP), \
        tFuncV(IN tArray, tMix, OUTARR) )

#define tMapStuff(IN,SUB,OUTFUN,OUTSET,OUTPROG,OUTARR,OUTMAP) \
  tOr( tFuncV(IN tFuncV(SUB, tSetvar(0, tAny), tSetvar(2,tAny)),     \
	      tVar(0), OUTFUN),					     \
       tMapStuffLow(IN,SUB,OUTSET,OUTPROG,OUTARR,OUTMAP))

  ADD_EFUN2("map", f_map,
	    tOr9(tFuncArg(tArr(tSetvar(1,tMix)),
			  tFuncArg(tFuncArg(tVar(1),
					    tSetvar(4,
						    tFuncV(tNone, tUnknown, tMix))),
				   tArr(tVar(4)))),
		 tMapStuffLow(tArr(tSetvar(1,tMix)),tVar(1),
			      tArr(tInt01),
			      tArr(tObj),
			      tArr(tArr(tMix)),
			      tArr(tOr(tInt0,tVar(2)))),

		 tMapStuff(tMap(tSetvar(3,tMix),tSetvar(1,tMix)),tVar(1),
			   tMap(tVar(3),tVar(2)),
			   tMap(tVar(3),tInt01),
			   tMap(tVar(3),tObj),
			   tMap(tVar(3),tArr(tMix)),
			   tMap(tVar(3),tOr(tInt0,tVar(2)))),

 		 tMapStuff(tSet(tSetvar(1,tMix)),tVar(1),
			   tSet(tVar(2)),
			   tSet(tInt01),
			   tSet(tObj),
			   tSet(tArr(tMix)),
			   tSet(tOr(tInt0,tVar(2)))),

		 tMapStuff(tAnd(tNot(tArray),tOr(tPrg(tObj),tFunction)),tMix,
			   tMap(tStr,tVar(2)),
			   tMap(tStr,tInt01),
			   tMap(tStr,tObj),
			   tMap(tStr,tArr(tMix)),
			   tMap(tStr,tOr(tInt0,tVar(2)))),

		 tOr3( tFuncV(tNStr(tSetvar(0, tInt))
			      tFuncV(tVar(0), tUnknown, tSetvar(1, tInt)),
			      tMix, tNStr(tVar(1))),
		       tFuncV(tString tSet(tMix),tMix,tString),
		       tFuncV(tString tMap(tMix,tInt), tMix, tString) ),

		 tOr4 (tFuncV(tArr(tStringIndicable) tString,tMix,tArray),
		       tFuncV(tMap(tSetvar(3,tMix),tStringIndicable) tString,tMix,
			      tMap(tVar(3),tMix)),
		       tFuncV(tSet(tStringIndicable) tString,tMix,tSet(tMix)),
		       tFuncV(tOr(tPrg(tObj),tFunction) tString,tMix,tMapping)),

		 tFunc(tSetvar(0, tOr4(tArray, tString,
				       tMap(tMix, tMix), tSet(tMix))),
		       tVar(0)),

		 tFuncV(tObj,tMix,tMix) ),
	    OPT_TRY_OPTIMIZE, fix_map_node_info, 0);

#if 1
  ADD_EFUN2("filter", f_filter,
	    tOr5(tFuncV(tSetvar(1,tOr(tMapping,tMultiset)),
			tMixed,
			tVar(1)),
		 tFuncV(tNStr(tSetvar(0,tInt)), tMixed, tNStr(tVar(0))),
		 tFuncV(tArr(tSetvar(0,tMixed)), tMixed, tArr(tVar(0))),
		 tFuncV(tOr(tPrg(tObj),tFunction),tMixed,tMap(tString,tMix)),
		 tFuncV(tObj,tMix,tMix) ) ,
	    OPT_TRY_OPTIMIZE, fix_map_node_info, 0);
#else
  ADD_EFUN2("filter", f_filter,
	    tOr3(tFuncV(tSetvar(1,tOr4(tArray,tMapping,tMultiset,tString)),
			tOr5(tFuncV(tMix, tMix, tAnd(tInt01,tNot(tVoid))),
			     tArray, tMapping, tMultiset, tString),
			tVar(1)),
		 tFuncV(tOr(tPrg(tObj),tFunction),tMixed,tMap(tString,tMix)),
		 tFuncV(tObj,tMix,tMix) ) ,
	    OPT_TRY_OPTIMIZE, fix_map_node_info, 0);
#endif /* 1 */

  ADD_EFUN("enumerate",f_enumerate,
	   tOr8(tFunc(tIntPos,tArr(tInt)),
		tFunc(tIntPos tInt,tArr(tInt)),
		tFunc(tIntPos tInt tOr(tVoid,tInt),tArr(tInt)),
		tFunc(tIntPos tFloat tOr3(tVoid,tInt,tFloat),tArr(tFloat)),
		tFunc(tIntPos tOr(tInt,tFloat) tFloat,tArr(tFloat)),
		tFunc(tIntPos tMix tObj,tArr(tVar(1))),
		tFunc(tIntPos tObj tOr(tVoid,tMix),tArr(tVar(1))),
		tFunc(tIntPos tMix tMix
		      tFuncV(tNone,tMix,tSetvar(1,tMix)),tArr(tVar(1)))),
	   OPT_TRY_OPTIMIZE);

  ADD_FUNCTION2("inherit_list", f_inherit_list,
		tFunc(tOr(tObj,tPrg(tObj)),tArr(tPrg(tObj))), 0,
                OPT_TRY_OPTIMIZE);

  ADD_FUNCTION2("program_identifier_defined", f_program_identifier_defined,
		tFunc(tOr(tObj,tPrg(tObj)) tString, tOr(tString, tZero)), 0,
                OPT_TRY_OPTIMIZE);

  ADD_FUNCTION2("function_defined", f_function_defined,
	       tFunc(tFunction,tString), 0, OPT_TRY_OPTIMIZE);

  ADD_EFUN("_gdb_breakpoint", pike_gdb_breakpoint,
	   tFunc(tNone,tVoid), OPT_SIDE_EFFECT);

  ADD_EFUN("_gc_status",f__gc_status,
	   tFunc(tNone,tMap(tString,tOr(tInt,tFloat))),
	   OPT_EXTERNAL_DEPEND);

  ADD_FUNCTION ("implicit_gc_real_time", f_implicit_gc_real_time,
		tFunc(tOr(tInt,tVoid),tInt), OPT_EXTERNAL_DEPEND);

  ADD_FUNCTION ("count_memory", f_count_memory,
		tFuncV(tOr(tInt,tMap(tString,tInt)), tMixed, tInt), 0);
  ADD_FUNCTION("identify_cycle", f_identify_cycle,
	       tFunc(tOr7(tArray,tMultiset,tMapping,tObj,tPrg(tObj),
			  tString,tType(tMix)),
		     tArr(tOr7(tArray,tMultiset,tMapping,tObj,tPrg(tObj),
			       tString,tType(tMix)))), 0);

  ADD_INT_CONSTANT ("NATIVE_INT_MAX", MAX_INT_TYPE, 0);
  ADD_INT_CONSTANT ("NATIVE_INT_MIN", MIN_INT_TYPE, 0);

  /* Maybe make PIKEFLOAT_MANT_DIG, PIKEFLOAT_MIN_EXP and
   * PIKEFLOAT_MAX_EXP available, but do we have to export FLT_RADIX
   * too? It'd be nice to always assume it's 2 to save the pike
   * programmer from that headache. */
  ADD_INT_CONSTANT ("FLOAT_DIGITS_10", PIKEFLOAT_DIG, 0);
  ADD_INT_CONSTANT ("FLOAT_MIN_10_EXP", PIKEFLOAT_MIN_10_EXP, 0);
  ADD_INT_CONSTANT ("FLOAT_MAX_10_EXP", PIKEFLOAT_MAX_10_EXP, 0);
  ADD_FLOAT_CONSTANT ("FLOAT_MAX", PIKEFLOAT_MAX, 0);
  ADD_FLOAT_CONSTANT ("FLOAT_MIN", PIKEFLOAT_MIN, 0);
  ADD_FLOAT_CONSTANT ("FLOAT_EPSILON", PIKEFLOAT_EPSILON, 0);

#ifdef WITH_DOUBLE_PRECISION_SVALUE
  ADD_INT_CONSTANT("__DOUBLE_PRECISION_FLOAT__",1,0);
#else
#ifdef WITH_LONG_DOUBLE_PRECISION_SVALUE
  ADD_INT_CONSTANT("__LONG_DOUBLE_PRECISION_FLOAT__",1,0);
#else
  ADD_INT_CONSTANT("__FLOAT_PRECISION_FLOAT__",1,0);
#endif
#endif

  ADD_INT_CONSTANT ("DESTRUCT_EXPLICIT", DESTRUCT_EXPLICIT, 0);
  ADD_INT_CONSTANT ("DESTRUCT_NO_REFS", DESTRUCT_NO_REFS, 0);
  ADD_INT_CONSTANT ("DESTRUCT_GC", DESTRUCT_GC, 0);
  ADD_INT_CONSTANT ("DESTRUCT_CLEANUP", DESTRUCT_CLEANUP, 0);

  ADD_INT_CONSTANT("LOWEST_COMPAT_MAJOR", LOWEST_COMPAT_MAJOR, 0);
  ADD_INT_CONSTANT("LOWEST_COMPAT_MINOR", LOWEST_COMPAT_MINOR, 0);
}

void exit_builtin_efuns(void)
{
  free_callback_list(&memory_usage_callback);
}
